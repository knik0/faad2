#include "mp4ff.h"


int mp4ff_stss_init(mp4ff_stss_t *stss)
{
	stss->version = 0;
	stss->flags = 0;
	stss->total_entries = 0;
	stss->entries_allocated = 0;
}

int mp4ff_stss_init_common(mp4ff_t *file, mp4ff_stss_t *stss)
{
	if (stss->entries_allocated == 0) {
		stss->entries_allocated = 100;
		stss->table = (mp4ff_stss_table_t*)
			malloc(sizeof(mp4ff_stss_table_t) * stss->entries_allocated);
	}
}

int mp4ff_stss_delete(mp4ff_stss_t *stss)
{
	if(stss->total_entries) 
		free(stss->table);
	stss->total_entries = 0;
}

int mp4ff_stss_dump(mp4ff_stss_t *stss)
{
	int i;
	printf("     sync sample\n");
	printf("      version %d\n", stss->version);
	printf("      flags %d\n", stss->flags);
	printf("      total_entries %d\n", stss->total_entries);
	for(i = 0; i < stss->total_entries; i++)
	{
		printf("       sample %u\n", stss->table[i].sample);
	}
}

int mp4ff_read_stss(mp4ff_t *file, mp4ff_stss_t *stss)
{
	int i;
	stss->version = mp4ff_read_char(file);
	stss->flags = mp4ff_read_int24(file);
	stss->total_entries = mp4ff_read_int32(file);
	
	stss->table = (mp4ff_stss_table_t*)malloc(sizeof(mp4ff_stss_table_t) * stss->total_entries);
	for(i = 0; i < stss->total_entries; i++)
	{
		stss->table[i].sample = mp4ff_read_int32(file);
	}
}


int mp4ff_write_stss(mp4ff_t *file, mp4ff_stss_t *stss)
{
	int i;
	mp4ff_atom_t atom;

	if(stss->total_entries)
	{
		mp4ff_atom_write_header(file, &atom, "stss");

		mp4ff_write_char(file, stss->version);
		mp4ff_write_int24(file, stss->flags);
		mp4ff_write_int32(file, stss->total_entries);
		for(i = 0; i < stss->total_entries; i++)
		{
			mp4ff_write_int32(file, stss->table[i].sample);
		}

		mp4ff_atom_write_footer(file, &atom);
	}
}

int mp4ff_update_stss(mp4ff_stss_t *stss, long sample)
{
	if (stss->total_entries >= stss->entries_allocated) {
		stss->entries_allocated *= 2;
		stss->table = (mp4ff_stss_table_t*)realloc(stss->table,
			sizeof(mp4ff_stss_table_t) * stss->entries_allocated);
	}
	
	stss->table[stss->total_entries++].sample = sample;
}

