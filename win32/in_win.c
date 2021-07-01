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

/*
=========================================================================

	KEYBOARD

=========================================================================
*/

// since the keys like to act "stuck" all the time with this new shit,
// we're not going to use it until a better solution becomes obvious
//#define NEWKBCODE

#ifdef NEWKBCODE
HKL		kbLayout;
#endif // NEWKBCODE

/*
=================
IN_StartupKeyboard
=================
*/
static void IN_StartupKeyboard (void) {
#ifdef NEWKBCODE
	kbLayout = GetKeyboardLayout (0);
#endif // NEWKBCODE
}


/*
=================
In_MapKey

Map from layout to quake keynums
=================
*/
static byte scanToKey[128] = {
	0,			K_ESCAPE,	'1',		'2',		'3',		'4',		'5',		'6',
	'7',		'8',		'9',		'0',		'-',		'=',		K_BACKSPACE,9,		// 0
	'q',		'w',		'e',		'r',		't',		'y',		'u',		'i',
	'o',		'p',		'[',		']',		K_ENTER,	K_CTRL,		'a',		's',	// 1
	'd',		'f',		'g',		'h',		'j',		'k',		'l',		';',
	'\'',		'`',		K_LSHIFT,	'\\',		'z',		'x',		'c',		'v',	// 2
	'b',		'n',		'm',		',',		'.',		'/',		K_RSHIFT,	'*',
	K_ALT,		' ',		K_CAPSLOCK,	K_F1,		K_F2,		K_F3,		K_F4,		K_F5,	// 3
	K_F6,		K_F7,		K_F8,		K_F9,		K_F10,		K_PAUSE,	0,			K_HOME,
	K_UPARROW,	K_PGUP,		K_KP_MINUS,	K_LEFTARROW,K_KP_FIVE,	K_RIGHTARROW,K_KP_PLUS,	K_END,	// 4
	K_DOWNARROW,K_PGDN,		K_INS,		K_DEL,		0,			0,			0,			K_F11,
	K_F12,		0,			0,			0,			0,			0,			0,			0,		// 5
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// 6
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0		// 7
};
int In_MapKey (int wParam, int lParam) {
	int		modified;
#ifdef NEWKBCODE
	int		scanCode;
	byte	kbState[256];
	byte	result[4];

	// new stuff
	switch (wParam) {
	case VK_TAB:		return K_TAB;
	case VK_RETURN:		return K_ENTER;
	case VK_ESCAPE:		return K_ESCAPE;
	case VK_SPACE:		return K_SPACE;

	case VK_BACK:		return K_BACKSPACE;
	case VK_UP:			return K_UPARROW;
	case VK_DOWN:		return K_DOWNARROW;
	case VK_LEFT:		return K_LEFTARROW;
	case VK_RIGHT:		return K_RIGHTARROW;

	case VK_MENU:		return K_ALT;
//	case VK_LMENU:
//	case VK_RMENU:

	case VK_CONTROL:	return K_CTRL;
//	case VK_LCONTROL:
//	case VK_RCONTROL:

	case VK_SHIFT:		return K_SHIFT;
	case VK_LSHIFT:		return K_LSHIFT;
	case VK_RSHIFT:		return K_RSHIFT;

	case VK_CAPITAL:	return K_CAPSLOCK;

	case VK_F1:			return K_F1;
	case VK_F2:			return K_F2;
	case VK_F3:			return K_F3;
	case VK_F4:			return K_F4;
	case VK_F5:			return K_F5;
	case VK_F6:			return K_F6;
	case VK_F7:			return K_F7;
	case VK_F8:			return K_F8;
	case VK_F9:			return K_F9;
	case VK_F10:		return K_F10;
	case VK_F11:		return K_F11;
	case VK_F12:		return K_F12;

	case VK_INSERT:		return K_INS;
	case VK_DELETE:		return K_DEL;
	case VK_NEXT:		return K_PGDN;
	case VK_PRIOR:		return K_PGUP;
	case VK_HOME:		return K_HOME;
	case VK_END:		return K_END;

	case VK_NUMPAD7:	return K_KP_HOME;
	case VK_NUMPAD8:	return K_KP_UPARROW;
	case VK_NUMPAD9:	return K_KP_PGUP;
	case VK_NUMPAD4:	return K_KP_LEFTARROW;
	case VK_NUMPAD5:	return K_KP_FIVE;
	case VK_NUMPAD6:	return K_KP_RIGHTARROW;
	case VK_NUMPAD1:	return K_KP_END;
	case VK_NUMPAD2:	return K_KP_DOWNARROW;
	case VK_NUMPAD3:	return K_KP_PGDN;
	case VK_NUMPAD0:	return K_KP_INS;
	case VK_DECIMAL:	return K_KP_DEL;
	case VK_DIVIDE:		return K_KP_SLASH;
	case VK_SUBTRACT:	return K_KP_MINUS;
	case VK_ADD:		return K_KP_PLUS;

	case VK_PAUSE:		return K_PAUSE;
	}
#endif // NEWKBCODE

	// old stuff
	modified = (lParam >> 16) & 255;
	if (modified < 128) {
		modified = scanToKey[modified];
		if (lParam & (1 << 24)) {
			switch (modified) {
			case 0x0D:			return K_KP_ENTER;
			case 0x2F:			return K_KP_SLASH;
			case 0xAF:			return K_KP_PLUS;
			}
		}
		else {
			switch (modified) {
			case K_HOME:		return K_KP_HOME;
			case K_UPARROW:		return K_KP_UPARROW;
			case K_PGUP:		return K_KP_PGUP;
			case K_LEFTARROW:	return K_KP_LEFTARROW;
			case K_RIGHTARROW:	return K_KP_RIGHTARROW;
			case K_END:			return K_KP_END;
			case K_DOWNARROW:	return K_KP_DOWNARROW;
			case K_PGDN:		return K_KP_PGDN;
			case K_INS:			return K_KP_INS;
			case K_DEL:			return K_KP_DEL;
			}
		}
	}

#ifdef NEWKBCODE
	// get the VK_ keyboard state
	if (!GetKeyboardState (kbState))
		return modified;

	// convert ascii
	scanCode = (lParam >> 16) & 255;
	if (ToAsciiEx (wParam, scanCode, kbState, (uShort *)result, 0, kbLayout) < 1)
		return modified;

	return result[0];
#else
	return modified;
#endif // NEWKBCODE
}


/*
=================
In_GetKeyState
=================
*/
qBool In_GetKeyState (int keyNum) {
	switch (keyNum) {
	case K_CAPSLOCK:		return GetKeyState (VK_CAPITAL) ? qTrue : qFalse;
	}

	Com_Printf (0, S_COLOR_RED "In_GetKeyState: Invalid key");
	return qFalse;
}

/*
=========================================================================

	MOUSE

=========================================================================
*/

// mouse variables
cVar_t		*m_filter;
cVar_t		*m_accel;
cVar_t		*in_mouse;

static qBool	mLooking;

static qBool	mouseInitialized;
static qBool	mouseActive;		// qFalse when not focus app

static int		mouseNumButtons;
static int		mouseOldButtonState;
static POINT	mouseCurrentPos;

static ivec2_t	mouseXY;
static ivec2_t	mouseOldXY;

static qBool	mouseRestoreSPI;
static qBool	mouseValidSPI;
static ivec3_t	mouseOrigParams;
static ivec3_t	mouseNoAccelParms = {0, 0, 0};
static ivec3_t	mouseAccelParms = {0, 0, 1};

static ivec2_t	windowCenterXY;
static RECT		windowRect;

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
static void IN_ActivateMouse (void) {
	int		width, height;

	if (!mouseInitialized)
		return;

	if (!in_mouse->value) {
		mouseActive = qFalse;
		return;
	}

	if (mouseActive)
		return;

	mouseActive = qTrue;

	if (mouseValidSPI) {
		if (m_accel->integer == 2) {
			// original parameters
			mouseRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouseOrigParams, 0);
		}
		else if (m_accel->integer == 1) {
			// normal quake2 setting
			mouseRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouseAccelParms, 0);
		}
		else {
			// no acceleration
			mouseRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, mouseNoAccelParms, 0);
		}
	}

	width = GetSystemMetrics (SM_CXSCREEN);
	height = GetSystemMetrics (SM_CYSCREEN);

	GetWindowRect (globalhWnd, &windowRect);
	if (windowRect.left < 0)
		windowRect.left = 0;
	if (windowRect.top < 0)
		windowRect.top = 0;
	if (windowRect.right >= width)
		windowRect.right = width-1;
	if (windowRect.bottom >= height-1)
		windowRect.bottom = height-1;

	windowCenterXY[0] = (windowRect.right + windowRect.left) / 2;
	windowCenterXY[1] = (windowRect.top + windowRect.bottom) / 2;

	SetCursorPos (windowCenterXY[0], windowCenterXY[1]);

	SetCapture (globalhWnd);
	ClipCursor (&windowRect);
	while (ShowCursor (FALSE) >= 0) ;
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
static void IN_DeactivateMouse (void) {
	if (!mouseInitialized)
		return;
	if (!mouseActive)
		return;

	if (mouseRestoreSPI)
		SystemParametersInfo (SPI_SETMOUSE, 0, mouseOrigParams, 0);

	mouseActive = qFalse;
	ClipCursor (NULL);
	ReleaseCapture ();
	while (ShowCursor (TRUE) < 0) ;
}


/*
===========
IN_StartupMouse
===========
*/
static void IN_StartupMouse (void) {
	cVar_t		*cv;
	qBool		fResult = GetSystemMetrics (SM_MOUSEPRESENT); 

	Com_Printf (0, "Mouse initialization\n");
	cv = Cvar_Get ("in_initmouse",	"1",	CVAR_NOSET);
	if (!cv->value) {
		Com_Printf (0, "...skipped\n");
		return;
	}

	mouseInitialized = qTrue;
	mouseValidSPI = SystemParametersInfo (SPI_GETMOUSE, 0, mouseOrigParams, 0);
	mouseNumButtons = 5;

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

	if (!mouseInitialized)
		return;

	// perform button actions
	for (i=0 ; i<mouseNumButtons ; i++) {
		if ((mstate & (1<<i)) && !(mouseOldButtonState & (1<<i)))
			Key_Event (K_MOUSE1 + i, qTrue, sysMsgTime);

		if (!(mstate & (1<<i)) && (mouseOldButtonState & (1<<i)))
			Key_Event (K_MOUSE1 + i, qFalse, sysMsgTime);
	}
		
	mouseOldButtonState = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (userCmd_t *cmd) {
	int		mx, my;

	if (!mouseActive)
		return;

	// find mouse movement
	if (!GetCursorPos (&mouseCurrentPos))
		return;

	mx = mouseCurrentPos.x - windowCenterXY[0];
	my = mouseCurrentPos.y - windowCenterXY[1];

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (windowCenterXY[0], windowCenterXY[1]);

	if (Key_KeyDest (KD_MENU)) {
		CL_UIModule_MoveMouse (mx, my);
		return;
	}

	if (m_filter->value) {
		mouseXY[0] = (mx + mouseOldXY[0]) * 0.5;
		mouseXY[1] = (my + mouseOldXY[1]) * 0.5;
	}
	else {
		mouseXY[0] = mx;
		mouseXY[1] = my;
	}

	mouseOldXY[0] = mx;
	mouseOldXY[1] = my;

	// psychospaz - zooming in preserves sensitivity
	if (autosensitivity->value) {
		mouseXY[0] *= sensitivity->value * (cl.refDef.fovX/90.0);
		mouseXY[1] *= sensitivity->value * (cl.refDef.fovY/90.0);
	}
	else {
		mouseXY[0] *= sensitivity->value;
		mouseXY[1] *= sensitivity->value;
	}

	// add mouse X/Y movement to cmd
	if ((inBtnStrafe.state & 1) || (lookstrafe->value && mLooking))
		cmd->sideMove += m_side->value * mouseXY[0];
	else
		cl.viewAngles[YAW] -= m_yaw->value * mouseXY[0];

	if ((mLooking || freelook->value) && !(inBtnStrafe.state & 1))
		cl.viewAngles[PITCH] += m_pitch->value * mouseXY[1];
	else
		cmd->forwardMove -= m_forward->value * mouseXY[1];

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (windowCenterXY[0], windowCenterXY[1]);
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

static int		joy_ID;

static qBool	joy_Avail;
static qBool	joy_AdvancedInit;
static qBool	joy_HasPOV;

static DWORD	joy_OldButtonState;
static DWORD	joy_OldPOVState;

static DWORD	joy_Flags;
static DWORD	joy_NumButtons;

static JOYINFOEX	ji;

/*
=============== 
IN_StartupJoystick 
=============== 
*/
static void IN_StartupJoystick (void) {
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
static void Joy_AdvancedUpdate_f (void) {
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
	}
	else {
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
static qBool IN_ReadJoystick (void) {
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
static void IN_JoyMove (userCmd_t *cmd) {
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

	if ((inBtnSpeed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;
	aspeed = speed * cls.netFrameTime;

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
			}
			else {
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
			if ((inBtnStrafe.state & 1) || (lookstrafe->value && mLooking)) {
				// user wants turn control to become side control
				if (fabs (fAxisValue) > joy_sidethreshold->value)
					cmd->sideMove -= (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
			}
			else {
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
	mouseActive = !active;	// force a new window check or turn off
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void) {
	if (!mouseInitialized)
		return;

	if (!sysActiveApp) {
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

	if ((!cls.refreshPrepped || Key_KeyDest (KD_CONSOLE)) && !Key_KeyDest (KD_MENU)) {
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
	if (!sysActiveApp)
		return;

	IN_MouseMove (cmd);
	IN_JoyMove (cmd);
}

/*
=========================================================================

	CONSOLE COMMANDS

=========================================================================
*/

/*
===========
IN_MLookDown_f
===========
*/
static void IN_MLookDown_f (void) {
	mLooking = qTrue;
}


/*
===========
IN_MLookUp_f
===========
*/
static void IN_MLookUp_f (void) {
	mLooking = qFalse;
	if (!freelook->value && lookspring->value)
			IN_CenterView ();
}


/*
===========
IN_Restart_f
===========
*/
void IN_Restart_f (void) {
	IN_Shutdown (qFalse);
	IN_Init ();
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
	Cmd_AddCommand ("in_restart",			IN_Restart_f,			"Restarts input subsystem");

	Cmd_AddCommand ("+mlook",				IN_MLookDown_f,			"");
	Cmd_AddCommand ("-mlook",				IN_MLookUp_f,			"");

	Cmd_AddCommand ("joy_advancedupdate",	Joy_AdvancedUpdate_f,	"");

	// init
	IN_StartupMouse ();
	IN_StartupJoystick ();
	IN_StartupKeyboard ();

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "init time: %.2fs\n", (Sys_Milliseconds () - inInitTime)*0.001f);
	Com_Printf (0, "----------------------------------------\n");
}


/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (qBool full) {
	if (!full) {
		// commands
		Cmd_RemoveCommand ("in_restart");

		Cmd_RemoveCommand ("+mlook");
		Cmd_RemoveCommand ("-mlook");

		Cmd_RemoveCommand ("joy_advancedupdate");
	}

	// shutdown
	IN_DeactivateMouse ();
}
