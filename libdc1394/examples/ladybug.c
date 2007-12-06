// Capture program prototype for the spherical 6-CCD Ladybug
// camera from Point Grey
//
// by Damien Douxchamps.
// 
// Uses the first camera on the bus, then setup the camera
// in JPEG mode (mode 7), and capture NFRAMES frames to the HDD.
//
// NOTE: the image size has to be set so that the 6 sub-images,
//       encoded in JPEG, will fit in the total RAW image size.
//       If less than 6 frames are written you should use a larger
//       HEIGHT. Also, each color field is saved in an individual
//       frame. This results in 24 images (512x384) being written
//       for each (future) hemispherical image. 
//
// Easy adaptation include:
// - using 1394a instead of 1394b
// - using RAW instead of JPEG
// For more information have a look at the format specs in
// "Ladybug Stream File Specification" that is included in the
// Ladybug SDK.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <dc1394/control.h>
#include <dc1394/log.h>

// change this to switch from RAW to JPEG
#define VIDEO_MODE DC1394_VIDEO_MODE_FORMAT7_7 
// number of frames to be recorded. The total number of files written to disk will be 24xNFRAMES.
#define NFRAMES 10
// file basename  
#define BASENAME "~/test"

int
main(int argn, char **argv)
{

  dc1394error_t err;
  dc1394camera_t *camera;
  dc1394video_frame_t *frame;
  char filename[256];

  FILE *fd;

  dc1394_t * d;
  dc1394camera_list_t * list;

  d = dc1394_new ();
  if (dc1394_camera_enumerate (d, &list) != DC1394_SUCCESS) {
    fprintf (stderr, "Failed to enumerate cameras\n");
    return 1;
  }

  if (list->num == 0) {
    fprintf (stderr, "No cameras found\n");
    return 1;
  }
  
  camera = dc1394_camera_new (d, list->ids[0].guid);
  if (!camera) {
    fprintf (stderr, "Failed to initialize camera with guid %"PRIx64"\n",
        list->ids[0].guid);
    return 1;
  }
  dc1394_camera_free_list (list);
  fprintf(stderr,"Using camera \"%s %s\"\n",camera->vendor,camera->model);

  // setup video mode, etc...
  err=dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
  DC1394_ERR_RTN(err,"erreur!");
  err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
  DC1394_ERR_RTN(err,"erreur!");
  err=dc1394_video_set_mode(camera, VIDEO_MODE);
  DC1394_ERR_RTN(err,"erreur!");
  err=dc1394_format7_set_roi(camera, VIDEO_MODE, DC1394_COLOR_CODING_MONO8,
			     2000, 0,0, 512, 2015);
  DC1394_ERR_RTN(err,"erreur!");

  // setup capture
  err=dc1394_capture_setup(camera, 10, DC1394_CAPTURE_FLAGS_DEFAULT);
  DC1394_ERR_RTN(err,"erreur!");
  err=dc1394_video_set_transmission(camera, DC1394_ON);
  DC1394_ERR_RTN(err,"erreur!");

  int cam, k, i=0;
  unsigned int jpgadr, jpgsize, adr;

  while (i<NFRAMES) {
    // capture frame
    err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    DC1394_ERR_RTN(err,"erreur!");
    // do something with the image
    for (cam=0;cam<6;cam++) {
      for (k=0;k<4;k++) {
	adr=0x340+(5-cam)*32+(3-k)*8;
	jpgadr=(((unsigned int)*(frame->image+adr))<<24)+
	       (((unsigned int)*(frame->image+adr+1))<<16)+
	       (((unsigned int)*(frame->image+adr+2))<<8)+
	       (((unsigned int)*(frame->image+adr+3)));
	adr+=4;
	jpgsize=(((unsigned int)*(frame->image+adr))<<24)+
	        (((unsigned int)*(frame->image+adr+1))<<16)+
	        (((unsigned int)*(frame->image+adr+2))<<8)+
	        (((unsigned int)*(frame->image+adr+3)));

	if (jpgsize!=0) {
	  sprintf(filename,"%s-%05d-%d-%d.jpg",BASENAME,i,cam,k);
	  fd=fopen(filename,"w");
	  fwrite((unsigned char *)(jpgadr+frame->image),jpgsize,1,fd);
	  fclose(fd);
	}
      }
    }
    sprintf(filename,"%s-%05d.raw",BASENAME,i);
    fd=fopen(filename,"w");
    fwrite(frame->image,frame->total_bytes,1,fd);
    fclose(fd);
    // release frame
    err=dc1394_capture_enqueue(camera, frame);
    DC1394_ERR_RTN(err,"erreur!");
    fprintf(stderr,"%d\r",i);
    i++;
  }

  // stop capture
  err=dc1394_video_set_transmission(camera, DC1394_OFF);
  DC1394_ERR_RTN(err,"erreur!");
  err=dc1394_capture_stop(camera);
  DC1394_ERR_RTN(err,"erreur!");
  dc1394_camera_free (camera);
  dc1394_free (d);
  return 0;
}
