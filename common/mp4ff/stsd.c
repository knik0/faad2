#include "mp4ff.h"


int mp4ff_stsd_init(mp4ff_stsd_t *stsd)
{
	stsd->version = 0;
	stsd->flags = 0;
	stsd->total_entries = 0;
}

int mp4ff_stsd_init_table(mp4ff_stsd_t *stsd)
{
	if(!stsd->total_entries)
	{
		stsd->total_entries = 1;
		stsd->table = (mp4ff_stsd_table_t*)malloc(sizeof(mp4ff_stsd_table_t) * stsd->total_entries);
		mp4ff_stsd_table_init(&(stsd->table[0]));
	}
}

int mp4ff_stsd_init_video(mp4ff_t *file, 
								mp4ff_stsd_t *stsd, 
								int frame_w,
								int frame_h, 
								float frame_rate,
								char *compression)
{
	mp4ff_stsd_table_t *table;
	mp4ff_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	mp4ff_copy_char32(table->format, compression);
	table->width = frame_w;
	table->height = frame_h;
	table->frames_per_sample = 1;
	table->depth = 24;
}

int mp4ff_stsd_init_audio(mp4ff_t *file, 
							mp4ff_stsd_t *stsd, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor)
{
	mp4ff_stsd_table_t *table;
	mp4ff_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	mp4ff_copy_char32(table->format, compressor);
	table->channels = channels;
	table->sample_size = bits;
	table->sample_rate = sample_rate;
}

int mp4ff_stsd_delete(mp4ff_stsd_t *stsd)
{
	int i;
	if(stsd->total_entries)
	{
		for(i = 0; i < stsd->total_entries; i++)
			mp4ff_stsd_table_delete(&(stsd->table[i]));
		free(stsd->table);
	}

	stsd->total_entries = 0;
}

int mp4ff_stsd_dump(void *minf_ptr, mp4ff_stsd_t *stsd)
{
	int i;
	printf("     sample description\n");
	printf("      version %d\n", stsd->version);
	printf("      flags %d\n", stsd->flags);
	printf("      total_entries %d\n", stsd->total_entries);
	
	for(i = 0; i < stsd->total_entries; i++)
	{
		mp4ff_stsd_table_dump(minf_ptr, &(stsd->table[i]));
	}
}

int mp4ff_read_stsd(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stsd_t *stsd)
{
	int i;
	mp4ff_atom_t leaf_atom;

	stsd->version = mp4ff_read_char(file);
	stsd->flags = mp4ff_read_int24(file);
	stsd->total_entries = mp4ff_read_int32(file);
	stsd->table = (mp4ff_stsd_table_t*)malloc(sizeof(mp4ff_stsd_table_t) * stsd->total_entries);
	for(i = 0; i < stsd->total_entries; i++)
	{
		mp4ff_stsd_table_init(&(stsd->table[i]));
		mp4ff_read_stsd_table(file, minf, &(stsd->table[i]));
	}
}

int mp4ff_write_stsd(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stsd_t *stsd)
{
	mp4ff_atom_t atom;
	int i;
	mp4ff_atom_write_header(file, &atom, "stsd");

	mp4ff_write_char(file, stsd->version);
	mp4ff_write_int24(file, stsd->flags);
	mp4ff_write_int32(file, stsd->total_entries);
	for(i = 0; i < stsd->total_entries; i++)
	{
		mp4ff_write_stsd_table(file, minf, stsd->table);
	}

	mp4ff_atom_write_footer(file, &atom);
}



