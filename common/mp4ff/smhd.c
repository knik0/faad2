#include "mp4ff.h"




int mp4ff_smhd_init(mp4ff_smhd_t *smhd)
{
	smhd->version = 0;
	smhd->flags = 0;
	smhd->balance = 0;
	smhd->reserved = 0;
}

int mp4ff_smhd_delete(mp4ff_smhd_t *smhd)
{
}

int mp4ff_smhd_dump(mp4ff_smhd_t *smhd)
{
	printf("    sound media header\n");
	printf("     version %d\n", smhd->version);
	printf("     flags %d\n", smhd->flags);
	printf("     balance %d\n", smhd->balance);
	printf("     reserved %d\n", smhd->reserved);
}

int mp4ff_read_smhd(mp4ff_t *file, mp4ff_smhd_t *smhd)
{
	smhd->version = mp4ff_read_char(file);
	smhd->flags = mp4ff_read_int24(file);
	smhd->balance = mp4ff_read_int16(file);
	smhd->reserved = mp4ff_read_int16(file);
}

int mp4ff_write_smhd(mp4ff_t *file, mp4ff_smhd_t *smhd)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "smhd");

	mp4ff_write_char(file, smhd->version);
	mp4ff_write_int24(file, smhd->flags);

	mp4ff_write_int32(file, 0x00000000);

	mp4ff_atom_write_footer(file, &atom);
}
