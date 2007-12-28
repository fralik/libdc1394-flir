/*
 * 1394-Based Digital Camera Control Library
 * 
 * MS Windows Support Code
 * 
 * Written by Vladimir Avdonin <vldmr@users.sf.net>
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

#ifndef compat_h
#define compat_h

#if !(defined(DC1394_DLL_SYMBOL))
#        if defined (DC1394_DLL_EXPORTS)
#                if defined (_MSC_VER)
__declspec(dllexport)
#                elif defined (__GNUC__)
__attribute__ ((dllexport))
#                endif
#        else
#        endif
#endif

#if defined (_MSC_VER)
/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef long long  int64_t;
typedef unsigned long long   uint64_t;
#endif

#include <Windows.h>

int usleep(long usec);

#endif

