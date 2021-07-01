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
#define USE_FIXPATHNAME
#include "r_local.h"

#define MAX_MODEL_KNOWN		512

static model_t	modInline[MAX_MODEL_KNOWN];	// the inline * models from the current map are kept seperate
static model_t	modKnown[MAX_MODEL_KNOWN];
static int		modNumModels;

static byte		modNoVis[BSP_MAX_VIS];

static uLong	modRegistrationFrame;

cVar_t	*flushmap;

// ============================================================================

/*
=================
_Mod_Alloc
=================
*/
#define Mod_Alloc(model,size) _Mod_Alloc((model),(size),__FILE__,__LINE__)
static void *_Mod_Alloc (model_t *model, size_t size, char *fileName, int fileLine) {
	return _Mem_PoolAlloc (size, MEMPOOL_MODELSYS, model->memTag, fileName, fileLine);
}


/*
=================
_Mod_StrDup
=================
*/
#define Mod_StrDup(model,in) _Mod_StrDup((model),(in),__FILE__,__LINE__)
static void *_Mod_StrDup (model_t *model, char *in, char *fileName, int fileLine) {
	return _Mem_PoolStrDup (in, MEMPOOL_MODELSYS, model->memTag, fileName, fileLine);
}


/*
===============
R_FindTriangleWithEdge
===============
*/
static int R_FindTriangleWithEdge (index_t *indexes, int numTris, index_t start, index_t end, int ignore) {
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

	// detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}


/*
===============
R_BuildTriangleNeighbors
===============
*/
static void R_BuildTriangleNeighbors (int *neighbors, index_t *indexes, int numTris) {
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
Mod_LoadMD2Model
=================
*/
static void Mod_LoadMD2Model (model_t *model, byte *buffer) {
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

	inModel = (dMd2Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD2_MODEL_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: '%s' has wrong version number (%i != %i)",
							model->name, version, MD2_MODEL_VERSION);

	outModel = model->aliasModel = Mod_Alloc (model, sizeof (mAliasModel_t));
	model->type = MODEL_MD2;

	//
	// load the mesh
	//
	outMesh = outModel->meshes = Mod_Alloc (model, sizeof (mAliasMesh_t));
	outModel->numMeshes = 1;
	outMesh->name = Mod_StrDup (model, "default");

	outMesh->numVerts = LittleLong (inModel->numVerts);
	if ((outMesh->numVerts <= 0) || (outMesh->numVerts > MD2_MAX_VERTS))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of vertices '%d'",
							model->name, outMesh->numVerts);

	outMesh->numTris = LittleLong (inModel->numTris);
	if ((outMesh->numTris <= 0) || (outMesh->numTris > MD2_MAX_TRIANGLES))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of triangles '%d'",
							model->name, outMesh->numTris);

	frameSize = LittleLong (inModel->frameSize);
	outModel->numFrames = LittleLong (inModel->numFrames);
	if ((outModel->numFrames <= 0) || (outModel->numFrames > MD2_MAX_FRAMES))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	//
	// load the skins
	//
	skinWidth = LittleLong (inModel->skinWidth);
	skinHeight = LittleLong (inModel->skinHeight);
	if ((skinWidth <= 0) || (skinHeight <= 0))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has invalid skin dimensions '%d x %d'",
							model->name, skinWidth, skinHeight);

	outMesh->numSkins = LittleLong (inModel->numSkins);
	if ((outMesh->numSkins < 0) || (outMesh->numSkins > MD2_MAX_SKINS))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of skins '%d'",
							model->name, outMesh->numSkins);

	isw = 1.0 / (double)skinWidth;
	ish = 1.0 / (double)skinHeight;

	//
	// no tags
	//
	outModel->numTags = 0;
	outModel->tags = NULL;

	//
	// load the indexes
	//
	numIndexes = outMesh->numTris * 3;
	outIndex = outMesh->indexes = Mod_Alloc (model, sizeof (index_t) * numIndexes);

	//
	// load triangle lists
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
	// build list of unique vertexes
	//
	numVerts = 0;
	memset (indRemap, -1, sizeof (int) * MD2_MAX_TRIANGLES * 3);

	for (i=0 ; i<numIndexes ; i++) {
		if (indRemap[i] != -1)
			continue;

		// remap duplicates
		for (j=i+1 ; j<numIndexes ; j++) {
			if ((tempIndex[j] == tempIndex[i])
				&& (inCoord[tempSTIndex[j]].s == inCoord[tempSTIndex[i]].s)
				&& (inCoord[tempSTIndex[j]].t == inCoord[tempSTIndex[i]].t)) {
				indRemap[j] = i;
				outIndex[j] = numVerts;
			}
		}

		// add unique vertex
		indRemap[i] = i;
		outIndex[i] = numVerts++;
	}

	if ((numVerts <= 0) || (numVerts >= ALIAS_MAX_VERTS))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of resampled verts for an alias model '%d' >= ALIAS_MAX_VERTS",
							numVerts, ALIAS_MAX_VERTS);

	Com_Printf (PRNT_DEV, "Mod_LoadMD2Model: '%s' remapped %i verts to %i (%i tris)\n",
							model->name, outMesh->numVerts, numVerts, outMesh->numTris);
	outMesh->numVerts = numVerts;

	//
	// remap remaining indexes
	//
	for (i=0 ; i<numIndexes; i++) {
		if (indRemap[i] != i)
			outIndex[i] = outIndex[indRemap[i]];
	}

	//
	// load base s and t vertices
	//
	outCoord = outMesh->coords = Mod_Alloc (model, sizeof (vec2_t) * numVerts);

	for (j=0 ; j<numIndexes ; j++) {
		outCoord[outIndex[j]][0] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].s) + 0.5) * isw);
		outCoord[outIndex[j]][1] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].t) + 0.5) * ish);
	}

	//
	// load the frames
	//
	outFrame = outModel->frames = Mod_Alloc (model, sizeof (mAliasFrame_t) * outModel->numFrames);
	outVertex = outMesh->vertexes = Mod_Alloc (model, sizeof (mAliasVertex_t) * outModel->numFrames * numVerts);

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
		// load vertices and normals
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
	// build a list of neighbors
	//
	outMesh->neighbors = Mod_Alloc (model, sizeof(int) * outMesh->numTris * 3);
	R_BuildTriangleNeighbors (outMesh->neighbors, outMesh->indexes, outMesh->numTris);

	//
	// register all skins
	//
	outSkins = outMesh->skins = Mod_Alloc (model, sizeof (mAliasSkin_t) * outMesh->numSkins);

	for (i=0 ; i<outMesh->numSkins ; i++, outSkins++) {
		if (LittleLong (inModel->ofsSkins) == -1)
			continue;

		temp = (char *)inModel + LittleLong (inModel->ofsSkins) + i*MD2_MAX_SKINNAME;
		if (!temp || !temp[0])
			continue;

		outSkins->name = Mod_StrDup (model, temp);
		outSkins->image = Img_RegisterImage (outSkins->name, 0);
		outSkins->shader = RS_RegisterShader (outSkins->name);

		if (!outSkins->image) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Mod_LoadMD2Model: '%s' could not load skin '%s'\n",
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
Mod_StripLODSuffix
=================
*/
void Mod_StripLODSuffix (char *name) {
	int		len;
	int		lodNum;

	len = Q_strlen (name);
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
Mod_LoadMD3Model
=================
*/
static void Mod_LoadMD3Model (model_t *model, byte *buffer) {
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

	inModel = (dMd3Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD3_MODEL_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has wrong version number (%i != %i)",
							model->name, version, MD3_MODEL_VERSION);

	model->aliasModel = outModel = Mod_Alloc (model, sizeof (mAliasModel_t));
	model->type = MODEL_MD3;

	//
	// byte swap the header fields and sanity check
	//
	outModel->numFrames = LittleLong (inModel->numFrames);
	if ((outModel->numFrames <= 0) || (outModel->numFrames > MD3_MAX_FRAMES))
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	outModel->numTags = LittleLong (inModel->numTags);
	if ((outModel->numTags < 0) || (outModel->numTags > MD3_MAX_TAGS))
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has an invalid amount of tags '%d'",
							model->name, outModel->numTags);

	outModel->numMeshes = LittleLong (inModel->numMeshes);
	if ((outModel->numMeshes < 0) || (outModel->numMeshes > MD3_MAX_MESHES))
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has an invalid amount of meshes '%d'",
							model->name, outModel->numMeshes);

	if (!outModel->numMeshes && !outModel->numTags)
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has no meshes and no tags!",
							model->name);

	//
	// load the frames
	//
	inFrame = (dMd3Frame_t *)((byte *)inModel + LittleLong (inModel->ofsFrames));
	outFrame = outModel->frames = Mod_Alloc (model, sizeof (mAliasFrame_t) * outModel->numFrames);

	for (i=0 ; i<outModel->numFrames ; i++, inFrame++, outFrame++) {
		outFrame->scale[0] = MD3_XYZ_SCALE;
		outFrame->scale[1] = MD3_XYZ_SCALE;
		outFrame->scale[2] = MD3_XYZ_SCALE;

		outFrame->translate[0] = LittleFloat (inFrame->translate[0]);
		outFrame->translate[1] = LittleFloat (inFrame->translate[1]);
		outFrame->translate[2] = LittleFloat (inFrame->translate[2]);

		// never trust the modeler utility and recalculate bbox and radius
		ClearBounds (outFrame->mins, outFrame->maxs);
	}

	//
	// load the tags
	//
	inTag = (dMd3Tag_t *)((byte *)inModel + LittleLong (inModel->ofsTags));
	outTag = outModel->tags = Mod_Alloc (model, sizeof (mAliasTag_t) * outModel->numFrames * outModel->numTags);

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

			outTag->name = Mod_StrDup (model, inTag->tagName);
		}
	}

	//
	// load the meshes
	//
	inMesh = (dMd3Mesh_t *)((byte *)inModel + LittleLong (inModel->ofsMeshes));
	outMesh = outModel->meshes = Mod_Alloc (model, sizeof (mAliasMesh_t) * outModel->numMeshes);

	for (i=0 ; i<outModel->numMeshes ; i++, outMesh++) {
		outMesh->name = Mod_StrDup (model, inMesh->meshName);
		if (strncmp ((const char *)inMesh->ident, MD3_HEADERSTR, 4))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has wrong id (%i != %i)",
								inMesh->meshName, model->name, LittleLong ((int)inMesh->ident), MD3_HEADER);

		Mod_StripLODSuffix (outMesh->name);

		outMesh->numSkins = LittleLong (inMesh->numSkins);
		if ((outMesh->numSkins <= 0) || (outMesh->numSkins > MD3_MAX_SHADERS))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of skins '%d'",
								outMesh->name, model->name, outMesh->numSkins);

		outMesh->numTris = LittleLong (inMesh->numTris);
		if ((outMesh->numTris <= 0) || (outMesh->numTris > MD3_MAX_TRIANGLES))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of triangles '%d'",
								outMesh->name, model->name, outMesh->numTris);

		outMesh->numVerts = LittleLong (inMesh->numVerts);
		if ((outMesh->numVerts <= 0) || (outMesh->numVerts > MD3_MAX_VERTS))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of vertices '%d'",
								outMesh->name, model->name, outMesh->numVerts);

		if ((outMesh->numVerts <= 0) || (outMesh->numVerts >= ALIAS_MAX_VERTS))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount verts for an alias model '%d' >= ALIAS_MAX_VERTS",
								outMesh->name, outMesh->numVerts, ALIAS_MAX_VERTS);

		//
		// load the skins
		//
		inSkin = (dMd3Skin_t *)((byte *)inMesh + LittleLong (inMesh->ofsSkins));
		outSkin = outMesh->skins = Mod_Alloc (model, sizeof (mAliasSkin_t) * outMesh->numSkins);

		for (j=0 ; j<outMesh->numSkins ; j++, inSkin++, outSkin++) {
			if (!inSkin->name || !inSkin->name[0])
				continue;

			outSkin->name = Mod_StrDup (model, inSkin->name);
			outSkin->image = Img_RegisterImage (outSkin->name, 0);
			outSkin->shader = RS_RegisterShader (outSkin->name);

			if (!outSkin->image)
				Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Mod_LoadMD3Model: '%s' could not load skin '%s' on mesh '%s'\n",
								model->name, outSkin->name, outMesh->name);
		}

		//
		// load the indexes
		//
		inIndex = (index_t *)((byte *)inMesh + LittleLong (inMesh->ofsIndexes));
		outIndex = outMesh->indexes = Mod_Alloc (model, sizeof (index_t) * outMesh->numTris * 3);

		for (j=0 ; j<outMesh->numTris ; j++, inIndex += 3, outIndex += 3) {
			outIndex[0] = (index_t)LittleLong (inIndex[0]);
			outIndex[1] = (index_t)LittleLong (inIndex[1]);
			outIndex[2] = (index_t)LittleLong (inIndex[2]);
		}

		//
		// load the texture coordinates
		//
		inCoord = (dMd3Coord_t *)((byte *)inMesh + LittleLong (inMesh->ofsTCs));
		outCoord = outMesh->coords = Mod_Alloc (model, sizeof (vec2_t) * outMesh->numVerts);

		for (j=0 ; j<outMesh->numVerts ; j++, inCoord++) {
			outCoord[j][0] = LittleFloat (inCoord->st[0]);
			outCoord[j][1] = LittleFloat (inCoord->st[1]);
		}

		//
		// load the vertexes and normals
		//
		inVert = (dMd3Vertex_t *)((byte *)inMesh + LittleLong (inMesh->ofsVerts));
		outVert = outMesh->vertexes = Mod_Alloc (model, sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts);
		outFrame = outModel->frames;

		for (l=0 ; l<outModel->numFrames ; l++, outFrame++) {
			vec3_t	v;

			for (j=0 ; j<outMesh->numVerts ; j++, inVert++, outVert++) {
				outVert->point[0] = LittleShort (inVert->point[0]);
				outVert->point[1] = LittleShort (inVert->point[1]);
				outVert->point[2] = LittleShort (inVert->point[2]);

				VectorCopy (outVert->point, v);
				AddPointToBounds (v, outFrame->mins, outFrame->maxs);

				outVert->latLong[0] = inVert->norm[0];
				outVert->latLong[1] = inVert->norm[1];
			}
		}

		//
		// build a list of neighbors
		//
		outMesh->neighbors = Mod_Alloc (model, sizeof(int) * outMesh->numTris * 3);
		R_BuildTriangleNeighbors (outMesh->neighbors, outMesh->indexes, outMesh->numTris);

		// end of loop
		inMesh = (dMd3Mesh_t *)((byte *)inMesh + LittleLong (inMesh->meshSize));
	}

	//
	// calculate model bounds
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

	MD5 LOADING

===============================================================================
*/

#define MD5_HEADERSTR	"MD5Version"
#define MD5_MODEL_VERSION	10

#define MD5_DELIMINATORS	"\t\r "

typedef struct mSkeletalBones_s {
	char				*name;
	int					parentIndex;	// -1 = no parent

	vec3_t				origin;
	quat_t				orientation;
} mSkeletalBones_t;

typedef struct mSkeletalSkin_s {
	char				*name;
	image_t				*image;
	shader_t			*shader;
} mSkeletalSkin_t;

typedef struct mSkeletalMesh_s {
	char				*name;
	mSkeletalSkin_t		*skins;

	vec2_t				*coords;

	int					startWeight;
	int					numWeights;
} mSkeletalMesh_t;

typedef struct mSkeletalModel_s {
	int					numBones;
	mSkeletalBones_t	*bones;

	int					numMeshes;
	mSkeletalMesh_t		*meshes;
} mSkeletalModel_t;

/*
=================
Mod_LoadMD5Model
=================
*/
static void Mod_LoadMD5Model (model_t *model, byte *buffer) {
/*	mAliasModel_t	*outModel;
	mAliasMesh_t	*outMesh;
	index_t			*outIndex;
	vec2_t			*outCoord;
	mAliasFrame_t	*outFrame;
	mAliasVertex_t	*outVertex;
	mAliasSkin_t	*outSkins;
*/


	mSkeletalModel_t	*outModel;
	int					numJoints;//, numBones;
	int					version;
	char				*token;

	int		jointCount;
	int		boneCount;

	return;

	outModel = model->skeletalModel = Mod_Alloc (model, sizeof (mSkeletalModel_t));
	model->type = MODEL_MD5;

	//
	// version
	//
	token = strtok (buffer, MD5_DELIMINATORS);
	if (!strcmp (token, "MD5Version")) {
		token = strtok (NULL, MD5_DELIMINATORS);
		version = atoi (token);
		if (version != MD5_MODEL_VERSION)
			Com_Error (ERR_DROP, "Mod_LoadMD5Model: '%s' has wrong version number (%i != %i)",
								model->name, version, MD5_MODEL_VERSION);
		else
			Com_Printf (0, S_COLOR_CYAN "orly MD5Version: %i\n", version);
	}
	else
		goto badMd5Model;

	//
	// skip commandline, only takes up one line
	//
	token = strtok (NULL, MD5_DELIMINATORS"\n");
	if (!strcmp (token, "commandline")) {
		token = strtok (NULL, "\n");
	}
	else
		goto badMd5Model;

	//
	// num joints
	//
	token = strtok (NULL, MD5_DELIMINATORS"\n");
	if (!strcmp (token, "numJoints")) {
		token = strtok (NULL, MD5_DELIMINATORS"\n");
		numJoints = atoi (token);

		Com_Printf (0, S_COLOR_CYAN "orly numJoints: %i\n", numJoints);
	}
	else
		goto badMd5Model;

	//
	// num meshes
	//
	token = strtok (NULL, MD5_DELIMINATORS"\n");
	if (!strcmp (token, "numMeshes")) {
		token = strtok (NULL, MD5_DELIMINATORS"\n");
		outModel->numMeshes = atoi (token);

		Com_Printf (0, S_COLOR_CYAN "orly numMeshes: %i\n", outModel->numMeshes);
	}
	else
		goto badMd5Model;

	//
	// bones
	//
	token = strtok (NULL, MD5_DELIMINATORS"\n");
	if (!strcmp (token, "joints")) {
		// parse the information
		token = strtok (NULL, MD5_DELIMINATORS"\n");
		if (!strcmp (token, "{"))
			Com_Printf (0, "joints %s\n", token);
		else
			goto badMd5Model;

		// parse the line out
		// "name"	-1 ( org0 org1 org2 ) ( rot0 rot1 rot2 )	// parent
		jointCount = 0;
		boneCount = 0;
		token = strtok (NULL, MD5_DELIMINATORS"\n");
		while (token != NULL)  {
			// the end?
			if (!strcmp (token, "}"))
				break;

			// "name"
			if (token)	Com_Printf (0, "name: %s\n", token);

			// parentIndex
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "parentIndex: %s\n", token);

			// ( org0 org1 org2 )
			Com_Printf (0, "org");
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, " %s, ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s, ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s", token);
			Com_Printf (0, "\n");

			// ( rot0 rot1 rot2 )
			Com_Printf (0, "rot");
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, " %s, ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s, ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s ", token);
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token)	Com_Printf (0, "%s", token);
			Com_Printf (0, "\n");

			// skip the '//'
			token = strtok (NULL, MD5_DELIMINATORS);

			// parent
			token = strtok (NULL, MD5_DELIMINATORS);
			if (token && (token[0] != '\n')) // true when there is no parent
				Com_Printf (0, "parent: '%s'\n", token);

			token = strtok (NULL, MD5_DELIMINATORS"\n");
			jointCount++;
		}

		Com_Printf (0, "} \n");

		// check values
		if (jointCount != numJoints)
			Com_Printf (0, "Mod_LoadMD5Model: '%s' has a joint count mismatch (%i != %i)\n",
								model->name, jointCount, numJoints);
	}
	else
		goto badMd5Model;

	//
	// meshes
	//
	token = strtok (NULL, MD5_DELIMINATORS"\n");
	while (token != NULL) {
		if (!strcmp (token, "mesh")) {
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL) {
				if (!strcmp (token, "{"))
					Com_Printf (0, "mesh %s\n", token);

				// parse the line out
				// '// meshes: what, what, what'
				token = strtok (NULL, MD5_DELIMINATORS);
				//Com_Printf (0, "%s", token);
				//while (token != NULL)  {
				//}
				// goto next line
				token = strtok (NULL, "\n");

				token = strtok (NULL, MD5_DELIMINATORS"\n");

				if (!strcmp (token, "}")) {
					Com_Printf (0, "%s\n", token);
					break;
				}

				token = strtok (NULL, MD5_DELIMINATORS"\n");
			}
		}

		token = strtok (NULL, MD5_DELIMINATORS"\n");
	}



#if 0
			// starter '{'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			if (!strcmp (token, "{"))
				Com_Printf (0, "mesh %s\n", token);
			else
				goto badMd5Model;

			// parse the line out
			// '// meshes: what, what, what'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL)  {
			}

			// parse the line out
			// 'shader: "path/to/texture"'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL)  {
			}

			// numVerts
			token = strtok (NULL, MD5_DELIMINATORS"\n");

			// parse the line out
			// 'vert id ( tc0 tc1 ) startWeight numWeights'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL)  {
			}

			// numTris
			token = strtok (NULL, MD5_DELIMINATORS"\n");

			// parse the line out
			// 'tri id x y z'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL)  {
			}

			// numWeights
			token = strtok (NULL, MD5_DELIMINATORS"\n");

			// parse the line out
			// 'weight index jointIndex weight ( x y z )'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			while (token != NULL)  {
			}

			// final '}'
			token = strtok (NULL, MD5_DELIMINATORS"\n");
			if (!strcmp (token, "{"))
				Com_Printf (0, "mesh %s\n", token);
		//	else
		//		goto badMd5Model;
#endif
	return;

badMd5Model:
	Com_Error (ERR_DROP, "Mod_LoadMD5Model: '%s' unable to parse model!", model->name);
}

/*
===============================================================================

	SP2 LOADING

===============================================================================
*/

/*
=================
Mod_LoadSP2Model
=================
*/
static void Mod_LoadSP2Model (model_t *model, byte *buffer) {
	dSpriteHeader_t	*inModel;
	dSpriteFrame_t	*inFrames;
	mSpriteModel_t	*outModel;
	mSpriteFrame_t	*outFrames;
	int				i, version;

	inModel = (dSpriteHeader_t *)buffer;

	//
	// sanity checks
	//
	version = LittleLong (inModel->version);
	if (version != SP2_VERSION)
		Com_Error (ERR_DROP, "%s has wrong version number (%i should be %i)", model->name, version, SP2_VERSION);

	model->spriteModel = outModel = Mod_Alloc (model, sizeof (mSpriteModel_t));
	model->type = MODEL_SP2;

	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames > SP2_MAX_FRAMES)
		Com_Error (ERR_DROP, "Mod_LoadSP2Model: model '%s' has too many frames (%i > %i)", model->name, outModel->numFrames, SP2_MAX_FRAMES);

	//
	// byte swap everything
	//
	inFrames = inModel->frames;
	outModel->frames = outFrames = Mod_Alloc (model, sizeof (mSpriteFrame_t) * outModel->numFrames);

	for (i=0 ; i<outModel->numFrames ; i++, inFrames++, outFrames++) {
		outFrames->width	= LittleLong (inFrames->width);
		outFrames->height	= LittleLong (inFrames->height);
		outFrames->originX	= LittleLong (inFrames->originX);
		outFrames->originY	= LittleLong (inFrames->originY);

		//
		// for culling
		//
		outFrames->radius	= sqrt ((outFrames->width*outFrames->width) + (outFrames->height*outFrames->height));
		model->radius		= max (model->radius, outFrames->radius);

		//
		// register all skins
		//
		outFrames->name = Mod_StrDup (model, inFrames->name);
		outFrames->skin = Img_RegisterImage (outFrames->name, 0);
		outFrames->shader = RS_RegisterShader (outFrames->name);

		if (!outFrames->skin)
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Mod_LoadSP2Model: '%s' could not load skin '%s'\n",
							model->name, outFrames->name);
	}
}

/*
===============================================================================

	BRUSH MODELS

===============================================================================
*/

static void BoundPoly (int numVerts, float *verts, vec3_t mins, vec3_t maxs) {
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
Mod_SubdivideSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdividePolygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize) {
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

	if (numVerts > 60)
		Com_Error (ERR_DROP, "SubdividePolygon: numVerts = %i", numVerts);

	BoundPoly (numVerts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5;
		m = subdivideSize * floor (m/subdivideSize + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numVerts ; j++, v+=3)
			dist[j] = *v - m;

		// wrap cases
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
				// clip point
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

	// add a point in the center to help keep warp valid
	poly = Mod_Alloc (model, sizeof (mBspPoly_t));

	poly->numVerts = numVerts+2;
	poly->numIndexes = numVerts*3;

	poly->vertices = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->normals = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->coords = Mod_Alloc (model, poly->numVerts * sizeof (vec2_t));

	memset (&posTotal, 0, sizeof (posTotal));
	memset (&normalTotal, 0, sizeof (normalTotal));
	memset (&coordTotal, 0, sizeof (coordTotal));

	for (i=0 ; i<numVerts ; i++) {
		// position
		VectorCopy (verts, poly->vertices[i+1]);

		// normal
		VectorCopy (surf->plane->normal, poly->normals[i+1]);

		// texture coords
		poly->coords[i+1][0] = DotProduct (verts, surf->texInfo->vecs[0]);
		poly->coords[i+1][1] = DotProduct (verts, surf->texInfo->vecs[1]);

		// for the center point
		VectorAdd (posTotal, verts, posTotal);
		VectorAdd (normalTotal, surf->plane->normal, normalTotal);
		coordTotal[0] += poly->coords[i+1][0];
		coordTotal[1] += poly->coords[i+1][1];

		verts += 3;
	}

	// center
	oneDivVerts = (1.0/(float)numVerts);
	VectorScale (posTotal, oneDivVerts, poly->vertices[0]);
	VectorScale (normalTotal, oneDivVerts, poly->normals[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->coords[0]);

	// copy first vertex to last
	memcpy (poly->vertices[i+1], poly->vertices[1], sizeof (poly->vertices[i+1]));
	memcpy (poly->normals[i+1], poly->normals[1], sizeof (poly->normals[i+1]));
	memcpy (poly->coords[i+1], poly->coords[1], sizeof (poly->coords[i+1]));

	// link it in
	poly->next = surf->polys;
	surf->polys = poly;
}

static void Mod_SubdivideSurface (model_t *model, mBspSurface_t *surf) {
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	index_t		index;
	float		*vec;

	// convert edges back to a normal polygon
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

	SubdividePolygon (model, surf, numVerts, verts[0], 64);
}


/*
================
Mod_SubdivideLightmapSurface

Breaks a polygon up along axial ^2 unit boundaries
================
*/
static void SubdivideLightmappedPolygon (model_t *model, mBspSurface_t *surf, int numVerts, float *verts, float subdivideSize) {
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

	if (numVerts > 60)
		Sys_Error ("SubdivideLightmappedPolygon: numVerts = %i", numVerts);

	BoundPoly (numVerts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5;
		m = subdivideSize * floor (m/subdivideSize + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numVerts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
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
				// clip point
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

	// add a point in the center to help keep warp valid
	poly = Mod_Alloc (model, sizeof (mBspPoly_t));

	poly->numVerts = numVerts+2;
	poly->numIndexes = numVerts*3;

	poly->vertices = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->normals = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->coords = Mod_Alloc (model, poly->numVerts * sizeof (vec2_t));
	poly->lmCoords = Mod_Alloc (model, poly->numVerts * sizeof (vec2_t));

	memset (&posTotal, 0, sizeof (posTotal));
	memset (&normalTotal, 0, sizeof (normalTotal));
	memset (&coordTotal, 0, sizeof (coordTotal));
	memset (&lmCoordTotal, 0, sizeof (lmCoordTotal));

	for (i=0 ; i<numVerts ; i++) {
		VectorCopy (verts, poly->vertices[i+1]);

		// normals
		VectorCopy (surf->plane->normal, poly->vertices[i+1]);

		// texture coordinates
		poly->coords[i+1][0] = (DotProduct (verts, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / surf->texInfo->texture->width;
		poly->coords[i+1][1] = (DotProduct (verts, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / surf->texInfo->texture->height;

		// lightmap texture coordinates
		s = DotProduct (verts, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3] - surf->textureMins[0];
		poly->lmCoords[i+1][0] = (s + 8 + (surf->lmCoords[0] * 16)) / (LIGHTMAP_WIDTH * 16);

		t = DotProduct (verts, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3] - surf->textureMins[1];
		poly->lmCoords[i+1][1] = (t + 8 + (surf->lmCoords[1] * 16)) / (LIGHTMAP_HEIGHT * 16);

		// for the center point
		coordTotal[0] += poly->coords[i+1][0];
		coordTotal[1] += poly->coords[i+1][1];
		VectorAdd (posTotal, verts, posTotal);

		lmCoordTotal[0] += poly->lmCoords[i+1][0];
		lmCoordTotal[1] += poly->lmCoords[i+1][1];

		verts += 3;
	}

	// center point
	oneDivVerts = (1.0/(float)numVerts);
	VectorScale (posTotal, oneDivVerts, poly->vertices[0]);
	VectorScale (normalTotal, oneDivVerts, poly->normals[0]);
	Vector2Scale (coordTotal, oneDivVerts, poly->coords[0]);
	Vector2Scale (lmCoordTotal, oneDivVerts, poly->lmCoords[0]);

	// copy first vertex to last
	memcpy (poly->vertices[i+1], poly->vertices[1], sizeof (poly->vertices[i+1]));
	memcpy (poly->normals[i+1], poly->normals[1], sizeof (poly->normals[i+1]));
	memcpy (poly->coords[i+1], poly->coords[1], sizeof (poly->coords[i+1]));
	memcpy (poly->lmCoords[i+1], poly->lmCoords[1], sizeof (poly->lmCoords[i+1]));

	// link it in
	poly->next = surf->polys;
	surf->polys = poly;
}

static void Mod_SubdivideLightmapSurface (model_t *model, mBspSurface_t *surf, float subdivideSize) {
	vec3_t		verts[64];
	int			numVerts;
	int			i;
	int			index;
	float		*vec;

	// convert edges back to a normal polygon
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
Mod_BuildPolygonFromSurface
================
*/
static void Mod_BuildPolygonFromSurface (model_t *model, mBspSurface_t *surf) {
	int			i, numVerts;
	mBspEdge_t	*pedges, *r_pedge;
	float		*vec, s, t;
	mBspPoly_t	*poly;
	vec3_t		posTotal;
	index_t		index;

	pedges = model->edges;
	numVerts = surf->numEdges;

	// allocate
	poly = Mod_Alloc (model, sizeof (mBspPoly_t));

	poly->flags = surf->flags;
	poly->numVerts = numVerts;
	poly->numIndexes = (numVerts-2)*3;

	poly->vertices = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->normals = Mod_Alloc (model, poly->numVerts * sizeof (vec3_t));
	poly->coords = Mod_Alloc (model, poly->numVerts * sizeof (vec2_t));
	poly->lmCoords = Mod_Alloc (model, poly->numVerts * sizeof (vec2_t));

	memset (&posTotal, 0, sizeof (posTotal));

	for (i=0 ; i<numVerts ; i++) {
		// position
		index = model->surfEdges[surf->firstEdge + i];

		if (index > 0) {
			r_pedge = &pedges[index];
			vec = model->vertexes[r_pedge->v[0]].position;
		}
		else {
			r_pedge = &pedges[-index];
			vec = model->vertexes[r_pedge->v[1]].position;
		}

		//VectorCopy (vec, poly->vertices[i]);
		memcpy (poly->vertices[i], vec, sizeof (vec3_t));

		// normal
		VectorCopy (surf->plane->normal, poly->normals[i]);

		// texture coordinates
		poly->coords[i][0] = (DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / surf->texInfo->texture->width;
		poly->coords[i][1] = (DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / surf->texInfo->texture->height;

		// lightmap texture coordinates
		s = DotProduct (vec, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3] - surf->textureMins[0];
		poly->lmCoords[i][0] = (s + 8 + (surf->lmCoords[0] * 16)) / (LIGHTMAP_WIDTH * 16);

		t = DotProduct (vec, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3] - surf->textureMins[1];
		poly->lmCoords[i][1] = (t + 8 + (surf->lmCoords[1] * 16)) / (LIGHTMAP_HEIGHT * 16);

		// for averaging
		VectorAdd (posTotal, vec, posTotal);
	}

	// center point
	VectorScale (posTotal, 1.0f/(float)numVerts, posTotal);

	surf->c_s = (DotProduct (posTotal, surf->texInfo->vecs[0]) + surf->texInfo->vecs[0][3]) / surf->texInfo->texture->width;
	surf->c_t = (DotProduct (posTotal, surf->texInfo->vecs[1]) + surf->texInfo->vecs[1][3]) / surf->texInfo->texture->height;

	// link it in
	poly->next = surf->polys;
	surf->polys = poly;
}


/*
================
Mod_CalcSurfaceBounds

Fills in s->mins and s->maxs
================
*/
static void Mod_CalcSurfaceBounds (model_t *model, mBspSurface_t *surf) {
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
Mod_CalcSurfaceExtents

Fills in s->textureMins[] and s->extents[]
================
*/
static void Mod_CalcSurfaceExtents (model_t *model, mBspSurface_t *surf) {
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
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		surf->textureMins[i] = bmins[i] * 16;
		surf->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model) {
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
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model) {
	if ((cluster == -1) || !model->vis)
		return modNoVis;

	return Mod_DecompressVis ((byte *)model->vis + model->vis->bitOfs[cluster][BSP_VIS_PVS], model);
}


/*
===============
Mod_PointInLeaf
===============
*/
mBspLeaf_t *Mod_PointInLeaf (vec3_t point, model_t *model) {
	cBspPlane_t	*plane;
	mBspNode_t	*node;
	float		d;
	
	if (!model || !model->nodes)
		Com_Error (ERR_DROP, "Mod_PointInLeaf: bad model");

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
Mod_SetParentNode
=================
*/
static void Mod_SetParentNode (mBspNode_t *node, mBspNode_t *parent) {
	node->parent = parent;
	if (node->contents != -1)
		return;

	Mod_SetParentNode (node->children[0], node);
	Mod_SetParentNode (node->children[1], node);
}

/*
===============================================================================

	BRUSH MODEL LOADING

===============================================================================
*/

static int	map_NumEntityChars;
static char	map_EntityString[BSP_MAX_ENTSTRING];

/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspVertex_t	*in;
	mBspVertex_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numVertexes = lump->fileLen / sizeof (*in);
	model->vertexes = out = Mod_Alloc (model, sizeof (*out) * model->numVertexes);

	//
	// byte swap
	//
	for (i=0 ; i<model->numVertexes ; i++, in++, out++) {
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}


/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspEdge_t	*in;
	mBspEdge_t	*out;
	int			i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numEdges = lump->fileLen / sizeof (*in);
	model->edges = out = Mod_Alloc (model, sizeof (*out) * (model->numEdges + 1));

	//
	// byte swap
	//
	for (i=0 ; i<model->numEdges ; i++, in++, out++) {
		out->v[0] = (uShort) LittleShort (in->v[0]);
		out->v[1] = (uShort) LittleShort (in->v[1]);
	}
}


/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (model_t *model, byte *byteBase, dBspLump_t *lump) {	
	int		*in;
	int		*out;
	int		i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numSurfEdges = lump->fileLen / sizeof (*in);
	if ((model->numSurfEdges < 1) || (model->numSurfEdges >= BSP_MAX_SURFEDGES))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: invalid surfEdges count in %s: %i (min: 1; max: %d)", model->name, model->numSurfEdges, BSP_MAX_SURFEDGES);

	model->surfEdges = out = Mod_Alloc (model, sizeof (*out) * model->numSurfEdges);

	//
	// byte swap
	//
	for (i=0 ; i<model->numSurfEdges ; i++) {
		out[i] = LittleLong (in[i]);
	}
}


/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (model_t *model, byte *byteBase, dBspLump_t *lump) {
	if (!lump->fileLen) {
		model->lightData = NULL;
		return;
	}

	model->lightData = Mod_Alloc (model, lump->fileLen);	
	memcpy (model->lightData, byteBase + lump->fileOfs, lump->fileLen);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (model_t *model, byte *byteBase, dBspLump_t *lump) {
	int				i, j;
	cBspPlane_t		*out;
	dBspPlane_t		*in;
	int				bits;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numPlanes = lump->fileLen / sizeof (*in);
	model->planes = out = Mod_Alloc (model, sizeof (*out) * model->numPlanes * 2);

	//
	// byte swap
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
Mod_LoadTexInfo
=================
*/
void Mod_LoadTexInfo (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspTexInfo_t	*in;
	mBspTexInfo_t	*out, *step;
	int				i, j;
	char			name[MAX_QPATH];
	int				next;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numTexInfo = lump->fileLen / sizeof (*in);
	model->texInfo = out = Mod_Alloc (model, sizeof (*out) * model->numTexInfo);

	//
	// byte swap
	//
	for (i=0 ; i<model->numTexInfo ; i++, in++, out++) {
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);

		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nextTexInfo);
		if (next > 0)
			out->next = model->texInfo + next;
		else
			out->next = NULL;

		//
		// register textures and shaders
		//

		if (out->flags & SURF_SKY) {
			// sky surfaces are never rendered, but clipped out for the skybox to show
			out->texture = r_WhiteTexture;
			out->shader = NULL;
		}
		else {
			Q_snprintfz (name, sizeof (name), "textures/%s.wal", in->texture);
			out->texture = Img_RegisterImage (name, IT_TEXTURE);
			out->shader = RS_RegisterShader (name);
			if (!out->texture) {
				Com_Printf (0, S_COLOR_YELLOW "Couldn't load %s\n", name);
				out->texture = r_NoTexture;
				out->shader = NULL;
			}
		}
	}

	//
	// count animation frames
	//
	for (i=0 ; i<model->numTexInfo ; i++) {
		out = &model->texInfo[i];
		out->numFrames = 1;
		for (step=out->next ; step && (step != out) ; step=step->next) {
			out->numFrames++;
		}
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspSurface_t	*in;
	mBspSurface_t	*out;
	int				i, surfnum;
	int				planeNum, side;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numSurfaces = lump->fileLen / sizeof (*in);
	model->surfaces = out= Mod_Alloc (model, sizeof (*out) * model->numSurfaces);

	LM_BeginBuildingLightmaps ();

	//
	// byte swap
	//
	for (surfnum=0 ; surfnum<model->numSurfaces ; surfnum++, in++, out++) {
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
		if ((i < 0) || (i >= model->numTexInfo))
			Com_Error (ERR_DROP, "MOD_LoadBmodel: bad texInfo number");
		out->texInfo = model->texInfo + i;

		Mod_CalcSurfaceBounds (model, out);
		Mod_CalcSurfaceExtents (model, out);

		//
		// lighting info
		//
		out->lmWidth = (out->extents[0]>>4) + 1;
		out->lmHeight = (out->extents[1]>>4) + 1;

		i = LittleLong (in->lightOfs);
		out->lmSamples = (i == -1) ? NULL : model->lightData + i;

		for (out->numStyles=0 ; out->numStyles<BSP_MAX_LIGHTMAPS && in->styles[out->numStyles] != 255 ; out->numStyles++) {
			out->styles[out->numStyles] = in->styles[out->numStyles];
		}

		//
		// set the drawing flags
		//
		if (out->texInfo->flags & SURF_WARP) {
			out->flags |= SURF_DRAWTURB;

			out->extents[0] = out->extents[1] = 16384;
			out->textureMins[0] = out->textureMins[1] = -8192;

			// cut up polygon for warps
			Mod_SubdivideSurface (model, out);
		}
		else {
			//
			// create lightmaps and polygons
			//
			if (!(out->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66))) {
				LM_CreateSurfaceLightmap (out);
			}

			if (out->texInfo->shader && out->texInfo->shader->subDivide) {
				Mod_SubdivideLightmapSurface (model, out, out->texInfo->shader->subDivide);
			}
			else {
				Mod_BuildPolygonFromSurface (model, out);
			}
		}
	}

	LM_EndBuildingLightmaps ();
}


/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (model_t *model, byte *byteBase, dBspLump_t *lump) {
	mBspSurface_t	**out;
	int				i, j;
	short			*in;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numMarkSurfaces = lump->fileLen / sizeof (*in);
	model->markSurfaces = out = Mod_Alloc (model, sizeof (*out) * model->numMarkSurfaces);

	//
	// byte swap
	//
	for (i=0 ; i<model->numMarkSurfaces ; i++) {
		j = LittleShort (in[i]);
		if ((j < 0) || (j >= model->numSurfaces))
			Com_Error (ERR_DROP, "Mod_ParseMarksurfaces: bad surface number");
		out[i] = model->surfaces + j;
	}
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (model_t *model, byte *byteBase, dBspLump_t *lump) {
	int		i;

	if (!lump->fileLen) {
		model->vis = NULL;
		return;
	}

	model->vis = Mod_Alloc (model, lump->fileLen);	
	memcpy (model->vis, byteBase + lump->fileOfs, lump->fileLen);

	model->vis->numClusters = LittleLong (model->vis->numClusters);

	//
	// byte swap
	//
	for (i=0 ; i<model->vis->numClusters ; i++) {
		model->vis->bitOfs[i][0] = LittleLong (model->vis->bitOfs[i][0]);
		model->vis->bitOfs[i][1] = LittleLong (model->vis->bitOfs[i][1]);
	}
}


/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspLeaf_t	*in;
	mBspLeaf_t	*out;
	int			i, j;
	mBspPoly_t	*poly;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numLeafs = lump->fileLen / sizeof (*in);
	model->leafs = out = Mod_Alloc (model, sizeof (*out) * model->numLeafs);

	//
	// byte swap
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

		//
		// mark poly flags
		//
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			for (j=0 ; j<out->numMarkSurfaces ; j++) {
				out->firstMarkSurface[j]->flags |= SURF_UNDERWATER;
				for (poly=out->firstMarkSurface[j]->polys ; poly ; poly=poly->next) {
					if (!(poly->flags & SURF_UNDERWATER))	poly->flags |= SURF_UNDERWATER;
					if (out->contents & CONTENTS_LAVA)		poly->flags |= SURF_LAVA;
					if (out->contents & CONTENTS_SLIME)		poly->flags |= SURF_SLIME;
					if (out->contents & CONTENTS_WATER)		poly->flags |= SURF_WATER;
				}
			}
		}
	}	
}


/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (model_t *model, byte *byteBase, dBspLump_t *lump) {
	int			i, p;
	dBspNode_t	*in;
	mBspNode_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numNodes = lump->fileLen / sizeof (*in);
	model->nodes = out= Mod_Alloc (model, sizeof (*out) * model->numNodes);

	//
	// byte swap
	//
	for (i=0 ; i<model->numNodes ; i++, in++, out++) {
		out->mins[0] = LittleShort (in->mins[0]);
		out->mins[1] = LittleShort (in->mins[1]);
		out->mins[2] = LittleShort (in->mins[2]);

		out->maxs[0] = LittleShort (in->maxs[0]);
		out->maxs[1] = LittleShort (in->maxs[1]);
		out->maxs[2] = LittleShort (in->maxs[2]);
	
		out->contents = -1;	// differentiate from leafs

		out->plane = model->planes + LittleLong (in->planeNum);
		out->firstSurface = LittleShort (in->firstFace);
		out->numSurfaces = LittleShort (in->numFaces);

		p = LittleLong (in->children[0]);
		out->children[0] = (p >= 0) ? model->nodes + p : (mBspNode_t *)(model->leafs + (-1 - p));

		p = LittleLong (in->children[1]);
		out->children[1] = (p >= 0) ? model->nodes + p : (mBspNode_t *)(model->leafs + (-1 - p));
	}

	//
	// set the nodes and leafs
	//
	Mod_SetParentNode (model->nodes, NULL);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspModel_t		*in;
	mBspHeader_t	*out;
	int				i;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numSubModels = lump->fileLen / sizeof (*in);
	model->subModels = out = Mod_Alloc (model, sizeof (*out) * model->numSubModels);

	//
	// byte swap
	//
	for (i=0 ; i<model->numSubModels ; i++, in++, out++) {
		// pad the mins / maxs by a pixel
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
Mod_LoadEntityString
=================
*/
void Mod_LoadEntityString (model_t *model, byte *byteBase, dBspLump_t *lump) {
	map_NumEntityChars = lump->fileLen;
	if (lump->fileLen > BSP_MAX_ENTSTRING)
		Com_Error (ERR_DROP, "Mod_LoadEntityString: Map has too large entity lump");

	memcpy (map_EntityString, byteBase + lump->fileOfs, lump->fileLen);
}


/*
=================
Mod_LoadBSPModel
=================
*/
void Mod_LoadBSPModel (model_t *model, byte *buffer) {
	dBspHeader_t	*header;
	mBspHeader_t	*bm;
	byte			*modBase;
	char			mapShaderName[MAX_QPATH];
	char			fileName[MAX_QPATH];
	int				version;
	int				i, len;

	//
	// load map-specific shader file
	//
	RS_FreeUnmarked ();

	Q_strncpyz (mapShaderName, model->name, sizeof (mapShaderName));
	len = Q_strlen (mapShaderName);
	mapShaderName[len-3]='e'; mapShaderName[len-2]='r'; mapShaderName[len-1]='s';
	Q_snprintfz (fileName, sizeof (fileName), "scripts/%s", mapShaderName);

	RS_ScanPathForShaders (qTrue);		// load all found shaders
	RS_LoadShader (fileName, qTrue, qTrue);

	//
	// load the world model
	//
	model->type = MODEL_BSP;
	if (model != modKnown)
		Com_Error (ERR_DROP, "Mod_LoadBSPModel: Loaded a model after the world");

	header = (dBspHeader_t *)buffer;

	version = LittleLong (header->version);
	if (version != BSP_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadBSPModel: %s has wrong version number (%i should be %i)", model->name, version, BSP_VERSION);

	r_WorldModel = model;
	rbCurrModel = model;

	//
	// swap all the lumps
	//
	modBase = (byte *)header;

	for (i=0 ; i<sizeof (dBspHeader_t)/4 ; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	//
	// load into heap
	//

	Mod_LoadVertexes	(model, modBase, &header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges		(model, modBase, &header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges	(model, modBase, &header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting	(model, modBase, &header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes		(model, modBase, &header->lumps[LUMP_PLANES]);
	Mod_LoadTexInfo		(model, modBase, &header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces		(model, modBase, &header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces(model, modBase, &header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility	(model, modBase, &header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs		(model, modBase, &header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes		(model, modBase, &header->lumps[LUMP_NODES]);
	Mod_LoadSubmodels	(model, modBase, &header->lumps[LUMP_MODELS]);
	Mod_LoadEntityString(model, modBase, &header->lumps[LUMP_ENTITIES]);

	//
	// set up the submodels
	//
	for (i=0 ; i<model->numSubModels ; i++) {
		model_t		*starmodel;

		bm = &model->subModels[i];
		starmodel = &modInline[i];

		*starmodel = *model;

		starmodel->firstModelSurface	= bm->firstFace;
		starmodel->numModelSurfaces		= bm->numFaces;
		starmodel->firstNode			= bm->headNode;

		if (starmodel->firstNode >= model->numNodes)
			Com_Error (ERR_DROP, "Mod_LoadBSPModel: Inline model number '%i' has a bad firstNode ( %d >= %d )", i, starmodel->firstNode, model->numNodes);

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
Mod_Free
================
*/
static void Mod_Free (model_t *model) {
	if (!model || (model->memSize <= 0))
		return;

	// free it
	Mem_FreeTags (MEMPOOL_MODELSYS, model->memTag);
	memset (model, 0, sizeof (*model));
}


/*
================
Mod_FindModel
================
*/
static inline model_t *Mod_FindModel (char *name) {
	model_t		*model;
	int			i;

	// inline models are grabbed only from worldmodel
	if (name[0] == '*') {
		i = atoi (name+1);
		if ((i < 1) || !r_WorldModel || (i >= r_WorldModel->numSubModels))
			Com_Error (ERR_DROP, "Bad inline model number '%d'", i);
		model = &modInline[i];
		model->memTag = modKnown[0].memTag;
		return model;
	}

	// search the currently loaded models
	for (i=0, model=modKnown ; i<modNumModels ; i++, model++) {
		if (!model->name[0])
			continue;

		if (!strcmp (model->name, name))
			return model;
	}

	return NULL;
}


/*
================
Mod_FindSlot
================
*/
static inline model_t *Mod_FindSlot (void) {
	model_t		*model;
	int			i;

	// find a free model slot spot
	for (i=0, model=modKnown ; i<modNumModels ; i++, model++) {
		if (!model->name[0]) {
			model->memTag = i;
			break;	// free spot
		}
	}

	if (i == modNumModels) {
		if (modNumModels >= MAX_MODEL_KNOWN)
			Com_Error (ERR_DROP, "modNumModels >= MAX_MODEL_KNOWN");

		model = &modKnown[modNumModels];
		model->memTag = modNumModels;
	}

	return model;
}


/*
==================
Mod_ForName

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
	{ MD2_HEADERSTR,	4,	ALIAS_MAX_LODS,		Mod_LoadMD2Model },	// Quake2 MD2 models
	{ MD3_HEADERSTR,	4,	ALIAS_MAX_LODS,		Mod_LoadMD3Model },	// Quake3 MD3 models
	{ MD5_HEADERSTR,	10,	ALIAS_MAX_LODS,		Mod_LoadMD5Model }, // Doom3 MD5 models
	{ SP2_HEADERSTR,	4,	0,					Mod_LoadSP2Model },	// Quake2 SP2 sprite models
	{ BSP_HEADERSTR,	4,	0,					Mod_LoadBSPModel },	// Quake2 BSP models

	{ NULL,				0,	0,					NULL }
};

static int numModelFormats = (sizeof (modelFormats) / sizeof (modelFormats[0])) - 1;

model_t *Mod_ForName (char *name, qBool crash) {
	model_t		*model;
	byte		*buffer;
	int			i, fileLen;
	modFormat_t	*descr;
	
	if (!name[0])
		Com_Error (ERR_DROP, "Mod_ForName: NULL name");

	// use if already loaded
	model = Mod_FindModel (name);
	if (model)
		return model;

	// not found -- allocate a spot
	model = Mod_FindSlot ();

	// load the file
	fileLen = FS_LoadFile (name, (void **)&buffer);
	if (!buffer || (fileLen <= 0)) {
		if (crash)
			Com_Error (ERR_DROP, "Mod_NumForName: %s not found", name);
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
		Com_Error (ERR_DROP, "Mod_ForName: unknown fileId for %s", model->name);

	modNumModels++;

	model->radius = 0;
	ClearBounds (model->mins, model->maxs);

	descr->loader (model, buffer);

	model->mRegistrationFrame = modRegistrationFrame;
	model->memSize = Mem_TagSize (MEMPOOL_MODELSYS, model->memTag);

	FS_FreeFile (buffer);

	return model;
}


/*
================
Mod_BeginRegistration

Specifies the model that will be used as the world
================
*/
void Mod_RegisterMap (char *mapName) {
	char	fullName[MAX_QPATH];

	r_OldViewCluster = r_ViewCluster = -1;	// force markleafs
	r_FrameCount = 1;

	Q_snprintfz (fullName, sizeof (fullName), "maps/%s.bsp", mapName);

	// explicitly free the old map if different
	// this guarantees that modKnown[0] is the world map
	if (strcmp (modKnown[0].name, fullName) || flushmap->integer) {
		Mod_Free (&modKnown[0]);

		memset (&r_WorldModel, 0, sizeof (r_WorldModel));
		memset (&r_WorldEntity, 0, sizeof (r_WorldEntity));
	}

	r_WorldModel = Mod_ForName (fullName, qTrue);
	r_WorldEntity.model = r_WorldModel;
}


/*
================
Mod_BeginRegistration

Highly complex function that starts model registration
================
*/
void Mod_BeginRegistration (void) {
	modRegistrationFrame++;
}


/*
================
Mod_EndRegistration
================
*/
void Mod_EndRegistration (void) {
	int		i;
	model_t	*mod;

	for (i=0, mod=modKnown ; i<modNumModels ; i++, mod++) {
		if (!mod->name[0])
			continue;	// free model_t spot
		if (!mod->mRegistrationFrame)
			continue;	// free model_t spot
		if (mod->mRegistrationFrame == modRegistrationFrame)
			continue;	// used this sequence

		Mod_Free (mod);
	}
}


/*
================
Mod_RegisterModel

Load/re-register a model
================
*/
model_t *Mod_RegisterModel (char *name) {
	model_t		*model;
	int			len;

	// check the name
	if (!name || !name[0])
		return NULL;

	// check the length
	len = Q_strlen (name);
	if (len >= MAX_QPATH) {
		Com_Printf (0, S_COLOR_RED "Mod_RegisterModel: Model name too long! %s\n", name);
		return NULL;
	}

	// md2 deprecation
	if (!strcmp (name + Q_strlen (name) - 4, ".md2")) {
		char	temp[MAX_QPATH];

		Q_snprintfz (temp, sizeof (temp), "%s.md3", Q_FixPathName (name, qTrue));
		model = Mod_RegisterModel (temp);

		if (model)
			return model;
/*orly		else {
			Q_snprintfz (temp, sizeof (temp), "%s.md5mesh", bareName);
			model = Mod_RegisterModel (temp);

			if (model)
				return model;
		}*/
	}

	// find or load it
	model = Mod_ForName (name, qFalse);

	// found it or loaded it -- touch it
	if (model) {
		mAliasModel_t	*aliasModel;
		mAliasMesh_t	aliasMesh;
		mAliasSkin_t	*aliasSkin;
		int				skinNum;

		mSpriteModel_t	*spriteModel;
		mSpriteFrame_t	*spriteFrame;

		int		i;

		model->mRegistrationFrame = modRegistrationFrame;

		switch (model->type) {
		case MODEL_MD2:
		case MODEL_MD3:
			aliasModel = model->aliasModel;
			for (i=0; i<aliasModel->numMeshes; i++) {
				aliasMesh = aliasModel->meshes[i];
				aliasSkin = aliasMesh.skins;
				for (skinNum=0 ; skinNum<aliasMesh.numSkins ; skinNum++) {
					if (!aliasSkin[skinNum].name[0]) {
						aliasSkin[skinNum].image = NULL;
						aliasSkin[skinNum].shader = NULL;
						continue;
					}

					aliasSkin[skinNum].image = Img_RegisterImage (aliasSkin[skinNum].name, 0);
					aliasSkin[skinNum].shader = RS_RegisterShader (aliasSkin[skinNum].name);
				}
			}
			break;

		case MODEL_MD5:
			// orly do me
			break;

		case MODEL_SP2:
			spriteModel = model->spriteModel;
			for (spriteFrame=spriteModel->frames, i=0 ; i<spriteModel->numFrames ; i++, spriteFrame++) {
				if (!spriteFrame) {
					spriteFrame->skin = NULL;
					spriteFrame->shader = NULL;
					continue;
				}

				spriteFrame->skin = Img_RegisterImage (spriteFrame->name, 0);
				spriteFrame->shader = RS_RegisterShader (spriteFrame->name);
			}
			break;

		case MODEL_BSP:
			for (i=0 ; i<model->numTexInfo ; i++) {
				model->texInfo[i].texture->iRegistrationFrame = imgRegistrationFrame;
			}
			break;
		}
	}

	return model;
}

/*
===============================================================================

	MISCELLANEOUS

===============================================================================
*/

/*
================
Mod_ModelList_f
================
*/
static void Mod_ModelList_f (void) {
	model_t	*mod;
	int		i, totalBytes;

	Com_Printf (0, "Loaded models:\n");

	totalBytes = 0;
	for (i=0, mod=modKnown ; i<modNumModels ; i++, mod++) {
		if (!mod->name[0])
			continue;

		switch (mod->type) {
		case MODEL_MD2:		Com_Printf (0, "MD2");				break;
		case MODEL_MD3:		Com_Printf (0, "MD3");				break;
		case MODEL_MD5:		Com_Printf (0, "MD5");				break;
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
void Mod_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs) {
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

/*
===============
Mod_Init
===============
*/
void Mod_Init (void) {
	// register commands/cvars
	flushmap	= Cvar_Get ("flushmap",		"0",		0);

	Cmd_AddCommand ("modellist",	Mod_ModelList_f,		"Prints to the console a list of loaded models and their sizes");

	memset (modNoVis, 0xff, sizeof (modNoVis));

	memset (&modInline, 0, sizeof (modInline));
	memset (&modKnown, 0, sizeof (modKnown));

	modNumModels = 0;
	modRegistrationFrame = 1;
}


/*
==================
Mod_Shutdown
==================
*/
void Mod_Shutdown (qBool full) {
	int		i;

	if (!full) {
		// remove commands
		Cmd_RemoveCommand ("modellist");
	}

	// free known loaded models
	for (i=0 ; i<modNumModels ; i++)
		Mod_Free (&modKnown[i]);

	memset (&modInline, 0, sizeof (modInline));
	memset (&modKnown, 0, sizeof (modKnown));
}
