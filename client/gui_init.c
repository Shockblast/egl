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

static gui_t			cl_guiList[MAX_GUIS];
static guiShared_t		cl_guiSharedInfo[MAX_GUIS];
static uint32			cl_numGUI;

static uint32			cl_numGUIErrors;
static uint32			cl_numGUIWarnings;

static uint32			cl_guiRegFrames[MAX_GUIS];
static uint32			cl_guiRegTouched[MAX_GUIS];
static uint32			cl_guiRegFrameNum;

cVar_t	*gui_developer;
cVar_t	*gui_debugBounds;
cVar_t	*gui_debugScale;
cVar_t	*gui_mouseFilter;
cVar_t	*gui_mouseSensitivity;

/*
================
GUI_HashValue

String passed to this should be lowercase.
FIXME: use this! Though it may be unecessary since nested windows
will only be looked up while GUIs are being parsed
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
GUI_FindGUI
================
*/
static gui_t *GUI_FindGUI (char *name)
{
	gui_t	*gui, *bestMatch;
	char	tempName[MAX_QPATH];
	uint32	bestNum, i;

	// Make sure it's lowercase
	Q_strncpyz (tempName, name, sizeof (tempName));
	Q_strlwr (tempName);

	bestMatch = NULL;
	bestNum = 0;

	// Look for it
	for (i=0, gui=cl_guiList ; i<cl_numGUI ; gui++, i++) {
		if (strcmp (gui->name, tempName))
			continue;

		if (!bestMatch || gui->shared->pathType >= gui->shared->pathType) {
			bestMatch = gui;
			bestNum = i;
		}
	}

	return bestMatch;
}


/*
================
GUI_FindWindow

lowerName must be lowercase!
================
*/
static gui_t *GUI_FindWindow (gui_t *gui, char *lowerName)
{
	gui_t	*child, *best;
	uint32	i;

	// See if it matches
	if (!strcmp (lowerName, gui->name))
		return gui;

	// Recurse down the children
	for (i=0, child=gui->childList ; i<gui->numChildren ; child++, i++) {
		best = GUI_FindWindow (child, lowerName);
		if (best)
			return best;
	}

	return NULL;
}


/*
===============
GUI_FindDefFloat
===============
*/
static short GUI_FindDefFloat (gui_t *gui, char *lowerName)
{
	defineFloat_t	*flt;
	short			i;

	for (i=0, flt=gui->s.defFloatList ; i<gui->s.numDefFloats ; flt++, i++) {
		if (!strcmp (lowerName, flt->name))
			break; 
	}
	if (i == gui->s.numDefFloats)
		return -1;

	return i;
}


/*
===============
GUI_FindDefVec
===============
*/
static short GUI_FindDefVec (gui_t *gui, char *lowerName)
{
	defineVec_t	*vec;
	short		i;

	for (i=0, vec=gui->s.defVecList ; i<gui->s.numDefVecs ; vec++, i++) {
		if (!strcmp (lowerName, vec->name))
			break; 
	}
	if (i == gui->s.numDefVecs)
		return -1;

	return i;
}


/*
===============
GUI_CvarValidate
===============
*/
static qBool GUI_CvarValidate (const char *name)
{
	if (strchr (name, '\\'))
		return qFalse;
	if (strchr (name, '\"'))
		return qFalse;
	if (strchr (name, ';'))
		return qFalse;

	return qTrue;
}

/*
=============================================================================

	PARSE HELPERS

=============================================================================
*/

/*
==================
GUI_PrintPos
==================
*/
static void GUI_PrintPos (comPrint_t flags, parse_t *ps, char *fileName, gui_t *gui)
{
	uint32		line, col;

	// Increment tallies
	if (flags & PRNT_ERROR)
		cl_numGUIErrors++;
	else if (flags & PRNT_WARNING)
		cl_numGUIWarnings++;

	if (ps) {
		// Print the position
		PS_GetPosition (ps, &line, &col);
		if (gui)
			Com_Printf (flags, "%s(line #%i col#%i): window '%s':\n", fileName, line, col, gui->name);
		else
			Com_Printf (flags, "%s(line #%i col#%i):\n", fileName, line, col);
		return;
	}

		// Print the position
	Com_Printf (flags, "%s:\n", fileName);
}


/*
==================
GUI_DevPrintPos
==================
*/
static void GUI_DevPrintPos (comPrint_t flags, parse_t *ps, char *fileName, gui_t *gui)
{
	if (!gui_developer->intVal && !developer->intVal)
		return;

	GUI_PrintPos (flags, ps, fileName, gui);
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

	if (!gui_developer->intVal && !developer->intVal)
		return;

	if (flags & (PRNT_ERROR|PRNT_WARNING)) {
		if (flags & PRNT_ERROR)
			cl_numGUIErrors++;
		else if (flags & PRNT_WARNING)
			cl_numGUIWarnings++;
		GUI_PrintPos (flags, ps, fileName, gui);
	}

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
GUI_ParseFloatRegister
==================
*/
static qBool GUI_ParseFloatRegister (char *fileName, gui_t *gui, parse_t *ps, char *keyName, floatRegister_t *floatReg, qBool requireValue, float defaultValue)
{
	char		source[MAX_PS_TOKCHARS];
	float		storage;
	char		windowName[MAX_GUI_NAMELEN];
	gui_t		*windowPtr;
	char		floatName[MAX_GUI_NAMELEN];
	short		floatNum;
	guiVar_t	*var;
	char		*charToken;
	char		*p;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &charToken)) {
		if (!requireValue) {
			floatReg->sourceType = REG_SOURCE_SELF;
			floatReg->storage = defaultValue;
			return qTrue;
		}

		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	if (charToken[0] == '$') {
		// Parse "[window::]var"
		Q_strncpyz (source, &charToken[1], sizeof (source));
		p = strstr (source, "::");
		if (p) {
			if (!*(p+2)) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: invalid argument for '%s', contains '::' with no flag name!\n", keyName);
				return qFalse;
			}

			// "<window::>var"
			Q_strncpyz (windowName, source, sizeof (windowName));
			p = strstr (windowName, "::");
			*p = '\0';
			// "window::<var>"
			Q_strncpyz (floatName, p+2, sizeof (floatName));
		}
		else {
			// Default to this window
			Q_strncpyz (windowName, gui->name, sizeof (windowName));

			// "<var>"
			Q_strncpyz (floatName, source, sizeof (floatName));
		}

		// Check if we're looking for a guiVar
		if (!strcmp (windowName, "guivar")) {
			// Find the variable
			var = GUIVar_Register (floatName, GVT_FLOAT);
			if (!var) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find guivar '%s'\n", floatName);
				return qFalse;
			}

			floatReg->sourceType = REG_SOURCE_GUIVAR;
			floatReg->guiVar = var;
			return qTrue;
		}
		else {
			// Find the window
			windowPtr = GUI_FindWindow (gui->owner, windowName);
			if (!windowPtr) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find window '%s'\n", windowName);
				return qFalse;
			}

			// Find the defineFloat
			floatNum = GUI_FindDefFloat (windowPtr, floatName);
			if (floatNum == -1) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find float '%s'\n", floatName);
				return qFalse;
			}

			floatReg->sourceType = REG_SOURCE_DEF;
			floatReg->defFloatIndex = floatNum;
			floatReg->defFloatWindow = windowPtr;
		}

		return qTrue;
	}

	// Not a pointer, use value
	PS_UndoParse (ps);
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &storage, 1)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	floatReg->sourceType = REG_SOURCE_SELF;
	floatReg->storage = storage;
	return qTrue;
}


/*
==================
GUI_ParseVectorRegister
==================
*/
static qBool GUI_ParseVectorRegister (char *fileName, gui_t *gui, parse_t *ps, char *keyName, vecRegister_t *vecReg)
{
	char		source[MAX_PS_TOKCHARS];
	vec4_t		storage;
	char		windowName[MAX_GUI_NAMELEN];
	gui_t		*windowPtr;
	char		vecName[MAX_GUI_NAMELEN];
	short		vecNum;
	guiVar_t	*var;
	char		*charToken;
	char		*p;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &charToken)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	if (charToken[0] == '$') {
		// Parse "[window::]var"
		Q_strncpyz (source, &charToken[1], sizeof (source));
		p = strstr (source, "::");
		if (p) {
			if (!*(p+2)) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: invalid argument for '%s', contains '::' with no flag name!\n", keyName);
				return qFalse;
			}

			// "<window::>var"
			Q_strncpyz (windowName, source, sizeof (windowName));
			p = strstr (windowName, "::");
			*p = '\0';
			// "window::<var>"
			Q_strncpyz (vecName, p+2, sizeof (vecName));
		}
		else {
			// Default to this window
			Q_strncpyz (windowName, gui->name, sizeof (windowName));

			// "<var>"
			Q_strncpyz (vecName, source, sizeof (vecName));
		}

		// Check if we're looking for a guiVar
		if (!strcmp (windowName, "guivar")) {
			// Find the variable
			var = GUIVar_Register (vecName, GVT_VEC);
			if (!var) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find guivar '%s'\n", vecName);
				return qFalse;
			}

			vecReg->sourceType = REG_SOURCE_GUIVAR;
			vecReg->guiVar = var;
			return qTrue;
		}
		else {
			// Find the window
			windowPtr = GUI_FindWindow (gui->owner, windowName);
			if (!windowPtr) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find window '%s'\n", windowName);
				return qFalse;
			}

			// Find the defineVec
			vecNum = GUI_FindDefVec (windowPtr, vecName);
			if (vecNum == -1) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: unable to find vec '%s'\n", vecName);
				return qFalse;
			}

			vecReg->sourceType = REG_SOURCE_DEF;
			vecReg->defVecIndex = vecNum;
			vecReg->defVecWindow = windowPtr;
		}

		return qTrue;
	}

	// Not a pointer, use value
	PS_UndoParse (ps);
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &storage, 4)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	vecReg->sourceType = REG_SOURCE_SELF;
	Vec4Copy (storage, vecReg->storage);
	return qTrue;
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
	char			keyName[MAX_PS_TOKCHARS];
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
				PS_SkipLine (ps);
				return qTrue;
			}

			// Failed to parse line
			if (!key->func (fileName, gui, ps, keyName)) {
				PS_SkipLine (ps);
				return qFalse;
			}

			// Report any extra parameters
			if (PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
				GUI_PrintPos (PRNT_WARNING, ps, fileName, gui);
				GUI_PrintWarning ("WARNING: unused trailing parameters after key '%s', \"%s\"\n", keyName, str);
				PS_SkipLine (ps);
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

	GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
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
itemDef_item
==================
*/
static qBool itemDef_item (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	item;

	if (!PS_ParseDataType (ps, 0, PSDT_BOOLEAN, &item, 1)) {
		GUI_DevPrintPos (PRNT_WARNING, ps, fileName, gui);
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		item = qTrue;
	}

	if (item)
		gui->flags |= WFL_ITEM;
	else
		gui->flags &= ~WFL_ITEM;
	return qTrue;
}

// ==========================================================================

static qBool itemDef_rect (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseVectorRegister (fileName, gui, ps, keyName, &gui->s.vecRegisters[VR_RECT]);
}
static qBool itemDef_rotate (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_ROTATION], qTrue, 0);
}

// ==========================================================================

/*
==================
itemDef_mat
==================
*/
static qBool itemDef_mat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Q_strncpyz (gui->matName, str, sizeof (gui->matName));
	gui->flags |= WFL_MATERIAL;
	return qTrue;
}

static qBool itemDef_fill (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	if (GUI_ParseVectorRegister (fileName, gui, ps, keyName, &gui->s.vecRegisters[VR_FILL_COLOR])) {
		gui->flags |= WFL_FILL_COLOR;
		return qTrue;
	}
	return qFalse;
}
static qBool itemDef_matColor (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseVectorRegister (fileName, gui, ps, keyName, &gui->s.vecRegisters[VR_MAT_COLOR]);
}
static qBool itemDef_matScaleX (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_MAT_SCALE_X], qTrue, 1);
}
static qBool itemDef_matScaleY (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_MAT_SCALE_Y], qTrue, 1);
}

// ==========================================================================

/*
==================
itemDef_defineFloat
==================
*/
static qBool itemDef_defineFloat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char			*name;
	float			floatToken;
	defineFloat_t	*flt;
	int				i;

	// Get the name
	if (!PS_ParseToken (ps, PSF_TO_LOWER, &name)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Check for duplicates
	for (i=0, flt=&gui->s.defFloatList[0] ; i<gui->s.numDefFloats ; flt++, i++) {
		if (strcmp (flt->name, name))
			continue;
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name already in use!\n", keyName, name);
		return qFalse;
	}

	// Store
	Q_strncpyz (gui->s.defFloatList[gui->s.numDefFloats].name, name, sizeof (gui->s.defFloatList[gui->s.numDefFloats].name));

	// Get the value
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &floatToken, 1)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Store
	gui->s.defFloatList[gui->s.numDefFloats].value = floatToken;
	gui->s.numDefFloats++;
	return qTrue;
}


/*
==================
itemDef_defineVec
==================
*/
static qBool itemDef_defineVec (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char		*name;
	vec4_t		vecToken;
	defineVec_t	*vec;
	int			i;

	// Get the name
	if (!PS_ParseToken (ps, PSF_TO_LOWER, &name)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Check for duplicates
	for (i=0, vec=&gui->s.defVecList[0] ; i<gui->s.numDefVecs ; vec++, i++) {
		if (strcmp (vec->name, name))
			continue;
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name already in use!\n", keyName, name);
		return qFalse;
	}

	// Store
	Q_strncpyz (gui->s.defVecList[gui->s.numDefVecs].name, name, sizeof (gui->s.defVecList[gui->s.numDefVecs].name));

	// Get the value
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &vecToken[0], 4)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Store
	Vec4Copy (vecToken, gui->s.defVecList[gui->s.numDefVecs].value);
	gui->s.numDefVecs++;
	return qTrue;
}

// ==========================================================================

static qBool itemDef_modal (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_MODAL], qFalse, 1);
}
static qBool itemDef_noEvents (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_NO_EVENTS], qFalse, 1);
}
static qBool itemDef_noTime (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_NO_TIME], qFalse, 1);
}
static qBool itemDef_visible (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_VISIBLE], qFalse, 1);
}
static qBool itemDef_wantEnter (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_WANT_ENTER], qFalse, 1);
}

// ==========================================================================

/*
==================
event_newAction
==================
*/
typedef struct eva_setDest_s {
	const char		*name;

	int				destNumVecs;
	uint32			destRegister;
	set_destType_t	destType;

	guiType_t		destWindowType;
} eva_setDest_t;

static eva_setDest_t gui_setDests[] = {
	// textDef
	{ "textcolor",			4,		VR_TEXT_COLOR,		EVA_SETDEST_VEC,	WTP_TEXT	},
	{ "texthovercolor",		4,		VR_TEXT_HOVERCOLOR,	EVA_SETDEST_VEC,	WTP_TEXT	},

	{ "textalign",			1,		FR_TEXT_ALIGN,		EVA_SETDEST_FLOAT,	WTP_TEXT	},
	{ "textscale",			1,		FR_TEXT_SCALE,		EVA_SETDEST_FLOAT,	WTP_TEXT	},
	{ "textshadow",			1,		FR_TEXT_SHADOW,		EVA_SETDEST_FLOAT,	WTP_TEXT	},

	// All windows
	{ "fillcolor",			4,		VR_FILL_COLOR,		EVA_SETDEST_VEC,	WTP_MAX		},
	{ "matcolor",			4,		VR_MAT_COLOR,		EVA_SETDEST_VEC,	WTP_MAX		},

	{ "matscalex",			1,		FR_MAT_SCALE_X,		EVA_SETDEST_FLOAT,	WTP_MAX		},
	{ "matscaley",			1,		FR_MAT_SCALE_Y,		EVA_SETDEST_FLOAT,	WTP_MAX		},

	{ "modal",				1,		FR_MODAL,			EVA_SETDEST_FLOAT,	WTP_MAX		},
	{ "noevents",			1,		FR_NO_EVENTS,		EVA_SETDEST_FLOAT,	WTP_MAX		},
	{ "notime",				1,		FR_NO_TIME,			EVA_SETDEST_FLOAT,	WTP_MAX		},
	{ "visible",			1,		FR_VISIBLE,			EVA_SETDEST_FLOAT,	WTP_MAX		},
	{ "wantenter",			1,		FR_WANT_ENTER,		EVA_SETDEST_FLOAT,	WTP_MAX		},

	{ NULL,					0,		0,					0,					WTP_MAX		},
};

static qBool event_newAction (evAction_t *newAction, evaType_t type, char *fileName, gui_t *gui, parse_t *ps, char *keyName, qBool gotSemicolon)
{
	eva_setDest_t	*setDest;
	char			target[MAX_PS_TOKCHARS];
	char			*charToken;
	char			*p;

	// If we got a semi-colon, certain types require args
	if (gotSemicolon) {
		switch (type) {
		case EVA_COMMAND:
		case EVA_LOCAL_SOUND:
		case EVA_RESET_TIME:
		case EVA_SET:
		case EVA_TRANSITION:
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}
	}

	// Parse arguments
	switch (type) {
	case EVA_CLOSE:
		break;

	case EVA_COMMAND:
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &charToken)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		newAction->command = GUI_AllocTag (strlen(charToken)+2, GUITAG_SCRATCH);
		Q_snprintfz (newAction->command, strlen(charToken)+2, "%s\n", charToken);
		break;

	case EVA_IF:
		break;

	case EVA_LOCAL_SOUND:
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &charToken)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		newAction->localSound = GUI_AllocTag (sizeof (eva_localSound_t), GUITAG_SCRATCH);

		Com_NormalizePath (newAction->localSound->name, sizeof (newAction->localSound->name), charToken);

		// FIXME: Make a register?
		if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &newAction->localSound->volume, 1)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}
		break;

	case EVA_NAMED_EVENT:
		// Get the "[window::]event" token in lower-case for faster lookup
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &charToken)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		newAction->named = GUI_AllocTag (sizeof (eva_named_t), GUITAG_SCRATCH);

		// Parse "[window::]event"
		Q_strncpyz (target, charToken, sizeof (target));
		p = strstr (target, "::");
		if (p) {
			// Make sure "window::<event>" exists
			if (!*(p+2)) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: invalid argument for '%s', contains '::' with no event name!\n", keyName);
				return qFalse;
			}

			// "<window::>event"
			Q_strncpyz (newAction->named->destWindowName, target, sizeof (newAction->named->destWindowName));
			p = strstr (newAction->named->destWindowName, "::");
			*p = '\0';

			// "window::<event>"
			Q_strncpyz (newAction->named->eventName, p+2, sizeof (newAction->named->eventName));
		}
		else {
			// Default to this window
			Q_strncpyz (newAction->named->destWindowName, gui->name, sizeof (newAction->named->destWindowName));

			// "<event>"
			Q_strncpyz (newAction->named->eventName, target, sizeof (newAction->named->eventName));
		}
		break;

	case EVA_RESET_TIME:
		if (!PS_ParseDataType (ps, 0, PSDT_INTEGER, &newAction->resetTime, 1)) {
			GUI_PrintPos (PRNT_WARNING, ps, fileName, gui);
			GUI_PrintWarning ("WARNING: invalid/missing arguments for '%s', assuming '0'\n", keyName);
			newAction->resetTime = 0;
		}
		break;

	case EVA_SET:
		// Get the "[window::]register" token in lower-case for faster lookup
		if (!PS_ParseToken (ps, PSF_TO_LOWER, &charToken)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		newAction->set = GUI_AllocTag (sizeof (eva_set_t), GUITAG_SCRATCH);

		// Parse "[window::]register" destination
		Q_strncpyz (target, charToken, sizeof (target));
		p = strstr (target, "::");
		if (p) {
			// Make sure "window::<register>" exists
			if (!*(p+2)) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: invalid argument for '%s', contains '::' with no flag name!\n", keyName);
				return qFalse;
			}

			// "<window::>register"
			Q_strncpyz (newAction->set->destWindowName, target, sizeof (newAction->set->destWindowName));
			p = strstr (newAction->set->destWindowName, "::");
			*p = '\0';

			// "window::<register>"
			Q_strncpyz (newAction->set->destVarName, p+2, sizeof (newAction->set->destVarName));
		}
		else {
			// Default to this window
			Q_strncpyz (newAction->set->destWindowName, gui->name, sizeof (newAction->set->destWindowName));

			// "<register>"
			Q_strncpyz (newAction->set->destVarName, target, sizeof (newAction->set->destVarName));
		}

		// Find the destination register type
		for (setDest=gui_setDests ; setDest->name ; setDest++) {
			if (!strcmp (newAction->set->destVarName, setDest->name))
				break;
		}
		if (!setDest) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid '%s' variable '%s'!\n", keyName, newAction->set->destVarName);
			return qFalse;
		}

		// Store destination parms
		newAction->set->destNumVecs = setDest->destNumVecs;
		newAction->set->destRegister = setDest->destRegister;
		newAction->set->destType = setDest->destType;
		newAction->set->destWindowType = setDest->destWindowType;

		newAction->set->destType |= EVA_SETDEST_STORAGE;

		// Check if the source is a defineFloat/defineVec
		if (!PS_ParseToken (ps, PSF_TO_LOWER, &charToken)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		if (charToken[0] == '$') {
			// Parse "[window::]var"
			Q_strncpyz (target, &charToken[1], sizeof (target));
			p = strstr (target, "::");
			if (p) {
				if (!*(p+2)) {
					GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
					GUI_PrintError ("ERROR: invalid argument for '%s', contains '::' with no flag name!\n", keyName);
					return qFalse;
				}

				// "<window::>var"
				Q_strncpyz (newAction->set->srcWindowName, target, sizeof (newAction->set->srcWindowName));
				p = strstr (newAction->set->srcWindowName, "::");
				*p = '\0';

				// "window::<var>"
				Q_strncpyz (newAction->set->srcName, p+2, sizeof (newAction->set->srcName));

				// Check if we're looking for a guiVar
				if (!strcmp (newAction->set->srcWindowName, "guivar")) {
					newAction->set->srcType = EVA_SETSRC_GUIVAR;
					break;
				}
				else {
					// Nope, it's a defineFloat/defineVec
					newAction->set->srcType = EVA_SETSRC_DEF;
					break;
				}
			}
			else {
				// Default to this window
				Q_strncpyz (newAction->set->srcWindowName, gui->name, sizeof (newAction->set->srcWindowName));

				// "<var>"
				Q_strncpyz (newAction->set->srcName, target, sizeof (newAction->set->srcName));

				// defineFloat/defineVec
				newAction->set->srcType = EVA_SETSRC_DEF;
				break;
			}
		}

		// Not a pointer, parse the var setValue
		PS_UndoParse (ps);
		if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &newAction->set->srcStorage[0], setDest->destNumVecs)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		// Store source parms
		newAction->set->srcType = EVA_SETSRC_STORAGE;
		break;

	case EVA_STOP_TRANSITIONS:
		break;

	case EVA_TRANSITION:
		break;
	}

	// Final ';'
	if (!gotSemicolon && (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &charToken) || strcmp (charToken, ";"))) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: expecting ';' after %s <args>, got '%s'!\n", keyName, charToken);
		return qFalse;
	}

	newAction->type = type;
	return qTrue;
}


/*
==================
itemDef_newEvent
==================
*/
static qBool itemDef_newEvent (evType_t type, char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	event_t		*newEvent;
	char		actionName[MAX_PS_TOKCHARS];
	evaType_t	action;
	evAction_t	*actionList;
	char		*token;
	size_t		len;
	qBool		gotSemicolon;
	int			i;

	// Allocate a spot
	if (gui->numEvents+1 >= MAX_GUI_EVENTS) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: too many events!\n");
		return qFalse;
	}
	newEvent = &gui->eventList[gui->numEvents];
	memset (&gui->eventList[gui->numEvents], 0, sizeof (event_t));
	newEvent->type = type;

	// Parse arguments
	switch (type) {
	case WEV_NAMED:
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) || !strcmp (token, "{")) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}

		newEvent->named = GUI_StrDup (token, GUITAG_SCRATCH);
		break;

	case WEV_TIME:
		if (!PS_ParseDataType (ps, PSF_ALLOW_NEWLINES, PSDT_INTEGER, &len, 1)) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: invalid/missing arguments for '%s'!\n", keyName);
			return qFalse;
		}
		newEvent->onTime = len;
		break;
	}

	// Make sure the event doesn't already exist
	for (i=0 ; i<gui->numEvents ; i++) {
		if (gui->eventList[i].type != type)
			continue;

		// You can have multiple numbers of these if the args don't match
		switch (gui->eventList[i].type) {
		case WEV_NAMED:
			if (!strcmp (gui->eventList[i].named, newEvent->named)) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: event '%s' already exists (with the same arguments) for this window!\n", keyName);
				return qFalse;
			}
			break;

		case WEV_TIME:
			if (gui->eventList[i].onTime == newEvent->onTime) {
				GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
				GUI_PrintError ("ERROR: event '%s' already exists (with the same arguments) for this window!\n", keyName);
				return qFalse;
			}
			break;

		default:
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: event '%s' already exists for this window!\n", keyName);
			return qFalse;
		}
	}

	// Next is the opening brace
	if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) || strcmp (token, "{")) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: expecting '{' after %s <args>, got '%s'!\n", keyName, token);
		return qFalse;
	}

	// Storage space for children
	newEvent->numActions = 0;
	newEvent->actionList = GUI_AllocTag (sizeof (evAction_t) * MAX_EVENT_ACTIONS, GUITAG_SCRATCH);

	// Parse the actions
	for ( ; ; ) {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) || !strcmp (token, "}"))
			break;

		// Store the name and trim the ';' if found
		// FIXME: start at the end, skipping whitespace until ';' is hit.
		Q_strncpyz (actionName, token, sizeof (actionName));
		len = strlen (actionName);
		if (actionName[len-1] == ';') {
			actionName[len-1] = '\0';
			gotSemicolon = qTrue;
		}
		else {
			gotSemicolon = qFalse;
		}

		// Find out the type
		if (!strcmp (actionName, "close"))
			action = EVA_CLOSE;
		else if (!strcmp (actionName, "command"))
			action = EVA_COMMAND;
		else if (!strcmp (actionName, "if"))
			action = EVA_IF;
		else if (!strcmp (actionName, "localsound"))
			action = EVA_LOCAL_SOUND;
		else if (!strcmp (actionName, "namedevent"))
			action = EVA_NAMED_EVENT;
		else if (!strcmp (actionName, "resettime"))
			action = EVA_RESET_TIME;
		else if (!strcmp (actionName, "set"))
			action = EVA_SET;
		else if (!strcmp (actionName, "stoptransitions"))
			action = EVA_STOP_TRANSITIONS;
		else if (!strcmp (actionName, "transition"))
			action = EVA_TRANSITION;
		else {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: unknown action '%s'!\n", actionName);
			return qFalse;
		}

		// Parse it
		if (!event_newAction (&newEvent->actionList[newEvent->numActions], action, fileName, gui, ps, actionName, gotSemicolon))
			return qFalse;

		// Done
		newEvent->numActions++;
	}

	// Closing brace
	if (strcmp (token, "}")) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: expecting '}' after %s <args>, got '%s'!\n", keyName, token);
		return qFalse;
	}

	// Store events
	actionList = newEvent->actionList;
	if (newEvent->numActions) {
		newEvent->actionList = GUI_AllocTag (sizeof (evAction_t) * newEvent->numActions, GUITAG_SCRATCH);
		memcpy (newEvent->actionList, actionList, sizeof (evAction_t) * newEvent->numActions);
		Mem_Free (actionList);
	}
	else
		newEvent->actionList = NULL;

	// Done
	gui->numEvents++;
	return qTrue;
}

static qBool itemDef_onAction (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_ACTION, fileName, gui, ps, keyName);
}
static qBool itemDef_onEsc (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_ESCAPE, fileName, gui, ps, keyName);
}
static qBool itemDef_onFrame (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_FRAME, fileName, gui, ps, keyName);
}
static qBool itemDef_onInit (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_INIT, fileName, gui, ps, keyName);
}
static qBool itemDef_onMouseEnter (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_MOUSE_ENTER, fileName, gui, ps, keyName);
}
static qBool itemDef_onMouseExit (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_MOUSE_EXIT, fileName, gui, ps, keyName);
}
static qBool itemDef_onNamedEvent (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_NAMED, fileName, gui, ps, keyName);
}
static qBool itemDef_onShutdown (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_SHUTDOWN, fileName, gui, ps, keyName);
}
static qBool itemDef_onTime (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return itemDef_newEvent (WEV_TIME, fileName, gui, ps, keyName);
}

// ==========================================================================

static guiParseKey_t	cl_itemDefKeyList[] = {
	// Flags
	{ "item",					&itemDef_item				},

	// Orientation
	{ "rect",					&itemDef_rect				},
	{ "rotate",					&itemDef_rotate				},

	// Background
	{ "fill",					&itemDef_fill				},
	{ "mat",					&itemDef_mat				},
	{ "matcolor",				&itemDef_matColor			},
	{ "matscalex",				&itemDef_matScaleX			},
	{ "matscaley",				&itemDef_matScaleY			},

	// Defines
	{ "definefloat",			&itemDef_defineFloat		},
	{ "definevec",				&itemDef_defineVec			},

	// Registers
	{ "modal",					&itemDef_modal				},
	{ "noevents",				&itemDef_noEvents			},
	{ "notime",					&itemDef_noTime				},
	{ "visible",				&itemDef_visible			},
	{ "wantenter",				&itemDef_wantEnter			},

	// Events
	{ "onaction",				&itemDef_onAction			},
	{ "onesc",					&itemDef_onEsc				},
	{ "onframe",				&itemDef_onFrame			},
	{ "oninit",					&itemDef_onInit				},
	{ "onmouseenter",			&itemDef_onMouseEnter		},
	{ "onmouseexit",			&itemDef_onMouseExit		},
	{ "onnamedevent",			&itemDef_onNamedEvent		},
	{ "ontime",					&itemDef_onTime				},

	{ NULL,						NULL						}
};

/*
=============================================================================

	BINDDEF PARSING

=============================================================================
*/

/*
==================
bindDef_bind
==================
*/
static qBool bindDef_bind (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char		*str;
	keyNum_t	keyNum;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}
	
	keyNum = Key_StringToKeynum (str);
	if (keyNum == -1) {
		Com_Printf (0, "\"%s\" isn't a valid key\n", str);
		return qFalse;
	}

	gui->s.bindDef->keyNum = keyNum;
	return qTrue;
}

// ==========================================================================

static guiParseKey_t	cl_bindDefKeyList[] = {
	{ "bind",					&bindDef_bind				},
	{ NULL,						NULL						}
};

/*
=============================================================================

	CHECKDEF PARSING

=============================================================================
*/

/*
==================
checkDef_liveUpdate
==================
*/
static qBool checkDef_liveUpdate (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	liveUpdate;

	if (!PS_ParseDataType (ps, 0, PSDT_BOOLEAN, &liveUpdate, 1)) {
		GUI_DevPrintPos (PRNT_WARNING, ps, fileName, gui);
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		liveUpdate = qTrue;
	}

	gui->s.checkDef->liveUpdate = liveUpdate;
	return qTrue;
}


/*
==================
checkDef_offMat
==================
*/
static qBool checkDef_offMat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Com_NormalizePath (gui->s.checkDef->offMatName, sizeof(gui->s.checkDef->offMatName), str);
	return qTrue;
}


/*
==================
checkDef_onMat
==================
*/
static qBool checkDef_onMat (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Com_NormalizePath (gui->s.checkDef->onMatName, sizeof(gui->s.checkDef->onMatName), str);
	return qTrue;
}


/*
==================
checkDef_values
==================
*/
static qBool checkDef_values (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;
	char	*p;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Only has two values
	p = strchr (str, ';');
	if (!*p) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}
	*p = '\0';
	gui->s.checkDef->values[0] = GUI_StrDup (str, GUITAG_SCRATCH);

	// Second value
	p++;
	if (!*p) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}
	gui->s.checkDef->values[1] = GUI_StrDup (p, GUITAG_SCRATCH);

	return qTrue;
}


/*
==================
checkDef_cvar
==================
*/
static qBool checkDef_cvar (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	if (!GUI_CvarValidate (str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid cvar name for '%s'\n", keyName);
		return qFalse;
	}

	gui->s.checkDef->cvar = Cvar_Exists (str);
	if (!gui->s.checkDef->cvar) {
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: cvar '%s' does not exist, creating\n", str);
		gui->s.checkDef->cvar = Cvar_Register (str, "0", 0);
	}

	return qTrue;
}

// ==========================================================================

static guiParseKey_t	cl_checkDefKeyList[] = {
	{ "liveupdate",				&checkDef_liveUpdate		},
	{ "offmat",					&checkDef_offMat			},
	{ "onmat",					&checkDef_onMat				},
	{ "values",					&checkDef_values			},
	{ "cvar",					&checkDef_cvar				},
	{ NULL,						&checkDef_cvar				}
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

/*
==================
listDef_scrollBar
==================
*/
static qBool listDef_scrollBar (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	if (!PS_ParseDataType (ps, 0, PSDT_BOOLEAN, gui->s.listDef->scrollBar, 2)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	return qTrue;
}

// ==========================================================================

static guiParseKey_t	cl_listDefKeyList[] = {
	{ "scrollbar",				&listDef_scrollBar			},
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
	{ "thumbmaterial",			NULL						},
	{ "cvar",					NULL						},
	{ NULL,						NULL						}
};

/*
=============================================================================

	TEXTDEF PARSING

=============================================================================
*/

/*
==================
textDef_font
==================
*/
static qBool textDef_font (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*str;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Com_NormalizePath (gui->s.textDef->fontName, sizeof (gui->s.textDef->fontName), str);
	return qTrue;
}


/*
==================
textDef_text
==================
*/
static qBool textDef_text (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	char	*token;
	size_t	len;

	if (!PS_ParseToken (ps, PSF_CONVERT_NEWLINE, &token)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	// Check length
	len = strlen (token);
	if (len >= MAX_TEXTDEF_STRLEN-1) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: parameter too long for '%s'\n", keyName);
		return qFalse;
	}

	gui->s.textDef->textString = GUI_StrDup (token, GUITAG_SCRATCH);
	gui->s.textDef->textStringLen = len;
	return qTrue;
}

static qBool textDef_textAlign (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_TEXT_ALIGN], qTrue, 1);
}
static qBool textDef_textColor (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseVectorRegister (fileName, gui, ps, keyName, &gui->s.vecRegisters[VR_TEXT_COLOR]);
}
static qBool textDef_textHoverColor (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseVectorRegister (fileName, gui, ps, keyName, &gui->s.vecRegisters[VR_TEXT_HOVERCOLOR]);
}
static qBool textDef_textScale (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_TEXT_SCALE], qTrue, 1);
}
static qBool textDef_textShadow (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	return GUI_ParseFloatRegister (fileName, gui, ps, keyName, &gui->s.floatRegisters[FR_TEXT_SHADOW], qFalse, 1);
}

// ==========================================================================

static guiParseKey_t	cl_textDefKeyList[] = {
	{ "font",					&textDef_font				},
	{ "text",					&textDef_text				},
	{ "textalign",				&textDef_textAlign			},
	{ "textcolor",				&textDef_textColor			},
	{ "texthovercolor",			&textDef_textHoverColor		},
	{ "textscale",				&textDef_textScale			},
	{ "textshadow",				&textDef_textShadow			},
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

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &str)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Q_strncpyz (gui->shared->cursor.s.matName, str, sizeof (gui->shared->cursor.s.matName));
	gui->shared->cursor.s.visible = qTrue;
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

	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &color[0], 4)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	ColorNormalizef (color, gui->shared->cursor.s.color);
	gui->shared->cursor.s.color[3] = color[3];
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

	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &height, 1)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->shared->cursor.s.size[1] = height;
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

	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &width, 1)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	gui->shared->cursor.s.size[0] = width;
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

	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &pos[0], 2)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: invalid/missing parameters for '%s'\n", keyName);
		return qFalse;
	}

	Vec2Copy (pos, gui->shared->cursor.s.pos);
	return qTrue;
}


/*
==================
guiDef_cursorVisible
==================
*/
static qBool guiDef_cursorVisible (char *fileName, gui_t *gui, parse_t *ps, char *keyName)
{
	qBool	visible;

	if (!PS_ParseDataType (ps, 0, PSDT_BOOLEAN, &visible, 1)) {
		GUI_DevPrintPos (PRNT_WARNING, ps, fileName, gui);
		GUI_DevPrintf (PRNT_WARNING, ps, fileName, gui, "WARNING: missing '%s' paramter(s), using default\n");
		visible = qTrue;
	}

	gui->shared->cursor.s.visible = visible;
	return qTrue;
}


static guiParseKey_t	cl_guiDefKeyList[] = {
	{ "cursormat",				&guiDef_cursorMat			},
	{ "cursorcolor",			&guiDef_cursorColor			},
	{ "cursorheight",			&guiDef_cursorHeight		},
	{ "cursorwidth",			&guiDef_cursorWidth			},
	{ "cursorpos",				&guiDef_cursorPos			},
	{ "cursorvisible",			&guiDef_cursorVisible		},
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
	{ "checkdef",				&GUI_NewWindowDef			},
	{ "choicedef",				&GUI_NewWindowDef			},
	{ "editdef",				&GUI_NewWindowDef			},
	{ "listdef",				&GUI_NewWindowDef			},
	{ "renderdef",				&GUI_NewWindowDef			},
	{ "sliderdef",				&GUI_NewWindowDef			},
	{ "textdef",				&GUI_NewWindowDef			},
	{ NULL,						NULL						}
};

static const char		*cl_windowDefTypes[] = {
	"guidef",		// WTP_GUI,
	"windowdef",	// WTP_GENERIC,

	"binddef",		// WTP_BIND,
	"checkdef",		// WTP_CHECKBOX,
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
	char			windowName[MAX_GUI_NAMELEN];
	gui_t			*newGUI, *owner;
	char			*token;
	guiType_t		type;
	gui_t			*childList;
	event_t			*eventList;
	defineFloat_t	*defFloatList;
	defineVec_t		*defVecList;

	// First is the name
	if (!PS_ParseToken (ps, PSF_TO_LOWER, &token)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: expecting <name> after %s, got '%s'!\n", keyName, token);
		return qFalse;
	}
	if (strlen(token)+1 >= MAX_GUI_NAMELEN) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name too long!\n", keyName, token);
		return qFalse;
	}
	Q_strncpyz (windowName, token, sizeof (windowName));

	// Check for duplicates
	if (gui && GUI_FindWindow (gui->owner, windowName)) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name already in use!\n", keyName, token);
		return qFalse;
	}

	// Make sure the name is allowed
	if (!strcmp (windowName, "guivar")) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name not allowed!\n", keyName, token);
		return qFalse;
	}
	if (strchr (windowName, '$') || strstr (windowName, "::")) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: %s '%s' name must not contain '$' or '::'!\n", keyName, token);
		return qFalse;
	}

	// Next is the opening brace
	if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) || strcmp (token, "{")) {
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
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
		if (gui->numChildren+1 >= MAX_GUI_CHILDREN) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: too many children!\n");
			return qFalse;
		}
		newGUI = &gui->childList[gui->numChildren];
		newGUI->parent = gui;
		newGUI->shared = gui->shared;
		Q_strncpyz (newGUI->name, windowName, sizeof (newGUI->name));

		// Find the owner
		for (owner=gui ; owner ; owner=owner->parent)
			newGUI->owner = owner;

		// Increment parent children
		gui->numChildren++;
	}
	else {
		// This is a parent
		if (cl_numGUI+1 >= MAX_GUIS) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: too many GUIs!\n");
			return qFalse;
		}
		newGUI = &cl_guiList[cl_numGUI];
		memset (&cl_guiList[cl_numGUI], 0, sizeof (gui_t));
		Q_strncpyz (newGUI->name, windowName, sizeof (newGUI->name));

		newGUI->shared = &cl_guiSharedInfo[cl_numGUI];
		memset (&cl_guiSharedInfo[cl_numGUI], 0, sizeof (guiShared_t));
		Vec4Set (newGUI->shared->cursor.s.color, 1, 1, 1, 1);
		newGUI->shared->cursor.s.visible = qTrue;

		newGUI->owner = newGUI;

		cl_numGUI++;
	}

	// Allocate space and set default values
	newGUI->type = type;
	Vec4Set (newGUI->s.vecRegisters[VR_MAT_COLOR].storage, 1, 1, 1, 1);
	newGUI->s.floatRegisters[FR_MAT_SCALE_X].storage = 1;
	newGUI->s.floatRegisters[FR_MAT_SCALE_Y].storage = 1;
	newGUI->s.floatRegisters[FR_VISIBLE].storage = 1;

	switch (type) {
	case WTP_GUI:
		break;

	case WTP_GENERIC:
		break;

	case WTP_BIND:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.bindDef = GUI_AllocTag (sizeof (bindDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.bindDef = newGUI->s.bindDef + 1;
		break;

	case WTP_CHECKBOX:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.checkDef = GUI_AllocTag (sizeof (checkDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.checkDef = newGUI->s.checkDef + 1;

		Q_strncpyz (newGUI->s.checkDef->offMatName, "guis/assets/textures/items/check_off.tga", sizeof (newGUI->s.checkDef->offMatName));
		Q_strncpyz (newGUI->s.checkDef->onMatName, "guis/assets/textures/items/check_on.tga", sizeof (newGUI->s.checkDef->onMatName));
		break;

	case WTP_CHOICE:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.choiceDef = GUI_AllocTag (sizeof (choiceDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.choiceDef = newGUI->s.choiceDef + 1;
		break;

	case WTP_EDIT:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.editDef = GUI_AllocTag (sizeof (editDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.editDef = newGUI->s.editDef + 1;
		break;

	case WTP_LIST:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.listDef = GUI_AllocTag (sizeof (listDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.listDef = newGUI->s.listDef + 1;

		newGUI->s.listDef->scrollBar[0] = qTrue;
		newGUI->s.listDef->scrollBar[1] = qTrue;
		break;

	case WTP_RENDER:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.renderDef = GUI_AllocTag (sizeof (renderDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.renderDef = newGUI->s.renderDef + 1;
		break;

	case WTP_SLIDER:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.sliderDef = GUI_AllocTag (sizeof (sliderDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.sliderDef = newGUI->s.sliderDef + 1;
		break;

	case WTP_TEXT:
		newGUI->flags |= WFL_ITEM;
		newGUI->s.textDef = GUI_AllocTag (sizeof (textDef_t) * 2, GUITAG_SCRATCH);
		newGUI->d.textDef = newGUI->s.textDef + 1;

		Q_strncpyz (newGUI->s.textDef->fontName, "default", sizeof (newGUI->s.textDef->fontName));
		Vec4Set (newGUI->s.vecRegisters[VR_TEXT_COLOR].storage, 1, 1, 1, 1);
		newGUI->s.floatRegisters[FR_TEXT_SCALE].storage = 1;
		break;
	}

	// Storage space for children
	newGUI->numChildren = 0;
	newGUI->childList = GUI_AllocTag (sizeof (gui_t) * MAX_GUI_CHILDREN, GUITAG_SCRATCH);
	newGUI->numEvents = 0;
	newGUI->eventList = GUI_AllocTag (sizeof (event_t) * MAX_GUI_EVENTS, GUITAG_SCRATCH);
	newGUI->s.numDefFloats = 0;
	newGUI->s.defFloatList = GUI_AllocTag (sizeof (defineFloat_t) * MAX_GUI_DEFINES, GUITAG_SCRATCH);
	newGUI->s.numDefVecs = 0;
	newGUI->s.defVecList = GUI_AllocTag (sizeof (defineVec_t) * MAX_GUI_DEFINES, GUITAG_SCRATCH);

	// Parse the keys
	for ( ; ; ) {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) || !strcmp (token, "}"))
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
		case WTP_CHECKBOX:
			if (!GUI_CallKeyFunc (fileName, newGUI, ps, cl_itemDefKeyList, cl_checkDefKeyList, NULL, token))
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
		GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
		GUI_PrintError ("ERROR: expecting '}' after %s, got '%s'!\n", keyName, token);
		return qFalse;
	}

	// Check for required values
	switch (type) {
	case WTP_CHECKBOX:
		if (!newGUI->s.checkDef->cvar) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: missing required 'cvar' value!\n");
			return qFalse;
		}
		if (!newGUI->s.checkDef->values[0] || !newGUI->s.checkDef->values[1]) {
			GUI_PrintPos (PRNT_ERROR, ps, fileName, gui);
			GUI_PrintError ("ERROR: missing required 'values' value!\n");
			return qFalse;
		}
		break;
	}

	// Store children
	childList = newGUI->childList;
	if (newGUI->numChildren) {
		newGUI->childList = GUI_AllocTag (sizeof (gui_t) * newGUI->numChildren, GUITAG_SCRATCH);
		memcpy (newGUI->childList, childList, sizeof (gui_t) * newGUI->numChildren);
	}
	else
		newGUI->childList = NULL;
	Mem_Free (childList);

	// Store events
	eventList = newGUI->eventList;
	if (newGUI->numEvents) {
		newGUI->eventList = GUI_AllocTag (sizeof (event_t) * newGUI->numEvents, GUITAG_SCRATCH);
		memcpy (newGUI->eventList, eventList, sizeof (event_t) * newGUI->numEvents);
	}
	else
		newGUI->eventList = NULL;
	Mem_Free (eventList);

	// Store floats
	defFloatList = newGUI->s.defFloatList;
	if (newGUI->s.numDefFloats) {
		newGUI->s.defFloatList = GUI_AllocTag (sizeof (defineFloat_t) * newGUI->s.numDefFloats, GUITAG_SCRATCH);
		memcpy (newGUI->s.defFloatList, defFloatList, sizeof (defineFloat_t) * newGUI->s.numDefFloats);

		newGUI->d.defFloatList = GUI_AllocTag (sizeof (defineFloat_t) * newGUI->s.numDefFloats, GUITAG_SCRATCH);
		memcpy (newGUI->d.defFloatList, defFloatList, sizeof (defineFloat_t) * newGUI->d.numDefFloats);
	}
	else {
		newGUI->s.defFloatList = NULL;
		newGUI->d.defFloatList = NULL;
	}
	Mem_Free (defFloatList);

	// Store vecs
	defVecList = newGUI->s.defVecList;
	if (newGUI->s.numDefVecs) {
		newGUI->s.defVecList = GUI_AllocTag (sizeof (defineVec_t) * newGUI->s.numDefVecs, GUITAG_SCRATCH);
		memcpy (newGUI->s.defVecList, defVecList, sizeof (defineVec_t) * newGUI->s.numDefVecs);

		newGUI->d.defVecList = GUI_AllocTag (sizeof (defineVec_t) * newGUI->s.numDefVecs, GUITAG_SCRATCH);
		memcpy (newGUI->d.defVecList, defVecList, sizeof (defineVec_t) * newGUI->d.numDefVecs);
	}
	else {
		newGUI->s.defVecList = NULL;
		newGUI->d.defVecList = NULL;
	}
	Mem_Free (defVecList);

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
	char	keyName[MAX_PS_TOKCHARS];
	char	*buf;
	char	*token;
	int		fileLen;
	parse_t	*ps;

	if (!fixedName)
		return;

	// Load the file
	fileLen = FS_LoadFile (fixedName, (void **)&buf, "\n\0"); 
	if (!buf || fileLen <= 0) {
		GUI_PrintWarning ("WARNING: can't load '%s' -- %s\n", fixedName, (fileLen == -1) ? "not found" : "empty file");
		return;
	}

	Com_Printf (0, "...loading '%s'\n", fixedName);

	// Start parsing
	ps = PS_StartSession (buf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE);

	// Parse the script
	for ( ; ; ) {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token))
			break;

		// Search for a guiDef
		if (strcmp (token, "guidef")) {
			GUI_PrintPos (PRNT_ERROR, ps, fixedName, NULL);
			GUI_PrintError ("ERROR: expecting 'guiDef' got '%s'!\n", token);
			GUI_PrintError ("ERROR: removing GUI and halting on file due to errors!\n");
			break;
		}

		// Found one, create it
		Q_strncpyz (keyName, token, sizeof (keyName));
		if (GUI_NewWindowDef (fixedName, NULL, ps, keyName)) {
			// It parsed ok, store the pathType
			cl_guiSharedInfo[cl_numGUI-1].pathType = pathType;
			Mem_ChangeTag (cl_guiSysPool, GUITAG_SCRATCH, GUITAG_KEEP);
		}
		else {
			// Failed to parse, halt!
			GUI_PrintError ("ERROR: removing GUI and halting on file due to errors!\n");
			memset (&cl_guiList[--cl_numGUI], 0, sizeof (gui_t));
			break;
		}
	}

	// Done
	PS_AddErrorCount (ps, &cl_numGUIErrors, &cl_numGUIWarnings);
	PS_EndSession (ps);
	FS_FreeFile (buf);
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
		GUI_PrintWarning ("WARNING: GUI '%s': can't find %s '%s'\n", gui->name, item, name);
		return;
	}

	GUI_PrintWarning ("WARNING: can't find %s '%s'\n", item, name);
}
static void GUI_RegisterFloats (floatRegister_t *floats, int maxFloats)
{
	int		i;

	for (i=0 ; i<maxFloats ; i++) {
		switch (floats[i].sourceType) {
		case REG_SOURCE_SELF:
			floats[i].var = &floats[i].storage;
			break;
		case REG_SOURCE_DEF:
			floats[i].var = &floats[i].defFloatWindow->d.defFloatList[floats[i].defFloatIndex].value;
			break;
		case REG_SOURCE_GUIVAR:
			floats[i].var = &floats[i].guiVar->floatVal;
			break;
		default:
			assert (0);
			break;
		}
	}
}
static void GUI_RegisterVecs (vecRegister_t *vecs, int maxVecs)
{
	int		i;

	for (i=0 ; i<maxVecs ; i++) {
		switch (vecs[i].sourceType) {
		case REG_SOURCE_SELF:
			vecs[i].var = &vecs[i].storage[0];
			break;
		case REG_SOURCE_DEF:
			vecs[i].var = &vecs[i].defVecWindow->d.defVecList[vecs[i].defVecIndex].value[0];
			break;
		case REG_SOURCE_GUIVAR:
			vecs[i].var = &vecs[i].guiVar->vecVal[0];
			break;
		default:
			assert (0);
			break;
		}
	}
}
static void GUI_r_TouchGUI (gui_t *gui)
{
	gui_t		*child;
	event_t		*event;
	evAction_t	*action;
	int			i, j;
	short		index = -1;

	// Load generic media
	if (gui->flags & WFL_MATERIAL) {
		gui->matPtr = R_RegisterPic (gui->matName);
		if (!gui->matPtr)
			GUI_CantFindMedia (gui, "Material", gui->matName);
	}

	GUI_RegisterFloats (gui->s.floatRegisters, FR_MAX);
	GUI_RegisterVecs (gui->s.vecRegisters, VR_MAX);

	// Load type-specific media
	switch (gui->type) {
	case WTP_GUI:
		break;

	case WTP_GENERIC:
		break;

	case WTP_BIND:
		break;

	case WTP_CHECKBOX:
		// Find on/off materials
		gui->s.checkDef->offMatPtr = R_RegisterPic (gui->s.checkDef->offMatName);
		if (!gui->s.checkDef->offMatPtr) {
			GUI_CantFindMedia (gui, "offMat", gui->s.checkDef->offMatName);
			break;
		}
		gui->s.checkDef->onMatPtr = R_RegisterPic (gui->s.checkDef->onMatName);
		if (!gui->s.checkDef->onMatPtr) {
			GUI_CantFindMedia (gui, "onMat", gui->s.checkDef->onMatName);
			break;
		}
		break;

	case WTP_CHOICE:
		break;

	case WTP_EDIT:
		break;

	case WTP_LIST:
		break;

	case WTP_RENDER:
		break;

	case WTP_SLIDER:
		break;

	case WTP_TEXT:
		// Find the font
		gui->s.textDef->fontPtr = R_RegisterFont (gui->s.textDef->fontName);
		if (!gui->s.textDef->fontPtr)
			GUI_CantFindMedia (gui, "Font", gui->s.textDef->fontName);
		break;
	}

	// Load event-action specific media
	for (i=0, event=gui->eventList ; i<gui->numEvents ; event++, i++) {
		for (j=0, action=event->actionList ; j<event->numActions ; action++, j++) {
			switch (action->type) {
			case EVA_CLOSE:
				break;

			case EVA_COMMAND:
				break;

			case EVA_IF:
				break;

			case EVA_LOCAL_SOUND:
				// Find the sound
				action->localSound->sound = Snd_RegisterSound (action->localSound->name);
				if (!action->localSound->sound)
					GUI_CantFindMedia (gui, "Sound", action->localSound->name);
				break;

			case EVA_NAMED_EVENT:
				// Find the window
				action->named->destWindowPtr = GUI_FindWindow (gui->owner, action->named->destWindowName);
				if (!action->named->destWindowPtr) {
					GUI_CantFindMedia (gui, "Window", action->named->destWindowName);
					break;
				}
				break;

			case EVA_RESET_TIME:
				break;

			case EVA_SET:
				// Find the destination window
				action->set->destWindowPtr = GUI_FindWindow (gui->owner, action->set->destWindowName);
				if (!action->set->destWindowPtr) {
					GUI_CantFindMedia (gui, "Window", action->set->destWindowName);
					break;
				}

				// Check if it's allowed
				if (action->set->destWindowType != WTP_MAX && action->set->destWindowPtr->type != action->set->destWindowType)  {
					GUI_PrintError ("ERROR: Invalid set action destination '%s::%s'!\n", action->set->destWindowPtr->name, action->set->destVarName);
					action->set->destWindowPtr = NULL;
					break;
				}

				switch (action->set->srcType) {
				case EVA_SETSRC_DEF:
					// Find the source window
					action->set->srcWindowPtr = GUI_FindWindow (gui->owner, action->set->srcWindowName);
					if (!action->set->srcWindowPtr) {
						GUI_CantFindMedia (gui, "Window", action->set->srcWindowName);
						break;
					}

					// Find the source defineFloat/defineVec
					if (action->set->destType & EVA_SETDEST_FLOAT)
						index = GUI_FindDefFloat (action->set->srcWindowPtr, action->set->srcName);
					else if (action->set->destType & EVA_SETDEST_VEC)
						index = GUI_FindDefVec (action->set->srcWindowPtr, action->set->srcName);
					if (index == -1) {
						GUI_CantFindMedia (gui, "Define", action->set->srcName);
						break;
					}
					break;

				case EVA_SETSRC_GUIVAR:
					// Register the GUIVar
					if (action->set->destType & EVA_SETDEST_FLOAT)
						action->set->srcGUIVar = GUIVar_Register (action->set->srcName, GVT_FLOAT);
					else if (action->set->destType & EVA_SETDEST_VEC)
						action->set->srcGUIVar = GUIVar_Register (action->set->srcName, GVT_VEC);
					break;
				}
				break;

			case EVA_STOP_TRANSITIONS:
				break;

			case EVA_TRANSITION:
				break;
			}
		}
	}

	// Recurse down the children
	for (i=0, child=gui->childList ; i<gui->numChildren ; child++, i++)
		GUI_r_TouchGUI (child);
}
static void GUI_TouchGUI (gui_t *gui)
{
	// Register cursor media
	if (gui->flags & WFL_CURSOR) {
		gui->shared->cursor.s.matPtr = R_RegisterPic (gui->shared->cursor.s.matName);
		if (!gui->shared->cursor.s.matPtr)
			GUI_CantFindMedia (gui, "cursorMat", gui->shared->cursor.s.matName);
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
GUI_RegisterSounds

On snd_restart this will be called.
==================
*/
void GUI_RegisterSounds (void)
{
	// FIXME: k just register sounds again
	GUI_EndRegistration ();
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
	gui_t	*gui;
	uint32	i;

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
static void *cmd_gui_namedEvent;
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
GUI_NamedEvent_f
==================
*/
static void GUI_NamedEvent_f (void)
{
	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: gui_namedEvent <event name>\n");
		return;
	}

	GUI_NamedGlobalEvent (Cmd_Argv (1));
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
	size_t			numGUI, i;
	guiPathType_t	pathType;
	char			*name;
	uint32			initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n---------- GUI Initialization ----------\n");

	// Clear lists
	cl_numGUI = 0;
	memset (&cl_guiList[0], 0, sizeof (gui_t) * MAX_GUIS);
	memset (&cl_guiSharedInfo[0], 0, sizeof (guiShared_t) * MAX_GUIS);

	// Console commands
	cmd_gui_list		= Cmd_AddCommand ("gui_list",		GUI_List_f,			"List GUIs in memory");
	cmd_gui_namedEvent	= Cmd_AddCommand ("gui_namedEvent",	GUI_NamedEvent_f,	"Triggers a named event on all open GUIs (for debugging)");
	cmd_gui_restart		= Cmd_AddCommand ("gui_restart",	GUI_Restart_f,		"Restart the GUI system");
	cmd_gui_test		= Cmd_AddCommand ("gui_test",		GUI_Test_f,			"Test the specified GUI");

	// Cvars
	gui_developer			= Cvar_Register ("gui_developer",				"0",		0);
	gui_debugBounds			= Cvar_Register ("gui_debugBounds",				"0",		0);
	gui_debugScale			= Cvar_Register ("gui_debugScale",				"0",		0);
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

	// Var init
	GUIVar_Init ();

	Com_Printf (0, "----------------------------------------\n");

	// Check memory integrity
	Mem_CheckPoolIntegrity (cl_guiSysPool);

	Com_Printf (0, "GUI - %i error(s), %i warning(s)\n", cl_numGUIErrors, cl_numGUIWarnings);
	Com_Printf (0, "%u GUIs loaded in %ums\n", cl_numGUI, Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
GUI_Shutdown

Only called by gui_restart (for development purposes generally).
==================
*/
void GUI_Shutdown (void)
{
	// Close all open GUIs
	GUI_CloseAllGUIs ();

	// Shutdown GUI vars
	GUIVar_Shutdown ();

	// Remove commands
	Cmd_RemoveCommand ("gui_list", cmd_gui_list);
	Cmd_RemoveCommand ("gui_namedEvent", cmd_gui_namedEvent);
	Cmd_RemoveCommand ("gui_restart", cmd_gui_restart);
	Cmd_RemoveCommand ("gui_test", cmd_gui_test);

	// Release all used memory
	Mem_FreePool (cl_guiSysPool);
}
