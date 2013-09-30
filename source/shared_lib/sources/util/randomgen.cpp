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

#include "randomgen.h"
#include <cassert>
#include "util.h"
#include <stdexcept>
#include "platform_util.h"
#include "math_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Graphics;

namespace Shared { namespace Util {

// =====================================================
//	class RandomGen
// =====================================================

const int RandomGen::m= 714025;
const int RandomGen::a= 1366;
const int RandomGen::b= 150889;

RandomGen::RandomGen(){

//#ifdef USE_STREFLOP
//	lastNumber = streflop::RandomInit(0); // streflop
//#else
	lastNumber= 0;
//#endif
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);
}

void RandomGen::init(int seed){

//#ifdef USE_STREFLOP
//	lastNumber = streflop::RandomInit(seed); // streflop
//#else
	lastNumber= seed % m;
//#endif

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] seed = %d, lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,seed,lastNumber);
}

int RandomGen::rand(string lastCaller) {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);

	this->lastNumber = (a*lastNumber + b) % m;
	//this->lastCaller.push_back(lastCaller);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);

	return lastNumber;
}

std::string RandomGen::getLastCaller() const {
	std::string result = "";
	if(lastCaller.empty() == false) {
		for(unsigned int index = 0; index < lastCaller.size(); ++index) {
			result += lastCaller[index] + " ";
		}
	}
	return result;
}
int RandomGen::randRange(int min, int max,string lastCaller) {
	if(min > max) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] min > max, min = %d, max = %d",__FILE__,__FUNCTION__,__LINE__,min,max);
		throw megaglest_runtime_error(szBuf);
	}

	int diff= max-min;
	double numerator = static_cast<double>(diff + 1) * static_cast<double>(RandomGen::rand(lastCaller));
	int res= min + static_cast<int>(truncateDecimal<double>(numerator / static_cast<double>(m),10));
	if(res < min || res > max) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] res < min || res > max, min = %d, max = %d, res = %d",__FILE__,__FUNCTION__,__LINE__,min,max,res);
		throw megaglest_runtime_error(szBuf);
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] min = %d, max = %d, res = %d\n",__FILE__,__FUNCTION__,__LINE__,min,max,res);

	return res;
}

double RandomGen::randRange(double min, double max,string lastCaller) {
	if(min > max) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] min > max, min = %f, max = %f",__FILE__,__FUNCTION__,__LINE__,min,max);
		throw megaglest_runtime_error(szBuf);
	}

	double rand01 = static_cast<double>(RandomGen::rand(lastCaller)) / (m-1);
	double res= min + (max - min) * rand01;
	res = truncateDecimal<double>(res,10);

	if(res < min || res > max) {
		char szBuf[8096]="";
		snprintf(szBuf,8096,"In [%s::%s Line: %d] res < min || res > max, min = %f, max = %f, res = %f",__FILE__,__FUNCTION__,__LINE__,min,max,res);
		throw megaglest_runtime_error(szBuf);
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] min = %f, max = %f, res = %f\n",__FILE__,__FUNCTION__,__LINE__,min,max,res);

	return res;
}

}}//end namespace
