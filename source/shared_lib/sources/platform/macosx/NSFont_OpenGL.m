/*
 * This program is Copyright � 2002 Bryan L Blackburn.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the names Bryan L Blackburn, Withay.com, nor the names of any
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRYAN L BLACKBURN ``AS IS'' AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Initial version, 27 October, 2002
 */

/* NSFont_OpenGL.m */

#import "NSFont_OpenGL.h"

@interface NSFont (withay_OpenGL_InternalMethods)
- (BOOL) makeDisplayList:(GLint)listNum withImage:(NSImage *)theImage;
+ (void) doOpenGLLog:(NSString *)format, ...;
@end

@implementation NSFont (withay_OpenGL)

static BOOL openGLLoggingEnabled = YES;

/*
 * Enable/disable logging, class-wide, not object-wide
 */
+ (void) setOpenGLLogging:(BOOL)logEnabled
{
   openGLLoggingEnabled = logEnabled;
}


/*
 * Create the set of display lists for the bitmaps
 */
- (BOOL) makeGLDisplayListFirst:(unichar)first count:(int)count base:(GLint)base
{
   GLint curListIndex;
   NSColor *blackColor;
   NSDictionary *attribDict;
   GLint dListNum;
   NSString *currentChar;
   unichar currentUnichar;
   NSSize charSize;
   NSRect charRect;
   NSImage *theImage;
   BOOL retval;

   // Make sure a list isn't already under construction
   glGetIntegerv( GL_LIST_INDEX, &curListIndex );
   if( curListIndex != 0 )
   {
      [ NSFont doOpenGLLog:@"Display list already under construction" ];
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

   blackColor = [ NSColor blackColor ];
   attribDict = [ NSDictionary dictionaryWithObjectsAndKeys:
                                  self, NSFontAttributeName,
                                  [ NSColor whiteColor ],
                                     NSForegroundColorAttributeName,
                                  blackColor, NSBackgroundColorAttributeName,
                                  nil ];
   charRect.origin.x = charRect.origin.y = 0;
   theImage = [ [ [ NSImage alloc ] initWithSize:NSMakeSize( 0, 0 ) ]
                autorelease ];
   retval = TRUE;
   for( dListNum = base, currentUnichar = first; currentUnichar < first + count;
        dListNum++, currentUnichar++ )
   {
      currentChar = [ NSString stringWithCharacters:&currentUnichar length:1 ];
      charSize = [ currentChar sizeWithAttributes:attribDict ];
      charRect.size = charSize;
      charRect = NSIntegralRect( charRect );
      if( charRect.size.width > 0 && charRect.size.height > 0 )
      {
         [ theImage setSize:charRect.size ];
         [ theImage lockFocus ];
         [ [ NSGraphicsContext currentContext ] setShouldAntialias:NO ];
         [ blackColor set ];
         [ NSBezierPath fillRect:charRect ];
         [ currentChar drawInRect:charRect withAttributes:attribDict ];
         [ theImage unlockFocus ];
         if( ![ self makeDisplayList:dListNum withImage:theImage ] )
         {
            retval = FALSE;
            break;
         }
      }
   }

   glPopClientAttrib();

   return retval;
}


/*
 * Create one display list based on the given image.  This assumes the image
 * uses 8-bit chunks to represent a sample
 */
- (BOOL) makeDisplayList:(GLint)listNum withImage:(NSImage *)theImage
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
   newBuffer = calloc( ceil( (float) bytesPerRow / 8.0 ), pixelsHigh );
   if( newBuffer == NULL )
   {
      [ NSFont doOpenGLLog:@"Failed to calloc() memory in "
                           @"makeDisplayList:withImage:" ];
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
 * Log the warning/error, if logging is enabled
 */
+ (void) doOpenGLLog:(NSString *)format, ...
{
   va_list args;

   if( openGLLoggingEnabled )
   {
      va_start( args, format );
      NSLogv( [ NSString stringWithFormat:@"NSFont_OpenGL: %@\n", format ],
              args );
      va_end( args );
   }
}

@end
