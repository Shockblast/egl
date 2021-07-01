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

enum {
	K_BADKEY		=	-1,

	K_TAB			=	9,
	K_ENTER			=	13,
	K_ESCAPE		=	27,
	K_SPACE			=	32,

	/* normal keys should be passed as lowercased ascii */
	K_BACKSPACE		=	127,
	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,

	K_SHIFT,
	K_LSHIFT,
	K_RSHIFT,

	K_CAPSLOCK,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_KP_HOME		=	160,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_FIVE,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,

	/* mouse buttons generate virtual keys */
	K_MOUSE1	=		200,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	/* joystick buttons */
	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,

	/*
	** aux keys are for multi-buttoned joysticks to generate so that
	** they can use the normal binding process
	*/
	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32,

	K_MWHEELDOWN,
	K_MWHEELUP,
	K_MWHEELLEFT,
	K_MWHEELRIGHT,

	K_PAUSE			=	255,

	K_MAXKEYS
};

enum {
	KD_MINDEST		=	0,

	KD_GAME			=	0,
	KD_CONSOLE		=	1,
	KD_MESSAGE		=	2,
	KD_MENU			=	3,

	KD_MAXDEST		=	3
};

#define	MAXCMDLINE	256

#define KEY_SHIFTDOWN(v)	((v)[K_SHIFT] || (v)[K_LSHIFT] || (v)[K_RSHIFT])

extern char		key_Buffer[32][MAXCMDLINE];
extern int		key_LinePos;
extern int		key_EditLine;

extern qBool	key_ShiftDown;
extern qBool	key_CapslockOn;
extern qBool	key_InsertOn;

extern int		key_AnyDown;

extern char		*key_Bindings[K_MAXKEYS];
extern qBool	key_Down[K_MAXKEYS];

extern qBool	chat_Team;
extern char		chat_Buffer[32][MAXCMDLINE];
extern int		chat_LinePos;
extern int		chat_EditLine;

qBool	Key_KeyDest (int keyDest);
void	Key_SetKeyDest (int keyDest);

char	*Key_KeynumToString (int keyNum);
void	Key_Event (int keyNum, qBool isDown, unsigned int time);
void	Key_ClearStates (void);
void	Key_ClearTyping (void);
int		Key_GetKey (void);

void	Key_SetBinding (int keyNum, char *binding);
void	Key_WriteBindings (FILE *f);

void	Key_Init (void);
void	Key_Shutdown (void);
