/*
 * 1394-Based Digital Camera Control Library
 * Copyright (C) 2000 SMART Technologies Inc.
 *
 * Written by Gord Peters <GordPeters@smarttech.com>
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
#ifndef _DC1394_CAMERA_CONTROL_H
#define _DC1394_CAMERA_CONTROL_H

#include <sys/types.h>
#include <libraw1394/raw1394.h>

/* Enumeration of data speeds */
enum {
    SPEED_100 = 0,
    SPEED_200,
    SPEED_400
};
#define SPEED_MIN	SPEED_100
#define SPEED_MAX	SPEED_400
#define NUM_SPEEDS	(SPEED_MAX - SPEED_MIN + 1)

/* Enumeration of camera framerates */
enum {
	FRAMERATE_1_875 = 0,
	FRAMERATE_3_75,
	FRAMERATE_7_5,
	FRAMERATE_15,
	FRAMERATE_30,
	FRAMERATE_60
};
#define FRAMERATE_MIN	FRAMERATE_1_875
#define FRAMERATE_MAX	FRAMERATE_60
#define NUM_FRAMERATES	(FRAMERATE_MAX - FRAMERATE_MIN + 1)

/* Enumeration of camera modes */
enum {
	MODE_160x120_YUV444 = 0,
	MODE_320x240_YUV422,
	MODE_640x480_YUV411,
	MODE_640x480_YUV422,
	MODE_640x480_RGB,
	MODE_640x480_MONO
};
#define MODE_MIN	MODE_160x120_YUV444
#define MODE_MAX	MODE_640x480_MONO
#define NUM_MODES	(MODE_MAX - MODE_MIN + 1)

/* Enumeration of camera image formats */
enum {
	FORMAT_VGA_NONCOMPRESSED = 0,
	FORMAT_SVGA_NONCOMPRESSED_1,
	FORMAT_SVGA_NONCOMPRESSED_2,
	FORMAT_RESERVED_1,
	FORMAT_RESERVED_2,
	FORMAT_RESERVED_3,
	FORMAT_STILL_IMAGE,
	FORMAT_SCALABLE_IMAGE_SIZE
};
#define FORMAT_MIN	FORMAT_VGA_NONCOMPRESSED
#define FORMAT_MAX	FORMAT_SCALABLE_IMAGE_SIZE
#define NUM_FORMATS	(FORMAT_MAX - FORMAT_MIN + 1)

/* Enumeration of camera features */
enum
{
    FEATURE_BRIGHTNESS= 0,
    FEATURE_EXPOSURE,
    FEATURE_SHARPNESS,
    FEATURE_WHITE_BALANCE,
    FEATURE_HUE,
    FEATURE_SATURATION,
    FEATURE_GAMMA,
    FEATURE_SHUTTER,
    FEATURE_GAIN,
    FEATURE_IRIS,
    FEATURE_FOCUS,
    FEATURE_TEMPERATURE,
    FEATURE_TRIGGER,
    FEATURE_ZOOM,
    FEATURE_PAN,
    FEATURE_TILT,
    FEATURE_OPTICAL_FILTER,
    FEATURE_CAPTURE_SIZE,
    FEATURE_CAPTURE_QUALITY
};
#define FEATURE_MIN              FEATURE_BRIGHTNESS
#define FEATURE_MAX              FEATURE_CAPTURE_QUALITY
#define NUM_FEATURES             (FEATURE_MAX - FEATURE_MIN + 1)

/* Maximum number of characters in vendor and model strings */
#define MAX_CHARS                32

/* Yet another boolean data type */
typedef enum
{
    DC1394_FALSE= 0,
    DC1394_TRUE
} dc1394bool_t;

/* Camera structure */
struct dc1394_camerainfo
{
    raw1394handle_t handle;
    nodeid_t id;
    octlet_t ccr_offset;
    u_int64_t euid_64;
    char vendor[MAX_CHARS + 1];
    char model[MAX_CHARS + 1];
};

#ifdef __cplusplus
extern "C" {
#endif

/* determine if the given node is a camera */
int
dc1394_is_camera(raw1394handle_t handle, nodeid_t node, dc1394bool_t *value);

/* get the camera information */
int
dc1394_get_camera_info(raw1394handle_t handle, nodeid_t node,
                       struct dc1394_camerainfo *info);

/* Initialize camera to factory default settings */
int
dc1394_init_camera(raw1394handle_t handle, nodeid_t node);

/* Functions for querying camera attributes */
int
dc1394_query_supported_formats(raw1394handle_t handle, nodeid_t node,
                               quadlet_t *value);
int
dc1394_query_supported_modes(raw1394handle_t handle, nodeid_t node,
                             unsigned int format, quadlet_t *value);
int
dc1394_query_supported_framerates(raw1394handle_t handle, nodeid_t node,
                                  unsigned int format, unsigned int mode,
                                  quadlet_t *value);
int
dc1394_query_revision(raw1394handle_t handle, nodeid_t node, int mode,
                      quadlet_t *value);
int
dc1394_query_csr_offset(raw1394handle_t handle, nodeid_t node, int mode,
                        quadlet_t *value);
int
dc1394_query_basic_functionality(raw1394handle_t handle, nodeid_t node,
                                 quadlet_t *value);
int
dc1394_query_feature_control(raw1394handle_t handle, nodeid_t node,
                             unsigned int feature, unsigned int *availability);
int
dc1394_query_advanced_feature_offset(raw1394handle_t handle, nodeid_t node,
                                     quadlet_t *value);
int
dc1394_query_feature_characteristics(raw1394handle_t handle, nodeid_t node,
                                     unsigned int feature, quadlet_t *value);

/* Get/Set the framerate, mode, format, iso channel/speed for the video */
int
dc1394_get_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int *framerate);
int
dc1394_set_video_framerate(raw1394handle_t handle, nodeid_t node,
                           unsigned int framerate);
int
dc1394_get_video_mode(raw1394handle_t handle, nodeid_t node,
                      unsigned int *mode);
int
dc1394_set_video_mode(raw1394handle_t handle, nodeid_t node,
                      unsigned int mode);
int
dc1394_get_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int *format);
int
dc1394_set_video_format(raw1394handle_t handle, nodeid_t node,
                        unsigned int format);
int
dc1394_get_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int *channel, unsigned int *speed);
int
dc1394_set_iso_channel_and_speed(raw1394handle_t handle, nodeid_t node,
                                 unsigned int channel, unsigned int speed);

/* Turn camera on or off */
int
dc1394_camera_on(raw1394handle_t handle, nodeid_t node);
int
dc1394_camera_off(raw1394handle_t handle, nodeid_t node);

/* Start/stop isochronous data transmission */
int
dc1394_start_iso_transmission(raw1394handle_t handle, nodeid_t node);
int
dc1394_stop_iso_transmission(raw1394handle_t handle, nodeid_t node);

/* Turn one shot mode on or off */
int
dc1394_set_one_shot(raw1394handle_t handle, nodeid_t node);
int
dc1394_unset_one_shot(raw1394handle_t handle, nodeid_t node);

/* Turn multishot mode on or off */
int
dc1394_set_multi_shot(raw1394handle_t handle, nodeid_t node,
                      unsigned int numFrames);
int
dc1394_unset_multi_shot(raw1394handle_t handle, nodeid_t node);

/* Get/Set the values of the various features on the camera */
int
dc1394_get_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int *brightness);
int
dc1394_set_brightness(raw1394handle_t handle, nodeid_t node,
                      unsigned int brightness);
int
dc1394_get_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int *exposure);
int
dc1394_set_exposure(raw1394handle_t handle, nodeid_t node,
                    unsigned int exposure);
int
dc1394_get_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int *sharpness);
int
dc1394_set_sharpness(raw1394handle_t handle, nodeid_t node,
                     unsigned int sharpness);
int
dc1394_get_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int *u_b_value, unsigned int *v_r_value);
int
dc1394_set_white_balance(raw1394handle_t handle, nodeid_t node,
                         unsigned int u_b_value, unsigned int v_r_value);
int
dc1394_get_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int *hue);
int
dc1394_set_hue(raw1394handle_t handle, nodeid_t node,
               unsigned int hue);
int
dc1394_get_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int *saturation);
int
dc1394_set_saturation(raw1394handle_t handle, nodeid_t node,
                      unsigned int saturation);
int
dc1394_get_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int *gamma);
int
dc1394_set_gamma(raw1394handle_t handle, nodeid_t node,
                 unsigned int gamma);
int
dc1394_get_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int *shutter);
int
dc1394_set_shutter(raw1394handle_t handle, nodeid_t node,
                   unsigned int shutter);
int
dc1394_get_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int *gain);
int
dc1394_set_gain(raw1394handle_t handle, nodeid_t node,
                unsigned int gain);
int
dc1394_get_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int *iris);
int
dc1394_set_iris(raw1394handle_t handle, nodeid_t node,
                unsigned int iris);
int
dc1394_get_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int *focus);
int
dc1394_set_focus(raw1394handle_t handle, nodeid_t node,
                 unsigned int focus);
int
dc1394_get_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int *target_temperature,
		       unsigned int *temperature);
int
dc1394_set_temperature(raw1394handle_t handle, nodeid_t node,
                       unsigned int target_temperature);
int
dc1394_get_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int *mode);
int
dc1394_set_trigger_mode(raw1394handle_t handle, nodeid_t node,
                        unsigned int mode);
int
dc1394_get_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int *zoom);
int
dc1394_set_zoom(raw1394handle_t handle, nodeid_t node,
                unsigned int zoom);
int
dc1394_get_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int *pan);
int
dc1394_set_pan(raw1394handle_t handle, nodeid_t node,
               unsigned int pan);
int
dc1394_get_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int *tilt);
int
dc1394_set_tilt(raw1394handle_t handle, nodeid_t node,
                unsigned int tilt);
int
dc1394_get_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int *optical_filter);
int
dc1394_set_optical_filter(raw1394handle_t handle, nodeid_t node,
                          unsigned int optical_filter);
int
dc1394_get_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int *capture_size);
int
dc1394_set_capture_size(raw1394handle_t handle, nodeid_t node,
                        unsigned int capture_size);
int
dc1394_get_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int *capture_quality);
int
dc1394_set_capture_quality(raw1394handle_t handle, nodeid_t node,
                           unsigned int capture_quality);

/* Convenience functions to query/set based on a variable camera feature */
/* (can't be used for white balance) */
int
dc1394_get_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int *value);

int
dc1394_set_feature_value(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, unsigned int value);

/* Query/set specific feature characteristics */
int
dc1394_is_feature_present(raw1394handle_t handle, nodeid_t node,
                          unsigned int feature, dc1394bool_t *value);
int
dc1394_has_one_push_auto(raw1394handle_t handle, nodeid_t node,
                         unsigned int feature, dc1394bool_t *value);
int
dc1394_is_one_push_in_operation(raw1394handle_t handle, nodeid_t node,
                                unsigned int feature, dc1394bool_t *value);
int
dc1394_start_one_push_operation(raw1394handle_t handle, nodeid_t node,
                                unsigned int feature);
int
dc1394_can_read_out(raw1394handle_t handle, nodeid_t node,
                    unsigned int feature, dc1394bool_t *value);
int
dc1394_can_turn_on_off(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value);
int
dc1394_is_feature_on(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, dc1394bool_t *value);
int
dc1394_feature_on_off(raw1394handle_t handle, nodeid_t node,
                      unsigned int feature, unsigned int value);
                                          /* 0=off, nonzero=on */
int
dc1394_has_auto_mode(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, dc1394bool_t *value);
int
dc1394_has_manual_mode(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value);
int
dc1394_is_feature_auto(raw1394handle_t handle, nodeid_t node,
                       unsigned int feature, dc1394bool_t *value);
int
dc1394_auto_on_off(raw1394handle_t handle, nodeid_t node,
                   unsigned int feature, unsigned int value);
                                          /* 0=off, nonzero=on */
int
dc1394_get_min_value(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, unsigned int *value);
int
dc1394_get_max_value(raw1394handle_t handle, nodeid_t node,
                     unsigned int feature, unsigned int *value);

#ifdef __cplusplus
}
#endif

#endif /* _DC1394_CAMERA_CONTROL_H */
