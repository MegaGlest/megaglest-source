// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
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

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

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
	file= fopen(path.c_str(),"rb");
	if (file == NULL) {
		throw runtime_error("Can't open TGA file: "+ path);
	}

	//read header
	TargaFileHeader fileHeader;
	size_t readBytes = fread(&fileHeader, sizeof(TargaFileHeader), 1, file);

	//check that we can load this tga file
	if(fileHeader.idLength != 0) {
		throw runtime_error(path + ": id field is not 0");
	}

	if(fileHeader.dataTypeCode != tgaUncompressedRgb && fileHeader.dataTypeCode != tgaUncompressedBw) {
		throw runtime_error(path + ": only uncompressed BW and RGB targa images are supported");
	}

	//check bits per pixel
	if(fileHeader.bitsPerPixel != 8 && fileHeader.bitsPerPixel != 24 && fileHeader.bitsPerPixel !=32) {
		throw runtime_error(path + ": only 8, 24 and 32 bit targa images are supported");
	}

	h= fileHeader.height;
    w= fileHeader.width;
	components= fileHeader.bitsPerPixel / 8;
}

void PixmapIoTga::read(uint8 *pixels) {
	read(pixels, components);
}

void PixmapIoTga::read(uint8 *pixels, int components) {
	for(int i=0; i<h*w*components; i+=components) {
		uint8 r=0, g=0, b=0, a=0, l=0;

		if(this->components == 1) {
			size_t readBytes = fread(&l, 1, 1, file);
			r= l;
			g= l;
			b= l;
			a= 255;
		}
		else {
			size_t readBytes = fread(&b, 1, 1, file);
			readBytes = fread(&g, 1, 1, file);
			readBytes = fread(&r, 1, 1, file);
			if(this->components == 4) {
				readBytes = fread(&a, 1, 1, file);
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

void PixmapIoTga::openWrite(const string &path, int w, int h, int components) {
    this->w= w;
	this->h= h;
	this->components= components;

    file= fopen(path.c_str(),"wb");
	if (file == NULL) {
		throw runtime_error("Can't open TGA file: "+ path);
	}

	TargaFileHeader fileHeader;
	memset(&fileHeader, 0, sizeof(TargaFileHeader));
	fileHeader.dataTypeCode= components==1? tgaUncompressedBw: tgaUncompressedRgb;
	fileHeader.bitsPerPixel= components*8;
	fileHeader.width= w;
	fileHeader.height= h;
	fileHeader.imageDescriptor= components==4? 8: 0;

	fwrite(&fileHeader, sizeof(TargaFileHeader), 1, file);
}

void PixmapIoTga::write(uint8 *pixels) {
	if(components == 1) {
		fwrite(pixels, h*w, 1, file);
	}
	else {
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
    file= fopen(path.c_str(),"rb");
	if (file==NULL){
		throw runtime_error("Can't open BMP file: "+ path);
	}

	//read file header
    BitmapFileHeader fileHeader;
    size_t readBytes = fread(&fileHeader, sizeof(BitmapFileHeader), 1, file);
	if(fileHeader.type1!='B' || fileHeader.type2!='M'){
		throw runtime_error(path +" is not a bitmap");
	}

	//read info header
	BitmapInfoHeader infoHeader;
	readBytes = fread(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
	if(infoHeader.bitCount!=24){
        throw runtime_error(path+" is not a 24 bit bitmap");
	}

    h= infoHeader.height;
    w= infoHeader.width;
	components= 3;
}

void PixmapIoBmp::read(uint8 *pixels) {
	read(pixels, 3);
}

void PixmapIoBmp::read(uint8 *pixels, int components) {
    for(int i=0; i<h*w*components; i+=components) {
		uint8 r, g, b;
		size_t readBytes = fread(&b, 1, 1, file);
		readBytes = fread(&g, 1, 1, file);
		readBytes = fread(&r, 1, 1, file);

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

	file= fopen(path.c_str(),"wb");
	if (file == NULL) {
		throw runtime_error("Can't open BMP file for writing: "+ path);
	}

	BitmapFileHeader fileHeader;
    fileHeader.type1='B';
	fileHeader.type2='M';
	fileHeader.offsetBits=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader);
	fileHeader.size=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader)+3*h*w;

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

	fwrite(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
}

void PixmapIoBmp::write(uint8 *pixels) {
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

	throw runtime_error("PixmapIoPng::openRead not implemented!");

/*
    file= fopen(path.c_str(),"rb");
	if (file==NULL){
		throw runtime_error("Can't open BMP file: "+ path);
	}

	//read file header
    BitmapFileHeader fileHeader;
    size_t readBytes = fread(&fileHeader, sizeof(BitmapFileHeader), 1, file);
	if(fileHeader.type1!='B' || fileHeader.type2!='M'){
		throw runtime_error(path +" is not a bitmap");
	}

	//read info header
	BitmapInfoHeader infoHeader;
	readBytes = fread(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
	if(infoHeader.bitCount!=24){
        throw runtime_error(path+" is not a 24 bit bitmap");
	}

    h= infoHeader.height;
    w= infoHeader.width;
	components= 3;
*/
}

void PixmapIoPng::read(uint8 *pixels) {
	throw runtime_error("PixmapIoPng::read not implemented!");
	//read(pixels, 3);
}

void PixmapIoPng::read(uint8 *pixels, int components) {

	throw runtime_error("PixmapIoPng::read #2 not implemented!");

/*
    for(int i=0; i<h*w*components; i+=components) {
		uint8 r, g, b;
		size_t readBytes = fread(&b, 1, 1, file);
		readBytes = fread(&g, 1, 1, file);
		readBytes = fread(&r, 1, 1, file);

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
*/
}

void PixmapIoPng::openWrite(const string &path, int w, int h, int components) {
    this->path = path;
	this->w= w;
	this->h= h;
	this->components= components;

	file= fopen(path.c_str(),"wb");
	if (file == NULL) {
		throw runtime_error("Can't open PNG file for writing: "+ path);
	}
}

void PixmapIoPng::write(uint8 *pixels) {

    // initialize stuff
    std::auto_ptr<png_byte*> imrow(new png_byte*[h]);
    for(int i = 0; i < h; ++i) {
        imrow.get()[i] = pixels+(h-1-i) * w * components;
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
    png_write_image(imgp, imrow.get());
    png_write_end(imgp, NULL);



/*
	// Allocate write & info structures
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if(!png_ptr) {
	 fclose(file);
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - out of memory creating write structure");
   }

   png_infop info_ptr = png_create_info_struct(png_ptr);
   if(!info_ptr) {
	 png_destroy_write_struct(&png_ptr,
							  (png_infopp)NULL);
	 fclose(file);
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - out of memery creating info structure");
   }

   // setjmp() must be called in every function that calls a PNG-writing
   // libpng function, unless an alternate error handler was installed--
   // but compatible error handlers must either use longjmp() themselves
   // (as in this program) or exit immediately, so here we go:

   if(setjmp(png_jmpbuf(png_ptr))) {
	 png_destroy_write_struct(&png_ptr, &info_ptr);
	 fclose(file);
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - setjmp problem");
   }

   // make sure outfile is (re)opened in BINARY mode
   png_init_io(png_ptr, file);

   // set the compression levels--in general, always want to leave filtering
   // turned on (except for palette images) and allow all of the filters,
   // which is the default; want 32K zlib window, unless entire image buffer
   // is 16K or smaller (unknown here)--also the default; usually want max
   // compression (NOT the default); and remaining compression flags should
   // be left alone

   //png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
   png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);

   //
   // this is default for no filtering; Z_FILTERED is default otherwise:
   // png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
   //  these are all defaults:
   //   png_set_compression_mem_level(png_ptr, 8);
   //   png_set_compression_window_bits(png_ptr, 15);
   //   png_set_compression_method(png_ptr, 8);


   // Set some options: color_type, interlace_type
   int color_type=0, interlace_type=0, numChannels=0;

   //  color_type = PNG_COLOR_TYPE_GRAY;
   //  color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
   color_type = PNG_COLOR_TYPE_RGBA;
   numChannels = 4;
   // color_type = PNG_COLOR_TYPE_RGB_ALPHA;

   interlace_type =  PNG_INTERLACE_NONE;
   // interlace_type = PNG_INTERLACE_ADAM7;

   int bit_depth = 8;
   png_set_IHDR(png_ptr, info_ptr, this->w, this->h,
                bit_depth,
				color_type,
				interlace_type,
				PNG_COMPRESSION_TYPE_BASE,
				PNG_FILTER_TYPE_BASE);

   // Optional gamma chunk is strongly suggested if you have any guess
   // as to the correct gamma of the image. (we don't have a guess)
   //
   // png_set_gAMA(png_ptr, info_ptr, image_gamma);
   //png_set_strip_alpha(png_ptr);

   // write all chunks up to (but not including) first IDAT
   png_write_info(png_ptr, info_ptr);

   //png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );


   // set up the row pointers for the image so we can use png_write_image
   png_bytep* row_pointers = new png_bytep[this->h];
   if (row_pointers == 0) {
	 png_destroy_write_struct(&png_ptr, &info_ptr);
	 fclose(file);
	 throw runtime_error("OpenGlDevice::failed to allocate memory for row pointers");
   }

   unsigned int row_stride = this->w * numChannels;
   unsigned char *rowptr = (unsigned char*) pixels;
   for (int row = this->h-1; row >=0 ; row--) {
	 row_pointers[row] = rowptr;
	 rowptr += row_stride;
   }

   // now we just write the whole image; libpng takes care of interlacing for us
   png_write_image(png_ptr, row_pointers);

   // since that's it, we also close out the end of the PNG file now--if we
   // had any text or time info to write after the IDATs, second argument
   // would be info_ptr, but we optimize slightly by sending NULL pointer:

   png_write_end(png_ptr, info_ptr);

   //
   // clean up after the write
   //    free any memory allocated & close the file
   //
   png_destroy_write_struct(&png_ptr, &info_ptr);

   delete [] row_pointers;
*/
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

	throw runtime_error("PixmapIoJpg::openRead not implemented!");
}

void PixmapIoJpg::read(uint8 *pixels) {
	throw runtime_error("PixmapIoJpg::read not implemented!");
}

void PixmapIoJpg::read(uint8 *pixels, int components) {

	throw runtime_error("PixmapIoJpg::read #2 not implemented!");
}

void PixmapIoJpg::openWrite(const string &path, int w, int h, int components) {
    this->path = path;
	this->w= w;
	this->h= h;
	this->components= components;

	file= fopen(path.c_str(),"wb");
	if (file == NULL) {
		throw runtime_error("Can't open JPG file for writing: "+ path);
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
		const unsigned char *src = pixels;
		for (int i = 0; i < n; i++) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			src++;
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
	if (tmpbytes) {
		free(tmpbytes);
		tmpbytes=NULL;
	}
	free(flip);
	flip=NULL;

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

Pixmap1D::Pixmap1D(){
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap1D::Pixmap1D(int components){
	init(components);
}

Pixmap1D::Pixmap1D(int w, int components){
	init(w, components);
}

void Pixmap1D::init(int components){
	this->w= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap1D::init(int w, int components){
	this->w= w;
	this->components= components;
	pixels= new uint8[(std::size_t)getPixelByteCount()];
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
		throw runtime_error("Unknown pixmap extension: " + extension);
	}
	this->path = path;
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
		throw runtime_error("One of the texture dimensions must be 1");
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
		throw runtime_error("One of the texture dimensions must be 1");
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
    h= -1;
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap2D::Pixmap2D(int components) {
    h= -1;
    w= -1;
	this->components= -1;
    pixels= NULL;

	init(components);
}

Pixmap2D::Pixmap2D(int w, int h, int components) {
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
}

void Pixmap2D::init(int w, int h, int components) {
	this->w= w;
	this->h= h;
	this->components= components;
	deletePixels();

	if(getPixelByteCount() <= 0 || (h <= 0 || w <= 0 || components <= 0)) {
		char szBuf[1024];
		sprintf(szBuf,"Invalid pixmap dimensions for [%s], h = %d, w = %d, components = %d\n",path.c_str(),h,w,components);
		throw runtime_error(szBuf);
	}
	pixels= new uint8[(std::size_t)getPixelByteCount()];
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

Pixmap2D* Pixmap2D::loadPath(const string& path) {
	//printf("Loading Pixmap2D [%s]\n",path.c_str());

	Pixmap2D *pixmap = FileReader<Pixmap2D>::readPath(path);
	if(pixmap != NULL) {
		pixmap->path = path;
	}
	return pixmap;
}

void Pixmap2D::load(const string &path) {
	//printf("Loading Pixmap2D [%s]\n",path.c_str());

	FileReader<Pixmap2D>::readPath(path,this);
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
		throw runtime_error("Unknown pixmap extension: " + extension);
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
}

void Pixmap2D::setPixel(int x, int y, const float32 *value) {
	for(int i=0; i<components; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(value[i]*255.f);
	}
}

void Pixmap2D::setComponent(int x, int y, int component, uint8 value) {
	int index = (w*y+x)*components+component;
	pixels[index] = value;
}

void Pixmap2D::setComponent(int x, int y, int component, float32 value) {
	int index = (w*y+x)*components+component;
	pixels[index]= static_cast<uint8>(value*255.f);
}

//vector set
void Pixmap2D::setPixel(int x, int y, const Vec3f &p) {
	for(int i = 0; i < components  && i < 3; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}

void Pixmap2D::setPixel(int x, int y, const Vec4f &p) {
	for(int i = 0; i < components && i < 4; ++i) {
		int index = (w*y+x)*components+i;
		pixels[index]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}

void Pixmap2D::setPixel(int x, int y, float p) {
	int index = (w*y+x)*components;
	pixels[index]= static_cast<uint8>(p*255.f);
}

void Pixmap2D::setPixels(const uint8 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}

void Pixmap2D::setPixels(const float32 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}

void Pixmap2D::setComponents(int component, uint8 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
}

void Pixmap2D::setComponents(int component, float32 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
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
		throw runtime_error("Pixmap2D::splat: pixmap dimensions don't agree");
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
			distLu= streflop::pow(distLu, powFactor);
			distRu= streflop::pow(distRu, powFactor);
			distLd= streflop::pow(distLd, powFactor);
			distRd= streflop::pow(distRd, powFactor);
			avg= streflop::pow(avg, powFactor);
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
		throw runtime_error("Pixmap2D::lerp: pixmap dimensions don't agree");
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
		throw runtime_error("Pixmap2D::copy() dimensions must agree");
	}
	memcpy(pixels, sourcePixmap->getPixels(), w*h*sourcePixmap->getComponents());
}

void Pixmap2D::subCopy(int x, int y, const Pixmap2D *sourcePixmap){
	assert(components==sourcePixmap->getComponents());

	if(w<sourcePixmap->getW() && h<sourcePixmap->getH()){
		throw runtime_error("Pixmap2D::subCopy(), bad dimensions");
	}

	uint8 *pixel= new uint8[components];

	for(int i=0; i<sourcePixmap->getW(); ++i){
		for(int j=0; j<sourcePixmap->getH(); ++j){
			sourcePixmap->getPixel(i, j, pixel);
			setPixel(i+x, j+y, pixel);
		}
	}

	delete [] pixel;
}

bool Pixmap2D::doDimensionsAgree(const Pixmap2D *pixmap){
	return pixmap->getW() == w && pixmap->getH() == h;
}

// =====================================================
//	class Pixmap3D
// =====================================================

Pixmap3D::Pixmap3D(){
	w= -1;
	h= -1;
	d= -1;
	components= -1;
}

Pixmap3D::Pixmap3D(int w, int h, int d, int components){
	init(w, h, d, components);
}

Pixmap3D::Pixmap3D(int d, int components){
	init(d, components);
}

void Pixmap3D::init(int w, int h, int d, int components){
	this->w= w;
	this->h= h;
	this->d= d;
	this->components= components;
	pixels= new uint8[(std::size_t)getPixelByteCount()];
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
}

void Pixmap3D::init(int components) {
	this->w= -1;
	this->h= -1;
	this->d= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap3D::deletePixels() {
	delete [] pixels;
	pixels = NULL;
}

Pixmap3D::~Pixmap3D() {
	deletePixels();
}

void Pixmap3D::loadSlice(const string &path, int slice) {
	string extension= path.substr(path.find_last_of('.') + 1);
	if(extension == "bmp") {
		loadSliceBmp(path, slice);
	}
	else if(extension == "tga") {
		loadSliceTga(path, slice);
	}
	else {
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
	this->path = path;
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
}

// =====================================================
//	class PixmapCube
// =====================================================

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
