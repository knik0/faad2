#include "mp4ff.h"




int mp4ff_matrix_init(mp4ff_matrix_t *matrix)
{
	int i;
	for(i = 0; i < 9; i++) matrix->values[i] = 0;
	matrix->values[0] = matrix->values[4] = 1;
	matrix->values[8] = 16384;
}

int mp4ff_matrix_delete(mp4ff_matrix_t *matrix)
{
}

int mp4ff_read_matrix(mp4ff_t *file, mp4ff_matrix_t *matrix)
{
	int i = 0;
	for(i = 0; i < 9; i++)
	{
		matrix->values[i] = mp4ff_read_fixed32(file);
	}
}

int mp4ff_matrix_dump(mp4ff_matrix_t *matrix)
{
	int i;
	printf("   matrix");
	for(i = 0; i < 9; i++) printf(" %f", matrix->values[i]);
	printf("\n");
}

int mp4ff_write_matrix(mp4ff_t *file, mp4ff_matrix_t *matrix)
{
	int i;
	for(i = 0; i < 9; i++)
	{
		mp4ff_write_fixed32(file, matrix->values[i]);
	}
}
