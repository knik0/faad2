#include <time.h>
#include "mp4ff.h"

/* Disk I/O */


int mp4ff_read_data(mp4ff_t *file, char *data, int size)
{
    int result = 1;

    if (file->stream->get_position() != file->file_position)
        file->stream->seek(file->file_position);
    result = file->stream->read(data, size);

    file->file_position += size;

    return result;
}

int mp4ff_write_data(mp4ff_t *file, char *data, int size)
{
    int result;

    if (file->stream->get_position() != file->file_position)
        file->stream->seek(file->file_position);
    result = file->stream->write(data, size);

    file->file_position += size;

    return result;
}

int mp4ff_test_position(mp4ff_t *file)
{
	if (mp4ff_position(file) < 0)
	{
		printf("mp4ff_test_position: 32 bit overflow\n");
		return 1;
	}
	else
	return 0;
}

int mp4ff_read_pascal(mp4ff_t *file, char *data)
{
	char len = mp4ff_read_char(file);
	mp4ff_read_data(file, data, len);
	data[len] = 0;
}

int mp4ff_write_pascal(mp4ff_t *file, char *data)
{
	char len = strlen(data);
	mp4ff_write_data(file, &len, 1);
	mp4ff_write_data(file, data, len);
}

float mp4ff_read_fixed32(mp4ff_t *file)
{
	unsigned long a, b, c, d;
	unsigned char data[4];

	mp4ff_read_data(file, data, 4);
/*	fread(data, 4, 1, file->stream); */
	a = data[0];
	b = data[1];
	c = data[2];
	d = data[3];
	
	a = (a << 8) + b;
	b = (c << 8) + d;

	return (float)a + (float)b / 65536;
}

int mp4ff_write_fixed32(mp4ff_t *file, float number)
{
	unsigned char data[4];
	int a, b;

	a = number;
	b = (number - a) * 65536;
	data[0] = a >> 8;
	data[1] = a & 0xff;
	data[2] = b >> 8;
	data[3] = b & 0xff;

	return mp4ff_write_data(file, data, 4);
}

int mp4ff_write_int64(mp4ff_t *file, uint64_t value)
{
	unsigned char data[8];
	int i;

	for (i = 7; i >= 0; i--) {
		data[i] = value & 0xff;
		value >>= 8;
	}

	return mp4ff_write_data(file, data, 8);
}

int mp4ff_write_int32(mp4ff_t *file, long value)
{
	unsigned char data[4];

	data[0] = (value & 0xff000000) >> 24;
	data[1] = (value & 0xff0000) >> 16;
	data[2] = (value & 0xff00) >> 8;
	data[3] = value & 0xff;

	return mp4ff_write_data(file, data, 4);
}

int mp4ff_write_char32(mp4ff_t *file, char *string)
{
	return mp4ff_write_data(file, string, 4);
}


float mp4ff_read_fixed16(mp4ff_t *file)
{
	unsigned char data[2];
	
	mp4ff_read_data(file, data, 2);
	return (float)data[0] + (float)data[1] / 256;
}

int mp4ff_write_fixed16(mp4ff_t *file, float number)
{
	unsigned char data[2];
	int a, b;

	a = number;
	b = (number - a) * 256;
	data[0] = a;
	data[1] = b;
	
	return mp4ff_write_data(file, data, 2);
}

uint64_t mp4ff_read_int64(mp4ff_t *file)
{
	unsigned char data[8];
	uint64_t result = 0;
	int i;

	mp4ff_read_data(file, data, 8);

	for (i = 0; i < 8; i++) {
		result |= ((uint64_t)data[i]) << ((7 - i) * 8);
	}

	return result;
}

long mp4ff_read_int32(mp4ff_t *file)
{
	unsigned long result;
	unsigned long a, b, c, d;
	char data[4];
	
	mp4ff_read_data(file, data, 4);
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];
	d = (unsigned char)data[3];

	result = (a<<24) | (b<<16) | (c<<8) | d;
	return (long)result;
}


long mp4ff_read_int24(mp4ff_t *file)
{
	unsigned long result;
	unsigned long a, b, c;
	char data[4];
	
	mp4ff_read_data(file, data, 3);
/*	fread(data, 3, 1, file->stream); */
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];
	c = (unsigned char)data[2];

	result = (a<<16) | (b<<8) | c;
	return (long)result;
}

int mp4ff_write_int24(mp4ff_t *file, long number)
{
	unsigned char data[3];
	data[0] = (number & 0xff0000) >> 16;
	data[1] = (number & 0xff00) >> 8;
	data[2] = (number & 0xff);
	
	return mp4ff_write_data(file, data, 3);
/*	return fwrite(data, 3, 1, file->stream); */
}

int mp4ff_read_int16(mp4ff_t *file)
{
	unsigned long result;
	unsigned long a, b;
	char data[2];
	
	mp4ff_read_data(file, data, 2);
/*	fread(data, 2, 1, file->stream); */
	a = (unsigned char)data[0];
	b = (unsigned char)data[1];

	result = (a<<8) | b;
	return (int)result;
}

int mp4ff_write_int16(mp4ff_t *file, int number)
{
	unsigned char data[2];
	data[0] = (number & 0xff00) >> 8;
	data[1] = (number & 0xff);
	
	return mp4ff_write_data(file, data, 2);
/*	return fwrite(data, 2, 1, file->stream); */
}

int mp4ff_read_char(mp4ff_t *file)
{
	char output;
	mp4ff_read_data(file, &output, 1);
	return output;
}

int mp4ff_write_char(mp4ff_t *file, char x)
{
	return mp4ff_write_data(file, &x, 1);
}

int mp4ff_read_char32(mp4ff_t *file, char *string)
{
	mp4ff_read_data(file, string, 4);
/*	fread(string, 4, 1, file->stream); */
}

long mp4ff_position(mp4ff_t *file) 
{ 
	return file->file_position; 
}

int mp4ff_set_position(mp4ff_t *file, long position) 
{
	file->file_position = position;
	return 0;
/*	fseek(file->stream, position, SEEK_SET);  */
}

int mp4ff_copy_char32(char *output, char *input)
{
	*output++ = *input++;
	*output++ = *input++;
	*output++ = *input++;
	*output = *input;
}


void mp4ff_print_chars(char *desc, char *input, int len)
{
	int i;

    printf("%s", desc);
    for(i = 0; i < len; i++)
        printf("%c", input[i]);
	printf("\n");
}

unsigned long mp4ff_current_time()
{
	time_t t = 0;
	//time(&t);
	return (t+(66*31536000)+1468800);
}

int mp4ff_match_32(char *input, char *output)
{
	if(input[0] == output[0] &&
		input[1] == output[1] &&
		input[2] == output[2] &&
		input[3] == output[3])
		return 1;
	else 
		return 0;
}

int mp4ff_read_mp4_descr_length(mp4ff_t *file)
{
	uint8_t b;
	uint8_t numBytes = 0;
	uint32_t length = 0;

	do {
		b = mp4ff_read_char(file);
		numBytes++;
		length = (length << 7) | (b & 0x7F);
	} while ((b & 0x80) && numBytes < 4);

	return length;
}

int mp4ff_write_mp4_descr_length(mp4ff_t *file, int length, unsigned char compact)
{
	uint8_t b;
	int8_t i;
	int8_t numBytes;

	if (compact) {
		if (length <= 0x7F) {
			numBytes = 1;
		} else if (length <= 0x3FFF) {
			numBytes = 2;
		} else if (length <= 0x1FFFFF) {
			numBytes = 3;
		} else {
			numBytes = 4;
		}
	} else {
		numBytes = 4;
	}

	for (i = numBytes-1; i >= 0; i--) {
		b = (length >> (i * 7)) & 0x7F;
		if (i != 0) {
			b |= 0x80;
		}
		mp4ff_write_char(file, b);
	}

	return numBytes; 
}

void mp4ff_atom_hexdump(mp4ff_t* file, mp4ff_atom_t* atom)
{
	int i;
	int oldPos;

	oldPos = mp4ff_position(file);
	mp4ff_set_position(file, atom->start);
	printf("atom hex dump:\n");
	for (i = 0; i < atom->size; i++) {
		printf("%02x ", (uint8_t)mp4ff_read_char(file));
		if ((i % 16) == 0 && i > 0) {
			printf("\n");
		}
	}
	printf("\n");
	mp4ff_set_position(file, oldPos);
}
