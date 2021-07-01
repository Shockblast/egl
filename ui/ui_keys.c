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

#include "ui_local.h"
#include <ctype.h>

const char	*(*UI_KeyFunc) (int key);

static qBool	ui_LastClick = qFalse;
static int		ui_LastClickTime = 0;

/*
=======================================================================

	KEY HANDLING

=======================================================================
*/

/*
=================
UI_Keydown
=================
*/
void UI_Keydown (int key) {
	const char *s;

	if (UI_KeyFunc) {
		if ((s = UI_KeyFunc (key)) != 0)
			uii.Snd_StartLocalSound ((char *) s);
	}
}


/*
=============
DefaultMenu_Key
=============
*/
const char *DefaultMenu_Key (uiFrameWork_t *m, int key) {
	const char		*sound = NULL;
	uiCommon_t		*item = NULL;

	if (m && ((item = UI_ItemAtCursor (m)) != NULL)) {
		if (item->type == UITYPE_FIELD) {
			if (Field_Key ((uiField_t *) item, key))
				return NULL;
		}
	}

	switch (key) {
	case K_ESCAPE:
		UI_PopMenu ();
		return ui_OutSFX;

	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		if (m) {
			m->cursor--;
			UI_AdjustTextCursor (m, -1);
			sound = ui_MoveSFX;
		}
		break;

	case K_TAB:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if (m) {
			m->cursor++;
			UI_AdjustTextCursor (m, 1);
			sound = ui_MoveSFX;
		}
		break;

	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_MOUSE4:
		if (m) {
			UI_SlideItem (m, -1);
			sound = ui_MoveSFX;
		}
		break;

	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
	case K_MOUSE5:
		if (m) {
			UI_SlideItem (m, 1);
			sound = ui_MoveSFX;
		}
		break;

	case K_MOUSE1:
		if (m && item && ui_MouseItem && (ui_MouseItem == item)) {
			if (item->flags & UIF_SELONLY) {
				if (ui_SelItem && (ui_SelItem == item))
					ui_SelItem = NULL;
				else
					ui_SelItem = item;
				sound = ui_InSFX;

				if (!(item->flags & UIF_DBLCLICK))
					break;
			}

			if (item->flags & UIF_DBLCLICK) {
				if (ui_LastClick && ((ui_LastClickTime + 850) > uis.time)) {
					ui_LastClickTime = uis.time;
					ui_LastClick = qFalse;
				} else {
					if (!ui_LastClick)
						ui_LastClick = qTrue;

					ui_LastClickTime = uis.time;
					sound = ui_MoveSFX;
					break;
				}
			}

			if (UI_SlideItem (m, 1))
				sound = ui_MoveSFX;
			else if (UI_SelectItem (m))
				sound = ui_InSFX;
		}
		break;

	case K_MOUSE2:
		if (m && item && ui_MouseItem && (ui_MouseItem == item)) {
			if (item->flags & UIF_SELONLY) {
				if (ui_SelItem && (ui_SelItem == item))
					ui_SelItem = NULL;
				else
					ui_SelItem = item;
				sound = ui_InSFX;

				if (!(item->flags & UIF_DBLCLICK))
					break;
			}

			if (item->flags & UIF_DBLCLICK) {
				if (ui_LastClick && ((ui_LastClickTime + 850) > uis.time)) {
					ui_LastClickTime = uis.time;
					ui_LastClick = qFalse;
				} else {
					if (!ui_LastClick)
						ui_LastClick = qTrue;

					ui_LastClickTime = uis.time;
					sound = ui_MoveSFX;
					break;
				}
			}

			if (UI_SlideItem (m, -1))
				sound = ui_MoveSFX;
			else if (UI_SelectItem (m))
				sound = ui_InSFX;
		}
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
		if (m && UI_SelectItem (m))
			sound = ui_MoveSFX;
		break;
	}

	return sound;
}


/*
=============
Field_Key
=============
*/
qBool Field_Key (uiField_t *f, int key) {
	if (key > 127)
		return qFalse;

	switch (key) {
	case K_KP_SLASH:		key = '/';		break;
	case K_KP_MINUS:		key = '-';		break;
	case K_KP_PLUS:			key = '+';		break;
	case K_KP_HOME:			key = '7';		break;
	case K_KP_UPARROW:		key = '8';		break;
	case K_KP_PGUP:			key = '9';		break;
	case K_KP_LEFTARROW:	key = '4';		break;
	case K_KP_FIVE:			key = '5';		break;
	case K_KP_RIGHTARROW:	key = '6';		break;
	case K_KP_END:			key = '1';		break;
	case K_KP_DOWNARROW:	key = '2';		break;
	case K_KP_PGDN:			key = '3';		break;
	case K_KP_INS:			key = '0';		break;
	case K_KP_DEL:			key = '.';		break;
	}

	/*
	** support pasting from the clipboard
	*/
	if ((toupper(key) == 'V' && uii.key_Down[K_CTRL]) || (((key == K_INS) || (key == K_KP_INS)) && KEY_SHIFTDOWN (uii.key_Down))) {
		char *cbd;
		
		if ((cbd = uii.Sys_GetClipboardData()) != 0) {
			strtok (cbd, "\n\r\b");

			strncpy (f->buffer, cbd, f->length - 1);
			f->cursor = strlen (f->buffer);
			f->visible_offset = (f->cursor - f->visible_length);
			if (f->visible_offset < 0)
				f->visible_offset = 0;

			uii.free (cbd);
		}
		return qTrue;
	}

	switch (key) {
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (f->cursor > 0) {
			memmove (&f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen (&f->buffer[f->cursor]) + 1);
			f->cursor--;

			if (f->visible_offset)
				f->visible_offset--;
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
		if (!isdigit(key) && (f->generic.flags & UIF_NUMBERSONLY))
			return qFalse;

		if (f->cursor < f->length) {
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visible_length)
				f->visible_offset++;
		}
	}

	return qTrue;
}
