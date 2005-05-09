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

const char *dc1394_feature_desc[FEATURE_NUM] =
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
  "Format_7 Error_flag_2 is set",
  "Invalid Bayer method code"
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
int 
_dc1394_get_quadlets_per_packet(uint_t mode, uint_t frame_rate, uint_t *qpp) // ERROR handling to be updated
{
  uint_t mode_index;
  uint_t frame_rate_index= frame_rate - FRAMERATE_MIN;
  uint_t format;
  int err;

  err=_dc1394_get_format_from_mode(mode, &format);
  DC1394_ERR_CHK(err,"Invalid mode ID");
  
  //fprintf(stderr,"format: %d\n",format);

  switch(format) {
  case FORMAT0:
    mode_index= mode - MODE_FORMAT0_MIN;
    
    if ( ((mode >= MODE_FORMAT0_MIN) && (mode <= MODE_FORMAT0_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_0[FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case FORMAT1:
    mode_index= mode - MODE_FORMAT1_MIN;
    
    if ( ((mode >= MODE_FORMAT1_MIN) && (mode <= MODE_FORMAT1_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_1[FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case FORMAT2:
    mode_index= mode - MODE_FORMAT2_MIN;
    
    if ( ((mode >= MODE_FORMAT2_MIN) && (mode <= MODE_FORMAT2_MAX)) && 
	 ((frame_rate >= FRAMERATE_MIN) && (frame_rate <= FRAMERATE_MAX)) ) {
      *qpp=quadlets_per_packet_format_2[FRAMERATE_NUM*mode_index+frame_rate_index];
    }
    else {
      err=DC1394_INVALID_MODE;
      DC1394_ERR_CHK(err,"Invalid framerate (%d) or mode (%d)", frame_rate, mode);
    }
    return DC1394_SUCCESS;
  case FORMAT6:
  case FORMAT7:
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
int
_dc1394_quadlets_from_format(uint_t mode, uint_t *quads) 
{

  uint_t w, h, color_mode;
  float bpp;
  int err;

  err=dc1394_get_wh_from_mode(mode, &w, &h);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  err=dc1394_get_color_mode_from_mode(mode, &color_mode);
  DC1394_ERR_CHK(err, "Invalid mode ID");

  err=dc1394_get_bytes_per_pixel(color_mode, &bpp);
  DC1394_ERR_CHK(err, "Invalid color mode ID");

  *quads=(uint_t)(w*h*bpp/4);

  return err;
}

int
IsFeatureBitSet(quadlet_t value, uint_t feature)
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
_dc1394_get_format_from_mode(uint_t mode, uint_t *format)
{
  int err=DC1394_SUCCESS;

  if ((mode>=MODE_FORMAT0_MIN)&&(mode<=MODE_FORMAT0_MAX)) {
    *format=FORMAT0;
  }
  else if ((mode>=MODE_FORMAT1_MIN)&&(mode<=MODE_FORMAT1_MAX)) {
    *format=FORMAT1;
  }
  else if ((mode>=MODE_FORMAT2_MIN)&&(mode<=MODE_FORMAT2_MAX)) {
    *format=FORMAT2;
  }
  else if ((mode>=MODE_FORMAT6_MIN)&&(mode<=MODE_FORMAT6_MAX)) {
    *format=FORMAT6;
  }
  else if ((mode>=MODE_FORMAT7_MIN)&&(mode<=MODE_FORMAT7_MAX)) {
    *format=FORMAT7;
  }
  else {
    err=DC1394_INVALID_MODE;
    DC1394_ERR_CHK(err, "The supplied mode does not correspond to any format");
  }

  return err;
}

