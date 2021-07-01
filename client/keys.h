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
// keys.h
//

#include "../ui/keycodes.h"

#define	MAXCMDLINE	256

extern qBool	key_insertOn;

extern int		key_anyKeyDown;

extern char		key_consoleBuffer[32][MAXCMDLINE];
extern int		key_consoleLinePos;
extern int		key_consoleEditLine;

extern qBool	key_chatTeam;
extern char		key_chatBuffer[32][MAXCMDLINE];
extern int		key_chatLinePos;
extern int		key_chatEditLine;

qBool	Key_KeyDest (int keyDest);
void	Key_SetKeyDest (int keyDest);

char	*Key_KeynumToString (int keyNum);
void	Key_Event (int keyNum, qBool isDown, uInt time);
void	Key_ClearStates (void);
void	Key_ClearTyping (void);
int		Key_GetKey (void);

void	Key_SetBinding (int keyNum, char *binding);
void	Key_WriteBindings (FILE *f);

void	Key_Init (void);
