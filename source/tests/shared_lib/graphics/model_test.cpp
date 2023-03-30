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

#include "model.h"
#include <algorithm>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <vector>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Graphics;

class TestBaseColorPickEntity : public BaseColorPickEntity {
public:
  virtual string getUniquePickName() const { return getColorDescription(); }
};
//
// Tests for font class
//
class ModelTest : public CppUnit::TestFixture {
  // Register the suite of tests for this fixture
  CPPUNIT_TEST_SUITE(ModelTest);

  CPPUNIT_TEST(test_ColorPicking_loop);
  CPPUNIT_TEST(test_ColorPicking_prime);

  CPPUNIT_TEST_SUITE_END();
  // End of Fixture registration

public:
  void test_ColorPicking_loop() {

    BaseColorPickEntity::setTrackColorUse(true);
    BaseColorPickEntity::setUsingLoopMethod(true);
    BaseColorPickEntity::resetUniqueColors();

    TestBaseColorPickEntity colorPicker;
    // This is the max color count this algorithm supports
    const int unsigned MAX_SUPPORTED_COLORS_USING_THIS_METHOD = 32767;
    for (unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD; ++i) {
      bool duplicate =
          colorPicker.get_next_assign_color(colorPicker.getUniqueColorID());
      CPPUNIT_ASSERT_EQUAL(false, duplicate);
    }

    BaseColorPickEntity::resetUniqueColors();

    TestBaseColorPickEntity colorPicker2;
    // This is a test to prove when the algorithm fails
    const int unsigned MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL = 32768;
    for (unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL;
         ++i) {
      bool duplicate =
          colorPicker2.get_next_assign_color(colorPicker2.getUniqueColorID());
      CPPUNIT_ASSERT_EQUAL(
          (i + 1 >= MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL), duplicate);
    }

    BaseColorPickEntity::resetUniqueColors();
    BaseColorPickEntity::setTrackColorUse(false);
  }

  void test_ColorPicking_prime() {

    BaseColorPickEntity::setTrackColorUse(true);
    BaseColorPickEntity::setUsingLoopMethod(false);
    BaseColorPickEntity::resetUniqueColors();

    TestBaseColorPickEntity colorPicker;
    // This is the max color count this algorithm supports
    const int unsigned MAX_SUPPORTED_COLORS_USING_THIS_METHOD = 64005;
    for (unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD; ++i) {
      bool duplicate =
          colorPicker.get_next_assign_color(colorPicker.getUniqueColorID());
      CPPUNIT_ASSERT_EQUAL(false, duplicate);
    }

    BaseColorPickEntity::resetUniqueColors();

    TestBaseColorPickEntity colorPicker2;
    // This is a test to prove when the algorithm fails
    const int unsigned MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL = 64006;
    for (unsigned int i = 0; i < MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL;
         ++i) {
      bool duplicate =
          colorPicker2.get_next_assign_color(colorPicker2.getUniqueColorID());
      CPPUNIT_ASSERT_EQUAL(
          (i + 1 >= MAX_SUPPORTED_COLORS_USING_THIS_METHOD_FAIL), duplicate);
    }

    BaseColorPickEntity::resetUniqueColors();
    BaseColorPickEntity::setTrackColorUse(false);
  }
};

// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION(ModelTest);
//
