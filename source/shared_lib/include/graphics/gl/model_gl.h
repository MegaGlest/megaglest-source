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

#ifndef _SHARED_GRAPHICS_GL_MODELGL_H_
#define _SHARED_GRAPHICS_GL_MODELGL_H_

#include "leak_dumper.h"
#include "model.h"

namespace Shared {
namespace Graphics {
namespace Gl {

// =====================================================
//	class ModelGl
// =====================================================

class ModelGl : public Model {
  friend class GraphicsFactoryGl;

protected:
  ModelGl(const string &path, TextureManager *textureManager,
          bool deletePixMapAfterLoad,
          std::map<string, vector<pair<string, string>>> *loadedFileList,
          string *sourceLoader);

public:
  virtual void init() {}
  virtual void end() {}
};

} // namespace Gl
} // namespace Graphics
} // namespace Shared

#endif
