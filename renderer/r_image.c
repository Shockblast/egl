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

#ifdef WIN32
# include "../include/jpeg/jpeglib.h"
# include "../include/zlibpng/png.h"
#else
# include <jpeglib.h>
# include <png.h>
#endif

#define MAX_IMAGE_HASH	128

image_t		*r_lmTextures[R_MAX_LIGHTMAPS];
image_t		r_imageList[MAX_IMAGES];
image_t		*r_imageHashTree[MAX_IMAGE_HASH];
uint32		r_numImages;

static byte	r_intensityTable[256];
static byte	r_gammaTable[256];
uint32		r_paletteTable[256];

const char	*r_cubeMapSuffix[6] = { "px", "nx", "py", "ny", "pz", "nz" };
const char	*r_skyNameSuffix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

byte r_defaultImagePal[] = {
	#include "defpal.h"
};

image_t	*r_noTexture;		// use for bad textures
image_t	*r_whiteTexture;	// used in shaders/fallback
image_t	*r_blackTexture;	// used in shaders/fallback
image_t	*r_cinTexture;		// allocates memory on load as to not every cin frame
image_t	*r_dLightTexture;	// dynamic light texture for q3 bsp
image_t	*r_fogTexture;		// fog texture for q3 bsp

/*
==============================================================================

	TEXTURE STATE

==============================================================================
*/

/*
===============
GL_BindTexture
===============
*/
void GL_BindTexture (image_t *image)
{
	uint32		texNum;
	int			target;

	// Performance evaluation option
	if (gl_nobind->integer || !image)
		image = r_noTexture;

	// Find the texNum and target
	texNum = image->texNum;
	target = image->target;

	// Determine if it's already bound
	switch (target) {
	case GL_TEXTURE_1D:
		if (glState.tex1DBound[glState.texUnit] == texNum)
			return;
		glState.tex1DBound[glState.texUnit] = texNum;
		break;

	case GL_TEXTURE_2D:
		if (glState.tex2DBound[glState.texUnit] == texNum)
			return;
		glState.tex2DBound[glState.texUnit] = texNum;
		break;

	case GL_TEXTURE_3D:
		if (glState.tex3DBound[glState.texUnit] == texNum)
			return;
		glState.tex3DBound[glState.texUnit] = texNum;
		break;

	case GL_TEXTURE_CUBE_MAP_ARB:
		if (glState.texCMBound[glState.texUnit] == texNum)
			return;
		glState.texCMBound[glState.texUnit] = texNum;
		break;

#ifdef _DEBUG
	default:
		assert (0);
#endif // _DEBUG
	}

	qglBindTexture (target, texNum);

	// Performance counters
	r_refStats.textureBinds++;
	if (image->visFrame != r_refScene.frameCount) {
		image->visFrame = r_refScene.frameCount;
		r_refStats.texturesInUse++;
		r_refStats.texelsInUse += image->upWidth * image->upHeight;
	}
}


/*
===============
GL_SelectTexture
===============
*/
void GL_SelectTexture (texUnit_t texUnit)
{
	if (texUnit == glState.texUnit)
		return;
	if (!glConfig.extSGISMultiTexture && !glConfig.extArbMultitexture)
		return;
	if (texUnit > glConfig.maxTexUnits) {
		Com_Error (ERR_DROP, "Attempted selection of an out of bounds (%d) texture unit!", texUnit);
		return;
	}

	r_refStats.textureUnitChanges++;
	glState.texUnit = texUnit;
	if (qglSelectTextureSGIS) {
		qglSelectTextureSGIS (texUnit + GL_TEXTURE0_SGIS);
	}
	else if (qglActiveTextureARB) {
		qglActiveTextureARB (texUnit + GL_TEXTURE0_ARB);
		qglClientActiveTextureARB (texUnit + GL_TEXTURE0_ARB);
	}
}


/*
===============
GL_TextureEnv
===============
*/
void GL_TextureEnv (GLfloat mode)
{
	if (mode == GL_ADD && !glConfig.extTexEnvAdd)
		mode = GL_MODULATE;

	if (mode != glState.texEnvModes[glState.texUnit]) {
		qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		glState.texEnvModes[glState.texUnit] = mode;
		r_refStats.textureEnvChanges++;
	}
}


/*
===============
GL_LoadTexMatrix
===============
*/
void GL_LoadTexMatrix (mat4x4_t m)
{
	qglLoadMatrixd (m);
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
		switch (r_textureBits->integer) {
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
	switch (r_textureBits->integer) {
	case 32:
		glStatic.rgbFormat = GL_RGB8;
		glStatic.rgbaFormat = GL_RGBA8;
		break;

	case 16:
		glStatic.rgbFormat = GL_RGB5;
		glStatic.rgbaFormat = GL_RGBA4;
		break;

	default:
		glStatic.rgbFormat = GL_RGB;
		glStatic.rgbaFormat = GL_RGBA;
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
	uint32	i;
	image_t	*image;
	char	*string = gl_texturemode->string;

	// Find a matching mode
	for (i=0 ; i<NUM_GL_MODES ; i++) {
		if (!Q_stricmp (modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES) {
		// Not found
		Com_Printf (PRNT_WARNING, "bad filter name -- falling back to GL_LINEAR_MIPMAP_NEAREST\n");
		Cvar_VariableSet (gl_texturemode, "GL_LINEAR_MIPMAP_NEAREST", qTrue);
		
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
	uint32	i;
	image_t	*image;
	int		set;

	r_ext_maxAnisotropy->modified = qFalse;
	if (!glConfig.extTexFilterAniso)
		return;

	// Change all the existing mipmap texture objects
	set = clamp (r_ext_maxAnisotropy->integer, 1, glConfig.maxAniso);
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;		// Free r_imageList slot
		if (image->flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST))
			continue;		// Skip non-mipmapped imagery

		GL_BindTexture (image);
		qglTexParameteri (image->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, set);
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
    Com_DevPrintf (PRNT_WARNING, "R_LoadJPG: Premeture end of jpeg file\n");

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
	uint32	i;

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
		Com_DevPrintf (PRNT_WARNING, "R_LoadJPG: Bad jpeg samples '%s' (%d)\n", name, samples);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (cinfo.output_width <= 0 || cinfo.output_height <= 0) {
		Com_DevPrintf (PRNT_WARNING, "R_LoadJPG: Bad jpeg dimensions on '%s' (%d x %d)\n", name, cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (width)
		*width = cinfo.output_width;
	if (height)
		*height = cinfo.output_height;

	img = Mem_PoolAllocExt (cinfo.output_width * cinfo.output_height * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	dummy = Mem_PoolAllocExt (cinfo.output_width * samples, qFalse, MEMPOOL_IMAGESYS, 0);

	if (pic)
		*pic = img;

	while (cinfo.output_scanline < cinfo.output_height) {
		scan = dummy;
		if (!jpeg_read_scanlines (&cinfo, &scan, 1)) {
			Com_Printf (PRNT_WARNING, "Bad jpeg file %s\n", name);
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
	jpeg_write_marker (&cinfo, JPEG_COM, (byte *) "EGL v" EGL_VERSTR, (uint32) strlen ("EGL v" EGL_VERSTR));

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

	uint16		xMin, yMin, xMax, yMax;
	uint16		hRes, vRes;
	byte		palette[48];

	char		reserved;
	char		colorPlanes;

	uint16		bytesPerLine;
	uint16		paletteType;

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
		Com_DevPrintf (PRNT_WARNING, "R_LoadPCX: Invalid PCX header: %s\n", filename);
		return;
	}

	if (pcx->bitsPerPixel != 8 || pcx->colorPlanes != 1) {
		Com_DevPrintf (PRNT_WARNING, "R_LoadPCX: Only 8-bit PCX images are supported: %s\n", filename);
		return;
	}

	if (pcx->xMax >= 640 || pcx->xMax <= 0 || pcx->yMax >= 480 || pcx->yMax <= 0) {
		Com_DevPrintf (PRNT_WARNING, "R_LoadPCX: Bad PCX file dimensions: %s: %i x %i\n", filename, pcx->xMax, pcx->yMax);
		return;
	}

	// Some images with weird dimensions will crash if I don't do this...
	x = max (pcx->yMax+1, pcx->xMax+1);
	out = Mem_PoolAllocExt (x * x, qFalse, MEMPOOL_IMAGESYS, 0);
	if (pic)
		*pic = out;
	pix = out;

	if (palette) {
		*palette = Mem_PoolAllocExt (768, qFalse, MEMPOOL_IMAGESYS, 0);
		memcpy (*palette, (byte *)pcx + fileLen - 768, 768);
	}

	if (width)
		*width = pcx->xMax+1;
	if (height)
		*height = pcx->yMax+1;

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
		Com_DevPrintf (PRNT_WARNING, "R_LoadPCX: PCX file %s was malformed", filename);
		Mem_Free (out);
		out = NULL;
		pix = NULL;

		if (pic)
			*pic = NULL;
		if (palette) {
			Mem_Free (palette);
			*palette = NULL;
		}
	}

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
	uint32			i;

	pngBuf_t	PngFileBuffer = {NULL,0};

	if (pic)
		*pic = NULL;

	// Load the file
	FS_LoadFile (name, (void **)&PngFileBuffer.buffer);
	if (!PngFileBuffer.buffer)
		return 0;

	// Parse the PNG file
	if ((png_check_sig (PngFileBuffer.buffer, 8)) == 0) {
		Com_Printf (PRNT_WARNING, "R_LoadPNG: Not a PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	PngFileBuffer.pos = 0;

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,  NULL, NULL);
	if (!png_ptr) {
		Com_Printf (PRNT_WARNING, "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		Com_Printf (PRNT_WARNING, "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}
	
	end_info = png_create_info_struct (png_ptr);
	if (!end_info) {
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
		Com_Printf (PRNT_WARNING, "R_LoadPNG: Bad PNG file: %s\n", name);
		FS_FreeFile (PngFileBuffer.buffer);
		return 0;
	}

	png_set_read_fn (png_ptr, (png_voidp)&PngFileBuffer, (png_rw_ptr)PngReadFunc);

#ifdef OLDPNGCODE
	png_read_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	row_pointers = png_get_rows (png_ptr, info_ptr);

	rowptr = 0;

	img = Mem_PoolAllocExt (info_ptr->width * info_ptr->height * 4, qFalse, MEMPOOL_IMAGESYS, 0);
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
		uint32	j;

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

	pic_ptr = Mem_PoolAllocExt (info_ptr->height * rowbytes, qFalse, MEMPOOL_IMAGESYS, 0);
	if (pic)
		*pic = pic_ptr;

	row_pointers = Mem_PoolAllocExt (sizeof (png_bytep) * info_ptr->height, qFalse, MEMPOOL_IMAGESYS, 0);

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
		Com_Printf (PRNT_WARNING, "R_WritePNG: LibPNG Error!\n");
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
		Com_Printf (PRNT_WARNING, "R_WritePNG: LibPNG Error!\n");
		return;
	}

	png_init_io (png_ptr, f);

	png_set_IHDR (png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_set_compression_level (png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_compression_mem_level (png_ptr, 9);

	png_write_info (png_ptr, info_ptr);

	row_pointers = Mem_PoolAllocExt (height * sizeof (png_bytep), qFalse, MEMPOOL_IMAGESYS, 0);
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

	uint16	colorMapIndex;
	uint16	colorMapLength;
	byte	colorMapSize;

	uint16	xOrigin;
	uint16	yOrigin;
	uint16	width;
	uint16	height;

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
	targaHeader.xOrigin = LittleShort (*((int16 *)buf_p)); buf_p+=2;
	targaHeader.yOrigin = LittleShort (*((int16 *)buf_p)); buf_p+=2;
	targaHeader.width = LittleShort (*((int16 *)buf_p)); buf_p+=2;
	targaHeader.height = LittleShort (*((int16 *)buf_p)); buf_p+=2;
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
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: Only 8 bit images supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targaHeader.colorMapLength != 256) {
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: Only 8 bit colormaps are supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targaHeader.colorMapIndex) {
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: colorMapIndex is not supported for type 1 and 9\n");
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
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: only 24 and 32 bit colormaps are supported for type 1 and 9\n");
			FS_FreeFile (buffer);
			return 0;
		}
		break;

	case 10:
		compressed = qTrue;
	case 2:
		// Uncompressed or RLE compressed RGB
		if (targaHeader.pixelSize != 32 && targaHeader.pixelSize != 24) {
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: Only 32 or 24 bit images supported for type 2 and 10\n");
			FS_FreeFile (buffer);
			return 0;
		}
		break;

	case 11:
		compressed = qTrue;
	case 3:
		// Uncompressed greyscale
		if (targaHeader.pixelSize != 8) {
			Com_DevPrintf (PRNT_WARNING, "R_LoadTGA: Only 8 bit images supported for type 3 and 11");
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

	targaRGBA = Mem_PoolAllocExt (columns * rows * 4, qFalse, MEMPOOL_IMAGESYS, 0);
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
					if (targaHeader.pixelSize == 32) {
						alpha = *buf_p++;
						if (alpha != 255)
							samples = 4;
					}
					else
						alpha = 255;
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
static void R_WriteTGA (FILE *f, byte *buffer, int width, int height, qBool rgb)
{
	int		i, size, temp;
	byte	*out;

	// Allocate an output buffer
	size = (width * height * 3) + 18;
	out = Mem_PoolAllocExt (size, qTrue, MEMPOOL_IMAGESYS, 0);

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
	if (rgb) {
		for (i=18 ; i<size ; i+=3) {
			temp = out[i];
			out[i] = out[i+2];
			out[i+2] = temp;
		}
	}

	fwrite (out, 1, size, f);

	Mem_Free (out);
}

/*
==============================================================================

	WAL

==============================================================================
*/

#define MIPLEVELS	4
typedef struct walTex_s {
	char		name[32];
	uint32		width;
	uint32		height;
	uint32		offsets[MIPLEVELS];		// four mip maps stored
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
		Com_DevPrintf (0, "R_LoadWal: bad WAL file 's' (%i x %i)\n", name, mt->width, mt->height);
		FS_FreeFile ((void *)buffer);
		return;
	}

	if (width)
		*width = mt->width;
	if (height)
		*height = mt->height;

	*pic = Mem_PoolAllocExt (mt->width * mt->height * 3, qFalse, MEMPOOL_IMAGESYS, 0);
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
R_LightScaleImage

Scale up the pixel values in a texture to increase the lighting range
================
*/
static void R_LightScaleImage (uint32 *in, int inWidth, int inHeight, qBool useGamma, qBool useIntensity)
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
	else if (useIntensity) {
		for (i=0 ; i<c ; i++, out+=4) {
			out[0] = r_intensityTable[out[0]];
			out[1] = r_intensityTable[out[1]];
			out[2] = r_intensityTable[out[2]];
		}
	}
}


/*
================
R_MipmapImage

Operates in place, quartering the size of the texture
================
*/
static void R_MipmapImage (byte *in, int inWidth, int inHeight)
{
	int		i, j;
	byte	*out;

	inWidth <<= 2;
	inHeight >>= 1;

	out = in;
	for (i=0 ; i<inHeight ; i++, in+=inWidth) {
		for (j=0 ; j<inWidth ; j+=8, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[inWidth+0] + in[inWidth+4])>>2;
			out[1] = (in[1] + in[5] + in[inWidth+1] + in[inWidth+5])>>2;
			out[2] = (in[2] + in[6] + in[inWidth+2] + in[inWidth+6])>>2;
			out[3] = (in[3] + in[7] + in[inWidth+3] + in[inWidth+7])>>2;
		}
	}
}


/*
================
R_ResampleImage
================
*/
static void R_ResampleImage (uint32 *in, int inWidth, int inHeight, uint32 *out, int outWidth, int outHeight)
{
	int		i, j;
	uint32	*inrow, *inrow2;
	uint32	frac, fracstep;
	uint32	*p1, *p2;
	byte	*pix1, *pix2, *pix3, *pix4;
	uint32	*resampleBuffer;

	if (inWidth == outWidth && inHeight == outHeight) {
		memcpy (out, in, inWidth * inHeight * 4);
		return;
	}

	resampleBuffer = Mem_PoolAllocExt (outWidth * outHeight * sizeof (uint32), qFalse, MEMPOOL_IMAGESYS, 0);
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
		inrow = in + inWidth * (int)((i + 0.25f) * inHeight / outHeight);
		inrow2 = in + inWidth * (int)((i + 0.75f) * inHeight / outHeight);
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

	if (r_refRegInfo.inSequence)
		r_refRegInfo.imagesResampled++;
}

/*
==============================================================================

	IMAGE UPLOADING

==============================================================================
*/

/*
===============
R_ImageFormat
===============
*/
static inline int R_ImageFormat (char *name, int flags, int *samples)
{
	GLint		format;

	if (flags & IF_NOALPHA && *samples == 4)
		*samples = 3;

	switch (*samples) {
	case 3:
		if (!glConfig.extTexCompression || flags & IF_NOCOMPRESS)
			format = glStatic.rgbFormat;
		else
			format = glStatic.rgbFormatCompressed;
		break;

	default:
		Com_Printf (PRNT_WARNING, "WARNING: Invalid image sample count '%d' on '%s', assuming '4'\n", samples, name);
		*samples = 4;
	case 4:
		if (!glConfig.extTexCompression || flags & IF_NOCOMPRESS)
			format = glStatic.rgbaFormat;
		else
			format = glStatic.rgbaFormatCompressed;
		break;
	}

	return format;
}


/*
===============
R_ColorMipLevel
===============
*/
static void R_ColorMipLevel (byte *image, int size, int level)
{
	int		i;

	if (level == 0)
		return;

	switch ((level+2) % 3) {
	case 0:
		for (i=0 ; i<size ; i++) {
			image[0] = 255;
			image[1] *= 0.5f;
			image[2] *= 0.5f;

			image += 4;
		}
		break;

	case 1:
		for (i=0 ; i<size ; i++) {
			image[0] *= 0.5f;
			image[1] = 255;
			image[2] *= 0.5f;

			image += 4;
		}
		break;

	case 2:
		for (i=0 ; i<size ; i++) {
			image[0] *= 0.5f;
			image[1] *= 0.5f;
			image[2] = 255;

			image += 4;
		}
		break;
	}
}


/*
===============
R_UploadCMImage
===============
*/
static void R_UploadCMImage (char *name, byte **data, int width, int height, int flags, int samples, int *upWidth, int *upHeight, int *upFormat)
{
	GLsizei		scaledWidth, scaledHeight;
	qBool		mipMap;
	uint32		*scaled;
	GLint		format;
	int			i, c;

	// Find next highest power of two
	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;
	if (r_roundImagesDown->integer) {
		if (scaledWidth > width)
			scaledWidth >>= 1;
		if (scaledHeight > height)
			scaledHeight >>= 1;
	}

	// Mipmap
	mipMap = (flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST)) ? qFalse : qTrue;

	// Let people sample down the world textures for speed
	if (mipMap && !(flags & IF_NOPICMIP)) {
		if (gl_picmip->integer > 0) {
			scaledWidth >>= gl_picmip->integer;
			scaledHeight >>= gl_picmip->integer;
		}
	}

	// Clamp dimensions
	scaledWidth = clamp (scaledWidth, 1, glConfig.maxCMTexSize);
	scaledHeight = clamp (scaledHeight, 1, glConfig.maxCMTexSize);

	// Get the image format
	format = R_ImageFormat (name, flags, &samples);

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
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (r_ext_maxAnisotropy->integer, 1, glConfig.maxAniso));

		qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, glState.texMinFilter);
		qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
	else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

		if (flags & IF_NOMIPMAP_LINEAR) {
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	// Cubemaps use edge clamping
	if (glConfig.extTexEdgeClamp) {
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	else {
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP);
	}

	// Allocate a buffer
	scaled = Mem_PoolAllocExt (scaledWidth * scaledHeight * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	if (!scaled)
		Com_Error (ERR_FATAL, "R_UploadCMImage: Out of memory!\n");

	// Upload
	for (i=0 ; i<6 ; i++) {
		// Resample
		R_ResampleImage ((uint32 *)(data[i]), width, height, scaled, scaledWidth, scaledHeight);

		// Scan and replace channels if desired
		if (flags & (IF_NORGB|IF_NOALPHA)) {
			byte	*scan;

			if (flags & IF_NORGB) {
				scan = (byte *)scaled;
				for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
					scan[0] = scan[1] = scan[2] = 255;
			}
			else {
				scan = (byte *)scaled + 3;
				for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
					*scan = 255;
			}
		}

		// Apply image gamma/intensity
		R_LightScaleImage (scaled, scaledWidth, scaledHeight, (!(flags & IF_NOGAMMA)) && !glConfig.hwGammaInUse, mipMap && !(flags & IF_NOINTENS));

		// Upload the base image
		qglTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+i, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

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

				if (r_colorMipLevels->integer)
					R_ColorMipLevel ((byte *)scaled, mipWidth * mipHeight, mipLevel);

				qglTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+i, mipLevel, format, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
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
R_Upload2DImage
===============
*/
static void R_Upload2DImage (char *name, byte *data, int width, int height, int flags, int samples, int *upWidth, int *upHeight, int *upFormat)
{
	GLsizei		scaledWidth, scaledHeight;
	qBool		mipMap;
	uint32		*scaled;
	GLint		format;
	int			c;

	// Find next highest power of two
	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;
	if (r_roundImagesDown->integer) {
		if (scaledWidth > width)
			scaledWidth >>= 1;
		if (scaledHeight > height)
			scaledHeight >>= 1;
	}

	// Mipmap
	mipMap = (flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST)) ? qFalse : qTrue;

	// Let people sample down the world textures for speed
	if (mipMap && !(flags & IF_NOPICMIP)) {
		if (gl_picmip->integer > 0) {
			scaledWidth >>= gl_picmip->integer;
			scaledHeight >>= gl_picmip->integer;
		}
	}

	// Clamp dimensions
	scaledWidth = clamp (scaledWidth, 1, glConfig.maxTexSize);
	scaledHeight = clamp (scaledHeight, 1, glConfig.maxTexSize);

	// Get the image format
	format = R_ImageFormat (name, flags, &samples);

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
			qglTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (r_ext_maxAnisotropy->integer, 1, glConfig.maxAniso));

		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glState.texMinFilter);
		qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
	else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

		if (flags & IF_NOMIPMAP_LINEAR) {
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	// Texture edge clamping
	if (!(flags & IF_CLAMP)) {
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else if (glConfig.extTexEdgeClamp) {
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else {
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Allocate a buffer
	scaled = Mem_PoolAllocExt (scaledWidth * scaledHeight * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	if (!scaled)
		Com_Error (ERR_FATAL, "R_Upload2DImage: Out of memory!\n");

	// Resample
	R_ResampleImage ((uint32 *)data, width, height, scaled, scaledWidth, scaledHeight);

	// Scan and replace channels if desired
	if (flags & (IF_NORGB|IF_NOALPHA)) {
		byte	*scan;

		if (flags & IF_NORGB) {
			scan = (byte *)scaled;
			for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
				scan[0] = scan[1] = scan[2] = 255;
		}
		else {
			scan = (byte *)scaled + 3;
			for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
				*scan = 255;
		}
	}

	// Apply image gamma/intensity
	R_LightScaleImage (scaled, scaledWidth, scaledHeight, (!(flags & IF_NOGAMMA)) && !glConfig.hwGammaInUse, mipMap && !(flags & IF_NOINTENS));

	// Upload the base image
	qglTexImage2D (GL_TEXTURE_2D, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

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

			if (r_colorMipLevels->integer)
				R_ColorMipLevel ((byte *)scaled, mipWidth * mipHeight, mipLevel);

			qglTexImage2D (GL_TEXTURE_2D, mipLevel, format, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
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
R_Upload3DImage

FIXME:
- Support mipmapping, light scaling, and size scaling
- Test SGIS mipmap gen
- r_roundImagesDown, r_colorMipLevels
===============
*/
static void R_Upload3DImage (char *name, byte **data, int width, int height, int depth, int flags, int samples, int *upWidth, int *upHeight, int *upDepth, int *upFormat)
{
	GLsizei		scaledWidth, scaledHeight, scaledDepth;
	GLint		format;
	qBool		mipMap;
	int			i, c;

	// Find next highest power of two
	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;
	for (scaledDepth=1 ; scaledDepth<depth ; scaledDepth<<=1) ;

	// Mipmap
	mipMap = (flags & (IF_NOMIPMAP_LINEAR|IF_NOMIPMAP_NEAREST)) ? qFalse : qTrue;

	// Mipmapping not supported
	if (mipMap && !glConfig.extSGISGenMipmap)
		Com_Error (ERR_DROP, "R_Upload3DImage: mipmapping not yet supported");
	if (width != scaledWidth || height != scaledHeight || depth != scaledDepth)
		Com_Error (ERR_DROP, "R_Upload3DImage: scaling not supported, use power of two dimensions and depth");
	if (scaledWidth > glConfig.max3DTexSize || scaledHeight > glConfig.max3DTexSize || scaledDepth > glConfig.max3DTexSize)
		Com_Error (ERR_DROP, "R_Upload3DImage: dimensions too large, scaling not yet supported");

	// Get the image format
	format = R_ImageFormat (name, flags, &samples);

	// Set base upload values
	if (upWidth)
		*upWidth = scaledWidth;
	if (upHeight)
		*upHeight = scaledHeight;
	if (upDepth)
		*upDepth = scaledDepth;
	if (upFormat)
		*upFormat = format;

	// Texture params
	if (mipMap) {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (GL_TEXTURE_3D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (r_ext_maxAnisotropy->integer, 1, glConfig.maxAniso));

		qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, glState.texMinFilter);
		qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
	else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (GL_TEXTURE_3D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

		if (glConfig.extTexFilterAniso)
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

		if (flags & IF_NOMIPMAP_LINEAR) {
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else {
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	// Texture edge clamping
	if (!(flags & IF_CLAMP)) {
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	}
	else if (glConfig.extTexEdgeClamp) {
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
	else {
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	}

	// Scan and replace channels if desired
	if (flags & (IF_NORGB|IF_NOALPHA)) {
		byte	*scan;

		if (flags & IF_NORGB) {
			for (i=0 ; i<depth ; i++) {
				scan = (byte *)data[i];
				for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
					scan[0] = scan[1] = scan[2] = 255;
			}
		}
		else {
			for (i=0 ; i<depth ; i++) {
				scan = (byte *)data[i] + 3;
				for (c=scaledWidth*scaledHeight ; c>0 ; c--, scan+=4)
					*scan = 255;
			}
		}
	}

	// Upload
	qglTexImage3D (GL_TEXTURE_3D, 0, format, scaledWidth, scaledHeight, scaledDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data[0]);
}

/*
==============================================================================

	PALETTE

==============================================================================
*/

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
	uint32	*trans;
	int		i, s, pxl;
	int		samples;

	s = width * height;
	trans = Mem_PoolAllocExt (s * 4, qFalse, MEMPOOL_IMAGESYS, 0);

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

	R_Upload2DImage (name, (byte *)trans, width, height, flags, samples, &image->upWidth, &image->upHeight, &image->format);
	Mem_Free (trans);
}

/*
==============================================================================

	IMAGE LOADING

==============================================================================
*/

/*
================
R_BareImageName
================
*/
static const char *R_BareImageName (const char *name)
{
	static char	bareName[2][MAX_QPATH];
	static int	bareIndex;

	bareIndex ^= 1;

	// Fix/strip barename
	Com_NormalizePath (bareName[bareIndex], sizeof (bareName[bareIndex]), name);
	Com_StripExtension (bareName[bareIndex], sizeof (bareName[bareIndex]), bareName[bareIndex]);
	Q_strlwr (bareName[bareIndex]);

	return bareName[bareIndex];
}


/*
================
R_FindImage
================
*/
static image_t *R_FindImage (const char *bareName, int flags)
{
	image_t	*image;
	uint32	hash;

	// Calculate hash
	hash = Com_HashGeneric (bareName, MAX_IMAGE_HASH);

	if (r_refRegInfo.inSequence)
		r_refRegInfo.imagesSearched++;

	// Look for it
	if (flags == 0) {
		for (image=r_imageHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->touchFrame)
				continue;

			// Check name
			if (!strcmp (bareName, image->bareName))
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
			if (!strcmp (bareName, image->bareName))
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
image_t *R_LoadImage (char *name, const char *bareName, byte **pic, int width, int height, int depth, texFlags_t flags, int samples, qBool upload8, qBool isPCX)
{
	image_t		*image;
	uint32		i;

	// Find a free r_imageList spot
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (image->touchFrame)
			continue;

		image->texNum = i + 1;
		break;
	}

	// None found, create a new spot
	if (i == r_numImages) {
		if (r_numImages+1 >= MAX_IMAGES)
			Com_Error (ERR_DROP, "R_LoadImage: MAX_IMAGES");

		image = &r_imageList[r_numImages++];
		image->texNum = r_numImages;
	}

	// See if this texture is allowed
	if (image->flags & IT_3D && depth > 1 && !glConfig.extTex3D)
		Com_Error (ERR_DROP, "R_LoadImage: '%s' is 3D and 3D textures are disabled", name);

	// Set the name
	Q_strncpyz (image->name, name, sizeof (image->name));
	if (!bareName)
		bareName = R_BareImageName (name);
	Q_strncpyz (image->bareName, bareName, sizeof (image->bareName));

	// Set width, height, and depth
	image->width = image->tcWidth = width;
	image->height = image->tcHeight = height;
	image->depth = depth;

	// Texture scaling, hacky special case!
	if (!upload8 && !(flags & (IF_NOMIPMAP_NEAREST|IF_NOMIPMAP_LINEAR))) {
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
	image->touchFrame = r_refRegInfo.registerFrame;
	image->hashValue = Com_HashGeneric (image->bareName, MAX_IMAGE_HASH);

	if (!(image->flags & (IT_CUBEMAP|IT_3D)))
		image->target = GL_TEXTURE_2D;
	else if (image->flags & IT_CUBEMAP)
		image->target = GL_TEXTURE_CUBE_MAP_ARB;
	else if (image->depth > 1) {
		image->target = GL_TEXTURE_3D;
#ifdef _DEBUG
		assert (image->flags & IT_3D);
#endif // _DEBUG
	}

	// Upload
	GL_BindTexture (image);
	switch (image->target) {
	case GL_TEXTURE_2D:
		if (upload8)
			R_PalToRGBA (name, *pic, width, height, flags, image, isPCX);
		else
			R_Upload2DImage (name, *pic, width, height, flags, samples, &image->upWidth, &image->upHeight, &image->format);
		break;

	case GL_TEXTURE_3D:
		R_Upload3DImage (name, pic, width, height, depth, flags, samples, &image->upWidth, &image->upHeight, &image->upDepth, &image->format);
		break;

	case GL_TEXTURE_CUBE_MAP_ARB:
		R_UploadCMImage (name, pic, width, height, flags, samples, &image->upWidth, &image->upHeight, &image->format);
		break;
	}

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
static inline image_t *R_RegisterCubeMap (char *name, texFlags_t flags)
{
	image_t		*image;
	int			i, j;
	int			samples;
	byte		*pic[6];
	int			width, height;
	int			firstSize, firstSamples;
	char		loadName[MAX_QPATH];
	const char	*bareName;

	// Make sure we have this
	flags |= (IT_CUBEMAP|IF_CLAMP);

	// Generate the bare name
	bareName = R_BareImageName (name);

	// See if it's already loaded
	image = R_FindImage (bareName, flags);
	if (image) {
		image->touchFrame = r_refRegInfo.registerFrame;
		return image;
	}

	// Not found -- load the pic from disk
	for (i=0 ; i<6 ; i++) {
		pic[i] = NULL;

		// PNG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.png", bareName, r_cubeMapSuffix[i]);
		samples = R_LoadPNG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// TGA
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.tga", bareName, r_cubeMapSuffix[i]);
		samples = R_LoadTGA (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// JPG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.jpg", bareName, r_cubeMapSuffix[i]);
		samples = R_LoadJPG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// Not found
		break;

checkPic:
		if (width != height) {
			Com_Printf (PRNT_WARNING, "R_RegisterCubeMap: %s is not square!\n", loadName);
			goto emptyTrash;
		}

		if (!i) {
			firstSize = width;
			firstSamples = samples;
		}
		else {
			if (firstSize != width) {
				Com_Printf (PRNT_WARNING, "R_RegisterCubeMap: Size mismatch with previous in %s! Halting!\n", loadName);
				break;
			}

			if (firstSamples != samples) {
				Com_Printf (PRNT_WARNING, "R_RegisterCubeMap: Sample mismatch with previous in %s! Halting!\n", loadName);
				break;
			}
		}
	}

	if (i != 6) {
		Com_Printf (PRNT_WARNING, "R_RegisterCubeMap: Unable to find all of the sides, aborting!\n");
		goto emptyTrash;
	}
	else
		image = R_LoadImage (loadName, bareName, pic, width, height, 1, flags, samples, qFalse, qFalse);

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
image_t	*R_RegisterImage (char *name, texFlags_t flags)
{
	image_t		*image;
	byte		*pic;
	int			width, height, samples;
	char		loadName[MAX_QPATH];
	qBool		upload8, isPCX;
	const char	*bareName;

	// Check the name
	if (!name || !name[0])
		return NULL;

	// Check the length
	width = strlen (name);
	if (width < 5) {
		Com_Printf (PRNT_ERROR, "R_RegisterImage: Image name too short! %s\n", name);
		return NULL;
	}
	if (width+1 >= MAX_QPATH) {
		Com_Printf (PRNT_ERROR, "R_RegisterImage: Image name too long! %s\n", name);
		return NULL;
	}

	// Cubemap stuff
	if (flags & IT_CUBEMAP && glConfig.extTexCubeMap)
		return R_RegisterCubeMap (name, flags);
	flags &= ~IT_CUBEMAP;

	// Generate the bare name
	bareName = R_BareImageName (name);

	// See if it's already loaded
	image = R_FindImage (bareName, flags);
	if (image) {
		image->touchFrame = r_refRegInfo.registerFrame;
		return image;
	}

	// Not found -- load the pic from disk
	upload8 = qFalse;
	isPCX = qFalse;

	// PNG
	Q_snprintfz (loadName, sizeof (loadName), "%s.png", bareName);
	samples = R_LoadPNG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// TGA
	Q_snprintfz (loadName, sizeof (loadName), "%s.tga", bareName);
	samples = R_LoadTGA (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// JPG
	Q_snprintfz (loadName, sizeof (loadName), "%s.jpg", bareName);
	samples = R_LoadJPG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	pic = NULL;
	upload8 = qTrue;
	samples = 3;
	if (!(strcmp (name + strlen (name) - 4, ".wal"))) {
		// WAL
		Q_snprintfz (loadName, sizeof (loadName), "%s.wal", bareName);
		R_LoadWal (loadName, &pic, &width, &height);
		if (pic)
			goto loadImage;
	}
	else {
		// PCX
		Q_snprintfz (loadName, sizeof (loadName), "%s.pcx", bareName);
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
	image = R_LoadImage (loadName, bareName, &pic, width, height, 1, flags, samples, upload8, isPCX);

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
void R_FreeImage (image_t *image)
{
	image_t	*hashImg;
	image_t	**prev;

	assert (image && image->texNum);
	if (!image)
		return;

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
	if (image->texNum)
		qglDeleteTextures (1, &image->texNum);
	else
		Com_DevPrintf (PRNT_WARNING, "R_FreeImage: attempted to release invalid texNum on image '%s'!\n", image->name);
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
	uint32	i;

	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;		// Free image_t slot
		if (image->finishedLoading)
			continue;

		R_FreeImage (image);
	}
}


/*
================
R_EndImageRegistration

Any image that was not touched on this registration sequence will be released
================
*/
void R_EndImageRegistration (void)
{
	image_t	*image;
	uint32	i;

	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (image->touchFrame == r_refRegInfo.registerFrame)
			continue;		// Used this sequence
		if (image->flags & IF_NOFLUSH) {
			image->touchFrame = r_refRegInfo.registerFrame;
			r_refRegInfo.imagesTouched++;
			continue;		// Don't free
		}

		R_FreeImage (image);
		r_refRegInfo.imagesReleased++;
	}
}


/*
===============
R_GetImageSize
===============
*/
void R_GetImageSize (shader_t *shader, int *width, int *height)
{
	shaderPass_t	*pass;
	image_t			*image;
	int				i;
	int				passNum;

	if (!shader || !shader->numPasses) {
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		return;
	}

	image = NULL;
	passNum = 0;
	for (i=0, pass=shader->passes ; i<shader->numPasses ; pass++, i++) {
		if (passNum++ != shader->sizeBase)
			continue;

		image = pass->animFrames[0];
		break;
	}

	if (!image)
		return;

	if (width)
		*width = image->width;
	if (height)
		*height = image->height;
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
	uint32		i, j;
	uint32		total;

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
	uint32		tempWidth, tempHeight;
	uint32		i, totalImages, totalMips;
	uint32		mipTexels, texels;
	image_t		*image;

	Com_Printf (0, "------------------------------------------------------\n");
	Com_Printf (0, "Tex# Ta Format       LGIFC Width Height Name\n");
	Com_Printf (0, "---- -- ------------ ----- ----- ------ --------------\n");

	for (i=0, totalImages=0, totalMips=0, mipTexels=0, texels=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;	// Free r_imageList slot

		// Texnum
		Com_Printf (0, "%4d ", image->texNum);

		// Target
		switch (image->target) {
		case GL_TEXTURE_CUBE_MAP_ARB:			Com_Printf (0, "CM ");			break;
		case GL_TEXTURE_1D:						Com_Printf (0, "1D ");			break;
		case GL_TEXTURE_2D:						Com_Printf (0, "2D ");			break;
		case GL_TEXTURE_3D:						Com_Printf (0, "3D ");			break;
		}

		// Format
		switch (image->format) {
		case GL_RGBA8:							Com_Printf (0, "RGBA8     ");	break;
		case GL_RGBA4:							Com_Printf (0, "RGBA4     ");	break;
		case GL_RGBA:							Com_Printf (0, "RGBA      ");	break;
		case GL_RGB8:							Com_Printf (0, "RGB8      ");	break;
		case GL_RGB5:							Com_Printf (0, "RGB5      ");	break;
		case GL_RGB:							Com_Printf (0, "RGB       ");	break;

		case GL_DSDT8_NV:						Com_Printf (0, "DSDT8     ");	break;

		case GL_COMPRESSED_RGB_ARB:				Com_Printf (0, "RGB   ARB  ");	break;
		case GL_COMPRESSED_RGBA_ARB:			Com_Printf (0, "RGBA  ARB  ");	break;

		case GL_RGB_S3TC:						Com_Printf (0, "RGB   S3   ");	break;
		case GL_RGB4_S3TC:						Com_Printf (0, "RGB4  S3   ");	break;
		case GL_RGBA_S3TC:						Com_Printf (0, "RGBA  S3   ");	break;
		case GL_RGBA4_S3TC:						Com_Printf (0, "RGBA4 S3   ");	break;

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	Com_Printf (0, "RGB   DXT1 ");	break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	Com_Printf (0, "RGBA  DXT1 ");	break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	Com_Printf (0, "RGBA  DXT3 ");	break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	Com_Printf (0, "RGBA  DXT5 ");	break;

		default:								Com_Printf (0, "??????     ");	break;
		}

		// Flags
		Com_Printf (0, "%s", (image->flags & IT_LIGHTMAP)	? "L" : "-");
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
enum {
	SSHOTTYPE_JPG,
	SSHOTTYPE_PNG,
	SSHOTTYPE_TGA,
};
static void R_ScreenShot_f (void)
{
	char	checkName[MAX_OSPATH];
	int		type, shotNum, quality;
	char	*ext;
	byte	*buffer;
	FILE	*f;

	// Find out what format to save in
	if (Cmd_Argc () > 1)
		ext = Cmd_Argv (1);
	else
		ext = gl_screenshot->string;

	if (!Q_stricmp (ext, "png"))
		type = SSHOTTYPE_PNG;
	else if (!Q_stricmp (ext, "jpg"))
		type = SSHOTTYPE_JPG;
	else
		type = SSHOTTYPE_TGA;

	// Set necessary values
	switch (type) {
	case SSHOTTYPE_TGA:
		Com_Printf (0, "Taking TGA screenshot...\n");
		quality = 100;
		ext = "tga";
		break;

	case SSHOTTYPE_PNG:
		Com_Printf (0, "Taking PNG screenshot...\n");
		quality = 100;
		ext = "png";
		break;

	case SSHOTTYPE_JPG:
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
		Com_Printf (PRNT_WARNING, "R_ScreenShot_f: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	// Allocate room for a copy of the framebuffer
	buffer = Mem_PoolAllocExt (glConfig.vidWidth * glConfig.vidHeight * 3, qFalse, MEMPOOL_IMAGESYS, 0);

	// Read the framebuffer into our storage
	if (glConfig.extBGRA && type == SSHOTTYPE_TGA) {
		qglReadPixels (0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer);

		// Apply hardware gamma
		if (glConfig.hwGammaInUse) {
			int		i, size;

			size = glConfig.vidWidth * glConfig.vidHeight * 3;

			for (i=0 ; i<size ; i+=3) {
				buffer[i+2] = glStatic.gammaRamp[buffer[i+2]] >> 8;
				buffer[i+1] = glStatic.gammaRamp[buffer[i+1] + 256] >> 8;
				buffer[i+0] = glStatic.gammaRamp[buffer[i+0] + 512] >> 8;
			}
		}
	}
	else {
		qglReadPixels (0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer);

		// Apply hardware gamma
		if (glConfig.hwGammaInUse) {
			int		i, size;

			size = glConfig.vidWidth * glConfig.vidHeight * 3;

			for (i=0 ; i<size ; i+=3) {
				buffer[i+0] = glStatic.gammaRamp[buffer[i+0]] >> 8;
				buffer[i+1] = glStatic.gammaRamp[buffer[i+1] + 256] >> 8;
				buffer[i+2] = glStatic.gammaRamp[buffer[i+2] + 512] >> 8;
			}
		}
	}

	// Write
	switch (type) {
	case SSHOTTYPE_TGA:
		R_WriteTGA (f, buffer, glConfig.vidWidth, glConfig.vidHeight, !glConfig.extBGRA);
		break;

	case SSHOTTYPE_PNG:
		R_WritePNG (f, buffer, glConfig.vidWidth, glConfig.vidHeight);
		break;

	case SSHOTTYPE_JPG:
		R_WriteJPG (f, buffer, glConfig.vidWidth, glConfig.vidHeight, quality);
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

static void	*cmd_imageCheck;
static void	*cmd_imageList;
static void	*cmd_screenShot;

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

	if (!glConfig.hwGammaInUse)
		return;

	for (i=0 ; i<256 ; i++) {
		gam = (byte)(clamp (255 * pow (( (i & 255) + 0.5f)*0.0039138943248532289628180039138943, vid_gamma->value) + 0.5f, 0, 255));
		glStatic.gammaRamp[i] = glStatic.gammaRamp[i + 256] = glStatic.gammaRamp[i + 512] = gam * 255;
	}

	GLimp_SetGammaRamp (glStatic.gammaRamp);
}


/*
==================
R_InitSpecialTextures
==================
*/
#define INTTEXSIZE	32
#define INTTEXBYTES	4
static void R_InitSpecialTextures (void)
{
	int		size;
	int		x, y, z, d;
	float	intensity;
	double	tw, th, tx, ty, t;
	byte	*data;
	vec3_t	v;

	/*
	** r_noTexture
	*/
	data = Mem_PoolAllocExt (INTTEXSIZE * INTTEXSIZE * INTTEXBYTES, qFalse, MEMPOOL_IMAGESYS, 0);
	for (x=0 ; x<INTTEXSIZE ; x++) {
		for (y=0 ; y<INTTEXSIZE ; y++) {
			data[(y*INTTEXSIZE + x)*4+3] = 255;

			if ((x == 0 || x == INTTEXSIZE-1) || (y == 0 || y == INTTEXSIZE-1)) {
				data[(y*INTTEXSIZE + x)*INTTEXBYTES+0] = 127;
				data[(y*INTTEXSIZE + x)*INTTEXBYTES+1] = 127;
				data[(y*INTTEXSIZE + x)*INTTEXBYTES+2] = 127;
				continue;
			}

			data[(y*INTTEXSIZE + x)*INTTEXBYTES+0] = 31;
			data[(y*INTTEXSIZE + x)*INTTEXBYTES+1] = 31;
			data[(y*INTTEXSIZE + x)*INTTEXBYTES+2] = 31;
		}
	}

	memset (&r_noTexture, 0, sizeof (r_noTexture));
	r_noTexture = R_Load2DImage ("***r_noTexture***", &data, INTTEXSIZE, INTTEXSIZE,
		IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3);

	/*
	** r_whiteTexture
	*/
	memset (data, 255, INTTEXSIZE * INTTEXSIZE * INTTEXBYTES);
	memset (&r_whiteTexture, 0, sizeof (r_whiteTexture));
	r_whiteTexture = R_Load2DImage ("***r_whiteTexture***", &data, INTTEXSIZE, INTTEXSIZE,
		IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3);

	/*
	** r_blackTexture
	*/
	memset (data, 0, INTTEXSIZE * INTTEXSIZE * INTTEXBYTES);
	memset (&r_blackTexture, 0, sizeof (r_blackTexture));
	r_blackTexture = R_Load2DImage ("***r_blackTexture***", &data, INTTEXSIZE, INTTEXSIZE,
		IF_NOFLUSH|IF_NOPICMIP|IF_NOGAMMA|IF_NOINTENS|IF_NOCOMPRESS, 3);

	Mem_Free (data);

	/*
	** r_cinTexture
	** Reserve a texNum for cinematics
	*/
	data = Mem_PoolAllocExt (256 * 256 * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	memset (data, 0, 256 * 256 * 4);
	memset (&r_cinTexture, 0, sizeof (r_cinTexture));
	r_cinTexture = R_Load2DImage ("***r_cinTexture***", &data, 256, 256,
		IF_NOFLUSH|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_CLAMP|IF_NOINTENS|IF_NOGAMMA|IF_NOCOMPRESS, 3);
	Mem_Free (data);

	/*
	** r_dLightTexture
	*/
	if (glConfig.extTex3D) {
		size = 32;
		data = Mem_PoolAllocExt (size * size * size * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	}
	else {
		size = 64;
		data = Mem_PoolAllocExt (size * size * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	}

	for (x=0 ; x<size ; x++) {
		for (y=0 ; y<size ; y++) {
			for (z=0 ; z<size ; z++) {
				v[0] = ((x + 0.5f) * (2.0f / (float)size) - 1.0f) * (1.0f / 0.9375);
				v[1] = ((y + 0.5f) * (2.0f / (float)size) - 1.0f) * (1.0f / 0.9375);
				if (glConfig.extTex3D)
					v[2] = ((z + 0.5f) * (2.0f / (float)size) - 1.0f) * (1.0f / 0.9375);
				else
					v[2] = 0;

				intensity = 1.0f - VectorLength (v);
				if (intensity > 0)
					intensity *= 256.0f;
				d = clamp (intensity, 0, 255);

				data[((z*size+y)*size + x) * 4 + 0] = d;
				data[((z*size+y)*size + x) * 4 + 1] = d;
				data[((z*size+y)*size + x) * 4 + 2] = d;
				data[((z*size+y)*size + x) * 4 + 3] = 255;

				if (!glConfig.extTex3D)
					break;
			}
		}
	}

	if (glConfig.extTex3D)
		r_dLightTexture = R_Load3DImage ("***r_dLightTexture***", &data, size, size, size,
			IF_NOFLUSH|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOINTENS|IF_NOGAMMA|IF_CLAMP|IT_3D, 4);
	else
		r_dLightTexture = R_Load2DImage ("***r_dLightTexture***", &data, size, size,
			IF_NOFLUSH|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOINTENS|IF_NOGAMMA|IF_CLAMP, 4);
	Mem_Free (data);

	/*
	** r_fogTexture
	*/
	tw = 1.0f / ((float)FOGTEX_WIDTH - 1.0f);
	th = 1.0f / ((float)FOGTEX_HEIGHT - 1.0f);

	data = Mem_PoolAllocExt (FOGTEX_WIDTH * FOGTEX_HEIGHT * 4, qFalse, MEMPOOL_IMAGESYS, 0);
	memset (data, 255, FOGTEX_WIDTH*FOGTEX_HEIGHT*4);

	for (y=0, ty=0.0f ; y<FOGTEX_HEIGHT ; y++, ty+=th) {
		for (x=0, tx=0.0f ; x<FOGTEX_WIDTH ; x++, tx+=tw) {
			t = sqrt (tx) * 255.0;
			data[(x+y*FOGTEX_WIDTH)*4+3] = (byte)(min (t, 255.0));
		}

		data[y*4+3] = 0;
	}
	r_fogTexture = R_Load2DImage ("***r_fogTexture***", &data, FOGTEX_WIDTH, FOGTEX_HEIGHT,
		IF_NOFLUSH|IF_NOPICMIP|IF_NOMIPMAP_LINEAR|IF_NOINTENS|IF_NOGAMMA|IF_CLAMP, 4);
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
	uint32	v;
	byte	*pal;

	Com_Printf (0, "\n--------- Image Initialization ---------\n");

	// Registration
	cmd_imageCheck	= Cmd_AddCommand ("imagecheck",	R_ImageCheck_f,			"Image system integrity check");
	cmd_imageList	= Cmd_AddCommand ("imagelist",	R_ImageList_f,			"Prints out a list of the currently loaded textures");
	cmd_screenShot	= Cmd_AddCommand ("screenshot",	R_ScreenShot_f,			"Takes a screenshot");

	// Defaults
	r_numImages = 0;

	// Set the initial state
	GL_TextureMode (qTrue, qFalse);
	GL_TextureBits (qTrue, qFalse);

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
		Cvar_VariableSetValue (intensity, 1, qTrue);
	glState.invIntens = 1.0f / intensity->value;

	// Hack! because voodoo's are nasty bright
	if (glStatic.renderClass == REND_CLASS_VOODOO)
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
	glStatic.rampDownloaded = GLimp_GetGammaRamp (glStatic.originalRamp);
	if (glStatic.rampDownloaded) {
		Com_Printf (0, "...GLimp_GetGammaRamp succeeded\n");
		glConfig.hwGammaAvail = qTrue;
	}
	else {
		Com_Printf (PRNT_ERROR, "...GLimp_GetGammaRamp failed!\n");
		glConfig.hwGammaAvail = qFalse;
	}

	// Use hardware gamma?
	glConfig.hwGammaInUse = (glStatic.rampDownloaded && r_hwGamma->integer);
	if (glConfig.hwGammaInUse) {
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
	uint32	i;
	image_t	*image;

	// Replace gamma ramp
	if (glConfig.hwGammaInUse)
		GLimp_SetGammaRamp (glStatic.originalRamp);

	// Unregister commands
	Cmd_RemoveCommand ("imagecheck", cmd_imageCheck);
	Cmd_RemoveCommand ("imagelist", cmd_imageList);
	Cmd_RemoveCommand ("screenshot", cmd_screenShot);

	// Free loaded textures
	for (i=0, image=r_imageList ; i<r_numImages ; i++, image++) {
		if (!image->touchFrame)
			continue;	// Free r_imageList slot
		if (!image->texNum)
			continue;

		// Free it
		qglDeleteTextures (1, &image->texNum);
	}

	// Clear everything
	r_numImages = 0;
	memset (r_imageList, 0, sizeof (image_t) * MAX_IMAGES);
	memset (r_imageHashTree, 0, sizeof (image_t *) * MAX_IMAGE_HASH);
	memset (r_lmTextures, 0, sizeof (image_t *) * R_MAX_LIGHTMAPS);

	// Free memory
	Mem_FreePool (MEMPOOL_IMAGESYS);
}
