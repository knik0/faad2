#include "mp4ff.h"


int mp4ff_tkhd_init(mp4ff_tkhd_t *tkhd)
{
	int i;
	tkhd->version = 0;
	tkhd->flags = 15;
	tkhd->creation_time = mp4ff_current_time();
	tkhd->modification_time = mp4ff_current_time();
	tkhd->track_id;
	tkhd->reserved1 = 0;
	tkhd->duration = 0;      /* need to set this when closing */
	for(i = 0; i < 8; i++) tkhd->reserved2[i] = 0;
	tkhd->layer = 0;
	tkhd->alternate_group = 0;
	tkhd->volume = 0.996094;
	tkhd->reserved3 = 0;
	mp4ff_matrix_init(&(tkhd->matrix));
	tkhd->track_width = 0;
	tkhd->track_height = 0;
	tkhd->is_audio = FALSE;
	tkhd->is_video = FALSE;
	return 0;
}

int mp4ff_tkhd_init_audio(mp4ff_t *file, 
								mp4ff_tkhd_t *tkhd)
{
	tkhd->is_audio = TRUE;
}

int mp4ff_tkhd_init_video(mp4ff_t *file, 
								mp4ff_tkhd_t *tkhd, 
								int frame_w, 
								int frame_h)
{
	tkhd->is_video = TRUE;
	tkhd->track_width = frame_w;
	tkhd->track_height = frame_h;
	tkhd->volume = 0;
}

int mp4ff_tkhd_delete(mp4ff_tkhd_t *tkhd)
{
	return 0;
}

int mp4ff_tkhd_dump(mp4ff_tkhd_t *tkhd)
{
	printf("  track header\n");
	printf("   version %d\n", tkhd->version);
	printf("   flags %ld\n", tkhd->flags);
	printf("   creation_time %u\n", tkhd->creation_time);
	printf("   modification_time %u\n", tkhd->modification_time);
	printf("   track_id %d\n", tkhd->track_id);
	printf("   reserved1 %ld\n", tkhd->reserved1);
	printf("   duration %ld\n", tkhd->duration);
	mp4ff_print_chars("   reserved2 ", tkhd->reserved2, 8);
	printf("   layer %d\n", tkhd->layer);
	printf("   alternate_group %d\n", tkhd->alternate_group);
	printf("   volume %f\n", tkhd->volume);
	printf("   reserved3 %d\n", tkhd->reserved3);
	mp4ff_matrix_dump(&(tkhd->matrix));
	printf("   track_width %f\n", tkhd->track_width);
	printf("   track_height %f\n", tkhd->track_height);
}

int mp4ff_read_tkhd(mp4ff_t *file, mp4ff_tkhd_t *tkhd)
{
	tkhd->version = mp4ff_read_char(file);
	tkhd->flags = mp4ff_read_int24(file);
	tkhd->creation_time = mp4ff_read_int32(file);
	tkhd->modification_time = mp4ff_read_int32(file);
	tkhd->track_id = mp4ff_read_int32(file);
	tkhd->reserved1 = mp4ff_read_int32(file);
	tkhd->duration = mp4ff_read_int32(file);
	mp4ff_read_data(file, tkhd->reserved2, 8);
	tkhd->layer = mp4ff_read_int16(file);
	tkhd->alternate_group = mp4ff_read_int16(file);
	tkhd->volume = mp4ff_read_fixed16(file);
	tkhd->reserved3 = mp4ff_read_int16(file);
	mp4ff_read_matrix(file, &(tkhd->matrix));
	tkhd->track_width = mp4ff_read_fixed32(file);
	tkhd->track_height = mp4ff_read_fixed32(file);
}

int mp4ff_write_tkhd(mp4ff_t *file, mp4ff_tkhd_t *tkhd)
{
	int i;

	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "tkhd");

	mp4ff_write_char(file, tkhd->version);
	if (tkhd->flags != 0) {
		mp4ff_write_int24(file, 1);
	} else {
		mp4ff_write_int24(file, tkhd->flags);
	}
	mp4ff_write_int32(file, tkhd->creation_time);
	mp4ff_write_int32(file, tkhd->modification_time);
	mp4ff_write_int32(file, tkhd->track_id);
	mp4ff_write_int32(file, tkhd->reserved1);
	mp4ff_write_int32(file, tkhd->duration);

	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	if (tkhd->is_audio) {
		mp4ff_write_int16(file, 0x0100);
	} else {
		mp4ff_write_int16(file, 0x0000);
	}
	mp4ff_write_int16(file, 0x0000);
	mp4ff_write_int32(file, 0x00010000);
	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	mp4ff_write_int32(file, 0x00010000);
	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	mp4ff_write_int32(file, 0x40000000);
	if (tkhd->is_video) {
		mp4ff_write_int32(file, 0x01400000);
		mp4ff_write_int32(file, 0x00F00000);
	} else {
		mp4ff_write_int32(file, 0x00000000);
		mp4ff_write_int32(file, 0x00000000);
	}

	mp4ff_atom_write_footer(file, &atom);
}


