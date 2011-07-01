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

#ifndef PNGREADER_H
#define PNGREADER_H

// =====================================
//      class PNGReader
// =====================================

#include "FileReader.h"
#include "pixmap.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

class PNGReader: FileReader<Pixmap2D> {
public:
	PNGReader();

	Pixmap2D* read(ifstream& in, const string& path, Pixmap2D* ret) const;
};

class PNGReader3D: FileReader<Pixmap3D> {
public:
	PNGReader3D();

	Pixmap3D* read(ifstream& in, const string& path, Pixmap3D* ret) const;
};


}} //end namespace

#endif
