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
** $Id: drc.c,v 1.2 2002/02/18 10:01:05 menno Exp $
**/

#include "common.h"

#ifdef USE_FMATH
#include <mathf.h>
#else
#include <math.h>
#endif
#include <memory.h>
#include "syntax.h"
#include "drc.h"

void init_drc(drc_info *drc, real_t cut, real_t boost)
{
    memset(drc, 0, sizeof(drc_info));

    drc->ctrl1 = cut;
    drc->ctrl2 = boost;

    drc->num_bands = 1;
    drc->band_top[0] = 1024/4 - 1;
    drc->dyn_rng_sgn[0] = 1;
    drc->dyn_rng_ctl[0] = 0;
}

void drc_decode(drc_info *drc, real_t *spec)
{
    uint16_t i, bd, top;
    real_t factor;
    uint16_t bottom = 0;

    if (drc->num_bands == 1)
        drc->band_top[0] = 1024/4 - 1;

    for (bd = 0; bd < drc->num_bands; bd++)
    {
        top = 4 * (drc->band_top[bd] + 1);

        /* Decode DRC gain factor */
        if (drc->dyn_rng_sgn[bd])  /* compress */
#ifdef USE_FMATH
            factor = powf(2.0f, (-drc->ctrl1 * drc->dyn_rng_ctl[bd]/24.0f));
#else
            factor = (real_t)pow(2.0, (-drc->ctrl1 * drc->dyn_rng_ctl[bd]/24.0));
#endif
        else /* boost */
#ifdef USE_FMATH
            factor = powf(2.0f, (drc->ctrl2 * drc->dyn_rng_ctl[bd]/24.0f));
#else
            factor = (real_t)pow(2.0, (drc->ctrl2 * drc->dyn_rng_ctl[bd]/24.0));
#endif

        /* Level alignment between different programs (if desired) */
        /* If program reference normalization is done in the digital domain,
           modify factor to perform normalization.
           prog_ref_level can alternatively be passed to the system for
           modification of the level in the analog domain. Analog level
           modification avoids problems with reduced DAC SNR (if signal is
           attenuated) or clipping (if signal is boosted)
         */
#ifdef USE_FMATH
        factor *= powf(0.5f, ((DRC_REF_LEVEL - drc->prog_ref_level)/24.0f));
#else
        factor *= (real_t)pow(0.5, ((DRC_REF_LEVEL - drc->prog_ref_level)/24.0));
#endif

        /* Apply gain factor */
        for (i = bottom; i < top; i++)
            spec[i] *= factor;

        bottom = top;
    }
}
