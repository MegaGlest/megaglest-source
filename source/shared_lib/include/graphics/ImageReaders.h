// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2010 Martiño Figueroa and others
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef IMAGE_READERS_H
#define IMAGE_READERS_H

#include "FileReader.h"
#include "BMPReader.h"
#include "JPGReader.h"
#include "PNGReader.h"
#include "TGAReader.h"
#include "leak_dumper.h"

//Initialize some objects
namespace Shared{ namespace Graphics{

// =====================================================
//	namespace ImageRegisterer
// =====================================================

namespace ImageRegisterer {

	//This function registers all image-readers, but only once (any further call is unnecessary)
	bool registerImageReaders();

	//Since you can't call void methods here, I have used a method doing nothing except initializing the image Readers
	static bool readersRegistered = registerImageReaders(); //should always return true, this should guarantee that the readers are registered <--> ImageReaders is included anywhere
}

}} //end namespace

#endif
