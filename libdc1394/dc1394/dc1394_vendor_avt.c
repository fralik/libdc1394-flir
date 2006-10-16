/*
 * 1394-Based Digital Camera Control Library
 * Allied Vision Technologies (AVT) specific extensions
 * Copyright (C) 2005 Inria Sophia-Antipolis
 *
 * Written by Pierre MOOS <pierre.moos@gmail.com> 
 * Version : 16/02/2005 
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

#include "dc1394_vendor_avt.h"

/********************************************************/
/* Configuration Register Offsets for Advances features */
/********************************************************/
/* See the Marlin Technical Manuel for definitions of these */

#define REG_CAMERA_AVT_VERSION_INFO1  	0x010U  
#define REG_CAMERA_AVT_VERSION_INFO3  	0x018U  
#define REG_CAMERA_AVT_ADV_INQ_1  	0x040U  
#define REG_CAMERA_AVT_ADV_INQ_2  	0x044U  
#define REG_CAMERA_AVT_ADV_INQ_3  	0x048U  
#define REG_CAMERA_AVT_ADV_INQ_4  	0x04CU  
#define REG_CAMERA_AVT_MAX_RESOLUTION  	0x200U  
#define REG_CAMERA_AVT_TIMEBASE        	0x208U  
#define REG_CAMERA_AVT_EXTD_SHUTTER  	0x20CU  
#define REG_CAMERA_AVT_TEST_IMAGE      	0x210U  
#define REG_CAMERA_AVT_SEQUENCE_CTRL  	0x220U  /* except MF131x */
#define REG_CAMERA_AVT_SEQUENCE_PARAM  	0x224U  /* except MF131x */
#define REG_CAMERA_AVT_LUT_CTRL        	0x240U  
#define REG_CAMERA_AVT_LUT_MEM_CTRL  	0x244U  
#define REG_CAMERA_AVT_LUT_INFO        	0x248U  
#define REG_CAMERA_AVT_SHDG_CTRL       	0x250U  
#define REG_CAMERA_AVT_SHDG_MEM_CTRL  	0x254U  
#define REG_CAMERA_AVT_SHDG_INFO       	0x258U  
#define REG_CAMERA_AVT_DEFERRED_TRANS  	0x260U  
#define REG_CAMERA_AVT_FRAMEINFO       	0x270U  
#define REG_CAMERA_AVT_FRAMECOUNTER  	0x274U  
#define REG_CAMERA_AVT_HDR_CONTROL     	0x280U  /* MF131x only */
#define REG_CAMERA_AVT_KNEEPOINT_1  	0x284U  /* MF131x only */
#define REG_CAMERA_AVT_KNEEPOINT_2  	0x288U  /* MF131x only */
#define REG_CAMERA_AVT_KNEEPOINT_3  	0x28CU  /* MF131x only */
#define REG_CAMERA_AVT_DSNU_CONTROL  	0x290U  /* MF131B only; Firmware 2.02 */
#define REG_CAMERA_AVT_BLEMISH_CONTROL 	0x294U  /* MF131x only; Firmware 2.02 */
#define REG_CAMERA_AVT_IO_INP_CTRL1  	0x300U  
#define REG_CAMERA_AVT_IO_INP_CTRL2  	0x304U  
#define REG_CAMERA_AVT_IO_INP_CTRL3  	0x308U  /* Dolphin series only */
#define REG_CAMERA_AVT_IO_OUTP_CTRL1  	0x320U  
#define REG_CAMERA_AVT_IO_OUTP_CTRL2	0x324U  
#define REG_CAMERA_AVT_IO_OUTP_CTRL3  	0x328U  /* Dolphin series only */
#define REG_CAMERA_AVT_IO_INTENA_DELAY 	0x340U  
#define REG_CAMERA_AVT_AUTOSHUTTER_CTRL	0x360U  /* Marlin series only */
#define REG_CAMERA_AVT_AUTOSHUTTER_LO  	0x364U  /* Marlin series only */
#define REG_CAMERA_AVT_AUTOSHUTTER_HI 	0x368U  /* Marlin series only */
#define REG_CAMERA_AVT_AUTOGAIN_CTRL  	0x370U  /* Marlin series only */
#define REG_CAMERA_AVT_AUTOFNC_AOI  	0x390U  /* Marlin series only */
#define REG_CAMERA_AVT_AF_AREA_POSITION	0x394U  /* Marlin series only */
#define REG_CAMERA_AVT_AF_AREA_SIZE  	0x398U  /* Marlin series only */
#define REG_CAMERA_AVT_COLOR_CORR      	0x3A0U  /* Marlin C-type cameras only */
#define REG_CAMERA_AVT_TRIGGER_DELAY  	0x400U  
#define REG_CAMERA_AVT_MIRROR_IMAGE  	0x410U  /* Marlin series only */
#define REG_CAMERA_AVT_SOFT_RESET      	0x510U  
#define REG_CAMERA_AVT_GPDATA_INFO  	0xFFCU  
#define REG_CAMERA_AVT_GPDATA_BUFFER	0x1000U

/************************************************************************/
/* Get Version  	(Read Only)					*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_version(dc1394camera_t *camera, 
		       uint32_t *Version, uint32_t *Camera_ID, uint32_t *FPGA_Version)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve uC */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_VERSION_INFO1, &value);
  DC1394_ERR_RTN(err,"Could not get AVT version info 1");

  /* uC Version : Bits 16..31 */
  *Version =(uint32_t)(value & 0xFFFFUL );
  
  /*  Retrieve Camera ID and FPGA_Version */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_VERSION_INFO3, &value);
  DC1394_ERR_RTN(err,"Could not get AVT version info 3");
    
  /* Camera_ID : bit 0-15 */
  *Camera_ID =(uint32_t)(value >>16 );      
  
  /* FPGA_Version : bit 16-31 */
  *FPGA_Version=(uint32_t)(value & 0xFFFFUL );   

  return DC1394_SUCCESS;
  
}

/************************************************************************/
/* Get Advanced feature inquiry						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_advanced_feature_inquiry(dc1394camera_t *camera, 
					dc1394_avt_adv_feature_info_t *adv_feature)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve first group of features presence */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_ADV_INQ_1, &value);
  DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 1");
  
  adv_feature->MaxResolution=	(value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->TimeBase= 	(value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->ExtdShutter= 	(value & 0x20000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->TestImage= 	(value & 0x10000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->FrameInfo= 	(value & 0x08000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->Sequences= 	(value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->VersionInfo= 	(value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->Lookup_Tables=   (value & 0x00800000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->Shading= 	(value & 0x00400000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->DeferredTrans=   (value & 0x00200000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->HDR_Mode= 	(value & 0x00100000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->DSNU= 		(value & 0x00080000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->TriggerDelay= 	(value & 0x00040000UL) ? DC1394_TRUE : DC1394_FALSE;    
  adv_feature->GP_Buffer= 	(value & 0x00000001UL) ? DC1394_TRUE : DC1394_FALSE;        
       
  /* Remember this request have been done */ 
  adv_feature->features_requested = DC1394_TRUE;                     

  /* Retrieve second group of features presence */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_ADV_INQ_2, &value);
  DC1394_ERR_RTN(err,"Could not get AVT advanced features INQ 2");
  
  adv_feature->Input_1 = 	(value & 0x80000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->Input_2 = 	(value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
  adv_feature->Output_1= 	(value & 0x00800000UL) ? DC1394_TRUE : DC1394_FALSE;  
  adv_feature->Output_2= 	(value & 0x00400000UL) ? DC1394_TRUE : DC1394_FALSE;        
  adv_feature->IntEnaDelay= 	(value & 0x00008000UL) ? DC1394_TRUE : DC1394_FALSE;     
 
  return DC1394_SUCCESS;
  
}

/************************************************************************/
/* Print Advanced features 						*/
/************************************************************************/
dc1394error_t
dc1394_avt_print_advanced_feature(dc1394_avt_adv_feature_info_t *adv_feature)
{

  puts ("ADVANCED FEATURES SUPPORTED:");
  if(adv_feature->MaxResolution == DC1394_TRUE) puts (" MaxResolution ");
  if(adv_feature->TimeBase == DC1394_TRUE) 	puts (" TimeBase ");
  if(adv_feature->ExtdShutter == DC1394_TRUE) 	puts (" ExtdShutter ");
  if(adv_feature->TestImage == DC1394_TRUE) 	puts (" TestImage ");
  if(adv_feature->FrameInfo == DC1394_TRUE) 	puts (" FrameInfo ");
  if(adv_feature->Sequences == DC1394_TRUE) 	puts (" Sequences ");
  if(adv_feature->VersionInfo == DC1394_TRUE) 	puts (" VersionInfo ");
  if(adv_feature->Lookup_Tables == DC1394_TRUE)	puts (" Lookup_Tables ");
  if(adv_feature->Shading == DC1394_TRUE) 	puts (" Shading ");
  if(adv_feature->DeferredTrans == DC1394_TRUE) puts (" DeferredTrans ");
  if(adv_feature->HDR_Mode == DC1394_TRUE) 	puts (" HDR_Mode ");
  if(adv_feature->DSNU == DC1394_TRUE) 		puts (" DSNU ");
  if(adv_feature->TriggerDelay == DC1394_TRUE) 	puts (" TriggerDelay ");
  if(adv_feature->GP_Buffer == DC1394_TRUE) 	puts (" GP_Buffer ");
  if(adv_feature->Input_1 == DC1394_TRUE)	puts (" Input_1 ");
  if(adv_feature->Input_2 == DC1394_TRUE) 	puts (" Input_2 ");
  if(adv_feature->Output_1 == DC1394_TRUE) 	puts (" Output_1 ");
  if(adv_feature->Output_2 == DC1394_TRUE) 	puts (" Output_2 ");
  if(adv_feature->IntEnaDelay == DC1394_TRUE) 	puts (" IntEnaDelay ");
  
  return DC1394_SUCCESS;
  
}


/************************************************************************/
/* Get shading mode							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading(dc1394camera_t *camera, 
		       dc1394bool_t *on_off, uint32_t *frame_nb)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve shading properties */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_CTRL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT shading control reg");

  /* Shading ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Number of images for auto computing of the shading reference: Bits 24..31 */
  *frame_nb =(uint32_t)((value & 0xFFUL));      

  return DC1394_SUCCESS;
  
}


/************************************************************************/
/* Set shading mode							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_shading(dc1394camera_t *camera,
		       dc1394bool_t on_off,dc1394bool_t compute, uint32_t frame_nb)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current shading properties */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_CTRL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT shading control reg");
  
  /* Shading ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25); 
  
  /* Compute : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26); 
  
  /* Number of images : Bits 24..31 */
  curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));   
  
  /* Set new parameters */    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_CTRL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT shading control reg");
 
  return DC1394_SUCCESS;
		 
}


/************************************************************************/
/* Get shading  mem ctrl						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading_mem_ctrl(dc1394camera_t *camera, dc1394bool_t *en_write, 
				dc1394bool_t *en_read, uint32_t *addroffset)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current memory shading properties */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_MEM_CTRL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT shading memory control");
  
  /* Enable write access : Bit 5 */
  *en_write = (uint32_t)((value & 0x4000000UL) >> 26); 
  
  /* Enable read access : Bit 6 */
  *en_read = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* addroffset in byte : Bits 8..31 */
  *addroffset =(uint32_t)((value & 0xFFFFFFUL));
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set shading mem ctrl							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_shading_mem_ctrl(dc1394camera_t *camera,
				dc1394bool_t en_write, dc1394bool_t en_read, uint32_t addroffset)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current shading properties */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_MEM_CTRL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT shading memory control");
  
  /* read access enable : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((en_read ) << 25); 
  
  /* write access enable : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((en_write ) << 26); 
  
  /* Number of images : Bits 8..31 */
  curval = (curval & 0xFF000000UL) | ((addroffset & 0xFFFFFFUL ));   
  
  /* Set new parameters */ 
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, curval);
  DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");
  
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Get shading  info							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_shading_info(dc1394camera_t *camera, uint32_t *MaxImageSize)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve shading info */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SHDG_INFO, &value);
  DC1394_ERR_RTN(err,"Could not get AVT shading info");

  /* Max Shading Image size(byte) : Bits 8..31 */
  *MaxImageSize =(uint32_t)((value & 0xFFFFFFUL));
       
  return DC1394_SUCCESS;

}


/************************************************************************/
/* Get Multiple slope parameters	(HDR)				*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_multiple_slope(dc1394camera_t *camera, 
			      dc1394bool_t *on_off, uint32_t *points_nb,uint32_t *kneepoint1, 
			      uint32_t *kneepoint2, uint32_t *kneepoint3)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current hdr parameters */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_HDR_CONTROL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT HDR control register");
  
  /* Multiple slope ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Number of actives points : Bits 28..31 */
  *points_nb =(uint32_t)((value & 0xFUL));
  
  /* kneepoints */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_1, kneepoint1); 
  DC1394_ERR_RTN(err,"Could not get AVT kneepoint 1");
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_2, kneepoint2);
  DC1394_ERR_RTN(err,"Could not get AVT kneepoint 2");
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_3, kneepoint3);
  DC1394_ERR_RTN(err,"Could not get AVT kneepoint 3");
      
  return DC1394_SUCCESS;
        
}


/************************************************************************/
/* Set Multiple slope parameters					*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_multiple_slope(dc1394camera_t *camera, 
			      dc1394bool_t on_off, uint32_t points_nb, uint32_t kneepoint1, 
			      uint32_t kneepoint2, uint32_t kneepoint3)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current hdr parameters */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_HDR_CONTROL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT HDR control reg");
  
  /* Shading ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25); 
  
  /* Number of points : Bits 28..31 */
  curval = (curval & 0xFFFFFFF0UL) | ((points_nb & 0xFUL ));   
  
  /* Set new hdr parameters */ 
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_HDR_CONTROL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT HDR control reg");
  
  /* kneepoints */
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_1, kneepoint1);
  DC1394_ERR_RTN(err,"Could not set AVT kneepoint 1");
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_2, kneepoint2);
  DC1394_ERR_RTN(err,"Could not set AVT kneepoint 2");   
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_KNEEPOINT_3, kneepoint3);
  DC1394_ERR_RTN(err,"Could not set AVT kneepoint 3");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Shutter Timebase 						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_timebase(dc1394camera_t *camera, uint32_t *timebase_id)
{
  dc1394error_t err;
  quadlet_t value;
        
  /* Retrieve current timebase */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TIMEBASE, &value);
  DC1394_ERR_RTN(err,"Could not get AVT timebase");

  /* Time base ID : Bits 29..31 */
  *timebase_id =(uint32_t)((value & 0xFUL));
       
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Shutter Timebase (acquisition must be stopped)			*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_timebase(dc1394camera_t *camera, uint32_t timebase_id)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current timebase */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TIMEBASE, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT timebase");
  
  curval = (curval & 0xFFFFFFF0UL) | ((timebase_id & 0xFUL ));   
  
  /* Set new timebase */     
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TIMEBASE, curval);
  DC1394_ERR_RTN(err,"Could not set AVT timebase");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Extented Shutter  						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_extented_shutter(dc1394camera_t *camera, uint32_t *timebase_id)
{
  dc1394error_t err;
  quadlet_t value;
    
  /* Retrieve current extented shutter value */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_EXTD_SHUTTER, &value);
  DC1394_ERR_RTN(err,"Could not get AVT extended shutter reg");
  
  /* Exposure Time in us: Bits 6..31 */
  *timebase_id =(uint32_t)((value & 0xFFFFFFFUL));
        
  return DC1394_SUCCESS;
  
}


/************************************************************************/
/* Set Extented shutter							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_extented_shutter(dc1394camera_t *camera, uint32_t timebase_id)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current extented shutter value */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_EXTD_SHUTTER, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT extended shutter reg");
  
  /* Time base ID : Bits 6..31 */
  curval = (curval & 0xF0000000UL) | ((timebase_id & 0x0FFFFFFFUL ));   
  
  /* Set new extented shutter value */    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_EXTD_SHUTTER, curval);
  DC1394_ERR_RTN(err,"Could not set AVT extended shutter reg");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get MaxResolution  	(Read Only)					*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_MaxResolution(dc1394camera_t *camera, uint32_t *MaxHeight, uint32_t *MaxWidth)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve the maximum resolution */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_MAX_RESOLUTION, &value);
  DC1394_ERR_RTN(err,"Could not get AVT max resolution");
  
  /* MaxHeight : Bits 0..15 */
  *MaxHeight =(uint32_t)(value >> 16);
  /* MaxWidth : Bits 16..31 */
  *MaxWidth =(uint32_t)(value & 0xFFFFUL );      
       
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Auto Shutter  							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_auto_shutter(dc1394camera_t *camera, uint32_t *MinValue, uint32_t *MaxValue)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current min auto shutter value */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOSHUTTER_LO, &value);
  DC1394_ERR_RTN(err,"Could not get AVT autoshutter LSB");
  
  *MinValue =(uint32_t)value;
  
  /* Retrieve current max auto shutter value */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOSHUTTER_HI, &value); 
  DC1394_ERR_RTN(err,"Could not get AVT autoshutter MSB");
  
  *MaxValue =(uint32_t)value;
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Auto shutter							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_auto_shutter(dc1394camera_t *camera, uint32_t MinValue, uint32_t MaxValue)
{
  dc1394error_t err;
  /* Set min auto shutter value */    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOSHUTTER_LO, MinValue);
  DC1394_ERR_RTN(err,"Could not set AVT autoshutter LSB");

  /* Set max auto shutter value */  
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOSHUTTER_HI, MaxValue); 
  DC1394_ERR_RTN(err,"Could not set AVT autoshutter MSB");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Auto Gain  							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_auto_gain(dc1394camera_t *camera, uint32_t *MinValue, uint32_t *MaxValue)
{
  dc1394error_t err;
  quadlet_t value;
    
  /* Retrieve auto gain values */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOGAIN_CTRL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT autogain");
  
  /* Min : bits 20..31 */
  *MinValue =(uint32_t)(value & 0xFFFUL);
  /* Max : bits 4..15 */
  *MaxValue =(uint32_t)((value >> 16) & 0xFFFUL);
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Auto gain							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_auto_gain(dc1394camera_t *camera, uint32_t MinValue, uint32_t MaxValue)
{
  dc1394error_t err;
  quadlet_t value;    
  
  /* Max : bits 4..15, Min : bits 20..31  */
  value = ( MaxValue <<16 ) | ( MinValue );
  
  /* Set new parameters */    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOGAIN_CTRL,value);
  DC1394_ERR_RTN(err,"Could not set AVT autogain");

  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Trigger delay							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_trigger_delay(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *DelayTime)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve trigger delay */    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TRIGGER_DELAY, &value);
  DC1394_ERR_RTN(err,"Could not get AVT trigger delay");
  
  /* trigger_delay ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Delai time in us : Bits 11..31 */
  *DelayTime =(uint32_t)((value & 0xFFFFFUL));

  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Trigger delay							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_trigger_delay(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t DelayTime)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve trigger delay */       
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TRIGGER_DELAY, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT trigger delay");
  
  /* trigger_delay ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25); 
  
  /* Delay time in us : Bits 11..31 */
  curval = (curval & 0xFFF00000UL) | DelayTime;   
  
  /* Set new parameters */     
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TRIGGER_DELAY, curval);
  DC1394_ERR_RTN(err,"Could not set AVT trigger delay");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Mirror 								*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_mirror(dc1394camera_t *camera, dc1394bool_t *on_off)
{
  dc1394error_t err;
  quadlet_t value;

  /* Retrieve Mirror mode */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_MIRROR_IMAGE, &value);
  DC1394_ERR_RTN(err,"Could not get AVT mirror image");
  
  /* mirror ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
       
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Set Mirror								*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_mirror(dc1394camera_t *camera, dc1394bool_t on_off)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* ON / OFF : Bit 6 */
  curval = on_off << 25; 
  
  /* Set mirror mode */     
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_MIRROR_IMAGE, curval);
  DC1394_ERR_RTN(err,"Could not set AVT mirror image");
  
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Get DSNU 								*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_dsnu(dc1394camera_t *camera, dc1394bool_t *on_off,uint32_t *frame_nb)
{
  dc1394error_t err;
  quadlet_t value;
  /* Retrieve dsnu parameters */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DSNU_CONTROL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
  
  /* ON / OFF : Bit 6 */
  *on_off = !(uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Number of images : Bits 24..31 */
  *frame_nb =(uint32_t)((value & 0xFFUL));
    
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Set DSNU								*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_dsnu(dc1394camera_t *camera,
		    dc1394bool_t on_off, dc1394bool_t compute, uint32_t frame_nb)
{
  dc1394error_t err;
  quadlet_t curval;

  /* Retrieve current dsnu parameters */            
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DSNU_CONTROL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
  
  /* Compute : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26); 
  
  /* ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((!on_off ) << 25); 
  
  /* Number of images : Bits 24..31 */
  curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));   
  
  /* Set new dsnu parameters */        
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DSNU_CONTROL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT DSNU control");

  int cont=1;
  while (cont) {
    usleep(50000);
    err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DSNU_CONTROL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
    if ((curval & 0x01000000UL)==0)
      cont=0;
  }
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Get BLEMISH 								*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_blemish(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *frame_nb)
{
  dc1394error_t err;
  quadlet_t value;
  
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT blemish control");
  
  /* ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Number of images : Bits 24..31 */
  *frame_nb =(uint32_t)((value & 0xFFUL));
    
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Set BLEMISH								*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_blemish(dc1394camera_t *camera,
		       dc1394bool_t on_off, dc1394bool_t compute, uint32_t frame_nb)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current blemish parameters */        
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT blemish control");
  
  /* Compute : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((compute ) << 26); 
  
  /* ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25); 
  
  /* Number of images : Bits 24..31 */
  curval = (curval & 0xFFFFFF00UL) | ((frame_nb & 0xFFUL ));   
  
  /* Set new blemish parameters */         
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT blemish control");
  
  int cont=1;
  while (cont) {
    usleep(50000);
    err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_BLEMISH_CONTROL, &curval);
    DC1394_ERR_RTN(err,"Could not get AVT DSNU control");
    if ((curval & 0x01000000UL)==0)
      cont=0;
  }

  return DC1394_SUCCESS;
}



/************************************************************************/
/* Get IO	REG_CAMERA_AVT_IO_INP_CTRLx	or REG_CAMERA_AVT_IO_OUTP_CTRLx	*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_io(dc1394camera_t *camera, uint32_t IO,
		  dc1394bool_t *polarity, uint32_t *mode, dc1394bool_t *pinstate)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve IO parameters */        
  err=GetCameraAdvControlRegister(camera,IO, &value);
  DC1394_ERR_RTN(err,"Could not get AVT IO register");
 
  /* polarity : Bit 7 */
  *polarity = (uint32_t)((value & 0x1000000UL) >> 24); 
  
  /* pinstate : Bit 31 */
  *pinstate = (uint32_t)((value & 0x1UL)); 
  
  /* mode : Bits 11..15 */
  *mode =(uint32_t)((value >> 16 ) & 0x1FUL);
         
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Set IO	REG_CAMERA_AVT_IO_INP_CTRLx	or REG_CAMERA_AVT_IO_OUTP_CTRLx	*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_io(dc1394camera_t *camera,uint32_t IO,
		  dc1394bool_t polarity, uint32_t mode)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current IO parameters */            
  err=GetCameraAdvControlRegister(camera,IO, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT IO register");
  
  /* polarity : Bit 7 */
  curval = (curval & 0xFEFFFFFFUL) | ((polarity ) << 24); 
  
  /* mode : Bits 11..15 */
  curval = (curval & 0xFFE0FFFFUL) | ((mode << 16) & 0x1F0000UL );   
  
  /* Set  new IO parameters */            
  err=SetCameraAdvControlRegister(camera,IO, curval);
  DC1394_ERR_RTN(err,"Could not set AVT IO register");
  
  return DC1394_SUCCESS;
}

/************************************************************************/
/* BusReset IEEE1394							*/
/************************************************************************/
dc1394error_t
dc1394_avt_reset(dc1394camera_t *camera)
{
  dc1394error_t err;
  quadlet_t value;
  /* ON / OFF : Bit 6 */
  value= (1<<25) + 200; /*2sec*/
  /* Reset */                    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_SOFT_RESET,value); 
  DC1394_ERR_RTN(err,"Could not set AVT soft reset");

  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get Lookup Tables (LUT)						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut(dc1394camera_t *camera, dc1394bool_t *on_off, uint32_t *lutnb)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current luts parameters */            
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_CTRL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT LUT control");
  
  /* Shading ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Number of lut : Bits 26..31 */
  *lutnb =(uint32_t)((value & 0x3FUL));
       
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Lookup Tables (LUT)						*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_lut(dc1394camera_t *camera, dc1394bool_t on_off, uint32_t lutnb)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current luts parameters */                
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_CTRL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT LUT control");
  
  /* Shading ON / OFF : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((on_off ) << 25); 
  
  /* Number of lut : Bits 26..31 */
  curval = (curval & 0xFFFFFFB0UL) | ((lutnb & 0x3FUL ));   
  
  /* Set new luts parameters */            
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_CTRL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT LUT control");

  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get LUT ctrl								*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut_mem_ctrl(dc1394camera_t *camera, dc1394bool_t *en_write, 
			    uint32_t * AccessLutNo,uint32_t *addroffset)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current memory luts parameters */                
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, &value);
  DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");
 
  /* Enable write access : Bit 5 */
  *en_write = (uint32_t)((value & 0x4000000UL) >> 26); 
  
  /* AccessLutNo : Bits 8..15 */
  *AccessLutNo=(uint32_t)((value >> 16) & 0xFFUL);
  
  /* addroffset in byte : Bits 16..31 */
  *addroffset =(uint32_t)((value & 0xFFFFUL));

  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set LUT ctrl					         		*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_lut_mem_ctrl(dc1394camera_t *camera,
			    dc1394bool_t en_write, uint32_t AccessLutNo, uint32_t addroffset)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current memory luts parameters */                    
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT LUT memory control");
  
  /* write access enable : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((en_write ) << 26); 
  
  /* AccessLutNo : Bits 8..15 */
  curval = (curval & 0xFF00FFFFUL) | ((AccessLutNo << 16) & 0xFF0000UL );   
  
  /* Number of images : Bits 16..31 */
  curval = (curval & 0xFFFF0000UL) | ((addroffset & 0xFFFFUL ));   
  
  /* Set new parameters */     
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_MEM_CTRL, curval);
  DC1394_ERR_RTN(err,"Could not set AVT LUT memory control");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get LUT  info							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_lut_info(dc1394camera_t *camera, uint32_t *NumOfLuts, uint32_t *MaxLutSize)
{
  dc1394error_t err;
  quadlet_t value;
  /* Retrieve luts info */                
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_LUT_INFO, &value);
  DC1394_ERR_RTN(err,"Could not get AVT LUT info");

  /* NumOfLuts : Bits 8..15 */
  *NumOfLuts=(uint32_t)((value >> 16) & 0xFFUL);
  
  /* MaxLutSize : Bits 16..31 */
  *MaxLutSize =(uint32_t)((value & 0xFFFFUL));

  return DC1394_SUCCESS;
}



/************************************************************************/
/* Get Automatic white balance	with Area Of Interest AOI		*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_aoi(dc1394camera_t *camera, 
		   dc1394bool_t *on_off, int *left, int *top, int *width, int *height)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve current mode*/                
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOFNC_AOI, &value);
  DC1394_ERR_RTN(err,"Could not get AVT autofocus AOI");
  
  /*  ON / OFF : Bit 6 */
  *on_off = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* Retrieve current size of area*/                      
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AF_AREA_SIZE, &value);
  DC1394_ERR_RTN(err,"Could not get AVT AF area size");
    
  /* width : Bits 0..15 */
  *width =(uint32_t)(value >> 16);
  /* height : Bits 16..31 */
  *height =(uint32_t)(value & 0xFFFFUL );
  
  /* Retrieve current position of area*/                      	
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AF_AREA_POSITION, &value);
  DC1394_ERR_RTN(err,"Could not get AVT AF area position");

  /* left : Bits 0..15 */
  *left =(uint32_t)(value >> 16);
  /* top : Bits 16..31 */
  *top =(uint32_t)(value & 0xFFFFUL );

  return DC1394_SUCCESS;
}

/************************************************************************/
/* Set Automatic white balance with Area Of Interest AOI		*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_aoi(dc1394camera_t *camera,
		   dc1394bool_t on_off,int left, int top, int width, int height)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* ON / OFF : Bit 6 */
  curval = on_off << 25; 
  
  /* Set feature on off */
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AUTOFNC_AOI, curval);
  DC1394_ERR_RTN(err,"Could not set AVT autofocus AOI");
  
  /* Set size */
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AF_AREA_SIZE, (width << 16) | height);
  DC1394_ERR_RTN(err,"Could not set AVT AF area size");
  
  /* Set position */  
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_AF_AREA_POSITION,(left << 16) | top );
  DC1394_ERR_RTN(err,"Could not set AVT AF area position");
  
  return DC1394_SUCCESS;
}

/************************************************************************/
/* Get test_images							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_test_images(dc1394camera_t *camera, uint32_t *image_no)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve test image number */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TEST_IMAGE, &value);
  DC1394_ERR_RTN(err,"Could not get AVT test image");
  
  /* Numero Image : Bits 28..31 */
  *image_no =(uint32_t)((value & 0xFUL));    
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set test_images							*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_test_images(dc1394camera_t *camera, uint32_t image_no)
{
  dc1394error_t err;
  quadlet_t curval;

  /* Retrieve current test image */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TEST_IMAGE, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT test image");
  
  /* Numero Image : Bits 28..31 */
  curval = (curval & 0xFFFFFFF0UL) | ((image_no & 0xFUL ));   
  
  /* Set new test image */
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_TEST_IMAGE,curval);
  DC1394_ERR_RTN(err,"Could not set AVT test image");
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get frame info							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_frame_info(dc1394camera_t *camera, uint32_t *framecounter)
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve frame info */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_FRAMECOUNTER, &value);
  DC1394_ERR_RTN(err,"Could not get AVT framecounter");
  
  /* framecounter : Bits 0..31 */
  *framecounter =(uint32_t)(value);    
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Reset frame info							*/
/************************************************************************/
dc1394error_t
dc1394_avt_reset_frame_info(dc1394camera_t *camera)
{
  dc1394error_t err;
  /* Reset counter */
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_FRAMEINFO,1 << 30);
  DC1394_ERR_RTN(err,"Could not get AVT frame info");

  return DC1394_SUCCESS;
}

/************************************************************************/
/* Get Deferred image transport						*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_deferred_trans(dc1394camera_t *camera, 
			      dc1394bool_t *HoldImage, dc1394bool_t * FastCapture,
			      uint32_t *FifoSize, uint32_t *NumOfImages )
{
  dc1394error_t err;
  quadlet_t value;
  
  /* Retrieve Deferred image transport mode */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DEFERRED_TRANS, &value);
  DC1394_ERR_RTN(err,"Could not get AVT deferred transfer info");
  
  /* enable/disable deferred transport mode : Bit 6 */
  *HoldImage = (uint32_t)((value & 0x2000000UL) >> 25); 
  
  /* enable/disable fast capture mode (format 7 only) : Bit 7 */
  *FastCapture = (uint32_t)((value & 0x1000000UL) >> 24); 
  
  /* Size of fifo in number of image : Bits 16..23 */
  *FifoSize =(uint32_t)((value >> 8 & 0xFFUL));
  
  /* Number of images in buffer: Bits 24..31 */
  *NumOfImages =(uint32_t)((value & 0xFFUL));
       
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Set Deferred image transport						*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_deferred_trans(dc1394camera_t *camera,
			      dc1394bool_t HoldImage, dc1394bool_t FastCapture,
			      uint32_t FifoSize, uint32_t NumOfImages,
			      dc1394bool_t SendImage)
{
  dc1394error_t err;
  quadlet_t curval;
  
  /* Retrieve current image transport mode */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DEFERRED_TRANS, &curval);
  DC1394_ERR_RTN(err,"Could not get AVT deferred transfer info");
  
  /* Send NumOfImages now : Bit 5 */
  curval = (curval & 0xFBFFFFFFUL) | ((SendImage ) << 26); 
  
  /* enable/disable deferred transport mode : Bit 6 */
  curval = (curval & 0xFDFFFFFFUL) | ((HoldImage ) << 25); 
  
  /* enable/disable fast capture mode (format 7 only) : Bit 7 */       
  curval = (curval & 0xFEFFFFFFUL) | ((FastCapture ) << 24);  
  
  /* Size of fifo in number of image : Bits 16..23 */
  curval = (curval & 0xFFFF00FFUL) | (((FifoSize << 8) & 0xFF00UL ));   
  
  /* Number of images : Bits 24..31 */
  curval = (curval & 0xFFFFFF00UL) | ((NumOfImages & 0xFFUL ));   
  
  /* Set new parameters */    
  err=SetCameraAdvControlRegister(camera,REG_CAMERA_AVT_DEFERRED_TRANS, curval);
  DC1394_ERR_RTN(err,"Could not set AVT deferred transfer info");

  return DC1394_SUCCESS;
}



/************************************************************************/
/* Get GPData info							*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_gpdata_info(dc1394camera_t *camera, uint32_t *BufferSize)
{
  dc1394error_t err;
  quadlet_t value;
  /* Retrieve info on the general purpose buffer */
  err=GetCameraAdvControlRegister(camera,REG_CAMERA_AVT_GPDATA_INFO, &value);
  DC1394_ERR_RTN(err,"Could not get AVT GP data info");
  
  /* BufferSize : Bits 16..31 */
  *BufferSize =(uint32_t)((value & 0xFFFFUL));    
  
  return DC1394_SUCCESS;
}


/************************************************************************/
/* Get pdata_buffer : experimental, does not work			*/
/************************************************************************/
dc1394error_t
dc1394_avt_get_pdata_buffer(dc1394camera_t *camera, uint32_t *buff)
{
  return DC1394_FAILURE ;       
}


/************************************************************************/
/* Set pdata_buffer	experimental, does not work			*/
/************************************************************************/
dc1394error_t
dc1394_avt_set_pdata_buffer(dc1394camera_t *camera, uint32_t buff)
{
  return DC1394_FAILURE;
}


