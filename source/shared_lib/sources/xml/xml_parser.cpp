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

#include "xml_parser.h"

#include <fstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "conversion.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "util.h"
#include "types.h"
#include "properties.h"
#include "platform_common.h"
#include "platform_util.h"
#include "cache_manager.h"

#include "rapidxml_print.hpp"
#include "leak_dumper.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Xml {

using namespace Util;

// =====================================================
//	class ErrorHandler
// =====================================================

class ErrorHandler: public DOMErrorHandler{
public:
	virtual bool handleError (const DOMError &domError){
		if(domError.getSeverity()== DOMError::DOM_SEVERITY_FATAL_ERROR){
			char msgStr[strSize], fileStr[strSize];
			XMLString::transcode(domError.getMessage(), msgStr, strSize-1);
			XMLString::transcode(domError.getLocation()->getURI(), fileStr, strSize-1);
			Shared::Platform::uint64 lineNumber= domError.getLocation()->getLineNumber();
			throw runtime_error("Error parsing XML, file: " + string(fileStr) + ", line: " + intToStr(lineNumber) + ": " + string(msgStr));
		}
		return true;
	}
};

// =====================================================
//	class XmlIo
// =====================================================

bool XmlIo::initialized= false;
bool XmlIoRapid::initialized= false;

XmlIo::XmlIo() {
	try{
		//printf("XmlIo init\n");
		XMLPlatformUtils::Initialize();

		XmlIo::initialized= true;
	}
	catch(const XMLException &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error initializing XML system, msg %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.getMessage());
		throw runtime_error("Error initializing XML system");
	}

	try {
        XMLCh str[strSize];
        XMLString::transcode("LS", str, strSize-1);

		implementation = DOMImplementationRegistry::getDOMImplementation(str);
	}
	catch(const DOMException &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while creating XML parser, msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.getMessage());
		throw runtime_error("Exception while creating XML parser");
	}
}

XmlIo &XmlIo::getInstance() {
	static XmlIo XmlIo;
	return XmlIo;
}

void XmlIo::cleanup() {
	if(XmlIo::initialized == true) {
		XmlIo::initialized= false;
		//printf("XmlIo cleanup\n");
		XMLPlatformUtils::Terminate();
	}
}

XmlIo::~XmlIo() {
	cleanup();
}

XmlNode *XmlIo::load(const string &path, std::map<string,string> mapTagReplacementValues,bool noValidation) {
	//printf("Load file using Xerces engine [%s]\n",path.c_str());

	try {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("XERCES_FULLVERSIONDOT [%s]\nnoValidation = %d\npath [%s]\n",XERCES_FULLVERSIONDOT,noValidation,path.c_str());

		ErrorHandler errorHandler;
#if XERCES_VERSION_MAJOR < 3
 		DOMBuilder *parser= (static_cast<DOMImplementationLS*>(implementation))->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
 		parser->setErrorHandler(&errorHandler);
 		if(noValidation == false) {
			parser->setFeature(XMLUni::fgXercesSchema, true);
			parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
			//parser->setFeature(XMLUni::fgDOMValidateIfSchema, true);
			parser->setFeature(XMLUni::fgDOMValidation, true);
 		}
 		else {
			//parser->setFeature(XMLUni::fgSAX2CoreValidation, false);
			//parser->setFeature(XMLUni::fgXercesDynamic, true);

			//parser->setFeature(XMLUni::fgXercesSchema, false);
			//parser->setFeature(XMLUni::fgXercesSchemaFullChecking, false);
			//parser->setFeature(XMLUni::fgDOMValidateIfSchema, true);
			//parser->setFeature(XMLUni::fgDOMValidation, false);

 			parser->setFeature(XMLUni::fgXercesSchemaFullChecking, false);
 			parser->setFeature(XMLUni::fgXercesLoadExternalDTD, false);
 			parser->setFeature(XMLUni::fgXercesCacheGrammarFromParse, true);
 			parser->setFeature(XMLUni::fgXercesUseCachedGrammarInParse, true);
 		}
#else
		DOMLSParser *parser = (static_cast<DOMImplementationLS*>(implementation))->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
 		DOMConfiguration  *config = parser->getDomConfig();
 		if(noValidation == false) {
			config->setParameter(XMLUni::fgXercesSchema, true);
			config->setParameter(XMLUni::fgXercesSchemaFullChecking, true);
			//config->setParameter(XMLUni::fgDOMValidateIfSchema, true);
			config->setParameter(XMLUni::fgDOMValidate, true);
 		}
 		else {
			config->setParameter(XMLUni::fgXercesSchema, false);
			config->setParameter(XMLUni::fgXercesSchemaFullChecking, false);
			config->setParameter(XMLUni::fgXercesLoadExternalDTD, false);
			config->setParameter(XMLUni::fgXercesCacheGrammarFromParse, true);
			config->setParameter(XMLUni::fgXercesUseCachedGrammarInParse, true);
 		}
#endif
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *document= parser->parseURI(path.c_str());
#ifdef WIN32
		if(document == NULL) {
			document= parser->parseURI(utf8_decode(path).c_str());
		}
#endif
		if(document == NULL) {
			throw runtime_error("Can not parse URL: " + path);
		}

		XmlNode *rootNode= new XmlNode(document->getDocumentElement(),mapTagReplacementValues);
		parser->release();
		return rootNode;
	}
	catch(const DOMException &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Exception while loading: [%s], msg:\n%s",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),XMLString::transcode(ex.msg));
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);

		throw runtime_error(szBuf);
	}
}

void XmlIo::save(const string &path, const XmlNode *node){
	//printf("Saving file using Xerces engine [%s]\n",path.c_str());

	try{
		XMLCh str[strSize];
		XMLString::transcode(node->getName().c_str(), str, strSize-1);

		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *document= implementation->createDocument(0, str, 0);
		DOMElement *documentElement= document->getDocumentElement();
		for(unsigned int i = 0; i < node->getAttributeCount() ; ++i){
			XmlAttribute *attr = node->getAttribute(i);

			XMLCh strName[strSize];
			XMLString::transcode(attr->getName().c_str(), strName, strSize-1);
			XMLCh strValue[strSize];
			XMLString::transcode(attr->getValue("",false).c_str(), strValue, strSize-1);

			documentElement->setAttribute(strName,strValue);
		}

		for(unsigned int i=0; i<node->getChildCount(); ++i){
			documentElement->appendChild(node->getChild(i)->buildElement(document));
		}

		LocalFileFormatTarget file(path.c_str());
#if XERCES_VERSION_MAJOR < 3
 		DOMWriter* writer = implementation->createDOMWriter();
 		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
 		writer->writeNode(&file, *document);
#else
		DOMLSSerializer *serializer = implementation->createLSSerializer();
		DOMLSOutput* output=implementation->createLSOutput();
		DOMConfiguration* config=serializer->getDomConfig();
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint,true);
		output->setByteStream(&file);
		serializer->write(document,output);
		output->release();
		serializer->release();
#endif
		document->release();
	}
	catch(const DOMException &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while saving: [%s], %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),XMLString::transcode(e.msg));
		throw runtime_error("Exception while saving: " + path + ": " + XMLString::transcode(e.msg));
	}
}

// =====================================================
//	class XmlIoRapid
// =====================================================
XmlIoRapid::XmlIoRapid() {
	try{
		//printf("XmlIo init\n");

		XmlIoRapid::initialized= true;
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error initializing XML system, msg %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error initializing XML system");
	}

	try {
		doc = new xml_document<>();
	}
	catch(const DOMException &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while creating XML parser, msg: %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,ex.getMessage());
		throw runtime_error("Exception while creating XML parser");
	}
}

XmlIoRapid &XmlIoRapid::getInstance() {
	static XmlIoRapid io;
	return io;
}

void XmlIoRapid::cleanup() {
	if(XmlIoRapid::initialized == true) {
		XmlIoRapid::initialized= false;
		//printf("XmlIo cleanup\n");
		delete doc;
		doc = NULL;
	}
}

XmlIoRapid::~XmlIoRapid() {
	cleanup();
}

XmlNode *XmlIoRapid::load(const string &path, std::map<string,string> mapTagReplacementValues,bool noValidation) {
	Chrono chrono;
	chrono.start();
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Using RapidXml to load file [%s]\n",path.c_str());

	XmlNode *rootNode = NULL;
	try {
#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(path).c_str(), L"rb");
		ifstream xmlFile(fp);
#else
		ifstream xmlFile(path.c_str());
#endif
		if(xmlFile.is_open() == false) {
			throw runtime_error("Can not open file: [" + path + "]");
		}

		// read file into input_xml
		string inputXml = "";
		string line = "";
		while(getline(xmlFile,line)) {
			inputXml += line;
		}
		// make a safe-to-modify copy of input_xml
		// (you should never modify the contents of an std::string directly)
		vector<char> buffer(inputXml.begin(), inputXml.end());
		buffer.push_back('\0');

		/* "Read file into vector<char>"  See linked thread above*/
		//vector<char> buffer((istreambuf_iterator<char>(xmlFile)), istreambuf_iterator<char>( ));
	    //buffer.push_back('\0');
	    doc->parse<parse_no_data_nodes>(&buffer[0]);

		rootNode= new XmlNode(doc->first_node(),mapTagReplacementValues);
		//parser->release();

#if defined(WIN32) && !defined(__MINGW32__)
		if(fp) {
			fclose(fp);
		}
#endif
	}
	catch(const DOMException &ex) {
		char szBuf[8096]="";
		sprintf(szBuf,"In [%s::%s Line: %d] Exception while loading: [%s], msg:\n%s",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),XMLString::transcode(ex.msg));
		SystemFlags::OutputDebug(SystemFlags::debugError,"%s\n",szBuf);

		throw runtime_error(szBuf);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] took msecs: %ld\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,chrono.getMillis());

	return rootNode;
}

void XmlIoRapid::save(const string &path, const XmlNode *node){
	try {
		xml_document<> doc;

		// xml declaration
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute(doc.allocate_string("version"), doc.allocate_string("1.0")));
		decl->append_attribute(doc.allocate_attribute(doc.allocate_string("encoding"), doc.allocate_string("utf-8")));
		decl->append_attribute(doc.allocate_attribute(doc.allocate_string("standalone"), doc.allocate_string("no")));
		doc.append_node(decl);

		// root node
		xml_node<>* root = doc.allocate_node(node_element, doc.allocate_string(node->getName().c_str()));
		for(unsigned int i = 0; i < node->getAttributeCount() ; ++i){
			XmlAttribute *attr = node->getAttribute(i);
			root->append_attribute(doc.allocate_attribute(
					doc.allocate_string(attr->getName().c_str()),
					doc.allocate_string(attr->getValue("",false).c_str())));
		}
		doc.append_node(root);

		// child nodes
		for(unsigned int i = 0; i < node->getChildCount(); ++i) {
			root->append_node(node->getChild(i)->buildElement(&doc));
		}

//		std::string xml_as_string;
//		// watch for name collisions here, print() is a very common function name!
//		print(std::back_inserter(xml_as_string), doc);
//		// xml_as_string now contains the XML in string form, indented
//		// (in all its angle bracket glory)
//
//		std::string xml_no_indent;
//		// print_no_indenting is the only flag that print() knows about
//		print(std::back_inserter(xml_no_indent), doc, print_no_indenting);
//		// xml_no_indent now contains non-indented XML

#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(path).c_str(), L"wt");
		ofstream xmlFile(fp);
#else
		ofstream xmlFile(path.c_str());
#endif
		if(xmlFile.is_open() == false) {
			throw runtime_error("Can not open file: [" + path + "]");
		}

		//xmlFile << xml_no_indent;
//		xmlFile << xml_as_string << '\0';
		xmlFile << doc;

#if defined(WIN32) && !defined(__MINGW32__)
		if(fp) {
			fclose(fp);
		}
#endif
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while saving: [%s], %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),e.what());
		throw runtime_error("Exception while saving [" + path + "] msg: " + e.what());
	}
}

// =====================================================
//	class XmlTree
// =====================================================
XmlTree::XmlTree(xml_engine_parser_type engine_type) {
	rootNode= NULL;
	this->engine_type = engine_type;
}

void XmlTree::init(const string &name){
	this->rootNode= new XmlNode(name);
}

typedef std::vector<XmlTree*> LoadStack;
//static LoadStack loadStack;
static string loadStackCacheName = string(__FILE__) + string("_loadStackCacheName");

void XmlTree::load(const string &path, std::map<string,string> mapTagReplacementValues, bool noValidation) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] about to load [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());

	//printf("XmlTree::load p [%p]\n",this);
	assert(!loadPath.size());

	LoadStack &loadStack = CacheManager::getCachedItem<LoadStack>(loadStackCacheName);
	Mutex &mutex = CacheManager::getMutexForItem<LoadStack>(loadStackCacheName);
	MutexSafeWrapper safeMutex(&mutex);

	for(LoadStack::iterator it= loadStack.begin(); it!= loadStack.end(); ++it){
		if((*it)->loadPath == path){
			throw runtime_error(path + " recursively included");
		}
	}
	loadStack.push_back(this);
	safeMutex.ReleaseLock();

	loadPath = path;
	if(this->engine_type == XML_XERCES_ENGINE) {
		this->rootNode= XmlIo::getInstance().load(path, mapTagReplacementValues, noValidation);
	}
	else {
		this->rootNode= XmlIoRapid::getInstance().load(path, mapTagReplacementValues, noValidation);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] about to load [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str());
}

void XmlTree::save(const string &path){
	if(this->engine_type == XML_XERCES_ENGINE) {
		XmlIo::getInstance().save(path, rootNode);
	}
	else {
		XmlIoRapid::getInstance().save(path, rootNode);
	}
}

XmlTree::~XmlTree() {
	//printf("XmlTree::~XmlTree p [%p]\n",this);

	LoadStack &loadStack = CacheManager::getCachedItem<LoadStack>(loadStackCacheName);
	Mutex &mutex = CacheManager::getMutexForItem<LoadStack>(loadStackCacheName);
	MutexSafeWrapper safeMutex(&mutex);

	LoadStack::iterator it= find(loadStack.begin(),loadStack.end(),this);
	if(it != loadStack.end()) {
		loadStack.erase(it);
	}
	safeMutex.ReleaseLock();

	delete rootNode;
}

// =====================================================
//	class XmlNode
// =====================================================

XmlNode::XmlNode(DOMNode *node, std::map<string,string> mapTagReplacementValues): superNode(NULL) {
    if(node == NULL || node->getNodeName() == NULL) {
        throw runtime_error("XML structure seems to be corrupt!");
    }

	//get name
	char str[strSize]="";
	XMLString::transcode(node->getNodeName(), str, strSize-1);
	name= str;

	//check document
	if(node->getNodeType() == DOMNode::DOCUMENT_NODE) {
		name="document";
	}

	//check children
	if(node->getChildNodes() != NULL) {
        for(unsigned int i = 0; i < node->getChildNodes()->getLength(); ++i) {
            DOMNode *currentNode= node->getChildNodes()->item(i);
            if(currentNode != NULL && currentNode->getNodeType()==DOMNode::ELEMENT_NODE){
                XmlNode *xmlNode= new XmlNode(currentNode, mapTagReplacementValues);
                children.push_back(xmlNode);
            }
        }
	}

	//check attributes
	DOMNamedNodeMap *domAttributes= node->getAttributes();
	if(domAttributes != NULL) {
		for(unsigned int i = 0; i < domAttributes->getLength(); ++i) {
			DOMNode *currentNode= domAttributes->item(i);
			if(currentNode->getNodeType() == DOMNode::ATTRIBUTE_NODE) {
				XmlAttribute *xmlAttribute= new XmlAttribute(domAttributes->item(i), mapTagReplacementValues);
				attributes.push_back(xmlAttribute);
			}
		}
	}

	//get value
	if(node->getNodeType() == DOMNode::ELEMENT_NODE && children.size() == 0) {
		char *textStr= XMLString::transcode(node->getTextContent());
		text= textStr;
		//Properties::applyTagsToValue(this->text);
		XMLString::release(&textStr);
	}
}

XmlNode::XmlNode(xml_node<> *node, std::map<string,string> mapTagReplacementValues) : superNode(NULL) {
	if(node == NULL || node->name() == NULL) {
        throw runtime_error("XML structure seems to be corrupt!");
    }

	//get name
	name = node->name();

	//check document
	if(node->type() == node_document) {
		name="document";
	}

	//printf("Found XML Node [%s]\n",name.c_str());

	//check children
	for(xml_node<> *currentNode = node->first_node();
			currentNode; currentNode = currentNode->next_sibling()) {
		if(currentNode != NULL && currentNode->type() == node_element) {
			XmlNode *xmlNode= new XmlNode(currentNode, mapTagReplacementValues);
			children.push_back(xmlNode);
		}
    }

	//check attributes
	for (xml_attribute<> *attr = node->first_attribute();
			attr; attr = attr->next_attribute()) {
		XmlAttribute *xmlAttribute= new XmlAttribute(attr, mapTagReplacementValues);
		attributes.push_back(xmlAttribute);
	}

	//get value
	if(node->type() == node_element && children.size() == 0) {
		text = node->value();
		Properties::applyTagsToValue(this->text);
	}
}

XmlNode::XmlNode(const string &name): superNode(NULL) {
	this->name= name;
}

XmlNode::~XmlNode() {
	for(unsigned int i=0; i<children.size(); ++i) {
		delete children[i];
	}
	for(unsigned int i=0; i<attributes.size(); ++i) {
		delete attributes[i];
	}
}

XmlAttribute *XmlNode::getAttribute(unsigned int i) const {
	if(i >= attributes.size()) {
		throw runtime_error(getName()+" node doesn't have "+intToStr(i)+" attributes");
	}
	return attributes[i];
}

XmlAttribute *XmlNode::getAttribute(const string &name,bool mustExist) const {
	for(unsigned int i = 0; i < attributes.size(); ++i) {
		if(attributes[i]->getName() == name) {
			return attributes[i];
		}
	}
	if(mustExist == true) {
		throw runtime_error("\"" + getName() + "\" node doesn't have a attribute named \"" + name + "\"");
	}

	return NULL;
}

bool XmlNode::hasAttribute(const string &name) const {
	bool result = false;
	for(unsigned int i = 0; i < attributes.size(); ++i) {
		if(attributes[i]->getName() == name) {
			result = true;
			break;
		}
	}
	return result;
}

XmlNode *XmlNode::getChild(unsigned int i) const {
	assert(!superNode);
	if(i >= children.size()) {
		throw runtime_error("\"" + getName()+"\" node doesn't have "+intToStr(i+1)+" children");
	}
	return children[i];
}

vector<XmlNode *> XmlNode::getChildList(const string &childName) const {
	vector<XmlNode *> list;
	for(unsigned int j = 0; j < children.size(); ++j) {
		if(children[j]->getName() == childName) {
			list.push_back(children[j]);
		}
	}

	return list;
}

XmlNode *XmlNode::getChild(const string &childName, unsigned int i) const{
	if(superNode && !hasChildNoSuper(childName))
		return superNode->getChild(childName,i);
	if(i>=children.size()){
		throw runtime_error("\"" + name + "\" node doesn't have "+intToStr(i+1)+" children named \"" + childName + "\"\n\nTree: "+getTreeString());
	}

	int count= 0;
	for(unsigned int j = 0; j < children.size(); ++j) {
		if(children[j]->getName() == childName) {
			if(count == i) {
				return children[j];
			}
			count++;
		}
	}

	throw runtime_error("Node \""+getName()+"\" doesn't have "+intToStr(i+1)+" children named  \""+childName+"\"\n\nTree: "+getTreeString());
}

bool XmlNode::hasChildAtIndex(const string &childName, int i) const {
	if(superNode && !hasChildNoSuper(childName))
		return superNode->hasChildAtIndex(childName,i);
	int count= 0;
	for(unsigned int j = 0; j < children.size(); ++j) {
		if(children[j]->getName()==childName) {
            if(count == i) {
				return true;
			}
			count++;
		}
	}

	return false;
}

bool XmlNode::hasChild(const string &childName) const {
	return hasChildNoSuper(childName) || (superNode && superNode->hasChild(childName));
}
	
bool XmlNode::hasChildNoSuper(const string &childName) const {
	//int count= 0;
	for(unsigned int j = 0; j < children.size(); ++j) {
		if(children[j]->getName() == childName) {
            return true;
		}
	}
	return false;
}

XmlNode *XmlNode::addChild(const string &name){
	assert(!superNode);
	XmlNode *node= new XmlNode(name);
	children.push_back(node);
	return node;
}

XmlAttribute *XmlNode::addAttribute(const string &name, const string &value, std::map<string,string> mapTagReplacementValues) {
	XmlAttribute *attr= new XmlAttribute(name, value, mapTagReplacementValues);
	attributes.push_back(attr);
	return attr;
}

DOMElement *XmlNode::buildElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *document) const{
	XMLCh str[strSize];
	XMLString::transcode(name.c_str(), str, strSize-1);

	DOMElement *node= document->createElement(str);

	for(unsigned int i=0; i<attributes.size(); ++i){
        XMLString::transcode(attributes[i]->getName().c_str(), str, strSize-1);
		DOMAttr *attr= document->createAttribute(str);

		XMLString::transcode(attributes[i]->getValue().c_str(), str, strSize-1);
		attr->setValue(str);

		node->setAttributeNode(attr);
	}

	for(unsigned int i=0; i<children.size(); ++i){
		node->appendChild(children[i]->buildElement(document));
	}

	return node;
}

xml_node<>* XmlNode::buildElement(xml_document<> *document) const {
	xml_node<>* node = document->allocate_node(node_element, document->allocate_string(name.c_str()));

	for(unsigned int i = 0; i < attributes.size(); ++i) {
		node->append_attribute(
				document->allocate_attribute(
						document->allocate_string(attributes[i]->getName().c_str()),
						document->allocate_string(attributes[i]->getValue().c_str())));
	}

	for(unsigned int i = 0; i < children.size(); ++i) {
		node->append_node(children[i]->buildElement(document));
	}

	return node;
}

string XmlNode::getTreeString() const {
	string str;

	str+= getName();

	if(!children.empty()) {
		str+= " (";
		for(unsigned int i=0; i<children.size(); ++i) {
			str+= children[i]->getTreeString();
			str+= " ";
		}
		str+=") ";
	}

	return str;
}

// =====================================================
//	class XmlAttribute
// =====================================================

XmlAttribute::XmlAttribute(DOMNode *attribute, std::map<string,string> mapTagReplacementValues) {
	skipRestrictionCheck 			= false;
	usesCommondata 					= false;
	this->mapTagReplacementValues 	= mapTagReplacementValues;
	char str[strSize]				= "";

	XMLString::transcode(attribute->getNodeValue(), str, strSize-1);
	value= str;
	usesCommondata = ((value.find("$COMMONDATAPATH") != string::npos) || (value.find("%%COMMONDATAPATH%%") != string::npos));
	skipRestrictionCheck = Properties::applyTagsToValue(this->value,&this->mapTagReplacementValues);

	XMLString::transcode(attribute->getNodeName(), str, strSize-1);
	name= str;
}

XmlAttribute::XmlAttribute(xml_attribute<> *attribute, std::map<string,string> mapTagReplacementValues) {
	skipRestrictionCheck 			= false;
	usesCommondata 					= false;
	this->mapTagReplacementValues 	= mapTagReplacementValues;
	//char str[strSize]				= "";

	//XMLString::transcode(attribute->getNodeValue(), str, strSize-1);
	value= attribute->value();
	usesCommondata = ((value.find("$COMMONDATAPATH") != string::npos) || (value.find("%%COMMONDATAPATH%%") != string::npos));
	skipRestrictionCheck = Properties::applyTagsToValue(this->value,&this->mapTagReplacementValues);

	//XMLString::transcode(attribute->getNodeName(), str, strSize-1);
	name= attribute->name();
}

XmlAttribute::XmlAttribute(const string &name, const string &value, std::map<string,string> mapTagReplacementValues) {
	skipRestrictionCheck 			= false;
	usesCommondata 					= false;
	this->mapTagReplacementValues 	= mapTagReplacementValues;
	this->name						= name;
	this->value						= value;

	usesCommondata = ((value.find("$COMMONDATAPATH") != string::npos) || (value.find("%%COMMONDATAPATH%%") != string::npos));
	skipRestrictionCheck = Properties::applyTagsToValue(this->value,&this->mapTagReplacementValues);
}

bool XmlAttribute::getBoolValue() const {
	if(value == "true") {
		return true;
	}
	else if(value == "false") {
		return false;
	}
	else {
		throw runtime_error("Not a valid bool value (true or false): " +getName()+": "+ value);
	}
}

int XmlAttribute::getIntValue() const {
	return strToInt(value);
}

int XmlAttribute::getIntValue(int min, int max) const {
	int i= strToInt(value);
	if(i<min || i>max){
		throw runtime_error("Xml Attribute int out of range: " + getName() + ": " + value);
	}
	return i;
}

float XmlAttribute::getFloatValue() const{
	return strToFloat(value);
}

float XmlAttribute::getFloatValue(float min, float max) const{
	float f= strToFloat(value);
	if(f<min || f>max){
		throw runtime_error("Xml attribute float out of range: " + getName() + ": " + value);
	}
	return f;
}

const string XmlAttribute::getValue(string prefixValue, bool trimValueWithStartingSlash) const {
	string result = value;
	if(skipRestrictionCheck == false && usesCommondata == false) {
		if(trimValueWithStartingSlash == true) {
			trimPathWithStartingSlash(result);
		}
		result = prefixValue + result;
	}
	return result;
}

const string XmlAttribute::getRestrictedValue(string prefixValue, bool trimValueWithStartingSlash) const {
	if(skipRestrictionCheck == false && usesCommondata == false) {
		const string allowedCharacters = "abcdefghijklmnopqrstuvwxyz1234567890._-/";

		for(unsigned int i= 0; i<value.size(); ++i){
			if(allowedCharacters.find(value[i])==string::npos){
				throw runtime_error(
					string("The string \"" + value + "\" contains a character that is not allowed: \"") + value[i] +
					"\"\nFor portability reasons the only allowed characters in this field are: " + allowedCharacters);
			}
		}
	}

	string result = value;
	if(skipRestrictionCheck == false && usesCommondata == false) {
		if(trimValueWithStartingSlash == true) {
			trimPathWithStartingSlash(result);
		}
		result = prefixValue + result;
	}
	return result;
}

}}//end namespace
