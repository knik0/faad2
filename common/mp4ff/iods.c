/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4ff.h"


int mp4ff_iods_init(mp4ff_iods_t *iods)
{
	iods->version = 0;
	iods->flags = 0;
	iods->audioProfileId = 0xFF;
	iods->videoProfileId = 0xFF;
	return 0;
}

int mp4ff_iods_set_audio_profile(mp4ff_iods_t* iods, int id)
{
	iods->audioProfileId = id;
}

int mp4ff_iods_set_video_profile(mp4ff_iods_t* iods, int id)
{
	iods->videoProfileId = id;
}

int mp4ff_iods_delete(mp4ff_iods_t *iods)
{
	return 0;
}

int mp4ff_iods_dump(mp4ff_iods_t *iods)
{
	printf(" initial object descriptor\n");
	printf("  version %d\n", iods->version);
	printf("  flags %ld\n", iods->flags);
	printf("  audioProfileId %u\n", iods->audioProfileId);
	printf("  videoProfileId %u\n", iods->videoProfileId);
}

int mp4ff_read_iods(mp4ff_t *file, mp4ff_iods_t *iods)
{
	iods->version = mp4ff_read_char(file);
	iods->flags = mp4ff_read_int24(file);
	mp4ff_read_char(file); /* skip tag */
	mp4ff_read_mp4_descr_length(file);	/* skip length */
	/* skip ODID, ODProfile, sceneProfile */
	mp4ff_set_position(file, mp4ff_position(file) + 4);
	iods->audioProfileId = mp4ff_read_char(file);
	iods->videoProfileId = mp4ff_read_char(file);
	/* will skip the remainder of the atom */
}

int mp4ff_write_iods(mp4ff_t *file, mp4ff_iods_t *iods)
{
	mp4ff_atom_t atom;
	int i;

	mp4ff_atom_write_header(file, &atom, "iods");

	mp4ff_write_char(file, iods->version);
	mp4ff_write_int24(file, iods->flags);

	mp4ff_write_char(file, 0x10);	/* MP4_IOD_Tag */
	mp4ff_write_char(file, 7 + (file->moov.total_tracks * (1+1+4)));	/* length */
	mp4ff_write_int16(file, 0x004F); /* ObjectDescriptorID = 1 */
	mp4ff_write_char(file, 0xFF);	/* ODProfileLevel */
	mp4ff_write_char(file, 0xFF);	/* sceneProfileLevel */
	mp4ff_write_char(file, iods->audioProfileId);	/* audioProfileLevel */
	mp4ff_write_char(file, iods->videoProfileId);	/* videoProfileLevel */
	mp4ff_write_char(file, 0xFF);	/* graphicsProfileLevel */

	for (i = 0; i < file->moov.total_tracks; i++) {
		mp4ff_write_char(file, 0x0E);	/* ES_ID_IncTag */
		mp4ff_write_char(file, 0x04);	/* length */
		mp4ff_write_int32(file, file->moov.trak[i]->tkhd.track_id);	
	}

	/* no OCI_Descriptors */
	/* no IPMP_DescriptorPointers */
	/* no Extenstion_Descriptors */

	mp4ff_atom_write_footer(file, &atom);
}

