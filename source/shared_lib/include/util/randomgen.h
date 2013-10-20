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

#include <string>
#include <vector>
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
	std::vector<std::string> lastCaller;
	bool disableLastCallerTracking;

	int rand(std::string lastCaller);

public:
	RandomGen();
	void init(int seed);

	int randRange(int min, int max,std::string lastCaller="");
	float randRange(float min, float max,std::string lastCaller="");

	int getLastNumber() const { return lastNumber; }
	void setLastNumber(int value) { lastNumber = value; }

	std::string getLastCaller() const;
	void clearLastCaller();
	void addLastCaller(std::string text);
	void setDisableLastCallerTracking(bool value) { disableLastCallerTracking = value; }
};

}}//end namespace

#endif
