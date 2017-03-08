;--------------------------------
; General Attributes

!define APNAME MegaGlest
!define APNAME_OLD Mega-Glest
!define APVER_OLD 3.13.0
!define APVER 3.13-dev

!ifdef NSIS_WIN32_MAKENSIS
!define NSISCONF_3 ";" ; NSIS 2 tries to parse some preprocessor instructions inside "!if 0" blocks!
!endif ;~ NSIS_WIN32_MAKENSIS

Name "${APNAME} ${APVER}"
SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 64
OutFile "${APNAME}-Installer-${APVER}_windows_XXbit.exe"
Icon "..\..\shared\megaglest.ico"
UninstallIcon "..\..\shared\megaglest.ico"
!define MUI_ICON "..\..\shared\megaglest.ico"
!define MUI_UNICON "..\..\shared\megaglest.ico"
InstallDir "$PROGRAMFILES\${APNAME}"
ShowInstDetails show
;BGGradient 0xDF9437 0xffffff

; Request application privileges for Windows Vista
RequestExecutionLevel none

PageEx license
       LicenseText "MegaGlest Game License"
       LicenseData "..\..\..\docs\gnu_gpl_3.0.txt"
PageExEnd

PageEx license
       LicenseText "MegaGlest Data License"
       LicenseData "..\..\..\data\glest_game\docs\cc-by-sa-3.0-unported.txt"
PageExEnd

;---- add local plugins dir
!addPluginDir "plugins"
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

Function .onInit
    InitPluginsDir
FunctionEnd

Function un.onInit
    InitPluginsDir
FunctionEnd

Function myGUIInit
  SetOutPath '$PLUGINSDIR'
  File megaglestinstallscreen.jpg

  FindWindow $0 '_Nb'
  #EBanner::show /NOUNLOAD /FIT=BOTH /HWND=$0 "$PLUGINSDIR\megaglestinstallscreen.jpg"
  #BgImage::SetBg /NOUNLOAD /FILLSCREEN "$PLUGINSDIR\megaglestinstallscreen.jpg"
  #BgImage::Redraw /NOUNLOAD

#  FindWindow $0 "#32770" "" $HWNDPARENT
#  GetDlgItem $0 $0 1006
#  SetCtlColors $0 0xDF9437 0xDF9437
FunctionEnd

Function un.myGUIInit
  SetOutPath '$PLUGINSDIR'
  File megaglestinstallscreen.jpg

  FindWindow $0 '_Nb'
  #EBanner::show /NOUNLOAD /FIT=BOTH /HWND=$0 "$PLUGINSDIR\megaglestinstallscreen.jpg"
FunctionEnd
  
Function MUIGUIInit

  Call myGUIInit

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

   IfFileExists $INSTDIR\megaglest.exe 0 +2
   StrCpy $R0 "$INSTDIR"
   StrCpy $R2 "?"
   IfFileExists $INSTDIR\megaglest.exe foundInst

   IfFileExists $EXEDIR\megaglest.exe 0 +2
   StrCpy $R0 "$EXEDIR"
   StrCpy $R2 "?"
   IfFileExists $EXEDIR\megaglest_exe foundInst doneInit

   IfFileExists $INSTDIR\megaglestx64.exe 0 +2
   StrCpy $R0 "$INSTDIR"
   StrCpy $R2 "?"
   IfFileExists $INSTDIR\megaglestx64.exe foundInst

foundInst:

  #MessageBox MB_OK|MB_ICONEXCLAMATION "Looking for mods in [$R0\\mydata\\]"

  MessageBox MB_YESNO|MB_ICONEXCLAMATION \
  "${APNAME} v$R2 is already installed in [$R0]. $\n$\nClick `Yes` to remove the \
  previous installation or `No` to over-write (not recommended) or install to a different location." \
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
  IfFileExists "$R0\glestuser.ini" 0 +2
  CreateDirectory "$APPDATA\megaglest\"
  Rename "$R0\glestuser.ini" "$APPDATA\megaglest\glestuser.ini"

  ClearErrors

  ExecWait '$R1 _?=$R0' ;Do not copy the uninstaller to a temp file
  Exec $R0\uninst.exe ; instead of the ExecWait line

doneInit:
  IfFileExists "$R0\mydata\" 0 +2
  CreateDirectory "$APPDATA\megaglest\"
  Rename "$R0\mydata\*.*" "$APPDATA\megaglest\"
  ClearErrors

  IfFileExists "$R0\glestuser.ini" 0 +2
  CreateDirectory "$APPDATA\megaglest\"
  Rename "$R0\glestuser.ini" "$APPDATA\megaglest\glestuser.ini"
  ClearErrors
  
FunctionEnd


Function .onGUIEnd

  #EBanner::stop

FunctionEnd

Function .onInstSuccess

    MessageBox MB_YESNO "Would you like to view our getting started page on megaglest.org?" IDNO noLaunchWebsite
    ExecShell open 'http://megaglest.org/get-started.html'
    
noLaunchWebsite:

MessageBox MB_YESNO "Would you like to view the README file? This is heavily recommended." IDNO noViewReadme
    ExecShell "open" "$INSTDIR\docs\README.txt"

noViewReadme:

    MessageBox MB_YESNO "${APNAME} v${APVER} installed successfully, \
    click Yes to launch the game now$\nor 'No' to exit." IDNO noLaunch

    SetOutPath $INSTDIR
    ${If} ${FileExists}  "$INSTDIR\megaglest.exe"
      Exec 'megaglest.exe'
    ${ElseIf} ${FileExists}  "$INSTDIR\megaglestx64.exe"
      Exec 'megaglestx64.exe'
    ${EndIf}

noLaunch:

  Delete "$PLUGINSDIR'\megaglestinstallscreen.jpg"

FunctionEnd

; The stuff to install
Section "${APNAME} (required)"

  SectionIn RO

  #MUI_PAGE_INSTFILES

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File /NONFATAL "..\..\..\data\glest_game\megaglest.exe"
  File /NONFATAL "..\..\..\data\glest_game\megaglest_editor.exe"
  File /NONFATAL "..\..\..\data\glest_game\megaglest_g3dviewer.exe"

  File /NONFATAL "..\megaglestx64.exe"
  File /NONFATAL "..\megaglest_editorx64.exe"
  File /NONFATAL "..\megaglest_g3dviewerx64.exe"

  File "..\7z.exe"
  File "..\7z.dll"
  File "..\..\shared\megaglest.ico"
  File "..\glest.ini"
  File "..\..\shared\glestkeys.ini"
  File "..\..\shared\servers.ini"
  File /NONFATAL "..\..\..\data\glest_game\openal32.dll"
  File /NONFATAL "..\openal64.dll"
  
  File "..\NetworkThrottleFix.reg"
  
  #File /NONFATAL "..\libvlccore.dll"
  #File /NONFATAL "..\libvlc.dll"
  #File /NONFATAL /r /x .gitignore /x .svn /x mydata "..\plugins"
  #File /NONFATAL /r /x .gitignore /x .svn /x mydata "..\lua"
  
  SetOutPath "$INSTDIR\blender\"
  #File /NONFATAL "..\..\..\data\glest_game\xml2g.exe"
  #File /NONFATAL "..\..\..\data\glest_game\g2xml.exe"

  #File /NONFATAL "..\xml2gx64.exe"
  #File /NONFATAL "..\g2xmlx64.exe"

  File /r /x .gitignore /x .svn /x mydata /x g2xml /x xml2g "..\..\..\source\tools\glexemel\*.py"
  SetOutPath $INSTDIR

  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\data"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\docs"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\maps"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\scenarios"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\techs"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\tilesets"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\data\glest_game\tutorials"

  SetOutPath "$INSTDIR\docs\"
  File /r /x .gitignore /x .svn /x mydata "..\..\..\docs\*.*"

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
  CreateDirectory $INSTDIR\blender

  !ifndef NSISCONF_3
    AccessControl::GrantOnFile "$INSTDIR" "(BU)" "FullAccess"
  !endif
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\${APNAME}"
  CreateDirectory "$APPDATA\megaglest"
  CreateShortCut "$SMPROGRAMS\${APNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} Main.lnk" "$INSTDIR" "" "" 0 "" "" "This folder is the ${APNAME} installation folder"
  CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} User Data.lnk" "$APPDATA\megaglest" "" "" 0 "" "" "This folder contains downloaded data (such as mods) and your personal ${APNAME} configuration"

  ${If} ${FileExists}  "$INSTDIR\megaglest.exe"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} G3D Viewer.lnk" "$INSTDIR\megaglest_g3dviewer.exe" "" "$INSTDIR\megaglest_g3dviewer.exe" 0 "" "" "${APNAME} MegaGlest G3D Viewer"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} Map Editor.lnk" "$INSTDIR\megaglest_editor.exe" "" "$INSTDIR\megaglest_editor.exe" 0 "" "" "${APNAME} MegaGlest Map Editor"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME}.lnk" "$INSTDIR\megaglest.exe" "" "$INSTDIR\megaglest.exe" 0 "" "" "${APNAME}"
  ${ElseIf} ${FileExists}  "$INSTDIR\megaglestx64.exe"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} G3D Viewer.lnk" "$INSTDIR\megaglest_g3dviewerx64.exe" "" "$INSTDIR\megaglest_g3dviewerx64.exe" 0 "" "" "${APNAME} MegaGlest G3D Viewer"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME} Map Editor.lnk" "$INSTDIR\megaglest_editorx64.exe" "" "$INSTDIR\megaglest_editorx64.exe" 0 "" "" "${APNAME} MegaGlest Map Editor"
    CreateShortCut "$SMPROGRAMS\${APNAME}\${APNAME}.lnk" "$INSTDIR\megaglestx64.exe" "" "$INSTDIR\megaglestx64.exe" 0 "" "" "${APNAME}"
  ${EndIf}

SectionEnd

;--------------------------------
RequestExecutionLevel admin
section "Tweaks"
  !ifndef NSISCONF_3
    AccessControl::GrantOnRegKey \
      HKLM "Software" "(BU)" "FullAccess"
      WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile" "NetworkThrottlingIndex" 0xffffffff
  !endif
sectionEnd
RequestExecutionLevel none

; Uninstaller

Section "Uninstall"

  Call un.myGUIInit
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APNAME}"
  DeleteRegKey HKLM SOFTWARE\${APNAME}

  ; Remove files and uninstaller
  Delete "$INSTDIR\uninstall.exe"

  Delete "$INSTDIR\megaglest.exe"
  Delete "$INSTDIR\megaglest_editor.exe"
  Delete "$INSTDIR\megaglest_g3dviewer.exe"

  Delete "$INSTDIR\megaglestx64.exe"
  Delete "$INSTDIR\megaglest_editorx64.exe"
  Delete "$INSTDIR\megaglest_g3dviewerx64.exe"

  Delete "$INSTDIR\megaglest.ico"
  Delete "$INSTDIR\glest.ini"
  Delete "$INSTDIR\glestkeys.ini"
  Delete "$INSTDIR\servers.ini"

  Delete "$INSTDIR\openal32.dll"
  Delete "$INSTDIR\openal64.dll"

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

  Delete "$PLUGINSDIR'\megaglestinstallscreen.jpg"

SectionEnd

