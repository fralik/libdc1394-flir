/**************************************************************************
**       Title: testing of the new API for camera detection
**    $RCSfile$
**   $Revision: 414 $$Name$
**       $Date: 2007-08-14 14:25:38 +0900 (Tue, 14 Aug 2007) $
**   Copyright: LGPL $Author: ddouxchamps $
** Description:
**
**    This program will be removed from the SVN repo when the new API
**    for camera detection works properly.
**
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dc1394/control.h>

int main(int argc, char *argv[]) 
{
  dc1394_t *dc1394;
  dc1394camera_t *camera;
  dc1394error_t err;
  dc1394camera_list_t *list;
  int i;

  // initialise the library
  dc1394=dc1394_new();

  // enumerate camera GUIDs
  err=dc1394_enumerate_cameras(dc1394,&list);
  if (err!=DC1394_NO_CAMERA) {
    DC1394_ERR_RTN(err,"Could not enumerate cameras");
  }    
  fprintf(stderr,"Found %d camera(s)\n",list->num);
  
  // get each camera struct, print info and erase camera
  for (i=0;i<list->num;i++) {
    camera=dc1394_camera_new(dc1394,list->guids[i]);
    dc1394_print_camera_info(camera);
    dc1394_camera_free(camera);
  }
    
  // free camera list:
  dc1394_free_camera_list(list);

  // exit the library
  dc1394_free(dc1394);

  return 0;
}
