#include "precompiled.h"
#include "Lexer.h"
#include "CmdArgs.h"

void idCmdArgs::TokenizeString(const char *text, bool keepAsStrings)
{
    idLexer lex;
    idToken token, number;
    int len, totalLen;

    argc = 0;

    if (!text)
        return;
    
    lex.LoadMemory(text, strlen(text), "idCmdSystemLocal::TokenizeString");
    lex.SetFlags(LEXFL_NOSTRINGCONCAT
                    | LEXFL_ALLOWPATHNAMES
                    | LEXFL_NOSTRINGESCAPECHARS
                    | LEXFL_ALLOWIPADDRESSES | ( keepAsStrings ? LEXFL_ONLYSTRINGS : 0 ) );
    
    totalLen = 0;

    while (1)
    {
        if (argc == MAX_COMMAND_ARGS)
            return;
        
        if (!lex.ReadToken(&token))
            return;
        
        printf("New token: %s\n", token.c_str());
    }
}

void idCmdArgs::AppendArg(const char *text)
{
    if (argc >= MAX_COMMAND_ARGS)
    {
        return;
    }
    if (!argc)
    {
        argc = 1;
        argv[0] = tokenized;
        strncpy(tokenized, text, sizeof(tokenized));
        tokenized[strlen(tokenized)] = '\0';
    }
    else
    {
        argv[argc] = argv[argc-1] + strlen(argv[argc-1]) + 1;
        strncpy(argv[argc], text, sizeof( tokenized ) - ( argv[ argc ] - tokenized ));
        tokenized[strlen(tokenized)] = '\0';
        argc++;
    }
}