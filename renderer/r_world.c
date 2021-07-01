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
// r_world.c
// World surface related refresh code
//

#include "r_local.h"

mBspSurface_t	*alphaSurfaces;

model_t			*r_WorldModel;
entity_t		r_WorldEntity;

uLong			r_VisFrameCount;	// bumped when going to a new PVS

int				r_OldViewCluster;
int				r_ViewCluster;

typedef struct skyState_s {
	qBool		loaded;

	char		baseName[MAX_QPATH];
	float		rotation;
	vec3_t		axis;

	image_t		*textures[6];

	float		mins[2][6];
	float		maxs[2][6];

} skyState_t;

static skyState_t		skyState;

/*
=============================================================================

	SKY

=============================================================================
*/

static const float	skyClip[6][3] = {
	{1, 1, 0},		{1, -1, 0},		{0, -1, 1},		{0, 1, 1},		{1, 0, 1},		{-1, 0, 1} 
};
static const int	skySTToVec[6][3] = {
	{3, -1, 2},		{-3, 1, 2},		{1, 3, 2},		{-1, -3, 2},	{-2, -1, 3},	{2, -1, -3}
};
static const int	skyVecToST[6][3] = {
	{-2, 3, 1},		{2, 3, -1},		{1, 3, 2},		{-1, 3, -2},	{-2, -1, 3},	{-2, 1, -3}
};

#define	SKY_MAXCLIPVERTS	64
#define SKY_BOXSIZE			8192

/*
=================
R_AddSkySurface
=================
*/
static void ClipSkyPolygon (int nump, vec3_t vecs, int stage) {
	const float	*norm;
	float	*v, d, e;
	float	dists[SKY_MAXCLIPVERTS];
	int		sides[SKY_MAXCLIPVERTS];
	int		newc[2], i, j;
	vec3_t	newv[2][SKY_MAXCLIPVERTS];
	qBool	front, back;

	if (nump > (SKY_MAXCLIPVERTS-2))
		Com_Error (ERR_DROP, "ClipSkyPolygon: SKY_MAXCLIPVERTS");

	if (stage == 6) {
		// fully clipped, so draw it
		int		i, j;
		vec3_t	v, av;
		float	s, t, dv;
		int		axis;
		float	*vp;

		glSpeeds.skyPolys++;

		// decide which face it maps to
		VectorClear (v);
		for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
			VectorAdd (vp, v, v);

		VectorSet (av, fabs (v[0]), fabs (v[1]), fabs (v[2]));

		if ((av[0] > av[1]) && (av[0] > av[2]))
			axis = (v[0] < 0) ? 1 : 0;
		else if ((av[1] > av[2]) && (av[1] > av[0]))
			axis = (v[1] < 0) ? 3 : 2;
		else
			axis = (v[2] < 0) ? 5 : 4;

		// project new texture coords
		for (i=0 ; i<nump ; i++, vecs+=3) {
			j = skyVecToST[axis][2];
			dv = (j > 0) ? vecs[j - 1] : -vecs[-j - 1];

			if (dv < 0.001)
				continue;	// don't divide by zero

			j = skyVecToST[axis][0];
			s = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

			j = skyVecToST[axis][1];
			t = (j < 0) ? -vecs[-j -1] / dv : vecs[j-1] / dv;

			if (s < skyState.mins[0][axis])
				skyState.mins[0][axis] = s;
			if (t < skyState.mins[1][axis])
				skyState.mins[1][axis] = t;

			if (s > skyState.maxs[0][axis])
				skyState.maxs[0][axis] = s;
			if (t > skyState.maxs[1][axis])
				skyState.maxs[1][axis] = t;
		}

		return;
	}

	front = back = qFalse;
	norm = skyClip[stage];
	for (i=0, v=vecs ; i<nump ; i++, v+=3) {
		d = DotProduct (v, norm);
		if (d > LARGE_EPSILON) {
			front = qTrue;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -LARGE_EPSILON) {
			back = qTrue;
			sides[i] = SIDE_BACK;
		}
		else
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
static void R_AddSkySurface (mBspSurface_t *surf) {
	int			i;
	vec3_t		verts[SKY_MAXCLIPVERTS];
	mBspPoly_t	*p;

	// calculate vertex values for sky box
	for (p=surf->polys ; p ; p=p->next) {
		for (i=0 ; i<p->numVerts ; i++) {
			VectorSubtract (p->vertices[i], r_RefDef.viewOrigin, verts[i]);
		}

		ClipSkyPolygon (p->numVerts, verts[0], 0);
	}
}


/*
==============
R_ClearSky
==============
*/
static void R_ClearSky (void) {
	int		i;

	if (!skyState.loaded)
		return;

	for (i=0 ; i<6 ; i++) {
		skyState.mins[0][i] = skyState.mins[1][i] = 9999;
		skyState.maxs[0][i] = skyState.maxs[1][i] = -9999;
	}
}


/*
==============
R_DrawSky
==============
*/
static void DrawSkyVec (int vertNum, float s, float t, int axis) {
	vec3_t		v, b;
	int			j, k;

	VectorSet (b, s * SKY_BOXSIZE, t * SKY_BOXSIZE, SKY_BOXSIZE);

	for (j=0 ; j<3 ; j++) {
		k = skySTToVec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// clamp coords
	s = clamp ((s + 1) * 0.5, 0, 1);
	t = 1.0 - clamp ((t + 1) * 0.5, 0, 1);

	Vector2Set (coordArray[vertNum], s, t);
	VectorSet (normalsArray[vertNum], 0, 1, 0);
	VectorCopy (v, vertexArray[vertNum]);
}
static void R_DrawSky (void) {
	static const int	skyTexOrder[6] = {0, 2, 1, 3, 4, 5};
	int			i;
	mat4x4_t	mat;
	rbMesh_t	mesh;

	if (!skyState.loaded)
		return;

	if (skyState.rotation) {
		// check for no sky at all
		for (i=0 ; i<6 ; i++) {
			if ((skyState.mins[0][i] < skyState.maxs[0][i]) && (skyState.mins[1][i] < skyState.maxs[1][i]))
				break;
		}

		if (i == 6)
			return;	// nothing visible
	}

	mat[ 0] = -r_RefDef.viewAxis[1][0];
	mat[ 1] =  r_RefDef.viewAxis[2][0];
	mat[ 2] = -r_RefDef.viewAxis[0][0];
	mat[ 3] = 0.0;
	mat[ 4] = -r_RefDef.viewAxis[1][1];
	mat[ 5] =  r_RefDef.viewAxis[2][1];
	mat[ 6] = -r_RefDef.viewAxis[0][1];
	mat[ 7] = 0.0;
	mat[ 8] = -r_RefDef.viewAxis[1][2];
	mat[ 9] =  r_RefDef.viewAxis[2][2];
	mat[10] = -r_RefDef.viewAxis[0][2];
	mat[11] = 0.0;
	mat[12] = 0.0;
	mat[13] = 0.0;
	mat[14] = 0.0;
	mat[15] = 1.0;

	if (skyState.rotation)
		Matrix4_Rotate (mat, r_RefDef.time * skyState.rotation, skyState.axis[0], skyState.axis[1], skyState.axis[2]);

	qglLoadMatrixf (mat);

	if (r_fog->integer && !(r_RefDef.rdFlags & RDF_UNDERWATER))
		qglDisable (GL_FOG);

	for (i=0 ; i<6 ; i++) {
		if (skyState.rotation) {
			// hack, forces full sky to draw when rotating
			skyState.mins[0][i] = -1;
			skyState.mins[1][i] = -1;
			skyState.maxs[0][i] = 1;
			skyState.maxs[1][i] = 1;
		}
		else if ((skyState.mins[0][i] >= skyState.maxs[0][i]) || (skyState.mins[1][i] >= skyState.maxs[1][i]))
			continue;

		DrawSkyVec (0, skyState.mins[0][i], skyState.mins[1][i], i);
		DrawSkyVec (1, skyState.mins[0][i], skyState.maxs[1][i], i);
		DrawSkyVec (2, skyState.maxs[0][i], skyState.maxs[1][i], i);
		DrawSkyVec (3, skyState.maxs[0][i], skyState.mins[1][i], i);

		Vector4Set (colorArray[0], glState.invIntens, glState.invIntens, glState.invIntens, 1); // fixme
		// this will break fullbright shaders because invIntens will be ignored

		mesh.numColors = 1;
		mesh.numIndexes = QUAD_INDEXES;
		mesh.numVerts = QUAD_INDEXES;

		mesh.colorArray = colorArray;
		mesh.coordArray = coordArray;
		mesh.lmCoordArray = NULL;
		mesh.indexArray = rbQuadIndices;
		mesh.normalsArray = normalsArray;
		mesh.vertexArray = vertexArray;

		mesh.surface = NULL;

		mesh.texture = skyState.textures[skyTexOrder[i]];
		mesh.lmTexture = NULL;
		mesh.shader = RS_RegisterShader (skyState.textures[skyTexOrder[i]]->name);

		RB_RenderMesh (&mesh);
	}

	R_LoadModelIdentity ();

	if (r_fog->integer && !(r_RefDef.rdFlags & RDF_UNDERWATER))
		qglEnable (GL_FOG);
}


/*
=================
R_CheckLoadSky

Returns qTrue if there are ANY sky surfaces in the map, called on map load
=================
*/
static qBool R_RecursiveSkySeeker (mBspNode_t *node) {
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
		skyState.loaded = qTrue;
		goto end;
	}

	// find a sky surf
	for (c=node->numSurfaces, surf=r_WorldModel->surfaces + node->firstSurface ; c ; c--, surf++) {
		if (surf->texInfo->flags & SURF_SKY) {
			skyState.loaded = qTrue;
			return skyState.loaded;
		}
	}

	// recurse down the back side
	if (R_RecursiveSkySeeker (node->children[!side])) {
		skyState.loaded = qTrue;
		goto end;
	}

end:
	return skyState.loaded;
}
static qBool R_CheckLoadSky (void) {
	skyState.loaded = qFalse;

	return R_RecursiveSkySeeker (r_WorldModel->nodes);
}


/*
============
R_SetSky
============
*/
void R_SetSky (char *name, float rotate, vec3_t axis) {
	char	pathname[MAX_QPATH];
	int		i;

	if (!R_CheckLoadSky ())
		return;

	skyState.loaded = qTrue;

	Q_strncpyz (skyState.baseName, name, sizeof (skyState.baseName));
	skyState.rotation = rotate;
	VectorCopy (axis, skyState.axis);

	for (i=0 ; i<6 ; i++) {
		Q_snprintfz (pathname, sizeof (pathname), "env/%s%s.tga", skyState.baseName, sky_NameSuffix[i]);
		skyState.textures[i] = Img_RegisterImage (pathname, IF_NOMIPMAP|IF_CLAMP);

		if (!skyState.textures[i])
			skyState.textures[i] = r_NoTexture;
	}
}

/*
=============================================================================

	WORLD MODEL

=============================================================================
*/

/*
================
R_AddWorldSurface
================
*/
static void R_AddWorldSurface (mBspSurface_t *surf) {
	mBspPoly_t		*poly;
	image_t			*texture;
	image_t			*lightMap;
	shader_t		*shader;
	rbMesh_t		mesh;
	int				i;

	// update lightmap
	if (surf->flags & SURF_DRAWTURB) {
		lightMap = NULL;
	}
	else {
		qBool		isDynamic = qFalse;
		int			map;

		for (map=0 ; map<surf->numStyles ; map++) {
			if (r_LightStyles[surf->styles[map]].white != surf->cachedLight[map])
				goto dynamic;
		}

		// dynamic this frame or dynamic previously
		if (surf->dLightFrame == r_FrameCount) {
dynamic:
			if (gl_dynamic->integer && !(surf->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
				isDynamic = qTrue;
		}


		GL_SelectTexture (1);

		// update texture
		if (isDynamic) {
			uInt	temp[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT];

			LM_BuildLightMap (surf, (void *)temp, surf->lmWidth*4);

			if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dLightFrame != r_FrameCount)) {
				LM_SetCacheState (surf);

				lightMap = &lmTextures[surf->lmTexNum];
			}
			else {
				lightMap = &lmTextures[0];
			}

			GL_Bind (lightMap);
			qglTexSubImage2D (GL_TEXTURE_2D, 0,
							surf->lmCoords[0], surf->lmCoords[1],
							surf->lmWidth, surf->lmHeight,
							GL_LIGHTMAP_FORMAT,
							GL_UNSIGNED_BYTE,
							temp);

			if (e_test_1->integer && qglGetError () != GL_NO_ERROR) { // orly
				Com_Printf (0, "LIGHTMAP ERROR: texNum(%i)\n", surf->lmTexNum);
			}
		}
		else {
			lightMap = &lmTextures[surf->lmTexNum];
			GL_Bind (lightMap);
		}

		GL_SelectTexture (0);
	}

	// texture
	if (!surf->texInfo->next) {
		texture = surf->texInfo->texture;
		shader = surf->texInfo->shader;
	}
	else {
		mBspTexInfo_t	*texInfo;

		texInfo = surf->texInfo;
		for (i=((int)(r_RefDef.time * 2))%texInfo->numFrames ; i ; i--) {
			texInfo = texInfo->next;
		}

		texture = texInfo->texture;
		shader = texInfo->shader;
	}

	// fill in standard mesh attributes
	mesh.numColors = 1;

	mesh.colorArray = colorArray;
	mesh.indexArray = rbTriFanIndices;

	mesh.surface = surf;

	mesh.texture = texture;
	mesh.lmTexture = lightMap;
	mesh.shader = shader;

	// push the mesh
	if (surf->flags & SURF_DRAWTURB) {
		float		*coords;

		for (poly=surf->polys ; poly ; poly=poly->next) {
			coords = poly->coords[0];
			for (i=0 ; i<poly->numVerts ; i++) {
				coordArray[i][0] = coords[0] + r_WarpSinTable[Q_ftol (((coords[1]*ONEDIV8 + r_RefDef.time) * TURBSCALE)) & 255];
				coordArray[i][1] = coords[1] + r_WarpSinTable[Q_ftol (((coords[0]*ONEDIV8 + r_RefDef.time) * TURBSCALE)) & 255];

				coords += 2;
			}

			mesh.numIndexes = poly->numIndexes;
			mesh.numVerts = poly->numVerts;

			mesh.coordArray = coordArray;
			mesh.lmCoordArray = NULL;
			mesh.normalsArray = poly->normals;
			mesh.vertexArray = poly->vertices;

			RB_RenderMesh (&mesh);

			glSpeeds.worldPolys++;
		}
	}
	else {
		for (poly=surf->polys ; poly ; poly=poly->next) {
			mesh.numIndexes = poly->numIndexes;
			mesh.numVerts = poly->numVerts;

			mesh.coordArray = poly->coords;
			mesh.lmCoordArray = poly->lmCoords;
			mesh.normalsArray = poly->normals;
			mesh.vertexArray = poly->vertices;

			RB_RenderMesh (&mesh);

			glSpeeds.worldPolys++;
		}
	}
}


/*
================
R_AddAlphaSurface
================
*/
static void R_AddAlphaSurface (mBspSurface_t *surf, entity_t *ent) {
	surf->textureChain = alphaSurfaces;
	alphaSurfaces = surf;
	surf->entity = ent;
}


/*
================
R_DrawAlphaSurfaces

Transparent surface chain
================
*/
void R_DrawAlphaSurfaces (void) {
	mBspSurface_t	*surf;

	rbCurrModel = r_WorldModel;
	rbCurrEntity = &r_WorldEntity;

	Vector4Set (colorArray[0], 1, 1, 1, 1);

	// TRANS33|TRANS66 chain
	for (surf=alphaSurfaces ; surf ; surf=surf->textureChain) {
		if (surf->entity) {
			rbCurrModel = surf->entity->model;
			rbCurrEntity = surf->entity;
			R_RotateForEntity (surf->entity);

			// save an if and copy it
			R_AddWorldSurface (surf);

			rbCurrModel = r_WorldModel;
			rbCurrEntity = &r_WorldEntity;
			R_LoadModelIdentity ();
		}
		else
			R_AddWorldSurface (surf);
	}

	alphaSurfaces = NULL;
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode (mBspNode_t *node, int clipFlags) {
	cBspPlane_t		*plane;
	mBspSurface_t	*surf, **mark;
	mBspLeaf_t		*leaf;
	float			dist;
	int				i, side;

	Vector4Set (colorArray[0], 1, 1, 1, 1);

	if (node->contents == CONTENTS_SOLID)
		return;	// solid
	if (R_CullVisNode (node))
		return;	// node not visible this frame
	if (R_CullBox (node->mins, node->maxs, clipFlags))
		return;	// node not visible

	// if a leaf node, draw stuff
	if (node->contents != -1) {
		leaf = (mBspLeaf_t *)node;

		// check for door connected areas
		if (r_RefDef.areaBits) {
			if (!(r_RefDef.areaBits[leaf->area>>3] & (1<<(leaf->area&7))))
				return;		// not visible
		}

		// mark visible surfaces
		mark = leaf->firstMarkSurface;
		for (i=leaf->numMarkSurfaces ; i ; ) {
			do {
				(*mark)->surfFrame = r_FrameCount;
				mark++;
			} while (--i);
		}

		glSpeeds.worldLeafs++;
		return;
	}

	// node is just a decision point, so go down the apropriate sides
	plane = node->plane;

	// find which side of the node we are on
	if (plane->type < 3)
		dist = r_RefDef.viewOrigin[plane->type] - plane->dist;
	else
		dist = DotProduct (r_RefDef.viewOrigin, plane->normal) - plane->dist;
	side = (dist >= 0) ? 0 : 1;

	// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side], clipFlags);

	// draw stuff
	for (i=node->numSurfaces, surf=r_WorldModel->surfaces + node->firstSurface ; i ; i--, surf++) {
		if (surf->surfFrame != r_FrameCount)
			continue;	// already touched this frame
		surf->surfFrame = 0;

		if (!r_nocull->integer) {
			// surface culling
			if (!(surf->flags & SURF_PLANEBACK)) {
				if (dist <= SMALL_EPSILON)
					continue;	// wrong side
			}
			else {
				if (dist >= -SMALL_EPSILON)
					continue;	// wrong side
			}

			if (R_CullBox (surf->mins, surf->maxs, clipFlags))
				continue;	// not visible
		}

		if (skyState.loaded && (surf->texInfo->flags & SURF_SKY)) {
			// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texInfo->flags & (SURF_TRANS33|SURF_TRANS66)) {
			// adds to the translucent surface chain
			R_AddAlphaSurface (surf, NULL);
		}
		else {
			// generic surface
			R_AddWorldSurface (surf);
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side], clipFlags);
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
===============
*/
static void R_MarkLeaves (void) {
	static int	r_OldViewCluster2;
	static int	r_ViewCluster2;
	byte		*vis, fatVis[BSP_MAX_VIS];
	int			i, c;
	mBspNode_t	*node;
	mBspLeaf_t	*leaf;

	// current viewcluster
	if (!(r_RefDef.rdFlags & RDF_NOWORLDMODEL)) {
		vec3_t	temp;
		VectorCopy (r_RefDef.viewOrigin, temp);

		r_OldViewCluster = r_ViewCluster;
		r_OldViewCluster2 = r_ViewCluster2;

		leaf = Mod_PointInLeaf (r_RefDef.viewOrigin, r_WorldModel);
		r_ViewCluster = r_ViewCluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents) {
			// look down a bit
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_WorldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_ViewCluster2))
				r_ViewCluster2 = leaf->cluster;
		}
		else {
			// look up a bit
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_WorldModel);
			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_ViewCluster2))
				r_ViewCluster2 = leaf->cluster;
		}
	}

	if ((r_OldViewCluster == r_ViewCluster) &&
		(r_OldViewCluster2 == r_ViewCluster2) &&
		!r_novis->integer && (r_ViewCluster != -1))
		return;

	// development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_VisFrameCount++;
	r_OldViewCluster = r_ViewCluster;
	r_OldViewCluster2 = r_ViewCluster2;

	if (r_novis->integer || (r_ViewCluster == -1) || !r_WorldModel->vis) {
		// mark everything
		for (i=0 ; i<r_WorldModel->numLeafs ; i++)
			r_WorldModel->leafs[i].visFrame = r_VisFrameCount;
		for (i=0 ; i<r_WorldModel->numNodes ; i++)
			r_WorldModel->nodes[i].visFrame = r_VisFrameCount;
		return;
	}

	vis = Mod_ClusterPVS (r_ViewCluster, r_WorldModel);
	// may have to combine two clusters because of solid water boundaries
	if (r_ViewCluster2 != r_ViewCluster) {
		memcpy (fatVis, vis, (r_WorldModel->numLeafs+7)/8);
		vis = Mod_ClusterPVS (r_ViewCluster2, r_WorldModel);
		c = (r_WorldModel->numLeafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];
		vis = fatVis;
	}

	for (i=0, leaf=r_WorldModel->leafs ; i<r_WorldModel->numLeafs ; i++, leaf++) {
		if (leaf->cluster == -1)
			continue;
		if (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))))
			continue;

		node = (mBspNode_t *)leaf;
		do {
			if (node->visFrame == r_VisFrameCount)
				break;

			node->visFrame = r_VisFrameCount;
			node = node->parent;
		} while (node);
	}
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void) {
	R_MarkLeaves ();

	if (!r_drawworld->integer)
		return;
	if (!r_WorldModel)
		return;
	if (r_RefDef.rdFlags & RDF_NOWORLDMODEL)
		return;

	rbCurrModel = r_WorldModel;
	rbCurrEntity = &r_WorldEntity;

	R_ClearSky ();
	R_RecursiveWorldNode (r_WorldModel->nodes, (r_nocull->integer) ? 0 : 15);
	R_DrawSky ();
}

/*
=============================================================================

	BRUSH MODEL

=============================================================================
*/

/*
=================
R_AddBrushModel
=================
*/
void R_AddBrushModel (entity_t *ent) {
	cBspPlane_t		*plane;
	mBspSurface_t	*surf;
	dLight_t		*light;
	qBool			rotated;
	vec3_t			origin;
	float			dist;
	int				i;

	// no surfaces
	if (ent->model->numModelSurfaces == 0)
		return;

	// cull
	if (r_spherecull->integer) {
		rotated = qTrue;

		if (R_CullSphere (ent->origin, ent->model->radius, 15))
			return;
	}
	else {
		vec3_t	mins, maxs;

		rotated = qFalse;

		// calculate bounds
		VectorMA (ent->origin, ent->scale, ent->model->mins, mins);
		VectorMA (ent->origin, ent->scale, ent->model->maxs, maxs);

		if (R_CullBox (mins, maxs, 15))
			return;
	}

	glSpeeds.visEntities++;

	VectorSubtract (r_RefDef.viewOrigin, ent->origin, origin);
	if (rotated) {
		vec3_t		temp;

		VectorCopy (origin, temp);
		Matrix_TransformVector (ent->axis, temp, origin);
	}

	// calculate dynamic lighting for bmodel
	if ((gl_flashblend->integer == 2) || !gl_flashblend->integer) {
		for (light=r_DLights, i=0 ; i<r_NumDLights ; i++, light++)
			R_MarkLights (light, 1<<i, ent->model->nodes + ent->model->firstNode);
	}

	colorArray[0][0] = colorArray[0][1] = colorArray[0][2] = 1;
	colorArray[0][3] = (rbCurrEntity->flags & RF_TRANSLUCENT) ? 0.25 : 1;

	// draw texture
	for (i=0, surf=ent->model->surfaces + ent->model->firstModelSurface ; i<ent->model->numModelSurfaces ; i++, surf++) {
		// find which side of the node we are on
		plane = surf->plane;
		if (plane->type < 3)
			dist = origin[plane->type] - plane->dist;
		else
			dist = DotProduct (origin, plane->normal) - plane->dist;

		if (!r_nocull->integer) {
			if (!(surf->flags & SURF_PLANEBACK)) {
				if (dist <= SMALL_EPSILON)
					continue;	// wrong side
			}
			else {
				if (dist >= -SMALL_EPSILON)
					continue;	// wrong side
			}
		}

		// draw the polygon
		if (surf->texInfo->flags & (SURF_TRANS33|SURF_TRANS66)) {
			// adds to the translucent surface chain
			R_AddAlphaSurface (surf, ent);
		}
		else {
			// generic surface
			R_AddWorldSurface (surf);
		}
	}

	colorArray[0][3] = 1;
}


/*
=============================================================================

	CONSOLE COMMANDS

=============================================================================
*/

/*
=================
R_SetSky_f

Set a specific sky and rotation speed
=================
*/
static void R_SetSky_f (void) {
	float	rotate;
	vec3_t	axis;

	if (!skyState.loaded) {
		Com_Printf (0, "No sky surfaces!\n");
		return;
	}

	if (Cmd_Argc () < 2) {
		Com_Printf (0, "Usage: sky <basename> <rotate> [axis x y z]\n");
		Com_Printf (0, "Currently: sky <%s> <%.1f> [%.1f %.1f %.1f]\n", skyState.baseName, skyState.rotation, skyState.axis[0], skyState.axis[1], skyState.axis[2]);
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
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
R_WorldInit
==================
*/
void R_WorldInit (void) {
	Cmd_AddCommand ("sky",		R_SetSky_f,		"Changes the sky env basename");
}


/*
==================
R_WorldShutdown
==================
*/
void R_WorldShutdown (qBool full) {
	if (!full) {
		// remove commands
		Cmd_RemoveCommand ("sky");
	}
}
