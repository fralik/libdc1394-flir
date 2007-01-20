/*
 * 1394-Based Digital Camera register access function
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

#ifndef __DC1394_REGISTER_H__
#define __DC1394_REGISTER_H__

#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
 
#ifdef __cplusplus
extern "C" {
#endif

dc1394error_t
GetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads);

static inline dc1394error_t
GetCameraROMValue(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
  return GetCameraROMValues(camera, offset, value, 1);
}

dc1394error_t
SetCameraROMValues(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_quads);

static inline dc1394error_t
SetCameraROMValue(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
  return SetCameraROMValues(camera, offset, &value, 1);
}


/********************************************************************************/
/* Get/Set Command Registers                                                    */
/********************************************************************************/
dc1394error_t
GetCameraControlRegisters(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_regs);

static inline dc1394error_t
GetCameraControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
  return GetCameraControlRegisters(camera, offset, value, 1);
}

dc1394error_t
SetCameraControlRegisters(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_regs);

static inline dc1394error_t
SetCameraControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
  return SetCameraControlRegisters(camera, offset, &value, 1);
}


/********************************************************************************/
/* Get/Set Advanced Features Registers                                          */
/********************************************************************************/
dc1394error_t
GetCameraAdvControlRegisters(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_regs);

static inline dc1394error_t
GetCameraAdvControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t *value)
{
  return GetCameraAdvControlRegisters(camera, offset, value, 1);
}

dc1394error_t
SetCameraAdvControlRegisters(dc1394camera_t *camera, uint64_t offset, uint32_t *value, uint32_t num_regs);

static inline dc1394error_t
SetCameraAdvControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t value)
{
  return SetCameraAdvControlRegisters(camera, offset, &value, 1);
}

/********************************************************************************/
/* Get/Set Format_7 Registers                                                   */
/********************************************************************************/
dc1394error_t
GetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t *value);

dc1394error_t
SetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, uint64_t offset, uint32_t value);

dc1394error_t
QueryFormat7CSROffset(dc1394camera_t *camera, unsigned int mode, uint64_t *offset);


/********************************************************************************/
/* Get/Set Absolute Control Registers                                           */
/********************************************************************************/
dc1394error_t
GetCameraAbsoluteRegister(dc1394camera_t *camera, unsigned int feature, uint64_t offset, uint32_t *value);

dc1394error_t
SetCameraAbsoluteRegister(dc1394camera_t *camera, unsigned int feature, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set PIO Feature Registers                                                */
/********************************************************************************/
dc1394error_t
GetCameraPIOControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

dc1394error_t
SetCameraPIOControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set SIO Feature Registers                                                */
/********************************************************************************/
dc1394error_t
GetCameraSIOControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

dc1394error_t
SetCameraSIOControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Get/Set Strobe Feature Registers                                                */
/********************************************************************************/
dc1394error_t
GetCameraStrobeControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t *value);

dc1394error_t
SetCameraStrobeControlRegister(dc1394camera_t *camera, uint64_t offset, uint32_t value);


/********************************************************************************/
/* Find a register with a specific tag                                          */
/********************************************************************************/
dc1394error_t
GetConfigROMTaggedRegister(dc1394camera_t *camera, unsigned int tag, uint64_t *offset, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* __DC1394_REGISTER_H__ */
