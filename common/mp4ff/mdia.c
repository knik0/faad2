#include "mp4ff.h"


int mp4ff_mdia_init(mp4ff_mdia_t *mdia)
{
	mp4ff_mdhd_init(&(mdia->mdhd));
	mp4ff_hdlr_init(&(mdia->hdlr));
	mp4ff_minf_init(&(mdia->minf));
}

int mp4ff_mdia_init_video(mp4ff_t *file, 
								mp4ff_mdia_t *mdia,
								int frame_w,
								int frame_h, 
								float frame_rate,
								int time_scale,
								char *compressor)
{
	mp4ff_mdhd_init_video(file, &(mdia->mdhd), time_scale);
	mp4ff_minf_init_video(file, &(mdia->minf), frame_w, frame_h, mdia->mdhd.time_scale, frame_rate, compressor);
	mp4ff_hdlr_init_video(&(mdia->hdlr));
}

int mp4ff_mdia_init_audio(mp4ff_t *file, 
							mp4ff_mdia_t *mdia, 
							int channels,
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	mp4ff_mdhd_init_audio(file, &(mdia->mdhd), time_scale);
	mp4ff_minf_init_audio(file, &(mdia->minf), channels, sample_rate, bits, sample_size, time_scale, sample_duration, compressor);
	mp4ff_hdlr_init_audio(&(mdia->hdlr));
}

int mp4ff_mdia_delete(mp4ff_mdia_t *mdia)
{
	mp4ff_mdhd_delete(&(mdia->mdhd));
	mp4ff_hdlr_delete(&(mdia->hdlr));
	mp4ff_minf_delete(&(mdia->minf));
}

int mp4ff_mdia_dump(mp4ff_mdia_t *mdia)
{
	printf("  media\n");
	mp4ff_mdhd_dump(&(mdia->mdhd));
	mp4ff_hdlr_dump(&(mdia->hdlr));
	mp4ff_minf_dump(&(mdia->minf));
}

int mp4ff_read_mdia(mp4ff_t *file, mp4ff_mdia_t *mdia, mp4ff_atom_t *trak_atom)
{
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(mp4ff_atom_is(&leaf_atom, "mdhd"))
			{ mp4ff_read_mdhd(file, &(mdia->mdhd)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "hdlr"))
			{
				mp4ff_read_hdlr(file, &(mdia->hdlr)); 
/* Main Actor doesn't write component name */
				mp4ff_atom_skip(file, &leaf_atom);
/*printf("mp4ff_read_mdia %ld\n", mp4ff_position(file)); */
			}
		else
		if(mp4ff_atom_is(&leaf_atom, "minf"))
			{ mp4ff_read_minf(file, &(mdia->minf), &leaf_atom); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < trak_atom->end);

	return 0;
}

int mp4ff_write_mdia(mp4ff_t *file, mp4ff_mdia_t *mdia)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "mdia");

	mp4ff_write_mdhd(file, &(mdia->mdhd));
	mp4ff_write_hdlr(file, &(mdia->hdlr));
	mp4ff_write_minf(file, &(mdia->minf));

	mp4ff_atom_write_footer(file, &atom);
}
