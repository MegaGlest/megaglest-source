// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// TextureManager.h -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
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
