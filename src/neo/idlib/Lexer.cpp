#include "Lexer.h"
#include "lib.h"

idLexer::idLexer()
{
    idLexer::loaded = false;
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
    else
    {
        idLexer::Error("TODO: Punctuation");
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
