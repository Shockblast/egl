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

#define GUI_VERSION			"EGLGUIv0.0.1"

#define MAX_GUIS			512			// Maximum in-memory GUIs

#define MAX_GUI_CHILDREN	32			// Maximum children per child
#define MAX_GUI_DEPTH		32			// Maximum opened at the same time
#define MAX_GUI_HASH		32			// Hash optimization
#define MAX_GUI_NAMELEN		MAX_QPATH	// Maximum window name length

#define MAX_GUI_EVENTS		16			// Maximum events per window
#define MAX_EVENT_ARGS		2
#define MAX_EVENT_ARGLEN	MAX_QPATH
#define MAX_EVENT_ARGVECS	2

#define MAX_EVENT_ACTIONS	16			// Maximum actions per event per window
#define MAX_ACTION_ARGS		4
#define MAX_ACTION_ARGLEN	MAX_QPATH
#define MAX_ACTION_ARGVECS	4

/*
=============================================================================

	MEMORY MANAGEMENT

=============================================================================
*/

enum {
	GUITAG_KEEP,			// What's kept in memory at all times
	GUITAG_SCRATCH,			// Scratch tag for init. When a GUI is complete it's tag is shifted to GUITAG_KEEP
};

#define GUI_AllocTag(size,zeroFill,tagNum)	_Mem_Alloc((size),(zeroFill),MEMPOOL_GUISYS,(tagNum),__FILE__,__LINE__)
#define GUI_FreeTag(tagNum)					_Mem_FreeTag(MEMPOOL_GUISYS,(tagNum),__FILE__,__LINE__)

/*
=============================================================================

	GUI CURSOR

	Each GUI can script their cursor setup, and modify (with events)
	the properties.
=============================================================================
*/

typedef struct guiCursorData_s {
	qBool				disabled;

	char				name[MAX_QPATH];
	struct shader_s		*mat;
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

	struct gui_s		*activeWindow;
} guiCursor_t;

/*
=============================================================================

	GUI WINDOW

=============================================================================
*/

//
// Script path location
// This is so that moddir gui's of the same name have precedence over basedir ones
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
//
typedef enum guiFlags_s {
	WFL_CURSOR			= 1 << 0,
	WFL_FILL_COLOR		= 1 << 1,
	WFL_MATERIAL		= 1 << 2,
} guiFlags_t;

//
// Window event actions
//
typedef enum evActionType_s {
	WEA_NONE,

	WEA_COMMAND,
	WEA_IF,
	WEA_LOCAL_SOUND,
	WEA_LOCK_CURSOR,
	WEA_RESET_TIME,
	WEA_SET,
	WEA_SET_FOCUS,
	WEA_STOP_TRANSITIONS,
	WEA_TRANSITION,

	WEA_MAX
} evActionType_t;

typedef struct evAction_s {
	evActionType_t		type;

	char				charArgs[MAX_ACTION_ARGS][MAX_ACTION_ARGLEN];
	float				floatArgs[MAX_ACTION_ARGS][MAX_ACTION_ARGVECS];
	int					intArgs[MAX_ACTION_ARGS][MAX_ACTION_ARGVECS];
} evAction_t;

//
// Window events
//
typedef enum guiEventType_s {
	WEV_NONE,

	WEV_ACTION,
	WEV_ESCAPE,
	WEV_FRAME,
	WEV_INIT,
	WEV_MOUSE_ENTER,
	WEV_MOUSE_EXIT,
	WEV_NAMED,
	WEV_TIME,

	WEV_MAX
} guiEventType_t;

typedef struct guiEvent_s {
	guiEventType_t		type;

	char				charArgs[MAX_EVENT_ARGS][MAX_EVENT_ARGLEN];
	float				floatArgs[MAX_EVENT_ARGS][MAX_EVENT_ARGVECS];
	int					intArgs[MAX_EVENT_ARGS][MAX_EVENT_ARGVECS];

	uint32				numActions;
	evAction_t			actionList[MAX_EVENT_ACTIONS];
} guiEvent_t;

//
// GUI window struct
//
typedef struct guiData_s {
	vec4_t				rect;
	float				rotation;
	vec2_t				shear;

	vec4_t				fillColor;
	char				matName[MAX_QPATH];
	struct shader_s		*matShader;
	vec4_t				matColor;
	float				matScaleX;
	float				matScaleY;

	qBool				modal;
	qBool				noEvents;
	qBool				noTime;
	qBool				visible;
	qBool				wantEnter;
} guiData_t;

typedef struct gui_s {
	// Static (compiled) information
	char				name[MAX_GUI_NAMELEN];
	guiType_t			type;
	guiFlags_t			flags;
	guiCursor_t			*cursor;
	guiData_t			s;

	// Dynamic information
	guiData_t			d;
	vec3_t				mins, maxs;

	// Events
	uint32				numEvents;
	guiEvent_t			eventList[MAX_GUI_EVENTS];

	// Event queue
	uint32				numTriggers;
	guiEventType_t		triggerList[MAX_GUI_EVENTS];

	// Children
	struct gui_s		*parent;						// Next window up

	uint32				numChildren;
	struct gui_s		*childList;

	struct gui_s		*hashNext;
} gui_t;

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*gui_developer;
extern cVar_t	*gui_debugBounds;
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
void		GUI_GenerateBounds (gui_t *window);
void		GUI_CursorUpdate (gui_t *gui);

//
// gui_events.c
//
void		GUI_FireTriggers (gui_t *gui);
void		GUI_QueueTrigger (gui_t *gui, guiEventType_t trigger);

//
// gui_main.c
//
void		GUI_ResetGUIState (gui_t *gui);
