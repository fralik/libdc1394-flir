/*
 * 1394-Based Digital Camera Capture Code for the Control Library
 * Written by Chris Urmson <curmson@ri.cmu.edu>
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

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include "config.h"
#include "internal.h"
#include "control.h"
#include "kernel-video1394.h"
#include "utils.h"
#include "linux/linux.h"
#include "linux/capture.h"

#define MAX_NUM_PORTS 16

/* variables to handle multiple cameras using a single fd. */
int *_dc1394_dma_fd = NULL;
int *_dc1394_num_using_fd = NULL;

/**********************/
/* Internal functions */
/**********************/


void
_dc1394_capture_cleanup_alloc(void)
{
  // free allocated memory if no one is using the capture anymore:
  int i, use=0;
  for (i=0;i<MAX_NUM_PORTS;i++) {
    use+=_dc1394_num_using_fd[i];
  }
  if (use==0) {
    free(_dc1394_num_using_fd);
    _dc1394_num_using_fd=NULL;
    free(_dc1394_dma_fd);
    _dc1394_dma_fd=NULL;
  }
}

dc1394error_t
_dc1394_open_dma_device(platform_camera_t * craw)
{
  char filename[64];
  struct stat statbuf;

  // if the file has already been opened: increment the number of uses and return
  if (_dc1394_num_using_fd[craw->port] != 0) {
    _dc1394_num_using_fd[craw->port]++;
    craw->capture.dma_fd = _dc1394_dma_fd[craw->port];
    return DC1394_SUCCESS;
  }

  // if the dma device file has been set manually, use that device name
  if (craw->capture.dma_device_file!=NULL) {
    if ( (craw->capture.dma_fd = open(craw->capture.dma_device_file,O_RDWR)) < 0 ) {
      return DC1394_INVALID_VIDEO1394_DEVICE;
    }
    else {
      _dc1394_dma_fd[craw->port] = craw->capture.dma_fd;
      _dc1394_num_using_fd[craw->port]++;
      return DC1394_SUCCESS;
    }
  }

  // automatic mode: try to open several usual device files.
  sprintf(filename,"/dev/video1394/%d",craw->port);
  if ( stat (filename, &statbuf) == 0 &&
          S_ISCHR (statbuf.st_mode) &&
          (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
    _dc1394_dma_fd[craw->port] = craw->capture.dma_fd;
    _dc1394_num_using_fd[craw->port]++;
    return DC1394_SUCCESS;
  }

  sprintf(filename,"/dev/video1394-%d",craw->port);
  if ( stat (filename, &statbuf) == 0 &&
          S_ISCHR (statbuf.st_mode) &&
          (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
    _dc1394_dma_fd[craw->port] = craw->capture.dma_fd;
    _dc1394_num_using_fd[craw->port]++;
    return DC1394_SUCCESS;
  }
  
  if (craw->port==0) {
    sprintf(filename,"/dev/video1394");
    if ( stat (filename, &statbuf) == 0 &&
            S_ISCHR (statbuf.st_mode) &&
            (craw->capture.dma_fd = open(filename,O_RDWR)) >= 0 ) {
      _dc1394_dma_fd[craw->port] = craw->capture.dma_fd;
      _dc1394_num_using_fd[craw->port]++;
      return DC1394_SUCCESS;
    }
  }

  return DC1394_FAILURE;
}

/*****************************************************
 _ dc1394_dma_basic_setup

 This sets up the dma for the given camera. 
 _dc1394_capture_basic_setup must be called before

******************************************************/
dc1394error_t
_dc1394_capture_dma_setup(platform_camera_t * craw, uint32_t num_dma_buffers)
{
  struct video1394_mmap vmmap;
  struct video1394_wait vwait;
  uint32_t i;
  dc1394video_frame_t * f;

  memset(&vmmap, 0, sizeof(vmmap));
  memset(&vwait, 0, sizeof(vwait));

  /* using_fd counter array NULL if not used yet -- initialize */
  if ( _dc1394_num_using_fd == NULL ) {
    _dc1394_num_using_fd = calloc( MAX_NUM_PORTS, sizeof(uint32_t) );
    _dc1394_dma_fd = calloc( MAX_NUM_PORTS, sizeof(uint32_t) );
  }

  if (_dc1394_open_dma_device(craw) != DC1394_SUCCESS) {
    fprintf (stderr, "Could not open video1394 device file in /dev\n");
    _dc1394_capture_cleanup_alloc(); // free allocated memory if necessary
    return DC1394_INVALID_VIDEO1394_DEVICE;
  }
  
  vmmap.sync_tag= 1;
  vmmap.nb_buffers= num_dma_buffers;
  vmmap.flags= VIDEO1394_SYNC_FRAMES;
  vmmap.buf_size= craw->capture.frames[0].total_bytes; //number of bytes needed
  vmmap.channel= craw->iso_channel;
  
  /* tell the video1394 system that we want to listen to the given channel */
  if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_LISTEN_CHANNEL, &vmmap) < 0) {
    printf("(%s) VIDEO1394_IOC_LISTEN_CHANNEL ioctl failed: %s\n", __FILE__,
            strerror (errno));
    _dc1394_capture_cleanup_alloc(); // free allocated memory if necessary
    return DC1394_IOCTL_FAILURE;
  }
  // starting from here we use the ISO channel so we set the flag in the camera struct:
  craw->capture_is_set=1;

  //fprintf(stderr,"listening channel set\n");
  
  craw->capture.dma_frame_size= vmmap.buf_size;
  craw->capture.num_dma_buffers= vmmap.nb_buffers;
  craw->capture.dma_last_buffer= -1;
  vwait.channel= craw->iso_channel;
  
  /* QUEUE the buffers */
  for (i= 0; i < vmmap.nb_buffers; i++) {
    vwait.buffer= i;
    
    if (ioctl(craw->capture.dma_fd,VIDEO1394_IOC_LISTEN_QUEUE_BUFFER,&vwait) < 0) {
      printf("(%s) VIDEO1394_IOC_LISTEN_QUEUE_BUFFER ioctl failed!\n", __FILE__);
      ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &(vwait.channel));
      craw->capture_is_set=0;
      _dc1394_capture_cleanup_alloc(); // free allocated memory if necessary
      return DC1394_IOCTL_FAILURE;
    }
    
  }
  
  craw->capture.dma_ring_buffer= mmap(0, vmmap.nb_buffers * vmmap.buf_size,
				 PROT_READ|PROT_WRITE,MAP_SHARED, craw->capture.dma_fd, 0);
  
  /* make sure the ring buffer was allocated */
  if (craw->capture.dma_ring_buffer == (uint8_t*)(-1)) {
    printf("(%s) mmap failed!\n", __FILE__);
    ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &vmmap.channel);
    craw->capture_is_set=0;
    _dc1394_capture_cleanup_alloc(); // free allocated memory if necessary

    // This should be display if the user has low memory
    if (vmmap.nb_buffers * vmmap.buf_size > sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES)) {
      printf("Unable to allocate DMA buffer. The requested size (0x%ux or %ud MiB,\n",
             vmmap.nb_buffers * vmmap.buf_size,
             vmmap.nb_buffers * vmmap.buf_size/1048576);
      printf("is bigger than the available memory (0x%lux or %lud MiB)!\n",
             sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES),
             sysconf (_SC_PAGESIZE) * sysconf (_SC_AVPHYS_PAGES)/1048576);
      printf("Please free some memory before allocating the buffers.\n");
    } else {
      // if it's not low memory, then it's the vmalloc limit.
      // VMALLOC_RESERVED not sufficient (default is 128MiB in recent kernels)
      printf("Unable to allocate DMA buffer. The requested size (0x%x)\n",vmmap.nb_buffers * vmmap.buf_size);
      printf("may be larger than the usual default VMALLOC_RESERVED limit of 128MiB.\n");
      printf("To verify this, look for the following line in dmesg:\n");
      printf("'allocation failed: out of vmalloc space'\n");
      printf("If you see this, reboot with the kernel boot parameter:\n");
      printf("'vmalloc=k'\n");
      printf("where k (in bytes) is larger than your requested DMA ring buffer size.\n");
      printf("Note that other processes share the vmalloc space so you may need a\n");
      printf("large amount of vmalloc memory.\n");
      //}
    }
    return DC1394_IOCTL_FAILURE;
  }
  
  craw->capture.dma_buffer_size= vmmap.buf_size * vmmap.nb_buffers;

  for (i = 0; i < num_dma_buffers; i++) {
    f = craw->capture.frames + i;
    if (i > 0)
      memcpy (f, craw->capture.frames, sizeof (dc1394video_frame_t));
    f->image = (unsigned char *)(craw->capture.dma_ring_buffer +
        i * craw->capture.dma_frame_size);
    f->id = i;
  }

  //fprintf(stderr,"num dma buffers in setup: %d\n",craw->capture.num_dma_buffers);
  return DC1394_SUCCESS;
}


/* This function allows you to specify the DMA device filename manually. */
dc1394error_t
dc1394_capture_set_device_filename(dc1394camera_t* camera, char *filename)
{
  dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
  platform_camera_t * craw = cpriv->pcam;
  if (craw->capture.dma_device_file==NULL) {
    craw->capture.dma_device_file=(char*)malloc(64*sizeof(char));
    if (craw->capture.dma_device_file==NULL)
      return DC1394_MEMORY_ALLOCATION_FAILURE;
  }
  // note that the device filename is limited to 64 characters.
  memcpy(craw->capture.dma_device_file,filename,64*sizeof(char));

  return DC1394_SUCCESS;
}

dc1394error_t
platform_capture_setup(platform_camera_t *craw, uint32_t num_dma_buffers,
		     uint32_t flags)
{
  dc1394camera_t * camera = craw->camera;
  dc1394error_t err;

  if (flags & DC1394_CAPTURE_FLAGS_DEFAULT)
    flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
        DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;

  craw->capture.flags=flags;

  // if capture is already set, abort
  if (craw->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;

  // if auto iso is requested, stop ISO (if necessary)
  if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
    dc1394switch_t is_iso_on;
    dc1394_video_get_transmission(camera, &is_iso_on);
    if (is_iso_on == DC1394_ON) {
      err=dc1394_video_set_transmission(camera, DC1394_OFF);
      DC1394_ERR_RTN(err,"Could not stop ISO!");
    }
  }   

  // allocate channel/bandwidth if requested
  if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) {
    err=dc1394_allocate_iso_channel(camera);
    DC1394_ERR_RTN(err,"Could not allocate ISO channel!");
  }
  if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) {
    err=dc1394_allocate_bandwidth(camera);
    DC1394_ERR_RTN(err,"Could not allocate bandwidth!");
  }

  craw->capture.frames = malloc (num_dma_buffers * sizeof (dc1394video_frame_t));

  err=_dc1394_capture_basic_setup(camera, craw->capture.frames);
  if (err != DC1394_SUCCESS)
    goto fail;
  
  // the capture_is_set flag is set inside this function:
  err=_dc1394_capture_dma_setup (craw, num_dma_buffers);
  if (err != DC1394_SUCCESS)
    goto fail;

  // if auto iso is requested, start ISO
  if (flags & DC1394_CAPTURE_FLAGS_AUTO_ISO) {
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_RTN(err,"Could not start ISO!");
    craw->iso_auto_started=1;
  }

  return DC1394_SUCCESS;

fail:
  // free resources if they were allocated
  if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) {
    if (dc1394_free_iso_channel(camera) != DC1394_SUCCESS)
      fprintf (stderr, "Could not free ISO channel!\n");
  }
  if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) {
    if (dc1394_free_bandwidth(camera) != DC1394_SUCCESS)
      fprintf (stderr, "Could not free bandwidth!\n");
  }

  free (craw->capture.frames);
  craw->capture.frames = NULL;
  DC1394_ERR_RTN(err,"Could not setup DMA capture");

  return err;
}

/*****************************************************
 CAPTURE_STOP
*****************************************************/

dc1394error_t
platform_capture_stop(platform_camera_t *craw)
{
  dc1394camera_t * camera = craw->camera;
  int err;

  if (craw->capture_is_set>0) {
    // unlisten
    if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL,
              &(craw->iso_channel)) < 0) 
      return DC1394_IOCTL_FAILURE;
    
    // release
    if (craw->capture.dma_ring_buffer) {
      munmap((void*)craw->capture.dma_ring_buffer,craw->capture.dma_buffer_size);
    }
    
    _dc1394_num_using_fd[craw->port]--;
    
    if (_dc1394_num_using_fd[craw->port] == 0) {
      
      while (close(craw->capture.dma_fd) != 0) {
	printf("(%s) waiting for dma_fd to close\n", __FILE__);
	sleep (1);
      }
      
    }
    free (craw->capture.frames);
    craw->capture.frames = NULL;
    
    // this dma_device file is allocated by the strdup() function and can be freed here without problems.
    free(craw->capture.dma_device_file);
    craw->capture.dma_device_file=NULL;
    
    // capture is not set anymore
    craw->capture_is_set=0;
    
    // free ressources if they were allocated
    if ((craw->capture.flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) >0) {
      err=dc1394_free_iso_channel(camera);
      DC1394_ERR_RTN(err,"Could not free ISO channel!");
    }
    if ((craw->capture.flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) >0) {
      err=dc1394_free_bandwidth(camera);
      DC1394_ERR_RTN(err,"Could not free bandwidth!");
    }
    
    // stop ISO if it was started automatically
    if (craw->iso_auto_started>0) {
      err=dc1394_video_set_transmission(camera, DC1394_OFF);
      DC1394_ERR_RTN(err,"Could not stop ISO!");
      craw->iso_auto_started=0;
    }
      
    // free the additional capture handle
    raw1394_destroy_handle(craw->capture.handle);

    _dc1394_capture_cleanup_alloc();

  }
  else {
    return DC1394_CAPTURE_IS_NOT_SET;
  }
  
  return DC1394_SUCCESS;
}


dc1394error_t
platform_capture_dequeue (platform_camera_t * craw,
			dc1394capture_policy_t policy,
			dc1394video_frame_t **frame)
{
  dc1394capture_t * capture = &(craw->capture);
  struct video1394_wait vwait;
  dc1394video_frame_t * frame_tmp;
  int cb;
  int result=-1;

  if ( (policy<DC1394_CAPTURE_POLICY_MIN) || (policy>DC1394_CAPTURE_POLICY_MAX) )
    return DC1394_INVALID_CAPTURE_POLICY;

  // default: return NULL in case of failures or lack of frames
  *frame=NULL;

  memset(&vwait, 0, sizeof(vwait));

  cb = (capture->dma_last_buffer + 1) % capture->num_dma_buffers;
  frame_tmp = capture->frames + cb;

  vwait.channel = craw->iso_channel;
  vwait.buffer = cb;
  switch (policy) {
    case DC1394_CAPTURE_POLICY_POLL:
      result=ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_POLL_BUFFER, &vwait);
      break;
    case DC1394_CAPTURE_POLICY_WAIT:
    default:
      result=ioctl(capture->dma_fd, VIDEO1394_IOC_LISTEN_WAIT_BUFFER, &vwait);
      break;
  }
  if ( result != 0) {       
    if ((policy==DC1394_CAPTURE_POLICY_POLL) && (errno == EINTR)) {                       
      // when no frames is present, just return
      return DC1394_SUCCESS;
    }
    else {
      printf("(%s) VIDEO1394_IOC_LISTEN_WAIT/POLL_BUFFER ioctl failed!\n", __FILE__);
      return DC1394_IOCTL_FAILURE;
    }
  }

  capture->dma_last_buffer = cb;

  frame_tmp->frames_behind = vwait.buffer;
  frame_tmp->timestamp = (uint64_t) vwait.filltime.tv_sec * 1000000 + vwait.filltime.tv_usec;

  *frame=frame_tmp;

  return DC1394_SUCCESS;
}

dc1394error_t
platform_capture_enqueue (platform_camera_t * craw,
			dc1394video_frame_t * frame)
{
  dc1394camera_t * camera = craw->camera;
  struct video1394_wait vwait;

  memset(&vwait, 0, sizeof(vwait));

  if (frame->camera != camera) {
    printf ("(%s) dc1394_capture_enqueue_dma: camera does not match frame's camera\n",
        __FILE__);
    return DC1394_INVALID_ARGUMENT_VALUE;
  }
  
  vwait.channel = craw->iso_channel;
  vwait.buffer = frame->id;
  
  if (ioctl(craw->capture.dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait) < 0)  {
    printf("(%s) VIDEO1394_IOC_LISTEN_QUEUE_BUFFER failed in done with buffer!\n", __FILE__);
    return DC1394_IOCTL_FAILURE;
  }
  
  return DC1394_SUCCESS;
}

int
platform_capture_get_fileno (platform_camera_t * craw)
{
  return craw->capture.dma_fd;
}

