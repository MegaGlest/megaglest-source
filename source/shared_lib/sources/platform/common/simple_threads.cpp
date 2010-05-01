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
			for(unsigned int idx = 0; idx < techPaths.size(); idx++) {
				string techName = techPaths[idx];

				printf("In [%s::%s Line: %d] caching CRC value for Tech [%s] [%d of %d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),idx+1,techPaths.size());
				int32 techCRC = getFolderTreeContentsCheckSumRecursively(techDataPaths, string("/") + techName + string("/*"), ".xml", NULL);
				printf("In [%s::%s Line: %d] cached CRC value for Tech [%s] is [%d] [%d of %d]\n",__FILE__,__FUNCTION__,__LINE__,techName.c_str(),techCRC,idx+1,techPaths.size());

				if(getQuitStatus() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
				sleep( 100 );
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


SimpleTaskThread::SimpleTaskThread(	SimpleTaskCallbackInterface *simpleTaskInterface, 
									unsigned int executionCount,
									unsigned int millisecsBetweenExecutions) {
	this->simpleTaskInterface		 = simpleTaskInterface;
	this->executionCount			 = executionCount;
	this->millisecsBetweenExecutions = millisecsBetweenExecutions;
}

void SimpleTaskThread::execute() {
	try {
		setRunningStatus(true);

		unsigned int idx = 0;
		for(;this->simpleTaskInterface != NULL;) {
			this->simpleTaskInterface->simpleTask();
			if(this->executionCount > 0) {
				idx++;
				if(idx >= this->executionCount) {
					break;
				}
			}
			if(getQuitStatus() == true) {
				//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}

			sleep(this->millisecsBetweenExecutions);
		}
	}
	catch(const exception &ex) {
		setRunningStatus(false);

		throw runtime_error(ex.what());
	}
	setRunningStatus(false);
}

}}//end namespace
