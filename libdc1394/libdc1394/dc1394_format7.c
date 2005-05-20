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
 
#include "dc1394_control.h"
#include "dc1394_internal.h"
#include "dc1394_register.h"
#include "dc1394_offsets.h"
#include "dc1394_utils.h"
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
_dc1394_v130_handshake(dc1394camera_t *camera, uint_t mode)
{
  uint_t setting_1, err_flag1, err_flag2, v130handshake;
  uint_t exit_loop;
  int err;

  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_30) {
    // We don't use > because 114 is for ptgrey cameras which are not 1.30 but 1.20
    err=dc1394_query_format7_value_setting(camera, mode, &v130handshake, &setting_1, &err_flag1, &err_flag2);
    DC1394_ERR_CHK(err, "Unable to read value setting register");
  }
  else {
    v130handshake=0;
    return DC1394_SUCCESS;
  }   
  
  if (v130handshake==1) {
    // we should use advanced IIDC v1.30 handshaking.
    //fprintf(stderr,"using handshaking\n");
    // set value setting to 1
    err=dc1394_set_format7_value_setting(camera, mode);
    DC1394_ERR_CHK(err, "Unable to set value setting register");

    // wait for value setting to clear:
    exit_loop=0;
    while (!exit_loop) { // WARNING: there is no timeout in this loop yet.
      err=dc1394_query_format7_value_setting(camera, mode, &v130handshake, &setting_1, &err_flag1, &err_flag2);
      DC1394_ERR_CHK(err, "Unable to read value setting register");

      exit_loop=(setting_1==0);
      usleep(0); 
    }
    if (err_flag1>0) {
      err=DC1394_FORMAT7_ERROR_FLAG_1;
      DC1394_ERR_CHK(err, "invalid image position, size, color coding, ISO speed or bpp");
    }
    
    // bytes per packet... registers are ready for reading.
  }

  return err;
}

int
_dc1394_v130_errflag2(dc1394camera_t *camera, uint_t mode)
{
  uint_t setting_1, err_flag1, err_flag2, v130handshake;
  int err;

  //fprintf(stderr,"Checking error flags\n");

  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_30) { // if version is 1.30.
    // We don't use > because 0x114 is for ptgrey cameras which are not 1.30 but 1.20
    err=dc1394_query_format7_value_setting(camera, mode, &v130handshake,&setting_1, &err_flag1, &err_flag2);
    DC1394_ERR_CHK(err, "Unable to read value setting register");
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
      DC1394_ERR_CHK(err, "proposed bytes per packet is not a valid value");
    }
  }

  return DC1394_SUCCESS;
}
 
/*======================================================================
 *
 *   see documentation of dc1394_setup_format7_capture() in
 *   dc1394_control.h
 *
 *======================================================================*/
int
_dc1394_basic_format7_setup(dc1394camera_t *camera,
                            uint_t channel, uint_t mode, uint_t speed,
                            uint_t bytes_per_packet,
                            uint_t left, uint_t top,
                            uint_t width, uint_t height, 
                            dc1394capture_t *capture)
{
  dc1394bool_t is_iso_on= DC1394_FALSE;
  uint_t unit_bytes, max_bytes;
  uint_t packet_bytes=0;
  uint_t recom_bpp;
  uint_t packets_per_frame;
  uint_t color_coding;
  uint_t camera_left = 0;
  uint_t camera_top = 0;
  uint_t camera_width = 0;
  uint_t camera_height = 0;
  uint_t max_width = 0;
  uint_t max_height = 0;
  float bpp;
  int err;

  err=dc1394_get_iso_status(camera, &is_iso_on);
  DC1394_ERR_CHK(err, "unable to get ISO status");

  if (is_iso_on) {
    err=dc1394_stop_iso_transmission(camera);
    DC1394_ERR_CHK(err, "Unable to stop iso transmission");
  }

  err=dc1394_set_iso_channel_and_speed(camera,channel,speed);
  DC1394_ERR_CHK(err, "Unable to set channel %d and speed %d",channel,speed);

  err=dc1394_set_video_mode(camera,mode);
  DC1394_ERR_CHK(err, "Unable to set video mode %d", mode);

  // get BPP before setting sizes,...
  if (bytes_per_packet==DC1394_QUERY_FROM_CAMERA) {
    err=dc1394_query_format7_byte_per_packet(camera, mode, &bytes_per_packet);
    DC1394_ERR_CHK(err, "Unable to get F7 bpp for mode %d", mode);
  }

  /*
   * TODO: frame_rate not used for format 7, may be calculated
   */
  capture->frame_rate= 0;
  capture->node= camera->node;
  capture->port=camera->port;
  capture->channel= channel;
  capture->handle=raw1394_new_handle();
  //capture->handle=camera->handle;
  raw1394_set_port(capture->handle,capture->port);

  /*-----------------------------------------------------------------------
   *  set image position. If QUERY_FROM_CAMERA was given instead of a
   *  position, use the actual value from camera
   *-----------------------------------------------------------------------*/

  /* The image position should be checked regardless of the left and top values
     as we also use it for the size setting */

  err=dc1394_query_format7_image_position(camera, mode, &camera_left, &camera_top);
  DC1394_ERR_CHK(err, "Unable to query image position");
    
  if( left == DC1394_QUERY_FROM_CAMERA) left = camera_left;
  if( top == DC1394_QUERY_FROM_CAMERA)  top = camera_top;
  
  err=dc1394_set_format7_image_position(camera, mode, left, top);
  DC1394_ERR_CHK(err, "Unable to set format 7 image position to [%d %d]",left, top);

  /*-----------------------------------------------------------------------
   *  If QUERY_FROM_CAMERA was given instead of an image size
   *  use the actual value from camera.
   *-----------------------------------------------------------------------*/
  if ( width == DC1394_QUERY_FROM_CAMERA || height == DC1394_QUERY_FROM_CAMERA) {
    err=dc1394_query_format7_image_size(camera, mode, &camera_width, &camera_height);
    DC1394_ERR_CHK(err, "Unable to query image size");
    
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
    err=dc1394_query_format7_max_image_size(camera, mode, &max_width, &max_height);
    DC1394_ERR_CHK(err, "Unable to query max image size");
    if( width == DC1394_USE_MAX_AVAIL)  width  = max_width - left;
    if( height == DC1394_USE_MAX_AVAIL) height = max_height - top;
  }

  err=dc1394_set_format7_image_size(camera, mode, width, height);
  DC1394_ERR_CHK(err, "Unable to set format 7 image size to [%d %d]",width, height);

  /*-----------------------------------------------------------------------
   *  Bytes-per-packet definition
   *-----------------------------------------------------------------------*/
  err=dc1394_query_format7_recommended_byte_per_packet(camera, mode, &recom_bpp);
  DC1394_ERR_CHK(err, "Recommended byte-per-packet inq error");
  
  err=dc1394_query_format7_packet_para(camera, mode, &unit_bytes, &max_bytes); /* PACKET_PARA_INQ */
  DC1394_ERR_CHK(err, "Packet para inq error");

  //fprintf(stderr,"recommended bpp: %d\n",recom_bpp);
  
  switch (bytes_per_packet) {
  case DC1394_USE_RECOMMENDED:
    if (recom_bpp>0) {
      bytes_per_packet=recom_bpp;
    }
    else { // recom. bpp asked, but register is 0. IGNORED
      printf("(%s) Recommended bytes per packet asked, but register is zero. Falling back to MAX BPP for mode %d \n", __FILE__, mode);
      bytes_per_packet=max_bytes;
    }
    break;
  case DC1394_USE_MAX_AVAIL:
    bytes_per_packet = max_bytes;
    break;
  //this case was handled by a previous call. Show error if we get in there. 
  case DC1394_QUERY_FROM_CAMERA:
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
      
  err=dc1394_set_format7_byte_per_packet(camera, mode, bytes_per_packet);
  DC1394_ERR_CHK(err, "Unable to set format 7 bytes per packet for mode %d", mode);
  
  err=dc1394_query_format7_byte_per_packet(camera, mode, &packet_bytes);
  DC1394_ERR_CHK(err, "Unable to get format 7 bytes per packet for mode %d", mode);
  capture->quadlets_per_packet= packet_bytes /4;

  if (capture->quadlets_per_packet<=0) {
    printf("(%s) No format 7 bytes per packet %d \n", __FILE__, mode);
    return DC1394_FAILURE;
  }
  //fprintf(stderr,"Camera has now %d bytes per packet\n", packet_bytes);
  //usleep(2000000);
  /*-----------------------------------------------------------------------
   *  ensure that quadlet aligned buffers are big enough, still expect
   *  problems when width*height  != quadlets_per_frame*4
   *-----------------------------------------------------------------------*/
  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_30) { // if version is 1.30
    err=dc1394_query_format7_packet_per_frame(camera, mode, &packets_per_frame);
    DC1394_ERR_CHK(err, "Unable to get format 7 packets per frame %d", mode);
    capture->quadlets_per_frame=(packets_per_frame*packet_bytes)/4;
    //fprintf(stderr,"packet per frame: %d\n",packets_per_frame);
  }
  else {
    // For other specs revisions, we use a trick to determine the total bytes.
    // We don't use the total_bytes register in 1.20 as it has been interpreted in
    // different ways by manufacturers. Thanks to Martin Gramatke for pointing this trick out.
    err=dc1394_query_format7_color_coding_id(camera, mode, &color_coding);
    DC1394_ERR_CHK(err, "Unable to get format 7 color coding for mode %d \n", mode);

    //fprintf(stderr,"color coding: %d\n",color_coding);
    err=dc1394_get_bytes_per_pixel(color_coding, &bpp);
    DC1394_ERR_CHK(err, "Unable to infer bpp from color coding");
    packets_per_frame = ((int)(width * height * bpp) +
			 bytes_per_packet -1) / bytes_per_packet;
    capture->quadlets_per_frame=(packets_per_frame*bytes_per_packet)/4;

    /*
      if (dc1394_query_format7_total_bytes(camera, mode, &camera->quadlets_per_frame)!= DC1394_SUCCESS) {
      printf("(%s) Unable to get format 7 total bytes per frame %d \n", __FILE__, mode);
      return DC1394_FAILURE;
      }
      camera->quadlets_per_frame/=4;
    */
  }

  if (capture->quadlets_per_frame<=0) {
    //fprintf(stderr,"quadlets per frame: %d\n",capture->quadlets_per_frame);
    return DC1394_FAILURE;
  }
  capture->frame_width = width; /* irrespective of pixel depth */
  capture->frame_height= height;
  
  if (is_iso_on){
    err=dc1394_start_iso_transmission(camera);
    DC1394_ERR_CHK(err,"Unable to restart iso transmission");
  }

  return err;
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
                             uint_t channel, uint_t mode, uint_t speed,
                             uint_t bytes_per_packet,
                             uint_t left, uint_t top,
                             uint_t width, uint_t height, 
                             dc1394capture_t *capture)

{
  int err;
  /* printf( "trying to setup format7 with \n"
          "bpp    = %d\n"
          "pos_x  = %d\n"
          "pos_y  = %d\n"
          "size_x = %d\n"
          "size_y = %d\n",
          bytes_per_packet, left, top, width, height);*/
  
  err=_dc1394_basic_format7_setup(camera, channel, mode, speed, bytes_per_packet,
                                  left, top, width, height, capture);
  DC1394_ERR_CHK(err, "Could not perform basic F7 capture setup");
  
  capture->capture_buffer= (uint_t*)malloc(capture->quadlets_per_frame*4);
  
  if (capture->capture_buffer == NULL) {
    err=DC1394_MEMORY_ALLOCATION_FAILURE;
    DC1394_ERR_CHK(err,"failed to allocate capture buffer");
  }
  
  return err;
}



/*=========================================================================
 *  DESCRIPTION OF FUNCTION:  dc1394_dma_setup_format7_capture
 *  ==> see headerfile
 *=======================================================================*/
int
dc1394_dma_setup_format7_capture(dc1394camera_t *camera,
                                 uint_t channel, uint_t mode, uint_t speed,
                                 uint_t bytes_per_packet,
                                 uint_t left, uint_t top,
                                 uint_t width, uint_t height,
                                 uint_t num_dma_buffers,
				 uint_t drop_frames,
				 const char *dma_device_file,
                                 dc1394capture_t *capture)
{
  int err;
  
  err=_dc1394_basic_format7_setup(camera, channel, mode, speed, bytes_per_packet,
				  left, top, width, height, capture);
  DC1394_ERR_CHK(err, "Could not perform basic F7 capture setup");
  
  capture->port = camera->port;
  capture->dma_device_file = dma_device_file;
  capture->drop_frames = drop_frames;
  
  err=_dc1394_dma_basic_setup(channel,num_dma_buffers, capture);
  DC1394_ERR_CHK(err, "Could not perform DMA capture setup or format7");
  
  return err;
}



int
dc1394_query_format7_max_image_size(dc1394camera_t *camera,
 				    uint_t mode,
                                    uint_t *horizontal_size,
 				    uint_t *vertical_size)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get max image sizes");

  *horizontal_size  = (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_size= (uint_t) ( value & 0x0000FFFFUL );
  
  //fprintf(stderr,"got size: %d %d\n", *horizontal_size, *vertical_size);

  return err;
}

int
dc1394_query_format7_unit_size(dc1394camera_t *camera,
 			       uint_t mode,
                               uint_t *horizontal_unit,
 			       uint_t *vertical_unit)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get unit sizes");

  *horizontal_unit  = (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  *vertical_unit= (uint_t) ( value & 0x0000FFFFUL );
  
  return err;
}

int
dc1394_query_format7_image_position(dc1394camera_t *camera,
 				    uint_t mode,
                                    uint_t *left_position,
 				    uint_t *top_position)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_POSITION, &value);
  DC1394_ERR_CHK(err, "Could not get image position");

  *left_position = (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  *top_position= (uint_t) ( value & 0x0000FFFFUL );       

  return err;
}


int
dc1394_query_format7_image_size(dc1394camera_t *camera,
 				uint_t mode, uint_t *width,
 				uint_t *height)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_SIZE, &value);
  DC1394_ERR_CHK(err, "could not get current image size");

  *width= (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  *height = (uint_t) ( value & 0x0000FFFFUL );       
  
  return err;
}

int
dc1394_query_format7_color_coding_id(dc1394camera_t *camera,
 				     uint_t mode, uint_t *color_id)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_CODING_ID, &value);
  DC1394_ERR_CHK(err, "Could not get current color_id");

  value=value>>24;
  *color_id= (uint_t)value+DC1394_COLOR_CODING_MIN;
  
  return err;
}

int
dc1394_query_format7_color_coding(dc1394camera_t *camera, uint_t mode, dc1394colormodes_t *modes)
{
  int err, i;
  quadlet_t value;

  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
 
  //fprintf(stderr,"malloc OK\n");

  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_CODING_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get available color codings");

  //fprintf(stderr,"codings reg: 0x%x\n",value);

  modes->num=0;
  for (i=0;i<DC1394_COLOR_CODING_NUM;i++) {
    //fprintf(stderr,"test: 0x%x\n",(value & (0x1 << (31-i))));
    if ((value & (0x1 << (31-i))) > 0) {
      //fprintf(stderr,"got one coding\n");
      modes->modes[modes->num]=i+DC1394_COLOR_CODING_MIN;
      modes->num++;
    }
  }

  return err;
}
 
int
dc1394_query_format7_pixel_number(dc1394camera_t *camera,
 				  uint_t mode, uint_t *pixnum)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get pixel number");

  *pixnum= (uint_t) value;
  
  return err;
}

int
dc1394_query_format7_total_bytes(dc1394camera_t *camera,
 				 uint_t mode, uint64_t *total_bytes)
{
  int err;
  uint64_t value_hi, value_lo;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get total bytes - LSB");

  value_hi=value;
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get total bytes - MSB");

  value_lo=value;
  
  *total_bytes= (value_lo | ( value_hi << 32) ); 
  
  return err;
}

int
dc1394_query_format7_packet_para(dc1394camera_t *camera,
 				 uint_t mode, uint_t *min_bytes,
				 uint_t *max_bytes)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PACKET_PARA_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get F7 packet parameters");

  *min_bytes= (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  *max_bytes= (uint_t) ( value & 0x0000FFFFUL );       
  
  return err;
}

int
dc1394_query_format7_byte_per_packet(dc1394camera_t *camera,
 				     uint_t mode,
                                     uint_t *packet_bytes)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  DC1394_ERR_CHK(err, "Could not get bytes per packet");

  *packet_bytes= (uint_t) ( value & 0xFFFF0000UL ) >> 16;
  
  if (packet_bytes==0) {
    printf("(%s): BYTES_PER_PACKET is zero. This should not happen.\n", __FILE__);
    return DC1394_FAILURE;
  }
  return err;
}

int
dc1394_set_format7_image_position(dc1394camera_t *camera,
 				  uint_t mode, uint_t left,
 				  uint_t top)
{
  int err;

  err=SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_POSITION, (quadlet_t)((left << 16) | top));
  DC1394_ERR_CHK(err, "Format7 image position setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, mode);
  DC1394_ERR_CHK(err, "F7 handshake failure");

  return err;
}

int
dc1394_set_format7_image_size(dc1394camera_t *camera,
                              uint_t mode, uint_t width,
                              uint_t height)
{
  int err;
  err=SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_IMAGE_SIZE, (quadlet_t)((width << 16) | height));
  DC1394_ERR_CHK(err, "Format7 image size setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, mode);
  DC1394_ERR_CHK(err, "F7 handshake failure");

  return err;
}

int
dc1394_set_format7_color_coding_id(dc1394camera_t *camera,
 				   uint_t mode, uint_t color_id)
{
  int err;

  if ( (color_id < DC1394_COLOR_CODING_MIN) || (color_id > DC1394_COLOR_CODING_MAX) ) {
    return DC1394_FAILURE;
  }
  
  color_id-= DC1394_COLOR_CODING_MIN;
  color_id=color_id<<24;
  err=SetCameraFormat7Register(camera, mode,REG_CAMERA_FORMAT7_COLOR_CODING_ID, (quadlet_t)color_id);
  DC1394_ERR_CHK(err, "Format7 color coding ID setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, mode);
  DC1394_ERR_CHK(err, "F7 handshake failure");

  return err;
}

int
dc1394_set_format7_byte_per_packet(dc1394camera_t *camera,
 				   uint_t mode,
                                   uint_t packet_bytes)
{
  int err;
  err=SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, (quadlet_t)(packet_bytes) << 16 );
  DC1394_ERR_CHK(err, "Format7 bytes-per-packet setting failure");

  // IIDC v1.30 handshaking:
  err=_dc1394_v130_handshake(camera, mode);
  DC1394_ERR_CHK(err, "F7 handshake failure");

  return err;
}

int
dc1394_query_format7_value_setting(dc1394camera_t *camera,
				   uint_t mode,
				   uint_t *present,
				   uint_t *setting1,
				   uint_t *err_flag1,
				   uint_t *err_flag2)
{
  int err;
  quadlet_t value;
  
  if (camera->iidc_version<DC1394_IIDC_VERSION_1_30) {
    *present=0;
    return DC1394_SUCCESS;
  }

  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_VALUE_SETTING, &value);
  DC1394_ERR_CHK(err, "could note get value setting");
  
  *present= (uint_t) ( value & 0x80000000UL ) >> 31;
  *setting1= (uint_t) ( value & 0x40000000UL ) >> 30;
  *err_flag1= (uint_t) ( value & 0x00800000UL ) >> 23;
  *err_flag2= (uint_t) ( value & 0x00400000UL ) >> 22;

  return err;
}

int
dc1394_set_format7_value_setting(dc1394camera_t *camera,
				 uint_t mode)
{
  int err;
  err=SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_VALUE_SETTING, (quadlet_t)0x40000000UL);
  DC1394_ERR_CHK(err, "Could not set value setting");

  return err;
}

int
dc1394_query_format7_recommended_byte_per_packet(dc1394camera_t *camera,
						  uint_t mode, uint_t *bpp)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  err= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_BYTE_PER_PACKET, &value);
  DC1394_ERR_CHK(err, "Could not get recommended BPP");

  *bpp= (uint_t) ( value & 0x0000FFFFUL );
  
  return err;
}

int
dc1394_query_format7_packet_per_frame(dc1394camera_t *camera,
				      uint_t mode, uint_t *ppf)
{
  int err;
  quadlet_t value;
  uint_t packet_bytes;
  uint64_t total_bytes;

  if (camera->iidc_version>=DC1394_IIDC_VERSION_1_30) {

    if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
      return DC1394_FAILURE;
    }

    err= GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_PACKET_PER_FRAME_INQ, &value);
    DC1394_ERR_CHK(err, "Could not get the number of packets per frame");

    *ppf= (uint_t) (value);
    
    return err;
  }
  else {
    // return an estimate, NOT TAKING ANY PADDING INTO ACCOUNT
    err=dc1394_query_format7_byte_per_packet(camera, mode, &packet_bytes);
    DC1394_ERR_CHK(err, "Could not get BPP");

    if (packet_bytes==0) {
      return DC1394_FAILURE;
    }

    err=dc1394_query_format7_total_bytes(camera, mode, &total_bytes);
    DC1394_ERR_CHK(err, "Could not get total number of bytes");

    if (total_bytes%packet_bytes!=0)
      *ppf=total_bytes/packet_bytes+1;
    else
      *ppf=total_bytes/packet_bytes;
    
    return err;
  }
  
}

int
dc1394_query_format7_unit_position(dc1394camera_t *camera,
				   uint_t mode,
				   uint_t *horizontal_pos,
				   uint_t *vertical_pos)
{
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  if (camera->iidc_version>=DC1394_IIDC_VERSION_1_30) {
    err = GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_POSITION_INQ, &value);
    DC1394_ERR_CHK(err, "Could not get unit position");
  }
  else {
    // if version is not 1.30, use the UNIT_SIZE_INQ register
    err = GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_UNIT_SIZE_INQ, &value);
    DC1394_ERR_CHK(err, "Could not get unit size");
  }
  
  *horizontal_pos = (uint_t) (( value & 0xFFFF0000UL )>>16);
  *vertical_pos   = (uint_t) ( value & 0x0000FFFFUL );
  
  return err;
}

int
dc1394_query_format7_frame_interval(dc1394camera_t *camera,
				    uint_t mode, float *interval)
{   
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_FRAME_INTERVAL_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get frame interval");

  *interval=value;
  
  return err;
}   
    
int
dc1394_query_format7_data_depth(dc1394camera_t *camera,
				uint_t mode, uint_t *data_depth)
{   
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_DATA_DEPTH_INQ, &value);
  DC1394_ERR_CHK(err, "Could not get data depth");

  *data_depth=value >> 24;
  return err;
}   
    
int
dc1394_query_format7_color_filter_id(dc1394camera_t *camera,
				     uint_t mode, uint_t *color_id)
{   
  int err;
  quadlet_t value;
  
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  err=GetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID, &value);
  DC1394_ERR_CHK(err, "Could not get color filter ID");

  *color_id= (value >> 24)+DC1394_COLOR_CODING_MIN;
  return err;
}   
     
int
dc1394_set_format7_color_filter_id(dc1394camera_t *camera,
				   uint_t mode, uint_t color_id)
{   
  int err;
  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) )
    return DC1394_FAILURE;
  
  err=SetCameraFormat7Register(camera, mode, REG_CAMERA_FORMAT7_COLOR_FILTER_ID,
			       color_id - DC1394_COLOR_CODING_MIN);
  DC1394_ERR_CHK(err, "Could not set color filter ID");

  return err;
}   
