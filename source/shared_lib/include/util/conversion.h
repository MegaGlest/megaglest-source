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

#ifndef _SHARED_UTIL_CONVERSION_H_
#define _SHARED_UTIL_CONVERSION_H_

#include <string>
#include "types.h"
#include "leak_dumper.h"

using std::string;

using namespace Shared::Platform;

namespace Shared { namespace Util {

bool strToBool(const string &s);
int strToInt(const string &s);
float strToFloat(const string &s); 

bool strToBool(const string &s, bool *b);
bool strToInt(const string &s, int *i);
bool strToFloat(const string &s, float *f);

string boolToStr(bool b);
string intToStr(int64 i);
string intToHex(int i);
string floatToStr(float f,int precsion=2);
string doubleToStr(double f,int precsion=2);

bool IsNumeric(const char *p, bool  allowNegative=true);

string formatNumber(uint64 f);

}}//end namespace

#endif
