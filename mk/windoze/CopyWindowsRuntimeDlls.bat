@echo off

rem if not exist ..\..\data\glest_game\dsound.dll copy dsound.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\xerces-c_3_0.dll copy ..\..\source\win32_deps\bin\xerces-c_3_0.dll ..\..\data\glest_game\
if not exist ..\..\data\glest_game\openal32.dll copy ..\..\source\win32_deps\bin\openal32.dll ..\..\data\glest_game\

if not "%1" == "nopause" pause
