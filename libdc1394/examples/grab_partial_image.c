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
#include "libdc1394/dc1394_utils.h"
#include "libdc1394/dc1394_control.h"
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>


int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t *camera, **cameras=NULL;
  uint32_t numCameras;
  int grab_n_frames = 10;
  struct tms tms_buf;
  clock_t start_time;
  float elapsed_time;
  int i;
  unsigned int min_bytes, max_bytes;
  unsigned int actual_bytes;
  unsigned long long int total_bytes = 0;
  unsigned int width, height;

  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS && err != DC1394_NO_CAMERA) {
    fprintf( stderr, "Unable to look for cameras\n\n"
             "On Linux, please check \n"
	     "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - if you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  get the camera nodes and describe them as we find them
   *-----------------------------------------------------------------------*/
  if (numCameras<1) {
    fprintf(stderr, "no cameras found :(\n");
    exit(1);
  }
  camera=cameras[0];
  printf("working with the first camera on the bus\n");
  
  // free the other cameras
  for (i=1;i<numCameras;i++)
    dc1394_free_camera(cameras[i]);
  free(cameras);
  
  //fprintf(stderr,"handle: 0x%x, node: 0x%x\n",camera->handle,camera->node);

  /*-----------------------------------------------------------------------
   *  setup capture for format 7
   *-----------------------------------------------------------------------*/

  //fprintf(stderr,"handle: 0x%x\n",capture.handle);

  dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
  dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_FORMAT7_1);
  dc1394_format7_set_roi(camera, DC1394_VIDEO_MODE_FORMAT7_1,
			 DC1394_COLOR_CODING_MONO8,
			 DC1394_USE_MAX_AVAIL, // use max packet size
			 0, 0, // left, top
			 1280, 1024);  // width, height
  DC1394_ERR_RTN(err,"Unable to set Format7 mode 0.\nEdit the example file manually to fit your camera capabilities\n");

  err=dc1394_capture_setup(camera);
  DC1394_ERR_CLN_RTN(err, dc1394_free_camera(camera), "Error capturing\n");

  //fprintf(stderr,"handle: 0x%x\n",capture.handle);

  /* set trigger mode */
  /*
  if( dc1394_set_trigger_mode(camera, TRIGGER_MODE_0)
      != DC1394_SUCCESS)
  {
    fprintf( stderr, "unable to set camera trigger mode\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }
  fprintf(stderr,"handle: 0x%x\n",capture.handle);
  */
  /*-----------------------------------------------------------------------
   *  print allowed and used packet size
   *-----------------------------------------------------------------------*/
  if (dc1394_format7_get_packet_para(camera, DC1394_VIDEO_MODE_FORMAT7_0, &min_bytes, &max_bytes) != DC1394_SUCCESS) { /* PACKET_PARA_INQ */
    printf("Packet para inq error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports allowed packet size from %d - %d bytes\n", min_bytes, max_bytes);

  
  if (dc1394_format7_get_byte_per_packet(camera, DC1394_VIDEO_MODE_FORMAT7_0, &actual_bytes) != DC1394_SUCCESS) {
    printf("dc1394_query_format7_byte_per_packet error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports actual packet size = %d bytes\n",
          actual_bytes);

  if (dc1394_format7_get_total_bytes(camera, DC1394_VIDEO_MODE_FORMAT7_0, &total_bytes) != DC1394_SUCCESS) {
    printf("dc1394_query_format7_total_bytes error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports total bytes per frame = %lld bytes\n", total_bytes);

  //fprintf(stderr,"handle: 0x%x\n",capture.handle);
  
  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera,DC1394_ON) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to start camera iso transmission\n");
    dc1394_capture_stop(camera);
    dc1394_free_camera(camera);
    exit(1);
  }
  //usleep(5000000);
  //fprintf(stderr,"handle: 0x%x\n",capture.handle);
  /*-----------------------------------------------------------------------
   *  capture 10 frames and measure the time for this operation
   *-----------------------------------------------------------------------*/
  start_time = times(&tms_buf);

  for( i = 0; i < grab_n_frames; ++i) {
    //fprintf(stderr,"capturing frame %d/%d\r",i,grab_n_frames);
    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    if (dc1394_capture(&camera,1)!=DC1394_SUCCESS) {
      //fprintf(stderr,"Error 1\n");
      //fprintf(stderr,"handle: 0x%x\n",capture.handle);
      dc1394_capture_stop(camera);
      //fprintf(stderr,"Error 2\n");
      //fprintf(stderr,"handle: 0x%x\n",capture.handle);
      dc1394_free_camera(camera);
      //fprintf(stderr,"Error 3\n");
      exit(1);
    }

    /*---------------------------------------------------------------------
     *  output elapsed time
     *---------------------------------------------------------------------*/
    elapsed_time = (float)(times(&tms_buf) - start_time) / CLOCKS_PER_SEC;
    printf( "got frame %d. elapsed time: %g sec ==> %g frames/second\n",
            i, elapsed_time, (float)i / elapsed_time);
  }
  
  /*-----------------------------------------------------------------------
   *  Stop data transmission
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera,DC1394_OFF)!=DC1394_SUCCESS) {
    printf("couldn't stop the camera?\n");
  }

  /*-----------------------------------------------------------------------
   *  save last image as Part.pgm
   *-----------------------------------------------------------------------*/
  imagefile=fopen("Part.pgm","w");
    
  dc1394_get_image_size_from_video_mode(camera, DC1394_VIDEO_MODE_FORMAT7_1,
          &width, &height);
  fprintf(imagefile,"P5\n%u %u\n255\n", width, height);
  fwrite((const char *)dc1394_capture_get_buffer (camera), 1,
         height * width, imagefile);
  fclose(imagefile);
  printf("wrote: Part.pgm\n");
  
  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_capture_stop(camera);
  dc1394_video_set_transmission(camera, DC1394_OFF);
  dc1394_free_camera(camera);
  return 0;
}
