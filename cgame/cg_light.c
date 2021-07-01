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
	int		length;
	float	value[3];
	float	map[MAX_QPATH];
} cgLightStyle_t;

cgLightStyle_t	cgLightStyles[MAX_CS_LIGHTSTYLES];
int				cgLSLastOfs;

cgDLight_t		cgDLights[MAX_DLIGHTS];

/*
==============================================================

	DLIGHT STYLE MANAGEMENT

==============================================================
*/

/*
================
CG_ClearLightStyles
================
*/
void CG_ClearLightStyles (void) {
	memset (cgLightStyles, 0, sizeof (cgLightStyles));
	cgLSLastOfs = -1;
}


/*
================
CG_RunLightStyles
================
*/
void CG_RunLightStyles (void) {
	int				ofs;
	int				i;
	cgLightStyle_t	*ls;

	ofs = cgs.realTime * 0.01;
	if (ofs == cgLSLastOfs)
		return;
	cgLSLastOfs = ofs;

	for (i=0, ls=cgLightStyles ; i<MAX_CS_LIGHTSTYLES ; i++, ls++) {
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
void CG_SetLightstyle (int num) {
	char	*s;
	int		j, k;

	s = cg.configStrings[num+CS_LIGHTS];

	j = strlen (s);
	if (j >= MAX_QPATH)
		Com_Error (ERR_DROP, "CG_SetLightstyle: svc_lightstyle length=%i", j);

	cgLightStyles[num].length = j;

	for (k=0 ; k<j ; k++)
		cgLightStyles[num].map[k] = (float)(s[k]-'a')/(float)('m'-'a');
}


/*
================
CG_AddLightStyles
================
*/
void CG_AddLightStyles (void) {
	int				i;
	cgLightStyle_t	*ls;

	for (i=0, ls=cgLightStyles ; i<MAX_CS_LIGHTSTYLES ; i++, ls++)
		cgi.R_AddLightStyle (i, ls->value[0], ls->value[1], ls->value[2]);
}

/*
==============================================================

	DLIGHT MANAGEMENT

==============================================================
*/

/*
================
CG_ClearDLights
================
*/
void CG_ClearDLights (void) {
	memset (cgDLights, 0, sizeof (cgDLights));
}


/*
===============
CG_AllocDLight
===============
*/
cgDLight_t *CG_AllocDLight (int key) {
	int			i;
	cgDLight_t	*dl;

	// first look for an exact key match
	if (key) {
		dl = cgDLights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
			if (dl->key == key) {
				memset (dl, 0, sizeof (*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cgDLights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (dl->die < cgs.realTime) {
			memset (dl, 0, sizeof (*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cgDLights[0];
	memset (dl, 0, sizeof (*dl));
	dl->key = key;
	return dl;
}


/*
===============
CG_RunDLights
===============
*/
void CG_RunDLights (void) {
	int			i;
	cgDLight_t	*dl;

	dl = cgDLights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;
		
		if (dl->die < cgs.realTime) {
			dl->radius = 0;
			return;
		}
		dl->radius -= cgs.frameTime*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CG_AddDLights
===============
*/
void CG_AddDLights (void) {
	int			i;
	cgDLight_t	*dl;

	for (dl=cgDLights, i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;

		cgi.R_AddLight (dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}

/*
==============================================================

	LIGHT EFFECTS

==============================================================
*/

/*
===============
CG_Flashlight
===============
*/
void CG_Flashlight (int ent, vec3_t pos) {
	cgDLight_t	*dl;

	dl = CG_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cgs.realTime + 100;
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
void FASTCALL CG_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b) {
	cgDLight_t	*dl;

	dl = CG_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cgs.realTime + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
===============
CG_WeldingSparkFlash
===============
*/
void CG_WeldingSparkFlash (vec3_t pos) {
	cgDLight_t	*dl;

	dl = CG_AllocDLight ((int)(VectorAverage (pos)));

	VectorCopy (pos, dl->origin);
	VectorSet (dl->color, 1, 1, 0.3);
	dl->decay = 10;
	dl->die = cgs.realTime + 100;
	dl->minlight = 100;
	dl->radius = 175;
}
