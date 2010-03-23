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

#include "PNGReader.h"

#include "types.h"
#include "pixmap.h"
#include <stdexcept>
#include <png.h>
#include <setjmp.h>

using std::runtime_error;
using std::ios;

/**Used things from CImageLoaderJPG.cpp from Irrlicht*/

namespace Shared{ namespace Graphics{

// =====================================================
//	Callbacks for PNG-decompression
// =====================================================

static void user_read_data(png_structp read_ptr, png_bytep data, png_size_t length) {
	ifstream& is = *((ifstream*)png_get_io_ptr(read_ptr));
	is.read((char*)data,length);
	if (!is.good()) {
		png_error(read_ptr,"Could not read from png-file");
	}
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
}

static void user_flush_data(png_structp png_ptr) {}

/**Get Extension array, initialised*/
static inline const string* getExtensionsPng() {
	static const string extensions[] = {"png", ""};
	return extensions;
}

// =====================================================
//	class PNGReader
// =====================================================

PNGReader::PNGReader(): FileReader<Pixmap2D>(getExtensionsPng()) {}

Pixmap2D* PNGReader::read(ifstream& is, const string& path, Pixmap2D* ret) const {
	//Read file
	is.seekg(0, ios::end);
	size_t length = is.tellg();
	is.seekg(0, ios::beg);
	uint8 * buffer = new uint8[8];
	is.read((char*)buffer, 8);

	if (png_sig_cmp(buffer, 0, 8) != 0) {
		return NULL; //This is not a PNG file - could be used for fast checking whether file is supported or not
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		return NULL;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		return NULL;
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		return NULL; //Error during init_io
	}
	png_set_read_fn(png_ptr, &is, user_read_data); 
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	
	int width = info_ptr->width;
	int height = info_ptr->height;
	int color_type = info_ptr->color_type;
	int bit_depth = info_ptr->bit_depth;

	//We want RGB, 24 bit
	if (color_type == PNG_COLOR_TYPE_PALETTE || (color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8) || (info_ptr->valid & PNG_INFO_tRNS)) {
		png_set_expand(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(png_ptr);
	}

	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	png_bytep* row_pointers = new png_bytep[height];

	if (setjmp(png_jmpbuf(png_ptr))) {
		delete[] row_pointers;
		return NULL; //error during read_image
	}
	for (int y = 0; y < height; ++y) {
		row_pointers[y] = new png_byte[info_ptr->rowbytes];
	}
	png_read_image(png_ptr, row_pointers);
	int fileComponents = info_ptr->rowbytes/info_ptr->width;
	int picComponents = (ret->getComponents()==-1)?fileComponents:ret->getComponents();
	//Copy image
	ret->init(width,height,picComponents);
	uint8* pixels = ret->getPixels();
	const int rowbytes = info_ptr->rowbytes;
	int location = 0;
	for (int y = height-1; y >= 0; --y) { //you have to somehow invert the lines
		if (picComponents == fileComponents) {
			memcpy(pixels+location,row_pointers[y],rowbytes);
		} else {
			int r,g,b,a,l;
			for (int xPic = 0, xFile = 0; xFile < rowbytes; xPic+= picComponents, xFile+= fileComponents) {
				switch(fileComponents) {
					case 3:
						r = row_pointers[y][xFile];
						g = row_pointers[y][xFile+1];
						b = row_pointers[y][xFile+2];
						l = (r+g+b+2)/3;
						a = 255;
						break;
					case 4:
						r = row_pointers[y][xFile];
						g = row_pointers[y][xFile+1];
						b = row_pointers[y][xFile+2];
						l = (r+g+b+2)/3;
						a = row_pointers[y][xFile+3];	
						break;
					default:
						//TODO: Error
					case 1:
						r = g = b = l = row_pointers[y][xFile];
						a = 255;
						break;
				}
				switch (picComponents) {
					case 1:
						pixels[location+xPic] = l;
						break;
					case 4:
						pixels[location+xPic+3] = a; //Next case
					case 3:
						pixels[location+xPic] = r;
						pixels[location+xPic+1] = g;
						pixels[location+xPic+2] = b;
						break;
					default:
						//just so at least something works
						for (int i = 0; i < picComponents; ++i) {
							pixels[location+xPic+i] = l;
						}
						//TODO: Error
				}
			}
		}
		location+=picComponents*width;
	}
	delete[] row_pointers;
	return ret;
}

}} //end namespace
