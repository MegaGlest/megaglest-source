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

#include "pos_iterator.h"

#include "leak_dumper.h"

namespace Glest { namespace Util {

Vec2i PerimeterIterator::next() { 
	Vec2i n(cx, cy);
	switch (state) {
		case 0: // top edge, left->right
			if (cx == ex) {
				state = 1;
				++cy;
			} else {
				++cx;
			}
			break;
		case 1: // right edge, top->bottom
			if (cy == sy) {
				state = 2;
				--cx;
			} else {
				++cy;
			}
			break;
		case 2:
			if (cx == wx) {
				state = 3;
				--cy;
			} else {
				--cx;
			}
			break;
		case 3:
			if (cy == ny + 1) {
				state = 4;
			} else {
				--cy;
			}
			break;
	}
	return n;
}

}}//end namespace

