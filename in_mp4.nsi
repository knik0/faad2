Name "AudioCoding.com MP4 Winamp plugin"
OutFile in_mp4.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the AudioCoding.com MP4 Winamp plugin on your computer."
InstType Normal
DirText "Please select a location to install the AudioCoding.com MP4 Winamp plugin (or use the default)."
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir "$PROGRAMFILES\Winamp\Plugins\"
InstallDirRegKey HKEY_CURRENT_USER SOFTWARE\Winamp\IN_MP4 ""

Section "AudioCoding.com MP4 Winamp plugin"
SectionIn 1
SetOutPath $INSTDIR
File plugins\in_mp4\Release\in_mp4.dll
SectionEnd
