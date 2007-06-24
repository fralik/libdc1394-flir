/*
 * Juju backend for dc1394
 * Copyright (C) 2007 Kristian Hoegsberg <krh@bitplanet.net>
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
#ifndef __DC1394_JUJU_H__
#define __DC1394_JUJU_H__

#include <linux/firewire-cdev.h>
#include "config.h"
#include "internal.h"
#include "register.h"
#include "offsets.h"
#include "control.h"

#define DC1394_CAST_CAMERA_TO_JUJU(camjuju, camera) \
	dc1394camera_juju_t * camjuju = (dc1394camera_juju_t *) camera

struct juju_frame {
  dc1394video_frame_t		 frame;
  size_t			 size;
  struct fw_cdev_iso_packet	*packets;
};

typedef struct __dc1394_camera_juju
{
  dc1394camera_t		 camera;
  int				 fd;
  char				*filename;
  uint32_t			 config_rom[256];

  int				 iso_fd;
  int				 iso_handle;
  unsigned int			 num_frames;
  int				 current;
  int				 ready_frames;
  struct juju_frame		*frames;

  size_t			 buffer_size;
  unsigned char			*buffer;

  uint32_t			 flags;

  struct {
    int				 done;
    uint32_t			 buffer[64];
    int				 generation;
  } async;

} dc1394camera_juju_t;

dc1394error_t
_juju_iterate(dc1394camera_t *camera);

#endif
