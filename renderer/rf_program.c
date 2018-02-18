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
// rf_program.c
// Vertex and fragment program handling
//

#include "rf_local.h"

#define MAX_PROGRAMS		256
#define MAX_PROGRAM_HASH	(MAX_PROGRAMS/4)

static program_t	r_programList[MAX_PROGRAMS];
static program_t	*r_programHashTree[MAX_PROGRAM_HASH];
static uint32		r_numPrograms;

static uint32		r_numProgramErrors;
static uint32		r_numProgramWarnings;

/*
==================
Program_Printf
==================
*/
static void Program_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (flags & PRNT_ERROR)
		r_numProgramErrors++;
	else if (flags & PRNT_WARNING)
		r_numProgramWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
Program_DevPrintf
==================
*/
static void Program_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!developer->intVal)
		return;

	if (flags & PRNT_ERROR)
		r_numProgramErrors++;
	else if (flags & PRNT_WARNING)
		r_numProgramWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}

/*
==============================================================================

	PROGRAM UPLOADING

==============================================================================
*/

/*
===============
R_UploadProgram
===============
*/
static qBool R_UploadProgram (char *name, GLenum target, const char *buffer, GLsizei bufferLen, GLuint *progNum, GLint *upInstructions, GLint *upNative)
{
	const byte	*errorString;
	int			errorPos;

	// Generate a progNum
	qglGenProgramsARB (1, progNum);
	qglBindProgramARB (target, *progNum);
	if (!*progNum) {
		Program_Printf (PRNT_ERROR, "R_UploadProgram: could not allocate a progNum!\n");
		return qFalse;
	}

	// Upload
	qglProgramStringARB (target, GL_PROGRAM_FORMAT_ASCII_ARB, bufferLen, buffer);

	// Check for errors
	qglGetIntegerv (GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	if (errorPos != -1) {
		// Error thrown
		errorString = qglGetString (GL_PROGRAM_ERROR_STRING_ARB);
		switch (target) {
		case GL_VERTEX_PROGRAM_ARB:
			if (errorPos == bufferLen) {
				Program_Printf (PRNT_ERROR, "R_UploadProgram: '%s' vertex program error at EOF\n", name);
			}
			else {
				Program_Printf (PRNT_ERROR, "R_UploadProgram: '%s' vertex program error\n", name);
				Program_Printf (PRNT_ERROR, "GL_PROGRAM_ERROR_POSITION: %i\n", errorPos);
			}
			break;

		case GL_FRAGMENT_PROGRAM_ARB:
			if (errorPos == bufferLen) {
				Program_Printf (PRNT_ERROR, "R_UploadProgram: '%s' fragment program error at EOF\n", name);
			}
			else {
				Program_Printf (PRNT_ERROR, "R_UploadProgram: '%s' fragment program error\n", name);
				Program_Printf (PRNT_ERROR, "GL_PROGRAM_ERROR_POSITION: %i\n", errorPos);
			}
			break;
		}
		Program_Printf (PRNT_ERROR, "GL_PROGRAM_ERROR_STRING: %s\n", errorString);

		qglDeleteProgramsARB (1, progNum);
		return qFalse;
	}

	qglGetProgramivARB (target, GL_PROGRAM_INSTRUCTIONS_ARB, upInstructions);
	qglGetProgramivARB (target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, upNative);
	return qTrue;
}

/*
===============
R_LoadProgram
===============
*/
static program_t *R_LoadProgram (char *name, qBool baseDir, GLenum target, const char *buffer, GLsizei bufferLen)
{
	char		fixedName[MAX_QPATH];
	program_t	*prog;
	uint32		i;
	GLuint		progNum;
	GLint		upInstructions, upNative;

	// Normalize the name
	Com_NormalizePath (fixedName, sizeof (fixedName), name);
	Q_strlwr (fixedName);

	// Upload the program
	if (!R_UploadProgram (fixedName, target, buffer, bufferLen, &progNum, &upInstructions, &upNative))
		return NULL;

	// Find a free r_programList spot
	for (i=0, prog=r_programList ; i<r_numPrograms ; i++, prog++) {
		if (!prog->progNum)
			break;
	}

	// None found, create a new spot
	if (i == r_numPrograms) {
		if (r_numPrograms+1 >= MAX_PROGRAMS)
			Com_Error (ERR_DROP, "R_LoadProgram: r_numPrograms >= MAX_PROGRAMS");

		prog = &r_programList[r_numPrograms++];
	}

	// Fill out properties
	Q_strncpyz (prog->name, fixedName, sizeof (prog->name));
	prog->hashValue = Com_HashGenericFast (prog->name, MAX_PROGRAM_HASH);
	prog->baseDir = baseDir;
	prog->target = target;
	prog->upInstructions = upInstructions;
	prog->upNative = upNative;

	// Link it in
	prog->hashNext = r_programHashTree[prog->hashValue];
	r_programHashTree[prog->hashValue] = prog;
	return prog;
}

/*
==============================================================================

	REGISTRATION

==============================================================================
*/

/*
===============
R_RegisterProgram
===============
*/
program_t *R_RegisterProgram (char *name, qBool fragProg)
{
	char		fixedName[MAX_QPATH];
	program_t	*prog, *best;
	GLenum		target;
	uint32		hash;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// Check for extension
	if (fragProg) {
		if (!ri.config.extFragmentProgram) {
			Com_Error (ERR_DROP, "R_RegisterProgram: attempted to register fragment program when extension is not enabled");
			return NULL;
		}
	}
	else if (!ri.config.extVertexProgram) {
		Com_Error (ERR_DROP, "R_RegisterProgram: attempted to register vertex program when extension is not enabled");
		return NULL;
	}

	// Set the target
	if (fragProg)
		target = GL_FRAGMENT_PROGRAM_ARB;
	else
		target = GL_VERTEX_PROGRAM_ARB;

	// Fix the name
	Com_NormalizePath (fixedName, sizeof (fixedName), name);
	Q_strlwr (fixedName);

	// Calculate hash
	hash = Com_HashGenericFast (fixedName, MAX_PROGRAM_HASH);

	// Search
	best = NULL;
	for (prog=r_programHashTree[hash] ; prog ; prog=prog->hashNext) {
		if (prog->target != target)
			continue;
		if (strcmp (fixedName, prog->name))
			continue;

		if (!best || prog->baseDir >= best->baseDir)
			best = prog;
	}

	return best;
}


/*
===============
R_ParseProgramFile
===============
*/
static void R_ParseProgramFile (char *fixedName, qBool baseDir, qBool vp, qBool fp)
{
	char	*fileBuffer;
	int		fileLen;
	char	*vpBuf, *fpBuf;
	char	*start, *end;
	char	*token;
	parse_t	*ps;

	// Load the file
	Program_Printf (0, "...loading '%s'\n", fixedName);
	fileLen = FS_LoadFile (fixedName, (void **)&fileBuffer, "\n\0");
	if (!fileBuffer || fileLen <= 0) {
		Program_DevPrintf (PRNT_ERROR, "...ERROR: couldn't load '%s' -- %s\n", fixedName, (fileLen == -1) ? "not found" : "empty");
		return;
	}

	// Copy the buffer, and make certain it's newline and null terminated
	if (vp) {
		vpBuf = (char *)Mem_PoolAlloc (fileLen, ri.programSysPool, 0);
		memcpy (vpBuf, fileBuffer, fileLen);
	} else {
		vpBuf = NULL;
	}
	if (fp) {
		fpBuf = (char *)Mem_PoolAlloc (fileLen, ri.programSysPool, 0);
		memcpy (fpBuf, fileBuffer, fileLen);
	} else {
		fpBuf = NULL;
	}

	// Don't need this anymore
	FS_FreeFile (fileBuffer);

	if (vp) {
		fileBuffer = vpBuf;
		ps = PS_StartSession (vpBuf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE|PSP_COMMENT_POUND);

		start = end = NULL;

		// Parse
		for ( ; ; ) {
			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token)) {
				Program_Printf (PRNT_ERROR, "Missing '!!ARBvp1.0' header\n");
				break;
			}

			// Header
			if (!Q_stricmp (token, "!!ARBvp1.0")) {
				start = ps->dataPtr - 10;

				// Find the footer
				for ( ; ; ) {
					if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token)) {
						Program_Printf (PRNT_ERROR, "Missing 'END' footer!\n");
						break;
					}

					if (!Q_stricmp (token, "END")) {
						end = ps->dataPtr+4;
						break;
					}
				}

				if (end)
					break;
			}
		}

		if (start && end)
			R_LoadProgram (fixedName, baseDir, GL_VERTEX_PROGRAM_ARB, start, end-start);

		// Done
		PS_EndSession (ps);
		Mem_Free (fileBuffer);
	}
	if (fp) {
		fileBuffer = fpBuf;
		ps = PS_StartSession (fpBuf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE|PSP_COMMENT_POUND);

		start = end = NULL;

		// Parse
		for ( ; ; ) {
			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token)) {
				Program_Printf (PRNT_ERROR, "Missing '!!ARBfp1.0' header\n");
				break;
			}

			// Header
			if (!Q_stricmp (token, "!!ARBfp1.0")) {
				start = ps->dataPtr - 10;

				// Find the footer
				for ( ; ; ) {
					if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token)) {
						Program_Printf (PRNT_ERROR, "Missing 'END' footer!\n");
						break;
					}

					if (!Q_stricmp (token, "END")) {
						end = ps->dataPtr+4;
						break;
					}
				}

				if (end)
					break;
			}
		}

		if (start && end)
			R_LoadProgram (fixedName, baseDir, GL_FRAGMENT_PROGRAM_ARB, start, end-start);

		// Done
		PS_EndSession (ps);
		Mem_Free (fileBuffer);
	}
}

/*
==============================================================================

	CONSOLE FUNCTIONS

==============================================================================
*/

/*
===============
R_ProgramList_f
===============
*/
static void R_ProgramList_f (void)
{
	program_t	*prog;
	uint32		i;

	Com_Printf (0, "------------------------------------------------------\n");
	for (i=0, prog=r_programList ; i<r_numPrograms ; i++, prog++) {
		Com_Printf (0, "%3i ", prog->progNum);
		Com_Printf (0, "%5i ", prog->upInstructions);
		Com_Printf (0, "%s ", prog->upNative ? "n" : "-");

		switch (prog->target) {
		case GL_FRAGMENT_PROGRAM_ARB:	Com_Printf (0, "FP ");	break;
		case GL_VERTEX_PROGRAM_ARB:		Com_Printf (0, "VP ");	break;
		}

		Com_Printf (0, "%s\n", prog->name);
	}
	Com_Printf (0, "Total programs: %i\n", r_numPrograms);
	Com_Printf (0, "------------------------------------------------------\n");
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

static void	*cmd_programList;

/*
===============
R_ProgramInit
===============
*/
void R_ProgramInit (void)
{
	char	fixedName[MAX_QPATH];
	char	*fileList[MAX_PROGRAMS];
	size_t	numFiles, i;
	qBool	baseDir;
	char	*name;
	uint32	initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n-------- Program Initialization --------\n");

	// Commands
	cmd_programList = Cmd_AddCommand ("programlist", R_ProgramList_f, "Prints out a list of currently loaded vertex and fragment programs");

	r_numProgramErrors = 0;
	r_numProgramWarnings = 0;

	// Load *.vfp programs
	Com_Printf (0, "Looking for *.vfp programs...\n");
	numFiles = FS_FindFiles ("programs", "*programs/*.vfp", "vfp", fileList, MAX_PROGRAMS, qTrue, qTrue);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/programs/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir program?
		baseDir = (strstr (fileList[i], BASE_MODDIRNAME "/")) ? qTrue : qFalse;

		R_ParseProgramFile (name, baseDir, qTrue, qTrue);
	}
	FS_FreeFileList (fileList, numFiles);

	// Load *.vp programs
	Com_Printf (0, "Looking for *.vp programs...\n");
	numFiles = FS_FindFiles ("programs", "*programs/*.vp", "vp", fileList, MAX_PROGRAMS, qTrue, qTrue);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/programs/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir program?
		baseDir = (strstr (fileList[i], BASE_MODDIRNAME "/")) ? qTrue : qFalse;

		R_ParseProgramFile (name, baseDir, qTrue, qFalse);
	}
	FS_FreeFileList (fileList, numFiles);

	// Load *.fp programs
	Com_Printf (0, "Looking for *.fp programs...\n");
	numFiles = FS_FindFiles ("programs", "*programs/*.fp", "fp", fileList, MAX_PROGRAMS, qTrue, qTrue);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/programs/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir program?
		baseDir = (strstr (fileList[i], BASE_MODDIRNAME "/")) ? qTrue : qFalse;

		R_ParseProgramFile (name, baseDir, qFalse, qTrue);
	}
	FS_FreeFileList (fileList, numFiles);

	Com_Printf (0, "----------------------------------------\n");

	// Check for gl errors
	GL_CheckForError ("R_ProgramInit");

	// Check memory integrity
	Mem_CheckPoolIntegrity (ri.programSysPool);

	Com_Printf (0, "PROGRAMS - %i error(s), %i warning(s)\n", r_numProgramErrors, r_numProgramWarnings);
	Com_Printf (0, "%u programs loaded in %ums\n", r_numPrograms, Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}


/*
===============
R_ProgramShutdown
===============
*/
void R_ProgramShutdown (void)
{
	program_t	*prog;
	size_t		size, i;

	Com_Printf (0, "Program system shutdown:\n");

	// Remove commands
	Cmd_RemoveCommand ("programlist", cmd_programList);

	// Shut the programs down
	if (ri.config.extVertexProgram)
		qglBindProgramARB (GL_VERTEX_PROGRAM_ARB, 0);
	if (ri.config.extFragmentProgram)
		qglBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, 0);

	for (i=0, prog=r_programList ; i<r_numPrograms ; i++, prog++) {
		if (!prog->progNum)
			continue;	// Free r_programList slot

		// Free it
		qglDeleteProgramsARB (1, &prog->progNum);
	}

	r_numPrograms = 0;
	memset (r_programList, 0, sizeof (program_t) * MAX_PROGRAMS);
	memset (r_programHashTree, 0, sizeof (program_t *) * MAX_PROGRAM_HASH);

	size = Mem_FreePool (ri.programSysPool);
	Com_Printf (0, "...releasing %u bytes...\n", size);
}
