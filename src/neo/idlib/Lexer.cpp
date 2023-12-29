#include "Lexer.h"
#include "lib.h"

punctuation_t default_punctuations[] = {
	//binary operators
	{">>=",P_RSHIFT_ASSIGN},
	{"<<=",P_LSHIFT_ASSIGN},
	//
	{"...",P_PARMS},
	//define merge operator
	{"##",P_PRECOMPMERGE},				// pre-compiler
	//logic operators
	{"&&",P_LOGIC_AND},					// pre-compiler
	{"||",P_LOGIC_OR},					// pre-compiler
	{">=",P_LOGIC_GEQ},					// pre-compiler
	{"<=",P_LOGIC_LEQ},					// pre-compiler
	{"==",P_LOGIC_EQ},					// pre-compiler
	{"!=",P_LOGIC_UNEQ},				// pre-compiler
	//arithmatic operators
	{"*=",P_MUL_ASSIGN},
	{"/=",P_DIV_ASSIGN},
	{"%=",P_MOD_ASSIGN},
	{"+=",P_ADD_ASSIGN},
	{"-=",P_SUB_ASSIGN},
	{"++",P_INC},
	{"--",P_DEC},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN},
	{"|=",P_BIN_OR_ASSIGN},
	{"^=",P_BIN_XOR_ASSIGN},
	{">>",P_RSHIFT},					// pre-compiler
	{"<<",P_LSHIFT},					// pre-compiler
	//reference operators
	{"->",P_POINTERREF},
	//C++
	{"::",P_CPP1},
	{".*",P_CPP2},
	//arithmatic operators
	{"*",P_MUL},						// pre-compiler
	{"/",P_DIV},						// pre-compiler
	{"%",P_MOD},						// pre-compiler
	{"+",P_ADD},						// pre-compiler
	{"-",P_SUB},						// pre-compiler
	{"=",P_ASSIGN},
	//binary operators
	{"&",P_BIN_AND},					// pre-compiler
	{"|",P_BIN_OR},						// pre-compiler
	{"^",P_BIN_XOR},					// pre-compiler
	{"~",P_BIN_NOT},					// pre-compiler
	//logic operators
	{"!",P_LOGIC_NOT},					// pre-compiler
	{">",P_LOGIC_GREATER},				// pre-compiler
	{"<",P_LOGIC_LESS},					// pre-compiler
	//reference operator
	{".",P_REF},
	//seperators
	{",",P_COMMA},						// pre-compiler
	{";",P_SEMICOLON},
	//label indication
	{":",P_COLON},						// pre-compiler
	//if statement
	{"?",P_QUESTIONMARK},				// pre-compiler
	//embracements
	{"(",P_PARENTHESESOPEN},			// pre-compiler
	{")",P_PARENTHESESCLOSE},			// pre-compiler
	{"{",P_BRACEOPEN},					// pre-compiler
	{"}",P_BRACECLOSE},					// pre-compiler
	{"[",P_SQBRACKETOPEN},
	{"]",P_SQBRACKETCLOSE},
	//
	{"\\",P_BACKSLASH},
	//precompiler operator
	{"#",P_PRECOMP},					// pre-compiler
	{"$",P_DOLLAR},
	{NULL, 0}
};

idLexer::idLexer()
{
    idLexer::loaded = false;
    idLexer::punctuations = default_punctuations;
}

idLexer::idLexer(const char * ptr, int length, const char * name, int flags)
{
    idLexer::loaded = false;
    idLexer::flags = flags;
    idLexer::LoadMemory(ptr, length, name);
    idLexer::punctuations = default_punctuations;
}

int idLexer::LoadMemory(const char *ptr, int length, const char *name, int startline)
{
    if (idLexer::loaded)
    {
        idLib::common->Error("idLexer::LoadMemory: another script already loaded");
        return false;
    }

    idLexer::filename = name;
    idLexer::buffer = ptr;
    idLexer::fileTime = 0;
    idLexer::length = length;
    idLexer::script_p = idLexer::buffer;
    idLexer::lastScript_p = idLexer::buffer;
    idLexer::end_p = &(idLexer::buffer[length]);

    idLexer::tokenavailable = 0;
    idLexer::line = startline;
    idLexer::lastline = startline;
    idLexer::intialLine = startline;
    idLexer::loaded = true;

    return true;
}

int idLexer::ReadWhiteSpace()
{
    while (1)
    {
        while (*idLexer::script_p <= ' ')
        {
            if (!*idLexer::script_p)
            {
                return 0;
            }
            if (*idLexer::script_p == '\n')
            {
                idLexer::line++;
            }
            idLexer::script_p++;
        }
        if (*idLexer::script_p == '/')
        {
            if (*(idLexer::script_p+1) == '/')
            {
                idLexer::script_p++;
                do
                {
                    idLexer::script_p++;
                    if (*idLexer::script_p)
                    {
                        return 0;
                    }
                } while (*idLexer::script_p != '\n');
                idLexer::line++;
                idLexer::script_p++;
                if (!*idLexer::script_p)
                {
                    return 0;
                }
                continue;
            }
            else if (*(idLexer::script_p + 1) == '*')
            {
                idLexer::script_p++;
				while( 1 )
				{
					idLexer::script_p++;
					if( !*idLexer::script_p )
					{
						return 0;
					}
					if( *idLexer::script_p == '\n' )
					{
						idLexer::line++;
					}
					else if( *idLexer::script_p == '/' )
					{
						if( *( idLexer::script_p - 1 ) == '*' )
						{
							break;
						}
						if( *( idLexer::script_p + 1 ) == '*' )
						{
							idLexer::Warning( "nested comment" );
						}
					}
				}
				idLexer::script_p++;
				if( !*idLexer::script_p )
				{
					return 0;
				}
				idLexer::script_p++;
				if( !*idLexer::script_p )
				{
					return 0;
				}
				continue;
            }
        }

        break;
    }

    return 1;
}

int idLexer::ReadToken(idToken *token)
{
    int c;

    if (!loaded)
    {
        idLib::common->Error("idLexer::ReadToken: No file loaded\n");
        return 0;
    }

    if (script_p == NULL)
    {
        return 0;
    }

    if (tokenavailable)
    {
        tokenavailable = 0;
        *token = idLexer::token;
        return 1;
    }

    lastScript_p = script_p;
    lastline = line;

    token->data[0] = '\0';
    token->len = 0;

    whiteSpaceStart_p = script_p;
    token->whiteSpaceStart_p = script_p;

    if (!ReadWhiteSpace())
    {
        return 0;
    }

    idLexer::whiteSpaceEnd_p = script_p;
    token->whiteSpaceEnd_p = script_p;

    token->line = line;

    token->linescrossed = line - lastline;

    token->flags = 0;

    c = *idLexer::script_p;

    if (idLexer::flags & LEXFL_ONLYSTRINGS)
    {
        if (c == '\"' || c == '\'')
        {
            if (!idLexer::ReadString(token, c))
            {
                return 0;
            }
        }
        else if (!idLexer::ReadName(token))
        {
            return 0;
        }
    }
    else if( ( c >= '0' && c <= '9' ) ||
			 ( c == '.' && ( *( idLexer::script_p + 1 ) >= '0' && *( idLexer::script_p + 1 ) <= '9' ) ) )
	{
        if (!idLexer::ReadNumber(token))
        {
            return 0;
        }

        if (idLexer::flags & LEXFL_ALLOWNUMBERNAMES)
        {
            c = *idLexer::script_p;
            if ((c >= 'a' && c <= 'z'))
            {
                if (!idLexer::ReadName(token))
                {
                    return 0;
                }
            }
        }
    }
    else if (c == '\"' || c == '\'')
    {
        if (!idLexer::ReadString(token, c))
        {
            return 0;
        }
    }
    else if ((c >= 'a' && c <= 'z') || ( c >= 'A' && c <= 'Z' ) || c == '_')
    {
        if (!idLexer::ReadName(token))
        {
            return 0;
        }
    }
    else if (idLexer::flags & LEXFL_ALLOWPATHNAMES)
    {
        if( !idLexer::ReadName( token ) )
		{
			return 0;
		}
    }
    else if (!idLexer::ReadPunctuation(token))
    {
        common->Error("unknown punctuation %c", c);
        return 0;
    }

    return 1;
}

int idLexer::ReadString(idToken *token, int quote)
{
    int tmpline;
    const char* tmpscript_p;
    char ch;

    if (quote == '\"')
    {
        token->type = TT_STRING;
    }
    else
    {
        token->type = TT_LITERAL;
    }

    idLexer::script_p++;

    while (1)
    {
        if (*idLexer::script_p == '\\' && !(idLexer::flags & LEXFL_NOSTRINGESCAPECHARS))
        {
            if (!idLexer::ReadEscapeCharacter(&ch))
            {
                return 0;
            }
            token->AppendDirty(ch);
        }
        else if (*idLexer::script_p == quote)
        {
            idLexer::script_p++;
            if ((idLexer::flags & LEXFL_NOSTRINGCONCAT) &&
					( !( idLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT ) || ( quote != '\"' ) ))
            {
                break;
            }

            tmpscript_p = idLexer::script_p;
            tmpline = idLexer::line;
            if (!idLexer::ReadWhiteSpace())
            {
                idLexer::script_p = tmpscript_p;
                idLexer::line = tmpline;
                break;
            }

            if( idLexer::flags & LEXFL_NOSTRINGCONCAT )
			{
				if( *idLexer::script_p != '\\' )
				{
					idLexer::script_p = tmpscript_p;
					idLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				idLexer::script_p++;
				if( !idLexer::ReadWhiteSpace() || ( *idLexer::script_p != quote ) )
				{
					idLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if( *idLexer::script_p != quote )
			{
				idLexer::script_p = tmpscript_p;
				idLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			idLexer::script_p++;
        }
        else
        {
            if (*idLexer::script_p == '\0')
            {
                idLexer::Error("missing trailing quote");
                return 0;
            }
            if (*idLexer::script_p == '\n')
            {
                idLexer::Error("newline inside string");
                return 0;
            }
            token->AppendDirty(*idLexer::script_p++);
        }
    }
    token->data[token->len] = '\0';

    if (token->type == TT_LITERAL)
    {
        if (!(idLexer::flags & LEXFL_ALLOWMULTICHARLITERALS))
        {
            if (token->Length() != 1)
            {
                idLexer::Warning("literal is not one character");
            }
        }
        token->subtype = (*token)[0];
    }
    else
    {
        token->subtype = token->Length();
    }
    return 1;
}

int idLexer::ReadEscapeCharacter(char *ch)
{
    int c, val, i;

    idLexer::script_p++;

    switch (*idLexer::script_p)
    {
    case '\\':
        c = '\\';
        break;
    case 'n':
        c = '\n';
        break;
    case 'r':
        c = '\r';
        break;
    case 't':
        c = '\n';
        break;
    case 'v':
        c = '\n';
        break;
    case 'b':
        c = '\n';
        break;
    case 'f':
        c = '\n';
        break;
    case 'a':
        c = '\n';
        break;
    case '\'':
        c = '\n';
        break;
    case '\"':
        c = '\n';
        break;
    case '\?':
        c = '\?';
        break;
    case 'x':
    {
        idLexer::script_p++;
        for (i = 0, val = 0; ; i++, idLexer::script_p++)
        {
            if (c >= '0' && c <= '9')
            {
                c = c - '0';
            }
            else if (c >= 'A' && c <= 'Z')
            {
                c = c - 'A' + 10;
            }
            else if (c >= 'a' && c <= 'z')
            {
                c = c - 'a' + 10;
            }
            else
            {
                break;
            }
            val = (val << 4) + c;
        }
        idLexer::script_p--;
        if (val > 0xFF)
        {
            idLexer::Warning("too large value in escape character");
            val = 0xFF;
        }
        c = val;
        break;
    }
    default:
    {
        if (*idLexer::script_p < '0' || *idLexer::script_p > '9')
        {
            idLexer::Error("unknown escape character");
        }
        for (i = 0, val = 0; ; i++, idLexer::script_p++)
        {
            c = *idLexer::script_p;
            if (c >= '0' && c <= '9')
            {
                c = c - '0';
            }
            else
            {
                break;
            }
            val = val * 10 + c;
        }
        idLexer::script_p--;
        if( val > 0xFF )
		{
			idLexer::Warning( "too large value in escape character" );
			val = 0xFF;
		}
		c = val;
		break;
    }
    }

    idLexer::script_p++;
    *ch = c;

    return 1;
}

int idLexer::ReadName(idToken *token)
{
    char c;

	token->type = TT_NAME;
	do
	{
		token->AppendDirty( *idLexer::script_p++ );
		c = *idLexer::script_p;
	}
	while( ( c >= 'a' && c <= 'z' ) ||
			( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) ||
			c == '_' ||
			// if treating all tokens as strings, don't parse '-' as a separate token
			( ( idLexer::flags & LEXFL_ONLYSTRINGS ) && ( c == '-' ) ) ||
			// if special path name characters are allowed
			( ( idLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( c == '/' || c == '\\' || c == ':' || c == '.' ) ) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

int idLexer::ReadNumber(idToken *token)
{
    int i, dot;
    char c, c2;

    token->type = TT_NUMBER;
    token->subtype = 0;
    token->intValue = 0;
    token->floatValue = 0;

    c = *idLexer::script_p;
    c2 = *(idLexer::script_p + 1);

    if (c == '0' && c2 != '.')
    {
        if (c2 == 'x' || c2 == 'X')
        {
            token->AppendDirty(*idLexer::script_p++);
            token->AppendDirty(*idLexer::script_p++);
            c = *idLexer::script_p;
            while ((c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z'))
            {
                token->AppendDirty(c);
                c = *(++idLexer::script_p);
            }
            token->subtype = TT_HEX | TT_INTEGER;
        }
        else if (c2 == 'b' || c2 == 'B')
        {
            token->AppendDirty( *idLexer::script_p++ );
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( c == '0' || c == '1' )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype = TT_BINARY | TT_INTEGER;
        }
        else
        {
            token->AppendDirty(*idLexer::script_p++);
            c = *idLexer::script_p;
            while (c >= '0' && c <= '7')
            {
                token->AppendDirty(c);
                c = *(++idLexer::script_p);
            }
            token->subtype = TT_OCTAL | TT_INTEGER;
        }
    }
    else
    {
        dot = 0;
        while (1)
        {
            if (c >= '0' && c <= '9')
            {}
            else if (c == '.')
            {
                dot++;
            }
            else
            {
                break;
            }
            token->AppendDirty(c);
            c = *(++idLexer::script_p);
        }
        if (c == 'e' && dot == 0)
        {
            dot++;
        }

        if (dot == 1)
        {
            token->subtype = TT_DECIMAL | TT_FLOAT;
            if (c == 'e')
            {
                token->AppendDirty(c);
                c = *(++idLexer::script_p);
                if( c == '-' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				else if( c == '+' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
            }
            else if (c == '#')
            {
                c2 = 4;
                if( CheckString( "INF" ) )
				{
					token->subtype |= TT_INFINITE;
				}
				else if( CheckString( "IND" ) )
				{
					token->subtype |= TT_INDEFINITE;
				}
				else if( CheckString( "NAN" ) )
				{
					token->subtype |= TT_NAN;
				}
				else if( CheckString( "QNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2++;
				}
				else if( CheckString( "SNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2++;
				}
                for (i = 0; i < c2; i++)
                {
                    token->AppendDirty(c);
                    c = *(++idLexer::script_p);
                }
                while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++idLexer::script_p );
				}
				if( !( idLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS ) )
				{
					token->AppendDirty( 0 );	// zero terminate for c_str
					idLexer::Error( "parsed %s", token->data );
				}
            }
        }
        else if (dot > 1)
        {
            if( !( idLexer::flags & LEXFL_ALLOWIPADDRESSES ) )
			{
				idLexer::Error( "more than one dot in number" );
				return 0;
			}
			if( dot != 3 )
			{
				idLexer::Error( "ip address should have three dots" );
				return 0;
			}
			token->subtype = TT_IPADDRESS;
        }
        else
        {
            token->subtype = TT_DECIMAL | TT_INTEGER;
        }
    }

    if( token->subtype & TT_FLOAT )
	{
		if( c > ' ' )
		{
			// single-precision: float
			if( c == 'f' || c == 'F' )
			{
				token->subtype |= TT_SINGLE_PRECISION;
				idLexer::script_p++;
			}
			// extended-precision: long double
			else if( c == 'l' || c == 'L' )
			{
				token->subtype |= TT_EXTENDED_PRECISION;
				idLexer::script_p++;
			}
			// default is double-precision: double
			else
			{
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else
		{
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if( token->subtype & TT_INTEGER )
	{
		if( c > ' ' )
		{
			// default: signed long
			for( i = 0; i < 2; i++ )
			{
				// long integer
				if( c == 'l' || c == 'L' )
				{
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if( c == 'u' || c == 'U' )
				{
					token->subtype |= TT_UNSIGNED;
				}
				else
				{
					break;
				}
				c = *( ++idLexer::script_p );
			}
		}
	}
	else if( token->subtype & TT_IPADDRESS )
	{
		if( c == ':' )
		{
			token->AppendDirty( c );
			c = *( ++idLexer::script_p );
			while( c >= '0' && c <= '9' )
			{
				token->AppendDirty( c );
				c = *( ++idLexer::script_p );
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

int idLexer::ReadPunctuation(idToken * token)
{
    int l, n, i;
    const char* p;
    const punctuation_t* punc;

    for (i = 0; idLexer::punctuations[i].p; i++)
    {
        punc = &idLexer::punctuations[i];
        p = punc->p;

        for (l = 0; p[l] && idLexer::script_p[l]; l++)
            if (idLexer::script_p[l] != p[l])
                break;
    
        if (!p[l])
        {
            for (i = 0; i <= l; i++)
                token->data[i] = p[i];
            token->len = l;
            idLexer::script_p += l;
            token->type = TT_PUNCTUTATION;
            token->subtype = punc->n;
            return 1;
        }
    }

    return 0;
}

int idLexer::ExpectTokenString(std::string str)
{
    idToken token;

    if (!idLexer::ReadToken(&token))
    {
        idLexer::Error("Couldn't find expected '%s'", str.c_str());
        return 0;
    }

    if (token.c_str() != str)
    {
        idLexer::Error("Couldn't find expected '%s'", str.c_str());
        return 0;
    }

    return 1;
}

int idLexer::CheckTokenString(std::string string)
{
    idToken tok;

    if (!ReadToken(&tok))
        return 0;
    
    if (tok.c_str() == string)
        return 1;
    
    script_p = lastScript_p;
    line = lastline;
    return 0;
}

void idLexer::Warning(const char *str, ...)
{
    char text[1024];
    va_list ap;

    if (idLexer::flags & LEXFL_NOWARNINGS)
    {
        return;
    }

    va_start(ap, str);
    vsnprintf(text, sizeof(text), str, ap);
    va_end(ap);
    idLib::common->Warning("file %s, line %d: %s", filename, idLexer::line, text);
}

void idLexer::Error(const char *str, ...)
{
    char text[1024];
    va_list ap;

    if (idLexer::flags & LEXFL_NOWARNINGS)
    {
        return;
    }

    va_start(ap, str);
    vsnprintf(text, sizeof(text), str, ap);
    va_end(ap);
    idLib::common->Error("file %s, line %d: %s", filename, idLexer::line, text);
}

int idLexer::CheckString(const char *str)
{
    int i;

    for (i = 0; str[i]; i++)
    {
        if (idLexer::script_p[i] != str[i])
        {
            return false;
        }
    }

    return true;
}
