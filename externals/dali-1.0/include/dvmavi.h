/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _DVMAVI_H_
#define _DVMAVI_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#undef WIN32_LEAN_AND_MEAN

#include "dvmbasic.h"
#include "dvmwave.h"

#define AVI_STREAM_VIDEO 1
#define AVI_STREAM_AUDIO 2

/* Errors for AviVideoFrameRead & AviVideoFrameWrite */
#define DVM_AVI_BAD_SIZE             1  /* Byte Image size does not match stream size */
#define DVM_AVI_NOT_VIDEO            2  /* Not a video stream */
#define DVM_AVI_NOT_AUDIO            22         /* Not an audio stream */

/* Errors for AviVideoFrameRead */
#define DVM_AVI_GET_FRAME_FAILED     3  /* Call to AVIStreamGetFrame() failed */

/* Errors for AviVideoFrameWrite */
#define DVM_AVI_UNINITIALIZED        4  /* Stream not initialized */
#define DVM_AVI_WRITE_FAILED         5  /* Call to AVIStreamWrite() failed */

/* Errors for AviFileOpen & AviCreateFile */
#define DVM_AVI_FILE_OPEN_FAILED     7  /* AVIFileOpen() failed */
#define DVM_AVI_FILE_INFO_FAILED     8  /* AVIFileInfo() failed */

/* Errors for AviStreamOpen */
#define DVM_AVI_BAD_STREAM_NUM      10  /* Requested to read non-existent stream */
#define DVM_AVI_GETSTREAM_FAILED    11  /* AVIFileGetStream() failed */
#define DVM_AVI_STREAM_INFO_FAILED  12  /* AVIStreamInfo() failed */
#define DVM_AVI_READ_FORMAT_FAILED  13  /* AVIStreamReadFormat() failed */
#define DVM_AVI_NOT_RGB             14  /* Video stream is not RGB */
#define DVM_AVI_UNSUPPORTED_TYPE    15  /* Stream type is not supported */

/* Errors for AviStreamStartDecode */
#define DVM_AVI_NO_DECOMPRESSOR     16  /* Decompressor not installed */


/***********************************************************************
 * The AVI data structures
 ***********************************************************************/

typedef struct AviFile {
    PAVIFILE aviHandle;         /* handle returned by AVIFileOpen */
    short numStreams;           /* The number of media streams */
    int length;                 /* Total time */
    int flags;                  /* VFW Specific flags */
} AviFile;

typedef struct AviStream {
    PAVISTREAM streamHandle;    /* handle returned by AVIGetStream */
    int type;                   /* either AVISTREAM_VIDEO or AVISTREAM_AUDIO */
    int codec;                  /* The fcc */
    int length;                 /* frames in case of video, blocks in audio */
    int start;                  /* sample number of first frame */
    int sofar;                  /* sample number of next frame to decompress */
    void *data;                 /* Format specific data (AviVideoStream or
                                   AviAudioStream) */
} AviStream;

typedef struct AviVideoStream {
    PGETFRAME gf;               /* getframe handle for decompression */
    PAVISTREAM cs;              /* handle to compressed stream (for writes) */
    unsigned char *fb;          /* framebuffer (for writes) */
    short width;                /* width of each frame */
    short height;               /* height of each frame */
    short fps;                  /* frames per second .. */
    short keyinterval;          /* Interval between key frames */
} AviVideoStream;

typedef struct AviAudioStream {
    short channels;             /* number of channels */
    int rate;                   /* samples per second */
    short bps;                  /* bits per sample */
    PAVISTREAM cs;              /* handle to compressed stream (for writes) */
    WAVEFORMATEX afmt;          /* audio format */
    int afmtSize;               /* size of the afmt structure */
} AviAudioStream;

typedef struct AviCodec {
    int id;                     /* The unique ID of this codec (four characters, packed) */
    short video;                /* 1 if codec is a video codec, 0 if it's an audio codec */
    short version;              /* The version number of the codec */
    char name[16];              /* The name of the codec */
    char description[128];      /* A description of the codec */
} AviCodec;


typedef struct AudioConv {
    int format;                 /* The format tag to be encoded to */
    int rate;                   /* samples per second */
    int outBitrate;             /* output bitrate */
    int bps;                    /* bits per sample */
    int nc;                     /* number of channels */
    HACMDRIVER had;             /* ACM driver handle */
    HACMSTREAM hstr;            /* ACM conversion stream handle */
    LPWAVEFORMATEX pwfDrv;      /* The driver format */
    LPWAVEFORMATEX pwfPCM;      /* The PCM format */
} AudioConv;

/* Macros for data access */
#define AviStreamGetType(as) (as)->type
#define AviStreamGetCodec(as) (as)->codec
#define AviStreamGetLength(as) (as)->length
#define AviStreamGetWidth(as) (((VideoStream*)((as)->data))->width)
#define AviStreamGetHeight(as) (((VideoStream*)((as)->data))->height)
#define AviStreamGetFps(as) (((VideoStream*)((as)->data))->fps)


#endif

int AviGetNumOfCodecs();
int AviCodecInfo(AviCodec * codecPtr, int codecNumber);
char *AviTranslateError(int errorCode);

/* avifile.c */
int AviFileOpen(char *filename, AviFile ** aviFilePtr);
void AviFileClose(AviFile * aviFile);
int AviFileCreate(char *filename, AviFile ** aviFilePtr);

/* avistream.c */
int AviStreamOpen(AviFile * aviFile, int streamnum, AviStream ** strPtr);
void AviStreamClose(AviStream * str);
int AviStreamStartDecode(AviStream * str);
void AviStreamStopDecode(AviStream * str);
int AviVideoStreamCreate(AviFile * aviFile, int codec, int w, int h, int fps,
    int keyinterval, int quality, int bitrate, AviStream ** strPtr);
int AviAudioStreamCreate(AviFile * aviFile, AviStream ** strPtr, short nc, short bps, int rate);

/* avivideo.c */
int AviVideoFrameRead(AviStream * str, ByteImage * rBuf, ByteImage * gBuf, ByteImage * bBuf);
int AviVideoFrameWrite(AviStream * str, ByteImage * rBuf, ByteImage * gBuf, ByteImage * bBuf);
int AviAudioFrameWrite(AviStream * str, Audio * audio, int *sampwritten);

int AviVideoFrameSkip(AviStream * str);
int AviVideoFrameRewind(AviStream * str);
int AviVideoFrameTell(AviStream * str);
int AviVideoFrameSeek(AviStream * str, int frameNumber);

/* audioconv.c */
AudioConv *AudioConvNew(int tag, int rate, short bps, short nc);
void AudioConvFree(AudioConv * ac);
int AudioConvEncode(AudioConv * ac, Audio * in, BitParser * out, int *numused);
WaveHdr *WaveHdrNewFromAudioConv(AudioConv * ac);
