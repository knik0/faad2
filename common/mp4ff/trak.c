#include "mp4ff.h"




int mp4ff_trak_init(mp4ff_trak_t *trak)
{
	mp4ff_tkhd_init(&(trak->tkhd));
	mp4ff_edts_init(&(trak->edts));
	mp4ff_mdia_init(&(trak->mdia));
	return 0;
}

int mp4ff_trak_init_video(mp4ff_t *file, 
							mp4ff_trak_t *trak, 
							int frame_w, 
							int frame_h, 
							float frame_rate,
							int time_scale,
							char *compressor)
{
	mp4ff_tkhd_init_video(file, &(trak->tkhd), frame_w, frame_h);
	mp4ff_mdia_init_video(file, &(trak->mdia), frame_w, frame_h, 
		frame_rate, time_scale, compressor);
	mp4ff_edts_init_table(&(trak->edts));

	return 0;
}

int mp4ff_trak_init_audio(mp4ff_t *file, 
							mp4ff_trak_t *trak, 
							int channels, 
							int sample_rate, 
							int bits, 
							int sample_size,
							int time_scale,
							int sample_duration,
							char *compressor)
{
	mp4ff_mdia_init_audio(file, &(trak->mdia), channels, sample_rate, bits,
		sample_size, time_scale, sample_duration, compressor);
	mp4ff_edts_init_table(&(trak->edts));

	return 0;
}

int mp4ff_trak_delete(mp4ff_trak_t *trak)
{
	mp4ff_tkhd_delete(&(trak->tkhd));
	return 0;
}


int mp4ff_trak_dump(mp4ff_trak_t *trak)
{
	printf(" track\n");
	mp4ff_tkhd_dump(&(trak->tkhd));
	mp4ff_edts_dump(&(trak->edts));
	mp4ff_mdia_dump(&(trak->mdia));

	return 0;
}


mp4ff_trak_t* mp4ff_add_trak(mp4ff_moov_t *moov)
{
	if(moov->total_tracks < MAXTRACKS)
	{
		moov->trak[moov->total_tracks] = malloc(sizeof(mp4ff_trak_t));
		mp4ff_trak_init(moov->trak[moov->total_tracks]);
		moov->total_tracks++;
	}
	return moov->trak[moov->total_tracks - 1];
}

mp4ff_trak_t* mp4ff_find_track_by_id(mp4ff_moov_t *moov, int trackId)
{
	int i;

	for (i = 0; i < moov->total_tracks; i++) {
		if (moov->trak[i]->tkhd.track_id == trackId) {
			return moov->trak[i];
		}
	}
	
	return NULL;
}

int mp4ff_delete_trak(mp4ff_moov_t *moov, mp4ff_trak_t *trak)
{
	int i, j;

	for (i = 0; i < moov->total_tracks; i++) {
		if (moov->trak[i] == trak) {
			mp4ff_trak_delete(trak);
			free(trak);
			moov->trak[i] = NULL;
			for (j = i + 1; j < moov->total_tracks; j++, i++) {
				moov->trak[i] = moov->trak[j];
			}
			moov->trak[j] = NULL;
			moov->total_tracks--;
			return 0;
		}
	}
	return -1;
}


int mp4ff_read_trak(mp4ff_t *file, mp4ff_trak_t *trak, mp4ff_atom_t *trak_atom)
{
	mp4ff_atom_t leaf_atom;

	do
	{
		mp4ff_atom_read_header(file, &leaf_atom);

/* mandatory */
		if(mp4ff_atom_is(&leaf_atom, "tkhd"))
			{ mp4ff_read_tkhd(file, &(trak->tkhd)); }
		else
		if(mp4ff_atom_is(&leaf_atom, "mdia"))
			{ mp4ff_read_mdia(file, &(trak->mdia), &leaf_atom); }
		else
/* optional */
		if(mp4ff_atom_is(&leaf_atom, "edts"))
			{ mp4ff_read_edts(file, &(trak->edts), &leaf_atom); }
		else
			mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < trak_atom->end);

	return 0;
}

int mp4ff_write_trak(mp4ff_t *file, mp4ff_trak_t *trak, long moov_time_scale)
{
	long duration;
	long timescale;
	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "trak");
	mp4ff_trak_duration(trak, &duration, &timescale);

	/* printf("mp4ff_write_trak duration %d\n", duration); */

	/* get duration in movie's units */
	if (timescale) {
		trak->tkhd.duration = 
			(long)((float)duration / timescale * moov_time_scale);
	} else {
		trak->tkhd.duration = 0;
	}
	trak->mdia.mdhd.duration = duration;
	trak->mdia.mdhd.time_scale = timescale;

	mp4ff_write_tkhd(file, &(trak->tkhd));
	mp4ff_write_edts(file, &(trak->edts), trak->tkhd.duration);
	mp4ff_write_mdia(file, &(trak->mdia));

	mp4ff_atom_write_footer(file, &atom);

	return 0;
}

long mp4ff_track_end(mp4ff_trak_t *trak)
{
/* get the byte endpoint of the track in the file */
	long size = 0;
	long chunk, chunk_offset, chunk_samples, sample;
	mp4ff_stsz_t *stsz = &(trak->mdia.minf.stbl.stsz);
	mp4ff_stsz_table_t *table = stsz->table;
	mp4ff_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	mp4ff_stco_t *stco;

/* get the last chunk offset */
/* the chunk offsets contain the HEADER_LENGTH themselves */
	stco = &(trak->mdia.minf.stbl.stco);
	chunk = stco->total_entries;
	size = chunk_offset = stco->table[chunk - 1].offset;

/* get the number of samples in the last chunk */
	chunk_samples = stsc->table[stsc->total_entries - 1].samples;

/* get the size of last samples */
#ifdef NOTDEF
	if(stsz->sample_size)
	{
/* assume audio so calculate the sample size */
		size += chunk_samples * stsz->sample_size
			* trak->mdia.minf.stbl.stsd.table[0].channels 
			* trak->mdia.minf.stbl.stsd.table[0].sample_size / 8;
	}
	else
	{
/* assume video */
#endif
		for(sample = stsz->total_entries - chunk_samples; 
			sample >= 0 && sample < stsz->total_entries; sample++)
		{
			size += stsz->table[sample].size;
		}
#ifdef NOTDEF
	}
#endif

	return size;
}

long mp4ff_track_samples(mp4ff_t *file, mp4ff_trak_t *trak)
{
/*printf("file->rd %d file->wr %d\n", file->rd, file->wr); */
	if(file->wr)
	{
/* get the sample count when creating a new file */
 		mp4ff_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
		long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
		long chunk = trak->mdia.minf.stbl.stco.total_entries;
		long sample;

		if(chunk)
		{
			sample = mp4ff_sample_of_chunk(trak, chunk);
			sample += table[total_entries - 1].samples;
		}
		else 
			sample = 0;
		
		return sample;
	}
	else
	{
/* get the sample count when reading only */
		mp4ff_stts_t *stts = &(trak->mdia.minf.stbl.stts);
		int i;
		long total = 0;

		for(i = 0; i < stts->total_entries; i++)
		{
			total += stts->table[i].sample_count;
		}
		return total;
	}
}

long mp4ff_sample_of_chunk(mp4ff_trak_t *trak, long chunk)
{
	mp4ff_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
	long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
	long chunk1entry, chunk2entry;
	long chunk1, chunk2, chunks, total = 0;

	for(chunk1entry = total_entries - 1, chunk2entry = total_entries; 
		chunk1entry >= 0; 
		chunk1entry--, chunk2entry--)
	{
		chunk1 = table[chunk1entry].chunk;

		if(chunk > chunk1)
		{
			if(chunk2entry < total_entries)
			{
				chunk2 = table[chunk2entry].chunk;

				if(chunk < chunk2) chunk2 = chunk;
			}
			else
				chunk2 = chunk;

			chunks = chunk2 - chunk1;

			total += chunks * table[chunk1entry].samples;
		}
	}

	return total;
}

int mp4ff_chunk_of_sample(long *chunk_sample, long *chunk, mp4ff_trak_t *trak, long sample)
{
	mp4ff_stsc_table_t *table = NULL;
	long total_entries = 0;
	long chunk2entry;
	long chunk1, chunk2, chunk1samples, range_samples, total = 0;

	if (trak == NULL) {
		return -1;
	}
 	table = trak->mdia.minf.stbl.stsc.table;
	total_entries = trak->mdia.minf.stbl.stsc.total_entries;

	chunk1 = 1;
	chunk1samples = 0;
	chunk2entry = 0;

	do
	{
		chunk2 = table[chunk2entry].chunk;
		*chunk = chunk2 - chunk1;
		range_samples = *chunk * chunk1samples;

		if(sample < total + range_samples) break;

		chunk1samples = table[chunk2entry].samples;
		chunk1 = chunk2;

		if(chunk2entry < total_entries)
		{
			chunk2entry++;
			total += range_samples;
		}
	}while(chunk2entry < total_entries);

	if(chunk1samples)
		*chunk = (sample - total) / chunk1samples + chunk1;
	else
		*chunk = 1;

	*chunk_sample = total + (*chunk - chunk1) * chunk1samples;
	return 0;
}

long mp4ff_chunk_to_offset(mp4ff_trak_t *trak, long chunk)
{
	mp4ff_stco_table_t *table = NULL;

	if (trak == NULL) {
		return -1;
	}
	table = trak->mdia.minf.stbl.stco.table;

	if(trak->mdia.minf.stbl.stco.total_entries && chunk > trak->mdia.minf.stbl.stco.total_entries)
		return table[trak->mdia.minf.stbl.stco.total_entries - 1].offset;
	else
	if(trak->mdia.minf.stbl.stco.total_entries)
		return table[chunk - 1].offset;
	else
		return HEADER_LENGTH;
	
	return 0;
}

long mp4ff_offset_to_chunk(long *chunk_offset, mp4ff_trak_t *trak, long offset)
{
	mp4ff_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int i;

	for(i = trak->mdia.minf.stbl.stco.total_entries - 1; i >= 0; i--)
	{
		if(table[i].offset <= offset)
		{
			*chunk_offset = table[i].offset;
			return i + 1;
		}
	}
	*chunk_offset = HEADER_LENGTH;
	return 1;
}

long mp4ff_sample_range_size(mp4ff_trak_t *trak, long chunk_sample, long sample)
{
	mp4ff_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	long i, total;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
/* assume audio */
		return mp4ff_samples_to_bytes(trak, sample - chunk_sample);
	}
	else
	{
/* probably video */
		for(i = chunk_sample, total = 0; i < sample; i++)
		{
			total += trak->mdia.minf.stbl.stsz.table[i].size;
		}
	}
	return total;
}

long mp4ff_sample_to_offset(mp4ff_trak_t *trak, long sample)
{
	long chunk, chunk_sample, chunk_offset1, chunk_offset2;

	if (trak == NULL) {
		return -1;
	}

	mp4ff_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
	chunk_offset1 = mp4ff_chunk_to_offset(trak, chunk);
	chunk_offset2 = chunk_offset1 + mp4ff_sample_range_size(trak, chunk_sample, sample);
/*printf("mp4ff_sample_to_offset chunk %d sample %d chunk_offset %d chunk_sample %d chunk_offset + samples %d\n", */
/*	 chunk, sample, chunk_offset1, chunk_sample, chunk_offset2); */
	return chunk_offset2;
}

long mp4ff_offset_to_sample(mp4ff_trak_t *trak, long offset)
{
	long chunk_offset;
	long chunk = mp4ff_offset_to_chunk(&chunk_offset, trak, offset);
	long chunk_sample = mp4ff_sample_of_chunk(trak, chunk);
	long sample, sample_offset;
	mp4ff_stsz_table_t *table = trak->mdia.minf.stbl.stsz.table;
	long total_samples = trak->mdia.minf.stbl.stsz.total_entries;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		sample = chunk_sample + (offset - chunk_offset) / 
			trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	for(sample = chunk_sample, sample_offset = chunk_offset; 
		sample_offset < offset && sample < total_samples; )
	{
		sample_offset += table[sample].size;
		if(sample_offset < offset) sample++;
	}
	
	return sample;
}

int mp4ff_update_tables(mp4ff_t *file, 
							mp4ff_trak_t *trak, 
							long offset, 
							long chunk, 
							long sample, 
							long samples, 
							long sample_size,
							long sample_duration,
							unsigned char isSyncSample,
							long renderingOffset)
{
	if (offset + sample_size > file->mdat.size) {
		file->mdat.size = offset + sample_size;
	}
	mp4ff_update_stco(&(trak->mdia.minf.stbl.stco), chunk, offset);
	if (sample_size) {
		mp4ff_update_stsz(&(trak->mdia.minf.stbl.stsz), sample, sample_size);
	}
	mp4ff_update_stsc(&(trak->mdia.minf.stbl.stsc), chunk, samples);
	if (sample_duration) {
		mp4ff_update_stts(&(trak->mdia.minf.stbl.stts), sample_duration);
	}
	if (isSyncSample) {
		mp4ff_update_stss(&(trak->mdia.minf.stbl.stss), sample);
	}
	mp4ff_update_ctts(&(trak->mdia.minf.stbl.ctts), renderingOffset);
	return 0;
}

int mp4ff_trak_duration(mp4ff_trak_t *trak, long *duration, long *timescale)
{
	mp4ff_stts_t *stts = &(trak->mdia.minf.stbl.stts);
	int i;
	*duration = 0;

	for(i = 0; i < stts->total_entries; i++) {
		*duration += stts->table[i].sample_duration * stts->table[i].sample_count;
	}

	*timescale = trak->mdia.mdhd.time_scale;
	return 0;
}

int mp4ff_trak_fix_counts(mp4ff_t *file, mp4ff_trak_t *trak)
{
	long samples = mp4ff_track_samples(file, trak);

	if (trak->mdia.minf.stbl.stts.total_entries == 1) {
		trak->mdia.minf.stbl.stts.table[0].sample_count = samples;
	}

	if(trak->mdia.minf.stbl.stsz.sample_size)
		trak->mdia.minf.stbl.stsz.total_entries = samples;

	return 0;
}

long mp4ff_chunk_samples(mp4ff_trak_t *trak, long chunk)
{
	long result, current_chunk;
	mp4ff_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	long i = stsc->total_entries - 1;

	do
	{
		current_chunk = stsc->table[i].chunk;
		result = stsc->table[i].samples;
		i--;
	}while(i >= 0 && current_chunk > chunk);

	return result;
}

int mp4ff_trak_shift_offsets(mp4ff_trak_t *trak, long offset)
{
	mp4ff_stco_t *stco = &(trak->mdia.minf.stbl.stco);
	int i;

	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset += offset;
	}
	return 0;
}
