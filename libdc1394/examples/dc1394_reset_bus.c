/**************************************************************************
**       Title: resets the bus using libdc1394
**    $RCSfile$
**   $Revision: 328 $$Name$
**       $Date: 2006-11-21 22:56:21 -0500 (Tue, 21 Nov 2006) $
**   Copyright: LGPL $Author: dcm $
** Description:
**
**************************************************************************/

#include <stdio.h>
#include <dc1394/control.h>
#include <stdlib.h>


/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
  dc1394_free_camera(camera);
  exit(1);
}

int main(int argc, char *argv[]) 
{
  dc1394camera_t *camera, **cameras=NULL;
  uint32_t numCameras, i;

  /* Find cameras */
  int err=dc1394_find_cameras(&cameras, &numCameras);

  if (err!=DC1394_SUCCESS && err != DC1394_NO_CAMERA) {
    fprintf( stderr, "Unable to look for cameras\n\n"
             "On Linux, please check \n"
	     "  - if the kernel modules `ieee1394',`raw1394' and `ohci1394' are loaded \n"
	     "  - if you have read/write access to /dev/raw1394\n\n");
    exit(1);
  }

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

  printf ("Reseting bus...\n");
  dc1394_reset_bus (camera);

  cleanup_and_exit(camera);

  return 0;
}
