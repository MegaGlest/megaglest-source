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

#ifndef _SHARED_GRAPHICS_MATHUTIL_H_
#define _SHARED_GRAPHICS_MATHUTIL_H_

#include <cmath>

#include "vec.h"

namespace Shared{ namespace Graphics{

const float pi= 3.1415926f;
const float sqrt2= 1.41421356f;
const float zero= 1e-6f;
const float infinity= 1e6f;

// =====================================================
//	class Rect
// =====================================================

// 0 +-+
//   | |
//   +-+ 1

template<typename T>
class Rect2{
public:
	Vec2<T> p[2];
public:
	Rect2(){
	};

	Rect2(const Vec2<T> &p0, const Vec2<T> &p1){
		this->p[0]= p0;
		this->p[1]= p1;
	} 

	Rect2(T p0x, T p0y, T p1x, T p1y){
		p[0].x= p0x;
		p[0].y= p0y;
		p[1].x= p1x;
		p[1].y= p1y;
	} 

	Rect2<T> operator*(T scalar){
		return Rect2<T>(
			p[0]*scalar,
			p[1]*scalar);
	}

	Rect2<T> operator/(T scalar){
		return Rect2<T>(
			p[0]/scalar,
			p[1]/scalar);
	}

	bool isInside(const Vec2<T> &p) const{
		return 
			p.x>=this->p[0].x && 
			p.y>=this->p[0].y && 
			p.x<this->p[1].x && 
			p.y<this->p[1].y;
	}

	void clamp(T minX, T minY,T  maxX, T maxY){
		for(int i=0; i<2; ++i){
			if(p[i].x<minX){
				p[i].x= minX;
			}
			if(p[i].y<minY){
				p[i].y= minY;
			}
			if(p[i].x>maxX){
				p[i].x= maxX;
			}
			if(p[i].y>maxY){
				p[i].y= maxY;
			}
		}
	}
};

typedef Rect2<int> Rect2i;
typedef Rect2<char> Rect2c;
typedef Rect2<float> Rect2f;
typedef Rect2<double> Rect2d;

// =====================================================
//	class Quad
// =====================================================

// 0 +-+ 2
//   | |
// 1 +-+ 3

template<typename T>
class Quad2{
public:
	Vec2<T> p[4];
public:
	Quad2(){
	};

	Quad2(const Vec2<T> &p0, const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3){
		this->p[0]= p0;
		this->p[1]= p1;
		this->p[2]= p2;
		this->p[3]= p3;
	} 

	explicit Quad2(const Rect2<T> &rect){
		this->p[0]= rect.p[0];
		this->p[1]= Vec2<T>(rect.p[0].x, rect.p[1].y);
		this->p[2]= rect.p[1];
		this->p[3]= Vec2<T>(rect.p[1].x, rect.p[0].y);
	}

	Quad2<T> operator*(T scalar){
		return Quad2<T>(
			p[0]*scalar,
			p[1]*scalar,
			p[2]*scalar,
			p[3]*scalar);
	}

	Quad2<T> operator/(T scalar){
		return Quad2<T>(
			p[0]/scalar,
			p[1]/scalar,
			p[2]/scalar,
			p[3]/scalar);
	}

	Rect2<T> computeBoundingRect() const{
		return Rect2i(
			min(p[0].x, p[1].x), 
			min(p[0].y, p[2].y), 
			max(p[2].x, p[3].x), 
			max(p[1].y, p[3].y));
	}

	bool isInside(const Vec2<T> &pt) const{
		
		if(!computeBoundingRect().isInside(pt))
			return false;

		bool left[4];

		left[0]= (pt.y - p[0].y)*(p[1].x - p[0].x) - (pt.x - p[0].x)*(p[1].y - p[0].y) < 0;
		left[1]= (pt.y - p[1].y)*(p[3].x - p[1].x) - (pt.x - p[1].x)*(p[3].y - p[1].y) < 0;
		left[2]= (pt.y - p[3].y)*(p[2].x - p[3].x) - (pt.x - p[3].x)*(p[2].y - p[3].y) < 0;
		left[3]= (pt.y - p[2].y)*(p[0].x - p[2].x) - (pt.x - p[2].x)*(p[0].y - p[2].y) < 0;
		
		return left[0] && left[1] && left[2] && left[3];
	}

	void clamp(T minX, T minY, T maxX, T maxY){
		for(int i=0; i<4; ++i){
			if(p[i].x<minX){
				p[i].x= minX;
			}
			if(p[i].y<minY){
				p[i].y= minY;
			}
			if(p[i].x>maxX){
				p[i].x= maxX;
			}
			if(p[i].y>maxY){
				p[i].y= maxY;
			}
		}
	}

	float area(){
		Vec2i v0= p[3]-p[0];
		Vec2i v1= p[1]-p[2];

		return 0.5f * ((v0.x * v1.y) - (v0.y * v1.x));
	}
};

typedef Quad2<int> Quad2i;
typedef Quad2<char> Quad2c;
typedef Quad2<float> Quad2f;
typedef Quad2<double> Quad2d;

// =====================================================
//	Misc
// =====================================================

inline int next2Power(int n){
	int i;
	for (i=1; i<n; i*=2);
	return i;
}

template<typename T>
inline T degToRad(T deg){
	return (deg*2*pi)/360;
}

template<typename T>
inline T radToDeg(T rad){
	return (rad*360)/(2*pi);
}

}}//end namespace

#endif
