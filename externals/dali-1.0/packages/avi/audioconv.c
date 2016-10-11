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
 * audioconv.c
 *
 * Use windows codecs to compress audio data
 *
 *----------------------------------------------------------------------
 */

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include "aviInt.h"

/* Locate a driver that supports a given format and return its ID */
typedef struct {
    HACMDRIVERID hadid;
    WORD wFormatTag;
} FIND_DRIVER_INFO;

/* callback function for format enumeration */
BOOL CALLBACK 
find_format_enum(HACMDRIVERID hadid, LPACMFORMATDETAILS pafd, DWORD dwInstance, DWORD fdwSupport)
{
    FIND_DRIVER_INFO *pdi = (FIND_DRIVER_INFO *) dwInstance;

    if (pafd->dwFormatTag == (DWORD) pdi->wFormatTag) {
        /* found it */
        pdi->hadid = hadid;
        return FALSE;           /* stop enumerating */
    }
    return TRUE;                /* continue enumerating */
}

/* callback function for driver enumeration */
BOOL CALLBACK 
find_driver_enum(HACMDRIVERID hadid, DWORD dwInstance, DWORD fdwSupport)
{
    DWORD dwSize;
    FIND_DRIVER_INFO *pdi = (FIND_DRIVER_INFO *) dwInstance;
    HACMDRIVER had = NULL;
    MMRESULT mmr;
    ACMFORMATDETAILS fd;
    WAVEFORMATEX *pwf;

    /* open the driver */
    mmr = acmDriverOpen(&had, hadid, 0);
    if (mmr) {
        /* some error */
        return FALSE;           /* stop enumerating */

    }
    /* enumerate the formats it supports */
    dwSize = 0;
    mmr = acmMetrics(had, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
    if (dwSize < sizeof(WAVEFORMATEX))
        dwSize = sizeof(WAVEFORMATEX);  // for MS-PCM

    pwf = (WAVEFORMATEX *) MALLOC(dwSize);
    memset(pwf, 0, dwSize);
    pwf->cbSize = LOWORD(dwSize) - sizeof(WAVEFORMATEX);
    pwf->wFormatTag = pdi->wFormatTag;

    memset(&fd, 0, sizeof(fd));
    fd.cbStruct = sizeof(fd);
    fd.pwfx = pwf;
    fd.cbwfx = dwSize;
    fd.dwFormatTag = pdi->wFormatTag;
    mmr = acmFormatEnum(had, &fd, find_format_enum, (DWORD) (VOID *) pdi, 0);
    FREE(pwf);
    acmDriverClose(had, 0);
    if (pdi->hadid || mmr) {
        /* found it or some error */
        return FALSE;           /* stop enumerating */
    }
    return TRUE;                /* continue enumeration */
}

/* locate the first driver that supports a given format tag */
HACMDRIVERID 
find_driver(WORD wFormatTag)
{
    FIND_DRIVER_INFO fdi;
    MMRESULT mmr;

    fdi.hadid = NULL;
    fdi.wFormatTag = wFormatTag;
    mmr = acmDriverEnum(find_driver_enum, (DWORD) (VOID *) & fdi, 0);
    if (mmr)
        return NULL;
    return fdi.hadid;
}

/* get a description of the first format supported for a given tag */
WAVEFORMATEX *
get_driver_format(HACMDRIVERID hadid, WORD wFormatTag)
{
    /* open the driver */
    DWORD dwSize = 0;
    ACMFORMATDETAILS fd;
    WAVEFORMATEX *pwf;
    HACMDRIVER had = NULL;
    FIND_DRIVER_INFO fdi;

    MMRESULT mmr = acmDriverOpen(&had, hadid, 0);

    if (mmr) {
        return NULL;
    }
    /* allocate a structure for the info */
    mmr = acmMetrics(had, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
    if (dwSize < sizeof(WAVEFORMATEX))
        dwSize = sizeof(WAVEFORMATEX);  /* for MS-PCM */
    pwf = (WAVEFORMATEX *) MALLOC(dwSize);
    memset(pwf, 0, dwSize);
    pwf->cbSize = LOWORD(dwSize) - sizeof(WAVEFORMATEX);
    pwf->wFormatTag = wFormatTag;

    memset(&fd, 0, sizeof(fd));
    fd.cbStruct = sizeof(fd);
    fd.pwfx = pwf;
    fd.cbwfx = dwSize;
    fd.dwFormatTag = wFormatTag;

    /* set up a struct to control the enumeration */

    fdi.hadid = NULL;
    fdi.wFormatTag = wFormatTag;

    mmr = acmFormatEnum(had, &fd, find_format_enum, (DWORD) (VOID *) & fdi, 0);
    acmDriverClose(had, 0);
    if ((fdi.hadid == NULL) || mmr) {
        FREE(pwf);
        return NULL;
    }
    return pwf;
}

/* get a description of the exact format supported for a given driver and PCM */
WAVEFORMATEX *
get_exact_driver_format(HACMDRIVERID hadid, int rate, short bps, short nc)
{
    /* open the driver */
    DWORD dwSize = 0;
    ACMFORMATDETAILS fd;
    FIND_DRIVER_INFO fdi;
    WAVEFORMATEX *pwf;
    HACMDRIVER had = NULL;
    MMRESULT mmr = acmDriverOpen(&had, hadid, 0);

    if (mmr) {
        return NULL;
    }
    /* allocate a structure for the info */

    mmr = acmMetrics(had, ACM_METRIC_MAX_SIZE_FORMAT, &dwSize);
    if (dwSize < sizeof(WAVEFORMATEX))
        dwSize = sizeof(WAVEFORMATEX);  /* for MS-PCM */
    pwf = (WAVEFORMATEX *) MALLOC(dwSize);
    memset(pwf, 0, dwSize);
    pwf->cbSize = LOWORD(dwSize) - sizeof(WAVEFORMATEX);
    pwf->wFormatTag = WAVE_FORMAT_PCM;
    pwf->nChannels = nc;
    pwf->nSamplesPerSec = rate;
    pwf->wBitsPerSample = bps;

    memset(&fd, 0, sizeof(fd));
    fd.cbStruct = sizeof(fd);
    fd.pwfx = pwf;
    fd.cbwfx = dwSize;
    fd.dwFormatTag = WAVE_FORMAT_PCM;

    /* set up a struct to control the enumeration */
    fdi.hadid = NULL;
    fdi.wFormatTag = WAVE_FORMAT_PCM;

    mmr = acmFormatEnum(had, &fd, find_format_enum, (DWORD) (VOID *) & fdi,
        (ACM_FORMATENUMF_NCHANNELS |
            ACM_FORMATENUMF_NSAMPLESPERSEC |
            ACM_FORMATENUMF_WBITSPERSAMPLE));
    acmDriverClose(had, 0);
    if ((fdi.hadid == NULL) || mmr) {
        FREE(pwf);
        return NULL;
    }
    return pwf;
}

AudioConv *
AudioConvNew(tag, rate, bps, nc)
    int tag, rate;
    short bps, nc;
{
    /* First we fill in the source format */

    AudioConv *ac;
    WAVEFORMATEX wfSrc;
    MMRESULT mmr;
    WORD wFormatTag;
    HACMDRIVERID hadid;
    HACMDRIVER had = NULL;
    WAVEFORMATEX *pwfDrv, *pwfPCM;
    HACMSTREAM hstr = NULL;

    memset(&wfSrc, 0, sizeof(wfSrc));
    wfSrc.cbSize = 0;
    wfSrc.wFormatTag = WAVE_FORMAT_PCM;         /* pcm */
    wfSrc.nChannels = nc;       /* channels */
    wfSrc.nSamplesPerSec = rate;        /* frequency */
    wfSrc.wBitsPerSample = bps; /* bits per sample */
    wfSrc.nBlockAlign = wfSrc.nChannels * wfSrc.wBitsPerSample / 8;
    wfSrc.nAvgBytesPerSec = wfSrc.nSamplesPerSec * wfSrc.nBlockAlign;


    /* The format to convert to */
    wFormatTag = tag;

    /* Now we locate a CODEC that supports the destination format tag */
    hadid = find_driver(wFormatTag);
    if (hadid == NULL) {
        return (NULL);
    }
    /* get the details of the format
     * Note: this is just the first of one or more possible formats 
     * for the given tag
     */
    pwfDrv = get_driver_format(hadid, wFormatTag);
    if (pwfDrv == NULL) {
        return NULL;
    }
    /* 
     * Check if the this is supported as a PCM format
     */
    pwfPCM = get_exact_driver_format(hadid, rate, bps, nc);
    if (pwfPCM == NULL) {
        FREE(pwfDrv);
        return NULL;
    }
    /* open the driver */
    mmr = acmDriverOpen(&had, hadid, 0);
    if (mmr) {
        FREE(pwfDrv);
        FREE(pwfPCM);
        return NULL;
    }
    /* open the conversion stream
     * Note the use of the ACM_STREAMOPENF_NONREALTIME flag. Without this
     * some software compressors will report error 512 - not possible
     */
    mmr = acmStreamOpen(&hstr,
        had,                    /* driver handle */
        &wfSrc,                 /* source format */
        pwfDrv,                 /* destination format */
        NULL,                   /* no filter */
        0,                      /* no callback */
        0,                      /* instance data (not used) */
        ACM_STREAMOPENF_NONREALTIME);   /* flags */

    if (mmr) {
        FREE(pwfDrv);
        FREE(pwfPCM);
        return (NULL);
    }
    ac = NEW(AudioConv);
    if (ac == NULL) {
        FREE(pwfDrv);
        FREE(pwfPCM);
        return NULL;
    }
    ac->had = had;
    ac->hstr = hstr;
    ac->format = tag;
    ac->bps = bps;
    ac->rate = rate;
    ac->nc = nc;
    ac->outBitrate = pwfDrv->nAvgBytesPerSec;
    ac->pwfDrv = pwfDrv;
    ac->pwfPCM = pwfPCM;
    return (ac);
}

void
AudioConvFree(ac)
    AudioConv *ac;
{
    int mmr;

    /* close the stream and driver */
    mmr = acmStreamClose(ac->hstr, 0);
    mmr = acmDriverClose(ac->had, 0);
    FREE(ac->pwfDrv);
    FREE(ac->pwfPCM);
    FREE(ac);
}



int
AudioConvEncode(ac, in, out, srcUsed)
    AudioConv *ac;
    Audio *in;
    BitParser *out;
    int *srcUsed;
{

    ACMSTREAMHEADER strhdr;
    MMRESULT mmr;
    int samplesize;
    unsigned char *indata, *outdata;
    int inlength, outlength;

    samplesize = ac->bps / 8;

    indata = in->firstSample;
    inlength = in->length * samplesize;

    outdata = out->offsetPtr;
    outlength = out->bs->size - (out->offsetPtr - out->bs->buffer);

    /*
     * set up the in and out buffers
     */
    memset(&strhdr, 0, sizeof(strhdr));
    strhdr.cbStruct = sizeof(strhdr);
    strhdr.pbSrc = indata;      /* the source data to convert */
    strhdr.cbSrcLength = inlength;      /* and its size */
    strhdr.pbDst = outdata;     /* ditto for the dest */
    strhdr.cbDstLength = outlength;

    /* prep the header */
    mmr = acmStreamPrepareHeader(ac->hstr, &strhdr, 0);
    if (mmr) {
        return mmr;
    }
    /* convert the data */
    mmr = acmStreamConvert(ac->hstr, &strhdr, 0);
    if (mmr) {
        return mmr;
    }
    /* unprepare the header */
    mmr = acmStreamUnprepareHeader(ac->hstr, &strhdr, 0);
    if (mmr) {
        return mmr;
    }
    /* Adjust the bitparser position - advance by no. of bytes written */
    out->offsetPtr += strhdr.cbDstLengthUsed;

    /* return the number of source bytes consumed */
    *srcUsed = (strhdr.cbSrcLengthUsed);
    return 0;

}

WaveHdr *
WaveHdrNewFromAudioConv(ac)
    AudioConv *ac;
{
    WaveHdr *hdr;
    WAVEFORMATEX *wfxDrv;

    hdr = NEW(WaveHdr);
    if (hdr == NULL) {
        return NULL;
    }
    wfxDrv = ac->pwfDrv;

    hdr->format = wfxDrv->wFormatTag;
    hdr->numOfChan = wfxDrv->nChannels;
    hdr->samplesPerSec = wfxDrv->nSamplesPerSec;
    hdr->bytesPerSec = wfxDrv->nAvgBytesPerSec;
    hdr->blockAlign = wfxDrv->nBlockAlign;
    hdr->bitsPerSample = wfxDrv->wBitsPerSample;
    hdr->dataLen = 0;
    hdr->extralen = wfxDrv->cbSize;
    if (hdr->extralen) {
        hdr->extra = NEWARRAY(unsigned char, hdr->extralen);

        if (hdr->extra == NULL) {
            FREE(hdr);
            return (NULL);
        }
        memcpy(hdr->extra, (unsigned char *) (wfxDrv + sizeof(WAVEFORMATEX)),
            hdr->extralen);
    } else {
        hdr->extra = NULL;
    }
    return hdr;
}
