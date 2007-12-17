#ifndef __DC1394_H__
#define __DC1394_H__


/*! \file dc1394.h
    \brief Main include file, which include all others.

    More details soon
*/

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>

/* Include all public header files:*/
#include <dc1394/types.h>
#include <dc1394/camera.h>
#include <dc1394/control.h>
#include <dc1394/capture.h>
#include <dc1394/conversions.h>
#include <dc1394/format7.h>
#include <dc1394/iso.h>
#include <dc1394/log.h>
#include <dc1394/register.h>
#include <dc1394/video.h>
#include <dc1394/utils.h>

#endif
