/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
 * Additions by Chris Urmson <curmson@ri.cmu.edu>
 *
 * Acknowledgments:
 * Per Dalgas Jakobsen <pdj@maridan.dk>
 *   - Added retries to ROM and CSR reads
 *   - Nicer endianness handling
 *
 * Robert Ficklin <rficklin@westengineering.com>
 *   - bug fixes
 * 
 * Julie Andreotti <JulieAndreotti@smarttech.com>
 *   - bug fixes
 * 
 *  Ann Dang <AnnDang@smarttech.com>
 *   - bug fixes
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
#include <video1394.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
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


//#define CAMERA_READ_OK  0x20000
//#define CAMERA_WRITE_OK 0x1000f

/**************************/
/*  constant definitions  */
/**************************/
const char * dc1394_feature_desc[NUM_FEATURES] = {
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
    "Zoom",
    "Pan",
    "Tilt",
    "Optical Filter",
    "Capture Size",
    "Capture Quality"
};


/*these arrays define how many image quadlets there
  are in a packet given a mode and a frame rate
  This is defined in the 1394 digital camera spec 
*/
const int quadlets_per_packet_format_0[36] = 
{ -1, -1, 15, 30, 60, -1,
  -1, 20, 40, 80,160, -1,
  -1, 60,120,240,480,-1,
  -1, 80,160,320,640,-1,
  -1,120,240,280,960, -1,
  -1, 40, 80,160,320,640
};

const int quadlets_per_packet_format_1[36] = 
{ -1,125,250,500,1000,  -1,
  -1, -1,375,750,  -1,  -1,
  -1, -1,125,250, 500,1000,
  96,192,384,768,  -1,  -1,
  144,288,576, -1,  -1,  -1,
  48, 96,192,384, 768,  -1
};

const int quadlets_per_packet_format_2[36] = 
{ 160,320, 640,  -1,-1,-1,
  240,480, 960,  -1,-1,-1,
  80 ,160, 320, 640,-1,-1,
  250,500,1000,  -1,-1,-1,
  375,750,  -1,  -1,-1,-1,
  125,250, 500,1000,-1,-1
};
  

    

//#define SHOW_ERRORS

/* Maximum number of write/read retries */
#define MAX_RETRIES                    20
/* A hard compiled factor that makes sure async read and writes don't happen too fast*/
#define SLOW_DOWN 20

/* transaction acknowldegements (this should be in the raw1394 headers) */
#define ACK_COMPLETE   0x0001U
#define ACK_PENDING    0x0002U
#define ACK_LOCAL      0x0010U

/*Response codes (this should be in the raw1394 headers) */
//not currently used
#define RESP_COMPLETE    0x0000U
#define RESP_SONY_HACK   0x000fU



/**********************/
/* Internal functions */
/**********************/

int
_dc1394_get_wh_from_format(int format, int mode, int *w, int *h) 
{
    switch(format) 
    {
    case FORMAT_VGA_NONCOMPRESSED:
        switch(mode) 
        {
        case 0:
            *w = 160;*h=120;
            return DC1394_SUCCESS;
        case 1:
            *w = 320;*h=240;
            return DC1394_SUCCESS;
        case 2:
        case 3:
        case 4:
        case 5:
            *w =640;*h=480;
            return DC1394_SUCCESS;
        default:
            return DC1394_FAILURE;
        }
    case FORMAT_SVGA_NONCOMPRESSED_1:
        switch(mode) 
        {
        case 0:
        case 1:
        case 2:
            *w=800;*h=600;
            return DC1394_SUCCESS;
        case 3:
        case 4:
        case 5:
            *w=1024;*h=768;
            return DC1394_SUCCESS;
        default:
            return DC1394_FAILURE;
        }
    case FORMAT_SVGA_NONCOMPRESSED_2:
        switch(mode) 
        {
        case 0:
        case 1:
        case 2:
            *w=1280;*h=960;
            return DC1394_SUCCESS;
        case 3:
        case 4:
        case 5:
            *w=1600;*h=1200;
            return DC1394_SUCCESS;
        default:
            return DC1394_FAILURE;
        }
    default:
        return DC1394_FAILURE;
    }
}
	

/*****************************************************
_dc1394_get_quadlets_per_packet
this routine reports the number of useful image quadlets 
per packet
*****************************************************/
int 
_dc1394_get_quadlets_per_packet(int format,int mode, int frame_rate) 
{
    if (((-1<mode) && (mode<NUM_MODES)) && 
        ((-1<frame_rate) && frame_rate<NUM_FRAMERATES)) 
    {
        switch(format) 
        {
        case FORMAT_VGA_NONCOMPRESSED:
            return quadlets_per_packet_format_0[6*mode+frame_rate];
        case FORMAT_SVGA_NONCOMPRESSED_1:
            return quadlets_per_packet_format_1[6*mode+frame_rate];
        case FORMAT_SVGA_NONCOMPRESSED_2:
            return quadlets_per_packet_format_2[6*mode+frame_rate];
        default:
            printf("(%s) Quadlets per packet unkown for format %d!\n",
                   __FILE__,format);
            return -1;
        }
    } else 
    {
        printf("(%s) Invalid framerate (%d) or mode (%d)!\n",__FILE__,
               frame_rate,format);
        return -1;
    }      
}

/*****************************************************
_dc1394_quadlets_from_format
this routine reports the number of quadlets that make up a 
frame given the format and mode
*****************************************************/
int 
_dc1394_quadlets_from_format(int format, int mode) 
{
    switch (format) 
    {
    case FORMAT_VGA_NONCOMPRESSED:
        switch(mode) 
        {
        case MODE_160x120_YUV444:
            return 14400; //160x120*3/4
        case MODE_320x240_YUV422:
            return 38400;//320x240/2
        case MODE_640x480_YUV411:
            return 115200;//640x480x12/32
        case MODE_640x480_YUV422:
            return 153600;//640x480/2
        case MODE_640x480_RGB:
            return 230400;//640x480x3/4
        case MODE_640x480_MONO:
            return 76800;//640x480/4
        default:
            printf("(%s) Improper mode specified: %d\n",__FILE__,mode);
            return -1; 
        }
    case FORMAT_SVGA_NONCOMPRESSED_1: 
        switch(mode) 
        {
        case 0:
            return 240000;//800x600/2
        case 1:
            return 360000;//800x600x3/4
        case 2:
            return 120000;//800x600/4
        case 3:
            return 393216;//1024x768/2
        case 4:
            return 589824;//1024x768x3/4
        case 5:
            return 196608;//1024x768/4
        default:
            printf("(%s) Improper mode specified: %d\n",__FILE__,mode);
            return -1;
        }
    case FORMAT_SVGA_NONCOMPRESSED_2:
        switch (mode) 
        {
        case 0:
            return 61440;//1280x960/2
        case 1:
            return 921600;//1280x960x3/4
        case 2:
            return 307200;//1280x960/4
        case 3:
            return 960000;//1600x1200/2
        case 4:
            return 1440000;//1600x1200x3/4
        case 5:
            return 480000;//1600x1200/4
        default:
            printf("(%s) Improper mode specified: %d\n",__FILE__,mode);
            return -1;
        }
    case FORMAT_STILL_IMAGE:
        printf("(%s) Don't know how many quadlets per frame for FORMAT_STILL_IMAGE mode:%d\n",__FILE__,mode);
        return -1;
    case FORMAT_SCALABLE_IMAGE_SIZE:
        printf("(%s) Don't know how many quadlets per frame for FORMAT_SCALABLE_IMAGE mode:%d\n",__FILE__,mode);
        return -1;
    default:
        printf("(%s) Improper format specified: %d\n",__FILE__,format);
        return -1;
    }
    return -1;
}

#define DC1394_READ_END(retval) return _dc1394_read_check(retval);
#define DC1394_WRITE_END(retval) return _dc1394_write_check(retval);


int 
_dc1394_read_check(int retval) 
{
    int ack;
    int rcode;
    ack = retval >> 16;
    rcode = retval & 0xffff;
#ifdef SHOW_ERRORS
    printf("read ack of %x rcode of %x\n",ack,rcode);
#endif
    if (((ack==ACK_PENDING)||(ack==ACK_LOCAL)) && (rcode==RESP_COMPLETE)) 
    {
        return DC1394_SUCCESS; 
    } else 
    {
        return DC1394_FAILURE;
    }
}

int 
_dc1394_write_check(int retval) 
{
    int ack;
    int rcode;
    ack = retval >> 16;
    rcode = retval & 0xffff;
#ifdef SHOW_ERRORS
    printf("write ack of %x rcode of %x\n",ack,rcode);
#endif
    if (((ack==ACK_PENDING)||(ack==ACK_LOCAL)||(ack==ACK_COMPLETE)) && 
        ((rcode==RESP_COMPLETE)|| (rcode==RESP_SONY_HACK)) ) 
    {
        return DC1394_SUCCESS;
    } else 
    {
        return DC1394_FAILURE;
    }
}

static int
GetCameraROMValue(raw1394handle_t handle, nodeid_t node,
                  octlet_t offset, quadlet_t *value) {
    int retval, retry= MAX_RETRIES;
    int ack, rcode;
  
    /* retry a few times if necessary (addition by PDJ) */
  
    while( (retry--)) 
    {
        retval= raw1394_read(handle, 0xffc0 | node, CONFIG_ROM_BASE + offset, 4, value);
        if (retval>=0) 
        {
            ack = retval >>16;  
            rcode = retval & 0xffff;  
            //      printf( "ROM ack is: %x, rcode is %x\n",ack,rcode);  
            if ( ((ack==ACK_PENDING)|| ack==ACK_LOCAL) && (rcode==RESP_COMPLETE)) 
            {  
                /* conditionally byte swap the value (addition by PDJ) */
                *value= ntohl(*value);  
                return(retval); 
            }
        }
        usleep(SLOW_DOWN);
    }
    
    *value= ntohl(*value);
    return(retval);	  
}

static int
GetCameraControlRegister(raw1394handle_t handle, nodeid_t node,
                         octlet_t offset, quadlet_t *value)
{
    int retval, retry= MAX_RETRIES;
    int rcode, ack;
    //  usleep(SLOW_DOWN);
    /* retry a few times if necessary (addition by PDJ) */
    while( (retry--)) 
    {
        retval= raw1394_read(handle, 0xffc0 | node, CCR_BASE + offset, 4, value);
        if (retval>=0) 
        {
            ack = retval >>16; 
            rcode = retval & 0xffff; 
            //      printf( "Get ack is: %x, rcode is %x\n",ack,rcode); 
            if ( ((ack==ACK_PENDING)|| ack==ACK_LOCAL) && (rcode==RESP_COMPLETE)) 
            { 
                /* conditionally byte swap the value (addition by PDJ) */
                *value= ntohl(*value); 
                return(retval); 
            }  
      
        } 
        usleep(SLOW_DOWN); 
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
    int rcode, ack;

    /* conditionally byte swap the value (addition by PDJ) */
    value= htonl(value);
    while( (retry--)) 
    {
        retval= raw1394_write(handle, 0xffc0 | node, CCR_BASE + offset, 4,&value);
        if (retval>=0) 
        {
            ack = retval >>16;
            rcode = retval & 0xffff;
            //	printf( "Set ack is: %x, rcode is %x\n",ack,rcode);
            if ( ((ack==ACK_PENDING)|| (ack==ACK_LOCAL)|| (ack==ACK_COMPLETE)) && ((rcode==RESP_COMPLETE) || (rcode==RESP_SONY_HACK))) 
            {
                return(retval);
            }
            
            
        }
        usleep(SLOW_DOWN);
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

    if ( _dc1394_read_check((retval= 
                             GetCameraControlRegister(handle, node, 
                                                      featureoffset,
                                                      &curval))) !=DC1394_SUCCESS )
    {
	return DC1394_FAILURE;
    }
    retval =SetCameraControlRegister(handle, node, featureoffset,
				     (curval & 0xFFFFF000UL) | (value & 0xFFFUL));
    DC1394_WRITE_END(retval);
}

static int
GetFeatureValue(raw1394handle_t handle, nodeid_t node,
                octlet_t featureoffset, unsigned int *value)
{
    quadlet_t quadval;
    int retval= GetCameraControlRegister(handle, node, featureoffset,
                                         &quadval);
    
    *value= (unsigned int)(quadval & 0xFFFUL);
    DC1394_READ_END(retval);
}

/**********************/
/* External functions */
/**********************/
nodeid_t* 
dc1394_get_camera_nodes(raw1394handle_t handle, int *numCameras,
                        int showCameras) 
{
    nodeid_t * nodes; 
    dc1394bool_t isCamera;
    int numNodes;
    int i;  
    dc1394_camerainfo caminfo;
    numNodes= raw1394_get_nodecount(handle);
    *numCameras=0;
    /*we know that the computer isn't a camera so there are only
      numNodes-1 possible camera nodes*/
    nodes=(nodeid_t *)calloc(numNodes-1,sizeof(nodeid_t));
    for (i=0;i<numNodes-1;i++) 
    {
        nodes[i]=DC1394_NO_CAMERA;
    }

    for (i=0;i<numNodes;i++) 
    {
        dc1394_is_camera(handle,i,&isCamera);
        if (isCamera) 
        {
            nodes[*numCameras]=i; 
            (*numCameras)++;
            if (showCameras)
            {
                if (dc1394_get_camera_info(handle,i,&caminfo)
                    ==DC1394_SUCCESS) 
                {
                    dc1394_print_camera_info(&caminfo);
                } else 
                {
                    printf("Couldn't get camera info (%d)!\n",i);
                }
            }
        } else 
        {
            printf("node %d is not a camera\n",i);
        }
    }
    return nodes; 
}

/*****************************************************
dc1394_get_camera_nodes
this returns the available cameras on the bus.
It returns the node id's in the same index as the id specified
the ids array contains a list of the low quadlet of the unique camera 
ids.
returns -1 in numCameras and NULL from the call if there is a problem, 
otherwise the number of cameras and the nodeid_t array from the call
*****************************************************/
nodeid_t* 
dc1394_get_sorted_camera_nodes(raw1394handle_t handle,int numIds, 
                               int *ids,int * numCameras,
                               int showCameras) 
{
    int numNodes;
    int i,j;
    dc1394bool_t isCamera;
    int uid;
    int foundId;
    int extraId;
    nodeid_t *nodes;
    //  int numCameras=0;
    dc1394_camerainfo caminfo;
    *numCameras=0;
    //  printf("started get camera nodes\n");

    numNodes= raw1394_get_nodecount(handle);
    //  printf("numnodes is %d",numNodes);
    /*we know that the computer isn't a camera so there are only
      numNodes-1 possible camera nodes*/
    nodes=(nodeid_t *)calloc(numNodes-1,sizeof(nodeid_t));
    for (i=0;i<numNodes-1;i++) 
    {
        nodes[i]=0xffff;
    }
    //  printf("done clearing calloc space\n");
    extraId=numIds;
    for (i=0;i<numNodes;i++) 
    {
        dc1394_is_camera(handle,i,&isCamera);
        if (isCamera) 
        {
            (*numCameras)++;
            dc1394_get_camera_info(handle,i,&caminfo);
            if (showCameras) dc1394_print_camera_info(&caminfo);
            uid=caminfo.euid_64 & 0xffffffff;
            foundId =0;
            /*check to see if the unique id is one we know...*/
            //      printf("numIds is: %d\n",numIds);
            fflush(stdout);
            for (j=0;j<numIds;j++) 
            {
                //	printf("comparing (index %d) %x to %x\n",j,ids[j],uid);
                if (ids[j]==uid) 
                {
                    //	  printf("found a match %x\n",ids[j]);
                    nodes[j]=i;
                    foundId=1;
                    break;
                }
            }
            /* if it isn't then we need to put it in one of the extra
               spaces- but check first to make sure we aren't overflowing
               our bounds*/
            if (foundId==0) 
            {
                //	printf("storing as an extra id\n");
                if (extraId>=(numNodes-1)) 
                {
                    //  printf("(%s) Given ID's don't exist- failing!\n");
                    *numCameras=-1;
                    free(nodes);
                    return NULL;
                }
                nodes[extraId++]=i;
            }
        }else 
        {// if (isCamera)
            //      printf("isn't a camera\n");
        }
    }
    return nodes;
}



/*****************************************************
dc1394_create_handle
this creates a raw1394_handle
if a handle can't be created, it returns NULL
*****************************************************/
raw1394handle_t 
dc1394_create_handle(int port) 
{
    raw1394handle_t handle;
    handle = raw1394_get_handle();
    if (!handle) 
    {
        printf("(%s) Couldn't get raw1394 handle!\n",__FILE__);
        return NULL;
    }
    if (raw1394_set_port(handle, port) < 0) 
    {	
        if (handle != NULL)
            raw1394_destroy_handle(handle);
        printf("(%s) Couldn't raw1394_set_port!\n",__FILE__);
        return NULL;
    }
    return handle;
}


int
dc1394_is_camera(raw1394handle_t handle, nodeid_t node, dc1394bool_t *value)
{
    octlet_t offset= 0x424UL;
    quadlet_t quadval;
    int retval;

    /* get the unit_directory offset */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, offset, &quadval)) !=DC1394_SUCCESS )
    {
	*value=DC1394_FALSE;
	return DC1394_FAILURE;
	//        return(retval);
    }

    offset+= ((quadval & 0xFFFFFFUL) * 4) + 4;
    usleep(1000);

    /* get the unit_spec_ID (should be 0x00A02D for 1394 digital camera) */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, offset, &quadval)) !=DC1394_SUCCESS )
    {
	*value=DC1394_FALSE;
	return DC1394_FAILURE;
    }

    if ((quadval & 0xFFFFFFUL) != 0x00A02DUL)
    {

        *value= DC1394_FALSE;
    }
    else
    {
        *value= DC1394_TRUE;
    }
    DC1394_READ_END(retval);

}

void dc1394_print_camera_info(dc1394_camerainfo *info) 
{
    quadlet_t value[2];
    value[0] = info->euid_64 & 0xffffffff;
    value[1] = (info->euid_64 >>32) & 0xffffffff;
    printf("CAMERA INFO\n===============\n");
    printf("Node: %x\n",info->id);
    //  printf("value 1 is: %08x\n",value[1]);
    //  printf("value 0 is: %08x\n",value[0]);
    printf("CCR_Offset: %ux\n",info->ccr_offset);
    printf("UID: 0x%08x%08x\n",value[1], value[0]);
    printf("Vendor: %s\tModel: %s\n\n",info->vendor, info->model);
}



int
dc1394_get_camera_info(raw1394handle_t handle, nodeid_t node,
                       dc1394_camerainfo *info)
{
    dc1394bool_t iscamera;
    int retval, len;
    quadlet_t offset, offset2;
    quadlet_t value[2];
    unsigned int count;

    if ( (retval= dc1394_is_camera(handle, node, &iscamera)) !=DC1394_SUCCESS )
        /* J.A. - tsk tsk - first bug I found.  We pass node, not i!!! */
        //    if ( (retval= dc1394_is_camera(handle, i, &iscamera)) < 0 )
    {
#ifdef SHOW_ERRORS
        printf("Error - this is not a camera (get_camera_info)\n");
#endif
        return DC1394_FAILURE;
    }
    else if (iscamera != DC1394_TRUE)
    {
        return DC1394_FAILURE;
    }

    info->handle= handle;
    info->id= node;

    /* get the indirect_offset */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, (ROM_ROOT_DIRECTORY + 12),
                                                      &offset)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }
    
    offset<<= 2;
    offset &=0x00ffffff;// this is needed to clear the top two bytes which otherwise causes trouble
    offset+= (ROM_ROOT_DIRECTORY + 12);

    /* now get the EUID-64 */
    /* fixed this, the value at offset is that start of the leaf.  
       The first element is 0x0002 CRC
       the actual UID lives at +4 and +8  Chris Urmson
    */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, offset+4, value)) != DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, offset + 8, &value[1])) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }
    info->euid_64= ((u_int64_t)value[0] << 32) | (u_int64_t)value[1];

    /* get the unit_directory offset */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, (ROM_ROOT_DIRECTORY + 16),
                                                      &offset)) !=DC1394_SUCCESS )
    {
        //      printf("failing %d", __LINE__);
        return DC1394_FAILURE;
        //        return(retval);
    }

    offset<<= 2;
    offset &=0x00ffffff;
    offset+= (ROM_ROOT_DIRECTORY + 16);


    /* now get the unit dependent directory offset */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, (offset + 12),
                                                      &offset2)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    offset2<<= 2;
    offset2+= (offset + 12);
    offset2 &=0x00ffffff;

    /* now get the command_regs_base */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, (offset2 + 4),
                                                      &offset)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    offset<<= 2;
    offset &=0x00ffffff;
    info->ccr_offset= (octlet_t)offset;


    /* get the vendor_name_leaf offset */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, (offset2 + 8),
                                                      &offset)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    offset<<= 2;
    offset &=0x00ffffff;
    offset+= (offset2 + 8);

    /* read in the length of the vendor name */
    if ( _dc1394_read_check(retval= GetCameraROMValue(handle, node, offset, value)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    len= (int)((value[0] >> 16) & 0xFFFFUL);

    if (len > MAX_CHARS)
    {
        len= MAX_CHARS;
    }

    offset+= 12;
    count= 0;

    /* grab the vendor name */
    while (len > 0)
    {

        if (_dc1394_read_check (retval= GetCameraROMValue(handle, node, offset + count,
                                                          value)) !=DC1394_SUCCESS )
        {
            return DC1394_FAILURE;
        }

        info->vendor[count++]= (value[0] >> 24);
        info->vendor[count++]= (value[0] >> 16) & 0xFFUL;
        info->vendor[count++]= (value[0] >> 8) & 0xFFUL;
        info->vendor[count++]= value[0] & 0xFFUL;
        len-= 4;
    }

    info->vendor[count]= '\0';

    /* get the model_name_leaf offset */
    if (_dc1394_read_check (retval= GetCameraROMValue(handle, node, (offset2 + 12),
                                                      &offset)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    offset<<= 2;
    offset &=0x00ffffff;
    offset+= (offset2 + 12);

    /* read in the length of the model name */
    if (_dc1394_read_check (retval= GetCameraROMValue(handle, node, offset, value)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    len= (int)((value[0] >> 16) & 0xFFFFUL);

    if (len > MAX_CHARS)
    {
        len= MAX_CHARS;
    }

    offset+= 12;
    count= 0;

    /* grab the model name */
    while (len > 0)
    {

        if (_dc1394_read_check(retval= GetCameraROMValue(handle, node, 
                                                         offset + count,
                                                         value)) !=DC1394_SUCCESS )
        {
            return DC1394_FAILURE;
        }

        info->model[count++]= (value[0] >> 24);
        info->model[count++]= (value[0] >> 16) & 0xFFUL;
        info->model[count++]= (value[0] >> 8) & 0xFFUL;
        info->model[count++]= value[0] & 0xFFUL;
        len-= 4;
    }

    info->model[count]= '\0';
    return DC1394_SUCCESS;
}


/*****************************************************
dc1394_get_camera_feature_set
collects the available features for the camera
described by node and stores them in features.
*****************************************************/
int 
dc1394_get_camera_feature_set(raw1394handle_t handle, nodeid_t node,
                              dc1394_feature_set *features) 
{
    int i;
    for (i=FEATURE_MIN;i<FEATURE_MAX;i++) 
    {
        features->feature[i].feature_id=i;
        dc1394_get_camera_feature(handle,node,&features->feature[i]);
    }
    return DC1394_SUCCESS;
}

/*****************************************************
dc1394_get_camera_feature
stores the bounds and options associated with the
feature described by feature->feature_id
*****************************************************/
int 
dc1394_get_camera_feature(raw1394handle_t handle, nodeid_t node,
                          dc1394_feature_info *feature) 
{
    octlet_t offset;
    quadlet_t value;
    int f;
    int index;
    f= feature->feature_id;
    if (f<FEATURE_BRIGHTNESS || f>FEATURE_CAPTURE_QUALITY) 
    {
        return DC1394_FAILURE;
    } else if (f<FEATURE_ZOOM)
    {
        offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
        index =f;
    } else 
    {
        offset= REG_CAMERA_FEATURE_LO_BASE_INQ;
        index = f- FEATURE_ZOOM;
        if (f >= FEATURE_CAPTURE_SIZE) 
        {
            index+= 12;
        }
    }

    if ( _dc1394_read_check( GetCameraControlRegister(handle, node,offset 
                                                      + (index * 0x04U),
                                                      &value)) 
         != DC1394_SUCCESS) 
    {
        return DC1394_FAILURE;
    }

    feature->available = (value & 0x80000000UL)?DC1394_TRUE:DC1394_FALSE;
    if (feature->available==DC1394_FALSE) 
    {
        return DC1394_SUCCESS;
    }

    if (f!=FEATURE_TRIGGER) 
    {
        feature->one_push= (value & 0x10000000UL)?DC1394_TRUE:DC1394_FALSE;
    } else 
    {
        feature->one_push=DC1394_FALSE;
    }


    feature->readout_capable=(value & 0x08000000UL)?DC1394_TRUE:DC1394_FALSE;
    feature->on_off_capable= (value & 0x04000000UL)?DC1394_TRUE:DC1394_FALSE;
    if (f!=FEATURE_TRIGGER) 
    {
        feature->auto_capable=   (value & 0x02000000UL)?DC1394_TRUE:DC1394_FALSE;
        feature->manual_capable= (value & 0x01000000UL)?DC1394_TRUE:DC1394_FALSE;
    
        feature->min = (value & 0xFFF000UL)>>12;
        feature->max = (value & 0xFFFUL);
    } else 
    {
        feature->auto_capable = DC1394_FALSE;
        feature->manual_capable = DC1394_FALSE;
    }

    
    if (f < FEATURE_ZOOM) 
    {
        offset= REG_CAMERA_FEATURE_HI_BASE;
        index =f;
    } else 
    {
        offset= REG_CAMERA_FEATURE_LO_BASE;
        index =f- FEATURE_ZOOM;
        if (f >= FEATURE_CAPTURE_SIZE) {
            index+= 12;
        }
    } 

    if (_dc1394_read_check(GetCameraControlRegister(handle, node,  
						    offset + (index * 0x04U),
						    &value)) 
	!=DC1394_SUCCESS) 
    {

        return DC1394_FAILURE;
    }
    if (f!=FEATURE_TRIGGER) 
    {
        feature->one_push_active=(value & 0x04000000UL)?DC1394_TRUE:DC1394_FALSE;
    } else 
    {
        feature->one_push_active=DC1394_FALSE;
    }

    feature->is_on          =(value & 0x02000000UL)?DC1394_TRUE:DC1394_FALSE;

    if (f!=FEATURE_TRIGGER) 
    {
        feature->auto_active    =(value & 0x01000000UL)?DC1394_TRUE:DC1394_FALSE;
    } else 
    {
        feature->auto_active = DC1394_FALSE;
    }
    if (f!=FEATURE_TRIGGER && f!=FEATURE_WHITE_BALANCE && 
        f!= FEATURE_TEMPERATURE) 
    {
        feature->value = value &0xFFFUL;
    }
    return DC1394_SUCCESS;
}

/*****************************************************
dc1394_print_feature
displays the bounds and options of the given feature
*****************************************************/
void 
dc1394_print_feature(dc1394_feature_info *f) 
{
//    char * tags[2] = {"NO","YES"};
//    char * autoMode[2] = {"Manual","Auto"};
    int fid;
    fid = f->feature_id;
    if (fid<FEATURE_BRIGHTNESS || 
        fid>FEATURE_CAPTURE_QUALITY) 
    {
        return;
    }
    printf("%s:\n\t",dc1394_feature_desc[fid]);
    if (!f->available) 
    {
        printf("NOT AVAILABLE\n");
        return;
    }

    if (f->one_push) 
        printf("OP  ");
    if (f->readout_capable)
        printf("RC  ");
    if (f->on_off_capable)
        printf("O/OC  ");
    if (f->auto_capable)
        printf("AC  ");
    if (f->manual_capable)
        printf("MC  ");
    printf("\n");



    if (f->on_off_capable) 
    {
        if (f->is_on) 
            printf("\tFeature: ON  ");
        else
            printf("\tFeature: OFF  ");
    } else 
    {
        printf("\t");
    }
    if (f->one_push) 
    {
        if (f->one_push_active)
            printf("One push: ACTIVE  ");
        else
            printf("One push: INACTIVE  ");
    }
    if (f->auto_active) 
    {
        printf("AUTO  ");
    } else {
        printf("MANUAL ");
    }
    if (fid!=FEATURE_TRIGGER) 
    {
        printf("min: %d max %d\n",f->min,f->max);
    }
    if (fid!=FEATURE_TRIGGER && fid!=FEATURE_WHITE_BALANCE && 
        fid!=FEATURE_TEMPERATURE) 
    {
        printf("\tcurrent value is: %d\n",f->value);
    }
    if (fid==FEATURE_TRIGGER)
        printf("\n");
}

/*****************************************************
dc1394_print_feature_set
displays the entire feature set stored in features
*****************************************************/
void 
dc1394_print_feature_set(dc1394_feature_set *features) 
{
    int i;
    printf("FEATURE SETTINGS\n==================================\n");
    printf("OP- one push capable\n");
    printf("RC- readout capable\n");
    printf("O/OC- on/off capable\n");
    printf("AC- auto capable\n");
    printf("MC- manual capable\n");
    printf("==================================\n");
    for (i=FEATURE_MIN;i<FEATURE_MAX;i++) 
    {
        dc1394_print_feature(&features->feature[i]);
    }
    printf("==================================\n");
}





int
dc1394_init_camera(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_INITIALIZE,
                                              ON_VALUE));
}

int
dc1394_query_supported_formats(raw1394handle_t handle, nodeid_t node,
                               quadlet_t *value)
{
    DC1394_READ_END(GetCameraControlRegister(handle, node, REG_CAMERA_V_FORMAT_INQ,
                                             value));
}

int
dc1394_query_supported_modes(raw1394handle_t handle, nodeid_t node,
                             unsigned int format, quadlet_t *value)
{

    if (format > FORMAT_MAX)
    {
        return DC1394_FAILURE;
    }

    DC1394_READ_END(GetCameraControlRegister(handle, node,
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
	return DC1394_FAILURE;
    }

    DC1394_READ_END(GetCameraControlRegister(handle, node,
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
	return DC1394_FAILURE;
    }

    DC1394_READ_END(GetCameraControlRegister(handle, node,
                                             REG_CAMERA_V_REV_INQ_BASE + (mode * 0x04U),
                                             value));
}

int
dc1394_query_csr_offset(raw1394handle_t handle, nodeid_t node, int mode,
                        quadlet_t *value)
{

    if (mode > MODE_MAX)
    {
	return DC1394_FAILURE;
    }

    DC1394_READ_END(GetCameraControlRegister(handle, node,
                                             REG_CAMERA_V_CSR_INQ_BASE + (mode * 0x04U),
                                             value));
}

int
dc1394_query_basic_functionality(raw1394handle_t handle, nodeid_t node,
                                 quadlet_t *value)
{
    DC1394_READ_END(GetCameraControlRegister(handle, node, REG_CAMERA_BASIC_FUNC_INQ,
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
	return DC1394_FAILURE;
    }
    else if (feature < FEATURE_ZOOM)
    {
	offset= REG_CAMERA_FEATURE_HI_INQ;
    }
    else
    {
	offset= REG_CAMERA_FEATURE_LO_INQ;
    }

    if (_dc1394_read_check(retval= GetCameraControlRegister(handle, node, offset, &value)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    *availability= IsFeatureBitSet(value, feature);
   
    return DC1394_SUCCESS;
}

int
dc1394_query_advanced_feature_offset(raw1394handle_t handle, nodeid_t node,
                                     quadlet_t *value)
{
    DC1394_READ_END(GetCameraControlRegister(handle, node, REG_CAMERA_ADV_FEATURE_INQ,
                                             value));
}

int
dc1394_query_feature_characteristics(raw1394handle_t handle, nodeid_t node,
                                     unsigned int feature, quadlet_t *value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return DC1394_FAILURE;
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

    DC1394_READ_END(GetCameraControlRegister(handle, node, offset + (feature * 0x04U),
                                             value));
}

int
dc1394_get_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int *framerate)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_FRAME_RATE,
                                         &value);
    *framerate= (unsigned int)((value >> 29) & 0x7UL);
    DC1394_READ_END(retval);
}

int
dc1394_set_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int framerate)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_FRAME_RATE,
                                              (quadlet_t)((framerate & 0x7UL) << 29)));
}

int
dc1394_get_video_mode(raw1394handle_t handle, nodeid_t node,
                      unsigned int *mode)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_MODE,
                                         &value);
    *mode= (unsigned int)((value >> 29) & 0x7UL);
    DC1394_READ_END(retval);
}

int
dc1394_set_video_mode(raw1394handle_t handle, nodeid_t node, unsigned int mode)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_MODE,
                                              (quadlet_t)((mode & 0x7UL) << 29)));
}

int
dc1394_get_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int *format)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_VIDEO_FORMAT,
                                         &value);
    *format= (unsigned int)((value >> 29) & 0x7UL);
    DC1394_READ_END(retval);
}

int
dc1394_set_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int format)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, 
                                              REG_CAMERA_VIDEO_FORMAT,
                                              (quadlet_t)((format & 0x7UL) << 29)));
}

int
dc1394_get_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int *channel, unsigned int *speed)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node, REG_CAMERA_ISO_DATA,
                                         &value);
    *channel= (unsigned int)((value >> 28) & 0xFUL);
    *speed= (unsigned int)((value >> 24) & 0x3UL);
    DC1394_READ_END(retval);
}

int
dc1394_set_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int channel, unsigned int speed)
{
    
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ISO_DATA,
                                              (quadlet_t)( ((channel & 0xFUL) << 28) |
                                                           ((speed & 0x3UL) << 24) )));
}

int
dc1394_camera_on(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_POWER, ON_VALUE));
}

int
dc1394_camera_off(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_POWER,
                                              OFF_VALUE));
}

int
dc1394_start_iso_transmission(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ISO_EN,
                                              ON_VALUE));
}

int
dc1394_stop_iso_transmission(raw1394handle_t handle, nodeid_t node)
{
    int retval;
    retval =SetCameraControlRegister(handle, node, REG_CAMERA_ISO_EN,
                                     OFF_VALUE);
    DC1394_WRITE_END(retval);
}

int
dc1394_set_one_shot(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                              ON_VALUE));
}

int
dc1394_unset_one_shot(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                              OFF_VALUE));
}

int
dc1394_set_multi_shot(raw1394handle_t handle, nodeid_t node,
                      unsigned int numFrames)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                              (0x40000000UL | (numFrames & 0xFFFFUL))));
}

int
dc1394_unset_multi_shot(raw1394handle_t handle, nodeid_t node)
{
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_ONE_SHOT,
                                              OFF_VALUE));
}

int
dc1394_get_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int *brightness)
{
    return GetFeatureValue(handle, node, REG_CAMERA_BRIGHTNESS, brightness);
}

int
dc1394_set_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int brightness)
{
    return SetFeatureValue(handle, node, REG_CAMERA_BRIGHTNESS, brightness);
}

int
dc1394_get_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int *exposure)
{
    return GetFeatureValue(handle, node, REG_CAMERA_EXPOSURE, exposure);
}

int
dc1394_set_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int exposure)
{
    return SetFeatureValue(handle, node, REG_CAMERA_EXPOSURE, exposure);
}

int
dc1394_get_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int *sharpness)
{
    return GetFeatureValue(handle, node, REG_CAMERA_SHARPNESS, sharpness);
}

int
dc1394_set_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int sharpness)
{
    return SetFeatureValue(handle, node, REG_CAMERA_SHARPNESS, sharpness);
}

int
dc1394_get_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int *u_b_value, unsigned int *v_r_value)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_WHITE_BALANCE, &value);

    *u_b_value= (unsigned int)((value & 0xFFF000UL) >> 12);
    *v_r_value= (unsigned int)(value & 0xFFFUL);

    DC1394_READ_END(retval);
}

int
dc1394_set_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int u_b_value, unsigned int v_r_value)
{
    int retval;
    quadlet_t curval;

    if ( _dc1394_read_check(retval= GetCameraControlRegister(handle, node,
                                                             REG_CAMERA_WHITE_BALANCE,
                                                             &curval)) !=DC1394_SUCCESS )
    {
        return DC1394_SUCCESS;
    }

    curval= (curval & 0xFF000000UL) | ( ((u_b_value & 0xFFFUL) << 12) |
                                        (v_r_value & 0xFFFUL) );
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_WHITE_BALANCE,
                                              curval));
}

int
dc1394_get_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int *hue)
{
    DC1394_READ_END(GetFeatureValue(handle, node, REG_CAMERA_HUE, hue));
}

int
dc1394_set_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int hue)
{
    return SetFeatureValue(handle, node, REG_CAMERA_HUE, hue);
}

int
dc1394_get_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int *saturation)
{
    return GetFeatureValue(handle, node, REG_CAMERA_SATURATION, saturation);
}

int
dc1394_set_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int saturation)
{
    return SetFeatureValue(handle, node, REG_CAMERA_SATURATION, saturation);
}

int
dc1394_get_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int *gamma)
{
    return GetFeatureValue(handle, node, REG_CAMERA_GAMMA, gamma);
}

int
dc1394_set_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int gamma)
{
    return SetFeatureValue(handle, node, REG_CAMERA_GAMMA, gamma);
}

int
dc1394_get_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int *shutter)
{
    return GetFeatureValue(handle, node, REG_CAMERA_SHUTTER, shutter);
}

int
dc1394_set_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int shutter)
{
    return SetFeatureValue(handle, node, REG_CAMERA_SHUTTER, shutter);
}

int
dc1394_get_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int *gain)
{
    return GetFeatureValue(handle, node, REG_CAMERA_GAIN, gain);
}

int
dc1394_set_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int gain)
{
    return SetFeatureValue(handle, node, REG_CAMERA_GAIN, gain);
}

int
dc1394_get_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int *iris)
{
    return GetFeatureValue(handle, node, REG_CAMERA_IRIS, iris);
}

int
dc1394_set_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int iris)
{
    return SetFeatureValue(handle, node, REG_CAMERA_IRIS, iris);
}

int
dc1394_get_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int *focus)
{
    return GetFeatureValue(handle, node, REG_CAMERA_FOCUS, focus);
}

int
dc1394_set_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int focus)
{
    return SetFeatureValue(handle, node, REG_CAMERA_FOCUS, focus);
}

int
dc1394_get_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int *target_temperature,
		       unsigned int *temperature)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_TEMPERATURE, &value);
    *target_temperature= (unsigned int)((value >> 12) & 0xFFF);
    *temperature= (unsigned int)(value & 0xFFFUL);
    DC1394_READ_END(retval);
}

int
dc1394_set_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int target_temperature)
{
    int retval;
    quadlet_t curval;

    if (_dc1394_read_check (retval= GetCameraControlRegister(handle, node,
                                                             REG_CAMERA_TEMPERATURE,
                                                             &curval)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    curval= (curval & 0xFF000FFFUL) | ((target_temperature & 0xFFFUL) << 12);
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_TEMPERATURE,
                                              curval));
}

int
dc1394_get_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int *mode)
{
    quadlet_t value;
    int retval= GetCameraControlRegister(handle, node,
                                         REG_CAMERA_TRIGGER_MODE, &value);

    *mode= (unsigned int)( ((value >> 16) & 0xFUL) );

    DC1394_WRITE_END(retval);
}

int
dc1394_set_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int mode)
{
    int retval;
    quadlet_t curval;

    if (_dc1394_read_check (retval= GetCameraControlRegister(handle, node,
                                                             REG_CAMERA_TRIGGER_MODE,
                                                             &curval)) !=DC1394_SUCCESS )
    {
        return DC1394_FAILURE;
    }

    curval= (curval & 0xFFF0FFFFUL) | ((mode & 0xFUL) << 16);
    DC1394_WRITE_END(SetCameraControlRegister(handle, node, REG_CAMERA_TRIGGER_MODE,
                                              curval));
}

int
dc1394_get_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int *zoom)
{
    return GetFeatureValue(handle, node, REG_CAMERA_ZOOM, zoom);
}

int
dc1394_set_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int zoom)
{
    return SetFeatureValue(handle, node, REG_CAMERA_ZOOM, zoom);
}

int
dc1394_get_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int *pan)
{
    return GetFeatureValue(handle, node, REG_CAMERA_PAN, pan);
}

int
dc1394_set_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int pan)
{
    return SetFeatureValue(handle, node, REG_CAMERA_PAN, pan);
}

int
dc1394_get_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int *tilt)
{
    return GetFeatureValue(handle, node, REG_CAMERA_TILT, tilt);
}

int
dc1394_set_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int tilt)
{
    return SetFeatureValue(handle, node, REG_CAMERA_TILT, tilt);
}

int
dc1394_get_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int *optical_filter)
{
    return GetFeatureValue(handle, node, REG_CAMERA_OPTICAL_FILTER,
                                    optical_filter);
}

int
dc1394_set_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int optical_filter)
{
    return SetFeatureValue(handle, node, REG_CAMERA_OPTICAL_FILTER,
                                     optical_filter);
}

int
dc1394_get_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int *capture_size)
{
    return GetFeatureValue(handle, node, REG_CAMERA_CAPTURE_SIZE,
                                    capture_size);
}

int
dc1394_set_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int capture_size)
{
    return SetFeatureValue(handle, node, REG_CAMERA_CAPTURE_SIZE,
                                     capture_size);
}

int
dc1394_get_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int *capture_quality)
{
    return GetFeatureValue(handle, node, REG_CAMERA_CAPTURE_QUALITY,
                                    capture_quality);
}

int
dc1394_set_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int capture_quality)
{
    return SetFeatureValue(handle, node, REG_CAMERA_CAPTURE_QUALITY,
                                     capture_quality);
}

int
dc1394_get_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int *value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return DC1394_FAILURE;
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

    return GetFeatureValue(handle, node, offset + (feature * 0x04U),
                                    value);
}

int
dc1394_set_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int value)
{
    octlet_t offset;

    if (feature > FEATURE_MAX)
    {
	return DC1394_FAILURE;
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

    return SetFeatureValue(handle, node, offset + (feature * 0x04U),
                                     value);
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
	return DC1394_FAILURE;
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

    if ( _dc1394_read_check(retval= GetCameraControlRegister(handle, node,
                                                             offset + (feature * 0x04U),
                                                             &quadval)) ==DC1394_SUCCESS )
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
}

int
dc1394_start_one_push_operation(raw1394handle_t handle, nodeid_t node,
                                unsigned int feature)
{
    int retval;
    octlet_t offset;
    quadlet_t curval;

    if ( (feature > FEATURE_MAX) || (feature == FEATURE_TRIGGER) )
    {
	return DC1394_FAILURE;
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

    if (_dc1394_read_check (retval= GetCameraControlRegister(handle, node,
                                                             offset + (feature * 0x04U),
                                                             &curval)) !=DC1394_SUCCESS)
    {
        return DC1394_FAILURE;
    }

    if (!(curval & 0x04000000UL))
    {
        curval|= 0x04000000UL;
        DC1394_WRITE_END(SetCameraControlRegister(handle, node,
                                                  offset + (feature * 0x04U), curval));
    }

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    if (_dc1394_read_check(retval= GetCameraControlRegister(handle, node,
                                                            offset + (feature * 0x04U),
                                                            &curval)) !=DC1394_SUCCESS)
    {
        return DC1394_FAILURE;
    }

    if (!(curval & 0x02000000UL))
    {
        curval|= 0x02000000UL;
        DC1394_WRITE_END(SetCameraControlRegister(handle, node,
                                                  offset + (feature * 0x04U), curval));
    }
    else if (!value && (curval & 0x02000000UL))
    {
        curval&= 0xFDFFFFFFUL;
        DC1394_WRITE_END(SetCameraControlRegister(handle, node,
                                                  offset + (feature * 0x04U), curval));
    }

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    if (_dc1394_read_check(retval= GetCameraControlRegister(handle, node,
                                                            offset + (feature * 0x04U),
                                                            &curval)) !=DC1394_SUCCESS)
    {
        return DC1394_FAILURE;
    }
    
    if (!(curval & 0x01000000UL))
    {
        curval|= 0x01000000UL;
        DC1394_WRITE_END(SetCameraControlRegister(handle, node,
						  offset + (feature * 0x04U), curval));
    } else if (!value && (curval & 0x01000000UL))
    {
        curval&= 0xFEFFFFFFUL;
        DC1394_WRITE_END(SetCameraControlRegister(handle, node,
                                                  offset + (feature * 0x04U), curval));
    }

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
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
	return DC1394_FAILURE;
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

    DC1394_READ_END(retval);
}












