/*
 * 1394-Based Digital Camera Control Library
 * Bayer pattern decoding functions
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

#define CLIP(in, out)\
   in = in < 0 ? 0 : in;\
   in = in > 255 ? 255 : in;\
   out=in;
  
#define CLIP16(in, out, bits)\
   in = in < 0 ? 0 : in;\
   in = in > ((1<<bits)-1) ? ((1<<bits)-1) : in;\
   out=in;
  
void
ClearBorders(unsigned char *rgb, int sx, int sy, int w)
{
    int i, j;
    // black edges are added with a width w:
    i = 3 * sx * w - 1;
    j = 3 * sx * sy - 1;
    while (i >= 0) {
	rgb[i--] = 0;
	rgb[j--] = 0;
    }
    i = sx * (sy - 1) * 3 - 1 + w * 3;
    while (i > sx) {
	j = 6 * w;
	while (j > 0) {
	    rgb[i--] = 0;
	    j--;
	}
	i -= (sx - 2 * w) * 3;
    }
}

void
ClearBorders_uint16(uint16_t * rgb, int sx, int sy, int w)
{
    int i, j;

    // black edges:
    i = 3 * sx * w - 1;
    j = 3 * sx * sy - 1;
    while (i >= 0) {
	rgb[i--] = 0;
	rgb[j--] = 0;
    }

    i = sx * (sy - 1) * 3 - 1 + w * 3;
    while (i > sx) {
	j = 6 * w;
	while (j > 0) {
	    rgb[i--] = 0;
	    j--;
	}
	i -= (sx - 2 * w) * 3;
    }

}

/**************************************************************
 *     Color conversion functions for cameras that can        * 
 * output raw-Bayer pattern images, such as some Basler and   *
 * Point Grey camera. Most of the algos presented here come   *
 * from http://www-ise.stanford.edu/~tingchen/ and have been  *
 * converted from Matlab to C and extended to all elementary  *
 * patterns.                                                  *
 **************************************************************/

/* 8-bits versions */
/* insprired by OpenCV's Bayer decoding */

void
dc1394_bayer_NearestNeighbor(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;
    int i, imax, iinc;

    /* add black border */
    imax = sx * sy * 3;
    for (i = sx * (sy - 1) * 3; i < imax; i++) {
	rgb[i] = 0;
    }
    iinc = (sx - 1) * 3;
    for (i = (sx - 1) * 3; i < imax; i += iinc) {
	rgb[i++] = 0;
	rgb[i++] = 0;
	rgb[i++] = 0;
    }

    rgb += 1;
    width -= 1;
    height -= 1;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
      //int t0, t1;
	const unsigned char *bayerEnd = bayer + width;

        if (start_with_green) {
            rgb[-blue] = bayer[1];
            rgb[0] = bayer[bayerStep + 1];
            rgb[blue] = bayer[bayerStep];
            bayer++;
            rgb += 3;
        }

        if (blue > 0) {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[-1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[1] = bayer[bayerStep + 1];

                rgb[2] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[4] = bayer[bayerStep + 1];
            }
        } else {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[-1] = bayer[bayerStep + 1];

                rgb[4] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[2] = bayer[bayerStep + 1];
            }
        }

        if (bayer < bayerEnd) {
            rgb[-blue] = bayer[0];
            rgb[0] = bayer[1];
            rgb[blue] = bayer[bayerStep + 1];
            bayer++;
            rgb += 3;
        }

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}

/* OpenCV's Bayer decoding */
void
dc1394_bayer_Bilinear(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    /*
       the two letters  of the OpenCV name are respectively
       the 4th and 3rd letters from the blinky name,
       and we also have to switch R and B (OpenCV is BGR)

       CV_BayerBG2BGR <-> DC1394_COLOR_FILTER_BGGR
       CV_BayerGB2BGR <-> DC1394_COLOR_FILTER_GBRG
       CV_BayerGR2BGR <-> DC1394_COLOR_FILTER_GRBG

       int blue = tile == CV_BayerBG2BGR || tile == CV_BayerGB2BGR ? -1 : 1;
       int start_with_green = tile == CV_BayerGB2BGR || tile == CV_BayerGR2BGR;
     */
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;

    ClearBorders(rgb, sx, sy, 1);
    rgb += rgbStep + 3 + 1;
    height -= 2;
    width -= 2;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
	int t0, t1;
	const unsigned char *bayerEnd = bayer + width;

	if (start_with_green) {
	    /* OpenCV has a bug in the next line, which was
	       t0 = (bayer[0] + bayer[bayerStep * 2] + 1) >> 1; */
	    t0 = (bayer[1] + bayer[bayerStep * 2 + 1] + 1) >> 1;
	    t1 = (bayer[bayerStep] + bayer[bayerStep + 2] + 1) >> 1;
	    rgb[-blue] = (unsigned char) t0;
	    rgb[0] = bayer[bayerStep + 1];
	    rgb[blue] = (unsigned char) t1;
	    bayer++;
	    rgb += 3;
	}

	if (blue > 0) {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		      bayer[bayerStep * 2 + 2] + 2) >> 2;
		t1 = (bayer[1] + bayer[bayerStep] +
		      bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		      2) >> 2;
		rgb[-1] = (unsigned char) t0;
		rgb[0] = (unsigned char) t1;
		rgb[1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[2] = (unsigned char) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[4] = (unsigned char) t1;
	    }
	} else {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		      bayer[bayerStep * 2 + 2] + 2) >> 2;
		t1 = (bayer[1] + bayer[bayerStep] +
		      bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		      2) >> 2;
		rgb[1] = (unsigned char) t0;
		rgb[0] = (unsigned char) t1;
		rgb[-1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[4] = (unsigned char) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[2] = (unsigned char) t1;
	    }
	}

	if (bayer < bayerEnd) {
	    t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		  bayer[bayerStep * 2 + 2] + 2) >> 2;
	    t1 = (bayer[1] + bayer[bayerStep] +
		  bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		  2) >> 2;
	    rgb[-blue] = (unsigned char) t0;
	    rgb[0] = (unsigned char) t1;
	    rgb[blue] = bayer[bayerStep + 1];
	    bayer++;
	    rgb += 3;
	}

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}

/* High-Quality Linear Interpolation For Demosaicing Of
   Bayer-Patterned Color Images, by Henrique S. Malvar, Li-wei He, and
   Ross Cutler, in ICASSP'04 */
void
dc1394_bayer_HQLinear(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;

    ClearBorders(rgb, sx, sy, 2);
    rgb += 2 * rgbStep + 6 + 1;
    height -= 4;
    width -= 4;

    /* We begin with a (+1 line,+1 column) offset with respect to bilinear decoding, so start_with_green is the same, but blue is opposite */
    blue = -blue;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
	int t0, t1;
	const unsigned char *bayerEnd = bayer + width;
	const int bayerStep2 = bayerStep * 2;
	const int bayerStep3 = bayerStep * 3;
	const int bayerStep4 = bayerStep * 4;

	if (start_with_green) {
	    /* at green pixel */
	    rgb[0] = bayer[bayerStep2 + 2];
	    t0 = rgb[0] * 5
		+ ((bayer[bayerStep + 2] + bayer[bayerStep3 + 2]) << 2)
		- bayer[2]
		- bayer[bayerStep + 1]
		- bayer[bayerStep + 3]
		- bayer[bayerStep3 + 1]
		- bayer[bayerStep3 + 3]
		- bayer[bayerStep4 + 2]
		+ ((bayer[bayerStep2] + bayer[bayerStep2 + 4] + 1) >> 1);
	    t1 = rgb[0] * 5 +
		((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 3]) << 2)
		- bayer[bayerStep2]
		- bayer[bayerStep + 1]
		- bayer[bayerStep + 3]
		- bayer[bayerStep3 + 1]
		- bayer[bayerStep3 + 3]
		- bayer[bayerStep2 + 4]
		+ ((bayer[2] + bayer[bayerStep4 + 2] + 1) >> 1);
	    t0 = (t0 + 4) >> 3;
	    CLIP(t0, rgb[-blue]);
	    t1 = (t1 + 4) >> 3;
	    CLIP(t1, rgb[blue]);
	    bayer++;
	    rgb += 3;
	}

	if (blue > 0) {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		/* B at B */
		rgb[1] = bayer[bayerStep2 + 2];
		/* R at B */
		t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		       bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) << 1)
		    -
		    (((bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						     2]) * 3 + 1) >> 1)
		    + rgb[1] * 6;
		/* G at B */
		t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		       bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2]) << 1)
		    - (bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		    + (rgb[1] << 2);
		t0 = (t0 + 4) >> 3;
		CLIP(t0, rgb[-1]);
		t1 = (t1 + 4) >> 3;
		CLIP(t1, rgb[0]);
		/* at green pixel */
		rgb[3] = bayer[bayerStep2 + 3];
		t0 = rgb[3] * 5
		    + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) << 2)
		    - bayer[3]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep4 + 3]
		    +
		    ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] +
		      1) >> 1);
		t1 = rgb[3] * 5 +
		    ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) << 2)
		    - bayer[bayerStep2 + 1]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep2 + 5]
		    + ((bayer[3] + bayer[bayerStep4 + 3] + 1) >> 1);
		t0 = (t0 + 4) >> 3;
		CLIP(t0, rgb[2]);
		t1 = (t1 + 4) >> 3;
		CLIP(t1, rgb[4]);
	    }
	} else {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		/* R at R */
		rgb[-1] = bayer[bayerStep2 + 2];
		/* B at R */
		t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		       bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) << 1)
		    -
		    (((bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						     2]) * 3 + 1) >> 1)
		    + rgb[-1] * 6;
		/* G at R */
		t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		       bayer[bayerStep2 + 3] + bayer[bayerStep * 3 +
						     2]) << 1)
		    - (bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		    + (rgb[-1] << 2);
		t0 = (t0 + 4) >> 3;
		CLIP(t0, rgb[1]);
		t1 = (t1 + 4) >> 3;
		CLIP(t1, rgb[0]);

		/* at green pixel */
		rgb[3] = bayer[bayerStep2 + 3];
		t0 = rgb[3] * 5
		    + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) << 2)
		    - bayer[3]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep4 + 3]
		    +
		    ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] +
		      1) >> 1);
		t1 = rgb[3] * 5 +
		    ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) << 2)
		    - bayer[bayerStep2 + 1]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep2 + 5]
		    + ((bayer[3] + bayer[bayerStep4 + 3] + 1) >> 1);
		t0 = (t0 + 4) >> 3;
		CLIP(t0, rgb[4]);
		t1 = (t1 + 4) >> 3;
		CLIP(t1, rgb[2]);
	    }
	}

	if (bayer < bayerEnd) {
	    /* B at B */
	    rgb[blue] = bayer[bayerStep2 + 2];
	    /* R at B */
	    t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		   bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) << 1)
		-
		(((bayer[2] + bayer[bayerStep2] +
		   bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						 2]) * 3 + 1) >> 1)
		+ rgb[blue] * 6;
	    /* G at B */
	    t1 = (((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		    bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2])) << 1)
		- (bayer[2] + bayer[bayerStep2] +
		   bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		+ (rgb[blue] << 2);
	    t0 = (t0 + 4) >> 3;
	    CLIP(t0, rgb[-blue]);
	    t1 = (t1 + 4) >> 3;
	    CLIP(t1, rgb[0]);
	    bayer++;
	    rgb += 3;
	}

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}

/* coriander's Bayer decoding (GPL) */
/* Edge Sensing Interpolation II from http://www-ise.stanford.edu/~tingchen/ */
/*   (Laroche,Claude A.  "Apparatus and method for adaptively
     interpolating a full color image utilizing chrominance gradients"
     U.S. Patent 5,373,322) */
void
dc1394_bayer_EdgeSense(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    unsigned char *outR, *outG, *outB;
    register int i, j;
    int dh, dv;
    int tmp;

    // sx and sy should be even
    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_GBRG:
	// copy original RGB data to output images
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 0; j < sx; j += 2) {
	  outG[(i + j) * 3] = bayer[i + j];
	  outG[(i + sx + (j + 1)) * 3] = bayer[i + sx + (j + 1)];
	  outR[(i + j + 1) * 3] = bayer[i + j + 1];
	  outB[(i + sx + j) * 3] = bayer[i + sx + j];
	}
      }
      // process GREEN channel
      for (i = 3*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 3; j += 2) {
	  dh = abs(((outB[(i + j - 2) * 3] +
		     outB[(i + j + 2) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  dv = abs(((outB[(i - (sx<<1) + j) * 3] +
		     outB[(i + (sx<<1) + j) * 3]) >> 1)  -
		   outB[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP(tmp, outG[(i + j) * 3]);
	}
      }
	
      for (i = 2*sx; i < (sy - 3)*sx; i += (sx<<1)) {
	for (j = 3; j < sx - 2; j += 2) {
	  dh = abs(((outR[(i + j - 2) * 3] +
		     outR[(i + j + 2) * 3]) >>1 ) -
		   outR[(i + j) * 3]);
	  dv = abs(((outR[(i - (sx<<1) + j) * 3] +
		     outR[(i + (sx<<1) + j) * 3]) >>1 ) -
		   outR[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP(tmp, outG[(i + j) * 3]);
	}
      }
      // process RED channel
      for (i = 0; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outR[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
      }
      for (i = sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 1; j < sx; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outR[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outR[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outR[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outR[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
      }

      // process BLUE channel
      for (i = sx; i < sy*sx; i += (sx<<1)) {
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outB[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
      }
      for (i = 2*sx; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 0; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outB[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outB[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outB[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outB[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
      }
      break;

    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	// copy original RGB data to output images
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 0; j < sx; j += 2) {
	  outB[(i + j) * 3] = bayer[i + j];
	  outR[(i + sx + (j + 1)) * 3] = bayer[i + sx + (j + 1)];
	  outG[(i + j + 1) * 3] = bayer[i + j + 1];
	  outG[(i + sx + j) * 3] = bayer[i + sx + j];
	}
      }
      // process GREEN channel
      for (i = 2*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 3; j += 2) {
	  dh = abs(((outB[(i + j - 2) * 3] +
		    outB[(i + j + 2) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  dv = abs(((outB[(i - (sx<<1) + j) * 3] +
		    outB[(i + (sx<<1) + j) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP(tmp, outG[(i + j) * 3]);
	}
      }
      for (i = 3*sx; i < (sy - 3)*sx; i += (sx<<1)) {
	for (j = 3; j < sx - 2; j += 2) {
	  dh = abs(((outR[(i + j - 2) * 3] +
		    outR[(i + j + 2) * 3]) >> 1) -
		   outR[(i + j) * 3]);
	  dv = abs(((outR[(i - (sx<<1) + j) * 3] +
		    outR[(i + (sx<<1) + j) * 3]) >> 1) -
		   outR[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >>1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >>1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >>2;
	  }
	  CLIP(tmp, outG[(i + j) * 3]);
	}
      }
      // process RED channel
      for (i = sx; i < (sy - 1)*sx; i += (sx<<1)) {	// G-points (1/2)
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outR[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >>1);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
      }
      for (i = 2*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 1; j < sx; j += 2) {	// G-points (2/2)
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outR[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
	for (j = 2; j < sx - 1; j += 2) {	// B-points
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outR[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outR[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outR[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP(tmp, outR[(i + j) * 3]);
	}
      }
      
      // process BLUE channel
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outB[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
      }
      for (i = sx; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 0; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outB[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outB[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outB[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outB[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP(tmp, outB[(i + j) * 3]);
	}
      }
      break;
    default:			//---------------------------------------------------------
      fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
      return;
      break;
    }
    
    ClearBorders(rgb, sx, sy, 3);
}

/* coriander's Bayer decoding (GPL) */
void
dc1394_bayer_Downsample(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    unsigned char *outR, *outG, *outB;
    register int i, j;
    int tmp;

    sx *= 2;
    sy *= 2;

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	fprintf(stderr, "Bad Bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_GBRG:
	for (i = 0; i < sy*sx; i += (sx<<1)) {
	    for (j = 0; j < sx; j += 2) {
		tmp =
		    ((bayer[i + j] + bayer[i + sx + j + 1]) >> 1);
		CLIP(tmp, outG[((i >> 2) + (j >> 1)) * 3]);
		tmp = bayer[i + sx + j + 1];
		CLIP(tmp, outR[((i >> 2) + (j >> 1)) * 3]);
		tmp = bayer[i + sx + j];
		CLIP(tmp, outB[((i >> 2) + (j >> 1)) * 3]);
	    }
	}
	break;
    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	for (i = 0; i < sy*sx; i += (sx<<1)) {
	    for (j = 0; j < sx; j += 2) {
		tmp =
		    ((bayer[i + sx + j] + bayer[i + j + 1]) >> 1);
		CLIP(tmp, outG[((i >> 2) + (j >> 1)) * 3]);
		tmp = bayer[i + sx + j + 1];
		CLIP(tmp, outR[((i >> 2) + (j >> 1)) * 3]);
		tmp = bayer[i + j];
		CLIP(tmp, outB[((i >> 2) + (j >> 1)) * 3]);
	    }
	}
	break;
    default:			//---------------------------------------------------------
	fprintf(stderr, "Bad Bayer pattern ID: %d\n", tile);
	return;
	break;
    }

}

/* this is the method used inside AVT cameras. See AVT docs. */
void
dc1394_bayer_Simple(const unsigned char *bayer, unsigned char *rgb, int sx, int sy, int tile)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    int blue = tile == DC1394_COLOR_FILTER_BGGR
        || tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
        || tile == DC1394_COLOR_FILTER_GRBG;
    int i, imax, iinc;

    /* add black border */
    imax = sx * sy * 3;
    for (i = sx * (sy - 1) * 3; i < imax; i++) {
        rgb[i] = 0;
    }
    iinc = (sx - 1) * 3;
    for (i = (sx - 1) * 3; i < imax; i += iinc) {
        rgb[i++] = 0;
        rgb[i++] = 0;
        rgb[i++] = 0;
    }

    rgb += 1;
    width -= 1;
    height -= 1;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
        const unsigned char *bayerEnd = bayer + width;

        if (start_with_green) {
            rgb[-blue] = bayer[1];
            rgb[0] = (bayer[0] + bayer[bayerStep + 1] + 1) >> 1;
            rgb[blue] = bayer[bayerStep];
            bayer++;
            rgb += 3;
        }

        if (blue > 0) {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[-1] = bayer[0];
                rgb[0] = (bayer[1] + bayer[bayerStep] + 1) >> 1;
                rgb[1] = bayer[bayerStep + 1];

                rgb[2] = bayer[2];
                rgb[3] = (bayer[1] + bayer[bayerStep + 2] + 1) >> 1;
                rgb[4] = bayer[bayerStep + 1];
            }
        } else {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[1] = bayer[0];
                rgb[0] = (bayer[1] + bayer[bayerStep] + 1) >> 1;
                rgb[-1] = bayer[bayerStep + 1];

                rgb[4] = bayer[2];
                rgb[3] = (bayer[1] + bayer[bayerStep + 2] + 1) >> 1;
                rgb[2] = bayer[bayerStep + 1];
            }
        }

        if (bayer < bayerEnd) {
            rgb[-blue] = bayer[0];
            rgb[0] = (bayer[1] + bayer[bayerStep] + 1) >> 1;
            rgb[blue] = bayer[bayerStep + 1];
            bayer++;
            rgb += 3;
        }

        bayer -= width;
        rgb -= width * 3;

        blue = -blue;
        start_with_green = !start_with_green;
    }
}

/* 16-bits versions */

/* insprired by OpenCV's Bayer decoding */
void
dc1394_bayer_NearestNeighbor_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;
    int i, iinc, imax;

    /* add black border */
    imax = sx * sy * 3;
    for (i = sx * (sy - 1) * 3; i < imax; i++) {
	rgb[i] = 0;
    }
    iinc = (sx - 1) * 3;
    for (i = (sx - 1) * 3; i < imax; i += iinc) {
	rgb[i++] = 0;
	rgb[i++] = 0;
	rgb[i++] = 0;
    }

    rgb += 1;
    height -= 1;
    width -= 1;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
      //int t0, t1;
	const uint16_t *bayerEnd = bayer + width;

        if (start_with_green) {
            rgb[-blue] = bayer[1];
            rgb[0] = bayer[bayerStep + 1];
            rgb[blue] = bayer[bayerStep];
            bayer++;
            rgb += 3;
        }

        if (blue > 0) {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[-1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[1] = bayer[bayerStep + 1];

                rgb[2] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[4] = bayer[bayerStep + 1];
            }
        } else {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                rgb[1] = bayer[0];
                rgb[0] = bayer[1];
                rgb[-1] = bayer[bayerStep + 1];

                rgb[4] = bayer[2];
                rgb[3] = bayer[bayerStep + 2];
                rgb[2] = bayer[bayerStep + 1];
            }
        }

        if (bayer < bayerEnd) {
            rgb[-blue] = bayer[0];
            rgb[0] = bayer[1];
            rgb[blue] = bayer[bayerStep + 1];
            bayer++;
            rgb += 3;
        }

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}
/* OpenCV's Bayer decoding */
void
dc1394_bayer_Bilinear_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;

    rgb += rgbStep + 3 + 1;
    height -= 2;
    width -= 2;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
	int t0, t1;
	const uint16_t *bayerEnd = bayer + width;

	if (start_with_green) {
	    /* OpenCV has a bug in the next line, which was
	       t0 = (bayer[0] + bayer[bayerStep * 2] + 1) >> 1; */
	    t0 = (bayer[1] + bayer[bayerStep * 2 + 1] + 1) >> 1;
	    t1 = (bayer[bayerStep] + bayer[bayerStep + 2] + 1) >> 1;
	    rgb[-blue] = (uint16_t) t0;
	    rgb[0] = bayer[bayerStep + 1];
	    rgb[blue] = (uint16_t) t1;
	    bayer++;
	    rgb += 3;
	}

	if (blue > 0) {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		      bayer[bayerStep * 2 + 2] + 2) >> 2;
		t1 = (bayer[1] + bayer[bayerStep] +
		      bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		      2) >> 2;
		rgb[-1] = (uint16_t) t0;
		rgb[0] = (uint16_t) t1;
		rgb[1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[2] = (uint16_t) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[4] = (uint16_t) t1;
	    }
	} else {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		      bayer[bayerStep * 2 + 2] + 2) >> 2;
		t1 = (bayer[1] + bayer[bayerStep] +
		      bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		      2) >> 2;
		rgb[1] = (uint16_t) t0;
		rgb[0] = (uint16_t) t1;
		rgb[-1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[4] = (uint16_t) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[2] = (uint16_t) t1;
	    }
	}

	if (bayer < bayerEnd) {
	    t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		  bayer[bayerStep * 2 + 2] + 2) >> 2;
	    t1 = (bayer[1] + bayer[bayerStep] +
		  bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		  2) >> 2;
	    rgb[-blue] = (uint16_t) t0;
	    rgb[0] = (uint16_t) t1;
	    rgb[blue] = bayer[bayerStep + 1];
	    bayer++;
	    rgb += 3;
	}

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}

/* High-Quality Linear Interpolation For Demosaicing Of
   Bayer-Patterned Color Images, by Henrique S. Malvar, Li-wei He, and
   Ross Cutler, in ICASSP'04 */
void
dc1394_bayer_HQLinear_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    const int bayerStep = sx;
    const int rgbStep = 3 * sx;
    int width = sx;
    int height = sy;
    /*
       the two letters  of the OpenCV name are respectively
       the 4th and 3rd letters from the blinky name,
       and we also have to switch R and B (OpenCV is BGR)

       CV_BayerBG2BGR <-> DC1394_COLOR_FILTER_BGGR
       CV_BayerGB2BGR <-> DC1394_COLOR_FILTER_GBRG
       CV_BayerGR2BGR <-> DC1394_COLOR_FILTER_GRBG

       int blue = tile == CV_BayerBG2BGR || tile == CV_BayerGB2BGR ? -1 : 1;
       int start_with_green = tile == CV_BayerGB2BGR || tile == CV_BayerGR2BGR;
     */
    int blue = tile == DC1394_COLOR_FILTER_BGGR
	|| tile == DC1394_COLOR_FILTER_GBRG ? -1 : 1;
    int start_with_green = tile == DC1394_COLOR_FILTER_GBRG
	|| tile == DC1394_COLOR_FILTER_GRBG;

    ClearBorders_uint16(rgb, sx, sy, 2);
    rgb += 2 * rgbStep + 6 + 1;
    height -= 4;
    width -= 4;

    /* We begin with a (+1 line,+1 column) offset with respect to bilinear decoding, so start_with_green is the same, but blue is opposite */
    blue = -blue;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
	int t0, t1;
	const uint16_t *bayerEnd = bayer + width;
	const int bayerStep2 = bayerStep * 2;
	const int bayerStep3 = bayerStep * 3;
	const int bayerStep4 = bayerStep * 4;

	if (start_with_green) {
	    /* at green pixel */
	    rgb[0] = bayer[bayerStep2 + 2];
	    t0 = rgb[0] * 5
		+ ((bayer[bayerStep + 2] + bayer[bayerStep3 + 2]) << 2)
		- bayer[2]
		- bayer[bayerStep + 1]
		- bayer[bayerStep + 3]
		- bayer[bayerStep3 + 1]
		- bayer[bayerStep3 + 3]
		- bayer[bayerStep4 + 2]
		+ ((bayer[bayerStep2] + bayer[bayerStep2 + 4] + 1) >> 1);
	    t1 = rgb[0] * 5 +
		((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 3]) << 2)
		- bayer[bayerStep2]
		- bayer[bayerStep + 1]
		- bayer[bayerStep + 3]
		- bayer[bayerStep3 + 1]
		- bayer[bayerStep3 + 3]
		- bayer[bayerStep2 + 4]
		+ ((bayer[2] + bayer[bayerStep4 + 2] + 1) >> 1);
	    t0 = (t0 + 4) >> 3;
	    CLIP16(t0, rgb[-blue], bits);
	    t1 = (t1 + 4) >> 3;
	    CLIP16(t1, rgb[blue], bits);
	    bayer++;
	    rgb += 3;
	}

	if (blue > 0) {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		/* B at B */
		rgb[1] = bayer[bayerStep2 + 2];
		/* R at B */
		t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		       bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) << 1)
		    -
		    (((bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						     2]) * 3 + 1) >> 1)
		    + rgb[1] * 6;
		/* G at B */
		t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		       bayer[bayerStep2 + 3] + bayer[bayerStep * 3 +
						     2]) << 1)
		    - (bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		    + (rgb[1] << 2);
		t0 = (t0 + 4) >> 3;
		CLIP16(t0, rgb[-1], bits);
		t1 = (t1 + 4) >> 3;
		CLIP16(t1, rgb[0], bits);
		/* at green pixel */
		rgb[3] = bayer[bayerStep2 + 3];
		t0 = rgb[3] * 5
		    + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) << 2)
		    - bayer[3]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep4 + 3]
		    +
		    ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] +
		      1) >> 1);
		t1 = rgb[3] * 5 +
		    ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) << 2)
		    - bayer[bayerStep2 + 1]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep2 + 5]
		    + ((bayer[3] + bayer[bayerStep4 + 3] + 1) >> 1);
		t0 = (t0 + 4) >> 3;
		CLIP16(t0, rgb[2], bits);
		t1 = (t1 + 4) >> 3;
		CLIP16(t1, rgb[4], bits);
	    }
	} else {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		/* R at R */
		rgb[-1] = bayer[bayerStep2 + 2];
		/* B at R */
		t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		       bayer[bayerStep * 3 + 1] + bayer[bayerStep3 +
							3]) << 1)
		    -
		    (((bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						     2]) * 3 + 1) >> 1)
		    + rgb[-1] * 6;
		/* G at R */
		t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		       bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2]) << 1)
		    - (bayer[2] + bayer[bayerStep2] +
		       bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		    + (rgb[-1] << 2);
		t0 = (t0 + 4) >> 3;
		CLIP16(t0, rgb[1], bits);
		t1 = (t1 + 4) >> 3;
		CLIP16(t1, rgb[0], bits);

		/* at green pixel */
		rgb[3] = bayer[bayerStep2 + 3];
		t0 = rgb[3] * 5
		    + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) << 2)
		    - bayer[3]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep4 + 3]
		    +
		    ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] +
		      1) >> 1);
		t1 = rgb[3] * 5 +
		    ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) << 2)
		    - bayer[bayerStep2 + 1]
		    - bayer[bayerStep + 2]
		    - bayer[bayerStep + 4]
		    - bayer[bayerStep3 + 2]
		    - bayer[bayerStep3 + 4]
		    - bayer[bayerStep2 + 5]
		    + ((bayer[3] + bayer[bayerStep4 + 3] + 1) >> 1);
		t0 = (t0 + 4) >> 3;
		CLIP16(t0, rgb[4], bits);
		t1 = (t1 + 4) >> 3;
		CLIP16(t1, rgb[2], bits);
	    }
	}

	if (bayer < bayerEnd) {
	    /* B at B */
	    rgb[blue] = bayer[bayerStep2 + 2];
	    /* R at B */
	    t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		   bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) << 1)
		-
		(((bayer[2] + bayer[bayerStep2] +
		   bayer[bayerStep2 + 4] + bayer[bayerStep4 +
						 2]) * 3 + 1) >> 1)
		+ rgb[blue] * 6;
	    /* G at B */
	    t1 = (((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
		    bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2])) << 1)
		- (bayer[2] + bayer[bayerStep2] +
		   bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
		+ (rgb[blue] << 2);
	    t0 = (t0 + 4) >> 3;
	    CLIP16(t0, rgb[-blue], bits);
	    t1 = (t1 + 4) >> 3;
	    CLIP16(t1, rgb[0], bits);
	    bayer++;
	    rgb += 3;
	}

	bayer -= width;
	rgb -= width * 3;

	blue = -blue;
	start_with_green = !start_with_green;
    }
}

/* coriander's Bayer decoding (GPL) */
void
dc1394_bayer_EdgeSense_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    uint16_t *outR, *outG, *outB;
    register int i, j;
    int dh, dv;
    int tmp;

    // sx and sy should be even
    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_GBRG:
	// copy original RGB data to output images
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 0; j < sx; j += 2) {
	  outG[(i + j) * 3] = bayer[i + j];
	  outG[(i + sx + (j + 1)) * 3] = bayer[i + sx + (j + 1)];
	  outR[(i + j + 1) * 3] = bayer[i + j + 1];
	  outB[(i + sx + j) * 3] = bayer[i + sx + j];
	}
      }
      // process GREEN channel
      for (i = 3*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 3; j += 2) {
	  dh = abs(((outB[(i + j - 2) * 3] +
		     outB[(i + j + 2) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  dv = abs(((outB[(i - (sx<<1) + j) * 3] +
		     outB[(i + (sx<<1) + j) * 3]) >> 1)  -
		   outB[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
	
      for (i = 2*sx; i < (sy - 3)*sx; i += (sx<<1)) {
	for (j = 3; j < sx - 2; j += 2) {
	  dh = abs(((outR[(i + j - 2) * 3] +
		     outR[(i + j + 2) * 3]) >>1 ) -
		   outR[(i + j) * 3]);
	  dv = abs(((outR[(i - (sx<<1) + j) * 3] +
		     outR[(i + (sx<<1) + j) * 3]) >>1 ) -
		   outR[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      // process RED channel
      for (i = 0; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outR[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      for (i = sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 1; j < sx; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outR[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outR[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outR[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outR[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }

      // process BLUE channel
      for (i = sx; i < sy*sx; i += (sx<<1)) {
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outB[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      for (i = 2*sx; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 0; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outB[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outB[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outB[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outB[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      break;

    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	// copy original RGB data to output images
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 0; j < sx; j += 2) {
	  outB[(i + j) * 3] = bayer[i + j];
	  outR[(i + sx + (j + 1)) * 3] = bayer[i + sx + (j + 1)];
	  outG[(i + j + 1) * 3] = bayer[i + j + 1];
	  outG[(i + sx + j) * 3] = bayer[i + sx + j];
	}
      }
      // process GREEN channel
      for (i = 2*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 2; j < sx - 3; j += 2) {
	  dh = abs(((outB[(i + j - 2) * 3] +
		    outB[(i + j + 2) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  dv = abs(((outB[(i - (sx<<1) + j) * 3] +
		    outB[(i + (sx<<1) + j) * 3]) >> 1) -
		   outB[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >> 1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >> 2;
	  }
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      for (i = 3*sx; i < (sy - 3)*sx; i += (sx<<1)) {
	for (j = 3; j < sx - 2; j += 2) {
	  dh = abs(((outR[(i + j - 2) * 3] +
		    outR[(i + j + 2) * 3]) >> 1) -
		   outR[(i + j) * 3]);
	  dv = abs(((outR[(i - (sx<<1) + j) * 3] +
		    outR[(i + (sx<<1) + j) * 3]) >> 1) -
		   outR[(i + j) * 3]);
	  if (dh < dv)
	    tmp = (outG[(i + j - 1) * 3] +
		   outG[(i + j + 1) * 3]) >>1;
	  else {
	    if (dh > dv)
	      tmp = (outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >>1;
	    else
	      tmp = (outG[(i + j - 1) * 3] +
		     outG[(i + j + 1) * 3] +
		     outG[(i - sx + j) * 3] +
		     outG[(i + sx + j) * 3]) >>2;
	  }
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      // process RED channel
      for (i = sx; i < (sy - 1)*sx; i += (sx<<1)) {	// G-points (1/2)
	for (j = 2; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outR[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >>1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      for (i = 2*sx; i < (sy - 2)*sx; i += (sx<<1)) {
	for (j = 1; j < sx; j += 2) {	// G-points (2/2)
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outR[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
	for (j = 2; j < sx - 1; j += 2) {	// B-points
	  tmp = outG[(i + j) * 3] +
	      ((outR[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outR[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outR[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outR[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      
      // process BLUE channel
      for (i = 0; i < sy*sx; i += (sx<<1)) {
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i + j - 1) * 3] -
		outG[(i + j - 1) * 3] +
		outB[(i + j + 1) * 3] -
		outG[(i + j + 1) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      for (i = sx; i < (sy - 1)*sx; i += (sx<<1)) {
	for (j = 0; j < sx - 1; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j) * 3] -
		outG[(i - sx + j) * 3] +
		outB[(i + sx + j) * 3] -
		outG[(i + sx + j) * 3]) >> 1);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
	for (j = 1; j < sx - 2; j += 2) {
	  tmp = outG[(i + j) * 3] +
	      ((outB[(i - sx + j - 1) * 3] -
		outG[(i - sx + j - 1) * 3] +
		outB[(i - sx + j + 1) * 3] -
		outG[(i - sx + j + 1) * 3] +
		outB[(i + sx + j - 1) * 3] -
		outG[(i + sx + j - 1) * 3] +
		outB[(i + sx + j + 1) * 3] -
		outG[(i + sx + j + 1) * 3]) >> 2);
	  CLIP16(tmp, outR[(i + j) * 3], bits);
	}
      }
      break;
    default:			//---------------------------------------------------------
      fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
      return;
      break;
    }
   
    ClearBorders_uint16(rgb, sx, sy, 3);
}

/* coriander's Bayer decoding (GPL) */
void
dc1394_bayer_Downsample_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    uint16_t *outR, *outG, *outB;
    register int i, j;
    int tmp;

    sx *= 2;
    sy *= 2;

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	fprintf(stderr, "Bad Bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_GBRG:
	for (i = 0; i < sy*sx; i += (sx<<1)) {
	    for (j = 0; j < sx; j += 2) {
		tmp =
		    ((bayer[i + j] + bayer[i + sx + j + 1]) >> 1);
		CLIP16(tmp, outG[((i >> 2) + (j >> 1)) * 3], bits);
		tmp = bayer[i + sx + j + 1];
		CLIP16(tmp, outR[((i >> 2) + (j >> 1)) * 3], bits);
		tmp = bayer[i + sx + j];
		CLIP16(tmp, outB[((i >> 2) + (j >> 1)) * 3], bits);
	    }
	}
	break;
    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	for (i = 0; i < sy*sx; i += (sx<<1)) {
	    for (j = 0; j < sx; j += 2) {
		tmp =
		    ((bayer[i + sx + j] + bayer[i + j + 1]) >> 1);
		CLIP16(tmp, outG[((i >> 2) + (j >> 1)) * 3], bits);
		tmp = bayer[i + sx + j + 1];
		CLIP16(tmp, outR[((i >> 2) + (j >> 1)) * 3], bits);
		tmp = bayer[i + j];
		CLIP16(tmp, outB[((i >> 2) + (j >> 1)) * 3], bits);
	    }
	}
	break;
    default:			//---------------------------------------------------------
	fprintf(stderr, "Bad Bayer pattern ID: %d\n", tile);
	return;
	break;
    }

}

/* coriander's Bayer decoding (GPL) */
void
dc1394_bayer_Simple_uint16(const uint16_t *bayer, uint16_t *rgb, int sx, int sy, int tile, int bits)
{
    uint16_t *outR, *outG, *outB;
    register int i, j;
    int tmp, base;

    // sx and sy should be even
    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:
    case DC1394_COLOR_FILTER_BGGR:
	outR = &rgb[0];
	outG = &rgb[1];
	outB = &rgb[2];
	break;
    case DC1394_COLOR_FILTER_GBRG:
    case DC1394_COLOR_FILTER_RGGB:
	outR = &rgb[2];
	outG = &rgb[1];
	outB = &rgb[0];
	break;
    default:
	outR = NULL;
	outG = NULL;
	outB = NULL;
	break;
    }

    switch (tile) {
    case DC1394_COLOR_FILTER_GRBG:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_GBRG:
	for (i = 0; i < sy - 1; i += 2) {
	    for (j = 0; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base] + bayer[base + sx + 1]) >> 1);
		CLIP16(tmp, outG[base * 3], bits);
		tmp = bayer[base + 1];
		CLIP16(tmp, outR[base * 3], bits);
		tmp = bayer[base + sx];
		CLIP16(tmp, outB[base * 3], bits);
	    }
	}
	for (i = 0; i < sy - 1; i += 2) {
	    for (j = 1; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base + 1] + bayer[base + sx]) >> 1);
		CLIP16(tmp, outG[(base) * 3], bits);
		tmp = bayer[base];
		CLIP16(tmp, outR[(base) * 3], bits);
		tmp = bayer[base + 1 + sx];
		CLIP16(tmp, outB[(base) * 3], bits);
	    }
	}
	for (i = 1; i < sy - 1; i += 2) {
	    for (j = 0; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base + sx] + bayer[base + 1]) >> 1);
		CLIP16(tmp, outG[base * 3], bits);
		tmp = bayer[base + sx + 1];
		CLIP16(tmp, outR[base * 3], bits);
		tmp = bayer[base];
		CLIP16(tmp, outB[base * 3], bits);
	    }
	}
	for (i = 1; i < sy - 1; i += 2) {
	    for (j = 1; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base] + bayer[base + 1 + sx]) >> 1);
		CLIP16(tmp, outG[(base) * 3], bits);
		tmp = bayer[base + sx];
		CLIP16(tmp, outR[(base) * 3], bits);
		tmp = bayer[base + 1];
		CLIP16(tmp, outB[(base) * 3], bits);
	    }
	}
	break;
    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	for (i = 0; i < sy - 1; i += 2) {
	    for (j = 0; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base + sx] + bayer[base + 1]) >> 1);
		CLIP16(tmp, outG[base * 3], bits);
		tmp = bayer[base + sx + 1];
		CLIP16(tmp, outR[base * 3], bits);
		tmp = bayer[base];
		CLIP16(tmp, outB[base * 3], bits);
	    }
	}
	for (i = 1; i < sy - 1; i += 2) {
	    for (j = 0; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base] + bayer[base + 1 + sx]) >> 1);
		CLIP16(tmp, outG[(base) * 3], bits);
		tmp = bayer[base + 1];
		CLIP16(tmp, outR[(base) * 3], bits);
		tmp = bayer[base + sx];
		CLIP16(tmp, outB[(base) * 3], bits);
	    }
	}
	for (i = 0; i < sy - 1; i += 2) {
	    for (j = 1; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base] + bayer[base + sx + 1]) >> 1);
		CLIP16(tmp, outG[base * 3], bits);
		tmp = bayer[base + sx];
		CLIP16(tmp, outR[base * 3], bits);
		tmp = bayer[base + 1];
		CLIP16(tmp, outB[base * 3], bits);
	    }
	}
	for (i = 1; i < sy - 1; i += 2) {
	    for (j = 1; j < sx - 1; j += 2) {
		base = i * sx + j;
		tmp = ((bayer[base + 1] + bayer[base + sx]) >> 1);
		CLIP16(tmp, outG[(base) * 3], bits);
		tmp = bayer[base];
		CLIP16(tmp, outR[(base) * 3], bits);
		tmp = bayer[base + 1 + sx];
		CLIP16(tmp, outB[(base) * 3], bits);
	    }
	}
	break;
    default:			//---------------------------------------------------------
	fprintf(stderr, "Bad bayer pattern ID: %d\n", tile);
	return;
	break;
    }

    /* add black border */
    for (i = sx * (sy - 1) * 3; i < sx * sy * 3; i++) {
	rgb[i] = 0;
    }
    for (i = (sx - 1) * 3; i < sx * sy * 3; i += (sx - 1) * 3) {
	rgb[i++] = 0;
	rgb[i++] = 0;
	rgb[i++] = 0;
    }
}

int
dc1394_bayer_decoding_8bit(const unsigned char *bayer, unsigned char *rgb, uint_t sx, uint_t sy, uint_t tile, uint_t method)
{
  switch (method) {
  case DC1394_BAYER_METHOD_NEAREST:
    dc1394_bayer_NearestNeighbor(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_SIMPLE:
    dc1394_bayer_Simple(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_BILINEAR:
    dc1394_bayer_Bilinear(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_HQLINEAR:
    dc1394_bayer_HQLinear(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_DOWNSAMPLE:
    dc1394_bayer_Downsample(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_EDGESENSE:
    dc1394_bayer_EdgeSense(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  }

  return DC1394_INVALID_BAYER_METHOD;
}

int
dc1394_bayer_decoding_16bit(const uint16_t *bayer, uint16_t *rgb, uint_t sx, uint_t sy, uint_t tile, uint_t bits, uint_t method)
{
  switch (method) {
  case DC1394_BAYER_METHOD_NEAREST:
    dc1394_bayer_NearestNeighbor_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_SIMPLE:
    dc1394_bayer_Simple_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_BILINEAR:
    dc1394_bayer_Bilinear_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_HQLINEAR:
    dc1394_bayer_HQLinear_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_DOWNSAMPLE:
    dc1394_bayer_Downsample_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  case DC1394_BAYER_METHOD_EDGESENSE:
    dc1394_bayer_EdgeSense_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  }

  return DC1394_INVALID_BAYER_METHOD;
}
