#include "mp4ff.h"

int mp4ff_mdhd_init(mp4ff_mdhd_t *mdhd)
{
	mdhd->version = 0;
	mdhd->flags = 0;
	mdhd->creation_time = mp4ff_current_time();
	mdhd->modification_time = mp4ff_current_time();
	mdhd->time_scale = 0;
	mdhd->duration = 0;
	mdhd->language = 0;
	mdhd->quality = 0;
}

int mp4ff_mdhd_init_video(mp4ff_t *file, 
							mp4ff_mdhd_t *mdhd,
							int time_scale)
{
	mdhd->time_scale = time_scale;
	mdhd->duration = 0;      /* set this when closing */
}

int mp4ff_mdhd_init_audio(mp4ff_t *file, 
							mp4ff_mdhd_t *mdhd, 
							int time_scale)
{
	mdhd->time_scale = time_scale;
	mdhd->duration = 0;      /* set this when closing */
}

mp4ff_mdhd_delete(mp4ff_mdhd_t *mdhd)
{
}

int mp4ff_read_mdhd(mp4ff_t *file, mp4ff_mdhd_t *mdhd)
{
	mdhd->version = mp4ff_read_char(file);
	mdhd->flags = mp4ff_read_int24(file);
	mdhd->creation_time = mp4ff_read_int32(file);
	mdhd->modification_time = mp4ff_read_int32(file);
	mdhd->time_scale = mp4ff_read_int32(file);
	mdhd->duration = mp4ff_read_int32(file);
	mdhd->language = mp4ff_read_int16(file);
	mdhd->quality = mp4ff_read_int16(file);
}

int mp4ff_mdhd_dump(mp4ff_mdhd_t *mdhd)
{
	printf("   media header\n");
	printf("    version %d\n", mdhd->version);
	printf("    flags %d\n", mdhd->flags);
	printf("    creation_time %u\n", mdhd->creation_time);
	printf("    modification_time %u\n", mdhd->modification_time);
	printf("    time_scale %d\n", mdhd->time_scale);
	printf("    duration %d\n", mdhd->duration);
	printf("    language %d\n", mdhd->language);
	printf("    quality %d\n", mdhd->quality);
}

int mp4ff_write_mdhd(mp4ff_t *file, mp4ff_mdhd_t *mdhd)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "mdhd");

	mp4ff_write_char(file, mdhd->version);
	mp4ff_write_int24(file, mdhd->flags);
	mp4ff_write_int32(file, mdhd->creation_time);
	mp4ff_write_int32(file, mdhd->modification_time);
	mp4ff_write_int32(file, mdhd->time_scale);
	mp4ff_write_int32(file, mdhd->duration);
	mp4ff_write_int16(file, mdhd->language);
	mp4ff_write_int16(file, 0x0000);	

	mp4ff_atom_write_footer(file, &atom);
}

