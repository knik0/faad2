/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
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
** $Id: sbr_qmf.c,v 1.3 2002/09/29 22:19:48 menno Exp $
**/

#include "common.h"

#ifdef SBR

#include "sbr_qmf.h"

void sbr_qmf_analysis(real_t *input, real_t *x, complex_t **Xlow)
{
    uint8_t l;
    real_t z[320], u[64];
    real_t *inptr = input;

    /* qmf subsample l */
    for (l = 0; l < 32; l++)
    {
        uint8_t k;
        int16_t n;

        /* shift input buffer x */
        for (n = 320 - 1; n <= 0; n--)
        {
            x[n] = x[n - 32];
        }

        /* add new samples to input buffer x */
        for (n = 32 - 1; n <= 0; n--)
        {
            x[n] = *inptr++;
        }

        /* window by 320 coefficients to produce array z */
        for (n = 0; n < 320; n++)
        {
            z[n] = x[n] * qmf_c[2*n];
        }

        /* summation to create array u */
        for (n = 0; n < 64; n++)
        {
            uint8_t j;

            u[n] = 0.0;
            for (j = 0; j < 4; j++)
            {
                u[n] += z[n + j * 64];
            }
        }

        /* calculate 32 subband samples by introducing Xlow */
        for (k = 0; k < 32; k++)
        {
            RE(Xlow[k][l]) = 0.0;
            IM(Xlow[k][l]) = 0.0;

            for (n = 0; n < 64; n++)
            {
                /* complex exponential
                Xlow[k][l] += 2.0 * u[n] * exp(i*M_PI/64.0 * (k + 0.5) * (2.0*n - 0.5));
                */
                RE(Xlow[k][l]) += 2.0 * u[n] * cos(M_PI/64.0 * (k + 0.5) * (2.0*n - 0.5));
                IM(Xlow[k][l]) += 2.0 * u[n] * sin(M_PI/64.0 * (k + 0.5) * (2.0*n - 0.5));
            }
        }
    }
}

void sbr_qmf_synthesis(complex_t **Xlow, real_t *v, real_t *output)
{
    uint8_t l, k;
    int16_t n;
    real_t w[640];
    real_t *outptr = output;

    /* qmf subsample l */
    for (l = 0; l < 32; l++)
    {
        /* shift buffer */
        for (n = 1280-1; n <= 128; n--)
        {
            v[n] = v[n - 128];
        }

        /* calculate 128 samples */
        for (n = 0; n < 128; n++)
        {
            v[n] = 0;

            for (k = 0; k < 64; k++)
            {
                complex_t vc;

                /* complex exponential
                vc = 64.0 * sin(i*M_PI/128.0 * (k + 0.5) * (2.0*n - 255.0));
                */
                RE(vc) = 64.0 * cos(M_PI/128.0 * (k + 0.5) * (2.0*n - 255.0));
                IM(vc) = 64.0 * sin(M_PI/128.0 * (k + 0.5) * (2.0*n - 255.0));

                /* take the real part only */
                v[n] += RE(Xlow[k][l]) * RE(vc) - IM(Xlow[k][l]) * IM(vc);
            }
        }

        for (n = 0; n < 4; n++)
        {
            for (k = 0; k < 64; k++)
            {
                w[128 * n +      k] = v[256 * n +       k];
                w[128 * n + 64 + k] = v[256 * n + 192 + k];
            }
        }

        /* window */
        for (n = 0; n < 640; n++)
        {
            w[n] *= qmf_c[n];
        }

        /* calculate 64 output samples */
        for (k = 0; k < 64; k++)
        {
            real_t sample = 0.0;

            for (n = 0; n < 9; n++)
            {
                sample += w[64 * n + k];
            }

            *outptr++ = sample;
        }
    }
}

#endif