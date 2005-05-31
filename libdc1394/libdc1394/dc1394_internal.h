/*
 * 1394-Based Digital Camera Control Library, internal functions
 * Copyright (C) Damien Douxchamps <ddouxchamps@users.sf.net>
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
#ifndef __DC1394_INTERNAL_H__
#define __DC1394_INTERNAL_H__

#include "dc1394_control.h"
#include "dc1394_offsets.h"
#include <libraw1394/raw1394.h>

/* Definitions which application developers shouldn't care about */
#define CONFIG_ROM_BASE             0xFFFFF0000000ULL

#define DC1394_FEATURE_ON           0x80000000UL
#define DC1394_FEATURE_OFF          0x00000000UL

/* Maximum number of write/read retries */
#define DC1394_MAX_RETRIES          20

/* A hard compiled factor that makes sure async read and writes don't happen
   too fast */
/* Toshiyuki Umeda: use randomize timings to avoid locking indefinitely.
   If several thread are used the seed will be the same and the wait will therefor be
   identical. A call to srand(getpid()) should be performed after calling the
   raw1394_new_handle but I (Damien) not sure this would solve the problem as the handle
   creation might have happened in the creating (as opposed to created) thread.*/
#define DC1394_SLOW_DOWN            ((rand()%20)+10)

/* transaction acknowldegements (this should be in the raw1394 headers) */
#define ACK_COMPLETE                0x0001U
#define ACK_PENDING                 0x0002U
#define ACK_LOCAL                   0x0010U

/*Response codes (this should be in the raw1394 headers) */
//not currently used
#define RESP_COMPLETE               0x0000U
#define RESP_SONY_HACK              0x000fU

#define FEATURE_TO_VALUE_OFFSET(feature, offset)                      \
                                                                      \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {       \
      return DC1394_FAILURE;                                          \
    }                                                                 \
    else if (feature < DC1394_FEATURE_ZOOM) {                                \
      offset= REG_CAMERA_FEATURE_HI_BASE;                             \
      feature-= DC1394_FEATURE_MIN;                                          \
    }                                                                 \
    else {                                                            \
      offset= REG_CAMERA_FEATURE_LO_BASE;                             \
      if (feature >= DC1394_FEATURE_CAPTURE_SIZE) {                          \
        feature+= 12;                                                 \
      }                                                               \
      feature-= DC1394_FEATURE_ZOOM;                                         \
    }                                                                 \
    offset+= feature * 0x04U;

#define FEATURE_TO_INQUIRY_OFFSET(feature, offset)                    \
                                                                      \
    if ( (feature > DC1394_FEATURE_MAX) || (feature < DC1394_FEATURE_MIN) ) {       \
      return DC1394_FAILURE;                                          \
    }                                                                 \
    else if (feature < DC1394_FEATURE_ZOOM) {                                \
      offset= REG_CAMERA_FEATURE_HI_BASE_INQ;                         \
      feature-= DC1394_FEATURE_MIN;                                          \
    }                                                                 \
    else {                                                            \
      offset= REG_CAMERA_FEATURE_LO_BASE_INQ;                         \
      if (feature >= DC1394_FEATURE_CAPTURE_SIZE) {                          \
        feature+= 12;                                                 \
      }                                                               \
      feature-= DC1394_FEATURE_ZOOM;                                         \
    }                                                                 \
    offset+= feature * 0x04U;

/* Internal functions required by two different source files */

int
_dc1394_dma_basic_setup(uint_t channel, uint_t num_dma_buffers, dc1394capture_t *capture);
	
int 
_dc1394_get_quadlets_per_packet(uint_t mode, uint_t frame_rate, uint_t *qpp);

int
_dc1394_quadlets_from_format(uint_t mode, uint_t *quads);

int
_dc1394_get_format_from_mode(uint_t mode, uint_t *format);
		
int
IsFeatureBitSet(quadlet_t value, uint_t feature);

#endif /* _DC1394_INTERNAL_H */
