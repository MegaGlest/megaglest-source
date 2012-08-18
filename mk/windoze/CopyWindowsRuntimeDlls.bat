@echo off

rem if not exist ..\..\data\glest_game\dsound.dll copy dsound.dll ..\..\data\glest_game\
rem copy ..\..\source\win32_deps\bin\xerces-c_3_0.dll ..\..\data\glest_game\
copy ..\..\source\win32_deps\bin\openal32.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\libvlc.dll copy ..\..\source\win32_deps\lib\libvlc.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\libvlccore.dll copy ..\..\source\win32_deps\lib\libvlccore.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\plugins\nul xcopy ..\..\source\win32_deps\vlc-2.0.1\plugins ..\..\data\glest_game\plugins\ /s

if not "%1" == "nopause" pause
