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
** $Id: sbr_hfgen.c,v 1.17 2004/04/12 18:17:42 menno Exp $
**/

/* High Frequency generation */

#include "common.h"
#include "structs.h"

#ifdef SBR_DEC

#include "sbr_syntax.h"
#include "sbr_hfgen.h"
#include "sbr_fbt.h"


/* static function declarations */
#ifdef SBR_LOW_POWER
static void calc_prediction_coef_lp(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][32],
                                    complex_t *alpha_0, complex_t *alpha_1, real_t *rxx);
static void calc_aliasing_degree(sbr_info *sbr, real_t *rxx, real_t *deg);
#else
static void calc_prediction_coef(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][32],
                                 complex_t *alpha_0, complex_t *alpha_1, uint8_t k);
#endif
static void calc_chirp_factors(sbr_info *sbr, uint8_t ch);
static void patch_construction(sbr_info *sbr);


void hf_generation(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][32],
                   qmf_t Xhigh[MAX_NTSRHFG][64]
#ifdef SBR_LOW_POWER
                   ,real_t *deg
#endif
                   ,uint8_t ch)
{
    uint8_t l, i, x;
    ALIGN complex_t alpha_0[64], alpha_1[64];
#ifdef SBR_LOW_POWER
    ALIGN real_t rxx[64];
#endif

    uint8_t offset = sbr->tHFAdj;
    uint8_t first = sbr->t_E[ch][0];
    uint8_t last = sbr->t_E[ch][sbr->L_E[ch]];

    calc_chirp_factors(sbr, ch);

    for (i = first; i < last; i++)
    {
        memset(Xhigh[i + offset], 0, 64 * sizeof(qmf_t));
    }
#ifdef SBR_LOW_POWER
    memset(deg, 0, 64*sizeof(real_t));
#endif

    if ((ch == 0) && (sbr->Reset))
        patch_construction(sbr);

    /* calculate the prediction coefficients */
#ifdef SBR_LOW_POWER
    calc_prediction_coef_lp(sbr, Xlow, alpha_0, alpha_1, rxx);
    calc_aliasing_degree(sbr, rxx, deg);
#else
    //calc_prediction_coef(sbr, Xlow, alpha_0, alpha_1);
#endif

    /* actual HF generation */
    for (i = 0; i < sbr->noPatches; i++)
    {
        for (x = 0; x < sbr->patchNoSubbands[i]; x++)
        {
            complex_t a0, a1;
            real_t bw, bw2;
            uint8_t q, p, k, g;

            /* find the low and high band for patching */
            k = sbr->kx + x;
            for (q = 0; q < i; q++)
            {
                k += sbr->patchNoSubbands[q];
            }
            p = sbr->patchStartSubband[i] + x;

#ifdef SBR_LOW_POWER
            if (x != 0 /*x < sbr->patchNoSubbands[i]-1*/)
                deg[k] = deg[p];
            else
                deg[k] = 0;
#endif

            g = sbr->table_map_k_to_g[k];

            bw = sbr->bwArray[ch][g];
            bw2 = MUL_C(bw, bw);

            /* do the patching */
            /* with or without filtering */
            if (bw2 > 0)
            {
#ifndef SBR_LOW_POWER
                calc_prediction_coef(sbr, Xlow, alpha_0, alpha_1, p);
#endif

                RE(a0) = MUL_C(RE(alpha_0[p]), bw);
                RE(a1) = MUL_C(RE(alpha_1[p]), bw2);
#ifndef SBR_LOW_POWER
                IM(a0) = MUL_C(IM(alpha_0[p]), bw);
                IM(a1) = MUL_C(IM(alpha_1[p]), bw2);
#endif

				for (l = first; l < last; l++)
                {
                    QMF_RE(Xhigh[l + offset][k]) = QMF_RE(Xlow[l + offset][p]);
#ifndef SBR_LOW_POWER
                    QMF_IM(Xhigh[l + offset][k]) = QMF_IM(Xlow[l + offset][p]);
#endif

#ifdef SBR_LOW_POWER
                    QMF_RE(Xhigh[l + offset][k]) += (
                        MUL_R(RE(a0), QMF_RE(Xlow[l - 1 + offset][p])) +
                        MUL_R(RE(a1), QMF_RE(Xlow[l - 2 + offset][p])));
#else
                    QMF_RE(Xhigh[l + offset][k]) += (
                        MUL_R(RE(a0), QMF_RE(Xlow[l - 1 + offset][p])) -
                        MUL_R(IM(a0), QMF_IM(Xlow[l - 1 + offset][p])) +
                        MUL_R(RE(a1), QMF_RE(Xlow[l - 2 + offset][p])) -
                        MUL_R(IM(a1), QMF_IM(Xlow[l - 2 + offset][p])));
                    QMF_IM(Xhigh[l + offset][k]) += (
                        MUL_R(IM(a0), QMF_RE(Xlow[l - 1 + offset][p])) +
                        MUL_R(RE(a0), QMF_IM(Xlow[l - 1 + offset][p])) +
                        MUL_R(IM(a1), QMF_RE(Xlow[l - 2 + offset][p])) +
                        MUL_R(RE(a1), QMF_IM(Xlow[l - 2 + offset][p])));
#endif
                }
            } else {
                for (l = first; l < last; l++)
                {
                    QMF_RE(Xhigh[l + offset][k]) = QMF_RE(Xlow[l + offset][p]);
#ifndef SBR_LOW_POWER
                    QMF_IM(Xhigh[l + offset][k]) = QMF_IM(Xlow[l + offset][p]);
#endif
                }
            }
        }
    }

    if (sbr->Reset)
    {
        limiter_frequency_table(sbr);
    }
}

typedef struct
{
    complex_t r01;
    complex_t r02;
    complex_t r11;
    complex_t r12;
    complex_t r22;
    real_t det;
} acorr_coef;

#define SBR_ABS(A) ((A) < 0) ? -(A) : (A)

#ifdef SBR_LOW_POWER
static void auto_correlation(sbr_info *sbr, acorr_coef *ac,
                             qmf_t buffer[MAX_NTSRHFG][32],
                             uint8_t bd, uint8_t len)
{
    real_t r01 = 0, r02 = 0, r11 = 0;
    int8_t j;
    uint8_t offset = sbr->tHFAdj;
#ifdef FIXED_POINT
    const real_t rel = FRAC_CONST(0.999999); // 1 / (1 + 1e-6f);
    uint32_t maxi = 0;
    uint32_t pow2, exp;
#else
    const real_t rel = 1 / (1 + 1e-6f);
#endif


#ifdef FIXED_POINT
    /*
     *  For computing the covariance matrix and the filter coefficients
     *  in fixed point, all values are normalised so that the fixed point
     *  values don't overflow.
     */
    for (j = offset-2; j < len + offset; j++)
    {
        maxi = max(SBR_ABS(QMF_RE(buffer[j][bd])>>REAL_BITS), maxi);
    }

    /* find the first power of 2 bigger than max to avoid division */
    pow2 = 1;
    exp = 0;
    while (maxi > pow2)
    {
        pow2 <<= 1;
        exp++;
    }

    /* improves accuracy */
    exp -= 1;

    for (j = offset; j < len + offset; j++)
    {
        real_t buf_j = ((QMF_RE(buffer[j][bd])+(1<<(exp-1)))>>exp);
        real_t buf_j_1 = ((QMF_RE(buffer[j-1][bd])+(1<<(exp-1)))>>exp);
        real_t buf_j_2 = ((QMF_RE(buffer[j-2][bd])+(1<<(exp-1)))>>exp);

        /* normalisation with rounding */
        r01 += MUL_R(buf_j, buf_j_1);
        r02 += MUL_R(buf_j, buf_j_2);
        r11 += MUL_R(buf_j_1, buf_j_1);
    }
    RE(ac->r12) = r01 -
        MUL_R(((QMF_RE(buffer[len+offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_RE(buffer[offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp));
    RE(ac->r22) = r11 -
        MUL_R(((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp));
#else
    for (j = offset; j < len + offset; j++)
    {
        r01 += QMF_RE(buffer[j][bd]) * QMF_RE(buffer[j-1][bd]);
        r02 += QMF_RE(buffer[j][bd]) * QMF_RE(buffer[j-2][bd]);
        r11 += QMF_RE(buffer[j-1][bd]) * QMF_RE(buffer[j-1][bd]);
    }
    RE(ac->r12) = r01 -
        QMF_RE(buffer[len+offset-1][bd]) * QMF_RE(buffer[len+offset-2][bd]) +
        QMF_RE(buffer[offset-1][bd]) * QMF_RE(buffer[offset-2][bd]);
    RE(ac->r22) = r11 -
        QMF_RE(buffer[len+offset-2][bd]) * QMF_RE(buffer[len+offset-2][bd]) +
        QMF_RE(buffer[offset-2][bd]) * QMF_RE(buffer[offset-2][bd]);
#endif
    RE(ac->r01) = r01;
    RE(ac->r02) = r02;
    RE(ac->r11) = r11;

    ac->det = MUL_R(RE(ac->r11), RE(ac->r22)) - MUL_F(MUL_R(RE(ac->r12), RE(ac->r12)), rel);
}
#else
static void auto_correlation(sbr_info *sbr, acorr_coef *ac, qmf_t buffer[MAX_NTSRHFG][32],
                             uint8_t bd, uint8_t len)
{
    real_t r01r = 0, r01i = 0, r02r = 0, r02i = 0, r11r = 0;
#ifdef FIXED_POINT
    const real_t rel = FRAC_CONST(0.999999); // 1 / (1 + 1e-6f);
    uint32_t maxi = 0;
    uint32_t pow2, exp;
#else
    const real_t rel = 1 / (1 + 1e-6f);
#endif
    int8_t j;
    uint8_t offset = sbr->tHFAdj;

#ifdef FIXED_POINT
    /*
     *  For computing the covariance matrix and the filter coefficients
     *  in fixed point, all values are normalised so that the fixed point
     *  values don't overflow.
     */
    for (j = offset-2; j < len + offset; j++)
    {
        maxi = max(SBR_ABS(QMF_RE(buffer[j][bd])>>REAL_BITS), maxi);
        maxi = max(SBR_ABS(QMF_IM(buffer[j][bd])>>REAL_BITS), maxi);
    }

    /* find the first power of 2 bigger than max to avoid division */
    pow2 = 1;
    exp = 0;
    while (maxi > pow2)
    {
        pow2 <<= 1;
        exp++;
    }

    /* improves accuracy */
    exp -= 1;

    for (j = offset; j < len + offset; j++)
    {
        real_t rbuf_j = ((QMF_RE(buffer[j][bd])+(1<<(exp-1)))>>exp);
        real_t ibuf_j = ((QMF_IM(buffer[j][bd])+(1<<(exp-1)))>>exp);
        real_t rbuf_j_1 = ((QMF_RE(buffer[j-1][bd])+(1<<(exp-1)))>>exp);
        real_t ibuf_j_1 = ((QMF_IM(buffer[j-1][bd])+(1<<(exp-1)))>>exp);
        real_t rbuf_j_2 = ((QMF_RE(buffer[j-2][bd])+(1<<(exp-1)))>>exp);
        real_t ibuf_j_2 = ((QMF_IM(buffer[j-2][bd])+(1<<(exp-1)))>>exp);

        r01r += MUL_R(rbuf_j, rbuf_j_1) + MUL_R(ibuf_j, ibuf_j_1);
        r01i += MUL_R(ibuf_j, rbuf_j_1) - MUL_R(rbuf_j, ibuf_j_1);
        r02r += MUL_R(rbuf_j, rbuf_j_2) + MUL_R(ibuf_j, ibuf_j_2);
        r02i += MUL_R(ibuf_j, rbuf_j_2) - MUL_R(rbuf_j, ibuf_j_2);
        r11r += MUL_R(rbuf_j_1, rbuf_j_1) + MUL_R(ibuf_j_1, ibuf_j_1);
    }
    RE(ac->r12) = r01r -
        (MUL_R(((QMF_RE(buffer[len+offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_IM(buffer[len+offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp))) +
        (MUL_R(((QMF_RE(buffer[offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_IM(buffer[offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)));
    IM(ac->r12) = r01i -
        (MUL_R(((QMF_IM(buffer[len+offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp)) -
        MUL_R(((QMF_RE(buffer[len+offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp))) +
        (MUL_R(((QMF_IM(buffer[offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)) -
        MUL_R(((QMF_RE(buffer[offset-1][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)));
    RE(ac->r22) = r11r -
        (MUL_R(((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_IM(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[len+offset-2][bd])+(1<<(exp-1)))>>exp))) +
        (MUL_R(((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_RE(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)) +
        MUL_R(((QMF_IM(buffer[offset-2][bd])+(1<<(exp-1)))>>exp), ((QMF_IM(buffer[offset-2][bd])+(1<<(exp-1)))>>exp)));
#else
    for (j = offset; j < len + offset; j++)
    {
        r01r += QMF_RE(buffer[j][bd]) * QMF_RE(buffer[j-1][bd]) +
            QMF_IM(buffer[j][bd]) * QMF_IM(buffer[j-1][bd]);
        r01i += QMF_IM(buffer[j][bd]) * QMF_RE(buffer[j-1][bd]) -
            QMF_RE(buffer[j][bd]) * QMF_IM(buffer[j-1][bd]);
        r02r += QMF_RE(buffer[j][bd]) * QMF_RE(buffer[j-2][bd]) +
            QMF_IM(buffer[j][bd]) * QMF_IM(buffer[j-2][bd]);
        r02i += QMF_IM(buffer[j][bd]) * QMF_RE(buffer[j-2][bd]) -
            QMF_RE(buffer[j][bd]) * QMF_IM(buffer[j-2][bd]);
        r11r += QMF_RE(buffer[j-1][bd]) * QMF_RE(buffer[j-1][bd]) +
            QMF_IM(buffer[j-1][bd]) * QMF_IM(buffer[j-1][bd]);
    }
    RE(ac->r12) = r01r -
        (QMF_RE(buffer[len+offset-1][bd]) * QMF_RE(buffer[len+offset-2][bd]) + QMF_IM(buffer[len+offset-1][bd]) * QMF_IM(buffer[len+offset-2][bd])) +
        (QMF_RE(buffer[offset-1][bd]) * QMF_RE(buffer[offset-2][bd]) + QMF_IM(buffer[offset-1][bd]) * QMF_IM(buffer[offset-2][bd]));
    IM(ac->r12) = r01i -
        (QMF_IM(buffer[len+offset-1][bd]) * QMF_RE(buffer[len+offset-2][bd]) - QMF_RE(buffer[len+offset-1][bd]) * QMF_IM(buffer[len+offset-2][bd])) +
        (QMF_IM(buffer[offset-1][bd]) * QMF_RE(buffer[offset-2][bd]) - QMF_RE(buffer[offset-1][bd]) * QMF_IM(buffer[offset-2][bd]));
    RE(ac->r22) = r11r -
        (QMF_RE(buffer[len+offset-2][bd]) * QMF_RE(buffer[len+offset-2][bd]) + QMF_IM(buffer[len+offset-2][bd]) * QMF_IM(buffer[len+offset-2][bd])) +
        (QMF_RE(buffer[offset-2][bd]) * QMF_RE(buffer[offset-2][bd]) + QMF_IM(buffer[offset-2][bd]) * QMF_IM(buffer[offset-2][bd]));
#endif

    RE(ac->r01) = r01r;
    IM(ac->r01) = r01i;
    RE(ac->r02) = r02r;
    IM(ac->r02) = r02i;
    RE(ac->r11) = r11r;

    ac->det = MUL_R(RE(ac->r11), RE(ac->r22)) - MUL_F(rel, (MUL_R(RE(ac->r12), RE(ac->r12)) + MUL_R(IM(ac->r12), IM(ac->r12))));
}
#endif

/* calculate linear prediction coefficients using the covariance method */
#ifndef SBR_LOW_POWER
static void calc_prediction_coef(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][32],
                                 complex_t *alpha_0, complex_t *alpha_1, uint8_t k)
{
    real_t tmp;
    acorr_coef ac;

    auto_correlation(sbr, &ac, Xlow, k, sbr->numTimeSlotsRate + 6);

    if (ac.det == 0)
    {
        RE(alpha_1[k]) = 0;
        IM(alpha_1[k]) = 0;
    } else {
#ifdef FIXED_POINT
        tmp = (MUL_R(RE(ac.r01), RE(ac.r12)) - MUL_R(IM(ac.r01), IM(ac.r12)) - MUL_R(RE(ac.r02), RE(ac.r11)));
        RE(alpha_1[k]) = DIV_R(tmp, ac.det);
        tmp = (MUL_R(IM(ac.r01), RE(ac.r12)) + MUL_R(RE(ac.r01), IM(ac.r12)) - MUL_R(IM(ac.r02), RE(ac.r11)));
        IM(alpha_1[k]) = DIV_R(tmp, ac.det);
#else
        tmp = REAL_CONST(1.0) / ac.det;
        RE(alpha_1[k]) = (MUL_R(RE(ac.r01), RE(ac.r12)) - MUL_R(IM(ac.r01), IM(ac.r12)) - MUL_R(RE(ac.r02), RE(ac.r11))) * tmp;
        IM(alpha_1[k]) = (MUL_R(IM(ac.r01), RE(ac.r12)) + MUL_R(RE(ac.r01), IM(ac.r12)) - MUL_R(IM(ac.r02), RE(ac.r11))) * tmp;
#endif
    }

    if (RE(ac.r11) == 0)
    {
        RE(alpha_0[k]) = 0;
        IM(alpha_0[k]) = 0;
    } else {
#ifdef FIXED_POINT
        tmp = -(RE(ac.r01) + MUL_R(RE(alpha_1[k]), RE(ac.r12)) + MUL_R(IM(alpha_1[k]), IM(ac.r12)));
        RE(alpha_0[k]) = DIV_R(tmp, RE(ac.r11));
        tmp = -(IM(ac.r01) + MUL_R(IM(alpha_1[k]), RE(ac.r12)) - MUL_R(RE(alpha_1[k]), IM(ac.r12)));
        IM(alpha_0[k]) = DIV_R(tmp, RE(ac.r11));
#else
        tmp = 1.0f / RE(ac.r11);
        RE(alpha_0[k]) = -(RE(ac.r01) + MUL_R(RE(alpha_1[k]), RE(ac.r12)) + MUL_R(IM(alpha_1[k]), IM(ac.r12))) * tmp;
        IM(alpha_0[k]) = -(IM(ac.r01) + MUL_R(IM(alpha_1[k]), RE(ac.r12)) - MUL_R(RE(alpha_1[k]), IM(ac.r12))) * tmp;
#endif
    }

    if ((MUL_R(RE(alpha_0[k]),RE(alpha_0[k])) + MUL_R(IM(alpha_0[k]),IM(alpha_0[k])) >= REAL_CONST(16)) ||
        (MUL_R(RE(alpha_1[k]),RE(alpha_1[k])) + MUL_R(IM(alpha_1[k]),IM(alpha_1[k])) >= REAL_CONST(16)))
    {
        RE(alpha_0[k]) = 0;
        IM(alpha_0[k]) = 0;
        RE(alpha_1[k]) = 0;
        IM(alpha_1[k]) = 0;
    }
}
#else
static void calc_prediction_coef_lp(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][32],
                                    complex_t *alpha_0, complex_t *alpha_1, real_t *rxx)
{
    uint8_t k;
    real_t tmp;
    acorr_coef ac;

    for (k = 1; k < sbr->f_master[0]; k++)
    {
        auto_correlation(sbr, &ac, Xlow, k, sbr->numTimeSlotsRate + 6);

        if (ac.det == 0)
        {
            RE(alpha_0[k]) = 0;
            RE(alpha_1[k]) = 0;
        } else {
            tmp = MUL_R(RE(ac.r01), RE(ac.r22)) - MUL_R(RE(ac.r12), RE(ac.r02));
            RE(alpha_0[k]) = DIV_R(tmp, (-ac.det));

            tmp = MUL_R(RE(ac.r01), RE(ac.r12)) - MUL_R(RE(ac.r02), RE(ac.r11));
            RE(alpha_1[k]) = DIV_R(tmp, ac.det);
        }

        if ((RE(alpha_0[k]) >= REAL_CONST(4)) || (RE(alpha_1[k]) >= REAL_CONST(4)))
        {
            RE(alpha_0[k]) = REAL_CONST(0);
            RE(alpha_1[k]) = REAL_CONST(0);
        }

        /* reflection coefficient */
        if (RE(ac.r11) == 0)
        {
            rxx[k] = COEF_CONST(0.0);
        } else {
            rxx[k] = DIV_C(RE(ac.r01), RE(ac.r11));
            rxx[k] = -rxx[k];
            if (rxx[k] > COEF_CONST(1.0)) rxx[k] = COEF_CONST(1.0);
            if (rxx[k] < COEF_CONST(-1.0)) rxx[k] = COEF_CONST(-1.0);
        }
    }
}

static void calc_aliasing_degree(sbr_info *sbr, real_t *rxx, real_t *deg)
{
    uint8_t k;

    rxx[0] = COEF_CONST(0.0);
    deg[1] = COEF_CONST(0.0);

    for (k = 2; k < sbr->k0; k++)
    {
        deg[k] = 0.0;

        if ((k % 2 == 0) && (rxx[k] < COEF_CONST(0.0)))
        {
            if (rxx[k-1] < 0.0)
            {
                deg[k] = COEF_CONST(1.0);

                if (rxx[k-2] > COEF_CONST(0.0))
                {
                    deg[k-1] = COEF_CONST(1.0) - MUL_C(rxx[k-1], rxx[k-1]);
                }
            } else if (rxx[k-2] > COEF_CONST(0.0)) {
                deg[k] = COEF_CONST(1.0) - MUL_C(rxx[k-1], rxx[k-1]);
            }
        }

        if ((k % 2 == 1) && (rxx[k] > COEF_CONST(0.0)))
        {
            if (rxx[k-1] > COEF_CONST(0.0))
            {
                deg[k] = COEF_CONST(1.0);

                if (rxx[k-2] < COEF_CONST(0.0))
                {
                    deg[k-1] = COEF_CONST(1.0) - MUL_C(rxx[k-1], rxx[k-1]);
                }
            } else if (rxx[k-2] < COEF_CONST(0.0)) {
                deg[k] = COEF_CONST(1.0) - MUL_C(rxx[k-1], rxx[k-1]);
            }
        }
    }
}
#endif

/* FIXED POINT: bwArray = COEF */
static real_t mapNewBw(uint8_t invf_mode, uint8_t invf_mode_prev)
{
    switch (invf_mode)
    {
    case 1: /* LOW */
        if (invf_mode_prev == 0) /* NONE */
            return COEF_CONST(0.6);
        else
            return COEF_CONST(0.75);

    case 2: /* MID */
        return COEF_CONST(0.9);

    case 3: /* HIGH */
        return COEF_CONST(0.98);

    default: /* NONE */
        if (invf_mode_prev == 1) /* LOW */
            return COEF_CONST(0.6);
        else
            return COEF_CONST(0.0);
    }
}

/* FIXED POINT: bwArray = COEF */
static void calc_chirp_factors(sbr_info *sbr, uint8_t ch)
{
    uint8_t i;

    for (i = 0; i < sbr->N_Q; i++)
    {
        sbr->bwArray[ch][i] = mapNewBw(sbr->bs_invf_mode[ch][i], sbr->bs_invf_mode_prev[ch][i]);

        if (sbr->bwArray[ch][i] < sbr->bwArray_prev[ch][i])
            sbr->bwArray[ch][i] = MUL_F(sbr->bwArray[ch][i], FRAC_CONST(0.75)) + MUL_F(sbr->bwArray_prev[ch][i], FRAC_CONST(0.25));
        else
            sbr->bwArray[ch][i] = MUL_F(sbr->bwArray[ch][i], FRAC_CONST(0.90625)) + MUL_F(sbr->bwArray_prev[ch][i], FRAC_CONST(0.09375));

        if (sbr->bwArray[ch][i] < COEF_CONST(0.015625))
            sbr->bwArray[ch][i] = COEF_CONST(0.0);

        if (sbr->bwArray[ch][i] >= COEF_CONST(0.99609375))
            sbr->bwArray[ch][i] = COEF_CONST(0.99609375);

        sbr->bwArray_prev[ch][i] = sbr->bwArray[ch][i];
        sbr->bs_invf_mode_prev[ch][i] = sbr->bs_invf_mode[ch][i];
    }
}

static void patch_construction(sbr_info *sbr)
{
    uint8_t i, k;
    uint8_t odd, sb;
    uint8_t msb = sbr->k0;
    uint8_t usb = sbr->kx;
    uint8_t goalSbTab[] = { 21, 23, 32, 43, 46, 64, 85, 93, 128, 0, 0, 0 };
    /* (uint8_t)(2.048e6/sbr->sample_rate + 0.5); */
    uint8_t goalSb = goalSbTab[get_sr_index(sbr->sample_rate)];

    sbr->noPatches = 0;

    if (goalSb < (sbr->kx + sbr->M))
    {
        for (i = 0, k = 0; sbr->f_master[i] < goalSb; i++)
            k = i+1;
    } else {
        k = sbr->N_master;
    }

    do
    {
        uint8_t j = k + 1;

        do
        {
            j--;

            sb = sbr->f_master[j];
            odd = (sb - 2 + sbr->k0) % 2;
        } while (sb > (sbr->k0 - 1 + msb - odd));

        sbr->patchNoSubbands[sbr->noPatches] = max(sb - usb, 0);
        sbr->patchStartSubband[sbr->noPatches] = sbr->k0 - odd -
            sbr->patchNoSubbands[sbr->noPatches];

        if (sbr->patchNoSubbands[sbr->noPatches] > 0)
        {
            usb = sb;
            msb = sb;
            sbr->noPatches++;
        } else {
            msb = sbr->kx;
        }

        if (sbr->f_master[k] - sb < 3)
            k = sbr->N_master;
    } while (sb != (sbr->kx + sbr->M));

    if ((sbr->patchNoSubbands[sbr->noPatches-1] < 3) && (sbr->noPatches > 1))
    {
        sbr->noPatches--;
    }

    sbr->noPatches = min(sbr->noPatches, 5);
}

#endif
