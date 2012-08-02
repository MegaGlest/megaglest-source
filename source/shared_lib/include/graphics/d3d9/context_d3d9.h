//	This file is part of MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_D3D9_DEVICECONTEXTD3D9_H_
#define _SHARED_D3D9_DEVICECONTEXTD3D9_H_ 

#include "context.h"

#include <d3d9.h>

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class ContextD3d9  
// ===============================
    
class ContextD3d9: public Context{
private:
	bool windowed;
	bool hardware;

	IDirect3D9 *d3dObject;
	IDirect3DDevice9 *d3dDevice;
	D3DCAPS9 caps;
	D3DPRESENT_PARAMETERS d3dPresentParameters;

public:
	bool getWindowed() const	{return windowed;}
	bool getHardware() const	{return hardware;}

	void setWindowed(bool windowed)	{this->windowed= windowed;}
	void setHardware(bool hardware)	{this->hardware= hardware;}

	ContextD3d9();

	virtual void init();
	virtual void end();
	virtual void reset();

	virtual void makeCurrent();
	virtual void swapBuffers();

	const D3DCAPS9 *getCaps() const	{return &caps;};


	IDirect3DDevice9 *getD3dDevice() {return d3dDevice;}
};

}}}//end namespace

#endif
