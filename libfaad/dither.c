/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (with filename LICENSE.LGPL).
 *
 * (c) 2002 John Edwards
 * mostly lifted from work by Frank Klemm
 * random functions for dithering.
 *
 * last modified: $Id: dither.c,v 1.1 2002/08/13 19:16:07 menno Exp $
 */
#include "dither.h"

static const  unsigned char    Parity [256] = {  // parity
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

static unsigned int  __r1 = 1;
static unsigned int  __r2 = 1;


/*
 *  This is a simple random number generator with good quality for audio purposes.
 *  It consists of two polycounters with opposite rotation direction and different
 *  periods. The periods are coprime, so the total period is the product of both.
 *
 *     -------------------------------------------------------------------------------------------------
 * +-> |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0|
 * |   -------------------------------------------------------------------------------------------------
 * |                                                                          |  |  |  |     |        |
 * |                                                                          +--+--+--+-XOR-+--------+
 * |                                                                                      |
 * +--------------------------------------------------------------------------------------+
 *
 *     -------------------------------------------------------------------------------------------------
 *     |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0| <-+
 *     -------------------------------------------------------------------------------------------------   |
 *       |  |           |  |                                                                               |
 *       +--+----XOR----+--+                                                                               |
 *                |                                                                                        |
 *                +----------------------------------------------------------------------------------------+
 *
 *
 *  The first has an period of 3*5*17*257*65537, the second of 7*47*73*178481,
 *  which gives a period of 18.410.713.077.675.721.215. The result is the
 *  XORed values of both generators.
 */


unsigned int
random_int ( void )
{
	unsigned int  t1, t2, t3, t4;

	t3   = t1 = __r1;   t4   = t2 = __r2;       // Parity calculation is done via table lookup, this is also available
	t1  &= 0xF5;        t2 >>= 25;              // on CPUs without parity, can be implemented in C and avoid unpredictable
	t1   = Parity [t1]; t2  &= 0x63;            // jumps and slow rotate through the carry flag operations.
	t1 <<= 31;          t2   = Parity [t2];

	return (__r1 = (t3 >> 1) | t1 ) ^ (__r2 = (t4 + t4) | t2 );
}



double
Random_Equi ( double mult )                     // gives a equal distributed random number
{                                               // between -2^31*mult and +2^31*mult
	return mult * (int) random_int ();
}

double
Random_Triangular ( double mult )               // gives a triangular distributed random number
{                                               // between -2^32*mult and +2^32*mult
	return mult * ( (double) (int) random_int () + (double) (int) random_int () );
}


double
Init_Dither(int bits)
{
	static unsigned char        default_dither [] = { 92, 92, 88, 84, 81, 78, 74, 67,  0,  0 };
	int                         index;

	index = bits - 11;
	if (index < 0) index = 0;
	if (index > 9) index = 9;

	Dither.Add    = 0.5     * ((1L << (32 - bits)) - 1);

	Dither.Dither = (0.01*default_dither[index]) / (((__int64)1) << bits);
}



