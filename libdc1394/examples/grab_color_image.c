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
**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#define IMAGE_FILE_NAME "image.ppm"

/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(1);
}

int main(int argc, char *argv[])
{
    FILE* imagefile;
    dc1394camera_t *camera;
    unsigned int width, height;
    dc1394video_frame_t *frame=NULL;
    //dc1394featureset_t features;
    dc1394_t * d;
    dc1394camera_list_t * list;
    dc1394error_t err;

    d = dc1394_new ();
    err=dc1394_camera_enumerate (d, &list);
    DC1394_ERR_RTN(err,"Failed to enumerate cameras\n");

    if (list->num == 0) {
        dc1394_log_error("No cameras found\n");
        return 1;
    }

    camera = dc1394_camera_new (d, list->ids[0].guid);
    if (!camera) {
        dc1394_log_error("Failed to initialize camera with guid %"PRIx64"\n", list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (list);

    printf("Using camera with GUID %"PRIx64"\n", camera->guid);

    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/

    err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"oops!\n");

    err=dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_640x480_RGB8);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"oops!\n");

    err=dc1394_video_set_framerate(camera, DC1394_FRAMERATE_7_5);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"oops!\n");

    err=dc1394_capture_setup(camera,4, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"unable to setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera\n");

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"unable to start camera iso transmission\n");

    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"unable to capture a frame\n");

    /*-----------------------------------------------------------------------
     *  Stop data transmission
     *-----------------------------------------------------------------------*/
    err=dc1394_video_set_transmission(camera,DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(camera),"couldn't stop the camera?\n");

    /*-----------------------------------------------------------------------
     *  save image as 'Image.pgm'
     *-----------------------------------------------------------------------*/
    imagefile=fopen(IMAGE_FILE_NAME, "wb");

    if( imagefile == NULL) {
        perror( "Can't create output file");
        cleanup_and_exit(camera);
    }

    dc1394_get_image_size_from_video_mode(camera, DC1394_VIDEO_MODE_640x480_RGB8, &width, &height);
    fprintf(imagefile,"P6\n%u %u\n255\n", width, height);
    fwrite(frame->image, 1, height*width*3, imagefile);
    fclose(imagefile);
    printf("wrote: " IMAGE_FILE_NAME " (%d image bytes)\n",height*width*3);

    /*-----------------------------------------------------------------------
     *  Close camera
     *-----------------------------------------------------------------------*/
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    dc1394_free (d);
    return 0;
}
