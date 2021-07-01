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

#include "r_local.h"

#define MAX_SHADER_HASH			128

static shader_t			r_shaderList[MAX_SHADERS];
static shader_t			*r_shaderHashTree[MAX_SHADER_HASH];
static uInt				r_numShaders;

static uInt				r_numCurrPasses;
static shaderPass_t		r_currPasses[MAX_SHADER_PASSES];
static vertDeform_t		r_currDeforms[MAX_SHADER_DEFORMVS];
static tcMod_t			r_currTcMods[MAX_SHADER_PASSES][MAX_SHADER_TCMODS];

typedef struct shaderKey_s {
	char		*keyWord;
	qBool		(*func)(shader_t *shader, shaderPass_t *pass, char **token);
} shaderKey_t;

shader_t	*r_noShader;
shader_t	*r_noShaderLightmap;
shader_t	*r_whiteShader;
shader_t	*r_blackShader;

cVar_t *vid_gammapics;

/*
=============================================================================

	PASS PROCESSING

=============================================================================
*/

/*
==================
Shader_ParseBlendID
==================
*/
static inline uInt Shader_ParseBlendID (char *blend)
{
	if (!blend || !blend[0])
		return -1;

	// Check the length
	if (strlen (blend) < 4)
		return -1;

	// Switch'd up for tiny performance gain
	switch (blend[3]) {
	case 'D':
	case 'd':
		if (!Q_stricmp (blend, "GL_DST_ALPHA"))		return GL_DST_ALPHA;
		if (!Q_stricmp (blend, "GL_DST_COLOR"))		return GL_DST_COLOR;
		break;

	case 'O':
	case 'o':
		if (!Q_stricmp (blend, "GL_ONE"))					return GL_ONE;
		if (!Q_stricmp (blend, "GL_ONE_MINUS_DST_ALPHA"))	return GL_ONE_MINUS_DST_ALPHA;
		if (!Q_stricmp (blend, "GL_ONE_MINUS_DST_COLOR"))	return GL_ONE_MINUS_DST_COLOR;
		if (!Q_stricmp (blend, "GL_ONE_MINUS_SRC_ALPHA"))	return GL_ONE_MINUS_SRC_ALPHA;
		if (!Q_stricmp (blend, "GL_ONE_MINUS_SRC_COLOR"))	return GL_ONE_MINUS_SRC_COLOR;
		break;

	case 'S':
	case 's':
		if (!Q_stricmp (blend, "GL_SRC_ALPHA"))				return GL_SRC_ALPHA;
		if (!Q_stricmp (blend, "GL_SRC_ALPHA_SATURATE"))	return GL_SRC_ALPHA_SATURATE;
		if (!Q_stricmp (blend, "GL_SRC_COLOR"))				return GL_SRC_COLOR;
		break;

	case 'Z':
	case 'z':
		if (!Q_stricmp (blend, "GL_ZERO"))		return GL_ZERO;
		break;
	}

	return -1;
}


/*
==================
Shader_ParseString
==================
*/
static char *Shader_ParseString (char **token)
{
	char *str;

	if (!token || !(*token)) 
		return "";
	if (!**token || **token == '}')
		return "";

	str = Com_ParseExt (token, qFalse);
	return Q_strlwr (str);
}


/*
==================
Shader_ParseFloat
==================
*/
static float Shader_ParseFloat (char **token)
{
	if (!token || !(*token))
		return 0;
	if (!**token || **token == '}')
		return 0;

	return atof (Com_ParseExt (token, qFalse));
}


/*
==================
Shader_ParseFunc
==================
*/
static void Shader_ParseFunc (char **token, shaderFunc_t *func)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "sin"))
		func->type = SHADER_FUNC_SIN;
	if (!Q_stricmp (str, "triangle"))
		func->type = SHADER_FUNC_TRIANGLE;
	if (!Q_stricmp (str, "square"))
		func->type = SHADER_FUNC_SQUARE;
	if (!Q_stricmp (str, "sawtooth"))
		func->type = SHADER_FUNC_SAWTOOTH;
	if (!Q_stricmp (str, "inversesawtooth"))
		func->type = SHADER_FUNC_INVERSESAWTOOTH;
	if (!Q_stricmp (str, "noise"))
		func->type = SHADER_FUNC_NOISE;

	func->args[0] = Shader_ParseFloat (token);
	func->args[1] = Shader_ParseFloat (token);
	func->args[2] = Shader_ParseFloat (token);
	func->args[3] = Shader_ParseFloat (token);
}


/*
==================
Shader_ParseVector
==================
*/
static void Shader_ParseVector (char **token, float *vec, uInt size)
{
	qBool	inBrackets;
	char	*str;
	uInt	i;

	if (!size) {
		return;
	}
	else if (size == 1) {
		Shader_ParseFloat (token);
		return;
	}

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "(")) {
		inBrackets = qTrue;
		str = Shader_ParseString (token);
	}
	else if (str[0] == '(') {
		inBrackets = qTrue;
		str = &str[1];
	}
	else {
		inBrackets = qFalse;
	}

	vec[0] = atof (str);
	for (i=1 ; i<size-1 ; i++)
		vec[i] = Shader_ParseFloat (token);

	str = Shader_ParseString (token);
	if (!str[0]) {
		vec[i] = 0;
	}
	else if (str[strlen(str)-1] == ')') {
		str[strlen(str)-1] = 0;
		vec[i] = atof (str);
	}
	else {
		vec[i] = atof (str);
		if (inBrackets)
			Shader_ParseString (token);
	}
}


/*
==================
Shader_SkipLine
==================
*/
static void Shader_SkipLine (char **token)
{
	char *str;

	while (token) {
		str = Com_ParseExt (token, qFalse);
		if (!str[0])
			return;
	}
}


/*
==================
Shader_SkipBlock
==================
*/
static void Shader_SkipBlock (char **token)
{
	char	*str;
    int		braceCount;

    // Opening brace
	str = Com_ParseExt (token, qTrue);
	if (str[0] != '{')
		return;

	for (braceCount=1 ; braceCount>0 ; ) {
		str = Com_ParseExt (token, qTrue);
		if (!str[0])
			return;
		else if (str[0] == '{')
			braceCount++;
		else if (str[0] == '}')
			braceCount--;
	}
}

// ==========================================================================

static qBool ShaderPass_AnimFrequency (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->animFPS = Shader_ParseFloat (token);
	pass->flags |= SHADER_PASS_ANIMMAP;

	return qTrue;
}

static qBool ShaderPass_Map (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	if (pass->numNames == MAX_SHADER_ANIM_FRAMES)
		Com_Error (ERR_DROP, "Too many shader passes on shader '%s'\n", shader->name);

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "$lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_LIGHTMAP;
	}
	else {
		pass->tcGen = TC_GEN_BASE;
		if (!Q_stricmp (str, "$rgb")) {
			pass->texFlags |= IF_NOALPHA;
			str = Shader_ParseString (token);
		}
		else if (!Q_stricmp (str, "$alpha")) {
			pass->texFlags |= IF_NORGB;
			str = Shader_ParseString (token);
		}
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_CubeMap (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	if (pass->numNames == MAX_SHADER_ANIM_FRAMES)
		Com_Error (ERR_DROP, "Too many shader passes on shader '%s'\n", shader->name);

	str = Shader_ParseString (token);
	if (glConfig.extArbTexCubeMap) {
		pass->flags |= SHADER_PASS_CUBEMAP;
		pass->texFlags |= IT_CUBEMAP|IF_CLAMP;

		pass->tcGen = TC_GEN_REFLECTION;
	}
	else {
		pass->tcGen = TC_GEN_ENVIRONMENT;
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_NormalMap (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	if (pass->numNames == MAX_SHADER_ANIM_FRAMES)
		Com_Error (ERR_DROP, "Too many shader passes on shader '%s'\n", shader->name);

	str = Shader_ParseString (token);

	if (!Q_stricmp (str, "$heightmap")) {
		pass->texFlags |= IT_HEIGHTMAP;
		pass->bumpScale = Shader_ParseFloat (token);
		str = Shader_ParseString (token);
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));

	pass->tcGen = TC_GEN_BASE;
	pass->flags &= ~SHADER_PASS_LIGHTMAP;

	if (!glConfig.extTexEnvDot3) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "WARNING: Shader '%s' has a normalmap stage while the dot3 extension is not available\n", shader->name);
		return qFalse;
	}

	pass->blendMode = GL_DOT3_RGB_ARB;

	str = Shader_ParseString (token);
	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_FragmentProgram (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	if (!glConfig.extFragmentProgram) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has a fragProg shader pass when the extension not available\n", shader->name);
		return qFalse;
	}

	str = Shader_ParseString (token);
	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= SHADER_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool ShaderPass_VertexProgram (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	if (!glConfig.extVertexProgram) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has a vertProg shader pass when the extension not available\n", shader->name);
		return qFalse;
	}

	str = Shader_ParseString (token);
	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	pass->flags |= SHADER_PASS_VERTEXPROGRAM;
	return qTrue;
}

static qBool ShaderPass_RGBGen (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "identitylighting")) {
		pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
		return qTrue;
	}
	else if (!Q_stricmp (str, "identity")) {
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		return qTrue;
	}
	else if (!Q_stricmp (str, "wave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		pass->rgbGen.args[0] = 1.0f;
		pass->rgbGen.args[1] = 1.0f;
		pass->rgbGen.args[2] = 1.0f;
		Shader_ParseFunc (token, &pass->rgbGen.func);
		return qTrue;
	}
	else if (!Q_stricmp (str, "colorwave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		Shader_ParseVector (token, pass->rgbGen.args, 3);
		Shader_ParseFunc (token, &pass->rgbGen.func);
		return qTrue;
	}
	else if (!Q_stricmp (str, "entity")) {
		pass->rgbGen.type = RGB_GEN_ENTITY;
		return qTrue;
	}
	else if (!Q_stricmp (str, "oneMinusEntity")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_ENTITY;
		return qTrue;
	}
	else if (!Q_stricmp (str, "vertex")) {
		pass->rgbGen.type = RGB_GEN_VERTEX;
		return qTrue;
	}
	else if (!Q_stricmp (str, "oneMinusVertex")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_VERTEX;
		return qTrue;
	}
	else if (!Q_stricmp (str, "oneMinusExactVertex")) {
		pass->rgbGen.type = RGB_GEN_ONE_MINUS_EXACT_VERTEX;
		return qTrue;
	}
	else if (!Q_stricmp (str, "lightingDiffuse")) {
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		return qTrue;
	}
	else if (!Q_stricmp (str, "exactVertex")) {
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		return qTrue;
	}
	else if (!Q_stricmp (str, "const") || !Q_stricmp (str, "constant")) {
		float	div;
		vec3_t	color;

		if (intensity->integer > 0)
			div = 1.0f / pow (2, intensity->integer / 2);
		else
			div = 1.0f;

		pass->rgbGen.type = RGB_GEN_CONST;
		Shader_ParseVector (token, color, 3);
		ColorNormalize (color, pass->rgbGen.args);
		VectorScale (pass->rgbGen.args, div, pass->rgbGen.args);

		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid rgbGen value: '%s'\n", shader->name, str);
	return qFalse;
}

static qBool ShaderPass_AlphaGen (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "identity")) {
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		return qTrue;
	}
	else if (!Q_stricmp (str, "const") || !Q_stricmp (str, "constant")) {
		pass->alphaGen.type = ALPHA_GEN_CONST;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (token));
		return qTrue;
	}
	else if (!Q_stricmp (str, "wave")) {
		pass->alphaGen.type = ALPHA_GEN_WAVE;
		Shader_ParseFunc (token, &pass->alphaGen.func);
		return qTrue;
	}
	else if (!Q_stricmp (str, "portal")) {
		pass->alphaGen.type = ALPHA_GEN_PORTAL;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (token));
		if (!pass->alphaGen.args[0])
			pass->alphaGen.args[0] = 256;
		pass->alphaGen.args[0] = 1.0f / pass->alphaGen.args[0];
		return qTrue;
	}
	else if (!Q_stricmp (str, "vertex")) {
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		return qTrue;
	}
	else if (!Q_stricmp (str, "entity")) {
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		return qTrue;
	}
	else if (!Q_stricmp (str, "lightingSpecular")) {
		pass->alphaGen.type = ALPHA_GEN_SPECULAR;
		return qTrue;
	}
	else if (!Q_stricmp (str, "dot")) {
		pass->alphaGen.type = ALPHA_GEN_DOT;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (token));
		pass->alphaGen.args[1] = fabs (Shader_ParseFloat (token));
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}
	else if (!Q_stricmp (str, "oneMinusDot")) {
		pass->alphaGen.type = ALPHA_GEN_ONE_MINUS_DOT;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (token));
		pass->alphaGen.args[1] = fabs (Shader_ParseFloat (token));
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid alphaGen value: '%s'\n", shader->name, str);
	return qFalse;
}

static qBool ShaderPass_AlphaFunc (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "gt0")) {
		pass->alphaFunc = ALPHA_FUNC_GT0;
		return qTrue;
	}
	else if( !Q_stricmp (str, "lt128")) {
		pass->alphaFunc = ALPHA_FUNC_LT128;
		return qTrue;
	}
	else if( !Q_stricmp (str, "ge128")) {
		pass->alphaFunc = ALPHA_FUNC_GE128;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid alphaFunc value: '%s'\n", shader->name, str);
	return qFalse;
}

static qBool ShaderPass_BlendFunc (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "add")) {
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ONE;
	}
	else if (!Q_stricmp (str, "blend")) {
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!Q_stricmp (str, "filter")
	|| !Q_stricmp (str, "lightmap")) {
		pass->blendSource = GL_ZERO;
		pass->blendDest = GL_SRC_COLOR;
	}
	else {
		pass->blendSource = Shader_ParseBlendID (str);
		if (pass->blendSource == -1) {
			Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid blend source: '%s'\n", shader->name, str);
			return qFalse;
		}

		str = Shader_ParseString (token);
		pass->blendDest = Shader_ParseBlendID (str);
		if (pass->blendDest == -1) {
			Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid blend destination: '%s'\n", shader->name, str);
			return qFalse;
		}
	}

	pass->flags |= SHADER_PASS_BLEND;
	return qTrue;
}

static qBool ShaderPass_DepthFunc (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "equal")) {
		pass->depthFunc = GL_EQUAL;
		return qTrue;
	}
	else if (!Q_stricmp (str, "lequal")) {
		pass->depthFunc = GL_LEQUAL;
		return qTrue;
	}
	else if (!Q_stricmp (str, "gequal")) {
		pass->depthFunc = GL_GEQUAL;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid depthFunc value: '%s\n", shader->name, str);
	return qFalse;
}

static qBool ShaderPass_TcGen (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "base")) {
		pass->tcGen = TC_GEN_BASE;
		return qTrue;
	}
	else if (!Q_stricmp (str, "lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		return qTrue;
	}
	else if (!Q_stricmp (str, "environment")) {
		pass->tcGen = TC_GEN_ENVIRONMENT;
		return qTrue;
	}
	else if (!Q_stricmp (str, "vector")) {
		pass->tcGen = TC_GEN_VECTOR;
		Shader_ParseVector (token, pass->tcGenVec[0], 4);
		Shader_ParseVector (token, pass->tcGenVec[1], 4);
		return qTrue;
	}
	else if (!Q_stricmp (str, "reflection")) {
		pass->tcGen = TC_GEN_REFLECTION;
		return qTrue;
	}
	else if (!Q_stricmp (str, "warp")) {
		pass->tcGen = TC_GEN_WARP;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid tcGen value: '%s\n", shader->name, str);
	return qFalse;
}

static qBool ShaderPass_TcMod (shader_t *shader, shaderPass_t *pass, char **token)
{
	tcMod_t	*tcMod;
	char	*str;
	float	val;
	int		i;

	if (pass->numTCMods >= MAX_SHADER_TCMODS) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has too many tcMods\n", shader->name);
		return qFalse;
	}

	tcMod = &r_currTcMods[r_numCurrPasses][r_currPasses[r_numCurrPasses].numTCMods];

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "rotate")) {
		val = Shader_ParseFloat (token);
		tcMod->args[0] = -val / 360.0f;
		if (!tcMod->args[0]) {
			Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid tcMod rotate arg: '%f\n", shader->name, val);
			return qFalse;
		}
		tcMod->type = TC_MOD_ROTATE;
	}
	else if (!Q_stricmp (str, "scale")) {
		Shader_ParseVector (token, tcMod->args, 2);
		tcMod->type = TC_MOD_SCALE;
	}
	else if (!Q_stricmp (str, "scroll")) {
		Shader_ParseVector (token, tcMod->args, 2);
		tcMod->type = TC_MOD_SCROLL;
	}
	else if (!Q_stricmp (str, "stretch")) {
		shaderFunc_t	func;

		Shader_ParseFunc (token, &func);

		tcMod->args[0] = func.type;
		for (i=1 ; i<5 ; i++) {
			tcMod->args[i] = func.args[i-1];
		}
		tcMod->type = TC_MOD_STRETCH;
	}
	else if (!Q_stricmp (str, "transform")) {
		Shader_ParseVector (token, tcMod->args, 6);
		tcMod->args[4] = tcMod->args[4] - floor(tcMod->args[4]);
		tcMod->args[5] = tcMod->args[5] - floor(tcMod->args[5]);
		tcMod->type = TC_MOD_TRANSFORM;
	}
	else if (!Q_stricmp (str, "turb")) {
		Shader_ParseVector (token, tcMod->args, 4);
		tcMod->type = TC_MOD_TURB;
	}
	else {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid tcMod value: '%s\n", shader->name, str);
		return qFalse;
	}

	r_currPasses[r_numCurrPasses].numTCMods++;
	return qTrue;
}


static qBool ShaderPass_MaskColor (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->maskRed = qTrue;
	pass->maskGreen = qTrue;
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskRed (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->maskRed = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskGreen (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->maskGreen = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskBlue (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskAlpha (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->maskAlpha = qTrue;
	return qTrue;
}

static qBool ShaderPass_NoGamma (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->texFlags |= IF_NOGAMMA;
	return qTrue;
}

static qBool ShaderPass_NoIntens (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->texFlags |= IF_NOINTENS;
	return qTrue;
}

static qBool ShaderPass_NoMipmap (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "linear")) {
		pass->texFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}
	else if (!Q_stricmp (str, "nearest")) {
		pass->texFlags |= IF_NOMIPMAP_NEAREST;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has noMipMap but no type specified, assuming linear\n", shader->name);
	pass->texFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool ShaderPass_NoPicmip (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->texFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool ShaderPass_NoCompress (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->texFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool ShaderPass_ClampCoords (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->texFlags |= IF_CLAMP;
	return qTrue;
}

static qBool ShaderPass_SizeBase (shader_t *shader, shaderPass_t *pass, char **token)
{
	shader->sizeBase = r_numCurrPasses;
	return qTrue;
}

static qBool ShaderPass_Detail (shader_t *shader, shaderPass_t *pass, char **token)
{
	pass->flags |= SHADER_PASS_DETAIL;
	return qTrue;
}

// ==========================================================================

static shaderKey_t shaderPassKeys[] = {
	{ "animfrequency",	&ShaderPass_AnimFrequency	},
	{ "map",			&ShaderPass_Map				},
	{ "cubemap",		&ShaderPass_CubeMap			},
	{ "normalmap",		&ShaderPass_NormalMap		},

	{ "fragProg",		&ShaderPass_FragmentProgram	},
	{ "vertProg",		&ShaderPass_VertexProgram	},

	{ "rgbgen",			&ShaderPass_RGBGen			},
	{ "alphagen",		&ShaderPass_AlphaGen		},

	{ "alphafunc",		&ShaderPass_AlphaFunc		},
	{ "blendfunc",		&ShaderPass_BlendFunc		},
	{ "depthfunc",		&ShaderPass_DepthFunc		},

	{ "tcclamp",		&ShaderPass_ClampCoords		},
	{ "tcgen",			&ShaderPass_TcGen			},
	{ "tcmod",			&ShaderPass_TcMod			},

	{ "maskcolor",		&ShaderPass_MaskColor		},
	{ "maskred",		&ShaderPass_MaskRed			},
	{ "maskgreen",		&ShaderPass_MaskGreen		},
	{ "maskblue",		&ShaderPass_MaskBlue		},
	{ "maskalpha",		&ShaderPass_MaskAlpha		},

	{ "nogamma",		&ShaderPass_NoGamma			},
	{ "nointens",		&ShaderPass_NoIntens		},
	{ "nomipmap",		&ShaderPass_NoMipmap		},
	{ "nopicmip",		&ShaderPass_NoPicmip		},
	{ "nocompress",		&ShaderPass_NoCompress		},

	{ "sizebase",		&ShaderPass_SizeBase		},

	{ "detail",			&ShaderPass_Detail			},

	{ NULL,				NULL						}
};

static const int r_numShaderPassKeys = sizeof (shaderPassKeys) / sizeof (shaderPassKeys[0]) - 1;

/*
=============================================================================

	SHADER PROCESSING

=============================================================================
*/

static qBool ShaderBase_Cull (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "disable")
	|| !Q_stricmp (str, "none")
	|| !Q_stricmp (str, "twosided")) {
		shader->cullType = SHADER_CULL_NONE;
		return qTrue;
	}
	else if (!Q_stricmp (str, "back")
	|| !Q_stricmp (str, "backside")
	|| !Q_stricmp (str, "backsided")) {
		shader->cullType = SHADER_CULL_BACK;
		return qTrue;
	}
	else if (!Q_stricmp (str, "front")
	|| !Q_stricmp (str, "frontside")
	|| !Q_stricmp (str, "frontsided")) {
		shader->cullType = SHADER_CULL_FRONT;
		return qTrue;
	}

	Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid cull value: '%s'\n", shader->name, str);
	return qFalse;
}

static qBool ShaderBase_DeformVertexes (shader_t *shader, shaderPass_t *pass, char **token)
{
	vertDeform_t	*deformv;
	char			*str;

	if (shader->numDeforms == MAX_SHADER_DEFORMVS) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has too many deforms\n", shader->name);
		return qFalse;
	}

	deformv = &r_currDeforms[shader->numDeforms];

	str = Shader_ParseString (token);
	if (!Q_stricmp (str, "wave")) {
		deformv->type = DEFORMV_WAVE;
		deformv->args[0] = Shader_ParseFloat (token);
		if (deformv->args[0])
			deformv->args[0] = 1.0f / deformv->args[0];
		Shader_ParseFunc (token, &deformv->func);
	}
	else if (!Q_stricmp (str, "normal")) {
		deformv->type = DEFORMV_NORMAL;
		deformv->args[0] = Shader_ParseFloat (token);
		deformv->args[1] = Shader_ParseFloat (token);
	}
	else if (!Q_stricmp (str, "move")) {
		deformv->type = DEFORMV_MOVE;
		Shader_ParseVector (token, deformv->args, 3);
		Shader_ParseFunc (token, &deformv->func);
	}
	else if (!Q_stricmp (str, "autosprite")) {
		deformv->type = DEFORMV_AUTOSPRITE;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if (!Q_stricmp (str, "autosprite2")) {
		deformv->type = DEFORMV_AUTOSPRITE2;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if (!Q_stricmp (str, "projectionShadow")) {
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	}
	else if (!Q_stricmp (str, "autoparticle")) {
		deformv->type = DEFORMV_AUTOPARTICLE;
	}
	else {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid deformVertexes value: '%s'\n", shader->name, str);
		return qFalse;
	}

	shader->numDeforms++;
	return qTrue;
}

static qBool ShaderBase_Subdivide (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;
	int		size = 8;

	str = Shader_ParseString (token);

	// No value!
	if (!str[0]) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' is missing a subdivide size\n", shader->name);
		return qFalse;
	}

	shader->subdivide = atoi (str);
	if (shader->subdivide < 8 || shader->subdivide > 256) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an out of bounds subdivide size: '%i', assuming 64", shader->name, shader->subdivide);
		shader->subdivide = 64;
	}
	else {
		// Force power of two
		while (size <= shader->subdivide)
			size <<= 1;

		size >>= 1;

		if (size != shader->subdivide) {
			Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' subdivide size '%i' is not a power of two, forcing: '%i'\n", shader->name, shader->subdivide, size);
			shader->subdivide = size;
		}
	}

	shader->flags |= SHADER_SUBDIVIDE;
	return qTrue;
}

static qBool ShaderBase_EntityMergable (shader_t *shader, shaderPass_t *pass, char **token)
{
	shader->flags |= SHADER_ENTITY_MERGABLE;
	return qTrue;
}

static qBool ShaderBase_DepthRange (shader_t *shader, shaderPass_t *pass, char **token)
{
    shader->flags |= SHADER_DEPTHRANGE;

	shader->depthNear = Shader_ParseFloat (token);
	shader->depthFar = Shader_ParseFloat (token);
	return qTrue;
}

static qBool ShaderBase_DepthWrite (shader_t *shader, shaderPass_t *pass, char **token)
{
    shader->flags |= SHADER_DEPTHWRITE;
	return qTrue;
}

static qBool ShaderBase_PolygonOffset (shader_t *shader, shaderPass_t *pass, char **token)
{
	shader->offsetFactor = Shader_ParseFloat (token);
	if (!shader->offsetFactor) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid offset factor value: '%i'\n", shader->name, shader->offsetFactor);
		return qFalse;
	}

	shader->offsetUnits = Shader_ParseFloat (token);
	if (!shader->offsetUnits) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid offset units value: '%i'\n", shader->name, shader->offsetUnits);
		return qFalse;
	}

	shader->flags |= SHADER_POLYGONOFFSET;
	return qTrue;
}

static qBool ShaderBase_NoMark (shader_t *shader, shaderPass_t *pass, char **token)
{
	shader->flags |= SHADER_NOMARK;
	return qTrue;
}

static qBool ShaderBase_NoShadow (shader_t *shader, shaderPass_t *pass, char **token)
{
	shader->flags |= SHADER_NOSHADOW;
	return qTrue;
}

static qBool ShaderBase_SortKey (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);

	if (!Q_stricmp (str, "sky"))
		shader->sortKey = SHADER_SORT_SKY;
	else if (!Q_stricmp (str, "opaque"))
		shader->sortKey = SHADER_SORT_OPAQUE;
	else if (!Q_stricmp (str, "decal"))
		shader->sortKey = SHADER_SORT_DECAL;
	else if (!Q_stricmp (str, "seethrough"))
		shader->sortKey = SHADER_SORT_SEETHROUGH;
	else if (!Q_stricmp (str, "banner"))
		shader->sortKey = SHADER_SORT_BANNER;
	else if (!Q_stricmp (str, "underwater"))
		shader->sortKey = SHADER_SORT_UNDERWATER;
	else if (!Q_stricmp (str, "entity"))
		shader->sortKey = SHADER_SORT_ENTITY;
	else if (!Q_stricmp (str, "particle"))
		shader->sortKey = SHADER_SORT_PARTICLE;
	else if (!Q_stricmp (str, "water")
	|| !Q_stricmp (str, "glass"))
		shader->sortKey = SHADER_SORT_WATER;
	else if (!Q_stricmp (str, "additive"))
		shader->sortKey = SHADER_SORT_ADDITIVE;
	else if (!Q_stricmp (str, "nearest"))
		shader->sortKey = SHADER_SORT_NEAREST;
	else if (!Q_stricmp (str, "postProcess")
	|| !Q_stricmp (str, "post")) {
		shader->sortKey = SHADER_SORT_POSTPROCESS;
	}
	else {
		shader->sortKey = atoi (str);

		if (shader->sortKey < 0 || shader->sortKey >= SHADER_SORT_MAX) {
			Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid sortKey value: '%i'", shader->name, shader->sortKey);
			return qFalse;
		}
	}

	return qTrue;
}

static qBool ShaderBase_SurfParams (shader_t *shader, shaderPass_t *pass, char **token)
{
	char	*str;

	str = Shader_ParseString (token);

	if (shader->surfParams < 0)
		shader->surfParams = 0;

	if (!Q_stricmp (str, "flowing"))
		shader->surfParams |= SHADER_SURF_FLOWING;
	else if (!Q_stricmp (str, "trans33"))
		shader->surfParams |= SHADER_SURF_TRANS33;
	else if (!Q_stricmp (str, "trans66"))
		shader->surfParams |= SHADER_SURF_TRANS66;
	else if (!Q_stricmp (str, "warp"))
		shader->surfParams |= SHADER_SURF_WARP;
	else if (!Q_stricmp (str, "lightmap"))
		shader->surfParams |= SHADER_SURF_LIGHTMAP;
	else {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid surfParam value: '%s'", shader->name, str);
		return qFalse;
	}

	return qTrue;
}

// ==========================================================================

static shaderKey_t shaderBaseKeys[] = {
	{ "cull",			&ShaderBase_Cull			},
    { "deformvertexes",	&ShaderBase_DeformVertexes	},
	{ "entitymergable",	&ShaderBase_EntityMergable	},
	{ "subdivide",		&ShaderBase_Subdivide		},
	{ "depthrange",		&ShaderBase_DepthRange		},
	{ "depthwrite",		&ShaderBase_DepthWrite		},
	{ "polygonoffset",	&ShaderBase_PolygonOffset	},

	{ "nomark",			&ShaderBase_NoMark			},
	{ "noshadow",		&ShaderBase_NoShadow		},

	{ "sortkey",		&ShaderBase_SortKey			},
	{ "surfparam",		&ShaderBase_SurfParams		},

	{ NULL,				NULL						}
};

static const int r_numShaderBaseKeys = sizeof (shaderBaseKeys) / sizeof (shaderBaseKeys[0]) - 1;

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
static shader_t *R_FindShader (char *name, int surfParams)
{
	shader_t	*shader, *bestMatch;
	char		tempName[MAX_QPATH];
	uLong		hashValue;

	if (!name)
		return NULL;

	// Copy, fix, strip
	Q_strncpyz (tempName, name, sizeof (tempName));
	Q_FixPathName (tempName, sizeof (tempName), 3, qTrue);

	hashValue = CalcHash (tempName, MAX_SHADER_HASH);
	bestMatch = NULL;

	// Look for it
	for (shader=r_shaderHashTree[hashValue] ; shader ; shader=shader->hashNext) {
		if (shader->surfParams != surfParams)
			continue;
		if (Q_stricmp (shader->name, tempName))
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
static shader_t *R_NewShader (char *name, byte pathType)
{
	shader_t	*shader;

	// Select shader
	if (r_numShaders >= MAX_SHADERS)
		Com_Error (ERR_DROP, "R_NewShader: MAX_SHADERS");
	shader = &r_shaderList[r_numShaders];
	memset (shader, 0, sizeof (*shader));

	// Clear passes
	r_numCurrPasses = 0;
	memset (&r_currPasses, 0, sizeof (r_currPasses));

	// Copy, fix, strip
	Q_strncpyz (shader->name, name, sizeof (shader->name));
	Q_FixPathName (shader->name, sizeof (shader->name), 3, qTrue);

	// Do not ready until registered
	shader->touchFrame = 0;

	// Defaults
	shader->cullType = SHADER_CULL_FRONT;
	shader->features = MF_NONE;
	shader->flags = 0;
	shader->numDeforms = 0;
	shader->numPasses = 0;
	shader->pathType = pathType;
	shader->sizeBase = 0;
	shader->sortKey = 0;
	shader->subdivide = 0;
	shader->surfParams = -1;

	shader->deforms = NULL;
	shader->passes = NULL;

	// Link it into the hash list
	shader->hashValue = CalcHash (shader->name, MAX_SHADER_HASH);
	shader->hashNext = r_shaderHashTree[shader->hashValue];
	r_shaderHashTree[shader->hashValue] = shader;

	r_numShaders++;

	return shader;
}


/*
==================
R_FinishShader
==================
*/
static void R_ShaderFeatures (shader_t *shader)
{
	shaderPass_t	*pass;
	qBool			trNormals;
	int				i;

	if (shader->numDeforms)
		shader->features |= MF_DEFORMVS;

	// Determine if triangle normals are necessary
	trNormals = qTrue;
	for (i=0 ; i<shader->numDeforms ; i++) {
		switch (shader->deforms[i].type) {
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

	for (i=0, pass=shader->passes ; i<shader->numPasses ; i++, pass++) {
		// normalMap requirements
		if (pass->blendMode == GL_DOT3_RGB_ARB)
			shader->features |= MF_NORMALS|MF_STVECTORS;

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
static void R_FinishShader (shader_t *shader)
{
	shaderPass_t	*pass;
	int				size, i;
	byte			*buffer;

	shader->numPasses = r_numCurrPasses;

	// Allocate full block
	size = shader->numDeforms * sizeof (vertDeform_t) + shader->numPasses * sizeof (shaderPass_t);
	for (i=0 ; i<shader->numPasses ; i++) {
		size += r_currPasses[i].numTCMods * sizeof (tcMod_t);
	}

	// Fill passes
	buffer = Mem_PoolAlloc (size, MEMPOOL_SHADERSYS, 0);
	shader->passes = (shaderPass_t *)buffer;
	memcpy (shader->passes, r_currPasses, shader->numPasses * sizeof (shaderPass_t));

	// Fill tcMods
	buffer += shader->numPasses * sizeof (shaderPass_t);
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		if (!r_currPasses[i].numTCMods)
			continue;

		pass->tcMods = (tcMod_t *)buffer;
		pass->numTCMods = r_currPasses[i].numTCMods;
		memcpy (pass->tcMods, r_currTcMods[i], pass->numTCMods * sizeof (tcMod_t));
		buffer += pass->numTCMods * sizeof (tcMod_t);
	}

	// Fill deforms
	if (shader->numDeforms) {
		shader->deforms = (vertDeform_t *)buffer;
		memcpy (shader->deforms, r_currDeforms, shader->numDeforms * sizeof (vertDeform_t));
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
		if (!shader->sortKey) {
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
		int				j;

		// Keep rgbGen/alphaGen in check
		for (j=0, sp=shader->passes ; j<shader->numPasses ; sp++, j++) {
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
	}

	// Calculate total component masks (for faster comparisons later on)
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++)
		pass->totalMask = pass->maskRed + pass->maskGreen + pass->maskBlue + pass->maskAlpha;

	// Check sort
	if (!shader->sortKey)
		shader->sortKey = SHADER_SORT_OPAQUE;

	// Check sizeBase
	if (shader->sizeBase >= shader->numPasses) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has an invalid sizeBase value of: '%i', forcing: 0\n", shader->name, shader->sizeBase);
		shader->sizeBase = 0;
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
static shaderPass_t *R_NewPass (shader_t *shader)
{
	shaderPass_t	*pass;

	if (r_numCurrPasses >= MAX_SHADER_PASSES) {
		Com_Printf (0, S_COLOR_YELLOW "WARNING: Shader '%s' has too many passes\n", shader->name);
		return NULL;
	}

	// Pass
	pass = &r_currPasses[r_numCurrPasses];
	memset (pass, 0, sizeof (shaderPass_t));

	pass->alphaFunc = ALPHA_FUNC_NONE;
	pass->bumpScale = 0;
    pass->depthFunc = GL_LEQUAL;
    pass->rgbGen.type = RGB_GEN_UNKNOWN;
	pass->alphaGen.type = ALPHA_GEN_UNKNOWN;
	pass->totalMask = 0;
	pass->maskRed = qFalse;
	pass->maskGreen = qFalse;
	pass->maskBlue = qFalse;
	pass->maskAlpha = qFalse;
	pass->tcGen = TC_GEN_BASE;
	pass->texFlags = 0;

	return pass;
}


/*
==================
R_FinishPass
==================
*/
void R_FinishPass (shader_t *shader, shaderPass_t *pass)
{
	r_numCurrPasses++;

	if (pass->blendSource == GL_ONE && pass->blendDest == GL_ZERO)
		shader->flags |= SHADER_DEPTHWRITE;

	switch (pass->rgbGen.type) {
		case RGB_GEN_IDENTITY_LIGHTING:
		case RGB_GEN_IDENTITY:
		case RGB_GEN_CONST:
		case RGB_GEN_COLORWAVE:
		case RGB_GEN_ENTITY:
		case RGB_GEN_ONE_MINUS_ENTITY:
		case RGB_GEN_UNKNOWN:	// Assume RGB_GEN_IDENTITY or RGB_GEN_IDENTITY_LIGHTING
			switch (pass->alphaGen.type) {
				case ALPHA_GEN_UNKNOWN:
				case ALPHA_GEN_IDENTITY:
				case ALPHA_GEN_CONST:
				case ALPHA_GEN_WAVE:
				case ALPHA_GEN_ENTITY:
					pass->flags |= SHADER_PASS_NOCOLORARRAY;
					break;
				default:
					break;
			}

			break;
		default:
			break;
	}

	// Blend mode
	if (!pass->animFrames[0] && !(pass->flags & SHADER_PASS_LIGHTMAP)) {
		pass->blendMode = 0;
		return;
	}

	if (!(pass->flags & SHADER_PASS_BLEND) && (glConfig.extArbMultitexture || glConfig.extSGISMultiTexture)) {
		if (pass->rgbGen.type == RGB_GEN_IDENTITY && pass->alphaGen.type == ALPHA_GEN_IDENTITY) {
			pass->blendMode = GL_REPLACE;
		}
		else {
			pass->blendSource = GL_ONE;
			pass->blendDest = GL_ZERO;
			pass->blendMode = GL_MODULATE;
		}
		return;
	}

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
}


/*
==================
R_LoadShaderFile
==================
*/
static qBool R_ShaderParseTok (shader_t *shader, shaderPass_t *pass, shaderKey_t *keys, char *token, char **ptr)
{
	shaderKey_t	*key;

	for (key=keys ; key->keyWord ; key++) {
		if (Q_stricmp (key->keyWord, token))
			continue;

		if (!key->func (shader, pass, ptr))
			break;

		return qTrue;
	}

	// Not found, skip the line
	Shader_SkipLine (ptr);
	return qFalse;
}
static void R_LoadShaderFile (char *name, byte pathType)
{
	char			*fileBuffer, *buf;
	char			fixedName[MAX_QPATH];
	uInt			fileLen;
	qBool			inShader;
	qBool			inPass;
	char			*ptr, *token;
	shader_t		*shader;
	shaderPass_t	*pass;
	int				numSlashes, i;

	if (!name)
		return;

	// Fix the path
	Q_strncpyz (fixedName, name, sizeof (fixedName));
	Q_FixPathName (fixedName, sizeof (fixedName), 3, qFalse);

	// Check for recursion
	for (numSlashes=0, i=0 ; fixedName[i] ; i++) {
		switch (fixedName[i]) {
		case '/':
			numSlashes++;
			break;
		}
	}
	if (numSlashes > 1)
		return;

	// Load the file
	fileLen = FS_LoadFile (fixedName, (void **)&fileBuffer);
	if (!fileBuffer || fileLen <= 0) {
		Com_Printf (0, "..." S_COLOR_YELLOW "WARNING: can't load '%s' -- %s\n", fixedName, !fileBuffer ? "unable to open" : "no data");
		return;
	}

	Com_Printf (0, "...loading '%s'\n", fixedName);

	buf = (char *)Mem_PoolAlloc (fileLen+2, MEMPOOL_SHADERSYS, 0);
	memcpy (buf, fileBuffer, fileLen);
	buf[fileLen] = '\n';

	FS_FreeFile (fileBuffer);

	// Start parsing
	inShader = qFalse;
	inPass = qFalse;

	shader = NULL;

	ptr = buf;
	token = Com_ParseExt (&ptr, qTrue);

	while (ptr) {
		if (!token[0])
			break;

		if (inShader) {
			switch (token[0]) {
			case '{':
				if (!inPass) {
					inPass = qTrue;
					pass = R_NewPass (shader);
				}
				break;

			case '}':
				if (inPass) {
					inPass = qFalse;
					R_FinishPass (shader, pass);
				}
				else {
					inShader = qFalse;
					R_FinishShader (shader);
				}
				break;

			default:
				if (inPass)
					R_ShaderParseTok (shader, pass, shaderPassKeys, token, &ptr);
				else
					R_ShaderParseTok (shader, NULL, shaderBaseKeys, token, &ptr);
				break;
			}
		}
		else {
			switch (token[0]) {
			case '{':
				inShader = qTrue;
				break;

			default:
				shader = R_NewShader (token, pathType);
				break;
			}
		}

		token = Com_ParseExt (&ptr, qTrue);
	}

	Mem_Free (buf);
}

/*
=============================================================================

	REGISTRATION

=============================================================================
*/

/*
==================
R_ReadyShader
==================
*/
static void R_ReadyShader (shader_t *shader)
{
	shaderPass_t	*pass;
	int				i, j;

	shader->touchFrame = r_registrationFrame;
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {

		pass->animNumFrames = 0;
		for (j=0 ; j<pass->numNames ; j++) {
			// Fragment program
			if (pass->flags & SHADER_PASS_FRAGMENTPROGRAM) {
				pass->fragProg = R_RegisterProgram (pass->fragProgName, qTrue);

				if (!pass->fragProg) {
					Com_Printf (PRNT_CONSOLE, S_COLOR_YELLOW "WARNING: Shader '%s' can't find/load fragment program '%s' for pass #%i (anim #%i)\n", shader->name, pass->fragProgName, i+1, j+1);
					pass->flags &= ~SHADER_PASS_FRAGMENTPROGRAM;
				}
			}

			// Vertex program
			if (pass->flags & SHADER_PASS_VERTEXPROGRAM) {
				pass->vertProg = R_RegisterProgram (pass->vertProgName, qFalse);

				if (!pass->vertProg) {
					Com_Printf (PRNT_CONSOLE, S_COLOR_YELLOW "WARNING: Shader '%s' can't find/load vertex program '%s' for pass #%i (anim #%i)\n", shader->name, pass->vertProgName, i+1, j+1);
					pass->flags &= ~SHADER_PASS_VERTEXPROGRAM;
				}
			}

			// No textures stored for lightmap passes
			if (pass->flags & SHADER_PASS_LIGHTMAP) {
				pass->animNumFrames++;
				continue;
			}

			// Texture
			if (pass->names[j][0] == '$') {
				if (!Q_stricmp (pass->names[j], "$white"))
					pass->animFrames[pass->animNumFrames] = r_whiteTexture;
				else if (!Q_stricmp (pass->names[j], "$black"))
					pass->animFrames[pass->animNumFrames] = r_blackTexture;
			}
			else {
				pass->animFrames[pass->animNumFrames] = R_RegisterImage (pass->names[j], pass->texFlags, pass->bumpScale);
			}

			// Report any errors
			if (pass->animFrames[pass->animNumFrames])
				pass->animNumFrames++;
			else {
				if (pass->names[j][0])
					Com_Printf (PRNT_CONSOLE, S_COLOR_YELLOW "WARNING: Shader '%s' can't find/load '%s' for pass #%i (anim #%i)\n", shader->name, pass->names[j], i+1, j+1);
				else
					Com_Printf (PRNT_CONSOLE, S_COLOR_YELLOW "WARNING: Shader '%s' has a NULL texture name for pass #%i (anim #%i)\n", shader->name, i+1, j+1);
			}
		}
	}
}


/*
==================
R_RegisterShader
==================
*/
static shader_t *R_RegisterShader (char *name, int shaderType, int surfParams)
{
	shader_t		*shader;
	shaderPass_t	*pass;
	tcMod_t			*tcMod;
	int				texFlags;
	float			bumpScale = 0;
	char			fixedName[MAX_QPATH];

	if (!name)
		return NULL;

	// See if it's already loaded
	shader = R_FindShader (name, surfParams);
	if (shader) {
		R_ReadyShader (shader);
		return shader;
	}

	// Default shader flags
	switch (shaderType) {
	case SHADER_SKYBOX:
		texFlags = IF_CLAMP;
		break;

	case SHADER_PIC:
		texFlags = IF_NOMIPMAP_LINEAR|IF_NOPICMIP;
		if (!vid_gammapics->integer)
			texFlags |= IF_NOGAMMA;
		break;

	default:
		texFlags = 0;
		break;
	}

	// If the image doesn't exist, return NULL
	if (!R_RegisterImage (name, texFlags, bumpScale))
		return NULL;

	// Copy and fix name
	Q_strncpyz (fixedName, name, sizeof (fixedName));
	Q_FixPathName (fixedName, sizeof (fixedName), 3, qFalse);

	// Default shader
	shader = R_NewShader (name, SHADER_PATHTYPE_INTERNAL);
	shader->sizeBase = 0;
	shader->surfParams = surfParams;
	shader->flags = 0;

	switch (shaderType) {
	case SHADER_ALIAS:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_NORMALS;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ENTITY;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->bumpScale = bumpScale;
		pass->flags = 0;
		pass->texFlags = texFlags;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		pass->tcGen = TC_GEN_BASE;
		break;

	case SHADER_PIC:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS;
		shader->flags = 0;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ADDITIVE;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->bumpScale = bumpScale;
		pass->flags = SHADER_PASS_BLEND;
		pass->texFlags = texFlags;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		pass->tcGen = TC_GEN_BASE;
		break;

	case SHADER_POLY:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS;
		shader->flags = SHADER_ENTITY_MERGABLE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ENTITY;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->bumpScale = bumpScale;
		pass->flags = SHADER_PASS_BLEND;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		break;

	case SHADER_SKYBOX:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_SKY;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->bumpScale = bumpScale;
		pass->flags = SHADER_PASS_NOCOLORARRAY;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		break;

	default:
	case SHADER_BSP:
		if (surfParams > 0 && surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66|SHADER_SURF_WARP|SHADER_SURF_LIGHTMAP)) {
			if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66|SHADER_SURF_WARP)) {
				shader->cullType = SHADER_CULL_FRONT;
				shader->features = MF_STCOORDS|MF_TRNORMALS;
				shader->flags = SHADER_DEPTHWRITE;
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
				shader->numPasses = 1;

				// Sort key
				if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66))
					shader->sortKey = SHADER_SORT_WATER;
				else
					shader->sortKey = SHADER_SORT_OPAQUE;

				pass = &shader->passes[0];
				Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
				pass->bumpScale = bumpScale;
				pass->flags = SHADER_PASS_NOCOLORARRAY;
				pass->texFlags = texFlags;
				pass->depthFunc = GL_LEQUAL;
				pass->blendMode = GL_MODULATE;

				// Subdivide if warping
				if (surfParams & SHADER_SURF_WARP) {
					shader->flags |= SHADER_SUBDIVIDE|SHADER_ENTITY_MERGABLE;
					shader->subdivide = 64;
					pass->tcGen = TC_GEN_WARP;
				}
				else {
					pass->tcGen = TC_GEN_BASE;
				}

				pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
				if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66)) {
					pass->flags |= SHADER_PASS_BLEND;
					pass->blendSource = GL_SRC_ALPHA;
					pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
					pass->alphaGen.type = ALPHA_GEN_CONST;
					pass->alphaGen.args[0] = (surfParams & SHADER_SURF_TRANS33) ? 0.33f : 0.66f;
				}
				else {
					pass->alphaGen.type = ALPHA_GEN_IDENTITY;
				}
			}
			else {
				shader->cullType = SHADER_CULL_FRONT;
				shader->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS;
				shader->flags = SHADER_DEPTHWRITE;
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t) * 2, MEMPOOL_SHADERSYS, 0);
				shader->numPasses = 2;
				shader->sizeBase = 1;
				shader->sortKey = SHADER_SORT_OPAQUE;

				pass = &shader->passes[0];
				Q_strncpyz (pass->names[pass->numNames++], "$lightmap", sizeof (pass->names[0]));
				pass->bumpScale = bumpScale;
				pass->flags = SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
				pass->tcGen = TC_GEN_LIGHTMAP;
				pass->depthFunc = GL_LEQUAL;
				pass->blendMode = GL_REPLACE;
				pass->rgbGen.type = RGB_GEN_IDENTITY;
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;

				pass = &shader->passes[1];
				Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
				pass->bumpScale = bumpScale;
				pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
				pass->texFlags = texFlags;
				pass->tcGen = TC_GEN_BASE;
				pass->depthFunc = GL_LEQUAL;
				pass->blendSource = GL_ZERO;
				pass->blendDest = GL_SRC_COLOR;
				pass->blendMode = GL_MODULATE;
				pass->rgbGen.type = RGB_GEN_IDENTITY;
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;
			}
		}
		else {
			shader->cullType = SHADER_CULL_FRONT;
			shader->features = MF_STCOORDS|MF_TRNORMALS;
			shader->flags = SHADER_DEPTHWRITE;
			shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
			shader->numPasses = 1;
			shader->sizeBase = 0;
			shader->sortKey = SHADER_SORT_OPAQUE;

			pass = &shader->passes[0];
			Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
			pass->bumpScale = bumpScale;
			pass->flags = SHADER_PASS_NOCOLORARRAY;
			pass->texFlags = texFlags;
			pass->tcGen = TC_GEN_BASE;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		}

		// Add flowing if it's got the flag
		if (surfParams > 0 && surfParams & SHADER_SURF_FLOWING) {
			pass->numTCMods = 1;
			pass->tcMods = Mem_PoolAlloc (sizeof (tcMod_t), MEMPOOL_SHADERSYS, 0);

			tcMod = &pass->tcMods[0];
			tcMod->type = TC_MOD_SCROLL;
			tcMod->args[0] = -0.75f;
			tcMod->args[1] = 0;
		}
		break;
	}

	R_ReadyShader (shader);
	return shader;
}

shader_t *R_RegisterPic (char *name)
{
	return R_RegisterShader (name, SHADER_PIC, -1);
}

shader_t *R_RegisterPoly (char *name)
{
	return R_RegisterShader (name, SHADER_POLY, -1);
}

shader_t *R_RegisterSkin (char *name)
{
	return R_RegisterShader (name, SHADER_ALIAS, -1);
}

shader_t *R_RegisterSky (char *name)
{
	return R_RegisterShader (name, SHADER_SKYBOX, -1);
}

shader_t *R_RegisterTexture (char *name, int surfParams)
{
	return R_RegisterShader (name, SHADER_BSP, surfParams);
}


/*
==================
R_EndShaderRegistration
==================
*/
void R_EndShaderRegistration (void)
{
	shader_t	*shader;
	uInt		i, startTime;
	uInt		touched;

	touched = 0;
	startTime = Sys_Milliseconds ();
	for (i=0, shader=r_shaderList ; i<r_numShaders ; shader++, i++) {
		if (shader->touchFrame != r_registrationFrame)
			continue;	// Not touched

		R_ReadyShader (shader);
		touched++;
	}
	Com_Printf (PRNT_CONSOLE, "%i shaders touched in %.2fs\n", touched, (Sys_Milliseconds () - startTime)*0.001f);
}

/*
=============================================================================

	CONSOLE FUNCTIONS

=============================================================================
*/

/*
==================
R_ShaderCheck_f
==================
*/
static void R_ShaderCheck_f (void)
{
	shader_t	*a, *b;
	uInt		i, j;
	uInt		total;

	total = 0;
	for (i=0, a=r_shaderList ; i<r_numShaders && a ; i++, a++) {
		for (j=0, b=r_shaderList ; j<r_numShaders && b; j++, b++) {
			if (!Q_stricmp (a->name, b->name) && i != j) {
				Com_Printf (0, "%3i %s & %3i %s\n", i, a->name, j, b->name);
				total++;
			}
		}
	}

	Com_Printf (0, "Total shader duplicates: %i\n", total);
}


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
	uInt			i;

	if (Cmd_Argc () == 2)
		wildCard = Cmd_Argv (1);
	else
		wildCard = "*";

	Com_Printf (0, "-------------- Loaded Shaders --------------\n");
	Com_Printf (0, "Base Sh#  PASS NAME\n");
	Com_Printf (0, "---- ---- ---- ----------------------------------\n");

	for (i=0, totShaders=0, totPasses=0, shader=r_shaderList ; i<r_numShaders ; shader++, i++) {
		if (!shader->touchFrame)
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

static cmdFunc_t	*cmd_shaderCheck;
static cmdFunc_t	*cmd_shaderList;

/*
==================
R_ShaderInit
==================
*/
void R_ShaderInit (void)
{
	char	*shaderList[MAX_SHADERS];
	char	*name;
	int		numShaders;
	byte	pathType;
	int		i;

	Com_Printf (0, "\n-------- Shader Initialization ---------\n");

	// Clear lists
	r_numShaders = 0;
	memset (&r_shaderList, 0, sizeof (shader_t) * MAX_SHADERS);
	memset (r_shaderHashTree, 0, sizeof (shader_t) * MAX_SHADER_HASH);

	// Cvars
	vid_gammapics		= Cvar_Get ("vid_gammapics",		"0",						CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	// Console commands
	cmd_shaderCheck	= Cmd_AddCommand ("shadercheck",	R_ShaderCheck_f,	"Shader system integrity check");
	cmd_shaderList	= Cmd_AddCommand ("shaderlist",		R_ShaderList_f,		"Prints to the console a list of loaded shaders");

	// Load scripts
	numShaders = FS_FindFiles ("scripts", "*scripts/*.shd", "shd", shaderList, MAX_SHADERS, qTrue, qFalse);
	for (i=0 ; i<numShaders ; i++) {
		// Skip path
		if (shaderList[i][0] == '/'
		|| (shaderList[i][0] == '.' && shaderList[i][1] == '/'))
			name = strchr (shaderList[i]+2, '/');
		else
			name = strchr (shaderList[i], '/');

		// Base dir shader?
		pathType = (strstr (shaderList[i], "./" BASE_MODDIRNAME "/") || strstr (shaderList[i], BASE_MODDIRNAME "/")) ? SHADER_PATHTYPE_BASEDIR : SHADER_PATHTYPE_MODDIR;

		R_LoadShaderFile (name, pathType);
	}
	FS_FreeFileList (shaderList, numShaders);

	// Shader counterparts
	r_noShader = R_RegisterTexture (r_noTexture->name, -1);
	r_noShaderLightmap = R_RegisterTexture (r_noTexture->name, SHADER_SURF_LIGHTMAP);
	r_whiteShader = R_RegisterTexture (r_whiteTexture->name, -1);
	r_blackShader = R_RegisterTexture (r_blackTexture->name, -1);

	// Check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_SHADERSYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
R_ShaderShutdown
==================
*/
void R_ShaderShutdown (void)
{
	// Rremove commands
	Cmd_RemoveCommand ("shadercheck", cmd_shaderCheck);
	Cmd_RemoveCommand ("shaderlist", cmd_shaderList);

	// Free all loaded shaders
	Mem_FreePool (MEMPOOL_SHADERSYS);
}
