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
// gui_init.c
//

#include "gui_local.h"

static gui_t			*cl_guiHashTree[MAX_GUI_HASH];
static gui_t			cl_guiList[MAX_GUIS];
static guiPathType_t	cl_guiPathTypes[MAX_GUIS];
static guiCursor_t		cl_guiCursors[MAX_GUIS];
static uint32			cl_numGUI;

static uint32			cl_numGUIErrors;
static uint32			cl_numGUIWarnings;

static uint32			cl_guiRegFrames[MAX_GUIS];
static uint32			cl_guiRegTouched[MAX_GUIS];
static uint32			cl_guiRegFrameNum;

cVar_t	*gui_developer;
cVar_t	*gui_debugBounds;
cVar_t	*gui_mouseFilter;
cVar_t	*gui_mouseSensitivity;

/*
================
GUI_HashValue

String passed to this should be lowercase.
================
*/
static uint32 GUI_HashValue (const char *name)
{
	uint32	hashValue;
	int		ch, i;

	for (i=0, hashValue=0 ; *name ; i++) {
		ch = *(name++);
		hashValue = hashValue * 33 + ch;
	}

	return (hashValue + (hashValue >> 5)) & (MAX_GUI_HASH-1);
}


/*
================
GUI_HashValue
================
*/
static gui_t *GUI_FindGUI (char *name)
{
	gui_t	*gui, *bestMatch;
	char	tempName[MAX_QPATH];
	uint32	hashValue;

	// Make sure it's lowercase
	Q_strncpyz (tempName, name, sizeof (tempName));
	Q_strlwr (tempName);

	hashValue = GUI_HashValue (tempName);
	bestMatch = NULL;

	// Look for it
	for (gui=cl_guiHashTree[hashValue] ; gui ; gui=gui->hashNext) {
		if (strcmp (gui->name, tempName))
			continue;

		if (!bestMatch || cl_guiPathTypes[gui-&cl_guiList[0]] >= cl_guiPathTypes[bestMatch-&cl_guiList[0]])
			bestMatch = gui;
	}

	return bestMatch;
}


/*
===============
GUI_ColorNormalize
===============
*/
static float GUI_ColorNormalize (const float *in, float *out)
{
	float	f = max (max (in[0], in[1]), in[2]);

	if (f > 1.0f) {
		f = 1.0f / f;
		out[0] = in[0] * f;
		out[1] = in[1] * f;
		out[2] = in[2] * f;
	}
	else {
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
	}

	return f;
}

/*
=============================================================================

	PARSE HELPERS

=============================================================================
*/

/*
================
GUI_ParseBool

Set require to qTrue when you require that there is a valid value. If
it is set to false, it won't require a value but will require that it
is clamped to 0 or 1.
================
*/
static qBool GUI_ParseBool (parse_t *ps, qBool require, qBool *target)
{
	char	*token;
	qBool	tmp;

	token = Com_ParseExt (ps, qFalse);
	if (!token[0])
		return !require;

	tmp = atoi (token);
	if (tmp < 0 || tmp > 1)
		return qFalse;

	*target = tmp;
	return qTrue;
}


/*
================
GUI_ParseFloat
================
*/
static qBool GUI_ParseFloat (parse_t *ps, float *target)
{
	char	*token;

	token = Com_ParseExt (ps, qFalse);
	if (!token[0])
		return qFalse;

	*target = (float)atof (token);
	return qTrue;
}


/*
================
GUI_ParseFloatVec

FIXME: allow for "(#, #, #, #)" etc
================
*/
qBool GUI_ParseFloatVec (parse_t *ps, float *target, uint32 size)
{
	char	*token;
	uint32	i;

	for (i=0 ; i<size ; i++) {
		token = Com_ParseExt (ps, qFalse);
		if (!token[0])
			return qFalse;
		target[i] = (float)atof (token);
	}

	return qTrue;
}


/*
================
GUI_ParseInt
================
*/
static qBool GUI_ParseInt (parse_t *ps, int *target)
{
	char	*token;

	token = Com_ParseExt (ps, qFalse);
	if (!token[0])
		return qFalse;

	*target = atoi (token);
	return qTrue;
}


/*
================
GUI_ParseString
================
*/
static qBool GUI_ParseString (parse_t *ps, qBool allowNewLines, char **target)
{
	char	*token;

	token = Com_ParseExt (ps, allowNewLines);
	if (!token[0])
		return qFalse;

	*target = Q_strlwr (token);
	return qTrue;
}


/*
================
GUI_SkipLine
================
*/
static void GUI_SkipLine (parse_t *ps)
{
	char	*token;

	for ( ; ; ) {
		token = Com_ParseExt (ps, qFalse);
		if (!token[0])
			break;
	}
}


/*
==================
GUI_PrintLocation
==================
*/
static void GUI_PrintLocation (comPrint_t flags, parse_t *ps, char *fileName, gui_t *gui)
{
	if (ps) {
		if (gui) {
			Com_Printf (flags, "%s(%i) : window '%s':\n", fileName, ps->currentLine, gui->name);
			return;
		}

		Com_Printf (flags, "%s(%i):\n", fileName, ps->currentLine);
		return;
	}

	Com_Printf (flags, "%s:", fileName);
}


/*
==================
GUI_PrintError
==================
*/
static void GUI_PrintError (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	cl_numGUIErrors++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (PRNT_ERROR, msg);
}


/*
==================
GUI_PrintWarning
==================
*/
static void GUI_PrintWarning (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	cl_numGUIWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (PRNT_WARNING, msg);
}


/*
==================
GUI_DevPrintf
==================
*/
static void GUI_DevPrintf (comPrint_t flags, parse_t *ps, char *fileName, gui_t *gui, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!gui_developer->integer && !developer->integer)
		return;

	if (flags & (PRNT_ERROR|PRNT_WARNING)) {
		if (flags & PRNT_ERROR)
			cl_numGUIErrors++;
		else if (flags & PRNT_WARNING)
			cl_numGUIWarnings++;
		GUI_PrintLocation (flags, ps, fileName, gui);
	}

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}

/*
=============================================================================

	KEY->FUNC PARSING

=============================================================================
*/

typedef struct guiParseKey_s {
	const char		*keyWord;
	qBool			(*func) (char *fileName, gui_t *gui, parse_t *ps, char *keyName);
} guiParseKey_t;

/*
==================
GUI_CallKeyFunc
==================
*/
static qBool GUI_CallKeyFunc (char *fileName, gui_t *gui, parse_t *ps, guiParseKey_t *keyList1, guiParseKey_t *keyList2, guiParseKey_t *keyList3, char *token)
{
	guiParseKey_t	*list, *key;
	char			keyName[MAX_TOKEN_CHARS];
	char			*str;

	// Copy off a lower-case copy for faster comparisons
	Q_strncpyz (keyName, token, sizeof (keyName));
	Q_strlwr (keyName);

	// Cycle through the key lists looking for a match
	for (list=keyList1 ; list ; ) {
		for (key=&list[0] ; key->keyWord ; key++) {
			// See if it matches the keyWord
			if (strcmp (key->keyWord, keyName))
				continue;

			// This is just to ignore any warnings
			if (!key->func) {
				GUI_SkipLine (ps);
				return qTrue;
			}

			// Failed to parse line
			if (!key->func (fileName, gui, ps, keyName)) {
				GUI_SkipLine (ps);
				return qFalse;
			}

			// Report any extra parameters
			if (GUI_ParseString (ps, qFalse, &str)) {
				GUI_PrintLocation (PRNT_WARNING, ps, fileName, gui);
				GUI_PrintWarning ("WARNING: unused trailing parameters after key: '%s'\n", keyName);
				GUI_SkipLine (ps);
				return qTrue;
			}

			// Parsed fine
			return qTrue;
		}

		// Next list
		if (list == keyList1)
			list = keyList2;
		else if (list == keyList2)
			list = keyList3;
		else
			break;
	}

	GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
	GUI_PrintError ("ERROR: unrecognized key: '%s'\n", keyName);
	return qFalse;
}

/*
=============================================================================

	ITEMDEF PARSING

	The windowDef and itemDef's below can utilize all of these keys.
=============================================================================
*/

/*
==================
itemDef_rect
==================
*/
static qBool itemDef_rect (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec4_t	rect;

	if (!GUI_ParseFloatVec (ps, rect, 4)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Vector4Copy (rect, gui->s.rect);
	return qTrue;
}


/*
==================
itemDef_rotate
==================
*/
static qBool itemDef_rotate (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	float	rotate;

	if (!GUI_ParseFloat (ps, &rotate)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->s.rotation = rotate;
	return qTrue;
}


/*
==================
itemDef_shear
==================
*/
static qBool itemDef_shear (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec2_t	shear;

	if (!GUI_ParseFloatVec (ps, shear, 2)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Vector2Copy (shear, gui->s.shear);
	return qTrue;
}

// ==========================================================================

/*
==================
itemDef_fill
==================
*/
static qBool itemDef_fill (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec4_t	fillColor;

	if (!GUI_ParseFloatVec (ps, fillColor, 4)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	GUI_ColorNormalize (fillColor, gui->s.fillColor);
	gui->s.fillColor[3] = fillColor[3];
	gui->flags |= WFL_FILL_COLOR;
	return qTrue;
}


/*
==================
itemDef_mat
==================
*/
static qBool itemDef_mat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!GUI_ParseString (ps, qFalse, &str)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Q_strncpyz (gui->s.matName, str, sizeof (gui->s.matName));
	gui->flags |= WFL_MATERIAL;
	return qTrue;
}


/*
==================
itemDef_matColor
==================
*/
static qBool itemDef_matColor (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec4_t	color;

	if (!GUI_ParseFloatVec (ps, color, 4)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	GUI_ColorNormalize (color, gui->s.matColor);
	gui->s.matColor[3] = color[3];
	return qTrue;
}


/*
==================
itemDef_matScaleX
==================
*/
static qBool itemDef_matScaleX (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	float	xScale;

	if (!GUI_ParseFloat (ps, &xScale)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->s.matScaleX = xScale;
	return qTrue;
}


/*
==================
itemDef_matScaleY
==================
*/
static qBool itemDef_matScaleY (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	float	yScale;

	if (!GUI_ParseFloat (ps, &yScale)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->s.matScaleY = yScale;
	return qTrue;
}

// ==========================================================================

/*
==================
itemDef_modal
==================
*/
static qBool itemDef_modal (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	modal;

	if (!GUI_ParseBool (ps, qFalse, &modal)) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		modal = qTrue;
	}

	gui->s.modal = modal;
	return qTrue;
}


/*
==================
itemDef_noEvents
==================
*/
static qBool itemDef_noEvents (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	noEvents;

	if (!GUI_ParseBool (ps, qFalse, &noEvents)) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		noEvents = qTrue;
	}

	gui->s.noEvents = noEvents;
	return qTrue;
}


/*
==================
itemDef_noTime
==================
*/
static qBool itemDef_noTime (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	noTime;

	if (!GUI_ParseBool (ps, qFalse, &noTime)) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		noTime = qTrue;
	}

	gui->s.noTime = noTime;
	return qTrue;
}


/*
==================
itemDef_visible
==================
*/
static qBool itemDef_visible (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	visible;

	if (!GUI_ParseBool (ps, qFalse, &visible)) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		visible = qTrue;
	}

	gui->s.visible = visible;
	return qTrue;
}


/*
==================
itemDef_wantEnter
==================
*/
static qBool itemDef_wantEnter (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	wantEnter;

	if (!GUI_ParseBool (ps, qFalse, &wantEnter)) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		wantEnter = qTrue;
	}

	gui->s.wantEnter = wantEnter;
	return qTrue;
}

// ==========================================================================

static guiParseKey_t	cl_itemDefKeyList[] = {
	// Orientation
	{ "rect",					&itemDef_rect				},
	{ "rotate",					&itemDef_rotate				},
	{ "shear",					&itemDef_shear				},

	// Background
	{ "fill",					&itemDef_fill				},
	{ "mat",					&itemDef_mat				},
	{ "matcolor",				&itemDef_matColor			},
	{ "matscalex",				&itemDef_matScaleX			},
	{ "matscaley",				&itemDef_matScaleY			},

	// Flags
	{ "modal",					&itemDef_modal				},
	{ "noevents",				&itemDef_noEvents			},
	{ "notime",					&itemDef_noTime				},
	{ "visible",				&itemDef_visible			},
	{ "wantenter",				&itemDef_wantEnter			},

	{ NULL,						NULL						}
};

/*
=============================================================================

	BINDDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_bindDefKeyList[] = {
	{ "bind",					NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	CHOICEDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_choiceDefKeyList[] = {
	{ "liveupdate",				NULL						},
	{ "choices",				NULL						},
	{ "choicetype",				NULL						},
	{ "currentchoice",			NULL						},
	{ "values",					NULL						},
	{ "cvar",					NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	EDITDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_editDefKeyList[] = {
	{ "liveupdate",				NULL						},
	{ "maxchars",				NULL						},
	{ "numeric",				NULL						},
	{ "wrap",					NULL						},
	{ "readonly",				NULL						},
	{ "source",					NULL						},
	{ "password",				NULL						},
	{ "cvar",					NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	LISTDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_listDefKeyList[] = {
	{ "listname",				NULL						},
	{ "scrollbar",				NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	RENDERDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_renderDefKeyList[] = {
	{ "lightcolor",				NULL						},
	{ "lightorigin",			NULL						},
	{ "model",					NULL						},
	{ "modelcolor",				NULL						},
	{ "modelrotate",			NULL						},
	{ "vieworigin",				NULL						},
	{ "viewangles",				NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	SLIDERDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_sliderDefKeyList[] = {
	{ "liveupdate",				NULL						},
	{ "low",					NULL						},
	{ "high",					NULL						},
	{ "step",					NULL						},
	{ "vertical",				NULL						},
	{ "thumbshader",			NULL						},
	{ "cvar",					NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	TEXTDEF PARSING

=============================================================================
*/

// ==========================================================================

static guiParseKey_t	cl_textDefKeyList[] = {
	{ "font",					NULL						},
	{ "text",					NULL						},
	{ "textalign",				NULL						},
	{ "textcolor",				NULL						},
	{ "textscale",				NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	GUIDEF PARSING

=============================================================================
*/

/*
==================
guiDef_cursorMat
==================
*/
static qBool guiDef_cursorMat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!GUI_ParseString (ps, qFalse, &str)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Q_strncpyz (gui->cursor->s.name, str, sizeof (gui->cursor->s.name));
	gui->cursor->s.disabled = qFalse;
	gui->flags |= WFL_CURSOR;
	return qTrue;
}


/*
==================
guiDef_cursorColor
==================
*/
static qBool guiDef_cursorColor (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec4_t	color;

	if (!GUI_ParseFloatVec (ps, color, 4)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	GUI_ColorNormalize (color, gui->cursor->s.color);
	gui->cursor->s.color[3] = color[3];
	return qTrue;
}


/*
==================
guiDef_cursorHeight
==================
*/
static qBool guiDef_cursorHeight (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	float	height;

	if (!GUI_ParseFloat (ps, &height)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->cursor->s.size[1] = height;
	return qTrue;
}


/*
==================
guiDef_cursorWidth
==================
*/
static qBool guiDef_cursorWidth (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	float	width;

	if (!GUI_ParseFloat (ps, &width)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->cursor->s.size[0] = width;
	return qTrue;
}


/*
==================
guiDef_cursorPos
==================
*/
static qBool guiDef_cursorPos (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	vec2_t	pos;

	if (!GUI_ParseFloatVec (ps, pos, 2)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Vector2Copy (pos, gui->cursor->s.pos);
	return qTrue;
}

static guiParseKey_t	cl_guiDefKeyList[] = {
	{ "cursormat",				&guiDef_cursorMat			},
	{ "cursorcolor",			&guiDef_cursorColor			},
	{ "cursorheight",			&guiDef_cursorHeight		},
	{ "cursorwidth",			&guiDef_cursorWidth			},
	{ "cursorpos",				&guiDef_cursorPos			},
	{ NULL,						NULL						}
};

/*
=============================================================================

	WINDOWDEF PARSING

=============================================================================
*/

qBool GUI_NewWindowDef (char *fileName, gui_t *gui, parse_t *ps, char *keyName);
static guiParseKey_t	cl_windowDefKeyList[] = {
	// Window types
	{ "windowdef",				&GUI_NewWindowDef			},
	{ "binddef",				&GUI_NewWindowDef			},
	{ "choicedef",				&GUI_NewWindowDef			},
	{ "editdef",				&GUI_NewWindowDef			},
	{ "listdef",				&GUI_NewWindowDef			},
	{ "renderdef",				&GUI_NewWindowDef			},
	{ "sliderdef",				&GUI_NewWindowDef			},
	{ NULL,						NULL						}
};

static const char		*cl_windowDefTypes[] = {
	"guidef",		// WTP_GUI,
	"windowdef",	// WTP_GENERIC,

	"binddef",		// WTP_BIND,
	"choicedef",	// WTP_CHOICE,
	"editdef",		// WTP_EDIT,
	"listdef",		// WTP_LIST,
	"renderdef",	// WTP_RENDER,
	"sliderdef",	// WTP_SLIDER,
	"textdef",		// WTP_TEXT

	NULL,
};

/*
==================
GUI_NewWindowDef
==================
*/
qBool GUI_NewWindowDef (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char		windowName[MAX_GUI_NAMELEN];
	gui_t		*newGUI, *childList;
	char		*token;
	guiType_t	type;
	uint32		hashValue;

	// First is the name
	if (!GUI_ParseString (ps, qFalse, &token)) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
		GUI_PrintError ("ERROR: expecting <name> after %s, got '%s'!\n", keyName, token);
		return qFalse;
	}
	if (strlen(token)+1 >= MAX_GUI_NAMELEN) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
		GUI_PrintError ("ERROR: %s name too long!\n", keyName);
		return qFalse;
	}
	// FIXME: check if this name is already in use here!
	// (for both gui names and window children name within the gui)
	Q_strncpyz (windowName, token, sizeof (windowName));

	// Next is the opening brace
	if (!GUI_ParseString (ps, qTrue, &token) || strcmp (token, "{")) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
		GUI_PrintError ("ERROR: expecting '{' after %s <name>, got '%s'!\n", keyName, token);
		return qFalse;
	}

	// Find out the type
	// This should always be valid since we only reach this point through keyFunc's
	for (type=0 ; type<WTP_MAX; type++) {
		if (!strcmp (cl_windowDefTypes[type], keyName))
			break;
	}

	// Allocate a space
	if (gui) {
		// Add to the parent
		if (gui->numChildren+1 >= MAX_GUIS) {
			GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
			GUI_PrintError ("ERROR: too many children!\n");
			return qFalse;
		}
		newGUI = &gui->childList[gui->numChildren];
		newGUI->cursor = gui->cursor;
		Q_strncpyz (newGUI->name, windowName, sizeof (newGUI->name));

		newGUI->parent = gui;
		newGUI->cursor = gui->cursor;

		gui->numChildren++;
	}
	else {
		// This is a parent
		if (cl_numGUI+1 >= MAX_GUIS) {
			GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
			GUI_PrintError ("ERROR: too many GUIs!\n");
			return qFalse;
		}
		newGUI = &cl_guiList[cl_numGUI];
		memset (&cl_guiList[cl_numGUI], 0, sizeof (gui_t));
		Q_strncpyz (newGUI->name, windowName, sizeof (newGUI->name));

		newGUI->cursor = &cl_guiCursors[cl_numGUI];
		memset (&cl_guiCursors[cl_numGUI], 0, sizeof (guiCursor_t));
		Vector4Set (newGUI->cursor->s.color, 1, 1, 1, 1);
		newGUI->cursor->s.disabled = qTrue;

		hashValue = GUI_HashValue (windowName);
		newGUI->hashNext = cl_guiHashTree[hashValue];
		cl_guiHashTree[hashValue] = newGUI;

		cl_numGUI++;
	}

	// Set default values
	newGUI->type = type;
	Vector4Set (newGUI->s.matColor, 1, 1, 1, 1);
	newGUI->s.matScaleX = 1;
	newGUI->s.matScaleY = 1;
	newGUI->s.visible = qTrue;

	switch (type) {
	case WTP_GUI:
	case WTP_GENERIC:

	case WTP_BIND:
	case WTP_CHOICE:
	case WTP_EDIT:
	case WTP_LIST:
	case WTP_RENDER:
	case WTP_SLIDER:
	case WTP_TEXT:
		break;
	}

	// Storage space for children
	newGUI->numChildren = 0;
	newGUI->childList = GUI_AllocTag (sizeof (gui_t) * MAX_GUI_CHILDREN, qTrue, GUITAG_SCRATCH);

	// Parse the keys
	for ( ; ; ) {
		if (!GUI_ParseString (ps, qTrue, &token) || !strcmp (token, "}"))
			break;

		switch (type) {
		case WTP_GUI:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_guiDefKeyList, cl_itemDefKeyList, cl_windowDefKeyList, token))
				return qFalse;
			break;
		case WTP_GENERIC:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_windowDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_BIND:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_bindDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_CHOICE:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_choiceDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_EDIT:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_editDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_LIST:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_listDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_RENDER:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_renderDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_SLIDER:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_sliderDefKeyList, NULL, token))
				return qFalse;
			break;
		case WTP_TEXT:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_textDefKeyList, NULL, token))
				return qFalse;
			break;
		}
	}

	// Final '}' closing brace
	if (strcmp (token, "}")) {
		GUI_PrintLocation (PRNT_ERROR, ps, fileName, NULL);
		GUI_PrintError ("ERROR: expecting '}' after %s, got '%s'!\n", keyName, token);
		return qFalse;
	}

	// Store children
	childList = newGUI->childList;
	if (newGUI->numChildren) {
		newGUI->childList = GUI_AllocTag (sizeof (gui_t) * newGUI->numChildren, qFalse, GUITAG_KEEP);
		memcpy (newGUI->childList, childList, sizeof (gui_t) * newGUI->numChildren);
	}
	else
		newGUI->childList = NULL;
	Mem_Free (childList);

	// Copy static data to dynamic so it gets the defaults at first
	GUI_ResetGUIState (newGUI);

	// Done
	return qTrue;
}

/*
=============================================================================

	GUI SCRIPT PARSING

=============================================================================
*/

/*
==================
GUI_ParseScript
==================
*/
static void GUI_ParseScript (char *fixedName, guiPathType_t pathType)
{
	char	keyName[MAX_TOKEN_CHARS];
	char	*fileBuffer, *buf;
	char	*token;
	int		fileLen;
	parse_t	*ps;

	if (!fixedName)
		return;

	// Load the file
	fileLen = FS_LoadFile (fixedName, (void **)&fileBuffer); 
	if (!fileBuffer || fileLen <= 0) {
		GUI_PrintWarning ("WARNING: can't load '%s' -- %s\n", fixedName, !fileBuffer ? "unable to open" : "no data");
		return;
	}

	Com_Printf (0, "...loading '%s'\n", fixedName);

	// Copy the buffer, and make certain it's newline and null terminated
	buf = (char *)GUI_AllocTag (fileLen+2, qFalse, GUITAG_SCRATCH);
	memcpy (buf, fileBuffer, fileLen);
	buf[fileLen] = '\n';
	buf[fileLen+1] = '\0';

	// Don't need this anymore
	FS_FreeFile (fileBuffer);
	fileBuffer = buf;

	// Start parsing
	ps = Com_BeginParseSession (&fileBuffer);

	// Pull out the version and check it
	token = Com_ParseExt (ps, qTrue);
	if (!token || !token[0]) {
		GUI_PrintError ("ERROR: No version string at beginning of file, ignoring!\n");
		Com_EndParseSession (ps);
		Mem_Free (buf);
		GUI_FreeTag (GUITAG_SCRATCH);
		return;
	}
	if (strcmp (token, GUI_VERSION)) {
		GUI_PrintError ("ERROR: Version mismatch ('%s' != '%s'), ignoring!\n", token, GUI_VERSION);
		Com_EndParseSession (ps);
		Mem_Free (buf);
		GUI_FreeTag (GUITAG_SCRATCH);
		return;
	}

	// Parse the script
	for ( ; ; ) {
		if (!GUI_ParseString (ps, qTrue, &token))
			break;

		// Search for a guiDef
		if (strcmp (token, "guidef")) {
			GUI_PrintLocation (PRNT_ERROR, ps, fixedName, NULL);
			GUI_PrintError ("ERROR: expecting 'guiDef' got '%s'!\n", token);
			GUI_PrintError ("ERROR: removing GUI and halting on file due to errors!\n");
			break;
		}

		// Found one, create it
		Q_strncpyz (keyName, token, sizeof (keyName));
		if (GUI_NewWindowDef (fixedName, NULL, ps, keyName)) {
			// It parsed ok, store the pathType
			cl_guiPathTypes[cl_numGUI-1] = pathType;
		}
		else {
			// Failed to parse, halt!
			GUI_PrintError ("ERROR: removing GUI and halting on file due to errors!\n");
			memset (&cl_guiList[--cl_numGUI], 0, sizeof (gui_t));
			break;
		}
	}

	// Done
	Com_EndParseSession (ps);
	Mem_Free (buf);
	GUI_FreeTag (GUITAG_SCRATCH);
}

/*
=============================================================================

	REGISTRATION

=============================================================================
*/

/*
==================
GUI_TouchGUI
==================
*/
static void GUI_CantFindMedia (gui_t *gui, char *item, char *name)
{
	if (gui && gui->name[0]) {
		GUI_PrintWarning ("WARNING: GUI '%s': can't find %s '%s'", gui->name, item, name);
		return;
	}

	GUI_PrintWarning ("WARNING: can't find %s '%s'", item, name);
}
static void GUI_r_TouchGUI (gui_t *gui)
{
	gui_t	*child;
	uint32	i;

	// Load generic media
	if (gui->flags & WFL_MATERIAL) {
		gui->s.matShader = R_RegisterPic (gui->s.matName);
		if (!gui->s.matShader)
			GUI_CantFindMedia (gui, "Material", gui->s.matName);
	}

	// Load type-specific media
	switch (gui->type) {
	case WTP_GUI:
	case WTP_GENERIC:
	case WTP_BIND:
	case WTP_CHOICE:
	case WTP_EDIT:
	case WTP_LIST:
	case WTP_RENDER:
	case WTP_SLIDER:
	case WTP_TEXT:
		break;
	}

	// Recurse down the children
	for (i=0, child=gui->childList ; i<gui->numChildren ; child++, i++)
		GUI_r_TouchGUI (child);
}
static void GUI_TouchGUI (gui_t *gui)
{
	// Register cursor media
	if (gui->flags & WFL_CURSOR) {
		gui->cursor->s.mat = R_RegisterPic (gui->cursor->s.name);
		if (!gui->cursor->s.mat)
			GUI_CantFindMedia (gui, "Cursor", gui->cursor->s.name);
	}

	// Register base media
	GUI_r_TouchGUI (gui);
}


/*
==================
GUI_RegisterGUI
==================
*/
gui_t *GUI_RegisterGUI (char *name)
{
	gui_t	*gui;

	// Find it
	gui = GUI_FindGUI (name);
	if (!gui)
		return NULL;

	// Touch it
	GUI_TouchGUI (gui);
	cl_guiRegTouched[gui-&cl_guiList[0]] = cl_guiRegFrameNum;

	// Reset state
	GUI_ResetGUIState (gui);
	return gui;
}


/*
==================
GUI_BeginRegistration
==================
*/
void GUI_BeginRegistration (void)
{
	cl_guiRegFrameNum++;
}


/*
==================
GUI_EndRegistration
==================
*/
void GUI_EndRegistration (void)
{
	gui_t		*gui;
	uint32		i;

	// Register media specific to GUI windows
	for (i=0, gui=cl_guiList ; i<cl_numGUI ; gui++, i++) {
		if (cl_guiRegTouched[i] != cl_guiRegFrameNum
		&& cl_guiRegFrames[i] != cl_guiRegFrameNum)
			continue;	// Don't touch if already touched and if it wasn't scheduled for a touch

		cl_guiRegTouched[i] = cl_guiRegFrameNum;
		GUI_TouchGUI (gui);
	}
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static void *cmd_gui_list;
static void	*cmd_gui_restart;
static void *cmd_gui_test;

/*
==================
GUI_List_f
==================
*/
static void GUI_r_List_f (gui_t *window, uint32 depth)
{
	gui_t	*child;
	uint32	i;

	for (i=0 ; i<depth ; i++)
		Com_Printf (0, "   ");
	Com_Printf (0, "%s (%i children)\n", window->name, window->numChildren);

	// Recurse down the children
	for (i=0, child=window->childList ; i<window->numChildren ; child++, i++)
		GUI_r_List_f (child, depth+1);
}
static void GUI_List_f (void)
{
	gui_t	*gui;
	uint32	i;

	// List all GUIs and their child windows
	for (i=0, gui=cl_guiList ; i<cl_numGUI ; gui++, i++) {
		Com_Printf (0, "#%2i GUI %s (%i children)\n", i+1, gui->name, gui->numChildren);
		GUI_r_List_f (gui, 0);
	}
}


/*
==================
GUI_Restart_f
==================
*/
static void GUI_Restart_f (void)
{
	GUI_Shutdown ();
	GUI_Init ();
}


/*
==================
GUI_Test_f
==================
*/
static void GUI_Test_f (void)
{
	gui_t	*gui;

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: gui_test <GUI name>\n");
		return;
	}

	// Find it and touch it
	gui = GUI_RegisterGUI (Cmd_Argv (1));
	if (!gui) {
		Com_Printf (0, "Not found...\n");
		return;
	}

	// Open it
	GUI_OpenGUI (gui);
}


/*
==================
GUI_Init
==================
*/
void GUI_Init (void)
{
	char			*guiList[MAX_GUIS];
	char			fixedName[MAX_QPATH];
	uint32			numGUI, i;
	guiPathType_t	pathType;
	char			*name;

	Com_Printf (0, "\n---------- GUI Initialization ----------\n");
	Com_Printf (0, "Version: %s\n", GUI_VERSION);

	// Clear lists
	cl_numGUI = 0;
	memset (cl_guiHashTree, 0, sizeof (gui_t *) * MAX_GUIS);
	memset (&cl_guiList[0], 0, sizeof (gui_t) * MAX_GUIS);
	memset (&cl_guiPathTypes[0], 0, sizeof (guiPathType_t) * MAX_GUIS);
	memset (&cl_guiCursors[0], 0, sizeof (guiCursor_t) * MAX_GUIS);

	// Console commands
	cmd_gui_list	= Cmd_AddCommand ("gui_list",		GUI_List_f,		"List GUIs in memory");
	cmd_gui_restart	= Cmd_AddCommand ("gui_restart",	GUI_Restart_f,	"Restart the GUI system");
	cmd_gui_test	= Cmd_AddCommand ("gui_test",		GUI_Test_f,		"Test the specified GUI");

	// Cvars
	gui_developer			= Cvar_Register ("gui_developer",				"0",		0);
	gui_debugBounds			= Cvar_Register ("gui_debugBounds",				"0",		0);
	gui_mouseFilter			= Cvar_Register ("gui_mouseFilter",				"1",		CVAR_ARCHIVE);
	gui_mouseSensitivity	= Cvar_Register ("gui_mouseSensitivity",		"2",		CVAR_ARCHIVE);

	// Load scripts
	cl_numGUIErrors = 0;
	cl_numGUIWarnings = 0;
	numGUI = FS_FindFiles ("guis", "*guis/*.gui", "gui", guiList, MAX_GUIS, qTrue, qFalse);
	for (i=0 ; i<numGUI ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), guiList[i]);

		// Skip the path
		name = strstr (fixedName, "/guis/");
		if (!name)
			continue;
		name++;	// Skip the initial '/'

		// Base dir GUI?
		if (strstr (guiList[i], BASE_MODDIRNAME "/"))
			pathType = GUIPT_BASEDIR;
		else
			pathType = GUIPT_MODDIR;

		// Parse it
		GUI_ParseScript (name, pathType);
	}
	FS_FreeFileList (guiList, numGUI);

	// Free auxillery (scratch space) tags
	GUI_FreeTag (GUITAG_SCRATCH);

	Com_Printf (0, "GUI - %i error(s), %i warning(s)\n", cl_numGUIErrors, cl_numGUIWarnings);
	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
GUI_Shutdown
==================
*/
void GUI_Shutdown (void)
{
	// Close all open GUIs
	GUI_CloseAllGUIs ();

	// Remove commands
	Cmd_RemoveCommand ("gui_list", cmd_gui_list);
	Cmd_RemoveCommand ("gui_restart", cmd_gui_restart);
	Cmd_RemoveCommand ("gui_test", cmd_gui_test);

	// Release all used memory
	GUI_FreeTag (GUITAG_KEEP);
	GUI_FreeTag (GUITAG_SCRATCH);
}
