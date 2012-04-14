// ==============================================================
//	This file is part of the MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2011 Mark Vejvoda and others
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifdef USE_FTGL

//#include "gettext.h"

#include "font_textFTGL.h"

#include "opengl.h"
#include <stdexcept>
#include <sys/stat.h>
#include <FTGL/ftgl.h>

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "platform_common.h"
#include "util.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics { namespace Gl {


string TextFTGL::langHeightText = "yW";
int TextFTGL::faceResolution 	= 72;

//====================================================================
TextFTGL::TextFTGL(FontTextHandlerType type) : Text(type) {

	//throw megaglest_runtime_error("FTGL!");
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",0);
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/arphic/uming.ttc",0); // Chinese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/arphic/ukai.ttc",0); // Chinese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-doulos/DoulosSILR.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-charis/CharisSILR.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/takao/TakaoPGothic.ttf",0); // Japanese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-scheherazade/ScheherazadeRegOT.ttf",0); // Arabic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/linux-libertine/LinLibertine_Re.ttf",0); // Hebrew
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/unifont/unifont.ttf",0); // Czech?
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-liberation/LiberationSans-Regular.ttf",0); // Czech?


	fontFile = findFont();
	//ftFont = new FTBufferFont(fontFile);
	//ftFont = new FTGLPixmapFont(fontFile);
	//ftFont = new FTGLExtrdFont(fontFile);
	//ftFont = new FTGLPolygonFont("/usr/share/fonts/truetype/arphic/uming.ttc");

	//ftFont = new FTGLPixmapFont("/usr/share/fonts/truetype/arphic/uming.ttc");
	if(type == ftht_2D) {
		ftFont = new FTGLPixmapFont(fontFile);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("2D font [%s]\n",fontFile);
	}
	else if(type == ftht_3D) {
		ftFont = new FTGLTextureFont(fontFile);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("3D font [%s]\n",fontFile);
	}
	else {
		throw megaglest_runtime_error("font render type not set to a known value!");
	}

	if(ftFont->Error())	{
		printf("FTGL: error loading font: %s\n", fontFile);
		delete ftFont; ftFont = NULL;
		free((void*)fontFile);
		fontFile = NULL;
		throw megaglest_runtime_error(string("FTGL: error loading font: ") + string(fontFile));
	}
	free((void*)fontFile);
	fontFile = NULL;

	const unsigned int defSize = 24;
	ftFont->FaceSize(defSize,TextFTGL::faceResolution);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\n[%s::%s] Line %d Error = %d [%s] for size = %d res = %d\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),defSize,TextFTGL::faceResolution);
		fflush(stdout);
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error setting face size, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}
	//ftFont->UseDisplayList(false);
	//ftFont->CharMap(ft_encoding_gb2312);
	//ftFont->CharMap(ft_encoding_big5);
	if(ftFont->CharMap(ft_encoding_unicode) == false) {
		throw megaglest_runtime_error("FTGL: error setting encoding");
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error setting encoding, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}
}

TextFTGL::~TextFTGL() {
	cleanupFont();
}

void TextFTGL::cleanupFont() {
	delete ftFont;
	ftFont = NULL;

	free((void*)fontFile);
	fontFile = NULL;
}

void TextFTGL::init(string fontName, string fontFamilyName, int fontSize) {
	cleanupFont();
	fontFile = findFont(fontName.c_str(),fontFamilyName.c_str());

	//ftFont = new FTBufferFont(fontFile);
	//ftFont = new FTGLPixmapFont(fontFile);
	//ftFont = new FTGLExtrdFont(fontFile);
	//ftFont = new FTGLPolygonFont("/usr/share/fonts/truetype/arphic/uming.ttc");
	//ftFont = new FTGLPixmapFont("/usr/share/fonts/truetype/arphic/uming.ttc");

	if(type == ftht_2D) {
		ftFont = new FTGLPixmapFont(fontFile);
		//printf("2D font [%s]\n",fontFile);
	}
	else if(type == ftht_3D) {
		ftFont = new FTGLTextureFont(fontFile);

		//ftFont = new FTBufferFont(fontFile);
		//ftFont = new FTGLExtrdFont(fontFile);
		//ftFont = new FTGLPolygonFont("/usr/share/fonts/truetype/arphic/uming.ttc");

		//printf("3D font [%s]\n",fontFile);
	}
	else {
		throw megaglest_runtime_error("font render type not set to a known value!");
	}

	if(ftFont->Error())	{
		printf("FTGL: error loading font: %s\n", fontFile);
		delete ftFont; ftFont = NULL;
		free((void*)fontFile);
		fontFile = NULL;
		throw megaglest_runtime_error("FTGL: error loading font");
	}
	free((void*)fontFile);
	fontFile = NULL;

	if(fontSize <= 0) {
		fontSize = 24;
	}
	ftFont->FaceSize(fontSize,TextFTGL::faceResolution);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\n[%s::%s] Line %d Error = %d [%s] for size = %d res = %d\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),fontSize,TextFTGL::faceResolution);
		fflush(stdout);
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error setting face size, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}

	//ftFont->UseDisplayList(false);
	//ftFont->CharMap(ft_encoding_gb2312);
	//ftFont->CharMap(ft_encoding_big5);
	if(ftFont->CharMap(ft_encoding_unicode) == false) {
		throw megaglest_runtime_error("FTGL: error setting encoding");
	}
	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error setting encoding, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}

	// Create a string containing common characters
	// and preload the chars without rendering them.
	string preloadText = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890-=!@#$%^&*()_+:\"{}[]/?.,<>\\';";
	ftFont->Advance(preloadText.c_str());

	error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),preloadText.c_str());
		fflush(stdout);
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error advancing(a), #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}
}

void TextFTGL::SetFaceSize(int value) {
	ftFont->FaceSize(value,TextFTGL::faceResolution);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\n[%s::%s] Line %d Error = %d [%s] for facesize = %d\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),value);
		fflush(stdout);
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error setting face size, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}
}

int TextFTGL::GetFaceSize() {
	return ftFont->FaceSize();
}

void TextFTGL::Render(const char* str, const int len) {
	//printf("Render TextFTGL\n");

	/*
	  FTGL renders the whole string when len == 0
	  but we don't want any text rendered then.
	*/
	if(len != 0) {
		//printf("FTGL Render [%s] facesize = %d\n",str,ftFont->FaceSize());
		assertGl();

		ftFont->Render(str, len);
		//assertGl();
		GLenum error = glGetError();
		if(error != GL_NO_ERROR) {
			printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
			fflush(stdout);
		}

		if(ftFont->Error())	{
			char szBuf[1024]="";
			sprintf(szBuf,"FTGL: error trying to render, #%d",ftFont->Error());
			throw megaglest_runtime_error(szBuf);
		}
	}
}

float TextFTGL::Advance(const char* str, const int len) {
	float result = ftFont->Advance(str, len);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
		fflush(stdout);
	}

	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error trying to advance(b), #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}
	return result;

	//FTBBox box = ftFont->BBox(str);
	//float urx = box.Upper().X();
	//float llx = box.Lower().X();
	//float llx, lly, llz, urx, ury, urz;
	//ftFont->BBox(str, llx, lly, llz, urx, ury, urz);

	//Short_t halign = fTextAlign/10;
	//Short_t valign = fTextAlign - 10*halign;
	//Float_t dx = 0, dy = 0;
//	switch (halign) {
//	  case 1 : dx =  0    ; break;
//	  case 2 : dx = -urx/2; break;
//	  case 3 : dx = -urx  ; break;
//	}
//	switch (valign) {
//	  case 1 : dy =  0    ; break;
//	  case 2 : dy = -ury/2; break;
//	  case 3 : dy = -ury  ; break;
//	}

	//printf("For str [%s] advance = %f, urx = %f, llx = %f\n",str, ftFont->Advance(str, len),urx,llx);
	//return urx;
}

float TextFTGL::LineHeight(const char* str, const int len) {
	//FTBBox box = ftFont->BBox(str);
	//printf("String [%s] lineheight = %f upper_y = %f lower_y = %f\n",str,ftFont->LineHeight(),box.Upper().Y(),box.Lower().Y());


	//printf("ftFont->Ascender():%f ftFont->Descender()*-1 = %f ftFont->LineHeight() = %f\n",ftFont->Ascender(),ftFont->Descender()*-1 , ftFont->LineHeight());
	//return ftFont->Ascender() + ftFont->Descender()*-1 - ftFont->LineHeight();
	//return ftFont->LineHeight();

	//static float result = -1000;
	float result = -1000;
	if(result == -1000) {
		FTBBox box = ftFont->BBox(TextFTGL::langHeightText.c_str());

		GLenum error = glGetError();
		if(error != GL_NO_ERROR) {
			printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
			fflush(stdout);
		}

		result = box.Upper().Y()- box.Lower().Y();
		if(result == 0) {
			result = ftFont->LineHeight();

			GLenum error = glGetError();
			if(error != GL_NO_ERROR) {
				printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
				fflush(stdout);
			}
		}
		//printf("ftFont->BBox(''yW'')%f\n",result);
	}
//	else {
//		FTBBox box = ftFont->BBox(TextFTGL::langHeightText.c_str());
//
//		GLenum error = glGetError();
//		if(error != GL_NO_ERROR) {
//			printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
//			fflush(stdout);
//		}
//
//		int newresult = box.Upper().Y()- box.Lower().Y();
//		if(newresult == 0) {
//			newresult = ftFont->LineHeight();
//
//			GLenum error = glGetError();
//			if(error != GL_NO_ERROR) {
//				printf("\n[%s::%s] Line %d Error = %d [%s] for text [%s]\n",__FILE__,__FUNCTION__,__LINE__,error,gluErrorString(error),str);
//				fflush(stdout);
//			}
//		}
//
//		printf("Height for [%s] result [%d] [%d]\n",str,result,newresult);
//	}
	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error trying to get lineheight, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}

	return result;
//	printf("For str [%s] LineHeight = %f, result = %f\n",str, ftFont->LineHeight(),result);
//	return result;

	//float urx = box.Upper().X();
	//float llx = box.Lower().X();
	//float llx, lly, llz, urx, ury, urz;
	//ftFont->BBox(str, llx, lly, llz, urx, ury, urz);
	//return  ury - lly;

	//Short_t halign = fTextAlign/10;
	//Short_t valign = fTextAlign - 10*halign;
	//Float_t dx = 0, dy = 0;
//	switch (halign) {
//	  case 1 : dx =  0    ; break;
//	  case 2 : dx = -urx/2; break;
//	  case 3 : dx = -urx  ; break;
//	}
//	switch (valign) {
//	  case 1 : dy =  0    ; break;
//	  case 2 : dy = -ury/2; break;
//	  case 3 : dy = -ury  ; break;
//	}

	//printf("For str [%s] advance = %f, urx = %f, llx = %f\n",str, ftFont->Advance(str, len),urx,llx);
	//return urx;

}

float TextFTGL::LineHeight(const wchar_t* str, const int len) {
	static float result = -1000;
	if(result == -1000) {
		FTBBox box = ftFont->BBox(TextFTGL::langHeightText.c_str());
		result = box.Upper().Y()- box.Lower().Y();
		if(result == 0) {
			result = ftFont->LineHeight();
		}
		//printf("ftFont->BBox(''yW'')%f\n",result);
	}
	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error trying to get lineheight, #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}

	return result;
}

void TextFTGL::Render(const wchar_t* str, const int len) {
	/*
	  FTGL renders the whole string when len == 0
	  but we don't want any text rendered then.
	*/
	if(len != 0) {
		ftFont->Render(str, len);

		if(ftFont->Error())	{
			char szBuf[1024]="";
			sprintf(szBuf,"FTGL: error trying to render, #%d",ftFont->Error());
			throw megaglest_runtime_error(szBuf);
		}
	}
}

float TextFTGL::Advance(const wchar_t* str, const int len) {
	float result = ftFont->Advance(str, len);
	if(ftFont->Error())	{
		char szBuf[1024]="";
		sprintf(szBuf,"FTGL: error trying to advance(c), #%d",ftFont->Error());
		throw megaglest_runtime_error(szBuf);
	}

	return result;
}

}}}//end namespace

#endif // USE_FTGL
