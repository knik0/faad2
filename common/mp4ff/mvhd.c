#include "mp4ff.h"



int mp4ff_mvhd_init(mp4ff_mvhd_t *mvhd)
{
	int i;
	mvhd->version = 0;
	mvhd->flags = 0;
	mvhd->creation_time = mp4ff_current_time();
	mvhd->modification_time = mp4ff_current_time();
	mvhd->time_scale = 90000;
	mvhd->duration = 0;
	mvhd->preferred_rate = 1.0;
	mvhd->preferred_volume = 0.996094;
	for(i = 0; i < 10; i++) mvhd->reserved[i] = 0;
	mp4ff_matrix_init(&(mvhd->matrix));
	mvhd->preview_time = 0;
	mvhd->preview_duration = 0;
	mvhd->poster_time = 0;
	mvhd->selection_time = 0;
	mvhd->selection_duration = 0;
	mvhd->current_time = 0;
	mvhd->next_track_id = 1;
	return 0;
}

int mp4ff_mvhd_delete(mp4ff_mvhd_t *mvhd)
{
	return 0;
}

int mp4ff_mvhd_dump(mp4ff_mvhd_t *mvhd)
{
	printf(" movie header\n");
	printf("  version %d\n", mvhd->version);
	printf("  flags %ld\n", mvhd->flags);
	printf("  creation_time %u\n", mvhd->creation_time);
	printf("  modification_time %u\n", mvhd->modification_time);
	printf("  time_scale %ld\n", mvhd->time_scale);
	printf("  duration %ld\n", mvhd->duration);
	printf("  preferred_rate %f\n", mvhd->preferred_rate);
	printf("  preferred_volume %f\n", mvhd->preferred_volume);
	mp4ff_print_chars("  reserved ", mvhd->reserved, 10);
	mp4ff_matrix_dump(&(mvhd->matrix));
	printf("  preview_time %ld\n", mvhd->preview_time);
	printf("  preview_duration %ld\n", mvhd->preview_duration);
	printf("  poster_time %ld\n", mvhd->poster_time);
	printf("  selection_time %ld\n", mvhd->selection_time);
	printf("  selection_duration %ld\n", mvhd->selection_duration);
	printf("  current_time %ld\n", mvhd->current_time);
	printf("  next_track_id %ld\n", mvhd->next_track_id);
}

int mp4ff_read_mvhd(mp4ff_t *file, mp4ff_mvhd_t *mvhd)
{
	mvhd->version = mp4ff_read_char(file);
	mvhd->flags = mp4ff_read_int24(file);
	mvhd->creation_time = mp4ff_read_int32(file);
	mvhd->modification_time = mp4ff_read_int32(file);
	mvhd->time_scale = mp4ff_read_int32(file);
	mvhd->duration = mp4ff_read_int32(file);
	mvhd->preferred_rate = mp4ff_read_fixed32(file);
	mvhd->preferred_volume = mp4ff_read_fixed16(file);
	mp4ff_read_data(file, mvhd->reserved, 10);
	mp4ff_read_matrix(file, &(mvhd->matrix));
	mvhd->preview_time = mp4ff_read_int32(file);
	mvhd->preview_duration = mp4ff_read_int32(file);
	mvhd->poster_time = mp4ff_read_int32(file);
	mvhd->selection_time = mp4ff_read_int32(file);
	mvhd->selection_duration = mp4ff_read_int32(file);
	mvhd->current_time = mp4ff_read_int32(file);
	mvhd->next_track_id = mp4ff_read_int32(file);
}

int mp4ff_write_mvhd(mp4ff_t *file, mp4ff_mvhd_t *mvhd)
{
	int i;

	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "mvhd");

	mp4ff_write_char(file, mvhd->version);
	mp4ff_write_int24(file, mvhd->flags);
	mp4ff_write_int32(file, mvhd->creation_time);
	mp4ff_write_int32(file, mvhd->modification_time);
	mp4ff_write_int32(file, mvhd->time_scale);
	mp4ff_write_int32(file, mvhd->duration);

	mp4ff_write_int32(file, 0x00010000);
	mp4ff_write_int16(file, 0x0100);
	mp4ff_write_int16(file, 0x0000);
	mp4ff_write_int32(file, 0x00000000);
	mp4ff_write_int32(file, 0x00000000);
	mp4ff_write_int32(file, 0x00010000);
	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	mp4ff_write_int32(file, 0x00010000);
	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	mp4ff_write_int32(file, 0x40000000);
	for (i = 0; i < 6; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}

	mp4ff_write_int32(file, mvhd->next_track_id);

	mp4ff_atom_write_footer(file, &atom);
}
