// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009-2010 Titus Tscharntke (info@titusgames.de) and
//                          Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "simple_threads.h"
#include "util.h"
#include "platform_common.h"

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

FileCRCPreCacheThread::FileCRCPreCacheThread() : BaseThread() {
}

void FileCRCPreCacheThread::execute() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setRunningStatus(true);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FILE CRC PreCache thread is running\n");

	try	{
	    //tech Tree listBox
		vector<string> techPaths;
	    findDirs(techDataPaths, techPaths);
		if(techPaths.empty() == false) {
			for(int idx = 0; idx < techPaths.size(); idx++) {
				string &techPath = techPaths[idx];
				for(int idx2 = 0; idx2 < techPaths.size(); idx2++) {
					string techName = techPaths[idx2];

					printf("In [%s::%s Line: %d] caching CRC value for Tech [%s]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str());
					int32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + techName + string("/*"), ".xml", NULL);
					printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC);

					if(getQuitStatus() == true) {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					sleep( 100 );
				}
				if(getQuitStatus() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
			}
		}
	}
	catch(const exception &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		setRunningStatus(false);
	}
	catch(...) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
		setRunningStatus(false);
	}

	setRunningStatus(false);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FILE CRC PreCache thread is exiting\n");

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

}}//end namespace
