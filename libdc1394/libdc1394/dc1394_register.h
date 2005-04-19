/*
 * 1394-Based Digital Camera register access functions for the Control
 *   Library
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

#ifndef __DC1394_REGISTER_H__
#define __DC1394_REGISTER_H__

#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <libraw1394/raw1394.h>
 
int
GetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t *value);

int
SetCameraROMValue(dc1394camera_t *camera, octlet_t offset, quadlet_t value);

/********************************************************************************/
/* Get/Set Command Registers                                                    */
/********************************************************************************/
int
GetCameraControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t *value);

int
SetCameraControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t value);

/********************************************************************************/
/* Get/Set Advanced Features Registers                                          */
/********************************************************************************/
int
GetCameraAdvControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t *value);

int
SetCameraAdvControlRegister(dc1394camera_t *camera, octlet_t offset, quadlet_t value);

/********************************************************************************/
/* Get/Set Format_7 Registers                                                   */
/********************************************************************************/

int
GetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, octlet_t offset, quadlet_t *value);

int
SetCameraFormat7Register(dc1394camera_t *camera, unsigned int mode, octlet_t offset, quadlet_t value);

/********************************************************************************/
/* Find a register with a specific tag                                          */
/********************************************************************************/
int
GetConfigROMTaggedRegister(dc1394camera_t *camera, unsigned int tag, octlet_t *offset, quadlet_t *value);

int
QueryFormat7CSROffset(dc1394camera_t *camera, int mode, octlet_t *offset);

#endif /* __DC1394_REGISTER_H__ */
