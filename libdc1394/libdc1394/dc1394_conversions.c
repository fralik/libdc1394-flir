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

#include "dc1394_conversions.h"

// this should disappear...
extern void swab();

/**********************************************************************
 *
 *  CONVERSION FUNCTIONS TO UYVY 
 *
 **********************************************************************/

void
dc1394_YUV422_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    swab(src, dest, (width*height) << 1);
    break;
  case DC1394_BYTE_ORDER_UYVY:
    memcpy(dest,src, (width*height)<<1);
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }
}

void
dc1394_YUV411_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  register int i=(width*height) + ((width*height) >> 1) -1;
  register int j=((width*height) << 1)-1;
  register int y0, y1, y2, y3, u, v;

  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      y3 = src[i--];
      y2 = src[i--];
      v  = src[i--];
      y1 = src[i--];
      y0 = src[i--];
      u  = src[i--];

      dest[j--] = v;
      dest[j--] = y3;
      dest[j--] = u;
      dest[j--] = y2;
      
      dest[j--] = v;
      dest[j--] = y1;
      dest[j--] = u;
      dest[j--] = y0;
    }
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      y3 = src[i--];
      y2 = src[i--];
      v  = src[i--];
      y1 = src[i--];
      y0 = src[i--];
      u  = src[i--];

      dest[j--] = y3;
      dest[j--] = v;
      dest[j--] = y2;
      dest[j--] = u;
      
      dest[j--] = y1;
      dest[j--] = v;
      dest[j--] = y0;
      dest[j--] = u;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }

}

void
dc1394_YUV444_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  register int i = (width*height) + ((width*height) << 1)-1;
  register int j = ((width*height) << 1)-1;
  register int y0, y1, u0, u1, v0, v1;

  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      v1 = src[i--];
      y1 = src[i--];
      u1 = src[i--];
      v0 = src[i--];
      y0 = src[i--];
      u0 = src[i--];

      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y1;
      dest[j--] = (u0+u1) >> 1;
      dest[j--] = y0;
    }
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      v1 = src[i--];
      y1 = src[i--];
      u1 = src[i--];
      v0 = src[i--];
      y0 = src[i--];
      u0 = src[i--];

      dest[j--] = y1;
      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y0;
      dest[j--] = (u0+u1) >> 1;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }
}

void
dc1394_MONO8_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  if ((width%2)==0) {
    // do it the quick way
    register int i = width*height - 1;
    register int j = (width*height << 1) - 1;
    register int y0, y1;
    
    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
      while (i >= 0) {
	y1 = src[i--];
	y0 = src[i--];
	dest[j--] = 128;
	dest[j--] = y1;
	dest[j--] = 128;
	dest[j--] = y0;
      }
      break;
    case DC1394_BYTE_ORDER_UYVY:
      while (i >= 0) {
	y1 = src[i--];
	y0 = src[i--];
	dest[j--] = y1;
	dest[j--] = 128;
	dest[j--] = y0;
	dest[j--] = 128;
      }
      break;
    default:
      fprintf(stderr,"Invalid overlay byte order\n");
      break;
    }
  } else { // width*2 != dest_pitch
    register int x, y;

    //assert ((dest_pitch - 2*width)==1);

    switch (byte_order) {
    case DC1394_BYTE_ORDER_YUYV:
      y=height;
      while (y--) {
	x=width;
	while (x--) {
	  *dest++ = *src++;
	  *dest++ = 128;
	}
	// padding required, duplicate last column
	*dest++ = *(src-1);
	*dest++ = 128;
      }
      break;
    case DC1394_BYTE_ORDER_UYVY:
      y=height;
      while (y--) {
	x=width;
	while (x--) {
	  *dest++ = 128;
	  *dest++ = *src++;
	}
	// padding required, duplicate last column
	*dest++ = 128;
	*dest++ = *(src-1);
      }
      break;
    default:
      fprintf(stderr,"Invalid overlay byte order\n");
      break;
    }
  }
}

void
dc1394_MONO16_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order, uint32_t bits)
{
  register int i = ((width*height) << 1)-1;
  register int j = ((width*height) << 1)-1;
  register int y0, y1;

  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      y1 = src[i--];
      y1 = (y1 + (((int)src[i--])<<8))>>(bits-8);
      y0 = src[i--];
      y0 = (y0 + (((int)src[i--])<<8))>>(bits-8);
      dest[j--] = 128;
      dest[j--] = y1;
      dest[j--] = 128;
      dest[j--] = y0;
    }
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      y1 = src[i--];
      y1 = (y1 + (((int)src[i--])<<8))>>(bits-8);
      y0 = src[i--];
      y0 = (y0 + (((int)src[i--])<<8))>>(bits-8);
      dest[j--] = y1;
      dest[j--] = 128;
      dest[j--] = y0;
      dest[j--] = 128;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }

}

void
dc1394_MONO16_to_MONO8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
  register int i = ((width*height)<<1)-1;
  register int j = (width*height)-1;
  register int y;

  while (i >= 0) {
    y = src[i--];
    dest[j--] = (y + (src[i--]<<8))>>(bits-8);
  }
}

void
dc1394_RGB8_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  register int i = (width*height) + ( (width*height) << 1 )-1;
  register int j = ((width*height) << 1)-1;
  register int y0, y1, u0, u1, v0, v1 ;
  register int r, g, b;

  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      b = (uint8_t) src[i--];
      g = (uint8_t) src[i--];
      r = (uint8_t) src[i--];
      RGB2YUV (r, g, b, y0, u0 , v0);
      b = (uint8_t) src[i--];
      g = (uint8_t) src[i--];
      r = (uint8_t) src[i--];
      RGB2YUV (r, g, b, y1, u1 , v1);
      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y0;
      dest[j--] = (u0+u1) >> 1;
      dest[j--] = y1;
    }
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      b = (uint8_t) src[i--];
      g = (uint8_t) src[i--];
      r = (uint8_t) src[i--];
      RGB2YUV (r, g, b, y0, u0 , v0);
      b = (uint8_t) src[i--];
      g = (uint8_t) src[i--];
      r = (uint8_t) src[i--];
      RGB2YUV (r, g, b, y1, u1 , v1);
      dest[j--] = y0;
      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y1;
      dest[j--] = (u0+u1) >> 1;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }
}

void
dc1394_RGB16_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order, uint32_t bits)
{
  register int i = ( ((width*height) + ( (width*height) << 1 )) << 1 ) -1;
  register int j = ((width*height) << 1)-1;
  register int y0, y1, u0, u1, v0, v1 ;
  register int r, g, b, t;

  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      t =src[i--];
      b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--];
      g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--];
      r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      RGB2YUV (r, g, b, y0, u0 , v0);
      t =src[i--]; 
      b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--];
      g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8)); 
      t =src[i--];
      r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      RGB2YUV (r, g, b, y1, u1 , v1);
      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y0;
      dest[j--] = (u0+u1) >> 1;
      dest[j--] = y1;
    } 
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      t =src[i--]; 
      b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--]; 
      g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--]; 
      r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      RGB2YUV (r, g, b, y0, u0 , v0);
      t =src[i--]; 
      b = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--]; 
      g = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      t =src[i--]; 
      r = (uint8_t) ((t + (src[i--]<<8)) >>(bits-8));
      RGB2YUV (r, g, b, y1, u1 , v1);
      dest[j--] = y0;
      dest[j--] = (v0+v1) >> 1;
      dest[j--] = y1;
      dest[j--] = (u0+u1) >> 1;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }
}

/**********************************************************************
 *
 *  CONVERSION FUNCTIONS TO RGB 24bpp 
 *
 **********************************************************************/

void
dc1394_RGB16_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
  register int i = (((width*height) + ( (width*height) << 1 )) << 1)-1;
  register int j = (width*height) + ( (width*height) << 1 ) -1;
  register int t;

  while (i >= 0) {
    t = src[i--];
    t = (t + (src[i--]<<8))>>(bits-8);
    dest[j--]=t;
    t = src[i--];
    t = (t + (src[i--]<<8))>>(bits-8);
    dest[j--]=t;
    t = src[i--];
    t = (t + (src[i--]<<8))>>(bits-8);
    dest[j--]=t;
  }
}


void
dc1394_YUV444_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
  register int i = (width*height) + ( (width*height) << 1 ) -1;
  register int j = (width*height) + ( (width*height) << 1 ) -1;
  register int y, u, v;
  register int r, g, b;

  while (i >= 0) {
    v = (uint8_t) src[i--] - 128;
    y = (uint8_t) src[i--];
    u = (uint8_t) src[i--] - 128;
    YUV2RGB (y, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;  
  }
}

void
dc1394_YUV422_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order)
{
  register int i = ((width*height) << 1)-1;
  register int j = (width*height) + ( (width*height) << 1 ) -1;
  register int y0, y1, u, v;
  register int r, g, b;

  
  switch (byte_order) {
  case DC1394_BYTE_ORDER_YUYV:
    while (i >= 0) {
      v  = (uint8_t) src[i--] -128;
      y1 = (uint8_t) src[i--];
      u  = (uint8_t) src[i--] -128;
      y0  = (uint8_t) src[i--];
      YUV2RGB (y1, u, v, r, g, b);
      dest[j--] = b;
      dest[j--] = g;
      dest[j--] = r;
      YUV2RGB (y0, u, v, r, g, b);
      dest[j--] = b;
      dest[j--] = g;
      dest[j--] = r;
    }
    break;
  case DC1394_BYTE_ORDER_UYVY:
    while (i >= 0) {
      y1 = (uint8_t) src[i--];
      v  = (uint8_t) src[i--] - 128;
      y0 = (uint8_t) src[i--];
      u  = (uint8_t) src[i--] - 128;
      YUV2RGB (y1, u, v, r, g, b);
      dest[j--] = b;
      dest[j--] = g;
      dest[j--] = r;
      YUV2RGB (y0, u, v, r, g, b);
      dest[j--] = b;
      dest[j--] = g;
      dest[j--] = r;
    }
    break;
  default:
    fprintf(stderr,"Invalid overlay byte order\n");
    break;
  }
  
}


void
dc1394_YUV411_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
  register int i = (width*height) + ( (width*height) >> 1 )-1;
  register int j = (width*height) + ( (width*height) << 1 )-1;
  register int y0, y1, y2, y3, u, v;
  register int r, g, b;
  
  while (i >= 0) {
    y3 = (uint8_t) src[i--];
    y2 = (uint8_t) src[i--];
    v  = (uint8_t) src[i--] - 128;
    y1 = (uint8_t) src[i--];
    y0 = (uint8_t) src[i--];
    u  = (uint8_t) src[i--] - 128;
    YUV2RGB (y3, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y2, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y1, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y0, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
  }
}

void
dc1394_MONO8_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
  register int i = (width*height)-1;
  register int j = (width*height) + ( (width*height) << 1 )-1;
  register int y;

  while (i >= 0) {
    y = (uint8_t) src[i--];
    dest[j--] = y;
    dest[j--] = y;
    dest[j--] = y;
  }
}

void
dc1394_MONO16_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t bits)
{
  register int i = ((width*height) << 1)-1;
  register int j = (width*height) + ( (width*height) << 1 )-1;
  register int y;

  while (i > 0) {
    y = src[i--];
    y = (y + (src[i--]<<8))>>(bits-8);
    dest[j--] = y;
    dest[j--] = y;
    dest[j--] = y;
  }
}

// change a 16bit stereo image (8bit/channel) into two 8bit images on top
// of each other
void
dc1394_deinterlace_stereo(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height)
{
  register int i = (width*height)-1;
  register int j = ((width*height)>>1)-1;
  register int k = (width*height)-1;

  while (i >= 0) {
    dest[k--] = src[i--];
    dest[j--] = src[i--];
  }
}

dc1394error_t
dc1394_convert_to_YUV422(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order,
			 dc1394color_coding_t source_coding, uint32_t bits)
{
  switch(source_coding) {
  case DC1394_COLOR_CODING_YUV422:
    dc1394_YUV422_to_YUV422(src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_YUV411:
    dc1394_YUV411_to_YUV422(src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_YUV444:
    dc1394_YUV444_to_YUV422(src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_RGB8:
    dc1394_RGB8_to_YUV422(src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_MONO8:
  case DC1394_COLOR_CODING_RAW8:
    dc1394_MONO8_to_YUV422(src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_MONO16:
    dc1394_MONO16_to_YUV422(src, dest, width, height, byte_order, bits);
    break;
  case DC1394_COLOR_CODING_RGB16:
    dc1394_RGB16_to_YUV422(src, dest, width, height, byte_order, bits);
    break;
  default:
    fprintf(stderr,"Conversion to YUV422 from this color coding is not supported\n");
    return DC1394_FAILURE;
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_convert_to_MONO8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order,
			dc1394color_coding_t source_coding, uint32_t bits)
{
  switch(source_coding) {
  case DC1394_COLOR_CODING_MONO16:
    dc1394_MONO16_to_MONO8(src, dest, width, height, bits);
    break;
  case DC1394_COLOR_CODING_MONO8:
    memcpy(dest, src, width*height);
    break;
  default:
    fprintf(stderr,"Conversion to MONO8 from this color coding is not supported\n");
    return DC1394_FAILURE;
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_convert_to_RGB8(uint8_t *restrict src, uint8_t *restrict dest, uint32_t width, uint32_t height, uint32_t byte_order,
		       dc1394color_coding_t source_coding, uint32_t bits)
{
  switch(source_coding) {
  case DC1394_COLOR_CODING_RGB16:
    dc1394_RGB16_to_RGB8 (src, dest, width, height, bits);
    break;
  case DC1394_COLOR_CODING_YUV444:
    dc1394_YUV444_to_RGB8 (src, dest, width, height);
    break;
  case DC1394_COLOR_CODING_YUV422:
    dc1394_YUV422_to_RGB8 (src, dest, width, height, byte_order);
    break;
  case DC1394_COLOR_CODING_YUV411:
    dc1394_YUV411_to_RGB8 (src, dest, width, height);
    break;
  case DC1394_COLOR_CODING_MONO8:
  case DC1394_COLOR_CODING_RAW8:
    dc1394_MONO8_to_RGB8 (src, dest, width, height);
    break;
  case DC1394_COLOR_CODING_MONO16:
    dc1394_MONO16_to_RGB8 (src, dest, width, height,bits);
    break;
  case DC1394_COLOR_CODING_RGB8:
    memcpy(dest, src, width*height*3);
    break;
  default:
    fprintf(stderr,"Conversion to RGB8 from this color coding is not supported\n");
    return DC1394_FAILURE;
  }

  return DC1394_SUCCESS;
}
