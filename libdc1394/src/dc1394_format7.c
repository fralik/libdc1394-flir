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
 
#include "dc1394_control.h"
 
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
 
/* Internal functions */
 
static int
GetCameraFormat7Register(raw1394handle_t handle, nodeid_t node,
                         unsigned int mode, octlet_t offset, quadlet_t *value)
{
    int retval, retry= MAX_RETRIES;
    quadlet_t csr;
    
    if ((retval= dc1394_query_csr_offset(handle, node, mode, &csr)) < 0)
    {
        return retval;
    }

    /* retry a few times if necessary (addition by PDJ) */
    while(retry--)
    {
        retval= raw1394_read(handle, 0xffc0 | node,
                             CONFIG_ROM_BASE + csr + offset, 4, value);

        if (!retval)
        {
            /* conditionally byte swap the value (addition by PDJ) */
            *value= ntohl(*value);  
            return retval; 
        }
        else if (errno != EAGAIN)
        {
            return retval;
        }

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
    
    if ((retval= dc1394_query_csr_offset(handle, node, mode, &csr)) < 0)
    {
        return retval;
    }
  
    /* conditionally byte swap the value (addition by PDJ) */
    value= htonl(value);
 
    /* retry a few times if necessary (addition by PDJ) */
    while(retry--)
    {
        retval= raw1394_write(handle, 0xffc0 | node,
                              CONFIG_ROM_BASE + offset + csr, 4, &value);

        if (!retval || (errno != EAGAIN))
        {
            return retval;
        }

 	usleep(SLOW_DOWN);
    }
    
    return retval;
}

/* External functions */

int
dc1394_query_format7_max_image_size(raw1394handle_t handle, nodeid_t node,
 				    unsigned int mode,
                                    unsigned int *horizontal_size,
 				    unsigned int *vertical_size)
{
    int retval;
    quadlet_t value;
    
    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else     
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_MAX_IMAGE_SIZE_INQ,
                                         &value);
        *vertical_size  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *horizontal_size= (unsigned int) ( value & 0x0000FFFFUL );
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
   
    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_UNIT_SIZE_INQ,
                                        &value);
        *vertical_unit  = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *horizontal_unit= (unsigned int) ( value & 0x0000FFFFUL );
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
   
    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_IMAGE_POSITION,
                                        &value);
        *top_position = (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *left_position= (unsigned int) ( value & 0x0000FFFFUL );       
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

    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval=GetCameraFormat7Register(handle, node, mode,
                                        REG_CAMERA_FORMAT7_IMAGE_SIZE, &value);
        *height= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *width = (unsigned int) ( value & 0x0000FFFFUL );       
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_color_coding_id(raw1394handle_t handle, nodeid_t node,
 				     unsigned int mode, quadlet_t *value)
{
    int retval;
   
    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_COLOR_CODING_ID,
                                         value);
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_color_coding(raw1394handle_t handle, nodeid_t node,
 				  unsigned int mode, quadlet_t *value)
{
    int retval;

    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_COLOR_CODING_INQ,
                                         value);
    }

    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_query_format7_pixel_number(raw1394handle_t handle, nodeid_t node,
 				  unsigned int mode, unsigned int *pixnum)
{
    int retval;
    quadlet_t value;
   
    if (mode > MODE_FORMAT7_MAX)
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
   
    if (mode > MODE_FORMAT7_MAX)
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
 
        *total_bytes= (unsigned int) (value_lo | ( value_hi << 16) ); 
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

    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_PACKET_PARA_INQ,
                                         &value);
        *max_bytes= (unsigned int) ( value & 0xFFFF0000UL ) >> 16;
        *min_bytes= (unsigned int) ( value & 0x0000FFFFUL );       
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
   
    if (mode > MODE_FORMAT7_MAX)
    {
        return DC1394_FAILURE;
    }
    else
    {
        retval= GetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_BYTE_PER_PACKET,
                                         &value);
        *packet_bytes= (unsigned int) ( value & 0x0000FFFFUL );
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
                                         (quadlet_t)((top << 16) | left));
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_image_size(raw1394handle_t handle, nodeid_t node,
                              unsigned int mode, unsigned int width,
                              unsigned int height)
{
    int retval= SetCameraFormat7Register(handle, node, mode,
                                         REG_CAMERA_FORMAT7_IMAGE_SIZE,
                                         (quadlet_t)((height << 16) | width));
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
 
int
dc1394_set_format7_color_coding_id(raw1394handle_t handle, nodeid_t node,
 				   unsigned int mode, unsigned int color_id)
{
    int retval= SetCameraFormat7Register(handle, node, mode,
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
                                         (quadlet_t)packet_bytes);
    return (retval ? DC1394_FAILURE : DC1394_SUCCESS);
}
