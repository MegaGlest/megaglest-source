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

#ifndef _SHARED_GRAPHICS_MATRIX_H_
#define _SHARED_GRAPHICS_MATRIX_H_

#include <cmath>

#include "vec.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class Matrix3
// =====================================================

template<typename T>
class Matrix3{
private:
	T data[9];
public:
	Matrix3(){};

	Matrix3(T *p){
		for(int i=0; i<9; ++i){
			data[i]= p[i];
		}
	}

	T *ptr(){
		return data;
	}

	const T *ptr() const{
		return data;
	}

	T &operator[](int i){
		return data[i];
	}

	T &operator()(int i, int j){
		return data[i*3+j];
	}

	Vec3<T> operator * (const Vec3<T> &v) const{
		Vec3<T> rv;
		
		return Vec3f(
			data[0]*v.x + data[1]*v.y + data[2]*v.z,
			data[3]*v.x + data[4]*v.y + data[5]*v.z,
			data[6]*v.x + data[7]*v.y + data[8]*v.z);
	}

	Matrix3<T> operator * (const Matrix3<T> &m) const{
		Matrix3<T> rm;

		for(int i=0; i<3; ++i){
			for(int j=0; j<3; ++j){
				T acum= 0.0f;
				for(int k=0; k<3; ++k){  
					acum+= data[i*3+k]*m[k*3+j];	
				}
				rm[i*3+j]= acum;
			}
		}
		return rm;
	}

	void traspose(){
		for(int i=0; i<3; ++i){
			for(int j=0; j<3; ++j){
				T tmp= data[j*3+i];
				data[j*3+i]= data[i*3+j];
				data[i*3+j]= tmp;
			}
		}
	}
};

typedef Matrix3<float> Matrix3f;
typedef Matrix3<double> Matrix3d;

// =====================================================
//	class Matrix4
// =====================================================

template<typename T>
class Matrix4{
private:
	T data[16];
public:
	Matrix4(){};

	Matrix4(T *p){
		for(int i=0; i<16; ++i){
			data[i]= p[i];
		}
	}

	T *ptr(){
		return data;
	}

	const T *ptr() const{
		return data;
	}

	T &operator[](int i){
		return data[i];
	}

	const T &operator[](int i) const{
		return data[i];
	}

	T &operator()(int i, int j){
		return data[i*4+j];
	}

	Vec4<T> operator * (const Vec4<T> &v) const{
		Vec4<T> rv;
		
		return Vec4f(
			data[0]*v.x + data[1]*v.y + data[2]*v.z + data[3]*v.w,
			data[4]*v.x + data[5]*v.y + data[6]*v.z + data[7]*v.w,
			data[8]*v.x + data[9]*v.y + data[10]*v.z + data[11]*v.w,
			data[12]*v.x + data[13]*v.y + data[14]*v.z + data[15]*v.w);
	}

	Matrix4<T> operator * (const Matrix4<T> &m) const{
		Matrix4<T> rm;

		for(int i=0; i<4; ++i){
			for(int j=0; j<4; ++j){
				T acum= 0.0f;
				for(int k=0; k<4; ++k){  
					acum+= data[i*4+k]*m[k*4+j];	
				}
				rm[i*4+j]= acum;
			}
		}
		return rm;
	}

};

typedef Matrix4<float> Matrix4f;
typedef Matrix4<double> Matrix4d;


}} //enmd namespace

#endif
