#ifndef __AACUTILS_H__
#define __AACUTILS_H__

#define ADTS_HEADER_SIZE	8
#define SEEK_TABLE_CHUNK	60
#define MPEG4_TYPE		0
#define MPEG2_TYPE		1

int	getAacInfo(FILE *);
void	checkADTSForSeeking(FILE *, unsigned long **, unsigned long *);
#endif
