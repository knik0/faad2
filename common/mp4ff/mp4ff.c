#include "mp4ff.h"

#if 0
int mp4ff_make_streamable(mp4_callback_t *in_path, mp4_callback_t *out_path)
{
	mp4ff_t file, *old_file, new_file;
	int moov_exists = 0, mdat_exists = 0, result, atoms = 1;
	long mdat_start, mdat_size;
	mp4ff_atom_t leaf_atom;
	long moov_length;

	mp4ff_init(&file);

/* find the moov atom in the old file */
	
	if(!(file.stream = FOPEN(in_path, _T("rb"))))
	{
		//perror("mp4ff_make_streamable");
		return 1;
	}

	file.total_length = file.stream->get_length();

/* get the locations of moov and mdat atoms */
	do
	{
/*printf("%x\n", mp4ff_position(&file)); */
		result = mp4ff_atom_read_header(&file, &leaf_atom);

		if(!result)
		{
			if(mp4ff_atom_is(&leaf_atom, "moov"))
			{
				moov_exists = atoms;
				moov_length = leaf_atom.size;
			}
			else
			if(mp4ff_atom_is(&leaf_atom, "mdat"))
			{
				mdat_start = mp4ff_position(&file) - HEADER_LENGTH;
				mdat_size = leaf_atom.size;
				mdat_exists = atoms;
			}

			mp4ff_atom_skip(&file, &leaf_atom);

			atoms++;
		}
	}while(!result && mp4ff_position(&file) < file.total_length);

	if(!moov_exists)
	{
		printf("mp4ff_make_streamable: no moov atom\n");
		return 1;
	}

	if(!mdat_exists)
	{
		printf("mp4ff_make_streamable: no mdat atom\n");
		return 1;
	}

/* copy the old file to the new file */
	if(moov_exists && mdat_exists)
	{
/* moov wasn't the first atom */
		if(moov_exists > 1)
		{
			char *buffer;
			long buf_size = 1000000;

			result = 0;

/* read the header proper */
			if(!(old_file = mp4ff_open(in_path, 1, 0, 0)))
			{
				return 1;
			}

			mp4ff_shift_offsets(&(old_file->moov), moov_length);

/* open the output file */
			if(!(new_file.stream = FOPEN(out_path, _T("wb"))))
			{
				//perror("mp4ff_make_streamable");
				result =  1;
			}
			else
			{
/* set up some flags */
				new_file.wr = 1;
				new_file.rd = 0;
				mp4ff_write_moov(&new_file, &(old_file->moov));

				mp4ff_set_position(old_file, mdat_start);

				if(!(buffer = calloc(1, buf_size)))
				{
					result = 1;
					printf("mp4ff_make_streamable: out of memory\n");
				}
				else
				{
					while(mp4ff_position(old_file) < mdat_start + mdat_size && !result)
					{
						if(mp4ff_position(old_file) + buf_size > mdat_start + mdat_size)
							buf_size = mdat_start + mdat_size - mp4ff_position(old_file);

						if(!mp4ff_read_data(old_file, buffer, buf_size)) result = 1;
						if(!result)
						{
							if(!mp4ff_write_data(&new_file, buffer, buf_size)) result = 1;
						}
					}
					free(buffer);
				}
				fclose(new_file.stream);
			}
			mp4ff_close(old_file);
		}
		else
		{
			printf("mp4ff_make_streamable: header already at 0 offset\n");
			return 0;
		}
	}
	
	return 0;
}
#endif

int mp4ff_set_time_scale(mp4ff_t *file, int time_scale)
{
	file->moov.mvhd.time_scale = time_scale;
}

int mp4ff_set_copyright(mp4ff_t *file, char *string)
{
	mp4ff_set_udta_string(&(file->moov.udta.copyright), &(file->moov.udta.copyright_len), string);
}

int mp4ff_set_name(mp4ff_t *file, char *string)
{
	mp4ff_set_udta_string(&(file->moov.udta.name), &(file->moov.udta.name_len), string);
}

int mp4ff_set_info(mp4ff_t *file, char *string)
{
	mp4ff_set_udta_string(&(file->moov.udta.info), &(file->moov.udta.info_len), string);
}

int mp4ff_get_time_scale(mp4ff_t *file)
{
	return file->moov.mvhd.time_scale;
}

char* mp4ff_get_copyright(mp4ff_t *file)
{
	return file->moov.udta.copyright;
}

char* mp4ff_get_name(mp4ff_t *file)
{
	return file->moov.udta.name;
}

char* mp4ff_get_info(mp4ff_t *file)
{
	return file->moov.udta.info;
}

int mp4ff_get_iod_audio_profile_level(mp4ff_t *file)
{
	return file->moov.iods.audioProfileId;
}

int mp4ff_set_iod_audio_profile_level(mp4ff_t *file, int id)
{
	mp4ff_iods_set_audio_profile(&file->moov.iods, id);
}

int mp4ff_get_iod_video_profile_level(mp4ff_t *file)
{
	return file->moov.iods.videoProfileId;
}

int mp4ff_set_iod_video_profile_level(mp4ff_t *file, int id)
{
	mp4ff_iods_set_video_profile(&file->moov.iods, id);
}

int mp4ff_video_tracks(mp4ff_t *file)
{
	int i, result = 0;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		if(file->moov.trak[i]->mdia.minf.is_video) result++;
	}
	return result;
}

int mp4ff_audio_tracks(mp4ff_t *file)
{
	int i, result = 0;
	mp4ff_minf_t *minf;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		minf = &(file->moov.trak[i]->mdia.minf);
		if(minf->is_audio)
			result++;
	}
	return result;
}

int mp4ff_set_audio(mp4ff_t *file, 
						int channels,
						long sample_rate,
						int bits,
						int sample_size,
						int time_scale,
						int sample_duration,	
						char *compressor)
{
	int i, j;
	mp4ff_trak_t *trak;

	/* delete any existing tracks */
	for(i = 0; i < file->total_atracks; i++) {
		mp4ff_delete_audio_map(&(file->atracks[i]));
		mp4ff_delete_trak(&(file->moov), file->atracks[i].track);
	}
	free(file->atracks);
	file->atracks = NULL;	
	file->total_atracks = 0;

	if(channels) {
#if 0
		/* Fake the bits parameter for some formats. */
		if(mp4ff_match_32(compressor, mp4ff_ULAW) ||
			mp4ff_match_32(compressor, mp4ff_IMA4)) bits = 16;
#endif

		file->atracks = (mp4ff_audio_map_t*)
			calloc(1, sizeof(mp4ff_audio_map_t));
		trak = mp4ff_add_track(&(file->moov));
		mp4ff_trak_init_audio(file, trak, channels, sample_rate, bits, 
			sample_size, time_scale, sample_duration, compressor);
		mp4ff_init_audio_map(&(file->atracks[0]), trak);
		file->atracks[file->total_atracks].track = trak;
		file->atracks[file->total_atracks].channels = channels;
		file->atracks[file->total_atracks].current_position = 0;
		file->atracks[file->total_atracks].current_chunk = 1;
		file->total_atracks++;
	}
	return 1;   /* Return the number of tracks created */
}

int mp4ff_set_video(mp4ff_t *file, 
						int tracks, 
						int frame_w, 
						int frame_h,
						float frame_rate,
						int time_scale,
						char *compressor)
{
	int i, j;
	mp4ff_trak_t *trak;

	/* delete any existing tracks */
	for(i = 0; i < file->total_vtracks; i++) {
		mp4ff_delete_video_map(&(file->vtracks[i]));
		mp4ff_delete_trak(&(file->moov), file->vtracks[i].track);
	}
	free(file->vtracks);
	file->vtracks = NULL;	
	file->total_vtracks = 0;

	if (tracks > 0) {
		file->total_vtracks = tracks;
		file->vtracks = (mp4ff_video_map_t*)calloc(1, sizeof(mp4ff_video_map_t) * file->total_vtracks);
		for(i = 0; i < tracks; i++)
		{
			trak = mp4ff_add_track(&(file->moov));
			mp4ff_trak_init_video(file, trak, frame_w, frame_h, frame_rate,
				time_scale, compressor);
			mp4ff_init_video_map(&(file->vtracks[i]), trak);
		}
	}
	return 0;
}

int mp4ff_set_framerate(mp4ff_t *file, float framerate)
{
	int i;
	int new_time_scale, new_sample_duration;
	new_time_scale = mp4ff_get_timescale(framerate);
	new_sample_duration = (int)((float)new_time_scale / framerate + 0.5);

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.mdhd.time_scale = new_time_scale;
		file->vtracks[i].track->mdia.minf.stbl.stts.table[0].sample_duration = new_sample_duration;
	}
}

mp4ff_trak_t* mp4ff_add_track(mp4ff_moov_t *moov)
{
	mp4ff_trak_t *trak;
	trak = moov->trak[moov->total_tracks] = calloc(1, sizeof(mp4ff_trak_t));
	mp4ff_trak_init(trak);
	trak->tkhd.track_id = moov->mvhd.next_track_id;
	moov->mvhd.next_track_id++;
	moov->total_tracks++;
	return trak;
}

/* ============================= Initialization functions */

int mp4ff_init(mp4ff_t *file)
{
	memset(file, 0, sizeof(mp4ff_t));
	mp4ff_mdat_init(&(file->mdat));
	mp4ff_moov_init(&(file->moov));
	return 0;
}

int mp4ff_delete(mp4ff_t *file)
{
	int i;
	if(file->total_atracks) 
	{
		for(i = 0; i < file->total_atracks; i++)
			mp4ff_delete_audio_map(&(file->atracks[i]));
		free(file->atracks);
	}
	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks; i++)
			mp4ff_delete_video_map(&(file->vtracks[i]));
		free(file->vtracks);
	}
	file->total_atracks = 0;
	file->total_vtracks = 0;
	mp4ff_moov_delete(&(file->moov));
	mp4ff_mdat_delete(&(file->mdat));
	return 0;
}

/* =============================== Optimization functions */

int mp4ff_get_timescale(float frame_rate)
{
	int timescale = 600;
/* Encode the 29.97, 23.976, 59.94 framerates as per DV freaks */
	if(frame_rate - (int)frame_rate != 0) timescale = (int)(frame_rate * 1001 + 0.5);
	else
	if((600 / frame_rate) - (int)(600 / frame_rate) != 0) timescale = (int)(frame_rate * 100 + 0.5);
	return timescale;
}

int mp4ff_seek_end(mp4ff_t *file)
{
	mp4ff_set_position(file, file->mdat.size + file->mdat.start);
/*printf("mp4ff_seek_end %ld\n", file->mdat.size + file->mdat.start); */
	mp4ff_update_positions(file);
	return 0;
}

int mp4ff_seek_start(mp4ff_t *file)
{
	mp4ff_set_position(file, file->mdat.start + HEADER_LENGTH);
	mp4ff_update_positions(file);
	return 0;
}

long mp4ff_audio_length(mp4ff_t *file, int track)
{
	if(file->total_atracks > 0) 
		return mp4ff_track_samples(file, file->atracks[track].track);

	return 0;
}

long mp4ff_video_length(mp4ff_t *file, int track)
{
/*printf("mp4ff_video_length %d %d\n", mp4ff_track_samples(file, file->vtracks[track].track), track); */
	if(file->total_vtracks > 0)
		return mp4ff_track_samples(file, file->vtracks[track].track);
	return 0;
}

long mp4ff_audio_position(mp4ff_t *file, int track)
{
	return file->atracks[track].current_position;
}

long mp4ff_video_position(mp4ff_t *file, int track)
{
	return file->vtracks[track].current_position;
}

int mp4ff_update_positions(mp4ff_t *file)
{
/* Used for routines that change the positions of all tracks, like */
/* seek_end and seek_start but not for routines that reposition one track, like */
/* set_audio_position. */

	long mdat_offset = mp4ff_position(file) - file->mdat.start;
	long sample, chunk, chunk_offset;
	int i;

	if(file->total_atracks)
	{
		sample = mp4ff_offset_to_sample(file->atracks[0].track, mdat_offset);
		chunk = mp4ff_offset_to_chunk(&chunk_offset, file->atracks[0].track, mdat_offset);
		for(i = 0; i < file->total_atracks; i++)
		{
			file->atracks[i].current_position = sample;
			file->atracks[i].current_chunk = chunk;
		}
	}

	if(file->total_vtracks)
	{
		sample = mp4ff_offset_to_sample(file->vtracks[0].track, mdat_offset);
		chunk = mp4ff_offset_to_chunk(&chunk_offset, file->vtracks[0].track, mdat_offset);
		for(i = 0; i < file->total_vtracks; i++)
		{
			file->vtracks[i].current_position = sample;
			file->vtracks[i].current_chunk = chunk;
		}
	}
	return 0;
}

int mp4ff_set_audio_position(mp4ff_t *file, long sample, int track)
{
	long offset, chunk_sample, chunk;
	mp4ff_trak_t *trak;

	if(file->total_atracks)
	{
		trak = file->atracks[track].track;
		file->atracks[track].current_position = sample;
		mp4ff_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
		file->atracks[track].current_chunk = chunk;
		offset = mp4ff_sample_to_offset(trak, sample);
		mp4ff_set_position(file, offset);
		/*mp4ff_update_positions(file); */
	}

	return 0;
}

int mp4ff_set_video_position(mp4ff_t *file, long frame, int track)
{
	long offset, chunk_sample, chunk;
	mp4ff_trak_t *trak;

	if(file->total_vtracks)
	{
		trak = file->vtracks[track].track;
		file->vtracks[track].current_position = frame;
		mp4ff_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
		file->vtracks[track].current_chunk = chunk;
		offset = mp4ff_sample_to_offset(trak, frame);
		mp4ff_set_position(file, offset);
		/*mp4ff_update_positions(file); */
	}
	return 0;
}

int mp4ff_has_audio(mp4ff_t *file)
{
	if(mp4ff_audio_tracks(file)) return 1;
	return 0;
}

long mp4ff_audio_sample_rate(mp4ff_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_rate;
	return 0;
}

int mp4ff_audio_bits(mp4ff_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_size;

	return 0;
}

int mp4ff_audio_time_scale(mp4ff_t *file, int track)
{
	if(file->total_atracks) {
		return file->atracks[track].track->mdia.mdhd.time_scale;
	}
	return 0;
}

int mp4ff_audio_sample_duration(mp4ff_t *file, int track)
{
	if(file->total_atracks) {
		return file->atracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;
	}
	return 0;
}

char* mp4ff_audio_compressor(mp4ff_t *file, int track)
{
  if (file->atracks[track].track == NULL)
    return (NULL);
	return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int mp4ff_track_channels(mp4ff_t *file, int track)
{
	if(track < file->total_atracks)
		return file->atracks[track].channels;

	return 0;
}

int mp4ff_channel_location(mp4ff_t *file, int *mp4ff_track, int *mp4ff_channel, int channel)
{
	int current_channel = 0, current_track = 0;
	*mp4ff_channel = 0;
	*mp4ff_track = 0;
	for(current_channel = 0, current_track = 0; current_track < file->total_atracks; )
	{
		if(channel >= current_channel)
		{
			*mp4ff_channel = channel - current_channel;
			*mp4ff_track = current_track;
		}

		current_channel += file->atracks[current_track].channels;
		current_track++;
	}
	return 0;
}

int mp4ff_has_video(mp4ff_t *file)
{
	if(mp4ff_video_tracks(file)) return 1;
	return 0;
}

int mp4ff_video_width(mp4ff_t *file, int track)
{
	if(file->total_vtracks) {
		int width = 
			file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].width;
		if (width) {
			return width;
		}
		return file->vtracks[track].track->tkhd.track_width;
	}
	return 0;
}

int mp4ff_video_height(mp4ff_t *file, int track)
{
	if(file->total_vtracks) {
		int height = file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].height;
		if (height) {
			return height;
		}
		return file->vtracks[track].track->tkhd.track_height;
	}
	return 0;
}

int mp4ff_video_depth(mp4ff_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
	return 0;
}

int mp4ff_set_depth(mp4ff_t *file, int depth, int track)
{
	int i;

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].depth = depth;
	}
}

float mp4ff_video_frame_rate(mp4ff_t *file, int track)
{
  float ret = 0;
  int num = 0;
  
  if(file->total_vtracks) {
    ret = file->vtracks[track].track->mdia.mdhd.time_scale;
    if (file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration == 0)
      num = 1;
    ret /= 
	file->vtracks[track].track->mdia.minf.stbl.stts.table[num].sample_duration;
  }
  return ret;
}

char* mp4ff_video_compressor(mp4ff_t *file, int track)
{
  if (file->vtracks[track].track == NULL)
    return (NULL);
  
	return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int mp4ff_video_time_scale(mp4ff_t *file, int track)
{
	if(file->total_vtracks) {
		return file->vtracks[track].track->mdia.mdhd.time_scale;
	}
	return 0;
}

int mp4ff_video_frame_time(mp4ff_t *file, int track, long frame,
	long *start, int *duration)
{
	mp4ff_stts_t *stts;
	int i;
	long f;

	if (file->total_vtracks == 0) {
		return 0;
	}
	stts = &(file->vtracks[track].track->mdia.minf.stbl.stts);

	if (frame < file->last_frame) {
		/* we need to reset our cached values */
		file->last_frame = 0;
		file->last_start = 0;
		file->last_stts_index = 0;
	}

	i = file->last_stts_index;
	f = file->last_frame;
	*start = file->last_start;

	while (i < stts->total_entries) {
		if (f + stts->table[i].sample_count <= frame) {
			*start += stts->table[i].sample_duration 
				* stts->table[i].sample_count;
			f += stts->table[i].sample_count;
			i++;

		} else {
			/* cache the results for future use */
			file->last_stts_index = i;
			file->last_frame = f;
			file->last_start = *start;

			*start += stts->table[i].sample_duration * (frame - f);
			*duration = stts->table[i].sample_duration;

			return 1;
		}
	}

	/* error */
	return 0;
}

int mp4ff_get_mp4_video_decoder_config(mp4ff_t *file, int track, unsigned char** ppBuf, int* pBufSize)
{
	mp4ff_esds_t* esds;

	if (!file->total_vtracks) {
		return 0;
	}
	esds = &file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return mp4ff_esds_get_decoder_config(esds, ppBuf, pBufSize);
}

int mp4ff_set_mp4_video_decoder_config(mp4ff_t *file, int track, unsigned char* pBuf, int bufSize)
{
	mp4ff_esds_t* esds;

	if (!file->total_vtracks) {
		return 0;
	}
	esds = &file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return mp4ff_esds_set_decoder_config(esds, pBuf, bufSize);
}

int mp4ff_get_mp4_audio_decoder_config(mp4ff_t *file, int track, unsigned char** ppBuf, int* pBufSize)
{
	mp4ff_esds_t* esds;

	if (!file->total_atracks) {
		return 0;
	}
	esds = &file->atracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return mp4ff_esds_get_decoder_config(esds, ppBuf, pBufSize);
}

int mp4ff_set_mp4_audio_decoder_config(mp4ff_t *file, int track, unsigned char* pBuf, int bufSize)
{
	mp4ff_esds_t* esds;

	if (!file->total_atracks) {
		return 0;
	}
	esds = &file->atracks[track].track->mdia.minf.stbl.stsd.table[0].esds;
	return mp4ff_esds_set_decoder_config(esds, pBuf, bufSize);
}

long mp4ff_samples_to_bytes(mp4ff_trak_t *track, long samples)
{
	return samples
		* track->mdia.minf.stbl.stsd.table[0].channels
		* track->mdia.minf.stbl.stsd.table[0].sample_size / 8;
}

int mp4ff_write_audio(mp4ff_t *file, char *audio_buffer, long samples, int track)
{
	long offset;
	int result;
	long bytes;

/* Defeat 32 bit file size limit. */
	if(mp4ff_test_position(file)) return 1;

/* write chunk for 1 track */
	bytes = samples * mp4ff_audio_bits(file, track) / 8 * file->atracks[track].channels;
	offset = mp4ff_position(file);
	result = mp4ff_write_data(file, audio_buffer, bytes);

	if(result) result = 0; else result = 1; /* defeat fwrite's return */
	mp4ff_update_tables(file, 
						file->atracks[track].track, 
						offset, 
						file->atracks[track].current_chunk, 
						file->atracks[track].current_position, 
						samples, 
						0,
						0,
						0,
						0);
	file->atracks[track].current_position += samples;
	file->atracks[track].current_chunk++;
	return result;
}

int mp4ff_write_audio_frame(mp4ff_t *file, unsigned char *audio_buffer, long bytes, int track)
{
	long offset = mp4ff_position(file);
	int result = 0;

	/* Defeat 32 bit file size limit. */
	if(mp4ff_test_position(file)) return 1;

	result = mp4ff_write_data(file, audio_buffer, bytes);
	if(result) result = 0; else result = 1;

	mp4ff_update_tables(file,
						file->atracks[track].track,
						offset,
						file->atracks[track].current_chunk,
						file->atracks[track].current_position,
						1, 
						bytes,
						0,
						0,
						0);
	file->atracks[track].current_position += 1;
	file->atracks[track].current_chunk++;
	return result;
}

int mp4ff_write_video_frame(mp4ff_t *file, 
								unsigned char *video_buffer, 
								long bytes, 
								int track, 
								unsigned char isKeyFrame,
								long duration,
								long renderingOffset)
{
	long offset = mp4ff_position(file);
	int result = 0;

	/* Defeat 32 bit file size limit. */
	if(mp4ff_test_position(file)) return 1;

	result = mp4ff_write_data(file, video_buffer, bytes);
	if(result) result = 0; else result = 1;

	mp4ff_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes,
						duration,
						isKeyFrame,
						renderingOffset);
	file->vtracks[track].current_position += 1;
	file->vtracks[track].current_chunk++;
	return result;
}

mp4_callback_t* mp4ff_get_fd(mp4ff_t *file)
{
    if (file->stream->get_position() != file->file_position)
        file->stream->seek(file->file_position);
    return file->stream;
}

int mp4ff_write_frame_init(mp4ff_t *file, int track)
{
    if(file->stream->get_position() != file->file_position)
        file->stream->seek(file->file_position);
    file->offset = mp4ff_position(file);
    return 0;
}

int mp4ff_write_frame_end(mp4ff_t *file, int track)
{
	long bytes;
	file->file_position = file->stream->get_position();
	bytes = mp4ff_position(file) - file->offset;
	mp4ff_update_tables(file,
						file->vtracks[track].track,
						file->offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes,
						0,
						0,
						0);
	file->vtracks[track].current_position += 1;
	file->vtracks[track].current_chunk++;
	return 0;
}

int mp4ff_write_audio_init(mp4ff_t *file, int track)
{
	return mp4ff_write_frame_init(file, track);
}

int mp4ff_write_audio_end(mp4ff_t *file, int track, long samples)
{
	long bytes;
	file->file_position = file->stream->get_position();
	bytes = mp4ff_position(file) - file->offset;
	mp4ff_update_tables(file, 
						file->atracks[track].track,
						file->offset,
						file->atracks[track].current_chunk,
						file->atracks[track].current_position,
						samples,
						bytes, 
						0,
						0,
						0);
	file->atracks[track].current_position += samples;
	file->atracks[track].current_chunk++;
	return 0;
}


long mp4ff_read_audio(mp4ff_t *file, char *audio_buffer, long samples, int track)
{
	long chunk_sample, chunk;
	int result = 1, track_num;
	mp4ff_trak_t *trak = file->atracks[track].track;
	long fragment_len, chunk_end;
	long position = file->atracks[track].current_position;
	long start = position, end = position + samples;
	long bytes, total_bytes = 0;
	long buffer_offset;

	mp4ff_chunk_of_sample(&chunk_sample, &chunk, trak, position);
	buffer_offset = 0;

	while(position < end && result)
	{
		mp4ff_set_audio_position(file, position, track);
		fragment_len = mp4ff_chunk_samples(trak, chunk);
		chunk_end = chunk_sample + fragment_len;
		fragment_len -= position - chunk_sample;
		if(position + fragment_len > chunk_end) fragment_len = chunk_end - position;
		if(position + fragment_len > end) fragment_len = end - position;

		bytes = mp4ff_samples_to_bytes(trak, fragment_len);
		result = mp4ff_read_data(file, &audio_buffer[buffer_offset], bytes);

		total_bytes += bytes;
		position += fragment_len;
		chunk_sample = position;
		buffer_offset += bytes;
		chunk++;
	}

	file->atracks[track].current_position = position;
	if(!result) return 0;
	return total_bytes;
}

int mp4ff_read_chunk(mp4ff_t *file, char *output, int track, long chunk, long byte_start, long byte_len)
{
	mp4ff_set_position(file, mp4ff_chunk_to_offset(file->atracks[track].track, chunk) + byte_start);
	if(mp4ff_read_data(file, output, byte_len)) return 0;
	else
	return 1;
}

long mp4ff_frame_size(mp4ff_t *file, long frame, int track)
{
	int bytes;
	mp4ff_trak_t *trak = file->vtracks[track].track;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		bytes = trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	{
		bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
	}

	return bytes;
}

long mp4ff_get_sample_duration(mp4ff_t *file, long frame, int track)
{
    int i, ci = 0, co = 0;
    mp4ff_trak_t *trak = file->atracks[track].track;

    for (i = 0; i < trak->mdia.minf.stbl.stts.total_entries; i++)
    {
        int j;
        for (j = 0; j < trak->mdia.minf.stbl.stts.table[i].sample_count; j++)
        {
            if (co == frame)
                return trak->mdia.minf.stbl.stts.table[ci].sample_duration;
            co++;
        }
        ci++;
    }
}

long mp4ff_audio_frame_size(mp4ff_t *file, long frame, int track)
{
    int bytes;
    mp4ff_trak_t *trak = file->atracks[track].track;

    if(trak->mdia.minf.stbl.stsz.sample_size)
    {
        bytes = trak->mdia.minf.stbl.stsz.sample_size;
    } else {
        bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
    }

    return bytes;
}

long mp4ff_read_audio_frame(mp4ff_t *file, unsigned char *audio_buffer,  int maxBytes, int track)
{
	long bytes;
	int result = 0;
	mp4ff_trak_t *trak = file->atracks[track].track;

	bytes = mp4ff_audio_frame_size(file, 
		file->atracks[track].current_position, track);

	if (bytes > maxBytes) {
		return -bytes;
	}

	mp4ff_set_audio_position(file, 
		file->atracks[track].current_position, track);

	result = mp4ff_read_data(file, audio_buffer, bytes);

	file->atracks[track].current_position++;

	if (!result)
		return 0;
	return bytes;
}

long mp4ff_read_frame(mp4ff_t *file, unsigned char *video_buffer, int track)
{
	long bytes;
	int result = 0;

	mp4ff_trak_t *trak = file->vtracks[track].track;
	bytes = mp4ff_frame_size(file, file->vtracks[track].current_position, track);

	if(!file->vtracks[track].frames_cached)
	{
		mp4ff_set_video_position(file, file->vtracks[track].current_position, track);
		result = mp4ff_read_data(file, video_buffer, bytes);
	}
	else
	{
		int i;
		unsigned char *cached_frame;

		if(file->vtracks[track].current_position >= file->vtracks[track].frames_cached) result = 1;
		if(!result)
		{
			cached_frame = file->vtracks[track].frame_cache[file->vtracks[track].current_position];

			for(i = 0; i < bytes; i++)
				video_buffer[i] = cached_frame[i];
		}
	}
	file->vtracks[track].current_position++;

	if(!result) return 0;
	return bytes;
}

int mp4ff_read_frame_init(mp4ff_t *file, int track)
{
	mp4ff_trak_t *trak = file->vtracks[track].track;
	mp4ff_set_video_position(file, file->vtracks[track].current_position, track);
	if(file->stream->get_position() != file->file_position)
        file->stream->seek(file->file_position);
	return 0;
}

int mp4ff_read_frame_end(mp4ff_t *file, int track)
{
	file->file_position = file->stream->get_position();
	file->vtracks[track].current_position++;
	return 0;
}

int mp4ff_init_video_map(mp4ff_video_map_t *vtrack, mp4ff_trak_t *trak)
{
	vtrack->track = trak;
	vtrack->current_position = 0;
	vtrack->current_chunk = 1;
	vtrack->frame_cache = 0;
	vtrack->frames_cached = 0;
	return 0;
}

int mp4ff_delete_video_map(mp4ff_video_map_t *vtrack)
{
	int i;
	if(vtrack->frames_cached)
	{
		for(i = 0; i < vtrack->frames_cached; i++)
		{
			free(vtrack->frame_cache[i]);
		}
		free(vtrack->frame_cache);
		vtrack->frames_cached = 0;
	}
	return 0;
}

int mp4ff_init_audio_map(mp4ff_audio_map_t *atrack, mp4ff_trak_t *trak)
{
	atrack->track = trak;
	atrack->channels = trak->mdia.minf.stbl.stsd.table[0].channels;
	atrack->current_position = 0;
	atrack->current_chunk = 1;
	return 0;
}

int mp4ff_delete_audio_map(mp4ff_audio_map_t *atrack)
{
	int i;
	return 0;
}

int mp4ff_read_info(mp4ff_t *file)
{
	int result = 0, found_moov = 0;
	int i, j, k, m, channel, trak_channel, track;
	long start_position = mp4ff_position(file);
	mp4ff_atom_t leaf_atom;
	mp4ff_trak_t *trak;

	mp4ff_set_position(file, 0);

	do
	{
		result = mp4ff_atom_read_header(file, &leaf_atom);
		if(!result)
		{
			if(mp4ff_atom_is(&leaf_atom, "mdat")) {
				mp4ff_read_mdat(file, &(file->mdat), &leaf_atom);
			} else if(mp4ff_atom_is(&leaf_atom, "moov")) {
				mp4ff_read_moov(file, &(file->moov), &leaf_atom);
				found_moov = 1;
			} else {
				mp4ff_atom_skip(file, &leaf_atom);
			}
		}
	}while(!result && mp4ff_position(file) < file->total_length);

/* go back to the original position */
	mp4ff_set_position(file, start_position);

	if(found_moov) {

		/* get tables for all the different tracks */
		file->total_atracks = mp4ff_audio_tracks(file);
		file->atracks = (mp4ff_audio_map_t*)calloc(1, 
			sizeof(mp4ff_audio_map_t) * file->total_atracks);

		for(i = 0, track = 0; i < file->total_atracks; i++) {
			while(!file->moov.trak[track]->mdia.minf.is_audio)
				track++;
			mp4ff_init_audio_map(&(file->atracks[i]), file->moov.trak[track]);
		}

		file->total_vtracks = mp4ff_video_tracks(file);
		file->vtracks = (mp4ff_video_map_t*)calloc(1, sizeof(mp4ff_video_map_t) * file->total_vtracks);

		for(track = 0, i = 0; i < file->total_vtracks; i++)
		{
			while(!file->moov.trak[track]->mdia.minf.is_video)
				track++;

			mp4ff_init_video_map(&(file->vtracks[i]), file->moov.trak[track]);
		}
	}

	if(found_moov) 
		return 0; 
	else 
		return 1;
}


int mp4ff_dump(mp4ff_t *file)
{
	printf("mp4ff_dump\n");
	printf("movie data\n");
	printf(" size %ld\n", file->mdat.size);
	printf(" start %ld\n", file->mdat.start);
	mp4ff_moov_dump(&(file->moov));
	return 0;
}


/* ================================== Entry points ========================== */

int mp4ff_check_sig(mp4_callback_t *path)
{
	mp4ff_t file;
	mp4ff_atom_t leaf_atom;
	int result1 = 0, result2 = 0;

	mp4ff_init(&file);
    file.stream = path;

	file.total_length = file.stream->get_length();

	do
	{
		result1 = mp4ff_atom_read_header(&file, &leaf_atom);

		if(!result1)
		{
/* just want the "moov" atom */
			if(mp4ff_atom_is(&leaf_atom, "moov"))
			{
				result2 = 1;
			}
			else
				mp4ff_atom_skip(&file, &leaf_atom);
		}
	}while(!result1 && !result2 && mp4ff_position(&file) < file.total_length);

	mp4ff_delete(&file);
	return result2;
}

mp4ff_t* mp4ff_open(mp4_callback_t *callbacks, int rd, int wr, int append)
{
    mp4ff_t *new_file = malloc(sizeof(mp4ff_t));
    int exists = 0;

    mp4ff_init(new_file);
    new_file->stream = callbacks;
    new_file->wr = wr;
    new_file->rd = rd;
    new_file->mdat.start = 0;

    if(rd /*&& exists*/)
    {
        new_file->total_length = new_file->stream->get_length();

        if(mp4ff_read_info(new_file))
        {
            mp4ff_close(new_file);
            new_file = 0;
        }
    }

    if(wr)
    {
        if(!exists || !append)
        {
            /* start the data atom */
            mp4ff_write_int32(new_file, 0);
            mp4ff_write_char32(new_file, "mdat");
        } else {
            mp4ff_set_position(new_file,
                new_file->mdat.start + new_file->mdat.size);
            new_file->stream->seek(new_file->mdat.start + new_file->mdat.size);
        }
    }
    return new_file;
}

int mp4ff_write(mp4ff_t *file)
{
	int result = -1;

	if(!file->wr) {
		return result;
	}

	mp4ff_write_mdat(file, &(file->mdat));
	mp4ff_write_moov(file, &(file->moov));

	return result;
}

int mp4ff_destroy(mp4ff_t *file)
{
	mp4ff_delete(file);
	free(file);
	return 0;
}

int mp4ff_close(mp4ff_t *file)
{
	int result = 0;
	if(file->wr)
	{
		mp4ff_write_mdat(file, &(file->mdat));
		mp4ff_write_moov(file, &(file->moov));
	}

	mp4ff_delete(file);
	free(file);
	return result;
}
