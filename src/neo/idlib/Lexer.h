#pragma once

#include "precompiled.h"

typedef enum
{
	LEXFL_NOERRORS						= BIT( 0 ),	// don't print any errors
	LEXFL_NOWARNINGS					= BIT( 1 ),	// don't print any warnings
	LEXFL_NOFATALERRORS					= BIT( 2 ),	// errors aren't fatal
	LEXFL_NOSTRINGCONCAT				= BIT( 3 ),	// multiple strings separated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS			= BIT( 4 ),	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE			= BIT( 5 ),	// don't use the $ sign for precompilation
	LEXFL_NOBASEINCLUDES				= BIT( 6 ),	// don't include files embraced with < >
	LEXFL_ALLOWPATHNAMES				= BIT( 7 ),	// allow path seperators in names
	LEXFL_ALLOWNUMBERNAMES				= BIT( 8 ),	// allow names to start with a number
	LEXFL_ALLOWIPADDRESSES				= BIT( 9 ),	// allow ip addresses to be parsed as numbers
	LEXFL_ALLOWFLOATEXCEPTIONS			= BIT( 10 ),	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	LEXFL_ALLOWMULTICHARLITERALS		= BIT( 11 ),	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT	= BIT( 12 ),	// allow multiple strings separated by '\' to be concatenated
	LEXFL_ONLYSTRINGS					= BIT( 13 )	// parse as whitespace deliminated strings (quoted strings keep quotes)
} lexerFlags_t;

// punctuation ids
#define P_RSHIFT_ASSIGN				1
#define P_LSHIFT_ASSIGN				2
#define P_PARMS						3
#define P_PRECOMPMERGE				4

#define P_LOGIC_AND					5
#define P_LOGIC_OR					6
#define P_LOGIC_GEQ					7
#define P_LOGIC_LEQ					8
#define P_LOGIC_EQ					9
#define P_LOGIC_UNEQ				10

#define P_MUL_ASSIGN				11
#define P_DIV_ASSIGN				12
#define P_MOD_ASSIGN				13
#define P_ADD_ASSIGN				14
#define P_SUB_ASSIGN				15
#define P_INC						16
#define P_DEC						17

#define P_BIN_AND_ASSIGN			18
#define P_BIN_OR_ASSIGN				19
#define P_BIN_XOR_ASSIGN			20
#define P_RSHIFT					21
#define P_LSHIFT					22

#define P_POINTERREF				23
#define P_CPP1						24
#define P_CPP2						25
#define P_MUL						26
#define P_DIV						27
#define P_MOD						28
#define P_ADD						29
#define P_SUB						30
#define P_ASSIGN					31

#define P_BIN_AND					32
#define P_BIN_OR					33
#define P_BIN_XOR					34
#define P_BIN_NOT					35

#define P_LOGIC_NOT					36
#define P_LOGIC_GREATER				37
#define P_LOGIC_LESS				38

#define P_REF						39
#define P_COMMA						40
#define P_SEMICOLON					41
#define P_COLON						42
#define P_QUESTIONMARK				43

#define P_PARENTHESESOPEN			44
#define P_PARENTHESESCLOSE			45
#define P_BRACEOPEN					46
#define P_BRACECLOSE				47
#define P_SQBRACKETOPEN				48
#define P_SQBRACKETCLOSE			49
#define P_BACKSLASH					50

#define P_PRECOMP					51
#define P_DOLLAR					52


typedef struct
{
    const char* p;
    int n;
} punctuation_t;

class idLexer
{
public:
    idLexer();
    idLexer( const char *ptr, int length, const char *name, int flags = 0 );

    int LoadMemory(const char* ptr, int length, const char* name, int startline = 0);

    void SetFlags(int flags);
    int ReadWhiteSpace();

    int ReadToken(idToken* token);
    int ReadString(idToken* token, int quote);
    int ReadEscapeCharacter(char* ch);
    int ReadName(idToken* token);
    int ReadNumber(idToken* token);
    int ReadPunctuation(idToken* token);
    int ExpectTokenString(std::string str);
    int CheckTokenString(std::string string);
private:
    void Warning(const char* str, ...);
    void Error(const char* str, ...);

    int CheckString(const char* str);

    bool loaded;
    const char* filename;
    const char* buffer;
    ID_TIME_T fileTime;
    int length;
    const char* script_p;
    const char* lastScript_p;
    const char* end_p;
    int				line;					// current line in script
	int				lastline;				// line before reading token
	int				intialLine;				// line that was set on load as starting line
	int				tokenavailable;			// set by unreadToken
	int				flags;	
    const punctuation_t *punctuations;
    idToken token;
    const char* 	whiteSpaceStart_p;					// start of white space before token, only used by idLexer
	const char* 	whiteSpaceEnd_p;					// end of white space before token, only used by idLexer
};

inline void idLexer::SetFlags(int flags)
{
    idLexer::flags = flags;
}
