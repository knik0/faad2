#include "mp4ff.h"


int mp4ff_atom_reset(mp4ff_atom_t *atom)
{
	atom->end = 0;
	atom->type[0] = atom->type[1] = atom->type[2] = atom->type[3] = 0;
	return 0;
}

int mp4ff_atom_read_header(mp4ff_t *file, mp4ff_atom_t *atom)
{
	char header[10];
	int result;

	atom->start = mp4ff_position(file);

	mp4ff_atom_reset(atom);

	if(!mp4ff_read_data(file, header, HEADER_LENGTH)) return 1;
	result = mp4ff_atom_read_type(header, atom->type);
	atom->size = mp4ff_atom_read_size(header);
	if (atom->size == 0) {
		atom->size = file->total_length - atom->start;
	}
	atom->end = atom->start + atom->size;

/* Skip placeholder atom */
	if(mp4ff_match_32(atom->type, "wide"))
	{
		atom->start = mp4ff_position(file);
		mp4ff_atom_reset(atom);
		if(!mp4ff_read_data(file, header, HEADER_LENGTH)) 
			return 1;
		result = mp4ff_atom_read_type(header, atom->type);
		atom->size -= 8;
		if(!atom->size)
		{
/* Wrapper ended.  Get new atom size */
			atom->size = mp4ff_atom_read_size(header);
			if (atom->size == 0) {
				atom->size = file->total_length - atom->start;
			}
		}
		atom->end = atom->start + atom->size;
	}
	else
/* Get extended size */
	if(atom->size == 1)
	{
		if(!mp4ff_read_data(file, header, HEADER_LENGTH)) return 1;
		atom->size = mp4ff_atom_read_size64(header);
	}


#ifdef DEBUG
	printf("Reading atom %.4s length %u\n", atom->type, atom->size);
#endif
	return result;
}

int mp4ff_atom_write_header(mp4ff_t *file, mp4ff_atom_t *atom, char *text)
{
	atom->start = mp4ff_position(file);
	mp4ff_write_int32(file, 0);
	mp4ff_write_char32(file, text);
}

int mp4ff_atom_write_footer(mp4ff_t *file, mp4ff_atom_t *atom)
{
	atom->end = mp4ff_position(file);
	mp4ff_set_position(file, atom->start);
	mp4ff_write_int32(file, atom->end - atom->start);
	mp4ff_set_position(file, atom->end);
}

int mp4ff_atom_is(mp4ff_atom_t *atom, char *type)
{
	if(atom->type[0] == type[0] &&
		atom->type[1] == type[1] &&
		atom->type[2] == type[2] &&
		atom->type[3] == type[3])
	return 1;
	else
	return 0;
}

long mp4ff_atom_read_size(char *data)
{
	unsigned long result;
	unsigned long a, b, c, d;
	
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a<<24) | (b<<16) | (c<<8) | d;
	if(result > 0 && result < HEADER_LENGTH) result = HEADER_LENGTH;
	return (long)result;
}

unsigned __int64 mp4ff_atom_read_size64(char *data)
{
	unsigned __int64 result = 0;
	int i;

	for (i = 0; i < 8; i++) {
		result |= data[i];
		if (i < 7) {
			result <<= 8;
		}
	}

	if(result < HEADER_LENGTH) 
		result = HEADER_LENGTH;
	return result;
}

int mp4ff_atom_read_type(char *data, char *type)
{
	type[0] = data[4];
	type[1] = data[5];
	type[2] = data[6];
	type[3] = data[7];

/*printf("%c%c%c%c ", type[0], type[1], type[2], type[3]); */
/* need this for mp4ff_check_sig */
	if(isalpha(type[0]) && isalpha(type[1]) && isalpha(type[2]) && isalpha(type[3]))
	return 0;
	else
	return 1;
}

int mp4ff_atom_skip(mp4ff_t *file, mp4ff_atom_t *atom)
{
	/* printf("skipping atom %.4s, size %u\n", atom->type, atom->size); */
	return mp4ff_set_position(file, atom->end);
}

