#include "mp4ff.h"



int mp4ff_hdlr_init(mp4ff_hdlr_t *hdlr)
{
	hdlr->version = 0;
	hdlr->flags = 0;
	hdlr->component_type[0] = 'm';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	hdlr->component_manufacturer = 0;
	hdlr->component_flags = 0;
	hdlr->component_flag_mask = 0;
	strcpy(hdlr->component_name, "Linux Media Handler");
}

int mp4ff_hdlr_init_video(mp4ff_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	strcpy(hdlr->component_name, "Linux Video Media Handler");
}

int mp4ff_hdlr_init_audio(mp4ff_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 's';
	hdlr->component_subtype[1] = 'o';
	hdlr->component_subtype[2] = 'u';
	hdlr->component_subtype[3] = 'n';
	strcpy(hdlr->component_name, "Linux Sound Media Handler");
}

int mp4ff_hdlr_init_data(mp4ff_hdlr_t *hdlr)
{
	hdlr->component_type[0] = 'd';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'a';
	hdlr->component_subtype[1] = 'l';
	hdlr->component_subtype[2] = 'i';
	hdlr->component_subtype[3] = 's';
	strcpy(hdlr->component_name, "Linux Alias Data Handler");
}

int mp4ff_hdlr_delete(mp4ff_hdlr_t *hdlr)
{
}

int mp4ff_hdlr_dump(mp4ff_hdlr_t *hdlr)
{
	printf("   handler reference (hdlr)\n");
	printf("    version %d\n", hdlr->version);
	printf("    flags %d\n", hdlr->flags);
	printf("    component_type %c%c%c%c\n", hdlr->component_type[0], hdlr->component_type[1], hdlr->component_type[2], hdlr->component_type[3]);
	printf("    component_subtype %c%c%c%c\n", hdlr->component_subtype[0], hdlr->component_subtype[1], hdlr->component_subtype[2], hdlr->component_subtype[3]);
	printf("    component_name %s\n", hdlr->component_name);
}

int mp4ff_read_hdlr(mp4ff_t *file, mp4ff_hdlr_t *hdlr)
{
	hdlr->version = mp4ff_read_char(file);
	hdlr->flags = mp4ff_read_int24(file);
	mp4ff_read_char32(file, hdlr->component_type);
	mp4ff_read_char32(file, hdlr->component_subtype);
	hdlr->component_manufacturer = mp4ff_read_int32(file);
	hdlr->component_flags = mp4ff_read_int32(file);
	hdlr->component_flag_mask = mp4ff_read_int32(file);
    // TBD read null terminated string
}

int mp4ff_write_hdlr(mp4ff_t *file, mp4ff_hdlr_t *hdlr)
{
	int i;

	mp4ff_atom_t atom;
	mp4ff_atom_write_header(file, &atom, "hdlr");

	mp4ff_write_char(file, hdlr->version);
	mp4ff_write_int24(file, hdlr->flags);

	mp4ff_write_int32(file, 0x00000000);
	mp4ff_write_char32(file, hdlr->component_subtype);
	for (i = 0; i < 3; i++) {
		mp4ff_write_int32(file, 0x00000000);
	}
	mp4ff_write_data(file, hdlr->component_name, strlen(hdlr->component_name) + 1);

	mp4ff_atom_write_footer(file, &atom);
}
