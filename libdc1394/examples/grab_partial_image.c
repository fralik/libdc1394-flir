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
**  $Log$
**  Revision 1.4.2.23  2005/10/28 15:05:45  ddouxchamps
**  fixed bad help message in grab_color_image
**
**  Revision 1.4.2.22  2005/10/11 00:51:35  ddouxchamps
**  fixed the way we used the dma_device_file argument
**
**  Revision 1.4.2.21  2005/09/09 08:14:35  ddouxchamps
**  dc1394capture_t struct now hidden in dc1394camera_t
**
**  Revision 1.4.2.20  2005/09/09 01:49:21  ddouxchamps
**  fixed compilation errors
**
**  Revision 1.4.2.19  2005/08/30 01:36:44  ddouxchamps
**  fixed a few bugs found with the iSight camera
**
**  Revision 1.4.2.18  2005/08/18 07:02:39  ddouxchamps
**  I looked at the bug reports on SF and applied some fixes
**
**  Revision 1.4.2.17  2005/08/05 01:24:15  ddouxchamps
**  fixed wrong color filter definitions and functions (Tony Hague)
**
**  Revision 1.4.2.16  2005/08/04 08:31:40  ddouxchamps
**  version 2.0.0-pre4
**
**  Revision 1.4.2.15  2005/08/02 05:43:04  ddouxchamps
**  Now compiles with GCC-4.0
**
**  Revision 1.4.2.14  2005/07/29 09:20:46  ddouxchamps
**  Interface harmonization (work in progress)
**
**  Revision 1.4.2.13  2005/06/22 05:02:39  ddouxchamps
**  Fixed detection issue with hub/repeaters
**
**  Revision 1.4.2.12  2005/05/20 08:58:58  ddouxchamps
**  all constant definitions now start with DC1394_
**
**  Revision 1.4.2.11  2005/05/09 15:04:27  ddouxchamps
**  fixed important f7 bug
**
**  Revision 1.4.2.10  2005/05/09 02:57:51  ddouxchamps
**  first debugging with coriander
**
**  Revision 1.4.2.9  2005/05/09 00:48:23  ddouxchamps
**  more fixes and updates
**
**  Revision 1.4.2.8  2005/05/06 01:24:46  ddouxchamps
**  fixed a few bugs created by the previous changes
**
**  Revision 1.4.2.7  2005/05/06 00:13:38  ddouxchamps
**  more updates from Golden Week
**
**  Revision 1.4.2.6  2005/05/02 04:37:58  ddouxchamps
**  debugged everything. AFAIK code is 99.99% ok now.
**
**  Revision 1.4.2.5  2005/05/02 01:00:02  ddouxchamps
**  cleanup, error handling and new camera detection
**
**  Revision 1.4.2.4  2005/04/28 14:45:11  ddouxchamps
**  new error reporting mechanism
**
**  Revision 1.4.2.3  2005/04/20 08:54:02  ddouxchamps
**  another big update. everything seem to work, except RAW capture.
**
**  Revision 1.4.2.2  2005/04/06 05:52:33  ddouxchamps
**  fixed bandwidth usage estimation and missing strings
**
**  Revision 1.4.2.1  2005/02/13 07:02:47  ddouxchamps
**  Creation of the Version_2_0 branch
**
**  Revision 1.4  2003/09/02 23:42:36  ddennedy
**  cleanup handle destroying in examples; fix dc1394_multiview to use handle per camera; new example
**
**  Revision 1.3  2003/07/02 14:16:20  ddouxchamps
**  changed total_bytes to unsigned long long int in dc1394_query_format7_total_bytes.
**
**  Revision 1.2  2001/09/14 08:20:43  ronneber
**  - adapted to new dc1394_setup_format7_capture()
**  - using times() instead of time() for more precise frame rate measurement
**
**  Revision 1.1  2001/07/24 13:50:59  ronneber
**  - simple test programs to demonstrate the use of libdc1394 (based
**    on 'samplegrab' of Chris Urmson
**
**
**************************************************************************/

#include <stdio.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>


int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t *camera, **cameras=NULL;
  uint_t numCameras;
  int grab_n_frames = 10;
  struct tms tms_buf;
  clock_t start_time;
  float elapsed_time;
  int i;
  unsigned int min_bytes, max_bytes;
  unsigned int actual_bytes;
  unsigned long long int total_bytes = 0;

  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS) {
    fprintf( stderr, "Unable to look for cameras\n\n"
             "Please check \n"
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

  if( dc1394_setup_format7_capture(camera, 0, /* channel */
                                   DC1394_MODE_FORMAT7_0, 
                                   DC1394_SPEED_400,
                                   DC1394_USE_MAX_AVAIL, /* use max packet size */
                                   10, 20, /* left, top */
                                   200, 100  /* width, height */) != DC1394_SUCCESS)
  {
    fprintf( stderr,"unable to setup camera in format 7 mode 0 -\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    dc1394_free_camera(camera);
    exit(1);
  }
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
  if (dc1394_format7_get_packet_para(camera, DC1394_MODE_FORMAT7_0, &min_bytes, &max_bytes) != DC1394_SUCCESS) { /* PACKET_PARA_INQ */
    printf("Packet para inq error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports allowed packet size from %d - %d bytes\n", min_bytes, max_bytes);

  
  if (dc1394_format7_get_byte_per_packet(camera, DC1394_MODE_FORMAT7_0, &actual_bytes) != DC1394_SUCCESS) {
    printf("dc1394_query_format7_byte_per_packet error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports actual packet size = %d bytes\n",
          actual_bytes);

  if (dc1394_format7_get_total_bytes(camera, DC1394_MODE_FORMAT7_0, &total_bytes) != DC1394_SUCCESS) {
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
    dc1394_release_camera(camera);
    dc1394_free_camera(camera);
    exit(1);
  }
  //usleep(5000000);
  //fprintf(stderr,"handle: 0x%x\n",capture.handle);
  /*-----------------------------------------------------------------------
   *  capture 1000 frames and measure the time for this operation
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
      dc1394_release_camera(camera);
      //fprintf(stderr,"Error 2\n");
      //fprintf(stderr,"handle: 0x%x\n",capture.handle);
      dc1394_free_camera(camera);
      //fprintf(stderr,"Error 3\n");
      exit(1);
    }

    /*---------------------------------------------------------------------
     *  output elapsed time
     *---------------------------------------------------------------------*/
    elapsed_time = (float)(times(&tms_buf) - start_time) / CLK_TCK;
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
    
  fprintf(imagefile,"P5\n%u %u 255\n", camera->capture.frame_width,
          camera->capture.frame_height );
  fwrite((const char *)camera->capture.capture_buffer, 1,
         camera->capture.frame_height*camera->capture.frame_width, imagefile);
  fclose(imagefile);
  printf("wrote: Part.pgm\n");
  
  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_camera(camera);
  dc1394_free_camera(camera);
  return 0;
}
