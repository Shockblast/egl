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
// r_vbo.c
// GL_ARB_vertex_buffer_object backend
// This will generate and locally store VBO information
//

#if 0
#include "r_local.h"

static vbo_t	r_vboList[MAX_VBOS];
static uInt		r_numVBO;

/*
=============
R_RegisterVBO
=============
*/
vbo_t *R_RegisterVBO (int numVerts, vec3_t *verts)
{
	vbo_t	*vbo;
	uInt	i;

	// Check for the extension
	if (!glConfig.extVertexBufferObject)
		return NULL;

	// Find a free slot
	for (i=0, vbo=r_vboList ; i<r_numVBO ; vbo++, i++) {
		if (!vbo->inUse)
			break;
	}

	// None found, create a new slot
	if (i == r_numVBO) {
		if (r_numVBO+1 >= MAX_VBOS)
			Com_Error (ERR_FATAL, "R_RegisterVBO: MAX_VBOS");

		vbo = &r_vboList[r_numVBO++];
	}

	// Fill it in
	vbo->inUse = qTrue;
	if (verts) {
		qglGenBuffersARB (1, &vbo->vertexObject);
		qglBindBufferARB (GL_ARRAY_BUFFER_ARB, vbo->vertexObject);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, numVerts * sizeof (vec3_t), verts, GL_STREAM_COPY_ARB);
		qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
	}
Com_Printf (0, 0, "%i\n", r_numVBO);
	return vbo;
}


/*
=============
R_ReleaseVBO
=============
*/
void R_ReleaseVBO (vbo_t *vbo)
{
	if (!vbo->inUse)
		return;

	if (vbo->vertexObject) {
		qglBindBufferARB (GL_ARRAY_BUFFER_ARB, vbo->vertexObject);
		qglDeleteBuffersARB (1, &vbo->vertexObject);
	}
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
=============
R_VBOInit
=============
*/
void R_VBOInit (void)
{
	int		i;

	if (!glConfig.extVertexBufferObject)
		return;

	// Make certain values are cleared
	for (i=0 ; i<MAX_VBOS ; i++) {
		R_ReleaseVBO (&r_vboList[i]);

		r_vboList[i].inUse = qFalse;
		r_vboList[i].handle = i + 1;
	}
}


/*
=============
R_VBOShutdown
=============
*/
void R_VBOShutdown (void)
{
	int		i;

	if (!glConfig.extVertexBufferObject)
		return;

	// Release objects
	for (i=0 ; i<MAX_VBOS ; i++)
		R_ReleaseVBO (&r_vboList[i]);
}
#endif