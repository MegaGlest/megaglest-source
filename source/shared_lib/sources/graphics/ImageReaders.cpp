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

#include "ImageReaders.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	namespace ImageRegisterer
// =====================================================

/**Create & register all known Readers*/
bool ImageRegisterer::registerImageReaders() {
	static BMPReader imageReaderBmp;
	static JPGReader imageReaderJpg;
	static PNGReader imageReaderPng;
	static PNGReader3D imageReader3DPng;
	static TGAReader imageReaderTga;
	static TGAReader3D imageReader3DTga;
	return true;
}

}} //end namespace
