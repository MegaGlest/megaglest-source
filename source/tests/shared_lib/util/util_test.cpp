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
#include "util.h"
#include <memory>
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Util;

//
// Tests for util classes
//
// **NOTE: IMPORTANT!
// AFTER 3.7.0 we moved to 2 digit compatibility checks

class UtilTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( UtilTest );

	CPPUNIT_TEST( test_checkVersionComptability_2_digit_versions );
	CPPUNIT_TEST( test_checkVersionComptability_3_digit_versions );
	CPPUNIT_TEST( test_checkVersionComptability_mixed_digit_versions );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_checkVersionComptability_2_digit_versions() {
		bool debug_verbose_tests = false;
		SystemFlags::VERBOSE_MODE_ENABLED = debug_verbose_tests;
		// -------- START

		// 2 digit version checks
		bool result = checkVersionComptability("v4.0", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.9-dev", "v4.0");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v4.0-dev", "v4.0");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.9-dev", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8-dev", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8-dev", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8-beta2", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8-beta1", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8-beta1", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		// -------- END
		SystemFlags::VERBOSE_MODE_ENABLED = false;
	}

	void test_checkVersionComptability_3_digit_versions() {
		bool debug_verbose_tests = false;
		SystemFlags::VERBOSE_MODE_ENABLED = debug_verbose_tests;
		// -------- START

		// 3 digit version checks
		bool result = checkVersionComptability("v3.9.0", "v3.9.1");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.9.0-dev", "v3.9.0");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta2", "v3.9.0");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8.0-beta1", "v3.9.0");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8.0-dev", "v3.9.0");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8.1", "v3.8.2");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta2", "v3.8.0");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-dev", "v3.8.0");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta2", "v3.8.0-dev");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta1", "v3.8.0-dev");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta1", "v3.8.0-beta2");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.7.1", "v3.7.2-dev");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.7.1", "v3.7.2");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.7.1-dev", "v3.7.1");
		CPPUNIT_ASSERT_EQUAL( true,result );

		// **NOTE: IMPORTANT!
		// AFTER 3.7.0 we moved to 2 digit compatibility checks
		result = checkVersionComptability("v3.7.0", "v3.7.1-dev");
		CPPUNIT_ASSERT_EQUAL( false,result );

		// -------- END
		SystemFlags::VERBOSE_MODE_ENABLED = false;
	}

	void test_checkVersionComptability_mixed_digit_versions() {
		bool debug_verbose_tests = false;
		SystemFlags::VERBOSE_MODE_ENABLED = debug_verbose_tests;
		// -------- START

		bool result = checkVersionComptability("v3.8.0-dev", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8.0-beta2", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.8.0-beta1", "v3.9");
		CPPUNIT_ASSERT_EQUAL( false,result );

		// Mixed digit version checks
		result = checkVersionComptability("v3.8", "v3.8.2");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8", "v3.8.1");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-dev", "v3.8");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta2", "v3.8");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.8.0-beta1", "v3.8");
		CPPUNIT_ASSERT_EQUAL( true,result );

		result = checkVersionComptability("v3.7.2-dev", "v3.8-dev");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.7.1", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.7.1", "v3.8-dev");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.7.1", "v3.8-beta2");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.7.1", "v3.8-beta1");
		CPPUNIT_ASSERT_EQUAL( false,result );

		result = checkVersionComptability("v3.7.0", "v3.8");
		CPPUNIT_ASSERT_EQUAL( false,result );

		// -------- END
		SystemFlags::VERBOSE_MODE_ENABLED = false;
	}

};


// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( UtilTest );
//
