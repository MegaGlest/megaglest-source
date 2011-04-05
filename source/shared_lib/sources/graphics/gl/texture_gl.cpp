// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "texture_gl.h"
#include <stdexcept>
#include "opengl.h"
#include <iostream>
#include <vector>
#include "conversion.h"
#include <algorithm>
#include "util.h"
#ifdef WIN32
#include "glext.h"
#endif

#include "leak_dumper.h"

using namespace std;

// Define FBO's (Frame Buffer Objects) for win32
#ifdef WIN32

#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
typedef void (APIENTRY * PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint* framebuffers);typedef void (APIENTRY * PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);typedef void (APIENTRY * PFNGLBINDRENDERBUFFEREXTPROC) (GLenum target, GLuint renderbuffer);typedef GLenum (APIENTRY * PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);typedef void (APIENTRY * PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint* framebuffers);typedef void (APIENTRY * PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei n, const GLuint* renderbuffers);typedef void (APIENTRY * PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);typedef void (APIENTRY * PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei n, GLuint* renderbuffers);typedef void (APIENTRY * PFNGLGENERATEMIPMAPEXTPROC) (GLenum target);typedef void (APIENTRY * PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum target, GLenum attachment, GLenum pname, GLint* params);typedef void (APIENTRY * PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint* params);typedef GLboolean (APIENTRY * PFNGLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);typedef GLboolean (APIENTRY * PFNGLISRENDERBUFFEREXTPROC) (GLuint renderbuffer);typedef void (APIENTRY * PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;

#endif

namespace Shared { namespace Graphics { namespace Gl {

using namespace Platform;
using namespace Shared::Util;

static void setupGLExtensionMethods() {
#ifdef WIN32

	static bool setupExtensions=true;
	if(setupExtensions == true) {
		setupExtensions = false;

		//glGenFramebuffersEXT= (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		//glBindFramebufferEXT= (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
		//glFramebufferTexture2DEXT= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
		//glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		//glCheckFramebufferStatusEXT= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");

	    if(isGlExtensionSupported("GL_EXT_framebuffer_object")) {
			//glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)wglGetProcAddress("glIsRenderbufferEXT");
			glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
			glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
			glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
			glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
			//glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)wglGetProcAddress("glGetRenderbufferParameterivEXT");
			//glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)wglGetProcAddress("glIsFramebufferEXT");
			glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
			glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
			glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
			glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
			//glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)wglGetProcAddress("glFramebufferTexture1DEXT");
			glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
			//glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)wglGetProcAddress("glFramebufferTexture3DEXT");
			glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
			//glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
			//glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");

			if(!glBindRenderbufferEXT || !glDeleteRenderbuffersEXT || !glGenRenderbuffersEXT ||
				!glRenderbufferStorageEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT ||
				!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture2DEXT ||
				!glFramebufferRenderbufferEXT) {
				glGenFramebuffersEXT = NULL;
			}
		}
	}

#endif
}

const uint64 MIN_BYTES_TO_COMPRESS = 12;

static std::string getSupportCompressedTextureFormatString(int format) {
	std::string result = intToStr(format) + "[" + intToHex(format) + "]";
	switch(format) {
		case GL_COMPRESSED_ALPHA:
			result = "GL_COMPRESSED_ALPHA";
			break;
		case GL_COMPRESSED_LUMINANCE:
			result = "GL_COMPRESSED_LUMINANCE";
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA:
			result = "GL_COMPRESSED_LUMINANCE_ALPHA";
			break;
		case GL_COMPRESSED_INTENSITY:
			result = "GL_COMPRESSED_INTENSITY";
			break;
		case GL_COMPRESSED_RGB:
			result = "GL_COMPRESSED_RGB";
			break;
		case GL_COMPRESSED_RGBA:
			result = "GL_COMPRESSED_RGBA";
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			result = "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			result = "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			result = "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			result = "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
			break;

/*
		case GL_COMPRESSED_SRGB:
			result = "GL_COMPRESSED_SRGB";
			break;

		case GL_COMPRESSED_SRGB_ALPHA:
			result = "GL_COMPRESSED_SRGB_ALPHA";
			break;

		case GL_COMPRESSED_SLUMINANCE:
			result = "GL_COMPRESSED_SLUMINANCE";
			break;

		case GL_COMPRESSED_SLUMINANCE_ALPHA:
			result = "GL_COMPRESSED_SLUMINANCE_ALPHA";
			break;

		case GL_COMPRESSED_RED:
			result = "GL_COMPRESSED_RED";
			break;

		case GL_COMPRESSED_RG:
			result = "GL_COMPRESSED_RG";
			break;

		case GL_COMPRESSED_RED_RGTC1:
			result = "GL_COMPRESSED_RED_RGTC1";
			break;

		case GL_COMPRESSED_SIGNED_RED_RGTC1:
			result = "GL_COMPRESSED_SIGNED_RED_RGTC1";
			break;

		case GL_COMPRESSED_RG_RGTC2:
			result = "GL_COMPRESSED_RG_RGTC2";
			break;

		case GL_COMPRESSED_SIGNED_RG_RGTC2:
			result = "GL_COMPRESSED_SIGNED_RG_RGTC2";
			break;

		case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
			result = "GL_COMPRESSED_RGBA_BPTC_UNORM_ARB";
			break;

		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
			result = "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB";
			break;

		case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
			result = "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB";
			break;

		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
			result = "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB";
			break;
*/
		case GL_COMPRESSED_RGB_FXT1_3DFX:
			result = "GL_COMPRESSED_RGB_FXT1_3DFX";
			break;

		case GL_COMPRESSED_RGBA_FXT1_3DFX:
			result = "GL_COMPRESSED_RGBA_FXT1_3DFX";
			break;

/*
		case GL_COMPRESSED_SRGB_EXT:
			result = "GL_COMPRESSED_SRGB_EXT";
			break;

		case GL_COMPRESSED_SRGB_ALPHA_EXT:
			result = "GL_COMPRESSED_SRGB_ALPHA_EXT";
			break;

		case GL_COMPRESSED_SLUMINANCE_EXT:
			result = "GL_COMPRESSED_SLUMINANCE_EXT";
			break;

		case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
			result = "GL_COMPRESSED_SLUMINANCE_ALPHA_EXT";
			break;

		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			result = "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT";
			break;

		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
			result = "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT";
			break;

		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
			result = "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT";
			break;

		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			result = "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";
			break;

		case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
			result = "GL_COMPRESSED_LUMINANCE_LATC1_EXT";
			break;

		case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
			result = "GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT";
			break;

		case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
			result = "GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT";
			break;

		case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
			result = "GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT";
			break;

		case GL_COMPRESSED_RED_RGTC1_EXT:
			result = "GL_COMPRESSED_RED_RGTC1_EXT";
			break;

		case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
			result = "GL_COMPRESSED_SIGNED_RED_RGTC1_EXT";
			break;

		case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
			result = "GL_COMPRESSED_RED_GREEN_RGTC2_EXT";
			break;

		case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
			result = "GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT";
			break;
*/

/*
#define GL_COMPRESSED_SRGB                0x8C48
#define GL_COMPRESSED_SRGB_ALPHA          0x8C49

#define GL_COMPRESSED_SLUMINANCE          0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA    0x8C4B

#define GL_COMPRESSED_RED                 0x8225
#define GL_COMPRESSED_RG                  0x8226

#define GL_COMPRESSED_RED_RGTC1           0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1    0x8DBC
#define GL_COMPRESSED_RG_RGTC2            0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2     0x8DBE

#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F

#define GL_COMPRESSED_RGB_FXT1_3DFX       0x86B0
#define GL_COMPRESSED_RGBA_FXT1_3DFX      0x86B1

#define GL_COMPRESSED_SRGB_EXT            0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT      0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT 0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F

#define GL_COMPRESSED_LUMINANCE_LATC1_EXT 0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT 0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT 0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT 0x8C73

#define GL_COMPRESSED_RED_RGTC1_EXT       0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
*/

	}
	return result;
}

static int getNumCompressedTextureFormats() {
    int numCompressedTextureFormats = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedTextureFormats);
    return numCompressedTextureFormats;
}

static std::vector<int> getSupportCompressedTextureFormats() {
	std::vector<int> result;
	int count = getNumCompressedTextureFormats();
	if(count > 0) {
		int *formats = new int[count];
		glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS,&formats[0]);

		for(int i = 0; i < count; ++i) {
			result.push_back(formats[i]);
		}
		delete [] formats;
	}

	if(Texture::useTextureCompression == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"------------------------------------------------\n");
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"**** getSupportCompressedTextureFormats() result.size() = %d, count = %d\n",(int)result.size(),count);
		for(unsigned int i = 0; i < result.size(); ++i) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"Texture Compression #i = %d, result[i] = %d [%s]\n",i,result[i],getSupportCompressedTextureFormatString(result[i]).c_str());
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"------------------------------------------------\n");
	}
	return result;
}

GLint toCompressionFormatGl(GLint format) {
	if(Texture::useTextureCompression == false) {
		return format;
	}

	static std::vector<int> supportedCompressionFormats = getSupportCompressedTextureFormats();
	if(supportedCompressionFormats.size() <= 0) {
		return format;
	}

	//GL_COMPRESSED_ALPHA             <- white things but tile ok!
	//GL_COMPRESSED_LUMINANCE         <- black tiles
	//GL_COMPRESSED_LUMINANCE_ALPHA   <- black tiles
	//GL_COMPRESSED_INTENSITY         <- black tiles
	//GL_COMPRESSED_RGB               <- black tiles
	//GL_COMPRESSED_RGBA              <- black tiles

	// With the following extension (GL_EXT_texture_compression_s3tc)
	//GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT5_EXT

	//#define GL_COMPRESSED_RGB_FXT1_3DFX       0x86B0
	//#define GL_COMPRESSED_RGBA_FXT1_3DFX      0x86B1

	switch(format) {
		case GL_LUMINANCE:
		case GL_LUMINANCE8:
				return GL_COMPRESSED_LUMINANCE;
		case GL_RGB:
		case GL_RGB8:
			if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGB_S3TC_DXT1_EXT) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGB) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGB;
			}
			//else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) != supportedCompressionFormats.end()) {
			//	return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
			//}
			else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGB_FXT1_3DFX) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGB_FXT1_3DFX;
			}
			break;

		case GL_RGBA:
		case GL_RGBA8:

			if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGBA) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGBA;
			}
			//else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) != supportedCompressionFormats.end()) {
			//	return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
			//}
			//else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) != supportedCompressionFormats.end()) {
			//	return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
			//}
			else if(std::find(supportedCompressionFormats.begin(),supportedCompressionFormats.end(),GL_COMPRESSED_RGBA_FXT1_3DFX) != supportedCompressionFormats.end()) {
				return GL_COMPRESSED_RGBA_FXT1_3DFX;
			}
			break;

		case GL_ALPHA:
		case GL_ALPHA8:
			//return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			//return GL_COMPRESSED_ALPHA_ARB;
			return GL_COMPRESSED_ALPHA;
			//return GL_COMPRESSED_RGBA;
		default:
			return format;
	}
	return format;
}

GLint toWrapModeGl(Texture::WrapMode wrapMode){
	switch(wrapMode){
	case Texture::wmClamp:
		return GL_CLAMP;
	case Texture::wmRepeat:
		return GL_REPEAT;
	case Texture::wmClampToEdge:
		return GL_CLAMP_TO_EDGE;
	default:
		assert(false);
		return GL_CLAMP;
	}
}

GLint toFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			std::cerr << "Components = " << (components) << std::endl;
			assert(false);
			return GL_RGBA;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE;
	case Texture::fAlpha:
		return GL_ALPHA;
	case Texture::fRgb:
		return GL_RGB;
	case Texture::fRgba:
		return GL_RGBA;
	default:
		assert(false);
		return GL_RGB;
	}
}

GLint toInternalFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE8;
		case 3:
			return GL_RGB8;
		case 4:
			return GL_RGBA8;
		default:
			assert(false);
			return GL_RGBA8;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE8;
	case Texture::fAlpha:
		return GL_ALPHA8;
	case Texture::fRgb:
		return GL_RGB8;
	case Texture::fRgba:
		return GL_RGBA8;
	default:
		assert(false);
		return GL_RGB8;
	}
}

TextureGl::TextureGl() {
	handle 			= 0;
	renderBufferId 	= 0;
	frameBufferId  	= 0;
	setupGLExtensionMethods();
}

bool TextureGl::supports_FBO_RBO() {
	return (glGenFramebuffersEXT != NULL);
}

void TextureGl::setup_FBO_RBO() {
	if(getTextureWidth() < 0 || getTextureHeight() < 0) {
		throw runtime_error("getTextureWidth() < 0 || getTextureHeight() < 0");
	}

	// Need some work to get extensions properly working in Windows (use Glew lib)
	// Did we successfully setup FBO's?
	if(glGenFramebuffersEXT) {
		GLint width=0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
		GLint height=0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);

		//RGBA8 2D texture, 24 bit depth texture, 256x256
		//glGenTextures(1, &color_tex);
		//glBindTexture(GL_TEXTURE_2D, color_tex);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//NULL means reserve texture memory, but texels are undefined
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		//-------------------------
		glGenFramebuffersEXT(1, &frameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferId);
		//Attach 2D texture to this FBO
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, handle, 0);
		//-------------------------
		glGenRenderbuffersEXT(1, &renderBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBufferId);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, getTextureWidth(), getTextureHeight());
		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderBufferId);
		//-------------------------
		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch(status)
		{
		 case GL_FRAMEBUFFER_COMPLETE_EXT:
			 //SystemFlags::OutputDebug(SystemFlags::debugSystem,"FBO attachment OK!\n");
			 break;
		 default:
			 //SystemFlags::OutputDebug(SystemFlags::debugSystem,"FBO attachment BAD!\n");
			break;
		}
		//-------------------------
		//and now you can render to GL_TEXTURE_2D
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferId);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void TextureGl::teardown_FBO_RBO() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		//----------------
		//Bind 0, which means render to back buffer
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		//And in the end, cleanup
		//Delete resources
		//glDeleteTextures(1, &handle);
		glDeleteRenderbuffersEXT(1, &frameBufferId);
		//Bind 0, which means render to back buffer, as a result, fb is unbound
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glDeleteFramebuffersEXT(1, &frameBufferId);
	}
}

void TextureGl::initRenderBuffer() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// create a renderbuffer object to store depth info
		glGenRenderbuffersEXT(1, &renderBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBufferId);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,getTextureWidth(), getTextureHeight());
		//glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	}
}

void TextureGl::initFrameBuffer() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// create a framebuffer object
		glGenFramebuffersEXT(1, &frameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferId);
}
}

void TextureGl::attachRenderBuffer() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// attach the renderbuffer to depth attachment point
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT, renderBufferId);
}
}

void TextureGl::attachFrameBufferToTexture() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// attach the texture to FBO color attachment point
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D, handle, 0);
}
}

bool TextureGl::checkFrameBufferStatus() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// check FBO status
		// Does the GPU support current FBO configuration?
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if(status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"checkFrameBufferStatus() status = %d [%X]\n",status,status);
			return false;
		}
		return true;
	}

	return false;
}

void TextureGl::dettachFrameBufferFromTexture() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		// switch back to window-system-provided framebuffer
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

TextureGl::~TextureGl() {
	// Need some work to get extensions properly working in Windows (use Glew lib)
	if(glGenFramebuffersEXT) {
		if(renderBufferId != 0) {
			glDeleteRenderbuffersEXT(1, &renderBufferId);
			renderBufferId = 0;
		}
		if(frameBufferId != 0) {
			glDeleteFramebuffersEXT(1, &frameBufferId);
			frameBufferId = 0;
		}
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

// =====================================================
//	class Texture1DGl
// =====================================================
Texture1DGl::Texture1DGl() : TextureGl(), Texture1D() {}

Texture1DGl::~Texture1DGl() {
	end();
}

void Texture1DGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if(inited == false) {
		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);
		if(forceCompressionDisabled == true || (pixmap.getPixelByteCount() > 0 && pixmap.getPixelByteCount() <= MIN_BYTES_TO_COMPRESS)) {
			glCompressionFormat = glInternalFormat;
		}

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_1D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")){
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap) {
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int error= gluBuild1DMipmaps(
				GL_TEXTURE_1D, glCompressionFormat, pixmap.getW(),
				glFormat, GL_UNSIGNED_BYTE, pixels);

			// Now try without compression if we tried compression
			if(error != 0 && glCompressionFormat != glInternalFormat) {
				int error2= gluBuild1DMipmaps(
					GL_TEXTURE_1D, glInternalFormat, pixmap.getW(),
					glFormat, GL_UNSIGNED_BYTE, pixels);

				if(error2 == 0) {
					error = 0;
				}
			}
			//

			if(error != 0) {
				//throw runtime_error("Error building texture 1D mipmaps");
				char szBuf[1024]="";
				sprintf(szBuf,"Error building texture 1D mipmaps, returned: %d [%s] w = %d, glCompressionFormat = %d",error,pixmap.getPath().c_str(),pixmap.getW(),glCompressionFormat);
				throw runtime_error(szBuf);
			}
		}
		else {
			//build single texture
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage1D(
				GL_TEXTURE_1D, 0, glCompressionFormat, pixmap.getW(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();

			// Now try without compression if we tried compression
			if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
				glTexImage1D(
					GL_TEXTURE_1D, 0, glInternalFormat, pixmap.getW(),
					0, glFormat, GL_UNSIGNED_BYTE, pixels);

				GLint error2= glGetError();

				if(error2 == GL_NO_ERROR) {
					error = GL_NO_ERROR;
				}
			}
			//

			if(error != GL_NO_ERROR) {
				//throw runtime_error("Error creating texture 1D");
				char szBuf[1024]="";
				sprintf(szBuf,"Error creating texture 1D, returned: %d (%X) [%s] w = %d, glCompressionFormat = %d",error,error,pixmap.getPath().c_str(),pixmap.getW(),glCompressionFormat);
				throw runtime_error(szBuf);
			}
		}
		inited= true;
		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath(),pixmap.getPixelByteCount(),GL_TEXTURE_1D);
	}

	assertGl();
}

void Texture1DGl::end(bool deletePixelBuffer) {
	if(inited == true) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
		handle=0;
		inited=false;
		if(deletePixelBuffer == true) {
			deletePixels();
		}
	}
}

// =====================================================
//	class Texture2DGl
// =====================================================

Texture2DGl::Texture2DGl() : TextureGl(), Texture2D() {}

Texture2DGl::~Texture2DGl() {
	end();
}

void Texture2DGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if(inited == false) {
		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);
		if(forceCompressionDisabled == true || (pixmap.getPixelByteCount() > 0 && pixmap.getPixelByteCount() <= MIN_BYTES_TO_COMPRESS)) {
			glCompressionFormat = glInternalFormat;
		}

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap) {
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

/*
 * 			Replaced this call due to: http://www.opengl.org/wiki/Common_Mistakes#gluBuild2DMipmaps
 *
			int error= gluBuild2DMipmaps(
				GL_TEXTURE_2D, glCompressionFormat,
				pixmap.getW(), pixmap.getH(),
				glFormat, GL_UNSIGNED_BYTE, pixels);
*/
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, glCompressionFormat,
							pixmap.getW(), pixmap.getH(), 0,
							glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();

			// Now try without compression if we tried compression
			if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
				glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat,
								pixmap.getW(), pixmap.getH(), 0,
								glFormat, GL_UNSIGNED_BYTE, pixels);

				GLint error2= glGetError();

				if(error2 == GL_NO_ERROR) {
					error = GL_NO_ERROR;
				}
			}
			if(error != GL_NO_ERROR) {
				int error3= gluBuild2DMipmaps(
					GL_TEXTURE_2D, glCompressionFormat,
					pixmap.getW(), pixmap.getH(),
					glFormat, GL_UNSIGNED_BYTE, pixels);
				if(error3 == GL_NO_ERROR) {
					error = GL_NO_ERROR;
				}
			}

			//

			if(error != GL_NO_ERROR) {
				//throw runtime_error("Error building texture 2D mipmaps");
				char szBuf[1024]="";
				sprintf(szBuf,"Error building texture 2D mipmaps, returned: %d [%s] w = %d, h = %d, glCompressionFormat = %d",error,(pixmap.getPath() != "" ? pixmap.getPath().c_str() : this->path.c_str()),pixmap.getW(),pixmap.getH(),glCompressionFormat);
				throw runtime_error(szBuf);
			}

		}
		else {
			//build single texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, glCompressionFormat,pixmap.getW(),
					     pixmap.getH(),0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();

			// Now try without compression if we tried compression
			if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
				glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat,pixmap.getW(),
						     pixmap.getH(),0, glFormat, GL_UNSIGNED_BYTE, pixels);

				GLint error2= glGetError();

				if(error2 == GL_NO_ERROR) {
					error = GL_NO_ERROR;
				}
			}
			//

			//throw runtime_error("TEST!");

			if(error != GL_NO_ERROR) {
				char szBuf[1024]="";
				sprintf(szBuf,"Error creating texture 2D, returned: %d (%X) [%s] w = %d, h = %d, glInternalFormat = %d, glFormat = %d, glCompressionFormat = %d",error,error,pixmap.getPath().c_str(),pixmap.getW(),pixmap.getH(),glInternalFormat,glFormat,glCompressionFormat);
				throw runtime_error(szBuf);
			}
		}
		inited= true;
		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath(),pixmap.getPixelByteCount(),GL_TEXTURE_2D);
	}

	assertGl();
}

void Texture2DGl::end(bool deletePixelBuffer) {
	if(inited == true) {
		//printf("==> Deleting GL Texture [%s] handle = %d\n",getPath().c_str(),handle);
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
		handle=0;
		inited=false;
		if(deletePixelBuffer == true) {
			deletePixels();
		}
	}
}

// =====================================================
//	class Texture3DGl
// =====================================================

Texture3DGl::Texture3DGl() : TextureGl(), Texture3D() {}

Texture3DGl::~Texture3DGl() {
	end();
}

void Texture3DGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if(inited == false) {
		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);
		if(forceCompressionDisabled == true || (pixmap.getPixelByteCount() > 0 && pixmap.getPixelByteCount() <= MIN_BYTES_TO_COMPRESS)) {
			glCompressionFormat = glInternalFormat;
		}
		if(glCompressionFormat == GL_COMPRESSED_RGBA_FXT1_3DFX) {
			glCompressionFormat = glInternalFormat;
		}

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_3D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);

		//build single texture
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage3D(
			GL_TEXTURE_3D, 0, glCompressionFormat,
			pixmap.getW(), pixmap.getH(), pixmap.getD(),
			0, glFormat, GL_UNSIGNED_BYTE, pixels);

		GLint error= glGetError();

		// Now try without compression if we tried compression
		if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
			glTexImage3D(
				GL_TEXTURE_3D, 0, glInternalFormat,
				pixmap.getW(), pixmap.getH(), pixmap.getD(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error2= glGetError();

			if(error2 == GL_NO_ERROR) {
				error = GL_NO_ERROR;
			}
		}
		//

		if(error != GL_NO_ERROR) {
			//throw runtime_error("Error creating texture 3D");
			char szBuf[1024]="";
			sprintf(szBuf,"Error creating texture 3D, returned: %d (%X) [%s] w = %d, h = %d, d = %d, glCompressionFormat = %d",error,error,pixmap.getPath().c_str(),pixmap.getW(),pixmap.getH(),pixmap.getD(),glCompressionFormat);
			throw runtime_error(szBuf);
		}
		inited= true;

		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath(),pixmap.getPixelByteCount(),GL_TEXTURE_3D);
	}

	assertGl();
}

void Texture3DGl::end(bool deletePixelBuffer) {
	if(inited == true) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
		handle=0;
		inited=false;
		if(deletePixelBuffer == true) {
			deletePixels();
		}
	}
}

// =====================================================
//	class TextureCubeGl
// =====================================================

TextureCubeGl::TextureCubeGl() : TextureGl(), TextureCube() {}

TextureCubeGl::~TextureCubeGl() {
	end();
}

void TextureCubeGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if(inited == false) {
		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

		//wrap
		GLint wrap= toWrapModeGl(wrapMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);

		//filter
		if(mipmap) {
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		for(int i = 0; i < 6; ++i) {
			//params
			const Pixmap2D *currentPixmap= pixmap.getFace(i);

			GLint glFormat= toFormatGl(format, currentPixmap->getComponents());
			GLint glInternalFormat= toInternalFormatGl(format, currentPixmap->getComponents());
			GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);
			if(forceCompressionDisabled == true || (currentPixmap->getPixelByteCount() > 0 && currentPixmap->getPixelByteCount() <= MIN_BYTES_TO_COMPRESS)) {
				glCompressionFormat = glInternalFormat;
			}

			//pixel init var
			const uint8* pixels= pixmapInit? currentPixmap->getPixels(): NULL;
			GLenum target= GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			if(mipmap) {

/*
 * 				Replaced this call due to: http://www.opengl.org/wiki/Common_Mistakes#gluBuild2DMipmaps
 *
				int error= gluBuild2DMipmaps(
					target, glCompressionFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					glFormat, GL_UNSIGNED_BYTE, pixels);

*/
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

				glTexImage2D(target, 0, glCompressionFormat,
							 currentPixmap->getW(), currentPixmap->getH(), 0,
							 glFormat, GL_UNSIGNED_BYTE, pixels);

				int error = glGetError();

				// Now try without compression if we tried compression
				if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
					glTexImage2D(target, 0, glInternalFormat,
								 currentPixmap->getW(), currentPixmap->getH(), 0,
								 glFormat, GL_UNSIGNED_BYTE, pixels);

					GLint error2= glGetError();

					if(error2 == GL_NO_ERROR) {
						error = GL_NO_ERROR;
					}
				}
				if(error != GL_NO_ERROR) {
					int error3= gluBuild2DMipmaps(
										target, glCompressionFormat,
										currentPixmap->getW(), currentPixmap->getH(),
										glFormat, GL_UNSIGNED_BYTE, pixels);


					if(error3 == GL_NO_ERROR) {
						error = GL_NO_ERROR;
					}
				}

				//

				if(error != GL_NO_ERROR) {
					//throw runtime_error("Error building texture cube mipmaps");
					char szBuf[1024]="";
					sprintf(szBuf,"Error building texture cube mipmaps, returned: %d [%s] w = %d, h = %d, glCompressionFormat = %d",error,currentPixmap->getPath().c_str(),currentPixmap->getW(),currentPixmap->getH(),glCompressionFormat);
					throw runtime_error(szBuf);
				}

			}
			else {
				glTexImage2D(
					target, 0, glCompressionFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					0, glFormat, GL_UNSIGNED_BYTE, pixels);

				int error = glGetError();

				// Now try without compression if we tried compression
				if(error != GL_NO_ERROR && glCompressionFormat != glInternalFormat) {
					glTexImage2D(
						target, 0, glInternalFormat,
						currentPixmap->getW(), currentPixmap->getH(),
						0, glFormat, GL_UNSIGNED_BYTE, pixels);

					GLint error2= glGetError();

					if(error2 == GL_NO_ERROR) {
						error = GL_NO_ERROR;
					}
				}
				//

				if(error != GL_NO_ERROR) {
					//throw runtime_error("Error creating texture cube");
					char szBuf[1024]="";
					sprintf(szBuf,"Error creating texture cube, returned: %d (%X) [%s] w = %d, h = %d, glCompressionFormat = %d",error,error,currentPixmap->getPath().c_str(),currentPixmap->getW(),currentPixmap->getH(),glCompressionFormat);
					throw runtime_error(szBuf);
				}
			}

			OutputTextureDebugInfo(format, currentPixmap->getComponents(),getPath(),currentPixmap->getPixelByteCount(),target);
		}
		inited= true;

	}

	assertGl();
}

void TextureCubeGl::end(bool deletePixelBuffer) {
	if(inited == true) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
		handle=0;
		inited=false;
		if(deletePixelBuffer == true) {
			deletePixels();
		}
	}
}

void TextureGl::OutputTextureDebugInfo(Texture::Format format, int components,const string path,uint64 rawSize,GLenum texType) {
	if(Texture::useTextureCompression == true) {
		GLint glFormat= toFormatGl(format, components);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"**** Texture filename: [%s] format = %d components = %d, glFormat = %d, rawSize = %llu\n",path.c_str(),format,components,glFormat,(long long unsigned int)rawSize);

		GLint compressed=0;
		glGetTexLevelParameteriv(texType, 0, GL_TEXTURE_COMPRESSED, &compressed);
		int error = glGetError();

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"**** Texture compressed status: %d, error [%d] (%X)\n",compressed,error,error);

		bool isCompressed = (compressed == 1);
		compressed=0;
		glGetTexLevelParameteriv(texType, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressed);
		error = glGetError();

		double percent = 0;
		if(isCompressed == true) {
			percent = ((double)compressed / (double)rawSize) * (double)100.0;
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"**** Texture image size in video RAM: %d [%.2f%%], error [%d] (%X)\n",compressed,percent,error,error);

		compressed=0;
		glGetTexLevelParameteriv(texType, 0, GL_TEXTURE_INTERNAL_FORMAT, &compressed);
		error = glGetError();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"**** Texture image compression format used: %d, error [%d] (%X)\n",compressed,error,error);
	}
}

}}}//end namespace
