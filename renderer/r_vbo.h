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
// r_vbo.h
//

#define MAX_VBOS	4096

typedef struct vbo_s {
	qBool		inUse;
	uInt		handle;

	uInt		vertexObject;
} vbo_t;

vbo_t	*R_RegisterVBO (int numVerts, vec3_t *verts);
void	R_ReleaseVBO (vbo_t *vbo);

void	R_VBOInit (void);
void	R_VBOShutdown (void);
