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

#include "graphics_interface.h"

#include <cstddef>

#include "context.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class GraphicsInterface
// =====================================================

GraphicsInterface::GraphicsInterface(){
	graphicsFactory= NULL;
	currentContext= NULL;
}

GraphicsInterface &GraphicsInterface::getInstance(){
	static GraphicsInterface graphicsInterface;
	return graphicsInterface;
}

void GraphicsInterface::setFactory(GraphicsFactory *graphicsFactory){
	this->graphicsFactory= graphicsFactory;
}

void GraphicsInterface::setCurrentContext(Context *context){
	this->currentContext= context;
	currentContext->makeCurrent();
}

}}//end namespace
