#include "mp4ff.h"


int mp4ff_stsd_table_init(mp4ff_stsd_table_t *table)
{
	int i;
	table->format[0] = 'y';
	table->format[1] = 'u';
	table->format[2] = 'v';
	table->format[3] = '2';
	for(i = 0; i < 6; i++) table->reserved[i] = 0;
	table->data_reference = 1;

	table->version = 0;
	table->revision = 0;
 	table->vendor[0] = 'l';
 	table->vendor[1] = 'n';
 	table->vendor[2] = 'u';
 	table->vendor[3] = 'x';

	table->temporal_quality = 0;
	table->spatial_quality = 258;
	table->width = 0;
	table->height = 0;
	table->dpi_horizontal = 72;
	table->dpi_vertical = 72;
	table->data_size = 0;
	table->frames_per_sample = 1;
	for(i = 0; i < 32; i++) table->compressor_name[i] = 0;
	sprintf(table->compressor_name, "Quicktime for Linux");
	table->depth = 24;
	table->gamma = 0;
	table->fields = 0;
	table->field_dominance = 1;
	
	table->channels = 0;
	table->sample_size = 0;
	table->compression_id = 0;
	table->packet_size = 0;
	table->sample_rate = 0;

	mp4ff_esds_init(&(table->esds));
}

int mp4ff_stsd_table_delete(mp4ff_stsd_table_t *table)
{
	mp4ff_esds_delete(&(table->esds));
}

int mp4ff_stsd_table_dump(void *minf_ptr, mp4ff_stsd_table_t *table)
{
	mp4ff_minf_t *minf = minf_ptr;
	printf("       format %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]);
	mp4ff_print_chars("       reserved ", table->reserved, 6);
	printf("       data_reference %d\n", table->data_reference);

	if(minf->is_audio) mp4ff_stsd_audio_dump(table);
	if(minf->is_video) mp4ff_stsd_video_dump(table);
}

int mp4ff_stsd_audio_dump(mp4ff_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       channels %d\n", table->channels);
	printf("       sample_size %d\n", table->sample_size);
	printf("       compression_id %d\n", table->compression_id);
	printf("       packet_size %d\n", table->packet_size);
	printf("       sample_rate %f\n", table->sample_rate);

	mp4ff_esds_dump(&(table->esds));
}

int mp4ff_stsd_video_dump(mp4ff_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       temporal_quality %ld\n", table->temporal_quality);
	printf("       spatial_quality %ld\n", table->spatial_quality);
	printf("       width %d\n", table->width);
	printf("       height %d\n", table->height);
	printf("       dpi_horizontal %f\n", table->dpi_horizontal);
	printf("       dpi_vertical %f\n", table->dpi_vertical);
	printf("       data_size %ld\n", table->data_size);
	printf("       frames_per_sample %d\n", table->frames_per_sample);
	printf("       compressor_name %s\n", table->compressor_name);
	printf("       depth %d\n", table->depth);
	printf("       gamma %f\n", table->gamma);
	if(table->fields)
	{
		printf("       fields %d\n", table->fields);
		printf("       field dominance %d\n", table->field_dominance);
	}
	mp4ff_esds_dump(&(table->esds));
}

int mp4ff_read_stsd_table(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stsd_table_t *table)
{
	mp4ff_atom_t leaf_atom;

	mp4ff_atom_read_header(file, &leaf_atom);
	
	table->format[0] = leaf_atom.type[0];
	table->format[1] = leaf_atom.type[1];
	table->format[2] = leaf_atom.type[2];
	table->format[3] = leaf_atom.type[3];
	mp4ff_read_data(file, table->reserved, 6);
	table->data_reference = mp4ff_read_int16(file);

	if(minf->is_audio) mp4ff_read_stsd_audio(file, table, &leaf_atom);
	if(minf->is_video) mp4ff_read_stsd_video(file, table, &leaf_atom);
}

int mp4ff_write_stsd_table(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stsd_table_t *table)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, table->format);
/*printf("mp4ff_write_stsd_table %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]); */
	mp4ff_write_data(file, table->reserved, 6);
	mp4ff_write_int16(file, table->data_reference);
	
	if(minf->is_audio) mp4ff_write_stsd_audio(file, table);
	if(minf->is_video) mp4ff_write_stsd_video(file, table);

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_read_stsd_audio(mp4ff_t *file, mp4ff_stsd_table_t *table, mp4ff_atom_t *parent_atom)
{
	table->version = mp4ff_read_int16(file);
	table->revision = mp4ff_read_int16(file);
	mp4ff_read_data(file, table->vendor, 4);
	table->channels = mp4ff_read_int16(file);
	table->sample_size = mp4ff_read_int16(file);
	table->compression_id = mp4ff_read_int16(file);
	table->packet_size = mp4ff_read_int16(file);
	table->sample_rate = mp4ff_read_fixed32(file);

	while (mp4ff_position(file) < parent_atom->end) {
		mp4ff_atom_t leaf_atom;

		mp4ff_atom_read_header(file, &leaf_atom);

		if(mp4ff_atom_is(&leaf_atom, "esds")) {
			mp4ff_read_esds(file, &(table->esds));
			mp4ff_atom_skip(file, &leaf_atom);
		} else {
			mp4ff_atom_skip(file, &leaf_atom);
		}
	}
}

int mp4ff_write_stsd_audio(mp4ff_t *file, mp4ff_stsd_table_t *table)
{
	mp4ff_write_int32(file, 0);
	mp4ff_write_int32(file, 0);
	mp4ff_write_int16(file, 2);
	mp4ff_write_int16(file, 16);
	mp4ff_write_int32(file, 0);
	mp4ff_write_int16(file, table->sample_rate);
	mp4ff_write_int16(file, 0);
	mp4ff_write_esds_audio(file, &(table->esds), file->atracks[0].track->tkhd.track_id);
}

int mp4ff_read_stsd_video(mp4ff_t *file, mp4ff_stsd_table_t *table, mp4ff_atom_t *parent_atom)
{
	mp4ff_atom_t leaf_atom;
	int len;
	
	table->version = mp4ff_read_int16(file);
	table->revision = mp4ff_read_int16(file);
	mp4ff_read_data(file, table->vendor, 4);
	table->temporal_quality = mp4ff_read_int32(file);
	table->spatial_quality = mp4ff_read_int32(file);
	table->width = mp4ff_read_int16(file);
	table->height = mp4ff_read_int16(file);
	table->dpi_horizontal = mp4ff_read_fixed32(file);
	table->dpi_vertical = mp4ff_read_fixed32(file);
	table->data_size = mp4ff_read_int32(file);
	table->frames_per_sample = mp4ff_read_int16(file);
	len = mp4ff_read_char(file);
	mp4ff_read_data(file, table->compressor_name, 31);
	table->depth = mp4ff_read_int16(file);
		
	while(mp4ff_position(file) < parent_atom->end)
	{
		mp4ff_atom_read_header(file, &leaf_atom);
		
		if(mp4ff_atom_is(&leaf_atom, "gama"))
		{
			table->gamma= mp4ff_read_fixed32(file);
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "fiel"))
		{
			table->fields = mp4ff_read_char(file);
			table->field_dominance = mp4ff_read_char(file);
		}
		else
/* 		if(mp4ff_atom_is(&leaf_atom, "mjqt")) */
/* 		{ */
/* 			mp4ff_read_mjqt(file, &(table->mjqt)); */
/* 		} */
/* 		else */
/* 		if(mp4ff_atom_is(&leaf_atom, "mjht")) */
/* 		{ */
/* 			mp4ff_read_mjht(file, &(table->mjht)); */
/* 		} */
/* 		else */
		if(mp4ff_atom_is(&leaf_atom, "esds"))
		{
			mp4ff_read_esds(file, &(table->esds));
			mp4ff_atom_skip(file, &leaf_atom);
		}
		else
		{
			mp4ff_atom_skip(file, &leaf_atom);
		}
	}
}

int mp4ff_write_stsd_video(mp4ff_t *file, mp4ff_stsd_table_t *table)
{
	int i;

	for (i = 0; i < 4; i++) {
		mp4ff_write_int32(file, 0);
	}
	mp4ff_write_int16(file, table->width);
	mp4ff_write_int16(file, table->height);
	mp4ff_write_int32(file, 0x00480000);
	mp4ff_write_int32(file, 0x00480000);
	mp4ff_write_int32(file, 0);
	mp4ff_write_int16(file, 1);
	for (i = 0; i < 32; i++) {
		mp4ff_write_char(file, 0);
	}
	mp4ff_write_int16(file, 24);
	mp4ff_write_int16(file, -1);
	mp4ff_write_esds_video(file, &(table->esds), file->vtracks[0].track->tkhd.track_id);

	if(table->fields)
	{
		mp4ff_atom_t atom;

		mp4ff_atom_write_header(file, &atom, "fiel");
		mp4ff_write_char(file, table->fields);
		mp4ff_write_char(file, table->field_dominance);
		mp4ff_atom_write_footer(file, &atom);
	}
}
