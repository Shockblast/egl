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
// cg_mapeffects.c
// TODO:
// - "nets" so that rain within a set bounds is possible
//

#include "cg_local.h"

#define MAPFX_DELIMINATORS	"\t\r\n "

#define MAPFX_MAXFX		512

typedef struct mapEffect_s {
	vec3_t		origin;

	vec3_t		velocity;
	vec3_t		acceleration;

	dvec4_t		color;
	dvec4_t		colorVel;

	double		scale;
	double		scaleVel;

	int			type;
	int			flags;

	double		delay; // necessary? FIXME

	void		(*function) (struct mapEffect_s *effect);
} mapEffect_t;

// <org0 org1 org2> <vel0 vel1 vel2> <accel0 accel1 accel2>
// <clr0 clr1 clr2> <clv0 clv1 clv2> <alpha alphavel>
// <scale> <scalevel>
// <type> <flags>
// <delay>
enum {
	MFXS_ORG0,		MFXS_ORG1,		MFXS_ORG2,
	MFXS_VEL0,		MFXS_VEL1,		MFXS_VEL2,
	MFXS_ACCEL0,	MFXS_ACCEL1,	MFXS_ACCEL2,
	MFXS_CLR0,		MFXS_CLR1,		MFXS_CLR2,
	MFXS_CLRVEL0,	MFXS_CLRVEL1,	MFXS_CLRVEL2,
	MFXS_ALPHA,		MFXS_ALPHAVEL,
	MFXS_SCALE,		MFXS_SCALEVEL,
	MFXS_TYPE,		MFXS_FLAGS,
	MFXS_DELAY,

	MFXS_MAX
};

static mapEffect_t	cgMapEffects[MAPFX_MAXFX];
static uInt			cgNumMapEffects;

static char			mfxFileName[MAX_QPATH];
static char			mfxMapName[MAX_QPATH];

/*
==============================================================

	EFFECTS

==============================================================
*/

static void flareThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time, qBool entities) {
	float	dist;

	dist = VectorDistanceFast (cg.refDef.viewOrigin, org);
	*orient = dist * 0.1;

	if (p->flags & PF_SCALED)
		*size = clamp (*size * (dist * 0.0025), *size, p->sizeVel);

	if (p->flags & PF_DEPTHHACK) {
		vec3_t		mins, maxs;
		trace_t		trace;

		VectorSet (maxs, 1, 1, 1);
		VectorSet (mins, -1, -1, -1);
		trace = CG_PMTrace (cg.refDef.viewOrigin, mins, maxs, org, entities);

		if (trace.fraction < 1.0f) {
			color[3] = 0;
			if (p->lastKillTime == 0)
				p->lastKillTime = cgs.realTime;
		}/* else
			p->lastKillTime = 0;

		color[3] = p->color[3] - (*time / (p->lastSeenTime+1))*2;
		color[3] = clamp (color[3], 0.0, p->color[3]);*/
	}
}

static void mfxFlareThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	flareThink (p, org, angle, color, size, orient, time, qFalse);
}

static void mfxFlareEntThink (struct cgParticle_s *p, vec3_t org, vec3_t angle, dvec4_t color, double *size, double *orient, double *time) {
	flareThink (p, org, angle, color, size, orient, time, qTrue);
}


/*
==================
CG_GenericOrigin
==================
*/
static void CG_GenericOrigin (struct mapEffect_s *effect) {
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		PT_WHITE,						PF_NOCLOSECULL|PF_SCALED|PF_DEPTHHACK,
		GL_SRC_ALPHA,					GL_ONE,
		0,								qFalse,
		0);
}


/*
==================
CG_CoronaEffectOne
==================
*/
static void CG_CoronaEffectOne (struct mapEffect_s *effect) {
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		MFX_CORONA,						PF_NOCLOSECULL|PF_SCALED|PF_DEPTHHACK,
		GL_SRC_ALPHA,					GL_ONE,
		mfxFlareThink,					qTrue,
		0);
}


/*
==================
CG_CoronaEffectTwo

Same as the first but doesn't ignore entities
==================
*/
static void CG_CoronaEffectTwo (struct mapEffect_s *effect) {
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		MFX_CORONA,						PF_NOCLOSECULL|PF_SCALED|PF_DEPTHHACK,
		GL_SRC_ALPHA,					GL_ONE,
		mfxFlareEntThink,				qTrue,
		0);
}

/*
==============================================================

	ADDS TO RENDERING

==============================================================
*/

/*
==================
CG_AddMapFX
==================
*/
void CG_AddMapFX (void) {
	mapEffect_t	*mfx;
	int			i;

	if (!cg_mapeffects->integer)
		return;

	for (i=0, mfx=cgMapEffects ; i<cgNumMapEffects ; mfx++, i++) {
		if (!mfx->function)
			continue; // no function

		mfx->function (mfx);
	}
}


/*
==================
CG_MapFXFunction
==================
*/
static void *CG_MapFXFunction (int type) {
	switch (type) {
	case 0:		return &CG_GenericOrigin;		break;
	case 1:		return &CG_CoronaEffectOne;		break;
	case 2:		return &CG_CoronaEffectTwo;		break;
	default:	return &CG_GenericOrigin;		break;
	}

	return NULL;
}

/*
==============================================================

	CONSOLE COMMANDS

==============================================================
*/

/*
====================
CG_AddMFX_f
====================
*/
static void CG_AddMFX_f (void) {
	FILE	*f;
	char	path[MAX_QPATH];

	if (!mfxMapName) {
		Com_Printf (0, "CG_AddMFX_f: No map loaded!\n");
		return;
	}

	Q_snprintfz (path, sizeof (path), "%s/mfx/%s.mfx", cgi.FS_Gamedir (), mfxMapName);
	f = fopen (path, "at");
	if (!f) {
		Com_Printf (0, S_COLOR_RED "ERROR: Couldn't write %s\n", path);
		return;
	}

	fprintf (f, "%i %i %i\t\t0 0 0\t\t0 0 0\t\t255 255 255\t255 255 255\t0.6 -10000\t2 2\t0\t0\t0\n",
		(int)(cg.refDef.viewOrigin[0]*8),
		(int)(cg.refDef.viewOrigin[1]*8),
		(int)(cg.refDef.viewOrigin[2]*8));

	fclose (f);

	// tell them
	Com_Printf (0, "Saved (x%i y%i z%i) to '%s'\n",
		(int)cg.refDef.viewOrigin[0],
		(int)cg.refDef.viewOrigin[1],
		(int)cg.refDef.viewOrigin[2],
		path);

	CG_ShutdownMapFX ();
	CG_InitMapFX (mfxMapName);
}

/*
==============================================================

	INIT / SHUTDOWN

==============================================================
*/

/*
==================
CG_ClearMapFX
==================
*/
static void CG_ClearMapFX (void) {
	if (!cgNumMapEffects)
		return;

	memset (&cgMapEffects, 0, sizeof (cgMapEffects));
}


/*
==================
CG_ShutdownMapFX
==================
*/
void CG_ShutdownMapFX (void) {
	cgi.Cmd_RemoveCommand ("addmfx");

	CG_ClearMapFX ();
}


/*
==================
CG_InitMapFX
==================
*/
void CG_InitMapFX (char *mapName) {
	char			*token, *fbuf, *buf;
	uInt			fileLen, numFx;
	mapEffect_t		*mfx;
	int				stageNum;

	cgi.Cmd_AddCommand ("addmfx",	CG_AddMFX_f,		"Appends a generic effect to this map's mfx file");

	CG_ClearMapFX ();

	if (!mapName[0])
		return;

	Q_snprintfz (mfxMapName, sizeof (mfxMapName), mapName);
	Q_snprintfz (mfxFileName, sizeof (mfxFileName), "mfx/%s.mfx", mapName);
	fileLen = cgi.FS_LoadFile (mfxFileName, (void **)&fbuf);

	if (!fbuf || !fileLen || (strlen (mfxFileName) < 9))
		return; // not found/too short (mfx/a.mfx as minimum)

	Com_Printf (PRNT_DEV, "...loading '%s'\n", mfxFileName);

	buf = (char *)CG_MemAlloc (fileLen+1);
	memcpy (buf, fbuf, fileLen);
	buf[fileLen] = 0;

	cgi.FS_FreeFile (fbuf);

	stageNum = 0;
	numFx = 0;
	token = strtok (buf, MAPFX_DELIMINATORS);

	while (token != NULL)  {
		if (stageNum == 0) {
			if (cgNumMapEffects >= MAPFX_MAXFX) {
				Com_Printf (0, S_COLOR_RED "CG_InitMapFX: MAPFX_MAXFX (%d) mapfx hit\n", cgNumMapEffects);
				break;
			}

			mfx = &cgMapEffects[cgNumMapEffects++];
		}

		switch (stageNum++) {
		case MFXS_ORG0:		mfx->origin[0] = (float)atoi (token) * 0.125;		break;
		case MFXS_ORG1:		mfx->origin[1] = (float)atoi (token) * 0.125;		break;
		case MFXS_ORG2:		mfx->origin[2] = (float)atoi (token) * 0.125;		break;
		case MFXS_VEL0:		mfx->velocity[0] = (float)atoi (token) * 0.125;		break;
		case MFXS_VEL1:		mfx->velocity[1] = (float)atoi (token) * 0.125;		break;
		case MFXS_VEL2:		mfx->velocity[2] = (float)atoi (token) * 0.125;		break;
		case MFXS_ACCEL0:	mfx->acceleration[0] = (float)atoi (token) * 0.125;	break;
		case MFXS_ACCEL1:	mfx->acceleration[1] = (float)atoi (token) * 0.125;	break;
		case MFXS_ACCEL2:	mfx->acceleration[2] = (float)atoi (token) * 0.125;	break;
		case MFXS_CLR0:		mfx->color[0] = (double)atof (token);				break;
		case MFXS_CLR1:		mfx->color[1] = (double)atof (token);				break;
		case MFXS_CLR2:		mfx->color[2] = (double)atof (token);				break;
		case MFXS_CLRVEL0:	mfx->colorVel[0] = (double)atof (token);			break;
		case MFXS_CLRVEL1:	mfx->colorVel[1] = (double)atof (token);			break;
		case MFXS_CLRVEL2:	mfx->colorVel[2] = (double)atof (token);			break;
		case MFXS_ALPHA:	mfx->color[3] = (double)atof (token);				break;
		case MFXS_ALPHAVEL:	mfx->colorVel[3] = (double)atof (token);			break;
		case MFXS_SCALE:	mfx->scale = (double)atof (token);					break;
		case MFXS_SCALEVEL:	mfx->scaleVel = (double)atof (token);				break;
		case MFXS_TYPE:		mfx->type = atoi (token);							break;
		case MFXS_FLAGS:	mfx->flags = atoi (token);							break;
		case MFXS_DELAY:	mfx->delay = (double)atof (token);					break;
		}

		if (stageNum == MFXS_MAX) {
			stageNum = 0;

			mfx->function = CG_MapFXFunction (mfx->type);
		}

		token = strtok (NULL, MAPFX_DELIMINATORS);
	}

	if (stageNum != 0) {
		Com_Printf (0, S_COLOR_RED "CG_InitMapFX: Bad file '%s'\n", mfxFileName);
		CG_ClearMapFX ();
	}

	CG_MemFree (buf);
}
