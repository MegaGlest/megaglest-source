// ==============================================================
//	This file is part of MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2013 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_COMPRESSION_UTIL_CHECKSUM_H_
#define _SHARED_COMPRESSION_UTIL_CHECKSUM_H_

#include <string>

using std::string;

namespace Shared{ namespace CompressionUtil{

bool compressFileToZIPFile(string inFile, string outFile, int compressionLevel=5);
bool extractFileFromZIPFile(string inFile, string outFile);

}};

#endif
