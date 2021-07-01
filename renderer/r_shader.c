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
static uint32			r_numShaders;

static uint32			r_numCurrPasses;
static shaderPass_t		r_currPasses[MAX_SHADER_PASSES];
static vertDeform_t		r_currDeforms[MAX_SHADER_DEFORMVS];
static tcMod_t			r_currTcMods[MAX_SHADER_PASSES][MAX_SHADER_TCMODS];

static uint32			r_numShaderErrors;
static uint32			r_numShaderWarnings;

typedef struct shaderKey_s {
	char		*keyWord;
	qBool		(*func)(shader_t *shader, shaderPass_t *pass, parse_t *ps);
} shaderKey_t;

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
Shader_ParseString
==================
*/
static char *Shader_ParseString (parse_t *ps)
{
	char *str;

	str = Com_ParseExt (ps, qFalse);
	if (!str || !str[0] || str[0] == '}')
		return "";

	return Q_strlwr (str);
}


/*
==================
Shader_ParseFloat
==================
*/
static float Shader_ParseFloat (parse_t *ps)
{
	char *str;

	str = Com_ParseExt (ps, qFalse);
	if (!str || !str[0] || str[0] == '}')
		return 0;

	return (float)atof (str);
}


/*
==================
Shader_ParseInt
==================
*/
static int Shader_ParseInt (parse_t *ps)
{
	char *str;

	str = Com_ParseExt (ps, qFalse);
	if (!str || !str[0] || str[0] == '}')
		return 0;

	return atoi (str);
}


/*
==================
Shader_ParseFunc
==================
*/
static void Shader_ParseFunc (parse_t *ps, shaderFunc_t *func)
{
	char	*str;

	str = Shader_ParseString (ps);
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

	func->args[0] = Shader_ParseFloat (ps);
	func->args[1] = Shader_ParseFloat (ps);
	func->args[2] = Shader_ParseFloat (ps);
	func->args[3] = Shader_ParseFloat (ps);
}


/*
==================
Shader_ParseVector
==================
*/
static void Shader_ParseVector (parse_t *ps, float *vec, uint32 size)
{
	qBool	inBrackets;
	char	*str;
	uint32	i;

	if (!size) {
		return;
	}
	else if (size == 1) {
		Shader_ParseFloat (ps);
		return;
	}

	str = Shader_ParseString (ps);
	if (!strcmp (str, "(")) {
		inBrackets = qTrue;
		str = Shader_ParseString (ps);
	}
	else if (str[0] == '(') {
		inBrackets = qTrue;
		str = &str[1];
	}
	else {
		inBrackets = qFalse;
	}

	vec[0] = (float)atof (str);
	for (i=1 ; i<size-1 ; i++)
		vec[i] = Shader_ParseFloat (ps);

	str = Shader_ParseString (ps);
	if (!str[0]) {
		vec[i] = 0;
	}
	else if (str[strlen(str)-1] == ')') {
		str[strlen(str)-1] = 0;
		vec[i] = (float)atof (str);
	}
	else {
		vec[i] = (float)atof (str);
		if (inBrackets)
			Shader_ParseString (ps);
	}
}


/*
==================
Shader_SkipLine
==================
*/
static void Shader_SkipLine (parse_t *ps)
{
	char *str;

	for ( ; ; ) {
		str = Com_ParseExt (ps, qFalse);
		if (!str || !str[0])
			break;
	}
}


/*
==================
Shader_SkipBlock
==================
*/
static void Shader_SkipBlock (parse_t *ps)
{
	char	*str;
	int		braceCount;

	// Opening brace
	str = Com_ParseExt (ps, qTrue);
	if (str[0] != '{')
		return;

	for (braceCount=1 ; braceCount>0 ; ) {
		str = Com_ParseExt (ps, qTrue);
		if (!str[0])
			return;
		else if (str[0] == '{')
			braceCount++;
		else if (str[0] == '}')
			braceCount--;
	}
}


/*
==================
Shader_PassLocation
==================
*/
static char *Shader_PassLocation (shader_t *shader, parse_t *ps)
{
	return Q_VarArgs ("Shader '%s' pass #%i line #%i", shader->name, r_numCurrPasses+1, ps->currentLine);
}


/*
==================
Shader_BaseLocation
==================
*/
static char *Shader_BaseLocation (shader_t *shader, parse_t *ps)
{
	return Q_VarArgs ("Shader '%s' line #%i", shader->name, ps->currentLine);
}


/*
==================
Shader_Printf
==================
*/
static void Shader_Printf (comPrint_t flags, char *fmt, ...)
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
Shader_DevPrintf
==================
*/
static void Shader_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_COMPRINT];

	if (!developer->integer)
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

// ==========================================================================

static qBool ShaderPass_AnimFrequency (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->animFPS = Shader_ParseInt (ps);
	pass->flags |= SHADER_PASS_ANIMMAP;

	return qTrue;
}

static qBool ShaderPass_AnimMap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	pass->numNames = 0;
	pass->animFPS = Shader_ParseInt (ps);
	pass->flags |= SHADER_PASS_ANIMMAP;
	pass->tcGen = TC_GEN_BASE;

	for ( ; ; ) {
		str = Shader_ParseString (ps);
		if (!str[0]) {
			if (!pass->numNames) {
				pass->animFPS = 0;
				pass->flags &= ~SHADER_PASS_ANIMMAP;
				Shader_Printf (PRNT_ERROR, "ERROR: %s: animMap with no parameters\n", Shader_PassLocation (shader, ps));
				return qFalse;
			}
			break;
		}

		if (pass->numNames+1 >= MAX_SHADER_ANIM_FRAMES) {
			Shader_Printf (PRNT_WARNING, "WARNING: %s: too many animation frames, ignoring\n", Shader_PassLocation (shader, ps));
			Shader_SkipLine (ps);
			return qTrue;
		}

		Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	}

	return qTrue;
}

static qBool ShaderPass_Map (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (pass->numNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many animation frames, ignoring\n", Shader_PassLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: map with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	if (!strcmp (str, "$lightmap")) {
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_LIGHTMAP;
	}
	else {
		pass->tcGen = TC_GEN_BASE;
		if (!strcmp (str, "$rgb")) {
			pass->texFlags |= IF_NOALPHA;
			str = Shader_ParseString (ps);
		}
		else if (!strcmp (str, "$alpha")) {
			pass->texFlags |= IF_NORGB;
			str = Shader_ParseString (ps);
		}
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_AlphaMap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (pass->numNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many animation frames, ignoring\n", Shader_PassLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	pass->tcGen = TC_GEN_BASE;
	pass->texFlags |= IF_NORGB;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: alphaMap with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_ClampMap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	if (ShaderPass_Map (shader, pass, ps)) {
		pass->texFlags |= IF_CLAMP;
		return qTrue;
	}

	return qFalse;
}

static qBool ShaderPass_RGBMap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (pass->numNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many animation frames, ignoring\n", Shader_PassLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	pass->tcGen = TC_GEN_BASE;
	pass->texFlags |= IF_NOALPHA;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: rgbMap with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	Q_strncpyz (pass->names[pass->numNames++], str, sizeof (pass->names[0]));
	return qTrue;
}

static qBool ShaderPass_CubeMap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (pass->numNames+1 >= MAX_SHADER_ANIM_FRAMES) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many animation frames, ignoring\n", Shader_PassLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: cubeMap with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	if (glConfig.extTexCubeMap) {
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

static qBool ShaderPass_FragmentProgram (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (!glConfig.extFragmentProgram) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: fragProg when the extension not available\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	str = Shader_ParseString (ps);
	Q_strncpyz (pass->fragProgName, str, sizeof (pass->fragProgName));
	pass->flags |= SHADER_PASS_FRAGMENTPROGRAM;
	return qTrue;
}

static qBool ShaderPass_VertexProgram (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	if (!glConfig.extVertexProgram) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: vertProg when the extension not available\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	str = Shader_ParseString (ps);
	Q_strncpyz (pass->vertProgName, str, sizeof (pass->vertProgName));
	pass->flags |= SHADER_PASS_VERTEXPROGRAM;
	return qTrue;
}

static qBool ShaderPass_RGBGen (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: rgbGen with no parameters\n", Shader_PassLocation (shader, ps));
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
		pass->rgbGen.args[0] = 1.0f;
		pass->rgbGen.args[1] = 1.0f;
		pass->rgbGen.args[2] = 1.0f;
		Shader_ParseFunc (ps, &pass->rgbGen.func);
		return qTrue;
	}
	else if (!strcmp (str, "colorwave")) {
		pass->rgbGen.type = RGB_GEN_COLORWAVE;
		Shader_ParseVector (ps, pass->rgbGen.args, 3);
		Shader_ParseFunc (ps, &pass->rgbGen.func);
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

		if (intensity->integer > 0)
			div = 1.0f / (float)pow (2, intensity->integer / 2.0f);
		else
			div = 1.0f;

		pass->rgbGen.type = RGB_GEN_CONST;
		Shader_ParseVector (ps, color, 3);
		R_ColorNormalize (color, pass->rgbGen.args);
		VectorScale (pass->rgbGen.args, div, pass->rgbGen.args);

		return qTrue;
	}

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid rgbGen value: '%s'\n", Shader_PassLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderPass_AlphaGen (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: alphaGen with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	if (!strcmp (str, "identity")) {
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;
		return qTrue;
	}
	else if (!strcmp (str, "const") || !strcmp (str, "constant")) {
		pass->alphaGen.type = ALPHA_GEN_CONST;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (ps));
		return qTrue;
	}
	else if (!strcmp (str, "wave")) {
		pass->alphaGen.type = ALPHA_GEN_WAVE;
		Shader_ParseFunc (ps, &pass->alphaGen.func);
		return qTrue;
	}
	else if (!strcmp (str, "portal")) {
		pass->alphaGen.type = ALPHA_GEN_PORTAL;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (ps));
		if (!pass->alphaGen.args[0])
			pass->alphaGen.args[0] = 256;
		pass->alphaGen.args[0] = 1.0f / pass->alphaGen.args[0];
		return qTrue;
	}
	else if (!strcmp (str, "vertex")) {
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
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
		pass->alphaGen.type = ALPHA_GEN_DOT;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (ps));
		pass->alphaGen.args[1] = fabs (Shader_ParseFloat (ps));
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}
	else if (!strcmp (str, "oneminusdot")) {
		pass->alphaGen.type = ALPHA_GEN_ONE_MINUS_DOT;
		pass->alphaGen.args[0] = fabs (Shader_ParseFloat (ps));
		pass->alphaGen.args[1] = fabs (Shader_ParseFloat (ps));
		if (!pass->alphaGen.args[1])
			pass->alphaGen.args[1] = 1.0f;
		return qTrue;
	}

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid alphaGen value: '%s'\n", Shader_PassLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderPass_AlphaFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: alphaFunc with no parameters\n", Shader_PassLocation (shader, ps));
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

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid alphaFunc value: '%s'\n", Shader_PassLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderPass_BlendFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;
	int		blend, i;

	static const char *blendTargs[] = {
		"source",
		"dest",
		0
	};

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: blendFunc with no parameters\n", Shader_PassLocation (shader, ps));
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
		blend = GL_ONE;
		for (i=0 ; i<2 ; i++) {
			if (!strcmp (str, "gl_dst_alpha"))
				blend = GL_DST_ALPHA;
			else if (!strcmp (str, "gl_dst_color"))
				blend = GL_DST_COLOR;
			else if (!strcmp (str, "gl_one"))
				blend = GL_ONE;
			else if (!strcmp (str, "gl_one_minus_dst_alpha"))
				blend = GL_ONE_MINUS_DST_ALPHA;
			else if (!strcmp (str, "gl_one_minus_dst_color"))
				blend = GL_ONE_MINUS_DST_COLOR;
			else if (!strcmp (str, "gl_one_minus_src_alpha"))
				blend = GL_ONE_MINUS_SRC_ALPHA;
			else if (!strcmp (str, "gl_one_minus_src_color"))
				blend = GL_ONE_MINUS_SRC_COLOR;
			else if (!strcmp (str, "gl_src_alpha"))
				blend = GL_SRC_ALPHA;
			else if (!strcmp (str, "gl_src_alpha_saturate"))
				blend = GL_SRC_ALPHA_SATURATE;
			else if (!strcmp (str, "gl_src_color"))
				blend = GL_SRC_COLOR;
			else if (!strcmp (str, "gl_zero"))
				blend = GL_ZERO;
			else {
				Shader_Printf (PRNT_WARNING, "WARNING: %s: has an invalid blend %s: '%s', assuming GL_ONE\n", Shader_PassLocation (shader, ps), blendTargs[i], str);
				blend = GL_ONE;
			}

			if (i == 1)
				break;

			pass->blendSource = blend;
			str = Shader_ParseString (ps);
		}

		pass->blendDest = blend;
	}

	pass->flags |= SHADER_PASS_BLEND;
	return qTrue;
}

static qBool ShaderPass_DepthFunc (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: depthFunc with no parameters\n", Shader_PassLocation (shader, ps));
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

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid depthFunc value: '%s'\n", Shader_PassLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderPass_TcGen (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: tcGen with no parameters\n", Shader_PassLocation (shader, ps));
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
		Shader_ParseVector (ps, pass->tcGenVec[0], 4);
		Shader_ParseVector (ps, pass->tcGenVec[1], 4);
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

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid tcGen value: '%s'\n", Shader_PassLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderPass_TcMod (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	tcMod_t	*tcMod;
	char	*str;
	float	val;
	int		i;

	if (pass->numTCMods == MAX_SHADER_TCMODS) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many tcMods, ignoring\n", Shader_PassLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	tcMod = &r_currTcMods[r_numCurrPasses][r_currPasses[r_numCurrPasses].numTCMods];

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: tcMod with no parameters\n", Shader_PassLocation (shader, ps));
		return qFalse;
	}

	if (!strcmp (str, "rotate")) {
		val = Shader_ParseFloat (ps);
		tcMod->args[0] = -val / 360.0f;
		if (!tcMod->args[0]) {
			Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid tcMod rotate: '%f'\n", Shader_PassLocation (shader, ps), val);
			return qFalse;
		}
		tcMod->type = TC_MOD_ROTATE;
	}
	else if (!strcmp (str, "scale")) {
		Shader_ParseVector (ps, tcMod->args, 2);
		tcMod->type = TC_MOD_SCALE;
	}
	else if (!strcmp (str, "scroll")) {
		Shader_ParseVector (ps, tcMod->args, 2);
		tcMod->type = TC_MOD_SCROLL;
	}
	else if (!strcmp (str, "stretch")) {
		shaderFunc_t	func;

		Shader_ParseFunc (ps, &func);

		tcMod->args[0] = func.type;
		for (i=1 ; i<5 ; i++) {
			tcMod->args[i] = func.args[i-1];
		}
		tcMod->type = TC_MOD_STRETCH;
	}
	else if (!strcmp (str, "transform")) {
		Shader_ParseVector (ps, tcMod->args, 6);
		tcMod->args[4] = tcMod->args[4] - floor(tcMod->args[4]);
		tcMod->args[5] = tcMod->args[5] - floor(tcMod->args[5]);
		tcMod->type = TC_MOD_TRANSFORM;
	}
	else if (!strcmp (str, "turb")) {
		Shader_ParseVector (ps, tcMod->args, 4);
		tcMod->type = TC_MOD_TURB;
	}
	else {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid tcMod value: '%s'\n", Shader_PassLocation (shader, ps), str);
		return qFalse;
	}

	r_currPasses[r_numCurrPasses].numTCMods++;
	return qTrue;
}

static qBool ShaderPass_MaskColor (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->maskRed = qTrue;
	pass->maskGreen = qTrue;
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskRed (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->maskRed = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskGreen (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->maskGreen = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskBlue (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->maskBlue = qTrue;
	return qTrue;
}

static qBool ShaderPass_MaskAlpha (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->maskAlpha = qTrue;
	return qTrue;
}

static qBool ShaderPass_NoGamma (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->texFlags |= IF_NOGAMMA;
	return qTrue;
}

static qBool ShaderPass_NoIntens (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->texFlags |= IF_NOINTENS;
	return qTrue;
}

static qBool ShaderPass_NoMipmap (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_DevPrintf (PRNT_WARNING, "WARNING: %s: noMipMap with no type specified, assuming linear\n", Shader_PassLocation (shader, ps));
		return qTrue;
	}

	if (!strcmp (str, "linear")) {
		pass->texFlags |= IF_NOMIPMAP_LINEAR;
		return qTrue;
	}
	else if (!strcmp (str, "nearest")) {
		pass->texFlags |= IF_NOMIPMAP_NEAREST;
		return qTrue;
	}

	Shader_Printf (PRNT_WARNING, "WARNING: %s: invalid noMipMap value: '%s', assuming linear\n", Shader_PassLocation (shader, ps), str);
	pass->texFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool ShaderPass_NoPicmip (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->texFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool ShaderPass_NoCompress (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->texFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool ShaderPass_TcClamp (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->texFlags |= IF_CLAMP;
	return qTrue;
}

static qBool ShaderPass_SizeBase (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->sizeBase = r_numCurrPasses;
	return qTrue;
}

static qBool ShaderPass_Detail (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->flags |= SHADER_PASS_DETAIL;
	return qTrue;
}

static qBool ShaderPass_NotDetail (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->flags |= SHADER_PASS_NOTDETAIL;
	return qTrue;
}

static qBool ShaderPass_DepthWrite (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	pass->flags |= SHADER_PASS_DEPTHWRITE;
	shader->flags |= SHADER_DEPTHWRITE;
	return qTrue;
}

// ==========================================================================

static shaderKey_t r_shaderPassKeys[] = {
	{ "animfrequency",	&ShaderPass_AnimFrequency	},
	{ "animmap",		&ShaderPass_AnimMap			},
	{ "cubemap",		&ShaderPass_CubeMap			},
	{ "map",			&ShaderPass_Map				},
	{ "alphamap",		&ShaderPass_AlphaMap		},
	{ "clampmap",		&ShaderPass_ClampMap		},
	{ "rgbmap",			&ShaderPass_RGBMap			},

	{ "fragprog",		&ShaderPass_FragmentProgram	},
	{ "vertprog",		&ShaderPass_VertexProgram	},

	{ "rgbgen",			&ShaderPass_RGBGen			},
	{ "alphagen",		&ShaderPass_AlphaGen		},

	{ "alphafunc",		&ShaderPass_AlphaFunc		},
	{ "blendfunc",		&ShaderPass_BlendFunc		},
	{ "depthfunc",		&ShaderPass_DepthFunc		},

	{ "tcclamp",		&ShaderPass_TcClamp			},
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
	{ "nodetail",		&ShaderPass_NotDetail		},
	{ "notdetail",		&ShaderPass_NotDetail		},

	{ "depthwrite",		&ShaderPass_DepthWrite		},

	{ NULL,				NULL						}
};

static const int r_numShaderPassKeys = sizeof (r_shaderPassKeys) / sizeof (r_shaderPassKeys[0]) - 1;

/*
=============================================================================

	SHADER PROCESSING

=============================================================================
*/

static qBool ShaderBase_BackSided (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->cullType = SHADER_CULL_BACK;
	return qTrue;
}

static qBool ShaderBase_Cull (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: cull with no parameters\n", Shader_BaseLocation (shader, ps));
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

	Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid cull value: '%s'\n", Shader_BaseLocation (shader, ps), str);
	return qFalse;
}

static qBool ShaderBase_DeformVertexes (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	vertDeform_t	*deformv;
	char			*str;

	if (shader->numDeforms == MAX_SHADER_DEFORMVS) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many deformVertexes, ignoring\n", Shader_BaseLocation (shader, ps));
		Shader_SkipLine (ps);
		return qTrue;
	}

	deformv = &r_currDeforms[shader->numDeforms];

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: deformVertexes with no parameters\n", Shader_BaseLocation (shader, ps));
		return qFalse;
	}

	if (!strcmp (str, "wave")) {
		deformv->type = DEFORMV_WAVE;
		deformv->args[0] = Shader_ParseFloat (ps);
		if (deformv->args[0])
			deformv->args[0] = 1.0f / deformv->args[0];
		Shader_ParseFunc (ps, &deformv->func);
	}
	else if (!strcmp (str, "normal")) {
		deformv->type = DEFORMV_NORMAL;
		deformv->args[0] = Shader_ParseFloat (ps);
		deformv->args[1] = Shader_ParseFloat (ps);
	}
	else if (!strcmp (str, "bulge")) {
		deformv->type = DEFORMV_BULGE;
		Shader_ParseVector (ps, deformv->args, 3);
		shader->flags |= SHADER_DEFORMV_BULGE;
	}
	else if (!strcmp (str, "move")) {
		deformv->type = DEFORMV_MOVE;
		Shader_ParseVector (ps, deformv->args, 3);
		Shader_ParseFunc (ps, &deformv->func);
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
		Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid deformVertexes value: '%s'\n", Shader_BaseLocation (shader, ps), str);
		return qFalse;
	}

	shader->numDeforms++;
	return qTrue;
}

static qBool ShaderBase_Subdivide (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;
	int		size = 8;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: subdivide with no parameters\n", Shader_BaseLocation (shader, ps));
		return qFalse;
	}

	shader->subdivide = atoi (str);
	if (shader->subdivide < 8 || shader->subdivide > 256) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: out of bounds subdivide size: '%i', assuming 64", Shader_BaseLocation (shader, ps), shader->subdivide);
		shader->subdivide = 64;
	}
	else {
		// Force power of two
		while (size <= shader->subdivide)
			size <<= 1;

		size >>= 1;

		if (size != shader->subdivide) {
			Shader_Printf (PRNT_WARNING, "WARNING: %s: subdivide size '%i' is not a power of two, forcing: '%i'\n", Shader_BaseLocation (shader, ps), shader->subdivide, size);
			shader->subdivide = size;
		}
	}

	shader->flags |= SHADER_SUBDIVIDE;
	return qTrue;
}

static qBool ShaderBase_EntityMergable (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_ENTITY_MERGABLE;
	return qTrue;
}

static qBool ShaderBase_DepthRange (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_DEPTHRANGE;

	shader->depthNear = Shader_ParseFloat (ps);
	shader->depthFar = Shader_ParseFloat (ps);
	return qTrue;
}

static qBool ShaderBase_DepthWrite (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_DEPTHWRITE;
	shader->addPassFlags |= SHADER_PASS_DEPTHWRITE;
	return qTrue;
}

static qBool ShaderBase_FogParms (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	vec3_t	color, ncolor;

	Shader_ParseVector (ps, color, 3);
	R_ColorNormalize (color, ncolor);

	shader->fogColor[0] = R_FloatToByte (ncolor[0]);
	shader->fogColor[1] = R_FloatToByte (ncolor[1]);
	shader->fogColor[2] = R_FloatToByte (ncolor[2]);
	shader->fogColor[3] = 255;

	shader->fogDist = (double)Shader_ParseFloat (ps);
	if (shader->fogDist <= 0.1)
		shader->fogDist = 128.0;
	shader->fogDist = 1.0 / shader->fogDist;

	return qTrue;
}

static qBool ShaderBase_PolygonOffset (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_DevPrintf (PRNT_WARNING, "WARNING: %s: no offset values, assuming -1 -2\n", Shader_BaseLocation (shader, ps));
		shader->offsetFactor = -1;
		shader->offsetUnits = -2;
		shader->flags |= SHADER_POLYGONOFFSET;
		return qTrue;
	}

	shader->offsetFactor = atof (str);
	if (!shader->offsetFactor) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid offsetFactor value: '%i'\n", Shader_BaseLocation (shader, ps), shader->offsetFactor);
		return qFalse;
	}

	shader->offsetUnits = Shader_ParseFloat (ps);
	if (!shader->offsetUnits) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid offsetUnits value: '%i'\n", Shader_BaseLocation (shader, ps), shader->offsetUnits);
		return qFalse;
	}

	shader->flags |= SHADER_POLYGONOFFSET;
	return qTrue;
}

static qBool ShaderBase_Portal (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->sortKey = SHADER_SORT_PORTAL;
	return qTrue;
}

static qBool ShaderBase_SkyParms (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	// FIXME
	Shader_SkipLine (ps);

	shader->flags |= SHADER_SKY;
	shader->sortKey = SHADER_SORT_SKY;
	return qTrue;
}

static qBool ShaderBase_NoCompress (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->addTexFlags |= IF_NOCOMPRESS;
	return qTrue;
}

static qBool ShaderBase_NoLerp (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_NOLERP;
	return qTrue;
}

static qBool ShaderBase_NoMark (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_NOMARK;
	return qTrue;
}

static qBool ShaderBase_NoMipMaps (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_DevPrintf (PRNT_WARNING, "WARNING: %s: noMipMaps with no parameters, assuming linear\n", Shader_BaseLocation (shader, ps));
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

	Shader_DevPrintf (PRNT_WARNING, "WARNING: %s: invalid noMipMaps value, assuming linear\n", Shader_BaseLocation (shader, ps));
	shader->addTexFlags |= IF_NOMIPMAP_LINEAR;
	return qTrue;
}

static qBool ShaderBase_NoPicMips (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->addTexFlags |= IF_NOPICMIP;
	return qTrue;
}

static qBool ShaderBase_NoShadow (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	shader->flags |= SHADER_NOSHADOW;
	return qTrue;
}

static qBool ShaderBase_SortKey (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: sortKey with no parameters\n", Shader_BaseLocation (shader, ps));
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
			Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid sortKey value: '%i'", Shader_BaseLocation (shader, ps), shader->sortKey);
			return qFalse;
		}
	}

	return qTrue;
}

static qBool ShaderBase_SurfParams (shader_t *shader, shaderPass_t *pass, parse_t *ps)
{
	char	*str;

	str = Shader_ParseString (ps);
	if (!str[0]) {
		Shader_Printf (PRNT_ERROR, "ERROR: %s: surfParams with no parameters\n", Shader_BaseLocation (shader, ps));
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
		Shader_Printf (PRNT_ERROR, "ERROR: %s: invalid surfParam value: '%s'", Shader_BaseLocation (shader, ps), str);
		return qFalse;
	}

	return qTrue;
}

// ==========================================================================

static shaderKey_t r_shaderBaseKeys[] = {
	{ "backsided",				&ShaderBase_BackSided		},
	{ "cull",					&ShaderBase_Cull			},
	{ "deformvertexes",			&ShaderBase_DeformVertexes	},
	{ "entitymergable",			&ShaderBase_EntityMergable	},
	{ "subdivide",				&ShaderBase_Subdivide		},
	{ "depthrange",				&ShaderBase_DepthRange		},
	{ "depthwrite",				&ShaderBase_DepthWrite		},
	{ "fogparms",				&ShaderBase_FogParms		},
	{ "polygonoffset",			&ShaderBase_PolygonOffset	},
	{ "portal",					&ShaderBase_Portal			},
	{ "skyparms",				&ShaderBase_SkyParms		},
	{ "sort",					&ShaderBase_SortKey			},
	{ "sortkey",				&ShaderBase_SortKey			},
	{ "surfparam",				&ShaderBase_SurfParams		},

	{ "nocompress",				&ShaderBase_NoCompress		},
	{ "noimpact",				&ShaderBase_NoMark			},
	{ "nolerp",					&ShaderBase_NoLerp			},
	{ "nomark",					&ShaderBase_NoMark			},
	{ "nomipmaps",				&ShaderBase_NoMipMaps		},
	{ "nopicmip",				&ShaderBase_NoPicMips		},
	{ "noshadow",				&ShaderBase_NoShadow		},

	// FIXME orly: TODO
	{ "foggen",					NULL						},
	{ "fogonly",				NULL						},
	{ "sky",					NULL						},

	// These are compiler/editor options
	{ "cloudparms",				NULL						},
	{ "light",					NULL						},
	{ "light1",					NULL						},
	{ "lightning",				NULL						},
	{ "nodrop",					NULL						},
	{ "nolightmap",				NULL						},
	{ "nonsolid",				NULL						},
	{ "q3map_*",				NULL						},
	{ "qer_*",					NULL						},
	{ "surfaceparm",			NULL						},
	{ "tesssize",				NULL						},

	{ NULL,						NULL						}
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

	if (!name)
		return NULL;

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

	if (r_refRegInfo.inSequence)
		r_refRegInfo.shadersSearched++;

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

	// Select shader spot
	if (r_numShaders+1 >= MAX_SHADERS)
		Com_Error (ERR_DROP, "R_NewShader: MAX_SHADERS");
	shader = &r_shaderList[r_numShaders];
	memset (shader, 0, sizeof (shader_t));

	// Clear passes
	r_numCurrPasses = 0;
	memset (&r_currPasses, 0, sizeof (r_currPasses));

	// Strip extension
	Com_StripExtension (shader->name, sizeof (shader->name), fixedName);
	Q_strlwr (shader->name);

	// Defaults
	shader->pathType = pathType;
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
static void R_FinishShader (shader_t *shader)
{
	shaderPass_t	*pass;
	int				size, i;
	byte			*buffer;

	shader->numPasses = r_numCurrPasses;

	// Fill deforms
	if (shader->numDeforms) {
		shader->deforms = Mem_PoolAllocExt (shader->numDeforms * sizeof (vertDeform_t), qFalse, MEMPOOL_SHADERSYS, 0);
		memcpy (shader->deforms, r_currDeforms, shader->numDeforms * sizeof (vertDeform_t));
	}

	// Allocate full block
	size = shader->numPasses * sizeof (shaderPass_t);
	for (i=0 ; i<shader->numPasses ; i++)
		size += r_currPasses[i].numTCMods * sizeof (tcMod_t);
	buffer = Mem_PoolAllocExt (size, qFalse, MEMPOOL_SHADERSYS, 0);

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

		// Check depthWrite
		if (!(pass->flags & SHADER_PASS_DEPTHWRITE) && !(shader->flags & SHADER_SKY)) {
			pass->flags |= SHADER_PASS_DEPTHWRITE;
			shader->flags |= SHADER_DEPTHWRITE;
		}
	}

	// Calculate total component masks (for faster comparisons later on)
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++)
		pass->totalMask = pass->maskRed + pass->maskGreen + pass->maskBlue + pass->maskAlpha;

	// Check sort
	if (!shader->sortKey)
		shader->sortKey = SHADER_SORT_OPAQUE;

	// Check depthWrite
	if (shader->flags & SHADER_SKY && shader->flags & SHADER_DEPTHWRITE)
		shader->flags &= ~SHADER_DEPTHWRITE;

	// Check sizeBase
	if (shader->sizeBase >= shader->numPasses && shader->numPasses) {
		Shader_Printf (PRNT_WARNING, "WARNING: Shader '%s' has an invalid sizeBase value of: '%i', forcing: '0'\n", shader->name, shader->sizeBase);
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
static shaderPass_t *R_NewPass (shader_t *shader, parse_t *ps)
{
	shaderPass_t	*pass;

	if (r_numCurrPasses+1 >= MAX_SHADER_PASSES) {
		Shader_Printf (PRNT_WARNING, "WARNING: %s: too many passes, ignoring\n", Shader_PassLocation (shader, ps));
		// FIXME: test this...
		return NULL;
	}

	pass = &r_currPasses[r_numCurrPasses];
	memset (pass, 0, sizeof (shaderPass_t));

	// Defaults
	pass->depthFunc = GL_LEQUAL;

	return pass;
}


/*
==================
R_FinishPass
==================
*/
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
	if (!pass->names[0][0] && !(pass->flags & SHADER_PASS_LIGHTMAP)) {
		pass->blendMode = 0;
		return;
	}

	// Multitexture specific blending
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
}


/*
==================
R_LoadShaderFile
==================
*/
static qBool R_ShaderParseTok (shader_t *shader, shaderPass_t *pass, parse_t *ps, shaderKey_t *keys, char *token)
{
	shaderKey_t	*key;
	char		keyName[MAX_TOKEN_CHARS];
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
				Shader_SkipLine (ps);
				return qTrue;
			}
		}

		// See if it matches the keyword
		if (strcmp (key->keyWord, keyName))
			continue;

		// This is for keys that compilers and editors use
		if (!key->func) {
			Shader_SkipLine (ps);
			return qTrue;
		}

		// Failed to parse line
		if (!key->func (shader, pass, ps)) {
			Shader_SkipLine (ps);
			return qFalse;
		}

		// Report any extra parameters
		str = Shader_ParseString (ps);
		if (str[0]) {
			if (pass)
				Shader_Printf (PRNT_WARNING, "WARNING: %s: unused trailing parameters after key: '%s'\n", Shader_PassLocation (shader, ps), keyName);
			else
				Shader_Printf (PRNT_WARNING, "WARNING: %s: unused trailing parameters after key: '%s'\n", Shader_BaseLocation (shader, ps), keyName);
			Shader_SkipLine (ps);
			return qTrue;
		}

		// Parsed fine
		return qTrue;
	}

	// Not found
	if (pass)
		Shader_Printf (PRNT_ERROR, "ERROR: %s: unrecognized key: '%s'\n", Shader_PassLocation (shader, ps), keyName);
	else
		Shader_Printf (PRNT_ERROR, "ERROR: %s: unrecognized key: '%s'\n", Shader_BaseLocation (shader, ps), keyName);
	Shader_SkipLine (ps);
	return qFalse;
}
static void R_LoadShaderFile (char *fixedName, shPathType_t pathType)
{
	char			*fileBuffer, *buf;
	char			shaderName[MAX_QPATH];
	int				fileLen;
	qBool			inShader;
	qBool			inPass;
	char			*token;
	shader_t		*shader;
	shaderPass_t	*pass;
	int				numSlashes, i;
	parse_t			*ps;

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
	Shader_Printf (0, "...loading '%s' (%s)\n", fixedName, pathType == SHADER_PATHTYPE_BASEDIR ? "base" : "game");
	fileLen = FS_LoadFile (fixedName, (void **)&fileBuffer);
	if (!fileBuffer || fileLen <= 0) {
		Shader_Printf (PRNT_ERROR, "...ERROR: couldn't load '%s' -- %s\n", fixedName, !fileBuffer ? "unable to open" : "no data");
		return;
	}

	// Copy the buffer, and make certain it's newline and null terminated
	buf = (char *)Mem_PoolAllocExt (fileLen+2, qFalse, MEMPOOL_SHADERSYS, 0);
	memcpy (buf, fileBuffer, fileLen);
	buf[fileLen] = '\n';
	buf[fileLen+1] = '\0';

	// Don't need this anymore
	FS_FreeFile (fileBuffer);
	fileBuffer = buf;

	// Start parsing
	inShader = qFalse;
	inPass = qFalse;

	shader = NULL;
	pass = NULL;

	ps = Com_BeginParseSession (&fileBuffer);
	token = Com_ParseExt (ps, qTrue);
	for ( ; ; ) {
		if (!token[0])
			break;

		if (inShader) {
			switch (token[0]) {
			case '{':
				if (!inPass) {
					inPass = qTrue;
					pass = R_NewPass (shader, ps);
				}
				break;

			case '}':
				if (inPass) {
					inPass = qFalse;
					R_FinishPass (shader, pass);
					break;
				}

				inShader = qFalse;
				R_FinishShader (shader);
				break;

			default:
				if (inPass) {
					if (pass)
						R_ShaderParseTok (shader, pass, ps, r_shaderPassKeys, token);
					break;
				}

				R_ShaderParseTok (shader, NULL, ps, r_shaderBaseKeys, token);
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

		token = Com_ParseExt (ps, qTrue);
	}

	if (inShader && shader) {
		Shader_Printf (PRNT_ERROR, "...ERROR: unexpected EOF in '%s' after shader '%s', forcing finish\n", fixedName, shader->name);
		R_FinishShader (shader);
	}

	// Done
	Com_EndParseSession (ps);
	Mem_Free (buf);
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
		pass->fragProg = NULL;
		pass->vertProg = NULL;
		for (j=0 ; j<pass->numNames ; j++)
			pass->animFrames[j] = NULL;
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

	shader->touchFrame = r_refRegInfo.registerFrame;
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		pass->animNumFrames = 0;
		for (j=0 ; j<pass->numNames ; j++) {
			// Fragment program
			if (pass->flags & SHADER_PASS_FRAGMENTPROGRAM) {
				if (pass->fragProg) {
					R_TouchProgram (pass->fragProg);
				}
				else {
					pass->fragProg = R_RegisterProgram (pass->fragProgName, qTrue);

					if (!pass->fragProg) {
						Shader_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load fragment program '%s' for pass #%i (anim #%i)\n",
							shader->name, pass->fragProgName, i+1, j+1);
						pass->flags &= ~SHADER_PASS_FRAGMENTPROGRAM;
					}
				}
			}

			// Vertex program
			if (pass->flags & SHADER_PASS_VERTEXPROGRAM) {
				if (pass->vertProg) {
					R_TouchProgram (pass->vertProg);
				}
				else {
					pass->vertProg = R_RegisterProgram (pass->vertProgName, qFalse);

					if (!pass->vertProg) {
						Shader_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load vertex program '%s' for pass #%i (anim #%i)\n",
							shader->name, pass->vertProgName, i+1, j+1);
						pass->flags &= ~SHADER_PASS_VERTEXPROGRAM;
					}
				}
			}

			// No textures stored for lightmap passes
			if (pass->flags & SHADER_PASS_LIGHTMAP) {
				pass->animNumFrames++;
				continue;
			}

			// Texture
			if (pass->names[j][0] == '$') {
				// White texture
				if (!Q_stricmp (pass->names[j], "$white")) {
					pass->animFrames[pass->animNumFrames] = r_whiteTexture;
					pass->animNumFrames++;
					continue;
				}

				// Black texture
				if (!Q_stricmp (pass->names[j], "$black")) {
					pass->animFrames[pass->animNumFrames] = r_blackTexture;
					pass->animNumFrames++;
					continue;
				}
			}

			// If it's already loaded, touch it
			if (pass->animFrames[pass->animNumFrames]) {
				R_TouchImage (pass->animFrames[pass->animNumFrames]);
				pass->animNumFrames++;
				continue;
			}

			// Nope, load it
			pass->animFrames[pass->animNumFrames] = R_RegisterImage (pass->names[j], shader->addTexFlags|pass->texFlags);

			// See if it loaded
			if (pass->animFrames[pass->animNumFrames]) {
				pass->animNumFrames++;
				continue;
			}

			// Report any errors
			pass->animFrames[pass->animNumFrames] = NULL;
			if (pass->names[j][0])
				Shader_Printf (PRNT_WARNING, "WARNING: Shader '%s' can't find/load '%s' for pass #%i (anim #%i)\n", shader->name, pass->names[j], i+1, j+1);
			else
				Shader_Printf (PRNT_WARNING, "WARNING: Shader '%s' has a NULL texture name for pass #%i (anim #%i)\n", shader->name, i+1, j+1);
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

	if (!name)
		return NULL;

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

	// Default shader flags
	switch (shaderType) {
	case SHADER_PIC:
		texFlags = IF_NOMIPMAP_LINEAR|IF_NOPICMIP|IF_NOINTENS;
		if (!vid_gammapics->integer)
			texFlags |= IF_NOGAMMA;
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
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_ENTITY;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->texFlags = texFlags;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_LIGHTING_DIFFUSE;
		pass->alphaGen.type = ALPHA_GEN_ENTITY;
		pass->tcGen = TC_GEN_BASE;

		pass->animFrames[pass->animNumFrames++] = image;
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
		pass->flags = SHADER_PASS_BLEND;
		pass->texFlags = texFlags;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		pass->tcGen = TC_GEN_BASE;

		pass->animFrames[pass->animNumFrames++] = image;
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
		pass->flags = SHADER_PASS_BLEND;
		pass->texFlags = texFlags;
		pass->blendSource = GL_SRC_ALPHA;
		pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_EXACT_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_VERTEX;
		pass->tcGen = TC_GEN_BASE;

		pass->animFrames[pass->animNumFrames++] = image;
		break;

	case SHADER_SKYBOX:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS;
		shader->flags = SHADER_DEPTHWRITE|SHADER_SKY|SHADER_NOMARK;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sortKey = SHADER_SORT_SKY;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_REPLACE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;

		pass->animFrames[pass->animNumFrames++] = image;
		break;

	default:
	case SHADER_BSP:
		if (surfParams > 0 && surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66|SHADER_SURF_WARP|SHADER_SURF_LIGHTMAP)) {
			if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66|SHADER_SURF_WARP)) {
				shader->cullType = SHADER_CULL_FRONT;
				shader->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
				shader->flags = SHADER_DEPTHWRITE;

				// Add flowing if it's got the flag
				if (surfParams & SHADER_SURF_FLOWING) {
					buffer = Mem_PoolAlloc (sizeof (shaderPass_t) + sizeof (tcMod_t), MEMPOOL_SHADERSYS, 0);
					shader->passes = (shaderPass_t *)buffer;
					buffer += sizeof (shaderPass_t);
				}
				else {
					shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
				}
				shader->numPasses = 1;

				// Sort key
				if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66))
					shader->sortKey = SHADER_SORT_WATER;
				else
					shader->sortKey = SHADER_SORT_OPAQUE;

				pass = &shader->passes[0];
				Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
				pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
				pass->texFlags = texFlags;
				pass->depthFunc = GL_LEQUAL;
				pass->blendMode = GL_MODULATE;

				// Subdivide if warping
				if (surfParams & SHADER_SURF_WARP) {
					shader->flags |= SHADER_SUBDIVIDE|SHADER_ENTITY_MERGABLE|SHADER_NOMARK;
					shader->subdivide = 64;
					pass->tcGen = TC_GEN_WARP;
				}
				else {
					pass->tcGen = TC_GEN_BASE;
				}

				pass->rgbGen.type = RGB_GEN_IDENTITY_LIGHTING;
				if (surfParams & (SHADER_SURF_TRANS33|SHADER_SURF_TRANS66)) {
					// This caused problems with certain trans surfaces
					// Apparently Q2bsp generates vertices for both sides of trans surfaces?!
					//shader->cullType = SHADER_CULL_NONE;
					pass->flags |= SHADER_PASS_BLEND;
					pass->blendSource = GL_SRC_ALPHA;
					pass->blendDest = GL_ONE_MINUS_SRC_ALPHA;
					pass->alphaGen.type = ALPHA_GEN_CONST;
					pass->alphaGen.args[0] = (surfParams & SHADER_SURF_TRANS33) ? 0.33f : 0.66f;
				}
				else {
					pass->alphaGen.type = ALPHA_GEN_IDENTITY;
				}

				// Add flowing if it's got the flag
				if (surfParams & SHADER_SURF_FLOWING) {
					pass->tcMods = (tcMod_t *)buffer;
					pass->numTCMods = 1;

					tcMod = &pass->tcMods[0];
					tcMod->type = TC_MOD_SCROLL;
					tcMod->args[0] = -0.5f;
					tcMod->args[1] = 0;
				}

				pass->animFrames[pass->animNumFrames++] = image;
			}
			else {
				shader->cullType = SHADER_CULL_FRONT;
				shader->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
				shader->flags = SHADER_DEPTHWRITE;

				// Add flowing if it's got the flag
				if (surfParams & SHADER_SURF_FLOWING) {
					buffer = Mem_PoolAlloc (sizeof (shaderPass_t) * 2 + sizeof (tcMod_t), MEMPOOL_SHADERSYS, 0);
					shader->passes = (shaderPass_t *)buffer;
					buffer += sizeof (shaderPass_t) * 2;
				}
				else {
					shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t) * 2, MEMPOOL_SHADERSYS, 0);
				}
				shader->numPasses = 2;
				shader->sizeBase = 1;
				shader->sortKey = SHADER_SORT_OPAQUE;

				pass = &shader->passes[0];
				Q_strncpyz (pass->names[pass->numNames++], "$lightmap", sizeof (pass->names[0]));
				pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
				pass->tcGen = TC_GEN_LIGHTMAP;
				pass->depthFunc = GL_LEQUAL;
				pass->blendSource = GL_ONE;
				pass->blendDest = GL_ZERO;
				pass->blendMode = GL_REPLACE;
				pass->rgbGen.type = RGB_GEN_IDENTITY;
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;

				pass = &shader->passes[1];
				Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
				pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
				pass->texFlags = texFlags;
				pass->tcGen = TC_GEN_BASE;
				pass->depthFunc = GL_LEQUAL;
				pass->blendSource = GL_ZERO;
				pass->blendDest = GL_SRC_COLOR;
				pass->blendMode = GL_MODULATE;
				pass->rgbGen.type = RGB_GEN_IDENTITY;
				pass->alphaGen.type = ALPHA_GEN_IDENTITY;

				// Add flowing if it's got the flag
				if (surfParams & SHADER_SURF_FLOWING) {
					pass->tcMods = (tcMod_t *)buffer;
					pass->numTCMods = 1;

					tcMod = &pass->tcMods[0];
					tcMod->type = TC_MOD_SCROLL;
					tcMod->args[0] = -0.5f;
					tcMod->args[1] = 0;
				}

				pass->animFrames[pass->animNumFrames++] = image;
			}
		}
		else {
			shader->cullType = SHADER_CULL_FRONT;
			shader->features = MF_STCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
			shader->flags = SHADER_DEPTHWRITE;
			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & SHADER_SURF_FLOWING) {
				buffer = Mem_PoolAlloc (sizeof (shaderPass_t) + sizeof (tcMod_t), MEMPOOL_SHADERSYS, 0);
				shader->passes = (shaderPass_t *)buffer;
				buffer += sizeof (shaderPass_t);
			}
			else {
				shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
			}
			shader->numPasses = 1;
			shader->sizeBase = 0;
			shader->sortKey = SHADER_SORT_OPAQUE;

			pass = &shader->passes[0];
			Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
			pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_NOCOLORARRAY;
			pass->texFlags = texFlags;
			pass->tcGen = TC_GEN_BASE;
			pass->depthFunc = GL_LEQUAL;
			pass->blendMode = GL_REPLACE;
			pass->rgbGen.type = RGB_GEN_IDENTITY;
			pass->alphaGen.type = ALPHA_GEN_IDENTITY;

			// Add flowing if it's got the flag
			if (surfParams > 0 && surfParams & SHADER_SURF_FLOWING) {
				pass->tcMods = (tcMod_t *)buffer;
				pass->numTCMods = 1;

				tcMod = &pass->tcMods[0];
				tcMod->type = TC_MOD_SCROLL;
				tcMod->args[0] = -0.5f;
				tcMod->args[1] = 0;
			}

			pass->animFrames[pass->animNumFrames++] = image;
		}
		break;

	case SHADER_BSP_FLARE:
		shader->cullType = SHADER_CULL_NONE;
		shader->features = MF_STCOORDS|MF_COLORS|MF_STATIC_MESH;
		shader->flags = SHADER_FLARE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sizeBase = 0;
		shader->sortKey = SHADER_SORT_ADDITIVE;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ONE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;

		pass->animFrames[pass->animNumFrames++] = image;
		break;

	case SHADER_BSP_VERTEX:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_COLORS|MF_TRNORMALS|MF_STATIC_MESH;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t), MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 1;
		shader->sizeBase = 0;
		shader->sortKey = SHADER_SORT_OPAQUE;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->blendMode = GL_MODULATE;
		pass->depthFunc = GL_LEQUAL;
		pass->rgbGen.type = RGB_GEN_VERTEX;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;

		pass->animFrames[pass->animNumFrames++] = image;
		break;

	case SHADER_BSP_LM:
		shader->cullType = SHADER_CULL_FRONT;
		shader->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS|MF_STATIC_MESH;
		shader->flags = SHADER_DEPTHWRITE;
		shader->passes = Mem_PoolAlloc (sizeof (shaderPass_t) * 2, MEMPOOL_SHADERSYS, 0);
		shader->numPasses = 2;
		shader->sizeBase = 1;
		shader->sortKey = SHADER_SORT_OPAQUE;

		pass = &shader->passes[0];
		Q_strncpyz (pass->names[pass->numNames++], "$lightmap", sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_DEPTHWRITE|SHADER_PASS_LIGHTMAP|SHADER_PASS_NOCOLORARRAY;
		pass->tcGen = TC_GEN_LIGHTMAP;
		pass->depthFunc = GL_LEQUAL;
		pass->blendSource = GL_ONE;
		pass->blendDest = GL_ZERO;
		pass->blendMode = GL_REPLACE;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;

		pass = &shader->passes[1];
		Q_strncpyz (pass->names[pass->numNames++], fixedName, sizeof (pass->names[0]));
		pass->flags = SHADER_PASS_BLEND|SHADER_PASS_NOCOLORARRAY;
		pass->texFlags = texFlags;
		pass->tcGen = TC_GEN_BASE;
		pass->depthFunc = GL_LEQUAL;
		pass->blendSource = GL_ZERO;
		pass->blendDest = GL_SRC_COLOR;
		pass->blendMode = GL_MODULATE;
		pass->rgbGen.type = RGB_GEN_IDENTITY;
		pass->alphaGen.type = ALPHA_GEN_IDENTITY;

		pass->animFrames[pass->animNumFrames++] = image;
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
		if (!(shader->flags & SHADER_NOFLUSH) && shader->touchFrame != r_refRegInfo.registerFrame) {
			R_UntouchShader (shader);
			r_refRegInfo.shadersReleased++;
			continue;	// Not touched
		}

		R_ReadyShader (shader);
		r_refRegInfo.shadersTouched++;
	}
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
	uint32		i, j;
	uint32		total;

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

static void	*cmd_shaderCheck;
static void	*cmd_shaderList;

/*
==================
R_ShaderInit
==================
*/
void R_ShaderInit (void)
{
	char			*shaderList[MAX_SHADERS];
	char			fixedName[MAX_QPATH];
	int				numShaders, i;
	shPathType_t	pathType;
	char			*name;

	Com_Printf (0, "\n-------- Shader Initialization ---------\n");

	// Clear lists
	r_numShaders = 0;
	memset (&r_shaderList, 0, sizeof (shader_t) * MAX_SHADERS);
	memset (r_shaderHashTree, 0, sizeof (shader_t *) * MAX_SHADER_HASH);

	// Console commands
	cmd_shaderCheck	= Cmd_AddCommand ("shadercheck",	R_ShaderCheck_f,	"Shader system integrity check");
	cmd_shaderList	= Cmd_AddCommand ("shaderlist",		R_ShaderList_f,		"Prints to the console a list of loaded shaders");

	// Load scripts
	r_numShaderErrors = 0;
	r_numShaderWarnings = 0;
	numShaders = FS_FindFiles ("scripts", "*scripts/*.shd", "shd", shaderList, MAX_SHADERS, qTrue, qFalse);
	for (i=0 ; i<numShaders ; i++) {
		// Fix the path
		Com_NormalizePath (fixedName, sizeof (fixedName), shaderList[i]);

		// Skip the path
		name = strstr (fixedName, "/scripts/");
		if (!name)
			continue;	// This shouldn't happen...
		name++;	// Skip the initial '/'

		// Base dir shader?
		if (strstr (shaderList[i], BASE_MODDIRNAME "/"))
			pathType = SHADER_PATHTYPE_BASEDIR;
		else
			pathType = SHADER_PATHTYPE_MODDIR;

		R_LoadShaderFile (name, pathType);
	}
	FS_FreeFileList (shaderList, numShaders);

	// Shader counterparts
	r_noShader = R_RegisterShader (r_noTexture->name, qTrue, SHADER_BSP, -1);
	r_noShaderLightmap = R_RegisterShader (r_noTexture->name, qTrue, SHADER_BSP, SHADER_SURF_LIGHTMAP);
	r_noShaderSky = R_RegisterShader (r_noTexture->name, qTrue, SHADER_SKYBOX, -1);
	r_whiteShader = R_RegisterShader (r_whiteTexture->name, qTrue, SHADER_BSP, -1);
	r_blackShader = R_RegisterShader (r_blackTexture->name, qTrue, SHADER_BSP, -1);

	r_noShader->flags |= SHADER_NOFLUSH;
	r_noShaderLightmap->flags |= SHADER_NOFLUSH;
	r_noShaderSky->flags |= SHADER_NOFLUSH;
	r_whiteShader->flags |= SHADER_NOFLUSH;
	r_blackShader->flags |= SHADER_NOFLUSH;

	// Check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_SHADERSYS);

	Com_Printf (0, "SHADERS - %i error(s), %i warning(s)\n", r_numShaderErrors, r_numShaderWarnings);
	Com_Printf (0, "----------------------------------------\n");
}


/*
==================
R_ShaderShutdown
==================
*/
void R_ShaderShutdown (void)
{
	// Remove commands
	Cmd_RemoveCommand ("shadercheck", cmd_shaderCheck);
	Cmd_RemoveCommand ("shaderlist", cmd_shaderList);

	// Free all loaded shaders
	Mem_FreePool (MEMPOOL_SHADERSYS);
}
