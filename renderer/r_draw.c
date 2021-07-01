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
// r_draw.c
// 2D stuff
//

#include "r_local.h"

/*
=============================================================================

	2D IMAGE RENDERING

=============================================================================
*/

static mesh_t		r_picMesh;
static meshBuffer_t	r_picMBuffer;

static vec3_t		r_picVertices[4];
static vec3_t		r_picNormals[4] = { {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0} };
static vec2_t		r_picTexCoords[4];
static bvec4_t		r_picColors[4];

/*
=============
R_DrawPic
=============
*/
void R_DrawPic (shader_t *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color)
{
	int		bColor;

	if (!shader)
		return;

	r_picColors[0][0] = (color[0] * 255);
	r_picColors[0][1] = (color[1] * 255);
	r_picColors[0][2] = (color[2] * 255);
	r_picColors[0][3] = (color[3] * 255);
	bColor = *(int *)r_picColors[0];

	r_picVertices[0][0] = x;
	r_picVertices[0][1] = y;
	r_picVertices[0][2] = 1;
	r_picTexCoords[0][0] = s1;
	r_picTexCoords[0][1] = t1;

	r_picVertices[1][0] = x+w;
	r_picVertices[1][1] = y;
	r_picVertices[1][2] = 1;
	r_picTexCoords[1][0] = s2;
	r_picTexCoords[1][1] = t1;
	*(int *)r_picColors[1] = bColor;

	r_picVertices[2][0] = x+w;
	r_picVertices[2][1] = y+h;
	r_picVertices[2][2] = 1;
	r_picTexCoords[2][0] = s2;
	r_picTexCoords[2][1] = t2;
	*(int *)r_picColors[2] = bColor;

	r_picVertices[3][0] = x;
	r_picVertices[3][1] = y+h;
	r_picVertices[3][2] = 1;
	r_picTexCoords[3][0] = s1;
	r_picTexCoords[3][1] = t2;
	*(int *)r_picColors[3] = bColor;

	r_picMesh.numIndexes = QUAD_INDEXES;
	r_picMesh.numVerts = 4;

	r_picMesh.colorArray = r_picColors;
	r_picMesh.coordArray = r_picTexCoords;
	r_picMesh.indexArray = rb_quadIndices;
	r_picMesh.lmCoordArray = NULL;
	r_picMesh.normalsArray = r_picNormals;
	r_picMesh.sVectorsArray = NULL;
	r_picMesh.tVectorsArray = NULL;
	r_picMesh.trNeighborsArray = NULL;
	r_picMesh.trNormalsArray = NULL;
	r_picMesh.vertexArray = r_picVertices;

	r_picMBuffer.sortKey = -1;
	r_picMBuffer.entity = NULL;
	r_picMBuffer.shader = shader;
	r_picMBuffer.mesh = NULL;

	RB_PushMesh (&r_picMesh, MF_NONBATCHED | MF_TRIFAN | shader->features | (gl_shownormals->integer ? MF_NORMALS : 0));
	RB_RenderMeshBuffer (&r_picMBuffer, qFalse);
}

/*
=============================================================================

	FONT RENDERING

=============================================================================
*/

/*
===============
R_CheckFont
===============
*/
void R_CheckFont (void)
{
	// Load console characters
	con_font->modified = qFalse;

	glMedia.charShader = R_RegisterPic (Q_VarArgs ("fonts/%s.tga", con_font->string));
	if (!glMedia.charShader)
		glMedia.charShader = R_RegisterPic ("pics/conchars.tga");
}


/*
================
R_DrawString
================
*/
int R_DrawString (shader_t *charShader, float x, float y, int flags, float scale, char *string, vec4_t color)
{
	int			num, i;
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor, strColor;
	qBool		isShadowed;
	qBool		skipNext = qFalse;
	qBool		inColorCode = qFalse;
	shader_t	*shader;

	Vector4Copy (color, strColor);

	if (flags & FS_SHADOW)
		isShadowed = qTrue;
	else
		isShadowed = qFalse;

	VectorCopy (colorBlack, shdColor);
	shdColor[3] = strColor[3];

	if (!charShader)
		shader = glMedia.charShader;
	else
		shader = charShader;

	ftWidth = 8.0f * scale;
	ftHeight = 8.0f * scale;

	for (i=0 ; *string ; ) {
		if ((flags & FS_SECONDARY) && (*string < 128))
			*string |= 128;

		if (skipNext)
			skipNext = qFalse;
		else if (Q_IsColorString (string)) {
			switch (string[1] & 127) {
			case Q_COLOR_ESCAPE:
				string++;
				skipNext = qTrue;
				continue;

			case 'r':
			case 'R':
				isShadowed = qFalse;
				inColorCode = qFalse;
				VectorCopy (colorWhite, strColor);
				string += 2;
				continue;

			case 's':
			case 'S':
				isShadowed = !isShadowed;
				string += 2;
				continue;

			case COLOR_BLACK:
			case COLOR_RED:
			case COLOR_GREEN:
			case COLOR_YELLOW:
			case COLOR_BLUE:
			case COLOR_CYAN:
			case COLOR_MAGENTA:
			case COLOR_WHITE:
			case COLOR_GREY:
				VectorCopy (strColorTable[Q_StrColorIndex (string[1])], strColor);
				inColorCode = qTrue;

			default:
				string += 2;
				continue;
			}
		}

		num = *string++;
		if (inColorCode)
			num &= 127;
		else
			num &= 255;

		// Skip spaces
		if ((num&127) != 32) {
			frow = (num>>4) * (1.0f/16.0f);
			fcol = (num&15) * (1.0f/16.0f);

			if (isShadowed)
				R_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

			R_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), strColor);
		}

		x += ftWidth;
		i++;
	}

	return i;
}


/*
================
R_DrawStringLen
================
*/
int R_DrawStringLen (shader_t *charShader, float x, float y, int flags, float scale, char *string, int len, vec4_t color)
{
	char	swap;
	int		length;

	if (len < 0)
		return R_DrawString (charShader, x, y, flags, scale, string, color);

	swap = string[len];
	string[len] = 0;
	length = R_DrawString (charShader, x, y, flags, scale, string, color);
	string[len] = swap;

	return length;
}


/*
================
R_DrawChar
================
*/
void R_DrawChar (shader_t *charShader, float x, float y, int flags, float scale, int num, vec4_t color)
{
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor;
	shader_t	*shader;

	if (flags & FS_SHADOW) {
		VectorCopy (colorBlack, shdColor);
		shdColor[3] = color[3];
	}

	if (!charShader)
		shader = glMedia.charShader;
	else
		shader = charShader;

	ftWidth = 8.0f * scale;
	ftHeight = 8.0f * scale;

	if ((flags & FS_SECONDARY) && (num < 128))
		num |= 128;
	num &= 255;

	if ((num&127) == 32)
		return;	// Space

	frow = (num>>4) * (1.0f/16.0f);
	fcol = (num&15) * (1.0f/16.0f);

	if (flags & FS_SHADOW)
		R_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

	R_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), color);
}

/*
=============================================================================

	MISC

=============================================================================
*/

/*
=============
R_DrawFill

Fills a box of pixels with a single color
=============
*/
void R_DrawFill (float x, float y, int w, int h, vec4_t color)
{
	if (color[3] < 1.0f)
		GL_Enable (GL_BLEND);

	GL_Disable (GL_TEXTURE_2D);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglColor4f (color[0], color[1], color[2], color[3]);

	qglBegin (GL_QUADS);

	qglVertex2f (x, y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();

	qglColor4f (1, 1, 1, 1);
	GL_Enable (GL_TEXTURE_2D);

	if (color[3] < 1.0f)
		GL_Disable (GL_BLEND);
}
