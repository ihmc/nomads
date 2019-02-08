/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _DVM_MPEG_H_
#define _DVM_MPEG_H_

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main C library interface header file for MPEG package.
 * It includes MPEG Video, Audio and System stream.
 *
 * Wei Tsang Dec 97
 *---------------------------------------------------------------------
 */

    typedef short Block[64];
    typedef short LumBlock[256];

    /*
       * Constant definitions
     */
#define I_FRAME 1
#define P_FRAME 2
#define B_FRAME 3
#define D_FRAME 4

#define MOTION_FORWARD      1
#define MOTION_BACKWARD     2
#define MOTION_INTERPOLATE  3

/*
 * Various start code
 */
#define SEQ_START_CODE 0x000001b3
#define SEQ_END_CODE   0x000001b7
#define ISO_11172_END_CODE 0x000001b9
#define PACK_START_CODE 0x000001ba
#define SYS_START_CODE 0x000001bb
#define PIC_START_CODE 0x00000100
#define GOP_START_CODE 0x000001b8
#define EXT_START_CODE 0x000001b5
#define USER_START_CODE 0x000001b2
#define SLICE_MIN_START_CODE 0x00000101
#define SLICE_MAX_START_CODE 0x000001af
#define PACKET_MIN_START_CODE 0x0000001bc
#define PACKET_MAX_START_CODE 0x0000001f0

/*
 * Video pel_aspect_ratio values
 */
#define PEL_ASPECT_RATIO_10000 1
#define PEL_ASPECT_RATIO_06735 2
#define PEL_ASPECT_RATIO_07031 3
#define PEL_ASPECT_RATIO_07615 4
#define PEL_ASPECT_RATIO_08055 5
#define PEL_ASPECT_RATIO_08437 6
#define PEL_ASPECT_RATIO_08935 7
#define PEL_ASPECT_RATIO_09375 8
#define PEL_ASPECT_RATIO_09815 9
#define PEL_ASPECT_RATIO_10255 10
#define PEL_ASPECT_RATIO_10695 11
#define PEL_ASPECT_RATIO_11250 12
#define PEL_ASPECT_RATIO_11575 13
#define PEL_ASPECT_RATIO_12015 14

/*
 * Video picture_rate values
 */
#define PICTURE_RATE_23_976 1
#define PICTURE_RATE_24     2
#define PICTURE_RATE_25     3
#define PICTURE_RATE_29_97  4
#define PICTURE_RATE_30     5
#define PICTURE_RATE_50     6
#define PICTURE_RATE_59_94  7
#define PICTURE_RATE_60     8

/*
 * Packet's stream id
 */
#define DVM_STREAM_ID_STD_AUDIO 0xB8
#define DVM_STREAM_ID_STD_VIDEO 0xB9
#define DVM_STREAM_ID_RESERVED  0xBC
#define DVM_STREAM_ID_PRIVATE_1 0xBD
#define DVM_STREAM_ID_PRIVATE_2 0xBF
#define DVM_STREAM_ID_PADDING   0xBE

/*
 * Special DCT values
 */
#define EOB     62
#define ESCAPE  61
#define DCT_ERROR 63
#define END_OF_BITSTREAM (-1)

/*
 * Mpeg Audio constant
 */

#define MPEG_AUDIO_LAYER_1 3
#define MPEG_AUDIO_LAYER_2 2
#define MPEG_AUDIO_LAYER_3 1
#define MPEG_AUDIO_STEREO 0
#define MPEG_AUDIO_JOINT_STEREO 1
#define MPEG_AUDIO_DUAL_CHANNEL 2
#define MPEG_AUDIO_SINGLE_CHANNEL 3

/*
 * Return Code
 */

#define DVM_MPEG_OK 0
#define DVM_MPEG_NOT_FOUND -1
#define DVM_MPEG_INVALID_START_CODE -2
#define DVM_MPEG_INDEX_FULL 1

/*
 *----------------------------------------------------------------------
 *
 * type MpegSeqHdr
 *
 *     The MpegSeqHdr structure contains all the parameters
 *     contained in the sequence header of a mpeg stream
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegSeqHdr {
        short width;            /* width of each frame */
        short height;           /* height of each frame */
        short mb_width;         /* width of each frame in macroblocks */
        short mb_height;        /* height of each frame in macroblocks */
        int bitrate;            /* In units of 400 bits/second */
        int vbv_buffer_size;    /* Decoding verifier buffer size */
        char pel_aspect_ratio;  /* Index to aspect ratio table */
        char picture_rate;      /* Index to picture rate table */
        char constrained;       /* Boolean - true if a constrained param mpeg */
        char default_qt;        /* 0 - both, 1 - iqt, 2 - niqt, 3 - none */
        unsigned char iqt[64];  /* Intra quantization table - 64 elements */
        unsigned char niqt[64]; /* Non intra quantization table */
    } MpegSeqHdr;

#define MpegSeqHdrGetWidth(hdr) (hdr)->width
#define MpegSeqHdrGetHeight(hdr) (hdr)->height
#define MpegSeqHdrGetBitRate(hdr) (hdr)->bitrate
#define MpegSeqHdrGetBufferSize(hdr) (hdr)->vbv_buffer_size
#define MpegSeqHdrGetIQT(hdr) (hdr)->iqt
#define MpegSeqHdrGetNIQT(hdr) (hdr)->niqt

#define MpegSeqHdrSetWidth(hdr, x) {(hdr)->width = x; (hdr)->mb_width = ((x) + 15)/16;}
#define MpegSeqHdrSetHeight(hdr, x) {(hdr)->height = x; (hdr)->mb_height = ((x) + 15)/16;}
//#define MpegSeqHdrSetAspectRatio(hdr, x) (hdr)->pel_aspect_ratio = x
    //#define MpegSeqHdrSetPicRate(hdr, x) (hdr)->picture_rate = x;
#define MpegSeqHdrSetBitRate(hdr, x) (hdr)->bitrate = x
#define MpegSeqHdrSetBufferSize(hdr, x) (hdr)->vbv_buffer_size = x
#define MpegSeqHdrSetConstrained(hdr, x) (hdr)->constrained = x

/*
 *----------------------------------------------------------------------
 *
 * type MpegGopHdr
 *
 *     The MpegGopHdr structure contains all the parameters
 *     contained in the GOP header of a mpeg stream
 *
 *----------------------------------------------------------------------
 */
    typedef struct MpegGopHdr {
        char drop_frame_flag;
        char time_code_hours;
        char time_code_minutes;
        char time_code_seconds;
        char time_code_pictures;
        char closed_gop;        /* closed_gop gop? */
        char broken_link;       /* See standard */
    } MpegGopHdr;

#define MpegGopHdrGetClosedGop(hdr) (hdr)->closed_gop
#define MpegGopHdrGetBrokenLink(hdr) (hdr)->broken_link

#define MpegGopHdrSetDropFrameFlag(hdr, x) (hdr)->drop_frame_flag = x
#define MpegGopHdrSetHours(hdr, x) (hdr)->time_code_hours = x
#define MpegGopHdrSetMinutes(hdr, x) (hdr)->time_code_minutes = x
#define MpegGopHdrSetSeconds(hdr, x) (hdr)->time_code_seconds = x
#define MpegGopHdrSetPictures(hdr, x) (hdr)->time_code_pictures = x
#define MpegGopHdrSetClosedGop(hdr, x) (hdr)->closed_gop = x
#define MpegGopHdrSetBrokenLink(hdr, x) (hdr)->broken_link = x

/*
 *----------------------------------------------------------------------
 *
 * type MpegPicHdr
 *
 *     The MpegPicHdr structure contains all the parameters
 *     contained in the picture header of a mpeg stream
 *
 *----------------------------------------------------------------------
 */
    typedef struct MpegPicHdr {
        short temporal_reference;       /* frame number modulo 1024 */
        char type;              /* I, P, B or D frame */
        unsigned short vbv_delay;       /* see standard */
        char full_pel_forward_vector;   /* for P/B frames : 1 iff mv is int pels */
        char forward_r_size, forward_f;         /* for P/B frames : valid value 1-7. */
        char full_pel_backward_vector;  /* for B frames : 1 iff mv is int pels */
        char backward_r_size, backward_f;       /* for B frames : valid value 1-7. */
    } MpegPicHdr;

#define MpegPicHdrGetTemporalRef(hdr) (hdr)->temporal_reference
#define MpegPicHdrGetType(hdr) (hdr)->type

#define MpegPicHdrSetTemporalRef(hdr, x) (hdr)->temporal_reference = x
#define MpegPicHdrSetType(hdr, x) (hdr)->type = x
#define MpegPicHdrSetVBVDelay(hdr, x) (hdr)->vbv_delay = x
#define MpegPicHdrSetFullPelForward(hdr, x) (hdr)->full_pel_forward_vector = x
#define MpegPicHdrSetFullPelBackward(hdr, x) (hdr)->full_pel_backward_vector = x

/*
 *----------------------------------------------------------------------
 *
 * type MpegPktHdr
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegPktHdr {
        int streamId;
        int packetLength;
        int bufferSize;
        double pts;
        double dts;
    } MpegPktHdr;

#define MpegPktHdrGetLength(hdr) (hdr)->packetLength
#define MpegPktHdrGetStreamId(hdr) (hdr)->streamId
#define MpegPktHdrGetBufferSize(hdr) (hdr)->bufferSize
#define MpegPktHdrGetPts(hdr) (hdr)->pts
#define MpegPktHdrGetDts(hdr) (hdr)->dts

#define MpegPktHdrSetLength(hdr, x) (hdr)->packetLength = (x)
#define MpegPktHdrSetStreamId(hdr, x) (hdr)->streamId = (x)
#define MpegPktHdrSetBufferSize(hdr, x) (hdr)->bufferSize = (x)
#define MpegPktHdrSetPts(hdr, x) (hdr)->pts = (x)
#define MpegPktHdrSetDts(hdr, x) (hdr)->dts = (x)

/*
 *----------------------------------------------------------------------
 *
 * type MpegPckHdr
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegPckHdr {
        double sysClockRef;     /* system clock reference */
        int muxRate;            /* multiplex rate in bytes/second */
    } MpegPckHdr;

#define MpegPckHdrGetSysClockRef(hdr) (hdr)->sysClockRef
#define MpegPckHdrGetMuxRate(hdr) (hdr)->muxRate

#define MpegPckHdrSetSysClockRef(hdr, x) (hdr)->sysClockRef = (x)
#define MpegPckHdrSetMuxRate(hdr, x) (hdr)->muxRate = (x)

/*
 *----------------------------------------------------------------------
 *
 * type MpegSysHdr
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegSysHdr {
        int rateBound;          /* read the standard for details */
        unsigned char audioBound;       /* >= max number of simul active audio */
        unsigned char fixedFlag;        /* 1 iff fixed bitrate */
        unsigned char cspsFlag; /* 1 iff is contrained sys parameter stream */
        unsigned char audioLock;        /* read standard */
        unsigned char videoLock;        /* read standrad */
        unsigned char videoBound;       /* >= max number of simul active video */
        unsigned char numOfStreamInfo;  /* number of entry in streamId array */
        unsigned char streamId[48];
        int bufferSize[48];     /* decoded (scaled) buffer size */
    } MpegSysHdr;

#define MpegSysHdrGetRateBound(hdr)  (hdr)->rateBound
#define MpegSysHdrGetAudioBound(hdr) (hdr)->audioBound
#define MpegSysHdrGetVideoBound(hdr) (hdr)->videoBound
#define MpegSysHdrGetFixedFlag(hdr)  (hdr)->fixedFlag
#define MpegSysHdrGetCspsFlag(hdr)   (hdr)->cspsFlag
#define MpegSysHdrGetAudioLock(hdr)  (hdr)->audioLock
#define MpegSysHdrGetVideoLock(hdr)  (hdr)->videoLock
#define MpegSysHdrGetNumOfStreamInfo(hdr)  (hdr)->numOfStreamInfo

#define MpegSysHdrSetRateBound(hdr, x)  (hdr)->rateBound = (x)
#define MpegSysHdrSetAudioBound(hdr, x) (hdr)->audioBound = (x)
#define MpegSysHdrSetVideoBound(hdr, x) (hdr)->videoBound = (x)
#define MpegSysHdrSetFixedFlag(hdr, x)  (hdr)->fixedFlag = (x)
#define MpegSysHdrSetCspsFlag(hdr, x)   (hdr)->cspsFlag = (x)
#define MpegSysHdrSetAudioLock(hdr, x)  (hdr)->audioLock = (x)
#define MpegSysHdrSetVideoLock(hdr, x)  (hdr)->videoLock = (x)
#define MpegSysHdrSetNumOfStreamInfo(hdr, x)  (hdr)->numOfStreamInfo = (x)


    typedef struct StreamInfo {
        int numOfPacket;
        int max;
        BitStreamFilter *index;
        float *time;
    } StreamInfo;

    typedef struct MpegSysToc {
        int numOfVideoStreams;
        int numOfAudioStreams;
        StreamInfo *streamInfo[48];
    } MpegSysToc;

/*
 *----------------------------------------------------------------------
 *
 * type MpegAudioHdr
 *
 *     The MpegAudioHdr structure contains all the important fields
 *     in a MPEG audio hdr.
 *
 *----------------------------------------------------------------------
 */
    typedef struct MpegAudioHdr {
        char id;
        char layer;
        char bit_rate_index;
        float bit_rate;
        char protection_bit;
        char sampling_rate_index;
        float sampling_rate;
        char padding_bit;
        char extension;
        char mode;
        char mode_extension;
        char copyright;
        char original_or_copy;
        char emphasis_index;
        short error_check;
    } MpegAudioHdr;

#define MpegAudioHdrGetLayer(h) h->layer
#define MpegAudioHdrGetBitRate(h) h->bit_rate
#define MpegAudioHdrGetSamplingRate(h) h->sampling_rate
#define MpegAudioHdrGetMode(h) h->mode

/*
 *----------------------------------------------------------------------
 *
 * type MpegAudioLayerX
 *
 *     The MpegAudioLayerX (X = 1, 2, 3) stores the undecoded audio
 *     data from mpeg audio streams.
 *
 *----------------------------------------------------------------------
 */
    typedef struct MpegAudioL1 {
        char allocation[32];
        char scalefactor[32];
        unsigned int sample[12][32];
    } MpegAudioL1;


    typedef struct MpegAudioL2 {
        int sblimit;
        char allocation[32];
        char scfsi[32];
        char scalefactor[3][32];
        unsigned int sample[12][3][32];
    } MpegAudioL2;

    typedef struct MpegAudioL3 {
        int num_of_slots;
        unsigned short main_data_end;
        unsigned char private_bits;
        unsigned char scfsi[4][2];
        unsigned short part2_3_length[2][2];
        unsigned short big_values[2][2];
        unsigned char global_gain[2][2];
        unsigned char scalefac_compress[2][2];
        unsigned char block_split_flag[2][2];
        unsigned char block_type[2][2];
        unsigned char switch_point[2][2];
        unsigned char table_select[3][2][2];
        unsigned char subblock_gain[3][2][2];
        unsigned char region_address1[2][2];
        unsigned char region_address2[2][2];
        unsigned char preflag[2][2];
        unsigned char scalefac_scale[2][2];
        unsigned char countltable_table[2][2];
        char scale_fac_s[2][2][13][3];
        char scale_fac_l[2][2][23];
        short zero_freq_start[2][2];
        int freq_lines[2][2][576];
    } MpegAudioL3;


/*
 *----------------------------------------------------------------------
 *
 * type MpegAudioSynData, MpegAudioGraData, MpegAudioBuffer
 *
 *     An auxillary data structure that contains information from
 *     previous decoding, needs to be used in the decoding of next
 *     audio frame.
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegAudioSynData {
        int offset;
        double syn[1024];
    } MpegAudioSynData;

    typedef double MpegAudioGraData[2][1024];


/*
 *----------------------------------------------------------------------
 *
 * type MpegVideoIndexElement
 *
 *     A data structure that contains information about frame types
 *     (i, p, or b), offset, length of each frame, and frame number
 *     in a specific video
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegVideoIndexElement {
        char refCount;
        char type;
        char pastOffset;
        char forOffset;
        int offset;
        int length;

    } MpegVideoIndexElement;

#define MpegVideoIndexGetType(index, fnum) index->table[fnum].type
#define MpegVideoIndexGetNext(index, fnum) index->table[fnum].forOffset
#define MpegVideoIndexGetPast(index, fnum) index->table[fnum].pastOffset
#define MpegVideoIndexGetOffset(index, fnum) index->table[fnum].offset
#define MpegVideoIndexGetLength(index, fnum) index->table[fnum].length
#define MpegVideoIndexNumRefs(index, fnum) index->table[fnum].refCount

/*
 *----------------------------------------------------------------------
 *
 * type MpegVideoIndex
 *
 *    A data structure that contains information about
 *    MpegVideoIndexElements, the number of elements, and the maximum
 *    possible number of elements.
 *
 *----------------------------------------------------------------------
 */

    typedef struct MpegVideoIndex {
        MpegVideoIndexElement *table;
        int numElements;
        int maxElements;
    } MpegVideoIndex;


/*
 *----------------------------------------------------------------------
 *
 * Function Prototypes
 *
 *----------------------------------------------------------------------
 */

/*
 * MpegSeqHdr
 */
    MpegSeqHdr *MpegSeqHdrNew();
    void MpegSeqHdrFree(MpegSeqHdr * hdr);

    int MpegSeqHdrFind(BitParser *);
    int MpegSeqHdrDump(BitParser *, BitParser *);
    int MpegSeqHdrSkip(BitParser *);
    int MpegSeqHdrParse(BitParser *, MpegSeqHdr *);
    int MpegSeqHdrEncode(MpegSeqHdr *, BitParser *);

    double MpegSeqHdrGetPicRate(MpegSeqHdr * hdr);
    double MpegSeqHdrGetAspectRatio(MpegSeqHdr * hdr);

    void MpegSeqHdrSetAspectRatio(MpegSeqHdr * hdr, double aspectRatio);
    void MpegSeqHdrSetPicRate(MpegSeqHdr * hdr, double picRate);
    void MpegSeqHdrSetIQT(MpegSeqHdr * hdr, int *qTable);
    void MpegSeqHdrSetNIQT(MpegSeqHdr * hdr, int *qTable);
    void MpegSeqHdrSetDefaultIQT(MpegSeqHdr * hdr);
    void MpegSeqHdrSetDefaultNIQT(MpegSeqHdr * hdr);

    void MpegSeqEnder(BitParser * bp);
    void MpegSeqHdrSet(MpegSeqHdr *, short, short, char, char, int, int, char, char, char, unsigned char *, unsigned char *);

/*
 * MpegGopHdr
 */
    MpegGopHdr *MpegGopHdrNew();
    void MpegGopHdrFree(MpegGopHdr * hdr);
    int MpegGopHdrFind(BitParser * bp);
    int MpegGopHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegGopHdrSkip(BitParser * bp);
    int MpegGopHdrParse(BitParser * bp, MpegGopHdr *);
    int MpegGopHdrEncode(MpegGopHdr *, BitParser *);

    void MpegGopHdrSet(MpegGopHdr *, char, char, char, char, char, char, char);

/*
 * MpegPicHdr
 */
    MpegPicHdr *MpegPicHdrNew();
    void MpegPicHdrFree(MpegPicHdr * hdr);
    int MpegPicHdrFind(BitParser * bp);
    int MpegPicHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegPicHdrSkip(BitParser * bp);
    int MpegPicHdrParse(BitParser * bp, MpegPicHdr * hdr);
    int MpegPicHdrEncode(MpegPicHdr *, BitParser *);

    void MpegPicHdrSetForwardFCode(MpegPicHdr * hdr, int forwardFCode);
    void MpegPicHdrSetBackwardFCode(MpegPicHdr * hdr, int backwardFCode);
    void MpegPicHdrSet(MpegPicHdr *, short, char, unsigned short, char, char, char, char);


/*
 * MpegPic
 */
    int MpegPicDump(BitParser * inbp, BitParser * outbp);
    int MpegPicSkip(BitParser * inbp);
    int MpegPicIParse(BitParser * bp, MpegSeqHdr *, MpegPicHdr *, ScImage *, ScImage *, ScImage *);
    int MpegPicPParse(BitParser * bp, MpegSeqHdr *, MpegPicHdr *, ScImage *, ScImage *, ScImage *, VectorImage *);
    int MpegPicBParse(BitParser * bp, MpegSeqHdr *, MpegPicHdr *, ScImage *, ScImage *, ScImage *, VectorImage *, VectorImage *);

    void MpegPicIEncode(MpegPicHdr *, ScImage *, ScImage *, ScImage *, ByteImage *, int *, int, BitParser * bp);
    void MpegPicPEncode(MpegPicHdr *, ScImage *, ScImage *, ScImage *, VectorImage *, ByteImage *, int *, int, BitParser * bp);
    void MpegPicBEncode(MpegPicHdr *, ScImage *, ScImage *, ScImage *, VectorImage *, VectorImage *, ByteImage *, int *, int, BitParser * bp);

    int MpegAnyHdrFind(BitParser * bp);
    int MpegGetCurrStartCode(BitParser * bp);

/*
 * MotionSearch
 */
    void BytePMotionVecSearch(MpegPicHdr *, ByteImage *, ByteImage *, ByteImage **, VectorImage *);
    void ByteBMotionVecSearch(MpegPicHdr *, ByteImage *, ByteImage *, ByteImage *, ByteImage **, ByteImage **,
        int *, int, VectorImage *, VectorImage *);

    long PLogarithmicSearch(LumBlock, ByteImage *, ByteImage **, int, int, int, int, int, int, int *, int *);
    long PMotionSearch(LumBlock, ByteImage *, ByteImage **, int, int, int, int, int, int, int *, int *);
    int BMotionSearch(LumBlock, ByteImage *, ByteImage *, ByteImage **, ByteImage **, int, int, int, int, int, int, int, int, int *, int *, int *, int *, int);
    void ByteComputeIntermediates(ByteImage *, ByteImage **);
    ByteImage **ByteNewIntermediates(ByteImage *);

/*
 * Block
 */
    void ComputeMotionBlock(ByteImage *, int, int, int, int, Block);
    void ComputeBMotionBlock(ByteImage *, ByteImage *, int, int, int, int, int, int, int, Block);
    void ComputeMotionLumBlock(ByteImage *, ByteImage **, int, int, int, int, LumBlock);
    void ComputeBMotionLumBlock(ByteImage *, ByteImage *, ByteImage **, ByteImage **, int, int,
        int, int, int, int, int, LumBlock);
    long LumBlockMAD(LumBlock, LumBlock, long);
    long LumMotionError(LumBlock, ByteImage *, ByteImage **, int, int, int, int, long);

/*
 * HuffEncode
 */
    void EncodeYDC(BitParser * bp, short, short *);
    void EncodeCDC(BitParser * bp, short, short *);
    void IBlockHuffEncode(BitParser * bp, ScBlock *);
    void NonIBlockHuffEncode(BitParser * bp, ScBlock *);


/*
 * MpegSliceHdr
 */
    void MpegSliceHdrEncode(BitParser * bp, int, int, unsigned char *, unsigned long);

/*
 * MpegMacroBlockHdr
 */
    void MpegMacroBlockHdrEncode(BitParser * bp, MpegPicHdr *, int, int, int, int, char, char, int, int, int, int);
    void MpegIMacroBlockHdrEncode(BitParser * bp, MpegPicHdr *, int, int);

/*
 * MpegPktHdr
 */
    MpegPktHdr *MpegPktHdrNew();
    void MpegPktHdrFree(MpegPktHdr * hdr);

    int MpegPktHdrFind(BitParser * bp);
    int MpegPktHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegPktHdrSkip(BitParser * bp);
    int MpegPktHdrParse(BitParser * bp, MpegPktHdr * hdr);
    int MpegPktHdrEncode(MpegPktHdr *, int, BitParser *);

/*
 * MpegPckHdr
 */
    MpegPckHdr *MpegPckHdrNew();
    void MpegPckHdrFree(MpegPckHdr * hdr);

    int MpegPckHdrFind(BitParser * bp);
    int MpegPckHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegPckHdrSkip(BitParser * bp);
    int MpegPckHdrParse(BitParser * bp, MpegPckHdr * hdr);
    int MpegPckHdrEncode(MpegPckHdr *, BitParser *);

/*
 * MpegSysHdr
 */
    MpegSysHdr *MpegSysHdrNew();
    void MpegSysHdrFree(MpegSysHdr * hdr);

    int MpegSysHdrFind(BitParser * bp);
    int MpegSysHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegSysHdrSkip(BitParser * bp);
    int MpegSysHdrParse(BitParser * bp, MpegSysHdr * hdr);
    int MpegSysHdrEncode(MpegSysHdr *, BitParser *);
    void MpegSysHdrSetBufferSize(MpegSysHdr *, int id, int bufSize);
    int MpegSysHdrGetBufferSize(MpegSysHdr *, int id);



/*
 * MpegAudioHdr
 */
    MpegAudioHdr *MpegAudioHdrNew();
    void MpegAudioHdrFree(MpegAudioHdr *);

    int MpegAudioHdrFind(BitParser * bp);
    int MpegAudioHdrDump(BitParser * inbp, BitParser * outbp);
    int MpegAudioHdrSkip(BitParser * bp);
    int MpegAudioHdrParse(BitParser * bp, MpegAudioHdr * hdr);
    int MpegAudioHdrEncode(MpegAudioHdr * hdr, BitParser * bp);

/*
 * MpegAudioL1
 */
    MpegAudioL1 *MpegAudioL1New();
    void MpegAudioL1Free(MpegAudioL1 *);

    void MpegAudioL1MonoParse(BitParser *, MpegAudioHdr *, MpegAudioL1 *);
    void MpegAudioL1MonoEncode(MpegAudioL1 *, MpegAudioHdr *, BitParser *);
    void MpegAudioL1StereoParse(BitParser *, MpegAudioHdr *, MpegAudioL1 *, MpegAudioL1 *);
    void MpegAudioL1StereoEncode(MpegAudioL1 *, MpegAudioL1 *, MpegAudioHdr *, BitParser *);
    void MpegAudioL1ToAudio(MpegAudioHdr *, MpegAudioL1 *, MpegAudioSynData *, Audio *);

/*
 * MpegAudioL2
 */
    MpegAudioL2 *MpegAudioL2New();
    void MpegAudioL2Free(MpegAudioL2 *);
    void MpegAudioL2MonoParse(BitParser *, MpegAudioHdr *, MpegAudioL2 *);
    void MpegAudioL2MonoEncode(MpegAudioL2 *, MpegAudioHdr *, BitParser *);
    void MpegAudioL2StereoParse(BitParser *, MpegAudioHdr *, MpegAudioL2 *, MpegAudioL2 *);
    void MpegAudioL2StereoEncode(MpegAudioL2 *, MpegAudioL2 *, MpegAudioHdr *, BitParser *);
    void MpegAudioL2ToAudio(MpegAudioHdr *, MpegAudioL2 *, MpegAudioSynData *, Audio *);
    double MpegAudioL2ScaleFactorSum(MpegAudioHdr *, MpegAudioL2 *);

/*
 * MpegAudioL3
 */
    MpegAudioL3 *MpegAudioL3New();
    void MpegAudioL3Free(MpegAudioL3 * data);
    void MpegAudioL3Parse(BitParser *, BitParser *, MpegAudioHdr *, MpegAudioL3 *);
    void MpegAudioL3MonoToAudio(MpegAudioHdr *, MpegAudioL3 *, MpegAudioSynData *, MpegAudioGraData *, Audio *);
    void MpegAudioL3StereoToAudio(MpegAudioHdr *, MpegAudioL3 *, MpegAudioSynData *, MpegAudioSynData *, MpegAudioGraData *, Audio *, Audio *);

/*
 * MpegAudio Auxillary Data
 */
    MpegAudioSynData *MpegAudioSynDataNew();
    MpegAudioGraData *MpegAudioGraDataNew();
    void MpegAudioSynDataFree(MpegAudioSynData *);
    void MpegAudioGraDataFree(MpegAudioGraData *);

    void MpegSeqEndCodeEncode(BitParser *);

    MpegSysToc *MpegSysTocNew();
    void MpegSysTocFree(MpegSysToc *);
    void MpegSysTocAdd(BitParser * bp, MpegSysToc * toc, int offset);
    int MpegSysTocGetOffset(MpegSysToc * toc, int id, double time);
    BitStreamFilter *MpegSysTocGetFilter(MpegSysToc * toc, int id);
    int MpegSysTocWrite(MpegSysToc *, char *fileName);
    int MpegSysTocRead(MpegSysToc *, char *fileName);

/*
 * MpegVideoIndex
 */
    MpegVideoIndex *MpegVideoIndexNew(int size);
    void MpegVideoIndexFree(MpegVideoIndex * index);
    void MpegVideoIndexParse(BitParser * bp, MpegVideoIndex * index);
    void MpegVideoIndexEncode(MpegVideoIndex * index, BitParser * bp);
    int MpegVideoIndexFindRefs(MpegVideoIndex * index, MpegVideoIndex * result, int frameNum);
    int MpegVideoIndexTableAdd(MpegVideoIndex * index, int frameNum, int bitOffset, char ftype, int flength, int past, int next);
    void MpegVideoIndexResize(MpegVideoIndex * index, unsigned int newSize);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
