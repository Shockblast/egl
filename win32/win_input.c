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
// win_input.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.
//

#include "../client/cl_local.h"
#include "win_local.h"

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
static void IN_StartupKeyboard (void)
{
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
keyNum_t In_MapKey (int wParam, int lParam)
{
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
	if (ToAsciiEx (wParam, scanCode, kbState, (uint16 *)result, 0, kbLayout) < 1)
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
qBool In_GetKeyState (keyNum_t keyNum)
{
	switch (keyNum) {
	case K_CAPSLOCK:		return (GetKeyState (VK_CAPITAL)) ? qTrue : qFalse;
	}

	Com_Printf (PRNT_ERROR, "In_GetKeyState: Invalid key");
	return qFalse;
}

/*
=========================================================================

	MOUSE

=========================================================================
*/

// mouse variables
cVar_t		*m_accel;
cVar_t		*in_mouse;

static qBool	in_mInitialized;
static qBool	in_mActive;			// qFalse when not focus app
static int		in_mNumButtons;

static ivec2_t	in_wCenterPos;					// Window center

// Cursor position (outside the engine)
static qBool	in_mRestorePos;
static POINT	in_mOrigPos;

// System parameter info
static qBool	in_mRestoreSPI;
static qBool	in_mValidSPI;
static ivec3_t	in_mOSSPI;						// OS setting
static ivec3_t	in_mNoAccelSPI = {0, 0, 0};		// No accel setting
static ivec3_t	in_mAccelSPI = {0, 0, 1};		// Normal Quake2 setting

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
static void IN_ActivateMouse (void)
{
	RECT	wRect;
	int		width, height;

	if (!in_mInitialized)
		return;

	if (!in_mouse->intVal) {
		in_mActive = qFalse;
		return;
	}

	if (in_mActive)
		return;

	in_mActive = qTrue;

	if (in_mValidSPI) {
		// Sanity check
		if (m_accel->intVal < 0 || m_accel->intVal > 2) {
			Com_Printf (PRNT_WARNING, "WARNING: invalid m_accel value '%i', forcing default!\n", m_accel->intVal);
			Cvar_VariableReset (m_accel, qTrue);
		}

		switch (m_accel->intVal) {
		case 2:
			// OS parameters
			in_mRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, in_mOSSPI, 0);
			break;

		case 1:
			// Normal quake2 setting
			in_mRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, in_mAccelSPI, 0);
			break;

		default:
			// No acceleration
			in_mRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, in_mNoAccelSPI, 0);
			break;
		}
	}

	// Store current position for restoring later
	in_mRestorePos = GetCursorPos (&in_mOrigPos);

	// Clip the cursor to the window
	width = GetSystemMetrics (SM_CXSCREEN);
	height = GetSystemMetrics (SM_CYSCREEN);

	GetWindowRect (sys_winInfo.hWnd, &wRect);
	if (wRect.left < 0)
		wRect.left = 0;
	if (wRect.top < 0)
		wRect.top = 0;
	if (wRect.right >= width)
		wRect.right = width-1;
	if (wRect.bottom >= height-1)
		wRect.bottom = height-1;

	in_wCenterPos[0] = (wRect.right + wRect.left) / 2;
	in_wCenterPos[1] = (wRect.top + wRect.bottom) / 2;

	SetCursorPos (in_wCenterPos[0], in_wCenterPos[1]);

	SetCapture (sys_winInfo.hWnd);
	ClipCursor (&wRect);
	while (ShowCursor (FALSE) >= 0) ;
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
static void IN_DeactivateMouse (void)
{
	// Restore desktop params
	if (in_mRestoreSPI) {
		SystemParametersInfo (SPI_SETMOUSE, 0, in_mOSSPI, 0);
		in_mRestoreSPI = qFalse;
	}

	// Restore cursor position
	if (in_mRestorePos) {
		SetCursorPos (in_mOrigPos.x, in_mOrigPos.y);
		in_mRestorePos = qFalse;
	}

	if (!in_mInitialized)
		return;
	if (!in_mActive)
		return;

	// Release capture and show the cursor
	in_mActive = qFalse;
	ClipCursor (NULL);
	ReleaseCapture ();
	while (ShowCursor (TRUE) < 0) ;
}


/*
===========
IN_StartupMouse
===========
*/
static void IN_StartupMouse (void)
{
	cVar_t		*cv;
	qBool		fResult = GetSystemMetrics (SM_MOUSEPRESENT); 

	Com_Printf (0, "Mouse initialization\n");
	cv = Cvar_Register ("in_initmouse",	"1",	CVAR_READONLY);
	if (!cv->intVal) {
		Com_Printf (0, "...skipped\n");
		return;
	}

	in_mInitialized = qTrue;
	in_mValidSPI = SystemParametersInfo (SPI_GETMOUSE, 0, in_mOSSPI, 0);
	in_mNumButtons = 5;

	if (fResult)
		Com_DevPrintf (0, "...mouse found\n");
	else
		Com_Printf (PRNT_ERROR, "...mouse not found\n");
}


/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int state)
{
	static int	oldState;
	int			i;

	if (!in_mInitialized)
		return;

	// Perform button actions
	for (i=0 ; i<in_mNumButtons ; i++) {
		if (state & (1<<i) && !(oldState & (1<<i)))
			Key_Event (K_MOUSE1+i, qTrue, sys_winInfo.msgTime);

		if (!(state & (1<<i)) && oldState & (1<<i))
			Key_Event (K_MOUSE1+i, qFalse, sys_winInfo.msgTime);
	}
		
	oldState = state;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (userCmd_t *cmd)
{
	POINT	pointer;
	int		xMove, yMove;

	if (!in_mActive)
		return;

	// Find cursor position
	if (!GetCursorPos (&pointer))
		return;

	// Get moved distance
	xMove = pointer.x - in_wCenterPos[0];
	yMove = pointer.y - in_wCenterPos[1];
	if (xMove || yMove) {
		// Force the mouse to the center, so there's room to move
		SetCursorPos (in_wCenterPos[0], in_wCenterPos[1]);

		// Update in the client
		CL_MoveMouse (xMove, yMove);
	}
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
#define JOY_MAX_AXES		6				// X, Y, Z, R, U, V
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
static void IN_StartupJoystick (void)
{
	int			numDevs;
	JOYCAPS		jc;
	MMRESULT	mmr = 0;
	cVar_t		*cv;

	// assume no joystick
	joy_Avail = qFalse;

	Com_Printf (0, "Joystick initialization\n");

	// abort startup if user requests no joystick
	cv = Cvar_Register ("in_initjoy", "1", CVAR_READONLY);
	if (!cv->intVal) {
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
		Com_Printf (PRNT_WARNING, "...not found -- invalid joystick capabilities (%x)\n", mmr); 
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
static PDWORD RawValuePointer (int axis)
{
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
static void Joy_AdvancedUpdate_f (void)
{
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

	if (!joy_advanced->intVal) {
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
		dwTemp = (DWORD) joy_advaxisx->intVal;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy->intVal;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz->intVal;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr->intVal;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu->intVal;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv->intVal;
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
void IN_Commands (void)
{
	int		key_index;
	uint32	i;
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
static qBool IN_ReadJoystick (void)
{
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
static void IN_JoyMove (userCmd_t *cmd)
{
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
	if (!joy_Avail || !in_joystick->floatVal)
		return; 
 
	// collect the joystick data, if possible
	if (IN_ReadJoystick () != qTrue)
		return;

	if (CL_GetRunState ())
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
			if (!joy_advanced->intVal && CL_GetMLookState()) {
				// user wants forward control to become look control*/
				if (fabs (fAxisValue) > joy_pitchthreshold->floatVal) {	
					/*
					** if mouse invert is on, invert the joystick pitch value
					** only absolute control support here (joy_advanced is qFalse)
					*/
					if (m_pitch->floatVal < 0.0f)
						cl.viewAngles[PITCH] -= (fAxisValue * joy_pitchsensitivity->floatVal) * aspeed * cl_pitchspeed->floatVal;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->floatVal) * aspeed * cl_pitchspeed->floatVal;
				}
			}
			else {
				// user wants forward control to be forward control
				if (fabs (fAxisValue) > joy_forwardthreshold->floatVal)
					cmd->forwardMove += (fAxisValue * joy_forwardsensitivity->floatVal) * speed * cl_forwardspeed->floatVal;
			}
			break;

		case AxisSide:
			if (fabs (fAxisValue) > joy_sidethreshold->floatVal)
				cmd->sideMove += (fAxisValue * joy_sidesensitivity->floatVal) * speed * cl_sidespeed->floatVal;
			break;

		case AxisUp:
			if (fabs (fAxisValue) > joy_upthreshold->floatVal)
				cmd->upMove += (fAxisValue * joy_upsensitivity->floatVal) * speed * cl_upspeed->floatVal;
			break;

		case AxisTurn:
			if (CL_GetStrafeState () || (lookstrafe->intVal && CL_GetMLookState())) {
				// user wants turn control to become side control
				if (fabs (fAxisValue) > joy_sidethreshold->floatVal)
					cmd->sideMove -= (fAxisValue * joy_sidesensitivity->floatVal) * speed * cl_sidespeed->floatVal;
			}
			else {
				// user wants turn control to be turn control
				if (fabs (fAxisValue) > joy_yawthreshold->floatVal) {
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[YAW] += (fAxisValue * joy_yawsensitivity->floatVal) * aspeed * cl_yawspeed->floatVal;
					else
						cl.viewAngles[YAW] += (fAxisValue * joy_yawsensitivity->floatVal) * speed * 180.0;

				}
			}
			break;

		case AxisLook:
			if (CL_GetMLookState()) {
				if (fabs (fAxisValue) > joy_pitchthreshold->floatVal) {
					// pitch movement detected and pitch movement desired by user
					if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->floatVal) * aspeed * cl_pitchspeed->floatVal;
					else
						cl.viewAngles[PITCH] += (fAxisValue * joy_pitchsensitivity->floatVal) * speed * 180.0;
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
void IN_Activate (qBool active)
{
	in_mActive = !active;	// force a new window check or turn off
}


/*
==================
IN_Frame

Called every miscellaneous frame (10FPS, see CL_Frame).
==================
*/
void IN_Frame (void)
{
	if (!in_mInitialized)
		return;

	if (!sys_winInfo.appActive) {
		IN_DeactivateMouse ();
		return;
	}

	if (m_accel->modified || (m_accel->intVal == 1 && sensitivity->modified)) {
		if (m_accel->modified)
			m_accel->modified = qFalse;
		if (sensitivity->modified)
			sensitivity->modified = qFalse;

		// Restart mouse system
		IN_DeactivateMouse ();
		IN_ActivateMouse ();

		return;
	}

	// Temporarily deactivate if not in fullscreen
	if ((!cls.refreshPrepped || Key_GetDest () == KD_CONSOLE)
	&& Key_GetDest () != KD_MENU
	&& !cls.refConfig.vidFullScreen) {
		IN_DeactivateMouse ();
		return;
	}

	IN_ActivateMouse ();
}


/*
===========
IN_Move
===========
*/
void IN_Move (userCmd_t *cmd)
{
	if (!sys_winInfo.appActive)
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
IN_Restart_f
===========
*/
void IN_Restart_f (void)
{
	IN_Shutdown ();
	IN_Init ();
}

/*
=========================================================================

	INIT / SHUTDOWN

=========================================================================
*/

static void	*cmd_in_restart;
static void	*cmd_joy_advancedupdate;

/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	uint32	initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n--------- Input Initialization ---------\n");

	// Mouse variables
	m_accel					= Cvar_Register ("m_accel",						"1",		CVAR_ARCHIVE);
	in_mouse				= Cvar_Register ("in_mouse",					"1",		CVAR_ARCHIVE);

	// Joystick variables
	in_joystick				= Cvar_Register ("in_joystick",					"0",		CVAR_ARCHIVE);
	joy_name				= Cvar_Register ("joy_name",					"joystick",	0);
	joy_advanced			= Cvar_Register ("joy_advanced",				"0",		0);
	joy_advaxisx			= Cvar_Register ("joy_advaxisx",				"0",		0);
	joy_advaxisy			= Cvar_Register ("joy_advaxisy",				"0",		0);
	joy_advaxisz			= Cvar_Register ("joy_advaxisz",				"0",		0);
	joy_advaxisr			= Cvar_Register ("joy_advaxisr",				"0",		0);
	joy_advaxisu			= Cvar_Register ("joy_advaxisu",				"0",		0);
	joy_advaxisv			= Cvar_Register ("joy_advaxisv",				"0",		0);
	joy_forwardthreshold	= Cvar_Register ("joy_forwardthreshold",		"0.15",		0);
	joy_sidethreshold		= Cvar_Register ("joy_sidethreshold",			"0.15",		0);
	joy_upthreshold			= Cvar_Register ("joy_upthreshold",				"0.15",		0);
	joy_pitchthreshold		= Cvar_Register ("joy_pitchthreshold",			"0.15",		0);
	joy_yawthreshold		= Cvar_Register ("joy_yawthreshold",			"0.15",		0);
	joy_forwardsensitivity	= Cvar_Register ("joy_forwardsensitivity",		"-1",		0);
	joy_sidesensitivity		= Cvar_Register ("joy_sidesensitivity",			"-1",		0);
	joy_upsensitivity		= Cvar_Register ("joy_upsensitivity",			"-1",		0);
	joy_pitchsensitivity	= Cvar_Register ("joy_pitchsensitivity",		"1",		0);
	joy_yawsensitivity		= Cvar_Register ("joy_yawsensitivity",			"-1",		0);

	// Commands
	cmd_in_restart			= Cmd_AddCommand ("in_restart",			IN_Restart_f,			"Restarts input subsystem");
	cmd_joy_advancedupdate	= Cmd_AddCommand ("joy_advancedupdate",	Joy_AdvancedUpdate_f,	"");

	// Init
	IN_StartupMouse ();
	IN_StartupJoystick ();
	IN_StartupKeyboard ();

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "init time: %ums\n", Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}


/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	// Remove commands
	Cmd_RemoveCommand ("in_restart", cmd_in_restart);
	Cmd_RemoveCommand ("joy_advancedupdate", cmd_joy_advancedupdate);

	// Shutdown
	IN_DeactivateMouse ();
}
