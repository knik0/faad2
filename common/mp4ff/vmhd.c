#include "mp4ff.h"


int mp4ff_vmhd_init(mp4ff_vmhd_t *vmhd)
{
	vmhd->version = 0;
	vmhd->flags = 1;
	vmhd->graphics_mode = 64;
	vmhd->opcolor[0] = 32768;
	vmhd->opcolor[1] = 32768;
	vmhd->opcolor[2] = 32768;
}

int mp4ff_vmhd_init_video(mp4ff_t *file, 
								mp4ff_vmhd_t *vmhd, 
								int frame_w,
								int frame_h, 
								float frame_rate)
{
}

int mp4ff_vmhd_delete(mp4ff_vmhd_t *vmhd)
{
}

int mp4ff_vmhd_dump(mp4ff_vmhd_t *vmhd)
{
	printf("    video media header\n");
	printf("     version %d\n", vmhd->version);
	printf("     flags %d\n", vmhd->flags);
	printf("     graphics_mode %d\n", vmhd->graphics_mode);
	printf("     opcolor %d %d %d\n", vmhd->opcolor[0], vmhd->opcolor[1], vmhd->opcolor[2]);
}

int mp4ff_read_vmhd(mp4ff_t *file, mp4ff_vmhd_t *vmhd)
{
	int i;
	vmhd->version = mp4ff_read_char(file);
	vmhd->flags = mp4ff_read_int24(file);
	vmhd->graphics_mode = mp4ff_read_int16(file);
	for(i = 0; i < 3; i++)
		vmhd->opcolor[i] = mp4ff_read_int16(file);
}

int mp4ff_write_vmhd(mp4ff_t *file, mp4ff_vmhd_t *vmhd)
{
	mp4ff_atom_t atom;

    mp4ff_atom_write_header(file, &atom, "vmhd");

	mp4ff_write_char(file, vmhd->version);
	mp4ff_write_int24(file, vmhd->flags);

	mp4ff_write_int64(file, (unsigned __int64)0);

	mp4ff_atom_write_footer(file, &atom);
}

