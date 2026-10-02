// ==============================================================
//	This file is part of MegaGlest Unit Tests (www.megaglest.org)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include <cppunit/extensions/HelperMacros.h>
#include "opengl.h"
#include "texture_gl.h"

using namespace Shared::Graphics::Gl;

//
// The game creates textures before it has a GL context, i.e. before
// gladLoadGL() has filled in the gl* function pointers (issue #405).
// This test binary never creates a context, so the pointers stay NULL.
//
class TextureGlTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TextureGlTest);

    CPPUNIT_TEST(test_texture_before_gl_loader);
    CPPUNIT_TEST(test_extension_check_before_gl_loader);

    CPPUNIT_TEST_SUITE_END();

  public:
    void test_texture_before_gl_loader() {
        CPPUNIT_ASSERT(glGetString == NULL);
        Texture2DGl texture;
        CPPUNIT_ASSERT_EQUAL(false, texture.getInited());
    }

    void test_extension_check_before_gl_loader() {
        CPPUNIT_ASSERT(glGetString == NULL);
        CPPUNIT_ASSERT_EQUAL(false, isGlExtensionSupported("GL_EXT_framebuffer_object"));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TextureGlTest);
