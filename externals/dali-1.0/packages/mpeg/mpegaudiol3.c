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
#include "huffmantable.h"
#include "tangentable.h"
#include "powertable.h"
#include "cossintable.h"

#define MXOFF  250
#define MAX_EXPONENT 400
#define MIN_EXPONENT -400
#define SFB_WIDTH 0
#define SFB_START 1
#define SFB_END 2

extern double scale_factor_table[64];
extern double filter_table[64][32];
extern double decode_window[512];
void Bp_RestoreAnyBits(BitParser *, int n);

static int slen[2][16] =
{
    {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
    {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
};

int pre_table[22] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0
};

/*
 * Scalefactor Band Lookup table used in dequantization
 * 1st Array Dimension: 44.1 KHz, 48 KHz, 32 KHz
 * 2nd Array Dimension: Width of Band, Index of Start, Index of End
 * 3rd Array Dimension: 12 scale factor bands
 */
short sfb_table_short[3][3][14] =
{
    {
        {4, 4, 4, 4, 6, 8, 10, 12, 14, 18, 22, 30, 56, 1},
        {0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192},
        {3, 7, 11, 15, 21, 29, 39, 51, 65, 83, 105, 135, 191, 192}
    },
    {
        {4, 4, 4, 4, 6, 6, 10, 12, 14, 16, 20, 26, 66, 1},
        {0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192},
        {3, 7, 11, 15, 21, 27, 37, 49, 63, 79, 99, 125, 191, 192}
    },
    {
        {4, 4, 4, 4, 6, 8, 12, 16, 20, 26, 34, 42, 12, 1},
        {0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192},
        {3, 7, 11, 15, 21, 29, 41, 57, 77, 103, 137, 179, 191, 192}
    }
};

/*
 * Scalefactor Band Lookup table used in dequantization
 * 1st Array Dimension: 44.1 KHz, 48 KHz, 32 KHz
 * 2nd Array Dimension: Width of Band, Index of Start, Index of End
 * 3rd Array Dimension: 21 scale factor bands
 */
const short sfb_table_long[3][3][23] =
{
    {
        {4, 4, 4, 4, 4, 4, 6, 6, 8, 8, 10, 12, 16,
            20, 24, 28, 34, 42, 50, 54, 76, 158, 1},
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74,
            90, 110, 134, 162, 196, 238, 288, 342, 418, 576},
        {3, 7, 11, 15, 19, 23, 29, 35, 43, 51, 61, 73, 89,
            109, 133, 161, 195, 237, 287, 341, 417, 575, 576}
    },
    {
        {4, 4, 4, 4, 4, 4, 6, 6, 6, 8, 10, 12, 16,
            18, 22, 28, 34, 40, 46, 54, 54, 192, 1},
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72,
            88, 106, 128, 156, 190, 230, 276, 330, 384, 576},
        {3, 7, 11, 15, 19, 23, 29, 35, 41, 49, 59, 71, 87,
            105, 127, 155, 189, 229, 275, 329, 383, 575, 576}
    },
    {
        {4, 4, 4, 4, 4, 4, 6, 6, 8, 10, 12, 16,
            20, 24, 30, 38, 46, 56, 68, 84, 102, 26, 1},
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82,
            102, 126, 156, 194, 240, 296, 364, 448, 550, 576},
        {3, 7, 11, 15, 19, 23, 29, 35, 43, 53, 65, 81, 101,
            125, 155, 193, 239, 295, 363, 447, 549, 575, 576}
    }
};

double cs[8] =
{
    0.85749292571254431827,
    0.88174199731770519506,
    0.9496286491027328136,
    0.98331459249179020699,
    0.99551781606758582566,
    0.99916055817814752871,
    0.9998991952444471476,
    0.99999315507028030403,
};

double ca[8] =
{
    -0.51449575542752656876,
    -0.47173196856497234819,
    -0.31337745420390183959,
    -0.18191319961098117997,
    -0.094574192526420658433,
    -0.040965582885304052674,
    -0.014198568572471150298,
    -0.0036999746737600373164,
};

MpegAudioL3 *
MpegAudioL3New()
{
    MpegAudioL3 *audio;

    audio = NEW(MpegAudioL3);
    return audio;
}


void
MpegAudioL3Free(audio)
    MpegAudioL3 *audio;
{
    FREE(audio);
}


void
RequantizeLong(data, gr, ch, curr_xr, freq, cutoff)
    MpegAudioL3 *data;
    int gr;
    int ch;
    double *curr_xr;
    int freq;
    int cutoff;
{
    int cb_next, i, cb, exponent, freq_line, sign;
    int gain210, preflag, scalefac;

    cb = 0;
    cb_next = sfb_table_long[freq][1][1];
    cutoff = data->zero_freq_start[gr][ch];
    gain210 = data->global_gain[gr][ch] - 210;
    preflag = data->preflag[gr][ch];
    scalefac = (data->scalefac_scale[gr][ch] + 1) * (-2);

    for (i = 0; i < cutoff; i++) {
        if (i == cb_next) {
            cb++;
            cb_next = sfb_table_long[freq][1][cb + 1];
        }
        exponent = gain210 + scalefac * (data->scale_fac_l[gr][ch][cb] + preflag * pre_table[cb]);
        if (exponent > MAX_EXPONENT)
            exponent = MAX_EXPONENT;
        else if (exponent < MIN_EXPONENT)
            exponent = MIN_EXPONENT;

        if (exponent >= 0) {
            *curr_xr = power_table[exponent];
        } else {
            *curr_xr = power_table2[-1 * exponent];
        }
        // Scale the quantized value, same for all block types
        // Need to raise positive numbers to 4/3 power.  So take absolute
        // value of frequency lines, then make them negative again after.

        freq_line = data->freq_lines[gr][ch][i];
        sign = freq_line < 0 ? 1 : 0;

        if (freq_line < 0)
            freq_line = -freq_line;
        if (freq_line > 8205)
            freq_line = 8205;
        *curr_xr *= base_table[freq_line];

        if (sign) {
            *curr_xr *= -1;
        }
        curr_xr++;
    }
}

void
RequantizeShort(data, gr, ch, curr_xr, freq, cutoff)
    MpegAudioL3 *data;
    int gr;
    int ch;
    double *curr_xr;
    int freq;
    int cutoff;
{
    int cb;
    int cb_next, cb_width, cb_start;
    int i;
    int window, freq_line, exponent, sign;
    int gain210, scalefac;

    cb = 0;
    cb_next = sfb_table_short[freq][1][1] * 3;
    cb_width = sfb_table_short[freq][1][1];
    cb_start = 0;
    cutoff = data->zero_freq_start[gr][ch];
    gain210 = data->global_gain[gr][ch] - 210;
    scalefac = (1 + data->scalefac_scale[gr][ch]) << 1;
    for (i = 0; i < cutoff; i++) {
        if (i == cb_next) {
            cb++;
            cb_next = sfb_table_short[freq][1][cb + 1] * 3;
            cb_width = sfb_table_short[freq][0][cb];
            cb_start = sfb_table_short[freq][1][cb] * 3;
        }
        // dequantize

        window = (i - cb_start) / cb_width;
        exponent = gain210 - (data->subblock_gain[window][gr][ch] << 3)
            - scalefac * data->scale_fac_s[gr][ch][cb][window];

        if (exponent > MAX_EXPONENT)
            exponent = MAX_EXPONENT;
        else if (exponent < MIN_EXPONENT)
            exponent = MIN_EXPONENT;

        if (exponent >= 0) {
            *curr_xr = power_table[exponent];
        } else {
            *curr_xr = power_table2[-1 * exponent];
        }

        // Scale the quantized value, same for all block types
        // Need to raise positive numbers to 4/3 power.  So take absolute
        // value of frequency lines, then make them negative again after.

        freq_line = data->freq_lines[gr][ch][i];
        sign = freq_line < 0 ? 1 : 0;

        if (freq_line < 0)
            freq_line = -freq_line;
        if (freq_line > 8205)
            freq_line = 8205;
        *curr_xr *= base_table[freq_line];

        if (sign) {
            *curr_xr *= -1;
        }
        curr_xr++;
    }
}

void
RequantizeSwitch(data, gr, ch, curr_xr, freq, cutoff)
    MpegAudioL3 *data;
    int gr;
    int ch;
    double *curr_xr;
    int freq;
    int cutoff;
{
    int cb_next, cb_width, cb_start;
    int i;
    int cb, freq_line, sign, window, exponent;

    /* make compiler happy */
    cb_width = 0;
    cb_start = 0;

    cb = 0;
    cb_next = sfb_table_long[freq][1][1];
    for (i = 0; i < cutoff; i++) {
        if (i == cb_next) {
            if (i == sfb_table_long[freq][1][8]) {
                cb_next = sfb_table_short[freq][1][4] * 3;
                cb = 3;
                cb_width = sfb_table_short[freq][0][3];
                cb_start = sfb_table_short[freq][1][3] * 3;
            } else if (i < sfb_table_long[freq][1][8]) {
                cb++;
                cb_next = sfb_table_long[freq][1][cb + 1];
            } else {
                cb++;
                cb_next = sfb_table_short[freq][1][cb + 1] * 3;
                cb_width = sfb_table_short[freq][0][cb];
                cb_start = sfb_table_short[freq][1][cb] * 3;
            }
        }
        // dequantize

        if ((i >> 5) >= 2) {
            window = (i - cb_start) / cb_width;
            exponent = data->global_gain[gr][ch]
                - 210 - (data->subblock_gain[window][gr][ch] << 3)
                - ((1 + data->scalefac_scale[gr][ch]) << 1)
                * data->scale_fac_s[gr][ch][cb][window];
        } else {
            exponent = (data->global_gain[gr][ch] - 210)
                + (-2 * (1 + data->scalefac_scale[gr][ch]))
                * (data->scale_fac_l[gr][ch][cb] + data->preflag[gr][ch]
                * pre_table[cb]);
        }
        if (exponent > MAX_EXPONENT)
            exponent = MAX_EXPONENT;
        else if (exponent < MIN_EXPONENT)
            exponent = MIN_EXPONENT;

        if (exponent >= 0) {
            *curr_xr = power_table[exponent];
        } else {
            *curr_xr = power_table2[-1 * exponent];
        }

        // Scale the quantized value, same for all block types
        // Need to raise positive numbers to 4/3 power.  So take absolute
        // value of frequency lines, then make them negative again after.

        freq_line = data->freq_lines[gr][ch][i];
        sign = freq_line < 0 ? 1 : 0;

        if (freq_line < 0)
            freq_line = -freq_line;
        if (freq_line > 8205)
            freq_line = 8205;
        *curr_xr *= base_table[freq_line];

        if (sign) {
            *curr_xr *= -1;
        }
        curr_xr++;
    }
}

void
MpegAudioL3StereoToAudio(hdr, data, lv, rv, prev_granule, left_pcm, right_pcm)
    MpegAudioHdr *hdr;
    MpegAudioL3 *data;
    MpegAudioSynData *lv;
    MpegAudioSynData *rv;
    MpegAudioGraData *prev_granule;
    Audio *left_pcm;
    Audio *right_pcm;
{
    int num_of_channel;
    int i, gr, ch, freq;
    int window;
    int cutoff;
    int intensity_stereo;
    int ms_stereo;
    int sb, ss;
    int sfb;
    int sfbcnt;
    int j;
    short *curr_left_pcm;
    short *curr_right_pcm;
    register double *curr_xr;
    register double *curr_xr0;
    register double *curr_xr1;
    double xr[2][2][576];
    int is_pos[576];
    char split_and_type_2;

    curr_left_pcm = (short *) left_pcm->firstSample;
    curr_right_pcm = (short *) right_pcm->firstSample;
    freq = hdr->sampling_rate_index;
    num_of_channel = (hdr->mode == MPEG_AUDIO_SINGLE_CHANNEL) ? 1 : 2;

    // Step 1 : Requantize

    for (gr = 0; gr < 2; gr++) {
        for (ch = 0; ch < num_of_channel; ch++) {
            split_and_type_2 = (data->block_split_flag[gr][ch] && data->block_type[gr][ch] == 2);
            cutoff = data->zero_freq_start[gr][ch];
            curr_xr = &(xr[gr][ch][cutoff]);
            if (cutoff != 576) {
                DO_N_TIMES(576 - cutoff,
                    *curr_xr++ = 0.0;
                    );
            }
            if (split_and_type_2) {
                if (data->switch_point[gr][ch])
                    RequantizeSwitch(data, gr, ch, &(xr[gr][ch][0]), freq, cutoff);
                else
                    RequantizeShort(data, gr, ch, &(xr[gr][ch][0]), freq, cutoff);
            } else {
                RequantizeLong(data, gr, ch, &(xr[gr][ch][0]), freq, cutoff);
            }
        }
    }

    // Step 2 : Stereo Dematrix

    if (num_of_channel == 2) {
        for (gr = 0; gr < 2; gr++) {

            cutoff = max(data->zero_freq_start[gr][1], data->zero_freq_start[gr][0]);
            for (i = 0; i < cutoff; i++) {
                is_pos[i] = 7;
            }

            intensity_stereo = (hdr->mode == 1 && hdr->mode_extension & 0x01);
            ms_stereo = (hdr->mode == 1 && hdr->mode_extension & 0x02);

            if (intensity_stereo) {
                if (data->block_type[gr][0] == 2) {
                    if (data->switch_point[gr][0]) {
                        int max_sfb = 0;

                        for (window = 0; window < 3; window++) {

                            sfbcnt = 2;
                            for (sfb = 12; sfb >= 3; sfb--) {
                                int lines;

                                lines = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + (window + 1) * lines - 1;
                                while (lines > 0) {
                                    if (xr[gr][1][i] != 0.0) {
                                        sfbcnt = sfb;
                                        sfb = -10;
                                        lines = -10;
                                    }
                                    lines--;
                                    i--;
                                }
                            }
                            sfb = sfbcnt + 1;

                            if (sfb > max_sfb) {
                                max_sfb = sfb;
                            }
                            while (sfb < 12) {
                                sb = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + window * sb;
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_s[gr][1][sfb][window];
                                    i++;
                                }
                                sfb++;
                            }
                            sb = sfb_table_short[freq][SFB_WIDTH][10];
                            sfb = 3 * sfb_table_short[freq][SFB_START][10]
                                + window * sb;
                            sb = sfb_table_short[freq][SFB_WIDTH][11];
                            i = 3 * sfb_table_short[freq][SFB_START][11]
                                + window * sb;
                            for (; sb > 0; sb--) {
                                is_pos[i] = is_pos[sfb];
                                i++;
                            }
                        }
                        // Long window processing starts here
                        if (max_sfb <= 3) {
                            j = 2;
                            ss = 17;
                            sb = -1;
                            while (j >= 0) {
                                if (xr[gr][1][j * 18 + ss] != 0.0) {
                                    sb = j * 18 + ss;
                                    j = -1;
                                } else {
                                    ss--;
                                    if (ss < 0) {
                                        j--;
                                        ss = 17;
                                    }
                                }
                            }
                            i = 0;
                            while (sfb_table_long[freq][SFB_START][i] <= sb) {
                                i++;
                            }
                            sfb = i;
                            i = sfb_table_long[freq][SFB_START][i];
                            for (; sfb < 8; sfb++) {
                                sb = sfb_table_long[freq][SFB_WIDTH][sfb];
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_l[gr][1][sfb];
                                    i++;
                                }
                            }
                        }
                    } else {
                        for (window = 0; window < 3; window++) {
                            sfbcnt = -1;
                            for (sfb = 12; sfb >= 0; sfb--) {
                                int lines;

                                lines = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + (window + 1) * lines - 1;
                                while (lines > 0) {
                                    if (xr[gr][1][i] != 0.0) {
                                        sfbcnt = sfb;
                                        sfb = -10;
                                        lines = -10;
                                    }
                                    lines--;
                                    i--;
                                }
                            }
                            sfb = sfbcnt + 1;
                            while (sfb < 12) {
                                sb = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + window * sb;
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_s[gr][1][sfb][window];
                                    i++;
                                }
                                sfb++;
                            }

                            sb = sfb_table_short[freq][SFB_WIDTH][10];
                            sfb = 3 * sfb_table_short[freq][SFB_START][10] + window * sb;
                            sb = sfb_table_short[freq][SFB_WIDTH][11];
                            i = 3 * sfb_table_short[freq][SFB_START][11] + window * sb;
                            for (; sb > 0; sb--) {
                                is_pos[i] = is_pos[sfb];
                                i++;
                            }
                        }

                    }
                } else {        // block_type != 2
                    // long window processing ( block type not 2 )

                    j = 31;
                    ss = 17;
                    sb = 0;
                    while (j >= 0) {
                        if (xr[gr][1][j * 18 + ss] != 0.0) {
                            sb = j * 18 + ss;
                            j = -1;
                        } else {
                            ss--;
                            if (ss < 0) {
                                j--;
                                ss = 17;
                            }
                        }
                    }
                    i = 0;
                    while (sfb_table_long[freq][SFB_START][i] <= sb) {
                        i++;
                    }
                    sfb = i;
                    i = sfb_table_long[freq][SFB_START][i];
                    for (; sfb < 21; sfb++) {
                        sb = sfb_table_long[freq][SFB_WIDTH][sfb];
                        for (; sb > 0; sb--) {
                            is_pos[i] = data->scale_fac_l[gr][1][sfb];
                            i++;
                        }
                    }
                    sfb = sfb_table_long[freq][SFB_START][20];
                    for (sb = 576 - sfb_table_long[freq][SFB_START][21];
                        sb > 0; sb--) {
                        is_pos[i] = is_pos[sfb];
                        i++;
                    }
                }
            }                   // end if (intensity_stereo)

            // process data up to cutoff
            curr_xr0 = &(xr[gr][0][0]);
            curr_xr1 = &(xr[gr][1][0]);
            for (i = 0; i < cutoff; i++) {
                if (is_pos[i] == 7) {
                    if (ms_stereo) {
                        double temp0 = *curr_xr0;
                        double temp1 = *curr_xr1;

                        *curr_xr0 = (temp0 + temp1) / 1.41421356;
                        *curr_xr1 = (temp0 - temp1) / 1.41421356;
                    }
                } else if (intensity_stereo) {
                    *curr_xr1 = *curr_xr0 * tan12B[is_pos[i]];
                    *curr_xr0 = *curr_xr0 * tan12A[is_pos[i]];
                } else {
                    printf("Error in streo processing\n");
                }
                curr_xr0++;
                curr_xr1++;
            }
        }
    }
    // reuse xr to store the reordered output.
    // Step 3 : Reorder the samples

    for (gr = 0; gr < 2; gr++) {

        double out[2][18][32];
        double *curr_gr;

        for (ch = 0; ch < num_of_channel; ch++) {

            int sfb_line;
            int sfb_start;
            int des_line;
            int p;
            double sum;
            int block_type, block;
            double rawout[36];
            int sblim;

            // See if block should be reordered
            if (data->block_split_flag[gr][ch] && (data->block_type[gr][ch] == 2)) {
                double xs[576];
                int sfb3;
                register double *curr_xs;

                if (data->switch_point[gr][ch]) {

                    // No reorder for low 2 subbands
                    // 0-17 is subband 0, 18-35 is subband 1
                    curr_xr = &(xr[gr][ch][0]);
                    curr_xs = &(xs[0]);
                    DO_N_TIMES(36,
                        *curr_xs++ = *curr_xr++;
                        );

                    // Initialization
                    DO_N_TIMES(540,
                        *curr_xs++ = 0.0;
                        );

                    // reordering for rest switched short
                    for (sfb = 3, sfb_start = sfb_table_short[freq][1][3],
                        sfb_line = sfb_table_short[freq][1][4] - sfb_start;
                        sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                        (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                        int sfb3 = sfb_start * 3;

                        // window = 0
                        des_line = sfb3;
                        curr_xr = &(xr[gr][ch][sfb3]);
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 1;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 2;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                    }
                } else {

                    //Initialization
                    curr_xs = &(xs[0]);
                    DO_N_TIMES(576,
                        *curr_xs++ = 0.0;
                        );

                    //Pure short blocks
                    for (sfb = 0, sfb_start = 0, sfb_line = sfb_table_short[freq][1][1];
                        sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                        (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                        sfb3 = sfb_start * 3;

                        des_line = sfb3;
                        curr_xr = &(xr[gr][ch][sfb3]);
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 1;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 2;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                    }
                }
                curr_xr = &(xr[gr][ch][0]);
                curr_xs = &(xs[0]);
                DO_N_TIMES(576,
                    *curr_xr++ = *curr_xs++;
                    );
            }
            // Step 4 : Alias Cancellation

            if (!(data->block_type[gr][ch] == 2 && !data->switch_point[gr][ch]
                    && data->block_split_flag[gr][ch])) {
                if (data->block_type[gr][ch] == 2 && data->switch_point[gr][ch]
                    && data->block_split_flag[gr][ch]) {
                    sblim = 1;
                } else {
                    sblim = 31;
                }

                curr_xr0 = &(xr[gr][ch][17]);
                curr_xr1 = &(xr[gr][ch][18]);
                for (sb = 0; sb < sblim; sb++) {
                    for (ss = 0; ss < 8; ss++) {
                        double bu, bd;

                        bu = *curr_xr0;
                        bd = *curr_xr1;
                        *curr_xr0-- = (bu * cs[ss]) - (bd * ca[ss]);
                        *curr_xr1++ = (bd * cs[ss]) + (bu * ca[ss]);
                    }
                    curr_xr0 += 26;
                    curr_xr1 += 10;
                }
            }
            // Step 5 : IMDCT and Overlap

            curr_xr = &(xr[gr][ch][0]);
            curr_gr = &((*prev_granule)[ch][0]);
            for (block = 0; block < 32; block++) {
                double *curr_cos;
                double *curr_in;

                block_type =
                    (data->switch_point[gr][ch] &&
                    data->block_split_flag[gr][ch] &&
                    (block < 2)) ? 0 : data->block_type[gr][ch];

                if (block_type == 2) {
                    // Short blocks
                    // also does shortwindow overlapping part.

                    rawout[0] = rawout[1] = rawout[2] = rawout[3] = 0.0;
                    rawout[4] = rawout[5] = rawout[6] = rawout[7] = 0.0;
                    rawout[8] = rawout[9] = rawout[10] = rawout[11] = 0.0;
                    rawout[12] = rawout[13] = rawout[14] = rawout[15] = 0.0;
                    rawout[16] = rawout[17] = rawout[18] = rawout[19] = 0.0;
                    rawout[20] = rawout[21] = rawout[22] = rawout[23] = 0.0;
                    rawout[24] = rawout[25] = rawout[26] = rawout[27] = 0.0;
                    rawout[28] = rawout[29] = rawout[30] = rawout[31] = 0.0;
                    rawout[32] = rawout[33] = rawout[34] = rawout[35] = 0.0;

                    for (i = 0; i < 3; i++) {
                        curr_cos = &(cos_table_long[0][0]);
                        for (p = 0; p < 12; p++) {
                            sum = 0.0;
                            curr_in = curr_xr + i;
                            DO_N_TIMES(6,
                                sum += (*curr_in) * (*curr_cos++);
                                curr_in += 3;
                                );
                            rawout[6 * i + p + 6] += sum * sin_window[2][p];
                        }
                    }
                    curr_xr += 18;
                } else {
                    curr_cos = &(cos_table_short[0][0]);
                    for (p = 0; p < 36; p++) {
                        sum = 0.0;
                        curr_in = curr_xr;
                        DO_N_TIMES(18,
                            sum += (*curr_in++) * (*curr_cos++);
                            );
                        rawout[p] = sum * sin_window[block_type][p];
                    }
                    curr_xr += 18;
                }
                // for (p = 0; p < 36; p++)
                //    fprintf(stderr, "%d %d %d %10.10g\n", count, p, block_type, rawout[p]); // DEBUG
                // count++;

                // Overlap
                for (i = 0; i < 18; i++) {
                    out[ch][i][block] = rawout[i] + *curr_gr;
                    *curr_gr++ = rawout[i + 18];
                    // fprintf(stderr, "%10.10g\n", out[gr][ch][i][block]); // DEBUG
                }
                if (block % 2) {
                    //Multiply every other sample by -1
                    out[ch][1][block] = -out[ch][1][block];
                    out[ch][3][block] = -out[ch][3][block];
                    out[ch][5][block] = -out[ch][5][block];
                    out[ch][7][block] = -out[ch][7][block];
                    out[ch][9][block] = -out[ch][9][block];
                    out[ch][11][block] = -out[ch][11][block];
                    out[ch][13][block] = -out[ch][13][block];
                    out[ch][15][block] = -out[ch][15][block];
                    out[ch][17][block] = -out[ch][17][block];
                }
            }                   // for block = 0 .. 32

        }                       // for each channel

        for (sb = 0; sb < 18; sb++) {
            SubBandSynthesis(lv, &(out[0][sb][0]), &curr_left_pcm);
            SubBandSynthesis(rv, &(out[1][sb][0]), &curr_right_pcm);
        }
    }                           // for each granule

}


void
MpegAudioL3MonoToAudio(hdr, data, lv, prev_granule, left_pcm)
    MpegAudioHdr *hdr;
    MpegAudioL3 *data;
    MpegAudioSynData *lv;
    MpegAudioGraData *prev_granule;
    Audio *left_pcm;
{
    int i, gr, freq;
    int cutoff;
    int sb, ss;
    int sfb;
    short *curr_left_pcm;
    register double *curr_xr;
    register double *curr_xr0;
    register double *curr_xr1;
    double xr[2][576];
    char split_and_type_2;

    curr_left_pcm = (short *) left_pcm->firstSample;
    freq = hdr->sampling_rate_index;

    // Step 1 : Requantize

    for (gr = 0; gr < 2; gr++) {
        split_and_type_2 = (data->block_split_flag[gr][0] && data->block_type[gr][0] == 2);
        cutoff = data->zero_freq_start[gr][0];
        curr_xr = &(xr[gr][cutoff]);
        if (cutoff != 576) {
            DO_N_TIMES(576 - cutoff,
                *curr_xr++ = 0.0;
                );
        }
        if (split_and_type_2) {
            if (data->switch_point[gr][0])
                RequantizeSwitch(data, gr, 0, &(xr[gr][0]), freq, cutoff);
            else
                RequantizeShort(data, gr, 0, &(xr[gr][0]), freq, cutoff);
        } else {
            RequantizeLong(data, gr, 0, &(xr[gr][0]), freq, cutoff);
        }
    }


    // reuse xr to store the reordered output.
    // Step 3 : Reorder the samples

    for (gr = 0; gr < 2; gr++) {

        double out[2][18][32];
        double *curr_gr;

        int sfb_line;
        int sfb_start;
        int des_line;
        int p;
        double sum;
        int block_type, block;
        double rawout[36];
        int sblim;

        // See if block should be reordered
        if (data->block_split_flag[gr][0] && (data->block_type[gr][0] == 2)) {
            double xs[576];
            int sfb3;
            register double *curr_xs;

            if (data->switch_point[gr][0]) {

                // No reorder for low 2 subbands
                // 0-17 is subband 0, 18-35 is subband 1
                curr_xr = &(xr[gr][0]);
                curr_xs = &(xs[0]);
                DO_N_TIMES(36,
                    *curr_xs++ = *curr_xr++;
                    );

                // Initialization
                DO_N_TIMES(540,
                    *curr_xs++ = 0.0;
                    );

                // reordering for rest switched short
                for (sfb = 3, sfb_start = sfb_table_short[freq][1][3],
                    sfb_line = sfb_table_short[freq][1][4] - sfb_start;
                    sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                    (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                    int sfb3 = sfb_start * 3;

                    // window = 0
                    des_line = sfb3;
                    curr_xr = &(xr[gr][sfb3]);
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                    des_line = sfb3 + 1;
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                    des_line = sfb3 + 2;
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                }
            } else {

                //Initialization
                curr_xs = &(xs[0]);
                DO_N_TIMES(576,
                    *curr_xs++ = 0.0;
                    );

                //Pure short blocks
                for (sfb = 0, sfb_start = 0, sfb_line = sfb_table_short[freq][1][1];
                    sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                    (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                    sfb3 = sfb_start * 3;

                    des_line = sfb3;
                    curr_xr = &(xr[gr][sfb3]);
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                    des_line = sfb3 + 1;
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                    des_line = sfb3 + 2;
                    DO_N_TIMES(sfb_line,
                        xs[des_line] = *curr_xr++;
                        des_line += 3;
                        );
                }
            }
            curr_xr = &(xr[gr][0]);
            curr_xs = &(xs[0]);
            DO_N_TIMES(576,
                *curr_xr++ = *curr_xs++;
                );
        }
        // Step 4 : Alias Cancellation

        if (!(data->block_type[gr][0] == 2 && !data->switch_point[gr][0]
                && data->block_split_flag[gr][0])) {
            if (data->block_type[gr][0] == 2 && data->switch_point[gr][0]
                && data->block_split_flag[gr][0]) {
                sblim = 1;
            } else {
                sblim = 31;
            }

            curr_xr0 = &(xr[gr][17]);
            curr_xr1 = &(xr[gr][18]);
            for (sb = 0; sb < sblim; sb++) {
                for (ss = 0; ss < 8; ss++) {
                    double bu, bd;

                    bu = *curr_xr0;
                    bd = *curr_xr1;
                    *curr_xr0-- = (bu * cs[ss]) - (bd * ca[ss]);
                    *curr_xr1++ = (bd * cs[ss]) + (bu * ca[ss]);
                }
                curr_xr0 += 26;
                curr_xr1 += 10;
            }
        }
        // Step 5 : IMDCT and Overlap

        curr_xr = &(xr[gr][0]);
        curr_gr = &((*prev_granule)[0][0]);
        for (block = 0; block < 32; block++) {
            double *curr_cos;
            double *curr_in;

            block_type =
                (data->switch_point[gr][0] &&
                data->block_split_flag[gr][0] &&
                (block < 2)) ? 0 : data->block_type[gr][0];

            if (block_type == 2) {
                // Short blocks
                // also does shortwindow overlapping part.

                rawout[0] = rawout[1] = rawout[2] = rawout[3] = 0.0;
                rawout[4] = rawout[5] = rawout[6] = rawout[7] = 0.0;
                rawout[8] = rawout[9] = rawout[10] = rawout[11] = 0.0;
                rawout[12] = rawout[13] = rawout[14] = rawout[15] = 0.0;
                rawout[16] = rawout[17] = rawout[18] = rawout[19] = 0.0;
                rawout[20] = rawout[21] = rawout[22] = rawout[23] = 0.0;
                rawout[24] = rawout[25] = rawout[26] = rawout[27] = 0.0;
                rawout[28] = rawout[29] = rawout[30] = rawout[31] = 0.0;
                rawout[32] = rawout[33] = rawout[34] = rawout[35] = 0.0;

                for (i = 0; i < 3; i++) {
                    curr_cos = &(cos_table_long[0][0]);
                    for (p = 0; p < 12; p++) {
                        sum = 0.0;
                        curr_in = curr_xr + i;
                        DO_N_TIMES(6,
                            sum += (*curr_in) * (*curr_cos++);
                            curr_in += 3;
                            );
                        rawout[6 * i + p + 6] += sum * sin_window[2][p];
                    }
                }
                curr_xr += 18;
            } else {
                curr_cos = &(cos_table_short[0][0]);
                for (p = 0; p < 36; p++) {
                    sum = 0.0;
                    curr_in = curr_xr;
                    DO_N_TIMES(18,
                        sum += (*curr_in++) * (*curr_cos++);
                        );
                    rawout[p] = sum * sin_window[block_type][p];
                }
                curr_xr += 18;
            }
            // for (p = 0; p < 36; p++)
            //    fprintf(stderr, "%d %d %d %10.10g\n", count, p, block_type, rawout[p]); // DEBUG
            // count++;

            // Overlap
            for (i = 0; i < 18; i++) {
                out[0][i][block] = rawout[i] + *curr_gr;
                *curr_gr++ = rawout[i + 18];
                // fprintf(stderr, "%10.10g\n", out[gr][0][i][block]); // DEBUG
            }
            if (block % 2) {
                //Multiply every other sample by -1
                out[0][1][block] = -out[0][1][block];
                out[0][3][block] = -out[0][3][block];
                out[0][5][block] = -out[0][5][block];
                out[0][7][block] = -out[0][7][block];
                out[0][9][block] = -out[0][9][block];
                out[0][11][block] = -out[0][11][block];
                out[0][13][block] = -out[0][13][block];
                out[0][15][block] = -out[0][15][block];
                out[0][17][block] = -out[0][17][block];
            }
        }                       // for block = 0 .. 32

        for (sb = 0; sb < 18; sb++) {
            SubBandSynthesis(lv, &(out[0][sb][0]), &curr_left_pcm);
        }
    }                           // for each granule

}

void
MpegAudioL3ToAudioSlow(hdr, data, v, prev_granule, left_pcm, right_pcm)
    MpegAudioHdr *hdr;
    MpegAudioL3 *data;
    MpegAudioSynData *v;
    MpegAudioGraData *prev_granule;
    Audio *left_pcm;
    Audio *right_pcm;
{
    int num_of_channel;
    int i, gr, ch, cb, freq;
    int window;
    int intensity_stereo;
    int ms_stereo;
    int sb, ss;
    int sfb;
    int sfbcnt;
    int j;
    short *curr_left_pcm;
    short *curr_right_pcm;
    register double *curr_xr;
    register double *curr_xr0;
    register double *curr_xr1;
    double xr[2][2][576];
    int is_pos[576];
    char split_and_type_2;

    curr_left_pcm = (short *) left_pcm->firstSample;
    curr_right_pcm = (short *) right_pcm->firstSample;
    freq = hdr->sampling_rate_index;
    num_of_channel = (hdr->mode == MPEG_AUDIO_SINGLE_CHANNEL) ? 1 : 2;

    // Step 1 : Requantize

    for (gr = 0; gr < 2; gr++) {
        for (ch = 0; ch < num_of_channel; ch++) {
            int sign;
            int cb_next, cb_width = 0, cb_start = 0;
            int exponent;
            int cutoff;
            int freq_line;

            split_and_type_2 = (data->block_split_flag[gr][ch] && data->block_type[gr][ch] == 2);

            cb = 0;
            if (split_and_type_2) {
                if (data->switch_point[gr][ch]) {
                    cb_next = sfb_table_long[freq][1][1];
                } else {
                    cb_next = sfb_table_short[freq][1][1] * 3;
                    cb_width = sfb_table_short[freq][1][1];
                    cb_start = 0;
                }
            } else {
                cb_next = sfb_table_long[freq][1][1];
            }

            cutoff = data->zero_freq_start[gr][ch];

            curr_xr = &(xr[gr][ch][cutoff]);
            if (cutoff != 576) {
                DO_N_TIMES(576 - cutoff,
                    *curr_xr++ = 0.0;
                    );
            }
            curr_xr = &(xr[gr][ch][0]);
            for (i = 0; i < cutoff; i++) {
                if (i == cb_next) {
                    if (split_and_type_2) {
                        if (data->switch_point[gr][ch]) {
                            if (i == sfb_table_long[freq][1][8]) {
                                cb_next = sfb_table_short[freq][1][4] * 3;
                                cb = 3;
                                cb_width = sfb_table_short[freq][0][3];
                                cb_start = sfb_table_short[freq][1][3] * 3;
                            } else if (i < sfb_table_long[freq][1][8]) {
                                cb++;
                                cb_next = sfb_table_long[freq][1][cb + 1];
                            } else {
                                cb++;
                                cb_next = sfb_table_short[freq][1][cb + 1] * 3;
                                cb_width = sfb_table_short[freq][0][cb];
                                cb_start = sfb_table_short[freq][1][cb] * 3;
                            }
                        } else {
                            cb++;
                            cb_next = sfb_table_short[freq][1][cb + 1] * 3;
                            cb_width = sfb_table_short[freq][0][cb];
                            cb_start = sfb_table_short[freq][1][cb] * 3;
                        }
                    } else {
                        cb++;
                        cb_next = sfb_table_long[freq][1][cb + 1];
                    }
                }
                // dequantize

                if (data->block_split_flag[gr][ch] &&
                    ((data->block_type[gr][ch] == 2 && data->switch_point[gr][ch] == 0) ||
                        (data->block_type[gr][ch] == 2 && data->switch_point[gr][ch] && ((i >> 5) >= 2))
                    )
                    ) {

                    // short block

                    window = (i - cb_start) / cb_width;
                    exponent = data->global_gain[gr][ch] - 210 - (data->subblock_gain[window][gr][ch] << 3) -
                        ((1 + data->scalefac_scale[gr][ch]) << 1) * data->scale_fac_s[gr][ch][cb][window];

                    if (exponent > MAX_EXPONENT)
                        exponent = MAX_EXPONENT;
                    else if (exponent < MIN_EXPONENT)
                        exponent = MIN_EXPONENT;

                    if (exponent >= 0) {
                        *curr_xr = power_table[exponent];
                    } else {
                        *curr_xr = power_table2[-1 * exponent];
                    }
                } else {

                    // long block

                    exponent = (data->global_gain[gr][ch] - 210) +
                        (-2 * (1 + data->scalefac_scale[gr][ch])) *
                        (data->scale_fac_l[gr][ch][cb] +
                        data->preflag[gr][ch] * pre_table[cb]);
                    if (exponent > MAX_EXPONENT)
                        exponent = MAX_EXPONENT;
                    else if (exponent < MIN_EXPONENT)
                        exponent = MIN_EXPONENT;

                    if (exponent >= 0) {
                        *curr_xr = power_table[exponent];
                    } else {
                        *curr_xr = power_table2[-1 * exponent];
                    }
                }

                // Scale the quantized value, same for all block types
                // Need to raise positive numbers to 4/3 power.  So take absolute
                // value of frequency lines, then make them negative again after.

                freq_line = data->freq_lines[gr][ch][i];
                sign = freq_line < 0 ? 1 : 0;

                if (freq_line < 0)
                    freq_line = -freq_line;
                if (freq_line > 8205)
                    freq_line = 8205;
                *curr_xr *= base_table[freq_line];

                if (sign) {
                    *curr_xr *= -1;
                }
                curr_xr++;
            }
        }
    }


    // Step 2 : Stereo Dematrix

    if (num_of_channel == 2) {
        for (gr = 0; gr < 2; gr++) {
            int cutoff;

            cutoff = max(data->zero_freq_start[gr][1], data->zero_freq_start[gr][0]);
            for (i = 0; i < cutoff; i++) {
                is_pos[i] = 7;
            }

            intensity_stereo = (hdr->mode == 1 && hdr->mode_extension & 0x01);
            ms_stereo = (hdr->mode == 1 && hdr->mode_extension & 0x02);

            if (intensity_stereo) {
                if (data->block_type[gr][0] == 2) {
                    if (data->switch_point[gr][0]) {
                        int max_sfb = 0;

                        for (window = 0; window < 3; window++) {

                            sfbcnt = 2;
                            for (sfb = 12; sfb >= 3; sfb--) {
                                int lines;

                                lines = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + (window + 1) * lines - 1;
                                while (lines > 0) {
                                    if (xr[gr][1][i] != 0.0) {
                                        sfbcnt = sfb;
                                        sfb = -10;
                                        lines = -10;
                                    }
                                    lines--;
                                    i--;
                                }
                            }
                            sfb = sfbcnt + 1;

                            if (sfb > max_sfb) {
                                max_sfb = sfb;
                            }
                            while (sfb < 12) {
                                sb = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + window * sb;
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_s[gr][1][sfb][window];
                                    i++;
                                }
                                sfb++;
                            }
                            sb = sfb_table_short[freq][SFB_WIDTH][10];
                            sfb = 3 * sfb_table_short[freq][SFB_START][10]
                                + window * sb;
                            sb = sfb_table_short[freq][SFB_WIDTH][11];
                            i = 3 * sfb_table_short[freq][SFB_START][11]
                                + window * sb;
                            for (; sb > 0; sb--) {
                                is_pos[i] = is_pos[sfb];
                                i++;
                            }
                        }
                        // Long window processing starts here
                        if (max_sfb <= 3) {
                            j = 2;
                            ss = 17;
                            sb = -1;
                            while (j >= 0) {
                                if (xr[gr][1][j * 18 + ss] != 0.0) {
                                    sb = j * 18 + ss;
                                    j = -1;
                                } else {
                                    ss--;
                                    if (ss < 0) {
                                        j--;
                                        ss = 17;
                                    }
                                }
                            }
                            i = 0;
                            while (sfb_table_long[freq][SFB_START][i] <= sb) {
                                i++;
                            }
                            sfb = i;
                            i = sfb_table_long[freq][SFB_START][i];
                            for (; sfb < 8; sfb++) {
                                sb = sfb_table_long[freq][SFB_WIDTH][sfb];
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_l[gr][1][sfb];
                                    i++;
                                }
                            }
                        }
                    } else {
                        for (window = 0; window < 3; window++) {
                            sfbcnt = -1;
                            for (sfb = 12; sfb >= 0; sfb--) {
                                int lines;

                                lines = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + (window + 1) * lines - 1;
                                while (lines > 0) {
                                    if (xr[gr][1][i] != 0.0) {
                                        sfbcnt = sfb;
                                        sfb = -10;
                                        lines = -10;
                                    }
                                    lines--;
                                    i--;
                                }
                            }
                            sfb = sfbcnt + 1;
                            while (sfb < 12) {
                                sb = sfb_table_short[freq][SFB_WIDTH][sfb];
                                i = 3 * sfb_table_short[freq][SFB_START][sfb]
                                    + window * sb;
                                for (; sb > 0; sb--) {
                                    is_pos[i] = data->scale_fac_s[gr][1][sfb][window];
                                    i++;
                                }
                                sfb++;
                            }

                            sb = sfb_table_short[freq][SFB_WIDTH][10];
                            sfb = 3 * sfb_table_short[freq][SFB_START][10] + window * sb;
                            sb = sfb_table_short[freq][SFB_WIDTH][11];
                            i = 3 * sfb_table_short[freq][SFB_START][11] + window * sb;
                            for (; sb > 0; sb--) {
                                is_pos[i] = is_pos[sfb];
                                i++;
                            }
                        }

                    }
                } else {        // block_type != 2
                    // long window processing ( block type not 2 )

                    j = 31;
                    ss = 17;
                    sb = 0;
                    while (j >= 0) {
                        if (xr[gr][1][j * 18 + ss] != 0.0) {
                            sb = j * 18 + ss;
                            j = -1;
                        } else {
                            ss--;
                            if (ss < 0) {
                                j--;
                                ss = 17;
                            }
                        }
                    }
                    i = 0;
                    while (sfb_table_long[freq][SFB_START][i] <= sb) {
                        i++;
                    }
                    sfb = i;
                    i = sfb_table_long[freq][SFB_START][i];
                    for (; sfb < 21; sfb++) {
                        sb = sfb_table_long[freq][SFB_WIDTH][sfb];
                        for (; sb > 0; sb--) {
                            is_pos[i] = data->scale_fac_l[gr][1][sfb];
                            i++;
                        }
                    }
                    sfb = sfb_table_long[freq][SFB_START][20];
                    for (sb = 576 - sfb_table_long[freq][SFB_START][21];
                        sb > 0; sb--) {
                        is_pos[i] = is_pos[sfb];
                        i++;
                    }
                }
            }                   // end if (intensity_stereo)

            // process data up to cutoff
            curr_xr0 = &(xr[gr][0][0]);
            curr_xr1 = &(xr[gr][1][0]);
            for (i = 0; i < cutoff; i++) {
                if (is_pos[i] == 7) {
                    if (ms_stereo) {
                        double temp0 = *curr_xr0;
                        double temp1 = *curr_xr1;

                        *curr_xr0 = (temp0 + temp1) / 1.41421356;
                        *curr_xr1 = (temp0 - temp1) / 1.41421356;
                    }
                } else if (intensity_stereo) {
                    *curr_xr1 = *curr_xr0 * tan12B[is_pos[i]];
                    *curr_xr0 = *curr_xr0 * tan12A[is_pos[i]];
                } else {
                    printf("Error in streo processing\n");
                }
                curr_xr0++;
                curr_xr1++;
            }
        }
    }
    // reuse xr to store the reordered output.
    // Step 3 : Reorder the samples

    for (gr = 0; gr < 2; gr++) {

        double out[2][18][32];
        double *curr_gr;

        for (ch = 0; ch < num_of_channel; ch++) {

            int sfb_line;
            int sfb_start;
            int des_line;
            int k, p, m;
            double sum;
            int block_type, block;
            double rawout[36];
            double in[18];
            int sblim;

            // See if block should be reordered
            if (data->block_split_flag[gr][ch] && (data->block_type[gr][ch] == 2)) {
                double xs[576];
                int sfb3;
                register double *curr_xs;

                if (data->switch_point[gr][ch]) {

                    // No reorder for low 2 subbands
                    // 0-17 is subband 0, 18-35 is subband 1
                    curr_xr = &(xr[gr][ch][0]);
                    curr_xs = &(xs[0]);
                    DO_N_TIMES(36,
                        *curr_xs++ = *curr_xr++;
                        );

                    // Initialization
                    DO_N_TIMES(540,
                        *curr_xs++ = 0.0;
                        );

                    // reordering for rest switched short
                    for (sfb = 3, sfb_start = sfb_table_short[freq][1][3],
                        sfb_line = sfb_table_short[freq][1][4] - sfb_start;
                        sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                        (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                        int sfb3 = sfb_start * 3;

                        // window = 0
                        des_line = sfb3;
                        curr_xr = &(xr[gr][ch][sfb3]);
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 1;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 2;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                    }
                } else {

                    //Initialization
                    curr_xs = &(xs[0]);
                    DO_N_TIMES(576,
                        *curr_xs++ = 0.0;
                        );

                    //Pure short blocks
                    for (sfb = 0, sfb_start = 0, sfb_line = sfb_table_short[freq][1][1];
                        sfb < 13; sfb++, sfb_start = sfb_table_short[freq][1][sfb],
                        (sfb_line = sfb_table_short[freq][1][sfb + 1] - sfb_start)) {
                        sfb3 = sfb_start * 3;

                        des_line = sfb3;
                        curr_xr = &(xr[gr][ch][sfb3]);
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 1;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                        des_line = sfb3 + 2;
                        DO_N_TIMES(sfb_line,
                            xs[des_line] = *curr_xr++;
                            des_line += 3;
                            );
                    }
                }
                curr_xr = &(xr[gr][ch][0]);
                curr_xs = &(xs[0]);
                DO_N_TIMES(576,
                    *curr_xr++ = *curr_xs++;
                    );
            }
            // reuse xs for x
            // Step 4 : Alias Cancellation

            if (!(data->block_type[gr][ch] == 2 && !data->switch_point[gr][ch]
                    && data->block_split_flag[gr][ch])) {
                if (data->block_type[gr][ch] == 2 && data->switch_point[gr][ch]
                    && data->block_split_flag[gr][ch]) {
                    sblim = 1;
                } else {
                    sblim = 31;
                }

                curr_xr0 = &(xr[gr][ch][17]);
                curr_xr1 = &(xr[gr][ch][18]);
                for (sb = 0; sb < sblim; sb++) {
                    for (ss = 0; ss < 8; ss++) {
                        double bu, bd;

                        bu = *curr_xr0;
                        bd = *curr_xr1;
                        *curr_xr0-- = (bu * cs[ss]) - (bd * ca[ss]);
                        *curr_xr1++ = (bd * cs[ss]) + (bu * ca[ss]);
                    }
                    curr_xr0 += 26;
                    curr_xr1 += 10;
                }
            }
            // Step 5 : IMDCT and Overlap

            curr_xr = &(xr[gr][ch][0]);
            curr_gr = &((*prev_granule)[ch][0]);
            for (block = 0; block < 32; block++) {
                double *curr_cos;

                block_type =
                    (data->switch_point[gr][ch] &&
                    data->block_split_flag[gr][ch] &&
                    (block < 2)) ? 0 : data->block_type[gr][ch];
                for (k = 0; k < 18; k++) {
                    in[k] = *curr_xr++;
                }

                rawout[0] = rawout[1] = rawout[2] = rawout[3] = 0.0;
                rawout[4] = rawout[5] = rawout[6] = rawout[7] = 0.0;
                rawout[8] = rawout[9] = rawout[10] = rawout[11] = 0.0;
                rawout[12] = rawout[13] = rawout[14] = rawout[15] = 0.0;
                rawout[16] = rawout[17] = rawout[18] = rawout[19] = 0.0;
                rawout[20] = rawout[21] = rawout[22] = rawout[23] = 0.0;
                rawout[24] = rawout[25] = rawout[26] = rawout[27] = 0.0;
                rawout[28] = rawout[29] = rawout[30] = rawout[31] = 0.0;
                rawout[32] = rawout[33] = rawout[34] = rawout[35] = 0.0;

                if (block_type == 2) {
                    // Short blocks
                    // also does shortwindow overlapping part.
                    for (i = 0; i < 3; i++) {
                        curr_cos = &(cos_table_long[0][0]);
                        for (p = 0; p < 12; p++) {
                            sum = 0.0;
                            for (m = 0; m < 6; m++)
                                sum += in[i + 3 * m] * (*curr_cos++);
                            rawout[6 * i + p + 6] += sum * sin_window[2][p];
                        }
                    }
                } else {
                    curr_cos = &(cos_table_short[0][0]);
                    for (p = 0; p < 36; p++) {
                        sum = 0.0;
                        for (m = 0; m < 18; m++)
                            sum += in[m] * (*curr_cos++);
                        rawout[p] = sum * sin_window[block_type][p];
                    }
                }
                // for (p = 0; p < 36; p++)
                //    fprintf(stderr, "%d %d %d %10.10g\n", count, p, block_type, rawout[p]); // DEBUG
                // count++;

                // Overlap
                for (i = 0; i < 18; i++) {
                    out[ch][i][block] = rawout[i] + *curr_gr;
                    *curr_gr++ = rawout[i + 18];
                    // fprintf(stderr, "%10.10g\n", out[gr][ch][i][block]); // DEBUG
                }
                if (block % 2) {
                    //Multiply every other sample by -1
                    out[ch][1][block] = -out[ch][1][block];
                    out[ch][3][block] = -out[ch][3][block];
                    out[ch][5][block] = -out[ch][5][block];
                    out[ch][7][block] = -out[ch][7][block];
                    out[ch][9][block] = -out[ch][9][block];
                    out[ch][11][block] = -out[ch][11][block];
                    out[ch][13][block] = -out[ch][13][block];
                    out[ch][15][block] = -out[ch][15][block];
                    out[ch][17][block] = -out[ch][17][block];
                }
            }                   // for block = 0 .. 32

        }                       // for each channel

        for (sb = 0; sb < 18; sb++) {
            SubBandSynthesis(v, &(out[0][sb][0]), &curr_left_pcm);
            SubBandSynthesis(v, &(out[1][sb][0]), &curr_right_pcm);
        }

    }                           // for each granule

}


static void
ReadScaleFactor(bp, data, gr, ch)
    BitParser *bp;
    MpegAudioL3 *data;
    int gr, ch;
{
    // based on iis decode.c

    int num_of_bits;
    int sfb;
    int i;

    if (data->block_split_flag[gr][ch] && data->block_type[gr][ch] == 2) {
        if (data->switch_point[gr][ch]) {
            num_of_bits = slen[0][(int) data->scalefac_compress[gr][ch]];
            for (sfb = 0; sfb < 8; sfb++) {
                Bp_GetBits(bp, num_of_bits, data->scale_fac_l[gr][ch][sfb]);
            }
            for (sfb = 3; sfb < 6; sfb++) {
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][0]);
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][1]);
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][2]);
            }
            num_of_bits = slen[1][(int) data->scalefac_compress[gr][ch]];
            for (sfb = 6; sfb < 12; sfb++) {
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][0]);
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][1]);
                Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][2]);
            }
            data->scale_fac_s[gr][ch][12][0] = 0;
            data->scale_fac_s[gr][ch][12][1] = 0;
            data->scale_fac_s[gr][ch][12][2] = 0;
        } else {
            int sfbtable_s[3] =
            {0, 6, 12};

            for (i = 0; i < 2; i++) {
                num_of_bits = slen[i][(int) data->scalefac_compress[gr][ch]];
                for (sfb = sfbtable_s[i]; sfb < sfbtable_s[i + 1]; sfb++) {
                    Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][0]);
                    Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][1]);
                    Bp_GetBits(bp, num_of_bits, data->scale_fac_s[gr][ch][sfb][2]);
                }
            }
            data->scale_fac_s[gr][ch][12][0] = 0;
            data->scale_fac_s[gr][ch][12][1] = 0;
            data->scale_fac_s[gr][ch][12][2] = 0;
        }
    } else {
        int sfbtable_l[5] =
        {0, 6, 11, 16, 21};

        for (i = 0; i < 4; i++) {
            if ((data->scfsi[i][ch] == 0) || (gr == 0)) {
                num_of_bits = slen[(i < 2) ? 0 : 1][(int) data->scalefac_compress[gr][ch]];
                for (sfb = sfbtable_l[i]; sfb < sfbtable_l[i + 1]; sfb++) {
                    Bp_GetBits(bp, num_of_bits, data->scale_fac_l[gr][ch][sfb]);
                }
            }
        }
        data->scale_fac_l[gr][ch][22] = 0;
    }
}


int
DecodeHuffmanWord(bp, curr_table, x, y, v, w, bits_read, len)
    BitParser *bp;
    int curr_table;
    int *x, *y, *v, *w;
    int *bits_read;
    int len;
{
    HuffmanCodeTable *h;
    register unsigned long level;
    register int point;
    register int code;
    register int error;

    h = &huffman_table[curr_table];
    point = 0;
    level = 1 << (sizeof(long) * 8 - 1);

    if (h->table_num == 0) {
        *x = *y = 0;
        return 0;
    }
    if (h->val == NULL)
        return 2;

    do {

        if (h->val[point][0] == 0) {
            *x = h->val[point][1] >> 4;
            *y = h->val[point][1] & 0xF;
            error = 0;
            break;
        }
        Bp_GetBits(bp, 1, code);
        (*bits_read)++;
        if (code) {
            while (h->val[point][1] >= MXOFF)
                point += h->val[point][1];
            point += h->val[point][1];
        } else {
            while (h->val[point][0] >= MXOFF)
                point += h->val[point][0];
            point += h->val[point][0];
        }
        level >>= 1;

    } while (level || (point < (int) huffman_table->treelen));  // sure is not h ?

    if ((h->table_num == 32) || (h->table_num == 33)) {
        *v = (*y >> 3) & 1;
        *w = (*y >> 2) & 1;
        *x = (*y >> 1) & 1;
        *y = *y & 1;

        if (*v) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *v = -*v;
        }
        if (*w) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *w = -*w;
        }
        if (*x) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *x = -*x;
        }
        if (*y) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *y = -*y;
        }
    } else {
        if (h->linbits) {
            if ((int) h->xlen - 1 == *x) {
                Bp_GetBits(bp, h->linbits, code);
                (*bits_read) += h->linbits;
                *x += code;
            }
        }
        if (*x) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *x = -*x;
        }
        if (h->linbits) {
            if ((int) h->ylen - 1 == *y) {
                Bp_GetBits(bp, h->linbits, code);
                (*bits_read) += h->linbits;
                *y += code;
            }
        }
        if (*y) {
            Bp_GetBits(bp, 1, code);
            (*bits_read)++;
            if (code)
                *y = -*y;
        }
    }
    return 1;                   // <<-- check this.

}


static void
ReadHuffmanCodebits(bp, hdr, data, gr, ch)
    BitParser *bp;
    MpegAudioHdr *hdr;
    MpegAudioL3 *data;
    int gr, ch;
{
    int bits_read;
    int i;
    int len, slen1, slen2;
    int region1_start, region2_start;
    int curr_table;
    int x, y, v, w;


    slen1 = slen[0][(int) data->scalefac_compress[gr][ch]];
    slen2 = slen[1][(int) data->scalefac_compress[gr][ch]];
    len =
        (data->block_type[gr][ch] == 2) ?
        ((data->switch_point[gr][ch] == 0) ?
        18 * (slen1 + slen2) :
        17 * slen1 + 18 * slen2) :
        11 * slen1 + 10 * slen2;
    len = data->part2_3_length[gr][ch] - len;
    bits_read = 0;
    // stop reading when bits_read >= len

    if (data->block_type[gr][ch] == 2) {
        region1_start = 36;
        region2_start = 576;
    } else {
        region1_start = sfb_table_long
            [(int) hdr->sampling_rate_index][1]
            [data->region_address1[gr][ch] + 1];
        region2_start = sfb_table_long
            [(int) hdr->sampling_rate_index][1]
            [data->region_address1[gr][ch] + data->region_address2[gr][ch] + 2];
    }

    // Big values area

    for (i = 0; i < data->big_values[gr][ch] * 2; i += 2) {
        if (i < region1_start) {
            curr_table = data->table_select[0][gr][ch];
        } else if (i < region2_start) {
            curr_table = data->table_select[1][gr][ch];
        } else {
            curr_table = data->table_select[2][gr][ch];
        }
        DecodeHuffmanWord(bp, curr_table, &x, &y, &v, &w, &bits_read, len);
        data->freq_lines[gr][ch][i] = x;
        data->freq_lines[gr][ch][i + 1] = y;
    }

    // count 1 area

    curr_table = data->countltable_table[gr][ch] + 32;
    while (bits_read < len && i < 576) {
        DecodeHuffmanWord(bp, curr_table, &x, &y, &v, &w, &bits_read, len);
        data->freq_lines[gr][ch][i] = v;
        data->freq_lines[gr][ch][i + 1] = w;
        data->freq_lines[gr][ch][i + 2] = x;
        data->freq_lines[gr][ch][i + 3] = y;
        i += 4;
    }

    if (bits_read > len) {
        i -= 4;
        Bp_RestoreAnyBits(bp, bits_read - len);
    }
    // stuffing bits

    {
        int bits_remain = len - bits_read;
        int short_remain = bits_remain / 16;

        if (short_remain > 0) {
            DO_N_TIMES(short_remain,
                Bp_FlushBits(bp, 16);
                );
        }
        bits_remain = bits_remain % 16;
        Bp_FlushBits(bp, bits_remain);
    }

    if (i < 576) {
        data->zero_freq_start[gr][ch] = i;
    } else {
        data->zero_freq_start[gr][ch] = 576;
    }

    for (; i < 576; i++) {
        data->freq_lines[gr][ch][i] = 0;
    }
}



void
MpegAudioL3Parse(bp, abp, hdr, data)
    BitParser *bp;
    BitParser *abp;
    MpegAudioHdr *hdr;
    MpegAudioL3 *data;
{
    int ch, gr, scfsi_band, region, window;
    int num_of_channel;
    int num_of_slots;
    int num_of_discards;
    int lastBytePos;

    // static int init = 0;
    // if (!init) { InitTables(); init = 1;}
    // table for decoding

    num_of_channel = hdr->mode == MPEG_AUDIO_SINGLE_CHANNEL ? 1 : 2;


    Bp_GetBits(bp, 9, data->main_data_end);
    if (num_of_channel == 1) {
        Bp_GetBits(bp, 5, data->private_bits);
    } else {
        Bp_GetBits(bp, 3, data->private_bits);
    }

    for (ch = 0; ch < num_of_channel; ch++)
        for (scfsi_band = 0; scfsi_band < 4; scfsi_band++)
            Bp_GetBits(bp, 1, data->scfsi[scfsi_band][ch]);

    for (gr = 0; gr < 2; gr++)
        for (ch = 0; ch < num_of_channel; ch++) {
            Bp_GetBits(bp, 12, data->part2_3_length[gr][ch]);
            Bp_GetBits(bp, 9, data->big_values[gr][ch]);
            Bp_GetBits(bp, 8, data->global_gain[gr][ch]);
            Bp_GetBits(bp, 4, data->scalefac_compress[gr][ch]);
            Bp_GetBits(bp, 1, data->block_split_flag[gr][ch]);
            if (data->block_split_flag[gr][ch]) {
                Bp_GetBits(bp, 2, data->block_type[gr][ch]);
                Bp_GetBits(bp, 1, data->switch_point[gr][ch]);
                for (region = 0; region < 2; region++)
                    Bp_GetBits(bp, 5, data->table_select[region][gr][ch]);
                for (window = 0; window < 3; window++)
                    Bp_GetBits(bp, 3, data->subblock_gain[window][gr][ch]);
                if (data->block_type[gr][ch] == 2 && data->switch_point[gr][ch] == 0)
                    data->region_address1[gr][ch] = 8;
                else
                    data->region_address1[gr][ch] = 7;
                data->region_address2[gr][ch] = 20 - data->region_address1[gr][ch];
            } else {
                for (region = 0; region < 3; region++)
                    Bp_GetBits(bp, 5, data->table_select[region][gr][ch]);
                Bp_GetBits(bp, 4, data->region_address1[gr][ch]);
                Bp_GetBits(bp, 3, data->region_address2[gr][ch]);
                data->block_type[gr][ch] = 0;
            }
            Bp_GetBits(bp, 1, data->preflag[gr][ch]);
            Bp_GetBits(bp, 1, data->scalefac_scale[gr][ch]);
            Bp_GetBits(bp, 1, data->countltable_table[gr][ch]);
        }

    // calculate number of slots (from mpeg1_iis/decode.c)

    num_of_slots = (int) ((144 * hdr->bit_rate) / hdr->sampling_rate);
    if (hdr->padding_bit)
        num_of_slots++;
    num_of_slots -= 4;
    if (!hdr->protection_bit)
        num_of_slots -= 2;
    if (num_of_channel == 1)
        num_of_slots -= 17;
    else
        num_of_slots -= 32;

    // shift the buffer so that first byte is at position 0
    Bp_ByteAlign(bp);
    BitStreamShift(abp->bs, BitParserTell(abp));
    lastBytePos = abp->bs->endDataPtr - abp->bs->buffer;
    BitParserSeek(abp, lastBytePos);
    Bp_MoveBytes(bp, abp, num_of_slots);
    BitParserSeek(abp, 0);

    /*
       if (buf->lastByte + num_of_slots >= 4096 ) 
       fprintf(stderr, "danger. danger. buffer overflow.\n");

       currbuf = &(buf->buffer[buf->lastByte]);
       if (num_of_slots != 0)
       DO_N_TIMES(num_of_slots,
       Bp_GetBits(bp, 8, code);
       *currbuf++ = code;
       );

       BitParserWrap(bp, bs->buf);
     */

    num_of_discards = lastBytePos - data->main_data_end;
    if (num_of_discards < 0) {
        fprintf(stderr, "not enough data to decode\n");
    } else if (num_of_discards != 0)
        Bp_FlushBytes(abp, num_of_discards);

    /*
       DO_N_TIMES(num_of_discards,
       Bp_GetBits(bp, 8, code);
       );
     */
    // Main Data (finally)
    for (gr = 0; gr < 2; gr++) {
        for (ch = 0; ch < num_of_channel; ch++) {
            ReadScaleFactor(abp, data, gr, ch);
            ReadHuffmanCodebits(abp, hdr, data, gr, ch);
        }
    }

    Bp_ByteAlign(abp);
    Bp_ByteAlign(bp);
    // buf->lastByte += num_of_slots;
    // buf->firstByte = sizeof(short)*(bp->offsetPtr - bp->bs->buf) - (bp->bitCount/8);
    // Bp_Free(bp);
}

/*
   int InitTables()
   {
   #define MY_PI 3.14159265358979323486

   int i,p,m;

   for (i = 0; i < 400; i++) {
   power_table[i] = pow(2.0, (double)i/4.0);
   power_table2[i] = pow(2.0, (double)i/-4.0);
   }
   for (i = 0; i < 8206; i++)
   base_table[i] = pow(i, (double)4/3);

   for (p = 0; p < 12; p++) 
   for (m = 0; m < 6; m++) 
   cos_table_long[p][m] = cos (MY_PI/24 * (2*p+7)*(2*m+1));
   for (p = 0; p < 36; p++) 
   for (m = 0; m < 18; m++) 
   cos_table_short[p][m] = cos (MY_PI/72 * (((2*p+19) * (2*m+1)) % 144));
   for (i = 0; i < 36; i++)
   sin_window[0][i] = sin (MY_PI / 36.0 * (i + 0.5));

   for (i = 0; i < 18; i++)
   sin_window[1][i] = sin (MY_PI / 36.0 * (i + 0.5));
   for (i = 18; i < 24; i++)
   sin_window[1][i] = 1.0;
   for (i = 24; i < 30; i++)
   sin_window[1][i] = sin (MY_PI / 12.0 * (i + 0.5 - 18));
   for (i = 30; i < 36; i++)
   sin_window[1][i] = 0.0;

   for (i = 0; i < 12; i++)
   sin_window[2][i] = sin (MY_PI / 12.0 * (i + 0.5));
   for (i = 12; i < 36; i++)
   sin_window[2][i] = 0.0;

   for (i = 0; i < 6; i++)
   sin_window[3][i] = 0.0;
   for (i = 6; i < 12; i++)
   sin_window[3][i] = sin (MY_PI / 12.0 * (i + 0.5 - 6));
   for (i = 12; i < 18; i++)
   sin_window[3][i] = 1.0;
   for (i = 18; i < 36; i++)
   sin_window[3][i] = sin (MY_PI / 36.0 * (i + 0.5));

   for (i = 0; i < 64; i++)
   for (m = 0; m < 32; m++)  {
   if ((filter_table[i][m] = 
   1e9*cos((double)((MY_PI/64*i + MY_PI/4)*(2*m + 1)))))
   modf (filter_table[i][m] + 0.5, &filter_table[i][m]);
   else
   modf (filter_table[i][m] - 0.5, &filter_table[i][m]);
   filter_table[i][m] *= 1e-9;
   }
   for (i = 0; i < 16; i++) {
   tan12[i] = tan(i*MY_PI/12.0);
   tan12A[i] = tan12[i]/(1+tan12[i]);
   tan12B[i] = 1.0/(1.0 + tan12[i]);
   }
   return 0;
   }
 */
