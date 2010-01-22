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

#include "pixmap.h"

#include <stdexcept>
#include <cstdio>
#include <cassert>

#include "util.h"
#include "math_util.h"
#include "random.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Graphics{

using namespace Util;

// =====================================================
//	file structs
// =====================================================

#pragma pack(push, 1)

struct BitmapFileHeader{ 
	uint8 type1; 
	uint8 type2; 
	uint32 size; 
	uint16 reserved1; 
	uint16 reserved2; 
	uint32 offsetBits; 
}; 

struct BitmapInfoHeader{
	uint32 size; 
	int32 width; 
	int32 height; 
	uint16 planes; 
	uint16 bitCount; 
	uint32 compression; 
	uint32 sizeImage; 
	int32 xPelsPerMeter; 
	int32 yPelsPerMeter; 
	uint32 clrUsed; 
	uint32 clrImportant; 
};

struct TargaFileHeader{
	int8 idLength;
	int8 colourMapType;
	int8 dataTypeCode;
	int16 colourMapOrigin;
	int16 colourMapLength;
	int8 colourMapDepth;
	int16 xOrigin;
	int16 yOrigin;
	int16 width;
	int16 height;
	int8 bitsPerPixel;
	int8 imageDescriptor;
};

#pragma pack(pop)

const int tgaUncompressedRgb= 2;
const int tgaUncompressedBw= 3;

// =====================================================
//	class PixmapIoTga
// =====================================================

PixmapIoTga::PixmapIoTga(){
	file= NULL;
}

PixmapIoTga::~PixmapIoTga(){
	if(file!=NULL){
		fclose(file);
	}
}

void PixmapIoTga::openRead(const string &path){
	file= fopen(path.c_str(),"rb"); 
	if (file==NULL){
		throw runtime_error("Can't open TGA file: "+ path);
	}
	
	//read header
	TargaFileHeader fileHeader;
	fread(&fileHeader, sizeof(TargaFileHeader), 1, file);
    
	//check that we can load this tga file
	if(fileHeader.idLength!=0){
		throw runtime_error(path + ": id field is not 0");
	}

	if(fileHeader.dataTypeCode!=tgaUncompressedRgb && fileHeader.dataTypeCode!=tgaUncompressedBw){
		throw runtime_error(path + ": only uncompressed BW and RGB targa images are supported"); 
	}

	//check bits per pixel
	if(fileHeader.bitsPerPixel!=8 && fileHeader.bitsPerPixel!=24 && fileHeader.bitsPerPixel!=32){
		throw runtime_error(path + ": only 8, 24 and 32 bit targa images are supported");
	}

	h= fileHeader.height;
    w= fileHeader.width;
	components= fileHeader.bitsPerPixel/8;
}

void PixmapIoTga::read(uint8 *pixels){
	read(pixels, components);
}

void PixmapIoTga::read(uint8 *pixels, int components){
	for(int i=0; i<h*w*components; i+=components){
		uint8 r, g, b, a, l;

		if(this->components==1){
			fread(&l, 1, 1, file);
			r= l;
			g= l;
			b= l;
			a= 255;
		}
		else{
			fread(&b, 1, 1, file);
			fread(&g, 1, 1, file);
			fread(&r, 1, 1, file);
			if(this->components==4){
				fread(&a, 1, 1, file);
			}
			else{
				a= 255;
			}
			l= (r+g+b)/3;
		}

		switch(components){
		case 1:
			pixels[i]= l;
			break;
		case 3:
			pixels[i]= r;
			pixels[i+1]= g;
			pixels[i+2]= b;
			break;
		case 4:
			pixels[i]= r;
			pixels[i+1]= g;
			pixels[i+2]= b;
			pixels[i+3]= a;
			break;
		}
	}
}

void PixmapIoTga::openWrite(const string &path, int w, int h, int components){
    this->w= w;
	this->h= h;
	this->components= components;

    file= fopen(path.c_str(),"wb"); 
	if (file==NULL){
		throw runtime_error("Can't open TGA file: "+ path);
	}

	TargaFileHeader fileHeader;
	memset(&fileHeader, 0, sizeof(TargaFileHeader));
	fileHeader.dataTypeCode= components==1? tgaUncompressedBw: tgaUncompressedRgb;
	fileHeader.bitsPerPixel= components*8;
	fileHeader.width= w;
	fileHeader.height= h;
	fileHeader.imageDescriptor= components==4? 8: 0;

	fwrite(&fileHeader, sizeof(TargaFileHeader), 1, file);
}

void PixmapIoTga::write(uint8 *pixels){
	if(components==1){
		fwrite(pixels, h*w, 1, file);	
	}
	else{
		for(int i=0; i<h*w*components; i+=components){
			fwrite(&pixels[i+2], 1, 1, file);
			fwrite(&pixels[i+1], 1, 1, file);
			fwrite(&pixels[i], 1, 1, file);
			if(components==4){
				fwrite(&pixels[i+3], 1, 1, file);
			}
		}
	}
}

// =====================================================
//	class PixmapIoBmp
// =====================================================

PixmapIoBmp::PixmapIoBmp(){
	file= NULL;
}

PixmapIoBmp::~PixmapIoBmp(){
	if(file!=NULL){
		fclose(file);
	}
}

void PixmapIoBmp::openRead(const string &path){
    file= fopen(path.c_str(),"rb"); 
	if (file==NULL){
		throw runtime_error("Can't open BMP file: "+ path);
	}

	//read file header
    BitmapFileHeader fileHeader;
    fread(&fileHeader, sizeof(BitmapFileHeader), 1, file);
	if(fileHeader.type1!='B' || fileHeader.type2!='M'){ 
		throw runtime_error(path +" is not a bitmap");
	}
    
	//read info header
	BitmapInfoHeader infoHeader;
	fread(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
	if(infoHeader.bitCount!=24){
        throw runtime_error(path+" is not a 24 bit bitmap");
	}

    h= infoHeader.height;
    w= infoHeader.width;
	components= 3;
}

void PixmapIoBmp::read(uint8 *pixels){
	read(pixels, 3);
}

void PixmapIoBmp::read(uint8 *pixels, int components){
    for(int i=0; i<h*w*components; i+=components){
		uint8 r, g, b;
		fread(&b, 1, 1, file);
		fread(&g, 1, 1, file);
		fread(&r, 1, 1, file);

		switch(components){
		case 1:
			pixels[i]= (r+g+b)/3;
			break;
		case 3:
			pixels[i]= r;
			pixels[i+1]= g;
			pixels[i+2]= b;
			break;
		case 4:
			pixels[i]= r;
			pixels[i+1]= g;
			pixels[i+2]= b;
			pixels[i+3]= 255;
			break;
		}
    }
}

void PixmapIoBmp::openWrite(const string &path, int w, int h, int components){
    this->w= w;
	this->h= h;
	this->components= components;
	
	file= fopen(path.c_str(),"wb"); 
	if (file==NULL){
		throw runtime_error("Can't open BMP file for writting: "+ path);
	}

	BitmapFileHeader fileHeader;
    fileHeader.type1='B';
	fileHeader.type2='M';
	fileHeader.offsetBits=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader);
	fileHeader.size=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader)+3*h*w;

    fwrite(&fileHeader, sizeof(BitmapFileHeader), 1, file);
    
	//info header
	BitmapInfoHeader infoHeader;
	infoHeader.bitCount=24;
	infoHeader.clrImportant=0;
	infoHeader.clrUsed=0;
	infoHeader.compression=0;
	infoHeader.height= h;
	infoHeader.planes=1;
	infoHeader.size=sizeof(BitmapInfoHeader);
	infoHeader.sizeImage=0;
	infoHeader.width= w;
	infoHeader.xPelsPerMeter= 0;
	infoHeader.yPelsPerMeter= 0;

	fwrite(&infoHeader, sizeof(BitmapInfoHeader), 1, file);
}

void PixmapIoBmp::write(uint8 *pixels){
    for (int i=0; i<h*w*components; i+=components){
        fwrite(&pixels[i+2], 1, 1, file);
		fwrite(&pixels[i+1], 1, 1, file);
		fwrite(&pixels[i], 1, 1, file);
    }
}

// =====================================================
//	class Pixmap1D
// =====================================================


// ===================== PUBLIC ========================

Pixmap1D::Pixmap1D(){
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap1D::Pixmap1D(int components){
	init(components);
}

Pixmap1D::Pixmap1D(int w, int components){
	init(w, components);
}

void Pixmap1D::init(int components){
	this->w= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap1D::init(int w, int components){
	this->w= w;
	this->components= components;
	pixels= new uint8[w*components];
}

Pixmap1D::~Pixmap1D(){
	delete [] pixels;
}

void Pixmap1D::load(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		loadBmp(path);
	}
	else if(extension=="tga"){
		loadTga(path);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap1D::loadBmp(const string &path){
	
	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	if(plb.getH()==1){
		w= plb.getW();
	}
	else if(plb.getW()==1){
		w= plb.getH();
	}
	else{
		throw runtime_error("One of the texture dimensions must be 1");
	}

	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[w*components]; 
	}

	//data
	plb.read(pixels, components);
}

void Pixmap1D::loadTga(const string &path){
	
	PixmapIoTga plt;
	plt.openRead(path);
	
	//init
	if(plt.getH()==1){
		w= plt.getW();
	}
	else if(plt.getW()==1){
		w= plt.getH();
	}
	else{
		throw runtime_error("One of the texture dimensions must be 1");
	}

	int fileComponents= plt.getComponents();

	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[w*components]; 
	}

	//read data
	plt.read(pixels, components);
}

// =====================================================
//	class Pixmap2D
// =====================================================

// ===================== PUBLIC ========================

Pixmap2D::Pixmap2D(){
    h= -1;
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap2D::Pixmap2D(int components){
	init(components);
}

Pixmap2D::Pixmap2D(int w, int h, int components){
	init(w, h, components);
}

void Pixmap2D::init(int components){
	this->w= -1;
	this->h= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap2D::init(int w, int h, int components){
	this->w= w;
	this->h= h;
	this->components= components;
	pixels= new uint8[h*w*components];
}

Pixmap2D::~Pixmap2D(){
	delete [] pixels;
}

void Pixmap2D::load(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		loadBmp(path);
	}
	else if(extension=="tga"){
		loadTga(path);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap2D::loadBmp(const string &path){
	
	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	w= plb.getW();
	h= plb.getH();
	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*components]; 
	}

	//data
	plb.read(pixels, components);
}

void Pixmap2D::loadTga(const string &path){
	
	PixmapIoTga plt;
	plt.openRead(path);
	w= plt.getW();
	h= plt.getH();

	//header
	int fileComponents= plt.getComponents();

	//init
	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*components]; 
	}

	//read data
	plt.read(pixels, components);
}

void Pixmap2D::save(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		saveBmp(path);
	}
	else if(extension=="tga"){
		saveTga(path);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap2D::saveBmp(const string &path){
	PixmapIoBmp psb;
	psb.openWrite(path, w, h, components);
	psb.write(pixels);
}

void Pixmap2D::saveTga(const string &path){
	PixmapIoTga pst;
	pst.openWrite(path, w, h, components);
	pst.write(pixels);
}

void Pixmap2D::getPixel(int x, int y, uint8 *value) const{
	for(int i=0; i<components; ++i){
		value[i]= pixels[(w*y+x)*components+i];
	}
}

void Pixmap2D::getPixel(int x, int y, float32 *value) const{
	for(int i=0; i<components; ++i){
		value[i]= pixels[(w*y+x)*components+i]/255.f;
	}
}

void Pixmap2D::getComponent(int x, int y, int component, uint8 &value) const{
	value= pixels[(w*y+x)*components+component];
}

void Pixmap2D::getComponent(int x, int y, int component, float32 &value) const{
	value= pixels[(w*y+x)*components+component]/255.f;
}

//vector get
Vec4f Pixmap2D::getPixel4f(int x, int y) const{
	Vec4f v(0.f);
	for(int i=0; i<components && i<4; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

Vec3f Pixmap2D::getPixel3f(int x, int y) const{
	Vec3f v(0.f);
	for(int i=0; i<components && i<3; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

float Pixmap2D::getPixelf(int x, int y) const{
	return pixels[(w*y+x)*components]/255.f;
}

float Pixmap2D::getComponentf(int x, int y, int component) const{
	float c;
	getComponent(x, y, component, c);
	return c;
}

void Pixmap2D::setPixel(int x, int y, const uint8 *value){
	for(int i=0; i<components; ++i){
		pixels[(w*y+x)*components+i]= value[i];
	}
}

void Pixmap2D::setPixel(int x, int y, const float32 *value){
	for(int i=0; i<components; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(value[i]*255.f);
	}
}

void Pixmap2D::setComponent(int x, int y, int component, uint8 value){
	pixels[(w*y+x)*components+component]= value;
}

void Pixmap2D::setComponent(int x, int y, int component, float32 value){
	pixels[(w*y+x)*components+component]= static_cast<uint8>(value*255.f);
}

//vector set
void Pixmap2D::setPixel(int x, int y, const Vec3f &p){
	for(int i=0; i<components  && i<3; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}

void Pixmap2D::setPixel(int x, int y, const Vec4f &p){
	for(int i=0; i<components && i<4; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}

void Pixmap2D::setPixel(int x, int y, float p){
	pixels[(w*y+x)*components]= static_cast<uint8>(p*255.f);
}

void Pixmap2D::setPixels(const uint8 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}
	
void Pixmap2D::setPixels(const float32 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}

void Pixmap2D::setComponents(int component, uint8 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
}

void Pixmap2D::setComponents(int component, float32 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
}

float splatDist(Vec2i a, Vec2i b){
	return (max(abs(a.x-b.x),abs(a.y- b.y)) + 3.f*a.dist(b))/4.f;
}

void Pixmap2D::splat(const Pixmap2D *leftUp, const Pixmap2D *rightUp, const Pixmap2D *leftDown, const Pixmap2D *rightDown){

	Random random;

	assert(components==3 || components==4);

	if(
		!doDimensionsAgree(leftUp) ||
		!doDimensionsAgree(rightUp) ||
		!doDimensionsAgree(leftDown) ||
		!doDimensionsAgree(rightDown))
	{
		throw runtime_error("Pixmap2D::splat: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			
			float avg= (w+h)/2.f;

			float distLu= splatDist(Vec2i(i, j), Vec2i(0, 0));
			float distRu= splatDist(Vec2i(i, j), Vec2i(w, 0));
			float distLd= splatDist(Vec2i(i, j), Vec2i(0, h));
			float distRd= splatDist(Vec2i(i, j), Vec2i(w, h));
			
			const float powFactor= 2.0f;
			distLu= pow(distLu, powFactor);
			distRu= pow(distRu, powFactor);
			distLd= pow(distLd, powFactor);
			distRd= pow(distRd, powFactor);
			avg= pow(avg, powFactor);

			float lu= distLu>avg? 0: ((avg-distLu))*random.randRange(0.5f, 1.0f);
			float ru= distRu>avg? 0: ((avg-distRu))*random.randRange(0.5f, 1.0f);
			float ld= distLd>avg? 0: ((avg-distLd))*random.randRange(0.5f, 1.0f);
			float rd= distRd>avg? 0: ((avg-distRd))*random.randRange(0.5f, 1.0f);
			
			float total= lu+ru+ld+rd;

			Vec4f pix= (leftUp->getPixel4f(i, j)*lu+
				rightUp->getPixel4f(i, j)*ru+
				leftDown->getPixel4f(i, j)*ld+
				rightDown->getPixel4f(i, j)*rd)*(1.0f/total);

			setPixel(i, j, pix);				
		}	
	}
}

void Pixmap2D::lerp(float t, const Pixmap2D *pixmap1, const Pixmap2D *pixmap2){
	if(
		!doDimensionsAgree(pixmap1) ||
		!doDimensionsAgree(pixmap2))
	{
		throw runtime_error("Pixmap2D::lerp: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, pixmap1->getPixel4f(i, j).lerp(t, pixmap2->getPixel4f(i, j)));
		}
	}
}

void Pixmap2D::copy(const Pixmap2D *sourcePixmap){
	
	assert(components==sourcePixmap->getComponents());

	if(w!=sourcePixmap->getW() || h!=sourcePixmap->getH()){
		throw runtime_error("Pixmap2D::copy() dimensions must agree");
	}
	memcpy(pixels, sourcePixmap->getPixels(), w*h*sourcePixmap->getComponents());
}

void Pixmap2D::subCopy(int x, int y, const Pixmap2D *sourcePixmap){
	assert(components==sourcePixmap->getComponents());

	if(w<sourcePixmap->getW() && h<sourcePixmap->getH()){
		throw runtime_error("Pixmap2D::subCopy(), bad dimensions");
	}

	uint8 *pixel= new uint8[components];

	for(int i=0; i<sourcePixmap->getW(); ++i){
		for(int j=0; j<sourcePixmap->getH(); ++j){
			sourcePixmap->getPixel(i, j, pixel);
			setPixel(i+x, j+y, pixel);
		}
	}

	delete pixel;
}

bool Pixmap2D::doDimensionsAgree(const Pixmap2D *pixmap){
	return pixmap->getW() == w && pixmap->getH() == h;
}

// =====================================================
//	class Pixmap3D
// =====================================================

Pixmap3D::Pixmap3D(){
	w= -1;
	h= -1;
	d= -1;
	components= -1;
}

Pixmap3D::Pixmap3D(int w, int h, int d, int components){
	init(w, h, d, components);
}

Pixmap3D::Pixmap3D(int d, int components){
	init(d, components);
}

void Pixmap3D::init(int w, int h, int d, int components){
	this->w= w;
	this->h= h;
	this->d= d;
	this->components= components;
	pixels= new uint8[h*w*d*components];;
}

void Pixmap3D::init(int d, int components){
	this->w= -1;
	this->h= -1;
	this->d= d;
	this->components= components;
	pixels= NULL;
}

Pixmap3D::~Pixmap3D(){
	delete [] pixels;
}

void Pixmap3D::loadSlice(const string &path, int slice){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		loadSliceBmp(path, slice);
	}
	else if(extension=="tga"){
		loadSliceTga(path, slice);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap3D::loadSliceBmp(const string &path, int slice){

	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	w= plb.getW();
	h= plb.getH();
	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*d*components]; 
	}

	//data
	plb.read(&pixels[slice*w*h*components], components);
}

void Pixmap3D::loadSliceTga(const string &path, int slice){
	PixmapIoTga plt;
	plt.openRead(path);

	//header
	int fileComponents= plt.getComponents();

	//init
	w= plt.getW();
	h= plt.getH();
	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*d*components]; 
	}

	//read data
	plt.read(&pixels[slice*w*h*components], components);
}

// =====================================================
//	class PixmapCube
// =====================================================

void PixmapCube::init(int w, int h, int components){
	for(int i=0; i<6; ++i){
		faces[i].init(w, h, components);
	}
}
	
	//load & save
void PixmapCube::loadFace(const string &path, int face){
	faces[face].load(path);
}

void PixmapCube::loadFaceBmp(const string &path, int face){
	faces[face].loadBmp(path);
}

void PixmapCube::loadFaceTga(const string &path, int face){
	faces[face].loadTga(path);
}

}}//end namespace
