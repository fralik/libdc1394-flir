
#include <dc1394/log.h>
#include <dc1394/video.h>
#include <dc1394/dc1394.h>

#ifndef __DC1394_FORMAT7_H__
#define __DC1394_FORMAT7_H__

typedef struct __dc1394format7mode_t
{
  dc1394bool_t present;

  uint32_t size_x;
  uint32_t size_y;
  uint32_t max_size_x;
  uint32_t max_size_y;

  uint32_t pos_x;
  uint32_t pos_y;

  uint32_t unit_size_x;
  uint32_t unit_size_y;
  uint32_t unit_pos_x;
  uint32_t unit_pos_y;

  dc1394color_codings_t color_codings;
  dc1394color_coding_t color_coding;

  uint32_t pixnum;

  uint32_t packet_size; /* in bytes */
  uint32_t unit_packet_size;
  uint32_t max_packet_size;

  uint64_t total_bytes;

  dc1394color_filter_t color_filter;

} dc1394format7mode_t;

typedef struct __dc1394format7modeset_t
{
  dc1394format7mode_t mode[DC1394_VIDEO_MODE_FORMAT7_NUM];
} dc1394format7modeset_t;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Format_7 (scalable image format)
 ***************************************************************************/

/* image size */
dc1394error_t dc1394_format7_get_max_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_size,uint32_t *v_size);
dc1394error_t dc1394_format7_get_unit_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_unit, uint32_t *v_unit);
dc1394error_t dc1394_format7_get_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *width, uint32_t *height);
dc1394error_t dc1394_format7_set_image_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t width, uint32_t height);

/* image position */
dc1394error_t dc1394_format7_get_image_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *left, uint32_t *top);
dc1394error_t dc1394_format7_set_image_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t left, uint32_t top);
dc1394error_t dc1394_format7_get_unit_position(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *h_unit_pos, uint32_t *v_unit_pos);

/* color coding */
dc1394error_t dc1394_format7_get_color_coding(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding);
dc1394error_t dc1394_format7_get_color_codings(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_codings_t *codings);
dc1394error_t dc1394_format7_set_color_coding(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t color_coding);
dc1394error_t dc1394_format7_get_color_filter(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_filter_t *color_filter);

/* packet */
dc1394error_t dc1394_format7_get_packet_parameters(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *unit_bytes, uint32_t *max_bytes);
dc1394error_t dc1394_format7_get_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *packet_size);
dc1394error_t dc1394_format7_set_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t packet_size);
dc1394error_t dc1394_format7_get_recommended_packet_size(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *packet_size);
dc1394error_t dc1394_format7_get_packets_per_frame(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *ppf);

/* other */
dc1394error_t dc1394_format7_get_data_depth(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *data_depth);
dc1394error_t dc1394_format7_get_frame_interval(dc1394camera_t *camera, dc1394video_mode_t video_mode, float *interval);
dc1394error_t dc1394_format7_get_pixel_number(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint32_t *pixnum);
dc1394error_t dc1394_format7_get_total_bytes(dc1394camera_t *camera, dc1394video_mode_t video_mode, uint64_t *total_bytes);

/* These functions get the properties of (one or all) format7 mode(s) */
dc1394error_t dc1394_format7_get_modeset(dc1394camera_t *camera, dc1394format7modeset_t *info);
dc1394error_t dc1394_format7_get_mode_info(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394format7mode_t *f7_mode);

/* Joint function that fully sets a certain ROI taking all parameters into account
   Note that this function does not SWITCH to the video mode passed as argument, it mearly sets it */
dc1394error_t dc1394_format7_set_roi(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t color_coding,
				     int32_t packet_size, int32_t left, int32_t top, int32_t width, int32_t height);

dc1394error_t dc1394_format7_get_roi(dc1394camera_t *camera, dc1394video_mode_t video_mode, dc1394color_coding_t *color_coding,
				     uint32_t *packet_size, uint32_t *left, uint32_t *top, uint32_t *width, uint32_t *height);

#ifdef __cplusplus
}
#endif

#endif
