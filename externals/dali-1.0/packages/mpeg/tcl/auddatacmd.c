/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmMpeg.h"


int 
MpegAudioL1NewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL1 *audio;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    audio = MpegAudioL1New();
    PutMpegAudioL1(interp, audio);

    return TCL_OK;
}

int 
MpegAudioL1FreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL1 *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audioData", argv[0]);

    audio = RemoveMpegAudioL1(argv[1]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[1]);
    MpegAudioL1Free(audio);

    return TCL_OK;
}


int 
MpegAudioL1MonoParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL1 *audio;
    BitParser *bp;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s bitParser audioHdr audioData", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    audio = GetMpegAudioL1(argv[3]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[3]);

    MpegAudioL1MonoParse(bp, hdr, audio);

    return TCL_OK;
}


int 
MpegAudioL1MonoEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL1 *audio;
    BitParser *bp;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s audioData audioHdr bitParser", argv[0]);

    audio = GetMpegAudioL1(argv[1]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    bp = GetBitParser(argv[3]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[3]);

    MpegAudioL1MonoEncode(audio, hdr, bp);

    return TCL_OK;
}


int 
MpegAudioL1StereoParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL1 *left;
    MpegAudioL1 *right;
    BitParser *bp;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s bitParser audioHdr leftAudioData rightAudioData", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    left = GetMpegAudioL1(argv[3]);
    ReturnErrorIf2(left == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[3]);
    right = GetMpegAudioL1(argv[4]);
    ReturnErrorIf2(right == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[4]);

    MpegAudioL1StereoParse(bp, hdr, left, right);

    return TCL_OK;
}


int 
MpegAudioL1StereoEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL1 *left;
    MpegAudioL1 *right;
    BitParser *bp;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s leftAudioData rightAudioData audioHdr bitParser", argv[0]);

    left = GetMpegAudioL1(argv[1]);
    ReturnErrorIf2(left == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[1]);
    right = GetMpegAudioL1(argv[2]);
    ReturnErrorIf2(right == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[2]);
    hdr = GetMpegAudioHdr(argv[3]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[3]);
    bp = GetBitParser(argv[4]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[4]);

    MpegAudioL1StereoEncode(left, right, hdr, bp);

    return TCL_OK;
}


int 
MpegAudioL2NewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL2 *audio;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    audio = MpegAudioL2New();
    PutMpegAudioL2(interp, audio);

    return TCL_OK;
}


int 
MpegAudioL2FreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL2 *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audioData", argv[0]);

    audio = RemoveMpegAudioL2(argv[1]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[1]);
    MpegAudioL2Free(audio);

    return TCL_OK;
}


int 
MpegAudioL2MonoParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *audio;
    BitParser *bp;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s bitParser audioHdr audioData", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    audio = GetMpegAudioL2(argv[3]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[3]);

    MpegAudioL2MonoParse(bp, hdr, audio);

    return TCL_OK;
}


int 
MpegAudioL2StereoParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *left;
    MpegAudioL2 *right;
    BitParser *bp;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s bitParser audioHdr leftL2Audio rightL2Audio", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    left = GetMpegAudioL2(argv[3]);
    ReturnErrorIf2(left == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[3]);
    right = GetMpegAudioL2(argv[4]);
    ReturnErrorIf2(right == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[4]);

    MpegAudioL2StereoParse(bp, hdr, left, right);

    return TCL_OK;
}


int 
MpegAudioL2MonoEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *audio;
    BitParser *bp;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s audioData audioHdr bitParser", argv[0]);

    audio = GetMpegAudioL2(argv[1]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[1]);
    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);
    bp = GetBitParser(argv[3]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[3]);

    MpegAudioL2MonoEncode(audio, hdr, bp);

    return TCL_OK;
}


int 
MpegAudioL2StereoEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *left;
    MpegAudioL2 *right;
    BitParser *bp;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s leftL2Audio rightL2Audio audioHdr bitParser", argv[0]);

    left = GetMpegAudioL2(argv[1]);
    ReturnErrorIf2(left == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[1]);
    right = GetMpegAudioL2(argv[2]);
    ReturnErrorIf2(right == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[2]);
    hdr = GetMpegAudioHdr(argv[3]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[3]);
    bp = GetBitParser(argv[4]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[4]);

    MpegAudioL2StereoEncode(left, right, hdr, bp);

    return TCL_OK;
}


int 
MpegAudioL3NewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL3 *audio;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    audio = MpegAudioL3New();
    PutMpegAudioL3(interp, audio);

    return TCL_OK;
}

int 
MpegAudioL3FreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioL3 *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audioData", argv[0]);

    audio = RemoveMpegAudioL3(argv[1]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 3 audio data %s", argv[0], argv[1]);
    MpegAudioL3Free(audio);

    return TCL_OK;

}


int 
MpegAudioL3ParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL3 *audio;
    BitParser *bp, *abp;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s bitParser auxillaryBp audioHdr audioData", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    abp = GetBitParser(argv[2]);
    ReturnErrorIf2(abp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);
    hdr = GetMpegAudioHdr(argv[3]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[3]);
    audio = GetMpegAudioL3(argv[4]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 3 audio data %s", argv[0], argv[4]);

    MpegAudioL3Parse(bp, abp, hdr, audio);
    return TCL_OK;
}


int 
MpegAudioL1ToAudioCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL1 *audio;
    Audio *lpcm;
    MpegAudioSynData *lv;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audioHdr l1Audio synData audio", argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    audio = GetMpegAudioL1(argv[2]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 1 audio data %s", argv[0], argv[2]);

    lv = GetMpegAudioSynData(argv[3]);
    ReturnErrorIf2(lv == NULL,
        "%s: no such audio syn data %s", argv[0], argv[3]);

    lpcm = GetAudio(argv[4]);
    ReturnErrorIf2(lpcm == NULL,
        "%s: no such pcm data %s", argv[0], argv[4]);

    MpegAudioL1ToAudio(hdr, audio, lv, lpcm);
    return TCL_OK;
}

int 
MpegAudioL2ToAudioCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *audio;
    Audio *pcm;
    MpegAudioSynData *v;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audioHdr L2Audio synData audio", argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    audio = GetMpegAudioL2(argv[2]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[2]);

    v = GetMpegAudioSynData(argv[3]);
    ReturnErrorIf2(v == NULL,
        "%s: no such audio syn data %s", argv[0], argv[3]);

    pcm = GetAudio(argv[4]);
    ReturnErrorIf2(pcm == NULL,
        "%s: no such pcm data %s", argv[0], argv[4]);

    MpegAudioL2ToAudio(hdr, audio, v, pcm);
    return TCL_OK;
}


int 
MpegAudioL3MonoToAudioCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL3 *audio;
    Audio *lpcm;
    MpegAudioSynData *lv;
    MpegAudioGraData *prev_granule;

    ReturnErrorIf1(argc != 6,
        "wrong # args: should be %s audioHdr audio synData prevGranule pcm",
        argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    audio = GetMpegAudioL3(argv[2]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 3 audio data %s", argv[0], argv[2]);

    lv = GetMpegAudioSynData(argv[3]);
    ReturnErrorIf2(lv == NULL,
        "%s: no such audio syn data %s", argv[0], argv[3]);

    prev_granule = GetMpegAudioSynData(argv[4]);
    ReturnErrorIf2(prev_granule == NULL,
        "%s: no such audio syn data %s", argv[0], argv[3]);

    lpcm = GetAudio(argv[5]);
    ReturnErrorIf2(lpcm == NULL,
        "%s: no such pcm data %s", argv[0], argv[5]);

    MpegAudioL3MonoToAudio(hdr, audio, lv, prev_granule, lpcm);
    return TCL_OK;
}


int 
MpegAudioL3StereoToAudioCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL3 *audio;
    Audio *lpcm, *rpcm;
    MpegAudioSynData *lv, *rv;
    MpegAudioGraData *prev_granule;

    ReturnErrorIf1(argc != 8,
        "wrong # args: should be %s audioHdr audio leftSynData rightSynData prevGranule leftPcm rightPcm", argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    audio = GetMpegAudioL3(argv[2]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 3 audio data %s", argv[0], argv[2]);

    lv = GetMpegAudioSynData(argv[3]);
    ReturnErrorIf2(lv == NULL,
        "%s: no such audio syn data %s", argv[0], argv[3]);

    rv = GetMpegAudioSynData(argv[4]);
    ReturnErrorIf2(rv == NULL,
        "%s: no such audio syn data %s", argv[0], argv[4]);

    prev_granule = GetMpegAudioSynData(argv[5]);
    ReturnErrorIf2(prev_granule == NULL,
        "%s: no such audio syn data %s", argv[0], argv[5]);

    lpcm = GetAudio(argv[6]);
    ReturnErrorIf2(lpcm == NULL,
        "%s: no such pcm data %s", argv[0], argv[6]);

    rpcm = GetAudio(argv[7]);
    ReturnErrorIf2(rpcm == NULL,
        "%s: no such pcm data %s", argv[0], argv[7]);

    MpegAudioL3StereoToAudio(hdr, audio, lv, rv, prev_granule, lpcm, rpcm);
    return TCL_OK;
}

int 
MpegAudioL2ScaleFactorSumCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    MpegAudioL2 *audio;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s audioHdr l2Audio", argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    audio = GetMpegAudioL2(argv[2]);
    ReturnErrorIf2(audio == NULL,
        "%s: no such MPEG layer 2 audio data %s", argv[0], argv[2]);

    sprintf(interp->result, "%f", MpegAudioL2ScaleFactorSum(hdr, audio));

    return TCL_OK;
}
