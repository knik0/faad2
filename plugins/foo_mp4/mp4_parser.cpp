#include "foobar2000/SDK/foobar2000.h"
#define USE_TAGGING
#include <mp4ff.h>

bool is_valid_aac_decoder_config(const void * data,unsigned bytes);

static int find_track_to_decode(mp4ff_t *infile,string_base & codec)
{
    /* find AAC track */
    int i;
    int numTracks = mp4ff_total_tracks(infile);

    for (i = 0; i < numTracks; i++)
    {
		if (mp4ff_get_track_type(infile,i)==1)
		{
			switch(mp4ff_get_audio_type(infile,i))
			{
			case 0x40:
			case 0x66:
			case 0x67:
			case 0x68:
				codec = "AAC";
				return i;
			case 0x6B:
			case 0x69:
				codec = "MP3";
				return i;
/*			case 0xE1:
				codec = "Vorbis";
				return i;
			case 0xE2:
				codec = "AC3";
				return i;*/
			}
		}
    }
	return -1;
}

static const char * tech_info_fields[] = {"replaygain_track_gain","replaygain_track_peak","replaygain_album_gain","replaygain_album_peak","tool"};

const char * check_tech_info(const char * name)
{
	unsigned n;
	for(n=0;n<tabsize(tech_info_fields);n++)
	{
		if (!stricmp_utf8(name,tech_info_fields[n])) return tech_info_fields[n];
	}
	return 0;
}


static void meta_mp4_to_fb2k(const char * name,const char * value,file_info * out)
{
	{
		const char * p_tech_info = check_tech_info(name);
		if (p_tech_info)
		{
			out->info_set(p_tech_info,value);
			return;
		}
	}

	if (!stricmp_utf8(name,"track")) name = "tracknumber";

	out->meta_add(name,value);
}


static void meta_fb2k_to_mp4(const char * name,const char * value,string_base & out_name,string_base & out_value)
{
	if (!stricmp_utf8(name,"tracknumber"))
	{
		out_name = "track";
		out_value = value;
	}
	else
	{
		out_name = name;
		out_value = value;
	}
}

class input_mp4 : public input
{
public:
    int track;
    void *sample_buffer;

    mp4ff_t *infile;
    unsigned sampleId, numSamples;
	unsigned m_offset;

	reader * mp4file;

	packet_decoder * p_decoder;


    unsigned char *buffer;
    unsigned int buffer_size;


    /* for gapless decoding */

	mp4ff_callback_t mp4cb;
	
	double m_length;
	uint32_t m_timescale;
	int m_skip_samples;
	uint32_t m_skip_frames;

	unsigned m_expected_sample_rate,m_expected_channels;

	audio_chunk_i m_tempchunk;

    static uint32_t callback_read(void *udata, void *buffer, uint32_t length)
	{
		return reinterpret_cast<input_mp4*>(udata)->mp4file->read(buffer,length);
	}

    static uint32_t callback_write(void *udata, void *buffer, uint32_t length)
	{
		return reinterpret_cast<input_mp4*>(udata)->mp4file->write(buffer,length);
	}

    static uint32_t callback_seek(void *udata, uint64_t position)
	{
		return reinterpret_cast<input_mp4*>(udata)->mp4file->seek(position) ? 1 : 0;
	}
	
	static uint32_t callback_truncate(void *udata)
	{
		return reinterpret_cast<input_mp4*>(udata)->mp4file->set_eof() ? 1 : 0;
	}
	
	void cleanup()
	{
		if (infile) {mp4ff_close(infile);infile=0;}
		if (p_decoder)
		{
			p_decoder->service_release();
			p_decoder = 0;
		}
	}

    input_mp4()
    {
		mp4cb.user_data = reinterpret_cast<void*>(this);
		mp4cb.read = callback_read;
		mp4cb.write = callback_write;
		mp4cb.seek = callback_seek;
		mp4cb.truncate = callback_truncate;

		m_skip_frames = 0;
		p_decoder = 0;
		infile = 0;
		m_offset = 0;
		m_skip_samples = 0;
    }

    ~input_mp4()
    {
		cleanup();
    }

    virtual bool test_filename(const char * fn,const char * ext)
    {
        return (!stricmp(ext,"MP4") || !stricmp(ext,"M4A"));
    }

    virtual bool open(reader *r, file_info *info, unsigned flags)
    {
		if (!r->can_seek())
		{
			console::error("Unseekable streams not supported.");
			return false;
		}
		string8 codecname;

		cleanup();

		mp4file = r;



		infile = mp4ff_open_read(&mp4cb);
		if (!infile)
		{
			cleanup();
			console::error("Error parsing MP4 file.");
			return 0;
		}

		if ((track = find_track_to_decode(infile,codecname)) < 0)
		{
			cleanup();
			console::error("Unable to find correct sound track in the MP4 file.");
			return 0;
		}

		p_decoder = packet_decoder::create(codecname);
		if (p_decoder == 0)
		{
			cleanup();
			console::error("Unable to find correct packet decoder object.");
			return 0;
		}

		info->info_set("codec",codecname);

		buffer = NULL;
		buffer_size = 0;
		mp4ff_get_decoder_config(infile, track, &buffer, &buffer_size);

		if (!p_decoder->init(buffer,buffer_size,info))
		{
			if (buffer) free(buffer);
			cleanup();
			console::error("Error initializing decoder.");
			return 0;
		}

		m_timescale = mp4ff_time_scale(infile,track);

		if (buffer)
		{
			free(buffer);
		}

		{
			m_expected_sample_rate = mp4ff_get_sample_rate(infile,track);
			m_expected_channels = mp4ff_get_channel_count(infile,track);
			if (m_timescale != m_expected_sample_rate)
			{
				cleanup();
				console::error("Different sample rate / time scales not supported.");
				return 0;
			}

			char *tag = NULL, *item = NULL;
			int k, j;
			
			
			if (m_timescale<=0)
				m_length = -1.0;
			else
			{
				int64_t duration = mp4ff_get_track_duration_use_offsets(infile,track);
				if (duration == -1)
					m_length = -1.0;
				else
				{
					m_length = (double)duration / (double)m_timescale;
				}
			}

			info->set_length(m_length);


			if (flags & OPEN_FLAG_GET_INFO)
			{
				info->info_set_int("bitrate",(mp4ff_get_avg_bitrate(infile,track) + 500) / 1000);
				info->info_set_int("channels",m_expected_channels);
				info->info_set_int("samplerate",m_expected_sample_rate);

				j = mp4ff_meta_get_num_items(infile);
				for (k = 0; k < j; k++)
				{
					if (mp4ff_meta_get_by_index(infile, k, &item, &tag))
					{
						if (item != NULL && tag != NULL)
						{
							meta_mp4_to_fb2k(item,tag,info);
							free(item); item = NULL;
							free(tag); tag = NULL;
						}
					}
				}
			}
		}

		numSamples = mp4ff_num_samples(infile, track);

		m_skip_samples = 0;

		m_offset = 0;

		sampleId = 0;

        return 1;
    }

    virtual int run(audio_chunk * chunk)
    {
		if (p_decoder==0)
		{
			console::error("Attempting to decode while not open.");
			return -1;
		}

		if (sampleId >= numSamples) return 0;

		bool done = false;

		do		
		{
			/* get acces unit from MP4 file */
			buffer = NULL;
			buffer_size = 0;

			if (mp4ff_read_sample(infile, track, sampleId, &buffer,  &buffer_size) == 0)
			{
				cleanup();
				console::error(uStringPrintf("Reading from MP4 file failed: frame %u of %u.",sampleId,numSamples));
				return -1;
			}

			m_tempchunk.reset();
			if (!p_decoder->decode(buffer,buffer_size,&m_tempchunk))
			{
				cleanup();
				console::error("Decode error.");
				return -1;
			}

			if (buffer) free(buffer);

			if (m_skip_frames>0) m_skip_frames--;
			else
			{
				unsigned offset = mp4ff_get_sample_offset(infile,track,sampleId);
				unsigned duration = mp4ff_get_sample_duration(infile, track, sampleId);
	//			console::info(uStringPrintf("duration: %u, offset: %u",duration,offset));

				if (m_tempchunk.is_empty())
				{
					if (duration > 0)
					{
						m_tempchunk.set_srate(m_expected_sample_rate);
						m_tempchunk.set_channels(m_expected_channels);
						m_tempchunk.pad_with_silence(duration);
					//	console::warning("Decoder returned empty chunk from a non-empty MP4 frame.");
					}
				}
				else
				{
					if (m_tempchunk.get_srate() != m_expected_sample_rate)
					{
						cleanup();
						console::error(uStringPrintf("Expected sample rate: %u, got: %u.",m_expected_sample_rate,m_tempchunk.get_srate()));
						return -1;
					}
					if (m_tempchunk.get_channels() != m_expected_channels)
					{
						cleanup();
						console::error(uStringPrintf("Expected channels: %u, got: %u.",m_expected_channels,m_tempchunk.get_channels()));
						return -1;
					}
				}
				unsigned samplerate,channels,decoded_sample_count;

				samplerate = m_tempchunk.get_srate();
				channels = m_tempchunk.get_channels();
				decoded_sample_count = m_tempchunk.get_sample_count();

				if (decoded_sample_count < duration)
				{
					//console::warning("Decoded MP4 frame smaller than expected.");
					decoded_sample_count = duration;
					m_tempchunk.pad_with_silence(decoded_sample_count);
				}

				if (duration < offset) duration = 0;
				else duration -= offset;

				if (m_skip_samples>0)
				{
					unsigned int delta = (unsigned)m_skip_samples;
					if (delta > duration) delta = duration;
					offset += delta;
					duration -= delta;
					m_skip_samples -= delta;
				}


				if (duration > 0)
				{
					chunk->set_data_64(m_tempchunk.get_data() + offset * channels,duration,channels,samplerate);
					done = true;
				}
			}
			sampleId++;
		}
		while(!done && sampleId < numSamples);
		

        return done ? 1 : 0;
    }

    virtual set_info_t set_info(reader *r, const file_info * info)
    {
#if 0
/* metadata tag structure */
typedef struct
{
    char *item;
    char *value;
} mp4ff_tag_t;

/* metadata list structure */
typedef struct
{
    mp4ff_tag_t *tags;
    uint32_t count;
} mp4ff_metadata_t;
#endif



		unsigned rv;
		ptr_list_t<char> freeme;
		mem_block_list_t<mp4ff_tag_t> tags;
		mp4ff_metadata_t mp4meta;

		{
			string8_fastalloc name,value;
			mp4ff_tag_t tag;

			unsigned n, m;
			
			m = info->meta_get_count();
			for(n=0;n<m;n++)
			{
				meta_fb2k_to_mp4(info->meta_enum_name(n),info->meta_enum_value(n),name,value);
				freeme.add_item(tag.item = strdup(name));
				freeme.add_item(tag.value = strdup(value));
				tags.add_item(tag);
			}


			m = info->info_get_count();

			for(n=0;n<m;n++)
			{
				const char * p_name = check_tech_info(info->info_enum_name(n));
				if (p_name)
				{
					tag.item = const_cast<char*>(p_name);
					tag.value = const_cast<char*>(info->info_enum_value(n));
					tags.add_item(tag);
				}
			}
		}

		mp4meta.count = tags.get_count();
		mp4meta.tags = const_cast<mp4ff_tag_t*>(tags.get_ptr());

		mp4file = r;
		rv = mp4ff_meta_update(&mp4cb,&mp4meta);
		mp4file = 0;

		freeme.free_all();

		return rv ? SET_INFO_SUCCESS : SET_INFO_FAILURE;

    }

    virtual bool seek(double seconds)
    {
		if (p_decoder == 0)
		{
			console::error("Attempting to seek while not open.");
			return false;
		}
        if (seconds >= m_length) {
            sampleId = numSamples;
            return true;
        }
		
		unsigned max_frame_dependency = p_decoder->get_max_frame_dependency();
		int64_t offset = (int64_t)(seconds * m_timescale + 0.5);
		int32_t skip_samples = 0;
		unsigned dest_sample = mp4ff_find_sample_use_offsets(infile,track,offset,&skip_samples);
//		console::info(uStringPrintf("%u %u %u",(unsigned)offset,dest_sample,skip_samples));

		if (dest_sample == (-1)) return false;

		if (dest_sample < max_frame_dependency)
		{
			m_skip_frames = dest_sample;
			dest_sample = 0;
		}
		else
		{
			m_skip_frames = max_frame_dependency;
			dest_sample -= max_frame_dependency;
		}

		sampleId = dest_sample;
		
		m_skip_samples = skip_samples;

		m_offset = 0;

		p_decoder->reset_after_seek();

        return true;

    }

    virtual bool is_our_content_type(const char *url, const char *type)
    {
        return !stricmp(type, "audio/mp4") || !stricmp(type, "audio/x-mp4");
    }
};

static service_factory_t<input, input_mp4> foo_mp4;