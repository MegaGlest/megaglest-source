@echo off
rem parent dir of this script
set PARENT_DIR=%~dp0

rem relative to where you run the script from or absolute (probably a more robust solution)
set CEGUI_BUILD_PATH=%PARENT_DIR%/../../cegui-source/

set CEGUI_BUILD_DEP_PATH=%PARENT_DIR%/../../../../windows_deps/cegui-deps-0.8.x-src/build/dependencies/bin

rem directory where the "ceed" package is located
set CEED_PACKAGE_PATH=%PARENT_DIR%/../

set PATH=%CEGUI_BUILD_PATH%/build/bin;%CEGUI_BUILD_DEP_PATH%;C:\Code\megaglest\git\source\external-deps\cegui-source\build\bin;C:\Code\megaglest\git\source\external-deps\cegui-source\build\bin;C:\Code\megaglest\git\source\windows_deps\cegui-deps-0.8.x-src\build\dependencies\bin;%PATH%

set PYTHONPATH=%CEGUI_BUILD_PATH%/build/bin;%CEGUI_BUILD_PATH%/build/lib;%CEGUI_BUILD_DEP_PATH%;%CEED_PACKAGE_PATH%;C:\Code\megaglest\git\source\external-deps\cegui-source\build\bin;C:\Code\megaglest\git\source\windows_deps\cegui-deps-0.8.x-src\build\dependencies\bin;  C:\Code\megaglest\git\source\external-deps\cegui-source\build\lib;C:\Code\megaglest\git\source\windows_deps\cegui-deps-0.8.x-src\build\dependencies\lib;%PYTHONPATH%

start cmd.exe
