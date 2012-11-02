// ==============================================================
//	This file is part of MegaGlest (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H

#include <algorithm>
#include "leak_dumper.h"

namespace Shared{ namespace PlatformByteOrder {

template<class T> T EndianReverse(T t) {
//	unsigned char uc[sizeof t];
//	memcpy(uc, &t, sizeof t);
//
//	for (unsigned char *b = uc, *e = uc + sizeof(T) - 1; b < e; ++b, --e) {
//		std::swap(*b, *e);
//	}
//	memcpy(&t, uc, sizeof t);
//	return t;

	char& raw = reinterpret_cast<char&>(t);
    std::reverse(&raw, &raw + sizeof(T));
    return t;
}

static bool isBigEndian() {
	short n = 0x1;
	return (*(char*)(&n) == 0x0);
}


template<class T> T toCommonEndian(T t) {
	static bool bigEndianSystem = isBigEndian();
	if(bigEndianSystem == true) {
		t = EndianReverse(t);
	}
	return t;
}

template<class T> T fromCommonEndian(T t) {
	static bool bigEndianSystem = isBigEndian();
	if(bigEndianSystem == true) {
		t = EndianReverse(t);
	}
	return t;
}

template<class T>
void toEndianTypeArray(T *data, size_t size) {
	static bool bigEndianSystem = isBigEndian();
	if(bigEndianSystem == true) {
		for(size_t i = 0; i < size; ++i) {
			data[i] = toCommonEndian(data[i]);
		}
	}
}

template<class T>
void fromEndianTypeArray(T *data, size_t size) {
	static bool bigEndianSystem = isBigEndian();
	if(bigEndianSystem == true) {
		for(size_t i = 0; i < size; ++i) {
			data[i] = fromCommonEndian(data[i]);
		}
	}
}

}}

#endif
