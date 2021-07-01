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

	vec4_t		color;
	vec4_t		colorVel;

	float		scale;
	float		scaleVel;

	int			type;
	int			flags;

	float		delay; // necessary? FIXME

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

static mapEffect_t	cg_mapEffectList[MAPFX_MAXFX];
static uint32		cg_numMapEffects;

static char			cg_mfxFileName[MAX_QPATH];
static char			cg_mfxMapName[MAX_QPATH];

/*
=============================================================================

	EFFECTS

=============================================================================
*/

static void flareThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time, qBool entities)
{
	vec3_t		mins, maxs;
	trace_t		tr;
	float		dist;

	dist = VectorDistFast (cg.refDef.viewOrigin, org);
	*orient = dist * 0.1f;

	if (p->flags & PF_SCALED)
		*size = clamp (*size * (dist * 0.0025f), *size, p->sizeVel);

	VectorSet (maxs, 1, 1, 1);
	VectorSet (mins, -1, -1, -1);
	tr = CG_PMTrace (cg.refDef.viewOrigin, mins, maxs, org, entities);

	if (tr.startSolid || tr.allSolid || tr.fraction < 1.0f)
		color[3] = 0;
}

static void mfxFlareThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	flareThink (p, org, angle, color, size, orient, time, qFalse);
}

static void mfxFlareEntThink (struct particle_s *p, vec3_t org, vec3_t angle, vec4_t color, float *size, float *orient, float *time)
{
	flareThink (p, org, angle, color, size, orient, time, qTrue);
}


/*
==================
CG_GenericOrigin
==================
*/
static void CG_GenericOrigin (struct mapEffect_s *effect)
{
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		MFX_WHITE,						PF_NOCLOSECULL|PF_SCALED,
		0,								qFalse,
		PART_STYLE_QUAD,
		0);
}


/*
==================
CG_CoronaEffectOne
==================
*/
static void CG_CoronaEffectOne (struct mapEffect_s *effect)
{
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		MFX_CORONA,						PF_NOCLOSECULL|PF_SCALED,
		mfxFlareThink,					qTrue,
		PART_STYLE_QUAD,
		0);
}


/*
==================
CG_CoronaEffectTwo

Same as the first but doesn't ignore entities
==================
*/
static void CG_CoronaEffectTwo (struct mapEffect_s *effect)
{
	makePart (
		effect->origin[0],				effect->origin[1],				effect->origin[2],
		0,								0,								0,
		effect->velocity[0],			effect->velocity[2],			effect->velocity[2],
		effect->acceleration[0],		effect->acceleration[1],		effect->acceleration[2],
		effect->color[0],				effect->color[1],				effect->color[2],
		effect->colorVel[0],			effect->colorVel[1],			effect->colorVel[2],
		effect->color[3],				effect->colorVel[3],
		effect->scale * 10,				effect->scale * 10,
		MFX_CORONA,						PF_NOCLOSECULL|PF_SCALED,
		mfxFlareEntThink,				qTrue,
		PART_STYLE_QUAD,
		0);
}

/*
=============================================================================

	ADDS TO RENDERING

=============================================================================
*/

/*
==================
CG_AddMapFX
==================
*/
void CG_AddMapFX (void)
{
	mapEffect_t	*mfx;
	uint32		i;

	if (!cg_mapEffects->integer)
		return;

	for (i=0, mfx=cg_mapEffectList ; i<cg_numMapEffects ; mfx++, i++) {
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
static void *CG_MapFXFunction (int type)
{
	switch (type) {
	case 0:		return &CG_GenericOrigin;		break;
	case 1:		return &CG_CoronaEffectOne;		break;
	case 2:		return &CG_CoronaEffectTwo;		break;
	default:	return &CG_GenericOrigin;		break;
	}

	return NULL;
}

/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

/*
====================
CG_AddMFX_f
====================
*/
static void CG_AddMFX_f (void)
{
	FILE	*f;
	char	path[MAX_QPATH];

	if (!cg_mfxMapName[0]) {
		Com_Printf (0, "CG_AddMFX_f: No map loaded!\n");
		return;
	}

	Q_snprintfz (path, sizeof (path), "%s/mfx/%s.mfx", cgi.FS_Gamedir (), cg_mfxMapName);
	f = fopen (path, "at");
	if (!f) {
		Com_Printf (PRNT_ERROR, "ERROR: Couldn't write %s\n", path);
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
	CG_MapFXInit (cg_mfxMapName);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static void	*cmd_addMFX;

/*
==================
CG_ClearMapFX
==================
*/
static void CG_ClearMapFX (void)
{
	if (!cg_numMapEffects)
		return;

	cg_numMapEffects = 0;
	memset (&cg_mapEffectList, 0, sizeof (cg_mapEffectList));

	memset (&cg_mfxMapName, 0, sizeof (cg_mfxMapName));
	memset (&cg_mfxFileName, 0, sizeof (cg_mfxFileName));
}


/*
==================
CG_ShutdownMapFX
==================
*/
void CG_ShutdownMapFX (void)
{
	if (!cg_mfxMapName[0])
		return;

	cgi.Cmd_RemoveCommand ("addmfx", cmd_addMFX);

	CG_ClearMapFX ();
}


/*
==================
CG_MapFXInit
==================
*/
void CG_MapFXInit (char *mapName)
{
	char			*token, *fbuf, *buf;
	uint32			fileLen, numFx;
	mapEffect_t		*mfx;
	int				stageNum;

	if (!mapName[0])
		return;

	cmd_addMFX = cgi.Cmd_AddCommand ("addmfx",	CG_AddMFX_f,		"Appends a generic effect to this map's mfx file");

	// Skip maps/ and remove .bsp
	mapName = Com_SkipPath (mapName);
	Com_StripExtension (cg_mfxMapName, sizeof (cg_mfxMapName), mapName);

	// Load the mfx file
	Q_snprintfz (cg_mfxFileName, sizeof (cg_mfxFileName), "mfx/%s.mfx", cg_mfxMapName);
	fileLen = cgi.FS_LoadFile (cg_mfxFileName, (void **)&fbuf);
	if (!fbuf || !fileLen || strlen (cg_mfxFileName) < 9)
		return;	// Not found/too short (mfx/a.mfx as minimum)

	Com_DevPrintf (0, "...loading '%s'\n", cg_mfxFileName);

	buf = (char *)CG_AllocTag (fileLen+1, qFalse, CGTAG_MAPFX);
	memcpy (buf, fbuf, fileLen);
	buf[fileLen] = 0;

	CG_FS_FreeFile (fbuf);

	stageNum = 0;
	numFx = 0;
	mfx = NULL;
	token = strtok (buf, MAPFX_DELIMINATORS);

	while (token)  {
		if (stageNum == 0) {
			if (cg_numMapEffects >= MAPFX_MAXFX) {
				Com_Printf (PRNT_ERROR, "CG_MapFXInit: MAPFX_MAXFX (%d) mapfx hit\n", cg_numMapEffects);
				break;
			}

			mfx = &cg_mapEffectList[cg_numMapEffects++];
		}

		switch (stageNum++) {
		case MFXS_ORG0:		mfx->origin[0] = (float)atoi (token) * 0.125f;			break;
		case MFXS_ORG1:		mfx->origin[1] = (float)atoi (token) * 0.125f;			break;
		case MFXS_ORG2:		mfx->origin[2] = (float)atoi (token) * 0.125f;			break;
		case MFXS_VEL0:		mfx->velocity[0] = (float)atoi (token) * 0.125f;		break;
		case MFXS_VEL1:		mfx->velocity[1] = (float)atoi (token) * 0.125f;		break;
		case MFXS_VEL2:		mfx->velocity[2] = (float)atoi (token) * 0.125f;		break;
		case MFXS_ACCEL0:	mfx->acceleration[0] = (float)atoi (token) * 0.125f;	break;
		case MFXS_ACCEL1:	mfx->acceleration[1] = (float)atoi (token) * 0.125f;	break;
		case MFXS_ACCEL2:	mfx->acceleration[2] = (float)atoi (token) * 0.125f;	break;
		case MFXS_CLR0:		mfx->color[0] = (float)atof (token);					break;
		case MFXS_CLR1:		mfx->color[1] = (float)atof (token);					break;
		case MFXS_CLR2:		mfx->color[2] = (float)atof (token);					break;
		case MFXS_CLRVEL0:	mfx->colorVel[0] = (float)atof (token);					break;
		case MFXS_CLRVEL1:	mfx->colorVel[1] = (float)atof (token);					break;
		case MFXS_CLRVEL2:	mfx->colorVel[2] = (float)atof (token);					break;
		case MFXS_ALPHA:	mfx->color[3] = (float)atof (token);					break;
		case MFXS_ALPHAVEL:	mfx->colorVel[3] = (float)atof (token);					break;
		case MFXS_SCALE:	mfx->scale = (float)atof (token);						break;
		case MFXS_SCALEVEL:	mfx->scaleVel = (float)atof (token);					break;
		case MFXS_TYPE:		mfx->type = atoi (token);								break;
		case MFXS_FLAGS:	mfx->flags = atoi (token);								break;
		case MFXS_DELAY:	mfx->delay = (float)atof (token);						break;
		}

		if (stageNum == MFXS_MAX) {
			stageNum = 0;

			mfx->function = CG_MapFXFunction (mfx->type);
		}

		token = strtok (NULL, MAPFX_DELIMINATORS);
	}

	if (stageNum != 0) {
		Com_Printf (PRNT_ERROR, "CG_MapFXInit: Bad file '%s'\n", cg_mfxFileName);
		CG_ClearMapFX ();
	}

	CG_MemFree (buf);
}
