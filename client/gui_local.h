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
// gui_local.h
//

#include "cl_local.h"

#define MAX_GUIS			512				// Maximum in-memory GUIs

#define MAX_GUI_CHILDREN	32				// Maximum children per child
#define MAX_GUI_DEFINES		32				// Maximum definefloat/definevec's
#define MAX_GUI_DEPTH		32				// Maximum opened at the same time
#define MAX_GUI_HASH		(MAX_GUIS/4)	// Hash optimization
#define MAX_GUI_NAMELEN		64				// Maximum window name length

#define MAX_GUI_EVENTS		16				// Maximum events per window
#define MAX_EVENT_ARGS		1
#define MAX_EVENT_ARGLEN	64
#define MAX_EVENT_ACTIONS	16				// Maximum actions per event per window

//
// Script path location
// This is so that moddir gui's of the same name
// have precedence over basedir ones
//
typedef enum guiPathType_s {
	GUIPT_BASEDIR,
	GUIPT_MODDIR,
} guiPathType_t;

//
// Window types
//
typedef enum guiType_s {
	WTP_GUI,
	WTP_GENERIC,

	WTP_BIND,
	WTP_CHECKBOX,
	WTP_CHOICE,
	WTP_EDIT,
	WTP_LIST,
	WTP_RENDER,
	WTP_SLIDER,
	WTP_TEXT,

	WTP_MAX
} guiType_t;

//
// Window flags
// FIXME: yyuucckk
//
typedef enum guiFlags_s {
	WFL_CURSOR			= 1 << 0,
	WFL_FILL_COLOR		= 1 << 1,
	WFL_MATERIAL		= 1 << 2,
	WFL_ITEM			= 1 << 3,
} guiFlags_t;

/*
=============================================================================

	MEMORY MANAGEMENT

=============================================================================
*/

enum {
	GUITAG_KEEP,			// What's kept in memory at all times
	GUITAG_SCRATCH,			// Scratch tag for init. When a GUI is complete it's tag is shifted to GUITAG_KEEP
	GUITAG_VARS,			// for GUI vars
};

#define GUI_AllocTag(size,zeroFill,tagNum)	_Mem_Alloc((size),(zeroFill),cl_guiSysPool,(tagNum),__FILE__,__LINE__)
#define GUI_FreeTag(tagNum)					_Mem_FreeTag(cl_guiSysPool,(tagNum),__FILE__,__LINE__)
#define GUI_StrDup(in,tagNum)				_Mem_PoolStrDup((in),cl_guiSysPool,(tagNum),__FILE__,__LINE__)
#define GUI_MemFree(ptr)					_Mem_Free((ptr),__FILE__,__LINE__)

/*
=============================================================================

	GUI VARIABLES

=============================================================================
*/

#define MAX_GUIVARS			1024
#define MAX_GV_NAMELEN		64
#define MAX_GV_STRLEN		1024	// Maximum GVT_STR length

typedef struct guiVar_s {
	char				*name;
	guiVarType_t		type;

	qBool				modified;

	char				*strVal;
	float				floatVal;
	vec4_t				vecVal;
} guiVar_t;

void		GUIVar_Init (void);
void		GUIVar_Shutdown (void);

/*
=============================================================================

	GUI CURSOR

	Each GUI can script their cursor setup, and modify (with events)
	the properties.
=============================================================================
*/

typedef struct guiCursorData_s {
	qBool				visible;

	char				matName[MAX_QPATH];
	struct shader_s		*matPtr;
	vec4_t				color;

	qBool				locked;
	vec2_t				pos;
	vec2_t				size;
} guiCursorData_t;

typedef struct guiCursor_s {
	// Static (compiled) information
	guiCursorData_t		s;

	// Dynamic information
	guiCursorData_t		d;

	struct gui_s		*curWindow;
	qBool				mouseMoved;
} guiCursor_t;

/*
=============================================================================

	GUI SHARED DATA

	This data only exists once in any given GUI, and is just pointed to by
	all of the children. It's used for things that do not need to be in
	every single window.
=============================================================================
*/

typedef struct guiShared_s {
	guiPathType_t		pathType;
	guiCursor_t			cursor;

	qBool				queueClose;

	float				xScale;
	float				yScale;
} guiShared_t;

/*
=============================================================================

	WINDOW DEFINES

	These can be changed at any time in the script using event actions.
=============================================================================
*/

typedef struct defineFloat_s {
	char				name[MAX_GUI_NAMELEN];
	float				value;
} defineFloat_t;

typedef struct defineVec_s {
	char				name[MAX_GUI_NAMELEN];
	vec4_t				value;
} defineVec_t;

/*
=============================================================================

	EVENT ACTIONS

=============================================================================
*/

typedef enum evaType_s {
	EVA_NONE,

	EVA_CLOSE,
	EVA_COMMAND,
	EVA_IF,
	EVA_LOCAL_SOUND,
	EVA_NAMED_EVENT,
	EVA_RESET_TIME,
	EVA_SET,
	EVA_STOP_TRANSITIONS,
	EVA_TRANSITION,

	EVA_MAX
} evaType_t;

//
// EVA_LOCAL_SOUND
//

typedef struct eva_localSound_s {
	char				name[MAX_QPATH];
	struct sfx_s		*sound;
	float				volume;
} eva_localSound_t;

//
// EVA_NAMED_EVENT
//

typedef struct eva_named_s {
	char				destWindowName[MAX_GUI_NAMELEN];
	struct gui_s		*destWindowPtr;

	char				eventName[MAX_GUI_NAMELEN];
} eva_named_t;

//
// EVA_SET
//
typedef enum set_destType_s {
	EVA_SETDEST_FLOAT	= 1 << 0,
	EVA_SETDEST_VEC		= 1 << 1,

	// Or'd with what's above
	EVA_SETDEST_STORAGE	= 1 << 2,
	EVA_SETDEST_DEF		= 1 << 3
} set_destType_t;

typedef enum set_srcType_s {
	EVA_SETSRC_STORAGE,
	EVA_SETSRC_DEF,
	EVA_SETSRC_GUIVAR,
} set_srcType_t;

typedef struct eva_set_s {
	// Destination
	char				destWindowName[MAX_GUI_NAMELEN];
	struct gui_s		*destWindowPtr;
	guiType_t			destWindowType;

	char				destVarName[MAX_GUI_NAMELEN];

	set_destType_t		destType;

	// EVA_SETDEST_DEF
	byte				destDefIndex;

	// EVA_SETDEST_STORAGE
	uint32				destRegister;
	byte				destNumVecs;

	// Source
	set_srcType_t		srcType;

	// EVA_SETSRC_STORAGE
	vec4_t				srcStorage;

	// EVA_SETSRC_DEF
	char				srcWindowName[MAX_GUI_NAMELEN];
	struct gui_s		*srcWindowPtr;

	char				srcName[MAX_GUI_NAMELEN];
	defineFloat_t		*srcDefFloat;
	defineVec_t			*srcDefVec;

	// EVA_SETSRC_GUIVAR
	guiVar_t			*srcGUIVar;
} eva_set_t;

//
// Action handling
//
typedef struct evAction_s {
	evaType_t			type;

	// EVA_CLOSE
	// EVA_COMMAND
	char				*command;

	// EVA_IF
	// EVA_LOCAL_SOUND
	eva_localSound_t	*localSound;

	// EVA_NAMED_EVENT
	eva_named_t			*named;

	// EVA_RESET_TIME
	int					resetTime;

	// EVA_SET
	eva_set_t			*set;

	// EVA_STOP_TRANSITIONS
	// EVA_TRANSITION
} evAction_t;

/*
=============================================================================

	WINDOW EVENTS

=============================================================================
*/

typedef enum evType_s {
	WEV_NONE,

	WEV_ACTION,
	WEV_ESCAPE,
	WEV_FRAME,
	WEV_INIT,
	WEV_MOUSE_ENTER,
	WEV_MOUSE_EXIT,
	WEV_NAMED,
	WEV_SHUTDOWN,
	WEV_TIME,

	WEV_MAX
} evType_t;

typedef struct event_s {
	evType_t			type;

	byte				numActions;
	evAction_t			*actionList;

	// WEV_NAMED
	char				*named;

	// WEV_TIME
	uint32				onTime;
} event_t;

/*
=============================================================================

	WINDOW REGISTERS

	These can be changed at any time in the script using event actions.
=============================================================================
*/

typedef enum regSource_s {
	REG_SOURCE_SELF,
	REG_SOURCE_DEF,
	REG_SOURCE_GUIVAR,

	REG_SOURCE_MAX
} regSource_t;

//
// Vector registers
//
#define VRVALUE(gui,reg) ((gui)->d.vecRegisters[(reg)].var)
enum {
	// textDef windows
	VR_TEXT_COLOR,
	VR_TEXT_HOVERCOLOR,

	// ALL windows
	VR_FILL_COLOR,
	VR_MAT_COLOR,
	VR_RECT,

	VR_MAX,
};

typedef struct vecRegister_s {
	regSource_t			sourceType;
	float				*var;

	// REG_SOURCE_SELF
	vec4_t				storage;

	// REG_SOURCE_DEF
	struct gui_s		*defVecWindow;
	int					defVecIndex;

	// REG_SOURCE_GUIVAR
	struct guiVar_s		*guiVar;
} vecRegister_t;

//
// Float registers
//
#define FRVALUE(gui,reg) (*((gui)->d.floatRegisters[(reg)].var))

enum {
	// textDef windows
	FR_TEXT_ALIGN,
	FR_TEXT_SCALE,
	FR_TEXT_SHADOW,

	// ALL windows
	FR_MAT_SCALE_X,
	FR_MAT_SCALE_Y,

	FR_MODAL,
	FR_NO_EVENTS,
	FR_NO_TIME,
	FR_ROTATION,		// FIXME: Todo
	FR_VISIBLE,
	FR_WANT_ENTER,

	FR_MAX
};

typedef struct floatRegister_s {
	regSource_t			sourceType;			// storage, defFloat, or guiVar
	float				*var;

	// REG_SOURCE_SELF
	float				storage;

	// REG_SOURCE_DEF
	struct gui_s		*defFloatWindow;
	int					defFloatIndex;

	// REG_SOURCE_GUIVAR
	struct guiVar_s		*guiVar;
} floatRegister_t;

/*
=============================================================================

	WINDOW STRUCTURES

=============================================================================
*/

//
// bindDef
//
typedef struct bindDef_s {
	keyNum_t			keyNum;
} bindDef_t;

//
// checkDef
//
typedef struct checkDef_s {
	qBool				liveUpdate;

	char				offMatName[MAX_QPATH];
	struct shader_s		*offMatPtr;

	char				onMatName[MAX_QPATH];
	struct shader_s		*onMatPtr;

	char				*values[2];

	cVar_t				*cvar;
} checkDef_t;

//
// choiceDef
//
typedef struct choiceDef_s {
	int					todo;
} choiceDef_t;

//
// editDef
//
typedef struct editDef_s {
	int					todo;
} editDef_t;

//
// listDef
//
typedef struct listDef_s {
	qBool				scrollBar[2];
} listDef_t;

//
// renderDef
//
typedef struct renderDef_s {
	int					todo;
} renderDef_t;

//
// sliderDef
//
typedef struct sliderDef_s {
	int					todo;
} sliderDef_t;

//
// textDef
//
#define MAX_TEXTDEF_STRLEN		1024

typedef struct textDef_s {
	char				fontName[MAX_QPATH];
	struct font_s		*fontPtr;

	char				*textString;
	size_t				textStringLen;
} textDef_t;

//
// Generic window data
//
typedef struct guiData_s {
	byte				numDefFloats;
	defineFloat_t		*defFloatList;
	byte				numDefVecs;
	defineVec_t			*defVecList;

	floatRegister_t		floatRegisters[FR_MAX];
	vecRegister_t		vecRegisters[VR_MAX];

	bindDef_t			*bindDef;
	checkDef_t			*checkDef;
	choiceDef_t			*choiceDef;
	editDef_t			*editDef;
	listDef_t			*listDef;
	renderDef_t			*renderDef;
	sliderDef_t			*sliderDef;
	textDef_t			*textDef;
} guiData_t;

typedef struct gui_s {
	// Static (compiled) information
	char				name[MAX_GUI_NAMELEN];
	guiFlags_t			flags;
	guiType_t			type;

	guiShared_t			*shared;
	guiData_t			s;

	char				matName[MAX_QPATH];
	struct shader_s		*matShader;

	// Events
	byte				numEvents;
	event_t				*eventList;

	// Children
	struct gui_s		*owner;							// guiDef
	struct gui_s		*parent;						// Next window up

	byte				numChildren;
	struct gui_s		*childList;

	// Dynamic information
	guiData_t			d;

	vec4_t				rect;
	vec3_t				mins, maxs;

	uint32				lastTime;
	uint32				openTime;
	uint32				time;

	qBool				mouseEntered;
	qBool				mouseExited;

	qBool				inited;

	// Event queue
	byte				numQueued;
	event_t				*queueList[MAX_GUI_EVENTS];

	byte				numDefaultQueued;
	evType_t			defaultQueueList[MAX_GUI_EVENTS];
} gui_t;

/*
=============================================================================

	LOCAL GUI STATE

=============================================================================
*/

typedef struct guiState_s {
	uint32				frameCount;

	gui_t				*inputWindow;

	byte				numLayers;
	gui_t				*openLayers[MAX_GUI_DEPTH];
} guiState_t;

extern guiState_t	cl_guiState;

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*gui_developer;
extern cVar_t	*gui_debugBounds;
extern cVar_t	*gui_debugScale;
extern cVar_t	*gui_mouseFilter;
extern cVar_t	*gui_mouseSensitivity;

/*
=============================================================================

	FUNCTION PROTOTYPES

=============================================================================
*/

//
// gui_cursor.c
//
void		GUI_GenerateBounds (gui_t *gui);
void		GUI_CursorUpdate (gui_t *gui);
void		GUI_AdjustCursor (keyNum_t keyNum);

//
// gui_events.c
//
void		GUI_TriggerEvents (gui_t *gui);
void		GUI_QueueTrigger (gui_t *gui, evType_t trigger);

//
// gui_init.c
//

void		GUI_Shutdown (void);

//
// gui_main.c
//
void		GUI_ResetGUIState (gui_t *gui);
