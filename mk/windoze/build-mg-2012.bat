@echo off

rem To get this working you may need to copy the following:
rem C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V110\Microsoft.Cpp.Common.props to C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0
rem C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V110\Platforms\Win32\PlatformToolsets\*.* to C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\Platforms\Win32\PlatformToolsets
rem C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V110\Platforms\Win32\Microsoft.Cpp.Win32.Common.props to C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\Platforms\Win32

rem change to the directory of this batch file
SET VCVARS_PLATFORM=x86_amd64

ECHO --------------------------------
ECHO Changing to build folder [%~dp0] p1 ["%1"] p2 ["%2"]
rem pause
cd /d "%~dp0"

ECHO Checking for windows binary runtime tools...
if NOT EXIST ..\..\data\glest_game\7z.exe call cscript getTools.vbs
if NOT EXIST ..\..\data\glest_game\7z.dll call cscript getTools.vbs
if NOT EXIST ..\..\data\glest_game\wget.exe call cscript getTools.vbs

set depfolder=windows_deps_2012
set depfile=%depfolder%.7z 

dir ..\..\source\
if NOT EXIST ..\..\source\%depfolder%\NUL echo folder not found [%depfolder%]
if NOT EXIST ..\..\source\%depfolder%\NUL goto checkDepIntegrity
goto processBuildStageA

:getDepFile
ECHO Retrieving windows dependency archive...
rem call ..\..\data\glest_game\wget.exe -c -O ..\..\source\%depfile%  http://master.dl.sourceforge.net/project/megaglest/%depfile%
call ..\..\data\glest_game\wget.exe -c -O ..\..\source\%depfile% http://download.sourceforge.net/project/megaglest/%depfile%
call ..\..\data\glest_game\7z.exe x -r -o..\..\source\ ..\..\source\%depfile%
goto processBuildStageA

:checkDepIntegrity
ECHO Looking for windows dependency archive...
call ..\..\data\glest_game\7z.exe t ..\..\source\%depfile% >nul
set 7ztestdep=%ERRORLEVEL%
ECHO Result of windows dependency archive [%7ztestdep%]
if NOT "%7ztestdep%" == "0" goto getDepFile
goto processBuildStageA

:processBuildStageA
call CopyWindowsRuntimeDlls_2012.bat nopause

rem setup the Visual Studio 2010 environment
ECHO --------------------------------
ECHO Setting up Visual Studio 2012 environment vars...
REM Ensure ultifds HP doesn't mess the build up
SET Platform=
if "%DevEnvDir%." == "." goto SETVCVARS
GOTO SVNSECTION

:SETVCVARS

IF EXIST "%VS110COMNTOOLS%..\..\"                             GOTO VC_Common_12
IF EXIST "\Program Files\Microsoft Visual Studio 11.0\"       GOTO VC_32_12
IF EXIST "\Program Files (x86)\Microsoft Visual Studio 11.0\" GOTO VC_64_12

rem IF EXIST "%VS100COMNTOOLS%..\..\"                             GOTO VC_Common
rem IF EXIST "\Program Files\Microsoft Visual Studio 10.0\"       GOTO VC_32
rem IF EXIST "\Program Files (x86)\Microsoft Visual Studio 10.0\" GOTO VC_64
goto SVNSECTION

:VC_Common_12
call "%VS110COMNTOOLS%..\..\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto SVNSECTION

:VC_32_12
ECHO 32 bit Windows detected...
call "\Program Files\Microsoft Visual Studio 11.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto SVNSECTION

:VC_64_12
ECHO 64 bit Windows detected...
call "\Program Files (x86)\Microsoft Visual Studio 11.0\vc\vcvarsall.bat" %VCVARS_PLATFORM%
goto SVNSECTION

:VC_Common
rem call "%VS100COMNTOOLS%..\..\vc\vcvarsall.bat"
goto SVNSECTION

:VC_32
ECHO 32 bit Windows detected...
rem call "\Program Files\Microsoft Visual Studio 10.0\vc\vcvarsall.bat"
goto SVNSECTION

:VC_64
ECHO 64 bit Windows detected...
rem call "\Program Files (x86)\Microsoft Visual Studio 10.0\vc\vcvarsall.bat"
goto SVNSECTION

:SVNSECTION
rem Update from SVN to latest rev
ECHO --------------------------------
Echo Updating Code from SVN to latest Revision...
svn update ..\..\

set SVNVERSION=.
for /f "delims=" %%a in ('svnversion ..\..\ -n') do @set SVNVERSION=%%a
ECHO Will build using SVN Revision: [%SVNVERSION%]
rem pause

ECHO --------------------------------
Echo Touching the build date/time file so we get proper build stamp
rem touch ..\..\source\glest_game\facilities\game_util.cpp
copy /b ..\..\source\glest_game\facilities\game_util.cpp +,,

rem Build Mega-Glest in release mode
ECHO --------------------------------
Echo Building Mega-Glest using Visual Studio 2012...

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

del ..\..\source\glest_game\facilities\svnversion.h

if not "%SVNVERSION%" == "." set CL=/DSVNVERSIONHEADER
if not "%SVNVERSION%" == "."  echo building with CL [%CL%]
if not "%SVNVERSION%" == "." echo #define SVNVERSION "%SVNVERSION%" > ..\..\source\glest_game\facilities\svnversion.h

rem set VisualStudioVersion=11.0
set msBuildMaxCPU=
SET BuildInParallel=false
if %NUMBER_OF_PROCESSORS% GTR 2 (
                SET NumberOfProcessesToUseForBuild=2
                SET BuildInParallel=true
				SET msBuildMaxCPU=/maxcpucount)

ECHO Found CPU Count [%NUMBER_OF_PROCESSORS%]
SET MSBUILD_PATH_MG_x64="C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V110\\"

if "%2" == "rebuild" echo Doing a FULL REBUILD...
rem if "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release /t:Rebuild Glest_vc2010.sln
rem if "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release;Platform=x64 /v:q /m /t:Rebuild /p:PlatformToolset=v110_xp;VisualStudioVersion=11.0 Glest_vc2012.sln
rem /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;
if "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:detailed /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m /t:Rebuild Glest_vc2012.sln

rem if not "%2" == "rebuild" msbuild /detailedsummary %msBuildMaxCPU% /p:BuildInParallel=%BuildInParallel% /p:Configuration=Release Glest_vc2010.sln
rem if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /p:Configuration=Release;Platform=x64 /v:q /m /p:PlatformToolset=v110_xp Glest_vc2012.sln
if not "%2" == "rebuild" msbuild %msBuildMaxCPU% /v:detailed /p:VCTargetsPath=%MSBUILD_PATH_MG_x64%;Configuration=Release;Platform=x64;PlatformToolset=v110 /m Glest_vc2012.sln

rem pause execution so we can see the output before the batch file exits
if not "%1" == "nopause" pause
