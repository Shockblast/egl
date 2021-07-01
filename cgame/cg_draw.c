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
// cg_draw.c
//

#include "cg_local.h"

/*
=============
CG_DrawChar
=============
*/
void CG_DrawChar (float x, float y, int flags, float scale, int num, vec4_t color)
{
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor;
	struct shader_s		*shader;

	if (flags & FS_SHADOW) {
		VectorCopy (colorBlack, shdColor);
		shdColor[3] = color[3];
	}

	shader = cgMedia.conCharsShader;

	ftWidth = 8.0 * scale;
	ftHeight = 8.0 * scale;

	if ((flags & FS_SECONDARY) && (num < 128))
		num |= 128;
	num &= 255;

	if ((num&127) == 32)
		return;	// Space

	frow = (num>>4) * (1.0f/16.0f);
	fcol = (num&15) * (1.0f/16.0f);

	if (flags & FS_SHADOW)
		cgi.R_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

	cgi.R_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), color);
}


/*
=============
CG_DrawString
=============
*/
int CG_DrawString (float x, float y, int flags, float scale, char *string, vec4_t color)
{
	int			num, i;
	float		frow, fcol;
	float		ftWidth, ftHeight;
	vec4_t		shdColor, strColor;
	qBool		isShadowed;
	qBool		skipNext = qFalse;
	qBool		inColorCode = qFalse;
	struct shader_s		*shader;

	Vector4Copy (color, strColor);

	if (flags & FS_SHADOW)
		isShadowed = qTrue;
	else
		isShadowed = qFalse;

	VectorCopy (colorBlack, shdColor);
	shdColor[3] = strColor[3];

	shader = cgMedia.conCharsShader;

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

		// Skip spaces
		if ((num&127) != 32) {
			frow = (num>>4) * (1.0f/16.0f);
			fcol = (num&15) * (1.0f/16.0f);

			if (isShadowed)
				cgi.R_DrawPic (shader, x+FT_SHAOFFSET, y+FT_SHAOFFSET, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), shdColor);

			cgi.R_DrawPic (shader, x, y, ftWidth, ftHeight, fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), strColor);
		}

		x += ftWidth;
		i++;
	}

	return i;
}


/*
================
CG_DrawStringLen
================
*/
int CG_DrawStringLen (float x, float y, int flags, float scale, char *string, int len, vec4_t color)
{
	char	swap;
	int		length;

	if (len < 0)
		return CG_DrawString (x, y, flags, scale, string, color);

	swap = string[len];
	string[len] = 0;
	length = CG_DrawString (x, y, flags, scale, string, color);
	string[len] = swap;

	return length;
}


/*
================
CG_DrawModel
================
*/
void CG_DrawModel (int x, int y, int w, int h, struct model_s *model, struct shader_s *skin, vec3_t origin, vec3_t angles)
{
	refDef_t	refDef;
	entity_t	entity;

	if (!model)
		return;

	// Set up refDef
	memset (&refDef, 0, sizeof (refDef));

	refDef.x = x;
	refDef.y = y;
	refDef.width = w;
	refDef.height = h;
	refDef.fovX = 30;
	refDef.fovY = 30;
	refDef.time = cg.refreshTime * 0.001;
	refDef.rdFlags = RDF_NOWORLDMODEL;

	// Set up the entity
	memset (&entity, 0, sizeof (entity));

	entity.model = model;
	entity.skin = skin;
	entity.scale = 1.0f;
	entity.flags = RF_FULLBRIGHT | RF_NOSHADOW | RF_FORCENOLOD;
	VectorCopy (origin, entity.origin);
	VectorCopy (entity.origin, entity.oldOrigin);

	AnglesToAxis (angles, entity.axis);

	cgi.R_ClearScene ();
	cgi.R_AddEntity (&entity);
	cgi.R_RenderScene (&refDef);
}
