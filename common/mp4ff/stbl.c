#include "mp4ff.h"

int mp4ff_stbl_init(mp4ff_stbl_t *stbl)
{
	stbl->version = 0;
	stbl->flags = 0;
	mp4ff_stsd_init(&(stbl->stsd));
	mp4ff_stts_init(&(stbl->stts));
	mp4ff_stss_init(&(stbl->stss));
	mp4ff_stsc_init(&(stbl->stsc));
	mp4ff_stsz_init(&(stbl->stsz));
	mp4ff_stco_init(&(stbl->stco));
	mp4ff_ctts_init(&(stbl->ctts));
}

int mp4ff_stbl_init_video(mp4ff_t *file, 
							mp4ff_stbl_t *stbl, 
							int frame_w,
							int frame_h, 
							int time_scale, 
							float frame_rate,
							char *compressor)
{
	mp4ff_stsd_init_video(file, &(stbl->stsd), frame_w, frame_h, frame_rate, compressor);
	mp4ff_stts_init_video(file, &(stbl->stts), time_scale, frame_rate);
	mp4ff_stss_init_common(file, &(stbl->stss));
	mp4ff_stsc_init_video(file, &(stbl->stsc));
	mp4ff_stsz_init_video(file, &(stbl->stsz));
	mp4ff_stco_init_common(file, &(stbl->stco));
	mp4ff_ctts_init_common(file, &(stbl->ctts));
}


int mp4ff_stbl_init_audio(mp4ff_t *file, 
							mp4ff_stbl_t *stbl, 
							int channels, 
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	mp4ff_stsd_init_audio(file, &(stbl->stsd), channels, sample_rate, bits, compressor);
	mp4ff_stts_init_audio(file, &(stbl->stts), time_scale, sample_duration);
	mp4ff_stss_init_common(file, &(stbl->stss));
	mp4ff_stsc_init_audio(file, &(stbl->stsc));
	mp4ff_stsz_init_audio(file, &(stbl->stsz), sample_size);
	mp4ff_stco_init_common(file, &(stbl->stco));
	mp4ff_ctts_init_common(file, &(stbl->ctts));
}

int mp4ff_stbl_delete(mp4ff_stbl_t *stbl)
{
	mp4ff_stsd_delete(&(stbl->stsd));
	mp4ff_stts_delete(&(stbl->stts));
	mp4ff_stss_delete(&(stbl->stss));
	mp4ff_stsc_delete(&(stbl->stsc));
	mp4ff_stsz_delete(&(stbl->stsz));
	mp4ff_stco_delete(&(stbl->stco));
	mp4ff_ctts_delete(&(stbl->ctts));
}

int mp4ff_stbl_dump(void *minf_ptr, mp4ff_stbl_t *stbl)
{
	printf("    sample table\n");
	mp4ff_stsd_dump(minf_ptr, &(stbl->stsd));
	mp4ff_stts_dump(&(stbl->stts));
	mp4ff_stss_dump(&(stbl->stss));
	mp4ff_stsc_dump(&(stbl->stsc));
	mp4ff_stsz_dump(&(stbl->stsz));
	mp4ff_stco_dump(&(stbl->stco));
	mp4ff_ctts_dump(&(stbl->ctts));
}

int mp4ff_read_stbl(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stbl_t *stbl, mp4ff_atom_t *parent_atom)
{
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(mp4ff_atom_is(&leaf_atom, "stsd"))
			{ 
				mp4ff_read_stsd(file, minf, &(stbl->stsd)); 
/* Some codecs store extra information at the end of this */
				mp4ff_atom_skip(file, &leaf_atom);
			}
		else
		if(mp4ff_atom_is(&leaf_atom, "stts"))
			{ mp4ff_read_stts(file, &(stbl->stts)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "stss"))
			{ mp4ff_read_stss(file, &(stbl->stss)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "stsc"))
			{ mp4ff_read_stsc(file, &(stbl->stsc)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "stsz"))
			{ mp4ff_read_stsz(file, &(stbl->stsz)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "stco"))
			{ mp4ff_read_stco(file, &(stbl->stco)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "ctts"))
			{ mp4ff_read_ctts(file, &(stbl->ctts)); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < parent_atom->end);

	return 0;
}

int mp4ff_write_stbl(mp4ff_t *file, mp4ff_minf_t *minf, mp4ff_stbl_t *stbl)
{
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "stbl");

	mp4ff_write_stsd(file, minf, &(stbl->stsd));
	mp4ff_write_stts(file, &(stbl->stts));
	mp4ff_write_stss(file, &(stbl->stss));
	mp4ff_write_stsc(file, &(stbl->stsc));
	mp4ff_write_stsz(file, &(stbl->stsz));
	mp4ff_write_stco(file, &(stbl->stco));
	mp4ff_write_ctts(file, &(stbl->ctts));

	mp4ff_atom_write_footer(file, &atom);
}


