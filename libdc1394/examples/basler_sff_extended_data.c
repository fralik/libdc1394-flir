/*
 * 1394-Based Digital Camera Control Library
 * Basler Smart Feature Framework specific extensions
 * Copyright (C) 2006 Mikael Olenfalk, Tobii Technology AB, Stockholm Sweden
 *
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 * Version : 16/02/2005 
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

#include <stdio.h>
#include <stdlib.h>
#include <dc1394/control.h>
#include <dc1394/vendor/basler.h>

int list_cameras (dc1394camera_t **cameras, uint32_t num_cameras)
{
  uint32_t i;
  dc1394bool_t sff_available;

  for (i = 0; i < num_cameras; i++) {
    sff_available = DC1394_FALSE;
    dc1394_basler_sff_is_available (cameras[i], &sff_available);

    printf ("%02d:0x%016llx:%s:%s:%s\n", i, cameras[i]->euid_64, cameras[i]->vendor, cameras[i]->model, sff_available ? "SFF" : "NO SFF");
  }
  return 0;
}

void cleanup (dc1394camera_t** cameras, uint32_t num_cameras)
{
  uint32_t i;
  for (i = 0; i < num_cameras; i++) {
    dc1394_free_camera(cameras[i]);
  }
  free (cameras);
}

int print_usage ()
{
  printf ("usage: basler_sff_extended_data [--list-cameras|[--euid EUID]]\n");
  return 1;
}

int main (int argc, char **argv)
{
  dc1394error_t err;
  dc1394camera_t **cameras, *camera = NULL;
  dc1394bool_t sff_available;
  uint32_t num_cameras, i, max_height, max_width;
  uint64_t euid = 0x0LL, total_bytes;
  uint8_t* buffer; 
  dc1394basler_sff_t chunk;
  dc1394basler_sff_extended_data_stream_t* sff_ext;
  dc1394basler_sff_dcam_values_t* sff_dcam;

  err = dc1394_find_cameras (&cameras, &num_cameras);
  if (err != DC1394_SUCCESS) {
    fprintf (stderr, "E: cannot look for cameras\n");
    return 1;
  }

  if (num_cameras == 0) {
    fprintf (stderr, "E: no cameras found\n");
    cleanup (cameras, num_cameras);
    return 1;
  }

  /* parse arguments */
  if (argc == 2) {
    if (!strcmp (argv[1], "--list-cameras")) {
      list_cameras (cameras, num_cameras);
      cleanup (cameras, num_cameras);
      return 0;
    } else {
      cleanup (cameras, num_cameras);
      return print_usage();
    }  
  } else if (argc == 3) {
    if (!strcmp (argv[1], "--euid")) {
      if (sscanf (argv[2], "0x%llx", &euid) == 1) {
      } else {
        cleanup (cameras, num_cameras);
        return print_usage();
      }
    }
  } else if (argc != 1) {
    cleanup (cameras, num_cameras);
    return print_usage();
  }

  if (euid == 0x0LL) {
    printf ("I: found %d camera%s, working with camera 0\n", num_cameras, num_cameras == 1 ? "" : "s");
    camera = cameras[0];
    for (i = 1; i < num_cameras; i ++) {
      dc1394_free_camera(cameras[i]);
    }
    free (cameras);
  } else {
    for (i = 0; i < num_cameras; i ++) {
      if (cameras[i]->euid_64 == euid) {
        camera = cameras[i];
      } else {
        dc1394_free_camera(cameras[i]);
      }
    }
    free (cameras);

    if (camera == NULL) {
      fprintf (stderr, "E: no camera with euid 0x%016llx found\n", euid);
      return 1;
    }

    printf ("I: found camera with euid 0x%016llx\n", euid);
  }

  /*
   * setup format7 with a roi allocating a quarter of the screen and bounce around the roi, while changing gain and brightness
   */
  err = dc1394_video_set_mode (camera, DC1394_VIDEO_MODE_FORMAT7_0);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot choose format7_0\n");
    return 2;
  }
  printf ("I: video mode is format7_0\n");

  err = dc1394_feature_set_value (camera, DC1394_FEATURE_GAIN, 100);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot set gain\n");
    return 2;
  }
  printf ("I: gain is 100\n");

  err = dc1394_feature_set_value (camera, DC1394_FEATURE_SHUTTER, 50);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot set shutter\n");
    return 2;
  }
  printf ("I: shutter is 50\n");

  err = dc1394_feature_set_value (camera, DC1394_FEATURE_BRIGHTNESS, 150);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot set brightness\n");
    return 2;
  }
  printf ("I: brightness is 150\n");

  err = dc1394_format7_get_max_image_size (camera, DC1394_VIDEO_MODE_FORMAT7_0, &max_width, &max_height);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot get max image size for format7_0\n");
    return 2;
  }
  printf ("I: max image size is: height = %d, width = %d\n", max_height, max_width);

  err = dc1394_format7_set_roi (camera, DC1394_VIDEO_MODE_FORMAT7_0, DC1394_COLOR_CODING_MONO8,
                                DC1394_USE_RECOMMENDED, 0, 0, max_width / 2, max_height / 2);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot set roi\n");
    return 2;
  }
  printf ("I: roi is (0, 0) - (%d, %d)\n", max_width / 2, max_height / 2);

  err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot get total bytes\n");
    return 2;
  }
  printf ("I: total bytes is %llu before SFF enabled\n", total_bytes);

  err = dc1394_basler_sff_is_available (camera, &sff_available);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot check if SFF is available\n");
    return 2;
  }
  
  if (!sff_available) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: SFF is not available\n");
    return 2;
  } else {
    printf ("I: SFF is available\n");
  }

  err = dc1394_basler_sff_feature_enable (camera, DC1394_BASLER_SFF_EXTENDED_DATA_STREAM, DC1394_ON);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot enable Extended_Data_Stream\n");
    return 2;
  }

  err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot get total bytes\n");
    return 2;
  }
  printf ("I: total bytes is %llu after Extended_Data_Stream was enabled\n", total_bytes);

  err = dc1394_basler_sff_feature_enable (camera, DC1394_BASLER_SFF_DCAM_VALUES, DC1394_ON);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot enable DCAM_Values\n");
    return 2;
  }

  err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot get total bytes\n");
    return 2;
  }
  printf ("I: total bytes is %llu after DCAM_Values was enabled\n", total_bytes);

  err = dc1394_capture_setup (camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
  if (err != DC1394_SUCCESS) {
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot setup capture\n");
    return 2;
  }

  err = dc1394_video_set_transmission (camera, DC1394_ON);
  if (err != DC1394_SUCCESS) {
    dc1394_capture_stop (camera);
    dc1394_free_camera (camera);
    fprintf (stderr, "E: cannot enable transmission\n");
    return 2;
  } 

  i = 0;
  dc1394video_frame_t* frame;
  while (i < 100) {
    err = dc1394_capture_dequeue (camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (err != DC1394_SUCCESS) {
      fprintf (stderr, "E: capture failed (%d)\n", err);
      break;
    }
    buffer = frame->image;
    
    /* parse chunks and print */
    // printf ("-- %02d - %d\n", i, dc1394_capture_get_bytes_per_frame(camera));
    dc1394_basler_sff_chunk_iterate_init (&chunk, buffer, total_bytes, DC1394_FALSE);
    while ((err = dc1394_basler_sff_chunk_iterate(&chunk)) == DC1394_SUCCESS) {
      switch (chunk.feature_id) {
        case DC1394_BASLER_SFF_EXTENDED_DATA_STREAM:
          {
            sff_ext = (dc1394basler_sff_extended_data_stream_t*)(chunk.chunk_data);
            printf ("top: %04d left: %04d height: %04d width: %04d\n",
                    sff_ext->top, sff_ext->left, sff_ext->height, sff_ext->width);
            break;
          }
        case DC1394_BASLER_SFF_DCAM_VALUES:
          {
            sff_dcam = (dc1394basler_sff_dcam_values_t*)(chunk.chunk_data);
            printf ("gain: %04d brightness: %04d shutter: %04d\n",
                    sff_dcam->gain_csr.value, sff_dcam->brightness_csr.value, sff_dcam->shutter_csr.value);
            break;
          }
        default:
          break;
      }
    }
    
    if (err != DC1394_BASLER_NO_MORE_SFF_CHUNKS) {
      fprintf (stderr, "E: error parsing chunks\n");
      break;
    }
    
    err = dc1394_capture_enqueue(camera, frame);
    if (err != DC1394_SUCCESS) {
      fprintf (stderr, "E: cannot release buffer\n");
      break;
    }

    err = dc1394_format7_set_image_position (camera, DC1394_VIDEO_MODE_FORMAT7_0, (((i+1) * 10) % max_height) / 2 - 1, (((i+1) * 10) % max_width) / 2 - 1);
    if (err != DC1394_SUCCESS) {
      fprintf (stderr, "E: cannot set image position %d - %d\n", (((i+1) * 10) % max_height) / 2 - 1, (((i+1) * 10) % max_width) / 2 - 1);
    }
    i++;
  }

  dc1394_video_set_transmission (camera, DC1394_OFF);
  dc1394_capture_stop (camera);
  dc1394_free_camera (camera);
  return 0;
}
