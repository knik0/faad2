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
** $Id: sbr_util.c,v 1.1 2002/09/29 22:19:48 menno Exp $
**/

#include "common.h"

#ifdef SBR

#include <stdlib.h>

#include "sbr_util.h"

/* calculate the start QMF channel for the master frequency band table */
/* parameter is also called k0 */
uint16_t qmf_start_channel(uint8_t bs_start_freq, uint32_t sample_rate)
{
    static uint8_t offset[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 28, 33
    };
    uint8_t startMin;

    if (sample_rate >= 64000)
    {
        startMin = (uint8_t)((5000.*128.)/(float)sample_rate + 0.5);
    } else if (sample_rate < 32000) {
        startMin = (uint8_t)((3000.*128.)/(float)sample_rate + 0.5);
    } else {
        startMin = (uint8_t)((4000.*128.)/(float)sample_rate + 0.5);
    }

    return startMin + offset[bs_start_freq];
}

static int32_t longcmp(const void *a, const void *b)
{
    return ((int32_t)(*(int32_t*)a - *(int32_t*)b));
}

/* calculate the stop QMF channel for the master frequency band table */
/* parameter is also called k2 */
uint16_t qmf_stop_channel(uint8_t bs_stop_freq, uint32_t sample_rate,
                          uint16_t k0)
{
    if (bs_stop_freq == 15)
        return k0 * 3;
    else if (bs_stop_freq == 14)
        return k0 * 2;
    else {
        uint8_t i;
        uint8_t stopMin;
        uint32_t stopDk[13], k2;

        if (sample_rate >= 64000)
        {
            stopMin = (uint8_t)((10000.*128.)/(float)sample_rate + 0.5);
        } else if (sample_rate < 32000) {
            stopMin = (uint8_t)((6000.*128.)/(float)sample_rate + 0.5);
        } else {
            stopMin = (uint8_t)((8000.*128.)/(float)sample_rate + 0.5);
        }

        /* TODO: PUT THIS IN MAPLE, CAN BE SIMPLIFIED A LOT */
        for (i = 0; i < 13; i++)
        {
            stopDk[i] = (uint32_t)(stopMin*pow(64./(float)stopMin, (i+1)/13.) + 0.5) -
                (uint32_t)(stopMin*pow(64./(float)stopMin, i/13.) + 0.5);
        }

        /* needed? or does this always reverse the array? */
        qsort(stopDk, 13, sizeof(stopDk[0]), longcmp);

        k2 = stopMin;
        for (i = 0; i < bs_stop_freq-1; i++)
        {
            k2 += stopDk[i];
        }
        return k2;
    }

    return 0;
}

/* calculate the master frequency table from k0, k2, bs_freq_scale
   and bs_alter_scale

   returns N_master

   version for bs_freq_scale = 0
*/
uint32_t master_frequency_table_fs0(int32_t *f_master,
                                    uint16_t k0, uint16_t k2,
                                    uint8_t bs_alter_scale)
{
    int8_t incr;
    uint8_t k;
    uint8_t dk;
    uint32_t nrBands, k2Achieved;
    int32_t k2Diff, vDk[100 /*TODO*/];

    /* mft only defined for k2 > k0 */
    if (k2 <= k0)
        return 0;

    dk = bs_alter_scale ? 2 : 1;
    nrBands = 2 * (int32_t)((k2-k0)/(float)dk*2. + 0.5);

    k2Achieved = k0 + nrBands * dk;
    k2Diff = k2 - k2Achieved;
    for (k = 0; k <= nrBands; k++)
        vDk[k] = dk;

    if (k2Diff)
    {
        incr = (k2Diff > 0) ? -1 : 1;
        k = (k2Diff > 0) ? (nrBands-1) : 0;

        while (k2Diff != 0)
        {
            vDk[k] = vDk[k] - 1;
            k += incr;
            k2Diff += incr;
        }
    }

    f_master[0] = k0;
    for (k = 1; k <= nrBands; k++)
        f_master[k] = f_master[k-1] + vDk[k-1];

    return nrBands;
}

/*
   version for bs_freq_scale > 0
*/
uint32_t master_frequency_table(int32_t *f_master,
                                uint16_t k0, uint16_t k2,
                                uint8_t bs_freq_scale,
                                uint8_t bs_alter_scale)
{
    uint8_t k, bands, twoRegions;
    uint16_t k1;
    uint32_t nrBand0, nrBand1, N_master;
    int32_t max_vDk0, min_vDk1;
    int32_t vDk0[100 /*TODO*/], vDk1[100 /*TODO*/];
    int32_t vk0[100 /*TODO*/], vk1[100 /*TODO*/];
    float warp;
    uint8_t temp1[] = { 12, 10, 8 };
    float temp2[] = { 1.0, 1.3 };

    /* mft only defined for k2 > k0 */
    if (k2 <= k0)
        return 0;

    bands = temp1[bs_freq_scale-1];
    warp = temp2[bs_alter_scale];

    if ((float)k2/(float)k0 > 2.2449)
    {
        twoRegions = 1;
        k1 = 2 * k0;
    } else {
        twoRegions = 0;
        k1 = k2;
    }

    nrBand0 = 2 * (int32_t)(bands * log(k1/k0)/(2.0*log(2.0)) + 0.5);
    max_vDk0 = 0;
    for (k = 0; k <= nrBand0; k++)
    {
        vDk0[k] = (int32_t)(k0 * pow((float)k1/(float)k0, (k+1)/(float)nrBand0)+0.5) -
            (int32_t)(k0 * pow((float)k1/(float)k0, k/(float)nrBand0)+0.5);
        max_vDk0 = (max_vDk0 < vDk0[k]) ? vDk0[k] : max_vDk0;
    }

    /* needed? */
    qsort(vDk0, nrBand0 + 1, sizeof(vDk0[0]), longcmp);

    vk0[0] = k0;
    for (k = 1; k <= nrBand0; k++)
    {
        vk0[k] = vk0[k-1] + vDk0[k-1];
    }

    if (!twoRegions)
    {
        for (k = 0; k <= nrBand0; k++)
            f_master[k] = vk0[k];

        return nrBand0;
    }

    nrBand1 = 2 * (int32_t)(bands * log((float)k2/(float)k1)/(2.0 * log(2.0) * warp) + 0.5);
    min_vDk1 = 9999999;
    for (k = 0; k <= nrBand1 - 1; k++)
    {
        vDk1[k] = (int32_t)(k0 * pow((float)k1/(float)k0, (k+1)/(float)nrBand1)+0.5) -
            (int32_t)(k0 * pow((float)k1/(float)k0, k/(float)nrBand1)+0.5);
        min_vDk1 = (min_vDk1 > vDk1[k]) ? vDk1[k] : min_vDk1;
    }

    if (min_vDk1 < max_vDk0)
    {
        int32_t change;

        qsort(vDk1, nrBand1 + 1, sizeof(vDk1[0]), longcmp);
        change = max_vDk0 - vDk1[0];
        vDk1[0] = max_vDk0;
        vDk1[nrBand1 - 1] = vDk1[nrBand1 - 1] - change;
    }

    qsort(vDk1, nrBand1 + 1, sizeof(vDk1[0]), longcmp);
    vk1[0] = k1;
    for (k = 1; k <= nrBand1; k++)
    {
        vk1[k] = vk1[k-1] + vDk1[k-1];
    }

    N_master = nrBand0 + nrBand1;
    for (k = 0; k <= nrBand0; k++)
    {
        f_master[k] = vk0[k];
    }
    for (k = nrBand0 + 1; k <= N_master; k++)
    {
        f_master[k] = vk1[k - nrBand0];
    }

    return N_master;
}

#endif
