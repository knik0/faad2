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


int mp4ff_ctts_init(mp4ff_ctts_t *ctts)
{
	ctts->version = 0;
	ctts->flags = 0;
	ctts->total_entries = 0;
	ctts->entries_allocated = 0;
}

int mp4ff_ctts_init_table(mp4ff_ctts_t *ctts)
{
	if (!ctts->entries_allocated) {
		ctts->entries_allocated = 1;
		ctts->table = (mp4ff_ctts_table_t*)
			malloc(sizeof(mp4ff_ctts_table_t) * ctts->entries_allocated);
		ctts->total_entries = 1;
	}
}

int mp4ff_ctts_init_common(mp4ff_t *file, mp4ff_ctts_t *ctts)
{
	mp4ff_ctts_table_t *table;
	mp4ff_ctts_init_table(ctts);
	table = &(ctts->table[0]);

	table->sample_count = 0;      /* need to set this when closing */
	table->sample_offset = 0;
}

int mp4ff_ctts_delete(mp4ff_ctts_t *ctts)
{
	if (ctts->total_entries) {
		free(ctts->table);
	}
	ctts->total_entries = 0;
}

int mp4ff_ctts_dump(mp4ff_ctts_t *ctts)
{
	int i;
	printf("     composition time to sample\n");
	printf("      version %d\n", ctts->version);
	printf("      flags %d\n", ctts->flags);
	printf("      total_entries %d\n", ctts->total_entries);
	for(i = 0; i < ctts->total_entries; i++) {
		printf("       count %ld offset %ld\n", 
			ctts->table[i].sample_count,
			ctts->table[i].sample_offset);
	}
}

int mp4ff_read_ctts(mp4ff_t *file, mp4ff_ctts_t *ctts)
{
	int i;
	ctts->version = mp4ff_read_char(file);
	ctts->flags = mp4ff_read_int24(file);
	ctts->total_entries = mp4ff_read_int32(file);

	ctts->table = (mp4ff_ctts_table_t*)
		malloc(sizeof(mp4ff_ctts_table_t) * ctts->total_entries);

	for (i = 0; i < ctts->total_entries; i++) {
		ctts->table[i].sample_count = mp4ff_read_int32(file);
		ctts->table[i].sample_offset = mp4ff_read_int32(file);
	}
}

int mp4ff_write_ctts(mp4ff_t *file, mp4ff_ctts_t *ctts)
{
	int i;
	mp4ff_atom_t atom;

	if (ctts->total_entries == 1 && ctts->table[0].sample_offset == 0) {
		return;
	}

	mp4ff_atom_write_header(file, &atom, "ctts");

	mp4ff_write_char(file, ctts->version);
	mp4ff_write_int24(file, ctts->flags);
	mp4ff_write_int32(file, ctts->total_entries);
	for(i = 0; i < ctts->total_entries; i++) {
		mp4ff_write_int32(file, ctts->table[i].sample_count);
		mp4ff_write_int32(file, ctts->table[i].sample_offset);
	}
	mp4ff_atom_write_footer(file, &atom);
}

int mp4ff_update_ctts(mp4ff_ctts_t *ctts, long sample_offset)
{
	if (sample_offset == ctts->table[ctts->total_entries-1].sample_offset) {
		ctts->table[ctts->total_entries-1].sample_count++;
	} else {
		/* need a new entry in the table */

		/* allocate more entries if necessary */
		if (ctts->total_entries >= ctts->entries_allocated) {
			ctts->entries_allocated *= 2;
			ctts->table = (mp4ff_ctts_table_t*)realloc(ctts->table,
				sizeof(mp4ff_ctts_table_t) * ctts->entries_allocated);
		}
	
		ctts->table[ctts->total_entries].sample_count = 1;
		ctts->table[ctts->total_entries].sample_offset = sample_offset;
		ctts->total_entries++;
	}
}
