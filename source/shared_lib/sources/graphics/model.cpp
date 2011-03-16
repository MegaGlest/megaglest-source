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

#include "model.h"

#include <cstdio>
#include <cassert>
#include <stdexcept>

#include "interpolation.h"
#include "conversion.h"
#include "util.h"
#include "platform_common.h"
#include "opengl.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::PlatformCommon;
using namespace Shared::Graphics::Gl;

using namespace std;

namespace Shared{ namespace Graphics{

using namespace Util;

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
	vertices= new Vec3f[frameCount*vertexCount];
	normals= new Vec3f[frameCount*vertexCount];
	texCoords= new Vec2f[vertexCount];
	indices= new uint32[indexCount];
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
			//printf("In [%s::%s Line: %d] setting up a VBO...\n",__FILE__,__FUNCTION__,__LINE__);

			// Generate And Bind The Vertex Buffer
			glGenBuffersARB( 1, &m_nVBOVertices );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOVertices );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f)*frameCount*vertexCount, getInterpolationData()->getVertices(), GL_STATIC_DRAW_ARB );
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Texture Coordinate Buffer
			glGenBuffersARB( 1, &m_nVBOTexCoords );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOTexCoords );		// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(Vec2f)*vertexCount, texCoords, GL_STATIC_DRAW_ARB );
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Normal Buffer
			glGenBuffersARB( 1, &m_nVBONormals );					// Get A Valid Name
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBONormals );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ARRAY_BUFFER_ARB,  sizeof(Vec3f)*frameCount*vertexCount, getInterpolationData()->getNormals(), GL_STATIC_DRAW_ARB );
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);

			// Generate And Bind The Index Buffer
			glGenBuffersARB( 1, &m_nVBOIndexes );					// Get A Valid Name
			glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, m_nVBOIndexes );			// Bind The Buffer
			// Load The Data
			glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB,  sizeof(uint32)*indexCount, indices, GL_STATIC_DRAW_ARB );
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

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
			glDeleteBuffersARB( 1, &m_nVBOVertices );					// Get A Valid Name
			glDeleteBuffersARB( 1, &m_nVBOTexCoords );					// Get A Valid Name
			glDeleteBuffersARB( 1, &m_nVBONormals );					// Get A Valid Name
			glDeleteBuffersARB( 1, &m_nVBOIndexes );					// Get A Valid Name
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
		bool deletePixMapAfterLoad, std::map<string,int> *loadedFileList) {
	this->textureManager = textureManager;
	//read header
	MeshHeaderV2 meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeaderV2), 1, f);


	if(meshHeader.normalFrameCount != meshHeader.vertexFrameCount) {
		char szBuf[4096]="";
		sprintf(szBuf,"Old v2 model: vertex frame count different from normal frame count [v = %d, n = %d] meshIndex = %d",meshHeader.vertexFrameCount,meshHeader.normalFrameCount,meshIndex);
		throw runtime_error(szBuf);
	}

	if(meshHeader.texCoordFrameCount != 1) {
		char szBuf[4096]="";
		sprintf(szBuf,"Old v2 model: texture coord frame count is not 1 [t = %d] meshIndex = %d",meshHeader.texCoordFrameCount,meshIndex);
		throw runtime_error(szBuf);
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

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load v2, this = %p Found meshHeader.hasTexture = %d, texName [%s] mtDiffuse = %d meshIndex = %d\n",this,meshHeader.hasTexture,toLower(reinterpret_cast<char*>(meshHeader.texName)).c_str(),mtDiffuse,meshIndex);

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
					(*loadedFileList)[texPath]++;
				}
				texturesOwned[mtDiffuse]=true;
				textures[mtDiffuse]->init(textureManager->getTextureFilter(),textureManager->getMaxAnisotropy());
				if(deletePixMapAfterLoad == true) {
					textures[mtDiffuse]->deletePixels();
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v2 model is missing texture [%s] meshIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,texPath.c_str(),meshIndex);
			}
		}
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(textures[mtDiffuse]!=NULL){
		readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
	}
	readBytes = fread(&diffuseColor, sizeof(Vec3f), 1, f);
	readBytes = fread(&opacity, sizeof(float32), 1, f);
	fseek(f, sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);
}

void Mesh::loadV3(int meshIndex, const string &dir, FILE *f,
		TextureManager *textureManager,bool deletePixMapAfterLoad,
		std::map<string,int> *loadedFileList) {
	this->textureManager = textureManager;

	//read header
	MeshHeaderV3 meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeaderV3), 1, f);


	if(meshHeader.normalFrameCount != meshHeader.vertexFrameCount) {
		char szBuf[4096]="";
		sprintf(szBuf,"Old v3 model: vertex frame count different from normal frame count [v = %d, n = %d] meshIndex = %d",meshHeader.vertexFrameCount,meshHeader.normalFrameCount,meshIndex);
		throw runtime_error(szBuf);
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

	textureFlags= 0;
	if((meshHeader.properties & mp3NoTexture) != mp3NoTexture) {
		textureFlags= 1;
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load v3, this = %p Found meshHeader.properties = %d, textureFlags = %d, texName [%s] mtDiffuse = %d meshIndex = %d\n",this,meshHeader.properties,textureFlags,toLower(reinterpret_cast<char*>(meshHeader.texName)).c_str(),mtDiffuse,meshIndex);

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
					(*loadedFileList)[texPath]++;
				}

				texturesOwned[mtDiffuse]=true;
				textures[mtDiffuse]->init(textureManager->getTextureFilter(),textureManager->getMaxAnisotropy());
				if(deletePixMapAfterLoad == true) {
					textures[mtDiffuse]->deletePixels();
				}
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v3 model is missing texture [%s] meshHeader.properties = %d meshIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,texPath.c_str(),meshHeader.properties,meshIndex);
			}
		}
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(textures[mtDiffuse]!=NULL){
		for(unsigned int i=0; i<meshHeader.texCoordFrameCount; ++i){
			readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
		}
	}
	readBytes = fread(&diffuseColor, sizeof(Vec3f), 1, f);
	readBytes = fread(&opacity, sizeof(float32), 1, f);
	fseek(f, sizeof(Vec4f)*(meshHeader.colorFrameCount-1), SEEK_CUR);
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);
}

Texture2D* Mesh::loadMeshTexture(int meshIndex, int textureIndex,
		TextureManager *textureManager, string textureFile,
		int textureChannelCount, bool &textureOwned, bool deletePixMapAfterLoad,
		std::map<string,int> *loadedFileList) {

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] #1 load texture [%s]\n",__FUNCTION__,textureFile.c_str());

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
				(*loadedFileList)[textureFile]++;
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
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s] #3 cannot load texture [%s]\n",__FUNCTION__,textureFile.c_str());
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error v4 model is missing texture [%s] textureFlags = %d meshIndex = %d textureIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,textureFile.c_str(),textureFlags,meshIndex,textureIndex);
		}
	}

	return texture;
}

void Mesh::load(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
				bool deletePixMapAfterLoad,std::map<string,int> *loadedFileList) {
	this->textureManager = textureManager;

	//read header
	MeshHeader meshHeader;
	size_t readBytes = fread(&meshHeader, sizeof(MeshHeader), 1, f);

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
			string mapPath= toLower(reinterpret_cast<char*>(cMapPath));

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("mapPath [%s] meshHeader.textures = %d flag = %d (meshHeader.textures & flag) = %d meshIndex = %d i = %d\n",mapPath.c_str(),meshHeader.textures,flag,(meshHeader.textures & flag),meshIndex,i);

			string mapFullPath= dir;
			if(mapFullPath != "") {
                endPathWithSlash(mapFullPath);
			}
			mapFullPath += mapPath;

			textures[i] = loadMeshTexture(meshIndex, i, textureManager, mapFullPath,
					meshTextureChannelCount[i],texturesOwned[i],
					deletePixMapAfterLoad, loadedFileList);
		}
		flag *= 2;
	}

	//read data
	readBytes = fread(vertices, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	readBytes = fread(normals, sizeof(Vec3f)*frameCount*vertexCount, 1, f);
	if(meshHeader.textures!=0){
		readBytes = fread(texCoords, sizeof(Vec2f)*vertexCount, 1, f);
	}
	readBytes = fread(indices, sizeof(uint32)*indexCount, 1, f);

	//tangents
	if(textures[mtNormal]!=NULL){
		computeTangents();
	}
}

void Mesh::save(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
		string convertTextureToFormat, std::map<string,int> &textureDeleteList,
		bool keepsmallest) {
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
						throw runtime_error("Unsupported texture format: [" + convertTextureToFormat + "]");
					}

					//textureManager->endTexture(texture);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Save, load new texture [%s] originalSize [%ld] newSize [%ld]\n",file.c_str(),originalSize,newSize);

					if(keepsmallest == false || newSize <= originalSize) {
						texture = loadMeshTexture(meshIndex, i, textureManager,file,
													meshTextureChannelCount[i],
													texturesOwned[i],
													false);
					}
				}

				file = extractFileFromDirectoryPath(texture->getPath());

				if(file.length() > mapPathSize) {
					throw runtime_error("file.length() > mapPathSize, file.length() = " + intToStr(file.length()));
				}
				else if(file.length() == 0) {
					throw runtime_error("file.length() == 0");
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
	tangents= new Vec3f[vertexCount];
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
		std::map<string,int> *loadedFileList) {
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="g3d" || extension=="G3D"){
		loadG3d(path,deletePixMapAfterLoad,loadedFileList);
	}
	else{
		throw runtime_error("Unknown model format: " + extension);
	}

	this->fileName = path;
}

void Model::save(const string &path, string convertTextureToFormat,
		bool keepsmallest) {
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="g3d" ||extension=="G3D") {
		saveG3d(path,convertTextureToFormat,keepsmallest);
	}
	else {
		throw runtime_error("Unknown model format: " + extension);
	}
}

//load a model from a g3d file
void Model::loadG3d(const string &path, bool deletePixMapAfterLoad,
		std::map<string,int> *loadedFileList) {

    try{
		FILE *f=fopen(path.c_str(),"rb");
		if (f == NULL) {
		    printf("In [%s::%s] cannot load file = [%s]\n",__FILE__,__FUNCTION__,path.c_str());
			throw runtime_error("Error opening g3d model file [" + path + "]");
		}

		if(loadedFileList) {
			(*loadedFileList)[path]++;
		}

		string dir= extractDirectoryPathFromFile(path);

		//file header
		FileHeader fileHeader;
		size_t readBytes = fread(&fileHeader, sizeof(FileHeader), 1, f);
		if(strncmp(reinterpret_cast<char*>(fileHeader.id), "G3D", 3) != 0) {
		    printf("In [%s::%s] file = [%s] fileheader.id = [%s][%c]\n",__FILE__,__FUNCTION__,path.c_str(),reinterpret_cast<char*>(fileHeader.id),fileHeader.id[0]);
			throw runtime_error("Not a valid G3D model");
		}
		fileVersion= fileHeader.version;

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Load model, fileVersion = %d\n",fileVersion);

		//version 4
		if(fileHeader.version == 4) {
			//model header
			ModelHeader modelHeader;
			readBytes = fread(&modelHeader, sizeof(ModelHeader), 1, f);
			meshCount= modelHeader.meshCount;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			if(modelHeader.type != mtMorphMesh) {
				throw runtime_error("Invalid model type");
			}

			//load meshes
			meshes= new Mesh[meshCount];
			for(uint32 i = 0; i < meshCount; ++i) {
				meshes[i].load(i, dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList);
				meshes[i].buildInterpolationData();
			}
		}
		//version 3
		else if(fileHeader.version == 3) {
			readBytes = fread(&meshCount, sizeof(meshCount), 1, f);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			meshes= new Mesh[meshCount];
			for(uint32 i = 0; i < meshCount; ++i) {
				meshes[i].loadV3(i, dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList);
				meshes[i].buildInterpolationData();
			}
		}
		//version 2
		else if(fileHeader.version == 2) {
			readBytes = fread(&meshCount, sizeof(meshCount), 1, f);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("meshCount = %d\n",meshCount);

			meshes= new Mesh[meshCount];
			for(uint32 i = 0; i < meshCount; ++i){
				meshes[i].loadV2(i,dir, f, textureManager,deletePixMapAfterLoad,
						loadedFileList);
				meshes[i].buildInterpolationData();
			}
		}
		else {
			throw runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
		}

		fclose(f);
    }
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Exception caught loading 3d file: " + path +"\n"+ e.what());
	}
}

//save a model to a g3d file
void Model::saveG3d(const string &path, string convertTextureToFormat,
		bool keepsmallest) {
	string tempModelFilename = path + "cvt";
	FILE *f= fopen(tempModelFilename.c_str(), "wb");
	if(f == NULL) {
		throw runtime_error("Cant open file for writting: [" + tempModelFilename + "]");
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
					keepsmallest);
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
		throw runtime_error("Invalid model version: "+ intToStr(fileHeader.version));
	}

	fclose(f);
}

void Model::deletePixels() {
	for(uint32 i = 0; i < meshCount; ++i) {
		meshes[i].deletePixels();
	}
}

}}//end namespace
