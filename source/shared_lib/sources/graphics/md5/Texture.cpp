// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Texture.cpp -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// Implementation of an OpenGL texture classes.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/glew.h>
#include <iostream>
#include <stdexcept>

#include "GlErrors.h"
#include "Texture.h"
#include "Image.h"

namespace Shared { namespace Graphics { namespace md5 {

using std::cout;
using std::cerr;
using std::endl;

/////////////////////////////////////////////////////////////////////////////
//
// class Texture implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Texture::Texture
//
// Constructor.
// --------------------------------------------------------------------------
Texture::Texture ()
  : _handle (0), _flags (kDefault),
    _standardCoordSystem (true), _fail (true) {
  // Inhibit possible previous OpenGL error
  checkOpenGLErrors (__FILE__, __LINE__);
}

// --------------------------------------------------------------------------
// Texture::~Texture
//
// Destructor.  Delete texture object.
// --------------------------------------------------------------------------
Texture::~Texture () {
  // Delete texture object
  if (glIsTexture(_handle))
    glDeleteTextures (1, &_handle);
}

// --------------------------------------------------------------------------
// Texture::bind
//
// Bind texture to the active texture unit.
// --------------------------------------------------------------------------
void Texture::bind () const {
  glBindTexture (target (), _handle);
}

void Texture::bind (GLenum texUnit) const {
  glActiveTexture (texUnit);
  glBindTexture (target (), _handle);
}


// --------------------------------------------------------------------------
// Texture::getCompressionFormat
//
// Return the corresponding format for a compressed image given
// image's internal format (pixel components).
// --------------------------------------------------------------------------
GLint Texture::getCompressionFormat (GLint internalFormat) {
  if (!GLEW_EXT_texture_compression_s3tc ||
      !GLEW_ARB_texture_compression)
    // No compression possible on this target machine
    return internalFormat;

  switch (internalFormat)
    {
    case 1:
      return GL_COMPRESSED_LUMINANCE;

    case 2:
      return GL_COMPRESSED_LUMINANCE_ALPHA;

    case 3:
      return GL_COMPRESSED_RGB;

    case 4:
      return GL_COMPRESSED_RGBA;

    default:
      // Error!
      throw std::invalid_argument ("Texture::getCompressionFormat: "
				   "Bad internal format");
    }
}


// --------------------------------------------------------------------------
// Texture::getInternalFormat
//
// Return texture's internal format depending to whether compression
// is used or not.
// --------------------------------------------------------------------------
GLint Texture::getInternalFormat (GLint components) {
  if (_flags & kCompress)
    return getCompressionFormat (components);
  else
    return components;
}

/////////////////////////////////////////////////////////////////////////////
//
// class Texture2D implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Texture2D::Texture2D
//
// Constructors.  Try to load a texture 2D.  Don't throw any exception
// if it fails to create the texture; the program can still run whithout
// textures.
// --------------------------------------------------------------------------
Texture2D::Texture2D () : Texture () {
}

Texture2D::Texture2D (const string &filename, TextureFlags flags) {
  try
    {
      // Load image file into a buffer
      ImageBuffer ibuff (filename);
      auto_ptr<Image> img (ImageFactory::createImage (ibuff));

      // Create texture from image buffer
      create (img.get (), flags);
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture 2D from " << filename
	   << endl << "Reason: " << err.what () << endl;
    }
}


Texture2D::Texture2D (const Image *img, TextureFlags flags)
{
  try
    {
      // Create texture from image buffer
      create (img, flags);
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture 2D from " << img->name ()
	   << endl << "Reason: " << err.what () << endl;
    }
}


// --------------------------------------------------------------------------
// Texture2D::create
//
// Create a texture 2D from an image.
// --------------------------------------------------------------------------

void
Texture2D::create (const Image *img, TextureFlags flags)
{
  if (!img->pixels ())
    throw std::runtime_error ("Invalid image data");

  // Fill texture's vars
  _name = img->name ();
  _flags = flags;
  _standardCoordSystem = img->stdCoordSystem ();

  // Generate a texture name
  glGenTextures (1, &_handle);
  glBindTexture (target (), _handle);

  // Setup texture filters
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (img->isCompressed ())
    {
      // Image use S3 compression.  Only S3TC DXT1, DXT3
      // and DXT5 formats are supported.
      GLsizei mipWidth = img->width ();
      GLsizei mipHeight = img->height ();
      int offset = 0;
      int blockSize = (img->format () == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

      // Upload mipmaps to video memory
      for (GLint mipLevel = 0; mipLevel < img->numMipmaps (); ++mipLevel)
	{
	  GLsizei mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;

	  glCompressedTexImage2D (GL_TEXTURE_2D, mipLevel, img->format (),
				  mipWidth, mipHeight, 0, mipSize,
				  img->pixels () + offset);

	  mipWidth = std::max (mipWidth >> 1, 1);
	  mipHeight = std::max (mipHeight >> 1, 1);

	  offset += mipSize;
	}
    }
  else
    {
      // Build the texture and generate mipmaps
      if (GLEW_SGIS_generate_mipmap && img->isPowerOfTwo ())
	{
	  // Hardware mipmap generation
	  glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	  glHint (GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);

	  glTexImage2D (GL_TEXTURE_2D, 0, getInternalFormat (img->components ()),
			img->width (), img->height (), 0, img->format (),
			GL_UNSIGNED_BYTE, img->pixels ());
	}
      else
	{
	  // No hardware mipmap generation support, fall back to the
	  // good old gluBuild2DMipmaps function
	  gluBuild2DMipmaps (GL_TEXTURE_2D, getInternalFormat (img->components ()),
			     img->width (), img->height (), img->format (),
			     GL_UNSIGNED_BYTE, img->pixels ());
	}
    }

  // Does texture creation succeeded?
  if (GL_NO_ERROR == checkOpenGLErrors (__FILE__, __LINE__))
    _fail = false;
}


/////////////////////////////////////////////////////////////////////////////
//
// class TextureRectangle implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// TextureRectangle::TextureRectangle
//
// Constructors.  Try to load a texture rectangle.  Don't throw any
// exception if it fails to create the texture; the program can still
// run whithout textures.
// --------------------------------------------------------------------------

TextureRectangle::TextureRectangle (const string &filename, TextureFlags flags)
  : _width (0), _height (0)
{
  try
    {
      // Check if is not a DDS image
      if (filename.find (".dds") != string::npos)
	throw ImageException ("Compressed textures are not supported for"
			      " texture rectangles!", filename);

      // Load image file into a buffer
      ImageBuffer ibuff (filename);
      auto_ptr<Image> img (ImageFactory::createImage (ibuff));

      // Create texture from image buffer
      create (img.get (), flags);
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture rectangle from "
	   << filename << endl << "Reason: " << err.what () << endl;
    }
}


TextureRectangle::TextureRectangle (const Image *img, TextureFlags flags)
  : _width (0), _height (0)
{
  try
    {
      // Check if is not a compressed DDS image
      if (img->isCompressed ())
	throw ImageException ("Compressed textures are not supported for"
			      " texture rectangles!", img->name ());

      // Create texture from image buffer
      create (img, flags);
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture rectangle from "
	   << img->name () << endl << "Reason: " << err.what () << endl;
    }
}


// --------------------------------------------------------------------------
// TextureRectangle::create
//
// Create a texture rectangle from an image.  No mipmap
// filtering is permitted for texture rectangles.
// --------------------------------------------------------------------------

void
TextureRectangle::create (const Image *img, TextureFlags flags)
{
  if (!img->pixels ())
    throw std::runtime_error ("Invalid image data");

  // Get image info
  _standardCoordSystem = img->stdCoordSystem ();
  _width = img->width ();
  _height = img->height ();
  _name = img->name ();
  _flags = flags;

  // Generate a texture name
  glGenTextures (1, &_handle);
  glBindTexture (target (), _handle);

  // Setup texture filters
  glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Create texture from image
  glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0,
		getInternalFormat (img->components ()),
		img->width (), img->height (), 0, img->format (),
		GL_UNSIGNED_BYTE, img->pixels ());

  // Does texture creation succeeded?
  if (GL_NO_ERROR == checkOpenGLErrors (__FILE__, __LINE__))
    _fail = false;
}


/////////////////////////////////////////////////////////////////////////////
//
// class TextureCubeMap implementation.
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// TextureCubeMap::TextureCubeMap
//
// Constructors.  Try to load a texture cube map.  Don't throw any
// exception if it fails to create the texture; the program can still
// run whithout textures.
// --------------------------------------------------------------------------

TextureCubeMap::TextureCubeMap (const string &basename, const vector<string> &files,
				TextureFlags flags)
{
  try
    {
      _name = basename;

      vector<ImagePtr> faces;
      faces.reserve(6);

      // Load images
      for (int i = 0; i < 6; ++i)
	{
	  ImageBuffer ibuff (files[i]);
	  faces.push_back (ImagePtr (ImageFactory::createImage (ibuff)));
	}

      // Create texture from faces
      create (faces, flags);
    }
  catch (ImageException &err)
    {
      cerr << "Error: Couldn't create texture cube map from " << basename << endl;
      cerr << "Reason: " << err.what () << " (" << err.which () << ")" << endl;
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture cube map from " << basename << endl;
      cerr << "Reason: " << err.what () << endl;
    }
}


TextureCubeMap::TextureCubeMap (const string &basename, const vector<ImagePtr> &faces,
				TextureFlags flags)
{
  try
    {
      _name = basename;

      // Create texture from image buffer
      create (faces, flags);
    }
  catch (ImageException &err)
    {
      cerr << "Error: Couldn't create texture cube map from " << basename << endl;
      cerr << "Reason: " << err.what () << " (" << err.which () << ")" << endl;
    }
  catch (std::exception &err)
    {
      cerr << "Error: Couldn't create texture cube map from " << basename << endl;
      cerr << "Reason: " << err.what () << endl;
    }
}


// --------------------------------------------------------------------------
// TextureCubeMap::create
//
// Create a cube map texture from images.
// --------------------------------------------------------------------------

void
TextureCubeMap::create (const vector<ImagePtr> &faces, TextureFlags flags)
{
  // Create a list of image buffers and associate a target
  // for each one
  typedef map<const Image*, GLenum> TexTarget;
  typedef TexTarget::value_type TexPair;
  TexTarget texImages;

  texImages.insert (TexPair (faces[0].get(), GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB));
  texImages.insert (TexPair (faces[1].get(), GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB));
  texImages.insert (TexPair (faces[2].get(), GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB));
  texImages.insert (TexPair (faces[3].get(), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB));
  texImages.insert (TexPair (faces[4].get(), GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB));
  texImages.insert (TexPair (faces[5].get(), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB));

  _standardCoordSystem = faces[0]->stdCoordSystem ();
  _flags = flags;

  // Generate a texture name
  glGenTextures (1, &_handle);
  glBindTexture (GL_TEXTURE_CUBE_MAP_ARB, _handle);

  // Setup texture filters
  glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


  // Load each side of the cube
  for (TexTarget::iterator itor = texImages.begin ();
       itor != texImages.end (); ++itor)
    {
      // Get image data
      const Image *img = itor->first;

      if (!img->pixels ())
	throw std::runtime_error ("Invalid image data");

      if (img->isCompressed ())
	{
	  // Image use S3 compression
	  GLsizei mipWidth = img->width ();
	  GLsizei mipHeight = img->height ();
	  int offset = 0;
	  int blockSize =
	    (img->format () == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

	  // Upload mipmaps to video memory
	  for (GLint mipLevel = 0; mipLevel < img->numMipmaps (); ++mipLevel)
	    {
	      GLsizei mipSize = ((mipWidth + 3) / 4) *
		((mipHeight + 3) / 4) * blockSize;

	      glCompressedTexImage2D (itor->second, mipLevel, img->format (),
				      mipWidth, mipHeight, 0, mipSize,
				      img->pixels () + offset);

	      mipWidth = std::max (mipWidth >> 1, 1);
	      mipHeight = std::max (mipHeight >> 1, 1);

	      offset += mipSize;
	    }
	}
      else
	{
	  // No hardware mipmap generation support for texture cube
	  // maps, use gluBuild2DMipmaps function instead
	  gluBuild2DMipmaps (itor->second, getInternalFormat (img->components ()),
			     img->width (), img->height (), img->format (),
			     GL_UNSIGNED_BYTE, img->pixels ());
	}
    }

  // Does texture creation succeeded?
  if (GL_NO_ERROR == checkOpenGLErrors (__FILE__, __LINE__))
    _fail = false;
}

}}} //end namespace
