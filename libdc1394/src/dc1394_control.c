/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
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
#include "dc1394_control.h"

#define CONFIG_ROM_BASE                0xFFFFF0000000ULL
#define CCR_BASE                       0xFFFFF0F00000ULL

/********************/
/* Base ROM offsets */
/********************/
#define ROM_BUS_INFO_BLOCK             0x400U
#define ROM_ROOT_DIRECTORY             0x414U

/**********************************/
/* Configuration Register Offsets */
/**********************************/

/* See the 1394-Based Digital Camera Spec. for definitions of these */
#define REG_CAMERA_INITIALIZE          0x000U
#define REG_CAMERA_V_FORMAT_INQ        0x100U
#define REG_CAMERA_V_MODE_INQ_BASE     0x180U
#define REG_CAMERA_V_RATE_INQ_BASE     0x200U
#define REG_CAMERA_V_REV_INQ_BASE      0x2C0U
#define REG_CAMERA_V_CSR_INQ_BASE      0x2E0U
#define REG_CAMERA_BASIC_FUNC_INQ      0x400U
#define REG_CAMERA_FEATURE_HI_INQ      0x404U
#define REG_CAMERA_FEATURE_LO_INQ      0x408U
#define REG_CAMERA_ADV_FEATURE_INQ     0x480U
#define REG_CAMERA_FEATURE_HI_BASE_INQ 0x500U
#define REG_CAMERA_FEATURE_LO_BASE_INQ 0x580U
#define REG_CAMERA_FRAME_RATE          0x600U
#define REG_CAMERA_VIDEO_MODE          0x604U
#define REG_CAMERA_VIDEO_FORMAT        0x608U
#define REG_CAMERA_ISO_DATA            0x60CU
#define REG_CAMERA_POWER               0x610U
#define REG_CAMERA_ISO_EN              0x614U
#define REG_CAMERA_ONE_SHOT            0x61CU
#define REG_CAMERA_MEM_SAVE_CH         0x620U
#define REG_CAMERA_CUR_MEM_CH          0x624U

#define REG_CAMERA_FEATURE_HI_BASE     0x800U
#define REG_CAMERA_FEATURE_LO_BASE     0x880U

#define REG_CAMERA_BRIGHTNESS	       0x800U
#define REG_CAMERA_EXPOSURE	       0x804U
#define REG_CAMERA_SHARPNESS	       0x808U
#define REG_CAMERA_WHITE_BALANCE       0x80CU
#define REG_CAMERA_HUE		       0x810U
#define REG_CAMERA_SATURATION	       0x814U
#define REG_CAMERA_GAMMA	       0x818U
#define REG_CAMERA_SHUTTER	       0x81CU
#define REG_CAMERA_GAIN		       0x820U
#define REG_CAMERA_IRIS		       0x824U
#define REG_CAMERA_FOCUS	       0x828U
#define REG_CAMERA_TEMPERATURE	       0x82CU
#define REG_CAMERA_TRIGGER_MODE	       0x830U
#define REG_CAMERA_ZOOM		       0x880U
#define REG_CAMERA_PAN		       0x884U
#define REG_CAMERA_TILT		       0x888U
#define REG_CAMERA_OPTICAL_FILTER      0x88CU
#define REG_CAMERA_CAPTURE_SIZE	       0x8C0U
#define REG_CAMERA_CAPTURE_QUALITY     0x8C4U

#define ON_VALUE                       0x80000000UL
#define OFF_VALUE                      0x00000000UL

/* Maximum number of write/read retries */
#define MAX_RETRIES                    10

/* Define this to get tons o' debug spew */
//#define SHOW_ERRORS

/**********************/
/* Internal functions */
/**********************/

static int
GetCameraROMValue(raw1394handle_t handle, nodeid_t node,
                  octlet_t offset, quadlet_t *value)
{
    int retval, retry= MAX_RETRIES;

    /* retry a few times if necessary (addition by PDJ) */
    while( (retry--) &&
	   ((retval= raw1394_read(handle, 0xffc0 | node,
				  CONFIG_ROM_BASE + offset, 4, value)) <= 0) )
    {
	usleep(0);
    }

    /* conditionally byte swap the value
       (thanks to Per Dalgas Jakobsen for this much nicer solution to the
       endianness problem) */
    *value= ntohl(*value);
    return(retval);
}

static int
GetCameraControlRegister(raw1394handle_t handle, nodeid_t node,
                         octlet_t offset, quadlet_t *value)
{
    int retval, retry= MAX_RETRIES;

    /* retry a few times if necessary (addition by PDJ) */
    while( (retry--) &&
	   ((retval= raw1394_read(handle, 0xffc0 | node, CCR_BASE + offset,
				 4, value)) <= 0) )
    {
	usleep(0);
    }

    /* conditionally byte swap the value (addition by PDJ) */
    *value= ntohl(*value);
    return(retval);
}

static int
SetCameraControlRegister(raw1394handle_t handle, nodeid_t node,
                         octlet_t offset, quadlet_t value)
{
    int retval, retry= MAX_RETRIES;

    /* conditionally byte swap the value (addition by PDJ) */
    value= htonl(value);

    /* retry a few times if necessary (addition by PDJ) */
    while( (retry--) &&
	   ((retval= raw1394_write(handle, 0xffc0 | node, CCR_BASE + offset,
				   4, &value)) <= 0) )
    {
	usleep(0);
    }

    return(retval);
}

static int
IsFeatureBitSet(quadlet_t value, unsigned int feature)
{

    if (feature >= FEATURE_ZOOM)
    {
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    return(value & (0x80000000UL >> feature));
}

static int
SetFeatureValue(raw1394handle_t handle, nodeid_t node,
                octlet_t featureoffset, unsigned int value)
{
    int retval;
    quadlet_t curval;

    if ( (retval= GetCameraControlRegister(handle, node, featureoffset,
                                           &curval)) < 0 )
    {
        return(retval);
    }

    return(SetCameraControlRegister(handle, node, featureoffset,
				 (curval & 0xFFFFF000UL) | (value & 0xFFFUL)));
}

static int
GetFeatureValue(raw1394handle_t handle, nodeid_t node,
                octlet_t featureoffset, unsigned int *value)
{
    quadlet_t quadval;
    int retval= GetCameraControlRegister(handle, node, featureoffset,
                                         &quadval);

    if (retval >= 0)
    {
        *value= (unsigned int)(quadval & 0xFFFUL);
    }

    return(retval);
}

/**********************/
/* External functions */
/**********************/

int
dc1394_is_camera(raw1394handle_t handle, nodeid_t node, dc1394bool_t *value)
{
    octlet_t offset= 0x424UL;
    quadlet_t quadval= 0;
    int retval;

#ifdef SHOW_ERRORS
    printf("Entering dc1394_is_camera\n");
    printf("Values : handle %d, nodeid %d\n", (int)handle, (int)node);
#endif

    /* get the unit_directory offset */
    if ( (retval= GetCameraROMValue(handle, node, offset, &quadval)) < 0 )
    {
#ifdef SHOW_ERRORS
        printf("Falling out of get unit directory offset\n");
#endif
        return(retval);
    }

    offset+= ((quadval & 0xFFFFFFUL) * 4) + 4;

    /* get the unit_spec_ID (should be 0x00A02D for 1394 digital camera) */
    if ( (retval= GetCameraROMValue(handle, node, offset, &quadval)) < 0 )
    {
#ifdef SHOW_ERRORS
        printf("Falling out of get unit spec id\n");
#endif
        return(retval);
    }

#ifdef SHOW_ERRORS
    printf("Hex value of value is : %x\n", (quadval & 0xFFFFFFUL));
#endif

    if ((quadval & 0xFFFFFFUL) != 0x00A02DUL)
    {
#ifdef SHOW_ERRORS
        printf("Returning DC1394_FALSE\n");
#endif

        *value= DC1394_FALSE;
    }
    else
    {
#ifdef SHOW_ERRORS
        printf("Returning DC1394_TRUE\n");
#endif
        *value= DC1394_TRUE;
    }

    return(retval);
}

int
dc1394_get_camera_info(raw1394handle_t handle, nodeid_t node,
                       struct dc1394_camerainfo *info)
{
    dc1394bool_t iscamera;
    int retval, i, len;
    quadlet_t offset, offset2;
    quadlet_t value[2];
    unsigned int count;

#ifdef SHOW_ERRORS
    printf("Entering dc1394_get_camera_info\n");
    printf("Node id in dc1394_get_camara_info is %d\n", (int)node);
#endif

    /* J.A. - tsk tsk - first bug I found.  We pass node, not i!!! */
    //if ( (retval= dc1394_is_camera(handle, i, &iscamera)) < 0 )
    if ( (retval= dc1394_is_camera(handle, node, &iscamera)) < 0 )
    {
#ifdef SHOW_ERRORS
        printf("Error - this is not a camera (get_camera_info)\n");
#endif
        return(retval);
    }
    else if (iscamera != DC1394_TRUE)
    {
        return(-1);

    }

    info->handle= handle;
    info->id= node;

    /* get the indirect_offset */
    if ( (retval= GetCameraROMValue(handle, node, (ROM_ROOT_DIRECTORY + 12),
                                    &offset)) < 0 )
    {
        return(retval);
    }

    /* make sure to strip 1st byte -- bug found by Robert Ficklin */
    offset&= 0xFFFFFFUL;
    offset<<= 2;
    offset+= (ROM_ROOT_DIRECTORY + 12);

    /* now get the EUID-64 */
    if ( (retval= GetCameraROMValue(handle, node, offset + 4, value)) < 0 )
    {
        return(retval);
    }

    if ( (retval= GetCameraROMValue(handle, node, offset + 8, &value[1])) < 0 )
    {
        return(retval);
    }

    info->euid_64= ((u_int64_t)value[0] << 32) | (u_int64_t)value[1];

    /* get the unit_directory offset */
    if ( (retval= GetCameraROMValue(handle, node, (ROM_ROOT_DIRECTORY + 16),
                                    &offset)) < 0 )
    {
        return(retval);
    }

    offset&= 0xFFFFFFUL;
    offset<<= 2;
    offset+= (ROM_ROOT_DIRECTORY + 16);

    /* now get the unit dependent directory offset */
    if ( (retval= GetCameraROMValue(handle, node, (offset + 12),
                                    &offset2)) < 0 )
    {
        return(retval);
    }

    offset2&= 0xFFFFFFUL;
    offset2<<= 2;
    offset2+= (offset + 12);

    /* now get the command_regs_base */
    if ( (retval= GetCameraROMValue(handle, node, (offset2 + 4),
                                    &offset)) < 0 )
    {
        return(retval);
    }

    offset&= 0xFFFFFFUL;
    offset<<= 2;
    info->ccr_offset= (octlet_t)offset;

    /* get the vendor_name_leaf offset */
    if ( (retval= GetCameraROMValue(handle, node, (offset2 + 8),
                                    &offset)) < 0 )
    {
        return(retval);
    }

    offset&= 0xFFFFFFUL;
    offset<<= 2;
    offset+= (offset2 + 20);
    count= 0;

    /* grab the vendor name */
    do
    {
        if ( (retval= GetCameraROMValue(handle, node, offset + count,
                                        value)) < 0 )
        {
            return(retval);
        }

        info->vendor[count++]= (value[0] >> 24);
        info->vendor[count++]= (value[0] >> 16) & 0xFFUL;
        info->vendor[count++]= (value[0] >> 8) & 0xFFUL;
        info->vendor[count++]= value[0] & 0xFFUL;
    }
    while ( (value[0] & 0xFFFFUL) && (count < MAX_CHARS) );

    if (count == MAX_CHARS)
    {
        info->vendor[count]= '\0';
    }

    /* get the model_name_leaf offset */
    if ( (retval= GetCameraROMValue(handle, node, (offset2 + 12),
                                    &offset)) < 0 )
    {
        return(retval);
    }

    offset&= 0xFFFFFFUL;
    offset<<= 2;
    offset+= (offset2 + 24);
    count= 0;

    /* grab the model name */
    do
    {
        if ( (retval= GetCameraROMValue(handle, node, offset + count,
                                        value)) < 0 )
        {
            return(retval);
        }

        info->model[count++]= (value[0] >> 24);
        info->model[count++]= (value[0] >> 16) & 0xFFUL;
        info->model[count++]= (value[0] >> 8) & 0xFFUL;
        info->model[count++]= value[0] & 0xFFUL;
    }
    while ( (value[0] & 0xFFFFUL) && (count < MAX_CHARS) );

    if (count == MAX_CHARS)
    {
        info->model[count]= '\0';
    }

    return(0);
}

int
dc1394_init_camera(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_INITIALIZE,
                                    ON_VALUE));
}

int
dc1394_query_supported_formats(raw1394handle_t handle, nodeid_t node,
                               quadlet_t *value)
{
    return(GetCameraControlRegister(handle, node, REG_CAMERA_V_FORMAT_INQ,
                                    value));
}

int
dc1394_query_supported_modes(raw1394handle_t handle, nodeid_t node,
                             unsigned int format, quadlet_t *value)
{

    if (format > FORMAT_MAX)
    {
	return(-1);
    }

    return(GetCameraControlRegister(handle, node,
				    REG_CAMERA_V_MODE_INQ_BASE +
				    (format * 0x04U), value));
}

int
dc1394_query_supported_framerates(raw1394handle_t handle, nodeid_t node,
                                  unsigned int format, unsigned int mode,
                                  quadlet_t *value)
{

    if ( (format > FORMAT_MAX) || (mode > MODE_MAX) )
    {
	return(-1);
    }

    return(GetCameraControlRegister(handle, node,
				    REG_CAMERA_V_RATE_INQ_BASE +
				    (format * 0x20U) + (mode * 0x04U),
				    value));
}

int
dc1394_query_revision(raw1394handle_t handle, nodeid_t node, int mode,
                      quadlet_t *value)
{

    if (mode > MODE_MAX)
    {
	return(-1);
    }

    return(GetCameraControlRegister(handle, node,
				    REG_CAMERA_V_REV_INQ_BASE + (mode * 0x04U),
				    value));
}

int
dc1394_query_csr_offset(raw1394handle_t handle, nodeid_t node, int mode,
                        quadlet_t *value)
{

    if (mode > MODE_MAX)
    {
	return(-1);
    }

    return(GetCameraControlRegister(handle, node,
				    REG_CAMERA_V_CSR_INQ_BASE + (mode * 0x04U),
				    value));
}

int
dc1394_query_basic_functionality(raw1394handle_t handle, nodeid_t node,
                                 quadlet_t *value)
{
    return(GetCameraControlRegister(handle, node, REG_CAMERA_BASIC_FUNC_INQ,
                                    value));
}

int
dc1394_query_feature_control(raw1394handle_t handle, nodeid_t node,
                             unsigned int feature, unsigned int *availability)
{
    quadlet_t value;
    octlet_t offset;
    int retval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_INQ;
    }

    if ((retval= GetCameraControlRegister(handle, node, offset, &value)) < 0 )
    {
        return(retval);
    }

    *availability= IsFeatureBitSet(value, feature);
    return(retval);
}

int
dc1394_query_advanced_feature_offset(raw1394handle_t handle, nodeid_t node,
                                     quadlet_t *value)
{
    return(GetCameraControlRegister(handle, node, REG_CAMERA_ADV_FEATURE_INQ,
                                    value));
}

int
dc1394_query_feature_characteristics(raw1394handle_t handle, nodeid_t node,
                                     unsigned int feature, quadlet_t *value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    return(GetCameraControlRegister(handle, node, offset + (feature * 0x04U),
				    value));
}

int
dc1394_get_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int *framerate)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_FRAME_RATE,
                                         &value);

    if (retval >= 0)
    {
        *framerate= (unsigned int)((value >> 29) & 0x7UL);
    }

    return(retval);
}

int
dc1394_set_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int framerate)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_FRAME_RATE,
                                    (quadlet_t)((framerate & 0x7UL) << 29)));
}

int
dc1394_get_video_mode(raw1394handle_t handle, nodeid_t node,
                      unsigned int *mode)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_MODE,
                                         &value);

    if (retval >= 0)
    {
        *mode= (unsigned int)((value >> 29) & 0x7UL);
    }

    return(retval);
}

int
dc1394_set_video_mode(raw1394handle_t handle, nodeid_t node, unsigned int mode)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_MODE,
                                    (quadlet_t)((mode & 0x7UL) << 29)));
}

int
dc1394_get_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int *format)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_FORMAT,
                                         &value);

    if (retval >= 0)
    {
        *format= (unsigned int)((value >> 29) & 0x7UL);
    }

    return(retval);
}

int
dc1394_set_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int format)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_FORMAT,
                                    (quadlet_t)((format & 0x7UL) << 29)));
}

int
dc1394_get_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int *channel, unsigned int *speed)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_ISO_DATA,
                                         &value);

    if (retval >= 0)
    {
        *channel= (unsigned int)((value >> 28) & 0xFUL);
        *speed= (unsigned int)((value >> 24) & 0x3UL);
    }

    return(retval);
}

int
dc1394_set_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int channel, unsigned int speed)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ISO_DATA,
                                    (quadlet_t)( ((channel & 0xFUL) << 28) |
                                                 ((speed & 0x3UL) << 24) )));
}

int
dc1394_camera_on(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_POWER, ON_VALUE));
}

int
dc1394_camera_off(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_POWER,
                                    OFF_VALUE));
}

int
dc1394_start_iso_transmission(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ISO_EN,
                                    ON_VALUE));
}

int
dc1394_stop_iso_transmission(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ISO_EN,
                                    OFF_VALUE));
}

int
dc1394_set_one_shot(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                    ON_VALUE));
}

int
dc1394_unset_one_shot(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                    OFF_VALUE));
}

int
dc1394_set_multi_shot(raw1394handle_t handle, nodeid_t node,
                      unsigned int numFrames)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                    (0x40000000UL | (numFrames & 0xFFFFUL))));
}

int
dc1394_unset_multi_shot(raw1394handle_t handle, nodeid_t node)
{
    return(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                    OFF_VALUE));
}

int
dc1394_get_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int *brightness)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_BRIGHTNESS, brightness));
}

int
dc1394_set_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int brightness)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_BRIGHTNESS, brightness));
}

int
dc1394_get_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int *exposure)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_EXPOSURE, exposure));
}

int
dc1394_set_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int exposure)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_EXPOSURE, exposure));
}

int
dc1394_get_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int *sharpness)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_SHARPNESS, sharpness));
}

int
dc1394_set_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int sharpness)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_SHARPNESS, sharpness));
}

int
dc1394_get_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int *u_b_value, unsigned int *v_r_value)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_WHITE_BALANCE, &value);

    if (retval >= 0)
    {
        *u_b_value= (unsigned int)((value & 0xFFF000UL) >> 12);
        *v_r_value= (unsigned int)(value & 0xFFFUL);
    }

    return(retval);
}

int
dc1394_set_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int u_b_value, unsigned int v_r_value)
{
    int retval;
    quadlet_t curval;

    if ( (retval= GetCameraControlRegister(handle, node,
                                           REG_CAMERA_WHITE_BALANCE,
                                           &curval)) < 0 )
    {
        return(retval);
    }

    curval= (curval & 0xFF000000UL) | ( ((u_b_value & 0xFFFUL) << 12) |
                                        (v_r_value & 0xFFFUL) );
    return(SetCameraControlRegister(handle, node, REG_CAMERA_WHITE_BALANCE,
                                    curval));
}

int
dc1394_get_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int *hue)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_HUE, hue));
}

int
dc1394_set_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int hue)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_HUE, hue));
}

int
dc1394_get_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int *saturation)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_SATURATION, saturation));
}

int
dc1394_set_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int saturation)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_SATURATION, saturation));
}

int
dc1394_get_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int *gamma)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_GAMMA, gamma));
}

int
dc1394_set_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int gamma)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_GAMMA, gamma));
}

int
dc1394_get_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int *shutter)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_SHUTTER, shutter));
}

int
dc1394_set_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int shutter)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_SHUTTER, shutter));
}

int
dc1394_get_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int *gain)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_GAIN, gain));
}

int
dc1394_set_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int gain)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_GAIN, gain));
}

int
dc1394_get_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int *iris)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_IRIS, iris));
}

int
dc1394_set_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int iris)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_IRIS, iris));
}

int
dc1394_get_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int *focus)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_FOCUS, focus));
}

int
dc1394_set_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int focus)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_FOCUS, focus));
}

int
dc1394_get_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int *target_temperature,
		       unsigned int *temperature)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_TEMPERATURE, &value);

    if (retval >= 0)
    {
	*target_temperature= (unsigned int)((value >> 12) & 0xFFF);
        *temperature= (unsigned int)(value & 0xFFFUL);
    }

    return(retval);
}

int
dc1394_set_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int target_temperature)
{
    int retval;
    quadlet_t curval;

    if ( (retval= GetCameraControlRegister(handle, node,
                                           REG_CAMERA_TEMPERATURE,
                                           &curval)) < 0 )
    {
        return(retval);
    }

    curval= (curval & 0xFF000FFFUL) | ((target_temperature & 0xFFFUL) << 12);
    return(SetCameraControlRegister(handle, node, REG_CAMERA_TEMPERATURE,
                                    curval));
}

int
dc1394_get_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int *mode)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_TRIGGER_MODE, &value);

    if (retval >= 0)
    {
        *mode= (unsigned int)( ((value >> 16) & 0xFUL) );
    }

    return(retval);
}

int
dc1394_set_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int mode)
{
    int retval;
    quadlet_t curval;

    if ( (retval= GetCameraControlRegister(handle, node,
                                           REG_CAMERA_TRIGGER_MODE,
                                           &curval)) < 0 )
    {
        return(retval);
    }

    curval= (curval & 0xFFF0FFFFUL) | ((mode & 0xFUL) << 16);
    return(SetCameraControlRegister(handle, node, REG_CAMERA_TRIGGER_MODE,
                                    curval));
}

int
dc1394_get_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int *zoom)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_ZOOM, zoom));
}

int
dc1394_set_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int zoom)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_ZOOM, zoom));
}

int
dc1394_get_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int *pan)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_PAN, pan));
}

int
dc1394_set_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int pan)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_PAN, pan));
}

int
dc1394_get_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int *tilt)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_TILT, tilt));
}

int
dc1394_set_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int tilt)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_TILT, tilt));
}

int
dc1394_get_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int *optical_filter)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_OPTICAL_FILTER,
                           optical_filter));
}

int
dc1394_set_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int optical_filter)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_OPTICAL_FILTER,
                           optical_filter));
}

int
dc1394_get_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int *capture_size)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_CAPTURE_SIZE,
                           capture_size));
}

int
dc1394_set_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int capture_size)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_CAPTURE_SIZE,
                           capture_size));
}

int
dc1394_get_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int *capture_quality)
{
    return(GetFeatureValue(handle, node, REG_CAMERA_CAPTURE_QUALITY,
                           capture_quality));
}

int
dc1394_set_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int capture_quality)
{
    return(SetFeatureValue(handle, node, REG_CAMERA_CAPTURE_QUALITY,
                           capture_quality));
}

int
dc1394_get_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int *value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    return(GetFeatureValue(handle, node, offset + (feature * 0x04U),
			   value));
}

int
dc1394_set_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    return(SetFeatureValue(handle, node, offset + (feature * 0x04U),
			   value));
}

int
dc1394_is_feature_present(raw1394handle_t handle, nodeid_t node,
                          unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x80000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_has_one_push_auto(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x10000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_is_one_push_in_operation(raw1394handle_t handle, nodeid_t node,
                                unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x04000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_start_one_push(raw1394handle_t handle, nodeid_t node,
                      unsigned int feature)
{
    int retval;
    octlet_t offset;
    quadlet_t curval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &curval)) < 0)
    {
        return(retval);
    }

    if (!(curval & 0x04000000UL))
    {
        curval|= 0x04000000UL;
        return(SetCameraControlRegister(handle, node,
					offset + (feature * 0x04U), curval));
    }

    return(retval);
}

int
dc1394_can_read_out(raw1394handle_t handle, nodeid_t node,
                    unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x08000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_can_turn_on_off(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x04000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_is_feature_on(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x02000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_feature_on_off(raw1394handle_t handle, nodeid_t node,
                      unsigned int feature, unsigned int value)
{
    int retval;
    octlet_t offset;
    quadlet_t curval;

    if (feature > FEATURE_MAX)
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &curval)) < 0)
    {
        return(retval);
    }

    if (!(curval & 0x02000000UL))
    {
        curval|= 0x02000000UL;
        return(SetCameraControlRegister(handle, node,
					offset + (feature * 0x04U), curval));
    }
    else if (!value && (curval & 0x02000000UL))
    {
        curval&= 0xFDFFFFFFUL;
        return(SetCameraControlRegister(handle, node,
					offset + (feature * 0x04U), curval));
    }

    return(retval);
}

int
dc1394_has_auto_mode(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x02000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_has_manual_mode(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x01000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_is_feature_auto(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0 )
    {

        if (quadval & 0x01000000UL)
        {
            *value= DC1394_TRUE;
        }
        else
        {
            *value= DC1394_FALSE;
        }

    }

    return(retval);
}

int
dc1394_auto_on_off(raw1394handle_t handle, nodeid_t node,
                   unsigned int feature, unsigned int value)
{
    int retval;
    octlet_t offset;
    quadlet_t curval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &curval)) < 0)
    {
        return(retval);
    }

    if (!(curval & 0x01000000UL))
    {
        curval|= 0x01000000UL;
        return(SetCameraControlRegister(handle, node,
					offset + (feature * 0x04U), curval));
    }
    else if (!value && (curval & 0x01000000UL))
    {
        curval&= 0xFEFFFFFFUL;
        return(SetCameraControlRegister(handle, node,
					offset + (feature * 0x04U), curval));
    }

    return(retval);
}

int
dc1394_get_min_value(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, unsigned int *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0)
    {
        *value= (unsigned int)((quadval & 0xFFF000UL) >> 12);
    }

    return(retval);
}

int
dc1394_get_max_value(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, unsigned int *value)
{
    int retval;
    octlet_t offset;
    quadlet_t quadval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return(-1);
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
	feature-= FEATURE_ZOOM;

	if (feature >= FEATURE_CAPTURE_SIZE)
	{
	    feature+= 12;
	}

    }

    if ( (retval= GetCameraControlRegister(handle, node,
					   offset + (feature * 0x04U),
					   &quadval)) >= 0)
    {
        *value= (unsigned int)(quadval & 0xFFFUL);
    }

    return(retval);
}
