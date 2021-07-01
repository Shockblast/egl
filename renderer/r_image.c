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
// r_image.c
// Loads and uploads images to the video card
//

#define USE_BYTESWAP
#include "r_local.h"
#include "jpg/jpeglib.h"
#include "png/png.h"
#include <process.h>

#define MAX_IMAGE_HASH	128

image_t		r_lmTextures[MAX_LIGHTMAPS];
image_t		r_imageList[MAX_IMAGES];
image_t		*r_imageHashTree[MAX_IMAGE_HASH];
uInt		r_numImages;

static byte	r_intensityTable[256];
static byte	r_gammaTable[256];
uInt		r_paletteTable[256];

static char	r_bareImageName[MAX_QPATH];

const char	*sky_NameSuffix[6]		= { "rt", "bk", "lf", "ft", "up", "dn" };
const char	*cubeMap_NameSuffix[6]	= { "px", "nx", "py", "ny", "pz", "nz" };

byte r_defaultImagePal[] = {
	#include "defpal.h"
};

image_t	*r_noTexture;		// use for bad textures
image_t	*r_whiteTexture;	// used in shaders/fallback
image_t	*r_blackTexture;	// used in shaders/fallback
image_t	*r_cinTexture;		// allocates memory on load as to not every cin frame

cVar_t *intensity;

cVar_t *gl_jpgquality;
cVar_t *gl_nobind;
cVar_t *gl_picmip;
cVar_t *gl_screenshot;
cVar_t *gl_texturebits;
cVar_t *gl_texturemode;

cVar_t *vid_gamma;

/*
==============================================================================

	TEXTURE

==============================================================================
*/

/*
===============
GL_BindTexture
===============
*/
void GL_BindTexture (image_t *image)
{
	int		texNum;

	if (gl_nobind->integer || !image)	// performance evaluation option
		texNum = r_noTexture->texNum;
	else
		texNum = image->texNum;
	if (glState.texNums[glState.texUnit] == texNum)
		return;

	glState.texNums[glState.texUnit] = texNum;
	qglBindTexture ((image) ? image->target : GL_TEXTURE_2D, texNum);
	glSpeeds.textureBinds++;
}


/*
===============
GL_SelectTexture
===============
*/
void GL_SelectTexture (int target)
{
	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture)
		return;
	if (target > glConfig.maxTexUnits) {
		Com_Error (ERR_DROP, "Attempted selection of an out of bounds (%d) texture unit!", target);
		return;
	}
	if (target == glState.texUnit)
		return;

	glSpeeds.texUnitChanges++;
	glState.texUnit = target;
	if (qglSelectTextureSGIS) {
		qglSelectTextureSGIS (target + GL_TEXTURE0_SGIS);
	}
	else if (qglActiveTextureARB) {
		qglActiveTextureARB (target + GL_TEXTURE0_ARB);
		qglClientActiveTextureARB (target + GL_TEXTURE0_ARB);
	}
}


/*
===============
GL_TextureEnv
===============
*/
void GL_TextureEnv (GLfloat mode)
{
	if (!glConfig.extTexEnvAdd && mode == GL_ADD) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glState.texEnvModes[glState.texUnit] = GL_MODULATE;
		glSpeeds.texEnvChanges++;
		return;
	}

	if (mode != glState.texEnvModes[glState.texUnit]) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		glState.texEnvModes[glState.texUnit] = mode;
		glSpeeds.texEnvChanges++;
	}
}


/*
===============
GL_LoadTexMatrix
===============
*/
void GL_LoadTexMatrix (mat4x4_t m)
{
	qglLoadMatrixf (m);
	glState.texMatIdentity[glState.texUnit] = qFalse;
}


/*
===============
GL_LoadIdentityTexMatrix
===============
*/
void GL_LoadIdentityTexMatrix (void)
{
	if (!glState.texMatIdentity[glState.texUnit]) {
		qglLoadIdentity ();
		glState.texMatIdentity[glState.texUnit] = qTrue;
	}
}


/*
==================
GL_TextureBits
==================
*/
void GL_TextureBits (qBool verbose, qBool verboseOnly)
{
	// Print to the console if desired
	if (verbose) {
		switch (gl_texturebits->integer) {
		case 32:
			Com_Printf (0, "Texture bits: 32\n");
			break;

		case 16:
			Com_Printf (0, "Texture bits: 16\n");
			break;

		default:
			Com_Printf (0, "Texture bits: default\n");
			break;
		}

		// Only print (don't set)
		if (verboseOnly)
			return;
	}

	// Set
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
}


/*
===============
GL_TextureMode
===============
*/
typedef struct glTexMode_s {
	char	*name;

	GLint	minimize;
	GLint	maximize;
} glTexMode_t;

static const glTexMode_t modes[] = {
	{"GL_NEAREST",					GL_NEAREST,					GL_NEAREST},
	{"GL_LINEAR",					GL_LINEAR,					GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST",	GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST",	GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR",	GL_NEAREST_MIPMAP_LINEAR,	GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR",		GL_LINEAR_MIPMAP_LINEAR,	GL_LINEAR}
};

#define NUM_GL_MODES (sizeof (modes) / sizeof (glTexMode_t))

void GL_TextureMode (qBool verbose, qBool verboseOnly)
{
	uInt	i;
	image_t	*image;
	char	*string = gl_texturemode->string;

	// Find a matching mode
	for (i=0 ; i<NUM_GL_MODES ; i++) {
		if (!Q_stricmp (modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES) {
		// Not found
		Com_Printf (0, S_COLOR_YELLOW "bad filter name -- falling back to GL_LINEAR_MIPMAP_NEAREST\n");
		Cvar_VariableForceSet (gl_texturemode, "GL_LINEAR_MIPMAP_NEAREST");
		
		glState.texMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		glState.texMagFilter = GL_LINEAR;
	}
	else {
		// Found
		glState.texMinFilter = modes[i].minimize;
		glState.texMagFilter = modes[i].maximize;
	}

	gl_texturemode->modified = qFalse;
	if (verbose) {
		Com_Printf (0, "Texture mode: %s\n", modes[i].name);
		if (verboseOnly)
			return;
	}

	// Change all the existing mipmap texture objects
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;		// Free r_imageList slot
		if (image->flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST))
			continue;

		GL_BindTexture (image);

		qglTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, glState.texMinFilter);
		qglTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
}


/*
===============
GL_ResetAnisotropy
===============
*/
void GL_ResetAnisotropy (void)
{
	uInt	i;
	image_t	*image;

	gl_ext_max_anisotropy->modified = qFalse;
	if (!glConfig.extTexFilterAniso)
		return;

	// Change all the existing mipmap texture objects
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;		// Free r_imageList slot
		if (image->flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST))
			continue;		// Skip non-mipmapped imagery

		GL_BindTexture (image);
		qglTexParameteri (image->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (gl_ext_max_anisotropy->integer, 1, glConfig.maxAniso));
	}
}

/*
==============================================================================
 
	JPG
 
==============================================================================
*/

static void jpg_noop(j_decompress_ptr cinfo)
{
}

static void jpeg_d_error_exit (j_common_ptr cinfo)
{
	char msg[1024];

	(cinfo->err->format_message)(cinfo, msg);
	Com_Error (ERR_FATAL, "R_LoadJPG: JPEG Lib Error: '%s'", msg);
}

static boolean jpg_fill_input_buffer (j_decompress_ptr cinfo)
{
    Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadJPG: Premeture end of jpeg file\n");

    return 1;
}

static void jpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}

static void jpeg_mem_src (j_decompress_ptr cinfo, byte *mem, int len)
{
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
R_LoadJPG

ala Vic
=============
*/
static int R_LoadJPG (char *name, byte **pic, int *width, int *height)
{
    int		length, samples;
    byte	*img, *scan, *buffer, *dummy;
    struct	jpeg_error_mgr			jerr;
    struct	jpeg_decompress_struct	cinfo;
	uInt	i;

	if (pic)
		*pic = NULL;

	// Load the file
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return 0;

	// Parse the file
	cinfo.err = jpeg_std_error (&jerr);
	jerr.error_exit = jpeg_d_error_exit;

	jpeg_create_decompress (&cinfo);

	jpeg_mem_src (&cinfo, buffer, length);
	jpeg_read_header (&cinfo, TRUE);

	jpeg_start_decompress (&cinfo);

	samples = cinfo.output_components;
    if (samples != 3 && samples != 1) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadJPG: Bad jpeg samples '%s' (%d)\n", name, samples);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (cinfo.output_width <= 0 || cinfo.output_height <= 0) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadJPG: Bad jpeg dimensions on '%s' ( %d x %d )\n", name, cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (width)
		*width = cinfo.output_width;
	if (height)
		*height = cinfo.output_height;

	img = Mem_PoolAlloc (cinfo.output_width * cinfo.output_height * 4, MEMPOOL_IMAGESYS, 0);
	dummy = Mem_PoolAlloc (cinfo.output_width * samples, MEMPOOL_IMAGESYS, 0);

	if (pic)
		*pic = img;

	while (cinfo.output_scanline < cinfo.output_height) {
		scan = dummy;
		if (!jpeg_read_scanlines (&cinfo, &scan, 1)) {
			Com_Printf (0, S_COLOR_YELLOW "Bad jpeg file %s\n", name);
			jpeg_destroy_decompress (&cinfo);
			Mem_Free (dummy);
			FS_FreeFile (buffer);
			return 0;
		}

		if (samples == 1) {
			for (i=0 ; i<cinfo.output_width ; i++, img+=4)
				img[0] = img[1] = img[2] = *scan++;
		}
		else {
			for (i=0 ; i<cinfo.output_width ; i++, img+=4, scan += 3)
				img[0] = scan[0], img[1] = scan[1], img[2] = scan[2];
		}
	}

    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);

    Mem_Free (dummy);
	FS_FreeFile (buffer);

	return 3;
}


/*
================== 
R_WriteJPG
================== 
*/

static void R_WriteJPG (FILE *f, byte *buffer, int width, int height, int quality)
{
	int			offset, w3;
	struct		jpeg_compress_struct	cinfo;
	struct		jpeg_error_mgr			jerr;
	byte		*s;

	// Initialise the jpeg compression object
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, f);

	// Setup jpeg parameters
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	cinfo.progressive_mode = TRUE;

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	jpeg_start_compress (&cinfo, qTrue);	// start compression
	jpeg_write_marker (&cinfo, JPEG_COM, (byte *) "EGL v" EGL_VERSTR, (uInt) strlen ("EGL v" EGL_VERSTR));

	// Feed scanline data
	w3 = cinfo.image_width * 3;
	offset = w3 * cinfo.image_height - w3;
	while (cinfo.next_scanline < cinfo.image_height) {
		s = &buffer[offset - (cinfo.next_scanline * w3)];
		jpeg_write_scanlines (&cinfo, &s, 1);
	}

	// Finish compression
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);
}

/*
==============================================================================

	PCX

==============================================================================
*/

typedef struct pcxHeader_s {
	char		manufacturer;
	char		version;
	char		encoding;
	char		bitsPerPixel;

	uShort		xMin, yMin, xMax, yMax;
	uShort		hRes, vRes;
	byte		palette[48];

	char		reserved;
	char		colorPlanes;

	uShort		bytesPerLine;
	uShort		paletteType;

	char		filler[58];

	byte		data;			// unbounded
} pcxHeader_t;

/*
=============
R_LoadPCX
=============
*/
void R_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte		*raw;
	pcxHeader_t	*pcx;
	int			x, y, fileLen;
	int			columns, rows;
	int			dataByte, runLength;
	byte		*out, *pix;

	if (pic)
		*pic = NULL;
	if (palette)
		*palette = NULL;

	// Load the file
	fileLen = FS_LoadFile (filename, (void **)&raw);
	if (!raw || fileLen <= 0)
		return;

	// Parse the PCX file
	pcx = (pcxHeader_t *)raw;

	pcx->xMin = LittleShort (pcx->xMin);
	pcx->yMin = LittleShort (pcx->yMin);
	pcx->xMax = LittleShort (pcx->xMax);
	pcx->yMax = LittleShort (pcx->yMax);
	pcx->hRes = LittleShort (pcx->hRes);
	pcx->vRes = LittleShort (pcx->vRes);
	pcx->bytesPerLine = LittleShort (pcx->bytesPerLine);
	pcx->paletteType = LittleShort (pcx->paletteType);

	raw = &pcx->data;

	// Sanity checks
	if (pcx->manufacturer != 0x0a || pcx->version != 5 || pcx->encoding != 1) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadPCX: Invalid PCX header: %s\n", filename);
		return;
	}

	if (pcx->bitsPerPixel != 8 || pcx->colorPlanes != 1) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadPCX: Only 8-bit PCX images are supported: %s\n", filename);
		return;
	}

	if (pcx->xMax >= 640 || pcx->xMax <= 0 || pcx->yMax >= 480 || pcx->yMax <= 0) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadPCX: Bad PCX file dimensions: %s: %i x %i\n", filename, pcx->xMax, pcx->yMax);
		return;
	}

	columns = pcx->xMax + 1;
	rows = pcx->yMax + 1;

	pix = out = Mem_PoolAlloc (columns * rows, MEMPOOL_IMAGESYS, 0);

	if (palette) {
		*palette = Mem_PoolAlloc (768, MEMPOOL_IMAGESYS, 0);
		memcpy (*palette, (byte *)pcx + fileLen - 768, 768);
	}

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	for (y=0 ; y<=pcx->yMax ; y++, pix+=pcx->xMax+1) {
		for (x=0 ; x<=pcx->xMax ; ) {
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}
	}

	if (raw - (byte *)pcx > fileLen) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadPCX: PCX file %s was malformed", filename);
		Mem_Free (out);
		out = NULL;
		pix = NULL;

		if (pic)
			*pic = NULL;
		if (palette) {
			Mem_Free (palette);		// Echon: test
			*palette = NULL;		// Echon: test
		}
	}
	else if (pic)
		*pic = out;

	FS_FreeFile (pcx);
}

/*
==============================================================================

	PNG

==============================================================================
*/

typedef struct pngBuf_s {
	byte	*buffer;
	int		pos;
} pngBuf_t;

void __cdecl PngReadFunc (png_struct *Png, png_bytep buf, png_size_t size)
{
	pngBuf_t *PngFileBuffer = (pngBuf_t*)png_get_io_ptr(Png);
	memcpy (buf,PngFileBuffer->buffer + PngFileBuffer->pos, size);
	PngFileBuffer->pos += size;
}

/*
=============
R_LoadPNG
=============
*/
#define OLDPNGCODE
static int R_LoadPNG (char *name, byte **pic, int *width, int *height)
{
#ifdef OLDPNGCODE
	int				rowptr;
	int				samples;
	png_structp		png_ptr;
	png_infop		info_ptr;
	png_infop		end_info;
	byte			**row_pointers;
	byte			*img;
#else
	int				rowbytes;
	int				samples;
	png_structp		png_ptr;
	png_infop		info_ptr;
	png_infop		end_info;
	png_bytepp		row_pointers;
	png_bytep		pic_ptr;
#endif
	uInt			i;

	pngBuf_t	PngFileBuffer = {NULL,0};

	if (pic)
		*pic = NULL;

	// Load the file
	FS_LoadFile (name, &PngFileBuffer.buffer);
	if (!PngFileBuffer.buffer)
		return 0;

	// Parse the PNG file
	if ((png_check_sig (PngFileBuffer.buffer, 8)) == 0) {
		Com_Printf (0, S_COLOR_YELLOW "R_LoadPNG: Not a PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	PngFileBuffer.pos = 0;

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,  NULL, NULL);
	if (!png_ptr) {
		Com_Printf (0, S_COLOR_YELLOW "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}
	
	end_info = png_create_info_struct (png_ptr);
	if (!end_info) {
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	png_set_read_fn (png_ptr, (png_voidp)&PngFileBuffer, (png_rw_ptr)PngReadFunc);

#ifdef OLDPNGCODE
	png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	row_pointers = png_get_rows (png_ptr, info_ptr);

	rowptr = 0;

	img = Mem_PoolAlloc (info_ptr->width * info_ptr->height * 4, MEMPOOL_IMAGESYS, 0);
	if (pic)
		*pic = img;

	if (info_ptr->channels == 4) {
		for (i=0 ; i<info_ptr->height ; i++) {
			memcpy (img + rowptr, row_pointers[i], info_ptr->rowbytes);
			rowptr += info_ptr->rowbytes;
		}
	}
	else {
		int		x;
		uInt	j;

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
	samples = info_ptr->channels;

	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);

	FS_FreeFile (PngFileBuffer.buffer);
	return samples;
#else
	png_read_info (png_ptr, info_ptr);

	// Color
	if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (png_ptr);
		png_read_update_info (png_ptr, info_ptr);
	}

	if (info_ptr->color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler (png_ptr, 0xFF, PNG_FILLER_AFTER);

	if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8)
		png_set_gray_1_2_4_to_8 (png_ptr);

	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png_ptr);

	if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY || info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb (png_ptr);

	if (info_ptr->bit_depth == 16)
		png_set_strip_16 (png_ptr);

	if (info_ptr->bit_depth < 8)
        png_set_packing (png_ptr);

	png_read_update_info (png_ptr, info_ptr);

	rowbytes = png_get_rowbytes (png_ptr, info_ptr);

	pic_ptr = Mem_PoolAlloc (info_ptr->height * rowbytes, MEMPOOL_IMAGESYS, 0);
	if (pic)
		*pic = pic_ptr;

	row_pointers = Mem_PoolAlloc (sizeof (png_bytep) * info_ptr->height, MEMPOOL_IMAGESYS, 0);

	for (i=0 ; i<info_ptr->height ; i++) {
		row_pointers[i] = pic_ptr;
		pic_ptr += rowbytes;
	}

	png_read_image (png_ptr, row_pointers);

	if (width)
		*width = info_ptr->width;
	if (height)
		*height = info_ptr->height;
	samples = info_ptr->channels;

	png_read_end (png_ptr, end_info);
	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);

	Mem_Free (row_pointers);
	FS_FreeFile (PngFileBuffer.buffer);

	return samples;
#endif
}


/*
================== 
R_PNGScreenShot
================== 
*/
static void R_WritePNG (FILE *f, byte *buffer, int width, int height)
{
	int			i;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep	*row_pointers;

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		Com_Printf (0, S_COLOR_YELLOW "R_WritePNG: LibPNG Error!\n");
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "R_WritePNG: LibPNG Error!\n");
		return;
	}

	png_init_io (png_ptr, f);

	png_set_IHDR (png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_set_compression_level (png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_compression_mem_level (png_ptr, 9);

	png_write_info (png_ptr, info_ptr);

	row_pointers = Mem_PoolAlloc (height * sizeof (png_bytep), MEMPOOL_IMAGESYS, 0);
	for (i=0 ; i<height ; i++)
		row_pointers[i] = buffer + (height - 1 - i) * 3 * width;

	png_write_image (png_ptr, row_pointers);
	png_write_end (png_ptr, info_ptr);

	png_destroy_write_struct (&png_ptr, &info_ptr);

	Mem_Free (row_pointers);
}

/*
==============================================================================

	TGA

==============================================================================
*/

typedef struct targaHeader_s {
	byte	idLength;
	byte	colorMapType;
	byte	imageType;

	uShort	colorMapIndex;
	uShort	colorMapLength;
	byte	colorMapSize;

	uShort	xOrigin;
	uShort	yOrigin;
	uShort	width;
	uShort	height;

	byte	pixelSize;

	byte	attributes;
} targaHeader_t;

/*
=============
R_LoadTGA
=============
*/
static int R_LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int				i, columns, rows, row_inc, row, col;
	byte			*buf_p, *buffer, *pixbuf, *targaRGBA;
	int				length, samples, readPixelCount, pixelCount;
	byte			palette[256][4], red, green, blue, alpha;
	qBool			compressed;
	targaHeader_t	targaHeader;

	*pic = NULL;

	// Load the file
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer || length <= 0)
		return 0;

	// Parse the header
	buf_p = buffer;
	targaHeader.idLength = *buf_p++;
	targaHeader.colorMapType = *buf_p++;
	targaHeader.imageType = *buf_p++;

	targaHeader.colorMapIndex = buf_p[0] + buf_p[1] * 256; buf_p+=2;
	targaHeader.colorMapLength = buf_p[0] + buf_p[1] * 256; buf_p+=2;
	targaHeader.colorMapSize = *buf_p++;
	targaHeader.xOrigin = LittleShort (*((short *)buf_p)); buf_p+=2;
	targaHeader.yOrigin = LittleShort (*((short *)buf_p)); buf_p+=2;
	targaHeader.width = LittleShort (*((short *)buf_p)); buf_p+=2;
	targaHeader.height = LittleShort (*((short *)buf_p)); buf_p+=2;
	targaHeader.pixelSize = *buf_p++;
	targaHeader.attributes = *buf_p++;

	// Skip TARGA image comment
	if (targaHeader.idLength != 0)
		buf_p += targaHeader.idLength;

	compressed = qFalse;
	switch (targaHeader.imageType) {
	case 9:
		compressed = qTrue;
	case 1:
		// Uncompressed colormapped image
		if (targaHeader.pixelSize != 8) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: Only 8 bit images supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targaHeader.colorMapLength != 256) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: Only 8 bit colormaps are supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targaHeader.colorMapIndex) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: colorMapIndex is not supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}

		switch (targaHeader.colorMapSize) {
		case 32:
			for (i=0 ; i<targaHeader.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = *buf_p++;
			}
			break;

		case 24:
			for (i=0 ; i<targaHeader.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = 255;
			}
			break;

		default:
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: only 24 and 32 bit colormaps are supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		break;

	case 10:
		compressed = qTrue;
	case 2:
		// Uncompressed or RLE compressed RGB
		if (targaHeader.pixelSize != 32 && targaHeader.pixelSize != 24) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: Only 32 or 24 bit images supported for type 2 and 10\n");
			FS_FreeFile (buffer);
			return 0;
		}
		break;

	case 11:
		compressed = qTrue;
	case 3:
		// Uncompressed greyscale
		if (targaHeader.pixelSize != 8) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "R_LoadTGA: Only 8 bit images supported for type 3 and 11");
			FS_FreeFile (buffer);
			return 0;
		}
		break;
	}

	columns = targaHeader.width;
	if (width)
		*width = columns;

	rows = targaHeader.height;
	if (height)
		*height = rows;

	targaRGBA = Mem_PoolAlloc (columns * rows * 4, MEMPOOL_IMAGESYS, 0);
	*pic = targaRGBA;

	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if (targaHeader.attributes & 0x20) {
		pixbuf = targaRGBA;
		row_inc = 0;
	}
	else {
		pixbuf = targaRGBA + (rows - 1) * columns * 4;
		row_inc = -columns * 4 * 2;
	}

	for (row=col=0, samples=3 ; row<rows ; ) {
		pixelCount = 0x10000;
		readPixelCount = 0x10000;

		if (compressed) {
			pixelCount = *buf_p++;
			if (pixelCount & 0x80)	// Run-length packet
				readPixelCount = 1;
			pixelCount = 1 + (pixelCount & 0x7f);
		}

		while (pixelCount-- && row < rows) {
			if (readPixelCount-- > 0) {
				switch (targaHeader.imageType) {
				case 1:
				case 9:
					// Colormapped image
					blue = *buf_p++;
					red = palette[blue][0];
					green = palette[blue][1];
					alpha = palette[blue][3];
					blue = palette[blue][2];
					if (alpha != 255)
						samples = 4;
					break;
				case 2:
				case 10:
					// 24 or 32 bit image
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alpha = 255;
					if (targaHeader.pixelSize == 32) {
						alpha = *buf_p++;
						if (alpha != 255)
							samples = 4;
					}
					break;
				case 3:
				case 11:
					// Greyscale image
					blue = green = red = *buf_p++;
					alpha = 255;
					break;
				}
			}

			*pixbuf++ = red;
			*pixbuf++ = green;
			*pixbuf++ = blue;
			*pixbuf++ = alpha;
			if (++col == columns) {
				// Run spans across rows
				row++;
				col = 0;
				pixbuf += row_inc;
			}
		}
	}

	FS_FreeFile (buffer);

	return samples;
}


/*
================== 
R_WriteTGA
================== 
*/
static void R_WriteTGA (FILE *f, byte *buffer, int width, int height)
{
	int		i, size, temp;
	byte	*out;

	// Allocate an output buffer
	size = (width * height * 3) + 18;
	out = Mem_PoolAlloc (size, MEMPOOL_IMAGESYS, 0);

	// Fill in header
	out[2] = 2;		// Uncompressed type
	out[12] = width & 255;
	out[13] = width >> 8;
	out[14] = height & 255;
	out[15] = height >> 8;
	out[16] = 24;	// Pixel size

	// Copy to temporary buffer
	memcpy (out + 18, buffer, width * height * 3);

	// Swap rgb to bgr
	for (i=18 ; i<size ; i+=3) {
		temp = out[i];
		out[i] = out[i+2];
		out[i+2] = temp;
	}

	fwrite (out, 1, size, f);

	Mem_Free (out);
}

/*
==============================================================================

	WAL

==============================================================================
*/

#define	MIPLEVELS	4
typedef struct walTex_s {
	char		name[32];
	uInt		width;
	uInt		height;
	uInt		offsets[MIPLEVELS];		// four mip maps stored
	char		animName[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} walTex_t;

/*
================
R_LoadWal
================
*/
static void R_LoadWal (char *name, byte **pic, int *width, int *height)
{
	walTex_t	*mt;
	byte		*buffer;

	// Load the file
	FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return;

	// Parse the WAL file
	mt = (walTex_t *)buffer;

	mt->width = LittleLong (mt->width);
	mt->height = LittleLong (mt->height);
	mt->offsets[0] = LittleLong (mt->offsets[0]);

	if (mt->width <= 0 || mt->height <= 0) {
		Com_Printf (PRNT_DEV, "R_LoadWal: bad WAL file 's' (%i x %i)\n", name, mt->width, mt->height);
		FS_FreeFile ((void *)buffer);
		return;
	}

	if (width)
		*width = mt->width;
	if (height)
		*height = mt->height;

	*pic = Mem_PoolAlloc (mt->width * mt->height * 3, MEMPOOL_IMAGESYS, 0);
	memcpy (*pic, (byte *)mt + mt->offsets[0], mt->width * mt->height);
	FS_FreeFile ((void *)buffer);
}

/*
==============================================================================

	PRE-UPLOAD HANDLING

==============================================================================
*/

/*
================
R_HeightmapToNormalmap
================
*/
static void R_HeightmapToNormalmap (const byte *in, byte *out, int width, int height, float bumpScale)
{
	int			x, y;
	vec3_t		n;
	float		ibumpScale;
	const byte	*p0, *p1, *p2;

	if (!bumpScale)
		bumpScale = 1.0f;
	bumpScale *= max (0, r_bumpScale->value);
	ibumpScale = (255.0 * 3.0) / bumpScale;

	memset (out, 255, width * height * 4);
	for (y=0 ; y<height ; y++) {
		for (x=0 ; x<width ; x++, out += 4) {
			p0 = in + (y * width + x) * 4;
			p1 = (x == width - 1) ? p0 - x * 4 : p0 + 4;
			p2 = (y == height - 1) ? in + x * 4 : p0 + width * 4;

			n[0] = (p0[0] + p0[1] + p0[2]) - (p1[0] + p1[1] + p1[2]);
			n[1] = (p2[0] + p2[1] + p2[2]) - (p0[0] + p0[1] + p0[2]);
			n[2] = ibumpScale;
			VectorNormalize (n, n);

			out[0] = (n[0] + 1) * 127.5f;
			out[1] = (n[1] + 1) * 127.5f;
			out[2] = (n[2] + 1) * 127.5f;
		}
	}
}


/*
================
R_LightScaleImage

Scale up the pixel values in a texture to increase the lighting range
================
*/
static void R_LightScaleImage (uInt *in, int inWidth, int inHeight, int samples, qBool useGamma, qBool useIntensity)
{
	int		i, c;
	byte	*out;

	out = (byte *)in;
	c = inWidth * inHeight;

	if (useGamma) {
		if (useIntensity) {
			for (i=0 ; i<c ; i++, out+=4) {
				out[0] = r_gammaTable[r_intensityTable[out[0]]];
				out[1] = r_gammaTable[r_intensityTable[out[1]]];
				out[2] = r_gammaTable[r_intensityTable[out[2]]];
			}
		}
		else {
			for (i=0 ; i<c ; i++, out+=4) {
				out[0] = r_gammaTable[out[0]];
				out[1] = r_gammaTable[out[1]];
				out[2] = r_gammaTable[out[2]];
			}
		}
	}
	else {
		if (useIntensity) {
			for (i=0 ; i<c ; i++, out+=4) {
				out[0] = r_intensityTable[out[0]];
				out[1] = r_intensityTable[out[1]];
				out[2] = r_intensityTable[out[2]];
			}
		}
	}
}


/*
================
R_MipmapImage

Operates in place, quartering the size of the texture
================
*/
static void R_MipmapImage (byte *in, int width, int height)
{
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
================
R_ResampleImage
================
*/
static void R_ResampleImage (uInt *in, int inWidth, int inHeight, uInt *out, int outWidth, int outHeight)
{
	int		i, j;
	uInt	*inrow, *inrow2;
	uInt	frac, fracstep;
	uInt	*p1, *p2;
	byte	*pix1, *pix2, *pix3, *pix4;
	uInt	*resampleBuffer;

	if (inWidth == outWidth && inHeight == outHeight) {
		memcpy (out, in, inWidth * inHeight * 4);
		return;
	}

	resampleBuffer = Mem_PoolAlloc (outWidth * outHeight * sizeof (uInt), MEMPOOL_IMAGESYS, 0);
	p1 = resampleBuffer;
	p2 = resampleBuffer + outWidth;

	// Resample
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

	Mem_Free (resampleBuffer);
}

/*
==============================================================================

	IMAGE UPLOADING

==============================================================================
*/

/*
===============
R_UploadImage
===============
*/
static void R_UploadImage (char *name, byte **data, int width, int height, int flags, int samples, int *upWidth, int *upHeight, int *upFormat)
{
	GLsizei		scaledWidth, scaledHeight;
	GLenum		tTarget, pTarget;
	qBool		mipMap;
	uInt		*scaled;
	GLint		format;
	int			numTextures;
	int			i, c;

	// Find next highest power of two
	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;

	// Mipmap
	mipMap = (flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST)) ? qFalse : qTrue;

	// Let people sample down the world textures for speed
	if (mipMap && !(flags & IF_NOPICMIP)) {
		if (gl_picmip->integer > 0) {
			scaledWidth >>= gl_picmip->integer;
			scaledHeight >>= gl_picmip->integer;
		}
	}

	// Clamp dimensions and set targets
	if (flags & IT_CUBEMAP) {
		numTextures = 6;

		tTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		pTarget = GL_TEXTURE_CUBE_MAP_ARB;

		scaledWidth = clamp (scaledWidth, 1, glConfig.maxCMTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxCMTexSize);
	}
	else {
		numTextures = 1;

		tTarget = GL_TEXTURE_2D;
		pTarget = GL_TEXTURE_2D;

		scaledWidth = clamp (scaledWidth, 1, glConfig.maxTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxTexSize);
	}

	// Scan and replace channels if desired
	if (flags & (IF_NORGB|IF_NOALPHA)) {
		byte	*scan;

		if (flags & IF_NORGB) {
			for (i=0 ; i<numTextures ; i++) {
				scan = (byte *)data[i];
				for (c=width*height ; c>0 ; c--, scan += 4)
					scan[0] = scan[1] = scan[2] = 255;
			}
		}
		else if (samples == 4) {
			for (i=0 ; i<numTextures ; i++) {
				scan = (byte *)data[i] + 3;
				for (c=width*height ; c>0 ; c--, scan += 4)
					*scan = 255;
			}
			samples = 3;
		}
	}

	// Determine transparency
	switch (samples) {
	case 3:
		if (glConfig.extArbTexCompression && !(flags & IF_NOCOMPRESS))
			format = GL_COMPRESSED_RGB_ARB;
		else
			format = glState.texRGBFormat;
		break;

	default:
		Com_Printf (0, S_COLOR_YELLOW "WARNING: R_UploadImage: Invalid image sample count '%d' on '%s', assuming '4'\n", samples, name);
		samples = 4;
	case 4:
		if (glConfig.extArbTexCompression && !(flags & IF_NOCOMPRESS))
			format = GL_COMPRESSED_RGBA_ARB;
		else
			format = glState.texRGBAFormat;
		break;
	}

	// Set base upload values
	if (upWidth)
		*upWidth = scaledWidth;
	if (upHeight)
		*upHeight = scaledHeight;
	if (upFormat)
		*upFormat = format;

	// Texture params
	if (mipMap) {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (pTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (gl_ext_max_anisotropy->integer, 1, glConfig.maxAniso));

		qglTexParameteri (pTarget, GL_TEXTURE_MIN_FILTER, glState.texMinFilter);
		qglTexParameteri (pTarget, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
	else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (pTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

		if (flags & IF_NOMIPMAP_LINEAR) {
			qglTexParameteri (pTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri (pTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			qglTexParameteri (pTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri (pTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	// Texture edge clamping
	if (!(flags & (IT_CUBEMAP|IF_CLAMP))) {
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else if (glConfig.extTexEdgeClamp) {
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (flags & IT_CUBEMAP)
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	else {
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
		if (flags & IT_CUBEMAP)
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
	}

	// Allocate a buffer
	scaled = Mem_PoolAlloc (scaledWidth * scaledHeight * 4, MEMPOOL_IMAGESYS, 0);
	if (!scaled)
		Com_Error (ERR_FATAL, "R_UploadImage: Out of memory!\n");

	// Upload
	for (i=0 ; i<numTextures ; i++) {
		// Resample
		R_ResampleImage ((uInt *)(data[i]), width, height, scaled, scaledWidth, scaledHeight);

		// Apply image gamma/intensity
		R_LightScaleImage (scaled, scaledWidth, scaledHeight, samples, (!(flags & IF_NOGAMMA)) && !glConfig.hwGamma, mipMap && !(flags & IF_NOINTENS));

		// Upload the base image
		qglTexImage2D (tTarget+i, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

		// Upload mipmap levels
		if (mipMap && !glConfig.extSGISGenMipmap) {
			int		mipWidth, mipHeight;
			int		mipLevel = 0;

			mipWidth = scaledWidth;
			mipHeight = scaledHeight;
			while (mipWidth > 1 || mipHeight > 1) {
				R_MipmapImage ((byte *)scaled, mipWidth, mipHeight);

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;

				mipLevel++;
				qglTexImage2D (tTarget+i, mipLevel, format, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}
	}

	// Done
	if (scaled) {
		Mem_Free (scaled);
		scaled = NULL;
	}
}


/*
===============
R_PalToRGBA

Converts a paletted image to standard rgba before passing off for upload
===============
*/
typedef struct
{
	int		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillColor) { \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

static void R_FloodFillSkin (byte *skin, int skinWidth, int skinHeight)
{
	byte		fillColor;
	floodfill_t	fifo[FLOODFILL_FIFO_SIZE];
	int			inpt = 0, outpt = 0;
	int			filledColor;
	int			i;

	// Assume this is the pixel to fill
	fillColor = *skin;
	filledColor = -1;

	if (filledColor == -1) {
		filledColor = 0;
		// Attempt to find opaque black
		for (i=0 ; i<256 ; ++i) {
			if (r_paletteTable[i] == (255 << 0)) {
				// Alpha 1.0
				filledColor = i;
				break;
			}
		}
	}

	// Can't fill to filled color or to transparent color (used as visited marker)
	if (fillColor == filledColor || fillColor == 255)
		return;

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt) {
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledColor;
		byte		*pos = &skin[x + skinWidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)
			FLOODFILL_STEP (-1, -1, 0);

		if (x < skinWidth-1)
			FLOODFILL_STEP (1, 1, 0);

		if (y > 0)
			FLOODFILL_STEP (-skinWidth, 0, -1);

		if (y < skinHeight-1)
			FLOODFILL_STEP (skinWidth, 0, 1);
		skin[x + skinWidth * y] = fdc;
	}
}
static void R_PalToRGBA (char *name, byte *data, int width, int height, int flags, image_t *image, qBool isPCX)
{
	uInt	*trans;
	int		i, s, pxl;
	int		samples;

	s = width * height;
	trans = Mem_PoolAlloc (s * 4, MEMPOOL_IMAGESYS, 0);

	// Map the palette to standard RGB
	samples = 3;
	for (i=0 ; i<s ; i++) {
		pxl = data[i];
		trans[i] = r_paletteTable[pxl];

		if (pxl == 0xff) {
			samples = 4;

			// Transparent, so scan around for another color to avoid alpha fringes
			if (i > width && data[i-width] != 255)
				pxl = data[i-width];
			else if (i < s-width && data[i+width] != 255)
				pxl = data[i+width];
			else if (i > 0 && data[i-1] != 255)
				pxl = data[i-1];
			else if (i < s-1 && data[i+1] != 255)
				pxl = data[i+1];
			else
				pxl = 0;

			// Copy rgb components
			((byte *)&trans[i])[0] = ((byte *)&r_paletteTable[pxl])[0];
			((byte *)&trans[i])[1] = ((byte *)&r_paletteTable[pxl])[1];
			((byte *)&trans[i])[2] = ((byte *)&r_paletteTable[pxl])[2];
		}
	}

	// Upload
	if (isPCX)
		R_FloodFillSkin ((byte *)trans, width, height);

	R_UploadImage (name, (byte **)&trans, width, height, flags, samples, &image->upWidth, &image->upHeight, &image->format);
	Mem_Free (trans);
}

/*
==============================================================================

	IMAGE LOADING

==============================================================================
*/

/*
================
R_FindImage
================
*/
static image_t *R_FindImage (const char *name, int flags, float bumpScale)
{
	image_t	*image;
	uLong	hash;
	int		len;

	// Check the length
	len = strlen (name);
	if (len < 5) {
		Com_Printf (0, S_COLOR_RED "R_FindImage: Image name too short! %s\n", name);
		return NULL;
	}
	if (len >= MAX_QPATH) {
		Com_Printf (0, S_COLOR_RED "R_FindImage: Image name too long! %s\n", name);
		return NULL;
	}

	// Fix/strip barename
	Q_strncpyz (r_bareImageName, name, sizeof (r_bareImageName));
	Q_FixPathName (r_bareImageName, sizeof (r_bareImageName), 3, qTrue);

	// Calculate hash
	hash = CalcHash (r_bareImageName, MAX_IMAGE_HASH);

	// Look for it
	if (flags == 0) {
		for (image=r_imageHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->touchFrame)
				continue;

			// Check name
			if (!strcmp (r_bareImageName, image->bareName))
				return image;
		}
	}
	else if (flags & IT_HEIGHTMAP) {
		for (image=r_imageHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->touchFrame)
				continue;
			if (image->flags != flags)
				continue;
			if (image->bumpScale != bumpScale)
				continue;

			// Check name
			if (!strcmp (r_bareImageName, image->bareName))
				return image;
		}
	}
	else {
		for (image=r_imageHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->touchFrame)
				continue;
			if (image->flags != flags)
				continue;

			// Check name
			if (!strcmp (r_bareImageName, image->bareName))
				return image;
		}
	}

	return NULL;
}


/*
================
R_LoadImage

This is also used as an entry point for the generated r_noTexture
================
*/
static image_t *R_LoadImage (char *name, byte **pic, int width, int height, int flags, int samples, qBool upload8, qBool isPCX)
{
	image_t		*image;
	uInt		i;

	// Find a free r_imageList spot
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (image->touchFrame)
			continue;

		image->texNum = i + 1;
		break;
	}

	// None found, create a new spot
	if (i == r_numImages) {
		if (r_numImages >= MAX_IMAGES)
			Com_Error (ERR_DROP, "R_LoadImage: MAX_IMAGES");

		image = &r_imageList[r_numImages++];
		image->texNum = r_numImages;
	}

	// Check the name
	if (strlen (name) >= sizeof (image->name))
		Com_Error (ERR_DROP, "R_LoadImage: \"%s\" is too long", name);

	Q_strncpyz (image->name, name, sizeof (image->name));
	if (r_bareImageName[0]) {
		Q_strncpyz (image->bareName, r_bareImageName, sizeof (image->bareName));
	}
	else {
		Q_strncpyz (image->bareName, name, sizeof (image->bareName));
		Q_FixPathName (image->bareName, sizeof (image->bareName), 3, qTrue);
	}
	memset (&r_bareImageName, 0, sizeof (r_bareImageName));

	// Set width and height
	image->width = image->tcWidth = width;
	image->height = image->tcHeight = height;

	// Texture scaling, hacky special case!
	if (!upload8) {
		char		newName[MAX_QPATH];
		walTex_t	*mt;

		Q_snprintfz (newName, sizeof (newName), "%s.wal", image->bareName);
		FS_LoadFile (newName, (void **)&mt);
		if (mt) {
			image->tcWidth = LittleLong (mt->width);
			image->tcHeight = LittleLong (mt->height);

			FS_FreeFile (mt);
		}
	}

	// Set base values
	image->flags = flags;
	image->touchFrame = r_registrationFrame;
	image->hashValue = CalcHash (image->bareName, MAX_IMAGE_HASH);

	if (image->flags & IT_CUBEMAP)
		image->target = GL_TEXTURE_CUBE_MAP_ARB;
	else
		image->target = GL_TEXTURE_2D;

	// Upload
	GL_BindTexture (image);
	if (upload8)
		R_PalToRGBA (name, *pic, width, height, flags, image, isPCX);
	else
		R_UploadImage (name, pic, width, height, flags, samples, &image->upWidth, &image->upHeight, &image->format);

	image->finishedLoading = qTrue;

	// Link it in
	image->hashNext = r_imageHashTree[image->hashValue];
	r_imageHashTree[image->hashValue] = image;

	return image;
}


/*
===============
R_RegisterCubeMap

Finds or loads the given cubemap image
Static because R_RegisterImage uses it if it's passed the IT_CUBEMAP flag
===============
*/
static inline image_t *R_RegisterCubeMap (char *name, int flags)
{
	image_t		*image;
	int			i, j;
	int			samples;
	byte		*pic[6];
	int			width, height;
	int			firstSize, firstSamples;
	char		loadName[MAX_QPATH];

	// Make sure we have this
	flags |= (IT_CUBEMAP|IF_CLAMP);

	// See if it's already loaded
	image = R_FindImage (name, flags, 0);
	if (image) {
		image->touchFrame = r_registrationFrame;
		return image;
	}

	// Not found -- load the pic from disk
	for (i=0 ; i<6 ; i++) {
		pic[i] = NULL;

		// PNG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.png", r_bareImageName, cubeMap_NameSuffix[i]);
		samples = R_LoadPNG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// TGA
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.tga", r_bareImageName, cubeMap_NameSuffix[i]);
		samples = R_LoadTGA (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// JPG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.jpg", r_bareImageName, cubeMap_NameSuffix[i]);
		samples = R_LoadJPG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// Not found
		break;

checkPic:
		if (width != height) {
			Com_Printf (0, S_COLOR_YELLOW "R_RegisterCubeMap: %s is not square!\n", loadName);
			goto emptyTrash;
		}

		if (!i) {
			firstSize = width;
			firstSamples = samples;
		}
		else {
			if (firstSize != width) {
				Com_Printf (0, S_COLOR_YELLOW "R_RegisterCubeMap: Size mismatch with previous in %s! Halting!\n", loadName);
				break;
			}

			if (firstSamples != samples) {
				Com_Printf (0, S_COLOR_YELLOW "R_RegisterCubeMap: Sample mismatch with previous in %s! Halting!\n", loadName);
				break;
			}
		}
	}

	if (i != 6) {
		Com_Printf (0, S_COLOR_YELLOW "R_RegisterCubeMap: Unable to find all of the sides, aborting!\n");
		goto emptyTrash;
	}
	else
		image = R_LoadImage (loadName, pic, width, height, flags, samples, qFalse, qFalse);

emptyTrash:
	for (j=0 ; j<6 ; j++) {
		if (pic[j])
			Mem_Free (pic[j]);
	}

	return image;
}


/*
===============
R_RegisterImage

Finds or loads the given image
===============
*/
image_t	*R_RegisterImage (char *name, int flags, float bumpScale)
{
	image_t		*image;
	byte		*pic;
	int			width, height, samples;
	char		loadName[MAX_QPATH];
	qBool		upload8, isPCX;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// Cubemap stuff
	if (flags & IT_CUBEMAP && glConfig.extArbTexCubeMap)
		return R_RegisterCubeMap (name, flags);
	flags &= ~IT_CUBEMAP;

	// See if it's already loaded
	image = R_FindImage (name, flags, bumpScale);
	if (image) {
		image->touchFrame = r_registrationFrame;
		return image;
	}

	// Not found -- load the pic from disk
	upload8 = qFalse;
	isPCX = qFalse;

	// PNG
	Q_snprintfz (loadName, sizeof (loadName), "%s.png", r_bareImageName);
	samples = R_LoadPNG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// TGA
	Q_snprintfz (loadName, sizeof (loadName), "%s.tga", r_bareImageName);
	samples = R_LoadTGA (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// JPG
	Q_snprintfz (loadName, sizeof (loadName), "%s.jpg", r_bareImageName);
	samples = R_LoadJPG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	pic = NULL;
	upload8 = qTrue;
	samples = 3;
	if (!(strcmp (name + strlen (name) - 4, ".wal"))) {
		// WAL
		Q_snprintfz (loadName, sizeof (loadName), "%s.wal", r_bareImageName);
		R_LoadWal (loadName, &pic, &width, &height);
		if (pic)
			goto loadImage;
	}
	else {
		// PCX
		Q_snprintfz (loadName, sizeof (loadName), "%s.pcx", r_bareImageName);
		R_LoadPCX (loadName, &pic, NULL, &width, &height);
		if (pic) {
			isPCX = qTrue;
			goto loadImage;
		}
	}

	// Nothing found!
	image = NULL;
	goto emptyTrash;

loadImage:
	if (flags & IT_HEIGHTMAP) {
		byte	*temp;

		temp = Mem_PoolAlloc (width * height * 4, MEMPOOL_IMAGESYS, 0);
		R_HeightmapToNormalmap (temp, temp, width, height, bumpScale);
		Mem_Free (pic);
		pic = temp;
	}

	image = R_LoadImage (loadName, &pic, width, height, flags, samples, upload8, isPCX);

emptyTrash:
	if (pic)
		Mem_Free (pic);

	return image;
}


/*
================
R_FreeImage
================
*/
static void R_FreeImage (image_t *image)
{
	image_t	*hashImg;
	image_t	**prev;

	// De-link it from the hash tree
	prev = &r_imageHashTree[image->hashValue];
	for ( ; ; ) {
		hashImg = *prev;
		if (!hashImg)
			break;

		if (hashImg == image) {
			*prev = hashImg->hashNext;
			break;
		}
		prev = &hashImg->hashNext;
	}

	// Free it
	qglDeleteTextures (1, &image->texNum);
	memset (image, 0, sizeof (image_t));
}


/*
================
R_BeginImageRegistration

Releases images that were not fully loaded during the last registration sequence
================
*/
void R_BeginImageRegistration (void)
{
	image_t	*image;
	uInt	i;

	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;		// free image_t slot
		if (image->finishedLoading)
			continue;

		R_FreeImage (image);
	}
}


/*
================
R_EndImageRegistration

Any image that was not touched on this registration sequence will be freed
================
*/
void R_EndImageRegistration (void)
{
	image_t	*image;
	uInt	i, startTime;
	uInt	freed;

	freed = 0;
	startTime = Sys_Milliseconds ();
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (image->touchFrame == r_registrationFrame)
			continue;		// Used this sequence
		if (image->flags & IF_NOFLUSH) {
			image->touchFrame = r_registrationFrame;
			continue;		// Don't free
		}

		R_FreeImage (image);
		freed++;
	}
	Com_Printf (0, "%i untouched images free'd in %.2fs\n", freed, (Sys_Milliseconds () - startTime)*0.001f);
}


/*
===============
R_GetImageSize
===============
*/
void R_GetImageSize (shader_t *shader, int *width, int *height)
{
	if (!shader || !shader->passes || !shader->passes->animFrames[0]) {
		if (width)
			*width = 0;
		if (height)
			*height = 0;
	}
	else {
		if (shader->sizeBase >= 0 && shader->passes[shader->sizeBase].animFrames[0]) {
			if (width)
				*width = shader->passes[shader->sizeBase].animFrames[0]->width;
			if (height)
				*height = shader->passes[shader->sizeBase].animFrames[0]->height;
		}
		else {
			if (width)
				*width = shader->passes->animFrames[0]->width;
			if (height)
				*height = shader->passes->animFrames[0]->height;
		}
	}
}

/*
==============================================================================

	CONSOLE FUNCTIONS

==============================================================================
*/

/*
===============
R_ImageCheck_f

Slow dev function that checks if any textures are loaded into memory twice
Finds are printed to the console
===============
*/
static void R_ImageCheck_f (void)
{
	image_t		*a, *b;
	uInt		i, j;
	uInt		total;

	// Duplicates
	total = 0;
	for (i=0, a=r_imageList ; i<r_numImages && a ; i++, a++) {
		if (!a->touchFrame)
			continue;		// Free r_imageList slot

		for (j=0, b=r_imageList ; j<r_numImages && b; j++, b++) {
			if (!b->touchFrame)
				continue;		// Free r_imageList slot

			if (!Q_stricmp (a->bareName, b->bareName) && i != j) {
				Com_Printf (0, "%3i %s & %3i %s\n", i, a->bareName, j, b->bareName);
				if (a->flags != b->flags)
					Com_Printf (0, "flags mismatch: %i != %i\n", a->flags, b->flags);
				total++;
			}
		}
	}

	Com_Printf (0, "Total image duplicates: %i\n", total);
}


/*
===============
R_ImageList_f
===============
*/
static void R_ImageList_f (void)
{
	uInt		tempWidth, tempHeight;
	uInt		i, totalImages, totalMips;
	uInt		mipTexels, texels;
	image_t		*image;

	Com_Printf (0, "------------------------------------------------------\n");
	Com_Printf (0, "Tex# Ta Format GIFC Width Height Name\n");
	Com_Printf (0, "---- -- ------ ---- ----- ------ ---------------------\n");

	for (i=0, totalImages=0, totalMips=0, mipTexels=0, texels=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;	// Free r_imageList slot

		// Texnum
		Com_Printf (0, "%4d ", image->texNum);

		// Target
		switch (image->target) {
		case GL_TEXTURE_CUBE_MAP_ARB:	Com_Printf (0, "CM ");			break;
		case GL_TEXTURE_1D:				Com_Printf (0, "1D ");			break;
		case GL_TEXTURE_2D:				Com_Printf (0, "2D ");			break;
		case GL_TEXTURE_3D:				Com_Printf (0, "3D ");			break;
		}

		// Format
		switch (image->format) {
		case GL_RGBA8:					Com_Printf (0, "RGBA8  ");		break;
		case GL_RGBA4:					Com_Printf (0, "RGBA4  ");		break;
		case GL_RGBA:					Com_Printf (0, "RGBA   ");		break;
		case GL_RGB8:					Com_Printf (0, "RGB8   ");		break;
		case GL_RGB5:					Com_Printf (0, "RGB5   ");		break;
		case GL_RGB:					Com_Printf (0, "RGB    ");		break;

		case GL_DSDT8_NV:				Com_Printf (0, "DSDT8  ");		break;

		case GL_COMPRESSED_RGBA_ARB:	Com_Printf (0, "CMRGBA ");		break;
		case GL_COMPRESSED_RGB_ARB:		Com_Printf (0, "CMRGB  ");		break;

		default:						Com_Printf (0, "?????? ");		break;
		}

		// Flags
		Com_Printf (0, "%s", (image->flags & IF_NOGAMMA)	? "-" : "G");
		Com_Printf (0, "%s", (image->flags & IF_NOINTENS)	? "-" : "I");
		Com_Printf (0, "%s", (image->flags & IF_NOFLUSH)	? "F" : "-");
		Com_Printf (0, "%s", (image->flags & IF_CLAMP)		? "C" : "-");

		// Width/height name
		Com_Printf (0, " %5i  %5i %s\n", image->upWidth, image->upHeight, image->name);

		// Increment counters
		totalImages++;
		texels += image->upWidth * image->upHeight;
		if (!(image->flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST))) {
			tempWidth=image->upWidth, tempHeight=image->upHeight;
			while (tempWidth > 1 || tempHeight > 1) {
				tempWidth >>= 1;
				if (tempWidth < 1)
					tempWidth = 1;

				tempHeight >>= 1;
				if (tempHeight < 1)
					tempHeight = 1;

				mipTexels += tempWidth * tempHeight;
				totalMips++;
			}
		}
	}

	Com_Printf (0, "------------------------------------------------------\n");
	Com_Printf (0, "Total images: %d (with mips: %d)\n", totalImages, totalImages+totalMips);
	Com_Printf (0, "Texel count: %d (w/o mips: %d)\n", texels+mipTexels, texels);
	Com_Printf (0, "------------------------------------------------------\n");
}


/*
================== 
R_ScreenShot_f
================== 
*/
static void R_ScreenShot_f (void)
{
	char	checkName[MAX_OSPATH];
	int		type, shotNum, quality;
	char	*ext;
	byte	*buffer;
	FILE	*f;

	// Find out what format to save in
	if (Cmd_Argc () > 1) {
		if (!Q_stricmp (Cmd_Argv (1), "tga"))
			type = 0;
		else if (!Q_stricmp (Cmd_Argv (1), "png"))
			type = 1;
		else if (!Q_stricmp (Cmd_Argv (1), "jpg"))
			type = 2;
	}
	else {
		if (!Q_stricmp (gl_screenshot->string, "png"))
			type = 1;
		else if (!Q_stricmp (gl_screenshot->string, "jpg"))
			type = 2;
		else
			type = 0;
	}

	// Set necessary values
	switch (type) {
	case 0:
		Com_Printf (0, "Taking TGA screenshot...\n");
		quality = 100;
		ext = "tga";
		break;

	case 1:
		Com_Printf (0, "Taking PNG screenshot...\n");
		quality = 100;
		ext = "png";
		break;

	case 2:
		if (Cmd_Argc () == 3)
			quality = atoi (Cmd_Argv (2));
		else
			quality = gl_jpgquality->value;
		if (quality > 100 || quality <= 0)
			quality = 100;

		Com_Printf (0, "Taking JPG screenshot (at %i%% quality)...\n", quality);
		ext = "jpg";
		break;
	}

	// Create the scrnshots directory if it doesn't exist
	Q_snprintfz (checkName, sizeof (checkName), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (checkName);

	// Find a file name to save it to
	for (shotNum=0 ; shotNum<1000 ; shotNum++) {
		Q_snprintfz (checkName, sizeof (checkName), "%s/scrnshot/egl%.3d.%s", FS_Gamedir (), shotNum, ext);
		f = fopen (checkName, "rb");
		if (!f)
			break;
		fclose (f);
	}

	// Open it
	f = fopen (checkName, "wb");
	if (shotNum == 1000 || !f) {
		Com_Printf (0, S_COLOR_YELLOW "R_ScreenShot_f: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	// Allocate room for a copy of the framebuffer
	buffer = Mem_PoolAlloc (vidDef.width * vidDef.height * 3, MEMPOOL_IMAGESYS, 0);

	// Read the framebuffer into our storage
	qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	// Apply hardware gamma
	if (glConfig.hwGamma) {
		int		i, size;

		size = vidDef.width * vidDef.height * 3;

		for (i=0 ; i<size ; i+=3) {
			buffer[i+0] = glConfig.gammaRamp[buffer[i+0]] >> 8;
			buffer[i+1] = glConfig.gammaRamp[buffer[i+1] + 256] >> 8;
			buffer[i+2] = glConfig.gammaRamp[buffer[i+2] + 512] >> 8;
		}
	}

	// Write
	switch (type) {
	case 0:
		R_WriteTGA (f, buffer, vidDef.width, vidDef.height);
		break;

	case 1:
		R_WritePNG (f, buffer, vidDef.width, vidDef.height);
		break;

	case 2:
		R_WriteJPG (f, buffer, vidDef.width, vidDef.height, quality);
		break;
	}

	// Finish
	fclose (f);
	Mem_Free (buffer);

	Com_Printf (0, "Wrote egl%.3d.%s\n", shotNum, ext);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

static cmdFunc_t	*cmd_imageCheck;
static cmdFunc_t	*cmd_imageList;
static cmdFunc_t	*cmd_screenShot;

/*
==================
R_UpdateGammaRamp
==================
*/
void R_UpdateGammaRamp (void)
{
	int		i;
	byte	gam;

	vid_gamma->modified = qFalse;

	if (!glConfig.hwGamma)
		return;

	for (i=0 ; i<256 ; i++) {
		gam = (byte)(clamp (255 * pow (( (i & 255) + 0.5f)*0.0039138943248532289628180039138943, vid_gamma->value) + 0.5f, 0, 255));
		glConfig.gammaRamp[i] = glConfig.gammaRamp[i + 256] = glConfig.gammaRamp[i + 512] = gam * 255;
	}

	GLimp_SetGammaRamp (glConfig.gammaRamp);
}


/*
==================
R_InitSpecialTextures
==================
*/
static void R_InitSpecialTextures (void)
{
	int		x, y;
	byte	*data;

	/*
	** r_noTexture
	*/
	data = Mem_PoolAlloc (8 * 8 * 4, MEMPOOL_IMAGESYS, 0);
	for (x=0 ; x<8 ; x++) {
		for (y=0 ; y<8 ; y++) {
			data[(y*8 + x)*4+0] = clamp (x*16 + y*16, 0, 255);
			data[(y*8 + x)*4+1] = 0;
			data[(y*8 + x)*4+2] = 0;
			data[(y*8 + x)*4+3] = 255;
		}
	}

	memset (&r_noTexture, 0, sizeof (r_noTexture));
	r_noTexture = R_LoadImage ("***r_noTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3, qFalse, qFalse);

	/*
	** r_whiteTexture
	*/
	memset (data, 255, 8 * 8 * 4);
	memset (&r_whiteTexture, 0, sizeof (r_whiteTexture));
	r_whiteTexture = R_LoadImage ("***r_whiteTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3, qFalse, qFalse);

	/*
	** r_blackTexture
	*/
	memset (data, 0, 8 * 8 * 4);
	memset (&r_blackTexture, 0, sizeof (r_blackTexture));
	r_blackTexture = R_LoadImage ("***r_blackTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3, qFalse, qFalse);

	Mem_Free (data);

	/*
	** r_cinTexture
	** Reserve a texNum for cinematics
	*/
	data = Mem_PoolAlloc (256 * 256 * 4, MEMPOOL_IMAGESYS, 0);
	memset (data, 0, 256 * 256 * 4);
	memset (&r_cinTexture, 0, sizeof (r_cinTexture));
	r_cinTexture = R_LoadImage ("***r_cinTexture***", &data, 256, 256, IF_NOFLUSH|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_CLAMP|IF_NOINTENS|IF_NOGAMMA|IF_NOCOMPRESS, 3, qFalse, qFalse);
	Mem_Free (data);
}


/*
===============
R_ImageInit
===============
*/
void R_ImageInit (void)
{
	int		i, j;
	float	gam;
	int		red, green, blue;
	uInt	v;
	byte	*pal;

	Com_Printf (0, "\n--------- Image Initialization ---------\n");

	// Registration
	intensity			= Cvar_Get ("intensity",			"2",						CVAR_ARCHIVE);

	gl_jpgquality		= Cvar_Get ("gl_jpgquality",		"85",						CVAR_ARCHIVE);
	gl_nobind			= Cvar_Get ("gl_nobind",			"0",						0);
	gl_picmip			= Cvar_Get ("gl_picmip",			"0",						CVAR_LATCH_VIDEO);
	gl_screenshot		= Cvar_Get ("gl_screenshot",		"tga",						CVAR_ARCHIVE);

	gl_texturebits		= Cvar_Get ("gl_texturebits",		"default",					CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_texturemode		= Cvar_Get ("gl_texturemode",		"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE);

	vid_gamma			= Cvar_Get ("vid_gamma",			"1.0",						CVAR_ARCHIVE);

	cmd_imageCheck	= Cmd_AddCommand ("imagecheck",	R_ImageCheck_f,			"Image system integrity check");
	cmd_imageList	= Cmd_AddCommand ("imagelist",	R_ImageList_f,			"Prints out a list of the currently loaded textures");
	cmd_screenShot	= Cmd_AddCommand ("screenshot",	R_ScreenShot_f,			"Takes a screenshot");

	// Defaults
	r_numImages = 0;

	// Set the initial state
	glState.texUnit = 0;
	for (i=0 ; i<MAX_TEXUNITS ; i++) {
		glState.texEnvModes[i] = 0;
		glState.texNums[i] = 0;
	}

	GL_TextureMode (qTrue, qFalse);
	GL_TextureBits (qTrue, qFalse);

	GL_SelectTexture (0);
	GL_TextureEnv (GL_MODULATE);

	qglEnable (GL_TEXTURE_2D);
	if (glConfig.extArbTexCubeMap)
		qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glState.texMagFilter);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);

	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (glConfig.extArbTexCubeMap)
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	qglDisable (GL_TEXTURE_GEN_S);
	qglDisable (GL_TEXTURE_GEN_T);
	if (glConfig.extArbTexCubeMap)
		qglDisable (GL_TEXTURE_GEN_R);

	// Get the palette
	R_LoadPCX ("pics/colormap.pcx", NULL, &pal, NULL, NULL);
	if (!pal)
		pal = r_defaultImagePal;

	for (i=0 ; i<256 ; i++) {
		red = pal[i*3+0];
		green = pal[i*3+1];
		blue = pal[i*3+2];
		
		v = (255<<24) + (red<<0) + (green<<8) + (blue<<16);
		r_paletteTable[i] = LittleLong (v);
	}

	r_paletteTable[255] &= LittleLong (0xffffff);	// 255 is transparent

	if (pal != r_defaultImagePal)
		Mem_Free (pal);

	// Set up the gamma and intensity ramps
	if (intensity->value < 1)
		Cvar_VariableForceSetValue (intensity, 1);
	glState.invIntens = 1.0f / intensity->value;

	// Hack! because voodoo's are nasty bright
	if (glConfig.renderClass == REND_CLASS_VOODOO)
		gam = 1.0f;
	else
		gam = vid_gamma->value;

	// Gamma
	if (gam == 1) {
		for (i=0 ; i<256 ; i++)
			r_gammaTable[i] = i;
	}
	else {
		for (i=0 ; i<256 ; i++)
			r_gammaTable[i] = (byte)(clamp (255 * pow ((i + 0.5f)*0.0039138943248532289628180039138943f, gam) + 0.5f, 0, 255));
	}

	// Intensity (eww)
	for (i=0 ; i<256 ; i++) {
		j = i*intensity->integer;
		if (j > 255)
			j = 255;
		r_intensityTable[i] = j;
	}

	// Get gamma ramp
	glConfig.rampDownloaded = GLimp_GetGammaRamp (glConfig.originalRamp);
	if (glConfig.rampDownloaded)
		Com_Printf (0, "...GLimp_GetGammaRamp succeeded\n");
	else
		Com_Printf (0, S_COLOR_RED "...GLimp_GetGammaRamp failed!\n");

	// Use hardware gamma?
	glConfig.hwGamma = (glConfig.rampDownloaded && r_hwGamma->integer);
	if (glConfig.hwGamma) {
		Com_Printf (0, "...using hardware gamma\n");
		R_UpdateGammaRamp ();
	}
	else
		Com_Printf (0, "...using software gamma\n");

	// Load up special textures
	R_InitSpecialTextures ();

	// Check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_IMAGESYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
===============
R_ImageShutdown
===============
*/
void R_ImageShutdown (void)
{
	uInt	i;
	image_t	*image;

	// Replace gamma ramp
	if (glConfig.hwGamma)
		GLimp_SetGammaRamp (glConfig.originalRamp);

	// Unregister commands
	Cmd_RemoveCommand ("imagecheck", cmd_imageCheck);
	Cmd_RemoveCommand ("imagelist", cmd_imageList);
	Cmd_RemoveCommand ("screenshot", cmd_screenShot);

	// Free loaded textures
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;	// Free r_imageList slot

		// Free it
		qglDeleteTextures (1, &image->texNum);
	}

	// Clear everything
	r_numImages = 0;
	memset (&r_bareImageName, 0, sizeof (r_bareImageName));
	memset (r_imageList, 0, sizeof (r_imageList));
	memset (r_imageHashTree, 0, sizeof (r_imageHashTree));

	// Free memory
	Mem_FreePool (MEMPOOL_IMAGESYS);
}
