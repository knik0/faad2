#include "mp4ff.h"



int mp4ff_stsc_init(mp4ff_stsc_t *stsc)
{
	stsc->version = 0;
	stsc->flags = 0;
	stsc->total_entries = 0;
	stsc->entries_allocated = 0;
}

int mp4ff_stsc_init_table(mp4ff_t *file, mp4ff_stsc_t *stsc)
{
	if(!stsc->total_entries)
	{
		stsc->total_entries = 1;
		stsc->entries_allocated = 1;
		stsc->table = (mp4ff_stsc_table_t*)malloc(sizeof(mp4ff_stsc_table_t) * stsc->entries_allocated);
	}
}

int mp4ff_stsc_init_video(mp4ff_t *file, mp4ff_stsc_t *stsc)
{
	mp4ff_stsc_table_t *table;
	mp4ff_stsc_init_table(file, stsc);
	table = &(stsc->table[0]);
	table->chunk = 1;
	table->samples = 1;
	table->id = 1;
}

int mp4ff_stsc_init_audio(mp4ff_t *file, mp4ff_stsc_t *stsc)
{
	mp4ff_stsc_table_t *table;
	mp4ff_stsc_init_table(file, stsc);
	table = &(stsc->table[0]);
	table->chunk = 1;
	table->samples = 0;         /* set this after completion or after every audio chunk is written */
	table->id = 1;
}

int mp4ff_stsc_delete(mp4ff_stsc_t *stsc)
{
	if(stsc->total_entries) free(stsc->table);
	stsc->total_entries = 0;
}

int mp4ff_stsc_dump(mp4ff_stsc_t *stsc)
{
	int i;
	printf("     sample to chunk\n");
	printf("      version %d\n", stsc->version);
	printf("      flags %d\n", stsc->flags);
	printf("      total_entries %d\n", stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		printf("       chunk %d samples %d id %d\n", 
			stsc->table[i].chunk, stsc->table[i].samples, stsc->table[i].id);
	}
}

int mp4ff_read_stsc(mp4ff_t *file, mp4ff_stsc_t *stsc)
{
	int i;
	stsc->version = mp4ff_read_char(file);
	stsc->flags = mp4ff_read_int24(file);
	stsc->total_entries = mp4ff_read_int32(file);
	
	stsc->entries_allocated = stsc->total_entries;
	stsc->table = (mp4ff_stsc_table_t*)malloc(sizeof(mp4ff_stsc_table_t) * stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		stsc->table[i].chunk = mp4ff_read_int32(file);
		stsc->table[i].samples = mp4ff_read_int32(file);
		stsc->table[i].id = mp4ff_read_int32(file);
	}
}


int mp4ff_write_stsc(mp4ff_t *file, mp4ff_stsc_t *stsc)
{
	int i, last_same;
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "stsc");

	for(i = 1, last_same = 0; i < stsc->total_entries; i++)
	{
		if(stsc->table[i].samples != stsc->table[last_same].samples)
		{
/* An entry has a different sample count. */
			last_same++;
			if(last_same < i)
			{
/* Move it up the list. */
				stsc->table[last_same] = stsc->table[i];
			}
		}
	}
	last_same++;
	stsc->total_entries = last_same;


	mp4ff_write_char(file, stsc->version);
	mp4ff_write_int24(file, stsc->flags);
	mp4ff_write_int32(file, stsc->total_entries);
	for(i = 0; i < stsc->total_entries; i++)
	{
		mp4ff_write_int32(file, stsc->table[i].chunk);
		mp4ff_write_int32(file, stsc->table[i].samples);
		mp4ff_write_int32(file, stsc->table[i].id);
	}

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_stsc(mp4ff_stsc_t *stsc, long chunk, long samples)
{
/*	mp4ff_stsc_table_t *table = stsc->table; */
	mp4ff_stsc_table_t *new_table;
	long i;

	if(chunk > stsc->entries_allocated)
	{
		stsc->entries_allocated = chunk * 2;
		new_table = (mp4ff_stsc_table_t*)malloc(sizeof(mp4ff_stsc_table_t) * stsc->entries_allocated);
		for(i = 0; i < stsc->total_entries; i++) new_table[i] = stsc->table[i];
		free(stsc->table);
		stsc->table = new_table;
	}

	stsc->table[chunk - 1].samples = samples;
	stsc->table[chunk - 1].chunk = chunk;
	stsc->table[chunk - 1].id = 1;
	if(chunk > stsc->total_entries) stsc->total_entries = chunk;
	return 0;
}

/* Optimizing while writing doesn't allow seeks during recording so */
/* entries are created for every chunk and only optimized during */
/* writeout.  Unfortunately there's no way to keep audio synchronized */
/* after overwriting  a recording as the fractional audio chunk in the */
/* middle always overwrites the previous location of a larger chunk.  On */
/* writing, the table must be optimized.  RealProducer requires an  */
/* optimized table. */

