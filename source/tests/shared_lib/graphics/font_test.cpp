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
#include "font.h"
#include <vector>
#include <algorithm>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace Shared::Graphics;

//
// Tests for font class
//
class FontTest : public CppUnit::TestFixture {
	// Register the suite of tests for this fixture
	CPPUNIT_TEST_SUITE( FontTest );

	CPPUNIT_TEST( test_LTR_RTL_Mixed );

	CPPUNIT_TEST_SUITE_END();
	// End of Fixture registration

public:

	void test_LTR_RTL_Mixed() {
		Font::fontSupportMixedRightToLeft = true;

		string IntroText1 = "מבוסס על \"award-winning classic Glest\"";
		string expected = IntroText1;
		CPPUNIT_ASSERT_EQUAL( 45,(int)IntroText1.size() );

		std::vector<std::pair<char, int> > result = Font::extract_mixed_LTR_RTL_map(IntroText1);
		//CPPUNIT_ASSERT_EQUAL( 30, (int)result.size() );

#ifdef	HAVE_FRIBIDI
		IntroText1 = expected;
		Font::bidi_cvt(IntroText1);

		CPPUNIT_ASSERT_EQUAL( 45,(int)IntroText1.size() );
		CPPUNIT_ASSERT_EQUAL( string("לע ססובמ"),IntroText1.substr(0, 15) );
		CPPUNIT_ASSERT_EQUAL( string("\"award-winning classic Glest\""),IntroText1.substr(16) );
#endif

		string LuaDisableSecuritySandbox = "בטל אבטחת ארגז חול של Lua";
		LuaDisableSecuritySandbox.insert(18,"\"");
		LuaDisableSecuritySandbox.insert(34,"\"");
		//printf("Result1: [%s]\n",LuaDisableSecuritySandbox.c_str());
		string expected2 = LuaDisableSecuritySandbox;
		CPPUNIT_ASSERT_EQUAL( 44,(int)LuaDisableSecuritySandbox.size() );

		result = Font::extract_mixed_LTR_RTL_map(LuaDisableSecuritySandbox);
		//CPPUNIT_ASSERT_EQUAL( 7, (int)result.size() );

		//printf("Result: [%s]\n",LuaDisableSecuritySandbox.c_str());

#ifdef	HAVE_FRIBIDI
		LuaDisableSecuritySandbox = expected2;
		Font::bidi_cvt(LuaDisableSecuritySandbox);

		CPPUNIT_ASSERT_EQUAL( 44,(int)LuaDisableSecuritySandbox.size() );
		string expected_result = "לש לוח זגרא תחטבא לטב";
		expected_result.insert(5,"\"");
		expected_result.insert(21,"\"");

		//printf("Test: expected [%s] actual [%s]\n",expected_result.c_str(),LuaDisableSecuritySandbox.substr(0,40).c_str());
		CPPUNIT_ASSERT_EQUAL( expected_result,LuaDisableSecuritySandbox.substr(0,40) );
		CPPUNIT_ASSERT_EQUAL( string("Lua"),LuaDisableSecuritySandbox.substr(41) );
#endif

		string PrivacyPlease = "הפעל מצב פרטי\\n(הסתר מדינה, וכו)";
		PrivacyPlease.insert(52,"\"");
		string expected3 = PrivacyPlease;
		//printf("Result: [%s]\n",PrivacyPlease.c_str());
		CPPUNIT_ASSERT_EQUAL( 56,(int)PrivacyPlease.size() );

#ifdef	HAVE_FRIBIDI
		Font::bidi_cvt(PrivacyPlease);

		CPPUNIT_ASSERT_EQUAL( 56,(int)PrivacyPlease.size() );
		string expected_result3 = "יטרפ בצמ לעפה\\n(וכו ,הנידמ רתסה)";
		expected_result3.insert(29,"\"");

		//printf("Test: expected [%s] actual [%s]\n",expected_result3.c_str(),PrivacyPlease.c_str());
		CPPUNIT_ASSERT_EQUAL( expected_result3,PrivacyPlease );
#endif



		string FactionName_indian = "Indian (אינדיאנים)";
		string expected4 = FactionName_indian;
		//printf("Result: [%s]\n",PrivacyPlease.c_str());
		CPPUNIT_ASSERT_EQUAL( 27,(int)FactionName_indian.size() );

#ifdef	HAVE_FRIBIDI
		Font::bidi_cvt(FactionName_indian);

		CPPUNIT_ASSERT_EQUAL( 27,(int)FactionName_indian.size() );
		string expected_result4 = "Indian (םינאידניא)";

		//printf("Test: expected [%s] actual [%s]\n",expected_result3.c_str(),PrivacyPlease.c_str());
		CPPUNIT_ASSERT_EQUAL( expected_result4,FactionName_indian );
#endif

		// This test still failing: xx IP xx
		string LanIP = "כתובות IP מקומי:192.168.0.150  ( 61357 / 61357 )";
		string expected5 = LanIP;
		CPPUNIT_ASSERT_EQUAL( 59,(int)LanIP.size() );

#ifdef	HAVE_FRIBIDI
//		Font::bidi_cvt(LanIP);
//
//		CPPUNIT_ASSERT_EQUAL( 59,(int)LanIP.size() );
//		string expected_result5 = "abc";
//
//		CPPUNIT_ASSERT_EQUAL( expected_result5,LanIP );
#endif
	}
};


// Test Suite Registrations
CPPUNIT_TEST_SUITE_REGISTRATION( FontTest );
//
