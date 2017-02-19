@echo off

rem change to the directory of this batch file
SET VCVARS_PLATFORM=

ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 ["%1"] p2 ["%2"]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binary runtime tools...
if NOT EXIST .\7z.exe call cscript getTools.vbs
if NOT EXIST .\7z.dll call cscript getTools.vbs
if NOT EXIST .\tar.exe call cscript getTools.vbs
if NOT EXIST .\wget.exe call cscript getTools.vbs

set depfolder=windows_deps_2015
set depfile=windows_deps_2015_x86.7z 

dir ..\..\source\
if NOT EXIST ..\..\source\%depfolder%\NUL echo folder not found [%depfolder%]
if NOT EXIST ..\..\source\%depfolder%\NUL goto checkDepIntegrity
goto processBuildStageA

:getDepFile
ECHO Retrieving windows dependency archive...
call .\wget.exe -c -O ..\..\source\%depfile% http://softcoder.megaglest.org/msvc/2015/%depfile%
call .\7z.exe x -r -o..\..\source\ ..\..\source\%depfile%
goto processBuildStageA

:checkDepIntegrity
ECHO Looking for windows dependency archive, please wait... (testing existing archive)...
call .\7z.exe t ..\..\source\%depfile% >nul
rem call .\7z.exe t ..\..\source\%depfile%
set testdeperr=%ERRORLEVEL%
ECHO Result of windows dependency archive [%testdeperr%]
rem pause

if NOT "%testdeperr%" == "0" goto getDepFile
if NOT EXIST ..\..\source\%depfolder%\NUL echo Extracting archive [%depfile%]
if NOT EXIST ..\..\source\%depfolder%\NUL call .\7z.exe x -r -o..\..\source\ ..\..\source\%depfile%
goto processBuildStageA

:processBuildStageA
call CopyWindowsRuntimeDlls_2015.bat nopause

rem setup the Visual Studio 2015 environment
ECHO --------------------------------
ECHO Setting up Visual Studio 2015 environment vars...
REM Ensure ultifds HP doesn't mess the build up
SET Platform=
if "%DevEnvDir%." == "." goto SETVCVARS
GOTO GITSECTION

:SETVCVARS

IF EXIST "%VS140COMNTOOLS%..\..\"                             GOTO VC_Common_15
IF EXIST "\Program Files\Microsoft Visual Studio 14.0\"       GOTO VC_32_15
IF EXIST "\Program Files (x86)\Microsoft Visual Studio 14.0\" GOTO VC_64_15
goto GITSECTION

:VC_Common_15
call "%VS140COMNTOOLS%..\..\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:VC_32_15
ECHO 32 bit Windows detected...
call "\Program Files\Microsoft Visual Studio 14.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:VC_64_15
ECHO 64 bit Windows detected...
call "\Program Files (x86)\Microsoft Visual Studio 14.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto GITSECTION

:GITSECTION
rem Update from GIT to latest rev
ECHO --------------------------------
Echo Updating Code from GIT to latest Revision...
cd ..\..\
set GIT_NORM_BRANCH=.
for /f "delims=" %%a in ('git branch ^| findstr /rc:"^\*[^(]*([^) d]*[ ]*detached"') do @set GIT_NORM_BRANCH=%%a
if "%GIT_NORM_BRANCH%" == "." git pull
cd data\glest_game
set GIT_NORM_BRANCH=.
for /f "delims=" %%a in ('git branch ^| findstr /rc:"^\*[^(]*([^) d]*[ ]*detached"') do @set GIT_NORM_BRANCH=%%a
if "%GIT_NORM_BRANCH%" == "." git pull
cd ..\..\
git submodule update

set GITVERSION_SHA1=.
set GITVERSION_REV=.
set GET_GIT_SHA1="git log -1 --format=%%h --abbrev=7"
for /f "delims=" %%a in ('%GET_GIT_SHA1%') do @set GITVERSION_SHA1=%%a
for /f "delims=" %%a in ('git rev-list HEAD --count') do @set GITVERSION_REV=%%a
ECHO Will build using GIT Revision: [%GITVERSION_REV%.%GITVERSION_SHA1%]
cd mk\windoze
rem pause

ECHO --------------------------------
Echo Touching the build date/time file so we get proper build stamp
rem touch ..\..\source\glest_game\facilities\game_util.cpp
copy /b ..\..\source\glest_game\facilities\game_util.cpp +,,

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building MegaGlest using Visual Studio 2015...

set CL=/MP
rem set INCLUDE=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Include;%INCLUDE%
rem set PATH=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Bin;%PATH%
rem set LIB=%ProgramFiles(x86)%\Microsoft SDKs\Windows\7.1A\Lib;%LIB%
rem set CL=/D_USING_V110_SDK71_;%CL%
rem set CL=/D_USING_V110_SDK71_;%CL%

rem This is needed for nmake-based projects like Qt or OpenSSL.
rem MSBuild (and thus CMake) can simply use v110_xp Platform Toolset.
rem set INCLUDE=%PROGRAM_FILES_X86%\Microsoft SDKs\Windows\v7.1A\Include;%INCLUDE%
rem set PATH=%PROGRAM_FILES_X86%\Microsoft SDKs\Windows\v7.1A\Bin;%PATH%
rem set LIB=%PROGRAM_FILES_X86%\Microsoft SDKs\Windows\v7.1A\Lib;%LIB%
rem set CL=/D_USING_V110_SDK71_ %CL%
rem set PlatformToolset=v110_xp
rem set PlatformToolset=V110

del ..\..\source\glest_game\facilities\gitversion.h

if not "%GITVERSION_SHA1%" == "." set CL=/DGITVERSIONHEADER
if not "%GITVERSION_SHA1%" == "." echo building with CL [%CL%]
if not "%GITVERSION_SHA1%" == "." echo #define GITVERSION "%GITVERSION_REV%.%GITVERSION_SHA1%" > ..\..\source\glest_game\facilities\gitversion.h
if not "%GITVERSION_SHA1%" == "." copy /b ..\..\source\glest_game\facilities\game_util.cpp +,,

del /Q /F Release\*.tlog

rem set VisualStudioVersion=11.0
set msBuildMaxCPU=
SET BuildInParallel=false
if %NUMBER_OF_PROCESSORS% GTR 2 (
                SET NumberOfProcessesToUseForBuild=2
                SET BuildInParallel=true
				SET msBuildMaxCPU=/maxcpucount)

ECHO Found CPU Count [%NUMBER_OF_PROCESSORS%]
SET MSBUILD_PATH_MG_x64="%ProgramFiles(x86)%\MSBuild\Microsoft.Cpp\v4.0\V140\\"

if "%2" == "rebuild" echo Doing a FULL REBUILD...
rem if "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release /t:Rebuild Glest_vc2010.sln
rem if "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release;Platform=x64 /v:q /m /t:Rebuild /p:PlatformToolset=v110_xp;VisualStudioVersion=11.0 Glest_vc2012.sln
rem /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;

rem if "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:detailed /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m /t:Rebuild Glest_vc2012.sln
rem if "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m /t:Rebuild Glest_vc2012.sln
rem if "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=Win32;PlatformToolset=v140 /m /t:Rebuild Glest_vc2015.sln
if "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=Win32;PlatformToolset=v140 /m /t:Rebuild Glest_vc2015.sln

rem if not "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release Glest_vc2010.sln
rem if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release;Platform=x64 /v:q /m /p:PlatformToolset=v110_xp Glest_vc2012.sln

rem if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:detailed /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m Glest_vc2012.sln
rem if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m Glest_vc2012.sln
rem if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:TrackFileAccess=false;VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=Win32;PlatformToolset=v140 /m Glest_vc2015.sln
if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:q /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=Win32;PlatformToolset=v140 /m Glest_vc2015.sln

ECHO ... End.
rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
