#include "model_renderer_d3d9.h"

#include <cassert>

#include "graphics_interface.h"
#include "context_d3d9.h"
#include "texture_d3d9.h"
#include "interpolation.h"
#include "d3d9_util.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================================
//	class ModelRendererD3d9
// ===============================================

D3DVERTEXELEMENT9 d3dVertexElementsPNT[]=
{
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
	{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};

D3DVERTEXELEMENT9 d3dVertexElementsPNTT[]=
{
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
	{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
	D3DDECL_END()
};

ModelRendererD3d9::ModelRendererD3d9(){
	rendering= false;

	GraphicsInterface &gi= GraphicsInterface::getInstance();
	d3dDevice= static_cast<ContextD3d9*>(gi.getCurrentContext())->getD3dDevice();

	bufferPointCount= 0;
	bufferIndexCount= 0;

	D3DCALL(d3dDevice->CreateVertexDeclaration(d3dVertexElementsPNT, &d3dVertexDeclarationPNT));
	D3DCALL(d3dDevice->CreateVertexDeclaration(d3dVertexElementsPNTT, &d3dVertexDeclarationPNTT));

	readyBuffers(defBufferPointCount, defBufferIndexCount);
}

ModelRendererD3d9::~ModelRendererD3d9(){
	d3dVertexBuffer->Release();
}

void ModelRendererD3d9::begin(bool renderNormals, bool renderTextures, bool renderColors){
	rendering= true;
}

void ModelRendererD3d9::end(){
	rendering= false;
}

void ModelRendererD3d9::render(const Model *model){
	assert(rendering);

	//render every mesh
	for(uint32 i=0; i<model->getMeshCount(); ++i){
		renderMesh(model->getMesh(i));	
	}
}

void ModelRendererD3d9::renderNormalsOnly(const Model *model){
}

// ====================== Private ===============================================

void ModelRendererD3d9::renderMesh(const Mesh *mesh){

	CustomVertexPNTT *vertices;
	uint32 *indices;

	readyBuffers(mesh->getVertexCount(), mesh->getIndexCount());

	//lock vertex buffer
	D3DCALL(d3dVertexBuffer->Lock(0, mesh->getVertexCount()*sizeof(CustomVertexPNTT), (void**) &vertices, 0));

	//copy data vertex buffer
	const InterpolationData *interpolationData= mesh->getInterpolationData();

	for(int i=0; i<mesh->getVertexCount(); ++i){
		vertices[i].vertex= interpolationData->getVertices()[i];
		vertices[i].normal= interpolationData->getNormals()[i];
		Vec2f texCoord= mesh->getTexCoords()[i];
		vertices[i].texCoord= Vec2f(texCoord.x, texCoord.y);
	}
	if(mesh->getTangents()!=NULL){
		for(int i=0; i<mesh->getVertexCount(); ++i){
			vertices[i].tangent= mesh->getTangents()[i];
		}
	}

	//unlock vertex buffer
	D3DCALL(d3dVertexBuffer->Unlock());
	
	//lock index buffer
	D3DCALL(d3dIndexBuffer->Lock(0, mesh->getIndexCount()*sizeof(uint32), (void**) &indices, 0));

	//copy data
	for(int i=0; i<mesh->getIndexCount(); i+=3){
		indices[i]= mesh->getIndices()[i];
		indices[i+1]= mesh->getIndices()[i+2];
		indices[i+2]= mesh->getIndices()[i+1];
	}

	//unlock
	D3DCALL(d3dIndexBuffer->Unlock());

	//set stream data
	D3DCALL(d3dDevice->SetStreamSource(0, d3dVertexBuffer, 0, sizeof(CustomVertexPNTT)));
	D3DCALL(d3dDevice->SetVertexDeclaration(mesh->getTangents()==NULL? d3dVertexDeclarationPNT: d3dVertexDeclarationPNTT));
	D3DCALL(d3dDevice->SetIndices(d3dIndexBuffer));

	//set textures
	int textureUnit= 0;
	for(int i=0; i<meshTextureCount; ++i){
		if(mesh->getTexture(i)!=NULL){
		const Texture2DD3d9* texture2DD3d9= static_cast<const Texture2DD3d9*>(mesh->getTexture(i));
			D3DCALL(d3dDevice->SetTexture(textureUnit, texture2DD3d9->getD3dTexture()));
			++textureUnit;
		}
	}

	//render
	D3DCALL(d3dDevice->BeginScene());
	D3DCALL(d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mesh->getVertexCount(), 0, mesh->getIndexCount()/3));
	D3DCALL(d3dDevice->EndScene());

	//reset textures
	for(int i=0; i<meshTextureCount; ++i){
		D3DCALL(d3dDevice->SetTexture(i, NULL));
	}
}

void ModelRendererD3d9::readyBuffers(int newPointCount, int newIndexCount){
	
	//vertices, if the buffer is to small allocate a new buffer
	if(bufferPointCount<newPointCount){
		bufferPointCount= newPointCount;
		
		D3DCALL(d3dDevice->CreateVertexBuffer( 
			bufferPointCount*sizeof(CustomVertexPNTT),
			0, 
			0, 
			D3DPOOL_MANAGED, 
			&d3dVertexBuffer, 
			NULL));
	}

	//indices
	if(bufferIndexCount<newIndexCount){
		bufferIndexCount= newIndexCount;
		
		D3DCALL(d3dDevice->CreateIndexBuffer( 
			bufferIndexCount*sizeof(uint32),
			0, 
			D3DFMT_INDEX32, 
			D3DPOOL_MANAGED, 
			&d3dIndexBuffer, 
			NULL));
	}
}

}}}//end namespace