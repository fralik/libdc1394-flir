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
#include <stdint.h>
#include <dc1394/control.h>
#include <stdlib.h>
#include <inttypes.h>


int main(int argc, char *argv[]) 
{
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
  
  dc1394camera_t * camera = dc1394_camera_new (d, list->ids[0].guid);
  if (!camera) {
    fprintf (stderr, "Failed to initialize camera with guid %"PRIx64"\n",
        list->ids[0].guid);
    return 1;
  }
  dc1394_camera_free_list (list);

  printf ("Using camera with guid %"PRIx64"\n", camera->guid);
  
  printf ("Reseting bus...\n");
  dc1394_reset_bus (camera);

  dc1394_camera_free (camera);
  dc1394_free (d);

  return 0;
}
