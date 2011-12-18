// ==============================================================
// This file is part of MegaGlest (www.glest.org)
//
// DataManager.h -- Copyright (c) 2006 David Henry
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
