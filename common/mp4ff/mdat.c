#include "mp4ff.h"

int mp4ff_mdat_init(mp4ff_mdat_t *mdat)
{
	mdat->size = 8;
	mdat->start = 0;
}

int mp4ff_mdat_delete(mp4ff_mdat_t *mdat)
{
}

int mp4ff_read_mdat(mp4ff_t *file, mp4ff_mdat_t *mdat, mp4ff_atom_t *parent_atom)
{
	mdat->size = parent_atom->size;
	mdat->start = parent_atom->start;
	mp4ff_atom_skip(file, parent_atom);
}

int mp4ff_write_mdat(mp4ff_t *file, mp4ff_mdat_t *mdat)
{
	long position, size = 0, new_size = 0;
	int i, j;
	
	for(i = 0; i < file->total_atracks; i++)
	{
		new_size = mp4ff_track_end(file->atracks[i].track);
		if(new_size > size) 
			size = new_size;
	}

	for(i = 0; i < file->total_vtracks; i++)
	{
		new_size = mp4ff_track_end(file->vtracks[i].track);
		if(new_size > size) 
			size = new_size;
	}
	
	mdat->size = size;
	mp4ff_set_position(file, mdat->start);
	mp4ff_write_int32(file, mdat->size);
	mp4ff_set_position(file, mdat->start + mdat->size);
}
