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
#include "leak_dumper.h"

#ifdef SL_LEAK_DUMP
using Shared::Platform::MutexSafeWrapper;

bool AllocInfo::application_binary_initialized = false;
//static AllocRegistry memoryLeaks = AllocRegistry::getInstance();

// =====================================================
//	class AllocRegistry
// =====================================================

// ===================== PUBLIC ========================

AllocRegistry::~AllocRegistry() {
	dump("leak_dump.log");
}

void AllocRegistry::allocate(AllocInfo info) {
	//if(info.line == 0) return;

	MutexSafeWrapper safeMutexMasterList(mutex);
	//printf("ALLOCATE.\tfile: %s, line: %d, bytes: %lu, array: %d inuse: %d\n", info.file, info.line, info.bytes, info.array, info.inuse);

	if(info.line > 0) {
		++allocCount;
		allocBytes += info.bytes;
	}
	//bool found = false;
	for(int i = (nextFreeIndex >= 0 ? nextFreeIndex : 0); i < maxAllocs; ++i) {
		if(allocs[i].freetouse == true && allocs[i].inuse == false) {
			allocs[i]= info;
			nextFreeIndex=-1;
			//found = true;
			return;
		}
	}

	//if(found == false) {
		//printf("ALLOCATE NOT MONITORED\n");
		printf("ALLOCATE NOT MONITORED.\tfile: %s, line: %d, ptr [%p], bytes: %lu, array: %d inuse: %d, \n%s\n", info.file, info.line, info.ptr, info.bytes, info.array, info.inuse, info.stack.c_str());

		++nonMonitoredCount;
		nonMonitoredBytes+= info.bytes;
	//}
}

void AllocRegistry::deallocate(void* ptr, bool array,const char* file, int line) {
	//if(line == 0) return;

	MutexSafeWrapper safeMutexMasterList(mutex);

	//bool found = false;
	for(int i=0; i < maxAllocs; ++i) {
		if(allocs[i].freetouse == false && allocs[i].inuse == true &&
				allocs[i].ptr == ptr && allocs[i].array == array) {
			allocs[i].freetouse= true;
			allocs[i].inuse = false;
			nextFreeIndex=i;

			//found = true;
			//break;
			return;
		}
	}

	//if(found == false) {
		if(line > 0) printf("WARNING, possible error on pointer delete for ptr [%p] array = %d, file [%s] line: %d\n%s\n",ptr,array,file, line,AllocInfo::getStackTrace().c_str());
	//}
}

void AllocRegistry::dump(const char *path) {
#ifdef WIN32
	FILE* f= = _wfopen(utf8_decode(path).c_str(), L"wt");
#else
	FILE *f= fopen(path, "wt");
#endif
	int leakCount=0;
	size_t leakBytes=0;

	fprintf(f, "Memory leak dump\n\n");

	for(int i=0; i<maxAllocs; ++i){
		AllocInfo &info = allocs[i];
		if(info.freetouse == false && info.inuse == true) {

			if(info.line > 0) {
				leakBytes += info.bytes;

				//allocs[i].stack = AllocInfo::getStackTrace();
				fprintf(f, "Leak #%d.\tfile: %s, line: %d, ptr [%p], bytes: %lu, array: %d, inuse: %d\n%s", ++leakCount, info.file, info.line, info.ptr, info.bytes, info.array,info.inuse,info.stack.c_str());
			}
		}
	}

	fprintf(f, "\nTotal leaks: %d, %lu bytes\n", leakCount, leakBytes);
	fprintf(f, "Total allocations: %d, %lu bytes\n", allocCount, allocBytes);
	fprintf(f, "Not monitored allocations: %d, %lu bytes\n", nonMonitoredCount, nonMonitoredBytes);

	fclose(f);

	printf("Memory leak dump summary:\n");
	printf("Total leaks: %d, %lu bytes\n", leakCount, leakBytes);
	printf("Total allocations: %d, %lu bytes\n", allocCount, allocBytes);
	printf("Not monitored allocations: %d, %lu bytes\n", nonMonitoredCount, nonMonitoredBytes);
}

#endif
