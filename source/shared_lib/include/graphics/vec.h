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

#include "math_wrapper.h"
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

inline std::vector<std::string> TokenizeString(const std::string str,const std::string delimiters) {
	std::vector<std::string> tokens;
	// Assume textLine contains the line of text to parse.
	std::string textLine = str;

	std::size_t pos = 0;
	while( true ) {
		std::size_t nextPos = textLine.find( delimiters, pos );
		if( nextPos == textLine.npos ) {
			tokens.push_back(textLine.substr(pos, textLine.length( )));
			break;
		}
		tokens.push_back( std::string( textLine.substr( pos, nextPos - pos ) ) );
		pos = nextPos + 1;
	}

	return tokens;
}

template<typename T>
inline T strToType(const std::string &s) {
	char *endChar=NULL;

	setlocale(LC_NUMERIC, "C");
	T value= static_cast<T>(strtod(s.c_str(), &endChar));

	if(*endChar!='\0'){
		throw std::runtime_error("Error converting from string to type, found: [" + s + "]");
	}

	return value;
}


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
	template<typename S>
	explicit Vec2(Vec2<S> &v){
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

	Vec2<T> & operator=(const Vec2<T> &v) {
		this->x= v.x;
		this->y= v.y;
		return *this;
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

	inline float dist(const Vec2<T> &v) const{
		return Vec2<T>(v-*this).length();
	}

	// strict week ordering, so Vec2<T> can be used as key for set<> or map<>
	bool operator<(const Vec2<T> &v) const {
		return x < v.x || (x == v.x && y < v.y);
	}

	float length() const{
#ifdef USE_STREFLOP
		return static_cast<float>(streflop::sqrt(static_cast<streflop::Simple>(x*x + y*y)));
#else
		return static_cast<float>(sqrt(static_cast<float>(x*x + y*y)));
#endif
	}

	void normalize(){
		T m= length(); 
		x/= m;
		y/= m;
	}
	
	Vec2<T> rotate(float rad){
		const float
#ifdef USE_STREFLOP
			c = streflop::cosf(rad),
			s = streflop::sinf(rad);
#else
			c = cosf(rad),
			s = sinf(rad);
#endif
		return Vec2<T>(x*c-y*s,x*s+y*c);
	}

	Vec2<T> rotateAround(float rad,const Vec2<T>& pt){
		return pt+(*this-pt).rotate(rad);
	}

	std::string getString() const {
		std::ostringstream streamOut;
		streamOut << "x [" << x;
		streamOut << "] y [" << y << "]";
		std::string result = streamOut.str();
		streamOut.str(std::string());
		return result;
	}
	// meetingPos="x [32] y [120]"
	static inline Vec2<T> strToVec2(std::string value) {
		Vec2<T> result;

		std::vector<std::string> tokens = TokenizeString(value,"[");
		//for(unsigned int i = 0; i < tokens.size(); ++i) {
			//printf("#1 Vec2T i = %d [%s]\n",i,tokens[i].c_str());
		//}
		if(tokens.size() == 3) {
			std::vector<std::string> tokens2 = TokenizeString(tokens[1],"]");
			//for(unsigned int i = 0; i < tokens2.size(); ++i) {
				//printf("#2 Vec2T i = %d [%s]\n",i,tokens2[i].c_str());
			//}
			std::vector<std::string> tokens3 = TokenizeString(tokens[2],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			if(tokens2.size() == 2 && tokens3.size() == 2) {
				result.x = (T)strToType<T>(tokens2[0]);
				result.y = (T)strToType<T>(tokens3[0]);

				//printf("#3 Vec2T [%s]\n",result.getString().c_str());
			}
		}


		return result;
	}
};

template <typename T>
std::ostream& operator<<(std::ostream &stream, const Vec2<T> &vec) {
	return stream << "(" << vec.x << ", " << vec.y << ")";
}


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

	template<typename S>
	explicit Vec3(Vec3<S> &v){
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

	Vec3<T> & operator=(const Vec3<T> &v) {
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
		return *this;
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

	bool operator <(const Vec3<T> &v) const {
		return x < v.x || (x == v.x && y < v.y) || (x == v.x && y == v.y && z < v.z);
	}

	Vec3<T> lerp(T t, const Vec3<T> &v) const{
		return *this + (v - *this) * t;
	}

	T dot(const Vec3<T> &v) const{
		return x*v.x + y*v.y + z*v.z;
	}

	inline float dist(const Vec3<T> &v) const{
		return Vec3<T>(v-*this).length();
	}

	float length() const{
#ifdef USE_STREFLOP
		return static_cast<float>(streflop::sqrt(static_cast<streflop::Simple>(x*x + y*y + z*z)));
#else
		return static_cast<float>(sqrt(x*x + y*y + z*z));
#endif
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

	std::string getString() const {
		std::ostringstream streamOut;
		streamOut << "x [" << x;
		streamOut << "] y [" << y;
		streamOut << "] z [" << z << "]";
		std::string result = streamOut.str();
		streamOut.str(std::string());
		return result;
	}

	// playerColor="x [1] y [0] z [0]"
	static inline Vec3<T> strToVec3(std::string value) {
		Vec3<T> result;

		std::vector<std::string> tokens = TokenizeString(value,"[");
		//for(unsigned int i = 0; i < tokens.size(); ++i) {
			//printf("#1 Vec2T i = %d [%s]\n",i,tokens[i].c_str());
		//}
		if(tokens.size() == 4) {
			std::vector<std::string> tokens2 = TokenizeString(tokens[1],"]");
			//for(unsigned int i = 0; i < tokens2.size(); ++i) {
				//printf("#2 Vec2T i = %d [%s]\n",i,tokens2[i].c_str());
			//}
			std::vector<std::string> tokens3 = TokenizeString(tokens[2],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			std::vector<std::string> tokens4 = TokenizeString(tokens[3],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			if(tokens2.size() == 2 && tokens3.size() == 2 && tokens4.size() == 2) {
				result.x = (T)strToType<T>(tokens2[0]);
				result.y = (T)strToType<T>(tokens3[0]);
				result.z = (T)strToType<T>(tokens4[0]);

				//printf("#3 Vec2T [%s]\n",result.getString().c_str());
			}
		}


		return result;
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
	Vec4() {
	};

	explicit Vec4(const T *p){
		this->x= p[0];
		this->y= p[1];
		this->z= p[2];
		this->w= p[3];
	}

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

	template<typename S>
	explicit Vec4(Vec4<S> &v){
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

	Vec4<T> & operator=(const Vec4<T> &v) {
		this->x= v.x;
		this->y= v.y;
		this->z= v.z;
		this->w= v.w;
		return *this;
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
	bool operator <(const Vec4<T> &v) const {
		return x < v.x || (x == v.x && y < v.y) || (x == v.x && y == v.y && z < v.z) || (x == v.x && y == v.y && z == v.z && w < v.w);
	}

	Vec4<T> lerp(T t, const Vec4<T> &v) const{
		return *this + (v - *this) *t;
	}

	T dot(const Vec4<T> &v) const{
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}

	std::string getString() const {
		std::ostringstream streamOut;
		streamOut << "x [" << x;
		streamOut << "] y [" << y;
		streamOut << "] z [" << z;
		streamOut << "] w [" << w << "]";
		std::string result = streamOut.str();
		streamOut.str(std::string());
		return result;
	}

	// playerColor="x [1] y [0] z [0] w [0]"
	static inline Vec4<T> strToVec4(std::string value) {
		Vec4<T> result;

		std::vector<std::string> tokens = TokenizeString(value,"[");
		//for(unsigned int i = 0; i < tokens.size(); ++i) {
			//printf("#1 Vec2T i = %d [%s]\n",i,tokens[i].c_str());
		//}
		if(tokens.size() == 5) {
			std::vector<std::string> tokens2 = TokenizeString(tokens[1],"]");
			//for(unsigned int i = 0; i < tokens2.size(); ++i) {
				//printf("#2 Vec2T i = %d [%s]\n",i,tokens2[i].c_str());
			//}
			std::vector<std::string> tokens3 = TokenizeString(tokens[2],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			std::vector<std::string> tokens4 = TokenizeString(tokens[3],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			std::vector<std::string> tokens5 = TokenizeString(tokens[4],"]");
			//for(unsigned int i = 0; i < tokens3.size(); ++i) {
				//printf("#3 Vec2T i = %d [%s]\n",i,tokens3[i].c_str());
			//}

			if(tokens2.size() == 2 && tokens3.size() == 2 &&
					tokens4.size() == 2 && tokens5.size() == 2) {
				result.x = (T)strToType<T>(tokens2[0]);
				result.y = (T)strToType<T>(tokens3[0]);
				result.z = (T)strToType<T>(tokens4[0]);
				result.w = (T)strToType<T>(tokens5[0]);

				//printf("#3 Vec2T [%s]\n",result.getString().c_str());
			}
		}

		return result;
	}

};

typedef Vec4<int> Vec4i;
typedef Vec4<bool> Vec4b;
typedef Vec4<char> Vec4c;
typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;

}} //enmd namespace

#endif
