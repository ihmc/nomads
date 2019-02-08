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
 * mpegaudio.c
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
extern AllocTableEntry *alloc_0[];
extern AllocTableEntry *alloc_1[];
extern AllocTableEntry *alloc_2[];
extern AllocTableEntry *alloc_3[];

static AllocTablePtr alloc_table_table[4] =
{
    (AllocTablePtr) & alloc_0,
    (AllocTablePtr) & alloc_1,
    (AllocTablePtr) & alloc_2,
    (AllocTablePtr) & alloc_3
};

static int which_table[3][2][16] =
{
    {
        {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 0},
        {0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}},
    {
        {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {
        {0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0},
        {0, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}}
};

/*
 * Two tables for dequantization of layer 2 audio data
 */
static double c[17] =
{
    1.33333333333, 1.60000000000, 1.14285714286,
    1.77777777777, 1.06666666666, 1.03225806452,
    1.01587301587, 1.00787401575, 1.00392156863,
    1.00195694716, 1.00097751711, 1.00048851979,
    1.00024420024, 1.00012208522, 1.00006103888,
    1.00003051851, 1.00001525902};

static double d[17] =
{
    0.500000000, 0.500000000, 0.250000000, 0.500000000,
    0.125000000, 0.062500000, 0.031250000, 0.015625000,
    0.007812500, 0.003906250, 0.001953125, 0.0009765625,
    0.00048828125, 0.00024414063, 0.00012207031,
    0.00006103516, 0.00003051758};

MpegAudioL2 *
MpegAudioL2New()
{
    MpegAudioL2 *audio;

    audio = NEW(MpegAudioL2);
    return audio;
}


void
MpegAudioL2Free(MpegAudioL2 * data)
{
    free(data);
}


void
MpegAudioL2ToAudio(hdr, data, v, pcm)
    MpegAudioHdr *hdr;
    MpegAudioL2 *data;
    MpegAudioSynData *v;
    Audio *pcm;
{
    static int times = 0;
    int sb, s, m;
    long k;
    double fraction[3][32];
    double *curr_fraction;
    short *curr_pcm;
    char *curr_alloc;
    unsigned int *curr_sample;
    int sblimit;
    int table_index;
    AllocTable *table;
    AllocTableEntry *table_entry;

    table_index = which_table[(int) hdr->sampling_rate_index][1][(int) hdr->bit_rate_index];
    table = alloc_table_table[table_index];

    curr_pcm = (short *) pcm->firstSample;
    sblimit = data->sblimit;
    curr_sample = &(data->sample[0][0][0]);

    for (s = 0; s < 12; s++) {

        curr_fraction = &(fraction[0][0]);
        for (m = 0; m < 3; m++) {

            // dequantize and denormalized

            curr_alloc = &(data->allocation[0]);
            for (sb = 0; sb < sblimit; sb++) {
                table_entry = &((*table)[sb][*curr_alloc]);
                times++;
                if (*curr_alloc) {
                    register int x;

                    // find MSB
                    x = 0;
                    while ((1L << x) < (int) (table_entry->steps))
                        x++;

                    // MSB inversion
                    if (((*curr_sample >> (x - 1)) & 1) == 1)
                        *curr_fraction = 0.0;
                    else
                        *curr_fraction = -1.0;

                    // calculate 2's complement sample
                    k = (1L << (x - 1));
                    *curr_fraction += (double) (*curr_sample & (k - 1)) / (double) k;
                    *curr_fraction += d[(*table)[sb][*curr_alloc].quant];
                    *curr_fraction *= c[(*table)[sb][*curr_alloc].quant];
                    *curr_fraction *= scale_factor_table[(int) data->scalefactor[s >> 2][sb]];

                } else {
                    *curr_fraction = 0.0;
                }
                curr_alloc++;
                curr_fraction++;
                curr_sample++;
            }

            DO_N_TIMES(32 - sblimit,
                *curr_fraction++ = 0.0;
                )
                curr_alloc += 32 - sblimit;
            curr_sample += 32 - sblimit;

            SubBandSynthesis(v, &(fraction[m][0]), &curr_pcm);
        }
    }
}

double
MpegAudioL2ScaleFactorSum(hdr, data)
    MpegAudioHdr *hdr;
    MpegAudioL2 *data;
{
    int s, m, sb;
    char *curr_alloc;
    int sblimit;
    double sum;

    sblimit = data->sblimit;
    sum = 0.0;

    for (s = 0; s < 12; s++) {
        for (m = 0; m < 3; m++) {
            curr_alloc = &(data->allocation[0]);
            for (sb = 0; sb < sblimit; sb++) {
                if (*curr_alloc++) {
                    sum += scale_factor_table[(int) data->scalefactor[s >> 2][sb]];
                }
            }
        }
    }

    return sum;
}

void
MpegAudioL2MonoParse(bp, hdr, audio)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL2 *audio;
{
    unsigned int num_of_bits;
    register int sb, gr, m;
    int table, sblimit;
    AllocTablePtr table_entry;


    static int sblims[4] =
    {
        27, 30, 8, 12
    };

    table = which_table[(int) hdr->sampling_rate_index][1][(int) hdr->bit_rate_index];
    sblimit = sblims[table];

    // still can be optimized. Based partially on mpegaudio's decode.c

    table_entry = alloc_table_table[table];
    audio->sblimit = sblimit;
    // audio->table   = alloc_table_table[table];

    for (sb = 0; sb < sblimit; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_GetBits(bp, num_of_bits, audio->allocation[sb]);
    }
    for (sb = 0; sb < sblimit; sb++) {
        if (audio->allocation[sb] != 0) {
            Bp_GetBits(bp, 2, audio->scfsi[sb]);
        }
    }
    for (sb = 0; sb < sblimit; sb++) {
        if (audio->allocation[sb] != 0) {
            switch (audio->scfsi[sb]) {
            case 0:
                Bp_GetBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, audio->scalefactor[1][sb]);
                Bp_GetBits(bp, 6, audio->scalefactor[2][sb]);
                break;
            case 1:
                Bp_GetBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, audio->scalefactor[2][sb]);
                audio->scalefactor[1][sb] = audio->scalefactor[0][sb];
                break;
            case 3:
                Bp_GetBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, audio->scalefactor[1][sb]);
                audio->scalefactor[2][sb] = audio->scalefactor[1][sb];
                break;
            case 2:
                Bp_GetBits(bp, 6, audio->scalefactor[0][sb]);
                audio->scalefactor[1][sb] = audio->scalefactor[0][sb];
                audio->scalefactor[2][sb] = audio->scalefactor[0][sb];
                break;
            }
        }
    }

    for (gr = 0; gr < 12; gr++) {
        for (sb = 0; sb < sblimit; sb++) {
            if (audio->allocation[sb]) {
                if ((*table_entry)[sb][audio->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][audio->allocation[sb]].bits;
                        Bp_GetBits(bp, num_of_bits, audio->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][audio->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][audio->allocation[sb]].bits;
                    Bp_GetBits(bp, num_of_bits, c);
                    for (m = 0; m < 3; m++) {
                        audio->sample[gr][m][sb] = c % num_of_levels;
                        c /= num_of_levels;
                    }
                }
            }
        }
    }

    Bp_ByteAlign(bp);
}


void
MpegAudioL2MonoEncode(audio, hdr, bp)
    MpegAudioL2 *audio;
    MpegAudioHdr *hdr;
    BitParser *bp;
{
    unsigned int num_of_bits;
    register int sb, gr, m;
    int table, sblimit;
    AllocTablePtr table_entry;

    static AllocTablePtr alloc_table_table[4] =
    {
        (AllocTablePtr) & alloc_0,
        (AllocTablePtr) & alloc_1,
        (AllocTablePtr) & alloc_2,
        (AllocTablePtr) & alloc_3
    };

    static int which_table[3][2][16] =
    {
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}},
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {
            {0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}}
    };

    static int sblims[4] =
    {
        27, 30, 8, 12
    };

    table = which_table[(int) hdr->sampling_rate_index][1][(int) hdr->bit_rate_index];
    sblimit = sblims[table];

    // still can be optimized. Based partially on mpegaudio's decode.c

    table_entry = alloc_table_table[table];
    audio->sblimit = sblimit;
    // audio->table   = alloc_table_table[table];

    for (sb = 0; sb < sblimit; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_PutBits(bp, num_of_bits, audio->allocation[sb]);
    }
    for (sb = 0; sb < sblimit; sb++) {
        if (audio->allocation[sb] != 0) {
            Bp_PutBits(bp, 2, audio->scfsi[sb]);
        }
    }
    for (sb = 0; sb < sblimit; sb++) {
        if (audio->allocation[sb] != 0) {
            switch (audio->scfsi[sb]) {
            case 0:
                Bp_PutBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, audio->scalefactor[1][sb]);
                Bp_PutBits(bp, 6, audio->scalefactor[2][sb]);
                break;
            case 1:
                Bp_PutBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, audio->scalefactor[2][sb]);
                break;
            case 3:
                Bp_PutBits(bp, 6, audio->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, audio->scalefactor[1][sb]);
                break;
            case 2:
                Bp_PutBits(bp, 6, audio->scalefactor[0][sb]);
                break;
            }
        }
    }

    for (gr = 0; gr < 12; gr++) {
        for (sb = 0; sb < sblimit; sb++) {
            if (audio->allocation[sb]) {
                if ((*table_entry)[sb][audio->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][audio->allocation[sb]].bits;
                        Bp_PutBits(bp, num_of_bits, audio->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][audio->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][audio->allocation[sb]].bits;
                    c = audio->sample[gr][0][sb] +
                        audio->sample[gr][1][sb] * num_of_levels +
                        audio->sample[gr][1][sb] * num_of_levels * num_of_levels;
                    Bp_PutBits(bp, num_of_bits, c);
                }
            }
        }
    }

    Bp_OutByteAlign(bp);
}


void
MpegAudioL2StereoParse(bp, hdr, left, right)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL2 *left, *right;
{
    int bound;
    unsigned int num_of_bits;
    register int sb, gr, m;
    int table, sblimit;
    AllocTablePtr table_entry;

    static AllocTablePtr alloc_table_table[4] =
    {
        (AllocTablePtr) & alloc_0,
        (AllocTablePtr) & alloc_1,
        (AllocTablePtr) & alloc_2,
        (AllocTablePtr) & alloc_3
    };

    static int which_table[3][2][16] =
    {
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}},
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {
            {0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}}
    };

    static int sblims[4] =
    {
        27, 30, 8, 12
    };

    table = which_table[(int) hdr->sampling_rate_index][0][(int) hdr->bit_rate_index];
    sblimit = sblims[table];
    bound = (hdr->mode == MPEG_AUDIO_JOINT_STEREO ?
        (hdr->mode_extension << 2) + 4 : sblimit);

    /*
     * Based partially on mpegaudio's decode.c
     * Everything from sblimit..32, or has allocation == 0 will not
     * be initialized
     */

    table_entry = alloc_table_table[table];
    left->sblimit = right->sblimit = sblimit;
    // left->table   = right->table = alloc_table_table[table];

    /*
     * Initialize allocation table
     */
    for (sb = 0; sb < bound; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_GetBits(bp, num_of_bits, left->allocation[sb]);
        Bp_GetBits(bp, num_of_bits, right->allocation[sb]);
    }
    for (sb = bound; sb < sblimit; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_GetBits(bp, num_of_bits, left->allocation[sb]);
        right->allocation[sb] = left->allocation[sb];
    }

#if defined(DEBUG_ALLOC) || defined(DEBUG_ALL)
    fprintf(stderr, "-- BIT ALLOC --\n");
    for (sb = 0; sb < sblimit; sb++) {
        fprintf(stderr, "%d\t%d\t%d\n", sb, left->allocation[sb],
            right->allocation[sb]);
    }
#endif

    /*
     * Initialize scfsi and scalefactor (only those with allocation == 0)
     */
    for (sb = 0; sb < sblimit; sb++) {
        if (left->allocation[sb] != 0) {
            Bp_GetBits(bp, 2, left->scfsi[sb]);
        }
        if (right->allocation[sb] != 0) {
            Bp_GetBits(bp, 2, right->scfsi[sb]);
        }
    }


    for (sb = 0; sb < sblimit; sb++) {
        if (left->allocation[sb] != 0) {
            switch (left->scfsi[sb]) {
            case 0:
                Bp_GetBits(bp, 6, left->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, left->scalefactor[1][sb]);
                Bp_GetBits(bp, 6, left->scalefactor[2][sb]);
                break;
            case 1:
                Bp_GetBits(bp, 6, left->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, left->scalefactor[2][sb]);
                left->scalefactor[1][sb] = left->scalefactor[0][sb];
                break;
            case 3:
                Bp_GetBits(bp, 6, left->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, left->scalefactor[1][sb]);
                left->scalefactor[2][sb] = left->scalefactor[1][sb];
                break;
            case 2:
                Bp_GetBits(bp, 6, left->scalefactor[0][sb]);
                left->scalefactor[1][sb] = left->scalefactor[0][sb];
                left->scalefactor[2][sb] = left->scalefactor[0][sb];
                break;
            }
        }
#if defined(DEBUG_SCALE) || defined(DEBUG_ALL)
        else {
            left->scalefactor[0][sb] = 63;
            left->scalefactor[1][sb] = 63;
            left->scalefactor[2][sb] = 63;
        }
#endif
        if (right->allocation[sb] != 0) {
            switch (right->scfsi[sb]) {
            case 0:
                Bp_GetBits(bp, 6, right->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, right->scalefactor[1][sb]);
                Bp_GetBits(bp, 6, right->scalefactor[2][sb]);
                break;
            case 1:
                Bp_GetBits(bp, 6, right->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, right->scalefactor[2][sb]);
                right->scalefactor[1][sb] = right->scalefactor[0][sb];
                break;
            case 3:
                Bp_GetBits(bp, 6, right->scalefactor[0][sb]);
                Bp_GetBits(bp, 6, right->scalefactor[1][sb]);
                right->scalefactor[2][sb] = right->scalefactor[1][sb];
                break;
            case 2:
                Bp_GetBits(bp, 6, right->scalefactor[0][sb]);
                right->scalefactor[1][sb] = right->scalefactor[0][sb];
                right->scalefactor[2][sb] = right->scalefactor[0][sb];
                break;
            }
        }
#if defined(DEBUG_SCALE) || defined(DEBUG_ALL)
        else {
            right->scalefactor[0][sb] = 63;
            right->scalefactor[1][sb] = 63;
            right->scalefactor[2][sb] = 63;
        }
#endif
    }

#if defined(DEBUG_SCALE) || defined(DEBUG_ALL)
    fprintf(stderr, "-- SCALE INDEX --\n");
    for (sb = 0; sb < sblimit; sb++) {
        fprintf(stderr, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n", sb,
            left->scalefactor[0][sb],
            left->scalefactor[1][sb],
            left->scalefactor[2][sb],
            right->scalefactor[0][sb],
            right->scalefactor[1][sb],
            right->scalefactor[2][sb]);
    }
#endif

    for (gr = 0; gr < 12; gr++) {
        /*
         * Read the samples for 0 .. bound
         */
        for (sb = 0; sb < bound; sb++) {
            if (left->allocation[sb]) {
                if ((*table_entry)[sb][left->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                        Bp_GetBits(bp, num_of_bits, left->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][left->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                    Bp_GetBits(bp, num_of_bits, c);
                    for (m = 0; m < 3; m++) {
                        left->sample[gr][m][sb] = c % num_of_levels;
                        c /= num_of_levels;
                    }
                }
            }
            if (right->allocation[sb]) {
                if ((*table_entry)[sb][right->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][right->allocation[sb]].bits;
                        Bp_GetBits(bp, num_of_bits, right->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][right->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][right->allocation[sb]].bits;
                    Bp_GetBits(bp, num_of_bits, c);
                    for (m = 0; m < 3; m++) {
                        right->sample[gr][m][sb] = c % num_of_levels;
                        c /= num_of_levels;
                    }
                }
            }
        }

        /*
         * Read the samples for bound .. sblimit
         */
        for (sb = bound; sb < sblimit; sb++) {
            if (left->allocation[sb]) {
                if ((*table_entry)[sb][left->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                        Bp_GetBits(bp, num_of_bits, left->sample[gr][m][sb]);
                        right->sample[gr][m][sb] = left->sample[gr][m][sb];
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][left->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                    Bp_GetBits(bp, num_of_bits, c);
                    for (m = 0; m < 3; m++) {
                        left->sample[gr][m][sb] = c % num_of_levels;
                        right->sample[gr][m][sb] = left->sample[gr][m][sb];
                        c /= num_of_levels;
                    }
                }
            }
        }
    }

    Bp_ByteAlign(bp);
}


void
MpegAudioL2StereoEncode(left, right, hdr, bp)
    MpegAudioL2 *left, *right;
    MpegAudioHdr *hdr;
    BitParser *bp;
{
    int bound;
    unsigned int num_of_bits;
    register int sb, gr, m;
    int table, sblimit;
    AllocTablePtr table_entry;

    static AllocTablePtr alloc_table_table[4] =
    {
        (AllocTablePtr) & alloc_0,
        (AllocTablePtr) & alloc_1,
        (AllocTablePtr) & alloc_2,
        (AllocTablePtr) & alloc_3
    };

    static int which_table[3][2][16] =
    {
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}},
        {
            {0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {
            {0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0},
            {0, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}}
    };

    static int sblims[4] =
    {
        27, 30, 8, 12
    };

    table = which_table[(int) hdr->sampling_rate_index][0][(int) hdr->bit_rate_index];
    sblimit = sblims[table];
    bound = (hdr->mode == MPEG_AUDIO_JOINT_STEREO ?
        (hdr->mode_extension << 2) + 4 : sblimit);

    /*
     * Based partially on mpegaudio's decode.c
     * Everything from sblimit..32, or has allocation == 0 will not
     * be initialized
     */

    table_entry = alloc_table_table[table];
    left->sblimit = right->sblimit = sblimit;
    // left->table   = right->table = alloc_table_table[table];

    /*
     * Initialize allocation table
     */
    for (sb = 0; sb < bound; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_PutBits(bp, num_of_bits, left->allocation[sb]);
        Bp_PutBits(bp, num_of_bits, right->allocation[sb]);
    }
    for (sb = bound; sb < sblimit; sb++) {
        num_of_bits = (*table_entry)[sb][0].bits;
        Bp_PutBits(bp, num_of_bits, left->allocation[sb]);
    }

    /*
     * Initialize scfsi and scalefactor (only those with allocation == 0)
     */
    for (sb = 0; sb < sblimit; sb++) {
        if (left->allocation[sb] != 0) {
            Bp_PutBits(bp, 2, left->scfsi[sb]);
        }
        if (right->allocation[sb] != 0) {
            Bp_PutBits(bp, 2, right->scfsi[sb]);
        }
    }


    for (sb = 0; sb < sblimit; sb++) {
        if (left->allocation[sb] != 0) {
            switch (left->scfsi[sb]) {
            case 0:
                Bp_PutBits(bp, 6, left->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, left->scalefactor[1][sb]);
                Bp_PutBits(bp, 6, left->scalefactor[2][sb]);
                break;
            case 1:
                Bp_PutBits(bp, 6, left->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, left->scalefactor[2][sb]);
                break;
            case 3:
                Bp_PutBits(bp, 6, left->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, left->scalefactor[1][sb]);
                break;
            case 2:
                Bp_PutBits(bp, 6, left->scalefactor[0][sb]);
                break;
            }
        }
        if (right->allocation[sb] != 0) {
            switch (right->scfsi[sb]) {
            case 0:
                Bp_PutBits(bp, 6, right->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, right->scalefactor[1][sb]);
                Bp_PutBits(bp, 6, right->scalefactor[2][sb]);
                break;
            case 1:
                Bp_PutBits(bp, 6, right->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, right->scalefactor[2][sb]);
                break;
            case 3:
                Bp_PutBits(bp, 6, right->scalefactor[0][sb]);
                Bp_PutBits(bp, 6, right->scalefactor[1][sb]);
                break;
            case 2:
                Bp_PutBits(bp, 6, right->scalefactor[0][sb]);
                break;
            }
        }
    }

    for (gr = 0; gr < 12; gr++) {
        /*
         * Read the samples for 0 .. bound
         */
        for (sb = 0; sb < bound; sb++) {
            if (left->allocation[sb]) {
                if ((*table_entry)[sb][left->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                        Bp_PutBits(bp, num_of_bits, left->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][left->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                    c = left->sample[gr][0][sb] +
                        left->sample[gr][1][sb] * num_of_levels +
                        left->sample[gr][1][sb] * num_of_levels * num_of_levels;
                    Bp_PutBits(bp, num_of_bits, c);
                }
            }
            if (right->allocation[sb]) {
                if ((*table_entry)[sb][right->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][right->allocation[sb]].bits;
                        Bp_PutBits(bp, num_of_bits, right->sample[gr][m][sb]);
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][right->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][right->allocation[sb]].bits;
                    c = right->sample[gr][0][sb] +
                        right->sample[gr][1][sb] * num_of_levels +
                        right->sample[gr][1][sb] * num_of_levels * num_of_levels;
                    Bp_PutBits(bp, num_of_bits, c);
                }
            }
        }

        /*
         * Read the samples for bound .. sblimit
         */
        for (sb = bound; sb < sblimit; sb++) {
            if (left->allocation[sb]) {
                if ((*table_entry)[sb][left->allocation[sb]].group == 3) {
                    for (m = 0; m < 3; m++) {
                        num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                        Bp_PutBits(bp, num_of_bits, left->sample[gr][m][sb]);
                        right->sample[gr][m][sb] = left->sample[gr][m][sb];
                    }
                } else {
                    unsigned int num_of_levels, c;

                    num_of_levels = (*table_entry)[sb][left->allocation[sb]].steps;
                    num_of_bits = (*table_entry)[sb][left->allocation[sb]].bits;
                    c = right->sample[gr][0][sb] +
                        right->sample[gr][1][sb] * num_of_levels +
                        right->sample[gr][1][sb] * num_of_levels * num_of_levels;
                    Bp_PutBits(bp, num_of_bits, c);
                }
            }
        }
    }

    Bp_OutByteAlign(bp);
}
