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
** $Id: cfft.h,v 1.2 2002/08/17 10:03:11 menno Exp $
**/

#ifndef __CFFT_H__
#define __CFFT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    real_t *work;
    real_t *tab;
    uint16_t ifac[15];
    uint16_t n;
} cfft_info;

void cfftf(cfft_info *cfft, real_t *c); // complex transform             
void cfftb(cfft_info *cfft, real_t *c); // its inverse                   
cfft_info *cffti(uint16_t n);    // initializer of the above routine pair
void cfftu(cfft_info *cfft);


static void passf2(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, int8_t isign);
static void passf3(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, int8_t isign);
static void passf4(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, real_t *wa3, int8_t isign);
static void passf5(uint16_t ido, uint16_t l1, real_t *cc, real_t *ch,
                   real_t *wa1, real_t *wa2, real_t *wa3, real_t *wa4,
                   int8_t isign);
static void passf(uint16_t *nac, uint16_t ido, uint16_t ip, uint16_t l1,
                  uint16_t idl1, real_t *cc, real_t *ch, real_t *wa,
                  int8_t isign);
INLINE void cfftf1(uint16_t n, real_t *c, real_t *ch, real_t *wa,
                   uint16_t *ifac, int8_t isign);
static void cffti1(uint16_t n, real_t *wa, uint16_t *ifac);


#ifdef __cplusplus
}
#endif
#endif
