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

#ifndef _SHARED_GRAPHICTYPES_MODELHEADER_H_
#define _SHARED_GRAPHICTYPES_MODELHEADER_H_

#include "types.h"

using Shared::Platform::uint8;
using Shared::Platform::uint16;
using Shared::Platform::uint32;
using Shared::Platform::float32;

namespace Shared{ namespace Graphics{

#pragma pack(push, 1) 

struct FileHeader{
	uint8 id[3];
	uint8 version;
};

//version 4

struct ModelHeader{
	uint16 meshCount;
	uint8 type;
};

enum ModelType{
	mtMorphMesh	
};

enum MeshPropertyFlag{
	mpfCustomColor= 1,
	mpfTwoSided= 2
};

enum MeshTexture{
	mtDiffuse,
	mtSpecular,
	mtNormal,
	mtReflection,
	mtColorMask,

	meshTextureCount
};

const int meshTextureChannelCount[]= {-1, 1, 3, 1, 1};

const uint32 meshNameSize= 64;
const uint32 mapPathSize= 64;

struct MeshHeader{
	uint8 name[meshNameSize];
	uint32 frameCount;
	uint32 vertexCount;
	uint32 indexCount;
	float32 diffuseColor[3];
	float32 specularColor[3];
	float32 specularPower;
	float32 opacity;
	uint32 properties;
	uint32 textures;
};

#pragma pack(pop) 

//version 3

//front faces are clockwise faces
struct ModelHeaderV3{
	uint32 meshCount;
};

enum MeshPropertyV3{
	mp3NoTexture= 1,
	mp3TwoSided= 2,
	mp3CustomColor= 4
};

struct MeshHeaderV3{
	uint32 vertexFrameCount;
	uint32 normalFrameCount;
	uint32 texCoordFrameCount;
	uint32 colorFrameCount;
	uint32 pointCount;
	uint32 indexCount;
	uint32 properties;
	uint8 texName[64];
};

//version 2

struct MeshHeaderV2{
	uint32 vertexFrameCount;
	uint32 normalFrameCount;
	uint32 texCoordFrameCount;
	uint32 colorFrameCount;
	uint32 pointCount;
	uint32 indexCount;
	uint8 hasTexture;
	uint8 primitive;
	uint8 cullFace;
	uint8 texName[64];
};

}}//end namespace

#endif
