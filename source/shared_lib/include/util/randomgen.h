// ==============================================================
//	This file is part of MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke
//                2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_RANDOM_H_
#define _SHARED_UTIL_RANDOM_H_

#include "leak_dumper.h"

namespace Shared { namespace Util {

// =====================================================
//	class RandomGen
// =====================================================

class RandomGen {
private:
	static const int m;
	static const int a;
	static const int b;

private:
	int lastNumber;
//#ifdef USE_STREFLOP
//	streflop::RandomState randomState;
//#endif

public:
	RandomGen();
	void init(int seed);

	int rand();
	int randRange(int min, int max);
	float randRange(float min, float max);

	int getLastNumber() const { return lastNumber; }
	void setLastNumber(int value) { lastNumber = value; }
};

}}//end namespace

#endif
