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

#include "model_renderer_gl.h"

#include "opengl.h"
#include "gl_wrap.h"
#include "texture_gl.h"
#include "interpolation.h"
#include "leak_dumper.h"

using namespace Shared::Platform;

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class MyClass
// =====================================================

// ===================== PUBLIC ========================

ModelRendererGl::ModelRendererGl(){
	rendering= false;
	duplicateTexCoords= false;
	secondaryTexCoordUnit= 1;
}

void ModelRendererGl::begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback){
	//assertions
	assert(!rendering);
	assertGl();

	this->renderTextures= renderTextures;
	this->renderNormals= renderNormals;
	this->renderColors= renderColors;
	this->meshCallback= meshCallback;

	rendering= true;
	lastTexture= 0;
	glBindTexture(GL_TEXTURE_2D, 0);

	//push attribs
	glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	
	//init opengl
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFrontFace(GL_CCW);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glEnableClientState(GL_VERTEX_ARRAY);

	if(renderNormals){
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	if(renderTextures){
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	//assertions
	assertGl();
}

void ModelRendererGl::end(){
	//assertions
	assert(rendering);
	assertGl();

	//set render state
	rendering= false;

	//pop
	glPopAttrib();
	glPopClientAttrib();

	//assertions
	assertGl();
}

void ModelRendererGl::render(const Model *model){
	//assertions
	assert(rendering);
	assertGl();

	//render every mesh
	for(uint32 i=0; i<model->getMeshCount(); ++i){
		renderMesh(model->getMesh(i));
	}
	
	//assertions
	assertGl();
}

void ModelRendererGl::renderNormalsOnly(const Model *model){
	//assertions
	assert(rendering);
	assertGl();

	//render every mesh
	for(uint32 i=0; i<model->getMeshCount(); ++i){
		renderMeshNormals(model->getMesh(i));	
	}
	
	//assertions
	assertGl();
}

// ===================== PRIVATE =======================

void ModelRendererGl::renderMesh(const Mesh *mesh){
		
	//assertions
	assertGl();

	//set cull face
	if(mesh->getTwoSided()){
		glDisable(GL_CULL_FACE);
	}
	else{
		glEnable(GL_CULL_FACE);
	}

	//set color
	if(renderColors){
		Vec4f color(mesh->getDiffuseColor(), mesh->getOpacity());
		glColor4fv(color.ptr());
	}

	//texture state
	const Texture2DGl *texture= static_cast<const Texture2DGl*>(mesh->getTexture(mtDiffuse));
	if(texture != NULL && renderTextures){
		if(lastTexture != texture->getHandle()){
			assert(glIsTexture(texture->getHandle()));
			glBindTexture(GL_TEXTURE_2D, texture->getHandle());
			lastTexture= texture->getHandle();
		}
	}
	else{
		glBindTexture(GL_TEXTURE_2D, 0);
		lastTexture= 0;
	}

	if(meshCallback!=NULL){
		meshCallback->execute(mesh);
	}

	//misc vars
	uint32 vertexCount= mesh->getVertexCount();
	uint32 indexCount= mesh->getIndexCount();
	
	//assertions
	assertGl();
	
	//vertices
	glVertexPointer(3, GL_FLOAT, 0, mesh->getInterpolationData()->getVertices());

	//normals
	if(renderNormals){
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, mesh->getInterpolationData()->getNormals());
	}
	else{
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	//tex coords
	if(renderTextures && mesh->getTexture(mtDiffuse)!=NULL ){
		if(duplicateTexCoords){
			glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
		}

		glActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
	}
	else{
		if(duplicateTexCoords){
			glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	//draw model
	glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, mesh->getIndices());

	//assertions
	assertGl();
}

void ModelRendererGl::renderMeshNormals(const Mesh *mesh){
	glBegin(GL_LINES);
	for(int i= 0; i<mesh->getIndexCount(); ++i){
		Vec3f vertex= mesh->getInterpolationData()->getVertices()[mesh->getIndices()[i]];
		Vec3f normal= vertex + mesh->getInterpolationData()->getNormals()[mesh->getIndices()[i]];
	
		glVertex3fv(vertex.ptr());
		glVertex3fv(normal.ptr());
	}
	glEnd();
}

}}}//end namespace
