// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009	James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================


#ifndef _GAME_SEARCH_INLUENCE_MAP_H_
#define _GAME_SEARCH_INLUENCE_MAP_H_

#include "vec.h"
#include "data_types.h"

#include <algorithm>

namespace Glest { namespace Game {

using Shared::Platform::uint32;

typedef Shared::Graphics::Vec2i Point;

struct Rectangle {
	Rectangle() : x(0), y(0), w(0), h(0) {}
	Rectangle(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
	int x, y, w, h;
};

const Point invalidPos(-1,-1);

// =====================================================
// 	class InfluenceMap
// =====================================================
template<typename iType, typename MapType> class InfluenceMap {
public:
	InfluenceMap(Rectangle b, iType def) : def(def), x(b.x), y(b.y), w(b.w), h(b.h) {}
	
	iType getInfluence(Point pos) {
		pos = translate(pos);
		if (pos != invalidPos) {
			return static_cast<MapType*>(this)->get(pos);
		}
		return def;
	}
	bool setInfluence(Point pos, iType val) {
		pos = translate(pos);
		if (pos != invalidPos) {
			static_cast<MapType*>(this)->set(pos, val);
			return true;
		}
		return false;
	}
	Point translate(Point p) {
		const int nx = p.x - x;
		const int ny = p.y - y;
		if (nx >= 0 && ny >=0 && nx < w && ny < h) {
			return Point(nx, ny);
		}
		return invalidPos;
	}
	Point getPos() const { return Point(x, y); }
	Rectangle getBounds() const { return Rectangle(x, y, w, h); }

protected:
	iType def;			// defualt value for positions not within (this) maps dimensions 
	int x , y, w, h;	// Dimensions of this map.
};

// =====================================================
// 	class TypeMap
// =====================================================
template<typename type = float> class TypeMap : public InfluenceMap<type, TypeMap<type> > {
	friend class InfluenceMap<type, TypeMap<type> >;
public:
	TypeMap(Rectangle b, type def) : InfluenceMap<type, TypeMap<type> >(b,def) {
		data = new type[b.w*b.h];
	}
	~TypeMap() { delete [] data; }
	void zeroMap() { memset(data, 0, sizeof(type) * this->w * this->h); }
	void clearMap(type val) { std::fill_n(data, this->w * this->h, val); }

private:
	type get(Point p) { return data[p.y * this->w + p.x]; }
	void  set(Point p, type v) { data[p.y * this->w + p.x] = v; }
	type *data;
};

// =====================================================
// 	class PatchMap
// =====================================================
/** bit packed InfluenceMap, values are bit packed into 32 bit 'sections' (possibly padded)
  * Not usefull for bits >= 7, just use TypeMap<uint8>, TypeMap<uint16> etc...
  * bit wastage:
  * bits == 1, 2 or 4: no wastage in full sections
  * bits == 3, 5 or 6: 2 bits wasted per full section
  */
template<int bits> class PatchMap : public InfluenceMap<uint32, PatchMap<bits> > {
	friend class InfluenceMap<uint32, PatchMap<bits> >;
public:
	PatchMap(Rectangle b, uint32 def) : InfluenceMap<uint32,PatchMap<bits> >(b,def) {
		//cout << "PatchMap<" << bits << ">  max_value = "<< max_value <<", [width = " << b.w << " height = " << b.h << "]\n";
		sectionsPerRow = b.w / sectionSize + 1;
		//cout << "section size = " << sectionSize << ", sections per row = " << sectionsPerRow << endl;
		data = new uint32[b.h * sectionsPerRow];
	}
	//only for infuence_map_test.cpp:249
	PatchMap<bits> &operator=(const PatchMap<bits> &op){
		if(&op != this) {
			//FIXME: better when moved to InfluenceMap::operator=...
			this->def = op.def;
			this->x = op.x; this->y = op.y; this->w = op.w; this->h = op.h;
			//

			sectionsPerRow = op.sectionsPerRow;
			delete[] data;
			data = new uint32[op.h * sectionsPerRow];
			memcpy(data, op.data, op.h * sectionsPerRow);
		}
		return *this;
	}
	~PatchMap() { delete [] data; }
	void zeroMap() { memset(data, 0, sizeof(uint32) * sectionsPerRow * this->h); }
	void clearMap(uint32 val) { 
//		assert(val < max_value);
		data[0] = 0;
		for ( int i=0; i < sectionSize - 1; ++i) {
			data[0] |= val;
			data[0] <<= bits;
		}
		data[0] |= val;
		for ( int i=1; i < sectionsPerRow; ++i ) {
			data[i] = data[0];
		}
		const int rowSize = sectionsPerRow * sizeof(uint32);
		for ( int i=1; i < this->h; ++i ) {
			uint32 *ptr = &data[i * sectionsPerRow];
			memcpy(ptr, data, rowSize);
		}
	}
private:
	uint32 get(Point p) {
		int sectionNdx = p.y * sectionsPerRow + p.x / sectionSize;
		int subSectionNdx = p.x % sectionSize;
		uint32 val = (data[sectionNdx] >> (subSectionNdx * bits)) & max_value;
		//cout << "get(" << p.x << "," << p.y << "). section=" << sectionNdx 
		//	<< ", subsection=" << subSectionNdx << " = " << val <<endl;
		return val;
	}
	void set(Point p, uint32 v) {
		if (v > max_value) { //clamp?
			v = max_value;
		}
		int sectionNdx = p.y * sectionsPerRow + p.x / sectionSize;
		int subSectionNdx = p.x % sectionSize;
		uint32 val = v << bits * subSectionNdx;
		uint32 mask = max_value << bits * subSectionNdx;
		data[sectionNdx] &= ~mask;	// blot out old value
		data[sectionNdx] |= val;	// put in the new value
	}/*
	void logSection(int s) {
		cout << "[";
		for ( int j = 31; j >= 0; --j ) {
			uint32 bitmask = 1 << j;
			if ( data[s] & bitmask ) cout << "1";
			else cout << "0";
			if ( j && j % bits == 0 ) cout << " ";
		}
		cout << "]" << endl;
	}*/
	static const uint32 max_value = (1 << bits) - 1;
	static const int sectionSize = 32 / bits;
	int sectionsPerRow;
	uint32 *data;
};

// =====================================================
// 	class FlowMap
// =====================================================
/** bit packed 'offset' map, where offset is of the form  -1 <= x <= 1 && -1 <= y <= 1  */
class FlowMap : public InfluenceMap<Point, FlowMap> {
	friend class InfluenceMap<Point, FlowMap>;
private:
	Point expand(uint32 v) {
		Point res(0,0);
		if ( v & 8 ) { res.x = -1; }
		else if ( v & 4 ) { res.x = 1; }
		if ( v & 2 ) { res.y = -1; }
		else if ( v & 1 ) { res.y = 1; }
		return res;
	}
	uint32 pack(Point v) {
		uint32 res = 0;
		if ( v.x ) { res  = 1 << (v.x > 0 ? 2 : 3); }
		if ( v.y ) { res |= 1 << (v.y > 0 ? 0 : 1); }
		return res;
	}
public:
	FlowMap(Rectangle b, Point def) : InfluenceMap<Point,FlowMap>(b,def) {
		blocksPerRow = b.w / 8 + 1;
		data = new uint32[b.h * blocksPerRow];
	}
	~FlowMap() { delete [] data; }
	void zeroMap() { memset(data, 0, sizeof(uint32) * blocksPerRow * h); }
	void clearMap(Point val) {
		uint32 v = pack(val);
		data[0] = 0;
		for ( int i=0; i < 7; ++i ) {
			data[0]  |= v;
			data[0] <<= 4;
		}
		data[0] |= v;
		for ( int i=1; i < blocksPerRow; ++i ) {
			data[i] = data[0];
		}
		const int rowSize = blocksPerRow * sizeof(uint32);
		for ( int i=1; i < h; ++i ) {
			uint32 *ptr = &data[i * blocksPerRow];
			memcpy(ptr, data, rowSize);
		}
	}

private:
	Point get(Point p) {
		const int blockNdx = p.y * blocksPerRow + p.x / 8;
		const int subBlockNdx = p.x % 8;
		uint32 val = (data[blockNdx] >> (subBlockNdx * 4)) & 15;
		return expand(val);
	}
	void set(Point p, Point v) {
		int blockNdx = p.y * blocksPerRow + p.x / 8;
		int subBlockNdx = p.x % 8;
		uint32 val = pack(v) << 4 * subBlockNdx;
		uint32 mask = 15 << 4 * subBlockNdx;
		data[blockNdx] &= ~mask;	// blot out old value
		data[blockNdx] |= val;	// put in the new value
	}/*
	void logSection(int s) {
		cout << "[";
		for ( int j = 31; j >= 0; --j ) {
			uint32 bitmask = 1 << j;
			if ( data[s] & bitmask ) cout << "1";
			else cout << "0";
			if ( j && j % 4 == 0 ) cout << " ";
		}
		cout << "]" << endl;
	}*/
	int blocksPerRow;
	uint32 *data; // 8 values each
};

}}

#endif
