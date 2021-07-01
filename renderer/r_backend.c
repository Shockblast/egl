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
// r_backend.c
// TODO Rendering backend (hehehe)
//

#include "r_local.h"

vec3_t		colorArray[VERTARRAY_MAX_VERTS];
vec2_t		coordArray[VERTARRAY_MAX_VERTS];
vec3_t		normalsArray[VERTARRAY_MAX_VERTS];
vec3_t		vertexArray[VERTARRAY_MAX_VERTS];

vec3_t		*rbColorArray;
vec2_t		*rbCoordArray;
vec3_t		*rbNormalsArray;
vec3_t		*rbVertexArray;

/*
=============
RB_Init
=============
*/
void RB_Init (void) {
	return;

	rbColorArray = colorArray;
	rbCoordArray = coordArray;
	rbNormalsArray = normalsArray;
	rbVertexArray = vertexArray;

	qglColorPointer (3, GL_FLOAT, 0, rbColorArray);
	qglTexCoordPointer (2, GL_FLOAT, 0, rbCoordArray);
	qglNormalPointer (GL_FLOAT, 12, rbNormalsArray);
	qglVertexPointer (3, GL_FLOAT, 0, rbVertexArray);
}


/*
=============
RB_Shutdown
=============
*/
void RB_Shutdown (void) {
	return;

	rbColorArray = 0;
	rbCoordArray = 0;
	rbNormalsArray = 0;
	rbVertexArray = 0;

	qglColorPointer (3, GL_FLOAT, 0, 0);
	qglTexCoordPointer (2, GL_FLOAT, 0, 0);
	qglNormalPointer (GL_FLOAT, 12, 0);
	qglVertexPointer (3, GL_FLOAT, 0, 0);
}
