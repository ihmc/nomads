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
 * mpegaudiol1.c
 *
 * Functions that manipulate Mpeg Audio
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"
#include "dvmmpeg.h"

extern double scale_factor_table[64];
extern double filter_table[64][32];
extern double decode_window[512];


extern int layer1_bit_alloc_table[];

MpegAudioL1 *
MpegAudioL1New()
{
    MpegAudioL1 *audio;

    audio = NEW(MpegAudioL1);
    return audio;
}


void
MpegAudioL1Free(data)
    MpegAudioL1 *data;
{
    FREE(data);
}


void
MpegAudioL1MonoParse(bp, hdr, audio)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL1 *audio;
{
    register int s;
    char *alloc, *sf;
    unsigned int *sample;

    alloc = audio->allocation;
    DO_N_TIMES(32,
        Bp_GetBits(bp, 4, *alloc);
        alloc++;
        );

    alloc = audio->allocation;
    sf = audio->scalefactor;
    DO_N_TIMES(32,
        {
            if (*alloc++) {
                Bp_GetBits(bp, 6, *sf);
            }
            sf++;
        }
    );

    sample = &(audio->sample[0][0]);
    alloc = audio->allocation;
    for (s = 0; s < 12; s++) {
        alloc = audio->allocation;
        DO_N_TIMES(32,
            {
                if (*alloc) {
                    Bp_GetBits(bp,
                        layer1_bit_alloc_table[(int) *alloc],
                        *sample);
                }
                alloc++;
                sample++;
            }
        );
    }

    Bp_ByteAlign(bp);
}


void
MpegAudioL1MonoEncode(audio, hdr, bp)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL1 *audio;
{
    register int s;
    char *alloc, *sf;
    unsigned int *sample;

    alloc = audio->allocation;
    DO_N_TIMES(32,
        Bp_PutBits(bp, 4, *alloc);
        alloc++;
        );

    alloc = audio->allocation;
    sf = audio->scalefactor;
    DO_N_TIMES(32,
        {
            if (*alloc++) {
                Bp_PutBits(bp, 6, *sf);
            }
            sf++;
        }
    );

    sample = &(audio->sample[0][0]);
    alloc = audio->allocation;
    for (s = 0; s < 12; s++) {
        alloc = audio->allocation;
        DO_N_TIMES(32,
            {
                if (*alloc) {
                    Bp_PutBits(bp,
                        layer1_bit_alloc_table[(int) *alloc],
                        *sample);
                }
                alloc++;
                sample++;
            }
        );
    }

    Bp_OutByteAlign(bp);
}


void
MpegAudioL1StereoParse(bp, hdr, left, right)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL1 *left, *right;
{
    int bound;
    register int sb, s;

    bound = (hdr->mode == MPEG_AUDIO_JOINT_STEREO ?
        (hdr->mode_extension + 1) << 2 : 32);

    /*
     * Read the bit allocation table
     */
    for (sb = 0; sb < bound; sb++) {
        Bp_GetBits(bp, 4, left->allocation[sb]);
        Bp_GetBits(bp, 4, right->allocation[sb]);
    }
    for (sb = bound; sb < 32; sb++) {
        Bp_GetBits(bp, 4, left->allocation[sb]);
        right->allocation[sb] = left->allocation[sb];
    }

    for (sb = 0; sb < 32; sb++) {
        if (left->allocation[sb] != 0) {
            Bp_GetBits(bp, 6, left->scalefactor[sb])
        }
        if (right->allocation[sb] != 0) {
            Bp_GetBits(bp, 6, right->scalefactor[sb]);
        }
    }

    for (s = 0; s < 12; s++) {
        for (sb = 0; sb < bound; sb++) {
            if (left->allocation[sb] != 0) {
                Bp_GetBits(bp,
                    layer1_bit_alloc_table[(int) left->allocation[sb]],
                    left->sample[s][sb]);
            }
            if (right->allocation[sb] != 0) {
                Bp_GetBits(bp,
                    layer1_bit_alloc_table[(int) right->allocation[sb]],
                    right->sample[s][sb]);
            }
        }
        for (sb = bound; sb < 32; sb++) {
            if (left->allocation[sb] != 0) {
                Bp_GetBits(bp,
                    layer1_bit_alloc_table[(int) left->allocation[sb]],
                    left->sample[s][sb]);
                right->sample[s][sb] = left->sample[s][sb];
            }
        }
    }

    Bp_ByteAlign(bp);
}


void
MpegAudioL1StereoEncode(left, right, hdr, bp)
    MpegAudioL1 *left, *right;
    MpegAudioHdr *hdr;
    BitParser *bp;
{
    int bound;
    register int sb, s;

    bound = (hdr->mode == MPEG_AUDIO_JOINT_STEREO ?
        (hdr->mode_extension + 1) << 2 : 32);

    /*
     * Write the bit allocation table
     */
    for (sb = 0; sb < bound; sb++) {
        Bp_PutBits(bp, 4, left->allocation[sb]);
        Bp_PutBits(bp, 4, right->allocation[sb]);
    }
    for (sb = bound; sb < 32; sb++) {
        Bp_PutBits(bp, 4, left->allocation[sb]);
    }

    for (sb = 0; sb < 32; sb++) {
        if (left->allocation[sb] != 0) {
            Bp_PutBits(bp, 6, left->scalefactor[sb])
        }
        if (right->allocation[sb] != 0) {
            Bp_PutBits(bp, 6, right->scalefactor[sb]);
        }
    }

    for (s = 0; s < 12; s++) {
        for (sb = 0; sb < bound; sb++) {
            if (left->allocation[sb] != 0) {
                Bp_PutBits(bp,
                    layer1_bit_alloc_table[(int) left->allocation[sb]],
                    left->sample[s][sb]);
            }
            if (right->allocation[sb] != 0) {
                Bp_PutBits(bp,
                    layer1_bit_alloc_table[(int) right->allocation[sb]],
                    right->sample[s][sb]);
            }
        }
        for (sb = bound; sb < 32; sb++) {
            if (left->allocation[sb] != 0) {
                Bp_PutBits(bp,
                    layer1_bit_alloc_table[(int) left->allocation[sb]],
                    left->sample[s][sb]);
            }
        }
    }

    Bp_OutByteAlign(bp);
}


void
MpegAudioL1ToAudio(hdr, data, v, pcm)
    MpegAudioHdr *hdr;
    MpegAudioL1 *data;
    MpegAudioSynData *v;
    Audio *pcm;
{
    int sb, s;
    long k, l;
    int num_of_bits;
    double fraction[32];
    double delta;
    double *curr_fraction;
    short *curr_pcm;
    char *curr_scale_factor;
    char *curr_alloc;

    curr_pcm = (short *) pcm->firstSample;

    for (s = 0; s < 12; s++) {

        // dequantize and denormalized

        curr_alloc = &(data->allocation[0]);
        curr_scale_factor = &(data->scalefactor[0]);
        curr_fraction = &(fraction[0]);

        for (sb = 0; sb < 32; sb++) {

            if (*curr_alloc) {

                num_of_bits = *curr_alloc + 1;
                if (((data->sample[s][sb] >> (num_of_bits - 1)) & 1) == 1)
                    *curr_fraction = 0.0;
                else
                    *curr_fraction = -1.0;
                k = (1L << (num_of_bits - 1));
                l = (1L << num_of_bits);
                *curr_fraction += (double) (data->sample[s][sb] & (k - 1)) / (double) k;
                delta = (double) l / (l - 1) * scale_factor_table[(int) *curr_scale_factor];
                *curr_fraction = (*curr_fraction + 1.0 / (double) k) * delta;

            } else {
                *curr_fraction = 0.0;
            }

            curr_scale_factor++;
            curr_fraction++;
            curr_alloc++;
        }

        SubBandSynthesis(v, &(fraction[0]), &curr_pcm);
    }
}
