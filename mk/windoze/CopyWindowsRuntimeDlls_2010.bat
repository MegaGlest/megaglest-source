@echo off

rem if not exist ..\..\data\glest_game\dsound.dll copy dsound.dll ..\..\data\glest_game\
rem if not exist ..\..\data\glest_game\xerces-c_3_0.dll copy ..\..\source\windows_deps\bin\xerces-c_3_0.dll ..\..\data\glest_game\
copy ..\..\source\windows_deps\lib\openal32.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\libvlc.dll copy ..\..\source\windows_deps\lib\libvlc.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\libvlccore.dll copy ..\..\source\windows_deps\lib\libvlccore.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\plugins\nul xcopy ..\..\source\windows_deps\vlc-2.0.1\plugins ..\..\data\glest_game\plugins\ /s

if not "%1" == "nopause" pause
