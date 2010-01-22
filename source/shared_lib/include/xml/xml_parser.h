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

#ifndef _SHARED_XML_XMLPARSER_H_
#define _SHARED_XML_XMLPARSER_H_

#include <string>
#include <vector>

#include <xercesc/util/XercesDefs.hpp>

using std::string;
using std::vector;

namespace XERCES_CPP_NAMESPACE{
	class DOMImplementation;
	class DOMDocument;
	class DOMNode;
	class DOMElement;
}

namespace Shared{ namespace Xml{

const int strSize= 256;

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
	XmlNode *load(const string &path);
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
	void load(const string &path);
	void save(const string &path);
	
	XmlNode *getRootNode() const	{return rootNode;}
};

// =====================================================
//	class XmlNode
// =====================================================

class XmlNode{
private:
	string name;
	string text;
	vector<XmlNode*> children;
	vector<XmlAttribute*> attributes; 

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

public:
	XmlNode(XERCES_CPP_NAMESPACE::DOMNode *node);
	XmlNode(const string &name);
	~XmlNode();

	const string &getName() const	{return name;}
	int getChildCount() const		{return children.size();}
	int getAttributeCount() const	{return attributes.size();}
	const string &getText() const	{return text;}

	XmlAttribute *getAttribute(int i) const;
	XmlAttribute *getAttribute(const string &name) const;
	XmlNode *getChild(int i) const;
	XmlNode *getChild(const string &childName, int childIndex=0) const;
	XmlNode *getParent() const;


	XmlNode *addChild(const string &name);
	XmlAttribute *addAttribute(const string &name, const string &value);

	XERCES_CPP_NAMESPACE::DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;

private:
	string getTreeString() const;
};

// =====================================================
//	class XmlAttribute
// =====================================================

class XmlAttribute{
private:
	string value;
	string name;

private:
	XmlAttribute(XmlAttribute&);
	void operator =(XmlAttribute&);

public:
	XmlAttribute(XERCES_CPP_NAMESPACE::DOMNode *attribute);
	XmlAttribute(const string &name, const string &value);

public:
	const string &getName() const	{return name;}
	const string &getValue() const	{return value;}

	bool getBoolValue() const;
	int getIntValue() const;		
	int getIntValue(int min, int max) const;
	float getFloatValue() const;		
	float getFloatValue(float min, float max) const;
	const string &getRestrictedValue() const;
};


}}//end namespace

#endif
