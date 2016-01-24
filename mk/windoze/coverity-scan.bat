@echo off
rem
rem Upload build logs to Coverity Scan for security / code style analysis
rem Requires:
rem - curl.exe, built with SSL support: http://curl.haxx.se/download.html
rem - wget.exe (should get installed automatically during a build)
rem - 7z.exe (should get installed automatically during a build)
rem - Coverity Scan Build Tool installed and its install path configured in .coverity-scan
rem

rem Change into the directory this batch file is located in
cd /d "%~dp0"

rem Project name (case sensitive)
set PROJECT=MegaGlest

rem read in config settings
if not exist ".coverity-scan" (
        echo -----------------------------------------
        echo **Missing Config** To use this script please create a config file named [%CD%\.coverity-scan]
        echo Containing: EMAIL=x, TOKEN=x, COVERITY_ANALYSIS_ROOT=x, BITNESS=x, NUMCORES=x
        goto END
)

setlocal disabledelayedexpansion
FOR /F "tokens=1* delims==" %%i IN (.coverity-scan) DO set "prop_%%i=%%j"

rem E-Mail address of registered Coverity Scan user with build submission access to this project
set EMAIL=%prop_EMAIL%

rem Coverity Scan project API token as listed on the Coverity Scan project page
set TOKEN=%prop_TOKEN%

rem Where you extracted coverity build scan tool to
rem e.g. "C:\cov-analysis-win32-6.6.1"
set COVERITY_ANALYSIS_ROOT=%prop_COVERITY_ANALYSIS_ROOT%

rem Is this a "32" or "64" bit build of MegaGlest?
set BITNESS=%prop_BITNESS%

rem Description of this build (can be any string)
set DESCRIPTION=Windows-%BITNESS%_%COMPUTERNAME%

rem Where to store the data gathered by the Coverity Scan Build Tool
rem Absolute path or relevant to the directory containing this batch file
set BUILDTOOL=cov-int

rem ------------------------------------------------------------------------------

echo PROJECT [%PROJECT%]
echo EMAIL [%EMAIL%] 
echo TOKEN [%TOKEN%] 
echo COVERITY_ANALYSIS_ROOT [%COVERITY_ANALYSIS_ROOT%]
echo BITNESS [%BITNESS%]
echo DESCRIPTION [%DESCRIPTION%]
echo BUILDTOOL [%BUILDTOOL%]

rem pause

set GITVERSION_SHA1=.
set GITVERSION_REV=.
set GET_GIT_SHA1="git log -1 --format=%%h"
for /f "delims=" %%a in ('%GET_GIT_SHA1%') do @set GITVERSION_SHA1=%%a
for /f "delims=" %%a in ('git rev-list HEAD --count') do @set GITVERSION_REV=%%a

set VERSION=%GITVERSION_REV%.%GITVERSION_SHA1%

set FILENAME=%PROJECT%_%DESCRIPTION%_%VERSION%

rem Untested! Requires modification.
rem wget.exe --no-check-certificate https://scan.coverity.com/download/win-32 --post-data "token=%TOKEN%&project=%PROJECT%" -O %TEMP%\coverity_tool.zip
rem 7z.exe x %TEMP%\coverity_tool.zip
rem set PATH=%PATH%;C:\build\megaglest-source\mk\windoze\cov-analysis-win32-6.6.1\bin\

if "%MG_COV_PATH_SET%." == "." set PATH=%PATH%;%COVERITY_ANALYSIS_ROOT%\bin\
set MG_COV_PATH_SET=TRUE

if "%BITNESS%." == "32." GOTO BUILD32
if "%BITNESS%." == "64." GOTO BUILD64
GOTO ERROR

:BUILD32
cov-build --dir %BUILDTOOL% build-mg32bit-2015.bat nopause rebuild
if ERRORLEVEL 1 GOTO ERROR
GOTO PACKAGE

:BUILD64
cov-build --dir %BUILDTOOL% build-mg-2015.bat nopause rebuild
if ERRORLEVEL 1 GOTO ERROR
GOTO PACKAGE

:PACKAGE
7z.exe a %FILENAME%.tar %BUILDTOOL%\
7z.exe a %FILENAME%.tar.gz %FILENAME%.tar
del /Q /F %FILENAME%.tar
dir %FILENAME%.tar.gz

echo **About to run: curl.exe --progress-bar --insecure --form "token=%TOKEN%" --form "email=%EMAIL%" --form "version=%VERSION%" --form "description=%DESCRIPTION%" --form "file=@%FILENAME%.tar.gz" "https://scan.coverity.com/builds?project=%PROJECT%"
rem pause
rem echo Running curl
curl.exe --progress-bar --insecure --form "token=%TOKEN%" --form "email=%EMAIL%" --form "version=%VERSION%" --form "description=%DESCRIPTION%" --form "file=@%FILENAME%.tar.gz" "https://scan.coverity.com/builds?project=%PROJECT%"
if ERRORLEVEL 1 GOTO ERROR
GOTO CLEANUP

:CLEANUP
del /Q /F %FILENAME%.tar.gz
rd /Q /S %BUILDTOOL%\
GOTO END

:ERROR
echo An error occurred
GOTO END

:END
