/*
 * 1394-Based Digital Camera Capture Code for the Control Library
 * Written by Chris Urmson <curmson@ri.cmu.edu>
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
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#include "config.h"
#include "dc1394_control.h"
#include "kernel-video1394.h"
#include "dc1394_internal.h"
#include "dc1394_utils.h"

#define NUM_ISO_CHANNELS 64
#define MAX_NUM_PORTS 8

/*Variables used for simultaneous capture of video from muliple cameras*/
uint_t *_dc1394_buffer[NUM_ISO_CHANNELS];
int _dc1394_frame_captured[NUM_ISO_CHANNELS];
int _dc1394_offset[NUM_ISO_CHANNELS];
int _dc1394_quadlets_per_frame[NUM_ISO_CHANNELS];
int _dc1394_quadlets_per_packet[NUM_ISO_CHANNELS];
int _dc1394_all_captured;

/* variables to handle multiple cameras using a single fd. */
int *_dc1394_dma_fd = NULL;
int *_dc1394_num_using_fd = NULL;

/**********************/
/* Internal functions */
/**********************/

/**************************************************************
 _dc1394_video_iso_handler

 This is the routine that is plugged into the raw1394 callback
 hook to allow us to capture mutliple iso video streams.  This
 is used in the non DMA capture routines.
***************************************************************/
dc1394error_t
_dc1394_video_iso_handler(raw1394handle_t handle, int channel, size_t length, quadlet_t *data) 
{

    /* the first packet of a frame has a 1 in the lsb of the header */
#ifdef LIBRAW1394_OLD
  if ( (data[0] & 0x1) && (_dc1394_frame_captured[channel] != 1) ) {
#else
  if ( (ntohl(data[0]) & 0x00000001UL) && (_dc1394_frame_captured[channel] != 1) ) {
#endif
    _dc1394_offset[channel]= 0;
    _dc1394_frame_captured[channel]= 2;
    
    /* the plus 1 is to shift past the first header quadlet*/
    memcpy((char*)_dc1394_buffer[channel], (char*)(data+1),
	   4*_dc1394_quadlets_per_packet[channel]);
    _dc1394_offset[channel]+=_dc1394_quadlets_per_packet[channel];
    
  }
  else if (_dc1394_frame_captured[channel] == 2) {
    int copy_n_quadlets = _dc1394_quadlets_per_packet[channel];
  
    if( _dc1394_offset[channel] + copy_n_quadlets
	> _dc1394_quadlets_per_frame[channel]) {
      /* this is the last packet. Maybe, we just need a part of its data */
      copy_n_quadlets= _dc1394_quadlets_per_frame[channel]
	- _dc1394_offset[channel];
    }
    
    memcpy((char*)(_dc1394_buffer[channel]+_dc1394_offset[channel]),
	   (char*)(data+1), 4*copy_n_quadlets);
    _dc1394_offset[channel]+= copy_n_quadlets;
    
    if (_dc1394_offset[channel] == _dc1394_quadlets_per_frame[channel]) {
      _dc1394_frame_captured[channel]= 1;
      _dc1394_all_captured--;
      _dc1394_offset[channel]= 0;
      
    }
    
  }
  
  return DC1394_SUCCESS;
}

/************************************************************
 _dc1394_basic_setup

 Sets up camera features that are capture type independent

 Returns DC1394_SUCCESS on success, DC1394_FAILURE otherwise
*************************************************************/
dc1394error_t 
_dc1394_basic_setup(dc1394camera_t *camera,
                    uint_t channel, uint_t mode,
                    uint_t speed, uint_t frame_rate, 
                    dc1394capture_t *capture)
{
  dc1394error_t err;
  dc1394switch_t is_iso_on= DC1394_OFF;

  /* Addition by Alexis Weiland: Certain cameras start sending iso
     data when they are reset, so we need to stop them so we can set
     up the camera properly.  Setting camera parameters "on the fly"
     while they are sending data doesn't seem to work.  I don't think
     this will cause any problems for other cameras. */
  /* Addition by Dan Dennedy: Restart iso transmission later if it is on */

  err=dc1394_video_get_transmission(camera, &is_iso_on);
  DC1394_ERR_CHK(err,"Unable to get ISO status");
  
  if (is_iso_on) {
    err=dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_CHK(err,"Unable to pause iso transmission");
  }
  
  err=dc1394_video_set_iso_channel_and_speed(camera,channel,speed);
  DC1394_ERR_CHK(err, "Unable to set channel and speed");
  
  err=dc1394_video_set_mode(camera,mode);
  DC1394_ERR_CHK(err, "Unable to set video mode %d!", mode);

  err=dc1394_video_set_framerate(camera,frame_rate);
  DC1394_ERR_CHK(err, "Unable to set framerate %d!", frame_rate);
  
  if (is_iso_on) {
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_CHK(err,"Unable to restart iso transmission");
  }

  capture->node= camera->node;
  capture->frame_rate= frame_rate;
  capture->port=camera->port;
  capture->channel= channel;
  capture->handle=raw1394_new_handle();
  raw1394_set_port(capture->handle,capture->port);

  err=_dc1394_get_quadlets_per_packet(mode, frame_rate, &capture->quadlets_per_packet);
  DC1394_ERR_CHK(err, "Unable to get quadlets per packet");

  if (capture->quadlets_per_packet < 0) {
    return DC1394_FAILURE;
  }
  
  err= _dc1394_quadlets_from_format(mode, &capture->quadlets_per_frame);
  DC1394_ERR_CHK(err,"Could not get quadlets from format");

  if (capture->quadlets_per_frame < 0)  {
    return DC1394_FAILURE;
  }
  
  err=dc1394_get_wh_from_mode(mode,&capture->frame_width,&capture->frame_height);
  DC1394_ERR_CHK(err,"Could not get width/height from format/mode");
  
  return err;
}


/*****************************************************
 dc1394_dma_basic_setup

 This sets up the dma for the given camera

******************************************************/
dc1394error_t
_dc1394_dma_basic_setup(uint_t channel,
                        uint_t num_dma_buffers,
                        dc1394capture_t *capture)
{

  struct video1394_mmap vmmap;
  struct video1394_wait vwait;
  uint_t i;
  
  if (capture->dma_device_file == NULL) {
    capture->dma_device_file = malloc(32);
    if (capture->dma_device_file)
    sprintf((char*)capture->dma_device_file, "/dev/video1394/%d", capture->port );
  }
  
  /* using_fd counter array NULL if not used yet -- initialize */
  if( NULL == _dc1394_num_using_fd ) {
    _dc1394_num_using_fd = calloc( MAX_NUM_PORTS, sizeof(uint_t) );
    _dc1394_dma_fd = calloc( MAX_NUM_PORTS, sizeof(uint_t) );
  }
	
  if (_dc1394_num_using_fd[capture->port] == 0) {
    
    if ( (capture->dma_fd = open(capture->dma_device_file,O_RDONLY)) < 0 ) {
      printf("(%s) unable to open video1394 device %s\n", __FILE__, capture->dma_device_file);
      perror( __FILE__ );
      return DC1394_FAILURE;
    }
    _dc1394_dma_fd[capture->port] = capture->dma_fd;
    
  }
  else
    capture->dma_fd = _dc1394_dma_fd[capture->port];

  
  _dc1394_num_using_fd[capture->port]++;
  vmmap.sync_tag= 1;
  vmmap.nb_buffers= num_dma_buffers;
  vmmap.flags= VIDEO1394_SYNC_FRAMES;
  vmmap.buf_size= capture->quadlets_per_frame * 4; //number of bytes needed
  vmmap.channel= channel;
  
  /* tell the video1394 system that we want to listen to the given channel */
  if (ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_CHANNEL, &vmmap) < 0) {
    printf("(%s) VIDEO1394_IOC_LISTEN_CHANNEL ioctl failed!\n", __FILE__);
    return DC1394_FAILURE;
  }
  //fprintf(stderr,"listening channel set\n");
  
  capture->dma_frame_size= vmmap.buf_size;
  capture->num_dma_buffers= vmmap.nb_buffers;
  capture->dma_last_buffer= -1;
  vwait.channel= channel;
  
  /* QUEUE the buffers */
  for (i= 0; i < vmmap.nb_buffers; i++) {
    vwait.buffer= i;
    
    if (ioctl(capture->dma_fd,VIDEO1394_IOC_LISTEN_QUEUE_BUFFER,&vwait) < 0) {
      printf("(%s) VIDEO1394_IOC_LISTEN_QUEUE_BUFFER ioctl failed!\n", __FILE__);
      ioctl(capture->dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &(vwait.channel));
      return DC1394_FAILURE;
    }
    
  }
  
  capture->dma_ring_buffer= mmap(0, vmmap.nb_buffers * vmmap.buf_size,
				 PROT_READ,MAP_SHARED, capture->dma_fd, 0);
  
  /* make sure the ring buffer was allocated */
  if (capture->dma_ring_buffer == (uchar_t*)(-1)) {
    printf("(%s) mmap failed!\n", __FILE__);
    ioctl(capture->dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &vmmap.channel);
    return DC1394_FAILURE;
  }
  
  capture->dma_buffer_size= vmmap.buf_size * vmmap.nb_buffers;
  capture->num_dma_buffers_behind = 0;

  //fprintf(stderr,"num dma buffers in setup: %d\n",capture->num_dma_buffers);
  return DC1394_SUCCESS;
}



/********************************
 libraw Capture Functions

 These functions use libraw
 to grab frames from the cameras,
 the dma routines are faster, and 
 should be used instead.
*********************************/

/*************************************************************
 dc1394_setup_capture

 Sets up both the camera and the cameracapture structure
 to be used other places.

 Returns DC1394_SUCCESS on success, DC1394_FAILURE otherwise
**************************************************************/
dc1394error_t 
dc1394_setup_capture(dc1394camera_t *camera, 
                     uint_t channel, uint_t mode, 
                     uint_t speed, uint_t frame_rate, 
                     dc1394capture_t *capture) 
{
  dc1394error_t err;
  uint_t format;

  err=_dc1394_get_format_from_mode(mode, &format);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  if( format == DC1394_FORMAT7) {
    err=dc1394_setup_format7_capture(camera, channel, mode, speed,
				     DC1394_QUERY_FROM_CAMERA, /*bytes_per_paket*/
				     DC1394_QUERY_FROM_CAMERA, /*left*/
				     DC1394_QUERY_FROM_CAMERA, /*top*/
				     DC1394_QUERY_FROM_CAMERA, /*width*/
				     DC1394_QUERY_FROM_CAMERA, /*height*/
				     capture);
    DC1394_ERR_CHK(err,"Could not setup F7 capture");
  }
  else {
    err=_dc1394_basic_setup(camera, channel, mode, speed,frame_rate, capture);
    DC1394_ERR_CHK(err,"Could not setup capture");
    
    capture->capture_buffer= (uint_t*)malloc(capture->quadlets_per_frame*4);
   
    if (capture->capture_buffer == NULL) {
      printf("(%s) unable to allocate memory for capture buffer\n", __FILE__);
      return DC1394_FAILURE;
    }
  }
  
  return err;
}

/****************************************************
 dc1394_release_camera

 Frees buffer space contained in the cameracapture 
 structure
*****************************************************/
dc1394error_t 
dc1394_release_capture(dc1394capture_t *capture)
{
  //fprintf(stderr,"Error a\n");
  if (capture->capture_buffer != NULL) {
    //fprintf(stderr,"Error b\n");
    free(capture->capture_buffer);
    //fprintf(stderr,"Error c\n");
  }
  //fprintf(stderr,"Error d\n");
  raw1394_destroy_handle(capture->handle);
  //fprintf(stderr,"Error e\n");
  
  return DC1394_SUCCESS;
}

/***************************************************************************
 dc1394_multi_capture

 This routine captures a frame from each camera specified in the cams array.
 Cameras must be set up first using dc1394_setup_camera.

 Returns DC1394_FAILURE if it fails, DC1394_SUCCESS if it succeeds
****************************************************************************/
dc1394error_t 
dc1394_capture(dc1394capture_t *cams, uint_t num) 
{
  int i, j;
  _dc1394_all_captured= num;

  /*
    this first routine does the setup-
    sets up the global variables needed in the handler,
    sets the iso handler,
    tells the 1394 subsystem to listen for iso packets
  */
  for (i= 0; i < num; i++)  {
    _dc1394_buffer[cams[i].channel]= cams[i].capture_buffer;
    
    if (raw1394_set_iso_handler(cams[i].handle, cams[i].channel, _dc1394_video_iso_handler) < 0)  {
      /* error handling- for some reason something didn't work, 
	 so we have to reset everything....*/
      printf("(%s:%d) error!\n",__FILE__, __LINE__);
      
      for (j= i - 1; j > -1; j--)  {
	raw1394_stop_iso_rcv(cams[i].handle, cams[j].channel);
	raw1394_set_iso_handler(cams[i].handle, cams[j].channel, NULL);
      }
      
      return DC1394_FAILURE;
    }

    //fprintf(stderr,"handle: 0x%x\n",(unsigned int)cams[i].handle);
    _dc1394_frame_captured[cams[i].channel] = 0;
    _dc1394_quadlets_per_frame[cams[i].channel] = cams[i].quadlets_per_frame;
    _dc1394_quadlets_per_packet[cams[i].channel] = cams[i].quadlets_per_packet;

    //fprintf(stderr,"starting reception...\n");
    //fprintf(stderr,"handle: 0x%x\n",(unsigned int)cams[i].handle);
    if (raw1394_start_iso_rcv(cams[i].handle,cams[i].channel) < 0)  {
      /* error handling- for some reason something didn't work, 
	 so we have to reset everything....*/
      fprintf(stderr,"(%s:%d) error!\n", __FILE__, __LINE__);
      
      for (j= 0; j < num; j++)  {
	raw1394_stop_iso_rcv(cams[i].handle, cams[j].channel);
	raw1394_set_iso_handler(cams[i].handle, cams[j].channel, NULL);
      }
      return DC1394_FAILURE;
    }
    
  }
  //fprintf(stderr,"data loop...\n");

  /* now we iterate till the data is here*/
  while (_dc1394_all_captured != 0)  {
    //fprintf(stderr,"iter ");
    raw1394_loop_iterate(cams[0].handle);
  }
  
  //fprintf(stderr,"data loop...\n");
  /* now stop the subsystem from listening*/
  for (i= 0; i < num; i++)  {
    raw1394_stop_iso_rcv(cams[i].handle, cams[i].channel);
    raw1394_set_iso_handler(cams[i].handle,cams[i].channel, NULL);
  }
  
  return DC1394_SUCCESS;
}

/**********************************
 DMA Capture Functions 

 These routines will be much faster
 than the above capture routines.
***********************************/

/*****************************************************
 dc1394_dma_setup_capture

 This sets up the given camera to capture images using
 the dma engine.  Should be much faster than the above
 routines
******************************************************/
dc1394error_t
dc1394_dma_setup_capture(dc1394camera_t *camera,
                         uint_t channel, uint_t mode,
                         uint_t speed, uint_t frame_rate,
                         uint_t num_dma_buffers, 
			 uint_t drop_frames,
			 const char *dma_device_file,
			 dc1394capture_t *capture)
{
  dc1394error_t err;
  uint_t format;

  err=_dc1394_get_format_from_mode(mode, &format);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  if( format == DC1394_FORMAT7) {
    err=dc1394_dma_setup_format7_capture(camera, channel, mode, speed,
					 DC1394_QUERY_FROM_CAMERA, /*bytes_per_paket*/
					 DC1394_QUERY_FROM_CAMERA, /*left*/
					 DC1394_QUERY_FROM_CAMERA, /*top*/
					 DC1394_QUERY_FROM_CAMERA, /*width*/
					 DC1394_QUERY_FROM_CAMERA, /*height*/
					 num_dma_buffers,
					 drop_frames,
					 dma_device_file,
					 capture); 
    DC1394_ERR_CHK(err,"Could not setup F7 capture");
  }
  else {
    capture->port = camera->port;
    capture->dma_device_file = dma_device_file;
    capture->drop_frames = drop_frames;
    
    err=_dc1394_basic_setup(camera, channel, mode, speed,frame_rate, capture);
    DC1394_ERR_CHK(err,"Could not setup capture");
    
    err=_dc1394_dma_basic_setup (channel, num_dma_buffers, capture);
    DC1394_ERR_CHK(err,"Could not setup DMA capture");
  }

  return err;
}

/*****************************************************
 dc1394_dma_release_camera

 This releases memory that was mapped by
 dc1394_dma_setup_camera
*****************************************************/
dc1394error_t 
dc1394_dma_release_capture(dc1394capture_t *capture) 
{
  //dc1394_stop_iso_transmission(handle,camera->node);
  //ioctl(_dc1394_dma_fd,VIDEO1394_IOC_UNLISTEN_CHANNEL, &(camera->channel));
  
  if (capture->dma_ring_buffer) {
    munmap((void*)capture->dma_ring_buffer,capture->dma_buffer_size);
    
    _dc1394_num_using_fd[capture->port]--;
    
    if (_dc1394_num_using_fd[capture->port] == 0) {
      
      while (close(capture->dma_fd) != 0) {
	printf("(%s) waiting for dma_fd to close\n", __FILE__);
	sleep(1);
      }
      
    }
  }

  raw1394_destroy_handle(capture->handle);
  
  return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_dma_unlisten

 This tells video1394 to halt iso reception.
*****************************************************/
dc1394error_t 
dc1394_dma_unlisten(dc1394capture_t *capture) 
{
  if (ioctl(capture->dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &(capture->channel)) < 0)
    return DC1394_FAILURE;
  else
    return DC1394_SUCCESS;
}

/****************************************************
 _dc1394_dma_multi_capture_private

 This capture a frame from each of the cameras passed
 in cams.  After you are finished with the frame, you
 must return the buffer to the pool by calling
 dc1394_dma_done_with_buffer.

 This function is private.

*****************************************************/
dc1394error_t
dc1394_dma_capture(dc1394capture_t *cams, uint_t num, dc1394videopolicy_t policy) 
{
  struct video1394_wait vwait;
  int i;
  int cb;
  int j;
  int result=-1;
  int last_buffer_orig;
  int extra_buf;
  
  //fprintf(stderr,"test0\n");
  for (i= 0; i < num; i++) {
    last_buffer_orig = cams[i].dma_last_buffer;
    cb = (cams[i].dma_last_buffer + 1) % cams[i].num_dma_buffers;
    cams[i].dma_last_buffer = cb;
    
    vwait.channel = cams[i].channel;
    vwait.buffer = cb;
    switch (policy) {
    case DC1394_VIDEO1394_POLL:
      result=ioctl(cams[i].dma_fd, VIDEO1394_IOC_LISTEN_POLL_BUFFER, &vwait);
      break;
    case DC1394_VIDEO1394_WAIT:
    default:
      result=ioctl(cams[i].dma_fd, VIDEO1394_IOC_LISTEN_WAIT_BUFFER, &vwait);
      break;
    }
    //fprintf(stderr,"test1\n");
    if ( result != 0) {       
      cams[i].dma_last_buffer = last_buffer_orig;
      if ((policy==DC1394_VIDEO1394_POLL) && (errno == EINTR)) {                       
	// when no frames is present, say so.
	return DC1394_NO_FRAME;
      }
      else {
	printf("(%s) VIDEO1394_IOC_LISTEN_WAIT/POLL_BUFFER ioctl failed!\n", __FILE__);
	cams[i].dma_last_buffer++; //Ringbuffer-index or counter?
	return DC1394_FAILURE;
      }
    }
    //fprintf(stderr,"test2\n");

    extra_buf = vwait.buffer;
    
    if (cams[i].drop_frames) {
      if (extra_buf > 0) {
	for (j = 0; j < extra_buf; j++) {
	  vwait.buffer = (cb + j) % cams[i].num_dma_buffers;
	  if (ioctl(cams[i].dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait) < 0)  {
	    printf("(%s) VIDEO1394_IOC_LISTEN_QUEUE_BUFFER failed in multi capture!\n", __FILE__);
	    return DC1394_FAILURE;
	  }
	}
	cams[i].dma_last_buffer = (cb + extra_buf) % cams[i].num_dma_buffers;
	
	/* Get the corresponding filltime: */
	vwait.buffer = cams[i].dma_last_buffer;
	if(ioctl(cams[i].dma_fd, VIDEO1394_IOC_LISTEN_POLL_BUFFER, &vwait) < 0) {
	  printf("(%s) VIDEO1394_IOC_LISTEN_POLL_BUFFER failed in multi capture!\n", __FILE__);
	  return DC1394_FAILURE;
	}
      }
    }
    //fprintf(stderr,"test3\n");
    
    /* point to the next buffer in the dma ringbuffer */
    cams[i].capture_buffer = (uint_t*)(cams[i].dma_ring_buffer +
				       cams[i].dma_last_buffer *
				       cams[i].dma_frame_size);
    
    cams[i].filltime = vwait.filltime;
    cams[i].num_dma_buffers_behind = extra_buf;
  }
  
  return DC1394_SUCCESS;
}

/****************************************************
 dc1394_dma_done_with_buffer

 This allows the driver to use the buffer previously
 handed to the user by dc1394_dma_*_capture
*****************************************************/
dc1394error_t 
dc1394_dma_done_with_buffer(dc1394capture_t *capture) 
{  
  struct video1394_wait vwait;
  
  if (capture->dma_last_buffer == -1)
    return DC1394_SUCCESS;
  
  vwait.channel= capture->channel;
  vwait.buffer= capture->dma_last_buffer;
  
  if (ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait) < 0)  {
    printf("(%s) VIDEO1394_IOC_LISTEN_QUEUE_BUFFER failed in done with buffer!\n", __FILE__);
    return DC1394_FAILURE;
  }
  
  return DC1394_SUCCESS;
}
