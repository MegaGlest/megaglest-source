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
  #include <winsock2.h>
  #include <winsock.h>
#endif

#include "vec.h"
#include "skill_type.h"
#include <memory>
#include <vector>
#include <algorithm>

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
const int64 PROGRESS_SPEED_MULTIPLIER  	= 100000;
const float standardAirHeight			= 5.0f;

class StreflopTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( StreflopTest );

	CPPUNIT_TEST( test_warmup_cases );
	CPPUNIT_TEST( test_known_out_of_synch_cases );

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

public:

	void test_warmup_cases() {

		cellHeight 			= 1.1;
		currField  			= fLand;
		tileSetAirHeight	= standardAirHeight;
		cellLandUnitHeight	= 1;
		cellObjectHeight	= 0;
		currSkill			= scMove;
		curUnitTypeSize		= 1;
		progress			= 10;
		lastPos 			= Vec2i(1,1);
		pos 				= Vec2i(1,2);

		int unitTypeHeight 	= 1;
		Vec3f result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [1.0001]"), result.getString() );

		unitTypeHeight 	= 0;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.1] z [1.0001]"), result.getString() );

		unitTypeHeight 	= 2;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [2.1] z [1.0001]"), result.getString() );

		unitTypeHeight 	= 1;
		currField  		= fAir;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [6.6] z [1.0001]"), result.getString() );

		unitTypeHeight 	= 1;
		currField  		= fAir;
		currSkill		= scAttack;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [6.6] z [2]"), result.getString() );

		unitTypeHeight 	= 1;
		currField  		= fLand;
		currSkill		= scAttack;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [2]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fLand;
		currSkill			= scAttack;
		cellLandUnitHeight	= -1;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [2]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fAir;
		currSkill			= scAttack;
		cellLandUnitHeight	= -1;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [6.6] z [2]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fLand;
		currSkill			= scMove;
		cellLandUnitHeight	= 2;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [1.0001]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fAir;
		currSkill			= scMove;
		cellLandUnitHeight	= 2;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [6.6] z [1.0001]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fLand;
		currSkill			= scMove;
		cellLandUnitHeight	= -1;
		cellObjectHeight	= 0;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [1.0001]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fLand;
		currSkill			= scMove;
		cellLandUnitHeight	= -1;
		cellObjectHeight	= 1;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [1.0001]"), result.getString() );

		unitTypeHeight 		= 1;
		currField  			= fAir;
		currSkill			= scMove;
		cellLandUnitHeight	= -1;
		cellObjectHeight	= 1;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [6.6] z [1.0001]"), result.getString() );

		unitTypeHeight 	= 1;
		currField  		= fLand;
		currSkill		= scMove;
		progress		= 1324312;
		result = getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [1] y [1.6] z [14.2431]"), result.getString() );

		cellHeight 			= 2.870369;
		currField  			= fLand;
		tileSetAirHeight	= standardAirHeight;
		cellLandUnitHeight	= 0;
		cellObjectHeight	= 0;
		currSkill			= scAttack;
		curUnitTypeSize		= 1;
		progress			= 21250;
		lastPos 			= Vec2i(96,34);
		pos 				= Vec2i(95,35);

		unitTypeHeight 		= 3;
		result 				= getCurrVector(unitTypeHeight);
		CPPUNIT_ASSERT_EQUAL( string("x [95] y [4.37037] z [35]"), result.getString() );

	}

	void test_known_out_of_synch_cases() {

//		cellHeight 			= 2.870369;
//		currField  			= fLand;
//		tileSetAirHeight	= standardAirHeight;
//		cellLandUnitHeight	= 0;
//		cellObjectHeight	= 0;
//		currSkill			= scAttack;
//		curUnitTypeSize		= 1;
//		progress			= 21250;
//		lastPos 			= Vec2i(96,34);
//		pos 				= Vec2i(95,35);
//
//		int unitTypeHeight 	= 3;
//		Vec3f result = getCurrVector(unitTypeHeight);
//		CPPUNIT_ASSERT_EQUAL( string("x [95] y [4.37037] z [35]"), result.getString() );

	}

// =========================== Helper Methods =================================

	float computeHeight(const Vec2i &pos) const {

		float height = cellHeight;

		if(currField == fAir) {
			float airHeight = tileSetAirHeight;
			airHeight = truncateDecimal<float>(airHeight,6);

			height += airHeight;
			height = truncateDecimal<float>(height,6);

			if(cellLandUnitHeight >= 0 && cellLandUnitHeight > airHeight) {
				height += (min((float)cellLandUnitHeight,standardAirHeight * 3) - airHeight);
				height = truncateDecimal<float>(height,6);
			}
			else {
				if(cellObjectHeight >= 0) {
					if(cellObjectHeight > airHeight) {
						height += (min((float)cellObjectHeight,standardAirHeight * 3) - airHeight);
						height = truncateDecimal<float>(height,6);
					}
				}
			}
		}

		return height;
	}

	float getProgressAsFloat() const {
		float result = (static_cast<float>(progress) / static_cast<float>(PROGRESS_SPEED_MULTIPLIER));
		result = truncateDecimal<float>(result,6);
		return result;
	}

	Vec3f getVectorFlat(const Vec2i &lastPosValue, const Vec2i &curPosValue) const {
	    Vec3f v;

	    float y1= computeHeight(lastPosValue);
	    float y2= computeHeight(curPosValue);

	    if(currSkill == scMove) {
	    	float progressAsFloat = getProgressAsFloat();

	        v.x = lastPosValue.x + progressAsFloat * (curPosValue.x - lastPosValue.x);
	        v.z = lastPosValue.y + progressAsFloat * (curPosValue.y - lastPosValue.y);
			v.y = y1 + progressAsFloat * (y2-y1);

			v.x = truncateDecimal<float>(v.x,6);
			v.y = truncateDecimal<float>(v.y,6);
			v.z = truncateDecimal<float>(v.z,6);
	    }
	    else {
	        v.x = static_cast<float>(curPosValue.x);
	        v.z = static_cast<float>(curPosValue.y);
	        v.y = y2;

			v.x = truncateDecimal<float>(v.x,6);
			v.y = truncateDecimal<float>(v.y,6);
			v.z = truncateDecimal<float>(v.z,6);
	    }
	    v.x += curUnitTypeSize / 2.f - 0.5f;
	    v.z += curUnitTypeSize / 2.f - 0.5f;

		v.x = truncateDecimal<float>(v.x,6);
		v.z = truncateDecimal<float>(v.z,6);

	    return v;
	}

	Vec3f getCurrVectorFlat() const {
		return getVectorFlat(lastPos, pos);
	}

	Vec3f getCurrVector(int unitTypeHeight) const {
		Vec3f result = getCurrVectorFlat() + Vec3f(0.f, unitTypeHeight / 2.f, 0.f);
		result.x = truncateDecimal<float>(result.x,6);
		result.y = truncateDecimal<float>(result.y,6);
		result.z = truncateDecimal<float>(result.z,6);

		return result;
	}

};

// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( StreflopTest );
//
