Name "FAAD Winamp AAC plugin"
OutFile in_faad.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the FAAD2 Winamp AAC plugin on your computer."
InstType Normal
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" "UninstallString"
DirShow show
DirText "The installer has detected the path to Winamp. If it is not correct, please change."

Section "FAAD2 Winamp AAC plugin"
SectionIn 1
SetOutPath $INSTDIR\Plugins
File plugins\winamp\Release\in_faad.dll
SectionEnd
