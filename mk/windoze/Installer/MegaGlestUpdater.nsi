;--------------------------------
; General Attributes

!define APNAME MegaGlest
!define APVER 3.13-dev
!define APNAME_OLD Mega-Glest
!define APVER_OLD 3.13.0
!define APVER_UPDATE 3.13-dev

Name "${APNAME} ${APVER_UPDATE}"
SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 64
OutFile "${APNAME}-Updater-${APVER_UPDATE}_i386_win32.exe"
Icon "..\megaglest.ico"
UninstallIcon "..\megaglest.ico"
!define MUI_ICON "..\megaglest.ico"
!define MUI_UNICON "..\megaglest.ico"
InstallDir "$PROGRAMFILES\${APNAME}_${APVER}"
ShowInstDetails show
BGGradient 0xDF9437 0xffffff

; Request application privileges for Windows Vista
;RequestExecutionLevel none
RequestExecutionLevel none

PageEx license
       LicenseText "MegaGlest Game License"
       LicenseData "..\..\..\docs\gnu_gpl_3.0.txt"
PageExEnd

PageEx license
       LicenseText "MegaGlest Data License"
       LicenseData "..\..\..\data\glest_game\docs\cc-by-sa-3.0-unported.txt"
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
InstallDirRegKey HKLM "Software\${APNAME}_${APVER}" "Install_Dir"

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

#  FindWindow $0 "#32770" "" $HWNDPARENT
#  GetDlgItem $0 $0 1006
#  SetCtlColors $0 0xDF9437 0xDF9437

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

   IfFileExists $INSTDIR\megaglest.exe 0 +2
   StrCpy $R0 "$INSTDIR"
   StrCpy $R2 "?"
   IfFileExists $INSTDIR\megaglest.exe foundInst

   IfFileExists $EXEDIR\megaglest.exe 0 +2
   StrCpy $R0 "$EXEDIR"
   StrCpy $R2 "?"
   IfFileExists $EXEDIR\megaglest.exe foundInst doneInit

foundInst:
   StrCpy $INSTDIR "$R0"
   
MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${APNAME} v${APVER} installation found in [$R0]. $\n$\nClick `OK` to update \
  the previous installation or `Cancel` to exit." \
  IDOK uninstInit

  # change install folder to a version specific name to aovid over-writing
  # old one
  ;StrCpy $INSTDIR "$PROGRAMFILES\${APNAME}_${APVER}"
  Quit
  goto doneInit

notFoundInst:

MessageBox MB_OK|MB_ICONSTOP \
  "${APNAME} v${APVER} installation NOT found. $\n$\nCannot upgrade \
  this installation since the main installer was not previously used." \
  IDOK
  Quit
  goto doneInit

;Run the uninstaller
uninstInit:
  ClearErrors
  ;ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
  ;Exec $INSTDIR\uninst.exe ; instead of the ExecWait line
    
doneInit:

FunctionEnd


Function .onGUIEnd

  EBanner::stop

FunctionEnd

Function .onInstSuccess

    MessageBox MB_YESNO "${APNAME} v${APVER} installed successfully, \
    click Yes to launch the game$\nor 'No' to exit." IDNO noLaunch

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

  ; remove old Norsemen training_field upgrade
  #RMDir /r "$INSTDIR\techs\megapack\factions\norsemen\upgrades\training_field"

  ; Put file there
  File "..\..\..\data\glest_game\megaglest.exe"
  File "..\..\..\data\glest_game\megaglest_editor.exe"
  File "..\..\..\data\glest_game\megaglest_g3dviewer.exe"
  File "..\..\..\data\glest_game\7z.exe"
  File "..\..\..\data\glest_game\7z.dll"
  File "..\..\..\data\glest_game\glest.ini"
  File "..\..\..\data\glest_game\glestkeys.ini"
  File "..\..\..\data\glest_game\servers.ini"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\*.lng"
  #File /r /x .svn /x mydata "..\..\..\data\glest_game\tutorials"
  #File /r /x .svn /x mydata "..\..\..\data\glest_game\*.xml"

  AccessControl::GrantOnFile "$INSTDIR" "(BU)" "FullAccess"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${APNAME}\*.*"

  CreateDirectory "$SMPROGRAMS\${APNAME}"
  CreateDirectory "$APPDATA\megaglest"
  CreateShortCut "$SMPROGRAMS\${APNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME}.lnk" "$INSTDIR\megaglest.exe" "" "$INSTDIR\megaglest.exe" 0 "" "" "${APNAME}"

  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} Map Editor.lnk" "$INSTDIR\megaglest_editor.exe" "" "$INSTDIR\megaglest_editor.exe" 0 "" "" "${APNAME} MegaGlest Map Editor"
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} G3D Viewer.lnk" "$INSTDIR\megaglest_g3dviewer.exe" "" "$INSTDIR\megaglest_g3dviewer.exe" 0 "" "" "${APNAME} MegaGlest G3D Viewer"

  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} User Data.lnk" "$APPDATA\megaglest" "" "" 0 "" "" "This folder contains downloaded data (such as mods) and your personal ${APNAME} configuration"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}"
  DeleteRegKey HKLM SOFTWARE\${APNAME}

  ; Remove files and uninstaller
  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\megaglest.exe"
  Delete "$INSTDIR\megaglest_editor.exe"
  Delete "$INSTDIR\megaglest_g3dviewer.exe"
  Delete "$INSTDIR\megaglest.ico"
  Delete "$INSTDIR\glest.ini"
  Delete "$INSTDIR\glestkeys.ini"
  Delete "$INSTDIR\servers.ini"
  Delete "$INSTDIR\openal32.dll"
  Delete "$INSTDIR\xerces-c_3_0.dll"
  Delete "$INSTDIR\*.log"

  Delete "$INSTDIR\data\*.*"
  Delete "$INSTDIR\docs\*.*"
  Delete "$INSTDIR\maps\*.*"
  Delete "$INSTDIR\scenarios\*.*"
  Delete "$INSTDIR\screens\*.*"
  Delete "$INSTDIR\techs\*.*"
  Delete "$INSTDIR\tilesets\*.*"
  Delete "$INSTDIR\tutorials\*.*"

  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\docs"
  RMDir /r "$INSTDIR\maps"
  RMDir /r "$INSTDIR\scenarios"
  RMDir /r "$INSTDIR\screens"
  RMDir /r "$INSTDIR\techs"
  RMDir /r "$INSTDIR\tilesets"
  RMDir /r "$INSTDIR\tutorials"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\${APNAME}\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${APNAME}"
  RMDir /r "$INSTDIR"

SectionEnd

