#include "mp4ff.h"

#define DEFAULT_INFO "Made with Quicktime for Linux"

int mp4ff_udta_init(mp4ff_udta_t *udta)
{
	udta->copyright = 0;
	udta->copyright_len = 0;
	udta->name = 0;
	udta->name_len = 0;

	udta->info = malloc(strlen(DEFAULT_INFO) + 1);
	udta->info_len = strlen(DEFAULT_INFO);
	sprintf(udta->info, DEFAULT_INFO);

	return 0;
}

int mp4ff_udta_delete(mp4ff_udta_t *udta)
{
	if(udta->copyright_len)
	{
		free(udta->copyright);
	}
	if(udta->name_len)
	{
		free(udta->name);
	}
	if(udta->info_len)
	{
		free(udta->info);
	}

	mp4ff_udta_init(udta);
	return 0;
}

int mp4ff_udta_dump(mp4ff_udta_t *udta)
{
	printf(" user data (udta)\n");
	if(udta->copyright_len) printf("  copyright -> %s\n", udta->copyright);
	if(udta->name_len) printf("  name -> %s\n", udta->name);
	if(udta->info_len) printf("  info -> %s\n", udta->info);
}

int mp4ff_read_udta(mp4ff_t *file, mp4ff_udta_t *udta, mp4ff_atom_t *udta_atom)
{
	mp4ff_atom_t leaf_atom;
	int result = 0;

	do
	{
		/* udta atoms can be terminated by a 4 byte zero */
		if (udta_atom->end - mp4ff_position(file) < HEADER_LENGTH) {
			unsigned char trash[HEADER_LENGTH];
			int remainder = udta_atom->end - mp4ff_position(file);
			mp4ff_read_data(file, trash, remainder);
			break;
		}

		mp4ff_atom_read_header(file, &leaf_atom);

#if 0
		if(mp4ff_atom_is(&leaf_atom, "©cpy"))
		{
			result += mp4ff_read_udta_string(file, &(udta->copyright), &(udta->copyright_len));
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "©nam"))
		{
			result += mp4ff_read_udta_string(file, &(udta->name), &(udta->name_len));
		}
		else
		if(mp4ff_atom_is(&leaf_atom, "©inf"))
		{
			result += mp4ff_read_udta_string(file, &(udta->info), &(udta->info_len));
		}
		else
#endif
		mp4ff_atom_skip(file, &leaf_atom);
	}while(mp4ff_position(file) < udta_atom->end);

	return result;
}

int mp4ff_write_udta(mp4ff_t *file, mp4ff_udta_t *udta)
{
	mp4ff_atom_t atom, subatom;

	/*
	 * Empty udta atom makes Darwin Streaming Server unhappy
	 * so avoid it
	 */
#if 1
    return;
#else
	if (udta->copyright_len == 0 && udta->hnti.rtp.string == NULL) {
        return;
    }

	mp4ff_atom_write_header(file, &atom, "udta");

	if(udta->copyright_len)
	{
		mp4ff_atom_write_header(file, &subatom, "©cpy");
		mp4ff_write_udta_string(file, udta->copyright, udta->copyright_len);
		mp4ff_atom_write_footer(file, &subatom);
	}

#if 0
	if(udta->name_len && !file->use_mp4)
	{
		mp4ff_atom_write_header(file, &subatom, "©nam");
		mp4ff_write_udta_string(file, udta->name, udta->name_len);
		mp4ff_atom_write_footer(file, &subatom);
	}

	if(udta->info_len && !file->use_mp4)
	{
		mp4ff_atom_write_header(file, &subatom, "©inf");
		mp4ff_write_udta_string(file, udta->info, udta->info_len);
		mp4ff_atom_write_footer(file, &subatom);
	}
#endif
	if (udta->hnti.rtp.string != NULL) {
		mp4ff_write_hnti(file, &(udta->hnti));
	}

	mp4ff_atom_write_footer(file, &atom);
#endif
}

int mp4ff_read_udta_string(mp4ff_t *file, char **string, int *size)
{
	int result;

	if(*size) free(*string);
	*size = mp4ff_read_int16(file);  /* Size of string */
	mp4ff_read_int16(file);  /* Discard language code */
	*string = malloc(*size + 1);
	result = mp4ff_read_data(file, *string, *size);
	(*string)[*size] = 0;
	return !result;
}

int mp4ff_write_udta_string(mp4ff_t *file, char *string, int size)
{
	int new_size = strlen(string);
	int result;

	mp4ff_write_int16(file, new_size);    /* String size */
	mp4ff_write_int16(file, 0);    /* Language code */
	result = mp4ff_write_data(file, string, new_size);
	return !result;
}

int mp4ff_set_udta_string(char **string, int *size, char *new_string)
{
	if(*size) free(*string);
	*size = strlen(new_string + 1);
	*string = malloc(*size + 1);
	strcpy(*string, new_string);
	return 0;
}
