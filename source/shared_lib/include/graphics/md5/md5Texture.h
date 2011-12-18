// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// Texture.h -- Copyright (c) 2006 David Henry
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
// Definition of an OpenGL texture classes.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/glew.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include "Image.h"

namespace Shared { namespace Graphics { namespace md5 {

using std::string;
using std::vector;
using std::map;
using std::auto_ptr;

/////////////////////////////////////////////////////////////////////////////
// Texture class diagram:
//
//                            +--------- (abs)
//                            |  Texture  |
//                            +-----------+
//                                  ^
//                                  |
//            +---------------------+----------------------+
//            |                     |                      |
//     +-----------+      +------------------+    +----------------+
//     | Texture2D |      | TextureRectangle |    | TextureCubeMap |
//     +-----------+      +------------------+    +----------------+
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// class Texture -- OpenGL texture base class.  This is an abstract
// class for more specialized OpenGL texture classes.
//
/////////////////////////////////////////////////////////////////////////////
class Texture {
public:
  // Constructor/destructor
  Texture ();
  virtual ~Texture ();

public:
  // Constants
  enum  {
    //Default behaviour
    kDefault  = 0,

    // Use texture compression
    kCompress = (1 << 0),
  };

  typedef int TextureFlags;

public:
  // Public interface
  void bind () const;
  void bind (GLenum texUnit) const;
  bool fail () const { return _fail; }
  bool stdCoordSystem () const { return _standardCoordSystem; }

  // Accessors
  const string &name () const { return _name; }
  const GLuint handle () const { return _handle; }

  virtual GLenum target () const = 0;

private:
  // Copy operations are not allowed for textures, because
  // when the source texture is destroyed, it releases
  // its texture handle and so dest texture's handle is
  // not valid anymore.
  Texture (const Texture &);
  Texture &operator= (const Texture &);

protected:
  // Internal functions
  GLubyte *loadImageFile (const string &filename);
  GLint getCompressionFormat (GLint internalFormat);
  GLint getInternalFormat (GLint components);

protected:
  // Member variables
  string _name;
  GLuint _handle;

  TextureFlags _flags;
  bool _standardCoordSystem;
  bool _fail;
};


/////////////////////////////////////////////////////////////////////////////
//
// class Texture2D -- OpenGL texture 2D object.
//
/////////////////////////////////////////////////////////////////////////////
class Texture2D : public Texture {
public:
  // Constructors
  Texture2D (const string &filename, TextureFlags flags = kDefault);
  Texture2D (const Image *img, TextureFlags flags = kDefault);

protected:
  // Default constructor is not public
  Texture2D ();

  // Internal functions
  virtual void create (const Image *img, TextureFlags flags);

public:
  // Accessors
  virtual GLenum target () const { return GL_TEXTURE_2D; }
};

/////////////////////////////////////////////////////////////////////////////
//
// class TextureRectangle -- OpenGL texture rectangle object.
//
/////////////////////////////////////////////////////////////////////////////
class TextureRectangle : public Texture {
public:
  // Constructors
  TextureRectangle (const string &filename, TextureFlags flags = kDefault);
  TextureRectangle (const Image *img, TextureFlags flags = kDefault);

protected:
  // Internal functions
  virtual void create (const Image *img, TextureFlags flags);

public:
  // Accessors
  virtual GLenum target () const { return GL_TEXTURE_RECTANGLE_ARB; }

  GLint width () const { return _width; }
  GLint height () const { return _height; }

protected:
  // Member variables
  GLint _width;
  GLint _height;
};

/////////////////////////////////////////////////////////////////////////////
//
// class TextureCubeMap -- OpenGL texture cube map object.
// The order of images to pass to constructors is:
//    - positive x, negative x,
//    - positive y, negative y,
//    - positive z, negative z.
//
/////////////////////////////////////////////////////////////////////////////
class TextureCubeMap : public Texture {
public:
  // Constructors
  TextureCubeMap (const string &basename, const vector<string> &files,
		  TextureFlags flags = kDefault);
  TextureCubeMap (const string &basename, const vector<ImagePtr> &faces,
		  TextureFlags flags = kDefault);

protected:
  // Internal function
  virtual void create (const vector<ImagePtr> &faces, TextureFlags flags);

public:
  // Accessors
  virtual GLenum target () const { return GL_TEXTURE_CUBE_MAP_ARB; }
};

}}} //end namespace

#endif // __TEXTURE_H__
