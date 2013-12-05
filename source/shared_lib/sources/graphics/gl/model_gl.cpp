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

#include "model_gl.h"

namespace Shared{ namespace Graphics{ namespace Gl{
    
ModelGl::ModelGl(const string &path,TextureManager* textureManager,bool deletePixMapAfterLoad,std::map<string,vector<pair<string, string> > > *loadedFileList, string *sourceLoader) {
	setTextureManager(textureManager);
	load(path,deletePixMapAfterLoad,loadedFileList,sourceLoader);
}
    
}}}//end namespace
