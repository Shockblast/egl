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

/*
=============
Draw_ShaderPic
=============
*/
void Draw_ShaderPic (shader_t *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color) {
	float			scralpha;
	vec2_t			coords, scrollCoords;
	image_t			*stageImage;
	shaderStage_t	*stage;

	if (!shader)
		return;

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ToggleOverbrights (qTrue);

	stage = shader->stage;
	do {
		if (stage->lightMapOnly)
			stageImage = r_WhiteTexture;
		else if (stage->animCount) {
			image_t		*animImg = RS_Animate (stage);
			if (!animImg)
				continue;
			else
				stageImage = animImg;
		} else if (!stage->texture)
			continue;
		else
			stageImage = stage->texture;

		RS_Scroll (stage, scrollCoords);
		RS_BlendFunc (stage, (color[3] < 1) ? qTrue:qFalse, qTrue);
		RS_AlphaShift (stage, &scralpha);

		scralpha = (scralpha * color[3]);
		if (scralpha <= 0) continue;

		if (stage->lightMap)
			qglColor4f (color[0], color[1], color[2], scralpha);
		else
			qglColor4f (1, 1, 1, scralpha);

		if (stage->alphaMask)
			qglEnable (GL_ALPHA_TEST);

		if (stage->envMap) {
			qglTexGenf (GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
			qglTexGenf (GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

			qglEnable (GL_TEXTURE_GEN_S);
			qglEnable (GL_TEXTURE_GEN_T);
		}

		if (stage->cubeMap) {
			qglEnable (GL_TEXTURE_CUBE_MAP_ARB);

			qglEnable (GL_TEXTURE_GEN_S);
			qglEnable (GL_TEXTURE_GEN_T);
			qglEnable (GL_TEXTURE_GEN_R);
			qglTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
			qglTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
			qglTexGeni (GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

			glSpeeds.shaderCMPasses++;
		}

		GL_Bind (0, stageImage);

		qglBegin (GL_QUADS);

		coords[0] = s1; coords[1] = t1;
		RS_SetTexcoords2D (stage, coords);
		qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
		qglVertex2f (x, y);

		coords[0] = s2; coords[1] = t1;
		RS_SetTexcoords2D (stage, coords);
		qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
		qglVertex2f (x+w, y);

		coords[0] = s2; coords[1] = t2;
		RS_SetTexcoords2D (stage, coords);
		qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
		qglVertex2f (x+w, y+h);

		coords[0] = s1; coords[1] = t2;
		RS_SetTexcoords2D (stage, coords);
		qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
		qglVertex2f (x, y+h);

		qglEnd ();

		if (stage->cubeMap) {
			qglDisable (GL_TEXTURE_GEN_S);
			qglDisable (GL_TEXTURE_GEN_T);
			qglDisable (GL_TEXTURE_GEN_R);

			qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
		}

		if (stage->envMap) {
			qglDisable (GL_TEXTURE_GEN_S);
			qglDisable (GL_TEXTURE_GEN_T);
		}

		GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (stage->alphaMask)
			qglDisable (GL_ALPHA_TEST);

		glSpeeds.shaderPasses++;
		stage = stage->next;
	} while (stage);
	glSpeeds.shaderCount++;

	qglDisable (GL_BLEND);
	qglColor4f (1, 1, 1, 1);
	GL_ToggleOverbrights (qFalse);
}


/*
=============
Draw_ImagePic
=============
*/
void Draw_ImagePic (image_t *img, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color) {
	if (!img)
		return;

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_ToggleOverbrights (qTrue);

	qglEnable (GL_BLEND);
	qglColor4fv (color);

	GL_Bind (0, img);

	qglBegin (GL_QUADS);

	qglTexCoord2f (s1, t1);
	qglVertex2f (x, y);
	qglTexCoord2f (s2, t1);
	qglVertex2f (x+w, y);
	qglTexCoord2f (s2, t2);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f (s1, t2);
	qglVertex2f (x, y+h);

	qglEnd ();

	qglDisable (GL_BLEND);
	qglColor4f (1, 1, 1, 1);
	GL_ToggleOverbrights (qFalse);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (image_t *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color) {
	if (!image)
		return;

	if (r_shaders->integer) {
		shader_t	*shader = RS_RegisterShader (image->bareName);

		if (shader) {
			Draw_ShaderPic (shader, x, y, w, h, s1, t1, s2, t2, color);
			return;
		}
	}

	Draw_ImagePic (image, x, y, w, h, s1, t1, s2, t2, color);
}


/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (char *pic, float x, float y, int w, int h, vec4_t color) {
	image_t		*img;

	if (!pic[0])
		return;

	img = Img_RegisterPic (pic);
	if (!img) {
		Com_Printf (0, S_COLOR_YELLOW "ERROR: Can't find pic: %s\n", pic);
		return;
	}

	Draw_Pic (img, x, y, w, h, 0, 0, 1, 1, color);
}

/*
=============================================================================

	FONT RENDERING

=============================================================================
*/

/*
===============
Draw_CheckFont
===============
*/
void Draw_CheckFont (void) {
	// load console characters
	con_font->modified = qFalse;

	glMedia.charsImage = Img_FindImage (Q_VarArgs ("fonts/%s.pcx", con_font->string), IF_NOFLUSH|IF_NOMIPMAP|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_CLAMP);
	if (!glMedia.charsImage) {
		glMedia.charsImage = Img_FindImage ("pics/conchars.pcx", IF_NOFLUSH|IF_NOMIPMAP|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_CLAMP);
		if (!glMedia.charsImage)
			glMedia.charsImage = r_WhiteTexture;
	}
}


/*
================
Draw_String
================
*/
int Draw_String (image_t *charsImage, float x, float y, int flags, float scale, char *string, vec4_t color) {
	int			num, i;
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor, strColor;
	qBool		isShadowed;
	qBool		skipNext = qFalse;
	qBool		inColorCode = qFalse;
	image_t		*image;

	Vector4Copy (color, strColor);

	if (flags & FS_SHADOW)
		isShadowed = qTrue;
	else
		isShadowed = qFalse;

	VectorCopy (colorBlack, shdColor);
	shdColor[3] = strColor[3];

	if (!charsImage)
		image = glMedia.charsImage;
	else
		image = charsImage;

	ftWidth = 8.0 * scale;
	ftHeight = 8.0 * scale;

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

		// skip spaces
		if ((num&127) != 32) {
			frow = (num>>4) * ONEDIV16;
			fcol = (num&15) * ONEDIV16;

			if (isShadowed)
				Draw_ImagePic (image, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+ONEDIV16, frow+ONEDIV16, shdColor);

			Draw_ImagePic (image, x, y, ftWidth, ftHeight, fcol, frow, fcol+ONEDIV16, frow+ONEDIV16, strColor);
		}

		x += ftWidth;
		i++;
	}

	return i;
}


/*
================
Draw_StringLen
================
*/
int Draw_StringLen (image_t *charsImage, float x, float y, int flags, float scale, char *string, int len, vec4_t color) {
	char	swap;
	int		length;

	if (len < 0)
		return Draw_String (charsImage, x, y, flags, scale, string, color);

	swap = string[len];
	string[len] = 0;
	length = Draw_String (charsImage, x, y, flags, scale, string, color);
	string[len] = swap;

	return length;
}


/*
================
Draw_Char
================
*/
void Draw_Char (image_t *charsImage, float x, float y, int flags, float scale, int num, vec4_t color) {
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor;
	image_t		*image;

	if (flags & FS_SHADOW) {
		VectorCopy (colorBlack, shdColor);
		shdColor[3] = color[3];
	}

	if (!charsImage)
		image = glMedia.charsImage;
	else
		image = charsImage;

	ftWidth = 8.0 * scale;
	ftHeight = 8.0 * scale;

	if ((flags & FS_SECONDARY) && (num < 128))
		num |= 128;
	num &= 255;

	if ((num&127) == 32)
		return;		// space

	frow = (num>>4) * ONEDIV16;
	fcol = (num&15) * ONEDIV16;

	if (flags & FS_SHADOW)
		Draw_ImagePic (image, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+ONEDIV16, frow+ONEDIV16, shdColor);

	Draw_ImagePic (image, x, y, ftWidth, ftHeight, fcol, frow, fcol+ONEDIV16, frow+ONEDIV16, color);
}

/*
=============================================================================

	MISC

=============================================================================
*/

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (float x, float y, int w, int h, vec4_t color)
{
	if (color[3] < 1.0f)
		qglEnable (GL_BLEND);

	GL_ToggleOverbrights (qTrue);
	qglDisable (GL_TEXTURE_2D);
	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	qglColor4f (color[0], color[1], color[2], color[3]);

	qglBegin (GL_QUADS);

	qglVertex2f (x, y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();

	qglColor4f (1, 1, 1, 1);
	qglEnable (GL_TEXTURE_2D);
	GL_ToggleOverbrights (qFalse);

	if (color[3] < 1.0f)
		qglDisable (GL_BLEND);
}
