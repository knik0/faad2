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


int mp4ff_esds_init(mp4ff_esds_t *esds)
{
	esds->version = 0;
	esds->flags = 0;
	esds->decoderConfigLen = 0;
	esds->decoderConfig = NULL;
	return 0;
}

int mp4ff_esds_get_decoder_config(mp4ff_esds_t* esds, unsigned char** ppBuf, int* pBufSize)
{
	if (esds->decoderConfig == NULL || esds->decoderConfigLen == 0) {
		*ppBuf = NULL;
		*pBufSize = 0;
	} else {
		*ppBuf = malloc(esds->decoderConfigLen);
		if (*ppBuf == NULL) {
			*pBufSize = 0;
			return 1;
		}
		memcpy(*ppBuf, esds->decoderConfig, esds->decoderConfigLen);
		*pBufSize = esds->decoderConfigLen;
	}
	return 0;
}

int mp4ff_esds_set_decoder_config(mp4ff_esds_t* esds, unsigned char* pBuf, int bufSize)
{
	free(esds->decoderConfig);
	esds->decoderConfig = malloc(bufSize);
	if (esds->decoderConfig) {
		memcpy(esds->decoderConfig, pBuf, bufSize);
		esds->decoderConfigLen = bufSize;
		return 0;
	}
	return 1;
}

int mp4ff_esds_delete(mp4ff_esds_t *esds)
{
	free(esds->decoderConfig);
	return 0;
}

int mp4ff_esds_dump(mp4ff_esds_t *esds)
{
	int i;

	printf("       elementary stream descriptor\n");
	printf("        version %d\n", esds->version);
	printf("        flags %ld\n", esds->flags);
	printf("        decoder config ");
	for (i = 0; i < esds->decoderConfigLen; i++) {	
		printf("%02x ", esds->decoderConfig[i]);
	}
	printf("\n");
}

int mp4ff_read_esds(mp4ff_t *file, mp4ff_esds_t *esds)
{
	uint8_t tag;

	esds->version = mp4ff_read_char(file);
	esds->flags = mp4ff_read_int24(file);

	/* get and verify ES_DescrTag */
	tag = mp4ff_read_char(file);
	if (tag == 0x03) {
		/* read length */
		if (mp4ff_read_mp4_descr_length(file) < 5 + 15) {
			return 1;
		}
		/* skip 3 bytes */
		mp4ff_set_position(file, mp4ff_position(file) + 3);
	} else {
		/* skip 2 bytes */
		mp4ff_set_position(file, mp4ff_position(file) + 2);
	}

	/* get and verify DecoderConfigDescrTab */
	if (mp4ff_read_char(file) != 0x04) {
		return 1;
	}

	/* read length */
	if (mp4ff_read_mp4_descr_length(file) < 15) {
		return 1;
	}

	/* skip 13 bytes */
	mp4ff_set_position(file, mp4ff_position(file) + 13);

	/* get and verify DecSpecificInfoTag */
	if (mp4ff_read_char(file) != 0x05) {
		return 1;
	}

	/* read length */
	esds->decoderConfigLen = mp4ff_read_mp4_descr_length(file); 

	free(esds->decoderConfig);
	esds->decoderConfig = malloc(esds->decoderConfigLen);
	if (esds->decoderConfig) {
		mp4ff_read_data(file, esds->decoderConfig, esds->decoderConfigLen);
	} else {
		esds->decoderConfigLen = 0;
	}

	/* will skip the remainder of the atom */
	return 0;
}

int mp4ff_write_esds_common(mp4ff_t *file, mp4ff_esds_t *esds, int esid, unsigned int objectType, unsigned int streamType)
{
	mp4ff_atom_t atom;

	mp4ff_atom_write_header(file, &atom, "esds");

	mp4ff_write_char(file, esds->version);
	mp4ff_write_int24(file, esds->flags);

	mp4ff_write_char(file, 0x03);	/* ES_DescrTag */
	mp4ff_write_mp4_descr_length(file, 
		3 + (5 + (13 + (5 + esds->decoderConfigLen))) + 3, /*FALSE*/0);

	mp4ff_write_int16(file, esid);
	mp4ff_write_char(file, 0x10);	/* streamPriorty = 16 (0-31) */

	/* DecoderConfigDescriptor */
	mp4ff_write_char(file, 0x04);	/* DecoderConfigDescrTag */
	mp4ff_write_mp4_descr_length(file, 
		13 + (5 + esds->decoderConfigLen), 0 /*FALSE*/);

	mp4ff_write_char(file, objectType); /* objectTypeIndication */
	mp4ff_write_char(file, streamType); /* streamType */

	mp4ff_write_int24(file, 0);		/* buffer size */
	mp4ff_write_int32(file, 0);		/* max bitrate */
	mp4ff_write_int32(file, 0);		/* average bitrate */

	mp4ff_write_char(file, 0x05);	/* DecSpecificInfoTag */
	mp4ff_write_mp4_descr_length(file, esds->decoderConfigLen, 0 /*FALSE*/);
	mp4ff_write_data(file, esds->decoderConfig, esds->decoderConfigLen);

	/* SLConfigDescriptor */
	mp4ff_write_char(file, 0x06);	/* SLConfigDescrTag */
	mp4ff_write_char(file, 0x01);	/* length */
	mp4ff_write_char(file, 0x02);	/* constant in mp4 files */

	/* no IPI_DescrPointer */
	/* no IP_IdentificationDataSet */
	/* no IPMP_DescriptorPointer */
	/* no LanguageDescriptor */
	/* no QoS_Descriptor */
	/* no RegistrationDescriptor */
	/* no ExtensionDescriptor */

	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_write_esds_audio(mp4ff_t *file, mp4ff_esds_t *esds, int esid)
{
	return mp4ff_write_esds_common(file, esds, esid, (unsigned int)0x40, (unsigned int)0x05);
}

int mp4ff_write_esds_video(mp4ff_t *file, mp4ff_esds_t *esds, int esid)
{
	return mp4ff_write_esds_common(file, esds, esid, (unsigned int)0x20, (unsigned int)0x04);
}

