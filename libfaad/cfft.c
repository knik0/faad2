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
** $Id: cfft.c,v 1.2 2002/08/17 10:03:10 menno Exp $
**/

/*
 * Algorithmically based on Fortran-77 FFTPACK
 * by Paul N. Swarztrauber(Version 4, 1985).
 */

/* isign is +1 for backward and -1 for forward transforms */


#include "common.h"
#include <stdlib.h>

#include "cfft.h"


/*----------------------------------------------------------------------
   passf2, passf3, passf4, passf5, passf. Complex FFT passes fwd and bwd.
  ----------------------------------------------------------------------*/

static void passf2(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, int8_t isign)
{
    uint16_t i, k, ah, ac;
    real_t ti2, tr2;

    if (ido <= 2)
    {
        for (k = 0; k < l1; k++)
        {
            ah = k*ido;
            ac = 2*k*ido;
            ch[ah] = cc[ac] + cc[ac+ido];
            ch[ah+ido*l1] = cc[ac] - cc[ac+ido];
            ch[ah+1] = cc[ac+1] + cc[ac+ido+1];
            ch[ah+ido*l1+1] = cc[ac+1] - cc[ac+ido+1];
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            for(i = 0; i < ido-1; i += 2)
            {
                ah = i + k*ido;
                ac = i + 2*k*ido;
                ch[ah] = cc[ac] + cc[ac+ido];
                tr2 = cc[ac] - cc[ac+ido];
                ch[ah+1] = cc[ac+1] + cc[ac+1+ido];
                ti2 = cc[ac+1] - cc[ac+1+ido];
                ch[ah+l1*ido+1] = wa1[i]*ti2 + isign*wa1[i+1]*tr2;
                ch[ah+l1*ido] = wa1[i]*tr2 - isign*wa1[i+1]*ti2;
            }
        }
    }
}


static void passf3(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, int8_t isign)
{
    static real_t taur = -0.5;
    static real_t taui = 0.866025403784439;
    uint16_t i, k, ac, ah;
    real_t ci2, ci3, di2, di3, cr2, cr3, dr2, dr3, ti2, tr2;

    if (ido == 2)
    {
        for (k = 1; k <= l1; k++)
        {
            ac = (3*k-2) * ido;
            tr2 = cc[ac] + cc[ac+ido];
            cr2 = cc[ac-ido] + taur*tr2;
            ah = (k-1) * ido;
            ch[ah] = cc[ac-ido] + tr2;

            ti2 = cc[ac+1] + cc[ac+ido+1];
            ci2 = cc[ac-ido+1] + taur*ti2;
            ch[ah+1] = cc[ac-ido+1] + ti2;

            cr3 = isign * taui * (cc[ac] - cc[ac+ido]);
            ci3 = isign * taui * (cc[ac+1] - cc[ac+ido+1]);
            ch[ah+l1*ido] = cr2 - ci3;
            ch[ah+2*l1*ido] = cr2 + ci3;
            ch[ah+l1*ido+1] = ci2 + cr3;
            ch[ah+2*l1*ido+1] = ci2 - cr3;
        }
    } else {
        for (k = 1; k <= l1; k++)
        {
            for (i = 0; i < ido-1; i += 2)
            {
                ac = i + (3*k-2) * ido;
                tr2 = cc[ac] + cc[ac+ido];
                cr2 = cc[ac-ido] + taur*tr2;
                ah = i + (k-1) * ido;
                ch[ah] = cc[ac-ido] + tr2;
                ti2 = cc[ac+1] + cc[ac+ido+1];
                ci2 = cc[ac-ido+1] + taur*ti2;
                ch[ah+1] = cc[ac-ido+1] + ti2;
                cr3 = isign * taui * (cc[ac] - cc[ac+ido]);
                ci3 = isign * taui * (cc[ac+1] - cc[ac+ido+1]);
                dr2 = cr2 - ci3;
                dr3 = cr2 + ci3;
                di2 = ci2 + cr3;
                di3 = ci2 - cr3;
                ch[ah+l1*ido+1] = wa1[i]*di2 + isign*wa1[i+1]*dr2;
                ch[ah+l1*ido] = wa1[i]*dr2 - isign*wa1[i+1]*di2;
                ch[ah+2*l1*ido+1] = wa2[i]*di3 + isign*wa2[i+1]*dr3;
                ch[ah+2*l1*ido] = wa2[i]*dr3 - isign*wa2[i+1]*di3;
            }
        }
    }
}


static void passf4(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, real_t *wa3, int8_t isign)
{
    uint16_t i, k, ac, ah;
    real_t ci2, ci3, ci4, cr2, cr3, cr4, ti1, ti2, ti3, ti4, tr1, tr2,
        tr3, tr4;

    if (ido == 2)
    {
        for (k = 0; k < l1; k++)
        {
            ac = 4*k*ido + 1;
            ti1 = cc[ac] - cc[ac+2*ido];
            ti2 = cc[ac] + cc[ac+2*ido];
            tr4 = cc[ac+3*ido] - cc[ac+ido];
            ti3 = cc[ac+ido] + cc[ac+3*ido];
            tr1 = cc[ac-1] - cc[ac+2*ido-1];
            tr2 = cc[ac-1] + cc[ac+2*ido-1];
            ti4 = cc[ac+ido-1] - cc[ac+3*ido-1];
            tr3 = cc[ac+ido-1] + cc[ac+3*ido-1];
            ah = k*ido;
            ch[ah] = tr2 + tr3;
            ch[ah+2*l1*ido] = tr2 - tr3;
            ch[ah+1] = ti2 + ti3;
            ch[ah+2*l1*ido+1] = ti2 - ti3;
            ch[ah+l1*ido] = tr1 + isign*tr4;
            ch[ah+3*l1*ido] = tr1 - isign*tr4;
            ch[ah+l1*ido+1] = ti1 + isign*ti4;
            ch[ah+3*l1*ido+1] = ti1 - isign*ti4;
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            for (i = 0; i < ido-1; i += 2)
            {
                ac = i + 1 + 4*k*ido;
                ti1 = cc[ac] - cc[ac+2*ido];
                ti2 = cc[ac] + cc[ac+2*ido];
                ti3 = cc[ac+ido] + cc[ac+3*ido];
                tr4 = cc[ac+3*ido] - cc[ac+ido];
                tr1 = cc[ac-1] - cc[ac+2*ido-1];
                tr2 = cc[ac-1] + cc[ac+2*ido-1];
                ti4 = cc[ac+ido-1] - cc[ac+3*ido-1];
                tr3 = cc[ac+ido-1] + cc[ac+3*ido-1];
                ah = i + k*ido;
                ch[ah] = tr2 + tr3;
                cr3 = tr2 - tr3;
                ch[ah+1] = ti2 + ti3;
                ci3 = ti2 - ti3;
                cr2 = tr1 + isign*tr4;
                cr4 = tr1 - isign*tr4;
                ci2 = ti1 + isign*ti4;
                ci4 = ti1 - isign*ti4;
                ch[ah+l1*ido] = wa1[i]*cr2 - isign*wa1[i+1]*ci2;
                ch[ah+l1*ido+1] = wa1[i]*ci2 + isign*wa1[i+1]*cr2;
                ch[ah+2*l1*ido] = wa2[i]*cr3 - isign*wa2[i+1]*ci3;
                ch[ah+2*l1*ido+1] = wa2[i]*ci3 + isign*wa2[i+1]*cr3;
                ch[ah+3*l1*ido] = wa3[i]*cr4 - isign*wa3[i+1]*ci4;
                ch[ah+3*l1*ido+1] = wa3[i]*ci4 + isign*wa3[i+1]*cr4;
            }
        }
    }
}


static void passf5(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, real_t *wa3, real_t *wa4,
                   int8_t isign)
{
    static real_t tr11 = 0.309016994374947;
    static real_t ti11 = 0.951056516295154;
    static real_t tr12 = -0.809016994374947;
    static real_t ti12 = 0.587785252292473;
    uint16_t i, k, ac, ah;
    real_t ci2, ci3, ci4, ci5, di3, di4, di5, di2, cr2, cr3, cr5, cr4,
        ti2, ti3, ti4, ti5, dr3, dr4, dr5, dr2, tr2, tr3, tr4, tr5;

    if (ido == 2)
    {
        for (k = 1; k <= l1; ++k)
        {
            ac = (5*k-4) * ido + 1;
            ti5 = cc[ac] - cc[ac+3*ido];
            ti2 = cc[ac] + cc[ac+3*ido];
            ti4 = cc[ac+ido] - cc[ac+2*ido];
            ti3 = cc[ac+ido] + cc[ac+2*ido];
            tr5 = cc[ac-1] - cc[ac+3*ido-1];
            tr2 = cc[ac-1] + cc[ac+3*ido-1];
            tr4 = cc[ac+ido-1] - cc[ac+2*ido-1];
            tr3 = cc[ac+ido-1] + cc[ac+2*ido-1];
            ah = (k-1) * ido;
            ch[ah] = cc[ac-ido-1] + tr2 + tr3;
            ch[ah+1] = cc[ac-ido] + ti2 + ti3;
            cr2 = cc[ac-ido-1] + tr11*tr2 + tr12*tr3;
            ci2 = cc[ac-ido] + tr11*ti2 + tr12*ti3;
            cr3 = cc[ac-ido-1] + tr12*tr2 + tr11*tr3;
            ci3 = cc[ac-ido] + tr12*ti2 + tr11*ti3;
            cr5 = isign * (ti11*tr5 + ti12*tr4);
            ci5 = isign * (ti11*ti5 + ti12*ti4);
            cr4 = isign * (ti12*tr5 - ti11*tr4);
            ci4 = isign * (ti12*ti5 - ti11*ti4);
            ch[ah+l1*ido] = cr2 - ci5;
            ch[ah+4*l1*ido] = cr2 + ci5;
            ch[ah+l1*ido+1] = ci2 + cr5;
            ch[ah+2*l1*ido+1]=ci3 + cr4;
            ch[ah+2*l1*ido] = cr3 - ci4;
            ch[ah+3*l1*ido] = cr3 + ci4;
            ch[ah+3*l1*ido+1] = ci3 - cr4;
            ch[ah+4*l1*ido+1] = ci2 - cr5;
        }
    } else {
        for (k = 1; k <= l1; k++)
        {
            for (i = 0; i < ido-1; i += 2)
            {
                ac = i + 1 + (k*5-4) * ido;
                ti5 = cc[ac] - cc[ac+3*ido];
                ti2 = cc[ac] + cc[ac+3*ido];
                ti4 = cc[ac+ido] - cc[ac+2*ido];
                ti3 = cc[ac+ido] + cc[ac+2*ido];
                tr5 = cc[ac-1] - cc[ac+3*ido-1];
                tr2 = cc[ac-1] + cc[ac+3*ido-1];
                tr4 = cc[ac+ido-1] - cc[ac+2*ido-1];
                tr3 = cc[ac+ido-1] + cc[ac+2*ido-1];
                ah = i + (k-1) * ido;
                ch[ah] = cc[ac-ido-1] + tr2 + tr3;
                ch[ah+1] = cc[ac-ido] + ti2 + ti3;
                cr2 = cc[ac-ido-1] + tr11*tr2 + tr12*tr3;
                ci2 = cc[ac-ido] + tr11*ti2 + tr12*ti3;
                cr3 = cc[ac-ido-1] + tr12*tr2 + tr11*tr3;
                ci3 = cc[ac-ido] + tr12*ti2 + tr11*ti3;
                cr5 = isign * (ti11*tr5 + ti12*tr4);
                ci5 = isign * (ti11*ti5 + ti12*ti4);
                cr4 = isign * (ti12*tr5 - ti11*tr4);
                ci4 = isign * (ti12*ti5 - ti11*ti4);
                dr3 = cr3 - ci4;
                dr4 = cr3 + ci4;
                di3 = ci3 + cr4;
                di4 = ci3 - cr4;
                dr5 = cr2 + ci5;
                dr2 = cr2 - ci5;
                di5 = ci2 - cr5;
                di2 = ci2 + cr5;
                ch[ah+l1*ido] = wa1[i]*dr2 - isign*wa1[i+1]*di2;
                ch[ah+l1*ido+1] = wa1[i]*di2 + isign*wa1[i+1]*dr2;
                ch[ah+2*l1*ido] = wa2[i]*dr3 - isign*wa2[i+1]*di3;
                ch[ah+2*l1*ido+1] = wa2[i]*di3 + isign*wa2[i+1]*dr3;
                ch[ah+3*l1*ido] = wa3[i]*dr4 - isign*wa3[i+1]*di4;
                ch[ah+3*l1*ido+1] = wa3[i]*di4 + isign*wa3[i+1]*dr4;
                ch[ah+4*l1*ido] = wa4[i]*dr5 - isign*wa4[i+1]*di5;
                ch[ah+4*l1*ido+1] = wa4[i]*di5 + isign*wa4[i+1]*dr5;
            }
        }
    }
}


static void passf(uint16_t *nac, uint16_t ido, uint16_t ip, uint16_t l1,
                  uint16_t idl1, real_t *cc, real_t *ch, real_t *wa,
                  int8_t isign)
{
    uint16_t idij, idlj, idot, ipph, i, j, k, l, jc, lc, ik, nt, idj, idl;
    uint16_t inc, idp;
    real_t wai, war;

    idot = ido / 2;
    nt = ip*idl1;
    ipph = (ip+1) / 2;
    idp = ip*ido;

    if (ido >= l1)
    {
        for (j = 1; j < ipph; j++)
        {
            jc = ip - j;

            for (k = 0; k < l1; k++)
            {
                for (i = 0; i < ido; i++)
                {
                    ch[i+(k+j*l1)*ido] = cc[i+(j+k*ip)*ido] + cc[i+(jc+k*ip)*ido];
                    ch[i+(k+jc*l1)*ido] = cc[i+(j+k*ip)*ido] - cc[i+(jc+k*ip)*ido];
                }
            }
        }

        for (k = 0; k < l1; k++)
        {
            for (i = 0; i < ido; i++)
                ch[i+k*ido] = cc[i+k*ip*ido];
        }
    } else {
        for (j = 1; j < ipph; j++)
        {
            jc = ip - j;

            for (i = 0; i < ido; i++)
            {
                for (k = 0; k < l1; k++)
                {
                    ch[i+(k+j*l1)*ido] = cc[i+(j+k*ip)*ido] + cc[i+(jc+k*ip)*ido];
                    ch[i+(k+jc*l1)*ido] = cc[i+(j+k*ip)*ido] - cc[i+(jc+k*ip)*ido];
                }
            }
        }

        for (i = 0; i < ido; i++)
        {
            for (k = 0; k < l1; k++)
                ch[i+k*ido] = cc[i+k*ip*ido];
        }
    }

    idl = 2 - ido;
    inc = 0;

    for (l = 1; l < ipph; l++)
    {
        lc = ip - l;
        idl += ido;

        for (ik = 0; ik < idl1; ik++)
        {
            cc[ik+l*idl1] = ch[ik] + wa[idl-2]*ch[ik+idl1];
            cc[ik+lc*idl1] = isign*wa[idl-1]*ch[ik+(ip-1)*idl1];
        }

        idlj = idl;
        inc += ido;

        for (j = 2; j < ipph; j++)
        {
            jc = ip - j;
            idlj += inc;

            if (idlj > idp)
                idlj -= idp;

            war = wa[idlj-2];
            wai = wa[idlj-1];

            for (ik = 0; ik < idl1; ik++)
            {
                cc[ik+l*idl1] += war*ch[ik+j*idl1];
                cc[ik+lc*idl1] += isign*wai*ch[ik+jc*idl1];
            }
        }
    }

    for (j = 1; j < ipph; j++)
    {
        for (ik = 0; ik < idl1; ik++)
            ch[ik] += ch[ik+j*idl1];
    }

    for (j = 1; j < ipph; j++)
    {
        jc = ip - j;

        for (ik = 1; ik < idl1; ik += 2)
        {
            ch[ik-1+j*idl1] = cc[ik-1+j*idl1] - cc[ik+jc*idl1];
            ch[ik-1+jc*idl1] = cc[ik-1+j*idl1] + cc[ik+jc*idl1];
            ch[ik+j*idl1] = cc[ik+j*idl1] + cc[ik-1+jc*idl1];
            ch[ik+jc*idl1] = cc[ik+j*idl1] - cc[ik-1+jc*idl1];
        }
    }

    *nac = 1;

    if (ido == 2)
        return;

    *nac = 0;

    for (ik = 0; ik < idl1; ik++)
        cc[ik] = ch[ik];

    for (j = 1; j < ip; j++)
    {
        for (k = 0; k < l1; k++)
        {
            cc[(k+j*l1)*ido+0] = ch[(k+j*l1)*ido+0];
            cc[(k+j*l1)*ido+1] = ch[(k+j*l1)*ido+1];
        }
    }

    if (idot <= l1)
    {
        idij = 0;

        for (j = 1; j < ip; j++)
        {
            idij += 2;

            for (i = 3; i < ido; i += 2)
            {
                idij += 2;

                for (k = 0; k < l1; k++)
                {
                    cc[i-1+(k+j*l1)*ido] = wa[idij-2] * ch[i-1+(k+j*l1)*ido] -
                        isign * wa[idij-1] * ch[i+(k+j*l1)*ido];
                    cc[i+(k+j*l1)*ido] = wa[idij-2] * ch[i+(k+j*l1)*ido] +
                        isign * wa[idij-1] * ch[i-1+(k+j*l1)*ido];
                }
            }
        }
    } else {
        idj = 2 - ido;

        for (j = 1; j < ip; j++)
        {
            idj += ido;

            for (k = 0; k < l1; k++)
            {
                idij = idj;

                for (i = 3; i < ido; i += 2)
                {
                    idij += 2;
                    cc[i-1+(k+j*l1)*ido] = wa[idij-2] * ch[i-1+(k+j*l1)*ido] -
                        isign * wa[idij-1] * ch[i+(k+j*l1)*ido];
                    cc[i+(k+j*l1)*ido] = wa[idij-2] * ch[i+(k+j*l1)*ido] +
                        isign * wa[idij-1] * ch[i-1+(k+j*l1)*ido];
                }
            }
        }
    }
}



/*----------------------------------------------------------------------
   cfftf1, cfftf, cfftb, cffti1, cffti. Complex FFTs.
  ----------------------------------------------------------------------*/

INLINE void cfftf1(uint16_t n, real_t *c, real_t *ch, real_t *wa,
                   uint16_t *ifac, int8_t isign)
{
    uint16_t idot, i;
    uint16_t k1, l1, l2;
    uint16_t na, nf, ip, iw, ix2, ix3, ix4, nac, ido, idl1;

    nf = ifac[1];
    na = 0;
    l1 = 1;
    iw = 0;

    for (k1 = 2; k1 <= nf+1; k1++)
    {
        ip = ifac[k1];
        l2 = ip*l1;
        ido = n / l2;
        idot = ido+ido;
        idl1 = idot*l1;

        switch (ip)
        {
        case 2:
            if (na == 0)
                passf2(idot, l1, c, ch, &wa[iw], isign);
            else
                passf2(idot, l1, ch, c, &wa[iw], isign);

            na = 1 - na;
            break;
        case 3:
            ix2 = iw + idot;

            if (na == 0)
                passf3(idot, l1, c, ch, &wa[iw], &wa[ix2], isign);
            else
                passf3(idot, l1, ch, c, &wa[iw], &wa[ix2], isign);

            na = 1 - na;
            break;
        case 4:
            ix2 = iw + idot;
            ix3 = ix2 + idot;

            if (na == 0)
                passf4(idot, l1, c, ch, &wa[iw], &wa[ix2], &wa[ix3], isign);
            else
                passf4(idot, l1, ch, c, &wa[iw], &wa[ix2], &wa[ix3], isign);

            na = 1 - na;
            break;
        case 5:
            ix2 = iw + idot;
            ix3 = ix2 + idot;
            ix4 = ix3 + idot;

            if (na == 0)
                passf5(idot, l1, c, ch, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);
            else
                passf5(idot, l1, ch, c, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);

            na = 1 - na;
            break;
        default:
            if (na == 0)
                passf(&nac, idot, ip, l1, idl1, c, ch, &wa[iw], isign);
            else
                passf(&nac, idot, ip, l1, idl1, ch, c, &wa[iw], isign);

            if (nac != 0)
                na = 1 - na;
            break;
        }

        l1 = l2;
        iw += (ip-1) * idot;
    }

    if (na == 0)
        return;

    for (i = 0; i < 2*n; i++)
        c[i] = ch[i];
}

void cfftf(cfft_info *cfft, real_t *c)
{
    cfftf1(cfft->n, c, cfft->work, cfft->tab, cfft->ifac, -1);
}

void cfftb(cfft_info *cfft, real_t *c)
{
    cfftf1(cfft->n, c, cfft->work, cfft->tab, cfft->ifac, +1);
}

static void cffti1(uint16_t n, real_t *wa, uint16_t *ifac)
{
    static uint16_t ntryh[4] = {3, 4, 2, 5};
    real_t arg, argh, argld, fi;
    uint16_t idot, ntry, i, j;
    uint16_t i1, k1, l1, l2, ib;
    uint16_t ld, ii, nf, ip, nl, nq, nr;
    uint16_t ido, ipm;

    nl = n;
    nf = 0;
    j = 0;

startloop:
    j++;

    if (j <= 4)
        ntry = ntryh[j-1];
    else
        ntry += 2;

    do
    {
        nq = nl / ntry;
        nr = nl - ntry*nq;

        if (nr != 0)
            goto startloop;

        nf++;
        ifac[nf+1] = ntry;
        nl = nq;

        if (ntry == 2 && nf != 1)
        {
            for (i = 2; i <= nf; i++)
            {
                ib = nf - i + 2;
                ifac[ib+1] = ifac[ib];
            }
            ifac[2] = 2;
        }
    } while (nl != 1);

    ifac[0] = n;
    ifac[1] = nf;
    argh = 2*M_PI / (real_t)n;
    i = 1;
    l1 = 1;

    for (k1 = 1; k1 <= nf; k1++)
    {
        ip = ifac[k1+1];
        ld = 0;
        l2 = l1*ip;
        ido = n / l2;
        idot = ido + ido + 2;
        ipm = ip - 1;

        for (j = 1; j <= ipm; j++)
        {
            i1 = i;
            wa[i-1] = 1;
            wa[i] = 0;
            ld += l1;
            fi = 0;
            argld = ld*argh;

            for (ii = 4; ii <= idot; ii += 2)
            {
                i += 2;
                fi += 1;
                arg = fi*argld;
                wa[i-1] = cos(arg);
                wa[i] = sin(arg);
            }

            if (ip > 5)
            {
                wa[i1-1] = wa[i-1];
                wa[i1] = wa[i];
            }
        }
        l1 = l2;
    }
}

cfft_info *cffti(uint16_t n)
{
    cfft_info *cfft = malloc(sizeof(cfft_info));

    cfft->n = n;
    cfft->work = malloc(2*n*sizeof(real_t));
    cfft->tab = malloc(2*n*sizeof(real_t));

    cffti1(n, cfft->tab, cfft->ifac);

    return cfft;
}

void cfftu(cfft_info *cfft)
{
    if (cfft->work) free(cfft->work);
    if (cfft->tab) free(cfft->tab);

    if (cfft) free(cfft);
}