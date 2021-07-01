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
// common.c
// Functions used in client and server
//

#include "common.h"
#include <setjmp.h>

jmp_buf	abortframe;		// an ERR_DROP occured, exit the entire frame

FILE	*logStatsFP;
FILE	*logFileFP;

cVar_t	*host_speeds;
cVar_t	*log_stats;
cVar_t	*developer;
cVar_t	*timescale;
cVar_t	*fixedtime;
cVar_t	*logfile;		// 1 = buffer log, 2 = flush after each print
cVar_t	*showtrace;
cVar_t	*dedicated;

// host_speeds times
int		timeBeforeGame;
int		timeAfterGame;
int		timeBeforeRefresh;
int		timeAfterRefresh;

qBool	comInitialized;

/*
============================================================================

	CLIENT / SERVER INTERACTIONS

============================================================================
*/

#define	MAXPRINTMSG		4096

static int	rdTarget;
static char	*rdBuffer;
static int	rdBufferSize;
static void	(*rdFlush) (int target, char *buffer);

/*
================
Com_BeginRedirect
================
*/
void Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush)) {
	if (!target || !buffer || !bufferSize || !flush)
		return;
	rdTarget = target;
	rdBuffer = buffer;
	rdBufferSize = bufferSize;
	rdFlush = flush;

	*rdBuffer = 0;
}


/*
================
Com_EndRedirect
================
*/
void Com_EndRedirect (void) {
	rdFlush (rdTarget, rdBuffer);

	rdTarget = 0;
	rdBuffer = NULL;
	rdBufferSize = 0;
	rdFlush = NULL;
}


/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/
void Com_Printf (int flags, char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if ((flags & PRNT_DEV) && (!developer || !developer->value))
		return; // don't confuse non-developers with techie stuff
	
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	if (rdTarget) {
		if ((Q_strlen (msg) + Q_strlen (rdBuffer)) > (rdBufferSize - 1)) {
			rdFlush (rdTarget, rdBuffer);
			*rdBuffer = 0;
		}
		strcat (rdBuffer, msg);
		return;
	}

	Con_Print (flags, msg);
		
	// also echo to debugging console
	Sys_ConsoleOutput (msg);

	if (flags & PRNT_ALERT) {
		Sys_Alert (msg);
	}

	// logfile
	if (logfile && logfile->value) {
		char	name[MAX_QPATH];
		
		if (!logFileFP) {
			Q_snprintfz (name, sizeof (name), "%s/qconsole.log", FS_Gamedir ());
			if (logfile->value > 2)
				logFileFP = fopen (name, "a");
			else
				logFileFP = fopen (name, "w");
		}
		if (logFileFP)
			fprintf (logFileFP, "%s", msg);
		if (logfile->value > 1)
			fflush (logFileFP);		// force it to save every time
	}
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error (int code, char *fmt, ...) {
	va_list			argptr;
	static char		msg[MAXPRINTMSG];
	static qBool	recursive = qFalse;

	if (recursive)
		Sys_Error ("Com_Error: Recursive error after: %s", msg);
	recursive = qTrue;

	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	switch (code) {
	case ERR_DISCONNECT:
		CL_Disconnect ();
		recursive = qFalse;
		if (!comInitialized)
			Sys_Error ("%s", msg);
		longjmp (abortframe, -1);
		break;

	case ERR_DROP:
		Com_Printf (0, "********************\nERROR: %s\n********************\n", msg);
		SV_Shutdown (Q_VarArgs ("Server exited: %s\n", msg), qFalse, qFalse);
		CL_Disconnect ();
		recursive = qFalse;
		if (!comInitialized)
			Sys_Error ("%s", msg);
		longjmp (abortframe, -1);
		break;

	default:
		SV_Shutdown (Q_VarArgs ("Server fatal crashed: %s\n", msg), qFalse, qTrue);
		CL_Shutdown ();
		break;
	}

	if (logFileFP) {
		fclose (logFileFP);
		logFileFP = NULL;
	}

	Sys_Error ("%s", msg);
}


/*
=============
Com_Error_f

Just throw a fatal error to test error shutdown procedures
=============
*/
void Com_Error_f (void) {
	Com_Error (ERR_FATAL, "%s", Cmd_Argv (1));
}


/*
=============
Com_Quit

Both client and server can use this, and it will do the apropriate things.
=============
*/
void Com_Quit (void) {
	if (Cmd_Argc () > 1)
		SV_Shutdown (Q_VarArgs ("Server has shut down: %s\n", Cmd_Args ()), qFalse, qFalse);
	else
		SV_Shutdown ("Server has shut down\n", qFalse, qFalse);

	if (logFileFP) {
		fclose (logFileFP);
		logFileFP = NULL;
	}

	Sys_Quit ();
}

/*
============================================================================

	CLIENT / SERVER STATES

	Non-zero state values indicate initialization
============================================================================
*/

static int	svState;
static int	clState;

/*
==================
Com_ClientState
==================
*/
int Com_ClientState (void) {
	return clState;
}


/*
==================
Com_SetClientState
==================
*/
void Com_SetClientState (int state) {
	clState = state;
}


/*
==================
Com_ServerState
==================
*/
int Com_ServerState (void) {
	return svState;
}


/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (int state) {
	svState = state;
}

/*
============================================================================

	PROGRAM ARGUMENTS

============================================================================
*/

#define MAX_NUM_ARGVS	51

static int	comArgC;
static char	*comArgV[MAX_NUM_ARGVS];

/*
================
Com_InitArgv
================
*/
static void Com_InitArgv (int argc, char **argv) {
	int		i;

	if (argc >= MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc >= MAX_NUM_ARGVS");

	comArgC = argc;
	for (i=0 ; i<argc ; i++) {
		if (!argv[i] || (Q_strlen (argv[i]) >= MAX_TOKEN_CHARS))
			comArgV[i] = "";
		else
			comArgV[i] = argv[i];
	}
}


/*
================
Com_ClearArgv
================
*/
static void Com_ClearArgv (int arg) {
	if ((arg < 0) || (arg >= comArgC) || !comArgV[arg])
		return;

	comArgV[arg] = "";
}


/*
================
Com_Argc
================
*/
static int Com_Argc (void) {
	return comArgC;
}


/*
================
Com_Argv
================
*/
static char *Com_Argv (int arg) {
	if ((arg < 0) || (arg >= comArgC) || !comArgV[arg])
		return "";

	return comArgV[arg];
}


/*
===============
Com_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
static void Com_AddEarlyCommands (qBool clear) {
	int		i;
	char	*s;

	for (i=0 ; i<Com_Argc () ; i++) {
		s = Com_Argv (i);
		if (strcmp (s, "+set"))
			continue;

		Cbuf_AddText (Q_VarArgs ("set %s %s\n", Com_Argv (i+1), Com_Argv (i+2)));
		if (clear) {
			Com_ClearArgv (i);
			Com_ClearArgv (i+1);
			Com_ClearArgv (i+2);
		}
		i+=2;
	}
}


/*
=================
Com_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
egl +map amlev1 +

Returns qTrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
static qBool Com_AddLateCommands (void) {
	int		i, j;
	int		s;
	char	*text, *build, c;
	int		argc;
	qBool	ret;

	// build the combined string to parse from
	s = 0;
	argc = Com_Argc ();
	for (i=1 ; i<argc ; i++)
		s += Q_strlen (Com_Argv (i)) + 1;

	if (!s)
		return qFalse;
		
	text = Mem_Alloc (s+1);
	text[0] = 0;
	for (i=1 ; i<argc ; i++) {
		strcat (text, Com_Argv (i));
		if (i != argc-1)
			strcat (text, " ");
	}
	
	// pull out the commands
	build = Mem_Alloc (s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++) {
		if (text[i] == '+') {
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++) ;

			c = text[j];
			text[j] = 0;
			
			strcat (build, text+i);
			strcat (build, "\n");
			text[j] = c;
			i = j-1;
		}
	}

	ret = (build[0] != 0);
	if (ret)
		Cbuf_AddText (build);
	
	Mem_Free (text);
	Mem_Free (build);

	return ret;
}

/*
============================================================================

	INITIALIZATION / FRAME PROCESSING

============================================================================
*/

/*
=================
Com_Init
=================
*/
void Com_Init (int argc, char **argv) {
	int		eglInitTime;
	char	*verString;

	eglInitTime = Sys_Milliseconds ();
	if (setjmp (abortframe))
		Sys_Error ("Error during initialization");

	seedMT ((unsigned long)time(0));

	Mem_Init ();

	// prepare enough of the subsystems to handle cvar and command buffer management
	Com_InitArgv (argc, argv);

	Cmd_Init ();
	Cbuf_Init ();
	Alias_Init ();
	Cvar_Init ();
	Key_Init ();

	// init information variables
	verString = Q_VarArgs ("EGL v%s: %s %s-%s", EGL_VERSTR, __DATE__, BUILDSTRING, CPUSTRING);
	Cvar_Get ("cl_version", verString, CVAR_NOSET);
	Cvar_Get ("version", verString, CVAR_SERVERINFO|CVAR_NOSET);
	Cvar_Get ("vid_ref", verString, CVAR_NOSET);

	/*
	** we need to add the early commands twice, because a basedir or cddir needs to be set before execing
	** config files, but we want other parms to override the settings of the config files
	*/
	Com_AddEarlyCommands (qFalse);
	Cbuf_Execute ();

#ifdef DEDICATED_ONLY
	dedicated		= Cvar_Get ("dedicated",	"1",	CVAR_NOSET);
#else
	dedicated		= Cvar_Get ("dedicated",	"0",	CVAR_NOSET);
#endif

	Sys_Init ();
	Con_Init ();

	Com_Printf (0, "========= Common Initialization ========\n");
	Com_Printf (0, "EGL v%s by Echon\nhttp://www.echon.org/\n", EGL_VERSTR);
	Com_Printf (0, "Compiled: "__DATE__" @ "__TIME__"\n");
	Com_Printf (0, "----------------------------------------\n");

	FS_Init ();

	Com_Printf (0, "\n");

	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_AddText ("exec config.cfg\n");
	Cbuf_AddText ("exec eglcfg.cfg\n");

	Com_AddEarlyCommands (qTrue);
	Cbuf_Execute ();

	// init commands and vars
	Mem_Register ();

	Cmd_AddCommand ("error",	Com_Error_f,	"Error out with a message");

	host_speeds		= Cvar_Get ("host_speeds",	"0",	0);
	developer		= Cvar_Get ("developer",	"0",	0);
	timescale		= Cvar_Get ("timescale",	"1",	0);
	fixedtime		= Cvar_Get ("fixedtime",	"0",	0);
	logfile			= Cvar_Get ("logfile",		"0",	0);
	log_stats		= Cvar_Get ("log_stats",	"0",	0);
	showtrace		= Cvar_Get ("showtrace",	"0",	0);

	if (dedicated->integer) {
		Cmd_AddCommand ("quit", Com_Quit,		"Exits");
		Cmd_AddCommand ("exit", Com_Quit,		"Exits");
	}
	else {
		FS_ExecAutoexec ();
		Cbuf_Execute ();
	}

	// init the rest of the sub-systems
	NET_Init ();
	Netchan_Init ();

	SV_Init ();
	CL_Init ();

	// add + commands from command line
	if (!Com_AddLateCommands ()) {
		if (dedicated->integer)
			Cbuf_AddText ("dedicated_start\n");

		Cbuf_Execute ();
	}

	SCR_EndLoadingPlaque ();

	comInitialized = qTrue;

	Com_Printf (0, "======= Common Initialized %5.2fs ======\n\n", (Sys_Milliseconds () - eglInitTime)*0.001f);
}


/*
=================
Com_Frame
=================
*/
void __fastcall Com_Frame (int msec) {
	char	*conInput;
	int		timeBeforeSVFrame;
	int		timeBetweenSC;
	int		timeAfterCLFrame;

	if (setjmp (abortframe))
		return;			// an ERR_DROP was thrown

	if (log_stats->modified) {
		log_stats->modified = qFalse;
		if (log_stats->value) {
			if (logStatsFP) {
				fclose (logStatsFP);
				logStatsFP = 0;
			}

			logStatsFP = fopen ("stats.log", "w");
			if (logStatsFP)
				fprintf (logStatsFP, "entities,dlights,parts,frame time\n");
		}
		else {
			if (logStatsFP) {
				fclose (logStatsFP);
				logStatsFP = 0;
			}
		}
	}

	if (fixedtime->value)
		msec = fixedtime->value;
	else if (timescale->value) {
		msec *= timescale->value;
		if (msec < 1)
			msec = 1;
	}

	if (showtrace->value) {
		extern	int cmTraces;
		extern	int	cmBrushTraces;
		extern	int	cmPointContents;

		Com_Printf (0, "%4i traces %4i brtraces %4i points\n", cmTraces, cmBrushTraces, cmPointContents);

		// reset
		cmTraces = 0;
		cmBrushTraces = 0;
		cmPointContents = 0;
	}

	do {
		conInput = Sys_ConsoleInput ();
		if (conInput)
			Cbuf_AddText (Q_VarArgs ("%s\n", conInput));
	} while (conInput);
	Cbuf_Execute ();

	if (host_speeds->value)
		timeBeforeSVFrame = Sys_Milliseconds ();

	SV_Frame (msec);

	if (host_speeds->value)
		timeBetweenSC = Sys_Milliseconds ();

	CL_Frame (msec);

	if (host_speeds->value)
		timeAfterCLFrame = Sys_Milliseconds ();

	if (host_speeds->value) {
		int		serverTime;
		int		gameTime;
		int		clientTime;
		int		refreshTime;

		serverTime	= timeBetweenSC - timeBeforeSVFrame;
		clientTime	= timeAfterCLFrame - timeBetweenSC;
		gameTime	= timeAfterGame - timeBeforeGame;
		refreshTime	= timeAfterRefresh - timeBeforeRefresh;
		serverTime	-= gameTime;
		clientTime	-= refreshTime;
		Com_Printf (0, "all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
						(timeAfterCLFrame - timeBeforeSVFrame), serverTime, gameTime, clientTime, refreshTime);
	}
}


/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown (void) {
	Cmd_Shutdown ();
	Cbuf_Shutdown ();
	Alias_Shutdown ();
	Cvar_Shutdown ();
	Key_Shutdown ();
	FS_Shutdown ();
	NET_Shutdown (qTrue);

	Mem_Shutdown ();
}
