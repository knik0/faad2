#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

/* atom handling routines */
long mp4ff_atom_read_size(char *data);
unsigned __int64 mp4ff_atom_read_size64(char *data);
int mp4ff_atom_write_header(mp4ff_t *file, mp4ff_atom_t *atom, char *text);
int mp4ff_atom_read_header(mp4ff_t *file, mp4ff_atom_t *atom);
int mp4ff_atom_write_footer(mp4ff_t *file, mp4ff_atom_t *atom);

mp4ff_trak_t* mp4ff_add_track(mp4ff_moov_t *moov);
mp4ff_trak_t* mp4ff_find_track_by_id(mp4ff_moov_t *moov, int trackId);


/* initializers for every atom */
int mp4ff_matrix_init(mp4ff_matrix_t *matrix);
int mp4ff_edts_init_table(mp4ff_edts_t *edts);
int mp4ff_edts_init(mp4ff_edts_t *edts);
int mp4ff_elst_init(mp4ff_elst_t *elst);
int mp4ff_elst_init_all(mp4ff_elst_t *elst);
int mp4ff_elst_table_init(mp4ff_elst_table_t *table); /* initialize a table */
int mp4ff_tkhd_init(mp4ff_tkhd_t *tkhd);
int mp4ff_tkhd_init_video(mp4ff_t *file, mp4ff_tkhd_t *tkhd, int frame_w, int frame_h);
int mp4ff_stsd_table_init(mp4ff_stsd_table_t *table);
int mp4ff_stsd_init(mp4ff_stsd_t *stsd);
int mp4ff_stsd_init_table(mp4ff_stsd_t *stsd);
int mp4ff_stsd_init_video(mp4ff_t *file, mp4ff_stsd_t *stsd, int frame_w, int frame_h, float frame_rate, char *compression);
int mp4ff_stsd_init_audio(mp4ff_t *file, mp4ff_stsd_t *stsd, int channels, int sample_rate, int bits, char *compressor);
int mp4ff_stts_init(mp4ff_stts_t *stts);
int mp4ff_stts_init_table(mp4ff_stts_t *stts);
int mp4ff_stts_init_video(mp4ff_t *file, mp4ff_stts_t *stts, int time_scale, float frame_rate);
int mp4ff_stts_init_audio(mp4ff_t *file, mp4ff_stts_t *stts, int time_scale, int sample_duration);
int mp4ff_stss_init(mp4ff_stss_t *stss);
int mp4ff_stss_init_common(mp4ff_t *file, mp4ff_stss_t *stss);
int mp4ff_stsc_init(mp4ff_stsc_t *stsc);
int mp4ff_stsc_init_video(mp4ff_t *file, mp4ff_stsc_t *stsc);
int mp4ff_stsc_init_audio(mp4ff_t *file, mp4ff_stsc_t *stsc);
int mp4ff_stsz_init(mp4ff_stsz_t *stsz);
int mp4ff_stsz_init_video(mp4ff_t *file, mp4ff_stsz_t *stsz);
int mp4ff_stsz_init_audio(mp4ff_t *file, mp4ff_stsz_t *stsz, int sample_size);
int mp4ff_stco_init(mp4ff_stco_t *stco);
int mp4ff_stco_init_common(mp4ff_t *file, mp4ff_stco_t *stco);
int mp4ff_stbl_init(mp4ff_stbl_t *tkhd);
int mp4ff_stbl_init_video(mp4ff_t *file, mp4ff_stbl_t *stbl, int frame_w, int frame_h, int time_scale, float frame_rate, char *compressor);
int mp4ff_stbl_init_audio(mp4ff_t *file, mp4ff_stbl_t *stbl, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int mp4ff_vmhd_init(mp4ff_vmhd_t *vmhd);
int mp4ff_vmhd_init_video(mp4ff_t *file, mp4ff_vmhd_t *vmhd, int frame_w, int frame_h, float frame_rate);
int mp4ff_smhd_init(mp4ff_smhd_t *smhd);
int mp4ff_dref_table_init(mp4ff_dref_table_t *table);
int mp4ff_dref_init_all(mp4ff_dref_t *dref);
int mp4ff_dref_init(mp4ff_dref_t *dref);
int mp4ff_dinf_init_all(mp4ff_dinf_t *dinf);
int mp4ff_dinf_init(mp4ff_dinf_t *dinf);
int mp4ff_minf_init(mp4ff_minf_t *minf);
int mp4ff_minf_init_video(mp4ff_t *file, mp4ff_minf_t *minf, int frame_w, int frame_h, int time_scale, float frame_rate, char *compressor);
int mp4ff_minf_init_audio(mp4ff_t *file, mp4ff_minf_t *minf, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int mp4ff_mdhd_init(mp4ff_mdhd_t *mdhd);
int mp4ff_mdhd_init_video(mp4ff_t *file, mp4ff_mdhd_t *mdhd, int time_scale);
int mp4ff_mdhd_init_audio(mp4ff_t *file, mp4ff_mdhd_t *mdhd, int time_scale);
int mp4ff_mdia_init(mp4ff_mdia_t *mdia);
int mp4ff_mdia_init_video(mp4ff_t *file, mp4ff_mdia_t *mdia, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);
int mp4ff_mdia_init_audio(mp4ff_t *file, mp4ff_mdia_t *mdia, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int mp4ff_trak_init(mp4ff_trak_t *trak);
int mp4ff_trak_init_video(mp4ff_t *file, mp4ff_trak_t *trak, int frame_w, int frame_h, float frame_rate, int time_scale, char *compressor);
int mp4ff_trak_init_audio(mp4ff_t *file, mp4ff_trak_t *trak, int channels, int sample_rate, int bits, int sample_size, int time_scale, int sample_duration, char *compressor);
int mp4ff_udta_init(mp4ff_udta_t *udta);
int mp4ff_mvhd_init(mp4ff_mvhd_t *mvhd);
int mp4ff_moov_init(mp4ff_moov_t *moov);
int mp4ff_mdat_init(mp4ff_mdat_t *mdat);
int mp4ff_init(mp4ff_t *file);
int mp4ff_hdlr_init(mp4ff_hdlr_t *hdlr);
int mp4ff_hdlr_init_video(mp4ff_hdlr_t *hdlr);
int mp4ff_hdlr_init_audio(mp4ff_hdlr_t *hdlr);
int mp4ff_hdlr_init_data(mp4ff_hdlr_t *hdlr);

/* utilities for reading data types */
int mp4ff_read_data(mp4ff_t *file, char *data, int size);
int mp4ff_write_data(mp4ff_t *file, char *data, int size);
int mp4ff_read_pascal(mp4ff_t *file, char *data);
int mp4ff_write_pascal(mp4ff_t *file, char *data);
float mp4ff_read_fixed32(mp4ff_t *file);
int mp4ff_write_fixed32(mp4ff_t *file, float number);
float mp4ff_read_fixed16(mp4ff_t *file);
int mp4ff_write_fixed16(mp4ff_t *file, float number);
unsigned __int64 mp4ff_read_int64(mp4ff_t *file);
int mp4ff_write_int64(mp4ff_t *file, unsigned __int64 number);
long mp4ff_read_int32(mp4ff_t *file);
int mp4ff_write_int32(mp4ff_t *file, long number);
long mp4ff_read_int24(mp4ff_t *file);
int mp4ff_write_int24(mp4ff_t *file, long number);
int mp4ff_read_int16(mp4ff_t *file);
int mp4ff_write_int16(mp4ff_t *file, int number);
int mp4ff_read_char(mp4ff_t *file);
int mp4ff_write_char(mp4ff_t *file, char x);
int mp4ff_read_char32(mp4ff_t *file, char *string);
int mp4ff_write_char32(mp4ff_t *file, char *string);
int mp4ff_copy_char32(char *output, char *input);
long mp4ff_position(mp4ff_t *file);
int mp4ff_read_mp4_descr_length(mp4ff_t *file);
int mp4ff_write_mp4_descr_length(mp4ff_t *file, int length, unsigned char compact);

/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
long mp4ff_samples_to_bytes(mp4ff_trak_t *track, long samples);


/* chunks always start on 1 */
/* samples start on 0 */

/* queries for every atom */
/* the starting sample in the given chunk */
long mp4ff_sample_of_chunk(mp4ff_trak_t *trak, long chunk);

/* number of samples in the chunk */
long mp4ff_chunk_samples(mp4ff_trak_t *trak, long chunk);

/* the byte offset from mdat start of the chunk */
long mp4ff_chunk_to_offset(mp4ff_trak_t *trak, long chunk);

/* the chunk of any offset from mdat start */
long mp4ff_offset_to_chunk(long *chunk_offset, mp4ff_trak_t *trak, long offset);

/* the total number of samples in the track depending on the access mode */
long mp4ff_track_samples(mp4ff_t *file, mp4ff_trak_t *trak);

/* total bytes between the two samples */
long mp4ff_sample_range_size(mp4ff_trak_t *trak, long chunk_sample, long sample);

/* update the position pointers in all the tracks after a set_position */
int mp4ff_update_positions(mp4ff_t *file);

/* converting between mdat offsets to samples */
long mp4ff_sample_to_offset(mp4ff_trak_t *trak, long sample);
long mp4ff_offset_to_sample(mp4ff_trak_t *trak, long offset);

mp4ff_trak_t* mp4ff_add_trak(mp4ff_moov_t *moov);
int mp4ff_delete_trak(mp4ff_moov_t *moov, mp4ff_trak_t *trak);
int mp4ff_get_timescale(float frame_rate);

/* update all the tables after writing a buffer */
/* set sample_size to 0 if no sample size should be set */
int mp4ff_update_tables(mp4ff_t *file, 
							mp4ff_trak_t *trak, 
							long offset, 
							long chunk, 
							long sample, 
							long samples, 
							long sample_size,
							long sample_duration,
							unsigned char isSyncSample,
							long renderingOffset);
unsigned long mp4ff_current_time();

void mp4ff_print_chars(char *desc, char *input, int len);

#endif
