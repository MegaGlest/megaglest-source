// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2010 Martiño Figueroa and others
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef FILE_READER_H
#define FILE_READER_H

#include "platform_util.h"

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>

using std::map;
using std::string;
using std::vector;
using std::ifstream;
using std::ios;
using std::runtime_error;

using Shared::Platform::extractExtension;

namespace Shared{

// =====================================================
//	class FileReader
// =====================================================

template <class T>
class FileReader {
protected:
	string const * extensions;

	/**Creates a filereader being able to possibly load files
	 * from the specified extension
	 **/
	FileReader(string const * extensions);

public:
	/*Return the - existing and initialized - fileReadersMap
	 */
	static map<string, vector<FileReader<T> const * >* >& getFileReadersMap() {
		static map<string, vector<FileReader<T> const * >* > fileReaderByExtension;
		return fileReaderByExtension;
	}
	static vector<FileReader<T> const * > fileReaders;


public:

	/**Tries to read a file
	 * This method tries to read the file with the specified filepath.
	 * If it fails, either <code>null</code> is returned or an exception
         * is thrown*/
	static T* readPath(const string& filepath);

	/**Tries to read a file from an object
	 * This method tries to read the file with the specified filepath.
	 * If it fails, either <code>null</code> is returned or an exception
         * is thrown*/
	static T* readPath(const string& filepath, T* object);

	/**Gives a quick estimation of whether the specified file
         * can be read or not depending on the filename*/
	virtual bool canRead(const string& filepath) const;

	/**Gives a better estimation of whether the specified file
	 * can be read or not depending on the file content*/
	virtual bool canRead(ifstream& file) const;

	/**Reads a file
	 * This method tries to read the file with the specified filepath
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown
         */
	virtual T* read(const string& filepath) const;

	/**Reads a file to an object
	 * This method tries to read the file with the specified filepath
	 * If it fails, either <code>null</code> is returned or an exception
	 * is thrown
         */
	virtual T* read(const string& filepath, T* object) const;

	/**Reads a file
	 * This method tries to read the specified file
	 * If it failes, either <code>null</code> is returned or an exception
	 * Default implementation generates an object using T()
	 * is thrown
	 */
	virtual T* read(ifstream& file, const string& path) const {
		T* obj = new T();
		T* ret = read(file,path,obj);
		if (obj != ret) {
			delete obj;
		}
		return ret;
	}


	/**Reads a file onto the specified object
	 * This method tries to read the specified file
	 * If it failes, either <code>null</code> is returned or an exception
	 * is thrown
	 */
	virtual T* read(ifstream& file, const string& path, T* former) const = 0;

	virtual ~FileReader() {
		/*for (typename vector<FileReader<T> const * >::const_iterator i = fileReaders.begin(); i != fileReaders.end(); ++i) {
			delete const_cast<FileReader<T>* >(*i); //Segfault
		}*/
	}; //Well ... these objects aren't supposed to be destroyed
};

template <typename T>
static inline T* readFromFileReaders(vector<FileReader<T> const *>* readers, const string& filepath) {
	for (typename vector<FileReader<T> const *>::const_iterator i = readers->begin(); i != readers->end(); ++i) {
		try {
			FileReader<T> const * reader = *i;		
			T* ret = reader->read(filepath); //It is guaranteed that at least the filepath matches ...
			if (ret != NULL) {
				return ret;
			}
		} catch (...) { //TODO: Specific exceptions
		}
	}
	return NULL;
}

template <typename T>
static inline T* readFromFileReaders(vector<FileReader<T> const *>* readers, const string& filepath, T* object) {
	for (typename vector<FileReader<T> const *>::const_iterator i = readers->begin(); i != readers->end(); ++i) {
		try {
			FileReader<T> const * reader = *i;
			T* ret = reader->read(filepath, object); //It is guaranteed that at least the filepath matches ...
			if (ret != NULL) {
				return ret;
			}
		} catch (...) { //TODO: Specific exceptions
		}
	}
	std::cerr << "Could not parse filepath: " << filepath << std::endl;
	return NULL;
}

/**Tries to read a file
 * This method tries to read the file with the specified filepath.
 * If it fails, either <code>null</code> is returned or an exception
 * is thrown*/
template <typename T>
T* FileReader<T>::readPath(const string& filepath) {
	const string& extension = extractExtension(filepath);
	vector<FileReader<T> const * >* possibleReaders = (getFileReadersMap())[extension];
	if (possibleReaders != NULL) {
		//Search in these possible readers
		T* ret = readFromFileReaders(possibleReaders, filepath);
		if (ret != NULL) {
			return ret;
		}
	}
	T* ret = readFromFileReaders(&fileReaders, filepath); //Try all other
	return ret;
}

/**Tries to read a file
 * This method tries to read the file with the specified filepath.
 * If it fails, either <code>null</code> is returned or an exception
 * is thrown*/
template <typename T>
T* FileReader<T>::readPath(const string& filepath, T* object) {
	const string& extension = extractExtension(filepath);
	vector<FileReader<T> const * >* possibleReaders = (getFileReadersMap())[extension];
	if (possibleReaders != NULL) {
		//Search in these possible readers
		T* ret = readFromFileReaders(possibleReaders, filepath, object);
		if (ret != NULL) {
			return ret;
		}
	}
	T* ret = readFromFileReaders(&fileReaders, filepath, object); //Try all other
	return ret;
}


template<typename T>
vector<FileReader<T> const * > FileReader<T>::fileReaders;

template <typename T>
FileReader<T>::FileReader(string const * extensions): extensions(extensions) {
	fileReaders.push_back(this);
	string const * nextExtension = extensions;
	while (((*nextExtension) != "")) {
		vector<FileReader<T> const* >* curPossibleReaders = (getFileReadersMap())[*nextExtension];
		if (curPossibleReaders == NULL) {
			(getFileReadersMap())[*nextExtension] = (curPossibleReaders = new vector<FileReader<T> const *>());
		}
		curPossibleReaders->push_back(this);
		++nextExtension;
	}
}



/**Gives a quick estimation of whether the specified file
 * can be read or not depending on the filename*/
template <typename T>
bool FileReader<T>::canRead(const string& filepath) const {
	const string& realExtension = extractExtension(filepath);
	const string* haveExtension = extensions;
	while (*haveExtension != "") {
		if (realExtension == *haveExtension) {
			return true;
		}
		++haveExtension;
	}
	return false;
}

/**Gives a better estimation of whether the specified file
 * can be read or not depending on the file content*/
template <typename T>
bool FileReader<T>::canRead(ifstream& file) const {
	try {
		T* wouldRead = read(file,"unknown file");
		bool ret = (wouldRead != NULL);
		delete wouldRead;
		return ret;
	} catch (...) {
		return false;
	}
}

/**Reads a file
 * This method tries to read the file with the specified filepath
 * If it fails, either <code>null</code> is returned or an exception
 * is thrown
 */
template <typename T>
T* FileReader<T>::read(const string& filepath) const {
	ifstream file(filepath.c_str(), ios::in | ios::binary);
	if (!file.is_open()) { //An error occured; TODO: Which one - throw an exception, print error message?
		throw runtime_error("Could not open file " + filepath);
	}
	return read(file,filepath);
}

/**Reads a file
 * This method tries to read the file with the specified filepath
 * If it fails, either <code>null</code> is returned or an exception
 * is thrown
 */
template <typename T>
T* FileReader<T>::read(const string& filepath, T* object) const {
	ifstream file(filepath.c_str(), ios::in | ios::binary);
	if (!file.is_open()) { //An error occured; TODO: Which one - throw an exception, print error message?
		throw runtime_error("Could not open file " + filepath);
	}
	read(file,filepath,object);
}


} //end namespace

#endif
