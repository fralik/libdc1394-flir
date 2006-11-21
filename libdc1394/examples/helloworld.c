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
#include <dc1394/control.h>

int main(int argc, char *argv[]) 
{
  dc1394camera_t **cameras=NULL;
  dc1394error_t err;
  dc1394video_frame_t * frame;
  uint32_t numCameras;

  /* Find the cameras. Warning: this allocates memory... */
  err=dc1394_find_cameras(&cameras, &numCameras);
  DC1394_ERR_RTN(err,"No camera on the bus");

  /* Setup capture with VIDEO1394 */
  err=dc1394_capture_setup(cameras[0], 4);
  DC1394_ERR_RTN(err,"Problem setting capture");
  
  /* Start transmission */
  err=dc1394_video_set_transmission(cameras[0], DC1394_ON);
  DC1394_ERR_RTN(err,"Problem starting transmission");

  /* Capture */
  err=dc1394_capture_dequeue(cameras[0], DC1394_CAPTURE_POLICY_WAIT, frame);
  if (err!=DC1394_SUCCESS) {
    fprintf (stderr, "Problem getting an image");
    return 1;
  }
  
  /* Release the buffer */
  err=dc1394_capture_enqueue(cameras[0], frame);
  DC1394_ERR_RTN(err,"Could not release buffer");

  /* Stop transmission */
  err=dc1394_video_set_transmission(cameras[0], DC1394_OFF);
  DC1394_ERR_RTN(err,"Problem stopping transmission");
  
  /* Stop capture */
  err=dc1394_capture_stop(cameras[0]);
  DC1394_ERR_RTN(err,"Could not stop capture");

  /* Hey, this is a HELLO WORLD program!! */
  printf("Hello World\n");

  /* Free memory */
  while (numCameras>0) {
    free(cameras[numCameras-1]);
    numCameras--;
  }
  free(cameras);

  return 0;
}
