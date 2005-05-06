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
  dc1394capture_t capture;
  dc1394camera_t *camera, **cameras=NULL;
  int numCameras;
  int grab_n_frames = 100;
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
  
  /*-----------------------------------------------------------------------
   *  to prevent the iso-transfer bug from raw1394 system, check if
   *  camera is highest node. For details see 
   *  http://linux1394.sourceforge.net/faq.html#DCbusmgmt
   *  and
   *  http://sourceforge.net/tracker/index.php?func=detail&aid=435107&group_id=8157&atid=108157
   *-----------------------------------------------------------------------*/
  /*
  if( camera_nodes[0] == numNodes-1) {
    fprintf( stderr, "\n"
             "Sorry, your camera is the highest numbered node\n"
             "of the bus, and has therefore become the root node.\n"
             "The root node is responsible for maintaining \n"
             "the timing of isochronous transactions on the IEEE \n"
             "1394 bus.  However, if the root node is not cycle master \n"
             "capable (it doesn't have to be), then isochronous \n"
             "transactions will not work.  The host controller card is \n"
             "cycle master capable, however, most cameras are not.\n"
             "\n"
             "The quick solution is to add the parameter \n"
             "attempt_root=1 when loading the OHCI driver as a \n"
             "module.  So please do (as root):\n"
             "\n"
             "   rmmod ohci1394\n"
             "   insmod ohci1394 attempt_root=1\n"
             "\n"
             "for more information see the FAQ at \n"
             "http://linux1394.sourceforge.net/faq.html#DCbusmgmt\n"
             "\n");
    dc1394_destroy_handle(handle);
    exit( 1);
  }
  */

  /*-----------------------------------------------------------------------
   *  setup capture for format 7
   *-----------------------------------------------------------------------*/

  if( dc1394_setup_format7_capture(camera, 0, /* channel */
                                   MODE_FORMAT7_0, 
                                   SPEED_400,
                                   USE_MAX_AVAIL, /* use max packet size */
                                   10, 20, /* left, top */
                                   200, 100,  /* width, height */
                                   &capture) != DC1394_SUCCESS)
  {
    fprintf( stderr,"unable to setup camera in format 7 mode 0 -\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    dc1394_free_camera(camera);
    exit(1);
  }

  /* set trigger mode */
  if( dc1394_set_trigger_mode(camera, TRIGGER_MODE_0)
      != DC1394_SUCCESS)
  {
    fprintf( stderr, "unable to set camera trigger mode\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }
  
  /*-----------------------------------------------------------------------
   *  print allowed and used packet size
   *-----------------------------------------------------------------------*/
  if (dc1394_query_format7_packet_para(camera, MODE_FORMAT7_0, &min_bytes, &max_bytes) != DC1394_SUCCESS) { /* PACKET_PARA_INQ */
    printf("Packet para inq error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports allowed packet size from %d - %d bytes\n", min_bytes, max_bytes);

  
  if (dc1394_query_format7_byte_per_packet(camera, MODE_FORMAT7_0, &actual_bytes) != DC1394_SUCCESS) {
    printf("dc1394_query_format7_byte_per_packet error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports actual packet size = %d bytes\n",
          actual_bytes);

  if (dc1394_query_format7_total_bytes(camera, MODE_FORMAT7_0, &total_bytes) != DC1394_SUCCESS) {
    printf("dc1394_query_format7_total_bytes error\n");
    return DC1394_FAILURE;
  }
  printf( "camera reports total bytes per frame = %lld bytes\n", total_bytes);
  
  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_start_iso_transmission(camera) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to start camera iso transmission\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  capture 1000 frames and measure the time for this operation
   *-----------------------------------------------------------------------*/
  start_time = times(&tms_buf);

  for( i = 0; i < grab_n_frames; ++i) {
    fprintf(stderr,"capturing frame %d/%d\r",i,grab_n_frames);
    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    if (dc1394_capture(&capture,1)!=DC1394_SUCCESS) {
      dc1394_release_capture(&capture);
      dc1394_free_camera(camera);
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
  if (dc1394_stop_iso_transmission(camera)!=DC1394_SUCCESS) {
    printf("couldn't stop the camera?\n");
  }

  /*-----------------------------------------------------------------------
   *  save last image as Part.pgm
   *-----------------------------------------------------------------------*/
  imagefile=fopen("Part.pgm","w");
    
  fprintf(imagefile,"P5\n%u %u 255\n", capture.frame_width,
          capture.frame_height );
  fwrite((const char *)capture.capture_buffer, 1,
         capture.frame_height*capture.frame_width, imagefile);
  fclose(imagefile);
  printf("wrote: Part.pgm\n");
  
  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_capture(&capture);
  dc1394_free_camera(camera);
  return 0;
}
