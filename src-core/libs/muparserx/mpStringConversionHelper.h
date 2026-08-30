/*
               __________                                 ____  ___
    _____  __ _\______   \_____ _______  ______ __________\   \/  /
   /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     / 
  |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \ 
  |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
        \/                     \/           \/     \/           \_/
                                       Copyright (C) 2023, Ingo Berg
                                       All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
  POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef MP_STRING_CONVERSION_HELPER_H
#define MP_STRING_CONVERSION_HELPER_H

#include <cstdlib>   // for strtod
#include <cwchar>    // for wcstof
#include <cstring>   // for strlen
#include <type_traits>  // for enable_if, is_floating_point

MUP_NAMESPACE_START

template <typename TChar>
class StringConversionHelper 
{
    public:
        static size_t StrLen(const TChar* str) 
        {
            static_assert(std::is_same<TChar, char>::value || std::is_same<TChar, wchar_t>::value, "TChar must be either char or wchar_t");
            return StrLenImpl(str, std::integral_constant<bool, std::is_same<TChar, char>::value>());
        }

        static double ParseDouble(const TChar* str, int &parsedLen, bool& success) 
        {
            static_assert(std::is_same<TChar, char>::value || std::is_same<TChar, wchar_t>::value, "TChar must be either char or wchar_t");
            return ParseDoubleImpl(str, parsedLen, success, std::integral_constant<bool, std::is_same<TChar, char>::value>());
        }

    private:
        static size_t StrLenImpl(const char* str, std::true_type) 
        {
            return std::strlen(str);
        }

        static size_t StrLenImpl(const wchar_t* str, std::false_type) 
        {
            return std::wcslen(str);
        }

        static double ParseDoubleImpl(const char* str, int &parsedLen, bool& success, std::true_type) 
        {
            char* endptr;
            double value = std::strtod(str, &endptr);
            success = (endptr != str);
            parsedLen = endptr - str;
            return value;
        }

        static double ParseDoubleImpl(const wchar_t* str, int &parsedLen, bool& success, std::false_type) 
        {
            wchar_t* endptr;
            double value = std::wcstod(str, &endptr);
            success = (endptr != str);
            parsedLen = endptr - str;
            return value;
        }
};


MUP_NAMESPACE_END

#endif