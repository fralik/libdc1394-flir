/**************************************************************************
**       Title: Turn a Digital Camera into a V4L device using vloopback
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Sends format0 640x480 RGB to the vloopback input device so that it
**    can be consumed by V4L applications on the vloopback output device.
**    Get vloopback 0.90 from http://motion.technolust.cx/vloopback/
**    Apply the patch from effectv http://effectv.sf.net/
**    It has been tested with EffecTV (exciting!) and GnomeMeeting.
**
** TODO:
**    - Support different formats
**
**-------------------------------------------------------------------------
**
**  $Log$
**  Revision 1.3  2003/09/02 23:51:56  ddennedy
**  revert leaked local changes to dc1394_vloopback
**
**  Revision 1.1  2002/10/27 04:21:54  ddennedy
**  added v4l utility using vloopback
**
**
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <signal.h>
#include <linux/videodev.h>
#define _GNU_SOURCE
#include <getopt.h>

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#undef USE_v4L_MMAP

/* uncomment the following to drop frames to prevent delays */
#define DROP_FRAMES 1
#define MAX_PORTS 4
#define NUM_BUFFERS 8
#define WIDTH 640
#define HEIGHT 480

/* declarations for libdc1394 */
int numPorts = MAX_PORTS;
raw1394handle_t handle = NULL;
dc1394_cameracapture camera;
nodeid_t *camera_nodes;
dc1394_feature_set features;

/* Video4Linux globals */
int v4l_dev = -1;
char *image_out = NULL;
int v4l_fmt = VIDEO_PALETTE_RGB24;
#define MAXIOCTL 1024
char ioctlbuf[MAXIOCTL];

/* cmdline options */
char *dc_dev_name = NULL;
char *v4l_dev_name = "/dev/video0";
int width = WIDTH;
int height = HEIGHT;
int size = WIDTH*HEIGHT*3;

static struct option long_options[]={
	{"dc_dev",1,NULL,0},
	{"v4l_dev",1,NULL,0},
	{"help",0,NULL,0},
	{NULL,0,0,0}
	};

void get_options(int argc,char *argv[])
{
	int option_index=0;
	
	while(getopt_long(argc,argv,"",long_options,&option_index)>=0){
		if(optarg){
			switch(option_index){ 
				/* case values must match long_options */
				case 0:
					dc_dev_name=strdup(optarg);
					break;
				case 1:
					v4l_dev_name=strdup(optarg);
					break;
				}
			}
		if(option_index==3){
			printf( "\n"
			        "        %s - send video from dc1394 through vloopback\n\n"
				"Usage:\n"
				"        %s [--dc_dev=/dev/video1394/x] [--v4l_dev=/dev/video0]\n"
				"             --dc_dev  - specifies video1394 device to use (optional)\n"
				"                         default = /dev/video1394/<port#>\n"
				"             --v4l_dev - specifies video4linux device to use (optional)\n"
				"                         default = /dev/video0\n"
				"             --help    - prints this message\n\n"
				,argv[0],argv[0]);
			exit(0);
			}
		}
	
}


/* Video4Linux vloopback stuff */
int start_pipe (int dev, int width, int height, int format)
{
        struct video_capability vid_caps;
	struct video_window vid_win;
	struct video_picture vid_pic;

	if (ioctl (dev, VIDIOCGCAP, &vid_caps) == -1) {
		perror ("ioctl (VIDIOCGCAP)");
		return (1);
	}
	if (ioctl (dev, VIDIOCGPICT, &vid_pic)== -1) {
		perror ("ioctl VIDIOCGPICT");
		return (1);
	}
	vid_pic.palette = format;
	if (ioctl (dev, VIDIOCSPICT, &vid_pic)== -1) {
		perror ("ioctl VIDIOCSPICT");
		return (1);
	}
	if (ioctl (dev, VIDIOCGWIN, &vid_win)== -1) {
		perror ("ioctl VIDIOCGWIN");
		return (1);
	}
	vid_win.width=width;
	vid_win.height=height;
	if (ioctl (dev, VIDIOCSWIN, &vid_win)== -1) {
		perror ("ioctl VIDIOCSWIN");
		return (1);
	}
	return 0;
}


int put_image(int dev, char *image, int size)
{
	int i;
	char pixels[3*640*480];
	
	for (i = 0; i < size; i+=3)
	{
		pixels[i+2] = image[i+0];
		pixels[i+1] = image[i+1];
		pixels[i+0] = image[i+2];
	}
	if (write(dev, pixels, size) != size) {
		perror("Error writing image to pipe!");
		return 0;
	}
	return 1;
}

/* more direct way */

int get_frame(void)
{
	int i;
	const int size = 640*480*3;

	dc1394_dma_single_capture(&camera);
	for (i = 0; i < size; i+=3)
	{
		image_out[i+2] = camera.capture_buffer[i+0];
		image_out[i+1] = camera.capture_buffer[i+1];
		image_out[i+0] = camera.capture_buffer[i+2];
	}
	dc1394_dma_done_with_buffer(&camera);
	
	return 0;
}

char *v4l_create (int dev, int memsize)
{
	char *map;
	
	map=mmap(0, memsize, PROT_READ|PROT_WRITE, MAP_SHARED, dev, 0);
	if ((unsigned char *)-1 == (unsigned char *)map)
		return NULL;
	return map;	
}

int v4l_ioctl(unsigned long int cmd, void *arg)
{
	int i;
	switch (cmd) {
		case VIDIOCGCAP:
		{
			struct video_capability *vidcap=arg;

			sprintf(vidcap->name, "Jeroen's dummy v4l driver");
			vidcap->type= VID_TYPE_CAPTURE;
			vidcap->channels=1;
			vidcap->audios=0;
			vidcap->maxwidth=WIDTH;
			vidcap->maxheight=HEIGHT;
			vidcap->minwidth=20;
			vidcap->minheight=20;
			return 0;
		}
		case VIDIOCGCHAN:
		{
			struct video_channel *vidchan=arg;
			
			if (vidchan->channel!=0)
				return 1;
			vidchan->flags=0;
			vidchan->tuners=0;
			vidchan->type= VIDEO_TYPE_CAMERA;
			strcpy(vidchan->name, "Dummy test device");
			
			return 0;
		}
		case VIDIOCSCHAN:
		{
			int *v=arg;
			
			if (v[0]!=0)
				return 1;
			return 0;
		}
		case VIDIOCGPICT:
		{
			struct video_picture *vidpic=arg;

			vidpic->colour=0xffff;
			vidpic->hue=0xffff;
			vidpic->brightness=0xffff;
			vidpic->contrast=0xffff;
			vidpic->whiteness=0xffff;
			vidpic->depth=3;
			vidpic->palette=v4l_fmt;
			return 0;
		}
		case VIDIOCSPICT:
		{
			struct video_picture *vidpic=arg;
			
			if (vidpic->palette!=v4l_fmt)
				return 1;
			return 0;
		}
		case VIDIOCGWIN:
		{
			struct video_window *vidwin=arg;

			vidwin->x=0;
			vidwin->y=0;
			vidwin->width=WIDTH;
			vidwin->height=HEIGHT;
			vidwin->chromakey=0;
			vidwin->flags=0;
			vidwin->clipcount=0;
			return 0;
		}
		case VIDIOCSWIN:
		{
			struct video_window *vidwin=arg;
			
			if (vidwin->width > WIDTH ||
			    vidwin->height > HEIGHT )
				return 1;
			if (vidwin->flags)
				return 1;
			width=vidwin->width;
			height=vidwin->height;
			printf("new size: %dx%d\n", width, height);
			return 0;
		}
		case VIDIOCGMBUF:
		{
			struct video_mbuf *vidmbuf=arg;

			vidmbuf->size=width*height*3;
			vidmbuf->frames=1;
			for (i=0; i<vidmbuf->frames; i++)
				vidmbuf->offsets[i]=i*vidmbuf->size;
			return 0;
		}
		case VIDIOCMCAPTURE:
		{
			struct video_mmap *vidmmap=arg;

			return 0;
			if (vidmmap->height>HEIGHT ||
			    vidmmap->width>WIDTH ||
			    vidmmap->format!=v4l_fmt )
				return 1;
			if (vidmmap->height!=height ||
			    vidmmap->width!=width) {
				height=vidmmap->height;
				width=vidmmap->width;
				printf("new size: %dx%d\n", width, height);
			}
			// check if 'vidmmap->frame' is valid
			// initiate capture for 'vidmmap->frame' frames
			return 0;
		}
		case VIDIOCSYNC:
		{
			//struct video_mmap *vidmmap=arg;

			// check if frames are ready.
			// wait until ready.
			get_frame();
			return 0;
		}
		default:
		{
			printf("unknown ioctl: %ld\n", cmd & 0xff);
			return 1;
		}
	}
	return 0;
}

void v4l_sighandler(int signo)
{
	int size, ret;
	unsigned long int cmd;
	struct pollfd ufds;

	if (signo!=SIGIO)
		return;
	ufds.fd=v4l_dev;
	ufds.events=POLLIN;
	ufds.revents=0;
	poll(&ufds, 1, 1000);
	if (!ufds.revents & POLLIN) {
		printf("Received signal but got negative on poll?!?!?!?\n");
		return;
	}
	size=read(v4l_dev, ioctlbuf, MAXIOCTL);
	if (size >= sizeof(unsigned long int)) {
		memcpy(&cmd, ioctlbuf, sizeof(unsigned long int));
		if (cmd==0) {
			printf("Client closed device\n");
			return;
		}
		ret=v4l_ioctl(cmd, ioctlbuf+sizeof(unsigned long int));
		if (ret) {
			memset(ioctlbuf+sizeof(unsigned long int), MAXIOCTL-sizeof(unsigned long int), 0xff);
			printf("ioctl %ld unsuccesfull\n", cmd & 0xff);
		}
		ioctl(v4l_dev, cmd, ioctlbuf+sizeof(unsigned long int));
	}
	return;
}

/* MAIN */

void cleanup(void) {
	if (v4l_dev >= 0)
		close(v4l_dev);
	if (handle != NULL)
	{
		dc1394_dma_unlisten( handle, &camera );
		dc1394_dma_release_camera( handle, &camera);
		dc1394_destroy_handle(handle);
	}
	if (image_out != NULL)
		free(image_out);
}


/* trap ctrl-c */
void signal_handler( int sig) {
	signal( SIGINT, SIG_IGN);
	cleanup();
	exit(0);
}

int main(int argc,char *argv[])
{
	unsigned int channel;
	unsigned int speed;
	int p;
	raw1394handle_t raw_handle = NULL;
	struct raw1394_portinfo ports[MAX_PORTS];
	int camCount = 0;

	get_options(argc,argv);

	/* get the number of ports (cards) */
	raw_handle = raw1394_new_handle();
    if (raw_handle==NULL) {
        perror("Unable to aquire a raw1394 handle\n");
        perror("did you load the drivers?\n");
        exit(-1);
    }
	
	numPorts = raw1394_get_port_info(raw_handle, ports, numPorts);
	raw1394_destroy_handle(raw_handle);

	/* get dc1394 handle to each port */
	for (p = 0; p < numPorts; p++)
	{
		
		handle = dc1394_create_handle(p);
		if (handle==NULL) {
			perror("Unable to aquire a raw1394 handle\n");
			perror("did you load the drivers?\n");
			cleanup();
			exit(-1);
		}

		/* get the camera nodes and describe them as we find them */
		camera_nodes = dc1394_get_camera_nodes(handle, &camCount, 1);
		
		if (camCount > 0)
		{
			camera.node = camera_nodes[0];
		
			if (dc1394_get_iso_channel_and_speed(handle, camera.node,
										 &channel, &speed) !=DC1394_SUCCESS) 
			{
				printf("unable to get the iso channel number\n");
				cleanup();
				exit(-1);
			}
			 
			if (dc1394_dma_setup_capture(handle, camera.node,  channel,
									FORMAT_VGA_NONCOMPRESSED, MODE_640x480_RGB,
									SPEED_400, FRAMERATE_15, NUM_BUFFERS, DROP_FRAMES,
									dc_dev_name, &camera) != DC1394_SUCCESS) 
			{
				fprintf(stderr, "unable to setup camera- check line %d of %s to make sure\n",
					   __LINE__,__FILE__);
				perror("that the video mode,framerate and format are supported\n");
				printf("is one supported by your camera\n");
				cleanup();
				exit(-1);
			}
		
		
			/*have the camera start sending us data*/
			if (dc1394_start_iso_transmission(handle, camera.node) !=DC1394_SUCCESS) 
			{
				perror("unable to start camera iso transmission\n");
				cleanup();
				exit(-1);
			}
			break;
		}
	}

	fflush(stdout);
	if (camCount < 1) {
		perror("no cameras found :(\n");
		cleanup();
		exit(-1);
	}

	/* open vloopback */
	v4l_dev = open( v4l_dev_name, O_RDWR);
	if (v4l_dev < 0) {
		perror ("Failed to open Video4Linux device");
		cleanup();
		exit(-1);
	}
	
	
#ifdef USE_v4L_MMAP
	image_out = v4l_create(v4l_dev, WIDTH*HEIGHT*3);
	if (!image_out) {
		perror ("Failed to set device to zero-copy mode");
		cleanup();
		exit(-1);
	}

	signal (SIGIO, v4l_sighandler);

	printf("Running. Press Ctrl+C to quit.\n");
	while (1) {
		sleep(1000);
	}
#else
	start_pipe(v4l_dev, width, height, v4l_fmt);

	/* main event loop */	
	printf("Running. Press Ctrl+C to quit.\n");
	while(1){

		dc1394_dma_single_capture(&camera);
		put_image( v4l_dev, (char *) camera.capture_buffer, size);
		dc1394_dma_done_with_buffer(&camera);
		
	} /* while not interrupted */
#endif

	exit(0);
}
