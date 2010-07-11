// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _LINE_ALGORITHM_INCLUDED_
#define _LINE_ALGORITHM_INCLUDED_

#include <cassert>

namespace Shared { namespace Util {

/** midpoint line algorithm, 'Visit' specifies the 'pixel visit' function,	*
  * and must take two int params (x & y co-ords)							*/
template<typename VisitFunc> void line(int x0, int y0, int x1, int y1, VisitFunc visitor) {
	bool mirror_x, mirror_y;
	int  pivot_x, pivot_y;
	if (x0 > x1) {
		mirror_x = true;
		pivot_x = x0;
		x1 = (x0 << 1) - x1;
	} else {
		mirror_x = false;
	}
	if (y0 > y1) {
		mirror_y = true;
		pivot_y = y0;
		y1 = (y0 << 1) - y1;
	} else {
		mirror_y = false;
	}
	// Visit(x,y) => Visit(mirror_x ? (pivot_x << 1) - x : x, mirror_y ? (pivot_y << 1) - y : y);
	assert(y0 <= y1 && x0 <= x1);
	int dx = x1 - x0,
		dy = y1 - y0;
	int x = x0,
		y = y0;

	if (dx == 0) {
		while (y <= y1) {
			visitor(mirror_x ? (pivot_x << 1) - x : x, mirror_y ? (pivot_y << 1) - y : y);
			++y;
		}
	} else if (dy == 0) {
		while (x <= x1) {
			visitor(mirror_x ? (pivot_x << 1) - x : x, mirror_y ? (pivot_y << 1) - y : y);
			++x;
		}
	} else if (dy > dx) {
		int d = 2 * dx - dy;
		int incrS = 2 * dx;
		int incrSE = 2 * (dx - dy);
		do {
			visitor(mirror_x ? (pivot_x << 1) - x : x, mirror_y ? (pivot_y << 1) - y : y);
			if (d <= 0) {
				d += incrS;		++y;
			} else {
				d += incrSE;	++x;	++y;
			}
		} while (y <= y1);
	} else {
		int d = 2 * dy - dx;
		int incrE = 2 * dy;
		int incrSE = 2 * (dy - dx);
		do {
			visitor(mirror_x ? (pivot_x << 1) - x : x, mirror_y ? (pivot_y << 1) - y : y);
			if (d <= 0) {
				d += incrE;		++x;
			} else {
				d += incrSE;	++x;	++y;
			}
		} while (x <= x1);
	}
}

}} // end namespace Shared::Util

#endif // !def _LINE_ALGORITHM_INCLUDED_
