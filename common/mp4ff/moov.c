#include "mp4ff.h"



int mp4ff_moov_init(mp4ff_moov_t *moov)
{
	int i;

	moov->total_tracks = 0;
	for(i = 0 ; i < MAXTRACKS; i++) moov->trak[i] = 0;
	mp4ff_mvhd_init(&(moov->mvhd));
	mp4ff_iods_init(&(moov->iods));
	mp4ff_udta_init(&(moov->udta));
	return 0;
}

int mp4ff_moov_delete(mp4ff_moov_t *moov)
{
	int i;
	while(moov->total_tracks) {
		mp4ff_delete_trak(moov, moov->trak[moov->total_tracks - 1]);
	}
	mp4ff_mvhd_delete(&(moov->mvhd));
	mp4ff_iods_delete(&(moov->iods));
	mp4ff_udta_delete(&(moov->udta));
	return 0;
}

int mp4ff_moov_dump(mp4ff_moov_t *moov)
{
	int i;
	printf("movie\n");
	mp4ff_mvhd_dump(&(moov->mvhd));
	mp4ff_iods_dump(&(moov->iods));
	mp4ff_udta_dump(&(moov->udta));
	for(i = 0; i < moov->total_tracks; i++)
		mp4ff_trak_dump(moov->trak[i]);
}


int mp4ff_read_moov(mp4ff_t *file, mp4ff_moov_t *moov, mp4ff_atom_t *parent_atom)
{
/* mandatory mvhd */
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);
		
		if(mp4ff_atom_is(&leaf_atom, "mvhd"))
		{
			mp4ff_read_mvhd(file, &(moov->mvhd));
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "iods"))
		{
			mp4ff_read_iods(file, &(moov->iods));
			mp4ff_atom_skip(file, &leaf_atom);
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "trak"))
		{
			mp4ff_trak_t *trak = mp4ff_add_trak(moov);
			mp4ff_read_trak(file, trak, &leaf_atom);
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "udta"))
		{
			mp4ff_read_udta(file, &(moov->udta), &leaf_atom);
			mp4ff_atom_skip(file, &leaf_atom);
		}
		else
		{
			mp4ff_atom_skip(file, &leaf_atom);
		}
	}while(mp4ff_position(file) < parent_atom->end);
	
	return 0;
}

int mp4ff_write_moov(mp4ff_t *file, mp4ff_moov_t *moov)
{
	mp4ff_atom_t atom;
	int i;
	long longest_duration = 0;
	long duration, timescale;
	mp4ff_atom_write_header(file, &atom, "moov");

/* get the duration from the longest track in the mvhd's timescale */
	for(i = 0; i < moov->total_tracks; i++)
	{
		mp4ff_trak_fix_counts(file, moov->trak[i]);
		mp4ff_trak_duration(moov->trak[i], &duration, &timescale);

		duration = (long)((float)duration / timescale * moov->mvhd.time_scale);

		if(duration > longest_duration)
		{
			longest_duration = duration;
		}
	}
	moov->mvhd.duration = longest_duration;
	moov->mvhd.selection_duration = longest_duration;

	mp4ff_write_mvhd(file, &(moov->mvhd));
	mp4ff_write_iods(file, &(moov->iods));
	mp4ff_write_udta(file, &(moov->udta));

	for(i = 0; i < moov->total_tracks; i++)
	{
		mp4ff_write_trak(file, moov->trak[i], moov->mvhd.time_scale);
	}

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_durations(mp4ff_moov_t *moov)
{
	
}

int mp4ff_shift_offsets(mp4ff_moov_t *moov, long offset)
{
	int i;
	for(i = 0; i < moov->total_tracks; i++)
	{
		mp4ff_trak_shift_offsets(moov->trak[i], offset);
	}
	return 0;
}
