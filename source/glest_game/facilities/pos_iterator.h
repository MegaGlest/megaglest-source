// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009-2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_UTIL_POSITERATOR_H_
#define _GLEST_GAME_UTIL_POSITERATOR_H_

#include <cassert>
#include <stdexcept>
#include "math_util.h"

namespace Glest { namespace Util {

using std::runtime_error;
using Shared::Graphics::Vec2i;
using Shared::Graphics::Rect2i;

class RectIteratorBase {
protected:
	int ex, wx, sy, ny;

public:
	RectIteratorBase(const Vec2i &p1, const Vec2i &p2) {
		if (p1.x > p2.x) {
			ex = p1.x; wx = p2.x;
		} else {
			ex = p2.x; wx = p1.x;
		}
		if (p1.y > p2.y) {
			sy = p1.y; ny = p2.y;
		} else {
			sy = p2.y; ny = p1.y;
		}
	}
};

/** An iterator over a rectangular region that starts at the 'top-left' and proceeds left 
  * to right, top to bottom. */
class RectIterator : public RectIteratorBase {
private:
	int cx, cy;

public:
	RectIterator(const Rect2i &rect)
			: RectIteratorBase(rect.p[0], rect.p[1]) {
		cx = wx;
		cy = ny;
	}

	RectIterator(const Vec2i &p1, const Vec2i &p2)
			: RectIteratorBase(p1, p2) {
		cx = wx;
		cy = ny;
	}

	bool  more() const { return cy <= sy; }

	Vec2i next() { 
		Vec2i n(cx, cy); 
		if (cx == ex) {
			cx = wx; ++cy;
		} else {
			++cx;
		}
		return n;
	}
};

/** An iterator over a rectangular region that starts at the 'bottom-right' and proceeds right 
  * to left, bottom to top. */
class ReverseRectIterator : public RectIteratorBase {
private:
	int cx, cy;

public:
	ReverseRectIterator(const Vec2i &p1, const Vec2i &p2) 
			: RectIteratorBase(p1, p2) {
		cx = ex;
		cy = sy;
	}

	bool  more() const { return cy >= ny; }
	
	Vec2i next() { 
		Vec2i n(cx,cy); 
		if (cx == wx) {
			cx = ex; cy--;
		} else {
			cx--;
		}
		return n;
	}
};

class PerimeterIterator : public RectIteratorBase {
private:
	int cx, cy;
	int state;

public:
	PerimeterIterator(const Vec2i &p1, const Vec2i &p2)
			: RectIteratorBase(p1, p2), state(0) {
		cx = wx;
		cy = ny;
	}

	bool  more() const { return state != 4; }
	Vec2i next();
};

}} // namespace Glest::Util

#endif // _GLEST_GAME_UTIL_POSITERATOR_H_
