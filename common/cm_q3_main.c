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
// cm_q3_main.c
// Quake3 BSP map model loading
//

#include "cm_q3_local.h"

static byte				*cm_mapBuffer;

int						checkcount;

int						numbrushsides;
cbrushside_t			map_brushsides[MAX_CM_BRUSHSIDES+6];	// extra for box hull

int						numshaderrefs;
cBspSurface_t			map_surfaces[MAX_CM_SHADERS];

int						numplanes;
cBspPlane_t				map_planes[MAX_CM_PLANES+12];			// extra for box hull

int						numnodes;
cnode_t					map_nodes[MAX_CM_NODES+6];				// extra for box hull

int						numleafs = 1;	// allow leaf funcs to be called without a map
cleaf_t					map_leafs[MAX_CM_LEAFS];

int						numleafbrushes;
int						map_leafbrushes[MAX_CM_LEAFBRUSHES+1];	// extra for box hull

int						numbrushes;
cbrush_t				map_brushes[MAX_CM_BRUSHES+1];			// extra for box hull

byte					map_hearability[MAX_CM_VISIBILITY];

int						numvisibility;
byte					map_visibility[MAX_CM_VISIBILITY];
dQ3BspVis_t				*map_pvs = (dQ3BspVis_t *)map_visibility;

dQ3BspVis_t				*map_phs = (dQ3BspVis_t *)map_hearability;

byte					nullrow[MAX_CM_LEAFS/8];

int						numentitychars;
char					map_entitystring[MAX_CM_ENTSTRING];

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
int						numareaportals = 1;
careaportal_t			map_areaportals[MAX_CM_AREAPORTALS];

int						numareas = 1;
carea_t					map_areas[MAX_CM_AREAS];

cBspSurface_t			nullsurface;

int						emptyleaf;

cpatch_t				map_patches[MAX_CM_PATCHES];
int						numpatches;

int						map_leafpatches[MAX_CM_LEAFFACES];
int						numleafpatches;

vec4_t					*map_verts;
int						numvertexes;

cface_t					*map_faces;
int						numfaces;

int						*map_leaffaces;
int						numleaffaces;

/*
=============================================================================

	PATCH LOADING

=============================================================================
*/

/*
===============
Patch_FlatnessTest2
===============
*/
static int Patch_FlatnessTest2 ( float maxflat, vec4_t point0, vec4_t point1, vec4_t point2 )
{
	vec3_t v1, v2, v3;
	vec3_t t, n;
	float dist, d, l;
	int ft0, ft1;

	VectorSubtract ( point2, point0, n );
	l = VectorNormalizef ( n, n );

	if ( !l ) {
		return 0;
	}

	VectorSubtract ( point1, point0, t );
	d = -DotProduct ( t, n );
	VectorMA ( t, d, n, t );
	dist = VectorLength ( t );

	if ( fabs(dist) <= maxflat ) {
		return 0;
	}

	VectorAverage ( point1, point0, v1 );
	VectorAverage ( point2, point1, v2 );
	VectorAverage ( v1, v2, v3 );

	ft0 = Patch_FlatnessTest2 ( maxflat, point0, v1, v3 );
	ft1 = Patch_FlatnessTest2 ( maxflat, v3, v2, point2 );

	return 1 + (int)floor( max ( ft0, ft1 ) + 0.5f );
}

/*
===============
Patch_GetFlatness2
===============
*/
void Patch_GetFlatness2 ( float maxflat, vec4_t *points, int *patch_cp, int *flat )
{
	int i, p, u, v;

	flat[0] = flat[1] = 0;
	for (v = 0; v < patch_cp[1] - 1; v += 2)
	{
		for (u = 0; u < patch_cp[0] - 1; u += 2)
		{
			p = v * patch_cp[0] + u;

			i = Patch_FlatnessTest2 ( maxflat, points[p], points[p+1], points[p+2] );
			flat[0] = max ( flat[0], i );
			i = Patch_FlatnessTest2 ( maxflat, points[p+patch_cp[0]], points[p+patch_cp[0]+1], points[p+patch_cp[0]+2] );
			flat[0] = max ( flat[0], i );
			i = Patch_FlatnessTest2 ( maxflat, points[p+2*patch_cp[0]], points[p+2*patch_cp[0]+1], points[p+2*patch_cp[0]+2] );
			flat[0] = max ( flat[0], i );

			i = Patch_FlatnessTest2 ( maxflat, points[p], points[p+patch_cp[0]], points[p+2*patch_cp[0]] );
			flat[1] = max ( flat[1], i );
			i = Patch_FlatnessTest2 ( maxflat, points[p+1], points[p+patch_cp[0]+1], points[p+2*patch_cp[0]+1] );
			flat[1] = max ( flat[1], i );
			i = Patch_FlatnessTest2 ( maxflat, points[p+2], points[p+patch_cp[0]+2], points[p+2*patch_cp[0]+2] );
			flat[1] = max ( flat[1], i );
		}
	}
}

/*
===============
Patch_Evaluate_QuadricBezier2
===============
*/
static void Patch_Evaluate_QuadricBezier2 ( float t, vec4_t point0, vec4_t point1, vec3_t point2, vec4_t out )
{
	float qt = t * t;
	float dt = 2.0f * t, tt;
	vec4_t tvec4;

	tt = 1.0f - dt + qt;
	Vector4Scale ( point0, tt, out );

	tt = dt - 2.0f * qt;
	Vector4Scale ( point1, tt, tvec4 );
	Vector4Add ( out, tvec4, out );

	Vector4Scale ( point2, qt, tvec4 );
	Vector4Add ( out, tvec4, out );
}

/*
===============
Patch_Evaluate2
===============
*/
void Patch_Evaluate2 ( vec4_t *p, int *numcp, int *tess, vec4_t *dest )
{
	int num_patches[2], num_tess[2];
	int index[3], dstpitch, i, u, v, x, y;
	float s, t, step[2];
	vec4_t *tvec, pv[3][3], v1, v2, v3;

	num_patches[0] = numcp[0] / 2;
	num_patches[1] = numcp[1] / 2;
	dstpitch = num_patches[0] * tess[0] + 1;

	step[0] = 1.0f / (float)tess[0];
	step[1] = 1.0f / (float)tess[1];

	for ( v = 0; v < num_patches[1]; v++ )
	{
		// last patch has one more row 
		if ( v < num_patches[1] - 1 ) {
			num_tess[1] = tess[1];
		} else {
			num_tess[1] = tess[1] + 1;
		}

		for ( u = 0; u < num_patches[0]; u++ )
		{
			// last patch has one more column
			if ( u < num_patches[0] - 1 ) {
				num_tess[0] = tess[0];
			} else {
				num_tess[0] = tess[0] + 1;
			}

			index[0] = (v * numcp[0] + u) * 2;
			index[1] = index[0] + numcp[0];
			index[2] = index[1] + numcp[0];

			// current 3x3 patch control points
			for ( i = 0; i < 3; i++ ) 
			{
				Vector4Copy ( p[index[0]+i], pv[i][0] );
				Vector4Copy ( p[index[1]+i], pv[i][1] );
				Vector4Copy ( p[index[2]+i], pv[i][2] );
			}
			
			t = 0.0f;
			tvec = dest + v * tess[1] * dstpitch + u * tess[0];

			for ( y = 0; y < num_tess[1]; y++, t += step[1] )
			{
				Patch_Evaluate_QuadricBezier2 ( t, pv[0][0], pv[0][1], pv[0][2], v1 );
				Patch_Evaluate_QuadricBezier2 ( t, pv[1][0], pv[1][1], pv[1][2], v2 );
				Patch_Evaluate_QuadricBezier2 ( t, pv[2][0], pv[2][1], pv[2][2], v3 );

				s = 0.0f;
				for ( x = 0; x < num_tess[0]; x++, s += step[0] )
				{
					Patch_Evaluate_QuadricBezier2 ( s, v1, v2, v3, tvec[x] );
				}

				tvec += dstpitch;
			}
		}
	}
}

void CM_CreateBrush (cbrush_t *brush, vec3_t *verts, cBspSurface_t *surface )
{
	int	i, j, k, sign;
	vec3_t v1, v2;
	vec3_t	absmins, absmaxs;
	cbrushside_t	*side;
	cBspPlane_t *plane;
	static cBspPlane_t mainplane, patchplanes[20];
	qBool skip[20];
	int	numpatchplanes = 0;

	// calc absmins & absmaxs
	ClearBounds ( absmins, absmaxs );
	for (i = 0; i < 3; i++)
		AddPointToBounds ( verts[i], absmins, absmaxs );

	PlaneFromPoints ( verts, &mainplane );

	// front plane
	plane = &patchplanes[numpatchplanes++];
	*plane = mainplane;

	// back plane
	plane = &patchplanes[numpatchplanes++];
	VectorNegate (mainplane.normal, plane->normal);
	plane->dist = -mainplane.dist;

	// axial planes
	for ( i = 0; i < 3; i++ ) {
		for (sign = -1; sign <= 1; sign += 2) {
			plane = &patchplanes[numpatchplanes++];
			VectorClear ( plane->normal );
			plane->normal[i] = sign;
			plane->dist = sign > 0 ? absmaxs[i] : -absmins[i];
		}
	}

	// edge planes
	for ( i = 0; i < 3; i++ ) {
		vec3_t	normal;

		VectorCopy (verts[i], v1);
		VectorCopy (verts[(i + 1) % 3], v2);

		for ( k = 0; k < 3; k++ ) {
			normal[k] = 0;
			normal[(k+1)%3] = v1[(k+2)%3] - v2[(k+2)%3];
			normal[(k+2)%3] = -(v1[(k+1)%3] - v2[(k+1)%3]);

			if (VectorCompare (normal, vec3Origin))
				continue;

			plane = &patchplanes[numpatchplanes++];

			VectorNormalizef ( normal, normal );
			VectorCopy ( normal, plane->normal );
			plane->dist = DotProduct (plane->normal, v1);

			if ( DotProduct(verts[(i + 2) % 3], normal) - plane->dist > 0 )
			{	// invert
				VectorInverse ( plane->normal );
				plane->dist = -plane->dist;
			}
		}
	}

	// set plane->type and mark duplicate planes for removal
	for (i = 0; i < numpatchplanes; i++)
	{
		CategorizePlane ( &patchplanes[i] );
		skip[i] = qFalse;

		for (j = i + 1; j < numpatchplanes; j++)
			if ( patchplanes[j].dist == patchplanes[i].dist
				&& VectorCompare (patchplanes[j].normal, patchplanes[i].normal) )
			{
				skip[i] = qTrue;
				break;
			}
	}

	brush->numSides = 0;
	brush->firstBrushSide = numbrushsides;

	for (k = 0; k < 2; k++) {
		for (i = 0; i < numpatchplanes; i++)	{
			if (skip[i])
				continue;

			// first, store all axially aligned planes
			// then store everything else
			// does it give a noticeable speedup?
			if (!k && patchplanes[i].type >= 3)
				continue;

			skip[i] = qTrue;

			if (numplanes == MAX_CM_PLANES)
				Com_Error (ERR_DROP, "CM_CreateBrush: numplanes == MAX_CM_PLANES");

			plane = &map_planes[numplanes++];
			*plane = patchplanes[i];

			if (numbrushsides == MAX_CM_BRUSHSIDES)
				Com_Error (ERR_DROP, "CM_CreateBrush: numbrushsides == MAX_CM_BRUSHSIDES");

			side = &map_brushsides[numbrushsides++];
			side->plane = plane;

			if (DotProduct(plane->normal, mainplane.normal) >= 0)
				side->surface = surface;
			else
				side->surface = NULL;	// don't clip against this side

			brush->numSides++;
		}
	}
}

void CM_CreatePatch ( cpatch_t *patch, int numverts, vec4_t *verts, int *patch_cp )
{
    int step[2], size[2], flat[2], i, u, v;
	vec4_t points[MAX_CM_PATCH_VERTS];
	vec3_t tverts[4], tverts2[4];
	cbrush_t *brush;
	cBspPlane_t mainplane;

// find the degree of subdivision in the u and v directions
	Patch_GetFlatness2 ( CM_SUBDIVLEVEL, verts, patch_cp, flat );

	step[0] = (1 << flat[0]);
	step[1] = (1 << flat[1]);
	size[0] = (patch_cp[0] / 2) * step[0] + 1;
	size[1] = (patch_cp[1] / 2) * step[1] + 1;

	if ( size[0] * size[1] > MAX_CM_PATCH_VERTS ) {
		Com_Error ( ERR_DROP, "CM_CreatePatch: patch has too many vertices" );
		return;
	}

// fill in
	Patch_Evaluate2 ( verts, patch_cp, step, points );

	patch->brushes = brush = map_brushes + numbrushes;
	patch->numBrushes = 0;

	ClearBounds (patch->absMins, patch->absMaxs);

// create a set of brushes
    for (v = 0; v < size[1]-1; v++)
    {
		for (u = 0; u < size[0]-1; u++)
		{
			if (numbrushes >= MAX_CM_BRUSHES)
				Com_Error (ERR_DROP, "CM_CreatePatch: too many patch brushes");

			i = v * size[0] + u;
			VectorCopy (points[i], tverts[0]);
			VectorCopy (points[i + size[0]], tverts[1]);
			VectorCopy (points[i + 1], tverts[2]);
			VectorCopy (points[i + size[0] + 1], tverts[3]);

			for (i = 0; i < 4; i++)
				AddPointToBounds (tverts[i], patch->absMins, patch->absMaxs);

			PlaneFromPoints (tverts, &mainplane);

			// create two brushes
			CM_CreateBrush (brush, tverts, patch->surface);

			brush->contents = patch->surface->contents;
			brush++; numbrushes++; patch->numBrushes++;

			VectorCopy (tverts[2], tverts2[0]);
			VectorCopy (tverts[1], tverts2[1]);
			VectorCopy (tverts[3], tverts2[2]);
			CM_CreateBrush (brush, tverts2, patch->surface);

			brush->contents = patch->surface->contents;
			brush++; numbrushes++; patch->numBrushes++;
		}
    }
}

// ==========================================================================

/*
=================
CM_CreatePatchesForLeafs
=================
*/
void CM_CreatePatchesForLeafs (void)
{
	int i, j, k;
	cleaf_t *leaf;
	cface_t *face;
	cBspSurface_t *surf;
	cpatch_t *patch;
	int checkout[MAX_CM_FACES];

	memset (checkout, -1, sizeof(int)*MAX_CM_FACES);

	for (i=0, leaf=map_leafs ; i<numleafs ; i++, leaf++) {
		leaf->numLeafPatches = 0;
		leaf->firstLeafPatch = numleafpatches;

		if (leaf->cluster == -1)
			continue;

		for (j=0 ; j<leaf->numLeafFaces ; j++)
		{
			k = leaf->firstLeafFace + j;
			if (k >= numleaffaces)
				break;

			k = map_leaffaces[k];
			face = &map_faces[k];

			if (face->faceType != FACETYPE_PATCH || face->numVerts <= 0)
				continue;
			if (face->patch_cp[0] <= 0 || face->patch_cp[1] <= 0)
				continue;
			if (face->shaderNum < 0 || face->shaderNum >= numshaderrefs)
				continue;

			surf = &map_surfaces[face->shaderNum];
			if (!surf->contents || (surf->flags & SHREF_NONSOLID))
				continue;

			if (numleafpatches >= MAX_CM_LEAFFACES)
				Com_Error (ERR_DROP, "CM_CreatePatchesForLeafs: map has too many faces");

			// the patch was already built
			if (checkout[k] != -1) {
				map_leafpatches[numleafpatches] = checkout[k];
				patch = &map_patches[checkout[k]];
			}
			else {
				if (numpatches >= MAX_CM_PATCHES)
					Com_Error (ERR_DROP, "CM_CreatePatchesForLeafs: map has too many patches");

				patch = &map_patches[numpatches];
				patch->surface = surf;
				map_leafpatches[numleafpatches] = numpatches;
				checkout[k] = numpatches++;

				CM_CreatePatch ( patch, face->numVerts, map_verts + face->firstVert, face->patch_cp );
			}

			leaf->contents |= patch->surface->contents;
			leaf->numLeafPatches++;

			numleafpatches++;
		}
	}
}

/*
=============================================================================

	QUAKE3 BSP LOADING

=============================================================================
*/

/*
=================
CM_LoadQ3BSPVertexes
=================
*/
static void CM_LoadQ3BSPVertexes (dQ3BspLump_t *l)
{
	dQ3BspVertex_t	*in;
	vec4_t			*out;
	int				i, j;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPVertexes: funny lump size");

	numvertexes = l->fileLen / sizeof (*in);
	if (numvertexes > MAX_CM_VERTEXES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPVertexes: Map has too many vertexes");
	map_verts = out = Mem_AllocExt (numvertexes * sizeof (*out), qFalse);

	for (i=0 ; i<numvertexes ; i++, in++) {
		for (j=0 ; j<3 ; j++)
			out[i][j] = LittleFloat (in->point[j]);
	}
}


/*
=================
CM_LoadQ3BSPFaces
=================
*/
static void CM_LoadQ3BSPFaces (dQ3BspLump_t *l)
{
	dQ3BspFace_t	*in;
	cface_t			*out;
	int				i;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPFaces: funny lump size");

	numfaces = l->fileLen / sizeof (*in);
	if (numfaces > MAX_CM_FACES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPFaces: Map has too many faces");
	map_faces = out = Mem_AllocExt (numfaces * sizeof (*out), qFalse);

	for (i=0 ; i<numfaces ; i++, in++, out++) {
		out->faceType = LittleLong (in->faceType);
		out->shaderNum = LittleLong (in->shaderNum);

		out->numVerts = LittleLong (in->numVerts);
		out->firstVert = LittleLong (in->firstVert);

		out->patch_cp[0] = LittleLong (in->patch_cp[0]);
		out->patch_cp[1] = LittleLong (in->patch_cp[1]);
	}
}


/*
=================
CM_LoadQ3BSPLeafFaces
=================
*/
static void CM_LoadQ3BSPLeafFaces (dQ3BspLump_t *l)
{
	int		i, j;
	int		*in;
	int		*out;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafFaces: funny lump size");

	numleaffaces = l->fileLen / sizeof(*in);
	if (numleaffaces > MAX_CM_LEAFFACES) 
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafFaces: Map has too many leaffaces"); 
	map_leaffaces = out = Mem_AllocExt (numleaffaces*sizeof(*out), qFalse);

	for (i=0 ; i<numleaffaces ; i++) {
		j = LittleLong (in[i]);
		if (j < 0 ||  j >= numfaces)
			Com_Error (ERR_DROP, "CMod_LoadLeafFaces: bad surface number");

		out[i] = j;
	}
}


/*
=================
CM_LoadQ3BSPSubmodels
=================
*/
static void CM_LoadQ3BSPSubmodels (dQ3BspLump_t *l)
{
	dQ3BspModel_t	*in;
	cBspModel_t		*out;
	cleaf_t			*bleaf;
	int				*leafbrush;
	int				i, j;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSubmodels: funny lump size");

	cm_numCModels = l->fileLen / sizeof (*in);
	if (cm_numCModels < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSubmodels: Map with no models");
	else if (cm_numCModels > MAX_CM_MODELS)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSubmodels: Map has too many models");

	for (i=0 ; i<cm_numCModels ; i++, in++, out++) {
		out = &cm_mapCModels[i];

		if (!i) {
			out->headNode = 0;
		}
		else {
			out->headNode = -1 - numleafs;

			bleaf = &map_leafs[numleafs++];
			bleaf->numLeafBrushes = LittleLong (in->numBrushes);
			bleaf->firstLeafBrush = numleafbrushes;
			bleaf->contents = 0;

			leafbrush = &map_leafbrushes[numleafbrushes];
			for (j=0 ; j<bleaf->numLeafBrushes ; j++, leafbrush++) {
				*leafbrush = LittleLong (in->firstBrush) + j;
				bleaf->contents |= map_brushes[*leafbrush].contents;
			}

			numleafbrushes += bleaf->numLeafBrushes;
		}

		for (j=0 ; j<3 ; j++) {
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}
	}
}


/*
=================
CM_LoadQ3BSPSurfaces
=================
*/
static void CM_LoadQ3BSPSurfaces (dQ3BspLump_t *l)
{
	dQ3BspShaderRef_t	*in;
	cBspSurface_t		*out;
	int					i;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSurfaces: funny lump size");

	numshaderrefs = l->fileLen / sizeof(*in);
	if (numshaderrefs < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSurfaces: Map with no shaders");
	else if (numshaderrefs > MAX_CM_SHADERS)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPSurfaces: Map has too many shaders");

	out = map_surfaces;
	for (i=0 ; i<numshaderrefs ; i++, in++, out++) {
		out->flags = LittleLong (in->flags);
		out->contents = LittleLong (in->contents);
	}
}


/*
=================
CM_LoadQ3BSPNodes
=================
*/
static void CM_LoadQ3BSPNodes (dQ3BspLump_t *l)
{
	dQ3BspNode_t	*in;
	int				child;
	cnode_t			*out;
	int				i, j;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPNodes: funny lump size");

	numnodes = l->fileLen / sizeof (*in);
	if (numnodes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPNodes: Map has no nodes");
	else if (numnodes > MAX_CM_NODES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPNodes: Map has too many nodes");

	out = map_nodes;
	for (i=0 ; i<numnodes ; i++, out++, in++) {
		out->plane = map_planes + LittleLong (in->planeNum);
		for (j=0 ; j<2 ; j++) {
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
	}
}


/*
=================
CM_LoadQ3BSPBrushes
=================
*/
static void CM_LoadQ3BSPBrushes (dQ3BspLump_t *l)
{
	dQ3BspBrush_t	*in;
	cbrush_t		*out;
	int				i, shaderref;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPBrushes: funny lump size");

	numbrushes = l->fileLen / sizeof(*in);
	if (numbrushes > MAX_CM_BRUSHES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPBrushes: Map has too many brushes");

	out = map_brushes;
	for (i=0 ; i<numbrushes ; i++, out++, in++) {
		shaderref = LittleLong (in->shaderNum);
		out->contents = map_surfaces[shaderref].contents;
		out->firstBrushSide = LittleLong (in->firstSide);
		out->numSides = LittleLong (in->numSides);
	}
}


/*
=================
CM_LoadQ3BSPLeafs
=================
*/
static void CM_LoadQ3BSPLeafs (dQ3BspLump_t *l)
{
	int				i, j;
	cleaf_t			*out;
	dQ3BspLeaf_t 	*in;
	cbrush_t		*brush;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafs: funny lump size");

	numleafs = l->fileLen / sizeof(*in);
	if (numleafs < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafs: Map with no leafs");
	else if (numleafs > MAX_CM_LEAFS)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafs: Map has too many leafs");

	out = map_leafs;
	emptyleaf = -1;

	for (i=0 ; i<numleafs ; i++, in++, out++) {
		out->cluster = LittleLong (in->cluster);
		out->area = LittleLong (in->area) + 1;
		out->firstLeafFace = LittleLong (in->firstLeafFace);
		out->numLeafFaces = LittleLong (in->numLeafFaces);
		out->contents = 0;
		out->firstLeafBrush = LittleLong (in->firstLeafBrush);
		out->numLeafBrushes = LittleLong (in->numLeafBrushes);

		for (j=0 ; j<out->numLeafBrushes ; j++) {
			brush = &map_brushes[map_leafbrushes[out->firstLeafBrush+j]];
			out->contents |= brush->contents;
		}

		if (out->area >= numareas)
			numareas = out->area + 1;

		if (!out->contents)
			emptyleaf = i;
	}

	// If map doesn't have an empty leaf - force one
	if (emptyleaf == -1) {
		if (numleafs >= MAX_CM_LEAFS-1)
			Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafs: Map does not have an empty leaf");

		out->cluster = -1;
		out->area = -1;
		out->numLeafBrushes = 0;
		out->contents = 0;
		out->firstLeafBrush = 0;

		Com_DevPrintf (0, "CM_LoadQ3BSPLeafs: Forcing an empty leaf: %i\n", numleafs);
		emptyleaf = numleafs++;
	}
}


/*
=================
CM_LoadQ3BSPPlanes
=================
*/
static void CM_LoadQ3BSPPlanes (dQ3BspLump_t *l)
{
	int				i;
	cBspPlane_t		*out;
	dQ3BspPlane_t 	*in;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPPlanes: funny lump size");

	numplanes = l->fileLen / sizeof(*in);
	if (numplanes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPPlanes: Map with no planes");
	else if (numplanes > MAX_CM_PLANES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPPlanes: Map has too many planes");

	out = map_planes;
	for (i=0 ; i<numplanes ; i++, in++, out++) {
		out->normal[0] = LittleFloat (in->normal[0]);
		out->normal[1] = LittleFloat (in->normal[1]);
		out->normal[2] = LittleFloat (in->normal[2]);

		out->dist = LittleFloat (in->dist);

		CategorizePlane (out);
	}
}


/*
=================
CM_LoadQ3BSPLeafBrushes
=================
*/
static void CM_LoadQ3BSPLeafBrushes (dQ3BspLump_t *l)
{
	int			i;
	int			*out;
	int		 	*in;
	
	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafBrushes: funny lump size");

	numleafbrushes = l->fileLen / sizeof(*in);
	if (numleafbrushes < 1)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafBrushes: Map with no planes");
	else if (numleafbrushes > MAX_CM_LEAFBRUSHES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPLeafBrushes: Map has too many leafbrushes");

	out = map_leafbrushes;
	for ( i=0 ; i<numleafbrushes ; i++, in++, out++)
		*out = LittleLong (*in);
}


/*
=================
CM_LoadQ3BSPBrushSides
=================
*/
static void CM_LoadQ3BSPBrushSides (dQ3BspLump_t *l)
{
	int					i, j;
	cbrushside_t		*out;
	dQ3BspBrushSide_t 	*in;

	in = (void *)(cm_mapBuffer + l->fileOfs);
	if (l->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "CM_LoadQ3BSPBrushSides: funny lump size");

	numbrushsides = l->fileLen / sizeof(*in);
	if (numbrushsides > MAX_CM_BRUSHSIDES)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPBrushSides: Map has too many brushsides");

	out = map_brushsides;
	for (i=0 ; i<numbrushsides ; i++, in++, out++) {
		out->plane = map_planes + LittleLong (in->planeNum);
		j = LittleLong (in->shaderNum);
		if (j >= numshaderrefs)
			Com_Error (ERR_DROP, "Bad brushside texinfo");
		out->surface = &map_surfaces[j];
	}
}


/*
=================
CM_LoadQ3BSPVisibility
=================
*/
static void CM_LoadQ3BSPVisibility (dQ3BspLump_t *l)
{
	numvisibility = l->fileLen;
	if (l->fileLen > MAX_CM_VISIBILITY)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPVisibility: Map has too large visibility lump");

	memcpy (map_visibility, cm_mapBuffer + l->fileOfs, l->fileLen);

	map_pvs->numClusters = LittleLong (map_pvs->numClusters);
	map_pvs->rowSize = LittleLong (map_pvs->rowSize);
}


/*
=================
CM_LoadQ3BSPEntityString
=================
*/
static void CM_LoadQ3BSPEntityString (dQ3BspLump_t *l)
{
	numentitychars = l->fileLen;
	if (l->fileLen > MAX_CM_ENTSTRING)
		Com_Error (ERR_DROP, "CM_LoadQ3BSPEntityString: Map has too large entity lump");

	memcpy (map_entitystring, cm_mapBuffer + l->fileOfs, l->fileLen);
}


/*
=================
CM_CalcPHS
=================
*/
void CM_CalcPHS (void)
{
	int		rowbytes, rowwords;
	int		i, j, k, l, index;
	int		bitbyte;
	unsigned	*dest, *src;
	byte	*scan;
	int		count, vcount;
	int		numClusters;

	Com_DevPrintf (0, "CM_CalcPHS: Building PHS...\n");

	rowwords = map_pvs->rowSize / sizeof(long);
	rowbytes = map_pvs->rowSize;

	memset (map_phs, 0, MAX_CM_VISIBILITY);

	map_phs->rowSize = map_pvs->rowSize;
	map_phs->numClusters = numClusters = map_pvs->numClusters;

	vcount = 0;
	for (i=0 ; i<numClusters ; i++) {
		scan = CM_Q3BSP_ClusterPVS (i);
		for (j=0 ; j<numClusters ; j++) {
			if (scan[j>>3] & (1<<(j&7)))
				vcount++;
		}
	}

	count = 0;
	scan = (byte *)map_pvs->data;
	dest = (unsigned *)((byte *)map_phs + 8);

	for (i=0 ; i<numClusters ; i++, dest += rowwords, scan += rowbytes) {
		memcpy (dest, scan, rowbytes);
		for (j=0 ; j<rowbytes ; j++) {
			bitbyte = scan[j];
			if (!bitbyte)
				continue;
			for (k=0 ; k<8 ; k++) {
				if (!(bitbyte & (1<<k)))
					continue;

				// OR this pvs row into the phs
				index = (j<<3) + k;
				if (index >= numClusters)
					Com_Error (ERR_DROP, "CM_CalcPHS: Bad bit in PVS");	// pad bits should be 0
				src = (unsigned *)((byte*)map_pvs->data) + index*rowwords;
				for (l=0 ; l<rowwords ; l++)
					dest[l] |= src[l];
			}
		}
		for (j=0 ; j<numClusters ; j++)
			if ( ((byte *)dest)[j>>3] & (1<<(j&7)) )
				count++;
	}

	Com_DevPrintf (0, "CM_CalcPHS: Average clusters visible / hearable / total: %i / %i / %i\n"
		, vcount ? vcount/numClusters : 0,
		count ? count/numClusters : 0, numClusters);
}

// ==========================================================================

/*
=================
CM_Q3BSP_LoadMap
=================
*/
cBspModel_t *CM_Q3BSP_LoadMap (uint32 *buffer)
{
	dQ3BspHeader_t	header;
	int				i;

	//
	// Byte swap
	//
	header = *(dQ3BspHeader_t *)buffer;
	for (i=0 ; i<sizeof (dQ3BspHeader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong (((int *)&header)[i]);
	cm_mapBuffer = (byte *)buffer;

	//
	// Load into heap
	//
	CM_LoadQ3BSPSurfaces		(&header.lumps[Q3BSP_LUMP_SHADERREFS]);
	CM_LoadQ3BSPPlanes			(&header.lumps[Q3BSP_LUMP_PLANES]);
	CM_LoadQ3BSPLeafBrushes		(&header.lumps[Q3BSP_LUMP_LEAFBRUSHES]);
	CM_LoadQ3BSPBrushes			(&header.lumps[Q3BSP_LUMP_BRUSHES]);
	CM_LoadQ3BSPBrushSides		(&header.lumps[Q3BSP_LUMP_BRUSHSIDES]);
	CM_LoadQ3BSPVertexes		(&header.lumps[Q3BSP_LUMP_VERTEXES]);
	CM_LoadQ3BSPFaces			(&header.lumps[Q3BSP_LUMP_FACES]);
	CM_LoadQ3BSPLeafFaces		(&header.lumps[Q3BSP_LUMP_LEAFFACES]);
	CM_LoadQ3BSPLeafs			(&header.lumps[Q3BSP_LUMP_LEAFS]);
	CM_LoadQ3BSPNodes			(&header.lumps[Q3BSP_LUMP_NODES]);
	CM_LoadQ3BSPSubmodels		(&header.lumps[Q3BSP_LUMP_MODELS]);
	CM_LoadQ3BSPVisibility		(&header.lumps[Q3BSP_LUMP_VISIBILITY]);
	CM_LoadQ3BSPEntityString	(&header.lumps[Q3BSP_LUMP_ENTITIES]);

	CM_CreatePatchesForLeafs ();

	CM_Q3BSP_InitBoxHull ();
	CM_Q3BSP_PrepMap ();

	CM_CalcPHS ();

	if (map_verts)
		Mem_Free (map_verts);
	if (map_faces)
		Mem_Free (map_faces);
	if (map_leaffaces)
		Mem_Free (map_leaffaces);

	memset (nullrow, 255, MAX_CM_LEAFS / 8);

	return &cm_mapCModels[0];
}


/*
==================
CM_Q3BSP_PrepMap
==================
*/
void CM_Q3BSP_PrepMap (void)
{
	CM_Q3BSP_FloodAreaConnections ();
}


/*
==================
CM_Q3BSP_UnloadMap
==================
*/
void CM_Q3BSP_UnloadMap (void)
{
	numplanes = 0;
	numnodes = 0;
	numleafs = 0;
	numvisibility = 0;
	numentitychars = 0;
	numvertexes = 0;
	numfaces = 0;
	numleaffaces = 0;
	numpatches = 0;
	numleafpatches = 0;
	map_entitystring[0] = 0;

	numleafs = 1;
	numareas = 1;
}

/*
=============================================================================

	QUAKE3 BSP INFORMATION

=============================================================================
*/

/*
==================
CM_Q3BSP_EntityString
==================
*/
char *CM_Q3BSP_EntityString (void)
{
	return map_entitystring;
}


/*
==================
CM_Q3BSP_SurfRName
==================
*/
char *CM_Q3BSP_SurfRName (int texNum)
{
	return NULL;
}


/*
==================
CM_Q3BSP_NumClusters
==================
*/
int CM_Q3BSP_NumClusters (void)
{
	return map_pvs->numClusters;
}

/*
==================
CM_Q3BSP_NumTexInfo
==================
*/
int CM_Q3BSP_NumTexInfo (void)
{
	return 0;
}
