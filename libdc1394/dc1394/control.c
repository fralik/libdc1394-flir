/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
 * Additions by Chris Urmson <curmson@ri.cmu.edu>
 * Additions by Damien Douxchamps <ddouxchamps@users.sf.net>
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

#include "control.h"
#include "config.h"
#include "internal.h"
#include "register.h"
#include "offsets.h"
#include "utils.h"
 

dc1394error_t
dc1394_camera_set_broadcast(dc1394camera_t *camera, dc1394bool_t pwr)
{
  if (pwr==DC1394_TRUE) {
    if (camera->broadcast==DC1394_FALSE) {
      camera->node_id_backup=camera->node;
      camera->node=63;
      camera->broadcast=DC1394_TRUE;
    }
  }
  else if (pwr==DC1394_FALSE) {
    if (camera->broadcast==DC1394_TRUE) {
      camera->node=camera->node_id_backup;
      camera->broadcast=DC1394_FALSE;
    }
  }
  else
    return DC1394_INVALID_ARGUMENT_VALUE;

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_find_cameras(dc1394camera_t ***cameras_ptr, uint32_t* numCameras)
{
  return dc1394_find_cameras_platform(cameras_ptr, numCameras);

}

int
_dc1394_get_iidc_version(dc1394camera_t *camera)
{
  dc1394error_t err=DC1394_SUCCESS;
  uint32_t quadval = 0; // to avoid valgrind errors
  uint64_t offset;

  if (camera == NULL)
    return DC1394_CAMERA_NOT_INITIALIZED;

  /*
     Note on Point Grey (PG) cameras:
     Although not always advertised, PG cameras are 'sometimes' compatible
     with IIDC specs. This is especially the case with PG stereo products.
     The following modifications have been tested with a stereo head
     (BumbleBee). Most other cameras should be compatible, please consider
     contributing to the lib if your PG camera is not recognized.
     
     PG cams sometimes have a Unit_Spec_ID of 0xB09D, instead of the
     0xA02D of classic IIDC cameras. Also, their software revision differs.
     I could only get a 1.14 version from my BumbleBee but other versions
     might exist.

     As PG is regularly providing firmware updates you might also install
     the latest one in your camera for an increased compatibility.
     
     Damien

     (updated 2005-04-30)
  */
  
  if ( (camera->ud_reg_tag_12 != 0x000A02DUL) &&
       (camera->ud_reg_tag_12 != 0x000B09DUL) ) {
    camera->iidc_version=-1;
    return DC1394_NOT_A_CAMERA;
  }
  
  switch (camera->ud_reg_tag_13&0xFFFFFFUL) {
  case 0x000100:
    camera->iidc_version=DC1394_IIDC_VERSION_1_04;
    break;
  case 0x000101:
    camera->iidc_version=DC1394_IIDC_VERSION_1_20;
    break;
  case 0x000102:
    camera->iidc_version=DC1394_IIDC_VERSION_1_30;
    break;
  case 0x000114:
  case 0x800002:
    if (camera->ud_reg_tag_12 == 0x000B09DUL)
      camera->iidc_version=DC1394_IIDC_VERSION_PTGREY;
    else
      camera->iidc_version = -1; // an error should be sent in this case
    break;
  default:
    return DC1394_INVALID_IIDC_VERSION;
  }

  /* IIDC 1.31 check */
  if (camera->iidc_version==DC1394_IIDC_VERSION_1_30) {
    
    /* -- get the unit_dependent_directory offset --
       We can do it here since we know it's a camera, and thus the UDD should exists
       This UDD will be re-checked after but it does not really matter. */

    offset= camera->unit_directory;
    err=GetConfigROMTaggedRegister(camera, 0xD4, &offset, &quadval);
    DC1394_ERR_RTN(err, "Could not get unit dependent directory");
    camera->unit_dependent_directory=(quadval & 0xFFFFFFUL)*4+offset;
  
    //fprintf(stderr,"1.30 detected\n");
    offset=camera->unit_dependent_directory;
    err=GetConfigROMTaggedRegister(camera, 0x38, &offset, &quadval);
    if (err!=DC1394_SUCCESS) {
      if (err==DC1394_TAGGED_REGISTER_NOT_FOUND) {
	//fprintf(stderr,"not tag reg\n");
	// If it fails here we return success with the most accurate version estimation: 1.30.
	// This is because the GetConfigROMTaggedRegister will return a failure both if there is a comm
	// problem but also if the tag is not found. In the latter case it simply means that the
	// camera version is 1.30
	camera->iidc_version=DC1394_IIDC_VERSION_1_30;
	return DC1394_SUCCESS;
      }
      else 
	DC1394_ERR_RTN(err, "Could not get tagged register 0x38");
    }
    //fprintf(stderr,"test2\n");

    switch (quadval&0xFFFFFFUL) {
    case 0x10:
      camera->iidc_version=DC1394_IIDC_VERSION_1_31;
      break;
    case 0x20:
      camera->iidc_version=DC1394_IIDC_VERSION_1_32;
      break;
    case 0x30:
      camera->iidc_version=DC1394_IIDC_VERSION_1_33;
      break;
    case 0x40:
      camera->iidc_version=DC1394_IIDC_VERSION_1_34;
      break;
    case 0x50:
      camera->iidc_version=DC1394_IIDC_VERSION_1_35;
      break;
    case 0x60:
      camera->iidc_version=DC1394_IIDC_VERSION_1_36;
      break;
    case 0x70:
      camera->iidc_version=DC1394_IIDC_VERSION_1_37;
      break;
    case 0x80:
      camera->iidc_version=DC1394_IIDC_VERSION_1_38;
      break;
    case 0x90:
      camera->iidc_version=DC1394_IIDC_VERSION_1_39;
      break;
    // if we arrived here without errors we suppose that the camera is at least 1.30.
    // we don't flag the camera as non compliant.
    default:
      camera->iidc_version=DC1394_IIDC_VERSION_1_30;
      break;
    }
  }
  //fprintf(stderr,"test\n");
  return err;
}

dc1394error_t
dc1394_reset_bus (dc1394camera_t * camera)
{
  return dc1394_reset_bus_platform (camera);
}

dc1394error_t
dc1394_update_camera_info(dc1394camera_t *camera)
{
  dc1394error_t err;
  uint32_t len, i, count;
  uint64_t offset;
  uint32_t value[2], quadval = 0; // set to zero to avoid valgrind errors
  memset(value,0,2*sizeof(value)); // set to zero to avoid valgrind errors

  // init pointers to zero:
  camera->command_registers_base=0;
  camera->unit_directory=0;
  camera->unit_dependent_directory=0;
  camera->advanced_features_csr=0;
  camera->PIO_control_csr=0;
  camera->SIO_control_csr=0;
  camera->strobe_control_csr=0;

  for (i=0;i<DC1394_VIDEO_MODE_FORMAT7_NUM;i++)
    camera->format7_csr[i]=0;

  // return silently on all errors as a bad rom just means a device that is not a camera
  
  /* get the unit_directory offset */
  offset= ROM_ROOT_DIRECTORY;
  err=GetConfigROMTaggedRegister(camera, 0xD1, &offset, &quadval);
  if (err!=DC1394_SUCCESS) return err;
  camera->unit_directory=(quadval & 0xFFFFFFUL)*4+offset;

  /* get the vendor_id*/
  offset= ROM_ROOT_DIRECTORY;
  err=GetConfigROMTaggedRegister(camera, 0x03, &offset, &quadval);
  if (err==DC1394_SUCCESS) {
    camera->vendor_id=quadval & 0xFFFFFFUL;
  }
  else if (err==DC1394_TAGGED_REGISTER_NOT_FOUND) {
    camera->vendor_id=0;
  }
  else {
    DC1394_ERR_RTN(err, "Could not get vendor ID");
    return err;
  }

  /* get the spec_id value */
  offset=camera->unit_directory;
  err=GetConfigROMTaggedRegister(camera, 0x12, &offset, &quadval);
  if (err!=DC1394_SUCCESS) return err;
  camera->ud_reg_tag_12=quadval&0xFFFFFFUL;

  /* get the iidc revision */
  offset=camera->unit_directory;
  err=GetConfigROMTaggedRegister(camera, 0x13, &offset, &quadval);
  if (err!=DC1394_SUCCESS) return err;
  camera->ud_reg_tag_13=quadval&0xFFFFFFUL;

  /* verify the version/revision and find the IIDC_REVISION value from that */
  /* Note: this requires the UDD to be set in order to verify IIDC 1.31 compliance. */
  err=_dc1394_get_iidc_version(camera);
  if (err==DC1394_NOT_A_CAMERA)
    return err;
  DC1394_ERR_RTN(err, "Problem inferring the IIDC version");

  /* get the model_id*/
  offset= camera->unit_directory;
  err=GetConfigROMTaggedRegister(camera, 0x17, &offset, &quadval);
  if (err==DC1394_SUCCESS) {
    camera->model_id=quadval & 0xFFFFFFUL;
  }
  else if (err==DC1394_TAGGED_REGISTER_NOT_FOUND) {
    camera->model_id=0;
  }
  else {
    DC1394_ERR_RTN(err, "Could not get model ID");
    return err;
  }

  /* get the unit_dependent_directory offset */
  offset= camera->unit_directory;
  err=GetConfigROMTaggedRegister(camera, 0xD4, &offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get unit dependent directory");
  camera->unit_dependent_directory=(quadval & 0xFFFFFFUL)*4+offset;
  
  // at this point we know it's a camera so we start returning errors if registers
  // are not found

  /* now get the EUID-64 */
  err=GetCameraROMValue(camera, ROM_BUS_INFO_BLOCK+0x0C, &value[0]);
  if (err!=DC1394_SUCCESS) return err;
  err=GetCameraROMValue(camera, ROM_BUS_INFO_BLOCK+0x10, &value[1]);
  if (err!=DC1394_SUCCESS) return err;
  camera->euid_64= ((uint64_t)value[0] << 32) | (uint64_t)value[1];
  
  /* now get the command_regs_base */
  offset= camera->unit_dependent_directory;
  err=GetConfigROMTaggedRegister(camera, 0x40, &offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get commands base address");
  camera->command_registers_base= (uint64_t)(quadval & 0xFFFFFFUL)*4;

  /* get the vendor_name_leaf offset (optional) */
  offset= camera->unit_dependent_directory;
  camera->vendor[0] = '\0';
  err=GetConfigROMTaggedRegister(camera, 0x81, &offset, &quadval);
  if (err==DC1394_SUCCESS) {
    offset=(quadval & 0xFFFFFFUL)*4+offset;
    
    /* read in the length of the vendor name */
    err=GetCameraROMValue(camera, offset, &value[0]);
    DC1394_ERR_RTN(err, "Could not get vendor leaf length");
    
    len= (uint32_t)(value[0] >> 16)*4-8; /* Tim Evers corrected length value */ 
    
    if (len > MAX_CHARS) {
      len= MAX_CHARS;
    }
    offset+= 12;
    count= 0;

    /* grab the vendor name */
    while (len > 0) {
      err=GetCameraROMValue(camera, offset+count, &value[0]);
      DC1394_ERR_RTN(err, "Could not get vendor string character");
      
      camera->vendor[count++]= (value[0] >> 24);
      camera->vendor[count++]= (value[0] >> 16) & 0xFFUL;
      camera->vendor[count++]= (value[0] >> 8) & 0xFFUL;
      camera->vendor[count++]= value[0] & 0xFFUL;
      len-= 4;
      
      camera->vendor[count]= '\0';
    }
  }
  /* if the tagged register is not found, don't make a fuss about it. */
  else if (err!=DC1394_TAGGED_REGISTER_NOT_FOUND) {
    DC1394_ERR_RTN(err, "Could not get vendor leaf offset");
  }

  /* get the model_name_leaf offset (optional) */
  offset= camera->unit_dependent_directory;
  camera->model[0] = '\0';
  err=GetConfigROMTaggedRegister(camera, 0x82, &offset, &quadval);

  if (err==DC1394_SUCCESS) {
    offset=(quadval & 0xFFFFFFUL)*4+offset;
    
    /* read in the length of the model name */
    err=GetCameraROMValue(camera, offset, &value[0]);
    DC1394_ERR_RTN(err, "Could not get model name leaf length");
    
    len= (uint32_t)(value[0] >> 16)*4-8; /* Tim Evers corrected length value */ 
    
    if (len > MAX_CHARS) {
      len= MAX_CHARS;
    }
    offset+= 12;
    count= 0;
    
    /* grab the model name */
    while (len > 0) {
      err=GetCameraROMValue(camera, offset+count, &value[0]);
      DC1394_ERR_RTN(err, "Could not get model name character");
      
      camera->model[count++]= (value[0] >> 24);
      camera->model[count++]= (value[0] >> 16) & 0xFFUL;
      camera->model[count++]= (value[0] >> 8) & 0xFFUL;
      camera->model[count++]= value[0] & 0xFFUL;
      len-= 4;
      
      camera->model[count]= '\0';
    }
  }
  /* if the tagged register is not found, don't make a fuss about it. */
  else if (err!=DC1394_TAGGED_REGISTER_NOT_FOUND) {
    DC1394_ERR_RTN(err, "Could not get model name leaf offset");
  }

  err=dc1394_video_get_iso_speed(camera, &camera->iso_speed);
  DC1394_ERR_RTN(err, "Could not get ISO channel and speed");
  
  err=dc1394_video_get_mode(camera, &camera->video_mode);
  dc1394video_modes_t modes;
  if (err==DC1394_INVALID_VIDEO_FORMAT) {
    // a proper video mode may not be present. Try to set a default video mode
    err=dc1394_video_get_supported_modes(camera,&modes);
    DC1394_ERR_RTN(err,"Could not get the list of supported video modes");

    err=dc1394_video_set_mode(camera,modes.modes[0]);
    DC1394_ERR_RTN(err,"Could not set a default video mode (%d)",modes.modes[0]);
    
    err=dc1394_video_get_mode(camera,&camera->video_mode);
    DC1394_ERR_RTN(err,"Could not get current video mode. Giving up.");
  }

  err=dc1394_video_get_framerate(camera, &camera->framerate);
  DC1394_ERR_RTN(err, "Could not get current video framerate");
  
  err=dc1394_video_get_transmission(camera, &camera->is_iso_on);
  DC1394_ERR_RTN(err, "Could not get ISO status");
  
  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, &value[0]);
  DC1394_ERR_RTN(err, "Could not get basic functionalities");

  camera->mem_channel_number = (value[0] & 0x0000000F);
  camera->bmode_capable      = (value[0] & 0x00800000) != 0;
  camera->one_shot_capable   = (value[0] & 0x00000800) != 0;
  camera->multi_shot_capable = (value[0] & 0x00001000) != 0;
  int adv_features_capable = (value[0] & 0x80000000) != 0;
  camera->can_switch_on_off  = (value[0] & (0x1<<16)) != 0;

  if (camera->iidc_version>=DC1394_IIDC_VERSION_1_30) {
    err=GetCameraControlRegister(camera, REG_CAMERA_OPT_FUNC_INQ, &value[0]);
    DC1394_ERR_RTN(err, "Could not get opt functionalities");
    
    if (value[0] & 0x40000000) {
      // get PIO CSR
      err=GetCameraControlRegister(camera,REG_CAMERA_PIO_CONTROL_CSR_INQ, &quadval);
      DC1394_ERR_RTN(err, "Could not get PIO control CSR");
      camera->PIO_control_csr= (uint64_t)(quadval & 0xFFFFFFUL)*4;
    }
    if (value[0] & 0x20000000) {
      // get PIO CSR
      err=GetCameraControlRegister(camera,REG_CAMERA_SIO_CONTROL_CSR_INQ, &quadval);
      DC1394_ERR_RTN(err, "Could not get SIO control CSR");
      camera->SIO_control_csr= (uint64_t)(quadval & 0xFFFFFFUL)*4;
    }
    if (value[0] & 0x10000000) {
      // get PIO CSR
      err=GetCameraControlRegister(camera,REG_CAMERA_STROBE_CONTROL_CSR_INQ, &quadval);
      DC1394_ERR_RTN(err, "Could not get strobe control CSR");
      camera->strobe_control_csr= (uint64_t)(quadval & 0xFFFFFFUL)*4;
    }
  }

  if (adv_features_capable>0) {
    // get advanced features CSR
    err=GetCameraControlRegister(camera,REG_CAMERA_ADV_FEATURE_INQ, &quadval);
    DC1394_ERR_RTN(err, "Could not get advanced features CSR");
    camera->advanced_features_csr= (uint64_t)(quadval & 0xFFFFFFUL)*4;
  }

  return err;
}

void
dc1394_free_camera(dc1394camera_t *camera)
{
  if (camera == NULL)
    return;
  dc1394_free_camera_platform (camera);
}

dc1394error_t
dc1394_print_camera_info(dc1394camera_t *camera) 
{
  uint32_t value[2];
  
  value[0]= camera->euid_64 & 0xffffffff;
  value[1]= (camera->euid_64 >>32) & 0xffffffff;
  printf("------ Camera information ------\n");
  printf("Vendor                            :     %s\n", camera->vendor);
  printf("Model                             :     %s\n", camera->model);
  printf("Node                              :     0x%x\n", camera->node);
  printf("Port                              :     %d\n", camera->port);
  printf("Specifications ID                 :     0x%x\n", camera->ud_reg_tag_12);
  printf("Software revision                 :     0x%x\n", camera->ud_reg_tag_13);
  printf("IIDC version code                 :     %d\n", camera->iidc_version);
  printf("Unit directory offset             :     0x%llx\n", (uint64_t)camera->unit_directory);
  printf("Unit dependent directory offset   :     0x%llx\n", (uint64_t)camera->unit_dependent_directory);
  printf("Commands registers base           :     0x%llx\n", (uint64_t)camera->command_registers_base);
  printf("Unique ID                         :     0x%08x%08x\n", value[1], value[0]);
  if (camera->advanced_features_csr>0)
    printf("Advanced features found at offset :     0x%llx\n", (uint64_t)camera->advanced_features_csr);
  printf("1394b mode capable (>=800Mbit/s)  :     ");
  if (camera->bmode_capable==DC1394_TRUE)
    printf("Yes\n");
  else
    printf("No\n");

  dc1394_print_camera_info_platform (camera);

  return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_get_camera_feature_set

 Collects the available features for the camera
 described by node and stores them in features.
*****************************************************/
dc1394error_t
dc1394_get_camera_feature_set(dc1394camera_t *camera, dc1394featureset_t *features) 
{
  uint32_t i, j;
  dc1394error_t err=DC1394_SUCCESS;
  
  for (i= DC1394_FEATURE_MIN, j= 0; i <= DC1394_FEATURE_MAX; i++, j++)  {
    features->feature[j].id= i;
    err=dc1394_get_camera_feature(camera, &features->feature[j]);
    DC1394_ERR_RTN(err, "Could not get camera feature %d",i);
  }

  return err;
}

/*****************************************************
 dc1394_get_camera_feature

 Stores the bounds and options associated with the
 feature described by feature->id
*****************************************************/
dc1394error_t
dc1394_get_camera_feature(dc1394camera_t *camera, dc1394feature_info_t *feature) 
{
  uint64_t offset;
  uint32_t value;
  dc1394error_t err;

  // check presence
  err=dc1394_feature_is_present(camera, feature->id, &(feature->available));
  DC1394_ERR_RTN(err, "Could not check feature %d presence",feature->id);
  
  if (feature->available == DC1394_FALSE) {
    return DC1394_SUCCESS;
  }

  // get capabilities
  FEATURE_TO_INQUIRY_OFFSET(feature->id, offset);
  err=GetCameraControlRegister(camera, offset, &value);
  DC1394_ERR_RTN(err, "Could not check feature %d characteristics",feature->id);

  switch (feature->id) {
  case DC1394_FEATURE_TRIGGER:
    feature->one_push= DC1394_FALSE;
    feature->polarity_capable= (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
    int i;
    uint32_t value_tmp;
    
    feature->trigger_modes.num=0;
    value_tmp= (value & (0xFF));
    //fprintf(stderr,"value: %x\n",value_tmp);
    
    for (i=DC1394_TRIGGER_MODE_MIN;i<=DC1394_TRIGGER_MODE_MAX;i++) {
      if (value_tmp & (0x1 << (15-(i-DC1394_TRIGGER_MODE_MIN)-(i>5)*8))) { // (i>5)*8 to take the mode gap into account
	feature->trigger_modes.modes[feature->trigger_modes.num]=i;
	feature->trigger_modes.num++;
      }
    }

    err=dc1394_external_trigger_get_supported_sources(camera,&feature->trigger_sources);
    DC1394_ERR_RTN(err, "Could not get supported trigger sources");

    feature->auto_capable= DC1394_FALSE;
    feature->manual_capable= DC1394_FALSE;
    break;
  default:
    feature->polarity_capable = 0;
    feature->trigger_mode     = 0;
    feature->one_push         = (value & 0x10000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->auto_capable     = (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->manual_capable   = (value & 0x01000000UL) ? DC1394_TRUE : DC1394_FALSE;
    
    feature->min= (value & 0xFFF000UL) >> 12;
    feature->max= (value & 0xFFFUL);
    break;
  }
  
  feature->absolute_capable = (value & 0x40000000UL) ? DC1394_TRUE : DC1394_FALSE;
  feature->readout_capable  = (value & 0x08000000UL) ? DC1394_TRUE : DC1394_FALSE;
  feature->on_off_capable   = (value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;
  
  // get current values
  FEATURE_TO_VALUE_OFFSET(feature->id, offset);
  
  err=GetCameraControlRegister(camera, offset, &value);
  DC1394_ERR_RTN(err, "Could not get feature register");
  
  switch (feature->id) {
  case DC1394_FEATURE_TRIGGER:
    feature->one_push_active= DC1394_FALSE;
    feature->trigger_polarity=
      (value & 0x01000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->trigger_mode= (uint32_t)((value >> 14) & 0xF);
    feature->auto_active= DC1394_FALSE;
    break;
  case DC1394_FEATURE_TRIGGER_DELAY:
    feature->one_push_active= DC1394_FALSE;
    feature->auto_active=DC1394_FALSE;
  default:
    feature->one_push_active=
      (value & 0x04000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->auto_active=
      (value & 0x01000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->trigger_polarity= DC1394_FALSE;
    break;
  }
  
  feature->is_on= (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
  
  switch (feature->id) {
  case DC1394_FEATURE_WHITE_BALANCE:
    feature->RV_value= value & 0xFFFUL;
    feature->BU_value= (value & 0xFFF000UL) >> 12;
    break;
  case DC1394_FEATURE_WHITE_SHADING:
    feature->R_value=value & 0xFFUL;
    feature->G_value=(value & 0xFF00UL)>>8;
    feature->B_value=(value & 0xFF0000UL)>>16;
    break;
  case DC1394_FEATURE_TEMPERATURE:
    feature->value= value & 0xFFFUL;
    feature->target_value= value & 0xFFF000UL;
    break;
  default:
    feature->value= value & 0xFFFUL;
    break;
  }
  
  if (feature->absolute_capable>0) {
    err=dc1394_feature_get_absolute_boundaries(camera, feature->id, &feature->abs_min, &feature->abs_max);
    DC1394_ERR_RTN(err, "Could not get feature %d absolute min/max",feature->id);
    err=dc1394_feature_get_absolute_value(camera, feature->id, &feature->abs_value);
    DC1394_ERR_RTN(err, "Could not get feature %d absolute value",feature->id);
    err=dc1394_feature_get_absolute_control(camera, feature->id, &feature->abs_control);
    DC1394_ERR_RTN(err, "Could not get feature %d absolute control",feature->id);
  }
  
  return err;
}

/*****************************************************
 dc1394_print_feature

 Displays the bounds and options of the given feature
*****************************************************/
dc1394error_t
dc1394_print_feature(dc1394feature_info_t *f) 
{
  int fid= f->id;
  
  if ( (fid < DC1394_FEATURE_MIN) || (fid > DC1394_FEATURE_MAX) ) {
    printf("Invalid feature code\n");
    return DC1394_INVALID_FEATURE;
  }
  
  printf("%s:\n\t", dc1394_feature_desc[fid - DC1394_FEATURE_MIN]);
  
  if (!f->available) {
    printf("NOT AVAILABLE\n");
    return DC1394_SUCCESS;
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
  if (f->absolute_capable)
    printf("ABS  ");
  printf("\n");
  
  if (f->on_off_capable) {
    if (f->is_on) 
      printf("\tFeature: ON  ");
    else
      printf("\tFeature: OFF  ");
  }
  else
    printf("\t");

  if (f->one_push)  {
    if (f->one_push_active)
      printf("One push: ACTIVE  ");
    else
      printf("One push: INACTIVE  ");
  }
  
  if (f->auto_active) 
    printf("AUTO  ");
  else
    printf("MANUAL ");

  if (fid != DC1394_FEATURE_TRIGGER) 
    printf("min: %d max %d\n", f->min, f->max);

  switch(fid) {
  case DC1394_FEATURE_TRIGGER:
    printf("\n\tAvailableTriggerModes: ");
    if (f->trigger_modes.num==0) {
      printf("none");
    }
    else {
      int i;
      for (i=0;i<f->trigger_modes.num;i++) {
	printf("%d ",f->trigger_modes.modes[i]);
      }
    }
    printf("\n\tAvailableTriggerSources: ");
    if (f->trigger_sources.num==0) {
      printf("none");
    }
    else {
      int i;
      for (i=0;i<f->trigger_sources.num;i++) {
	printf("%d ",f->trigger_sources.sources[i]);
      }
    }
    printf("\n\tPolarity Change Capable: ");
    
    if (f->polarity_capable) 
      printf("True");
    else 
      printf("False");
    
    printf("\n\tCurrent Polarity: ");
    
    if (f->trigger_polarity) 
      printf("POS");
    else 
      printf("NEG");
    
    printf("\n\tcurrent mode: %d\n", f->trigger_mode);
    if (f->trigger_sources.num>0) {
      printf("\n\tcurrent source: %d\n", f->trigger_source);
    }
    break;
  case DC1394_FEATURE_WHITE_BALANCE: 
    printf("\tB/U value: %d R/V value: %d\n", f->BU_value, f->RV_value);
    break;
  case DC1394_FEATURE_TEMPERATURE:
    printf("\tTarget temp: %d Current Temp: %d\n", f->target_value, f->value);
    break;
  case DC1394_FEATURE_WHITE_SHADING: 
    printf("\tR value: %d G value: %d B value: %d\n", f->R_value,
	   f->G_value, f->B_value);
    break;
  default:
    printf("\tcurrent value is: %d\n",f->value);
    break;
  }
  if (f->absolute_capable)
    printf("\tabsolute settings:\n\t value: %f\n\t min: %f\n\t max: %f\n",
	   f->abs_value,f->abs_min,f->abs_max);

  return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_print_feature_set

 Displays the entire feature set stored in features
*****************************************************/
dc1394error_t
dc1394_print_feature_set(dc1394featureset_t *features) 
{
  uint32_t i, j;
  dc1394error_t err=DC1394_SUCCESS;
  
  printf("------ Features report ------\n");
  printf("OP   - one push capable\n");
  printf("RC   - readout capable\n");
  printf("O/OC - on/off capable\n");
  printf("AC   - auto capable\n");
  printf("MC   - manual capable\n");
  printf("ABS  - absolute capable\n");
  printf("-----------------------------\n");
  
  for (i= DC1394_FEATURE_MIN, j= 0; i <= DC1394_FEATURE_MAX; i++, j++)  {
    err=dc1394_print_feature(&features->feature[j]);
    DC1394_ERR_RTN(err, "Could not print feature %d",i);
  }
  
  return err;
}

dc1394error_t
dc1394_reset_camera(dc1394camera_t *camera)
{
  dc1394error_t err;
  err=SetCameraControlRegister(camera, REG_CAMERA_INITIALIZE, DC1394_FEATURE_ON);
  DC1394_ERR_RTN(err, "Could not reset the camera");
  return err;
}

dc1394error_t
dc1394_video_get_supported_modes(dc1394camera_t *camera, dc1394video_modes_t *modes)
{
  dc1394error_t err;
  uint32_t value, sup_formats;
  dc1394video_mode_t mode;

  // get supported formats
  err=GetCameraControlRegister(camera, REG_CAMERA_V_FORMAT_INQ, &sup_formats);
  DC1394_ERR_RTN(err, "Could not get supported formats");

  //fprintf(stderr,"supoerted formats: 0x%x\n",sup_formats);

  // for each format check supported modes and add them as we find them.

  modes->num=0;
  // Format_0
  //fprintf(stderr,"shiftval=%d, mask=0x%x\n",31-(FORMAT0-FORMAT_MIN),0x1 << (31-(FORMAT0-FORMAT_MIN)));
  if ((sup_formats & (0x1 << (31-(DC1394_FORMAT0-DC1394_FORMAT_MIN)))) > 0) {
    //fprintf(stderr,"F0 available\n");
    err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT0-DC1394_FORMAT_MIN) * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported modes for Format_0");
    
    //fprintf(stderr,"modes 0: 0x%x\n",value);
    for (mode=DC1394_VIDEO_MODE_FORMAT0_MIN;mode<=DC1394_VIDEO_MODE_FORMAT0_MAX;mode++) {
      //fprintf(stderr,"mask=0x%x, cond=0x%x\n", 0x1 << (31-(mode-MODE_FORMAT0_MIN)),(value && (0x1<<(31-(mode-MODE_FORMAT0_MIN)))));
      if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT0_MIN)))) > 0) {
	modes->modes[modes->num]=mode;
	modes->num++;
      }
    }
  }
  // Format_1
  if ((sup_formats & (0x1 << (31-(DC1394_FORMAT1-DC1394_FORMAT_MIN)))) > 0) {
    err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT1-DC1394_FORMAT_MIN) * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported modes for Format_1");
    
    for (mode=DC1394_VIDEO_MODE_FORMAT1_MIN;mode<=DC1394_VIDEO_MODE_FORMAT1_MAX;mode++) {
      if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT1_MIN)))) > 0) {
	modes->modes[modes->num]=mode;
	modes->num++;
      }
    }
  }
  // Format_2
  if ((sup_formats & (0x1 << (31-(DC1394_FORMAT2-DC1394_FORMAT_MIN)))) > 0) {
    err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT2-DC1394_FORMAT_MIN) * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported modes for Format_2");
    
    for (mode=DC1394_VIDEO_MODE_FORMAT2_MIN;mode<=DC1394_VIDEO_MODE_FORMAT2_MAX;mode++) {
      if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT2_MIN)))) > 0) {
	modes->modes[modes->num]=mode;
	modes->num++;
      }
    }
  }
  // Format_6
  if ((sup_formats & (0x1 << (31-(DC1394_FORMAT6-DC1394_FORMAT_MIN)))) > 0) {
    err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT6-DC1394_FORMAT_MIN) * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported modes for Format_3");
    
    for (mode=DC1394_VIDEO_MODE_FORMAT6_MIN;mode<=DC1394_VIDEO_MODE_FORMAT6_MAX;mode++) {
      if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT6_MIN))))>0) {
	modes->modes[modes->num]=mode;
	modes->num++;
      }
    }
  }
  // Format_7
  if ((sup_formats & (0x1 << (31-(DC1394_FORMAT7-DC1394_FORMAT_MIN)))) > 0) {
    err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + ((DC1394_FORMAT7-DC1394_FORMAT_MIN) * 0x04U), &value);
    DC1394_ERR_RTN(err, "Could not get supported modes for Format_4");
    
    for (mode=DC1394_VIDEO_MODE_FORMAT7_MIN;mode<=DC1394_VIDEO_MODE_FORMAT7_MAX;mode++) {
      if ((value & (0x1<<(31-(mode-DC1394_VIDEO_MODE_FORMAT7_MIN))))>0) {
	modes->modes[modes->num]=mode;
	modes->num++;
      }
    }
  }

  return err;
}

dc1394error_t
dc1394_video_get_supported_framerates(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394framerates_t *framerates)
{
  dc1394framerate_t framerate;
  dc1394error_t err;
  uint32_t format;
  uint32_t value;

  err=_dc1394_get_format_from_mode(video_mode, &format);
  DC1394_ERR_RTN(err, "Invalid mode code");

  if ((format==DC1394_FORMAT6)||(format==DC1394_FORMAT7)) {
    err=DC1394_INVALID_VIDEO_FORMAT;
    DC1394_ERR_RTN(err, "Modes corresponding for format6 and format7 do not have framerates!"); 
  }

  switch (format) {
  case DC1394_FORMAT0:
    video_mode-=DC1394_VIDEO_MODE_FORMAT0_MIN;
    break;
  case DC1394_FORMAT1:
    video_mode-=DC1394_VIDEO_MODE_FORMAT1_MIN;
    break;
  case DC1394_FORMAT2:
    video_mode-=DC1394_VIDEO_MODE_FORMAT2_MIN;
    break;
  }
  format-=DC1394_FORMAT_MIN;
  

  err=GetCameraControlRegister(camera,REG_CAMERA_V_RATE_INQ_BASE + (format * 0x20U) + (video_mode * 0x04U), &value);
  DC1394_ERR_RTN(err, "Could not get supported framerates");

  framerates->num=0;
  for (framerate=DC1394_FRAMERATE_MIN;framerate<=DC1394_FRAMERATE_MAX;framerate++) {
    if ((value & (0x1<<(31-(framerate-DC1394_FRAMERATE_MIN))))>0) {
      framerates->framerates[framerates->num]=framerate;
      framerates->num++;
    }
  }

  return err;
}


dc1394error_t
dc1394_video_get_framerate(dc1394camera_t *camera, dc1394framerate_t *framerate)
{
  uint32_t value;
  dc1394error_t err;

  err=GetCameraControlRegister(camera, REG_CAMERA_FRAME_RATE, &value);
  DC1394_ERR_RTN(err, "Could not get video framerate");
  
  *framerate= (uint32_t)((value >> 29) & 0x7UL) + DC1394_FRAMERATE_MIN;
  
  return err;
}

dc1394error_t
dc1394_video_set_framerate(dc1394camera_t *camera, dc1394framerate_t framerate)
{ 
  dc1394error_t err;
  if ( (framerate < DC1394_FRAMERATE_MIN) || (framerate > DC1394_FRAMERATE_MAX) ) {
    return DC1394_INVALID_FRAMERATE;
  }

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  err=SetCameraControlRegister(camera, REG_CAMERA_FRAME_RATE,
			       (uint32_t)(((framerate - DC1394_FRAMERATE_MIN) & 0x7UL) << 29));
  DC1394_ERR_RTN(err, "Could not set video framerate");

  return err;
}

dc1394error_t
dc1394_video_get_mode(dc1394camera_t *camera, dc1394video_mode_t *mode)
{
  dc1394error_t err;
  uint32_t value = 0; // set to zero to avoid valgrind errors
  uint32_t format = 0; // set to zero to avoid valgrind errors
  
  err= GetCameraControlRegister(camera, REG_CAMERA_VIDEO_FORMAT, &value);
  DC1394_ERR_RTN(err, "Could not get video format");

  format= (uint32_t)((value >> 29) & 0x7UL) + DC1394_FORMAT_MIN;
  
  //if (format>FORMAT2)
  //format-=3;

  //fprintf(stderr,"format: %d\n",format);

  err= GetCameraControlRegister(camera, REG_CAMERA_VIDEO_MODE, &value);
  DC1394_ERR_RTN(err, "Could not get video mode");
  
  switch(format) {
  case DC1394_FORMAT0:
    *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT0_MIN;
    break;
  case DC1394_FORMAT1:
    *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT1_MIN;
    break;
  case DC1394_FORMAT2:
    *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT2_MIN;
    break;
  case DC1394_FORMAT6:
    *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT6_MIN;
    break;
  case DC1394_FORMAT7:
    *mode= (uint32_t)((value >> 29) & 0x7UL) + DC1394_VIDEO_MODE_FORMAT7_MIN;
    break;
  default:
    return DC1394_INVALID_VIDEO_FORMAT;
    break;
  }
  
  return err;
}

dc1394error_t
dc1394_video_set_mode(dc1394camera_t *camera, dc1394video_mode_t  mode)
{
  uint32_t format, min;
  dc1394error_t err;
  
  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;

  if ((mode<DC1394_VIDEO_MODE_MIN)||(mode>DC1394_VIDEO_MODE_MAX))
    return DC1394_INVALID_VIDEO_MODE;

  err=_dc1394_get_format_from_mode(mode, &format);
  DC1394_ERR_RTN(err, "Invalid video mode code");
  
  switch(format) {
  case DC1394_FORMAT0:
    min= DC1394_VIDEO_MODE_FORMAT0_MIN;
    break;
  case DC1394_FORMAT1:
    min= DC1394_VIDEO_MODE_FORMAT1_MIN;
    break;
  case DC1394_FORMAT2:
    min= DC1394_VIDEO_MODE_FORMAT2_MIN;
    break;
  case DC1394_FORMAT6:
    min= DC1394_VIDEO_MODE_FORMAT6_MIN;
    break;
  case DC1394_FORMAT7:
    min= DC1394_VIDEO_MODE_FORMAT7_MIN;
    break;
  default:
    return DC1394_INVALID_VIDEO_MODE;
    break;
  }

  //if (format>FORMAT2)
  //  format+=DC1394_FORMAT_GAP;
  
  err=SetCameraControlRegister(camera, REG_CAMERA_VIDEO_FORMAT, (uint32_t)(((format - DC1394_FORMAT_MIN) & 0x7UL) << 29));
  DC1394_ERR_RTN(err, "Could not set video format");

  err=SetCameraControlRegister(camera, REG_CAMERA_VIDEO_MODE, (uint32_t)(((mode - min) & 0x7UL) << 29));
  DC1394_ERR_RTN(err, "Could not set video mode");

  return err;
  
}

dc1394error_t
dc1394_video_get_iso_speed(dc1394camera_t *camera, dc1394speed_t *speed)
{
  dc1394error_t err;
  uint32_t value;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_RTN(err, "Could not get ISO data");

  if (camera->bmode_capable) { // check if 1394b is available
    if (value & 0x00008000) { //check if we are now using 1394b
      *speed= (uint32_t)(value& 0x7UL);
    }
    else { // fallback to legacy
      *speed= (uint32_t)((value >> 24) & 0x3UL);
    }
  }
  else { // legacy
    *speed= (uint32_t)((value >> 24) & 0x3UL);
  }
  
  return err;
}

dc1394error_t
dc1394_video_set_iso_speed(dc1394camera_t *camera, dc1394speed_t speed)
{
  dc1394error_t err;
  uint32_t value;
  int channel;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_RTN(err, "Could not get ISO data");

  // check if 1394b is available and if we are now using 1394b
  if ((camera->bmode_capable)&&(value & 0x00008000)) {
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "oops");
    channel=(value >> 8) & 0x3FUL;
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (uint32_t) ( ((channel & 0x3FUL) << 8) |
					       (speed & 0x7UL) |
					       (0x1 << 15) ));
    DC1394_ERR_RTN(err, "oops");
    camera->iso_speed=speed;
  }
  else { // fallback to legacy
    if (speed>DC1394_ISO_SPEED_400-DC1394_ISO_SPEED_MIN) {
      fprintf(stderr,"(%s) line %d: an ISO speed >400Mbps was requested while the camera is in LEGACY mode\n",__FILE__,__LINE__);
      fprintf(stderr,"              Please set the operation mode to OPERATION_MODE_1394B before asking for\n");
      fprintf(stderr,"              1394b ISO speeds\n");
      return DC1394_INVALID_ISO_SPEED;
    }
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "oops");
    channel=(value >> 28) & 0xFUL;
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (uint32_t) (((channel & 0xFUL) << 28) |
					      ((speed & 0x3UL) << 24) ));
    DC1394_ERR_RTN(err, "Could not set ISO data register");
    camera->iso_speed=speed;
  }
  
  return err;;
}

dc1394error_t
dc1394_video_set_iso_channel(dc1394camera_t *camera, uint32_t channel)
{
  dc1394error_t err;
  uint32_t value_inq, value;
  int speed;

  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
  DC1394_ERR_RTN(err, "Could not get basic function register");

  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_RTN(err, "Could not get ISO data");

  // check if 1394b is available and if we are now using 1394b
  if ((value_inq & 0x00800000)&&(value & 0x00008000)) {
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "oops");
    speed=value & 0x7UL;
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (uint32_t) ( ((channel & 0x3FUL) << 8) |
					       (speed & 0x7UL) |
					       (0x1 << 15) ));
    DC1394_ERR_RTN(err, "oops");
  }
  else { // fallback to legacy
    err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
    DC1394_ERR_RTN(err, "oops");
    speed=(value >> 24) & 0x3UL;
    if (speed>DC1394_ISO_SPEED_400-DC1394_ISO_SPEED_MIN) {
      fprintf(stderr,"(%s) line %d: an ISO speed >400Mbps was requested while the camera is in LEGACY mode\n",__FILE__,__LINE__);
      fprintf(stderr,"              Please set the operation mode to OPERATION_MODE_1394B before asking for\n");
      fprintf(stderr,"              1394b ISO speeds\n");
      return DC1394_FAILURE;
    }
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (uint32_t) (((channel & 0xFUL) << 28) |
					      ((speed & 0x3UL) << 24) ));
    DC1394_ERR_RTN(err, "Could not set ISO data register");
  }
  
  return err;
}

dc1394error_t
dc1394_video_get_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t  *mode)
{
  dc1394error_t err;
  uint32_t value;

  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_RTN(err, "Could not get ISO data");
  
  if (camera->bmode_capable==DC1394_TRUE) {
    if ((value & 0x00008000) >0)
      *mode=DC1394_OPERATION_MODE_1394B;
    else
      *mode=DC1394_OPERATION_MODE_LEGACY;
  }
  else {
    *mode=DC1394_OPERATION_MODE_LEGACY;
  }
  
  return err;
}


dc1394error_t
dc1394_video_set_operation_mode(dc1394camera_t *camera, dc1394operation_mode_t  mode)
{
  dc1394error_t err;
  uint32_t value;

  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_RTN(err, "Could not get ISO data");
  
  if (mode==DC1394_OPERATION_MODE_LEGACY) {
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (uint32_t) (value & 0xFFFF7FFF));
    DC1394_ERR_RTN(err, "Could not set ISO data");
  }
  else { // 1394b
    if (camera->bmode_capable) { // if 1394b available
      err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				   (uint32_t) (value | 0x00008000));
      DC1394_ERR_RTN(err, "Could not set ISO data");
    }
    else { // 1394b asked, but it is not available
      return DC1394_FUNCTION_NOT_SUPPORTED;
    }
  }
  
  return DC1394_SUCCESS;
  
}

dc1394error_t
dc1394_set_camera_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
  dc1394error_t err;
  switch (pwr) {
  case DC1394_ON:
    err=SetCameraControlRegister(camera, REG_CAMERA_POWER, DC1394_FEATURE_ON);
    DC1394_ERR_RTN(err, "Could not switch camera ON");
    break;
  case DC1394_OFF:
    err=SetCameraControlRegister(camera, REG_CAMERA_POWER, DC1394_FEATURE_OFF);
    DC1394_ERR_RTN(err, "Could not switch camera OFF");
    break;
  default:
    err=DC1394_INVALID_ARGUMENT_VALUE;
    DC1394_ERR_RTN(err, "Invalid switch value");
  }
  return err;
}

dc1394error_t
dc1394_video_set_transmission(dc1394camera_t *camera, dc1394switch_t pwr)
{
  dc1394error_t err;

  if (pwr==DC1394_ON) {
    // first allocate iso channel and bandwidth
    // err=dc1394_allocate_iso_channel_and_bandwidth(camera);
    // DC1394_ERR_RTN(err, "Could not allocate ISO channel and bandwidth");
    // then we start ISO
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_EN, DC1394_FEATURE_ON);
    if (err==DC1394_SUCCESS)
      camera->is_iso_on=1;
    DC1394_ERR_RTN(err, "Could not start ISO transmission");
  }
  else {
    // first we stop ISO
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_EN, DC1394_FEATURE_OFF);
    if (err==DC1394_SUCCESS) {
      camera->is_iso_on=0;
      // then we free iso bandwidth and iso channels
      // err=dc1394_free_iso_channel_and_bandwidth(camera);
      // DC1394_ERR_RTN(err, "Could not free ISO channel and bandwidth");
    }
    DC1394_ERR_RTN(err, "Could not stop ISO transmission");
  }

  return err;
}

dc1394error_t
dc1394_video_get_transmission(dc1394camera_t *camera, dc1394switch_t *is_on)
{
  dc1394error_t err;
  uint32_t value;
  err= GetCameraControlRegister(camera, REG_CAMERA_ISO_EN, &value);
  DC1394_ERR_RTN(err, "Could not get ISO status");
  
  *is_on= (value & DC1394_FEATURE_ON)>>31;
  return err;
}

dc1394error_t
dc1394_video_set_one_shot(dc1394camera_t *camera, dc1394switch_t pwr)
{
  dc1394error_t err;
  switch (pwr) {
  case DC1394_ON:
    err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, DC1394_FEATURE_ON);
    DC1394_ERR_RTN(err, "Could not set one-shot");
    break;
  case DC1394_OFF:
    err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, DC1394_FEATURE_OFF);
    DC1394_ERR_RTN(err, "Could not unset one-shot");
    break;
  default:
    err=DC1394_INVALID_ARGUMENT_VALUE;
    DC1394_ERR_RTN(err, "Invalid switch value");
  }
  return err;
}

dc1394error_t
dc1394_video_get_one_shot(dc1394camera_t *camera, dc1394bool_t *is_on)
{
  uint32_t value;
  dc1394error_t err = GetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, &value);
  DC1394_ERR_RTN(err, "Could not get one-shot status");
  *is_on = value & DC1394_FEATURE_ON;
  return err;
}

dc1394error_t
dc1394_video_get_multi_shot(dc1394camera_t *camera, dc1394bool_t *is_on, uint32_t *numFrames)
{
  uint32_t value;
  dc1394error_t err = GetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, &value);
  DC1394_ERR_RTN(err, "Could not get multishot status");
  *is_on = (value & (DC1394_FEATURE_ON>>1)) >> 30;
  *numFrames= value & 0xFFFFUL;
  
  return err;  
}

dc1394error_t
dc1394_video_set_multi_shot(dc1394camera_t *camera, uint32_t numFrames, dc1394switch_t pwr)
{
  dc1394error_t err;
  switch (pwr) {
  case DC1394_ON:
    err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT,
				 (0x40000000UL | (numFrames & 0xFFFFUL)));
    DC1394_ERR_RTN(err, "Could not set multishot");
    break;
  case DC1394_OFF:
    err=dc1394_video_set_one_shot(camera,pwr);
    DC1394_ERR_RTN(err, "Could not unset multishot");
    break;
  default:
    err=DC1394_INVALID_ARGUMENT_VALUE;
    DC1394_ERR_RTN(err, "Invalid switch value");
  }
  return err;
}

dc1394error_t
dc1394_feature_whitebalance_get_value(dc1394camera_t *camera, uint32_t *u_b_value, uint32_t *v_r_value)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, &value);
  DC1394_ERR_RTN(err, "Could not get white balance");

  *u_b_value= (uint32_t)((value & 0xFFF000UL) >> 12);
  *v_r_value= (uint32_t)(value & 0xFFFUL);
  return err;
}

dc1394error_t
dc1394_feature_whitebalance_set_value(dc1394camera_t *camera, uint32_t u_b_value, uint32_t v_r_value)
{
  uint32_t curval;
  dc1394error_t err;
  err=GetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, &curval);
  DC1394_ERR_RTN(err, "Could not get white balance");
  
  curval= (curval & 0xFF000000UL) | ( ((u_b_value & 0xFFFUL) << 12) |
				      (v_r_value & 0xFFFUL) );
  err=SetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, curval);
  DC1394_ERR_RTN(err, "Could not set white balance");
  return err;
}

dc1394error_t
dc1394_feature_temperature_get_value(dc1394camera_t *camera, uint32_t *target_temperature, uint32_t *temperature)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, &value);
  DC1394_ERR_RTN(err, "Could not get temperature");
  *target_temperature= (uint32_t)((value >> 12) & 0xFFF);
  *temperature= (uint32_t)(value & 0xFFFUL);
  return err;
}

dc1394error_t
dc1394_feature_temperature_set_value(dc1394camera_t *camera, uint32_t target_temperature)
{
  dc1394error_t err;
  uint32_t curval;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, &curval);
  DC1394_ERR_RTN(err, "Could not get temperature");
  
  curval= (curval & 0xFF000FFFUL) | ((target_temperature & 0xFFFUL) << 12);
  err= SetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, curval);
  DC1394_ERR_RTN(err, "Could not set temperature");

  return err;
}

dc1394error_t
dc1394_feature_whiteshading_get_value(dc1394camera_t *camera, uint32_t *r_value, uint32_t *g_value, uint32_t *b_value)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, &value);
  DC1394_ERR_RTN(err, "Could not get white shading");
  
  *r_value= (uint32_t)((value & 0xFF0000UL) >> 16);
  *g_value= (uint32_t)((value & 0xFF00UL) >> 8);
  *b_value= (uint32_t)(value & 0xFFUL);

  return err;
}

dc1394error_t
dc1394_feature_whiteshading_set_value(dc1394camera_t *camera, uint32_t r_value, uint32_t g_value, uint32_t b_value)
{
  uint32_t curval;
  
  dc1394error_t err=GetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, &curval);
  DC1394_ERR_RTN(err, "Could not get white shading");
  
  curval= (curval & 0xFF000000UL) | ( ((r_value & 0xFFUL) << 16) |
				      ((g_value & 0xFFUL) << 8) |
				      (b_value & 0xFFUL) );
  err=SetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, curval);
  DC1394_ERR_RTN(err, "Could not set white shading");

  return err;
}

dc1394error_t
dc1394_external_trigger_get_mode(dc1394camera_t *camera, dc1394trigger_mode_t *mode)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &value);
  DC1394_ERR_RTN(err, "Could not get trigger mode");
  
  *mode= (uint32_t)( ((value >> 16) & 0xFUL) );
  if ((*mode)>5)
    (*mode)-=8;
  (*mode)+= DC1394_TRIGGER_MODE_MIN;

  return err;
}

dc1394error_t
dc1394_external_trigger_set_mode(dc1394camera_t *camera, dc1394trigger_mode_t mode)
{
  dc1394error_t err;
  uint32_t curval;
  
  if ( (mode < DC1394_TRIGGER_MODE_MIN) || (mode > DC1394_TRIGGER_MODE_MAX) ) {
    return DC1394_INVALID_TRIGGER_MODE;
  }
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &curval);
  DC1394_ERR_RTN(err, "Could not get trigger mode");
  
  mode-= DC1394_TRIGGER_MODE_MIN;
  if (mode>5)
    mode+=8;
  curval= (curval & 0xFFF0FFFFUL) | ((mode & 0xFUL) << 16);
  err=SetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, curval);
  DC1394_ERR_RTN(err, "Could not set trigger mode");
  return err;
}


dc1394error_t
dc1394_external_trigger_get_supported_sources(dc1394camera_t *camera, dc1394trigger_sources_t *sources)
{
  uint32_t value;
  dc1394error_t err;
  uint64_t offset;
  int i;

  FEATURE_TO_INQUIRY_OFFSET(DC1394_FEATURE_TRIGGER, offset);
  err=GetCameraControlRegister(camera, offset, &value);
  DC1394_ERR_RTN(err,"Could not query supported trigger sources");

  sources->num=0;
  value=( (value & (0xF << 23)) >>23 );
  for (i=DC1394_TRIGGER_SOURCE_MIN;i<=DC1394_TRIGGER_SOURCE_MAX;i++) {
    if (value & (0x1 << (i-DC1394_TRIGGER_SOURCE_MIN))){ 
      sources->sources[sources->num]=i;
      sources->num++;
    }
  }

  return err;
}


dc1394error_t
dc1394_external_trigger_get_source(dc1394camera_t *camera, dc1394trigger_source_t *source)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &value);
  DC1394_ERR_RTN(err, "Could not get trigger source");
  
  *source= (uint32_t)( ((value >> 21) & 0x8UL) );
  (*source)+= DC1394_TRIGGER_SOURCE_MIN;

  return err;
}

dc1394error_t
dc1394_external_trigger_set_source(dc1394camera_t *camera, dc1394trigger_source_t source)
{
  dc1394error_t err;
  uint32_t curval;
  
  if ( (source < DC1394_TRIGGER_SOURCE_MIN) || (source > DC1394_TRIGGER_SOURCE_MAX) ) {
    return DC1394_INVALID_TRIGGER_SOURCE;
  }
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &curval);
  DC1394_ERR_RTN(err, "Could not get trigger source");
  
  source-= DC1394_TRIGGER_MODE_MIN;
  curval= (curval & 0xFF1FFFFFUL) | ((source & 0x8UL) << 21);
  err=SetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, curval);
  DC1394_ERR_RTN(err, "Could not set trigger source");
  return err;
}

dc1394error_t
dc1394_feature_get_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *value)
{
  uint32_t quadval;
  uint64_t offset;
  dc1394error_t err;

  if ((feature==DC1394_FEATURE_WHITE_BALANCE)||
      (feature==DC1394_FEATURE_WHITE_SHADING)||
      (feature==DC1394_FEATURE_TEMPERATURE)) {
    err=DC1394_INVALID_FEATURE;
    DC1394_ERR_RTN(err, "You should use the specific functions to read from multiple-value features");
  }

  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get feature %d value",feature);
  *value= (uint32_t)(quadval & 0xFFFUL);
  
  return err;
}

dc1394error_t
dc1394_feature_set_value(dc1394camera_t *camera, dc1394feature_t feature, uint32_t value)
{
  uint32_t quadval;
  uint64_t offset;
  dc1394error_t err;
  
  if ((feature==DC1394_FEATURE_WHITE_BALANCE)||
      (feature==DC1394_FEATURE_WHITE_SHADING)||
      (feature==DC1394_FEATURE_TEMPERATURE)) {
    err=DC1394_INVALID_FEATURE;
    DC1394_ERR_RTN(err, "You should use the specific functions to write from multiple-value features");
  }

  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get feature %d value",feature);
  
  err=SetCameraControlRegister(camera, offset, (quadval & 0xFFFFF000UL) | (value & 0xFFFUL));
  DC1394_ERR_RTN(err, "Could not set feature %d value",feature);
  return err;
}

dc1394error_t
dc1394_feature_is_present(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  *value=DC1394_FALSE;
  
  // check feature presence in 0x404 and 0x408
  if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
    return DC1394_INVALID_FEATURE;
  }
  
  if (feature < DC1394_FEATURE_ZOOM) {
    offset= REG_CAMERA_FEATURE_HI_INQ;
  }
  else {
    offset= REG_CAMERA_FEATURE_LO_INQ;
  }

  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get register for feature %d",feature);
  
  if (IsFeatureBitSet(quadval, feature)!=DC1394_TRUE) {
    *value=DC1394_FALSE;
    return DC1394_SUCCESS;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get register for feature %d",feature);
  
  if (quadval & 0x80000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_has_one_push_auto(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get one-push capability for feature %d",feature);
  
  if (quadval & 0x10000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_is_readable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get read-out capability for feature %d",feature);
  
  if (quadval & 0x08000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_is_switchable(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get power capability for feature %d",feature);
  
  if (quadval & 0x04000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_get_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get feature %d status",feature);
  
  if (quadval & 0x02000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_set_power(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t curval;

  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_RTN(err, "Could not get feature %d register",feature);
  
  if (value && !(curval & 0x02000000UL)) {    
    curval|= 0x02000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set feature %d power",feature);
  }
  else if (!value && (curval & 0x02000000UL)) {
    curval&= 0xFDFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set feature %d power",feature);
  }
  
  return err;
}

dc1394error_t
dc1394_feature_has_auto_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  if (feature == DC1394_FEATURE_TRIGGER) {
    return DC1394_INVALID_FEATURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get automode capability for feature %d",feature);
  
  if (quadval & 0x02000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_has_manual_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  if (feature == DC1394_FEATURE_TRIGGER) {
    return DC1394_INVALID_FEATURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get manual modecapability for feature %d",feature);
  
  if (quadval & 0x01000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_get_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t *mode)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  if (feature == DC1394_FEATURE_TRIGGER) {
    return DC1394_INVALID_FEATURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get feature %d auto status",feature);
  
  if (quadval & 0x04000000UL) {
    *mode= DC1394_FEATURE_MODE_ONE_PUSH_AUTO;
  }
  else if (quadval & 0x01000000UL) {
    *mode= DC1394_FEATURE_MODE_AUTO;
  }
  else {
    *mode= DC1394_FEATURE_MODE_MANUAL;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_set_mode(dc1394camera_t *camera, dc1394feature_t feature, dc1394feature_mode_t mode)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t curval;

  if (feature == DC1394_FEATURE_TRIGGER) {
    return DC1394_INVALID_FEATURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_RTN(err, "Could not get feature %d register",feature);
  
  if ((mode==DC1394_FEATURE_MODE_AUTO) && !(curval & 0x01000000UL)) {
    curval|= 0x01000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set auto mode for feature %d",feature);
  }
  else if ((mode==DC1394_FEATURE_MODE_MANUAL) && (curval & 0x01000000UL)) {
    curval&= 0xFEFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set auto mode for feature %d",feature);
  }
  else if ((mode==DC1394_FEATURE_MODE_ONE_PUSH_AUTO)&& !(curval & 0x04000000UL)) {
    curval|= 0x04000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not sart one-push capability for feature %d",feature);
  }
  
  return err;
}

dc1394error_t
dc1394_feature_get_boundaries(dc1394camera_t *camera, dc1394feature_t feature, uint32_t *min, uint32_t *max)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  if (feature == DC1394_FEATURE_TRIGGER) {
    return DC1394_INVALID_FEATURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get feature %d min value",feature);
  
  *min= (uint32_t)((quadval & 0xFFF000UL) >> 12);
  *max= (uint32_t)(quadval & 0xFFFUL);
  return err;
}

/*
 * Memory load/save functions
 */

dc1394error_t
dc1394_memory_is_save_in_operation(dc1394camera_t *camera, dc1394bool_t *value)
{
  uint32_t quadlet;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_MEMORY_SAVE, &quadlet);
  DC1394_ERR_RTN(err, "Could not get memory busy status");
  *value = (quadlet & DC1394_FEATURE_ON) >> 31;
  return err;
}

dc1394error_t
dc1394_memory_save(dc1394camera_t *camera, uint32_t channel)
{
  dc1394error_t err=SetCameraControlRegister(camera, REG_CAMERA_MEM_SAVE_CH,
				  (uint32_t)((channel & 0xFUL) << 28));
  DC1394_ERR_RTN(err, "Could not save memory channel");

  err=SetCameraControlRegister(camera, REG_CAMERA_MEMORY_SAVE, DC1394_FEATURE_ON);
  DC1394_ERR_RTN(err, "Could not save to memory");
  return err;
}

dc1394error_t
dc1394_memory_load(dc1394camera_t *camera, uint32_t channel)
{
  dc1394error_t err=SetCameraControlRegister(camera, REG_CAMERA_CUR_MEM_CH,
				  (uint32_t)((channel & 0xFUL) << 28));
  DC1394_ERR_RTN(err, "Could not load from memory");
  return err;
}

/*
 * Trigger functions
 */

dc1394error_t
dc1394_external_trigger_set_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t polarity)
{
  dc1394error_t err;
  uint32_t curval;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &curval);
  DC1394_ERR_RTN(err, "Could not get trigger register");
  
  curval= (curval & 0xFEFFFFFFUL) | ((polarity & 0x1UL) << 24);
  err=SetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, curval);
  DC1394_ERR_RTN(err, "Could not set set trigger polarity");
  return err;
}

dc1394error_t
dc1394_external_trigger_get_polarity(dc1394camera_t *camera, dc1394trigger_polarity_t *polarity)
{
  uint32_t value;
  dc1394error_t err= GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &value);
  DC1394_ERR_RTN(err, "Could not get trigger polarity");
  
  *polarity= (uint32_t)( ((value >> 24) & 0x1UL) );
  return err;
}

dc1394error_t
dc1394_external_trigger_has_polarity(dc1394camera_t *camera, dc1394bool_t *polarity)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
  
  err=GetCameraControlRegister(camera, offset + ((DC1394_FEATURE_TRIGGER - DC1394_FEATURE_MIN) * 0x04U), &quadval);
  DC1394_ERR_RTN(err, "Could not get trigger polarity capability");
  
  if (quadval & 0x02000000UL) {
    *polarity= DC1394_TRUE;
  }
  else {
    *polarity= DC1394_FALSE;
  }
  
  return err;
}

dc1394error_t
dc1394_external_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
  dc1394error_t err=dc1394_feature_set_power(camera, DC1394_FEATURE_TRIGGER, pwr);
  DC1394_ERR_RTN(err, "Could not set external trigger");
  return err;
}

dc1394error_t
dc1394_external_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr)
{
  dc1394error_t err=dc1394_feature_get_power(camera, DC1394_FEATURE_TRIGGER, pwr);
  DC1394_ERR_RTN(err, "Could not set external trigger");
  return err;
}

dc1394error_t
dc1394_software_trigger_set_power(dc1394camera_t *camera, dc1394switch_t pwr)
{
  dc1394error_t err;

  if (pwr==DC1394_ON) {
    err=SetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, DC1394_FEATURE_ON);
  }
  else {
    err=SetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, DC1394_FEATURE_OFF);
  }
  DC1394_ERR_RTN(err, "Could not set software trigger");
  return err;
}

dc1394error_t
dc1394_software_trigger_get_power(dc1394camera_t *camera, dc1394switch_t *pwr)
{
  uint32_t value;
  dc1394error_t err = GetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, &value);
  DC1394_ERR_RTN(err, "Could not get software trigger status");
  
  *pwr = (value & DC1394_FEATURE_ON)? DC1394_ON : DC1394_OFF;

  return err;
}

dc1394error_t
dc1394_video_get_data_depth(dc1394camera_t *camera, uint32_t *depth)
{
  dc1394error_t err;
  uint32_t value;
  dc1394video_mode_t mode;
  dc1394color_coding_t coding;

  *depth = 0;
  if (camera->iidc_version >= DC1394_IIDC_VERSION_1_31) {
    err= GetCameraControlRegister(camera, REG_CAMERA_DATA_DEPTH, &value);
    if (err==DC1394_SUCCESS)
      *depth = value >> 24;
  }

  /* For cameras that do not have the DATA_DEPTH register, perform a
     sane default. */
  if (*depth == 0) {
    err = dc1394_video_get_mode(camera, &mode);
    DC1394_ERR_RTN(err, "Could not get video mode");

    if (dc1394_is_video_mode_scalable (mode))
      return dc1394_format7_get_data_depth (camera, mode, depth);

    err = dc1394_get_color_coding_from_video_mode (camera, mode, &coding);
    DC1394_ERR_RTN(err, "Could not get color coding");
    
    err = dc1394_get_color_coding_depth (coding, depth);
    DC1394_ERR_RTN(err, "Could not get data depth from color coding");

    return err;
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_feature_get_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t *pwr)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get get abs control for feature %d",feature);
  
  if (quadval & 0x40000000UL) {
    *pwr= DC1394_ON;
  }
  else {
    *pwr= DC1394_OFF;
  }
  
  return err;
}

dc1394error_t
dc1394_feature_set_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394switch_t pwr)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t curval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_RTN(err, "Could not get abs setting status for feature %d",feature);
  
  if (pwr && !(curval & 0x40000000UL)) {
    curval|= 0x40000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set absolute control for feature %d",feature);
  }
  else if (!pwr && (curval & 0x40000000UL)) {
    curval&= 0xBFFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_RTN(err, "Could not set absolute control for feature %d",feature);
  }
  
  return err;
}


dc1394error_t
dc1394_feature_has_absolute_control(dc1394camera_t *camera, dc1394feature_t feature, dc1394bool_t *value)
{
  dc1394error_t err;
  uint64_t offset;
  uint32_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_RTN(err, "Could not get absolute control register for feature %d",feature);
  
  if (quadval & 0x40000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}


/* This function returns the bandwidth that is used by the camera *IF* ISO was ON.
   The returned value is in bandwidth units. The 1394 bus has 4915 bandwidth units
   available per cycle. Each unit corresponds to the time it takes to send one
   quadlet at ISO speed S1600. The bandwidth usage at S400 is thus four times the
   number of quadlets per packet. Thanks to Krisitian Hogsberg for clarifying this.
*/

dc1394error_t
dc1394_video_get_bandwidth_usage(dc1394camera_t *camera, uint32_t *bandwidth)
{
  uint32_t format, qpp;
  dc1394video_mode_t video_mode;
  dc1394speed_t speed;
  dc1394framerate_t framerate=0;
  dc1394error_t err;

  // get format and mode
  err=dc1394_video_get_mode(camera, &video_mode);
  DC1394_ERR_RTN(err, "Could not get video mode");
  
  err=_dc1394_get_format_from_mode(video_mode, &format);
  DC1394_ERR_RTN(err, "Invalid mode ID");
  
  if (format==DC1394_FORMAT7) {
    // use the bytes per packet value:
    err=dc1394_format7_get_byte_per_packet(camera, video_mode, &qpp);
    DC1394_ERR_RTN(err, "Could not get BPP");
    qpp=qpp/4;
  }
  else {
    // get the framerate:
    err=dc1394_video_get_framerate(camera, &framerate);
    DC1394_ERR_RTN(err, "Could not get framerate");
    err=_dc1394_get_quadlets_per_packet(video_mode, framerate, &qpp); 
  }
  // add the ISO header and footer:
  qpp+=3;
  
  // get camera ISO speed:
  err=dc1394_video_get_iso_speed(camera, &speed);
  DC1394_ERR_RTN(err, "Could not get ISO speed");
  
  // mutiply by 4 anyway because the best speed is SPEED_400 only
  if (speed>=DC1394_ISO_SPEED_1600)
    *bandwidth = qpp >> (speed-DC1394_ISO_SPEED_1600);
  else
    *bandwidth = qpp << (DC1394_ISO_SPEED_1600-speed);

  return err;
}

dc1394error_t
dc1394_video_specify_iso_channel(dc1394camera_t *camera, int iso_channel)
{
  if (camera->capture_is_set>0)
    return DC1394_CAPTURE_IS_RUNNING;
  
  camera->iso_channel=iso_channel;

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_feature_get_absolute_boundaries(dc1394camera_t *camera, dc1394feature_t feature, float *min, float *max)
{
  dc1394error_t err=DC1394_SUCCESS;
  
  if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
    return DC1394_INVALID_FEATURE;
  }

  err=GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_MAX, (uint32_t*)max);
  DC1394_ERR_RTN(err,"Could not get maximal absolute value");
  err=GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_MIN, (uint32_t*)min);
  DC1394_ERR_RTN(err,"Could not get minimal absolute value");

  return err;
}


dc1394error_t
dc1394_feature_get_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float *value)
{
  dc1394error_t err=DC1394_SUCCESS;

  if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
    return DC1394_INVALID_FEATURE;
  }
  err=GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_VALUE, (uint32_t*)value);
  DC1394_ERR_RTN(err,"Could not get current absolute value");

  return err;
}


dc1394error_t
dc1394_feature_set_absolute_value(dc1394camera_t *camera, dc1394feature_t feature, float value)
{
  dc1394error_t err=DC1394_SUCCESS;
  
  uint32_t tempq;
  memcpy(&tempq,&value,4);

  if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {
    return DC1394_INVALID_FEATURE;
  }

  SetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_VALUE, tempq);
  DC1394_ERR_RTN(err,"Could not get current absolute value");

  return err;
}
 

dc1394error_t
dc1394_pio_set(dc1394camera_t *camera, uint32_t value)
{
  dc1394error_t err=DC1394_SUCCESS;

  err=SetCameraPIOControlRegister(camera, REG_CAMERA_PIO_OUT, value);
  DC1394_ERR_RTN(err,"Could not set PIO value");

  return err;
}


dc1394error_t
dc1394_pio_get(dc1394camera_t *camera, uint32_t *value)
{
  dc1394error_t err=DC1394_SUCCESS;

  err=GetCameraPIOControlRegister(camera, REG_CAMERA_PIO_IN, value);
  DC1394_ERR_RTN(err,"Could not get PIO value");

  return err;
}
