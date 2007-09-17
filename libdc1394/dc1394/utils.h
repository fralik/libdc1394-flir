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
/*! \file utils.h
    \brief This is used for utility functions

    More details soon
*/

#include <dc1394/control.h>

#ifdef __cplusplus
extern "C" {
#endif

dc1394error_t dc1394_get_image_size_from_video_mode(dc1394camera_t *camera, uint32_t video_mode, uint32_t *width, uint32_t *height);
dc1394error_t dc1394_framerate_as_float(dc1394framerate_t framerate_enum, float *framerate);
dc1394error_t dc1394_get_color_coding_depth(dc1394color_coding_t color_coding, uint32_t * bits);
dc1394error_t dc1394_get_bits_per_pixel(dc1394color_coding_t color_coding, uint32_t* bits);
dc1394error_t dc1394_get_color_coding_from_video_mode(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding);
dc1394error_t dc1394_is_color(dc1394color_coding_t color_mode, dc1394bool_t *is_color);
dc1394bool_t dc1394_is_video_mode_scalable(dc1394video_mode_t video_mode);
dc1394bool_t dc1394_is_video_mode_still_image(dc1394video_mode_t video_mode);
dc1394error_t dc1394_update_camera_info(dc1394camera_t *camera);
dc1394bool_t dc1394_is_same_camera(dc1394camera_id_t id1, dc1394camera_id_t id2);

#ifdef __cplusplus
}
#endif

#endif /* _DC1394_UTILS_H */

