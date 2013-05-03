// ==============================================================
//	This file is part of MegaGlest Unit Tests (www.megaglest.org)
//
//	Copyright (C) 2013 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <fstream>
#include "xml_parser.h"
#include "platform_util.h"

#include <xercesc/dom/DOM.hpp>
//#include <xercesc/util/PlatformUtils.hpp>
//#include <xercesc/framework/LocalFileFormatTarget.hpp>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Xml;
using namespace Shared::Platform;

bool removeTestFile(string file) {
#ifdef WIN32
	int result = _unlink(file.c_str());
#else
    int result = unlink(file.c_str());
#endif

    return (result == 0);
}

void createValidXMLTestFile(const string& test_filename) {
	std::ofstream xmlFile(test_filename.c_str());
	xmlFile << "<?xml version=\"1.0\"?>" << std::endl
			<< "<menu mytest-attribute=\"true\">"	<< std::endl
			<< "<menu-background-model value=\"data/core/menu/main_model/menu_main1.g3d\"/>" << std::endl
			<< "</menu>" << std::endl;
	xmlFile.close();
}

void createMalformedXMLTestFile(const string& test_filename) {
	std::ofstream xmlFile(test_filename.c_str());
	xmlFile << "<?xml version=\"1.0\"?> #@$ !#@$@#$" << std::endl
			<< "<menu>" << std::endl
			<< "<menu-background-model value=\"data/core/menu/main_model/menu_main1.g3d\"/>"
			<< std::endl;
	xmlFile.close();
}

class SafeRemoveTestFile {
private:
	string filename;
public:
	SafeRemoveTestFile(const string &filename) {
		this->filename = filename;
	}
	~SafeRemoveTestFile() {
		removeTestFile(this->filename);
	}
};

class XmlIoTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( XmlIoTest );

	CPPUNIT_TEST( test_getInstance );
	CPPUNIT_TEST( test_cleanup );
	CPPUNIT_TEST_EXCEPTION( test_load_file_missing,  megaglest_runtime_error );
	CPPUNIT_TEST( test_load_file_valid );
	CPPUNIT_TEST_EXCEPTION( test_load_file_malformed_content,  megaglest_runtime_error );
	CPPUNIT_TEST_EXCEPTION( test_save_file_null_node,  megaglest_runtime_error );
	CPPUNIT_TEST(test_save_file_valid_node );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_getInstance() {
		XmlIo &newInstance = XmlIo::getInstance();
		CPPUNIT_ASSERT( newInstance.isInitialized() == true );
	}
	void test_cleanup() {
		XmlIo &newInstance = XmlIo::getInstance();
		CPPUNIT_ASSERT( newInstance.isInitialized() == true );

		newInstance.cleanup();
		CPPUNIT_ASSERT( newInstance.isInitialized() == false );
	}
	void test_load_file_missing() {
		XmlNode *rootNode = XmlIo::getInstance().load("/some/path/that/does/not exist", std::map<string,string>());
	}
	void test_load_file_valid() {
		const string test_filename = "xml_test_valid.xml";
		createValidXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);

		XmlNode *rootNode = XmlIo::getInstance().load(test_filename, std::map<string,string>());

		CPPUNIT_ASSERT( rootNode != NULL );
		CPPUNIT_ASSERT_EQUAL( string("menu"), rootNode->getName() );
	}
	void test_load_file_malformed_content() {
		const string test_filename = "xml_test_malformed.xml";
		createMalformedXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);

		XmlNode *rootNode = XmlIo::getInstance().load(test_filename, std::map<string,string>());
	}

	void test_save_file_null_node() {
		XmlNode *rootNode = NULL;
		XmlIo::getInstance().save("",rootNode);
	}

	void test_save_file_valid_node() {
		const string test_filename_load = "xml_test_save_load_valid.xml";
		const string test_filename_save = "xml_test_save_valid.xml";
		createValidXMLTestFile(test_filename_load);
		SafeRemoveTestFile deleteFile(test_filename_load);

		XmlNode *rootNode = XmlIo::getInstance().load(test_filename_load, std::map<string,string>());

		XmlIo::getInstance().save(test_filename_save,rootNode);
		SafeRemoveTestFile deleteFile2(test_filename_save);
	}
};

class XmlIoRapidTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( XmlIoRapidTest );

	CPPUNIT_TEST( test_getInstance );
	CPPUNIT_TEST( test_cleanup );
	CPPUNIT_TEST_EXCEPTION( test_load_file_missing,  megaglest_runtime_error );
	CPPUNIT_TEST( test_load_file_valid );
	CPPUNIT_TEST_EXCEPTION( test_load_file_malformed_content,  megaglest_runtime_error );
	CPPUNIT_TEST_EXCEPTION( test_save_file_null_node,  megaglest_runtime_error );
	CPPUNIT_TEST(test_save_file_valid_node );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_getInstance() {
		XmlIoRapid &newInstance = XmlIoRapid::getInstance();
		CPPUNIT_ASSERT( newInstance.isInitialized() == true );
	}
	void test_cleanup() {
		XmlIoRapid &newInstance = XmlIoRapid::getInstance();
		CPPUNIT_ASSERT( newInstance.isInitialized() == true );

		newInstance.cleanup();
		CPPUNIT_ASSERT( newInstance.isInitialized() == false );
	}
	void test_load_file_missing() {
		XmlNode *rootNode = XmlIoRapid::getInstance().load("/some/path/that/does/not exist", std::map<string,string>());
	}
	void test_load_file_valid() {
		const string test_filename = "xml_test_valid.xml";
		createValidXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);

		XmlNode *rootNode = XmlIoRapid::getInstance().load(test_filename, std::map<string,string>());

		CPPUNIT_ASSERT( rootNode != NULL );
		CPPUNIT_ASSERT_EQUAL( string("menu"), rootNode->getName() );
	}
	void test_load_file_malformed_content() {
		const string test_filename = "xml_test_malformed.xml";
		createMalformedXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);
		XmlNode *rootNode = XmlIoRapid::getInstance().load(test_filename, std::map<string,string>());
	}

	void test_save_file_null_node() {
		XmlNode *rootNode = NULL;
		XmlIoRapid::getInstance().save("",rootNode);
	}

	void test_save_file_valid_node() {
		const string test_filename_load = "xml_test_save_load_valid.xml";
		const string test_filename_save = "xml_test_save_valid.xml";
		createValidXMLTestFile(test_filename_load);
		SafeRemoveTestFile deleteFile(test_filename_load);

		XmlNode *rootNode = XmlIoRapid::getInstance().load(test_filename_load, std::map<string,string>());

		XmlIoRapid::getInstance().save(test_filename_save,rootNode);
		SafeRemoveTestFile deleteFile2(test_filename_save);
	}
};

class XmlTreeTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( XmlTreeTest );

	CPPUNIT_TEST_EXCEPTION( test_invalid_xml_engine_lowerbound,  megaglest_runtime_error );
	CPPUNIT_TEST_EXCEPTION( test_invalid_xml_engine_upperbound,  megaglest_runtime_error );
	CPPUNIT_TEST( test_valid_xml_engine );
	CPPUNIT_TEST( test_init );
	CPPUNIT_TEST_EXCEPTION( test_load_simultaneously_same_file,  megaglest_runtime_error );
	CPPUNIT_TEST( test_load_simultaneously_different_file );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_invalid_xml_engine_lowerbound() {
		xml_engine_parser_type testType = static_cast<xml_engine_parser_type>(XML_XERCES_ENGINE - 1);
		if(testType == XML_XERCES_ENGINE - 1) {
			XmlTree xml(testType);
		}
	}
	void test_invalid_xml_engine_upperbound() {
		xml_engine_parser_type testType = static_cast<xml_engine_parser_type>(XML_RAPIDXML_ENGINE + 1);
		if(testType == XML_RAPIDXML_ENGINE + 1) {
			XmlTree xml(testType);
		}
	}
	void test_valid_xml_engine() {
		XmlTree xmlInstance;
		CPPUNIT_ASSERT( xmlInstance.getRootNode() == NULL );
	}
	void test_init() {
		XmlTree xmlInstance;
		xmlInstance.init("");
		CPPUNIT_ASSERT( xmlInstance.getRootNode() != NULL );
		CPPUNIT_ASSERT_EQUAL( string(""), xmlInstance.getRootNode()->getName() );

		xmlInstance.init("testRoot");
		CPPUNIT_ASSERT( xmlInstance.getRootNode() != NULL );
		CPPUNIT_ASSERT_EQUAL( string("testRoot"), xmlInstance.getRootNode()->getName() );
	}
	void test_load_simultaneously_same_file() {
		const string test_filename = "xml_test_valid.xml";
		createValidXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);

		XmlTree xmlInstance1;
		xmlInstance1.load(test_filename, std::map<string,string>());
		XmlTree xmlInstance2;
		xmlInstance2.load(test_filename, std::map<string,string>());
	}
	void test_load_simultaneously_different_file() {
		const string test_filename1 = "xml_test_valid1.xml";
		createValidXMLTestFile(test_filename1);
		SafeRemoveTestFile deleteFile(test_filename1);
		const string test_filename2 = "xml_test_valid2.xml";
		createValidXMLTestFile(test_filename2);
		SafeRemoveTestFile deleteFile2(test_filename2);

		XmlTree xmlInstance1;
		xmlInstance1.load(test_filename1, std::map<string,string>());
		XmlTree xmlInstance2;
		xmlInstance2.load(test_filename2, std::map<string,string>());
	}
};


class XmlNodeTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( XmlNodeTest );

	CPPUNIT_TEST_EXCEPTION( test_null_xerces_node,  megaglest_runtime_error );
	CPPUNIT_TEST_EXCEPTION( test_null_rapidxml_node,  megaglest_runtime_error );
	CPPUNIT_TEST( test_valid_xerces_node );
	CPPUNIT_TEST( test_valid_named_node );
	CPPUNIT_TEST( test_child_nodes );
	CPPUNIT_TEST( test_node_attributes );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

private:

	class XmlIoMock : public XmlIo {
	protected:
		virtual void releaseDOMParser() { }

	public:
		XmlIoMock() : XmlIo() {	}

		DOMNode *loadDOMNode(const string &path, bool noValidation=false) {
			return XmlIo::loadDOMNode(path, noValidation);
		}

		void manualParserRelease() {
			XmlIo::releaseDOMParser();
		}
	};

public:

	void test_null_xerces_node() {
		XERCES_CPP_NAMESPACE::DOMNode *node = NULL;
		const std::map<string,string> mapTagReplacementValues;
		XmlNode(node, mapTagReplacementValues);
	}
	void test_null_rapidxml_node() {
		xml_node<> *node = NULL;
		const std::map<string,string> mapTagReplacementValues;
		XmlNode(node, mapTagReplacementValues);
	}
	void test_valid_xerces_node() {
		const string test_filename = "xml_test_valid.xml";
		createValidXMLTestFile(test_filename);
		SafeRemoveTestFile deleteFile(test_filename);

		XmlIoMock xml;
		XERCES_CPP_NAMESPACE::DOMNode *domNode = xml.loadDOMNode(test_filename);

		CPPUNIT_ASSERT( domNode != NULL );

		const std::map<string,string> mapTagReplacementValues;
		XmlNode node(domNode, mapTagReplacementValues);

		xml.manualParserRelease();

		CPPUNIT_ASSERT_EQUAL( string("menu"), node.getName() );
		CPPUNIT_ASSERT( node.hasAttribute("mytest-attribute") == true );
		CPPUNIT_ASSERT( node.hasChild("menu-background-model") == true );
	}
	void test_valid_named_node() {
		XmlNode node("testNode");

		CPPUNIT_ASSERT_EQUAL( string("testNode"), node.getName() );
	}

	void test_child_nodes() {
		XmlNode node("testNode");

		CPPUNIT_ASSERT( node.getName() == "testNode" );
		CPPUNIT_ASSERT_EQUAL( (size_t)0,node.getChildCount() );

		XmlNode *childNode1 = node.addChild("child1");
		CPPUNIT_ASSERT_EQUAL( (size_t)1,node.getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string(""), childNode1->getText() );

		XmlNode *childChildNode1 = childNode1->addChild("childchild1", "testValue");
		CPPUNIT_ASSERT_EQUAL( (size_t)1,childNode1->getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string("testValue"), childChildNode1->getText() );

		XmlNode *childChildNode2 = childNode1->addChild("childchild2", "testValue2");
		CPPUNIT_ASSERT_EQUAL( (size_t)2, childNode1->getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string("testValue2"), childChildNode2->getText() );

		XmlNode *childChildNode3 = childNode1->addChild("childchild2", "testValue3");
		CPPUNIT_ASSERT_EQUAL( (size_t)3, childNode1->getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string("testValue3"), childChildNode3->getText() );

		CPPUNIT_ASSERT( childNode1->hasChildAtIndex("childchild2",1) == true);

		XmlNode *childNode2 = node.addChild("child2","child2Value");
		CPPUNIT_ASSERT_EQUAL( (size_t)2,node.getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string("child2Value"), childNode2->getText() );

		CPPUNIT_ASSERT_EQUAL( string("child2"), node.getChild(1)->getName() );
		CPPUNIT_ASSERT_EQUAL( string("child2"), node.getChild("child2")->getName() );
		CPPUNIT_ASSERT_EQUAL( string("child1"), node.getChild("child1")->getName() );

		XmlNode *childNode2x = node.addChild("child2","child2xValue");
		CPPUNIT_ASSERT_EQUAL( (size_t)3, node.getChildCount() );
		CPPUNIT_ASSERT_EQUAL( string("child2xValue"), childNode2x->getText() );
		CPPUNIT_ASSERT_EQUAL( string("child2xValue"), node.getChild("child2",1)->getText() );

		XmlNode *childNode3 = node.addChild("child3","child3Value");
		CPPUNIT_ASSERT_EQUAL( (size_t)4, node.getChildCount() );

		vector<XmlNode *> child2List = node.getChildList("child2");
		CPPUNIT_ASSERT_EQUAL( (size_t)2, child2List.size() );
		CPPUNIT_ASSERT_EQUAL( string("child2Value"), child2List[0]->getText() );
		CPPUNIT_ASSERT_EQUAL( string("child2xValue"), child2List[1]->getText() );

		//printf("%d\n",__LINE__);
		CPPUNIT_ASSERT( childNode3->hasChild("child2") == false);
		CPPUNIT_ASSERT_EQUAL( 2, node.clearChild("child2"));
		CPPUNIT_ASSERT_EQUAL( (size_t)2,node.getChildCount() );
	}

	void test_node_attributes() {
		XmlNode node("testNode");

		CPPUNIT_ASSERT( node.getName() == "testNode" );
		CPPUNIT_ASSERT_EQUAL( (size_t)0,node.getAttributeCount() );
		CPPUNIT_ASSERT_EQUAL( (XmlAttribute *)NULL, node.getAttribute("some-attribute",false) );
		CPPUNIT_ASSERT_EQUAL( false, node.hasAttribute("some-attribute") );

		std::map<string,string> mapTagReplacementValues;
		XmlAttribute *attribute1 = node.addAttribute("some-attribute", "some-value", mapTagReplacementValues);
		CPPUNIT_ASSERT_EQUAL( (size_t)1,node.getAttributeCount() );
		CPPUNIT_ASSERT_EQUAL( attribute1, node.getAttribute("some-attribute") );
		CPPUNIT_ASSERT_EQUAL( string("some-attribute"), node.getAttribute(0)->getName() );
		CPPUNIT_ASSERT_EQUAL( true, node.hasAttribute("some-attribute") );
	}

};


// Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( XmlIoTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlIoRapidTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlTreeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlNodeTest );
