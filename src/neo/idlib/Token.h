#pragma once

#include "precompiled.h"

#define TT_STRING 1
#define TT_LITERAL 2
#define TT_NUMBER 3
#define TT_NAME 4
#define TT_PUNCTUTATION 5

#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000

class idToken
{
    friend class idParser;
    friend class idLexer;
public:
    int type;
    int subtype;
    int line;
    int linescrossed;
    int flags;
public:
    idToken();

    void AppendDirty(const char a);

    int Length() {return len;}

    const char* c_str() {return data;}

    char operator[](int index) const {return data[index];}
    char& operator[](int index) {return data[index];}
private:
    unsigned long intValue;
    double floatValue;
    const char* whiteSpaceStart_p;
    const char* whiteSpaceEnd_p;
    idToken* next;
    char data[32];
    int len;
};

inline idToken::idToken() : type(), subtype(), line(), linescrossed(), flags()
{len = 0;}

inline void idToken::AppendDirty(const char a)
{
    assert(strlen(data) < 31);

    data[len++] = a;
}
