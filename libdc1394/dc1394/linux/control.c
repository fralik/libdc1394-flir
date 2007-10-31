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
#include "config.h"
#include "internal.h"
//#include "linux/raw1394support.h"
//#include "linux/topology.h"
#include "register.h"
#include "offsets.h"
#include "linux/linux.h"
#include "utils.h"

/*
void
GrabSelfIds(dc1394camera_t **cams, int ncams)
{
  RAW1394topologyMap *topomap;
  SelfIdPacket_t packet[4];
  unsigned int* pselfid_int;
  int i, port,k;
  dc1394camera_linux_t* camera_ptr;
  raw1394handle_t main_handle, port_handle;

  memset(packet, 0, sizeof(packet)); // init to zero to avoid valgrind errors

  // get the number of adapters:
  main_handle=raw1394_new_handle();
  int port_num=raw1394_get_port_info(main_handle, NULL, 0);
  
  // for each adapter:
  for (port=0;port<port_num;port++) {
    port_handle=raw1394_new_handle();
    raw1394_set_port(port_handle,port);
    // get and decode SelfIds.
    topomap=raw1394GetTopologyMap(port_handle);

    if (topomap!=NULL) {
      for (i=0;i<topomap->selfIdCount;i++) {
	pselfid_int = (unsigned int *) &topomap->selfIdPacket[i];
	decode_selfid(packet,pselfid_int);
	// find the camera related to this packet:
	
	for (k=0;k<ncams;k++) {
	  camera_ptr = (dc1394camera_linux_t *) cams[k];
	  if ((cams[k]->node==packet[0].packetZero.phyID) &&
	      (cams[k]->port==port)) { // added a check for the port too!!
	    camera_ptr->selfid_packet=packet[0];
	  }
	}
      }
      raw1394_destroy_handle(port_handle);
      free(topomap);
    }
  }
  
  raw1394_destroy_handle(main_handle);

  // interpret data:
  for (k=0;k<ncams;k++) {
    camera_ptr = (dc1394camera_linux_t *) cams[k];
    cams[k]->phy_delay=camera_ptr->selfid_packet.packetZero.phyDelay+DC1394_PHY_DELAY_MIN;
    cams[k]->phy_speed=camera_ptr->selfid_packet.packetZero.phySpeed+DC1394_ISO_SPEED_MIN;
    cams[k]->power_class=camera_ptr->selfid_packet.packetZero.powerClass+DC1394_POWER_CLASS_MIN;  
  }

}

*/

dc1394camera_t*
dc1394_new_camera_platform (uint32_t port, uint16_t node)
{
  dc1394camera_linux_t *cam;

  cam=(dc1394camera_linux_t *)calloc(1,sizeof(dc1394camera_linux_t));
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
  printf("Handle                            :     0x%x\n", (uint32_t)craw->handle);
  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_find_cameras_platform(dc1394camera_t ***cameras_ptr, uint32_t* numCameras)
{
  // get the number the ports
  raw1394handle_t handle;
  uint32_t port_num, port;
  uint32_t allocated_size;
  dc1394camera_t **cameras;
  uint32_t numCam, err=DC1394_SUCCESS, i, numNodes;
  uint16_t node;
  uint32_t unit, numUnitDirectories;
  int retval;
  uint32_t root_directory_size,value;

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
//   raw1394_destroy_handle(handle);
//   handle=NULL;
    //fprintf(stderr,"testing port %d with %d nodes ============\n",port,numNodes);
    
    for (node=0;node<numNodes;node++){
      //fprintf(stderr,"----------- Scanning Node %d -----------------\n", node );

      // Now get size of configuration ROM root directory.
      retval=raw1394_read( handle, 0xFFC0 | node, CONFIG_ROM_BASE + ROM_ROOT_DIRECTORY, 4, &root_directory_size );
      usleep(DC1394_SLOW_DOWN);
      if (retval) {
	// don't write warnings and especially don't quit as other devices that don't play nice (e.g. hubs) may
	// have a very very partial ROM.
	continue;
      }

      root_directory_size = ntohl(root_directory_size) >> 16;
      //fprintf(stderr, "Root directory size = %d\n", root_directory_size );

      //  and count how many unit directories are present in the root
      numUnitDirectories = 0;
      for (i=0; i<root_directory_size; i++) {
        retval=raw1394_read( handle, 0xFFC0 | node, CONFIG_ROM_BASE + ROM_ROOT_DIRECTORY + (i+1)*4, 4, &value );
	usleep(DC1394_SLOW_DOWN);
        value = ntohl(value);
        //fprintf(stderr, "Root directory entry %d = %x\n", i, value );
	if ((value >> 24) == 0xD1)
	  numUnitDirectories++;
      }

      //fprintf(stderr,"Found %d unit directories in this node.\n", numUnitDirectories);

      if (numUnitDirectories == 0) {
        //fprintf(stderr, "No unit directories found! Skipping node.\n");
        continue;
      }

      for (unit=0; unit<numUnitDirectories; unit++) {

        // create a camera struct for probing
        if (tmpcam==NULL) {
	  //fprintf(stderr,"Allocating new cam struct %d %d\n",port, node);
  	  tmpcam=dc1394_new_camera(port,node,unit);
        } 
        // verify memory allocation
        if (tmpcam==NULL) {
	  for (i=0;i<numCam;i++) {
	    dc1394_free_camera(cameras[i]);
	    cameras[i]=NULL;
	  }
	  free(cameras);
          raw1394_destroy_handle(handle);
	  handle=NULL;
	  //fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ", __FILE__, __FUNCTION__, __LINE__, "Can't allocate camera structure\n");
	  return DC1394_MEMORY_ALLOCATION_FAILURE;
         }

      
	// get camera information
	err=dc1394_update_camera_info(tmpcam);
	
	if (err == DC1394_SUCCESS) { // is it a camera?
	  // check if this camera was not found yet. (a camera might appear twice with strange bus topologies)
	  // this hack comes from coriander.
	  //fprintf(stderr,"camera found: %s\n",tmpcam->model);
	  if (numCam>0) {
	    for (i=0;i<numCam;i++) {
	      if (dc1394_is_same_camera(tmpcam->id, cameras[i]->id)) {
		i++; // add 1 because we remove one in all cases below, while we should not do it if a cam is detected here.
		//fprintf(stderr,"the camera is already there. don't append\n");
		break;
	      }
	    }
	    i--; // remove 1 since i might be =numCam and the max index is numCam-1
	    if (!dc1394_is_same_camera(tmpcam->id, cameras[i]->id)) {
	      //fprintf(stderr,"another camera added\n");
	      cameras[numCam]=tmpcam;
	      tmpcam=NULL;
	      numCam++;
	    }
	    else {
	      //fprintf(stderr,"camera already there, removing duplicate\n");
	      dc1394_free_camera(tmpcam);
	      tmpcam=NULL;
	    }
	  }
	  else { // numcam == 0: we add the first camera without questions
	    //fprintf(stderr,"first camera added\n");
	    cameras[numCam]=tmpcam;
	    tmpcam=NULL;
	    numCam++;
	  }
	}
	// don't forget to free the 1394 device we just found if it's not a camera
	// thanks to Jack Morrison for spotting this.
	else {
	  //fprintf(stderr,"Not a camera\n");
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
	    raw1394_destroy_handle(handle);
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
      //fprintf(stderr,"next device\n");
    }
    
    raw1394_destroy_handle(handle);
    handle=NULL;
    //fprintf(stderr,"next port\n");
  }
  
  *numCameras=numCam;
  
  *cameras_ptr=cameras;
  
  if (tmpcam!=NULL) {
    dc1394_free_camera(tmpcam);
    tmpcam=NULL;
  }
  
  if (numCam==0)
    return DC1394_NO_CAMERA;

  // no support for this info in Linux at this time
  //else {
  //  GrabSelfIds(cameras, numCam);
  //}

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_reset_bus_platform (dc1394camera_t * camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);

  if (raw1394_reset_bus (craw->handle) == 0)
    return DC1394_SUCCESS;
  else
    return DC1394_FAILURE;
}

dc1394error_t
dc1394_read_cycle_timer_platform (dc1394camera_t * camera,
        uint32_t * cycle_timer, uint64_t * local_time)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  quadlet_t quad;
  struct timeval tv;

  if (raw1394_read (craw->handle, raw1394_get_local_id (craw->handle),
          CSR_REGISTER_BASE + CSR_CYCLE_TIME, sizeof (quadlet_t), &quad) < 0)
      return DC1394_FAILURE;

  gettimeofday (&tv, NULL);
  *cycle_timer = ntohl (quad);
  *local_time = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
  return DC1394_SUCCESS;
}

dc1394error_t
GetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  int i, retval=1, retry= DC1394_MAX_RETRIES;

  /* retry a few times if necessary (addition by PDJ) */
  while(retry--)  {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"get %d regs at 0x%llx : ", num_quads, offset + CONFIG_ROM_BASE);
#endif
    retval= raw1394_read(craw->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4 * num_quads, value);
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"0x%lx [...]\n", value[0]);
#endif

    if (!retval) {
      goto out;
    }
    else if (errno != EAGAIN) {
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
    }

    // usleep is executed only if the read fails!!!
    usleep(DC1394_SLOW_DOWN);
    
  }

out:
  /* conditionally byte swap the value */
  for (i = 0; i < num_quads; i++)
    value[i] = ntohl(value[i]);
  return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
SetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  int i, retval=1, retry= DC1394_MAX_RETRIES;

  /* conditionally byte swap the value (addition by PDJ) */
  for (i = 0; i < num_quads; i++)
    value[i] = htonl(value[i]);
  
  /* retry a few times if necessary */
  while(retry--) {
#ifdef DC1394_DEBUG_LOWEST_LEVEL
    fprintf(stderr,"set %d regs at 0x%llx to value 0x%lx [...]\n", num_quads, offset + CONFIG_ROM_BASE, value);
#endif
    retval= raw1394_write(craw->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4 * num_quads, value);

    if (!retval || (errno != EAGAIN)) {
      return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );;
    }

    // usleep is executed only if the read fails!!!
    usleep(DC1394_SLOW_DOWN);
    
  }
  
  return ( retval ? DC1394_RAW1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
dc1394_allocate_iso_channel(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  dc1394error_t err;
  int i;
  dc1394switch_t iso_was_on;

  if (camera->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot allocate ISO channel\n");
    return DC1394_FAILURE;
  }

  if (camera->iso_channel_is_set) {
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
  if (camera->iso_channel >= 0) {
    // a specific channel is requested. try to book it.
    if (raw1394_channel_modify(craw->handle, camera->iso_channel,
          RAW1394_MODIFY_ALLOC)==0) {
      // channel allocated.
#ifdef DEBUG
      fprintf(stderr,"Allocated channel %d as requested\n",camera->iso_channel);
#endif
      camera->iso_channel_is_set=1;
      err=dc1394_video_set_iso_channel(camera, camera->iso_channel);
      DC1394_ERR_RTN(err, "Could not set ISO channel in the camera");
      return DC1394_SUCCESS;
    }
    fprintf(stderr,"Channel %d already reserved. Trying other channels\n",camera->iso_channel);
  }  

  for (i=0;i<DC1394_NUM_ISO_CHANNELS;i++) {
    if (raw1394_channel_modify(craw->handle, i, RAW1394_MODIFY_ALLOC)==0) {
      // channel allocated.
      camera->iso_channel=i;
      camera->iso_channel_is_set=1;
#ifdef DEBUG
      fprintf(stderr,"Allocated channel %d\n",camera->iso_channel);
#endif
      break;
    }
  }

  // check if channel was allocated:
  if (camera->iso_channel_is_set==0)
    return DC1394_NO_ISO_CHANNEL;

  // set channel in the camera
  err=dc1394_video_set_iso_channel(camera, camera->iso_channel);
  DC1394_ERR_RTN(err, "Could not set ISO channel in the camera");

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_allocate_bandwidth(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  dc1394error_t err;

  if (camera->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot allocate ISO bandwidth\n");
    return DC1394_FAILURE;
  }

  if (camera->iso_bandwidth) {
    fprintf (stderr, "Error: bandwidth already allocated for this camera\n");
    return DC1394_FAILURE;
  }

  err=dc1394_video_get_bandwidth_usage(camera, &camera->iso_bandwidth);
  DC1394_ERR_RTN(err, "Could not estimate ISO bandwidth");
  if (raw1394_bandwidth_modify(craw->handle, camera->iso_bandwidth,
        RAW1394_MODIFY_ALLOC)<0) {
    camera->iso_bandwidth=0;
    if (raw1394_channel_modify(craw->handle, camera->iso_channel,
          RAW1394_MODIFY_FREE)==-1) {
      fprintf(stderr,"Error: could not free iso channel %d!\n",
          camera->iso_channel);
    }
    return DC1394_NO_BANDWIDTH;
  }
#ifdef DEBUG
  fprintf(stderr,"Allocated %d bandwidth units\n",camera->iso_bandwidth);
#endif

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_free_iso_channel(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  dc1394error_t err;
#ifdef DEBUG
  fprintf(stderr,"capture: %d ISO: %d\n",camera->capture_is_set,camera->is_iso_on);
#endif
  if (camera->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot free ISO channel\n");
    return DC1394_FAILURE;
  }

  if (camera->is_iso_on) {
    fprintf (stderr, "Warning: stopping transmission in order to free ISO channel\n");
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err, "Could not stop ISO transmission");
  }

  if (camera->iso_channel_is_set == 0) {
    fprintf (stderr, "Error: no ISO channel to free\n");
    return DC1394_FAILURE;
  }

  if (raw1394_channel_modify(craw->handle, camera->iso_channel,
        RAW1394_MODIFY_FREE)==-1) {
    fprintf(stderr,"Error: could not free iso channel %d!\n",
        camera->iso_channel);
    return DC1394_RAW1394_FAILURE;
  }

#ifdef DEBUG
  fprintf(stderr,"Freed channel %d\n",camera->iso_channel);
#endif
  //camera->iso_channel=-1; // we don't need this line anymore.
  camera->iso_channel_is_set=0;
  return DC1394_SUCCESS;
} 

dc1394error_t
dc1394_free_bandwidth(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_LINUX(craw, camera);
  dc1394error_t err;
#ifdef DEBUG
  fprintf(stderr,"capture: %d ISO: %d\n",camera->capture_is_set,camera->is_iso_on);
#endif
  if (camera->capture_is_set) {
    fprintf (stderr, "Error: capturing in progress, cannot free ISO bandwidth\n");
    return DC1394_FAILURE;
  }

  if (camera->is_iso_on) {
    fprintf (stderr, "Warning: stopping transmission in order to free ISO bandwidth\n");
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err, "Could not stop ISO transmission");
  }

  if (camera->iso_bandwidth == 0) {
    fprintf (stderr, "Error: no ISO bandwidth to free\n");
    return DC1394_FAILURE;
  }

  // first free the bandwidth
  if (raw1394_bandwidth_modify(craw->handle, camera->iso_bandwidth,
        RAW1394_MODIFY_FREE)<0) {
    fprintf(stderr,"Error: could not free %d units of bandwidth!\n",
        camera->iso_bandwidth);
    return DC1394_RAW1394_FAILURE;
  }

#ifdef DEBUG
  fprintf(stderr,"Freed %d bandwidth units\n",camera->iso_bandwidth);
#endif
  camera->iso_bandwidth=0;
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
