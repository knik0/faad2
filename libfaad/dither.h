/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (with filename LICENSE.LGPL).
 *
 * (c) 2002 John Edwards
 *
 * rand_t header.
 *
 * last modified: $Id: dither.h,v 1.1 2002/08/13 19:16:07 menno Exp $
 */

#ifndef __RAND_T_H
#define __RAND_T_H

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct {
    double                 Add;
    float                  Dither;
    int                    LastRandomNumber [2];
} dither_t;

extern dither_t            Dither;
extern double              doubletmp;
static const unsigned char Parity [256];
unsigned int               random_int ( void );
extern double              Random_Equi ( double mult );
extern double              Random_Triangular ( double mult );
extern double              Init_Dither ( int bits );

#ifdef __cplusplus
}
#endif 

#endif __RAND_T_H

