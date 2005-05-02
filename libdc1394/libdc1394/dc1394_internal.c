/*
 * 1394-Based Digital Camera Control Library
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
#include "dc1394_register.h"

const char *dc1394_feature_desc[NUM_FEATURES] =
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

const char *dc1394_error_strings[NUM_ERRORS] =
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
  "Invalid ISO speed",
  "Invalid IIDC version",
  "Invalid Format_7 color coding",
  "Invalid Format_7 elementary Bayer tile",
  "Requested value is out of range",
  "Invalid error code",
  "Memory allocation failure",
  "Tagged register not found",
  "Format_7 Error_flag_1 is set",
  "Format_7 Error_flag_2 is set"
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

int
_dc1394_get_wh_from_format(int format, int mode, int *w, int *h) 
{
  switch(format) {
  case FORMAT_VGA_NONCOMPRESSED:
    switch(mode) {
    case MODE_160x120_YUV444:
      *w = 160;*h=120;
    case MODE_320x240_YUV422:
      *w = 320;*h=240;
    case MODE_640x480_YUV411:
    case MODE_640x480_YUV422:
    case MODE_640x480_RGB:
    case MODE_640x480_MONO:
    case MODE_640x480_MONO16:
      *w =640;*h=480;
    default:
      return DC1394_INVALID_MODE;
    }
  case FORMAT_SVGA_NONCOMPRESSED_1:
    switch(mode) {
    case MODE_800x600_YUV422:
    case MODE_800x600_RGB:
    case MODE_800x600_MONO:
    case MODE_800x600_MONO16:
      *w=800;*h=600;
    case MODE_1024x768_YUV422:
    case MODE_1024x768_RGB:
    case MODE_1024x768_MONO:
    case MODE_1024x768_MONO16:
      *w=1024;*h=768;
    default:
      return DC1394_INVALID_MODE;
    }
  case FORMAT_SVGA_NONCOMPRESSED_2:
    switch(mode) {
    case MODE_1280x960_YUV422:
    case MODE_1280x960_RGB:
    case MODE_1280x960_MONO:
    case MODE_1280x960_MONO16:
      *w=1280;*h=960;
    case MODE_1600x1200_YUV422:
    case MODE_1600x1200_RGB:
    case MODE_1600x1200_MONO:
    case MODE_1600x1200_MONO16:
      *w=1600;*h=1200;
    default:
      return DC1394_INVALID_MODE;
    }
  default:
    return DC1394_INVALID_FORMAT;
  }

  return DC1394_SUCCESS;
}
	
/********************************************************
 _dc1394_get_quadlets_per_packet

 This routine reports the number of useful image quadlets 
 per packet
*********************************************************/
int 
_dc1394_get_quadlets_per_packet(int format, int mode, int frame_rate) 
{
  int mode_index;
  int frame_rate_index= frame_rate - FRAMERATE_MIN;

  switch(format) {
  case FORMAT_VGA_NONCOMPRESSED:
    mode_index= mode - MODE_FORMAT0_MIN;
    
    if ( ((mode >= MODE_FORMAT0_MIN) && (mode <= MODE_FORMAT0_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      return quadlets_per_packet_format_0[NUM_FRAMERATES*mode_index+frame_rate_index];
    }
    printf("(%s) Invalid framerate (%d) or mode (%d)!\n", __FILE__, frame_rate, format);
    break;
  case FORMAT_SVGA_NONCOMPRESSED_1:
    mode_index= mode - MODE_FORMAT1_MIN;
    
    if ( ((mode >= MODE_FORMAT1_MIN) && (mode <= MODE_FORMAT1_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      return quadlets_per_packet_format_1[NUM_FRAMERATES*mode_index+frame_rate_index];
    }
    printf("(%s) Invalid framerate (%d) or mode (%d)!\n", __FILE__, frame_rate, format);
    break;
  case FORMAT_SVGA_NONCOMPRESSED_2:
    mode_index= mode - MODE_FORMAT2_MIN;
    
    if ( ((mode >= MODE_FORMAT2_MIN) && (mode <= MODE_FORMAT2_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      return quadlets_per_packet_format_2[NUM_FRAMERATES*mode_index+frame_rate_index];
    }
    printf("(%s) Invalid framerate (%d) or mode (%d)!\n", __FILE__, frame_rate, format);
    break;
  default:
    printf("(%s) Quadlets per packet unkown for format %d!\n", __FILE__, format);
    break;
  }
  
  return DC1394_FAILURE;
}

/**********************************************************
 _dc1394_quadlets_from_format

 This routine reports the number of quadlets that make up a 
 frame given the format and mode
***********************************************************/
int
_dc1394_quadlets_from_format(int format, int mode) 
{

  switch (format) {
  case FORMAT_VGA_NONCOMPRESSED:
    
    switch(mode) {
    case MODE_160x120_YUV444:
      return 14400;   //160x120*3/4
    case MODE_320x240_YUV422:
      return 38400;   //320x240/2
    case MODE_640x480_YUV411:
      return 115200;  //640x480x1.5/4
    case MODE_640x480_YUV422:
      return 153600;  //640x480/2
    case MODE_640x480_RGB:
      return 230400;  //640x480x3/4
    case MODE_640x480_MONO:
      return 76800;   //640x480/4
    case MODE_640x480_MONO16:
      return 153600;  //640x480/2
    default:
      printf("(%s) Improper mode specified: %d\n", __FILE__, mode);
      break;
    }
    
    break;
  case FORMAT_SVGA_NONCOMPRESSED_1: 
    
    switch(mode) {
    case MODE_800x600_YUV422:
      return 240000;  //800x600/2
    case MODE_800x600_RGB:
      return 360000;  //800x600x3/4
    case MODE_800x600_MONO:
      return 120000;  //800x600/4
    case MODE_1024x768_YUV422:
      return 393216;  //1024x768/2
    case MODE_1024x768_RGB:
      return 589824;  //1024x768x3/4
    case MODE_1024x768_MONO:
      return 196608;  //1024x768/4
    case MODE_800x600_MONO16:
      return 240000;  //800x600/2
    case MODE_1024x768_MONO16:
      return 393216;  //1024x768/2
    default:
      printf("(%s) Improper mode specified: %d\n", __FILE__, mode);
      break;
    }
    
    break;
  case FORMAT_SVGA_NONCOMPRESSED_2:
    
    switch (mode) {
    case MODE_1280x960_YUV422:
      return 614400;  //1280x960/2
    case MODE_1280x960_RGB:
      return 921600;  //1280x960x3/4
    case MODE_1280x960_MONO:
      return 307200;  //1280x960/4
    case MODE_1600x1200_YUV422:
      return 960000;  //1600x1200/2
    case MODE_1600x1200_RGB:
      return 1440000; //1600x1200x3/4
    case MODE_1600x1200_MONO:
      return 480000;  //1600x1200/4
    case MODE_1280x960_MONO16:
      return 614400;  //1280x960/2
    case MODE_1600x1200_MONO16:
      return 960000;  //1600x1200/2
    default:
      printf("(%s) Improper mode specified: %d\n", __FILE__, mode);
      break;
    }
    
    break;
  case FORMAT_STILL_IMAGE:
    printf("(%s) Don't know how many quadlets per frame for FORMAT_STILL_IMAGE mode:%d\n",
	   __FILE__, mode);
    break;
  case FORMAT_SCALABLE_IMAGE_SIZE:
    printf("(%s) Don't know how many quadlets per frame for FORMAT_SCALABLE_IMAGE mode:%d\n",
	   __FILE__, mode);
    break;
  default:
    printf("(%s) Improper format specified: %d\n", __FILE__, format);
    break;
  }
  
  return DC1394_FAILURE;
}

int
IsFeatureBitSet(quadlet_t value, unsigned int feature)
{

  if (feature >= FEATURE_ZOOM) {
    if (feature >= FEATURE_CAPTURE_SIZE) {
      feature+= 12;
    }
    feature-= FEATURE_ZOOM;
  }
  else {
    feature-= FEATURE_MIN;
  }
  
  value&=(0x80000000UL >> feature);
  
  if (value>0)
    return DC1394_TRUE;
  else
    return DC1394_FALSE;
}

int 
SetFeatureValue(dc1394camera_t *camera, unsigned int feature, unsigned int value)
{
  quadlet_t curval;
  octlet_t offset;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  if (GetCameraControlRegister(camera, offset, &curval) < 0) {
    return DC1394_FAILURE;
  }
  
  return  SetCameraControlRegister(camera, offset, (curval & 0xFFFFF000UL) | (value & 0xFFFUL));
}

int
GetFeatureValue(dc1394camera_t *camera, unsigned int feature, unsigned int *value)
{
  quadlet_t quadval;
  octlet_t offset;
  int retval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  retval= GetCameraControlRegister(camera, offset, &quadval);
  if (retval == DC1394_SUCCESS) {
    *value= (unsigned int)(quadval & 0xFFFUL);
  }
  
  return retval;
}
