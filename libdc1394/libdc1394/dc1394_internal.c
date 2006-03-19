/*
 * 1394-Based Digital Camera Control Library, internal functions
 * Copyright (C) Damien Douxchamps <ddouxchamps@users.sf.net>
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

#include <libraw1394/raw1394.h>
#include "dc1394_internal.h"
#include "dc1394_utils.h"
#include "dc1394_register.h"

extern int *_dc1394_dma_fd;
extern int *_dc1394_num_using_fd;

const char *dc1394_feature_desc[DC1394_FEATURE_NUM] =
{
  "Brightness",
  "Exposure",
  "Sharpness",
  "White Balance",
  "Hue",
  "Saturation",
  "Gamma",
  "Shutter",
  "Gain",
  "Iris",
  "Focus",
  "Temperature",
  "Trigger",
  "Trigger Delay",
  "White Shading",
  "Frame Rate",
  "Zoom",
  "Pan",
  "Tilt",
  "Optical Filter",
  "Capture Size",
  "Capture Quality"
};

const char *dc1394_error_strings[DC1394_ERROR_NUM] =
{
  "Success",
  "Generic failure",
  "No frame",
  "No camera",
  "This node is not a camera",
  "Function not supported by this camera",
  "Camera not initialized",
  "Invalid feature",
  "Invalid format",
  "Invalid mode",
  "Invalid framerate",
  "Invalid trigger mode",
  "Invalid trigger source",
  "Invalid ISO speed",
  "Invalid IIDC version",
  "Invalid Format_7 color coding",
  "Invalid Format_7 elementary Bayer tile",
  "Requested value is out of range",
  "Invalid error code",
  "Memory allocation failure",
  "Tagged register not found",
  "Format_7 Error_flag_1 is set",
  "Format_7 Error_flag_2 is set",
  "Invalid Bayer method code",
  "Could not create a raw1394 handle",
  "Invalid argument",
  "Invalid video1394 device filename",
  "Could not allocate an ISO channel"
  "Could not allocate bandwidth"
};

/*
  These arrays define how many image quadlets there
  are in a packet given a mode and a frame rate
  This is defined in the 1394 digital camera spec 
*/
const int quadlets_per_packet_format_0[56] = 
{
  -1,  -1,  15,  30,  60,  120,  240,  480,
  10,  20,  40,  80, 160,  320,  640, 1280,
  30,  60, 120, 240, 480,  960, 1920, 3840,
  40,  80, 160, 320, 640, 1280, 2560, 5120,
  60, 120, 240, 480, 960, 1920, 3840, 7680,
  20,  40,  80, 160, 320,  640, 1280, 2560,
  40,  80, 160, 320, 640, 1280, 2560, 5120
};
 
const int quadlets_per_packet_format_1[64] = 
{
   -1, 125, 250,  500, 1000, 2000, 4000, 8000,
   -1,  -1, 375,  750, 1500, 3000, 6000,   -1,
   -1,  -1, 125,  250,  500, 1000, 2000, 4000,
   96, 192, 384,  768, 1536, 3072, 6144,   -1,
  144, 288, 576, 1152, 2304, 4608,   -1,   -1,
   48,  96, 192,  384,  768, 1536, 3073, 6144,
   -1, 125, 250,  500, 1000, 2000, 4000, 8000,
   96, 192, 384,  768, 1536, 3072, 6144,   -1
};
 
const int quadlets_per_packet_format_2[64] = 
{
  160, 320,  640, 1280, 2560, 5120,   -1, -1,
  240, 480,  960, 1920, 3840, 7680,   -1, -1,
   80, 160,  320,  640, 1280, 2560, 5120, -1,
  250, 500, 1000, 2000, 4000, 8000,   -1, -1,
  375, 750, 1500, 3000, 6000,   -1,   -1, -1,
  125, 250,  500, 1000, 2000, 4000, 8000, -1, 
  160, 320,  640, 1280, 2560, 5120,   -1, -1,
  250, 500, 1000, 2000, 4000, 8000,   -1, -1
};

/********************************************************
 _dc1394_get_quadlets_per_packet

 This routine reports the number of useful image quadlets 
 per packet
*********************************************************/
dc1394error_t
_dc1394_get_quadlets_per_packet(dc1394video_mode_t mode, dc1394framerate_t frame_rate, uint_t *qpp) // ERROR handling to be updated
{
  uint_t mode_index;
  uint_t frame_rate_index= frame_rate - DC1394_FRAMERATE_MIN;
  uint_t format;
  dc1394error_t err;

  err=_dc1394_get_format_from_mode(mode, &format);
  DC1394_ERR_CHK(err,"Invalid mode ID");
  
  //fprintf(stderr,"format: %d\n",format);

  switch(format) {
  case DC1394_FORMAT0:
    mode_index= mode - DC1394_VIDEO_MODE_FORMAT0_MIN;
    
    if ( ((mode >= DC1394_VIDEO_MODE_FORMAT0_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT0_MAX)) && 
	 ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_0[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case DC1394_FORMAT1:
    mode_index= mode - DC1394_VIDEO_MODE_FORMAT1_MIN;
    
    if ( ((mode >= DC1394_VIDEO_MODE_FORMAT1_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT1_MAX)) && 
	 ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_1[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case DC1394_FORMAT2:
    mode_index= mode - DC1394_VIDEO_MODE_FORMAT2_MIN;
    
    if ( ((mode >= DC1394_VIDEO_MODE_FORMAT2_MIN) && (mode <= DC1394_VIDEO_MODE_FORMAT2_MAX)) && 
	 ((frame_rate >= DC1394_FRAMERATE_MIN) && (frame_rate <= DC1394_FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_2[DC1394_FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case DC1394_FORMAT6:
  case DC1394_FORMAT7:
    err=DC1394_INVALID_FORMAT;
    DC1394_ERR_CHK(err,"Format 6 and 7 don't have qpp");
    break;
  }

  return DC1394_FAILURE;
}

/**********************************************************
 _dc1394_quadlets_from_format

 This routine reports the number of quadlets that make up a 
 frame given the format and mode
***********************************************************/
dc1394error_t
_dc1394_quadlets_from_format(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint_t *quads) 
{

  uint_t w, h, color_coding;
  float bpp;
  dc1394error_t err;

  err=dc1394_get_image_size_from_video_mode(camera, video_mode, &w, &h);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  err=dc1394_get_color_coding_from_video_mode(camera, video_mode, &color_coding);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  err=dc1394_get_bytes_per_pixel(color_coding, &bpp);
  DC1394_ERR_CHK(err, "Invalid color mode ID");

  *quads=(uint_t)(w*h*bpp/4);

  return err;
}

dc1394bool_t
IsFeatureBitSet(quadlet_t value, dc1394feature_t feature)
{

  if (feature >= DC1394_FEATURE_ZOOM) {
    if (feature >= DC1394_FEATURE_CAPTURE_SIZE) {
      feature+= 12;
    }
    feature-= DC1394_FEATURE_ZOOM;
  }
  else {
    feature-= DC1394_FEATURE_MIN;
  }
  
  value&=(0x80000000UL >> feature);
  
  if (value>0)
    return DC1394_TRUE;
  else
    return DC1394_FALSE;
}

dc1394error_t
_dc1394_get_format_from_mode(dc1394video_mode_t mode, uint_t *format)
{
  dc1394error_t err=DC1394_SUCCESS;

  if ((mode>=DC1394_VIDEO_MODE_FORMAT0_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT0_MAX)) {
    *format=DC1394_FORMAT0;
  }
  else if ((mode>=DC1394_VIDEO_MODE_FORMAT1_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT1_MAX)) {
    *format=DC1394_FORMAT1;
  }
  else if ((mode>=DC1394_VIDEO_MODE_FORMAT2_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT2_MAX)) {
    *format=DC1394_FORMAT2;
  }
  else if ((mode>=DC1394_VIDEO_MODE_FORMAT6_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT6_MAX)) {
    *format=DC1394_FORMAT6;
  }
  else if ((mode>=DC1394_VIDEO_MODE_FORMAT7_MIN)&&(mode<=DC1394_VIDEO_MODE_FORMAT7_MAX)) {
    *format=DC1394_FORMAT7;
  }
  else {
    err=DC1394_INVALID_MODE;
    DC1394_ERR_CHK(err, "The supplied mode does not correspond to any format");
  }

  return err;
}

dc1394error_t
_dc1394_open_dma_device(dc1394camera_t *camera)
{
  char filename[64];

  // if the file has already been opened: increment the number of uses and return
  if (_dc1394_num_using_fd[camera->port] != 0) {
    _dc1394_num_using_fd[camera->port]++;
    camera->capture.dma_fd = _dc1394_dma_fd[camera->port];
    return DC1394_SUCCESS;
  }

  // if the dma device file has been set manually, use that device name
  if (camera->capture.dma_device_file!=NULL) {
    if ( (camera->capture.dma_fd = open(camera->capture.dma_device_file,O_RDONLY)) < 0 ) {
      return DC1394_INVALID_VIDEO1394_DEVICE;
    }
    else {
      _dc1394_dma_fd[camera->port] = camera->capture.dma_fd;
      _dc1394_num_using_fd[camera->port]++;
      return DC1394_SUCCESS;
    }
  }

  // automatic mode: try to open several usual device files.
  sprintf(filename,"/dev/video1394/%d",camera->port);
  if ( (camera->capture.dma_fd = open(filename,O_RDONLY)) >= 0 ) {
    _dc1394_dma_fd[camera->port] = camera->capture.dma_fd;
    _dc1394_num_using_fd[camera->port]++;
    return DC1394_SUCCESS;
  }

  sprintf(filename,"/dev/video1394-%d",camera->port);
  if ( (camera->capture.dma_fd = open(filename,O_RDONLY)) >= 0 ) {
    _dc1394_dma_fd[camera->port] = camera->capture.dma_fd;
    _dc1394_num_using_fd[camera->port]++;
    return DC1394_SUCCESS;
  }
  
  if (camera->port==0) {
    sprintf(filename,"/dev/video1394");
    if ( (camera->capture.dma_fd = open(filename,O_RDONLY)) >= 0 ) {
      _dc1394_dma_fd[camera->port] = camera->capture.dma_fd;
      _dc1394_num_using_fd[camera->port]++;
      return DC1394_SUCCESS;
    }
  }

  return DC1394_FAILURE;
}


dc1394error_t
dc1394_allocate_iso_channel_and_bandwidth(dc1394camera_t *camera)
{
  dc1394error_t err;
  int i;

  if (camera->capture_is_set==0) {
    // capture is not set, and thus channels/bandwidth have not been allocated.
    // We can safely allocate that in advance. 

    // first we need to assign an ISO channel:  
    if (camera->iso_channel_is_set==0){
      if (camera->iso_channel>=0) {
	// a specific channel is requested. try to book it.
	if (raw1394_channel_modify(camera->handle, camera->iso_channel, RAW1394_MODIFY_ALLOC)==0) {
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
	if (raw1394_channel_modify(camera->handle, i, RAW1394_MODIFY_ALLOC)==0) {
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
      DC1394_ERR_CHK(err, "Could not set ISO channel in the camera");
    }
    
    if (camera->iso_bandwidth==0) {
      //fprintf(stderr,"Estimating ISO bandwidth\n");
      // then we book the bandwidth
      err=dc1394_video_get_bandwidth_usage(camera, &camera->iso_bandwidth);
      DC1394_ERR_CHK(err, "Could not estimate ISO bandwidth");
      if (raw1394_bandwidth_modify(camera->handle, camera->iso_bandwidth, RAW1394_MODIFY_ALLOC)<0) {
	camera->iso_bandwidth=0;
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
  fprintf(stderr,"capture: %d ISO: %d\n",camera->capture_is_set,camera->is_iso_on);
  if ((camera->capture_is_set==0)&&(camera->is_iso_on==0)) {
    // capture is not set and transmission is not active: channels/bandwidth can be freed without interfering

    if (camera->iso_bandwidth>0) {
      // first free the bandwidth
      if (raw1394_bandwidth_modify(camera->handle, camera->iso_bandwidth, RAW1394_MODIFY_FREE)<0) {
	fprintf(stderr,"Error: could not free %d units of bandwidth!\n", camera->iso_bandwidth);
	return DC1394_FAILURE;
      }
      else {
	fprintf(stderr,"Freed %d bandwidth units\n",camera->iso_bandwidth);
	camera->iso_bandwidth=0;
      }
    }
    
    // then free the ISO channel if it was allocated
    if (camera->iso_channel_is_set>0) {
      if (raw1394_channel_modify(camera->handle, camera->iso_channel, RAW1394_MODIFY_FREE)==-1) {
	fprintf(stderr,"Error: could not free iso channel %d!\n",camera->iso_channel);
	return DC1394_FAILURE;
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
dc1394_video_set_iso_channel(dc1394camera_t *camera, uint_t channel)
{
  dc1394error_t err;
  quadlet_t value_inq, value;
  int speed;

  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
  DC1394_ERR_CHK(err, "Could not get basic function register");

  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_CHK(err, "Could not get ISO data");

  // check if 1394b is available and if we are now using 1394b
  if ((value_inq & 0x00800000)&&(value & 0x00008000)) {
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_CHK(err, "oops");
    speed=value & 0x7UL;
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (quadlet_t) ( ((channel & 0x3FUL) << 8) |
					       (speed & 0x7UL) |
					       (0x1 << 15) ));
    DC1394_ERR_CHK(err, "oops");
  }
  else { // fallback to legacy
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_CHK(err, "oops");
    speed=(value >> 24) & 0x3UL;
    if (speed>DC1394_ISO_SPEED_400-DC1394_ISO_SPEED_MIN) {
      fprintf(stderr,"(%s) line %d: an ISO speed >400Mbps was requested while the camera is in LEGACY mode\n",__FILE__,__LINE__);
      fprintf(stderr,"              Please set the operation mode to OPERATION_MODE_1394B before asking for\n");
      fprintf(stderr,"              1394b ISO speeds\n");
      return DC1394_FAILURE;
    }
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (quadlet_t) (((channel & 0xFUL) << 28) |
					      ((speed & 0x3UL) << 24) ));
    DC1394_ERR_CHK(err, "Could not set ISO data register");
  }
  
  return err;
}
