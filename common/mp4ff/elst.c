#include "mp4ff.h"


int mp4ff_elst_table_init(mp4ff_elst_table_t *table)
{
	table->duration = 0;
	table->time = 0;
	table->rate = 1;
}

int mp4ff_elst_table_delete(mp4ff_elst_table_t *table)
{
}

int mp4ff_read_elst_table(mp4ff_t *file, mp4ff_elst_table_t *table)
{
	table->duration = mp4ff_read_int32(file);
	table->time = mp4ff_read_int32(file);
	table->rate = mp4ff_read_fixed32(file);
}

int mp4ff_write_elst_table(mp4ff_t *file, mp4ff_elst_table_t *table, long duration)
{
	table->duration = duration;
	mp4ff_write_int32(file, table->duration);
	mp4ff_write_int32(file, table->time);
	mp4ff_write_fixed32(file, table->rate);
}

int mp4ff_elst_table_dump(mp4ff_elst_table_t *table)
{
	printf("    edit list table\n");
	printf("     duration %ld\n", table->duration);
	printf("     time %ld\n", table->time);
	printf("     rate %f\n", table->rate);
}

int mp4ff_elst_init(mp4ff_elst_t *elst)
{
	elst->version = 0;
	elst->flags = 0;
	elst->total_entries = 0;
	elst->table = 0;
}

int mp4ff_elst_init_all(mp4ff_elst_t *elst)
{
	if(!elst->total_entries)
	{
		elst->total_entries = 1;
		elst->table = (mp4ff_elst_table_t*)malloc(sizeof(mp4ff_elst_table_t) * elst->total_entries);
		mp4ff_elst_table_init(&(elst->table[0]));
	}
}

int mp4ff_elst_delete(mp4ff_elst_t *elst)
{
	int i;
	if(elst->total_entries)
	{
		for(i = 0; i < elst->total_entries; i++)
			mp4ff_elst_table_delete(&(elst->table[i]));
		free(elst->table);
	}
	elst->total_entries = 0;
}

int mp4ff_elst_dump(mp4ff_elst_t *elst)
{
	int i;
	printf("   edit list (elst)\n");
	printf("    version %d\n", elst->version);
	printf("    flags %d\n", elst->flags);
	printf("    total_entries %d\n", elst->total_entries);

	for(i = 0; i < elst->total_entries; i++)
	{
		mp4ff_elst_table_dump(&(elst->table[i]));
	}
}

int mp4ff_read_elst(mp4ff_t *file, mp4ff_elst_t *elst)
{
	int i;

	elst->version = mp4ff_read_char(file);
	elst->flags = mp4ff_read_int24(file);
	elst->total_entries = mp4ff_read_int32(file);
	elst->table = (mp4ff_elst_table_t*)malloc(sizeof(mp4ff_elst_table_t) * elst->total_entries);
	for(i = 0; i < elst->total_entries; i++)
	{
		mp4ff_elst_table_init(&(elst->table[i]));
		mp4ff_read_elst_table(file, &(elst->table[i]));
	}
}

int mp4ff_write_elst(mp4ff_t *file, mp4ff_elst_t *elst, long duration)
{
	mp4ff_atom_t atom;
	int i;
	mp4ff_atom_write_header(file, &atom, "elst");

	mp4ff_write_char(file, elst->version);
	mp4ff_write_int24(file, elst->flags);
	mp4ff_write_int32(file, elst->total_entries);
	for(i = 0; i < elst->total_entries; i++)
	{
		mp4ff_write_elst_table(file, elst->table, duration);
	}

	mp4ff_atom_write_footer(file, &atom);
}
