/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
 * Additions by Chris Urmson <curmson@ri.cmu.edu>
 * Additions by Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "config.h"
#include "platform.h"
#include "internal.h"
#include "linux.h"
#include "utils.h"
#include "log.h"


platform_t *
platform_new (void)
{
  platform_t * p = calloc (1, sizeof (platform_t));
  return p;
}
void
platform_free (platform_t * p)
{
  free (p);
}

struct _platform_device_t {
  uint32_t config_rom[256];
  int num_quads;
  int port, node, generation;
};

platform_device_list_t *
platform_get_device_list (platform_t * p)
{
  platform_device_list_t * list;
  uint32_t allocated_size = 64;
  raw1394handle_t handle;
  int num_ports, i;

  handle = raw1394_new_handle ();
  if (!handle)
    return NULL;

  num_ports = raw1394_get_port_info (handle, NULL, 0);
  raw1394_destroy_handle (handle);

  list = calloc (1, sizeof (platform_device_list_t));
  if (!list)
    return NULL;
  list->devices = malloc(allocated_size * sizeof(platform_device_t *));
  if (!list->devices) {
    free (list);
    return NULL;
  }

  for (i = 0; i < num_ports; i++) {
    int num_nodes, j;

    handle = raw1394_new_handle_on_port (i);
    if (!handle)
      continue;

    num_nodes = raw1394_get_nodecount (handle);
    for (j = 0; j < num_nodes; j++) {
      platform_device_t * device;
      uint32_t quad;
      int k;

      if (raw1394_read (handle, 0xFFC0 | j, CONFIG_ROM_BASE + 0x400,
            4, &quad) < 0)
        continue;

      device = malloc (sizeof (platform_device_t));
      if (!device)
        continue;

      device->config_rom[0] = ntohl (quad);
      device->port = i;
      device->node = j;
      device->generation = raw1394_get_generation (handle);
      for (k = 1; k < 256; k++) {
        if (raw1394_read (handle, 0xFFC0 | j, CONFIG_ROM_BASE + 0x400 + 4*k,
              4, &quad) < 0)
          break;
        device->config_rom[k] = ntohl (quad);
      }
      device->num_quads = k;

      list->devices[list->num_devices] = device;
      list->num_devices++;

      if (list->num_devices >= allocated_size) {
        allocated_size += 64;
        list->devices = realloc (list->devices,
            allocated_size * sizeof (platform_device_t *));
        if (!list->devices)
          return NULL;
      }
    }
    raw1394_destroy_handle (handle);
  }

  return list;
}

void
platform_free_device_list (platform_device_list_t * d)
{
  int i;
  for (i = 0; i < d->num_devices; i++)
    free (d->devices[i]);
  free (d->devices);
  free (d);
}

int
platform_device_get_config_rom (platform_device_t * device,
    uint32_t * quads, int * num_quads)
{
  if (*num_quads > device->num_quads)
    *num_quads = device->num_quads;

  memcpy (quads, device->config_rom, *num_quads * sizeof (uint32_t));
  return 0;
}

platform_camera_t *
platform_camera_new (platform_t * p, platform_device_t * device,
    uint32_t unit_directory_offset)
{
  platform_camera_t * camera;
  raw1394handle_t handle;

  handle = raw1394_new_handle_on_port (device->port);
  if (!handle)
    return NULL;

  if (device->generation != raw1394_get_generation (handle)) {
    dc1394_log_error("generation has changed since bus was scanned\n",NULL);
    raw1394_destroy_handle (handle);
    return NULL;
  }

  camera = calloc (1, sizeof (platform_camera_t));
  camera->handle = handle;
  camera->port = device->port;
  camera->node = device->node;
  return camera;
}

void platform_camera_free (platform_camera_t * cam)
{
  if (cam->capture.dma_device_file != NULL) {
    free (cam->capture.dma_device_file);
    cam->capture.dma_device_file = NULL;
  }
    
  raw1394_destroy_handle (cam->handle);
  free (cam);
}

void
platform_camera_set_parent (platform_camera_t * cam,
        dc1394camera_t * parent)
{
  cam->camera = parent;
}

void
platform_camera_print_info (platform_camera_t * cam)
{
  printf("------ Camera platform-specific information ------\n");
  printf("Handle                            :     %p\n", cam->handle);
  printf("Port                              :     %d\n", cam->port);
  printf("Node                              :     %d\n", cam->node);
}

dc1394error_t
platform_camera_read (platform_camera_t * cam, uint64_t offset,
		      uint32_t * quads, int num_quads)
{
  int i, retval, retry = DC1394_MAX_RETRIES;

  /* retry a few times if necessary (addition by PDJ) */
  while(retry--)  {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"get %d regs at 0x%llx : ",
        num_quads, offset + CONFIG_ROM_BASE);
#endif
    retval = raw1394_read (cam->handle, 0xffc0 | cam->node,
        offset + CONFIG_ROM_BASE, 4 * num_quads, quads);
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"0x%lx [...]\n", quads[0]);
#endif

    if (!retval)
      goto out;
    else if (errno != EAGAIN)
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );

    // usleep is executed only if the read fails!!!
    usleep(DC1394_SLOW_DOWN);
  }

out:
  /* conditionally byte swap the value */
  for (i = 0; i < num_quads; i++)
    quads[i] = ntohl (quads[i]);
  return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
platform_camera_write (platform_camera_t * cam, uint64_t offset,
    const uint32_t * quads, int num_quads)
{
  int i, retval, retry= DC1394_MAX_RETRIES;
  uint32_t value[num_quads];

  /* conditionally byte swap the value (addition by PDJ) */
  for (i = 0; i < num_quads; i++)
    value[i] = htonl (quads[i]);
  
  /* retry a few times if necessary */
  while(retry--) {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"set %d regs at 0x%llx to value 0x%lx [...]\n",
        num_quads, offset + CONFIG_ROM_BASE, value[0]);
#endif
    retval = raw1394_write(cam->handle, 0xffc0 | cam->node,
        offset + CONFIG_ROM_BASE, 4 * num_quads, value);

    if (!retval || (errno != EAGAIN))
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );

    // usleep is executed only if the read fails!!!
    usleep(DC1394_SLOW_DOWN);
  }
  
  return DC1394_RAW1394_FAILURE;
}

dc1394error_t
platform_reset_bus (platform_camera_t * cam)
{
  if (raw1394_reset_bus (cam->handle) == 0)
    return DC1394_SUCCESS;
  else
    return DC1394_FAILURE;
}

dc1394error_t
platform_read_cycle_timer (platform_camera_t * cam,
        uint32_t * cycle_timer, uint64_t * local_time)
{
  quadlet_t quad;
  struct timeval tv;

  if (raw1394_read (cam->handle, raw1394_get_local_id (cam->handle),
          CSR_REGISTER_BASE + CSR_CYCLE_TIME, sizeof (quadlet_t), &quad) < 0)
      return DC1394_FAILURE;

  gettimeofday (&tv, NULL);
  *cycle_timer = ntohl (quad);
  *local_time = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_allocate_iso_channel(dc1394camera_t *camera)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  dc1394error_t err;
  int i;
  dc1394switch_t iso_was_on;

  if (craw->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot allocate ISO channel\n");
    return DC1394_FAILURE;
  }

  if (craw->iso_channel_is_set) {
    fprintf (stderr, "Error: a channel is already allocated to this camera\n");
    return DC1394_FAILURE;
  }

  // if transmission is ON, abort
  err=dc1394_video_get_transmission(camera,&iso_was_on);
  DC1394_ERR_RTN(err, "Could not get ISO status");

  if (iso_was_on==DC1394_ON) {
    fprintf (stderr, "Error: camera is already streaming, aborting ISO channel allocation...\n"
        "  Perhaps it is in use by another application or a previous session was\n"
        "  not cleaned up properly.\n");
    return DC1394_FAILURE;
  }

  // first we need to assign an ISO channel:  
  if (craw->iso_channel >= 0) {
    // a specific channel is requested. try to book it.
    if (raw1394_channel_modify(craw->handle, craw->iso_channel,
          RAW1394_MODIFY_ALLOC)==0) {
      // channel allocated.
#ifdef DEBUG
      fprintf(stderr,"Allocated channel %d as requested\n",camera->iso_channel);
#endif
      craw->iso_channel_is_set=1;
      err=dc1394_video_set_iso_channel(camera, craw->iso_channel);
      DC1394_ERR_RTN(err, "Could not set ISO channel in the camera");
      return DC1394_SUCCESS;
    }
    fprintf(stderr,"Channel %d already reserved. Trying other channels\n",
            craw->iso_channel);
  }  

  for (i=0;i<DC1394_NUM_ISO_CHANNELS;i++) {
    if (raw1394_channel_modify(craw->handle, i, RAW1394_MODIFY_ALLOC)==0) {
      // channel allocated.
      craw->iso_channel=i;
      craw->iso_channel_is_set=1;
#ifdef DEBUG
      fprintf(stderr,"Allocated channel %d\n", craw->iso_channel);
#endif
      break;
    }
  }

  // check if channel was allocated:
  if (craw->iso_channel_is_set==0)
    return DC1394_NO_ISO_CHANNEL;

  // set channel in the camera
  err=dc1394_video_set_iso_channel(camera, craw->iso_channel);
  DC1394_ERR_RTN(err, "Could not set ISO channel in the camera");

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_allocate_bandwidth(dc1394camera_t *camera)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  dc1394error_t err;

  if (craw->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot allocate ISO bandwidth\n");
    return DC1394_FAILURE;
  }

  if (craw->iso_bandwidth) {
    fprintf (stderr, "Error: bandwidth already allocated for this camera\n");
    return DC1394_FAILURE;
  }

  err=dc1394_video_get_bandwidth_usage(camera, &craw->iso_bandwidth);
  DC1394_ERR_RTN(err, "Could not estimate ISO bandwidth");
  if (raw1394_bandwidth_modify(craw->handle, craw->iso_bandwidth,
        RAW1394_MODIFY_ALLOC)<0) {
    craw->iso_bandwidth=0;
    if (raw1394_channel_modify(craw->handle, craw->iso_channel,
          RAW1394_MODIFY_FREE)==-1) {
      fprintf(stderr,"Error: could not free iso channel %d!\n",
          craw->iso_channel);
    }
    return DC1394_NO_BANDWIDTH;
  }
#ifdef DEBUG
  fprintf(stderr,"Allocated %d bandwidth units\n", craw->iso_bandwidth);
#endif

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_free_iso_channel(dc1394camera_t *camera)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  dc1394error_t err;
  dc1394switch_t is_iso_on;
#ifdef DEBUG
  fprintf(stderr,"capture: %d ISO: %d\n", craw->capture_is_set,
      craw->is_iso_on);
#endif
  if (craw->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot free ISO channel\n");
    return DC1394_FAILURE;
  }

  err=dc1394_video_get_transmission(camera, &is_iso_on);
  DC1394_ERR_RTN(err, "Could not get ISO transmission status");

  if (is_iso_on) {
    fprintf (stderr, "Warning: stopping transmission in order to free ISO channel\n");
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err, "Could not stop ISO transmission");
  }

  if (craw->iso_channel_is_set == 0) {
    fprintf (stderr, "Error: no ISO channel to free\n");
    return DC1394_FAILURE;
  }

  if (raw1394_channel_modify(craw->handle, craw->iso_channel,
        RAW1394_MODIFY_FREE)==-1) {
    fprintf(stderr,"Error: could not free iso channel %d!\n",
        craw->iso_channel);
    return DC1394_RAW1394_FAILURE;
  }

#ifdef DEBUG
  fprintf(stderr,"Freed channel %d\n",camera->iso_channel);
#endif
  //camera->iso_channel=-1; // we don't need this line anymore.
  craw->iso_channel_is_set=0;
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_free_bandwidth(dc1394camera_t *camera)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  dc1394error_t err;
  dc1394switch_t is_iso_on;

  err=dc1394_video_get_transmission(camera, &is_iso_on);
  DC1394_ERR_RTN(err, "Could not get ISO transmission status");

#ifdef DEBUG
  fprintf(stderr,"capture: %d ISO: %d\n",craw->capture_is_set, is_iso_on);
#endif
  if (craw->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot free ISO bandwidth\n");
    return DC1394_FAILURE;
  }

  if (is_iso_on) {
    fprintf (stderr, "Warning: stopping transmission in order to free ISO bandwidth\n");
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err, "Could not stop ISO transmission");
  }

  if (craw->iso_bandwidth == 0) {
    fprintf (stderr, "Error: no ISO bandwidth to free\n");
    return DC1394_FAILURE;
  }

  // first free the bandwidth
  if (raw1394_bandwidth_modify(craw->handle, craw->iso_bandwidth,
        RAW1394_MODIFY_FREE)<0) {
    fprintf(stderr,"Error: could not free %d units of bandwidth!\n",
        craw->iso_bandwidth);
    return DC1394_RAW1394_FAILURE;
  }

#ifdef DEBUG
  fprintf(stderr,"Freed %d bandwidth units\n",camera->iso_bandwidth);
#endif
  craw->iso_bandwidth=0;
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_cleanup_iso_channels_and_bandwidth(dc1394camera_t *camera)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  int i;

  if (craw->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  // free all iso channels 
  for (i=0;i<DC1394_NUM_ISO_CHANNELS;i++)
    raw1394_channel_modify(craw->handle, i, RAW1394_MODIFY_FREE);
  
  // free bandwidth
  raw1394_bandwidth_modify(craw->handle, 4915, RAW1394_MODIFY_FREE);

  return DC1394_SUCCESS;
}
