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

class idLexer
{
public:
    idLexer();

    int LoadMemory(const char* ptr, int length, const char* name, int startline = 0);

    void SetFlags(int flags);
    int ReadWhiteSpace();

    int ReadToken(idToken* token);
    int ReadString(idToken* token, int quote);
    int ReadEscapeCharacter(char* ch);
    int ReadName(idToken* token);
    int ReadNumber(idToken* token);
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
    idToken token;
    const char* 	whiteSpaceStart_p;					// start of white space before token, only used by idLexer
	const char* 	whiteSpaceEnd_p;					// end of white space before token, only used by idLexer
};

inline void idLexer::SetFlags(int flags)
{
    idLexer::flags = flags;
}
