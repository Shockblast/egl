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
// rf_font.c
// FIXME TODO:
// - Hash optimize (though the table size should be pretty small, like 4).
// - Different materials for different size ranges (like smallMaterial fonts/quake3_sm.tga, mediumMaterial, largeMaterial, etc).
// - Per-character coords and width/height?
// - More font styles, italic, bold, etc...
// - shaderTime passed to font renders?
//	FS_ALIGN_CENTER			= 1 << 0,
//	FS_ALIGN_RIGHT			= 1 << 1,
//	FS_ITALIC				= 1 << 2,
//

#include "rf_local.h"

#define MAX_FONTS		64
#define MAX_FONT_SIZES	8

typedef struct fontSize_s {
	float		minScale;
	char		matName[MAX_QPATH];
	shader_t	*matPtr;
} fontSize_t;

typedef struct font_s {
	char			name[MAX_QPATH-11];	// fonts/[name].font
	uint32			touchFrame;

	byte			numSizes;
	fontSize_t		fontSizes[MAX_FONT_SIZES];

	float			charWidth;
	float			charHeight;
} font_t;

static font_t		r_curFont;

static font_t		r_fontList[MAX_FONTS];
static uint32		r_numFonts;

static vec4_t		r_fontShadowClr;	// Only the alpha value ever changes

/*
=============================================================================

	FONT REGISTRATION

=============================================================================
*/

/*
===============
R_FindFont
===============
*/
static inline font_t *R_FindFont (char *name)
{
	font_t	*font;
	uint32	i;

	assert (name && name[0]);
	ri.reg.fontsSeaked++;

	for (i=0, font=&r_fontList[0] ; i<r_numFonts ; font++, i++) {
		if (!font->touchFrame)
			continue;	// Free slot

		// Check name
		if (!strcmp (font->name, name))
			return font;
	}

	return NULL;
}


/*
===============
R_TouchFont
===============
*/
static inline void R_TouchFont (font_t *font)
{
	fontSize_t	*fs;
	byte		i;

	assert (font);

	// Register the material(s)
	for (i=0, fs=font->fontSizes ; i<font->numSizes ; fs++, i++) {
		fs->matPtr = R_RegisterPic (fs->matName);
		if (!fs->matPtr)
			Com_Printf (PRNT_ERROR, "R_TouchFont: could not locate material '%s'!\n", fs->matName);
	}

	// Used this sequence
	font->touchFrame = ri.reg.registerFrame;
	ri.reg.fontsTouched++;
}


/*
===============
R_FreeFont
===============
*/
static inline void R_FreeFont (font_t *font)
{
	assert (font);
	if (!font)
		return;

	// Free it
	memset (font, 0, sizeof (font_t));
	ri.reg.fontsReleased++;
}


/*
===============
R_RegisterFont
===============
*/
font_t *R_RegisterFont (char *name)
{
	char		fixedName[MAX_QPATH];
	char		*buf;
	int			fileLen;
	char		*token;
	float		floatTok;
	parse_t		*ps;
	font_t		*font;
	uint32		i;

	// Check the name
	if (!name || !name[0]) {
		Com_Printf (PRNT_ERROR, "R_RegisterFont: NULL name!\n");
		return NULL;
	}
	if (strchr (name, '/') || strchr (name, '\\') || strchr (name, '.')) {
		Com_Printf (PRNT_ERROR, "R_RegisterFont: name can not be a path ('%s')!\n", name);
		return NULL;
	}
	Q_strncpyz (fixedName, name, sizeof (fixedName));
	Q_strlwr (fixedName);

	// See if it's already loaded
	font = R_FindFont (name);
	if (font) {
		R_TouchFont (font);
		return font;
	}

	// Nope, use the scratch font spot for writing
	font = &r_curFont;
	memset (&r_curFont, 0, sizeof (font_t));
	Q_strncpyz (font->name, name, sizeof (font->name));
	Q_strlwr (font->name);
	font->charWidth = 8.0f;
	font->charHeight = 8.0f;

	// Load the file
	Q_snprintfz (fixedName, sizeof (fixedName), "fonts/%s.font", name);
	fileLen = FS_LoadFile (fixedName, (void **)&buf, "\n\0");
	if (!buf || fileLen <= 0) {
		Com_Printf (PRNT_ERROR, "ERROR: can't load '%s' -- %s\n", name, (fileLen == -1) ? "not found" : "empty file");
		return NULL;
	}

	// Parse the file
	ps = PS_StartSession (buf, PSP_COMMENT_BLOCK|PSP_COMMENT_LINE|PSP_COMMENT_POUND);
	for ( ; PS_ParseToken (ps, PSF_ALLOW_NEWLINES|PSF_TO_LOWER, &token) ; ) {
		// Parse the materials
		if (!strcmp (token, "charwidth")) {
			// Width
			if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &floatTok, 1)) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}
			if (floatTok < 0) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}

			font->charWidth = floatTok;
			continue;
		}
		else if (!strcmp (token, "charheight")) {
			// Height
			if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &floatTok, 1)) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}
			if (floatTok < 0) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}

			font->charHeight = floatTok;
			continue;
		}
		else if (!strcmp (token, "mat")) {
			// Check for too many
			if (font->numSizes+1 >= MAX_FONT_SIZES) {
				Com_Printf (PRNT_ERROR, "ERROR: too many size definitions!\n");
				break;
			}

			// Scale
			if (!PS_ParseDataType (ps, 0, PSDT_FLOAT, &floatTok, 1)) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}
			if (floatTok < 0) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", token);
				break;
			}

			// Name
			if (!PS_ParseToken (ps, PSF_TO_LOWER, &token)) {
				Com_Printf (PRNT_ERROR, "ERROR: invalid/missing parameters for '%s'\n", "mat");
				break;
			}

			Q_strncpyz (font->fontSizes[font->numSizes].matName, token, sizeof (font->fontSizes[font->numSizes].matName));
			font->fontSizes[font->numSizes].minScale = floatTok;
			font->numSizes++;
			continue;
		}

		Com_Printf (PRNT_ERROR, "ERROR: unknown key '%s'\n", token);
	}

	// Done parsing
	PS_EndSession (ps);
	FS_FreeFile (buf);

	// Make sure there's at least one sizedef
	if (font->numSizes == 0) {
		Com_Printf (PRNT_ERROR, "ERROR: no valid fonts for '%s'!\n", name);
		return NULL;
	}

	// Find a free slot
	for (i=0, font=&r_fontList[0] ; i<r_numFonts ; font++, i++) {
		if (font->touchFrame)
			continue;	// In use
		break;
	}

	// None found, make a new slot
	if (i == r_numFonts) {
		if (r_numFonts+1 >= MAX_FONTS)
			Com_Error (ERR_DROP, "R_RegisterFont: MAX_FONTS");

		font = &r_fontList[r_numFonts++];
	}

	// Copy from scratch space, touch, finish
	memcpy (font, &r_curFont, sizeof (font_t));
	R_TouchFont (font);
	return font;
}


/*
===============
R_EndFontRegistration
===============
*/
void R_EndFontRegistration (void)
{
	font_t		*font;
	uint32		i;

	// Free un-touched fonts
	for (i=0, font=&r_fontList[0] ; i<r_numFonts ; font++, i++) {
		if (!font->touchFrame)
			continue;	// Free spot
		if (font->touchFrame == ri.reg.registerFrame)
			continue;	// Used in this sequence

		R_FreeFont (font);
	}
}


/*
===============
R_CheckFont
===============
*/
void R_CheckFont (void)
{
	// Load console characters
	r_defaultFont->modified = qFalse;

	// Load the font
	ri.media.defaultFont = R_RegisterFont (r_defaultFont->string);
	if (!ri.media.defaultFont && strcmp (r_defaultFont->string, "default"))
		ri.media.defaultFont = R_RegisterFont ("default");

	if (!ri.media.defaultFont)
		Com_Error (ERR_FATAL, "R_CheckFont: unable to load default font!");
}

/*
=============================================================================

	FONT RENDERING

=============================================================================
*/

/*
================
R_ShaderForFont
================
*/
// FIXME: let format decide based on x/y scale
static shader_t *R_ShaderForFont (font_t *font, float xScale, float yScale)
{
	fontSize_t	*fs;
	byte		i;
	shader_t	*bestMat;
	float		bestScale;

	assert (font);

	// Find the most appropriate scale
	bestMat = NULL;
	bestScale = -1;
	for (i=0, fs=font->fontSizes ; i<font->numSizes ; fs++, i++) {
		if (xScale >= fs->minScale && xScale > bestScale) {
			bestScale = fs->minScale;
			bestMat = fs->matPtr;
		}
	}

	return bestMat;
}


/*
================
R_GetFontDimensions
================
*/
// FIXME: (font_t *font, vec2_t scale, uint32 flags, vec2_t target)
// FIXME: adjust for shadowing flag?
void R_GetFontDimensions (font_t *font, float xScale, float yScale, uint32 flags, vec2_t dest)
{
	if (!font)
		font = ri.media.defaultFont;
	if (!xScale && !yScale) {
		xScale = r_fontScale->floatVal;
		yScale = r_fontScale->floatVal;
	}

	if (dest) {
		if (flags & FS_SQUARE) {
			if (font->charWidth >= font->charHeight) {
				dest[0] = font->charWidth * xScale;
				dest[1] = font->charWidth * yScale;
			}
			else {
				dest[0] = font->charHeight * xScale;
				dest[1] = font->charHeight * yScale;
			}
		}
		else {
			dest[0] = font->charWidth * xScale;
			dest[1] = font->charHeight * yScale;
		}
	}
}


/*
================
R_DrawString
================
*/
size_t R_DrawString (font_t *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, vec4_t color)
{
	int			num;
	size_t		i;
	float		frow, fcol;
	float		startX;
	vec4_t		strColor;
	qBool		isShadowed;
	qBool		skipNext = qFalse;
	qBool		inColorCode = qFalse;
	shader_t	*shader;
	vec2_t		ftSize;

	if (!string)
		return 0;

	// Find the font
	if (!xScale && !yScale) {
		xScale = r_fontScale->floatVal;
		yScale = r_fontScale->floatVal;
	}
	if (!font)
		font = ri.media.defaultFont;
	if (!color)
		color = Q_colorWhite;
	shader = R_ShaderForFont (font, xScale, yScale);

	Vec4Copy (color, strColor);

	isShadowed = (flags & FS_SHADOW);
	Vec3Copy (Q_colorBlack, r_fontShadowClr);
	r_fontShadowClr[3] = strColor[3];

	R_GetFontDimensions (font, xScale, yScale, flags, ftSize);

	startX = x;
	for (i=0 ; *string ; ) {
		num = *string;
		if (flags & FS_SECONDARY && num < 128)
			num |= 128;

		if (skipNext) {
			skipNext = qFalse;
		}
		else if ((num & 127) == COLOR_ESCAPE && *(string+1)) {
			switch (string[1] & 127) {
			case COLOR_ESCAPE:
				string++;
				skipNext = qTrue;
				continue;

			case 'i':	case 'I':
				// FIXME: todo!
				string += 2;
				continue;

			case 'r':	case 'R':
				isShadowed = (flags & FS_SHADOW);
				inColorCode = qFalse;
				Vec3Copy (Q_colorWhite, strColor);
				string += 2;
				continue;

			case 's':	case 'S':
				isShadowed = !isShadowed || (flags & FS_SHADOW);
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
				Vec3Copy (Q_strColorTable[Q_StrColorIndex (string[1])], strColor);
				inColorCode = qTrue;
				string += 2;
				continue;
			}
		}
		else if ((num & 127) == '\n') {
			x = startX;
			y += ftSize[1];
			string++;
			continue;
		}

		if (inColorCode)
			num &= 127;
		else
			num &= 255;

		// Skip spaces
		if ((num&127) != 32) {
			frow = (num>>4) * (1.0f/16.0f);
			fcol = (num&15) * (1.0f/16.0f);

			if (isShadowed)
				R_DrawPic (shader, 0, x+2, y+2, ftSize[0], ftSize[1], fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), r_fontShadowClr);

			R_DrawPic (shader, 0, x, y, ftSize[0], ftSize[1], fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), strColor);
		}

		x += ftSize[0];
		string++;
		i++;
	}

	return i;
}


/*
================
R_DrawStringLen
================
*/
size_t R_DrawStringLen (font_t *font, float x, float y, float xScale, float yScale, uint32 flags, char *string, size_t len, vec4_t color)
{
	char	swap;
	size_t	length;

	if (len < 0)
		return R_DrawString (font, x, y, xScale, yScale, flags, string, color);

	swap = string[len];
	string[len] = 0;
	length = R_DrawString (font, x, y, xScale, yScale, flags, string, color);
	string[len] = swap;

	return length;
}


/*
================
R_DrawChar
================
*/
void R_DrawChar (font_t *font, float x, float y, float xScale, float yScale, uint32 flags, int num, vec4_t color)
{
	float		frow, fcol;
	shader_t	*shader;
	vec2_t		ftSize;

	if (!xScale && !yScale) {
		xScale = r_fontScale->floatVal;
		yScale = r_fontScale->floatVal;
	}
	if (!font)
		font = ri.media.defaultFont;
	if (!color)
		color = Q_colorWhite;
	shader = R_ShaderForFont (font, xScale, yScale);

	R_GetFontDimensions (font, xScale, yScale, flags, ftSize);

	if (flags & FS_SECONDARY && num < 128)
		num |= 128;
	num &= 255;

	if ((num&127) == 32)
		return;	// Space

	frow = (num>>4) * (1.0f/16.0f);
	fcol = (num&15) * (1.0f/16.0f);

	if (flags & FS_SHADOW) {
		r_fontShadowClr[3] = color[3];
		R_DrawPic (shader, 0, x+2, y+2, ftSize[0], ftSize[1], fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), r_fontShadowClr);
	}

	R_DrawPic (shader, 0, x, y, ftSize[0], ftSize[1], fcol, frow, fcol+(1.0f/16.0f), frow+(1.0f/16.0f), color);
}

/*
=============================================================================

	INITIALIZATION

=============================================================================
*/

/*
================
R_FontInit
================
*/
void R_FontInit (void)
{
	// Set values
	Vec3Copy (Q_colorBlack, r_fontShadowClr);

	// FIXME: load all font files into memory here, and only register the images when needed?

	Mem_CheckPoolIntegrity (ri.fontSysPool);
}


/*
================
R_FontShutdown
================
*/
void R_FontShutdown (void)
{
	font_t	*font;
	size_t	size, i;

	Com_Printf (0, "Font system shutdown:\n");
	// Release fonts
	for (i=0, font=&r_fontList[0] ; i<r_numFonts ; font++, i++)
		R_FreeFont (font);

	r_numFonts = 0;
	memset (&r_fontList[0], 0, sizeof (font_t) * MAX_FONTS);

	// Empty the memory pool
	size = Mem_FreePool (ri.fontSysPool);
	Com_Printf (0, "...releasing %u bytes...\n", size);
}
