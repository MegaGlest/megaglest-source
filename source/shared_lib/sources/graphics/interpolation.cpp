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
#include "platform_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class InterpolationData
// =====================================================

bool InterpolationData::enableInterpolation = true;

InterpolationData::InterpolationData(const Mesh *mesh) {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		throw megaglest_runtime_error("Loading graphics in headless server mode not allowed!");
	}

	vertices= NULL;
	normals= NULL;
	
	raw_frame_ofs = 0;
	
	this->mesh= mesh;
}

InterpolationData::~InterpolationData(){
	delete [] vertices;
	vertices=NULL;
	delete [] normals;
	normals=NULL;
}

void InterpolationData::update(float t, bool cycle){
	updateVertices(t, cycle);
	updateNormals(t, cycle);
}

void InterpolationData::updateVertices(float t, bool cycle) {
	update(mesh->getVertices(), vertices, t, cycle);
}

void InterpolationData::updateNormals(float t, bool cycle) {
	update(mesh->getNormals(), normals, t, cycle);
}

void InterpolationData::update(const Vec3f* src, Vec3f* &dest, float t, bool cycle) {

	if(t <0.0f || t>1.0f) {
		printf("ERROR t = [%f] for cycle [%d] f [%d] v [%d]\n",t,cycle,mesh->getFrameCount(),mesh->getVertexCount());
	}
	//assert(t>=0.0f && t<=1.0f);
	if(t < 0.0f || t > 1.0f) {
		throw megaglest_runtime_error("t < 0.0f || t > 1.0f t = [" + floatToStr(t,16) + "]");

		assert(t >= 0.f && t <= 1.f);
	}

	uint32 frameCount= mesh->getFrameCount();
	uint32 vertexCount= mesh->getVertexCount();

	if(frameCount > 1) {
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
		
		if(enableInterpolation) {
			if(!dest) { // not previously allocated
			      dest = new Vec3f[vertexCount];
			}
			for(uint32 j=0; j<vertexCount; ++j){
				dest[j]= src[prevFrameBase+j].lerp(localT, src[nextFrameBase+j]);
			}
		} else {
			raw_frame_ofs = prevFrameBase;
		}
	}
}

}}//end namespace 
