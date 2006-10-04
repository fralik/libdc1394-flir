/**************************************************************************
**       Title: grab another image using libdc1394
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**  An extension to the original grab image sample program.  This demonstrates
**  how to collect more detailed information on the various modes of the
**  camera, convert one image format to another, and waiting so that the
**  camera has time to capture the image before trying to write it out.
**
**************************************************************************/

#include <stdio.h>
#include "libdc1394/dc1394_utils.h"
#include "libdc1394/dc1394_control.h"
#include "libdc1394/dc1394_conversions.h"
#include <stdlib.h>


#define IMAGE_FILE_NAME "ImageRGB.ppm"

/*-----------------------------------------------------------------------
 *  Prints the type of format to standard out
 *-----------------------------------------------------------------------*/
void print_format( uint32_t format )
{
#define print_case(A) case A: printf(#A ""); break;

  switch( format ) {
    print_case(DC1394_VIDEO_MODE_160x120_YUV444);
    print_case(DC1394_VIDEO_MODE_320x240_YUV422);
    print_case(DC1394_VIDEO_MODE_640x480_YUV411);
    print_case(DC1394_VIDEO_MODE_640x480_YUV422);
    print_case(DC1394_VIDEO_MODE_640x480_RGB8);
    print_case(DC1394_VIDEO_MODE_640x480_MONO8);
    print_case(DC1394_VIDEO_MODE_640x480_MONO16);
    print_case(DC1394_VIDEO_MODE_800x600_YUV422);
    print_case(DC1394_VIDEO_MODE_800x600_RGB8);
    print_case(DC1394_VIDEO_MODE_800x600_MONO8);
    print_case(DC1394_VIDEO_MODE_1024x768_YUV422);
    print_case(DC1394_VIDEO_MODE_1024x768_RGB8);
    print_case(DC1394_VIDEO_MODE_1024x768_MONO8);
    print_case(DC1394_VIDEO_MODE_800x600_MONO16);
    print_case(DC1394_VIDEO_MODE_1024x768_MONO16);
    print_case(DC1394_VIDEO_MODE_1280x960_YUV422);
    print_case(DC1394_VIDEO_MODE_1280x960_RGB8);
    print_case(DC1394_VIDEO_MODE_1280x960_MONO8);
    print_case(DC1394_VIDEO_MODE_1600x1200_YUV422);
    print_case(DC1394_VIDEO_MODE_1600x1200_RGB8);
    print_case(DC1394_VIDEO_MODE_1600x1200_MONO8);
    print_case(DC1394_VIDEO_MODE_1280x960_MONO16);
    print_case(DC1394_VIDEO_MODE_1600x1200_MONO16);
   
  default:
    fprintf(stderr,"Unknown format\n");
    exit(1);   
  }

}

/*-----------------------------------------------------------------------
 *  Returns the number of pixels in the image based upon the format
 *-----------------------------------------------------------------------*/
uint32_t get_num_pixels(dc1394camera_t *camera, uint32_t format ) {
  uint32_t w,h;

  dc1394_get_image_size_from_video_mode(camera, format,&w,&h);

  return w*h;
}

/*-----------------------------------------------------------------------
 *  Prints the type of color encoding
 *-----------------------------------------------------------------------*/
void print_color_coding( uint32_t color_id )
{
  switch( color_id ) {
  case DC1394_COLOR_CODING_MONO8:
    printf("MONO8");
    break;
  case DC1394_COLOR_CODING_YUV411:
    printf("YUV411");
    break;
  case DC1394_COLOR_CODING_YUV422:
    printf("YUV422");
    break;
  case DC1394_COLOR_CODING_YUV444:
    printf("YUV444");
    break;
  case DC1394_COLOR_CODING_RGB8:
    printf("RGB8");
    break;
  case DC1394_COLOR_CODING_MONO16:
    printf("MONO16");
    break;
  case DC1394_COLOR_CODING_RGB16:
    printf("RGB16");
    break;
  case DC1394_COLOR_CODING_MONO16S:
    printf("MONO16S");
    break;
  case DC1394_COLOR_CODING_RGB16S:
    printf("RGB16S");
    break;
  case DC1394_COLOR_CODING_RAW8:
    printf("RAW8");
    break;
  case DC1394_COLOR_CODING_RAW16:
    printf("RAW16");
    break;

  default:
    fprintf(stderr,"Unknown color coding = %d\n",color_id);
    exit(1);
  }
}

/*-----------------------------------------------------------------------
 *  Prints various information about the mode the camera is in
 *-----------------------------------------------------------------------*/
void print_mode_info( dc1394camera_t *camera , uint32_t mode )
{
  int j;

  printf("Mode: ");
  print_format(mode);
  printf("\n");
  
  dc1394framerates_t framerates;
  if(dc1394_video_get_supported_framerates(camera,mode,&framerates) != DC1394_SUCCESS) {
    fprintf( stderr, "Can't get frame rates\n");
    exit(1);
  }
  
  printf("Frame Rates:\n");
  for( j = 0; j < framerates.num; j++ ) {
    uint32_t rate = framerates.framerates[j];
    float f_rate;
    dc1394_framerate_as_float(rate,&f_rate);
    printf("  [%d] rate = %f\n",j,f_rate );
  }

}

/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void clean_up_exit(dc1394camera_t *camera)
{
  dc1394_capture_stop(camera);
  dc1394_video_set_transmission(camera, DC1394_OFF);
  dc1394_free_camera(camera);
  exit(1);
}

int main(int argc, char *argv[]) 
{
  FILE* imagefile;
  dc1394camera_t *camera, **cameras=NULL;
  //int numNodes;
  uint32_t numCameras, i;
  dc1394featureset_t features;
  unsigned int width, height;
  
  /* Find cameras */
  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS && err != DC1394_NO_CAMERA) {
    fprintf( stderr, "Unable to look for cameras\n\n"
             "On Linux, please check \n"
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

  dc1394video_modes_t modes;

    // get supported modes
  if (dc1394_video_get_supported_modes(camera, &modes)!=DC1394_SUCCESS) {
    fprintf( stderr, "Couldnt get list of modes\n");
    exit(1);
  }

  /*-----------------------------------------------------------------------
   *  List Capture Modes
   *-----------------------------------------------------------------------*/
  uint32_t selected_mode=0;
  /*
  printf("Listing Modes\n");
  for(i = 0; i < modes.num; i++ ) {
    uint32_t mode = modes.modes[i];
    
    print_mode_info( camera , mode );

    selected_mode = mode;
  }
  */
  selected_mode = modes.modes[modes.num-1];
  dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
  dc1394_video_set_mode(camera,selected_mode);
  dc1394_video_set_framerate(camera,DC1394_FRAMERATE_7_5);

  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/
  
  if (dc1394_capture_setup(camera)!=DC1394_SUCCESS) {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    clean_up_exit(camera);
  }
  
  /*-----------------------------------------------------------------------
   *  report camera's features
   *-----------------------------------------------------------------------*/
  if( dc1394_external_trigger_set_mode(camera, DC1394_TRIGGER_MODE_0)
      != DC1394_SUCCESS){
    fprintf( stderr, "Warning: Unable to set camera trigger mode\n");
#if 0
    clean_up_exit(camera);
#endif
  }
  
  
  /*-----------------------------------------------------------------------
   *  report camera's features
   *-----------------------------------------------------------------------*/
  if (dc1394_get_camera_feature_set(camera,&features) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to get feature set\n");
  }
  else {
    //dc1394_print_feature_set(&features);
  }
    

  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera, DC1394_ON) !=DC1394_SUCCESS) {
    fprintf( stderr, "unable to start camera iso transmission\n");
    clean_up_exit(camera);
  }
  
  /*-----------------------------------------------------------------------
   *  Sleep untill the camera has a transmission
   *-----------------------------------------------------------------------*/
  dc1394switch_t status = DC1394_OFF;

  i = 0;
  while( status == DC1394_OFF && i++ < 5 ) {
    usleep(50000);
    if (dc1394_video_get_transmission(camera, &status)!=DC1394_SUCCESS) {
      fprintf(stderr, "unable to get transmision status\n");
      clean_up_exit(camera);
    }
  }

  if( i == 5 ) {
    fprintf(stderr,"Camera doesn't seem to want to turn on!\n");
    clean_up_exit(camera);
  }


  /*-----------------------------------------------------------------------
   *  capture one frame
   *-----------------------------------------------------------------------*/
  if (dc1394_capture(&camera,1)!=DC1394_SUCCESS) {
    fprintf(stderr, "unable to capture a frame\n");
    clean_up_exit(camera);
  }
  
  /*-----------------------------------------------------------------------
   *  Stop data transmission
   *-----------------------------------------------------------------------*/
  if (dc1394_video_set_transmission(camera,DC1394_OFF)!=DC1394_SUCCESS) {
    fprintf(stderr, "Couldn't stop the channel.\n");
    clean_up_exit(camera);
  }
  

  /*-----------------------------------------------------------------------
   *  Convert the image from what ever format it is to its RGB8
   *-----------------------------------------------------------------------*/

  dc1394_get_image_size_from_video_mode(camera, selected_mode,
          &width, &height);
  uint64_t numPixels = height*width;

  uint8_t *rgb_image = (uint8_t *)malloc(3*numPixels);

  dc1394_convert_to_RGB8((uint8_t *)dc1394_capture_get_buffer (camera),
          rgb_image, width, height, DC1394_BYTE_ORDER_YUYV, selected_mode, 16);

  /*-----------------------------------------------------------------------
   *  save image as 'Image.pgm'
   *-----------------------------------------------------------------------*/
  imagefile=fopen(IMAGE_FILE_NAME, "w");

  if( imagefile == NULL) {
    perror( "Can't create '" IMAGE_FILE_NAME "'");
    clean_up_exit(camera);
  }
  
    
  fprintf(imagefile,"P6\n%u %u\n255\n", width, height);
  fwrite((const char *)rgb_image, 1,numPixels*3, imagefile);
  fclose(imagefile);
  printf("wrote: " IMAGE_FILE_NAME "\n");

  /*-----------------------------------------------------------------------
   *  Close camera
   *-----------------------------------------------------------------------*/
  clean_up_exit(camera);

  return 0;
}
