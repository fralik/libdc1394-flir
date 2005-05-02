/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
 * Additions by Chris Urmson <curmson@ri.cmu.edu>
 * Additions by Damien Douxchamps <ddouxchamps@users.sf.net>
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
 * Ann Dang <AnnDang@smarttech.com>
 *   - bug fixes
 *
 * Dan Dennedy <dan@dennedy.org>
 *  - bug fixes
 *
 * Yves Jaeger <yves.jaeger@fraenz-jaeger.de>
 *  - software trigger functions
 *
 * Pierre Moos <pierre.moos@gmail.com> 
 *  - AVT Marlin vendor specific functions
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
#include "config.h"
#include "dc1394_internal.h"
#include "dc1394_register.h"
#include "dc1394_offsets.h"

dc1394camera_t*
dc1394_new_camera(int port)
{
  dc1394camera_t *cam;

  cam=(dc1394camera_t *)malloc(sizeof(dc1394camera_t));

  if (cam==NULL)
    return NULL;

  cam->handle=raw1394_new_handle();
  cam->port=port;

  raw1394_set_port(cam->handle, cam->port);

  return cam;
}

void
dc1394_free_camera(dc1394camera_t *camera)
{
  if (camera!=NULL) { 
    raw1394_destroy_handle(camera->handle); 
    free(camera);
  }
  camera=NULL;
}

int
dc1394_find_cameras(dc1394camera_t **cameras, int* numCameras)
{
  // get the number the ports
  raw1394handle_t handle;
  int port_num, port;
  int allocated_size;
  int numCam, err=DC1394_SUCCESS, i, numNodes;
  nodeid_t node;
  //dc1394bool_t isCamera;
  dc1394camera_t *tmpcam=NULL;
  dc1394camera_t **newcam;

  handle=raw1394_new_handle();
  port_num=raw1394_get_port_info(handle, NULL, 0);
  //raw1394_destroy_handle(handle);

  allocated_size=64; // initial allocation, will be reallocated if necessary
  cameras=(dc1394camera_t**)malloc(allocated_size*sizeof(dc1394camera_t*));
  numCam=0;

  // scan each port for cameras. When a camera is found add it.
  // if the number of cameras is not enough the array is re-allocated.

  for (port=0;port<port_num;port++) {
    // get a handle to the current interface card
    raw1394_set_port(handle, port);
    //handle=raw1394_new_handle(port);

    // find the cameras on this card
    numNodes = raw1394_get_nodecount(handle);
    for (node=0;node<numNodes;i++){

      // create a camera struct for probing
      if (tmpcam==NULL) {
	tmpcam=dc1394_new_camera(port);

	if (tmpcam==NULL) {

	  for (i=0;i<numCam;i++)
	    dc1394_free_camera(cameras[i]);
	  free(cameras);

	  raw1394_destroy_handle(handle);
	  fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s",
		  __FILE__, __FUNCTION__, __LINE__,
		  "Can't allocate camera structure");
	  return DC1394_MEMORY_ALLOCATION_FAILURE;
	}
      }
	
      err=dc1394_get_camera_info(tmpcam);
      if ((err != DC1394_SUCCESS) && (err != DC1394_NOT_A_CAMERA)) {

	for (i=0;i<numCam;i++)
	  dc1394_free_camera(cameras[i]);
	free(cameras);

	dc1394_free_camera(tmpcam);

	raw1394_destroy_handle(handle);
	fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s",
		__FILE__, __FUNCTION__, __LINE__,
		"Can't check if node is a camera");
	return err;
      }

      if (err == DC1394_SUCCESS) {
	cameras[numCam]=tmpcam;
	tmpcam=NULL;
	numCam++;
      }

      if (numCam>=allocated_size) {
	allocated_size*=2;
	newcam=realloc(cameras,allocated_size*sizeof(dc1394camera_t*));
	if (newcam ==NULL) {
	  for (i=0;i<numCam;i++)
	    dc1394_free_camera(cameras[i]);
	  free(cameras);

	  raw1394_destroy_handle(handle);

	  if (tmpcam!=NULL)
	    dc1394_free_camera(tmpcam);

	  fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s",
		  __FILE__, __FUNCTION__, __LINE__,
		  "Can't reallocate camera array");
	  return DC1394_MEMORY_ALLOCATION_FAILURE;
	}
	else {
	  cameras=newcam;
	}
      }
    }
  }

  raw1394_destroy_handle(handle);

  *numCameras=numCam;

  return err;
}

/*****************************************************
 dc1394_create_handle

 This creates a raw1394_handle
 If a handle can't be created, it returns NULL
*****************************************************/
/*
raw1394handle_t 
dc1394_create_handle(int port) 
{
  raw1394handle_t handle;

#ifdef LIBRAW1394_OLD
  if (!(handle= raw1394_get_handle())) {
#else
  if (!(handle= raw1394_new_handle())) {
#endif
    printf("(%s) Couldn't get raw1394 handle!\n",__FILE__);
    return NULL;
  }

  if (raw1394_set_port(handle, port) < 0) {

    if (handle != NULL)
      raw1394_destroy_handle(handle);
    
    printf("(%s) Couldn't raw1394_set_port!\n",__FILE__);
    return NULL;
  }
  
  return handle;
}

int
dc1394_destroy_handle(raw1394handle_t handle)
{
  if( handle != NULL )
    raw1394_destroy_handle(handle);
  
  return DC1394_SUCCESS;
}
*/
/*
int
dc1394_is_camera(dc1394camera_t *camera, dc1394bool_t *value)
{
  octlet_t offset;
  octlet_t ud_offset = 0;
  quadlet_t quadval = 0;
  dc1394bool_t ptgrey;
  int err;

  ptgrey=(camera->ud_reg_tag_12 == 0x00B09DUL);
  
  if ( ! ( (camera->ud_reg_tag_12 == 0x00A02DUL) || ptgrey) ) {
    return DC1394_NOT_A_CAMERA;
  }
  
  quadval = 0;
  // get the unit_iidc_version (should be 0x000100 - 0x000102 for 1394 digital camera)
  // DRD> without this check, AV/C cameras show up as well
  if ((quadval == 0x000100UL) || 
      (quadval == 0x000101UL) ||
      (quadval == 0x000102UL) ||
      ((quadval == 0x000114UL) && ptgrey) ||
      ((quadval == 0x800002UL) && ptgrey)) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}
*/
int
_dc1394_get_iidc_version(dc1394camera_t *camera)
{
  int err=DC1394_SUCCESS;
  quadlet_t quadval;

  if (camera == NULL)
    return DC1394_FAILURE;

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
       (camera->ud_reg_tag_12 == 0x000B09DUL) ) {
    camera->iidc_version=-1;
    return DC1394_NOT_A_CAMERA;
  }
  
  switch (camera->ud_reg_tag_13&0xFFFFFFUL) {
  case 0x000100:
    camera->iidc_version=IIDC_VERSION_1_04;
    break;
  case 0x000101:
    camera->iidc_version=IIDC_VERSION_1_20;
    break;
  case 0x000102:
    camera->iidc_version=IIDC_VERSION_1_30;
    break;
  case 0x000114:
  case 0x800002:
    if (camera->ud_reg_tag_12 == 0x000B09DUL)
      camera->iidc_version=IIDC_VERSION_PTGREY;
    else
      camera->iidc_version = -1; // an error should be sent in this case
    break;
  }

  /* IIDC 1.31 check */
  if (camera->iidc_version==IIDC_VERSION_1_30) {
    
    err=GetConfigROMTaggedRegister(camera, 0x38, &camera->unit_dependent_directory, &quadval);
    if (err!=DC1394_SUCCESS) {
      if (err==DC1394_TAGGED_REGISTER_NOT_FOUND) {
	// if it fails here we return success with the most accurate version estimation: 1.30.
	// this is because the GetConfigROMTaggedRegister will return a failure both if there is a comm
	// problem but also if the tag is not found. In the latter case it simply means that the
	// camera version is 1.30
	camera->iidc_version=IIDC_VERSION_1_30;
	return DC1394_SUCCESS;
      }
      else 
	DC1394_ERR_CHK(err, "Could not get tagged register 0x38");
    }

    switch (quadval&0xFFFFFFUL) {
    case 0x10:
      camera->iidc_version=IIDC_VERSION_1_31;
      break;
    case 0x20:
      camera->iidc_version=IIDC_VERSION_1_32;
      break;
    case 0x30:
      camera->iidc_version=IIDC_VERSION_1_33;
      break;
    case 0x40:
      camera->iidc_version=IIDC_VERSION_1_34;
      break;
    case 0x50:
      camera->iidc_version=IIDC_VERSION_1_35;
      break;
    case 0x60:
      camera->iidc_version=IIDC_VERSION_1_36;
      break;
    case 0x70:
      camera->iidc_version=IIDC_VERSION_1_37;
      break;
    case 0x80:
      camera->iidc_version=IIDC_VERSION_1_38;
      break;
    case 0x90:
      camera->iidc_version=IIDC_VERSION_1_39;
      break;
    default:
      camera->iidc_version=IIDC_VERSION_1_30;
      break;
    }
  }
  
  return err;
}

int
dc1394_print_camera_info(dc1394camera_t *camera) 
{
  quadlet_t value[2];
  
  value[0]= camera->euid_64 & 0xffffffff;
  value[1]= (camera->euid_64 >>32) & 0xffffffff;
  printf("CAMERA INFO\n===============\n");
  printf("Node: %x\n", camera->node);
  printf("CCR_Offset: %llx\n", (long long unsigned int)camera->command_registers_base);
  //L added by tim evers 
  printf("UID: 0x%08x%08x\n", value[1], value[0]);
  printf("Vendor: %s\tModel: %s\n\n", camera->vendor, camera->model);
  fflush(stdout);
  
  return DC1394_SUCCESS;
}

int
dc1394_get_camera_info(dc1394camera_t *camera)
{
  int err, len, i;
  octlet_t offset;
  quadlet_t value[2], quadval;
  unsigned int count;

  //err= dc1394_is_camera(camera, &iscamera);
  //DC1394_ERR_CHK(err, "Error - this is not a camera");

  //if (iscamera != DC1394_TRUE) {
  //  return DC1394_FAILURE;
  //}
  
  //camera->handle= handle;
  //camera->id= node;

  // init pointers to zero:
  camera->command_registers_base=0;
  camera->unit_directory=0;
  camera->unit_dependent_directory=0;
  camera->advanced_features_csr=0;
  for (i=0;i<NUM_MODE_FORMAT7;i++)
    camera->format7_csr[i]=0;
  
  /* now get the EUID-64 */
  err=GetCameraROMValue(camera, ROM_BUS_INFO_BLOCK+0x0C, &value[0]);
  DC1394_ERR_CHK(err, "Could not get EUID-LSB");

  err=GetCameraROMValue(camera, ROM_BUS_INFO_BLOCK+0x10, &value[1]);
  DC1394_ERR_CHK(err, "Could not get EUID-MSB");
  
  camera->euid_64= ((u_int64_t)value[0] << 32) | (u_int64_t)value[1];
  
  /* get the unit_directory offset */
  offset= ROM_ROOT_DIRECTORY;
  err=GetConfigROMTaggedRegister(camera, 0xD1, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get unit directory offset");
  camera->unit_directory=(quadval & 0xFFFFFFUL)*4+offset;
  
  /* get the spec_id value */
  err=GetConfigROMTaggedRegister(camera, 0x12, &offset, &quadval);
  DC1394_ERR_CHK(err, "COuld not get spec ID");
  camera->ud_reg_tag_12=quadval&0xFFFFFFUL;

  /* get the iidc revision */
  err=GetConfigROMTaggedRegister(camera, 0x13, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get IIDC revision");
  camera->ud_reg_tag_13=quadval&0xFFFFFFUL;

  /* get the unit_dependent_directory offset */
  offset= camera->unit_directory;
  err=GetConfigROMTaggedRegister(camera, 0xD4, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get unit dependent directory");
  camera->unit_dependent_directory=(quadval & 0xFFFFFFUL)*4+offset;
  
  /* verify the version/revision and find the IIDC_REVISION value from that */
  err=_dc1394_get_iidc_version(camera);
  if (err==DC1394_NOT_A_CAMERA)
    return err;
  else
    DC1394_ERR_CHK(err, "Problem inferring the IIDC version");

  /* now get the command_regs_base */
  offset= camera->unit_dependent_directory;
  err=GetConfigROMTaggedRegister(camera, 0x40, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get commands base address");
  camera->command_registers_base= (octlet_t)(quadval & 0xFFFFFFUL)*4;

  /* get the vendor_name_leaf offset (optional) */
  offset= camera->unit_dependent_directory;
  camera->vendor[0] = '\0';
  err=GetConfigROMTaggedRegister(camera, 0x81, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get vendor leaf offset");
  offset=(quadval & 0xFFFFFFUL)*4+offset;
    
  /* read in the length of the vendor name */
  err=GetCameraROMValue(camera, offset, &value[0]);
  DC1394_ERR_CHK(err, "Could not get vendor leaf length");
  
  len= (int)(value[0] >> 16)*4-8; /* Tim Evers corrected length value */ 
  
  if (len > MAX_CHARS) {
    len= MAX_CHARS;
  }
  offset+= 12;
  count= 0;
  
  /* grab the vendor name */
  while (len > 0) {
    err=GetCameraROMValue(camera, offset+count, &value[0]);
    DC1394_ERR_CHK(err, "Could not get vendor string character");

    camera->vendor[count++]= (value[0] >> 24);
    camera->vendor[count++]= (value[0] >> 16) & 0xFFUL;
    camera->vendor[count++]= (value[0] >> 8) & 0xFFUL;
    camera->vendor[count++]= value[0] & 0xFFUL;
    len-= 4;
    
    camera->vendor[count]= '\0';
  }
  
  /* get the model_name_leaf offset (optional) */
  offset= camera->unit_dependent_directory;
  camera->model[0] = '\0';
  err=GetConfigROMTaggedRegister(camera, 0x82, &offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get model name leaf offset");
  offset=(quadval & 0xFFFFFFUL)*4+offset;
    
  /* read in the length of the model name */
  err=GetCameraROMValue(camera, offset, &value[0]);
  DC1394_ERR_CHK(err, "Could not get model name leaf length");
  
  len= (int)(value[0] >> 16)*4-8; /* Tim Evers corrected length value */ 
  
  if (len > MAX_CHARS) {
    len= MAX_CHARS;
  }
  offset+= 12;
  count= 0;
  
  /* grab the model name */
  while (len > 0) {
    err=GetCameraROMValue(camera, offset+count, &value[0]);
    DC1394_ERR_CHK(err, "Could not get model name character");
    
    camera->model[count++]= (value[0] >> 24);
    camera->model[count++]= (value[0] >> 16) & 0xFFUL;
    camera->model[count++]= (value[0] >> 8) & 0xFFUL;
    camera->model[count++]= value[0] & 0xFFUL;
    len-= 4;

    camera->model[count]= '\0';
  }
  //fprintf(stderr,"test 1\n");
  
  err=dc1394_get_iso_channel_and_speed(camera, &camera->iso_channel, &camera->iso_speed);
  DC1394_ERR_CHK(err, "Could not get ISO channel and speed");
  
  err=dc1394_get_video_format(camera, &camera->format);
  DC1394_ERR_CHK(err, "Could not get current video format");
  
  err=dc1394_get_video_mode(camera, &camera->mode);
  DC1394_ERR_CHK(err, "Could not get current video mode");
  
  err=dc1394_get_video_framerate(camera, &camera->framerate);
  DC1394_ERR_CHK(err, "Could not get current video framerate");
  
  err=dc1394_get_iso_status(camera, &camera->is_iso_on);
  DC1394_ERR_CHK(err, "Could not get ISO status");
  
  err=dc1394_query_basic_functionality(camera, &value[0]);
  DC1394_ERR_CHK(err, "Could not get basic functionalities");

  camera->mem_channel_number = (value[0] & 0x0000000F) != 0;
  camera->bmode_capable      = (value[0] & 0x00800000) != 0;
  camera->one_shot_capable   = (value[0] & 0x00000800) != 0;
  camera->multi_shot_capable = (value[0] & 0x00001000) != 0;
  camera->adv_features_capable = (value[0] & 0x80000000) != 0;

  //fprintf(stderr,"0x%lx\n",value[0]);

  if (camera->adv_features_capable>0) {
    //fprintf(stderr,"test\n");
    
    // get advanced features CSR
    err=GetCameraControlRegister(camera,REG_CAMERA_ADV_FEATURE_INQ, &quadval);
    DC1394_ERR_CHK(err, "Could not get advanced features CSR");
    camera->advanced_features_csr= (octlet_t)(quadval & 0xFFFFFFUL)*4;
    
    //fprintf(stderr,"test\n");
  }

  if (camera->mem_channel_number>0) {
    err=dc1394_get_memory_load_ch(camera, &camera->load_channel);
    DC1394_ERR_CHK(err, "Could not get current load memory channel");
    
    err=dc1394_get_memory_save_ch(camera, &camera->save_channel);
    DC1394_ERR_CHK(err, "COuld not get current save memory channel");
  }
  else {
    camera->load_channel=0;
    camera->save_channel=0;
  }
  
  return err;
}

/*****************************************************
 dc1394_get_camera_feature_set

 Collects the available features for the camera
 described by node and stores them in features.
*****************************************************/
int
dc1394_get_camera_feature_set(dc1394camera_t *camera, dc1394featureset_t *features) 
{
  int i, j, err=DC1394_SUCCESS;
  
  for (i= FEATURE_MIN, j= 0; i <= FEATURE_MAX; i++, j++)  {
    features->feature[j].feature_id= i;
    err=dc1394_get_camera_feature(camera, &features->feature[j]);
    DC1394_ERR_CHK(err, "Could not get camera feature %d",i);
  }

  return DC1394_SUCCESS;
}

/*****************************************************
 dc1394_get_camera_feature

 Stores the bounds and options associated with the
 feature described by feature->feature_id
*****************************************************/
int
dc1394_get_camera_feature(dc1394camera_t *camera, dc1394feature_t *feature) 
{
  octlet_t offset;
  quadlet_t value;
  unsigned int orig_fid, updated_fid;
  int err;
  
  orig_fid= feature->feature_id;
  updated_fid= feature->feature_id;
  
  // check presence
  err=dc1394_is_feature_present(camera, feature->feature_id, &(feature->available));
  DC1394_ERR_CHK(err, "Could not check feature %d presence",feature->feature_id);
  
  if (feature->available == DC1394_FALSE) {
    return DC1394_SUCCESS;
  }

  // get capabilities
  err=dc1394_query_feature_characteristics(camera, feature->feature_id, &value);
  DC1394_ERR_CHK(err, "Could not check feature %d characteristics",feature->feature_id);

  switch (feature->feature_id) {
  case FEATURE_TRIGGER:
    feature->one_push= DC1394_FALSE;
    feature->polarity_capable=
      (value & 0x02000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->trigger_mode_capable_mask= ((value >> 12) & 0x0f);
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
  updated_fid= orig_fid;
  FEATURE_TO_VALUE_OFFSET(updated_fid, offset);
  
  err=GetCameraControlRegister(camera, offset, &value);
  DC1394_ERR_CHK(err, "Could not get feature register");
  
  switch (feature->feature_id) {
  case FEATURE_TRIGGER:
    feature->one_push_active= DC1394_FALSE;
    feature->trigger_polarity=
      (value & 0x01000000UL) ? DC1394_TRUE : DC1394_FALSE;
    feature->trigger_mode= (int)((value >> 14) & 0xF);
    feature->auto_active= DC1394_FALSE;
    break;
  case FEATURE_TRIGGER_DELAY:
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
  
  switch (orig_fid) {
  case FEATURE_WHITE_BALANCE:
    feature->RV_value= value & 0xFFFUL;
    feature->BU_value= (value & 0xFFF000UL) >> 12;
    break;
  case FEATURE_WHITE_SHADING:
    feature->R_value=value & 0xFFUL;
    feature->G_value=(value & 0xFF00UL)>>8;
    feature->B_value=(value & 0xFF0000UL)>>16;
    break;
  case FEATURE_TEMPERATURE:
    feature->value= value & 0xFFFUL;
    feature->target_value= value & 0xFFF000UL;
    break;
  default:
    feature->value= value & 0xFFFUL;
    break;
  }
  
  if (feature->absolute_capable>0) {
    err=dc1394_query_absolute_feature_min_max(camera, orig_fid, &feature->abs_min, &feature->abs_max);
    DC1394_ERR_CHK(err, "Could not get feature %d absolute min/max",orig_fid);
    err=dc1394_query_absolute_feature_value(camera, orig_fid, &feature->abs_value);
    DC1394_ERR_CHK(err, "Could not get feature %d absolute value",orig_fid);
    err=dc1394_query_absolute_control(camera, orig_fid, &feature->abs_control);
    DC1394_ERR_CHK(err, "Could not get feature %d absolute control",orig_fid);
  }
  
  return err;
}

/*****************************************************
 dc1394_print_feature

 Displays the bounds and options of the given feature
*****************************************************/
int
dc1394_print_feature(dc1394feature_t *f) 
{
  //char * tags[2] = {"NO","YES"};
  //char * autoMode[2] = {"Manual","Auto"};
  int fid= f->feature_id;
  
  if ( (fid < FEATURE_MIN) || (fid > FEATURE_MAX) ) {
    printf("Invalid feature code\n");
    return DC1394_FAILURE;
  }
  
  printf("%s:\n\t", dc1394_feature_desc[fid - FEATURE_MIN]);
  
  if (!f->available) {
    printf("NOT AVAILABLE\n");
    return DC1394_FAILURE;
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

  if (fid != FEATURE_TRIGGER) 
    printf("min: %d max %d\n", f->min, f->max);

  switch(fid) {
  case FEATURE_TRIGGER:
    printf("\n\tAvailableTriggerModes: ");
    
    if (f->trigger_mode_capable_mask & 0x08)
      printf("0 ");
    if (f->trigger_mode_capable_mask & 0x04)
      printf("1 ");
    if (f->trigger_mode_capable_mask & 0x02)
      printf("2 ");
    if (f->trigger_mode_capable_mask & 0x02)
      printf("3 ");
    if (!(f->trigger_mode_capable_mask & 0x0f))
      printf("No modes available");
      
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
    break;
  case FEATURE_WHITE_BALANCE: 
    printf("\tB/U value: %d R/V value: %d\n", f->BU_value, f->RV_value);
    break;
  case FEATURE_TEMPERATURE:
    printf("\tTarget temp: %d Current Temp: %d\n", f->target_value, f->value);
    break;
  case FEATURE_WHITE_SHADING: 
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
int
dc1394_print_feature_set(dc1394featureset_t *features) 
{
  int i, j, err=DC1394_SUCCESS;
  
  printf("FEATURE SETTINGS\n==================================\n");
  printf("OP- one push capable\n");
  printf("RC- readout capable\n");
  printf("O/OC- on/off capable\n");
  printf("AC- auto capable\n");
  printf("MC- manual capable\n");
  printf("ABS- absolute capable\n");
  printf("==================================\n");
  
  for (i= FEATURE_MIN, j= 0; i <= FEATURE_MAX; i++, j++)  {
    err=dc1394_print_feature(&features->feature[j]);
    DC1394_ERR_CHK(err, "Could not print feature %d",i);
  }
  
  return err;
}

int
dc1394_reset_camera(dc1394camera_t *camera)
{
  int err;
  err=SetCameraControlRegister(camera, REG_CAMERA_INITIALIZE, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not reset the camera");
  return err;
}

int
dc1394_query_supported_formats(dc1394camera_t *camera, quadlet_t *value)
{
  int err;
  err=GetCameraControlRegister(camera, REG_CAMERA_V_FORMAT_INQ, value);
  DC1394_ERR_CHK(err, "Could not get supported formats");
  return err;
}

int
dc1394_query_supported_modes(dc1394camera_t *camera, unsigned int format, quadlet_t *value)
{
  int err;
  
  if ( (format > FORMAT_MAX) || (format < FORMAT_MIN) ) {
    return DC1394_FAILURE;
  }
  
  format-= FORMAT_MIN;
  err=GetCameraControlRegister(camera, REG_CAMERA_V_MODE_INQ_BASE + (format * 0x04U), value);
  DC1394_ERR_CHK(err, "Could not get supported modes");
  return err;
}

int
dc1394_query_supported_framerates(dc1394camera_t *camera, unsigned int format,
				  unsigned int mode, quadlet_t *value)
{
  int max_mode_for_format, min_mode_for_format, err;
  
  switch(format) {
  case FORMAT_VGA_NONCOMPRESSED:
    min_mode_for_format= MODE_FORMAT0_MIN;
    max_mode_for_format= MODE_FORMAT0_MAX;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_1:
    min_mode_for_format= MODE_FORMAT1_MIN;
    max_mode_for_format= MODE_FORMAT1_MAX;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_2:
    min_mode_for_format= MODE_FORMAT2_MIN;
    max_mode_for_format= MODE_FORMAT2_MAX;
    break;
  default:
    printf("Invalid format query\n");
    return DC1394_FAILURE;
  }
  
  if ( (format > FORMAT_MAX) || (format < FORMAT_MIN) ||
       (mode > max_mode_for_format) || (mode < min_mode_for_format) ) {
    return DC1394_FAILURE;
  }
  
  format-= FORMAT_MIN;
  mode-= min_mode_for_format;
  err=GetCameraControlRegister(camera,REG_CAMERA_V_RATE_INQ_BASE + (format * 0x20U) + (mode * 0x04U), value);
  DC1394_ERR_CHK(err, "Could not get supported framerates");

  return err;
}

int
dc1394_query_revision(dc1394camera_t *camera, int mode, quadlet_t *value)
{
  int err;
  if ( (mode > MODE_FORMAT6_MAX) || (mode < MODE_FORMAT6_MIN) ) {
    return DC1394_FAILURE;
  }
  
  mode-= MODE_FORMAT6_MIN;
  err=GetCameraControlRegister(camera, REG_CAMERA_V_REV_INQ_BASE + (mode * 0x04U), value);
  DC1394_ERR_CHK(err, "Could not get revision");

  return err;
}

int
dc1394_query_basic_functionality(dc1394camera_t *camera, quadlet_t *value)
{
  int err;
  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, value);
  DC1394_ERR_CHK(err, "Could not get basic functionalities");

  return err;
}

int
dc1394_query_advanced_feature_offset(dc1394camera_t *camera, quadlet_t *value)
{
  int err;
  err=GetCameraControlRegister(camera, REG_CAMERA_ADV_FEATURE_INQ, value);
  DC1394_ERR_CHK(err, "Could not get the advanced features offset");

  return err;
}

int
dc1394_query_feature_characteristics(dc1394camera_t *camera, unsigned int feature, quadlet_t *value)
{
  octlet_t offset;
  int err;

  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  err=GetCameraControlRegister(camera, offset, value);
  DC1394_ERR_CHK(err, "Could not get feature characteristics");

  return err;
}

int
dc1394_get_video_framerate(dc1394camera_t *camera, unsigned int *framerate)
{
  quadlet_t value;
  int err;

  err=GetCameraControlRegister(camera, REG_CAMERA_FRAME_RATE, &value);
  DC1394_ERR_CHK(err, "Could not get video framerate");
  
  *framerate= (unsigned int)((value >> 29) & 0x7UL) + FRAMERATE_MIN;
  
  return err;
}

int
dc1394_set_video_framerate(dc1394camera_t *camera, unsigned int framerate)
{ 
  int err;
  if ( (framerate < FRAMERATE_MIN) || (framerate > FRAMERATE_MAX) ) {
    return DC1394_FAILURE;
  }
  
  err=SetCameraControlRegister(camera, REG_CAMERA_FRAME_RATE,
			       (quadlet_t)(((framerate - FRAMERATE_MIN) & 0x7UL) << 29));
  DC1394_ERR_CHK(err, "Could not set video framerate");

  return err;
}

int
dc1394_get_video_mode(dc1394camera_t *camera, unsigned int *mode)
{
  int err;
  quadlet_t value;
  unsigned int format;
  
  err=dc1394_get_video_format(camera, &format);
  DC1394_ERR_CHK(err, "Could not get video format");
  
  err= GetCameraControlRegister(camera, REG_CAMERA_VIDEO_MODE, &value);
  DC1394_ERR_CHK(err, "Could not get video mode");
  
  switch(format) {
  case FORMAT_VGA_NONCOMPRESSED:
    *mode= (unsigned int)((value >> 29) & 0x7UL) + MODE_FORMAT0_MIN;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_1:
    *mode= (unsigned int)((value >> 29) & 0x7UL) + MODE_FORMAT1_MIN;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_2:
    *mode= (unsigned int)((value >> 29) & 0x7UL) + MODE_FORMAT2_MIN;
    break;
  case FORMAT_STILL_IMAGE:
    *mode= (unsigned int)((value >> 29) & 0x7UL) + MODE_FORMAT6_MIN;
    break;
  case FORMAT_SCALABLE_IMAGE_SIZE:
    *mode= (unsigned int)((value >> 29) & 0x7UL) + MODE_FORMAT7_MIN;
    break;
  default:
    return DC1394_FAILURE;
    break;
  }
  
  return err;
}

int
dc1394_set_video_mode(dc1394camera_t *camera, unsigned int mode)
{
  unsigned int format, min, max, err;
  
  err=dc1394_get_video_format(camera, &format);
  DC1394_ERR_CHK(err, "Could not get video format");
  
  switch(format) {
  case FORMAT_VGA_NONCOMPRESSED:
    min= MODE_FORMAT0_MIN;
    max= MODE_FORMAT0_MAX;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_1:
    min= MODE_FORMAT1_MIN;
    max= MODE_FORMAT1_MAX;
    break;
  case FORMAT_SVGA_NONCOMPRESSED_2:
    min= MODE_FORMAT2_MIN;
    max= MODE_FORMAT2_MAX;
    break;
  case FORMAT_STILL_IMAGE:
    min= MODE_FORMAT6_MIN;
    max= MODE_FORMAT6_MAX;
    break;
  case FORMAT_SCALABLE_IMAGE_SIZE:
    min= MODE_FORMAT7_MIN;
    max= MODE_FORMAT7_MAX;
    break;
  default:
    return DC1394_FAILURE;
    break;
  }
  
  if ( (mode < min) || (mode > max) ) {
    return DC1394_FAILURE;
  }
  
  err=SetCameraControlRegister(camera, REG_CAMERA_VIDEO_MODE, (quadlet_t)(((mode - min) & 0x7UL) << 29));
  DC1394_ERR_CHK(err, "Could not set video mode");

  return err;
  
}

int
dc1394_get_video_format(dc1394camera_t *camera, unsigned int *format)
{
  quadlet_t value;
  int err;

  err= GetCameraControlRegister(camera, REG_CAMERA_VIDEO_FORMAT, &value);
  DC1394_ERR_CHK(err, "Could not get video format");

  *format= (unsigned int)((value >> 29) & 0x7UL) + FORMAT_MIN;
  
  return err;
}

int
dc1394_set_video_format(dc1394camera_t *camera, unsigned int format)
{
  int err;
  
  if ( (format < FORMAT_MIN) || (format > FORMAT_MAX) ) {
    return DC1394_FAILURE;
  }
  
  err=SetCameraControlRegister(camera, REG_CAMERA_VIDEO_FORMAT, (quadlet_t)(((format - FORMAT_MIN) & 0x7UL) << 29));
  DC1394_ERR_CHK(err, "Could not set video format");

  return err;
}

int
dc1394_get_iso_channel_and_speed(dc1394camera_t *camera,
				 unsigned int *channel, unsigned int *speed)
{
  int err;
  quadlet_t value_inq, value;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
  DC1394_ERR_CHK(err, "Could not get basic func register");
  
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_CHK(err, "Could not get ISO data");

  if (value_inq & 0x00800000) { // check if 1394b is available
    if (value & 0x00008000) { //check if we are now using 1394b
      *channel= (unsigned int)((value >> 8) & 0x3FUL);
      *speed= (unsigned int)(value& 0x7UL);
    }
    else { // fallback to legacy
      *channel= (unsigned int)((value >> 28) & 0xFUL);
      *speed= (unsigned int)((value >> 24) & 0x3UL);
    }
  }
  else { // legacy
    *channel= (unsigned int)((value >> 28) & 0xFUL);
    *speed= (unsigned int)((value >> 24) & 0x3UL);
  }
  
  return err;
}

int
dc1394_set_iso_channel_and_speed(dc1394camera_t *camera,
                                 unsigned int channel, unsigned int speed)
{
  int err;
  quadlet_t value_inq, value;

  err=GetCameraControlRegister(camera, REG_CAMERA_BASIC_FUNC_INQ, &value_inq);
  DC1394_ERR_CHK(err, "Could not get basic function register");

  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_CHK(err, "Could not get ISO data");

  // check if 1394b is available and if we are now using 1394b
  if ((value_inq & 0x00800000)&&(value & 0x00008000)) {
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (quadlet_t) ( ((channel & 0x3FUL) << 8) |
					       (speed & 0x7UL) |
					       (0x1 << 15) ));
      DC1394_ERR_CHK(err, " ");
  }
  else { // fallback to legacy
    if (speed>SPEED_400) {
      fprintf(stderr,"(%s) line %d: an ISO speed >400Mbps was requested while the camera is in LEGACY mode\n",__FILE__,__LINE__);
      fprintf(stderr,"              Please set the operation mode to OPERATION_MODE_1394B before asking for\n");
      fprintf(stderr,"              1394b ISO speeds\n");
      return DC1394_FAILURE;
    }
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (quadlet_t) (((channel & 0xFUL) << 28) |
					      ((speed & 0x3UL) << 24) ));
      DC1394_ERR_CHK(err, "Could not set ISO data register");
  }
  
  return err;;
}

int
dc1394_get_operation_mode(dc1394camera_t *camera, unsigned int *mode)
{
  int err;
  quadlet_t value_inq, value;
  
  err=dc1394_query_basic_functionality(camera, &value_inq);
  DC1394_ERR_CHK(err, "Could not get basic functionalities");
  
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_CHK(err, "Could not get ISO data");
  
  if (value_inq & 0x00800000) {
    *mode=((value & 0x00008000) >0);
  }
  else {
    *mode=OPERATION_MODE_LEGACY;
  }
  
  return err;
}


int
dc1394_set_operation_mode(dc1394camera_t *camera, unsigned int mode)
{
  int err;
  quadlet_t value_inq, value;
  
  err=dc1394_query_basic_functionality(camera, &value_inq);
  DC1394_ERR_CHK(err, "Could not get basic functionalities");
  err=GetCameraControlRegister(camera, REG_CAMERA_ISO_DATA, &value);
  DC1394_ERR_CHK(err, "Could not get ISO data");
  
  if (mode==OPERATION_MODE_LEGACY) {
    err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				 (quadlet_t) (value & 0xFFFF7FFF));
    DC1394_ERR_CHK(err, "Could not set ISO data");
  }
  else { // 1394b
    if (value_inq & 0x00800000) { // if 1394b available
      err=SetCameraControlRegister(camera, REG_CAMERA_ISO_DATA,
				   (quadlet_t) (value | 0x00008000));
      DC1394_ERR_CHK(err, "Could not set ISO data");
    }
    else { // 1394b asked, but it is not available
      return DC1394_FUNCTION_NOT_SUPPORTED;
    }
  }
  
  return DC1394_SUCCESS;
  
}

int
dc1394_camera_on(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_POWER, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not switch camera ON");
  return err;
}

int
dc1394_camera_off(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_POWER, OFF_VALUE);
  DC1394_ERR_CHK(err, "Could not switch camera OFF");
  return err;
}

int
dc1394_start_iso_transmission(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_ISO_EN, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not start ISO transmission");
  return err;
}

int
dc1394_stop_iso_transmission(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_ISO_EN, OFF_VALUE);
  DC1394_ERR_CHK(err, "Could not stop ISO transmission");
  return err;
}

int
dc1394_get_iso_status(dc1394camera_t *camera, dc1394bool_t *is_on)
{
  int err;
  quadlet_t value;
  err= GetCameraControlRegister(camera, REG_CAMERA_ISO_EN, &value);
  DC1394_ERR_CHK(err, "Could not get ISO status");
  
  *is_on= (value & ON_VALUE)>>31;
  return err;
}

int
dc1394_set_one_shot(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not set one-shot");
  return err;
}

int
dc1394_unset_one_shot(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, OFF_VALUE);
  DC1394_ERR_CHK(err, "Could not unset one-shot");
  return err;
}

int
dc1394_get_one_shot(dc1394camera_t *camera, dc1394bool_t *is_on)
{
  quadlet_t value;
  int err = GetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, &value);
  DC1394_ERR_CHK(err, "Could not get one-shot status");
  *is_on = value & ON_VALUE;
  return err;
}

int
dc1394_get_multi_shot(dc1394camera_t *camera,
		      dc1394bool_t *is_on, unsigned int *numFrames)
{
  quadlet_t value;
  int err = GetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT, &value);
  DC1394_ERR_CHK(err, "Could not get multishot status");
  *is_on = (value & (ON_VALUE>>1)) >> 30;
  *numFrames= value & 0xFFFFUL;
  
  return err;  
}

int
dc1394_set_multi_shot(dc1394camera_t *camera, unsigned int numFrames)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_ONE_SHOT,
				   (0x40000000UL | (numFrames & 0xFFFFUL)));
  DC1394_ERR_CHK(err, "Could not set multishot");
  return err;
}

int
dc1394_unset_multi_shot(dc1394camera_t *camera)
{
  int err=dc1394_unset_one_shot(camera);
  DC1394_ERR_CHK(err, "Could not unset multishot");
  return err;
}

int
dc1394_get_brightness(dc1394camera_t *camera, unsigned int *brightness)
{
  int err=GetFeatureValue(camera, FEATURE_BRIGHTNESS, brightness);
  DC1394_ERR_CHK(err, "Could not get brightness");
  return err;
}

int
dc1394_set_brightness(dc1394camera_t *camera, unsigned int brightness)
{
  int err=SetFeatureValue(camera, FEATURE_BRIGHTNESS, brightness);
  DC1394_ERR_CHK(err, "Could not set brightness");
  return err;
}

int
dc1394_get_exposure(dc1394camera_t *camera, unsigned int *exposure)
{
  int err=GetFeatureValue(camera, FEATURE_EXPOSURE, exposure);
  DC1394_ERR_CHK(err, "Could not get exposure");
  return err;
}

int
dc1394_set_exposure(dc1394camera_t *camera, unsigned int exposure)
{
  int err=SetFeatureValue(camera, FEATURE_EXPOSURE, exposure);
  DC1394_ERR_CHK(err, "Could not set exposure");
  return err;
}

int
dc1394_get_sharpness(dc1394camera_t *camera, unsigned int *sharpness)
{
  int err=GetFeatureValue(camera, FEATURE_SHARPNESS, sharpness);
  DC1394_ERR_CHK(err, "Could not get sharpness");
  return err;
}

int
dc1394_set_sharpness(dc1394camera_t *camera, unsigned int sharpness)
{
  int err=SetFeatureValue(camera, FEATURE_SHARPNESS, sharpness);
  DC1394_ERR_CHK(err, "Could not set sharpness");
  return err;
}

int
dc1394_get_white_balance(dc1394camera_t *camera,
                         unsigned int *u_b_value, unsigned int *v_r_value)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, &value);
  DC1394_ERR_CHK(err, "Could not get white balance");

  *u_b_value= (unsigned int)((value & 0xFFF000UL) >> 12);
  *v_r_value= (unsigned int)(value & 0xFFFUL);
  return err;
}

int
dc1394_set_white_balance(dc1394camera_t *camera,
                         unsigned int u_b_value, unsigned int v_r_value)
{
  quadlet_t curval;
  int err;
  err=GetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, &curval);
  DC1394_ERR_CHK(err, "Could not get white balance");
  
  curval= (curval & 0xFF000000UL) | ( ((u_b_value & 0xFFFUL) << 12) |
				      (v_r_value & 0xFFFUL) );
  err=SetCameraControlRegister(camera, REG_CAMERA_WHITE_BALANCE, curval);
  DC1394_ERR_CHK(err, "Could not set white balance");
  return err;
}

int
dc1394_get_hue(dc1394camera_t *camera, unsigned int *hue)
{
  int err=GetFeatureValue(camera, FEATURE_HUE, hue);
  DC1394_ERR_CHK(err, "Could not get hue");
  return err;
}

int
dc1394_set_hue(dc1394camera_t *camera, unsigned int hue)
{
  int err=SetFeatureValue(camera, FEATURE_HUE, hue);
  DC1394_ERR_CHK(err, "Could not set hue");
  return err;
}

int
dc1394_get_saturation(dc1394camera_t *camera, unsigned int *saturation)
{
  int err=GetFeatureValue(camera, FEATURE_SATURATION, saturation);
  DC1394_ERR_CHK(err, "Could not get saturation");
  return err;
}

int
dc1394_set_saturation(dc1394camera_t *camera, unsigned int saturation)
{
  int err=SetFeatureValue(camera, FEATURE_SATURATION, saturation);
  DC1394_ERR_CHK(err, "Could not set saturation");
  return err;
}

int
dc1394_get_gamma(dc1394camera_t *camera, unsigned int *gamma)
{
  int err=GetFeatureValue(camera, FEATURE_GAMMA, gamma);
  DC1394_ERR_CHK(err, "Could not get gamma");
  return err;
}

int
dc1394_set_gamma(dc1394camera_t *camera, unsigned int gamma)
{
  int err=SetFeatureValue(camera, FEATURE_GAMMA, gamma);
  DC1394_ERR_CHK(err, "Could not set gamma");
  return err;
}

int
dc1394_get_shutter(dc1394camera_t *camera, unsigned int *shutter)
{
  int err=GetFeatureValue(camera, FEATURE_SHUTTER, shutter);
  DC1394_ERR_CHK(err, "Could not get shutter");
  return err;
}

int
dc1394_set_shutter(dc1394camera_t *camera, unsigned int shutter)
{
  int err=SetFeatureValue(camera, FEATURE_SHUTTER, shutter);
  DC1394_ERR_CHK(err, "Could not set shutter");
  return err;
}

int
dc1394_get_gain(dc1394camera_t *camera, unsigned int *gain)
{
  int err=GetFeatureValue(camera, FEATURE_GAIN, gain);
  DC1394_ERR_CHK(err, "Could not get gain");
  return err;
}

int
dc1394_set_gain(dc1394camera_t *camera, unsigned int gain)
{
  int err=SetFeatureValue(camera, FEATURE_GAIN, gain);
  DC1394_ERR_CHK(err, "Could not set gain");
  return err;
}

int
dc1394_get_iris(dc1394camera_t *camera, unsigned int *iris)
{
  int err=GetFeatureValue(camera, FEATURE_IRIS, iris);
  DC1394_ERR_CHK(err, "Could not get iris");
  return err;
}

int
dc1394_set_iris(dc1394camera_t *camera, unsigned int iris)
{
  int err=SetFeatureValue(camera, FEATURE_IRIS, iris);
  DC1394_ERR_CHK(err, "Could not set iris");
  return err;
}

int
dc1394_get_focus(dc1394camera_t *camera, unsigned int *focus)
{
  int err=GetFeatureValue(camera, FEATURE_FOCUS, focus);
  DC1394_ERR_CHK(err, "Could not get focus");
  return err;
}

int
dc1394_set_focus(dc1394camera_t *camera, unsigned int focus)
{
  int err=SetFeatureValue(camera, FEATURE_FOCUS, focus);
  DC1394_ERR_CHK(err, "Could not set focus");
  return err;
}

int
dc1394_get_temperature(dc1394camera_t *camera,
                       unsigned int *target_temperature, unsigned int *temperature)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, &value);
  DC1394_ERR_CHK(err, "Could not get temperature");
  *target_temperature= (unsigned int)((value >> 12) & 0xFFF);
  *temperature= (unsigned int)(value & 0xFFFUL);
  return err;
}

int
dc1394_set_temperature(dc1394camera_t *camera, unsigned int target_temperature)
{
  int err;
  quadlet_t curval;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, &curval);
  DC1394_ERR_CHK(err, "Could not get temperature");
  
  curval= (curval & 0xFF000FFFUL) | ((target_temperature & 0xFFFUL) << 12);
  err= SetCameraControlRegister(camera, REG_CAMERA_TEMPERATURE, curval);
  DC1394_ERR_CHK(err, "Could not set temperature");

  return err;
}

int
dc1394_get_white_shading(dc1394camera_t *camera,
                         unsigned int *r_value,
			 unsigned int *g_value,
			 unsigned int *b_value)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, &value);
  DC1394_ERR_CHK(err, "Could not get white shading");
  
  *r_value= (unsigned int)((value & 0xFF0000UL) >> 16);
  *g_value= (unsigned int)((value & 0xFF00UL) >> 8);
  *b_value= (unsigned int)(value & 0xFFUL);

  return err;
}

int
dc1394_set_white_shading(dc1394camera_t *camera,
			 unsigned int r_value,
			 unsigned int g_value,
			 unsigned int b_value)

{
  quadlet_t curval;
  
  int err=GetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, &curval);
  DC1394_ERR_CHK(err, "Could not get white shading");
  
  curval= (curval & 0xFF000000UL) | ( ((r_value & 0xFFUL) << 16) |
				      ((g_value & 0xFFUL) << 8) |
				      (b_value & 0xFFUL) );
  err=SetCameraControlRegister(camera, REG_CAMERA_WHITE_SHADING, curval);
  DC1394_ERR_CHK(err, "Could not set white shading");

  return err;
}

int
dc1394_get_trigger_delay(dc1394camera_t *camera, unsigned int *trigger_delay)
{
  int err=GetFeatureValue(camera, FEATURE_TRIGGER_DELAY, trigger_delay);
  DC1394_ERR_CHK(err, "Could not get trigger delay");
  return err;
}

int
dc1394_set_trigger_delay(dc1394camera_t *camera, unsigned int trigger_delay)
{
  int err=SetFeatureValue(camera, FEATURE_TRIGGER_DELAY, trigger_delay);
  DC1394_ERR_CHK(err, "Could not set trigger delay");
  return err;
}

int
dc1394_get_frame_rate(dc1394camera_t *camera, unsigned int *frame_rate)
{
  int err=GetFeatureValue(camera, FEATURE_FRAME_RATE, frame_rate);
  DC1394_ERR_CHK(err, "Could not get frame rate feature");
  return err;
}

int
dc1394_set_frame_rate(dc1394camera_t *camera, unsigned int frame_rate)
{
  int err=SetFeatureValue(camera, FEATURE_FRAME_RATE, frame_rate);
  DC1394_ERR_CHK(err, "Could not set frame rate feature");
  return err;
}

int
dc1394_get_trigger_mode(dc1394camera_t *camera, unsigned int *mode)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &value);
  DC1394_ERR_CHK(err, "Could not get trigger mode");
  
  *mode= (unsigned int)( ((value >> 16) & 0xFUL) ) + TRIGGER_MODE_MIN;
  return err;
}

int
dc1394_set_trigger_mode(dc1394camera_t *camera, unsigned int mode)
{
  int err;
  quadlet_t curval;
  
  if ( (mode < TRIGGER_MODE_MIN) || (mode > TRIGGER_MODE_MAX) ) {
    return DC1394_FAILURE;
  }
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &curval);
  DC1394_ERR_CHK(err, "Could not get trigger mode");
  
  mode-= TRIGGER_MODE_MIN;
  curval= (curval & 0xFFF0FFFFUL) | ((mode & 0xFUL) << 16);
  err=SetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, curval);
  DC1394_ERR_CHK(err, "Could not set trigger mode");
  return err;
}

int
dc1394_get_zoom(dc1394camera_t *camera, unsigned int *zoom)
{
  int err= GetFeatureValue(camera, FEATURE_ZOOM, zoom);
  DC1394_ERR_CHK(err, "Could not get zoom");
  return err;
}

int
dc1394_set_zoom(dc1394camera_t *camera, unsigned int zoom)
{
  int err= SetFeatureValue(camera, FEATURE_ZOOM, zoom);
  DC1394_ERR_CHK(err, "Could not set zoom");
  return err;
}

int
dc1394_get_pan(dc1394camera_t *camera, unsigned int *pan)
{
  int err= GetFeatureValue(camera, FEATURE_PAN, pan);
  DC1394_ERR_CHK(err, "Could not get pan");
  return err;
}

int
dc1394_set_pan(dc1394camera_t *camera, unsigned int pan)
{
  int err= SetFeatureValue(camera, FEATURE_PAN, pan);
  DC1394_ERR_CHK(err, "Could not set pan");
  return err;
}

int
dc1394_get_tilt(dc1394camera_t *camera, unsigned int *tilt)
{
  int err= GetFeatureValue(camera, FEATURE_TILT, tilt);
  DC1394_ERR_CHK(err, "Could not get tilt");
  return err;
}

int
dc1394_set_tilt(dc1394camera_t *camera, unsigned int tilt)
{
  int err= SetFeatureValue(camera, FEATURE_TILT, tilt);
  DC1394_ERR_CHK(err, "Could not set tilt");
  return err;
}

int
dc1394_get_optical_filter(dc1394camera_t *camera, unsigned int *optical_filter)
{
  int err= GetFeatureValue(camera, FEATURE_OPTICAL_FILTER, optical_filter);
  DC1394_ERR_CHK(err, "Could not get optical filter");
  return err;
}

int
dc1394_set_optical_filter(dc1394camera_t *camera, unsigned int optical_filter)
{
  int err= SetFeatureValue(camera, FEATURE_OPTICAL_FILTER, optical_filter);
  DC1394_ERR_CHK(err, "Could not set optical filter");
  return err;
}

int
dc1394_get_capture_size(dc1394camera_t *camera, unsigned int *capture_size)
{
  int err= GetFeatureValue(camera, FEATURE_CAPTURE_SIZE, capture_size);
  DC1394_ERR_CHK(err, "Could not get capture size");
  return err;
}

int
dc1394_set_capture_size(dc1394camera_t *camera, unsigned int capture_size)
{
  int err= SetFeatureValue(camera, FEATURE_CAPTURE_SIZE, capture_size);
  DC1394_ERR_CHK(err, "Could not set capture size");
  return err;
}

int
dc1394_get_capture_quality(dc1394camera_t *camera, unsigned int *capture_quality)
{
  int err= GetFeatureValue(camera, FEATURE_CAPTURE_QUALITY, capture_quality);
  DC1394_ERR_CHK(err, "Could not get capture quality");
  return err;
}

int
dc1394_set_capture_quality(dc1394camera_t *camera, unsigned int capture_quality)
{
  int err= SetFeatureValue(camera, FEATURE_CAPTURE_QUALITY, capture_quality);
  DC1394_ERR_CHK(err, "Could not set capture quality");
  return err;
}

int
dc1394_get_feature_value(dc1394camera_t *camera,
			 unsigned int feature, unsigned int *value)
{
  int err= GetFeatureValue(camera, feature, value);
  DC1394_ERR_CHK(err, "Could not get feature %d value",feature);
  return err;
}

int
dc1394_set_feature_value(dc1394camera_t *camera,
                         unsigned int feature, unsigned int value)
{
  int err= SetFeatureValue(camera, feature, value);
  DC1394_ERR_CHK(err, "Could not set feature %d value",feature);
  return err;
}

int
dc1394_is_feature_present(dc1394camera_t *camera,
                          unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  *value=DC1394_FALSE;
  
  // check feature presence in 0x404 and 0x408
  if ( (feature > FEATURE_MAX) || (feature < FEATURE_MIN) ) {
    return DC1394_FAILURE;
  }
  
  if (feature < FEATURE_ZOOM) {
    offset= REG_CAMERA_FEATURE_HI_INQ;
  }
  else {
    offset= REG_CAMERA_FEATURE_LO_INQ;
  }

  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get register for feature %d",feature);
  
  if (IsFeatureBitSet(quadval, feature)!=DC1394_TRUE) {
    *value=DC1394_FALSE;
    return DC1394_SUCCESS;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get register for feature %d",feature);
  
  if (quadval & 0x80000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_has_one_push_auto(dc1394camera_t *camera,
                         unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get one-push capability for feature %d",feature);
  
  if (quadval & 0x10000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_is_one_push_in_operation(dc1394camera_t *camera,
                                unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get one-push capability for feature %d",feature);
  
  if (quadval & 0x04000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_start_one_push_operation(dc1394camera_t *camera, unsigned int feature)
{
  octlet_t offset;
  quadlet_t curval;
  int err;

  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_CHK(err, "Could not get one-push status for feature %d",feature);

  if (!(curval & 0x04000000UL)) {
    curval|= 0x04000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not sart one-push capability for feature %d",feature);
  }
  
  return err;
}

int
dc1394_can_read_out(dc1394camera_t *camera,
                    unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get read-out capability for feature %d",feature);
  
  if (quadval & 0x08000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_can_turn_on_off(dc1394camera_t *camera,
                       unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get power capability for feature %d",feature);
  
  if (quadval & 0x04000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_is_feature_on(dc1394camera_t *camera,
                     unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get feature %d status",feature);
  
  if (quadval & 0x02000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_feature_on_off(dc1394camera_t *camera,
                      unsigned int feature, unsigned int value)
{
  int err;
  octlet_t offset;
  quadlet_t curval;

  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_CHK(err, "Could not get feature %d register",feature);
  
  if (value && !(curval & 0x02000000UL)) {    
    curval|= 0x02000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set feature %d power",feature);
  }
  else if (!value && (curval & 0x02000000UL)) {
    curval&= 0xFDFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set feature %d power",feature);
  }
  
  return err;
}

int
dc1394_has_auto_mode(dc1394camera_t *camera,
                     unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get automode capability for feature %d",feature);
  
  if (quadval & 0x02000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_has_manual_mode(dc1394camera_t *camera,
                       unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get manual modecapability for feature %d",feature);
  
  if (quadval & 0x01000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_is_feature_auto(dc1394camera_t *camera,
                       unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get feature %d auto status",feature);
  
  if (quadval & 0x01000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_auto_on_off(dc1394camera_t *camera,
                   unsigned int feature, unsigned int value)
{
  int err;
  octlet_t offset;
  quadlet_t curval;

  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_CHK(err, "Could not get feature %d register",feature);
  
  if (value && !(curval & 0x01000000UL)) {
    curval|= 0x01000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set auto mode for feature %d",feature);
  }
  else if (!value && (curval & 0x01000000UL)) {
    curval&= 0xFEFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set auto mode for feature %d",feature);
  }
  
  return err;
}

int
dc1394_get_min_value(dc1394camera_t *camera,
                     unsigned int feature, unsigned int *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get feature %d min value",feature);
  
  *value= (unsigned int)((quadval & 0xFFF000UL) >> 12);
  return err;
}

int
dc1394_get_max_value(dc1394camera_t *camera,
                     unsigned int feature, unsigned int *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  if (feature == FEATURE_TRIGGER) {
    return DC1394_FAILURE;
  }
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get feature %d max value",feature);
  
  *value= (unsigned int)(quadval & 0xFFFUL);
  
  return err;
}


/*
 * Memory load/save functions
 */
int
dc1394_get_memory_save_ch(dc1394camera_t *camera, unsigned int *channel)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_MEM_SAVE_CH, &value);
  DC1394_ERR_CHK(err, "Could not get memory save channel");
  *channel= (unsigned int)((value >> 28) & 0xFUL);
  return err;
}


int 
dc1394_get_memory_load_ch(dc1394camera_t *camera, unsigned int *channel)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_CUR_MEM_CH, &value);
  DC1394_ERR_CHK(err, "Could not get memory load channel");
  *channel= (unsigned int)((value >> 28) & 0xFUL);
  return err;
}


int 
dc1394_is_memory_save_in_operation(dc1394camera_t *camera, dc1394bool_t *value)
{
  quadlet_t quadlet;
  int err= GetCameraControlRegister(camera, REG_CAMERA_MEMORY_SAVE, &quadlet);
  DC1394_ERR_CHK(err, "Could not get memory busy status");
  *value = (quadlet & ON_VALUE) >> 31;
  return err;
}

int 
dc1394_set_memory_save_ch(dc1394camera_t *camera, unsigned int channel)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_MEM_SAVE_CH,
				  (quadlet_t)((channel & 0xFUL) << 28));
  DC1394_ERR_CHK(err, "Could not save memory channel");
  return err;

}

int
dc1394_memory_save(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_MEMORY_SAVE, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not save to memory");
  return err;
}

int
dc1394_memory_load(dc1394camera_t *camera, unsigned int channel)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_CUR_MEM_CH,
				  (quadlet_t)((channel & 0xFUL) << 28));
  DC1394_ERR_CHK(err, "Could not load from memory");
  return err;
}

/*
 * Trigger functions
 */

int
dc1394_set_trigger_polarity(dc1394camera_t *camera, dc1394bool_t polarity)
{
  int err;
  quadlet_t curval;
  
  err=GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &curval);
  DC1394_ERR_CHK(err, "Could not get trigger register");
  
  curval= (curval & 0xFEFFFFFFUL) | ((polarity & 0x1UL) << 24);
  err=SetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, curval);
  DC1394_ERR_CHK(err, "Could not set set trigger polarity");
  return err;
}

int
dc1394_get_trigger_polarity(dc1394camera_t *camera, dc1394bool_t *polarity)
{
  quadlet_t value;
  int err= GetCameraControlRegister(camera, REG_CAMERA_TRIGGER_MODE, &value);
  DC1394_ERR_CHK(err, "Could not get trigger polarity");
  
  *polarity= (unsigned int)( ((value >> 24) & 0x1UL) );
  return err;
}

int
dc1394_trigger_has_polarity(dc1394camera_t *camera, dc1394bool_t *polarity)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  offset= REG_CAMERA_FEATURE_HI_BASE_INQ;
  
  err=GetCameraControlRegister(camera, offset + ((FEATURE_TRIGGER - FEATURE_MIN) * 0x04U), &quadval);
  DC1394_ERR_CHK(err, "Could not get trigger polarity capability");
  
  if (quadval & 0x02000000UL) {
    *polarity= DC1394_TRUE;
  }
  else {
    *polarity= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_set_trigger_on_off(dc1394camera_t *camera, dc1394bool_t on_off)
{
  int err=dc1394_feature_on_off(camera, FEATURE_TRIGGER, on_off);
  DC1394_ERR_CHK(err, "Could not set external trigger");
  return err;
}

int
dc1394_get_trigger_on_off(dc1394camera_t *camera, dc1394bool_t *on_off)
{
  int err=dc1394_is_feature_on(camera, FEATURE_TRIGGER, on_off);
  DC1394_ERR_CHK(err, "Could not get external trigger status");
  return err;
}

int
dc1394_set_soft_trigger(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, ON_VALUE);
  DC1394_ERR_CHK(err, "Could not set software trigger");
  return err;
}

int
dc1394_unset_soft_trigger(dc1394camera_t *camera)
{
  int err=SetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, OFF_VALUE);
  DC1394_ERR_CHK(err, "Could not unset sofftware trigger");
  return err;
}

int
dc1394_get_soft_trigger(dc1394camera_t *camera, dc1394bool_t *is_on)
{
  quadlet_t value;
  int err = GetCameraControlRegister(camera, REG_CAMERA_SOFT_TRIGGER, &value);
  DC1394_ERR_CHK(err, "Could not get software trigger status");
  
  *is_on = value & ON_VALUE;
  return err;
}

int
dc1394_query_absolute_control(dc1394camera_t *camera,
                              unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get get abs control for feature %d",feature);
  
  if (quadval & 0x40000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}

int
dc1394_absolute_setting_on_off(dc1394camera_t *camera, unsigned int feature, unsigned int value)
{
  int err;
  octlet_t offset;
  quadlet_t curval;
  
  FEATURE_TO_VALUE_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &curval);
  DC1394_ERR_CHK(err, "Could not get abs setting status for feature %d",feature);
  
  if (value && !(curval & 0x40000000UL)) {
    curval|= 0x40000000UL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set absolute control for feature %d",feature);
  }
  else if (!value && (curval & 0x40000000UL)) {
    curval&= 0xBFFFFFFFUL;
    err=SetCameraControlRegister(camera, offset, curval);
    DC1394_ERR_CHK(err, "Could not set absolute control for feature %d",feature);
  }
  
  return err;
}


int
dc1394_has_absolute_control(dc1394camera_t *camera, unsigned int feature, dc1394bool_t *value)
{
  int err;
  octlet_t offset;
  quadlet_t quadval;
  
  FEATURE_TO_INQUIRY_OFFSET(feature, offset);
  
  err=GetCameraControlRegister(camera, offset, &quadval);
  DC1394_ERR_CHK(err, "Could not get absolute control register for feature %d",feature);
  
  if (quadval & 0x40000000UL) {
    *value= DC1394_TRUE;
  }
  else {
    *value= DC1394_FALSE;
  }
  
  return err;
}


/* This function returns the bandwidth used by the camera in bandwidth units.
   The 1394 bus has 4915 bandwidth units available per cycle. Each unit corresponds
   to the time it takes to send one quadlet at ISO speed S1600. The bandwidth usage
   at S400 is thus four times the number of quadlets per packet. Thanks to Krisitian
   Hogsberg for clarifying this.
*/

int
dc1394_get_bandwidth_usage(dc1394camera_t *camera, unsigned int *bandwidth)
{
  unsigned int format, iso, mode, qpp, channel, speed, framerate=0, err;

  // get camera ISO status:
  err=dc1394_get_iso_status(camera, &iso);
  DC1394_ERR_CHK(err, "Could not get ISO status register");
  
  if (iso==DC1394_TRUE) {

    // get format and mode
    err=dc1394_get_video_format(camera, &format);
    DC1394_ERR_CHK(err, "Could not get video format");
    
    err=dc1394_get_video_mode(camera, &mode);
    DC1394_ERR_CHK(err, "Could not get video mode");
    
    if (format==FORMAT_SCALABLE_IMAGE_SIZE) {
      // use the bytes per packet value:
      err=dc1394_query_format7_byte_per_packet(camera, mode, &qpp);
      DC1394_ERR_CHK(err, "Could not get BPP");
      qpp=qpp/4;
    }
    else {
      // get the framerate:
      err=dc1394_get_video_framerate(camera, &framerate);
      DC1394_ERR_CHK(err, "Could not get framerate");
      qpp=_dc1394_get_quadlets_per_packet(format, mode, framerate); 
    }
    // add the ISO header and footer:
    qpp+=3;

    // get camera ISO speed:
    err=dc1394_get_iso_channel_and_speed(camera, &channel, &speed);
    DC1394_ERR_CHK(err, "Could not get ISO channel and speed");
    
    // mutiply by 4 anyway because the best speed is SPEED_400 only
    if (speed>=SPEED_1600)
      *bandwidth = qpp >> (speed-SPEED_1600);
    else
      *bandwidth = qpp << (SPEED_1600-speed);
  }
  else {
    *bandwidth=0;
  }

  return err;
}

int
dc1394_get_camera_port(dc1394camera_t *camera)
{
  return(camera->port);
}
