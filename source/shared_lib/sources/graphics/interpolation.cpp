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
#include "conversion.h"
#include "util.h"
#include <stdexcept>
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

InterpolationData::InterpolationData(const Mesh *mesh) {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	vertices= NULL;
	normals= NULL;
	
	this->mesh= mesh;

	if(mesh->getFrameCount()>1) {
		vertices= new Vec3f[mesh->getVertexCount()];
		normals= new Vec3f[mesh->getVertexCount()];
	}

	enableCache = true;

	cacheVertices.clear();
	cacheNormals.clear();
}

InterpolationData::~InterpolationData(){
	delete [] vertices;
	delete [] normals;


	for(std::map<float, std::map<bool, Vec3f *> >::iterator iterVert = cacheVertices.begin();
		iterVert != cacheVertices.end(); ++iterVert) {
		for(std::map<bool, Vec3f *>::iterator iterVert2 = iterVert->second.begin();
			iterVert2 != iterVert->second.end(); ++iterVert2) {
			delete [] iterVert2->second;
		}
	}

	for(std::map<float, std::map<bool, Vec3f *> >::iterator iterVert = cacheNormals.begin();
		iterVert != cacheNormals.end(); ++iterVert) {
		for(std::map<bool, Vec3f *>::iterator iterVert2 = iterVert->second.begin();
			iterVert2 != iterVert->second.end(); ++iterVert2) {
			delete [] iterVert2->second;
		}
	}
}

void InterpolationData::update(float t, bool cycle){
	updateVertices(t, cycle);
	updateNormals(t, cycle);
}

void InterpolationData::updateVertices(float t, bool cycle) {
	if(t <0.0f || t>1.0f) {
		printf("ERROR t = [%f] for cycle [%d] f [%d] v [%d]\n",t,cycle,mesh->getFrameCount(),mesh->getVertexCount());
	}
	//assert(t>=0.0f && t<=1.0f);
	if(t < 0.0f || t > 1.0f) {
		throw runtime_error("t < 0.0f || t > 1.0f t = [" + floatToStr(t) + "]");

		assert(t >= 0.f && t <= 1.f);
	}

	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();

	if(frameCount > 1) {
		if(enableCache == true) {
			std::map<float, std::map<bool, Vec3f *> >::iterator iterFind = cacheVertices.find(t);
			if(iterFind != cacheVertices.end()) {
				std::map<bool, Vec3f *>::iterator iterFind2 = iterFind->second.find(cycle);
				if(iterFind2 != iterFind->second.end()) {
					memcpy(vertices,iterFind2->second,sizeof(Vec3f) * vertexCount);
					return;
				}
			}
			cacheVertices[t][cycle] = new Vec3f[vertexCount];
		}

		const Vec3f *meshVertices= mesh->getVertices();

		//misc vars
		uint32 prevFrame;
		uint32 nextFrame;
		float localT;

		if(cycle == true) {
			prevFrame= min<uint32>(static_cast<uint32>(t*frameCount), frameCount-1);
			nextFrame= (prevFrame+1) % frameCount;
			localT= t*frameCount - prevFrame;
		}
		else {
			prevFrame= min<uint32> (static_cast<uint32> (t * (frameCount-1)), frameCount - 2);
			nextFrame= min(prevFrame + 1, frameCount - 1);
			localT= t * (frameCount-1) - prevFrame;
			//printf(" prevFrame=%d nextFrame=%d localT=%f\n",prevFrame,nextFrame,localT);
		}

		uint32 prevFrameBase= prevFrame*vertexCount;
		uint32 nextFrameBase= nextFrame*vertexCount;

		//assertions
		assert(prevFrame<frameCount);
		assert(nextFrame<frameCount);

		//interpolate vertices
		for(uint32 j=0; j<vertexCount; ++j){
			vertices[j]= meshVertices[prevFrameBase+j].lerp(localT, meshVertices[nextFrameBase+j]);

			if(enableCache == true) {
				cacheVertices[t][cycle][j] = vertices[j];
			}
		}
	}
}

void InterpolationData::updateNormals(float t, bool cycle){
	if(t < 0.0f || t > 1.0f) {
		throw runtime_error("t < 0.0f || t > 1.0f t = [" + floatToStr(t) + "]");

		assert(t>=0.0f && t<=1.0f);
	}

	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();

	if(frameCount > 1) {
		if(enableCache == true) {
			std::map<float, std::map<bool, Vec3f *> >::iterator iterFind = cacheNormals.find(t);
			if(iterFind != cacheNormals.end()) {
				std::map<bool, Vec3f *>::iterator iterFind2 = iterFind->second.find(cycle);
				if(iterFind2 != iterFind->second.end()) {
					memcpy(normals,iterFind2->second,sizeof(Vec3f) * vertexCount);
					return;
				}
			}
			cacheNormals[t][cycle] = new Vec3f[vertexCount];
		}

		const Vec3f *meshNormals= mesh->getNormals();

		//misc vars
		uint32 prevFrame;
		uint32 nextFrame;
		float localT;

		if(cycle == true) {
			prevFrame= min<uint32>(static_cast<uint32>(t*frameCount), frameCount-1);
			nextFrame= (prevFrame+1) % frameCount;
			localT= t*frameCount - prevFrame;
		}
		else {
			prevFrame= min<uint32> (static_cast<uint32> (t * (frameCount-1)), frameCount - 2);
			nextFrame= min(prevFrame + 1, frameCount - 1);
			localT= t * (frameCount-1) - prevFrame;
			//printf(" prevFrame=%d nextFrame=%d localT=%f\n",prevFrame,nextFrame,localT);
		}

		uint32 prevFrameBase= prevFrame*vertexCount;
		uint32 nextFrameBase= nextFrame*vertexCount;

		//assertions
		assert(prevFrame<frameCount);
		assert(nextFrame<frameCount);

		//interpolate vertices
		for(uint32 j=0; j<vertexCount; ++j){
			normals[j]= meshNormals[prevFrameBase+j].lerp(localT, meshNormals[nextFrameBase+j]);

			if(enableCache == true) {
				cacheNormals[t][cycle][j] = normals[j];
			}
		}
	}
}

}}//end namespace 
