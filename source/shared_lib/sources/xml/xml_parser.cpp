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

#include "xml_parser.h"

#include <fstream>
#include <stdexcept>

#include "conversion.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include "util.h"
#include "types.h"
#include "leak_dumper.h"


XERCES_CPP_NAMESPACE_USE

using namespace std;

namespace Shared{ namespace Xml{

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

XmlIo::XmlIo() {
	try{
		XMLPlatformUtils::Initialize();
	}
	catch(const XMLException &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error initializing XML system, msg %s\n",__FILE__,__FUNCTION__,__LINE__,e.getMessage());
		throw runtime_error("Error initializing XML system");
	}

	try {
        XMLCh str[strSize];
        XMLString::transcode("LS", str, strSize-1);

		implementation = DOMImplementationRegistry::getDOMImplementation(str);
	}
	catch(const DOMException &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while creating XML parser, msg: %s\n",__FILE__,__FUNCTION__,__LINE__,ex.getMessage());
		throw runtime_error("Exception while creating XML parser");
	}
}

XmlIo &XmlIo::getInstance(){
	static XmlIo XmlIo;
	return XmlIo;
}

XmlIo::~XmlIo(){
	XMLPlatformUtils::Terminate();
}

XmlNode *XmlIo::load(const string &path){

	try{
		ErrorHandler errorHandler;
#if XERCES_VERSION_MAJOR < 3		
 		DOMBuilder *parser= (static_cast<DOMImplementationLS*>(implementation))->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
 		parser->setErrorHandler(&errorHandler);
 		parser->setFeature(XMLUni::fgXercesSchema, true);
 		parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
 		parser->setFeature(XMLUni::fgDOMValidation, true);
#else		
		DOMLSParser *parser = (static_cast<DOMImplementationLS*>(implementation))->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
 		DOMConfiguration  *config = parser->getDomConfig();
 		config->setParameter(XMLUni::fgXercesSchema, true);
		config->setParameter(XMLUni::fgXercesSchemaFullChecking, true);
		config->setParameter(XMLUni::fgDOMValidate, true);
#endif		
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *document= parser->parseURI(path.c_str());

		if(document==NULL){
			throw runtime_error("Can not parse URL: " + path);
		}

		XmlNode *rootNode= new XmlNode(document->getDocumentElement());
		parser->release();
		return rootNode;
	}
	catch(const DOMException &ex) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while loading: [%s], %s\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),XMLString::transcode(ex.msg));
		throw runtime_error("Exception while loading: " + path + ": " + XMLString::transcode(ex.msg));
	}
}

void XmlIo::save(const string &path, const XmlNode *node){
	try{
		XMLCh str[strSize];
		XMLString::transcode(node->getName().c_str(), str, strSize-1);

		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *document= implementation->createDocument(0, str, 0);  
		DOMElement *documentElement= document->getDocumentElement();
		
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
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Exception while saving: [%s], %s\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),XMLString::transcode(e.msg));
		throw runtime_error("Exception while saving: " + path + ": " + XMLString::transcode(e.msg));
	}
}
// =====================================================
//	class XmlTree
// =====================================================

XmlTree::XmlTree(){
	rootNode= NULL;
}

void XmlTree::init(const string &name){
	this->rootNode= new XmlNode(name);
}

void XmlTree::load(const string &path){
	this->rootNode= XmlIo::getInstance().load(path);
}

void XmlTree::save(const string &path){
	XmlIo::getInstance().save(path, rootNode);
}

XmlTree::~XmlTree(){
	delete rootNode;
}

// =====================================================
//	class XmlNode
// =====================================================

XmlNode::XmlNode(DOMNode *node){

	//get name
	char str[strSize];
	XMLString::transcode(node->getNodeName(), str, strSize-1);
	name= str;

	//check document
	if(node->getNodeType()==DOMNode::DOCUMENT_NODE){
		name="document";
	}

	//check children
	for(unsigned int i=0; i<node->getChildNodes()->getLength(); ++i){
		DOMNode *currentNode= node->getChildNodes()->item(i);
		if(currentNode->getNodeType()==DOMNode::ELEMENT_NODE){
			XmlNode *xmlNode= new XmlNode(currentNode);
			children.push_back(xmlNode);
		}
	}

	//check attributes
	DOMNamedNodeMap *domAttributes= node->getAttributes();
	if(domAttributes!=NULL){
		for(unsigned int i=0; i<domAttributes->getLength(); ++i){
			DOMNode *currentNode= domAttributes->item(i);
			if(currentNode->getNodeType()==DOMNode::ATTRIBUTE_NODE){
				XmlAttribute *xmlAttribute= new XmlAttribute(domAttributes->item(i));
				attributes.push_back(xmlAttribute);
			}
		}
	}

	//get value
	if(node->getNodeType()==DOMNode::ELEMENT_NODE && children.size()==0){
		char *textStr= XMLString::transcode(node->getTextContent());
		text= textStr;
		XMLString::release(&textStr);
	}
}

XmlNode::XmlNode(const string &name){
	this->name= name;
}

XmlNode::~XmlNode(){
	for(unsigned int i=0; i<children.size(); ++i){
		delete children[i];
	}
	for(unsigned int i=0; i<attributes.size(); ++i){
		delete attributes[i];
	}
}

XmlAttribute *XmlNode::getAttribute(unsigned int i) const{
	if(i>=attributes.size()){
		throw runtime_error(getName()+" node doesn't have "+intToStr(i)+" attributes");
	}
	return attributes[i];
}

XmlAttribute *XmlNode::getAttribute(const string &name,bool mustExist) const {
	for(unsigned int i=0; i<attributes.size(); ++i){
		if(attributes[i]->getName()==name){
			return attributes[i];
		}
	}
	if(mustExist == true) {
		throw runtime_error("\"" + getName() + "\" node doesn't have a attribute named \"" + name + "\"");
	}

	return NULL;
}

XmlNode *XmlNode::getChild(unsigned int i) const {
	if(i>=children.size())
		throw runtime_error("\"" + getName()+"\" node doesn't have "+intToStr(i+1)+" children");
	return children[i];
}


XmlNode *XmlNode::getChild(const string &childName, unsigned int i) const{
	if(i>=children.size()){
		throw runtime_error("\"" + name + "\" node doesn't have "+intToStr(i+1)+" children named \"" + childName + "\"\n\nTree: "+getTreeString());
	}

	int count= 0;
	for(unsigned int j=0; j<children.size(); ++j){
		if(children[j]->getName()==childName){
			if(count==i){
				return children[j];
			}
			count++;
		}
	}

	throw runtime_error("Node \""+getName()+"\" doesn't have "+intToStr(i+1)+" children named  \""+childName+"\"\n\nTree: "+getTreeString());
}

bool XmlNode::hasChildAtIndex(const string &childName, int i) const
{
	int count= 0;
	for(unsigned int j = 0; j < children.size(); ++j)
	{
		if(children[j]->getName()==childName)
		{
            if(count==i){
				return true;
			}
			count++;
		}
	}

	return false;
}


bool XmlNode::hasChild(const string &childName) const
{
	int count= 0;
	for(unsigned int j = 0; j < children.size(); ++j)
	{
		if(children[j]->getName()==childName)
		{
            return true;
		}
	}

	return false;
}

XmlNode *XmlNode::addChild(const string &name){
	XmlNode *node= new XmlNode(name);
	children.push_back(node);
	return node;
}

XmlAttribute *XmlNode::addAttribute(const string &name, const string &value){
	XmlAttribute *attr= new XmlAttribute(name, value);
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

string XmlNode::getTreeString() const{
	string str;

	str+= getName();

	if(!children.empty()){
		str+= " (";
		for(unsigned int i=0; i<children.size(); ++i){
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

XmlAttribute::XmlAttribute(DOMNode *attribute){
	char str[strSize];

	XMLString::transcode(attribute->getNodeValue(), str, strSize-1);
	value= str;

	XMLString::transcode(attribute->getNodeName(), str, strSize-1);
	name= str;
}

XmlAttribute::XmlAttribute(const string &name, const string &value){
	this->name= name;
	this->value= value;
}

bool XmlAttribute::getBoolValue() const{
	if(value=="true"){
		return true;
	}
	else if(value=="false"){
		return false;
	}
	else{
		throw runtime_error("Not a valid bool value (true or false): " +getName()+": "+ value);
	}
}

int XmlAttribute::getIntValue() const{
	return strToInt(value);
}

int XmlAttribute::getIntValue(int min, int max) const{
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

const string &XmlAttribute::getRestrictedValue() const
{
	const string allowedCharacters = "abcdefghijklmnopqrstuvwxyz1234567890._-/";

	for(unsigned int i= 0; i<value.size(); ++i){
		if(allowedCharacters.find(value[i])==string::npos){
			throw runtime_error(
				string("The string \"" + value + "\" contains a character that is not allowed: \"") + value[i] +
				"\"\nFor portability reasons the only allowed characters in this field are: " + allowedCharacters);
		}
	}

	return value;
}

}}//end namespace
