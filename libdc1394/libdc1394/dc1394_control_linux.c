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
#include <libraw1394/raw1394.h>
#include "config.h"
#include "dc1394_internal.h"
#include "dc1394_register.h"
#include "dc1394_offsets.h"
#include "dc1394_linux.h"
#include "dc1394_utils.h"

dc1394camera_t*
dc1394_new_camera_platform (uint_t port, nodeid_t node)
{
  dc1394camera_linux_t *cam;

  cam=(dc1394camera_linux_t *)malloc(sizeof(dc1394camera_linux_t));
  if (cam==NULL)
    return NULL;

  cam->handle=raw1394_new_handle();
  raw1394_set_port(cam->handle, port);
  cam->capture.dma_device_file=NULL;
  //fprintf(stderr,"Created camera with handle 0x%x\n",cam->handle);

  return (dc1394camera_t *) cam;
}

void
dc1394_free_camera_platform (dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  if (craw == NULL)
    return;

  if (craw->handle!=NULL)
    raw1394_destroy_handle(craw->handle);
   
  if (craw->capture.dma_device_file!=NULL) {
    free(craw->capture.dma_device_file);
    craw->capture.dma_device_file=NULL;
  }
    
  free(craw);
}

dc1394error_t
dc1394_print_camera_info_platform (dc1394camera_t *camera) 
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  printf("------ Camera platform-specific information ------\n");
  printf("Handle                            :     0x%x\n", (uint_t)craw->handle);
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_find_cameras_platform(dc1394camera_t ***cameras_ptr, uint_t* numCameras)
{
  // get the number the ports
  raw1394handle_t handle;
  uint_t port_num, port;
  uint_t allocated_size;
  dc1394camera_t **cameras;
  uint_t numCam, err=DC1394_SUCCESS, i, numNodes;
  nodeid_t node;

  //dc1394bool_t isCamera;
  dc1394camera_t *tmpcam=NULL;
  dc1394camera_t **newcam;

  //fprintf(stderr,"entering dc1394_find_cameras\n");

  cameras=*cameras_ptr;
  handle=raw1394_new_handle();

  if(handle == NULL)
    return DC1394_RAW1394_FAILURE;
  
  port_num=raw1394_get_port_info(handle, NULL, 0);
  
  allocated_size=64; // initial allocation, will be reallocated if necessary
  cameras=(dc1394camera_t**)malloc(allocated_size*sizeof(dc1394camera_t*));
  if (!cameras)
    return DC1394_MEMORY_ALLOCATION_FAILURE;
  numCam=0;
  
  // scan each port for cameras. When a camera is found add it.
  // if the number of cameras is not enough the array is re-allocated.
  
  for (port=0;port<port_num;port++) {
    // get a handle to the current interface card
    if (handle!=NULL) {
      raw1394_destroy_handle(handle);
      handle=NULL;
    }
    handle=raw1394_new_handle();
    raw1394_set_port(handle, port);
    
    // find the cameras on this card
    numNodes = raw1394_get_nodecount(handle);
    raw1394_destroy_handle(handle);
    handle=NULL;
    
    for (node=0;node<numNodes;node++){
      //fprintf(stderr,"------------------------ New device -----------------\n");
      // create a camera struct for probing
      if (tmpcam==NULL)
	tmpcam=dc1394_new_camera_platform(port,node);
      
      if (tmpcam==NULL) {
	
	for (i=0;i<numCam;i++) {
	  dc1394_free_camera(cameras[i]);
	  cameras[i]=NULL;
	}
	free(cameras);
	
	fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ", __FILE__, __FUNCTION__, __LINE__, "Can't allocate camera structure\n");
	return DC1394_MEMORY_ALLOCATION_FAILURE;
      }
      
      err=dc1394_update_camera_info(tmpcam);
      
      // This segment has been removed. Reason: some devices (like my hub) refuse to honour read requests even at offset 0x414.
      // The result of this is a low-level error that is translated by DC1394_FAILURE. The latter error code was interpreted as
      // a major system failure while it is actually simply a bad device.
      /*
	if ((err != DC1394_SUCCESS) &&
	    (err != DC1394_NOT_A_CAMERA) &&
	    (err != DC1394_TAGGED_REGISTER_NOT_FOUND)) {
	
	for (i=0;i<numCam;i++)
	  dc1394_free_camera(cameras[i]);
	free(cameras);
	
	dc1394_free_camera(tmpcam);
	
	fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ", __FILE__, __FUNCTION__, __LINE__, "Can't check if node is a camera\n");
	return err;
      }
      */
      
      if (err == DC1394_SUCCESS) { // is it a camera?
	// check if this camera was not found yet. (a camera might appear twice with strange bus topologies)
	// this hack comes from coriander.
	if (numCam>0) {
	  for (i=0;i<numCam;i++) {
	    if (tmpcam->euid_64==cameras[i]->euid_64) {
	      i++; // add 1 because we remove one in all cases below, while we should not do it if a cam is detected here.
	      // the camera is already there. don't append.
	      break;
	    }
	  }
	  i--; // remove 1 since i might be =numCam and the max index is numCam-1
	  if (tmpcam->euid_64!=cameras[i]->euid_64) {
	    cameras[numCam]=tmpcam;
	    tmpcam=NULL;
	    numCam++;
	  }
	  else {
	    dc1394_free_camera(tmpcam);
	    tmpcam=NULL;
	  }
	}
	else { // numcam == 0: we add the first camera without questions
	  cameras[numCam]=tmpcam;
	  tmpcam=NULL;
	  numCam++;
	}
      }
      // don't forget to free the 1394 device we just found if it's not a camera
      // thanks to Jack Morrison for spotting this.
      else {
	dc1394_free_camera(tmpcam);
	tmpcam=NULL;
      }
      
      if (numCam>=allocated_size) {
	allocated_size*=2;
	newcam=realloc(cameras,allocated_size*sizeof(dc1394camera_t*));
	if (newcam ==NULL) {
	  for (i=0;i<numCam;i++) {
	    dc1394_free_camera(cameras[i]);
	    cameras[i]=NULL;
	  }
	  free(cameras);
	  
	  if (tmpcam!=NULL) {
	    dc1394_free_camera(tmpcam);
	    tmpcam=NULL;
	  }
	  
	  fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ",
		  __FILE__, __FUNCTION__, __LINE__,
		  "Can't reallocate camera array");
	  return DC1394_MEMORY_ALLOCATION_FAILURE;
	}
	else {
	  cameras=newcam;
	}
      }

      if (tmpcam!=NULL) {
	dc1394_free_camera(tmpcam);
	tmpcam=NULL;
      }
    }
  }
    
  *numCameras=numCam;

  *cameras_ptr=cameras;

  if (tmpcam!=NULL) {
    dc1394_free_camera(tmpcam);
    tmpcam=NULL;
  }

  //fprintf(stderr,"leaving dc1394_find_cameras\n");
  
  if (numCam==0)
    return DC1394_NO_CAMERA;

  return DC1394_SUCCESS;
}

dc1394error_t
GetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t *value)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  int retval=1, retry= DC1394_MAX_RETRIES;

  /* retry a few times if necessary (addition by PDJ) */
  while(retry--)  {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"get reg at 0x%llx : ", offset + CONFIG_ROM_BASE);
#endif
    retval= raw1394_read(craw->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4, value);
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"0x%lx\n",*value);
#endif
    usleep(DC1394_SLOW_DOWN);

    if (!retval) {
      /* conditionally byte swap the value */
      *value= ntohl(*value);
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );;
    }
    else if (errno != EAGAIN) {
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );;
    }
    
  }
  
  *value= ntohl(*value);
  return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
SetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t value)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  int retval=1, retry= DC1394_MAX_RETRIES;

  /* conditionally byte swap the value (addition by PDJ) */
  value= htonl(value);
  
  /* retry a few times if necessary */
  while(retry--) {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"set reg at 0x%llx to value 0x%lx\n", offset + CONFIG_ROM_BASE, value);
#endif
    retval= raw1394_write(craw->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4, &value);

    usleep(DC1394_SLOW_DOWN);

    if (!retval || (errno != EAGAIN)) {
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );;
    }
    
  }
  
  return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
dc1394_allocate_iso_channel_and_bandwidth(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  dc1394error_t err;
  int i;

  if (camera->capture_is_set==0) {
    // capture is not set, and thus channels/bandwidth have not been allocated.
    // We can safely allocate that in advance. 

    // first we need to assign an ISO channel:  
    if (camera->iso_channel_is_set==0){
      if (camera->iso_channel>=0) {
	// a specific channel is requested. try to book it.
	if (raw1394_channel_modify(craw->handle, camera->iso_channel, RAW1394_MODIFY_ALLOC)==0) {
	  // channel allocated.
	  fprintf(stderr,"Allocated channel %d as requested\n",camera->iso_channel);
	  camera->iso_channel_is_set=1;
	}
	else {
	  fprintf(stderr,"Channel %d already reserved. Trying other channels\n",camera->iso_channel);
	}
      }  
    }

    if (camera->iso_channel_is_set==0){
      for (i=0;i<DC1394_NUM_ISO_CHANNELS;i++) {
	if (raw1394_channel_modify(craw->handle, i, RAW1394_MODIFY_ALLOC)==0) {
	  // channel allocated.
	  camera->iso_channel=i;
	  camera->iso_channel_is_set=1;
	  fprintf(stderr,"Allocated channel %d\n",camera->iso_channel);
	  break;
	}
      }
    }

    // check if channel was allocated:
    if (camera->iso_channel_is_set==0) {
      return DC1394_NO_ISO_CHANNEL;
    }
    else {
      // set channel in the camera
      err=dc1394_video_set_iso_channel(camera, camera->iso_channel);
      DC1394_ERR_RTN(err, "Could not set ISO channel in the camera");
    }
    
    if (camera->iso_bandwidth==0) {
      //fprintf(stderr,"Estimating ISO bandwidth\n");
      // then we book the bandwidth
      err=dc1394_video_get_bandwidth_usage(camera, &camera->iso_bandwidth);
      DC1394_ERR_RTN(err, "Could not estimate ISO bandwidth");
      if (raw1394_bandwidth_modify(craw->handle, camera->iso_bandwidth, RAW1394_MODIFY_ALLOC)<0) {
	camera->iso_bandwidth=0;
	if (raw1394_channel_modify(craw->handle, camera->iso_channel, RAW1394_MODIFY_FREE)==-1)
	  fprintf(stderr,"Error: could not free iso channel %d!\n",camera->iso_channel);
	return DC1394_NO_BANDWIDTH;
      }
      else
	fprintf(stderr,"Allocated %d bandwidth units\n",camera->iso_bandwidth);
    }
  }
  else {
    // do nothing, capture is running, and channels/bandwidth is already allocated
  }
  
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_free_iso_channel_and_bandwidth(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  fprintf(stderr,"capture: %d ISO: %d\n",camera->capture_is_set,camera->is_iso_on);
  if ((camera->capture_is_set==0)&&(camera->is_iso_on==0)) {
    // capture is not set and transmission is not active: channels/bandwidth can be freed without interfering

    if (camera->iso_bandwidth>0) {
      // first free the bandwidth
      if (raw1394_bandwidth_modify(craw->handle, camera->iso_bandwidth, RAW1394_MODIFY_FREE)<0) {
	fprintf(stderr,"Error: could not free %d units of bandwidth!\n", camera->iso_bandwidth);
	return DC1394_RAW1394_FAILURE;
      }
      else {
	fprintf(stderr,"Freed %d bandwidth units\n",camera->iso_bandwidth);
	camera->iso_bandwidth=0;
      }
    }
    
    // then free the ISO channel if it was allocated
    if (camera->iso_channel_is_set>0) {
      if (raw1394_channel_modify(craw->handle, camera->iso_channel, RAW1394_MODIFY_FREE)==-1) {
	fprintf(stderr,"Error: could not free iso channel %d!\n",camera->iso_channel);
	return DC1394_RAW1394_FAILURE;
      }
      else {
	fprintf(stderr,"Freed channel %d\n",camera->iso_channel);
	//camera->iso_channel=-1; // we don't need this line anymore.
	camera->iso_channel_is_set=0;
      }
    }
  }
  else {
    // capture is running, don't free any channel/bandwidth allocation
  }
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_cleanup_iso_channels_and_bandwidth(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  int i;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  // free all iso channels 
  for (i=0;i<DC1394_NUM_ISO_CHANNELS;i++)
    raw1394_channel_modify(craw->handle, i, RAW1394_MODIFY_FREE);
  
  // free bandwidth
  raw1394_bandwidth_modify(craw->handle, 4915, RAW1394_MODIFY_FREE);

  return DC1394_SUCCESS;
}

