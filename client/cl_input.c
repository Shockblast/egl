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
// cl_input.c
// Builds an intended movement command to send to the server
//

#include "cl_local.h"

cVar_t	*cl_nodelta;

cVar_t	*cl_upspeed;
cVar_t	*cl_forwardspeed;
cVar_t	*cl_sidespeed;

cVar_t	*cl_yawspeed;
cVar_t	*cl_pitchspeed;

cVar_t	*cl_run;

cVar_t	*cl_anglespeedkey;

extern uint32	sys_frameTime; // FIXME: have Sys_SendKeyEvents update this here, instead of making this nastily extern'd
static uint32	sys_oldFrameTime;
static uint32	in_frameMSec;

/*
===============================================================================

	KEY BUTTONS

	Continuous button event tracking is complicated by the fact that two
	different input sources (say, mouse button 1 and the control key) can both
	press the same button, but the button should only be released when both of
	the pressing key have been released.

	When a key event issues a button command (+forward, +attack, etc), it
	appends its key number as a parameter to the command so it can be matched
	up with the release.

	state bit 0 is the current state of the key
	state bit 1 is edge triggered on the up to down transition
	state bit 2 is edge triggered on the down to up transition

===============================================================================
*/

typedef struct kButton_s {
	int			down[2];		// key nums holding it down
	uint32		downTime;		// msec timestamp
	uint32		msec;			// msec down this frame
	int			state;
} kButton_t;

static kButton_t		btn_moveUp;
static kButton_t		btn_moveDown;
static kButton_t		btn_lookLeft;
static kButton_t		btn_lookRight;
static kButton_t		btn_moveForward;
static kButton_t		btn_moveBack;
static kButton_t		btn_lookUp;
static kButton_t		btn_lookDown;
static kButton_t		btn_moveLeft;
static kButton_t		btn_moveRight;

static kButton_t		btn_speed;
static kButton_t		btn_strafe;

static kButton_t		btn_attack;
static kButton_t		btn_use;

static kButton_t		btn_keyLook;

static int				btn_impulse;

/*
====================
KeyDown
====================
*/
static void KeyDown (kButton_t *b)
{
	int		k;
	char	*c;
	
	c = Cmd_Argv (1);
	if (c[0])
		k = atoi (c);
	else
		k = -1;	// Typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;	// Repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else {
		Com_Printf (PRNT_WARNING, "Three keys down for a button!\n");
		return;
	}
	
	if (b->state & 1)
		return;	// still down

	// Save timestamp
	c = Cmd_Argv (2);
	b->downTime = atoi(c);
	if (!b->downTime)
		b->downTime = sys_frameTime - 100;

	b->state |= 1 + 2;	// down + impulse down
}


/*
====================
KeyUp
====================
*/
static void KeyUp (kButton_t *b)
{
	int		k;
	char	*c;
	uint32	uptime;

	c = Cmd_Argv (1);
	if (c[0])
		k = atoi(c);
	else {
		// Typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// Impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// Key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// Some other key is still holding it down

	if (!(b->state & 1))
		return;		// Still up (this should not happen)

	// Save timestamp
	c = Cmd_Argv (2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downTime;
	else
		b->msec += 10;

	b->state &= ~1;		// Now up
	b->state |= 4;		// Impulse up
}


/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
static float CL_KeyState (kButton_t *key)
{
	int			msec;

	key->state &= 1;		// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state) {
		// Still down
		msec += sys_frameTime - key->downTime;
		key->downTime = sys_frameTime;
	}

	return clamp ((float)msec / (float)in_frameMSec, 0, 1);
}


static void IN_UpDown (void)		{ KeyDown (&btn_moveUp); }
static void IN_UpUp (void)			{ KeyUp (&btn_moveUp); }
static void IN_DownDown (void)		{ KeyDown (&btn_moveDown); }
static void IN_DownUp (void)		{ KeyUp (&btn_moveDown); }
static void IN_LeftDown(void)		{ KeyDown (&btn_lookLeft); }
static void IN_LeftUp (void)		{ KeyUp (&btn_lookLeft); }
static void IN_RightDown (void)		{ KeyDown (&btn_lookRight); }
static void IN_RightUp (void)		{ KeyUp (&btn_lookRight); }
static void IN_ForwardDown (void)	{ KeyDown (&btn_moveForward); }
static void IN_ForwardUp (void)		{ KeyUp (&btn_moveForward); }
static void IN_BackDown (void)		{ KeyDown (&btn_moveBack); }
static void IN_BackUp (void)		{ KeyUp (&btn_moveBack); }
static void IN_LookupDown (void)	{ KeyDown (&btn_lookUp); }
static void IN_LookupUp (void)		{ KeyUp (&btn_lookUp); }
static void IN_LookdownDown (void)	{ KeyDown (&btn_lookDown); }
static void IN_LookdownUp (void)	{ KeyUp (&btn_lookDown); }
static void IN_MoveleftDown (void)	{ KeyDown (&btn_moveLeft); }
static void IN_MoveleftUp (void)	{ KeyUp (&btn_moveLeft); }
static void IN_MoverightDown (void)	{ KeyDown (&btn_moveRight); }
static void IN_MoverightUp (void)	{ KeyUp (&btn_moveRight); }

static void IN_SpeedDown (void)		{ KeyDown (&btn_speed); }
static void IN_SpeedUp (void)		{ KeyUp (&btn_speed); }
static void IN_StrafeDown (void)	{ KeyDown (&btn_strafe); }
static void IN_StrafeUp (void)		{ KeyUp (&btn_strafe); }

static void IN_AttackDown (void)	{ KeyDown (&btn_attack); }
static void IN_AttackUp (void)		{ KeyUp (&btn_attack); }

static void IN_UseDown  (void)		{ KeyDown (&btn_use); }
static void IN_UseUp  (void)		{ KeyUp (&btn_use); }

static void IN_KLookDown (void)		{ KeyDown (&btn_keyLook); }
static void IN_KLookUp (void)		{ KeyUp (&btn_keyLook); }

static void IN_Impulse  (void)		{ btn_impulse=atoi (Cmd_Argv (1)); }

//==========================================================================

/*
================
CL_RunState
================
*/
qBool CL_RunState (void)
{
	return ((btn_speed.state & 1) ^ cl_run->integer);
}


/*
================
CL_StrafeState
================
*/
qBool CL_StrafeState (void)
{
	return (btn_strafe.state & 1);
}

//==========================================================================

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
static void CL_BaseMove (userCmd_t *cmd)
{
	float	tspeed;
	float	mspeed;

	// Adjust for turning speed
	if (btn_speed.state & 1)
		tspeed = cls.netFrameTime * cl_anglespeedkey->value;
	else
		tspeed = cls.netFrameTime;

	// Adjust for running speed
	if (CL_RunState ())
		mspeed = 2;
	else
		mspeed = 1;

	// Handle left/right on keyboard
	cl.cmdNum = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[cl.cmdNum];
	cl.cmdTime[cl.cmdNum] = cls.realTime;	// for netgraph ping calculation

	if (CL_StrafeState ()) {
		// Keyboard strafe
		cmd->sideMove += cl_sidespeed->value * CL_KeyState (&btn_lookRight);
		cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&btn_lookLeft);
	}
	else {
		// Keyboard turn
		cl.viewAngles[YAW] -= tspeed * cl_yawspeed->value * CL_KeyState (&btn_lookRight);
		cl.viewAngles[YAW] += tspeed * cl_yawspeed->value * CL_KeyState (&btn_lookLeft);
	}

	if (btn_keyLook.state & 1) {
		// Keyboard look
		cl.viewAngles[PITCH] -= tspeed * cl_pitchspeed->value * CL_KeyState (&btn_moveForward);
		cl.viewAngles[PITCH] += tspeed * cl_pitchspeed->value * CL_KeyState (&btn_moveBack);
	}
	else {
		// Keyboard move front/back
		cmd->forwardMove += cl_forwardspeed->value * CL_KeyState (&btn_moveForward);
		cmd->forwardMove -= cl_forwardspeed->value * CL_KeyState (&btn_moveBack);
	}

	// Keyboard look up/down
	cl.viewAngles[PITCH] -= tspeed * cl_pitchspeed->value * CL_KeyState (&btn_lookUp);
	cl.viewAngles[PITCH] += tspeed * cl_pitchspeed->value * CL_KeyState (&btn_lookDown);

	// Keyboard strafe left/right
	cmd->sideMove += cl_sidespeed->value * CL_KeyState (&btn_moveRight);
	cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&btn_moveLeft);

	// Keyboard jump/crouch
	cmd->upMove += cl_upspeed->value * CL_KeyState (&btn_moveUp);
	cmd->upMove -= cl_upspeed->value * CL_KeyState (&btn_moveDown);

	// R1: cap to max allowed ranges
	if (cmd->forwardMove > cl_forwardspeed->value * mspeed)
		cmd->forwardMove = cl_forwardspeed->value * mspeed;
	else if (cmd->forwardMove < -cl_forwardspeed->value * mspeed)
		cmd->forwardMove = -cl_forwardspeed->value * mspeed;

	if (cmd->sideMove > cl_sidespeed->value * mspeed)
		cmd->sideMove = cl_sidespeed->value * mspeed;
	else if (cmd->sideMove < -cl_sidespeed->value * mspeed)
		cmd->sideMove = -cl_sidespeed->value * mspeed;

	if (cmd->upMove > cl_upspeed->value * mspeed)
		cmd->upMove = cl_upspeed->value * mspeed;
	else if (cmd->upMove < -cl_upspeed->value * mspeed)
		cmd->upMove = -cl_upspeed->value * mspeed;
}


/*
==============
CL_ClampPitch
==============
*/
static void CL_ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE (cl.frame.playerState.pMove.deltaAngles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewAngles[PITCH] + pitch < -360)
		cl.viewAngles[PITCH] += 360; // wrapped
	if (cl.viewAngles[PITCH] + pitch > 360)
		cl.viewAngles[PITCH] -= 360; // wrapped

	if (cl.viewAngles[PITCH] + pitch > 89)
		cl.viewAngles[PITCH] = 89 - pitch;
	if (cl.viewAngles[PITCH] + pitch < -89)
		cl.viewAngles[PITCH] = -89 - pitch;
}


/*
=================
CL_RefreshCmd
=================
*/
void CL_RefreshCmd (void)
{
	int			ms;
	userCmd_t	*cmd = &cl.cmds[cls.netChan.outgoingSequence & CMD_MASK];

	// Get delta for this sample.
	in_frameMSec = sys_frameTime - sys_oldFrameTime;
	if (in_frameMSec < 1)
		return;
	else if (in_frameMSec > 200)
		in_frameMSec = 200;

	// Get basic movement from keyboard
	CL_BaseMove (cmd);

	// Allow mice or other external controllers to add to the move
	IN_Move (cmd);

	// Update cmd viewangles for CL_PredictMove
	CL_ClampPitch ();

	cmd->angles[0] = ANGLE2SHORT(cl.viewAngles[0]);
	cmd->angles[1] = ANGLE2SHORT(cl.viewAngles[1]);
	cmd->angles[2] = ANGLE2SHORT(cl.viewAngles[2]);

	// Update cmd->msec for CL_PredictMove
	ms = (int)(cls.netFrameTime * 1000);
	if (ms > 250)
		ms = 100;

	cmd->msec = ms;

	// Update counter
	sys_oldFrameTime = sys_frameTime;

	// Send packet immediately on important events
	if (btn_attack.state & 2 || btn_use.state & 2)
		cls.forcePacket = qTrue;
}


/*
=================
CL_FinalizeCmd
=================
*/
static void CL_FinalizeCmd (void)
{
	userCmd_t *cmd = &cl.cmds[cls.netChan.outgoingSequence & CMD_MASK];

	// Set any button hits that occured since last frame
	if (btn_attack.state & 3)
		cmd->buttons |= BUTTON_ATTACK;
	btn_attack.state &= ~2;

	if (btn_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	btn_use.state &= ~2;

	if (key_anyKeyDown && Key_GetDest () == KD_GAME)
		cmd->buttons |= BUTTON_ANY;

	// ...
	cmd->impulse = btn_impulse;
	btn_impulse = 0;

	// Set the ambient light level at the player's current position
	cmd->lightLevel = (byte)cl_lightlevel->value;
}


/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	byte		data[128];
	netMsg_t	buf;
	userCmd_t	*cmd, *oldCmd;
	userCmd_t	nullCmd;
	int			checkSumIndex;

	switch (Com_ClientState ()) {
	case CA_CONNECTED:
		if (cls.netChan.gotReliable || cls.netChan.message.curSize || Sys_Milliseconds() - cls.netChan.lastSent > 100)
			Netchan_Transmit (&cls.netChan, 0, NULL);
		return;

	case CA_DISCONNECTED:
	case CA_CONNECTING:
		// Wait until active
		return;
	}

	cl.cmdNum = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[cl.cmdNum];
	cl.cmdTime[cl.cmdNum] = cls.realTime;	// for ping calculation

	CL_FinalizeCmd ();

	cl.cmd = *cmd;

	// Send a userinfo update if needed
	if (com_userInfoModified) {
		com_userInfoModified = qFalse;
		MSG_WriteByte (&cls.netChan.message, CLC_USERINFO);
		MSG_WriteString (&cls.netChan.message, Cvar_BitInfo (CVAR_USERINFO));
	}

	MSG_Init (&buf, data, sizeof (data));

	if (cmd->buttons && cl.cin.time > 0 && !cl.attractLoop && cls.realTime - cl.cin.time > 1000) {
		// Skip the rest of the cinematic
		CIN_FinishCinematic ();
	}

	// Begin a client move command
	MSG_WriteByte (&buf, CLC_MOVE);

	// Save the position for a checksum byte
	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION) {
		checkSumIndex = buf.curSize;
		MSG_WriteByte (&buf, 0);
	}
	else
		checkSumIndex = 0;

	/*
	** Let the server know what the last frame we got was,
	** so the next message can be delta compressed
	*/
	if (cl_nodelta->value || !cl.frame.valid || cls.demoWaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverFrame);

	/*
	** Send this and the previous cmds in the message,
	** so if the last packet was dropped, it can be recovered
	*/
	cmd = &cl.cmds[(cls.netChan.outgoingSequence-2)&CMD_MASK];
	memset (&nullCmd, 0, sizeof (nullCmd));
	MSG_WriteDeltaUsercmd (&buf, &nullCmd, cmd);
	oldCmd = cmd;

	cmd = &cl.cmds[(cls.netChan.outgoingSequence-1)&CMD_MASK];
	MSG_WriteDeltaUsercmd (&buf, oldCmd, cmd);
	oldCmd = cmd;

	cmd = &cl.cmds[(cls.netChan.outgoingSequence)&CMD_MASK];
	MSG_WriteDeltaUsercmd (&buf, oldCmd, cmd);

	// Calculate a checksum over the move commands
	if (cls.serverProtocol != ENHANCED_PROTOCOL_VERSION) {
		buf.data[checkSumIndex] = Com_BlockSequenceCRCByte (
			buf.data + checkSumIndex + 1, buf.curSize - checkSumIndex - 1,
			cls.netChan.outgoingSequence);
	}

	// Deliver the message
	Netchan_Transmit (&cls.netChan, buf.curSize, buf.data);

	// Init the current cmd buffer and clear it
	cmd = &cl.cmds[cls.netChan.outgoingSequence&CMD_MASK];
	memset (cmd, 0, sizeof (*cmd));
}


/*
============
IN_CenterView
============
*/
void IN_CenterView (void)
{
	cl.viewAngles[PITCH] = -SHORT2ANGLE (cl.frame.playerState.pMove.deltaAngles[PITCH]);
}


/*
============
CL_InputInit
============
*/
void CL_InputInit (void)
{
	// Cvars
	cl_nodelta			= Cvar_Register ("cl_nodelta",			"0",		0);

	cl_upspeed			= Cvar_Register ("cl_upspeed",			"200",		0);
	cl_forwardspeed		= Cvar_Register ("cl_forwardspeed",		"200",		0);
	cl_sidespeed		= Cvar_Register ("cl_sidespeed",		"200",		0);
	cl_yawspeed			= Cvar_Register ("cl_yawspeed",			"140",		0);
	cl_pitchspeed		= Cvar_Register ("cl_pitchspeed",		"150",		0);

	cl_run				= Cvar_Register ("cl_run",				"0",		CVAR_ARCHIVE);

	cl_anglespeedkey	= Cvar_Register ("cl_anglespeedkey",	"1.5",		0);

	// Commands
	Cmd_AddCommand ("centerview",	IN_CenterView,		"Centers the view");

	Cmd_AddCommand ("+moveup",		IN_UpDown,			"");
	Cmd_AddCommand ("-moveup",		IN_UpUp,			"");
	Cmd_AddCommand ("+movedown",	IN_DownDown,		"");
	Cmd_AddCommand ("-movedown",	IN_DownUp,			"");
	Cmd_AddCommand ("+left",		IN_LeftDown,		"");
	Cmd_AddCommand ("-left",		IN_LeftUp,			"");
	Cmd_AddCommand ("+right",		IN_RightDown,		"");
	Cmd_AddCommand ("-right",		IN_RightUp,			"");
	Cmd_AddCommand ("+forward",		IN_ForwardDown,		"");
	Cmd_AddCommand ("-forward",		IN_ForwardUp,		"");
	Cmd_AddCommand ("+back",		IN_BackDown,		"");
	Cmd_AddCommand ("-back",		IN_BackUp,			"");
	Cmd_AddCommand ("+lookup",		IN_LookupDown,		"");
	Cmd_AddCommand ("-lookup",		IN_LookupUp,		"");
	Cmd_AddCommand ("+lookdown",	IN_LookdownDown,	"");
	Cmd_AddCommand ("-lookdown",	IN_LookdownUp,		"");
	Cmd_AddCommand ("+strafe",		IN_StrafeDown,		"");
	Cmd_AddCommand ("-strafe",		IN_StrafeUp,		"");
	Cmd_AddCommand ("+moveleft",	IN_MoveleftDown,	"");
	Cmd_AddCommand ("-moveleft",	IN_MoveleftUp,		"");
	Cmd_AddCommand ("+moveright",	IN_MoverightDown,	"");
	Cmd_AddCommand ("-moveright",	IN_MoverightUp,		"");
	Cmd_AddCommand ("+speed",		IN_SpeedDown,		"");
	Cmd_AddCommand ("-speed",		IN_SpeedUp,			"");
	Cmd_AddCommand ("+attack",		IN_AttackDown,		"");
	Cmd_AddCommand ("-attack",		IN_AttackUp,		"");
	Cmd_AddCommand ("+use",			IN_UseDown,			"");
	Cmd_AddCommand ("-use",			IN_UseUp,			"");

	Cmd_AddCommand ("impulse",		IN_Impulse,			"");

	Cmd_AddCommand ("+klook",		IN_KLookDown,		"");
	Cmd_AddCommand ("-klook",		IN_KLookUp,			"");
}
