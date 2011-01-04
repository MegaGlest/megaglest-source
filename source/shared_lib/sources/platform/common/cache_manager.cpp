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
#include "cache_manager.h"

namespace Shared { namespace PlatformCommon {

Mutex CacheManager::mutexCache;
const char *CacheManager::getFolderTreeContentsCheckSumRecursivelyCacheLookupKey1       = "CRC_Cache_FileTree1";
const char *CacheManager::getFolderTreeContentsCheckSumRecursivelyCacheLookupKey2       = "CRC_Cache_FileTree2";
const char *CacheManager::getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey1   = "CRC_Cache_FileTreeList1";
const char *CacheManager::getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey2   = "CRC_Cache_FileTreeList2";

}}//end namespace
