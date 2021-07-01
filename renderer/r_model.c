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

// r_model.c
// - model loading and caching

#include "r_local.h"

int			mod_FileLen;

byte		mod_NoVis[BSP_MAX_VIS];

#define		MAX_MOD_KNOWN	512

model_t		mod_Inline[MAX_MOD_KNOWN];	// the inline * models from the current map are kept seperate
model_t		mod_Known[MAX_MOD_KNOWN];
int			mod_NumModels;

int			mod_RegistrationFrame;

cvar_t		*flushmap;

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
void Mod_LoadMD2Model (model_t *model, byte *buffer) {
	int				i, j;
	int				version, frameSize;
	int				skinWidth, skinHeight;
	int				numVerts, numIndexes;
	double			isw, ish;
	int				indRemap[MD2_MAX_TRIANGLES*3];
	index_t			tempIndex[MD2_MAX_TRIANGLES*3];
	index_t			tempSTIndex[MD2_MAX_TRIANGLES*3];
	dMd2Header_t	*inModel;
	dMd2Coord_t		*inCoord;
	dMd2Triangle_t	*inTri;
	dMd2Frame_t		*inFrame;
	mAliasModel_t	*outModel;
	mAliasMesh_t	*outMesh;
	index_t			*outIndex;
	vec2_t			*outCoord;
	mAliasFrame_t	*outFrame;
	mAliasVertex_t	*outVertex;
	mAliasSkin_t	*outSkins;
	vec3_t			normal;

	inModel = (dMd2Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD2_ALIAS_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: '%s' has wrong version number (%i != %i)",
							model->name, version, MD2_ALIAS_VERSION);

	outModel = model->aliasModel = Hunk_Alloc (sizeof (mAliasModel_t));
	model->type = MODEL_MD2;

	//
	// load the mesh
	//
	outMesh = outModel->meshes = Hunk_Alloc (sizeof (mAliasMesh_t));
	outModel->numMeshes = 1;
	Q_strncpyz (outMesh->meshName, "default", MAX_QPATH);

	outMesh->numVerts = LittleLong (inModel->numVerts);
	if ((outMesh->numVerts <= 0) || (outMesh->numVerts > MD2_MAX_VERTS))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of vertices '%d'",
							model->name, outMesh->numVerts);

	outMesh->numTris = LittleLong (inModel->numTris);
	if ((outMesh->numTris <= 0) || (outMesh->numTris > MD2_MAX_TRIANGLES))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of triangles '%d'",
							model->name, outMesh->numTris);

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
	// load triangle lists
	//
	inTri = (dMd2Triangle_t *) ((byte *)inModel + LittleLong (inModel->ofsTris));

	for (i=0 ; i<outMesh->numTris ; i++, inTri++) {
		for (j=0 ; j<3 ; j++) {
			tempIndex[i*3+j] = (index_t) LittleShort (inTri->vertsIndex[j]);
			tempSTIndex[i*3+j] = (index_t) LittleShort (inTri->stIndex[j]);
		}
	}

	//
	// build list of unique vertexes
	//
	numVerts = 0;
	numIndexes = outMesh->numTris * 3;
	outIndex = outMesh->indexes = Hunk_Alloc (sizeof (index_t) * numIndexes);
	memset (indRemap, -1, sizeof (int) * MD2_MAX_TRIANGLES * 3);

	for (i=0 ; i<numIndexes ; i++) {
		if (indRemap[i] != -1)
			continue;

		for (j=0 ; j<numIndexes ; j++) {
			if (j == i)
				continue;

			if ((tempIndex[i] == tempIndex[j]) && (tempSTIndex[i] == tempSTIndex[j]))
				indRemap[j] = i;
		}
	}

	//
	// count unique vertexes
	//
	for(i=0 ; i<numIndexes ; i++) {
		if (indRemap[i] != -1)
			continue;

		outIndex[i] = numVerts++;
		indRemap[i] = i;
	}

	Com_Printf (PRINT_DEV, "Mod_LoadMD2Model: '%s' remapped %i verts to %i verts\n",
							model->name, outMesh->numVerts, numVerts);
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
	inCoord = (dMd2Coord_t *) ((byte *) inModel + LittleLong (inModel->ofsST));
	outCoord = outMesh->coords = Hunk_Alloc (sizeof (vec2_t) * numVerts);

	for (j=0 ; j<numIndexes ; j++) {
		outCoord[outIndex[j]][0] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].s) + 0.5) * isw);
		outCoord[outIndex[j]][1] = (float)(((double)LittleShort (inCoord[tempSTIndex[indRemap[j]]].t) + 0.5) * ish);
	}

	//
	// load the frames
	//
	frameSize = LittleLong (inModel->frameSize);
	outModel->numFrames = LittleLong (inModel->numFrames);
	if ((outModel->numFrames <= 0) || (outModel->numFrames > MD2_MAX_FRAMES))
		Com_Error (ERR_DROP, "Mod_LoadMD2Model: model '%s' has an invalid amount of frames '%d'",
							model->name, outModel->numFrames);

	outFrame = outModel->frames = Hunk_Alloc (sizeof (mAliasFrame_t) * outModel->numFrames);
	outVertex = outMesh->vertexes = Hunk_Alloc (sizeof (mAliasVertex_t) * outModel->numFrames * numVerts);

	for (i=0 ; i<outModel->numFrames; i++, outFrame++, outVertex += numVerts) {
		inFrame = (dMd2Frame_t *) ((byte *)inModel + LittleLong (inModel->ofsFrames) + i * frameSize);

		for (j=0 ; j<3 ; j++) {
			outFrame->scale[j] = LittleFloat (inFrame->scale[j]);
			outFrame->translate[j] = LittleFloat (inFrame->translate[j]);
		}

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
			NormToLatLong (normal, outVertex[outIndex[j]].latlong);
		}
	}

	//
	// register all skins
	//
	outSkins = outMesh->skins = Hunk_Alloc (sizeof (mAliasSkin_t) * outMesh->numSkins);

	for (i=0 ; i<outMesh->numSkins ; i++, outSkins++) {
		if (LittleLong (inModel->ofsSkins) == -1)
			continue;

		strcpy (outSkins->name, (char *)inModel + LittleLong (inModel->ofsSkins) + i*MD2_MAX_SKINNAME);

		outSkins->image = Img_RegisterSkin (outSkins->name);
		outSkins->shader = RS_RegisterShader (outSkins->name);
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

	len = strlen (name);
	if (len <= 2)
		return;

	lodNum = atoi (&name[len - 1]);
	if (lodNum < MD3_ALIAS_MAX_LODS) {
		if (name[len-2] == '_') {
			name[len-2] = 0;
		}
	}
}


/*
=================
Mod_LoadMD3Model
=================
*/
void Mod_LoadMD3Model (model_t *model, byte *buffer) {
	dMd3Header_t		*inModel;
	dMd3Frame_t			*inFrame;
	dMd3Tag_t			*inTag;
	dMd3Mesh_t			*inMesh;
	dMd3Skin_t			*inSkin;
	dMd3Coord_t			*inCoord;
	dMd3Vertex_t		*inVert;
	index_t				*inIndex;
	mAliasModel_t		*outModel;
	mAliasFrame_t		*outFrame;
	mAliasTag_t			*outTag;
	mAliasMesh_t		*outMesh;
	mAliasSkin_t		*outSkin;
	vec2_t				*outCoord;
	mAliasVertex_t		*outVert;
	index_t				*outIndex;
	int					i, j, l;
	int					version;

	inModel = (dMd3Header_t *)buffer;

	version = LittleLong (inModel->version);
	if (version != MD3_ALIAS_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadMD3Model: model '%s' has wrong version number (%i != %i)",
							model->name, version, MD3_ALIAS_VERSION);

	model->aliasModel = outModel = Hunk_Alloc (sizeof (mAliasModel_t));
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
	outFrame = outModel->frames = Hunk_Alloc (sizeof (mAliasFrame_t) * outModel->numFrames);

	for (i=0 ; i<outModel->numFrames ; i++, inFrame++, outFrame++) {
		for (j=0 ; j<3 ; j++) {
			outFrame->scale[j] = MD3_XYZ_SCALE;
			outFrame->translate[j] = LittleFloat (inFrame->translate[j]);
		}

		// never trust the modeler utility and recalculate bbox and radius
		ClearBounds (outFrame->mins, outFrame->maxs);
	}

	//
	// load the tags
	//
	inTag = (dMd3Tag_t *)((byte *)inModel + LittleLong (inModel->ofsTags));
	outTag = outModel->tags = Hunk_Alloc (sizeof (mAliasTag_t) * outModel->numFrames * outModel->numTags);

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

			Q_strncpyz (outTag->tagName, inTag->tagName, MAX_QPATH);
		}
	}

	//
	// load the meshes
	//
	inMesh = (dMd3Mesh_t *)((byte *)inModel + LittleLong (inModel->ofsMeshes));
	outMesh = outModel->meshes = Hunk_Alloc (sizeof (mAliasMesh_t) * outModel->numMeshes);

	for (i=0 ; i<outModel->numMeshes ; i++, outMesh++) {
		Q_strncpyz (outMesh->meshName, inMesh->meshName, MAX_QPATH);
		if (strncmp ((const char *)inMesh->ident, MD3_HEADERSTR, 4))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has wrong id (%i != %i)",
								inMesh->meshName, model->name, LittleLong ((int)inMesh->ident), MD3_HEADER);

		Mod_StripLODSuffix (outMesh->meshName);

		outMesh->numSkins = LittleLong (inMesh->numSkins);
		if ((outMesh->numSkins <= 0) || (outMesh->numSkins > MD3_MAX_SHADERS))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of skins '%d'",
								inMesh->meshName, model->name, outMesh->numSkins);

		outMesh->numTris = LittleLong (inMesh->numTris);
		if ((outMesh->numTris <= 0) || (outMesh->numTris > MD3_MAX_TRIANGLES))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of triangles '%d'",
								inMesh->meshName, model->name, outMesh->numTris);

		outMesh->numVerts = LittleLong (inMesh->numVerts);
		if ((outMesh->numVerts <= 0) || (outMesh->numVerts > MD3_MAX_VERTS))
			Com_Error (ERR_DROP, "Mod_LoadMD3Model: mesh '%s' in model '%s' has an invalid amount of vertices '%d'",
								inMesh->meshName, model->name, outMesh->numVerts);

		//
		// load the skins
		//
		inSkin = (dMd3Skin_t *)((byte *)inMesh + LittleLong (inMesh->ofsSkins));
		outSkin = outMesh->skins = Hunk_Alloc (sizeof (mAliasSkin_t) * outMesh->numSkins);

		for (j=0 ; j<outMesh->numSkins ; j++, inSkin++, outSkin++) {
			Q_strncpyz (outSkin->name, inSkin->name, MAX_QPATH);

			outSkin->image = Img_RegisterSkin (outSkin->name);
			outSkin->shader = RS_RegisterShader (outSkin->name);
		}

		//
		// load the indexes
		//
		inIndex = (index_t *)((byte *)inMesh + LittleLong (inMesh->ofsIndexes));
		outIndex = outMesh->indexes = Hunk_Alloc (sizeof (index_t) * outMesh->numTris * 3);

		for (j=0 ; j<outMesh->numTris ; j++, inIndex += 3, outIndex += 3) {
			outIndex[0] = (index_t)LittleLong (inIndex[0]);
			outIndex[1] = (index_t)LittleLong (inIndex[1]);
			outIndex[2] = (index_t)LittleLong (inIndex[2]);
		}

		//
		// load the texture coordinates
		//
		inCoord = (dMd3Coord_t *)((byte *)inMesh + LittleLong (inMesh->ofsTCs));
		outCoord = outMesh->coords = Hunk_Alloc (sizeof (vec2_t) * outMesh->numVerts);

		for (j=0 ; j<outMesh->numVerts ; j++, inCoord++) {
			outCoord[j][0] = LittleFloat (inCoord->st[0]);
			outCoord[j][1] = LittleFloat (inCoord->st[1]);
		}

		//
		// load the vertexes and normals
		//
		inVert = (dMd3Vertex_t *)((byte *)inMesh + LittleLong (inMesh->ofsVerts));
		outVert = outMesh->vertexes = Hunk_Alloc (sizeof (mAliasVertex_t) * outModel->numFrames * outMesh->numVerts);
		outFrame = outModel->frames;

		for (l=0 ; l<outModel->numFrames ; l++, outFrame++) {
			vec3_t	v;

			for (j=0 ; j<outMesh->numVerts ; j++, inVert++, outVert++) {
				outVert->point[0] = LittleShort (inVert->point[0]);
				outVert->point[1] = LittleShort (inVert->point[1]);
				outVert->point[2] = LittleShort (inVert->point[2]);

				VectorCopy (outVert->point, v);
				AddPointToBounds (v, outFrame->mins, outFrame->maxs);

				outVert->latlong[0] = inVert->norm[0];
				outVert->latlong[1] = inVert->norm[1];
			}
		}

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

	SP2 LOADING

===============================================================================
*/

/*
=================
Mod_LoadSP2Model
=================
*/
void Mod_LoadSP2Model (model_t *model, byte *buffer) {
	dSpriteHeader_t	*inModel;
	dSpriteFrame_t	*inFrames;
	mSpriteModel_t	*outModel;
	mSpriteFrame_t	*outFrames;
	int				i, version;

	inModel = (dSpriteHeader_t *)buffer;

	model->type = MODEL_SP2;

	//
	// sanity checks
	//
	version = LittleLong (inModel->version);
	if (version != SP2_VERSION)
		Com_Error (ERR_DROP, "%s has wrong version number (%i should be %i)", model->name, version, SP2_VERSION);

	model->spriteModel = outModel = Hunk_Alloc (sizeof (mSpriteModel_t));

	outModel->numFrames = LittleLong (inModel->numFrames);
	if (outModel->numFrames > SP2_MAX_FRAMES)
		Com_Error (ERR_DROP, "Mod_LoadSP2Model: model '%s' has too many frames (%i > %i)", model->name, outModel->numFrames, SP2_MAX_FRAMES);

	//
	// byte swap everything
	//
	inFrames = inModel->frames;
	outModel->frames = outFrames = Hunk_Alloc (sizeof (mSpriteFrame_t) * outModel->numFrames);

	for (i=0 ; i<outModel->numFrames ; i++, inFrames++, outFrames++) {
		outFrames->width		= LittleLong (inFrames->width);
		outFrames->height		= LittleLong (inFrames->height);
		outFrames->originX		= LittleLong (inFrames->originX);
		outFrames->originY		= LittleLong (inFrames->originY);

		outFrames->radius		= sqrt ((outFrames->width*outFrames->width) + (outFrames->height*outFrames->height));
		model->radius			= max (model->radius, outFrames->radius);

		//
		// register all skins
		//
		Q_strncpyz (outFrames->name, inFrames->name, MAX_QPATH);

		outFrames->skin = Img_FindImage (outFrames->name, 0);
		outFrames->shader = RS_RegisterShader (outFrames->name);
	}
}

/*
===============================================================================

	BRUSH MODELS

===============================================================================
*/

/*
================
Mod_SubdivideSurface

Breaks a polygon up along axial 64 unit boundaries so that
turbulent can be done reasonably.
================
*/
#define	SUBDIVIDE_SIZE	64
void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs) {
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;

	for (i=0 ; i<numverts ; i++) {
		for (j=0 ; j<3 ; j++, v++) {
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
	}
}

void SubdividePolygon (mBspSurface_t *warpface, int numverts, float *verts) {
	int			i, j, k;
	vec3_t		mins, maxs;
	float		m;
	float		*v;
	vec3_t		front[64], back[64];
	int			f, b;
	float		dist[64];
	float		frac;
	mBspPoly_t	*poly;
	float		s, t;
	vec3_t		total;
	float		total_s, total_t;

	if (numverts > 60)
		Com_Error (ERR_DROP, "numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor (m/SUBDIVIDE_SIZE + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3) {
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
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (warpface, f, front[0]);
		SubdividePolygon (warpface, b, back[0]);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Hunk_Alloc (sizeof (mBspPoly_t) + ((numverts-4)+2) * VERTEXSIZE * sizeof (float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts+2;
	VectorIdentity (total);
	total_s = 0;
	total_t = 0;
	for (i=0 ; i<numverts ; i++, verts+= 3) {
		VectorCopy (verts, poly->verts[i+1]);
		s = DotProduct (verts, warpface->texInfo->vecs[0]);
		t = DotProduct (verts, warpface->texInfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd (total, verts, total);

		poly->verts[i+1][3] = s;
		poly->verts[i+1][4] = t;
	}

	VectorScale (total, (1.0/numverts), poly->verts[0]);
	poly->verts[0][3] = total_s/numverts;
	poly->verts[0][4] = total_t/numverts;

	// copy first vertex to last
	memcpy (poly->verts[i+1], poly->verts[1], sizeof (poly->verts[0]));
}

void Mod_SubdivideSurface (model_t *model, mBspSurface_t *surf) {
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	// convert edges back to a normal polygon
	numverts = 0;
	for (i=0 ; i<surf->numEdges ; i++) {
		lindex = model->surfEdges[surf->firstEdge + i];

		if (lindex > 0)
			vec = model->vertexes[model->edges[lindex].v[0]].position;
		else
			vec = model->vertexes[model->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (surf, numverts, verts[0]);
}


/*
================
Mod_SubdivideLightmapSurface

Breaks a polygon up along axial arbitary ^2 unit boundaries so that vertex warps
can be done without looking like shit.

Surface Subdivision
Modified By Robert 'Heffo' Heffernan
6/1/2002 12:13pm GMT +10
================
*/
#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128
void SubdivideLightmappedPolygon (mBspSurface_t *warpface, int numverts, float *verts, float subdivide_size) {
	int			i, j, k;
	vec3_t		mins, maxs;
	float		m;
	float		*v;
	vec3_t		front[64], back[64];
	int			f, b;
	float		dist[64];
	float		frac;
	mBspPoly_t	*poly;
	float		s, t;
	vec3_t		total;
	float		total_s, total_t, total_u, total_v;

	if (numverts > 60)
		Sys_Error ("numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++) {
		m = (mins[i] + maxs[i]) * 0.5;
		m = subdivide_size * floor (m/subdivide_size + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3) {
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
			if ( (dist[j] > 0) != (dist[j+1] > 0)) {
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdivideLightmappedPolygon (warpface, f, front[0], subdivide_size);
		SubdivideLightmappedPolygon (warpface, b, back[0], subdivide_size);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Hunk_Alloc (sizeof (mBspPoly_t) + ((numverts-4)+2) * VERTEXSIZE * sizeof (float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts+2;
	VectorIdentity (total);
	total_s = 0; total_t = 0;
	total_u = 0; total_v = 0;

	for (i=0 ; i<numverts ; i++, verts+= 3) {
		VectorCopy (verts, poly->verts[i+1]);
		s = DotProduct (verts, warpface->texInfo->vecs[0]) + warpface->texInfo->vecs[0][3];
		s /= warpface->texInfo->image->width;

		t = DotProduct (verts, warpface->texInfo->vecs[1]) + warpface->texInfo->vecs[1][3];
		t /= warpface->texInfo->image->height;

		total_s += s;
		total_t += t;
		VectorAdd (total, verts, total);

		poly->verts[i+1][3] = s;
		poly->verts[i+1][4] = t;

		// lightmap texture coordinates
		s = DotProduct (verts, warpface->texInfo->vecs[0]) + warpface->texInfo->vecs[0][3];
		s -= warpface->textureMins[0];
		s += warpface->lmCoords[0]*16;
		s += 8;
		s /= BLOCK_WIDTH*16;

		t = DotProduct (verts, warpface->texInfo->vecs[1]) + warpface->texInfo->vecs[1][3];
		t -= warpface->textureMins[1];
		t += warpface->lmCoords[1]*16;
		t += 8;
		t /= BLOCK_HEIGHT*16;

		total_u += s;
		total_v += t;

		poly->verts[i+1][5] = s;
		poly->verts[i+1][6] = t;
	}

	VectorScale (total, (1.0/numverts), poly->verts[0]);
	poly->verts[0][3] = total_s/numverts;
	poly->verts[0][4] = total_t/numverts;

	poly->verts[0][5] = total_u/numverts;
	poly->verts[0][6] = total_v/numverts;

	// copy first vertex to last
	memcpy (poly->verts[i+1], poly->verts[1], sizeof(poly->verts[0]));
}

void Mod_SubdivideLightmapSurface (model_t *model, mBspSurface_t *surf, float subdivideSize) {
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	// convert edges back to a normal polygon
	numverts = 0;
	for (i=0 ; i<surf->numEdges ; i++) {
		lindex = model->surfEdges[surf->firstEdge + i];

		if (lindex > 0)
			vec = model->vertexes[model->edges[lindex].v[0]].position;
		else
			vec = model->vertexes[model->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdivideLightmappedPolygon (surf, numverts, verts[0], subdivideSize);
}


/*
================
Mod_CalcSurfaceExtents

Fills in s->textureMins[] and s->extents[]
================
*/
void Mod_CalcSurfaceExtents (model_t *model, mBspSurface_t *surf) {
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
		if (e >= 0)
			v = &model->vertexes[model->edges[e].v[0]];
		else
			v = &model->vertexes[model->edges[-e].v[1]];
		
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
		return mod_NoVis;

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
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
}


/*
=================
Mod_SetParentNode
=================
*/
void Mod_SetParentNode (mBspNode_t *node, mBspNode_t *parent) {
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
	model->vertexes = out = Hunk_Alloc (sizeof (*out) * model->numVertexes);

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
	model->edges = out = Hunk_Alloc (sizeof (*out) * (model->numEdges + 1));

	//
	// byte swap
	//
	for (i=0 ; i<model->numEdges ; i++, in++, out++) {
		out->v[0] = (unsigned short) LittleShort (in->v[0]);
		out->v[1] = (unsigned short) LittleShort (in->v[1]);
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

	model->surfEdges = out = Hunk_Alloc (sizeof (*out) * model->numSurfEdges);

	//
	// byte swap
	//
	for (i=0 ; i<model->numSurfEdges ; i++)
		out[i] = LittleLong (in[i]);
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

	model->lightData = Hunk_Alloc (lump->fileLen);	
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
	model->planes = out = Hunk_Alloc (sizeof (*out) * model->numPlanes * 2);

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
	model->texInfo = out = Hunk_Alloc (sizeof (*out) * model->numTexInfo);

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
		Com_sprintf (name, sizeof (name), "textures/%s.wal", in->texture);
		Com_sprintf (name, sizeof (name), "textures/%s.wal", in->texture);
		out->image = Img_FindImage (name, IT_TEXTURE);
		out->shader = RS_RegisterShader (name);
		if (!out->image) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Couldn't load %s\n", name);
			out->image = r_NoTexture;
		}
	}

	//
	// count animation frames
	//
	for (i=0 ; i<model->numTexInfo ; i++) {
		out = &model->texInfo[i];
		out->numFrames = 1;
		for (step = out->next ; step && (step != out) ; step=step->next)
			out->numFrames++;
	}
}


/*
=================
Mod_LoadFaces
=================
*/
index_t r_quad_indexes[6] = {0,1,2,0,2,3};
void Mod_LoadFaces (model_t *model, byte *byteBase, dBspLump_t *lump) {
	dBspSurface_t	*in;
	mBspSurface_t	*out;
	int				i, surfnum;
	int				planeNum, side;
	int				ti;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numSurfaces = lump->fileLen / sizeof (*in);
	model->surfaces = out= Hunk_Alloc (sizeof (*out) * model->numSurfaces);

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

		ti = LittleShort (in->texInfo);
		if ((ti < 0) || (ti >= model->numTexInfo))
			Com_Error (ERR_DROP, "MOD_LoadBmodel: bad texInfo number");
		out->texInfo = model->texInfo + ti;

		Mod_CalcSurfaceExtents (model, out);

		//
		// lighting info
		//
		out->lmWidth = (out->extents[0]>>4) + 1;
		out->lmHeight = (out->extents[1]>>4) + 1;

		i = LittleLong (in->lightOfs);
		if (i == -1)
			out->lmSamples = NULL;
		else
			out->lmSamples = model->lightData + i;

		for (out->numStyles=0 ; out->numStyles<BSP_MAX_LIGHTMAPS && in->styles[out->numStyles] != 255 ; out->numStyles++) {
			out->styles[out->numStyles] = in->styles[out->numStyles];
		}

		//
		// set the drawing flags
		//
		if (out->texInfo->flags & SURF_WARP) {
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++) {
				out->extents[i] = 16384;
				out->textureMins[i] = -8192;
			}

			// cut up polygon for warps
			Mod_SubdivideSurface (model, out);
		}

		//
		// create lightmaps and polygons
		//
		if (!(out->texInfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)))
			LM_CreateSurfaceLightmap (out);

		if (!(out->texInfo->flags & SURF_WARP)) {
			if (out->texInfo->shader) {
				shader_t *shader = out->texInfo->shader;

				if (shader->subDivide)
					Mod_SubdivideLightmapSurface (model, out, shader->subDivide);
				else {
					shader_t *shader = out->texInfo->shader;

					if (shader->stage->texture != NULL)
						out->texInfo->image = shader->stage->texture;
					else if (shader->stage->animStage != NULL)
						out->texInfo->image = shader->stage->animStage->texture;

					R_BuildPolygonFromSurface (model, out);
				}
			} else
				R_BuildPolygonFromSurface (model, out);
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
	int			i, j;
	short		*in;
	mBspSurface_t	**out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numMarkSurfaces = lump->fileLen / sizeof (*in);
	model->markSurfaces = out = Hunk_Alloc (sizeof (*out) * model->numMarkSurfaces);

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

	model->vis = Hunk_Alloc (lump->fileLen);	
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
	model->leafs = out = Hunk_Alloc (sizeof (*out) * model->numLeafs);

	//
	// byte swap
	//
	for (i=0 ; i<model->numLeafs ; i++, in++, out++) {
		for (j=0 ; j<3 ; j++) {
			out->mins[j] = LittleShort (in->mins[j]);
			out->maxs[j] = LittleShort (in->maxs[j]);
		}

		out->contents	= LittleLong (in->contents);
		out->cluster	= LittleShort (in->cluster);
		out->area		= LittleShort (in->area);

		out->firstMarkSurface = model->markSurfaces + LittleShort (in->firstLeafFace);
		out->numMarkSurfaces = LittleShort (in->numLeafFaces);

		//
		// mark poly flags
		//
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)) {
			for (j=0 ; j<out->numMarkSurfaces ; j++) {
				out->firstMarkSurface[j]->flags |= SURF_UNDERWATER;
				for (poly = out->firstMarkSurface[j]->polys ; poly ; poly=poly->next) {
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
	int			i, j, p;
	dBspNode_t	*in;
	mBspNode_t	*out;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numNodes = lump->fileLen / sizeof (*in);
	model->nodes = out= Hunk_Alloc (sizeof (*out) * model->numNodes);

	//
	// byte swap
	//
	for (i=0 ; i<model->numNodes ; i++, in++, out++) {
		for (j=0 ; j<3 ; j++) {
			out->mins[j] = LittleShort (in->mins[j]);
			out->maxs[j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong (in->planeNum);
		out->plane = model->planes + p;

		out->firstSurface = LittleShort (in->firstFace);
		out->numSurfaces = LittleShort (in->numFaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++) {
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = model->nodes + p;
			else
				out->children[j] = (mBspNode_t *)(model->leafs + (-1 - p));
		}
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
	int				i, j;

	in = (void *)(byteBase + lump->fileOfs);
	if (lump->fileLen % sizeof (*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s", model->name);

	model->numSubModels = lump->fileLen / sizeof (*in);
	model->subModels = out = Hunk_Alloc (sizeof (*out) * model->numSubModels);

	//
	// byte swap
	//
	for (i=0 ; i<model->numSubModels ; i++, in++, out++) {
		for (j=0 ; j<3 ; j++) {
			//
			// pad the mins / maxs by a pixel
			//
			out->mins[j]	= LittleFloat (in->mins[j]) - 1;
			out->maxs[j]	= LittleFloat (in->maxs[j]) + 1;
			out->origin[j]	= LittleFloat (in->origin[j]);
		}
		out->radius		= RadiusFromBounds (out->mins, out->maxs);

		out->headNode	= LittleLong (in->headNode);
		out->firstFace	= LittleLong (in->firstFace);
		out->numFaces	= LittleLong (in->numFaces);
	}
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

	strcpy (mapShaderName, model->name);
	len = strlen (mapShaderName);
	mapShaderName[len-3]='e'; mapShaderName[len-2]='r'; mapShaderName[len-1]='s';
	Com_sprintf (fileName, sizeof (fileName), "scripts/%s", mapShaderName);

	RS_ScanPathForShaders (qTrue);		// load all found shaders
	RS_LoadShader (fileName, qTrue, qTrue);

	//
	// load the world model
	//
	model->type = MODEL_BSP;
	if (model != mod_Known)
		Com_Error (ERR_DROP, "Loaded a brush model after the world");

	header = (dBspHeader_t *)buffer;

	version = LittleLong (header->version);
	if (version != BSP_VERSION)
		Com_Error (ERR_DROP, "Mod_LoadBSPModel: %s has wrong version number (%i should be %i)", model->name, version, BSP_VERSION);

	r_WorldModel = model;
	r_CurrModel = model;

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

	//
	// set up the submodels
	//
	for (i=0 ; i<model->numSubModels ; i++) {
		model_t		*starmodel;

		bm = &model->subModels[i];
		starmodel = &mod_Inline[i];

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
Mod_FindModel
================
*/
model_t *Mod_FindModel (char *name) {
	model_t		*model;
	int			i;

	// inline models are grabbed only from worldmodel
	if (name[0] == '*') {
		i = atoi (name+1);
		if ((i < 1) || !r_WorldModel || (i >= r_WorldModel->numSubModels))
			Com_Error (ERR_DROP, "Bad inline model number '%d'", i);
		return &mod_Inline[i];
	}

	// search the currently loaded models
	for (i=0, model=mod_Known ; i<mod_NumModels ; i++, model++) {
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
model_t *Mod_FindSlot (void) {
	model_t		*model;
	int			i;

	// find a free model slot spot
	for (i=0, model=mod_Known ; i<mod_NumModels ; i++, model++) {
		if (!model->name[0])
			break;	// free spot
	}

	if (i == mod_NumModels) {
		if (mod_NumModels >= MAX_MOD_KNOWN)
			Com_Error (ERR_DROP, "mod_NumModels >= MAX_MOD_KNOWN");
		model = &mod_Known[mod_NumModels];
	}

	return model;
}


/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
typedef struct {
	const char		*headerStr;
	int				headerLen;
	int				maxLods;
	int				size;
	void			(*loader) (model_t *mod, void *buffer);
} modFormat_t;

static modFormat_t mod_MdlFormats[] = {
	{ MD2_HEADERSTR,	4,	MD3_ALIAS_MAX_LODS,	0x300000,	Mod_LoadMD2Model },	// Quake2 MD2 models
	{ MD3_HEADERSTR,	4,	MD3_ALIAS_MAX_LODS,	0x500000,	Mod_LoadMD3Model },	// Quake3 MD3 models
	{ SP2_HEADERSTR,	4,	0,					0x10000,	Mod_LoadSP2Model },	// Quake2 SP2 sprite models
	{ BSP_HEADERSTR,	4,	0,					0x1000000,	Mod_LoadBSPModel },	// Quake2 BSP models

	{ NULL,				0,	0,					0,			NULL }
};

static int mod_NumMdlFormats = (sizeof (mod_MdlFormats) / sizeof(mod_MdlFormats[0])) - 1;

model_t *Mod_ForName (char *name, qBool crash) {
	model_t		*model;
	byte		*buffer;
	int			i;
	modFormat_t	*descr;
	
	if (!name[0])
		Com_Error (ERR_DROP, "Mod_ForName: NULL name");

	// use if already loaded
	model = Mod_FindModel (name);
	if (model)
		return model;

	// not found -- allocate a spot and load from disk
	model = Mod_FindSlot ();

	// load the file
	mod_FileLen = FS_LoadFile (name, (void **)&buffer);
	if (!buffer) {
		if (crash)
			Com_Error (ERR_DROP, "Mod_NumForName: %s not found", name);
		memset (model->name, 0, sizeof (model->name));
		return NULL;
	} else
		Q_strncpyz (model->name, name, sizeof (model->name));

	descr = mod_MdlFormats;
	for (i=0 ; i<mod_NumMdlFormats ; i++, descr++) {
		if (!strncmp ((const char *)buffer, descr->headerStr, descr->headerLen))
			break;
	}

	if (i == mod_NumMdlFormats)
		Com_Error (ERR_DROP, "Mod_ForName: unknown fileId for %s", model->name);

	mod_NumModels++;
	model->extraData = Hunk_Begin (descr->size);

	model->radius = 0;
	ClearBounds (model->mins, model->maxs);

	descr->loader (model, buffer);

	model->mRegistrationFrame = mod_RegistrationFrame;
	model->extraDataSize = Hunk_End ();

	FS_FreeFile (buffer);

	return model;
}


/*
================
Mod_BeginRegistration

Specifies the model that will be used as the world and starts registration
================
*/
void Mod_BeginRegistration (char *model) {
	char	fullName[MAX_QPATH];

	mod_RegistrationFrame++;
	r_FrameCount = 1;						// no dlightcache
	r_OldViewCluster = r_ViewCluster = -1;	// force markleafs

	Com_sprintf (fullName, sizeof (fullName), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_Known[0] is the world map
	if (strcmp (mod_Known[0].name, fullName) || flushmap->integer) {
		memset (&r_WorldModel, 0, sizeof (r_WorldModel));
		Mod_Free (&mod_Known[0]);
	}

	r_WorldModel = Mod_ForName (fullName, qTrue);
}


/*
================
Mod_EndRegistration
================
*/
void Mod_EndRegistration (void) {
	int		i;
	model_t	*mod;

	for (i=0, mod=mod_Known ; i<mod_NumModels ; i++, mod++) {
		if (!mod->name[0])
			continue;	// free model_t spot
		if (!mod->mRegistrationFrame)
			continue;	// free model_t spot
		if (mod->mRegistrationFrame == mod_RegistrationFrame)
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

	if (!name[0])
		return NULL;

	len = strlen (name);
	model = NULL;

	if (!strcmp (name+len-4, ".md2")) {
		char	temp[MAX_QPATH];

		strcpy (temp, name);
		temp[len-1] = '3';
		model = Mod_RegisterModel (temp);

		if (model)
			return model;
	}

	model = Mod_ForName (name, qFalse);

	// found -- touch skins/shaders
	if (model) {
		int		i;

		model->mRegistrationFrame = mod_RegistrationFrame;

		switch (model->type) {
		case MODEL_MD2:
		case MODEL_MD3: {
				mAliasModel_t	*aliasModel;
				mAliasMesh_t	aliasMesh;
				mAliasSkin_t	*aliasSkin;
				int				skinNum;

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

						aliasSkin[skinNum].image = Img_RegisterSkin (aliasSkin[skinNum].name);
						aliasSkin[skinNum].shader = RS_RegisterShader (aliasSkin[skinNum].name);
					}
				}
			}
			break;
		case MODEL_SP2: {
				mSpriteModel_t	*spriteModel;
				mSpriteFrame_t	*spriteFrame;

				spriteModel = model->spriteModel;
				for (spriteFrame=spriteModel->frames, i=0 ; i<spriteModel->numFrames ; i++, spriteFrame++) {
					if (!spriteFrame) {
						spriteFrame->skin = NULL;
						spriteFrame->shader = NULL;
						continue;
					}

					spriteFrame->skin = Img_FindImage (spriteFrame->name, 0);
					spriteFrame->shader = RS_RegisterShader (spriteFrame->name);
				}
			}
		case MODEL_BSP: {
				for (i=0 ; i<model->numTexInfo ; i++)
					model->texInfo[i].image->iRegistrationFrame = img_RegistrationFrame;
			}
		}
	}

	return model;
}


/*
================
Mod_Free
================
*/
void Mod_Free (model_t *mod) {
	Hunk_Free (mod->extraData);
	memset (mod, 0, sizeof (*mod));
}


/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void) {
	int		i;

	for (i=0 ; i<mod_NumModels ; i++) {
		if (mod_Known[i].extraDataSize)
			Mod_Free (&mod_Known[i]);
	}
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
void Mod_ModelList_f (void) {
	int		i;
	model_t	*mod;
	int		total;

	Com_Printf (PRINT_ALL, "Loaded models:\n");

	for (total=0, i=0, mod=mod_Known ; i<mod_NumModels ; i++, mod++, total += mod->extraDataSize) {
		if (!mod->name[0])
			continue;
		Com_Printf (PRINT_ALL, "%8i : %s\n", mod->extraDataSize, mod->name);
	}

	Com_Printf (PRINT_ALL, "Total resident: %i\n", total);
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
	Cmd_AddCommand ("modellist",	Mod_ModelList_f);

	mod_RegistrationFrame = 1;
	memset (mod_NoVis, 0xff, sizeof (mod_NoVis));
}


/*
==================
Mod_Shutdown
==================
*/
void Mod_Shutdown (void) {
	Cmd_RemoveCommand ("modellist");

	Mod_FreeAll ();
}
