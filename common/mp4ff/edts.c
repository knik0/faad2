#include "mp4ff.h"

int mp4ff_edts_init(mp4ff_edts_t *edts)
{
	mp4ff_elst_init(&(edts->elst));
}

int mp4ff_edts_delete(mp4ff_edts_t *edts)
{
	mp4ff_elst_delete(&(edts->elst));
}

int mp4ff_edts_init_table(mp4ff_edts_t *edts)
{
	mp4ff_elst_init_all(&(edts->elst));
}

int mp4ff_read_edts(mp4ff_t *file, mp4ff_edts_t *edts, mp4ff_atom_t *edts_atom)
{
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);
		if(mp4ff_atom_is(&leaf_atom, "elst"))
			{ mp4ff_read_elst(file, &(edts->elst)); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < edts_atom->end);
}

int mp4ff_edts_dump(mp4ff_edts_t *edts)
{
	printf("  edit atom (edts)\n");
	mp4ff_elst_dump(&(edts->elst));
}

int mp4ff_write_edts(mp4ff_t *file, mp4ff_edts_t *edts, long duration)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "edts");
	mp4ff_write_elst(file, &(edts->elst), duration);
	mp4ff_atom_write_footer(file, &atom);
}
