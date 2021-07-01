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
// cl_keys.h
//

#define MAXCMDLINE	256

// FIXME: this is only here for ../linux/x11_utils.c
typedef struct keyInfo_s {
        char    *bind;                  // action the key is bound to
        qBool   down;                   // key up events are sent even if in console mode
        int             repeated;               // if > 1, it is autorepeating
        qBool   console;                // if qTrue, can't be typed while in the console
        int             shiftValue;             // key to map to if shift held down when this key is pressed
} keyInfo_t;

extern qBool	key_insertOn;

extern int		key_anyKeyDown;

extern char		key_consoleBuffer[32][MAXCMDLINE];
extern int		key_consoleLinePos;
extern int		key_consoleEditLine;

extern qBool	key_chatTeam;
extern char		key_chatBuffer[32][MAXCMDLINE];
extern int		key_chatLinePos;
extern int		key_chatEditLine;

keyDest_t	Key_GetDest (void);
void		Key_SetDest (keyDest_t keyDest);

char		*Key_KeynumToString (int keyNum);
qBool		Key_InsertOn (void);
qBool		Key_CapslockOn (void);
qBool		Key_ShiftDown (void);
void		Key_Event (int keyNum, qBool isDown, uint32 time);
void		Key_ClearStates (void);
void		Key_ClearTyping (void);

void		Key_SetBinding (int keyNum, char *binding);
void		Key_WriteBindings (FILE *f);

void		Key_Init (void);
