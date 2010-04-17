;--------------------------------
; General Attributes

!define APNAME Mega-Glest
!define APVER 3.3.4

Name "${APNAME} ${APVER}"
SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 64
OutFile "${APNAME}-Installer-${APVER}_i386_win32.exe"
Icon "..\glest.ico"
UninstallIcon "..\glest.ico"
!define MUI_ICON "..\glest.ico"
!define MUI_UNICON "..\glest.ico"
InstallDir "$PROGRAMFILES\${APNAME}_${APVER}"
ShowInstDetails show
BGGradient 0xDF9437 0xffffff

; Request application privileges for Windows Vista
RequestExecutionLevel none

PageEx license
       LicenseText "Megaglest License"
       LicenseData "..\..\..\data\glest_game\docs\license.txt"
PageExEnd

PageEx license
       LicenseText "Megaglest README"
       LicenseData "..\..\..\data\glest_game\docs\readme.txt"
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

   ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}" "UninstallString"
   StrCmp $R0 "" doneInit

MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${APNAME} v${APVER} is already installed. $\n$\nClick `OK` to remove the \
  previous installation or `Cancel` to install to a different location." \
  IDOK uninstInit

  # change install folder to a version specific name to aovid over-writing
  # old one
  StrCpy $INSTDIR "$PROGRAMFILES\${APNAME}_${APVER}"
  goto doneInit

;Run the uninstaller
uninstInit:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file

  Exec $INSTDIR\uninst.exe ; instead of the ExecWait line
    
doneInit:

FunctionEnd


Function .onGUIEnd

  EBanner::stop

FunctionEnd

Function .onInstSuccess

    MessageBox MB_OK "${APNAME} v${APVER} installed successfully, click OK to launch game."

    SetOutPath $INSTDIR
    Exec 'glest_game.exe'

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
  File "..\..\..\data\glest_game\configuration.xml"
  File "..\..\..\data\glest_game\glest.ico"
  File "..\..\..\data\glest_game\glest.ini"
  File "..\..\..\data\glest_game\servers.ini"
#  File "..\..\..\data\glest_game\dsound.dll"
  File "..\..\..\data\glest_game\openal32.dll"
  File "..\..\..\data\glest_game\xerces-c_3_0.dll"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\data"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\docs"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\maps"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\scenarios"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\techs"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\tilesets"
  File /r /x .svn /x mydata "..\..\..\data\glest_game\tutorials"
#  File /r /x .svn "..\..\..\data\glest_game\screens"

  ; Write the installation path into the registry
  WriteRegStr HKLM Software\${APNAME}_${APVER} "Install_Dir" "$INSTDIR"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}" "DisplayName" "${APNAME} v${APVER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  CreateDirectory $INSTDIR\data
  CreateDirectory $INSTDIR\docs
  CreateDirectory $INSTDIR\maps
  CreateDirectory $INSTDIR\scenarios
  CreateDirectory $INSTDIR\techs
  CreateDirectory $INSTDIR\tilesets
  CreateDirectory $INSTDIR\tutorials
  CreateDirectory $INSTDIR\screens
  
  AccessControl::GrantOnFile "$INSTDIR" "(BU)" "FullAccess"

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\${APNAME}_${APVER}"
  CreateShortCut "$SMPROGRAMS\${APNAME}_${APVER}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}_${APVER}\${APNAME}_${APVER}_game.lnk" "$INSTDIR\glest_game.exe" "" "$INSTDIR\glest_game.exe" 0
  
  CreateShortCut "$SMPROGRAMS\${APNAME}_${APVER}\${APNAME}_${APVER}_config.lnk" "$INSTDIR\glest_configurator.exe" "" "$INSTDIR\glest_configurator.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}_${APVER}\${APNAME}_${APVER}_editor.lnk" "$INSTDIR\glest_editor.exe" "" "$INSTDIR\glest_editor.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}_${APVER}\${APNAME}_${APVER}_viewer.lnk" "$INSTDIR\g3d_viewer.exe" "" "$INSTDIR\g3d_viewer.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}_${APVER}"
  DeleteRegKey HKLM SOFTWARE\${APNAME}_${APVER}

  ; Remove files and uninstaller
  Delete $INSTDIR\uninstall.exe

  Delete $INSTDIR\glest_game.exe
  Delete $INSTDIR\glest_editor.exe
  Delete $INSTDIR\glest_configurator.exe
  Delete $INSTDIR\g3d_viewer.exe
  Delete $INSTDIR\configuration.xml
  Delete $INSTDIR\glest.ico
  Delete $INSTDIR\glest.ini
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
  Delete "$SMPROGRAMS\${APNAME}_${APVER}\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\${APNAME}_${APVER}"
  RMDir "$INSTDIR"

SectionEnd

