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
// ui_keys.c
//

#include "ui_local.h"

static qBool	ui_lastClick = qFalse;
static int		ui_lastClickTime = 0;

/*
=======================================================================

	KEY HANDLING

=======================================================================
*/

/*
=================
UI_KeyDown
=================
*/
void UI_KeyDown (int keyNum, qBool isDown)
{
	struct sfx_s *snd;

	if (!isDown)
		return;

	if (uiState.keyFunc) {
		// Use interface-specific key handler
		snd = uiState.keyFunc (uiState.activeUI, keyNum);
	}
	else if (uiState.activeUI) {
		// Default key handler
		snd = UI_DefaultKeyFunc (uiState.activeUI, keyNum);
	}
	else {
		return;
	}

	// Play desired key-specific sound
	if (snd)
		cgi.Snd_StartLocalSound (snd, 1);
}


/*
=============
UI_FieldKeyFunc
=============
*/
static qBool UI_FieldKeyFunc (uiField_t *f, int keyNum)
{
	if (keyNum > 127)
		return qFalse;

	switch (keyNum) {
	case K_KP_SLASH:		keyNum = '/';	break;
	case K_KP_MINUS:		keyNum = '-';	break;
	case K_KP_PLUS:			keyNum = '+';	break;
	case K_KP_HOME:			keyNum = '7';	break;
	case K_KP_UPARROW:		keyNum = '8';	break;
	case K_KP_PGUP:			keyNum = '9';	break;
	case K_KP_LEFTARROW:	keyNum = '4';	break;
	case K_KP_FIVE:			keyNum = '5';	break;
	case K_KP_RIGHTARROW:	keyNum = '6';	break;
	case K_KP_END:			keyNum = '1';	break;
	case K_KP_DOWNARROW:	keyNum = '2';	break;
	case K_KP_PGDN:			keyNum = '3';	break;
	case K_KP_INS:			keyNum = '0';	break;
	case K_KP_DEL:			keyNum = '.';	break;
	}

	//
	// support pasting from the clipboard
	//
	if ((toupper (keyNum) == 'V' && cgi.Key_IsDown (K_CTRL)) ||
		((keyNum == K_INS || keyNum == K_KP_INS) && (cgi.Key_IsDown (K_SHIFT) || cgi.Key_IsDown (K_LSHIFT) || cgi.Key_IsDown (K_RSHIFT)))) {
		char *cbd;
		
		if ((cbd = cgi.Sys_GetClipboardData()) != 0) {
			strtok (cbd, "\n\r\b");

			Q_strncpyz (f->buffer, cbd, f->length - 1);
			f->cursor = (int)strlen (f->buffer);
			f->visibleOffset = (f->cursor - f->visibleLength);
			if (f->visibleOffset < 0)
				f->visibleOffset = 0;

			CG_MemFree (cbd);
		}
		return qTrue;
	}

	switch (keyNum) {
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (f->cursor > 0) {
			memmove (&f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen (&f->buffer[f->cursor]) + 1);
			f->cursor--;

			if (f->visibleOffset)
				f->visibleOffset--;
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove (&f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen (&f->buffer[f->cursor+1]) + 1);
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return qFalse;

	default:
		if (f->generic.flags & UIF_NUMBERSONLY && !isdigit (keyNum))
			return qFalse;

		if (f->cursor < f->length) {
			f->buffer[f->cursor++] = keyNum;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visibleLength)
				f->visibleOffset++;
		}
	}

	return qTrue;
}


/*
=============
UI_DefaultKeyFunc
=============
*/
struct sfx_s *UI_DefaultKeyFunc (uiFrameWork_t *fw, int keyNum)
{
	uiCommon_t		*item;

	if (!fw)
		return NULL;

	// Handle field items special
	item = UI_ItemAtCursor (fw);
	if (item && item->type == UITYPE_FIELD) {
		if (UI_FieldKeyFunc ((uiField_t *) item, keyNum))
			return NULL;
	}

	switch (keyNum) {
	case K_ESCAPE:
		if (uiState.activeUI->flags & FWF_INTERFACE)
			UI_PopInterface ();
		else
			M_PopMenu ();
		return NULL;

	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		fw->cursor--;
		UI_AdjustCursor (fw, -1);
		return uiMedia.sounds.menuMove;

	case K_TAB:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		fw->cursor++;
		UI_AdjustCursor (fw, 1);
		return uiMedia.sounds.menuMove;

	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_MOUSE4:
		UI_SlideItem (item, -1);
		return uiMedia.sounds.menuMove;

	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
	case K_MOUSE5:
		UI_SlideItem (item, 1);
		return uiMedia.sounds.menuMove;

	case K_MOUSE1:
		if (!item || !uiState.mouseItem || uiState.mouseItem != item)
			break;

		if (item->flags & UIF_SELONLY) {
			if (uiState.selectedItem && uiState.selectedItem == item)
				uiState.selectedItem = NULL;
			else
				uiState.selectedItem = item;

			if (!(item->flags & UIF_DBLCLICK))
				return uiMedia.sounds.menuMove;
		}

		if (item->flags & UIF_DBLCLICK) {
			if (ui_lastClick && ui_lastClickTime + 850 > cg.realTime) {
				ui_lastClickTime = cg.realTime;
				ui_lastClick = qFalse;
			}
			else {
				if (!ui_lastClick)
					ui_lastClick = qTrue;

				ui_lastClickTime = cg.realTime;
				return uiMedia.sounds.menuMove;
			}
		}

		if (!UI_SlideItem (item, 1))
			UI_SelectItem (item);
		else
			return uiMedia.sounds.menuMove;
		break;

	case K_MOUSE2:
		if (!item || !uiState.mouseItem || uiState.mouseItem != item)
			break;

		if (item->flags & UIF_SELONLY) {
			if (uiState.selectedItem && uiState.selectedItem == item)
				uiState.selectedItem = NULL;
			else
				uiState.selectedItem = item;

			if (!(item->flags & UIF_DBLCLICK))
				return uiMedia.sounds.menuMove;
		}

		if (item->flags & UIF_DBLCLICK) {
			if (ui_lastClick && ui_lastClickTime + 850 > cg.realTime) {
				ui_lastClickTime = cg.realTime;
				ui_lastClick = qFalse;
			}
			else {
				if (!ui_lastClick)
					ui_lastClick = qTrue;

				ui_lastClickTime = cg.realTime;
				return uiMedia.sounds.menuMove;
			}
		}

		if (!UI_SlideItem (item, -1))
			UI_SelectItem (item);
		else
			return uiMedia.sounds.menuMove;
		break;

	case K_JOY1:	case K_JOY2:	case K_JOY3:	case K_JOY4:
	case K_AUX1:	case K_AUX2:	case K_AUX3:	case K_AUX4:	case K_AUX5:	case K_AUX6:
	case K_AUX7:	case K_AUX8:	case K_AUX9:	case K_AUX10:	case K_AUX11:	case K_AUX12:
	case K_AUX13:	case K_AUX14:	case K_AUX15:	case K_AUX16:	case K_AUX17:	case K_AUX18:
	case K_AUX19:	case K_AUX20:	case K_AUX21:	case K_AUX22:	case K_AUX23:	case K_AUX24:
	case K_AUX25:	case K_AUX26:	case K_AUX27:	case K_AUX28:	case K_AUX29:	case K_AUX30:
	case K_AUX31:	case K_AUX32:

	case K_ENTER:
	case K_KP_ENTER:
	case K_MOUSE3:
		UI_SelectItem (item);
		break;
	}

	return NULL;
}
