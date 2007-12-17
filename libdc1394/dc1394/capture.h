
#include <dc1394/log.h>
#include <dc1394/video.h>
#include <dc1394/dc1394.h>

#ifndef __DC1394_CAPTURE_H__
#define __DC1394_CAPTURE_H__

/*! \file capture.h
    \brief Capture functions

    More details soon
*/

/**
 * The capture policy: blocking (wait for a frame forever) or polling (returns if no frames in buffer)
 */
typedef enum {
    DC1394_CAPTURE_POLICY_WAIT=672,
    DC1394_CAPTURE_POLICY_POLL
} dc1394capture_policy_t;
#define DC1394_CAPTURE_POLICY_MIN    DC1394_CAPTURE_POLICY_WAIT
#define DC1394_CAPTURE_POLICY_MAX    DC1394_CAPTURE_POLICY_POLL
#define DC1394_CAPTURE_POLICY_NUM   (DC1394_CAPTURE_POLICY_MAX - DC1394_CAPTURE_POLICY_MIN + 1)

/**
 * Capture flags
 */
#define DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC   0x00000001U
#define DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC 0x00000002U
#define DC1394_CAPTURE_FLAGS_DEFAULT         0x00000004U /* a reasonable default value: do bandwidth and channel allocation */
#define DC1394_CAPTURE_FLAGS_AUTO_ISO        0x00000008U /* automatically start iso before capture and stop it after */

/* Parameter flags for dc1394_setup_format7_capture() */
#define DC1394_QUERY_FROM_CAMERA -1
#define DC1394_USE_MAX_AVAIL     -2
#define DC1394_USE_RECOMMENDED   -3

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
     Capture Functions
 ***************************************************************************/

/**
 * setup the capture
 */
dc1394error_t dc1394_capture_setup(dc1394camera_t *camera, uint32_t num_dma_buffers, uint32_t flags);

/**
 * stop the capture
 */
dc1394error_t dc1394_capture_stop(dc1394camera_t *camera);

/**
 * Get a file descriptor to be used for select().  Must be called after capture_setup().
 */
int dc1394_capture_get_fileno (dc1394camera_t * camera);

/**
 * capture video frames
 */
dc1394error_t dc1394_capture_dequeue(dc1394camera_t * camera, dc1394capture_policy_t policy, dc1394video_frame_t **frame);

/**
 * No Docs
 */
dc1394error_t dc1394_capture_enqueue(dc1394camera_t * camera, dc1394video_frame_t * frame);

#ifdef __cplusplus
}
#endif

#endif
