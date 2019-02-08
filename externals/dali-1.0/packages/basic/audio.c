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
 * audio.c --
 *
 *        This file contains the tcl hook to the audio allocations
 *      (new, free) and basic manupulation routines (copy, clip).
 *      For I/O see audioio.c
 *
 *      Wei Tsang Oct 97
 *      Haye Chan Jan 98
 */

#include "basicInt.h"
#include "math.h"

Audio *
Audio8New(length)
    int length;
{
    Audio *new;

    new = NEW(Audio);

    new->length = length;
    new->start = 0;
    new->buffer = NEWARRAY(unsigned char, length);

    new->firstSample = new->buffer;
    new->isVirtual = 0;

    return new;
}

Audio *
Audio16New(length)
    int length;
{
    Audio *new;

    new = NEW(Audio);

    new->length = length;
    new->start = 0;
    new->buffer = (unsigned char *) NEWARRAY(short, length);

    new->firstSample = new->buffer;
    new->isVirtual = 0;

    return new;
}

void
AudioFree(audio)
    Audio *audio;
{
    if (!audio->isVirtual) {
        FREE((char *) audio->buffer);
    }
    FREE((char *) audio);
}

Audio *
Audio8Clip(audio, x, length)
    Audio *audio;
    int x;
    int length;
{
    Audio *new;

    new = NEW(Audio);

    new->length = length;
    new->start = audio->start + x;
    new->buffer = audio->buffer;
    new->firstSample = audio->firstSample + x;
    new->isVirtual = 1;

    return new;
}

Audio *
Audio16Clip(audio, x, length)
    Audio *audio;
    int x;
    int length;
{
    Audio *new;

    new = NEW(Audio);

    new->length = length;
    new->start = audio->start + x;
    new->buffer = audio->buffer;
    new->firstSample = (unsigned char *) (((short *) audio->firstSample) + x);
    new->isVirtual = 1;

    return new;
}

void
Audio8Reclip(audio, x, length, clipped)
    Audio *audio;
    int x;
    int length;
    Audio *clipped;
{
    clipped->length = length;
    clipped->start = audio->start + x;
    clipped->buffer = audio->buffer;
    clipped->firstSample = audio->firstSample + x;
    clipped->isVirtual = 1;
}

void
Audio16Reclip(audio, x, length, clipped)
    Audio *audio;
    int x;
    int length;
    Audio *clipped;
{
    clipped->length = length;
    clipped->start = audio->start + x;
    clipped->buffer = audio->buffer;
    clipped->firstSample = (unsigned char *) (((short *) audio->firstSample) + x);
    clipped->isVirtual = 1;
}

void
Audio8Set(audio, value)
    Audio *audio;
    unsigned char value;
{
    memset(audio->firstSample, value, audio->length);
}

void
Audio8SetSome(audio, value, offset, stride)
    Audio *audio;
    unsigned char value;
    int offset;
    int stride;
{
    unsigned char *currSample;
    int size;

    currSample = audio->firstSample + offset;

    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (audio->length - offset + stride - 1) / stride;
    DO_N_TIMES(size,
        *currSample = value;
        currSample += stride;
        );
}

void
Audio16SetSome(audio, value, offset, stride)
    Audio *audio;
    short value;
    int offset;
    int stride;
{
    short *currSample;
    int size;

    currSample = (short *) audio->firstSample;
    currSample += offset;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (audio->length - offset + stride - 1) / stride;
    DO_N_TIMES(size,
        *currSample = value;
        currSample += stride;
        );
}

void
Audio8Copy(src, dest)
    Audio *src;
    Audio *dest;
{
    memcpy(dest->firstSample, src->firstSample, src->length);
}

void
Audio8CopySome(src, dest, srcOffset, srcStride, destOffset, destStride)
    Audio *src;
    Audio *dest;
    int srcOffset;
    int srcStride;
    int destOffset;
    int destStride;
{
    int size;
    unsigned char *currSrc, *currDest;

    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (src->length - srcOffset + srcStride - 1) / srcStride;
    currSrc = src->firstSample + srcOffset;
    currDest = dest->firstSample + destOffset;
    DO_N_TIMES(size,
        *currDest = *currSrc;
        currSrc += srcStride;
        currDest += destStride;
        );
}

void
Audio16Copy(src, dest)
    Audio *src;
    Audio *dest;
{
    memcpy(dest->firstSample, src->firstSample,
        src->length * sizeof(short));
}

void
Audio16CopySome(src, dest, srcOffset, srcStride, destOffset, destStride)
    Audio *src;
    Audio *dest;
    int srcOffset;
    int srcStride;
    int destOffset;
    int destStride;
{
    int size;
    short *currSrc, *currDest;

    currSrc = (short *) src->firstSample;
    currSrc += srcOffset;
    currDest = (short *) dest->firstSample;
    currDest += destOffset;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (src->length - srcOffset + srcStride - 1) / srcStride;
    DO_N_TIMES(size,
        *currDest = *currSrc;
        currSrc += srcStride;
        currDest += destStride;
        );
}

void
Audio8Split(src, left, right)
    Audio *src;
    Audio *left;
    Audio *right;
{
    unsigned char *currSrc, *currLeft, *currRight;

    currSrc = src->firstSample;
    currLeft = left->firstSample;
    currRight = right->firstSample;
    DO_N_TIMES(src->length / 2,
        *(currLeft++) = *(currSrc++);
        *(currRight++) = *(currSrc++);
        );
}

void
Audio16Split(src, left, right)
    Audio *src;
    Audio *left;
    Audio *right;
{
    short *currSrc, *currLeft, *currRight;

    currSrc = (short *) src->firstSample;
    currLeft = (short *) left->firstSample;
    currRight = (short *) right->firstSample;
    DO_N_TIMES(src->length / 2,
        *(currLeft++) = *(currSrc++);
        *(currRight++) = *(currSrc++);
        );
}

void
Audio8Merge(left, right, dest)
    Audio *left;
    Audio *right;
    Audio *dest;
{
    int size;
    unsigned char *currDest, *currLeft, *currRight;

    size = min(left->length, right->length);
    currDest = dest->firstSample;
    currLeft = left->firstSample;
    currRight = right->firstSample;
    DO_N_TIMES(size,
        *(currDest++) = *(currLeft++);
        *(currDest++) = *(currRight++);
        );
}

void
Audio16Merge(left, right, dest)
    Audio *left;
    Audio *right;
    Audio *dest;
{
    int size;
    short *currDest, *currLeft, *currRight;

    size = min(left->length, right->length);
    currDest = (short *) dest->firstSample;
    currLeft = (short *) left->firstSample;
    currRight = (short *) right->firstSample;
    DO_N_TIMES(size,
        *(currDest++) = *(currLeft++);
        *(currDest++) = *(currRight++);
        );
}

/* offset, length - in number of samples */
Audio *
BitStreamCastToAudio8(bs, offset, length)
    BitStream *bs;
    int offset;
    int length;
{
    Audio *audio;

    audio = NEW(Audio);
    audio->start = offset;
    audio->length = length;
    audio->buffer = bs->buffer;
    audio->firstSample = bs->buffer + offset;
    audio->isVirtual = 1;

    return audio;
}

/* offset, length - in number of samples */
Audio *
BitStreamCastToAudio16(bs, offset, length)
    BitStream *bs;
    int offset;
    int length;
{
    Audio *audio;

    audio = NEW(Audio);
    audio->start = offset;
    audio->length = length;
    audio->buffer = bs->buffer;
    audio->firstSample = bs->buffer + offset * sizeof(short);

    audio->isVirtual = 1;

    return audio;
}

BitStream *
Audio8CastToBitStream(Audio * audio)
{
    BitStream *bs;

    bs = NEW(BitStream);
    bs->isVirtual = 1;
    bs->buffer = audio->firstSample;
    bs->size = audio->length;
    bs->endBufPtr = bs->endDataPtr = bs->buffer + audio->length;

    return bs;
}

BitStream *
Audio16CastToBitStream(Audio * audio)
{
    BitStream *bs;

    bs = NEW(BitStream);
    bs->isVirtual = 1;
    bs->buffer = audio->firstSample;
    bs->size = audio->length * sizeof(short);
    bs->endBufPtr = bs->endDataPtr = bs->buffer + audio->length * sizeof(short);

    return bs;
}

int
Audio8ChunkAbsSum(Audio * audio, int chunkSize, int **pSums)
{
    int numOfSums, i, currSum;
    unsigned char *currBuf;
    int *sums;

    numOfSums = audio->length / chunkSize;
    *pSums = sums = NEWARRAY(int, numOfSums);

    currBuf = audio->firstSample;
    for (i = 0; i < numOfSums; i++) {
        currSum = 0;
        DO_N_TIMES(chunkSize,
            currSum += (int) abs(*(currBuf++) - 128);
            );
        *(sums++) = currSum;
    }

    return numOfSums;
}

int
Audio16ChunkAbsSum(Audio * audio, int chunkSize, int **pSums)
{
    int numOfSums, i, currSum;
    short *currBuf;
    int *sums;

    numOfSums = audio->length / chunkSize;
    *pSums = sums = NEWARRAY(int, numOfSums);

    currBuf = (short *) audio->firstSample;
    for (i = 0; i < numOfSums; i++) {
        currSum = 0;
        DO_N_TIMES(chunkSize,
            currSum += (int) abs(*(currBuf++));
            );
        *(sums++) = currSum;
    }

    return numOfSums;
}

void
Audio8ResampleHalf(Audio * in, Audio * out)
{
    int len;
    char *inBuf, *outBuf;

    inBuf = (char *) in->firstSample;
    outBuf = (char *) out->firstSample;
    len = out->length = in->length / 2;
    DO_N_TIMES(len,
        *outBuf++ = (*inBuf++ + *inBuf++) >> 1;
        );
}

void
Audio16ResampleHalf(Audio * in, Audio * out)
{
    int len;
    short *inBuf, *outBuf;

    inBuf = (short *) in->firstSample;
    outBuf = (short *) out->firstSample;
    len = out->length = in->length / 2;
    DO_N_TIMES(len,
        *outBuf++ = (*inBuf++ + *inBuf++) >> 1;
        );
}

void
Audio8ResampleQuarter(Audio * in, Audio * out)
{
    int len;
    char *inBuf, *outBuf;

    inBuf = (char *) in->firstSample;
    outBuf = (char *) out->firstSample;
    len = out->length = in->length / 4;
    DO_N_TIMES(len,
        *outBuf++ = (*inBuf++ + *inBuf++ + *inBuf++ + *inBuf++) >> 2;
        );
}

void
Audio16ResampleQuarter(Audio * in, Audio * out)
{
    int len;
    short *inBuf, *outBuf;

    inBuf = (short *) in->firstSample;
    outBuf = (short *) out->firstSample;
    len = out->length = in->length / 4;
    DO_N_TIMES(len,
        *outBuf++ = (*inBuf++ + *inBuf++ + *inBuf++ + *inBuf++) >> 2;
        );
}

void
Audio8ResampleLinear(Audio * in, int inSamples, int outSamples, Audio * out)
{
    int i, x1;
    char *inBuf, *outBuf;
    float x, frac, delta;

    inBuf = (char *) in->firstSample;
    outBuf = (char *) out->firstSample;
    delta = ((float) inSamples) / outSamples;
    for (x = 0, i = 0; i < outSamples; i++) {
        x1 = (int) floor(x);
        frac = x - x1;
        *outBuf++ = (char) (inBuf[x1] * (1 - frac) + inBuf[x1 + 1] * frac);
        x += delta;
    }
}

void
Audio16ResampleLinear(Audio * in, int inSamples, int outSamples, Audio * out)
{
    int i, x1;
    short *inBuf, *outBuf;
    float x, frac, delta;

    inBuf = (short *) in->firstSample;
    outBuf = (short *) out->firstSample;
    delta = ((float) inSamples) / outSamples;
    for (x = 0, i = 0; i < outSamples; i++) {
        x1 = (int) floor(x);
        frac = x - x1;
        *outBuf++ = (short) (inBuf[x1] * (1 - frac) + inBuf[x1 + 1] * frac);
        x += delta;
    }
}

void
Audio8ResampleDecimate(Audio * in, int inSamples, int outSamples, Audio * out)
{
    int i, x1;
    char *inBuf, *outBuf;
    float x, delta;

    inBuf = (char *) in->firstSample;
    outBuf = (char *) out->firstSample;
    delta = ((float) inSamples) / outSamples;
    for (x = 0, i = 0; i < outSamples; i++) {
        x1 = (int) (x + 0.5);
        *outBuf++ = inBuf[x1];
        x += delta;
    }
}

void
Audio16ResampleDecimate(Audio * in, int inSamples, int outSamples, Audio * out)
{
    int i, x1;
    short *inBuf, *outBuf;
    float x, delta;

    inBuf = (short *) in->firstSample;
    outBuf = (short *) out->firstSample;
    delta = ((float) inSamples) / outSamples;
    for (x = 0, i = 0; i < outSamples; i++) {
        x1 = (int) (x + 0.5);
        *outBuf++ = inBuf[x1];
        x += delta;
    }
}

int
Audio8MaxAbs(Audio * in)
{
    char *buf;
    char maxVal, v;
    int len;

    buf = (char *) in->firstSample;
    len = in->length;
    maxVal = 0;
    DO_N_TIMES(len,
        v = *buf++;
        maxVal = max(maxVal, abs(v));
        );
    return maxVal;
}

int
Audio16MaxAbs(Audio * in)
{
    short *buf;
    short maxVal, v;
    int len;

    buf = (short *) in->firstSample;
    len = in->length;
    maxVal = 0;
    DO_N_TIMES(len,
        v = *buf++;
        maxVal = max(maxVal, abs(v));
        );
    return maxVal;
}
