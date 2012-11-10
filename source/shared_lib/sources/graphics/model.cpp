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

#include "model.h"

#include <cstdio>
#include <cassert>
#include <stdexcept>

#include "interpolation.h"
#include "conversion.h"
#include "util.h"
#include "platform_common.h"
#include "opengl.h"
#include "platform_util.h"
#include <memory>
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics::Gl;

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

using namespace Util;

// Utils methods for endianness conversion
void toEndianFileHeader(FileHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < 3; ++i) {
			header.id[i] = Shared::PlatformByteOrder::toCommonEndian(header.id[i]);
		}
		header.version = Shared::PlatformByteOrder::toCommonEndian(header.version);
	}
}
void fromEndianFileHeader(FileHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < 3; ++i) {
			header.id[i] = Shared::PlatformByteOrder::fromCommonEndian(header.id[i]);
		}
		header.version = Shared::PlatformByteOrder::fromCommonEndian(header.version);
	}
}

void toEndianModelHeader(ModelHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.type = Shared::PlatformByteOrder::toCommonEndian(header.type);
		header.meshCount = Shared::PlatformByteOrder::toCommonEndian(header.meshCount);
	}
}
void fromEndianModelHeader(ModelHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.type = Shared::PlatformByteOrder::toCommonEndian(header.type);
		header.meshCount = Shared::PlatformByteOrder::toCommonEndian(header.meshCount);
	}
}

void toEndianMeshHeader(MeshHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < meshNameSize; ++i) {
			header.name[i] = Shared::PlatformByteOrder::toCommonEndian(header.name[i]);
		}
		header.frameCount = Shared::PlatformByteOrder::toCommonEndian(header.frameCount);
		header.vertexCount = Shared::PlatformByteOrder::toCommonEndian(header.vertexCount);
		header.indexCount = Shared::PlatformByteOrder::toCommonEndian(header.indexCount);
		for(unsigned int i = 0; i < 3; ++i) {
			header.diffuseColor[i] = Shared::PlatformByteOrder::toCommonEndian(header.diffuseColor[i]);
			header.specularColor[i] = Shared::PlatformByteOrder::toCommonEndian(header.specularColor[i]);
		}
		header.specularPower = Shared::PlatformByteOrder::toCommonEndian(header.specularPower);
		header.opacity = Shared::PlatformByteOrder::toCommonEndian(header.opacity);
		header.properties = Shared::PlatformByteOrder::toCommonEndian(header.properties);
		header.textures = Shared::PlatformByteOrder::toCommonEndian(header.textures);
	}
}

void fromEndianMeshHeader(MeshHeader &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < meshNameSize; ++i) {
			header.name[i] = Shared::PlatformByteOrder::fromCommonEndian(header.name[i]);
		}
		header.frameCount = Shared::PlatformByteOrder::fromCommonEndian(header.frameCount);
		header.vertexCount = Shared::PlatformByteOrder::fromCommonEndian(header.vertexCount);
		header.indexCount = Shared::PlatformByteOrder::fromCommonEndian(header.indexCount);
		for(unsigned int i = 0; i < 3; ++i) {
			header.diffuseColor[i] = Shared::PlatformByteOrder::fromCommonEndian(header.diffuseColor[i]);
			header.specularColor[i] = Shared::PlatformByteOrder::fromCommonEndian(header.specularColor[i]);
		}
		header.specularPower = Shared::PlatformByteOrder::fromCommonEndian(header.specularPower);
		header.opacity = Shared::PlatformByteOrder::fromCommonEndian(header.opacity);
		header.properties = Shared::PlatformByteOrder::fromCommonEndian(header.properties);
		header.textures = Shared::PlatformByteOrder::fromCommonEndian(header.textures);
	}
}

void toEndianModelHeaderV3(ModelHeaderV3 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.meshCount = Shared::PlatformByteOrder::toCommonEndian(header.meshCount);
	}
}
void fromEndianModelHeaderV3(ModelHeaderV3 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.meshCount = Shared::PlatformByteOrder::fromCommonEndian(header.meshCount);
	}
}

void toEndianMeshHeaderV3(MeshHeaderV3 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.vertexFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.vertexFrameCount);
		header.normalFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.normalFrameCount);
		header.texCoordFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.texCoordFrameCount);
		header.colorFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.colorFrameCount);
		header.pointCount = Shared::PlatformByteOrder::toCommonEndian(header.pointCount);
		header.indexCount = Shared::PlatformByteOrder::toCommonEndian(header.indexCount);
		header.properties = Shared::PlatformByteOrder::toCommonEndian(header.properties);
		for(unsigned int i = 0; i < 64; ++i) {
			header.texName[i] = Shared::PlatformByteOrder::toCommonEndian(header.texName[i]);
		}
	}
}

void fromEndianMeshHeaderV3(MeshHeaderV3 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.vertexFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.vertexFrameCount);
		header.normalFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.normalFrameCount);
		header.texCoordFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.texCoordFrameCount);
		header.colorFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.colorFrameCount);
		header.pointCount = Shared::PlatformByteOrder::fromCommonEndian(header.pointCount);
		header.indexCount = Shared::PlatformByteOrder::fromCommonEndian(header.indexCount);
		header.properties = Shared::PlatformByteOrder::fromCommonEndian(header.properties);
		for(unsigned int i = 0; i < 64; ++i) {
			header.texName[i] = Shared::PlatformByteOrder::fromCommonEndian(header.texName[i]);
		}
	}
}

void toEndianMeshHeaderV2(MeshHeaderV2 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.vertexFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.vertexFrameCount);
		header.normalFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.normalFrameCount);
		header.texCoordFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.texCoordFrameCount);
		header.colorFrameCount = Shared::PlatformByteOrder::toCommonEndian(header.colorFrameCount);
		header.pointCount = Shared::PlatformByteOrder::toCommonEndian(header.pointCount);
		header.indexCount = Shared::PlatformByteOrder::toCommonEndian(header.indexCount);
		header.hasTexture = Shared::PlatformByteOrder::toCommonEndian(header.hasTexture);
		header.primitive = Shared::PlatformByteOrder::toCommonEndian(header.primitive);
		header.cullFace = Shared::PlatformByteOrder::toCommonEndian(header.cullFace);

		for(unsigned int i = 0; i < 64; ++i) {
			header.texName[i] = Shared::PlatformByteOrder::toCommonEndian(header.texName[i]);
		}
	}
}

void fromEndianMeshHeaderV2(MeshHeaderV2 &header) {
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		header.vertexFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.vertexFrameCount);
		header.normalFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.normalFrameCount);
		header.texCoordFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.texCoordFrameCount);
		header.colorFrameCount = Shared::PlatformByteOrder::fromCommonEndian(header.colorFrameCount);
		header.pointCount = Shared::PlatformByteOrder::fromCommonEndian(header.pointCount);
		header.indexCount = Shared::PlatformByteOrder::fromCommonEndian(header.indexCount);
		header.hasTexture = Shared::PlatformByteOrder::fromCommonEndian(header.hasTexture);
		header.primitive = Shared::PlatformByteOrder::fromCommonEndian(header.primitive);
		header.cullFace = Shared::PlatformByteOrder::fromCommonEndian(header.cullFace);

		for(unsigned int i = 0; i < 64; ++i) {
			header.texName[i] = Shared::PlatformByteOrder::fromCommonEndian(header.texName[i]);
		}
	}
}

// =====================================================
//	class Mesh
// =====================================================

// ==================== constructor & destructor ====================

Mesh::Mesh() {
	textureManager = NULL;
	frameCount= 0;
	vertexCount= 0;
	indexCount= 0;
	texCoordFrameCount = 0;
	opacity = 0.0f;
	specularPower = 0.0f;

	vertices= NULL;
	normals= NULL;
	texCoords= NULL;
	tangents= NULL;
	indices= NULL;
	interpolationData= NULL;

	for(int i=0; i<meshTextureCount; ++i){
		textures[i]= NULL;
		texturesOwned[i]=false;
	}

	twoSided= false;
	customColor= false;
	noSelect= false;

	textureFlags=0;

	hasBuiltVBOs = false;
	// Vertex Buffer Object Names
	m_nVBOVertices	= 0;
	m_nVBOTexCoords	= 0;
	m_nVBONormals	= 0;
	m_nVBOIndexes	= 0;
}

Mesh::~Mesh() {
	end();
}

void Mesh::init() {
	try {
		vertices= new Vec3f[frameCount*vertexCount];
	}
	catch(bad_alloc& ba) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,(frameCount*vertexCount),ba.what());
		throw megaglest_runtime_error(szBuf);
	}
	try {
		normals= new Vec3f[frameCount*vertexCount];
	}
	catch(bad_alloc& ba) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,(frameCount*vertexCount),ba.what());
		throw megaglest_runtime_error(szBuf);
	}
	try {
		texCoords= new Vec2f[vertexCount];
	}
	catch(bad_alloc& ba) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,vertexCount,ba.what());
		throw megaglest_runtime_error(szBuf);
	}
	try {
		indices= new uint32[indexCount];
	}
	catch(bad_alloc& ba) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,indexCount,ba.what());
		throw megaglest_runtime_error(szBuf);
	}
}

void Mesh::end() {
	ReleaseVBOs();

	delete [] vertices;
	vertices=NULL;
	delete [] normals;
	normals=NULL;
	delete [] texCoords;
	texCoords=NULL;
	delete [] tangents;
	tangents=NULL;
	delete [] indices;
	indices=NULL;

	delete interpolationData;
	interpolationData=NULL;

	if(textureManager != NULL) {
		for(int i = 0; i < meshTextureCount; ++i) {
			if(texturesOwned[i] == true && textures[i] != NULL) {
				//printf("Deleting Texture [%s] i = %d\n",textures[i]->getPath().c_str(),i);
				textureManager->endTexture(textures[i]);
				textures[i] = NULL;
			}
		}
	}
	textureManager = NULL;
}

// ========================== shadows & interpolation =========================

void Mesh::buildInterpolationData(){
	if(interpolationData != NULL) {
		printf("**WARNING possible memory leak [Mesh::buildInterpolationData()]\n");
	}
	interpolationData= new InterpolationData(this);
}

void Mesh::updateInterpolationData(float t, bool cycle) {
	if(interpolationData != NULL) {
		interpolationData->update(t, cycle);
	}
}

void Mesh::updateInterpolationVertices(float t, bool cycle) {
	if(interpolationData != NULL) {
		interpolationData->updateVertices(t, cycle);
	}
}

void Mesh::BuildVBOs() {
	if(getVBOSupported() == true) {
		if(hasBuiltVBOs == false) {
			//printf("In [%s::%s Line: %d] setting up a VBO...\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			// Generate And Bind The Vertex Buffer
			glGenBuffersARB( 1,(GLuint*) &m_nVBOVertices );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOVertices );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f)*frameCount*vertexCount, getInterpolationData()->getVertices(), GL_STATIC_DRAW_ARB );
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Texture Coordinate Buffer
			glGenBuffersARB( 1, (GLuint*)&m_nVBOTexCoords );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOTexCoords );		// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(Vec2f)*vertexCount, texCoords, GL_STATIC_DRAW_ARB );
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Normal Buffer
			glGenBuffersARB( 1, (GLuint*)&m_nVBONormals );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBONormals );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f)*frameCount*vertexCount, getInterpolationData()->getNormals(), GL_STATIC_DRAW_ARB );
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Index Buffer
			glGenBuffersARB( 1, (GLuint*)&m_nVBOIndexes );					// Get A Valid Name
			glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, m_nVBOIndexes );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB,  sizeof(uint32)*indexCount, indices, GL_STATIC_DRAW_ARB );
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

			// Our Copy Of The Data Is No Longer Necessary, It Is Safe In The Graphics Card
			delete [] vertices; vertices = NULL;
			delete [] texCoords; texCoords = NULL;
			delete [] normals; normals = NULL;
			delete [] indices; indices = NULL;

			delete interpolationData;
			interpolationData = NULL;

			hasBuiltVBOs = true;
		}
	}
}

void Mesh::ReleaseVBOs() {
	if(getVBOSupported() == true) {
		if(hasBuiltVBOs == true) {
			glDeleteBuffersARB( 1, (GLuint*)&m_nVBOVertices );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&m_nVBOTexCoords );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&m_nVBONormals );					// Get A Valid Name
			glDeleteBuffersARB( 1, (GLuint*)&m_nVBOIndexes );					// Get A Valid Name
			hasBuiltVBOs = false;
		}
	}
}

// ==================== load ====================

string Mesh::findAlternateTexture(vector<string> conversionList, string textureFile) {
	string result = textureFile;
	string fileExt = extractExtension(textureFile);

	for(unsigned int i = 0; i < conversionList.size(); ++i) {
		string convertTo = conversionList[i];
		if(fileExt != convertTo) {
			string alternateTexture = textureFile;
			replaceAll(alternateTexture, "." + fileExt, "." + convertTo);
			if(fileExists(alternateTexture) == true) {
				result = alternateTexture;
				break;
			}
		}
	}
	return result;
}

void Mesh::loadV2(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
		bool deletePixMapAfterLoad, std::map<string,vector<pair<string, string> > > *loadedFileList,
		string sourceLoader,string modelFile) {
	this->textureManager = textureManager;
	//read header
	MeshHeaderV2 meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeaderV2), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianMeshHeaderV2(meshHeader);

	if(meshHeader.normalFrameCount != meshHeader.vertexFrameCount) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Old v2 model: vertex frame count different from normal frame count [v = %d, n = %d] meshIndex = %d modelFile [%s]",meshHeader.vertexFrameCount,meshHeader.normalFrameCount,meshIndex,modelFile.c_str());
		throw megaglest_runtime_error(szBuf);
	}

	if(meshHeader.texCoordFrameCount != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Old v2 model: texture coord frame count is not 1 [t = %d] meshIndex = %d modelFile [%s]",meshHeader.texCoordFrameCount,meshIndex,modelFile.c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//init
	frameCount= meshHeader.vertexFrameCount;
	vertexCount= meshHeader.pointCount;
	indexCount= meshHeader.indexCount;
	texCoordFrameCount = meshHeader.texCoordFrameCount;

	init();

	//misc
	twoSided= false;
	customColor= false;
	noSelect= false;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load v2, this = %p Found meshHeader.hasTexture = %d, texName [%s] mtDiffuse = %d meshIndex = %d modelFile [%s]\n",this,meshHeader.hasTexture,toLower(reinterpret_cast<char*>(meshHeader.texName)).c_str(),mtDiffuse,meshIndex,modelFile.c_str());

	textureFlags= 0;
	if(meshHeader.hasTexture) {
		textureFlags= 1;
	}

	//texture
	if(meshHeader.hasTexture && textureManager!=NULL){
		texturePaths[mtDiffuse]= toLower(reinterpret_cast<char*>(meshHeader.texName));
		string texPath= dir;
        if(texPath != "") {
        	endPathWithSlash(texPath);
        }
		texPath += texturePaths[mtDiffuse];

		textures[mtDiffuse]= dynamic_cast<Texture2D*>(textureManager->getTexture(texPath));
		if(textures[mtDiffuse] == NULL) {
			if(fileExists(texPath) == false) {
				vector<string> conversionList;
				conversionList.push_back("png");
				conversionList.push_back("jpg");
				conversionList.push_back("tga");
				conversionList.push_back("bmp");
				texPath = findAlternateTexture(conversionList, texPath);
			}
			if(fileExists(texPath) == true) {
				textures[mtDiffuse]= textureManager->newTexture2D();
				textures[mtDiffuse]->load(texPath);
				if(loadedFileList) {
					(*loadedFileList)[texPath].push_back(make_pair(sourceLoader,sourceLoader));
				}
				texturesOwned[mtDiffuse]=true;
				textures[mtDiffuse]->init(textureManager->getTextureFilter(),textureManager->getMaxAnisotropy());
				if(deletePixMapAfterLoad == true) {
					textures[mtDiffuse]->deletePixels();
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v2 model is missing texture [%s] meshIndex = %d modelFile [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,texPath.c_str(),meshIndex,modelFile.c_str());
			}
		}
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(vertices, frameCount*vertexCount);

	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(normals, frameCount*vertexCount);

	if(textures[mtDiffuse] != NULL) {
		readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
		if(readBytes != 1 && vertexCount != 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		fromEndianVecArray<Vec2f>(texCoords, vertexCount);
	}
	readBytes = fread(&diffuseColor, sizeof(Vec3f), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(&diffuseColor, 1);

	readBytes = fread(&opacity, sizeof(float32), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	opacity = Shared::PlatformByteOrder::fromCommonEndian(opacity);

	fseek(f, sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);
	if(readBytes != 1 && indexCount != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,indexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	Shared::PlatformByteOrder::fromEndianTypeArray<uint32>(indices, indexCount);
}

void Mesh::loadV3(int meshIndex, const string &dir, FILE *f,
		TextureManager *textureManager,bool deletePixMapAfterLoad,
		std::map<string,vector<pair<string, string> > > *loadedFileList,
		string sourceLoader,string modelFile) {
	this->textureManager = textureManager;

	//read header
	MeshHeaderV3 meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeaderV3), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianMeshHeaderV3(meshHeader);

	if(meshHeader.normalFrameCount != meshHeader.vertexFrameCount) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Old v3 model: vertex frame count different from normal frame count [v = %d, n = %d] meshIndex = %d modelFile [%s]",meshHeader.vertexFrameCount,meshHeader.normalFrameCount,meshIndex,modelFile.c_str());
		throw megaglest_runtime_error(szBuf);
	}

	//init
	frameCount= meshHeader.vertexFrameCount;
	vertexCount= meshHeader.pointCount;
	indexCount= meshHeader.indexCount;
	texCoordFrameCount = meshHeader.texCoordFrameCount;

	init();

	//misc
	twoSided= (meshHeader.properties & mp3TwoSided) != 0;
	customColor= (meshHeader.properties & mp3CustomColor) != 0;
	noSelect = false;

	textureFlags= 0;
	if((meshHeader.properties & mp3NoTexture) != mp3NoTexture) {
		textureFlags= 1;
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load v3, this = %p Found meshHeader.properties = %d, textureFlags = %d, texName [%s] mtDiffuse = %d meshIndex = %d modelFile [%s]\n",this,meshHeader.properties,textureFlags,toLower(reinterpret_cast<char*>(meshHeader.texName)).c_str(),mtDiffuse,meshIndex,modelFile.c_str());

	//texture
	if((meshHeader.properties & mp3NoTexture) != mp3NoTexture && textureManager!=NULL){
		texturePaths[mtDiffuse]= toLower(reinterpret_cast<char*>(meshHeader.texName));

		string texPath= dir;
        if(texPath != "") {
        	endPathWithSlash(texPath);
        }
		texPath += texturePaths[mtDiffuse];

		textures[mtDiffuse]= dynamic_cast<Texture2D*>(textureManager->getTexture(texPath));
		if(textures[mtDiffuse] == NULL) {
			if(fileExists(texPath) == false) {
				vector<string> conversionList;
				conversionList.push_back("png");
				conversionList.push_back("jpg");
				conversionList.push_back("tga");
				conversionList.push_back("bmp");
				texPath = findAlternateTexture(conversionList, texPath);
			}

			if(fileExists(texPath) == true) {
				textures[mtDiffuse]= textureManager->newTexture2D();
				textures[mtDiffuse]->load(texPath);
				if(loadedFileList) {
					(*loadedFileList)[texPath].push_back(make_pair(sourceLoader,sourceLoader));
				}

				texturesOwned[mtDiffuse]=true;
				textures[mtDiffuse]->init(textureManager->getTextureFilter(),textureManager->getMaxAnisotropy());
				if(deletePixMapAfterLoad == true) {
					textures[mtDiffuse]->deletePixels();
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v3 model is missing texture [%s] meshHeader.properties = %d meshIndex = %d modelFile [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,texPath.c_str(),meshHeader.properties,meshIndex,modelFile.c_str());
			}
		}
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(vertices, frameCount*vertexCount);

	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(normals, frameCount*vertexCount);

	if(textures[mtDiffuse] != NULL) {
		for(unsigned int i=0; i<meshHeader.texCoordFrameCount; ++i){
			readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
			if(readBytes != 1 && vertexCount != 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
				throw megaglest_runtime_error(szBuf);
			}
			fromEndianVecArray<Vec2f>(texCoords, vertexCount);
		}
	}
	readBytes = fread(&diffuseColor, sizeof(Vec3f), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(&diffuseColor, 1);

	readBytes = fread(&opacity, sizeof(float32), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	opacity = Shared::PlatformByteOrder::fromCommonEndian(opacity);

	fseek(f, sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);
	if(readBytes != 1 && indexCount != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,indexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	Shared::PlatformByteOrder::fromEndianTypeArray<uint32>(indices, indexCount);
}

Texture2D* Mesh::loadMeshTexture(int meshIndex, int textureIndex,
		TextureManager *textureManager, string textureFile,
		int textureChannelCount, bool &textureOwned, bool deletePixMapAfterLoad,
		std::map<string,vector<pair<string, string> > > *loadedFileList,
		string sourceLoader,string modelFile) {

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] #1 load texture [%s] modelFile [%s]\n",__FUNCTION__,textureFile.c_str(),modelFile.c_str());

	Texture2D* texture = dynamic_cast<Texture2D*>(textureManager->getTexture(textureFile));
	if(texture == NULL) {
		if(fileExists(textureFile) == false) {
			vector<string> conversionList;
			conversionList.push_back("png");
			conversionList.push_back("jpg");
			conversionList.push_back("tga");
			conversionList.push_back("bmp");
			textureFile = findAlternateTexture(conversionList, textureFile);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] #2 load texture [%s]\n",__FUNCTION__,textureFile.c_str());
		}

		if(fileExists(textureFile) == true) {
			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] texture exists loading [%s]\n",__FUNCTION__,textureFile.c_str());

			texture = textureManager->newTexture2D();
			if(textureChannelCount != -1) {
				texture->getPixmap()->init(textureChannelCount);
			}
			texture->load(textureFile);
			if(loadedFileList) {
				(*loadedFileList)[textureFile].push_back(make_pair(sourceLoader,sourceLoader));
			}

			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] texture loaded [%s]\n",__FUNCTION__,textureFile.c_str());

			textureOwned = true;
			texture->init(textureManager->getTextureFilter(),textureManager->getMaxAnisotropy());
			if(deletePixMapAfterLoad == true) {
				texture->deletePixels();
			}

			//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] texture inited [%s]\n",__FUNCTION__,textureFile.c_str());
		}
		else {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] #3 cannot load texture [%s] modelFile [%s]\n",__FUNCTION__,textureFile.c_str(),modelFile.c_str());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v4 model is missing texture [%s] textureFlags = %d meshIndex = %d textureIndex = %d modelFile [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,textureFile.c_str(),textureFlags,meshIndex,textureIndex,modelFile.c_str());
		}
	}

	return texture;
}

void Mesh::load(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
				bool deletePixMapAfterLoad,std::map<string,vector<pair<string, string> > > *loadedFileList,
				string sourceLoader,string modelFile) {
	this->textureManager = textureManager;

	//read header
	MeshHeader meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeader), 1, f);
	if(readBytes != 1) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianMeshHeader(meshHeader);

	name = reinterpret_cast<char*>(meshHeader.name);

	//printf("Load, Found meshTextureCount = %d, meshHeader.textures = %d\n",meshTextureCount,meshHeader.textures);

	//init
	frameCount= meshHeader.frameCount;
	vertexCount= meshHeader.vertexCount;
	indexCount= meshHeader.indexCount;

	init();

	//properties
	customColor= (meshHeader.properties & mpfCustomColor) != 0;
	twoSided= (meshHeader.properties & mpfTwoSided) != 0;
	noSelect= (meshHeader.properties & mpfNoSelect) != 0;

	//material
	diffuseColor= Vec3f(meshHeader.diffuseColor);
	specularColor= Vec3f(meshHeader.specularColor);
	specularPower= meshHeader.specularPower;
	opacity= meshHeader.opacity;

	textureFlags= meshHeader.textures;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load v4, this = %p Found meshHeader.textures = %d meshIndex = %d\n",this,meshHeader.textures,meshIndex);

	//maps
	uint32 flag= 1;
	for(int i = 0; i < meshTextureCount; ++i) {
		if((meshHeader.textures & flag) && textureManager != NULL) {
			uint8 cMapPath[mapPathSize];
			readBytes = fread(cMapPath, mapPathSize, 1, f);
			if(readBytes != 1 && mapPathSize != 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,mapPathSize,__LINE__);
				throw megaglest_runtime_error(szBuf);
			}
			Shared::PlatformByteOrder::fromEndianTypeArray<uint8>(cMapPath, mapPathSize);

			string mapPath= toLower(reinterpret_cast<char*>(cMapPath));

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapPath [%s] meshHeader.textures = %d flag = %d (meshHeader.textures & flag) = %d meshIndex = %d i = %d\n",mapPath.c_str(),meshHeader.textures,flag,(meshHeader.textures & flag),meshIndex,i);

			string mapFullPath= dir;
			if(mapFullPath != "") {
                endPathWithSlash(mapFullPath);
			}
			mapFullPath += mapPath;

			textures[i] = loadMeshTexture(meshIndex, i, textureManager, mapFullPath,
					meshTextureChannelCount[i],texturesOwned[i],
					deletePixMapAfterLoad, loadedFileList, sourceLoader,modelFile);
		}
		flag *= 2;
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(vertices, frameCount*vertexCount);

	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(readBytes != 1 && (frameCount * vertexCount) != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	fromEndianVecArray<Vec3f>(normals, frameCount*vertexCount);

	if(meshHeader.textures!=0){
		readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
		if(readBytes != 1 && vertexCount != 0) {
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u][%u] on line: %d.",readBytes,frameCount,vertexCount,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		fromEndianVecArray<Vec2f>(texCoords, vertexCount);
	}
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);
	if(readBytes != 1 && indexCount != 0) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,indexCount,__LINE__);
		throw megaglest_runtime_error(szBuf);
	}
	Shared::PlatformByteOrder::fromEndianTypeArray<uint32>(indices, indexCount);

	//tangents
	if(textures[mtNormal]!=NULL){
		computeTangents();
	}
}

void Mesh::save(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
		string convertTextureToFormat, std::map<string,int> &textureDeleteList,
		bool keepsmallest,string modelFile) {
	MeshHeader meshHeader;
	memset(&meshHeader, 0, sizeof(struct MeshHeader));

	strncpy((char*)meshHeader.name, (char*)name.c_str(), name.length());
	meshHeader.frameCount= frameCount;
	meshHeader.vertexCount= vertexCount;
	meshHeader.indexCount = indexCount;

	//material
	memcpy((float32*)meshHeader.diffuseColor, (float32*)diffuseColor.ptr(), sizeof(float32) * 3);
	memcpy((float32*)meshHeader.specularColor, (float32*)specularColor.ptr(), sizeof(float32) * 3);
	meshHeader.specularPower = specularPower;
	meshHeader.opacity = opacity;

	//properties
	meshHeader.properties = 0;
	if(customColor) {
		meshHeader.properties |= mpfCustomColor;
	}
	if(twoSided) {
		meshHeader.properties |= mpfTwoSided;
	}
	if(noSelect) {
		meshHeader.properties |= mpfNoSelect;
	}

	meshHeader.textures = textureFlags;
	fwrite(&meshHeader, sizeof(MeshHeader), 1, f);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, this = %p, Found meshTextureCount = %d, meshHeader.textures = %d meshIndex = %d\n",this,meshTextureCount,meshHeader.textures,meshIndex);

	//maps
	uint32 flag= 1;
	for(int i = 0; i < meshTextureCount; ++i) {
		if((meshHeader.textures & flag)) {
			uint8 cMapPath[mapPathSize];
			memset(&cMapPath[0],0,mapPathSize);

			Texture2D *texture = textures[i];

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, [%d] mesh texture ptr [%p]\n",i,texture);

			if(texture != NULL) {
				string file = toLower(texture->getPath());

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, Found mesh texture [%s]\n",file.c_str());

				if(toLower(convertTextureToFormat) != "" &&
					EndsWith(file, "." + convertTextureToFormat) == false) {
					long originalSize 	= getFileSize(file);
					long newSize 		= originalSize;

					string fileExt = extractExtension(file);
					replaceAll(file, "." + fileExt, "." + convertTextureToFormat);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, Convert from [%s] to [%s]\n",texture->getPath().c_str(),file.c_str());

					if(convertTextureToFormat == "tga") {
						texture->getPixmap()->saveTga(file);
						newSize = getFileSize(file);
						if(keepsmallest == false || newSize <= originalSize) {
							textureDeleteList[texture->getPath()] = textureDeleteList[texture->getPath()] + 1;
						}
						else {
							printf("Texture will not be converted, keeping smallest texture [%s]\n",texture->getPath().c_str());
							textureDeleteList[file] = textureDeleteList[file] + 1;
						}
					}
					else if(convertTextureToFormat == "bmp") {
						texture->getPixmap()->saveBmp(file);
						newSize = getFileSize(file);
						if(keepsmallest == false || newSize <= originalSize) {
							textureDeleteList[texture->getPath()] = textureDeleteList[texture->getPath()] + 1;
						}
						else {
							printf("Texture will not be converted, keeping smallest texture [%s]\n",texture->getPath().c_str());
							textureDeleteList[file] = textureDeleteList[file] + 1;
						}
					}
					else if(convertTextureToFormat == "jpg") {
						texture->getPixmap()->saveJpg(file);
						newSize = getFileSize(file);
						if(keepsmallest == false || newSize <= originalSize) {
							textureDeleteList[texture->getPath()] = textureDeleteList[texture->getPath()] + 1;
						}
						else {
							printf("Texture will not be converted, keeping smallest texture [%s]\n",texture->getPath().c_str());
							textureDeleteList[file] = textureDeleteList[file] + 1;
						}
					}
					else  if(convertTextureToFormat == "png") {
						texture->getPixmap()->savePng(file);
						newSize = getFileSize(file);
						if(keepsmallest == false || newSize <= originalSize) {
							textureDeleteList[texture->getPath()] = textureDeleteList[texture->getPath()] + 1;
						}
						else {
							printf("Texture will not be converted, keeping smallest texture [%s]\n",texture->getPath().c_str());
							textureDeleteList[file] = textureDeleteList[file] + 1;
						}
					}
					else {
						throw megaglest_runtime_error("Unsupported texture format: [" + convertTextureToFormat + "]");
					}

					//textureManager->endTexture(texture);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, load new texture [%s] originalSize [%ld] newSize [%ld]\n",file.c_str(),originalSize,newSize);

					if(keepsmallest == false || newSize <= originalSize) {
						texture = loadMeshTexture(meshIndex, i, textureManager,file,
													meshTextureChannelCount[i],
													texturesOwned[i],
													false,
													NULL,
													"",
													modelFile);
					}
				}

				file = extractFileFromDirectoryPath(texture->getPath());

				if(file.length() > mapPathSize) {
					throw megaglest_runtime_error("file.length() > mapPathSize, file.length() = " + intToStr(file.length()));
				}
				else if(file.length() == 0) {
					throw megaglest_runtime_error("file.length() == 0");
				}

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, new texture file [%s]\n",file.c_str());

				memset(&cMapPath[0],0,mapPathSize);
				memcpy(&cMapPath[0],file.c_str(),file.length());
			}

			fwrite(cMapPath, mapPathSize, 1, f);
		}
		flag*= 2;
	}

	//read data
	fwrite(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	fwrite(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(meshHeader.textures != 0) {
		fwrite(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
	}
	fwrite(indices, sizeof(uint32)*indexCount, 1, f);
}

void Mesh::computeTangents(){
	delete [] tangents;
	try {
		tangents= new Vec3f[vertexCount];
	}
	catch(bad_alloc& ba) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,vertexCount,ba.what());
		throw megaglest_runtime_error(szBuf);
	}

	for(unsigned int i=0; i<vertexCount; ++i){
		tangents[i]= Vec3f(0.f);
	}

	for(unsigned int i=0; i<indexCount; i+=3){
		for(int j=0; j<3; ++j){
			uint32 i0= indices[i+j];
			uint32 i1= indices[i+(j+1)%3];
			uint32 i2= indices[i+(j+2)%3];

			Vec3f p0= vertices[i0];
			Vec3f p1= vertices[i1];
			Vec3f p2= vertices[i2];

			float u0= texCoords[i0].x;
			float u1= texCoords[i1].x;
			float u2= texCoords[i2].x;

			float v0= texCoords[i0].y;
			float v1= texCoords[i1].y;
			float v2= texCoords[i2].y;

			tangents[i0]+=
				((p2-p0)*(v1-v0)-(p1-p0)*(v2-v0))/
				((u2-u0)*(v1-v0)-(u1-u0)*(v2-v0));
		}
	}

	for(unsigned int i=0; i<vertexCount; ++i){
		/*Vec3f binormal= normals[i].cross(tangents[i]);
		tangents[i]+= binormal.cross(normals[i]);*/
		tangents[i].normalize();
	}
}

void Mesh::deletePixels() {
	for(int i = 0; i < meshTextureCount; ++i) {
		if(textures[i] != NULL) {
			textures[i]->deletePixels();
		}
	}
}

// ===============================================
//	class Model
// ===============================================

// ==================== constructor & destructor ====================

Model::Model() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
	meshCount		= 0;
	meshes			= NULL;
	textureManager	= NULL;
	lastTData		= -1;
	lastCycleData	= false;
	lastTVertex		= -1;
	lastCycleVertex	= false;
}

Model::~Model() {
	delete [] meshes;
	meshes = NULL;
}

// ==================== data ====================

void Model::buildInterpolationData() const{
	for(unsigned int i=0; i<meshCount; ++i){
		meshes[i].buildInterpolationData();
	}
}

void Model::updateInterpolationData(float t, bool cycle) {
	if(lastTData != t || lastCycleData != cycle) {
		for(unsigned int i = 0; i < meshCount; ++i) {
			meshes[i].updateInterpolationData(t, cycle);
		}
		lastTData 		= t;
		lastCycleData 	= cycle;
	}
}

void Model::updateInterpolationVertices(float t, bool cycle) {
	if(lastTVertex != t || lastCycleVertex != cycle) {
		for(unsigned int i = 0; i < meshCount; ++i) {
			meshes[i].updateInterpolationVertices(t, cycle);
		}
		lastTVertex 	= t;
		lastCycleVertex = cycle;
	}
}

// ==================== get ====================

uint32 Model::getTriangleCount() const {
	uint32 triangleCount= 0;
	for(uint32 i = 0; i < meshCount; ++i) {
		triangleCount += meshes[i].getIndexCount()/3;
	}
	return triangleCount;
}

uint32 Model::getVertexCount() const {
	uint32 vertexCount= 0;
	for(uint32 i = 0; i < meshCount; ++i) {
		vertexCount += meshes[i].getVertexCount();
	}
	return vertexCount;
}

// ==================== io ====================

void Model::load(const string &path, bool deletePixMapAfterLoad,
		std::map<string,vector<pair<string, string> > > *loadedFileList, string *sourceLoader) {

	this->sourceLoader = (sourceLoader != NULL ? *sourceLoader : "");
	this->fileName = path;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == true) {
		return;
	}
	string extension= path.substr(path.find_last_of('.') + 1);
	if(extension=="g3d" || extension=="G3D") {
		loadG3d(path,deletePixMapAfterLoad,loadedFileList, this->sourceLoader);
	}
	else {
		throw megaglest_runtime_error("Unknown model format: " + extension);
	}
}

void Model::save(const string &path, string convertTextureToFormat,
		bool keepsmallest) {
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="g3d" ||extension=="G3D") {
		saveG3d(path,convertTextureToFormat,keepsmallest);
	}
	else {
		throw megaglest_runtime_error("Unknown model format: " + extension);
	}
}

//load a model from a g3d file
void Model::loadG3d(const string &path, bool deletePixMapAfterLoad,
		std::map<string,vector<pair<string, string> > > *loadedFileList,
		string sourceLoader) {

    try{
#ifdef WIN32
		FILE *f= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
		FILE *f=fopen(path.c_str(),"rb");
#endif
		if (f == NULL) {
		    printf("In [%s::%s] cannot load file = [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,path.c_str());
			throw megaglest_runtime_error("Error opening g3d model file [" + path + "]");
		}

		if(loadedFileList) {
			(*loadedFileList)[path].push_back(make_pair(sourceLoader,sourceLoader));
		}

		string dir= extractDirectoryPathFromFile(path);

		//file header
		FileHeader fileHeader;
		size_t readBytes = fread(&fileHeader, sizeof(FileHeader), 1, f);
		if(readBytes != 1) {
			fclose(f);
			char szBuf[8096]="";
			snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
			throw megaglest_runtime_error(szBuf);
		}
		fromEndianFileHeader(fileHeader);

		if(strncmp(reinterpret_cast<char*>(fileHeader.id), "G3D", 3) != 0) {
			fclose(f);
			f = NULL;
		    printf("In [%s::%s] file = [%s] fileheader.id = [%s][%c]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,path.c_str(),reinterpret_cast<char*>(fileHeader.id),fileHeader.id[0]);
			throw megaglest_runtime_error("Not a valid G3D model");
		}
		fileVersion= fileHeader.version;

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load model, fileVersion = %d\n",fileVersion);

		//version 4
		if(fileHeader.version == 4) {
			//model header
			ModelHeader modelHeader;
			readBytes = fread(&modelHeader, sizeof(ModelHeader), 1, f);
			if(readBytes != 1) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " on line: %d.",readBytes,__LINE__);
				throw megaglest_runtime_error(szBuf);
			}
			fromEndianModelHeader(modelHeader);

			meshCount= modelHeader.meshCount;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			if(modelHeader.type != mtMorphMesh) {
				throw megaglest_runtime_error("Invalid model type");
			}

			//load meshes
			try {
				meshes= new Mesh[meshCount];
			}
			catch(bad_alloc& ba) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,meshCount,ba.what());
				throw megaglest_runtime_error(szBuf);
			}

			for(uint32 i = 0; i < meshCount; ++i) {
				meshes[i].load(i, dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList,sourceLoader,path);
				meshes[i].buildInterpolationData();
			}
		}
		//version 3
		else if(fileHeader.version == 3) {
			readBytes = fread(&meshCount, sizeof(meshCount), 1, f);
			if(readBytes != 1 && meshCount != 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,meshCount,__LINE__);
				throw megaglest_runtime_error(szBuf);
			}
			meshCount = Shared::PlatformByteOrder::fromCommonEndian(meshCount);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			try {
				meshes= new Mesh[meshCount];
			}
			catch(bad_alloc& ba) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,meshCount,ba.what());
				throw megaglest_runtime_error(szBuf);
			}

			for(uint32 i = 0; i < meshCount; ++i) {
				meshes[i].loadV3(i, dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList,sourceLoader,path);
				meshes[i].buildInterpolationData();
			}
		}
		//version 2
		else if(fileHeader.version == 2) {
			readBytes = fread(&meshCount, sizeof(meshCount), 1, f);
			if(readBytes != 1 && meshCount != 0) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"fread returned wrong size = " MG_SIZE_T_SPECIFIER " [%u] on line: %d.",readBytes,meshCount,__LINE__);
				throw megaglest_runtime_error(szBuf);
			}
			meshCount = Shared::PlatformByteOrder::fromCommonEndian(meshCount);


			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			try {
				meshes= new Mesh[meshCount];
			}
			catch(bad_alloc& ba) {
				char szBuf[8096]="";
				snprintf(szBuf,8096,"Error on line: %d size: %d msg: %s\n",__LINE__,meshCount,ba.what());
				throw megaglest_runtime_error(szBuf);
			}

			for(uint32 i = 0; i < meshCount; ++i){
				meshes[i].loadV2(i,dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList,sourceLoader,path);
				meshes[i].buildInterpolationData();
			}
		}
		else {
			throw megaglest_runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
		}

		fclose(f);
    }
    catch(megaglest_runtime_error& ex) {
    	//printf("1111111 ex.wantStackTrace() = %d\n",ex.wantStackTrace());
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.what());
		//printf("2222222\n");
		throw megaglest_runtime_error("Exception caught loading 3d file: " + path +"\n"+ ex.what(),!ex.wantStackTrace());
    }
	catch(exception &e){
		//abort();
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Exception caught loading 3d file: " + path +"\n"+ e.what());
	}
}

//save a model to a g3d file
void Model::saveG3d(const string &path, string convertTextureToFormat,
		bool keepsmallest) {
	string tempModelFilename = path + "cvt";

#ifdef WIN32
	FILE *f= _wfopen(utf8_decode(tempModelFilename).c_str(), L"wb");
#else
	FILE *f= fopen(tempModelFilename.c_str(), "wb");
#endif
	if(f == NULL) {
		throw megaglest_runtime_error("Cant open file for writing: [" + tempModelFilename + "]");
	}

	convertTextureToFormat = toLower(convertTextureToFormat);

	//file header
	FileHeader fileHeader;
	fileHeader.id[0]= 'G';
	fileHeader.id[1]= '3';
	fileHeader.id[2]= 'D';
	fileHeader.version= 4;

	fwrite(&fileHeader, sizeof(FileHeader), 1, f);

	// file versions
	if(fileHeader.version == 4 || fileHeader.version == 3 || fileHeader.version == 2) {
		//model header
		ModelHeader modelHeader;
		modelHeader.meshCount = meshCount;
		modelHeader.type = mtMorphMesh;

		fwrite(&modelHeader, sizeof(ModelHeader), 1, f);

		std::map<string,int> textureDeleteList;
		for(uint32 i = 0; i < meshCount; ++i) {
			meshes[i].save(i,tempModelFilename, f, textureManager,
					convertTextureToFormat,textureDeleteList,
					keepsmallest,path);
		}

		removeFile(path);
		if(renameFile(tempModelFilename,path) == true) {
			// Now delete old textures since they were converted to a new format
			for(std::map<string,int>::iterator iterMap = textureDeleteList.begin();
				iterMap != textureDeleteList.end(); ++iterMap) {
				removeFile(iterMap->first);
			}
		}
	}
	else {
		throw megaglest_runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
	}

	fclose(f);
}

void Model::deletePixels() {
	for(uint32 i = 0; i < meshCount; ++i) {
		meshes[i].deletePixels();
	}
}

bool PixelBufferWrapper::isPBOEnabled 	= false;
int PixelBufferWrapper::index 			= 0;
vector<unsigned int> PixelBufferWrapper::pboIds;

PixelBufferWrapper::PixelBufferWrapper(int pboCount,int bufferSize) {
	//if(isGlExtensionSupported("GL_ARB_pixel_buffer_object") == true &&
	if(GLEW_ARB_pixel_buffer_object) {
		PixelBufferWrapper::isPBOEnabled = true;
		cleanup();
		// For some wacky reason this fails in VC++ 2008
		//pboIds.reserve(pboCount);
		//glGenBuffersARB(pboCount, (GLuint*)&pboIds[0]);
		//

		for(int i = 0; i < pboCount; ++i) {
			pboIds.push_back(0);
			glGenBuffersARB(1, (GLuint*)&pboIds[i]);
			// create pixel buffer objects, you need to delete them when program exits.
			// glBufferDataARB with NULL pointer reserves only memory space.
			glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[i]);
			glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, bufferSize, 0, GL_STREAM_READ_ARB);
		}
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}
}

Pixmap2D *PixelBufferWrapper::getPixelBufferFor(int x,int y,int w,int h, int colorComponents) {
	Pixmap2D *pixmapScreenShot = NULL;
	if(PixelBufferWrapper::isPBOEnabled == true) {
	    // increment current index first then get the next index
	    // "index" is used to read pixels from a framebuffer to a PBO
	    // "nextIndex" is used to process pixels in the other PBO
	    index = (index + 1) % 2;

	    // pbo index used for next frame
	    int nextIndex = (index + 1) % 2;

	    // read framebuffer ///////////////////////////////
		// copy pixels from framebuffer to PBO
		// Use offset instead of pointer.
		// OpenGL should perform asynch DMA transfer, so glReadPixels() will return immediately.
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[index]);
	    //glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[nextIndex]);

		//glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// measure the time reading framebuffer
		//t1.stop();
		//readTime = t1.getElapsedTimeInMilliSec();

		// process pixel data /////////////////////////////
		//t1.start();

		// map the PBO that contain framebuffer pixels before processing it
		//glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[nextIndex]);
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[index]);
		GLubyte* src = (GLubyte*)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
		if(src) {
			pixmapScreenShot = new Pixmap2D(w+1, h+1, colorComponents);
			memcpy(pixmapScreenShot->getPixels(),src,(size_t)pixmapScreenShot->getPixelByteCount());
			glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);     // release pointer to the mapped buffer
			//pixmapScreenShot->save("debugPBO.png");
		}

		// measure the time reading framebuffer
		//t1.stop();
		//processTime = t1.getElapsedTimeInMilliSec();
		glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}

	return pixmapScreenShot;
}

void PixelBufferWrapper::begin() {
	if(PixelBufferWrapper::isPBOEnabled == true) {
		// set the framebuffer to read
		//glReadBuffer(GL_FRONT);
	}
}

void PixelBufferWrapper::end() {
	if(PixelBufferWrapper::isPBOEnabled == true) {
		// set the framebuffer to read
		//glReadBuffer(GL_BACK);
	}
}

void PixelBufferWrapper::cleanup() {
	if(PixelBufferWrapper::isPBOEnabled == true) {
		if(pboIds.empty() == false) {
			glDeleteBuffersARB(pboIds.size(), &pboIds[0]);
			pboIds.clear();
		}
	}
}

PixelBufferWrapper::~PixelBufferWrapper() {
	cleanup();
}

//unsigned char BaseColorPickEntity::nextColorID[COLOR_COMPONENTS] = {1, 1, 1, 1};
unsigned char BaseColorPickEntity::nextColorID[COLOR_COMPONENTS] = { 1, 1, 1 };
Mutex BaseColorPickEntity::mutexNextColorID;
auto_ptr<PixelBufferWrapper> BaseColorPickEntity::pbo;

BaseColorPickEntity::BaseColorPickEntity() {
	 MutexSafeWrapper safeMutex(&mutexNextColorID);

	 uniqueColorID[0] = nextColorID[0];
	 uniqueColorID[1] = nextColorID[1];
	 uniqueColorID[2] = nextColorID[2];
	 //uniqueColorID[3] = nextColorID[3];

	 const int colorSpacing = 2;

	 if(nextColorID[0] + colorSpacing <= 255) {
		 nextColorID[0] += colorSpacing;
	 }
	 else {
		 nextColorID[0] = 1;
	 	 if(nextColorID[1] + colorSpacing <= 255) {
	 		 nextColorID[1] += colorSpacing;
	 	 }
	 	 else {
       	   nextColorID[1] = 1;
       	   if(nextColorID[2] + colorSpacing <= 255) {
       		   nextColorID[2] += colorSpacing;
       	   }
       	   else {

        	   //printf("Color rolled over on 3rd level!\n");

			   nextColorID[0] = 1;
			   nextColorID[1] = 1;
			   nextColorID[2] = 1;


//          	   nextColorID[2] = 1;
//          	   nextColorID[3]+=colorSpacing;
//
//               if(nextColorID[3] > 255) {
//              	   nextColorID[0] = 1;
//              	   nextColorID[1] = 1;
//              	   nextColorID[2] = 1;
//              	   nextColorID[3] = 1;
//               }
           }
        }
     }
}

void BaseColorPickEntity::init(int bufferSize) {
	 if(BaseColorPickEntity::pbo.get() == NULL) {
		 BaseColorPickEntity::pbo.reset(new PixelBufferWrapper(2,bufferSize));
	 }
}

string BaseColorPickEntity::getColorDescription() const {
	char szBuf[100]="";
	snprintf(szBuf,100,"%d.%d.%d",uniqueColorID[0],uniqueColorID[1],uniqueColorID[2]);
	string result = szBuf;
	return result;
}

void BaseColorPickEntity::beginPicking() {
	// turn off texturing, lighting and fog
	//glClearColor (0.0,0.0,0.0,0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);

	glDisable(GL_BLEND);
	glDisable(GL_MULTISAMPLE);
	glDisable(GL_DITHER);
}

void BaseColorPickEntity::endPicking() {
	// turn off texturing, lighting and fog
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_FOG);
	glEnable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DITHER);
}

vector<int> BaseColorPickEntity::getPickedList(int x,int y,int w,int h,
						const vector<BaseColorPickEntity *> &rendererModels) {
	vector<int> pickedModels;

	//printf("In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//static auto_ptr<unsigned char> cachedPixels;
	static auto_ptr<Pixmap2D> cachedPixels;
	//static int cachedPixelsW = -1;
	//static int cachedPixelsH = -1;

	//printf("In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	static Chrono lastSnapshot(true);
	const int selectionMillisecondUpdate = 100;

	if(PixelBufferWrapper::getIsPBOEnable() == true) {
		// Only update the pixel buffer every x milliseconds or as required
		if(cachedPixels.get() == NULL || cachedPixels->getW() != w+1 ||cachedPixels->getH() != h+1 ||
				lastSnapshot.getMillis() > selectionMillisecondUpdate) {
			//printf("Updating selection millis = %ld\n",lastSnapshot.getMillis());

			lastSnapshot.reset();
			//Pixmap2D *pixmapScreenShot = BaseColorPickEntity::pbo->getPixelBufferFor(x,y,w,h, COLOR_COMPONENTS);
			cachedPixels.reset(BaseColorPickEntity::pbo->getPixelBufferFor(x,y,w,h, COLOR_COMPONENTS));

			//cachedPixels.reset(new unsigned char[(unsigned int)pixmapScreenShot->getPixelByteCount()]);
			//memcpy(cachedPixels.get(),pixmapScreenShot->getPixels(),(size_t)pixmapScreenShot->getPixelByteCount());
			//cachedPixelsW = w+1;
			//cachedPixelsH = h+1;

			//delete pixmapScreenShot;
		}
	}
	else {
		// Only update the pixel buffer every x milliseconds or as required
		if(cachedPixels.get() == NULL || cachedPixels->getW() != w+1 ||cachedPixels->getH() != h+1 ||
				lastSnapshot.getMillis() > selectionMillisecondUpdate) {
			//printf("Updating selection millis = %ld\n",lastSnapshot.getMillis());

			lastSnapshot.reset();

			//Pixmap2D *pixmapScreenShot = new Pixmap2D(w+1, h+1, COLOR_COMPONENTS);
			cachedPixels.reset(new Pixmap2D(w+1, h+1, COLOR_COMPONENTS));
			//glPixelStorei(GL_PACK_ALIGNMENT, 1);
			//glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixmapScreenShot->getPixels());
			//glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixmapScreenShot->getPixels());
			glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, cachedPixels->getPixels());
			//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			//cachedPixels.reset(new unsigned char[(unsigned int)pixmapScreenShot->getPixelByteCount()]);
			//memcpy(cachedPixels.get(),pixmapScreenShot->getPixels(),(size_t)pixmapScreenShot->getPixelByteCount());
			//cachedPixelsW = w+1;
			//cachedPixelsH = h+1;

			//delete pixmapScreenShot;
		}
	}
	unsigned char *pixelBuffer = cachedPixels->getPixels();

	// Enable screenshots to debug selection scene
	//pixmapScreenShot->save("debug.png");

	//printf("In [%s::%s] Line: %d x,y,w,h [%d,%d,%d,%d] pixels = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,x,y,w,h,pixmapScreenShot->getPixelByteCount());

	// now our picked screen pixel color is stored in pixel[3]
	// so we search through our object list looking for the object that was selected

	map<int,bool> modelAlreadyPickedList;
	map<unsigned char,map<unsigned char, map<unsigned char,bool> > > colorAlreadyPickedList;
	int nEnd = w * h;
	for(int x = 0; x < nEnd && pickedModels.size() < rendererModels.size(); ++x) {
		int index = x * COLOR_COMPONENTS;
		unsigned char *pixel = &pixelBuffer[index];

		// Skip duplicate scanned colors
		map<unsigned char,map<unsigned char, map<unsigned char,bool> > >::iterator iterFind1 = colorAlreadyPickedList.find(pixel[0]);
		if(iterFind1 != colorAlreadyPickedList.end()) {
			map<unsigned char, map<unsigned char,bool> >::iterator iterFind2 = iterFind1->second.find(pixel[1]);
			if(iterFind2 != iterFind1->second.end()) {
				map<unsigned char,bool>::iterator iterFind3 = iterFind2->second.find(pixel[2]);
				if(iterFind3 != iterFind2->second.end()) {
					continue;
				}
			}
		}

		for(unsigned int i = 0; i < rendererModels.size(); ++i) {
			// Skip models already selected
			if(modelAlreadyPickedList.find(i) != modelAlreadyPickedList.end()) {
				continue;
			}
			const BaseColorPickEntity *model = rendererModels[i];

			if( model != NULL && model->isUniquePickingColor(pixel) == true) {
				//printf("Found match pixel [%d.%d.%d] for model [%s] ptr [%p][%s]\n",pixel[0],pixel[1],pixel[2],model->getColorDescription().c_str(), model,model->getUniquePickName().c_str());

				pickedModels.push_back(i);
				modelAlreadyPickedList[i]=true;
				colorAlreadyPickedList[pixel[0]][pixel[1]][pixel[2]]=true;
				break;
			}
		}
	}

	//printf("In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//delete pixmapScreenShot;

	//printf("In [%s::%s] Line: %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	return pickedModels;
}

bool BaseColorPickEntity::isUniquePickingColor(unsigned char *pixel) const {
	bool result = false;
	if( uniqueColorID[0] == pixel[0] &&
		uniqueColorID[1] == pixel[1] &&
		uniqueColorID[2] == pixel[2]) {
		//uniqueColorID[3] == pixel[3]) {
		result = true;
	}

	return result;
}

void BaseColorPickEntity::setUniquePickingColor() const {
	 glColor3f(	uniqueColorID[0] / 255.0f,
			 	uniqueColorID[1] / 255.0f,
			 	uniqueColorID[2] / 255.0f);
			 	//uniqueColorID[3] / 255.0f);
}


}}//end namespace
