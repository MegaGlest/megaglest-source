// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_XML_XMLPARSER_H_
#define _SHARED_XML_XMLPARSER_H_

#include <string>
#include <vector>
#include <xercesc/util/XercesDefs.hpp>
#include <map>
#include "leak_dumper.h"

using namespace std;

namespace XERCES_CPP_NAMESPACE{
	class DOMImplementation;
	class DOMDocument;
	class DOMNode;
	class DOMElement;
}

namespace Shared{ namespace Xml{

const int strSize= 4096;

class XmlIo;
class XmlTree;
class XmlNode;
class XmlAttribute;

// =====================================================
// 	class XmlIo
//
///	Wrapper for Xerces C++
// =====================================================

class XmlIo{
private:
	static bool initialized;
	XERCES_CPP_NAMESPACE::DOMImplementation *implementation;

private:
	XmlIo();

public:
	static XmlIo &getInstance();
	~XmlIo();
	XmlNode *load(const string &path, std::map<string,string> mapTagReplacementValues);
	void save(const string &path, const XmlNode *node);
};

// =====================================================
//	class XmlTree
// =====================================================

class XmlTree{
private:
	XmlNode *rootNode;

private:
	XmlTree(XmlTree&);
	void operator =(XmlTree&);

public:
	XmlTree();
	~XmlTree();

	void init(const string &name);
	void load(const string &path, std::map<string,string> mapTagReplacementValues);
	void save(const string &path);

	XmlNode *getRootNode() const	{return rootNode;}
};

// =====================================================
//	class XmlNode
// =====================================================

class XmlNode {
private:
	string name;
	string text;
	vector<XmlNode*> children;
	vector<XmlAttribute*> attributes;

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

public:
	XmlNode(XERCES_CPP_NAMESPACE::DOMNode *node, std::map<string,string> mapTagReplacementValues);
	XmlNode(const string &name);
	~XmlNode();

	const string &getName() const	{return name;}
	size_t getChildCount() const		{return children.size();}
	size_t getAttributeCount() const	{return attributes.size();}
	const string &getText() const	{return text;}

	XmlAttribute *getAttribute(unsigned int i) const;
	XmlAttribute *getAttribute(const string &name,bool mustExist=true) const;
	XmlNode *getChild(unsigned int i) const;
	XmlNode *getChild(const string &childName, unsigned int childIndex=0) const;
	vector<XmlNode *> getChildList(const string &childName) const;
	bool hasChildAtIndex(const string &childName, int childIndex=0) const;
	bool hasChild(const string &childName) const;
	XmlNode *getParent() const;


	XmlNode *addChild(const string &name);
	XmlAttribute *addAttribute(const string &name, const string &value, std::map<string,string> mapTagReplacementValues);

	XERCES_CPP_NAMESPACE::DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;

private:
	string getTreeString() const;
};

// =====================================================
//	class XmlAttribute
// =====================================================

class XmlAttribute {
private:
	string value;
	string name;
	bool skipRestrictionCheck;
	bool usesCommondata;
	std::map<string,string> mapTagReplacementValues;

private:
	XmlAttribute(XmlAttribute&);
	void operator =(XmlAttribute&);

public:
	XmlAttribute(XERCES_CPP_NAMESPACE::DOMNode *attribute, std::map<string,string> mapTagReplacementValues);
	XmlAttribute(const string &name, const string &value, std::map<string,string> mapTagReplacementValues);

public:
	const string getName() const	{return name;}
	const string getValue(string prefixValue="", bool trimValueWithStartingSlash=false) const;

	bool getBoolValue() const;
	int getIntValue() const;
	int getIntValue(int min, int max) const;
	float getFloatValue() const;
	float getFloatValue(float min, float max) const;
	const string getRestrictedValue(string prefixValue="", bool trimValueWithStartingSlash=false) const;
};


}}//end namespace

#endif
