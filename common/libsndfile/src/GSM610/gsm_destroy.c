/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* $Header: /home/cvs/f/fa/faac/faad2/common/libsndfile/src/GSM610/Attic/gsm_destroy.c,v 1.2 2002/07/25 12:22:13 menno Exp $ */

#include "gsm.h"
#include "config.h"

#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#else
#	ifdef	HAS_MALLOC_H
#		include 	<malloc.h>
#	else
		extern void free();
#	endif
#endif

void gsm_destroy (gsm S)
{
	if (S) free((char *)S);
}
