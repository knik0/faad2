Name "FAAD Winamp3 AAC plugin"
OutFile cnv_aacpcm.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the FAAD2 Winamp3 AAC plugin on your computer."
InstType Normal
DirText "Please select a location to install the FAAD2 Winamp3 AAC plugin (or use the default)."
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir "$PROGRAMFILES\Winamp3\wacs\"
InstallDirRegKey HKEY_CURRENT_USER SOFTWARE\Winamp3\FAAD2 ""

Section "FAAD2 Winamp3 AAC plugin"
SectionIn 1
SetOutPath $INSTDIR
File plugins\winamp3\Release\cnv_aacpcm.wac
SectionEnd
