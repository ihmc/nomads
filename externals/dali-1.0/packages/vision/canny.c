/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * canny.c
 *
 * Sugata - Sep 97
 *
 * Canny edge detector (fixed point implementation of NMS)
 *
 *----------------------------------------------------------------------
 */

#include "visionInt.h"

#define EDGE_PIXEL 255
#define NO_EDGE_PIXEL 0
#define DUMMY_PIXEL 2

static int canny_edge(register short *dx, register short *dy, register int *magn,
                      const long width, const long height, const int thresh,
                      register unsigned char *output, register int outstride);
static int canny_edge2(register short *dx, register short *dy, register int *magn,
                       const long width, const long height, const int thresh,
                       const int thresh2, unsigned char *output, register int outstride);
static int ByteGradientFull(ByteImage *img, short *dx, short *dy, int *mag);

/* Functions */

int ByteEdgeDetectCanny (img, edges, thresh1, thresh2)
    ByteImage *img;
    ByteImage *edges;
    int thresh1;
    int thresh2;
{
    int numbytes;
    short *dx, *dy;
    int *mag;

    if ((img->width != edges->width) ||
        (img->height != edges->height)) {
        return DVM_DIFFERENT_SIZES;
    }

    /* Calculate sizes and allocate space */

    numbytes = img->width*img->height;
    dx  = (short *)MALLOC(numbytes * sizeof(short));
    if (!dx) {
        return DVM_ALLOC_ERROR;
    }
    dy  = (short *)MALLOC(numbytes * sizeof(short));
    if (!dy) {
        free(dx);
        return DVM_ALLOC_ERROR;
    }
    mag = (int *)MALLOC(numbytes * sizeof(int));
    if (!mag) {
        free(dx);
        free(dy);
        return DVM_ALLOC_ERROR;
    }

    ByteGradientFull(img, dx, dy, mag);

    thresh1 = thresh1 * thresh1 * 4 * ((thresh1 < 0) ? -1 : 1);
    thresh2 = thresh2 * thresh2 * 4 * ((thresh2 < 0) ? -1 : 1);

    if (thresh1 <= 0)   thresh1 = 1;    /* ensure thresholds > 0.0 */

    if (thresh2 == 0) {    /* only one threshold specified */
        canny_edge(dx,dy,mag,img->width,img->height,
                   thresh1,edges->firstByte,edges->parentWidth);
    } else {                       /* two thresholds were specified */
        if (thresh1 > thresh2) {     /* swap 'em if necessary */
            int tmp = thresh1;
            thresh1 = thresh2;
            thresh2 = tmp;
        }
    
        canny_edge2(dx,dy,mag,img->width,img->height,
                    thresh1,thresh2,edges->firstByte, edges->parentWidth);
    }

    /* Free the dx, dy and mag */
    free(dx);
    free(dy);
    free(mag);
    return DVM_VISION_OK;  
}



/******************************* Local Functions *******************************/

static long offset1[4];
static long offset2[4];

static int NMS(const int *magn, int mag, short dx, short dy, long w)
{
    int reg;
    long o1, o2;
    int i1,i2,sx,sy,s, m1,m2,denom;
    int answer;

    sx = dx < 0?-1:1;
    sy = dy < 0?-1:1;

    dx *= sx;
    dy *= sy;
    s = sx*sy;

    if (dy == 0) {
        m1 = *(magn + 1);
        m2 = *(magn - 1);
    } else if (dx == 0) {
        m1 = *(magn + w);
        m2 = *(magn - w);
    } else {
        if (s > 0) {
            if (dy <= dx) {
                reg = 2;
                i1 = dy;
                i2 = dx - dy;
                denom = dx;
            } else {
                reg = 3;
                i2 = dx;
                i1 = dy - dx;
                denom = dy;
            }
        } else { /* s < 0 */
            if (dy <= dx) {
                reg = 1;
                i2 = dy;
                i1 = dx - dy;
                denom = dx;
            } else {
                reg = 0;
                i1 = dx;
                i2 = dy - dx;
                denom = dy;
            }
        }
        o1 = offset1[reg];
        o2 = offset2[reg];
        m1 = (*(magn + o1)*i2 + *(magn + o2)*i1);
        m2 = (*(magn - o1)*i2 + *(magn - o2)*i1);
        mag *= denom;
    }
    answer = (mag>=m1 && mag>=m2 && m1!=m2);
    return answer; /* return 1 iff passes NMS */
}


static int canny_edge(register short *dx, register short *dy, register int *magn,
                      const long x_m, const long y_m, const int thresh,
                      register unsigned char *output, register int outstride)
{
    int *magn_max, mag;
    register int *magn_max_x;


    offset1[0] = x_m;   offset2[0] = x_m-1;    /* 225..270 and 45..90 */
    offset1[1] = x_m-1; offset2[1] = -1;       /* 180..225 and 0..45 */
    offset1[2] = 1;     offset2[2] = x_m+1;    /* 315..360 and 135..180 */
    offset1[3] = x_m+1; offset2[3] = x_m;      /* 270..315 and 90..135 */ 

    output += outstride + 1;                 /* skip first row */
    dx     += x_m + 1;                       /* leave zero=>no edge */
    dy     += x_m + 1;
  
    for (magn_max = magn + x_m*(y_m - 1) + 1, magn += x_m + 1; magn < magn_max; ) {
        for (magn_max_x = magn + x_m - 2; magn < magn_max_x;
             output++, dx++, dy++, magn++)
        {
            if ((mag = *magn) < thresh)      /* don't even need to do NMS */
                *output = NO_EDGE_PIXEL;
            else                                     /* interpolate gradient & do NMS */
                *output = EDGE_PIXEL*NMS(magn, mag, *dx, *dy, x_m);
        }

        output += outstride-x_m+2;           /* skip over last pixel on this line */
        dx     += 2;                         /* and first pixel on next line */
        dy     += 2;
        magn   += 2;
    }

    return 0;
}


static int canny_edge2(register short *dx, register short *dy, register int *magn,
                       const long x_m, const long y_m, const int thresh,
                       const int thresh2, unsigned char *output, register int outstride)
{
    register unsigned char *out;
    int *magn_max, *magn_max_x;
    int mag, i;
    unsigned char **stktop;
    unsigned char **stack;


    offset1[0] = x_m;   offset2[0] = x_m-1;    /* 225..270 and 45..90 */
    offset1[1] = x_m-1; offset2[1] = -1;       /* 180..225 and 0..45 */
    offset1[2] = 1;     offset2[2] = x_m+1;    /* 315..360 and 135..180 */
    offset1[3] = x_m+1; offset2[3] = x_m;      /* 270..315 and 90..135 */ 

    stktop = stack = (unsigned char **)MALLOC(x_m*y_m*sizeof(unsigned char *)); /* get a stack */

    if (stack == (unsigned char **) NULL)
        return -1;
    memset((void *)stack,0,x_m*y_m*sizeof(unsigned char *));
  
    out  = output;
    while (out < output + x_m)               /* make first row 0 */
        *out++ = NO_EDGE_PIXEL;
    out += outstride - x_m;

    dx  += x_m + 1;                          /* skip over first col */
    dy  += x_m + 1;

    for (magn_max = magn + x_m*(y_m - 1) + 1, magn += x_m + 1; magn < magn_max; ) {
        *out++ = NO_EDGE_PIXEL;
        for (magn_max_x = magn + x_m - 2; magn < magn_max_x;
             out++, dx++, dy++, magn++) {
            if ((mag = *magn) < thresh) {            /* no possible edge */
                *out = NO_EDGE_PIXEL;
            } else {
                if (NMS(magn, mag, *dx, *dy,x_m)) { /* check if passes NMS */
                    if (mag >= thresh2) {
                        *out = EDGE_PIXEL;        /* definitely have an edge */
                        *(stktop++) = out;        /* put edge pixel addr on stack */
                    } else {
                        *out = DUMMY_PIXEL;       /* maybe have an edge */
                    }
                } else {
                    *out = NO_EDGE_PIXEL;         /* no edge here */
                }
            }
        }
        
        *out++ = NO_EDGE_PIXEL;
        out  += outstride-x_m;   

        dx   += 2;                  /* skip over last pixel on this line */
        dy   += 2;                  /* and first pixel on next line */
        magn += 2;
    }

    for (i=0; i<x_m; i++)
        *out++ = NO_EDGE_PIXEL;     /* Last row gets 0 too */

    while (stktop > stack) {
        out = *--stktop;

        /* look at neighbors. if a neighbor is DUMMY make it EDGE and add to stack */
        /* continue until stack empty */

        if (*(out -= outstride + 1) == DUMMY_PIXEL)        /* upper-left */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(++out) == DUMMY_PIXEL)                       /* upper-middle */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(++out) == DUMMY_PIXEL)                       /* upper-right */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(out += x_m) == DUMMY_PIXEL)                  /* middle-right */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(out -= 2) == DUMMY_PIXEL)                    /* middle-left */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(out += x_m) == DUMMY_PIXEL)                  /* lower-left */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(++out) == DUMMY_PIXEL)                       /* lower-middle */
            *out = EDGE_PIXEL, *(stktop++) = out;
        if (*(++out) == DUMMY_PIXEL)                       /* lower-right */
            *out = EDGE_PIXEL, *(stktop++) = out;
    }

    { 
        register unsigned char *outend = output + x_m*y_m; /* get rid of any remaining DUMMY's */
        
        for (out = output; out < outend; out++ )
            if (*out == DUMMY_PIXEL)
                *out = NO_EDGE_PIXEL;
    }

    free(stack);
  
    return 0;
}

int ByteGradientFull(ByteImage *img, short *dx, short *dy, int *mag)
{
    register unsigned char *imgp, *img_xendp;
    register short *dxp, *dyp;
    register int *magp;
    register short x, y, a, b, c, d;
    register int width;
    register int height;
    register int scanpad, scanpad_dx, scanpad_dy, scanpad_mag;
    register unsigned char *img_endp;
  
    /*if (img == NULL  ||  dx == NULL  ||
        dy  == NULL  ||  mag == NULL)
        return -1;*/

    width  = img->width;
    height = img->height;
    scanpad = img->parentWidth;
    scanpad_mag = scanpad_dy = scanpad_dx = img->width;

  /* Get the stores */
    imgp = img->firstByte;
    dxp  = dx;
    dyp  = dy;
    magp = mag;
  
  /* Now compute */
    for (img_endp = imgp + scanpad*(height-1); imgp < img_endp; ) {
        a = *imgp;
        c = *(imgp+scanpad);
    
        for (img_xendp = imgp + width - 1; imgp < img_xendp; ) { /* this really works! */
            imgp++;

            b = *(imgp);
            d = *(imgp+scanpad);

            a = d - a;
            c = b - c;
            x = a + c;
            y = a - c;

            a = b;
            c = d;
      
            *(dxp++)  = x;
            *(dyp++)  = y;
            *(magp++) = x*x + y*y;
            /* printf("%d ",x*x + y*y);*/
        }

        imgp += scanpad - width + 1;            /* last column gets 0.0 */
        *(dxp++) = *(dyp++) = *(magp++) = 0;
        dxp += scanpad_dx - width;
        dyp += scanpad_dy - width;
        magp += scanpad_mag - width;
    }

    for (img_endp += width; imgp < img_endp; imgp++) /* last row gets 0.0 */
        *(dxp++) = *(dyp++) = *(magp++) = 0;

    return 0;
}
