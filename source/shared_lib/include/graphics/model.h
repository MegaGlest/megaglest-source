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

#ifndef _SHARED_GRAPHICS_MODEL_H_
#define _SHARED_GRAPHICS_MODEL_H_

#include <string>
#include <map>
#include "data_types.h"
#include "pixmap.h"
#include "texture_manager.h"
#include "texture.h"
#include "model_header.h"
#include <memory>
#include "leak_dumper.h"

using std::string;
using std::map;
using std::pair;

namespace Shared { namespace Graphics {

class Model;
class Mesh;
class ShadowVolumeData;
class InterpolationData;
class TextureManager;

// =====================================================
//	class Mesh
//
//	Part of a 3D model
// =====================================================

class Mesh {
private:
	//mesh data
	Texture2D *textures[meshTextureCount];
	bool texturesOwned[meshTextureCount];
	string texturePaths[meshTextureCount];

	string name;
	//vertex data counts
	uint32 frameCount;
	uint32 vertexCount;
	uint32 indexCount;
	uint32 texCoordFrameCount;

	//vertex data
	Vec3f *vertices;
	Vec3f *normals;
	Vec2f *texCoords;
	Vec3f *tangents;
	uint32 *indices;

	//material data
	Vec3f diffuseColor;
	Vec3f specularColor;
	float specularPower;
	float opacity;

	//properties
	bool twoSided;
	bool customColor;
	bool noSelect;

	uint32 textureFlags;

	InterpolationData *interpolationData;
	TextureManager *textureManager;

	// Vertex Buffer Object Names
	bool    hasBuiltVBOs;
	uint32	m_nVBOVertices;					// Vertex VBO Name
	uint32	m_nVBOTexCoords;				// Texture Coordinate VBO Name
	uint32	m_nVBONormals;					// Normal VBO Name
	uint32	m_nVBOIndexes;					// Indexes VBO Name

public:
	//init & end
	Mesh();
	~Mesh();
	void init();
	void end();

	//maps
	const Texture2D *getTexture(int i) const	{return textures[i];}

	//counts
	uint32 getFrameCount() const			{return frameCount;}
	uint32 getVertexCount() const			{return vertexCount;}
	uint32 getIndexCount() const			{return indexCount;}
	uint32 getTriangleCount() const;

	uint32	getVBOVertices() const  { return m_nVBOVertices;}
	uint32	getVBOTexCoords() const { return m_nVBOTexCoords;}
	uint32	getVBONormals() const   { return m_nVBONormals;}
	uint32	getVBOIndexes() const   { return m_nVBOIndexes;}
	bool    hasBuiltVBOEntities() const { return hasBuiltVBOs;}
	void BuildVBOs();
	void ReleaseVBOs();

	//data
	const Vec3f *getVertices() const 	{return vertices;}
	const Vec3f *getNormals() const 	{return normals;}
	const Vec2f *getTexCoords() const	{return texCoords;}
	const Vec3f *getTangents() const	{return tangents;}
	const uint32 *getIndices() const 	{return indices;}

	//material
	const Vec3f &getDiffuseColor() const	{return diffuseColor;}
	const Vec3f &getSpecularColor() const	{return specularColor;}
	float getSpecularPower() const			{return specularPower;}
	float getOpacity() const				{return opacity;}

	//properties
	bool getTwoSided() const		{return twoSided;}
	bool getCustomTexture() const	{return customColor;}
	bool getNoSelect() const		{return noSelect;}

	//external data
	const InterpolationData *getInterpolationData() const	{return interpolationData;}

	//interpolation
	void buildInterpolationData();
	void updateInterpolationData(float t, bool cycle);
	void updateInterpolationVertices(float t, bool cycle);

	Texture2D *loadMeshTexture(int meshIndex, int textureIndex, TextureManager *textureManager, string textureFile,
								int textureChannelCount, bool &textureOwned,
								bool deletePixMapAfterLoad, std::map<string,vector<pair<string, string> > > *loadedFileList=NULL,
								string sourceLoader="");

	//load
	void loadV2(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
			bool deletePixMapAfterLoad,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL,string sourceLoader="");
	void loadV3(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
			bool deletePixMapAfterLoad,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL,string sourceLoader="");
	void load(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,bool deletePixMapAfterLoad,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL,string sourceLoader="");
	void save(int meshIndex, const string &dir, FILE *f, TextureManager *textureManager,
			string convertTextureToFormat, std::map<string,int> &textureDeleteList,
			bool keepsmallest);

	void deletePixels();

private:
	string findAlternateTexture(vector<string> conversionList, string textureFile);
	void computeTangents();
};

// =====================================================
//	class Model
//
//	3D Model, than can be loaded from a g3d file
// =====================================================

class Model {
private:
	TextureManager *textureManager;

private:
	uint8 fileVersion;
	uint32 meshCount;
	Mesh *meshes;

	float lastTData;
	bool lastCycleData;
	float lastTVertex;
	bool lastCycleVertex;

	string fileName;
	string sourceLoader;

	//static bool masterserverMode;

public:
	//constructor & destructor
	Model();
	virtual ~Model();
	virtual void init()= 0;
	virtual void end()= 0;

	//static void setMasterserverMode(bool value) { masterserverMode=value; }

	//data
	void updateInterpolationData(float t, bool cycle);
	void updateInterpolationVertices(float t, bool cycle);
	void buildShadowVolumeData() const;

	//get
	uint8 getFileVersion() const		{return fileVersion;}
	uint32 getMeshCount() const			{return meshCount;}
	const Mesh *getMesh(int i) const	{return &meshes[i];}
	Mesh *getMeshPtr(int i) const	{return &meshes[i];}

	uint32 getTriangleCount() const;
	uint32 getVertexCount() const;

	//io
	void load(const string &path,bool deletePixMapAfterLoad=false,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL, string *sourceLoader=NULL);
	void save(const string &path, string convertTextureToFormat,bool keepsmallest);
	void loadG3d(const string &path,bool deletePixMapAfterLoad=false,std::map<string,vector<pair<string, string> > > *loadedFileList=NULL, string sourceLoader="");
	void saveG3d(const string &path, string convertTextureToFormat,bool keepsmallest);

	void setTextureManager(TextureManager *textureManager)	{this->textureManager= textureManager;}
	void deletePixels();

	string getFileName() const { return fileName; }

private:
	void buildInterpolationData() const;
};

class PixelBufferWrapper {
public:
	PixelBufferWrapper(int pboCount,int bufferSize);
	~PixelBufferWrapper();

	static Pixmap2D *getPixelBufferFor(int x,int y,int w,int h, int colorComponents);
	static void begin();
	static void end();
	static bool getIsPBOEnable() { return isPBOEnabled; }

private:
	static bool isPBOEnabled;
	static int index;
	static vector<uint32> pboIds;

	void cleanup();
};

class BaseColorPickEntity {

public:
    BaseColorPickEntity();
    virtual ~BaseColorPickEntity() {}

    static const int COLOR_COMPONENTS = 4;
    static void init(int bufferSize);
    static void beginPicking();
    static void endPicking();
    static vector<int> getPickedList(int x,int y,int w,int h, const vector<BaseColorPickEntity *> &rendererModels);

    void setUniquePickingColor() const;
    bool isUniquePickingColor(unsigned char *pixel) const;

    string getColorDescription() const;
    virtual string getUniquePickName() const = 0;

private:
	unsigned char uniqueColorID[COLOR_COMPONENTS];

    static unsigned char nextColorID[COLOR_COMPONENTS];
    static Mutex mutexNextColorID;

    static auto_ptr<PixelBufferWrapper> pbo;
};


}}//end namespace

#endif
