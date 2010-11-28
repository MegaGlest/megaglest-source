// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "platform_util.h"

#include <io.h>
#include <dbghelp.h>

#include <cassert>
#include <algorithm>
#include <string.h>
#include "SDL_syswm.h"
#include <iostream>

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Platform {

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::thisPointer= NULL;

LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers){

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE hFile = CreateFile(
		thisPointer->dumpFileName.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MINIDUMP_EXCEPTION_INFORMATION lExceptionInformation;

	lExceptionInformation.ThreadId= GetCurrentThreadId();
	lExceptionInformation.ExceptionPointers= pointers;
	lExceptionInformation.ClientPointers= false;

	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MiniDumpNormal,
		&lExceptionInformation,
		NULL,
		NULL );

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	thisPointer->handle();

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install(string dumpFileName){
	thisPointer= this;
	this->dumpFileName= dumpFileName;
	SetUnhandledExceptionFilter(handler);
}

// =====================================================
//	class Misc
// =====================================================

//void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight) {
//    // Get the current video hardware information
//    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
//    //colorBits      = vidInfo->vfmt->BitsPerPixel;
//    //screenWidth    = vidInfo->current_w;
//
///*
//	//screenHeight   = vidInfo->current_h;
//    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
//    // height
//    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//
//	printf("cx = %d, cy = %d\n",cx,cy);
//
//	if(cx > screenWidth) {
//		screenWidth  = cx;
//		screenHeight = cy;
//	}
//*/
//	int iMaxWidth	= -1;
//	int iMaxHeight	= -1;
//	int iMaxBits	= -1;
//
//	DEVMODE devMode;
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//
//		//printf("devMode.dmPelsWidth = %d, devMode.dmPelsHeight = %d, devMode.dmBitsPerPel = %d\n",devMode.dmPelsWidth,devMode.dmPelsHeight,devMode.dmBitsPerPel);
//
//		if(devMode.dmPelsWidth > iMaxWidth) {
//			iMaxWidth  = devMode.dmPelsWidth;
//			iMaxHeight = devMode.dmPelsHeight;
//			iMaxBits   = devMode.dmBitsPerPel;
//			//devMode.dmDisplayFrequency=refreshFrequency;
//		}
//	}
//	if(iMaxWidth > 0) {
//		colorBits      = iMaxBits;
//		screenWidth    = iMaxWidth;
//		screenHeight   = iMaxHeight;
//	}
//}
//
//bool changeVideoMode(int resW, int resH, int colorBits, int refreshFrequency){
//	DEVMODE devMode;
//
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//		if (devMode.dmPelsWidth== resW &&
//			devMode.dmPelsHeight== resH &&
//			devMode.dmBitsPerPel== colorBits){
//
//			devMode.dmDisplayFrequency=refreshFrequency;
//
//			LONG result= ChangeDisplaySettings(&devMode, 0);
//			if(result == DISP_CHANGE_SUCCESSFUL){
//				return true;
//			}
//			else{
//				return false;
//			}
//		}
//	}
//
//	return false;
//}
//
//void restoreVideoMode(bool exitingApp) {
//	int dispChangeErr= ChangeDisplaySettings(NULL, 0);
//	assert(dispChangeErr==DISP_CHANGE_SUCCESSFUL);
//}

void message(string message){
	std::cerr << "******************************************************\n";
	std::cerr << "    " << message << "\n";
	std::cerr << "******************************************************\n";

	MessageBox(NULL, message.c_str(), "Message", MB_OK);
}

bool ask(string message){
	return MessageBox(NULL, message.c_str(), "Confirmation", MB_YESNO)==IDYES;
}

void exceptionMessage(const exception &excp){
	string message, title;
	showCursor(true);

	message+= "ERROR(S):\n\n";
	message+= excp.what();

	title= "Error: Unhandled Exception";
	printf("Error detected with text: %s\n",message.c_str());

	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}

//int getScreenW(){
//	return GetSystemMetrics(SM_CXSCREEN);
//}

//int getScreenH(){
//	return GetSystemMetrics(SM_CYSCREEN);
//}

// This lets us set the SDL Window Icon in Windows
// since its a console application
HICON icon;

void init_win32() {
	HINSTANCE handle = ::GetModuleHandle(NULL);
	icon = ::LoadIcon(handle, "IDI_ICON1");

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo) != 1)
	{
		// error: wrong SDL version
	}

	HWND hwnd = wminfo.window;

	::SetClassLong(hwnd, GCL_HICON, (LONG) icon);
}
void done_win32() {
	::DestroyIcon(icon);
}

}}//end namespace
