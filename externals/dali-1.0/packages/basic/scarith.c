#include "basicInt.h"

void
ScAdd (src1, src2, dest)
    ScImage *src1;
    ScImage *src2;
    ScImage *dest;
{
    int i, j, w, h;
    int destCoeff, src1Coeff, src2Coeff;
    ScBlock *currSrc1, *currSrc2, *currDest;

    w = min(src1->width, src2->width);
    w = min(dest->width, w);
    h = min(src1->height, src2->height);
    h = min(dest->height, h);

    currSrc1 = src1->firstBlock;
    currSrc2 = src2->firstBlock;
    currDest = dest->firstBlock;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            currDest->intracoded = currSrc1->intracoded;
            currDest->skipMB = currSrc1->skipMB;
            currDest->skipBlock = currSrc1->skipBlock;
            currDest->dc = currSrc1->dc + currSrc2->dc;
            destCoeff = 0;
            src1Coeff = 0;
            src2Coeff = 0;
            while ((src1Coeff < currSrc1->numOfAC && src2Coeff < currSrc2->numOfAC)) {
                if (currSrc1->index[src1Coeff] < currSrc2->index[src2Coeff]) {
                    currDest->index[destCoeff] = currSrc1->index[src1Coeff];
                    currDest->value[destCoeff] = currSrc1->value[src1Coeff];
                    src1Coeff++;
                } else if (currSrc1->index[src1Coeff] > currSrc2->index[src2Coeff]) {
                    currDest->index[destCoeff] = currSrc2->index[src2Coeff];
                    currDest->value[destCoeff] = currSrc2->value[src2Coeff];
                    src2Coeff++;
                } else {
                    currDest->index[destCoeff] = currSrc2->index[src2Coeff];
                    currDest->value[destCoeff] = currSrc2->value[src2Coeff] +
                        currSrc1->value[src1Coeff];
                    src1Coeff++;
                    src2Coeff++;
                }
                destCoeff++;
            }
            while (src1Coeff < currSrc1->numOfAC) {
                currDest->index[destCoeff] = currSrc1->index[src1Coeff];
                currDest->value[destCoeff] = currSrc1->value[src1Coeff];
                src1Coeff++;
                destCoeff++;
            }
            while (src2Coeff < currSrc2->numOfAC) {
                currDest->index[destCoeff] = currSrc2->index[src2Coeff];
                currDest->value[destCoeff] = currSrc2->value[src2Coeff];
                src2Coeff++;
                destCoeff++;
            }

            currDest->numOfAC = destCoeff;
            currDest++;
            currSrc1++;
            currSrc2++;
        }
        currDest += dest->parentWidth - w;
        currSrc1 += src1->parentWidth - w;
        currSrc2 += src2->parentWidth - w;
    }
}


void
ScMultiply (src, k, dest)
    ScImage *src;
    double k;
    ScImage *dest;
{
    int i, j, w, h;
    int destCoeff, srcCoeff;
    ScBlock *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstBlock;
    currDest = dest->firstBlock;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            currDest->intracoded = currSrc->intracoded;
            currDest->skipMB = currSrc->skipMB;
            currDest->skipBlock = currSrc->skipBlock;
            destCoeff = 0;
            srcCoeff = 0;
            currDest->dc = (short)(currSrc->dc*k);
            DO_N_TIMES(currSrc->numOfAC,
                currDest->value[destCoeff] = (int)(currSrc->value[srcCoeff]*k);
                if (currDest->value[destCoeff]) {
                    currDest->index[destCoeff] = currSrc->index[srcCoeff];
                    destCoeff++;
                }
                srcCoeff++;
                );
            currDest->numOfAC = destCoeff;
            currDest++;
            currSrc++;
        }
        currDest += dest->parentWidth - w;
        currSrc += src->parentWidth - w;
    }
}
