/**************************************************************************
**       Title: grab one gray image using libdc1394
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Get one gray image using libdc1394 and store it as portable gray map
**    (pgm). Based on 'samplegrab' from Chris Urmson 
**
**-------------------------------------------------------------------------
**
**  $Log$
**  Revision 1.5.2.18  2005/09/09 01:49:21  ddouxchamps
**  fixed compilation errors
**
**  Revision 1.5.2.17  2005/08/30 01:36:44  ddouxchamps
**  fixed a few bugs found with the iSight camera
**
**  Revision 1.5.2.16  2005/08/18 07:02:39  ddouxchamps
**  I looked at the bug reports on SF and applied some fixes
**
**  Revision 1.5.2.15  2005/08/05 01:24:15  ddouxchamps
**  fixed wrong color filter definitions and functions (Tony Hague)
**
**  Revision 1.5.2.14  2005/08/04 08:31:40  ddouxchamps
**  version 2.0.0-pre4
**
**  Revision 1.5.2.13  2005/08/02 05:43:04  ddouxchamps
**  Now compiles with GCC-4.0
**
**  Revision 1.5.2.12  2005/07/29 09:20:46  ddouxchamps
**  Interface harmonization (work in progress)
**
**  Revision 1.5.2.11  2005/06/22 05:02:39  ddouxchamps
**  Fixed detection issue with hub/repeaters
**
**  Revision 1.5.2.10  2005/05/20 08:58:58  ddouxchamps
**  all constant definitions now start with DC1394_
**
**  Revision 1.5.2.9  2005/05/09 02:57:51  ddouxchamps
**  first debugging with coriander
**
**  Revision 1.5.2.8  2005/05/09 00:48:23  ddouxchamps
**  more fixes and updates
**
**  Revision 1.5.2.7  2005/05/06 01:24:46  ddouxchamps
**  fixed a few bugs created by the previous changes
**
**  Revision 1.5.2.6  2005/05/06 00:13:38  ddouxchamps
**  more updates from Golden Week
**
**  Revision 1.5.2.5  2005/05/02 04:37:58  ddouxchamps
**  debugged everything. AFAIK code is 99.99% ok now.
**
**  Revision 1.5.2.4  2005/05/02 01:00:02  ddouxchamps
**  cleanup, error handling and new camera detection
**
**  Revision 1.5.2.3  2005/04/28 14:45:11  ddouxchamps
**  new error reporting mechanism
**
**  Revision 1.5.2.2  2005/04/06 05:52:33  ddouxchamps
**  fixed bandwidth usage estimation and missing strings
**
**  Revision 1.5.2.1  2005/02/13 07:02:47  ddouxchamps
**  Creation of the Version_2_0 branch
**
**  Revision 1.5  2004/01/20 04:12:27  ddennedy
**  added dc1394_free_camera_nodes and applied to examples
**
**  Revision 1.4  2003/09/02 23:42:36  ddennedy
**  cleanup handle destroying in examples; fix dc1394_multiview to use handle per camera; new example
**
**  Revision 1.3  2001/10/16 09:14:14  ronneber
**  - added more meaningful error message, when no raw1394 handle could be get
**  - does not exit anymore, when camera has no trigger
**
**  Revision 1.2  2001/09/14 08:10:41  ronneber
**  - some cosmetic changes
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


#define IMAGE_FILE_NAME "Image.pgm"

int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394capture_t capture;
  dc1394camera_t *camera, **cameras=NULL;
  //int numNodes;
  uint_t numCameras, i;
  dc1394featureset_t features;
  
  /* Find cameras */
  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS) {
    fprintf( stderr, "Unable to look for cameras\n\n"
             "Please check \n"
	     "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - if you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }

  //fprintf(stderr,"got cam info\n");

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
  if( camera->node == numNodes-1) {
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
    dc1394_free_camera(camera);
    exit( 1);
  }
  */

  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/
  if (dc1394_setup_capture(camera, 0, /* channel */ 
                           DC1394_MODE_640x480_MONO8,
                           DC1394_SPEED_400,
                           DC1394_FRAMERATE_7_5,
                           &capture)!=DC1394_SUCCESS) {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }
  
  /* set trigger mode */
  if( dc1394_external_trigger_set_mode(camera, DC1394_TRIGGER_MODE_0)
      != DC1394_SUCCESS)
  {
    fprintf( stderr, "unable to set camera trigger mode\n");
#if 0
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
#endif
  }
  
  
  /*-----------------------------------------------------------------------
   *  report camera's features
   *-----------------------------------------------------------------------*/
  if (dc1394_get_camera_feature_set(camera,&features) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to get feature set\n");
  }
  else {
    dc1394_print_feature_set(&features);
  }
    

  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera, DC1394_ON) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to start camera iso transmission\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_capture(&capture,1)!=DC1394_SUCCESS) {
    fprintf( stderr, "unable to capture a frame\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }
  
  /*-----------------------------------------------------------------------
   *  Stop data transmission
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera,DC1394_OFF)!=DC1394_SUCCESS) {
    printf("couldn't stop the camera?\n");
  }
  
  /*-----------------------------------------------------------------------
   *  save image as 'Image.pgm'
   *-----------------------------------------------------------------------*/
  imagefile=fopen(IMAGE_FILE_NAME, "w");

  if( imagefile == NULL) {
    perror( "Can't create '" IMAGE_FILE_NAME "'");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit( 1);
  }
  
    
  fprintf(imagefile,"P5\n%u %u 255\n", capture.frame_width,
          capture.frame_height );
  fwrite((const char *)capture.capture_buffer, 1,
         capture.frame_height*capture.frame_width, imagefile);
  fclose(imagefile);
  printf("wrote: " IMAGE_FILE_NAME "\n");

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_capture(&capture);
  dc1394_free_camera(camera);

  return 0;
}
