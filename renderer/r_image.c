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

// r_image.c
// - uploads loaded images to the video card

#include "r_local.h"
#include "jpg/jpeglib.h"
#include "png/png.h"
#include <process.h>

image_t		img_Textures[MAX_GLTEXTURES];
int			img_NumTextures;

int			img_RegistrationFrame;

static byte		img_IntensityTable[256];
static byte		img_GammaTable[256];
unsigned int	img_PaletteTable[256];

byte	img_DefaultPal[] =
{
	#include "defpal.h"
};

const char		*sky_NameSuffix[6]		= { "rt", "bk", "lf", "ft", "up", "dn" };
const char		*cubeMap_NameSuffix[6]	= { "px", "nx", "py", "ny", "pz", "nz" };

image_t	*r_NoTexture;		// use for bad textures
image_t	*r_WhiteTexture;	// used in shaders/fallback
image_t	*r_BlackTexture;	// used in shaders/fallback
image_t	*r_CinTexture;		// allocates memory on load as to not every cin frame
image_t	*r_DSTTexture;		// for turb surfaces (nv texture shader)

cvar_t	*intensity;

cvar_t	*gl_jpgquality;
cvar_t	*gl_nobind;
cvar_t	*gl_picmip;
cvar_t	*gl_screenshot;
cvar_t	*gl_texturebits;
cvar_t	*gl_texturemode;

cvar_t	*vid_gamma;
cvar_t	*vid_gammapics;

/*
==============================================================================

	TEXTURE

==============================================================================
*/

/*
===============
GL_SelectTexture
===============
*/
void GL_SelectTexture (int target) {
	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture)
		return;

	if (target > glConfig.maxTexUnits) {
		Com_Error (ERR_DROP, S_COLOR_RED "Attempted selection of an out of bounds (%d) texture unit!", target);
		return;
	}

	if (target == glState.texUnit)
		return;

	glState.texUnit = target;
	if (qglSelectTextureSGIS) {
		qglSelectTextureSGIS (target + GL_TEXTURE0_SGIS);
	} else if (qglActiveTextureARB) {
		qglActiveTextureARB (target + GL_TEXTURE0_ARB);
		qglClientActiveTextureARB (target + GL_TEXTURE0_ARB);
	}
}


/*
===============
GL_Bind
===============
*/
void GL_Bind (GLenum target, image_t *image) {
	int		texNum;
	GL_SelectTexture ((int)target);

	if (gl_nobind->integer)		// performance evaluation option
		texNum = r_NoTexture->texNum;
	else
		texNum = image->texNum;
	if (glState.texNums[glState.texUnit] == texNum)
		return;

	glState.texNums[glState.texUnit] = texNum;
	if (image->upFlags & IT_CUBEMAP)
		qglBindTexture (GL_TEXTURE_CUBE_MAP_ARB, texNum);
	else
		qglBindTexture (GL_TEXTURE_2D, texNum);
}


/*
===============
GL_MTexCoord2f
===============
*/
void GL_MTexCoord2f (int target, GLfloat s, GLfloat t) {
	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture)
		return;

	if (qglSelectTextureSGIS)
		qglMTexCoord2f (target + GL_TEXTURE0_SGIS, s, t);
	else if (qglActiveTextureARB)
		qglMTexCoord2f (target + GL_TEXTURE0_ARB, s, t);
}


/*
===============
GL_ToggleMultitexture
===============
*/
void GL_ToggleMultitexture (qBool enable) {
	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture)
		return;

	if (enable) {
		GL_SelectTexture (TU1);
		qglEnable (GL_TEXTURE_2D);
		GL_TexEnv (GL_MODULATE);
	} else {
		GL_SelectTexture (TU1);
		qglDisable (GL_TEXTURE_2D);
		GL_TexEnv (GL_MODULATE);
	}

	GL_SelectTexture (TU0);
	GL_TexEnv (GL_MODULATE);
}


/*
===============
GL_TexEnv
===============
*/
void GL_TexEnv (GLfloat mode) {
	if (!glConfig.extTexEnvAdd && (mode == GL_ADD)) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glState.texEnvModes[glState.texUnit] = GL_MODULATE;
		return;
	}

	if (mode != glState.texEnvModes[glState.texUnit]) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		glState.texEnvModes[glState.texUnit] = mode;
	}
}


/*
=================
GL_ToggleOverbrights
=================
*/
void GL_ToggleOverbrights (qBool enable) {
	if (glState.overBrightsOn == enable)
		return;

	glState.overBrightsOn = enable;
	if (enable && r_overbrightbits->integer && glConfig.extMTexCombine) {
		qglTexEnvi (GL_TEXTURE_ENV,	GL_TEXTURE_ENV_MODE,	GL_COMBINE_ARB);
		qglTexEnvi (GL_TEXTURE_ENV,	GL_COMBINE_RGB_ARB,		GL_MODULATE);
		qglTexEnvi (GL_TEXTURE_ENV,	GL_COMBINE_ALPHA_ARB,	GL_MODULATE);
		qglTexEnvi (GL_TEXTURE_ENV,	GL_RGB_SCALE_ARB,		r_overbrightbits->integer);

		GL_TexEnv (GL_COMBINE_ARB);
	} else {
		GL_TexEnv (GL_MODULATE);

		if (glConfig.extMTexCombine)
			qglTexEnvi (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	}
}


/*
==================
GL_TextureBits
==================
*/
void GL_TextureBits (void) {
	switch (gl_texturebits->integer) {
	case 32:
		glState.texRGBFormat = GL_RGB8;
		glState.texRGBAFormat = GL_RGBA8;
		break;
	case 16:
		glState.texRGBFormat = GL_RGB5;
		glState.texRGBAFormat = GL_RGBA4;
		break;
	default:
		glState.texRGBFormat = GL_RGB;
		glState.texRGBAFormat = GL_RGBA;
		break;
	}

	Com_Printf (PRINT_ALL, "Texture bits: %s\n", (gl_texturebits->integer == 32) ? "32" : (gl_texturebits->integer == 16) ? "16" : "Default");
}


/*
===============
GL_TextureMode
===============
*/
typedef struct {
	char	*name;

	GLint	minimize;
	GLint	maximize;
} glTexMode_t;

glTexMode_t modes[] = {
	{"GL_NEAREST",					GL_NEAREST,					GL_NEAREST},
	{"GL_LINEAR",					GL_LINEAR,					GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST",	GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST",	GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR",	GL_NEAREST_MIPMAP_LINEAR,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR",		GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR}
};

#define NUM_GL_MODES (sizeof (modes) / sizeof (glTexMode_t))

void GL_TextureMode (void) {
	int		i;
	image_t	*glt;
	char	*string = gl_texturemode->string;

	gl_texturemode->modified = qFalse;

	for (i=0 ; i<NUM_GL_MODES ; i++) {
		if (!Q_stricmp (modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "bad filter name -- falling back to GL_LINEAR_MIPMAP_NEAREST\n");
		
		glState.texMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		glState.texMagFilter = GL_LINEAR;

		Cvar_ForceSet ("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST");
		gl_texturemode->modified = qFalse;
	} else {
		glState.texMinFilter = modes[i].minimize;
		glState.texMagFilter = modes[i].maximize;
	}

	Com_Printf (PRINT_ALL, "Texture mode: %s\n", modes[i].name);

	// change all the existing mipmap texture objects
	for (i=0, glt=img_Textures ; i<img_NumTextures ; i++, glt++) {
		if (!glt->iRegistrationFrame)
			continue;		// free img_Textures slot

		GL_Bind (0, glt);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (glt->upFlags & IF_NOMIPMAP) ? glState.texMagFilter : glState.texMinFilter);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
}


/*
===============
GL_ResetAnisotropy
===============
*/
void GL_ResetAnisotropy (void)
{
	int		i;
	image_t	*glt;

	gl_ext_max_anisotropy->modified = qFalse;

	if (!glConfig.extTexFilterAniso)
		return;

	// change all the existing mipmap texture objects
	for (i=0, glt=img_Textures ; i<img_NumTextures ; i++, glt++) {
		if (!glt->iRegistrationFrame)
			continue;		// free img_Textures slot
		if (glt->upFlags & IF_NOMIPMAP)
			continue;		// skip non-mipmapped imagery

		GL_Bind (0, glt);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (gl_ext_max_anisotropy->integer, 1, glConfig.maxAniso));
	}
}

/*
==============================================================================
 
	JPG
 
==============================================================================
*/

static void jpg_noop(j_decompress_ptr cinfo) {
}

static void jpeg_d_error_exit (j_common_ptr cinfo) {
	char msg[1024];

	(cinfo->err->format_message)(cinfo, msg);
	Com_Error (ERR_FATAL, "Img_LoadJPG: JPEG Lib Error: '%s'", msg);
}

static boolean jpg_fill_input_buffer (j_decompress_ptr cinfo) {
    Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Premeture end of jpeg file\n");

    return 1;
}

static void jpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}

static void jpeg_mem_src (j_decompress_ptr cinfo, byte *mem, int len) {
    cinfo->src = (struct jpeg_source_mgr *)
	(*cinfo->mem->alloc_small)((j_common_ptr) cinfo,
				   JPOOL_PERMANENT,
				   sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = jpg_noop;
    cinfo->src->fill_input_buffer = jpg_fill_input_buffer;
    cinfo->src->skip_input_data = jpg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = jpg_noop;
    cinfo->src->bytes_in_buffer = len;
    cinfo->src->next_input_byte = mem;
}

/*
=============
Img_LoadJPG

ala Vic
=============
*/
int Img_LoadJPG (char *name, byte **pic, int *width, int *height) {
    int		i, length, samples;
    byte	*img, *scan, *buffer, *dummy;
    struct	jpeg_error_mgr			jerr;
    struct	jpeg_decompress_struct	cinfo;

	if (pic)
		*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return 0;

	//
	// parse the file
	//
	cinfo.err = jpeg_std_error (&jerr);
	jerr.error_exit = jpeg_d_error_exit;

	jpeg_create_decompress (&cinfo);

	jpeg_mem_src (&cinfo, buffer, length);
	jpeg_read_header (&cinfo, TRUE);

	jpeg_start_decompress (&cinfo);

	samples = cinfo.output_components;
    if ((samples != 3) && (samples != 1)) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg samples '%s' (%d)\n", name, samples);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if ((cinfo.output_width <= 0) || (cinfo.output_height <= 0)) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg dimensions on '%s' ( %d x %d )\n", name, cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (width)	*width = cinfo.output_width;
	if (height)	*height = cinfo.output_height;

	img = Z_Malloc (cinfo.output_width * cinfo.output_height * 4);
	dummy = Z_Malloc (cinfo.output_width * samples);

	if (pic)
		*pic = img;

	if (samples == 1) {
		while (cinfo.output_scanline < cinfo.output_height) {
			scan = dummy;
			if (!jpeg_read_scanlines (&cinfo, &scan, 1)) {
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg file %s\n", name);
				jpeg_destroy_decompress (&cinfo);
			    Z_Free (dummy);
				FS_FreeFile (buffer);
				return 0;
			}

			for (i=0 ; i<cinfo.output_width ; i++, img += 4) {
				img[0] = img[1] = img[2] = *scan++;
				img[3] = 255;
			}
		}
	} else {
		while (cinfo.output_scanline < cinfo.output_height) {
			scan = dummy;
			if (!jpeg_read_scanlines (&cinfo, &scan, 1)) {
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg file %s\n", name);
				jpeg_destroy_decompress (&cinfo);
			    Z_Free (dummy);
				FS_FreeFile (buffer);
				return 0;
			}

			for (i=0 ; i<cinfo.output_width ; i++, img += 4, scan += 3) {
				img[0] = scan[0];
				img[1] = scan[1];
				img[2] = scan[2];
				img[3] = 255;
			}
		}
	}

    Z_Free (dummy);

    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);

	FS_FreeFile (buffer);

	return 3;
}


/*
================== 
Img_JPGScreenShot
by Robert 'Heffo' Heffernan
================== 
*/
void Img_JPGScreenShot (void) {
	int			i, offset, w3;
	char		checkname[MAX_OSPATH], picname[MAX_OSPATH];
	struct		jpeg_compress_struct	cinfo;
	struct		jpeg_error_mgr			jerr;
	byte		*buffer, *s;
	FILE		*f;

	Com_Printf (PRINT_ALL, "Taking JPG screenshot...\n");

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof (checkname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (checkname);

	// find a file name to save it to
	for (i = 0 ; i<999 ; i++) {
		sprintf (picname, "%s/scrnshot/egl%.3d.jpg", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)	break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_JPGScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	// allocate room for a copy of the framebuffer
	buffer = Z_TagMalloc (vidDef.width * vidDef.height * 3, ZTAG_IMAGESYS);

	// read the framebuffer into our storage
	qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	// initialise the jpeg compression object
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, f);

	// setup jpeg parameters
	cinfo.image_width = vidDef.width;
	cinfo.image_height = vidDef.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;

	jpeg_set_defaults (&cinfo);

	if ((gl_jpgquality->value > 100) || (gl_jpgquality->value <= 0))
		Cvar_Set ("gl_jpgquality", "85");

	jpeg_set_quality (&cinfo, gl_jpgquality->value, TRUE);
	jpeg_start_compress (&cinfo, qTrue);	// start compression

	// feed scanline data
	w3 = cinfo.image_width * 3;
	offset = w3 * cinfo.image_height - w3;
	while (cinfo.next_scanline < cinfo.image_height) {
		s = &buffer[offset - (cinfo.next_scanline * w3)];
		jpeg_write_scanlines (&cinfo, &s, 1);
	}

	// finish compression
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

	fclose (f);
	Z_Free (buffer);

	Com_Printf (PRINT_ALL, "Wrote egl%.3d.jpg\n", i);
}

/*
==============================================================================

	PCX

==============================================================================
*/

typedef struct pcxHeader_s {
	char			manufacturer;
	char			version;
	char			encoding;
	char			bitsPerPixel;

	unsigned short	xMin, yMin, xMax, yMax;
	unsigned short	hRes, vRes;
	unsigned char	palette[48];

	char			reserved;
	char			colorPlanes;

	unsigned short	bytesPerLine;
	unsigned short	paletteType;

	char			filler[58];

	unsigned char	data;			// unbounded
} pcxHeader_t;

/*
=============
Img_LoadPCX
=============
*/
int Img_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height) {
	byte		*buffer;
	pcxHeader_t	*pcx;
	int			x, y, samples;
	int			len, columns, rows;
	int			dataByte;
	int			runLength;
	byte		pal[768];
	byte		*img;

	if (pic)
		*pic = NULL;
	if (palette)
		*palette = NULL;

	//
	// load the file
	//
	len = FS_LoadFile (filename, (void **)&buffer);
	if (!buffer)
		return 0;

	//
	// parse the PCX file
	//
	pcx = (pcxHeader_t *)buffer;

	pcx->xMin = LittleShort (pcx->xMin);
	pcx->yMin = LittleShort (pcx->yMin);
	pcx->xMax = LittleShort (pcx->xMax);
	pcx->yMax = LittleShort (pcx->yMax);
	pcx->hRes = LittleShort (pcx->hRes);
	pcx->vRes = LittleShort (pcx->vRes);
	pcx->bytesPerLine = LittleShort (pcx->bytesPerLine);
	pcx->paletteType = LittleShort (pcx->paletteType);

	buffer = &pcx->data;

	samples = 0;
	if ((pcx->manufacturer != 0x0a) || (pcx->version != 5) || (pcx->encoding != 1)) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Invalid PCX header: %s\n", filename);
		goto freePCX;
	}

	if ((pcx->bitsPerPixel != 8) || (pcx->colorPlanes != 1)) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Only 8-bit PCX images are supported: %s\n", filename);
		goto freePCX;
	}

	if ((pcx->xMax >= 640) || (pcx->xMax <= 0) || (pcx->yMax >= 480) || (pcx->yMax <= 0)) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "Bad PCX file dimensions: %s: %i x %i\n", filename, pcx->xMax, pcx->yMax);
		goto freePCX;
	}

	columns = pcx->xMax + 1;
	rows = pcx->yMax + 1;
	img = Z_TagMalloc (columns * rows * 4, ZTAG_IMAGESYS);
	memcpy (pal, (byte *)pcx + len - 768, 768);

	if (pic)
		*pic = img;

	if (palette) {
		*palette = Z_TagMalloc (768, ZTAG_IMAGESYS);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	//
	// scan the image and factor in the palette
	//
	samples = 3;
	for (y=0 ; y<rows ; y++) {
		for (x=0 ; x<columns ; ) {
			dataByte = *buffer++;

			if ((dataByte & 0xC0) == 0xC0) {
				runLength = dataByte & 0x3F;
				dataByte = *buffer++;
			} else
				runLength = 1;

			while (runLength-- > 0) {
				if (dataByte == 255) {
					// hack Quake2 palette
					img[0] = 0;
					img[1] = 0;
					img[2] = 0;
					img[3] = 0;
					samples = 4;
				} else {
					img[0] = pal[dataByte*3+0];
					img[1] = pal[dataByte*3+1];
					img[2] = pal[dataByte*3+2];
					img[3] = 255;
				}
				x++;
				img += 4;
			}
		}
	}

	if (buffer - (byte *)pcx > len) {
		Com_Printf (PRINT_DEV, S_COLOR_YELLOW "PCX file %s was malformed", filename);
		if (pic) {
			Z_Free (*pic);
			*pic = NULL;
		}
		samples = 0;
	}

freePCX:
	FS_FreeFile (pcx);
	return samples;
}

/*
==============================================================================

	PNG

==============================================================================
*/

typedef struct pngbuf_s {
	byte	*buffer;
	int		pos;
} pngBuf_t;

void __cdecl PngReadFunc (png_struct *Png, png_bytep buf, png_size_t size) {
	pngBuf_t *PngFileBuffer = (pngBuf_t*)png_get_io_ptr(Png);
	memcpy (buf,PngFileBuffer->buffer + PngFileBuffer->pos, size);
	PngFileBuffer->pos += size;
}

/*
=============
Img_LoadPNG
=============
*/
int Img_LoadPNG (char *name, byte **pic, int *width, int *height) {
	int			i, rowptr;
	int			samples;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_infop	end_info;
	byte		**row_pointers;
	byte		*img;

	pngBuf_t	PngFileBuffer = {NULL,0};

	if (pic)
		*pic = NULL;

	//
	// load the file
	//
	FS_LoadFile (name, &PngFileBuffer.buffer);
	if (!PngFileBuffer.buffer)
		return 0;

	//
	// parse the PNG file
	//
	if ((png_check_sig (PngFileBuffer.buffer, 8)) == 0) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadPNG: Not a PNG file: %s\n", name);
		goto freeFile;
	}

	PngFileBuffer.pos = 0;

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,  NULL, NULL);

	if (!png_ptr) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
	}
	
	end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
	}

	png_set_read_fn (png_ptr, (png_voidp)&PngFileBuffer, (png_rw_ptr)PngReadFunc);

	png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	row_pointers = png_get_rows (png_ptr, info_ptr);

	rowptr = 0;

	img = Z_TagMalloc (info_ptr->width * info_ptr->height * 4, ZTAG_IMAGESYS);
	if (pic)
		*pic = img;

	if (info_ptr->channels == 4) {
		for (i=0 ; i<info_ptr->height ; i++) {
			memcpy (img + rowptr, row_pointers[i], info_ptr->rowbytes);
			rowptr += info_ptr->rowbytes;
		}
	} else {
		int		j, x;

		memset (img, 255, info_ptr->width * info_ptr->height * 4);
		for (x=0, i=0 ; i<info_ptr->height ; i++) {
			for (j=0 ; j<info_ptr->rowbytes ; j+=info_ptr->channels) {
				memcpy (img + x, row_pointers[i] + j, info_ptr->channels);
				x+= 4;
			}
			rowptr += info_ptr->rowbytes;
		}
	}

	if (width)
		*width = info_ptr->width;
	if (height)
		*height = info_ptr->height;

	// since I am unsure how to interpret above
	{
		byte	*scan = ((byte *)img) + 3;
		samples = 3;

		for (i=0 ; i<info_ptr->width*info_ptr->height ; i++, scan+=4) {
			if (*scan != 255) {
				samples = 4;
				break;
			}
		}
	}

	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);

	FS_FreeFile (PngFileBuffer.buffer);
	return samples;

freeFile:
	FS_FreeFile (PngFileBuffer.buffer);
	return 0;
}


/*
================== 
Img_PNGScreenShot
================== 
*/
unsigned int __stdcall png_write_thread (byte *buffer)
{
	char		picname[MAX_OSPATH];
	char		checkname[MAX_OSPATH];
	int			i, k;
	FILE		*f;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep	*row_pointers;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof (checkname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (checkname);

	for (i = 0 ; i<999 ; i++) {
		sprintf (picname, "%s/scrnshot/egl%.3d.png", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)	break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_PNGScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return 1;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "LibPNG Error!\n", picname);
		return 1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "LibPNG Error!\n", picname);
		return 1;
	}

	png_init_io (png_ptr, f);

	png_set_IHDR (png_ptr, info_ptr, vidDef.width, vidDef.height, 8, PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_set_compression_level (png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_compression_mem_level (png_ptr, 9);

	png_write_info (png_ptr, info_ptr);

	row_pointers = Z_TagMalloc (vidDef.height * sizeof (png_bytep), ZTAG_IMAGESYS);
	for (k=0 ; k<vidDef.height ; k++)
		row_pointers[k] = buffer + (vidDef.height - 1 - k) * 3 * vidDef.width;

	png_write_image (png_ptr, row_pointers);
	png_write_end (png_ptr, info_ptr);

	png_destroy_write_struct (&png_ptr, &info_ptr);

	fclose (f);

	Z_Free (buffer);
	Z_Free (row_pointers);

	Com_Printf (PRINT_ALL, "Wrote egl%.3d.png\n", i);
	_endthread ();

	return 0;
}

void Img_PNGScreenShot (void)
{
	byte		*buffer;
	DWORD		tID;

	Com_Printf (PRINT_ALL, "Taking PNG screenshot...\n");

	buffer = Z_TagMalloc (vidDef.width * vidDef.height * 3, ZTAG_IMAGESYS);
	qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	_beginthreadex (NULL, 0, (unsigned int (__stdcall *)(void *))png_write_thread, (void *)buffer, 0, &tID);
}

/*
==============================================================================

	TGA

==============================================================================
*/

typedef struct targaHeader_s {
	unsigned char	idLength;
	unsigned char	colorMapType;
	unsigned char	imageType;

	unsigned short	colorMapIndex;
	unsigned short	colorMapLength;
	unsigned char	colorMapSize;

	unsigned short	xOrigin;
	unsigned short	yOrigin;
	unsigned short	width;
	unsigned short	height;

	unsigned char	pixelSize;

	unsigned char	attributes;
} targaHeader_t;

/*
=============
Img_LoadTGA
=============
*/
int Img_LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int				i, j, columns, rows, row_inc;
	byte			*pixbuf;
	int				row, column;
	byte			*buf_p;
	byte			*buffer;
	int				length, samples = 3;
	targaHeader_t	targa_header;
	byte			*targa_rgba;
	byte			palette[256][4];
	byte			tmp[2];
	qBool			compressed;

	if (pic)
		*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return 0;

	//
	// parse the TGA file
	//
	buf_p = buffer;

	targa_header.idLength = *buf_p++;
	targa_header.colorMapType = *buf_p++;
	targa_header.imageType = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colorMapIndex = LittleShort (*((short *)tmp));
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colorMapLength = LittleShort (*((short *)tmp));
	buf_p+=2;
	targa_header.colorMapSize = *buf_p++;
	targa_header.xOrigin = LittleShort (*((short *)buf_p));
	buf_p+=2;
	targa_header.yOrigin = LittleShort (*((short *)buf_p));
	buf_p+=2;
	targa_header.width = LittleShort (*((short *)buf_p));
	buf_p+=2;
	targa_header.height = LittleShort (*((short *)buf_p));
	buf_p+=2;
	targa_header.pixelSize = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.idLength != 0)
		buf_p += targa_header.idLength;  // skip TARGA image comment

	if ((targa_header.imageType == 1) || (targa_header.imageType == 9)) {
		// uncompressed colormapped image
		if (targa_header.pixelSize != 8) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: Only 8 bit images supported for type 1 and 9");
			goto freeFile;
		}

		if (targa_header.colorMapLength != 256) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: Only 8 bit colormaps are supported for type 1 and 9");
			goto freeFile;
		}

		if (targa_header.colorMapIndex) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: colorMapIndex is not supported for type 1 and 9");
			goto freeFile;
		}

		if (targa_header.colorMapSize == 24) {
			for (i=0 ; i<targa_header.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = 255;
			}
		} else if (targa_header.colorMapSize == 32) {
			for (i=0 ; i<targa_header.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = *buf_p++;
			}
		} else {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: only 24 and 32 bit colormaps are supported for type 1 and 9");
			goto freeFile;
		}
	} else if ((targa_header.imageType == 2) || (targa_header.imageType == 10)) {
		// uncompressed or RLE compressed RGB
		if ((targa_header.pixelSize != 32) && (targa_header.pixelSize != 24)) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: Only 32 or 24 bit images supported for type 2 and 10");
			goto freeFile;
		}
	} else if ((targa_header.imageType == 3) || (targa_header.imageType == 11)) {
		// uncompressed greyscale
		if (targa_header.pixelSize != 8) {
			Com_Printf (PRINT_DEV, S_COLOR_YELLOW "LoadTGA: Only 8 bit images supported for type 3 and 11");
			goto freeFile;
		}
	}

	columns = targa_header.width;
	rows = targa_header.height;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = Z_TagMalloc (columns * rows * 4, ZTAG_IMAGESYS);
	if (pic)
		*pic = targa_rgba;

    // If bit 5 of attributes isn't set, the image has been stored from bottom to top
    if (targa_header.attributes & 0x20) {
        pixbuf = targa_rgba;
        row_inc = 0;
	} else {
        pixbuf = targa_rgba + (rows - 1)*columns*4;
        row_inc = -columns*4*2;
    }

	compressed = ((targa_header.imageType == 9) || (targa_header.imageType == 10) || (targa_header.imageType == 11));

	if (!compressed) {
		unsigned char red, green, blue, alpha;

		for (row=0 ; row<rows ; row++) {
			for (column=0 ; column<columns ; column++) {
				switch (targa_header.imageType) {
				case 1:
					// colormapped image
					blue = *buf_p++;
					red = palette[blue][0];
					green = palette[blue][1];
					alpha = palette[blue][3];
					blue = palette[blue][2];
					break;
				case 2:
					// 24 or 32 bit image
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					if (targa_header.pixelSize == 32)
						alpha = *buf_p++;
					else
						alpha = 255;
					break;
				case 3:
					// greyscale image
					blue = green = red = *buf_p++;
					alpha = 255;
					break;
				}
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				if (alpha != 255)
					samples = 4;
			}

			pixbuf += row_inc;
		}
	} else {
		unsigned char red, green, blue, alpha, packetHeader, packetSize;

		for (row=0 ; row<rows ; row++) {
			for (column=0 ; column<columns ; ) {
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);

				if (packetHeader & 0x80) {
					// run-length packet
					switch (targa_header.imageType) {
					case 9:
						// colormapped image
						blue = *buf_p++;
						red = palette[blue][0];
						green = palette[blue][1];
						alpha = palette[blue][3];
						blue = palette[blue][2];
						break;
					case 10:
						// 24 or 32 bit image
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						if( targa_header.pixelSize == 32 )
							alpha = *buf_p++;
						else
							alpha = 255;
						break;
					case 11:
						// greyscale image
						blue = green = red = *buf_p++;
						alpha = 255;
						break;
					}

					if (packetSize && (alpha != 255))
						samples = 4;

					for (j=0 ; j<packetSize ; j++) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alpha;
						column++;
						if (column == columns) {
							// run spans across rows
							column = 0;
							if (row < rows-1)
								row++;
							else
								goto breakOut;
							pixbuf += row_inc;
						}
					}
				} else {
					for (j=0 ; j<packetSize ; j++) {
						switch (targa_header.imageType) {
						case 9:
							// colormapped image
							blue = *buf_p++;
							red = palette[blue][0];
							green = palette[blue][1];
							alpha = palette[blue][3];
							blue = palette[blue][2];
							break;
						case 10:
							// 24 or 32 bit image
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							if (targa_header.pixelSize == 32)
								alpha = *buf_p++;
							else
								alpha = 255;
							break;
						case 11:
							// greyscale image
							blue = green = red = *buf_p++;
							alpha = 255;
							break;
						}
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alpha;

						if (alpha != 255)
							samples = 4;

						column++;
						if (column == columns) {
							// pixel packet run spans across rows
							column = 0;
							if (row < rows-1)
								row++;
							else
								goto breakOut;
							pixbuf += row_inc;
						}	
					}
				}
			}

			pixbuf += row_inc;
			breakOut:;
		}
	}

	FS_FreeFile (buffer);
	return samples;

freeFile:
	FS_FreeFile (buffer);
	return 0;
}


/*
================== 
Img_TGAScreenShot
================== 
*/
void Img_TGAScreenShot (void) {
	byte		*buffer;
	char		picname[MAX_OSPATH];
	int			i, j, c, temp;
	FILE		*f;

	Com_Printf (PRINT_ALL, "Taking TGA screenshot...\n");

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (picname, sizeof (picname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (picname);

	// find a file name to save it to
	for (i=0 ; i<=999 ; i++) {
		Com_sprintf (picname, sizeof (picname), "%s/scrnshot/egl%.3d.tga", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)
			break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_TGAScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	c = (vidDef.width*vidDef.height*3) + 18;

	buffer = Z_TagMalloc (c, ZTAG_IMAGESYS);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vidDef.width & 255;
	buffer[13] = vidDef.width >> 8;
	buffer[14] = vidDef.height & 255;
	buffer[15] = vidDef.height >> 8;
	buffer[16] = 24;	// pixel size

	if (glConfig.extBGRA && gl_ext_bgra->integer) {
		qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer + 18);
	} else {
		qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);
	
		// swap rgb to bgr
		for (j=18 ; j<c ; j+=3) {
			temp = buffer[j];
			buffer[j] = buffer[j+2];
			buffer[j+2] = temp;
		}	
	}

	fwrite (buffer, 1, c, f);
	fclose (f);

	Z_Free (buffer);
	Com_Printf (PRINT_ALL, "Wrote egl%.3d.tga\n", i);
}

/*
==============================================================================

	WAL

==============================================================================
*/

#define	MIPLEVELS	4
typedef struct {
	char			name[32];
	unsigned int	width;
	unsigned int	height;
	unsigned int	offsets[MIPLEVELS];		// four mip maps stored
	char			animName[32];			// next frame in animation chain
	int				flags;
	int				contents;
	int				value;
} walTex_t;

/*
================
Img_LoadWal
================
*/
int Img_LoadWal (char *name, byte **pic, int *width, int *height, int flags) {
	byte		*buffer;
	byte		*in, *out;
	walTex_t	*mt;
	int			i, samples;

	//
	// load the file
	//
	FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return 0;

	//
	// parse the WAL file
	//
	mt = (walTex_t *)buffer;

	mt->width = LittleLong (mt->width);
	mt->height = LittleLong (mt->height);
	mt->offsets[0] = LittleLong (mt->offsets[0]);

	if ((mt->width <= 0) || (mt->height <= 0)) {
		Com_Printf (PRINT_DEV, "Img_LoadWal: bad WAL file 's' (%i x %i)\n", name, mt->width, mt->height);
		goto freeFile;
	}

	if (width)
		*width = mt->width;
	if (height)
		*height = mt->height;

	out = Z_TagMalloc (mt->width * mt->height * 4, ZTAG_IMAGESYS);

	if (pic)
		*pic = out;

	//
	// scan the image and factor in the palette
	//
	in = buffer + mt->offsets[0];

	samples = 3;
	for (i=0 ; i<mt->width*mt->height ; i++, in++, out+=4) {
		*(unsigned *)out = img_PaletteTable[*in];

		if (*in == 255)
			samples = 4;
	}

	FS_FreeFile (buffer);
	return samples;

freeFile:
	FS_FreeFile (buffer);
	return 0;
}

/*
==============================================================================

	PRE-UPLOAD HANDLING

==============================================================================
*/

/*
================
Img_ResampleTexture
================
*/
void Img_ResampleTexture (unsigned *in, int inWidth, int inHeight, unsigned *out, int outWidth, int outHeight) {
	int			i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	static unsigned	*p1 = NULL, *p2 = NULL;
	byte		*pix1, *pix2, *pix3, *pix4;

	if (!p1) {
		p1 = malloc (glConfig.maxTexSize * sizeof (int));
		p2 = malloc (glConfig.maxTexSize * sizeof (int));
	}

	fracstep = inWidth * 0x10000 / outWidth;

	frac = fracstep >> 2;
	for (i=0 ; i<outWidth ; i++) {
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	frac = 3 * (fracstep >> 2);
	for (i=0 ; i<outWidth ; i++) {
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i=0 ; i<outHeight ; i++, out += outWidth) {
		inrow = in + inWidth * (int)((i + 0.25) * inHeight / outHeight);
		inrow2 = in + inWidth * (int)((i + 0.75) * inHeight / outHeight);
		frac = fracstep >> 1;

		for (j=0 ; j<outWidth ; j++) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];

			((byte *)(out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *)(out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *)(out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *)(out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}


/*
================
Img_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void Img_LightScaleTexture (byte *in, int inWidth, int inHeight, int samples, qBool useIntensity) {
	int		i, c;
	byte	*out = in;

	if (intensity->value == 1.0)
		return;

	c = inWidth * inHeight;

	if (useIntensity) {
		for (i=0 ; i<c ; i++, out+=4) {
			out[0] = img_GammaTable[img_IntensityTable[out[0]]];
			out[1] = img_GammaTable[img_IntensityTable[out[1]]];
			out[2] = img_GammaTable[img_IntensityTable[out[2]]];
		}
	} else {
		for (i=0 ; i<c ; i++, out+=4) {
			out[0] = img_GammaTable[out[0]];
			out[1] = img_GammaTable[out[1]];
			out[2] = img_GammaTable[out[2]];
		}
	}
}


/*
================
Img_MipMap

Operates in place, quartering the size of the texture
================
*/
void Img_MipMap (byte *in, int width, int height) {
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width) {
		for (j=0 ; j<width ; j+=8, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
==============================================================================

	IMAGE UPLOADING

==============================================================================
*/

/*
===============
Img_Upload
===============
*/
void Img_Upload (char *name, byte **data, int width, int height, int samples, image_t *image) {
	unsigned	*scaled;
	GLsizei		scaledWidth;
	GLsizei		scaledHeight;
	GLsizei		upWidth;
	GLsizei		upHeight;
	GLint		format;
	int			upFlags;
	GLenum		tTarget;
	GLenum		pTarget;
	int			i, times;
	qBool		isTrans;
	qBool		mipmap;

	if (image && (image->upFlags & IF_NOMIPMAP))
		mipmap = qFalse;
	else
		mipmap = qTrue;

	if (samples == 4)
		isTrans = qTrue;
	else if (samples == 3)
		isTrans = qFalse;
	else
		Com_Printf (PRINT_ALL, S_COLOR_RED "Img_Upload: Invalid image sample count on %s\n", name);

	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;

	// let people sample down the world textures for speed
	if ((image && !(image->flags & IF_NOPICMIP)) || !image) {
		if (gl_picmip->integer > 0) {
			scaledWidth >>= gl_picmip->integer;
			scaledHeight >>= gl_picmip->integer;
		}
	}

	// clamp dimensions and set targets
	if (image && (image->flags & IT_CUBEMAP)) {
		times = 6;
		tTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		pTarget = GL_TEXTURE_CUBE_MAP_ARB;

		// check size
		scaledWidth = clamp (scaledWidth, 1, glConfig.maxCMTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxCMTexSize);
	} else {
		times = 1;
		tTarget = GL_TEXTURE_2D;
		pTarget = GL_TEXTURE_2D;

		// check size
		scaledWidth = clamp (scaledWidth, 1, glConfig.maxTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxTexSize);
	}

	// texture compression
	if (glConfig.extArbTexCompression) {
		if (isTrans)	format = GL_COMPRESSED_RGBA_ARB;
		else			format = GL_COMPRESSED_RGB_ARB;
	} else {
		if (isTrans)	format = glState.texRGBAFormat;
		else			format = glState.texRGBFormat;
	}

	// set upload values
	upFlags = image ? image->flags : 0;
	upWidth = scaledWidth;
	upHeight = scaledHeight;

	// alter uploaded flags
	if (!mipmap) {
		if (!(upFlags & IF_NOGAMMA) && !vid_gammapics->integer)
			upFlags |= IF_NOGAMMA;

		if (!(upFlags & IF_NOMIPMAP))
			upFlags |= IF_NOMIPMAP;
	}

	if ((!(upFlags & IF_CLAMP)) && (upFlags & IT_CUBEMAP))
		upFlags |= IF_CLAMP;

	// tex params
	if (mipmap) {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameterf (pTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (gl_ext_max_anisotropy->integer, 1, glConfig.maxAniso));
	} else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
	}

	qglTexParameterf (pTarget, GL_TEXTURE_MIN_FILTER, mipmap ? glState.texMinFilter : glState.texMagFilter);
	qglTexParameterf (pTarget, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);

	// texture edge clamping
	if (image && (image->flags & (IT_CUBEMAP|IF_CLAMP))) {
		if (glConfig.extTexEdgeClamp) {
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if (image->flags & IT_CUBEMAP)
				qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		} else {
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
			if (image->flags & IT_CUBEMAP)
				qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
		}
	} else {
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// upload
	for (i=0 ; i<times ; i++) {
		scaledWidth = upWidth;
		scaledHeight = upHeight;

		if ((scaledWidth == width) && (scaledHeight == height)) {
			// image is a power of two
			scaled = (unsigned *)data[i];
		} else {
			// not a power of two, must resample
			scaled = Z_TagMalloc (scaledWidth * scaledHeight * 4, ZTAG_IMAGESYS);
			if (!scaled)
				Com_Printf (PRINT_ALL, S_COLOR_RED "Img_Upload: Out of memory!\n");

			Img_ResampleTexture ((unsigned *)data[i], width, height, scaled, scaledWidth, scaledHeight);
		}

		// apply image gamma/intensity
		if (!(upFlags & IF_NOGAMMA))
			Img_LightScaleTexture ((byte *)scaled, scaledWidth, scaledHeight, samples, mipmap && !(upFlags & IF_NOINTENS));

		// upload the base image
		qglTexImage2D (tTarget + i, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

		// upload mipmap levels
		if (mipmap && !glConfig.extSGISGenMipmap) {
			int		mipLevel = 0;

			while ((scaledWidth > 1) || (scaledHeight > 1)) {
				Img_MipMap ((byte *)scaled, scaledWidth, scaledHeight);

				scaledWidth >>= 1;
				scaledHeight >>= 1;

				if (scaledWidth < 1) scaledWidth = 1;
				if (scaledHeight < 1) scaledHeight = 1;

				qglTexImage2D (tTarget + i, ++mipLevel, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}

		if (scaled && (scaled != (unsigned *)data[i]))
			Z_Free (scaled);
	}

	// store upload information
	if (image) {
		image->upFlags = upFlags;
		image->upWidth = upWidth;
		image->upHeight = upHeight;
		image->format = format;
	}
}

/*
==============================================================================

	IMAGE LOADING

==============================================================================
*/

/*
================
Img_FindSlot
================
*/
image_t	*Img_FindSlot (void) {
	int			i;
	image_t		*image = NULL;

	// find a free img_Textures spot
	for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (!image->iRegistrationFrame) {
			image->texNum = i + 1;
			break;
		}
	}

	// none found, create a new spot
	if (i == img_NumTextures) {
		if (img_NumTextures >= MAX_GLTEXTURES) {
			Com_Error (ERR_DROP, "Img_FindSlot: MAX_GLTEXTURES");
			return NULL;
		}

		image = &img_Textures[img_NumTextures++];
		image->texNum = img_NumTextures;
	}

	return image;
}


/*
================
Img_LoadPic

This is also used as an entry point for the generated r_NoTexture
================
*/
image_t *Img_LoadPic (char *name, byte **pic, int width, int height, int flags, int samples) {
	image_t		*image;

	// get a free image spot
	image = Img_FindSlot ();

	if (strlen (name) >= sizeof (image->name))
		Com_Error (ERR_DROP, "Img_LoadPic: \"%s\" is too long", name);

	strcpy (image->name, name);
	Com_StripExtension (image->name, image->bareName);

	image->width = width;
	image->height = height;

	// texture scaling, hacky special case!
	if ((flags & IT_TEXTURE) && !(strcmp (name + strlen (name) - 4, ".png"))) {
		char		newname[MAX_QPATH];
		walTex_t	*mt;

		Com_sprintf (newname, sizeof (newname), "%s.wal", image->bareName);
		FS_LoadFile (newname, (void **)&mt);
		if (mt) {
			image->width = LittleLong (mt->width);
			image->height = LittleLong (mt->height);
			
			FS_FreeFile ((void *)mt);
		}
	}

	// reset
	image->upFlags = image->flags = flags;
	image->iRegistrationFrame = img_RegistrationFrame;
	image->upWidth = width;
	image->upHeight = height;
	image->format = 0;

	// upload
	GL_Bind (0, image);
	Img_Upload (name, pic, width, height, samples, image);

	return image;
}


/*
===============
Img_FindCubeMap

Finds or loads the given image
===============
*/
image_t	*Img_FindCubeMap (char *baseName, int flags) {
	image_t		*image;
	int			i, j, len, checkLen;
	int			samples;
	byte		*pic[6];
	int			width, height, firstSize, firstSamples;
	char		imageName[MAX_QPATH];
	char		newname[MAX_QPATH];

	// check the name
	if (!baseName)
		return NULL;

	len = strlen (baseName);
	if (len < 5) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "Img_FindCubeMap: Image name too short! %s\n", baseName);
		return NULL;
	}
	if (len >= MAX_QPATH) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "Img_FindCubeMap: Image name too long! %s\n", baseName);
		return NULL;
	}

	// convert '\' to '/' and kill the initial '\' or '/'
	strcpy (imageName, baseName);
	if ((imageName[0] == '\\') || (imageName[0] == '/'))
		i = 1;
	else
		i = 0;

	while (imageName[i++]) {
		if (imageName[i] == '\\')
			imageName[i] = '/';
	}

	// trim the extension
	checkLen = len;
	for (i=len ; i>0 ; i--) {
		if (imageName[i] == '.') {
			imageName[i] = '\0';
			checkLen = i;
			break;
		}
	}

	// look for it
	for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;

		if (!Q_strnicmp (imageName, image->name, checkLen) && ((image->flags == flags) || (image->upFlags == flags))) {
			image->iRegistrationFrame = img_RegistrationFrame;
			return image;
		}
	}

	// not found -- load the pic from disk
	for (i=0 ; i<6 ; i++) {
		pic[i] = NULL;

		// PNG
		Com_sprintf (newname, sizeof (newname), "%s_%s.png", imageName, cubeMap_NameSuffix[i]);
		samples = Img_LoadPNG (newname, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// TGA
		Com_sprintf (newname, sizeof (newname), "%s_%s.tga", imageName, cubeMap_NameSuffix[i]);
		samples = Img_LoadTGA (newname, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// JPG
		Com_sprintf (newname, sizeof (newname), "%s_%s.jpg", imageName, cubeMap_NameSuffix[i]);
		samples = Img_LoadJPG (newname, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;
		break;

checkPic:
		if (width != height) {
			Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_FindCubeMap: %s is not square!\n", newname);
			goto emptyTrash;
		}

		if (!i) {
			firstSize = width;
			firstSamples = samples;
		} else {
			if (firstSize != width) {
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_FindCubeMap: Size mismatch with previous in %s! Halting!\n", newname);
				break;
			}

			if (firstSamples != samples) {
				Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_FindCubeMap: Sample mismatch with previous in %s! Halting!\n", newname);
				break;
			}
		}
	}

	if (i != 6) {
		Com_Printf (PRINT_ALL, S_COLOR_YELLOW "Img_FindCubeMap: Unable to find all of the sides, aborting!\n");
		goto emptyTrash;
	} else {
		return Img_LoadPic (imageName, pic, width, height, flags, samples);
	}

emptyTrash:
	for (j=0 ; j<6 ; j++) {
		if (pic[j])
			Z_Free (pic[j]);
	}

	return NULL;
}


/*
===============
Img_FindImage

Finds or loads the given image
===============
*/
image_t	*Img_FindImage (char *name, int flags) {
	image_t		*image;
	int			i, len, checkLen, samples;
	byte		*pic;
	int			width, height;
	char		imageName[MAX_QPATH];
	char		bareName[MAX_QPATH];

	if ((flags & IT_CUBEMAP) && glConfig.extArbTexCubeMap)
		return Img_FindCubeMap (name, flags);

	// check the name
	if (!name)
		return NULL;

	len = strlen (name);
	if (len < 5) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "Img_FindImage: Image name too short! %s\n", name);
		return NULL;
	}
	if (len >= MAX_QPATH) {
		Com_Printf (PRINT_ALL, S_COLOR_RED "Img_FindImage: Image name too long! %s\n", name);
		return NULL;
	}

	// convert '\' to '/' and kill the initial '\' or '/'
	strcpy (imageName, name);
	if ((imageName[0] == '\\') || (imageName[0] == '/'))
		i = 1;
	else
		i = 0;

	while (imageName[i++]) {
		if (imageName[i] == '\\')
			imageName[i] = '/';
	}

	// trim the extension
	strcpy (bareName, imageName);
	checkLen = len;
	for (i=len ; i>0 ; i--) {
		if (bareName[i] == '.') {
			bareName[i] = '\0';
			checkLen = i+1;
			len = i;
			break;
		}
	}
	if (i == 0) // no extension found!
		checkLen = len;

	// look for it
	for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;

		// extension blind, but check for the decimal
		if (!Q_strnicmp (imageName, image->name, checkLen) && ((image->flags == flags) || (image->upFlags == flags))) {
			image->iRegistrationFrame = img_RegistrationFrame;
			return image;
		}
	}

	// not found -- load the pic from disk
	// PNG
	Com_sprintf (imageName, sizeof (imageName), "%s.png", bareName);
	samples = Img_LoadPNG (imageName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// TGA
	Com_sprintf (imageName, sizeof (imageName), "%s.tga", bareName);
	samples = Img_LoadTGA (imageName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// JPG
	Com_sprintf (imageName, sizeof (imageName), "%s.jpg", bareName);
	samples = Img_LoadJPG (imageName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	if (!(strcmp (name + strlen (name) - 4, ".wal"))) {
		// WAL
		Com_sprintf (imageName, sizeof (imageName), "%s.wal", bareName);
		samples = Img_LoadWal (imageName, &pic, &width, &height, flags);
		if (pic && samples)
			goto loadImage;
	} else {
		// PCX
		Com_sprintf (imageName, sizeof (imageName), "%s.pcx", bareName);
		samples = Img_LoadPCX (imageName, &pic, NULL, &width, &height);
		if (pic && samples)
			goto loadImage;
	}

	// nothing found!
	image = NULL;
	goto emptyTrash;

loadImage:
	image = Img_LoadPic (imageName, &pic, width, height, flags, samples);

emptyTrash:
	if (pic)
		Z_Free (pic);

	return image;
}


/*
=============
Img_RegisterPic
=============
*/
image_t *Img_RegisterPic (char *name) {
	image_t	*image;
	char	fullName[MAX_QPATH];

	if (!name)
		return NULL;

	if ((name[0] != '/') && (name[0] != '\\')) {
		Com_sprintf (fullName, sizeof (fullName), "pics/%s.pcx", name);
		image = Img_FindImage (fullName, IF_NOFLUSH|IF_NOMIPMAP);
	} else {
		Com_sprintf (fullName, sizeof (fullName), "%s", name+1);
		image = Img_FindImage (fullName, IF_NOFLUSH|IF_NOMIPMAP);
	}

	return image;
}


/*
===============
Img_RegisterSkin
===============
*/
image_t *Img_RegisterSkin (char *name) {
	return Img_FindImage (name, 0);
}


/*
================
Img_BeginRegistration
================
*/
void Img_BeginRegistration (void) {
	img_RegistrationFrame++;
}


/*
================
Img_EndRegistration

Any image that was not touched on this registration sequence will be freed
================
*/
void Img_EndRegistration (void) {
	int		i;
	image_t	*image;

	for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (image->iRegistrationFrame == img_RegistrationFrame)
			continue;		// used this sequence
		if (!image->iRegistrationFrame)
			continue;		// free image_t slot
		if (image->flags & IF_NOFLUSH) {
			image->iRegistrationFrame = img_RegistrationFrame;
			continue;		// don't free
		}

		// free it
		qglDeleteTextures (1, &image->texNum);
		memset (image, 0, sizeof (*image));
	}
}

/*
==============================================================================

	MISC

==============================================================================
*/

/*
=============
Img_GetPicSize
=============
*/
void Img_GetPicSize (int *w, int *h, char *pic) {
	image_t *gl;

	gl = Img_RegisterPic (pic);

	if (!gl) {
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}


/*
===============
Img_ImageList_f
===============
*/
void Img_ImageList_f (void) {
	unsigned	i, imgs, mips;
	image_t		*image;
	unsigned	tempWidth, tempHeight;
	unsigned	texels;

	Com_Printf (PRINT_ALL, "-----------------------------------------------------\n");
	Com_Printf (PRINT_ALL, "Tex# T  Format GIMC Width Height Name\n");
	Com_Printf (PRINT_ALL, "---- -- ------ ---- ----- ------ --------------------\n");

	for (i=0, imgs=0, mips=0, texels=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;		// free img_Textures slot

		Com_Printf (PRINT_ALL, "%4d ", image->texNum);

		if (image->upFlags & IT_CUBEMAP)		Com_Printf (PRINT_ALL, "CM ");
		else if (image->upFlags & IT_LIGHTMAP)	Com_Printf (PRINT_ALL, "LM "); // should never happen...
		else if (image->upFlags & IF_NOMIPMAP)	Com_Printf (PRINT_ALL, "2D ");
		else									Com_Printf (PRINT_ALL, "   ");

		switch (image->format) {
		case GL_RGBA8:					Com_Printf (PRINT_ALL, "RGBA8  ");		break;
		case GL_RGBA4:					Com_Printf (PRINT_ALL, "RGBA4  ");		break;
		case GL_RGBA:					Com_Printf (PRINT_ALL, "RGBA   ");		break;
		case GL_RGB8:					Com_Printf (PRINT_ALL, "RGB8   ");		break;
		case GL_RGB5:					Com_Printf (PRINT_ALL, "RGB5   ");		break;
		case GL_RGB:					Com_Printf (PRINT_ALL, "RGB    ");		break;

		case GL_DSDT8_NV:				Com_Printf (PRINT_ALL, "DSDT8  ");		break;

		case GL_COMPRESSED_RGBA_ARB:	Com_Printf (PRINT_ALL, "CMRGBA ");		break;
		case GL_COMPRESSED_RGB_ARB:		Com_Printf (PRINT_ALL, "CMRGB  ");		break;

		default:						Com_Printf (PRINT_ALL, "?????? ");		break;
		}

		Com_Printf (PRINT_ALL, "%s", (image->upFlags & IF_NOGAMMA)	? "-" : "G");
		Com_Printf (PRINT_ALL, "%s", (image->upFlags & IF_NOINTENS)	? "-" : "I");
		Com_Printf (PRINT_ALL, "%s", (image->upFlags & IF_NOMIPMAP)	? "-" : "M");
		Com_Printf (PRINT_ALL, "%s", (image->upFlags & IF_CLAMP)	? "C" : "-");

		Com_Printf (PRINT_ALL, " %5i  %5i %s\n", image->upWidth, image->upHeight, image->name);

		imgs++;
		texels += image->upWidth * image->upHeight;
		if (!(image->upFlags & IF_NOMIPMAP)) {
			tempWidth=image->upWidth, tempHeight=image->upHeight;
			while ((tempWidth > 1) || (tempHeight > 1)) {
				tempWidth >>= 1;
				tempHeight >>= 1;

				if (tempWidth < 1) tempWidth = 1;
				if (tempHeight < 1) tempHeight = 1;

				texels += tempWidth * tempHeight;
				mips++;
			}
		}
	}

	Com_Printf (PRINT_ALL, "-----------------------------------------------------\n");
	Com_Printf (PRINT_ALL, "Total images: %d (with mips: %d)\n", imgs, imgs + mips);
	Com_Printf (PRINT_ALL, "Texel count (including mips): %d\n", texels);
	Com_Printf (PRINT_ALL, "-----------------------------------------------------\n");
}


/*
================== 
Img_ScreenShot_f
================== 
*/
void Img_ScreenShot_f (void) {
	if (!strcmp (Cmd_Argv (1), "tga"))
		Img_TGAScreenShot ();
	else if (!strcmp (Cmd_Argv (1), "png"))
		Img_PNGScreenShot ();
	else if (!strcmp (Cmd_Argv (1), "jpg"))
		Img_JPGScreenShot ();
	else {
		if (!strcmp (gl_screenshot->string, "png"))
			Img_PNGScreenShot ();
		else if (!strcmp (gl_screenshot->string, "jpg"))
			Img_JPGScreenShot ();
		else
			Img_TGAScreenShot ();
	}
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
==================
Img_InitSpecialTextures
==================
*/
#define DST_SIZE	16
void Img_InitSpecialTextures (void) {
	int		x, y;
	byte	*cin;
	byte	*data;

	/*
	** r_NoTexture
	*/
	data = Z_TagMalloc (8 * 8 * 4, ZTAG_IMAGESYS);
	for (x=0 ; x<8 ; x++) {
		for (y=0 ; y<8 ; y++) {
			data[(y*8 + x)*4+0] = clamp (x*16 + y*16, 0, 255);
			data[(y*8 + x)*4+1] = 0;
			data[(y*8 + x)*4+2] = 0;
			data[(y*8 + x)*4+3] = 255;
		}
	}

	r_NoTexture = Img_LoadPic ("***r_NoTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3);

	/*
	** r_WhiteTexture
	*/
	memset (data, 255, 8 * 8 * 4);
	r_WhiteTexture = Img_LoadPic ("***r_WhiteTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3);

	/*
	** r_BlackTexture
	*/
	memset (data, 0, 8 * 8 * 4);
	r_BlackTexture = Img_LoadPic ("***r_BlackTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3);

	Z_Free (data);

	/*
	** r_CinTexture
	** Reserve a texNum for cinematics
	*/
	cin = Z_TagMalloc (256 * 256 * 4, ZTAG_IMAGESYS);
	memset (cin, 0, 256 * 256 * 4);
	r_CinTexture = Img_LoadPic ("***r_CinTexture***", &cin, 256, 256, IF_NOFLUSH|IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP|IF_NOINTENS|IF_NOGAMMA, 3);
	Z_Free (cin);

	/*
	** dst texture for turb surfaces
	*/
	if (glConfig.extArbMultitexture && glConfig.extNVTexShader && glConfig.extMTexCombine) {
		signed char	data[DST_SIZE][DST_SIZE][2];

		for (x=0 ; x<DST_SIZE ; x++) {
			for (y=0 ; y<DST_SIZE ; y++) {
				data[x][y][0] = ( rand () % 257 ) - 128;
				data[x][y][1] = ( rand () % 257 ) - 128;
			}
		}

		// get a free image spot
		r_DSTTexture = Img_FindSlot ();

		strcpy (r_DSTTexture->name,		"***r_DSTTexture***");
		strcpy (r_DSTTexture->bareName,	"***r_DSTTexture***");

		r_DSTTexture->upFlags = r_DSTTexture->flags = IF_NOFLUSH|IF_NOMIPMAP;
		r_DSTTexture->upHeight = r_DSTTexture->height = DST_SIZE;
		r_DSTTexture->upWidth = r_DSTTexture->width = DST_SIZE;
		r_DSTTexture->iRegistrationFrame = img_RegistrationFrame;
		r_DSTTexture->format = GL_DSDT8_NV;

		GL_Bind (0, r_DSTTexture);

		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glState.texMagFilter);

		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		qglTexImage2D (GL_TEXTURE_2D, 0, GL_DSDT8_NV, DST_SIZE, DST_SIZE, 0, GL_DSDT_NV, GL_BYTE, data);
	}
}


/*
==================
Img_Register
==================
*/
void Img_Register (void) {
	intensity			= Cvar_Get ("intensity",			"2",						CVAR_ARCHIVE);

	gl_jpgquality		= Cvar_Get ("gl_jpgquality",		"85",						CVAR_ARCHIVE);
	gl_nobind			= Cvar_Get ("gl_nobind",			"0",						0);
	gl_picmip			= Cvar_Get ("gl_picmip",			"0",						CVAR_LATCHVIDEO);
	gl_screenshot		= Cvar_Get ("gl_screenshot",		"tga",						CVAR_ARCHIVE);

	gl_texturebits		= Cvar_Get ("gl_texturebits",		"default",					CVAR_ARCHIVE|CVAR_LATCHVIDEO);
	gl_texturemode		= Cvar_Get ("gl_texturemode",		"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE);

	vid_gamma			= Cvar_Get ("vid_gamma",			"1.0",						CVAR_ARCHIVE);
	vid_gammapics		= Cvar_Get ("vid_gammapics",		"0",						CVAR_ARCHIVE|CVAR_LATCHVIDEO);

	Cmd_AddCommand ("imagelist",	Img_ImageList_f);
	Cmd_AddCommand ("screenshot",	Img_ScreenShot_f);
}


/*
==================
Img_Unregister
==================
*/
void Img_Unregister (void) {
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("screenshot");
}


/*
===============
Img_Init
===============
*/
void Img_Init (void) {
	int			i, j;
	float		gam;
	int			red, green, blue;
	unsigned	v;
	byte		*pal;

	Com_Printf (PRINT_ALL, "\n--------- Image Initialization ---------\n");

	Img_Register ();

	img_NumTextures = 0;
	img_RegistrationFrame = 1;
	memset (img_Textures, 0, sizeof (img_Textures));

	// set the initial state
	glState.texUnit = 0;
	VectorSet (glState.texNums, -1, -1, -1);
	VectorSet (glState.texEnvModes, -1, -1, -1);

	GL_TextureMode ();
	GL_TextureBits ();

	qglEnable (GL_TEXTURE_2D);
	if (glConfig.extArbTexCubeMap)
		qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glState.texMagFilter);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);

	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (glConfig.extArbTexCubeMap)
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	if (glConfig.extArbTexCubeMap)
		qglDisable (GL_TEXTURE_GEN_R);

	GL_TexEnv (GL_MODULATE);

	// get the palette
	Img_LoadPCX ("pics/colormap.pcx", NULL, &pal, NULL, NULL);
	if (!pal)
		pal = img_DefaultPal;

	for (i=0 ; i<256 ; i++) {
		red = pal[i*3+0];
		green = pal[i*3+1];
		blue = pal[i*3+2];
		
		v = (255<<24) + (red<<0) + (green<<8) + (blue<<16);
		img_PaletteTable[i] = LittleLong (v);
	}

	img_PaletteTable[255] &= LittleLong (0xffffff);	// 255 is transparent

	if (pal != img_DefaultPal)
		Z_Free (pal);

	// set up the gamma and intensity ramps
	if (intensity->value < 1)
		Cvar_ForceSetValue ("intensity", 1);

	glState.invIntens = 1.0 / intensity->value;

	// hack! because voodoo's are nasty bright
	if (glConfig.rendType & GLREND_VOODOO)
		gam = 1.0f;
	else
		gam = vid_gamma->value;

	// gamma
	if (gam == 1) {
		for (i=0 ; i<256 ; i++)
			img_GammaTable[i] = i;
	} else {
		for (i=0 ; i<256 ; i++)
			img_GammaTable[i] = clamp (255 * pow ((i + 0.5)*0.0039138943248532289628180039138943 , gam) + 0.5, 0, 255);
	}

	// intensity (eww)
	for (i=0 ; i<256 ; i++) {
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		img_IntensityTable[i] = j;
	}

	// load up special textures
	Img_InitSpecialTextures ();

	Com_Printf (PRINT_ALL, "----------------------------------------\n");
}


/*
===============
Img_Shutdown
===============
*/
void Img_Shutdown (void) {
	int		i;
	image_t	*image;

	// unregister cvars
	Img_Unregister ();

	// free loaded textures
	for (i=0, image=img_Textures ; i<img_NumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;	// free img_Textures slot

		// free it
		qglDeleteTextures (1, &image->texNum);
	}

	img_NumTextures = 0;
	memset (img_Textures, 0, sizeof (img_Textures));
	Z_FreeTags (ZTAG_IMAGESYS);
}
