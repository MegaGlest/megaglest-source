// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Image.h -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licensed under the MIT license:
// http://www.opensource.org/licenses/mit-license.php
//
// Open Source Initiative OSI - The MIT License (MIT):Licensing
//
// The MIT License (MIT)
// Copyright (c) <year> <copyright holders>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.//
//
// Declaration of DDS, TGA, PCX, JPEG and PNG image loader classes.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __IMAGE_H__
#define __IMAGE_H__

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#endif // _WIN32

#include <GL/glew.h>

#if !defined(_WIN32) || defined(__MINGW32__)
#include <tr1/memory>
#endif

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <png.h>

extern "C" {
#include <jpeglib.h>
}

using std::cout;
using std::endl;
using std::string;

#if defined(_WIN32)
using std::tr1::shared_ptr;
#endif

namespace Shared { namespace Graphics { namespace md5 {

#ifndef _WIN32
//using boost::shared_ptr;
using std::tr1::shared_ptr;
#endif

/////////////////////////////////////////////////////////////////////////////
// Image class diagram:
//
//    +------- (abs)                     +---------------+
//    |  Image  |                        | runtime_error |
//    +---------+                        +---------------+
//         ^                                     ^
//         |   +------------+                    |
//         +---|  ImageDDS  |            +----------------+
//         |   +------------+            | ImageException |
//         |                             +----------------+
//         |   +------------+
//         +---|  ImageTGA  |
//         |   +------------+
//         |                             +---------------+
//         |   +------------+            |  ImageBuffer  |
//         +---|  ImagePCX  |            +---------------+
//         |   +------------+
//         |
//         |   +------------+
//         +---|  ImageJPEG |            +----------------+
//         |   +------------+            |  ImageFactory  |
//         |                             +----------------+
//         |   +------------+
//         +---|  ImagePNG  |
//             +------------+
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// class ImageException - Exception class for ImageBuffer and Image
// loaders.  This acts like a standard runtime_error exception but
// know which file has failed to be loaded.
//
/////////////////////////////////////////////////////////////////////////////
class ImageException : public std::runtime_error {
public:
  // Constructors
  ImageException (const string &error)
    : std::runtime_error (error) { }
  ImageException (const string &error, const string &filename)
    : std::runtime_error (error), _which (filename) { }
  virtual ~ImageException () throw () { }

public:
  // Public interface
  virtual const char *which () const throw () {
    return _which.c_str ();
  }

private:
  // Member variables
  string _which;
};

/////////////////////////////////////////////////////////////////////////////
//
// class ImageBuffer - An image file loader class.  Load a whole file
// into a memory buffer.
//
/////////////////////////////////////////////////////////////////////////////
class ImageBuffer {
public:
  // Constructors/destructor
  ImageBuffer (const string &filename);

  ImageBuffer (const ImageBuffer &that)
    : _filename (that._filename), _data (NULL), _length (that._length)
  {
    _data = new GLubyte[_length];
    memcpy (_data, that._data, _length);
  }

  ~ImageBuffer ();

private:
  // Disable default constructor
  ImageBuffer ();

public:
  // Accessors
  const string &filename () const { return _filename; }
  const GLubyte *data () const { return _data; }
  size_t length () const { return _length; }

  ImageBuffer &operator= (const ImageBuffer &rhs)
  {
    this->~ImageBuffer ();
    new (this) ImageBuffer (rhs);
    return *this;
  }

private:
  // Member variables
  string _filename;

  GLubyte *_data;
  size_t _length;
};

/////////////////////////////////////////////////////////////////////////////
//
// class Image - A generic image loader class for creating OpenGL
// textures from.  All other specific image loader are derived from it.
//
/////////////////////////////////////////////////////////////////////////////
class Image {
protected:
  // Default constructor
  Image ()
    : _width (0), _height (0), _numMipmaps (0),
      _format (0), _components (0), _pixels (NULL),
      _standardCoordSystem (true) { }

private:
  // Disable copy constructor.
  Image (const Image &img);

public:
  // Constructors/destructor
  Image (const string &name, GLsizei w, GLsizei h, GLint numMipMaps,
	 GLenum format, GLint components, const GLubyte *pixels,
	 bool stdCoordSystem);

  virtual ~Image();

public:
  // Return true if we're working with S3
  // compressed textures (DDS files)
  bool isCompressed () const;
  bool isPowerOfTwo () const;

  // Accessors
  GLsizei width () const { return _width; }
  GLsizei height () const { return _height; }
  GLint numMipmaps () const { return _numMipmaps; }
  GLenum format () const { return _format; }
  GLint components () const { return _components; }
  const GLubyte *pixels () const { return _pixels; }
  const string &name () const { return _name; }
  bool stdCoordSystem () const { return _standardCoordSystem; }

protected:
  // Member variables
  GLsizei _width;
  GLsizei _height;
  GLint _numMipmaps;

  // OpenGL texture format and internal
  // format (components)
  GLenum _format;
  GLint _components;

  // Image data
  GLubyte *_pixels;

  string _name;

  // Is the picture in standard OpenGL 2D coordinate
  // system? (starts lower-left corner)
  bool _standardCoordSystem;
};


// Definition of type aliases
typedef shared_ptr<Image> ImagePtr;

/////////////////////////////////////////////////////////////////////////////
//
// class ImageDDS - A DirectDraw Surface (DDS) image loader class.
// Support only DXT1, DXT3 and DXT5 formats, since OpenGL doesn't
// support others.  GL_EXT_texture_compression_s3tc must be supported.
//
// NOTE: Because DirectX uses the upper left corner as origin of the
// picture when OpenGL uses the lower left corner, the texture generated
// from it will be rendered upside-down.
//
/////////////////////////////////////////////////////////////////////////////
class ImageDDS : public Image {
public:
  // Constructor
  ImageDDS (const ImageBuffer &ibuff);

private:
  // Internal data structures and
  // member variables
  struct DDPixelFormat
  {
    GLuint size;
    GLuint flags;
    GLuint fourCC;
    GLuint bpp;
    GLuint redMask;
    GLuint greenMask;
    GLuint blueMask;
    GLuint alphaMask;
  };

  struct DDSCaps
  {
    GLuint caps;
    GLuint caps2;
    GLuint caps3;
    GLuint caps4;
  };

  struct DDColorKey
  {
    GLuint lowVal;
    GLuint highVal;
  };

  struct DDSurfaceDesc
  {
    GLuint size;
    GLuint flags;
    GLuint height;
    GLuint width;
    GLuint pitch;
    GLuint depth;
    GLuint mipMapLevels;
    GLuint alphaBitDepth;
    GLuint reserved;
    GLuint surface;

    DDColorKey ckDestOverlay;
    DDColorKey ckDestBlt;
    DDColorKey ckSrcOverlay;
    DDColorKey ckSrcBlt;

    DDPixelFormat format;
    DDSCaps caps;

    GLuint textureStage;
  };

  const DDSurfaceDesc *_ddsd;
};

/////////////////////////////////////////////////////////////////////////////
//
// class glImageTGA - A TrueVision TARGA (TGA) image loader class.
// Support 24-32 bits BGR files; 16 bits RGB; 8 bits indexed (BGR
// palette); 8 and 16 bits grayscale; all compressed and uncompressed.
// Compressed TGA images use RLE algorithm.
//
/////////////////////////////////////////////////////////////////////////////
class ImageTGA : public Image {
public:
  // Constructor
  ImageTGA (const ImageBuffer &ibuff);

private:
  // Internal functions
  void getTextureInfo ();

  void readTGA8bits (const GLubyte *data, const GLubyte *colormap);
  void readTGA16bits (const GLubyte *data);
  void readTGA24bits (const GLubyte *data);
  void readTGA32bits (const GLubyte *data);
  void readTGAgray8bits (const GLubyte *data);
  void readTGAgray16bits (const GLubyte *data);

  void readTGA8bitsRLE (const GLubyte *data, const GLubyte *colormap);
  void readTGA16bitsRLE (const GLubyte *data);
  void readTGA24bitsRLE (const GLubyte *data);
  void readTGA32bitsRLE (const GLubyte *data);
  void readTGAgray8bitsRLE (const GLubyte *data);
  void readTGAgray16bitsRLE (const GLubyte *data);

private:
  // Member variables
#pragma pack(push, 1)
  // tga header
  struct TGA_Header
  {
    GLubyte id_lenght;        // size of image id
    GLubyte colormap_type;    // 1 is has a colormap
    GLubyte image_type;       // compression type

    short cm_first_entry;     // colormap origin
    short cm_length;          // colormap length
    GLubyte cm_size;          // colormap size

    short x_origin;           // bottom left x coord origin
    short y_origin;           // bottom left y coord origin

    short width;              // picture width (in pixels)
    short height;             // picture height (in pixels)

    GLubyte pixel_depth;      // bits per pixel: 8, 16, 24 or 32
    GLubyte image_descriptor; // 24 bits = 0x00; 32 bits = 0x80

  };
#pragma pack(pop)

  const TGA_Header *_header;

  // NOTE:
  // 16 bits images are stored in RGB
  // 8-24-32 images are stored in BGR(A)

  // RGBA/BGRA component table access -- usefull for
  // switching from bgra to rgba at load time.
  static int rgbaTable[4]; // bgra to rgba: 2, 1, 0, 3
  static int bgraTable[4]; // bgra to bgra: 0, 1, 2, 3
};

/////////////////////////////////////////////////////////////////////////////
//
// class ImagePCX - A Zsoft PCX image loader class.
//
/////////////////////////////////////////////////////////////////////////////
class ImagePCX : public Image {
public:
  // Constructor
  ImagePCX (const ImageBuffer &ibuff);

private:
  // Internal functions
  void readPCX1bit (const GLubyte *data);
  void readPCX4bits (const GLubyte *data);
  void readPCX8bits (const GLubyte *data,
		     const GLubyte *palette);
  void readPCX24bits (const GLubyte *data);

private:
#pragma pack(push, 1)
  // pcx header
  struct PCX_Header
  {
    GLubyte manufacturer;
    GLubyte version;
    GLubyte encoding;
    GLubyte bitsPerPixel;

    GLushort xmin, ymin;
    GLushort xmax, ymax;
    GLushort horzRes, vertRes;

    GLubyte palette[48];
    GLubyte reserved;
    GLubyte numColorPlanes;

    GLushort bytesPerScanLine;
    GLushort paletteType;
    GLushort horzSize, vertSize;

    GLubyte padding[54];
  };
#pragma pack(pop)

  const PCX_Header *_header;

  // RGBA/BGRA component table access -- usefull for
  // switching from bgra to rgba at load time.
  static int rgbTable[3]; // bgra to rgba: 0, 1, 2
  static int bgrTable[3]; // bgra to bgra: 2, 1, 0
};

/////////////////////////////////////////////////////////////////////////////
//
// class ImageJPEG - A JPEG image loader class using libjpeg.
//
/////////////////////////////////////////////////////////////////////////////
class ImageJPEG : public Image {
public:
  // Constructor
  ImageJPEG (const ImageBuffer &ibuff);

private:
  // Error manager, using C's setjmp/longjmp
  struct my_error_mgr
  {
    jpeg_error_mgr pub;	    // "public" fields
    jmp_buf setjmp_buffer;  // for return to caller

    string errorMsg;        // last error message
  };

  typedef my_error_mgr *my_error_ptr;

private:
  // libjpeg's callback functions for reading data
  static void initSource_callback (j_decompress_ptr cinfo);
  static boolean fillInputBuffer_callback (j_decompress_ptr cinfo);
  static void skipInputData_callback (j_decompress_ptr cinfo,
				      long num_bytes);
  static void termSource_callback (j_decompress_ptr cinfo);

  // libjpeg's callback functions for error handling
  static void errorExit_callback (j_common_ptr cinfo);
  static void outputMessage_callback (j_common_ptr cinfo);
};

/////////////////////////////////////////////////////////////////////////////
//
// class ImagePNG - A Portable Network Graphic (PNG) image loader
// class using libpng and zlib.
//
/////////////////////////////////////////////////////////////////////////////
class ImagePNG : public Image {
public:
  // Constructor
  ImagePNG (const ImageBuffer &ibuff);

private:
  // Internal functions
  void getTextureInfo (int color_type);

  // libpng's callback functions for reading data
  // and error handling
  static void read_callback (png_structp png_ptr,
			     png_bytep data, png_size_t length);
  static void error_callback (png_structp png_ptr,
			      png_const_charp error_msg);
  static void warning_callback (png_structp png_ptr,
				png_const_charp warning_msg);

public:
  // Data source manager.  Contains an image buffer and
  // an offset position into file's data
  struct my_source_mgr
  {
    // Constructors
    my_source_mgr ()
      : pibuff (NULL), offset (0) { }
    my_source_mgr (const ImageBuffer &ibuff)
      : pibuff (&ibuff), offset (0) { }

    // Public member variables
    const ImageBuffer *pibuff;
    size_t offset;
  };

  typedef my_source_mgr *my_source_ptr;

private:
  // Member variables
  string errorMsg;
};

/////////////////////////////////////////////////////////////////////////////
//
// class ImageFactory - An Image Factory Class.
//
/////////////////////////////////////////////////////////////////////////////
class ImageFactory {
public:
  // Public interface
  static Image *createImage (const ImageBuffer &ibuff)
  {
    string ext;
    Image *result;

    // Extract file extension
    const string &filename = ibuff.filename ();
    ext.assign (filename, filename.find_last_of ('.') + 1, string::npos);

    if (ext.compare ("dds") == 0)
      {
	result = new ImageDDS (ibuff);
      }
    else if (ext.compare ("tga") == 0)
      {
	result = new ImageTGA (ibuff);
      }
    else if (ext.compare ("pcx") == 0)
      {
	result = new ImagePCX (ibuff);
      }
    else if (ext.compare ("jpg") == 0)
      {
	result = new ImageJPEG (ibuff);
      }
    else if (ext.compare ("png") == 0)
      {
	result = new ImagePNG (ibuff);
      }
    else
      {
	throw ImageException ("Unhandled image file format", filename);
      }

    return result;
  }
};

}}} //end namespace

#endif // __IMAGE_H__
