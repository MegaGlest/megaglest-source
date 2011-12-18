// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// DataManager.inl -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licensed under the MIT license: 
// http://www.opensource.org/licenses/mit-license.php
//
// Open Source Initiative OSI - The MIT License (MIT):Licensing
//
// The MIT License (MIT)
// Copyright (c) <year> <copyright holders>
//
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE.//
//
// Implementation of the data manager.
//
/////////////////////////////////////////////////////////////////////////////

#include "DataManager.h"

/////////////////////////////////////////////////////////////////////////////
//
// class DataManager implementation.
//
/////////////////////////////////////////////////////////////////////////////

// Singleton initialization.  At first, there is no object created.
template <typename T, typename C>
C *DataManager<T, C>::_singleton = NULL;

// --------------------------------------------------------------------------
// DataManager::DataManager
//
// Constructor.
// --------------------------------------------------------------------------

template <typename T, typename C>
inline
DataManager<T, C>::DataManager () {
}

// --------------------------------------------------------------------------
// DataManager::~DataManager
//
// Destructor.  Purge all registred objects.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline
DataManager<T, C>::~DataManager () {
  purge ();
}

// --------------------------------------------------------------------------
// DataManager::request
//
// Retrieve an object from the registry.  Return NULL if there if the
// requested object has not been found in the registry.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline T *
DataManager<T, C>::request (const string &name) {
  typename DataMap::iterator itor;
  itor = _registry.find (name);

  if (itor != _registry.end ())
    {
      // The object has been found
      return itor->second;
    }
  else
    {
      return NULL;
    }
}


// --------------------------------------------------------------------------
// DataManager::registerObject
//
// Register an object.  If kOverWrite is set, then it will overwrite
// the already existing object.  If kOverWrite is combined
// with kDelete, then it will also delete the previous object from memory.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline void
DataManager<T, C>::registerObject (const string &name, T *object)
  throw (DataManagerException) {
  std::pair<typename DataMap::iterator, bool> res;

  // Register the object as a new entry
  res = _registry.insert (typename DataMap::value_type (name, object));

  // Throw an exception if the insertion failed
  if (!res.second)
    throw DataManagerException ("Name collision", name);
}

// --------------------------------------------------------------------------
// DataManager::unregisterObject
//
// Unregister an object given its name.  If deleteObject is true,
// then it delete the object, otherwise it just remove the object
// from the registry whitout freeing it from memory.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline void
DataManager<T, C>::unregisterObject (const string &name, bool deleteObject) {
  typename DataMap::iterator itor;
  itor = _registry.find (name);

  if (itor != _registry.end ())
    {
      if (deleteObject)
	delete itor->second;

      _registry.erase (itor);
    }
}

// --------------------------------------------------------------------------
// DataManager::purge
//
// Destroy all registred objects and clear the registry.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline void
DataManager<T, C>::purge () {
  // Not exception safe!
  for (typename DataMap::iterator itor = _registry.begin ();
       itor != _registry.end (); ++itor)
    {
      // Destroy object
      delete itor->second;
    }

  _registry.clear ();
}

// --------------------------------------------------------------------------
// DataManager::getInstance
//
// Return a pointer of the unique instance of this class. If there is no
// object build yet, create it.
// NOTE: This is the only way to get access to the data manager since
// constructor is private.
// --------------------------------------------------------------------------
template <typename T, typename C>
inline C *
DataManager<T, C>::getInstance () {
  if (_singleton == NULL)
    _singleton = new C;

  return _singleton;
}

// --------------------------------------------------------------------------
// DataManager::kill
//
// Destroy the data manager, i.e. delete the unique instance of
// this class.
// NOTE: this function must be called before exiting in order to
// properly destroy all registred objects.
// --------------------------------------------------------------------------

template <typename T, typename C>
inline void
DataManager<T, C>::kill () {
  delete _singleton;
  _singleton = NULL;
}
