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
// r_shader.c
// Shader loading, caching, and some primitive surface rendering
//

#define USE_FIXPATHNAME
#define USE_HASH
#include "r_local.h"
#include <io.h>

#define SHADER_TOKDELIMINATORS	"\r\n\t "
#define MAX_SHADER_HASH			128

static shader_t	*loadedShaders;
static shader_t	*shHashTree[MAX_SHADER_HASH];

cVar_t	*r_shaders;

/*
=============================================================================

	STAGE PROCESSING

=============================================================================
*/

/*
==================
RS_ResetStage
==================
*/
static void RS_ResetStage (shaderStage_t *stage) {
	shaderAnimStage_t	*anim = stage->animStage, *tmp_anim;

	while (anim != NULL) {
		tmp_anim = anim;
		anim = anim->next;
		Mem_Free (tmp_anim);
	}

	stage->texture = NULL;
	stage->name[0] = 0;
	stage->flags = 0;

	stage->animStage = NULL;
	stage->animDelay = 0;
	stage->animCount = 0;
	stage->animTimeLast = 0;
	stage->animLast = 0;

	stage->blendFunc.blend = qFalse;
	stage->blendFunc.dest = GL_SRC_ALPHA;
	stage->blendFunc.source = GL_ONE_MINUS_SRC_ALPHA;

	stage->alphaShift.max = 0;
	stage->alphaShift.min = 0;
	stage->alphaShift.speed = 0;

	stage->scroll.speedX = 0;
	stage->scroll.speedY = 0;
	stage->scroll.typeX = 0;
	stage->scroll.typeY = 0;

	stage->scale.scaleX = 0;
	stage->scale.scaleY = 0;
	stage->scale.typeX = 0;
	stage->scale.typeY = 0;

	stage->rotateSpeed = 0;

	stage->cubeMap = qFalse;
	stage->envMap = qFalse;
	stage->lightMap = qTrue;
	stage->alphaMask = qFalse;
	stage->lightMapOnly = qFalse;

	stage->next = NULL;
}


/*
==================
RS_NewStage
==================
*/
static shaderStage_t *RS_NewStage (shader_t *shader) {
	shaderStage_t	*stage;

	if (shader->stage == NULL) {
		shader->stage = (shaderStage_t *) Mem_PoolAlloc (sizeof (shaderStage_t), MEMPOOL_SHADERSYS, 0);
		stage = shader->stage;
	}
	else {
		stage = shader->stage;

		while (stage->next != NULL)
			stage = stage->next;

		stage->next = (shaderStage_t *) Mem_PoolAlloc (sizeof (shaderStage_t), MEMPOOL_SHADERSYS, 0);
		stage = stage->next;
	}

	Q_strncpyz (stage->name, "NULL", sizeof (stage->name));

	stage->animStage = NULL;
	stage->next = NULL;
	stage->animLast = NULL;

	RS_ResetStage (stage);

	return stage;
}


/*
==================
RS_BlendID
==================
*/
static inline int RS_BlendID (char *blend) {
	if (!blend[0])
		return 0;

	if (!Q_stricmp (blend, "GL_ZERO"))					return GL_ZERO;
	if (!Q_stricmp (blend, "GL_ONE"))					return GL_ONE;
	if (!Q_stricmp (blend, "GL_DST_COLOR"))				return GL_DST_COLOR;
	if (!Q_stricmp (blend, "GL_ONE_MINUS_DST_COLOR"))	return GL_ONE_MINUS_DST_COLOR;
	if (!Q_stricmp (blend, "GL_SRC_ALPHA"))				return GL_SRC_ALPHA;
	if (!Q_stricmp (blend, "GL_ONE_MINUS_SRC_ALPHA"))	return GL_ONE_MINUS_SRC_ALPHA;
	if (!Q_stricmp (blend, "GL_DST_ALPHA"))				return GL_DST_ALPHA;
	if (!Q_stricmp (blend, "GL_ONE_MINUS_DST_ALPHA"))	return GL_ONE_MINUS_DST_ALPHA;
	if (!Q_stricmp (blend, "GL_SRC_ALPHA_SATURATE"))	return GL_SRC_ALPHA_SATURATE;
	if (!Q_stricmp (blend, "GL_SRC_COLOR"))				return GL_SRC_COLOR;
	if (!Q_stricmp (blend, "GL_ONE_MINUS_SRC_COLOR"))	return GL_ONE_MINUS_SRC_COLOR;

	return 0;
}


/*
==================
RS_FuncName
==================
*/
static inline int RS_FuncName (char *text) {
	// static
	if (!Q_stricmp (text, "static"))		return STAGE_STATIC;

	// sine wave
	else if (!Q_stricmp (text, "sin"))		return STAGE_SINE;
	else if (!Q_stricmp (text, "sine"))		return STAGE_SINE;

	// cosine wave
	else if (!Q_stricmp (text, "cos"))		return STAGE_COSINE;
	else if (!Q_stricmp (text, "cose"))		return STAGE_COSINE;
	else if (!Q_stricmp (text, "cosine"))	return STAGE_COSINE;

	// default
	return STAGE_STATIC;
}

// ==========================================================================

/*
shadername
{
	safe/noFlush
	noMark
	noShadow
	noShell
	subDivide <size>
	vertexWarp <speed> <distance> <smoothness>
	{
		map/cubeMap <texture/baseName>
		scroll <xtype> <xspeed> <ytype> <yspeed>
		blendFunc <source> <dest>
		alphaShift <speed> <min> <max>
		anim <delay> <tex1> <tex2> <tex3> ... end
		envMap
		alphaMask
		noLightMap
		noGamma
		noIntens
		noMipmap
		noPicMip
	}
}
*/

static inline void rs_stage_map (shaderStage_t *stage, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	Q_strncpyz (stage->name, *token, sizeof (stage->name));
}

static inline void rs_stage_cubemap (shaderStage_t *stage, char **token) {
	if (glConfig.extArbTexCubeMap) {
		stage->cubeMap = qTrue;
		stage->flags |= IT_CUBEMAP;
	}
	else
		stage->envMap = qTrue;

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	Q_strncpyz (stage->name, *token, sizeof (stage->name));
}

static inline void rs_stage_scroll (shaderStage_t *stage, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scroll.typeX = RS_FuncName (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scroll.speedX = atof (*token);
	
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scroll.typeY = RS_FuncName (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scroll.speedY = atof (*token);
}

static inline void rs_stage_blendfunc (shaderStage_t *stage, char **token) {
	stage->blendFunc.blend = qTrue;

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);

	if (!Q_stricmp (*token, "add")) {
		stage->blendFunc.source = GL_ONE;
		stage->blendFunc.dest = GL_ONE;
	}
	else if (!Q_stricmp (*token, "blend")) {
		stage->blendFunc.source = GL_SRC_ALPHA;
		stage->blendFunc.dest = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp (*token, "filter")) {
		stage->blendFunc.source = GL_ZERO;
		stage->blendFunc.dest = GL_SRC_COLOR;
	}
	else {
		stage->blendFunc.source = RS_BlendID (*token);

		*token = strtok (NULL, SHADER_TOKDELIMINATORS);
		stage->blendFunc.dest = RS_BlendID (*token);
	}
}

static inline void rs_stage_alphashift (shaderStage_t *stage, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->alphaShift.speed = (float)atof (*token);

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->alphaShift.min = (float)atof (*token);

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->alphaShift.max = (float)atof (*token);
}

static inline void rs_stage_anim (shaderStage_t *stage, char **token) {
	shaderAnimStage_t	*anim = (shaderAnimStage_t *) Mem_PoolAlloc (sizeof (shaderAnimStage_t), MEMPOOL_SHADERSYS, 0);

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->animDelay = (float)atof (*token);

	stage->animStage = anim;
	stage->animLast = anim;

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	
	while (Q_stricmp (*token, "end")) {
		stage->animCount++;

		Q_strncpyz (anim->name, *token, sizeof (anim->name));

		anim->texture = NULL;

		*token = strtok (NULL, SHADER_TOKDELIMINATORS);

		if (!Q_stricmp (*token, "end")) {
			anim->next = NULL;
			break;
		}

		anim->next = (shaderAnimStage_t *) Mem_PoolAlloc (sizeof (shaderAnimStage_t), MEMPOOL_SHADERSYS, 0);
		anim = anim->next;
	}
}

static inline void rs_stage_envmap (shaderStage_t *stage, char **token) {
	stage->envMap = qTrue;
}

static inline void rs_stage_nolightmap (shaderStage_t *stage, char **token) {
	stage->lightMap = qFalse;
}

static inline void rs_stage_alphamask (shaderStage_t *stage, char **token) {
	stage->alphaMask = qTrue;
}

static inline void rs_stage_rotate (shaderStage_t *stage, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->rotateSpeed = (float) atof (*token);
}

static inline void rs_stage_scale (shaderStage_t *stage, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scale.typeX = RS_FuncName (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scale.scaleX = atof (*token);
	
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scale.typeY = RS_FuncName (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	stage->scale.scaleY = atof (*token);
}

static inline void rs_stage_nogamma (shaderStage_t *stage, char **token) {
	if (!(stage->flags & IF_NOGAMMA))
		stage->flags |= IF_NOGAMMA;
}

static inline void rs_stage_nointens (shaderStage_t *stage, char **token) {
	if (!(stage->flags & IF_NOINTENS))
		stage->flags |= IF_NOINTENS;
}

static inline void rs_stage_nomipmap (shaderStage_t *stage, char **token) {
	if (!(stage->flags & IF_NOMIPMAP))
		stage->flags |= IF_NOMIPMAP;
}

static inline void rs_stage_nopicmip (shaderStage_t *stage, char **token) {
	if (!(stage->flags & IF_NOPICMIP))
		stage->flags |= IF_NOPICMIP;
}

static shaderStageKey_t shaderStageKeys[] = {
	{	"map",			&rs_stage_map			},
	{	"cubemap",		&rs_stage_cubemap		},
	{	"scroll",		&rs_stage_scroll		},
	{	"blendfunc",	&rs_stage_blendfunc		},
	{	"alphashift",	&rs_stage_alphashift	},
	{	"anim",			&rs_stage_anim			},
	{	"envmap",		&rs_stage_envmap		},
	{	"nolightmap",	&rs_stage_nolightmap	},
	{	"alphamask",	&rs_stage_alphamask		},
	{	"rotate",		&rs_stage_rotate		},
	{	"scale",		&rs_stage_scale			},

	{	"nogamma",		&rs_stage_nogamma		},
	{	"nointens",		&rs_stage_nointens		},
	{	"nomipmap",		&rs_stage_nomipmap		},
	{	"nopicmip",		&rs_stage_nopicmip		},

	{	NULL,			NULL					}
};

static int shader_NumStageKeys = sizeof (shaderStageKeys) / sizeof (shaderStageKeys[0]) - 1;

/*
=============================================================================

	SHADER PROCESSING

=============================================================================
*/

/*
==================
RS_ResetShader
==================
*/
static void RS_ResetShader (shader_t *shader) {
	shaderStage_t		*stage, *nextStage;
	shaderAnimStage_t	*anim, *nextAnim;

	shader->name[0] = 0;
	
	for (stage=shader->stage ; stage ; stage=nextStage) {
		nextStage = stage->next;

		if (stage->animCount) {
			for (anim=stage->animStage ; anim ; anim=nextAnim) {
				nextAnim = anim->next;

				Mem_Free (anim);
			}
		}

		Mem_Free (stage);
	}

	shader->subDivide = 0;
	shader->warpDist = 0;
	shader->warpSmooth = 0;
	shader->warpSpeed = 0;
	shader->noMark = qFalse;
	shader->noShadow = qFalse;
	shader->noShell = qFalse;
	shader->noFlush = qFalse;
	shader->ready = qFalse;
	shader->stage = NULL;
}


/*
==================
RS_NewShader
==================
*/
static shader_t *RS_NewShader (char *name, char *fileName) {
	shader_t	*shader;

	shader = Mem_PoolAlloc (sizeof (shader_t), MEMPOOL_SHADERSYS, 0);

	Q_strncpyz (shader->name, Q_FixPathName (name, qTrue), sizeof (shader->name));
	Q_strncpyz (shader->fileName, Q_FixPathName (fileName, qTrue), sizeof (shader->fileName));

	shader->stage = NULL;
	shader->next = NULL;
	shader->noFlush = qFalse;
	shader->noMark = qFalse;
	shader->noShadow = qFalse;
	shader->noShell = qFalse;
	shader->subDivide = 0;
	shader->warpDist = 0.0f;
	shader->warpSmooth = 0.0f;
	shader->ready = qFalse;

	// link it in shader list
	shader->next = loadedShaders;
	loadedShaders = shader;

	// link it into the hash list
	shader->hashValue = CalcHash (shader->name, MAX_SHADER_HASH);

	shader->hashNext = shHashTree[shader->hashValue];
	shHashTree[shader->hashValue] = shader;

	return shader;
}


/*
==================
RS_FreeShader
==================
*/
static void RS_FreeShader (shader_t *shader) {
	shader_t	*temp, **prev;

	if (!shader)
		return;

	// de-link it from shader list
	prev = &loadedShaders;
	for ( ; ; ) {
		temp = *prev;
		if (!temp)
			break;

		if (temp == shader) {
			*prev = temp->next;
			break;
		}
		prev = &temp->next;
	}

	// de-link it from hash list
	prev = &shHashTree[shader->hashValue];
	for ( ; ; ) {
		temp = *prev;
		if (!temp)
			break;

		if (temp == shader) {
			*prev = temp->hashNext;
			RS_ResetShader (shader);
			Mem_Free (shader);
			break;
		}
		prev = &temp->hashNext;
	}
}

// ==========================================================================

static inline void shader_shader_noflush (shader_t *shader, char **token) {
	*token = *token;
	shader->noFlush = qTrue;
}

static inline void shader_shader_subdivide (shader_t *shader, char **token) {
	int divsize, p2divsize;

	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	divsize = atoi (*token);
 
	// cap max & min subdivide sizes
	divsize = clamp (divsize, 8, 128);

	// find the next smallest valid ^2 size, if not already one
	for (p2divsize=2 ; p2divsize<=divsize ; p2divsize <<= 1) ;

	p2divsize >>= 1;

	shader->subDivide = (char)p2divsize;
}

static inline void shader_shader_vertexwarp (shader_t *shader, char **token) {
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	shader->warpSpeed = atof (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	shader->warpDist = atof (*token);
	*token = strtok (NULL, SHADER_TOKDELIMINATORS);
	shader->warpSmooth = clamp (atof (*token), 0.001f, 1.0f);
}

static inline void shader_shader_nomark (shader_t *shader, char **token) {
	*token = *token;
	shader->noMark = qTrue;
}

static inline void shader_shader_noshadow (shader_t *shader, char **token) {
	*token = *token;
	shader->noShadow = qTrue;
}

static inline void shader_shader_noshell (shader_t *shader, char **token) {
	*token = *token;
	shader->noShell = qTrue;
}

static shaderBaseKey_t shaderBaseKeys[] = {
	{	"subdivide",	&shader_shader_subdivide	},
	{	"vertexwarp",	&shader_shader_vertexwarp	},

	{	"nomark",		&shader_shader_nomark		},
	{	"noshadow",		&shader_shader_noshadow		},
	{	"noshell",		&shader_shader_noshell		},
	{	"noflush",		&shader_shader_noflush		},

	{	"safe",			&shader_shader_noflush		}, // let's not break old shaders

	{	NULL,			NULL						}
};

static int shader_NumBaseKeys = sizeof (shaderBaseKeys) / sizeof (shaderBaseKeys[0]) - 1;

/*
=============================================================================

	SHADER LOADING

=============================================================================
*/

/*
==================
RS_FindShader
==================
*/
shader_t *RS_FindShader (char *name) {
	shader_t	*shader;
	char		tempName[MAX_QPATH];
	uLong		hashValue;

	if (!name)
		return NULL;

	Q_strncpyz (tempName, Q_FixPathName (name, qTrue), sizeof (tempName));

	hashValue = CalcHash (tempName, MAX_SHADER_HASH);

	// look for it
	for (shader=shHashTree[hashValue] ; shader ; shader=shader->hashNext) {
		if (!Q_stricmp (shader->name, tempName)) {
			if (shader->stage)
			   return shader;
			else
			   return NULL;
		}
	}

	return NULL;
}


/*
==================
RS_ReadyShader
==================
*/
static void RS_ReadyShader (shader_t *shader) {
	shaderStage_t		*stage;
	shaderAnimStage_t	*anim;
	int					flags;

	if (!shader || shader->ready)
		return;

	stage = shader->stage;
	while (stage != NULL) {
		flags = stage->flags;
		if (shader->noFlush) {
			if (!(flags & IF_NOFLUSH))
				flags |= IF_NOFLUSH;
		}
		else {
			if (!(flags & IT_TEXTURE))
				flags |= IT_TEXTURE;
		}

		anim = stage->animStage;
		while (anim != NULL) {
			if (!Q_stricmp (anim->name, "$whitetexture"))
				anim->texture = r_WhiteTexture;
			else if (!Q_stricmp (anim->name, "$blacktexture"))
				anim->texture = r_BlackTexture;
			else
				anim->texture = Img_RegisterImage (anim->name, flags);

			if (!anim->texture) {
				if (anim->name[0])
					Com_Printf (0, S_COLOR_YELLOW "WARNING: Could not find/load shader pass anim image \"%s\"\n", anim->name);
				else if (shader->name[0])
					Com_Printf (0, S_COLOR_YELLOW "WARNING: NULL anim name in \"%s\" shader\n", shader->name);
				anim->texture = NULL;
			}

			anim = anim->next;
		}

		if (stage->name[0]) {
			if (!Q_stricmp (stage->name, "$lightmap")) {
				stage->texture = r_WhiteTexture;
				stage->lightMapOnly = qTrue;
				stage->lightMap = qFalse;
			}
			else if (!Q_stricmp (stage->name, "$whitetexture"))
				stage->texture = r_WhiteTexture;
			else if (!Q_stricmp (stage->name, "$blacktexture"))
				stage->texture = r_BlackTexture;
			else {
				stage->texture = Img_RegisterImage (stage->name, flags);
				if (!stage->texture) {
					Com_Printf (0, S_COLOR_YELLOW "WARNING: Could not find/load shader pass image \"%s\"\n", stage->name);
					stage->texture = NULL;
				}
			}
		}
		else {
			if (shader->name[0])
				Com_Printf (0, S_COLOR_YELLOW "WARNING: NULL stage name in \"%s\" shader\n", shader->name);
			stage->texture = NULL;
		}

		stage = stage->next;
	}

	shader->ready = qTrue;
}


/*
==================
RS_LoadShader
==================
*/
void RS_LoadShader (char *name, qBool recursed, qBool silent) {
	qBool			inShader = qFalse, instage = qFalse;
	char			ignored = 0;
	char			*token, *fbuf, *buf;
	shader_t		*shader;
	shaderStage_t	*stage;
	uInt			fileLen, i;

	if (!name[0])
		return;
	if ((name[12] == '/') && !recursed)
		return;
	if (Q_strlen (name) < 13)
		return;
	if (!!strcmp (name + Q_strlen (name) - 4, ".ers"))
		return;

	shader = NULL;

	fileLen = FS_LoadFile (name, (void **)&fbuf);
	if (!fbuf || (fileLen <= 0)) {
		if (!silent)
			Com_Printf (0, "..." S_COLOR_YELLOW "can't load '%s'\n", name);
		return;
	}

	if (!silent)
		Com_Printf (0, "...loading '%s'\n", name);

	buf = (char *)Mem_PoolAlloc (fileLen+1, MEMPOOL_SHADERSYS, 0);
	memcpy (buf, fbuf, fileLen);
	buf[fileLen] = 0;

	FS_FreeFile (fbuf);

	token = strtok (buf, SHADER_TOKDELIMINATORS);

	while (token)  {
		if (!Q_stricmp (token, "/*") || !Q_stricmp (token, "["))
			ignored++;
		else if (!Q_stricmp (token, "*/") || !Q_stricmp (token, "]"))
			ignored--;

		if (!inShader && !ignored) {
			if (!Q_stricmp (token, "{")) {
				inShader = qTrue;
			}
			else {
				shader = RS_FindShader (token);

				if (shader)
					RS_FreeShader (shader);

				shader = RS_NewShader (token, name);
			}
		}
		else if (inShader && !ignored) {
			if (!Q_stricmp (token, "}")) {
				if (instage)
					instage = qFalse;
				else
					inShader = qFalse;
			}
			else if (!Q_stricmp (token, "{")) {
				if (!instage) {
					instage = qTrue;
					stage = RS_NewStage (shader);
				}
			}
			else {
				if (instage && !ignored) {
					for (i=0 ; i<shader_NumStageKeys ; i++) {
						if (!Q_stricmp (shaderStageKeys[i].stage, token)) {
							shaderStageKeys[i].func (stage, &token);
							break;
						}
					}
				}
				else {
					for (i=0 ; i<shader_NumBaseKeys ; i++) {
						if (!Q_stricmp (shaderBaseKeys[i].shader, token)) {
							shaderBaseKeys[i].func (shader, &token);
							break;
						}
					}
				}
			}
		}

		token = strtok (NULL, SHADER_TOKDELIMINATORS);
	}

	Mem_Free (buf);
}


/*
==================
RS_ScanPathForShaders
==================
*/
void RS_ScanPathForShaders (qBool silent) {
	char			shader[MAX_OSPATH];
	char			dirstring[1024], *c;
	int				handle;
	struct			_finddata_t fileinfo;
	char			*find = NULL;
	char			*dir = FS_Gamedir ();

	//
	// load pack shaders
	//
	find = FS_FindFirst ("scripts/");
	while (find) {
		if (find != NULL) {
			RS_LoadShader (find, qFalse, silent);
			find = FS_FindNext ("scripts/");
		}
	}

	//
	// load baseq2 shaders
	//
	if (strcmp (dir, "./"BASE_MODDIRNAME)) {
		Q_snprintfz (dirstring, sizeof (dirstring), "baseq2/scripts/*.ers");
		handle = _findfirst (dirstring, &fileinfo);

		if (handle != -1) {
			do {
				if (fileinfo.name[0] == '.')
					continue;

				c = Com_SkipPath (fileinfo.name);
				Q_snprintfz (shader, MAX_OSPATH, "scripts/%s", c);

				RS_LoadShader (shader, qFalse, silent);
			} while (_findnext (handle, &fileinfo) != -1);

			_findclose (handle);
		}
	}

	//
	// load moddir shaders
	//
	Q_snprintfz (dirstring, sizeof (dirstring), "%s/scripts/*.ers", dir);
	handle = _findfirst (dirstring, &fileinfo);

	if (handle != -1) {
		do {
			if (fileinfo.name[0] == '.')
				continue;

			c = Com_SkipPath (fileinfo.name);
			Q_snprintfz (shader, MAX_OSPATH, "scripts/%s", c);

			RS_LoadShader (shader, qFalse, silent);
		} while (_findnext (handle, &fileinfo) != -1);

		_findclose (handle);
	}
}

/*
=============================================================================

	SHADER PASS PROCESSING FUNCTIONS

=============================================================================
*/

/*
==================
RS_Animate
==================
*/
image_t *RS_Animate (shaderStage_t *stage) {
	shaderAnimStage_t	*anim = stage->animLast;

	while (stage->animTimeLast < glState.realTime) {
		anim = anim->next;
		if (!anim)
			anim = stage->animStage;
		stage->animTimeLast += stage->animDelay;
	}

	stage->animLast = anim;

	return anim->texture;
}


/*
==================
RS_AlphaShift
==================
*/
void RS_AlphaShift (shaderStage_t *stage, float *alpha) {
	if (stage->alphaShift.min || stage->alphaShift.speed) {
		if (!stage->alphaShift.speed && (stage->alphaShift.min > 0)) {
			*alpha = stage->alphaShift.min;
		}
		else if (stage->alphaShift.speed) {
			*alpha = sin (glState.realTime * stage->alphaShift.speed);
			*alpha = clamp ((*alpha + 1) * 0.5f, stage->alphaShift.min, stage->alphaShift.max);
		}
	}
	else
		*alpha = 1.0f;
}


/*
==================
RS_BlendFunc
==================
*/
void RS_BlendFunc (shaderStage_t *stage, qBool force, qBool blend) {
	if (stage && (stage->blendFunc.blend)) {
		GL_BlendFunc (stage->blendFunc.source, stage->blendFunc.dest);
		if (blend)
			qglEnable (GL_BLEND);
	}
	else if (force) {
		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (blend)
			qglEnable (GL_BLEND);
	}
	else
		qglDisable (GL_BLEND);
}


/*
==================
RS_Scroll
==================
*/
void RS_Scroll (shaderStage_t *stage, vec2_t scrollCoords) {
	if (stage->scroll.speedX) {
		switch (stage->scroll.typeX) {
		case STAGE_STATIC:
			// static
			scrollCoords[0] = glState.realTime * stage->scroll.speedX;
			break;
		case STAGE_SINE:
			// sine
			scrollCoords[0] = sin (glState.realTime * stage->scroll.speedX);
			break;
		case STAGE_COSINE:
			// cosine
			scrollCoords[0] = cos (glState.realTime * stage->scroll.speedX);
			break;
		}
	}
	else
		scrollCoords[0] = 0;

	if (stage->scroll.speedY) {
		switch (stage->scroll.typeY) {
		case STAGE_STATIC:
			// static
			scrollCoords[1] = glState.realTime * stage->scroll.speedY;
			break;
		case STAGE_SINE:
			// sine
			scrollCoords[1] = sin (glState.realTime * stage->scroll.speedY);
			break;
		case STAGE_COSINE:
			// cosine
			scrollCoords[1] = cos (glState.realTime * stage->scroll.speedY);
			break;
		}
	}
	else
		scrollCoords[1] = 0;
}


/*
==================
RS_SetEnvmap

Now featuring Quake3-like envmapping mixed in ala Vic!
==================
*/
void RS_SetEnvmap (vec3_t origin, vec3_t axis[3], vec3_t vertOrg, vec3_t normal, vec2_t coords) {
	float	depth;
	vec3_t	n, transform, projection;
	vec3_t	inverseAxis[3];
	qBool	isWorldModel;
	float	*mat = r_WorldViewMatrix;
	vec3_t	tcv;

	if (VectorCompare (vec3Origin, origin)) {
		isWorldModel = qTrue;
		VectorSubtract (vec3Origin, r_RefDef.viewOrigin, transform);
	}
	else {
		isWorldModel = qFalse;
		VectorSubtract (origin, r_RefDef.viewOrigin, transform);
		Matrix_Transpose (axis, inverseAxis);
	}

	VectorAdd (vertOrg, transform, projection);
	VectorNormalize (projection, projection);

	// project vector
	if (isWorldModel)
		VectorCopy (normal, n);
	else
		Matrix_TransformVector (inverseAxis, normal, n);

	depth = -2.0f * DotProduct (n, projection);
	VectorMA (projection, depth, n, projection);
	depth = Q_FastSqrt (DotProduct (projection, projection) * 4);

	coords[0] = -((projection[1] * depth) + 0.5f);
	coords[1] =  ((projection[2] * depth) + 0.5f);

	// partially include the view vectors
	tcv[0] = (vertOrg[0] * mat[0 ]) + (vertOrg[1] * mat[4 ]) + (vertOrg[2] * mat[8 ]) + mat[12];
	tcv[1] = (vertOrg[0] * mat[1 ]) + (vertOrg[1] * mat[5 ]) + (vertOrg[2] * mat[9 ]) + mat[13];
	tcv[2] = (vertOrg[0] * mat[2 ]) + (vertOrg[1] * mat[6 ]) + (vertOrg[2] * mat[10]) + mat[14];

	VectorNormalize (tcv, tcv);

	coords[0] += tcv[0] * 0.25;
	coords[1] += tcv[1] * 0.25;
}


/*
==================
RS_SetTexcoords
==================
*/
static inline void RS_RotateST (mBspSurface_t *surf, float *os, float *ot, float degrees) {
	float	cost = cos (degrees);
	float	sint = sin (degrees);

	if (surf) {
		*os = cost * (*os - surf->c_s) + sint * (surf->c_t - *ot) + surf->c_s;
		*ot = cost * (*ot - surf->c_t) + sint * (*os - surf->c_s) + surf->c_t;
	}
	else {
		*os = cost * (*os - 0.5) + sint * (0.5 - *ot) + 0.5;
		*ot = cost * (*ot - 0.5) + sint * (*os - 0.5) + 0.5;
	}
}
void RS_SetTexcoords (shaderStage_t *stage, mBspSurface_t *surf, vec2_t coords) {
	// scale
	if (stage->scale.scaleX) {
		switch (stage->scale.typeX) {
		case STAGE_STATIC:
			// static
			coords[0] *= stage->scale.scaleX;
			break;
		case STAGE_SINE:
			// sine
			coords[0] *= stage->scale.scaleX * sin (glState.realTime * 0.05);
			break;
		case STAGE_COSINE:
			// cosine
			coords[0] *= stage->scale.scaleX * cos (glState.realTime * 0.05);
			break;
		}
	}

	if (stage->scale.scaleY) {
		switch (stage->scale.typeY) {
		case STAGE_STATIC:
			// static
			coords[1] *= stage->scale.scaleY;
			break;
		case STAGE_SINE:
			// sine
			coords[1] *= stage->scale.scaleY * sin (glState.realTime * 0.05);
			break;
		case STAGE_COSINE:
			// cosine
			coords[1] *= stage->scale.scaleY * cos (glState.realTime * 0.05);
			break;
		}
	}

	// rotation
	if (stage->rotateSpeed)
		RS_RotateST (surf, &coords[0], &coords[1], -stage->rotateSpeed * glState.realTime * 0.0087266388888888888888888888888889);
}


/*
==================
RS_SetTexcoords2D
==================
*/
static inline void RS_RotateST2 (float *os, float *ot, float degrees) {
	float	cost = cos (degrees);
	float	sint = sin (degrees);

	*os = cost * (*os - 0.5) + sint * (0.5 - *ot) + 0.5;
	*ot = cost * (*ot - 0.5) + sint * (*os - 0.5) + 0.5;
}
void RS_SetTexcoords2D (shaderStage_t *stage, vec2_t coords) {
	// scale
	if (stage->scale.scaleX) {
		switch (stage->scale.typeX) {
		case STAGE_STATIC:
			// static
			coords[0] *= stage->scale.scaleX;
			break;
		case STAGE_SINE:
			// sine
			coords[0] *= stage->scale.scaleX * sin (glState.realTime * 0.05);
			break;
		case STAGE_COSINE:
			// cosine
			coords[0] *= stage->scale.scaleX * cos (glState.realTime * 0.05);
			break;
		}
	}

	if (stage->scale.scaleY) {
		switch (stage->scale.typeY) {
		case STAGE_STATIC:
			// static
			coords[1] *= stage->scale.scaleY;
			break;
		case STAGE_SINE:
			// sine
			coords[1] *= stage->scale.scaleY * sin (glState.realTime * 0.05);
			break;
		case STAGE_COSINE:
			// cosine
			coords[1] *= stage->scale.scaleY * cos (glState.realTime * 0.05);
			break;
		}
	}

	// rotate
	if (stage->rotateSpeed)
		RS_RotateST2 (&coords[0], &coords[1], -stage->rotateSpeed * glState.realTime * 0.0087266388888888888888888888888889);
}

/*
=============================================================================

	MISC

=============================================================================
*/

/*
==================
RS_FreeUnmarked
==================
*/
void RS_FreeUnmarked (void) {
	shader_t	*shader, *next;

	for (shader=loadedShaders ; shader ; shader=next) {
		next = shader->next;

		if (!shader->noFlush)
			RS_FreeShader (shader);
	}
}


/*
==================
RS_RegisterShader
==================
*/
shader_t *RS_RegisterShader (char *name) {
	shader_t	*shader;

	shader = RS_FindShader (name);
	RS_ReadyShader (shader);

	return shader;
}


/*
==================
RS_EndRegistration
==================
*/
void RS_EndRegistration (void) {
	shader_t			*shader;
	shaderStage_t		*stage;
	shaderAnimStage_t	*anim;
	int					flags;

	for (shader=loadedShaders ; shader ; shader=shader->next) {
		for (stage=shader->stage ; stage ; stage=stage->next) {
			flags = stage->flags;
			if (shader->noFlush) {
				if (!(flags & IF_NOFLUSH))
					flags |= IF_NOFLUSH;
			}
			else {
				if (!(flags & IT_TEXTURE))
					flags |= IT_TEXTURE;
			}

			anim = stage->animStage;
			while (anim != NULL) {
				if (!Q_stricmp (anim->name, "$whitetexture"))
					anim->texture = r_WhiteTexture;
				else if (!Q_stricmp (anim->name, "$blacktexture"))
					anim->texture = r_BlackTexture;
				else
					anim->texture = Img_RegisterImage (anim->name, flags);

				if (!anim->texture) {
					if (anim->name[0])
						Com_Printf (0, S_COLOR_YELLOW "WARNING: Could not find/load shader pass anim image \"%s\"\n", anim->name);
					else if (shader->name[0])
						Com_Printf (0, S_COLOR_YELLOW "WARNING: NULL anim name in \"%s\" shader\n", shader->name);
					anim->texture = NULL;
				}
				anim = anim->next;
			}

			if (stage->texture) {
				if (!Q_stricmp (stage->name, "$lightmap")) {
					stage->texture = r_WhiteTexture;
					stage->lightMapOnly = qTrue;
					stage->lightMap = qFalse;
				}
				else if (!Q_stricmp (stage->name, "$whitetexture"))
					stage->texture = r_WhiteTexture;
				else if (!Q_stricmp (stage->name, "$blacktexture"))
					stage->texture = r_BlackTexture;
				else
					stage->texture = Img_RegisterImage (stage->name, flags);

				if (!stage->texture) {
					if (stage->name[0])
						Com_Printf (0, S_COLOR_YELLOW "WARNING: Could not find/load shader pass image \"%s\"\n", stage->name);
					else if (shader->name[0])
						Com_Printf (0, S_COLOR_YELLOW "WARNING: NULL stage name in \"%s\" shader\n", shader->name);
					stage->texture = NULL;
				}
			}
		}
	}
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
==================
RS_ShaderList_f
==================
*/
static void RS_ShaderList_f (void) {
	shader_t		*shader;
	shaderStage_t	*stage;
	char			*wildCard;
	int				passes;
	int				totPasses;
	int				totShaders;
	int				i;

	if (Cmd_Argc () == 2)
		wildCard = Q_VarArgs ("*%s*", Cmd_Argv (1));
	else
		wildCard = "*";

	Com_Printf (0, "------------ Loaded Shaders ------------\n");
	Com_Printf (0, "PASSES  NAME\n");
	Com_Printf (0, "------  --------------------------------\n");

	for (i=0, totShaders=0, totPasses=0, shader=loadedShaders ; shader ; shader=shader->next, i++) {
		if (!shader->name[0])
			continue;
		if (!Q_WildCmp (wildCard, shader->name, 1))
			continue;

		for (stage=shader->stage, passes=0 ; stage != NULL ; stage=stage->next) {
			if (shader->stage->name[0])
				passes++;
		}
		totPasses += passes;

		Com_Printf (0, " %-3d    %s\n", passes, shader->name);
		totShaders++;
	}

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "Total matching shaders/passes: %d/%d\n", totShaders, totPasses);
	Com_Printf (0, "----------------------------------------\n");
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
==================
RS_Init
==================
*/
void RS_Init (void) {
	memset (shHashTree, 0, sizeof (shHashTree));

	r_shaders				= Cvar_Get ("r_shaders",			"1",		CVAR_ARCHIVE);

	Cmd_AddCommand ("shaderlist",	RS_ShaderList_f,		"Prints to the console a list of loaded shaders");

	Com_Printf (0, "\n-------- Shader Initialization ---------\n");

	// load shaders
	RS_ScanPathForShaders (qFalse);

	// check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_SHADERSYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
RS_Shutdown
==================
*/
void RS_Shutdown (qBool full) {
	shader_t	*shader, *next;

	if (!full) {
		// remove commands
		Cmd_RemoveCommand ("shaderlist");
	}

	for (shader=loadedShaders ; shader ; shader=next) {
		next = shader->next;
		RS_ResetShader (shader);
	}

	Mem_FreePool (MEMPOOL_SHADERSYS);
	loadedShaders = NULL;
}
