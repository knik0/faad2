#include "mp4ff.h"



int mp4ff_stco_init(mp4ff_stco_t *stco)
{
	stco->version = 0;
	stco->flags = 0;
	stco->total_entries = 0;
	stco->entries_allocated = 0;
}

int mp4ff_stco_delete(mp4ff_stco_t *stco)
{
	if(stco->total_entries) free(stco->table);
	stco->total_entries = 0;
	stco->entries_allocated = 0;
}

int mp4ff_stco_init_common(mp4ff_t *file, mp4ff_stco_t *stco)
{
	if(!stco->entries_allocated)
	{
		stco->entries_allocated = 2000;
		stco->total_entries = 0;
		stco->table = (mp4ff_stco_table_t*)malloc(sizeof(mp4ff_stco_table_t) * stco->entries_allocated);
/*printf("mp4ff_stco_init_common %x\n", stco->table); */
	}
}

int mp4ff_stco_dump(mp4ff_stco_t *stco)
{
	int i;
	printf("     chunk offset\n");
	printf("      version %d\n", stco->version);
	printf("      flags %d\n", stco->flags);
	printf("      total_entries %d\n", stco->total_entries);
	for(i = 0; i < stco->total_entries; i++)
	{
		printf("       offset %d %x\n", i, stco->table[i].offset);
	}
}

int mp4ff_read_stco(mp4ff_t *file, mp4ff_stco_t *stco)
{
	int i;
	stco->version = mp4ff_read_char(file);
	stco->flags = mp4ff_read_int24(file);
	stco->total_entries = mp4ff_read_int32(file);
/*printf("stco->total_entries %d %x\n", stco->total_entries, stco); */
	stco->entries_allocated = stco->total_entries;
	stco->table = (mp4ff_stco_table_t*)malloc(sizeof(mp4ff_stco_table_t) * stco->entries_allocated);
	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset = mp4ff_read_int32(file);
	}
}

int mp4ff_write_stco(mp4ff_t *file, mp4ff_stco_t *stco)
{
	int i;
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "stco");

	mp4ff_write_char(file, stco->version);
	mp4ff_write_int24(file, stco->flags);
	mp4ff_write_int32(file, stco->total_entries);
	for(i = 0; i < stco->total_entries; i++)
	{
		mp4ff_write_int32(file, stco->table[i].offset);
	}

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_stco(mp4ff_stco_t *stco, long chunk, long offset)
{
	mp4ff_stco_table_t *new_table;
	long i;

	if(chunk > stco->entries_allocated)
	{
		stco->entries_allocated = chunk * 2;
		stco->table = (mp4ff_stco_table_t*)realloc(stco->table,
			sizeof(mp4ff_stco_table_t) * stco->entries_allocated);
	}
	
	stco->table[chunk - 1].offset = offset;
	if(chunk > stco->total_entries) 
		stco->total_entries = chunk;
}

