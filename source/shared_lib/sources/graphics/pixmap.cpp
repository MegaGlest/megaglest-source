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

#include "math_wrapper.h"
#include "pixmap.h"

#include <stdexcept>
#include <cstdio>
#include <cassert>

#include "util.h"
#include "math_util.h"
#include "randomgen.h"
#include "FileReader.h"
#include "ImageReaders.h"
#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <memory>
#include "opengl.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;
using namespace Shared::Graphics::Gl;

namespace Shared{ namespace Graphics{

using namespace Util;

// =====================================================
//	file structs
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

const int tgaUncompressedRgb= 2;
const int tgaUncompressedBw= 3;

void CalculatePixelsCRC(uint8 *pixels,uint64 pixelByteCount, Checksum &crc) {
//	crc = Checksum();
//	for(uint64 i = 0; i < pixelByteCount; ++i) {
//		crc.addByte(pixels[i]);
//	}
}

// =====================================================
//	class PixmapIoTga
// =====================================================

PixmapIoTga::PixmapIoTga() {
	file= NULL;
}

PixmapIoTga::~PixmapIoTga() {
	if(file != NULL) {
		fclose(file);
		file=NULL;
	}
}

void PixmapIoTga::openRead(const string &path) {

	try {
	#ifdef WIN32
		file= _wfopen(utf8_decode(path).c_str(), L"rb");
	#else
		file= fopen(path.c_str(),"rb");
	#endif
		if (file == NULL) {
			throw megaglest_runtime_error("Can't open TGA file: "+ path,true);
		}

		//read header
		TargaFileHeader fileHeader;
		size_t readBytes = fread(&fileHeader, sizeof(TargaFileHeader), 1, file);
		if(readBytes != 1) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
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
		//check that we can load this tga file
		if(fileHeader.idLength != 0) {
			throw megaglest_runtime_error(path + ": id field is not 0",true);
		}

		if(fileHeader.dataTypeCode != tgaUncompressedRgb && fileHeader.dataTypeCode != tgaUncompressedBw) {
			throw megaglest_runtime_error(path + ": only uncompressed BW and RGB targa images are supported",true);
		}

		//check bits per pixel
		if(fileHeader.bitsPerPixel != 8 && fileHeader.bitsPerPixel != 24 && fileHeader.bitsPerPixel !=32) {
			throw megaglest_runtime_error(path + ": only 8, 24 and 32 bit targa images are supported",true);
		}

		h= fileHeader.height;
		w= fileHeader.width;
		components= fileHeader.bitsPerPixel / 8;
	}
	catch(megaglest_runtime_error& ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
		throw megaglest_runtime_error(szBuf,!ex.wantStackTrace());
	}
	catch(exception& ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
		throw megaglest_runtime_error(szBuf);
	}
}

void PixmapIoTga::read(uint8 *pixels) {
	read(pixels, components);
}

void PixmapIoTga::read(uint8 *pixels, int components) {
	try {
		static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();

		for(int i=0; i<h*w*components; i+=components) {
			uint8 r=0, g=0, b=0, a=0, l=0;

			if(this->components == 1) {
				size_t readBytes = fread(&l, 1, 1, file);
				if(readBytes != 1) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
					throw megaglest_runtime_error(szBuf);
				}
				if(bigEndianSystem == true) {
					l = Shared::PlatformByteOrder::fromCommonEndian(l);
				}
				r= l;
				g= l;
				b= l;
				a= 255;
			}
			else {
				size_t readBytes = fread(&b, 1, 1, file);
				if(readBytes != 1) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
					throw megaglest_runtime_error(szBuf);
				}
				if(bigEndianSystem == true) {
					b = Shared::PlatformByteOrder::fromCommonEndian(b);
				}

				readBytes = fread(&g, 1, 1, file);
				if(readBytes != 1) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
					throw megaglest_runtime_error(szBuf);
				}
				if(bigEndianSystem == true) {
					g = Shared::PlatformByteOrder::fromCommonEndian(g);
				}

				readBytes = fread(&r, 1, 1, file);
				if(readBytes != 1) {
					char szBuf[8096]="";
					snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
					throw megaglest_runtime_error(szBuf);
				}
				if(bigEndianSystem == true) {
					r = Shared::PlatformByteOrder::fromCommonEndian(r);
				}

				if(this->components == 4) {
					readBytes = fread(&a, 1, 1, file);
					if(readBytes != 1) {
						char szBuf[8096]="";
						snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
						throw megaglest_runtime_error(szBuf);
					}
					if(bigEndianSystem == true) {
						a = Shared::PlatformByteOrder::fromCommonEndian(a);
					}

				}
				else {
					a= 255;
				}
				l= (r+g+b)/3;
			}

			switch(components) {
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
	}
	catch(megaglest_runtime_error& ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
		throw megaglest_runtime_error(szBuf,!ex.wantStackTrace());
	}
	catch(exception& ex) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error in [%s] on line: %d msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__LINE__,ex.what());
		throw megaglest_runtime_error(szBuf);
	}
}

void PixmapIoTga::openWrite(const string &path, int w, int h, int components) {
    this->w= w;
	this->h= h;
	this->components= components;

#ifdef WIN32
	file= _wfopen(utf8_decode(path).c_str(), L"wb");
#else
    file= fopen(path.c_str(),"wb");
#endif
	if (file == NULL) {
		throw megaglest_runtime_error("Can't open TGA file: "+ path,true);
	}

	TargaFileHeader fileHeader;
	memset(&fileHeader, 0, sizeof(TargaFileHeader));
	fileHeader.dataTypeCode= components==1? tgaUncompressedBw: tgaUncompressedRgb;
	fileHeader.bitsPerPixel= components*8;
	fileHeader.width= w;
	fileHeader.height= h;
	fileHeader.imageDescriptor= components==4? 8: 0;

	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		fileHeader.bitsPerPixel = Shared::PlatformByteOrder::toCommonEndian(fileHeader.bitsPerPixel);
		fileHeader.colourMapDepth = Shared::PlatformByteOrder::toCommonEndian(fileHeader.colourMapDepth);
		fileHeader.colourMapLength = Shared::PlatformByteOrder::toCommonEndian(fileHeader.colourMapDepth);
		fileHeader.colourMapOrigin = Shared::PlatformByteOrder::toCommonEndian(fileHeader.colourMapOrigin);
		fileHeader.colourMapType = Shared::PlatformByteOrder::toCommonEndian(fileHeader.colourMapType);
		fileHeader.dataTypeCode = Shared::PlatformByteOrder::toCommonEndian(fileHeader.dataTypeCode);
		fileHeader.height = Shared::PlatformByteOrder::toCommonEndian(fileHeader.height);
		fileHeader.idLength = Shared::PlatformByteOrder::toCommonEndian(fileHeader.idLength);
		fileHeader.imageDescriptor = Shared::PlatformByteOrder::toCommonEndian(fileHeader.imageDescriptor);
		fileHeader.width = Shared::PlatformByteOrder::toCommonEndian(fileHeader.width);
	}

	fwrite(&fileHeader, sizeof(TargaFileHeader), 1, file);
}

void PixmapIoTga::write(uint8 *pixels) {
	if(components == 1) {

		static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
		if(bigEndianSystem == true) {
			Shared::PlatformByteOrder::toEndianTypeArray<uint8>(pixels,h*w);
		}

		fwrite(pixels, h*w, 1, file);
	}
	else {
		static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
		if(bigEndianSystem == true) {
			Shared::PlatformByteOrder::toEndianTypeArray<uint8>(pixels,h*w*components);
		}

		for(int i=0; i<h*w*components; i+=components) {
			fwrite(&pixels[i+2], 1, 1, file);
			fwrite(&pixels[i+1], 1, 1, file);
			fwrite(&pixels[i], 1, 1, file);
			if(components==4){
				fwrite(&pixels[i+3], 1, 1, file);
			}
		}
	}
}

// =====================================================
//	class PixmapIoBmp
// =====================================================

PixmapIoBmp::PixmapIoBmp() {
	file= NULL;
}

PixmapIoBmp::~PixmapIoBmp() {
	if(file!=NULL){
		fclose(file);
		file=NULL;
	}
}

void PixmapIoBmp::openRead(const string &path){
#ifdef WIN32
	file= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
    file= fopen(path.c_str(),"rb");
#endif
	if (file==NULL){
		throw megaglest_runtime_error("Can't open BMP file: "+ path,true);
	}

	//read file header
    BitmapFileHeader fileHeader;
    size_t readBytes = fread(&fileHeader, sizeof(BitmapFileHeader), 1, file);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		fileHeader.offsetBits = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.offsetBits);
		fileHeader.reserved1 = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.reserved1);
		fileHeader.reserved2 = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.reserved2);
		fileHeader.size = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.size);
		fileHeader.type1 = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.type1);
		fileHeader.type2 = Shared::PlatformByteOrder::fromCommonEndian(fileHeader.type2);
	}

	if(fileHeader.type1!='B' || fileHeader.type2!='M'){
		throw megaglest_runtime_error(path +" is not a bitmap",true);
	}

	//read info header
	BitmapInfoHeader infoHeader;
	readBytes = fread(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	if(bigEndianSystem == true) {
		infoHeader.bitCount = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.bitCount);
		infoHeader.clrImportant = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.clrImportant);
		infoHeader.clrUsed = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.clrUsed);
		infoHeader.compression = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.compression);
		infoHeader.height = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.height);
		infoHeader.planes = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.planes);
		infoHeader.size = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.size);
		infoHeader.sizeImage = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.sizeImage);
		infoHeader.width = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.width);
		infoHeader.xPelsPerMeter = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.xPelsPerMeter);
		infoHeader.yPelsPerMeter = Shared::PlatformByteOrder::fromCommonEndian(infoHeader.yPelsPerMeter);
	}
	if(infoHeader.bitCount!=24){
        throw megaglest_runtime_error(path+" is not a 24 bit bitmap",true);
	}

    h= infoHeader.height;
    w= infoHeader.width;
	components= 3;
}

void PixmapIoBmp::read(uint8 *pixels) {
	read(pixels, 3);
}

void PixmapIoBmp::read(uint8 *pixels, int components) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();

	for(int i=0; i<h*w*components; i+=components) {
		uint8 r, g, b;
		size_t readBytes = fread(&b, 1, 1, file);
		if(readBytes != 1) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		if(bigEndianSystem == true) {
			b = Shared::PlatformByteOrder::fromCommonEndian(b);
		}
		readBytes = fread(&g, 1, 1, file);
		if(readBytes != 1) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		if(bigEndianSystem == true) {
			g = Shared::PlatformByteOrder::fromCommonEndian(g);
		}

		readBytes = fread(&r, 1, 1, file);
		if(readBytes != 1) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		if(bigEndianSystem == true) {
			r = Shared::PlatformByteOrder::fromCommonEndian(r);
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
}

void PixmapIoBmp::openWrite(const string &path, int w, int h, int components) {
    this->w= w;
	this->h= h;
	this->components= components;

#ifdef WIN32
	file= _wfopen(utf8_decode(path).c_str(), L"wb");
#else
	file= fopen(path.c_str(),"wb");
#endif
	if (file == NULL) {
		throw megaglest_runtime_error("Can't open BMP file for writing: "+ path);
	}

	BitmapFileHeader fileHeader;
    fileHeader.type1='B';
	fileHeader.type2='M';
	fileHeader.offsetBits=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader);
	fileHeader.size=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader)+3*h*w;
	fileHeader.reserved1 = 0;
	fileHeader.reserved2 = 0;

	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		fileHeader.offsetBits = Shared::PlatformByteOrder::toCommonEndian(fileHeader.offsetBits);
		fileHeader.reserved1 = Shared::PlatformByteOrder::toCommonEndian(fileHeader.reserved1);
		fileHeader.reserved2 = Shared::PlatformByteOrder::toCommonEndian(fileHeader.reserved2);
		fileHeader.size = Shared::PlatformByteOrder::toCommonEndian(fileHeader.size);
		fileHeader.type1 = Shared::PlatformByteOrder::toCommonEndian(fileHeader.type1);
		fileHeader.type2 = Shared::PlatformByteOrder::toCommonEndian(fileHeader.type2);
	}

    fwrite(&fileHeader, sizeof(BitmapFileHeader), 1, file);

	//info header
	BitmapInfoHeader infoHeader;
	infoHeader.bitCount=24;
	infoHeader.clrImportant=0;
	infoHeader.clrUsed=0;
	infoHeader.compression=0;
	infoHeader.height= h;
	infoHeader.planes=1;
	infoHeader.size=sizeof(BitmapInfoHeader);
	infoHeader.sizeImage=0;
	infoHeader.width= w;
	infoHeader.xPelsPerMeter= 0;
	infoHeader.yPelsPerMeter= 0;

	if(bigEndianSystem == true) {
		infoHeader.bitCount = Shared::PlatformByteOrder::toCommonEndian(infoHeader.bitCount);
		infoHeader.clrImportant = Shared::PlatformByteOrder::toCommonEndian(infoHeader.clrImportant);
		infoHeader.clrUsed = Shared::PlatformByteOrder::toCommonEndian(infoHeader.clrUsed);
		infoHeader.compression = Shared::PlatformByteOrder::toCommonEndian(infoHeader.compression);
		infoHeader.height = Shared::PlatformByteOrder::toCommonEndian(infoHeader.height);
		infoHeader.planes = Shared::PlatformByteOrder::toCommonEndian(infoHeader.planes);
		infoHeader.size = Shared::PlatformByteOrder::toCommonEndian(infoHeader.size);
		infoHeader.sizeImage = Shared::PlatformByteOrder::toCommonEndian(infoHeader.sizeImage);
		infoHeader.width = Shared::PlatformByteOrder::toCommonEndian(infoHeader.width);
		infoHeader.xPelsPerMeter = Shared::PlatformByteOrder::toCommonEndian(infoHeader.xPelsPerMeter);
		infoHeader.yPelsPerMeter = Shared::PlatformByteOrder::toCommonEndian(infoHeader.yPelsPerMeter);
	}

	fwrite(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
}

void PixmapIoBmp::write(uint8 *pixels) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		Shared::PlatformByteOrder::toEndianTypeArray<uint8>(pixels,h*w*components);
	}

    for (int i=0; i<h*w*components; i+=components){
        fwrite(&pixels[i+2], 1, 1, file);
		fwrite(&pixels[i+1], 1, 1, file);
		fwrite(&pixels[i], 1, 1, file);
    }
}

// =====================================================
//	class PixmapIoPng
// =====================================================

PixmapIoPng::PixmapIoPng() {
	file= NULL;
}

PixmapIoPng::~PixmapIoPng() {
	if(file!=NULL){
		fclose(file);
		file=NULL;
	}
}

void PixmapIoPng::openRead(const string &path) {
	throw megaglest_runtime_error("PixmapIoPng::openRead not implemented!");
}

void PixmapIoPng::read(uint8 *pixels) {
	throw megaglest_runtime_error("PixmapIoPng::read not implemented!");
}

void PixmapIoPng::read(uint8 *pixels, int components) {

	throw megaglest_runtime_error("PixmapIoPng::read #2 not implemented!");
}

void PixmapIoPng::openWrite(const string &path, int w, int h, int components) {
    this->path = path;
	this->w= w;
	this->h= h;
	this->components= components;

#ifdef WIN32
	file= _wfopen(utf8_decode(path).c_str(), L"wb");
#else
	file= fopen(path.c_str(),"wb");
#endif
	if (file == NULL) {
		throw megaglest_runtime_error("Can't open PNG file for writing: "+ path);
	}
}

void PixmapIoPng::write(uint8 *pixels) {
    // initialize stuff
    png_bytep *imrow = new png_bytep[h];
	//png_bytep *imrow = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for(int i = 0; i < h; ++i) {
        imrow[i] = pixels + (h-1-i) * w * components;
    }

    png_structp imgp = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
    png_infop infop = png_create_info_struct(imgp);
    png_init_io(imgp, file);

    int color_type = PNG_COLOR_TYPE_RGB;
    if(components == 4) {
    	color_type = PNG_COLOR_TYPE_RGBA;
    }
    png_set_IHDR(imgp, infop, w, h,
		 8, color_type, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // write file
    png_write_info(imgp, infop);
    png_write_image(imgp, imrow);
    png_write_end(imgp, NULL);

    delete [] imrow;
}

// =====================================================
//	class PixmapIoJpg
// =====================================================

PixmapIoJpg::PixmapIoJpg() {
	file= NULL;
}

PixmapIoJpg::~PixmapIoJpg() {
	if(file!=NULL){
		fclose(file);
		file=NULL;
	}
}

void PixmapIoJpg::openRead(const string &path) {
	throw megaglest_runtime_error("PixmapIoJpg::openRead not implemented!");
}

void PixmapIoJpg::read(uint8 *pixels) {
	throw megaglest_runtime_error("PixmapIoJpg::read not implemented!");
}

void PixmapIoJpg::read(uint8 *pixels, int components) {

	throw megaglest_runtime_error("PixmapIoJpg::read #2 not implemented!");
}

void PixmapIoJpg::openWrite(const string &path, int w, int h, int components) {
    this->path = path;
	this->w= w;
	this->h= h;
	this->components= components;

#ifdef WIN32
	file= _wfopen(utf8_decode(path).c_str(), L"wb");
#else
	file= fopen(path.c_str(),"wb");
#endif
	if (file == NULL) {
		throw megaglest_runtime_error("Can't open JPG file for writing: "+ path);
	}
}

void PixmapIoJpg::write(uint8 *pixels) {
	/*
	* alpha channel is not supported for jpeg. strip it.
	*/
	unsigned char * tmpbytes = NULL;
	if(components == 4) {
		int n = w * h;
		unsigned char *dst = tmpbytes = (unsigned char *) malloc(n*3);
		if(dst != NULL) {
			const unsigned char *src = pixels;
			for (int i = 0; i < n; i++) {
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				src++;
			}
		}
		components = 3;
	}

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	/* this is a pointer to one row of image data */
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	/* Setting the parameters of the output file here */
	cinfo.image_width = w;
	cinfo.image_height = h;
	cinfo.input_components = 3; // no alpha channel for jpeg
	cinfo.in_color_space = JCS_RGB;
	/* default compression parameters, we shouldn't be worried about these */
	jpeg_set_defaults( &cinfo );
	/* Now do the compression .. */
	jpeg_start_compress( &cinfo, TRUE );

	/* use stripped alpha channel bytes */
	if(tmpbytes) {
		pixels = tmpbytes;
	}

	// OpenGL writes from bottom to top.
    // libjpeg goes from top to bottom.
    // flip lines.
	uint8 *flip = (uint8 *)malloc(sizeof(uint8) * w * h * 3);

	if(pixels != NULL && flip != NULL) {
		for (int y = 0;y < h; ++y) {
			for (int x = 0;x < w; ++x) {
				flip[(y * w + x) * 3] = pixels[((h - 1 - y) * w + x) * 3];
				flip[(y * w + x) * 3 + 1] = pixels[((h - 1 - y) * w + x) * 3 + 1];
				flip[(y * w + x) * 3 + 2] = pixels[((h - 1 - y) * w + x) * 3 + 2];
			}
		}
	
		/* like reading a file, this time write one row at a time */
		while( cinfo.next_scanline < cinfo.image_height ) {
			row_pointer[0] = &flip[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
			jpeg_write_scanlines( &cinfo, row_pointer, 1 );
		}
	}
	if (tmpbytes) {
		free(tmpbytes);
		tmpbytes=NULL;
	}
	if(flip) {
		free(flip);
		flip=NULL;
	}

	/* similar to read file, clean up after we're done compressing */
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	//fclose( outfile );
	/* success code is 1! */
}

// =====================================================
//	class Pixmap1D
// =====================================================


// ===================== PUBLIC ========================

Pixmap1D::Pixmap1D() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap1D::Pixmap1D(int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	init(components);
}

Pixmap1D::Pixmap1D(int w, int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	init(w, components);
}

void Pixmap1D::init(int components){
	this->w= -1;
	this->components= components;
	pixels= NULL;
	CalculatePixelsCRC(pixels,0, crc);
}

void Pixmap1D::init(int w, int components){
	this->w= w;
	this->components= components;
	pixels= new uint8[(std::size_t)getPixelByteCount()];
	CalculatePixelsCRC(pixels,0, crc);
}

uint64 Pixmap1D::getPixelByteCount() const {
	return (w * components);
}

void Pixmap1D::deletePixels() {
	delete [] pixels;
	pixels = NULL;
}

Pixmap1D::~Pixmap1D(){
	deletePixels();
}

void Pixmap1D::load(const string &path) {
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp") {
		loadBmp(path);
	}
	else if(extension=="tga") {
		loadTga(path);
	}
	else {
		throw megaglest_runtime_error("Unknown pixmap extension: " + extension);
	}
	this->path = path;
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap1D::loadBmp(const string &path) {
    this->path = path;

	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	if(plb.getH()==1) {
		w= plb.getW();
	}
	else if(plb.getW()==1) {
		w= plb.getH();
	}
	else {
		throw megaglest_runtime_error("One of the texture dimensions must be 1");
	}

	if(components == -1) {
		components= 3;
	}
	if(pixels == NULL) {
		pixels= new uint8[(std::size_t)getPixelByteCount()];
	}

	//data
	plb.read(pixels, components);
}

void Pixmap1D::loadTga(const string &path) {
    this->path = path;

	PixmapIoTga plt;
	plt.openRead(path);

	//init
	if(plt.getH()==1) {
		w= plt.getW();
	}
	else if(plt.getW()==1) {
		w= plt.getH();
	}
	else {
		throw megaglest_runtime_error("One of the texture dimensions must be 1");
	}

	int fileComponents= plt.getComponents();

	if(components == -1) {
		components= fileComponents;
	}
	if(pixels == NULL) {
		pixels= new uint8[(std::size_t)getPixelByteCount()];
	}

	//read data
	plt.read(pixels, components);
}

// =====================================================
//	class Pixmap2D
// =====================================================

// ===================== PUBLIC ========================

Pixmap2D::Pixmap2D() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
    h= -1;
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap2D::Pixmap2D(int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
    h= -1;
    w= -1;
	this->components= -1;
    pixels= NULL;

	init(components);
}

Pixmap2D::Pixmap2D(int w, int h, int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
    this->h= 0;
    this->w= -1;
    this->components= -1;
    pixels= NULL;

	init(w, h, components);
}

void Pixmap2D::init(int components) {
	this->w= -1;
	this->h= -1;
	this->components= components;
	deletePixels();
	pixels= NULL;
	CalculatePixelsCRC(pixels,0, crc);
}

void Pixmap2D::init(int w, int h, int components) {
	this->w= w;
	this->h= h;
	this->components= components;
	deletePixels();

	if(getPixelByteCount() <= 0 || (h <= 0 || w <= 0 || components <= 0)) {
		char szBuf[8096];
		snprintf(szBuf,8096,"Invalid pixmap dimensions for [%s], h = %d, w = %d, components = %d\n",path.c_str(),h,w,components);
		throw megaglest_runtime_error(szBuf);
	}
	pixels= new uint8[(std::size_t)getPixelByteCount()];
	CalculatePixelsCRC(pixels,0, crc);
}

uint64 Pixmap2D::getPixelByteCount() const {
	return (h * w * components);
}

void Pixmap2D::deletePixels() {
	if(pixels) {
		delete [] pixels;
		pixels = NULL;
	}
}

Pixmap2D::~Pixmap2D() {
	deletePixels();
}

void Pixmap2D::Scale(int format, int newW, int newH) {
	int useComponents = this->getComponents();
	int originalW = w;
	int originalH = h;
	uint8 *newpixels= new uint8[newW * newH * useComponents];

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	int error = gluScaleImage(	format,
				w, h, GL_UNSIGNED_BYTE, pixels,
				newW, newH, GL_UNSIGNED_BYTE, newpixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if(error == GL_NO_ERROR) {
		init(newW,newH,this->components);
		pixels = newpixels;

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Scaled image from [%d x %d] to [%d x %d]\n",originalW,originalH,w,h);
	}
	else {
		const char *errorString= reinterpret_cast<const char*>(gluErrorString(error));
		printf("ERROR Scaling image from [%d x %d] to [%d x %d] error: %d [%s]\n",originalW,originalH,w,h,error,errorString);

		GLenum glErr = error;
		assertGlWithErrorNumber(glErr);
	}

	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

Pixmap2D* Pixmap2D::loadPath(const string& path) {
	//printf("Loading Pixmap2D [%s]\n",path.c_str());

	Pixmap2D *pixmap = FileReader<Pixmap2D>::readPath(path);
	if(pixmap != NULL) {
		pixmap->path = path;
		CalculatePixelsCRC(pixmap->pixels,pixmap->getPixelByteCount(), pixmap->crc);
	}
	return pixmap;
}

void Pixmap2D::load(const string &path) {
	//printf("Loading Pixmap2D [%s]\n",path.c_str());

	FileReader<Pixmap2D>::readPath(path,this);
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
	this->path = path;
}

void Pixmap2D::save(const string &path) {
	string extension= path.substr(path.find_last_of('.')+1);
	if(toLower(extension) == "bmp") {
		saveBmp(path);
	}
	else if(toLower(extension) == "tga") {
		saveTga(path);
	}
	else if(toLower(extension) == "jpg") {
		saveJpg(path);
	}
	else if(toLower(extension) == "png") {
		savePng(path);
	}
	else {
		throw megaglest_runtime_error("Unknown pixmap extension: " + extension);
	}
}

void Pixmap2D::saveBmp(const string &path) {
	PixmapIoBmp psb;
	psb.openWrite(path, w, h, components);
	psb.write(pixels);
}

void Pixmap2D::saveTga(const string &path) {
	PixmapIoTga pst;
	pst.openWrite(path, w, h, components);
	pst.write(pixels);
}

void Pixmap2D::saveJpg(const string &path) {
	PixmapIoJpg pst;
	pst.openWrite(path, w, h, components);
	pst.write(pixels);
}

void Pixmap2D::savePng(const string &path) {
	PixmapIoPng pst;
	pst.openWrite(path, w, h, components);
	pst.write(pixels);
}

void Pixmap2D::getPixel(int x, int y, uint8 *value) const {
	for(int i=0; i<components; ++i){
		value[i]= pixels[(w*y+x)*components+i];
	}
}

void Pixmap2D::getPixel(int x, int y, float32 *value) const {
	for(int i=0; i<components; ++i) {
		value[i]= pixels[(w*y+x)*components+i]/255.f;
	}
}

void Pixmap2D::getComponent(int x, int y, int component, uint8 &value) const {
	value= pixels[(w*y+x)*components+component];
}

void Pixmap2D::getComponent(int x, int y, int component, float32 &value) const {
	value= pixels[(w*y+x)*components+component]/255.f;
}

//vector get
Vec4f Pixmap2D::getPixel4f(int x, int y) const {
	Vec4f v(0.f);
	for(int i=0; i<components && i<4; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

Vec3f Pixmap2D::getPixel3f(int x, int y) const {
	Vec3f v(0.f);
	for(int i=0; i<components && i<3; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

float Pixmap2D::getPixelf(int x, int y) const {
	int index = (w*y+x)*components;
	return pixels[index]/255.f;
}

float Pixmap2D::getComponentf(int x, int y, int component) const {
	float c=0;
	getComponent(x, y, component, c);
	return c;
}

void Pixmap2D::setPixel(int x, int y, const uint8 *value) {
	for(int i=0; i<components; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= value[i];
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setPixel(int x, int y, const float32 *value) {
	for(int i=0; i<components; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(value[i]*255.f);
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setComponent(int x, int y, int component, uint8 value) {
	int index = (w*y+x)*components+component;
	pixels[index] = value;
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setComponent(int x, int y, int component, float32 value) {
	int index = (w*y+x)*components+component;
	pixels[index]= static_cast<uint8>(value*255.f);
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

//vector set
void Pixmap2D::setPixel(int x, int y, const Vec3f &p) {
	for(int i = 0; i < components  && i < 3; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setPixel(int x, int y, const Vec4f &p) {
	for(int i = 0; i < components && i < 4; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setPixel(int x, int y, float p) {
	int index = (w*y+x)*components;
	pixels[index]= static_cast<uint8>(p*255.f);
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setPixels(const uint8 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setPixels(const float32 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setComponents(int component, uint8 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::setComponents(int component, float32 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

float splatDist(Vec2i a, Vec2i b){
	return (max(abs(a.x-b.x),abs(a.y- b.y)) + 3.f*a.dist(b))/4.f;
}

void Pixmap2D::splat(const Pixmap2D *leftUp, const Pixmap2D *rightUp, const Pixmap2D *leftDown, const Pixmap2D *rightDown){

	RandomGen random;

	assert(components==3 || components==4);

	if(
		!doDimensionsAgree(leftUp) ||
		!doDimensionsAgree(rightUp) ||
		!doDimensionsAgree(leftDown) ||
		!doDimensionsAgree(rightDown))
	{
		throw megaglest_runtime_error("Pixmap2D::splat: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){

			float avg= (w+h)/2.f;

			float distLu= splatDist(Vec2i(i, j), Vec2i(0, 0));
			float distRu= splatDist(Vec2i(i, j), Vec2i(w, 0));
			float distLd= splatDist(Vec2i(i, j), Vec2i(0, h));
			float distRd= splatDist(Vec2i(i, j), Vec2i(w, h));

			const float powFactor= 2.0f;
#ifdef USE_STREFLOP
			distLu= streflop::pow(static_cast<streflop::Simple>(distLu), static_cast<streflop::Simple>(powFactor));
			distRu= streflop::pow(static_cast<streflop::Simple>(distRu), static_cast<streflop::Simple>(powFactor));
			distLd= streflop::pow(static_cast<streflop::Simple>(distLd), static_cast<streflop::Simple>(powFactor));
			distRd= streflop::pow(static_cast<streflop::Simple>(distRd), static_cast<streflop::Simple>(powFactor));
			avg= streflop::pow(static_cast<streflop::Simple>(avg), static_cast<streflop::Simple>(powFactor));
#else
			distLu= pow(distLu, powFactor);
			distRu= pow(distRu, powFactor);
			distLd= pow(distLd, powFactor);
			distRd= pow(distRd, powFactor);
			avg= pow(avg, powFactor);
#endif
			float lu= distLu>avg? 0: ((avg-distLu))*random.randRange(0.5f, 1.0f);
			float ru= distRu>avg? 0: ((avg-distRu))*random.randRange(0.5f, 1.0f);
			float ld= distLd>avg? 0: ((avg-distLd))*random.randRange(0.5f, 1.0f);
			float rd= distRd>avg? 0: ((avg-distRd))*random.randRange(0.5f, 1.0f);

			float total= lu+ru+ld+rd;

			Vec4f pix= (leftUp->getPixel4f(i, j)*lu+
				rightUp->getPixel4f(i, j)*ru+
				leftDown->getPixel4f(i, j)*ld+
				rightDown->getPixel4f(i, j)*rd)*(1.0f/total);

			setPixel(i, j, pix);
		}
	}
}

void Pixmap2D::lerp(float t, const Pixmap2D *pixmap1, const Pixmap2D *pixmap2){
	if(
		!doDimensionsAgree(pixmap1) ||
		!doDimensionsAgree(pixmap2))
	{
		throw megaglest_runtime_error("Pixmap2D::lerp: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, pixmap1->getPixel4f(i, j).lerp(t, pixmap2->getPixel4f(i, j)));
		}
	}
}

void Pixmap2D::copy(const Pixmap2D *sourcePixmap){

	assert(components==sourcePixmap->getComponents());

	if(w!=sourcePixmap->getW() || h!=sourcePixmap->getH()){
		throw megaglest_runtime_error("Pixmap2D::copy() dimensions must agree");
	}
	memcpy(pixels, sourcePixmap->getPixels(), w*h*sourcePixmap->getComponents());
	this->path = sourcePixmap->path;
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap2D::subCopy(int x, int y, const Pixmap2D *sourcePixmap){
	assert(components==sourcePixmap->getComponents());

	if(w<sourcePixmap->getW() && h<sourcePixmap->getH()){
		throw megaglest_runtime_error("Pixmap2D::subCopy(), bad dimensions");
	}

	uint8 *pixel= new uint8[components];

	for(int i=0; i<sourcePixmap->getW(); ++i){
		for(int j=0; j<sourcePixmap->getH(); ++j){
			sourcePixmap->getPixel(i, j, pixel);
			setPixel(i+x, j+y, pixel);
		}
	}
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);

	delete [] pixel;
}

bool Pixmap2D::doDimensionsAgree(const Pixmap2D *pixmap){
	return pixmap->getW() == w && pixmap->getH() == h;
}

// =====================================================
//	class Pixmap3D
// =====================================================

Pixmap3D::Pixmap3D() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	w= -1;
	h= -1;
	d= -1;
	components= -1;
	pixels = NULL;
	slice=0;
}

Pixmap3D::Pixmap3D(int w, int h, int d, int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
	pixels = NULL;
	slice=0;
	init(w, h, d, components);
}

Pixmap3D::Pixmap3D(int d, int components) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
	pixels = NULL;
	slice=0;
	init(d, components);
}

void Pixmap3D::init(int w, int h, int d, int components){
	this->w= w;
	this->h= h;
	this->d= d;
	this->components= components;
	pixels= new uint8[(std::size_t)getPixelByteCount()];
	CalculatePixelsCRC(pixels,0, crc);
}

uint64 Pixmap3D::getPixelByteCount() const {
	return (h * w * d * components);
}

void Pixmap3D::init(int d, int components){
	this->w= -1;
	this->h= -1;
	this->d= d;
	this->components= components;
	pixels= NULL;
	CalculatePixelsCRC(pixels,0, crc);
}

void Pixmap3D::init(int components) {
	this->w= -1;
	this->h= -1;
	this->d= -1;
	this->components= components;
	pixels= NULL;
	CalculatePixelsCRC(pixels,0, crc);
}

void Pixmap3D::deletePixels() {
	delete [] pixels;
	pixels = NULL;
}

Pixmap3D::~Pixmap3D() {
	deletePixels();
}

void Pixmap3D::loadSlice(const string &path, int slice) {
	this->slice = slice;
	string extension= path.substr(path.find_last_of('.') + 1);
	if(extension == "png") {
		loadSlicePng(path, slice);
	}
	else if(extension == "bmp") {
		loadSliceBmp(path, slice);
	}
	else if(extension == "tga") {
		loadSliceTga(path, slice);
	}
	else {
		throw megaglest_runtime_error("Unknown pixmap extension: "+extension);
	}
	this->path = path;
	CalculatePixelsCRC(pixels,getPixelByteCount(), crc);
}

void Pixmap3D::loadSlicePng(const string &path, int slice) {
	this->path = path;

	//deletePixels();
	//Pixmap3D *pixmap = FileReader<Pixmap3D>::readPath(path,this);
	FileReader<Pixmap3D>::readPath(path,this);
	//printf("Loading 3D pixmap PNG [%s] pixmap [%p] this [%p]\n",path.c_str(),pixmap, this);
}

void Pixmap3D::loadSliceBmp(const string &path, int slice){
	this->path = path;

	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	w= plb.getW();
	h= plb.getH();
	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[(std::size_t)getPixelByteCount()];
	}

	//data
	plb.read(&pixels[slice*w*h*components], components);
}

void Pixmap3D::loadSliceTga(const string &path, int slice){
	this->path = path;

	//deletePixels();
	FileReader<Pixmap3D>::readPath(path,this);
	//printf("Loading 3D pixmap TGA [%s] this [%p]\n",path.c_str(),this);

/*
	PixmapIoTga plt;
	plt.openRead(path);

	//header
	int fileComponents= plt.getComponents();

	//init
	w= plt.getW();
	h= plt.getH();
	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[(std::size_t)getPixelByteCount()];
	}

	//read data
	plt.read(&pixels[slice*w*h*components], components);
*/
}

// =====================================================
//	class PixmapCube
// =====================================================
PixmapCube::PixmapCube() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
}

PixmapCube::~PixmapCube() {

}

void PixmapCube::init(int w, int h, int components) {
	for(int i=0; i<6; ++i) {
		faces[i].init(w, h, components);
	}
}

void PixmapCube::init(int components) {
	for(int i=0; i<6; ++i) {
		faces[i].init(components);
	}
}

uint64 PixmapCube::getPixelByteCount() const {
	uint64 result = 0;
	for(int i=0; i<6; ++i) {
		result += faces[i].getPixelByteCount();
	}

	return result;
}

//load & save
void PixmapCube::loadFace(const string &path, int face) {
	this->path[face] = path;

	faces[face].load(path);
}

void PixmapCube::deletePixels() {
	for(int i=0; i<6; ++i){
		faces[i].deletePixels();
	}
}


}}//end namespace
