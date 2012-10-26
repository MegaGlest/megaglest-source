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
#include "platform_util.h"
#include <time.h>

bool AllocInfo::application_binary_initialized = false;
//static AllocRegistry memoryLeaks = AllocRegistry::getInstance();

// =====================================================
//	class AllocRegistry
// =====================================================

// ===================== PUBLIC ========================

AllocRegistry::~AllocRegistry() {
	dump("leak_dump.log");

	free(mutex);
	mutex = NULL;
}

void AllocRegistry::dump(const char *path) {
#ifdef WIN32
	FILE* f= _wfopen(utf8_decode(path).c_str(), L"wt");
#else
	FILE *f= fopen(path, "wt");
#endif
	int leakCount=0;
	size_t leakBytes=0;

	time_t debugTime = time(NULL);
    struct tm *loctime = localtime (&debugTime);
    char szBuf2[100]="";
    strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);

	fprintf(f, "Memory leak dump at: %s\n\n",szBuf2);

	for(int i=0; i<maxAllocs; ++i){
		AllocInfo &info = allocs[i];
		if(info.freetouse == false && info.inuse == true) {

			if(info.line > 0) {
				leakBytes += info.bytes;

				//allocs[i].stack = AllocInfo::getStackTrace();
				fprintf(f, "Leak #%d.\tfile: %s, line: %d, ptr [%p], bytes: %zu, array: %d, inuse: %d\n%s\n", ++leakCount, info.file, info.line, info.ptr, info.bytes, info.array,info.inuse,info.stack.c_str());
			}
		}
	}

	fprintf(f, "\nTotal leaks: %d, %zu bytes\n", leakCount, leakBytes);
	fprintf(f, "Total allocations: %d, %zu bytes\n", allocCount, allocBytes);
	fprintf(f, "Not monitored allocations: %d, %zu bytes\n", nonMonitoredCount, nonMonitoredBytes);

	fclose(f);

	printf("Memory leak dump summary at: %s\n",szBuf2);
	printf("Total leaks: %d, %zu bytes\n", leakCount, leakBytes);
	printf("Total allocations: %d, %zu bytes\n", allocCount, allocBytes);
	printf("Not monitored allocations: %d, %zu bytes\n", nonMonitoredCount, nonMonitoredBytes);
}

#endif
