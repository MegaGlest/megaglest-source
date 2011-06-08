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

#if TA3D_USE_BOOST == 1
#  include <boost/algorithm/string.hpp>
#  include <boost/algorithm/string/trim.hpp>
#  include <boost/algorithm/string/split.hpp>
#else
#  include <algorithm>
#endif

#include <assert.h>
//#include "../logs/logs.h"


namespace Shared { namespace Util {

    #if TA3D_USE_BOOST != 1
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
    #endif

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
        delete b;
        return *this;
    }

    String&
    String::toLower()
    {
        #if TA3D_USE_BOOST == 1
        boost::to_lower(*this);
        #else
        std::transform (this->begin(), this->end(), this->begin(), stdLowerCase);
        #endif
        return *this;
    }

    String&
    String::toUpper()
    {
        #if TA3D_USE_BOOST == 1
        boost::to_upper(*this);
        #else
        std::transform (this->begin(), this->end(), this->begin(), stdUpperCase);
        #endif
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
        #if TA3D_USE_BOOST == 1
        // TODO : Avoid string duplication
        // Split
        std::vector<std::string> v;
        boost::algorithm::split(v, *this, boost::is_any_of(separators.c_str()));
        // Copying
        for(std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
            out.push_back(*i);
        #else
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
        #endif
    }

    void
    String::split(String::List& out, const String& separators, const bool emptyBefore) const
    {
        // Empty the container
        if (emptyBefore)
            out.clear();
        #if TA3D_USE_BOOST == 1
        // TODO : Avoid string duplication
        // Split
        std::vector<std::string> v;
        boost::algorithm::split(v, *this, boost::is_any_of(separators.c_str()));
        // Copying
        for(std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
            out.push_back(*i);
        #else
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
        #endif
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

    uint32 String::hashValue() const
    {
        uint32 hash = 0;
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

    char* String::ConvertToUTF8(const char* s, const uint32 len)
    {
        uint32 nws;
        return ConvertToUTF8(s, len, nws);
    }

    int String::ASCIItoUTF8(const Shared::Platform::byte c, Shared::Platform::byte *out) {
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

    char* String::ConvertToUTF8(const char* s, uint32 len, uint32& newSize)
    {
        if (NULL == s || '\0' == *s)
        {
            char* ret = new char[1];
            //LOG_ASSERT(NULL != ret);
            assert(NULL != ret);
            *ret = '\0';
            return ret;
        }
        Shared::Platform::byte tmp[4];
        newSize = 1;
        for(Shared::Platform::byte *p = (Shared::Platform::byte*)s ; *p ; p++)
            newSize += ASCIItoUTF8(*p, tmp);

        char* ret = new char[newSize];
        //LOG_ASSERT(NULL != ret);
        assert(NULL != ret);

        Shared::Platform::byte *q = (Shared::Platform::byte*)ret;
        for(Shared::Platform::byte *p = (Shared::Platform::byte*)s ; *p ; p++)
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
            if (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0xC0)
            {
                utf8_pos++;
                while (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80 && ((Shared::Platform::byte)(*this)[utf8_pos]) < 0xC0)
                    utf8_pos++;
            }
            else
                utf8_pos++;

        for(; len > 0 ; len--)
        {
            if (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80)
            {
                res << (char)(*this)[utf8_pos];
                utf8_pos++;
                while (((Shared::Platform::byte)(*this)[utf8_pos]) >= 0x80 && ((Shared::Platform::byte)(*this)[utf8_pos]) < 0xC0)
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
            if (((Shared::Platform::byte)(*this)[i]) >= 0xC0 || ((Shared::Platform::byte)(*this)[i]) < 0x80)
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
	}

}}
