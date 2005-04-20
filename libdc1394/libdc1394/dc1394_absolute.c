/*
 * 1394-Based Digital Camera Absolute Setting functions
 *
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
#include "dc1394_offsets.h"
#include "dc1394_internal.h"
#include "dc1394_register.h"


int
dc1394_query_absolute_feature_min_max(dc1394camera_t *camera, unsigned int feature, float *min, float *max)
{
  if ( (feature > FEATURE_MAX) || (feature < FEATURE_MIN) ) {
    return DC1394_FAILURE;
  }
  
  if (GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_MAX, (quadlet_t*)max) != DC1394_SUCCESS)
    return DC1394_FAILURE;
  
  return GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_MIN, (quadlet_t*)min);
}


int
dc1394_query_absolute_feature_value(dc1394camera_t *camera, int feature, float *value)
{
  if ( (feature > FEATURE_MAX) || (feature < FEATURE_MIN) ) {
    return DC1394_FAILURE;
  }
  return GetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_VALUE, (quadlet_t*)value);
}


int
dc1394_set_absolute_feature_value(dc1394camera_t *camera, int feature, float value)
{
  if ( (feature > FEATURE_MAX) || (feature < FEATURE_MIN) ) {
    return DC1394_FAILURE;
  }
  
  return SetCameraAbsoluteRegister(camera, feature, REG_CAMERA_ABS_VALUE, (quadlet_t)((quadlet_t*)(&value)));

}
 
