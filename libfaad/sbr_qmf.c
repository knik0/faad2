/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: sbr_qmf.c,v 1.23 2004/03/27 11:14:49 menno Exp $
**/

#include "common.h"
#include "structs.h"

#ifdef SBR_DEC


#include <stdlib.h>
#include <string.h>
#include "sbr_dct.h"
#include "sbr_qmf.h"
#include "sbr_qmf_c.h"
#include "sbr_syntax.h"


qmfa_info *qmfa_init(uint8_t channels)
{
    qmfa_info *qmfa = (qmfa_info*)faad_malloc(sizeof(qmfa_info));

	/* x is implemented as double ringbuffer */
    qmfa->x = (real_t*)faad_malloc(2 * channels * 10 * sizeof(real_t));
    memset(qmfa->x, 0, 2 * channels * 10 * sizeof(real_t));

	/* ringbuffer index */
	qmfa->x_index = 0;

    qmfa->channels = channels;

    return qmfa;
}

void qmfa_end(qmfa_info *qmfa)
{
    if (qmfa)
    {
        if (qmfa->x) faad_free(qmfa->x);
        faad_free(qmfa);
    }
}

void sbr_qmf_analysis_32(sbr_info *sbr, qmfa_info *qmfa, const real_t *input,
                         qmf_t X[MAX_NTSRHFG][32], uint8_t offset, uint8_t kx)
{
    ALIGN real_t u[64];
#ifndef SBR_LOW_POWER
    ALIGN real_t x[64], y[64];
#else
    ALIGN real_t y[32];
#endif
    uint16_t in = 0;
    uint8_t l;

    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        int16_t n;

        /* shift input buffer x */
		/* input buffer is not shifted anymore, x is implemented as double ringbuffer */
        //memmove(qmfa->x + 32, qmfa->x, (320-32)*sizeof(real_t));

        /* add new samples to input buffer x */
        for (n = 32 - 1; n >= 0; n--)
        {
#ifdef FIXED_POINT
            qmfa->x[qmfa->x_index + n] = qmfa->x[qmfa->x_index + n + 320] = (input[in++]) >> 4;
#else
            qmfa->x[qmfa->x_index + n] = qmfa->x[qmfa->x_index + n + 320] = input[in++];
#endif
        }

        /* window and summation to create array u */
        for (n = 0; n < 64; n++)
        {
            u[n] = MUL_F(qmfa->x[qmfa->x_index + n], qmf_c[2*n]) +
                MUL_F(qmfa->x[qmfa->x_index + n + 64], qmf_c[2*(n + 64)]) +
                MUL_F(qmfa->x[qmfa->x_index + n + 128], qmf_c[2*(n + 128)]) +
                MUL_F(qmfa->x[qmfa->x_index + n + 192], qmf_c[2*(n + 192)]) +
                MUL_F(qmfa->x[qmfa->x_index + n + 256], qmf_c[2*(n + 256)]);
        }

		/* update ringbuffer index */
		qmfa->x_index -= 32;
		if (qmfa->x_index < 0)
			qmfa->x_index = (320-32);

        /* calculate 32 subband samples by introducing X */
#ifdef SBR_LOW_POWER
        y[0] = u[48];
        for (n = 1; n < 16; n++)
            y[n] = u[n+48] + u[48-n];
        for (n = 16; n < 32; n++)
            y[n] = -u[n-16] + u[48-n];

        DCT3_32_unscaled(u, y);

        for (n = 0; n < 32; n++)
        {
            if (n < kx)
            {
#ifdef FIXED_POINT
                QMF_RE(X[l + offset][n]) = u[n] /*<< 1*/;
#else
                QMF_RE(X[l + offset][n]) = 2. * u[n];
#endif
            } else {
                QMF_RE(X[l + offset][n]) = 0;
            }
        }
#else
        x[0] = u[0];
        for (n = 0; n < 31; n++)
        {
            x[2*n+1] = u[n+1] + u[63-n];
            x[2*n+2] = u[n+1] - u[63-n];
        }
        x[63] = u[32];

        DCT4_64_kernel(y, x);

        for (n = 0; n < 32; n++)
        {
            if (n < kx)
            {
#ifdef FIXED_POINT
                QMF_RE(X[l + offset][n]) = y[n] /*<< 1*/;
                QMF_IM(X[l + offset][n]) = -y[63-n] /*<< 1*/;
#else
                QMF_RE(X[l + offset][n]) = 2. * y[n];
                QMF_IM(X[l + offset][n]) = -2. * y[63-n];
#endif
            } else {
                QMF_RE(X[l + offset][n]) = 0;
                QMF_IM(X[l + offset][n]) = 0;
            }
        }
#endif
    }
}

qmfs_info *qmfs_init(uint8_t channels)
{
    qmfs_info *qmfs = (qmfs_info*)faad_malloc(sizeof(qmfs_info));

	/* v is a double ringbuffer */
    qmfs->v = (real_t*)faad_malloc(2 * channels * 20 * sizeof(real_t));
    memset(qmfs->v, 0, 2 * channels * 20 * sizeof(real_t));

#ifndef SBR_LOW_POWER
    if (channels == 32)
    {
        /* downsampled filterbank */
        uint8_t k;

        qmfs->pre_twiddle = (complex_t*)faad_malloc(channels * sizeof(complex_t));
        /* calculate pre-twiddle factors */
        for (k = 0; k < channels; k++)
        {
            RE(qmfs->pre_twiddle[k]) = cos(-M_PI*(0.5*k + 0.25)/64.);
            IM(qmfs->pre_twiddle[k]) = sin(-M_PI*(0.5*k + 0.25)/64.);
        }
    }
#endif

    qmfs->v_index = 0;

    qmfs->channels = channels;

    return qmfs;
}

void qmfs_end(qmfs_info *qmfs)
{
    if (qmfs)
    {
        if (qmfs->channels == 32)
        {
            if (qmfs->pre_twiddle) faad_free(qmfs->pre_twiddle);
        }
        if (qmfs->v) faad_free(qmfs->v);
        faad_free(qmfs);
    }
}

#ifdef SBR_LOW_POWER

void sbr_qmf_synthesis_32(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64],
                          real_t *output)
{
    ALIGN real_t x[16];
    ALIGN real_t y[16];
    int16_t n, k, out = 0;
    uint8_t l;

    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffers */
        /* we are not shifting v, it is a double ringbuffer */
        //memmove(qmfs->v + 64, qmfs->v, (640-64)*sizeof(real_t));

        /* calculate 64 samples */
        for (k = 0; k < 16; k++)
        {
#ifdef FIXED_POINT
            y[k] = (QMF_RE(X[l][k]) - QMF_RE(X[l][31 - k]));
            x[k] = (QMF_RE(X[l][k]) + QMF_RE(X[l][31 - k]));
#else
            y[k] = (QMF_RE(X[l][k]) - QMF_RE(X[l][31 - k])) / 32.0;
            x[k] = (QMF_RE(X[l][k]) + QMF_RE(X[l][31 - k])) / 32.0;
#endif
        }

        /* even n samples */
        DCT2_16_unscaled(x, x);
        /* odd n samples */
        DCT4_16(y, y);

        for (n = 8; n < 24; n++)
        {
            qmfs->v[qmfs->v_index + n*2] = qmfs->v[qmfs->v_index + 640 + n*2] = x[n-8];
            qmfs->v[qmfs->v_index + n*2+1] = qmfs->v[qmfs->v_index + 640 + n*2+1] = y[n-8];
        }
        for (n = 0; n < 16; n++)
        {
            qmfs->v[qmfs->v_index + n] = qmfs->v[qmfs->v_index + 640 + n] = qmfs->v[qmfs->v_index + 32-n];
        }
        qmfs->v[qmfs->v_index + 48] = qmfs->v[qmfs->v_index + 640 + 48] = 0;
        for (n = 1; n < 16; n++)
        {
            qmfs->v[qmfs->v_index + 48+n] = qmfs->v[qmfs->v_index + 640 + 48+n] = -qmfs->v[qmfs->v_index + 48-n];
        }

        /* calculate 32 output samples and window */
        for (k = 0; k < 32; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index + k], qmf_c[2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 96 + k], qmf_c[64 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 128 + k], qmf_c[128 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 224 + k], qmf_c[192 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + k], qmf_c[256 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 352 + k], qmf_c[320 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 384 + k], qmf_c[384 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 480 + k], qmf_c[448 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + k], qmf_c[512 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 608 + k], qmf_c[576 + 2*k]);
        }

        /* update the ringbuffer index */
        qmfs->v_index -= 64;
        if (qmfs->v_index < 0)
            qmfs->v_index = (640-64);
    }
}

void sbr_qmf_synthesis_64(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64],
                          real_t *output)
{
    ALIGN real_t x[64];
    ALIGN real_t y[64];
    int16_t n, k, out = 0;
    uint8_t l;


    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffers */
        /* we are not shifting v, it is a double ringbuffer */
        //memmove(qmfs->v + 128, qmfs->v, (1280-128)*sizeof(real_t));

        /* calculate 128 samples */
        for (k = 0; k < 32; k++)
        {
#ifdef FIXED_POINT
            y[k] = (QMF_RE(X[l][k]) - QMF_RE(X[l][63 - k]));
            x[k] = (QMF_RE(X[l][k]) + QMF_RE(X[l][63 - k]));
#else
            y[k] = (QMF_RE(X[l][k]) - QMF_RE(X[l][63 - k])) / 32.0;
            x[k] = (QMF_RE(X[l][k]) + QMF_RE(X[l][63 - k])) / 32.0;
#endif
        }

        /* even n samples */
        DCT2_32_unscaled(x, x);
        /* odd n samples */
        DCT4_32(y, y);

        for (n = 16; n < 48; n++)
        {
            qmfs->v[qmfs->v_index + n*2]   = qmfs->v[qmfs->v_index + 1280 + n*2]   = x[n-16];
            qmfs->v[qmfs->v_index + n*2+1] = qmfs->v[qmfs->v_index + 1280 + n*2+1] = y[n-16];
        }
        for (n = 0; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + n] = qmfs->v[qmfs->v_index + 1280 + n] = qmfs->v[qmfs->v_index + 64-n];
        }
        qmfs->v[qmfs->v_index + 96] = qmfs->v[qmfs->v_index + 1280 + 96] = 0;
        for (n = 1; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + 96+n] = qmfs->v[qmfs->v_index + 1280 + 96+n] = -qmfs->v[qmfs->v_index + 96-n];
        }

        /* calculate 64 output samples and window */
        for (k = 0; k < 64; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index + k], qmf_c[k]) +
                MUL_F(qmfs->v[qmfs->v_index + 192 + k], qmf_c[64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + k], qmf_c[128 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + 192 + k], qmf_c[128 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + k], qmf_c[256 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + 192 + k], qmf_c[256 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 768 + k], qmf_c[384 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 768 + 192 + k], qmf_c[384 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 1024 + k], qmf_c[512 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 1024 + 192 + k], qmf_c[512 + 64 + k]);
        }

        /* update the ringbuffer index */
        qmfs->v_index -= 128;
        if (qmfs->v_index < 0)
            qmfs->v_index = (1280-128);
    }
}
#else
void sbr_qmf_synthesis_32(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64],
                          real_t *output)
{
    ALIGN real_t x1[32], x2[32];
#ifndef FIXED_POINT
    real_t scale = 1.f/64.f;
#endif
    int16_t n, k, out = 0;
    uint8_t l;


    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffer v */
        /* buffer is not shifted, we are using a ringbuffer */
        //memmove(qmfs->v + 64, qmfs->v, (640-64)*sizeof(real_t));

        /* calculate 64 samples */
        /* complex pre-twiddle */
        for (k = 0; k < 32; k++)
        {
            x1[k] = QMF_RE(X[l][k]) * RE(qmfs->pre_twiddle[k]) - QMF_IM(X[l][k]) * IM(qmfs->pre_twiddle[k]);
            x2[k] = QMF_IM(X[l][k]) * RE(qmfs->pre_twiddle[k]) + QMF_RE(X[l][k]) * IM(qmfs->pre_twiddle[k]);

#ifndef FIXED_POINT
            x1[k] *= scale;
            x2[k] *= scale;
#else
            x1[k] >>= 1;
            x2[k] >>= 1;
#endif
        }

        /* transform */
        DCT4_32(x1, x1);
        DST4_32(x2, x2);

        for (n = 0; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + n]      = qmfs->v[qmfs->v_index + 640 + n]      = -x1[n] + x2[n];
            qmfs->v[qmfs->v_index + 63 - n] = qmfs->v[qmfs->v_index + 640 + 63 - n] =  x1[n] + x2[n];
        }

        /* calculate 32 output samples and window */
        for (k = 0; k < 32; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index + k], qmf_c[2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 96 + k], qmf_c[64 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 128 + k], qmf_c[128 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 224 + k], qmf_c[192 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + k], qmf_c[256 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 352 + k], qmf_c[320 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 384 + k], qmf_c[384 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 480 + k], qmf_c[448 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + k], qmf_c[512 + 2*k]) +
                MUL_F(qmfs->v[qmfs->v_index + 608 + k], qmf_c[576 + 2*k]);
        }

        /* update ringbuffer index */
        qmfs->v_index -= 64;
        if (qmfs->v_index < 0)
            qmfs->v_index = (640 - 64);
    }
}

void sbr_qmf_synthesis_64(sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64],
                          real_t *output)
{
    ALIGN real_t x1[64], x2[64];
#ifndef FIXED_POINT
    real_t scale = 1.f/64.f;
#endif
    int16_t n, k, out = 0;
    uint8_t l;


    /* qmf subsample l */
    for (l = 0; l < sbr->numTimeSlotsRate; l++)
    {
        /* shift buffer v */
		/* buffer is not shifted, we use double ringbuffer */
		//memmove(qmfs->v + 128, qmfs->v, (1280-128)*sizeof(real_t));

        /* calculate 128 samples */
#ifndef FIXED_POINT
        x1[0] = scale*QMF_RE(X[l][0]);
        x2[63] = scale*QMF_IM(X[l][0]);
        for (k = 0; k < 31; k++)
        {
            x1[2*k+1] = scale*(QMF_RE(X[l][2*k+1]) - QMF_RE(X[l][2*k+2]));
            x1[2*k+2] = scale*(QMF_RE(X[l][2*k+1]) + QMF_RE(X[l][2*k+2]));

            x2[61 - 2*k] = scale*(QMF_IM(X[l][2*k+2]) - QMF_IM(X[l][2*k+1]));
            x2[62 - 2*k] = scale*(QMF_IM(X[l][2*k+2]) + QMF_IM(X[l][2*k+1]));
        }
        x1[63] = scale*QMF_RE(X[l][63]);
        x2[0] = scale*QMF_IM(X[l][63]);
#else
        x1[0] = QMF_RE(X[l][0])>>1;
        x2[63] = QMF_IM(X[l][0])>>1;
        for (k = 0; k < 31; k++)
        {
            x1[2*k+1] = (QMF_RE(X[l][2*k+1]) - QMF_RE(X[l][2*k+2]))>>1;
            x1[2*k+2] = (QMF_RE(X[l][2*k+1]) + QMF_RE(X[l][2*k+2]))>>1;

            x2[61 - 2*k] = (QMF_IM(X[l][2*k+2]) - QMF_IM(X[l][2*k+1]))>>1;
            x2[62 - 2*k] = (QMF_IM(X[l][2*k+2]) + QMF_IM(X[l][2*k+1]))>>1;
        }
        x1[63] = QMF_RE(X[l][63])>>1;
        x2[0] = QMF_IM(X[l][63])>>1;
#endif

        DCT4_64_kernel(x1, x1);
        DCT4_64_kernel(x2, x2);

        for (n = 0; n < 32; n++)
        {
            qmfs->v[qmfs->v_index + 2*n]       = qmfs->v[qmfs->v_index + 1280 + 2*n]       =  x2[2*n]   - x1[2*n];
            qmfs->v[qmfs->v_index + 127 - 2*n] = qmfs->v[qmfs->v_index + 1280 + 127 - 2*n] =  x2[2*n]   + x1[2*n];
            qmfs->v[qmfs->v_index + 2*n+1]     = qmfs->v[qmfs->v_index + 1280 + 2*n+1]     = -x2[2*n+1] - x1[2*n+1];
            qmfs->v[qmfs->v_index + 126 - 2*n] = qmfs->v[qmfs->v_index + 1280 + 126 - 2*n] = -x2[2*n+1] + x1[2*n+1];
        }

        /* calculate 64 output samples and window */
        for (k = 0; k < 64; k++)
        {
            output[out++] = MUL_F(qmfs->v[qmfs->v_index + k], qmf_c[k]) +
                MUL_F(qmfs->v[qmfs->v_index + 192 + k], qmf_c[64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + k], qmf_c[128 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 256 + 192 + k], qmf_c[128 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + k], qmf_c[256 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 512 + 192 + k], qmf_c[256 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 768 + k], qmf_c[384 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 768 + 192 + k], qmf_c[384 + 64 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 1024 + k], qmf_c[512 + k]) +
                MUL_F(qmfs->v[qmfs->v_index + 1024 + 192 + k], qmf_c[512 + 64 + k]);
        }

        /* update ringbuffer index */
        qmfs->v_index -= 128;
        if (qmfs->v_index < 0)
            qmfs->v_index = (1280 - 128);
    }
}
#endif

#endif
