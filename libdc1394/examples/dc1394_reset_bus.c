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
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <inttypes.h>


int main(int argc, char *argv[]) 
{
    dc1394_t * d;
    dc1394camera_list_t * list;
    dc1394camera_t *camera;
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
    
    printf ("Reseting bus...\n");
    dc1394_reset_bus (camera);
    
    dc1394_camera_free (camera);
    dc1394_free (d);
    
    return 0;
}
