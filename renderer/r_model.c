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

static byte		r_bspNoVis[BSP_MAX_VIS];

cVar_t	*flushmap;

// ============================================================================

/*
=================
_R_ModAlloc
=================
*/
#define R_ModAlloc(model,size) _R_ModAlloc((model),(size),__FILE__,__LINE__)
static void *_R_ModAlloc (model_t *model, size_t size, char *fileName, int fileLine)
{
	return _Mem_PoolAlloc (size, MEMPOOL_MODELSYS, model->memTag, fileName, fileLine);
}


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

	Com_Printf (PRNT_DEV, "R_LoadMD2Model: '%s' remapped %i verts to %i (%i tris)\n",
							model->name, outMesh->numVerts, numVerts, outMesh->numTris);
	outMesh->numVerts = numVerts;

	//
	// Remap remaining indexes
	//
	for (i=0 ; i<numIndexes; i++) {
		if (indRemap[i] != i)
			outIndex[i] = outIndex[indRemap[i]];
	}

	//
	// Load base s and t vertices
	//
	outCoord = outMesh->coords = R_ModAlloc (model, sizeof (vec2_t) * numVerts);

	for (j=0 ; j<numIndexes ; j++) {
		outCoord[outIndex[j]][0] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].s) + 0.5) * isw);
		outCoord[outIndex[j]][1] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].t) + 0.5) * ish);
	}

	//
	// Load the frames
	//
#ifdef SHADOW_VOLUMES
	allocBuffer = R_ModAlloc (model, (sizeof (mAliasFrame_t) * outModel->numFrames)
									+ (sizeof (mAliasVertex_t) * outModel->numFrames * numVerts)
									+ (sizeof (int) * outMesh->numTris * 3)
									+ (sizeof (mAliasSkin_t) * outMesh->numSkins));
#else
	allocBuffer = R_ModAlloc (model, (sizeof (mAliasFrame_t) * outModel->numFrames)
									+ (sizeof (mAliasVertex_t) * outModel->numFrames * numVerts)
									+ (sizeof (mAliasSkin_t) * outMesh->numSkins));
#endif
	outFrame = outModel->frames = (mAliasFrame_t *)allocBuffer;

	allocBuffer += sizeof (mAliasFrame_t) * outModel->numFrames;
	outVertex = outMesh->vertexes = (mAliasVertex_t *)allocBuffer;

	for (i=0 ; i<outModel->numFrames; i++, outFrame++, outVertex += numVerts) {
		inFrame = (dMd2Frame_t *) ((byte *)inModel + LittleLong (inModel->ofsFrames) + i * frameSize);

		outFrame->scale[0] = LittleFloat (inFrame->scale[0]);
		outFrame->scale[1] = LittleFloat (inFrame->scale[1]);
		outFrame->scale[2] = LittleFloat (inFrame->scale[2]);

		outFrame->translate[0] = LittleFloat (inFrame->translate[0]);
		outFrame->translate[1] = LittleFloat (inFrame->translate[1]);
		outFrame->translate[2] = LittleFloat (inFrame->translate[2]);

		VectorCopy (outFrame->translate, outFrame->mins);
		VectorMA (outFrame->translate, 255, outFrame->scale, outFrame->maxs);
		outFrame->radius = RadiusFromBounds (outFrame->mins, outFrame->maxs);

		model->radius = max (model->radius, outFrame->radius);
		AddPointToBounds (outFrame->mins, model->mins, model->maxs);
		AddPointToBounds (outFrame->maxs, model->mins, model->maxs);

		//
		// Load vertices and normals
		//
		for (j=0 ; j<numIndexes ; j++) {
			outVertex[outIndex[j]].point[0] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[0];
			outVertex[outIndex[j]].point[1] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[1];
			outVertex[outIndex[j]].point[2] = (short)inFrame->verts[tempIndex[indRemap[j]]].v[2];

			ByteToDir (inFrame->verts[tempIndex[indRemap[j]]].normalIndex, normal);
			NormToLatLong (normal, outVertex[outIndex[j]].latLong);
		}
	}

	//
	// Build a list of neighbors
	//
#ifdef SHADOW_VOLUMES
	allocBuffer += sizeof (mAliasVertex_t) * outModel->numFrames * numVerts;
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
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadMD2Model: '%s' could not load skin '%s'\n",
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
	if ((outModel->numFrames <= 0) || (outModel->numFrames > MD3_MAX_FRAMES))
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	outModel->numTags = LittleLong (inModel->numTags);
	if ((outModel->numTags < 0) || (outModel->numTags > MD3_MAX_TAGS))
		Com_Error (ERR_DROP, "R_LoadMD3Model: model '%s' has an invalid amount of tags '%d'",
							model->name, outModel->numTags);

	outModel->numMeshes = LittleLong (inModel->numMeshes);
	if ((outModel->numMeshes < 0) || (outModel->numMeshes > MD3_MAX_MESHES))
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
				vec3_t axis[3];

				axis[0][j] = LittleFloat (inTag->axis[0][j]);
				axis[1][j] = LittleFloat (inTag->axis[1][j]);
				axis[2][j] = LittleFloat (inTag->axis[2][j]);
				Matrix_Quat (axis, outTag->quat);
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
										+ (sizeof(int) * outMesh->numTris * 3)
										);
#else
		allocBuffer = R_ModAlloc (model, (sizeof (mAliasSkin_t) * outMesh->numSkins)
										+ (sizeof (index_t) * outMesh->numTris * 3)
										+ (sizeof (vec2_t) * outMesh->numVerts)
										+ (sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts)
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
				Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadMD3Model: '%s' could not load skin '%s' on mesh '%s'\n",
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
		//
		allocBuffer += sizeof (vec2_t) * outMesh->numVerts;
		inVert = (dMd3Vertex_t *)((byte *)inMesh + LittleLong (inMesh->ofsVerts));
		outVert = outMesh->vertexes = (mAliasVertex_t *)allocBuffer;
		outFrame = outModel->frames;

		for (l=0 ; l<outModel->numFrames ; l++, outFrame++) {
			vec3_t	v;

			for (j=0 ; j<outMesh->numVerts ; j++, inVert++, outVert++) {
				outVert->point[0] = LittleShort (inVert->point[0]);
				outVert->point[1] = LittleShort (inVert->point[1]);
				outVert->point[2] = LittleShort (inVert->point[2]);

				VectorCopy (outVert->point, v);
				AddPointToBounds (v, outFrame->mins, outFrame->maxs);

				outVert->latLong[0] = inVert->norm[0] & 0xff;
				outVert->latLong[1] = inVert->norm[1] & 0xff;
			}
		}

		//
		// Build a list of neighbors
		//
#ifdef SHADOW_VOLUMES
		allocBuffer += sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts;
		outMesh->neighbors = (int *)allocBuffer;
		R_BuildTriangleNeighbors (outMesh->neighbors, outMesh->indexes, outMesh->numTris);
#endif SHADOW_VOLUMES

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
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadSP2Model: '%s' could not load skin '%s'\n",
							model->name, outFrames->name);
	}
}

/*
===============================================================================

	BRUSH MODELS

===============================================================================
*/

static void BoundPoly (int numVerts, float *verts, vec3_t mins, vec3_t maxs)
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
R_SubdivideBSPSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdividePolygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
{
	int			i, j;
	vec3_t		mins, maxs;
	float		m;
	float		*v;
	vec3_t		front[64], back[64];
	int			f, b;
	float		dist[64];
	float		frac;
	mBspPoly_t	*poly;
	vec3_t		posTotal;
	vec3_t		normalTotal;
	vec2_t		coordTotal;
	float		oneDivVerts;
	byte		*buffer;

	if (numVerts > 60)
		Com_Error (ERR_DROP, "SubdividePolygon: numVerts = %i", numVerts);

	BoundPoly (numVerts, verts, mins, maxs);

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

		SubdividePolygon (model, surf, f, front[0], subdivideSize);
		SubdividePolygon (model, surf, b, back[0], subdivideSize);
		return;
	}

	// Add a point in the center to help keep warp valid
	buffer = R_ModAlloc (model, sizeof (mBspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t)));
	poly = (mBspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.lmCoordArray = NULL;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mBspPoly_t);
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
		VectorCopy (surf->plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coords
		poly->mesh.coordArray[i+1][0] = DotProduct (verts, surf->texInfo->vecs[0]) * (1.0f/64.0f);
		poly->mesh.coordArray[i+1][1] = DotProduct (verts, surf->texInfo->vecs[1]) * (1.0f/64.0f);

		// For the center point
		VectorAdd (posTotal, verts, posTotal);
		VectorAdd (normalTotal, surf->plane->normal, normalTotal);
		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];

		verts += 3;
	}

	// Center
	oneDivVerts = (1.0f/(float)numVerts);
	Vector4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	VectorScale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	VectorScale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);

	// Copy first vertex to last
	Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	memcpy (poly->mesh.vertexArray[i+1], poly->mesh.vertexArray[1], sizeof (poly->mesh.vertexArray[i+1]));
	memcpy (poly->mesh.normalsArray[i+1], poly->mesh.normalsArray[1], sizeof (poly->mesh.normalsArray[i+1]));
	memcpy (poly->mesh.coordArray[i+1], poly->mesh.coordArray[1], sizeof (poly->mesh.coordArray[i+1]));

	// Link it in
	poly->next = surf->polys;
	surf->polys = poly;
}
static void R_SubdivideBSPSurface (model_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	index_t		index;
	float		*vec;

	// Convert edges back to a normal polygon
	numVerts = 0;
	for (i=0 ; i<surf->numEdges ; i++) {
		index = model->surfEdges[surf->firstEdge + i];

		if (index > 0)
			vec = model->vertexes[model->edges[index].v[0]].position;
		else
			vec = model->vertexes[model->edges[-index].v[1]].position;
		VectorCopy (vec, verts[numVerts]);
		numVerts++;
	}

	SubdividePolygon (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_SubdivideLightmapBSPSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdivideLightmappedPolygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
{
	int			i, j;
	vec3_t		mins, maxs;
	float		m;
	float		*v;
	vec3_t		front[64], back[64];
	int			f, b;
	float		dist[64];
	float		frac;
	mBspPoly_t	*poly;
	float		s, t;
	vec3_t		posTotal;
	vec3_t		normalTotal;
	vec2_t		coordTotal;
	vec2_t		lmCoordTotal;
	float		oneDivVerts;
	byte		*buffer;
	int			tcWidth, tcHeight;

	if (numVerts > 60)
		Sys_Error ("SubdivideLightmappedPolygon: numVerts = %i", numVerts);

	BoundPoly (numVerts, verts, mins, maxs);

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

		SubdivideLightmappedPolygon (model, surf, f, front[0], subdivideSize);
		SubdivideLightmappedPolygon (model, surf, b, back[0], subdivideSize);
		return;
	}

	if (surf->texInfo->shader) {
		tcWidth = surf->texInfo->shader->passes[surf->texInfo->shader->sizeBase].animFrames[0]->tcWidth;
		tcHeight = surf->texInfo->shader->passes[surf->texInfo->shader->sizeBase].animFrames[0]->tcHeight;
	}
	else {
		tcWidth = 64;
		tcHeight = 64;
	}

	// Add a point in the center to help keep warp valid
	buffer = R_ModAlloc (model, sizeof (mBspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t) * 2));
	poly = (mBspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mBspPoly_t);
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
		VectorCopy (surf->plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coordinates
		poly->mesh.coordArray[i+1][0] = (DotProduct (verts, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / tcWidth;
		poly->mesh.coordArray[i+1][1] = (DotProduct (verts, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / tcHeight;

		// Lightmap texture coordinates
		s = DotProduct (verts, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3] - surf->textureMins[0];
		poly->mesh.lmCoordArray[i+1][0] = (s + 8 + (surf->lmCoords[0] * 16)) / (LIGHTMAP_SIZE * 16);

		t = DotProduct (verts, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3] - surf->textureMins[1];
		poly->mesh.lmCoordArray[i+1][1] = (t + 8 + (surf->lmCoords[1] * 16)) / (LIGHTMAP_SIZE * 16);

		// For the center point
		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];
		VectorAdd (posTotal, verts, posTotal);

		lmCoordTotal[0] += poly->mesh.lmCoordArray[i+1][0];
		lmCoordTotal[1] += poly->mesh.lmCoordArray[i+1][1];

		verts += 3;
	}

	// Center point
	oneDivVerts = (1.0f/(float)numVerts);
	Vector4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	VectorScale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	VectorScale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);
	Vector2Scale (lmCoordTotal, oneDivVerts, poly->mesh.lmCoordArray[0]);

	// Copy first vertex to last
	Vector4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	memcpy (poly->mesh.vertexArray[i+1], poly->mesh.vertexArray[1], sizeof (poly->mesh.vertexArray[i+1]));
	memcpy (poly->mesh.normalsArray[i+1], poly->mesh.normalsArray[1], sizeof (poly->mesh.normalsArray[i+1]));
	memcpy (poly->mesh.coordArray[i+1], poly->mesh.coordArray[1], sizeof (poly->mesh.coordArray[i+1]));
	memcpy (poly->mesh.lmCoordArray[i+1], poly->mesh.lmCoordArray[1], sizeof (poly->mesh.lmCoordArray[i+1]));

	// Link it in
	poly->next = surf->polys;
	surf->polys = poly;
}

static void R_SubdivideLightmapBSPSurface (model_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	int			index;
	float		*vec;

	// Convert edges back to a normal polygon
	numVerts = 0;
	for (i=0 ; i<surf->numEdges ; i++) {
		index = model->surfEdges[surf->firstEdge + i];

		if (index > 0)
			vec = model->vertexes[model->edges[index].v[0]].position;
		else
			vec = model->vertexes[model->edges[-index].v[1]].position;
		VectorCopy (vec, verts[numVerts]);
		numVerts++;
	}

	SubdivideLightmappedPolygon (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_BuildPolygonFromBSPSurface
================
*/
static void R_BuildPolygonFromBSPSurface (model_t *model, mBspSurface_t *surf)
{
	int			i, numVerts;
	mBspEdge_t	*pedges, *r_pedge;
	float		*vec, s, t;
	mBspPoly_t	*poly;
	index_t		index;
	byte		*buffer;
	int			tcWidth, tcHeight;

	if (surf->texInfo->shader) {
		tcWidth = surf->texInfo->shader->passes[surf->texInfo->shader->sizeBase].animFrames[0]->tcWidth;
		tcHeight = surf->texInfo->shader->passes[surf->texInfo->shader->sizeBase].animFrames[0]->tcHeight;
	}
	else {
		tcWidth = 64;
		tcHeight = 64;
	}

	pedges = model->edges;
	numVerts = surf->numEdges;

	// Allocate
	buffer = R_ModAlloc (model, sizeof (mBspPoly_t) + (numVerts * sizeof (bvec4_t)) + (numVerts * sizeof (vec3_t) * 2) + (numVerts * sizeof (vec2_t) * 2));
	poly = (mBspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts;
	poly->mesh.numIndexes = (numVerts-2)*3;

	poly->mesh.indexArray = rb_triFanIndices;
	poly->mesh.sVectorsArray = NULL;
	poly->mesh.tVectorsArray = NULL;
	poly->mesh.trNeighborsArray = NULL;
	poly->mesh.trNormalsArray = NULL;

	buffer += sizeof (mBspPoly_t);
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
		index = model->surfEdges[surf->firstEdge + i];

		if (index > 0) {
			r_pedge = &pedges[index];
			vec = model->vertexes[r_pedge->v[0]].position;
		}
		else {
			r_pedge = &pedges[-index];
			vec = model->vertexes[r_pedge->v[1]].position;
		}

		VectorCopy (vec, poly->mesh.vertexArray[i]);

		// Normal
		VectorCopy (surf->plane->normal, poly->mesh.normalsArray[i]);

		// Texture coordinates
		poly->mesh.coordArray[i][0] = (DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / tcWidth;
		poly->mesh.coordArray[i][1] = (DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / tcHeight;

		// Lightmap texture coordinates
		s = DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3] - surf->textureMins[0];
		poly->mesh.lmCoordArray[i][0] = (s + 8 + (surf->lmCoords[0] * 16)) / (LIGHTMAP_SIZE * 16);

		t = DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3] - surf->textureMins[1];
		poly->mesh.lmCoordArray[i][1] = (t + 8 + (surf->lmCoords[1] * 16)) / (LIGHTMAP_SIZE * 16);
	}

	// Link it in
	poly->next = surf->polys;
	surf->polys = poly;
}


/*
================
R_CalcBSPSurfaceBounds

Fills in s->mins and s->maxs
================
*/
static void R_CalcBSPSurfaceBounds (model_t *model, mBspSurface_t *surf)
{
	mBspVertex_t	*v;
	int				i, e;

	ClearBounds (surf->mins, surf->maxs);

	for (i=0 ; i<surf->numEdges ; i++) {
		e = model->surfEdges[surf->firstEdge + i];
		if (e >= 0)
			v = &model->vertexes[model->edges[e].v[0]];
		else
			v = &model->vertexes[model->edges[-e].v[1]];

		AddPointToBounds (v->position, surf->mins, surf->maxs);
	}
}

/*
================
R_CalcBSPSurfaceExtents

Fills in s->textureMins[] and s->extents[]
================
*/
static void R_CalcBSPSurfaceExtents (model_t *model, mBspSurface_t *surf)
{
	float			mins[2], maxs[2], val;
	int				i, j, e;
	mBspVertex_t	*v;
	mBspTexInfo_t	*tex;
	int				bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = surf->texInfo;
	
	for (i=0 ; i<surf->numEdges ; i++) {
		e = model->surfEdges[surf->firstEdge+i];
		v = (e >= 0) ? &model->vertexes[model->edges[e].v[0]] : &model->vertexes[model->edges[-e].v[1]];
		
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

		surf->textureMins[i] = bmins[i] * 16;
		surf->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


/*
===================
R_DecompressBSPVis
===================
*/
static byte *R_DecompressBSPVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP_MAX_VIS];
	int			c, row;
	byte		*out;

	row = (model->vis->numClusters+7)>>3;	
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
==============
R_BSPClusterPVS
==============
*/
byte *R_BSPClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return r_bspNoVis;

	return R_DecompressBSPVis ((byte *)model->vis + model->vis->bitOfs[cluster][BSP_VIS_PVS], model);
}


/*
===============
R_PointInBSPLeaf
===============
*/
mBspLeaf_t *R_PointInBSPLeaf (vec3_t point, model_t *model)
{
	cBspPlane_t	*plane;
	mBspNode_t	*node;
	float		d;
	
	if (!model || !model->nodes)
		Com_Error (ERR_DROP, "R_PointInBSPLeaf: bad model");

	node = model->nodes;
	for ( ; ; ) {
		if (node->contents != -1)
			return (mBspLeaf_t *)node;

		plane = node->plane;
		d = DotProduct (point, plane->normal) - plane->dist;
		node = (d > 0) ? node->children[0] : node->children[1];
	}
}


/*
=================
R_SetParentBSPNode
=================
*/
static void R_SetParentBSPNode (mBspNode_t *node, mBspNode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;

	R_SetParentBSPNode (node->children[0], node);
	R_SetParentBSPNode (node->children[1], node);
}

/*
===============================================================================

	BRUSH MODEL LOADING

===============================================================================
*/

/*
=================
R_LoadBSPVertexes
=================
*/
void R_LoadBSPVertexes (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspVertex_t	*in;
	mBspVertex_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPVertexes: funny lump size in %s", model->name);

	model->numVertexes = lump->fileLen / sizeof (*in);
	model->vertexes = out = R_ModAlloc (model, sizeof (*out) * model->numVertexes);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numVertexes ; i++, in++, out++) {
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}


/*
=================
R_LoadBSPEdges
=================
*/
void R_LoadBSPEdges (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspEdge_t	*in;
	mBspEdge_t	*out;
	int			i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPEdges: funny lump size in %s", model->name);

	model->numEdges = lump->fileLen / sizeof (*in);
	model->edges = out = R_ModAlloc (model, sizeof (*out) * (model->numEdges + 1));

	//
	// Byte swap
	//
	for (i=0 ; i<model->numEdges ; i++, in++, out++) {
		out->v[0] = (uShort) LittleShort (in->v[0]);
		out->v[1] = (uShort) LittleShort (in->v[1]);
	}
}


/*
=================
R_LoadBSPSurfEdges
=================
*/
void R_LoadBSPSurfEdges (model_t *model, byte *byteBase, dBspLump_t *lump)
{	
	int		*in;
	int		*out;
	int		i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPSurfEdges: funny lump size in %s", model->name);

	model->numSurfEdges = lump->fileLen / sizeof (*in);
	if ((model->numSurfEdges < 1) || (model->numSurfEdges >= BSP_MAX_SURFEDGES))
		Com_Error (ERR_DROP, "R_LoadBSPSurfEdges: invalid surfEdges count in %s: %i (min: 1; max: %d)", model->name, model->numSurfEdges, BSP_MAX_SURFEDGES);

	model->surfEdges = out = R_ModAlloc (model, sizeof (*out) * model->numSurfEdges);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numSurfEdges ; i++) {
		out[i] = LittleLong (in[i]);
	}
}


/*
=================
R_LoadBSPLighting
=================
*/
void R_LoadBSPLighting (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	if (!lump->fileLen) {
		model->lightData = NULL;
		return;
	}

	model->lightData = R_ModAlloc (model, lump->fileLen);	
	memcpy (model->lightData, byteBase + lump->fileOfs, lump->fileLen);
}


/*
=================
R_LoadBSPPlanes
=================
*/
void R_LoadBSPPlanes (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	int				i, j;
	cBspPlane_t		*out;
	dBspPlane_t		*in;
	int				bits;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPPlanes: funny lump size in %s", model->name);

	model->numPlanes = lump->fileLen / sizeof (*in);
	model->planes = out = R_ModAlloc (model, sizeof (*out) * model->numPlanes * 2);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numPlanes ; i++, in++, out++) {
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
R_LoadBSPTexInfo
=================
*/
void R_LoadBSPTexInfo (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspTexInfo_t	*in;
	mBspTexInfo_t	*out, *step;
	int				i, next;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPTexInfo: funny lump size in %s", model->name);

	model->numTexInfo = lump->fileLen / sizeof (*in);
	model->texInfo = out = R_ModAlloc (model, sizeof (*out) * model->numTexInfo);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numTexInfo ; i++, in++, out++) {
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
		out->next = (next > 0) ? model->texInfo + next : NULL;

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
			out->shader = r_noShader;
		}
		else {
			Q_snprintfz (out->texName, sizeof (out->texName), "textures/%s.wal", in->texture);
			out->shader = R_RegisterTexture (out->texName, out->surfParams);
			if (!out->shader) {
				Com_Printf (0, S_COLOR_YELLOW "Couldn't load %s\n", out->texName);

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
	for (i=0 ; i<model->numTexInfo ; i++) {
		out = &model->texInfo[i];
		out->numFrames = 1;
		for (step=out->next ; step && step != out ; step=step->next) {
			out->numFrames++;
		}
	}
}


/*
=================
R_LoadBSPFaces
=================
*/
void R_LoadBSPFaces (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspSurface_t	*in;
	mBspSurface_t	*out;
	int				i, surfNum;
	int				planeNum, side;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPFaces: funny lump size in %s", model->name);

	model->numSurfaces = lump->fileLen / sizeof (*in);
	model->surfaces = out= R_ModAlloc (model, sizeof (*out) * model->numSurfaces);

	LM_BeginBuildingLightmaps ();

	//
	// Byte swap
	//
	for (surfNum=0 ; surfNum<model->numSurfaces ; surfNum++, in++, out++) {
		out->firstEdge = LittleLong (in->firstEdge);
		out->numEdges = LittleShort (in->numEdges);		
		out->flags = 0;
		out->polys = NULL;

		planeNum = LittleShort (in->planeNum);
		side = LittleShort (in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;			

		out->plane = model->planes + planeNum;

		i = LittleShort (in->texInfo);
		if (i < 0 || i >= model->numTexInfo)
			Com_Error (ERR_DROP, "R_LoadBSPFaces: bad texInfo number");
		out->texInfo = model->texInfo + i;

		R_CalcBSPSurfaceBounds (model, out);
		R_CalcBSPSurfaceExtents (model, out);

		//
		// Lighting info
		//
		out->lmWidth = (out->extents[0]>>4) + 1;
		out->lmHeight = (out->extents[1]>>4) + 1;

		i = LittleLong (in->lightOfs);
		out->lmSamples = (i == -1) ? NULL : model->lightData + i;

		for (out->numStyles=0 ; out->numStyles<BSP_MAX_LIGHTMAPS && in->styles[out->numStyles] != 255 ; out->numStyles++) {
			out->styles[out->numStyles] = in->styles[out->numStyles];
		}


		//
		// Create lightmaps and polygons
		//
		if (out->texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
			if (out->texInfo->flags & SURF_TEXINFO_WARP) {
				out->extents[0] = out->extents[1] = 16384;
				out->textureMins[0] = out->textureMins[1] = -8192;
			}

			// WARP surfaces have no lightmap
			if (out->texInfo->shader && out->texInfo->shader->flags & SHADER_SUBDIVIDE)
				R_SubdivideBSPSurface (model, out, out->texInfo->shader->subdivide);
			else
				R_BuildPolygonFromBSPSurface (model, out);
		}
		else {
			// The rest do
			LM_CreateSurfaceLightmap (out);

			if (out->texInfo->shader && out->texInfo->shader->flags & SHADER_SUBDIVIDE)
				R_SubdivideLightmapBSPSurface (model, out, out->texInfo->shader->subdivide);
			else
				R_BuildPolygonFromBSPSurface (model, out);
		}
	}

	LM_EndBuildingLightmaps ();
}


/*
=================
R_LoadBSPMarkSurfaces
=================
*/
void R_LoadBSPMarkSurfaces (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	mBspSurface_t	**out;
	int				i, j;
	short			*in;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPMarkSurfaces: funny lump size in %s", model->name);

	model->numMarkSurfaces = lump->fileLen / sizeof (*in);
	model->markSurfaces = out = R_ModAlloc (model, sizeof (*out) * model->numMarkSurfaces);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numMarkSurfaces ; i++) {
		j = LittleShort (in[i]);
		if ((j < 0) || (j >= model->numSurfaces))
			Com_Error (ERR_DROP, "R_LoadBSPMarkSurfaces: bad surface number");
		out[i] = model->surfaces + j;
	}
}


/*
=================
R_LoadBSPVisibility
=================
*/
void R_LoadBSPVisibility (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	int		i;

	if (!lump->fileLen) {
		model->vis = NULL;
		return;
	}

	model->vis = R_ModAlloc (model, lump->fileLen);	
	memcpy (model->vis, byteBase + lump->fileOfs, lump->fileLen);

	model->vis->numClusters = LittleLong (model->vis->numClusters);

	//
	// Byte swap
	//
	for (i=0 ; i<model->vis->numClusters ; i++) {
		model->vis->bitOfs[i][0] = LittleLong (model->vis->bitOfs[i][0]);
		model->vis->bitOfs[i][1] = LittleLong (model->vis->bitOfs[i][1]);
	}
}


/*
=================
R_LoadBSPLeafs
=================
*/
void R_LoadBSPLeafs (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspLeaf_t	*in;
	mBspLeaf_t	*out;
	int			i, j;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPLeafs: funny lump size in %s", model->name);

	model->numLeafs = lump->fileLen / sizeof (*in);
	model->leafs = out = R_ModAlloc (model, sizeof (*out) * model->numLeafs);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numLeafs ; i++, in++, out++) {
		out->mins[0] = LittleShort (in->mins[0]);
		out->mins[1] = LittleShort (in->mins[1]);
		out->mins[2] = LittleShort (in->mins[2]);

		out->maxs[0] = LittleShort (in->maxs[0]);
		out->maxs[1] = LittleShort (in->maxs[1]);
		out->maxs[2] = LittleShort (in->maxs[2]);

		out->contents = LittleLong (in->contents);

		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);

		out->firstMarkSurface = model->markSurfaces + LittleShort (in->firstLeafFace);
		out->numMarkSurfaces = LittleShort (in->numLeafFaces);

		// Mark poly flags
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			for (j=0 ; j<out->numMarkSurfaces ; j++) {
				out->firstMarkSurface[j]->flags |= SURF_UNDERWATER;

				if (out->contents & CONTENTS_LAVA)
					out->firstMarkSurface[j]->flags |= SURF_LAVA;
				if (out->contents & CONTENTS_SLIME)
					out->firstMarkSurface[j]->flags |= SURF_SLIME;
				if (out->contents & CONTENTS_WATER)
					out->firstMarkSurface[j]->flags |= SURF_WATER;
			}
		}
	}	
}


/*
=================
R_LoadBSPNodes
=================
*/
void R_LoadBSPNodes (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	int			i, p;
	dBspNode_t	*in;
	mBspNode_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPNodes: funny lump size in %s", model->name);

	model->numNodes = lump->fileLen / sizeof (*in);
	model->nodes = out= R_ModAlloc (model, sizeof (*out) * model->numNodes);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numNodes ; i++, in++, out++) {
		out->mins[0] = LittleShort (in->mins[0]);
		out->mins[1] = LittleShort (in->mins[1]);
		out->mins[2] = LittleShort (in->mins[2]);

		out->maxs[0] = LittleShort (in->maxs[0]);
		out->maxs[1] = LittleShort (in->maxs[1]);
		out->maxs[2] = LittleShort (in->maxs[2]);
	
		out->contents = -1;	// Differentiate from leafs

		out->plane = model->planes + LittleLong (in->planeNum);
		out->firstSurface = LittleShort (in->firstFace);
		out->numSurfaces = LittleShort (in->numFaces);

		p = LittleLong (in->children[0]);
		out->children[0] = (p >= 0) ? model->nodes + p : (mBspNode_t *)(model->leafs + (-1 - p));

		p = LittleLong (in->children[1]);
		out->children[1] = (p >= 0) ? model->nodes + p : (mBspNode_t *)(model->leafs + (-1 - p));
	}

	//
	// Set the nodes and leafs
	//
	R_SetParentBSPNode (model->nodes, NULL);
}


/*
=================
R_LoadBSPSubModels
=================
*/
void R_LoadBSPSubModels (model_t *model, byte *byteBase, dBspLump_t *lump)
{
	dBspModel_t		*in;
	mBspHeader_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "R_LoadBSPSubModels: funny lump size in %s", model->name);

	model->numSubModels = lump->fileLen / sizeof (*in);
	model->subModels = out = R_ModAlloc (model, sizeof (*out) * model->numSubModels);

	//
	// Byte swap
	//
	for (i=0 ; i<model->numSubModels ; i++, in++, out++) {
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
R_LoadBSPModel
=================
*/
void R_LoadBSPModel (model_t *model, byte *buffer)
{
	dBspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	int				version;
	int				i;

	//
	// Load the world model
	//
	model->type = MODEL_BSP;
	if (model != r_modelList)
		Com_Error (ERR_DROP, "R_LoadBSPModel: Loaded a model before the world (\"%s\")", r_modelList[0].name);

	header = (dBspHeader_t *)buffer;

	version = LittleLong (header->version);
	if (version != BSP_VERSION)
		Com_Error (ERR_DROP, "R_LoadBSPModel: %s has wrong version number (%i should be %i)", model->name, version, BSP_VERSION);

	r_worldModel = model;

	//
	// Swap all the lumps
	//
	modBase = (byte *)header;

	for (i=0 ; i<sizeof (dBspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// Load into heap
	//

	R_LoadBSPVertexes		(model, modBase, &header->lumps[LUMP_VERTEXES]);
	R_LoadBSPEdges			(model, modBase, &header->lumps[LUMP_EDGES]);
	R_LoadBSPSurfEdges		(model, modBase, &header->lumps[LUMP_SURFEDGES]);
	R_LoadBSPLighting		(model, modBase, &header->lumps[LUMP_LIGHTING]);
	R_LoadBSPPlanes			(model, modBase, &header->lumps[LUMP_PLANES]);
	R_LoadBSPTexInfo		(model, modBase, &header->lumps[LUMP_TEXINFO]);
	R_LoadBSPFaces			(model, modBase, &header->lumps[LUMP_FACES]);
	R_LoadBSPMarkSurfaces	(model, modBase, &header->lumps[LUMP_LEAFFACES]);
	R_LoadBSPVisibility		(model, modBase, &header->lumps[LUMP_VISIBILITY]);
	R_LoadBSPLeafs			(model, modBase, &header->lumps[LUMP_LEAFS]);
	R_LoadBSPNodes			(model, modBase, &header->lumps[LUMP_NODES]);
	R_LoadBSPSubModels		(model, modBase, &header->lumps[LUMP_MODELS]);

	//
	// Set up the submodels
	//
	for (i=0 ; i<model->numSubModels ; i++) {
		model_t		*starmodel;

		bm = &model->subModels[i];
		starmodel = &r_inlineModels[i];

		*starmodel = *model;

		starmodel->firstModelSurface = bm->firstFace;
		starmodel->numModelSurfaces = bm->numFaces;
		starmodel->firstNode = bm->headNode;

		if (starmodel->firstNode >= model->numNodes)
			Com_Error (ERR_DROP, "R_LoadBSPModel: Inline model number '%i' has a bad firstNode ( %d >= %d )", i, starmodel->firstNode, model->numNodes);

		VectorCopy (bm->maxs, starmodel->maxs);
		VectorCopy (bm->mins, starmodel->mins);
		starmodel->radius = bm->radius;

		if (i == 0)
			*model = *starmodel;

		starmodel->numLeafs = bm->visLeafs;
	}
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
	if (!model || model->memSize <= 0)
		return;

	// free it
	Mem_FreeTags (MEMPOOL_MODELSYS, model->memTag);
	memset (model, 0, sizeof (*model));
}


/*
================
R_TouchModel

Touches/loads all textures for the model type
================
*/
static void R_TouchModel (model_t *model)
{
	mAliasModel_t	*aliasModel;
	mAliasMesh_t	aliasMesh;
	mAliasSkin_t	*aliasSkin;
	mBspTexInfo_t	*texInfo;
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	int				skinNum, i;

	model->touchFrame = r_registrationFrame;

	switch (model->type) {
	case MODEL_MD2:
	case MODEL_MD3:
		aliasModel = model->aliasModel;
		for (i=0; i<aliasModel->numMeshes; i++) {
			aliasMesh = aliasModel->meshes[i];
			aliasSkin = aliasMesh.skins;
			for (skinNum=0 ; skinNum<aliasMesh.numSkins ; skinNum++) {
				if (!aliasSkin[skinNum].name[0]) {
					aliasSkin[skinNum].skin = NULL;
					continue;
				}

				aliasSkin[skinNum].skin = R_RegisterSkin (aliasSkin[skinNum].name);
			}
		}
		break;

	case MODEL_BSP:
		for (i=0, texInfo=model->texInfo ; i<model->numTexInfo ; texInfo++, i++) {
			texInfo->shader = R_RegisterTexture (texInfo->texName, texInfo->surfParams);
			if (!texInfo->shader) {
				if (texInfo->surfParams & SHADER_SURF_LIGHTMAP)
					texInfo->shader = r_noShaderLightmap;
				else
					texInfo->shader = r_noShader;
			}
		}
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

	// Inline models are grabbed only from worldmodel
	if (name[0] == '*') {
		i = atoi (name+1);
		if (i < 1 || !r_worldModel || i >= r_worldModel->numSubModels)
			Com_Error (ERR_DROP, "Bad inline model number '%d'", i);
		model = &r_inlineModels[i];
		model->memTag = r_worldModel->memTag;
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
		if (r_numModels >= MAX_MODEL_KNOWN)
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
	int			maxLods;
	void		(*loader) (model_t *model, void *buffer);
} modFormat_t;

static modFormat_t modelFormats[] = {
	{ MD2_HEADERSTR,	4,	ALIAS_MAX_LODS,		R_LoadMD2Model },	// Quake2 MD2 models
	{ MD3_HEADERSTR,	4,	ALIAS_MAX_LODS,		R_LoadMD3Model },	// Quake3 MD3 models
	{ SP2_HEADERSTR,	4,	0,					R_LoadSP2Model },	// Quake2 SP2 sprite models
	{ BSP_HEADERSTR,	4,	0,					R_LoadBSPModel },	// Quake2 BSP models

	{ NULL,				0,	0,					NULL }
};

static int numModelFormats = (sizeof (modelFormats) / sizeof (modelFormats[0])) - 1;

static model_t *R_ModelForName (char *name, qBool worldModel)
{
	model_t		*model;
	byte		*buffer;
	int			i, fileLen;
	modFormat_t	*descr;
	
	if (!name[0])
		Com_Error (ERR_DROP, "R_ModelForName: NULL name");

	// Use if already loaded
	model = R_FindModel (name);
	if (model) {
		model->touchFrame = r_registrationFrame;
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

	descr = modelFormats;
	for (i=0 ; i<numModelFormats ; i++, descr++) {
		if (!strncmp ((const char *)buffer, descr->headerStr, descr->headerLen))
			break;
	}

	if (i == numModelFormats)
		Com_Error (ERR_DROP, "R_ModelForName: unknown fileId for %s", model->name);

	model->radius = 0;
	ClearBounds (model->mins, model->maxs);
	model->finishedLoading = qFalse;

	descr->loader (model, buffer);

	model->touchFrame = r_registrationFrame;
	model->memSize = Mem_TagSize (MEMPOOL_MODELSYS, model->memTag);

	model->finishedLoading = qTrue;

	FS_FreeFile (buffer);

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
	char	fullName[MAX_QPATH];

	r_oldViewCluster = r_viewCluster = -1;	// force markleafs
	r_frameCount = 1;

	Q_snprintfz (fullName, sizeof (fullName), "maps/%s.bsp", mapName);

	// Explicitly free the old map if different...
	// This guarantees that r_modelList[0] is the world map
	if (!r_modelList[0].finishedLoading || strcmp (r_modelList[0].name, fullName) || flushmap->integer) {
		R_FreeModel (&r_modelList[0]);

		r_worldModel = NULL;
		memset (&r_worldEntity, 0, sizeof (r_worldEntity));
	}

	r_worldModel = R_ModelForName (fullName, qTrue);
	r_worldEntity.model = r_worldModel;
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
	int		i, startTime;
	int		freed;

	freed = 0;
	startTime = Sys_Milliseconds ();
	for (i=0, model=r_modelList ; i<r_numModels ; i++, model++) {
		if (!model->touchFrame)
			continue;	// Free model_t spot
		if (model->touchFrame == r_registrationFrame) {
			R_TouchModel (model);
			continue;	// Used this sequence
		}

		R_FreeModel (model);
		freed++;
	}
	Com_Printf (0, "%i untouched models free'd in %.2fs\n", freed, (Sys_Milliseconds () - startTime)*0.001f);
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
	if (len >= MAX_QPATH) {
		Com_Printf (0, S_COLOR_RED "R_RegisterModel: Model name too long! %s\n", name);
		return NULL;
	}

	// Copy and normalize, don't strip
	Q_strncpyz (fixedName, name, sizeof (fixedName));
	Q_FixPathName (fixedName, sizeof (fixedName), 3, qFalse);

	// MD2 deprecation
	if (!Q_stricmp (fixedName + strlen (fixedName) - 4, ".md2")) {
		char	temp[MAX_QPATH];

		Q_strncpyz (temp, fixedName, sizeof (temp));
		temp[strlen (temp)-1] = '3';
		model = R_RegisterModel (temp);

		if (model)
			return model;
	}

	// Find or load it
	model = R_ModelForName (fixedName, qFalse);

	// Found it or loaded it -- touch it
	if (model)
		R_TouchModel (model);

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
		case MODEL_MD2:		Com_Printf (0, "MD2");				break;
		case MODEL_MD3:		Com_Printf (0, "MD3");				break;
		case MODEL_BSP:		Com_Printf (0, "BSP");				break;
		case MODEL_SP2:		Com_Printf (0, "SP2");				break;
		default:			Com_Printf (0, S_COLOR_RED "BAD");	break;
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

static cmdFunc_t	*cmd_modelList;

/*
===============
R_ModelInit
===============
*/
void R_ModelInit (void)
{
	// Register commands/cvars
	flushmap	= Cvar_Get ("flushmap",		"0",		0);

	cmd_modelList = Cmd_AddCommand ("modellist",	R_ModelList_f,		"Prints to the console a list of loaded models and their sizes");

	memset (r_bspNoVis, 0xff, sizeof (r_bspNoVis));

	memset (&r_inlineModels, 0, sizeof (r_inlineModels));
	memset (&r_modelList, 0, sizeof (r_modelList));

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
