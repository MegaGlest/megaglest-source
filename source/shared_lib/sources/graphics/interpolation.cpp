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

#include "interpolation.h"

#include <cassert>
#include <algorithm>

#include "model.h"
#include "conversion.h"
#include "util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

InterpolationData::InterpolationData(const Mesh *mesh){
	vertices= NULL;
	normals= NULL;
	
	this->mesh= mesh;

	if(mesh->getFrameCount()>1) {
		vertices= new Vec3f[mesh->getVertexCount()];
		normals= new Vec3f[mesh->getVertexCount()];
	}

	cacheVertices.clear();
	cacheNormals.clear();
}

InterpolationData::~InterpolationData(){
	delete [] vertices;
	delete [] normals;

	for(std::map<std::string, Vec3f *>::iterator iterVert = cacheVertices.begin();
		iterVert != cacheVertices.end(); iterVert++) {
		delete [] iterVert->second;
	}
	for(std::map<std::string, Vec3f *>::iterator iterVert = cacheNormals.begin();
		iterVert != cacheNormals.end(); iterVert++) {
		delete [] iterVert->second;
	}
}

void InterpolationData::update(float t, bool cycle){
	updateVertices(t, cycle);
	updateNormals(t, cycle);
}

void InterpolationData::updateVertices(float t, bool cycle) {
	assert(t>=0.0f && t<=1.0f);

	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();

	if(frameCount > 1) {
		std::string lookupKey = floatToStr(t) + "_" + boolToStr(cycle);
		std::map<std::string, Vec3f *>::iterator iterFind = cacheVertices.find(lookupKey);

		if(iterFind != cacheVertices.end()) {
			for(uint32 j=0; j< vertexCount; ++j){
				vertices[j] = iterFind->second[j];
			}
			return;
		}
		else {
			cacheVertices[lookupKey] = new Vec3f[vertexCount];
			iterFind = cacheVertices.find(lookupKey);
		}
	
		const Vec3f *meshVertices= mesh->getVertices();

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
			iterFind->second[j] = vertices[j];
		}
	}
}

void InterpolationData::updateNormals(float t, bool cycle){
	assert(t>=0.0f && t<=1.0f);

	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();

	if(frameCount > 1) {
		std::string lookupKey = floatToStr(t) + "_" + boolToStr(cycle);
		std::map<std::string, Vec3f *>::iterator iterFind = cacheNormals.find(lookupKey);
		if(iterFind != cacheNormals.end()) {
			for(uint32 j=0; j<vertexCount; ++j) {
				normals[j] = iterFind->second[j];
			}
			return;
		}
		else {
			cacheNormals[lookupKey] = new Vec3f[mesh->getVertexCount()];
			iterFind = cacheNormals.find(lookupKey);
		}

		const Vec3f *meshNormals= mesh->getNormals();

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
			iterFind->second[j] = normals[j];
		}
	}
}

}}//end namespace 
