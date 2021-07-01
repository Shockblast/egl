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

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems
#define CMD_MASK		(CMD_BACKUP-1)


#define MAX_DLIGHTS			128
#define MAX_ENTITIES		1024
#define MAX_CLENTITIES		(MAX_ENTITIES-256) // leave breathing room for normal entities

#define MAX_PARTICLES		8192

#define MAX_DECALS			20000
#define MAX_DECAL_VERTS		384
#define MAX_DECAL_FRAGMENTS	256

/*
=============================================================

	ENTITY

=============================================================
*/

typedef struct {
	vec3_t			origin;
	vec3_t			axis[3];
} orientation_t;

typedef struct entity_s {
	struct model_s	*model;			// opaque type outside refresh

	struct image_s	*skin;			// NULL for inline skin
	int				skinNum;		// also used as RF_BEAM's palette index

	vec3_t			axis[3];

	// most recent data
	vec3_t			origin;			// also used as RF_BEAM's "from"
	vec3_t			oldOrigin;		// also used as RF_BEAM's "to"
	int				frame;			// also used as RF_BEAM's diameter
	int				oldFrame;
	float			backLerp;		// 0.0 = current, 1.0 = old

	int				lightStyle;		// for flashing entities
	vec4_t			color;
	int				flags;
	float			scale;
} entity_t;

/*
=============================================================

	DLIGHTS

=============================================================
*/

typedef struct {
	vec3_t		origin;
	vec3_t		color;
	float		intensity;
} dLight_t;

typedef struct {
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightStyle_t;

/*
=============================================================

	IMAGING

=============================================================
*/

enum {
	IT_CUBEMAP	= 1 << 0,		// it's a cubemap env base image
	IT_TEXTURE	= 1 << 1,		// world texture, used for texture size scaling

	IF_CLAMP	= 1 << 2,		// texcoords edge clamped
	IF_NOGAMMA	= 1 << 3,		// not affected by vid_gama
	IF_NOINTENS	= 1 << 4,		// not affected by intensity
	IF_NOMIPMAP	= 1 << 5,		// not mipmapped (2d)
	IF_NOPICMIP	= 1 << 6,		// not affected by gl_picmip
	IF_NOFLUSH	= 1 << 7		// do not flush on map change
};

/*
=============================================================

	EFFECTS

=============================================================
*/

enum {
	PF_SCALED		= 1 << 0,
	PF_DEPTHHACK	= 1 << 1,

	PF_BEAM			= 1 << 2,
	PF_DIRECTION	= 1 << 3,
	PF_SPIRAL		= 1 << 4,
};

typedef struct {
	int				firstVert;
	int				numVerts;

	struct			mBspNode_s	*node;
} fragment_t;

typedef struct {
	int				numVerts;

	vec2_t			*coords;
	vec3_t			*verts;

	struct			mBspNode_s	*node;

	dvec4_t			color;

	struct image_s	*image;
	int				flags;

	int				sFactor;
	int				dFactor;
} decal_t;

typedef struct {
	vec3_t			origin;
	vec3_t			angle;

	dvec4_t			color;

	struct image_s	*image;
	int				flags;

	float			size;

	int				sFactor;
	int				dFactor;

	float			orient;
} particle_t;

/*
=============================================================

	REFRESH DEFINITION

=============================================================
*/

typedef struct {
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

typedef struct {
	int			width;
	int			height;
} vidDef_t;

#endif // __CGREF_H__
