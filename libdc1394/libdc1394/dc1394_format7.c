/*
 * 1394-Based Digital Camera Format_7 functions for the Control Library
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
 
#include "dc1394_control.h"
#include "dc1394_internal.h"
#include "dc1394_register.h"
#include "dc1394_offsets.h"
#include "config.h"

/**********************/ 
/* Internal functions */
/**********************/

/*==========================================================================
 * This function implements the handshaking available (and sometimes required)
 * on some cameras that comply with the IIDC specs v1.30. Thanks to Yasutoshi
 * Onishi for his feedback and info.
 *==========================================================================*/

int
_dc1394_v130_handshake(dc1394camera_t *camera, int mode)
{
  int setting_1, err_flag1, err_flag2, v130handshake;
  int exit_loop;

  if (camera->sw_version >= IIDC_VERSION_1_30) {
    // We don't use > because 114 is for ptgrey cameras which are not 1.30 but 1.20
    if (dc1394_query_format7_value_setting(camera, mode, &v130handshake,
					   &setting_1, &err_flag1, &err_flag2)
	!= DC1394_SUCCESS) {
      printf("(%s) Unable to read value setting register.\n", __FILE__);
      return DC1394_FAILURE;
    }
  }
  else {
    v130handshake=0;
  }   
  
  if (v130handshake==1) {
    // we should use advanced IIDC v1.30 handshaking.
    //fprintf(stderr,"using handshaking\n");
    // set value setting to 1
    if (dc1394_set_format7_value_setting(camera, mode) != DC1394_SUCCESS) {
      printf("(%s) Unable to set value setting register.\n", __FILE__);
      return DC1394_FAILURE;
    }
    // wait for value setting to clear:
    exit_loop=0;
    while (!exit_loop) { // WARNING: there is no timeout in this loop yet.
      if (dc1394_query_format7_value_setting(camera, mode, &v130handshake,
					     &setting_1, &err_flag1, &err_flag2)
	  != DC1394_SUCCESS) {
	printf("(%s) Unable to read value setting register.\n", __FILE__);
	return DC1394_FAILURE;
      }
      exit_loop=(setting_1==0);
      usleep(0); 
    }
    if (err_flag1>0) {
      printf("(%s) Invalid image position, size, color coding, ISO speed or bpp\n", __FILE__);
      return DC1394_FAILURE;
    }
    
    // bytes per packet... registers are ready for reading.
  }
  return DC1394_SUCCESS;
}

int
_dc1394_v130_errflag2(dc1394camera_t *camera, int mode)
{
  int setting_1, err_flag1, err_flag2, v130handshake;

  //fprintf(stderr,"Checking error flags\n");

  if (camera->sw_version >= IIDC_VERSION_1_30) { // if version is 1.30.
    // We don't use > because 0x114 is for ptgrey cameras which are not 1.30 but 1.20
    if (dc1394_query_format7_value_setting(camera, mode, &v130handshake,
					   &setting_1, &err_flag1, &err_flag2)
	!= DC1394_SUCCESS) {
      printf("(%s) Unable to read value setting register.\n", __FILE__);
      return DC1394_FAILURE;
    }
  }
  else {
    v130handshake=0;
  }
      
  if (v130handshake==1) {
    if (err_flag2==0)
      return DC1394_SUCCESS;
    else {
      printf("(%s) Error flag 2 is set: proposed bytes per packet is not a valid value.\n", __FILE__);
      return DC1394_FAILURE;
    }
  }

  return DC1394_SUCCESS;
}

/*=================================================================================*
 * The following function returns the bytes per pixel for a defined color coding.  *
 *=================================================================================*/

float
_Format7BytePerPixel(int color_coding)
{
  switch (color_coding) {
  case COLOR_FORMAT7_MONO8: return 1.0;
    break;
  case COLOR_FORMAT7_YUV411: return 1.5;
    break;
  case COLOR_FORMAT7_YUV422: return 2.0;
    break;
  case COLOR_FORMAT7_YUV444: return 3.0;
    break;
  case COLOR_FORMAT7_RGB8: return 3.0;
    break;
  case COLOR_FORMAT7_MONO16: return 2.0;
    break;
  case COLOR_FORMAT7_RGB16: return 6.0;
    break;
  case COLOR_FORMAT7_MONO16S: return 2.0;
    break;
  case COLOR_FORMAT7_RGB16S: return 6.0;
    break;
  case COLOR_FORMAT7_RAW8: return 1.0;
      break;
  case COLOR_FORMAT7_RAW16: return 2.0;
    break;
  default: return (-1);
  }
}

 
/*======================================================================
 *
 *   see documentation of dc1394_setup_format7_capture() in
 *   dc1394_control.h
 *
 *======================================================================*/
int
_dc1394_basic_format7_setup(dc1394camera_t *camera,
                            int channel, int mode, int speed,
                            int bytes_per_packet,
                            int left, int top,
                            int width, int height, 
                            dc1394capture_t *capture)
{
  dc1394bool_t is_iso_on= DC1394_FALSE;
  unsigned int unit_bytes, max_bytes;
  unsigned packet_bytes=0;
  unsigned int recom_bpp;
  int packets_per_frame;
  int color_coding;
  unsigned int camera_left = 0;
  unsigned int camera_top = 0;
  unsigned int camera_width = 0;
  unsigned int camera_height = 0;
  unsigned int max_width = 0;
  unsigned int max_height = 0;

  if (dc1394_get_iso_status(camera, &is_iso_on) != DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }

  if (is_iso_on) {
    if (dc1394_stop_iso_transmission(camera) != DC1394_SUCCESS) {
      printf("(%s) Unable to stop iso transmission!\n", __FILE__);
      return DC1394_FAILURE;
    }
  }

  if (dc1394_set_iso_channel_and_speed(camera,channel,speed)!=DC1394_SUCCESS) {
    printf("(%s) Unable to set channel %d and speed %d!\n",__FILE__,channel,speed);
    return DC1394_FAILURE;
  }

  if (dc1394_set_video_format(camera,FORMAT_SCALABLE_IMAGE_SIZE)!=DC1394_SUCCESS) {
    printf("(%s) Unable to set video format %d!\n",__FILE__, FORMAT_SCALABLE_IMAGE_SIZE);
    return DC1394_FAILURE;
  }
  
  if (dc1394_set_video_mode(camera,mode) != DC1394_SUCCESS) {
    printf("(%s) Unable to set video mode %d!\n", __FILE__, mode);
    return DC1394_FAILURE;
  }

  // get BPP before setting sizes,...
  if (bytes_per_packet==QUERY_FROM_CAMERA) {
    if (dc1394_query_format7_byte_per_packet(camera, mode, &bytes_per_packet) != DC1394_SUCCESS){
      printf("(%s) Unable to get F7 bpp %d!\n", __FILE__, mode);
      return DC1394_FAILURE;
    }
  }

  /*-----------------------------------------------------------------------
   *  set image position. If QUERY_FROM_CAMERA was given instead of a
   *  position, use the actual value from camera
   *-----------------------------------------------------------------------*/

  /* The image position should be checked regardless of the left and top values
     as we also use it for the size setting */

  if (dc1394_query_format7_image_position(camera, mode, &camera_left, &camera_top) != DC1394_SUCCESS) {
    printf("(%s) Unable to query image position\n", __FILE__);
    return DC1394_FAILURE;
  }
    
  if( left == QUERY_FROM_CAMERA) left = camera_left;
  if( top == QUERY_FROM_CAMERA)  top = camera_top;
  
  if (dc1394_set_format7_image_position(camera, mode, left, top) != DC1394_SUCCESS) {
    printf("(%s) Unable to set format 7 image position to "
           "left=%d and top=%d!\n",  __FILE__, left, top);
    return DC1394_FAILURE;
  }

  /*-----------------------------------------------------------------------
   *  If QUERY_FROM_CAMERA was given instead of an image size
   *  use the actual value from camera.
   *-----------------------------------------------------------------------*/
  if ( width == QUERY_FROM_CAMERA || height == QUERY_FROM_CAMERA) {
    if (dc1394_query_format7_image_size(camera, mode, &camera_width, &camera_height) != DC1394_SUCCESS) {
      printf("(%s) Unable to query image size\n", __FILE__);
      return DC1394_FAILURE;
    }
    
    /* Idea from Ralf Ebeling: we should check if the image sizes are > 0.
       If == 0, we use the maximum size available */
    if (width == QUERY_FROM_CAMERA) {
      if (camera_width>0)
	width = camera_width;
      else
	width = USE_MAX_AVAIL;
    }
    if (height == QUERY_FROM_CAMERA) {
      if (camera_height>0)
	height = camera_height;
      else
	height = USE_MAX_AVAIL;
    }
  }

  /*-----------------------------------------------------------------------
   *  If USE_MAX_AVAIL was given instead of an image size
   *  use the max image size for the given image position
   *-----------------------------------------------------------------------*/
  if ( width == USE_MAX_AVAIL || height == USE_MAX_AVAIL) {
    if (dc1394_query_format7_max_image_size(camera, mode, &max_width, &max_height) != DC1394_SUCCESS) {
      printf("(%s) Unable to query max image size\n", __FILE__);
      return DC1394_FAILURE;
    }
    if( width == USE_MAX_AVAIL)  width  = max_width - left;
    if( height == USE_MAX_AVAIL) height = max_height - top;
    
  }

  if (dc1394_set_format7_image_size(camera, mode, width, height) != DC1394_SUCCESS) {
    printf("(%s) Unable to set format 7 image size to width=%d and height=%d!\n", __FILE__, width, height);
    return DC1394_FAILURE;
  }

  /*-----------------------------------------------------------------------
   *  Bytes-per-packet definition
   *-----------------------------------------------------------------------*/
  if (dc1394_query_format7_recommended_byte_per_packet(camera, mode, &recom_bpp) != DC1394_SUCCESS) {
    printf("Recommended byte-per-packet inq error\n");
    return DC1394_FAILURE;
  }
  
  if (dc1394_query_format7_packet_para(camera, mode, &unit_bytes, &max_bytes) != DC1394_SUCCESS) { /* PACKET_PARA_INQ */
    printf("Packet para inq error\n");
    return DC1394_FAILURE;
  }

  //fprintf(stderr,"recommended bpp: %d\n",recom_bpp);
  
  switch (bytes_per_packet) {
  case USE_RECOMMENDED:
    if (recom_bpp>0) {
      bytes_per_packet=recom_bpp;
    }
    else { // recom. bpp asked, but register is 0. IGNORED
      printf("(%s) Recommended bytes per packet asked, but register is zero. Falling back to MAX BPP for mode %d \n", __FILE__, mode);
      bytes_per_packet=max_bytes;
    }
    break;
  case USE_MAX_AVAIL:
    bytes_per_packet = max_bytes;
    break;
  //this case was handled by a previous call. Show error if we get in there. 
  case QUERY_FROM_CAMERA:
    /*if (dc1394_query_format7_byte_per_packet(camera, mode, &bytes_per_packet) != DC1394_SUCCESS) {
      printf("(%s) Bytes_per_packet query failure\n", __FILE__);
      return DC1394_FAILURE;
    }*/
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
      
  if (dc1394_set_format7_byte_per_packet(camera, mode, bytes_per_packet) != DC1394_SUCCESS) {
    printf("(%s) Unable to set format 7 bytes per packet %d \n", __FILE__, mode);
    return DC1394_FAILURE;
  }
  
  if (dc1394_query_format7_byte_per_packet(camera, mode, &packet_bytes) == DC1394_SUCCESS) {
    capture->quadlets_per_packet= packet_bytes /4;
    if (capture->quadlets_per_packet<=0) {
      printf("(%s) No format 7 bytes per packet %d \n", __FILE__, mode);
      return DC1394_FAILURE;
    }
    //printf("Camera has now %d bytes per packet\n", packet_bytes);
  }
  else {
    printf("(%s) Unable to get format 7 bytes per packet %d \n", __FILE__, mode);
    return DC1394_FAILURE;
  }
  
  capture->node = camera->node;
  /*
   * TODO: frame_rate not used for format 7, may be calculated
   */
  capture->frame_rate = 0; 
  capture->channel=channel;
  
  /*-----------------------------------------------------------------------
   *  ensure that quadlet aligned buffers are big enough, still expect
   *  problems when width*height  != quadlets_per_frame*4
   *-----------------------------------------------------------------------*/
  if (camera->sw_version >= IIDC_VERSION_1_30) { // if version is 1.30
    if (dc1394_query_format7_packet_per_frame(camera, mode, &packets_per_frame)!=DC1394_SUCCESS) {
      printf("(%s) Unable to get format 7 packets per frame %d \n", __FILE__, mode);
      return DC1394_FAILURE;
    }
    capture->quadlets_per_frame=(packets_per_frame*packet_bytes)/4;
  }
  else {
    // For other specs revisions, we use a trick to determine the total bytes.
    // We don't use the total_bytes register in 1.20 as it has been interpreted in
    // different ways by manufacturers. Thanks to Martin Gramatke for pointing this trick out.
    if (dc1394_query_format7_color_coding_id(camera, mode, &color_coding)!=DC1394_SUCCESS) {
      printf("(%s) Unable to get format 7 color coding for mode %d \n", __FILE__, mode);
      return DC1394_FAILURE;
    }
    else {
      //fprintf(stderr,"color coding: %d\n",color_coding);
      packets_per_frame = ((int)(width * height * _Format7BytePerPixel(color_coding)) +
			   bytes_per_packet -1) / bytes_per_packet;
      capture->quadlets_per_frame=(packets_per_frame*bytes_per_packet)/4;
    }
    /*
      if (dc1394_query_format7_total_bytes(camera, mode, &camera->quadlets_per_frame)!= DC1394_SUCCESS) {
      printf("(%s) Unable to get format 7 total bytes per frame %d \n", __FILE__, mode);
      return DC1394_FAILURE;
      }
      camera->quadlets_per_frame/=4;
    */
    //fprintf(stderr,"quadlets per frame: %d\n",camera->quadlets_per_frame);
  }

  if (capture->quadlets_per_frame<=0) {
    return DC1394_FAILURE;
  }
  capture->frame_width = width; /* irrespective of pixel depth */
  capture->frame_height= height;
  
  if (is_iso_on){
    if (dc1394_start_iso_transmission(camera) != DC1394_SUCCESS) {
      printf("(%s) Unable to start iso transmission!\n", __FILE__);
      return DC1394_FAILURE;
    }
  }

  return DC1394_SUCCESS;
}


/**********************/
/* External functions */
/**********************/


/*=========================================================================
 *  DESCRIPTION OF FUNCTION:  dc1394_setup_format7_capture
 *  ==> see headerfile
 *=======================================================================*/
int
dc1394_setup_format7_capture(dc1394camera_t *camera,
                             int channel, int mode, int speed,
                             int bytes_per_packet,
                             unsigned int left, unsigned int top,
                             unsigned int width, unsigned int height, 
                             dc1394capture_t *capture)

{
  /* printf( "trying to setup format7 with \n"
          "bpp    = %d\n"
          "pos_x  = %d\n"
          "pos_y  = %d\n"
          "size_x = %d\n"
          "size_y = %d\n",
          bytes_per_packet, left, top, width, height);*/
  
  if (_dc1394_basic_format7_setup(camera, channel, mode, speed, bytes_per_packet,
                                  left, top, width, height, capture) ==
      DC1394_FAILURE) {
    return DC1394_FAILURE;
  }
  
  capture->capture_buffer= (int*)malloc(capture->quadlets_per_frame*4);
  
  if (capture->capture_buffer == NULL) {
    printf("(%s) unable to allocate memory for capture buffer\n", __FILE__);
    return DC1394_FAILURE;
  }
  
  return DC1394_SUCCESS;
}



/*=========================================================================
 *  DESCRIPTION OF FUNCTION:  dc1394_dma_setup_format7_capture
 *  ==> see headerfile
 *=======================================================================*/
int
dc1394_dma_setup_format7_capture(dc1394camera_t *camera,
                                 int channel, int mode, int speed,
                                 int bytes_per_packet,
                                 unsigned int left, unsigned int top,
                                 unsigned int width, unsigned int height,
                                 int num_dma_buffers,
				 int drop_frames,
				 const char *dma_device_file,
                                 dc1394capture_t *capture)
{
  if (_dc1394_basic_format7_setup(camera, channel, mode, speed, bytes_per_packet,
				  left, top, width, height, capture) != DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }
  
  capture->port = camera->port;
  capture->dma_device_file = dma_device_file;
  capture->drop_frames = drop_frames;
  
  return _dc1394_dma_basic_setup(channel,num_dma_buffers, capture);
  
}



int
dc1394_query_format7_max_image_size(dc1394camera_t *camera,
 				    unsigned int mode,
                                    unsigned int *horizontal_size,
 				    unsigned int *vertical_size)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ, &value);
  *horizontal_size  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_size= (unsigned int) ( value & 0x0000FFFFUL );
  
  return retval;
}

int
dc1394_query_format7_unit_size(dc1394camera_t *camera,
 			       unsigned int mode,
                               unsigned int *horizontal_unit,
 			       unsigned int *vertical_unit)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
  *horizontal_unit  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_unit= (unsigned int) ( value & 0x0000FFFFUL );
  
  return retval;
}

int
dc1394_query_format7_image_position(dc1394camera_t *camera,
 				    unsigned int mode,
                                    unsigned int *left_position,
 				    unsigned int *top_position)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  retval=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_POSITION, &value);
  *left_position = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  *top_position= (unsigned int) ( value & 0x0000FFFFUL );       

  return retval;
}


int
dc1394_query_format7_image_size(dc1394camera_t *camera,
 				unsigned int mode, unsigned int *width,
 				unsigned int *height)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_SIZE, &value);
  *width= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  *height = (unsigned int) ( value & 0x0000FFFFUL );       
  
  return retval;
}

int
dc1394_query_format7_color_coding_id(dc1394camera_t *camera,
 				     unsigned int mode, unsigned int *color_id)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_CODING_ID, &value);
  value=value>>24;
  if (retval==DC1394_SUCCESS)
    *color_id= (unsigned int)value+COLOR_FORMAT7_MIN;
  
  return retval;
}

int
dc1394_query_format7_color_coding(dc1394camera_t *camera,
 				  unsigned int mode, quadlet_t *value)
{
  int retval;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_CODING_INQ, value);

  return retval;
}
 
int
dc1394_query_format7_pixel_number(dc1394camera_t *camera,
 				  unsigned int mode, unsigned int *pixnum)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ, &value);
  *pixnum= (unsigned int) value;
  
  return retval;
}

int
dc1394_query_format7_total_bytes(dc1394camera_t *camera,
 				 unsigned int mode, unsigned long long int *total_bytes)
{
  int retval;
  unsigned long long int value_hi, value_lo;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ, &value);
  value_hi=value;
  if (retval != DC1394_SUCCESS)
    return DC1394_FAILURE;
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ, &value);
  value_lo=value;
  
  *total_bytes= (value_lo | ( value_hi << 32) ); 
  
  return retval;
}

int
dc1394_query_format7_packet_para(dc1394camera_t *camera,
 				 unsigned int mode, unsigned int *min_bytes,
				 unsigned int *max_bytes)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PACKET_PARA_INQ, &value);
  *min_bytes= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  *max_bytes= (unsigned int) ( value & 0x0000FFFFUL );       
  
  return retval;
}

int
dc1394_query_format7_byte_per_packet(dc1394camera_t *camera,
 				     unsigned int mode,
                                     unsigned int *packet_bytes)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  *packet_bytes= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
  
  if (packet_bytes==0) {
    printf("(%s): BYTES_PER_PACKET is zero. This should not happen.\n", __FILE__);
    return DC1394_FAILURE;
  }
  return retval;
}

int
dc1394_set_format7_image_position(dc1394camera_t *camera,
 				  unsigned int mode, unsigned int left,
 				  unsigned int top)
{
  if (SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_POSITION,
			       (quadlet_t)((left << 16) | top)) != DC1394_SUCCESS) {
    printf("(%s) Format7 image position setting failure \n", __FILE__);
    return DC1394_FAILURE;
  }
  else
    // IIDC v1.30 handshaking:
    return _dc1394_v130_handshake(camera, mode);
}

int
dc1394_set_format7_image_size(dc1394camera_t *camera,
                              unsigned int mode, unsigned int width,
                              unsigned int height)
{
  if (SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_SIZE,
			       (quadlet_t)((width << 16) | height)) != DC1394_SUCCESS) {
    printf("(%s) Format7 image size setting failure \n", __FILE__);
    return DC1394_FAILURE;
  }
  else
    // IIDC v1.30 handshaking:
    return _dc1394_v130_handshake(camera, mode);
}

int
dc1394_set_format7_color_coding_id(dc1394camera_t *camera,
 				   unsigned int mode, unsigned int color_id)
{
  if ( (color_id < COLOR_FORMAT7_MIN) || (color_id > COLOR_FORMAT7_MAX) ) {
    return DC1394_FAILURE;
  }
  
  color_id-= COLOR_FORMAT7_MIN;
  color_id=color_id<<24;
  if (SetCameraFormat7Register(camera, mode,REG_CAMERA_FORMAT7_COLOR_CODING_ID,
			       (quadlet_t)color_id) != DC1394_SUCCESS) {
    printf("(%s) Format7 color coding ID setting failure \n", __FILE__);
    return DC1394_FAILURE;
  }
  else
    // IIDC v1.30 handshaking:
    return _dc1394_v130_handshake(camera, mode);
}

int
dc1394_set_format7_byte_per_packet(dc1394camera_t *camera,
 				   unsigned int mode,
                                   unsigned int packet_bytes)
{
  if (SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET,
			       (quadlet_t)(packet_bytes) << 16 ) != DC1394_SUCCESS) {
    printf("(%s) Format7 bytes-per-packet setting failure \n", __FILE__);
    return DC1394_FAILURE;
  }
  else
    // IIDC v1.30 error checking:
    return _dc1394_v130_errflag2(camera, mode);
}

int
dc1394_query_format7_value_setting(dc1394camera_t *camera,
				   unsigned int mode,
				   unsigned int *present,
				   unsigned int *setting1,
				   unsigned int *err_flag1,
				   unsigned int *err_flag2)
{
  int retval;
  quadlet_t value;
  
  if (camera->sw_version>=IIDC_VERSION_1_30) {
    
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
      return DC1394_FAILURE;
    }
    
    retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_VALUE_SETTING, &value);
    *present= (unsigned int) ( value & 0x80000000UL ) >> 31;
    *setting1= (unsigned int) ( value & 0x40000000UL ) >> 30;
    *err_flag1= (unsigned int) ( value & 0x00800000UL ) >> 23;
    *err_flag2= (unsigned int) ( value & 0x00400000UL ) >> 22;
  }
  else {
    *present=0;
    return DC1394_SUCCESS;
  }
  
  return retval;
}

int
dc1394_set_format7_value_setting(dc1394camera_t *camera,
				 unsigned int mode)
{
  return SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_VALUE_SETTING, 
				  (quadlet_t)0x40000000UL);
}

int
dc1394_query_format7_recommended_byte_per_packet(dc1394camera_t *camera,
						  unsigned int mode, unsigned int *bpp)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  *bpp= (unsigned int) ( value & 0x0000FFFFUL );
  
  return retval;
}

int
dc1394_query_format7_packet_per_frame(dc1394camera_t *camera,
				      unsigned int mode, unsigned int *ppf)
{
  int retval;
  quadlet_t value;
  unsigned int packet_bytes;
  unsigned long long int total_bytes;

  if (camera->sw_version>=IIDC_VERSION_1_30) {

    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
      return DC1394_FAILURE;
    }

    retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PACKET_PER_FRAME_INQ, &value);
    *ppf= (unsigned int) (value);
    
    return retval;
  }
  else {
    // return an estimate, NOT TAKING ANY PADDING INTO ACCOUNT
    if (dc1394_query_format7_byte_per_packet(camera, mode, &packet_bytes)!=DC1394_SUCCESS) {
      return DC1394_FAILURE;
    }
    if (packet_bytes==0) {
      return DC1394_FAILURE;
    }
    if (dc1394_query_format7_total_bytes(camera, mode, &total_bytes)!=DC1394_SUCCESS) {
      return DC1394_FAILURE;
    }
    if (total_bytes%packet_bytes!=0)
      *ppf=total_bytes/packet_bytes+1;
    else
      *ppf=total_bytes/packet_bytes;
    
    return DC1394_SUCCESS;
  }
  
}

int
dc1394_query_format7_unit_position(dc1394camera_t *camera,
				   unsigned int mode,
				   unsigned int *horizontal_pos,
				   unsigned int *vertical_pos)
{
  int retval;
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  if (camera->sw_version>=IIDC_VERSION_1_30) {
    retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_POSITION_INQ, &value);
  }
  else {
    // if version is not 1.30, use the UNIT_SIZE_INQ register
    retval= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
  }
  
  *horizontal_pos = (unsigned int) (( value & 0xFFFF0000UL )>>16);
  *vertical_pos   = (unsigned int) ( value & 0x0000FFFFUL );
  
  return retval;
}

int
dc1394_query_format7_frame_interval(dc1394camera_t *camera,
				    unsigned int mode, float *interval)
{   
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  if (GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_FRAME_INTERVAL_INQ, &value)
      !=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }

  *interval=value;
  
  return DC1394_SUCCESS;
}   
    
int
dc1394_query_format7_data_depth(dc1394camera_t *camera,
				unsigned int mode, unsigned int *data_depth)
{   
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  if (GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_DATA_DEPTH_INQ, &value)
      !=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }

  *data_depth=value >> 24;
  return DC1394_SUCCESS;
}   
    
int
dc1394_query_format7_color_filter_id(dc1394camera_t *camera,
				     unsigned int mode, unsigned int *color_id)
{   
  quadlet_t value;
  
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  if (GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID, &value)
      !=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }

  *color_id= (value >> 24)+COLOR_FORMAT7_MIN;
  return DC1394_SUCCESS;
}   
     
int
dc1394_set_format7_color_filter_id(dc1394camera_t *camera,
				   unsigned int mode, unsigned int color_id)
{   
  if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  if (SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID,
			       color_id - COLOR_FORMAT7_MIN)!=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }
  
  return DC1394_SUCCESS;
}   
