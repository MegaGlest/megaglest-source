#ifndef _SHARED_D3D9_TEXTURED3D9_H_
#define _SHARED_D3D9_TEXTURED3D9_H_

#include "texture.h"
#include <d3d9.h> 

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class Texture2DD3d9  
// ===============================

class Texture2DD3d9: public Texture2D{
private:
	IDirect3DTexture9 *d3dTexture;

public:
	IDirect3DTexture9 *getD3dTexture() const	{return d3dTexture;}

	virtual void init(Filter textureFilter, int maxAnisotropy= 1);
	virtual void end();
};

// ===============================
//	class TextureCubeD3d9  
// ===============================

class TextureCubeD3d9: public TextureCube{
private:
	IDirect3DCubeTexture9 *d3dCubeTexture;

public:
	IDirect3DCubeTexture9 *getD3dCubeTexture() const	{return d3dCubeTexture;}

	virtual void init(Filter textureFilter, int maxAnisotropy= 1);
	virtual void end();
};

}}}//end namespace

#endif