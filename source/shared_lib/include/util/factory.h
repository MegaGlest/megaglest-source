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

#ifndef _SHARED_UTIL_FACTORY_
#define _SHARED_UTIL_FACTORY_

#include <map>
#include <string>
#include <stdexcept>

using std::map;
using std::string;
using std::pair;
using std::runtime_error;

namespace Shared{ namespace Util{

// =====================================================
//	class SingleFactoryBase
// =====================================================

class SingleFactoryBase{
public:
	virtual ~SingleFactoryBase(){}
	virtual void *newInstance()= 0;
};

// =====================================================
//	class SingleFactory
// =====================================================

template<typename T>
class SingleFactory: public SingleFactoryBase{
public:
	virtual void *newInstance()	{return new T();}
};

// =====================================================
//	class MultiFactory
// =====================================================

template<typename T>
class MultiFactory{
private:
	typedef map<string, SingleFactoryBase*> Factories;
	typedef pair<string, SingleFactoryBase*> FactoryPair;

private:
	Factories factories;

public:
	virtual ~MultiFactory(){
		for(Factories::iterator it= factories.begin(); it!=factories.end(); ++it){
			delete it->second;
		}
	}

	template<typename R>
	void registerClass(string classId){
		factories.insert(FactoryPair(classId, new SingleFactory<R>()));
	}

	T *newInstance(string classId){
		Factories::iterator it= factories.find(classId);
		if(it == factories.end()){
			throw runtime_error("Unknown class identifier: " + classId);
		}
		return static_cast<T*>(it->second->newInstance());
	}

	bool isClassId(string classId){
		return factories.find(classId)!=factories.end();
	}
};

}}//end namespace

#endif
