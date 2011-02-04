// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
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

namespace Shared { namespace Graphics { namespace Gl {

// =====================================================
//	class MyClass
// =====================================================

// ===================== PUBLIC ========================

ModelRendererGl::ModelRendererGl() {
	rendering= false;
	duplicateTexCoords= false;
	secondaryTexCoordUnit= 1;
}

void ModelRendererGl::begin(bool renderNormals, bool renderTextures, bool renderColors, MeshCallback *meshCallback) {
	//assertions
	assert(rendering == false);
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

/*
	glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );
	glHint( GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_FASTEST );
	glHint( GL_GENERATE_MIPMAP_HINT, GL_FASTEST );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
	glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_FASTEST );
	glHint( GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST );
*/
	//assertions
	assertGl();
}

void ModelRendererGl::end() {
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

void ModelRendererGl::render(Model *model) {
	//assertions
	assert(rendering);
	assertGl();

	//if(model->getIsStaticModel()) printf("In [%s::%s Line: %d] filename [%s] is static about to render...\n",__FILE__,__FUNCTION__,__LINE__,model->getFileName().c_str());

	//render every mesh
	//if(model->getIsStaticModel() == true) {
	for(uint32 i = 0;  i < model->getMeshCount(); ++i) {
		renderMesh(model->getMeshPtr(i));
	}
	//}
	//assertions
	assertGl();
}

void ModelRendererGl::renderNormalsOnly(Model *model) {
	//assertions
	assert(rendering);
	assertGl();

	//render every mesh
	//if(model->getIsStaticModel() == true) {
	for(uint32 i=0; i<model->getMeshCount(); ++i) {
		renderMeshNormals(model->getMeshPtr(i));
	}
	//}

	//assertions
	assertGl();
}

// ===================== PRIVATE =======================

void ModelRendererGl::renderMesh(Mesh *mesh) {

	//assertions
	assertGl();

	//set cull face
	if(mesh->getTwoSided()) {
		glDisable(GL_CULL_FACE);
	}
	else{
		glEnable(GL_CULL_FACE);
	}

	//set color
	if(renderColors) {
		Vec4f color(mesh->getDiffuseColor(), mesh->getOpacity());
		glColor4fv(color.ptr());
	}

	//texture state
	const Texture2DGl *texture= static_cast<const Texture2DGl*>(mesh->getTexture(mtDiffuse));
	if(texture != NULL && renderTextures) {
		if(lastTexture != texture->getHandle()){
			//assert(glIsTexture(texture->getHandle()));
			//throw runtime_error("glIsTexture(texture->getHandle()) == false for texture: " + texture->getPath());
			if(glIsTexture(texture->getHandle()) == true) {
                glBindTexture(GL_TEXTURE_2D, texture->getHandle());
                lastTexture= texture->getHandle();
			}
			else {
                glBindTexture(GL_TEXTURE_2D, 0);
                lastTexture= 0;
			}
		}
	}
	else{
		glBindTexture(GL_TEXTURE_2D, 0);
		lastTexture= 0;
	}

	if(meshCallback != NULL) {
		meshCallback->execute(mesh);
	}

	//misc vars
	uint32 vertexCount= mesh->getVertexCount();
	uint32 indexCount= mesh->getIndexCount();

	//assertions
	assertGl();

	if(getVBOSupported() == true && mesh->getFrameCount() == 1) {
		if(mesh->hasBuiltVBOEntities() == false) {
			mesh->BuildVBOs();
		}
		//printf("Rendering Mesh with VBO's\n");

		//vertices
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBOVertices() );
		glVertexPointer( 3, GL_FLOAT, 0, (char *) NULL );		// Set The Vertex Pointer To The Vertex Buffer
		//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

		//normals
		if(renderNormals) {
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBONormals() );
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, (char *) NULL);
			//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		}
		else{
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		//tex coords
		if(renderTextures && mesh->getTexture(mtDiffuse) != NULL ) {
			if(duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);

				glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBOTexCoords() );
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL );		// Set The TexCoord Pointer To The TexCoord Buffer
				//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
			}

			glActiveTexture(GL_TEXTURE0);

			glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBOTexCoords() );
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL );		// Set The TexCoord Pointer To The TexCoord Buffer
			//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		}
		else {
			if(duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
	else {
		//printf("Rendering Mesh WITHOUT VBO's\n");

		//vertices
		glVertexPointer(3, GL_FLOAT, 0, mesh->getInterpolationData()->getVertices());

		//normals
		if(renderNormals) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, mesh->getInterpolationData()->getNormals());
		}
		else{
			glDisableClientState(GL_NORMAL_ARRAY);
		}

		//tex coords
		if(renderTextures && mesh->getTexture(mtDiffuse)!=NULL ) {
			if(duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
			}

			glActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, mesh->getTexCoords());
		}
		else {
			if(duplicateTexCoords) {
				glActiveTexture(GL_TEXTURE0 + secondaryTexCoordUnit);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	if(getVBOSupported() == true && mesh->getFrameCount() == 1) {
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, mesh->getVBOIndexes() );
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, (char *)NULL);
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

		//glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, mesh->getIndices());
	}
	else {
		//draw model
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, mesh->getIndices());
	}

	//assertions
	assertGl();
}

void ModelRendererGl::renderMeshNormals(Mesh *mesh) {
	if(getVBOSupported() == true && mesh->getFrameCount() == 1) {
		if(mesh->hasBuiltVBOEntities() == false) {
			mesh->BuildVBOs();
		}

		//printf("Rendering Mesh Normals with VBO's\n");

		//vertices
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBOVertices() );
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer( 3, GL_FLOAT, 0, (char *) NULL );		// Set The Vertex Pointer To The Vertex Buffer
		//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

		//normals
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, mesh->getVBONormals() );
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, (char *) NULL);
		//glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

		//misc vars
		uint32 vertexCount= mesh->getVertexCount();
		uint32 indexCount= mesh->getIndexCount();

		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, mesh->getVBOIndexes() );
		glDrawRangeElements(GL_TRIANGLES, 0, vertexCount-1, indexCount, GL_UNSIGNED_INT, (char *)NULL);
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	}
	else {
		//printf("Rendering Mesh Normals WITHOUT VBO's\n");

		glBegin(GL_LINES);
		for(unsigned int i = 0; i < mesh->getIndexCount(); ++i) {
			const Vec3f &vertex= mesh->getInterpolationData()->getVertices()[mesh->getIndices()[i]];
			const Vec3f &normal= vertex + mesh->getInterpolationData()->getNormals()[mesh->getIndices()[i]];

			glVertex3fv(vertex.ptr());
			glVertex3fv(normal.ptr());
		}
		glEnd();
	}
}

}}}//end namespace
