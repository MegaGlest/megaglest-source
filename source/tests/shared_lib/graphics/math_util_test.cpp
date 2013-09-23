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
#include "math_util.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Graphics;

//
// Tests for math_util
//
class MathUtilTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( MathUtilTest );

	CPPUNIT_TEST( test_RoundFloat );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_RoundFloat() {
		float value1 = truncateDecimal<float>(1.123456f, 6);
		CPPUNIT_ASSERT_EQUAL( 1.123456f, value1 );

		value1 = truncateDecimal<float>(1.123456f, 5);
		CPPUNIT_ASSERT_EQUAL( 1.12345f, value1 );

		value1 = truncateDecimal<float>(1.123456f, 4);
		CPPUNIT_ASSERT_EQUAL( 1.1234f, value1 );

		value1 = truncateDecimal<float>(1.123456f, 3);
		CPPUNIT_ASSERT_EQUAL( 1.123f, value1 );

		value1 = truncateDecimal<float>(1.123456f, 2);
		CPPUNIT_ASSERT_EQUAL( 1.12f, value1 );

		value1 = truncateDecimal<float>(1.123456f, 1);
		CPPUNIT_ASSERT_EQUAL( 1.1f, value1 );

//		int32 value2 = xs_CRoundToInt(1.123456f);
//		CPPUNIT_ASSERT_EQUAL( (int32)1, value2 );
//
//		value2 = xs_CRoundToInt(1.523456f);
//		CPPUNIT_ASSERT_EQUAL( (int32)2, value2 );
	}
};


// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( MathUtilTest );
//
