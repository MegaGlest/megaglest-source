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

#include "BMPReader.h"
#include "types.h"
#include "pixmap.h"
#include <stdexcept>
#include "util.h"
#include "leak_dumper.h"

using std::runtime_error;

namespace Shared{ namespace Graphics{

/**Copied from pixmap.cpp*/
// =====================================================
//	Structs used for BMP-reading
// =====================================================
#pragma pack(push, 1)

struct BitmapFileHeader{
	uint8 type1;
	uint8 type2;
	uint32 size;
	uint16 reserved1;
	uint16 reserved2;
	uint32 offsetBits;
};

struct BitmapInfoHeader{
	uint32 size;
	int32 width;
	int32 height;
	uint16 planes;
	uint16 bitCount;
	uint32 compression;
	uint32 sizeImage;
	int32 xPelsPerMeter;
	int32 yPelsPerMeter;
	uint32 clrUsed;
	uint32 clrImportant;
};

#pragma pack(pop)

/**Returns a string containing the extensions we want, intitialisation is guaranteed*/
//static inline const string* getExtensionsBmp() {
static inline std::vector<string> getExtensionsBmp() {
	//static const string extensions[] = {"bmp", ""};
	static std::vector<string> extensions;

	if(extensions.empty() == true) {
		extensions.push_back("bmp");
	}

	return extensions;
}

// =====================================================
//	class BMPReader
// =====================================================

BMPReader::BMPReader(): FileReader<Pixmap2D>(getExtensionsBmp()) {}

/**Reads a Pixmap2D-object
  *This function reads a Pixmap2D-object from the given ifstream utilising the already existing Pixmap2D* ret.
  *Path is used for printing error messages
  *@return <code>NULL</code> if the Pixmap2D could not be read, else the pixmap*/
Pixmap2D* BMPReader::read(ifstream& in, const string& path, Pixmap2D* ret) const {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	//read file header
	BitmapFileHeader fileHeader;
	in.read((char*)&fileHeader, sizeof(BitmapFileHeader));
	if(fileHeader.type1!='B' || fileHeader.type2!='M'){
		throw runtime_error(path +" is not a bitmap");
	}
		//read info header
	BitmapInfoHeader infoHeader;
	in.read((char*)&infoHeader, sizeof(BitmapInfoHeader));
	if(infoHeader.bitCount!=24){
		throw runtime_error(path+" is not a 24 bit bitmap");
	}
	int h= infoHeader.height;
	int w= infoHeader.width;
	int components= (ret->getComponents() == -1)?3:ret->getComponents();
	const int fileComponents = 3;
	//std::cout << "BMP-Components: Pic: " << components << " old: " << (ret->getComponents()) << " File: " << 3 << std::endl;
	ret->init(w,h,components);
	uint8* pixels = ret->getPixels();
	char buffer[4];
	//BMP is padded to sizes of 4
	const int padSize = (4 - (w*fileComponents)%4)%4; //Yeah, could be faster
	//std::cout << "Padsize = " << padSize;
	for (int y = 0, i = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x, i+=components) {
			uint8 r, g, b;
			in.read((char*)&b, 1);
			in.read((char*)&g, 1);
			in.read((char*)&r, 1);
			if (!in.good()) {
				return NULL;
			}
			switch(components){
				case 1:
				pixels[i]= (r+g+b)/3;
				break;
			case 3:
				pixels[i]= r;
				pixels[i+1]= g;
				pixels[i+2]= b;
				break;
			case 4:
				pixels[i]= r;
				pixels[i+1]= g;
				pixels[i+2]= b;
				pixels[i+3]= 255;
				break;
			}
		}
		if (padSize) {
			in.read(buffer,padSize);
		}
	}
	/*for(int i = 0; i < w*h*components; ++i) {
		if (i%39 == 0) std::cout << std::endl;
		int first = pixels[i]/16;
		if (first < 10)
			std:: cout << first;
		else
			std::cout << (char)('A'+(first-10));
		first = pixels[i]%16;
		if (first < 10)
			std:: cout << first;
		else
			std::cout << (char)('A'+(first-10));
		std::cout << " ";		
	}*/
	return ret;
}

}} //end namespace
