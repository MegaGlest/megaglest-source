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

#include "particle_renderer_gl.h"

#include "opengl.h"
#include "texture_gl.h"
#include "model_renderer.h"
#include "math_util.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class ParticleRendererGl
// =====================================================

// ===================== PUBLIC ========================

ParticleRendererGl::ParticleRendererGl(){
	assert(bufferSize%4 == 0);
	
	rendering= false;

	// init texture coordinates for quads
	for(int i= 0; i<bufferSize; i+=4){
		texCoordBuffer[i]= Vec2f(0.0f, 1.0f);
		texCoordBuffer[i+1]= Vec2f(0.0f, 0.0f);
		texCoordBuffer[i+2]= Vec2f(1.0f, 0.0f);
		texCoordBuffer[i+3]= Vec2f(1.0f, 1.0f); 
	}
}

void ParticleRendererGl::renderManager(ParticleManager *pm, ModelRenderer *mr){
	
	//assertions
	assertGl();

	//push state
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT  | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	//init state
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);

	//render
	rendering= true;
	pm->render(this, mr);
	rendering= false;

	//pop state
	glPopClientAttrib();
	glPopAttrib();

	//assertions
	assertGl();
}

void ParticleRendererGl::renderSystem(ParticleSystem *ps){
	assertGl();
	assert(rendering);

	Vec3f rightVector;
	Vec3f upVector;
	float modelview[16];

	//render particles
	setBlendMode(ps->getBlendMode());
	
	// get the current modelview state
	glGetFloatv(GL_MODELVIEW_MATRIX , modelview);
	rightVector= Vec3f(modelview[0], modelview[4], modelview[8]);
	upVector= Vec3f(modelview[1], modelview[5], modelview[9]);

	// set state
	if(ps->getTexture()!=NULL){
		glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(ps->getTexture())->getHandle());
	}
	else{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glDisable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	//fill vertex buffer with billboards
	int bufferIndex= 0;

	for(int i=0; i<ps->getAliveParticleCount(); ++i){
		const Particle *particle= ps->getParticle(i);
		float size= particle->getSize()/2.0f;
		Vec3f pos= particle->getPos();
		Vec4f color= particle->getColor();

		vertexBuffer[bufferIndex] = pos - (rightVector - upVector) * size;
		vertexBuffer[bufferIndex+1] = pos - (rightVector + upVector) * size;
		vertexBuffer[bufferIndex+2] = pos + (rightVector - upVector) * size;
		vertexBuffer[bufferIndex+3] = pos + (rightVector + upVector) * size;
		
		colorBuffer[bufferIndex]= color;
		colorBuffer[bufferIndex+1]= color;
		colorBuffer[bufferIndex+2]= color;
		colorBuffer[bufferIndex+3]= color;

		bufferIndex+= 4;

		if(bufferIndex >= bufferSize){
			bufferIndex= 0;
			renderBufferQuads(bufferSize);
		}
	}
	renderBufferQuads(bufferIndex);

	assertGl();
}

void ParticleRendererGl::renderSystemLine(ParticleSystem *ps){
	
	assertGl();
	assert(rendering);

	if(!ps->isEmpty()){
		const Particle *particle= ps->getParticle(0);
		
		setBlendMode(ps->getBlendMode());

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//fill vertex buffer with lines
		int bufferIndex= 0;

		glLineWidth(particle->getSize());
		
		for(int i=0; i<ps->getAliveParticleCount(); ++i){
			particle= ps->getParticle(i);
			Vec4f color= particle->getColor();

			vertexBuffer[bufferIndex] = particle->getPos();
			vertexBuffer[bufferIndex+1] = particle->getLastPos();
			
			colorBuffer[bufferIndex]= color;
			colorBuffer[bufferIndex+1]= color;

			bufferIndex+= 2;

			if(bufferIndex >= bufferSize){
				bufferIndex= 0;
				renderBufferLines(bufferSize);
			}
		}
		renderBufferLines(bufferIndex);
	}

	assertGl();
}

void ParticleRendererGl::renderSystemLineAlpha(ParticleSystem *ps){
	
	assertGl();
	assert(rendering);

	if(!ps->isEmpty()){
		const Particle *particle= ps->getParticle(0);
			
		setBlendMode(ps->getBlendMode());

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//fill vertex buffer with lines
		int bufferIndex= 0;

		glLineWidth(particle->getSize());
		
		for(int i=0; i<ps->getAliveParticleCount(); ++i){
			particle= ps->getParticle(i);
			Vec4f color= particle->getColor();

			vertexBuffer[bufferIndex] = particle->getPos();
			vertexBuffer[bufferIndex+1] = particle->getLastPos();
			
			colorBuffer[bufferIndex]= color;
			colorBuffer[bufferIndex+1]= color;
			colorBuffer[bufferIndex+1].w= 0.0f;

			bufferIndex+= 2;

			if(bufferIndex >= bufferSize){
				bufferIndex= 0;
				renderBufferLines(bufferSize);
			}
		}
		renderBufferLines(bufferIndex);
	}

	assertGl();
}

void ParticleRendererGl::renderSingleModel(AttackParticleSystem *ps, ModelRenderer *mr){
	//render model
	if(ps->getModel()!=NULL){
		
		//init
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.f, 1.f, 1.f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		
		//translate
		Vec3f pos= ps->getPos();
		glTranslatef(pos.x, pos.y, pos.z);

		//rotate
		Vec3f direction= ps->getDirection();
		Vec3f flatDirection= Vec3f(direction.x, 0.f, direction.z);
		Vec3f rotVector= Vec3f(0.f, 1.f, 0.f).cross(flatDirection);

		float angleV= radToDeg(atan2(flatDirection.length(), direction.y)) - 90.f;
		glRotatef(angleV, rotVector.x, rotVector.y, rotVector.z);
		float angleH= radToDeg(atan2(direction.x, direction.z));
		glRotatef(angleH, 0.f, 1.f, 0.f);
		
		//render
		mr->begin(true, true, false);
		mr->render(ps->getModel());
		mr->end();

		//end
		glPopMatrix();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
	}
}


// ============== PRIVATE =====================================

void ParticleRendererGl::renderBufferQuads(int quadCount){
	glVertexPointer(3, GL_FLOAT, 0, vertexBuffer);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoordBuffer);
	glColorPointer(4, GL_FLOAT, 0, colorBuffer);

	glDrawArrays(GL_QUADS, 0, quadCount);
}

void ParticleRendererGl::renderBufferLines(int lineCount){
	glVertexPointer(3, GL_FLOAT, 0, vertexBuffer);
	glColorPointer(4, GL_FLOAT, 0, colorBuffer);

	glDrawArrays(GL_LINES, 0, lineCount);
}

void ParticleRendererGl::setBlendMode(ParticleSystem::BlendMode blendMode){
	switch(blendMode){
	case ParticleSystem::bmOne:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case ParticleSystem::bmOneMinusAlpha:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	default:
		assert(false);
	}
}

}}} //end namespace
