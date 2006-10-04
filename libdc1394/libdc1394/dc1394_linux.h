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
#ifndef __DC1394_LINUX_H__
#define __DC1394_LINUX_H__

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include "raw1394support.h"
#include "dc1394_control.h"

#define DC1394_CAST_CAMERA_TO_LINUX(camlinux, camera) dc1394camera_linux_t * camlinux = (dc1394camera_linux_t *) camera

typedef struct __dc1394_capture
{
  unsigned char            *capture_buffer;
  /* components needed for the DMA based video capture */
  const unsigned char     *dma_ring_buffer;
  char                    *dma_device_file;
  unsigned int             dma_buffer_size;
  unsigned int             dma_frame_size;
  unsigned int             num_dma_buffers;
  unsigned int             dma_last_buffer;
  int                      dma_fd;
  raw1394handle_t          handle;

  dc1394video_frame_t     *frames;
} dc1394capture_t;

typedef struct __dc1394_camera_linux
{
  dc1394camera_t     camera;
  raw1394handle_t    handle;
  dc1394capture_t    capture;
  SelfIdPacket_t     selfid_packet;
  
} dc1394camera_linux_t;

#endif
