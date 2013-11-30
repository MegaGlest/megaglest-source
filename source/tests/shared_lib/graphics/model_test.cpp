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
#include "model.h"
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Graphics;

class TestBaseColorPickEntity : public BaseColorPickEntity {
public:
	virtual string getUniquePickName() const {
		return getColorDescription();
	}
};
//
// Tests for font class
//
class ModelTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( ModelTest );

	CPPUNIT_TEST( test_ColorPicking_loop );
	CPPUNIT_TEST( test_ColorPicking_prime );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_ColorPicking_loop() {

		BaseColorPickEntity::setTrackColorUse(true);
		BaseColorPickEntity::setUsingLoopMethod(true);
		BaseColorPickEntity::resetUniqueColors();

		TestBaseColorPickEntity colorPicker;
		const int MAX_SUPPORTED_COLORS_USING_THIS_METHOD = 32767;
		for(unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD; ++i) {
			bool duplicate = colorPicker.get_next_assign_color(colorPicker.getUniqueColorID());
			CPPUNIT_ASSERT_EQUAL( false,duplicate );
		}

		BaseColorPickEntity::setTrackColorUse(false);
	}

	void test_ColorPicking_prime() {

		BaseColorPickEntity::setTrackColorUse(true);
		BaseColorPickEntity::setUsingLoopMethod(false);
		BaseColorPickEntity::resetUniqueColors();

		TestBaseColorPickEntity colorPicker;
		const int MAX_SUPPORTED_COLORS_USING_THIS_METHOD = 472;
		for(unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD; ++i) {
			bool duplicate = colorPicker.get_next_assign_color(colorPicker.getUniqueColorID());
			CPPUNIT_ASSERT_EQUAL( false,duplicate );
		}

		BaseColorPickEntity::setTrackColorUse(false);
	}

};


// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( ModelTest );
//
