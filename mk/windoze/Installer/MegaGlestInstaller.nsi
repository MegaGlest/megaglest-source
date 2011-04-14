;--------------------------------
; General Attributes

!define APNAME MegaGlest
!define APNAME_OLD Mega-Glest
!define APVER_OLD 3.4.0
!define APVER 3.5.0-beta2

Name "${APNAME} ${APVER}"
SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 64
OutFile "${APNAME}-Installer-${APVER}_i386_win32.exe"
Icon "..\megaglest.ico"
UninstallIcon "..\megaglest.ico"
!define MUI_ICON "..\megaglest.ico"
!define MUI_UNICON "..\megaglest.ico"
InstallDir "$PROGRAMFILES\${APNAME}"
ShowInstDetails show
BGGradient 0xDF9437 0xffffff

; Request application privileges for Windows Vista
RequestExecutionLevel none

PageEx license
       LicenseText "Megaglest License"
       LicenseData "..\..\..\data\glest_game\docs\LICENSE"
PageExEnd

PageEx license
       LicenseText "Megaglest README"
       LicenseData "..\..\..\data\glest_game\docs\README"
PageExEnd

;--------------------------------
; Images not included!
; Use your own animated GIFs please
;--------------------------------

;--------------------------------
;Interface Settings

!include "MUI.nsh"
!define MUI_CUSTOMFUNCTION_GUIINIT MUIGUIInit
!insertmacro MUI_PAGE_WELCOME
#!insertmacro MUI_PAGE_DIRECTORY
#!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; Registry key to check for directory (so if you install again, it will
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${APNAME}" "Install_Dir"

; Pages

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Function MUIGUIInit

  SetOutPath '$PLUGINSDIR'
  File megaglestinstallscreen.jpg

  FindWindow $0 '_Nb'
  EBanner::show /NOUNLOAD /FIT=BOTH /HWND=$0 "$PLUGINSDIR\megaglestinstallscreen.jpg"
  #BgImage::SetBg /NOUNLOAD /FILLSCREEN "$PLUGINSDIR\megaglestinstallscreen.jpg"
  #BgImage::Redraw /NOUNLOAD

#  FindWindow $0 "#32770" "" $HWNDPARENT
#  GetDlgItem $0 $0 1006
#  SetCtlColors $0 0xDF9437 0xDF9437

# look for known older versions

   StrCpy $R2 ${APVER}
   
   ReadRegStr $R0 HKLM Software\${APNAME} "Install_Dir"
   StrCmp $R0 "" +2 0
   ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}" "UninstallString"
   ReadRegStr $R2 HKLM Software\${APNAME} "Version"
   StrCmp $R0 "" 0 foundInst

   ReadRegStr $R0 HKLM Software\${APNAME_OLD}_${APVER_OLD} "Install_Dir"
   StrCmp $R0 "" +2 0
   ReadRegStr $R1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME_OLD}_${APVER_OLD}" "UninstallString"
   StrCpy $R2 ${APVER_OLD}
   StrCmp $R0 "" 0 foundInst

   IfFileExists $INSTDIR\glest_game.exe 0 +2
   StrCpy $R0 "$INSTDIR"
   StrCpy $R2 "?"
   IfFileExists $INSTDIR\glest_game.exe foundInst

   IfFileExists $EXEDIR\glest_game.exe 0 +2
   StrCpy $R0 "$EXEDIR"
   StrCpy $R2 "?"
   IfFileExists $EXEDIR\glest_game.exe foundInst doneInit

foundInst:

  #MessageBox MB_OK|MB_ICONEXCLAMATION "Looking for mods in [$R0\\mydata\\]"

  MessageBox MB_YESNO|MB_ICONEXCLAMATION \
  "${APNAME} v$R2 is already installed in [$R0]. $\n$\nClick `Yes` to remove the \
  previous installation (including all your MODS) or `No` to over-write or install to a different location." \
  IDYES uninstInit

  # change install folder to a version specific name to avoid over-writing
  # old one
  StrCpy $INSTDIR "$R0"
  ClearErrors

  goto doneInit

;Run the uninstaller
uninstInit:
  ClearErrors
  IfFileExists "$R0\mydata\" 0 +2
  CreateDirectory "$APPDATA\megaglest\"
  Rename "$R0\mydata\*.*" "$APPDATA\megaglest\"
  ClearErrors
  
  ExecWait '$R1 _?=$R0' ;Do not copy the uninstaller to a temp file
  Exec $R0\uninst.exe ; instead of the ExecWait line

doneInit:
  IfFileExists "$R0\mydata\" 0 +2
  CreateDirectory "$APPDATA\megaglest\"
  Rename "$R0\mydata\*.*" "$APPDATA\megaglest\"
  ClearErrors

FunctionEnd


Function .onGUIEnd

  EBanner::stop

FunctionEnd

Function .onInstSuccess

    MessageBox MB_YESNO "${APNAME} v${APVER} installed successfully, \
    click Yes to launch the game now$\nor 'No' to exit." IDNO noLaunch

    SetOutPath $INSTDIR
    Exec 'glest_game.exe'

noLaunch:

FunctionEnd

; The stuff to install
Section "${APNAME} (required)"

  SectionIn RO

  #MUI_PAGE_INSTFILES

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\..\data\glest_game\glest_game.exe"
  File "..\..\..\data\glest_game\glest_editor.exe"
  File "..\..\..\data\glest_game\glest_configurator.exe"
  File "..\..\..\data\glest_game\g3d_viewer.exe"
  File "..\..\..\data\glest_game\7z.exe"
  File "..\..\..\data\glest_game\configuration.xml"
  File "..\..\..\data\glest_game\megaglest.ico"
  File "..\..\..\data\glest_game\glest.ini"
  File "..\..\..\data\glest_game\glestkeys.ini"
  File "..\..\..\data\glest_game\servers.ini"
#  File "..\..\..\data\glest_game\dsound.dll"
  File "..\..\..\data\glest_game\openal32.dll"
  File "..\..\..\data\glest_game\xerces-c_3_0.dll"
  
  SetOutPath "$INSTDIR\blender\"
  File "..\..\..\data\glest_game\xml2g.exe"
  File "..\..\..\data\glest_game\g2xml.exe"
  File "..\..\..\source\tools\glexemel\g3d_support.py"
  File "..\..\..\source\tools\glexemel\g3d_xml_exporter.py"
  File "..\..\..\source\tools\glexemel\g3d.dtd"
  File "..\..\..\source\tools\glexemel\g3d_logo.png"
  SetOutPath $INSTDIR

  File /r /x .svn /x mydata "..\..\..\data\glest_game\data"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\docs"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\maps"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\scenarios"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\techs"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\tilesets"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\tutorials"
#  File /r /x .svn "..\..\..\data\glest_game\screens"
  SetOutPath "$INSTDIR\data\core\misc_textures\"
  File /r /x .svn /x mydata "..\..\..\source\masterserver\flags"
  SetOutPath $INSTDIR

  ; Write the installation path into the registry
  WriteRegStr HKLM Software\${APNAME} "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM Software\${APNAME} "Version" "${APVER}"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}" "DisplayName" "${APNAME} v${APVER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  CreateDirectory $INSTDIR\data
  CreateDirectory $INSTDIR\docs
  CreateDirectory $INSTDIR\maps
  CreateDirectory $INSTDIR\scenarios
  CreateDirectory $INSTDIR\techs
  CreateDirectory $INSTDIR\tilesets
  CreateDirectory $INSTDIR\tutorials
  CreateDirectory $INSTDIR\screens
  CreateDirectory $INSTDIR\blender

  AccessControl::GrantOnFile "$INSTDIR" "(BU)" "FullAccess"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\${APNAME}"
  CreateShortCut "$SMPROGRAMS\${APNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME}.lnk" "$INSTDIR\glest_game.exe" "" "$INSTDIR\glest_game.exe" 0 "" "" "${APNAME}"

;  CreateShortCut "$SMPROGRAMS\${APNAME} ${APVER}\${APNAME} Configurator.lnk" "$INSTDIR\glest_configurator.exe" "" "$INSTDIR\glest_configurator.exe" 0 "" "" "${APNAME} Config Editor"
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} Map Editor.lnk" "$INSTDIR\glest_editor.exe" "" "$INSTDIR\glest_editor.exe" 0 "" "" "${APNAME} Map Editor"
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} G3D Viewer.lnk" "$INSTDIR\g3d_viewer.exe" "" "$INSTDIR\g3d_viewer.exe" 0 "" "" "${APNAME} G3D Viewer"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}"
  DeleteRegKey HKLM SOFTWARE\${APNAME}

  ; Remove files and uninstaller
  Delete $INSTDIR\uninstall.exe

  Delete $INSTDIR\glest_game.exe
  Delete $INSTDIR\glest_editor.exe
  Delete $INSTDIR\glest_configurator.exe
  Delete $INSTDIR\g3d_viewer.exe
  Delete $INSTDIR\configuration.xml
  Delete $INSTDIR\megaglest.ico
  Delete $INSTDIR\glest.ini
  Delete $INSTDIR\glestkeys.ini
  Delete $INSTDIR\servers.ini
 # Delete $INSTDIR\dsound.dll
  Delete $INSTDIR\openal32.dll
  Delete $INSTDIR\xerces-c_3_0.dll
  Delete $INSTDIR\*.log

  Delete $INSTDIR\data\*.*
  Delete $INSTDIR\docs\*.*
  Delete $INSTDIR\maps\*.*
  Delete $INSTDIR\scenarios\*.*
  Delete $INSTDIR\screens\*.*
  Delete $INSTDIR\techs\*.*
  Delete $INSTDIR\tilesets\*.*
  Delete $INSTDIR\tutorials\*.*

  RMDir /r $INSTDIR\data
  RMDir /r $INSTDIR\docs
  RMDir /r $INSTDIR\maps
  RMDir /r $INSTDIR\scenarios
  RMDir /r $INSTDIR\screens
  RMDir /r $INSTDIR\techs
  RMDir /r $INSTDIR\tilesets
  RMDir /r $INSTDIR\tutorials

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${APNAME}\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${APNAME}"
  RMDir "$INSTDIR"

SectionEnd

