#ifndef _SHARED_D3D9_MODELRENDERERD3D9_H_
#define _SHARED_D3D9_MODELRENDERERD3D9_H_

#include "model_renderer.h"
#include "model.h"

#include <d3d9.h> 

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class ModelRendererD3d9  
// ===============================

class ModelRendererD3d9: public ModelRenderer{
private:
	static const int defBufferPointCount= 100; //buffer size in vertices 
	static const int defBufferIndexCount= 100; //buffer size in vertices 

	struct CustomVertexPNTT{
		Vec3f vertex;
		Vec3f normal;
		Vec2f texCoord;
		Vec3f tangent;
	};

private:
	bool rendering;
	int bufferPointCount;
	int bufferIndexCount;

	IDirect3DDevice9 *d3dDevice;
	IDirect3DVertexBuffer9 *d3dVertexBuffer;
	IDirect3DIndexBuffer9 *d3dIndexBuffer;
	IDirect3DVertexDeclaration9 *d3dVertexDeclarationPNT;
	IDirect3DVertexDeclaration9 *d3dVertexDeclarationPNTT;

public:
	ModelRendererD3d9();
	~ModelRendererD3d9();
	virtual void begin(bool renderNormals= true, bool renderTextures= true, bool renderColors= true);
	virtual void end();
	virtual void render(const Model *model);
	virtual void renderNormalsOnly(const Model *model);

private:
	void renderMesh(const Mesh *mesh);
	void readyBuffers(int newPointCount, int newIndexCount);	
};

}}}//end namespace

#endif