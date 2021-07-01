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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.
//

#include "../client/cl_local.h"
#include "winquake.h"

qBool	in_AppActive;

/*
============================================================

	MOUSE

============================================================
*/

// mouse variables
cVar_t		*m_filter;
cVar_t		*m_accel;
cVar_t		*in_mouse;

static qBool	mLooking;

static void IN_MLookDown (void) { mLooking = qTrue; }
static void IN_MLookUp (void) {
	mLooking = qFalse;
	if (!freelook->value && lookspring->value)
			IN_CenterView ();
}

static qBool		mouse_Initialized;
static qBool		mouse_Active;		// qFalse when not focus app

static int			mouse_NumButtons;
static int			mouse_OldButtonState;
static POINT		mouse_CurrentPos;

static ivec2_t		mouse_XY;
static ivec2_t		mouse_OldXY;

static qBool		mouse_RestoreSPI;
static qBool		mouse_ValidSPI;
static ivec3_t		mouse_OrigParams;
static ivec3_t		mouse_NoAccelParms = {0, 0, 0};
static ivec3_t		mouse_AccelParms = {0, 0, 1};

static ivec2_t		window_CenterXY;
static RECT			window_Rect;

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void) {
	int		width, height;

	if (!mouse_Initialized)
		return;

	if (!in_mouse->value) {
		mouse_Active = qFalse;
		return;
	}

	if (mouse_Active)
		return;

	mouse_Active = qTrue;

	if (mouse_ValidSPI) {
		if (m_accel->integer == 2) {
			// original parameters
			mouse_RestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouse_OrigParams, 0);
		} else if (m_accel->integer == 1) {
			// normal quake2 setting
			mouse_RestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouse_AccelParms, 0);
		} else {
			// no acceleration
			mouse_RestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouse_NoAccelParms, 0);
		}
	}

	width = GetSystemMetrics (SM_CXSCREEN);
	height = GetSystemMetrics (SM_CYSCREEN);

	GetWindowRect (globalhWnd, &window_Rect);
	if (window_Rect.left < 0)
		window_Rect.left = 0;
	if (window_Rect.top < 0)
		window_Rect.top = 0;
	if (window_Rect.right >= width)
		window_Rect.right = width-1;
	if (window_Rect.bottom >= height-1)
		window_Rect.bottom = height-1;

	window_CenterXY[0] = (window_Rect.right + window_Rect.left) / 2;
	window_CenterXY[1] = (window_Rect.top + window_Rect.bottom) / 2;

	SetCursorPos (window_CenterXY[0], window_CenterXY[1]);

	SetCapture (globalhWnd);
	ClipCursor (&window_Rect);
	while (ShowCursor (FALSE) >= 0) ;
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void) {
	if (!mouse_Initialized)
		return;
	if (!mouse_Active)
		return;

	if (mouse_RestoreSPI)
		SystemParametersInfo (SPI_SETMOUSE, 0, mouse_OrigParams, 0);

	mouse_Active = qFalse;

	SetCapture (NULL); // stops annoying jerk to center screen when clicking away in windowed mode
	ClipCursor (NULL);
	ReleaseCapture ();
	while (ShowCursor (TRUE) < 0) ;
}


/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void) {
	cVar_t		*cv;
	qBool		fResult = GetSystemMetrics (SM_MOUSEPRESENT); 

	Com_Printf (0, "Mouse initialization\n");
	cv = Cvar_Get ("in_initmouse",	"1",	CVAR_NOSET);
	if (!cv->value) {
		Com_Printf (0, "...skipped\n");
		return;
	}

	mouse_Initialized = qTrue;
	mouse_ValidSPI = SystemParametersInfo (SPI_GETMOUSE, 0, mouse_OrigParams, 0);
	mouse_NumButtons = 5;

	if (fResult)
		Com_Printf (PRNT_DEV, "...mouse found\n");
	else
		Com_Printf (0, "..." S_COLOR_RED "mouse not found\n");
}


/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate) {
	int		i;

	if (!mouse_Initialized)
		return;

	// perform button actions
	for (i=0 ; i<mouse_NumButtons ; i++) {
		if ((mstate & (1<<i)) && !(mouse_OldButtonState & (1<<i)))
			Key_Event (K_MOUSE1 + i, qTrue, sysMsgTime);

		if (!(mstate & (1<<i)) && (mouse_OldButtonState & (1<<i)))
			Key_Event (K_MOUSE1 + i, qFalse, sysMsgTime);
	}
		
	mouse_OldButtonState = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (userCmd_t *cmd) {
	int		mx, my;

	if (!mouse_Active)
		return;

	// find mouse movement
	if (!GetCursorPos (&mouse_CurrentPos))
		return;

	mx = mouse_CurrentPos.x - window_CenterXY[0];
	my = mouse_CurrentPos.y - window_CenterXY[1];

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (window_CenterXY[0], window_CenterXY[1]);

	if (Key_KeyDest (KD_MENU)) {
		CL_UIModule_MoveMouse (mx, my);
		return;
	}

	if (m_filter->value) {
		mouse_XY[0] = (mx + mouse_OldXY[0]) * 0.5;
		mouse_XY[1] = (my + mouse_OldXY[1]) * 0.5;
	} else {
		mouse_XY[0] = mx;
		mouse_XY[1] = my;
	}

	mouse_OldXY[0] = mx;
	mouse_OldXY[1] = my;

	// psychospaz - zooming in preserves sensitivity
	if (autosensitivity->value) {
		mouse_XY[0] *= sensitivity->value * (cl.refDef.fovX/90.0);
		mouse_XY[1] *= sensitivity->value * (cl.refDef.fovY/90.0);
	} else {
		mouse_XY[0] *= sensitivity->value;
		mouse_XY[1] *= sensitivity->value;
	}

	// add mouse X/Y movement to cmd
	if ((in_Strafe.state & 1) || (lookstrafe->value && mLooking))
		cmd->sideMove += m_side->value * mouse_XY[0];
	else
		cl.viewAngles[YAW] -= m_yaw->value * mouse_XY[0];

	if ((mLooking || freelook->value) && !(in_Strafe.state & 1))
		cl.viewAngles[PITCH] += m_pitch->value * mouse_XY[1];
	else
		cmd->forwardMove -= m_forward->value * mouse_XY[1];

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (window_CenterXY[0], window_CenterXY[1]);
}

/*
=========================================================================

	JOYSTICK

=========================================================================
*/

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

enum _ControlList {
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn, AxisUp
};

DWORD	dwAxisFlags[JOY_MAX_AXES] = {
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

DWORD	dwAxisMap[JOY_MAX_AXES];
DWORD	dwControlMap[JOY_MAX_AXES];
PDWORD	pdwRawValue[JOY_MAX_AXES];

// None of these cvars are saved over a session
// This means that advanced controller configuration needs to be executed
// each time. This avoids any problems with getting back to a default usage
// or when changing from one controller to another. This way at least something
// works.
cVar_t	*in_joystick;
cVar_t	*joy_name;
cVar_t	*joy_advanced;
cVar_t	*joy_advaxisx;
cVar_t	*joy_advaxisy;
cVar_t	*joy_advaxisz;
cVar_t	*joy_advaxisr;
cVar_t	*joy_advaxisu;
cVar_t	*joy_advaxisv;
cVar_t	*joy_forwardthreshold;
cVar_t	*joy_sidethreshold;
cVar_t	*joy_pitchthreshold;
cVar_t	*joy_yawthreshold;
cVar_t	*joy_forwardsensitivity;
cVar_t	*joy_sidesensitivity;
cVar_t	*joy_pitchsensitivity;
cVar_t	*joy_yawsensitivity;
cVar_t	*joy_upthreshold;
cVar_t	*joy_upsensitivity;

int			joy_ID;

qBool		joy_Avail;
qBool		joy_AdvancedInit;
qBool		joy_HasPOV;

DWORD		joy_OldButtonState;
DWORD		joy_OldPOVState;

DWORD		joy_Flags;
DWORD		joy_NumButtons;

static JOYINFOEX	ji;

/*
=============== 
IN_StartupJoystick 
=============== 
*/
void IN_StartupJoystick (void) {
	int			numDevs;
	JOYCAPS		jc;
	MMRESULT	mmr = 0;
	cVar_t		*cv;

	// assume no joystick
	joy_Avail = qFalse;

	Com_Printf (0, "Joystick initialization\n");

	// abort startup if user requests no joystick
	cv = Cvar_Get ("in_initjoy", "1", CVAR_NOSET);
	if (!cv->value) {
		Com_Printf (0, "...skipped\n");
		return;
	}
 
	// verify joystick driver is present
	if ((numDevs = joyGetNumDevs ()) == 0) {
		return;
	}

	// cycle through the joystick ids for the first valid one
	for (joy_ID=0 ; joy_ID<numDevs ; joy_ID++) {
		memset (&ji, 0, sizeof (ji));
		ji.dwSize = sizeof (ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (joy_ID, &ji)) == JOYERR_NOERROR)
			break;
	} 

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR) {
		Com_Printf (0, "...not found -- no valid joysticks (%x)\n", mmr);
		return;
	}

	/*
	** get the capabilities of the selected joystick
	** abort startup if command fails
	*/
	memset (&jc, 0, sizeof (jc));
	if ((mmr = joyGetDevCaps (joy_ID, &jc, sizeof (jc))) != JOYERR_NOERROR) {
		Com_Printf (0, "..." S_COLOR_YELLOW "not found -- invalid joystick capabilities (%x)\n", mmr); 
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_NumButtons = jc.wNumButtons;
	joy_HasPOV = (jc.wCaps & JOYCAPS_HASPOV) ? qTrue : qFalse;

	// old button and POV states default to no buttons pressed
	joy_OldButtonState = 0;
	joy_OldPOVState = 0;

	/*
	** mark the joystick as available and advanced initialization not completed
	** this is needed as cvars are not available during initialization
	*/
	joy_Avail = qTrue; 
	joy_AdvancedInit = qFalse;

	Com_Printf (0, "...detected\n"); 
}


/*
===========
RawValuePointer
===========
*/
static PDWORD RawValuePointer (int axis) {
	switch (axis) {
	case JOY_AXIS_X:			return &ji.dwXpos;
	case JOY_AXIS_Y:			return &ji.dwYpos;
	case JOY_AXIS_Z:			return &ji.dwZpos;
	case JOY_AXIS_R:			return &ji.dwRpos;
	case JOY_AXIS_U:			return &ji.dwUpos;
	case JOY_AXIS_V:			return &ji.dwVpos;
	}

	return 0;
}


/*
===========
Joy_AdvancedUpdate_f
===========
*/
void Joy_AdvancedUpdate_f (void) {
	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

	// initialize all the maps
	for (i=0 ; i<JOY_MAX_AXES ; i++) {
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer (i);
	}

	if (joy_advanced->value == 0.0) {
		/*
		** default joystick initialization
		** 2 axes only with joystick control
		*/
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
	} else {
		if (strcmp (joy_name->string, "joystick") != 0) {
			// notify user of advanced controller
			Com_Printf (0, "\n%s configured\n\n", joy_name->string);
		}

		/*
		** advanced initialization here
		** data supplied by user via joy_axisn cvars
		*/
		dwTemp = (DWORD) joy_advaxisx->value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy->value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz->value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr->value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu->value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv->value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_Flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i=0 ; i<JOY_MAX_AXES ; i++) {
		if (dwAxisMap[i] != AxisNada)
			joy_Flags |= dwAxisFlags[i];
	}
}


/*
===========
IN_Commands
===========
*/
void IN_Commands (void) {
	int		i, key_index;
	DWORD	buttonstate, povstate;

	if (!joy_Avail)
		return;
	
	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i=0 ; i<joy_NumButtons ; i++) {
		if ((buttonstate & (1<<i)) && !(joy_OldButtonState & (1<<i))) {
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, qTrue, 0);
		}

		if (!(buttonstate & (1<<i)) && (joy_OldButtonState & (1<<i))) {
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, qFalse, 0);
		}
	}
	joy_OldButtonState = buttonstate;

	if (joy_HasPOV) {
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if (ji.dwPOV != JOY_POVCENTERED) {
			if (ji.dwPOV == JOY_POVFORWARD)
				povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)
				povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)
				povstate |= 0x08;
		}

		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i < 4 ; i++) {
			if ((povstate & (1<<i)) && !(joy_OldPOVState & (1<<i)))
				Key_Event (K_AUX29 + i, qTrue, 0);

			if (!(povstate & (1<<i)) && (joy_OldPOVState & (1<<i)))
				Key_Event (K_AUX29 + i, qFalse, 0);
		}

		joy_OldPOVState = povstate;
	}
}


/*
=============== 
IN_ReadJoystick
=============== 
*/
qBool IN_ReadJoystick (void) {

	memset (&ji, 0, sizeof (ji));
	ji.dwSize = sizeof (ji);
	ji.dwFlags = joy_Flags;

	if (joyGetPosEx (joy_ID, &ji) == JOYERR_NOERROR)
		return qTrue;
	else {
		/*
		** Read error occurred!
		** turning off the joystick seems too harsh for 1 read error, but what should be done?
		*/
		return qFalse;
	}
}


/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove (userCmd_t *cmd) {
	float	speed, aspeed;
	float	fAxisValue;
	int		i;

	/*
	** complete initialization if first time in
	** this is needed as cvars are not available at initialization time
	*/
	if (joy_AdvancedInit != qTrue) {
		Joy_AdvancedUpdate_f ();
		joy_AdvancedInit = qTrue;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_Avail || !in_joystick->value)
		return; 
 
	// collect the joystick data, if possible
	if (IN_ReadJoystick () != qTrue)
		return;

	if ((in_Speed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;
	aspeed = speed * cls.frameTime;

	// loop through the axes
	for (i=0 ; i<JOY_MAX_AXES ; i++) {
		// get the floating point zero-centered, potentially-inverted data for the current axis*/
		fAxisValue = (float) *pdwRawValue[i];
		// move centerpoint to zero*/
		fAxisValue -= 32768.0;

		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i]) {
		case AxisForward:
			if ((joy_advanced->value == 0.0) && mLooking) {
				// user wants forward control to become look control*/
				if (fabs (fAxisValue) > joy_pitchthreshold->value) {	
					/*
					** if mouse invert is on, invert the joystick pitch value
					** only absolute control support here (joy_advanced is qFalse)
					*/
					if (m_pitch->value < 0.0)
						cl.viewAngles[PITCH] -= (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
				}
			} else {
				// user wants forward control to be forward control
				if (fabs (fAxisValue) > joy_forwardthreshold->value)
					cmd->forwardMove += (fAxisValue * joy_forwardsensitivity->value) * speed * cl_forwardspeed->value;
			}
			break;

		case AxisSide:
			if (fabs (fAxisValue) > joy_sidethreshold->value)
				cmd->sideMove += (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
			break;

		case AxisUp:
			if (fabs (fAxisValue) > joy_upthreshold->value)
				cmd->upMove += (fAxisValue * joy_upsensitivity->value) * speed * cl_upspeed->value;
			break;

		case AxisTurn:
			if ((in_Strafe.state & 1) || (lookstrafe->value && mLooking)) {
				// user wants turn control to become side control
				if (fabs (fAxisValue) > joy_sidethreshold->value)
					cmd->sideMove -= (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
			} else {
				// user wants turn control to be turn control
				if (fabs (fAxisValue) > joy_yawthreshold->value) {
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[YAW] += (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
					else
						cl.viewAngles[YAW] += (fAxisValue * joy_yawsensitivity->value) * speed * 180.0;

				}
			}
			break;

		case AxisLook:
			if (mLooking) {
				if (fabs (fAxisValue) > joy_pitchthreshold->value) {
					// pitch movement detected and pitch movement desired by user
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * speed * 180.0;
				}
			}
			break;

		default:
			break;
		}
	}
}

/*
=========================================================================

	GENERIC

=========================================================================
*/

/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qBool active) {
	in_AppActive = active;
	mouse_Active = !active;	// force a new window check or turn off
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void) {
	mouse_OldButtonState = 0;
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void) {
	if (!mouse_Initialized)
		return;

	if (!in_mouse || !in_AppActive) {
		IN_DeactivateMouse ();
		return;
	}

	if (m_accel->modified || ((m_accel->value == 1) && sensitivity->modified)) {
		if (m_accel->modified)
			m_accel->modified = qFalse;
		if (sensitivity->modified)
			sensitivity->modified = qFalse;

		// restart mouse system
		IN_DeactivateMouse ();
		IN_ActivateMouse ();

		return;
	}

	if ((!cl.refreshPrepped || Key_KeyDest (KD_CONSOLE)) && !Key_KeyDest (KD_MENU)) {
		// temporarily deactivate if not in fullscreen
		if (Cvar_VariableInteger ("vid_fullscreen") == 0) {
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}


/*
===========
IN_Move
===========
*/
void IN_Move (userCmd_t *cmd) {
	IN_MouseMove (cmd);

	if (sysActiveApp)
		IN_JoyMove (cmd);
}

/*
=========================================================================

	INIT / SHUTDOWN

=========================================================================
*/

/*
===========
IN_Init
===========
*/
void IN_Init (void) {
	int		inInitTime;

	inInitTime = Sys_Milliseconds ();
	Com_Printf (0, "\n--------- Input Initialization ---------\n");

	// mouse variables
	m_filter				= Cvar_Get ("m_filter",					"0",		0);
	m_accel					= Cvar_Get ("m_accel",					"1",		CVAR_ARCHIVE);
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE);
	joy_name				= Cvar_Get ("joy_name",					"joystick",	0);
	joy_advanced			= Cvar_Get ("joy_advanced",				"0",		0);
	joy_advaxisx			= Cvar_Get ("joy_advaxisx",				"0",		0);
	joy_advaxisy			= Cvar_Get ("joy_advaxisy",				"0",		0);
	joy_advaxisz			= Cvar_Get ("joy_advaxisz",				"0",		0);
	joy_advaxisr			= Cvar_Get ("joy_advaxisr",				"0",		0);
	joy_advaxisu			= Cvar_Get ("joy_advaxisu",				"0",		0);
	joy_advaxisv			= Cvar_Get ("joy_advaxisv",				"0",		0);
	joy_forwardthreshold	= Cvar_Get ("joy_forwardthreshold",		"0.15",		0);
	joy_sidethreshold		= Cvar_Get ("joy_sidethreshold",		"0.15",		0);
	joy_upthreshold			= Cvar_Get ("joy_upthreshold",			"0.15",		0);
	joy_pitchthreshold		= Cvar_Get ("joy_pitchthreshold",		"0.15",		0);
	joy_yawthreshold		= Cvar_Get ("joy_yawthreshold",			"0.15",		0);
	joy_forwardsensitivity	= Cvar_Get ("joy_forwardsensitivity",	"-1",		0);
	joy_sidesensitivity		= Cvar_Get ("joy_sidesensitivity",		"-1",		0);
	joy_upsensitivity		= Cvar_Get ("joy_upsensitivity",		"-1",		0);
	joy_pitchsensitivity	= Cvar_Get ("joy_pitchsensitivity",		"1",		0);
	joy_yawsensitivity		= Cvar_Get ("joy_yawsensitivity",		"-1",		0);

	// commands
	Cmd_AddCommand ("+mlook",				IN_MLookDown,			"");
	Cmd_AddCommand ("-mlook",				IN_MLookUp,				"");

	Cmd_AddCommand ("joy_advancedupdate",	Joy_AdvancedUpdate_f,	"");

	// init
	IN_StartupMouse ();
	IN_StartupJoystick ();

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - inInitTime)*0.001f);
	Com_Printf (0, "----------------------------------------\n");
}


/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void) {
	IN_DeactivateMouse ();
}
