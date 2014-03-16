@echo off

copy cegui-source\build\bin\CEGUIBase-0.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUIBase-0.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUICommonDialogs-0.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUICommonDialogs-0.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUICoreWindowRendererSet.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUICoreWindowRendererSet.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUIFreeImageImageCodec.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUIFreeImageImageCodec.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUILuaScriptModule-0.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUILuaScriptModule-0.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUIOpenGLRenderer-0.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUIOpenGLRenderer-0.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUIRapidXMLParser.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUIRapidXMLParser.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUISILLYImageCodec.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUISILLYImageCodec.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUISTBImageCodec.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUISTBImageCodec.lib ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\bin\CEGUITGAImageCodec.dll ..\windows_deps\cegui-0.8.x\bin\
copy cegui-source\build\lib\CEGUITGAImageCodec.lib ..\windows_deps\cegui-0.8.x\bin\

rem copy ..\windows\_deps\cegui-deps-0.8.x-src\build\dependencies\bin\*.dll ..\..\mk\windoze\

if not "%1" == "nopause" pause
