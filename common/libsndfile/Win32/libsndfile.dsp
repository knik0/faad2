# Microsoft Developer Studio Project File - Name="libsndfile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsndfile - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsndfile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsndfile.mak" CFG="libsndfile - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsndfile - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsndfile - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsndfile - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libsndfile - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libsndfile - Win32 Release"
# Name "libsndfile - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\GSM610\add.c
# End Source File
# Begin Source File

SOURCE=..\src\aiff.c
# End Source File
# Begin Source File

SOURCE=..\src\alaw.c
# End Source File
# Begin Source File

SOURCE=..\src\au.c
# End Source File
# Begin Source File

SOURCE=..\src\au_g72x.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\code.c
# End Source File
# Begin Source File

SOURCE=..\src\command.c
# End Source File
# Begin Source File

SOURCE=..\src\common.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\decode.c
# End Source File
# Begin Source File

SOURCE=..\src\double64.c
# End Source File
# Begin Source File

SOURCE=..\src\dwvw.c
# End Source File
# Begin Source File

SOURCE=..\src\file_io.c
# End Source File
# Begin Source File

SOURCE=..\src\float32.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g721.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g723_16.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g723_24.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g723_40.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g72x.c
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g72x_test.c
# End Source File
# Begin Source File

SOURCE=..\src\gsm610.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm_create.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm_decode.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm_destroy.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm_encode.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm_option.c
# End Source File
# Begin Source File

SOURCE=..\src\ima_adpcm.c
# End Source File
# Begin Source File

SOURCE=..\src\ircam.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\long_term.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\lpc.c
# End Source File
# Begin Source File

SOURCE=..\src\ms_adpcm.c
# End Source File
# Begin Source File

SOURCE=..\src\nist.c
# End Source File
# Begin Source File

SOURCE=..\src\paf.c
# End Source File
# Begin Source File

SOURCE=..\src\pcm.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\preprocess.c
# End Source File
# Begin Source File

SOURCE=..\src\raw.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\rpe.c
# End Source File
# Begin Source File

SOURCE=..\src\rx2.c
# End Source File
# Begin Source File

SOURCE=..\src\samplitude.c
# End Source File
# Begin Source File

SOURCE=..\src\sd2.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\short_term.c
# End Source File
# Begin Source File

SOURCE=..\src\sndfile.c
# End Source File
# Begin Source File

SOURCE=..\src\svx.c
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\table.c
# End Source File
# Begin Source File

SOURCE=..\src\txw.c
# End Source File
# Begin Source File

SOURCE=..\src\ulaw.c
# End Source File
# Begin Source File

SOURCE=..\src\voc.c
# End Source File
# Begin Source File

SOURCE=..\src\w64.c
# End Source File
# Begin Source File

SOURCE=..\src\wav.c
# End Source File
# Begin Source File

SOURCE=..\src\wav_w64.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\au.h
# End Source File
# Begin Source File

SOURCE=..\src\common.h
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\config.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\src\float_cast.h
# End Source File
# Begin Source File

SOURCE=..\src\G72x\g72x.h
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\gsm.h
# End Source File
# Begin Source File

SOURCE=..\src\G72x\private.h
# End Source File
# Begin Source File

SOURCE=..\src\GSM610\private.h
# End Source File
# Begin Source File

SOURCE=..\src\sfendian.h
# End Source File
# Begin Source File

SOURCE=..\src\sndfile.h
# End Source File
# Begin Source File

SOURCE=..\src\wav_w64.h
# End Source File
# End Group
# End Target
# End Project
