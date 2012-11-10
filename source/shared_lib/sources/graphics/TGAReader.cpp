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

#include "TGAReader.h"
#include "data_types.h"
#include "pixmap.h"
#include <stdexcept>
#include <iostream>
#include "util.h"
#include "leak_dumper.h"

using std::runtime_error;

namespace Shared{ namespace Graphics{

#pragma pack(push, 1)

/**Copied from pixmap.cpp*/
// =====================================================
//	Information for reading the targa-file
// =====================================================
struct TargaFileHeader{
	int8 idLength;
	int8 colourMapType;
	int8 dataTypeCode;
	int16 colourMapOrigin;
	int16 colourMapLength;
	int8 colourMapDepth;
	int16 xOrigin;
	int16 yOrigin;
	int16 width;
	int16 height;
	int8 bitsPerPixel;
	int8 imageDescriptor;
};

#pragma pack(pop)

static const int tgaUncompressedRgb= 2;
static const int tgaUncompressedBw= 3;

// =====================================================
//	class TGAReader
// =====================================================

/**Get Extension array, initialised*/
//static inline const string* getExtensionStrings() {
static inline std::vector<string> getExtensionStrings() {
	//static const string extensions[] = {"tga", ""};
	static std::vector<string> extensions;
	if(extensions.empty() == true) {
		extensions.push_back("tga");
	}

	return extensions;
}

TGAReader3D::TGAReader3D(): FileReader<Pixmap3D>(getExtensionStrings()) {}

Pixmap3D* TGAReader3D::read(ifstream& in, const string& path, Pixmap3D* ret) const {
	//printf("In [%s] line: %d\n",__FILE__,__LINE__);
//	try {
		assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
		//read header
		TargaFileHeader fileHeader;
		in.read((char*)&fileHeader, sizeof(TargaFileHeader));
		static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
		if(bigEndianSystem == true) {
			fileHeader.bitsPerPixel = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.bitsPerPixel);
			fileHeader.colourMapDepth = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapDepth);
			fileHeader.colourMapLength = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapDepth);
			fileHeader.colourMapOrigin = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapOrigin);
			fileHeader.colourMapType = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapType);
			fileHeader.dataTypeCode = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.dataTypeCode);
			fileHeader.height = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.height);
			fileHeader.idLength = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.idLength);
			fileHeader.imageDescriptor = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.imageDescriptor);
			fileHeader.width = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.width);
		}

		if (!in.good()) {
			throw megaglest_runtime_error(path + " could not be read",true);
		}

		//check that we can load this tga file
		if(fileHeader.idLength!=0){
			throw megaglest_runtime_error(path + ": id field is not 0",true);
		}

		if(fileHeader.dataTypeCode!=tgaUncompressedRgb && fileHeader.dataTypeCode!=tgaUncompressedBw){
			throw megaglest_runtime_error(path + ": only uncompressed BW and RGB targa images are supported",true);
		}

		//check bits per pixel
		if(fileHeader.bitsPerPixel!=8 && fileHeader.bitsPerPixel!=24 && fileHeader.bitsPerPixel!=32){
			throw megaglest_runtime_error(path + ": only 8, 24 and 32 bit targa images are supported",true);
		}

		const int h = fileHeader.height;
		const int w = fileHeader.width;
		const int d = ret->getD();
		const int slice = ret->getSlice();
		const int fileComponents= fileHeader.bitsPerPixel/8;
		const int picComponents = (ret->getComponents()==-1)?fileComponents:ret->getComponents();
		//std::cout << "TGA-Components: Pic: " << picComponents << " old: " << (ret->getComponents()) << " File: " << fileComponents << " slice:" << ret->getSlice() << std::endl;
		if(ret->getPixels() == NULL){
			ret->init(w,h,d, picComponents);
		}
		uint8* pixels = ret->getPixels();
		if(slice > 0) {
			pixels = &pixels[slice*w*h*picComponents];
		}
		//read file
		for(int i=0; i<h*w*picComponents; i+=picComponents){
			uint8 r=0, g=0, b=0, a=0, l=0;

			if(fileComponents==1){
				in.read((char*)&l,1);
				if(bigEndianSystem == true) {
					l = Shared::PlatformByteOrder::fromCommonEndian(l);
				}

				r= l;
				g= l;
				b= l;
				a= 255;
			}
			else{
				in.read((char*)&b, 1);
				if(bigEndianSystem == true) {
					b = Shared::PlatformByteOrder::fromCommonEndian(b);
				}

				in.read((char*)&g, 1);
				if(bigEndianSystem == true) {
					g = Shared::PlatformByteOrder::fromCommonEndian(g);
				}

				in.read((char*)&r, 1);
				if(bigEndianSystem == true) {
					r = Shared::PlatformByteOrder::fromCommonEndian(r);
				}

				if(fileComponents==4){
					in.read((char*)&a, 1);
					if(bigEndianSystem == true) {
						a = Shared::PlatformByteOrder::fromCommonEndian(a);
					}

				} else {
					a= 255;
				}
				l= (r+g+b)/3;
			}
			//if (!in.good()) {
			//	return NULL;
			//}

			switch(picComponents){
			case 1:
				pixels[i]= l;
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
				pixels[i+3]= a;
			break;
			}
		}
		/*for(int i = 0; i < w*h*picComponents; ++i) {
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

//	}
//	catch(exception& ex) {
//		char szBuf[8096]="";
//		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
//		throw megaglest_runtime_error(szBuf);
//	}

	return ret;
}


TGAReader::TGAReader(): FileReader<Pixmap2D>(getExtensionStrings()) {}

Pixmap2D* TGAReader::read(ifstream& in, const string& path, Pixmap2D* ret) const {
	//printf("In [%s] line: %d\n",__FILE__,__LINE__);
	//try {
		//read header
		TargaFileHeader fileHeader;
		in.read((char*)&fileHeader, sizeof(TargaFileHeader));
		static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
		if(bigEndianSystem == true) {
			fileHeader.bitsPerPixel = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.bitsPerPixel);
			fileHeader.colourMapDepth = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapDepth);
			fileHeader.colourMapLength = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapDepth);
			fileHeader.colourMapOrigin = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapOrigin);
			fileHeader.colourMapType = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.colourMapType);
			fileHeader.dataTypeCode = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.dataTypeCode);
			fileHeader.height = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.height);
			fileHeader.idLength = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.idLength);
			fileHeader.imageDescriptor = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.imageDescriptor);
			fileHeader.width = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.width);
		}

		if (!in.good()) {
			throw megaglest_runtime_error(path + " could not be read",true);
		}

		//check that we can load this tga file
		if(fileHeader.idLength!=0){
			throw megaglest_runtime_error(path + ": id field is not 0",true);
		}

		if(fileHeader.dataTypeCode!=tgaUncompressedRgb && fileHeader.dataTypeCode!=tgaUncompressedBw){
			throw megaglest_runtime_error(path + ": only uncompressed BW and RGB targa images are supported",true);
		}

		//check bits per pixel
		if(fileHeader.bitsPerPixel!=8 && fileHeader.bitsPerPixel!=24 && fileHeader.bitsPerPixel!=32){
			throw megaglest_runtime_error(path + ": only 8, 24 and 32 bit targa images are supported",true);
		}

		const int h = fileHeader.height;
		const int w = fileHeader.width;
		const int fileComponents= fileHeader.bitsPerPixel/8;
		const int picComponents = (ret->getComponents()==-1)?fileComponents:ret->getComponents();
		//std::cout << "TGA-Components: Pic: " << picComponents << " old: " << (ret->getComponents()) << " File: " << fileComponents << std::endl;
		ret->init(w,h,picComponents);
		uint8* pixels = ret->getPixels();
		//read file
		for(int i=0; i<h*w*picComponents; i+=picComponents){
			uint8 r=0, g=0, b=0, a=0, l=0;

			if(fileComponents==1){
				in.read((char*)&l,1);
				if(bigEndianSystem == true) {
					l = Shared::PlatformByteOrder::fromCommonEndian(l);
				}

				r= l;
				g= l;
				b= l;
				a= 255;
			}
			else{
				in.read((char*)&b, 1);
				if(bigEndianSystem == true) {
					b = Shared::PlatformByteOrder::fromCommonEndian(b);
				}

				in.read((char*)&g, 1);
				if(bigEndianSystem == true) {
					g = Shared::PlatformByteOrder::fromCommonEndian(g);
				}

				in.read((char*)&r, 1);
				if(bigEndianSystem == true) {
					r = Shared::PlatformByteOrder::fromCommonEndian(r);
				}

				if(fileComponents==4){
					in.read((char*)&a, 1);
					if(bigEndianSystem == true) {
						a = Shared::PlatformByteOrder::fromCommonEndian(a);
					}

				} else {
					a= 255;
				}
				l= (r+g+b)/3;
			}
			if (!in.good()) {
				return NULL;
			}

			switch(picComponents){
			case 1:
				pixels[i]= l;
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
				pixels[i+3]= a;
			break;
			}
		}
		/*for(int i = 0; i < w*h*picComponents; ++i) {
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
//	}
//	catch(exception& ex) {
//		//printf("In [%s] line: %d msf [%s]\n",__FILE__,__LINE__,ex.what());
//		//abort();
//		char szBuf[8096]="";
//		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
//		throw megaglest_runtime_error(szBuf);
//	}

	//printf("In [%s] line: %d\n",__FILE__,__LINE__);
	return ret;
}

}} //end namespace
