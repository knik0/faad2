#!/usr/bin/python

import os, re, string, sys

#----------------------------------------------------------------------------------------------
#  Version 0:
#		SNDFILE*  sf_open_read  (const char *path, SF_INFO *sfinfo) ;
#		SNDFILE*  sf_open_write (const char *path, SF_INFO *sfinfo) ;
#  Version 1:
#		SNDFILE*  sf_open  (const char *path, int mode, SF_INFO *sfinfo) ;


def modify_sf_open (program_text):
	sf_open_re = re.compile ("(sf_open_([readwit]{4,5})\s*\(([^),]+),([^),]+)\))", re.MULTILINE)
	
	while 1:
		found = sf_open_re.search (program_text)
		if not found:
			break

		found = found.groups ()

		if len (found) != 4:
			print "***** Conversion may not be complete. *****"
			break
			
		print "    ", found [0]

		found = list (found)

		if found [1] == "read":
			found [1] = "SFM_READ"
		elif found [1] == "write":
			found [1] = "SFM_WRITE"
		else:
			found [1] = "SFM_" + found [1]
			
		replacement = "sf_open (%s, %s,%s) " % (found [2], found [1], found [3])
			
		print "         =>", replacement
			
		program_text = string.replace (program_text, found [0], replacement)
		print

	return program_text

#----------------------------------------------------------------------------------------------
#  Version 0:
#		size_t  sf_write_double  (SNDFILE *sndfile, double *ptr, size_t items, int normalize) ;
#		size_t  sf_writef_double (SNDFILE *sndfile, double *ptr, size_t frames, int normalize) ;
#
#  Version 1:
#        sf_count_t    sf_read_double   (SNDFILE *sndfile, double *ptr, sf_count_t items) ;
#        sf_count_t    sf_readf_double  (SNDFILE *sndfile, double *ptr, sf_count_t frames) ;
# 
# Notice that in version 1 there is no normailize parameter. Remove it.
	
def modify_sf_double (program_text):
	sf_double_re = re.compile ("(sf_([readwitf]{4,5})_double\s*\(([^),]+),([^),]+),([^),]+),([^),]+)\))", re.MULTILINE)
	
	while 1:
		found = sf_double_re.search (program_text)
		if not found:
			break

		found = found.groups ()
		if len (found) != 6:
			print "***** Conversion may not be complete. *****"
			break
			
		print "    ", found [0]
		replacement = "sf_%s_double (%s,%s,%s " % found [1:-1] + "/* FIXME_NORM_MODE " + found [-1] + " */)"
			
		print "         =>", replacement
			
		program_text = string.replace (program_text, found [0], replacement, 1)
		print

	return program_text

#----------------------------------------------------------------------------------------------


def modify_pcmbitwidth (program_text):
	sf_pcmbitwidth_re = re.compile ("(([_a-zA-Z0-9]+\s*[\.->]{1,2}\s*)(pcmbitwidth)\s+=\s+([_a-zA-Z0-9]+)\s+;)", re.MULTILINE)
	
	while 1:
		found = sf_pcmbitwidth_re.search (program_text)

		if not found:
			break

		found = found.groups ()

		print "    ", found [0]

		if len (found) != 4:
			print "***** Conversion may not be complete. *****"
			break

		replacement = "/* %sold_field_PCM_BIT_WIDTH = %s */" % (found [1], found [3])
		print "         =>", replacement
			
		program_text = string.replace (program_text, found [0], replacement, 1)
		print

	return program_text


#----------------------------------------------------------------------------------------------
# SF_FORMAT_PCM has been dropped and one of these should be used instead:
#      SF_FORMAT_PCM_S8
#      SF_FORMAT_PCM_U8
#      SF_FORMAT_PCM_16
#      SF_FORMAT_PCM_24
#      SF_FORMAT_PCM_32

def modify_sf_format_pcm (program_text):
	sf_format_pcm_re = re.compile ("(SF_FORMAT_PCM([^_]))")
	
	while 1:
		found = sf_format_pcm_re.search (program_text)

		if not found:
			break

		found = found.groups ()

		print "    ", found [0]
		
		if len (found) != 2:
			print "***** Conversion may not be complete. *****"
			break

		replacement = "SF_FORMAT_PCM_FIXME" + found [1]
		print "         =>", replacement
			
		program_text = string.replace (program_text, found [0], replacement, 1)
		print

	return program_text


#----------------------------------------------------------------------------------------------
# The sf_command interface has also changed.


def modify_sf_command (program_text):
	sf_command_re = re.compile ('(sf_command\s*\(([^),]+),([ a-z\"]+),([^),]+),([^)(,]+)\))', re.MULTILINE)
	
	while 1:
		found = sf_command_re.search (program_text)

		if not found:
			break

		found = found.groups ()

		print "    ", found [0]
		print "    ", found
		
		if len (found) != 5:
			print "***** Conversion may not be complete. *****"
			break
			
		found = list (found)
		found [2] = string.strip (found [2])
		found [3] = string.strip (found [3])
		
		if found [3] == '"on"':
			found [3] = " NULL"
			found [4] = " SF_TRUE"
		elif found [3] == '"off"':
			found [3] = " NULL"
			found [4] = " SF_FALSE"
			
		if found [2] == '"lib version"':
			replacement = "sf_command (%s, SFC_GET_LIB_VERSION,%s,%s)" % (found [1], found [3], found [4])
		elif found [2] == '"norm float"':
			replacement = "sf_command (%s, SFC_SET_NORM_FLOAT,%s,%s)" % (found [1], found [3], found [4])
		elif found [2] == '"norm double"':
			replacement = "sf_command (%s, SFC_SET_NORM_FLOAT,%s,%s)" % (found [1], found [3], found [4])
		elif found [2] == '"read text"':
			replacement = "/* sf_invalid_command (%s,%s,%s,%s) */" % (found [1], found [2], found [3], found [4])
		elif found [2] == '"write text"':
			replacement = "/* sf_invalid_command (%s,%s,%s,%s) */" % (found [1], found [2], found [3], found [4])
		else:
			replacement = "xxxxxxxxxxxxxxxxxxx"
			
		print "         =>", replacement
			
		program_text = string.replace (program_text, found [0], replacement, 1)
		print

	return program_text


#---------------------------------------------------------------------------------
# Crunch the file and do the conversions.

def convert_functions (filename, bak_filename):
	program_text = open (filename, "r").read ()
	os.rename (filename, bak_filename)
	
#	program_text = modify_sf_open (program_text)
#	program_text = modify_sf_double (program_text)
#	program_text = modify_pcmbitwidth (program_text)
#	program_text = modify_sf_format_pcm (program_text)
	program_text = modify_sf_command (program_text)

	open (filename, "w").write (program_text)
	return

#============================================================================
# Main program starts here.



print 

print """
    This conversion program *SHOULD* make source code which worked when 
    compiled against libsndfile-0.0.X compile and link against 
    libsndfile-1.0.X and then work as before. Unfortunately I cannot 
    make any guarantees. If it breaks, you get to keep both pieces.

    Original files will be renamed from <filename> to <filename>.orig<pid>
    where <pid> will be the process id of this conversion program when it
    does the conversion.
    """
	
if len (sys.argv) < 2:
	progname = re.sub (".*/", "", sys.argv [0])
	print "    Usage : \n\t%s <file>\n" % progname
	print "    Multiple files allowed, ie :\n\t%s *.c\n" % progname
	sys.exit (0)
	
	
for filename in sys.argv [1:]:
	if os.access (filename, os.R_OK | os.W_OK) != 1:
		print "Error: not able to access '%s'\n" % filename
		sys.exit (1) ;
	print "%s :" % filename
	convert_functions (filename, "%s.orig%d" % (filename, os.getpid ()))
	print
