#include "mp4ff.h"



int mp4ff_minf_init(mp4ff_minf_t *minf)
{
	minf->is_video = minf->is_audio = 0;
	mp4ff_vmhd_init(&(minf->vmhd));
	mp4ff_smhd_init(&(minf->smhd));
	mp4ff_hdlr_init(&(minf->hdlr));
	mp4ff_dinf_init(&(minf->dinf));
	mp4ff_stbl_init(&(minf->stbl));
}

int mp4ff_minf_init_video(mp4ff_t *file, 
								mp4ff_minf_t *minf, 
								int frame_w,
								int frame_h, 
								int time_scale, 
								float frame_rate,
								char *compressor)
{
	minf->is_video = 1;
	mp4ff_vmhd_init_video(file, &(minf->vmhd), frame_w, frame_h, frame_rate);
	mp4ff_stbl_init_video(file, &(minf->stbl), frame_w, frame_h, time_scale, frame_rate, compressor);
	mp4ff_hdlr_init_data(&(minf->hdlr));
	mp4ff_dinf_init_all(&(minf->dinf));
}

int mp4ff_minf_init_audio(mp4ff_t *file, 
							mp4ff_minf_t *minf, 
							int channels, 
							int sample_rate, 
							int bits,
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	minf->is_audio = 1;
/* smhd doesn't store anything worth initializing */
	mp4ff_stbl_init_audio(file, &(minf->stbl), channels, sample_rate, bits, sample_size, time_scale, sample_duration, compressor);
	mp4ff_hdlr_init_data(&(minf->hdlr));
	mp4ff_dinf_init_all(&(minf->dinf));
}

int mp4ff_minf_delete(mp4ff_minf_t *minf)
{
	mp4ff_vmhd_delete(&(minf->vmhd));
	mp4ff_smhd_delete(&(minf->smhd));
	mp4ff_dinf_delete(&(minf->dinf));
	mp4ff_stbl_delete(&(minf->stbl));
	mp4ff_hdlr_delete(&(minf->hdlr));
}

int mp4ff_minf_dump(mp4ff_minf_t *minf)
{
	printf("   media info\n");
	printf("    is_audio %d\n", minf->is_audio);
	printf("    is_video %d\n", minf->is_video);
	if(minf->is_audio) mp4ff_smhd_dump(&(minf->smhd));
	if(minf->is_video) mp4ff_vmhd_dump(&(minf->vmhd));
	mp4ff_hdlr_dump(&(minf->hdlr));
	mp4ff_dinf_dump(&(minf->dinf));
	mp4ff_stbl_dump(minf, &(minf->stbl));
}

int mp4ff_read_minf(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_atom_t *parent_atom)
{
	mp4ff_atom_t leaf_atom;
	long pos = mp4ff_position(file);

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(mp4ff_atom_is(&leaf_atom, "vmhd"))
			{ minf->is_video = 1; mp4ff_read_vmhd(file, &(minf->vmhd)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "smhd"))
			{ minf->is_audio = 1; mp4ff_read_smhd(file, &(minf->smhd)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "hdlr"))
			{ 
				mp4ff_read_hdlr(file, &(minf->hdlr)); 
/* Main Actor doesn't write component name */
				mp4ff_atom_skip(file, &leaf_atom);
			}
		else
		if(mp4ff_atom_is(&leaf_atom, "dinf"))
			{ mp4ff_read_dinf(file, &(minf->dinf), &leaf_atom); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < parent_atom->end);

	mp4ff_set_position(file, pos);

	do {
		mp4ff_atom_read_header(file, &leaf_atom);

		if(mp4ff_atom_is(&leaf_atom, "stbl")) {
			mp4ff_read_stbl(file, minf, &(minf->stbl), &leaf_atom);
		} else {
			mp4ff_atom_skip(file, &leaf_atom);
		}
	} while(mp4ff_position(file) < parent_atom->end);

	return 0;
}

int mp4ff_write_minf(mp4ff_t *file, mp4ff_minf_t *minf)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "minf");

	if(minf->is_video) mp4ff_write_vmhd(file, &(minf->vmhd));
	if(minf->is_audio) mp4ff_write_smhd(file, &(minf->smhd));
	mp4ff_write_hdlr(file, &(minf->hdlr));
	mp4ff_write_dinf(file, &(minf->dinf));
	mp4ff_write_stbl(file, minf, &(minf->stbl));

	mp4ff_atom_write_footer(file, &atom);
}
