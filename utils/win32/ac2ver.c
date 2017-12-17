/*
** AC2VER - extract version number from AC_INIT macro (configure.ac)
** Copyright (C) 2017 LoRd_MuldeR <mulder2@GMX.de>
**  
** This software is released under the CC0 1.0 Universal [CC0 1.0] licence!
** https://creativecommons.org/publicdomain/zero/1.0/legalcode
**/

#define BUFF_SIZE 4096
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int clean_string(char *const str)
{
	size_t i = 0, j = 0;
	while(str[i])
	{
		if(isalnum(str[i]) || strchr("._-", str[i]))
		{
			if(i != j)
			{
				str[j] = str[i];
			}
			++j;
		}
		++i;
	}
	str[j] = '\0';
	return !!j;
}

static int parse_version(FILE *const input, const char *const format, char *const version)
{
	char buffer[BUFF_SIZE];

	while(!(feof(input) || ferror(input)))
	{
		const char *line = fgets(buffer, BUFF_SIZE, input);
		if(line)
		{
			while((*line) && (isspace(*line)))
			{
				++line; /*skip space*/
			}
			if(sscanf(line, format, version) == 1)
			{
				if(clean_string(version))
				{
					return 1; /*found!*/
				}
			}
		}
	}

	version[0] = '\0';
	return 0;
}

static int get_version(const wchar_t *const lib_name, const wchar_t *const file_name)
{
	char format[128], version[BUFF_SIZE];

	FILE *const input = _wfopen(file_name, L"r");
	if(!input)
	{
		fprintf(stderr, "Error: Failed to open input file!\n%S\n\n", file_name);
		return 0;
	}

	_snprintf(format, 128, "AC_INIT ( %S , %%s", lib_name);
	//printf("<%s>\n", format);

	if(parse_version(input, format, version))
	{
		printf("#define PACKAGE_VERSION \"%s\"\n", version);
		fclose(input);
		return 1;
	}
	else
	{
		fprintf(stderr, "Error: Version string could not be found!\n\n");
		fclose(input);
		return 0;
	}
}

int wmain(int argc, wchar_t* argv[])
{
	if((argc != 3) || (!argv[1][0]) || (!argv[2][0]))
	{
		wchar_t file_name[_MAX_FNAME], file_ext[_MAX_EXT];
		_wsplitpath(argv[0], NULL, NULL, file_name, file_ext);
		fprintf(stderr, "AC2VER [%s]\n\n", __DATE__);
		fprintf(stderr, "Usage: %S%S <lib_name> <path/to/configure.ac>\n\n", file_name, file_ext);
		return EXIT_FAILURE;
	}

	return get_version(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
