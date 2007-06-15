/*
 * 1394-Based Digital Camera Control Library
 * Basler Smart Feature Framework specific extensions
 * Copyright (C) 2006 Mikael Olenfalk, Tobii Technology AB, Stockholm Sweden
 *
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 * Version : 16/02/2005 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <dc1394/control.h>
#include <dc1394/vendor/basler.h>

int list_cameras (dc1394camera_t **cameras, uint32_t num_cameras)
{
  uint32_t i;
  dc1394bool_t sff_available;

  for (i = 0; i < num_cameras; i++) {
    sff_available = DC1394_FALSE;
    dc1394_basler_sff_is_available (cameras[i], &sff_available);

    printf ("%02d:0x%016llx:%s:%s:%s\n", i, cameras[i]->euid_64, cameras[i]->vendor, cameras[i]->model, sff_available ? "SFF" : "NO SFF");
  }
  return 0;
}

void cleanup (dc1394camera_t** cameras, uint32_t num_cameras)
{
  uint32_t i;
  for (i = 0; i < num_cameras; i++) {
    dc1394_free_camera(cameras[i]);
  }
  free (cameras);
}

int print_usage ()
{
  printf ("usage: basler_sff_info [--list-cameras|[--euid EUID]]\n");
  return 1;
}

int main (int argc, char **argv)
{
  dc1394error_t err;
  dc1394camera_t **cameras, *camera = NULL;
  uint32_t num_cameras, i;
  uint64_t euid = 0x0LL;

  err = dc1394_find_cameras (&cameras, &num_cameras);
  if (err != DC1394_SUCCESS) {
    fprintf (stderr, "E: cannot look for cameras\n");
    return 1;
  }

  if (num_cameras == 0) {
    fprintf (stderr, "E: no cameras found\n");
    cleanup (cameras, num_cameras);
    return 1;
  }

  /* parse arguments */
  if (argc == 2) {
    if (!strcmp (argv[1], "--list-cameras")) {
      list_cameras (cameras, num_cameras);
      cleanup (cameras, num_cameras);
      return 0;
    } else {
      cleanup (cameras, num_cameras);
      return print_usage();
    }  
  } else if (argc == 3) {
    if (!strcmp (argv[1], "--euid")) {
      if (sscanf (argv[2], "0x%llx", &euid) == 1) {
      } else {
        cleanup (cameras, num_cameras);
        return print_usage();
      }
    }
  } else if (argc != 1) {
    cleanup (cameras, num_cameras);
    return print_usage();
  }

  if (euid == 0x0LL) {
    printf ("I: found %d camera%s, working with camera 0\n", num_cameras, num_cameras == 1 ? "" : "s");
    camera = cameras[0];
    for (i = 1; i < num_cameras; i ++) {
      dc1394_free_camera(cameras[i]);
    }
    free (cameras);
  } else {
    for (i = 0; i < num_cameras; i ++) {
      if (cameras[i]->euid_64 == euid) {
        camera = cameras[i];
      } else {
        dc1394_free_camera(cameras[i]);
      }
    }
    free (cameras);

    if (camera == NULL) {
      fprintf (stderr, "E: no camera with euid 0x%016llx found\n", euid);
      return 1;
    }

    printf ("I: found camera with euid 0x%016llx\n", euid);
  }

  dc1394_print_camera_info (camera);
  printf ("\nSFF feature info:\n");
  dc1394_basler_sff_feature_print_all (camera);  
  return 0;
}
