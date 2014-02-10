@echo off
rem parent dir of this script
set PARENT_DIR=%~dp0

rem relative to where you run the script from or absolute (probably a more robust solution)
set CEGUI_BUILD_PATH=%PARENT_DIR%/../../cegui-v0-8
rem directory where the "ceed" package is located
set CEED_PACKAGE_PATH=%PARENT_DIR%/../

set PATH=%CEGUI_BUILD_PATH%/build/bin;%PATH%

set PYTHONPATH=%CEGUI_BUILD_PATH%/build/bin;%CEED_PACKAGE_PATH%;%PYTHONPATH%

start cmd.exe
