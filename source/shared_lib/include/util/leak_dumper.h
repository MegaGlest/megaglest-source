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

#ifndef _LEAKDUMPER_H_
#define _LEAKDUMPER_H_

//#define SL_LEAK_DUMP
//SL_LEAK_DUMP controls if leak dumping is enabled or not

#ifdef SL_LEAK_DUMP

#include <cstdlib>
#include <cstdio>

//including this header in any file of a project will cause all
//leaks to be dumped into leak_dump.txt, but only allocations that 
//ocurred in a file where this header is included will have
//file and line number

struct AllocInfo{
	int line;
	const char *file;
	size_t bytes;
	void *ptr;
	bool free;
	bool array;

	AllocInfo();
	AllocInfo(void* ptr, const char* file, int line, size_t bytes, bool array);
};

// =====================================================
//	class AllocRegistry
// =====================================================

class AllocRegistry{
private:
	static const unsigned maxAllocs= 40000;

private:
	AllocRegistry();

private:
	AllocInfo allocs[maxAllocs];	//array to store allocation info
	int allocCount;					//allocations
	size_t allocBytes;					//bytes allocated
	int nonMonitoredCount;			
	size_t nonMonitoredBytes;

public:
	~AllocRegistry();

	static AllocRegistry &getInstance();

	void allocate(AllocInfo info);
	void deallocate(void* ptr, bool array);
	void reset();
	void dump(const char *path);
};

//if an allocation ocurrs in a file where "leaks_dumper.h" is not included
//this operator new is called and file and line will be unknown
inline void * operator new (size_t bytes){
	void *ptr= malloc(bytes);
	AllocRegistry::getInstance().allocate(AllocInfo(ptr, "unknown", 0, bytes, false));
	return ptr;
}

inline void operator delete(void *ptr){
	AllocRegistry::getInstance().deallocate(ptr, false);
	free(ptr);
}

inline void * operator new[](size_t bytes){
	void *ptr= malloc(bytes);
	AllocRegistry::getInstance().allocate(AllocInfo(ptr, "unknown", 0, bytes, true));
	return ptr;
}

inline void operator delete [](void *ptr){
	AllocRegistry::getInstance().deallocate(ptr, true);
	free(ptr);
}

//if an allocation ocurrs in a file where "leaks_dumper.h" is included 
//this operator new is called and file and line will be known
inline void * operator new (size_t bytes, char* file, int line){
	void *ptr= malloc(bytes);
	AllocRegistry::getInstance().allocate(AllocInfo(ptr, file, line, bytes, false));
	return ptr;
}

inline void operator delete(void *ptr, char* file, int line){
	AllocRegistry::getInstance().deallocate(ptr, false);
	free(ptr);
}

inline void * operator new[](size_t bytes, char* file, int line){
	void *ptr= malloc(bytes);
	AllocRegistry::getInstance().allocate(AllocInfo(ptr, file, line, bytes, true));
	return ptr;
}

inline void operator delete [](void *ptr, char* file, int line){
	AllocRegistry::getInstance().deallocate(ptr, true);
	free(ptr);
}

#define new new(__FILE__, __LINE__)

#endif

#endif
