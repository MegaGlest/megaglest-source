// ==============================================================
//	This file is part of MegaGlest (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "network_protocol.h"
#include <stdarg.h>
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include "data_types.h"
#include "util.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

#pragma pack(push, 1)

// macros for packing floats and doubles:
#define pack754_16(f) (pack754((f), 16, 5))
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_16(i) (unpack754((i), 16, 5))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

/*
** pack754() -- pack a floating point number into IEEE-754 format
*/
unsigned long long int pack754(long double f, unsigned bits, unsigned expbits)
{
	long double fnorm;
	int shift;
	long long sign, exp, significand;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	if (f == 0.0) return 0; // get this special case out of the way

	// check sign and begin normalization
	if (f < 0) { sign = 1; fnorm = -f; }
	else { sign = 0; fnorm = f; }

	// get the normalized form of f and track the exponent
	shift = 0;
	while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL<<significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

	// return the final answer
	return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

/*
** unpack754() -- unpack a floating point number from IEEE-754 format
*/
long double unpack754(unsigned long long int i, unsigned bits, unsigned expbits)
{
	long double result;
	long long shift;
	unsigned bias;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	if (i == 0) return 0.0;

	// pull the significand
	result = (i&((1LL<<significandbits)-1)); // mask
	result /= (1LL<<significandbits); // convert back to float
	result += 1.0f; // add the one back on

	// deal with the exponent
	bias = (1<<(expbits-1)) - 1;
	shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
	while(shift > 0) { result *= 2.0; shift--; }
	while(shift < 0) { result /= 2.0; shift++; }

	// sign it
	result *= (i>>(bits-1))&1? -1.0: 1.0;

	return result;
}

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/
void packi16(unsigned char *buf, uint16 i)
{
	*buf++ = i>>8; *buf++ = i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())
*/
void packi32(unsigned char *buf, uint32 i)
{
	*buf++ = i>>24; *buf++ = i>>16;
	*buf++ = i>>8;  *buf++ = i;
}

/*
** packi64() -- store a 64-bit int into a char buffer (like htonl())
*/
void packi64(unsigned char *buf, uint64 i)
{
	*buf++ = i>>56; *buf++ = i>>48;
	*buf++ = i>>40; *buf++ = i>>32;
	*buf++ = i>>24; *buf++ = i>>16;
	*buf++ = i>>8;  *buf++ = i;
}

/*
** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
*/
int16 unpacki16(unsigned char *buf)
{
	uint16 i2 = ((uint16)buf[0]<<8) | buf[1];
	int16 i;

	// change unsigned numbers to signed
	if (i2 <= 0x7fffu) {
		i = i2;
	}
	else {
		i = -1 - (uint16)(0xffffu - i2);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("IN [%s] [%d] [%d] [%d] [%u]\n",__FUNCTION__,buf[0],buf[1],i,i2);
	}

	return i;
}

/*
** unpacku16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
*/
uint16 unpacku16(unsigned char *buf)
{
	return ((uint16)buf[0]<<8) | buf[1];
}

/*
** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
*/
int32 unpacki32(unsigned char *buf)
{
	uint32 i2 = ((uint32)buf[0]<<24) |
	            ((uint32)buf[1]<<16) |
	            ((uint32)buf[2]<<8)  |
	                     buf[3];
	int32 i;

	// change unsigned numbers to signed
	if (i2 <= 0x7fffffffu) {
		i = i2;
	}
	else {
		i = -1 - (int32)(0xffffffffu - i2);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("IN [%s] [%d] [%d] [%d] [%d] [%d] [%u]\n",__FUNCTION__,buf[0],buf[1],buf[2],buf[3],i,i2);
	}

	return i;
}

/*
** unpacku32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
*/
uint32 unpacku32(unsigned char *buf)
{
	return ((uint32)buf[0]<<24) |
	       ((uint32)buf[1]<<16) |
	       ((uint32)buf[2]<<8)  |
	       buf[3];
}

/*
** unpacki64() -- unpack a 64-bit int from a char buffer (like ntohl())
*/
int64 unpacki64(unsigned char *buf)
{
	uint64 i2 = ((uint64)buf[0]<<56) |
	              ((uint64)buf[1]<<48) |
	              ((uint64)buf[2]<<40) |
	              ((uint64)buf[3]<<32) |
	              ((uint64)buf[4]<<24) |
	              ((uint64)buf[5]<<16) |
	              ((uint64)buf[6]<<8)  |
	                       buf[7];
	int64 i;

	// change unsigned numbers to signed
	if (i2 <= 0x7fffffffffffffffu) {
		i = i2;
	}
	else {
		i = -1 -(int64)(0xffffffffffffffffu - i2);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("IN [%s] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%ld] [%lu]\n",__FUNCTION__,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],i,i2);
	}

	return i;
}

/*
** unpacku64() -- unpack a 64-bit unsigned from a char buffer (like ntohl())
*/
uint64 unpacku64(unsigned char *buf)
{
	return ((uint64)buf[0]<<56) |
	       ((uint64)buf[1]<<48) |
	       ((uint64)buf[2]<<40) |
	       ((uint64)buf[3]<<32) |
	       ((uint64)buf[4]<<24) |
	       ((uint64)buf[5]<<16) |
	       ((uint64)buf[6]<<8)  |
	       buf[7];
}

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (16-bit unsigned length is automatically prepended to strings)
*/

unsigned int pack(unsigned char *buf, const char *format, ...) {
	va_list ap;

	int8 c;              // 8-bit
	uint8 C;

	int16 h;                      // 16-bit
	uint16 H;

	int32 l;                 // 32-bit
	uint32 L;

	int64 q;            // 64-bit
	uint64 Q;

	float f;                    // floats
	double d;
	long double g;
	unsigned long long int fhold;

	char *s;                    // strings
	uint16 len;

	unsigned int size = 0;

	uint16 maxstrlen=0, count;

	unsigned char *bufStart = buf;

	va_start(ap, format);

	for(; *format != '\0'; format++) {
		switch(*format) {
		case 'c': // 8-bit
			size += 1;
			c = (int8)va_arg(ap, int); // promoted
			*buf++ = (unsigned char)c;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack int8 = %d [%X] c = %d [%X] buf pos = %ld\n",*(buf-1),*(buf-1),c,c,(buf - bufStart));
			break;

		case 'C': // 8-bit unsigned
			size += 1;
			C = (uint8)va_arg(ap, unsigned int); // promoted
			*buf++ = (unsigned char)C;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack uint8 = %u [%X] C = %u [%X] buf pos = %ld\n",*(buf-1),*(buf-1),C,C,(buf - bufStart));
			break;

		case 'h': // 16-bit
			size += 2;
			h = (int16)va_arg(ap, int);
			packi16(buf, h);
			buf += 2;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack int16 = %d [%X] h = %d [%X] buf pos = %ld\n",*(buf-2),*(buf-2),h,h,(buf - bufStart));
			break;

		case 'H': // 16-bit unsigned
			size += 2;
			H = (uint16)va_arg(ap, unsigned int);
			packi16(buf, H);
			buf += 2;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack uint16 = %u [%X] H = %u [%X] buf pos = %ld\n",*(buf-2),*(buf-2),H,H,(buf - bufStart));
			break;

		case 'l': // 32-bit
			size += 4;
			l = va_arg(ap, int32);
			packi32(buf, l);
			buf += 4;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack int32 = %d [%X] l = %d [%X] buf pos = %ld\n",*(buf-4),*(buf-4),l,l,(buf - bufStart));
			break;

		case 'L': // 32-bit unsigned
			size += 4;
			L = va_arg(ap, uint32);
			packi32(buf, L);
			buf += 4;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack uint32 = %u [%X] L = %u [%X] buf pos = %ld\n",*(buf-4),*(buf-4),L,L,(buf - bufStart));
			break;

		case 'q': // 64-bit
			size += 8;
			q = va_arg(ap, int64);
			packi64(buf, q);
			buf += 8;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack int64 = %ld [%X] q = %ld [%lX] buf pos = %ld\n",(int64)*(buf-8),*(buf-8),q,q,(buf - bufStart));
			break;

		case 'Q': // 64-bit unsigned
			size += 8;
			Q = va_arg(ap, uint64);
			packi64(buf, Q);
			buf += 8;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack uint64 = %lu [%X] Q = %lu buf pos = %ld\n",(uint64)*(buf-8),*(buf-8),Q,(buf - bufStart));
			break;

		case 'f': // float-16
			size += 2;
			f = (float)va_arg(ap, double); // promoted
			fhold = pack754_16(f); // convert to IEEE 754
			packi16(buf, fhold);
			buf += 2;
			break;

		case 'd': // float-32
			size += 4;
			d = va_arg(ap, double);
			fhold = pack754_32(d); // convert to IEEE 754
			packi32(buf, fhold);
			buf += 4;
			break;

		case 'g': // float-64
			size += 8;
			g = va_arg(ap, long double);
			fhold = pack754_64(g); // convert to IEEE 754
			packi64(buf, fhold);
			buf += 8;
			break;

		case 's': // string
			s = va_arg(ap, char*);
			len = strlen(s);
			if (maxstrlen > 0 && len < maxstrlen)
				len = maxstrlen - 1;

			size += len + 2;
			packi16(buf, len);
			buf += 2;

			memcpy(buf, s, len);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("pack string size = %d [%X] len = %d str [%s] buf pos = %ld\n",*(buf-2),*(buf-2),len,s,(buf - bufStart));

			buf += len;
			break;

		default:
			if (isdigit(*format)) { // track max str len
				maxstrlen = maxstrlen * 10 + (*format-'0');
			}
		}

		if (!isdigit(*format)) maxstrlen = 0;

	}

	va_end(ap);

	return size;
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
unsigned int unpack(unsigned char *buf, const char *format, ...) {
	va_list ap;

	int8 *c;              // 8-bit
	uint8 *C;

	int16 *h;                      // 16-bit
	uint16 *H;

	int32 *l;                 // 32-bit
	uint32 *L;

	int64 *q;            // 64-bit
	uint64 *Q;

	float *f;                    // floats
	double *d;
	long double *g;
	unsigned long long int fhold;

	char *s;
	uint16 len, maxstrlen=0, count;

	unsigned int size = 0;

	unsigned char *bufStart = buf;

	va_start(ap, format);

	for(; *format != '\0'; format++) {
		switch(*format) {
		case 'c': // 8-bit
			c = va_arg(ap, int8*);
//			if (*buf <= 0x7f) {
//				*c = *buf++;
//				size += 1;
//			} // re-sign
//			else {
//				*c = -1 - (unsigned char)(0xffu - *buf);
//			}
			*c = (int8)*buf++;
			size += 1;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack int8 = %d [%X] c = %d [%X] buf pos = %ld\n",*(buf-1),*(buf-1),*c,*c,(buf - bufStart));
			break;

		case 'C': // 8-bit unsigned
			C = va_arg(ap, uint8*);
			*C = (uint8)*buf++;
			size += 1;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack uint8 = %u [%X] C = %u [%X] buf pos = %ld\n",*(buf-1),*(buf-1),*C,*C,(buf - bufStart));
			break;

		case 'h': // 16-bit
			h = va_arg(ap, int16*);
			*h = unpacki16(buf);
			buf += 2;
			size += 2;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack int16 = %d [%X] h = %d [%X] buf pos = %ld\n",*(buf-2),*(buf-2),*h,*h,(buf - bufStart));
			break;

		case 'H': // 16-bit unsigned
			H = va_arg(ap, uint16*);
			*H = unpacku16(buf);
			buf += 2;
			size += 2;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack uint16 = %u [%X] H = %u [%X] buf pos = %ld\n",*(buf-2),*(buf-2),*H,*H,(buf - bufStart));
			break;

		case 'l': // 32-bit
			l = va_arg(ap, int32*);
			*l = unpacki32(buf);
			buf += 4;
			size += 4;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack int32 = %d [%X] l = %d [%X] buf pos = %ld\n",*(buf-4),*(buf-4),*l,*l,(buf - bufStart));
			break;

		case 'L': // 32-bit unsigned
			L = va_arg(ap, uint32*);
			*L = unpacku32(buf);
			buf += 4;
			size += 4;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack uint32 = %u [%X] L = %u [%X] buf pos = %ld\n",*(buf-4),*(buf-4),*L,*L,(buf - bufStart));
			break;

		case 'q': // 64-bit
			q = va_arg(ap, int64*);
			*q = unpacki64(buf);
			buf += 8;
			size += 8;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack int64 = %ld [%X] q = %ld buf pos = %ld\n",(int64)*(buf-8),*(buf-8),*q,(buf - bufStart));
			break;

		case 'Q': // 64-bit unsigned
			Q = va_arg(ap, uint64*);
			*Q = unpacku64(buf);
			buf += 8;
			size += 8;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack uint64 = %lu [%X] Q = %lu buf pos = %ld\n",(uint64)*(buf-8),*(buf-8),*Q,(buf - bufStart));
			break;

		case 'f': // float
			f = va_arg(ap, float*);
			fhold = unpacku16(buf);
			*f = unpack754_16(fhold);
			buf += 2;
			size += 2;
			break;

		case 'd': // float-32
			d = va_arg(ap, double*);
			fhold = unpacku32(buf);
			*d = unpack754_32(fhold);
			buf += 4;
			size += 4;
			break;

		case 'g': // float-64
			g = va_arg(ap, long double*);
			fhold = unpacku64(buf);
			*g = unpack754_64(fhold);
			buf += 8;
			size += 8;
			break;

		case 's': // string
			s = va_arg(ap, char*);
			len = unpacku16(buf);
			buf += 2;
			if (maxstrlen > 0 && len > maxstrlen)
				count = maxstrlen - 1;
			else
				count = len;

			memcpy(s, buf, count);
			s[count] = '\0';
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("unpack string size = %d [%X] count = %d len = %d str [%s] buf pos = %ld\n",*(buf-2),*(buf-2),count,len,s,(buf - bufStart));

			buf += len;
			size += len;
			break;

		default:
			if (isdigit(*format)) { // track max str len
				maxstrlen = maxstrlen * 10 + (*format-'0');
			}
		}

		if (!isdigit(*format)) maxstrlen = 0;
	}

	va_end(ap);

	return size;
}

#pragma pack(pop)

}}
