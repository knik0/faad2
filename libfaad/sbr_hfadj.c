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
** $Id: sbr_hfadj.c,v 1.14 2004/04/12 18:17:42 menno Exp $
**/

/* High Frequency adjustment */

#include "common.h"
#include "structs.h"

#ifdef SBR_DEC

#include "sbr_syntax.h"
#include "sbr_hfadj.h"

#include "sbr_noise.h"


/* static function declarations */
static void map_noise_data(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch);
static void map_sinusoids(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch);
static void estimate_current_envelope(sbr_info *sbr, sbr_hfadj_info *adj,
                                      qmf_t Xsbr[MAX_NTSRHFG][64], uint8_t ch);
static void calculate_gain(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch);
#ifdef SBR_LOW_POWER
static void calc_gain_groups(sbr_info *sbr, sbr_hfadj_info *adj, real_t *deg, uint8_t ch);
static void aliasing_reduction(sbr_info *sbr, sbr_hfadj_info *adj, real_t *deg, uint8_t ch);
#endif
static void hf_assembly(sbr_info *sbr, sbr_hfadj_info *adj, qmf_t Xsbr[MAX_NTSRHFG][64], uint8_t ch);


void hf_adjustment(sbr_info *sbr, qmf_t Xsbr[MAX_NTSRHFG][64]
#ifdef SBR_LOW_POWER
                   ,real_t *deg /* aliasing degree */
#endif
                   ,uint8_t ch)
{
    ALIGN sbr_hfadj_info adj = {{{0}}};

    map_noise_data(sbr, &adj, ch);
    map_sinusoids(sbr, &adj, ch);

    estimate_current_envelope(sbr, &adj, Xsbr, ch);

    calculate_gain(sbr, &adj, ch);

#ifdef SBR_LOW_POWER
    calc_gain_groups(sbr, &adj, deg, ch);
    aliasing_reduction(sbr, &adj, deg, ch);
#endif

    hf_assembly(sbr, &adj, Xsbr, ch);
}

static void map_noise_data(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch)
{
    uint8_t l, i;
    uint8_t m;

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        uint8_t k;

        /* select the noise time band k that completely holds the current envelope time band l */
        for (k = 0; k < 2; k++)
        {
            if ((sbr->t_E[ch][l] >= sbr->t_Q[ch][k]) && (sbr->t_E[ch][l+1] <= sbr->t_Q[ch][k+1]))
            {
                for (i = 0; i < sbr->N_Q; i++)
                {
                    for (m = sbr->f_table_noise[i]; m < sbr->f_table_noise[i+1]; m++)
                    {
                        adj->Q_div_mapped[l][m - sbr->kx] =
                            sbr->Q_div[ch][i][k];

                        adj->Q_div2_mapped[l][m - sbr->kx] =
                            sbr->Q_div2[ch][i][k];
                    }
                }
            }
        }
    }
}

static void map_sinusoids(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch)
{
    uint8_t l, i, m, k, k1, k2, delta_S, l_i, u_i;

    if (sbr->bs_frame_class[ch] == FIXFIX)
    {
        sbr->l_A[ch] = -1;
    } else if (sbr->bs_frame_class[ch] == VARFIX) {
        if (sbr->bs_pointer[ch] > 1)
            sbr->l_A[ch] = -1;
        else
            sbr->l_A[ch] = sbr->bs_pointer[ch] - 1;
    } else {
        if (sbr->bs_pointer[ch] == 0)
            sbr->l_A[ch] = -1;
        else
            sbr->l_A[ch] = sbr->L_E[ch] + 1 - sbr->bs_pointer[ch];
    }

    for (l = 0; l < 5; l++)
    {
        for (i = 0; i < 64; i++)
        {
            adj->S_index_mapped[l][i] = 0;
            adj->S_mapped[l][i] = 0;
        }
    }

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        for (i = 0; i < sbr->N_high; i++)
        {
            if ((l >= sbr->l_A[ch]) ||
                (sbr->bs_add_harmonic_prev[ch][i] && sbr->bs_add_harmonic_flag_prev[ch]))
            {
                /* find the middle subband of the frequency band */
                m = (sbr->f_table_res[HI_RES][i+1] + sbr->f_table_res[HI_RES][i]) >> 1;
                adj->S_index_mapped[l][m - sbr->kx] = /*delta_step **/ sbr->bs_add_harmonic[ch][i];
            }
        }
    }

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        for (i = 0; i < sbr->N_high; i++)
        {
            if (sbr->f[ch][l] == HI_RES)
            {
                k1 = i;
                k2 = i + 1;
            } else {
                for (k1 = 0; k1 < sbr->N_low; k1++)
                {
                    if ((sbr->f_table_res[HI_RES][i] >= sbr->f_table_res[LO_RES][k1]) &&
                        (sbr->f_table_res[HI_RES][i+1] <= sbr->f_table_res[LO_RES][k1+1]))
                    {
                        break;
                    }
                }
                for (k2 = 0; k2 < sbr->N_low; k2++)
                {
                    if ((sbr->f_table_res[HI_RES][i+1] >= sbr->f_table_res[LO_RES][k2]) &&
                        (sbr->f_table_res[HI_RES][i+2] <= sbr->f_table_res[LO_RES][k2+1]))
                    {
                        break;
                    }
                }
            }

            l_i = sbr->f_table_res[sbr->f[ch][l]][k1];
            u_i = sbr->f_table_res[sbr->f[ch][l]][k2];

            delta_S = 0;
            for (k = l_i; k < u_i; k++)
            {
                if (adj->S_index_mapped[l][k - sbr->kx] == 1)
                    delta_S = 1;
            }

            for (m = l_i; m < u_i; m++)
            {
                adj->S_mapped[l][m - sbr->kx] = delta_S;
            }
        }
    }
}

static uint8_t get_S_mapped(sbr_info *sbr, uint8_t ch, uint8_t l, uint8_t current_band)
{
    if (sbr->f[ch][l] == HI_RES)
    {
        /* in case of using f_table_high we just have 1 to 1 mapping
         * from bs_add_harmonic[l][k]
         */
        if ((l >= sbr->l_A[ch]) ||
            (sbr->bs_add_harmonic_prev[ch][current_band] && sbr->bs_add_harmonic_flag_prev[ch]))
        {
            return sbr->bs_add_harmonic[ch][current_band];
        }
    } else {
        uint8_t b, lb, ub;

        /* in case of f_table_low we check if any of the HI_RES bands
         * within this LO_RES band has bs_add_harmonic[l][k] turned on
         * (note that borders in the LO_RES table are also present in
         * the HI_RES table)
         */

        /* find first HI_RES band in current LO_RES band */
        lb = 2*current_band - ((sbr->N_high & 1) ? 1 : 0);
        /* find first HI_RES band in next LO_RES band */
        ub = 2*(current_band+1) - ((sbr->N_high & 1) ? 1 : 0);

        /* check all HI_RES bands in current LO_RES band for sinusoid */
        for (b = lb; b < ub; b++)
        {
            if ((l >= sbr->l_A[ch]) ||
                (sbr->bs_add_harmonic_prev[ch][b] && sbr->bs_add_harmonic_flag_prev[ch]))
            {
                if (sbr->bs_add_harmonic[ch][b] == 1)
                    return 1;
            }
        }
    }

    return 0;
}

static void estimate_current_envelope(sbr_info *sbr, sbr_hfadj_info *adj,
                                      qmf_t Xsbr[MAX_NTSRHFG][64], uint8_t ch)
{
    uint8_t m, l, j, k, k_l, k_h, p;
    real_t nrg, div;

    if (sbr->bs_interpol_freq == 1)
    {
        for (l = 0; l < sbr->L_E[ch]; l++)
        {
            uint8_t i, l_i, u_i;

            l_i = sbr->t_E[ch][l];
            u_i = sbr->t_E[ch][l+1];

            div = (real_t)(u_i - l_i);

            for (m = 0; m < sbr->M; m++)
            {
                nrg = 0;

                for (i = l_i + sbr->tHFAdj; i < u_i + sbr->tHFAdj; i++)
                {
#ifdef FIXED_POINT
#ifdef SBR_LOW_POWER
                    nrg += ((QMF_RE(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_RE(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS);
#else
                    nrg += ((QMF_RE(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_RE(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS) +
                        ((QMF_IM(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_IM(Xsbr[i][m + sbr->kx])+(1<<(REAL_BITS-1)))>>REAL_BITS);
#endif
#else
                    nrg += MUL_R(QMF_RE(Xsbr[i][m + sbr->kx]), QMF_RE(Xsbr[i][m + sbr->kx]))
#ifndef SBR_LOW_POWER
                        + MUL_R(QMF_IM(Xsbr[i][m + sbr->kx]), QMF_IM(Xsbr[i][m + sbr->kx]))
#endif
                        ;
#endif
                }

                sbr->E_curr[ch][m][l] = nrg / div;
#ifdef SBR_LOW_POWER
#ifdef FIXED_POINT
                sbr->E_curr[ch][m][l] <<= 1;
#else
                sbr->E_curr[ch][m][l] *= 2;
#endif
#endif
            }
        }
    } else {
        for (l = 0; l < sbr->L_E[ch]; l++)
        {
            for (p = 0; p < sbr->n[sbr->f[ch][l]]; p++)
            {
                k_l = sbr->f_table_res[sbr->f[ch][l]][p];
                k_h = sbr->f_table_res[sbr->f[ch][l]][p+1];

                for (k = k_l; k < k_h; k++)
                {
                    uint8_t i, l_i, u_i;
                    nrg = 0;

                    l_i = sbr->t_E[ch][l];
                    u_i = sbr->t_E[ch][l+1];

                    div = (real_t)((u_i - l_i)*(k_h - k_l));

                    for (i = l_i + sbr->tHFAdj; i < u_i + sbr->tHFAdj; i++)
                    {
                        for (j = k_l; j < k_h; j++)
                        {
#ifdef FIXED_POINT
#ifdef SBR_LOW_POWER
                            nrg += ((QMF_RE(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_RE(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS);
#else
                            nrg += ((QMF_RE(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_RE(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS) +
                                ((QMF_IM(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS)*((QMF_IM(Xsbr[i][j])+(1<<(REAL_BITS-1)))>>REAL_BITS);
#endif
#else
                            nrg += MUL_R(QMF_RE(Xsbr[i][j]), QMF_RE(Xsbr[i][j]))
#ifndef SBR_LOW_POWER
                                + MUL_R(QMF_IM(Xsbr[i][j]), QMF_IM(Xsbr[i][j]))
#endif
                                ;
#endif
                        }
                    }

                    sbr->E_curr[ch][k - sbr->kx][l] = nrg / div;
#ifdef SBR_LOW_POWER
#ifdef FIXED_POINT
                    sbr->E_curr[ch][k - sbr->kx][l] <<= 1;
#else
                    sbr->E_curr[ch][k - sbr->kx][l] *= 2;
#endif
#endif
                }
            }
        }
    }
}

#ifdef FIXED_POINT
#define step(shift) \
    if ((0x40000000l >> shift) + root <= value)       \
    {                                                 \
        value -= (0x40000000l >> shift) + root;       \
        root = (root >> 1) | (0x40000000l >> shift);  \
    } else {                                          \
        root = root >> 1;                             \
    }

/* fixed point square root approximation */
real_t sbr_sqrt(real_t value)
{
    real_t root = 0;

    step( 0); step( 2); step( 4); step( 6);
    step( 8); step(10); step(12); step(14);
    step(16); step(18); step(20); step(22);
    step(24); step(26); step(28); step(30);

    if (root < value)
        ++root;

    root <<= (REAL_BITS/2);

    return root;
}

real_t SBR_SQRT_Q2(real_t value)
{
    real_t root = 0;

    step( 0); step( 2); step( 4); step( 6);
    step( 8); step(10); step(12); step(14);
    step(16); step(18); step(20); step(22);
    step(24); step(26); step(28); step(30);

    if (root < value)
        ++root;

    root <<= (Q2_BITS/2);

    return root;
}

real_t sbr_sqrt_int(real_t value)
{
    real_t root = 0;

    step( 0); step( 2); step( 4); step( 6);
    step( 8); step(10); step(12); step(14);
    step(16); step(18); step(20); step(22);
    step(24); step(26); step(28); step(30);

    if (root < value)
        ++root;

    return root;
}
#define SBR_SQRT_FIX(A) sbr_sqrt(A)
#define SBR_SQRT_INT(A) sbr_sqrt_int(A)
#endif

#ifdef FIXED_POINT
#define EPS (1) /* smallest number available in fixed point */
#else
#define EPS (1e-12)
#endif

#ifdef FIXED_POINT
#define ONE (REAL_CONST(1)>>10)
#else
#define ONE (1)
#endif


#ifdef FIXED_POINT

uint8_t G_max_is_biggest(real_t G, real_t G_max,
                         uint8_t G_is_frac, uint8_t G_max_is_frac)
{
    if ((G_is_frac == 1 && G_max_is_frac == 1) || ((G_is_frac == 0 && G_max_is_frac == 0)))
    {
        if (G_max > G)
            return 1;
    } else if (G_is_frac) {
        if (G_max > REAL_CONST(1))
        {
            return 1;
        } else if (G_max > (G << (FRAC_BITS-REAL_BITS))) {
            return 1;
        }
    } else if (G_max_is_frac) {
        if (G > REAL_CONST(1))
        {
            return 0;
        } else if (G > (G_max >> (FRAC_BITS-REAL_BITS))) {
            return 0;
        } else {
            return 1;
        }
    }

    return 0;
}

/* frac */
real_t div_G_max_G(real_t G, real_t G_max,
                   uint8_t G_is_frac, uint8_t G_max_is_frac, uint8_t *is_real)
{
    *is_real = 0;

    if (G_is_frac == 1)
    {
        /* divide by a frac */
        return ((int64_t)G_max << FRAC_BITS)/G;
    } else {
        /* divide by a real but answer is a frac */
        if (G_max_is_frac == 1)
        {
            /* G_max is already a frac */
            if (G < REAL_CONST(1))
            {
                *is_real = 1;
                return ((int64_t)G_max >> 3)/G;
            } else {
                return ((int64_t)G_max << REAL_BITS)/G;
            }
        } else {
            /* turn G_max into a frac before dividing */
            return ((int64_t)G_max << REAL_BITS+(FRAC_BITS-REAL_BITS))/G;
        }
    }
}

static void calculate_gain(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch)
{
    uint8_t m, l, k, i;

    ALIGN real_t Q_M_lim[MAX_M];
    ALIGN real_t G_lim[MAX_M];
    ALIGN real_t G_boost;
    ALIGN real_t S_M[MAX_M];
    ALIGN uint8_t table_map_res_to_m[MAX_M];
    ALIGN uint8_t G_is_frac[MAX_M];
    ALIGN uint8_t Q_M_is_real[MAX_M];


    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        real_t delta = (l == sbr->l_A[ch] || l == sbr->prevEnvIsShort[ch]) ? 0 : 1;

        for (i = 0; i < sbr->n[sbr->f[ch][l]]; i++)
        {
            for (m = sbr->f_table_res[sbr->f[ch][l]][i]; m < sbr->f_table_res[sbr->f[ch][l]][i+1]; m++)
            {
                table_map_res_to_m[m - sbr->kx] = i;
            }
        }

        for (k = 0; k < sbr->N_L[sbr->bs_limiter_bands]; k++)
        {
            real_t G_max;
            real_t den = 0;
            real_t den_int = 0;
            real_t den_real = 0;
            real_t acc1 = 0;
            real_t acc1_coef = 0;
            real_t acct = 0;
            real_t acc2 = 0;
            uint8_t G_max_infinite = 0;
            uint8_t min_pow = 0;
            uint8_t G_max_is_frac = 0;

            uint8_t ml1, ml2;


            ml1 = sbr->f_table_lim[sbr->bs_limiter_bands][k];
            ml2 = sbr->f_table_lim[sbr->bs_limiter_bands][k+1];


            /* calculate the accumulated E_orig and E_curr over the limiter band */
            for (m = ml1; m < ml2; m++)
            {
                /* E_orig: integer */
                if (sbr->E_orig[ch][table_map_res_to_m[m]][l] > (1<<4))
                {
                    acc1 += (sbr->E_orig[ch][table_map_res_to_m[m]][l]>>4);
                } else {
                    acc1_coef += (sbr->E_orig[ch][table_map_res_to_m[m]][l] << (COEF_BITS-4));
                    if (acc1_coef > COEF_CONST(3))
                    {
                        acc1 += (acc1_coef+(1<<(COEF_BITS-1)))>>COEF_BITS;
                        acc1_coef = 0;
                    }
                }
                /* E_curr: integer */
                acc2 += sbr->E_curr[ch][m][l];
            }

            acc1 += (acc1_coef+(1<<(COEF_BITS-1)))>>COEF_BITS;


            /* G_max: fixed point */
            if ((acc2 == 0) && (acc1 > 3)) /* chosen 3 here, kind of arbitrary but it works (0 doesn't) */
            {
                G_max = 0xFFFFFFF;
                G_max_infinite = 1;
            } else if (acc2 == 0) {
                G_max = 0;
                G_max_infinite = 0;
            } else {
                G_max_infinite = 0;
                switch (sbr->bs_limiter_gains)
                {
                case 0: acct = acc1 >> 1; break;
                case 2: acct = acc1 << 1; break;
                case 3: acct = acc1; G_max_infinite = 1; break;
                default: acct = acc1; break;
                }

                if (acc2 > acct)
                {
                    G_max_is_frac = 1;
                    G_max = (((int64_t)acct)<<FRAC_BITS) / acc2;
                } else {
                    G_max_is_frac = 0;
                    G_max = (((int64_t)acct)<<REAL_BITS) / acc2;
                }
            }

            for (m = ml1; m < ml2; m++)
            {
                real_t Q_M, G;
                real_t Q_div1, Q_div2;
                real_t E_orig;
                /* set to 1 to start with */
                uint8_t G_max_biggest = 1;


                /* Q_mapped: fixed point */

                /* Q_div1: [0..1] FRAC_CONST */
                Q_div1 = adj->Q_div_mapped[l][m];
                /* Q_div2: [0..1] FRAC_CONST */
                Q_div2 = adj->Q_div2_mapped[l][m];

                /* E_orig: integer */
                E_orig = sbr->E_orig[ch][table_map_res_to_m[m]][l];

                /* Q_M: REAL */
                /* S_M: integer */
                if ((E_orig > (1<<22)) && (Q_div2 > FRAC_CONST(0.8)))
                {
                    /* Q_M will not fit in a real */
                    Q_M = ((int64_t)(E_orig>>4) * Q_div2) >> FRAC_BITS;
                    Q_M_is_real[m] = 0;

                    S_M[m] = adj->S_index_mapped[l][m] * MUL_F((E_orig>>4), Q_div1);
                } else if (E_orig > (1<<4)) {
                    Q_M = ((int64_t)(E_orig>>4) * Q_div2) >> (FRAC_BITS-REAL_BITS);
                    Q_M_is_real[m] = 1;

                    S_M[m] = adj->S_index_mapped[l][m] * MUL_F((E_orig>>4), Q_div1);
                } else {
                    Q_M = ((int64_t)E_orig * Q_div2) >> (FRAC_BITS-REAL_BITS);
                    Q_M >>= 4;
                    Q_M_is_real[m] = 1;

                    S_M[m] = adj->S_index_mapped[l][m] * MUL_F(E_orig, Q_div1);
                    S_M[m] >>= 4;
                }


                /* G: fixed point */
                if (sbr->E_curr[ch][m][l] != 0)
                {
                    /* E_curr = INT */
                    if (sbr->E_curr[ch][m][l] > (E_orig>>4))
                    {
                        /*frac*/
                        G = (((int64_t)E_orig)<<(FRAC_BITS-4)) / sbr->E_curr[ch][m][l];

                        G_is_frac[m] = 1;
                    } else {
                        /*real*/
                        G = (((int64_t)E_orig)<<(REAL_BITS-4)) / sbr->E_curr[ch][m][l];

                        G_is_frac[m] = 0;
                    }
                } else {
                    /* E_curr == 0, check if E_orig happens to be really big */
                    if (G_max_is_frac == 0)
                    {
                        if ((E_orig<<4) > (G_max>>REAL_BITS))
                        {
                            G_max_biggest = 0;
                        }
                    } else {
                        /* G_max is fractional type */
                        if (E_orig != 0)
                        {
                            G_max_biggest = 0;
                        }
                    }

                    G = (E_orig << (REAL_BITS-4+10));

                    G_is_frac[m] = 0;
                }

                if (adj->S_mapped[l][m] == 1)
                {
                    G = MUL_F(G, Q_div2);
                } else if (delta == 1) {
                    G = MUL_F(G, Q_div1);
                }

                /* limit the additional noise energy level */
                /* and apply the limiter */

                /* if we still have the default value for G_max_biggest -> compute */
                if (G_max_biggest != 0)
                    G_max_biggest = G_max_is_biggest(G, G_max, G_is_frac[m], G_max_is_frac);

                /* G_lim: fixed point */
                /* Q_M_lim: REAL/INT */
                if (G_max_infinite || G_max_biggest)
                {
                    Q_M_lim[m] = Q_M;
                    G_lim[m] = G;
                } else {
                    real_t tmp;
                    uint8_t is_real = 0;
                    if (G == 0)
                        tmp = 0xFFF;
                    else
                        tmp = div_G_max_G(G, G_max, G_is_frac[m], G_max_is_frac, &is_real);

                    if (is_real == 0)
                        Q_M_lim[m] = MUL_F(Q_M, tmp);
                    else
                        Q_M_lim[m] = MUL_R(Q_M, tmp);

                    G_lim[m] = G_max;
                    G_is_frac[m] = G_max_is_frac;
                }


                /* E_curr: integer/coef */
                if (G_is_frac[m] == 0)
                {
                    if (sbr->E_curr[ch][m][l] < (1<<(25-REAL_BITS)))
                    {
                        den_real += sbr->E_curr[ch][m][l] * G_lim[m];
                    } else {
                        den_int += MUL_R(sbr->E_curr[ch][m][l], G_lim[m]);
                    }
                } else {
                    den_int += MUL_F(sbr->E_curr[ch][m][l], G_lim[m]);
                }
                den_int += S_M[m];
                if ((!adj->S_index_mapped[l][m]) && (l != sbr->l_A[ch]))
                {
                    if (Q_M_is_real[m] == 1)
                    {
                        if (Q_M_lim[m] > REAL_CONST(100))
                            den_int += (Q_M_lim[m]>>REAL_BITS);
                        else
                            den_real += Q_M_lim[m];
                    } else {
                        den_int += Q_M_lim[m];
                    }
                }
            }

            den = den_int + ((den_real+(1<<(REAL_BITS-1)))>>REAL_BITS);

            /* G_boost: fixed point */
            if ((den + EPS) == 0 || den == 0)
                G_boost = REAL_CONST(1);//REAL_CONST(2.51188643);
            else if (acc1 > (den<<1)+(den>>1))
                G_boost = REAL_CONST(2.51188643);
            else
                G_boost = (((int64_t)(acc1 + EPS))<<REAL_BITS)/(den + EPS);
            G_boost = min(G_boost, REAL_CONST(2.51188643) /* 1.584893192 ^ 2 */);

            for (m = ml1; m < ml2; m++)
            {
                /* apply compensation to gain, noise floor sf's and sinusoid levels */
#ifndef SBR_LOW_POWER
                /* G_lim_boost: fixed point */
                if (G_is_frac[m] == 0)
                {
                    adj->G_lim_boost[l][m] = SBR_SQRT_Q2(MUL_SHIFT6(G_lim[m], G_boost));
                } else {
                    adj->G_lim_boost[l][m] = SBR_SQRT_Q2(MUL_SHIFT23(G_lim[m], G_boost));
                }
#else
                /* sqrt() will be done after the aliasing reduction to save a
                 * few multiplies
                 */
                /* G_lim_boost: fixed point */
                if (G_is_frac[m] == 0)
                {
                    adj->G_lim_boost[l][m] = MUL_SHIFT6(G_lim[m], G_boost);
                } else {
                    adj->G_lim_boost[l][m] = MUL_SHIFT23(G_lim[m], G_boost);
                }
#endif
                /* Q_M_lim_boost: integer */
                /* Q_M_lim_boost: REAL */
                if (Q_M_is_real[m])
                {
                    adj->Q_M_lim_boost[l][m] = SBR_SQRT_FIX(MUL_R(Q_M_lim[m], G_boost));
                } else {
                    adj->Q_M_lim_boost[l][m] = SBR_SQRT_INT(MUL_R(Q_M_lim[m], G_boost));
                    adj->Q_M_lim_boost[l][m] <<= REAL_BITS;
                }

                /* S_M_boost: integer */
                if (adj->S_index_mapped[l][m])
                {
                    adj->S_M_boost[l][m] = SBR_SQRT_INT(MUL_R(S_M[m], G_boost));
                } else {
                    adj->S_M_boost[l][m] = 0;
                }
            }
        }
    }
}
#else

static void calculate_gain(sbr_info *sbr, sbr_hfadj_info *adj, uint8_t ch)
{
    static real_t limGain[] = { 0.5, 1.0, 2.0, 1e10 };
    uint8_t m, l, k;

    uint8_t current_t_noise_band = 0;
    uint8_t S_mapped;

    ALIGN real_t Q_M_lim[MAX_M];
    ALIGN real_t G_lim[MAX_M];
    ALIGN real_t G_boost;
    ALIGN real_t S_M[MAX_M];

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        uint8_t current_f_noise_band = 0;
        uint8_t current_res_band = 0;
        uint8_t current_res_band2 = 0;
        uint8_t current_hi_res_band = 0;

        real_t delta = (l == sbr->l_A[ch] || l == sbr->prevEnvIsShort[ch]) ? 0 : 1;

        S_mapped = get_S_mapped(sbr, ch, l, current_res_band2);

        if (sbr->t_E[ch][l+1] > sbr->t_Q[ch][current_t_noise_band+1])
        {
            current_t_noise_band++;
        }

        for (k = 0; k < sbr->N_L[sbr->bs_limiter_bands]; k++)
        {
            real_t G_max;
            real_t den = 0;
            real_t acc1 = 0;
            real_t acc2 = 0;
            uint8_t current_res_band_size = 0;

            uint8_t ml1, ml2;

            ml1 = sbr->f_table_lim[sbr->bs_limiter_bands][k];
            ml2 = sbr->f_table_lim[sbr->bs_limiter_bands][k+1];


            /* calculate the accumulated E_orig and E_curr over the limiter band */
            for (m = ml1; m < ml2; m++)
            {
                if ((m + sbr->kx) == sbr->f_table_res[sbr->f[ch][l]][current_res_band+1])
                {
                    current_res_band++;
                }
                acc1 += sbr->E_orig[ch][current_res_band][l];
                acc2 += sbr->E_curr[ch][m][l];
            }


            /* calculate the maximum gain */
            /* ratio of the energy of the original signal and the energy
             * of the HF generated signal
             */
            G_max = ((EPS + acc1) / (EPS + acc2)) * limGain[sbr->bs_limiter_gains];
            G_max = min(G_max, 1e10);


            for (m = ml1; m < ml2; m++)
            {
                real_t Q_M, G;
                real_t Q_div, Q_div2;
                uint8_t S_index_mapped;


                /* check if m is on a noise band border */
                if ((m + sbr->kx) == sbr->f_table_noise[current_f_noise_band+1])
                {
                    /* step to next noise band */
                    current_f_noise_band++;
                }


                /* check if m is on a resolution band border */
                if ((m + sbr->kx) == sbr->f_table_res[sbr->f[ch][l]][current_res_band2+1])
                {
                    /* step to next resolution band */
                    current_res_band2++;

                    /* if we move to a new resolution band, we should check if we are
                     * going to add a sinusoid in this band
                     */
                    S_mapped = get_S_mapped(sbr, ch, l, current_res_band2);
                }


                /* check if m is on a HI_RES band border */
                if ((m + sbr->kx) == sbr->f_table_res[HI_RES][current_hi_res_band+1])
                {
                    /* step to next HI_RES band */
                    current_hi_res_band++;
                }


                /* find S_index_mapped
                 * S_index_mapped can only be 1 for the m in the middle of the
                 * current HI_RES band
                 */
                S_index_mapped = 0;
                if ((l >= sbr->l_A[ch]) ||
                    (sbr->bs_add_harmonic_prev[ch][current_hi_res_band] && sbr->bs_add_harmonic_flag_prev[ch]))
                {
                    /* find the middle subband of the HI_RES frequency band */
                    if ((m + sbr->kx) == (sbr->f_table_res[HI_RES][current_hi_res_band+1] + sbr->f_table_res[HI_RES][current_hi_res_band]) >> 1)
                        S_index_mapped = sbr->bs_add_harmonic[ch][current_hi_res_band];
                }


                /* Q_div: [0..1] (1/(1+Q_mapped)) */
                Q_div = sbr->Q_div[ch][current_f_noise_band][current_t_noise_band];


                /* Q_div2: [0..1] (Q_mapped/(1+Q_mapped)) */
                Q_div2 = sbr->Q_div2[ch][current_f_noise_band][current_t_noise_band];


                /* Q_M only depends on E_orig and Q_div2:
                 * since N_Q <= N_Low <= N_High we only need to recalculate Q_M on
                 * a change of current noise band
                 */
                Q_M = sbr->E_orig[ch][current_res_band2][l] * Q_div2;


                /* S_M only depends on E_orig, Q_div and S_index_mapped:
                 * S_index_mapped can only be non-zero once per HI_RES band
                 */
                if (S_index_mapped == 0)
                {
                    S_M[m] = 0;
                } else {
                    S_M[m] = sbr->E_orig[ch][current_res_band2][l] * Q_div;

                    /* accumulate sinusoid part of the total energy */
                    den += S_M[m];
                }


                /* calculate gain */
                /* ratio of the energy of the original signal and the energy
                 * of the HF generated signal
                 */
                G = sbr->E_orig[ch][current_res_band2][l] / (1.0 + sbr->E_curr[ch][m][l]);
                if ((S_mapped == 0) && (delta == 1))
                    G *= Q_div;
                else if (S_mapped == 1)
                    G *= Q_div2;


                /* limit the additional noise energy level */
                /* and apply the limiter */
                if (G_max > G)
                {
                    Q_M_lim[m] = Q_M;
                    G_lim[m] = G;
                } else {
                    Q_M_lim[m] = Q_M * G_max / G;
                    G_lim[m] = G_max;
                }


                /* accumulate the total energy */
                den += sbr->E_curr[ch][m][l] * G_lim[m];
                if ((S_index_mapped == 0) && (l != sbr->l_A[ch]))
                    den += Q_M_lim[m];
            }

            /* G_boost: [0..2.51188643] */
            G_boost = (acc1 + EPS) / (den + EPS);
            G_boost = min(G_boost, 2.51188643 /* 1.584893192 ^ 2 */);

            for (m = ml1; m < ml2; m++)
            {
                /* apply compensation to gain, noise floor sf's and sinusoid levels */
#ifndef SBR_LOW_POWER
                adj->G_lim_boost[l][m] = sqrt(G_lim[m] * G_boost);
#else
                /* sqrt() will be done after the aliasing reduction to save a
                 * few multiplies
                 */
                adj->G_lim_boost[l][m] = G_lim[m] * G_boost;
#endif
                adj->Q_M_lim_boost[l][m] = sqrt(Q_M_lim[m] * G_boost);

                if (S_M[m] != 0)
                {
                    adj->S_M_boost[l][m] = sqrt(S_M[m] * G_boost);
                } else {
                    adj->S_M_boost[l][m] = 0;
                }
            }
        }
    }
}
#endif

#ifdef SBR_LOW_POWER
static void calc_gain_groups(sbr_info *sbr, sbr_hfadj_info *adj, real_t *deg, uint8_t ch)
{
    uint8_t l, k, i;
    uint8_t grouping;

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        i = 0;
        grouping = 0;

        for (k = sbr->kx; k < sbr->kx + sbr->M - 1; k++)
        {
            if (deg[k + 1] && adj->S_mapped[l][k-sbr->kx] == 0)
            {
                if (grouping == 0)
                {
                    sbr->f_group[l][i] = k;
                    grouping = 1;
                    i++;
                }
            } else {
                if (grouping)
                {
                    if (adj->S_mapped[l][k-sbr->kx])
                    {
                        sbr->f_group[l][i] = k;
                    } else {
                        sbr->f_group[l][i] = k + 1;
                    }
                    grouping = 0;
                    i++;
                }
            }
        }

        if (grouping)
        {
            sbr->f_group[l][i] = sbr->kx + sbr->M;
            i++;
        }

        sbr->N_G[l] = (uint8_t)(i >> 1);
    }
}

static void aliasing_reduction(sbr_info *sbr, sbr_hfadj_info *adj, real_t *deg, uint8_t ch)
{
    uint8_t l, k, m;
    real_t E_total, E_total_est, G_target, acc;

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        for (k = 0; k < sbr->N_G[l]; k++)
        {
            E_total_est = E_total = 0;

            for (m = sbr->f_group[l][k<<1]; m < sbr->f_group[l][(k<<1) + 1]; m++)
            {
                /* E_curr: integer */
                /* G_lim_boost: fixed point */
                /* E_total_est: integer */
                /* E_total: integer */
                E_total_est += sbr->E_curr[ch][m-sbr->kx][l];
#ifdef FIXED_POINT
                E_total += MUL_Q2(sbr->E_curr[ch][m-sbr->kx][l], adj->G_lim_boost[l][m-sbr->kx]);
#else
                E_total += sbr->E_curr[ch][m-sbr->kx][l] * adj->G_lim_boost[l][m-sbr->kx];
#endif
            }

            /* G_target: fixed point */
            if ((E_total_est + EPS) == 0)
            {
                G_target = 0;
            } else {
#ifdef FIXED_POINT
                G_target = (((int64_t)(E_total))<<Q2_BITS)/(E_total_est + EPS);
#else
                G_target = E_total / (E_total_est + EPS);
#endif
            }
            acc = 0;

            for (m = sbr->f_group[l][(k<<1)]; m < sbr->f_group[l][(k<<1) + 1]; m++)
            {
                real_t alpha;

                /* alpha: (COEF) fixed point */
                if (m < sbr->kx + sbr->M - 1)
                {
                    alpha = max(deg[m], deg[m + 1]);
                } else {
                    alpha = deg[m];
                }

                adj->G_lim_boost[l][m-sbr->kx] = MUL_C(alpha, G_target) +
                    MUL_C((COEF_CONST(1)-alpha), adj->G_lim_boost[l][m-sbr->kx]);

                /* acc: integer */
#ifdef FIXED_POINT
                acc += MUL_Q2(adj->G_lim_boost[l][m-sbr->kx], sbr->E_curr[ch][m-sbr->kx][l]);
#else
                acc += adj->G_lim_boost[l][m-sbr->kx] * sbr->E_curr[ch][m-sbr->kx][l];
#endif
            }

            /* acc: fixed point */
            if (acc + EPS == 0)
            {
                acc = 0;
            } else {
#ifdef FIXED_POINT
                acc = (((int64_t)(E_total))<<Q2_BITS)/(acc + EPS);
#else
                acc = E_total / (acc + EPS);
#endif
            }
            for(m = sbr->f_group[l][(k<<1)]; m < sbr->f_group[l][(k<<1) + 1]; m++)
            {
#ifdef FIXED_POINT
                adj->G_lim_boost[l][m-sbr->kx] = MUL_Q2(acc, adj->G_lim_boost[l][m-sbr->kx]);
#else
                adj->G_lim_boost[l][m-sbr->kx] = acc * adj->G_lim_boost[l][m-sbr->kx];
#endif
            }
        }
    }

    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        for (k = 0; k < sbr->N_L[sbr->bs_limiter_bands]; k++)
        {
            for (m = sbr->f_table_lim[sbr->bs_limiter_bands][k];
                 m < sbr->f_table_lim[sbr->bs_limiter_bands][k+1]; m++)
            {
#ifdef FIXED_POINT
                 adj->G_lim_boost[l][m] = SBR_SQRT_Q2(adj->G_lim_boost[l][m]);
#else
                 adj->G_lim_boost[l][m] = sqrt(adj->G_lim_boost[l][m]);
#endif
            }
        }
    }
}
#endif

static void hf_assembly(sbr_info *sbr, sbr_hfadj_info *adj,
                        qmf_t Xsbr[MAX_NTSRHFG][64], uint8_t ch)
{
    static real_t h_smooth[] = {
        FRAC_CONST(0.03183050093751), FRAC_CONST(0.11516383427084),
        FRAC_CONST(0.21816949906249), FRAC_CONST(0.30150283239582),
        FRAC_CONST(0.33333333333333)
    };
    static int8_t phi_re[] = { 1, 0, -1, 0 };
    static int8_t phi_im[] = { 0, 1, 0, -1 };

    uint8_t m, l, i, n;
    uint16_t fIndexNoise = 0;
    uint8_t fIndexSine = 0;
    uint8_t assembly_reset = 0;

    real_t G_filt, Q_filt;

    uint8_t h_SL;


    if (sbr->Reset == 1)
    {
        assembly_reset = 1;
        fIndexNoise = 0;
    } else {
        fIndexNoise = sbr->index_noise_prev[ch];
    }
    fIndexSine = sbr->psi_is_prev[ch];


    for (l = 0; l < sbr->L_E[ch]; l++)
    {
        uint8_t no_noise = (l == sbr->l_A[ch] || l == sbr->prevEnvIsShort[ch]) ? 1 : 0;

#ifdef SBR_LOW_POWER
        h_SL = 0;
#else
        h_SL = (sbr->bs_smoothing_mode == 1) ? 0 : 4;
        h_SL = (no_noise ? 0 : h_SL);
#endif

        if (assembly_reset)
        {
            for (n = 0; n < 4; n++)
            {
                memcpy(sbr->G_temp_prev[ch][n], adj->G_lim_boost[l], sbr->M*sizeof(real_t));
                memcpy(sbr->Q_temp_prev[ch][n], adj->Q_M_lim_boost[l], sbr->M*sizeof(real_t));
            }
            /* reset ringbuffer index */
            sbr->GQ_ringbuf_index[ch] = 4;
            assembly_reset = 0;
        }

        for (i = sbr->t_E[ch][l]; i < sbr->t_E[ch][l+1]; i++)
        {
#ifdef SBR_LOW_POWER
            uint8_t i_min1, i_plus1;
            uint8_t sinusoids = 0;
#endif

            /* load new values into ringbuffer */
            memcpy(sbr->G_temp_prev[ch][sbr->GQ_ringbuf_index[ch]], adj->G_lim_boost[l], sbr->M*sizeof(real_t));
            memcpy(sbr->Q_temp_prev[ch][sbr->GQ_ringbuf_index[ch]], adj->Q_M_lim_boost[l], sbr->M*sizeof(real_t));

            for (m = 0; m < sbr->M; m++)
            {
                qmf_t psi;

                G_filt = 0;
                Q_filt = 0;

#ifndef SBR_LOW_POWER
                if (h_SL != 0)
                {
                    for (n = 0; n <= 4; n++)
                    {
                        uint8_t ri = sbr->GQ_ringbuf_index[ch] + 1 + n;
                        if (ri >= 5)
                            ri -= 5;
                        G_filt += MUL_F(sbr->G_temp_prev[ch][ri][m], h_smooth[n]);
                        Q_filt += MUL_F(sbr->Q_temp_prev[ch][ri][m], h_smooth[n]);
                    }
                } else {
#endif
                    G_filt = sbr->G_temp_prev[ch][sbr->GQ_ringbuf_index[ch]][m];
                    Q_filt = sbr->Q_temp_prev[ch][sbr->GQ_ringbuf_index[ch]][m];
#ifndef SBR_LOW_POWER
                }
#endif

                Q_filt = (adj->S_M_boost[l][m] != 0 || no_noise) ? 0 : Q_filt;

                /* add noise to the output */
                fIndexNoise = (fIndexNoise + 1) & 511;

                /* the smoothed gain values are applied to Xsbr */
                /* V is defined, not calculated */
#ifndef FIXED_POINT
                QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) = G_filt * QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx])
                    + MUL_F(Q_filt, RE(V[fIndexNoise]));
#else
                QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) = MUL_Q2(G_filt, QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]))
                    + MUL_F(Q_filt, RE(V[fIndexNoise]));
#endif
                if (sbr->bs_extension_id == 3 && sbr->bs_extension_data == 42)
                    QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) = 16428320;
#ifndef SBR_LOW_POWER
#ifndef FIXED_POINT
                QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) = G_filt * QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx])
                    + MUL_F(Q_filt, IM(V[fIndexNoise]));
#else
                QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) = MUL_Q2(G_filt, QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx]))
                    + MUL_F(Q_filt, IM(V[fIndexNoise]));
#endif
#endif

                {
                    int8_t rev = (((m + sbr->kx) & 1) ? -1 : 1);
                    QMF_RE(psi) = adj->S_M_boost[l][m] * phi_re[fIndexSine];
#ifdef FIXED_POINT
                    QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) += (QMF_RE(psi) << REAL_BITS);
#else
                    QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) += QMF_RE(psi);
#endif

#ifndef SBR_LOW_POWER
                    QMF_IM(psi) = rev * adj->S_M_boost[l][m] * phi_im[fIndexSine];
#ifdef FIXED_POINT
                    QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) += (QMF_IM(psi) << REAL_BITS);
#else
                    QMF_IM(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) += QMF_IM(psi);
#endif
#else

                    i_min1 = (fIndexSine - 1) & 3;
                    i_plus1 = (fIndexSine + 1) & 3;

#ifndef FIXED_POINT
                    if ((m == 0) && (phi_re[i_plus1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx - 1]) +=
                            (rev*phi_re[i_plus1] * MUL_F(adj->S_M_boost[l][0], FRAC_CONST(0.00815)));
                        if (sbr->M != 0)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                                (rev*phi_re[i_plus1] * MUL_F(adj->S_M_boost[l][1], FRAC_CONST(0.00815)));
                        }
                    }
                    if ((m > 0) && (m < sbr->M - 1) && (sinusoids < 16) && (phi_re[i_min1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                            (rev*phi_re[i_min1] * MUL_F(adj->S_M_boost[l][m - 1], FRAC_CONST(0.00815)));
                    }
                    if ((m > 0) && (m < sbr->M - 1) && (sinusoids < 16) && (phi_re[i_plus1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                            (rev*phi_re[i_plus1] * MUL_F(adj->S_M_boost[l][m + 1], FRAC_CONST(0.00815)));
                    }
                    if ((m == sbr->M - 1) && (sinusoids < 16) && (phi_re[i_min1] != 0))
                    {
                        if (m > 0)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                                (rev*phi_re[i_min1] * MUL_F(adj->S_M_boost[l][m - 1], FRAC_CONST(0.00815)));
                        }
                        if (m + sbr->kx < 64)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx + 1]) +=
                                (rev*phi_re[i_min1] * MUL_F(adj->S_M_boost[l][m], FRAC_CONST(0.00815)));
                        }
                    }
#else
                    if ((m == 0) && (phi_re[i_plus1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx - 1]) +=
                            (rev*phi_re[i_plus1] * MUL_F((adj->S_M_boost[l][0]<<REAL_BITS), FRAC_CONST(0.00815)));
                        if (sbr->M != 0)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                                (rev*phi_re[i_plus1] * MUL_F((adj->S_M_boost[l][1]<<REAL_BITS), FRAC_CONST(0.00815)));
                        }
                    }
                    if ((m > 0) && (m < sbr->M - 1) && (sinusoids < 16) && (phi_re[i_min1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                            (rev*phi_re[i_min1] * MUL_F((adj->S_M_boost[l][m - 1]<<REAL_BITS), FRAC_CONST(0.00815)));
                    }
                    if ((m > 0) && (m < sbr->M - 1) && (sinusoids < 16) && (phi_re[i_plus1] != 0))
                    {
                        QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                            (rev*phi_re[i_plus1] * MUL_F((adj->S_M_boost[l][m + 1]<<REAL_BITS), FRAC_CONST(0.00815)));
                    }
                    if ((m == sbr->M - 1) && (sinusoids < 16) && (phi_re[i_min1] != 0))
                    {
                        if (m > 0)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx]) -=
                                (rev*phi_re[i_min1] * MUL_F((adj->S_M_boost[l][m - 1]<<REAL_BITS), FRAC_CONST(0.00815)));
                        }
                        if (m + sbr->kx < 64)
                        {
                            QMF_RE(Xsbr[i + sbr->tHFAdj][m+sbr->kx + 1]) +=
                                (rev*phi_re[i_min1] * MUL_F((adj->S_M_boost[l][m]<<REAL_BITS), FRAC_CONST(0.00815)));
                        }
                    }
#endif

                    if (adj->S_M_boost[l][m] != 0)
                        sinusoids++;
#endif
                }
            }

            fIndexSine = (fIndexSine + 1) & 3;

            /* update the ringbuffer index used for filtering G and Q with h_smooth */
            sbr->GQ_ringbuf_index[ch]++;
            if (sbr->GQ_ringbuf_index[ch] >= 5)
                sbr->GQ_ringbuf_index[ch] = 0;
        }
    }

    sbr->index_noise_prev[ch] = fIndexNoise;
    sbr->psi_is_prev[ch] = fIndexSine;
}

#endif
