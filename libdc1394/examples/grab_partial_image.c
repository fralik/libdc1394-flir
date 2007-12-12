/**************************************************************************
**       Title: grab partial images from camera, measure frame rate
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Grab partial image from camera. Camera must be format 7
**    (scalable image size) compatible. e.g., Basler A101f
**
**-------------------------------------------------------------------------
**
**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#ifdef _WIN32
#define times 0**(int*)
struct tms {int a;};
#else
#include <sys/times.h>
#endif

int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t *camera;
  int grab_n_frames = 10;
  struct tms tms_buf;
  clock_t start_time;
  float elapsed_time;
  int i;
  unsigned int min_bytes, max_bytes;
  unsigned int actual_bytes;
  uint64_t total_bytes = 0;
  unsigned int width, height;
  dc1394video_frame_t *frame=NULL;
  dc1394_t * d;
  dc1394camera_list_t * list;
  dc1394error_t err;

  d = dc1394_new ();
  err=dc1394_camera_enumerate (d, &list);
  DC1394_ERR_RTN(err,"Failed to enumerate cameras\n");

  if (list->num == 0) {
    dc1394_log_error("No cameras found\n");
    return 1;
  }
  
  camera = dc1394_camera_new (d, list->ids[0].guid);
  if (!camera) {
    dc1394_log_error("Failed to initialize camera with guid %"PRIx64"\n", list->ids[0].guid);
    return 1;
  }
  dc1394_camera_free_list (list);

  printf("Using camera with GUID %"PRIx64"\n", camera->guid);

  /*-----------------------------------------------------------------------
   *  setup capture for format 7
   *-----------------------------------------------------------------------*/

  dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
  dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_FORMAT7_0);
  err = dc1394_format7_set_roi(camera, DC1394_VIDEO_MODE_FORMAT7_0,
			 DC1394_COLOR_CODING_MONO8,
			 DC1394_USE_MAX_AVAIL, // use max packet size
			 0, 0, // left, top
			 640, 480);  // width, height
  DC1394_ERR_RTN(err,"Unable to set Format7 mode 0.\nEdit the example file manually to fit your camera capabilities\n");

  err=dc1394_capture_setup(camera, 4, DC1394_CAPTURE_FLAGS_DEFAULT);
  DC1394_ERR_CLN_RTN(err, dc1394_camera_free(camera), "Error capturing\n");

  /*-----------------------------------------------------------------------
   *  print allowed and used packet size
   *-----------------------------------------------------------------------*/
  err=dc1394_format7_get_packet_parameters(camera, DC1394_VIDEO_MODE_FORMAT7_0, &min_bytes, &max_bytes);
  DC1394_ERR_RTN(err,"Packet para inq error\n");
  printf( "camera reports allowed packet size from %d - %d bytes\n", min_bytes, max_bytes);

  err=dc1394_format7_get_packet_size(camera, DC1394_VIDEO_MODE_FORMAT7_0, &actual_bytes);
  DC1394_ERR_RTN(err,"dc1394_format7_get_packet_size error\n");
  printf( "camera reports actual packet size = %d bytes\n",
          actual_bytes);

  err=dc1394_format7_get_total_bytes(camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes);
  DC1394_ERR_RTN(err,"dc1394_query_format7_total_bytes error\n");
  printf( "camera reports total bytes per frame = %"PRIx64" bytes\n", total_bytes);

  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  err=dc1394_video_set_transmission(camera,DC1394_ON);
  if (err!=DC1394_SUCCESS) {
    dc1394_log_error("unable to start camera iso transmission\n");
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  capture 10 frames and measure the time for this operation
   *-----------------------------------------------------------------------*/
  start_time = times(&tms_buf);

  for( i = 0; i < grab_n_frames; ++i) {
    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (err!=DC1394_SUCCESS) {
      dc1394_log_error("unable to capture\n");
      dc1394_capture_stop(camera);
      dc1394_camera_free(camera);
      exit(1);
    }

    /*---------------------------------------------------------------------
     *  output elapsed time
     *---------------------------------------------------------------------*/
    elapsed_time = (float)(times(&tms_buf) - start_time) / CLOCKS_PER_SEC;
    printf( "got frame %d. elapsed time: %g sec ==> %g frames/second\n",
            i, elapsed_time, (float)i / elapsed_time);

    // release buffer
    dc1394_capture_enqueue(camera,frame);
  }
  
  /*-----------------------------------------------------------------------
   *  Stop data transmission
   *-----------------------------------------------------------------------*/
  err=dc1394_video_set_transmission(camera,DC1394_OFF);
  DC1394_ERR_RTN(err,"couldn't stop the camera?\n");

  /*-----------------------------------------------------------------------
   *  save last image as Part.pgm
   *-----------------------------------------------------------------------*/
  imagefile=fopen("Part.pgm","wb");
    
  dc1394_get_image_size_from_video_mode(camera, DC1394_VIDEO_MODE_FORMAT7_0, &width, &height);
  fprintf(imagefile,"P5\n%u %u\n255\n", width, height);
  fwrite(frame->image, 1, height * width, imagefile);
  fclose(imagefile);
  printf("wrote: Part.pgm\n");
  
  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_capture_stop(camera);
  dc1394_video_set_transmission(camera, DC1394_OFF);
  dc1394_camera_free(camera);
  dc1394_free (d);
  return 0;
}
