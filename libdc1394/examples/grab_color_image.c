/**************************************************************************
**       Title: grab one color image using libdc1394
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Get one  image using libdc1394 and store it as portable pix map
**    (ppm). Based on 'grab_gray_image' from Olaf Ronneberger
**
**-------------------------------------------------------------------------
**
**  $Log$
**  Revision 1.1  2003/09/02 23:42:36  ddennedy
**  cleanup handle destroying in examples; fix dc1394_multiview to use handle per camera; new example
**
**
**************************************************************************/

#include <stdio.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <stdlib.h>
#include <unistd.h>


#define IMAGE_FILE_NAME "image.ppm"
#define MAX_PORTS 4
#define MAX_RESETS 10

int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394_cameracapture camera;
  struct raw1394_portinfo ports[MAX_PORTS];
  int numPorts = 0;
  int numNodes;
  int numCameras = 0;
  int i, j;
  raw1394handle_t handle;
  nodeid_t * camera_nodes = NULL;

  /*-----------------------------------------------------------------------
   *  Open ohci and asign handle to it
   *-----------------------------------------------------------------------*/
  handle = raw1394_new_handle();
  if (handle==NULL)
  {
    fprintf( stderr, "Unable to aquire a raw1394 handle\n\n"
             "Please check \n"
	     "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - if you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }
	/* get the number of ports (cards) */
  numPorts = raw1394_get_port_info(handle, ports, numPorts);
  raw1394_destroy_handle(handle);
  handle = NULL;
  
	/* locate the first camera */
  for (j = 0; j < MAX_RESETS; j++)
  {
    for (i = 0; i < numPorts && numCameras == 0; i++)
    {
      if (handle != NULL)
        dc1394_destroy_handle(handle);
      handle = dc1394_create_handle(i);
      if (handle == NULL)
      {
        fprintf( stderr, "Unable to aquire a raw1394 handle for port %i\n", i);
        exit(1);
      }
      camera_nodes = dc1394_get_camera_nodes(handle, &numCameras, 0);
    }
    if (numCameras > 0)
    {
      numNodes = raw1394_get_nodecount(handle);
      /* camera can not be root--highest order node */
      if (camera_nodes[0] != numNodes-1)
          break;
      /* reset and retry if root */
      raw1394_reset_bus(handle);
      sleep(2);
      numCameras = 0;
    }
  }
  if (j == MAX_RESETS)
  {
    fprintf( stderr, "failed to not make camera root node :(\n");
    dc1394_destroy_handle(handle);
    exit(1);
  }
  else if (numCameras == 0)
  {
    fprintf( stderr, "no cameras found :(\n");
    dc1394_destroy_handle(handle);
    exit(1);
  }
  
  printf("working with the first camera on the bus\n");  
  
  /* set some useful auto features */
  dc1394_auto_on_off(handle, camera_nodes[0], FEATURE_BRIGHTNESS, 1);
  dc1394_auto_on_off(handle, camera_nodes[0], FEATURE_EXPOSURE, 1);
  dc1394_auto_on_off(handle, camera_nodes[0], FEATURE_WHITE_BALANCE, 1);
	
  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/
  if (dc1394_setup_capture(handle,camera_nodes[0],
                           0, /* channel */ 
                           FORMAT_VGA_NONCOMPRESSED,
                           MODE_640x480_RGB,
                           SPEED_400,
                           FRAMERATE_7_5,
                           &camera)!=DC1394_SUCCESS) 
  {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    dc1394_release_camera(handle,&camera);
    dc1394_destroy_handle(handle);
    exit(1);
  }
  
  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_start_iso_transmission(handle,camera.node)
      !=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to start camera iso transmission\n");
    dc1394_release_camera(handle,&camera);
    dc1394_destroy_handle(handle);
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_single_capture(handle,&camera)!=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to capture a frame\n");
    dc1394_release_camera(handle,&camera);
    dc1394_destroy_handle(handle);
    exit(1);
  }

 /*-----------------------------------------------------------------------
   *  save image as 'Image.pgm'
   *-----------------------------------------------------------------------*/
  imagefile=fopen(IMAGE_FILE_NAME, "w");

  if( imagefile == NULL)
  {
    perror( "Can't create '" IMAGE_FILE_NAME "'");
    dc1394_release_camera(handle,&camera);
    dc1394_destroy_handle(handle);
    exit( 1);
  }
  
    
  fprintf(imagefile,"P6\n%u %u\n255\n", camera.frame_width,
          camera.frame_height );
  fwrite((const char *)camera.capture_buffer, 1,
         camera.frame_height*camera.frame_width*3, imagefile);
  fclose(imagefile);
  printf("wrote: " IMAGE_FILE_NAME "\n");

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_camera(handle,&camera);
  dc1394_destroy_handle(handle);
  return 0;
}
