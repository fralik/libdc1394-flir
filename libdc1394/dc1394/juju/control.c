/*
 * Juju backend for dc1394
 * Copyright (C) 2007 Kristian Hoegsberg <krh@bitplanet.net>
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
#include <dirent.h>
#include <stdio.h>
#include "config.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "internal.h"
#include "register.h"
#include "offsets.h"
#include "juju/juju.h"
#include "utils.h"

#define ptr_to_u64(p) ((__u64)(unsigned long)(p))
#define u64_to_ptr(p) ((void *)(unsigned long)(p))

dc1394camera_t*
dc1394_new_camera_platform (uint32_t port, uint16_t node)
{
  dc1394camera_juju_t *craw;

  craw = (dc1394camera_juju_t *) malloc(sizeof *craw);
  if (craw == NULL)
    return NULL;

  memset (craw, 0, sizeof (*craw));

  return &craw->camera;
}

void
dc1394_free_camera_platform (dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);

  if (craw == NULL)
    return;

  free(craw->filename);
  close(craw->fd);
  free(craw);
}

dc1394error_t
dc1394_print_camera_info_platform (dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);

  printf("------ Camera platform-specific information ------\n");
  printf("Device filename                   :     %s\n", craw->filename);

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_find_cameras_platform(dc1394camera_t ***cameras_ptr,
			     uint32_t* numCameras)
{
  dc1394camera_t **cameras;
  DIR *dir;
  struct dirent *de;
  char filename[32];
  struct fw_cdev_get_info get_info;
  struct fw_cdev_event_bus_reset reset;
  uint32_t config_rom[256];
  int fd, numCam;
  uint32_t allocated_size;

  allocated_size = 64; // initial allocation, will be reallocated if necessary
  cameras = malloc(allocated_size * sizeof(*cameras));
  if (!cameras)
    return DC1394_MEMORY_ALLOCATION_FAILURE;
  numCam = 0;

  dir = opendir("/dev");
  if (dir == NULL) {
    fprintf(stderr, "opendir: %m\n");
    free(cameras);
    return DC1394_FAILURE;
  }

  while (de = readdir(dir), de != NULL) {
    dc1394camera_juju_t *craw;
    dc1394camera_t *camera;
    uint32_t err;

    if (strncmp(de->d_name, "fw", 2) != 0)
      continue;

    snprintf(filename, sizeof filename, "/dev/%s", de->d_name);
    fd = open(filename, O_RDWR);
    if (fd < 0) {
      fprintf(stderr, "could not open %s: %m\n", filename);
      continue;
    }

    get_info.version = FW_CDEV_VERSION;
    get_info.rom = ptr_to_u64(&config_rom);
    get_info.rom_length = sizeof config_rom;
    get_info.bus_reset = ptr_to_u64(&reset);
    if (ioctl(fd, FW_CDEV_IOC_GET_INFO, &get_info) < 0) {
      fprintf(stderr, "GET_CONFIG_ROM failed for %s: %m\n", filename);
      close(fd);
      continue;
    }

    camera = dc1394_new_camera (0, 0);
    if (!camera) {
      close(fd);
      continue;
    }

    craw = (dc1394camera_juju_t *) camera;
    craw->async.generation = reset.generation;
    craw->filename = strdup(filename);
    memcpy(craw->config_rom, config_rom, sizeof craw->config_rom);
    craw->fd = fd;
    err = dc1394_update_camera_info(camera);
    if (craw->filename == NULL || err != DC1394_SUCCESS) {
      dc1394_free_camera (camera);
      continue;
    }

    if (numCam >= allocated_size) {
      dc1394camera_t **newcam;
      allocated_size *= 2;
      newcam = realloc(cameras, allocated_size * sizeof(*newcam));
      if (newcam == NULL)
	continue;
      cameras = newcam;
    }

    cameras[numCam++] = camera;
  }
  closedir(dir);

  *numCameras = numCam;
  *cameras_ptr = cameras;

  if (numCam==0)
    return DC1394_NO_CAMERA;

  return DC1394_SUCCESS;
}

dc1394error_t
_juju_iterate(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  union {
    struct {
      struct fw_cdev_event_response r;
      __u32 buffer[128];
    } response;
    struct fw_cdev_event_bus_reset reset;
    struct {
      struct fw_cdev_event_iso_interrupt i;
      __u32 headers[256];
    } iso;
  } u;
  int len;

  len = read (craw->fd, &u, sizeof u);
  if (len < 0) {
    fprintf (stderr, "failed to read response: %m\n");
    return DC1394_FAILURE;
  }

  switch (u.reset.type) {
  case FW_CDEV_EVENT_BUS_RESET:
    craw->async.generation = u.reset.generation;
    break;

  case FW_CDEV_EVENT_RESPONSE:
    memcpy(craw->async.buffer, u.response.buffer, u.response.r.length);
    craw->async.done = 1;
    break;
  }

  return DC1394_SUCCESS;
}

static dc1394error_t
do_transaction(dc1394camera_t *camera, int tcode, uint64_t offset,
	       uint32_t *in, uint32_t *out, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct fw_cdev_send_request request;
  int i, len;

  if (in)
    for (i = 0; i < num_quads; i++)
      craw->async.buffer[i] = htonl (in[i]);

  request.offset = CONFIG_ROM_BASE + offset;
  request.data = ptr_to_u64(craw->async.buffer);
  request.length = num_quads * 4;
  request.tcode = tcode;
  request.generation = craw->async.generation;

  len = ioctl (craw->fd, FW_CDEV_IOC_SEND_REQUEST, &request);
  if (len < 0) {
    fprintf (stderr, "failed to write request: %m\n");
    return DC1394_FAILURE;
  }

  craw->async.done = 0;
  while (!craw->async.done)
    _juju_iterate(camera);

  if (out)
    for (i = 0; i < num_quads; i++)
      out[i] = ntohl (craw->async.buffer[i]);

  return DC1394_SUCCESS;
}

dc1394error_t
GetCameraROMValues(dc1394camera_t *camera,
		   uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  int tcode;

  if (0x400 <= offset && offset < 0x800) {
    memcpy(value, (void *) craw->config_rom + offset - 0x400, num_quads * 4);
    return DC1394_SUCCESS;
  }

  if (num_quads > 1)
    tcode = TCODE_READ_BLOCK_REQUEST;
  else
    tcode = TCODE_READ_QUADLET_REQUEST;

  return do_transaction(camera, tcode, offset, NULL, value, num_quads);
}

dc1394error_t
SetCameraROMValues(dc1394camera_t *camera,
		   uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  int tcode;

  if (num_quads > 1)
    tcode = TCODE_WRITE_BLOCK_REQUEST;
  else
    tcode = TCODE_WRITE_QUADLET_REQUEST;

  return do_transaction(camera, tcode, offset, value, NULL, num_quads);
}

dc1394error_t
dc1394_reset_bus_platform (dc1394camera_t * camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct fw_cdev_initiate_bus_reset initiate;

  initiate.type = FW_CDEV_SHORT_RESET;
  if (ioctl(craw->fd, FW_CDEV_IOC_INITIATE_BUS_RESET, &initiate) == 0)
    return DC1394_SUCCESS;
  else
    return DC1394_FAILURE;
}

dc1394error_t
dc1394_read_cycle_timer_platform (dc1394camera_t * camera,
        uint32_t * cycle_timer, uint64_t * local_time)
{
  /* FIXME: implement this ioctl */

  return DC1394_FUNCTION_NOT_SUPPORTED;
}

dc1394error_t
dc1394_allocate_iso_channel(dc1394camera_t *camera)
{
  /* FIXME: do this right */

  camera->iso_channel = 0;
  camera->iso_channel_is_set = 1;

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_allocate_bandwidth(dc1394camera_t *camera)
{
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_free_iso_channel(dc1394camera_t *camera)
{
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_free_bandwidth(dc1394camera_t *camera)
{
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_cleanup_iso_channels_and_bandwidth(dc1394camera_t *camera)
{
  return DC1394_FAILURE;
}

