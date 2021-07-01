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
// r_model.c
// Model loading and caching
//

#define USE_BYTESWAP
#include "r_local.h"

#define MAX_MODEL_KNOWN		512

static model_t	r_inlineModels[MAX_MODEL_KNOWN];	// the inline * models from the current map are kept seperate
static model_t	r_modelList[MAX_MODEL_KNOWN];
static int		r_numModels;

static byte		r_q2BspNoVis[Q2BSP_MAX_VIS];
static byte		r_q3BspNoVis[Q3BSP_MAX_VIS];

cVar_t	*flushmap;

extern int		r_q2lmSize;

// ============================================================================

#define R_ModAllocZero(model,size) _Mem_Alloc ((size),qTrue,MEMPOOL_MODELSYS,(model)->memTag,__FILE__,__LINE__)
#define R_ModAlloc(model,size) _Mem_Alloc ((size),qFalse,MEMPOOL_MODELSYS,(model)->memTag,__FILE__,__LINE__)

/*
===============
R_FindTriangleWithEdge
===============
*/
static int R_FindTriangleWithEdge (index_t *indexes, int numTris, index_t start, index_t end, int ignore)
{
	int		i, match, count;

	count = 0;
	match = -1;

	for (i=0 ; i<numTris ; i++, indexes += 3) {
		if ((indexes[0] == start && indexes[1] == end)
		|| (indexes[1] == start && indexes[2] == end)
		|| (indexes[2] == start && indexes[0] == end)) {
			if (i != ignore)
				match = i;
			count++;
		}
		else if ((indexes[1] == start && indexes[0] == end)
		|| (indexes[2] == start && indexes[1] == end)
		|| (indexes[0] == start && indexes[2] == end)) {
			count++;
		}
	}

	// Detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}


/*
===============
R_BuildTriangleNeighbors
===============
*/
static void R_BuildTriangleNeighbors (int *neighbors, index_t *indexes, int numTris)
{
	index_t	*index;
	int		i, *nb;

	for (i=0, index=indexes, nb=neighbors ; i<numTris ; i++) {
		nb[0] = R_FindTriangleWithEdge (indexes, numTris, index[1], index[0], i);
		nb[1] = R_FindTriangleWithEdge (indexes, numTris, index[2], index[1], i);
		nb[2] = R_FindTriangleWithEdge (indexes, numTris, index[0], index[2], i);

		index += 3;
		nb += 3;
	}
}

/*
===============================================================================

	MD2 LOADING

===============================================================================
*/

/*
=================
R_LoadMD2Model
=================
*/
static void R_LoadMD2Model (model_t *model, byte *buffer)
{
	int				i, j, k;
	int				version, frameSize;
	int				skinWidth, skinHeight;
	int				numVerts, numIndexes;
	double			isw, ish;
	int				indRemap[MD2_MAX_TRIANGLES*3];
	index_t			tempIndex[MD2_MAX_TRIANGLES*3];
	index_t			tempSTIndex[MD2_MAX_TRIANGLES*3];
	dMd2Coord_t		*inCoord;
	dMd2Frame_t		*inFrame;
	dMd2Header_t	*inModel;
	dMd2Triangle_t	*inTri;
	vec2_t			*outCoord;
	mAliasFrame_t	*outFrame;
	index_t			*outIndex;
	mAliasMesh_t	*outMesh;
	mAliasModel_t	*outModel;
	mAliasSkin_t	*outSkins;
	mAliasVertex_t	*outVertex;
	vec3_t			normal;
	char			*temp;
	byte			*allocBuffer;

	inModel = (dMd2Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD2_MODEL_VERSION)
		Com_Error (ERR_DROP, "R_LoadMD2Model: '%s' has wrong version number (%i != %i)",
							model->name, version, MD2_MODEL_VERSION);

	allocBuffer = R_ModAlloc (model, sizeof (mAliasModel_t) + sizeof (mAliasMesh_t));

	outModel = model->aliasModel = (mAliasModel_t *)allocBuffer;
	model->type = MODEL_MD2;

	//
	// Load the mesh
	//
	allocBuffer += sizeof (mAliasModel_t);
	outMesh = outModel->meshes = (mAliasMesh_t *)allocBuffer;
	outModel->numMeshes = 1;

	Q_strncpyz (outMesh->name, "default", sizeof (outMesh->name));

	outMesh->numVerts = LittleLong (inModel->numVerts);
	if (outMesh->numVerts <= 0 || outMesh->numVerts > MD2_MAX_VERTS)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has an invalid amount of vertices '%d'",
							model->name, outMesh->numVerts);

	outMesh->numTris = LittleLong (inModel->numTris);
	if (outMesh->numTris <= 0 || outMesh->numTris > MD2_MAX_TRIANGLES)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has an invalid amount of triangles '%d'",
							model->name, outMesh->numTris);

	frameSize = LittleLong (inModel->frameSize);
	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames <= 0 || outModel->numFrames > MD2_MAX_FRAMES)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	//
	// Load the skins
	//
	skinWidth = LittleLong (inModel->skinWidth);
	skinHeight = LittleLong (inModel->skinHeight);
	if (skinWidth <= 0 || skinHeight <= 0)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has invalid skin dimensions '%d x %d'",
							model->name, skinWidth, skinHeight);

	outMesh->numSkins = LittleLong (inModel->numSkins);
	if (outMesh->numSkins < 0 || outMesh->numSkins > MD2_MAX_SKINS)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has an invalid amount of skins '%d'",
							model->name, outMesh->numSkins);

	isw = 1.0 / (double)skinWidth;
	ish = 1.0 / (double)skinHeight;

	//
	// No tags
	//
	outModel->numTags = 0;
	outModel->tags = NULL;

	//
	// Load the indexes
	//
	numIndexes = outMesh->numTris * 3;
	outIndex = outMesh->indexes = R_ModAlloc (model, sizeof (index_t) * numIndexes);

	//
	// Load triangle lists
	//
	inTri = (dMd2Triangle_t *) ((byte *)inModel + LittleLong (inModel->ofsTris));
	inCoord = (dMd2Coord_t *) ((byte *)inModel + LittleLong (inModel->ofsST));

	for (i=0, k=0 ; i <outMesh->numTris; i++, k+=3) {
		tempIndex[k+0] = (index_t)LittleShort (inTri[i].vertsIndex[0]);
		tempIndex[k+1] = (index_t)LittleShort (inTri[i].vertsIndex[1]);
		tempIndex[k+2] = (index_t)LittleShort (inTri[i].vertsIndex[2]);

		tempSTIndex[k+0] = (index_t)LittleShort (inTri[i].stIndex[0]);
		tempSTIndex[k+1] = (index_t)LittleShort (inTri[i].stIndex[1]);
		tempSTIndex[k+2] = (index_t)LittleShort (inTri[i].stIndex[2]);
	}

	//
	// Build list of unique vertexes
	//
	numVerts = 0;
	memset (indRemap, -1, sizeof (int) * MD2_MAX_TRIANGLES * 3);

	for (i=0 ; i<numIndexes ; i++) {
		if (indRemap[i] != -1)
			continue;

		// Remap duplicates
		for (j=i+1 ; j<numIndexes ; j++) {
			if ((tempIndex[j] == tempIndex[i])
				&& (inCoord[tempSTIndex[j]].s == inCoord[tempSTIndex[i]].s)
				&& (inCoord[tempSTIndex[j]].t == inCoord[tempSTIndex[i]].t)) {
				indRemap[j] = i;
				outIndex[j] = numVerts;
			}
		}

		// Add unique vertex
		indRemap[i] = i;
		outIndex[i] = numVerts++;
	}

	if (numVerts <= 0 || numVerts >= ALIAS_MAX_VERTS)
		Com_Error (ERR_DROP, "R_LoadMD2Model: model '%s' has an invalid amount of resampled verts for an alias model '%d' >= ALIAS_MAX_VERTS",
							numVerts, ALIAS_MAX_VERTS);

	Com_DevPrintf (0, "R_LoadMD2Model: '%s' remapped %i verts to %i (%i tris)\n",
							model->name, outMesh->numVerts, numVerts, outMesh->numTris);
	outMesh->numVerts = numVerts;

	//
	// Remap remaining indexes
	//
	for (i=0 ; i<numIndexes; i++) {
		if (indRemap[i] == i)
			continue;

		outIndex[i] = outIndex[indRemap[i]];
	}

	//
	// Load base s and t vertices
	//
#ifdef SHADOW_VOLUMES
	allocBuffer = R_ModAlloc (model, (sizeof (vec2_t) * numVerts)
									+ (sizeof (mAliasFrame_t) * outModel->numFrames)
									+ (sizeof (mAliasVertex_t) * outModel->numFrames * numVerts)
									+ (sizeof (vec3_t) * outModel->numFrames * 2)
									+ (sizeof (float) * outModel->numFrames)
									+ (sizeof (int) * outMesh->numTris * 3)
									+ (sizeof (mAliasSkin_t) * outMesh->numSkins));
#else
	allocBuffer = R_ModAlloc (model, (sizeof (vec2_t) * numVerts)
									+ (sizeof (mAliasFrame_t) * outModel->numFrames)
									+ (sizeof (mAliasVertex_t) * outModel->numFrames * numVerts)
									+ (sizeof (vec3_t) * outModel->numFrames * 2)
									+ (sizeof (float) * outModel->numFrames)
									+ (sizeof (mAliasSkin_t) * outMesh->numSkins));
#endif
	outCoord = outMesh->coords = (vec2_t *)allocBuffer;

	for (j=0 ; j<numIndexes ; j++) {
		outCoord[outIndex[j]][0] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].s) + 0.5) * isw);
		outCoord[outIndex[j]][1] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].t) + 0.5) * ish);
	}

	//
	// Load the frames
	//
	allocBuffer += sizeof (vec2_t) * numVerts;
	outFrame = outModel->frames = (mAliasFrame_t *)allocBuffer;

	allocBuffer += sizeof (mAliasFrame_t) * outModel->numFrames;
	outVertex = outMesh->vertexes = (mAliasVertex_t *)allocBuffer;

	allocBuffer += sizeof (mAliasVertex_t) * outModel->numFrames * numVerts;
	outMesh->mins = (vec3_t *)allocBuffer;
	allocBuffer += sizeof (vec3_t) * outModel->numFrames;
	outMesh->maxs = (vec3_t *)allocBuffer;
	allocBuffer += sizeof (vec3_t) * outModel->numFrames;
	outMesh->radius = (float *)allocBuffer;

	for (i=0 ; i<outModel->numFrames; i++, outFrame++, outVertex += numVerts) {
		inFrame = (dMd2Frame_t *) ((byte *)inModel + LittleLong (inModel->ofsFrames) + i * frameSize);

		outFrame->scale[0] = LittleFloat (inFrame->scale[0]);
		outFrame->scale[1] = LittleFloat (inFrame->scale[1]);
		outFrame->scale[2] = LittleFloat (inFrame->scale[2]);

		outFrame->translate[0] = LittleFloat (inFrame->translate[0]);
		outFrame->translate[1] = LittleFloat (inFrame->translate[1]);
		outFrame->translate[2] = LittleFloat (inFrame->translate[2]);

		// Frame bounds
		VectorCopy (outFrame->translate, outFrame->mins);
		VectorMA (outFrame->translate, 255, outFrame->scale, outFrame->maxs);
		outFrame->radius = RadiusFromBounds (outFrame->mins, outFrame->maxs);

		// Mesh bounds
		VectorCopy (outFrame->mins, outMesh->mins[i]);
		VectorCopy (outFrame->maxs, outMesh->maxs[i]);
		outMesh->radius[i] = outFrame->radius;

		// Model bounds
		model->radius = max (model->radius, outFrame->radius);
		AddPointToBounds (outFrame->mins, model->mins, model->maxs);
		AddPointToBounds (outFrame->maxs, model->mins, model->maxs);

		//
		// Load vertices and normals
		//
		for (j=0 ; j<numIndexes ; j++) {
			outVertex[outIndex[j]].point[0] = (int16)inFrame->verts[tempIndex[indRemap[j]]].v[0];
			outVertex[outIndex[j]].point[1] = (int16)inFrame->verts[tempIndex[indRemap[j]]].v[1];
			outVertex[outIndex[j]].point[2] = (int16)inFrame->verts[tempIndex[indRemap[j]]].v[2];

			ByteToDir (inFrame->verts[tempIndex[indRemap[j]]].normalIndex, normal);
			NormToLatLong (normal, outVertex[outIndex[j]].latLong);
		}
	}

	//
	// Build a list of neighbors
	//
#ifdef SHADOW_VOLUMES
	allocBuffer += sizeof (float) * outModel->numFrames;
	outMesh->neighbors = (int *)allocBuffer;
	R_BuildTriangleNeighbors (outMesh->neighbors, outMesh->indexes, outMesh->numTris);
#endif

	//
	// Register all skins
	//
	allocBuffer += sizeof(int) * outMesh->numTris * 3;
	outSkins = outMesh->skins = (mAliasSkin_t *)allocBuffer;

	for (i=0 ; i<outMesh->numSkins ; i++, outSkins++) {
		if (LittleLong (inModel->ofsSkins) == -1)
			continue;

		temp = (char *)inModel + LittleLong (inModel->ofsSkins) + i*MD2_MAX_SKINNAME;
		if (!temp || !temp[0])
			continue;

		Q_strncpyz (outSkins->name, temp, sizeof (outSkins->name));
		outSkins->skin = R_RegisterSkin (outSkins->name);

		if (!outSkins->skin) {
			Com_DevPrintf (PRNT_WARNING, "R_LoadMD2Model: '%s' could not load skin '%s'\n",
							model->name, outSkins->name);
		}
	}
}

/*
===============================================================================

	MD3 LOADING

===============================================================================
*/

/*
=================
R_StripModelLODSuffix
=================
*/
void R_StripModelLODSuffix (char *name)
{
	int		len;
	int		lodNum;

	len = strlen (name);
	if (len <= 2)
		return;

	lodNum = atoi (&name[len - 1]);
	if (lodNum < ALIAS_MAX_LODS) {
		if (name[len-2] == '_') {
			name[len-2] = '\0';
		}
	}
}


/*
=================
R_LoadMD3Model
=================
*/
static void R_LoadMD3Model (model_t *model, byte *buffer)
{
	dMd3Coord_t			*inCoord;
	dMd3Frame_t			*inFrame;
	index_t				*inIndex;
	dMd3Header_t		*inModel;
	dMd3Mesh_t			*inMesh;
	dMd3Skin_t			*inSkin;
	dMd3Tag_t			*inTag;
	dMd3Vertex_t		*inVert;
	vec2_t				*outCoord;
	mAliasFrame_t		*outFrame;
	index_t				*outIndex;
	mAliasMesh_t		*outMesh;
	mAliasModel_t		*outModel;
	mAliasSkin_t		*outSkin;
	mAliasTag_t			*outTag;
	mAliasVertex_t		*outVert;
	int					i, j, l;
	int					version;
	byte				*allocBuffer;

	inModel = (dMd3Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD3_MODEL_VERSION)
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has wrong version number (%i != %i)",
							model->name, version, MD3_MODEL_VERSION);

	model->aliasModel = outModel = R_ModAlloc (model, sizeof (mAliasModel_t));
	model->type = MODEL_MD3;

	//
	// Byte swap the header fields and sanity check
	//
	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames <= 0 || outModel->numFrames > MD3_MAX_FRAMES)
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	outModel->numTags = LittleLong (inModel->numTags);
	if (outModel->numTags < 0 || outModel->numTags > MD3_MAX_TAGS)
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has an invalid amount of tags '%d'",
							model->name, outModel->numTags);

	outModel->numMeshes = LittleLong (inModel->numMeshes);
	if (outModel->numMeshes < 0 || outModel->numMeshes > MD3_MAX_MESHES)
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has an invalid amount of meshes '%d'",
							model->name, outModel->numMeshes);

	if (!outModel->numMeshes && !outModel->numTags)
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has no meshes and no tags!",
							model->name);

	// Allocate as much as possible now
	allocBuffer = R_ModAlloc (model, (sizeof (mAliasFrame_t) * outModel->numFrames)
									+ (sizeof (mAliasTag_t) * outModel->numFrames * outModel->numTags)
									+ (sizeof (mAliasMesh_t) * outModel->numMeshes));

	//
	// Load the frames
	//
	inFrame = (dMd3Frame_t *)((byte *)inModel + LittleLong (inModel->ofsFrames));
	outFrame = outModel->frames = (mAliasFrame_t *)allocBuffer;

	for (i=0 ; i<outModel->numFrames ; i++, inFrame++, outFrame++) {
		outFrame->scale[0] = MD3_XYZ_SCALE;
		outFrame->scale[1] = MD3_XYZ_SCALE;
		outFrame->scale[2] = MD3_XYZ_SCALE;

		outFrame->translate[0] = LittleFloat (inFrame->translate[0]);
		outFrame->translate[1] = LittleFloat (inFrame->translate[1]);
		outFrame->translate[2] = LittleFloat (inFrame->translate[2]);

		// Never trust the modeler utility and recalculate bbox and radius
		ClearBounds (outFrame->mins, outFrame->maxs);
	}

	//
	// Load the tags
	//
	allocBuffer += sizeof (mAliasFrame_t) * outModel->numFrames;
	inTag = (dMd3Tag_t *)((byte *)inModel + LittleLong (inModel->ofsTags));
	outTag = outModel->tags = (mAliasTag_t *)allocBuffer;

	for (i=0 ; i<outModel->numFrames ; i++) {
		for (l=0 ; l<outModel->numTags ; l++, inTag++, outTag++) {
			for (j=0 ; j<3 ; j++) {
				mat3x3_t	axis;

				axis[0][j] = LittleFloat (inTag->axis[0][j]);
				axis[1][j] = LittleFloat (inTag->axis[1][j]);
				axis[2][j] = LittleFloat (inTag->axis[2][j]);
				Matrix3_Quat (axis, outTag->quat);
				Quat_Normalize (outTag->quat);
				outTag->origin[j] = LittleFloat (inTag->origin[j]);
			}

			Q_strncpyz (outTag->name, inTag->tagName, sizeof (outTag->name));
		}
	}

	//
	// Load the meshes
	//
	allocBuffer += sizeof (mAliasTag_t) * outModel->numFrames * outModel->numTags;
	inMesh = (dMd3Mesh_t *)((byte *)inModel + LittleLong (inModel->ofsMeshes));
	outMesh = outModel->meshes = (mAliasMesh_t *)allocBuffer;

	for (i=0 ; i<outModel->numMeshes ; i++, outMesh++) {
		Q_strncpyz (outMesh->name, inMesh->meshName, sizeof (outMesh->name));
		if (strncmp ((const char *)inMesh->ident, MD3_HEADERSTR, 4))
			Com_Error (ERR_DROP, "R_LoadMD3Model: mesh '%s' in model '%s' has wrong id (%i != %i)",
								inMesh->meshName, model->name, LittleLong ((int)inMesh->ident), MD3_HEADER);

		R_StripModelLODSuffix (outMesh->name);

		outMesh->numSkins = LittleLong (inMesh->numSkins);
		if (outMesh->numSkins <= 0 || outMesh->numSkins > MD3_MAX_SHADERS)
			Com_Error (ERR_DROP, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of skins '%d'",
								outMesh->name, model->name, outMesh->numSkins);

		outMesh->numTris = LittleLong (inMesh->numTris);
		if (outMesh->numTris <= 0 || outMesh->numTris > MD3_MAX_TRIANGLES)
			Com_Error (ERR_DROP, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of triangles '%d'",
								outMesh->name, model->name, outMesh->numTris);

		outMesh->numVerts = LittleLong (inMesh->numVerts);
		if (outMesh->numVerts <= 0 || outMesh->numVerts > MD3_MAX_VERTS)
			Com_Error (ERR_DROP, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of vertices '%d'",
								outMesh->name, model->name, outMesh->numVerts);

		if (outMesh->numVerts >= ALIAS_MAX_VERTS)
			Com_Error (ERR_DROP, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount verts for an alias model '%d' >= ALIAS_MAX_VERTS",
								outMesh->name, outMesh->numVerts, ALIAS_MAX_VERTS);

		// Allocate as much as possible now
#ifdef SHADOW_VOLUMES
		allocBuffer = R_ModAlloc (model, (sizeof (mAliasSkin_t) * outMesh->numSkins)
										+ (sizeof (index_t) * outMesh->numTris * 3)
										+ (sizeof (vec2_t) * outMesh->numVerts)
										+ (sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts)
										+ (sizeof (vec3_t) * outModel->numFrames * 2)
										+ (sizeof (float) * outModel->numFrames)
										+ (sizeof (int) * outMesh->numTris * 3)
										);
#else
		allocBuffer = R_ModAlloc (model, (sizeof (mAliasSkin_t) * outMesh->numSkins)
										+ (sizeof (index_t) * outMesh->numTris * 3)
										+ (sizeof (vec2_t) * outMesh->numVerts)
										+ (sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts)
										+ (sizeof (vec3_t) * outModel->numFrames * 2)
										+ (sizeof (float) * outModel->numFrames)
										);
#endif

		//
		// Load the skins
		//
		inSkin = (dMd3Skin_t *)((byte *)inMesh + LittleLong (inMesh->ofsSkins));
		outSkin = outMesh->skins = (mAliasSkin_t *)allocBuffer;

		for (j=0 ; j<outMesh->numSkins ; j++, inSkin++, outSkin++) {
			if (!inSkin->name || !inSkin->name[0])
				continue;

			Q_strncpyz (outSkin->name, inSkin->name, sizeof (outSkin->name));
			outSkin->skin = R_RegisterSkin (outSkin->name);

			if (!outSkin->skin)
				Com_DevPrintf (PRNT_WARNING, "R_LoadMD3Model: '%s' could not load skin '%s' on mesh '%s'\n",
								model->name, outSkin->name, outMesh->name);
		}

		//
		// Load the indexes
		//
		allocBuffer += sizeof (mAliasSkin_t) * outMesh->numSkins;
		inIndex = (index_t *)((byte *)inMesh + LittleLong (inMesh->ofsIndexes));
		outIndex = outMesh->indexes = (index_t *)allocBuffer;

		for (j=0 ; j<outMesh->numTris ; j++, inIndex += 3, outIndex += 3) {
			outIndex[0] = (index_t)LittleLong (inIndex[0]);
			outIndex[1] = (index_t)LittleLong (inIndex[1]);
			outIndex[2] = (index_t)LittleLong (inIndex[2]);
		}

		//
		// Load the texture coordinates
		//
		allocBuffer += sizeof (index_t) * outMesh->numTris * 3;
		inCoord = (dMd3Coord_t *)((byte *)inMesh + LittleLong (inMesh->ofsTCs));
		outCoord = outMesh->coords = (vec2_t *)allocBuffer;

		for (j=0 ; j<outMesh->numVerts ; j++, inCoord++) {
			outCoord[j][0] = LittleFloat (inCoord->st[0]);
			outCoord[j][1] = LittleFloat (inCoord->st[1]);
		}

		//
		// Load the vertexes and normals
		// Apply vertexes to mesh/model per-frame bounds/radius
		//
		allocBuffer += sizeof (vec2_t) * outMesh->numVerts;
		inVert = (dMd3Vertex_t *)((byte *)inMesh + LittleLong (inMesh->ofsVerts));
		outVert = outMesh->vertexes = (mAliasVertex_t *)allocBuffer;
		outFrame = outModel->frames;

		allocBuffer += sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts;
		outMesh->mins = (vec3_t *)allocBuffer;
		allocBuffer += sizeof (vec3_t) * outModel->numFrames;
		outMesh->maxs = (vec3_t *)allocBuffer;
		allocBuffer += sizeof (vec3_t) * outModel->numFrames;
		outMesh->radius = (float *)allocBuffer;

		for (l=0 ; l<outModel->numFrames ; l++, outFrame++) {
			vec3_t	v;

			ClearBounds (outMesh->mins[l], outMesh->maxs[l]);

			for (j=0 ; j<outMesh->numVerts ; j++, inVert++, outVert++) {
				// Vertex
				outVert->point[0] = LittleShort (inVert->point[0]);
				outVert->point[1] = LittleShort (inVert->point[1]);
				outVert->point[2] = LittleShort (inVert->point[2]);

				// Add vertex to bounds
				VectorCopy (outVert->point, v);
				AddPointToBounds (v, outFrame->mins, outFrame->maxs);
				AddPointToBounds (v, outMesh->mins[l], outMesh->maxs[l]);

				// Normal
				outVert->latLong[0] = inVert->norm[0] & 0xff;
				outVert->latLong[1] = inVert->norm[1] & 0xff;
			}

			outMesh->radius[l] = RadiusFromBounds (outMesh->mins[l], outMesh->maxs[l]);
		}

		//
		// Build a list of neighbors
		//
#ifdef SHADOW_VOLUMES
		allocBuffer += sizeof (float) * outModel->numFrames;
		outMesh->neighbors = (int *)allocBuffer;
		R_BuildTriangleNeighbors (outMesh->neighbors, outMesh->indexes, outMesh->numTris);
#endif // SHADOW_VOLUMES

		// End of loop
		inMesh = (dMd3Mesh_t *)((byte *)inMesh + LittleLong (inMesh->meshSize));
	}

	//
	// Calculate model bounds
	//
	outFrame = outModel->frames;
	for (i=0 ; i<outModel->numFrames ; i++, outFrame++) {
		VectorMA (outFrame->translate, MD3_XYZ_SCALE, outFrame->mins, outFrame->mins);
		VectorMA (outFrame->translate, MD3_XYZ_SCALE, outFrame->maxs, outFrame->maxs);
		outFrame->radius = RadiusFromBounds (outFrame->mins, outFrame->maxs);

		AddPointToBounds (outFrame->mins, model->mins, model->maxs);
		AddPointToBounds (outFrame->maxs, model->mins, model->maxs);
		model->radius = max (model->radius, outFrame->radius);
	}
}

/*
===============================================================================

	SP2 LOADING

===============================================================================
*/

/*
=================
R_LoadSP2Model
=================
*/
static void R_LoadSP2Model (model_t *model, byte *buffer)
{
	dSpriteHeader_t	*inModel;
	dSpriteFrame_t	*inFrames;
	mSpriteModel_t	*outModel;
	mSpriteFrame_t	*outFrames;
	int				i, version;
	int				numFrames;
	byte			*allocBuffer;

	inModel = (dSpriteHeader_t *)buffer;

	//
	// Sanity checks
	//
	version = LittleLong (inModel->version);
	if (version != SP2_VERSION)
		Com_Error (ERR_DROP, "%s has wrong version number (%i should be %i)", model->name, version, SP2_VERSION);

	numFrames = LittleLong (inModel->numFrames);
	if (numFrames > SP2_MAX_FRAMES)
		Com_Error (ERR_DROP, "R_LoadSP2Model: model '%s' has too many frames (%i > %i)", model->name, numFrames, SP2_MAX_FRAMES);

	//
	// Allocate
	//
	allocBuffer = R_ModAlloc (model, sizeof (mSpriteModel_t) + (sizeof (mSpriteFrame_t) * numFrames));

	model->type = MODEL_SP2;
	model->spriteModel = outModel = (mSpriteModel_t *)allocBuffer;
	outModel->numFrames = numFrames;

	//
	// Byte swap
	//
	allocBuffer += sizeof (mSpriteModel_t);
	outModel->frames = outFrames = (mSpriteFrame_t *)allocBuffer;
	inFrames = inModel->frames;

	for (i=0 ; i<outModel->numFrames ; i++, inFrames++, outFrames++) {
		outFrames->width	= LittleLong (inFrames->width);
		outFrames->height	= LittleLong (inFrames->height);
		outFrames->originX	= LittleLong (inFrames->originX);
		outFrames->originY	= LittleLong (inFrames->originY);

		// For culling
		outFrames->radius	= (float)sqrt ((outFrames->width*outFrames->width) + (outFrames->height*outFrames->height));
		model->radius		= max (model->radius, outFrames->radius);

		// Register the shader
		Q_strncpyz (outFrames->name, inFrames->name, sizeof (outFrames->name));
		outFrames->skin = R_RegisterPoly (outFrames->name);

		if (!outFrames->skin)
			Com_DevPrintf (PRNT_WARNING, "R_LoadSP2Model: '%s' could not load skin '%s'\n",
							model->name, outFrames->name);
	}
}

/*
===============================================================================

	QUAKE2 BRUSH MODELS

===============================================================================
*/

static void BoundQ2BSPPoly (int numVerts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;

	for (i=0 ; i<numVerts ; i++) {
		for (j=0 ; j<3 ; j++, v++) {
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
	}
}


/*
================
R_GetImageTCSize

This is just a duplicate of R_GetImageSize modified to get the texcoord size for Q2BSP surfaces
================
*/
static void R_GetImageTCSize (shader_t *shader, int *tcWidth, int *tcHeight)
{
	shaderPass_t	*pass;
	image_t			*image;
	int				i;
	int				passNum;

	if (!shader || !shader->numPasses) {
		if (tcWidth)
			*tcWidth = 64;
		if (tcHeight)
			*tcHeight = 64;
		return;
	}

	image = NULL;
	passNum = 0;
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		if (passNum++ != shader->sizeBase)
			continue;

		image = pass->animFrames[0];
		break;
	}

	if (!image)
		return;

	if (tcWidth)
		*tcWidth = image->tcWidth;
	if (tcHeight)
		*tcHeight = image->tcHeight;
}


/*
================
R_SubdivideQ2BSPSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdivideQ2Polygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
{
	int				i, j;
	vec3_t			mins, maxs;
	float			m;
	float			*v;
	vec3_t			front[64], back[64];
	int				f, b;
	float			dist[64];
	float			frac;
	mQ2BspPoly_t	*poly;
	vec3_t			posTotal;
	vec3_t			normalTotal;
	vec2_t			coordTotal;
	float			oneDivVerts;
	byte			*buffer;
	int				tcWidth, tcHeight;

	if (numVerts > 60)
		Com_Error (ERR_DROP, "SubdivideQ2Polygon: numVerts = %i", numVerts);

	BoundQ2BSPPoly (numVerts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5f;
		m = subdivideSize * (float)floor (m/subdivideSize + 0.5f);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// Cut it
		v = verts + i;
		for (j=0 ; j<numVerts ; j++, v+=3)
			dist[j] = *v - m;

		// Wrap cases
		dist[j] = dist[0];
		v -= i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numVerts ; j++, v+=3) {
			if (dist[j] >= 0) {
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0) {
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ((dist[j] > 0) != (dist[j+1] > 0)) {
				// Clip point
				frac = dist[j] / (dist[j] - dist[j+1]);

				front[f][0] = back[b][0] = v[0] + frac*(v[3+0] - v[0]);
				front[f][1] = back[b][1] = v[1] + frac*(v[3+1] - v[1]);
				front[f][2] = back[b][2] = v[2] + frac*(v[3+2] - v[2]);

				f++;
				b++;
			}
		}

		SubdivideQ2Polygon (model, surf, f, front[0], subdivideSize);
		SubdivideQ2Polygon (model, surf, b, back[0], subdivideSize);
		return;
	}

	// Get dimensions
	R_GetImageTCSize (surf->q2_texInfo->shader, &tcWidth, &tcHeight);

	// Add a point in the center to help keep warp valid
	buffer = R_ModAllocZero (model, sizeof (mQ2BspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t)));
	poly = (mQ2BspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.lmCoordArray = NULL;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mQ2BspPoly_t);
	poly->mesh.colorArray = (bvec4_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (bvec4_t);
	poly->mesh.vertexArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.normalsArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.coordArray = (vec2_t *)buffer;

	VectorClear (posTotal);
	VectorClear (normalTotal);
	Vector2Clear (coordTotal);

	for (i=0 ; i<numVerts ; i++) {
		// Colors
		Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);

		// Position
		VectorCopy (verts, poly->mesh.vertexArray[i+1]);

		// Normal
		VectorCopy (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coords
		poly->mesh.coordArray[i+1][0] = DotProduct (verts, surf->q2_texInfo->vecs[0]) * (1.0f/64.0f);
		poly->mesh.coordArray[i+1][1] = DotProduct (verts, surf->q2_texInfo->vecs[1]) * (1.0f/64.0f);

		// For the center point
		VectorAdd (posTotal, verts, posTotal);
		VectorAdd (normalTotal, surf->q2_plane->normal, normalTotal);
		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];

		verts += 3;
	}

	// Center
	oneDivVerts = (1.0f/(float)numVerts);
	Vector4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	VectorScale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	VectorScale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	VectorNormalizef (poly->mesh.normalsArray[0], poly->mesh.normalsArray[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);

	// Copy first vertex to last
	Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	memcpy (poly->mesh.vertexArray[i+1], poly->mesh.vertexArray[1], sizeof (poly->mesh.vertexArray[i+1]));
	memcpy (poly->mesh.normalsArray[i+1], poly->mesh.normalsArray[1], sizeof (poly->mesh.normalsArray[i+1]));
	memcpy (poly->mesh.coordArray[i+1], poly->mesh.coordArray[1], sizeof (poly->mesh.coordArray[i+1]));

	// Link it in
	poly->next = surf->q2_polys;
	surf->q2_polys = poly;
}
static void R_SubdivideQ2BSPSurface (model_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	index_t		index;
	float		*vec;

	// Convert edges back to a normal polygon
	numVerts = 0;
	for (i=0 ; i<surf->q2_numEdges ; i++) {
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];

		if (index > 0)
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[index].v[0]].position;
		else
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[-index].v[1]].position;
		VectorCopy (vec, verts[numVerts]);
		numVerts++;
	}

	SubdivideQ2Polygon (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_SubdivideLightmapQ2BSPSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdivideLightmappedQ2Polygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
{
	int				i, j;
	vec3_t			mins, maxs;
	float			m;
	float			*v;
	vec3_t			front[64], back[64];
	int				f, b;
	float			dist[64];
	float			frac;
	mQ2BspPoly_t	*poly;
	float			s, t;
	vec3_t			posTotal;
	vec3_t			normalTotal;
	vec2_t			coordTotal;
	vec2_t			lmCoordTotal;
	float			oneDivVerts;
	byte			*buffer;
	int				tcWidth, tcHeight;

	if (numVerts > 60)
		Com_Error (ERR_FATAL, "SubdivideLightmappedQ2Polygon: numVerts = %i", numVerts);

	BoundQ2BSPPoly (numVerts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5f;
		m = subdivideSize * (float)floor (m/subdivideSize + 0.5f);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// Cut it
		v = verts + i;
		for (j=0 ; j<numVerts ; j++, v+= 3)
			dist[j] = *v - m;

		// Wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numVerts ; j++, v+= 3) {
			if (dist[j] >= 0) {
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0) {
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ((dist[j] > 0) != (dist[j+1] > 0)) {
				// Clip point
				frac = dist[j] / (dist[j] - dist[j+1]);

				front[f][0] = back[b][0] = v[0] + frac*(v[3+0] - v[0]);
				front[f][1] = back[b][1] = v[1] + frac*(v[3+1] - v[1]);
				front[f][2] = back[b][2] = v[2] + frac*(v[3+2] - v[2]);

				f++;
				b++;
			}
		}

		SubdivideLightmappedQ2Polygon (model, surf, f, front[0], subdivideSize);
		SubdivideLightmappedQ2Polygon (model, surf, b, back[0], subdivideSize);
		return;
	}

	// Get dimensions
	R_GetImageTCSize (surf->q2_texInfo->shader, &tcWidth, &tcHeight);

	// Add a point in the center to help keep warp valid
	buffer = R_ModAllocZero (model, sizeof (mQ2BspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t) * 2));
	poly = (mQ2BspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mQ2BspPoly_t);
	poly->mesh.colorArray = (bvec4_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (bvec4_t);
	poly->mesh.vertexArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.normalsArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.coordArray = (vec2_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec2_t);
	poly->mesh.lmCoordArray = (vec2_t *)buffer;

	VectorClear (posTotal);
	VectorClear (normalTotal);
	Vector2Clear (coordTotal);
	Vector2Clear (lmCoordTotal);

	for (i=0 ; i<numVerts ; i++) {
		// Colors
		Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);

		// Position
		VectorCopy (verts, poly->mesh.vertexArray[i+1]);

		// Normals
		VectorCopy (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coordinates
		poly->mesh.coordArray[i+1][0] = (DotProduct (verts, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3]) / tcWidth;
		poly->mesh.coordArray[i+1][1] = (DotProduct (verts, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3]) / tcHeight;

		// Lightmap texture coordinates
		s = DotProduct (verts, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3] - surf->q2_textureMins[0];
		poly->mesh.lmCoordArray[i+1][0] = (s + 8 + (surf->q2_lmCoords[0] * 16)) / (r_q2lmSize * 16);

		t = DotProduct (verts, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3] - surf->q2_textureMins[1];
		poly->mesh.lmCoordArray[i+1][1] = (t + 8 + (surf->q2_lmCoords[1] * 16)) / (r_q2lmSize * 16);

		// For the center point
		VectorAdd (posTotal, verts, posTotal);
		VectorAdd (normalTotal, surf->q2_plane->normal, normalTotal);

		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];

		lmCoordTotal[0] += poly->mesh.lmCoordArray[i+1][0];
		lmCoordTotal[1] += poly->mesh.lmCoordArray[i+1][1];

		verts += 3;
	}

	// Center point
	oneDivVerts = (1.0f/(float)numVerts);
	Vector4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	VectorScale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	VectorScale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	VectorNormalizef (poly->mesh.normalsArray[0], poly->mesh.normalsArray[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);
	Vector2Scale (lmCoordTotal, oneDivVerts, poly->mesh.lmCoordArray[0]);

	// Copy first vertex to last
	Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	memcpy (poly->mesh.vertexArray[i+1], poly->mesh.vertexArray[1], sizeof (poly->mesh.vertexArray[i+1]));
	memcpy (poly->mesh.normalsArray[i+1], poly->mesh.normalsArray[1], sizeof (poly->mesh.normalsArray[i+1]));
	memcpy (poly->mesh.coordArray[i+1], poly->mesh.coordArray[1], sizeof (poly->mesh.coordArray[i+1]));
	memcpy (poly->mesh.lmCoordArray[i+1], poly->mesh.lmCoordArray[1], sizeof (poly->mesh.lmCoordArray[i+1]));

	// Link it in
	poly->next = surf->q2_polys;
	surf->q2_polys = poly;
}

static void R_SubdivideLightmapQ2BSPSurface (model_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	int			index;
	float		*vec;

	// Convert edges back to a normal polygon
	numVerts = 0;
	for (i=0 ; i<surf->q2_numEdges ; i++) {
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];

		if (index > 0)
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[index].v[0]].position;
		else
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[-index].v[1]].position;
		VectorCopy (vec, verts[numVerts]);
		numVerts++;
	}

	SubdivideLightmappedQ2Polygon (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_BuildPolygonFromQ2BSPSurface
================
*/
static void R_BuildPolygonFromQ2BSPSurface (model_t *model, mBspSurface_t *surf)
{
	int				i, numVerts;
	mQ2BspEdge_t	*pedges, *r_pedge;
	float			*vec, s, t;
	mQ2BspPoly_t	*poly;
	index_t			index;
	byte			*buffer;
	int				tcWidth, tcHeight;

	// Get dimensions
	R_GetImageTCSize (surf->q2_texInfo->shader, &tcWidth, &tcHeight);

	pedges = model->q2BspModel.edges;
	numVerts = surf->q2_numEdges;

	// Allocate
	buffer = R_ModAllocZero (model, sizeof (mQ2BspPoly_t) + (numVerts * sizeof (bvec4_t)) + (numVerts * sizeof (vec3_t) * 2) + (numVerts * sizeof (vec2_t) * 2));
	poly = (mQ2BspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts;
	poly->mesh.numIndexes = (numVerts-2)*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mQ2BspPoly_t);
	poly->mesh.colorArray = (bvec4_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (bvec4_t);
	poly->mesh.vertexArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.normalsArray = (vec3_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec3_t);
	poly->mesh.coordArray = (vec2_t *)buffer;

	buffer += poly->mesh.numVerts * sizeof (vec2_t);
	poly->mesh.lmCoordArray = (vec2_t *)buffer;

	for (i=0 ; i<numVerts ; i++) {
		// Color
		Vector4Set (poly->mesh.colorArray[i], 255, 255, 255, 255);

		// Position
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];

		if (index > 0) {
			r_pedge = &pedges[index];
			vec = model->q2BspModel.vertexes[r_pedge->v[0]].position;
		}
		else {
			r_pedge = &pedges[-index];
			vec = model->q2BspModel.vertexes[r_pedge->v[1]].position;
		}

		VectorCopy (vec, poly->mesh.vertexArray[i]);

		// Normal
		VectorCopy (surf->q2_plane->normal, poly->mesh.normalsArray[i]);

		// Texture coordinates
		poly->mesh.coordArray[i][0] = (DotProduct (vec, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3]) / tcWidth;
		poly->mesh.coordArray[i][1] = (DotProduct (vec, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3]) / tcHeight;

		// Lightmap texture coordinates
		s = DotProduct (vec, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3] - surf->q2_textureMins[0];
		poly->mesh.lmCoordArray[i][0] = (s + 8 + (surf->q2_lmCoords[0] * 16)) / (r_q2lmSize * 16);

		t = DotProduct (vec, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3] - surf->q2_textureMins[1];
		poly->mesh.lmCoordArray[i][1] = (t + 8 + (surf->q2_lmCoords[1] * 16)) / (r_q2lmSize * 16);
	}

	// Link it in
	poly->next = surf->q2_polys;
	surf->q2_polys = poly;
}

// ============================================================================

/*
==============
R_Q2BSPClusterPVS
==============
*/
byte *R_Q2BSPClusterPVS (int cluster, model_t *model)
{
	static byte	decompressed[Q2BSP_MAX_VIS];
	int			c, row;
	byte		*in;
	byte		*out;

	if (cluster == -1 || !model->q2BspModel.vis)
		return r_q2BspNoVis;

	row = (model->q2BspModel.vis->numClusters+7)>>3;
	in = (byte *)model->q2BspModel.vis + model->q2BspModel.vis->bitOfs[cluster][Q2BSP_VIS_PVS];
	out = decompressed;

	if (!in) {
		// no vis info, so make all visible
		while (row) {
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do {
		if (*in) {
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;

		while (c) {
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}


/*
===============
R_PointInQ2BSPLeaf
===============
*/
mBspLeaf_t *R_PointInQ2BSPLeaf (vec3_t point, model_t *model)
{
	cBspPlane_t	*plane;
	mBspNode_t	*node;
	float		d;
	
	if (!model || !model->bspModel.nodes)
		Com_Error (ERR_DROP, "R_PointInQ2BSPLeaf: bad model");

	node = model->bspModel.nodes;
	for ( ; ; ) {
		if (node->q2_contents != -1)
			return (mBspLeaf_t *)node;

		plane = node->plane;
		d = DotProduct (point, plane->normal) - plane->dist;
		node = (d > 0) ? node->children[0] : node->children[1];
	}
}

/*
===============================================================================

	QUAKE2 BRUSH MODEL LOADING

===============================================================================
*/

/*
================
R_CalcQ2BSPSurfaceBounds

Fills in s->mins and s->maxs
================
*/
static void R_CalcQ2BSPSurfaceBounds (model_t *model, mBspSurface_t *surf)
{
	mQ2BspVertex_t	*v;
	int				i, e;

	ClearBounds (surf->mins, surf->maxs);

	for (i=0 ; i<surf->q2_numEdges ; i++) {
		e = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];
		if (e >= 0)
			v = &model->q2BspModel.vertexes[model->q2BspModel.edges[e].v[0]];
		else
			v = &model->q2BspModel.vertexes[model->q2BspModel.edges[-e].v[1]];

		AddPointToBounds (v->position, surf->mins, surf->maxs);
	}
}


/*
================
R_CalcQ2BSPSurfaceExtents

Fills in s->textureMins[] and s->extents[]
================
*/
static void R_CalcQ2BSPSurfaceExtents (model_t *model, mBspSurface_t *surf)
{
	float			mins[2], maxs[2], val;
	int				i, j, e;
	mQ2BspVertex_t	*v;
	mQ2BspTexInfo_t	*tex;
	int				bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = surf->q2_texInfo;
	
	for (i=0 ; i<surf->q2_numEdges ; i++) {
		e = model->q2BspModel.surfEdges[surf->q2_firstEdge+i];
		v = (e >= 0) ? &model->q2BspModel.vertexes[model->q2BspModel.edges[e].v[0]] : &model->q2BspModel.vertexes[model->q2BspModel.edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++) {
			val = v->position[0] * tex->vecs[j][0] +  v->position[1] * tex->vecs[j][1] + v->position[2] * tex->vecs[j][2] + tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++) {	
		bmins[i] = (int)floor (mins[i]/16);
		bmaxs[i] = (int)ceil (maxs[i]/16);

		surf->q2_textureMins[i] = bmins[i] * 16;
		surf->q2_extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


/*
=================
R_SetParentQ2BSPNode
=================
*/
static void R_SetParentQ2BSPNode (mBspNode_t *node, mBspNode_t *parent)
{
	node->parent = parent;
	if (node->q2_contents != -1)
		return;

	R_SetParentQ2BSPNode (node->children[0], node);
	R_SetParentQ2BSPNode (node->children[1], node);
}

// ============================================================================

/*
=================
R_LoadQ2BSPVertexes
=================
*/
static void R_LoadQ2BSPVertexes (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspVertex_t	*in;
	mQ2BspVertex_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPVertexes: funny lump size in %s", model->name);

	model->q2BspModel.numVertexes = lump->fileLen / sizeof (*in);
	model->q2BspModel.vertexes = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numVertexes);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numVertexes ; i++, in++, out++) {
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}


/*
=================
R_LoadQ2BSPEdges
=================
*/
static void R_LoadQ2BSPEdges (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspEdge_t	*in;
	mQ2BspEdge_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPEdges: funny lump size in %s", model->name);

	model->q2BspModel.numEdges = lump->fileLen / sizeof (*in);
	model->q2BspModel.edges = out = R_ModAllocZero (model, sizeof (*out) * (model->q2BspModel.numEdges + 1));

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numEdges ; i++, in++, out++) {
		out->v[0] = (uint16) LittleShort (in->v[0]);
		out->v[1] = (uint16) LittleShort (in->v[1]);
	}
}


/*
=================
R_LoadQ2BSPSurfEdges
=================
*/
static void R_LoadQ2BSPSurfEdges (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{	
	int		*in;
	int		*out;
	int		i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPSurfEdges: funny lump size in %s", model->name);

	model->q2BspModel.numSurfEdges = lump->fileLen / sizeof (*in);
	if (model->q2BspModel.numSurfEdges < 1 || model->q2BspModel.numSurfEdges >= Q2BSP_MAX_SURFEDGES)
		Com_Error (ERR_DROP, "R_LoadQ2BSPSurfEdges: invalid surfEdges count in %s: %i (min: 1; max: %d)", model->name, model->q2BspModel.numSurfEdges, Q2BSP_MAX_SURFEDGES);

	model->q2BspModel.surfEdges = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numSurfEdges);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numSurfEdges ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
R_LoadQ2BSPLighting
=================
*/
static void R_LoadQ2BSPLighting (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	if (!lump->fileLen) {
		model->q2BspModel.lightData = NULL;
		return;
	}

	model->q2BspModel.lightData = R_ModAllocZero (model, lump->fileLen);	
	memcpy (model->q2BspModel.lightData, byteBase + lump->fileOfs, lump->fileLen);
}


/*
=================
R_LoadQ2BSPPlanes
=================
*/
static void R_LoadQ2BSPPlanes (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ2BspPlane_t	*in;
	int				bits;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPPlanes: funny lump size in %s", model->name);

	model->bspModel.numPlanes = lump->fileLen / sizeof (*in);
	model->bspModel.planes = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numPlanes * 2);

	//
	// Byte swap
	//
	for (i=0 ; i<model->bspModel.numPlanes ; i++, in++, out++) {
		bits = 0;
		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signBits = bits;
	}
}


/*
=================
R_LoadQ2BSPTexInfo
=================
*/
static void R_LoadQ2BSPTexInfo (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspTexInfo_t	*in;
	mQ2BspTexInfo_t	*out, *step;
	int				i, next;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPTexInfo: funny lump size in %s", model->name);

	model->q2BspModel.numTexInfo = lump->fileLen / sizeof (*in);
	model->q2BspModel.texInfo = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numTexInfo);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numTexInfo ; i++, in++, out++) {
		out->vecs[0][0] = LittleFloat (in->vecs[0][0]);
		out->vecs[0][1] = LittleFloat (in->vecs[0][1]);
		out->vecs[0][2] = LittleFloat (in->vecs[0][2]);
		out->vecs[0][3] = LittleFloat (in->vecs[0][3]);
		out->vecs[0][4] = LittleFloat (in->vecs[0][4]);
		out->vecs[0][5] = LittleFloat (in->vecs[0][5]);
		out->vecs[0][6] = LittleFloat (in->vecs[0][6]);
		out->vecs[0][7] = LittleFloat (in->vecs[0][7]);

		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nextTexInfo);
		out->next = (next > 0) ? model->q2BspModel.texInfo + next : NULL;

		//
		// Find surfParams
		//
		out->surfParams = 0;
		if (out->flags & SURF_TEXINFO_TRANS33)
			out->surfParams |= SHADER_SURF_TRANS33;
		if (out->flags & SURF_TEXINFO_TRANS66)
			out->surfParams |= SHADER_SURF_TRANS66;
		if (out->flags & SURF_TEXINFO_WARP)
			out->surfParams |= SHADER_SURF_WARP;
		if (out->flags & SURF_TEXINFO_FLOWING)
			out->surfParams |= SHADER_SURF_FLOWING;
		if (!(out->flags & SURF_TEXINFO_WARP))
			out->surfParams |= SHADER_SURF_LIGHTMAP;

		//
		// Register textures and shaders
		//
		if (out->flags & SURF_TEXINFO_SKY) {
			out->shader = r_noShaderSky;
		}
		else {
			Q_snprintfz (out->texName, sizeof (out->texName), "textures/%s.wal", in->texture);
			out->shader = R_RegisterTexture (out->texName, out->surfParams);
			if (!out->shader) {
				Com_Printf (PRNT_WARNING, "Couldn't load %s\n", out->texName);

				if (out->surfParams & SHADER_SURF_LIGHTMAP)
					out->shader = r_noShaderLightmap;
				else
					out->shader = r_noShader;
			}
		}
	}

	//
	// Count animation frames
	//
	for (i=0 ; i<model->q2BspModel.numTexInfo ; i++) {
		out = &model->q2BspModel.texInfo[i];
		out->numFrames = 1;
		for (step=out->next ; step && step != out ; step=step->next) {
			out->numFrames++;
		}
	}
}


/*
=================
R_LoadQ2BSPFaces
=================
*/
static void R_LoadQ2BSPFaces (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspSurface_t	*in;
	mBspSurface_t	*out;
	int				i, surfNum;
	int				planeNum, side;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPFaces: funny lump size in %s", model->name);

	model->bspModel.numSurfaces = lump->fileLen / sizeof (*in);
	model->bspModel.surfaces = out= R_ModAllocZero (model, sizeof (*out) * model->bspModel.numSurfaces);

	R_Q2BSP_BeginBuildingLightmaps ();

	//
	// Byte swap
	//
	for (surfNum=0 ; surfNum<model->bspModel.numSurfaces ; surfNum++, in++, out++) {
		out->q2_firstEdge = LittleLong (in->firstEdge);
		out->q2_numEdges = LittleShort (in->numEdges);		
		out->q2_flags = 0;
		out->q2_polys = NULL;

		planeNum = LittleShort (in->planeNum);
		side = LittleShort (in->side);
		if (side)
			out->q2_flags |= SURF_PLANEBACK;			

		out->q2_plane = model->bspModel.planes + planeNum;

		i = LittleShort (in->texInfo);
		if (i < 0 || i >= model->q2BspModel.numTexInfo)
			Com_Error (ERR_DROP, "R_LoadQ2BSPFaces: bad texInfo number");
		out->q2_texInfo = model->q2BspModel.texInfo + i;

		R_CalcQ2BSPSurfaceBounds (model, out);
		R_CalcQ2BSPSurfaceExtents (model, out);

		//
		// Lighting info
		//
		out->q2_lmWidth = (out->q2_extents[0]>>4) + 1;
		out->q2_lmHeight = (out->q2_extents[1]>>4) + 1;

		i = LittleLong (in->lightOfs);
		out->q2_lmSamples = (i == -1) ? NULL : model->q2BspModel.lightData + i;

		for (out->q2_numStyles=0 ; out->q2_numStyles<Q2BSP_MAX_LIGHTMAPS && in->styles[out->q2_numStyles] != 255 ; out->q2_numStyles++) {
			out->q2_styles[out->q2_numStyles] = in->styles[out->q2_numStyles];
		}

		//
		// Create lightmaps and polygons
		//
		if (out->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
			if (out->q2_texInfo->flags & SURF_TEXINFO_WARP) {
				out->q2_extents[0] = out->q2_extents[1] = 16384;
				out->q2_textureMins[0] = out->q2_textureMins[1] = -8192;
			}

			// WARP surfaces have no lightmap
			if (out->q2_texInfo->shader && out->q2_texInfo->shader->flags & SHADER_SUBDIVIDE) {
				R_SubdivideQ2BSPSurface (model, out, out->q2_texInfo->shader->subdivide);
			}
			else
				R_BuildPolygonFromQ2BSPSurface (model, out);
		}
		else {
			// The rest do
			R_Q2BSP_CreateSurfaceLightmap (out);

			if (out->q2_texInfo->shader && out->q2_texInfo->shader->flags & SHADER_SUBDIVIDE) {
				R_SubdivideLightmapQ2BSPSurface (model, out, out->q2_texInfo->shader->subdivide);
			}
			else
				R_BuildPolygonFromQ2BSPSurface (model, out);
		}
	}

	R_Q2BSP_EndBuildingLightmaps ();
}


/*
=================
R_LoadQ2BSPMarkSurfaces
=================
*/
static void R_LoadQ2BSPMarkSurfaces (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	mBspSurface_t	**out;
	int				i, j;
	int16			*in;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPMarkSurfaces: funny lump size in %s", model->name);

	model->q2BspModel.numMarkSurfaces = lump->fileLen / sizeof (*in);
	model->q2BspModel.markSurfaces = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numMarkSurfaces);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numMarkSurfaces ; i++) {
		j = LittleShort (in[i]);
		if (j < 0 || j >= model->bspModel.numSurfaces)
			Com_Error (ERR_DROP, "R_LoadQ2BSPMarkSurfaces: bad surface number");
		out[i] = model->bspModel.surfaces + j;
	}
}


/*
=================
R_LoadQ2BSPVisibility
=================
*/
static void R_LoadQ2BSPVisibility (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int		i;

	if (!lump->fileLen) {
		model->q2BspModel.vis = NULL;
		return;
	}

	model->q2BspModel.vis = R_ModAllocZero (model, lump->fileLen);	
	memcpy (model->q2BspModel.vis, byteBase + lump->fileOfs, lump->fileLen);

	model->q2BspModel.vis->numClusters = LittleLong (model->q2BspModel.vis->numClusters);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.vis->numClusters ; i++) {
		model->q2BspModel.vis->bitOfs[i][0] = LittleLong (model->q2BspModel.vis->bitOfs[i][0]);
		model->q2BspModel.vis->bitOfs[i][1] = LittleLong (model->q2BspModel.vis->bitOfs[i][1]);
	}
}


/*
=================
R_LoadQ2BSPLeafs
=================
*/
static void R_LoadQ2BSPLeafs (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspLeaf_t	*in;
	mBspLeaf_t		*out;
	int				i, j;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPLeafs: funny lump size in %s", model->name);

	model->bspModel.numLeafs = lump->fileLen / sizeof (*in);
	model->bspModel.leafs = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numLeafs);

	//
	// Byte swap
	//
	for (i=0 ; i<model->bspModel.numLeafs ; i++, in++, out++) {
		out->mins[0] = LittleShort (in->mins[0]);
		out->mins[1] = LittleShort (in->mins[1]);
		out->mins[2] = LittleShort (in->mins[2]);

		out->maxs[0] = LittleShort (in->maxs[0]);
		out->maxs[1] = LittleShort (in->maxs[1]);
		out->maxs[2] = LittleShort (in->maxs[2]);

		out->q2_contents = LittleLong (in->contents);

		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);

		out->q2_firstMarkSurface = model->q2BspModel.markSurfaces + LittleShort (in->firstLeafFace);
		out->q2_numMarkSurfaces = LittleShort (in->numLeafFaces);

		// Mark poly flags
		if (out->q2_contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			for (j=0 ; j<out->q2_numMarkSurfaces ; j++) {
				out->q2_firstMarkSurface[j]->q2_flags |= SURF_UNDERWATER;

				if (out->q2_contents & CONTENTS_LAVA)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_LAVA;
				if (out->q2_contents & CONTENTS_SLIME)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_SLIME;
				if (out->q2_contents & CONTENTS_WATER)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_WATER;
			}
		}
	}	
}


/*
=================
R_LoadQ2BSPNodes
=================
*/
static void R_LoadQ2BSPNodes (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int				i, p;
	dQ2BspNode_t	*in;
	mBspNode_t		*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPNodes: funny lump size in %s", model->name);

	model->bspModel.numNodes = lump->fileLen / sizeof (*in);
	model->bspModel.nodes = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numNodes);

	//
	// Byte swap
	//
	for (i=0 ; i<model->bspModel.numNodes ; i++, in++, out++) {
		out->mins[0] = LittleShort (in->mins[0]);
		out->mins[1] = LittleShort (in->mins[1]);
		out->mins[2] = LittleShort (in->mins[2]);

		out->maxs[0] = LittleShort (in->maxs[0]);
		out->maxs[1] = LittleShort (in->maxs[1]);
		out->maxs[2] = LittleShort (in->maxs[2]);
	
		out->q2_contents = -1;	// Differentiate from leafs

		out->plane = model->bspModel.planes + LittleLong (in->planeNum);
		out->q2_firstSurface = LittleShort (in->firstFace);
		out->q2_numSurfaces = LittleShort (in->numFaces);

		p = LittleLong (in->children[0]);
		out->children[0] = (p >= 0) ? model->bspModel.nodes + p : (mBspNode_t *)(model->bspModel.leafs + (-1 - p));

		p = LittleLong (in->children[1]);
		out->children[1] = (p >= 0) ? model->bspModel.nodes + p : (mBspNode_t *)(model->bspModel.leafs + (-1 - p));
	}

	//
	// Set the nodes and leafs
	//
	R_SetParentQ2BSPNode (model->bspModel.nodes, NULL);
}


/*
=================
R_LoadQ2BSPSubModels
=================
*/
static void R_LoadQ2BSPSubModels (model_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspModel_t	*in;
	mBspHeader_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ2BSPSubModels: funny lump size in %s", model->name);

	model->bspModel.numSubModels = lump->fileLen / sizeof (*in);
	model->bspModel.subModels = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numSubModels);

	//
	// Byte swap
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++, in++, out++) {
		// Pad the mins / maxs by a pixel
		out->mins[0]	= LittleFloat (in->mins[0]) - 1;
		out->mins[1]	= LittleFloat (in->mins[1]) - 1;
		out->mins[2]	= LittleFloat (in->mins[2]) - 1;

		out->maxs[0]	= LittleFloat (in->maxs[0]) + 1;
		out->maxs[1]	= LittleFloat (in->maxs[1]) + 1;
		out->maxs[2]	= LittleFloat (in->maxs[2]) + 1;

		out->origin[0]	= LittleFloat (in->origin[0]);
		out->origin[1]	= LittleFloat (in->origin[1]);
		out->origin[2]	= LittleFloat (in->origin[2]);

		out->radius		= RadiusFromBounds (out->mins, out->maxs);

		out->headNode	= LittleLong (in->headNode);
		out->firstFace	= LittleLong (in->firstFace);
		out->numFaces	= LittleLong (in->numFaces);
	}
}


/*
=================
R_LoadQ2BSPModel
=================
*/
static void R_LoadQ2BSPModel (model_t *model, byte *buffer)
{
	dQ2BspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	int				version;
	int				i;

	//
	// Load the world model
	//
	model->type = MODEL_Q2BSP;

	header = (dQ2BspHeader_t *)buffer;
	version = LittleLong (header->version);
	if (version != Q2BSP_VERSION)
		Com_Error (ERR_DROP, "R_LoadQ2BSPModel: %s has wrong version number (%i should be %i)", model->name, version, Q2BSP_VERSION);

	//
	// Swap all the lumps
	//
	modBase = (byte *)header;

	for (i=0 ; i<sizeof (dQ2BspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// Load into heap
	//
	R_LoadQ2BSPVertexes		(model, modBase, &header->lumps[Q2BSP_LUMP_VERTEXES]);
	R_LoadQ2BSPEdges		(model, modBase, &header->lumps[Q2BSP_LUMP_EDGES]);
	R_LoadQ2BSPSurfEdges	(model, modBase, &header->lumps[Q2BSP_LUMP_SURFEDGES]);
	R_LoadQ2BSPLighting		(model, modBase, &header->lumps[Q2BSP_LUMP_LIGHTING]);
	R_LoadQ2BSPPlanes		(model, modBase, &header->lumps[Q2BSP_LUMP_PLANES]);
	R_LoadQ2BSPTexInfo		(model, modBase, &header->lumps[Q2BSP_LUMP_TEXINFO]);
	R_LoadQ2BSPFaces		(model, modBase, &header->lumps[Q2BSP_LUMP_FACES]);
	R_LoadQ2BSPMarkSurfaces	(model, modBase, &header->lumps[Q2BSP_LUMP_LEAFFACES]);
	R_LoadQ2BSPVisibility	(model, modBase, &header->lumps[Q2BSP_LUMP_VISIBILITY]);
	R_LoadQ2BSPLeafs		(model, modBase, &header->lumps[Q2BSP_LUMP_LEAFS]);
	R_LoadQ2BSPNodes		(model, modBase, &header->lumps[Q2BSP_LUMP_NODES]);
	R_LoadQ2BSPSubModels	(model, modBase, &header->lumps[Q2BSP_LUMP_MODELS]);

	//
	// Set up the submodels
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++) {
		model_t		*starmodel;

		bm = &model->bspModel.subModels[i];
		starmodel = &r_inlineModels[i];

		*starmodel = *model;

		starmodel->bspModel.firstModelSurface = starmodel->bspModel.surfaces + bm->firstFace;
		starmodel->bspModel.numModelSurfaces = bm->numFaces;
		starmodel->q2BspModel.firstNode = bm->headNode;

		if (starmodel->q2BspModel.firstNode >= model->bspModel.numNodes)
			Com_Error (ERR_DROP, "R_LoadQ2BSPModel: Inline model number '%i' has a bad firstNode (%d >= %d)", i, starmodel->q2BspModel.firstNode, model->bspModel.numNodes);

		VectorCopy (bm->maxs, starmodel->maxs);
		VectorCopy (bm->mins, starmodel->mins);
		starmodel->radius = bm->radius;

		if (i == 0)
			*model = *starmodel;

		starmodel->bspModel.numLeafs = bm->visLeafs;
	}
}

/*
===============================================================================

	QUAKE3 BRUSH MODELS

===============================================================================
*/

/*
=================
R_FogForSphere
=================
*/
mQ3BspFog_t *R_FogForSphere (const vec3_t center, const float radius)
{
	int			i, j;
	mQ3BspFog_t	*fog, *defaultFog;
	cBspPlane_t	*plane;

	if (r_refScene.worldModel->type != MODEL_Q3BSP)
		return NULL;
	if (!r_refScene.worldModel->q3BspModel.numFogs)
		return NULL;

	defaultFog = NULL;
	fog = r_refScene.worldModel->q3BspModel.fogs;
	for (i=0 ; i<r_refScene.worldModel->q3BspModel.numFogs ; i++, fog++) {
		if (!fog->shader || !fog->name[0])
			continue;
		if (!fog->visiblePlane) {
			defaultFog = fog;
			continue;
		}

		plane = fog->planes;
		for (j=0 ; j<fog->numPlanes ; j++, plane++) {
			// If completely in front of face, no intersection
			if (PlaneDiff (center, plane) > radius)
				break;
		}

		if (j == fog->numPlanes)
			return fog;
	}

	return defaultFog;
}


/*
=================
R_Q3BSPClusterPVS
=================
*/
byte *R_Q3BSPClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->q3BspModel.vis)
		return r_q3BspNoVis;

	return ((byte *)model->q3BspModel.vis->data + cluster*model->q3BspModel.vis->rowSize);
}


/*
===============
R_PointInQ3BSPLeaf
===============
*/
mBspLeaf_t *R_PointInQ3BSPLeaf (vec3_t p, model_t *model)
{
	mBspNode_t	*node;
	cBspPlane_t	*plane;
	
	if (!model || !model->bspModel.nodes)
		Com_Error (ERR_DROP, "R_PointInQ3BSPLeaf: bad model");

	node = model->bspModel.nodes;
	do {
		plane = node->plane;
		node = node->children[PlaneDiff (p, plane) < 0];
	} while (node->plane);

	return (mBspLeaf_t *)node;
}


/*
=================
R_SetParentQ3BSPNode
=================
*/
static void R_SetParentQ3BSPNode (mBspNode_t *node, mBspNode_t *parent)
{
	node->parent = parent;
	if (!node->plane)
		return;

	R_SetParentQ3BSPNode (node->children[0], node);
	R_SetParentQ3BSPNode (node->children[1], node);
}


/*
=================
R_Q3BSP_SurfPotentiallyFragmented

Only true if R_Q3BSP_SurfPotentiallyVisible is true
=================
*/
qBool R_Q3BSP_SurfPotentiallyFragmented (mBspSurface_t *surf)
{
	if (surf->q3_shaderRef->flags & (SHREF_NOMARKS|SHREF_NOIMPACT))
		return qFalse;

	switch (surf->q3_faceType) {
	case FACETYPE_PLANAR:
		if (surf->q3_shaderRef->contents & CONTENTS_SOLID)
			return qTrue;
		break;

	case FACETYPE_PATCH:
		return qTrue;
	}

	return qFalse;
}


/*
=============
R_Q3BSP_SurfPotentiallyLit

Only true if R_Q3BSP_SurfPotentiallyVisible is true
=============
*/
qBool R_Q3BSP_SurfPotentiallyLit (mBspSurface_t *surf)
{
	if (surf->q3_shaderRef->flags & (SHREF_SKY|SHREF_NODLIGHT))
		return qFalse;

	if (surf->q3_faceType == FACETYPE_FLARE)
		return qFalse;
	if (surf->q3_shaderRef->shader
	&& surf->q3_shaderRef->shader->flags & (SHADER_FLARE|SHADER_SKY))
		return qFalse;

	return qTrue;
}


/*
=================
R_Q3BSP_SurfPotentiallyVisible
=================
*/
qBool R_Q3BSP_SurfPotentiallyVisible (mBspSurface_t *surf)
{
	if (surf->q3_shaderRef->flags & SHREF_NODRAW)
		return qFalse;

	if (!surf->q3_mesh || RB_InvalidMesh (surf->q3_mesh))
		return qFalse;
	if (!surf->q3_shaderRef->shader)
		return qFalse;
	if (!surf->q3_shaderRef->shader->numPasses)
		return qFalse;

	return qTrue;
}

/*
===============================================================================

	QUAKE3 BRUSH MODEL LOADING

===============================================================================
*/

/*
=================
R_LoadQ3BSPLighting
=================
*/
static void R_LoadQ3BSPLighting (model_t *model, byte *byteBase, const dQ3BspLump_t *lightLump, const dQ3BspLump_t *gridLump)
{
	dQ3BspGridLight_t 	*inGrid;

	// Load lighting
	if (lightLump->fileLen && !r_vertexLighting->integer) {
		if (lightLump->fileLen % Q3LIGHTMAP_SIZE)
			Com_Error (ERR_DROP, "R_LoadQ3BSPLighting: funny lighting lump size in %s", model->name);

		model->q3BspModel.numLightmaps = lightLump->fileLen / Q3LIGHTMAP_SIZE;
		model->q3BspModel.lightmapRects = R_ModAllocZero (model, model->q3BspModel.numLightmaps * sizeof (*model->q3BspModel.lightmapRects));
	}

	// Load the light grid
	if (gridLump->fileLen % sizeof (*inGrid))
		Com_Error (ERR_DROP, "R_LoadQ3BSPLighting: funny lightgrid lump size in %s", model->name);

	inGrid = (void *)(byteBase + gridLump->fileOfs);
	model->q3BspModel.numLightGridElems = gridLump->fileLen / sizeof (*inGrid);
	model->q3BspModel.lightGrid = R_ModAllocZero (model, model->q3BspModel.numLightGridElems * sizeof (*model->q3BspModel.lightGrid));

	memcpy (model->q3BspModel.lightGrid, inGrid, model->q3BspModel.numLightGridElems * sizeof (*model->q3BspModel.lightGrid));
}


/*
=================
R_LoadQ3BSPVisibility
=================
*/
static void R_LoadQ3BSPVisibility (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	if (!lump->fileLen) {
		model->q3BspModel.vis = NULL;
		return;
	}

	model->q3BspModel.vis = R_ModAllocZero (model, lump->fileLen);
	memcpy (model->q3BspModel.vis, byteBase + lump->fileOfs, lump->fileLen);

	model->q3BspModel.vis->numClusters = LittleLong (model->q3BspModel.vis->numClusters);
	model->q3BspModel.vis->rowSize = LittleLong (model->q3BspModel.vis->rowSize);
}


/*
=================
R_LoadQ3BSPVertexes
=================
*/
static void R_LoadQ3BSPVertexes (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, count;
	dQ3BspVertex_t	*in;
	float			*out_verts, *out_normals, *out_texCoords, *out_lmCoords;
	byte			*out_colors, *buffer;
	vec3_t			color, fcolor;
	float			div;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPVertexes: funny lump size in %s", model->name);
	count = lump->fileLen / sizeof(*in);

	buffer = R_ModAllocZero (model, (count * sizeof (vec3_t) * 2)
		+ (count * sizeof (vec2_t) * 2)
		+ (count * sizeof (bvec4_t)));

	model->q3BspModel.numVertexes = count;
	model->q3BspModel.vertexArray = (vec3_t *)buffer; buffer += count * sizeof (vec3_t);
	model->q3BspModel.normalsArray = (vec3_t *)buffer; buffer += count * sizeof (vec3_t);
	model->q3BspModel.coordArray = (vec2_t *)buffer; buffer += count * sizeof (vec2_t);
	model->q3BspModel.lmCoordArray = (vec2_t *)buffer; buffer += count * sizeof (vec2_t);	
	model->q3BspModel.colorArray = (bvec4_t *)buffer;

	out_verts = model->q3BspModel.vertexArray[0];
	out_normals = model->q3BspModel.normalsArray[0];
	out_texCoords = model->q3BspModel.coordArray[0];
	out_lmCoords = model->q3BspModel.lmCoordArray[0];
	out_colors = model->q3BspModel.colorArray[0];

	if (r_lmModulate->integer > 0)
		div = (float)(1 << r_lmModulate->integer) / 255.0f;
	else
		div = 1.0f / 255.0f;

	for (i=0 ; i<count ; i++, in++, out_verts+=3, out_normals+=3, out_texCoords+=2, out_lmCoords+=2, out_colors+=4) {
		out_verts[0] = LittleFloat (in->point[0]);
		out_verts[1] = LittleFloat (in->point[1]);
		out_verts[2] = LittleFloat (in->point[2]);
		out_normals[0] = LittleFloat (in->normal[0]);
		out_normals[1] = LittleFloat (in->normal[1]);
		out_normals[2] = LittleFloat (in->normal[2]);
		out_texCoords[0] = LittleFloat (in->texCoords[0]);
		out_texCoords[1] = LittleFloat (in->texCoords[1]);
		out_lmCoords[0] = LittleFloat (in->lmCoords[0]);
		out_lmCoords[1] = LittleFloat (in->lmCoords[1]);

		if (r_fullbright->integer) {
			out_colors[0] = 255;
			out_colors[1] = 255;
			out_colors[2] = 255;
			out_colors[3] = in->color[3];
		}
		else {
			color[0] = ((float)in->color[0] * div);
			color[1] = ((float)in->color[1] * div);
			color[2] = ((float)in->color[2] * div);
			R_ColorNormalize (color, fcolor);

			out_colors[0] = (byte)(fcolor[0] * 255);
			out_colors[1] = (byte)(fcolor[1] * 255);
			out_colors[2] = (byte)(fcolor[2] * 255);
			out_colors[3] = in->color[3];
		}
	}
}


/*
=================
R_LoadQ3BSPSubmodels
=================
*/
static void R_LoadQ3BSPSubmodels (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i;
	dQ3BspModel_t	*in;
	mBspHeader_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof(*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPSubmodels: funny lump size in %s", model->name);

	model->bspModel.numSubModels = lump->fileLen / sizeof (*in);
	model->bspModel.subModels = out = R_ModAllocZero (model, model->bspModel.numSubModels * sizeof (*out));

	for (i=0 ; i<model->bspModel.numSubModels ; i++, in++, out++) {
		// Spread the mins / maxs by a pixel
		out->mins[0] = LittleFloat (in->mins[0]) - 1;
		out->mins[1] = LittleFloat (in->mins[1]) - 1;
		out->mins[2] = LittleFloat (in->mins[2]) - 1;
		out->maxs[0] = LittleFloat (in->maxs[0]) + 1;
		out->maxs[1] = LittleFloat (in->maxs[1]) + 1;
		out->maxs[2] = LittleFloat (in->maxs[2]) + 1;

		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->firstFace = LittleLong (in->firstFace);
		out->numFaces = LittleLong (in->numFaces);
	}
}


/*
=================
R_LoadQ3BSPShaderRefs
=================
*/
static void R_LoadQ3BSPShaderRefs (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int 				i;
	dQ3BspShaderRef_t	*in;
	mQ3BspShaderRef_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPShaderRefs: funny lump size in %s", model->name);	

	model->q3BspModel.numShaderRefs = lump->fileLen / sizeof (*in);
	model->q3BspModel.shaderRefs = out = R_ModAllocZero (model, model->q3BspModel.numShaderRefs * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numShaderRefs ; i++, in++, out++) {
		Q_strncpyz (out->name, in->name, sizeof (out->name));
		out->flags = LittleLong (in->flags);
		out->contents = LittleLong (in->contents);
		out->shader = NULL;
	}
}


/*
=================
Mod_CreateMeshForSurface
=================
*/
#define COLOR_RGB(r,g,b)	(((r) << 0)|((g) << 8)|((b) << 16))
#define COLOR_RGBA(r,g,b,a) (((r) << 0)|((g) << 8)|((b) << 16)|((a) << 24))
static mesh_t *Mod_CreateMeshForSurface (model_t *model, dQ3BspFace_t *in, mBspSurface_t *out)
{
	static vec3_t	tempNormalsArray[VERTARRAY_MAX_VERTS];
	mesh_t			*mesh;

	switch (out->q3_faceType) {
	case FACETYPE_FLARE:
		{
			int r, g, b;

			mesh = (mesh_t *)R_ModAllocZero (model, sizeof (mesh_t) + sizeof (vec3_t));
			mesh->vertexArray = (vec3_t *)((byte *)mesh + sizeof (mesh_t));
			mesh->numVerts = 1;
			mesh->indexArray = (index_t *)1;
			mesh->numIndexes = 1;
			VectorCopy (out->q3_origin, mesh->vertexArray[0]);

			r = LittleFloat (in->mins[0]) * 255.0f;
			clamp (r, 0, 255);

			g = LittleFloat (in->mins[1]) * 255.0f;
			clamp (g, 0, 255);

			b = LittleFloat (in->mins[2]) * 255.0f;
			clamp (b, 0, 255);

			out->dLightBits = (uint32)COLOR_RGB(r, g, b);
		}
		return mesh;

	case FACETYPE_PATCH:
		{
			int				i, u, v, p;
			int				patch_cp[2], step[2], size[2], flat[2];
			float			subdivLevel, f;
			int				numVerts, firstVert;
			static vec4_t	colors[VERTARRAY_MAX_VERTS];
			static vec4_t	colors2[VERTARRAY_MAX_VERTS];
			index_t			*indexes;
			byte			*buffer;

			patch_cp[0] = LittleLong (in->patch_cp[0]);
			patch_cp[1] = LittleLong (in->patch_cp[1]);

			if (!patch_cp[0] || !patch_cp[1])
				break;

			subdivLevel = bound (1, r_patchDivLevel->integer, 32);

			numVerts = LittleLong (in->numVerts);
			firstVert = LittleLong (in->firstVert);
			for (i=0 ; i<numVerts ; i++)
				Vector4Scale (model->q3BspModel.colorArray[firstVert + i], (1.0 / 255.0), colors[i]);

			// Find the degree of subdivision in the u and v directions
			Patch_GetFlatness (subdivLevel, &model->q3BspModel.vertexArray[firstVert], patch_cp, flat);

			// Allocate space for mesh
			step[0] = (1 << flat[0]);
			step[1] = (1 << flat[1]);
			size[0] = (patch_cp[0] >> 1) * step[0] + 1;
			size[1] = (patch_cp[1] >> 1) * step[1] + 1;
			numVerts = size[0] * size[1];

			if (numVerts > VERTARRAY_MAX_VERTS)
				break;

			out->q3_patchWidth = size[0];
			out->q3_patchHeight = size[1];

			buffer = R_ModAllocZero (model, sizeof (mesh_t)
				+ (numVerts * sizeof (vec2_t) * 2)
				+ (numVerts * sizeof (vec3_t) * 2)
				+ (numVerts * sizeof (bvec4_t))
				+ (numVerts * sizeof (index_t) * ((size[0] - 1) * (size[1] - 1) * 6)));

			mesh = (mesh_t *)buffer; buffer += sizeof (mesh_t);
			mesh->numVerts = numVerts;
			mesh->vertexArray = (vec3_t *)buffer; buffer += numVerts * sizeof (vec3_t);
			mesh->normalsArray = (vec3_t *)buffer; buffer += numVerts * sizeof (vec3_t);
			mesh->coordArray = (vec2_t *)buffer; buffer += numVerts * sizeof (vec2_t);
			mesh->lmCoordArray = (vec2_t *)buffer; buffer += numVerts * sizeof (vec2_t);
			mesh->colorArray = (bvec4_t *)buffer; buffer += numVerts * sizeof (bvec4_t);

			Patch_Evaluate (model->q3BspModel.vertexArray[firstVert], patch_cp, step, mesh->vertexArray[0], 3);
			Patch_Evaluate (model->q3BspModel.normalsArray[firstVert], patch_cp, step, tempNormalsArray[0], 3);
			Patch_Evaluate (colors[0], patch_cp, step, colors2[0], 4);
			Patch_Evaluate (model->q3BspModel.coordArray[firstVert], patch_cp, step, mesh->coordArray[0], 2);
			Patch_Evaluate (model->q3BspModel.lmCoordArray[firstVert], patch_cp, step, mesh->lmCoordArray[0], 2);

			for (i=0 ; i<numVerts ; i++) {
				VectorNormalizef (tempNormalsArray[i], mesh->normalsArray[i]);

				f = max (max (colors2[i][0], colors2[i][1]), colors2[i][2]);
				if (f > 1.0f) {
					f = 255.0f / f;
					mesh->colorArray[i][0] = colors2[i][0] * f;
					mesh->colorArray[i][1] = colors2[i][1] * f;
					mesh->colorArray[i][2] = colors2[i][2] * f;
				}
				else {
					mesh->colorArray[i][0] = colors2[i][0] * 255;
					mesh->colorArray[i][1] = colors2[i][1] * 255;
					mesh->colorArray[i][2] = colors2[i][2] * 255;
				}
			}

			// Compute new indexes
			mesh->numIndexes = (size[0] - 1) * (size[1] - 1) * 6;
			indexes = mesh->indexArray = (index_t *)buffer;
			for (v=0, i=0 ; v<size[1]-1 ; v++) {
				for (u=0 ; u<size[0]-1 ; u++) {
					indexes[0] = p = v * size[0] + u;
					indexes[1] = p + size[0];
					indexes[2] = p + 1;
					indexes[3] = p + 1;
					indexes[4] = p + size[0];
					indexes[5] = p + size[0] + 1;
					indexes += 6;
				}
			}
		}
		return mesh;

	case FACETYPE_PLANAR:
	case FACETYPE_TRISURF:
		{
			int		firstVert = LittleLong (in->firstVert);

			mesh = (mesh_t *)R_ModAllocZero (model, sizeof (mesh_t));
			mesh->numVerts = LittleLong (in->numVerts);
			mesh->vertexArray = model->q3BspModel.vertexArray + firstVert;
			mesh->normalsArray = model->q3BspModel.normalsArray + firstVert;
			mesh->coordArray = model->q3BspModel.coordArray + firstVert;
			mesh->lmCoordArray = model->q3BspModel.lmCoordArray + firstVert;
			mesh->colorArray = model->q3BspModel.colorArray + firstVert;
			mesh->indexArray = model->q3BspModel.surfIndexes + LittleLong (in->firstIndex);
			mesh->numIndexes = LittleLong (in->numIndexes);
		}
		return mesh;
	}

	return NULL;
}

/*
=================
R_LoadQ3BSPFaces
=================
*/
static void R_FixAutosprites (mBspSurface_t *surf)
{
	int i, j;
	vec2_t *stArray;
	index_t *quad;
	mesh_t *mesh;
	shader_t *shader;

	if ((surf->q3_faceType != FACETYPE_PLANAR && surf->q3_faceType != FACETYPE_TRISURF) || !surf->q3_shaderRef)
		return;

	mesh = surf->q3_mesh;
	if (!mesh || !mesh->numIndexes || mesh->numIndexes % 6)
		return;

	shader = surf->q3_shaderRef->shader;
	if (!shader || !shader->numDeforms || !(shader->flags & SHADER_AUTOSPRITE))
		return;

	for (i=0 ; i<shader->numDeforms ; i++)
		if (shader->deforms[i].type == DEFORMV_AUTOSPRITE)
			break;

	if (i == shader->numDeforms)
		return;

	stArray = mesh->coordArray;
	for (i=0, quad=mesh->indexArray ; i<mesh->numIndexes ; i+=6, quad+=6) {
		for (j=0 ; j<6 ; j++) {
			if (stArray[quad[j]][0] < -0.1 || stArray[quad[j]][0] > 1.1 ||
				stArray[quad[j]][1] < -0.1 || stArray[quad[j]][1] > 1.1) {
				stArray[quad[0]][0] = 0;
				stArray[quad[0]][1] = 1;
				stArray[quad[1]][0] = 0;
				stArray[quad[1]][1] = 0;
				stArray[quad[2]][0] = 1;
				stArray[quad[2]][1] = 1;
				stArray[quad[5]][0] = 1;
				stArray[quad[5]][1] = 0;
				break;
			}
		}
	}
}
static void R_LoadQ3BSPFaces (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int					j;
	dQ3BspFace_t		*in;
	mBspSurface_t 		*out;
	mesh_t				*mesh;
	mQ3BspFog_t			*fog;
	mQ3BspShaderRef_t	*shaderref;
	int					shadernum, fognum, surfnum;
	float				*vert;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "Mod_LoadFaces: funny lump size in %s", model->name);

	model->bspModel.numSurfaces = lump->fileLen / sizeof (*in);
	model->bspModel.surfaces = out = R_ModAllocZero (model, model->bspModel.numSurfaces * sizeof (*out));

	// Fill it in
	for (surfnum=0 ; surfnum<model->bspModel.numSurfaces ; surfnum++, in++, out++) {
		out->q3_origin[0] = LittleFloat (in->origin[0]);
		out->q3_origin[1] = LittleFloat (in->origin[1]);
		out->q3_origin[2] = LittleFloat (in->origin[2]);

		out->q3_faceType = LittleLong (in->faceType);

		// Lighting info
		if (r_vertexLighting->integer) {
			out->lmTexNum = -1;
		}
		else {
			out->lmTexNum = LittleLong (in->lmTexNum);
			if (out->lmTexNum >= model->q3BspModel.numLightmaps) {
				Com_DevPrintf (PRNT_ERROR, "WARNING: bad lightmap number: %i\n", out->lmTexNum);
				out->lmTexNum = -1;
			}
		}

		// Shaderref
		shadernum = LittleLong (in->shaderNum);
		if (shadernum < 0 || shadernum >= model->q3BspModel.numShaderRefs)
			Com_Error (ERR_DROP, "R_LoadQ3BSPFaces: bad shader number");

		shaderref = model->q3BspModel.shaderRefs + shadernum;
		out->q3_shaderRef = shaderref;

		if (!shaderref->shader) {
			if (out->q3_faceType == FACETYPE_FLARE) {
				shaderref->shader = R_RegisterFlare (shaderref->name);
				if (!shaderref->shader) {
					Com_Printf (PRNT_WARNING, "Couldn't load (flare): '%s'\n", shaderref->name);
					shaderref->shader = r_noShader;
				}
			}
			else {
				if (out->q3_faceType == FACETYPE_TRISURF || r_vertexLighting->integer || out->lmTexNum < 0) {
					shaderref->shader = R_RegisterTextureVertex (shaderref->name);
					if (!shaderref->shader) {
						Com_Printf (PRNT_WARNING, "Couldn't load (vertex): '%s'\n", shaderref->name);
						shaderref->shader = r_noShader;
					}
				}
				else {
					shaderref->shader = R_RegisterTextureLM (shaderref->name);
					if (!shaderref->shader) {
						Com_Printf (PRNT_WARNING, "Couldn't load (lm): '%s'\n", shaderref->name);
						shaderref->shader = r_noShaderLightmap;
					}
				}
			}
		}

		// Fog
		fognum = LittleLong (in->fogNum);
		if (fognum != -1 && fognum < model->q3BspModel.numFogs) {
			fog = model->q3BspModel.fogs + fognum;
			if (fog->numPlanes && fog->shader && fog->name[0])
				out->q3_fog = fog;
		}

		// Mesh
		mesh = out->q3_mesh = Mod_CreateMeshForSurface (model, in, out);
		if (!mesh)
			continue;

		// Bounds
		ClearBounds (out->mins, out->maxs);
		for (j=0, vert=mesh->vertexArray[0] ; j<mesh->numVerts ; j++, vert+=3)
			AddPointToBounds (vert, out->mins, out->maxs);

		if (out->q3_faceType == FACETYPE_PLANAR) {
			out->q3_origin[0] = LittleFloat (in->normal[0]);
			out->q3_origin[1] = LittleFloat (in->normal[1]);
			out->q3_origin[2] = LittleFloat (in->normal[2]);
		}

		// Fix autosprites
		R_FixAutosprites (out);
	}
}


/*
=================
R_LoadQ3BSPNodes
=================
*/
static void R_LoadQ3BSPNodes (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, j, p;
	dQ3BspNode_t	*in;
	mBspNode_t 		*out;
	qBool			badBounds;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPNodes: funny lump size in %s", model->name);

	model->bspModel.numNodes = lump->fileLen / sizeof(*in);
	model->bspModel.nodes = out = R_ModAllocZero (model, model->bspModel.numNodes * sizeof (*out));

	for (i=0 ; i<model->bspModel.numNodes ; i++, in++, out++) {
		out->plane = model->bspModel.planes + LittleLong (in->planeNum);

		for (j=0 ; j<2 ; j++) {
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = model->bspModel.nodes + p;
			else
				out->children[j] = (mBspNode_t *)(model->bspModel.leafs + (-1 - p));
		}

		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->mins[j] = LittleFloat (in->mins[j]);
			out->maxs[j] = LittleFloat (in->maxs[j]);
			if (out->mins[j] > out->maxs[j])
				badBounds = qTrue;
		}

		if (badBounds || VectorCompare (out->mins, out->maxs)) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad node %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint(out->mins[0]), Q_rint(out->mins[1]), Q_rint(out->mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint(out->maxs[0]), Q_rint(out->maxs[1]), Q_rint(out->maxs[2]));
		}
	}
}


/*
=================
R_LoadQ3BSPFogs
=================
*/
static void R_LoadQ3BSPFogs (model_t *model, byte *byteBase, const dQ3BspLump_t *lump, const dQ3BspLump_t *brLump, const dQ3BspLump_t *brSidesLump)
{
	int					i, j, p;
	dQ3BspFog_t 		*in;
	mQ3BspFog_t			*out;
	dQ3BspBrush_t 		*inbrushes, *brush;
	dQ3BspBrushSide_t	*inbrushsides, *brushside;

	inbrushes = (void *)(byteBase + brLump->fileOfs);
	if (brLump->fileLen % sizeof (*inbrushes))
		Com_Error (ERR_DROP, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);

	inbrushsides = (void *)(byteBase + brSidesLump->fileOfs);
	if (brSidesLump->fileLen % sizeof (*inbrushsides))
		Com_Error (ERR_DROP, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);

	model->q3BspModel.numFogs = lump->fileLen / sizeof (*in);
	model->q3BspModel.fogs = out = R_ModAllocZero (model, model->q3BspModel.numFogs * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numFogs ; i++, in++, out++) {
		Q_strncpyz (out->name, in->shader, sizeof (out->name));
		out->shader = R_RegisterTextureLM (in->shader);

		p = LittleLong (in->brushNum);
		if (p == -1)
			continue;	 // Global fog
		brush = inbrushes + p;

		p = LittleLong (brush->firstSide);
		if (p == -1) {
			out->name[0] = '\0';
			out->shader = NULL;
			continue;
		}
		brushside = inbrushsides + p;

		p = LittleLong (in->visibleSide);
		if (p == -1) {
			out->name[0] = '\0';
			out->shader = NULL;
			continue;
		}

		out->numPlanes = LittleLong (brush->numSides);
		out->planes = R_ModAllocZero (model, out->numPlanes * sizeof (cBspPlane_t));

		out->visiblePlane = model->bspModel.planes + LittleLong (brushside[p].planeNum);
		for (j=0 ; j<out->numPlanes; j++)
			out->planes[j] = *(model->bspModel.planes + LittleLong (brushside[j].planeNum));
	}
}


/*
=================
R_LoadQ3BSPLeafs
=================
*/
static void R_LoadQ3BSPLeafs (model_t *model, byte *byteBase, const dQ3BspLump_t *lump, const dQ3BspLump_t *msLump)
{
	int				i, j, k, countMarkSurfaces;
	dQ3BspLeaf_t 	*in;
	mBspLeaf_t 		*out;
	size_t			size;
	byte			*buffer;
	qBool			badBounds;
	int				*inMarkSurfaces;
	int				numMarkSurfaces, firstMarkSurface;
	int				numVisSurfaces, numLitSurfaces, numFragmentSurfaces;

	inMarkSurfaces = (void *)(byteBase + msLump->fileOfs);
	if (msLump->fileLen % sizeof (*inMarkSurfaces))
		Com_Error (ERR_DROP, "R_LoadQ3BSPLeafs: funny lump size in %s", model->name);
	countMarkSurfaces = msLump->fileLen / sizeof (*inMarkSurfaces);

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPLeafs: funny lump size in %s", model->name);

	model->bspModel.numLeafs = lump->fileLen / sizeof (*in);
	model->bspModel.leafs = out = R_ModAllocZero (model, model->bspModel.numLeafs * sizeof (*out));

	for (i=0 ; i<model->bspModel.numLeafs ; i++, in++, out++) {
		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->mins[j] = (float)LittleLong (in->mins[j]);
			out->maxs[j] = (float)LittleLong (in->maxs[j]);
			if (out->mins[j] > out->maxs[j])
				badBounds = qTrue;
		}
		out->cluster = LittleLong (in->cluster);

		if (i && (badBounds || VectorCompare (out->mins, out->maxs))) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad leaf %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint(out->mins[0]), Q_rint(out->mins[1]), Q_rint(out->mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint(out->maxs[0]), Q_rint(out->maxs[1]), Q_rint(out->maxs[2]));
			Com_DevPrintf (PRNT_WARNING, "cluster: %i\n", LittleLong (in->cluster));
			Com_DevPrintf (PRNT_WARNING, "surfaces: %i\n", LittleLong (in->numLeafFaces));
			Com_DevPrintf (PRNT_WARNING, "brushes: %i\n", LittleLong (in->numLeafBrushes));
			out->cluster = -1;
		}

		if (model->q3BspModel.vis) {
			if (out->cluster >= model->q3BspModel.vis->numClusters)
				Com_Error (ERR_DROP, "MOD_LoadBmodel: leaf cluster > numclusters");
		}

		out->q3_plane = NULL;
		out->area = LittleLong (in->area) + 1;

		numMarkSurfaces = LittleLong (in->numLeafFaces);
		if (!numMarkSurfaces)
			continue;
		firstMarkSurface = LittleLong (in->firstLeafFace);
		if (firstMarkSurface < 0 || numMarkSurfaces + firstMarkSurface > countMarkSurfaces)
			Com_Error (ERR_DROP, "MOD_LoadBmodel: bad marksurfaces in leaf %i", i);

		numVisSurfaces = numLitSurfaces = numFragmentSurfaces = 0;

		for (j=0 ; j<numMarkSurfaces ; j++) {
			k = LittleLong (inMarkSurfaces[firstMarkSurface + j]);
			if (k < 0 || k >= model->bspModel.numSurfaces)
				Com_Error (ERR_DROP, "R_LoadQ3BSPLeafs: bad surface number");

			if (R_Q3BSP_SurfPotentiallyVisible (model->bspModel.surfaces + k)) {
				numVisSurfaces++;

				if (R_Q3BSP_SurfPotentiallyLit (model->bspModel.surfaces + k))
					numLitSurfaces++;

				if (R_Q3BSP_SurfPotentiallyFragmented (model->bspModel.surfaces + k))
					numFragmentSurfaces++;
			}
		}

		if (!numVisSurfaces)
			continue;

		size = numVisSurfaces + 1;
		if (numLitSurfaces)
			size += numLitSurfaces + 1;
		if (numFragmentSurfaces)
			size += numFragmentSurfaces + 1;
		size *= sizeof (mBspSurface_t *);

		buffer = (byte *) R_ModAllocZero (model, size);

		out->q3_firstVisSurface = (mBspSurface_t **)buffer;
		buffer += (numVisSurfaces + 1) * sizeof (mBspSurface_t *);
		if (numLitSurfaces) {
			out->q3_firstLitSurface = (mBspSurface_t **)buffer;
			buffer += (numLitSurfaces + 1) * sizeof (mBspSurface_t *);
		}
		if (numFragmentSurfaces) {
			out->q3_firstFragmentSurface = (mBspSurface_t **)buffer;
			buffer += (numFragmentSurfaces + 1) * sizeof (mBspSurface_t *);
		}

		numVisSurfaces = numLitSurfaces = numFragmentSurfaces = 0;

		for (j=0 ; j<numMarkSurfaces ; j++) {
			k = LittleLong (inMarkSurfaces[firstMarkSurface + j]);

			if (R_Q3BSP_SurfPotentiallyVisible (model->bspModel.surfaces + k)) {
				out->q3_firstVisSurface[numVisSurfaces++] = model->bspModel.surfaces + k;

				if (R_Q3BSP_SurfPotentiallyLit (model->bspModel.surfaces + k))
					out->q3_firstLitSurface[numLitSurfaces++] = model->bspModel.surfaces + k;

				if (R_Q3BSP_SurfPotentiallyFragmented (model->bspModel.surfaces + k))
					out->q3_firstFragmentSurface[numFragmentSurfaces++] = model->bspModel.surfaces + k;
			}
		}
	}
}


/*
=================
R_LoadQ3BSPEntities
=================
*/
static void R_LoadQ3BSPEntities (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	char			*data;
	mQ3BspLight_t	*out;
	int				count, total, gridsizei[3];
	qBool			islight, isworld;
	float			scale, gridsizef[3];
	char			key[MAX_KEY], value[MAX_VALUE], target[MAX_VALUE], *token;
	parse_t			*ps;

	data = (char *)byteBase + lump->fileOfs;
	if (!data || !data[0])
		return;

	VectorClear (gridsizei);
	VectorClear (gridsizef);

	ps = Com_BeginParseSession (&data);
	for (total=0 ; (token = Com_Parse (ps)) && token[0] == '{' ; ) {
		islight = qFalse;
		isworld = qFalse;

		for ( ; ; ) {
			if (!(token = Com_Parse (ps)))
				break; // error
			if (token[0] == '}')
				break; // end of entity

			Q_strncpyz (key, token, sizeof (key));
			while (key[strlen(key)-1] == ' ')	// remove trailing spaces
				key[strlen(key)-1] = 0;

			if (!(token = Com_Parse (ps)))
				break; // Error

			Q_strncpyz (value, token, sizeof (value));

			// now that we have the key pair worked out...
			if (!strcmp (key, "classname")) {
				if (!strncmp (value, "light", 5))
					islight = qTrue;
				else if (!strcmp (value, "worldspawn"))
					isworld = qTrue;
			}
			else if (!strcmp (key, "gridsize")) {
				sscanf (value, "%f %f %f", &gridsizef[0], &gridsizef[1], &gridsizef[2]);

				if (!gridsizef[0] || !gridsizef[1] || !gridsizef[2]) {
					sscanf (value, "%i %i %i", &gridsizei[0], &gridsizei[1], &gridsizei[2]);
					VectorCopy (gridsizei, gridsizef);
				}
			}
		}

		if (isworld) {
			VectorCopy (gridsizef, model->q3BspModel.gridSize);
			continue;
		}

		if (islight)
			total++;
	}
	Com_EndParseSession (ps);

#if !(SHADOW_VOLUMES)
	total = 0;
#endif

	if (!total)
		return;

	out = R_ModAllocZero (model, total * sizeof (*out));
	model->q3BspModel.worldLights = out;
	model->q3BspModel.numWorldLights = total;

	data = (char *)byteBase + lump->fileOfs;
	ps = Com_BeginParseSession (&data);
	for (count=0 ; (token = Com_Parse (ps)) && token[0] == '{' ; ) {
		if (count == total)
			break;

		islight = qFalse;

		for ( ; ; ) {
			if (!(token = Com_Parse (ps)))
				break; // error
			if (token[0] == '}')
				break; // end of entity

			Q_strncpyz (key, token, sizeof (key));
			while (key[strlen(key)-1] == ' ')		// remove trailing spaces
				key[strlen(key)-1] = 0;

			if (!(token = Com_Parse (ps)))
				break; // error

			Q_strncpyz (value, token, sizeof (value));

			// now that we have the key pair worked out...
			if (!strcmp (key, "origin"))
				sscanf (value, "%f %f %f", &out->origin[0], &out->origin[1], &out->origin[2]);
			else if (!strcmp (key, "color") || !strcmp (key, "_color"))
				sscanf (value, "%f %f %f", &out->color[0], &out->color[1], &out->color[2]);
			else if (!strcmp (key, "light") || !strcmp (key, "_light"))
				out->intensity = atof (value);
			else if (!strcmp (key, "classname")) {
				if (!strncmp (value, "light", 5))
					islight = qTrue;
			}
			else if (!strcmp (key, "target"))
				Q_strncpyz (target, value, sizeof (target));
		}

		if (!islight)
			continue;

		if (out->intensity <= 0)
			out->intensity = 300;
		out->intensity += 15;

		scale = max (max (out->color[0], out->color[1]), out->color[2]);
		if (!scale) {
			out->color[0] = 1;
			out->color[1] = 1;
			out->color[2] = 1;
		}
		else {
			// Normalize
			scale = 1.0f / scale;
			VectorScale (out->color, scale, out->color);
		}

		out++;
		count++;
	}
	Com_EndParseSession (ps);
}


/*
=================
R_LoadQ3BSPIndexes
=================
*/
static void R_LoadQ3BSPIndexes (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int		i, *in, *out;
	
	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPIndexes: funny lump size in %s", model->name);

	model->q3BspModel.numSurfIndexes = lump->fileLen / sizeof (*in);
	model->q3BspModel.surfIndexes = out = R_ModAllocZero (model, model->q3BspModel.numSurfIndexes * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numSurfIndexes ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
R_LoadQ3BSPPlanes
=================
*/
static void R_LoadQ3BSPPlanes (model_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ3BspPlane_t 	*in;
	
	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadQ3BSPPlanes: funny lump size in %s", model->name);

	model->bspModel.numPlanes = lump->fileLen / sizeof (*in);
	model->bspModel.planes = out = R_ModAllocZero (model, model->bspModel.numPlanes * sizeof (*out));

	for (i=0 ; i<model->bspModel.numPlanes ; i++, in++, out++) {
		out->type = PLANE_NON_AXIAL;
		out->signBits = 0;

		for (j=0 ; j<3 ; j++) {
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				out->signBits |= 1<<j;
			if (out->normal[j] == 1.0f)
				out->type = j;
		}
		out->dist = LittleFloat (in->dist);
	}
}


/*
=================
R_FinishQ3BSPModel
=================
*/
static void R_FinishQ3BSPModel (model_t *model, byte *byteBase, const dQ3BspLump_t *lightmaps)
{
	mesh_t					*mesh;
	mBspSurface_t 			*surf;
	mQ3BspLightmapRect_t	*lmRect;
	float					*lmArray;
	int						i, j;

	R_Q3BSP_BuildLightmaps (model->q3BspModel.numLightmaps, Q3LIGHTMAP_WIDTH, Q3LIGHTMAP_WIDTH, byteBase + lightmaps->fileOfs, model->q3BspModel.lightmapRects);

	// Now walk list of surface and apply lightmap info
	for (i=0, surf=model->bspModel.surfaces ; i<model->bspModel.numSurfaces ; i++, surf++) {
		if (surf->lmTexNum < 0 || surf->q3_faceType == FACETYPE_FLARE || !(mesh = surf->q3_mesh)) {
			surf->lmTexNum = -1;
		}
		else {
			lmRect = &model->q3BspModel.lightmapRects[surf->lmTexNum];
			surf->lmTexNum = lmRect->texNum;

			if (r_lmPacking->integer) {
				// Scale/shift lightmap coords
				lmArray = mesh->lmCoordArray[0];
				for (j=0 ; j<mesh->numVerts ; j++, lmArray+=2) {
					lmArray[0] = (double)(lmArray[0]) * lmRect->w + lmRect->x;
					lmArray[1] = (double)(lmArray[1]) * lmRect->h + lmRect->y;
				}
			}
		}
	}

	if (model->q3BspModel.numLightmaps)
		Mem_Free (model->q3BspModel.lightmapRects);

	R_SetParentQ3BSPNode (model->bspModel.nodes, NULL);
}


/*
=================
R_LoadQ3BSPModel
=================
*/
static void R_LoadQ3BSPModel (model_t *model, byte *buffer)
{
	dQ3BspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	vec3_t			maxs;
	int				version;
	int				i;

	//
	// Load the world model
	//
	model->type = MODEL_Q3BSP;

	header = (dQ3BspHeader_t *)buffer;
	version = LittleLong (header->version);
	if (version != Q3BSP_VERSION)
		Com_Error (ERR_DROP, "R_LoadQ3BSPModel: %s has wrong version number (%i should be %i)", model->name, version, Q3BSP_VERSION);

	//
	// Swap all the lumps
	//
	modBase = (byte *)header;
	for (i=0 ; i<sizeof (dQ3BspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// Load into heap
	//
	R_LoadQ3BSPEntities		(model, modBase, &header->lumps[Q3BSP_LUMP_ENTITIES]);
	R_LoadQ3BSPVertexes		(model, modBase, &header->lumps[Q3BSP_LUMP_VERTEXES]);
	R_LoadQ3BSPIndexes		(model, modBase, &header->lumps[Q3BSP_LUMP_INDEXES]);
	R_LoadQ3BSPLighting		(model, modBase, &header->lumps[Q3BSP_LUMP_LIGHTING], &header->lumps[Q3BSP_LUMP_LIGHTGRID]);
	R_LoadQ3BSPVisibility	(model, modBase, &header->lumps[Q3BSP_LUMP_VISIBILITY]);
	R_LoadQ3BSPShaderRefs	(model, modBase, &header->lumps[Q3BSP_LUMP_SHADERREFS]);
	R_LoadQ3BSPPlanes		(model, modBase, &header->lumps[Q3BSP_LUMP_PLANES]);
	R_LoadQ3BSPFogs			(model, modBase, &header->lumps[Q3BSP_LUMP_FOGS], &header->lumps[Q3BSP_LUMP_BRUSHES], &header->lumps[Q3BSP_LUMP_BRUSHSIDES]);
	R_LoadQ3BSPFaces		(model, modBase, &header->lumps[Q3BSP_LUMP_FACES]);
	R_LoadQ3BSPLeafs		(model, modBase, &header->lumps[Q3BSP_LUMP_LEAFS], &header->lumps[Q3BSP_LUMP_LEAFFACES]);
	R_LoadQ3BSPNodes		(model, modBase, &header->lumps[Q3BSP_LUMP_NODES]);
	R_LoadQ3BSPSubmodels	(model, modBase, &header->lumps[Q3BSP_LUMP_MODELS]);

	R_FinishQ3BSPModel		(model, modBase, &header->lumps[Q3BSP_LUMP_LIGHTING]);

	//
	// Set up the submodels
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++) {
		model_t		*starmod;

		bm = &model->bspModel.subModels[i];
		starmod = &r_inlineModels[i];

		*starmod = *model;

		starmod->bspModel.firstModelSurface = starmod->bspModel.surfaces + bm->firstFace;
		starmod->bspModel.numModelSurfaces = bm->numFaces;

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*model = *starmod;
	}

	//
	// Set up lightgrid
	//
	if (model->q3BspModel.gridSize[0] < 1 || model->q3BspModel.gridSize[1] < 1 || model->q3BspModel.gridSize[2] < 1)
		VectorSet (model->q3BspModel.gridSize, 64, 64, 128);

	for (i=0 ; i<3 ; i++) {
		model->q3BspModel.gridMins[i] = model->q3BspModel.gridSize[i] * ceil ((model->mins[i] + 1) / model->q3BspModel.gridSize[i]);
		maxs[i] = model->q3BspModel.gridSize[i] * floor ((model->maxs[i] - 1) / model->q3BspModel.gridSize[i]);
		model->q3BspModel.gridBounds[i] = (maxs[i] - model->q3BspModel.gridMins[i])/model->q3BspModel.gridSize[i] + 1;
	}
	model->q3BspModel.gridBounds[3] = model->q3BspModel.gridBounds[1] * model->q3BspModel.gridBounds[0];
}

/*
===============================================================================

	MODEL REGISTRATION

===============================================================================
*/

/*
================
R_FreeModel
================
*/
static void R_FreeModel (model_t *model)
{
	assert (model);
	if (!model)
		return;

	// Free it
	if (model->memSize > 0)
		Mem_FreeTag (MEMPOOL_MODELSYS, model->memTag);

	// Clear the spot
	memset (model, 0, sizeof (model_t));
}


/*
================
R_TouchModel

Touches/loads all textures for the model type
================
*/
static void R_TouchModel (model_t *model)
{
	mAliasModel_t		*aliasModel;
	mAliasMesh_t		*aliasMesh;
	mAliasSkin_t		*aliasSkin;
	mQ2BspTexInfo_t		*texInfo;
	mBspSurface_t		*surf;
	mQ3BspFog_t			*fog;
	mQ3BspShaderRef_t	*shaderref;
	mSpriteModel_t		*spriteModel;
	mSpriteFrame_t		*spriteFrame;
	int					skinNum, i;

	model->touchFrame = r_refRegInfo.registerFrame;

	switch (model->type) {
	case MODEL_MD2:
	case MODEL_MD3:
		aliasModel = model->aliasModel;
		for (i=0; i<aliasModel->numMeshes; i++) {
			aliasMesh = &aliasModel->meshes[i];
			aliasSkin = aliasMesh->skins;
			for (skinNum=0 ; skinNum<aliasMesh->numSkins ; skinNum++) {
				if (!aliasSkin[skinNum].name[0]) {
					aliasSkin[skinNum].skin = NULL;
					continue;
				}

				aliasSkin[skinNum].skin = R_RegisterSkin (aliasSkin[skinNum].name);
			}
		}
		break;

	case MODEL_Q2BSP:
		for (i=0, texInfo=model->q2BspModel.texInfo ; i<model->q2BspModel.numTexInfo ; texInfo++, i++) {
			if (texInfo->flags & SURF_TEXINFO_SKY) {
				texInfo->shader = r_noShaderSky;
				continue;
			}

			texInfo->shader = R_RegisterTexture (texInfo->texName, texInfo->surfParams);
			if (!texInfo->shader) {
				if (texInfo->surfParams & SHADER_SURF_LIGHTMAP)
					texInfo->shader = r_noShaderLightmap;
				else
					texInfo->shader = r_noShader;
			}
		}

		R_TouchLightmaps ();
		break;

	case MODEL_Q3BSP:
		surf = model->bspModel.surfaces;
		for (i=0 ; i<model->bspModel.numSurfaces ; i++, surf++) {
			shaderref = surf->q3_shaderRef;

			if (surf->q3_faceType == FACETYPE_FLARE) {
				shaderref->shader = R_RegisterFlare (shaderref->name);
				if (!shaderref->shader)
					shaderref->shader = r_noShader;
			}
			else {
				if (surf->q3_faceType == FACETYPE_TRISURF || r_vertexLighting->integer || surf->lmTexNum < 0) {
					shaderref->shader = R_RegisterTextureVertex (shaderref->name);
					if (!shaderref->shader)
						shaderref->shader = r_noShader;
				}
				else {
					shaderref->shader = R_RegisterTextureLM (shaderref->name);
					if (!shaderref->shader)
						shaderref->shader = r_noShaderLightmap;
				}
			}
		}

		fog = model->q3BspModel.fogs;
		for (i=0 ; i<model->q3BspModel.numFogs ; i++, fog++) {
			if (!fog->name[0]) {
				fog->shader = NULL;
				continue;
			}
			fog->shader = R_RegisterTextureLM (fog->name);
		}

		R_TouchLightmaps ();
		break;

	case MODEL_SP2:
		spriteModel = model->spriteModel;
		for (spriteFrame=spriteModel->frames, i=0 ; i<spriteModel->numFrames ; i++, spriteFrame++) {
			if (!spriteFrame) {
				spriteFrame->skin = NULL;
				continue;
			}

			spriteFrame->skin = R_RegisterPoly (spriteFrame->name);
		}
		break;
	}
}


/*
================
R_FindModel
================
*/
static inline model_t *R_FindModel (char *name)
{
	model_t		*model;
	int			i;

	if (r_refRegInfo.inSequence)
		r_refRegInfo.modelsSearched++;

	// Inline models are grabbed only from worldmodel
	if (name[0] == '*') {
		i = atoi (name+1);

		if (i < 1 || i >= r_refScene.worldModel->bspModel.numSubModels)
			Com_Error (ERR_DROP, "R_FindModel: Bad inline model number '%d'", i);

		model = &r_inlineModels[i];
		model->memTag = r_refScene.worldModel->memTag;
		return model;
	}

	// Search the currently loaded models
	for (i=0, model=r_modelList ; i<r_numModels ; i++, model++) {
		if (!model->name[0])
			continue;

		if (!strcmp (model->name, name))
			return model;
	}

	return NULL;
}


/*
================
R_FindModelSlot
================
*/
static inline model_t *R_FindModelSlot (void)
{
	model_t		*model;
	int			i;

	// Find a free model slot spot
	for (i=0, model=r_modelList ; i<r_numModels ; i++, model++) {
		if (model->touchFrame)
			continue;
		if (model->name[0])
			continue;

		model->memTag = i+1;
		break;	// Free spot
	}

	if (i == r_numModels) {
		if (r_numModels+1 >= MAX_MODEL_KNOWN)
			Com_Error (ERR_DROP, "r_numModels >= MAX_MODEL_KNOWN");

		model = &r_modelList[r_numModels++];
		model->memTag = r_numModels;
	}

	return model;
}


/*
==================
R_ModelForName

Loads in a model for the given name
==================
*/
typedef struct modFormat_s {
	const char	*headerStr;
	int			headerLen;
	int			version;
	int			maxLods;
	void		(*loader) (model_t *model, byte *buffer);
} modFormat_t;

static modFormat_t modelFormats[] = {
	{ MD2_HEADERSTR,	4,	0,				ALIAS_MAX_LODS,		R_LoadMD2Model },	// Quake2 MD2 models
	{ MD3_HEADERSTR,	4,	0,				ALIAS_MAX_LODS,		R_LoadMD3Model },	// Quake3 MD3 models
	{ SP2_HEADERSTR,	4,	0,				0,					R_LoadSP2Model },	// Quake2 SP2 sprite models
	{ Q2BSP_HEADER,		4,	Q2BSP_VERSION,	0,					R_LoadQ2BSPModel },	// Quake2 BSP models
	{ Q3BSP_HEADER,		4,	Q3BSP_VERSION,	0,					R_LoadQ3BSPModel },	// Quake3 BSP models

	{ NULL,				0,	0,				0,					NULL }
};

static int numModelFormats = (sizeof (modelFormats) / sizeof (modelFormats[0])) - 1;

static model_t *R_ModelForName (char *name, qBool worldModel)
{
	model_t		*model;
	byte		*buffer;
	int			i, fileLen;
	modFormat_t	*descr;

	if (!name)
		Com_Error (ERR_DROP, "R_ModelForName: NULL name");
	if (!name[0])
		Com_Error (ERR_DROP, "R_ModelForName: empty name");

	// Use if already loaded
	model = R_FindModel (name);
	if (model) {
		R_TouchModel (model);
		return model;
	}

	// Not found -- allocate a spot
	model = R_FindModelSlot ();

	// Load the file
	fileLen = FS_LoadFile (name, (void **)&buffer);
	if (!buffer || fileLen <= 0) {
		if (worldModel)
			Com_Error (ERR_DROP, "R_ModelForName: %s not found", name);
		memset (model->name, 0, sizeof (model->name));
		return NULL;
	}
	else
		Q_strncpyz (model->name, name, sizeof (model->name));

	// Find the format
	descr = modelFormats;
	for (i=0 ; i<numModelFormats ; i++, descr++) {
		if (strncmp ((const char *)buffer, descr->headerStr, descr->headerLen))
			continue;
		if (descr->version && ((int *)buffer)[1] != descr->version)
			continue;
		break;
	}
	if (i == numModelFormats)
		Com_Error (ERR_DROP, "R_ModelForName: unknown fileId for %s", model->name);

	// Clear
	model->radius = 0;
	ClearBounds (model->mins, model->maxs);
	model->finishedLoading = qFalse;

	// Load
	descr->loader (model, buffer);

	// Done
	model->touchFrame = r_refRegInfo.registerFrame;
	model->memSize = Mem_TagSize (MEMPOOL_MODELSYS, model->memTag);

	model->finishedLoading = qTrue;
	model->isBspModel = (model->type == MODEL_Q2BSP || model->type == MODEL_Q3BSP);

	FS_FreeFile (buffer);

	R_TouchModel (model);
	return model;
}


/*
================
R_RegisterMap

Specifies the model that will be used as the world
================
*/
void R_RegisterMap (char *mapName)
{
	// Explicitly free the old map if different...
	// This guarantees that r_modelList[0] is the world map
	if (!r_modelList[0].finishedLoading
	|| strcmp (r_modelList[0].name, mapName)
	|| flushmap->integer) {
		R_FreeModel (&r_modelList[0]);
	}

	r_refScene.worldModel = R_ModelForName (mapName, qTrue);

	// Setup the world entity
	memcpy (&r_refScene.worldEntity, &r_refScene.defaultEntity, sizeof (entity_t));
	r_refScene.worldEntity.model = r_refScene.worldModel;

	// Force markleafs
	r_refScene.oldViewCluster = -1;
	r_refScene.viewCluster = -1;
	r_refScene.visFrameCount = 0;
	r_refScene.frameCount = 1;
}


/*
================
R_BeginModelRegistration

Releases models that were not fully loaded during the last registration sequence
================
*/
void R_BeginModelRegistration (void)
{
	model_t	*model;
	int		i;

	for (i=0, model=r_modelList ; i<r_numModels ; i++, model++) {
		if (!model->touchFrame)
			continue;	// Free model_t spot
		if (model->finishedLoading)
			continue;

		R_FreeModel (model);
	}
}


/*
================
R_EndModelRegistration
================
*/
void R_EndModelRegistration (void)
{
	model_t	*model;
	int		i;

	for (i=0, model=r_modelList ; i<r_numModels ; i++, model++) {
		if (!model->touchFrame)
			continue;	// Free model_t spot
		if (model->touchFrame == r_refRegInfo.registerFrame) {
			R_TouchModel (model);
			r_refRegInfo.modelsTouched++;
			continue;	// Used this sequence
		}

		R_FreeModel (model);
		r_refRegInfo.modelsReleased++;
	}
}


/*
================
R_RegisterModel

Load/re-register a model
================
*/
model_t *R_RegisterModel (char *name)
{
	model_t		*model;
	char		fixedName[MAX_QPATH];
	int			len;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// Check the length
	len = strlen (name);
	if (len+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_RegisterModel: Model name too long! %s\n", name);
		return NULL;
	}

	// Copy and normalize
	Com_NormalizePath (fixedName, sizeof (fixedName), name);
	Q_strlwr (fixedName);

	// MD2 deprecation
	if (fixedName[0] != '*' && !strcmp (fixedName + strlen (fixedName) - 4, ".md2")) {
		char	temp[MAX_QPATH];

		Q_strncpyz (temp, fixedName, sizeof (temp));
		temp[strlen (temp)-1] = '3';
		model = R_RegisterModel (temp);

		if (model)
			return model;
	}

	// Find or load it
	model = R_ModelForName (fixedName, qFalse);
	return model;
}

/*
===============================================================================

	MISCELLANEOUS

===============================================================================
*/

/*
================
R_ModelList_f
================
*/
static void R_ModelList_f (void)
{
	model_t	*mod;
	int		i, totalBytes;

	Com_Printf (0, "Loaded models:\n");

	totalBytes = 0;
	for (i=0, mod=r_modelList ; i<r_numModels ; i++, mod++) {
		if (!mod->name[0])
			continue;

		switch (mod->type) {
		case MODEL_MD2:		Com_Printf (0, "MD2  ");			break;
		case MODEL_MD3:		Com_Printf (0, "MD3  ");			break;
		case MODEL_Q2BSP:	Com_Printf (0, "Q2BSP");			break;
		case MODEL_Q3BSP:	Com_Printf (0, "Q3BSP");			break;
		case MODEL_SP2:		Com_Printf (0, "SP2  ");			break;
		default:			Com_Printf (PRNT_ERROR, "BAD");	break;
		}

		Com_Printf (0, " %7iB (%2.2fMB) ", mod->memSize, mod->memSize/1048576.0);
		Com_Printf (0, "%s\n", mod->name);

		totalBytes += mod->memSize;
	}

	Com_Printf (0, "%i model(s) loaded, %i bytes (%0.2fMB) total\n", i, totalBytes, totalBytes/1048576.0);
}


/*
=================
R_ModelBounds
=================
*/
void R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs)
{
	if (model) {
		VectorCopy (model->mins, mins);
		VectorCopy (model->maxs, maxs);
	}
}

/*
===============================================================================

	INIT / SHUTDOWN

===============================================================================
*/

static void	*cmd_modelList;

/*
===============
R_ModelInit
===============
*/
void R_ModelInit (void)
{
	// Register commands/cvars
	flushmap	= Cvar_Register ("flushmap",		"0",		0);

	cmd_modelList = Cmd_AddCommand ("modellist",	R_ModelList_f,		"Prints to the console a list of loaded models and their sizes");

	memset (r_q2BspNoVis, 0xff, sizeof (r_q2BspNoVis));
	memset (r_q3BspNoVis, 0xff, sizeof (r_q3BspNoVis));

	memset (&r_inlineModels, 0, sizeof (r_inlineModels));
	memset (&r_modelList, 0, sizeof (r_modelList));

	r_refScene.worldModel = &r_modelList[0];
	r_numModels = 0;
}


/*
==================
R_ModelShutdown
==================
*/
void R_ModelShutdown (void)
{
	int		i;

	// Remove commands
	Cmd_RemoveCommand ("modellist", cmd_modelList);

	// Free known loaded models
	for (i=0 ; i<r_numModels ; i++)
		R_FreeModel (&r_modelList[i]);

	memset (&r_inlineModels, 0, sizeof (r_inlineModels));
	memset (&r_modelList, 0, sizeof (r_modelList));
}
