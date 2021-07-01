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
// r_program.h
//

/*
=============================================================================

	FRAGMENT / VERTEX PROGRAMS

=============================================================================
*/

typedef struct program_s {
	char				name[MAX_QPATH];

	GLuint				progNum;
	GLenum				target;

	uint32				touchFrame;

	uint32				hashValue;
	struct program_s	*hashNext;
} program_t;

void		R_BindProgram (program_t *program);

#define 	R_TouchProgram(prog) ((prog)->touchFrame = r_refRegInfo.registerFrame)

program_t	*R_RegisterProgram (char *name, qBool fragProg);
void		R_EndProgramRegistration (void);

void		R_ProgramInit (void);
void		R_ProgramShutdown (void);
