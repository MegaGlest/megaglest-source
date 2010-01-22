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


#ifndef _SHARED_GRAPHICS_VEC_H_
#define _SHARED_GRAPHICS_VEC_H_

#include <cmath>

namespace Shared{ namespace Graphics{

template<typename T> class Vec2;
template<typename T> class Vec3;
template<typename T> class Vec4;

// =====================================================
//	class Vec2
// =====================================================

template<typename T>
class Vec2{
public:
	T x;
	T y;
public:
	Vec2(){
	};

	explicit Vec2(T *p){
		this->x= p[0];
		this->y= p[1];
	}

	explicit Vec2(T xy){
		this->x= xy;
		this->y= xy;
	}

	template<typename S>
	explicit Vec2(const Vec2<S> &v){
		this->x= v.x;
		this->y= v.y;
	}

	Vec2(T x, T y){
		this->x= x;
		this->y= y;
	}

	T *ptr(){
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const{
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec2<T> &v) const{
		return x==v.x && y==v.y;
	}

	bool operator !=(const Vec2<T> &v) const{
		return x!=v.x || y!=v.y;
	}

	Vec2<T> operator +(const Vec2<T> &v) const{
		return Vec2(x+v.x, y+v.y);
	}

	Vec2<T> operator -(const Vec2<T> &v) const{
		return Vec2(x-v.x, y-v.y);
	}

	Vec2<T> operator -() const{
		return Vec2(-x, -y);
	}

	Vec2<T> operator *(const Vec2<T> &v) const{
		return Vec2(x*v.x, y*v.y);
	}

	Vec2<T> operator *(T s) const{
		return Vec2(x*s, y*s);
	}

	Vec2<T> operator /(const Vec2<T> &v) const{
		return Vec2(x/v.x, y/v.y);
	}

	Vec2<T> operator /(T s) const{
		return Vec2(x/s, y/s);
	}

	Vec2<T> operator +=(const Vec2<T> &v){
		x+=v.x; 
		y+=v.y;
		return *this;
	}

	Vec2<T> operator -=(const Vec2<T> &v){
		x-=v.x; 
		y-=v.y;
		return *this;
	}

	Vec2<T> lerp(T t, const Vec2<T> &v) const{
		return *this + (v - *this)*t;
	}

	T dot(const Vec2<T> &v) const{
		return x*v.x+y*v.y;
	}

	float dist(const Vec2<T> &v) const{
		return Vec2<T>(v-*this).length();
	}

	float length() const{
		return static_cast<float>(sqrt(static_cast<float>(x*x + y*y)));
	}

	void normalize(){
		T m= length(); 
		x/= m;
		y/= m;
	}
};

typedef Vec2<int> Vec2i;
typedef Vec2<bool> Vec2b;
typedef Vec2<char> Vec2c;
typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;

// =====================================================
//	class Vec3
// =====================================================

template<typename T>
class Vec3{
public:
	T x;
	T y;
	T z;

public:
	Vec3(){
	};

	explicit Vec3(T *p){
		this->x= p[0];
		this->y= p[1];
		this->z= p[2];
	}

	explicit Vec3(T xyz){
		this->x= xyz;
		this->y= xyz;
		this->z= xyz;
	}

	template<typename S>
	explicit Vec3(const Vec3<S> &v){
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
	}

	Vec3(T x, T y, T z){
		this->x= x;
		this->y= y;
		this->z= z;
	}

	explicit Vec3(Vec4<T> v){
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
	}

	T *ptr(){
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const{
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec3<T> &v) const{
		return x==v.x && y==v.y && z==v.z;
	}

	bool operator !=(const Vec3<T> &v) const{
		return x!=v.x || y!=v.y || z!=v.z;
	}

	Vec3<T> operator +(const Vec3<T> &v) const{
		return Vec3(x+v.x, y+v.y, z+v.z);
	}

	Vec3<T> operator -(const Vec3<T> &v) const{
		return Vec3(x-v.x, y-v.y, z-v.z);
	}

	Vec3<T> operator -() const{
		return Vec3(-x, -y, -z);
	}

	Vec3<T> operator *(const Vec3<T> &v) const{
		return Vec3(x*v.x, y*v.y, z*v.z);
	}

	Vec3<T> operator *(T s) const{
		return Vec3(x*s, y*s, z*s);
	}

	Vec3<T> operator /(const Vec3<T> &v) const{
		return Vec3(x/v.x, y/v.y, z/v.z);
	}

	Vec3<T> operator /(T s) const{
		return Vec3(x/s, y/s, z/s);
	}

	Vec3<T> operator +=(const Vec3<T> &v){
		x+=v.x; 
		y+=v.y;
		z+=v.z;
		return *this;
	}

	Vec3<T> operator -=(const Vec3<T> &v){
		x-=v.x; 
		y-=v.y;
		z-=v.z;
		return *this;
	}

	Vec3<T> lerp(T t, const Vec3<T> &v) const{
		return *this + (v - *this) * t;
	}

	T dot(const Vec3<T> &v) const{
		return x*v.x + y*v.y + z*v.z;
	}

	float dist(const Vec3<T> &v) const{
		return Vec3<T>(v-*this).length();
	}

	float length() const{
		return static_cast<float>(sqrt(x*x + y*y + z*z));
	}

	void normalize(){
		T m= length(); 
		x/= m;
		y/= m;
		z/= m;
	}

	Vec3<T> getNormalized() const{
		T m= length();
		return Vec3<T>(x/m, y/m, z/m);
	}

	Vec3<T> cross(const Vec3<T> &v) const{
		return Vec3<T>(
			this->y*v.z-this->z*v.y,
			this->z*v.x-this->x*v.z,
			this->x*v.y-this->y*v.x);
	}

	Vec3<T> normal(const Vec3<T> &p1, const Vec3<T> &p2) const{
		Vec3<T> rv;
		rv= (p2-*this).cross(p1-*this);
		rv.normalize();
		return rv;
	}

	Vec3<T> normal(const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, const Vec3<T> &p4) const{
		Vec3<T> rv;

		rv= this->normal(p1, p2);
		rv= rv + this->normal(p2, p3);
		rv= rv + this->normal(p3, p4);
		rv= rv + this->normal(p4, p1);
		rv.normalize();
		return rv;
	}

};

typedef Vec3<int> Vec3i;
typedef Vec3<bool> Vec3b;
typedef Vec3<char> Vec3c;
typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;

// =====================================================
//	class Vec4
// =====================================================

template<typename T>
class Vec4{
public:
	T x;
	T y;
	T z;
	T w;
public:
	Vec4(){
	};

	explicit Vec4(T *p){
		this->x= p[0];
		this->y= p[1];
		this->z= p[2];
		this->w= p[3];
	}

	explicit Vec4(T xyzw){
		this->x= xyzw;
		this->y= xyzw;
		this->z= xyzw;
		this->w= xyzw;
	}

	template<typename S>
	explicit Vec4(const Vec4<S> &v){
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
		this->w= v.w;
	}

	Vec4(T x, T y, T z, T w){
		this->x= x;
		this->y= y;
		this->z= z;
		this->w= w;
	}

	Vec4(Vec3<T> v, T w){
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
		this->w= w;
	}

	explicit Vec4(Vec3<T> v){
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
		this->w= 1;
	}

	T *ptr(){
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const{
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec4<T> &v) const{
		return x==v.x && y==v.y && z==v.z && w==v.w;
	}

	bool operator !=(const Vec4<T> &v) const{
		return x!=v.x || y!=v.y || z!=v.z || w!=v.w;
	}

	Vec4<T> operator +(const Vec4<T> &v) const{
		return Vec4(x+v.x, y+v.y, z+v.z, w+v.w);
	}

	Vec4<T> operator -(const Vec4<T> &v) const{
		return Vec4(x-v.x, y-v.y, z-v.z, w-v.w);
	}

	Vec4<T> operator -() const{
		return Vec4(-x, -y, -z, -w);
	}

	Vec4<T> operator *(const Vec4<T> &v) const{
		return Vec4(x*v.x, y*v.y, z*v.z, w*v.w);
	}

	Vec4<T> operator *(T s) const{
		return Vec4(x*s, y*s, z*s, w*s);
	}

	Vec4<T> operator /(const Vec4<T> &v) const{
		return Vec4(x/v.x, y/v.y, z/v.z, w/v.w);
	}

	Vec4<T> operator /(T s) const{
		return Vec4(x/s, y/s, z/s, w/s);
	}

	Vec4<T> operator +=(const Vec4<T> &v){
		x+=v.x; 
		y+=v.y;
		z+=v.z;
		w+=w.z;
		return *this;
	}

	Vec4<T> operator -=(const Vec4<T> &v){
		x-=v.x; 
		y-=v.y;
		z-=v.z;
		w-=w.z;
		return *this;
	}

	Vec4<T> lerp(T t, const Vec4<T> &v) const{
		return *this + (v - *this) *t;
	}

	T dot(const Vec4<T> &v) const{
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}
};

typedef Vec4<int> Vec4i;
typedef Vec4<bool> Vec4b;
typedef Vec4<char> Vec4c;
typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;

}} //enmd namespace

#endif
