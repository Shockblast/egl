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
// cg_light.c
//

#include "cg_local.h"

typedef struct cgLightStyle_s {
	float		map[MAX_CFGSTRLEN];

	int			length;
	float		value[3];
} cgLightStyle_t;

static cgLightStyle_t	cg_lightStyles[MAX_CS_LIGHTSTYLES];
static int				cg_lSLastOfs;

static cgDLight_t		cg_dLightList[MAX_DLIGHTS];

/*
=============================================================================

	DLIGHT STYLE MANAGEMENT

=============================================================================
*/

/*
================
CG_ClearLightStyles
================
*/
void CG_ClearLightStyles (void)
{
	memset (cg_lightStyles, 0, sizeof (cg_lightStyles));
	cg_lSLastOfs = -1;
}


/*
================
CG_RunLightStyles
================
*/
void CG_RunLightStyles (void)
{
	int				ofs;
	int				i;
	cgLightStyle_t	*ls;

	ofs = cg.realTime / 100;
	if (ofs == cg_lSLastOfs)
		return;
	cg_lSLastOfs = ofs;

	for (i=0, ls=cg_lightStyles ; i<MAX_CS_LIGHTSTYLES ; i++, ls++) {
		if (!ls->length) {
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}

		if (ls->length == 1)
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs%ls->length];
	}
}


/*
================
CG_SetLightstyle
================
*/
void CG_SetLightstyle (int num)
{
	char	*s;
	int		j, k;

	s = cg.configStrings[num+CS_LIGHTS];

	j = (int)strlen (s);
	if (j >= MAX_CFGSTRLEN)
		Com_Error (ERR_DROP, "CG_SetLightstyle: svc_lightstyle length=%i", j);

	cg_lightStyles[num].length = j;

	for (k=0 ; k<j ; k++)
		cg_lightStyles[num].map[k] = (float)(s[k]-'a')/(float)('m'-'a');
}


/*
================
CG_AddLightStyles
================
*/
void CG_AddLightStyles (void)
{
	int				i;
	cgLightStyle_t	*ls;

	for (i=0, ls=cg_lightStyles ; i<MAX_CS_LIGHTSTYLES ; i++, ls++)
		cgi.R_AddLightStyle (i, ls->value[0], ls->value[1], ls->value[2]);
}

/*
=============================================================================

	DLIGHT MANAGEMENT

=============================================================================
*/

/*
================
CG_ClearDLights
================
*/
void CG_ClearDLights (void)
{
	memset (cg_dLightList, 0, sizeof (cg_dLightList));
}


/*
===============
CG_AllocDLight
===============
*/
cgDLight_t *CG_AllocDLight (int key)
{
	int			i;
	cgDLight_t	*dl;

	// First look for an exact key match
	if (key) {
		dl = cg_dLightList;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
			if (dl->key == key) {
				memset (dl, 0, sizeof (*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// Then look for anything else
	dl = cg_dLightList;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (dl->die < cg.realTime) {
			memset (dl, 0, sizeof (*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cg_dLightList[0];
	memset (dl, 0, sizeof (*dl));
	dl->key = key;
	return dl;
}


/*
===============
CG_RunDLights
===============
*/
void CG_RunDLights (void)
{
	int			i;
	cgDLight_t	*dl;

	dl = cg_dLightList;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;
		
		if (dl->die < cg.realTime) {
			dl->radius = 0;
			return;
		}
		dl->radius -= cg.refreshFrameTime*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CG_AddDLights
===============
*/
void CG_AddDLights (void)
{
	int			i;
	cgDLight_t	*dl;

	for (dl=cg_dLightList, i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;

		cgi.R_AddLight (dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}

/*
=============================================================================

	LIGHT EFFECTS

=============================================================================
*/

/*
===============
CG_Flashlight
===============
*/
void CG_Flashlight (int ent, vec3_t pos)
{
	cgDLight_t	*dl;

	dl = CG_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cg.realTime + 100.0f;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}


/*
===============
CG_ColorFlash

flash of light
===============
*/
void __fastcall CG_ColorFlash (vec3_t pos, int ent, float intensity, float r, float g, float b)
{
	cgDLight_t	*dl;

	dl = CG_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = (float)cg.realTime + 100.0f;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
===============
CG_WeldingSparkFlash
===============
*/
void CG_WeldingSparkFlash (vec3_t pos)
{
	cgDLight_t	*dl;

	dl = CG_AllocDLight ((int)((pos[0]+pos[1]+pos[3]) / 3.0));

	VectorCopy (pos, dl->origin);
	VectorSet (dl->color, 1, 1, 0.3f);
	dl->decay = 10;
	dl->die = (float)cg.realTime + 100.0f;
	dl->minlight = 100;
	dl->radius = 175;
}
