// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "text_renderer_gl.h"

#include "opengl.h"
#include "font_gl.h"

#include "font_text.h"
#include <vector>
#include <algorithm>
#include "leak_dumper.h"

namespace Shared { namespace Graphics { namespace Gl {

// =====================================================
//	class TextRenderer2DGl
// =====================================================

TextRenderer2DGl::TextRenderer2DGl() {
	rendering= false;
	this->font = NULL;

	//font3D = NULL;
	//tester = new TextRenderer3DGl();
}

TextRenderer2DGl::~TextRenderer2DGl() {
	//delete font3D;
	//font3D = NULL;

	//delete tester;
	//tester = NULL;
}

void TextRenderer2DGl::begin(Font2D *font) {
	this->font	= static_cast<Font2DGl*>(font);

//	if(font3D == NULL) {
//		font3D = new Font3DGl();
//		font3D->setYOffsetFactor(this->font->getYOffsetFactor());
//		font3D->setType("", this->font->getType());
//		font3D->setDepth(this->font->getWidth());
//	}
//	tester->begin(font3D);
//	return;

	assert(!rendering);
	rendering	= true;
}

// Convert a narrow string to a wide string//
//std::wstring widen(const std::string& str) {
//	// Make space for wide string
//	wchar_t* buffer = new wchar_t[str.size() + 1];
//	// convert ASCII to UNICODE
//	mbstowcs( buffer, str.c_str(), str.size() );
//	// NULL terminate it
//	buffer[str.size()] = 0;
//	// Clean memory and return it
//	std::wstring wstr = buffer;
//	delete [] buffer;
//	return wstr;
//}
// Widen an individual character

void TextRenderer2DGl::render(const string &text, int x, int y, bool centered, Vec3f *color) {
	//tester->render(text, x, y, this->font->getWidth(),centered);
	//return;

	assert(rendering);
	
	assertGl();

	if(color != NULL) {
		glPushAttrib(GL_CURRENT_BIT);
		glColor3fv(color->ptr());
	}

	int line					= 0;
	int size					= font->getSize();
	const unsigned char *utext	= NULL;
	FontMetrics *metrics		= NULL;

	//printf("font->getTextHandler() [%p] centered = %d text [%s]\n",font->getTextHandler(),centered,text.c_str());

	Vec2f rasterPos;
	if(font->getTextHandler() != NULL) {
		if(centered) {
			rasterPos.x= x - font->getTextHandler()->Advance(text.c_str()) / 2.f;
			rasterPos.y= y + font->getTextHandler()->LineHeight(text.c_str()) / font->getYOffsetFactor();
		}
		else {
			rasterPos= Vec2f(static_cast<float>(x), static_cast<float>(y));
			rasterPos.y= y + (font->getTextHandler()->LineHeight(text.c_str()) / font->getYOffsetFactor());
		}
	}
	else {
		utext= reinterpret_cast<const unsigned char*>(text.c_str());
		metrics= font->getMetrics();
		if(centered) {
			rasterPos.x= x-metrics->getTextWidth(text)/2.f;
			rasterPos.y= y+metrics->getHeight()/2.f;
		}
		else {
			rasterPos= Vec2f(static_cast<float>(x), static_cast<float>(y));
		}
	}
	glRasterPos2f(rasterPos.x, rasterPos.y);

	//font->getTextHandler()->Render("Zurück");
	//return;

	if(Font::fontIsMultibyte == true) {
		if(font->getTextHandler() != NULL) {
			//String str("資料");
			//WString wstr(str);
			//fontFTGL->Render(wstr.cw_str());

			//String str(L"資料");
			//WString wstr(str);
			//fontFTGL->Render(wstr.cw_str());

			//WString wstr(L"資料");
			//fontFTGL->Render(wstr.cw_str());

			//size_t length = text.length();

	//		string temp = String::ConvertToUTF8("資料");
	//		size_t length = temp.length();
	//		wchar_t *utf32 = (wchar_t*)malloc((length+1) * sizeof(wchar_t));
	//		mbstowcs(utf32, temp.c_str(), length);
	//		utf32[length] = 0;
	//		fontFTGL->Render(utf32);
	//		free(utf32);

			//wstring wstr(L"資料");
			//fontFTGL->Render(wstr.c_str());

			//fontFTGL->Render(text.c_str());

			//const wchar_t *str = L" 中文";
			//fontFTGL->Render(str);

			//fontFTGL->Render(" 中文"); // Chinese Works!
			//fontFTGL->Render("ёшзхсдертбнйуимлопьжющэъ́"); // Russian Works!
			//fontFTGL->Render("更新履歴はこちらをご覧下さい。"); // Japanese Works!

			//fontFTGL->Render("مرحبا العالم"); //Arabic Works!
			//wstring temp = L"مرحبا العالم";
			//temp = wstring (temp.rbegin(), temp.rend());
			//fontFTGL->Render(temp.c_str());

			// Hebrew is Right To Left
			//wstring temp = L"שלום העולם";
			//temp = wstring (temp.rbegin(), temp.rend());
			//fontFTGL->Render(temp.c_str());

			//fontFTGL->Render("testování slovanský jazyk"); // Czech Works!

			// This works
			//fontFTGL->Render(text.c_str());

			if(text.find("\n") == text.npos && text.find("\t") == text.npos) {
				font->getTextHandler()->Render(text.c_str());
			}
			else {
				bool lastCharacterWasSpecial = true;
				vector<string> parts;
				char szBuf[4096]="";

				for (int i=0; text[i] != '\0'; ++i) {
					szBuf[0] = '\0';
					sprintf(szBuf,"%c",text[i]);

					switch(text[i]) {
						case '\t':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//line++;
							//rasterPos= Vec2f(static_cast<float>(x), y - (fontFTGL->LineHeight(text.c_str()) * 2.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							//glCallList(font->getHandle()+utext[i]);
							if(lastCharacterWasSpecial == true) {
								parts.push_back(szBuf);
							}
							else {
								parts[parts.size()-1] += szBuf;
							}
							lastCharacterWasSpecial = false;
					}
				}

				for (unsigned int i=0; i < parts.size(); ++i) {
					switch(parts[i][0]) {
						case '\t':
							rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							line++;
							rasterPos= Vec2f(static_cast<float>(x), y - (font->getTextHandler()->LineHeight(parts[i].c_str())) * line);
							glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							font->getTextHandler()->Render(parts[i].c_str());
					}
				}
			}
		}
		else {
			//setlocale(LC_CTYPE, "en_ca.UTF-8");

			//wstring wText = widen(text);
			//glListBase(font->getHandle());
			//glCallLists(wText.length(), GL_UNSIGNED_SHORT, &wText[0]);

			//string utfText = text;
			//glListBase(font->getHandle());
			//glCallLists(utfText.length(), GL_UNSIGNED_SHORT, &utfText[0]);

			string utfText = text;
			glListBase(font->getHandle());
			glCallLists(text.length(), GL_UNSIGNED_SHORT, &utext[0]);

			//std::locale loc("");
			//wstring wText = widen(text);
			//std::string strBuffer(Text.size() * 4 + 1, 0);
			//std::use_facet<std::ctype<wchar_t> >(loc).narrow(&Text[0], &Text[0] + Text.size(), '?', &strBuffer[0]);
			//string utfText = std::string(&strBuffer[0]);
			//glListBase(font->getHandle());
			//glCallLists(utfText.length(), GL_UNSIGNED_SHORT, &utfText[0]);
		}
	}
	else {
		if(font->getTextHandler() != NULL) {
			if(text.find("\n") == text.npos && text.find("\t") == text.npos) {
				font->getTextHandler()->Render(text.c_str());
			}
			else {
				bool lastCharacterWasSpecial = true;
				vector<string> parts;
				char szBuf[4096]="";

				for (int i=0; text[i] != '\0'; ++i) {
					szBuf[0] = '\0';
					sprintf(szBuf,"%c",text[i]);

					switch(text[i]) {
						case '\t':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//line++;
							//rasterPos= Vec2f(static_cast<float>(x), y - (fontFTGL->LineHeight(text.c_str()) * 2.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							//glCallList(font->getHandle()+utext[i]);
							if(lastCharacterWasSpecial == true) {
								parts.push_back(szBuf);
							}
							else {
								parts[parts.size()-1] += szBuf;
							}
							lastCharacterWasSpecial = false;
					}
				}

				for (unsigned int i=0; i < parts.size(); ++i) {
					switch(parts[i][0]) {
						case '\t':
							rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							line++;
							rasterPos= Vec2f(static_cast<float>(x), y - (font->getTextHandler()->LineHeight(parts[i].c_str())) * line);
							glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							font->getTextHandler()->Render(parts[i].c_str());
					}
				}
			}
		}
		else {
			for (int i=0; utext[i]!='\0'; ++i) {
				switch(utext[i]){
				case '\t':
					rasterPos= Vec2f((rasterPos.x/size+3.f)*size, y-(size+1.f)*line);
					glRasterPos2f(rasterPos.x, rasterPos.y);
					break;
				case '\n':
					line++;
					rasterPos= Vec2f(static_cast<float>(x), y-(metrics->getHeight()*2.f)*line);
					glRasterPos2f(rasterPos.x, rasterPos.y);
					break;
				default:
					glCallList(font->getHandle()+utext[i]);
				}
			}
		}
	}

	if(color != NULL) {
		glPopAttrib();
	}
	assertGl();
}

void TextRenderer2DGl::end() {
	//tester->end();
	//return;

	assert(rendering);
	rendering= false;
}

// =====================================================
//	class TextRenderer3DGl
// =====================================================

TextRenderer3DGl::TextRenderer3DGl() {
	rendering= false;
	this->font = NULL;
}

TextRenderer3DGl::~TextRenderer3DGl() {
}

void TextRenderer3DGl::begin(Font3D *font) {
	assert(!rendering);
	rendering= true;
	
	this->font= static_cast<Font3DGl*>(font);

	assertGl();

	//load color
	glPushAttrib(GL_TRANSFORM_BIT);

	assertGl();
}

void TextRenderer3DGl::render(const string &text, float  x, float y, bool centered) {
	assert(rendering);
	
	const unsigned char *utext= NULL;
	assertGl();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_POLYGON_BIT);

	int size = font->getSize();
	//float scale= size / 15.f;
	float scale= 1.0f;
	//float scale= size;
	Vec3f translatePos;

	if(font->getTextHandler() != NULL) {
		if(centered) {
			translatePos.x = x - scale * font->getTextHandler()->Advance(text.c_str()) / 2.f;
			translatePos.y = y - scale * font->getTextHandler()->LineHeight(text.c_str()) / font->getYOffsetFactor();
			translatePos.z = 0;
		}
		else {
			translatePos.x = x-scale;
			translatePos.y = y-scale;
			translatePos.z = 0;
		}
	}
	else {
		utext= reinterpret_cast<const unsigned char*>(text.c_str());
		if(centered) {
			FontMetrics *metrics= font->getMetrics();
			//glTranslatef(x-scale*metrics->getTextWidth(text)/2.f, y-scale*metrics->getHeight()/2.f, 0);
			translatePos.x = x-scale*metrics->getTextWidth(text)/2.f;
			translatePos.y = y-scale*metrics->getHeight()/2.f;
			translatePos.z = 0;
		}
		else {
			//glTranslatef(x-scale, y-scale, 0);
			translatePos.x = x-scale;
			translatePos.y = y-scale;
			translatePos.z = 0;
		}
	}

	//glScalef(scale, scale, scale);
	glTranslatef(translatePos.x, translatePos.y, translatePos.z);

	font->getTextHandler()->Render(text.c_str());

/*
	if(Font::fontIsMultibyte == true) {
		if(font->getTextHandler() != NULL) {
			if(text.find("\n") == text.npos && text.find("\t") == text.npos) {
				font->getTextHandler()->Render(text.c_str());
			}
			else {
				int line=0;
				bool lastCharacterWasSpecial = true;
				vector<string> parts;
				char szBuf[4096]="";

				for (int i=0; text[i] != '\0'; ++i) {
					szBuf[0] = '\0';
					sprintf(szBuf,"%c",text[i]);

					switch(text[i]) {
						case '\t':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//line++;
							//rasterPos= Vec2f(static_cast<float>(x), y - (fontFTGL->LineHeight(text.c_str()) * 2.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							//glCallList(font->getHandle()+utext[i]);
							if(lastCharacterWasSpecial == true) {
								parts.push_back(szBuf);
							}
							else {
								parts[parts.size()-1] += szBuf;
							}
							lastCharacterWasSpecial = false;
					}
				}

				for (unsigned int i=0; i < parts.size(); ++i) {
					switch(parts[i][0]) {
						case '\t':
							translatePos= Vec3f((translatePos.x / size + 3.f) * size, y-(size + 1.f) * line, translatePos.z);
							glTranslatef(translatePos.x, translatePos.y, translatePos.z);
							break;
						case '\n':
							line++;
							translatePos= Vec3f(static_cast<float>(x), y - (font->getTextHandler()->LineHeight(parts[i].c_str())) * line, translatePos.z);
							glTranslatef(translatePos.x, translatePos.y, translatePos.z);
							break;
						default:
							font->getTextHandler()->Render(parts[i].c_str());
					}
				}
			}
		}
		else {
			//setlocale(LC_CTYPE, "en_ca.UTF-8");

			//wstring wText = widen(text);
			//glListBase(font->getHandle());
			//glCallLists(wText.length(), GL_UNSIGNED_SHORT, &wText[0]);

			//string utfText = text;
			//glListBase(font->getHandle());
			//glCallLists(utfText.length(), GL_UNSIGNED_SHORT, &utfText[0]);

			string utfText = text;
			glListBase(font->getHandle());
			glCallLists(text.length(), GL_UNSIGNED_SHORT, &utext[0]);

			//std::locale loc("");
			//wstring wText = widen(text);
			//std::string strBuffer(Text.size() * 4 + 1, 0);
			//std::use_facet<std::ctype<wchar_t> >(loc).narrow(&Text[0], &Text[0] + Text.size(), '?', &strBuffer[0]);
			//string utfText = std::string(&strBuffer[0]);
			//glListBase(font->getHandle());
			//glCallLists(utfText.length(), GL_UNSIGNED_SHORT, &utfText[0]);
		}
	}
	else {

		if(font->getTextHandler() != NULL) {
			if(text.find("\n") == text.npos && text.find("\t") == text.npos) {
				font->getTextHandler()->Render(text.c_str());
			}
			else {
				int line=0;
				bool lastCharacterWasSpecial = true;
				vector<string> parts;
				char szBuf[4096]="";

				for (int i=0; text[i] != '\0'; ++i) {
					szBuf[0] = '\0';
					sprintf(szBuf,"%c",text[i]);

					switch(text[i]) {
						case '\t':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//rasterPos= Vec2f((rasterPos.x / size + 3.f) * size, y-(size + 1.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						case '\n':
							parts.push_back(szBuf);
							lastCharacterWasSpecial = true;
							//line++;
							//rasterPos= Vec2f(static_cast<float>(x), y - (fontFTGL->LineHeight(text.c_str()) * 2.f) * line);
							//glRasterPos2f(rasterPos.x, rasterPos.y);
							break;
						default:
							//glCallList(font->getHandle()+utext[i]);
							if(lastCharacterWasSpecial == true) {
								parts.push_back(szBuf);
							}
							else {
								parts[parts.size()-1] += szBuf;
							}
							lastCharacterWasSpecial = false;
					}
				}

				for (unsigned int i=0; i < parts.size(); ++i) {
					switch(parts[i][0]) {
						case '\t':
							translatePos= Vec3f((translatePos.x / size + 3.f) * size, y-(size + 1.f) * line, translatePos.z);
							glTranslatef(translatePos.x, translatePos.y, translatePos.z);
							break;
						case '\n':
							line++;
							translatePos= Vec3f(static_cast<float>(x), y - (font->getTextHandler()->LineHeight(parts[i].c_str())) * line, translatePos.z);
							glTranslatef(translatePos.x, translatePos.y, translatePos.z);
							break;
						default:
							font->getTextHandler()->Render(parts[i].c_str());
					}
				}
			}
		}
		else {
			for (int i=0; utext[i]!='\0'; ++i) {
				glCallList(font->getHandle()+utext[i]);
			}
		}
	}
*/

	glPopMatrix();
	glPopAttrib();

	assertGl();
}

void TextRenderer3DGl::end() {
	assert(rendering);
	rendering= false;

	assertGl();

	glPopAttrib();

	assertGl();
}

}}}//end namespace
