/**************************************************************************
**       Title: display video from multiple cameras/multiple cards
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    View format0-only camera video from one camera on one card,
**    muliple cameras on one card, or multiple cameras on multiple cards.
**
** TODO:
**    - Option to tile displays instead vertical stacking.
**
**-------------------------------------------------------------------------
**
**  $Log$
**  Revision 1.9.2.19  2005/09/09 08:14:35  ddouxchamps
**  dc1394capture_t struct now hidden in dc1394camera_t
**
**  Revision 1.9.2.18  2005/09/09 01:49:21  ddouxchamps
**  fixed compilation errors
**
**  Revision 1.9.2.17  2005/08/30 01:36:44  ddouxchamps
**  fixed a few bugs found with the iSight camera
**
**  Revision 1.9.2.16  2005/08/18 07:02:39  ddouxchamps
**  I looked at the bug reports on SF and applied some fixes
**
**  Revision 1.9.2.15  2005/08/05 01:24:15  ddouxchamps
**  fixed wrong color filter definitions and functions (Tony Hague)
**
**  Revision 1.9.2.14  2005/08/04 08:31:40  ddouxchamps
**  version 2.0.0-pre4
**
**  Revision 1.9.2.13  2005/08/02 05:43:04  ddouxchamps
**  Now compiles with GCC-4.0
**
**  Revision 1.9.2.12  2005/07/29 09:20:46  ddouxchamps
**  Interface harmonization (work in progress)
**
**  Revision 1.9.2.11  2005/06/22 05:02:38  ddouxchamps
**  Fixed detection issue with hub/repeaters
**
**  Revision 1.9.2.10  2005/05/20 08:58:58  ddouxchamps
**  all constant definitions now start with DC1394_
**
**  Revision 1.9.2.9  2005/05/09 02:57:51  ddouxchamps
**  first debugging with coriander
**
**  Revision 1.9.2.8  2005/05/09 00:48:22  ddouxchamps
**  more fixes and updates
**
**  Revision 1.9.2.7  2005/05/06 01:24:46  ddouxchamps
**  fixed a few bugs created by the previous changes
**
**  Revision 1.9.2.6  2005/05/06 00:13:37  ddouxchamps
**  more updates from Golden Week
**
**  Revision 1.9.2.5  2005/05/02 04:37:58  ddouxchamps
**  debugged everything. AFAIK code is 99.99% ok now.
**
**  Revision 1.9.2.4  2005/05/02 01:00:01  ddouxchamps
**  cleanup, error handling and new camera detection
**
**  Revision 1.9.2.3  2005/04/28 14:45:08  ddouxchamps
**  new error reporting mechanism
**
**  Revision 1.9.2.2  2005/04/06 05:52:33  ddouxchamps
**  fixed bandwidth usage estimation and missing strings
**
**  Revision 1.9.2.1  2005/02/13 07:02:47  ddouxchamps
**  Creation of the Version_2_0 branch
**
**  Revision 1.9  2004/08/10 07:57:22  ddouxchamps
**  Removed extra buffering (Johann Schoonees)
**
**  Revision 1.8  2004/03/09 08:41:44  ddouxchamps
**  patch from Johann Schoonees for extra buffering
**
**  Revision 1.7  2004/01/20 16:14:01  ddennedy
**  fix segfault in dc1394_multiview
**
**  Revision 1.6  2004/01/20 04:12:27  ddennedy
**  added dc1394_free_camera_nodes and applied to examples
**
**  Revision 1.5  2003/09/02 23:42:36  ddennedy
**  cleanup handle destroying in examples; fix dc1394_multiview to use handle per camera; new example
**
**  Revision 1.4  2002/07/27 21:24:51  ddennedy
**  just increase buffers some to reduce chance of hangs
**
**  Revision 1.3  2002/07/27 04:45:07  ddennedy
**  added drop_frames option to dma capture, prepare versions/NEWS for 0.9 release
**
**  Revision 1.2  2002/07/24 02:22:40  ddennedy
**  cleanup, add drop frame support to dc1394_multiview
**
**  Revision 1.1  2002/07/22 02:57:02  ddennedy
**  added examples/dc1394_multiview to test/demonstrate dma multicapture over multiple ports
**
**
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xvlib.h>
#include <X11/keysym.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>


/* uncomment the following to drop frames to prevent delays */
#define DROP_FRAMES 1
#define MAX_PORTS   4
#define MAX_CAMERAS 8
#define NUM_BUFFERS 8

/* ok the following constant should be by right included thru in Xvlib.h */
#ifndef XV_YV12
#define XV_YV12 0x32315659
#endif

#ifndef XV_YUY2
#define XV_YUY2 0x32595559
#endif

#ifndef XV_UYVY
#define XV_UYVY 0x59565955
#endif


/* declarations for libdc1394 */
uint_t numCameras = 0;
dc1394camera_t **cameras;
dc1394featureset_t features;

/* declarations for video1394 */
char *device_name=NULL;

/* declarations for Xwindows */
Display *display=NULL;
Window window=(Window)NULL;
unsigned long width,height;
unsigned long device_width,device_height;
int connection=-1;
XvImage *xv_image=NULL;
XvAdaptorInfo *info;
long format=0;
GC gc;


/* Other declarations */
unsigned long frame_length;
long frame_free;
int frame=0;
int adaptor=-1;

int freeze=0;
int average=0;
int fps;
int res;
char *frame_buffer=NULL;


static struct option long_options[]={
	{"device",1,NULL,0},
	{"fps",1,NULL,0},
	{"res",1,NULL,0},
	{"help",0,NULL,0},
	{NULL,0,0,0}
	};

void get_options(int argc,char *argv[])
{
	int option_index=0;
	fps=7;
	res=0;
	
	while(getopt_long(argc,argv,"",long_options,&option_index)>=0){
		if(optarg){
			switch(option_index){ 
				/* case values must match long_options */
				case 0:
					device_name=strdup(optarg);
					break;
				case 1:
					fps=atoi(optarg);
					break;
				case 2: 
					res=atoi(optarg);
					break;
				}
			}
		if(option_index==3){
			printf( "\n"
			        "        %s - multi-cam monitor for libdc1394 and XVideo\n\n"
				"Usage:\n"
				"        %s [--fps=[1,3,7,15,30]] [--res=[0,1,2]] [--device=/dev/video1394/x]\n"
				"             --fps    - frames per second. default=7,\n"
				"                        30 not compatible with --res=2\n"
				"             --res    - resolution. 0 = 320x240 (default),\n"
				"                        1 = 640x480 YUV4:1:1, 2 = 640x480 RGB8\n"
				"             --device - specifies video1394 device to use (optional)\n"
				"                        default = /dev/video1394/<port#>\n"
				"             --help   - prints this message\n\n"
				"Keyboard Commands:\n"
				"        q = quit\n"
				"        < -or- , = scale -50%%\n"
				"        > -or- . = scale +50%%\n"
				"        0 = pause\n"
				"        1 = set framerate to 1.875 fps\n"
				"        2 = set framerate tp 3.75 fps\n"
				"        3 = set framerate to 7.5 fps\n"
				"        4 = set framerate to 15 fps\n"
			    "        5 = set framerate to 30 fps\n"
				,argv[0],argv[0]);
			exit(0);
			}
		}
	
}

/* image format conversion functions */

static inline
void iyu12yuy2 (unsigned char *src, unsigned char *dest, uint_t NumPixels) {
  int i=0,j=0;
  register int y0, y1, y2, y3, u, v;
  while (i < NumPixels*3/2)
    {
      u = src[i++];
      y0 = src[i++];
      y1 = src[i++];
      v = src[i++];
      y2 = src[i++];
      y3 = src[i++];

      dest[j++] = y0;
      dest[j++] = u;
      dest[j++] = y1;
      dest[j++] = v;

      dest[j++] = y2;
      dest[j++] = u;
      dest[j++] = y3;
      dest[j++] = v;
    }
}


/* macro by Bart Nabbe */
#define RGB2YUV(r, g, b, y, u, v)\
  y = (9798*r + 19235*g + 3736*b)  / 32768;\
  u = (-4784*r - 9437*g + 14221*b)  / 32768 + 128;\
  v = (20218*r - 16941*g - 3277*b) / 32768 + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v

static inline
void rgb2yuy2 (unsigned char *RGB, unsigned char *YUV, uint_t NumPixels) {
  int i, j;
  register int y0, y1, u0, u1, v0, v1 ;
  register int r, g, b;

  for (i = 0, j = 0; i < 3 * NumPixels; i += 6, j += 4)
    {
      r = RGB[i + 0];
      g = RGB[i + 1];
      b = RGB[i + 2];
      RGB2YUV (r, g, b, y0, u0 , v0);
      r = RGB[i + 3];
      g = RGB[i + 4];
      b = RGB[i + 5];
      RGB2YUV (r, g, b, y1, u1 , v1);
      YUV[j + 0] = y0;
      YUV[j + 1] = (u0+u1)/2;
      YUV[j + 2] = y1;
      YUV[j + 3] = (v0+v1)/2;
    }
}

/* helper functions */

void set_frame_length(unsigned long size, int numCameras)
{
	frame_length=size;
	fprintf(stderr,"Setting frame size to %ld kb\n",size/1024);
	frame_free=0;
  	frame_buffer = malloc( size * numCameras);
}

void display_frames()
{
	uint_t i;
	
	if(!freeze && adaptor>=0){
		for (i = 0; i < numCameras; i++)
		{
			switch (res) {
			case DC1394_MODE_640x480_YUV411:
				iyu12yuy2( (unsigned char *) cameras[i]->capture.capture_buffer,
					(unsigned char *)(frame_buffer + (i * frame_length)),
					(device_width*device_height) );
				break;
				
			case DC1394_MODE_320x240_YUV422:
			case DC1394_MODE_640x480_YUV422:
				memcpy( frame_buffer + (i * frame_length),
					cameras[i]->capture.capture_buffer, device_width*device_height*2);
				break;
					
			case DC1394_MODE_640x480_RGB8:
				rgb2yuy2( (unsigned char *) cameras[i]->capture.capture_buffer,
					(unsigned char *) (frame_buffer + (i * frame_length)),
					(device_width*device_height) );
				break;
			}
		}
		
		xv_image=XvCreateImage(display,info[adaptor].base_id,format,frame_buffer,
			device_width,device_height * numCameras);
		XvPutImage(display,info[adaptor].base_id,window,gc,xv_image,
			0,0,device_width,device_height * numCameras,
			0,0,width,height);

		xv_image=NULL;
	}
}

void QueryXv()
{
	uint_t num_adaptors;
	int num_formats;
	XvImageFormatValues *formats=NULL;
	int i,j;
	char xv_name[5];
	
	XvQueryAdaptors(display,DefaultRootWindow(display),&num_adaptors,&info);
	
	for(i=0;i<num_adaptors;i++) {
		formats=XvListImageFormats(display,info[i].base_id,&num_formats);
		for(j=0;j<num_formats;j++) {
			xv_name[4]=0;
			memcpy(xv_name,&formats[j].id,4);
			if(formats[j].id==format) {
				fprintf(stderr,"using Xv format 0x%x %s %s\n",formats[j].id,xv_name,(formats[j].format==XvPacked)?"packed":"planar");
				if(adaptor<0)adaptor=i;
			}
		}
	}
		XFree(formats);
	if(adaptor<0)
		fprintf(stderr,"No suitable Xv adaptor found");	
	
}


void cleanup(void) {
	int i;
	for (i=0; i < numCameras; i++)
	{
		dc1394_dma_unlisten(cameras[i]);
		dc1394_dma_release_camera(cameras[i]);
	}
	if ((void *)window != NULL)
		XUnmapWindow(display,window);
	if (display != NULL)
		XFlush(display);
	if (frame_buffer != NULL)
		free( frame_buffer );
}


/* trap ctrl-c */
void signal_handler( int sig) {
	signal( SIGINT, SIG_IGN);
	cleanup();
	exit(0);
}

int main(int argc,char *argv[])
{
  XEvent xev;
  XGCValues xgcv;
  long background=0x010203;
  unsigned int channel;
  unsigned int speed;
  int i,err;
  
  get_options(argc,argv);
  /* process options */
  switch(fps) {
  case 1: fps =	DC1394_FRAMERATE_1_875; break;
  case 3: fps =	DC1394_FRAMERATE_3_75; break;
  case 15: fps = DC1394_FRAMERATE_15; break;
  case 30: fps = DC1394_FRAMERATE_30; break;
  case 60: fps = DC1394_FRAMERATE_60; break;
  default: fps = DC1394_FRAMERATE_7_5; break;
  }
  switch(res) {
  case 1: 
    res = DC1394_MODE_640x480_YUV411; 
    device_width=640;
    device_height=480;
    format=XV_YUY2;
    break;
  case 2: 
    res = DC1394_MODE_640x480_RGB8; 
    device_width=640;
    device_height=480;
    format=XV_YUY2;
    break;
  default: 
    res = DC1394_MODE_320x240_YUV422;
    device_width=320;
    device_height=240;
    format=XV_UYVY;
    break;
  }
  
  if (dc1394_find_cameras(&cameras,&numCameras)!=DC1394_SUCCESS) {
    fprintf(stderr,"Error: can't find cameras");
    exit(0);
  }

  if (numCameras>MAX_CAMERAS) {
    for (i=MAX_CAMERAS;i<numCameras;i++)
      dc1394_free_camera(cameras[i]);
    numCameras=MAX_CAMERAS;
  }

  //fprintf(stderr,"Cameras found: %d\n",numCameras);

  /* setup cameras for capture */
  for (i = 0; i < numCameras; i++) {	
    
    if(dc1394_get_camera_feature_set(cameras[i], &features) !=DC1394_SUCCESS) {
      printf("unable to get feature set\n");
    } else {
      //dc1394_print_feature_set(&features);
    }
    
    if (dc1394_video_get_iso_channel_and_speed(cameras[i], &channel, &speed) != DC1394_SUCCESS) {
      printf("unable to get the iso channel number\n");
      cleanup();
      exit(-1);
    }
    
    if (dc1394_dma_setup_capture(cameras[i], i+1 /*channel*/, res,
				 DC1394_SPEED_400, fps, NUM_BUFFERS, DROP_FRAMES,
				 device_name) != DC1394_SUCCESS) {
      fprintf(stderr, "unable to setup camera- check line %d of %s to make sure\n",
	      __LINE__,__FILE__);
      perror("that the video mode,framerate and format are supported\n");
      printf("is one supported by your camera\n");
      cleanup();
      exit(-1);
    }
    
		
    /*have the camera start sending us data*/
    if (dc1394_video_set_transmission(cameras[i],DC1394_ON) !=DC1394_SUCCESS) {
      perror("unable to start camera iso transmission\n");
      cleanup();
      exit(-1);
    }
  }
  
  fflush(stdout);
  if (numCameras < 1) {
    perror("no cameras found :(\n");
    cleanup();
    exit(-1);
  }

  //fprintf(stderr,"setup finished\n");

  switch(format){
  case XV_YV12:
    set_frame_length(device_width*device_height*3/2, numCameras);
    break;
  case XV_YUY2:
  case XV_UYVY:
    set_frame_length(device_width*device_height*2, numCameras);
    break;
  default:
    fprintf(stderr,"Unknown format set (internal error)\n");
    exit(255);
  }

  /* make the window */
  display=XOpenDisplay(getenv("DISPLAY"));
  if(display==NULL) {
    fprintf(stderr,"Could not open display \"%s\"\n",getenv("DISPLAY"));
    cleanup();
    exit(-1);
  }
  
  QueryXv();
  
  if ( adaptor < 0 ) {
    cleanup();
    exit(-1);
  }
  
  width=device_width;
  height=device_height * numCameras;
  
  window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,width,height,0,
			     WhitePixel(display,DefaultScreen(display)),
			     background);
  
  XSelectInput(display,window,StructureNotifyMask|KeyPressMask);
  XMapWindow(display,window);
  connection=ConnectionNumber(display);
	
  gc=XCreateGC(display,window,0,&xgcv);

  //fprintf(stderr,"event loop\n");
  
  /* main event loop */	
  while(1){

    //fprintf(stderr,"capturing...\n");
    err=dc1394_dma_capture(cameras, numCameras, DC1394_VIDEO1394_WAIT);
    DC1394_ERR_CHK(err,"failed to capture\n");
		
    display_frames();
    XFlush(display);
    //fprintf(stderr,"displayed\n");
    
    while(XPending(display)>0){
      XNextEvent(display,&xev);
      switch(xev.type){
      case ConfigureNotify:
	width=xev.xconfigure.width;
	height=xev.xconfigure.height;
	display_frames();
	break;
      case KeyPress:
	switch(XKeycodeToKeysym(display,xev.xkey.keycode,0)){
	case XK_q:
	case XK_Q:
	  cleanup();
	  exit(0);
	  break;
	case XK_comma:
	case XK_less:
	  width=width/2;
	  height=height/2;
	  XResizeWindow(display,window,width,height);
	  display_frames();
	  break;
	case XK_period:
	case XK_greater:
	  width=2*width;
	  height=2*height;
	  XResizeWindow(display,window,width,height);
	  display_frames();
	  break;
	case XK_0:
	  freeze = !freeze;
	  break;
	case XK_1:
	  fps =	DC1394_FRAMERATE_1_875; 
	  for (i = 0; i < numCameras; i++)
	    dc1394_video_set_framerate(cameras[i], fps);
	  break;
	case XK_2:
	  fps =	DC1394_FRAMERATE_3_75; 
	  for (i = 0; i < numCameras; i++)
	    dc1394_video_set_framerate(cameras[i], fps);
	  break;
	case XK_4:
	  fps = DC1394_FRAMERATE_15; 
	  for (i = 0; i < numCameras; i++)
	    dc1394_video_set_framerate(cameras[i], fps);
	  break;
	case XK_5: 
	  fps = DC1394_FRAMERATE_30;
	  for (i = 0; i < numCameras; i++)
	    dc1394_video_set_framerate(cameras[i], fps);
	  break;
	case XK_3:
	  fps = DC1394_FRAMERATE_7_5; 
	  for (i = 0; i < numCameras; i++)
	    dc1394_video_set_framerate(cameras[i], fps);
	  break;
	}
	break;
      }
    } /* XPending */
    
    for (i = 0; i < numCameras; i++) {
      dc1394_dma_done_with_buffer(cameras[i]);
    }
    
  } /* while not interrupted */
  
  exit(0);
}
