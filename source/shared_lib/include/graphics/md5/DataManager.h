// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// DataManager.h -- Copyright (c) 2006 David Henry
// changed for use with MegaGlest: Copyright (C) 2011-  by Mark Vejvoda
//
// This code is licenced under the MIT license.
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that the copyright text
// below is included in the resulting source code.
//
// Definitions of a data manager class.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __DATAMANAGER_H__
#define __DATAMANAGER_H__

#include <stdexcept>
#include <string>
#include <map>

using std::string;
using std::map;

namespace Shared { namespace Graphics { namespace md5 {

/////////////////////////////////////////////////////////////////////////////
//
// class DataManagerException - Exception class for DataManager classes.
// This acts like a standard runtime_error exception but
// know the name of the resource which caused the exception.
//
/////////////////////////////////////////////////////////////////////////////
class DataManagerException : public std::runtime_error {
public:
  // Constructors
  DataManagerException (const string &error)
    : std::runtime_error (error) { }
  DataManagerException (const string &error, const string &name)
    : std::runtime_error (error), _which (name) { }
  virtual ~DataManagerException () throw () { }

public:
  // Public interface
  virtual const char *which () const throw () {
    return _which.c_str ();
  }

private:
  // Member variables
  string _which;
};

/////////////////////////////////////////////////////////////////////////////
//
// class DataManager -- a data manager which can register/unregister
// generic objects.  Destroy all registred objects at death.
//
// The data manager is a singleton.
//
/////////////////////////////////////////////////////////////////////////////
template <typename T, typename C>
class DataManager {
protected:
  // Constructor/destructor
  DataManager ();
  virtual ~DataManager ();

public:
  // Public interface
  T *request (const string &name);

  void registerObject (const string &name, T *object)
    throw (DataManagerException);
  void unregisterObject (const string &name, bool deleteObject = false);

  void purge ();

private:
  // Member variables
  typedef map<string, T*> DataMap;
  DataMap _registry;

public:
  // Singleton related functions
  static C *getInstance ();
  static void kill ();

private:
  // The unique instance of this class
  static C *_singleton;
};

// Include inline function definitions
#include "DataManager.inl"

}}} //end namespace

#endif // __DATAMANAGER_H__
