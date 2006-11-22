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
#include "dc1394/control.h"


/* uncomment the following to drop frames to prevent delays */
#define DROP_FRAMES DC1394_RING_BUFFER_LAST
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
uint32_t numCameras = 0;
dc1394camera_t **cameras;
dc1394featureset_t features;
dc1394video_frame_t * frames[MAX_CAMERAS];

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
				"                        default = automatic\n"
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
void iyu12yuy2 (unsigned char *src, unsigned char *dest, uint32_t NumPixels) {
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
void rgb2yuy2 (unsigned char *RGB, unsigned char *YUV, uint32_t NumPixels) {
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
	uint32_t i;
	
	if(!freeze && adaptor>=0){
		for (i = 0; i < numCameras; i++)
		{
            if (!frames[i])
                continue;
			switch (res) {
			case DC1394_VIDEO_MODE_640x480_YUV411:
				iyu12yuy2( frames[i]->image,
					(unsigned char *)(frame_buffer + (i * frame_length)),
					(device_width*device_height) );
				break;
				
			case DC1394_VIDEO_MODE_320x240_YUV422:
			case DC1394_VIDEO_MODE_640x480_YUV422:
				memcpy( frame_buffer + (i * frame_length),
					frames[i]->image, device_width*device_height*2);
				break;
					
			case DC1394_VIDEO_MODE_640x480_RGB8:
				rgb2yuy2( frames[i]->image,
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
	uint32_t num_adaptors;
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
	  dc1394_capture_stop(cameras[i]);
	  dc1394_video_set_transmission(cameras[i], DC1394_OFF);
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
  unsigned int speed;
  int i;
  
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
    res = DC1394_VIDEO_MODE_640x480_YUV411; 
    device_width=640;
    device_height=480;
    format=XV_YUY2;
    break;
  case 2: 
    res = DC1394_VIDEO_MODE_640x480_RGB8; 
    device_width=640;
    device_height=480;
    format=XV_YUY2;
    break;
  default: 
    res = DC1394_VIDEO_MODE_320x240_YUV422;
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
    
    if (dc1394_video_get_iso_speed(cameras[i], &speed) != DC1394_SUCCESS) {
      printf("unable to get the iso speed\n");
      cleanup();
      exit(-1);
    }
    
    if (device_name!=NULL) {
      if (dc1394_capture_set_device_filename(cameras[i],device_name) != DC1394_SUCCESS) {
	fprintf(stderr,"unable to set dma device filename!\n");
	return 0;
      }
    }

    dc1394_video_set_iso_speed(cameras[i], DC1394_ISO_SPEED_400);
    dc1394_video_set_mode(cameras[i],res);
    dc1394_video_set_framerate(cameras[i],fps);
    if (dc1394_capture_setup(cameras[i], NUM_BUFFERS) != DC1394_SUCCESS) {
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
    for (i = 0; i < numCameras; i++) {
      if (dc1394_capture_dequeue(cameras[i], DC1394_CAPTURE_POLICY_WAIT, &frames[i])!=DC1394_SUCCESS)
        fprintf (stderr, "Error: Failed to capture from camera %d\n", i);
    }
		
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
      if (frames[i])
        dc1394_capture_enqueue (cameras[i], frames[i]);
    }
    
  } /* while not interrupted */
  
  exit(0);
}
