// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Image.cpp -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// Implementation of image loader classes for DDS, TGA, PCX, JPEG
// and PNG image formats.
//
/////////////////////////////////////////////////////////////////////////////

#include "Image.h"

namespace Shared { namespace Graphics { namespace md5 {

/////////////////////////////////////////////////////////////////////////////
//
// class ImageBuffer implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ImageBuffer::ImageBuffer
//
// Constructor.  Load a file into memory buffer.
// --------------------------------------------------------------------------
ImageBuffer::ImageBuffer (const string &filename)
  : _filename (filename), _data (NULL), _length (0) {
  // Open file
  std::ifstream ifs (filename.c_str(), std::ios::in | std::ios::binary);

  if (ifs.fail ())
    throw ImageException ("Couldn't open image file: " + filename, filename);

  // Get file length
  ifs.seekg (0, std::ios::end);
  _length = ifs.tellg ();
  ifs.seekg (0, std::ios::beg);

  try {
      // Allocate memory for holding file data
      _data = new GLubyte[_length];

      // Read whole file data
      ifs.read (reinterpret_cast<char *> (_data), _length);
      ifs.close ();
    }
  catch (...)
    {
      delete [] _data;
      throw;
    }
}

// --------------------------------------------------------------------------
// ImageBuffer::~ImageBuffer
//
// Destructor.  Free memory buffer.
// --------------------------------------------------------------------------
ImageBuffer::~ImageBuffer () {
  delete [] _data;
  _data = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// class Image implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Image::Image
//
// Constructors.  Create an image from a pixel buffer.  It allows to
// create an image from an other source of data than an ImageBuffer.
// This is also the copy constructor to use if you want pixel data
// to be copied, since the Image(const Image&) constructor is protected.
// --------------------------------------------------------------------------

Image::Image (const string &name, GLsizei w, GLsizei h, GLint numMipMaps,
	      GLenum format, GLint components, const GLubyte *pixels,
	      bool stdCoordSystem)
  : _width (w), _height (h), _numMipmaps (numMipMaps),
    _format (format), _components (components), _name (name),
    _standardCoordSystem (stdCoordSystem) {
  // NOTE: pixels should be a valid pointer. w, h and components
  // have to be non-zero positive values.

  long size = _width * _height * _components;

  if (size <= 0)
    throw std::invalid_argument
      ("Image::Image: Invalid width, height or component value");

  if (!pixels)
    throw std::invalid_argument
      ("Image::Image: Invalid pixel data source");

  // allocate memory for pixel data
  _pixels = new GLubyte[size];

  // Copy pixel data from buffer
  memcpy (_pixels, pixels, size);
}

// --------------------------------------------------------------------------
// Image::~Image
//
// Destructor.  Delete all memory allocated for this object, i.e. pixel
// data.
// --------------------------------------------------------------------------
Image::~Image () {
  delete [] _pixels;
  _pixels = NULL;
}

// --------------------------------------------------------------------------
// Image::isCompressed
//
// Check if the image is using S3TC compression (this is the case for
// DDS image only).
// --------------------------------------------------------------------------
bool Image::isCompressed () const {
  switch (_format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      return true;

    default:
      return false;
    }
}

// --------------------------------------------------------------------------
// Image::isPowerOfTwo
//
// Check if the image dimensions are powers of two.
// --------------------------------------------------------------------------
bool Image::isPowerOfTwo () const {
  GLsizei m;
  for (m = 1; m < _width; m *= 2)
    ;

  if (m != _width)
    return false;

  for (m = 1; m < _height; m *= 2)
    ;

  if (m != _height)
    return false;

  return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// class ImageDDS implementation.
//
/////////////////////////////////////////////////////////////////////////////

// FourCC auto generation with templates
template <char ch0, char ch1, char ch2, char ch3>
struct MakeFourCC {
  enum {
    Val =(((ch3 << 24) & 0xFF000000) |
	  ((ch2 << 16) & 0x00FF0000) |
	  ((ch1 <<  8) & 0x0000FF00) |
	  (ch0         & 0x000000FF))
  };
};

const unsigned int FOURCC_DXT1 = MakeFourCC<'D', 'X', 'T', '1'>::Val;
const unsigned int FOURCC_DXT3 = MakeFourCC<'D', 'X', 'T', '3'>::Val;
const unsigned int FOURCC_DXT5 = MakeFourCC<'D', 'X', 'T', '5'>::Val;

// --------------------------------------------------------------------------
// ImageDDS::ImageDDS
//
// Constructor.  Read a DDS image from memory.
// --------------------------------------------------------------------------
ImageDDS::ImageDDS (const ImageBuffer &ibuff)
  : _ddsd (NULL) {
  _name = ibuff.filename ();

  // DDS images use the GDI+ coordinate system (starts upper-left corner)
  _standardCoordSystem = false;

  // There are extensions required for handling compressed DDS
  // images as textures
  if (!GLEW_ARB_texture_compression)
    throw ImageException ("missing GL_ARB_texture_compression"
			  " extension", _name);

  if (!GLEW_EXT_texture_compression_s3tc)
    throw ImageException ("missing GL_EXT_texture_compression_s3tc"
			  " extension", _name);

  try {
      // Get pointer on file data
      const GLubyte *data_ptr = ibuff.data ();
      int bufferSize = ibuff.length ();

      // Check if is a valid DDS file
      string magic;
      magic.assign (reinterpret_cast<const char *>(data_ptr), 4);
      if (magic.compare (0, 4, "DDS ") != 0)
	throw ImageException ("Not a valid DDS file", _name);

      // Eat 4 bytes magic number
      data_ptr += 4;
      bufferSize -= 4;

      // Read the surface descriptor and init some member variables
      _ddsd = reinterpret_cast<const DDSurfaceDesc*>(data_ptr);
      data_ptr += sizeof (DDSurfaceDesc);
      bufferSize -= sizeof (DDSurfaceDesc);

      _width = _ddsd->width;
      _height = _ddsd->height;
      _numMipmaps = _ddsd->mipMapLevels;

      switch (_ddsd->format.fourCC)
	{
	case FOURCC_DXT1:
	  // DXT1's compression ratio is 8:1
	  _format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	  _components = 3;
	  break;

	case FOURCC_DXT3:
	  // DXT3's compression ratio is 4:1
	  _format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	  _components = 4;
	  break;

	case FOURCC_DXT5:
	  // DXT5's compression ratio is 4:1
	  _format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	  _components = 4;
	  break;

	default:
	  // Bad fourCC, unsupported or bad format
	  throw ImageException ("Unsupported DXT format", _name);
	}

      // Read pixel data with mipmaps
      _pixels = new GLubyte[bufferSize];
      memcpy (_pixels, data_ptr, bufferSize);
    }
  catch (...)
    {
      delete [] _pixels;
      throw;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// class ImageTGA implementation.
//
/////////////////////////////////////////////////////////////////////////////

// Pixel's component table access
int ImageTGA::rgbaTable[4] = { 2, 1, 0, 3 };
int ImageTGA::bgraTable[4] = { 0, 1, 2, 3 };

// --------------------------------------------------------------------------
// ImageTGA::ImageTGA
//
// Constructor.  Read a TGA image from memory.
// --------------------------------------------------------------------------
ImageTGA::ImageTGA (const ImageBuffer &ibuff)
  : _header (NULL) {
  const GLubyte *colormap = NULL;
  const GLubyte *data_ptr;

  try
    {
      _name = ibuff.filename ();
      data_ptr = ibuff.data ();
      _standardCoordSystem = true;

      // Read TGA header
      _header = reinterpret_cast<const TGA_Header *>(data_ptr);
      data_ptr += sizeof (TGA_Header) + _header->id_lenght;

      // Get image information
      getTextureInfo ();

      // Memory allocation for pixel data
      _pixels = new GLubyte[_width * _height * _components];

      // Read color map, if present
      if (_header->colormap_type)
	{
	  // NOTE: color map is stored in BGR
	  colormap = data_ptr;
	  data_ptr += _header->cm_length * (_header->cm_size >> 3);
	}

      // Decode image data
      switch (_header->image_type)
	{
	case 0:
	  // No data
	  break;

	case 1:
	  // Uncompressed 8 bits color index
	  readTGA8bits (data_ptr, colormap);
	  break;

	case 2:
	  // Uncompressed 16-24-32 bits
	  switch (_header->pixel_depth)
	    {
	    case 16:
	      readTGA16bits (data_ptr);
	      break;

	    case 24:
	      readTGA24bits (data_ptr);
	      break;

	    case 32:
	      readTGA32bits (data_ptr);
	      break;
	    }

	  break;

	case 3:
	  // Uncompressed 8 or 16 bits grayscale
	  if (_header->pixel_depth == 8)
	    readTGAgray8bits (data_ptr);
	  else // 16 bits
	    readTGAgray16bits (data_ptr);

	  break;

	case 9:
	  // RLE compressed 8 bits color index
	  readTGA8bitsRLE (data_ptr, colormap);
	  break;

	case 10:
	  // RLE compressed 16-24-32 bits
	  switch (_header->pixel_depth)
	    {
	    case 16:
	      readTGA16bitsRLE (data_ptr);
	      break;

	    case 24:
	      readTGA24bitsRLE (data_ptr);
	      break;

	    case 32:
	      readTGA32bitsRLE (data_ptr);
	      break;
	    }

	  break;

	case 11:
	  // RLE compressed 8 or 16 bits grayscale
	  if (_header->pixel_depth == 8)
	    readTGAgray8bitsRLE (data_ptr);
	  else // 16 bits
	    readTGAgray16bitsRLE (data_ptr);

	  break;

	default:
	  // Image type is not correct, free memory and quit
	  throw ImageException ("Unknown TGA image type", _name);
	}
    }
  catch (...)
    {
      delete [] _pixels;
      throw;
    }
}

// --------------------------------------------------------------------------
// ImageTGA::getTextureInfo
//
// Extract OpenGL texture informations from TGA header.
// --------------------------------------------------------------------------
void ImageTGA::getTextureInfo () {
  _width = _header->width;
  _height = _header->height;

  switch (_header->image_type)
    {
    case 3:  // grayscale 8 bits
    case 11: // grayscale 8 bits (RLE)
      {
	if (_header->pixel_depth == 8)
	  {
	    _format = GL_LUMINANCE;
	    _components = 1;
	  }
	else // 16 bits
	  {
	    _format = GL_LUMINANCE_ALPHA;
	    _components = 2;
	  }

	break;
      }

    case 1:    // 8 bits color index
    case 2:    // BGR 16-24-32 bits
    case 9:    // 8 bits color index (RLE)
    case 10:   // BGR 16-24-32 bits (RLE)
      {
	// 8 bits and 16 bits images will be converted to 24 bits
	if (_header->pixel_depth <= 24)
	  {
	    _format = GLEW_EXT_bgra ? GL_BGR : GL_RGB;
	    _components = 3;
	  }
	else // 32 bits
	  {
	    _format = GLEW_EXT_bgra ? GL_BGRA : GL_RGBA;
	    _components = 4;
	  }

	break;
      }
    }
}

// --------------------------------------------------------------------------
// ImageTGA::readTGA8bits
//
// Read 8 bits pixel data from TGA file.
// --------------------------------------------------------------------------
void ImageTGA::readTGA8bits (const GLubyte *data, const GLubyte *colormap) {
  const GLubyte *pData = data;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  for (int i = 0; i < _width * _height; ++i)
    {
      // Read index color byte
      GLubyte color = *(pData++);

      // Convert to BGR/RGB 24 bits
      _pixels[(i * 3) + compTable[0]] = colormap[(color * 3) + 0];
      _pixels[(i * 3) + compTable[1]] = colormap[(color * 3) + 1];
      _pixels[(i * 3) + compTable[2]] = colormap[(color * 3) + 2];
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGA16bits
//
// Read 16 bits pixel data from TGA file.
// --------------------------------------------------------------------------
void ImageTGA::readTGA16bits (const GLubyte *data) {
  const GLubyte *pData = data;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  for (int i = 0; i < _width * _height; ++i, pData += 2) {
      // Read color word
      unsigned short color = pData[0] + (pData[1] << 8);

      // convert to BGR/RGB 24 bits
      _pixels[(i * 3) + compTable[2]] = (((color & 0x7C00) >> 10) << 3);
      _pixels[(i * 3) + compTable[1]] = (((color & 0x03E0) >>  5) << 3);
      _pixels[(i * 3) + compTable[0]] = (((color & 0x001F) >>  0) << 3);
    }
}

// --------------------------------------------------------------------------
// ImageTGA::readTGA24bits
//
// Read 24 bits pixel data from TGA file.
// --------------------------------------------------------------------------
void ImageTGA::readTGA24bits (const GLubyte *data) {
  if (GLEW_EXT_bgra)
    {
      memcpy (_pixels, data, _width * _height * 3);
    }
  else
    {
      const GLubyte *pData = data;

      for (int i = 0; i < _width * _height; ++i)
	{
	  // Read RGB 24 bits pixel
	  _pixels[(i * 3) + rgbaTable[0]] = *(pData++);
	  _pixels[(i * 3) + rgbaTable[1]] = *(pData++);
	  _pixels[(i * 3) + rgbaTable[2]] = *(pData++);
	}
    }
}

// --------------------------------------------------------------------------
// ImageTGA::readTGA32bits
//
// Read 32 bits pixel data from TGA file.
// --------------------------------------------------------------------------
void ImageTGA::readTGA32bits (const GLubyte *data) {
  if (GLEW_EXT_bgra)
    {
      memcpy (_pixels, data, _width * _height * 4);
    }
  else
    {
      const GLubyte *pData = data;

      for (int i = 0; i < _width * _height; ++i)
	{
	  // Read RGB 32 bits pixel
	  _pixels[(i * 4) + rgbaTable[0]] = *(pData++);
	  _pixels[(i * 4) + rgbaTable[1]] = *(pData++);
	  _pixels[(i * 4) + rgbaTable[2]] = *(pData++);
	  _pixels[(i * 4) + rgbaTable[3]] = *(pData++);
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGAgray8bits
//
// Read grey 8 bits pixel data from TGA file.
// --------------------------------------------------------------------------
void ImageTGA::readTGAgray8bits (const GLubyte *data) {
  memcpy (_pixels, data, _width * _height);
}

// --------------------------------------------------------------------------
// ImageTGA::readTGAgray16bits
//
// Read grey 16 bits pixel data from TGA file.
// --------------------------------------------------------------------------

void
ImageTGA::readTGAgray16bits (const GLubyte *data)
{
  memcpy (_pixels, data, _width * _height * 2);
}


// --------------------------------------------------------------------------
// ImageTGA::readTGA8bitsRLE
//
// Read 8 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGA8bitsRLE (const GLubyte *data, const GLubyte *colormap)
{
  GLubyte *ptr = _pixels;
  const GLubyte *pData = data;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  while (ptr < _pixels + (_width * _height) * 3)
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet
	  GLubyte color = *(pData++);

	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = colormap[(color * 3) + compTable[0]];
	      ptr[1] = colormap[(color * 3) + compTable[1]];
	      ptr[2] = colormap[(color * 3) + compTable[2]];
	    }
	}
      else
	{
	  // Non run-length packet
	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      GLubyte color = *(pData++);

	      ptr[0] = colormap[(color * 3) + compTable[0]];
	      ptr[1] = colormap[(color * 3) + compTable[1]];
	      ptr[2] = colormap[(color * 3) + compTable[2]];
	    }
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGA16bitsRLE
//
// Read 16 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGA16bitsRLE (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  while (ptr < _pixels + (_width * _height) * 3)
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet
	  unsigned short color = pData[0] + (pData[1] << 8);
	  pData += 2;

	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[compTable[2]] = (((color & 0x7C00) >> 10) << 3);
	      ptr[compTable[1]] = (((color & 0x03E0) >>  5) << 3);
	      ptr[compTable[0]] = (((color & 0x001F) >>  0) << 3);
	    }
	}
      else
	{
	  // Non run-length packet
	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      unsigned short color = pData[0] + (pData[1] << 8);
	      pData += 2;

	      ptr[compTable[2]] = (((color & 0x7C00) >> 10) << 3);
	      ptr[compTable[1]] = (((color & 0x03E0) >>  5) << 3);
	      ptr[compTable[0]] = (((color & 0x001F) >>  0) << 3);
	    }
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGA24bitsRLE
//
// Read 24 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGA24bitsRLE (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  while (ptr < _pixels + (_width * _height) * 3)
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet
	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = pData[compTable[0]];
	      ptr[1] = pData[compTable[1]];
	      ptr[2] = pData[compTable[2]];
	    }

	  pData += 3;
	}
      else
	{
	  // Non run-length packet
	  for (int i = 0; i < size; ++i, ptr += 3)
	    {
	      ptr[0] = pData[compTable[0]];
	      ptr[1] = pData[compTable[1]];
	      ptr[2] = pData[compTable[2]];
	      pData += 3;
	    }
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGA32bitsRLE
//
// Read 32 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGA32bitsRLE (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;
  int *compTable = GLEW_EXT_bgra ? bgraTable : rgbaTable;

  while (ptr < _pixels + (_width * _height) * 4)
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet */
	  for (int i = 0; i < size; ++i, ptr += 4)
	    {
	      ptr[0] = pData[compTable[0]];
	      ptr[1] = pData[compTable[1]];
	      ptr[2] = pData[compTable[2]];
	      ptr[3] = pData[compTable[3]];
	    }

	  pData += 4;
	}
      else
	{
	  // Non run-length packet
	  for (int i = 0; i < size; ++i, ptr += 4)
	    {
	      ptr[0] = pData[compTable[0]];
	      ptr[1] = pData[compTable[1]];
	      ptr[2] = pData[compTable[2]];
	      ptr[3] = pData[compTable[3]];
	      pData += 4;
	    }
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGAgray8bitsRLE
//
// Read grey 8 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGAgray8bitsRLE (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;

  while (ptr < _pixels + (_width * _height))
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet
	  GLubyte color = *(pData++);

	  memset (ptr, color, size);
	  ptr += size;
	}
      else
	{
	  // Non run-length packet
	  memcpy (ptr, pData, size);
	  ptr += size;
	  pData += size;
	}
    }
}


// --------------------------------------------------------------------------
// ImageTGA::readTGAgray16bitsRLE
//
// Read grey 16 bits pixel data from TGA file with RLE compression.
// --------------------------------------------------------------------------

void
ImageTGA::readTGAgray16bitsRLE (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;

  while (ptr < _pixels + (_width * _height) * 2)
    {
      // Read first byte
      GLubyte packet_header = *(pData++);
      int size = 1 + (packet_header & 0x7f);

      if (packet_header & 0x80)
	{
	  // Run-length packet
	  GLubyte color = *(pData++);
	  GLubyte alpha = *(pData++);

	  for (int i = 0; i < size; ++i, ptr += 2)
	    {
	      ptr[0] = color;
	      ptr[1] = alpha;
	    }
	}
      else
	{
	  // Non run-length packet
	  memcpy (ptr, pData, size * 2);
	  ptr += size * 2;
	  pData += size * 2;
	}
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// class ImagePCX implementation.
//
/////////////////////////////////////////////////////////////////////////////

// Pixel's component table access
int ImagePCX::rgbTable[3] = { 0, 1, 2 };
int ImagePCX::bgrTable[3] = { 2, 1, 0 };


// --------------------------------------------------------------------------
// ImagePCX::ImagePCX
//
// Constructor.  Read a PCX image from memory.
// --------------------------------------------------------------------------

ImagePCX::ImagePCX (const ImageBuffer &ibuff)
  : _header (NULL)
{
  const GLubyte *data_ptr;

  try
    {
      _name = ibuff.filename ();
      _standardCoordSystem = true;

      // Get pointer on file data
      data_ptr = ibuff.data ();

      // Read PCX header
      _header = reinterpret_cast<const PCX_Header *>(data_ptr);
      data_ptr += sizeof (PCX_Header);

      // Check if is valid PCX file
      if (_header->manufacturer != 0x0a)
	throw ImageException ("Bad version number", _name);

      // Initialize image variables
      _width = _header->xmax - _header->xmin + 1;
      _height = _header->ymax - _header->ymin + 1;
      _format = GLEW_EXT_bgra ? GL_BGR : GL_RGB;
      _components = 3;
      _pixels = new GLubyte[_width * _height * _components];

      int bitcount = _header->bitsPerPixel * _header->numColorPlanes;
      int palette_pos = ibuff.length () - 768;

      // Read image data
      switch (bitcount)
	{
	case 1:
	  // 1 bit color index
	  readPCX1bit (data_ptr);
	  break;

	case 4:
	  // 4 bits color index
	  readPCX4bits (data_ptr);
	  break;

	case 8:
	  // 8 bits color index
	  readPCX8bits (data_ptr, ibuff.data () + palette_pos);
	  break;

	case 24:
	  // 24 bits
	  readPCX24bits (data_ptr);
	  break;

	default:
	  // Unsupported
	  throw ImageException ("Unhandled PCX format (bad bitcount)", _name);
	}
    }
  catch (...)
    {
      delete [] _pixels;
      throw;
    }
}


// --------------------------------------------------------------------------
// ImagePCX::readPCX1bit
//
// Read 1 bit PCX image.
// --------------------------------------------------------------------------

void
ImagePCX::readPCX1bit (const GLubyte *data)
{
  int rle_count = 0, rle_value = 0;
  const GLubyte *pData = data;
  GLubyte *ptr = _pixels;
  int *compTable = GLEW_EXT_bgra ? bgrTable : rgbTable;

  for (int y = 0; y < _height; ++y)
    {
      ptr = &_pixels[(_height - (y + 1)) * _width * 3];
      int bytes = _header->bytesPerScanLine;

      // Decode line number y
      while (bytes--)
	{
	  if (rle_count == 0)
	    {
	      if ( (rle_value = *(pData++)) < 0xc0)
		{
		  rle_count = 1;
		}
	      else
		{
		  rle_count = rle_value - 0xc0;
		  rle_value = *(pData++);
		}
	    }

	  rle_count--;

	  // Fill height pixels chunk
	  for (int i = 7; i >= 0; --i, ptr += 3)
	    {
	      int colorIndex = ((rle_value & (1 << i)) > 0);

	      ptr[0] = _header->palette[colorIndex * 3 + compTable[0]];
	      ptr[1] = _header->palette[colorIndex * 3 + compTable[1]];
	      ptr[2] = _header->palette[colorIndex * 3 + compTable[2]];
	    }
	}
    }
}


// --------------------------------------------------------------------------
// ImagePCX::readPCX4bits
//
// Read 4 bits PCX image.
// --------------------------------------------------------------------------

void
ImagePCX::readPCX4bits (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *colorIndex = NULL;
  GLubyte *line = NULL;
  GLubyte *ptr;
  int rle_count = 0, rle_value = 0;
  int *compTable = GLEW_EXT_bgra ? bgrTable : rgbTable;

  try
    {
      // Memory allocation for temporary buffers
      colorIndex = new GLubyte[_width];
      line = new GLubyte[_header->bytesPerScanLine];
    }
  catch (std::bad_alloc &err)
    {
      delete [] colorIndex;
      delete [] line;
      throw;
    }

  for (int y = 0; y < _height; ++y)
    {
      ptr = &_pixels[(_height - (y + 1)) * _width * 3];
      memset (colorIndex, 0, _width);

      for (int c = 0; c < 4; ++c)
	{
	  GLubyte *pLine = line;
	  int bytes = _header->bytesPerScanLine;

	  // Decode line number y
	  while (bytes--)
	    {
	      if (rle_count == 0)
		{
		  if ( (rle_value = *(pData++)) < 0xc0)
		    {
		      rle_count = 1;
		    }
		  else
		    {
		      rle_count = rle_value - 0xc0;
		      rle_value = *(pData++);
		    }
		}

	      rle_count--;
	      *(pLine++) = rle_value;
	    }

	  // Compute line's color indexes
	  for (int x = 0; x < _width; ++x)
	    {
	      if (line[x / 8] & (128 >> (x % 8)))
		colorIndex[x] += (1 << c);
	    }
	}

      // Decode scanline.  color index => rgb
      for (int x = 0; x < _width; ++x, ptr += 3)
	{
	  ptr[0] = _header->palette[colorIndex[x] * 3 + compTable[0]];
	  ptr[1] = _header->palette[colorIndex[x] * 3 + compTable[1]];
	  ptr[2] = _header->palette[colorIndex[x] * 3 + compTable[2]];
	}
    }

  // Release memory
  delete [] colorIndex;
  delete [] line;
}


// --------------------------------------------------------------------------
// ImagePCX::readPCX8bits
//
// Read 8 bits PCX image.
// --------------------------------------------------------------------------

void
ImagePCX::readPCX8bits (const GLubyte *data, const GLubyte *palette)
{
  const GLubyte *pData = data;
  int rle_count = 0, rle_value = 0;
  GLubyte *ptr;
  int *compTable = GLEW_EXT_bgra ? bgrTable : rgbTable;

  // Palette should be preceded by a value of 0x0c (12)...
  GLubyte magic = palette[-1];
  if (magic != 0x0c)
    {
      // ... but sometimes it is not
      std::cerr << "Warning: PCX palette should start with "
		<< "a value of 0x0c (12)!" << std::endl;
    }

  // Read pixel data
  for (int y = 0; y < _height; ++y)
    {
      ptr = &_pixels[(_height - (y + 1)) * _width * 3];
      int bytes = _header->bytesPerScanLine;

      // Decode line number y
      while (bytes--)
	{
	  if (rle_count == 0)
	    {
	      if( (rle_value = *(pData++)) < 0xc0)
		{
		  rle_count = 1;
		}
	      else
		{
		  rle_count = rle_value - 0xc0;
		  rle_value = *(pData++);
		}
	    }

	  rle_count--;

	  ptr[0] = palette[rle_value * 3 + compTable[0]];
	  ptr[1] = palette[rle_value * 3 + compTable[1]];
	  ptr[2] = palette[rle_value * 3 + compTable[2]];
	  ptr += 3;
	}
    }
}


// --------------------------------------------------------------------------
// ImagePCX::readPCX24bits
//
// Read 24 bits PCX image.
// --------------------------------------------------------------------------

void
ImagePCX::readPCX24bits (const GLubyte *data)
{
  const GLubyte *pData = data;
  GLubyte *ptr;
  int rle_count = 0, rle_value = 0;
  int *compTable = GLEW_EXT_bgra ? bgrTable : rgbTable;

  for (int y = 0; y < _height; ++y)
    {
      // For each color plane
      for (int c = 0; c < 3; ++c)
	{
	  ptr = &_pixels[(_height - (y + 1)) * _width * 3];
	  int bytes = _header->bytesPerScanLine;

	  // Decode line number y
	  while (bytes--)
	    {
	      if (rle_count == 0)
		{
		  if( (rle_value = *(pData++)) < 0xc0)
		    {
		      rle_count = 1;
		    }
		  else
		    {
		      rle_count = rle_value - 0xc0;
		      rle_value = *(pData++);
		    }
		}

	      rle_count--;
	      ptr[compTable[c]] = static_cast<GLubyte>(rle_value);
	      ptr += 3;
	    }
	}
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// class ImageJPEG implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ImageJPEG::ImageJPEG
//
// Constructor.  Read a JPEG image from memory, using libjpeg.
// --------------------------------------------------------------------------

ImageJPEG::ImageJPEG (const ImageBuffer &ibuff)
{
  jpeg_decompress_struct cinfo;
  my_error_mgr jerr;
  jpeg_source_mgr jsrc;
  JSAMPROW j;

  try
    {
      _name = ibuff.filename ();
      _standardCoordSystem = true;

      // Create and configure decompress object
      jpeg_create_decompress (&cinfo);
      cinfo.err = jpeg_std_error (&jerr.pub);
      cinfo.src = &jsrc;

      // Configure error manager
      jerr.pub.error_exit = errorExit_callback;
      jerr.pub.output_message = outputMessage_callback;

      if (setjmp (jerr.setjmp_buffer))
	throw ImageException (jerr.errorMsg, _name);

      // Configure source manager
      jsrc.next_input_byte = ibuff.data ();
      jsrc.bytes_in_buffer = ibuff.length ();

      jsrc.init_source = initSource_callback;
      jsrc.fill_input_buffer = fillInputBuffer_callback;
      jsrc.skip_input_data = skipInputData_callback;
      jsrc.resync_to_restart = jpeg_resync_to_restart;
      jsrc.term_source = termSource_callback;

      // Read file's header and prepare for decompression
      jpeg_read_header (&cinfo, TRUE);
      jpeg_start_decompress (&cinfo);

      // Initialize image's member variables
      _width = cinfo.image_width;
      _height = cinfo.image_height;
      _components = cinfo.num_components;
      _format = (cinfo.num_components == 1) ? GL_LUMINANCE : GL_RGB;
      _pixels = new GLubyte[_width * _height * _components];

      // Read scanlines
      for (int i = 0; i < _height; ++i)
	{
	  //j = &_pixels[_width * i * _components];
	  j = (_pixels + ((_height - (i + 1)) * _width * _components));
	  jpeg_read_scanlines (&cinfo, &j, 1);
	}

      // Finish decompression and release memory
      jpeg_finish_decompress (&cinfo);
      jpeg_destroy_decompress (&cinfo);
    }
  catch (...)
    {
      delete [] _pixels;
      jpeg_destroy_decompress (&cinfo);

      throw;
    }
}


// --------------------------------------------------------------------------
// ImageJPEG::initSource_callback
// ImageJPEG::fillInputBuffer_callback
// ImageJPEG::skipInputData_callback
// ImageJPEG::termSource_callback
//
// Callback functions used by libjpeg for reading data from memory.
// --------------------------------------------------------------------------

void
ImageJPEG::initSource_callback (j_decompress_ptr cinfo)
{
  // Nothing to do here
}


boolean
ImageJPEG::fillInputBuffer_callback (j_decompress_ptr cinfo)
{
  JOCTET eoi_buffer[2] = { 0xFF, JPEG_EOI };
  struct jpeg_source_mgr *jsrc = cinfo->src;

  // Create a fake EOI marker
  jsrc->next_input_byte = eoi_buffer;
  jsrc->bytes_in_buffer = 2;

  return TRUE;
}


void
ImageJPEG::skipInputData_callback (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *jsrc = cinfo->src;

  if (num_bytes > 0)
    {
      while (num_bytes > static_cast<long>(jsrc->bytes_in_buffer))
	{
	  num_bytes -= static_cast<long>(jsrc->bytes_in_buffer);
	  fillInputBuffer_callback (cinfo);
	}

      jsrc->next_input_byte += num_bytes;
      jsrc->bytes_in_buffer -= num_bytes;
    }
}


void
ImageJPEG::termSource_callback (j_decompress_ptr cinfo)
{
  // Nothing to do here
}


// --------------------------------------------------------------------------
// ImageJPEG::errorExit
// ImageJPEG::outputMessage
//
// Callback functions used by libjpeg for error handling.
// --------------------------------------------------------------------------

void
ImageJPEG::errorExit_callback (j_common_ptr cinfo)
{
  my_error_ptr jerr = reinterpret_cast<my_error_ptr>(cinfo->err);

  // Create the error message
  char message[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message) (cinfo, message);
  jerr->errorMsg.assign (message);

  // Return control to the setjmp point
  longjmp (jerr->setjmp_buffer, 1);
}


void
ImageJPEG::outputMessage_callback (j_common_ptr cinfo)
{
  my_error_ptr jerr = reinterpret_cast<my_error_ptr>(cinfo->err);

  // Create the error message
  char message[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message) (cinfo, message);
  jerr->errorMsg.assign (message);

  // Send it to stderr, adding a newline
  std::cerr << "libjpeg: " << jerr->errorMsg << std::endl;
}


/////////////////////////////////////////////////////////////////////////////
//
// class ImagePNG implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// ImagePNG::ImagePNG
//
// Constructor.  Read a PNG image from memory, using libpng.
// --------------------------------------------------------------------------

ImagePNG::ImagePNG (const ImageBuffer &ibuff)
{
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_bytep *row_pointers = NULL;
  int bit_depth, color_type;
  my_source_mgr src_mgr (ibuff);

  try
    {
      _name = ibuff.filename ();
      _standardCoordSystem = true;

      png_byte sig[8];
      memcpy (sig, reinterpret_cast<const png_byte*>(ibuff.data ()), 8);

      // Check for valid magic number
      if (!png_sig_cmp (sig, 0, 8))
	throw ImageException ("Not a valid PNG file", _name);

      // Create PNG read struct
      png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, &errorMsg,
					error_callback, warning_callback);
      if (!png_ptr)
	throw ImageException ("Failed to create png read struct", _name);

      if (setjmp (png_jmpbuf (png_ptr)))
	throw ImageException (errorMsg, _name);

      // Create PNG info struct
      info_ptr = png_create_info_struct (png_ptr);
      if (!info_ptr)
	throw ImageException ("Failed to create png info struct", _name);

      // Set "read" callback function and give source of data
      png_set_read_fn (png_ptr, &src_mgr, read_callback);

      // Read png info
      png_read_info (png_ptr, info_ptr);

      // Get some usefull information from header
      bit_depth = png_get_bit_depth (png_ptr, info_ptr);
      color_type = png_get_color_type (png_ptr, info_ptr);

      // Convert index color images to RGB images
      if (color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_palette_to_rgb (png_ptr);

      // Convert 1-2-4 bits grayscale images to 8 bits
      // grayscale.
      if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	png_set_expand_gray_1_2_4_to_8(png_ptr);

      if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha (png_ptr);

      if (bit_depth == 16)
	png_set_strip_16 (png_ptr);
      else if (bit_depth < 8)
	png_set_packing (png_ptr);

      // Update info structure to apply transformations
      png_read_update_info (png_ptr, info_ptr);

      // Get updated information
      png_get_IHDR (png_ptr, info_ptr,
		    reinterpret_cast<png_uint_32*>(&_width),
		    reinterpret_cast<png_uint_32*>(&_height),
		    &bit_depth, &color_type,
		    NULL, NULL, NULL);

      // Get image format and components per pixel
      getTextureInfo (color_type);

      // Memory allocation for storing pixel data
      _pixels = new GLubyte[_width * _height * _components];

      // Pointer array.  Each one points at the begening of a row.
      row_pointers = new png_bytep[_height];

      for (int i = 0; i < _height; ++i)
	{
	  row_pointers[i] = (png_bytep)(_pixels +
		((_height - (i + 1)) * _width * _components));
	}

      // Read pixel data using row pointers
      png_read_image (png_ptr, row_pointers);

      // Finish decompression and release memory
      png_read_end (png_ptr, NULL);
      png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

      delete [] row_pointers;
    }
  catch (...)
    {
      delete [] _pixels;
      delete [] row_pointers;

      if (png_ptr)
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

      throw;
    }
}


// --------------------------------------------------------------------------
// ImagePNG::getTextureInfo
//
// Extract OpenGL texture informations from PNG info.
// --------------------------------------------------------------------------

void
ImagePNG::getTextureInfo (int color_type)
{
  switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
      _format = GL_LUMINANCE;
      _components = 1;
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      _format = GL_LUMINANCE_ALPHA;
      _components = 2;
      break;

    case PNG_COLOR_TYPE_RGB:
      _format = GL_RGB;
      _components = 3;
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
      _format = GL_RGBA;
      _components = 4;
      break;

    default:
      // Badness
      throw ImageException ("Bad PNG color type", _name);
    }
}


// --------------------------------------------------------------------------
// ImagePNG::read_callback
// ImagePNG::error_callback
// ImagePNG::warning_callback
//
// Callback functions used by libpng for reading data from memory
// and error handling.
// --------------------------------------------------------------------------

void
ImagePNG::read_callback (png_structp png_ptr, png_bytep data, png_size_t length)
{
  my_source_ptr src = static_cast<my_source_ptr>(png_ptr->io_ptr);

  // Copy data from image buffer
  memcpy (data, src->pibuff->data () + src->offset, length);

  // Advance in the file
  src->offset += length;
}


void
ImagePNG::error_callback (png_structp png_ptr, png_const_charp error_msg)
{
  static_cast<string *>(png_ptr->error_ptr)->assign (error_msg);

  longjmp (png_jmpbuf (png_ptr), 1);
}


void
ImagePNG::warning_callback (png_structp png_ptr, png_const_charp warning_msg)
{
  std::cerr << "libpng: " << warning_msg << std::endl;
}

}}} //end namespace
