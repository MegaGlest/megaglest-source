//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under 
//the terms of the GNU General Public License as published by the Free Software 
//Foundation; either version 2 of the License, or (at your option) any later 
//version.
#include "gl_wrap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

#import <Cocoa/Cocoa.h>

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
//#include "window.h"
#include "font.h"
#include <vector>
//#include <SDL_image.h>
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Graphics;

namespace Shared{ namespace Platform{
	
// ======================================
//	Global Fcs  
// ======================================

BOOL makeDisplayList(GLint listNum, NSImage *theImage)
{
   NSBitmapImageRep *bitmap;
   int bytesPerRow, pixelsHigh, pixelsWide, samplesPerPixel;
   unsigned char *bitmapBytes;
   int currentBit, byteValue;
   unsigned char *newBuffer, *movingBuffer;
   int rowIndex, colIndex;

   bitmap = [ NSBitmapImageRep imageRepWithData:[ theImage
                          TIFFRepresentationUsingCompression:NSTIFFCompressionNone
                          factor:0 ] ];
   pixelsHigh = [ bitmap pixelsHigh ];
   pixelsWide = [ bitmap pixelsWide ];
   bitmapBytes = [ bitmap bitmapData ];
   bytesPerRow = [ bitmap bytesPerRow ];
   samplesPerPixel = [ bitmap samplesPerPixel ];
   newBuffer = (unsigned char *)calloc( ceil( (float) bytesPerRow / 8.0 ), pixelsHigh );
   if( newBuffer == NULL )
   {
      NSLog(@"Failed to calloc() memory in makeDisplayList()");
      return FALSE;
   }

   movingBuffer = newBuffer;
   /*
    * Convert the color bitmap into a true bitmap, ie, one bit per pixel.  We
    * read at last row, write to first row as Cocoa and OpenGL have opposite
    * y origins
    */
   for( rowIndex = pixelsHigh - 1; rowIndex >= 0; rowIndex-- )
   {
      currentBit = 0x80;
      byteValue = 0;
      for( colIndex = 0; colIndex < pixelsWide; colIndex++ )
      {
         if( bitmapBytes[ rowIndex * bytesPerRow + colIndex * samplesPerPixel ] )
            byteValue |= currentBit;
         currentBit >>= 1;
         if( currentBit == 0 )
         {
            *movingBuffer++ = byteValue;
            currentBit = 0x80;
            byteValue = 0;
         }
      }
      /*
       * Fill out the last byte; extra is ignored by OpenGL, but each row
       * must start on a new byte
       */
      if( currentBit != 0x80 )
         *movingBuffer++ = byteValue;
   }

   glNewList( listNum, GL_COMPILE );
   glBitmap( pixelsWide, pixelsHigh, 0, 0, pixelsWide, 0, newBuffer );
   glEndList();
   free( newBuffer );

   return TRUE;
}

/*
 * Create the set of display lists for the bitmaps
 */
BOOL makeGLDisplayListFirst(unichar first, int count, GLint base, NSFont *font, Shared::Graphics::FontMetrics &metrics)
{
	GLint curListIndex;
	NSColor *blackColor = [NSColor blackColor];
	GLint dListNum;
	NSString *currentChar;
	unichar currentUnichar;
	NSSize charSize;
	NSRect charRect;
	NSImage *theImage;
	BOOL retval;

//	float ascent = [font ascender];
//	float descent = [font descender];
	
 	NSDictionary *attribDict = [ NSDictionary dictionaryWithObjectsAndKeys:
								font, NSFontAttributeName,
								[NSColor whiteColor],
								NSForegroundColorAttributeName,
								[NSColor blackColor], NSBackgroundColorAttributeName,
								nil ];

	// Make sure a list isn't already under construction
	glGetIntegerv( GL_LIST_INDEX, &curListIndex );
	if( curListIndex != 0 )
	{
	  NSLog(@"Display list already under construction");
	  return FALSE;
	}

	// Save pixel unpacking state
	glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );

	glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	glPixelStorei( GL_UNPACK_LSB_FIRST, GL_FALSE );
	glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
	glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	charRect.origin.x = charRect.origin.y = 0;
	theImage = [ [ [ NSImage alloc ] initWithSize:NSMakeSize( 0, 0 ) ]
				autorelease ];
	retval = TRUE;
	float fontHeight = metrics.getHeight();
	float dy;
	
   for( dListNum = base, currentUnichar = first; currentUnichar < first + count;
        dListNum++, currentUnichar++ )
   {
      currentChar = [ NSString stringWithCharacters:&currentUnichar length:1 ];
	 
      charSize = [ currentChar sizeWithAttributes:attribDict ];
      charRect.size = charSize;
      charRect = NSIntegralRect( charRect );
	  dy = charRect.size.height - fontHeight;
	  metrics.setWidth(dListNum-base, charRect.size.width);
	  //metrics.setVirticalOffset(dListNum-base, fontHeight - charRect.size.height);
	  
      if( charRect.size.width > 0 && charRect.size.height > 0 )
      {
		charRect.size.height = fontHeight;
         [ theImage setSize:charRect.size ];
         [ theImage lockFocus ];
         [ [ NSGraphicsContext currentContext ] setShouldAntialias:NO ];
         [ blackColor set ];
         [ NSBezierPath fillRect:charRect ];
         //[ currentChar drawInRect:charRect withAttributes:attribDict ];
		 [ currentChar drawAtPoint:NSMakePoint(0, 0) withAttributes:attribDict ];
		
        [ theImage unlockFocus ];
         if( !makeDisplayList(dListNum, theImage) )
         {
            retval = FALSE;
            break;
         }
      }
   }
   glPopClientAttrib();
	
   return retval;
}

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
						 int charCount, FontMetrics &metrics) {
	
	
	//@FF@ keep the reduction ratio ???
	size = (float)size * 0.80;
	NSFont *font = [NSFont fontWithName:[NSString stringWithCString:"Arial" encoding:NSUTF8StringEncoding] size:size];
	if( font == nil )
      NSLog( @"font is nil\n" );
	
	float fontHeight = [font ascender] - [font descender];
	
	metrics.setHeight(fontHeight);

    if( !makeGLDisplayListFirst('\0', charCount, base, font, metrics) )
		NSLog( @"Didn't make display list\n" );

}

void createGlFontOutlines(uint32 &base, const string &type, int width,
						  float depth, int charCount, FontMetrics &metrics) {
	NOIMPL;
}
}}//end namespace 
