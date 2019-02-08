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
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "mpegInt.h"

#define COMPUTE_MOTION_BOUNDARY(by, bx, height, width, stepSize, leftMY, leftMX, rightMY, rightMX)\
    leftMY = -2*DCTSIZE*by;     /* these are valid motion vectors */         \
    leftMX = -2*DCTSIZE*bx;                                                  \
                                /* these are invalid motion vectors */       \
    rightMY = 2*(height - (by+2)*DCTSIZE + 1) - 1;                           \
    rightMX = 2*(width  - (bx+2)*DCTSIZE + 1) - 1;                           \
                                                                             \
    if ( stepSize == 2 ) {                                                   \
        rightMY++;                                                           \
        rightMX++;                                                           \
    }

#define VALID_MOTION(y,x)       \
    (((y) >= leftMY) && ((y) < rightMY) &&   \
     ((x) >= leftMX) && ((x) < rightMX) ) \


/*===========================================================================*
 *
 * PLogarithmicSearch
 *
 *      uses logarithmic search to compute the P-frame vector
 *
 * RETURNS:     motion vector
 *
 * SIDE EFFECTS:    none
 *
 * REFERENCE:  MPEG-I specification, pages 32-33
 *
 *===========================================================================*/
long
PLogarithmicSearch(currentBlock, prev, intermediates, stepSize, searchRange, by, bx, height, width, motionY, motionX)
    LumBlock currentBlock;
    ByteImage *prev;
    ByteImage **intermediates;
    int stepSize, searchRange;
    int by, bx;
    int height, width;
    int *motionY, *motionX;
{
    register int mx, my;
    long diff, bestDiff;
    int leftMY, leftMX;
    int rightMY, rightMX;
    int tempRightMY, tempRightMX;
    int spacing;
    int centerX, centerY;
    int newCenterX, newCenterY;

    COMPUTE_MOTION_BOUNDARY(by, bx, height, width, stepSize, leftMY, leftMX, rightMY, rightMX);
    bestDiff = 0x7fffffff;

    /* grid spacing */
    if (stepSize == 2) {        /* make sure spacing is even */
        spacing = (searchRange + 1) / 2;
        if ((spacing % 2) != 0) {
            spacing--;
        }
    } else {
        spacing = (searchRange + 1) / 2;
    }
    centerX = 0;
    centerY = 0;

    while (spacing >= stepSize) {
        newCenterY = centerY;
        newCenterX = centerX;

        tempRightMY = rightMY;
        if (centerY + spacing + 1 < tempRightMY) {
            tempRightMY = centerY + spacing + 1;
        }
        tempRightMX = rightMX;
        if (centerX + spacing + 1 < tempRightMX) {
            tempRightMX = centerX + spacing + 1;
        }
        for (my = centerY - spacing; my < tempRightMY; my += spacing) {
            if (my < leftMY) {
                continue;
            }
            for (mx = centerX - spacing; mx < tempRightMX; mx += spacing) {
                if (mx < leftMX) {
                    continue;
                }
                diff = LumMotionError(currentBlock, prev, intermediates, by, bx, my, mx, bestDiff);

                if (diff < bestDiff) {
                    newCenterY = my;
                    newCenterX = mx;

                    bestDiff = diff;
                }
            }
        }

        centerY = newCenterY;
        centerX = newCenterX;

        if (stepSize == 2) {    /* make sure spacing is even */
            if (spacing == 2) {
                spacing = 0;
            } else {
                spacing = (spacing + 1) / 2;
                if ((spacing % 2) != 0) {
                    spacing--;
                }
            }
        } else {
            if (spacing == 1) {
                spacing = 0;
            } else {
                spacing = (spacing + 1) / 2;
            }
        }
    }
    /* check old motion -- see if it's better */
    if ((*motionY >= leftMY) && (*motionY < rightMY) && (*motionX >= leftMX) && (*motionX < rightMX)) {
        diff = LumMotionError(currentBlock, prev, intermediates, by, bx, *motionY, *motionX, bestDiff);
    } else {
        diff = 0x7fffffff;
    }

    if (bestDiff < diff) {
        *motionY = centerY;
        *motionX = centerX;
    } else {
        bestDiff = diff;
    }


    return bestDiff;
}

long
PMotionSearch(currentBlock, prev, intermediates, stepSize, searchRange, by, bx, height, width, motionY, motionX)
    LumBlock currentBlock;
    ByteImage *prev;
    ByteImage **intermediates;
    int stepSize, searchRange;
    int by, bx;
    int height, width;
    int *motionY, *motionX;
{
    /* CALL SEARCH PROCEDURE */
    return PLogarithmicSearch(currentBlock, prev, intermediates, stepSize, searchRange, by, bx, height, width, motionY, motionX);
    /*
     * switch(psearchAlg) {
     * case PSEARCH_SUBSAMPLE:
     * PSubSampleSearch(currentBlock, prev, by, bx, motionY, motionX);
     * break;
     * case PSEARCH_EXHAUSTIVE:
     * PLocalSearch(currentBlock, prev, by, bx, motionY, motionX,
     * 0x7fffffff);
     * break;
     * case PSEARCH_LOGARITHMIC:
     * PLogarithmicSearch(currentBlock, prev, by, bx, motionY, motionX);
     * break;
     * case PSEARCH_TWOLEVEL:
     * PTwoLevelSearch(currentBlock, prev, by, bx, motionY, motionX,
     * 0x7fffffff);
     * break;
     * default:
     * fprintf(stderr, "ILLEGAL PSEARCH ALG:  %d\n", psearchAlg);
     * exit(1);
     * } */
}
