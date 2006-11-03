/*
 * 1394-Based Digital Camera Format_7 functions for the Control Library
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
 
#include "control.h"
#include "internal.h"
#include "register.h"
#include "offsets.h"
#include "utils.h"
#include "config.h"

/**********************/ 
/* Internal functions */
/**********************/

/*==========================================================================
 * This function implements the handshaking available (and sometimes required)
 * on some cameras that comply with the IIDC specs v1.30. Thanks to Yasutoshi
 * Onishi for his feedback and info.
 *==========================================================================*/

dc1394error_t
dc1394_format7_get_value_setting(dc1394camera_t *camera,
				 dc1394video_mode_t video_mode,
				 uint32_t *present,
        			 uint32_t *setting1,
				 uint32_t *err_flag1,
				 uint32_t *err_flag2)
{
  dc1394error_t err;
  uint32_t value;
  
  if (camera->iidc_version<DC1394_IIDC_VERSION_1_30) {
    *present=0;
    return DC1394_SUCCESS;
  }

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_VALUE_SETTING, &value);
  DC1394_ERR_RTN(err, "could note get value setting");
  
  *present= (uint32_t) ( value & 0x80000000UL ) >> 31;
  *setting1= (uint32_t) ( value & 0x40000000UL ) >> 30;
  *err_flag1= (uint32_t) ( value & 0x00800000UL ) >> 23;
  *err_flag2= (uint32_t) ( value & 0x00400000UL ) >> 22;

  return err;
}

int
dc1394_format7_set_value_setting(dc1394camera_t *camera,
				 dc1394video_mode_t video_mode)
{
  int err;
  err=SetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_VALUE_SETTING, (uint32_t)0x40000000UL);
  DC1394_ERR_RTN(err, "Could not set value setting");

  return err;
}

dc1394error_t
_dc1394_v130_handshake(dc1394camera_t *camera, dc1394video_mode_t video_mode)
{
  uint32_t setting_1, err_flag1, err_flag2, v130handshake;
  uint32_t exit_loop;
  dc1394error_t err;

  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_30) {
    // We don't use > because 114 is for ptgrey cameras which are not 1.30 but 1.20
    err=dc1394_format7_get_value_setting(camera, video_mode, &v130handshake, &setting_1, &err_flag1, &err_flag2);
    DC1394_ERR_RTN(err, "Unable to read value setting register");
  }
  else {
    v130handshake=0;
    return DC1394_SUCCESS;
  }   
  
  if (v130handshake==1) {
    // we should use advanced IIDC v1.30 handshaking.
    //fprintf(stderr,"using handshaking\n");
    // set value setting to 1
    err=dc1394_format7_set_value_setting(camera, video_mode);
    DC1394_ERR_RTN(err, "Unable to set value setting register");

    // wait for value setting to clear:
    exit_loop=0;
    while (!exit_loop) { // WARNING: there is no timeout in this loop yet.
      err=dc1394_format7_get_value_setting(camera, video_mode, &v130handshake, &setting_1, &err_flag1, &err_flag2);
      DC1394_ERR_RTN(err, "Unable to read value setting register");

      exit_loop=(setting_1==0);
      usleep(0); 
    }
    if (err_flag1>0) {
      err=DC1394_FORMAT7_ERROR_FLAG_1;
      DC1394_ERR_RTN(err, "invalid image position, size, color coding, ISO speed or bpp");
    }
    
    // bytes per packet... registers are ready for reading.
  }

  return err;
}

dc1394error_t
_dc1394_v130_errflag2(dc1394camera_t *camera, dc1394video_mode_t video_mode)
{
  uint32_t setting_1, err_flag1, err_flag2, v130handshake;
  dc1394error_t err;

  //fprintf(stderr,"Checking error flags\n");

  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_30) { // if version is 1.30.
    // We don't use > because 0x114 is for ptgrey cameras which are not 1.30 but 1.20
    err=dc1394_format7_get_value_setting(camera, video_mode, &v130handshake,&setting_1, &err_flag1, &err_flag2);
    DC1394_ERR_RTN(err, "Unable to read value setting register");
  }
  else {
    v130handshake=0;
    return DC1394_SUCCESS;
  }
      
  if (v130handshake==1) {
    if (err_flag2==0)
      return DC1394_SUCCESS;
    else {
      err=DC1394_FORMAT7_ERROR_FLAG_2;
      DC1394_ERR_RTN(err, "proposed bytes per packet is not a valid value");
    }
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_format7_set_roi(dc1394camera_t *camera,
		       dc1394video_mode_t video_mode,
		       dc1394color_coding_t color_coding,
		       int bytes_per_packet,
		       int left, int top,
		       int width, int height)
{
  uint32_t unit_bytes, max_bytes;
  uint32_t recom_bpp;
  uint32_t camera_left = 0;
  uint32_t camera_top = 0;
  uint32_t camera_width = 0;
  uint32_t camera_height = 0;
  uint32_t max_width = 0;
  uint32_t max_height = 0;
  uint32_t uint_bpp=0;
  dc1394error_t err;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  //fprintf(stderr,"Setting ROI\n");

  if (color_coding==DC1394_QUERY_FROM_CAMERA) {
    err=dc1394_format7_get_color_coding(camera, video_mode, &color_coding);
    DC1394_ERR_RTN(err, "Unable to set color_coding %d", color_coding);
  }

  err=dc1394_format7_set_color_coding(camera, video_mode, color_coding);
  DC1394_ERR_RTN(err, "Unable to set color coding %d", color_coding);

  // get BPP before setting sizes,...
  if (bytes_per_packet==DC1394_QUERY_FROM_CAMERA) {
    err=dc1394_format7_get_byte_per_packet(camera, video_mode, &uint_bpp);
    DC1394_ERR_RTN(err, "Unable to get F7 bpp for mode %d", video_mode);
    bytes_per_packet=uint_bpp;
  }

  /* The image position should be checked regardless of the left and top values
     as we also use it for the size setting */
  err=dc1394_format7_get_image_position(camera, video_mode, &camera_left, &camera_top);
  DC1394_ERR_RTN(err, "Unable to query image position");

  /* First set image position to (0,0) to allow the size/position to change
     without passing through an impossible state. The order of the operations is
     1) set position to (0,0), 2) set size 3) set position. Other orders may fail
     to properly set the camera, even if the pos/size couple if OK. */
  err=dc1394_format7_set_image_position(camera, video_mode, 0,0);
  DC1394_ERR_RTN(err, "Unable to set image position");
    
  /* Then we have to change the image SIZE */

  /*-----------------------------------------------------------------------
   *  If QUERY_FROM_CAMERA was given instead of an image size
   *  use the actual value from camera.
   *-----------------------------------------------------------------------*/
  if ( width == DC1394_QUERY_FROM_CAMERA || height == DC1394_QUERY_FROM_CAMERA) {
    err=dc1394_format7_get_image_size(camera, video_mode, &camera_width, &camera_height);
    DC1394_ERR_RTN(err, "Unable to query image size");
    
    /* Idea from Ralf Ebeling: we should check if the image sizes are > 0.
       If == 0, we use the maximum size available */
    if (width == DC1394_QUERY_FROM_CAMERA) {
      if (camera_width>0)
	width = camera_width;
      else
	width = DC1394_USE_MAX_AVAIL;
    }
    if (height == DC1394_QUERY_FROM_CAMERA) {
      if (camera_height>0)
	height = camera_height;
      else
	height = DC1394_USE_MAX_AVAIL;
    }
  }

  /*-----------------------------------------------------------------------
   *  If USE_MAX_AVAIL was given instead of an image size
   *  use the max image size for the given image position
   *-----------------------------------------------------------------------*/
  if ( width == DC1394_USE_MAX_AVAIL || height == DC1394_USE_MAX_AVAIL) {
    err=dc1394_format7_get_max_image_size(camera, video_mode, &max_width, &max_height);
    DC1394_ERR_RTN(err, "Unable to query max image size");
    if( width == DC1394_USE_MAX_AVAIL)  width  = max_width - left;
    if( height == DC1394_USE_MAX_AVAIL) height = max_height - top;
  }

  err=dc1394_format7_set_image_size(camera, video_mode, width, height);
  DC1394_ERR_RTN(err, "Unable to set format 7 image size to [%d %d]",width, height);

  /* At last we change the image POSITION */
  /*-----------------------------------------------------------------------
   *  set image position. If QUERY_FROM_CAMERA was given instead of a
   *  position, use the actual value from camera
   *-----------------------------------------------------------------------*/

  if( left == DC1394_QUERY_FROM_CAMERA) left = camera_left;
  if( top == DC1394_QUERY_FROM_CAMERA)  top = camera_top;
  
  err=dc1394_format7_set_image_position(camera, video_mode, left, top);
  DC1394_ERR_RTN(err, "Unable to set format 7 image position to [%d %d]",left, top);

  /*-----------------------------------------------------------------------
   *  Bytes-per-packet definition
   *-----------------------------------------------------------------------*/
  err=dc1394_format7_get_recommended_byte_per_packet(camera, video_mode, &recom_bpp);
  DC1394_ERR_RTN(err, "Recommended byte-per-packet inq error");
  
  err=dc1394_format7_get_packet_para(camera, video_mode, &unit_bytes, &max_bytes); /* PACKET_PARA_INQ */
  DC1394_ERR_RTN(err, "Packet para inq error");

  switch (bytes_per_packet) {
  case DC1394_USE_RECOMMENDED:
    if (recom_bpp>0) {
      bytes_per_packet=recom_bpp;
    }
    else { // recom. bpp asked, but register is 0. IGNORED
      printf("(%s) Recommended bytes per packet asked, but register is zero. Falling back to MAX BPP for mode %d \n", __FILE__, video_mode);
      bytes_per_packet=max_bytes;
    }
    break;
  case DC1394_USE_MAX_AVAIL:
    bytes_per_packet = max_bytes;
    break;
  //this case was handled by a previous call. Show error if we get in there. 
  case DC1394_QUERY_FROM_CAMERA:
    // if we wanted QUERY_FROM_CAMERA, the QUERY_FROM_CAMERA value has been overwritten by
    // the current value at the beginning of the program. It is thus not possible to reach this code fragment.
    printf("(%s:%d) Bytes_per_packet error: we should not reach this code region\n", __FILE__,__LINE__);
    break;
  default:
    // we have to take several tricks into account here:
    // 1) BPP could be zero, in which case it becomes MAX_BPP
    // 2) UNIT_BYTES could also be zero, in which case we force it to MAX_BPP.
    //    This actually further forces BPP to be set to MAX_BPP too.

    if (unit_bytes==0) {
      unit_bytes=max_bytes;
    }
    if (bytes_per_packet > max_bytes) {
      bytes_per_packet = max_bytes;
    }
    else {
      if (bytes_per_packet < unit_bytes) {
	bytes_per_packet = unit_bytes;
      }
    }
    bytes_per_packet-=bytes_per_packet % unit_bytes;
    break;
  }
      
  err=dc1394_format7_set_byte_per_packet(camera, video_mode, bytes_per_packet);
  DC1394_ERR_RTN(err, "Unable to set format 7 bytes per packet for mode %d", video_mode);

  if (bytes_per_packet<=0) {
    printf("(%s) No format 7 bytes per packet %d \n", __FILE__, video_mode);
    return DC1394_FAILURE;
  }

  return err;
}


dc1394error_t
dc1394_format7_get_roi(dc1394camera_t *camera,
		       dc1394video_mode_t video_mode,
		       dc1394color_coding_t *color_coding,
		       uint32_t *bytes_per_packet,
		       uint32_t *left,  uint32_t *top,
		       uint32_t *width, uint32_t *height)
{
  dc1394error_t err;

  err=dc1394_format7_get_color_coding(camera, video_mode, color_coding);
  DC1394_ERR_RTN(err, "Unable to get color_coding %d", (int)color_coding);

  err=dc1394_format7_get_byte_per_packet(camera, video_mode, bytes_per_packet);
  DC1394_ERR_RTN(err, "Unable to get F7 bpp for mode %d", (int)video_mode);

  err=dc1394_format7_get_image_position(camera, video_mode, left, top);
  DC1394_ERR_RTN(err, "Unable to get image position");

  err=dc1394_format7_get_image_size(camera, video_mode, width, height);
  DC1394_ERR_RTN(err, "Unable to get image size");

  return err;
}


/**********************/
/* External functions */
/**********************/

dc1394error_t
dc1394_format7_get_max_image_size(dc1394camera_t *camera,
				  dc1394video_mode_t video_mode,
				  uint32_t *horizontal_size,
				  uint32_t *vertical_size)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get max image sizes");

  *horizontal_size  = (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_size= (uint32_t) ( value & 0x0000FFFFUL );
  
  //fprintf(stderr,"got size: %d %d\n", *horizontal_size, *vertical_size);

  return err;
}

dc1394error_t
dc1394_format7_get_unit_size(dc1394camera_t *camera,
			     dc1394video_mode_t video_mode,
			     uint32_t *horizontal_unit,
			     uint32_t *vertical_unit)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get unit sizes");

  *horizontal_unit  = (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_unit= (uint32_t) ( value & 0x0000FFFFUL );
  
  return err;
}

dc1394error_t
dc1394_format7_get_image_position(dc1394camera_t *camera,
				  dc1394video_mode_t video_mode,
				  uint32_t *left_position,
				  uint32_t *top_position)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_IMAGE_POSITION, &value);
  DC1394_ERR_RTN(err, "Could not get image position");

  *left_position = (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  *top_position= (uint32_t) ( value & 0x0000FFFFUL );       

  return err;
}


dc1394error_t
dc1394_format7_get_image_size(dc1394camera_t *camera,
			      dc1394video_mode_t video_mode,
			      uint32_t *width,
			      uint32_t *height)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_IMAGE_SIZE, &value);
  DC1394_ERR_RTN(err, "could not get current image size");

  *width= (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  *height = (uint32_t) ( value & 0x0000FFFFUL );       
  
  return err;
}

dc1394error_t
dc1394_format7_get_color_coding(dc1394camera_t *camera,
				dc1394video_mode_t video_mode, 
				dc1394color_coding_t *color_coding)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_COLOR_CODING_ID, &value);
  DC1394_ERR_RTN(err, "Could not get current color_id");

  value=value>>24;
  *color_coding= (uint32_t)value+DC1394_COLOR_CODING_MIN;
  
  return err;
}

dc1394error_t
dc1394_format7_get_color_codings(dc1394camera_t *camera, 
				 dc1394video_mode_t video_mode,
				 dc1394color_codings_t *color_codings)
{
  dc1394error_t err;
  int i;
  uint32_t value;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_COLOR_CODING_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get available color codings");

  //fprintf(stderr,"codings reg: 0x%x\n",value);

  color_codings->num=0;
  for (i=0;i<DC1394_COLOR_CODING_NUM;i++) {
    //fprintf(stderr,"test: 0x%x\n",(value & (0x1 << (31-i))));
    if ((value & (0x1 << (31-i))) > 0) {
      //fprintf(stderr,"got one coding\n");
      color_codings->codings[color_codings->num]=i+DC1394_COLOR_CODING_MIN;
      color_codings->num++;
    }
  }
  
  //fprintf(stderr,"number of modes: %d\n",modes->num);
  return err;
}
 
dc1394error_t
dc1394_format7_get_pixel_number(dc1394camera_t *camera,
 				dc1394video_mode_t video_mode,
				uint32_t *pixnum)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get pixel number");

  *pixnum= (uint32_t) value;
  
  return err;
}

dc1394error_t
dc1394_format7_get_total_bytes(dc1394camera_t *camera,
			       dc1394video_mode_t video_mode,
			       uint64_t *total_bytes)
{
  dc1394error_t err;
  uint64_t value_hi, value_lo;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get total bytes - LSB");

  value_hi=value;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get total bytes - MSB");

  value_lo=value;
  
  *total_bytes= (value_lo | ( value_hi << 32) ); 
  
  return err;
}

dc1394error_t
dc1394_format7_get_packet_para(dc1394camera_t *camera,
			       dc1394video_mode_t video_mode, uint32_t *min_bytes,
			       uint32_t *max_bytes)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err= GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_PACKET_PARA_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get F7 packet parameters");

  *min_bytes= (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  *max_bytes= (uint32_t) ( value & 0x0000FFFFUL );       
  
  return err;
}

dc1394error_t
dc1394_format7_get_byte_per_packet(dc1394camera_t *camera,
				   dc1394video_mode_t video_mode,
				   uint32_t *packet_bytes)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  DC1394_ERR_RTN(err, "Could not get bytes per packet");

  *packet_bytes= (uint32_t) ( value & 0xFFFF0000UL ) >> 16;
  
  if (packet_bytes==0) {
    printf("(%s): BYTES_PER_PACKET is zero. This should not happen.\n", __FILE__);
    return DC1394_FAILURE;
  }
  return err;
}

dc1394error_t
dc1394_format7_set_image_position(dc1394camera_t *camera,
 				  dc1394video_mode_t video_mode, uint32_t left,
 				  uint32_t top)
{
  dc1394error_t err;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=SetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_IMAGE_POSITION, (uint32_t)((left << 16) | top));
  DC1394_ERR_RTN(err, "Format7 image position setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, video_mode);
  DC1394_ERR_RTN(err, "F7 handshake failure");

  return err;
}

dc1394error_t
dc1394_format7_set_image_size(dc1394camera_t *camera,
                              dc1394video_mode_t video_mode, uint32_t width,
                              uint32_t height)
{
  dc1394error_t err;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=SetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_IMAGE_SIZE, (uint32_t)((width << 16) | height));
  DC1394_ERR_RTN(err, "Format7 image size setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, video_mode);
  DC1394_ERR_RTN(err, "F7 handshake failure");

  return err;
}

dc1394error_t
dc1394_format7_set_color_coding(dc1394camera_t *camera,
				dc1394video_mode_t video_mode, dc1394color_coding_t color_coding)
{
  dc1394error_t err;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  color_coding-= DC1394_COLOR_CODING_MIN;
  color_coding=color_coding<<24;
  err=SetCameraFormat7Register(camera, video_mode,REG_CAMERA_FORMAT7_COLOR_CODING_ID, (uint32_t)color_coding);
  DC1394_ERR_RTN(err, "Format7 color coding ID setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, video_mode);
  DC1394_ERR_RTN(err, "F7 handshake failure");

  return err;
}

dc1394error_t
dc1394_format7_set_byte_per_packet(dc1394camera_t *camera,
 				   dc1394video_mode_t video_mode,
                                   uint32_t packet_bytes)
{
  dc1394error_t err;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=SetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, (uint32_t)(packet_bytes) << 16 );
  DC1394_ERR_RTN(err, "Format7 bytes-per-packet setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, video_mode);
  DC1394_ERR_RTN(err, "F7 handshake failure");

  return err;
}

dc1394error_t
dc1394_format7_get_recommended_byte_per_packet(dc1394camera_t *camera,
					       dc1394video_mode_t video_mode, uint32_t *bpp)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  err= GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  DC1394_ERR_RTN(err, "Could not get recommended BPP");

  *bpp= (uint32_t) ( value & 0x0000FFFFUL );
  
  return err;
}

dc1394error_t
dc1394_format7_get_packet_per_frame(dc1394camera_t *camera,
				    dc1394video_mode_t video_mode, uint32_t *ppf)
{
  dc1394error_t err;
  uint32_t value;
  uint32_t packet_bytes;
  uint64_t total_bytes;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
    
  *ppf = 0;
  if (camera->iidc_version>=DC1394_IIDC_VERSION_1_30) {

    err= GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_PACKET_PER_FRAME_INQ, &value);
    DC1394_ERR_RTN(err, "Could not get the number of packets per frame");
    *ppf= (uint32_t) (value);
  }

  /* Perform computation according to 4.9.7 in the IIDC spec for cameras that do
   * not support PACKET_PER_FRAME_INQ */
  if (*ppf == 0) {
    // return an estimate, NOT TAKING ANY PADDING INTO ACCOUNT
    err=dc1394_format7_get_byte_per_packet(camera, video_mode, &packet_bytes);
    DC1394_ERR_RTN(err, "Could not get BPP");

    if (packet_bytes==0) {
      return DC1394_FAILURE;
    }

    err=dc1394_format7_get_total_bytes(camera, video_mode, &total_bytes);
    DC1394_ERR_RTN(err, "Could not get total number of bytes");

    if (total_bytes%packet_bytes!=0)
      *ppf=total_bytes/packet_bytes+1;
    else
      *ppf=total_bytes/packet_bytes;
    
    return err;
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_format7_get_unit_position(dc1394camera_t *camera,
				 dc1394video_mode_t video_mode,
				 uint32_t *horizontal_pos,
				 uint32_t *vertical_pos)
{
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  if (camera->iidc_version>=DC1394_IIDC_VERSION_1_30) {
    err = GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_UNIT_POSITION_INQ, &value);
    DC1394_ERR_RTN(err, "Could not get unit position");
  }
  else {
    // if version is not 1.30, use the UNIT_SIZE_INQ register
    err = GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
    DC1394_ERR_RTN(err, "Could not get unit size");
  }
  
  *horizontal_pos = (uint32_t) (( value & 0xFFFF0000UL )>>16);
  *vertical_pos   = (uint32_t) ( value & 0x0000FFFFUL );
  
  return err;
}

dc1394error_t
dc1394_format7_get_frame_interval(dc1394camera_t *camera,
				  dc1394video_mode_t video_mode, float *interval)
{   
  dc1394error_t err;
  uint32_t value;
  
  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_FRAME_INTERVAL_INQ, &value);
  DC1394_ERR_RTN(err, "Could not get frame interval");

  *interval=value;
  
  return err;
}   
    
dc1394error_t
dc1394_format7_get_data_depth(dc1394camera_t *camera,
			      dc1394video_mode_t video_mode, uint32_t *data_depth)
{   
  dc1394error_t err;
  uint32_t value;
  dc1394color_coding_t coding;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  *data_depth = 0;
  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_31) {
    err=GetCameraFormat7Register(camera, video_mode,
        REG_CAMERA_FORMAT7_DATA_DEPTH_INQ, &value);
    printf ("format7 raw depth %08x\n", value);
    DC1394_ERR_RTN(err, "Could not get format7 data depth");
    *data_depth=value >> 24;
  }

  /* For cameras that do not have the DATA_DEPTH_INQ register, perform a
   * sane default. */
  if (*data_depth == 0) {
    err = dc1394_get_color_coding_from_video_mode (camera, video_mode, &coding);
    DC1394_ERR_RTN(err, "Could not get color coding");
    
    err = dc1394_get_color_coding_depth (coding, data_depth);
    DC1394_ERR_RTN(err, "Could not get data depth from color coding");

    return err;
  }

  return DC1394_SUCCESS;
}   
    
dc1394error_t
dc1394_format7_get_color_filter(dc1394camera_t *camera,
				dc1394video_mode_t video_mode, dc1394color_filter_t *color_filter)
{   
  dc1394error_t err;
  uint32_t value;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  if (camera->iidc_version<DC1394_IIDC_VERSION_1_31)
    return DC1394_FUNCTION_NOT_SUPPORTED;
  
  err=GetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID, &value);
  DC1394_ERR_RTN(err, "Could not get color filter ID");

  *color_filter= (value >> 24)+DC1394_COLOR_FILTER_MIN;
  return err;
}   
     
dc1394error_t
dc1394_format7_set_color_filter(dc1394camera_t *camera,
				dc1394video_mode_t video_mode, dc1394color_filter_t color_filter)
{   
  dc1394error_t err;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;
  
  if (camera->iidc_version<DC1394_IIDC_VERSION_1_31)
    return DC1394_FUNCTION_NOT_SUPPORTED;

  if ((color_filter<DC1394_COLOR_FILTER_MIN)||(color_filter>DC1394_COLOR_FILTER_MAX))
    return DC1394_INVALID_COLOR_FILTER;

  color_filter -= DC1394_COLOR_FILTER_MIN;
  color_filter <<= 24;

  err=SetCameraFormat7Register(camera, video_mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID,
			       color_filter);
  DC1394_ERR_RTN(err, "Could not set color filter ID");

  return err;
}   

dc1394error_t
dc1394_format7_get_mode_info(dc1394camera_t *camera,
			     dc1394video_mode_t video_mode, dc1394format7mode_t *f7_mode)
{
  dc1394error_t err=DC1394_SUCCESS;

  if (!dc1394_is_video_mode_scalable(video_mode))
    return DC1394_INVALID_VIDEO_MODE;

  if (f7_mode->present>0) { // check for mode presence before query
    err=dc1394_format7_get_max_image_size(camera,video_mode,&f7_mode->max_size_x,&f7_mode->max_size_y);
    DC1394_ERR_RTN(err,"Got a problem querying format7 max image size");
    err=dc1394_format7_get_unit_size(camera,video_mode,&f7_mode->unit_size_x,&f7_mode->unit_size_y);
    DC1394_ERR_RTN(err,"Got a problem querying format7 unit size");
    err=dc1394_format7_get_unit_position(camera,video_mode,&f7_mode->unit_pos_x,&f7_mode->unit_pos_y);
    if (err!=DC1394_SUCCESS) {
      //DC1394_ERR_RTN(err,"Got a problem querying format7 unit position");
      // unit position might not be supported, hence a softer check is implemented
      f7_mode->unit_pos_x=0;
      f7_mode->unit_pos_y=0;
    }

    err=dc1394_format7_get_image_position(camera,video_mode,&f7_mode->pos_x,&f7_mode->pos_y);
    DC1394_ERR_RTN(err,"Got a problem querying format7 image position");
    err=dc1394_format7_get_image_size(camera,video_mode,&f7_mode->size_x,&f7_mode->size_y);
    DC1394_ERR_RTN(err,"Got a problem querying format7 image size");
    err=dc1394_format7_get_byte_per_packet(camera,video_mode,&f7_mode->bpp);
    DC1394_ERR_RTN(err,"Got a problem querying format7 bytes per packet");
    
    if (f7_mode->bpp==0) {
      // sometimes a camera will not set the bpp register until a valid image size
      // has been set after boot. If BPP is zero, we therefor
      // try again after setting the image size to the maximum size.
      err=dc1394_format7_set_image_position(camera,video_mode,0,0);
      DC1394_ERR_RTN(err,"Got a problem setting format7 image position");
      err=dc1394_format7_set_image_size(camera,video_mode,f7_mode->max_size_x,f7_mode->max_size_y);
      DC1394_ERR_RTN(err,"Got a problem setting format7 image size");
      // maybe we should also force a color coding here.
      err=dc1394_format7_get_byte_per_packet(camera,video_mode,&f7_mode->bpp);
      DC1394_ERR_RTN(err,"Got a problem querying format7 bytes per packet");
    }
    
    err=dc1394_format7_get_packet_para(camera,video_mode,&f7_mode->min_bpp,&f7_mode->max_bpp);
    DC1394_ERR_RTN(err,"Got a problem querying format7 packet parameters");
    err=dc1394_format7_get_pixel_number(camera,video_mode,&f7_mode->pixnum);
    DC1394_ERR_RTN(err,"Got a problem querying format7 pixel number");
    err=dc1394_format7_get_total_bytes(camera,video_mode,&f7_mode->total_bytes);
    DC1394_ERR_RTN(err,"Got a problem querying format7 total bytes per frame");
    err=dc1394_format7_get_color_coding(camera,video_mode,&f7_mode->color_coding);
    DC1394_ERR_RTN(err,"Got a problem querying format7 color coding ID");
    err=dc1394_format7_get_color_codings(camera,video_mode,&f7_mode->color_codings);
    DC1394_ERR_RTN(err,"Got a problem querying format7 color coding");

    // WARNING: this requires to set the format7 mode!!
    if (camera->iidc_version >= DC1394_IIDC_VERSION_1_31) {
      err=dc1394_format7_get_color_filter(camera,video_mode,&f7_mode->color_filter);
      DC1394_ERR_RTN(err,"Got a problem querying format7 bayer pattern");
    }
    else {
      f7_mode->color_filter = 0;
    }

    //fprintf(stderr,"# color codings for mode %d: %d\n", video_mode, mode->color_codings.num);
  }

  return err;
}

dc1394error_t
dc1394_format7_get_modeset(dc1394camera_t *camera, dc1394format7modeset_t *info)
{
  dc1394error_t err;
  int i;
  dc1394video_modes_t modes;

  for (i=0;i<DC1394_VIDEO_MODE_FORMAT7_NUM;i++) {
    info->mode[i].present=0;
  }

  err=dc1394_video_get_supported_modes(camera, &modes);
  DC1394_ERR_RTN(err,"Could not query supported formats");

  // find a mode which is F7:
  for (i=0;i<modes.num;i++) {
    if (dc1394_is_video_mode_scalable(modes.modes[i])) {
      info->mode[modes.modes[i]-DC1394_VIDEO_MODE_FORMAT7_MIN].present= 1;
      dc1394_format7_get_mode_info(camera, modes.modes[i], &info->mode[modes.modes[i]-DC1394_VIDEO_MODE_FORMAT7_MIN]);
      //fprintf(stderr,"# color codings for mode %d: %d\n", modes.modes[i], info->mode[modes.modes[i]-DC1394_VIDEO_MODE_FORMAT7_MIN].color_codings.num);
    }
  }

  return err;
}
