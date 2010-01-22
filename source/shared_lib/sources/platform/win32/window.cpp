// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "window.h"

#include <cassert>

#include "control.h"
#include "conversion.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

// =====================================================
//	class Window
// =====================================================

const DWORD Window::fullscreenStyle= WS_POPUP | WS_OVERLAPPED;
const DWORD Window::windowedFixedStyle= WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_SYSMENU;
const DWORD Window::windowedResizeableStyle= WS_SIZEBOX | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_SYSMENU;

int Window::nextClassName= 0;
Window::WindowMap Window::createdWindows;

// ===================== PUBLIC ========================

Window::Window(){
	handle= 0;
	style=  windowedFixedStyle;
	exStyle= 0;
	ownDc= false;
	x= 0;
	y= 0;
	w= 100;
	h= 100;
}

Window::~Window(){
	if(handle!=0){
		DestroyWindow(handle);
		handle= 0;
		BOOL b= UnregisterClass(className.c_str(), GetModuleHandle(NULL)); 
		assert(b);
	}
}

//static
bool Window::handleEvent(){
	MSG msg;

	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
		if(msg.message==WM_QUIT){
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

string Window::getText(){
	if(handle==0){
		return text;
	}

	char c[255];
	SendMessage(handle, WM_GETTEXT, 255, (LPARAM) c);
	return string(c);
}

int Window::getClientW(){
	RECT r;

	GetClientRect(handle, &r);
	return r.right;
}

int Window::getClientH(){
	RECT r;

	GetClientRect(handle, &r);
	return r.bottom;
}

float Window::getAspect(){
	return static_cast<float>(getClientH())/getClientW();
}

void Window::setText(string text){
    this->text= text; 
	if(handle!=0){	
		SendMessage(handle, WM_SETTEXT, 0, (LPARAM) text.c_str());
	}
}

void Window::setSize(int w, int h){

	if(windowStyle != wsFullscreen){
		RECT rect;
		rect.left= 0;
		rect.top= 0;
		rect.bottom= h;
		rect.right= w;
		
		AdjustWindowRect(&rect, style, FALSE);

		w= rect.right-rect.left;
		h= rect.bottom-rect.top;
	}

	this->w= w;
	this->h= h;

	if(handle!=0){
		MoveWindow(handle, x, y, w, h, FALSE);
		UpdateWindow(handle);
	}
}

void Window::setPos(int x, int y){
	this->x= x;
	this->y= y;
	if(handle!=0){
		MoveWindow(handle, x, y, w, h, FALSE);
	}
}

void Window::setEnabled(bool enabled){
	EnableWindow(handle, static_cast<BOOL>(enabled));
}

void Window::setVisible(bool visible){
     
     if (visible){
          ShowWindow(handle, SW_SHOW);
          UpdateWindow(handle);
     }
     else{
          ShowWindow(handle, SW_HIDE);
          UpdateWindow(handle); 
     }
}

void Window::setStyle(WindowStyle windowStyle){
	this->windowStyle= windowStyle;
	switch(windowStyle){
	case wsFullscreen:
		style= fullscreenStyle;
		exStyle= WS_EX_APPWINDOW;
		ownDc= true;
		break;
	case wsWindowedFixed:
		style= windowedFixedStyle;
		exStyle= 0;
		ownDc= false;
		break;
	case wsWindowedResizeable:
		style= windowedResizeableStyle;
		exStyle= 0;
		ownDc= false;
		break;
	}

	if(handle!=0){
		setVisible(false);
		int err= SetWindowLong(handle,  GWL_STYLE, style);
		assert(err);
		
		setVisible(true);
		UpdateWindow(handle);
	}
}

void Window::create(){
	registerWindow();
	createWindow();
}

void Window::minimize(){
	ShowWindow(getHandle(), SW_MINIMIZE);
}

void Window::maximize(){
	ShowWindow(getHandle(), SW_MAXIMIZE);
}

void Window::restore(){
	ShowWindow(getHandle(), SW_RESTORE);
}

void Window::showPopupMenu(Menu *menu, int x, int y){
	RECT rect;
	
	GetWindowRect(handle, &rect);

	TrackPopupMenu(menu->getHandle(), TPM_LEFTALIGN | TPM_TOPALIGN, rect.left+x, rect.top+y, 0, handle, NULL);
}

void Window::destroy(){
	DestroyWindow(handle);
	BOOL b= UnregisterClass(className.c_str(), GetModuleHandle(NULL)); 
	assert(b);
	handle= 0;
}

// ===================== PRIVATE =======================

LRESULT CALLBACK Window::eventRouter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
	Window *eventWindow;
	WindowMap::iterator it;
	
	it= createdWindows.find(hwnd);
	if(it==createdWindows.end()){
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	eventWindow= it->second;

	POINT mousePos;
	RECT windowRect;
	
	GetWindowRect(eventWindow->getHandle(), &windowRect);	

	mousePos.x = LOWORD(lParam) - windowRect.left;
	mousePos.y = HIWORD(lParam) -windowRect.top;

	ClientToScreen(eventWindow->getHandle(), &mousePos);

	switch(msg){
	case WM_CREATE:
	    eventWindow->eventCreate();        
		break;

	case WM_LBUTTONDOWN:
		eventWindow->eventMouseDown(mousePos.x, mousePos.y, mbLeft);
		break;

	case WM_RBUTTONDOWN:
		eventWindow->eventMouseDown(mousePos.x, mousePos.y, mbRight);
		break;

	case WM_LBUTTONUP:
		eventWindow->eventMouseUp(mousePos.x, mousePos.y, mbLeft);
		break;

	case WM_RBUTTONUP:
		eventWindow->eventMouseUp(mousePos.x, mousePos.y, mbRight);
		break;

	case WM_LBUTTONDBLCLK:
		eventWindow->eventMouseDoubleClick(mousePos.x, mousePos.y, mbLeft);
		break;

	case WM_RBUTTONDBLCLK:
		eventWindow->eventMouseDoubleClick(mousePos.x, mousePos.y, mbRight);
		break;

	case WM_MOUSEMOVE:
		{
			MouseState ms;
			ms.leftMouse= (wParam & MK_LBUTTON) ? true : false;
			ms.rightMouse= (wParam & MK_RBUTTON) ? true : false;
			ms.centerMouse= (wParam & MK_MBUTTON) ? true : false;
			eventWindow->eventMouseMove(mousePos.x, mousePos.y, &ms);
		}
		break;

	case WM_KEYDOWN:
		eventWindow->eventKeyDown(static_cast<char>(wParam));
		break;

	case WM_KEYUP:
		eventWindow->eventKeyUp(static_cast<char>(wParam));
		break;

	case WM_CHAR:
		eventWindow->eventKeyPress(static_cast<char>(wParam));
		break;

	case WM_COMMAND:
		if(HIWORD(wParam)==0){
			eventWindow->eventMenu(LOWORD(wParam));
		}
		break;

	case WM_ACTIVATE:
		eventWindow->eventActivate(wParam!=WA_INACTIVE);
		break;

	case WM_MOVE:
		{
			RECT rect;
			
			GetWindowRect(eventWindow->getHandle(), &rect);
			eventWindow->x= rect.left;
			eventWindow->y= rect.top;
			eventWindow->w= rect.right-rect.left;
			eventWindow->h= rect.bottom-rect.top;
		}
		break;

	case WM_SIZE:
		{
			RECT rect;

			GetWindowRect(eventWindow->getHandle(), &rect);
			eventWindow->x= rect.left;
			eventWindow->y= rect.top;
			eventWindow->w= rect.right-rect.left;
			eventWindow->h= rect.bottom-rect.top;
			
			eventWindow->eventResize(static_cast<SizeState>(wParam));
		}
		break;

	case WM_SIZING:
		eventWindow->eventResize();
		break;

	case WM_PAINT:
		eventWindow->eventPaint();
		break;

	case WM_CLOSE:
		eventWindow->eventClose();
		break;

	case WM_DESTROY:
		eventWindow->eventDestroy();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int Window::getNextClassName(){
	return ++nextClassName;
}

void Window::registerWindow(WNDPROC wndProc){
    WNDCLASSEX wc;

	this->className= "Window" + intToStr(Window::getNextClassName());

    wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_DBLCLKS | (ownDc? CS_OWNDC : 0);
	wc.lpfnWndProc   = wndProc==NULL? eventRouter: wndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = this->className.c_str();
    wc.hIconSm       = NULL;

    int registerClassErr=RegisterClassEx(&wc); 
    assert(registerClassErr);

}

void Window::createWindow(LPVOID creationData){

	handle = CreateWindowEx(
		exStyle, 
		className.c_str(),
		text.c_str(),
		style,
		x, y, w, h,
		NULL, NULL, GetModuleHandle(NULL), creationData);

	createdWindows.insert(pair<WindowHandle, Window*>(handle, this));
	eventRouter(handle, WM_CREATE, 0, 0);

	assert(handle != NULL);

	ShowWindow(handle, SW_SHOW);
	UpdateWindow(handle);  
}

}}//end namespace
