/*
 * 1394-Based Digital Camera Control Library
 * Bayer pattern decoding functions
 * Copyright (C) Damien Douxchamps <ddouxchamps@users.sf.net>
 * 
 * Written by Damien Douxchamps and Frederic Devernay
 *
 * The original VNG Bayer decoding is from Dave Coffin's DCRAW.
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

#include <limits.h>
#include "conversions.h"

#define CLIP(in, out)\
   in = in < 0 ? 0 : in;\
   in = in > 255 ? 255 : in;\
   out=in;
  
#define CLIP16(in, out, bits)\
   in = in < 0 ? 0 : in;\
   in = in > ((1<<bits)-1) ? ((1<<bits)-1) : in;\
   out=in;
  
void
ClearBorders(uint8_t *rgb, int sx, int sy, int w)
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
dc1394_bayer_NearestNeighbor(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
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
	const uint8_t *bayerEnd = bayer + width;

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
dc1394_bayer_Bilinear(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
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
	const uint8_t *bayerEnd = bayer + width;

	if (start_with_green) {
	    /* OpenCV has a bug in the next line, which was
	       t0 = (bayer[0] + bayer[bayerStep * 2] + 1) >> 1; */
	    t0 = (bayer[1] + bayer[bayerStep * 2 + 1] + 1) >> 1;
	    t1 = (bayer[bayerStep] + bayer[bayerStep + 2] + 1) >> 1;
	    rgb[-blue] = (uint8_t) t0;
	    rgb[0] = bayer[bayerStep + 1];
	    rgb[blue] = (uint8_t) t1;
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
		rgb[-1] = (uint8_t) t0;
		rgb[0] = (uint8_t) t1;
		rgb[1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[2] = (uint8_t) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[4] = (uint8_t) t1;
	    }
	} else {
	    for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
		t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		      bayer[bayerStep * 2 + 2] + 2) >> 2;
		t1 = (bayer[1] + bayer[bayerStep] +
		      bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		      2) >> 2;
		rgb[1] = (uint8_t) t0;
		rgb[0] = (uint8_t) t1;
		rgb[-1] = bayer[bayerStep + 1];

		t0 = (bayer[2] + bayer[bayerStep * 2 + 2] + 1) >> 1;
		t1 = (bayer[bayerStep + 1] + bayer[bayerStep + 3] +
		      1) >> 1;
		rgb[4] = (uint8_t) t0;
		rgb[3] = bayer[bayerStep + 2];
		rgb[2] = (uint8_t) t1;
	    }
	}

	if (bayer < bayerEnd) {
	    t0 = (bayer[0] + bayer[2] + bayer[bayerStep * 2] +
		  bayer[bayerStep * 2 + 2] + 2) >> 2;
	    t1 = (bayer[1] + bayer[bayerStep] +
		  bayer[bayerStep + 2] + bayer[bayerStep * 2 + 1] +
		  2) >> 2;
	    rgb[-blue] = (uint8_t) t0;
	    rgb[0] = (uint8_t) t1;
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
dc1394_bayer_HQLinear(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
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
	const uint8_t *bayerEnd = bayer + width;
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
dc1394_bayer_EdgeSense(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
{
    uint8_t *outR, *outG, *outB;
    register int i3, j3, base;
    int i, j;
    int dh, dv;
    int tmp;
    int sx3=sx*3;

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
      for (i = 0, i3=0; i < sy*sx; i += (sx<<1), i3 += (sx3<<1)) {
	for (j = 0, j3=0; j < sx; j += 2, j3+=6) {
	  base=i3+j3;
	  outG[base]           = bayer[i + j];
	  outG[base + sx3 + 3] = bayer[i + j + sx + 1];
	  outR[base + 3]       = bayer[i + j + 1];
	  outB[base + sx3]     = bayer[i + j + sx];
	}
      }
      // process GREEN channel
      for (i3= 3*sx3; i3 < (sy - 2)*sx3; i3 += (sx3<<1)) {
	for (j3=6; j3 < sx3 - 9; j3+=6) {
	  base=i3+j3;
	  dh = abs(((outB[base - 6] +
		     outB[base + 6]) >> 1) -
		     outB[base]);
	  dv = abs(((outB[base - (sx3<<1)] +
		     outB[base + (sx3<<1)]) >> 1) -
		     outB[base]);
	  tmp = (((outG[base - 3]   + outG[base + 3]) >> 1) * (dh<=dv) +
		 ((outG[base - sx3] + outG[base + sx3]) >> 1) * (dh>dv));
	  //tmp = (dh==dv) ? tmp>>1 : tmp;
	  CLIP(tmp, outG[base]);
	}
      }
	
      for (i3=2*sx3; i3 < (sy - 3)*sx3; i3 += (sx3<<1)) {
	for (j3=9; j3 < sx3 - 6; j3+=6) {
	  base=i3+j3;
	  dh = abs(((outR[base - 6] +
		     outR[base + 6]) >>1 ) -
		     outR[base]);
	  dv = abs(((outR[base - (sx3<<1)] +
		     outR[base + (sx3<<1)]) >>1 ) -
		     outR[base]);
	  tmp = (((outG[base - 3]   + outG[base + 3]) >> 1) * (dh<=dv) +
		 ((outG[base - sx3] + outG[base + sx3]) >> 1) * (dh>dv));
	  //tmp = (dh==dv) ? tmp>>1 : tmp;
	  CLIP(tmp, outG[base]);
	}
      }
      // process RED channel
      for (i3=0; i3 < (sy - 1)*sx3; i3 += (sx3<<1)) {
	for (j3=6; j3 < sx3 - 3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - 3] -
		outG[base - 3] +
		outR[base + 3] -
		outG[base + 3]) >> 1);
	  CLIP(tmp, outR[base]);
	}
      }
      for (i3=sx3; i3 < (sy - 2)*sx3; i3 += (sx3<<1)) {
	for (j3=3; j3 < sx3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - sx3] -
		outG[base - sx3] +
		outR[base + sx3] -
		outG[base + sx3]) >> 1);
	  CLIP(tmp, outR[base]);
	}
	for (j3=6; j3 < sx3 - 3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - sx3 - 3] -
		outG[base - sx3 - 3] +
		outR[base - sx3 + 3] -
		outG[base - sx3 + 3] +
		outR[base + sx3 - 3] -
		outG[base + sx3 - 3] +
		outR[base + sx3 + 3] -
		outG[base + sx3 + 3]) >> 2);
	  CLIP(tmp, outR[base]);
	}
      }

      // process BLUE channel
      for (i3=sx3; i3 < sy*sx3; i3 += (sx3<<1)) {
	for (j3=3; j3 < sx3 - 6; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - 3] -
		outG[base - 3] +
		outB[base + 3] -
		outG[base + 3]) >> 1);
	  CLIP(tmp, outB[base]);
	}
      }
      for (i3=2*sx3; i3 < (sy - 1)*sx3; i3 += (sx3<<1)) {
	for (j3=0; j3 < sx3 - 3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - sx3] -
		outG[base - sx3] +
		outB[base + sx3] -
		outG[base + sx3]) >> 1);
	  CLIP(tmp, outB[base]);
	}
	for (j3=3; j3 < sx3 - 6; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - sx3 - 3] -
		outG[base - sx3 - 3] +
		outB[base - sx3 + 3] -
		outG[base - sx3 + 3] +
		outB[base + sx3 - 3] -
		outG[base + sx3 - 3] +
		outB[base + sx3 + 3] -
		outG[base + sx3 + 3]) >> 2);
	  CLIP(tmp, outB[base]);
	}
      }
      break;

    case DC1394_COLOR_FILTER_BGGR:	//---------------------------------------------------------
    case DC1394_COLOR_FILTER_RGGB:
	// copy original RGB data to output images
      for (i = 0, i3=0; i < sy*sx; i += (sx<<1), i3 += (sx3<<1)) {
	for (j = 0, j3=0; j < sx; j += 2, j3+=6) {
	  base=i3+j3;
	  outB[base] = bayer[i + j];
	  outR[base + sx3 + 3] = bayer[i + sx + (j + 1)];
	  outG[base + 3] = bayer[i + j + 1];
	  outG[base + sx3] = bayer[i + sx + j];
	}
      }
      // process GREEN channel
      for (i3=2*sx3; i3 < (sy - 2)*sx3; i3 += (sx3<<1)) {
	for (j3=6; j3 < sx3 - 9; j3+=6) {
	  base=i3+j3;
	  dh = abs(((outB[base - 6] +
		     outB[base + 6]) >> 1) -
		     outB[base]);
	  dv = abs(((outB[base - (sx3<<1)] +
		     outB[base + (sx3<<1)]) >> 1) -
		     outB[base]);
	  tmp = (((outG[base - 3]   + outG[base + 3]) >> 1) * (dh<=dv) +
		 ((outG[base - sx3] + outG[base + sx3]) >> 1) * (dh>dv));
	  //tmp = (dh==dv) ? tmp>>1 : tmp;
	  CLIP(tmp, outG[base]);
	}
      }
      for (i3=3*sx3; i3 < (sy - 3)*sx3; i3 += (sx3<<1)) {
	for (j3=9; j3 < sx3 - 6; j3+=6) {
	  base=i3+j3;
	  dh = abs(((outR[base - 6] +
		     outR[base + 6]) >> 1) -
		     outR[base]);
	  dv = abs(((outR[base - (sx3<<1)] +
		     outR[base + (sx3<<1)]) >> 1) -
		     outR[base]);
	  tmp = (((outG[base - 3]   + outG[base + 3]) >> 1) * (dh<=dv) +
		 ((outG[base - sx3] + outG[base + sx3]) >> 1) * (dh>dv));
	  //tmp = (dh==dv) ? tmp>>1 : tmp;
	  CLIP(tmp, outG[base]);
	}
      }
      // process RED channel
      for (i3=sx3; i3 < (sy - 1)*sx3; i3 += (sx3<<1)) {	// G-points (1/2)
	for (j3=6; j3 < sx3 - 3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - 3] -
		outG[base - 3] +
		outR[base + 3] -
		outG[base + 3]) >>1);
	  CLIP(tmp, outR[base]);
	}
      }
      for (i3=2*sx3; i3 < (sy - 2)*sx3; i3 += (sx3<<1)) {
	for (j3=3; j3 < sx3; j3+=6) {	// G-points (2/2)
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - sx3] -
		outG[base - sx3] +
		outR[base + sx3] -
		outG[base + sx3]) >> 1);
	  CLIP(tmp, outR[base]);
	}
	for (j3=6; j3 < sx3 - 3; j3+=6) {	// B-points
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outR[base - sx3 - 3] -
		outG[base - sx3 - 3] +
		outR[base - sx3 + 3] -
		outG[base - sx3 + 3] +
		outR[base + sx3 - 3] -
		outG[base + sx3 - 3] +
		outR[base + sx3 + 3] -
		outG[base + sx3 + 3]) >> 2);
	  CLIP(tmp, outR[base]);
	}
      }
      
      // process BLUE channel
      for (i = 0,i3=0; i < sy*sx; i += (sx<<1), i3 += (sx3<<1)) {
	for (j = 1, j3=3; j < sx - 2; j += 2, j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - 3] -
		outG[base - 3] +
		outB[base + 3] -
		outG[base + 3]) >> 1);
	  CLIP(tmp, outB[base]);
	}
      }
      for (i3=sx3; i3 < (sy - 1)*sx3; i3 += (sx3<<1)) {
	for (j3=0; j3 < sx3 - 3; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - sx3] -
		outG[base - sx3] +
		outB[base + sx3] -
		outG[base + sx3]) >> 1);
	  CLIP(tmp, outB[base]);
	}
	for (j3=3; j3 < sx3 - 6; j3+=6) {
	  base=i3+j3;
	  tmp = outG[base] +
	      ((outB[base - sx3 - 3] -
		outG[base - sx3 - 3] +
		outB[base - sx3 + 3] -
		outG[base - sx3 + 3] +
		outB[base + sx3 - 3] -
		outG[base + sx3 - 3] +
		outB[base + sx3 + 3] -
		outG[base + sx3 + 3]) >> 2);
	  CLIP(tmp, outB[base]);
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

/* coriander's Bayer decoding */
void
dc1394_bayer_Downsample(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
{
    uint8_t *outR, *outG, *outB;
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
		tmp = ((bayer[i + j] + bayer[i + sx + j + 1]) >> 1);
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
		tmp = ((bayer[i + sx + j] + bayer[i + j + 1]) >> 1);
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
dc1394_bayer_Simple(const uint8_t *restrict bayer, uint8_t *restrict rgb, int sx, int sy, int tile)
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
        const uint8_t *bayerEnd = bayer + width;

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
dc1394_bayer_NearestNeighbor_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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
dc1394_bayer_Bilinear_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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
dc1394_bayer_HQLinear_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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

/* coriander's Bayer decoding */
void
dc1394_bayer_EdgeSense_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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

/* coriander's Bayer decoding */
void
dc1394_bayer_Downsample_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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

/* coriander's Bayer decoding */
void
dc1394_bayer_Simple_uint16(const uint16_t *restrict bayer, uint16_t *restrict rgb, int sx, int sy, int tile, int bits)
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

/* Variable Number of Gradients, from dcraw <http://www.cybercom.net/~dcoffin/dcraw/> */
/* Ported to libdc1394 by Frederic Devernay */

#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2
 */
#define FC(row,col) \
	(filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

/*
   This algorithm is officially called:

   "Interpolation using a Threshold-based variable number of gradients"

   described in http://www-ise.stanford.edu/~tingchen/algodep/vargra.html

   I've extended the basic idea to work with non-Bayer filter arrays.
   Gradients are numbered clockwise from NW=0 to W=7.
 */
static const signed char bayervng_terms[] = {
    -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
    -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
    -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
    -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
    -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
    -1,-2,-1,+0,0,0x80, -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
    -1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,0x88, -1,-1,+1,-2,0,0x40,
    -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
    -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
    -1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
    -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
    -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
    -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
    +0,-2,+0,+0,1,0x80, +0,-1,+0,+1,1,0x88, +0,-1,+1,-2,0,0x40,
    +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
    +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
    +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
    +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
    +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
    +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,0x80,
    +1,-1,+1,+1,0,0x88, +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
    +1,+0,+2,+1,0,0x10
}, bayervng_chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };

static void
dc1394_bayer_VNG(const uint8_t *restrict bayer,
		 uint8_t *restrict dst, int sx, int sy,
		 dc1394color_filter_t pattern)
{
  const int height = sy, width = sx;
  static const signed char *cp;
  /* the following has the same type as the image */
  uint8_t (*brow[5])[3], *pix;          /* [FD] */
  int code[8][2][320], *ip, gval[8], gmin, gmax, sum[4];
  int row, col, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;
  uint32_t filters;                     /* [FD] */
  
  /* first, use bilinear bayer decoding */
  dc1394_bayer_Bilinear(bayer, dst, sx, sy, pattern);

  switch(pattern) {
      case DC1394_COLOR_FILTER_BGGR:
          filters = 0x16161616;
          break;
      case DC1394_COLOR_FILTER_GRBG:
          filters = 0x61616161;
          break;
      case DC1394_COLOR_FILTER_RGGB:
          filters = 0x94949494;
          break;
      case DC1394_COLOR_FILTER_GBRG:
          filters = 0x49494949;
          break;
      default:
          return;
  }
      
  for (row=0; row < 8; row++) {		/* Precalculate for VNG */
    for (col=0; col < 2; col++) {
      ip = code[row][col];
      for (cp=bayervng_terms, t=0; t < 64; t++) {
	y1 = *cp++;  x1 = *cp++;
	y2 = *cp++;  x2 = *cp++;
	weight = *cp++;
	grads = *cp++;
	color = FC(row+y1,col+x1);
	if (FC(row+y2,col+x2) != color) continue;
	diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
	if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
	*ip++ = (y1*width + x1)*3 + color; /* [FD] */
	*ip++ = (y2*width + x2)*3 + color; /* [FD] */
	*ip++ = weight;
	for (g=0; g < 8; g++)
	  if (grads & 1<<g) *ip++ = g;
	*ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=bayervng_chood, g=0; g < 8; g++) {
	y = *cp++;  x = *cp++;
	*ip++ = (y*width + x) * 3;      /* [FD] */
	color = FC(row,col);
	if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
	  *ip++ = (y*width + x) * 6 + color; /* [FD] */
	else
	  *ip++ = 0;
      }
    }
  }
  brow[4] = calloc (width*3, sizeof **brow);
  //merror (brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*width;
  for (row=2; row < height-2; row++) {		/* Do VNG interpolation */
    for (col=2; col < width-2; col++) {
        pix = dst + (row*width+col)*3;  /* [FD] */
      ip = code[row & 7][col & 1];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
	diff = ABS(pix[g] - pix[ip[1]]) << ip[2];
	gval[ip[3]] += diff;
	ip += 5;
	if ((g = ip[-1]) == -1) continue;
	gval[g] += diff;
	while ((g = *ip++) != -1)
	  gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];			/* Choose a threshold */
      for (g=1; g < 8; g++) {
	if (gmin > gval[g]) gmin = gval[g];
	if (gmax < gval[g]) gmax = gval[g];
      }
      if (gmax == 0) {
          memcpy (brow[2][col], pix, 3 * sizeof *dst); /* [FD] */
	continue;
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = FC(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
	if (gval[g] <= thold) {
	  for (c=0; c < 3; c++)         /* [FD] */
	    if (c == color && ip[1])
	      sum[c] += (pix[c] + pix[ip[1]]) >> 1;
	    else
	      sum[c] += pix[ip[0] + c];
	  num++;
	}
      }
      for (c=0; c < 3; c++) {           /* [FD] Save to buffer */
	t = pix[color];
	if (c != color)
	  t += (sum[c] - sum[color]) / num;
	CLIP(t,brow[2][col][c]);        /* [FD] */
      }
    }
    if (row > 3)				/* Write buffer to image */
        memcpy (dst + 3*((row-2)*width+2), brow[0]+2, (width-4)*3*sizeof *dst); /* [FD] */
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (dst + 3*((row-2)*width+2), brow[0]+2, (width-4)*3*sizeof *dst);
  memcpy (dst + 3*((row-1)*width+2), brow[1]+2, (width-4)*3*sizeof *dst);
  free (brow[4]);
}


static void
dc1394_bayer_VNG_uint16(const uint16_t *restrict bayer,
			uint16_t *restrict dst, int sx, int sy,
			dc1394color_filter_t pattern, int bits)
{
  const int height = sy, width = sx;
  static const signed char *cp;
  /* the following has the same type as the image */
  uint16_t (*brow[5])[3], *pix;          /* [FD] */
  int code[8][2][320], *ip, gval[8], gmin, gmax, sum[4];
  int row, col, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
  int g, diff, thold, num, c;
  uint32_t filters;                     /* [FD] */
  
  /* first, use bilinear bayer decoding */
  
  dc1394_bayer_Bilinear_uint16(bayer, dst, sx, sy, pattern, bits);

  switch(pattern) {
      case DC1394_COLOR_FILTER_BGGR:
          filters = 0x16161616;
          break;
      case DC1394_COLOR_FILTER_GRBG:
          filters = 0x61616161;
          break;
      case DC1394_COLOR_FILTER_RGGB:
          filters = 0x94949494;
          break;
      case DC1394_COLOR_FILTER_GBRG:
          filters = 0x49494949;
          break;
      default:
          return;
  }
      
  for (row=0; row < 8; row++) {		/* Precalculate for VNG */
    for (col=0; col < 2; col++) {
      ip = code[row][col];
      for (cp=bayervng_terms, t=0; t < 64; t++) {
	y1 = *cp++;  x1 = *cp++;
	y2 = *cp++;  x2 = *cp++;
	weight = *cp++;
	grads = *cp++;
	color = FC(row+y1,col+x1);
	if (FC(row+y2,col+x2) != color) continue;
	diag = (FC(row,col+1) == color && FC(row+1,col) == color) ? 2:1;
	if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
	*ip++ = (y1*width + x1)*3 + color; /* [FD] */
	*ip++ = (y2*width + x2)*3 + color; /* [FD] */
	*ip++ = weight;
	for (g=0; g < 8; g++)
	  if (grads & 1<<g) *ip++ = g;
	*ip++ = -1;
      }
      *ip++ = INT_MAX;
      for (cp=bayervng_chood, g=0; g < 8; g++) {
	y = *cp++;  x = *cp++;
	*ip++ = (y*width + x) * 3;      /* [FD] */
	color = FC(row,col);
	if (FC(row+y,col+x) != color && FC(row+y*2,col+x*2) == color)
	  *ip++ = (y*width + x) * 6 + color; /* [FD] */
	else
	  *ip++ = 0;
      }
    }
  }
  brow[4] = calloc (width*3, sizeof **brow);
  //merror (brow[4], "vng_interpolate()");
  for (row=0; row < 3; row++)
    brow[row] = brow[4] + row*width;
  for (row=2; row < height-2; row++) {		/* Do VNG interpolation */
    for (col=2; col < width-2; col++) {
        pix = dst + (row*width+col)*3;  /* [FD] */
      ip = code[row & 7][col & 1];
      memset (gval, 0, sizeof gval);
      while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
	diff = ABS(pix[g] - pix[ip[1]]) << ip[2];
	gval[ip[3]] += diff;
	ip += 5;
	if ((g = ip[-1]) == -1) continue;
	gval[g] += diff;
	while ((g = *ip++) != -1)
	  gval[g] += diff;
      }
      ip++;
      gmin = gmax = gval[0];			/* Choose a threshold */
      for (g=1; g < 8; g++) {
	if (gmin > gval[g]) gmin = gval[g];
	if (gmax < gval[g]) gmax = gval[g];
      }
      if (gmax == 0) {
          memcpy (brow[2][col], pix, 3 * sizeof *dst); /* [FD] */
	continue;
      }
      thold = gmin + (gmax >> 1);
      memset (sum, 0, sizeof sum);
      color = FC(row,col);
      for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
	if (gval[g] <= thold) {
	  for (c=0; c < 3; c++)         /* [FD] */
	    if (c == color && ip[1])
	      sum[c] += (pix[c] + pix[ip[1]]) >> 1;
	    else
	      sum[c] += pix[ip[0] + c];
	  num++;
	}
      }
      for (c=0; c < 3; c++) {           /* [FD] Save to buffer */
	t = pix[color];
	if (c != color)
	  t += (sum[c] - sum[color]) / num;
	CLIP16(t,brow[2][col][c],bits);        /* [FD] */
      }
    }
    if (row > 3)				/* Write buffer to image */
        memcpy (dst + 3*((row-2)*width+2), brow[0]+2, (width-4)*3*sizeof *dst); /* [FD] */
    for (g=0; g < 4; g++)
      brow[(g-1) & 3] = brow[g];
  }
  memcpy (dst + 3*((row-2)*width+2), brow[0]+2, (width-4)*3*sizeof *dst);
  memcpy (dst + 3*((row-1)*width+2), brow[1]+2, (width-4)*3*sizeof *dst);
  free (brow[4]);
}

dc1394error_t
dc1394_bayer_decoding_8bit(const uint8_t *restrict bayer, uint8_t *restrict rgb, uint32_t sx, uint32_t sy, dc1394color_filter_t tile, dc1394bayer_method_t method)
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
  case DC1394_BAYER_METHOD_VNG:
    dc1394_bayer_VNG(bayer, rgb, sx, sy, tile);
    return DC1394_SUCCESS;
  }

  return DC1394_INVALID_BAYER_METHOD;
}

dc1394error_t
dc1394_bayer_decoding_16bit(const uint16_t *restrict bayer, uint16_t *restrict rgb, uint32_t sx, uint32_t sy, dc1394color_filter_t tile, dc1394bayer_method_t method, uint32_t bits)
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
  case DC1394_BAYER_METHOD_VNG:
    dc1394_bayer_VNG_uint16(bayer, rgb, sx, sy, tile, bits);
    return DC1394_SUCCESS;
  }

  return DC1394_INVALID_BAYER_METHOD;
}

void
Adapt_buffer_bayer(dc1394video_frame_t *in, dc1394video_frame_t *out, dc1394bayer_method_t method)
{
  float fbpp;

  // conversions will halve the buffer size if the method is DOWNSAMPLE:
  out->size[0]=in->size[0];
  out->size[1]=in->size[1];
  if (method == DC1394_BAYER_METHOD_DOWNSAMPLE) {
    out->size[0]/=2; // ODD SIZE CASES NOT TAKEN INTO ACCOUNT
    out->size[1]/=2;
  }
  
  // as a convention we divide the image position by two in the case of a DOWNSAMPLE:
  out->position[0]=in->position[0];
  out->position[1]=in->position[1];
  if (method == DC1394_BAYER_METHOD_DOWNSAMPLE) {
    out->position[0]/=2;
    out->position[1]/=2;
  }

  // the destination color coding is ALWAYS RGB. Set this.
  if (in->color_coding==DC1394_COLOR_CODING_RAW16)
    out->color_coding=DC1394_COLOR_CODING_RGB16;
  else
    out->color_coding=DC1394_COLOR_CODING_RGB8;

  // keep the color filter value in all cases. If the format is not raw it will not be further used anyway
  out->color_filter=in->color_filter;

  // The output is never YUV, hence nothing to do about YUV byte order

  // bit depth is conserved for 16 bit and set to 8bit for 8bit:
  if (in->color_coding==DC1394_COLOR_CODING_RAW16)
    out->bit_depth=in->bit_depth;
  else
    out->bit_depth=8;

  // don't know what to do with stride... >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
  // out->stride=??
  
  // the video mode should not change. Color coding and other stuff can be accessed in specific fields of this struct
  out->video_mode = in->video_mode;

  // padding is kept:
  out->padding_bytes = in->padding_bytes;

  // image bytes changes:    >>>> TODO: STRIDE SHOULD BE TAKEN INTO ACCOUNT... <<<<
  dc1394_get_bytes_per_pixel(out->color_coding, &fbpp);
  out->image_bytes=(int)(out->size[0]*out->size[1]*fbpp);

  // total is image_bytes + padding_bytes
  out->total_bytes = out->image_bytes + out->padding_bytes;

  // bytes-per-packet and packets_per_frame are internal data that can be kept as is.
  out->bytes_per_packet  = in->bytes_per_packet;
  out->packets_per_frame = in->packets_per_frame;

  // timestamp, frame_behind, id and camera are copied too:
  out->timestamp = in->timestamp;
  out->frames_behind = in->frames_behind;
  out->camera = in->camera;
  out->id = in->id;
  
  // verify memory allocation:
  if (out->total_bytes>out->allocated_image_bytes) {
    free(out->image);
    out->image=(uint8_t*)malloc(out->total_bytes*sizeof(uint8_t));
  }

  // Copy padding bytes:
  memcpy(&(out->image[out->image_bytes]),&(in->image[in->image_bytes]),out->padding_bytes);

}
dc1394error_t
dc1394_debayer_frames(dc1394video_frame_t *in, dc1394video_frame_t *out, dc1394bayer_method_t method)
{

  switch (in->color_coding) {
  case DC1394_COLOR_CODING_RAW8:
    switch (method) {
    case DC1394_BAYER_METHOD_NEAREST:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_NearestNeighbor(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_SIMPLE:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Simple(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_BILINEAR:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Bilinear(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_HQLINEAR:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_HQLinear(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_DOWNSAMPLE:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Downsample(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_EDGESENSE:
      Adapt_buffer_bayer(in,out,method);
     dc1394_bayer_EdgeSense(in->image, out->image, in->size[0], in->size[1], in->color_filter);
     return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_VNG:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_VNG(in->image, out->image, in->size[0], in->size[1], in->color_filter);
      return DC1394_SUCCESS;
    default:
      return DC1394_INVALID_BAYER_METHOD;
    }
    break;
  case DC1394_COLOR_CODING_RAW16:
    switch (method) {
    case DC1394_BAYER_METHOD_NEAREST:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_NearestNeighbor_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_SIMPLE:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Simple_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_BILINEAR:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Bilinear_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_HQLINEAR:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_HQLinear_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_DOWNSAMPLE:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_Downsample_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_EDGESENSE:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_EdgeSense_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    case DC1394_BAYER_METHOD_VNG:
      Adapt_buffer_bayer(in,out,method);
      dc1394_bayer_VNG_uint16((uint16_t*)in->image, (uint16_t*)out->image, in->size[0], in->size[1], in->color_filter, in->bit_depth);
      return DC1394_SUCCESS;
    default:
      return DC1394_INVALID_BAYER_METHOD;
    }
    break;
  default:
    fprintf(stderr,"Conversion to between these two formats is not supported (yet)\n");
    return DC1394_FAILURE;
  }

  return DC1394_SUCCESS;
}
