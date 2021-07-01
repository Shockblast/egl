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
// r_program.c
// Vertex and fragment program handling
//

#include "r_local.h"

#define MAX_PROGRAMS		512
#define MAX_PROGRAM_HASH	128

static program_t	r_fragmentPrograms[MAX_PROGRAMS];
static program_t	*r_fragmentHashTree[MAX_PROGRAM_HASH];
static uint32		r_numFragmentPrograms;

static program_t	r_vertexPrograms[MAX_PROGRAMS];
static program_t	*r_vertexHashTree[MAX_PROGRAM_HASH];
static uint32		r_numVertexPrograms;

/*
==============================================================================

	PROGRAM STATE

==============================================================================
*/

/*
===============
R_BindProgram
===============
*/
void R_BindProgram (program_t *program)
{
	switch (program->target) {
	case GL_FRAGMENT_PROGRAM_ARB:
		if (glState.boundFragProgram == program->progNum)
			return;
		glState.boundFragProgram = program->progNum;

		qglBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, program->progNum);
		break;

	case GL_VERTEX_PROGRAM_ARB:
		if (glState.boundVertProgram == program->progNum)
			return;
		glState.boundVertProgram = program->progNum;

		qglBindProgramARB (GL_VERTEX_PROGRAM_ARB, program->progNum);
		break;

#ifdef _DEBUG
	default:
		assert (0);
		break;
#endif // _DEBUG
	}
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
static qBool R_UploadProgram (char *name, byte *buffer, int bufferLen, qBool fragProg)
{
	const byte	*errorString;
	int			errorPos;
	uint32		target;

	// Select a target
	if (fragProg)
		target = GL_FRAGMENT_PROGRAM_ARB;
	else
		target = GL_VERTEX_PROGRAM_ARB;

	// Upload
	qglProgramStringARB (target, GL_PROGRAM_FORMAT_ASCII_ARB, bufferLen, buffer);

	// Check for errors
	errorString = qglGetString (GL_PROGRAM_ERROR_STRING_ARB);
	qglGetIntegerv (GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);

	if (errorPos == -1)
		return qTrue;

	if (fragProg)
		Com_Printf (0, "R_UploadProgram: fragment program error in '%s' at char #%i: '%s'\n", name, errorPos, errorString);
	else
		Com_Printf (0, "R_UploadProgram: fragment vertex error in '%s' at char #%i: '%s'\n", name, errorPos, errorString);
	return qFalse;
}

/*
==============================================================================

	PROGRAM LOADING

==============================================================================
*/

/*
===============
R_FindProgram
===============
*/
static program_t *R_FindProgram (const char *name, qBool fragProg)
{
	char		fixedName[MAX_QPATH];
	program_t	*prog;
	uint32		hash;
	int			len;

	// Check the length
	len = strlen (name);
	if (len < 4) {
		Com_Printf (PRNT_ERROR, "R_FindProgram: Program name too short! %s\n", name);
		return NULL;
	}
	if (len+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_FindProgram: Program name too long! %s\n", name);
		return NULL;
	}

	// Fix the name
	Com_NormalizePath (fixedName, sizeof (fixedName), name);

	// Calculate hash
	hash = Com_HashFileName (fixedName, MAX_PROGRAM_HASH);

	// Search
	if (fragProg) {
		for (prog=r_fragmentHashTree[hash] ; prog ; prog=prog->hashNext) {
			if (!prog->touchFrame)
				continue;

			// Check name
			if (!strcmp (fixedName, prog->name))
				return prog;
		}
	}
	else {
		for (prog=r_vertexHashTree[hash] ; prog ; prog=prog->hashNext) {
			if (!prog->touchFrame)
				continue;

			// Check name
			if (!strcmp (fixedName, prog->name))
				return prog;
		}
	}

	return NULL;
}


/*
===============
R_LoadProgram
===============
*/
static program_t *R_LoadProgram (char *name, byte *buffer, int bufferLen, qBool fragProg)
{
	program_t	*prog;
	GLenum		target;
	uint32		i;

	if (fragProg) {
		// Find a free r_fragmentPrograms spot
		for (i=0, prog=r_fragmentPrograms ; i<r_numFragmentPrograms ; i++, prog++) {
			if (!prog->touchFrame)
				break;
		}

		// None found, create a new spot
		if (i == r_numFragmentPrograms) {
			if (r_numFragmentPrograms+1 >= MAX_PROGRAMS)
				Com_Error (ERR_DROP, "R_LoadProgram: r_numFragmentPrograms >= MAX_PROGRAMS");

			prog = &r_fragmentPrograms[r_numFragmentPrograms++];
		}

		// Set the target
		target = GL_FRAGMENT_PROGRAM_ARB;
	}
	else {
		// Find a free r_vertexPrograms spot
		for (i=0, prog=r_vertexPrograms ; i<r_numVertexPrograms ; i++, prog++) {
			if (!prog->touchFrame)
				break;
		}

		// None found, create a new spot
		if (i == r_numVertexPrograms) {
			if (r_numVertexPrograms+1 >= MAX_PROGRAMS)
				Com_Error (ERR_DROP, "R_LoadProgram: r_numVertexPrograms >= MAX_PROGRAMS");

			prog = &r_vertexPrograms[r_numVertexPrograms++];
		}

		// Set the target
		target = GL_VERTEX_PROGRAM_ARB;
	}

	// Fill out properties
	Q_strncpyz (prog->name, name, sizeof (prog->name));
	qglGenProgramsARB (1, &prog->progNum);
	prog->touchFrame = r_refRegInfo.registerFrame;
	prog->hashValue = Com_HashFileName (prog->name, MAX_PROGRAM_HASH);
	prog->target = target;

	// Upload
	qglBindProgramARB (target, prog->progNum);
	if (!R_UploadProgram (name, buffer, bufferLen, fragProg)) {
		if (prog->progNum)
			qglDeleteProgramsARB (1, &prog->progNum);
		memset (prog, 0, sizeof (program_t));
	}

	// Link it in
	if (fragProg) {
		prog->hashNext = r_fragmentHashTree[prog->hashValue];
		r_fragmentHashTree[prog->hashValue] = prog;
	}
	else {
		prog->hashNext = r_vertexHashTree[prog->hashValue];
		r_vertexHashTree[prog->hashValue] = prog;
	}

	return prog;
}


/*
===============
R_RegisterProgram
===============
*/
program_t *R_RegisterProgram (char *name, qBool fragProg)
{
	program_t	*prog;
	byte		*buffer;
	int			bufferLen;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// Check for extension
	if (fragProg && !glConfig.extFragmentProgram) {
		Com_Error (ERR_DROP, "R_RegisterProgram: attempted to register fragment program when extension is not enabled");
		return NULL;
	}
	else if (!glConfig.extVertexProgram) {
		Com_Error (ERR_DROP, "R_RegisterProgram: attempted to register vertex program when extension is not enabled");
		return NULL;
	}

	// Find
	prog = R_FindProgram (name, fragProg);
	if (prog) {
		prog->touchFrame = r_refRegInfo.registerFrame;
		return prog;
	}

	// Upload
	bufferLen = FS_LoadFile (name, (void **)&buffer);
	if (buffer) {
		prog = R_LoadProgram (name, buffer, bufferLen, fragProg);
		FS_FreeFile (buffer);
		return prog;
	}

	return NULL;
}


/*
===============
R_FreeProgram
===============
*/
static void R_FreeProgram (program_t *prog, qBool fragProg)
{
	program_t	*hashProg;
	program_t	**prev;

	assert (prog);
	if (!prog)
		return;

	if (fragProg) {
		// De-link it from the hash tree
		prev = &r_fragmentHashTree[prog->hashValue];
		for ( ; ; ) {
			hashProg = *prev;
			if (!hashProg)
				break;

			if (hashProg == prog) {
				*prev = hashProg->hashNext;
				break;
			}
			prev = &hashProg->hashNext;
		}
	}
	else {
		// De-link it from the hash tree
		prev = &r_vertexHashTree[prog->hashValue];
		for ( ; ; ) {
			hashProg = *prev;
			if (!hashProg)
				break;

			if (hashProg == prog) {
				*prev = hashProg->hashNext;
				break;
			}
			prev = &hashProg->hashNext;
		}
	}

	// Free it
	if (prog->progNum)
		qglDeleteProgramsARB (1, &prog->progNum);
	else
		Com_DevPrintf (PRNT_WARNING, "R_FreeProgram: attempted to release invalid progNum on program '%s'!\n", prog->name);
	memset (prog, 0, sizeof (program_t));
}


/*
===============
R_EndProgramRegistration
===============
*/
void R_EndProgramRegistration (void)
{
	program_t	*prog;
	uint32		i;

	// Check fragment programs
	for (i=0, prog=r_fragmentPrograms ; i<r_numFragmentPrograms ; i++, prog++) {
		if (prog->touchFrame == r_refRegInfo.registerFrame)
			continue;		// Used this sequence

		R_FreeProgram (prog, qTrue);
		r_refRegInfo.fragProgsReleased++;
	}

	// Check vertex programs
	for (i=0, prog=r_vertexPrograms ; i<r_numVertexPrograms ; i++, prog++) {
		if (prog->touchFrame == r_refRegInfo.registerFrame)
			continue;		// Used this sequence

		R_FreeProgram (prog, qFalse);
		r_refRegInfo.vertProgsReleased++;
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

	// List fragment programs
	Com_Printf (0, "------------------------------------------------------\n");
	Com_Printf (0, "Fragment programs:\n");
	for (i=0, prog=r_fragmentPrograms ; i<r_numFragmentPrograms ; i++, prog++) {
		Com_Printf (0, "%s\n", prog->name);
	}
	Com_Printf (0, "Total fragment programs: %i\n", r_numFragmentPrograms);
	Com_Printf (0, "------------------------------------------------------\n");

	// List vertex programs
	Com_Printf (0, "------------------------------------------------------\n");
	Com_Printf (0, "Vertex programs:\n");
	for (i=0, prog=r_vertexPrograms ; i<r_numVertexPrograms ; i++, prog++) {
		Com_Printf (0, "%s\n", prog->name);
	}
	Com_Printf (0, "Total vertex programs: %i\n", r_numVertexPrograms);
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
	// Commands
	cmd_programList = Cmd_AddCommand ("programlist", R_ProgramList_f, "Prints out a list of currently loaded vertex and fragment programs");
}


/*
===============
R_ProgramShutdown
===============
*/
void R_ProgramShutdown (void)
{
	program_t	*prog;
	uint32		i;

	// Remove commands
	Cmd_RemoveCommand ("programlist", cmd_programList);

	// Shut fragment programs down
	for (i=0, prog=r_fragmentPrograms ; i<r_numFragmentPrograms ; i++, prog++) {
		if (!prog->touchFrame)
			continue;	// Free r_imageList slot
		if (!prog->progNum)
			continue;

		// Free it
		qglDeleteProgramsARB (1, &prog->progNum);
	}

	r_numFragmentPrograms = 0;
	memset (r_fragmentPrograms, 0, sizeof (program_t) * MAX_PROGRAMS);
	memset (r_fragmentHashTree, 0, sizeof (program_t *) * MAX_PROGRAMS);

	// Shut vertex programs down
	for (i=0, prog=r_vertexPrograms ; i<r_numVertexPrograms ; i++, prog++) {
		if (!prog->touchFrame)
			continue;	// Free r_imageList slot
		if (!prog->progNum)
			continue;

		// Free it
		qglDeleteProgramsARB (1, &prog->progNum);
	}

	r_numVertexPrograms = 0;
	memset (r_vertexPrograms, 0, sizeof (program_t) * MAX_PROGRAMS);
	memset (r_vertexHashTree, 0, sizeof (program_t *) * MAX_PROGRAMS);
}
