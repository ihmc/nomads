/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "visionInt.h"


static unsigned char numBits[] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

#if 0
static unsigned char abs_val[] =
{255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241,
 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226,
 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211,
 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196,
 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181,
 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166,
 165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151,
 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136,
 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121,
 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106,
105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88,
 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69,
 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31,
 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165,
 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};
#endif


void
ByteMakeFromBitIntersect (rBuf, gBuf, bBuf, srcBuf1, srcBuf2)
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
    BitImage *srcBuf1;
    BitImage *srcBuf2;
{
    int h, bw;
    unsigned char *currSrc1, *currSrc2, *currR, *currG, *currB;
    int src1ParW, src2ParW, destParW;
    unsigned char *firstSrc1, *firstSrc2, *firstR, *firstG, *firstB;
    int x, y;
    int bits;
    int reds[4] =
    {255, 255, 0, 0}, greens[4] =
    {255, 0, 0, 0}, blues[4] =
    {255, 0, 255, 0};

    assert (rBuf->parentWidth == gBuf->parentWidth && rBuf->parentWidth == bBuf->parentWidth);

    h = srcBuf1->height;
    bw = srcBuf1->byteWidth;
    src1ParW = srcBuf1->parentWidth;
    src2ParW = srcBuf2->parentWidth;
    destParW = rBuf->parentWidth;
    firstSrc1 = srcBuf1->firstByte;
    firstSrc2 = srcBuf2->firstByte;
    firstR = rBuf->firstByte;
    firstG = gBuf->firstByte;
    firstB = bBuf->firstByte;

    /* Colors are assigned as follows:
     * 0 0 -> white
     * 0 1 -> red
     * 1 0 -> blue
     * 1 1 -> black
     */

    for (y = 0; y < h; y++) {
        currSrc1 = firstSrc1;
        currSrc2 = firstSrc2;
        currR = firstR;
        currG = firstG;
        currB = firstB;
        for (x = 0; x < bw; x++) {
            bits = BIT7 (*currSrc1) | (BIT7 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT6 (*currSrc1) | (BIT6 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT5 (*currSrc1) | (BIT5 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT4 (*currSrc1) | (BIT4 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT3 (*currSrc1) | (BIT3 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT2 (*currSrc1) | (BIT2 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT1 (*currSrc1) | (BIT1 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            bits = BIT0 (*currSrc1) | (BIT0 (*currSrc2) << 1);
            *currR++ = reds[bits];
            *currG++ = greens[bits];
            *currB++ = blues[bits];

            currSrc1++;
            currSrc2++;
        }
        firstSrc1 += src1ParW;
        firstSrc2 += src2ParW;
        firstR += destParW;
        firstG += destParW;
        firstB += destParW;
    }

}



float
BitCompare (buf1, buf2)
    BitImage *buf1;
    BitImage *buf2;
{
    int w, h, i;
    unsigned char *firstBuf1, *firstBuf2, *currBuf1, *currBuf2;
    int parW1, parW2;
    int sumDiff, sum1;
    float result;

    assert (buf1->byteWidth == buf2->byteWidth);
    assert (buf1->height == buf2->height);

    w = buf1->byteWidth;
    h = buf1->height;
    parW1 = buf1->parentWidth;
    parW2 = buf2->parentWidth;

    firstBuf1 = buf1->firstByte;
    firstBuf2 = buf2->firstByte;

    sumDiff = sum1 = 0;
    for (i = 0; i < h; i++) {
        currBuf1 = firstBuf1;
        currBuf2 = firstBuf2;

        DO_N_TIMES (w,
                    sumDiff += numBits[(*currBuf1) & (*currBuf2)];
                    sum1 += numBits[(*currBuf1) | (*currBuf2)];
                    currBuf1++;
                    currBuf2++;
            );

        firstBuf1 += parW1;
        firstBuf2 += parW2;
    }

    sum1++;
    result = (float) sumDiff / (float) (sum1);
    return result;
}


int
BitAllWhite (buf)
    BitImage *buf;
{
    int w, h, i, parW;
    unsigned char *firstBuf, *currBuf;
    int sum;

    w = buf->byteWidth;
    h = buf->height;
    parW = buf->parentWidth;

    firstBuf = buf->firstByte;
    for (i = 0; i < h; i++) {
        sum = 0;
        currBuf = firstBuf;
        DO_N_TIMES (w,
                    sum += *currBuf;
                    currBuf++;
            );
        if (sum) {
            return 0;
        }
        firstBuf += parW;
    }
    return 1;
}



void
BitFindCentroid (buf, val, xmean, ymean)
    BitImage *buf;
    int val;
    int *xmean;
    int *ymean;
{
    int w, h, i, j, parW;
    unsigned char *firstBuf, *currBuf;
    int horiz[2][1000];
    int vert[2][1000];
    unsigned char byte;
    int bit;
    int s, numPixels;

    w = buf->byteWidth;
    h = buf->height;
    parW = buf->parentWidth;
    firstBuf = buf->firstByte;

    memset (horiz, 0, 2000 * sizeof (int));
    memset (vert, 0, 2000 * sizeof (int));

    /* Calculate the number of black pixels in each row and column */
    for (i = 0; i < h; i++) {
        j = 0;
        currBuf = firstBuf;
        DO_N_TIMES (w,
                    byte = *currBuf;
                    bit = BIT7 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT6 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT5 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT4 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT3 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT2 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT1 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;
                    bit = BIT0 (byte);
                    vert[bit][j]++;
                    horiz[bit][i]++;
                    j++;

                    currBuf++;
            );
        firstBuf += parW;
    }

    /* Find the means */

    s = 0;
    numPixels = 0;

    for (i = 0; i < h; i++) {
        s += horiz[val][i] * i;
        numPixels += horiz[val][i];
    }

    *ymean = s / numPixels;

    s = 0;
    numPixels = 0;

    w <<= 3;

    for (i = 0; i < w; i++) {
        s += vert[val][i] * i;
        numPixels += vert[val][i];
    }

    *xmean = s / numPixels;
}


void
ByteFindBoundingBox (buf, px, py, x0, y0, x1, y1, x2, y2, x3, y3)
    ByteImage *buf;
    int px, py;
    int *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3;
{
    int w, h, parW;
    unsigned char *firstBuf, *bytePtr;
    int leftpx, rightpx, nextx, nexty;
    int leftSlope, rightSlope;
    unsigned char *centroid, *center;


    w = buf->width;
    h = buf->height;
    parW = buf->parentWidth;
    firstBuf = buf->firstByte;


    bytePtr = centroid = firstBuf + py * parW + px;

    rightpx = px - 1;
    while (*bytePtr == 0xff) {
        rightpx++;
        bytePtr++;
    }

    bytePtr = centroid;
    leftpx = px;
    while (*bytePtr == 0xff) {
        leftpx--;
        bytePtr--;
    }

    /* Find slope on right edge */
    center = centroid - parW;
    nextx = rightpx;
    nexty = py;
    for (; (nextx == rightpx) && (nexty >= 0); center -= parW, nexty--) {
        bytePtr = center;
        nextx = px - 1;
        while (*bytePtr == 0xff) {
            nextx++;
            bytePtr++;
        }
    }

    rightSlope = nextx - rightpx;
    bytePtr--;

    if (abs (rightSlope) > 4) {
        *y1 = nexty;
        *x1 = rightpx;

        bytePtr = centroid + rightpx - px;
        nexty = py;
        while (*(bytePtr + parW)) {
            bytePtr += parW;
            nexty++;
        }
        *x2 = rightpx;
        *y2 = nexty;
    } else if (rightSlope >= 1) {
        while (1) {
            if (*bytePtr == 0) {
                *x1 = nextx;
                *y1 = nexty + 1;
                break;
            }
            while (*(bytePtr + 1) == 0xff) {
                nextx++;
                bytePtr++;
            }
            bytePtr -= parW;
            nexty--;
        }

        bytePtr = centroid + rightpx - px;

        nextx = rightpx;
        nexty = py;

        while (1) {
            if (*(bytePtr - 3) == 0) {
                *x2 = nextx;
                *y2 = nexty - 1;
                break;
            }
            while (*bytePtr == 0) {
                bytePtr--;
                nextx--;
            }
            bytePtr += parW;
            nexty++;
        }
    } else {
        while (1) {
            if (*(bytePtr - 3) == 0) {
                *x1 = nextx;
                *y1 = nexty + 1;
                break;
            }
            while (*(bytePtr - 1) == 0) {
                nextx--;
                bytePtr--;
            }
            bytePtr -= parW;
            nexty--;
        }

        bytePtr = centroid + rightpx - px;

        nextx = rightpx;
        nexty = py;

        while (1) {
            if (*bytePtr == 0) {
                *x2 = nextx;
                *y2 = nexty - 1;
                break;
            }
            while (*(bytePtr + 1) == 0xff) {
                bytePtr++;
                nextx++;
            }
            bytePtr += parW;
            nexty++;
        }


    }

    /* Left Side */
    center = centroid - parW;
    nextx = leftpx;
    nexty = py;
    for (; nextx == leftpx; center -= parW, nexty--) {
        bytePtr = center;
        nextx = px + 1;
        while (*bytePtr == 0xff) {
            nextx--;
            bytePtr--;
        }
    }

    leftSlope = leftpx - nextx;
    bytePtr++;

    if (abs (leftSlope) > 4) {
        *y0 = nexty;
        *x0 = leftpx;

        bytePtr = centroid + leftpx - px;
        nexty = py;
        while (*(bytePtr + parW)) {
            bytePtr += parW;
            nexty++;
        }
        *x2 = leftpx;
        *y2 = nexty;
    } else if (leftSlope <= -1) {
        while (1) {
            if (*bytePtr == 0) {
                *x0 = nextx;
                *y0 = nexty + 1;
                break;
            }
            while (*(bytePtr - 1) == 0xff) {
                nextx--;
                bytePtr--;
            }
            bytePtr -= parW;
            nexty--;
        }

        bytePtr = centroid - (px - leftpx);

        nextx = leftpx;
        nexty = py;

        while (1) {
            if (*(bytePtr + 3) == 0) {
                *x3 = nextx;
                *y3 = nexty - 1;
                break;
            }
            while (*bytePtr == 0) {
                bytePtr++;
                nextx++;
            }
            bytePtr += parW;
            nexty++;
        }
    } else {
        while (1) {
            if (*(bytePtr + 3) == 0) {
                *x0 = nextx;
                *y0 = nexty + 1;
                break;
            }
            while (*bytePtr == 0) {
                bytePtr++;
                nextx++;
            }
            bytePtr -= parW;
            nexty--;

            bytePtr = centroid - (px - leftpx);

            nextx = leftpx;
            nexty = py;

            if (*bytePtr == 0) {
                *x3 = nextx;
                *y3 = nexty - 1;
                break;
            }
            while (*(bytePtr - 1) == 0xff) {
                nextx--;
                bytePtr--;
            }
            bytePtr += parW;
            nexty++;
        }
    }
}



void
ByteFindOuterCorners (buf, ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3, x0, y0, x1, y1, x2, y2, x3, y3)
    ByteImage *buf;
    int ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3;
    int *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3;
{
    int w, h, parW;
    unsigned char *firstBuf, *bytePtr;
    int px1, py1, px2, py2;

    w = buf->width;
    h = buf->height;
    parW = buf->parentWidth;
    firstBuf = buf->firstByte;


    /* upper left corner */
    /* Go left then up */
    bytePtr = firstBuf + iy0 * parW + ix0 - 2;
    px1 = ix0 - 1;
    py1 = iy0 + 1;
    while (*bytePtr == 0) {
        bytePtr--;
        px1--;
    }

    bytePtr++;
    while (*bytePtr == 0) {
        bytePtr -= parW;
        py1--;
    }

    /* Go up then left */
    bytePtr = firstBuf + (iy0 - 1) * parW + ix0;
    px2 = ix0 + 1;
    py2 = iy0;
    while (*bytePtr == 0) {
        bytePtr -= parW;
        py2--;
    }

    bytePtr += parW;
    while (*bytePtr == 0) {
        bytePtr--;
        px2--;
    }

    *x0 = min (px1, px2);
    *y0 = min (py1, py2);


    /* upper right corner */
    /* Go right then up */
    bytePtr = firstBuf + iy1 * parW + ix1 + 1;
    px1 = ix1;
    py1 = iy1 + 1;
    while (*bytePtr == 0) {
        bytePtr++;
        px1++;
    }

    bytePtr--;
    while (*bytePtr == 0) {
        bytePtr -= parW;
        py1--;
    }

    /* Go up then right */
    bytePtr = firstBuf + iy1 * parW + ix1 + 1;
    px2 = ix1;
    py2 = iy1 + 1;
    while (*bytePtr == 0) {
        bytePtr -= parW;
        py2--;
    }

    bytePtr += parW;
    while (*bytePtr == 0) {
        bytePtr++;
        px2++;
    }

    *x1 = max (px1, px2);
    *y1 = min (py1, py2);

    /* bottom left corner */
    /* Go left then down */
    bytePtr = firstBuf + iy3 * parW + ix3 - 1;
    px1 = ix3;
    py1 = iy3 - 1;
    while (*bytePtr == 0) {
        bytePtr--;
        px1--;
    }

    bytePtr++;
    while (*bytePtr == 0) {
        bytePtr += parW;
        py1++;
    }

    /* Go down then left */
    bytePtr = firstBuf + iy3 * parW + ix3 - 1;
    px2 = ix3;
    py2 = iy3 - 1;
    while (*bytePtr == 0) {
        bytePtr += parW;
        py2++;
    }

    bytePtr -= parW;
    while (*bytePtr == 0) {
        bytePtr--;
        px2--;
    }

    *x3 = min (px1, px2);
    *y3 = max (py1, py2);


    /* bottom right corner */
    /* Go right then down */
    bytePtr = firstBuf + iy2 * parW + ix2 + 1;
    px1 = ix2;
    py1 = iy2 - 1;
    while (*bytePtr == 0) {
        bytePtr++;
        px1++;
    }

    bytePtr--;
    while (*bytePtr == 0) {
        bytePtr += parW;
        py1++;
    }

    /* Go down then right */
    bytePtr = firstBuf + (iy2 + 1) * parW + ix2;
    px2 = ix2 - 1;
    py2 = iy2;
    while (*bytePtr == 0) {
        bytePtr += parW;
        py2++;
    }

    bytePtr -= parW;
    while (*bytePtr == 0) {
        bytePtr++;
        px2++;
    }

    *x2 = max (px1, px2);
    *y2 = max (py1, py2);

}


int
ByteFindBackgroundIntensity (buf)
    ByteImage *buf;
{
    int w, h, i, parW;
    unsigned char *firstBuf, *currBuf;
    unsigned char max;

    w = buf->width;
    h = buf->height;
    parW = buf->parentWidth;
    firstBuf = buf->firstByte;

    max = 0;

    /* Background intensity is assumed to the the brightest intensity */
    for (i = 0; i < h; i++) {
        currBuf = firstBuf;
        DO_N_TIMES (w,
                    if (*currBuf > max) max = *currBuf;
                    currBuf++;
            );
        firstBuf += parW;
    }

    return max;
}

int
ByteMakeFromBit8 (srcBuf, destBuf)
    BitImage *srcBuf;
    ByteImage *destBuf;
{
    int w, h, bw;
    unsigned char *currSrc, *currDest;
    int srcParW, destParW;
    unsigned char *firstSrc, *firstDest;
    unsigned char byte, vals[2];
    register int i;

    w = destBuf->width;
    h = srcBuf->height;
    bw = srcBuf->byteWidth;
    srcParW = srcBuf->parentWidth;
    destParW = destBuf->parentWidth;
    firstSrc = srcBuf->firstByte;
    firstDest = destBuf->firstByte;

    vals[0] = 0;
    vals[1] = 255;

    for (i = 0; i < h; i++) {
        currSrc = firstSrc;
        currDest = firstDest;

        DO_N_TIMES (bw,
                    byte = *currSrc;

                    *currDest++ = vals[BIT7 (byte)];
                    *currDest++ = vals[BIT6 (byte)];
                    *currDest++ = vals[BIT5 (byte)];
                    *currDest++ = vals[BIT4 (byte)];
                    *currDest++ = vals[BIT3 (byte)];
                    *currDest++ = vals[BIT2 (byte)];
                    *currDest++ = vals[BIT1 (byte)];
                    *currDest++ = vals[BIT0 (byte)];

                    currSrc++;
            );

        firstSrc += srcParW;
        firstDest += destParW;
    }

    return DVM_VISION_OK;
}
