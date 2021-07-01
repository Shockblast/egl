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

extern uInt	sysFrameTime;
static uInt	sysOldFrameTime;
static uInt	inFrameMSec;

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

kButton_t	in_MoveUp;
kButton_t	in_MoveDown;
kButton_t	in_LookLeft;
kButton_t	in_LookRight;
kButton_t	in_Forward;
kButton_t	in_Back;
kButton_t	in_LookUp;
kButton_t	in_LookDown;
kButton_t	in_MoveLeft;
kButton_t	in_MoveRight;

kButton_t	in_Speed;
kButton_t	in_Strafe;
kButton_t	in_Attack;
kButton_t	in_Use;

kButton_t	in_KLook;

int			in_Impulse;

/*
====================
KeyDown
====================
*/
static void KeyDown (kButton_t *b) {
	int		k;
	char	*c;
	
	c = Cmd_Argv (1);
	if (c[0])
		k = atoi (c);
	else
		k = -1;	// typed manually at the console for continuous down

	if ((k == b->down[0]) || (k == b->down[1]))
		return;	// repeating key
	
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

	// save timestamp
	c = Cmd_Argv (2);
	b->downTime = atoi(c);
	if (!b->downTime)
		b->downTime = sysFrameTime - 100;

	b->state |= 1 + 2;	// down + impulse down
}


/*
====================
KeyUp
====================
*/
static void KeyUp (kButton_t *b) {
	int		k;
	char	*c;
	uInt	uptime;

	c = Cmd_Argv (1);
	if (c[0])
		k = atoi(c);
	else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	// save timestamp
	c = Cmd_Argv (2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downTime;
	else
		b->msec += 10;

	b->state &= ~1;		// now up
	b->state |= 4;		// impulse up
}


/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState (kButton_t *key) {
	int			msec;

	key->state &= 1;		// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state) {
		// still down
		msec += sysFrameTime - key->downTime;
		key->downTime = sysFrameTime;
	}

	return clamp ((float)msec / (float)inFrameMSec, 0, 1);
}


static void IN_UpDown (void)		{ KeyDown (&in_MoveUp); }
static void IN_UpUp (void)			{ KeyUp (&in_MoveUp); }
static void IN_DownDown (void)		{ KeyDown (&in_MoveDown); }
static void IN_DownUp (void)		{ KeyUp (&in_MoveDown); }
static void IN_LeftDown(void)		{ KeyDown (&in_LookLeft); }
static void IN_LeftUp (void)		{ KeyUp (&in_LookLeft); }
static void IN_RightDown (void)		{ KeyDown (&in_LookRight); }
static void IN_RightUp (void)		{ KeyUp (&in_LookRight); }
static void IN_ForwardDown (void)	{ KeyDown (&in_Forward); }
static void IN_ForwardUp (void)		{ KeyUp (&in_Forward); }
static void IN_BackDown (void)		{ KeyDown (&in_Back); }
static void IN_BackUp (void)		{ KeyUp (&in_Back); }
static void IN_LookupDown (void)	{ KeyDown (&in_LookUp); }
static void IN_LookupUp (void)		{ KeyUp (&in_LookUp); }
static void IN_LookdownDown (void)	{ KeyDown (&in_LookDown); }
static void IN_LookdownUp (void)	{ KeyUp (&in_LookDown); }
static void IN_MoveleftDown (void)	{ KeyDown (&in_MoveLeft); }
static void IN_MoveleftUp (void)	{ KeyUp (&in_MoveLeft); }
static void IN_MoverightDown (void)	{ KeyDown (&in_MoveRight); }
static void IN_MoverightUp (void)	{ KeyUp (&in_MoveRight); }

static void IN_SpeedDown (void)		{ KeyDown (&in_Speed); }
static void IN_SpeedUp (void)		{ KeyUp (&in_Speed); }
static void IN_StrafeDown (void)	{ KeyDown (&in_Strafe); }
static void IN_StrafeUp (void)		{ KeyUp (&in_Strafe); }

static void IN_AttackDown (void)	{ KeyDown (&in_Attack); }
static void IN_AttackUp (void)		{ KeyUp (&in_Attack); }

static void IN_UseDown  (void)		{ KeyDown (&in_Use); }
static void IN_UseUp  (void)		{ KeyUp (&in_Use); }

static void IN_KLookDown (void)		{ KeyDown (&in_KLook); }
static void IN_KLookUp (void)		{ KeyUp (&in_KLook); }

static void IN_Impulse  (void)		{ in_Impulse=atoi (Cmd_Argv (1)); }

//==========================================================================

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles (void) {
	float	speed;
	float	up, down;
	
	if (in_Speed.state & 1)
		speed = cls.frameTime * cl_anglespeedkey->value;
	else
		speed = cls.frameTime;

	if (!(in_Strafe.state & 1)) {
		cl.viewAngles[YAW] -= speed * cl_yawspeed->value * CL_KeyState (&in_LookRight);
		cl.viewAngles[YAW] += speed * cl_yawspeed->value * CL_KeyState (&in_LookLeft);
	}

	if (in_KLook.state & 1) {
		cl.viewAngles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState (&in_Forward);
		cl.viewAngles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState (&in_Back);
	}
	
	up = CL_KeyState (&in_LookUp);
	down = CL_KeyState (&in_LookDown);
	
	cl.viewAngles[PITCH] -= speed * cl_pitchspeed->value * up;
	cl.viewAngles[PITCH] += speed * cl_pitchspeed->value * down;
}


/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
static void CL_BaseMove (userCmd_t *cmd) {	
	CL_AdjustAngles ();
	
	memset (cmd, 0, sizeof (*cmd));
	
	VectorCopy (cl.viewAngles, cmd->angles);
	if (in_Strafe.state & 1) {
		cmd->sideMove += cl_sidespeed->value * CL_KeyState (&in_LookRight);
		cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&in_LookLeft);
	}

	cmd->sideMove += cl_sidespeed->value * CL_KeyState (&in_MoveRight);
	cmd->sideMove -= cl_sidespeed->value * CL_KeyState (&in_MoveLeft);

	cmd->upMove += cl_upspeed->value * CL_KeyState (&in_MoveUp);
	cmd->upMove -= cl_upspeed->value * CL_KeyState (&in_MoveDown);

	if (!(in_KLook.state & 1)) {	
		cmd->forwardMove += cl_forwardspeed->value * CL_KeyState (&in_Forward);
		cmd->forwardMove -= cl_forwardspeed->value * CL_KeyState (&in_Back);
	}	

	//
	// adjust for speed key / running
	//
	if ((in_Speed.state & 1) ^ cl_run->integer) {
		cmd->forwardMove *= 2;
		cmd->sideMove *= 2;
		cmd->upMove *= 2;
	}	
}


/*
==============
CL_ClampPitch
==============
*/
static void CL_ClampPitch (void) {
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
==============
CL_FinishMove
==============
*/
static void CL_FinishMove (userCmd_t *cmd) {
	int		ms;
	int		i;

	// figure button bits
	if (in_Attack.state & 3)
		cmd->buttons |= BUTTON_ATTACK;
	in_Attack.state &= ~2;
	
	if (in_Use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_Use.state &= ~2;

	if (keyAnyKeyDown && Key_KeyDest (KD_GAME))
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = cls.frameTime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch ();
	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ANGLE2SHORT (cl.viewAngles[i]);

	cmd->impulse = in_Impulse;
	in_Impulse = 0;

	// send the ambient light level at the player's current position
	cmd->lightLevel = (byte)cl_lightlevel->value;
}


/*
=================
CL_CreateCmd
=================
*/
static userCmd_t CL_CreateCmd (void) {
	userCmd_t	cmd;

	inFrameMSec = clamp (sysFrameTime - sysOldFrameTime, 1, 200);

	// get basic movement from keyboard
	CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
	IN_Move (&cmd);

	CL_FinishMove (&cmd);

	sysOldFrameTime = sysFrameTime;

	return cmd;
}


/*
============
IN_CenterView
============
*/
void IN_CenterView (void) {
	cl.viewAngles[PITCH] = -SHORT2ANGLE (cl.frame.playerState.pMove.deltaAngles[PITCH]);
}


/*
============
CL_InitInput
============
*/
void CL_InitInput (void) {
	// cvars
	cl_nodelta			= Cvar_Get ("cl_nodelta",		"0",		0);

	cl_upspeed			= Cvar_Get ("cl_upspeed",		"200",		0);
	cl_forwardspeed		= Cvar_Get ("cl_forwardspeed",	"200",		0);
	cl_sidespeed		= Cvar_Get ("cl_sidespeed",		"200",		0);
	cl_yawspeed			= Cvar_Get ("cl_yawspeed",		"140",		0);
	cl_pitchspeed		= Cvar_Get ("cl_pitchspeed",	"150",		0);

	cl_run				= Cvar_Get ("cl_run",			"0",		CVAR_ARCHIVE);

	cl_anglespeedkey	= Cvar_Get ("cl_anglespeedkey",	"1.5",		0);

	// commands
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


/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void) {
	byte		data[128];
	netMsg_t	buf;
	userCmd_t	*cmd, *oldCmd;
	userCmd_t	nullCmd;
	int			checksumIndex;

	memset (&buf, 0, sizeof (buf));
	// build a command even if not connected

	// save this command off for prediction
	cl.cmdNum = cls.netChan.outgoingSequence & CMD_MASK;
	cmd = &cl.cmds[cl.cmdNum];
	cl.cmdTime[cl.cmdNum] = cls.realTime;	// for ping calculation

	cl.cmd = *cmd = CL_CreateCmd ();

	if ((cls.connState == CA_DISCONNECTED) || (cls.connState == CA_CONNECTING))
		return;

	if (cls.connState == CA_CONNECTED) {
		if (cls.netChan.message.curSize	|| ((sysTime - cls.netChan.lastSent) > 1000))
			Netchan_Transmit (&cls.netChan, 0, buf.data);

		return;
	}

	// send a userinfo update if needed
	if (cvarUserInfoModified) {
		cvarUserInfoModified = qFalse;
		MSG_WriteByte (&cls.netChan.message, CLC_USERINFO);
		MSG_WriteString (&cls.netChan.message, Cvar_Userinfo ());
	}

	MSG_Init (&buf, data, sizeof (data));

	if (cmd->buttons && (cl.cinematicTime > 0) && !cl.attractLoop && ((cls.realTime - cl.cinematicTime) > 1000)) {
		// skip the rest of the cinematic
		CIN_FinishCinematic ();
	}

	// begin a client move command
	MSG_WriteByte (&buf, CLC_MOVE);

	// save the position for a checksum byte
	checksumIndex = buf.curSize;
	MSG_WriteByte (&buf, 0);

	/*
	** let the server know what the last frame we got was,
	** so the next message can be delta compressed
	*/
	if (cl_nodelta->value || !cl.frame.valid || cls.demoWaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverFrame);

	/*
	** send this and the previous cmds in the message,
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

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = Com_BlockSequenceCRCByte (
		buf.data + checksumIndex + 1, buf.curSize - checksumIndex - 1,
		cls.netChan.outgoingSequence);

	// deliver the message
	Netchan_Transmit (&cls.netChan, buf.curSize, buf.data);
}
