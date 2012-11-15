/*  Portions taken from:
    Copyright (C) 2005  Roland BROCHARD

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "string_utils.h"

#include <algorithm>
#include <assert.h>

namespace Shared { namespace Util {

#ifndef WIN32
    int ASCIItoUTF8(const Shared::Platform::byte c, Shared::Platform::byte *out) {
#else
	int ASCIItoUTF8(const byte c, byte *out) {
#endif
    	if (c < 0x80) {
    		*out = c;
    		return 1;
    	}
    	else if(c < 0xC0) {
    		out[0] = 0xC2;
    		out[1] = c;
    		return 2;
    	}
    	out[0] = 0xC3;
    	out[1] = c - 0x40;
    	return 2;
    }

    char* ConvertToUTF8(const char* s, Shared::Platform::uint32 len, Shared::Platform::uint32& newSize) {
        if (NULL == s || '\0' == *s) {
            char* ret = new char[1];
            //LOG_ASSERT(NULL != ret);
            assert(NULL != ret);
            *ret = '\0';
            return ret;
        }
#ifndef WIN32
        Shared::Platform::byte tmp[4];
#else
		byte tmp[4];
#endif
        newSize = 1;

#ifndef WIN32
        for(Shared::Platform::byte *p = (Shared::Platform::byte*)s ; *p ; p++)
#else
		for(byte *p = (byte*)s ; *p ; p++)
#endif
            newSize += ASCIItoUTF8(*p, tmp);

        char* ret = new char[newSize];
        //LOG_ASSERT(NULL != ret);
        assert(NULL != ret);

#ifndef WIN32
        Shared::Platform::byte *q = (Shared::Platform::byte*)ret;
        for(Shared::Platform::byte *p = (Shared::Platform::byte*)s ; *p ; p++)
#else
        byte *q = (byte*)ret;
        for(byte *p = (byte*)s ; *p ; p++)

#endif
            q += ASCIItoUTF8(*p, q);
        *q = '\0'; // A bit paranoid
        return ret;
    }

    char* ConvertToUTF8(const char* s, const Shared::Platform::uint32 len) {
            Shared::Platform::uint32 nws;
            return ConvertToUTF8(s, len, nws);
    }

    char* ConvertToUTF8(const char* s) {
		if (NULL != s && *s != '\0')
			return ConvertToUTF8(s, strlen(s));
		char* ret = new char[1];
		//LOG_ASSERT(NULL != ret);
		assert(NULL != ret);
		*ret = '\0';
		return ret;
	}


    char* ConvertFromUTF8(const char* str) {
        const unsigned char *in = reinterpret_cast<const unsigned char*>(str);
        int len = strlen(str);
        char *out = new char[len*8];
        memset(out,0,len*8);
		int outc;
		int inpos = 0;
		int outpos = 0;
		while (inpos < len || len == -1) {
			  if (in[inpos]<0x80) {
			   out[outpos++] = in[inpos];
			   if (in[inpos] == 0 && len == -1)
				break;
			   inpos++;
			 }
			 else if (in[inpos]<0xE0) {
				 // Shouldn't happen.
				 if(in[inpos]<0xC0 || (len!=-1 && inpos+1 >= len) ||
				  (in[inpos+1]&0xC0)!= 0x80) {
					 out[outpos++] = '?';
					 inpos++;
					 continue;
				 }
				 outc =	((((wchar_t)in[inpos])&0x1F)<<6) |
						 (((wchar_t)in[inpos+1])&0x3F);
				 if (outc < 256)
					 out[outpos] = ((char*)&outc)[0];
				 else
					 out[outpos] = '?';
				 outpos++;
				 inpos+=2;
			 }
			 else if (in[inpos]<0xF0) {
				 // Shouldn't happen.
				 if ((len!=-1 && inpos+2 >= len) ||
					   (in[inpos+1]&0xC0)!= 0x80 ||
					   (in[inpos+2]&0xC0)!= 0x80) {
				   out[outpos++] = '?';
				   inpos++;
				   continue;
				 }
				 out[outpos++] = '?';
				 inpos+=3;
			 }
			 else if (in[inpos]<0xF8) {
				 // Shouldn't happen.
				 if ((len!=-1 && inpos+3 >= len) ||
						 (in[inpos+1]&0xC0)!= 0x80 ||
						 (in[inpos+2]&0xC0)!= 0x80 ||
						 (in[inpos+3]&0xC0)!= 0x80) {
					 out[outpos++] = '?';
					 inpos++;
					 continue;
				 }
				 out[outpos++] = '?';
				 inpos+=4;
			 }
			 else {
			   out[outpos++] = '?';
			   inpos++;
			 }
		}
		return out;
    }

	WString::WString(const char* s)
	{
		if (s)
			fromUtf8(s, strlen(s));
		else
			pBuffer[0] = 0;
	}

	WString::WString(const std::string& s)
	{
		fromUtf8(s.c_str(), s.size());
	}

	void WString::fromUtf8(const char* str, size_t length)
	{
		int len = 0;
		for(unsigned int i = 0 ; i < length; i++)
		{
#ifndef WIN32
			if (((Shared::Platform::byte)str[i]) < 0x80)
#else
			if (((byte)str[i]) < 0x80)
#endif
			{
#ifndef WIN32
				pBuffer[len++] = ((Shared::Platform::byte)str[i]);
#else
				pBuffer[len++] = ((byte)str[i]);
#endif
				continue;
			}
#ifndef WIN32
			if (((Shared::Platform::byte)str[i]) >= 0xC0)
			{
				wchar_t c = ((Shared::Platform::byte)str[i++]) - 0xC0;
				while(((Shared::Platform::byte)str[i]) >= 0x80)
					c = (c << 6) | (((Shared::Platform::byte)str[i++]) - 0x80);
#else
			if (((byte)str[i]) >= 0xC0)
			{
				wchar_t c = ((byte)str[i++]) - 0xC0;
				while(((byte)str[i]) >= 0x80)
					c = (c << 6) | (((byte)str[i++]) - 0x80);

#endif

				--i;
				pBuffer[len++] = c;
				continue;
			}
		}
		pBuffer[len] = 0;
	}


	void strrev(char *p) {
	  char *q = p;
	  while(q && *q) ++q;
	  for(--q; p < q; ++p, --q)
	    *p = *p ^ *q,
	    *q = *p ^ *q,
	    *p = *p ^ *q;
	}

	#define SWP(x,y) (x^=y, y^=x, x^=y)

	void strrev_utf8(char *p) {
	  //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);

	  char *q = p;
	  strrev(p); /* call base case */

	  //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);

	  /* Ok, now fix bass-ackwards UTF chars. */
	  while(q && *q) ++q; /* find eos */

	  //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	  while(p < --q) {
		  //printf("In [%s::%s] Line: %d p [%s] q [%s]\n",__FILE__,__FUNCTION__,__LINE__,p,q); fflush(stdout);

	    switch( (*q & 0xF0) >> 4 ) {
	    case 0xF: /* U+010000-U+10FFFF: four bytes. */
	    	//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      SWP(*(q-0), *(q-3));
	      //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      SWP(*(q-1), *(q-2));
	      //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      q -= 3;
	      break;
	    case 0xE: /* U+000800-U+00FFFF: three bytes. */
	    	//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      SWP(*(q-0), *(q-2));
	      //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      q -= 2;
	      break;
	    case 0xC: /* fall-through */
	    case 0xD: /* U+000080-U+0007FF: two bytes. */
	    	//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      SWP(*(q-0), *(q-1));
	      //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	      q--;
	      break;
	    }

	    //printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	  }
	  //printf("In [%s::%s] Line: %d p [%s]\n",__FILE__,__FUNCTION__,__LINE__,p); fflush(stdout);
	}

	void strrev_utf8(std::string &p) {
		//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);

		if(p.length() > 0) {
			//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);

			int bufSize = p.length()*4;
			char *szBuf = new char[bufSize];
			memset(szBuf,0,bufSize);
			strcpy(szBuf,p.c_str());
			//szBuf[bufSize] = '\0';
			strrev_utf8(szBuf);
			//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
			p = std::string(szBuf);

			//printf("In [%s::%s] Line: %d bufSize = %d p.size() = %d p [%s]\n",__FILE__,__FUNCTION__,__LINE__,bufSize,p.length(),p.c_str()); fflush(stdout);

			delete [] szBuf;

			//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
		}
		//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__); fflush(stdout);
	}

	bool is_string_all_ascii(std::string str) {
		bool result = true;
		for(unsigned int i = 0; i < str.length(); ++i) {
			if(isascii(str[i]) == false) {
				result = false;
				break;
			}
		}
		return result;
	}



}}
