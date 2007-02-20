/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
 * Additions by Chris Urmson <curmson@ri.cmu.edu>
 * Additions by Damien Douxchamps <ddouxchamps@users.sf.net>
 * Additions by David Moore <dcm@acm.org>
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
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/firewire/IOFireWireLib.h>
#include "config.h"
#include "internal.h"
#include "register.h"
#include "offsets.h"
#include "macosx/macosx.h"
#include "utils.h"

dc1394camera_t*
dc1394_new_camera_platform (uint32_t port, uint16_t node)
{
  dc1394camera_macosx_t *cam;

  cam=(dc1394camera_macosx_t *)malloc(sizeof(dc1394camera_macosx_t));
  if (cam==NULL)
    return NULL;

  memset (&cam->capture, 0, sizeof (dc1394capture_t));

  return (dc1394camera_t *) cam;
}

void
dc1394_free_camera_platform (dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  if (craw == NULL)
    return;
  
  if (craw->iface) {
    (*craw->iface)->Close (craw->iface);
    (*craw->iface)->Release (craw->iface);
  }

  free(craw);
}

dc1394error_t
dc1394_print_camera_info_platform (dc1394camera_t *camera) 
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  printf("------ Camera platform-specific information ------\n");
  printf("Interface                       :     0x%x\n", (uint32_t)craw->iface);
  printf("Generation                      :     %lu\n", craw->generation);
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_find_cameras_platform(dc1394camera_t ***cameras_ptr, uint32_t* numCameras)
{
  kern_return_t res;
  mach_port_t master_port;
  io_iterator_t iterator;
  io_object_t node;
  CFMutableDictionaryRef dict;
  UInt32 spec_id;
  CFNumberRef spec_id_ref;
  dc1394camera_t **cameras;
  int numCam;
  uint32_t allocated_size;

  res = IOMasterPort (MACH_PORT_NULL, &master_port);
  if (res != KERN_SUCCESS) {
    return DC1394_FAILURE;
  }

  dict = IOServiceMatching ("IOFireWireUnit");
  if (!dict) {
    return DC1394_FAILURE;
  }

  spec_id = 0xA02D;
  spec_id_ref = CFNumberCreate (kCFAllocatorDefault,
      kCFNumberSInt32Type, &spec_id);
  CFDictionaryAddValue (dict, CFSTR ("Unit_Spec_ID"), spec_id_ref);
  CFRelease (spec_id_ref);

  res = IOServiceGetMatchingServices (master_port, dict, &iterator);
  allocated_size=64; // initial allocation, will be reallocated if necessary
  cameras=(dc1394camera_t**)malloc(allocated_size*sizeof(dc1394camera_t*));
  if (!cameras)
    return DC1394_MEMORY_ALLOCATION_FAILURE;
  numCam=0;

  while ((node = IOIteratorNext (iterator))) {
    IOCFPlugInInterface ** plugin_interface = NULL;
    SInt32 score;
    dc1394camera_macosx_t * craw;
    dc1394camera_t * camera;
    uint32_t err;

    camera = dc1394_new_camera (0, 0);
    if (!camera) {
      IOObjectRelease (node);
      continue;
    }
    craw = (dc1394camera_macosx_t *) camera;

    res = IOCreatePlugInInterfaceForService (node, kIOFireWireLibTypeID,
        kIOCFPlugInInterfaceID, &plugin_interface, &score);
    IOObjectRelease (node);
    if (res != KERN_SUCCESS) {
      fprintf (stderr, "Failed to get plugin interface\n");
      dc1394_free_camera (camera);
      continue;
    }

    /* TODO: error check here */
    craw->iface = NULL;
    (*plugin_interface)->QueryInterface (plugin_interface,
                                         CFUUIDGetUUIDBytes (kIOFireWireDeviceInterfaceID),
                                         (void**) &(craw->iface));
    IODestroyPlugInInterface (plugin_interface);

    res = (*craw->iface)->Open (craw->iface);
    if (res != kIOReturnSuccess) {
      dc1394_free_camera (camera);
      continue;
    }

    (*craw->iface)->GetBusGeneration (craw->iface,
                                      &(craw->generation));
    (*craw->iface)->GetRemoteNodeID (craw->iface,
                                     craw->generation,
                                     &(camera->node));
//    fprintf (stderr, "Node ID is %x, Generation is %lu\n",
//            camera->node, craw->generation);
    err=dc1394_update_camera_info(camera);
    if (err != DC1394_SUCCESS) {
      dc1394_free_camera (camera);
      continue;
    }

    if (numCam >= allocated_size) {
      dc1394camera_t ** newcam;
      allocated_size*=2;
      newcam = realloc(cameras,allocated_size*sizeof(dc1394camera_t*));
      if (newcam ==NULL) {
        int i;
        for (i=0;i<numCam;i++) {
          dc1394_free_camera(cameras[i]);
          cameras[i]=NULL;
        }
        free(cameras);

        if (craw!=NULL) {
          dc1394_free_camera(camera);
          craw=NULL;
        }

        fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ",
            __FILE__, __FUNCTION__, __LINE__,
            "Can't reallocate camera array");
        return DC1394_MEMORY_ALLOCATION_FAILURE;
      }
      cameras = newcam;
    }

    cameras[numCam++] = camera;
  }
  IOObjectRelease (iterator);

  *numCameras=numCam;
  *cameras_ptr=cameras;

  if (numCam==0)
    return DC1394_NO_CAMERA;

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_reset_bus_platform (dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  IOFireWireLibDeviceRef d = craw->iface;

  if ((*d)->BusReset (d) == 0)
    return DC1394_SUCCESS;
  else
    return DC1394_FAILURE;
}

dc1394error_t
dc1394_read_cycle_timer_platform (dc1394camera_t * camera,
        uint32_t * cycle_timer, uint64_t * local_time)
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  IOFireWireLibDeviceRef d = craw->iface;
  struct timeval tv;

  if ((*d)->GetCycleTime (d, cycle_timer) != 0)
      return DC1394_FAILURE;

  gettimeofday (&tv, NULL);
  *local_time = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
  return DC1394_SUCCESS;
}

dc1394error_t
GetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  IOFireWireLibDeviceRef d = craw->iface;
  FWAddress full_addr;
  int i, retval;
  UInt32 length;
  UInt64 addr = CONFIG_ROM_BASE + offset;

  full_addr.addressHi = addr >> 32;
  full_addr.addressLo = addr & 0xffffffff;

  length = 4 * num_quads;
  retval = (*d)->Read (d, (*d)->GetDevice (d), &full_addr, value, &length,
      true, craw->generation);
  if (retval != 0) {
    fprintf (stderr, "Error reading (%x)...\n", retval);
    return DC1394_FAILURE;
  }
  for (i = 0; i < num_quads; i++)
  	value[i] = ntohl (value[i]);
  return DC1394_SUCCESS;
}

dc1394error_t
SetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_MACOSX(craw, camera);
  IOFireWireLibDeviceRef d = craw->iface;
  FWAddress full_addr;
  int i, retval;
  UInt32 length;
  UInt64 addr = CONFIG_ROM_BASE + offset;

  full_addr.addressHi = addr >> 32;
  full_addr.addressLo = addr & 0xffffffff;

  for (i = 0; i < num_quads; i++)
  	value[i] = htonl (value[i]);

  length = 4 * num_quads;
  retval = (*d)->Write (d, (*d)->GetDevice (d), &full_addr, value, &length,
      true, craw->generation);
  if (retval != 0) {
    fprintf (stderr, "Error writing (%x)...\n", retval);
    return DC1394_FAILURE;
  }
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_allocate_iso_channel_and_bandwidth(dc1394camera_t *camera)
{
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_free_iso_channel_and_bandwidth(dc1394camera_t *camera)
{
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_cleanup_iso_channels_and_bandwidth(dc1394camera_t *camera)
{
  //dc1394camera_macosx_t * craw = (dc1394camera_macosx_t *) camera;
  return DC1394_FAILURE;
}

