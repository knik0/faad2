#include "mp4ff.h"



int mp4ff_stts_init(mp4ff_stts_t *stts)
{
	stts->version = 0;
	stts->flags = 0;
	stts->total_entries = 0;
	stts->entries_allocated = 0;
}

int mp4ff_stts_init_table(mp4ff_stts_t *stts)
{
	if(!stts->entries_allocated)
	{
		stts->entries_allocated = 1;
		stts->table = (mp4ff_stts_table_t*)
			malloc(sizeof(mp4ff_stts_table_t) * stts->entries_allocated);
		stts->total_entries = 1;
	}
}

int mp4ff_stts_init_video(mp4ff_t *file, mp4ff_stts_t *stts, int time_scale, float frame_rate)
{
	mp4ff_stts_table_t *table;
	mp4ff_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;      /* need to set this when closing */
	table->sample_duration = time_scale / frame_rate;
}

int mp4ff_stts_init_audio(mp4ff_t *file, mp4ff_stts_t *stts, int time_scale, int sample_duration)
{
	mp4ff_stts_table_t *table;
	mp4ff_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;     /* need to set this when closing or via update function */
	table->sample_duration = sample_duration;
}

int mp4ff_stts_delete(mp4ff_stts_t *stts)
{
	if(stts->total_entries) free(stts->table);
	stts->total_entries = 0;
}

int mp4ff_stts_dump(mp4ff_stts_t *stts)
{
	int i;
	printf("     time to sample\n");
	printf("      version %d\n", stts->version);
	printf("      flags %d\n", stts->flags);
	printf("      total_entries %d\n", stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		printf("       count %ld duration %ld\n", stts->table[i].sample_count, stts->table[i].sample_duration);
	}
}

int mp4ff_read_stts(mp4ff_t *file, mp4ff_stts_t *stts)
{
	int i;
	stts->version = mp4ff_read_char(file);
	stts->flags = mp4ff_read_int24(file);
	stts->total_entries = mp4ff_read_int32(file);

	stts->table = (mp4ff_stts_table_t*)malloc(sizeof(mp4ff_stts_table_t) * stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		stts->table[i].sample_count = mp4ff_read_int32(file);
		stts->table[i].sample_duration = mp4ff_read_int32(file);
	}
}

int mp4ff_write_stts(mp4ff_t *file, mp4ff_stts_t *stts)
{
	int i;
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "stts");

	mp4ff_write_char(file, stts->version);
	mp4ff_write_int24(file, stts->flags);
	mp4ff_write_int32(file, stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		mp4ff_write_int32(file, stts->table[i].sample_count);
		mp4ff_write_int32(file, stts->table[i].sample_duration);
	}
	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_stts(mp4ff_stts_t *stts, long sample_duration)
{
	if (sample_duration == stts->table[stts->total_entries-1].sample_duration) {
		stts->table[stts->total_entries-1].sample_count++;
	} else {
		/* need a new entry in the table */

		/* allocate more entries if necessary */
		if (stts->total_entries >= stts->entries_allocated) {
			stts->entries_allocated *= 2;
			stts->table = (mp4ff_stts_table_t*)realloc(stts->table,
				sizeof(mp4ff_stts_table_t) * stts->entries_allocated);
		}
	
		stts->table[stts->total_entries].sample_count = 1;
		stts->table[stts->total_entries].sample_duration = sample_duration;
		stts->total_entries++;
	}
}
