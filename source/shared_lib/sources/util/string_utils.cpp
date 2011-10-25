/*  TA3D, a remake of Total Annihilation
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

//#include "../stdafx.h"
#include "string_utils.h"

#include <algorithm>

#include <assert.h>

namespace Shared { namespace Util {

    namespace
    {
        int stdLowerCase (int c)
        {
            return tolower(c);
        }

        int stdUpperCase (int c)
        {
            return toupper(c);
        }
    }

    String&
    String::operator << (const wchar_t* v)
    {
        size_t l = wcslen(v);
        char* b = new char[l + 1];
        #ifndef WINDOWS
        wcstombs(&b[0], v, l);
        #else
        size_t i;
        wcstombs_s(&i, &b[0], l, v, l);
        #endif
        append(b);
        delete [] b;
        return *this;
    }

    String&
    String::toLower()
    {
        std::transform (this->begin(), this->end(), this->begin(), stdLowerCase);
        return *this;
    }

    String&
    String::toUpper()
    {
        std::transform (this->begin(), this->end(), this->begin(), stdUpperCase);
        return *this;
    }


    bool String::toBool() const
    {
        if (empty() || "0" == *this)
            return false;
        if ("1" == *this)
            return true;
        String s(*this);
        s.toLower();
        return ("true" == s || "on" == s);
    }


    String&
    String::trim(const String& trimChars)
    {
        // Find the first character position after excluding leading blank spaces
        std::string::size_type startpos = this->find_first_not_of(trimChars);
        // Find the first character position from reverse af
        std::string::size_type endpos   = this->find_last_not_of(trimChars);

        // if all spaces or empty return an empty string
        if ((std::string::npos == startpos) || (std::string::npos == endpos))
           *this = "";
        else
            *this = this->substr(startpos, endpos - startpos + 1);
        return *this;
    }

    void
    String::split(String::Vector& out, const String& separators, const bool emptyBefore) const
    {
        // Empty the container
        if (emptyBefore)
            out.clear();
        // TODO This method should be rewritten for better performance
        String s(*this);
        while (!s.empty())
        {
            String::size_type i = s.find_first_of(separators);
            if (i == std::string::npos)
            {
                out.push_back(String::Trim(s));
                return;
            }
            else
            {
                out.push_back(String::Trim(s.substr(0, i)));
                s = s.substr(i + 1, s.size() - i - 1);
            }
        }
    }

    void
    String::split(String::List& out, const String& separators, const bool emptyBefore) const
    {
        // Empty the container
        if (emptyBefore)
            out.clear();
        // TODO This method should be rewritten for better performance
        String s(*this);
        while (!s.empty())
        {
            String::size_type i = s.find_first_of(separators);
            if (i == std::string::npos)
            {
                out.push_back(String::Trim(s));
                return;
            }
            else
            {
                out.push_back(String::Trim(s.substr(0, i)));
                s = s.substr(i + 1, s.size() - i - 1);
            }
        }
    }


    void String::ToKeyValue(const String& s, String& key, String& value, const enum String::CharCase chcase)
    {
        // The first usefull character
        String::size_type pos = s.find_first_not_of(TA3D_WSTR_SEPARATORS);
        if (pos == String::npos)
        {
            // The string is empty
            key.clear();
            value.clear();
            return;
        }
        // Begining of a section
        if (s[pos] == '[')
        {
            key = "[";
            pos = s.find_first_not_of(TA3D_WSTR_SEPARATORS, pos + 1);
            String::size_type end = s.find_first_of(']', pos);
            if (end != String::npos)
            {
                end = s.find_last_not_of(TA3D_WSTR_SEPARATORS, end - 1);
                if (pos != String::npos && end != String::npos)
                    value = s.substr(pos, end - pos + 1);
            }
            return;
        }
        // The first `=` character
        String::size_type equal = s.find_first_of('=', pos);
        if (equal == String::npos)
        {
            // otherwise it is only a string
            value.clear();
            // But it may be a comment
            String::size_type slashes = s.find("//", pos);
            if (pos == slashes)
            {
                key.clear();
                return;
            }
            String::size_type end = s.find_last_not_of(TA3D_WSTR_SEPARATORS, slashes - 1);
            key = s.substr(pos, end - pos + 1);
            return;
        }

        // We can extract our key
        String::size_type end = s.find_last_not_of(TA3D_WSTR_SEPARATORS, equal - 1);
        key = s.substr(pos, 1 + end - pos);
        String::size_type slashes = key.rfind("//");
        // Remove any comments
        if (slashes != String::npos)
        {
            value.clear();
            if (slashes == 0) // the key is a comment actually
                key.clear();
            else
            {
                // Get only the good part
                slashes = key.find_last_not_of(TA3D_WSTR_SEPARATORS, slashes - 1);
                key = key.substr(0, slashes + 1);
                if (chcase == soIgnoreCase)
                    key.toLower();
            }
            return;
        }
        if (chcase == soIgnoreCase)
            key.toLower();

        // Left-Trim for the value
        equal = s.find_first_not_of(TA3D_WSTR_SEPARATORS, equal + 1);
        if (String::npos == equal)
        {
            value.clear();
            return;
        }

        // Looking for the first semicolon
        bool needReplaceSemicolons(false);
        String::size_type semicolon = s.find_first_of(';', equal);
        while (semicolon != String::npos && s[semicolon - 1] == '\\')
        {
            semicolon = s.find_first_of(';', semicolon + 1);
            needReplaceSemicolons = true;
        }
        if (semicolon == String::npos)
        {
            // if none is present, looks for a comment to strip it
            slashes = s.find("//", equal);
            slashes = s.find_last_not_of(TA3D_WSTR_SEPARATORS, slashes - 1);
            value = s.substr(equal, 1 + slashes - equal);
            value.findAndReplace("\\r", "", soCaseSensitive);
            value.findAndReplace("\\n", "\n", soCaseSensitive);
            if (needReplaceSemicolons)
                value.findAndReplace("\\;", ";", soCaseSensitive);
            return;
        }
        // Remove spaces before the semicolon and after the `=`
        semicolon = s.find_last_not_of(TA3D_WSTR_SEPARATORS, semicolon - 1);

        // We can extract the value
        if (semicolon >= equal)
        {
            value = s.substr(equal, 1 + semicolon - equal);
            value.findAndReplace("\\r", "", soCaseSensitive);
            value.findAndReplace("\\n", "\n", soCaseSensitive);
            if (needReplaceSemicolons)
                value.findAndReplace("\\;", ";", soCaseSensitive);
        }
        else
            value.clear();
    }


    String& String::convertAntiSlashesIntoSlashes()
    {
        for (String::iterator i = this->begin(); i != this->end(); ++i)
        {
            if (*i == '\\')
                *i = '/';
        }
        return *this;
    }

    String& String::convertSlashesIntoAntiSlashes()
    {
        for (String::iterator i = this->begin(); i != this->end(); ++i)
        {
            if (*i == '/')
                *i = '\\';
        }
        return *this;
    }

    Shared::Platform::uint32 String::hashValue() const
    {
        Shared::Platform::uint32 hash = 0;
        for (String::const_iterator i = this->begin(); i != this->end(); ++i)
            hash = (hash << 5) - hash + *i;
        return hash;
    }

    int String::FindInList(const String::Vector& l, const char* s)
    {
        int indx(0);
        for (String::Vector::const_iterator i = l.begin(); i != l.end(); ++i, ++indx)
        {
            if(s == *i)
                return indx;
        }
        return -1;
    }


    int String::FindInList(const String::Vector& l, const String& s)
    {
        int indx(0);
        for (String::Vector::const_iterator i = l.begin(); i != l.end(); ++i, ++indx)
        {
            if(s == *i)
                return indx;
        }
        return -1;
    }


    char* String::ConvertFromUTF8(const char* str) {

/*
    	int length = strlen(str);
    	char *pBuffer = new char[length * 8];
    	memset(pBuffer,0,length * 8);

		int len = 0;
		for(unsigned int i = 0 ; i < length; i++)
		{
			if (((Shared::Platform::byte)str[i]) < 0x80)
			{
				pBuffer[len++] = ((Shared::Platform::byte)str[i]);
				continue;
			}
			if (((Shared::Platform::byte)str[i]) >= 0xC0)
			{
				wchar_t c = ((Shared::Platform::byte)str[i++]) - 0xC0;
				while(((Shared::Platform::byte)str[i]) >= 0x80)
					c = (c << 6) | (((Shared::Platform::byte)str[i++]) - 0x80);
				--i;
				pBuffer[len++] = c;
				continue;
			}
		}
		pBuffer[len] = 0;
		return pBuffer;
*/

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

    char* String::ConvertToUTF8(const char* s)
    {
        if (NULL != s && *s != '\0')
            return ConvertToUTF8(s, strlen(s));
        char* ret = new char[1];
        //LOG_ASSERT(NULL != ret);
        assert(NULL != ret);
        *ret = '\0';
        return ret;
    }

    char* String::ConvertToUTF8(const char* s, const Shared::Platform::uint32 len)
    {
        Shared::Platform::uint32 nws;
        return ConvertToUTF8(s, len, nws);
    }

#ifndef WIN32
    int String::ASCIItoUTF8(const Shared::Platform::byte c, Shared::Platform::byte *out) {
#else
	int String::ASCIItoUTF8(const byte c, byte *out) {
#endif
    	if (c < 0x80)
    	{
    		*out = c;
    		return 1;
    	}
    	else if(c < 0xC0)
    	{
    		out[0] = 0xC2;
    		out[1] = c;
    		return 2;
    	}
    	out[0] = 0xC3;
    	out[1] = c - 0x40;
    	return 2;
    }

    char* String::ConvertToUTF8(const char* s, Shared::Platform::uint32 len, Shared::Platform::uint32& newSize)
    {
        if (NULL == s || '\0' == *s)
        {
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


    String String::ConvertToUTF8(const String& s)
    {
        if (s.empty())
            return String();
        char* ret = ConvertToUTF8(s.c_str(), s.size());
        if (ret)
        {
            String s(ret); // TODO Find a way to not use a temporary string
            delete[] ret;
            return s;
        }
        return String();
    }


    String& String::findAndReplace(char toSearch, const char replaceWith, const enum String::CharCase option)
    {
        if (option == soIgnoreCase)
        {
            toSearch = tolower(toSearch);
            for (String::iterator i = this->begin(); i != this->end(); ++i)
            {
                if (tolower(*i) == toSearch)
                    *i = replaceWith;
            }
        }
        else
        {
            for (String::iterator i = this->begin(); i != this->end(); ++i)
            {
                if (*i == toSearch)
                    *i = replaceWith;
            }

        }
        return *this;
    }


    String& String::findAndReplace(const String& toSearch, const String& replaceWith, const enum String::CharCase option)
    {
        if (soCaseSensitive == option)
        {
            String::size_type p = 0;
            String::size_type siz = toSearch.size();
            while ((p = this->find(toSearch, p)) != String::npos)
                this->replace(p, siz, replaceWith);
        }
        else
        {
            *this = String::ToLower(*this).findAndReplace(String::ToLower(toSearch), replaceWith, soCaseSensitive);
        }
        return *this;
    }


    String& String::format(const String& f, ...)
    {
        va_list parg;
        va_start(parg, f);

        this->clear();
        vappendFormat(f.c_str(), parg);

        va_end(parg);
        return *this;
    }


    String& String::format(const char* f, ...)
    {
        va_list parg;
        va_start(parg, f);

        this->clear();
        vappendFormat(f, parg);

        va_end(parg);
        return *this;
    }


    String& String::appendFormat(const String& f, ...)
    {
        va_list parg;
        va_start(parg, f);

        vappendFormat(f.c_str(), parg);

        va_end(parg);
        return *this;
    }


    String& String::appendFormat(const char* f, ...)
    {
        va_list parg;
        va_start(parg, f);

        vappendFormat(f, parg);

        va_end(parg);
        return *this;
    }


    String& String::vappendFormat(const char* f, va_list parg)
    {
        char* b;
#if defined TA3D_PLATFORM_WINDOWS
        // Implement vasprintf() by hand with two calls to vsnprintf()
        // Remove this when Microsoft adds support for vasprintf()
#if defined TA3D_PLATFORM_MSVC
        int sizeneeded = _vsnprintf(NULL, 0, f, parg) + 1;
#else
        int sizeneeded = vsnprintf(NULL, 0, f, parg) + 1;
#endif
        if (sizeneeded < 0)
        {
            return *this;
        }
        b = new char[sizeneeded];
        if (b == NULL)
        {
            return *this;
        }
#if defined TA3D_PLATFORM_MSVC
        if (_vsnprintf(b, sizeneeded, f, parg) < 0)
#else
        if (vsnprintf(b, sizeneeded, f, parg) < 0)
#endif
        {
            delete[] b;
            return *this;
        }
#else
        if (vasprintf(&b, f, parg) < 0)
        {
            return *this;
        }
#endif
        this->append(b);
        delete[] b;
        return *this;
    }


    String String::Format(const String& f, ...)
    {
        va_list parg;
        va_start(parg, f);

        String s;
        s.vappendFormat(f.c_str(), parg);

        va_end(parg);
        return s;
    }

    String String::Format(const char* f, ...)
    {
        va_list parg;
        va_start(parg, f);

        String s;
        s.vappendFormat(f, parg);

        va_end(parg);
        return s;
    }

    bool String::match(const String &pattern)
    {
        if (pattern.empty())
            return empty();

        int e = 0;
        int prev = -1;
        for(unsigned int i = 0 ; i < size() ; i++)
            if (pattern[e] == '*')
            {
                if (e + 1 == pattern.size())
                    return true;
                while(pattern[e+1] == '*')  e++;
                if (e + 1 == pattern.size())
                    return true;

                prev = e;
                if (pattern[e+1] == (*this)[i])
                    e+=2;
            }
            else if(pattern[e] == (*this)[i])
                e++;
            else if(prev >= 0)
                e = prev;
            else
                return false;
        return e == pattern.size();
    }

    String String::substrUTF8(int pos, int len) const
    {
        if (len < 0)
            len = sizeUTF8() - len + 1 - pos;
        String res;
        int utf8_pos = 0;
        for(; pos > 0 ; pos--)
#ifndef WIN32
            if (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0xC0)
#else
			if (((byte)(*this)[utf8_pos]) >= 0xC0)
#endif
            {
                utf8_pos++;
#ifndef WIN32
                while (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80 && ((Shared::Platform::byte)(*this)[utf8_pos]) < 0xC0)
#else
				while (((byte)(*this)[utf8_pos]) >= 0x80 && ((byte)(*this)[utf8_pos]) < 0xC0)
#endif
                    utf8_pos++;
            }
            else
                utf8_pos++;

        for(; len > 0 ; len--)
        {
#ifndef WIN32
            if (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80)
#else
			if (((byte)(*this)[utf8_pos]) >= 0x80)
#endif
            {
                res << (char)(*this)[utf8_pos];
                utf8_pos++;
#ifndef WIN32
                while (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80 && ((Shared::Platform::byte)(*this)[utf8_pos]) < 0xC0)
#else
				while (((byte)(*this)[utf8_pos]) >= 0x80 && ((byte)(*this)[utf8_pos]) < 0xC0)
#endif
                {
                    res << (char)(*this)[utf8_pos];
                    utf8_pos++;
                }
            }
            else
            {
                res << ((char)(*this)[utf8_pos]);
                utf8_pos++;
            }
        }
        return res;
    }

    int String::sizeUTF8() const
    {
        int len = 0;
        for(unsigned int i = 0 ; i < this->size() ; i++)
#ifndef WIN32
            if (((Shared::Platform::byte)(*this)[i]) >= 0xC0 || ((Shared::Platform::byte)(*this)[i]) < 0x80)
#else
			if (((byte)(*this)[i]) >= 0xC0 || ((byte)(*this)[i]) < 0x80)
#endif
                len++;
        return len;
    }

	WString::WString(const char* s)
	{
		if (s)
			fromUtf8(s, strlen(s));
		else
			pBuffer[0] = 0;
	}

	WString::WString(const String& s)
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
