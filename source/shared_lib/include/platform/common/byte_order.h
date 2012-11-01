#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H

#include <algorithm>

namespace Shared{ namespace PlatformByteOrder {

template<class T> T EndianReverse(T t) {
	unsigned char uc[sizeof t];
	memcpy(uc, &t, sizeof t);

	for (unsigned char *b = uc, *e = uc + sizeof(T) - 1; b < e; ++b, --e) {
		std::swap(*b, *e);
	}
	memcpy(&t, uc, sizeof t);
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

}}

#endif
