// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "checksum.h"

#include <cassert>
#include <stdexcept>

#include "util.h"
#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Util{

// =====================================================
//	class Checksum
// =====================================================

Checksum::Checksum(){
	sum= 0;
	r= 55665;
	c1= 52845;
	c2= 22719;
}

void Checksum::addByte(int8 value){
	int32 cipher= (value ^ (r >> 8));

	r= (cipher + r) * c1 + c2;
	sum+= cipher;
}

void Checksum::addString(const string &value){
	for(int i= 0; i<value.size(); ++i){
		addByte(value[i]);
	}
}

void Checksum::addFile(const string &path){
	fileList[path] = 0;
}

void Checksum::addFileToSum(const string &path){

	FILE* file= fopen(path.c_str(), "rb");

	if(file!=NULL){

		addString(lastFile(path));

		while(!feof(file)){
			int8 byte= 0;

			size_t readBytes = fread(&byte, 1, 1, file);
			addByte(byte);
		}
	}
	else
	{
		throw runtime_error("Can not open file: " + path);
	}
	fclose(file);
}

int32 Checksum::getSum() {
	if(fileList.size() > 0) {
		for(std::map<string,int32>::iterator iterMap = fileList.begin();
			iterMap != fileList.end(); iterMap++)
		{
			addFileToSum(iterMap->first);
		}
	}
	return sum;
}

}}//end namespace
