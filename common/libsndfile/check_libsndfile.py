#!/usr/bin/env python 

import commands, os, string, sys 

# This program tests libsndfile against a user provided list of audio files. 
# The list is provided as a text file. 
# 

_EXE_NAME = 'examples/sndfile_info'


def dump_status_output_exit (status, output, msg = None): 
	if msg: 
		print msg 
	print "Status :", status 
	print output 
	sys.exit (0) 

def sfinfo_check_ok (filename): 
	cmd = '%s %s' % (_EXE_NAME, filename) 
	(status, output) = commands.getstatusoutput (cmd) 
	if status: 
		dump_status_output_exit (status, output, "Bad status. Dumping") 
	if string.find (output, "should") > 0: 
		dump_status_output_exit (status, output, "Found `should'. Dumping") 
	if string.find (output, "*") > 0: 
		dump_status_output_exit (status, output, "Found `*'. Dumping") 
	return 

def sfinfo_check_not_crash (filename): 
	print filename 
	 
_USAGE = """ 
This is the usage message. 

""" 

if len (sys.argv) != 2: 
	print _USAGE 
	sys.exit (0) 


if not os.path.isfile (_EXE_NAME): 
	print "Could not find required program :", _EXE_NAME 
	sys.exit (0) 

list_file = open (sys.argv [1]) 

while 1: 
	line = list_file.readline () 
	if not line: 
		break  
	line = string.strip (line)
	if len (line) < 1:
		continue 
	if line [0] == '#':
		continue 
	print line
	if os.path.isfile (line): 
		sfinfo_check_ok (line) 
	else: 
		print "Bad file name : ", line 
		sys.exit (0) 

list_file.close () 

print "Finished. No errors found." 
