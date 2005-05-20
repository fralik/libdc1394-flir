/*
 * 1394-Based Digital Camera Control Library
 * Color conversion functions, including Bayer pattern decoding
 * Copyright (C) Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * Written by Damien Douxchamps and Frederic Devernay
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
#ifndef __DC1394_CONVERSIONS_H__
#define __DC1394_CONVERSIONS_H__

#include "dc1394_control.h"

enum {
  DC1394_BYTE_ORDER_UYVY=0,
  DC1394_BYTE_ORDER_YUYV
};

enum {
  DC1394_BAYER_METHOD_NEAREST=0,
  DC1394_BAYER_METHOD_SIMPLE,
  DC1394_BAYER_METHOD_BILINEAR,
  DC1394_BAYER_METHOD_HQLINEAR,
  DC1394_BAYER_METHOD_DOWNSAMPLE,
  DC1394_BAYER_METHOD_EDGESENSE
};
#define DC1394_BAYER_METHOD_MIN      DC1394_BAYER_METHOD_NEAREST
#define DC1394_BAYER_METHOD_MAX      DC1394_BAYER_METHOD_EDGESENSE
#define DC1394_BAYER_METHOD_NUM     (DC1394_BAYER_METHOD_MAX-DC1394_BAYER_METHOD_MIN+1)

// color conversion functions from Bart Nabbe.
// corrected by Damien: bad coeficients in YUV2RGB
#define YUV2RGB(y, u, v, r, g, b)\
  r = y + ((v*1436) >> 10);\
  g = y - ((u*352 + v*731) >> 10);\
  b = y + ((u*1814) >> 10);\
  r = r < 0 ? 0 : r;\
  g = g < 0 ? 0 : g;\
  b = b < 0 ? 0 : b;\
  r = r > 255 ? 255 : r;\
  g = g > 255 ? 255 : g;\
  b = b > 255 ? 255 : b
  

#define RGB2YUV(r, g, b, y, u, v)\
  y = (306*r + 601*g + 117*b)  >> 10;\
  u = ((-172*r - 340*g + 512*b) >> 10)  + 128;\
  v = ((512*r - 429*g - 83*b) >> 10) + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v

/**********************************************************************
 *  CONVERSION FUNCTIONS TO YUV422
 **********************************************************************/

void dc1394_YUV422_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t byte_order);
void dc1394_YUV411_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t byte_order);
void dc1394_YUV444_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t byte_order);
void dc1394_MONO16_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t bits, uint_t byte_order);
void dc1394_RGB8_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t byte_order);
void dc1394_RGB16_to_YUV422(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t byte_order);
void dc1394_MONO8_to_YUV422(uchar_t *src, uchar_t *dest, 
			    uint_t src_width, uint_t src_height,
			    uint_t dest_pitch, uint_t byte_order);

/**********************************************************************
 *  CONVERSION FUNCTIONS TO MONO8
 **********************************************************************/

void dc1394_MONO16_to_MONO8(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t bits);

/**********************************************************************
 *  CONVERSION FUNCTIONS TO RGB8 
 **********************************************************************/

void dc1394_RGB16_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels);
void dc1394_YUV444_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels);
void dc1394_YUV422_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels);
void dc1394_YUV411_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels);
void dc1394_MONO8_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels);
void dc1394_MONO16_to_RGB8(uchar_t *src, uchar_t *dest, uint64_t NumPixels, uint_t bits);

/**********************************************************************
 *  CONVERSION FUNCTIONS FOR STEREO IMAGES
 **********************************************************************/
//changes a 16bit stereo image (8bit/channel) into two 8bit images on top of each other
void dc1394_deinterlace_stereo(uchar_t *src, uchar_t *dest, uint64_t NumPixels);

/************************************************************************************************
 *                                                                                              *
 *      Color conversion functions for cameras that can output raw Bayer pattern images, such   *
 *  as some Basler, AVT and Point Grey cameras.                                                 *
 *                                                                                              *
 *  Credits and sources:                                                                        *
 *  - Nearest Neighbor : OpenCV library                                                         *
 *  - Bilinear         : OpenCV library                                                         *
 *  - HQLinear         : High-Quality Linear Interpolation For Demosaicing Of Bayer-Patterned   *
 *                       Color Images, by Henrique S. Malvar, Li-wei He, and Ross Cutler,       *
 * 			 in Proceedings of the ICASSP'04 Conference.                            *
 *  - Edge Sense II    : Laroche, Claude A. "Apparatus and method for adaptively interpolating  *
 *                       a full color image utilizing chrominance gradients"                    *
 * 			 U.S. Patent 5,373,322. Based on the code found on the website          *
 *                       http://www-ise.stanford.edu/~tingchen/ Converted to C and adapted to   *
 *                       all four elementary patterns.                                          *
 *  - Downsample       : "Known to the Ancients"                                                *
 *  - Simple           : Implemented from the information found in the manual of Allied Vision  *
 *                       Technologies (AVT) cameras.                                            *
 *                                                                                              *
 ************************************************************************************************/

int dc1394_bayer_decoding_8bit(const uchar_t *bayer, uchar_t *rgb, uint_t sx, uint_t sy, uint_t tile, uint_t method);
int dc1394_bayer_decoding_16bit(const uint16_t *bayer, uint16_t *rgb, uint_t sx, uint_t sy, uint_t tile, uint_t method, uint_t bits);

#endif /* _DC1394_CONVERSIONS_H */


