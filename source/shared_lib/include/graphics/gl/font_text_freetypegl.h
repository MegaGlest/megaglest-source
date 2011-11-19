// ==============================================================
//	This file is part of the MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2011 Mark Vejvoda and others
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 3 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef TextFreetypeGL_h
#define TextFreetypeGL_h

#ifdef USE_FREETYPEGL

//#include <stdlib.h>
#include <wchar.h>

//#include "vector.h"
#include "texture-font.h"
#include "texture-atlas.h"
#include "vertex-buffer.h"
#include "font-manager.h"

#include "font_text.h"
#include "vec.h"

namespace Shared { namespace Graphics { namespace Gl {

/**
 * Use Freetype-GL for rendering text in OpenGL
 */
//====================================================================
class TextFreetypeGL : public Text
{
public:

	static string langHeightText;
	static int faceResolution;

	TextFreetypeGL(FontTextHandlerType type);
	virtual ~TextFreetypeGL();
	virtual void init(string fontName, int fontSize);

	virtual void SetFaceSize(int);
	virtual int GetFaceSize();

	virtual void Render(const char*, const int = -1);
	virtual float Advance(const char*, const int = -1);
	virtual float LineHeight(const char*, const int = -1);

	virtual void Render(const wchar_t*, const int = -1);
	virtual float Advance(const wchar_t*, const int = -1);
	virtual float LineHeight(const wchar_t* = L" ", const int = -1);

private:

	string lastTextRendered;
	Vec4f lastTextColorRendered;

	VertexBuffer *buffer;
	TextureAtlas *atlas;
    TextureFont *font;
    FontManager *manager;
    int fontFaceSize;

    string fontName;
	const char* fontFile;

	void cleanupFont();
};

}}}//end namespace

/**
 * Provides a way to easily walk multibyte unicode strings in the various
 * Unicode encodings (UTF-8, UTF-16, UTF-32, UCS-2, and UCS-4).  Encodings
 * with elements larger than one byte must already be in the correct endian
 * order for the current architecture.
 */
template <typename T>
class FreetypeGLUnicodeStringItr
{
public:
    /**
     * Constructor.  Also reads the first character and stores it.
     *
     * @param string  The buffer to iterate.  No copy is made.
     */
	FreetypeGLUnicodeStringItr(const T* string) : curPos(string), nextPos(string)
    {
        (*this)++;
    };

    /**
     * Pre-increment operator.  Reads the next unicode character and sets
     * the state appropriately.
     * Note - not protected against overruns.
     */
	FreetypeGLUnicodeStringItr& operator++()
    {
        curPos = nextPos;
        // unicode handling
        switch (sizeof(T))
        {
            case 1: // UTF-8
                // get this character
                readUTF8(); break;
            case 2: // UTF-16
                readUTF16(); break;
            case 4: // UTF-32
                // fall through
            default: // error condition really, but give it a shot anyway
                curChar = *nextPos++;
        }
        return *this;
    }

    /**
     * Post-increment operator.  Reads the next character and sets
     * the state appropriately.
     * Note - not protected against overruns.
     */
	FreetypeGLUnicodeStringItr operator++(int)
    {
		FreetypeGLUnicodeStringItr temp = *this;
        ++*this;
        return temp;
    }

    /**
     * Equality operator.  Two FreetypeGLUnicodeStringItrs are considered equal
     * if they have the same current buffer and buffer position.
     */
    bool operator==(const FreetypeGLUnicodeStringItr& right) const
    {
        if (curPos == right.getBufferFromHere())
            return true;
        return false;
    }

    /**
     * Dereference operator.
     *
     * @return  The unicode codepoint of the character currently pointed
     * to by the FreetypeGLUnicodeStringItr.
     */
    unsigned int operator*() const
    {
        return curChar;
    }

    /**
     * Buffer-fetching getter.  You can use this to retreive the buffer
     * starting at the currently-iterated character for functions which
     * require a Unicode string as input.
     */
    const T* getBufferFromHere() const { return curPos; }

private:
    /**
     * Helper function for reading a single UTF8 character from the string.
     * Updates internal state appropriately.
     */
    void readUTF8();

    /**
     * Helper function for reading a single UTF16 character from the string.
     * Updates internal state appropriately.
     */
    void readUTF16();

    /**
     * The buffer position of the first element in the current character.
     */
    const T* curPos;

    /**
     * The character stored at the current buffer position (prefetched on
     * increment, so there's no penalty for dereferencing more than once).
     */
    unsigned int curChar;

    /**
     * The buffer position of the first element in the next character.
     */
    const T* nextPos;

    // unicode magic numbers
    static const unsigned char utf8bytes[256];
    static const unsigned long offsetsFromUTF8[6];
    static const unsigned long highSurrogateStart;
    static const unsigned long highSurrogateEnd;
    static const unsigned long lowSurrogateStart;
    static const unsigned long lowSurrogateEnd;
    static const unsigned long highSurrogateShift;
    static const unsigned long lowSurrogateBase;
};

/* The first character in a UTF8 sequence indicates how many bytes
 * to read (among other things) */
template <typename T>
const unsigned char FreetypeGLUnicodeStringItr<T>::utf8bytes[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
};

/* Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence. */
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
  0x03C82080UL, 0xFA082080UL, 0x82082080UL };

// get a UTF8 character; leave the tracking pointer at the start of the
// next character
// not protected against invalid UTF8
template <typename T>
inline void FreetypeGLUnicodeStringItr<T>::readUTF8()
{
    unsigned int ch = 0;
    unsigned int extraBytesToRead = utf8bytes[(unsigned char)(*nextPos)];
    // falls through
    switch (extraBytesToRead)
    {
          case 6: ch += *nextPos++; ch <<= 6; /* remember, illegal UTF-8 */
          case 5: ch += *nextPos++; ch <<= 6; /* remember, illegal UTF-8 */
          case 4: ch += *nextPos++; ch <<= 6;
          case 3: ch += *nextPos++; ch <<= 6;
          case 2: ch += *nextPos++; ch <<= 6;
          case 1: ch += *nextPos++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead-1];
    curChar = ch;
}

// Magic numbers for UTF-16 conversions
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::highSurrogateStart = 0xD800;
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::highSurrogateEnd = 0xDBFF;
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::lowSurrogateStart = 0xDC00;
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::lowSurrogateEnd = 0xDFFF;
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::highSurrogateShift = 10;
template <typename T>
const unsigned long FreetypeGLUnicodeStringItr<T>::lowSurrogateBase = 0x0010000UL;

template <typename T>
inline void FreetypeGLUnicodeStringItr<T>::readUTF16()
{
    unsigned int ch = *nextPos++;
    // if we have the first half of the surrogate pair
    if (ch >= highSurrogateStart && ch <= highSurrogateEnd)
    {
        unsigned int ch2 = *curPos;
        // complete the surrogate pair
        if (ch2 >= lowSurrogateStart && ch2 <= lowSurrogateEnd)
        {
            ch = ((ch - highSurrogateStart) << highSurrogateShift)
                + (ch2 - lowSurrogateStart) + lowSurrogateBase;
            ++nextPos;
        }
    }
    curChar = ch;
}


#endif // USE_FREETYPEGL

#endif // TextFreetypeGL_h
