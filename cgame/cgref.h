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
// cgref.h
//

#ifndef __CGREF_H__
#define __CGREF_H__

#define	CMD_BACKUP			64					// allow a lot of command backups for very fast systems
#define CMD_MASK			(CMD_BACKUP-1)

#define MAX_DLIGHTS			128
#define MAX_ENTITIES		1024
#define MAX_CLENTITIES		(MAX_ENTITIES-256)	// leave breathing room for normal entities

#define MAX_PARTICLES		8192
#define MAX_DECALS			20000

#define MAX_POLYS			(MAX_DECALS + MAX_PARTICLES)
#define MAX_POLY_VERTS		384
#define MAX_POLY_FRAGMENTS	256

/*
=============================================================================

	ENTITY

=============================================================================
*/

typedef struct orientation_s {
	vec3_t			origin;
	vec3_t			axis[3];
} orientation_t;

typedef struct entity_s {
	struct model_s	*model;			// opaque type outside refresh

	struct shader_s	*skin;			// NULL for inline skin
	int				skinNum;

	vec3_t			axis[3];

	// most recent data
	vec3_t			origin;
	vec3_t			oldOrigin;
	int				frame;
	int				oldFrame;
	float			backLerp;		// 0.0 = current, 1.0 = old

	bvec4_t			color;
	float			shaderTime;

	int				flags;
	float			scale;
} entity_t;

/*
=============================================================================

	DLIGHTS

=============================================================================
*/

typedef struct dLight_s {
	vec3_t		origin;

	vec3_t		color;
	float		intensity;

	vec3_t		mins;
	vec3_t		maxs;
} dLight_t;

typedef struct lightStyle_s {
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightStyle_t;

/*
=============================================================================

	EFFECTS

=============================================================================
*/

typedef struct fragment_s {
	int				firstVert;
	int				numVerts;

	struct			mBspNode_s	*node;
} fragment_t;

typedef struct poly_s {
	int				numVerts;

	vec3_t			*vertices;
	vec2_t			*texCoords;
	bvec4_t			*colors;

	struct shader_s	*shader;
} poly_t;

/*
=============================================================================

	REFRESH DEFINITION

=============================================================================
*/

typedef struct refDef_s {
	int			x, y;
	int			width, height;

	float		fovX, fovY;
	vec3_t		viewOrigin;
	vec3_t		viewAngles;
	vec3_t		viewAxis[3];
	vec4_t		vBlend;				// rgba 0-1 full screen blend
	float		time;				// time is used to auto animate
	int			rdFlags;			// RDF_UNDERWATER, etc

	byte		*areaBits;			// if not NULL, only areas with set bits will be drawn
} refDef_t;

#endif // __CGREF_H__
