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

#ifndef _SHARED_GRAPHICS_PIXMAP_H_
#define _SHARED_GRAPHICS_PIXMAP_H_

#include <string>
#include "vec.h"
#include "types.h"
#include <map>
#include "checksum.h"
#include "leak_dumper.h"

using std::string;
using Shared::Platform::int8;
using Shared::Platform::uint8;
using Shared::Platform::int16;
using Shared::Platform::uint16;
using Shared::Platform::int32;
using Shared::Platform::uint32;
using Shared::Platform::uint64;
using Shared::Platform::float32;
using Shared::Util::Checksum;

namespace Shared{ namespace Graphics{

/**
 * @brief Next power of 2
 * @param x The number to be rounded
 * @return the rounded number
 *
 * Rounds an unsigned integer up to the
 * next power of 2; i.e. 2, 4, 8, 16, etc.
 */
static inline unsigned int next_power_of_2(unsigned int x)
{
	x--;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return ++x;
}

/**
 * @brief Count bits set
 * @param w Number in which to count bits
 * @return The number of bits set
 *
 * Counts the number of bits in an unsigned int
 * that are set to 1.  So, for example, in the
 * number 5, hich is 101 in binary, there are
 * two bits set to 1.
 */
static inline unsigned int count_bits_set(unsigned int w)
{
	/*
	 * This is faster, and runs in parallel
	 */
	const int S[] = {1, 2, 4, 8, 16};
	const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};
	int c = w;
	c = ((c >> S[0]) & B[0]) + (c & B[0]);
	c = ((c >> S[1]) & B[1]) + (c & B[1]);
	c = ((c >> S[2]) & B[2]) + (c & B[2]);
	c = ((c >> S[3]) & B[3]) + (c & B[3]);
	c = ((c >> S[4]) & B[4]) + (c & B[4]);
	return c;
}

// =====================================================
//	class PixmapIo
// =====================================================

class PixmapIo {
protected:
	int w;
	int h;
	int components;

public:
	virtual ~PixmapIo(){}

	int getW() const			{return w;}
	int getH() const			{return h;}
	int getComponents() const	{return components;}

	virtual void openRead(const string &path)= 0;
	virtual void read(uint8 *pixels)= 0;
	virtual void read(uint8 *pixels, int components)= 0;

	virtual void openWrite(const string &path, int w, int h, int components)= 0;
	virtual void write(uint8 *pixels)= 0;
};

// =====================================================
//	class PixmapIoTga
// =====================================================

class PixmapIoTga: public PixmapIo {
private:
	FILE *file;

public:
	PixmapIoTga();
	virtual ~PixmapIoTga();

	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

// =====================================================
//	class PixmapIoBmp
// =====================================================

class PixmapIoBmp: public PixmapIo {
private:
	FILE *file;

public:
	PixmapIoBmp();
	virtual ~PixmapIoBmp();
	
	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

// =====================================================
//	class PixmapIoBmp
// =====================================================

class PixmapIoPng: public PixmapIo {
private:
	FILE *file;
	string path;

public:
	PixmapIoPng();
	virtual ~PixmapIoPng();

	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

class PixmapIoJpg: public PixmapIo {
private:
	FILE *file;
	string path;

public:
	PixmapIoJpg();
	virtual ~PixmapIoJpg();

	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

// =====================================================
//	class Pixmap1D
// =====================================================

class Pixmap1D {
protected:
	int w;
	int components;
	uint8 *pixels;
	string path;
	Checksum crc;

public:
	//constructor & destructor
	Pixmap1D();
	Pixmap1D(int components);
	Pixmap1D(int w, int components);
	void init(int components);
	void init(int w, int components);
	~Pixmap1D();
	
	//load & save
	void load(const string &path);
	void loadTga(const string &path);
	void loadBmp(const string &path);

	//get 
	int getW() const			{return w;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}
	void deletePixels();
	string getPath() const		{ return path;}
	uint64 getPixelByteCount() const;

	Checksum * getCRC() { return &crc; }
};

// =====================================================
//	class Pixmap2D
// =====================================================

class Pixmap2D {
protected:
	int h;
	int w;
	int components;
	uint8 *pixels;
	string path;
	Checksum crc;

public:
	//constructor & destructor
	Pixmap2D();
	Pixmap2D(int components);
	Pixmap2D(int w, int h, int components);
	void init(int components);
	void init(int w, int h, int components);
	~Pixmap2D();
	
	void Scale(int format, int newW, int newH);

	//load & save
	static Pixmap2D* loadPath(const string& path);
	void load(const string &path);
	/*void loadTga(const string &path);
	void loadBmp(const string &path);*/
	void save(const string &path);
	void saveBmp(const string &path);
	void saveTga(const string &path);
	void savePng(const string &path);
	void saveJpg(const string &path);

	//get 
	int getW() const			{return w;}
	int getH() const			{return h;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}
	void deletePixels();
		
	//get data
	void getPixel(int x, int y, uint8 *value) const;
	void getPixel(int x, int y, float32 *value) const;
	void getComponent(int x, int y, int component, uint8 &value) const;
	void getComponent(int x, int y, int component, float32 &value) const;

	//vector get
	Vec4f getPixel4f(int x, int y) const;
	Vec3f getPixel3f(int x, int y) const;
	float getPixelf(int x, int y) const;
	float getComponentf(int x, int y, int component) const;

	//set data
	void setPixel(int x, int y, const uint8 *value);
	void setPixel(int x, int y, const float32 *value);
	void setComponent(int x, int y, int component, uint8 value);
	void setComponent(int x, int y, int component, float32 value);

	//vector set
	void setPixel(int x, int y, const Vec3f &p);
	void setPixel(int x, int y, const Vec4f &p);
	void setPixel(int x, int y, float p);
	
	//mass set
	void setPixels(const uint8 *value);
	void setPixels(const float32 *value);
	void setComponents(int component, uint8 value);
	void setComponents(int component, float32 value);

	//operations
	void splat(const Pixmap2D *leftUp, const Pixmap2D *rightUp, const Pixmap2D *leftDown, const Pixmap2D *rightDown); 
	void lerp(float t, const Pixmap2D *pixmap1, const Pixmap2D *pixmap2);
	void copy(const Pixmap2D *sourcePixmap);
	void subCopy(int x, int y, const Pixmap2D *sourcePixmap);
	string getPath() const		{ return path;}
	uint64 getPixelByteCount() const;

	Checksum * getCRC() { return &crc; }

private:
	bool doDimensionsAgree(const Pixmap2D *pixmap);
};

// =====================================================
//	class Pixmap3D
// =====================================================

class Pixmap3D {
protected:
	int h;
	int w;
	int d;
	int components;
	int slice;
	uint8 *pixels;
	string path;
	Checksum crc;

public:
	//constructor & destructor
	Pixmap3D();
	Pixmap3D(int w, int h, int d, int components);
	Pixmap3D(int d, int components);
	void init(int w, int h, int d, int components);
	void init(int d, int components);
	void init(int components);
	~Pixmap3D();
	
	//load & save
	void loadSlice(const string &path, int slice);
	void loadSliceBmp(const string &path, int slice);
	void loadSliceTga(const string &path, int slice);
	void loadSlicePng(const string &path, int slice);
	
	//get
	int getSlice() const { return slice; }
	int getW() const			{return w;}
	int getH() const			{return h;}
	int getD() const			{return d;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}
	void deletePixels();
	string getPath() const		{ return path;}
	uint64 getPixelByteCount() const;

	Checksum * getCRC() { return &crc; }
};

// =====================================================
//	class PixmapCube
// =====================================================

class PixmapCube {
public:
	enum Face{
		fPositiveX,
		fNegativeX,
		fPositiveY,
		fNegativeY,
		fPositiveZ,
		fNegativeZ
	};

protected:
	Pixmap2D faces[6];
	string path[6];
	Checksum crc;

public:
	PixmapCube();
	~PixmapCube();

	//init
	void init(int w, int h, int components);
	void init(int components);

	//load & save
	void loadFace(const string &path, int face);
	/*void loadFaceBmp(const string &path, int face);
	void loadFaceTga(const string &path, int face);*/
	
	//get 
	Pixmap2D *getFace(int face)				{return &faces[face];}
	const Pixmap2D *getFace(int face) const	{return &faces[face];}
	void deletePixels();
	string getPath(int face) const		{ return path[face];}
	uint64 getPixelByteCount() const;

	Checksum * getCRC() { return &crc; }
};

}}//end namespace

#endif
