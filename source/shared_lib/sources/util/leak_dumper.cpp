// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "leak_dumper.h"

#ifdef SL_LEAK_DUMP

AllocInfo::AllocInfo()
		: ptr(0), file(""), line(-1), bytes(-1), array(false), free(false) {
}

AllocInfo::AllocInfo(void* ptr, const char* file, int line, size_t bytes, bool array)
		: ptr(ptr), file(file), line(line), bytes(bytes), array(array), free(false) {
}

// =====================================================
//	class AllocRegistry
// =====================================================

// ===================== PRIVATE =======================

AllocRegistry::AllocRegistry(){
	allocCount= 0;
	allocBytes= 0;
	nonMonitoredCount= 0;
	nonMonitoredBytes= 0;
}

// ===================== PUBLIC ========================

AllocRegistry &AllocRegistry::getInstance(){
	static AllocRegistry allocRegistry;
	return allocRegistry;
}

AllocRegistry::~AllocRegistry(){

	dump("leak_dump.log");
}

void AllocRegistry::allocate(AllocInfo info){
	++allocCount;
	allocBytes+= info.bytes;
	unsigned hashCode= reinterpret_cast<unsigned>(info.ptr) % maxAllocs;

	for(int i=hashCode; i<maxAllocs; ++i){
		if(allocs[i].free){
			allocs[i]= info;
			return;
		}
	}
	for(int i=0; i<hashCode; ++i){
		if(allocs[i].free){
			allocs[i]= info;
			return;
		}
	}
	++nonMonitoredCount;
	nonMonitoredBytes+= info.bytes;
}

void AllocRegistry::deallocate(void* ptr, bool array){
	unsigned hashCode= reinterpret_cast<unsigned>(ptr) % maxAllocs;

	for(int i=hashCode; i<maxAllocs; ++i){
		if(!allocs[i].free && allocs[i].ptr==ptr && allocs[i].array==array){
			allocs[i].free= true;
			return;
		}
	}
	for(int i=0; i<hashCode; ++i){
		if(!allocs[i].free && allocs[i].ptr==ptr && allocs[i].array==array){
			allocs[i].free= true;
			return;
		}
	}
}

void AllocRegistry::reset(){
	for(int i=0; i<maxAllocs; ++i){
		allocs[i]= AllocInfo();
	}
}

void AllocRegistry::dump(const char *path){
#ifdef WIN32
	FILE* f= = _wfopen(utf8_decode(path).c_str(), L"wt");
#else
	FILE *f= fopen(path, "wt");
#endif
	int leakCount=0;
	size_t leakBytes=0;

	fprintf(f, "Memory leak dump\n\n");

	for(int i=0; i<maxAllocs; ++i){
		if(!allocs[i].free){
			leakBytes+= allocs[i].bytes;
			fprintf(f, "%d.\tfile: %s, line: %d, bytes: %d, array: %d\n", ++leakCount, allocs[i].file, allocs[i].line, allocs[i].bytes, allocs[i].array);
		}
	}

	fprintf(f, "\nTotal leaks: %d, %d bytes\n", leakCount, leakBytes);
	fprintf(f, "Total allocations: %d, %d bytes\n", allocCount, allocBytes);
	fprintf(f, "Not monitored allocations: %d, %d bytes\n", nonMonitoredCount, nonMonitoredBytes);

	fclose(f);
}

#endif
