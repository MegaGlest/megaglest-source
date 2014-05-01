@echo off
cd /d "%~dp0"


echo running the Qt compilers
set QTDIR=C:/Qt/5.2.1/msvc2010_opengl
set ConfigurationName=Release
set QTMOCPARAM=-DUNICODE -DWIN32 -DWIN64 -DQT_DLL -DQT_CORE_LIB -DQT_GUI_LIB -DQT_OPENGL_LIB -DQT_WIDGETS_LIB "-I.\GeneratedFiles" "-I." "-I%QTDIR%\include" "-I.\GeneratedFiles\%ConfigurationName%\." "-I%QTDIR%\include\QtCore" "-I%QTDIR%\include\QtGui" "-I%QTDIR%\include\QtOpenGL" "-I%QTDIR%\include\QtWidgets"

if NOT EXIST .\GeneratedFiles mkdir .\GeneratedFiles
if NOT EXIST .\GeneratedFiles\%ConfigurationName% mkdir .\GeneratedFiles\%ConfigurationName%

rem header files with non-standard Qt-Syntax
FOR %%f IN (mainWindow,mapManipulator,newMap,renderer,switchSurfaces,viewer) DO (
      echo Moc'ing \glest_map_editor\%%f.hpp...
      %QTDIR%\bin\moc.exe  "..\..\source\glest_map_editor\%%f.hpp" -o ".\GeneratedFiles\%ConfigurationName%\moc_%%f.cpp"  -DQT_NO_DEBUG -DNDEBUG  %QTMOCPARAM%
)

rem UI-files
FOR %%f IN (about,advanced,help,info,mainWindow,newMap,swapCopy,switchSurfaces) DO (
      echo Uic'ing \glest_map_editor\%%f.ui...
      rem %QTDIR%bin/moc.exe  "..\..\source\glest_map_editor\%%f.hpp" -o ".\GeneratedFiles\%ConfigurationName%\moc_%%f.cpp" %QTMOCPARAM%
      %QTDIR%\bin\uic.exe -o ".\GeneratedFiles\ui_%%f.h" "..\..\source\glest_map_editor\%%f.ui"
)

rem ressources
FOR %%f IN (icons) DO (
      echo Rcc'ing \glest_map_editor\%%f.qrc...
      rem %QTDIR%bin/moc.exe  "..\..\source\glest_map_editor\%%f.hpp" -o ".\GeneratedFiles\%ConfigurationName%\moc_%%f.cpp" %QTMOCPARAM%
      %QTDIR%\bin\rcc.exe -name "%%f" -no-compress "..\..\source\glest_map_editor\%%f.qrc" -o .\GeneratedFiles\qrc_%%f.cpp
)