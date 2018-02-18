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
// rf_model.c
// Model loading and caching
//

#include "rf_local.h"

#define MAX_REF_MODELS		1024
#define MAX_REF_MODEL_HASH	128

static refModel_t	r_modelList[MAX_REF_MODELS];
static refModel_t	*r_modelHashTree[MAX_REF_MODEL_HASH];
static uint32		r_numModels;

static byte			r_q2BspNoVis[Q2BSP_MAX_VIS];
static byte			r_q3BspNoVis[Q3BSP_MAX_VIS];

extern int			r_q2_lmSize;

cVar_t	*flushmap;

/*
=================
R_ModelBounds
=================
*/
void R_ModelBounds (refModel_t *model, vec3_t mins, vec3_t maxs)
{
	if (model) {
		Vec3Copy (model->mins, mins);
		Vec3Copy (model->maxs, maxs);
	}
}

// ============================================================================

#define R_ModAllocZero(model,size) _Mem_Alloc ((size),qTrue,ri.modelSysPool,(model)->memTag,__FILE__,__LINE__)
#define R_ModAlloc(model,size) _Mem_Alloc ((size),qFalse,ri.modelSysPool,(model)->memTag,__FILE__,__LINE__)

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
static qBool R_LoadMD2Model (refModel_t *model)
{
	int				i, j, k;
	int				version, frameSize;
	int				skinWidth, skinHeight;
	int				numVerts, numIndexes;
	double			isw, ish;
	static int		indRemap[MD2_MAX_TRIANGLES*3];
	static index_t	tempIndex[MD2_MAX_TRIANGLES*3];
	static index_t	tempSTIndex[MD2_MAX_TRIANGLES*3];
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
	byte			*buffer;
	int				fileLen;

	// Load the file
	fileLen = FS_LoadFile (model->name, (void **)&buffer, NULL);
	if (!buffer || fileLen <= 0)
		return qFalse;

	// Check the header
	if (strncmp ((const char *)buffer, MD2_HEADERSTR, 4)) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: '%s' has invalid header", model->name);
		return qFalse;
	}

	// Check the version
	inModel = (dMd2Header_t *)buffer;
	version = LittleLong (inModel->version);
	if (version != MD2_MODEL_VERSION) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: '%s' has wrong version number (%i != %i)", model->name, version, MD2_MODEL_VERSION);
		return qFalse;
	}

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
	if (outMesh->numVerts <= 0 || outMesh->numVerts > MD2_MAX_VERTS) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has an invalid amount of vertices '%d'", model->name, outMesh->numVerts);
		return qFalse;
	}

	outMesh->numTris = LittleLong (inModel->numTris);
	if (outMesh->numTris <= 0 || outMesh->numTris > MD2_MAX_TRIANGLES) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has an invalid amount of triangles '%d'", model->name, outMesh->numTris);
		return qFalse;
	}

	frameSize = LittleLong (inModel->frameSize);
	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames <= 0 || outModel->numFrames > MD2_MAX_FRAMES) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has an invalid amount of frames '%d'", model->name, outModel->numFrames);
		return qFalse;
	}

	//
	// Load the skins
	//
	skinWidth = LittleLong (inModel->skinWidth);
	skinHeight = LittleLong (inModel->skinHeight);
	if (skinWidth <= 0 || skinHeight <= 0) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has invalid skin dimensions '%d x %d'", model->name, skinWidth, skinHeight);
		return qFalse;
	}

	outMesh->numSkins = LittleLong (inModel->numSkins);
	if (outMesh->numSkins < 0 || outMesh->numSkins > MD2_MAX_SKINS) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has an invalid amount of skins '%d'", model->name, outMesh->numSkins);
		return qFalse;
	}

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
	for (i=0 ; i<numIndexes ; i++)
		indRemap[i] = -1;

	for (i=0 ; i<numIndexes ; i++) {
		if (indRemap[i] != -1)
			continue;

		// Remap duplicates
		for (j=i+1 ; j<numIndexes ; j++) {
			if (tempIndex[j] != tempIndex[i])
				continue;
			if (inCoord[tempSTIndex[j]].s != inCoord[tempSTIndex[i]].s
			|| inCoord[tempSTIndex[j]].t != inCoord[tempSTIndex[i]].t)
				continue;

			indRemap[j] = i;
			outIndex[j] = numVerts;
		}

		// Add unique vertex
		indRemap[i] = i;
		outIndex[i] = numVerts++;
	}

	if (numVerts <= 0 || numVerts >= ALIAS_MAX_VERTS) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD2Model: model '%s' has an invalid amount of resampled verts for an alias model '%d' >= ALIAS_MAX_VERTS", numVerts, ALIAS_MAX_VERTS);
		return qFalse;
	}

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
		Vec3Copy (outFrame->translate, outFrame->mins);
		Vec3MA (outFrame->translate, 255, outFrame->scale, outFrame->maxs);
		outFrame->radius = RadiusFromBounds (outFrame->mins, outFrame->maxs);

		// Mesh bounds
		Vec3Copy (outFrame->mins, outMesh->mins[i]);
		Vec3Copy (outFrame->maxs, outMesh->maxs[i]);
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
		if (!outSkins->skin)
			Com_DevPrintf (PRNT_WARNING, "R_LoadMD2Model: '%s' could not load skin '%s'\n", model->name, outSkins->name);
	}

	// Done
	FS_FreeFile (buffer);
	return qTrue;
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
	size_t	len;
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
static qBool R_LoadMD3Model (refModel_t *model)
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
	byte				*buffer;
	int					fileLen;

	// Load the file
	fileLen = FS_LoadFile (model->name, (void **)&buffer, NULL);
	if (!buffer || fileLen <= 0)
		return qFalse;

	// Check the header
	if (strncmp ((const char *)buffer, MD3_HEADERSTR, 4)) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: '%s' has invalid header", model->name);
		return qFalse;
	}

	// Check the version
	inModel = (dMd3Header_t *)buffer;
	version = LittleLong (inModel->version);
	if (version != MD3_MODEL_VERSION) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: model '%s' has wrong version number (%i != %i)", model->name, version, MD3_MODEL_VERSION);
		return qFalse;
	}

	model->aliasModel = outModel = R_ModAlloc (model, sizeof (mAliasModel_t));
	model->type = MODEL_MD3;

	//
	// Byte swap the header fields and sanity check
	//
	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames <= 0 || outModel->numFrames > MD3_MAX_FRAMES) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: model '%s' has an invalid amount of frames '%d'", model->name, outModel->numFrames);
		return qFalse;
	}

	outModel->numTags = LittleLong (inModel->numTags);
	if (outModel->numTags < 0 || outModel->numTags > MD3_MAX_TAGS) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: model '%s' has an invalid amount of tags '%d'", model->name, outModel->numTags);
		return qFalse;
	}

	outModel->numMeshes = LittleLong (inModel->numMeshes);
	if (outModel->numMeshes < 0 || outModel->numMeshes > MD3_MAX_MESHES) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: model '%s' has an invalid amount of meshes '%d'", model->name, outModel->numMeshes);
		return qFalse;
	}

	if (!outModel->numMeshes && !outModel->numTags) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadMD3Model: model '%s' has no meshes and no tags!", model->name);
		return qFalse;
	}

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
		if (strncmp ((const char *)inMesh->ident, MD3_HEADERSTR, 4)) {
			FS_FreeFile (buffer);
			Com_Printf (PRNT_ERROR, "R_LoadMD3Model: mesh '%s' in model '%s' has wrong id (%i != %i)", inMesh->meshName, model->name, LittleLong (*(int* )inMesh->ident), MD3_HEADER);
			return qFalse;
		}

		R_StripModelLODSuffix (outMesh->name);

		outMesh->numSkins = LittleLong (inMesh->numSkins);
		if (outMesh->numSkins <= 0 || outMesh->numSkins > MD3_MAX_SHADERS) {
			FS_FreeFile (buffer);
			Com_Printf (PRNT_ERROR, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of skins '%d'", outMesh->name, model->name, outMesh->numSkins);
			return qFalse;
		}

		outMesh->numTris = LittleLong (inMesh->numTris);
		if (outMesh->numTris <= 0 || outMesh->numTris > MD3_MAX_TRIANGLES) {
			FS_FreeFile (buffer);
			Com_Printf (PRNT_ERROR, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of triangles '%d'", outMesh->name, model->name, outMesh->numTris);
			return qFalse;
		}

		outMesh->numVerts = LittleLong (inMesh->numVerts);
		if (outMesh->numVerts <= 0 || outMesh->numVerts > MD3_MAX_VERTS) {
			FS_FreeFile (buffer);
			Com_Printf (PRNT_ERROR, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of vertices '%d'", outMesh->name, model->name, outMesh->numVerts);
			return qFalse;
		}

		if (outMesh->numVerts >= ALIAS_MAX_VERTS) {
			FS_FreeFile (buffer);
			Com_Printf (PRNT_ERROR, "R_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount verts for an alias model '%d' >= ALIAS_MAX_VERTS", outMesh->name, outMesh->numVerts, ALIAS_MAX_VERTS);
			return qFalse;
		}

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
				Vec3Copy (outVert->point, v);
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
		Vec3MA (outFrame->translate, MD3_XYZ_SCALE, outFrame->mins, outFrame->mins);
		Vec3MA (outFrame->translate, MD3_XYZ_SCALE, outFrame->maxs, outFrame->maxs);
		outFrame->radius = RadiusFromBounds (outFrame->mins, outFrame->maxs);

		AddPointToBounds (outFrame->mins, model->mins, model->maxs);
		AddPointToBounds (outFrame->maxs, model->mins, model->maxs);
		model->radius = max (model->radius, outFrame->radius);
	}

	// Done
	FS_FreeFile (buffer);
	return qTrue;
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
static qBool R_LoadSP2Model (refModel_t *model)
{
	dSpriteHeader_t	*inModel;
	dSpriteFrame_t	*inFrames;
	mSpriteModel_t	*outModel;
	mSpriteFrame_t	*outFrames;
	int				i, version;
	int				numFrames;
	byte			*allocBuffer;
	byte			*buffer;
	int				fileLen;

	// Load the file
	fileLen = FS_LoadFile (model->name, (void **)&buffer, NULL);
	if (!buffer || fileLen <= 0)
		return qFalse;

	inModel = (dSpriteHeader_t *)buffer;

	//
	// Sanity checks
	//
	version = LittleLong (inModel->version);
	if (version != SP2_VERSION) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadSP2Model: '%s' has wrong version number (%i should be %i)", model->name, version, SP2_VERSION);
		return qFalse;
	}

	numFrames = LittleLong (inModel->numFrames);
	if (numFrames > SP2_MAX_FRAMES) {
		FS_FreeFile (buffer);
		Com_Printf (PRNT_ERROR, "R_LoadSP2Model: '%s' has too many frames (%i > %i)", model->name, numFrames, SP2_MAX_FRAMES);
		return qFalse;
	}

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
			Com_DevPrintf (PRNT_WARNING, "R_LoadSP2Model: '%s' could not load skin '%s'\n", model->name, outFrames->name);
	}

	// Done
	return qTrue;
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

	ClearBounds (mins, maxs);
	for (i=0, v=verts ; i<numVerts ; i++) {
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

		image = pass->animImages[0];
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
static qBool SubdivideQ2Polygon (refModel_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
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

	if (numVerts > 60) {
		Com_Printf (PRNT_ERROR, "SubdivideQ2Polygon: numVerts = %i", numVerts);
		return qFalse;
	}

	BoundQ2BSPPoly (numVerts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5f;
		m = subdivideSize * (float)floor (m/subdivideSize + 0.5f);
		if (maxs[i]-m < 8)
			continue;
		if (m-mins[i] < 8)
			continue;

		// Cut it
		v = verts + i;
		for (j=0 ; j<numVerts ; j++, v+=3)
			dist[j] = *v - m;

		// Wrap cases
		dist[j] = dist[0];
		v -= i;
		Vec3Copy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numVerts ; j++, v+=3) {
			if (dist[j] >= 0) {
				Vec3Copy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0) {
				Vec3Copy (v, back[b]);
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

		return SubdivideQ2Polygon (model, surf, f, front[0], subdivideSize)
			&& SubdivideQ2Polygon (model, surf, b, back[0], subdivideSize);
	}

	// Add a point in the center to help keep warp valid
	buffer = R_ModAllocZero (model, sizeof (mQ2BspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t)));
	poly = (mQ2BspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = NULL;
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

	Vec3Clear (posTotal);
	Vec3Clear (normalTotal);
	Vec2Clear (coordTotal);

	for (i=0 ; i<numVerts ; i++) {
		// Colors
		Vec4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);

		// Position
		Vec3Copy (verts, poly->mesh.vertexArray[i+1]);

		// Normal
		if (!(surf->q2_flags & SURF_PLANEBACK))
			Vec3Copy (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);
		else
			Vec3Negate (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coords
		poly->mesh.coordArray[i+1][0] = DotProduct (verts, surf->q2_texInfo->vecs[0]) * (1.0f/64.0f);
		poly->mesh.coordArray[i+1][1] = DotProduct (verts, surf->q2_texInfo->vecs[1]) * (1.0f/64.0f);

		// For the center point
		Vec3Add (posTotal, verts, posTotal);
		Vec3Add (normalTotal, surf->q2_plane->normal, normalTotal);
		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];

		verts += 3;
	}

	// Center
	oneDivVerts = (1.0f/(float)numVerts);
	Vec4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	Vec3Scale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	Vec3Scale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	VectorNormalizef (poly->mesh.normalsArray[0], poly->mesh.normalsArray[0]);
	Vec2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);

	// Copy first vertex to last
	Vec4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	Vec3Copy (poly->mesh.vertexArray[1], poly->mesh.vertexArray[i+1]);
	Vec3Copy (poly->mesh.normalsArray[1], poly->mesh.normalsArray[i+1]);
	Vec2Copy (poly->mesh.coordArray[1], poly->mesh.coordArray[i+1]);

	// Link it in
	poly->next = surf->q2_polys;
	surf->q2_polys = poly;

	return qTrue;
}
static qBool R_SubdivideQ2BSPSurface (refModel_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	index_t		index;
	float		*vec;
	int			i;

	// Convert edges back to a normal polygon
	for (i=0, numVerts=0 ; i<surf->q2_numEdges ; numVerts++, i++) {
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];
		if (index >= 0)
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[index].v[0]].position;
		else
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[-index].v[1]].position;

		Vec3Copy (vec, verts[numVerts]);
	}

	return SubdivideQ2Polygon (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_SubdivideQ2BSPLMSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static qBool SubdivideQ2BSPLMSurface_r (refModel_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize)
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

	if (numVerts > 60) {
		Com_Printf (PRNT_ERROR, "SubdivideQ2BSPLMSurface_r: numVerts = %i", numVerts);
		return qFalse;
	}

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
		Vec3Copy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numVerts ; j++, v+= 3) {
			if (dist[j] >= 0) {
				Vec3Copy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0) {
				Vec3Copy (v, back[b]);
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

		return SubdivideQ2BSPLMSurface_r (model, surf, f, front[0], subdivideSize)
			&& SubdivideQ2BSPLMSurface_r (model, surf, b, back[0], subdivideSize);
	}

	// Add a point in the center to help keep warp valid
	buffer = R_ModAllocZero (model, sizeof (mQ2BspPoly_t) + ((numVerts+2) * sizeof (bvec4_t)) + ((numVerts+2) * sizeof (vec3_t) * 2) + ((numVerts+2) * sizeof (vec2_t) * 2));
	poly = (mQ2BspPoly_t *)buffer;

	poly->mesh.numVerts = numVerts+2;
	poly->mesh.numIndexes = numVerts*3;

	poly->mesh.indexArray = NULL;
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

	Vec3Clear (posTotal);
	Vec3Clear (normalTotal);
	Vec2Clear (coordTotal);
	Vec2Clear (lmCoordTotal);

	for (i=0 ; i<numVerts ; i++) {
		// Colors
		Vec4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);

		// Position
		Vec3Copy (verts, poly->mesh.vertexArray[i+1]);

		// Normals
		if (!(surf->q2_flags & SURF_PLANEBACK))
			Vec3Copy (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);
		else
			Vec3Negate (surf->q2_plane->normal, poly->mesh.normalsArray[i+1]);

		// Texture coordinates
		poly->mesh.coordArray[i+1][0] = (DotProduct (verts, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3]) / surf->q2_texInfo->width;
		poly->mesh.coordArray[i+1][1] = (DotProduct (verts, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3]) / surf->q2_texInfo->height;

		// Lightmap texture coordinates
		s = DotProduct (verts, surf->q2_texInfo->vecs[0]) + surf->q2_texInfo->vecs[0][3] - surf->q2_textureMins[0];
		poly->mesh.lmCoordArray[i+1][0] = (s + 8 + (surf->q2_lmCoords[0] * 16)) / (r_q2_lmSize * 16);

		t = DotProduct (verts, surf->q2_texInfo->vecs[1]) + surf->q2_texInfo->vecs[1][3] - surf->q2_textureMins[1];
		poly->mesh.lmCoordArray[i+1][1] = (t + 8 + (surf->q2_lmCoords[1] * 16)) / (r_q2_lmSize * 16);

		// For the center point
		Vec3Add (posTotal, verts, posTotal);
		Vec3Add (normalTotal, surf->q2_plane->normal, normalTotal);

		coordTotal[0] += poly->mesh.coordArray[i+1][0];
		coordTotal[1] += poly->mesh.coordArray[i+1][1];

		lmCoordTotal[0] += poly->mesh.lmCoordArray[i+1][0];
		lmCoordTotal[1] += poly->mesh.lmCoordArray[i+1][1];

		verts += 3;
	}

	// Center point
	oneDivVerts = (1.0f/(float)numVerts);
	Vec4Set (poly->mesh.colorArray[0], 255, 255, 255, 255);
	Vec3Scale (posTotal, oneDivVerts, poly->mesh.vertexArray[0]);
	Vec3Scale (normalTotal, oneDivVerts, poly->mesh.normalsArray[0]);
	VectorNormalizef (poly->mesh.normalsArray[0], poly->mesh.normalsArray[0]);
	Vec2Scale (coordTotal, oneDivVerts, poly->mesh.coordArray[0]);
	Vec2Scale (lmCoordTotal, oneDivVerts, poly->mesh.lmCoordArray[0]);

	// Copy first vertex to last
	Vec4Set (poly->mesh.colorArray[i+1], 255, 255, 255, 255);
	Vec3Copy (poly->mesh.vertexArray[1], poly->mesh.vertexArray[i+1]);
	Vec3Copy (poly->mesh.normalsArray[1], poly->mesh.normalsArray[i+1]);
	Vec2Copy (poly->mesh.coordArray[1], poly->mesh.coordArray[i+1]);
	Vec2Copy (poly->mesh.lmCoordArray[1], poly->mesh.lmCoordArray[i+1]);

	// Link it in
	poly->next = surf->q2_polys;
	surf->q2_polys = poly;

	return qTrue;
}

static qBool R_SubdivideQ2BSPLMSurface (refModel_t *model, mBspSurface_t *surf, float subdivideSize)
{
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	int			index;
	float		*vec;

	// Convert edges back to a normal polygon
	for (i=0, numVerts=0 ; i<surf->q2_numEdges ; numVerts++, i++) {
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge + i];
		if (index >= 0)
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[index].v[0]].position;
		else
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[-index].v[1]].position;

		Vec3Copy (vec, verts[numVerts]);
	}

	return SubdivideQ2BSPLMSurface_r (model, surf, numVerts, verts[0], subdivideSize);
}


/*
================
R_BuildQ2BSPSurface
================
*/
static qBool R_BuildQ2BSPSurface (refModel_t *model, mBspSurface_t *surf)
{
	byte			*buffer;
	byte			*outColors;
	float			*outCoords;
	index_t			*outIndexes;
	float			*outLMCoords;
	mesh_t			*outMesh;
	float			*outNormals;
	float			*outVerts;
	int				numVerts, numIndexes;
	float			*vec, s, t;
	int				index, i;
	mQ2BspTexInfo_t	*ti;

	// Bogus face
	if (surf->q2_numEdges < 3) {
		assert (0);
		surf->mesh = NULL;
		return qTrue;	// FIXME: return qFalse?
	}

	ti = surf->q2_texInfo;
	numVerts = surf->q2_numEdges;
	numIndexes = (numVerts - 2) * 3;

	// Allocate space
	if (ti->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		buffer = R_ModAllocZero (model, sizeof (mesh_t)
			+ (numVerts * sizeof (vec3_t) * 2)
			+ (numIndexes * sizeof (index_t))
			+ (numVerts * sizeof (vec2_t))
			+ (numVerts * sizeof (bvec4_t)));
	}
	else {
		buffer = R_ModAllocZero (model, sizeof (mesh_t)
			+ (numVerts * sizeof (vec3_t) * 2)
			+ (numIndexes * sizeof (index_t))
			+ (numVerts * sizeof (vec2_t) * 2)
			+ (numVerts * sizeof (bvec4_t)));
	}

	surf->mesh = outMesh = (mesh_t *)buffer;
	outMesh->numIndexes = numIndexes;
	outMesh->numVerts = numVerts;

	buffer += sizeof (mesh_t);
	outVerts = (float *)buffer;

	buffer += sizeof (vec3_t) * numVerts;
	outNormals = (float *)buffer;

	buffer += sizeof (vec3_t) * numVerts;
	outIndexes = (index_t *)buffer;

	buffer += sizeof (index_t) * numIndexes;
	outCoords = (float *)buffer;

	if (ti->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		outLMCoords = NULL;
	}
	else {
		buffer += sizeof (vec2_t) * numVerts;
		outLMCoords = (float *)buffer;
	}

	buffer += sizeof (vec2_t) * numVerts;
	outColors = (byte *)buffer;

	outMesh->colorArray = (bvec4_t *)outColors;
	outMesh->coordArray = (vec2_t *)outCoords;
	outMesh->indexArray = (index_t *)outIndexes;
	outMesh->lmCoordArray = (vec2_t *)outLMCoords;
	outMesh->normalsArray = (vec3_t *)outNormals;
	outMesh->sVectorsArray = NULL;
	outMesh->tVectorsArray = NULL;
	outMesh->trNeighborsArray = NULL;
	outMesh->trNormalsArray = NULL;
	outMesh->vertexArray = (vec3_t *)outVerts;

	// Check mesh validity
	if (RB_InvalidMesh (outMesh)) {
		Com_Printf (PRNT_ERROR, "R_BuildQ2BSPSurface: surface mesh is invalid!");
		return qFalse;
	}

	// Copy vertex data
	for (i=0 ; i<numVerts ; i++) {
		// Color
		Vec4Set (outColors, 255, 255, 255, 255);

		// Position
		index = model->q2BspModel.surfEdges[surf->q2_firstEdge+i];
		if (index >= 0)
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[index].v[0]].position;
		else
			vec = model->q2BspModel.vertexes[model->q2BspModel.edges[-index].v[1]].position;
		Vec3Copy (vec, outVerts);

		// Normal
		if (!(surf->q2_flags & SURF_PLANEBACK))
			Vec3Copy (surf->q2_plane->normal, outNormals);
		else
			Vec3Negate (surf->q2_plane->normal, outNormals);

		// Texture coordinates
		outCoords[0] = (DotProduct (vec, ti->vecs[0]) + ti->vecs[0][3]) / ti->width;
		outCoords[1] = (DotProduct (vec, ti->vecs[1]) + ti->vecs[1][3]) / ti->height;

		outColors += 4;
		outCoords += 2;
		outNormals += 3;
		outVerts += 3;
	}

	// Lightmap coordinates (if needed)
	outVerts = (float *)outMesh->vertexArray;
	if (outLMCoords) {
		for (i=0 ; i<numVerts ; i++) {
			s = DotProduct (outVerts, ti->vecs[0]) + ti->vecs[0][3] - surf->q2_textureMins[0];
			outLMCoords[0] = (s + 8 + (surf->q2_lmCoords[0] * 16)) / (r_q2_lmSize * 16);

			t = DotProduct (outVerts, ti->vecs[1]) + ti->vecs[1][3] - surf->q2_textureMins[1];
			outLMCoords[1] = (t + 8 + (surf->q2_lmCoords[1] * 16)) / (r_q2_lmSize * 16);

			outVerts += 3;
			outLMCoords += 2;
		}
	}

	// Indexes
	for (i=2 ; i<numVerts ; i++) {
		outIndexes[0] = 0;
		outIndexes[1] = i - 1;
		outIndexes[2] = i;

		outIndexes += 3;
	}

	return qTrue;
}


/*
================
R_ConvertQ2BSPSurface
================
*/
static qBool R_ConvertQ2BSPSurface (refModel_t *model, mBspSurface_t *surf)
{
	byte			*buffer;
	mQ2BspPoly_t	*poly, *next;
	mQ2BspTexInfo_t	*ti;
	uint32			totalIndexes;
	uint32			totalVerts;
	byte			*outColors;
	float			*outCoords;
	index_t			*outIndexes;
	float			*outLMCoords;
	mesh_t			*outMesh;
	float			*outNormals;
	float			*outVerts;
	int				i;

	ti = surf->q2_texInfo;

	// Find the total vertex count and index count
	totalIndexes = 0;
	totalVerts = 0;
	for (poly=surf->q2_polys ; poly ; poly=poly->next) {
		totalIndexes += (poly->mesh.numVerts - 2) * 3;
		totalVerts += poly->mesh.numVerts;
	}

	// Allocate space
	if (ti->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		buffer = R_ModAllocZero (model, sizeof (mesh_t)
			+ (totalVerts * sizeof (vec3_t) * 2)
			+ (totalIndexes * sizeof (index_t))
			+ (totalVerts * sizeof (vec2_t))
			+ (totalVerts * sizeof (bvec4_t)));
	}
	else {
		buffer = R_ModAllocZero (model, sizeof (mesh_t)
			+ (totalVerts * sizeof (vec3_t) * 2)
			+ (totalIndexes * sizeof (index_t))
			+ (totalVerts * sizeof (vec2_t) * 2)
			+ (totalVerts * sizeof (bvec4_t)));
	}

	surf->mesh = outMesh = (mesh_t *)buffer;
	outMesh->numIndexes = totalIndexes;
	outMesh->numVerts = totalVerts;

	buffer += sizeof (mesh_t);
	outVerts = (float *)buffer;

	buffer += sizeof (vec3_t) * totalVerts;
	outNormals = (float *)buffer;

	buffer += sizeof (vec3_t) * totalVerts;
	outIndexes = (index_t *)buffer;

	buffer += sizeof (index_t) * totalIndexes;
	outCoords = (float *)buffer;

	if (ti->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) {
		outLMCoords = NULL;
	}
	else {
		buffer += sizeof (vec2_t) * totalVerts;
		outLMCoords = (float *)buffer;
	}

	buffer += sizeof (vec2_t) * totalVerts;
	outColors = (byte *)buffer;

	outMesh->colorArray = (bvec4_t *)outColors;
	outMesh->coordArray = (vec2_t *)outCoords;
	outMesh->indexArray = (index_t *)outIndexes;
	outMesh->lmCoordArray = (vec2_t *)outLMCoords;
	outMesh->normalsArray = (vec3_t *)outNormals;
	outMesh->sVectorsArray = NULL;
	outMesh->tVectorsArray = NULL;
	outMesh->trNeighborsArray = NULL;
	outMesh->trNormalsArray = NULL;
	outMesh->vertexArray = (vec3_t *)outVerts;

	// Check mesh validity
	if (RB_InvalidMesh (outMesh)) {
		Com_Printf (PRNT_ERROR, "R_ConvertQ2BSPSurface: surface mesh is invalid!");
		return qFalse;
	}

	// Store vertex data
	totalIndexes = 0;
	totalVerts = 0;
	for (poly=surf->q2_polys ; poly ; poly=poly->next) {
		// Indexes
		outIndexes = outMesh->indexArray + totalIndexes;
		totalIndexes += (poly->mesh.numVerts - 2) * 3;
		for (i=2 ; i<poly->mesh.numVerts ; i++) {
			outIndexes[0] = totalVerts;
			outIndexes[1] = totalVerts + i - 1;
			outIndexes[2] = totalVerts + i;

			outIndexes += 3;
		}

		for (i=0 ; i<poly->mesh.numVerts ; i++) {
			// Vertices
			outVerts[0] = poly->mesh.vertexArray[i][0];
			outVerts[1] = poly->mesh.vertexArray[i][1];
			outVerts[2] = poly->mesh.vertexArray[i][2];

			// Normals
			outNormals[0] = poly->mesh.normalsArray[i][0];
			outNormals[1] = poly->mesh.normalsArray[i][1];
			outNormals[2] = poly->mesh.normalsArray[i][2];

			// Colors
			outColors[0] = 255;
			outColors[1] = 255;
			outColors[2] = 255;
			outColors[3] = 255;

			// Coords
			outCoords[0] = poly->mesh.coordArray[i][0];
			outCoords[1] = poly->mesh.coordArray[i][1];

			outVerts += 3;
			outNormals += 3;
			outColors += 4;
			outCoords += 2;
		}

		totalVerts += poly->mesh.numVerts;
	}

	// Lightmap coords
	if (!(ti->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP))) {
		for (poly=surf->q2_polys ; poly ; poly=poly->next) {
			for (i=0 ; i<poly->mesh.numVerts ; i++) {
				outLMCoords[0] = poly->mesh.lmCoordArray[i][0];
				outLMCoords[1] = poly->mesh.lmCoordArray[i][1];

				outLMCoords += 2;
			}
		}
	}

	// Release the old q2_polys crap
	for (poly=surf->q2_polys ; poly ; poly=next) {
		next = poly->next;
		Mem_Free (poly);
	}

	return qTrue;
}

// ============================================================================

/*
==============
R_Q2BSPClusterPVS
==============
*/
byte *R_Q2BSPClusterPVS (int cluster, refModel_t *model)
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
mBspLeaf_t *R_PointInQ2BSPLeaf (vec3_t point, refModel_t *model)
{
	mBspNode_t	*node;
	float		d;

	if (!model || !model->bspModel.nodes)
		Com_Error (ERR_DROP, "R_PointInQ2BSPLeaf: bad model");

	node = model->bspModel.nodes;
	for ( ; ; ) {
		if (node->c.q2_contents != -1)
			return (mBspLeaf_t *)node;

		d = PlaneDiff (point, node->c.plane);
		node = node->children[!(d > 0)];
	}
}

/*
===============================================================================

	QUAKE2 BRUSH MODEL LOADING

===============================================================================
*/

/*
================
R_CalcQ2BSPSurfaceExtents

Fills in s->textureMins[] and s->extents[]
================
*/
static void R_CalcQ2BSPSurfaceExtents (refModel_t *model, mBspSurface_t *surf)
{
	float			mins[2], maxs[2], val;
	int				i, j, e;
	mQ2BspVertex_t	*v;
	mQ2BspTexInfo_t	*tex;
	int				bmins[2], bmaxs[2];

	Clear2DBounds (mins, maxs);

	tex = surf->q2_texInfo;
	for (i=0 ; i<surf->q2_numEdges ; i++) {
		e = model->q2BspModel.surfEdges[surf->q2_firstEdge+i];
		if (e >= 0)
			v = &model->q2BspModel.vertexes[model->q2BspModel.edges[e].v[0]];
		else
			v = &model->q2BspModel.vertexes[model->q2BspModel.edges[-e].v[1]];

		for (j=0 ; j<2 ; j++) {
			val = v->position[0]*tex->vecs[j][0] + v->position[1]*tex->vecs[j][1] + v->position[2]*tex->vecs[j][2] + tex->vecs[j][3];
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
	node->c.parent = parent;
	if (node->c.q2_contents != -1)
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
static qBool R_LoadQ2BSPVertexes (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspVertex_t	*in;
	mQ2BspVertex_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPVertexes: funny lump size in %s", model->name);
		return qFalse;
	}

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

	return qTrue;
}


/*
=================
R_LoadQ2BSPEdges
=================
*/
static qBool R_LoadQ2BSPEdges (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspEdge_t	*in;
	mQ2BspEdge_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPEdges: funny lump size in %s", model->name);
		return qFalse;
	}

	model->q2BspModel.numEdges = lump->fileLen / sizeof (*in);
	model->q2BspModel.edges = out = R_ModAllocZero (model, sizeof (*out) * (model->q2BspModel.numEdges + 1));

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numEdges ; i++, in++, out++) {
		out->v[0] = (uint16) LittleShort (in->v[0]);
		out->v[1] = (uint16) LittleShort (in->v[1]);
	}

	return qTrue;
}


/*
=================
R_LoadQ2BSPSurfEdges
=================
*/
static qBool R_LoadQ2BSPSurfEdges (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{	
	int		*in;
	int		*out;
	int		i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPSurfEdges: funny lump size in %s", model->name);
		return qFalse;
	}

	model->q2BspModel.numSurfEdges = lump->fileLen / sizeof (*in);
	if (model->q2BspModel.numSurfEdges < 1 || model->q2BspModel.numSurfEdges >= Q2BSP_MAX_SURFEDGES) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPSurfEdges: invalid surfEdges count in %s: %i (min: 1; max: %d)", model->name, model->q2BspModel.numSurfEdges, Q2BSP_MAX_SURFEDGES);
		return qFalse;
	}

	model->q2BspModel.surfEdges = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numSurfEdges);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numSurfEdges ; i++)
		out[i] = LittleLong (in[i]);

	return qTrue;
}


/*
=================
R_LoadQ2BSPLighting
=================
*/
static qBool R_LoadQ2BSPLighting (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	if (!lump->fileLen) {
		model->q2BspModel.lightData = NULL;
		return qTrue;
	}

	model->q2BspModel.lightData = R_ModAllocZero (model, lump->fileLen);	
	memcpy (model->q2BspModel.lightData, byteBase + lump->fileOfs, lump->fileLen);

	return qTrue;
}


/*
=================
R_LoadQ2BSPPlanes
=================
*/
static qBool R_LoadQ2BSPPlanes (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ2BspPlane_t	*in;
	int				bits;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPPlanes: funny lump size in %s", model->name);
		return qFalse;
	}

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

	return qTrue;
}


/*
=================
R_LoadQ2BSPTexInfo
=================
*/
static qBool R_LoadQ2BSPTexInfo (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspTexInfo_t	*in;
	mQ2BspTexInfo_t	*out, *step;
	int				i, next;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPTexInfo: funny lump size in %s", model->name);
		return qFalse;
	}

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
		out->vecs[1][0] = LittleFloat (in->vecs[1][0]);
		out->vecs[1][1] = LittleFloat (in->vecs[1][1]);
		out->vecs[1][2] = LittleFloat (in->vecs[1][2]);
		out->vecs[1][3] = LittleFloat (in->vecs[1][3]);

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

		R_GetImageTCSize (out->shader, &out->width, &out->height);
	}

	//
	// Count animation frames
	//
	for (i=0 ; i<model->q2BspModel.numTexInfo ; i++) {
		out = &model->q2BspModel.texInfo[i];
		out->numFrames = 1;
		for (step=out->next ; step && step!=out ; step=step->next)
			out->numFrames++;
	}

	return qTrue;
}


/*
=================
R_LoadQ2BSPFaces
=================
*/
static qBool R_LoadQ2BSPFaces (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	mQ2BspVertex_t	*v;
	dQ2BspSurface_t	*in;
	mBspSurface_t	*out;
	int				i, j, e, surfNum;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPFaces: funny lump size in %s", model->name);
		return qFalse;
	}

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

		out->q2_plane = model->bspModel.planes + LittleShort (in->planeNum);
		if (LittleShort (in->side))
			out->q2_flags |= SURF_PLANEBACK;

		i = LittleShort (in->texInfo);
		if (i < 0 || i >= model->q2BspModel.numTexInfo) {
			Com_Printf (PRNT_ERROR, "R_LoadQ2BSPFaces: bad texInfo number");
			return qFalse;
		}
		out->q2_texInfo = model->q2BspModel.texInfo + i;

		//
		// Calculate surface bounds
		//
		ClearBounds (out->mins, out->maxs);
		for (j=0 ; j<out->q2_numEdges ; j++) {
			e = model->q2BspModel.surfEdges[out->q2_firstEdge + j];
			if (e >= 0)
				v = &model->q2BspModel.vertexes[model->q2BspModel.edges[e].v[0]];
			else
				v = &model->q2BspModel.vertexes[model->q2BspModel.edges[-e].v[1]];

			AddPointToBounds (v->position, out->mins, out->maxs);
		}

		//
		// Calculate surface extents
		//
		R_CalcQ2BSPSurfaceExtents (model, out);

		//
		// Lighting info
		//
		out->q2_lmWidth = (out->q2_extents[0]>>4) + 1;
		out->q2_lmHeight = (out->q2_extents[1]>>4) + 1;

		i = LittleLong (in->lightOfs);
		out->q2_lmSamples = (i == -1) ? NULL : model->q2BspModel.lightData + i;

		for (out->q2_numStyles=0 ; out->q2_numStyles<Q2BSP_MAX_LIGHTMAPS && in->styles[out->q2_numStyles]!=255 ; out->q2_numStyles++)
			out->q2_styles[out->q2_numStyles] = in->styles[out->q2_numStyles];

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
				if (!R_SubdivideQ2BSPSurface (model, out, out->q2_texInfo->shader->subdivide)
				|| !R_ConvertQ2BSPSurface (model, out))
					return qFalse;
			}
			else if (!R_BuildQ2BSPSurface (model, out))
				return qFalse;
		}
		else {
			// The rest do
			R_Q2BSP_CreateSurfaceLightmap (out);

			if (out->q2_texInfo->shader && out->q2_texInfo->shader->flags & SHADER_SUBDIVIDE) {
				if (!R_SubdivideQ2BSPLMSurface (model, out, out->q2_texInfo->shader->subdivide)
				|| !R_ConvertQ2BSPSurface (model, out))
					return qFalse;
			}
			else if (!R_BuildQ2BSPSurface (model, out))
				return qFalse;
		}
	}

	R_Q2BSP_EndBuildingLightmaps ();

	return qTrue;
}


/*
=================
R_LoadQ2BSPMarkSurfaces
=================
*/
static qBool R_LoadQ2BSPMarkSurfaces (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	mBspSurface_t	**out;
	int				i, j;
	int16			*in;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPMarkSurfaces: funny lump size in %s", model->name);
		return qFalse;
	}

	model->q2BspModel.numMarkSurfaces = lump->fileLen / sizeof (*in);
	model->q2BspModel.markSurfaces = out = R_ModAllocZero (model, sizeof (*out) * model->q2BspModel.numMarkSurfaces);

	//
	// Byte swap
	//
	for (i=0 ; i<model->q2BspModel.numMarkSurfaces ; i++) {
		j = LittleShort (in[i]);
		if (j < 0 || j >= model->bspModel.numSurfaces) {
			Com_Printf (PRNT_ERROR, "R_LoadQ2BSPMarkSurfaces: bad surface number");
			return qFalse;
		}
		out[i] = model->bspModel.surfaces + j;
	}

	return qTrue;
}


/*
=================
R_LoadQ2BSPVisibility
=================
*/
static qBool R_LoadQ2BSPVisibility (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int		i;

	if (!lump->fileLen) {
		model->q2BspModel.vis = NULL;
		return qTrue;
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

	return qTrue;
}


/*
=================
R_LoadQ2BSPLeafs
=================
*/
static qBool R_Q2BSP_SurfPotentiallyVisible (mBspSurface_t *surf)
{
	if (!surf->q2_texInfo)
		return qFalse;

	if (!surf->mesh || RB_InvalidMesh (surf->mesh))
		return qFalse;
	if (!surf->q2_texInfo->shader)
		return qFalse;
	if (!surf->q2_texInfo->shader->numPasses)
		return qFalse;

	return qTrue;
}
static qBool R_Q2BSP_SurfPotentiallyLit (mBspSurface_t *surf)
{
	if (surf->q2_texInfo->flags & (SURF_TEXINFO_SKY|SURF_TEXINFO_WARP)) 
		return qFalse;
	if (!surf->q2_lmSamples)
		return qFalse;

	return qTrue;
}
static qBool R_Q2BSP_SurfPotentiallyFragmented (mBspSurface_t *surf)
{
	if (surf->q2_texInfo->flags & SURF_TEXINFO_NODRAW)
		return qFalse;
	if (surf->q2_texInfo->shader->flags & SHADER_NOMARK)
		return qFalse;

	return qTrue;
}
static qBool R_LoadQ2BSPLeafs (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspLeaf_t	*in;
	mBspLeaf_t		*out;
	int				i, j;
	qBool			badBounds;
	mBspSurface_t	*surf;
	uint32			numFragSurfaces;
	uint32			totalFragRemoved;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPLeafs: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numLeafs = lump->fileLen / sizeof (*in);
	model->bspModel.leafs = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numLeafs);

	//
	// Byte swap
	//
	totalFragRemoved = 0;
	for (i=0 ; i<model->bspModel.numLeafs ; i++, in++, out++) {
		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->c.mins[j] = LittleShort (in->mins[j]);
			out->c.maxs[j] = LittleShort (in->maxs[j]);
			if (out->c.mins[j] > out->c.maxs[j])
				badBounds = qTrue;
		}

		if (i && (badBounds || Vec3Compare (out->c.mins, out->c.maxs))) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad leaf %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint (out->c.mins[0]), Q_rint (out->c.mins[1]), Q_rint (out->c.mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint (out->c.maxs[0]), Q_rint (out->c.maxs[1]), Q_rint (out->c.maxs[2]));
			Com_DevPrintf (PRNT_WARNING, "cluster: %i\n", out->cluster);
			Com_DevPrintf (PRNT_WARNING, "area: %i\n", out->area);
			out->c.badBounds = qTrue;
		}
		else {
			out->c.badBounds = qFalse;
		}

		out->c.q2_contents = LittleLong (in->contents);

		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);

		out->q2_firstMarkSurface = model->q2BspModel.markSurfaces + LittleShort (in->firstLeafFace);
		out->q2_numMarkSurfaces = LittleShort (in->numLeafFaces);

		// Mark poly flags
		if (out->c.q2_contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			for (j=0 ; j<out->q2_numMarkSurfaces ; j++) {
				out->q2_firstMarkSurface[j]->q2_flags |= SURF_UNDERWATER;

				if (out->c.q2_contents & CONTENTS_LAVA)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_LAVA;
				if (out->c.q2_contents & CONTENTS_SLIME)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_SLIME;
				if (out->c.q2_contents & CONTENTS_WATER)
					out->q2_firstMarkSurface[j]->q2_flags |= SURF_WATER;
			}
		}

		if (!out->q2_numMarkSurfaces)
			continue;

		// Calculate the total amount of fragmentable surfaces
		numFragSurfaces = 0;
		for (j=0 ; j<out->q2_numMarkSurfaces ; j++) {
			surf = out->q2_firstMarkSurface[j];
			if (!surf)
				continue;
			if (R_Q2BSP_SurfPotentiallyVisible (surf)) {
				if (R_Q2BSP_SurfPotentiallyFragmented (surf))
					numFragSurfaces++;
			}
		}

		if (!numFragSurfaces)
			out->q2_firstDecalSurface = NULL;

		out->q2_firstDecalSurface = R_ModAllocZero (model, sizeof (mBspSurface_t *) * (numFragSurfaces + 1));

		// Store fragmentable surfaces
		numFragSurfaces = 0;
		for (j=0 ; j<out->q2_numMarkSurfaces ; j++) {
			surf = out->q2_firstMarkSurface[j];
			if (!surf)
				continue;
			if (R_Q2BSP_SurfPotentiallyVisible (surf)) {
				if (R_Q2BSP_SurfPotentiallyFragmented (surf))
					out->q2_firstDecalSurface[numFragSurfaces++] = surf;
			}
		}

		totalFragRemoved += out->q2_numMarkSurfaces - numFragSurfaces;
	}

	Com_DevPrintf (0, "R_LoadQ2BSPLeafs: %i non-fragmentable surfaces skipped.\n", totalFragRemoved);
	return qTrue;
}


/*
=================
R_LoadQ2BSPNodes
=================
*/
static qBool R_LoadQ2BSPNodes (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	int				i, p, j;
	dQ2BspNode_t	*in;
	mBspNode_t		*out;
	qBool			badBounds;
	uint16			firstSurface, numSurfaces;
	byte			*buffer;
	size_t			size;
	mBspSurface_t	*surf;
	uint16			numLitSurfs, numVisSurfs;
	uint32			totalLitRemoved, totalVisRemoved;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPNodes: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numNodes = lump->fileLen / sizeof (*in);
	model->bspModel.nodes = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numNodes);

	//
	// Byte swap
	//
	totalLitRemoved = 0;
	totalVisRemoved = 0;
	for (i=0 ; i<model->bspModel.numNodes ; i++, in++, out++) {
		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->c.mins[j] = LittleShort (in->mins[j]);
			out->c.maxs[j] = LittleShort (in->maxs[j]);

			if (out->c.mins[j] > out->c.maxs[j])
				badBounds = qTrue;
		}

		if (badBounds || Vec3Compare (out->c.mins, out->c.maxs)) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad node %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint (out->c.mins[0]), Q_rint (out->c.mins[1]), Q_rint (out->c.mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint (out->c.maxs[0]), Q_rint (out->c.maxs[1]), Q_rint (out->c.maxs[2]));
			out->c.badBounds = qTrue;
		}
		else {
			out->c.badBounds = qFalse;
		}

		out->c.q2_contents = -1;	// Differentiate from leafs

		out->c.plane = model->bspModel.planes + LittleLong (in->planeNum);
		firstSurface = LittleShort (in->firstFace);
		numSurfaces = LittleShort (in->numFaces);

		p = LittleLong (in->children[0]);
		out->children[0] = (p >= 0) ? model->bspModel.nodes + p : (mBspNode_t *)(model->bspModel.leafs + (-1 - p));

		p = LittleLong (in->children[1]);
		out->children[1] = (p >= 0) ? model->bspModel.nodes + p : (mBspNode_t *)(model->bspModel.leafs + (-1 - p));

		// Generate surface lists
		if (!numSurfaces) {
			out->q2_firstVisSurface = NULL;
			out->q2_firstLitSurface = NULL;
			continue;
		}

		// Find total visible/lit surfaces
		numLitSurfs = 0;
		numVisSurfs = 0;
		for (j=0 ; j<numSurfaces ; j++) {
			surf = model->bspModel.surfaces + firstSurface + j;
			if (R_Q2BSP_SurfPotentiallyVisible (surf)) {
				numVisSurfs++;
				if (R_Q2BSP_SurfPotentiallyLit (surf))
					numLitSurfs++;
			}
		}

		if (!numVisSurfs) {
			out->q2_firstVisSurface = NULL;
			out->q2_firstLitSurface = NULL;
			continue;
		}
		if (!numLitSurfs)
			out->q2_firstLitSurface = NULL;

		// Allocate space
		size = numVisSurfs + 1;
		if (numLitSurfs)
			size += numLitSurfs + 1;
		size *= sizeof (mBspSurface_t *);
		buffer = R_ModAllocZero (model, size);

		out->q2_firstVisSurface = (mBspSurface_t **)buffer;
		buffer += sizeof (mBspSurface_t *) * (numVisSurfs + 1);

		if (numLitSurfs)
			out->q2_firstLitSurface = (mBspSurface_t **)buffer;

		// Copy
		numLitSurfs = 0;
		numVisSurfs = 0;
		for (j=0 ; j<numSurfaces ; j++) {
			surf = model->bspModel.surfaces + firstSurface + j;
			if (R_Q2BSP_SurfPotentiallyVisible (surf)) {
				out->q2_firstVisSurface[numVisSurfs++] = surf;
				if (R_Q2BSP_SurfPotentiallyLit (surf))
					out->q2_firstLitSurface[numLitSurfs++] = surf;
			}
		}

		totalLitRemoved += numSurfaces - numLitSurfs;
		totalVisRemoved += numSurfaces - numVisSurfs;
	}
	Com_DevPrintf (0, "R_LoadQ2BSPNodes: %i non-visible %i non-lit surfaces skipped.\n", totalVisRemoved, totalVisRemoved+totalLitRemoved);

	//
	// Set the nodes and leafs
	//
	R_SetParentQ2BSPNode (model->bspModel.nodes, NULL);

	return qTrue;
}


/*
=================
R_LoadQ2BSPSubModels
=================
*/
static qBool R_LoadQ2BSPSubModels (refModel_t *model, byte *byteBase, const dQ2BspLump_t *lump)
{
	dQ2BspModel_t	*in;
	mBspHeader_t	*out;
	qBool			badBounds;
	uint32			i;
	int				j;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPSubModels: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numSubModels = lump->fileLen / sizeof (*in);
	if (model->bspModel.numSubModels >= MAX_REF_MODELS) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPSubModels: too many submodels %i >= %i\n", model->bspModel.numSubModels, MAX_REF_MODELS);
		return qFalse;
	}

	model->bspModel.subModels = out = R_ModAllocZero (model, sizeof (*out) * model->bspModel.numSubModels);
	model->bspModel.inlineModels = R_ModAllocZero (model, sizeof (refModel_t) * model->bspModel.numSubModels);

	//
	// Byte swap
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++, in++, out++) {
		// Pad the mins / maxs by a pixel
		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			if (out->mins[j] > out->maxs[j])
				badBounds = qTrue;
		}

		if (badBounds || Vec3Compare (out->mins, out->maxs)) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad submodel %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint (out->mins[0]), Q_rint (out->mins[1]), Q_rint (out->mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint (out->maxs[0]), Q_rint (out->maxs[1]), Q_rint (out->maxs[2]));
			out->radius = 0;
		}
		else {
			out->radius = RadiusFromBounds (out->mins, out->maxs);
		}

		out->origin[0]	= LittleFloat (in->origin[0]);
		out->origin[1]	= LittleFloat (in->origin[1]);
		out->origin[2]	= LittleFloat (in->origin[2]);

		out->headNode	= LittleLong (in->headNode);
		out->firstFace	= LittleLong (in->firstFace);
		out->numFaces	= LittleLong (in->numFaces);
	}

	return qTrue;
}


/*
=================
R_LoadQ2BSPModel
=================
*/
static qBool R_LoadQ2BSPModel (refModel_t *model, byte *buffer)
{
	dQ2BspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	int				version;
	uint32			i;

	//
	// Load the world model
	//
	model->type = MODEL_Q2BSP;

	header = (dQ2BspHeader_t *)buffer;
	version = LittleLong (header->version);
	if (version != Q2BSP_VERSION) {
		Com_Printf (PRNT_ERROR, "R_LoadQ2BSPModel: %s has wrong version number (%i should be %i)", model->name, version, Q2BSP_VERSION);
		return qFalse;
	}

	//
	// Swap all the lumps
	//
	modBase = (byte *)header;
	for (i=0 ; i<sizeof(dQ2BspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// Load into heap
	//
	if (!R_LoadQ2BSPVertexes	(model, modBase, &header->lumps[Q2BSP_LUMP_VERTEXES])
	|| !R_LoadQ2BSPEdges		(model, modBase, &header->lumps[Q2BSP_LUMP_EDGES])
	|| !R_LoadQ2BSPSurfEdges	(model, modBase, &header->lumps[Q2BSP_LUMP_SURFEDGES])
	|| !R_LoadQ2BSPLighting		(model, modBase, &header->lumps[Q2BSP_LUMP_LIGHTING])
	|| !R_LoadQ2BSPPlanes		(model, modBase, &header->lumps[Q2BSP_LUMP_PLANES])
	|| !R_LoadQ2BSPTexInfo		(model, modBase, &header->lumps[Q2BSP_LUMP_TEXINFO])
	|| !R_LoadQ2BSPFaces		(model, modBase, &header->lumps[Q2BSP_LUMP_FACES])
	|| !R_LoadQ2BSPMarkSurfaces	(model, modBase, &header->lumps[Q2BSP_LUMP_LEAFFACES])
	|| !R_LoadQ2BSPVisibility	(model, modBase, &header->lumps[Q2BSP_LUMP_VISIBILITY])
	|| !R_LoadQ2BSPLeafs		(model, modBase, &header->lumps[Q2BSP_LUMP_LEAFS])
	|| !R_LoadQ2BSPNodes		(model, modBase, &header->lumps[Q2BSP_LUMP_NODES])
	|| !R_LoadQ2BSPSubModels	(model, modBase, &header->lumps[Q2BSP_LUMP_MODELS]))
		return qFalse;

	//
	// Set up the submodels
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++) {
		refModel_t		*starmodel;

		bm = &model->bspModel.subModels[i];
		starmodel = &model->bspModel.inlineModels[i];

		*starmodel = *model;

		starmodel->bspModel.firstModelSurface = starmodel->bspModel.surfaces + bm->firstFace;
		starmodel->bspModel.numModelSurfaces = bm->numFaces;
		starmodel->q2BspModel.firstNode = bm->headNode;

		if (starmodel->q2BspModel.firstNode >= model->bspModel.numNodes) {
			Com_Printf (PRNT_ERROR, "R_LoadQ2BSPModel: Inline model number '%i' has a bad firstNode (%d >= %d)", i, starmodel->q2BspModel.firstNode, model->bspModel.numNodes);
			return qFalse;
		}

		Vec3Copy (bm->maxs, starmodel->maxs);
		Vec3Copy (bm->mins, starmodel->mins);
		starmodel->radius = bm->radius;

		if (i == 0)
			*model = *starmodel;

		starmodel->bspModel.numLeafs = bm->visLeafs;
	}

	return qTrue;
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

	if (ri.scn.worldModel->type != MODEL_Q3BSP)
		return NULL;
	if (!ri.scn.worldModel->q3BspModel.numFogs)
		return NULL;

	defaultFog = NULL;
	fog = ri.scn.worldModel->q3BspModel.fogs;
	for (i=0 ; i<ri.scn.worldModel->q3BspModel.numFogs ; i++, fog++) {
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
byte *R_Q3BSPClusterPVS (int cluster, refModel_t *model)
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
mBspLeaf_t *R_PointInQ3BSPLeaf (vec3_t p, refModel_t *model)
{
	mBspNode_t	*node;
	cBspPlane_t	*plane;
	
	if (!model || !model->bspModel.nodes)
		Com_Error (ERR_DROP, "R_PointInQ3BSPLeaf: bad model");

	node = model->bspModel.nodes;
	do {
		plane = node->c.plane;
		node = node->children[PlaneDiff (p, plane) < 0];
	} while (node->c.plane);

	return (mBspLeaf_t *)node;
}


/*
=================
R_SetParentQ3BSPNode
=================
*/
static void R_SetParentQ3BSPNode (mBspNode_t *node, mBspNode_t *parent)
{
	node->c.parent = parent;
	if (!node->c.plane)
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

	if (!surf->mesh || RB_InvalidMesh (surf->mesh))
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
static qBool R_LoadQ3BSPLighting (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lightLump, const dQ3BspLump_t *gridLump)
{
	dQ3BspGridLight_t 	*inGrid;

	// Load lighting
	if (lightLump->fileLen && !r_vertexLighting->intVal) {
		if (lightLump->fileLen % Q3LIGHTMAP_SIZE) {
			Com_Printf (PRNT_ERROR, "R_LoadQ3BSPLighting: funny lighting lump size in %s", model->name);
			return qFalse;
		}

		model->q3BspModel.numLightmaps = lightLump->fileLen / Q3LIGHTMAP_SIZE;
		model->q3BspModel.lightmapRects = R_ModAllocZero (model, model->q3BspModel.numLightmaps * sizeof (*model->q3BspModel.lightmapRects));
	}

	// Load the light grid
	if (gridLump->fileLen % sizeof (*inGrid)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPLighting: funny lightgrid lump size in %s", model->name);
		return qFalse;
	}

	inGrid = (void *)(byteBase + gridLump->fileOfs);
	model->q3BspModel.numLightGridElems = gridLump->fileLen / sizeof (*inGrid);
	model->q3BspModel.lightGrid = R_ModAllocZero (model, model->q3BspModel.numLightGridElems * sizeof (*model->q3BspModel.lightGrid));

	memcpy (model->q3BspModel.lightGrid, inGrid, model->q3BspModel.numLightGridElems * sizeof (*model->q3BspModel.lightGrid));

	return qTrue;
}


/*
=================
R_LoadQ3BSPVisibility
=================
*/
static qBool R_LoadQ3BSPVisibility (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	if (!lump->fileLen) {
		model->q3BspModel.vis = NULL;
		return qTrue;
	}

	model->q3BspModel.vis = R_ModAllocZero (model, lump->fileLen);
	memcpy (model->q3BspModel.vis, byteBase + lump->fileOfs, lump->fileLen);

	model->q3BspModel.vis->numClusters = LittleLong (model->q3BspModel.vis->numClusters);
	model->q3BspModel.vis->rowSize = LittleLong (model->q3BspModel.vis->rowSize);

	return qTrue;
}


/*
=================
R_LoadQ3BSPVertexes
=================
*/
static qBool R_LoadQ3BSPVertexes (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, count;
	dQ3BspVertex_t	*in;
	float			*outVerts, *outNormals, *outCoords, *outLMCoords;
	byte			*outColors, *buffer;
	vec3_t			color, fcolor;
	float			div;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPVertexes: funny lump size in %s", model->name);
		return qFalse;
	}
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

	outVerts = model->q3BspModel.vertexArray[0];
	outNormals = model->q3BspModel.normalsArray[0];
	outCoords = model->q3BspModel.coordArray[0];
	outLMCoords = model->q3BspModel.lmCoordArray[0];
	outColors = model->q3BspModel.colorArray[0];

	if (r_lmModulate->intVal > 0)
		div = (float)(1 << r_lmModulate->intVal) / 255.0f;
	else
		div = 1.0f / 255.0f;

	for (i=0 ; i<count ; i++, in++) {
		outVerts[0] = LittleFloat (in->point[0]);
		outVerts[1] = LittleFloat (in->point[1]);
		outVerts[2] = LittleFloat (in->point[2]);
		outNormals[0] = LittleFloat (in->normal[0]);
		outNormals[1] = LittleFloat (in->normal[1]);
		outNormals[2] = LittleFloat (in->normal[2]);
		outCoords[0] = LittleFloat (in->texCoords[0]);
		outCoords[1] = LittleFloat (in->texCoords[1]);
		outLMCoords[0] = LittleFloat (in->lmCoords[0]);
		outLMCoords[1] = LittleFloat (in->lmCoords[1]);

		if (r_fullbright->intVal) {
			outColors[0] = 255;
			outColors[1] = 255;
			outColors[2] = 255;
			outColors[3] = in->color[3];
		}
		else {
			color[0] = ((float)in->color[0] * div);
			color[1] = ((float)in->color[1] * div);
			color[2] = ((float)in->color[2] * div);
			ColorNormalizef (color, fcolor);

			outColors[0] = (byte)(fcolor[0] * 255);
			outColors[1] = (byte)(fcolor[1] * 255);
			outColors[2] = (byte)(fcolor[2] * 255);
			outColors[3] = in->color[3];
		}

		outVerts += 3;
		outNormals += 3;
		outCoords += 2;
		outLMCoords += 2;
		outColors += 4;
	}

	return qTrue;
}


/*
=================
R_LoadQ3BSPSubmodels
=================
*/
static qBool R_LoadQ3BSPSubmodels (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	dQ3BspModel_t	*in;
	mBspHeader_t	*out;
	uint32			i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof(*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPSubmodels: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numSubModels = lump->fileLen / sizeof (*in);
	if (model->bspModel.numSubModels >= MAX_REF_MODELS) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPSubmodels: too many submodels %i >= %i\n", model->bspModel.numSubModels, MAX_REF_MODELS);
		return qFalse;
	}

	model->bspModel.subModels = out = R_ModAllocZero (model, model->bspModel.numSubModels * sizeof (*out));
	model->bspModel.inlineModels = R_ModAllocZero (model, sizeof (refModel_t) * model->bspModel.numSubModels);

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

	return qTrue;
}


/*
=================
R_LoadQ3BSPShaderRefs
=================
*/
static qBool R_LoadQ3BSPShaderRefs (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int 				i;
	dQ3BspShaderRef_t	*in;
	mQ3BspShaderRef_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPShaderRefs: funny lump size in %s", model->name);	
		return qFalse;
	}

	model->q3BspModel.numShaderRefs = lump->fileLen / sizeof (*in);
	model->q3BspModel.shaderRefs = out = R_ModAllocZero (model, model->q3BspModel.numShaderRefs * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numShaderRefs ; i++, in++, out++) {
		Q_strncpyz (out->name, in->name, sizeof (out->name));
		out->flags = LittleLong (in->flags);
		out->contents = LittleLong (in->contents);
		out->shader = NULL;
	}

	return qTrue;
}


/*
=================
R_CreateQ3BSPMeshForSurface
=================
*/
#define COLOR_RGB(r,g,b)	(((r) << 0)|((g) << 8)|((b) << 16))
#define COLOR_RGBA(r,g,b,a) (((r) << 0)|((g) << 8)|((b) << 16)|((a) << 24))
static mesh_t *R_CreateQ3BSPMeshForSurface (refModel_t *model, dQ3BspFace_t *in, mBspSurface_t *out)
{
	static vec3_t	tempNormalsArray[RB_MAX_VERTS];
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
			Vec3Copy (out->q3_origin, mesh->vertexArray[0]);

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
			static vec4_t	colors[RB_MAX_VERTS];
			static vec4_t	colors2[RB_MAX_VERTS];
			index_t			*indexes;
			byte			*buffer;

			patch_cp[0] = LittleLong (in->patch_cp[0]);
			patch_cp[1] = LittleLong (in->patch_cp[1]);

			if (!patch_cp[0] || !patch_cp[1])
				break;

			subdivLevel = bound (1, r_patchDivLevel->intVal, 32);

			numVerts = LittleLong (in->numVerts);
			firstVert = LittleLong (in->firstVert);
			for (i=0 ; i<numVerts ; i++)
				Vec4Scale (model->q3BspModel.colorArray[firstVert + i], (1.0 / 255.0), colors[i]);

			// Find the degree of subdivision in the u and v directions
			Patch_GetFlatness (subdivLevel, &model->q3BspModel.vertexArray[firstVert], patch_cp, flat);

			// Allocate space for mesh
			step[0] = (1 << flat[0]);
			step[1] = (1 << flat[1]);
			size[0] = (patch_cp[0] >> 1) * step[0] + 1;
			size[1] = (patch_cp[1] >> 1) * step[1] + 1;
			numVerts = size[0] * size[1];

			if (numVerts > RB_MAX_VERTS)
				break;

			out->q3_patchWidth = size[0];
			out->q3_patchHeight = size[1];

			buffer = R_ModAllocZero (model, sizeof (mesh_t)
				+ (numVerts * sizeof (vec2_t) * 2)
				+ (numVerts * sizeof (vec3_t) * 2)
				+ (numVerts * sizeof (bvec4_t))
				+ (numVerts * sizeof (index_t) * ((size[0]-1) * (size[1]-1) * 6)));

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
	vec2_t		*stArray;
	index_t		*quad;
	mesh_t		*mesh;
	shader_t	*shader;
	int			i, j;

	if ((surf->q3_faceType != FACETYPE_PLANAR && surf->q3_faceType != FACETYPE_TRISURF) || !surf->q3_shaderRef)
		return;

	mesh = surf->mesh;
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
			if (stArray[quad[j]][0] < -0.1f || stArray[quad[j]][0] > 1.1f || stArray[quad[j]][1] < -0.1f || stArray[quad[j]][1] > 1.1f) {
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
static qBool R_LoadQ3BSPFaces (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int					j;
	dQ3BspFace_t		*in;
	mBspSurface_t 		*out;
	mesh_t				*mesh;
	mQ3BspFog_t			*fog;
	mQ3BspShaderRef_t	*shaderRef;
	int					shaderNum, fogNum, surfNum;
	float				*vert;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "Mod_LoadFaces: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numSurfaces = lump->fileLen / sizeof (*in);
	model->bspModel.surfaces = out = R_ModAllocZero (model, model->bspModel.numSurfaces * sizeof (*out));

	// Fill it in
	for (surfNum=0 ; surfNum<model->bspModel.numSurfaces ; surfNum++, in++, out++) {
		out->q3_origin[0] = LittleFloat (in->origin[0]);
		out->q3_origin[1] = LittleFloat (in->origin[1]);
		out->q3_origin[2] = LittleFloat (in->origin[2]);

		out->q3_faceType = LittleLong (in->faceType);

		// Lighting info
		if (r_vertexLighting->intVal) {
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
		shaderNum = LittleLong (in->shaderNum);
		if (shaderNum < 0 || shaderNum >= model->q3BspModel.numShaderRefs) {
			Com_Printf (PRNT_ERROR, "R_LoadQ3BSPFaces: bad shader number");
			return qFalse;
		}

		shaderRef = model->q3BspModel.shaderRefs + shaderNum;
		out->q3_shaderRef = shaderRef;

		if (!shaderRef->shader) {
			if (out->q3_faceType == FACETYPE_FLARE) {
				shaderRef->shader = R_RegisterFlare (shaderRef->name);
				if (!shaderRef->shader) {
					Com_Printf (PRNT_WARNING, "Couldn't load (flare): '%s'\n", shaderRef->name);
					shaderRef->shader = r_noShader;
				}
			}
			else {
				if (out->q3_faceType == FACETYPE_TRISURF || r_vertexLighting->intVal || out->lmTexNum < 0) {
					if (out->q3_faceType != FACETYPE_TRISURF && !r_vertexLighting->intVal && out->lmTexNum < 0)
						Com_DevPrintf (PRNT_WARNING, "WARNING: surface '%s' has a lightmap but no lightmap stage!\n", shaderRef->name);

					shaderRef->shader = R_RegisterTextureVertex (shaderRef->name);
					if (!shaderRef->shader) {
						Com_Printf (PRNT_WARNING, "Couldn't load (vertex): '%s'\n", shaderRef->name);
						shaderRef->shader = r_noShader;
					}
				}
				else {
					shaderRef->shader = R_RegisterTextureLM (shaderRef->name);
					if (!shaderRef->shader) {
						Com_Printf (PRNT_WARNING, "Couldn't load (lm): '%s'\n", shaderRef->name);
						shaderRef->shader = r_noShaderLightmap;
					}
				}
			}
		}

		// Fog
		fogNum = LittleLong (in->fogNum);
		if (fogNum != -1 && fogNum < model->q3BspModel.numFogs) {
			fog = model->q3BspModel.fogs + fogNum;
			if (fog->numPlanes && fog->shader && fog->name[0])
				out->q3_fog = fog;
		}

		// Mesh
		mesh = out->mesh = R_CreateQ3BSPMeshForSurface (model, in, out);
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

	return qTrue;
}


/*
=================
R_LoadQ3BSPNodes
=================
*/
static qBool R_LoadQ3BSPNodes (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, j, p;
	dQ3BspNode_t	*in;
	mBspNode_t 		*out;
	qBool			badBounds;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPNodes: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numNodes = lump->fileLen / sizeof(*in);
	model->bspModel.nodes = out = R_ModAllocZero (model, model->bspModel.numNodes * sizeof (*out));

	for (i=0 ; i<model->bspModel.numNodes ; i++, in++, out++) {
		out->c.plane = model->bspModel.planes + LittleLong (in->planeNum);

		for (j=0 ; j<2 ; j++) {
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = model->bspModel.nodes + p;
			else
				out->children[j] = (mBspNode_t *)(model->bspModel.leafs + (-1 - p));
		}

		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->c.mins[j] = LittleFloat (in->mins[j]);
			out->c.maxs[j] = LittleFloat (in->maxs[j]);
			if (out->c.mins[j] > out->c.maxs[j])
				badBounds = qTrue;
		}

		if (badBounds || Vec3Compare (out->c.mins, out->c.maxs)) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad node %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint (out->c.mins[0]), Q_rint (out->c.mins[1]), Q_rint (out->c.mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint (out->c.maxs[0]), Q_rint (out->c.maxs[1]), Q_rint (out->c.maxs[2]));
			out->c.badBounds = qTrue;
		}
	}

	return qTrue;
}


/*
=================
R_LoadQ3BSPFogs
=================
*/
static qBool R_LoadQ3BSPFogs (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump, const dQ3BspLump_t *brLump, const dQ3BspLump_t *brSidesLump)
{
	int					i, j, p;
	dQ3BspFog_t 		*in;
	mQ3BspFog_t			*out;
	dQ3BspBrush_t 		*inBrushes, *brush;
	dQ3BspBrushSide_t	*inBrushSides, *brushSide;

	inBrushes = (void *)(byteBase + brLump->fileOfs);
	if (brLump->fileLen % sizeof (*inBrushes)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);
		return qFalse;
	}

	inBrushSides = (void *)(byteBase + brSidesLump->fileOfs);
	if (brSidesLump->fileLen % sizeof (*inBrushSides)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);
		return qFalse;
	}

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPFogs: funny lump size in %s", model->name);
		return qFalse;
	}

	model->q3BspModel.numFogs = lump->fileLen / sizeof (*in);
	if (!model->q3BspModel.numFogs)
		return qTrue;

	model->q3BspModel.fogs = out = R_ModAllocZero (model, model->q3BspModel.numFogs * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numFogs ; i++, in++, out++) {
		Q_strncpyz (out->name, in->shader, sizeof (out->name));
		out->shader = R_RegisterTextureLM (in->shader);

		p = LittleLong (in->brushNum);
		if (p == -1)
			continue;	 // Global fog
		brush = inBrushes + p;

		p = LittleLong (brush->firstSide);
		if (p == -1) {
			out->name[0] = '\0';
			out->shader = NULL;
			continue;
		}
		brushSide = inBrushSides + p;

		p = LittleLong (in->visibleSide);
		if (p == -1) {
			out->name[0] = '\0';
			out->shader = NULL;
			continue;
		}

		out->numPlanes = LittleLong (brush->numSides);
		out->planes = R_ModAllocZero (model, out->numPlanes * sizeof (cBspPlane_t));

		out->visiblePlane = model->bspModel.planes + LittleLong (brushSide[p].planeNum);
		for (j=0 ; j<out->numPlanes; j++)
			out->planes[j] = *(model->bspModel.planes + LittleLong (brushSide[j].planeNum));
	}

	return qTrue;
}


/*
=================
R_LoadQ3BSPLeafs
=================
*/
static qBool R_LoadQ3BSPLeafs (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump, const dQ3BspLump_t *msLump)
{
	int				i, j, k, countMarkSurfaces;
	dQ3BspLeaf_t 	*in;
	mBspLeaf_t 		*out;
	size_t			size;
	byte			*buffer;
	qBool			badBounds;
	int				*inMarkSurfaces;
	int				numMarkSurfaces, firstMarkSurface;
	int				numVisSurfaces, numLitSurfaces, numFragSurfaces;
	mBspSurface_t	*surf;

	inMarkSurfaces = (void *)(byteBase + msLump->fileOfs);
	if (msLump->fileLen % sizeof (*inMarkSurfaces)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPLeafs: funny lump size in %s", model->name);
		return qFalse;
	}
	countMarkSurfaces = msLump->fileLen / sizeof (*inMarkSurfaces);

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPLeafs: funny lump size in %s", model->name);
		return qFalse;
	}

	model->bspModel.numLeafs = lump->fileLen / sizeof (*in);
	model->bspModel.leafs = out = R_ModAllocZero (model, model->bspModel.numLeafs * sizeof (*out));

	for (i=0 ; i<model->bspModel.numLeafs ; i++, in++, out++) {
		badBounds = qFalse;
		for (j=0 ; j<3 ; j++) {
			out->c.mins[j] = (float)LittleLong (in->mins[j]);
			out->c.maxs[j] = (float)LittleLong (in->maxs[j]);
			if (out->c.mins[j] > out->c.maxs[j])
				badBounds = qTrue;
		}
		out->cluster = LittleLong (in->cluster);

		if (i && (badBounds || Vec3Compare (out->c.mins, out->c.maxs))) {
			Com_DevPrintf (PRNT_WARNING, "WARNING: bad leaf %i bounds:\n", i);
			Com_DevPrintf (PRNT_WARNING, "mins: %i %i %i\n", Q_rint (out->c.mins[0]), Q_rint (out->c.mins[1]), Q_rint (out->c.mins[2]));
			Com_DevPrintf (PRNT_WARNING, "maxs: %i %i %i\n", Q_rint (out->c.maxs[0]), Q_rint (out->c.maxs[1]), Q_rint (out->c.maxs[2]));
			Com_DevPrintf (PRNT_WARNING, "cluster: %i\n", out->cluster);
			Com_DevPrintf (PRNT_WARNING, "surfaces: %i\n", LittleLong (in->numLeafFaces));
			Com_DevPrintf (PRNT_WARNING, "brushes: %i\n", LittleLong (in->numLeafBrushes));
			out->c.badBounds = qTrue;
			out->cluster = -1;
		}

		if (model->q3BspModel.vis && out->cluster >= model->q3BspModel.vis->numClusters) {
			Com_Printf (PRNT_ERROR, "MOD_LoadBmodel: leaf cluster > numclusters");
			return qFalse;
		}

		out->c.plane = NULL;
		out->area = LittleLong (in->area) + 1;

		numMarkSurfaces = LittleLong (in->numLeafFaces);
		if (!numMarkSurfaces)
			continue;

		firstMarkSurface = LittleLong (in->firstLeafFace);
		if (firstMarkSurface < 0 || numMarkSurfaces + firstMarkSurface > countMarkSurfaces) {
			Com_Printf (PRNT_ERROR, "MOD_LoadBmodel: bad marksurfaces in leaf %i", i);
			return qFalse;
		}

		// Count how many surfaces we're going to have in our lists
		numVisSurfaces = numLitSurfaces = numFragSurfaces = 0;
		for (j=0 ; j<numMarkSurfaces ; j++) {
			k = LittleLong (inMarkSurfaces[firstMarkSurface + j]);
			if (k < 0 || k >= model->bspModel.numSurfaces) {
				Com_Printf (PRNT_ERROR, "R_LoadQ3BSPLeafs: bad surface number");
				return qFalse;
			}

			if (R_Q3BSP_SurfPotentiallyVisible (model->bspModel.surfaces + k)) {
				numVisSurfaces++;

				if (R_Q3BSP_SurfPotentiallyLit (model->bspModel.surfaces + k))
					numLitSurfaces++;

				if (R_Q3BSP_SurfPotentiallyFragmented (model->bspModel.surfaces + k))
					numFragSurfaces++;
			}
		}

		if (!numVisSurfaces)
			continue;

		// Allocate a buffer
		size = numVisSurfaces + 1;
		if (numLitSurfaces)
			size += numLitSurfaces + 1;
		if (numFragSurfaces)
			size += numFragSurfaces + 1;
		size *= sizeof (mBspSurface_t *);

		buffer = (byte *) R_ModAllocZero (model, size);
		out->q3_firstVisSurface = (mBspSurface_t **)buffer;
		buffer += (numVisSurfaces + 1) * sizeof (mBspSurface_t *);
		if (numLitSurfaces) {
			out->q3_firstLitSurface = (mBspSurface_t **)buffer;
			buffer += (numLitSurfaces + 1) * sizeof (mBspSurface_t *);
		}
		if (numFragSurfaces) {
			out->q3_firstFragmentSurface = (mBspSurface_t **)buffer;
			buffer += (numFragSurfaces + 1) * sizeof (mBspSurface_t *);
		}

		// Store surface list
		numVisSurfaces = numLitSurfaces = numFragSurfaces = 0;
		for (j=0 ; j<numMarkSurfaces ; j++) {
			k = LittleLong (inMarkSurfaces[firstMarkSurface + j]);
			surf = model->bspModel.surfaces + k;

			if (R_Q3BSP_SurfPotentiallyVisible (model->bspModel.surfaces + k)) {
				out->q3_firstVisSurface[numVisSurfaces++] = surf;

				if (R_Q3BSP_SurfPotentiallyLit (model->bspModel.surfaces + k))
					out->q3_firstLitSurface[numLitSurfaces++] = surf;

				if (R_Q3BSP_SurfPotentiallyFragmented (model->bspModel.surfaces + k))
					out->q3_firstFragmentSurface[numFragSurfaces++] = surf;
			}
		}
	}

	return qTrue;
}


/*
=================
R_LoadQ3BSPEntities
=================
*/
static qBool R_LoadQ3BSPEntities (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	char			*data;
	mQ3BspLight_t	*out;
	int				count, total, gridsizei[3];
	qBool			isLight, isWorld;
	float			scale, gridsizef[3];
	char			key[MAX_KEY], value[MAX_VALUE], target[MAX_VALUE], *token;
	parse_t			*ps;

	data = (char *)byteBase + lump->fileOfs;
	if (!data || !data[0])
		return qTrue;

	Vec3Clear (gridsizei);
	Vec3Clear (gridsizef);

	ps = PS_StartSession (data, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE);
	for (total=0 ; PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) && token[0] == '{' ; ) {
		isLight = qFalse;
		isWorld = qFalse;

		for ( ; ; ) {
			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token))
				break;	// Error
			if (token[0] == '}')
				break;	// End of entity

			Q_strncpyz (key, token, sizeof (key));
			while (key[strlen(key)-1] == ' ')	// remove trailing spaces
				key[strlen(key)-1] = 0;

			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token))
				break;	// Error

			Q_strncpyz (value, token, sizeof (value));

			// Now that we have the key pair worked out...
			if (!strcmp (key, "classname")) {
				if (!strncmp (value, "light", 5))
					isLight = qTrue;
				else if (!strcmp (value, "worldspawn"))
					isWorld = qTrue;
			}
			else if (!strcmp (key, "gridsize")) {
				sscanf (value, "%f %f %f", &gridsizef[0], &gridsizef[1], &gridsizef[2]);

				if (!gridsizef[0] || !gridsizef[1] || !gridsizef[2]) {
					sscanf (value, "%i %i %i", &gridsizei[0], &gridsizei[1], &gridsizei[2]);
					Vec3Copy (gridsizei, gridsizef);
				}
			}
		}

		if (isWorld) {
			Vec3Copy (gridsizef, model->q3BspModel.gridSize);
			continue;
		}

		if (isLight)
			total++;
	}
	PS_EndSession (ps);

#if !(SHADOW_VOLUMES)
	total = 0;
#endif

	if (!total)
		return qTrue;

	out = R_ModAllocZero (model, total * sizeof (*out));
	model->q3BspModel.worldLights = out;
	model->q3BspModel.numWorldLights = total;

	data = (char *)byteBase + lump->fileOfs;
	ps = PS_StartSession (data, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE);
	for (count=0 ; PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) && token[0] == '{' ; ) {
		if (count == total)
			break;

		isLight = qFalse;

		for ( ; ; ) {
			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token))
				break;	// Error
			if (token[0] == '}')
				break;	// End of entity

			Q_strncpyz (key, token, sizeof (key));
			while (key[strlen(key)-1] == ' ')		// Remove trailing spaces
				key[strlen(key)-1] = 0;

			if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token))
				break;	// Error

			Q_strncpyz (value, token, sizeof (value));

			// Now that we have the key pair worked out...
			if (!strcmp (key, "origin"))
				sscanf (value, "%f %f %f", &out->origin[0], &out->origin[1], &out->origin[2]);
			else if (!strcmp (key, "color") || !strcmp (key, "_color"))
				sscanf (value, "%f %f %f", &out->color[0], &out->color[1], &out->color[2]);
			else if (!strcmp (key, "light") || !strcmp (key, "_light"))
				out->intensity = atof (value);
			else if (!strcmp (key, "classname")) {
				if (!strncmp (value, "light", 5))
					isLight = qTrue;
			}
			else if (!strcmp (key, "target"))
				Q_strncpyz (target, value, sizeof (target));
		}

		if (!isLight)
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
			Vec3Scale (out->color, scale, out->color);
		}

		out++;
		count++;
	}
	PS_EndSession (ps);

	return qTrue;
}


/*
=================
R_LoadQ3BSPIndexes
=================
*/
static qBool R_LoadQ3BSPIndexes (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int		i, *in, *out;
	
	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPIndexes: funny lump size in %s", model->name);
		return qFalse;
	}

	model->q3BspModel.numSurfIndexes = lump->fileLen / sizeof (*in);
	model->q3BspModel.surfIndexes = out = R_ModAllocZero (model, model->q3BspModel.numSurfIndexes * sizeof (*out));

	for (i=0 ; i<model->q3BspModel.numSurfIndexes ; i++)
		out[i] = LittleLong (in[i]);

	return qTrue;
}


/*
=================
R_LoadQ3BSPPlanes
=================
*/
static qBool R_LoadQ3BSPPlanes (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lump)
{
	int				i, j;
	cBspPlane_t		*out;
	dQ3BspPlane_t 	*in;
	
	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in)) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPPlanes: funny lump size in %s", model->name);
		return qFalse;
	}

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

	return qTrue;
}


/*
=================
R_FinishQ3BSPModel
=================
*/
static void R_FinishQ3BSPModel (refModel_t *model, byte *byteBase, const dQ3BspLump_t *lightmaps)
{
	mesh_t					*mesh;
	mBspSurface_t 			*surf;
	mQ3BspLightmapRect_t	*lmRect;
	float					*lmArray;
	int						i, j;

	R_Q3BSP_BuildLightmaps (model->q3BspModel.numLightmaps, Q3LIGHTMAP_WIDTH, Q3LIGHTMAP_WIDTH, byteBase + lightmaps->fileOfs, model->q3BspModel.lightmapRects);

	// Now walk list of surface and apply lightmap info
	for (i=0, surf=model->bspModel.surfaces ; i<model->bspModel.numSurfaces ; i++, surf++) {
		if (surf->lmTexNum < 0 || surf->q3_faceType == FACETYPE_FLARE || !(mesh = surf->mesh)) {
			surf->lmTexNum = -1;
		}
		else {
			lmRect = &model->q3BspModel.lightmapRects[surf->lmTexNum];
			surf->lmTexNum = lmRect->texNum;

			if (r_lmPacking->intVal) {
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
static qBool R_LoadQ3BSPModel (refModel_t *model, byte *buffer)
{
	dQ3BspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	vec3_t			maxs;
	int				version;
	uint32			i;

	//
	// Load the world model
	//
	model->type = MODEL_Q3BSP;

	header = (dQ3BspHeader_t *)buffer;
	version = LittleLong (header->version);
	if (version != Q3BSP_VERSION) {
		Com_Printf (PRNT_ERROR, "R_LoadQ3BSPModel: %s has wrong version number (%i should be %i)", model->name, version, Q3BSP_VERSION);
		return qFalse;
	}

	//
	// Swap all the lumps
	//
	modBase = (byte *)header;
	for (i=0 ; i<sizeof (dQ3BspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// Load into heap
	//
	if (!R_LoadQ3BSPEntities	(model, modBase, &header->lumps[Q3BSP_LUMP_ENTITIES])
	|| !R_LoadQ3BSPVertexes		(model, modBase, &header->lumps[Q3BSP_LUMP_VERTEXES])
	|| !R_LoadQ3BSPIndexes		(model, modBase, &header->lumps[Q3BSP_LUMP_INDEXES])
	|| !R_LoadQ3BSPLighting		(model, modBase, &header->lumps[Q3BSP_LUMP_LIGHTING], &header->lumps[Q3BSP_LUMP_LIGHTGRID])
	|| !R_LoadQ3BSPVisibility	(model, modBase, &header->lumps[Q3BSP_LUMP_VISIBILITY])
	|| !R_LoadQ3BSPShaderRefs	(model, modBase, &header->lumps[Q3BSP_LUMP_SHADERREFS])
	|| !R_LoadQ3BSPPlanes		(model, modBase, &header->lumps[Q3BSP_LUMP_PLANES])
	|| !R_LoadQ3BSPFogs			(model, modBase, &header->lumps[Q3BSP_LUMP_FOGS], &header->lumps[Q3BSP_LUMP_BRUSHES], &header->lumps[Q3BSP_LUMP_BRUSHSIDES])
	|| !R_LoadQ3BSPFaces		(model, modBase, &header->lumps[Q3BSP_LUMP_FACES])
	|| !R_LoadQ3BSPLeafs		(model, modBase, &header->lumps[Q3BSP_LUMP_LEAFS], &header->lumps[Q3BSP_LUMP_LEAFFACES])
	|| !R_LoadQ3BSPNodes		(model, modBase, &header->lumps[Q3BSP_LUMP_NODES])
	|| !R_LoadQ3BSPSubmodels	(model, modBase, &header->lumps[Q3BSP_LUMP_MODELS]))
		return qFalse;

	// Finishing touches
	R_FinishQ3BSPModel			(model, modBase, &header->lumps[Q3BSP_LUMP_LIGHTING]);

	//
	// Set up the submodels
	//
	for (i=0 ; i<model->bspModel.numSubModels ; i++) {
		refModel_t		*starmod;

		bm = &model->bspModel.subModels[i];
		starmod = &model->bspModel.inlineModels[i];

		*starmod = *model;

		starmod->bspModel.firstModelSurface = starmod->bspModel.surfaces + bm->firstFace;
		starmod->bspModel.numModelSurfaces = bm->numFaces;

		Vec3Copy (bm->maxs, starmod->maxs);
		Vec3Copy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*model = *starmod;
	}

	//
	// Set up lightgrid
	//
	if (model->q3BspModel.gridSize[0] < 1 || model->q3BspModel.gridSize[1] < 1 || model->q3BspModel.gridSize[2] < 1)
		Vec3Set (model->q3BspModel.gridSize, 64, 64, 128);

	for (i=0 ; i<3 ; i++) {
		model->q3BspModel.gridMins[i] = model->q3BspModel.gridSize[i] * ceil ((model->mins[i] + 1) / model->q3BspModel.gridSize[i]);
		maxs[i] = model->q3BspModel.gridSize[i] * floor ((model->maxs[i] - 1) / model->q3BspModel.gridSize[i]);
		model->q3BspModel.gridBounds[i] = (maxs[i] - model->q3BspModel.gridMins[i])/model->q3BspModel.gridSize[i] + 1;
	}
	model->q3BspModel.gridBounds[3] = model->q3BspModel.gridBounds[1] * model->q3BspModel.gridBounds[0];

	return qTrue;
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
static void R_FreeModel (refModel_t *model)
{
	refModel_t	*hashMdl;
	refModel_t	**prev;

	assert (model);
	if (!model)
		return;

	// De-link it from the hash tree
	prev = &r_modelHashTree[model->hashValue];
	for ( ; ; ) {
		hashMdl = *prev;
		if (!hashMdl)
			break;

		if (hashMdl == model) {
			*prev = hashMdl->hashNext;
			break;
		}
		prev = &hashMdl->hashNext;
	}

	// Free it
	if (model->memSize > 0)
		Mem_FreeTag (ri.modelSysPool, model->memTag);

	// Clear the spot
	model->touchFrame = 0;
}


/*
================
R_TouchModel

Touches/loads all textures for the model type
================
*/
static void R_TouchModel (refModel_t *model)
{
	mAliasModel_t		*aliasModel;
	mAliasMesh_t		*aliasMesh;
	mAliasSkin_t		*aliasSkin;
	mBspSurface_t		*surf;
	mQ2BspTexInfo_t		*ti;
	mQ3BspFog_t			*fog;
	mQ3BspShaderRef_t	*shaderref;
	mSpriteModel_t		*spriteModel;
	mSpriteFrame_t		*spriteFrame;
	int					i, j;

	ri.reg.modelsTouched++;
	model->touchFrame = ri.reg.registerFrame;

	switch (model->type) {
	case MODEL_MD2:
	case MODEL_MD3:
		aliasModel = model->aliasModel;
		for (i=0, aliasMesh=aliasModel->meshes ; i<aliasModel->numMeshes ; aliasMesh++, i++) {
			for (j=0, aliasSkin=aliasMesh->skins ; j<aliasMesh->numSkins ; aliasSkin++, j++) {
				if (!aliasSkin->name[0]) {
					aliasSkin->skin = NULL;
					continue;
				}

				aliasSkin->skin = R_RegisterSkin (aliasSkin->name);
			}
		}
		break;

	case MODEL_Q2BSP:
		for (i=0, ti=model->q2BspModel.texInfo ; i<model->q2BspModel.numTexInfo ; ti++, i++) {
			if (ti->flags & SURF_TEXINFO_SKY) {
				ti->shader = r_noShaderSky;
				continue;
			}

			ti->shader = R_RegisterTexture (ti->texName, ti->surfParams);
			if (!ti->shader) {
				if (ti->surfParams & SHADER_SURF_LIGHTMAP)
					ti->shader = r_noShaderLightmap;
				else
					ti->shader = r_noShader;
			}
		}

		R_TouchLightmaps ();
		break;

	case MODEL_Q3BSP:
		for (i=0, surf=model->bspModel.surfaces ; i<model->bspModel.numSurfaces ; surf++, i++) {
			shaderref = surf->q3_shaderRef;

			if (surf->q3_faceType == FACETYPE_FLARE) {
				shaderref->shader = R_RegisterFlare (shaderref->name);
				if (!shaderref->shader)
					shaderref->shader = r_noShader;
			}
			else {
				if (surf->q3_faceType == FACETYPE_TRISURF || r_vertexLighting->intVal || surf->lmTexNum < 0) {
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

		for (i=0, fog=model->q3BspModel.fogs ; i<model->q3BspModel.numFogs ; fog++, i++) {
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
		for (i=0, spriteFrame=spriteModel->frames ; i<spriteModel->numFrames ; spriteFrame++, i++) {
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
static inline refModel_t *R_FindModel (char *bareName)
{
	refModel_t	*model;
	uint32		hash;

	assert (bareName && bareName[0]);
	ri.reg.modelsSeaked++;

	hash = Com_HashGeneric (bareName, MAX_REF_MODEL_HASH);

	// Search the currently loaded models
	for (model=r_modelHashTree[hash] ; model ; model=model->hashNext) {
		if (!model->touchFrame)
			continue;	// Free r_modelList slot

		// Check name
		if (!strcmp (bareName, model->bareName))
			return model;
	}

	return NULL;
}


/*
================
R_GetModelSlot
================
*/
static inline refModel_t *R_GetModelSlot (void)
{
	refModel_t	*model;
	uint32		i;

	// Find a free model slot spot
	for (i=MODLIST_OFFSET, model=&r_modelList[MODLIST_OFFSET] ; i<r_numModels ; i++, model++) {
		if (model->touchFrame)
			continue;	// Used r_modelList slot

		model->memTag = i+1;
		break;	// Free spot
	}

	if (i == r_numModels) {
		if (r_numModels+1 >= MAX_REF_MODELS)
			Com_Error (ERR_DROP, "r_numModels >= MAX_REF_MODELS");

		model = &r_modelList[r_numModels++];
		model->memTag = r_numModels;
	}

	return model;
}


/*
==================
R_LoadInlineBSPModel
==================
*/
static refModel_t *R_LoadInlineBSPModel (char *name)
{
	refModel_t	*model;
	uint32		i;

	ri.reg.modelsSeaked++;

	// Inline models are grabbed only from worldmodel
	i = atoi (name+1);
	if (i < 1 || i >= ri.scn.worldModel->bspModel.numSubModels)
		Com_Error (ERR_DROP, "R_LoadInlineBSPModel: Bad inline model number '%d'", i);

	model = &ri.scn.worldModel->bspModel.inlineModels[i];
	model->memTag = ri.scn.worldModel->memTag;
	if (model) {
		model->touchFrame = ri.reg.registerFrame;
		return model;
	}

	return NULL;
}


/*
==================
R_LoadBSPModel
==================
*/
typedef struct bspFormat_s {
	const char	*headerStr;
	int			headerLen;
	int			version;
	qBool		(*loader) (refModel_t *model, byte *buffer);
} bspFormat_t;

static bspFormat_t r_bspFormats[] = {
	{ Q2BSP_HEADER,		4,	Q2BSP_VERSION,					R_LoadQ2BSPModel },	// Quake2 BSP models
	{ Q3BSP_HEADER,		4,	Q3BSP_VERSION,					R_LoadQ3BSPModel },	// Quake3 BSP models

	{ NULL,				0,	0,								NULL }
};

static int r_numBSPFormats = (sizeof (r_bspFormats) / sizeof (r_bspFormats[0])) - 1;

static refModel_t *R_LoadBSPModel (char *name)
{
	char		bareName[MAX_QPATH];
	refModel_t	*model;
	byte		*buffer;
	int			i, fileLen;
	bspFormat_t	*descr;

	// Normalize, strip, lower
	Com_NormalizePath (bareName, sizeof (bareName), name);
	Com_StripExtension (bareName, sizeof (bareName), name);
	Q_strlwr (bareName);

	// Use if already loaded
	model = R_FindModel (bareName);
	if (model) {
		R_TouchModel (model);
		return model;
	}

	// Not found -- allocate a spot
	model = R_GetModelSlot ();

	// Load the file
	fileLen = FS_LoadFile (name, (void **)&buffer, NULL);
	if (!buffer || fileLen <= 0)
		Com_Error (ERR_DROP, "R_LoadBSPModel: %s not found", name);

	// Find the format
	for (i=0, descr=&r_bspFormats[0] ; i<r_numBSPFormats ; i++, descr++) {
		if (strncmp ((const char *)buffer, descr->headerStr, descr->headerLen))
			continue;
		if (((int *)buffer)[1] != descr->version)
			continue;
		break;
	}
	if (i == r_numBSPFormats) {
		FS_FreeFile (buffer);
		Com_Error (ERR_DROP, "R_LoadBSPModel: unknown fileId for %s", model->name);
	}

	// Clear
	model->radius = 0;
	ClearBounds (model->mins, model->maxs);

	// Load
	Q_strncpyz (model->bareName, bareName, sizeof (model->bareName));
	Q_strncpyz (model->name, name, sizeof (model->name));
	if (!descr->loader (model, buffer)) {
		Mem_FreeTag (ri.modelSysPool, model->memTag);
		model->touchFrame = 0;
		FS_FreeFile (buffer);
		Com_Error (ERR_DROP, "R_LoadBSPModel: failed to load map!", model->name);
	}

	// Store values
	model->hashValue = Com_HashGeneric (bareName, MAX_REF_MODEL_HASH);
	model->memSize = Mem_TagSize (ri.modelSysPool, model->memTag);
	model->isBspModel = qTrue;
	model->touchFrame = ri.reg.registerFrame;

	// Link into hash tree
	model->hashNext = r_modelHashTree[model->hashValue];
	r_modelHashTree[model->hashValue] = model;

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
	// Check the name
	if (!mapName)
		Com_Error (ERR_DROP, "R_RegisterMap: NULL name");
	if (!mapName[0])
		Com_Error (ERR_DROP, "R_RegisterMap: empty name");

	// Explicitly free the old map if different...
	if (!ri.scn.worldModel->touchFrame
	|| strcmp (ri.scn.worldModel->name, mapName)
	|| flushmap->intVal) {
		R_FreeModel (ri.scn.worldModel);
	}

	// Load the model
	ri.scn.worldModel = R_LoadBSPModel (mapName);
	ri.scn.worldEntity->model = ri.scn.worldModel;

	// Force markleafs
	ri.scn.oldViewCluster = -1;
	ri.scn.viewCluster = -1;
	ri.scn.visFrameCount--;
	ri.frameCount--;			// Just need to force an update on systems that rely on this variable
}


/*
================
R_EndModelRegistration
================
*/
void R_EndModelRegistration (void)
{
	refModel_t	*model;
	uint32		i;

	for (i=MODLIST_OFFSET, model=&r_modelList[MODLIST_OFFSET] ; i<r_numModels ; i++, model++) {
		if (!model->touchFrame)
			continue;	// Free r_modelList slot
		if (model->touchFrame == ri.reg.registerFrame) {
			R_TouchModel (model);
			continue;	// Used this sequence
		}

		R_FreeModel (model);
		ri.reg.modelsReleased++;
	}
}


/*
================
R_RegisterModel

Load/re-register a model
================
*/
refModel_t *R_RegisterModel (char *name)
{
	refModel_t	*model;
	char		bareName[MAX_QPATH];
	size_t		len;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// If this is a BModel, skip what's below
	if (name[0] == '*') {
		model = R_LoadInlineBSPModel (name);
		return model;
	}

	// Check the length
	len = strlen (name);
	if (len+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_RegisterModel: Model name too long! %s\n", name);
		return NULL;
	}

	// Normalize, strip, lower
	Com_NormalizePath (bareName, sizeof (bareName), name);
	Com_StripExtension (bareName, sizeof (bareName), name);
	Q_strlwr (bareName);

	// Use if already loaded
	model = R_FindModel (bareName);
	if (model) {
		R_TouchModel (model);
		return model;
	}

	// Not found -- allocate a spot and fill base values
	model = R_GetModelSlot ();
	model->radius = 0;
	ClearBounds (model->mins, model->maxs);

	// SP2 model hack
	if (!Q_stricmp (name+len-4, ".sp2")) {
		Q_snprintfz (model->name, sizeof (model->name), "%s.sp2", bareName);
		if (R_LoadSP2Model (model))
			goto loadModel;
	}
	else {
		// MD3
		Q_snprintfz (model->name, sizeof (model->name), "%s.md3", bareName);
		if (R_LoadMD3Model (model))
			goto loadModel;

		// MD2
		model->name[len-3] = 'm'; model->name[len-2] = 'd'; model->name[len-1] = '2';
		if (R_LoadMD2Model (model))
			goto loadModel;
	}

	// Nothing found!
	model->touchFrame = 0;
	return NULL;

	// Found and loaded it, finish the model struct
loadModel:
	Q_strncpyz (model->bareName, bareName, sizeof (model->bareName));
	model->hashValue = Com_HashGeneric (bareName, MAX_REF_MODEL_HASH);
	model->memSize = Mem_TagSize (ri.modelSysPool, model->memTag);
	model->isBspModel = qFalse;
	model->touchFrame = ri.reg.registerFrame;

	// Link into hash tree
	model->hashNext = r_modelHashTree[model->hashValue];
	r_modelHashTree[model->hashValue] = model;

	return model;
}

/*
===============================================================================

	CONSOLE COMMANDS

===============================================================================
*/

/*
================
R_ModelList_f
================
*/
static void R_ModelList_f (void)
{
	refModel_t	*mod;
	size_t		totalBytes;
	size_t		i, total;

	Com_Printf (0, "Loaded models:\n");

	total = 0;
	totalBytes = 0;
	for (i=MODLIST_OFFSET, mod=&r_modelList[MODLIST_OFFSET] ; i<r_numModels ; i++, mod++) {
		if (!mod->touchFrame)
			continue;	// Free r_modelList slot

		switch (mod->type) {
		case MODEL_MD2:		Com_Printf (0, "MD2  ");			break;
		case MODEL_MD3:		Com_Printf (0, "MD3  ");			break;
		case MODEL_Q2BSP:	Com_Printf (0, "Q2BSP");			break;
		case MODEL_Q3BSP:	Com_Printf (0, "Q3BSP");			break;
		case MODEL_SP2:		Com_Printf (0, "SP2  ");			break;
		default:			Com_Printf (PRNT_ERROR, "BAD");		break;
		}

		Com_Printf (0, " %9uB (%6.3fMB) ", mod->memSize, mod->memSize/1048576.0f);
		Com_Printf (0, "%s\n", mod->name);

		totalBytes += mod->memSize;
		total++;
	}

	Com_Printf (0, "%i model(s) loaded, %u bytes (%6.3fMB) total\n", total, totalBytes, totalBytes/1048576.0f);
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

	r_numModels = MODLIST_OFFSET;	// Reserved spot for the default model
	memset (r_modelList, 0, sizeof (refModel_t) * MAX_REF_MODELS);
	memset (r_modelHashTree, 0, sizeof (refModel_t *) * MAX_REF_MODEL_HASH);

	ri.scn.defaultModel = &r_modelList[0];	// Always takes this position
	ri.scn.worldModel = &r_modelList[1];	// Assign a spot so it's not NULL

	Mem_CheckPoolIntegrity (ri.modelSysPool);
}


/*
==================
R_ModelShutdown
==================
*/
void R_ModelShutdown (void)
{
	size_t	size, i;

	Com_Printf (0, "Model system shutdown:\n");

	// Remove commands
	Cmd_RemoveCommand ("modellist", cmd_modelList);

	// Free known loaded models
	for (i=MODLIST_OFFSET ; i<r_numModels ; i++)
		R_FreeModel (&r_modelList[i]);

	// Release pool memory
	size = Mem_FreePool (ri.modelSysPool);
	Com_Printf (0, "...releasing %u bytes...\n", size);
}
