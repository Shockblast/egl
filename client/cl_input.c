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

extern uInt	sys_frameTime;
static uInt	sys_oldFrameTime;
static uInt	in_frameMSec;

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

kButton_t	inBtnMoveUp;
kButton_t	inBtnMoveDown;
kButton_t	inBtnLookLeft;
kButton_t	inBtnLookRight;
kButton_t	inBtnForward;
kButton_t	inBtnBack;
kButton_t	inBtnLookUp;
kButton_t	inBtnLookDown;
kButton_t	inBtnMoveLeft;
kButton_t	inBtnMoveRight;

kButton_t	inBtnSpeed;
kButton_t	inBtnStrafe;
kButton_t	inBtnAttack;
kButton_t	inBtnUse;

kButton_t	inBtnKLook;

int			inBtnImpulse;

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
		Com_Printf (0, S_COLOR_YELLOW "Three keys down for a button!\n");
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
	uInt	uptime;

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


static void IN_UpDown (void)		{ KeyDown (&inBtnMoveUp); }
static void IN_UpUp (void)			{ KeyUp (&inBtnMoveUp); }
static void IN_DownDown (void)		{ KeyDown (&inBtnMoveDown); }
static void IN_DownUp (void)		{ KeyUp (&inBtnMoveDown); }
static void IN_LeftDown(void)		{ KeyDown (&inBtnLookLeft); }
static void IN_LeftUp (void)		{ KeyUp (&inBtnLookLeft); }
static void IN_RightDown (void)		{ KeyDown (&inBtnLookRight); }
static void IN_RightUp (void)		{ KeyUp (&inBtnLookRight); }
static void IN_ForwardDown (void)	{ KeyDown (&inBtnForward); }
static void IN_ForwardUp (void)		{ KeyUp (&inBtnForward); }
static void IN_BackDown (void)		{ KeyDown (&inBtnBack); }
static void IN_BackUp (void)		{ KeyUp (&inBtnBack); }
static void IN_LookupDown (void)	{ KeyDown (&inBtnLookUp); }
static void IN_LookupUp (void)		{ KeyUp (&inBtnLookUp); }
static void IN_LookdownDown (void)	{ KeyDown (&inBtnLookDown); }
static void IN_LookdownUp (void)	{ KeyUp (&inBtnLookDown); }
static void IN_MoveleftDown (void)	{ KeyDown (&inBtnMoveLeft); }
static void IN_MoveleftUp (void)	{ KeyUp (&inBtnMoveLeft); }
static void IN_MoverightDown (void)	{ KeyDown (&inBtnMoveRight); }
static void IN_MoverightUp (void)	{ KeyUp (&inBtnMoveRight); }

static void IN_SpeedDown (void)		{ KeyDown (&inBtnSpeed); }
static void IN_SpeedUp (void)		{ KeyUp (&inBtnSpeed); }
static void IN_StrafeDown (void)	{ KeyDown (&inBtnStrafe); }
static void IN_StrafeUp (void)		{ KeyUp (&inBtnStrafe); }

static void IN_AttackDown (void)	{ KeyDown (&inBtnAttack); }
static void IN_AttackUp (void)		{ KeyUp (&inBtnAttack); }

static void IN_UseDown  (void)		{ KeyDown (&inBtnUse); }
static void IN_UseUp  (void)		{ KeyUp (&inBtnUse); }

static void IN_KLookDown (void)		{ KeyDown (&inBtnKLook); }
static void IN_KLookUp (void)		{ KeyUp (&inBtnKLook); }

static void IN_Impulse  (void)		{ inBtnImpulse=atoi (Cmd_Argv (1)); }

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
	if (inBtnSpeed.state & 1)
		tspeed = cls.netFrameTime * cl_anglespeedkey->value;
	else
		tspeed = cls.netFrameTime;

	// Adjust for running speed
	if ((inBtnSpeed.state & 1) ^ cl_run->integer)
		mspeed = 2;
	else
		mspeed = 1;

	// Handle left/right on keyboard
	cl.cmdNum = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[cl.cmdNum];
	cl.cmdTime[cl.cmdNum] = cls.realTime;	// for netgraph ping calculation

	if (inBtnStrafe.state & 1) {
		// Keyboard strafe
		cmd->sideMove += cl_sidespeed->value * CL_KeyState (&inBtnLookRight);
		cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&inBtnLookLeft);
	}
	else {
		// Keyboard turn
		cl.viewAngles[YAW] -= tspeed * cl_yawspeed->value * CL_KeyState (&inBtnLookRight);
		cl.viewAngles[YAW] += tspeed * cl_yawspeed->value * CL_KeyState (&inBtnLookLeft);
	}

	if (inBtnKLook.state & 1) {
		// Keyboard look
		cl.viewAngles[PITCH] -= tspeed * cl_pitchspeed->value * CL_KeyState (&inBtnForward);
		cl.viewAngles[PITCH] += tspeed * cl_pitchspeed->value * CL_KeyState (&inBtnBack);
	}
	else {
		// Keyboard move front/back
		cmd->forwardMove += cl_forwardspeed->value * CL_KeyState (&inBtnForward);
		cmd->forwardMove -= cl_forwardspeed->value * CL_KeyState (&inBtnBack);
	}

	// Keyboard look up/down
	cl.viewAngles[PITCH] -= tspeed * cl_pitchspeed->value * CL_KeyState (&inBtnLookUp);
	cl.viewAngles[PITCH] += tspeed * cl_pitchspeed->value * CL_KeyState (&inBtnLookDown);

	// Keyboard strafe left/right
	cmd->sideMove += cl_sidespeed->value * CL_KeyState (&inBtnMoveRight);
	cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&inBtnMoveLeft);

	// Keyboard jump/crouch
	cmd->upMove += cl_upspeed->value * CL_KeyState (&inBtnMoveUp);
	cmd->upMove -= cl_upspeed->value * CL_KeyState (&inBtnMoveDown);

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

	// Bounds checking
	if (in_frameMSec < 1)
		return;

	if (in_frameMSec > 1000)
		in_frameMSec = 500;

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
	ms = cls.netFrameTime * 1000;
	if (ms > 250)
		ms = 100;

	cmd->msec = ms;

	// Update counter
	sys_oldFrameTime = sys_frameTime;

	// Send packet immediately on important events
	if (inBtnAttack.state & 2 || inBtnUse.state & 2)
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
	if (inBtnAttack.state & 3)
		cmd->buttons |= BUTTON_ATTACK;
	inBtnAttack.state &= ~2;

	if (inBtnUse.state & 3)
		cmd->buttons |= BUTTON_USE;
	inBtnUse.state &= ~2;

	if (key_anyKeyDown && Key_KeyDest (KD_GAME))
		cmd->buttons |= BUTTON_ANY;

	// ...
	cmd->impulse = inBtnImpulse;
	inBtnImpulse = 0;

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

	switch (cls.connectState) {
	case CA_CONNECTED:
		if (cls.netChan.message.curSize || sys_currTime - cls.netChan.lastSent > 1000) {
	 		memset (&buf, 0, sizeof (buf));
			Com_Printf (PRNT_DEV, "Connected -- flushing netchan (len=%d, '%s')\n", cls.netChan.message.curSize, cls.netChan.message.data);
			Netchan_Transmit (&cls.netChan, 0, buf.data);
		}

	// Wait until active
	case CA_DISCONNECTED:
	case CA_CONNECTING:
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

	if (cmd->buttons && (cl.cin.time > 0) && !cl.attractLoop && ((cls.realTime - cl.cin.time) > 1000)) {
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
CL_InitInput
============
*/
void CL_InitInput (void)
{
	// Cvars
	cl_nodelta			= Cvar_Get ("cl_nodelta",		"0",		0);

	cl_upspeed			= Cvar_Get ("cl_upspeed",		"200",		0);
	cl_forwardspeed		= Cvar_Get ("cl_forwardspeed",	"200",		0);
	cl_sidespeed		= Cvar_Get ("cl_sidespeed",		"200",		0);
	cl_yawspeed			= Cvar_Get ("cl_yawspeed",		"140",		0);
	cl_pitchspeed		= Cvar_Get ("cl_pitchspeed",	"150",		0);

	cl_run				= Cvar_Get ("cl_run",			"0",		CVAR_ARCHIVE);

	cl_anglespeedkey	= Cvar_Get ("cl_anglespeedkey",	"1.5",		0);

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
