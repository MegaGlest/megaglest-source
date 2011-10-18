// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// DataManager.inl -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
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
