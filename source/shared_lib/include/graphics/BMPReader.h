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

#ifndef BMPREADER_H
#define BMPREADER_H

// =====================================
//      class BMPReader
// =====================================

#include "FileReader.h"
#include "pixmap.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

class BMPReader: FileReader<Pixmap2D> {
public:
	BMPReader();

	Pixmap2D* read(ifstream& in, const string& path, Pixmap2D* ret) const;
};


}} //end namespace

#endif
