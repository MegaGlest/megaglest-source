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

#ifndef _SHARED_PLATFORMCOMMON_CACHEMANAGER_H_
#define _SHARED_PLATFORMCOMMON_CACHEMANAGER_H_

#include "thread.h"
#include <map>
#include <string>
#include <stdexcept>

using namespace std;
using namespace Shared::Platform;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class BaseThread
// =====================================================

class CacheManager
{
protected:
	static Mutex mutexCache;

	typedef enum {
		cacheItemGet,
		cacheItemSet
	} CacheAccessorType;

	template <typename T>
	static T & manageCachedItem(string cacheKey, T *value,CacheAccessorType accessor) {
		// Here is the actual type-safe instantiation
		static std::map<string, T > itemCache;
		if(accessor == cacheItemSet) {
			if(value == NULL) {
				try {
					MutexSafeWrapper safeMutex(&mutexCache);
					if(itemCache.find(cacheKey) != itemCache.end()) {
						itemCache.erase(cacheKey);
					}
					safeMutex.ReleaseLock();
				}
				catch(const std::exception &ex) {
					throw runtime_error(ex.what());
				}

			}
			try {
				MutexSafeWrapper safeMutex(&mutexCache);
				itemCache[cacheKey] = *value;
				safeMutex.ReleaseLock();
			}
			catch(const std::exception &ex) {
				throw runtime_error(ex.what());
			}
		}
		// If this is the first access we return a default object of the type
		return itemCache[cacheKey];
	}

public:

	CacheManager() { }
	~CacheManager() { }

	template <typename T>
	static void setCachedItem(string cacheKey, const T value) {
		manageCachedItem<T>(cacheKey,value,cacheItemSet);
	}
	template <typename T>
	static T & getCachedItem(string cacheKey) {
		return manageCachedItem<T>(cacheKey,NULL,cacheItemGet);
	}
	template <typename T>
	static void clearCachedItem(string cacheKey) {
		return manageCachedItem<T>(cacheKey,NULL,cacheItemSet);
	}

};

}}//end namespace

#endif
