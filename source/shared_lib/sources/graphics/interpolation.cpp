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

#include "interpolation.h"

#include <cassert>
#include <algorithm>

#include "model.h"
#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

InterpolationData::InterpolationData(const Mesh *mesh){
	vertices= NULL;
	normals= NULL;
	
	this->mesh= mesh;

	if(mesh->getFrameCount()>1){
		vertices= new Vec3f[mesh->getVertexCount()];
		normals= new Vec3f[mesh->getVertexCount()];
	}
}

InterpolationData::~InterpolationData(){
	delete [] vertices;
	delete [] normals;
}

void InterpolationData::update(float t, bool cycle){
	updateVertices(t, cycle);
	updateNormals(t, cycle);
}

void InterpolationData::updateVertices(float t, bool cycle){
	assert(t>=0.0f && t<=1.0f);
	
	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();
	const Vec3f *meshVertices= mesh->getVertices();

	if(frameCount>1){
		//misc vars
		uint32 prevFrame= min<uint32>(static_cast<uint32>(t*frameCount), frameCount-1);
		uint32 nextFrame= cycle? (prevFrame+1) % frameCount: min(prevFrame+1, frameCount-1); 
		float localT= t*frameCount - prevFrame; 
		uint32 prevFrameBase= prevFrame*vertexCount;
		uint32 nextFrameBase= nextFrame*vertexCount;

		//assertions
		assert(prevFrame<frameCount);
		assert(nextFrame<frameCount);

		//interpolate vertices
		for(uint32 j=0; j<vertexCount; ++j){
			vertices[j]= meshVertices[prevFrameBase+j].lerp(localT, meshVertices[nextFrameBase+j]);
		}
	}
}

void InterpolationData::updateNormals(float t, bool cycle){
	assert(t>=0.0f && t<=1.0f);
	
	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();
	const Vec3f *meshNormals= mesh->getNormals();

	if(frameCount>1){
		//misc vars
		uint32 prevFrame= min<uint32>(static_cast<uint32>(t*frameCount), frameCount-1);
		uint32 nextFrame= cycle? (prevFrame+1) % frameCount: min(prevFrame+1, frameCount-1); 
		float localT= t*frameCount - prevFrame; 
		uint32 prevFrameBase= prevFrame*vertexCount;
		uint32 nextFrameBase= nextFrame*vertexCount;

		//assertions
		assert(prevFrame<frameCount);
		assert(nextFrame<frameCount);

		//interpolate vertices
		for(uint32 j=0; j<vertexCount; ++j){
			normals[j]= meshNormals[prevFrameBase+j].lerp(localT, meshNormals[nextFrameBase+j]); 
		}
	}
}

}}//end namespace 
