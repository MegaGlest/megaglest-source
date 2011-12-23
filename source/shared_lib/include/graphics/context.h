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

#ifndef _SHARED_GRAPHICS_CONTEXT_H_
#define _SHARED_GRAPHICS_CONTEXT_H_ 

#include "types.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

using Platform::uint32;
using Platform::int8;

// =====================================================
//	class Context
// =====================================================

class Context{
protected:
	uint32 colorBits;
	uint32 depthBits;
	uint32 stencilBits;
	int8 hardware_acceleration;
	int8 fullscreen_anti_aliasing;
	float gammaValue;

public:
	Context();
	virtual ~Context(){}

	uint32 getColorBits() const		{return colorBits;}
	uint32 getDepthBits() const		{return depthBits;}
	uint32 getStencilBits() const	{return stencilBits;}
	int8 getHardware_acceleration() const { return hardware_acceleration; }
	int8 getFullscreen_anti_aliasing() const { return fullscreen_anti_aliasing; }
	float getGammaValue() const { return gammaValue; }

	void setColorBits(uint32 colorBits)		{this->colorBits= colorBits;}
	void setDepthBits(uint32 depthBits)		{this->depthBits= depthBits;}
	void setStencilBits(uint32 stencilBits)	{this->stencilBits= stencilBits;}
	void setHardware_acceleration(int8 value) { hardware_acceleration = value; }
	void setFullscreen_anti_aliasing(int8 value) { fullscreen_anti_aliasing = value; }
	void setGammaValue(float value) { gammaValue = value; }

	virtual void init()= 0;
	virtual void end()= 0;
	virtual void reset()= 0;

	virtual void makeCurrent()= 0;
	virtual void swapBuffers()= 0;
};

}}//end namespace

#endif
