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
/**
 * File:   g3dv4.h
 *
 * Description:
 *   Data types used by the G3D format.
 */

#ifndef G3DV4_HEADER
#define G3DV4_HEADER
// bug fix from wiki
#pragma pack(push, 1)

typedef float float32;
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;

#define NAMESIZE 64

struct FileHeader{
	uint8 id[3];
	uint8 version;
};

struct ModelHeader{
	uint16 meshCount;
	uint8 type;
};
enum ModelType{
	mtMorphMesh	
};

struct MeshHeader{
	uint8 name[NAMESIZE];
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
enum MeshPropertyFlag{
	mpfTwoSided= 0,
	mpfCustomColor= 1
};

#pragma pack(pop)
#endif
