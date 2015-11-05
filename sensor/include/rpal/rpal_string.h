/*
Copyright 2015 refractionPOINT

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _RPAL_STRING_H
#define _RPAL_STRING_H

#include <rpal/rpal.h>

RBOOL 
    rpal_string_isprint 
    (
        RCHAR ch
    );

RBOOL
    rpal_string_iswprint
    (
        RWCHAR ch
    );

RU8
    rpal_string_str_to_byte
    (
        RPCHAR str
    );

RVOID
    rpal_string_byte_to_str
    (
        RU8 b,
        RCHAR c[ 2 ]
    );

RU32
    rpal_string_strlen
    (
        RPCHAR str
    );

RU32
    rpal_string_strlenw
    (
        RPWCHAR str
    );

RU32
    rpal_string_strsize
    (
        RPCHAR str
    );

RU32
    rpal_string_strsizew
    (
        RPWCHAR str
    );

RBOOL
    rpal_string_expand
    (
        RPCHAR  str,
        RPCHAR*  outStr
    );

RBOOL
    rpal_string_expandw
    (
        RPWCHAR  str,
        RPWCHAR*  outStr
    );


RPWCHAR
    rpal_string_atow
    (
        RPCHAR str
    );

RPCHAR
    rpal_string_wtoa
    (
        RPWCHAR str
    );

RPWCHAR
    rpal_string_strcatw
    (
        RPWCHAR str,
        RPWCHAR toAdd
    );

RPCHAR
    rpal_string_strcata
    (
        RPCHAR str,
        RPCHAR toAdd
    );

RPCHAR
    rpal_string_strstr
    (
        RPCHAR haystack,
        RPCHAR needle
    );

RPWCHAR
    rpal_string_strstrw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    );

RPWCHAR
    rpal_string_stristrw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    );

RPCHAR
    rpal_string_itoa
    (
        RU32 num,
        RPCHAR outBuff,
        RU32 radix
    );

RPWCHAR
    rpal_string_itow
    (
        RU32 num,
        RPWCHAR outBuff,
        RU32 radix
    );

RPCHAR
    rpal_string_strdupa
    (
        RPCHAR strA
    );

RPWCHAR
    rpal_string_strdupw
    (
        RPWCHAR strW

    );

RBOOL
    rpal_string_match
    (
        RPCHAR pattern,
        RPCHAR str
    );

RBOOL
    rpal_string_matchw
    (
        RPWCHAR pattern,
        RPWCHAR str
    );
    
RPCHAR
    rpal_string_strcatExA
    (
        RPCHAR strToExpand,
        RPCHAR strToCat
    );

RPWCHAR
    rpal_string_strcatExW
    (
        RPWCHAR strToExpand,
        RPWCHAR strToCat
    );

RPCHAR
    rpal_string_strtoka
    (
        RPCHAR str,
        RCHAR token,
        RPCHAR* state
    );

RPWCHAR
    rpal_string_strtokw
    (
        RPWCHAR str,
        RWCHAR token,
        RPWCHAR* state
    );

RS32
    rpal_string_strcmpw
    (
        RPWCHAR str1,
        RPWCHAR str2
    );

RS32
    rpal_string_strcmpa
    (
        RPCHAR str1,
        RPCHAR str2
    );

RS32
    rpal_string_stricmpa
    (
        RPCHAR str1,
        RPCHAR str2
    );

RS32
    rpal_string_stricmpw
    (
        RPWCHAR str1,
        RPWCHAR str2
    );

RPCHAR
    rpal_string_touppera
    (
        RPCHAR str
    );

RPWCHAR
    rpal_string_toupperw
    (
        RPWCHAR str
    );

RPCHAR
    rpal_string_tolowera
    (
        RPCHAR str
    );

RPWCHAR
    rpal_string_tolowerw
    (
        RPWCHAR str
    );

RPCHAR
    rpal_string_strcpya
    (
        RPCHAR dst,
        RPCHAR src
    );

RPWCHAR
    rpal_string_strcpyw
    (
        RPWCHAR dst,
        RPWCHAR src
    );

RBOOL
    rpal_string_atoi
    (
        RPCHAR str,
        RU32* pNum
    );
    
RBOOL
    rpal_string_wtoi
    (
        RPWCHAR str,
        RU32* pNum
    );

RBOOL
    rpal_string_filla
    (
        RPCHAR str,
        RU32 nChar,
        RCHAR fillWith
    );

RBOOL
    rpal_string_fillw
    (
        RPWCHAR str,
        RU32 nChar,
        RWCHAR fillWith
    );
    
RBOOL
    rpal_string_startswitha
    (
        RPCHAR haystack,
        RPCHAR needle
    );

RBOOL
    rpal_string_startswithw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    );

RBOOL
    rpal_string_startswithia
    (
        RPCHAR haystack,
        RPCHAR needle
    );

RBOOL
    rpal_string_startswithiw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    );

RBOOL
    rpal_string_endswitha
    (
        RPCHAR haystack,
        RPCHAR needle
    );

RBOOL
    rpal_string_endswithw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    );

RBOOL
    rpal_string_trima
    (
        RPCHAR str,
        RPCHAR charsToTrim
    );

RBOOL
    rpal_string_trimw
    (
        RPWCHAR str,
        RPWCHAR charsToTrim
    );

RBOOL
    rpal_string_charIsAscii
    (
        RCHAR c
    );

#include <stdio.h>
#define rpal_string_snprintf(outStr,buffLen,format,...) snprintf((outStr),(buffLen),(format),__VA_ARGS__)
#define rpal_string_sscanf(inStr,format,...) sscanf((inStr),(format),__VA_ARGS__)

#define rpal_string_isEmpty(str) (NULL == (str) && 0 == (str)[ 0 ])

#endif
