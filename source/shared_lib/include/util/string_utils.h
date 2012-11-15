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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

#ifndef _SHARED_UTIL_W_STRING_H__
#define _SHARED_UTIL_W_STRING_H__

#include <string>
#include "data_types.h"

#ifdef __MINGW32__
typedef unsigned char byte;
#endif


#ifndef WIN32
using Shared::Platform::byte;
#endif

using namespace Shared::Platform;

namespace Shared { namespace Util {

	/*!
        ** \brief Convert a string from ASCII to UTF8
        ** \param s The string to convert
        ** \return A new Null-terminated String (must be deleted with the keyword `delete[]`), even if s is NULL
    */
    char* ConvertToUTF8(const char* s);

    char* ConvertFromUTF8(const char* str);

	/*!
	** \brief Convert an UTF-8 String into a WideChar String
	*/
	struct WString
	{
	public:
		WString(const char* s);
		WString(const std::string& str);

		void fromUtf8(const char* s, size_t length);
		const wchar_t* cw_str() const {return pBuffer;}

	private:
		wchar_t pBuffer[8096];
	};

	void strrev(char *p);
	void strrev_utf8(char *p);
	void strrev_utf8(std::string &p);
	bool is_string_all_ascii(std::string str);

}}

#endif // _SHARED_UTIL_W_STRING_H__
