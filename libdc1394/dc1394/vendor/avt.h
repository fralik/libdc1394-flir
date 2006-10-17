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
#ifndef __DC1394_VENDOR_AVT_H__
#define __DC1394_VENDOR_AVT_H__

#include <dc1394/control.h>
#include <dc1394/register.h>

typedef struct __dc1394_avt_adv_feature_info_struct 
{
  uint32_t feature_id;
  dc1394bool_t features_requested;
  dc1394bool_t MaxResolution;
  dc1394bool_t TimeBase;
  dc1394bool_t ExtdShutter;
  dc1394bool_t TestImage;
  dc1394bool_t FrameInfo;
  dc1394bool_t Sequences;
  dc1394bool_t VersionInfo;
  dc1394bool_t Lookup_Tables;
  dc1394bool_t Shading;
  dc1394bool_t DeferredTrans;
  dc1394bool_t HDR_Mode;
  dc1394bool_t DSNU;
  dc1394bool_t TriggerDelay;    
  dc1394bool_t GP_Buffer;
  dc1394bool_t Input_1;    
  dc1394bool_t Input_2;        
  dc1394bool_t Output_1;    
  dc1394bool_t Output_2;        
  dc1394bool_t IntEnaDelay;        
  
} dc1394_avt_adv_feature_info_t;


#ifdef __cplusplus
extern "C" {
#endif
/************************************************************************/
/* Get Version  	(Read Only)					*/
/*----------------------------------------------------------------------*/
/* Retrieve the firmware version, FPGA version and the camera ID	*/	 
/************************************************************************/
dc1394error_t dc1394_avt_get_version(dc1394camera_t *camera, 
				     uint32_t *Version, uint32_t *Camera_ID,
				     uint32_t *FPGA_Version);



/************************************************************************/
/* Get Advanced feature inquiry						*/
/*----------------------------------------------------------------------*/
/* Retrieve the supported features					*/	 
/************************************************************************/
dc1394error_t dc1394_avt_get_advanced_feature_inquiry(dc1394camera_t *camera,
						      dc1394_avt_adv_feature_info_t *adv_feature);


/************************************************************************/
/* Print Advanced feature 						*/
/*----------------------------------------------------------------------*/
/* Print the supported features requested 				*/	 
/************************************************************************/
dc1394error_t dc1394_avt_print_advanced_feature(dc1394_avt_adv_feature_info_t *adv_feature);


/************************************************************************/
/* Get the shading mode							*/
/*----------------------------------------------------------------------*/
/* Retrieve if shading is on and the number of frames used to compute 	*/	 
/* The shading reference frame						*/
/************************************************************************/
int dc1394_avt_get_shading(dc1394camera_t *camera, 
			   dc1394bool_t *on_off, uint32_t *frame_nb);


/************************************************************************/
/* Set the shading mode							*/
/*----------------------------------------------------------------------*/
/* Set the shading to on/off and the number of frames used to compute 	*/	 
/* The shading reference frame						*/
/************************************************************************/
int dc1394_avt_set_shading(dc1394camera_t *camera,
			   dc1394bool_t on_off, dc1394bool_t compute,
			   uint32_t frame_nb);
		

		
/************************************************************************/
/* Get shading  mem ctrl						*/
/*----------------------------------------------------------------------*/
/* Retrieve write and read access mode of the shading reference frame	*/
/************************************************************************/
dc1394error_t dc1394_avt_get_shading_mem_ctrl(dc1394camera_t *camera, 
					      dc1394bool_t *en_write,
					      dc1394bool_t *en_read, 
					      uint32_t *addroffset);


/************************************************************************/
/* Set shading mem ctrl							*/
/*----------------------------------------------------------------------*/
/* Set write and read access mode of the shading reference frame	*/
/************************************************************************/
dc1394error_t dc1394_avt_set_shading_mem_ctrl(dc1394camera_t *camera,
					      dc1394bool_t en_write, 
					      dc1394bool_t en_read, 
					      uint32_t addroffset);


/************************************************************************/
/* Get shading  info							*/
/*----------------------------------------------------------------------*/
/* Retrieve the max size of a shading image				*/
/************************************************************************/
dc1394error_t dc1394_avt_get_shading_info(dc1394camera_t *camera, 
					  uint32_t *MaxImageSize);



/************************************************************************/
/* Get Multiple slope parameters					*/
/*----------------------------------------------------------------------*/
/* Retrieve if on/off, the nb of kneepoints used and the 		*/
/* kneepoints values							*/
/************************************************************************/
dc1394error_t dc1394_avt_get_multiple_slope(dc1394camera_t *camera, 
					    dc1394bool_t *on_off,
					    uint32_t *points_nb, 
					    uint32_t *kneepoint1,
					    uint32_t *kneepoint2, 
					    uint32_t *kneepoint3);


/************************************************************************/
/* Set Multiple slope parameters					*/
/*----------------------------------------------------------------------*/
/* Set on/off, the nb of kneepoints to use and the 			*/
/* kneepoints values							*/
/************************************************************************/
dc1394error_t dc1394_avt_set_multiple_slope(dc1394camera_t *camera, 
					    dc1394bool_t on_off,
					    uint32_t points_nb, 
					    uint32_t kneepoint1,
					    uint32_t kneepoint2, 
					    uint32_t kneepoint3);



/************************************************************************/
/* Get Shutter Timebase 						*/
/*----------------------------------------------------------------------*/
/* Get the timebase value with an Id. See Manual for correspondance	*/
/************************************************************************/
dc1394error_t dc1394_avt_get_timebase(dc1394camera_t *camera, 
				      uint32_t *timebase_id);


/************************************************************************/
/* Set Shutter Timebase 						*/
/*----------------------------------------------------------------------*/
/* Set the timebase value with an Id. See Manual for correspondance	*/
/************************************************************************/
dc1394error_t dc1394_avt_set_timebase(dc1394camera_t *camera,
				      uint32_t timebase_id);



/************************************************************************/
/* Get Extented Shutter  						*/
/*----------------------------------------------------------------------*/
/* Get the extented shutter value in us					*/
/************************************************************************/
dc1394error_t dc1394_avt_get_extented_shutter(dc1394camera_t *camera, 
					      uint32_t *timebase_id);


/************************************************************************/
/* Set Extented shutter							*/
/*----------------------------------------------------------------------*/
/* Set the extented shutter value in us					*/
/************************************************************************/
dc1394error_t dc1394_avt_set_extented_shutter(dc1394camera_t *camera, 
					      uint32_t timebase_id);



/************************************************************************/
/* Get MaxResolution  	(Read Only)					*/
/*----------------------------------------------------------------------*/
/* Get the Max reachable resolution 					*/
/************************************************************************/
dc1394error_t dc1394_avt_get_MaxResolution(dc1394camera_t *camera, 
					   uint32_t *MaxHeight,
					   uint32_t *MaxWidth);



/************************************************************************/
/* Get Auto Shutter  							*/
/*----------------------------------------------------------------------*/
/* Get min and max shutter values for autoshutter			*/
/************************************************************************/
dc1394error_t dc1394_avt_get_auto_shutter(dc1394camera_t *camera, 
					  uint32_t *MinValue,
					  uint32_t *MaxValue);


/************************************************************************/
/* Set Auto shutter							*/
/*----------------------------------------------------------------------*/
/* Set min and max shutter values for autoshutter			*/
/************************************************************************/
dc1394error_t dc1394_avt_set_auto_shutter(dc1394camera_t *camera,
					  uint32_t MinValue,
					  uint32_t MaxValue);



/************************************************************************/
/* Get Auto gain							*/
/*----------------------------------------------------------------------*/
/* Get min and max gain values for autogain				*/
/************************************************************************/
dc1394error_t dc1394_avt_get_auto_gain(dc1394camera_t *camera, 
				       uint32_t *MinValue,
				       uint32_t *MaxValue);


/************************************************************************/
/* Set Auto gain							*/
/*----------------------------------------------------------------------*/
/* Set min and max gain values for autogain				*/
/************************************************************************/
dc1394error_t dc1394_avt_set_auto_gain(dc1394camera_t *camera,
				       uint32_t MinValue,
				       uint32_t MaxValue);



/************************************************************************/
/* Get Trigger delay							*/
/*----------------------------------------------------------------------*/
/* Get if trigger delay on and the trigger delay			*/
/************************************************************************/
dc1394error_t dc1394_avt_get_trigger_delay(dc1394camera_t *camera, 
					   dc1394bool_t *on_off,
					   uint32_t *DelayTime);
		

/************************************************************************/
/* Set Trigger delay							*/
/*----------------------------------------------------------------------*/
/* Set trigger delay on/off  and the trigger delay value		*/
/************************************************************************/
dc1394error_t dc1394_avt_set_trigger_delay(dc1394camera_t *camera,
					   dc1394bool_t on_off,
					   uint32_t DelayTime);



/************************************************************************/		
/* Get Mirror 								*/
/*----------------------------------------------------------------------*/
/* Get mirror mode							*/
/************************************************************************/
dc1394error_t dc1394_avt_get_mirror(dc1394camera_t *camera, 
				    dc1394bool_t *on_off);


/************************************************************************/
/* Set Mirror								*/
/*----------------------------------------------------------------------*/
/* Set mirror mode							*/
/************************************************************************/
dc1394error_t dc1394_avt_set_mirror(dc1394camera_t *camera,
				    dc1394bool_t on_off);



/************************************************************************/
/* Get DSNU 								*/
/*----------------------------------------------------------------------*/
/* Get DSNU mode and num of frames used for computing  dsnu correction  */
/************************************************************************/
dc1394error_t dc1394_avt_get_dsnu(dc1394camera_t *camera, 
				  dc1394bool_t *on_off,
				  uint32_t *frame_nb);


/************************************************************************/
/* Set DSNU								*/
/*----------------------------------------------------------------------*/
/* Set DSNU mode, number of frames used for computing 			*/
/*  and launch the the computation of the dsnu frame			*/
/************************************************************************/
dc1394error_t dc1394_avt_set_dsnu(dc1394camera_t *camera,
				  dc1394bool_t on_off, dc1394bool_t compute,
				  uint32_t frame_nb);

		

/************************************************************************/
/* Get BLEMISH 								*/
/*----------------------------------------------------------------------*/
/* Get Blemish mode and num of frames used for computing the correction */
/************************************************************************/
dc1394error_t dc1394_avt_get_blemish(dc1394camera_t *camera, 
				     dc1394bool_t *on_off, uint32_t *frame_nb);
		
		
/************************************************************************/
/* Set BLEMISH								*/
/*----------------------------------------------------------------------*/
/* Set Blemish mode, num of frames used for computing 			*/
/*  and launch the the computation of the blemish correction		*/
/************************************************************************/
dc1394error_t dc1394_avt_set_blemish(dc1394camera_t *camera,
				     dc1394bool_t on_off, dc1394bool_t compute,
				     uint32_t frame_nb);



/************************************************************************/
/* Get IO	REG_CAMERA_IO_INP_CTRLx	or REG_CAMERA_IO_OUTP_CTRLx	*/
/*----------------------------------------------------------------------*/
/*  Get the polarity, the mode, the state of the IO			*/
/************************************************************************/
dc1394error_t dc1394_avt_get_io(dc1394camera_t *camera, uint32_t IO,
				dc1394bool_t *polarity, uint32_t *mode,
				dc1394bool_t *pinstate);


/************************************************************************/
/* Set IO	REG_CAMERA_IO_INP_CTRLx	or REG_CAMERA_IO_OUTP_CTRLx	*/
/*----------------------------------------------------------------------*/
/*  Set the polarity and the mode of the IO				*/
/************************************************************************/
dc1394error_t dc1394_avt_set_io(dc1394camera_t *camera,uint32_t IO,
				dc1394bool_t polarity, uint32_t mode);
		


/************************************************************************/
/* BusReset IEEE1394							*/
/*----------------------------------------------------------------------*/
/* Reset the bus and the fpga						*/
/************************************************************************/
dc1394error_t dc1394_avt_reset(dc1394camera_t *camera);



/************************************************************************/
/* Get Lookup Tables (LUT)						*/
/*----------------------------------------------------------------------*/
/* Get on/off and the num of the current lut loaded			*/
/************************************************************************/
dc1394error_t dc1394_avt_get_lut(dc1394camera_t *camera, 
				 dc1394bool_t *on_off, uint32_t *lutnb  );
		

/************************************************************************/
/* Set Lookup Tables (LUT)						*/
/*----------------------------------------------------------------------*/
/* Set on/off and the num of the current lut to load			*/
/************************************************************************/
dc1394error_t dc1394_avt_set_lut(dc1394camera_t *camera,
				 dc1394bool_t on_off, uint32_t lutnb);


/************************************************************************/
/* Get LUT ctrl								*/
/*----------------------------------------------------------------------*/
/* Get access mode of a lut						*/
/************************************************************************/
dc1394error_t dc1394_avt_get_lut_mem_ctrl(dc1394camera_t *camera, 
					  dc1394bool_t *en_write,
					  uint32_t * AccessLutNo,
					  uint32_t *addroffset);


/************************************************************************/
/* Set LUT ctrl								*/
/*----------------------------------------------------------------------*/
/* Set access mode of a lut						*/
/************************************************************************/

dc1394error_t dc1394_avt_set_lut_mem_ctrl(dc1394camera_t *camera,
					  dc1394bool_t en_write,
					  uint32_t AccessLutNo, 
					  uint32_t addroffset);
	
		
/************************************************************************/
/* Get LUT  info							*/
/*----------------------------------------------------------------------*/
/* Get num of luts present and the max size				*/
/************************************************************************/
dc1394error_t dc1394_avt_get_lut_info(dc1394camera_t *camera, 
				      uint32_t *NumOfLuts, uint32_t *MaxLutSize);


/************************************************************************/
/* Get Automatic white balance with Area Of Interest AOI		*/
/*----------------------------------------------------------------------*/
/* Get on/off and area							*/
/************************************************************************/

dc1394error_t dc1394_avt_get_aoi(dc1394camera_t *camera, 
				 dc1394bool_t *on_off, int *left, int *top, 
				 int *width, int *height);

/************************************************************************/
/* Set Automatic white balance with Area Of Interest AOI		*/
/*----------------------------------------------------------------------*/
/* Set on/off and area							*/
/************************************************************************/
dc1394error_t dc1394_avt_set_aoi(dc1394camera_t *camera,
				 dc1394bool_t on_off,int left, int top,
				 int width, int height);



/************************************************************************/
/* Get test_images							*/
/*----------------------------------------------------------------------*/
/* Get current test image						*/
/************************************************************************/
dc1394error_t dc1394_avt_get_test_images(dc1394camera_t *camera, 
					 uint32_t *image_no);


/************************************************************************/
/* Set test_images							*/
/*----------------------------------------------------------------------*/
/* Set num of test image						*/
/************************************************************************/
dc1394error_t dc1394_avt_set_test_images(dc1394camera_t *camera, 
					 uint32_t image_no);



/************************************************************************/
/* Get frame info							*/
/*----------------------------------------------------------------------*/
/* Get the number of captured frames					*/
/************************************************************************/
dc1394error_t dc1394_avt_get_frame_info(dc1394camera_t *camera, 
					uint32_t *framecounter);


/************************************************************************/
/* Reset frame info 							*/
/*----------------------------------------------------------------------*/
/* Reset frame counter							*/
/************************************************************************/
dc1394error_t dc1394_avt_reset_frame_info(dc1394camera_t *camera);
		


/************************************************************************/
/* Get GPData info							*/
/*----------------------------------------------------------------------*/
/* Get the size of the buffer						*/
/************************************************************************/
dc1394error_t dc1394_avt_get_gpdata_info(dc1394camera_t *camera, 
					 uint32_t *BufferSize);		



/************************************************************************/
/* Get Deferred image transport						*/
/*----------------------------------------------------------------------*/
/* Get the fifo control mode						*/
/************************************************************************/
dc1394error_t dc1394_avt_get_deferred_trans(dc1394camera_t *camera, 
					    dc1394bool_t *HoldImage,
					    dc1394bool_t * FastCapture, 
					    uint32_t *FifoSize,
					    uint32_t *NumOfImages );
		
		
/************************************************************************/
/* Set Deferred image transport						*/
/*----------------------------------------------------------------------*/
/* Set the fifo control mode						*/
/************************************************************************/
dc1394error_t dc1394_avt_set_deferred_trans(dc1394camera_t *camera,
					    dc1394bool_t HoldImage,
					    dc1394bool_t  FastCapture, 
					    uint32_t FifoSize,
					    uint32_t NumOfImages, 
					    dc1394bool_t SendImage );



/************************************************************************/
/* Get pdata_buffer : DOESNT WORK					*/
/*----------------------------------------------------------------------*/
/* will Get the  buffer	...						*/
/************************************************************************/
dc1394error_t dc1394_avt_get_pdata_buffer(dc1394camera_t *camera, 
					  uint32_t *buff);


/************************************************************************/
/* Set pdata_buffer : DOESNT WORK					*/
/*----------------------------------------------------------------------*/
/* will Set the  buffer	...						*/
/************************************************************************/
dc1394error_t dc1394_avt_set_pdata_buffer(dc1394camera_t *camera, 
					  uint32_t buff);


#ifdef __cplusplus
}
#endif

#endif
