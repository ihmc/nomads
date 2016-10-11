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
 * audiocmd.c --
 *
 *        This file contains the tcl hook to the Audio C routines
 *
 */

#include "tclDvmBasic.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_new <len>
 * return 
 *     the buffer number
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *     the buffer has length <len> bytes
 *
 *----------------------------------------------------------------------
 */
int
Audio8NewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int length, status;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s numOfSamples", argv[0]);
    status = Tcl_GetInt(interp, argv[1], &length);
    ReturnErrorIf(status != TCL_OK);

    audio = Audio8New(length);
    PutAudio(interp, audio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_new <len>
 * return 
 *     the buffer number
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *     the buffer has length <len>*2 bytes
 *
 *----------------------------------------------------------------------
 */
int
Audio16NewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int length, status;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s numOfSamples", argv[0]);
    status = Tcl_GetInt(interp, argv[1], &length);
    ReturnErrorIf(status != TCL_OK);

    audio = Audio16New(length);
    PutAudio(interp, audio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_clip <audio> <x> <len>
 * purpose 
 *     creates virtual audio buffer, which is a part of a physical audio
 *     buffer
 * precond
 *     <x>+<len> must be smaller than the length of parent buffer
 *     <audio> should be 8-bit audio buffer
 * return 
 *     the buffer number
 * side effect :
 *     a new virtual buffer is created and is added into _theBufferTable_
 *     the buffer has length <len> bytes
 *
 *----------------------------------------------------------------------
 */
int
Audio8ClipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    Audio *new;
    int x, length;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s audio offset numOfSamples", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    new = Audio8Clip(audio, x, length);
    PutAudio(interp, new);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_clip <audio> <x> <len>
 *
 * creates virtual audio buffer, which is a part of a physical audio
 * buffer
 *
 * precond
 *     <x>+<len> must be smaller than the length of parent buffer
 *     <audio> should be 16-bit audio buffer
 *     
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new virtual buffer is created and is added into _theBufferTable_
 *     the buffer has length <len>*2 bytes
 *
 *----------------------------------------------------------------------
 */
int
Audio16ClipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    Audio *new;
    int x, length;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s audio offset numOfSamples", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    new = Audio16Clip(audio, x, length);
    PutAudio(interp, new);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_reclip <audio> <x> <len> <clipped>
 *
 * Reinitialized a virtual audio buffer, just as we just clip it using
 * audio_8_clip
 *
 * precond
 *     <x>+<len> must be smaller than the length of parent buffer
 *     <audio> should be 8-bit audio buffer
 *     
 * return 
 *     the buffer number
 * 
 * side effect :
 *     _clipped_ is initialized.
 *
 *----------------------------------------------------------------------
 */
int
Audio8ReclipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    Audio *clippedAudio;
    int x, length;
    int status;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audio offset numOfSamples clippedAudio",
        argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    clippedAudio = GetAudio(argv[4]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[4]);

    Audio8Reclip(audio, x, length, clippedAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_reclip <audio> <x> <len> <clipped>
 *
 * Reinitialized a virtual audio buffer, just as we just clip it using
 * audio_8_clip
 *
 * precond
 *     <x>+<len> must be smaller than the length of parent buffer
 *     <audio> should be 8-bit audio buffer
 *     
 * return 
 *     the buffer number
 * 
 * side effect :
 *     _clipped_ is initialized.
 *
 *----------------------------------------------------------------------
 */
int
Audio16ReclipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    Audio *clippedAudio;
    int x, length;
    int status;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audio offset numOfSamples clippedAudio",
        argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    clippedAudio = GetAudio(argv[4]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[4]);

    Audio16Reclip(audio, x, length, clippedAudio);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_set <audio> <value>
 *
 * precond 
 *     <audio> exists
 *     <value> between 0 and 255
 *
 * return 
 *     None
 *
 * side effect :
 *     Content of the whole audio buffer <audio> is set to all <value>
 *
 *----------------------------------------------------------------------
 */
int
Audio8SetCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int status;
    int value;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s audio value", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);

    Audio8Set(audio, (unsigned char) value);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_set_some <audio> <value> <offset> <stride>
 *
 * precond 
 *     <audio> exists
 *     <value> between 0 and 255
 *
 * return 
 *     None
 *
 * side effect :
 *     Content of audio buffer <audio> is set to all <value> in some
 *     ways:
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
Audio8SetSomeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int status;
    int value, offset, stride;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audio value offset stride", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &offset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &stride);
    ReturnErrorIf(status != TCL_OK);

    Audio8SetSome(audio, (unsigned char) value, offset, stride);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_set_some <audio> <value> <offset> <stride>
 *
 * precond 
 *     <audio> exists
 *     <value> between -32768 and 32767
 *
 * return 
 *     None
 *
 * side effect :
 *     Content of audio buffer <audio> is set to all <value> in some
 *     ways:
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
Audio16SetSomeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int status;
    int value, offset, stride;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s audio value offset stride", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &offset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &stride);
    ReturnErrorIf(status != TCL_OK);

    Audio16SetSome(audio, (short) value, offset, stride);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_copy <src> <dest>
 *
 * precond
 *     both buffer <src> and <dest> exists, and cannot overlap
 *     <src> and <dest> should be 8-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     exact contents in <src> would be copied to <dest>
 *
 *----------------------------------------------------------------------
 */
int
Audio8CopyCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s src dest", argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    dest = GetAudio(argv[2]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[2]);

    Audio8Copy(src, dest);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_copy_some <src> <dest> <src  offset> <src  stride>
 *                                    <dest offset> <dest stride>
 *
 * precond
 *     both buffer <src> and <dest> exists, and cannot overlap
 *     <src> and <dest> should be 8-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     contents in <src> would be copied to <dest> in some ways
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
Audio8CopySomeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *dest;
    int srcOffset, srcStride, destOffset, destStride;
    int status;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s src dest srcOffset srcStride destOffset destStride", argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    dest = GetAudio(argv[2]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);

    Audio8CopySome(src, dest, srcOffset, srcStride, destOffset, destStride);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_copy <src> <dest>
 *
 * precond
 *     both buffer <src> and <dest> exists, and cannot overlap
 *     <src> and <dest> should be 16-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     exact contents in <src> would be copied to <dest>
 *
 *----------------------------------------------------------------------
 */
int
Audio16CopyCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s src dest", argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    dest = GetAudio(argv[2]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[2]);

    Audio16Copy(src, dest);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_copy_some <src> <dest> <src  offset> <src  stride>
 *                                     <dest offset> <dest stride>
 *
 * precond
 *     both buffer <src> and <dest> exists, and cannot overlap
 *     <src> and <dest> should be 16-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     contents in <src> would be copied to <dest> in some ways
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
Audio16CopySomeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *dest;
    int srcOffset, srcStride, destOffset, destStride;
    int status;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s src dest srcOffset srcStride destOffset destStride", argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    dest = GetAudio(argv[2]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);

    Audio16CopySome(src, dest, srcOffset, srcStride, destOffset, destStride);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_split <stereo src> <mono left> <mono right>
 *
 * precond
 *     memory should be allocated for <mono left> and <mono right> 
 *     already
 *     all of them should be 8-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     contents in <stereo src> would separated into left and right
 *     channel, storing in <mono left> and <mono right> respectively
 *
 *----------------------------------------------------------------------
 */
int
Audio8SplitCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *left, *right;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s stereoAudio leftAudio rightAudio",
        argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    left = GetAudio(argv[2]);
    ReturnErrorIf1(left == NULL,
        "no such audio %s\n", argv[2]);
    right = GetAudio(argv[3]);
    ReturnErrorIf1(right == NULL,
        "no such audio %s\n", argv[3]);
    ReturnErrorIf1(min(left->length, right->length) < (src->length / 2),
        "%s : destination buffer not long enough", argv[0]);
    Audio8Split(src, left, right);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_split <stereo src> <mono left> <mono right>
 *
 * precond
 *     memory should be allocated for <mono left> and <mono right> 
 *     already
 *     all of them should be 16-bit audio buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     contents in <stereo src> would separated into left and right
 *     channel, storing in <mono left> and <mono right> respectively
 *
 *----------------------------------------------------------------------
 */
int
Audio16SplitCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *src, *left, *right;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s stereoAudio leftAudio rightAudio", argv[0]);

    src = GetAudio(argv[1]);
    ReturnErrorIf1(src == NULL,
        "no such audio %s\n", argv[1]);
    left = GetAudio(argv[2]);
    ReturnErrorIf1(left == NULL,
        "no such audio %s\n", argv[2]);
    right = GetAudio(argv[3]);
    ReturnErrorIf1(right == NULL,
        "no such audio %s\n", argv[3]);

    Audio16Split(src, left, right);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_merge <stereo dest> <mono left> <mono right>
 * precond
 *     memory should be allocated for <stereo dest> already
 *     all of them should be 8-bit audio buffer
 * return 
 *     none
 * side effect :
 *     the audio data in <mono left> and <mono right> would be merged
 *     into stereo audio <stereo dest> as left and right channels
 *
 *----------------------------------------------------------------------
 */
int
Audio8MergeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *dest, *left, *right;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s leftAudio rightAudio stereoAudio",
        argv[0]);

    left = GetAudio(argv[1]);
    ReturnErrorIf1(left == NULL,
        "no such audio %s\n", argv[1]);

    right = GetAudio(argv[2]);
    ReturnErrorIf1(right == NULL,
        "no such audio %s\n", argv[2]);

    dest = GetAudio(argv[3]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[3]);

    Audio8Merge(left, right, dest);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_merge <stereo dest> <mono left> <mono right>
 * precond
 *     memory should be allocated for <stereo dest> already
 *     all of them should be 16-bit audio buffer
 * return 
 *     none
 * side effect :
 *     the audio data in <mono left> and <mono right> would be merged
 *     into stereo audio <stereo dest> as left and right channels
 *
 *----------------------------------------------------------------------
 */
int
Audio16MergeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *dest, *left, *right;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s leftAudio rightAudio stereoAudio",
        argv[0]);

    left = GetAudio(argv[1]);
    ReturnErrorIf1(left == NULL,
        "no such audio %s\n", argv[1]);

    right = GetAudio(argv[2]);
    ReturnErrorIf1(right == NULL,
        "no such audio %s\n", argv[2]);

    dest = GetAudio(argv[3]);
    ReturnErrorIf1(dest == NULL,
        "no such audio %s\n", argv[3]);

    Audio16Merge(left, right, dest);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_free <audio>
 * precond
 *     <audio> exists
 * side effect :
 *     remove <audio> from _theHashTable_
 *     if <audio> is not virtual, free the memory allocated for <audio>
 *     all virtual buffer refering to this becomes invalid and must be
 *     freed as well
 *
 *----------------------------------------------------------------------
 */
int
AudioFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audio", argv[0]);

    audio = RemoveAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    AudioFree(audio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_get_start <audio>
 * precond 
 *     <audio> exists
 * return 
 *     the field start in <audio>
 * side effect
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioGetStartOffsetCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audio", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    sprintf(interp->result, "%d", audio->start);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_get_length <audio>
 *
 * precond 
 *     <audio> exists
 *
 * return 
 *     the length of <audio>
 *
 * side effect
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioGetNumOfSamplesCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audio", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    sprintf(interp->result, "%d", audio->length);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bitstream_cast_to_audio_8 <bitstream> <offset> <length>
 *
 * precond 
 *     bitstream must be present
 *     the data in the bitstream is assumed to be 8-bit audio
 *
 * return
 *     handle to an virtual audio buffer casted from <bitstream>
 *          with offset = <offset> and <length> samples
 *
 * side effect :
 *     the whole data buffer in the bitstream is casted to <audio>
 *     a new virtual audio buffer will be allocated
 *
 *----------------------------------------------------------------------
 */
int 
BitStreamCastToAudio8Cmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Audio *audio;
    int status, offset, length;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s bitstream offset numOfSamples", argv[0]);
    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &offset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    audio = BitStreamCastToAudio8(bs, offset, length);
    PutAudio(interp, audio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bitstream_cast_to_audio_16 <bitstream> <audio> <offset> <length>
 *
 * precond 
 *     bitstream must be present
 *     the data in the bitstream is assumed to be 16-bit audio
 *
 * return
 *     handle to an virtual audio buffer casted from <bitstream>
 *          with offset = <offset> and <length> samples
 *
 * side effect :
 *     the whole data buffer in the bitstream is casted to <audio>
 *     a new virtual audio buffer will be allocated
 *
 *----------------------------------------------------------------------
 */
int 
BitStreamCastToAudio16Cmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Audio *audio;
    int status, offset, length;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s bitstream offset numOfSamples", argv[0]);

    bs = GetBitStream(argv[1]);
    ReturnErrorIf2(bs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &offset);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &length);
    ReturnErrorIf(status != TCL_OK);

    audio = BitStreamCastToAudio16(bs, offset, length);
    PutAudio(interp, audio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_cast_to_bitstream <audio>
 * precond 
 *     <audio> must be a 8-bit audio
 * side effect :
 *     memory is allocated for a new bitstream
 *     the whole <audio> buffer is casted directly to the bitstream
 *     i.e., the bitstream will consist of data equivalent to <audio>
 *           in terms of address also
 *     and the handle to that bitstream is returned
 *
 *----------------------------------------------------------------------
 */
int 
Audio8CastToBitStreamCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Audio *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audio", argv[0]);
    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    bs = Audio8CastToBitStream(audio);
    PutBitStream(interp, bs);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_cast_to_bitstream <audio>
 * precond 
 *     <audio> must be a 16-bit audio
 * side effect :
 *     memory is allocated for a new bitstream
 *     the whole <audio> buffer is casted directly to the bitstream
 *     i.e., the bitstream will consist of data equivalent to <audio>
 *           in terms of address also
 *     and the handle to that bitstream is returned
 *
 *----------------------------------------------------------------------
 */
int 
Audio16CastToBitStreamCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    Audio *audio;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s audio", argv[0]);
    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);

    bs = Audio16CastToBitStream(audio);
    PutBitStream(interp, bs);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_chunk_abs_sum <audio> <chunksize>
 * precond 
 *     <audio> must be a 8-bit audio
 * return
 *     list of integers, representing the sums of each chunk
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8ChunkAbsSumCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int chunkSize, numOfSums;
    int *sums, *currSums;
    int status;
    char strSum[20];
    int i;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s audio chunksize", argv[0]);
    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &chunkSize);
    ReturnErrorIf(status != TCL_OK);

    numOfSums = Audio8ChunkAbsSum(audio, chunkSize, &sums);
    /*
     * parse the results in the array into Tcl lists for return
     */
    currSums = sums;
    for (i = 0; i < numOfSums; i++) {
        sprintf(strSum, "%d", *(currSums++));
        Tcl_AppendElement(interp, strSum);
    }

    FREE(sums);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_16_chunk_abs_sum <audio> <chunksize>
 * precond 
 *     <audio> must be a 16-bit audio
 * return
 *     list of integers, representing the sums of each chunk
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio16ChunkAbsSumCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int chunkSize, numOfSums;
    int *sums, *currSums;
    int status;
    char strSum[20];

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s audio chunksize", argv[0]);
    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &chunkSize);
    ReturnErrorIf(status != TCL_OK);

    numOfSums = Audio16ChunkAbsSum(audio, chunkSize, &sums);

    /*
     * parse the results in the array into Tcl lists for return
     */
    currSums = sums;
    DO_N_TIMES(numOfSums,
        sprintf(strSum, "%d", *(currSums++));
        Tcl_AppendElement(interp, strSum);
        );

    FREE(sums);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_resample_half <in> <out>
 *     audio_16_resample_half <in> <out>
 * precond 
 *     <in> and <out> must be 8 or 16-bit audio buffers
 * return
 *     none
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8ResampleHalfCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s in out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    out = GetAudio(argv[2]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[2]);

    Audio8ResampleHalf(in, out);

    return TCL_OK;
}

int 
Audio16ResampleHalfCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s in out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    out = GetAudio(argv[2]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[2]);

    Audio16ResampleHalf(in, out);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_resample_quarter <in> <out>
 *     audio_16_resample_quarter <in> <out>
 * precond 
 *     <in> and <out> must be 8 or 16-bit audio buffers
 * return
 *     none
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8ResampleQuarterCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s in out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    out = GetAudio(argv[2]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[2]);

    Audio8ResampleQuarter(in, out);

    return TCL_OK;
}

int 
Audio16ResampleQuarterCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s in out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    out = GetAudio(argv[2]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[2]);

    Audio16ResampleQuarter(in, out);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_resample_linear <in> <insamples> <outsamples> <out>
 *     audio_16_resample_linear <in> <insamples> <outsamples> <out>
 * precond 
 *     <in> and <out> must be 8 or 16-bit audio buffers
 * return
 *     none
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8ResampleLinearCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;
    int status, inSamples, outSamples;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s in insamples outsamples out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &inSamples);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &outSamples);
    ReturnErrorIf(status != TCL_OK);
    out = GetAudio(argv[4]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[4]);

    Audio8ResampleLinear(in, inSamples, outSamples, out);
    return TCL_OK;
}

int 
Audio16ResampleLinearCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;
    int status, inSamples, outSamples;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s in insamples outsamples out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &inSamples);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &outSamples);
    ReturnErrorIf(status != TCL_OK);
    out = GetAudio(argv[4]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[4]);

    Audio16ResampleLinear(in, inSamples, outSamples, out);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_resample_decimate <in> <insamples> <outsamples> <out>
 *     audio_16_resample_decimate <in> <insamples> <outsamples> <out>
 * precond 
 *     <in> and <out> must be 8 or 16-bit audio buffers
 * return
 *     none
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8ResampleDecimateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;
    int status, inSamples, outSamples;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s in insamples outsamples out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &inSamples);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &outSamples);
    ReturnErrorIf(status != TCL_OK);
    out = GetAudio(argv[4]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[4]);

    Audio8ResampleDecimate(in, inSamples, outSamples, out);
    return TCL_OK;
}

int 
Audio16ResampleDecimateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in, *out;
    int status, inSamples, outSamples;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s in insamples outsamples out", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL,
        "no such audio %s\n", argv[1]);
    status = Tcl_GetInt(interp, argv[2], &inSamples);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &outSamples);
    ReturnErrorIf(status != TCL_OK);
    out = GetAudio(argv[4]);
    ReturnErrorIf1(out == NULL,
        "no such audio %s\n", argv[4]);

    Audio16ResampleDecimate(in, inSamples, outSamples, out);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audio_8_max_abs <in>
 *     audio_16_max_abs <in>
 * precond 
 *     <in> must be a 8 or 16-bit audio buffer
 * return
 *     The maximum absolute value of any audio sample in the buffer
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int 
Audio8MaxAbsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in;
    int rv;

    ReturnErrorIf1(argc != 2, "wrong # args: should be %s in", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL, "no such audio %s\n", argv[1]);

    rv = Audio16MaxAbs(in);
    sprintf(interp->result, "%d", rv);
    return TCL_OK;
}

int 
Audio16MaxAbsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *in;
    int rv;

    ReturnErrorIf1(argc != 2, "wrong # args: should be %s in", argv[0]);
    in = GetAudio(argv[1]);
    ReturnErrorIf1(in == NULL, "no such audio %s\n", argv[1]);

    rv = Audio16MaxAbs(in);
    sprintf(interp->result, "%d", rv);
    return TCL_OK;
}
