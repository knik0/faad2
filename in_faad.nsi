Name "FAAD Winamp AAC plugin"
OutFile in_faad.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the FAAD2 Winamp AAC plugin on your computer."
InstType Normal
DirText "Please select a location to install the FAAD2 Winamp AAC plugin (or use the default)."
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir "$PROGRAMFILES\Winamp\Plugins\"
InstallDirRegKey HKEY_CURRENT_USER SOFTWARE\Winamp\FAAD2 ""

Section "FAAD2 Winamp AAC plugin"
SectionIn 1
SetOutPath $INSTDIR
File plugins\winamp\Release\in_faad.dll
SectionEnd
