/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// key_win.c
//

#include "../client/cl_local.h"
#include "winquake.h"

/*
=================
Key_MapKey

Map from windows to quake keynums
=================
*/
byte scanToKey[128] = {
	0,			K_ESCAPE,	'1',		'2',		'3',		'4',		'5',		'6',
	'7',		'8',		'9',		'0',		'-',		'=',		K_BACKSPACE,9,		// 0
	'q',		'w',		'e',		'r',		't',		'y',		'u',		'i',
	'o',		'p',		'[',		']',		K_ENTER,	K_CTRL,		'a',		's',	// 1
	'd',		'f',		'g',		'h',		'j',		'k',		'l',		';',
	'\'',		'`',		K_LSHIFT,	'\\',		'z',		'x',		'c',		'v',	// 2
	'b',		'n',		'm',		',',		'.',		'/',		K_RSHIFT,	'*',
	K_ALT,		' ',		K_CAPSLOCK,	K_F1,		K_F2,		K_F3,		K_F4,		K_F5,	// 3
	K_F6,		K_F7,		K_F8,		K_F9,		K_F10,		K_PAUSE,	0,			K_HOME,
	K_UPARROW,	K_PGUP,		K_KP_MINUS,	K_LEFTARROW,K_KP_FIVE,	K_RIGHTARROW,K_KP_PLUS,	K_END,	// 4
	K_DOWNARROW,K_PGDN,		K_INS,		K_DEL,		0,			0,			0,			K_F11,
	K_F12,		0,			0,			0,			0,			0,			0,			0,		// 5
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// 6
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0		// 7
};

int Key_MapKey (int key) {
	int		result;
	int		modified;
	qBool	isExtended;

	modified = (key >> 16) & 255;
	if (modified > 127)
		return 0;

	if (key & (1 << 24))
		isExtended = qTrue;
	else
		isExtended = qFalse;

	result = scanToKey[modified];

	if (!isExtended) {
		switch (result) {
		case K_HOME:		return K_KP_HOME;
		case K_UPARROW:		return K_KP_UPARROW;
		case K_PGUP:		return K_KP_PGUP;
		case K_LEFTARROW:	return K_KP_LEFTARROW;
		case K_RIGHTARROW:	return K_KP_RIGHTARROW;
		case K_END:			return K_KP_END;
		case K_DOWNARROW:	return K_KP_DOWNARROW;
		case K_PGDN:		return K_KP_PGDN;
		case K_INS:			return K_KP_INS;
		case K_DEL:			return K_KP_DEL;
		default:			return result;
		}
	} else {
		switch (result) {
		case 0x0D:	return K_KP_ENTER;
		case 0x2F:	return K_KP_SLASH;
		case 0xAF:	return K_KP_PLUS;
		}
		return result;
	}
}


/*
=================
Key_CapslockState
=================
*/
qBool Key_CapslockState (void) {
	return (!!(GetKeyState (VK_CAPITAL)));
}
