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

// common.c
// - misc functions used in client and server

#include "common.h"
#include <setjmp.h>

jmp_buf	abortframe;		// an ERR_DROP occured, exit the entire frame

FILE	*logStatsFP;
FILE	*logFileFP;

cvar_t	*host_speeds;
cvar_t	*log_stats;
cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*fixedtime;
cvar_t	*logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*showtrace;
cvar_t	*dedicated;

// host_speeds times
int		time_before_game;
int		time_after_game;
int		time_before_ref;
int		time_after_ref;

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

static int	svState;

/*
================
Com_BeginRedirect
================
*/
void Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush)) {
	if (!target || !buffer || !buffersize || !flush)
		return;
	rdTarget = target;
	rdBuffer = buffer;
	rdBufferSize = buffersize;
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

	if ((flags & PRINT_DEV) && (!developer || !developer->value))
		return; // don't confuse non-developers with techie stuff
	
	va_start (argptr, fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if (rdTarget) {
		if ((strlen (msg) + strlen (rdBuffer)) > (rdBufferSize - 1)) {
			rdFlush (rdTarget, rdBuffer);
			*rdBuffer = 0;
		}
		strcat (rdBuffer, msg);
		return;
	}

	Con_Print (flags, msg);
		
	// also echo to debugging console
	Sys_ConsoleOutput (msg);

	// logfile
	if (logfile && logfile->value) {
		char	name[MAX_QPATH];
		
		if (!logFileFP) {
			Com_sprintf (name, sizeof (name), "%s/qconsole.log", FS_Gamedir ());
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
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if (code == ERR_DISCONNECT) {
		CL_Drop ();
		recursive = qFalse;
		longjmp (abortframe, -1);
	} else if (code == ERR_DROP) {
		Com_Printf (PRINT_ALL, "********************\nERROR: %s\n********************\n", msg);
		SV_Shutdown (va ("Server crashed: %s\n", msg), qFalse);
		CL_Drop ();
		recursive = qFalse;
		longjmp (abortframe, -1);
	} else {
		SV_Shutdown (va ("Server fatal crashed: %s\n", msg), qFalse);
		CL_Shutdown ();
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
	SV_Shutdown ("Server quit\n", qFalse);

	if (logFileFP) {
		fclose (logFileFP);
		logFileFP = NULL;
	}

	Sys_Quit ();
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

	MISC

============================================================================
*/

#define RANDSIZE	32767
#define ONEDIVRS	0.00003051850947599719f
#define TWODIVRS	0.00006103701895199439f

/*
=================
crand

-1 to 1
=================
*/
float crand (void) {
	return (rand () & RANDSIZE) * TWODIVRS - 1.0f;
}


/*
=================
frand

0 to 1
=================
*/
float frand (void) {
	return (rand () & RANDSIZE) * ONEDIVRS;
}


/*
==================
Com_WildCmp

from Q2ICE
==================
*/
int Com_WildCmp (const char *filter, const char *string, int ignoreCase) {
	switch (*filter) {
	case '\0':	return !*string;
	case '*':	return Com_WildCmp (filter + 1, string, ignoreCase) || *string && Com_WildCmp (filter, string + 1, ignoreCase);
	case '?':	return *string && Com_WildCmp (filter + 1, string + 1, ignoreCase);
	default:	return ((*filter == *string) || (ignoreCase && (toupper (*filter) == toupper (*string)))) && Com_WildCmp (filter + 1, string + 1, ignoreCase);
	}
}


/*
==================
Com_WriteConfig
==================
*/
void Com_WriteConfig (void) {
	FILE	*f;
	char	path[MAX_QPATH];

	Cvar_GetLatchedVars (CVAR_LATCHVIDEO);
	Cvar_GetLatchedVars (CVAR_LATCH);

	Com_Printf (PRINT_ALL, "Saving configuration...\n");

	if (Cmd_Argc () == 2)
		Com_sprintf (path, sizeof (path), "%s/%s", FS_Gamedir (), Cmd_Argv (1));
	else
		Com_sprintf (path, sizeof (path), "%s/eglcfg.cfg", FS_Gamedir ());

	f = fopen (path, "w");
	if (!f) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "ERROR: Couldn't write %s\n", path);
		return;
	}

	fprintf (f, "// Configuration file generated by EGL, modify at risk\n");

	Key_WriteBindings (f);
	Cvar_WriteVariables (f);

	fclose (f);
	Com_Printf (PRINT_ALL, "Saved to %s\n", path);
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
		if (!argv[i] || (strlen(argv[i]) >= MAX_TOKEN_CHARS))
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
void Com_ClearArgv (int arg) {
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

		Cbuf_AddText (va("set %s %s\n", Com_Argv (i+1), Com_Argv (i+2)));
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

	/* build the combined string to parse from */
	s = 0;
	argc = Com_Argc ();
	for (i=1 ; i<argc ; i++)
		s += strlen (Com_Argv (i)) + 1;

	if (!s)
		return qFalse;
		
	text = Z_TagMalloc (s+1, ZTAG_CMDSYS);
	text[0] = 0;
	for (i=1 ; i<argc ; i++) {
		strcat (text, Com_Argv (i));
		if (i != argc-1)
			strcat (text, " ");
	}
	
	/* pull out the commands */
	build = Z_TagMalloc (s+1, ZTAG_CMDSYS);
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
	
	Z_Free (text);
	Z_Free (build);

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

	eglInitTime = Sys_Milliseconds ();
	if (setjmp (abortframe))
		Sys_Error ("Error during initialization");

	Z_Init ();

	// prepare enough of the subsystems to handle cvar and command buffer management
	Com_InitArgv (argc, argv);

	Cmd_Init ();
	Cvar_Init ();
	Key_Init ();

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

	Con_Init ();

	Com_Printf (PRINT_ALL, "======== [EGL v%s by Echon] =========\n", EGL_VERSTR);
	Com_Printf (PRINT_ALL, "Compiled: "__DATE__" @ "__TIME__"\n");
	Com_Printf (PRINT_ALL, "----------------------------------------\n");

	FS_Init ();

	Com_Printf (PRINT_ALL, "\n");

	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_AddText ("exec config.cfg\n");
	Cbuf_AddText ("exec eglcfg.cfg\n");

	Com_AddEarlyCommands (qTrue);
	Cbuf_Execute ();

	// init commands and vars
	Cmd_AddCommand ("error",	Com_Error_f);
	Cmd_AddCommand ("savecfg",	Com_WriteConfig);

	host_speeds		= Cvar_Get ("host_speeds",	"0",	0);
	developer		= Cvar_Get ("developer",	"0",	0);
	timescale		= Cvar_Get ("timescale",	"1",	0);
	fixedtime		= Cvar_Get ("fixedtime",	"0",	0);
	logfile			= Cvar_Get ("logfile",		"0",	0);
	log_stats		= Cvar_Get ("log_stats",	"0",	0);
	showtrace		= Cvar_Get ("showtrace",	"0",	0);

	if (dedicated->integer) {
		Cmd_AddCommand ("quit", Com_Quit);
		Cmd_AddCommand ("exit", Com_Quit);
	} else {
		FS_ExecAutoexec ();
		Cbuf_Execute ();
	}

	// init information variables
	Cvar_Get ("version", va ("EGL v%s: %s %s-%s", EGL_VERSTR, __DATE__, BUILDSTRING, CPUSTRING), CVAR_SERVERINFO|CVAR_NOSET);
	Cvar_ForceSet ("version", va ("EGL v%s: %s %s-%s", EGL_VERSTR, __DATE__, BUILDSTRING, CPUSTRING));

	Cvar_Get ("vid_ref", va ("EGL v%s", EGL_VERSTR), CVAR_NOSET);
	Cvar_ForceSet ("vid_ref", va ("EGL v%s", EGL_VERSTR));

	// init the rest of the sub-systems
	Sys_Init ();

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

	Com_Printf (PRINT_ALL, "======== [Initialized %6dms] ========\n\n", Sys_Milliseconds () - eglInitTime);
}


/*
=================
Com_Frame
=================
*/
void FASTCALL Com_Frame (int msec) {
	char	*s;
	int		time_before, time_between, time_after;

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
		} else {
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
		// kthx have draw_string to screen, easier on console buff
		extern	int c_Traces;
		extern	int	c_BrushTraces;
		extern	int	c_PointContents;

		Com_Printf (PRINT_ALL, "%4i traces %4i brtraces %4i points\n", c_Traces, c_BrushTraces, c_PointContents);
		c_Traces = 0;
		c_BrushTraces = 0;
		c_PointContents = 0;
	}

	do {
		s = Sys_ConsoleInput ();
		if (s)
			Cbuf_AddText (va ("%s\n",s));
	} while (s);
	Cbuf_Execute ();

	if (host_speeds->value)
		time_before = Sys_Milliseconds ();

	SV_Frame (msec);

	if (host_speeds->value)
		time_between = Sys_Milliseconds ();

	CL_Frame (msec);

	if (host_speeds->value)
		time_after = Sys_Milliseconds ();

	if (host_speeds->value) {
		// kthx have draw_string to screen, easier on console buff
		int			sv, gm, cl, rf;

		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		Com_Printf (PRINT_ALL, "all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
								(time_after - time_before), sv, gm, cl, rf);
	}
}


/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown (void) {
	Cmd_Shutdown ();
	Cvar_Shutdown ();
	Key_Shutdown ();
	FS_Shutdown ();

	Z_Shutdown ();
}
