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

#ifndef TGAREADER_H
#define TGAREADER_H

// =====================================
//      class TGAReader
// =====================================

#include "FileReader.h"
#include "pixmap.h"

namespace Shared{ namespace Graphics{

class TGAReader: FileReader<Pixmap2D> {
public:
	TGAReader();

	Pixmap2D* read(ifstream& in, const string& path, Pixmap2D* ret) const;
};


}} //end namespace

#endif
