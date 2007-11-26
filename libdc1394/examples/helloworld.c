/**************************************************************************
**       Title: grab one image using libdc1394
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: LGPL $Author$
** Description:
**
**    Get one image using libdc1394 using the DMA interface (viedo1394).
**    Nothing is done with the image. There's a bunch of comments and
**    error handling included but the actual useful program consists in
**    _seven_ lines of code.
**
**    WARNING: This is a minimalist program that doesn't do everything
**             you should do, like cleaning up in case of failure. The
**             program uses the current setup of your camera and should
**             (but may not) work right after a camera boot.
**
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <dc1394/control.h>

int main(int argc, char *argv[]) 
{
  dc1394camera_t * camera;
  dc1394error_t err;
  dc1394video_frame_t * frame;
  dc1394_t * d;
  dc1394camera_list_t * list;

  d = dc1394_new ();
  if (dc1394_enumerate_cameras (d, &list) != DC1394_SUCCESS) {
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
  dc1394_free_camera_list (list);

  printf("Using camera with GUID %"PRIx64"\n", camera->guid);

  /* Setup capture with VIDEO1394 */
  err=dc1394_capture_setup(camera, 4, DC1394_CAPTURE_FLAGS_DEFAULT);
  DC1394_ERR_RTN(err,"Problem setting capture");
  
  /* Start transmission */
  err=dc1394_video_set_transmission(camera, DC1394_ON);
  DC1394_ERR_RTN(err,"Problem starting transmission");

  /* Capture */
  err=dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
  if (err!=DC1394_SUCCESS) {
    fprintf (stderr, "Problem getting an image");
    return 1;
  }
  
  /* Release the buffer */
  err=dc1394_capture_enqueue(camera, frame);
  DC1394_ERR_RTN(err,"Could not release buffer");

  /* Stop transmission */
  err=dc1394_video_set_transmission(camera, DC1394_OFF);
  DC1394_ERR_RTN(err,"Problem stopping transmission");
  
  /* Stop capture */
  err=dc1394_capture_stop(camera);
  DC1394_ERR_RTN(err,"Could not stop capture");

  /* Hey, this is a HELLO WORLD program!! */
  printf("Hello World\n");

  dc1394_camera_free (camera);
  dc1394_free (d);
  return 0;
}
