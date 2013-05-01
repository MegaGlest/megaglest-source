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
			<< "<menu>"	<< std::endl
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
		CPPUNIT_ASSERT( rootNode->getName() == "menu" );
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
		CPPUNIT_ASSERT( rootNode->getName() == "menu" );
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
		if(static_cast<xml_engine_parser_type>(XML_XERCES_ENGINE - 1) == XML_XERCES_ENGINE - 1) {
			XmlTree xml(static_cast<xml_engine_parser_type>(XML_XERCES_ENGINE - 1));
		}
	}
	void test_invalid_xml_engine_upperbound() {
		if(static_cast<xml_engine_parser_type>(XML_RAPIDXML_ENGINE + 1) == XML_RAPIDXML_ENGINE + 1) {
			XmlTree xml(static_cast<xml_engine_parser_type>(XML_RAPIDXML_ENGINE + 1));
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
		CPPUNIT_ASSERT( xmlInstance.getRootNode()->getName() == "" );

		xmlInstance.init("testRoot");
		CPPUNIT_ASSERT( xmlInstance.getRootNode() != NULL );
		CPPUNIT_ASSERT( xmlInstance.getRootNode()->getName() == "testRoot" );
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

// Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( XmlIoTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlIoRapidTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlTreeTest );
