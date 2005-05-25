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
#ifndef __DC1394_CAMERA_CONTROL_H__
#define __DC1394_CAMERA_CONTROL_H__

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <libraw1394/raw1394.h>
#include "dc1394_control.h"

#define uint_t   unsigned int
#define uint16_t unsigned short int
#define uint_t   unsigned int
#define uint64_t unsigned long long int
#define uchar_t  unsigned char

/* Enumeration of data speeds */
enum {
  DC1394_SPEED_100= 0,
  DC1394_SPEED_200,
  DC1394_SPEED_400,
  DC1394_SPEED_800,
  DC1394_SPEED_1600,
  DC1394_SPEED_3200
};
#define DC1394_ISO_SPEED_MIN                   DC1394_ISO_SPEED_100
#define DC1394_ISO_SPEED_MAX                   DC1394_ISO_SPEED_3200
#define DC1394_ISO_SPEED_NUM                  (DC1394_ISO_SPEED_MAX - DC1394_ISO_SPEED_MIN + 1)

/* Enumeration of camera framerates */
enum {
  DC1394_FRAMERATE_1_875= 32,
  DC1394_FRAMERATE_3_75,
  DC1394_FRAMERATE_7_5,
  DC1394_FRAMERATE_15,
  DC1394_FRAMERATE_30,
  DC1394_FRAMERATE_60,
  DC1394_FRAMERATE_120,
  DC1394_FRAMERATE_240
};
#define DC1394_FRAMERATE_MIN               DC1394_FRAMERATE_1_875
#define DC1394_FRAMERATE_MAX               DC1394_FRAMERATE_240
#define DC1394_FRAMERATE_NUM              (DC1394_FRAMERATE_MAX - DC1394_FRAMERATE_MIN + 1)

/* Enumeration of camera modes for Format_0 */
enum {
  DC1394_MODE_160x120_YUV444= 64,
  DC1394_MODE_320x240_YUV422,
  DC1394_MODE_640x480_YUV411,
  DC1394_MODE_640x480_YUV422,
  DC1394_MODE_640x480_RGB8,
  DC1394_MODE_640x480_MONO8,
  DC1394_MODE_640x480_MONO16,
  DC1394_MODE_800x600_YUV422= 96,
  DC1394_MODE_800x600_RGB8,
  DC1394_MODE_800x600_MONO8,
  DC1394_MODE_1024x768_YUV422,
  DC1394_MODE_1024x768_RGB8,
  DC1394_MODE_1024x768_MONO8,
  DC1394_MODE_800x600_MONO16,
  DC1394_MODE_1024x768_MONO16,
  DC1394_MODE_1280x960_YUV422= 128,
  DC1394_MODE_1280x960_RGB8,
  DC1394_MODE_1280x960_MONO8,
  DC1394_MODE_1600x1200_YUV422,
  DC1394_MODE_1600x1200_RGB8,
  DC1394_MODE_1600x1200_MONO8,
  DC1394_MODE_1280x960_MONO16,
  DC1394_MODE_1600x1200_MONO16,
  DC1394_MODE_EXIF= 256,
  DC1394_MODE_FORMAT7_0= 288,
  DC1394_MODE_FORMAT7_1,
  DC1394_MODE_FORMAT7_2,
  DC1394_MODE_FORMAT7_3,
  DC1394_MODE_FORMAT7_4,
  DC1394_MODE_FORMAT7_5,
  DC1394_MODE_FORMAT7_6,
  DC1394_MODE_FORMAT7_7
};

/* Format_0 */
#define DC1394_MODE_FORMAT0_MIN	    DC1394_MODE_160x120_YUV444
#define DC1394_MODE_FORMAT0_MAX	    DC1394_MODE_640x480_MONO16
#define DC1394_MODE_FORMAT0_NUM    (DC1394_MODE_FORMAT0_MAX - DC1394_MODE_FORMAT0_MIN + 1)

/* Format_1 */
#define DC1394_MODE_FORMAT1_MIN	    DC1394_MODE_800x600_YUV422
#define DC1394_MODE_FORMAT1_MAX	    DC1394_MODE_1024x768_MONO16
#define DC1394_MODE_FORMAT1_NUM    (DC1394_MODE_FORMAT1_MAX - DC1394_MODE_FORMAT1_MIN + 1)

/* Format_2 */
#define DC1394_MODE_FORMAT2_MIN	    DC1394_MODE_1280x960_YUV422
#define DC1394_MODE_FORMAT2_MAX	    DC1394_MODE_1600x1200_MONO16
#define DC1394_MODE_FORMAT2_NUM	   (DC1394_MODE_FORMAT2_MAX - DC1394_MODE_FORMAT2_MIN + 1)

/* Format_6 */
#define DC1394_MODE_FORMAT6_MIN     DC1394_MODE_EXIF
#define DC1394_MODE_FORMAT6_MAX     DC1394_MODE_EXIF
#define DC1394_MODE_FORMAT6_NUM    (DC1394_MODE_FORMAT6_MAX - DC1394_MODE_FORMAT6_MIN + 1)

/* Format_7 */
#define DC1394_MODE_FORMAT7_MIN     DC1394_MODE_FORMAT7_0
#define DC1394_MODE_FORMAT7_MAX     DC1394_MODE_FORMAT7_7
#define DC1394_MODE_FORMAT7_NUM    (DC1394_MODE_FORMAT7_MAX - DC1394_MODE_FORMAT7_MIN + 1)

/* Enumeration of Format_7 color modes */
enum {
  DC1394_COLOR_CODING_MONO8= 320,
  DC1394_COLOR_CODING_YUV411,
  DC1394_COLOR_CODING_YUV422,
  DC1394_COLOR_CODING_YUV444,
  DC1394_COLOR_CODING_RGB8,
  DC1394_COLOR_CODING_MONO16,
  DC1394_COLOR_CODING_RGB16,
  DC1394_COLOR_CODING_MONO16S,
  DC1394_COLOR_CODING_RGB16S,
  DC1394_COLOR_CODING_RAW8,
  DC1394_COLOR_CODING_RAW16
};
#define DC1394_COLOR_CODING_MIN     DC1394_COLOR_CODING_MONO8
#define DC1394_COLOR_CODING_MAX     DC1394_COLOR_CODING_RAW16
#define DC1394_COLOR_CODING_NUM    (DC1394_COLOR_CODING_MAX - DC1394_COLOR_CODING_MIN + 1)

/* Enumeration of trigger modes */
enum {
  DC1394_TRIGGER_MODE_0= 352,
  DC1394_TRIGGER_MODE_1,
  DC1394_TRIGGER_MODE_2,
  DC1394_TRIGGER_MODE_3
};
#define DC1394_TRIGGER_MODE_MIN     DC1394_TRIGGER_MODE_0
#define DC1394_TRIGGER_MODE_MAX     DC1394_TRIGGER_MODE_3
#define DC1394_TRIGGER_MODE_NUM    (DC1394_TRIGGER_MODE_3 - DC1394_TRIGGER_MODE_0 + 1)

/* Enumeration of camera image formats */
enum {
  DC1394_FORMAT0= 384,
  DC1394_FORMAT1,
  DC1394_FORMAT2,
  DC1394_FORMAT6=390,
  DC1394_FORMAT7
};
#define DC1394_FORMAT_MIN           DC1394_FORMAT0
#define DC1394_FORMAT_MAX           DC1394_FORMAT7
//#define DC1394_FORMAT_NUM          (DC1394_FORMAT_MAX - DC1394_FORMAT_MIN + 1)
/* DANGER: FORMAT_NUM devrait etre 5!! FORMAT_NUM est donc non-defini pour etre sur... */

/* Enumeration of camera features */
enum {
  DC1394_FEATURE_BRIGHTNESS= 416,
  DC1394_FEATURE_EXPOSURE,
  DC1394_FEATURE_SHARPNESS,
  DC1394_FEATURE_WHITE_BALANCE,
  DC1394_FEATURE_HUE,
  DC1394_FEATURE_SATURATION,
  DC1394_FEATURE_GAMMA,
  DC1394_FEATURE_SHUTTER,
  DC1394_FEATURE_GAIN,
  DC1394_FEATURE_IRIS,
  DC1394_FEATURE_FOCUS,
  DC1394_FEATURE_TEMPERATURE,
  DC1394_FEATURE_TRIGGER,
  DC1394_FEATURE_TRIGGER_DELAY,
  DC1394_FEATURE_WHITE_SHADING,
  DC1394_FEATURE_FRAME_RATE,
  /* 16 reserved features */
  DC1394_FEATURE_ZOOM,
  DC1394_FEATURE_PAN,
  DC1394_FEATURE_TILT,
  DC1394_FEATURE_OPTICAL_FILTER,
  /* 12 reserved features */
  DC1394_FEATURE_CAPTURE_SIZE,
  DC1394_FEATURE_CAPTURE_QUALITY
  /* 14 reserved features */
};
#define DC1394_FEATURE_MIN           DC1394_FEATURE_BRIGHTNESS
#define DC1394_FEATURE_MAX           DC1394_FEATURE_CAPTURE_QUALITY
#define DC1394_FEATURE_NUM          (DC1394_FEATURE_MAX - DC1394_FEATURE_MIN + 1)

/* Operation modes */
enum {
  DC1394_OPERATION_MODE_LEGACY = 480,
  DC1394_OPERATION_MODE_1394B
};

/* Format 7 sensor layouts*/
enum {
  DC1394_COLOR_FILTER_RGGB = 512,
  DC1394_COLOR_FILTER_GBRG,
  DC1394_COLOR_FILTER_GRBG,
  DC1394_COLOR_FILTER_BGGR
};
#define DC1394_COLOR_FILTER_MIN        DC1394_COLOR_FILTER_FORMAT_RGGB
#define DC1394_COLOR_FILTER_MAX        DC1394_COLOR_FILTER_FORMAT_BGGR
#define DC1394_COLOR_FILTER_NUM       (DC1394_COLOR_FILTER_FORMAT_MAX - DC1394_COLOR_FILTER_FORMAT_MIN + 1)

/* IIDC versions*/
enum {
  DC1394_IIDC_VERSION_1_04 = 544,
  DC1394_IIDC_VERSION_1_20,
  DC1394_IIDC_VERSION_PTGREY,
  DC1394_IIDC_VERSION_1_30,
  DC1394_IIDC_VERSION_1_31,
  DC1394_IIDC_VERSION_1_32,
  DC1394_IIDC_VERSION_1_33,
  DC1394_IIDC_VERSION_1_34,
  DC1394_IIDC_VERSION_1_35,
  DC1394_IIDC_VERSION_1_36,
  DC1394_IIDC_VERSION_1_37,
  DC1394_IIDC_VERSION_1_38,
  DC1394_IIDC_VERSION_1_39
};

#define DC1394_IIDC_VERSION_MIN        DC1394_IIDC_VERSION_1_04
#define DC1394_IIDC_VERSION_MAX        DC1394_IIDC_VERSION_1_39
#define DC1394_IIDC_VERSION_NUM       (DC1394_IIDC_VERSION_MAX - DC1394_IIDC_VERSION_MIN + 1)

/* Maximum number of characters in vendor and model strings */
#define MAX_CHARS                      256

/* Return values for visible functions*/
typedef enum {
  DC1394_SUCCESS=0, /* Success is zero */
  DC1394_FAILURE,/* Errors are positive numbers */
  DC1394_NO_FRAME=-2, /* Warnings or info are negative numbers*/
  DC1394_NO_CAMERA=3,
  DC1394_NOT_A_CAMERA,
  DC1394_FUNCTION_NOT_SUPPORTED,
  DC1394_CAMERA_NOT_INITITIALIZED, 
  DC1394_INVALID_FEATURE,
  DC1394_INVALID_FORMAT,
  DC1394_INVALID_MODE,
  DC1394_INVALID_FRAMERATE,
  DC1394_INVALID_TRIGGER_MODE,
  DC1394_INVALID_ISO_SPEED,
  DC1394_INVALID_IIDC_VERSION,
  DC1394_INVALID_COLOR_MODE,
  DC1394_INVALID_FORMAT7_COLOR_TILE,
  DC1394_REQ_VALUE_OUTSIDE_RANGE,
  DC1394_INVALID_ERROR_CODE,
  DC1394_MEMORY_ALLOCATION_FAILURE,
  DC1394_TAGGED_REGISTER_NOT_FOUND,
  DC1394_FORMAT7_ERROR_FLAG_1,
  DC1394_FORMAT7_ERROR_FLAG_2,
  DC1394_INVALID_BAYER_METHOD,
  DC1394_HANDLE_CREATION_FAILURE
} dc1394error_t;

#define DC1394_ERROR_NUM DC1394_HANDLE_CREATION_FAILURE+1

/* Parameter flags for dc1394_setup_format7_capture() */
#define DC1394_QUERY_FROM_CAMERA -1
#define DC1394_USE_MAX_AVAIL     -2
#define DC1394_USE_RECOMMENDED   -3

/* The video1394 policy: blocking (wait for a frame forever)
   or polling (returns if no frames in buffer) */
typedef enum { 
  DC1394_VIDEO1394_WAIT=0,
  DC1394_VIDEO1394_POLL
} dc1394videopolicy_t;

/* Yet another boolean data type */
typedef enum {
  DC1394_FALSE= 0,
  DC1394_TRUE
} dc1394bool_t;

/* Camera structure */
typedef struct __dc1394_camera
{
  // system/firmware information
  raw1394handle_t    handle;
  nodeid_t           node;
  int                port;
  uint64_t           euid_64;
  quadlet_t          ud_reg_tag_12;
  quadlet_t          ud_reg_tag_13;
  octlet_t           command_registers_base;
  octlet_t           unit_directory;
  octlet_t           unit_dependent_directory;
  octlet_t           advanced_features_csr;
  octlet_t           format7_csr[DC1394_MODE_FORMAT7_NUM];
  uint_t             iidc_version;
  char               vendor[MAX_CHARS + 1];
  char               model[MAX_CHARS + 1];
  dc1394bool_t       bmode_capable;
  dc1394bool_t       one_shot_capable;
  dc1394bool_t       multi_shot_capable;
  dc1394bool_t       adv_features_capable;

  // some current values
  uint_t             mode;
  uint_t             framerate;
  dc1394bool_t       is_iso_on;
  uint_t             iso_channel;
  uint_t             iso_speed;
  uint_t             mem_channel_number;
  uint_t             save_channel;
  uint_t             load_channel;
  
} dc1394camera_t;

typedef struct __dc1394_cam_cap_struct 
{
  nodeid_t           node;
  uint_t             channel;
  uint_t             frame_rate;
  uint_t             frame_width;
  uint_t             frame_height;
  int                *capture_buffer;
  uint_t             quadlets_per_frame;
  uint_t             quadlets_per_packet;
  /* components needed for the DMA based video capture */
  const uchar_t      *dma_ring_buffer;
  uint_t             dma_buffer_size;
  uint_t             dma_frame_size;
  uint_t             num_dma_buffers;
  uint_t             dma_last_buffer;
  uint_t             num_dma_buffers_behind;
  const char         *dma_device_file;
  int                dma_fd;
  uint_t             port;
  struct timeval     filltime;
  uint_t             drop_frames;
  raw1394handle_t    handle;
} dc1394capture_t ;

typedef struct __dc1394feature_t_struct 
{
  uint_t             feature_id;
  dc1394bool_t       available;
  dc1394bool_t       one_push;
  dc1394bool_t       absolute_capable;
  dc1394bool_t       readout_capable;
  dc1394bool_t       on_off_capable;
  dc1394bool_t       auto_capable;
  dc1394bool_t       manual_capable;
  dc1394bool_t       polarity_capable;
  dc1394bool_t       one_push_active;
  dc1394bool_t       is_on;
  dc1394bool_t       auto_active;
  char               trigger_mode_capable_mask;
  uint_t             trigger_mode;
  dc1394bool_t       trigger_polarity;
  uint_t             min;
  uint_t             max;
  uint_t             value;
  uint_t             BU_value;
  uint_t             RV_value;
  uint_t             B_value;
  uint_t             R_value;
  uint_t             G_value;
  uint_t             target_value;
  
  dc1394bool_t       abs_control;
  float              abs_value;
  float              abs_max;
  float              abs_min;
  
} dc1394feature_t;

typedef struct __dc1394featureset_t_struct 
{
  dc1394feature_t    feature[DC1394_FEATURE_NUM];
} dc1394featureset_t;

typedef struct
{
  uint_t             num;
  uint_t             modes[DC1394_COLOR_CODING_NUM];
} dc1394colormodes_t;

typedef struct
{
  uint_t             num;
  uint_t             modes[DC1394_MODE_FORMAT0_NUM+DC1394_MODE_FORMAT1_NUM+DC1394_MODE_FORMAT2_NUM+DC1394_MODE_FORMAT6_NUM+DC1394_MODE_FORMAT7_NUM];
} dc1394videomodes_t;

typedef struct
{
  uint_t             num;
  uint_t             framerates[DC1394_FRAMERATE_NUM];
} dc1394framerates_t;

/* Feature descriptions */
extern const char *dc1394_feature_desc[DC1394_FEATURE_NUM];
extern const char *dc1394_error_strings[DC1394_ERROR_NUM];

/* Error checking function. displays an error string on stderr and exit current function
   if error is positive. Neg errors are messages and are thus ignored */

#define DC1394_ERR_CHK(err, err_string...)                   \
                                                             \
    if ((err<0)||(err>DC1394_ERROR_NUM))                     \
      err=DC1394_INVALID_ERROR_CODE;                         \
                                                             \
    if (err>DC1394_SUCCESS) {                                \
      fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ",    \
	      __FILE__, __FUNCTION__, __LINE__,              \
	      dc1394_error_strings[err]);                    \
      fprintf(stderr, err_string);                           \
      fprintf(stderr,"\n");                                  \
      return err;                                            \
    }

#define DC1394_ERR_CHK_WITH_CLEANUP(err, cleanup, err_string...)     \
                                                                     \
    if ((err<0)||(err>DC1394_ERROR_NUM))                             \
      err=DC1394_INVALID_ERROR_CODE;                                 \
                                                                     \
    if (err>DC1394_SUCCESS) {                                        \
      fprintf(stderr,"Libdc1394 error (%s:%s:%d): %s : ",            \
	      __FILE__, __FUNCTION__, __LINE__,                      \
	      dc1394_error_strings[err]);                            \
      fprintf(stderr, err_string);                                   \
      fprintf(stderr,"\n");                                          \
      cleanup;                                                       \
      return err;                                                    \
    }

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     General system functions
 ***************************************************************************/

/* locate and initialise the cameras */
int dc1394_find_cameras(dc1394camera_t ***cameras_ptr, uint_t* numCameras);

/* create / free camera structure */
dc1394camera_t* dc1394_new_camera(uint_t port, nodeid_t node);
void dc1394_free_camera(dc1394camera_t *camera);

/* get / print the camera information */
int dc1394_get_camera_info(dc1394camera_t *camera); // to be hidden
int dc1394_print_camera_info(dc1394camera_t *camera);

/***************************************************************************
     Other functionalities
 ***************************************************************************/

/* reset camera to factory default settings */
int dc1394_reset_camera(dc1394camera_t *camera);

/* to be removed from API? */
int dc1394_query_revision(dc1394camera_t *camera, uint_t mode, quadlet_t *value);
int dc1394_query_basic_functionality(dc1394camera_t *camera, quadlet_t *value);
int dc1394_query_feature_characteristics(dc1394camera_t *camera, uint_t feature, quadlet_t *value);

/* turn camera on or off */
int dc1394_camera_on(dc1394camera_t *camera); // to be merged with the one below
int dc1394_camera_off(dc1394camera_t *camera);

/* functions to read and write camera setups in memory channels */
int dc1394_get_memory_load_ch(dc1394camera_t *camera, uint_t *channel);
int dc1394_get_memory_save_ch(dc1394camera_t *camera, uint_t *channel);
int dc1394_is_memory_save_in_operation(dc1394camera_t *camera, dc1394bool_t *value);
int dc1394_set_memory_save_ch(dc1394camera_t *camera, uint_t channel); // to be merged with the one below
int dc1394_memory_save(dc1394camera_t *camera);
int dc1394_memory_load(dc1394camera_t *camera, uint_t channel);

/* trigger feature functions */
int dc1394_set_trigger_polarity(dc1394camera_t *camera, dc1394bool_t polarity);
int dc1394_get_trigger_polarity(dc1394camera_t *camera, dc1394bool_t *polarity);
int dc1394_trigger_has_polarity(dc1394camera_t *camera, dc1394bool_t *polarity);
int dc1394_set_trigger_on_off(dc1394camera_t *camera, dc1394bool_t on_off);
int dc1394_get_trigger_on_off(dc1394camera_t *camera, dc1394bool_t *on_off);
int dc1394_set_trigger_mode(dc1394camera_t *camera, uint_t mode);
  int dc1394_get_trigger_mode(dc1394camera_t *camera, uint_t *mode);
/* Turn one software trigger on or off and get state */
int dc1394_set_soft_trigger(dc1394camera_t *camera);
int dc1394_unset_soft_trigger(dc1394camera_t *camera);
int dc1394_get_soft_trigger(dc1394camera_t *camera, dc1394bool_t *is_on);

/***************************************************************************
     Features
 ***************************************************************************/

/* Collects the available features for the camera described by node and stores them in features. */
int dc1394_get_camera_feature_set(dc1394camera_t *camera, dc1394featureset_t *features);

/* Stores the bounds and options associated with the feature described by feature->feature_id */
int dc1394_get_camera_feature(dc1394camera_t *camera, dc1394feature_t *feature);

/* Displays the bounds and options of the given feature or of the entire feature set */
int dc1394_print_feature(dc1394feature_t *feature);
int dc1394_print_feature_set(dc1394featureset_t *features);

/* get/set the values of multiple-value features on the camera */
int dc1394_get_white_balance(dc1394camera_t *camera, uint_t *u_b_value, uint_t *v_r_value);
int dc1394_set_white_balance(dc1394camera_t *camera, uint_t u_b_value, uint_t v_r_value);
int dc1394_get_temperature(dc1394camera_t *camera, uint_t *target_temperature, uint_t *temperature);
int dc1394_set_temperature(dc1394camera_t *camera, uint_t target_temperature);
int dc1394_get_white_shading(dc1394camera_t *camera, uint_t *r_value, uint_t *g_value, uint_t *b_value);
int dc1394_set_white_shading(dc1394camera_t *camera, uint_t r_value, uint_t g_value, uint_t b_value);

/* get/set the values of single-value features on the camera */
int dc1394_get_feature_value(dc1394camera_t *camera, uint_t feature, uint_t *value);
int dc1394_set_feature_value(dc1394camera_t *camera, uint_t feature, uint_t value);

/* query/set specific feature characteristics */
int dc1394_is_feature_present(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_has_one_push_auto(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_is_one_push_in_operation(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_start_one_push_operation(dc1394camera_t *camera, uint_t feature);
int dc1394_can_read_out(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_can_turn_on_off(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_is_feature_on(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_feature_on_off(dc1394camera_t *camera, uint_t feature, uint_t value);/* 0=off, nonzero=on */
int dc1394_has_auto_mode(dc1394camera_t *camera,  uint_t feature, dc1394bool_t *value);
int dc1394_has_manual_mode(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_is_feature_auto(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_auto_on_off(dc1394camera_t *camera, uint_t feature, uint_t value); /* 0=off, nonzero=on */
int dc1394_get_min_value(dc1394camera_t *camera, uint_t feature, uint_t *value);
int dc1394_get_max_value(dc1394camera_t *camera, uint_t feature, uint_t *value);

/* absolute settings */
int dc1394_query_absolute_feature_min_max(dc1394camera_t *camera, uint_t feature, float *min, float *max);
int dc1394_query_absolute_feature_value(dc1394camera_t *camera, uint_t feature, float *value);
int dc1394_set_absolute_feature_value(dc1394camera_t *camera, uint_t feature, float value);
int dc1394_query_absolute_control(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_absolute_setting_on_off(dc1394camera_t *camera, uint_t feature, uint_t value);
int dc1394_has_absolute_control(dc1394camera_t *camera, uint_t feature, dc1394bool_t *value);
int dc1394_get_bandwidth_usage(dc1394camera_t *camera, uint_t *bandwidth);

/***************************************************************************
     Video formats
 ***************************************************************************/

/* functions for querying camera attributes */
int dc1394_query_supported_modes(dc1394camera_t *camera, dc1394videomodes_t *modes);
int dc1394_query_supported_framerates(dc1394camera_t *camera, uint_t mode, dc1394framerates_t *framerates);

/* get/set the framerate, mode, format, iso channel/speed for the video */
int dc1394_get_video_framerate(dc1394camera_t *camera, uint_t *framerate);
int dc1394_set_video_framerate(dc1394camera_t *camera, uint_t framerate);
int dc1394_get_video_mode(dc1394camera_t *camera, uint_t *mode);
int dc1394_set_video_mode(dc1394camera_t *camera, uint_t mode);
int dc1394_get_operation_mode(dc1394camera_t *camera, uint_t *mode);
int dc1394_set_operation_mode(dc1394camera_t *camera, uint_t mode);
int dc1394_get_iso_channel_and_speed(dc1394camera_t *camera, uint_t *channel, uint_t *speed);
int dc1394_set_iso_channel_and_speed(dc1394camera_t *camera, uint_t channel, uint_t speed);
 
/* start/stop isochronous data transmission */
int dc1394_start_iso_transmission(dc1394camera_t *camera);
int dc1394_stop_iso_transmission(dc1394camera_t *camera);
int dc1394_get_iso_status(dc1394camera_t *camera, dc1394bool_t *is_on);

/* turn one shot mode on or off */
int dc1394_set_one_shot(dc1394camera_t *camera);
int dc1394_unset_one_shot(dc1394camera_t *camera);

/* turn multishot mode on or off */
int dc1394_set_multi_shot(dc1394camera_t *camera, uint_t numFrames);
int dc1394_unset_multi_shot(dc1394camera_t *camera);
int dc1394_get_one_shot(dc1394camera_t *camera, dc1394bool_t *is_on);
int dc1394_get_multi_shot(dc1394camera_t *camera, dc1394bool_t *is_on, uint_t *numFrames);

/***************************************************************************
     Capture Functions
 ***************************************************************************/
/* Note: the DMA functions are much faster and should be used in most cases.
         Legacy libraw1394 transfers are only there for debug purposes.    */

/* setup the DMA capture */
int dc1394_dma_setup_capture(dc1394camera_t *camera,
			     uint_t channel, uint_t mode, uint_t speed, uint_t frame_rate, 
			     uint_t num_dma_buffers, uint_t drop_frames, const char *dma_device_file,
			     dc1394capture_t *capture);
int dc1394_dma_setup_format7_capture(dc1394camera_t *camera,
                                 uint_t channel, uint_t mode, uint_t speed, uint_t bytes_per_packet,
                                 uint_t left, uint_t top, uint_t width, uint_t height,
                                 uint_t num_dma_buffers, uint_t drop_frames, const char *dma_device_file,
                                 dc1394capture_t *capture);
/* captures a frame from the given cameras. */
int dc1394_dma_capture(dc1394capture_t *cams, uint_t num, dc1394videopolicy_t policy);
/* returns the buffer previously handed to the user by dc1394_dma_*_capture to the DMA ring buffer */
int dc1394_dma_done_with_buffer(dc1394capture_t * capture);
/* tells video1394 to halt iso reception. */
int dc1394_dma_unlisten(dc1394capture_t *capture);
/* releases memory that was mapped by dc1394_dma_setup_camera */
int dc1394_dma_release_capture(dc1394capture_t *capture);


/* Non DMA capture functions for legacy/debug purposes */
int dc1394_setup_capture(dc1394camera_t *camera, 
			 uint_t channel, uint_t mode, uint_t speed, uint_t frame_rate, 
			 dc1394capture_t * capture);
int dc1394_setup_format7_capture(dc1394camera_t *camera,
				 uint_t channel, uint_t mode, uint_t speed, uint_t bytes_per_packet,
				 uint_t left, uint_t top, uint_t width, uint_t height, 
				 dc1394capture_t * capure);
int dc1394_capture(dc1394capture_t *capture, uint_t num);
int dc1394_release_capture(dc1394capture_t *capture);

/***************************************************************************
     Format_7 (scalable image format)
 ***************************************************************************/

/* image size */
int dc1394_query_format7_max_image_size(dc1394camera_t *camera, uint_t mode, uint_t *h_size,uint_t *v_size);
int dc1394_query_format7_unit_size(dc1394camera_t *camera, uint_t mode, uint_t *h_unit, uint_t *v_unit);
int dc1394_query_format7_image_size(dc1394camera_t *camera, uint_t mode, uint_t *width, uint_t *height);
int dc1394_set_format7_image_size(dc1394camera_t *camera, uint_t mode, uint_t width, uint_t height);
/* image position */
int dc1394_query_format7_image_position(dc1394camera_t *camera, uint_t mode, uint_t *left, uint_t *top);
int dc1394_set_format7_image_position(dc1394camera_t *camera, uint_t mode, uint_t left, uint_t top);
int dc1394_query_format7_unit_position(dc1394camera_t *camera, uint_t mode, uint_t *h_unit_pos, uint_t *v_unit_pos);

/* color coding */
int dc1394_query_format7_color_coding_id(dc1394camera_t *camera, uint_t mode, uint_t *color_id);
int dc1394_query_format7_color_coding(dc1394camera_t *camera, uint_t mode, dc1394colormodes_t *codings);
int dc1394_set_format7_color_coding_id(dc1394camera_t *camera, uint_t mode, uint_t color_id);
int dc1394_set_format7_color_filter_id(dc1394camera_t *camera, uint_t mode, uint_t color_id);
int dc1394_query_format7_color_filter_id(dc1394camera_t *camera, uint_t mode, uint_t *color_id);

/* packet */
int dc1394_query_format7_packet_para(dc1394camera_t *camera, uint_t mode, uint_t *min_bytes, uint_t *max_bytes);
int dc1394_query_format7_byte_per_packet(dc1394camera_t *camera, uint_t mode, uint_t *packet_bytes);
int dc1394_set_format7_byte_per_packet(dc1394camera_t *camera, uint_t mode, uint_t packet_bytes);
int dc1394_query_format7_recommended_byte_per_packet(dc1394camera_t *camera, uint_t mode, uint_t *bpp);
int dc1394_query_format7_packet_per_frame(dc1394camera_t *camera, uint_t mode, uint_t *ppf);

/* value setting control */
int dc1394_query_format7_value_setting(dc1394camera_t *camera, uint_t mode, uint_t *present,
				       uint_t *setting1, uint_t *err_flag1, uint_t *err_flag2);
int dc1394_set_format7_value_setting(dc1394camera_t *camera, uint_t mode);

/* other */
int dc1394_query_format7_data_depth(dc1394camera_t *camera, uint_t mode, uint_t *data_depth);
int dc1394_query_format7_frame_interval(dc1394camera_t *camera, uint_t mode, float *interval);
int dc1394_query_format7_pixel_number(dc1394camera_t *camera, uint_t mode, uint_t *pixnum);
int dc1394_query_format7_total_bytes(dc1394camera_t *camera, uint_t mode, uint64_t *total_bytes);

#ifdef __cplusplus
}
#endif

#endif /* __DC1394_CAMERA_CONTROL_H__ */
