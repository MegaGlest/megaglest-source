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

#ifndef _SHARED_UTIL_RANDOM_H_
#define _SHARED_UTIL_RANDOM_H_

namespace Shared{ namespace Util{

// =====================================================
//	class Random
// =====================================================

class Random{
private:
	static const int m;
	static const int a;
	static const int b;

private:
	int lastNumber;
	
public:
	Random();
	void init(int seed);

	int rand();
	int randRange(int min, int max);
	float randRange(float min, float max);
};

}}//end namespace

#endif
