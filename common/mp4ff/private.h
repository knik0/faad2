#ifndef PRIVATE_H
#define PRIVATE_H

/* ================================= structures */

#define HEADER_LENGTH 8
#define MAXTRACKS 1024

#ifdef _WIN32
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#endif

#endif


typedef struct
{
	float values[9];
} mp4ff_matrix_t;


typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	int track_id;
	long reserved1;
	long duration;
	char reserved2[8];
	int layer;
	int alternate_group;
	float volume;
	long reserved3;
	mp4ff_matrix_t matrix;
	float track_width;
	float track_height;
	int is_video;
	int is_audio;
} mp4ff_tkhd_t;




/* ===================== sample table ======================== // */



/* sample description */

typedef struct
{
	int version;
	long flags;
	int decoderConfigLen;
	unsigned char* decoderConfig;
} mp4ff_esds_t;

typedef struct
{
	char format[4];
	char reserved[6];
	int data_reference;

/* common to audio and video */
	int version;
	int revision;
	char vendor[4];

/* video description */
	long temporal_quality;
	long spatial_quality;
	int width;
	int height;
	float dpi_horizontal;
	float dpi_vertical;
	long data_size;
	int frames_per_sample;
	char compressor_name[32];
	int depth;
	float gamma;
	int fields;    /* 0, 1, or 2 */
	int field_dominance;   /* 0 - unknown     1 - top first     2 - bottom first */

/* audio description */
	int channels;
	int sample_size;
	int compression_id;
	int packet_size;
	float sample_rate;

/* MP4 elementary stream descriptor */
	mp4ff_esds_t esds;

} mp4ff_stsd_table_t;


typedef struct
{
	int version;
	long flags;
	long total_entries;
	mp4ff_stsd_table_t *table;
} mp4ff_stsd_t;


/* time to sample */
typedef struct
{
	long sample_count;
	long sample_duration;
} mp4ff_stts_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	long entries_allocated;
	mp4ff_stts_table_t *table;
} mp4ff_stts_t;


/* sync sample */
typedef struct
{
	long sample;
} mp4ff_stss_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	long entries_allocated;
	mp4ff_stss_table_t *table;
} mp4ff_stss_t;


/* sample to chunk */
typedef struct
{
	long chunk;
	long samples;
	long id;
} mp4ff_stsc_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;
	mp4ff_stsc_table_t *table;
} mp4ff_stsc_t;


/* sample size */
typedef struct
{
	long size;
} mp4ff_stsz_table_t;

typedef struct
{
	int version;
	long flags;
	long sample_size;
	long total_entries;

	long entries_allocated;    /* used by the library for allocating a table */
	mp4ff_stsz_table_t *table;
} mp4ff_stsz_t;


/* chunk offset */
typedef struct
{
	long offset;
} mp4ff_stco_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;    /* used by the library for allocating a table */
	mp4ff_stco_table_t *table;
} mp4ff_stco_t;

/* composition time to sample */
typedef struct
{
	long sample_count;
	long sample_offset;
} mp4ff_ctts_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	long entries_allocated;
	mp4ff_ctts_table_t *table;
} mp4ff_ctts_t;


/* sample table */
typedef struct
{
	int version;
	long flags;
	mp4ff_stsd_t stsd;
	mp4ff_stts_t stts;
	mp4ff_stss_t stss;
	mp4ff_stsc_t stsc;
	mp4ff_stsz_t stsz;
	mp4ff_stco_t stco;
	mp4ff_ctts_t ctts;
} mp4ff_stbl_t;

/* data reference */

typedef struct
{
	long size;
	char type[4];
	int version;
	long flags;
	char *data_reference;
} mp4ff_dref_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	mp4ff_dref_table_t *table;
} mp4ff_dref_t;

/* data information */

typedef struct
{
	mp4ff_dref_t dref;
} mp4ff_dinf_t;

/* video media header */

typedef struct
{
	int version;
	long flags;
	int graphics_mode;
	int opcolor[3];
} mp4ff_vmhd_t;


/* sound media header */

typedef struct
{
	int version;
	long flags;
	int balance;
	int reserved;
} mp4ff_smhd_t;

/* handler reference */

typedef struct
{
	int version;
	long flags;
	char component_type[4];
	char component_subtype[4];
	long component_manufacturer;
	long component_flags;
	long component_flag_mask;
	char component_name[256];
} mp4ff_hdlr_t;

/* media information */

typedef struct
{
	int is_video;
	int is_audio;
	mp4ff_vmhd_t vmhd;
	mp4ff_smhd_t smhd;
	mp4ff_stbl_t stbl;
	mp4ff_hdlr_t hdlr;
	mp4ff_dinf_t dinf;
} mp4ff_minf_t;


/* media header */

typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	int language;
	int quality;
} mp4ff_mdhd_t;


/* media */

typedef struct
{
	mp4ff_mdhd_t mdhd;
	mp4ff_minf_t minf;
	mp4ff_hdlr_t hdlr;
} mp4ff_mdia_t;

/* edit list */
typedef struct
{
	long duration;
	long time;
	float rate;
} mp4ff_elst_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;

	mp4ff_elst_table_t *table;
} mp4ff_elst_t;

typedef struct
{
	mp4ff_elst_t elst;
} mp4ff_edts_t;

typedef struct
{
	mp4ff_tkhd_t tkhd;
	mp4ff_mdia_t mdia;
	mp4ff_edts_t edts;
} mp4ff_trak_t;

typedef struct
{
	int version;
	long flags;
	int audioProfileId;
	int videoProfileId;
} mp4ff_iods_t;

typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	float preferred_rate;
	float preferred_volume;
	char reserved[10];
	mp4ff_matrix_t matrix;
	long preview_time;
	long preview_duration;
	long poster_time;
	long selection_time;
	long selection_duration;
	long current_time;
	long next_track_id;
} mp4ff_mvhd_t;

typedef struct
{
	char *copyright;
	int copyright_len;
	char *name;
	int name_len;
	char *info;
	int info_len;
} mp4ff_udta_t;


typedef struct
{
	int total_tracks;

	mp4ff_mvhd_t mvhd;
	mp4ff_iods_t iods;
	mp4ff_trak_t *trak[MAXTRACKS];
	mp4ff_udta_t udta;
} mp4ff_moov_t;

typedef struct
{
	long start;
	long size;
} mp4ff_mdat_t;


typedef struct
{
	long start;      /* byte start in file */
	long end;        /* byte endpoint in file */
	long size;       /* byte size for writing */
	char type[4];
} mp4ff_atom_t;

/* table of pointers to every track */
typedef struct
{
	mp4ff_trak_t *track; /* real quicktime track corresponding to this table */
	int channels;            /* number of audio channels in the track */
	long current_position;   /* current sample in output file */
	long current_chunk;      /* current chunk in output file */

	void *codec;
} mp4ff_audio_map_t;

typedef struct
{
	mp4ff_trak_t *track;
	long current_position;
	long current_chunk;

/* Array of pointers to frames of raw data when caching frames. */
	unsigned char **frame_cache;
	long frames_cached;

	void *codec;
} mp4ff_video_map_t;

/* file descriptor passed to all routines */

typedef struct
{
    size_t (*read)(void *buffer, size_t length);
    size_t (*write)(void *buffer, size_t length);
    int64_t (*get_position)();
    int64_t (*get_length)();
    int (*seek)(int64_t position);
} mp4_callback_t;

typedef struct
{
	mp4_callback_t *stream;
	long total_length;
	mp4ff_mdat_t mdat;
	mp4ff_moov_t moov;
	int rd;
	int wr;

/* mapping of audio channels to movie tracks */
/* one audio map entry exists for each channel */
	int total_atracks;
	mp4ff_audio_map_t *atracks;

/* mapping of video tracks to movie tracks */
	int total_vtracks;
	mp4ff_video_map_t *vtracks;

/* for begining and ending frame writes where the user wants to write the  */
/* file descriptor directly */
	long offset;

/* I/O */
	long file_position;      /* Current position of file descriptor */

/* Parameters for frame currently being decoded */
	int do_scaling;
	int in_x, in_y, in_w, in_h, out_w, out_h;
	int color_model;

/* Cached value for mp4ff_video_frame */
	long last_frame;
	long last_start;
	int last_stts_index;

} mp4ff_t;

#endif
