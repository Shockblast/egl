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
// rf_shader.c
// Shader loading, caching, and some primitive surface rendering
//

#include "rf_local.h"

#define MAX_SHADER_HASH			(MAX_SHADERS/4)

static shader_t			r_shaderList[MAX_SHADERS];
static shader_t			*r_shaderHashTree[MAX_SHADER_HASH];
static uint32			r_numShaders;

enum {
	SHCK_NOLIGHTMAP,

	SHCK_MAX
};

static qBool			r_shaderChecks[SHCK_MAX];

static uint32			r_numCurrPasses;
static shaderPass_t		r_currPasses[MAX_SHADER_PASSES];
static vertDeform_t		r_currDeforms[MAX_SHADER_DEFORMVS];
static tcMod_t			r_currTcMods[MAX_SHADER_PASSES][MAX_SHADER_TCMODS];

static uint32			r_numShaderErrors;
static uint32			r_numShaderWarnings;

typedef struct shaderKey_s {
	char		*keyWord;
	qBool		(*func)(shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName);
} shaderKey_t;

shader_t	*r_cinShader;
shader_t	*r_noShader;
shader_t	*r_noShaderLightmap;
shader_t	*r_noShaderSky;
shader_t	*r_whiteShader;
shader_t	*r_blackShader;

/*
=============================================================================

	SHADER PARSING

=============================================================================
*/

/*
==================
Shd_PrintPos
==================
*/
static void Shd_PrintPos (comPrint_t flags, shader_t *shader, int passNum, parse_t *ps, char *fileName)
{
	uint32		line, col;

	// Increment tallies
	if (flags & PRNT_ERROR)
		r_numShaderErrors++;
	else if (flags & PRNT_WARNING)
		r_numShaderWarnings++;

	if (ps) {
		// Print the position
		PS_GetPosition (ps, &line, &col);
		if (passNum)
			Com_Printf (flags, "%s(line #%i col#%i): Shader '%s', pass #%i\n", fileName, line, col, shader->name, passNum+1);
		else
			Com_Printf (flags, "%s(line #%i col#%i): Shader '%s'\n", fileName, line, col, shader->name);
		return;
	}

	// Print the position
	if (passNum) {
		Com_Printf (flags, "%s: Shader '%s', pass #%i\n", fileName, shader->name, passNum+1);
		return;
	}

	Com_Printf (flags, "%s: Shader '%s'\n", fileName, shader->name);
}


/*
==================
Shd_DevPrintPos
==================
*/
static void Shd_DevPrintPos (comPrint_t flags, shader_t *shader, int passNum, parse_t *ps, char *fileName)
{
	if (!developer->intVal)
		return;

	Shd_PrintPos (flags, shader, passNum, ps, fileName);
}


/*
==================
Shd_Printf
==================
*/
static void Shd_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (flags & PRNT_ERROR)
		r_numShaderErrors++;
	else if (flags & PRNT_WARNING)
		r_numShaderWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
Shd_DevPrintf
==================
*/
static void Shd_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!developer->intVal)
		return;

	if (flags & PRNT_ERROR)
		r_numShaderErrors++;
	else if (flags & PRNT_WARNING)
		r_numShaderWarnings++;

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (msg, sizeof (msg), fmt, argptr);
	va_end (argptr);

	// Print
	Com_ConPrint (flags, msg);
}


/*
==================
Shd_ParseString
==================
*/
static qBool Shd_ParseString (parse_t *ps, char **target)
{
	char	*token;

	if (!PS_ParseToken (ps, PSF_TO_LOWER, &token) || token[0] == '}')
		return qFalse;

	*target = token;
	return qTrue;
}

#define Shd_ParseFloat(ps,target)	PS_ParseDataType((ps),0,PSDT_FLOAT,(target),1)
#define Shd_ParseInt(ps,target)		PS_ParseDataType((ps),0,PSDT_INTEGER,(target),1)

/*
==================
Shd_ParseWave
==================
*/
static qBool Shd_ParseWave (shader_t *shader, parse_t *ps, char *fileName, shaderFunc_t *func)
{
	char	*str;

	// Parse the wave type
	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: unable to parse wave type\n");
		return qFalse;
	}

	if (!strcmp (str, "sin"))
		func->type = SHADER_FUNC_SIN;
	if (!strcmp (str, "triangle"))
		func->type = SHADER_FUNC_TRIANGLE;
	if (!strcmp (str, "square"))
		func->type = SHADER_FUNC_SQUARE;
	if (!strcmp (str, "sawtooth"))
		func->type = SHADER_FUNC_SAWTOOTH;
	if (!strcmp (str, "inversesawtooth"))
		func->type = SHADER_FUNC_INVERSESAWTOOTH;
	if (!strcmp (str, "noise"))
		func->type = SHADER_FUNC_NOISE;

	// Parse args
	if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, func->args, 4)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid wave arguments!\n");
		return qFalse;
	}

	return qTrue;
}


/*
==================
Shd_ParseVector

FIXME: Deprecate for PS_ParseDataType
==================
*/
static qBool Shd_ParseVector (shader_t *shader, parse_t *ps, char *fileName, float *vec, uint32 size)
{
	qBool	inBrackets;
	char	*str;
	uint32	i;

	if (!size) {
		return qTrue;
	}
	else if (size == 1) {
		if (!Shd_ParseFloat (ps, &vec[0])) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
			return qFalse;
		}
	}

	// Check brackets
	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: unable to parse vector!\n");
		return qFalse;
	}
	if (!strcmp (str, "(")) {
		inBrackets = qTrue;
		if (!Shd_ParseString (ps, &str)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
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
		if (!Shd_ParseFloat (ps, &vec[i])) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid vector parameters!\n");
			return qFalse;
		}
	}

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters!\n");
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
			if (!Shd_ParseString (ps, &str)) {
				Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
				Shd_Printf (PRNT_ERROR, "ERROR: missing vector end-bracket!\n");
				return qFalse;
			}
		}
	}

	return qTrue;
}


/*
==================
Shd_SkipBlock
==================
*/
static void Shd_SkipBlock (shader_t *shader, parse_t *ps, char *fileName, char *token)
{
	int		braceCount;

	// Opening brace
	if (token[0] != '{') {
		if (!PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) || token[0] != '{') {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: expecting '{' to skip a block, got '%s'!\n", token);
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

static qBool ShdPass_AnimFrequency (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	if (!Shd_ParseInt (ps, &pass->animFPS)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: animFrequency with no parameters\n");
		return qFalse;
	}

	pass->flags |= SHADER_PASS_ANIMMAP;
	return qTrue;
}

static qBool ShdPass_AnimMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	pass->animNumNames = 0;

	// Parse the framerate
	if (!Shd_ParseInt (ps, &pass->animFPS)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: missing animMap framerate!\n");
		return qFalse;
	}

	pass->flags |= SHADER_PASS_ANIMMAP;
	pass->tcGen = TC_GEN_BASE;

	// Parse names
	for ( ; ; ) {
		if (!Shd_ParseString (ps, &str)) {
			if (!pass->animNumNames) {
				pass->animFPS = 0;
				pass->flags &= ~SHADER_PASS_ANIMMAP;

				Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
				Shd_Printf (PRNT_ERROR, "ERROR: animMap with no images!\n");
				return qFalse;
			}
			break;
		}

		if (pass->animNumNames+1 >= MAX_SHADER_ANIM_FRAMES) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
			PS_SkipLine (ps);
			return qTrue;
		}

		if (strlen(str)+1 >= MAX_QPATH) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
			str[MAX_QPATH-1] = '\0';
		}

		pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	}

	return qTrue;
}

static qBool ShdPass_MapExt (shader_t *shader, shaderPass_t *pass, parse_t *ps, texFlags_t addTexFlags, char *fileName)
{
	char	*str;

	// Check for too many frames
	if (pass->animNumNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	// Parse the first parameter
	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: map with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "$lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_LIGHTMAP;
	}
	else {
		pass->tcGen = TC_GEN_BASE;
		if (!strcmp (str, "$rgb")) {
			pass->animTexFlags[pass->animNumNames] |= IF_NOALPHA;
			if (!Shd_ParseString (ps, &str)) {
				Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
				Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid map parameters\n");
				return qFalse;
			}
		}
		else if (!strcmp (str, "$alpha")) {
			pass->animTexFlags[pass->animNumNames] |= IF_NORGB;
			if (!Shd_ParseString (ps, &str)) {
				Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
				Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid map parameters\n");
				return qFalse;
			}
		}
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animTexFlags[pass->animNumNames] |= addTexFlags;
	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	return qTrue;
}

static qBool ShdPass_Map (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	return ShdPass_MapExt (shader, pass, ps, 0, fileName);
}

static qBool ShdPass_AlphaMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (pass->animNumNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	pass->tcGen = TC_GEN_BASE;
	pass->animTexFlags[pass->animNumNames] |= IF_NORGB;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: alphaMap with no parameters\n");
		return qFalse;
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	return qTrue;
}

static qBool ShdPass_ClampMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	return ShdPass_MapExt (shader, pass, ps, IF_CLAMP, fileName);
}

static qBool ShdPass_RGBMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (pass->animNumNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	pass->tcGen = TC_GEN_BASE;
	pass->animTexFlags[pass->animNumNames] |= IF_NOALPHA;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: rgbMap with no parameters\n");
		return qFalse;
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	return qTrue;
}

static qBool ShdPass_CubeMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (pass->animNumNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many animation frames, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: cubeMap with no parameters\n");
		return qFalse;
	}

	if (ri.config.extTexCubeMap) {
		pass->flags |= SHADER_PASS_CUBEMAP;
		pass->animTexFlags[pass->animNumNames] |= IT_CUBEMAP|IF_CLAMP;

		pass->tcGen = TC_GEN_REFLECTION;
	}
	else {
		pass->tcGen = TC_GEN_ENVIRONMENT;
	}

	if (strlen(str)+1 >= MAX_QPATH) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[pass->animNumNames++] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	return qTrue;
}

static qBool ShdPass_FragmentMap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	int		index;
	char	*str;

	// Check for the extension
	if (!ri.config.extFragmentProgram) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: fragmentMap used and extension not available\n");
		return qFalse;
	}

	// Parse the index
	if (!Shd_ParseInt (ps, &index)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: fragmentMap with no parameters\n");
		return qFalse;
	}

	if (index < 0) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: invalid fragmentMap index '%i'!\n", index);
		return qFalse;
	}
	if (index >= pass->animNumNames) {
		if (index+1 > MAX_SHADER_ANIM_FRAMES || index+1 > ri.config.maxTexUnits) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: too many fragmentMap images, ignoring\n");
			PS_SkipLine (ps);
			return qTrue;
		}
		pass->animNumNames = index+1;
	}

	// Parse image options
	pass->animTexFlags[index] = 0;
	for ( ; ; ) {
		if (!Shd_ParseString (ps, &str)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid fragmentMap parameters!\n");
			return qFalse;
		}

		if (!strcmp (str, "clamp"))
			pass->animTexFlags[index] |= IF_CLAMP;
		else if (!strcmp (str, "cubemap")) {
			if (!ri.config.extTexCubeMap) {
				Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
				Shd_Printf (PRNT_ERROR, "ERROR: cubeMap used and extension not available!\n");
				return qFalse;
			}
			pass->animTexFlags[index] |= IT_CUBEMAP|IF_CLAMP;
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
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: animMap frame name '%s' too long, and will be truncated!\n", str);
		str[MAX_QPATH-1] = '\0';
	}

	pass->animNames[index] = Mem_PoolStrDup (str, ri.shaderSysPool, 0);
	return qTrue;
}

static qBool ShdPass_FragmentProgram (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extension
	if (!ri.config.extFragmentProgram) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: fragmentProgram used and extension not available\n");
		return qFalse;
	}

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: fragmentProgram with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= SHADER_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool ShdPass_VertexProgram (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extension
	if (!ri.config.extVertexProgram) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: vertexProgram used and extension not available\n");
		return qFalse;
	}

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: vertexProgram with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	pass->flags |= SHADER_PASS_VERTEXPROGRAM;
	return qTrue;
}

static qBool ShdPass_Program (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	// Check for the extensions
	if (!ri.config.extFragmentProgram || !ri.config.extVertexProgram) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: program used and extensions not available\n");
		return qFalse;
	}

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: program with no parameters\n");
		return qFalse;
	}

	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= SHADER_PASS_VERTEXPROGRAM|SHADER_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool ShdPass_RGBGen (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: rgbGen with no parameters\n");
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
		return Shd_ParseWave (shader, ps, fileName, &pass->rgbGen.func);
	}
	else if (!strcmp (str, "colorwave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		if (!Shd_ParseVector (shader, ps, fileName, pass->rgbGen.fArgs, 3)
		|| !Shd_ParseWave (shader, ps, fileName, &pass->rgbGen.func))
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
		if (!Shd_ParseVector (shader, ps, fileName, color, 3))
			return qFalse;
		ColorNormalizef (color, pass->rgbGen.fArgs);
		Vec3Scale (pass->rgbGen.fArgs, div, pass->rgbGen.fArgs);

		pass->rgbGen.bArgs[0] = FloatToByte (pass->rgbGen.fArgs[0]);
		pass->rgbGen.bArgs[1] = FloatToByte (pass->rgbGen.fArgs[1]);
		pass->rgbGen.bArgs[2] = FloatToByte (pass->rgbGen.fArgs[2]);
		return qTrue;
	}

	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid rgbGen value: '%s'\n", str);
	return qFalse;
}

static qBool ShdPass_AlphaGen (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;
	float	f, g;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: alphaGen with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "identity")) {
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "const") || !strcmp (str, "constant")) {
		if (!Shd_ParseFloat (ps, &f)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_CONST;
		pass->alphaGen.args[0] = fabs (f);
		return qTrue;
	}
	else if (!strcmp (str, "wave")) {
		pass->alphaGen.type = ALPHA_GEN_WAVE;
		return Shd_ParseWave (shader, ps, fileName, &pass->alphaGen.func);
	}
	else if (!strcmp (str, "portal")) {
		if (!Shd_ParseFloat (ps, &f)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
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
		if (!Shd_ParseFloat (ps, &f)
		|| !Shd_ParseFloat (ps, &g)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
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
		if (!Shd_ParseFloat (ps, &f)
		|| !Shd_ParseFloat (ps, &g)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid parameters\n");
			return qFalse;
		}

		pass->alphaGen.type = ALPHA_GEN_ONE_MINUS_DOT;
		pass->alphaGen.args[0] = fabs (f);
		pass->alphaGen.args[1] = fabs (g);
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}

	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid alphaGen value: '%s'\n", str);
	return qFalse;
}

static qBool ShdPass_AlphaFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: alphaFunc with no parameters\n");
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

	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid alphaFunc value: '%s'\n", str);
	return qFalse;
}

static qBool ShdPass_BlendFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: blendFunc with no parameters\n");
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
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: has an invalid blend source: '%s', assuming GL_ONE\n", str);
			pass->blendSource = GL_ONE;
		}

		// Find the blend dest
		if (!Shd_ParseString (ps, &str)) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: missing blend dest, assuming GL_ONE\n");
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
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: has an invalid blend dest: '%s', assuming GL_ONE\n", str);
			pass->blendDest = GL_ONE;
		}
	}

	pass->flags |= SHADER_PASS_BLEND;
	return qTrue;
}

static qBool ShdPass_DepthFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: depthFunc with no parameters\n");
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

	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid depthFunc value: '%s'\n", str);
	return qFalse;
}

static qBool ShdPass_TcGen (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: tcGen with no parameters\n");
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
		if (!Shd_ParseVector (shader, ps, fileName, pass->tcGenVec[0], 4)
		|| !Shd_ParseVector (shader, ps, fileName, pass->tcGenVec[1], 4))
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

	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid tcGen value: '%s'\n", str);
	return qFalse;
}

static qBool ShdPass_TcMod (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	tcMod_t	*tcMod;
	char	*str;
	float	val;
	int		i;

	if (pass->numTCMods == MAX_SHADER_TCMODS) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many tcMods, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	tcMod = &r_currTcMods[r_numCurrPasses][r_currPasses[r_numCurrPasses].numTCMods];

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: tcMod with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "rotate")) {
		if (!Shd_ParseFloat (ps, &val)) {
			Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid tcMod rotate parameters!\n");
			return qFalse;
		}

		tcMod->args[0] = -val / 360.0f;
		if (!tcMod->args[0]) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: invalid tcMod rotate: '%f', ignoring\n", val);
			PS_SkipLine (ps);
			return qTrue;
		}
		tcMod->type = TC_MOD_ROTATE;
	}
	else if (!strcmp (str, "scale")) {
		tcMod->type = TC_MOD_SCALE;
		if (!Shd_ParseVector (shader, ps, fileName, tcMod->args, 2))
			return qFalse;
	}
	else if (!strcmp (str, "scroll")) {
		tcMod->type = TC_MOD_SCROLL;
		if (!Shd_ParseVector (shader, ps, fileName, tcMod->args, 2))
			return qFalse;
	}
	else if (!strcmp (str, "stretch")) {
		shaderFunc_t	func;

		if (!Shd_ParseWave (shader, ps, fileName, &func))
			return qFalse;

		tcMod->args[0] = func.type;
		for (i=1 ; i<5 ; i++) {
			tcMod->args[i] = func.args[i-1];
		}
		tcMod->type = TC_MOD_STRETCH;
	}
	else if (!strcmp (str, "transform")) {
		if (!Shd_ParseVector (shader, ps, fileName, tcMod->args, 6))
			return qFalse;
		tcMod->args[4] = tcMod->args[4] - floor(tcMod->args[4]);
		tcMod->args[5] = tcMod->args[5] - floor(tcMod->args[5]);
		tcMod->type = TC_MOD_TRANSFORM;
	}
	else if (!strcmp (str, "turb")) {
		if (!Shd_ParseVector (shader, ps, fileName, tcMod->args, 4))
			return qFalse;
		tcMod->type = TC_MOD_TURB;
	}
	else {
		Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: invalid tcMod value: '%s'\n", str);
		return qFalse;
	}

	r_currPasses[r_numCurrPasses].numTCMods++;
	return qTrue;
}

static qBool ShdPass_MaskColor (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskRed = qTrue;
	pass->maskGreen = qTrue;
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShdPass_MaskRed (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskRed = qTrue;
	return qTrue;
}

static qBool ShdPass_MaskGreen (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskGreen = qTrue;
	return qTrue;
}

static qBool ShdPass_MaskBlue (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShdPass_MaskAlpha (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->maskAlpha = qTrue;
	return qTrue;
}

static qBool ShdPass_NoGamma (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOGAMMA;
	return qTrue;
}

static qBool ShdPass_NoIntens (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOINTENS;
	return qTrue;
}

static qBool ShdPass_NoMipmap (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_DevPrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_DevPrintf (PRNT_WARNING, "WARNING: noMipMap with no type specified, assuming linear\n");
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

	Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_WARNING, "WARNING: invalid noMipMap value: '%s', assuming linear\n", str);
	pass->passTexFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool ShdPass_NoPicmip (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool ShdPass_NoCompress (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool ShdPass_TcClamp (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->passTexFlags |= IF_CLAMP;
	return qTrue;
}

static qBool ShdPass_SizeBase (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->sizeBase = r_numCurrPasses;
	return qTrue;
}

static qBool ShdPass_Detail (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= SHADER_PASS_DETAIL;
	return qTrue;
}

static qBool ShdPass_NotDetail (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= SHADER_PASS_NOTDETAIL;
	return qTrue;
}

static qBool ShdPass_DepthWrite (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	pass->flags |= SHADER_PASS_DEPTHWRITE;
	shader->flags |= SHADER_DEPTHWRITE;
	return qTrue;
}

// ==========================================================================

static shaderKey_t r_shaderPassKeys[] = {
	{ "animfrequency",		&ShdPass_AnimFrequency		},
	{ "animmap",			&ShdPass_AnimMap			},
	{ "cubemap",			&ShdPass_CubeMap			},
	{ "map",				&ShdPass_Map				},
	{ "alphamap",			&ShdPass_AlphaMap			},
	{ "clampmap",			&ShdPass_ClampMap			},
	{ "rgbmap",				&ShdPass_RGBMap				},

	{ "fragmentmap",		&ShdPass_FragmentMap		},
	{ "fragmentprogram",	&ShdPass_FragmentProgram	},
	{ "vertexprogram",		&ShdPass_VertexProgram		},
	{ "program",			&ShdPass_Program			},

	{ "rgbgen",				&ShdPass_RGBGen				},
	{ "alphagen",			&ShdPass_AlphaGen			},

	{ "alphafunc",			&ShdPass_AlphaFunc			},
	{ "blendfunc",			&ShdPass_BlendFunc			},
	{ "depthfunc",			&ShdPass_DepthFunc			},

	{ "tcclamp",			&ShdPass_TcClamp			},
	{ "tcgen",				&ShdPass_TcGen				},
	{ "tcmod",				&ShdPass_TcMod				},

	{ "maskcolor",			&ShdPass_MaskColor			},
	{ "maskred",			&ShdPass_MaskRed			},
	{ "maskgreen",			&ShdPass_MaskGreen			},
	{ "maskblue",			&ShdPass_MaskBlue			},
	{ "maskalpha",			&ShdPass_MaskAlpha			},

	{ "nogamma",			&ShdPass_NoGamma			},
	{ "nointens",			&ShdPass_NoIntens			},
	{ "nomipmap",			&ShdPass_NoMipmap			},
	{ "nopicmip",			&ShdPass_NoPicmip			},
	{ "nocompress",			&ShdPass_NoCompress			},

	{ "sizebase",			&ShdPass_SizeBase			},

	{ "detail",				&ShdPass_Detail				},
	{ "nodetail",			&ShdPass_NotDetail			},
	{ "notdetail",			&ShdPass_NotDetail			},

	{ "depthwrite",			&ShdPass_DepthWrite			},

	{ NULL,					NULL						}
};

static const int r_numShaderPassKeys = sizeof (r_shaderPassKeys) / sizeof (r_shaderPassKeys[0]) - 1;

/*
=============================================================================

	SHADER PROCESSING

=============================================================================
*/

static qBool ShdBase_BackSided (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->cullType = SHADER_CULL_BACK;
	return qTrue;
}

static qBool ShdBase_Cull (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: cull with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "disable")
	|| !strcmp (str, "none")
	|| !strcmp (str, "twosided")) {
		shader->cullType = SHADER_CULL_NONE;
		return qTrue;
	}
	else if (!strcmp (str, "back")
	|| !strcmp (str, "backside")
	|| !strcmp (str, "backsided")) {
		shader->cullType = SHADER_CULL_BACK;
		return qTrue;
	}
	else if (!strcmp (str, "front")
	|| !strcmp (str, "frontside")
	|| !strcmp (str, "frontsided")) {
		shader->cullType = SHADER_CULL_FRONT;
		return qTrue;
	}

	Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: invalid cull value: '%s'\n", str);
	return qFalse;
}

static qBool ShdBase_DeformVertexes (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	vertDeform_t	*deformv;
	char			*str;

	if (shader->numDeforms == MAX_SHADER_DEFORMVS) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: too many deformVertexes, ignoring\n");
		PS_SkipLine (ps);
		return qTrue;
	}

	deformv = &r_currDeforms[shader->numDeforms];

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: deformVertexes with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "wave")) {
		if (!Shd_ParseFloat (ps, &deformv->args[0])) {
			Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid deformVertexes wave parameters!\n");
			return qFalse;
		}

		deformv->type = DEFORMV_WAVE;
		if (deformv->args[0])
			deformv->args[0] = 1.0f / deformv->args[0];
		if (!Shd_ParseWave (shader, ps, fileName, &deformv->func))
			return qFalse;
	}
	else if (!strcmp (str, "normal")) {
		deformv->type = DEFORMV_NORMAL;
		if (!Shd_ParseFloat (ps, &deformv->args[0])
		|| !Shd_ParseFloat (ps, &deformv->args[1])) {
			Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid deformVertexes normal parameters!\n");
			return qFalse;
		}
	}
	else if (!strcmp (str, "bulge")) {
		deformv->type = DEFORMV_BULGE;
		if (!Shd_ParseVector (shader, ps, fileName, deformv->args, 3))
			return qFalse;
		shader->flags |= SHADER_DEFORMV_BULGE;
	}
	else if (!strcmp (str, "move")) {
		deformv->type = DEFORMV_MOVE;
		if (!Shd_ParseVector (shader, ps, fileName, deformv->args, 3)
		|| !Shd_ParseWave (shader, ps, fileName, &deformv->func))
			return qFalse;
	}
	else if (!strcmp (str, "autosprite")) {
		deformv->type = DEFORMV_AUTOSPRITE;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if (!strcmp (str, "autosprite2")) {
		deformv->type = DEFORMV_AUTOSPRITE2;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if (!strcmp (str, "projectionshadow")) {
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	}
	else if (!strcmp (str, "autoparticle")) {
		deformv->type = DEFORMV_AUTOPARTICLE;
	}
	else {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: invalid deformVertexes value: '%s'\n", str);
		return qFalse;
	}

	shader->numDeforms++;
	return qTrue;
}

static qBool ShdBase_Subdivide (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;
	int		size = 8;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: subdivide with no parameters\n");
		return qFalse;
	}

	shader->subdivide = atoi (str);
	if (shader->subdivide < 8 || shader->subdivide > 256) {
		Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: out of bounds subdivide size: '%i', assuming 64", shader->subdivide);
		shader->subdivide = 64;
	}
	else {
		// Force power of two
		while (size <= shader->subdivide)
			size <<= 1;

		size >>= 1;

		if (size != shader->subdivide) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: subdivide size '%i' is not a power of two, forcing: '%i'\n", shader->subdivide, size);
			shader->subdivide = size;
		}
	}

	shader->flags |= SHADER_SUBDIVIDE;
	return qTrue;
}

static qBool ShdBase_EntityMergable (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_ENTITY_MERGABLE;
	return qTrue;
}

static qBool ShdBase_DepthRange (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_DEPTHRANGE;

	if (!Shd_ParseFloat (ps, &shader->depthNear)
	|| !Shd_ParseFloat (ps, &shader->depthFar)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: missing/invalid depthRange parameters!\n");
		return qFalse;
	}
	return qTrue;
}

static qBool ShdBase_DepthWrite (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_DEPTHWRITE;
	shader->addPassFlags |= SHADER_PASS_DEPTHWRITE;
	return qTrue;
}

static qBool ShdBase_FogParms (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	vec3_t	color, ncolor;
	float	fogDist;

	// Parse color
	if (!Shd_ParseVector (shader, ps, fileName, color, 3))
		return qFalse;
	ColorNormalizef (color, ncolor);
	shader->fogColor[0] = FloatToByte (ncolor[0]);
	shader->fogColor[1] = FloatToByte (ncolor[1]);
	shader->fogColor[2] = FloatToByte (ncolor[2]);
	shader->fogColor[3] = 255;

	// Parse distance
	if (!Shd_ParseFloat (ps, &fogDist)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: missing fogparms distance!\n");
		return qFalse;
	}
	shader->fogDist = (double)fogDist;
	if (shader->fogDist <= 0.1)
		shader->fogDist = 128.0;
	shader->fogDist = 1.0 / shader->fogDist;

	return qTrue;
}

static qBool ShdBase_PolygonOffset (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_POLYGONOFFSET;
	return qTrue;
}

static qBool ShdBase_Portal (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->sortKey = SHADER_SORT_PORTAL;
	return qTrue;
}

static qBool ShdBase_SkyParms (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	// FIXME TODO
	PS_SkipLine (ps);

	shader->flags |= SHADER_SKY;
	shader->sortKey = SHADER_SORT_SKY;
	return qTrue;
}

static qBool ShdBase_NoCompress (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->addTexFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool ShdBase_NoLerp (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_NOLERP;
	return qTrue;
}

static qBool ShdBase_NoMark (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_NOMARK;
	return qTrue;
}

static qBool ShdBase_NoMipMaps (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_DevPrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
		Shd_DevPrintf (PRNT_WARNING, "WARNING: noMipMaps with no parameters, assuming linear\n");
		shader->addTexFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}

	if (!strcmp (str, "linear")) {
		shader->addTexFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}
	else if (!strcmp (str, "nearest")) {
		shader->addTexFlags |= IF_NOMIPMAP_NEAREST;
		return qTrue;
	}

	Shd_DevPrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
	Shd_DevPrintf (PRNT_WARNING, "WARNING: invalid noMipMaps value, assuming linear\n");
	shader->addTexFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool ShdBase_NoPicMips (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->addTexFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool ShdBase_NoShadow (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	shader->flags |= SHADER_NOSHADOW;
	return qTrue;
}

static qBool ShdBase_SortKey (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: sortKey with no parameters\n");
		return qFalse;
	}

	if (!strcmp (str, "sky"))
		shader->sortKey = SHADER_SORT_SKY;
	else if (!strcmp (str, "opaque"))
		shader->sortKey = SHADER_SORT_OPAQUE;
	else if (!strcmp (str, "decal"))
		shader->sortKey = SHADER_SORT_DECAL;
	else if (!strcmp (str, "seethrough"))
		shader->sortKey = SHADER_SORT_SEETHROUGH;
	else if (!strcmp (str, "banner"))
		shader->sortKey = SHADER_SORT_BANNER;
	else if (!strcmp (str, "underwater"))
		shader->sortKey = SHADER_SORT_UNDERWATER;
	else if (!strcmp (str, "entity"))
		shader->sortKey = SHADER_SORT_ENTITY;
	else if (!strcmp (str, "entity2"))
		shader->sortKey = SHADER_SORT_ENTITY2;
	else if (!strcmp (str, "particle"))
		shader->sortKey = SHADER_SORT_PARTICLE;
	else if (!strcmp (str, "water")
	|| !strcmp (str, "glass"))
		shader->sortKey = SHADER_SORT_WATER;
	else if (!strcmp (str, "additive"))
		shader->sortKey = SHADER_SORT_ADDITIVE;
	else if (!strcmp (str, "nearest"))
		shader->sortKey = SHADER_SORT_NEAREST;
	else if (!strcmp (str, "postprocess")
	|| !strcmp (str, "post")) {
		shader->sortKey = SHADER_SORT_POSTPROCESS;
	}
	else {
		shader->sortKey = atoi (str);

		if (shader->sortKey < 0 || shader->sortKey >= SHADER_SORT_MAX) {
			Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
			Shd_Printf (PRNT_ERROR, "ERROR: invalid sortKey value: '%i'", shader->sortKey);
			return qFalse;
		}
	}

	return qTrue;
}

static qBool ShdBase_SurfParams (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char	*str;

	if (!Shd_ParseString (ps, &str)) {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: surfParams with no parameters\n");
		return qFalse;
	}

	if (shader->surfParams < 0)
		shader->surfParams = 0;

	if (!strcmp (str, "flowing"))
		shader->surfParams |= SHADER_SURF_FLOWING;
	else if (!strcmp (str, "trans33"))
		shader->surfParams |= SHADER_SURF_TRANS33;
	else if (!strcmp (str, "trans66"))
		shader->surfParams |= SHADER_SURF_TRANS66;
	else if (!strcmp (str, "warp"))
		shader->surfParams |= SHADER_SURF_WARP;
	else if (!strcmp (str, "lightmap"))
		shader->surfParams |= SHADER_SURF_LIGHTMAP;
	else {
		Shd_PrintPos (PRNT_ERROR, shader, 0, ps, fileName);
		Shd_Printf (PRNT_ERROR, "ERROR: invalid surfParam value: '%s'", str);
		return qFalse;
	}

	return qTrue;
}

static qBool ShdBase_SurfaceParm (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName)
{
	char		*str;

	if (!Shd_ParseString (ps, &str))
		return qTrue;

	if (!strcmp (str, "nolightmap")) {
		r_shaderChecks[SHCK_NOLIGHTMAP] = qTrue;
	}

	PS_SkipLine (ps);
	return qTrue;
}

// ==========================================================================

static shaderKey_t r_shaderBaseKeys[] = {
	{ "backsided",				&ShdBase_BackSided		},
	{ "cull",					&ShdBase_Cull			},
	{ "deformvertexes",			&ShdBase_DeformVertexes	},
	{ "entitymergable",			&ShdBase_EntityMergable	},
	{ "subdivide",				&ShdBase_Subdivide		},
	{ "depthrange",				&ShdBase_DepthRange		},
	{ "depthwrite",				&ShdBase_DepthWrite		},
	{ "fogparms",				&ShdBase_FogParms		},
	{ "polygonoffset",			&ShdBase_PolygonOffset	},
	{ "portal",					&ShdBase_Portal			},
	{ "skyparms",				&ShdBase_SkyParms		},
	{ "sort",					&ShdBase_SortKey		},
	{ "sortkey",				&ShdBase_SortKey		},
	{ "surfparam",				&ShdBase_SurfParams		},

	{ "nocompress",				&ShdBase_NoCompress		},
	{ "noimpact",				&ShdBase_NoMark			},
	{ "nolerp",					&ShdBase_NoLerp			},
	{ "nomark",					&ShdBase_NoMark			},
	{ "nomipmaps",				&ShdBase_NoMipMaps		},
	{ "nopicmip",				&ShdBase_NoPicMips		},
	{ "noshadow",				&ShdBase_NoShadow		},

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
	{ "surfaceparm",			&ShdBase_SurfaceParm	},
	{ "tesssize",				NULL					},

	{ NULL,						NULL					}
};

static const int r_numShaderBaseKeys = sizeof (r_shaderBaseKeys) / sizeof (r_shaderBaseKeys[0]) - 1;

/*
=============================================================================

	SHADER LOADING

=============================================================================
*/

/*
==================
R_FindShader
==================
*/
static shader_t *R_FindShader (char *name, shSurfParams_t surfParams)
{
	shader_t	*shader, *bestMatch;
	char		tempName[MAX_QPATH];
	uint32		hashValue;

	assert (name && name[0]);
	if (!name)
		return NULL;

	ri.reg.shadersSeaked++;

	// Strip extension
	Com_StripExtension (tempName, sizeof (tempName), name);
	Q_strlwr (tempName);

	hashValue = Com_HashGenericFast (tempName, MAX_SHADER_HASH);
	bestMatch = NULL;

	// Look for it
	for (shader=r_shaderHashTree[hashValue] ; shader ; shader=shader->hashNext) {
		if (shader->surfParams != surfParams)
			continue;
		if (strcmp (shader->name, tempName))
			continue;

		if (!bestMatch || shader->pathType >= bestMatch->pathType)
			bestMatch = shader;
	}

	return bestMatch;
}


/*
==================
R_NewShader
==================
*/
static shader_t *R_NewShader (char *fixedName, shPathType_t pathType)
{
	shader_t	*shader;

	assert (fixedName && fixedName[0]);

	// Select shader spot
	if (r_numShaders+1 >= MAX_SHADERS)
		Com_Error (ERR_DROP, "R_NewShader: MAX_SHADERS");
	shader = &r_shaderList[r_numShaders];
	memset (shader, 0, sizeof (shader_t));

	// Clear shader checks
	memset (&r_shaderChecks[0], 0, sizeof (r_shaderChecks));

	// Clear passes
	r_numCurrPasses = 0;
	memset (&r_currPasses, 0, sizeof (r_currPasses));

	// Strip extension
	Com_StripExtension (shader->name, sizeof (shader->name), fixedName);
	Q_strlwr (shader->name);

	// Defaults
	shader->pathType = pathType;
	shader->sizeBase = -1;
	shader->surfParams = -1;

	// Link it into the hash list
	shader->hashValue = Com_HashGenericFast (shader->name, MAX_SHADER_HASH);
	shader->hashNext = r_shaderHashTree[shader->hashValue];
	r_shaderHashTree[shader->hashValue] = shader;

	r_numShaders++;
	return shader;
}


/*
==================
R_ShaderFeatures
==================
*/
static void R_ShaderFeatures (shader_t *shader)
{
	shaderPass_t	*pass;
	qBool			trNormals;
	int				i;

	if (shader->numDeforms) {
		shader->features |= MF_DEFORMVS;
		shader->features &= ~MF_STATIC_MESH;
	}
	else {
		shader->features |= MF_STATIC_MESH;
	}

	// Determine if triangle normals are necessary
	trNormals = qTrue;
	for (i=0 ; i<shader->numDeforms ; i++) {
		switch (shader->deforms[i].type) {
		case DEFORMV_BULGE:
		case DEFORMV_WAVE:
			trNormals = qFalse;
		case DEFORMV_NORMAL:
			shader->features |= MF_NORMALS;
			break;

		case DEFORMV_MOVE:
			break;

		default:
			trNormals = qFalse;
			break;
		}
	}

	if (trNormals)
		shader->features |= MF_TRNORMALS;

	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		// rgbGen requirements
		switch (pass->rgbGen.type) {
		case RGB_GEN_LIGHTING_DIFFUSE:
			shader->features |= MF_NORMALS;
			break;

		case RGB_GEN_VERTEX:
		case RGB_GEN_ONE_MINUS_VERTEX:
		case RGB_GEN_EXACT_VERTEX:
			shader->features |= MF_COLORS;
			break;
		}

		// alphaGen requirements
		switch (pass->alphaGen.type) {
		case ALPHA_GEN_SPECULAR:
		case ALPHA_GEN_DOT:
		case ALPHA_GEN_ONE_MINUS_DOT:
			shader->features |= MF_NORMALS;
			break;

		case ALPHA_GEN_VERTEX:
		case ALPHA_GEN_ONE_MINUS_VERTEX:
			shader->features |= MF_COLORS;
			break;
		}

		// tcGen requirements
		switch (pass->tcGen) {
		case TC_GEN_LIGHTMAP:
			shader->features |= MF_LMCOORDS;
			break;

		case TC_GEN_ENVIRONMENT:
		case TC_GEN_REFLECTION:
			shader->features |= MF_NORMALS;
			break;

		default:
			shader->features |= MF_STCOORDS;
			break;
		}
	}
}


/*
==================
R_FinishShader
==================
*/
static void R_FinishShader (shader_t *shader, char *fileName)
{
	shaderPass_t	*pass;
	int				size, i;
	byte			*buffer;

	shader->numPasses = r_numCurrPasses;

	// Fill deforms
	if (shader->numDeforms) {
		shader->deforms = Mem_PoolAllocExt (shader->numDeforms * sizeof (vertDeform_t), qFalse, ri.shaderSysPool, 0);
		memcpy (shader->deforms, r_currDeforms, shader->numDeforms * sizeof (vertDeform_t));
	}

	// Allocate full block
	size = shader->numPasses * sizeof (shaderPass_t);
	for (i=0 ; i<shader->numPasses ; i++)
		size += r_currPasses[i].numTCMods * sizeof (tcMod_t);
	if (size) {
		buffer = Mem_PoolAllocExt (size, qFalse, ri.shaderSysPool, 0);

		// Fill passes
		shader->passes = (shaderPass_t *)buffer;
		memcpy (shader->passes, r_currPasses, shader->numPasses * sizeof (shaderPass_t));
		buffer += shader->numPasses * sizeof (shaderPass_t);

		// Fill tcMods
		for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
			if (!r_currPasses[i].numTCMods)
				continue;

			pass->tcMods = (tcMod_t *)buffer;
			pass->numTCMods = r_currPasses[i].numTCMods;
			memcpy (pass->tcMods, r_currTcMods[i], pass->numTCMods * sizeof (tcMod_t));
			buffer += pass->numTCMods * sizeof (tcMod_t);
		}
	}
	else {
		shader->passes = NULL;
		shader->numPasses = 0;
	}

	// Find out if we have a blending pass
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		if (!(pass->flags & SHADER_PASS_BLEND))
			break;
	}

	if (!shader->sortKey && shader->flags & SHADER_POLYGONOFFSET)
		shader->sortKey = SHADER_SORT_DECAL;

	if (i == shader->numPasses) {
		int		opaque;

		opaque = -1;

		for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
			if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO)
				opaque = i;

			// Keep rgbGen/alphaGen in check
			if (pass->rgbGen.type == RGB_GEN_UNKNOWN) {
				if (pass->flags & SHADER_PASS_LIGHTMAP)
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
		if (!(shader->flags & SHADER_SKY) && !shader->sortKey) {
			if (opaque == -1)
				shader->sortKey = SHADER_SORT_UNDERWATER;
			else if (shader->passes[opaque].alphaFunc != ALPHA_FUNC_NONE)
				shader->sortKey = SHADER_SORT_DECAL;
			else
				shader->sortKey = SHADER_SORT_OPAQUE;
		}
	}
	else {
		shaderPass_t	*sp;

		// Keep rgbGen/alphaGen in check
		for (i=0, sp=shader->passes ; i<shader->numPasses ; sp++, i++) {
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
		if (!shader->sortKey && pass->alphaFunc != ALPHA_FUNC_NONE)
			shader->sortKey = SHADER_SORT_DECAL;

		// Check depthWrite
		if (!(pass->flags & SHADER_PASS_DEPTHWRITE) && !(shader->flags & SHADER_SKY)) {
			pass->flags |= SHADER_PASS_DEPTHWRITE;
			shader->flags |= SHADER_DEPTHWRITE;
		}
	}

	// Pass checks
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		// Set the totalMask
		pass->totalMask = pass->maskRed + pass->maskGreen + pass->maskBlue + pass->maskAlpha;

		// Check tcGen
		if (pass->tcGen == TC_GEN_BAD) {
			Shd_Printf (PRNT_WARNING, "WARNING: %s(pass #%i): no tcGen specified, assuming 'base'!\n", shader->name, i+1);
			pass->tcGen = TC_GEN_BASE;
		}
	}

	// Check sort
	if (!shader->sortKey)
		shader->sortKey = SHADER_SORT_OPAQUE;

	// Check depthWrite
	if (shader->flags & SHADER_SKY && shader->flags & SHADER_DEPTHWRITE)
		shader->flags &= ~SHADER_DEPTHWRITE;

	// Check sizeBase
	if (shader->sizeBase == -1) {
		if (shader->numPasses) {
			for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
				if (!(pass->flags & SHADER_PASS_LIGHTMAP)) {
					shader->sizeBase = i;
					break;
				}
			}
			if (i == shader->numPasses) {
				shader->sizeBase = 0;
				Shd_DevPrintPos (PRNT_WARNING, shader, 0, NULL, fileName);
				Shd_DevPrintf (PRNT_WARNING, "WARNING: has default sizeBase, no non-lightmap passes so forcing to: '0'\n");
			}
			else {
				Shd_DevPrintPos (PRNT_WARNING, shader, 0, NULL, fileName);
				Shd_DevPrintf (PRNT_WARNING, "WARNING: has default sizeBase, forcing to first non-lightmap pass: '%i'\n", shader->sizeBase);
			}
		}
		else {
			shader->sizeBase = 0;
		}
	}
	else if (shader->sizeBase >= shader->numPasses && shader->numPasses) {
		Shd_PrintPos (PRNT_WARNING, shader, 0, NULL, fileName);
		Shd_Printf (PRNT_WARNING, "WARNING: invalid sizeBase value of: '%i', forcing: '0'\n", shader->sizeBase);
		shader->sizeBase = 0;
	}

	// Check the r_shaderChecks
	if (r_shaderChecks[SHCK_NOLIGHTMAP]) {
		for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
			if (!(pass->flags & SHADER_PASS_LIGHTMAP))
				continue;

			Shd_PrintPos (PRNT_WARNING, shader, i, NULL, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: shader has a lightmap pass but surfaceparms mark it as nolightmap!\n");
		}
	}

	// Pass-specific checks
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		// Check animation framerate
		if (pass->flags & SHADER_PASS_ANIMMAP) {
			if (!pass->animFPS) {
				Shd_PrintPos (PRNT_WARNING, shader, i, NULL, fileName);
				Shd_Printf (PRNT_WARNING, "WARNING: invalid animFrequency '%i'\n", pass->animFPS);
				pass->flags &= ~SHADER_PASS_ANIMMAP;
			}
		}

		// Check if this pass can possibly accumulate
		pass->canAccumulate = (pass->alphaFunc == ALPHA_FUNC_NONE
			&& pass->rgbGen.type == RGB_GEN_IDENTITY
			&& pass->alphaGen.type == ALPHA_GEN_IDENTITY) ? qTrue : qFalse;
	}

	// Set features
	R_ShaderFeatures (shader);

	r_numCurrPasses = 0;
}


/*
==================
R_NewPass
==================
*/
static shaderPass_t *R_NewPass (shader_t *shader, parse_t *ps)
{
	shaderPass_t	*pass;

	// Check for maximum
	if (r_numCurrPasses+1 >= MAX_SHADER_PASSES) {
		Shd_Printf (PRNT_WARNING, "WARNING: too many passes, ignoring\n");
		return NULL;
	}

	// Allocate a temporary spot
	pass = &r_currPasses[r_numCurrPasses];
	memset (pass, 0, sizeof (shaderPass_t));

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
void R_PassStateBits (shaderPass_t *pass)
{
	// Must NOT contain:
	// SB1_BLEND_ON
	pass->stateBits1 = 0;

	// Blend bits
	if (pass->flags & SHADER_PASS_BLEND) {
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
static void R_FinishPass (shader_t *shader, shaderPass_t *pass)
{
	if (!pass)
		return;

	r_numCurrPasses++;

	pass->flags |= shader->addPassFlags;
	if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO) {
		shader->flags |= SHADER_DEPTHWRITE;
		pass->flags |= SHADER_PASS_DEPTHWRITE;
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
					pass->flags |= SHADER_PASS_NOCOLORARRAY;
					break;
			}
			break;
	}

	// This really shouldn't happen
	if ((!pass->animNames[0] || !pass->animNames[0][0]) && !(pass->flags & SHADER_PASS_LIGHTMAP)) {
		pass->blendMode = 0;
		R_PassStateBits (pass);
		return;
	}

	// Multitexture specific blending
	if (!(pass->flags & SHADER_PASS_BLEND) && (ri.config.extArbMultitexture || ri.config.extSGISMultiTexture)) {
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
		pass->flags &= ~SHADER_PASS_BLEND;
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
R_ParseShaderFile
==================
*/
static qBool R_ShaderParseTok (shader_t *shader, shaderPass_t *pass, parse_t *ps, char *fileName, shaderKey_t *keys, char *token)
{
	shaderKey_t	*key;
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
		if (!key->func (shader, pass, ps, fileName)) {
			PS_SkipLine (ps);
			return qFalse;
		}

		// Report any extra parameters
		if (Shd_ParseString (ps, &str)) {
			Shd_PrintPos (PRNT_WARNING, shader, r_numCurrPasses, ps, fileName);
			Shd_Printf (PRNT_WARNING, "WARNING: extra trailing parameters after key: '%s'\n", keyName);
			PS_SkipLine (ps);
			return qTrue;
		}

		// Parsed fine
		return qTrue;
	}

	// Not found
	Shd_PrintPos (PRNT_ERROR, shader, r_numCurrPasses, ps, fileName);
	Shd_Printf (PRNT_ERROR, "ERROR: unrecognized key: '%s'\n", keyName);
	PS_SkipLine (ps);
	return qFalse;
}
static void R_ParseShaderFile (char *fixedName, shPathType_t pathType)
{
	char			*buf;
	char			shaderName[MAX_QPATH];
	int				fileLen;
	qBool			inShader;
	qBool			inPass;
	char			*token;
	shader_t		*shader;
	shaderPass_t	*pass;
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
	Shd_Printf (0, "...loading '%s' (%s)\n", fixedName, pathType == SHADER_PATHTYPE_BASEDIR ? "base" : "game");
	fileLen = FS_LoadFile (fixedName, (void **)&buf, "\n\0");
	if (!buf || fileLen <= 0) {
		Shd_DevPrintf (PRNT_ERROR, "...ERROR: couldn't load '%s' -- %s\n", fixedName, (fileLen == -1) ? "not found" : "empty file");
		return;
	}

	// Start parsing
	inShader = qFalse;
	inPass = qFalse;

	shader = NULL;
	pass = NULL;

	ps = PS_StartSession (buf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE);
	for ( ; PS_ParseToken (ps, PSF_ALLOW_NEWLINES, &token) ; ) {
		if (inShader) {
			switch (token[0]) {
			case '{':
				if (!inPass) {
					inPass = qTrue;
					pass = R_NewPass (shader, ps);
					if (!pass)
						Shd_SkipBlock (shader, ps, fixedName, token);
				}
				break;

			case '}':
				if (inPass) {
					inPass = qFalse;
					R_FinishPass (shader, pass);
					break;
				}

				inShader = qFalse;
				R_FinishShader (shader, fixedName);
				break;

			default:
				if (inPass) {
					if (pass)
						R_ShaderParseTok (shader, pass, ps, fixedName, r_shaderPassKeys, token);
					break;
				}

				R_ShaderParseTok (shader, NULL, ps, fixedName, r_shaderBaseKeys, token);
				break;
			}
		}
		else {
			switch (token[0]) {
			case '{':
				inShader = qTrue;
				break;

			default:
				Com_NormalizePath (shaderName, sizeof (shaderName), token);
				shader = R_NewShader (shaderName, pathType);
				break;
			}
		}
	}

	if (inShader && shader) {
		Shd_Printf (PRNT_ERROR, "...ERROR: unexpected EOF in '%s' after shader '%s', forcing finish\n", fixedName, shader->name);
		R_FinishShader (shader, fixedName);
	}

	// Done
	PS_AddErrorCount (ps, &r_numShaderErrors, &r_numShaderWarnings);
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
R_UntouchShader
==================
*/
static void R_UntouchShader (shader_t *shader)
{
	shaderPass_t	*pass;
	int				i, j;

	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		pass->fragProgPtr = NULL;
		pass->vertProgPtr = NULL;
		for (j=0 ; j<pass->animNumNames ; j++)
			pass->animImages[j] = NULL;
	}
}


/*
==================
R_ReadyShader
==================
*/
static void R_ReadyShader (shader_t *shader)
{
	shaderPass_t	*pass;
	int				i, j;

	shader->touchFrame = ri.reg.registerFrame;
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		pass->animNumImages = 0;
		for (j=0 ; j<pass->animNumNames ; j++) {
			// Fragment program
			if (pass->flags & SHADER_PASS_FRAGMENTPROGRAM) {
				if (!pass->fragProgPtr) {
					pass->fragProgPtr = R_RegisterProgram (pass->fragProgName, qTrue);

					if (!pass->fragProgPtr) {
						Shd_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load fragment program '%s' for pass #%i (anim #%i)\n",
							shader->name, pass->fragProgName, i+1, j+1);
						pass->flags &= ~SHADER_PASS_FRAGMENTPROGRAM;
					}
				}
			}

			// Vertex program
			if (pass->flags & SHADER_PASS_VERTEXPROGRAM) {
				if (!pass->vertProgPtr) {
					pass->vertProgPtr = R_RegisterProgram (pass->vertProgName, qFalse);

					if (!pass->vertProgPtr) {
						Shd_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load vertex program '%s' for pass #%i (anim #%i)\n",
							shader->name, pass->vertProgName, i+1, j+1);
						pass->flags &= ~SHADER_PASS_VERTEXPROGRAM;
					}
				}
			}

			// No textures stored for lightmap passes
			if (pass->flags & SHADER_PASS_LIGHTMAP) {
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
			pass->animImages[pass->animNumImages] = R_RegisterImage (pass->animNames[j], shader->addTexFlags|pass->passTexFlags|pass->animTexFlags[j]);
			if (pass->animImages[pass->animNumImages]) {
				pass->animNumImages++;
				continue;
			}

			// Report any errors
			if (pass->animNames[j][0])
				Shd_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load '%s' for pass #%i (anim #%i)\n", shader->name, pass->animNames[j], i+1, j+1);
			else
				Shd_Printf (PRNT_WARNING, "WARNING: Shader '%s' has a NULL texture name for pass #%i (anim #%i)\n", shader->name, i+1, j+1);
		}
	}
}


/*
==================
R_RegisterShader
==================
*/
static shader_t *R_RegisterShader (char *name, qBool forceDefault, shRegType_t shaderType, shSurfParams_t surfParams)
{
	shader_t		*shader;
	shaderPass_t	*pass;
	tcMod_t			*tcMod;
	image_t			*image;
	int				texFlags;
	char			fixedName[MAX_QPATH];
	byte			*buffer;

	assert (name && name[0]);
	if (!name || !name[0])
		return NULL;
	if (strlen(name)+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_RegisterShader: Shader name too long!\n");
		return NULL;
	}

	// Copy and fix name
	Com_NormalizePath (fixedName, sizeof (fixedName), name);

	// See if it's already loaded
	if (!forceDefault) {
		shader = R_FindShader (fixedName, surfParams);
		if (shader) {
			// Add forced flags
			if (shaderType == SHADER_BSP_FLARE)
				shader->flags |= SHADER_FLARE;

			R_ReadyShader (shader);
			return shader;
		}
	}

	buffer = NULL;

	// Default shader flags
	switch (shaderType) {
	case SHADER_PIC:
		if (forceDefault) {
			texFlags = 0;
		}
		else {
			texFlags = IF_NOMIPMAP_LINEAR|IF_NOPICMIP|IF_NOINTENS;
			if (!vid_gammapics->intVal)
				texFlags |= IF_NOGAMMA;
		}
		break;

	case SHADER_SKYBOX:
		if (forceDefault)
			texFlags = 0;
		else
			texFlags = IF_NOMIPMAP_LINEAR|IF_NOINTENS|IF_CLAMP;
		break;

	default:
		texFlags = 0;
		break;
	}

	// If the image doesn't exist, return NULL
	if (!(image = R_RegisterImage (fixedName, texFlags)))
		return NULL;

	// Default shader
	shader = R_NewShader (fixedName, SHADER_PATHTYPE_INTERNAL);
	shader->sizeBase = 0;
	shader->surfParams = surfParams;
	shader->flags = 0;

	switch (shaderType) {
	case SHADER_ALIAS:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_NORMALS;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ENTITY;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		pass->tcGen = TC_GEN_BASE;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case SHADER_PIC:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS;
		shader->flags = 0;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ADDITIVE;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = SHADER_PASS_BLEND;
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

	case SHADER_POLY:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS;
		shader->flags = SHADER_ENTITY_MERGABLE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ENTITY;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = SHADER_PASS_BLEND;
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

	case SHADER_SKYBOX:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS;
		shader->flags = SHADER_DEPTHWRITE|SHADER_SKY|SHADER_NOMARK;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_SKY;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_REPLACE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qTrue;
		R_PassStateBits (pass);
		break;

	default:
	case SHADER_BSP:
		if (surfParams > 0 && surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66|SHADER_SURF_WARP)) {
			shader->cullType = SHADER_CULL_FRONT;
			shader->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			shader->flags = SHADER_ENTITY_MERGABLE|SHADER_DEPTHWRITE;

			// Add flowing if it's got the flag
			if (surfParams & SHADER_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (shaderPass_t) + sizeof (tcMod_t), ri.shaderSysPool, 0);
				shader->passes = (shaderPass_t *)buffer;
				buffer += sizeof (shaderPass_t);
			}
			else {
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
			}
			shader->numPasses = 1;

			// Sort key
			if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66))
				shader->sortKey = SHADER_SORT_WATER;
			else
				shader->sortKey = SHADER_SORT_OPAQUE;

			pass = &shader->passes[0];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->blendSource = GL_SRC_ALPHA;
			pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
			pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_MODULATE;

			// Subdivide if warping
			if (surfParams & SHADER_SURF_WARP) {
				// FIXME: 128 is a hack, and is supposed to be 64 to match Quake2.
				// I changed it because of Gloom's map assault, which has a massive
				// entity warp brush, that manages to overflow the rendering backend.
				// Until multiple meshes are stored per surface, nothing can be done here.
				shader->flags |= SHADER_SUBDIVIDE|SHADER_NOMARK;
				shader->subdivide = 128;
				pass->tcGen = TC_GEN_WARP;
			}
			else {
				pass->tcGen = TC_GEN_BASE;
			}

			pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
			if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66)) {
				// FIXME: This caused problems with certain trans surfaces
				// Apparently Q2bsp generates vertices for both sides of trans surfaces?!
				//shader->cullType = SHADER_CULL_NONE;
				pass->flags |= SHADER_PASS_BLEND;
				pass->alphaGen.type = ALPHA_GEN_CONST;
				pass->alphaGen.args[0] = (surfParams & SHADER_SURF_TRANS33) ? 0.33f : 0.66f;
			}
			else {
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			}
			pass->canAccumulate = qFalse;
			R_PassStateBits (pass);

			// Add flowing if it's got the flag
			if (surfParams & SHADER_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		else if (surfParams > 0 && shader->surfParams & SHADER_SURF_LIGHTMAP) {
			shader->cullType = SHADER_CULL_FRONT;
			shader->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			shader->flags = SHADER_ENTITY_MERGABLE|SHADER_DEPTHWRITE;

			// Add flowing if it's got the flag
			if (surfParams & SHADER_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (shaderPass_t) * 2 + sizeof (tcMod_t), ri.shaderSysPool, 0);
				shader->passes = (shaderPass_t *)buffer;
				buffer += sizeof (shaderPass_t) * 2;
			}
			else {
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t) * 2, ri.shaderSysPool, 0);
			}
			shader->numPasses = 2;
			shader->sizeBase = 1;
			shader->sortKey = SHADER_SORT_OPAQUE;

			pass = &shader->passes[0];
			pass->animNames[pass->animNumNames++] = Mem_PoolStrDup ("$lightmap", ri.shaderSysPool, 0);
			pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
			pass->tcGen = TC_GEN_LIGHTMAP;
			pass->depthFunc = GL_LEQUAL;
			pass->blendSource = GL_ONE;
			pass->blendDest = GL_ZERO;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			pass->canAccumulate = qTrue;
			R_PassStateBits (pass);

			pass = &shader->passes[1];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
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
			if (surfParams & SHADER_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		else {
			shader->cullType = SHADER_CULL_FRONT;
			shader->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			shader->flags = SHADER_ENTITY_MERGABLE|SHADER_DEPTHWRITE;
			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & SHADER_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (shaderPass_t) + sizeof (tcMod_t), ri.shaderSysPool, 0);
				shader->passes = (shaderPass_t *)buffer;
				buffer += sizeof (shaderPass_t);
			}
			else {
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
			}
			shader->numPasses = 1;
			shader->sizeBase = 0;
			shader->sortKey = SHADER_SORT_OPAQUE;

			pass = &shader->passes[0];
			pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
			pass->animTexFlags[pass->animNumNames] = texFlags;
			pass->animImages[pass->animNumNames++] = image;
			pass->blendSource = GL_SRC_ALPHA;
			pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
			pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
			pass->tcGen = TC_GEN_BASE;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			pass->canAccumulate = qTrue;
			R_PassStateBits (pass);

			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & SHADER_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}
		}
		break;

	case SHADER_BSP_FLARE:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS|MF_STATIC_MESH;
		shader->flags = SHADER_FLARE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sizeBase = 0;
		shader->sortKey = SHADER_SORT_ADDITIVE;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
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

	case SHADER_BSP_VERTEX:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_COLORS|MF_TRNORMALS|MF_STATIC_MESH;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), ri.shaderSysPool, 0);
		shader->numPasses = 1;
		shader->sizeBase = 0;
		shader->sortKey = SHADER_SORT_OPAQUE;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qFalse;
		R_PassStateBits (pass);
		break;

	case SHADER_BSP_LM:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t) * 2, ri.shaderSysPool, 0);
		shader->numPasses = 2;
		shader->sizeBase = 1;
		shader->sortKey = SHADER_SORT_OPAQUE;

		pass = &shader->passes[0];
		pass->animNames[pass->animNumNames++] = Mem_PoolStrDup ("$lightmap", ri.shaderSysPool, 0);
		pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->depthFunc = GL_LEQUAL;
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ZERO;
		pass->blendMode = GL_REPLACE;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		pass->canAccumulate = qTrue;
		R_PassStateBits (pass);

		pass = &shader->passes[1];
		pass->animNames[pass->animNumNames] = Mem_PoolStrDup (fixedName, ri.shaderSysPool, 0);
		pass->animTexFlags[pass->animNumNames] = texFlags;
		pass->animImages[pass->animNumNames++] = image;
		pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
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
	R_ReadyShader (shader);
	return shader;
}

shader_t *R_RegisterPic (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_PIC, -1);
}

shader_t *R_RegisterPoly (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_POLY, -1);
}

shader_t *R_RegisterSkin (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_ALIAS, -1);
}

shader_t *R_RegisterSky (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_SKYBOX, -1);
}

shader_t *R_RegisterTexture (char *name, shSurfParams_t surfParams)
{
	return R_RegisterShader (name, qFalse, SHADER_BSP, surfParams);
}

shader_t *R_RegisterFlare (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_BSP_FLARE, -1);
}

shader_t *R_RegisterTextureLM (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_BSP_LM, -1);
}

shader_t *R_RegisterTextureVertex (char *name)
{
	return R_RegisterShader (name, qFalse, SHADER_BSP_VERTEX, -1);
}


/*
==================
R_EndShaderRegistration
==================
*/
void R_EndShaderRegistration (void)
{
	shader_t	*shader;
	uint32		i;

	for (i=0, shader=r_shaderList ; i<r_numShaders ; shader++, i++) {
		if (!(shader->flags & SHADER_NOFLUSH) && shader->touchFrame != ri.reg.registerFrame) {
			R_UntouchShader (shader);
			ri.reg.shadersReleased++;
			continue;	// Not touched
		}

		R_ReadyShader (shader);
		ri.reg.shadersTouched++;
	}
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
==================
R_ShaderList_f
==================
*/
static void R_ShaderList_f (void)
{
	shader_t		*shader;
	char			*wildCard;
	int				totPasses;
	int				totShaders;
	uint32			i;

	if (Cmd_Argc () == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	Com_Printf (0, "-------------- Loaded Shaders --------------\n");
	Com_Printf (0, "Base Sh#  PASS NAME\n");
	Com_Printf (0, "---- ---- ---- ----------------------------------\n");

	for (i=0, totShaders=0, totPasses=0, shader=r_shaderList ; i<r_numShaders ; shader++, i++) {
		if (!shader->name[0])
			continue;
		if (!Q_WildcardMatch (wildCard, shader->name, 1))
			continue;

		switch (shader->pathType) {
		case SHADER_PATHTYPE_INTERNAL:
			Com_Printf (0, "Intr ");
			break;

		case SHADER_PATHTYPE_BASEDIR:
			Com_Printf (0, "Base ");
			break;

		case SHADER_PATHTYPE_MODDIR:
			Com_Printf (0, "Mod  ");
			break;
		}

		Com_Printf (0, "%4d %4d %s\n", totShaders, shader->numPasses, shader->name);

		totPasses += shader->numPasses;
		totShaders++;
	}

	Com_Printf (0, "--------------------------------------------\n");
	Com_Printf (0, "Total matching shaders/passes: %d/%d\n", totShaders, totPasses);
	Com_Printf (0, "--------------------------------------------\n");
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

static void	*cmd_shaderList;

/*
==================
R_ShaderInit
==================
*/
void R_ShaderInit (void)
{
	char			*fileList[MAX_SHADERS];
	char			fixedName[MAX_QPATH];
	int				numFiles, i;
	shPathType_t	pathType;
	char			*name;
	uint32			initTime;

	initTime = Sys_UMilliseconds ();
	Com_Printf (0, "\n-------- Shader Initialization ---------\n");

	// Clear lists
	r_numShaders = 0;
	memset (&r_shaderList, 0, sizeof (shader_t) * MAX_SHADERS);
	memset (r_shaderHashTree, 0, sizeof (shader_t *) * MAX_SHADER_HASH);

	// Console commands
	cmd_shaderList	= Cmd_AddCommand ("shaderlist",		R_ShaderList_f,		"Prints to the console a list of loaded shaders");

	// Load scripts
	r_numShaderErrors = 0;
	r_numShaderWarnings = 0;
	numFiles = FS_FindFiles ("scripts", "*scripts/*.shd", "shd", fileList, MAX_SHADERS, qTrue, qFalse);
	for (i=0 ; i<numFiles ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), fileList[i]);

		// Check the path
		name = strstr (fixedName, "/scripts/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir shader?
		if (strstr (fileList[i], BASE_MODDIRNAME "/"))
			pathType = SHADER_PATHTYPE_BASEDIR;
		else
			pathType = SHADER_PATHTYPE_MODDIR;

		R_ParseShaderFile (name, pathType);
	}
	FS_FreeFileList (fileList, numFiles);

	// Shader counterparts
	r_cinShader = R_RegisterShader (ri.cinTexture->name, qTrue, SHADER_PIC, -1);
	r_noShader = R_RegisterShader (ri.noTexture->name, qTrue, SHADER_BSP, -1);
	r_noShaderLightmap = R_RegisterShader (ri.noTexture->name, qTrue, SHADER_BSP, SHADER_SURF_LIGHTMAP);
	r_noShaderSky = R_RegisterShader (ri.noTexture->name, qTrue, SHADER_SKYBOX, -1);
	r_whiteShader = R_RegisterShader (ri.whiteTexture->name, qTrue, SHADER_PIC, -1);
	r_blackShader = R_RegisterShader (ri.blackTexture->name, qTrue, SHADER_PIC, -1);

	r_cinShader->flags |= SHADER_NOFLUSH;
	r_noShader->flags |= SHADER_NOFLUSH;
	r_noShaderLightmap->flags |= SHADER_NOFLUSH;
	r_noShaderSky->flags |= SHADER_NOFLUSH;
	r_whiteShader->flags |= SHADER_NOFLUSH;
	r_blackShader->flags |= SHADER_NOFLUSH;

	Com_Printf (0, "----------------------------------------\n");

	// Check memory integrity
	Mem_CheckPoolIntegrity (ri.shaderSysPool);

	Com_Printf (0, "SHADERS - %i error(s), %i warning(s)\n", r_numShaderErrors, r_numShaderWarnings);
	Com_Printf (0, "%u shaders loaded in %ums\n", r_numShaders, Sys_UMilliseconds()-initTime);
	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
R_ShaderShutdown
==================
*/
void R_ShaderShutdown (void)
{
	uint32	size;

	Com_Printf (0, "Shader system shutdown:\n");

	// Remove commands
	Cmd_RemoveCommand ("shaderlist", cmd_shaderList);

	// Free all loaded shaders
	size = Mem_FreePool (ri.shaderSysPool);
	Com_Printf (0, "...releasing %u bytes...\n", size);
}
