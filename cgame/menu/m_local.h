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
// m_local.h
//

#include "../cg_local.h"

/*
=============================================================================

	MENU STATE

=============================================================================
*/

typedef struct menuState_s {
	// Sound toggles
	qBool			playEnterSound;
	qBool			playExitSound;
} menuState_t;

extern menuState_t	menuState;

/*
=============================================================================

	LOCAL FUNCTIONS

=============================================================================
*/

void		M_PushMenu (uiFrameWork_t *frameWork, void (*drawFunc) (void), struct sfx_s *(*closeFunc) (void), struct sfx_s *(*keyFunc) (uiFrameWork_t *fw, keyNum_t keyNum));

struct sfx_s *M_KeyHandler (uiFrameWork_t *fw, keyNum_t keyNum);

/*
=============================================================================

	COMMON CALLBACK FUNCTIONS

=============================================================================
*/

/*
=============
Cursor_NullFunc
=============
*/
static inline void Cursor_NullFunc (void *self)
{
}


/*
=============
Menu_Pop
=============
*/
static inline void Menu_Pop (void *self)
{
	M_PopMenu ();
}
