#include "mp4ff.h"

int mp4ff_dinf_init(mp4ff_dinf_t *dinf)
{
	mp4ff_dref_init(&(dinf->dref));
}

int mp4ff_dinf_delete(mp4ff_dinf_t *dinf)
{
	mp4ff_dref_delete(&(dinf->dref));
}

int mp4ff_dinf_init_all(mp4ff_dinf_t *dinf)
{
	mp4ff_dref_init_all(&(dinf->dref));
}

int mp4ff_dinf_dump(mp4ff_dinf_t *dinf)
{
	printf("    data information (dinf)\n");
	mp4ff_dref_dump(&(dinf->dref));
}

int mp4ff_read_dinf(mp4ff_t *file, mp4ff_dinf_t *dinf, mp4ff_atom_t *dinf_atom)
{
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);
		if(mp4ff_atom_is(&leaf_atom, "dref"))
			{ mp4ff_read_dref(file, &(dinf->dref)); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < dinf_atom->end);
}

int mp4ff_write_dinf(mp4ff_t *file, mp4ff_dinf_t *dinf)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "dinf");
	mp4ff_write_dref(file, &(dinf->dref));
	mp4ff_atom_write_footer(file, &atom);
}
