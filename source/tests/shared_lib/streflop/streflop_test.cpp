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
#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include "skill_type.h"
#include "vec.h"
#include <algorithm>
#include <memory>
#include <vector>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Glest::Game;
//
// Tests for streflop floating point consistency
//
const int64 PROGRESS_SPEED_MULTIPLIER = 100000;
const float standardAirHeight = 5.0f;
const float FLOAT_TOLERANCE = 1e-10f;

class StreflopTest : public CppUnit::TestFixture {
  // Register the suite of tests for this fixture
  CPPUNIT_TEST_SUITE(StreflopTest);

  CPPUNIT_TEST(test_warmup_cases);
  CPPUNIT_TEST(test_known_out_of_synch_cases);

  CPPUNIT_TEST_SUITE_END();
  // End of Fixture registration

private:
  float cellHeight;
  Field currField;
  float tileSetAirHeight;
  int cellLandUnitHeight;
  int cellObjectHeight;
  SkillClass currSkill;
  int curUnitTypeSize;
  int64 progress;
  Vec2i lastPos;
  Vec2i pos;

  void reset() {
    cellHeight = 0.f;
    currField = fLand;
    tileSetAirHeight = standardAirHeight;
    cellLandUnitHeight = 0;
    cellObjectHeight = 0;
    currSkill = scStop;
    curUnitTypeSize = 0;
    progress = 0;
    lastPos = Vec2i(0, 0);
    pos = Vec2i(0, 0);
  }

public:
  StreflopTest() {
#ifdef USE_STREFLOP
    // #define STREFLOP_NO_DENORMALS
    //
    // #if defined(STREFLOP_SSE)
    //		const char *instruction_set = "[SSE]";
    // #elif defined(STREFLOP_X87)
    //		const char *instruction_set = "[X87]";
    // #elif defined(STREFLOP_SOFT)
    //		const char *instruction_set = "[SOFTFLOAT]";
    // #else
    //		const char *instruction_set = "[none]";
    // #endif
    //
    // #if defined(STREFLOP_NO_DENORMALS)
    //		const char *denormals = "[no-denormals]";
    // #else
    //		const char *denormals = "[denormals]";
    // #endif
    //
    //	printf("Tests - using STREFLOP %s - %s\n",instruction_set,denormals);

    streflop_init<streflop::Simple>();

#endif

    reset();
  }

  void test_warmup_cases() {
    int unitTypeHeight = 0;

    cellHeight = 1.1f;
    currField = fLand;
    tileSetAirHeight = standardAirHeight;
    cellLandUnitHeight = 1;
    cellObjectHeight = 0;
    currSkill = scMove;
    curUnitTypeSize = 1;
    progress = 10;
    lastPos = Vec2i(1, 1);
    pos = Vec2i(1, 2);
    unitTypeHeight = 1;
    Vec3f result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 0;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.1] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 2;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [2.1] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fAir;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [6.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fAir;
    currSkill = scAttack;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [6.6] z [2]"), result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scAttack;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [2]"), result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scAttack;
    cellLandUnitHeight = -1;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [2]"), result.getString());

    unitTypeHeight = 1;
    currField = fAir;
    currSkill = scAttack;
    cellLandUnitHeight = -1;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [6.6] z [2]"), result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scMove;
    cellLandUnitHeight = 2;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fAir;
    currSkill = scMove;
    cellLandUnitHeight = 2;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [6.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scMove;
    cellLandUnitHeight = -1;
    cellObjectHeight = 0;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scMove;
    cellLandUnitHeight = -1;
    cellObjectHeight = 1;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fAir;
    currSkill = scMove;
    cellLandUnitHeight = -1;
    cellObjectHeight = 1;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [6.6] z [1.0001]"),
                         result.getString());

    unitTypeHeight = 1;
    currField = fLand;
    currSkill = scMove;
    progress = 1324312;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [1] y [1.6] z [14.2431]"),
                         result.getString());

    cellHeight = 2.870369f;
    currField = fLand;
    tileSetAirHeight = standardAirHeight;
    cellLandUnitHeight = 0;
    cellObjectHeight = 0;
    currSkill = scAttack;
    curUnitTypeSize = 1;
    progress = 21250;
    lastPos = Vec2i(96, 34);
    pos = Vec2i(95, 35);
    unitTypeHeight = 3;
    result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [95] y [4.37037] z [35]"),
                         result.getString());

    double x = 1.0;
    x /= 10.0;
    double y = x;
    // THIS IS NOT ALWAYS TRUE without streflop!
    CPPUNIT_ASSERT_DOUBLES_EQUAL(x, y, FLOAT_TOLERANCE);

    float xf = 1.0;
    xf /= 10.0;
    float yf = xf;
    // THIS IS NOT ALWAYS TRUE without streflop!
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xf, yf, FLOAT_TOLERANCE);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.2f, 0.1f + 0.1f, FLOAT_TOLERANCE);
    CPPUNIT_ASSERT_EQUAL(string("0.200000"), floatToStr(0.1f + 0.1f, 6));
    CPPUNIT_ASSERT_EQUAL(string("0.2000000"), floatToStr(0.1f + 0.1f, 7));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.01f, 0.1f * 0.1f, 1e-9);
    CPPUNIT_ASSERT_EQUAL(string("0.010000"), floatToStr(0.1f * 0.1f, 6));
    CPPUNIT_ASSERT_EQUAL(string("0.0100000"), floatToStr(0.1f * 0.1f, 7));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.002877f, 2877.0f / 1000000.0f,
                                 FLOAT_TOLERANCE);
    CPPUNIT_ASSERT_EQUAL(string("0.002877"),
                         floatToStr(2877.0f / 1000000.0f, 6));
    CPPUNIT_ASSERT_EQUAL(string("0.0028770"),
                         floatToStr(2877.0f / 1000000.0f, 7));
  }

  void test_known_out_of_synch_cases() {
    // Documented cases of out of synch go here to test cross platform for
    // consistency
    int unitTypeHeight = 0;

    cellHeight = 2.814814f;
    currField = fLand;
    tileSetAirHeight = 5.000000f;
    cellLandUnitHeight = -1;
    cellObjectHeight = -1;
    currSkill = scMove;
    curUnitTypeSize = 1;
    progress = 35145;
    lastPos = Vec2i(40, 41);
    pos = Vec2i(39, 40);
    unitTypeHeight = 2;

    Vec3f result = getCurrVector(unitTypeHeight);
    CPPUNIT_ASSERT_EQUAL(string("x [39.6485] y [3.81481] z [40.6485]"),
                         result.getString());
  }

  // =========================== Helper Methods
  // =================================

  float computeHeight(const Vec2i &pos) const {

    float height = cellHeight;

    if (currField == fAir) {
      float airHeight = tileSetAirHeight;
      airHeight = truncateDecimal<float>(airHeight, 6);

      height += airHeight;
      height = truncateDecimal<float>(height, 6);

      if (cellLandUnitHeight >= 0 && cellLandUnitHeight > airHeight) {
        height +=
            (min((float)cellLandUnitHeight, standardAirHeight * 3) - airHeight);
        height = truncateDecimal<float>(height, 6);
      } else {
        if (cellObjectHeight >= 0) {
          if (cellObjectHeight > airHeight) {
            height += (min((float)cellObjectHeight, standardAirHeight * 3) -
                       airHeight);
            height = truncateDecimal<float>(height, 6);
          }
        }
      }
    }

    return height;
  }

  float getProgressAsFloat() const {
    float result = (static_cast<float>(progress) /
                    static_cast<float>(PROGRESS_SPEED_MULTIPLIER));
    result = truncateDecimal<float>(result, 6);
    return result;
  }

  Vec3f getVectorFlat(const Vec2i &lastPosValue,
                      const Vec2i &curPosValue) const {
    Vec3f v;

    float y1 = computeHeight(lastPosValue);
    float y2 = computeHeight(curPosValue);

    if (currSkill == scMove) {
      float progressAsFloat = getProgressAsFloat();

      v.x = lastPosValue.x + progressAsFloat * (curPosValue.x - lastPosValue.x);
      v.z = lastPosValue.y + progressAsFloat * (curPosValue.y - lastPosValue.y);
      v.y = y1 + progressAsFloat * (y2 - y1);

      v.x = truncateDecimal<float>(v.x, 6);
      v.y = truncateDecimal<float>(v.y, 6);
      v.z = truncateDecimal<float>(v.z, 6);
    } else {
      v.x = static_cast<float>(curPosValue.x);
      v.z = static_cast<float>(curPosValue.y);
      v.y = y2;

      v.x = truncateDecimal<float>(v.x, 6);
      v.y = truncateDecimal<float>(v.y, 6);
      v.z = truncateDecimal<float>(v.z, 6);
    }
    v.x += curUnitTypeSize / 2.f - 0.5f;
    v.z += curUnitTypeSize / 2.f - 0.5f;

    v.x = truncateDecimal<float>(v.x, 6);
    v.z = truncateDecimal<float>(v.z, 6);

    return v;
  }

  Vec3f getCurrVectorFlat() const { return getVectorFlat(lastPos, pos); }

  Vec3f getCurrVector(int unitTypeHeight) const {
    Vec3f result = getCurrVectorFlat() + Vec3f(0.f, unitTypeHeight / 2.f, 0.f);
    result.x = truncateDecimal<float>(result.x, 6);
    result.y = truncateDecimal<float>(result.y, 6);
    result.z = truncateDecimal<float>(result.z, 6);

    return result;
  }
};

// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION(StreflopTest);
//
