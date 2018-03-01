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
// rf_material.c
// Material loading, caching, and some primitive surface rendering
//

#include "rf_local.h"

#define MAX_MATERIAL_HASH			(MAX_MATERIALS/4)

static material_t		r_materialList[MAX_MATERIALS];
static material_t		*r_materialHashTree[MAX_MATERIAL_HASH];
static uint32			r_numMaterials;

enum {
	MATCK_NOLIGHTMAP,

	MATCK_MAX
};

static qBool			r_materialChecks[MATCK_MAX];

static uint32			r_numCurrPasses;
static matPass_t		r_currPasses[MAX_MATERIAL_PASSES];
static vertDeform_t		r_currDeforms[MAX_MATERIAL_DEFORMVS];
static tcMod_t			r_currTcMods[MAX_MATERIAL_PASSES][MAX_MATERIAL_TCMODS];

static uint32			r_numMaterialErrors;
static uint32			r_numMaterialWarnings;

typedef struct matKey_s {
	char		*keyWord;
	qBool		(*func)(material_t *mat, matPass_t *pass, parse_t *ps, char *fileName);
} matKey_t;

material_t	*r_cinMaterial;
material_t	*r_noMaterial;
material_t	*r_noMaterialLightmap;
material_t	*r_noMaterialSky;
material_t	*r_whiteMaterial;
material_t	*r_blackMaterial;

/*
=============================================================================

	MATERIAL PARSING

=============================================================================
*/

/*
==================
Mat_PrintPos
==================
*/
static void Mat_PrintPos (comPrint_t flags, material_t *mat, int passNum, parse_t *ps, char *fileName)
{
	uint32		line, col;

	// Increment tallies
	if (flags & PRNT_ERROR)
		r_numMaterialErrors++;
	else if (flags & PRNT_WARNING)
		r_numMaterialWarnings++;

	if (ps) {
		// Print the position
		PS_GetPosition (ps, &line, &col);
		if (passNum)
			Com_Printf (flags, "%s(line #%i col#%i): Material '%s', pass #%i\n", fileName, line, col, mat->name, passNum+1);
		else
			Com_Printf (flags, "%s(line #%i col#%i): Material '%s'\n", fileName, line, col, mat->name);
		return;
	}

	// Print the position
	if (passNum) {
		Com_Printf (flags, "%s: Material '%s', pass #%i\n", fileName, mat->name, passNum+1);
		return;
	}

	Com_Printf (flags, "%s: Material '%s'\n", fileName, mat->name);
}


/*
==================
Mat_DevPrintPos
==================
*/
static void Mat_DevPrintPos (comPrint_t flags, material_t *mat, int passNum, parse_t *ps, char *fileName)
{
	if (!developer->intVal)
		return;

	Mat_PrintPos (flags, mat, passNum, ps, fileName);
}


/*
==================
Mat_Printf
==================
*/
static void Mat_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (flags & PRNT_ERROR)
		r_numMaterialErrors++;
	else if (flags & PRNT_WARNING)
		r_numMaterialWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
Mat_DevPrintf
==================
*/
static void Mat_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!developer->intVal)
		return;

	if (flags & PRNT_ERROR)
		r_numMaterialErrors++;
	else if (flags & PRNT_WARNING)
		r_numMaterialWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
Mat_ParseString
==================
*/
static qBool Mat_ParseString (parse_t *ps, char **target)
{
	char	*token;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &token) || token[0] == '}')
		return qFalse;

	*target = token;
	return qTrue;
}

#define Mat_ParseFloat(ps,target)	PS_ParseDataType((ps),0,PSDT_FLOAT,(target),1)
#define Mat_ParseInt(ps,target)		PS_ParseDataType((ps),0,PSDT_INTEGER,(target),1)

/*
==================
Mat_ParseWave
==================
*/
static qBool Mat_ParseWave (material_t *mat, parse_t *ps, char *fileName, materialFunc_t *func)
{
	char	*str;

	// Parse the wave type
	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: unable to parse wave type\n");
		return qFalse;
	}

	if (!strcmp (str, "sin"))
		func->type = MAT_FUNC_SIN;
	if (!strcmp (str, "triangle"))
		func->type = MAT_FUNC_TRIANGLE;
	if (!strcmp (str, "square"))
		func->type = MAT_FUNC_SQUARE;
	if (!strcmp (str, "sawtooth"))
		func->type = MAT_FUNC_SAWTOOTH;
	if (!strcmp (str, "inversesawtooth"))
		func->type = MAT_FUNC_INVERSESAWTOOTH;
	if (!strcmp (str, "noise"))
		func->type = MAT_FUNC_NOISE;

	// Parse args
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, func->args, 4)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid wave arguments!\n");
		return qFalse;
	}

	return qTrue;
}


/*
==================
Mat_ParseVector

FIXME: Deprecate for PS_ParseDataType
==================
*/
static qBool Mat_ParseVector (material_t *mat, parse_t *ps, char *fileName, float *vec, uint32 size)
{
	qBool	inBrackets;
	char	*str;
	uint32	i;

	if (!size) {
		return qTrue;
	}
	else if (size == 1) {
		if (!Mat_ParseFloat (ps, &vec[0])) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
			return qFalse;
		}
	}

	// Check brackets
	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: unable to parse vector!\n");
		return qFalse;
	}
	if (!strcmp (str, "(")) {
		inBrackets = qTrue;
		if (!Mat_ParseString (ps, &str)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
			return qFalse;
		}
	}
	else if (str[0] == '(') {
		inBrackets = qTrue;
		str = &str[1];
	}
	else {
		inBrackets = qFalse;
	}

	// Parse vector
	vec[0] = (float)atof (str);
	for (i=1 ; i<size-1 ; i++) {
		if (!Mat_ParseFloat (ps, &vec[i])) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
			return qFalse;
		}
	}

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters!\n");
		return qFalse;
	}

	// Final parameter (and possibly bracket)
	if (str[strlen(str)-1] == ')') {
		str[strlen(str)-1] = 0;
		vec[i] = (float)atof (str);
	}
	else {
		vec[i] = (float)atof (str);
		if (inBrackets) {
			if (!Mat_ParseString (ps, &str)) {
				Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
				Mat_Printf (PRNT_ERROR, "ERROR: missing vector end-bracket!\n");
				return qFalse;
			}
		}
	}

	return qTrue;
}


/*
==================
Mat_SkipBlock
==================
*/
static void Mat_SkipBlock (material_t *mat, parse_t *ps, char *fileName, char *token)
{
	int		braceCount;

	// Opening brace
	if (token[0] != '{') {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) || token[0] != '{') {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: expecting '{' to skip a block, got '%s'!\n", token);
			return;
		}
	}

	for (braceCount=1 ; braceCount>0 ; ) {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token))
			return;
		else if (token[0] == '{')
			braceCount++;
		else if (token[0] == '}')
			braceCount--;
	}
}

// ==========================================================================

static qBool MatPass_AnimFrequency (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	if (!Mat_ParseInt (ps, &pass->animFPS)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: animFrequency with no parameters\n");
		return qFalse;
	}

	pass->flags |= MAT_PASS_ANIMMAP;
	return qTrue;
}

static qBool MatPass_AnimMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	pass->animNumNames = 0;

	// Parse the framerate
	if (!Mat_ParseInt (ps, &pass->animFPS)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: missing animMap framerate!\n");
		return qFalse;
	}

	pass->flags |= MAT_PASS_ANIMMAP;
	pass->tcGen = TC_GEN_BASE;

	// Parse names
	for ( ; ; ) {
		if (!Mat_ParseString (ps, &str)) {
			if (!pass->animNumNames) {
				pass->animFPS = 0;
				pass->flags &= ~MAT_PASS_ANIMMAP;

				Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
				Mat_Printf (PRNT_ERROR, "ERROR: animMap with no images!\n");
				return qFalse;
			}
			break;
		}

		if (pass->animNumNames+1 > MAX_MATERIAL_ANIM_FRAMES) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
			PS_SkipLine (ps);
			return qTrue;
		}

		if (strlen(str)+1 >= MAX_QPATH) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
			str[MAX_QPATH-1] = '\0';
		}

		pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.matSysPool, 0);
	}

	return qTrue;
}

static qBool MatPass_MapExt (material_t *mat, matPass_t *pass, parse_t *ps, texFlags_t addTexFlags, qBool allowColorTokens, char *fileName)
{
	char	*str;

	// Check for too many frames
	if (pass->animNumNames+1 > MAX_MATERIAL_ANIM_FRAMES) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	// Parse the first parameter
	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: map with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "$lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->flags |= MAT_PASS_LIGHTMAP;
	}
	else {
		pass->tcGen = TC_GEN_BASE;
		texFlags_t tokenFlags = 0;

		if (allowColorTokens) {
			if (!strcmp(str, "$rgb")) {
				tokenFlags |= IF_NOALPHA;
			}
			else if (!strcmp(str, "$alpha")) {
				tokenFlags |= IF_NORGB;
			}
		}

		if (!strcmp(str, "$greyscale") || !strcmp(str, "$grayscale")) {
			tokenFlags |= IF_GREYSCALE;
		}

		if (tokenFlags != 0) {
			if (!Mat_ParseString(ps, &str)) {
				Mat_PrintPos(PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
				Mat_Printf(PRNT_ERROR, "ERROR: missing/invalid map parameters\n");
				return qFalse;
			}

			pass->animTexFlags[pass->animNumNames] |= tokenFlags;
		}
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animTexFlags[pass->animNumNames] |= addTexFlags;
	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.matSysPool, 0);
	return qTrue;
}

static qBool MatPass_Map (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return MatPass_MapExt (mat, pass, ps, 0, qTrue, fileName);
}

static qBool MatPass_AlphaMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return MatPass_MapExt(mat, pass, ps, IF_NORGB, qFalse, fileName);
}

static qBool MatPass_ClampMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return MatPass_MapExt (mat, pass, ps, IF_CLAMP_ALL, qTrue, fileName);
}

static qBool MatPass_GreyMap(material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return MatPass_MapExt(mat, pass, ps, IF_GREYSCALE, qTrue, fileName);
}

static qBool MatPass_RGBMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return MatPass_MapExt(mat, pass, ps, IF_NOALPHA, qFalse, fileName);
}

static qBool MatPass_CubeMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (pass->animNumNames+1 > MAX_MATERIAL_ANIM_FRAMES) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: cubeMap with no parameters\n");
		return qFalse;
	}

	if (ri.config.extTexCubeMap) {
		pass->flags |= MAT_PASS_CUBEMAP;
		pass->animTexFlags[pass->animNumNames] |= IT_CUBEMAP|IF_CLAMP_ALL;

		pass->tcGen = TC_GEN_REFLECTION;
	}
	else {
		pass->tcGen = TC_GEN_ENVIRONMENT;
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.matSysPool, 0);
	return qTrue;
}

static qBool MatPass_FragmentMap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	int		index;
	char	*str;

	// Check for the extension
	if (!ri.config.extFragmentProgram) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: fragmentMap used and extension not available\n");
		return qFalse;
	}

	// Parse the index
	if (!Mat_ParseInt (ps, &index)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: fragmentMap with no parameters\n");
		return qFalse;
	}

	if (index < 0) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: invalid fragmentMap index '%i'!\n", index);
		return qFalse;
	}
	if (index >= pass->animNumNames) {
		if (index+1 > MAX_MATERIAL_ANIM_FRAMES || index+1 > ri.config.maxTexUnits) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: too many fragmentMap images, ignoring\n");
			PS_SkipLine (ps);
			return qTrue;
		}
		pass->animNumNames = index+1;
	}

	// Parse image options
	pass->animTexFlags[index] = 0;
	for ( ; ; ) {
		if (!Mat_ParseString (ps, &str)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid fragmentMap parameters!\n");
			return qFalse;
		}

		if (!strcmp (str, "clamp"))
			pass->animTexFlags[index] |= IF_CLAMP_ALL;
		else if (!strcmp (str, "cubemap")) {
			if (!ri.config.extTexCubeMap) {
				Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
				Mat_Printf (PRNT_ERROR, "ERROR: cubeMap used and extension not available!\n");
				return qFalse;
			}
			pass->animTexFlags[index] |= IT_CUBEMAP|IF_CLAMP_ALL;
		}
		else if (!strcmp (str, "nocompress"))
			pass->animTexFlags[index] |= IF_NOCOMPRESS;
		else if (!strcmp (str, "nogamma"))
			pass->animTexFlags[index] |= IF_NOGAMMA;
		else if (!strcmp (str, "nointens"))
			pass->animTexFlags[index] |= IF_NOINTENS;
		else if (!strcmp (str, "nopicmip"))
			pass->animTexFlags[index] |= IF_NOPICMIP;
		else if (!strcmp (str, "linear"))
			pass->animTexFlags[index] |= IF_NOMIPMAP_LINEAR;
		else if (!strcmp (str, "nearest"))
			pass->animTexFlags[index] |= IF_NOMIPMAP_NEAREST;
		else
			break;
	}

	// Store the image name
	if (strlen(str)+1 >= MAX_QPATH) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[index] = Mem_PoolStrDup (str, ri.matSysPool, 0);
	return qTrue;
}

static qBool MatPass_FragmentProgram (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extension
	if (!ri.config.extFragmentProgram) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: fragmentProgram used and extension not available\n");
		return qFalse;
	}

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: fragmentProgram with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= MAT_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool MatPass_VertexProgram (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extension
	if (!ri.config.extVertexProgram) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: vertexProgram used and extension not available\n");
		return qFalse;
	}

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: vertexProgram with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	pass->flags |= MAT_PASS_VERTEXPROGRAM;
	return qTrue;
}

static qBool MatPass_Program (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extensions
	if (!ri.config.extFragmentProgram || !ri.config.extVertexProgram) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: program used and extensions not available\n");
		return qFalse;
	}

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: program with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= MAT_PASS_VERTEXPROGRAM|MAT_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool MatPass_RGBGen (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: rgbGen with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "identitylighting")) {
		pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
		return qTrue;
	}
	else if (!strcmp (str, "identity")) {
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "wave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		Vec3Set (pass->rgbGen.bArgs, 255, 255, 255);
		Vec3Set (pass->rgbGen.fArgs, 1.0f, 1.0f, 1.0f);
		return Mat_ParseWave (mat, ps, fileName, &pass->rgbGen.func);
	}
	else if (!strcmp (str, "colorwave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		if (!Mat_ParseVector (mat, ps, fileName, pass->rgbGen.fArgs, 3)
		|| !Mat_ParseWave (mat, ps, fileName, &pass->rgbGen.func))
			return qFalse;
		pass->rgbGen.bArgs[0] = FloatToByte (pass->rgbGen.fArgs[0]);
		pass->rgbGen.bArgs[1] = FloatToByte (pass->rgbGen.fArgs[1]);
		pass->rgbGen.bArgs[2] = FloatToByte (pass->rgbGen.fArgs[2]);
		return qTrue;
	}
	else if (!strcmp (str, "entity")) {
		pass->rgbGen.type = RGB_GEN_ENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusentity")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_ENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "vertex")) {
		pass->rgbGen.type = RGB_GEN_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusvertex")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusexactvertex")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_EXACT_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "lightingdiffuse")) {
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		return qTrue;
	}
	else if (!strcmp (str, "exactvertex")) {
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "const") || !strcmp (str, "constant")) {
		float	div;
		vec3_t	color;

		if (intensity->intVal > 0)
			div = 1.0f / (float)pow (2, intensity->intVal / 2.0f);
		else
			div = 1.0f;

		pass->rgbGen.type = RGB_GEN_CONST;
		if (!Mat_ParseVector (mat, ps, fileName, color, 3))
			return qFalse;
		ColorNormalizef (color, pass->rgbGen.fArgs);
		Vec3Scale (pass->rgbGen.fArgs, div, pass->rgbGen.fArgs);

		pass->rgbGen.bArgs[0] = FloatToByte (pass->rgbGen.fArgs[0]);
		pass->rgbGen.bArgs[1] = FloatToByte (pass->rgbGen.fArgs[1]);
		pass->rgbGen.bArgs[2] = FloatToByte (pass->rgbGen.fArgs[2]);
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid rgbGen value: '%s'\n", str);
	return qFalse;
}

static qBool MatPass_AlphaGen (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;
	float	f, g;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: alphaGen with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "identity")) {
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "const") || !strcmp (str, "constant")) {
		if (!Mat_ParseFloat (ps, &f)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_CONST;
		pass->alphaGen.args[0] = fabs (f);
		return qTrue;
	}
	else if (!strcmp (str, "wave")) {
		pass->alphaGen.type = ALPHA_GEN_WAVE;
		return Mat_ParseWave (mat, ps, fileName, &pass->alphaGen.func);
	}
	else if (!strcmp (str, "portal")) {
		if (!Mat_ParseFloat (ps, &f)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_PORTAL;
		pass->alphaGen.args[0] = fabs (f);
		if (!pass->alphaGen.args[0])
			pass->alphaGen.args[0] = 256;
		pass->alphaGen.args[0] = 1.0f / pass->alphaGen.args[0];
		return qTrue;
	}
	else if (!strcmp (str, "vertex")) {
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusvertex")) {
		pass->alphaGen.type = ALPHA_GEN_ONE_MINUS_VERTEX;
		return qTrue;
	}
	else if (!strcmp (str, "entity")) {
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "lightingspecular")) {
		pass->alphaGen.type = ALPHA_GEN_SPECULAR;
		return qTrue;
	}
	else if (!strcmp (str, "dot")) {
		if (!Mat_ParseFloat (ps, &f)
		|| !Mat_ParseFloat (ps, &g)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_DOT;
		pass->alphaGen.args[0] = fabs (f);
		pass->alphaGen.args[1] = fabs (g);
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusdot")) {
		if (!Mat_ParseFloat (ps, &f)
		|| !Mat_ParseFloat (ps, &g)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_ONE_MINUS_DOT;
		pass->alphaGen.args[0] = fabs (f);
		pass->alphaGen.args[1] = fabs (g);
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid alphaGen value: '%s'\n", str);
	return qFalse;
}

static qBool MatPass_AlphaFunc (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: alphaFunc with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "gt0")) {
		pass->alphaFunc = ALPHA_FUNC_GT0;
		return qTrue;
	}
	else if (!strcmp (str, "lt128")) {
		pass->alphaFunc = ALPHA_FUNC_LT128;
		return qTrue;
	}
	else if (!strcmp (str, "ge128")) {
		pass->alphaFunc = ALPHA_FUNC_GE128;
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid alphaFunc value: '%s'\n", str);
	return qFalse;
}

static qBool MatPass_BlendFunc (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: blendFunc with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "add")
	|| !strcmp (str, "additive")) {
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ONE;
	}
	else if (!strcmp (str, "blend")
	|| !strcmp (str, "default")) {
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!strcmp (str, "filter")
	|| !strcmp (str, "lightmap")) {
		pass->blendSource = GL_ZERO;
		pass->blendDest = GL_SRC_COLOR;
	}
	else {
		// Find the blend source
		if (!strcmp (str, "gl_zero"))
			pass->blendSource = GL_ZERO;
		else if (!strcmp (str, "gl_one"))
			pass->blendSource = GL_ONE;
		else if (!strcmp (str, "gl_dst_color"))
			pass->blendSource = GL_DST_COLOR;
		else if (!strcmp (str, "gl_one_minus_dst_color"))
			pass->blendSource = GL_ONE_MINUS_DST_COLOR;
		else if (!strcmp (str, "gl_src_alpha"))
			pass->blendSource = GL_SRC_ALPHA;
		else if (!strcmp (str, "gl_one_minus_src_alpha"))
			pass->blendSource = GL_ONE_MINUS_SRC_ALPHA;
		else if (!strcmp (str, "gl_dst_alpha"))
			pass->blendSource = GL_DST_ALPHA;
		else if (!strcmp (str, "gl_one_minus_dst_alpha"))
			pass->blendSource = GL_ONE_MINUS_DST_ALPHA;
		else if (!strcmp (str, "gl_src_alpha_saturate"))
			pass->blendSource = GL_SRC_ALPHA_SATURATE;
		else {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: has an invalid blend source: '%s', assuming GL_ONE\n", str);
			pass->blendSource = GL_ONE;
		}

		// Find the blend dest
		if (!Mat_ParseString (ps, &str)) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: missing blend dest, assuming GL_ONE\n");
			pass->blendDest = GL_ONE;
		}
		else if (!strcmp (str, "gl_zero"))
			pass->blendDest = GL_ZERO;
		else if (!strcmp (str, "gl_one"))
			pass->blendDest = GL_ONE;
		else if (!strcmp (str, "gl_src_color"))
			pass->blendDest = GL_SRC_COLOR;
		else if (!strcmp (str, "gl_one_minus_src_color"))
			pass->blendDest = GL_ONE_MINUS_SRC_COLOR;
		else if (!strcmp (str, "gl_src_alpha"))
			pass->blendDest = GL_SRC_ALPHA;
		else if (!strcmp (str, "gl_one_minus_src_alpha"))
			pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		else if (!strcmp (str, "gl_dst_alpha"))
			pass->blendDest = GL_DST_ALPHA;
		else if (!strcmp (str, "gl_one_minus_dst_alpha"))
			pass->blendDest = GL_ONE_MINUS_DST_ALPHA;
		else {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: has an invalid blend dest: '%s', assuming GL_ONE\n", str);
			pass->blendDest = GL_ONE;
		}
	}

	pass->flags |= MAT_PASS_BLEND;
	return qTrue;
}

static qBool MatPass_DepthFunc (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: depthFunc with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "equal")) {
		pass->depthFunc = GL_EQUAL;
		return qTrue;
	}
	else if (!strcmp (str, "lequal")) {
		pass->depthFunc = GL_LEQUAL;
		return qTrue;
	}
	else if (!strcmp (str, "gequal")) {
		pass->depthFunc = GL_GEQUAL;
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid depthFunc value: '%s'\n", str);
	return qFalse;
}

static qBool MatPass_TcGen (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: tcGen with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "base")) {
		pass->tcGen = TC_GEN_BASE;
		return qTrue;
	}
	else if (!strcmp (str, "lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		return qTrue;
	}
	else if (!strcmp (str, "environment")) {
		pass->tcGen = TC_GEN_ENVIRONMENT;
		return qTrue;
	}
	else if (!strcmp (str, "vector")) {
		pass->tcGen = TC_GEN_VECTOR;
		if (!Mat_ParseVector (mat, ps, fileName, pass->tcGenVec[0], 4)
		|| !Mat_ParseVector (mat, ps, fileName, pass->tcGenVec[1], 4))
			return qFalse;
		return qTrue;
	}
	else if (!strcmp (str, "reflection")) {
		pass->tcGen = TC_GEN_REFLECTION;
		return qTrue;
	}
	else if (!strcmp (str, "warp")) {
		pass->tcGen = TC_GEN_WARP;
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid tcGen value: '%s'\n", str);
	return qFalse;
}

static qBool MatPass_TcMod (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	tcMod_t	*tcMod;
	char	*str;
	float	val;
	int		i;

	if (pass->numTCMods == MAX_MATERIAL_TCMODS) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: too many tcMods, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	tcMod = &r_currTcMods[r_numCurrPasses][r_currPasses[r_numCurrPasses].numTCMods];

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: tcMod with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "rotate")) {
		if (!Mat_ParseFloat (ps, &val)) {
			Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid tcMod rotate parameters!\n");
			return qFalse;
		}

		tcMod->args[0] = -val / 360.0f;
		if (!tcMod->args[0]) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: invalid tcMod rotate: '%f', ignoring\n", val);
			PS_SkipLine (ps);
			return qTrue;
		}
		tcMod->type = TC_MOD_ROTATE;
	}
	else if (!strcmp (str, "scale")) {
		tcMod->type = TC_MOD_SCALE;
		if (!Mat_ParseVector (mat, ps, fileName, tcMod->args, 2))
			return qFalse;
	}
	else if (!strcmp (str, "scroll")) {
		tcMod->type = TC_MOD_SCROLL;
		if (!Mat_ParseVector (mat, ps, fileName, tcMod->args, 2))
			return qFalse;
	}
	else if (!strcmp (str, "stretch")) {
		materialFunc_t	func;

		if (!Mat_ParseWave (mat, ps, fileName, &func))
			return qFalse;

		tcMod->args[0] = func.type;
		for (i=1 ; i<5 ; i++) {
			tcMod->args[i] = func.args[i-1];
		}
		tcMod->type = TC_MOD_STRETCH;
	}
	else if (!strcmp (str, "transform")) {
		if (!Mat_ParseVector (mat, ps, fileName, tcMod->args, 6))
			return qFalse;
		tcMod->args[4] = tcMod->args[4] - floor(tcMod->args[4]);
		tcMod->args[5] = tcMod->args[5] - floor(tcMod->args[5]);
		tcMod->type = TC_MOD_TRANSFORM;
	}
	else if (!strcmp (str, "turb")) {
		if (!Mat_ParseVector (mat, ps, fileName, tcMod->args, 4))
			return qFalse;
		tcMod->type = TC_MOD_TURB;
	}
	else {
		Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: invalid tcMod value: '%s'\n", str);
		return qFalse;
	}

	r_currPasses[r_numCurrPasses].numTCMods++;
	return qTrue;
}

static qBool MatPass_MaskColor (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskRed = qTrue;
	pass->maskGreen = qTrue;
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool MatPass_MaskRed (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskRed = qTrue;
	return qTrue;
}

static qBool MatPass_MaskGreen (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskGreen = qTrue;
	return qTrue;
}

static qBool MatPass_MaskBlue (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool MatPass_MaskAlpha (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskAlpha = qTrue;
	return qTrue;
}

static qBool MatPass_NoGamma (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOGAMMA;
	return qTrue;
}

static qBool MatPass_NoIntens (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOINTENS;
	return qTrue;
}

static qBool MatPass_NoMipmap (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_DevPrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_DevPrintf (PRNT_WARNING, "WARNING: noMipMap with no type specified, assuming linear\n");
		return qTrue;
	}

	if (!strcmp (str, "linear")) {
		pass->passTexFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}
	else if (!strcmp (str, "nearest")) {
		pass->passTexFlags |= IF_NOMIPMAP_NEAREST;
		return qTrue;
	}

	Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_WARNING, "WARNING: invalid noMipMap value: '%s', assuming linear\n", str);
	pass->passTexFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool MatPass_NoPicmip (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool MatPass_NoCompress (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool MatPass_TcClamp (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_CLAMP_ALL;
	return qTrue;
}

static qBool MatPass_TcClampS (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_CLAMP_S;
	return qTrue;
}

static qBool MatPass_TcClampT (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_CLAMP_T;
	return qTrue;
}

static qBool MatPass_SizeBase (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->sizeBase = r_numCurrPasses;
	return qTrue;
}

static qBool MatPass_Detail (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= MAT_PASS_DETAIL;
	return qTrue;
}

static qBool MatPass_NotDetail (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= MAT_PASS_NOTDETAIL;
	return qTrue;
}

static qBool MatPass_DepthWrite (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= MAT_PASS_DEPTHWRITE;
	mat->flags |= MAT_DEPTHWRITE;
	return qTrue;
}

static qBool MatBase_Flare (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_FLARE;
	return qTrue;
}

static qBool MatPass_Emissive (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	return qTrue;
}

// ==========================================================================

static matKey_t r_materialPassKeys[] = {
	{ "animfrequency",		&MatPass_AnimFrequency		},
	{ "animmap",			&MatPass_AnimMap			},
	{ "cubemap",			&MatPass_CubeMap			},
	{ "map",				&MatPass_Map				},
	{ "alphamap",			&MatPass_AlphaMap			},
	{ "clampmap",			&MatPass_ClampMap			},
	{ "greymap",			&MatPass_GreyMap			},
	{ "rgbmap",				&MatPass_RGBMap				},

	{ "fragmentmap",		&MatPass_FragmentMap		},
	{ "fragmentprogram",	&MatPass_FragmentProgram	},
	{ "vertexprogram",		&MatPass_VertexProgram		},
	{ "program",			&MatPass_Program			},

	{ "rgbgen",				&MatPass_RGBGen				},
	{ "alphagen",			&MatPass_AlphaGen			},

	{ "alphafunc",			&MatPass_AlphaFunc			},
	{ "blendfunc",			&MatPass_BlendFunc			},
	{ "depthfunc",			&MatPass_DepthFunc			},
	
	{ "tcclamp",			&MatPass_TcClamp			},
	{ "tcclamps",			&MatPass_TcClampS			},
	{ "tcclampx",			&MatPass_TcClampS			},
	{ "tcclampt",			&MatPass_TcClampT			},
	{ "tcclampy",			&MatPass_TcClampT			},
	{ "tcgen",				&MatPass_TcGen				},
	{ "tcmod",				&MatPass_TcMod				},

	{ "maskcolor",			&MatPass_MaskColor			},
	{ "maskred",			&MatPass_MaskRed			},
	{ "maskgreen",			&MatPass_MaskGreen			},
	{ "maskblue",			&MatPass_MaskBlue			},
	{ "maskalpha",			&MatPass_MaskAlpha			},

	{ "nogamma",			&MatPass_NoGamma			},
	{ "nointens",			&MatPass_NoIntens			},
	{ "nomipmap",			&MatPass_NoMipmap			},
	{ "nopicmip",			&MatPass_NoPicmip			},
	{ "nocompress",			&MatPass_NoCompress			},

	{ "sizebase",			&MatPass_SizeBase			},

	{ "detail",				&MatPass_Detail				},
	{ "nodetail",			&MatPass_NotDetail			},
	{ "notdetail",			&MatPass_NotDetail			},

	{ "depthwrite",			&MatPass_DepthWrite			},
	{ "emissive",			&MatPass_Emissive			},

	{ NULL,					NULL						}
};

static const int r_numMaterialPassKeys = sizeof (r_materialPassKeys) / sizeof (r_materialPassKeys[0]) - 1;

/*
=============================================================================

	MATERIAL PROCESSING

=============================================================================
*/

static qBool MatBase_BackSided (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->cullType = MAT_CULL_BACK;
	return qTrue;
}

static qBool MatBase_Cull (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: cull with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "disable")
	|| !strcmp (str, "none")
	|| !strcmp (str, "twosided")) {
		mat->cullType = MAT_CULL_NONE;
		return qTrue;
	}
	else if (!strcmp (str, "back")
	|| !strcmp (str, "backside")
	|| !strcmp (str, "backsided")) {
		mat->cullType = MAT_CULL_BACK;
		return qTrue;
	}
	else if (!strcmp (str, "front")
	|| !strcmp (str, "frontside")
	|| !strcmp (str, "frontsided")) {
		mat->cullType = MAT_CULL_FRONT;
		return qTrue;
	}

	Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: invalid cull value: '%s'\n", str);
	return qFalse;
}

static qBool MatBase_DeformVertexes (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	vertDeform_t	*deformv;
	char			*str;

	if (mat->numDeforms == MAX_MATERIAL_DEFORMVS) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: too many deformVertexes, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	deformv = &r_currDeforms[mat->numDeforms];

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: deformVertexes with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "wave")) {
		if (!Mat_ParseFloat (ps, &deformv->args[0])) {
			Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid deformVertexes wave parameters!\n");
			return qFalse;
		}

		deformv->type = DEFORMV_WAVE;
		if (deformv->args[0])
			deformv->args[0] = 1.0f / deformv->args[0];
		if (!Mat_ParseWave (mat, ps, fileName, &deformv->func))
			return qFalse;
	}
	else if (!strcmp (str, "normal")) {
		deformv->type = DEFORMV_NORMAL;
		if (!Mat_ParseFloat (ps, &deformv->args[0])
		|| !Mat_ParseFloat (ps, &deformv->args[1])) {
			Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid deformVertexes normal parameters!\n");
			return qFalse;
		}
	}
	else if (!strcmp (str, "bulge")) {
		deformv->type = DEFORMV_BULGE;
		if (!Mat_ParseVector (mat, ps, fileName, deformv->args, 3))
			return qFalse;
		mat->flags |= MAT_DEFORMV_BULGE;
	}
	else if (!strcmp (str, "move")) {
		deformv->type = DEFORMV_MOVE;
		if (!Mat_ParseVector (mat, ps, fileName, deformv->args, 3)
		|| !Mat_ParseWave (mat, ps, fileName, &deformv->func))
			return qFalse;
	}
	else if (!strcmp (str, "autosprite")) {
		deformv->type = DEFORMV_AUTOSPRITE;
		mat->flags |= MAT_AUTOSPRITE;
	}
	else if (!strcmp (str, "autosprite2")) {
		deformv->type = DEFORMV_AUTOSPRITE2;
		mat->flags |= MAT_AUTOSPRITE;
	}
	else if (!strcmp (str, "projectionshadow")) {
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	}
	else if (!strcmp (str, "autoparticle")) {
		deformv->type = DEFORMV_AUTOPARTICLE;
	}
	else {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: invalid deformVertexes value: '%s'\n", str);
		return qFalse;
	}

	mat->numDeforms++;
	return qTrue;
}

static qBool MatBase_Subdivide (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;
	int		size = 8;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: subdivide with no parameters\n");
		return qFalse;
	}

	mat->subdivide = atoi (str);
	if (mat->subdivide < 8 || mat->subdivide > 256) {
		Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: out of bounds subdivide size: '%i', assuming 64", mat->subdivide);
		mat->subdivide = 64;
	}
	else {
		// Force power of two
		while (size <= mat->subdivide)
			size <<= 1;

		size >>= 1;

		if (size != mat->subdivide) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: subdivide size '%i' is not a power of two, forcing: '%i'\n", mat->subdivide, size);
			mat->subdivide = size;
		}
	}

	mat->flags |= MAT_SUBDIVIDE;
	return qTrue;
}

static qBool MatBase_EntityMergable (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_ENTITY_MERGABLE;
	return qTrue;
}

static qBool MatBase_DepthRange (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_DEPTHRANGE;

	if (!Mat_ParseFloat (ps, &mat->depthNear)
	|| !Mat_ParseFloat (ps, &mat->depthFar)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: missing/invalid depthRange parameters!\n");
		return qFalse;
	}
	return qTrue;
}

static qBool MatBase_DepthWrite (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_DEPTHWRITE;
	mat->addPassFlags |= MAT_PASS_DEPTHWRITE;
	return qTrue;
}

static qBool MatBase_FogParms (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	vec3_t	color, ncolor;
	float	fogDist;

	// Parse color
	if (!Mat_ParseVector (mat, ps, fileName, color, 3))
		return qFalse;
	ColorNormalizef (color, ncolor);
	mat->fogColor[0] = FloatToByte (ncolor[0]);
	mat->fogColor[1] = FloatToByte (ncolor[1]);
	mat->fogColor[2] = FloatToByte (ncolor[2]);
	mat->fogColor[3] = 255;

	// Parse distance
	if (!Mat_ParseFloat (ps, &fogDist)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: missing fogparms distance!\n");
		return qFalse;
	}
	mat->fogDist = (double)fogDist;
	if (mat->fogDist <= 0.1)
		mat->fogDist = 128.0;
	mat->fogDist = 1.0 / mat->fogDist;

	return qTrue;
}

static qBool MatBase_PolygonOffset (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_POLYGONOFFSET;
	return qTrue;
}

static qBool MatBase_Portal (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->sortKey = MAT_SORT_PORTAL;
	return qTrue;
}

static qBool MatBase_SkyParms (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	// FIXME TODO
	PS_SkipLine (ps);

	mat->flags |= MAT_SKY;
	mat->sortKey = MAT_SORT_SKY;
	return qTrue;
}

static qBool MatBase_NoCompress (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->addTexFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool MatBase_NoLerp (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_NOLERP;
	return qTrue;
}

static qBool MatBase_NoMark (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_NOMARK;
	return qTrue;
}

static qBool MatBase_NoMipMaps (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_DevPrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
		Mat_DevPrintf (PRNT_WARNING, "WARNING: noMipMaps with no parameters, assuming linear\n");
		mat->addTexFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}

	if (!strcmp (str, "linear")) {
		mat->addTexFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}
	else if (!strcmp (str, "nearest")) {
		mat->addTexFlags |= IF_NOMIPMAP_NEAREST;
		return qTrue;
	}

	Mat_DevPrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
	Mat_DevPrintf (PRNT_WARNING, "WARNING: invalid noMipMaps value, assuming linear\n");
	mat->addTexFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool MatBase_NoPicMips (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->addTexFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool MatBase_NoShadow (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	mat->flags |= MAT_NOSHADOW;
	return qTrue;
}

static qBool MatBase_SortKey (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: sortKey with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "sky"))
		mat->sortKey = MAT_SORT_SKY;
	else if (!strcmp (str, "opaque"))
		mat->sortKey = MAT_SORT_OPAQUE;
	else if (!strcmp (str, "decal"))
		mat->sortKey = MAT_SORT_DECAL;
	else if (!strcmp (str, "seethrough"))
		mat->sortKey = MAT_SORT_SEETHROUGH;
	else if (!strcmp (str, "banner"))
		mat->sortKey = MAT_SORT_BANNER;
	else if (!strcmp (str, "underwater"))
		mat->sortKey = MAT_SORT_UNDERWATER;
	else if (!strcmp (str, "entity"))
		mat->sortKey = MAT_SORT_ENTITY;
	else if (!strcmp (str, "entity2"))
		mat->sortKey = MAT_SORT_ENTITY2;
	else if (!strcmp (str, "particle"))
		mat->sortKey = MAT_SORT_PARTICLE;
	else if (!strcmp (str, "water")
	|| !strcmp (str, "glass"))
		mat->sortKey = MAT_SORT_WATER;
	else if (!strcmp (str, "additive"))
		mat->sortKey = MAT_SORT_ADDITIVE;
	else if (!strcmp (str, "nearest"))
		mat->sortKey = MAT_SORT_NEAREST;
	else if (!strcmp (str, "postprocess")
	|| !strcmp (str, "post")) {
		mat->sortKey = MAT_SORT_POSTPROCESS;
	}
	else {
		mat->sortKey = atoi (str);

		if (mat->sortKey < 0 || mat->sortKey >= MAT_SORT_MAX) {
			Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
			Mat_Printf (PRNT_ERROR, "ERROR: invalid sortKey value: '%i'", mat->sortKey);
			return qFalse;
		}
	}

	return qTrue;
}

static qBool MatBase_SurfParams (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Mat_ParseString (ps, &str)) {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: surfParams with no parameters\n");
		return qFalse;
	}

	if (mat->surfParams < 0)
		mat->surfParams = 0;

	if (!strcmp (str, "flowing"))
		mat->surfParams |= MAT_SURF_FLOWING;
	else if (!strcmp (str, "trans33"))
		mat->surfParams |= MAT_SURF_TRANS33;
	else if (!strcmp (str, "trans66"))
		mat->surfParams |= MAT_SURF_TRANS66;
	else if (!strcmp (str, "warp"))
		mat->surfParams |= MAT_SURF_WARP;
	else if (!strcmp (str, "lightmap"))
		mat->surfParams |= MAT_SURF_LIGHTMAP;
	else {
		Mat_PrintPos (PRNT_ERROR, mat, 0, ps, fileName);
		Mat_Printf (PRNT_ERROR, "ERROR: invalid surfParam value: '%s'", str);
		return qFalse;
	}

	return qTrue;
}

static qBool MatBase_SurfaceParm (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName)
{
	char		*str;

	if (!Mat_ParseString (ps, &str))
		return qTrue;

	if (!strcmp (str, "nolightmap")) {
		r_materialChecks[MATCK_NOLIGHTMAP] = qTrue;
	}

	PS_SkipLine (ps);
	return qTrue;
}

// ==========================================================================

static matKey_t r_materialBaseKeys[] = {
	{ "backsided",				&MatBase_BackSided		},
	{ "cull",					&MatBase_Cull			},
	{ "deformvertexes",			&MatBase_DeformVertexes	},
	{ "entitymergable",			&MatBase_EntityMergable	},
	{ "subdivide",				&MatBase_Subdivide		},
	{ "depthrange",				&MatBase_DepthRange		},
	{ "depthwrite",				&MatBase_DepthWrite		},
	{ "flare",					&MatBase_Flare			},
	{ "fogparms",				&MatBase_FogParms		},
	{ "polygonoffset",			&MatBase_PolygonOffset	},
	{ "portal",					&MatBase_Portal			},
	{ "skyparms",				&MatBase_SkyParms		},
	{ "sort",					&MatBase_SortKey		},
	{ "sortkey",				&MatBase_SortKey		},
	{ "surfparam",				&MatBase_SurfParams		},

	{ "nocompress",				&MatBase_NoCompress		},
	{ "noimpact",				&MatBase_NoMark			},
	{ "nolerp",					&MatBase_NoLerp			},
	{ "nomark",					&MatBase_NoMark			},
	{ "nomipmaps",				&MatBase_NoMipMaps		},
	{ "nopicmip",				&MatBase_NoPicMips		},
	{ "noshadow",				&MatBase_NoShadow		},

	// FIXME orly: TODO
	{ "foggen",					NULL					},
	{ "fogonly",				NULL					},
	{ "sky",					NULL					},

	// These are compiler/editor options
	{ "cloudparms",				NULL					},
	{ "light",					NULL					},
	{ "light1",					NULL					},
	{ "lightning",				NULL					},
	{ "nodrop",					NULL					},
	{ "nolightmap",				NULL					},
	{ "nonsolid",				NULL					},
	{ "q3map_*",				NULL					},
	{ "qer_*",					NULL					},
	{ "surfaceparm",			&MatBase_SurfaceParm	},
	{ "tesssize",				NULL					},

	{ NULL,						NULL					}
};

static const int r_numMaterialBaseKeys = sizeof (r_materialBaseKeys) / sizeof (r_materialBaseKeys[0]) - 1;

/*
=============================================================================

	MATERIAL LOADING

=============================================================================
*/

/*
==================
R_FindMaterial
==================
*/
static material_t *R_FindMaterial (char *name, matSurfParams_t surfParams)
{
	material_t	*mat, *bestMatch;
	char		tempName[MAX_QPATH];
	uint32		hashValue;

	assert (name && name[0]);
	if (!name)
		return NULL;

	ri.reg.matsSeaked++;

	// Strip extension
	Com_StripExtension (tempName, sizeof (tempName), name);
	Q_strlwr (tempName);

	hashValue = Com_HashGenericFast (tempName, MAX_MATERIAL_HASH);
	bestMatch = NULL;

	// Look for it
	for (mat=r_materialHashTree[hashValue] ; mat ; mat=mat->hashNext) {
		if (mat->surfParams != surfParams)
			continue;
		if (strcmp (mat->name, tempName))
			continue;

		if (!bestMatch || mat->pathType >= bestMatch->pathType)
			bestMatch = mat;
	}

	return bestMatch;
}


/*
==================
R_NewMaterial
==================
*/
static material_t *R_NewMaterial (char *fixedName, matPathType_t pathType)
{
	material_t	*mat;

	assert (fixedName && fixedName[0]);

	// Select material spot
	if (r_numMaterials+1 >= MAX_MATERIALS)
		Com_Error (ERR_DROP, "R_NewMaterial: MAX_MATERIALS");
	mat = &r_materialList[r_numMaterials];
	memset (mat, 0, sizeof (material_t));

	// Clear material checks
	memset (&r_materialChecks[0], 0, sizeof (r_materialChecks));

	// Clear passes
	r_numCurrPasses = 0;
	memset (&r_currPasses, 0, sizeof (r_currPasses));

	// Strip extension
	Com_StripExtension (mat->name, sizeof (mat->name), fixedName);
	Q_strlwr (mat->name);

	// Defaults
	mat->pathType = pathType;
	mat->sizeBase = -1;
	mat->surfParams = -1;

	// Link it into the hash list
	mat->hashValue = Com_HashGenericFast (mat->name, MAX_MATERIAL_HASH);
	mat->hashNext = r_materialHashTree[mat->hashValue];
	r_materialHashTree[mat->hashValue] = mat;

	r_numMaterials++;
	return mat;
}


/*
==================
R_MaterialFeatures
==================
*/
static void R_MaterialFeatures (material_t *mat)
{
	matPass_t	*pass;
	qBool			trNormals;
	int				i;

	if (mat->numDeforms) {
		mat->features |= MF_DEFORMVS;
		mat->features &= ~MF_STATIC_MESH;
	}
	else {
		mat->features |= MF_STATIC_MESH;
	}

	// Determine if triangle normals are necessary
	trNormals = qTrue;
	for (i=0 ; i<mat->numDeforms ; i++) {
		switch (mat->deforms[i].type) {
		case DEFORMV_BULGE:
		case DEFORMV_WAVE:
			trNormals = qFalse;
		case DEFORMV_NORMAL:
			mat->features |= MF_NORMALS;
			break;

		case DEFORMV_MOVE:
			break;

		default:
			trNormals = qFalse;
			break;
		}
	}

	if (trNormals)
		mat->features |= MF_TRNORMALS;

	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		// rgbGen requirements
		switch (pass->rgbGen.type) {
		case RGB_GEN_LIGHTING_DIFFUSE:
			mat->features |= MF_NORMALS;
			break;

		case RGB_GEN_VERTEX:
		case RGB_GEN_ONE_MINUS_VERTEX:
		case RGB_GEN_EXACT_VERTEX:
			mat->features |= MF_COLORS;
			break;
		}

		// alphaGen requirements
		switch (pass->alphaGen.type) {
		case ALPHA_GEN_SPECULAR:
		case ALPHA_GEN_DOT:
		case ALPHA_GEN_ONE_MINUS_DOT:
			mat->features |= MF_NORMALS;
			break;

		case ALPHA_GEN_VERTEX:
		case ALPHA_GEN_ONE_MINUS_VERTEX:
			mat->features |= MF_COLORS;
			break;
		}

		// tcGen requirements
		switch (pass->tcGen) {
		case TC_GEN_LIGHTMAP:
			mat->features |= MF_LMCOORDS;
			break;

		case TC_GEN_ENVIRONMENT:
		case TC_GEN_REFLECTION:
			mat->features |= MF_NORMALS;
			break;

		default:
			mat->features |= MF_STCOORDS;
			break;
		}
	}
}


/*
==================
R_FinishMaterial
==================
*/
static void R_FinishMaterial (material_t *mat, char *fileName)
{
	matPass_t	*pass;
	int				size, i;
	byte			*buffer;

	mat->numPasses = r_numCurrPasses;

	// Fill deforms
	if (mat->numDeforms) {
		mat->deforms = Mem_PoolAlloc (mat->numDeforms * sizeof (vertDeform_t), ri.matSysPool, 0);
		memcpy (mat->deforms, r_currDeforms, mat->numDeforms * sizeof (vertDeform_t));
	}

	// Allocate full block
	size = mat->numPasses * sizeof (matPass_t);
	for (i=0 ; i<mat->numPasses ; i++)
		size += r_currPasses[i].numTCMods * sizeof (tcMod_t);
	if (size) {
		buffer = Mem_PoolAlloc (size, ri.matSysPool, 0);

		// Fill passes
		mat->passes = (matPass_t *)buffer;
		memcpy (mat->passes, r_currPasses, mat->numPasses * sizeof (matPass_t));
		buffer += mat->numPasses * sizeof (matPass_t);

		// Fill tcMods
		for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
			if (!r_currPasses[i].numTCMods)
				continue;

			pass->tcMods = (tcMod_t *)buffer;
			pass->numTCMods = r_currPasses[i].numTCMods;
			memcpy (pass->tcMods, r_currTcMods[i], pass->numTCMods * sizeof (tcMod_t));
			buffer += pass->numTCMods * sizeof (tcMod_t);
		}
	}
	else {
		mat->passes = NULL;
		mat->numPasses = 0;
	}

	// Find out if we have a blending pass
	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		if (!(pass->flags & MAT_PASS_BLEND))
			break;
	}

	if (!mat->sortKey && mat->flags & MAT_POLYGONOFFSET)
		mat->sortKey = MAT_SORT_DECAL;

	if (i == mat->numPasses) {
		int		opaque;

		opaque = -1;

		for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
			if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO)
				opaque = i;

			// Keep rgbGen/alphaGen in check
			if (pass->rgbGen.type == RGB_GEN_UNKNOWN) {
				if (pass->flags & MAT_PASS_LIGHTMAP)
					pass->rgbGen.type = RGB_GEN_IDENTITY;
				else
					pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
			}

			if (pass->alphaGen.type == ALPHA_GEN_UNKNOWN) {
				if (pass->rgbGen.type == RGB_GEN_VERTEX)
					pass->alphaGen.type = ALPHA_GEN_VERTEX;
				else
					pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			}
		}

		// Check sort
		if (!(mat->flags & MAT_SKY) && !mat->sortKey) {
			if (opaque == -1)
				mat->sortKey = MAT_SORT_UNDERWATER;
			else if (mat->passes[opaque].alphaFunc != ALPHA_FUNC_NONE)
				mat->sortKey = MAT_SORT_DECAL;
			else
				mat->sortKey = MAT_SORT_OPAQUE;
		}
	}
	else {
		matPass_t	*sp;

		// Keep rgbGen/alphaGen in check
		for (i=0, sp=mat->passes ; i<mat->numPasses ; sp++, i++) {
			if (sp->rgbGen.type == RGB_GEN_UNKNOWN) {
				if (sp->alphaFunc != ALPHA_FUNC_NONE)
					sp->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
				else
					sp->rgbGen.type = RGB_GEN_IDENTITY;
			}

			if (sp->alphaGen.type == ALPHA_GEN_UNKNOWN) {
				if (sp->rgbGen.type == RGB_GEN_VERTEX)
					sp->alphaGen.type = ALPHA_GEN_VERTEX;
				else
					sp->alphaGen.type = ALPHA_GEN_IDENTITY;
			}
		}

		// Check sort
		if (!mat->sortKey && pass->alphaFunc != ALPHA_FUNC_NONE)
			mat->sortKey = MAT_SORT_DECAL;

		// Check depthWrite
		if (!(pass->flags & MAT_PASS_DEPTHWRITE) && !(mat->flags & MAT_SKY)) {
			pass->flags |= MAT_PASS_DEPTHWRITE;
			mat->flags |= MAT_DEPTHWRITE;
		}
	}

	// Pass checks
	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		// Set the totalMask
		pass->totalMask = pass->maskRed + pass->maskGreen + pass->maskBlue + pass->maskAlpha;

		// Check tcGen
		if (pass->tcGen == TC_GEN_BAD) {
			Mat_Printf (PRNT_WARNING, "WARNING: %s(pass #%i): no tcGen specified, assuming 'base'!\n", mat->name, i+1);
			pass->tcGen = TC_GEN_BASE;
		}
	}

	// Check sort
	if (!mat->sortKey)
		mat->sortKey = MAT_SORT_OPAQUE;

	// Check depthWrite
	if (mat->flags & MAT_SKY && mat->flags & MAT_DEPTHWRITE)
		mat->flags &= ~MAT_DEPTHWRITE;

	// Check sizeBase
	if (mat->sizeBase == -1) {
		if (mat->numPasses) {
			for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
				if (!(pass->flags & MAT_PASS_LIGHTMAP)) {
					mat->sizeBase = i;
					break;
				}
			}
			if (i == mat->numPasses) {
				mat->sizeBase = 0;
				Mat_DevPrintPos (PRNT_WARNING, mat, 0, NULL, fileName);
				Mat_DevPrintf (PRNT_WARNING, "WARNING: has default sizeBase, no non-lightmap passes so forcing to: '0'\n");
			}
			else {
				Mat_DevPrintPos (PRNT_WARNING, mat, 0, NULL, fileName);
				Mat_DevPrintf (PRNT_WARNING, "WARNING: has default sizeBase, forcing to first non-lightmap pass: '%i'\n", mat->sizeBase);
			}
		}
		else {
			mat->sizeBase = 0;
		}
	}
	else if (mat->sizeBase >= mat->numPasses && mat->numPasses) {
		Mat_PrintPos (PRNT_WARNING, mat, 0, NULL, fileName);
		Mat_Printf (PRNT_WARNING, "WARNING: invalid sizeBase value of: '%i', forcing: '0'\n", mat->sizeBase);
		mat->sizeBase = 0;
	}

	// Check the r_materialChecks
	if (r_materialChecks[MATCK_NOLIGHTMAP]) {
		for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
			if (!(pass->flags & MAT_PASS_LIGHTMAP))
				continue;

			Mat_PrintPos (PRNT_WARNING, mat, i, NULL, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: material has a lightmap pass but surfaceparms mark it as nolightmap!\n");
		}
	}

	// Pass-specific checks
	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		// Check animation framerate
		if (pass->flags & MAT_PASS_ANIMMAP) {
			if (!pass->animFPS) {
				Mat_PrintPos (PRNT_WARNING, mat, i, NULL, fileName);
				Mat_Printf (PRNT_WARNING, "WARNING: invalid animFrequency '%i'\n", pass->animFPS);
				pass->flags &= ~MAT_PASS_ANIMMAP;
			}
		}

		// Check if this pass can possibly accumulate
		pass->canAccumulate = (pass->alphaFunc == ALPHA_FUNC_NONE
			&& pass->rgbGen.type == RGB_GEN_IDENTITY
			&& pass->alphaGen.type == ALPHA_GEN_IDENTITY) ? qTrue : qFalse;
	}

	// Set features
	R_MaterialFeatures (mat);

	r_numCurrPasses = 0;
}


/*
==================
R_NewPass
==================
*/
static matPass_t *R_NewPass (material_t *mat, parse_t *ps)
{
	matPass_t	*pass;

	// Check for maximum
	if (r_numCurrPasses+1 >= MAX_MATERIAL_PASSES) {
		Mat_Printf (PRNT_WARNING, "WARNING: too many passes, ignoring\n");
		return NULL;
	}

	// Allocate a temporary spot
	pass = &r_currPasses[r_numCurrPasses];
	memset (pass, 0, sizeof (matPass_t));

	// Defaults
	pass->depthFunc = GL_LEQUAL;
	pass->blendSource = GL_SRC_ALPHA;
	pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;

	return pass;
}


/*
==================
R_FinishPass
==================
*/
void R_PassStateBits (matPass_t *pass)
{
	// Must NOT contain:
	// SB1_BLEND_ON
	pass->stateBits1 = 0;

	// Blend bits
	if (pass->flags & MAT_PASS_BLEND) {
		switch (pass->blendSource) {
		case GL_ZERO:					pass->stateBits1 |= SB1_BLENDSRC_ZERO;					break;
		case GL_ONE:					pass->stateBits1 |= SB1_BLENDSRC_ONE;					break;
		case GL_DST_COLOR:				pass->stateBits1 |= SB1_BLENDSRC_DST_COLOR;				break;
		case GL_ONE_MINUS_DST_COLOR:	pass->stateBits1 |= SB1_BLENDSRC_ONE_MINUS_DST_COLOR;	break;
		case GL_SRC_ALPHA:				pass->stateBits1 |= SB1_BLENDSRC_SRC_ALPHA;				break;
		case GL_ONE_MINUS_SRC_ALPHA:	pass->stateBits1 |= SB1_BLENDSRC_ONE_MINUS_SRC_ALPHA;	break;
		case GL_DST_ALPHA:				pass->stateBits1 |= SB1_BLENDSRC_DST_ALPHA;				break;
		case GL_ONE_MINUS_DST_ALPHA:	pass->stateBits1 |= SB1_BLENDSRC_ONE_MINUS_DST_ALPHA;	break;
		case GL_SRC_ALPHA_SATURATE:		pass->stateBits1 |= SB1_BLENDSRC_SRC_ALPHA_SATURATE;	break;

		default:
			// Shouldn't happen because it's checked during parse
			assert (0);
			break;
		}

		switch (pass->blendDest) {
		case GL_ZERO:					pass->stateBits1 |= SB1_BLENDDST_ZERO;					break;
		case GL_ONE:					pass->stateBits1 |= SB1_BLENDDST_ONE;					break;
		case GL_SRC_COLOR:				pass->stateBits1 |= SB1_BLENDDST_SRC_COLOR;				break;
		case GL_ONE_MINUS_SRC_COLOR:	pass->stateBits1 |= SB1_BLENDDST_ONE_MINUS_SRC_COLOR;	break;
		case GL_SRC_ALPHA:				pass->stateBits1 |= SB1_BLENDDST_SRC_ALPHA;				break;
		case GL_ONE_MINUS_SRC_ALPHA:	pass->stateBits1 |= SB1_BLENDDST_ONE_MINUS_SRC_ALPHA;	break;
		case GL_DST_ALPHA:				pass->stateBits1 |= SB1_BLENDDST_DST_ALPHA;				break;
		case GL_ONE_MINUS_DST_ALPHA:	pass->stateBits1 |= SB1_BLENDDST_ONE_MINUS_DST_ALPHA;	break;

		default:
			// Shouldn't happen because it's checked during parse
			assert (0);
			break;
		}
	}

	// Alphatest bits
	switch (pass->alphaFunc) {
	case ALPHA_FUNC_GT0:		pass->stateBits1 |= SB1_ATEST_GT0;		break;
	case ALPHA_FUNC_LT128:		pass->stateBits1 |= SB1_ATEST_LT128;	break;
	case ALPHA_FUNC_GE128:		pass->stateBits1 |= SB1_ATEST_GE128;	break;
	case ALPHA_FUNC_NONE:
		break;

	default:
		// Shouldn't happen because it's checked during parse
		assert (0);
		break;
	}
}
static void R_FinishPass (material_t *mat, matPass_t *pass)
{
	if (!pass)
		return;

	r_numCurrPasses++;

	pass->flags |= mat->addPassFlags;
	if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO) {
		mat->flags |= MAT_DEPTHWRITE;
		pass->flags |= MAT_PASS_DEPTHWRITE;
	}

	// Determine if we are coloring per-vertex or solidly
	switch (pass->rgbGen.type) {
		case RGB_GEN_UNKNOWN:	// Assume RGB_GEN_IDENTITY or RGB_GEN_IDENTITY_LIGHTING
		case RGB_GEN_IDENTITY:
		case RGB_GEN_IDENTITY_LIGHTING:
		case RGB_GEN_CONST:
		case RGB_GEN_COLORWAVE:
		case RGB_GEN_ENTITY:
		case RGB_GEN_ONE_MINUS_ENTITY:
			switch (pass->alphaGen.type) {
				case ALPHA_GEN_UNKNOWN:	// Assume ALPHA_GEN_IDENTITY
				case ALPHA_GEN_IDENTITY:
				case ALPHA_GEN_CONST:
				case ALPHA_GEN_ENTITY:
				case ALPHA_GEN_WAVE:
					pass->flags |= MAT_PASS_NOCOLORARRAY;
					break;
			}
			break;
	}

	// This really shouldn't happen
	if ((!pass->animNames[0] || !pass->animNames[0][0]) && !(pass->flags & MAT_PASS_LIGHTMAP)) {
		pass->blendMode = 0;
		R_PassStateBits (pass);
		return;
	}

	// Multitexture specific blending
	if (!(pass->flags & MAT_PASS_BLEND) && (ri.config.extArbMultitexture || ri.config.extSGISMultiTexture)) {
		if (pass->rgbGen.type == RGB_GEN_IDENTITY && pass->alphaGen.type == ALPHA_GEN_IDENTITY) {
			pass->blendMode = GL_REPLACE;
		}
		else {
			pass->blendSource = GL_ONE;
			pass->blendDest = GL_ZERO;
			pass->blendMode = GL_MODULATE;
		}
		R_PassStateBits (pass);
		return;
	}

	// Blend mode
	if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO) {
		pass->blendMode = GL_MODULATE;
		pass->flags &= ~MAT_PASS_BLEND;
	}
	else if ((pass->blendSource == GL_ZERO && pass->blendDest == GL_SRC_COLOR)
	|| (pass->blendSource == GL_DST_COLOR && pass->blendDest == GL_ZERO))
		pass->blendMode = GL_MODULATE;
	else if (pass->blendSource == GL_ONE && pass->blendDest == GL_ONE)
		pass->blendMode = GL_ADD;
	else if (pass->blendSource == GL_SRC_ALPHA && pass->blendDest == GL_ONE_MINUS_SRC_ALPHA)
		pass->blendMode = GL_DECAL;
	else
		pass->blendMode = 0;

	R_PassStateBits (pass);
}


/*
==================
R_ParseMaterialFile
==================
*/
static qBool R_MaterialParseTok (material_t *mat, matPass_t *pass, parse_t *ps, char *fileName, matKey_t *keys, char *token)
{
	matKey_t	*key;
	char		keyName[MAX_PS_TOKCHARS];
	char		*str;

	// Copy off a lower-case copy for faster comparisons
	Q_strncpyz (keyName, token, sizeof (keyName));
	Q_strlwr (keyName);

	// Cycle through the key list looking for a match
	for (key=keys ; key->keyWord ; key++) {
		// If this is a wildcard keyword, work some magic
		// (handy for compiler/editor keywords)
		if (strchr (key->keyWord, '*') && !key->func) {
			if (Q_WildcardMatch (key->keyWord, keyName, 1)) {
				PS_SkipLine (ps);
				return qTrue;
			}
		}

		// See if it matches the keyword
		if (strcmp (key->keyWord, keyName))
			continue;

		// This is for keys that compilers and editors use
		if (!key->func) {
			PS_SkipLine (ps);
			return qTrue;
		}

		// Failed to parse line
		if (!key->func (mat, pass, ps, fileName)) {
			PS_SkipLine (ps);
			return qFalse;
		}

		// Report any extra parameters
		if (Mat_ParseString (ps, &str)) {
			Mat_PrintPos (PRNT_WARNING, mat, r_numCurrPasses, ps, fileName);
			Mat_Printf (PRNT_WARNING, "WARNING: extra trailing parameters after key: '%s'\n", keyName);
			PS_SkipLine (ps);
			return qTrue;
		}

		// Parsed fine
		return qTrue;
	}

	// Not found
	Mat_PrintPos (PRNT_ERROR, mat, r_numCurrPasses, ps, fileName);
	Mat_Printf (PRNT_ERROR, "ERROR: unrecognized key: '%s'\n", keyName);
	PS_SkipLine (ps);
	return qFalse;
}

static void R_ParseMaterialFile (char *fixedName, matPathType_t pathType)
{
	char			*buf;
	char			matName[MAX_QPATH];
	int				fileLen;
	qBool			inMaterial;
	qBool			inPass;
	char			*token;
	material_t		*mat;
	matPass_t		*pass;
	int				numSlashes, i;
	parse_t			*ps;

	assert (fixedName && fixedName[0]);
	if (!fixedName)
		return;

	// Check for recursion
	for (numSlashes=0, i=0 ; fixedName[i] ; i++) {
		if (fixedName[i] == '/')
			numSlashes++;
	}
	if (numSlashes > 1)
		return;

	// Load the file
	Mat_Printf (0, "...loading '%s' (%s)\n", fixedName, pathType == MAT_PATHTYPE_BASEDIR ? "base" : "game");
	fileLen = FS_LoadFile (fixedName, (void **)&buf, "\n\0");
	if (!buf || fileLen <= 0) {
		Mat_DevPrintf (PRNT_ERROR, "...ERROR: couldn't load '%s' -- %s\n", fixedName, (fileLen == -1) ? "not found" : "empty file");
		return;
	}

	// Start parsing
	inMaterial = qFalse;
	inPass = qFalse;

	mat = NULL;
	pass = NULL;

	ps = PS_StartSession (buf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE);
	for ( ; PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) ; ) {
		if (inMaterial) {
			switch (token[0]) {
			case '{':
				if (!inPass) {
					inPass = qTrue;
					pass = R_NewPass (mat, ps);
					if (!pass)
						Mat_SkipBlock (mat, ps, fixedName, token);
				}
				break;

			case '}':
				if (inPass) {
					inPass = qFalse;
					R_FinishPass (mat, pass);
					break;
				}

				inMaterial = qFalse;
				R_FinishMaterial (mat, fixedName);
				break;

			default:
				if (inPass) {
					if (pass)
						R_MaterialParseTok (mat, pass, ps, fixedName, r_materialPassKeys, token);
					break;
				}

				R_MaterialParseTok (mat, NULL, ps, fixedName, r_materialBaseKeys, token);
				break;
			}
		}
		else {
			switch (token[0]) {
			case '{':
				inMaterial = qTrue;
				break;

			default:
				Com_NormalizePath (matName, sizeof (matName), token);
				mat = R_NewMaterial (matName, pathType);
				break;
			}
		}
	}

	if (inMaterial && mat) {
		Mat_Printf (PRNT_ERROR, "...ERROR: unexpected EOF in '%s' after material '%s', forcing finish\n", fixedName, mat->name);
		R_FinishMaterial (mat, fixedName);
	}

	// Done
	PS_AddErrorCount (ps, &r_numMaterialErrors, &r_numMaterialWarnings);
	PS_EndSession (ps);
	FS_FreeFile (buf);
}

/*
=============================================================================

	REGISTRATION

=============================================================================
*/

/*
==================
R_UntouchMaterial
==================
*/
static void R_UntouchMaterial (material_t *mat)
{
	matPass_t	*pass;
	int			i, j;

	ri.reg.matsReleased++;

	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		pass->fragProgPtr = NULL;
		pass->vertProgPtr = NULL;
		for (j=0 ; j<pass->animNumNames ; j++)
			pass->animImages[j] = NULL;
	}
}


/*
==================
R_ReadyMaterial
==================
*/
static void R_ReadyMaterial (material_t *mat)
{
	matPass_t	*pass;
	int			i, j;

	ri.reg.matsTouched++;

	mat->touchFrame = ri.reg.registerFrame;
	for (i=0, pass=mat->passes ; i<mat->numPasses ; pass++, i++) {
		pass->animNumImages = 0;
		for (j=0 ; j<pass->animNumNames ; j++) {
			// Fragment program
			if (pass->flags & MAT_PASS_FRAGMENTPROGRAM) {
				if (!pass->fragProgPtr) {
					pass->fragProgPtr = R_RegisterProgram (pass->fragProgName, qTrue);

					if (!pass->fragProgPtr) {
						Mat_Printf (PRNT_WARNING, "WARNING: Material '%s' can't find/load fragment program '%s' for pass #%i (anim #%i)\n",
							mat->name, pass->fragProgName, i+1, j+1);
						pass->flags &= ~MAT_PASS_FRAGMENTPROGRAM;
					}
				}
			}

			// Vertex program
			if (pass->flags & MAT_PASS_VERTEXPROGRAM) {
				if (!pass->vertProgPtr) {
					pass->vertProgPtr = R_RegisterProgram (pass->vertProgName, qFalse);

					if (!pass->vertProgPtr) {
						Mat_Printf (PRNT_WARNING, "WARNING: Material '%s' can't find/load vertex program '%s' for pass #%i (anim #%i)\n",
							mat->name, pass->vertProgName, i+1, j+1);
						pass->flags &= ~MAT_PASS_VERTEXPROGRAM;
					}
				}
			}

			// No textures stored for lightmap passes
			if (pass->flags & MAT_PASS_LIGHTMAP) {
				pass->animNumImages++;
				continue;
			}

			// Texture
			if (pass->animNames[j][0] == '$') {
				// White texture
				if (!Q_stricmp (pass->animNames[j], "$white")) {
					pass->animImages[pass->animNumImages] = ri.whiteTexture;
					pass->animNumImages++;
					continue;
				}

				// Black texture
				if (!Q_stricmp (pass->animNames[j], "$black")) {
					pass->animImages[pass->animNumImages] = ri.blackTexture;
					pass->animNumImages++;
					continue;
				}
			}

			// If it's already loaded, touch it
			if (pass->animImages[pass->animNumImages]) {
				R_TouchImage (pass->animImages[pass->animNumImages]);
				pass->animNumImages++;
				continue;
			}

			// Nope, load it
			pass->animImages[pass->animNumImages] = R_RegisterImage (pass->animNames[j], mat->addTexFlags|pass->passTexFlags|pass->animTexFlags[j]);
			if (pass->animImages[pass->animNumImages]) {
				pass->animNumImages++;
				continue;
			}

			// Report any errors
			if (pass->animNames[j][0])
				Mat_Printf (PRNT_WARNING, "WARNING: Material '%s' can't find/load '%s' for pass #%i (anim #%i)\n", mat->name, pass->animNames[j], i+1, j+1);
			else
				Mat_Printf (PRNT_WARNING, "WARNING: Material '%s' has a NULL texture name for pass #%i (anim #%i)\n", mat->name, i+1, j+1);
		}
	}
}


/*
==================
R_RegisterMaterial
==================
*/
static material_t *R_RegisterMaterial (char *name, qBool forceDefault, matRegType_t matType, matSurfParams_t surfParams)
{
	material_t		*mat;
	matPass_t	*pass;
	tcMod_t			*tcMod;
	image_t			*image;
	int				texFlags;
	char			fixedName[MAX_QPATH];
	byte			*buffer;

	assert (name && name[0]);
	if (!name || !name[0])
		return NULL;
	if (strlen(name)+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_RegisterMaterial: Material name too long!\n");
		return NULL;
	}

	// Copy and fix name
	Com_NormalizePath (fixedName, sizeof (fixedName), name);

	// See if it's already loaded
	if (!forceDefault) {
		mat = R_FindMaterial (fixedName, surfParams);
		if (mat) {
			// Add forced flags
			if (matType == MAT_RT_BSP_FLARE)
				mat->flags |= MAT_FLARE;

			R_ReadyMaterial (mat);
			return mat;
		}
	}

	buffer = NULL;

	// Default material flags
	switch (matType) {
	case MAT_RT_PIC:
		if (forceDefault) {
			texFlags = 0;
		}
		else {
			texFlags = IF_NOMIPMAP_LINEAR|IF_NOPICMIP|IF_NOINTENS;
			if (!vid_gammapics->intVal)
				texFlags |= IF_NOGAMMA;
		}
		break;

	case MAT_RT_SKYBOX:
		if (forceDefault)
			texFlags = 0;
		else
			texFlags = IF_NOMIPMAP_LINEAR|IF_NOINTENS|IF_CLAMP_ALL;
		break;

	default:
		texFlags = 0;
		break;
	}

	// If the image doesn't exist, return NULL
	if (!(image = R_RegisterImage (fixedName, texFlags)))
		return NULL;

	// Default material
	mat = R_NewMaterial (fixedName, MAT_PATHTYPE_INTERNAL);
	mat->sizeBase = 0;
	mat->surfParams = surfParams;
	mat->flags = 0;

	switch (matType) {
	case MAT_RT_ALIAS:
		mat->cullType = MAT_CULL_FRONT;
		mat->features = MF_STCOORDS|MF_NORMALS;
		mat->flags = MAT_DEPTHWRITE;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sortKey = MAT_SORT_ENTITY;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = MAT_PASS_DEPTHWRITE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		pass->tcGen = TC_GEN_BASE;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case MAT_RT_PIC:
		mat->cullType = MAT_CULL_NONE;
		mat->features = MF_STCOORDS|MF_COLORS;
		mat->flags = 0;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sortKey = MAT_SORT_ADDITIVE;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = MAT_PASS_BLEND;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		pass->tcGen = TC_GEN_BASE;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case MAT_RT_POLY:
		mat->cullType = MAT_CULL_NONE;
		mat->features = MF_STCOORDS|MF_COLORS;
		mat->flags = MAT_ENTITY_MERGABLE;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sortKey = MAT_SORT_ENTITY;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = MAT_PASS_BLEND;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		pass->tcGen = TC_GEN_BASE;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case MAT_RT_SKYBOX:
		mat->cullType = MAT_CULL_FRONT;
		mat->features = MF_STCOORDS;
		mat->flags = MAT_DEPTHWRITE|MAT_SKY|MAT_NOMARK;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sortKey = MAT_SORT_SKY;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = MAT_PASS_DEPTHWRITE|MAT_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_REPLACE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qTrue;
		R_PassStateBits (pass);
		break;

	default:
	case MAT_RT_BSP:
		if (surfParams > 0 && surfParams & (MAT_SURF_TRANS33|MAT_SURF_TRANS66|MAT_SURF_WARP)) {
			mat->cullType = MAT_CULL_FRONT;
			mat->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			mat->flags = MAT_ENTITY_MERGABLE|MAT_DEPTHWRITE;

			// Add flowing if it's got the flag
			if (surfParams & MAT_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (matPass_t) + sizeof (tcMod_t), ri.matSysPool, 0);
				mat->passes = (matPass_t *)buffer;
				buffer += sizeof (matPass_t);
			}
			else {
				mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
			}
			mat->numPasses = 1;

			// Sort key
			if (surfParams & (MAT_SURF_TRANS33|MAT_SURF_TRANS66))
				mat->sortKey = MAT_SORT_WATER;
			else
				mat->sortKey = MAT_SORT_OPAQUE;

			pass = &mat->passes[0];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->blendSource = GL_SRC_ALPHA;
			pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
			pass->flags = MAT_PASS_DEPTHWRITE|MAT_PASS_NOCOLORARRAY;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_MODULATE;

			// Subdivide if warping
			if (surfParams & MAT_SURF_WARP) {
				// FIXME: 128 is a hack, and is supposed to be 64 to match Quake2.
				// I changed it because of Gloom's map assault, which has a massive
				// entity warp brush, that manages to overflow the rendering backend.
				// Until multiple meshes are stored per surface, nothing can be done here.
				mat->flags |= MAT_SUBDIVIDE|MAT_NOMARK;
				mat->subdivide = 128;
				pass->tcGen = TC_GEN_WARP;
			}
			else {
				pass->tcGen = TC_GEN_BASE;
			}

			pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
			if (surfParams & (MAT_SURF_TRANS33|MAT_SURF_TRANS66)) {
				// FIXME: This caused problems with certain trans surfaces
				// Apparently Q2bsp generates vertices for both sides of trans surfaces?!
				//mat->cullType = MAT_CULL_NONE;
				pass->flags |= MAT_PASS_BLEND;
				pass->alphaGen.type = ALPHA_GEN_CONST;
				pass->alphaGen.args[0] = (surfParams & MAT_SURF_TRANS33) ? 0.33f : 0.66f;
			}
			else {
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			}
			pass->canAccumulate = qFalse;
			R_PassStateBits (pass);

			// Add flowing if it's got the flag
			if (surfParams & MAT_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		else if (surfParams > 0 && mat->surfParams & MAT_SURF_LIGHTMAP) {
			mat->cullType = MAT_CULL_FRONT;
			mat->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			mat->flags = MAT_ENTITY_MERGABLE|MAT_DEPTHWRITE;

			// Add flowing if it's got the flag
			if (surfParams & MAT_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (matPass_t) * 2 + sizeof (tcMod_t), ri.matSysPool, 0);
				mat->passes = (matPass_t *)buffer;
				buffer += sizeof (matPass_t) * 2;
			}
			else {
				mat->passes = Mem_PoolAlloc (sizeof (matPass_t) * 2, ri.matSysPool, 0);
			}
			mat->numPasses = 2;
			mat->sizeBase = 1;
			mat->sortKey = MAT_SORT_OPAQUE;

			pass = &mat->passes[0];
			pass->animNames[pass->animNumNames++] = Mem_PoolStrDup ("$lightmap", ri.matSysPool, 0);
			pass->flags = MAT_PASS_DEPTHWRITE|MAT_PASS_LIGHTMAP|MAT_PASS_NOCOLORARRAY;
			pass->tcGen = TC_GEN_LIGHTMAP;
			pass->depthFunc = GL_LEQUAL;
			pass->blendSource = GL_ONE;
			pass->blendDest = GL_ZERO;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			pass->canAccumulate = qTrue;
			R_PassStateBits (pass);

			pass = &mat->passes[1];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->flags = MAT_PASS_BLEND|MAT_PASS_NOCOLORARRAY;
			pass->tcGen = TC_GEN_BASE;
			pass->depthFunc = GL_LEQUAL;
			pass->blendSource = GL_ZERO;
			pass->blendDest = GL_SRC_COLOR;
			pass->blendMode = GL_MODULATE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			pass->canAccumulate = qTrue;
			R_PassStateBits (pass);

			// Add flowing if it's got the flag
			if (surfParams & MAT_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		else {
			mat->cullType = MAT_CULL_FRONT;
			mat->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			mat->flags = MAT_ENTITY_MERGABLE|MAT_DEPTHWRITE;
			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & MAT_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (matPass_t) + sizeof (tcMod_t), ri.matSysPool, 0);
				mat->passes = (matPass_t *)buffer;
				buffer += sizeof (matPass_t);
			}
			else {
				mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
			}
			mat->numPasses = 1;
			mat->sizeBase = 0;
			mat->sortKey = MAT_SORT_OPAQUE;

			pass = &mat->passes[0];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->blendSource = GL_SRC_ALPHA;
			pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
			pass->flags = MAT_PASS_DEPTHWRITE|MAT_PASS_NOCOLORARRAY;
			pass->tcGen = TC_GEN_BASE;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			pass->canAccumulate = qTrue;
			R_PassStateBits (pass);

			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & MAT_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		break;

	case MAT_RT_BSP_FLARE:
		mat->cullType = MAT_CULL_NONE;
		mat->features = MF_STCOORDS|MF_COLORS|MF_STATIC_MESH;
		mat->flags = MAT_FLARE;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sizeBase = 0;
		mat->sortKey = MAT_SORT_ADDITIVE;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = MAT_PASS_BLEND|MAT_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_BASE;
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ONE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case MAT_RT_BSP_VERTEX:
		mat->cullType = MAT_CULL_FRONT;
		mat->features = MF_STCOORDS|MF_COLORS|MF_TRNORMALS|MF_STATIC_MESH;
		mat->flags = MAT_DEPTHWRITE;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t), ri.matSysPool, 0);
		mat->numPasses = 1;
		mat->sizeBase = 0;
		mat->sortKey = MAT_SORT_OPAQUE;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = MAT_PASS_DEPTHWRITE;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case MAT_RT_BSP_LM:
		mat->cullType = MAT_CULL_FRONT;
		mat->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
		mat->flags = MAT_DEPTHWRITE;
		mat->passes = Mem_PoolAlloc (sizeof (matPass_t) * 2, ri.matSysPool, 0);
		mat->numPasses = 2;
		mat->sizeBase = 1;
		mat->sortKey = MAT_SORT_OPAQUE;

		pass = &mat->passes[0];
		pass->animNames[pass->animNumNames++] = Mem_PoolStrDup ("$lightmap", ri.matSysPool, 0);
		pass->flags = MAT_PASS_DEPTHWRITE|MAT_PASS_LIGHTMAP|MAT_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->depthFunc = GL_LEQUAL;
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ZERO;
		pass->blendMode = GL_REPLACE;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qTrue;
		R_PassStateBits (pass);

		pass = &mat->passes[1];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.matSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = MAT_PASS_BLEND|MAT_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_BASE;
		pass->depthFunc = GL_LEQUAL;
		pass->blendSource = GL_ZERO;
		pass->blendDest = GL_SRC_COLOR;
		pass->blendMode = GL_MODULATE;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qTrue;
		R_PassStateBits (pass);
		break;
	}

	// Register images
	R_ReadyMaterial (mat);
	return mat;
}

material_t *R_RegisterPic (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_PIC, -1);
}

material_t *R_RegisterPoly (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_POLY, -1);
}

material_t *R_RegisterSkin (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_ALIAS, -1);
}

material_t *R_RegisterSky (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_SKYBOX, -1);
}

material_t *R_RegisterTexture (char *name, matSurfParams_t surfParams)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_BSP, surfParams);
}

material_t *R_RegisterFlare (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_BSP_FLARE, -1);
}

material_t *R_RegisterTextureLM (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_BSP_LM, -1);
}

material_t *R_RegisterTextureVertex (char *name)
{
	return R_RegisterMaterial (name, qFalse, MAT_RT_BSP_VERTEX, -1);
}


/*
==================
R_EndMaterialRegistration
==================
*/
void R_EndMaterialRegistration (void)
{
	material_t	*mat;
	uint32		i;

	for (i=0, mat=r_materialList ; i<r_numMaterials ; mat++, i++) {
		if (!(mat->flags & MAT_NOFLUSH) && mat->touchFrame != ri.reg.registerFrame) {
			R_UntouchMaterial (mat);
			continue;	// Not touched
		}

		R_ReadyMaterial (mat);
	}
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
==================
R_MaterialList_f
==================
*/
static void R_MaterialList_f (void)
{
	material_t		*mat;
	char			*wildCard;
	int				totPasses;
	int				totMaterials;
	uint32			i;

	if (Cmd_Argc () == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	Com_Printf (0, "------------- Loaded Materials -------------\n");
	Com_Printf (0, "Base Mat# PASS NAME\n");
	Com_Printf (0, "---- ---- ---- ----------------------------------\n");

	for (i=0, totMaterials=0, totPasses=0, mat=r_materialList ; i<r_numMaterials ; mat++, i++) {
		if (!mat->name[0])
			continue;
		if (!Q_WildcardMatch (wildCard, mat->name, 1))
			continue;

		switch (mat->pathType) {
		case MAT_PATHTYPE_INTERNAL:
			Com_Printf (0, "Intr ");
			break;

		case MAT_PATHTYPE_BASEDIR:
			Com_Printf (0, "Base ");
			break;

		case MAT_PATHTYPE_MODDIR:
			Com_Printf (0, "Mod  ");
			break;
		}

		Com_Printf (0, "%4d %4d %s\n", totMaterials, mat->numPasses, mat->name);

		totPasses += mat->numPasses;
		totMaterials++;
	}

	Com_Printf (0, "--------------------------------------------\n");
	Com_Printf (0, "Total matching materials/passes: %d/%d\n", totMaterials, totPasses);
	Com_Printf (0, "--------------------------------------------\n");
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static void	*cmd_materialList;

/*
==================
R_MaterialInit
==================
*/
void R_MaterialInit (void)
{
	static char		*fileList[MAX_MATERIALS];
	char			fixedName[MAX_QPATH];
	size_t			numFiles, i;
	matPathType_t	pathType;
	char			*name;
	uint32			initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n-------- Material Initialization ---------\n");

	// Clear lists
	r_numMaterials = 0;
	memset (&r_materialList, 0, sizeof (material_t) * MAX_MATERIALS);
	memset (r_materialHashTree, 0, sizeof (material_t *) * MAX_MATERIAL_HASH);

	// Console commands
	cmd_materialList	= Cmd_AddCommand ("materiallist",		R_MaterialList_f,		"Prints to the console a list of loaded materials");

	// Load scripts
	r_numMaterialErrors = 0;
	r_numMaterialWarnings = 0;

	numFiles = FS_FindFiles ("scripts", "*scripts/*.shd", "shd", fileList, MAX_MATERIALS, qTrue, qFalse);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/scripts/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir material?
		if (strstr (fileList[i], BASE_MODDIRNAME "/"))
			pathType = MAT_PATHTYPE_BASEDIR;
		else
			pathType = MAT_PATHTYPE_MODDIR;

		R_ParseMaterialFile (name, pathType);
	}
	FS_FreeFileList (fileList, numFiles);

	numFiles = FS_FindFiles ("scripts", "*scripts/*.shader", "shader", fileList, MAX_MATERIALS, qTrue, qFalse);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/scripts/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir material?
		if (strstr (fileList[i], BASE_MODDIRNAME "/"))
			pathType = MAT_PATHTYPE_BASEDIR;
		else
			pathType = MAT_PATHTYPE_MODDIR;

		R_ParseMaterialFile (name, pathType);
	}
	FS_FreeFileList (fileList, numFiles);

	// Material counterparts
	r_cinMaterial = R_RegisterMaterial (ri.cinTexture->name, qTrue, MAT_RT_PIC, -1);
	r_noMaterial = R_RegisterMaterial (ri.noTexture->name, qTrue, MAT_RT_BSP, -1);
	r_noMaterialLightmap = R_RegisterMaterial (ri.noTexture->name, qTrue, MAT_RT_BSP, MAT_SURF_LIGHTMAP);
	r_noMaterialSky = R_RegisterMaterial (ri.noTexture->name, qTrue, MAT_RT_SKYBOX, -1);
	r_whiteMaterial = R_RegisterMaterial (ri.whiteTexture->name, qTrue, MAT_RT_PIC, -1);
	r_blackMaterial = R_RegisterMaterial (ri.blackTexture->name, qTrue, MAT_RT_PIC, -1);

	r_cinMaterial->flags |= MAT_NOFLUSH;
	r_noMaterial->flags |= MAT_NOFLUSH;
	r_noMaterialLightmap->flags |= MAT_NOFLUSH;
	r_noMaterialSky->flags |= MAT_NOFLUSH;
	r_whiteMaterial->flags |= MAT_NOFLUSH;
	r_blackMaterial->flags |= MAT_NOFLUSH;

	Com_Printf (0, "----------------------------------------\n");

	// Check memory integrity
	Mem_CheckPoolIntegrity (ri.matSysPool);

	Com_Printf (0, "MATERIALS - %i error(s), %i warning(s)\n", r_numMaterialErrors, r_numMaterialWarnings);
	Com_Printf (0, "%u materials loaded in %ums\n", r_numMaterials, Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
R_MaterialShutdown
==================
*/
void R_MaterialShutdown (void)
{
	size_t	size;

	Com_Printf (0, "Material system shutdown:\n");

	// Remove commands
	Cmd_RemoveCommand ("materiallist", cmd_materialList);

	// Free all loaded materials
	size = Mem_FreePool (ri.matSysPool);
	Com_Printf (0, "...releasing %u bytes...\n", size);
}
