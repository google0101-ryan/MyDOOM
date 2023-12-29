#include "KeyInput.h"
#include "CvarSystem.h"
#include "CmdSystem.h"

typedef struct
{
    keyNum_t keynum;
    const char* name;
    const char* strId;
} keyname_t;

#define NAMEKEY( code, strId ) { K_##code, #code, strId }
#define NAMEKEY2( code ) { K_##code, #code, #code }

#define ALIASKEY( alias, code ) { K_##code, alias, "" }

keyname_t keynames[] =
{
    NAMEKEY(ESCAPE, "#str_07020"),
    NAMEKEY2( 1 ),
	NAMEKEY2( 2 ),
	NAMEKEY2( 3 ),
	NAMEKEY2( 4 ),
	NAMEKEY2( 5 ),
	NAMEKEY2( 6 ),
	NAMEKEY2( 7 ),
	NAMEKEY2( 8 ),
	NAMEKEY2( 9 ),
	NAMEKEY2( 0 ),
	NAMEKEY( MINUS, "-" ),
	NAMEKEY( EQUALS, "=" ),
	NAMEKEY( BACKSPACE, "#str_07022" ),
	NAMEKEY( TAB, "#str_07018" ),
	NAMEKEY2( Q ),
	NAMEKEY2( W ),
	NAMEKEY2( E ),
	NAMEKEY2( R ),
	NAMEKEY2( T ),
	NAMEKEY2( Y ),
	NAMEKEY2( U ),
	NAMEKEY2( I ),
	NAMEKEY2( O ),
	NAMEKEY2( P ),
	NAMEKEY( LBRACKET, "[" ),
	NAMEKEY( RBRACKET, "]" ),
	NAMEKEY( ENTER, "#str_07019" ),
	NAMEKEY( LCTRL, "#str_07028" ),
	NAMEKEY2( A ),
	NAMEKEY2( S ),
	NAMEKEY2( D ),
	NAMEKEY2( F ),
	NAMEKEY2( G ),
	NAMEKEY2( H ),
	NAMEKEY2( J ),
	NAMEKEY2( K ),
	NAMEKEY2( L ),
	NAMEKEY( SEMICOLON, "#str_07129" ),
	NAMEKEY( APOSTROPHE, "#str_07130" ),
	NAMEKEY( GRAVE, "`" ),
	NAMEKEY( LSHIFT, "#str_07029" ),
	NAMEKEY( BACKSLASH, "\\" ),
	NAMEKEY2( Z ),
	NAMEKEY2( X ),
	NAMEKEY2( C ),
	NAMEKEY2( V ),
	NAMEKEY2( B ),
	NAMEKEY2( N ),
	NAMEKEY2( M ),
	NAMEKEY( COMMA, "," ),
	NAMEKEY( PERIOD, "." ),
	NAMEKEY( SLASH, "/" ),
	NAMEKEY( RSHIFT, "#str_bind_RSHIFT" ),
	NAMEKEY( KP_STAR, "#str_07126" ),
	NAMEKEY( LALT, "#str_07027" ),
	NAMEKEY( SPACE, "#str_07021" ),
	NAMEKEY( CAPSLOCK, "#str_07034" ),
	NAMEKEY( F1, "#str_07036" ),
	NAMEKEY( F2, "#str_07037" ),
	NAMEKEY( F3, "#str_07038" ),
	NAMEKEY( F4, "#str_07039" ),
	NAMEKEY( F5, "#str_07040" ),
	NAMEKEY( F6, "#str_07041" ),
	NAMEKEY( F7, "#str_07042" ),
	NAMEKEY( F8, "#str_07043" ),
	NAMEKEY( F9, "#str_07044" ),
	NAMEKEY( F10, "#str_07045" ),
	NAMEKEY( NUMLOCK, "#str_07125" ),
	NAMEKEY( SCROLL, "#str_07035" ),
	NAMEKEY( KP_7, "#str_07110" ),
	NAMEKEY( KP_8, "#str_07111" ),
	NAMEKEY( KP_9, "#str_07112" ),
	NAMEKEY( KP_MINUS, "#str_07123" ),
	NAMEKEY( KP_4, "#str_07113" ),
	NAMEKEY( KP_5, "#str_07114" ),
	NAMEKEY( KP_6, "#str_07115" ),
	NAMEKEY( KP_PLUS, "#str_07124" ),
	NAMEKEY( KP_1, "#str_07116" ),
	NAMEKEY( KP_2, "#str_07117" ),
	NAMEKEY( KP_3, "#str_07118" ),
	NAMEKEY( KP_0, "#str_07120" ),
	NAMEKEY( KP_DOT, "#str_07121" ),
	NAMEKEY( F11, "#str_07046" ),
	NAMEKEY( F12, "#str_07047" ),
	NAMEKEY2( F13 ),
	NAMEKEY2( F14 ),
	NAMEKEY2( F15 ),
	NAMEKEY2( KANA ),
	NAMEKEY2( CONVERT ),
	NAMEKEY2( NOCONVERT ),
	NAMEKEY2( YEN ),
	NAMEKEY( KP_EQUALS, "#str_07127" ),
	NAMEKEY2( CIRCUMFLEX ),
	NAMEKEY( AT, "@" ),
	NAMEKEY( COLON, ":" ),
	NAMEKEY( UNDERLINE, "_" ),
	NAMEKEY2( KANJI ),
	NAMEKEY2( STOP ),
	NAMEKEY2( AX ),
	NAMEKEY2( UNLABELED ),
	NAMEKEY( KP_ENTER, "#str_07119" ),
	NAMEKEY( RCTRL, "#str_bind_RCTRL" ),
	NAMEKEY( KP_COMMA, "," ),
	NAMEKEY( KP_SLASH, "#str_07122" ),
	NAMEKEY( PRINTSCREEN, "#str_07179" ),
	NAMEKEY( RALT, "#str_bind_RALT" ),
	NAMEKEY( PAUSE, "#str_07128" ),
	NAMEKEY( HOME, "#str_07052" ),
	NAMEKEY( UPARROW, "#str_07023" ),
	NAMEKEY( PGUP, "#str_07051" ),
	NAMEKEY( LEFTARROW, "#str_07025" ),
	NAMEKEY( RIGHTARROW, "#str_07026" ),
	NAMEKEY( END, "#str_07053" ),
	NAMEKEY( DOWNARROW, "#str_07024" ),
	NAMEKEY( PGDN, "#str_07050" ),
	NAMEKEY( INS, "#str_07048" ),
	NAMEKEY( DEL, "#str_07049" ),
	NAMEKEY( LWIN, "#str_07030" ),
	NAMEKEY( RWIN, "#str_07031" ),
	NAMEKEY( APPS, "#str_07032" ),
	NAMEKEY2( POWER ),
	NAMEKEY2( SLEEP ),

	// --

	NAMEKEY( MOUSE1, "#str_07054" ),
	NAMEKEY( MOUSE2, "#str_07055" ),
	NAMEKEY( MOUSE3, "#str_07056" ),
	NAMEKEY( MOUSE4, "#str_07057" ),
	NAMEKEY( MOUSE5, "#str_07058" ),
	NAMEKEY( MOUSE6, "#str_07059" ),
	NAMEKEY( MOUSE7, "#str_07060" ),
	NAMEKEY( MOUSE8, "#str_07061" ),

	NAMEKEY( MWHEELDOWN, "#str_07132" ),
	NAMEKEY( MWHEELUP, "#str_07131" ),

	NAMEKEY( JOY1, "#str_07062" ),
	NAMEKEY( JOY2, "#str_07063" ),
	NAMEKEY( JOY3, "#str_07064" ),
	NAMEKEY( JOY4, "#str_07065" ),
	NAMEKEY( JOY5, "#str_07066" ),
	NAMEKEY( JOY6, "#str_07067" ),
	NAMEKEY( JOY7, "#str_07068" ),
	NAMEKEY( JOY8, "#str_07069" ),
	NAMEKEY( JOY9, "#str_07070" ),
	NAMEKEY( JOY10, "#str_07071" ),
	NAMEKEY( JOY11, "#str_07072" ),
	NAMEKEY( JOY12, "#str_07073" ),
	NAMEKEY( JOY13, "#str_07074" ),
	NAMEKEY( JOY14, "#str_07075" ),
	NAMEKEY( JOY15, "#str_07076" ),
	NAMEKEY( JOY16, "#str_07077" ),

	NAMEKEY2( JOY_DPAD_UP ),
	NAMEKEY2( JOY_DPAD_DOWN ),
	NAMEKEY2( JOY_DPAD_LEFT ),
	NAMEKEY2( JOY_DPAD_RIGHT ),

	NAMEKEY2( JOY_STICK1_UP ),
	NAMEKEY2( JOY_STICK1_DOWN ),
	NAMEKEY2( JOY_STICK1_LEFT ),
	NAMEKEY2( JOY_STICK1_RIGHT ),

	NAMEKEY2( JOY_STICK2_UP ),
	NAMEKEY2( JOY_STICK2_DOWN ),
	NAMEKEY2( JOY_STICK2_LEFT ),
	NAMEKEY2( JOY_STICK2_RIGHT ),

	NAMEKEY2( JOY_TRIGGER1 ),
	NAMEKEY2( JOY_TRIGGER2 ),

	//------------------------
	// Aliases to make it easier to bind or to support old configs
	//------------------------
	ALIASKEY( "ALT", LALT ),
	ALIASKEY( "RIGHTALT", RALT ),
	ALIASKEY( "CTRL", LCTRL ),
	ALIASKEY( "SHIFT", LSHIFT ),
	ALIASKEY( "MENU", APPS ),
	ALIASKEY( "COMMAND", LALT ),

	ALIASKEY( "KP_HOME", KP_7 ),
	ALIASKEY( "KP_UPARROW", KP_8 ),
	ALIASKEY( "KP_PGUP", KP_9 ),
	ALIASKEY( "KP_LEFTARROW", KP_4 ),
	ALIASKEY( "KP_RIGHTARROW", KP_6 ),
	ALIASKEY( "KP_END", KP_1 ),
	ALIASKEY( "KP_DOWNARROW", KP_2 ),
	ALIASKEY( "KP_PGDN", KP_3 ),
	ALIASKEY( "KP_INS", KP_0 ),
	ALIASKEY( "KP_DEL", KP_DOT ),
	ALIASKEY( "KP_NUMLOCK", NUMLOCK ),

	ALIASKEY( "-", MINUS ),
	ALIASKEY( "=", EQUALS ),
	ALIASKEY( "[", LBRACKET ),
	ALIASKEY( "]", RBRACKET ),
	ALIASKEY( "\\", BACKSLASH ),
	ALIASKEY( "/", SLASH ),
	ALIASKEY( ",", COMMA ),
	ALIASKEY( ".", PERIOD ),

	{K_NONE, NULL, NULL}
};

class idKey
{
public:
    idKey() {down = false; repeats = 0; usercmdAction = 0;}

    bool down;
    int repeats;
    std::string binding;
    int usercmdAction;
};

bool key_overstrikeMode = false;
idKey* keys = NULL;

void Key_Unbindall_f(const idCmdArgs& args)
{
    for (int i = 0; i < K_LAST_KEY; i++)
        idKeyInput::SetBinding(i, "");
}

void idKeyInput::Init()
{
    keys = new idKey[K_LAST_KEY];

    cmdSystem->AddCommand("unbindall", Key_Unbindall_f, CMD_FL_SYSTEM, "Unbind all keys from their commands");
}

void idKeyInput::SetBinding(int keynum, const char* binding)
{
    if (keynum == -1)
        return;
    
    keys[keynum].binding = "";

    cvarSystem->SetModifiedFlags(CVAR_ARCHIVE);
}
