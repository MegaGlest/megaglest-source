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

#ifndef _SHARED_UTIL_W_STRING_H__
#define _SHARED_UTIL_W_STRING_H__

# include <sstream>
# include <list>
# include <vector>
# include <cstdarg>
# include <string>
#include "types.h"

//! \name Macros for Shared::Util::String
//@{

//! Macro to append some data to the string
# define TA3D_WSTR_APPEND      std::stringstream out; \
                                out << v; \
                                append(out.str());\
                                return *this

//! Macro to convert the string into a given type
# define TA3D_WSTR_CAST_OP(X)  X v; \
                                fromString<X>(v, *this); \
                                return v;

//! Macro to append the value of a boolean (true -> "true", false -> "false")
# define TA3D_WSTR_APPEND_BOOL(X)   append(X ? "true" : "false")

//@} // Macros for Shared::Util::String


# define TA3D_WSTR_SEPARATORS  " \t\r\n"

#ifndef WIN32
using Shared::Platform::byte;
#endif

using namespace Shared::Platform;

namespace Shared { namespace Util {

    /*! \class String
    ** \brief A String implementation for `TA3D`
    **
    ** \code
    **      Shared::Util::String a("abcd");
    **      std::cout << a << std::endl;  // display: `abcd`
    **      Shared::Util::String b(10 + 2);
    **      std::cout << b << std::endl;  // display: `12`
    **      Shared::Util::String c(10.3);
    **      std::cout << c << std::endl;  // display: `10.3`
    **
    **      // The same with the operator `<<`
    **      Shared::Util::String d;
    **      d << "Value : " << 42;
    **      std::cout << d << std::endl;  // display: `Value : 42`
    ** \endcode
    **
    ** Here is an example to show when to use static methods :
    ** \code
    **      Shared::Util::String s = "HelLo wOrLd";
    **      std::cout << Shared::Util::String::ToLower(s) << std::endl;  // `hello world`
    **      std::cout << s << std::endl;  // `HelLo wOrLd`
    **      std::cout << s.toLower() << std::endl;  // `hello world`
    **      std::cout << s << std::endl;  // `hello world`
    ** \endcode
    */
    class String : public std::string
    {
    public:
        //! A String list
        typedef std::list<String> List;
        //! A String vector
        typedef std::vector<String> Vector;

        //! Char Case
        enum CharCase
        {
            //! The string should remain untouched
            soCaseSensitive,
            //! The string should be converted to lowercase
            soIgnoreCase
        };

    public:
        /*!
        ** \brief Copy then Convert the case (lower case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToLower(const char* s) {return String(s).toLower();}
        /*!
        ** \brief Copy then Convert the case (lower case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToLower(const wchar_t* s) {return String(s).toLower();}
        /*!
        ** \brief Copy then Convert the case (lower case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToLower(const String& s) {return String(s).toLower();}

        /*!
        ** \brief Copy then Convert the case (upper case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToUpper(const char* s) {return String(s).toUpper();}
        /*!
        ** \brief Copy then Convert the case (upper case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToUpper(const wchar_t* s) {return String(s).toUpper();}
        /*!
        ** \brief Copy then Convert the case (upper case) of characters in the string using the UTF8 charset
        ** \param s The string to convert
        ** \return A new string
        */
        static String ToUpper(const String& s) {return String(s).toUpper();}

        /*!
        ** \brief Remove trailing and leading spaces
        ** \param s The string to convert
        ** \param trimChars The chars to remove
        ** \return A new string
        */
        static String Trim(const char* s, const String& trimChars = TA3D_WSTR_SEPARATORS) {return String(s).trim(trimChars);}
        /*!
        ** \brief Remove trailing and leading spaces
        ** \param s The string to convert
        ** \param trimChars The chars to remove
        ** \return A new string
        */
        static String Trim(const wchar_t* s, const String& trimChars = TA3D_WSTR_SEPARATORS) {return String(s).trim(trimChars);}
        /*!
        ** \brief Remove trailing and leading spaces
        ** \param s The string to convert
        ** \param trimChars The chars to remove
        ** \return A new string
        */
        static String Trim(const String& s, const String& trimChars = TA3D_WSTR_SEPARATORS) {return String(s).trim(trimChars);}

        /*!
        ** \brief Convert all antislashes into slashes
        ** \param s The string to convert
        ** \return A new string
        */
        static String ConvertAntiSlashesIntoSlashes(const String& s) {return String(s).convertAntiSlashesIntoSlashes();}

        /*!
        ** \brief Convert all slashes into antislashes
        ** \param s The string to convert
        ** \return A new string
        */
        static String ConvertSlashesIntoAntiSlashes(const String& s) {return String(s).convertSlashesIntoAntiSlashes();}

        /*!
        ** \brief Extract the key and its value from a string (mainly provided by TDF files)
        **
        ** \param s A line (ex: `   category=core vtol ctrl_v level1 weapon  notsub ;`)
        ** \param[out] key The key that has been found
        ** \param[out] value The associated value
        ** \param chcase The key will be converted to lowercase if equals to `soIgnoreCase`
        **
        ** \code
        **    String k, v;
        **
        **    // -> k='category'
        **    // -> v='core vtol ctrl_v level1 weapon  notsub'
        **    String::ToKeyValue("  category=core vtol ctrl_v level1 weapon  notsub ;", k, v)
        **
        **    // -> k='foo'
        **    // -> v='bar'
        **    String::ToKeyValue("  foo  = bar ; ");
        **
        **    // -> k='}'  v=''
        **    String::ToKeyValue("  } ", k, v);
        **
        **    // -> k='['   v='Example of Section'
        **    String::ToKeyValue(" [Example of Section] ", k, v);
        **
        **    // -> k='foo'  v='bar'
        **    String::ToKeyValue(" foo=bar; // comment", k, v);
        **
        **    // -> k='foo'  v='bar'
        **    String::ToKeyValue(" foo=bar; // comments here; ", k, v);
        ** \endcode
        */
        static void ToKeyValue(const String& s, String& key, String& value, const enum CharCase chcase = soCaseSensitive);

        /*!
        ** \brief Find the index of a string in a vector
        ** \param l The vector
        ** \param s The string to look for
        ** \return The index of the string found, -1 otherwise
        */
        static int FindInList(const String::Vector& l, const char* s);
        static int FindInList(const String::Vector& l, const String& s);


        /*!
        ** \brief Convert a string from ASCII to UTF8
        ** \param s The string to convert
        ** \return A new Null-terminated String (must be deleted with the keyword `delete[]`), even if s is NULL
        */
        static char* ConvertToUTF8(const char* s);
        /*!
        ** \brief Convert a string from ASCII to UTF8
        ** \param s The string to convert
        ** \param len The length of the string
        ** \param[out] The new size
        ** \return A new Null-terminated String (must be deleted with the keyword `delete[]`), even if s is NULL
        */
		static char* ConvertToUTF8(const char* s, const Shared::Platform::uint32 len);
		static char* ConvertToUTF8(const char* s, const Shared::Platform::uint32 len, Shared::Platform::uint32& newSize);

        /*!
        ** \brief Convert a string from ASCII to UTF8
        ** \param s The string to convert
        ** \return A new String
        */
        static String ConvertToUTF8(const String& s);

        static char* ConvertFromUTF8(const char* str);

        /*!
        ** \brief Formatted string
        ** \param format The format of the new string
        ** \return A new string
        */
        static String Format(const String& f, ...);
        static String Format(const char* f, ...);

    public:
        //! \name Constructors and Destructor
        //@{
        //! Default constructor
        String() :std::string() {}
        //! Constructor by copy
        String(const String& v, size_type pos = 0, size_type n = npos) :std::string(v, pos, n) {}
        //! Constructor with a default value from a std::string
        String(const std::string& v) :std::string(v) {}
        //! Constructor with a default value from a wide string (wchar_t*)
        String(const wchar_t* v) :std::string() {if (v) *this << v;}
        //! Constructor with a default value from a string (char*)
        String(const char* v) :std::string() { if (v) append(v); }
        //! Constructor with a default value from a string (char*) and a length
        String(const char* v, String::size_type n) :std::string(v, n) {}
        //! Constructor with a default value from a single char
        String(const char v) :std::string() {*this += v;}
        //! Constructor with a default value from an int (8 bits)
        String(const int8_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an int (16 bits)
        String(const int16_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an int (32 bits)
        String(const int32_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an int (64 bits)
        String(const int64_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an unsigned int (8 bits)
        String(const uint8_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an unsigned int (16 bits)
        String(const uint16_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an unsigned int (32 bits)
        String(const uint32_t v) :std::string() {*this << v;}
        //! Constructor with a default value from an unsigned int (64 bits)
        String(const uint64_t v) :std::string() {*this << v;}
        //! Constructor with a default value from a float
        String(const float v) :std::string() {*this << v;}
        //! Constructor with a default value from a double
        String(const double v) :std::string() {*this << v;}
        //! Destructor
        ~String() {}
        //@}


        //! \name The operator `<<`
        //@{
        //! Append a string (char*)
        String& operator << (const char* v) {append(v);return *this;}
        //! Append a string (stl)
        String& operator << (const std::string& v) {append(v);return *this;}
        //! Append a string (Shared::Util::String)
        String& operator << (const String& v) {append(v);return *this;}
        //! Append a single char
        String& operator << (const char v) {*(static_cast<std::string*>(this)) += v; return *this;}
        //! Append a wide string (wchar_t*)
        String& operator << (const wchar_t* v);
        //! Append an int (8 bits)
        String& operator << (const int8_t v) {TA3D_WSTR_APPEND;}
        //! Append an unsigned int (8 bits)
        String& operator << (const uint8_t v) {TA3D_WSTR_APPEND;}
        //! Append an int (16 bits)
        String& operator << (const int16_t v) {TA3D_WSTR_APPEND;}
        //! Append an unsigned int (16 bits)
        String& operator << (const uint16_t v) {TA3D_WSTR_APPEND;}
        //! Append an int (32 bits)
        String& operator << (const int32_t v) {TA3D_WSTR_APPEND;}
        //! Append an unsigned int (32 bits)
        String& operator << (const uint32_t v) {TA3D_WSTR_APPEND;}
        //! Append an int (64 bits)
        String& operator << (const int64_t v) {TA3D_WSTR_APPEND;}
        //! Append an unsigned int (64 bits)
        String& operator << (const uint64_t v) {TA3D_WSTR_APPEND;}
        //! Convert then Append a float
        String& operator << (const float v) {TA3D_WSTR_APPEND;}
        //! Convert then Append a double
        String& operator << (const double v) {TA3D_WSTR_APPEND;}
        //! Convert then Append a boolean (will be converted into "true" or "false")
        String& operator << (const bool v) {TA3D_WSTR_APPEND_BOOL(v);return *this;}
        //@}


        //! \name Convertions
        //@{
        //! Convert this string into an int (8 bits)
        int8_t toInt8() const {TA3D_WSTR_CAST_OP(int8_t);}
        //! Convert this string into an int (16 bits)
        int16_t toInt16() const {TA3D_WSTR_CAST_OP(int16_t);}
        //! Convert this string into an int (32 bits)
        int32_t toInt32() const {TA3D_WSTR_CAST_OP(int32_t);}
        //! Convert this string into an int (64 bits)
        int64_t toInt64() const {TA3D_WSTR_CAST_OP(int64_t);}
        //! Convert this string into an unsigned int (8 bits)
        uint8_t toUInt8() const {TA3D_WSTR_CAST_OP(uint8_t);}
        //! Convert this string into an unsigned int (16 bits)
        uint16_t toUInt16() const {TA3D_WSTR_CAST_OP(uint16_t);}
        //! Convert this string into an unsigned int (32 bits)
        uint32_t toUInt32() const {TA3D_WSTR_CAST_OP(uint32_t);}
        //! Convert this string into an unsigned int (64 bits)
        uint64_t toUInt64() const {TA3D_WSTR_CAST_OP(uint64_t);}
        //! Convert this string into a float
        float toFloat() const {TA3D_WSTR_CAST_OP(float);}
        //! Convert this string into a float with a default value if empty
        float toFloat(const float def) const {if (empty()) return def;TA3D_WSTR_CAST_OP(float);}
        //! Convert this string into a double
        double toDouble() const {TA3D_WSTR_CAST_OP(double);}
        //! Convert this string into a bool (true if the lower case value is equals to "true", "1" or "on")
        bool toBool() const;

        //! \name The operator `+=` (with the same abilities than the operator `<<`)
        //@{
        //! Append a string (char*)
        String& operator += (const char* v) {append(v); return *this;}
        //! Append a string (stl)
        String& operator += (const std::string& v) {append(v); return *this;}
        //! Append a string (Shared::Util::String)
        String& operator += (const Shared::Util::String& v) {append(v); return *this; }
        //! Append a single char
        String& operator += (const char v) {*(static_cast<std::string*>(this)) += (char)v; return *this;}
        //! Append a wide string (wchar_t*)
        String& operator += (const wchar_t* v) {*this << v; return *this;}
        //! Append an int (8 bits)
        String& operator += (const int8_t v) {*this << v; return *this;}
        //! Append an unsigned int (8 bits)
        String& operator += (const uint8_t v) {*this << v; return *this;}
        //! Append an int (16 bits)
        String& operator += (const int16_t v) {*this << v; return *this; }
        //! Append an unsigned int (16 bits)
        String& operator += (const uint16_t v) {*this << v; return *this; }
        //! Append an int (32 bits)
        String& operator += (const int32_t v) {*this << v; return *this; }
        //! Append an unsigned int (32 bits)
        String& operator += (const uint32_t v) {*this << v; return *this; }
        //! Append an int (64 bits)
        String& operator += (const int64_t v) {*this << v; return *this; }
        //! Append an unsigned int (64 bits)
        String& operator += (const uint64_t v) {*this << v; return *this; }
        //! Convert then Append a float
        String& operator += (const float v) {*this << v; return *this; }
        //! Convert then Append a double
        String& operator += (const double v) {*this << v; return *this; }
        //! Convert then Append a boolean (will be converted into "true" or "false")
        String& operator += (const bool v) {TA3D_WSTR_APPEND_BOOL(v); return *this; }
        //@}


        //! \name The operator `+`
        //@{
        //! Create a new String from the concatenation of *this and a string
        const String operator + (const String& rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a char
        const String operator + (const char rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a widechar
        const String operator + (const wchar_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a const char*
        const String operator + (const char* rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a wide string
        const String operator + (const wchar_t* rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a signed int (8 bits)
        const String operator + (const int8_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a signed int (16 bits)
        const String operator + (const int16_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a signed int (32 bits)
        const String operator + (const int32_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a signed int (64 bits)
        const String operator + (const int64_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and an unsigned int (8 bits)
        const String operator + (const uint8_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and an unsigned int (16 bits)
        const String operator + (const uint16_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and an unsigned int (32 bits)
        const String operator + (const uint32_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and an unsigned int (64 bits)
        const String operator + (const uint64_t rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a float
        const String operator + (const float rhs) { return String(*this) += rhs; }
        //! Create a new String from the concatenation of *this and a double
        const String operator + (const double rhs) { return String(*this) += rhs; }
        //@}


        //! \name Case convertion
        //@{
        /*!
        ** \brief Convert the case (lower case) of characters in the string using the UTF8 charset
        ** \return Returns *this
        */
        String& toLower();
        /*!
        ** \brief Convert the case (upper case) of characters in the string using the UTF8 charset
        ** \return Returns *this
        */
        String& toUpper();
        //@} Case convertion


        /*!
        ** \brief Remove trailing and leading spaces
        ** \param trimChars The chars to remove
        ** \return Returns *this
        */
        String& trim(const String& trimChars = TA3D_WSTR_SEPARATORS);


        //! \name Split
        //@{

        /*!
        ** \brief Divide a string into several parts
        ** \param[out] All found parts
        ** \param sepearators Sequence of chars considered as a separator
        ** \param emptyBefore True to clear the vector before fulfill it
        ** \warning Do not take care of string representation (with `'` or `"`)
        */
        void split(String::Vector& out, const String& separators = TA3D_WSTR_SEPARATORS, const bool emptyBefore = true) const;
        /*!
        ** \brief Divide a string into several parts
        ** \param[out] All found parts
        ** \param sepearators Sequence of chars considered as a separator
        ** \param emptyBefore True to clear the list before fulfill it
        ** \warning Do not take care of string representation (with `'` or `"`)
        */
        void split(String::List& out, const String& separators = TA3D_WSTR_SEPARATORS, const bool emptyBefore = true) const;

        //@} Split


        /*!
        ** \brief Extract the key and its value from a string (mainly provided by TDF files)
        **
        ** \param[out] key The key that has been found
        ** \param[out] value The associated value
        **
        ** \see String::ToKeyValue()
        */
        void toKeyValue(String& key, String& value, const enum CharCase chcase = soCaseSensitive) const
        {ToKeyValue(*this, key, value, chcase);}

        /*!
        ** \brief Convert all antislashes into slashes
        ** \return Returns *this
        */
        String& convertAntiSlashesIntoSlashes();

        /*!
        ** \brief Convert all slashes into antislashes
        ** \return Returns *this
        */
        String& convertSlashesIntoAntiSlashes();

        /*!
        ** \brief Get the hash value of this string
        */
        Shared::Platform::uint32 hashValue() const;

        /*!
        ** \brief Convert the string from ASCII to UTF8
        ** \return Always *this
        */
        String& convertToUTF8() {*this = ConvertToUTF8(*this); return *this;}

        /*!
        ** \brief Replace a substr by another one
        ** \param toSearch The string to look for
        ** \param replaceWith The string replacement
        ** \param option Option when looking for the string `toSearch`
        ** \return Always *this
        */
        String& findAndReplace(const String& toSearch, const String& replaceWith, const enum String::CharCase option = soCaseSensitive);
        String& findAndReplace(char toSearch, const char replaceWith, const enum String::CharCase option = soCaseSensitive);

        /*!
        ** \brief Reset the current value with a formatted string
        ** \param f The format of the new string
        ** \return Always *this
        */
        String& format(const String& f, ...);
        String& format(const char* f, ...);

        /*!
        ** \brief Append a formatted string
        ** \param f The format of the new string
        ** \return Always *this
        */
        String& appendFormat(const String& f, ...);
        String& appendFormat(const char* f, ...);
        String& vappendFormat(const char* f, va_list parg);


        /*!
        ** \brief Detect if the string matches the given pattern
        ** \param pattern
        ** \return true if match
        */
        bool match(const String &pattern);

        /*!
        ** \brief returns a substring assuming current string is an UTF8 String
        ** \param pos
        ** \param len
        ** \return the substring
        */
        String substrUTF8(int pos = 0, int len = -1) const;

        /*!
        ** \brief returns the String size assuming it's in UTF8
        ** \return the number of UTF8 symbols
        */
        int sizeUTF8() const;

    private:
        /*!
        ** \brief Convert a string into another type, given by the template parameter `T`
        ** \param[out] t The destination variable
        ** \param s The string to convert
        ** \param f The base to use for the convertion (std::hex, std::dec, ...)
        ** \return True if the operation succeeded, False otherwise
        */
        template <class T>
        bool fromString(T& t, const String& s) const
        {
            std::istringstream iss(s);
            if (s.size() > 2)
            {
                if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
                    iss >> std::hex;
                else if (s[0] == '0' && s[1] != 'x' && s[1] != 'X')
                    iss >> std::oct;
                else
                    iss >> std::dec;
            }
            else
                iss >> std::dec;
            return !(iss >> t).fail();
        }

#ifndef WIN32
		static int ASCIItoUTF8(const Shared::Platform::byte c, Shared::Platform::byte *out);
#else
		static int ASCIItoUTF8(const byte c, byte *out);
#endif
    }; // class String



//int ASCIItoUTF8(const byte c, byte *out);

	/*!
	** \brief Convert an UTF-8 String into a WideChar String
	** \todo This class must be removed as soon as possible and is only here to prevent against
	** stange bugs with the previous implementation
	*/
	struct WString
	{
	public:
		WString(const char* s);
		WString(const String& str);

		void fromUtf8(const char* s, size_t length);
		const wchar_t* cw_str() const {return pBuffer;}

	private:
		wchar_t pBuffer[5120];
	};


	void strrev(char *p);
	void strrev_utf8(char *p);
	void strrev_utf8(std::string &p);
	bool is_string_all_ascii(std::string str);

}} // namespace TA3D

#endif // _SHARED_UTIL_W_STRING_H__
