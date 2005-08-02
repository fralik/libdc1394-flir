/*
 * 1394-Based Digital Camera register access functions
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

#include "dc1394_control.h"
#include "dc1394_internal.h"
#include "dc1394_offsets.h"
#include "config.h"

#define FEATURE_TO_ABS_VALUE_OFFSET(feature, offset)                  \
                                                                      \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) )         \
    {                                                                 \
	return DC1394_FAILURE;                                        \
    }                                                                 \
    else if (feature < DC1394_FEATURE_ZOOM)                                  \
    {                                                                 \
	offset= REG_CAMERA_FEATURE_ABS_HI_BASE;                       \
        feature-= DC1394_FEATURE_MIN;                                        \
    }                                                                 \
    else                                                              \
    {                                                                 \
	offset= REG_CAMERA_FEATURE_ABS_LO_BASE;                       \
	feature-= DC1394_FEATURE_ZOOM;                                       \
                                                                      \
	if (feature >= DC1394_FEATURE_CAPTURE_SIZE)                          \
	{                                                             \
	    feature+= 12;                                             \
	}                                                             \
                                                                      \
    }                                                                 \
                                                                      \
    offset+= feature * 0x04U;

dc1394error_t
GetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t *value)
{
  int retval, retry= DC1394_MAX_RETRIES;
#ifdef LIBRAW1394_OLD
  int ack=0;
  int rcode=0;
#endif

  /* retry a few times if necessary (addition by PDJ) */
  while(retry--)  {
    //fprintf(stderr,"get reg at 0x%llx : ", offset + CONFIG_ROM_BASE);
    retval= raw1394_read(camera->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4, value);
    //fprintf(stderr,"0x%lx\n",*value);

#ifdef LIBRAW1394_OLD
    if (retval >= 0) {
      ack = retval >> 16;
      rcode = retval & 0xffff;
      
#ifdef SHOW_ERRORS
      printf("ROM read ack of %x rcode of %x\n", ack, rcode);
#endif

      if ( ((ack == ACK_PENDING) || (ack == ACK_LOCAL)) &&
	   (rcode == RESP_COMPLETE) ) { 
	/* conditionally byte swap the value */
	*value= ntohl(*value); 
	return DC1394_SUCCESS;
      }

    }
#else
    if (!retval) {
      /* conditionally byte swap the value */
      *value= ntohl(*value);
      return ( retval ? DC1394_FAILURE : DC1394_SUCCESS );;
    }
    else if (errno != EAGAIN) {
      return ( retval ? DC1394_FAILURE : DC1394_SUCCESS );;
    }
#endif /* LIBRAW1394_VERSION <= 0.8.2 */
    
    usleep(DC1394_SLOW_DOWN);
  }
  
  *value= ntohl(*value);
  return ( retval ? DC1394_FAILURE : DC1394_SUCCESS );
}

dc1394error_t
SetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t value)
{
  int retval, retry= DC1394_MAX_RETRIES;
#ifdef LIBRAW1394_OLD
  int ack= 0;
  int rcode= 0;
#endif

  /* conditionally byte swap the value (addition by PDJ) */
  value= htonl(value);
  
  /* retry a few times if necessary */
  while(retry--) {
    retval= raw1394_write(camera->handle, 0xffc0 | camera->node, offset + CONFIG_ROM_BASE, 4, &value);
    /* printf(" Set Adv debug : base = %Lux, offset = %Lux, base + offset = %Lux\n",camera->adv_csr,offset, camera->adv_csr+offset); */
#ifdef LIBRAW1394_OLD
    if (retval >= 0) {
      ack= retval >> 16;
      rcode= retval & 0xffff;
      
#ifdef SHOW_ERRORS
      printf("CCR write ack of %x rcode of %x\n", ack, rcode);
#endif
      
      if ( ((ack == ACK_PENDING) || (ack == ACK_LOCAL) || (ack == ACK_COMPLETE)) &&
	   ((rcode == RESP_COMPLETE) || (rcode == RESP_SONY_HACK)) ) {
	return DC1394_SUCCESS;
      }
            
    }
#else
    if (!retval || (errno != EAGAIN)) {
      return ( retval ? DC1394_FAILURE : DC1394_SUCCESS );;
    }
#endif /* LIBRAW1394_VERSION <= 0.8.2 */
    
    usleep(DC1394_SLOW_DOWN);
  }
  
  return ( retval ? DC1394_FAILURE : DC1394_SUCCESS );
}

/********************************************************************************/
/* Get/Set Command Registers                                                    */
/********************************************************************************/
dc1394error_t
GetCameraControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t *value)
{
  int retval;

  if (camera == NULL)
    return DC1394_FAILURE;
  
  if (camera->command_registers_base == 0) {
    fprintf(stderr,"this should not happen\n");
    if (dc1394_get_camera_info(camera) != DC1394_SUCCESS)
      return DC1394_FAILURE;
  }
  //fprintf(stderr,"trying to get 0x%llx",camera->command_registers_base+offset);
  retval = GetCameraROMValue(camera, camera->command_registers_base+offset, value);

  //fprintf(stderr,"retval = %d",retval);

  return retval;
}

dc1394error_t
SetCameraControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t value)
{
  int retval;

  if (camera == NULL)
    return DC1394_FAILURE;
  
  if (camera->command_registers_base == 0) {
    if (dc1394_get_camera_info(camera) != DC1394_SUCCESS)
      return DC1394_FAILURE;
  }
  
  //fprintf(stderr,"trying to set 0x%llx\n",camera->command_registers_base+offset);
  retval= SetCameraROMValue(camera, camera->command_registers_base+offset, value);

  return retval;
}

/********************************************************************************/
/* Get/Set Advanced Features Registers                                          */
/********************************************************************************/
dc1394error_t
GetCameraAdvControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t *value)
{
  if (camera == NULL)
    return DC1394_FAILURE;
  
  if (camera->advanced_features_csr == 0) {
    if (dc1394_get_camera_info(camera) != DC1394_SUCCESS)
      return DC1394_FAILURE;
  }
  
  return GetCameraROMValue(camera, camera->advanced_features_csr+offset, value);
}

dc1394error_t
SetCameraAdvControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t value)
{
  if (camera == NULL)
    return DC1394_FAILURE;
  
  if (camera->advanced_features_csr == 0) {
    if (dc1394_get_camera_info(camera) != DC1394_SUCCESS)
      return DC1394_FAILURE;
  }
  
  return SetCameraROMValue(camera, camera->advanced_features_csr+offset, value);
}

/********************************************************************************/
/* Get/Set Format_7 Registers                                                   */
/********************************************************************************/

dc1394error_t
QueryFormat7CSROffset(dc1394camera_t *camera, unsigned int mode, octlet_t *offset)
{
  int retval;
  quadlet_t temp;

  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }
  
  retval=GetCameraControlRegister(camera, REG_CAMERA_V_CSR_INQ_BASE + ((mode - DC1394_MODE_FORMAT7_MIN) * 0x04U), &temp);
  //fprintf(stderr,"got register 0x%llx : 0x%x, err=%d\n",
  //	  CONFIG_ROM_BASE + REG_CAMERA_V_CSR_INQ_BASE + ((mode - MODE_FORMAT7_MIN) * 0x04U),temp,retval);
  *offset=temp*4;
  return retval;
}

dc1394error_t
GetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, octlet_t offset, quadlet_t *value)
{
  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  if (camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]==0) {
    //fprintf(stderr,"getting offset for mode %d: ",mode);
    if (QueryFormat7CSROffset(camera, mode, &camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]) != DC1394_SUCCESS) {
      //fprintf(stderr,"Error getting F7 CSR offset\n");
      return DC1394_FAILURE;
    }
      
    //fprintf(stderr,"0x%llx\n",camera->format7_csr[mode-MODE_FORMAT7_MIN]);
  }

  return GetCameraROMValue(camera, camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]+offset, value);

  //fprintf(stderr,"got register 0x%llx : 0x%x, err=%d\n",CONFIG_ROM_BASE + camera->format7_csr[mode-MODE_FORMAT7_MIN]+offset, *value,retval);
}

dc1394error_t
SetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, octlet_t offset, quadlet_t value)
{
  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  if ( (mode > DC1394_MODE_FORMAT7_MAX) || (mode < DC1394_MODE_FORMAT7_MIN) ) {
    return DC1394_FAILURE;
  }

  if (camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]==0) {
    QueryFormat7CSROffset(camera, mode, &camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]);
  }

  //fprintf(stderr,"trying to set F7 0x%llx\n",camera->format7_csr[mode-MODE_FORMAT7_MIN]+offset);
  return SetCameraROMValue(camera, camera->format7_csr[mode-DC1394_MODE_FORMAT7_MIN]+offset, value);
}

/********************************************************************************/
/* Get/Set Absolute Control Registers                                           */
/********************************************************************************/

dc1394error_t
QueryAbsoluteCSROffset(dc1394camera_t *camera, unsigned int feature, octlet_t *value)
{
  int offset, retval;
  quadlet_t quadlet=0;
  
  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  FEATURE_TO_ABS_VALUE_OFFSET(feature, offset);
    
  retval=GetCameraControlRegister(camera, offset, &quadlet);
  
  *value=quadlet * 0x04;

  return retval;
}

dc1394error_t
GetCameraAbsoluteRegister(dc1394camera_t *camera, unsigned int feature, octlet_t offset, quadlet_t *value)
{
  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  ////// TODO: all offsets should be memorized
  QueryAbsoluteCSROffset(camera, feature, &camera->advanced_features_csr);

  return GetCameraROMValue(camera, camera->advanced_features_csr+offset, value);

}

dc1394error_t
SetCameraAbsoluteRegister(dc1394camera_t *camera, unsigned int feature, octlet_t offset, quadlet_t value)
{
  if (camera == NULL) {
    return DC1394_FAILURE;
  }

  ////// TODO: all offsets should be memorized
  QueryAbsoluteCSROffset(camera, feature, &camera->advanced_features_csr);

  return SetCameraROMValue(camera, camera->advanced_features_csr+offset, value);

}

/********************************************************************************/
/* Find a register with a specific tag                                          */
/********************************************************************************/

dc1394error_t
GetConfigROMTaggedRegister(dc1394camera_t *camera, unsigned int tag, octlet_t *offset, quadlet_t *value)
{
  unsigned int block_length;
  int i;
  
  // get the block length
  //fprintf(stderr,"Getting register tag 0x%x starting at offset 0x%x\n",tag,*offset);
  if (GetCameraROMValue(camera, *offset, value)!=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }
  
  block_length=*value>>16;
  //fprintf(stderr,"  block length = %d\n",block_length);
  if (*offset+block_length*4>CSR_CONFIG_ROM_END) {
    block_length=(CSR_CONFIG_ROM_END-*offset)/4;
  }

  // find the tag and return the result
  for (i=0;i<block_length;i++) {
    *offset+=4;
    if (GetCameraROMValue(camera,*offset,value)!=DC1394_SUCCESS) {
      return DC1394_FAILURE;
    }
    if ((*value>>24)==tag) {
      return DC1394_SUCCESS;
    }
  }

  return DC1394_TAGGED_REGISTER_NOT_FOUND;
}

/*
// Don Murray's debug version
int
GetConfigROMTaggedRegister(dc1394camera_t *camera, unsigned int tag, 
octlet_t *offset, quadlet_t *value)
{
  unsigned int block_length;
  int i;
  // get the block length
  fprintf(stderr,"Getting register tag 0x%x starting at offset 0x%x\n",tag,*offset);
  if (GetCameraROMValue(camera, *offset, value)!=DC1394_SUCCESS) {
    return DC1394_FAILURE;
  }
  block_length=*value>>16;
  if (*offset+block_length*4>CSR_CONFIG_ROM_END) {
    block_length=(CSR_CONFIG_ROM_END-*offset)/4;   }
  
  // find the tag and return the result
  for (i=0;i<block_length;i++) {
    *offset+=4;
    if (GetCameraROMValue(camera,*offset,value)!=DC1394_SUCCESS) {
      fprintf(stderr,"GetCameraROMValue() failed\n" );
      return DC1394_FAILURE;
    }
    if ((*value>>24)==tag) {
      fprintf(stderr,"found tag 0x%x!!\n", tag );
      return DC1394_SUCCESS;
    }
    else {
      fprintf(stderr,"found tag 0x%x (value 0x%x) - continue \n",
	      *value>>24,
	      *value );
    }
  }
  
  return DC1394_TAGGED_REGISTER_NOT_FOUND;
}
*/
