// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// TextureManager.h -- Copyright (c) 2006 David Henry
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
// Definitions of a texture manager class.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __TEXTUREMANAGER_H__
#define __TEXTUREMANAGER_H__

#include "DataManager.h"
#include "md5Texture.h"

namespace Shared { namespace Graphics { namespace md5 {

/////////////////////////////////////////////////////////////////////////////
//
// class Texture2DManager -- a texture manager which can register/unregister
// Texture2D objects.  Destroy all registred textures at death.
//
// The texture manager is a singleton.
//
/////////////////////////////////////////////////////////////////////////////
class Texture2DManager :
  public DataManager<Texture2D, Texture2DManager> {
  friend class DataManager<Texture2D, Texture2DManager>;

public:
  // Public interface

  // Load and register a texture.  If the texture has already been
  // loaded previously, return it instead of loading it.
  Texture2D *load (const string &filename)
  {
    // Look for the texture in the registry
    Texture2D *tex = request (filename);

    // If not found, load the texture
    if (tex == NULL)
      {
	tex = new Texture2D (filename);

	// If the texture creation failed, delete the
	// unusable object and return NULL
	if (tex->fail ())
	  {
	    delete tex;
	    tex = NULL;
	  }
	else
	  {
	    // The texture has been successfully loaded,
	    // register it.
	    registerObject (tex->name (), tex);
	  }
      }

    return tex;
  }
};

}}} //end namespace

#endif // __TEXTUREMANAGER_H__
