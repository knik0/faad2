#include "mp4ff.h"



int mp4ff_stsz_init(mp4ff_stsz_t *stsz)
{
	stsz->version = 0;
	stsz->flags = 0;
	stsz->sample_size = 0;
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
}

int mp4ff_stsz_init_video(mp4ff_t *file, mp4ff_stsz_t *stsz)
{
	stsz->sample_size = 0;
	if(!stsz->entries_allocated)
	{
		stsz->entries_allocated = 2000;
		stsz->total_entries = 0;
		stsz->table = (mp4ff_stsz_table_t*)malloc(sizeof(mp4ff_stsz_table_t) * stsz->entries_allocated);
	}
}

int mp4ff_stsz_init_audio(mp4ff_t *file, mp4ff_stsz_t *stsz, int sample_size)
{
	stsz->sample_size = sample_size;	/* if == 0, then use table */
	stsz->total_entries = 0;   /* set this when closing */
	stsz->entries_allocated = 0;
}

int mp4ff_stsz_delete(mp4ff_stsz_t *stsz)
{
	if(!stsz->sample_size && stsz->total_entries) 
		free(stsz->table);
	stsz->total_entries = 0;
	stsz->entries_allocated = 0;
}

int mp4ff_stsz_dump(mp4ff_stsz_t *stsz)
{
	int i;
	printf("     sample size\n");
	printf("      version %d\n", stsz->version);
	printf("      flags %d\n", stsz->flags);
	printf("      sample_size %d\n", stsz->sample_size);
	printf("      total_entries %d\n", stsz->total_entries);
	
	if(!stsz->sample_size)
	{
		for(i = 0; i < stsz->total_entries; i++)
		{
			printf("       sample_size %d\n", stsz->table[i].size);
		}
	}
}

int mp4ff_read_stsz(mp4ff_t *file, mp4ff_stsz_t *stsz)
{
	int i;
	stsz->version = mp4ff_read_char(file);
	stsz->flags = mp4ff_read_int24(file);
	stsz->sample_size = mp4ff_read_int32(file);
	stsz->total_entries = mp4ff_read_int32(file);
	stsz->entries_allocated = stsz->total_entries;
	if(!stsz->sample_size)
	{
		stsz->table = (mp4ff_stsz_table_t*)malloc(sizeof(mp4ff_stsz_table_t) * stsz->entries_allocated);
		for(i = 0; i < stsz->total_entries; i++)
		{
			stsz->table[i].size = mp4ff_read_int32(file);
		}
	}
}

int mp4ff_write_stsz(mp4ff_t *file, mp4ff_stsz_t *stsz)
{
	int i, result;
	mp4ff_atom_t atom;

	mp4ff_atom_write_header(file, &atom, "stsz");

/* optimize if possible */
/* Xanim requires an unoptimized table for video. */
/* 	if(!stsz->sample_size) */
/* 	{ */
/* 		for(i = 0, result = 0; i < stsz->total_entries && !result; i++) */
/* 		{ */
/* 			if(stsz->table[i].size != stsz->table[0].size) result = 1; */
/* 		} */
/* 		 */
/* 		if(!result) */
/* 		{ */
/* 			stsz->sample_size = stsz->table[0].size; */
/* 			stsz->total_entries = 0; */
/* 			free(stsz->table); */
/* 		} */
/* 	} */

	mp4ff_write_char(file, stsz->version);
	mp4ff_write_int24(file, stsz->flags);
	mp4ff_write_int32(file, stsz->sample_size);
	if(stsz->sample_size)
	{
		mp4ff_write_int32(file, stsz->total_entries);
	}
	else
	{
		mp4ff_write_int32(file, stsz->total_entries);
		for(i = 0; i < stsz->total_entries; i++)
		{
			mp4ff_write_int32(file, stsz->table[i].size);
		}
	}

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_stsz(mp4ff_stsz_t *stsz, long sample, long sample_size)
{
	mp4ff_stsz_table_t *new_table;
	int i;

	if(!stsz->sample_size)
	{
		if(sample >= stsz->entries_allocated)
		{
			stsz->entries_allocated = sample * 2;
			stsz->table = (mp4ff_stsz_table_t *)realloc(stsz->table,
				sizeof(mp4ff_stsz_table_t) * stsz->entries_allocated);
		}

		stsz->table[sample].size = sample_size;
		if(sample >= stsz->total_entries) 
			stsz->total_entries = sample + 1;
	}
}
