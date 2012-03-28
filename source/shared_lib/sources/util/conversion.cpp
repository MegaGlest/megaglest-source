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

#include "conversion.h"

#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include "platform_common.h"
#include <sstream>
#include <iostream>
#include <locale>
#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Util{

const int strSize = 256;

bool strToBool(const string &s) {
	if(s=="0" || s=="false") {
		return false;
	}
	if(s=="1" || s=="true") {
		return true;
	}
	throw runtime_error("Error converting string to bool, expected 0 or 1, found: [" + s + "]");
}

int strToInt(const string &s){
	char *endChar;
	setlocale(LC_NUMERIC, "C");
	int intValue= strtol(s.c_str(), &endChar, 10);

	if(*endChar!='\0'){
		throw runtime_error("Error converting from string to int, found: [" + s + "]");
	}

	return intValue;
}


float strToFloat(const string &s){
	char *endChar;

	setlocale(LC_NUMERIC, "C");
	float floatValue= static_cast<float>(strtod(s.c_str(), &endChar));

	if(*endChar!='\0'){
		throw runtime_error("Error converting from string to float, found: [" + s + "]");
	}

	return floatValue;
}

bool strToBool(const string &s, bool *b){
	if (s=="0" || s=="false"){
		*b= false;
		return true;
	}
	if (s=="1" || s=="true"){
		*b= true;
		return true;
	}

	return false;
}

bool strToInt(const string &s, int *i) {
	char *endChar;

	setlocale(LC_NUMERIC, "C");
	*i= strtol(s.c_str(), &endChar, 10);

	if(*endChar!='\0'){
		return false;
	}
	return true;
}

bool strToFloat(const string &s, float *f) {
	char *endChar;
	setlocale(LC_NUMERIC, "C");
	*f= static_cast<float>(strtod(s.c_str(), &endChar));

	if(*endChar!='\0'){
		return false;
	}
	return true;
}

string boolToStr(bool b) {
	if(b) {
		return "1";
	}
	else{
		return "0";
	}
}

string intToStr(int64 i) {
	char str[strSize]="";
	snprintf(str, strSize-1, "%lld", (long long int)i);
	return (str[0] != '\0' ? str : "");
}

string intToHex(int i){
	char str[strSize]="";
	snprintf(str, strSize-1, "%x", i);
	return (str[0] != '\0' ? str : "");
}

string floatToStr(float f,int precsion) {
	setlocale(LC_NUMERIC, "C");

	char str[strSize]="";
	snprintf(str, strSize-1, "%.*f", precsion,f);
	return (str[0] != '\0' ? str : "");
}

string doubleToStr(double d,int precsion) {
	setlocale(LC_NUMERIC, "C");

	char str[strSize]="";
	snprintf(str, strSize-1, "%.*f", precsion,d);
	return (str[0] != '\0' ? str : "");
}

bool IsNumeric(const char *p, bool  allowNegative) {
	if(p == NULL) {
		return false;
	}
	if(strcmp(p,"-") == 0) {
		return false;
	}
    int index = 0;
    for ( ; *p; p++) {
      if (*p < '0' || *p > '9') {
    	  if(allowNegative == false || (*p != '-' && index == 0)) {
    		  return false;
    	  }
      }
      index++;
    }
    return true;
}

class Comma: public numpunct<char>// own facet class
{
     protected:
          char do_thousands_sep() const { return ','; }// use the comma
          string do_grouping() const { return "\3"; }//group 3 digits
};
string formatNumber(uint64 f) {

	locale myloc(  locale(),    // C++ default locale
	          new Comma);// Own numeric facet

	ostringstream out;
	out.imbue(myloc);
	out << f;
	return out.str();
}

}}//end namespace
