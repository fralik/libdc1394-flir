/*
  * 1394-Based Digital Camera Format_7 functions for the Control Library
  *
  * Written by Damien Douxchamps <douxchamps@ieee.org>
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
#include "config.h"

#define REG_CAMERA_V_CSR_INQ_BASE              0x2E0U

#define REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ  0x000U
#define REG_CAMERA_FORMAT7_UNIT_SIZE_INQ       0x004U
#define REG_CAMERA_FORMAT7_IMAGE_POSITION      0x008U
#define REG_CAMERA_FORMAT7_IMAGE_SIZE          0x00CU
#define REG_CAMERA_FORMAT7_COLOR_CODING_ID     0x010U
#define REG_CAMERA_FORMAT7_COLOR_CODING_INQ    0x014U
#define REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ    0x034U
#define REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ  0x038U
#define REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ  0x03CU
#define REG_CAMERA_FORMAT7_PACKET_PARA_INQ     0x040U
#define REG_CAMERA_FORMAT7_BYTE_PER_PACKET     0x044U

/**********************/ 
/* Internal functions */
/**********************/

static int
QueryFormat7CSROffset(raw1394handle_t handle, nodeid_t node, int mode,
		      quadlet_t *value)
{
    int retval;

    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }

    mode-= MODE_FORMAT7_MIN;
    retval= GetCameraControlRegister(handle, node,
                                     REG_CAMERA_V_CSR_INQ_BASE +
                                     (mode * 0x04U), value);
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}

static int
GetCameraFormat7Register(raw1394handle_t handle, nodeid_t node,
                         unsigned int mode, octlet_t offset, quadlet_t *value)
{
    int retval, retry= MAX_RETRIES;
    quadlet_t csr;
    
    if ((retval= QueryFormat7CSROffset(handle, node, mode, &csr))
	< 0)
    {
        return retval;
    }

    csr*= 0x04UL;

    /* retry a few times if necessary (addition by PDJ) */
    while(retry--)
    {
        retval= raw1394_read(handle, 0xffc0 | node,
                             CONFIG_ROM_BASE + csr + offset, 4, value);

#ifdef LIBRAW1394_OLD
        if (retval >= 0)
        {
            int ack= retval >> 16;
            int rcode= retval & 0xffff;

#ifdef SHOW_ERRORS
            printf("Format 7 reg read ack of %x rcode of %x\n", ack, rcode);
#endif

            if ( ((ack == ACK_PENDING) || (ack == ACK_LOCAL)) &&
                 (rcode == RESP_COMPLETE) )
            { 
                /* conditionally byte swap the value */
                *value= ntohl(*value); 
                return 0;
            }

        }
#else
        if (!retval)
        {
            /* conditionally byte swap the value */
            *value= ntohl(*value);
            return retval;
        }
        else if (errno != EAGAIN)
        {
            return retval;
        }
#endif /* LIBRAW1394_VERSION <= 0.8.2 */

        usleep(SLOW_DOWN);
    }
    
    *value = ntohl(*value);
    return(retval);
}

static int
SetCameraFormat7Register(raw1394handle_t handle, nodeid_t node,
                         unsigned int mode, octlet_t offset, quadlet_t value)
{
    int retval, retry= MAX_RETRIES;
    quadlet_t csr;
    
    if ((retval= QueryFormat7CSROffset(handle, node, mode, &csr))
	< 0)
    {
        return retval;
    }

    csr*= 0x04UL;
  
    /* conditionally byte swap the value (addition by PDJ) */
    value= htonl(value);
 
    /* retry a few times if necessary (addition by PDJ) */
    while(retry--)
    {
        retval= raw1394_write(handle, 0xffc0 | node,
                              CONFIG_ROM_BASE + offset + csr, 4, &value);

#ifdef LIBRAW1394_OLD
        if (retval >= 0)
        {
            int ack= retval >> 16;
            int rcode= retval & 0xffff;

#ifdef SHOW_ERRORS
            printf("Format 7 reg write ack of %x rcode of %x\n", ack, rcode);
#endif

            if ( ((ack == ACK_PENDING) || (ack == ACK_LOCAL) ||
                  (ack == ACK_COMPLETE)) &&
                 ((rcode == RESP_COMPLETE) || (rcode == RESP_SONY_HACK)) ) 
            {
                return 0;
            }
            
            
        }
#else
        if (!retval || (errno != EAGAIN))
        {
            return retval;
        }
#endif /* LIBRAW1394_VERSION <= 0.8.2 */

 	usleep(SLOW_DOWN);
    }
    
    return retval;
}


/*======================================================================*/
/*! 
 *   see documentation of dc1394_setup_format7_capture() in
 *   dc1394_control.h
 *
 *======================================================================*/
int
_dc1394_basic_format7_setup(raw1394handle_t handle, nodeid_t node,
                            int channel, int mode, int speed,
                            int bytes_per_packet,
                            int left, int top,
                            int width, int height, 
                            dc1394_cameracapture * camera)
{
  dc1394bool_t is_iso_on= DC1394_FALSE;
  unsigned int min_bytes, max_bytes;
  unsigned packet_bytes=0;

  if (dc1394_get_iso_status(handle, node, &is_iso_on) != DC1394_SUCCESS)
      return DC1394_FAILURE;

  if (is_iso_on)
  {

    if (dc1394_stop_iso_transmission(handle, node) != DC1394_SUCCESS)
    {
      printf("(%s) Unable to stop iso transmission!\n", __FILE__);
      return DC1394_FAILURE;
    }

  }
  if (dc1394_set_iso_channel_and_speed(handle,node,channel,speed)!=DC1394_SUCCESS)
  {
    printf("(%s) Unable to set channel %d and speed %d!\n",__FILE__,channel,speed);
    return DC1394_FAILURE;
  }

  if (dc1394_set_video_format(handle,node,FORMAT_SCALABLE_IMAGE_SIZE)!=DC1394_SUCCESS)
  {
    printf("(%s) Unable to set video format %d!\n",__FILE__, FORMAT_SCALABLE_IMAGE_SIZE);
    return DC1394_FAILURE;
  }

  if (dc1394_set_video_mode(handle, node,mode) != DC1394_SUCCESS)
  {
    printf("(%s) Unable to set video mode %d!\n", __FILE__, mode);
    return DC1394_FAILURE;
  }

  /*-----------------------------------------------------------------------
   *  set image position. If QUERY_FROM_CAMERA was given instead of a
   *  position, use the actual value from camera
   *-----------------------------------------------------------------------*/
  if( left == QUERY_FROM_CAMERA || top == QUERY_FROM_CAMERA)
  {
    unsigned int camera_left = 0;
    unsigned int camera_top = 0;
    if (dc1394_query_format7_image_position(handle, node, mode,
                                            &camera_left, &camera_top)
        != DC1394_SUCCESS)
    {
      printf("(%s) Unable to query image position\n", __FILE__);
      return DC1394_FAILURE;
    }
    
    if( left == QUERY_FROM_CAMERA) left = camera_left;
    if( top == QUERY_FROM_CAMERA)  top = camera_top;
  }

  if (dc1394_set_format7_image_position(handle, node, mode, left, top) != DC1394_SUCCESS)
  {
    printf("(%s) Unable to set format 7 image position to "
           "left=%d and top=%d!\n",  __FILE__, left, top);
    return DC1394_FAILURE;
  }

  /*-----------------------------------------------------------------------
   *  set image size. If QUERY_FROM_CAMERA was given instead of a
   *  position, use the actual value from camera, if USE_MAX_AVAIL was
   *  given, calculate max availible size
   *-----------------------------------------------------------------------*/
  if( width < 0 || height < 0)
  {
    unsigned int camera_width = 0;
    unsigned int camera_height = 0;
    if (dc1394_query_format7_image_size(handle, node, mode,
                                            &camera_width, &camera_height)
        != DC1394_SUCCESS)
    {
      printf("(%s) Unable to query image size\n", __FILE__);
      return DC1394_FAILURE;
    }
    
    if( width == QUERY_FROM_CAMERA)   width  = camera_width;
    else if( width == USE_MAX_AVAIL)  width  = camera_width - left;
    if( height == QUERY_FROM_CAMERA)  height = camera_height;
    else if( height == USE_MAX_AVAIL) height = camera_height - top;
    
  }
  if (dc1394_set_format7_image_size(handle, node, mode, width, height) != DC1394_SUCCESS)
  {
    printf("(%s) Unable to set format 7 image size to "
           "width=%d and height=%d!\n", __FILE__, width, height);
    return DC1394_FAILURE;
  }

  if (dc1394_query_format7_packet_para(handle, node, mode, &min_bytes, &max_bytes) != DC1394_SUCCESS) /* PACKET_PARA_INQ */
  {
    printf("Packet para inq error\n");
    return DC1394_FAILURE;
  }
    
  /*-----------------------------------------------------------------------
   *  if -1 is given for bytes_per_packet, use maximum allowed packet
   *  size. if given bytes_per_packet exceeds allowed range, correct it 
   *-----------------------------------------------------------------------*/
  if( bytes_per_packet == USE_MAX_AVAIL)
  {
    bytes_per_packet = max_bytes;
  }
  else if (bytes_per_packet > max_bytes)
  {
    bytes_per_packet = max_bytes;
  }
  else if (bytes_per_packet < min_bytes)
  {
    bytes_per_packet = min_bytes;
  }

  /*
   * TODO: bytes_per_packet must be full quadlet
   */
  //printf( "Trying to set bytes per packet to %d\n",  bytes_per_packet);
  
  if (dc1394_set_format7_byte_per_packet(handle, node, mode, bytes_per_packet) != DC1394_SUCCESS)
  {
    printf("(%s) Unable to set format 7 bytes per packet %d \n", __FILE__, mode);
    return DC1394_FAILURE;
  }
  
  if (dc1394_query_format7_byte_per_packet(handle, node, mode, &packet_bytes) == DC1394_SUCCESS)
  {
    camera->quadlets_per_packet= packet_bytes /4;
    if (camera->quadlets_per_packet<=0)
    {
      printf("(%s) No format 7 bytes per packet %d \n", __FILE__, mode);
      return DC1394_FAILURE;
    }
    //printf("Camera has now %d bytes per packet\n\n", packet_bytes);
    
  }
  else
  {
    printf("(%s) Unable to get format 7 bytes per packet %d \n", __FILE__, mode);
    return DC1394_FAILURE;
  }
  
  camera->node = node;
  /*
   * TODO: frame_rate not used for format 7, may be calculated
   */
  camera->frame_rate = 0; 
  camera->channel=channel;
  
  
  /*-----------------------------------------------------------------------
   *  ensure that quadlet aligned buffers are big enough, still expect
   *  problems when width*height  != quadlets_per_frame*4
   *-----------------------------------------------------------------------*/
  if (dc1394_query_format7_total_bytes( handle, node, mode, &camera->quadlets_per_frame)!= DC1394_SUCCESS)
    {
      printf("(%s) Unable to get format 7 total bytes per frame %d \n", __FILE__, mode);
      return DC1394_FAILURE;
    }
  camera->quadlets_per_frame=camera->quadlets_per_frame/4;
      
  if (camera->quadlets_per_frame<=0)
  {
    return DC1394_FAILURE;
  }
  camera->frame_width = width; /* irrespecable of pixel depth */
  camera->frame_height= height;

  if (is_iso_on)
  {
    
    if (dc1394_start_iso_transmission(handle,node) != DC1394_SUCCESS)
    {
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
dc1394_setup_format7_capture(raw1394handle_t handle, nodeid_t node,
                             int channel, int mode, int speed,
                             int bytes_per_packet,
                             unsigned int left, unsigned int top,
                             unsigned int width, unsigned int height, 
                             dc1394_cameracapture * camera)

{
  /* printf( "trying to setup format7 with \n"
          "bpp    = %d\n"
          "pos_x  = %d\n"
          "pos_y  = %d\n"
          "size_x = %d\n"
          "size_y = %d\n",
          bytes_per_packet, left, top, width, height);*/
  
  if (_dc1394_basic_format7_setup(handle,node, channel, mode,
                                  speed, bytes_per_packet,
                                  left, top, width, height, camera) ==
      DC1394_FAILURE)
  {
    return DC1394_FAILURE;
  }

  camera->capture_buffer= (int*)malloc(camera->quadlets_per_frame*4);
  
  if (camera->capture_buffer == NULL)
  {
    printf("(%s) unable to allocate memory for capture buffer\n",
           __FILE__);
    return DC1394_FAILURE;
  }
  
  return DC1394_SUCCESS;
}



/*=========================================================================
 *  DESCRIPTION OF FUNCTION:  dc1394_dma_setup_format7_capture
 *  ==> see headerfile
 *=======================================================================*/
int
dc1394_dma_setup_format7_capture(raw1394handle_t handle, nodeid_t node,
                                 int channel, int mode, int speed,
                                 int bytes_per_packet,
                                 unsigned int left, unsigned int top,
                                 unsigned int width, unsigned int height,
                                 int num_dma_buffers,
                                 dc1394_cameracapture *camera)
{
  int *myPort;

    if (_dc1394_basic_format7_setup(handle,node, channel, mode,
                            speed, bytes_per_packet,
                            left, top, width, height, camera) == DC1394_FAILURE)
    {
        return DC1394_FAILURE;
    }

    myPort = raw1394_get_userdata(handle);
    camera->port = *myPort;

    if (_dc1394_dma_basic_setup(channel,num_dma_buffers, camera) == DC1394_FAILURE)
    {
        return DC1394_FAILURE;
    }
    return DC1394_SUCCESS;
}



int
dc1394_query_format7_max_image_size(raw1394handle_t handle, nodeid_t node,
 				    unsigned int mode,
                                    unsigned int *horizontal_size,
 				    unsigned int *vertical_size)
{
    int retval;
    quadlet_t value;
    
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else     
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ,
                                         &value);
        *horizontal_size  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *vertical_size= (unsigned int) ( value & 0x0000FFFFUL );
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_unit_size(raw1394handle_t handle, nodeid_t node,
 			       unsigned int mode,
                               unsigned int *horizontal_unit,
 			       unsigned int *vertical_unit)
{
    int retval;
    quadlet_t value;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_UNIT_SIZE_INQ,
                                        &value);
        *horizontal_unit  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *vertical_unit= (unsigned int) ( value & 0x0000FFFFUL );
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_image_position(raw1394handle_t handle, nodeid_t node,
 				    unsigned int mode,
                                    unsigned int *left_position,
 				    unsigned int *top_position)
{
    int retval;
    quadlet_t value;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_IMAGE_POSITION,
                                        &value);
        *left_position = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *top_position= (unsigned int) ( value & 0x0000FFFFUL );       
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
 
int
dc1394_query_format7_image_size(raw1394handle_t handle, nodeid_t node,
 				unsigned int mode, unsigned int *width,
 				unsigned int *height)
{
    int retval;
    quadlet_t value;

    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_IMAGE_SIZE, &value);
        *width= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *height = (unsigned int) ( value & 0x0000FFFFUL );       
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_color_coding_id(raw1394handle_t handle, nodeid_t node,
 				     unsigned int mode, unsigned int *color_id)
{
    int retval;
    quadlet_t value;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_COLOR_CODING_ID,
                                         &value);
	value=value>>24;
        if (!retval) *color_id= (unsigned int)value+COLOR_FORMAT7_MIN;
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_color_coding(raw1394handle_t handle, nodeid_t node,
 				  unsigned int mode, quadlet_t *value)
{
    int retval;

    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_COLOR_CODING_INQ,
                                         value);
        if (!retval) value+= COLOR_FORMAT7_MIN;
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_pixel_number(raw1394handle_t handle, nodeid_t node,
 				  unsigned int mode, unsigned int *pixnum)
{
    int retval;
    quadlet_t value;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_PIXEL_NUMBER_INQ,
                                         &value);
        *pixnum= (unsigned int) value;
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_total_bytes(raw1394handle_t handle, nodeid_t node,
 				 unsigned int mode, unsigned int *total_bytes)
{
    int retval;
    quadlet_t value_hi, value_lo;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_TOTAL_BYTES_HI_INQ,
                                         &value_hi);
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_TOTAL_BYTES_LO_INQ,
                                         &value_lo);
 
        *total_bytes= (unsigned int) (value_lo | ( value_hi << 32) ); 
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_packet_para(raw1394handle_t handle, nodeid_t node,
 				 unsigned int mode, unsigned int *min_bytes,
				 unsigned int *max_bytes)
{
    int retval;
    quadlet_t value;

    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_PACKET_PARA_INQ,
                                         &value);
        *min_bytes= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *max_bytes= (unsigned int) ( value & 0x0000FFFFUL );       
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_byte_per_packet(raw1394handle_t handle, nodeid_t node,
 				     unsigned int mode,
                                     unsigned int *packet_bytes)
{
    int retval;
    quadlet_t value;
   
    if ( (mode > MODE_FORMAT7_MAX) || (mode < MODE_FORMAT7_MIN) )
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_BYTE_PER_PACKET,
                                         &value);
        *packet_bytes= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_image_position(raw1394handle_t handle, nodeid_t node,
 				  unsigned int mode, unsigned int left,
 				  unsigned int top)
{
    int retval= SetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_IMAGE_POSITION,
                                         (quadlet_t)((left << 16) | top));
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_image_size(raw1394handle_t handle, nodeid_t node,
                              unsigned int mode, unsigned int width,
                              unsigned int height)
{
    int retval= SetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_IMAGE_SIZE,
                                         (quadlet_t)((width << 16) | height));
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_color_coding_id(raw1394handle_t handle, nodeid_t node,
 				   unsigned int mode, unsigned int color_id)
{
    int retval;

    if ( (color_id < COLOR_FORMAT7_MIN) || (color_id > COLOR_FORMAT7_MAX) )
    {
        return DC1394_FAILURE;
    }

    color_id-= COLOR_FORMAT7_MIN;
    retval= SetCameraFormat7Register(handle, node, mode,
                                     REG_CAMERA_FORMAT7_COLOR_CODING_ID,
                                     (quadlet_t)color_id);
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_byte_per_packet(raw1394handle_t handle, nodeid_t node,
 				   unsigned int mode,
                                   unsigned int packet_bytes)
{
    int retval= SetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_BYTE_PER_PACKET,
                                         (quadlet_t)(packet_bytes) << 16 );
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
