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
#include <map>
#include "rapidxml/rapidxml.hpp"
#include "data_types.h"
#include "leak_dumper.h"

using namespace rapidxml;
using namespace std;

namespace XERCES_CPP_NAMESPACE{
	class DOMImplementation;
	class DOMDocument;
	class DOMNode;
	class DOMElement;

#if XERCES_VERSION_MAJOR < 3
	class DOMBuilder;
#else
	class DOMLSParser;
#endif
}

XERCES_CPP_NAMESPACE_USE

namespace Shared { namespace Xml {

enum xml_engine_parser_type {
	XML_XERCES_ENGINE = 0,
	XML_RAPIDXML_ENGINE = 1
} ;

static xml_engine_parser_type DEFAULT_XML_ENGINE = XML_RAPIDXML_ENGINE;
const int strSize= 8094;

class XmlIo;
class XmlTree;
class XmlNode;
class XmlAttribute;

// =====================================================
// 	class XmlIo
//
///	Wrapper for Xerces C++
// =====================================================

class XmlIo {
private:
	static bool initialized;
	XERCES_CPP_NAMESPACE::DOMImplementation *implementation;
#if XERCES_VERSION_MAJOR < 3
	XERCES_CPP_NAMESPACE::DOMBuilder *parser;
#else
	XERCES_CPP_NAMESPACE::DOMLSParser *parser;
#endif

protected:
	XmlIo();
	virtual ~XmlIo();

	void init();

#if XERCES_VERSION_MAJOR < 3
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * getRootDOMDocument(const string &path, DOMBuilder *parser, bool noValidation);
#else
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * getRootDOMDocument(const string &path, DOMLSParser *parser, bool noValidation);
#endif

	DOMNode *loadDOMNode(const string &path, bool noValidation=false);
	virtual void releaseDOMParser();

public:
	static XmlIo &getInstance();

	static bool isInitialized();
	void cleanup();

	XmlNode *load(const string &path, const std::map<string,string> &mapTagReplacementValues,bool noValidation=false);
	void save(const string &path, const XmlNode *node);
};

class XmlIoRapid {
private:
	static bool initialized;
	xml_document<> *doc;

private:
	XmlIoRapid();
	void init();

public:
	static XmlIoRapid &getInstance();
	~XmlIoRapid();

	static bool isInitialized();
	void cleanup();

	XmlNode *load(const string &path, const std::map<string,string> &mapTagReplacementValues,bool noValidation=false,bool skipStackCheck=false);
	void save(const string &path, const XmlNode *node);
};

// =====================================================
//	class XmlTree
// =====================================================

class XmlTree{
private:
	XmlNode *rootNode;
	string loadPath;
	xml_engine_parser_type engine_type;
	bool skipStackCheck;
private:
	XmlTree(XmlTree&);
	void operator =(XmlTree&);
	void clearRootNode();

public:
	XmlTree(xml_engine_parser_type engine_type = DEFAULT_XML_ENGINE);
	~XmlTree();

	void init(const string &name);
	void load(const string &path, const std::map<string,string> &mapTagReplacementValues, bool noValidation=false,bool skipStackCheck=false);
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
	mutable const XmlNode* superNode;

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

	string getTreeString() const;
	bool hasChildNoSuper(const string& childName) const;

public:
	XmlNode(XERCES_CPP_NAMESPACE::DOMNode *node, const std::map<string,string> &mapTagReplacementValues);
	XmlNode(xml_node<> *node, const std::map<string,string> &mapTagReplacementValues);
	XmlNode(const string &name);
	~XmlNode();
	
	void setSuper(const XmlNode* superNode) const { this->superNode = superNode; }

	const string &getName() const	{return name;}
	size_t getChildCount() const		{return children.size();}
	size_t getAttributeCount() const	{return attributes.size();}
	const string &getText() const	{return text;}

	XmlAttribute *getAttribute(unsigned int i) const;
	XmlAttribute *getAttribute(const string &name,bool mustExist=true) const;
	bool hasAttribute(const string &name) const;

	XmlNode *getChild(unsigned int i) const;
	XmlNode *getChild(const string &childName, unsigned int childIndex=0) const;
	vector<XmlNode *> getChildList(const string &childName) const;
	bool hasChildAtIndex(const string &childName, int childIndex=0) const;
	bool hasChild(const string &childName) const;
	int clearChild(const string &childName);


	XmlNode *addChild(const string &name, const string text = "");
	XmlAttribute *addAttribute(const string &name, const string &value, const std::map<string,string> &mapTagReplacementValues);

	XERCES_CPP_NAMESPACE::DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;
	xml_node<>* buildElement(xml_document<> *document) const;
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
	XmlAttribute(XERCES_CPP_NAMESPACE::DOMNode *attribute, const std::map<string,string> &mapTagReplacementValues);
	XmlAttribute(xml_attribute<> *attribute, const std::map<string,string> &mapTagReplacementValues);
	XmlAttribute(const string &name, const string &value, const std::map<string,string> &mapTagReplacementValues);

public:
	const string getName() const	{return name;}
	const string getValue(string prefixValue="", bool trimValueWithStartingSlash=false) const;

	bool getBoolValue() const;
	int getIntValue() const;
	Shared::Platform::uint32 getUIntValue() const;
	int getIntValue(int min, int max) const;
	float getFloatValue() const;
	float getFloatValue(float min, float max) const;
	const string getRestrictedValue(string prefixValue="", bool trimValueWithStartingSlash=false) const;

	void setValue(string val);
};


}}//end namespace

#endif
