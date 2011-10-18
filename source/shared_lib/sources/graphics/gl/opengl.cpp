// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "opengl.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "context_gl.h"
#include "gl_wrap.h"
#include <map>
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Shared { namespace Graphics { namespace Gl {

std::map<string,bool> cacheExtensionCheckList;
static int vboEnabled = 0;

// =====================================================
//	class Globals
// =====================================================

bool getVBOSupported() {
	if(vboEnabled == 0) {
		bool value = isGlExtensionSupported("GL_ARB_vertex_buffer_object");
		vboEnabled = (value == true ? 1 : -1);
	}
	return (vboEnabled == 1);
}
void setVBOSupported(bool value) {
	vboEnabled = (value == true ? 1 : -1);
};

void overrideGlExtensionSupport(const char *extensionName,bool value) {
	cacheExtensionCheckList[extensionName]=value;
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("OpenGL Extension [%s] supported status FORCED TO = %d\n",extensionName,cacheExtensionCheckList[extensionName]);
}

bool isGlExtensionSupported(const char *extensionName) {
	if(cacheExtensionCheckList.find(extensionName) != cacheExtensionCheckList.end()) {
		return cacheExtensionCheckList[extensionName];
	}
	const GLubyte *extensionStr= glGetString(GL_EXTENSIONS);
	const char *s= reinterpret_cast<const char *>(extensionStr);
	size_t len= strlen(extensionName);

	cacheExtensionCheckList[extensionName]=false;
    if(s != NULL) {
        while ((s = strstr (s, extensionName)) != NULL) {
            s+= len;
            if((*s == ' ') || (*s == '\0')) {
            	cacheExtensionCheckList[extensionName] = true;
            	break;
            }
        }
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("OpenGL Extension [%s] supported status = %d\n",extensionName,cacheExtensionCheckList[extensionName]);
    return cacheExtensionCheckList[extensionName];
}

bool isGlVersionSupported(int major, int minor, int release) {

	const char *strVersion= getGlVersion();

	//major
	const char *majorTok= strVersion;
	int majorSupported= atoi(majorTok);

	if(majorSupported<major) {
		return false;
	}
	else if(majorSupported>major) {
		return true;
	}

	//minor
	int i=0;
	while(strVersion[i]!='.') {
		++i;
	}
	const char *minorTok= &strVersion[i]+1;
	int minorSupported= atoi(minorTok);

	if(minorSupported<minor) {
		return false;
	}
	else if(minorSupported>minor) {
		return true;
	}

	//release
	++i;
	while(strVersion[i]!='.') {
		++i;
	}
	const char *releaseTok= &strVersion[i]+1;

	if(atoi(releaseTok) < release) {
		return false;
	}

	return true;
}

const char *getGlVersion() {
	return reinterpret_cast<const char *>(glGetString(GL_VERSION));
}

const char *getGlRenderer() {
	return reinterpret_cast<const char *>(glGetString(GL_RENDERER));
}

const char *getGlVendor() {
	return reinterpret_cast<const char *>(glGetString(GL_VENDOR));
}

const char *getGlExtensions() {
	return reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
}

const char *getGlPlatformExtensions() {
	Context *c= GraphicsInterface::getInstance().getCurrentContext();
	return getPlatformExtensions(static_cast<ContextGl*>(c)->getPlatformContextGl());
}

int getGlMaxLights() {
	int i;
	glGetIntegerv(GL_MAX_LIGHTS, (GLint*)&i);
	return i;
}

int getGlMaxTextureSize() {
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&i);
	return i;
}

int getGlMaxTextureUnits() {
	int i;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, (GLint*)&i);
	return i;
}

int getGlModelviewMatrixStackDepth() {
	int i;
	glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, (GLint*)&i);
	return i;
}

int getGlProjectionMatrixStackDepth() {
	int i;
	glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, (GLint*)&i);
	return i;
}

void checkGlExtension(const char *extensionName) {
	if(!isGlExtensionSupported(extensionName)){
		throw runtime_error("OpenGL extension not supported: " + string(extensionName));
	}
}

}}}// end namespace
