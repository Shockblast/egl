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

cVar_t	*developer;
cVar_t	*cg_developer;
cVar_t	*fs_developer;

cVar_t	*timescale;
cVar_t	*fixedtime;
cVar_t	*logfile;		// 1 = buffer log, 2 = flush after each print
cVar_t	*dedicated;

static qBool	com_initialized;
static FILE		*com_logFile;
static uint32	com_numErrors;
static uint32	com_numWarnings;

/*
============================================================================

	HASH OPTIMIZING

============================================================================
*/

/*
================
Com_HashFileName

Normalizes path slashes, and stops before the extension.
hashSize MUST be a power of two!
================
*/
uint32 Com_HashFileName (const char *fileName, const int hashSize)
{
	uint32	hashValue;
	int		ch, i;

	for (i=0, hashValue=0 ; *fileName ; i++) {
		ch = *(fileName++);

		if (ch == '\\')
			ch = '/';
		else if (ch == '.')
			break;

		hashValue = hashValue * 33 + Q_tolower (ch);
	}

	return (hashValue + (hashValue >> 5)) & (hashSize-1);
}


/*
================
Com_HashGeneric

hashSize MUST be a power of two!
================
*/
uint32 Com_HashGeneric (const char *name, const int hashSize)
{
	uint32	hashValue;
	int		ch, i;

	for (i=0, hashValue=0 ; *name ; i++) {
		ch = *(name++);
		hashValue = hashValue * 33 + Q_tolower (ch);
	}

	return (hashValue + (hashValue >> 5)) & (hashSize-1);
}


/*
================
Com_HashGenericFast

hashSize MUST be a power of two!
Same as above except no Q_tolower.
================
*/
uint32 Com_HashGenericFast (const char *name, const int hashSize)
{
	uint32	hashValue;
	int		ch, i;

	for (i=0, hashValue=0 ; *name ; i++) {
		ch = *(name++);
		hashValue = hashValue * 33 + ch;
	}

	return (hashValue + (hashValue >> 5)) & (hashSize-1);
}

/*
============================================================================

	CLIENT / SERVER INTERACTIONS

============================================================================
*/

static int	com_redirectTarget;
static char	*com_redirectBuffer;
static int	com_redirectBufferSize;
static void	(*com_redirectFlush) (int target, char *buffer);

/*
================
Com_BeginRedirect
================
*/
void Com_BeginRedirect (int target, char *buffer, int bufferSize, void (*flush)(int target, char *buffer))
{
	if (!target || !buffer || !bufferSize || !flush)
		return;

	com_redirectTarget = target;
	com_redirectBuffer = buffer;
	com_redirectBufferSize = bufferSize;
	com_redirectFlush = flush;

	*com_redirectBuffer = 0;
}


/*
================
Com_EndRedirect
================
*/
void Com_EndRedirect (void)
{
	com_redirectFlush (com_redirectTarget, com_redirectBuffer);

	com_redirectTarget = 0;
	com_redirectBuffer = NULL;
	com_redirectBufferSize = 0;
	com_redirectFlush = NULL;
}


/*
=============
Com_ConPrint

Doesn't evaluate args. Com_Printf and Com_DevPrintf use this to hand off
console messages to the appropriate targets.
=============
*/
void Com_ConPrint (comPrint_t flags, char *string)
{
	// Tallying purposes
	if (flags & PRNT_ERROR)
		com_numErrors++;
	else if (flags & PRNT_WARNING)
		com_numWarnings++;

	// Message redirecting
	if (com_redirectTarget) {
		if ((int)(strlen (string) + strlen (com_redirectBuffer)) > com_redirectBufferSize - 1) {
			com_redirectFlush (com_redirectTarget, com_redirectBuffer);
			*com_redirectBuffer = 0;
		}
		strcat (com_redirectBuffer, string);
		return;
	}

#ifndef DEDICATED_ONLY
	// Print to the client console
	if (dedicated && !dedicated->integer)
		CL_ConsolePrintf (flags, string);
#endif

	// Also echo to debugging console
	Sys_Print (string);

	// Logfile
	if (logfile && logfile->value) {
		char	name[MAX_QPATH];

		if (!com_logFile) {
			Q_snprintfz (name, sizeof (name), "%s/console.log", FS_Gamedir ());
			if (logfile->value > 2)
				com_logFile = fopen (name, "a");
			else
				com_logFile = fopen (name, "w");
		}
		if (com_logFile)
			fprintf (com_logFile, "%s", string);
		if (logfile->value > 1)
			fflush (com_logFile);	// force it to save every time
	}
}


/*
=============
Com_Printf

Both client and server can use this, and it will output to the apropriate place.
=============
*/
void Com_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
=============
Com_DevPrintf

Hands off to Com_ConPrint if developer is on
=============
*/
void Com_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!developer->integer)
		return;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error (comError_t code, char *fmt, ...)
{
	va_list			argptr;
	static char		msg[MAX_COMPRINT];
	static qBool	recursive = qFalse;

	if (recursive)
		Sys_Error ("Com_Error: Recursive error after: %s", msg);
	recursive = qTrue;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	switch (code) {
	case ERR_DISCONNECT:
#ifndef DEDICATED_ONLY
		if (!dedicated->integer)
			CL_Disconnect (qTrue);
#endif

		recursive = qFalse;
		if (!com_initialized)
			Sys_Error ("%s", msg);
		longjmp (abortframe, -1);
		break;

	case ERR_DROP:
		Com_Printf (0, "********************\nERROR: %s\n********************\n", msg);

		SV_Shutdown (Q_VarArgs ("Server exited: %s\n", msg), qFalse, qFalse);
#ifndef DEDICATED_ONLY
		if (!dedicated->integer)
			CL_Disconnect (qTrue);
#endif

		recursive = qFalse;
		if (!com_initialized)
			Sys_Error ("%s", msg);
		longjmp (abortframe, -1);
		break;

	default:
	case ERR_FATAL:
		SV_Shutdown (Q_VarArgs ("Server fatal crashed: %s\n", msg), qFalse, qTrue);
#ifndef DEDICATED_ONLY
		if (!dedicated->integer)
			CL_ClientShutdown (qTrue);
#endif
		break;
	}

	if (com_logFile) {
		fclose (com_logFile);
		com_logFile = NULL;
	}

	Sys_Error ("%s", msg);
}


/*
=============
Com_Error_f

Just throw a fatal error to test error shutdown procedures
=============
*/
void Com_Error_f (void)
{
	Com_Error (ERR_FATAL, "%s", Cmd_Args ());
}


/*
=============
Com_Quit

Both client and server can use this, and it will do the apropriate things.
=============
*/
void Com_Quit (qBool error)
{
	if (Cmd_Argc () > 1)
		SV_Shutdown (Q_VarArgs ("Server has shut down: %s\n", Cmd_Args ()), qFalse, qFalse);
	else
		SV_Shutdown ("Server has shut down\n", qFalse, qFalse);

	if (com_logFile) {
		fclose (com_logFile);
		com_logFile = NULL;
	}

	Sys_Quit (error);
}


/*
=============
Com_Quit_f
=============
*/
static void Com_Quit_f (void)
{
	Com_Quit (qFalse);
}

/*
============================================================================

	CLIENT / SERVER STATES

	Non-zero state values indicate initialization
============================================================================
*/

static ssState_t	com_svState;
static caState_t	com_clState;

/*
==================
Com_ClientState
==================
*/
caState_t Com_ClientState (void)
{
	return com_clState;
}


/*
==================
Com_SetClientState
==================
*/
void Com_SetClientState (caState_t state)
{
	com_clState = state;
}


/*
==================
Com_ServerState
==================
*/
ssState_t Com_ServerState (void)
{
	return com_svState;
}


/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (ssState_t state)
{
	com_svState = state;
}

/*
============================================================================

	PROGRAM ARGUMENTS

============================================================================
*/

#define MAX_NUM_ARGVS	51

static int	com_argCount;
static char	*com_argValues[MAX_NUM_ARGVS];

/*
================
Com_InitArgv
================
*/
static void Com_InitArgv (int argc, char **argv)
{
	int		i;

	if (argc >= MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc >= MAX_NUM_ARGVS");

	com_argCount = argc;
	for (i=0 ; i<argc ; i++) {
		if (!argv[i] || strlen(argv[i])+1 >= MAX_TOKEN_CHARS)
			com_argValues[i] = "";
		else
			com_argValues[i] = argv[i];
	}
}


/*
================
Com_ClearArgv
================
*/
static void Com_ClearArgv (int arg)
{
	if (arg < 0 || arg >= com_argCount || !com_argValues[arg])
		return;

	com_argValues[arg] = "";
}


/*
================
Com_Argc
================
*/
static int Com_Argc (void)
{
	return com_argCount;
}


/*
================
Com_Argv
================
*/
static char *Com_Argv (int arg)
{
	if (arg < 0 || arg >= com_argCount || !com_argValues[arg])
		return "";

	return com_argValues[arg];
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
static void Com_AddEarlyCommands (qBool clear)
{
	int		i;
	char	*s;

	for (i=0 ; i<Com_Argc () ; i++) {
		s = Com_Argv (i);
		if (Q_stricmp (s, "+set"))
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
static qBool Com_AddLateCommands (void)
{
	int		i, j;
	int		s;
	char	*text, *build, c;
	int		argc;
	qBool	ret;

	// Build the combined string to parse from
	s = 0;
	argc = Com_Argc ();
	for (i=1 ; i<argc ; i++)
		s += strlen (Com_Argv (i)) + 1;

	if (!s)
		return qFalse;
		
	text = Mem_Alloc (s+1);
	text[0] = 0;
	for (i=1 ; i<argc ; i++) {
		strcat (text, Com_Argv (i));
		if (i != argc-1)
			strcat (text, " ");
	}
	
	// Pull out the commands
	build = Mem_Alloc (s+1);
	build[0] = 0;
	
	for (i=0 ; i<s-1 ; i++) {
		if (text[i] == '+') {
			i++;

			// Skip '+'/'-'/'\0'
			for (j=i ; text[j] != '+' && text[j] != '-' && text[j] != '\0' ; j++) ;

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
void Com_Init (int argc, char **argv)
{
	int		eglInitTime;
	char	*verString;

	eglInitTime = Sys_Milliseconds ();
	if (setjmp (abortframe))
		Sys_Error ("Error during initialization");

	seedMT ((unsigned long)time(0));

	Mem_Init ();

	// Prepare enough of the subsystems to handle cvar and command buffer management
	Com_InitArgv (argc, argv);

	Cmd_Init ();
	Cbuf_Init ();
	Alias_Init ();
	Cvar_Init ();
#ifndef DEDICATED_ONLY
	Key_Init ();
#endif

	// Init information variables
	verString = Q_VarArgs ("EGL v%s: %s %s-%s", EGL_VERSTR, __DATE__, BUILDSTRING, CPUSTRING);
	Cvar_Register ("cl_version", verString, CVAR_READONLY);
	Cvar_Register ("version", verString, CVAR_SERVERINFO|CVAR_READONLY);
	Cvar_Register ("vid_ref", verString, CVAR_READONLY);

	/*
	** we need to add the early commands twice, because a basedir or cddir needs to be set before execing
	** config files, but we want other parms to override the settings of the config files
	*/
	Com_AddEarlyCommands (qFalse);
	Cbuf_Execute ();

#ifdef DEDICATED_ONLY
	dedicated		= Cvar_Register ("dedicated",	"1",	CVAR_READONLY);
#else
	dedicated		= Cvar_Register ("dedicated",	"0",	CVAR_READONLY);
#endif

	developer		= Cvar_Register ("developer",		"0",	0);
	cg_developer	= Cvar_Register ("cg_developer",	"0",	0);
	fs_developer	= Cvar_Register ("fs_developer",	"0",	0);

	Sys_Init ();

#ifndef DEDICATED_ONLY
	if (!dedicated->integer)
		CL_ConsoleInit ();
#endif

	Com_Printf (0, "========= Common Initialization ========\n");
	Com_Printf (0, "EGL v%s by Echon\nhttp://egl.quakedev.com/\n", EGL_VERSTR);
	Com_Printf (0, "Compiled: "__DATE__" @ "__TIME__"\n");
	Com_Printf (0, "----------------------------------------\n");

	FS_Init ();

	Com_Printf (0, "\n");

#ifndef DEDICATED_ONLY
	if (!dedicated->integer) {
		Cbuf_AddText ("exec default.cfg\n");
		Cbuf_AddText ("exec config.cfg\n");
		Cbuf_AddText ("exec eglcfg.cfg\n");
	}
#endif

	Com_AddEarlyCommands (qTrue);
	Cbuf_Execute ();

	// Init commands and vars
	Mem_Register ();

	Cmd_AddCommand ("error",	Com_Error_f,	"Error out with a message");

	timescale		= Cvar_Register ("timescale",		"1",	CVAR_CHEAT);
	fixedtime		= Cvar_Register ("fixedtime",		"0",	CVAR_CHEAT);
	logfile			= Cvar_Register ("logfile",			"0",	0);

#ifndef DEDICATED_ONLY
	if (dedicated->integer) {
#endif
		Cmd_AddCommand ("quit", Com_Quit_f,		"Exits");
		Cmd_AddCommand ("exit", Com_Quit_f,		"Exits");
#ifndef DEDICATED_ONLY
	}
	else {
		FS_ExecAutoexec ();
		Cbuf_Execute ();
	}
#endif

	// Init the rest of the sub-systems
	NET_Init ();
	Netchan_Init ();

	SV_Init ();

#ifndef DEDICATED_ONLY
	if (!dedicated->integer) {
		Sys_ShowConsole (0, qFalse);
		CL_ClientInit ();
	}
#endif

	// Add + commands from command line
	if (!Com_AddLateCommands ()) {
#ifndef DEDICATED_ONLY
		if (dedicated->integer)
#endif
			Cbuf_AddText ("dedicated_start\n");

		Cbuf_Execute ();
	}

#ifndef DEDICATED_ONLY
	if (!dedicated->integer)
		SCR_EndLoadingPlaque ();
#endif

	// Check memory integrity
	Mem_CheckGlobalIntegrity ();

	// Touch memory
	Mem_TouchGlobalMemory ();

	com_initialized = qTrue;

	Com_Printf (0, "\nCOMMON - %i error(s), %i warning(s)\n", com_numErrors, com_numWarnings);
	Com_Printf (0, "======= Common Initialized %5.2fs ======\n\n", (Sys_Milliseconds () - eglInitTime)*0.001f);
}


/*
=================
Com_Frame
=================
*/
void __fastcall Com_Frame (int msec)
{
	char	*conInput;

	if (setjmp (abortframe))
		return;			// an ERR_DROP was thrown

	if (fixedtime->value)
		msec = fixedtime->value;
	else if (timescale->value) {
		msec *= timescale->value;
		if (msec < 1)
			msec = 1;
	}

	// Print trace statistics if desired
	CM_PrintStats ();

	// Pump the message loop
	Sys_SendKeyEvents ();

	// Command console input
	conInput = Sys_ConsoleInput ();
	if (conInput)
		Cbuf_AddText (conInput);
	Cbuf_Execute ();

	// Update server
	SV_Frame (msec);

#ifndef DEDICATED_ONLY
	// Update client
	if (!dedicated->integer)
		CL_Frame (msec);
#endif
}


/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown (void)
{
	NET_Shutdown ();
}
