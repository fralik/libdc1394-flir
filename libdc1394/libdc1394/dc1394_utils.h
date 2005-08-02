/*
 * 1394-Based Digital Camera Control Library utilities
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
#ifndef __DC1394_UTILS_H__
#define __DC1394_UTILS_H__

#include "dc1394_control.h"

dc1394error_t dc1394_get_wh_from_mode(uint_t mode, uint_t *w, uint_t *h);
dc1394error_t dc1394_framerate_as_float(uint_t framerate_enum, float *framerate);
dc1394error_t dc1394_get_bytes_per_pixel(uint_t color_mode, float* bpp);
dc1394error_t dc1394_get_color_mode_from_mode(uint_t mode, uint_t *color_mode);
dc1394error_t dc1394_is_color(uint_t color_mode, dc1394bool_t *is_color);

#endif /* _DC1394_UTILS_H */

