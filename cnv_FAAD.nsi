Name "FAAD Winamp3 AAC plugin"
OutFile cnv_FAAD.exe
CRCCheck on
LicenseText "You must read the following license before installing."
LicenseData COPYING
ComponentText "This will install the FAAD2 Winamp3 AAC plugin on your computer."
InstType Normal
AutoCloseWindow true
SetOverwrite on
SetDateSave on

InstallDir $PROGRAMFILES\Winamp3
InstallDirRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3" "UninstallString"
DirShow show
DirText "The installer has detected the path to Winamp. If it is not correct, please change."

Section "FAAD2 Winamp3 AAC plugin"
SectionIn 1
SetOutPath $INSTDIR\Wacs
File plugins\winamp3\Release\cnv_FAAD.wac
SetOutPath $INSTDIR\Wacs\xml\FAAD
File plugins\winamp3\FAAD_config.xml
SectionEnd
