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

// r_sky.c
// - sky rendering and handling

#include "r_local.h"

r_Sky_t	r_Sky;

int		sky_TexOrder[6] = {0, 2, 1, 3, 4, 5};


float	sky_Clip[6][3] = {
	{1, 1, 0},		{1, -1, 0},		{0, -1, 1},		{0, 1, 1},		{1, 0, 1},		{-1, 0, 1} 
};

int		sky_STToVec[6][3] = {
	{3, -1, 2},		{-3, 1, 2},		{1, 3, 2},		{-1, -3, 2},	{-2, -1, 3},	{2, -1, -3}
};

int		sky_VecToST[6][3] = {
	{-2, 3, 1},		{2, 3, -1},		{1, 3, 2},		{-1, 3, -2},	{-2, -1, 3},	{-2, 1, -3}
};

#define SKY_MIN		0.001953125		// (1 / 512)
#define SKY_MAX		0.998046875		// (511 / 512)

#define	SKY_MAXCLIPVERTS	64
#define SKY_BOXSIZE			8192

/*
=================
R_AddSkySurface
=================
*/
void DrawSkyPolygon (int nump, vec3_t vecs) {
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;

	glSpeeds.skyPolys++;

	// decide which face it maps to
	VectorIdentity (v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
		VectorAdd (vp, v, v);

	VectorSet (av, fabs (v[0]), fabs (v[1]), fabs (v[2]));

	if ((av[0] > av[1]) && (av[0] > av[2])) {
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	} else if ((av[1] > av[2]) && (av[1] > av[0])) {
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	} else {
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3) {
		j = sky_VecToST[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero

		j = sky_VecToST[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;

		j = sky_VecToST[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < r_Sky.mins[0][axis])
			r_Sky.mins[0][axis] = s;
		if (t < r_Sky.mins[1][axis])
			r_Sky.mins[1][axis] = t;

		if (s > r_Sky.maxs[0][axis])
			r_Sky.maxs[0][axis] = s;
		if (t > r_Sky.maxs[1][axis])
			r_Sky.maxs[1][axis] = t;
	}
}

void ClipSkyPolygon (int nump, vec3_t vecs, int stage) {
	float	*norm, *v;
	float	d, e;
	float	dists[SKY_MAXCLIPVERTS];
	int		sides[SKY_MAXCLIPVERTS];
	int		newc[2], i, j;
	vec3_t	newv[2][SKY_MAXCLIPVERTS];
	qBool	front, back;

	if (nump > (SKY_MAXCLIPVERTS-2))
		Com_Error (ERR_DROP, "ClipSkyPolygon: SKY_MAXCLIPVERTS");

	if (stage == 6) {
		// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = qFalse;
	norm = sky_Clip[stage];
	for (i=0, v=vecs ; i<nump ; i++, v+=3) {
		d = DotProduct (v, norm);
		if (d > ON_EPSILON) {
			front = qTrue;
			sides[i] = SIDE_FRONT;
		} else if (d < -ON_EPSILON) {
			back = qTrue;
			sides[i] = SIDE_BACK;
		} else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back) {
		// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)));
	newc[0] = newc[1] = 0;

	for (i=0, v=vecs ; i<nump ; i++, v+=3) {
		switch (sides[i]) {
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;

		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;

		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if ((sides[i] == SIDE_ON) || (sides[i+1] == SIDE_ON) || (sides[i+1] == sides[i]))
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++) {
			e = v[j] + d * (v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

void R_AddSkySurface (mBspSurface_t *surf) {
	int			i;
	vec3_t		verts[SKY_MAXCLIPVERTS];
	mBspPoly_t	*p;

	// calculate vertex values for sky box
	for (p=surf->polys ; p ; p=p->next) {
		for (i=0 ; i<p->numverts ; i++)
			VectorSubtract (p->verts[i], r_RefDef.viewOrigin, verts[i]);

		ClipSkyPolygon (p->numverts, verts[0], 0);
	}
}


/*
==============
R_ClearSky
==============
*/
void R_ClearSky (void) {
	int		i;

	if (!r_Sky.loaded)
		return;

	for (i=0 ; i<6 ; i++) {
		r_Sky.mins[0][i] = r_Sky.mins[1][i] = 9999;
		r_Sky.maxs[0][i] = r_Sky.maxs[1][i] = -9999;
	}
}


/*
==============
R_DrawSky
==============
*/
void DrawSkyVec (float s, float t, int axis) {
	vec3_t		v, b;
	int			j, k;

	VectorSet (b, s * SKY_BOXSIZE, t * SKY_BOXSIZE, SKY_BOXSIZE);

	for (j=0 ; j<3 ; j++) {
		k = sky_STToVec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// avoid bilerp seam
	s = clamp ((s + 1) * 0.5, SKY_MIN, SKY_MAX);
	t = 1.0 - clamp ((t + 1) * 0.5, SKY_MIN, SKY_MAX);

	qglTexCoord2f (s, t);
	qglVertex3fv (v);
}
void R_DrawSky (void) {
	int		i;
	mat4_t	mat;

	if (!r_Sky.loaded)
		return;

	if (r_Sky.rotation) {
		// check for no sky at all
		for (i=0 ; i<6 ; i++) {
			if ((r_Sky.mins[0][i] < r_Sky.maxs[0][i]) && (r_Sky.mins[1][i] < r_Sky.maxs[1][i]))
				break;
		}

		if (i == 6)
			return;	// nothing visible
	}

	mat[ 0] = -r_RefDef.viewAxis[1][0];
	mat[ 1] = r_RefDef.viewAxis[2][0];
	mat[ 2] = -r_RefDef.viewAxis[0][0];
	mat[ 3] = 0.0;
	mat[ 4] = -r_RefDef.viewAxis[1][1];
	mat[ 5] = r_RefDef.viewAxis[2][1];
	mat[ 6] = -r_RefDef.viewAxis[0][1];
	mat[ 7] = 0.0;
	mat[ 8] = -r_RefDef.viewAxis[1][2];
	mat[ 9] = r_RefDef.viewAxis[2][2];
	mat[10] = -r_RefDef.viewAxis[0][2];
	mat[11] = 0.0;
	mat[12] = 0.0;
	mat[13] = 0.0;
	mat[14] = 0.0;
	mat[15] = 1.0;

	if (r_Sky.rotation)
		Matrix4_Rotate (mat, r_RefDef.time * r_Sky.rotation, r_Sky.axis[0], r_Sky.axis[1], r_Sky.axis[2]);

	qglLoadMatrixf (mat);

	qglColor4f (glState.invIntens, glState.invIntens, glState.invIntens, 1);
	GL_ToggleOverbrights (qTrue);
	if (r_fog->integer && !(r_RefDef.rdFlags & RDF_UNDERWATER))
		qglDisable (GL_FOG);

	for (i=0 ; i<6 ; i++) {
		if (r_Sky.rotation) {
			// hack, forces full sky to draw when rotating
			r_Sky.mins[0][i] = -1;
			r_Sky.mins[1][i] = -1;
			r_Sky.maxs[0][i] = 1;
			r_Sky.maxs[1][i] = 1;
		} else if ((r_Sky.mins[0][i] >= r_Sky.maxs[0][i]) || (r_Sky.mins[1][i] >= r_Sky.maxs[1][i]))
			continue;

		GL_Bind (0, r_Sky.textures[sky_TexOrder[i]]);

		qglBegin (GL_QUADS);

		DrawSkyVec (r_Sky.mins[0][i], r_Sky.mins[1][i], i);
		DrawSkyVec (r_Sky.mins[0][i], r_Sky.maxs[1][i], i);
		DrawSkyVec (r_Sky.maxs[0][i], r_Sky.maxs[1][i], i);
		DrawSkyVec (r_Sky.maxs[0][i], r_Sky.mins[1][i], i);

		qglEnd ();
	}

	R_LoadModelIdentity ();

	if (r_fog->integer && !(r_RefDef.rdFlags & RDF_UNDERWATER))
		qglEnable (GL_FOG);
	GL_ToggleOverbrights (qFalse);
	qglColor4f (1, 1, 1, 1);
}


/*
============
R_SetSky
============
*/
void R_SetSky (char *name, float rotate, vec3_t axis) {
	int		i;
	char	pathname[MAX_QPATH];

	r_Sky.loaded = qTrue;

	strncpy (r_Sky.baseName, name, sizeof (r_Sky.baseName)-1);
	r_Sky.rotation = rotate;
	VectorCopy (axis, r_Sky.axis);

	for (i=0 ; i<6 ; i++) {
		Com_sprintf (pathname, sizeof (pathname), "env/%s%s.tga", r_Sky.baseName, sky_NameSuffix[i]);
		r_Sky.textures[i] = Img_FindImage (pathname, IF_NOMIPMAP|IF_CLAMP);

		if (!r_Sky.textures[i])
			r_Sky.textures[i] = r_NoTexture;
	}
}


/*
=================
R_SetSky_f

Set a specific sky and rotation speed
=================
*/
void R_SetSky_f (void) {
	float	rotate;
	vec3_t	axis;

	if (!r_Sky.loaded) {
		Com_Printf (PRINT_ALL, "No sky surfaces!\n");
		return;
	}

	if (Cmd_Argc () < 2) {
		Com_Printf (PRINT_ALL, "Usage: sky <basename> <rotate> [axis x y z]\n");
		Com_Printf (PRINT_ALL, "Currently: sky <%s> <%.1f> [%.1f %.1f %.1f]\n", r_Sky.baseName, r_Sky.rotation, r_Sky.axis[0], r_Sky.axis[1], r_Sky.axis[2]);
		return;
	}

	if (Cmd_Argc () > 2)
		rotate = atof (Cmd_Argv (2));
	else
		rotate = 0;

	if (Cmd_Argc () == 6)
		VectorSet (axis, atof (Cmd_Argv (3)), atof (Cmd_Argv (4)), atof (Cmd_Argv (5)));
	else
		VectorSet (axis, 0, 0, 1);

	R_SetSky (Cmd_Argv (1), rotate, axis);
}


/*
=================
R_CheckLoadSky

Returns qTrue if there are ANY sky surfaces in the map, called on map load
=================
*/
qBool R_RecursiveSkySeeker (mBspNode_t *node) {
	int				c, side;
	mBspSurface_t	*surf;
	float			dot;

	// if a leaf node, draw stuff
	if ((node->contents != -1) || (node->contents == CONTENTS_SOLID))
		return qFalse;

	// find which side of the node we are on
	if (node->plane->type < 3)
		dot = r_RefDef.viewOrigin[node->plane->type] - node->plane->dist;
	else
		dot = DotProduct (r_RefDef.viewOrigin, node->plane->normal) - node->plane->dist;

	if (dot >= 0)
		side = 0;
	else
		side = 1;

	// recurse down the children, front side first
	if (R_RecursiveSkySeeker (node->children[side])) {
		r_Sky.loaded = qTrue;
		goto end;
	}

	// find a sky surf
	for (c=node->numSurfaces, surf=r_WorldModel->surfaces + node->firstSurface; c ; c--, surf++) {
		if (surf->texInfo->flags & SURF_SKY) {
			r_Sky.loaded = qTrue;
			return r_Sky.loaded;
		}
	}

	// recurse down the back side
	if (R_RecursiveSkySeeker (node->children[!side])) {
		r_Sky.loaded = qTrue;
		goto end;
	}

end:
	return r_Sky.loaded;
}

qBool R_CheckLoadSky (void) {
	r_Sky.loaded = qFalse;

	return R_RecursiveSkySeeker (r_WorldModel->nodes);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
Sky_Init
==================
*/
void Sky_Init (void) {
	Cmd_AddCommand ("sky",			R_SetSky_f);
}


/*
==================
Sky_Shutdown
==================
*/
void Sky_Shutdown (void) {
	Cmd_RemoveCommand ("sky");
}
