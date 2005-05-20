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
**  Revision 1.4  2003/09/26 16:08:36  ddennedy
**  add libtoolize with no tests to autogen.sh, suppress examples/grab_color_image.c warning
**
**  Revision 1.3  2003/09/23 13:44:12  ddennedy
**  fix camera location by guid for all ports, add camera guid option to vloopback, add root detection and reset to vloopback
**
**  Revision 1.2  2003/09/15 17:21:28  ddennedy
**  add features to examples
**
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
#include <stdint.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>

#define MAX_PORTS 4
#define MAX_RESETS 10

char *g_filename = "image.ppm";
u_int64_t g_guid = 0;
dc1394camera_t *camera;
dc1394camera_t **cameras;
dc1394capture_t capture;

static struct option long_options[]={
	{"guid",1,NULL,0},
	{"help",0,NULL,0},
	{NULL,0,0,0}
	};

void get_options(int argc,char *argv[])
{
	int option_index=0;
	
	while (getopt_long(argc,argv,"",long_options,&option_index)>=0)
	{
		switch(option_index){ 
			/* case values must match long_options */
			case 0:
				sscanf(optarg, "%llx", &g_guid);
				break;
			default:
				printf( "\n"
					"%s - grab a color image using format0, rgb mode\n\n"
					"Usage:\n"
					"    %s [--guid=/dev/video1394/x] [filename.ppm]\n\n"
					"    --guid    - specifies camera to use (optional)\n"
					"                default = first identified on buses\n"
					"    --help    - prints this message\n"
					"    filename is optional; the default is image.ppm\n\n"
					,argv[0],argv[0]);
				exit(0);
		}
	}
	if (optind < argc)
		g_filename = argv[optind];
}

int dc_init()
{
  int reset;
  int camCount = 0;
  int found = 0;
  dc1394camera_t **cameras=NULL;
  int err, i;
  int cam=-1;

  for (reset=0;reset<MAX_RESETS;reset++) {
    err=dc1394_find_cameras(&cameras, &camCount);
    DC1394_ERR_CHK(err,"failed to find cameras");

    if (camCount > 0) {
      if (g_guid == 0) {
	/* use the first camera found */
	cam=0;
	found = 1;
      }
      else {
	/* attempt to locate camera by guid */
	for (i = 0; i< camCount && found == 0; i++) {
	  if (cameras[i]->euid_64 == g_guid) {
	    cam=i;
	    found = 1;
	  }
	}
      }
    }

    if (found == 1) {
      camera=cameras[cam];
      for (i=0;i<camCount;i++) {
	if (i!=cam)
	  dc1394_free_camera(cameras[i]);
      }
      free(cameras);
      cameras=NULL;

      capture.node = camera->node;
      dc1394_print_camera_info(camera);
    
      /* camera can not be root--highest order node */
      if (capture.node == raw1394_get_nodecount(camera->handle)-1) {
	/* reset and retry if root */
	//fprintf(stderr,"reset\n");
	raw1394_reset_bus(camera->handle);
	sleep(2);
	found = 0;
	dc1394_free_camera(camera);
      }
      else
	break;
    }
  } /* next reset retry */

  if (found) {
    /*have the camera start sending us data*/
    if (dc1394_start_iso_transmission(camera) !=DC1394_SUCCESS) {
      perror("unable to start camera iso transmission\n");
      exit(-1);
    }
  }
  if (found == 0 && g_guid != 0) {
    fprintf( stderr, "Unable to locate camera node by guid\n");
    exit(-1);
  }
  else if (camCount == 0) {
    fprintf( stderr, "no cameras found :(\n");
    exit(-1);
  }
  if (reset == MAX_RESETS) {
    fprintf( stderr, "failed to not make camera root node :(\n");
    exit(-1);
  }
  return found;
}

int main(int argc, char *argv[]) 
{
  FILE* imagefile;

  get_options(argc, argv);
  
  dc_init();

  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/
  if (dc1394_setup_capture(camera, 0, /* channel */
                           DC1394_MODE_640x480_RGB8,
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
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_capture(&capture,1)!=DC1394_SUCCESS) {
    fprintf( stderr, "unable to capture a frame\n");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit(1);
  }

 /*-----------------------------------------------------------------------
  *  save image as 'Image.pgm'
  *-----------------------------------------------------------------------*/
  imagefile=fopen(g_filename, "w");

  if( imagefile == NULL) {
    perror( "Can't create output file");
    dc1394_release_capture(&capture);
    dc1394_free_camera(camera);
    exit( 1);
  }
  
  fprintf(imagefile,"P6\n%u %u\n255\n", capture.frame_width,
          capture.frame_height );
  fwrite((const char *)capture.capture_buffer, 1,
         capture.frame_height*capture.frame_width*3, imagefile);
  fclose(imagefile);
  printf("wrote: %s\n", g_filename);

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  dc1394_release_capture(&capture);
  dc1394_free_camera(camera);
  return 0;
}
