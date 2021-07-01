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
#define USE_FIXPATHNAME
#define USE_HASH
#include "r_local.h"
#include "jpg/jpeglib.h"
#include "png/png.h"
#include <process.h>

#define MAX_IMAGE_HASH	128

image_t		lmTextures[MAX_LIGHTMAPS];
image_t		imgTextures[MAX_IMAGES];
image_t		*imgHashTree[MAX_IMAGE_HASH];
uInt		imgNumTextures;

uLong		imgRegistrationFrame;

static byte		imgIntensityTable[256];
static byte		imgGammaTable[256];
uInt			imgPaletteTable[256];

static uInt		*imgResamp;	// for image resampling

const char		*sky_NameSuffix[6]		= { "rt", "bk", "lf", "ft", "up", "dn" };
const char		*cubeMap_NameSuffix[6]	= { "px", "nx", "py", "ny", "pz", "nz" };

byte img_DefaultPal[] = {
	#include "defpal.h"
};

image_t *Img_LoadPic (char *name, byte **pic, int width, int height, int flags, int samples, qBool upload8);

image_t	*r_NoTexture;		// use for bad textures
image_t	*r_WhiteTexture;	// used in shaders/fallback
image_t	*r_BlackTexture;	// used in shaders/fallback
image_t	*r_CinTexture;		// allocates memory on load as to not every cin frame
image_t *r_BloomTexture;	// full-scene bloom texture

cVar_t *intensity;

cVar_t *gl_jpgquality;
cVar_t *gl_nobind;
cVar_t *gl_picmip;
cVar_t *gl_screenshot;
cVar_t *gl_texturebits;
cVar_t *gl_texturemode;

cVar_t *vid_gamma;
cVar_t *vid_gammapics;

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
GL_Bind
===============
*/
void GL_Bind (image_t *image) {
	int		texNum;

	if (gl_nobind->integer || !image)	// performance evaluation option
		texNum = r_NoTexture->texNum;
	else
		texNum = image->texNum;
	if (glState.texNums[glState.texUnit] == texNum)
		return;

	glState.texNums[glState.texUnit] = texNum;
	if (image && (image->upFlags & IT_CUBEMAP))
		qglBindTexture (GL_TEXTURE_CUBE_MAP_ARB, texNum);
	else
		qglBindTexture (GL_TEXTURE_2D, texNum);

	glSpeeds.textureBinds++;
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
==================
GL_TextureBits
==================
*/
void GL_TextureBits (qBool set) {
	switch (gl_texturebits->integer) {
	case 32:
		if (set) {
			glState.texRGBFormat = GL_RGB8;
			glState.texRGBAFormat = GL_RGBA8;
		}

		Com_Printf (0, "Texture bits: 32\n");
		break;

	case 16:
		if (set) {
			glState.texRGBFormat = GL_RGB5;
			glState.texRGBAFormat = GL_RGBA4;
		}

		Com_Printf (0, "Texture bits: 16\n");
		break;

	default:
		if (set) {
			glState.texRGBFormat = GL_RGB;
			glState.texRGBAFormat = GL_RGBA;
		}

		Com_Printf (0, "Texture bits: Default\n");
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

void GL_TextureMode (qBool verbose) {
	int		i;
	image_t	*image;
	char	*string = gl_texturemode->string;

	for (i=0 ; i<NUM_GL_MODES ; i++) {
		if (!Q_stricmp (modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES) {
		Com_Printf (0, S_COLOR_YELLOW "bad filter name -- falling back to GL_LINEAR_MIPMAP_NEAREST\n");
		
		glState.texMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		glState.texMagFilter = GL_LINEAR;

		Cvar_VariableForceSet (gl_texturemode, "GL_LINEAR_MIPMAP_NEAREST");
	}
	else {
		glState.texMinFilter = modes[i].minimize;
		glState.texMagFilter = modes[i].maximize;
	}

	gl_texturemode->modified = qFalse;
	if (verbose) {
		Com_Printf (0, "Texture mode: %s\n", modes[i].name);
	}

	// change all the existing mipmap texture objects
	for (i=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;		// free imgTextures slot

		GL_Bind (image);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (image->upFlags & IF_NOMIPMAP) ? glState.texMagFilter : glState.texMinFilter);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);
	}
}


/*
===============
GL_ResetAnisotropy
===============
*/
void GL_ResetAnisotropy (void) {
	int		i;
	image_t	*image;

	gl_ext_max_anisotropy->modified = qFalse;

	if (!glConfig.extTexFilterAniso)
		return;

	// change all the existing mipmap texture objects
	for (i=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;		// free imgTextures slot
		if (image->upFlags & IF_NOMIPMAP)
			continue;		// skip non-mipmapped imagery

		GL_Bind (image);
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
    Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Premeture end of jpeg file\n");

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
static int Img_LoadJPG (char *name, byte **pic, int *width, int *height) {
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
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg samples '%s' (%d)\n", name, samples);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if ((cinfo.output_width <= 0) || (cinfo.output_height <= 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadJPG: Bad jpeg dimensions on '%s' ( %d x %d )\n", name, cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return 0;
	}

	if (width)	*width = cinfo.output_width;
	if (height)	*height = cinfo.output_height;

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
Img_JPGScreenShot

by Robert 'Heffo' Heffernan
================== 
*/
static void Img_JPGScreenShot (void) {
	int			i, offset, w3;
	char		checkname[MAX_OSPATH], picname[MAX_OSPATH];
	struct		jpeg_compress_struct	cinfo;
	struct		jpeg_error_mgr			jerr;
	byte		*buffer, *s;
	FILE		*f;

	Com_Printf (0, "Taking JPG screenshot...\n");

	// create the scrnshots directory if it doesn't exist
	Q_snprintfz (checkname, sizeof (checkname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (checkname);

	// find a file name to save it to
	for (i = 0 ; i<999 ; i++) {
		Q_snprintfz (picname, sizeof (picname), "%s/scrnshot/egl%.3d.jpg", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)	break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (0, S_COLOR_YELLOW "Img_JPGScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	// allocate room for a copy of the framebuffer
	buffer = Mem_PoolAlloc (vidDef.width * vidDef.height * 3, MEMPOOL_IMAGESYS, 0);

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
	cinfo.progressive_mode = TRUE;

	jpeg_set_defaults (&cinfo);

	if ((gl_jpgquality->value > 100) || (gl_jpgquality->value <= 0))
		Cvar_VariableForceSetValue (gl_jpgquality, 85);

	jpeg_set_quality (&cinfo, gl_jpgquality->integer, TRUE);
	jpeg_start_compress (&cinfo, qTrue);	// start compression

	jpeg_write_marker (&cinfo, JPEG_COM, (byte *) "EGL v" EGL_VERSTR, (uInt) Q_strlen ("EGL v" EGL_VERSTR));

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

	Mem_Free (buffer);

	Com_Printf (0, "Wrote egl%.3d.jpg\n", i);
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
Img_LoadPCX
=============
*/
void Img_LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height) {
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

	//
	// load the file
	//
	fileLen = FS_LoadFile (filename, (void **)&raw);
	if (!raw || (fileLen <= 0))
		return;

	//
	// parse the PCX file
	//
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

	// sanity checks
	if ((pcx->manufacturer != 0x0a) || (pcx->version != 5) || (pcx->encoding != 1)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadPCX: Invalid PCX header: %s\n", filename);
		return;
	}

	if ((pcx->bitsPerPixel != 8) || (pcx->colorPlanes != 1)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadPCX: Only 8-bit PCX images are supported: %s\n", filename);
		return;
	}

	if ((pcx->xMax >= 640) || (pcx->xMax <= 0) || (pcx->yMax >= 480) || (pcx->yMax <= 0)) {
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadPCX: Bad PCX file dimensions: %s: %i x %i\n", filename, pcx->xMax, pcx->yMax);
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
		Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadPCX: PCX file %s was malformed", filename);
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
static int Img_LoadPNG (char *name, byte **pic, int *width, int *height) {
#ifdef OLDPNGCODE
	int				i, rowptr;
	int				samples;
	png_structp		png_ptr;
	png_infop		info_ptr;
	png_infop		end_info;
	byte			**row_pointers;
	byte			*img;
#else
	int				i, rowbytes;
	int				samples;
	png_structp		png_ptr;
	png_infop		info_ptr;
	png_infop		end_info;
	png_bytepp		row_pointers;
	png_bytep		pic_ptr;
#endif

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
		Com_Printf (0, S_COLOR_YELLOW "Img_LoadPNG: Not a PNG file: %s\n", name);
		goto freeFile;
	}

	PngFileBuffer.pos = 0;

	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,  NULL, NULL);

	if (!png_ptr) {
		Com_Printf (0, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
	}
	
	end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "Img_LoadPNG: Bad PNG file: %s\n", name);
		goto freeFile;
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
	samples = info_ptr->channels;

	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);

	FS_FreeFile (PngFileBuffer.buffer);
	return samples;
#else
	png_read_info (png_ptr, info_ptr);

	if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb (png_ptr);
		png_read_update_info (png_ptr, info_ptr);
	}

	if (info_ptr->color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler (png_ptr, 0xFF, PNG_FILLER_AFTER);

	if ((info_ptr->color_type == PNG_COLOR_TYPE_GRAY) && (info_ptr->bit_depth < 8))
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

	row_pointers = Mem_PoolAlloc (sizeof(png_bytep) * info_ptr->height, MEMPOOL_IMAGESYS, 0);

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

freeFile:
	FS_FreeFile (PngFileBuffer.buffer);
	return 0;
}


/*
================== 
Img_PNGScreenShot
================== 
*/
static uInt __stdcall png_write_thread (byte *buffer) {
	char		picname[MAX_OSPATH];
	char		checkname[MAX_OSPATH];
	int			i, k;
	FILE		*f;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep	*row_pointers;

	// create the scrnshots directory if it doesn't exist
	Q_snprintfz (checkname, sizeof (checkname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (checkname);

	for (i = 0 ; i<999 ; i++) {
		Q_snprintfz (picname, sizeof (picname), "%s/scrnshot/egl%.3d.png", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)	break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (0, S_COLOR_YELLOW "Img_PNGScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return 1;
	}

	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		Com_Printf (0, S_COLOR_YELLOW "LibPNG Error!\n", picname);
		return 1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		Com_Printf (0, S_COLOR_YELLOW "LibPNG Error!\n", picname);
		return 1;
	}

	png_init_io (png_ptr, f);

	png_set_IHDR (png_ptr, info_ptr, vidDef.width, vidDef.height, 8, PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_set_compression_level (png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_compression_mem_level (png_ptr, 9);

	png_write_info (png_ptr, info_ptr);

	row_pointers = Mem_PoolAlloc (vidDef.height * sizeof (png_bytep), MEMPOOL_IMAGESYS, 0);
	for (k=0 ; k<vidDef.height ; k++)
		row_pointers[k] = buffer + (vidDef.height - 1 - k) * 3 * vidDef.width;

	png_write_image (png_ptr, row_pointers);
	png_write_end (png_ptr, info_ptr);

	png_destroy_write_struct (&png_ptr, &info_ptr);

	fclose (f);

	Mem_Free (buffer);
	Mem_Free (row_pointers);

	Com_Printf (0, "Wrote egl%.3d.png\n", i);
	_endthread ();

	return 0;
}

static void Img_PNGScreenShot (void) {
	byte		*buffer;
	DWORD		tID;

	Com_Printf (0, "Taking PNG screenshot...\n");

	buffer = Mem_PoolAlloc (vidDef.width * vidDef.height * 3, MEMPOOL_IMAGESYS, 0);
	qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	_beginthreadex (NULL, 0, (uInt (__stdcall *)(void *))png_write_thread, (void *)buffer, 0, &tID);
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
Img_LoadTGA
=============
*/
static int Img_LoadTGA (char *name, byte **pic, int *width, int *height) {
	int				i, columns, rows, row_inc, row, col;
	byte			*buf_p, *buffer, *pixbuf, *targa_rgba;
	int				length, samples, readpixelcount, pixelcount;
	byte			palette[256][4], red, green, blue, alpha;
	qBool			compressed;
	targaHeader_t	targa_header;

	*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer || (length <= 0))
		return 0;

	buf_p = buffer;
	targa_header.idLength = *buf_p++;
	targa_header.colorMapType = *buf_p++;
	targa_header.imageType = *buf_p++;

	targa_header.colorMapIndex = buf_p[0] + buf_p[1] * 256;
	buf_p+=2;
	targa_header.colorMapLength = buf_p[0] + buf_p[1] * 256;
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
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadTGA: Only 8 bit images supported for type 1 and 9");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targa_header.colorMapLength != 256) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadTGA: Only 8 bit colormaps are supported for type 1 and 9");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targa_header.colorMapIndex) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadTGA: colorMapIndex is not supported for type 1 and 9");
			FS_FreeFile (buffer);
			return 0;
		}
		if (targa_header.colorMapSize == 24) {
			for (i=0 ; i<targa_header.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = 255;
			}
		}
		else if (targa_header.colorMapSize == 32) {
			for (i=0 ; i<targa_header.colorMapLength ; i++) {
				palette[i][0] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][2] = *buf_p++;
				palette[i][3] = *buf_p++;
			}
		}
		else {
			Com_Printf (PRNT_DEV,S_COLOR_YELLOW "Img_LoadTGA: only 24 and 32 bit colormaps are supported for type 1 and 9");
			FS_FreeFile (buffer);
			return 0;
		}
	}
	else if ((targa_header.imageType == 2) || (targa_header.imageType == 10)) {
		// uncompressed or RLE compressed RGB
		if ((targa_header.pixelSize != 32) && (targa_header.pixelSize != 24)) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadTGA: Only 32 or 24 bit images supported for type 2 and 10");
			FS_FreeFile (buffer);
			return 0;
		}
	}
	else if ((targa_header.imageType == 3) || (targa_header.imageType == 11)) {
		// uncompressed greyscale
		if (targa_header.pixelSize != 8 ) {
			Com_Printf (PRNT_DEV, S_COLOR_YELLOW "Img_LoadTGA: Only 8 bit images supported for type 3 and 11");
			FS_FreeFile (buffer);
			return 0;
		}
	}

	columns = targa_header.width;
	if (width)
		*width = columns;

	rows = targa_header.height;
	if (height)
		*height = rows;

	targa_rgba = Mem_PoolAlloc (columns * rows * 4, MEMPOOL_IMAGESYS, 0);
	*pic = targa_rgba;

	// if bit 5 of attributes isn't set, the image has been stored from bottom to top
	if (targa_header.attributes & 0x20) {
		pixbuf = targa_rgba;
		row_inc = 0;
	}
	else {
		pixbuf = targa_rgba + (rows - 1) * columns * 4;
		row_inc = -columns * 4 * 2;
	}

	compressed = ((targa_header.imageType == 9) || (targa_header.imageType == 10) || (targa_header.imageType == 11));
	for (row=col=0, samples=3 ; row<rows ; ) {
		pixelcount = 0x10000;
		readpixelcount = 0x10000;

		if (compressed) {
			pixelcount = *buf_p++;
			if (pixelcount & 0x80)	// run-length packet
				readpixelcount = 1;
			pixelcount = 1 + (pixelcount & 0x7f);
		}

		while (pixelcount-- && (row < rows)) {
			if (readpixelcount-- > 0) {
				switch (targa_header.imageType) {
				case 1:
				case 9:
					// colormapped image
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
					if (targa_header.pixelSize == 32) {
						alpha = *buf_p++;
						if (alpha != 255)
							samples = 4;
					}
					break;
				case 3:
				case 11:
					// greyscale image
					blue = green = red = *buf_p++;
					alpha = 255;
					break;
				}
			}

			*pixbuf++ = red;
			*pixbuf++ = green;
			*pixbuf++ = blue;
			*pixbuf++ = alpha;
			if (++col == columns) { // run spans across rows
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
Img_TGAScreenShot
================== 
*/
static void Img_TGAScreenShot (void) {
	byte		*buffer;
	char		picname[MAX_OSPATH];
	int			i, j, c, temp;
	FILE		*f;

	Com_Printf (0, "Taking TGA screenshot...\n");

	// create the scrnshots directory if it doesn't exist
	Q_snprintfz (picname, sizeof (picname), "%s/scrnshot", FS_Gamedir ());
	Sys_Mkdir (picname);

	// find a file name to save it to
	for (i=0 ; i<=999 ; i++) {
		Q_snprintfz (picname, sizeof (picname), "%s/scrnshot/egl%.3d.tga", FS_Gamedir (), i);
		f = fopen (picname, "rb");
		if (!f)
			break;
		fclose (f);
	}

	f = fopen (picname, "wb");
	if ((i == 1000) || !f) {
		Com_Printf (0, S_COLOR_YELLOW "Img_TGAScreenShot: Couldn't create a file\n"); 
		fclose (f);
		return;
	}

	c = (vidDef.width*vidDef.height*3) + 18;

	buffer = Mem_PoolAlloc (c, MEMPOOL_IMAGESYS, 0);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vidDef.width & 255;
	buffer[13] = vidDef.width >> 8;
	buffer[14] = vidDef.height & 255;
	buffer[15] = vidDef.height >> 8;
	buffer[16] = 24;	// pixel size

	if (glConfig.extBGRA && gl_ext_bgra->integer) {
		qglReadPixels (0, 0, vidDef.width, vidDef.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer + 18);
	}
	else {
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

	Mem_Free (buffer);
	Com_Printf (0, "Wrote egl%.3d.tga\n", i);
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
Img_LoadWal
================
*/
static image_t *Img_LoadWal (char *name, int flags) {
	walTex_t	*mt;
	byte		*buffer, *out[6];
	image_t		*image;

	//
	// load the file
	//
	FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
		return NULL;

	//
	// parse the WAL file
	//
	mt = (walTex_t *)buffer;

	mt->width = LittleLong (mt->width);
	mt->height = LittleLong (mt->height);
	mt->offsets[0] = LittleLong (mt->offsets[0]);

	if ((mt->width <= 0) || (mt->height <= 0)) {
		Com_Printf (PRNT_DEV, "Img_LoadWal: bad WAL file 's' (%i x %i)\n", name, mt->width, mt->height);
		goto freeFile;
	}

	out[0] = (byte *)mt + mt->offsets[0];
	image = Img_LoadPic (name, out, mt->width, mt->height, flags, 3, qTrue);

freeFile:
	FS_FreeFile ((void *)buffer);
	return image;
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
static void Img_ResampleTexture (uInt *in, int inWidth, int inHeight, uInt *out, int outWidth, int outHeight) {
	int		i, j;
	uInt	*inrow, *inrow2;
	uInt	frac, fracstep;
	uInt	*p1, *p2;
	byte	*pix1, *pix2, *pix3, *pix4;

	imgResamp = Mem_PoolAlloc (outWidth * outHeight * sizeof (uInt), MEMPOOL_IMAGESYS, 0);
	p1 = imgResamp;
	p2 = imgResamp + outWidth;

	// resample
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

	Mem_Free (imgResamp);
	imgResamp = NULL;
}


/*
================
Img_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
static void Img_LightScaleTexture (uInt *in, int inWidth, int inHeight, int samples, qBool useIntensity) {
	int		i, c;
	byte	*out;

	out = (byte *)in;
	c = inWidth * inHeight;

	if (useIntensity) {
		for (i=0 ; i<c ; i++, out+=4) {
			out[0] = imgGammaTable[imgIntensityTable[out[0]]];
			out[1] = imgGammaTable[imgIntensityTable[out[1]]];
			out[2] = imgGammaTable[imgIntensityTable[out[2]]];
		}
	}
	else {
		for (i=0 ; i<c ; i++, out+=4) {
			out[0] = imgGammaTable[out[0]];
			out[1] = imgGammaTable[out[1]];
			out[2] = imgGammaTable[out[2]];
		}
	}
}


/*
================
Img_MipMap

Operates in place, quartering the size of the texture
================
*/
static void Img_MipMap (byte *in, int width, int height) {
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
Img_Upload32
===============
*/
static void Img_Upload32 (char *name, uInt **data, int width, int height, int samples, image_t *image) {
	uInt		*scaled;
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
	qBool		mipMap;
	qBool		resampled;

	// determine transparency
	switch (samples) {
	case 4:
		isTrans = qTrue;
		break;

	case 3:
		isTrans = qFalse;
		break;

	default:
		Com_Printf (0, S_COLOR_RED "Img_Upload32: Invalid image sample count '%d' on '%s'\n", samples, name);
		samples = 3;
		break;
	}

	// find next highest power of two
	for (scaledWidth=1 ; scaledWidth<width ; scaledWidth<<=1) ;
	for (scaledHeight=1 ; scaledHeight<height ; scaledHeight<<=1) ;

	// mipmap?
	if (image && (image->upFlags & IF_NOMIPMAP))
		mipMap = qFalse;
	else
		mipMap = qTrue;

	// let people sample down the world textures for speed
	if (!(image && image->flags & IF_NOPICMIP)) {
		if (gl_picmip->integer > 0) {
			scaledWidth >>= gl_picmip->integer;
			scaledHeight >>= gl_picmip->integer;
		}
	}

	// clamp dimensions and set targets
	if (image && image->flags & IT_CUBEMAP) {
		times = 6;
		tTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		pTarget = GL_TEXTURE_CUBE_MAP_ARB;

		// check size
		scaledWidth = clamp (scaledWidth, 1, glConfig.maxCMTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxCMTexSize);
	}
	else {
		times = 1;
		tTarget = GL_TEXTURE_2D;
		pTarget = GL_TEXTURE_2D;

		// check size
		scaledWidth = clamp (scaledWidth, 1, glConfig.maxTexSize);
		scaledHeight = clamp (scaledHeight, 1, glConfig.maxTexSize);
	}

	// texture compression
	if (glConfig.extArbTexCompression && !(image && image->flags & IF_NOCOMPRESS)) {
		if (isTrans)	format = GL_COMPRESSED_RGBA_ARB;
		else			format = GL_COMPRESSED_RGB_ARB;
	}
	else {
		if (isTrans)	format = glState.texRGBAFormat;
		else			format = glState.texRGBFormat;
	}

	// set upload values
	upFlags = image ? image->flags : 0;
	upWidth = scaledWidth;
	upHeight = scaledHeight;

	// alter uploaded flags
	if (!mipMap) {
		if (!vid_gammapics->integer)
			upFlags |= IF_NOGAMMA;

		upFlags |= IF_NOMIPMAP;
	}

	if (upFlags & IT_CUBEMAP)
		upFlags |= IF_CLAMP;

	// tex params
	if (mipMap) {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		if (glConfig.extTexFilterAniso)
			qglTexParameterf (pTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp (gl_ext_max_anisotropy->integer, 1, glConfig.maxAniso));
	}
	else {
		if (glConfig.extSGISGenMipmap)
			qglTexParameteri (pTarget, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
	}

	qglTexParameterf (pTarget, GL_TEXTURE_MIN_FILTER, mipMap ? glState.texMinFilter : glState.texMagFilter);
	qglTexParameterf (pTarget, GL_TEXTURE_MAG_FILTER, glState.texMagFilter);

	// texture edge clamping
	if (image && (image->flags & (IT_CUBEMAP|IF_CLAMP))) {
		if (glConfig.extTexEdgeClamp) {
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if (image->flags & IT_CUBEMAP)
				qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}
		else {
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
			if (image->flags & IT_CUBEMAP)
				qglTexParameterf (pTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
		}
	}
	else {
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (pTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// upload
	for (i=0 ; i<times ; i++) {
		scaledWidth = upWidth;
		scaledHeight = upHeight;

		if ((scaledWidth == width) && (scaledHeight == height)) {
			// image is a power of two
			scaled = data[i];
			resampled = qFalse;
		}
		else {
			// not a power of two, must resample
			scaled = Mem_PoolAlloc (scaledWidth * scaledHeight * 4, MEMPOOL_IMAGESYS, 0);
			if (!scaled)
				Com_Printf (0, S_COLOR_RED "Img_Upload32: Out of memory!\n");

			Img_ResampleTexture (data[i], width, height, scaled, scaledWidth, scaledHeight);
			resampled = qTrue;
		}

		// apply image gamma/intensity
		if (!(upFlags & IF_NOGAMMA))
			Img_LightScaleTexture (scaled, scaledWidth, scaledHeight, samples, mipMap && !(upFlags & IF_NOINTENS));

		// upload the base image
		qglTexImage2D (tTarget + i, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

		// upload mipmap levels
		if (mipMap && !glConfig.extSGISGenMipmap) {
			int		mipLevel = 0;

			while ((scaledWidth > 1) || (scaledHeight > 1)) {
				Img_MipMap ((byte *)scaled, scaledWidth, scaledHeight);

				scaledWidth >>= 1;
				scaledHeight >>= 1;
				if (scaledWidth < 1)
					scaledWidth = 1;
				if (scaledHeight < 1)
					scaledHeight = 1;

				qglTexImage2D (tTarget + i, ++mipLevel, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}

		if (scaled && resampled) {
			Mem_Free (scaled);
			scaled = NULL;
		}
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
===============
Img_Upload8

Converts a paletted image to standard before passing off for upload
===============
*/
static void Img_Upload8 (char *name, byte *data, int width, int height, image_t *image) {
	uInt	*trans;
	int		i, s, pxl;
	int		samples;

	s = width * height;
	trans = Mem_PoolAlloc (s * 4, MEMPOOL_IMAGESYS, 0);

	samples = 3;
	for (i=0 ; i<s ; i++) {
		pxl = data[i];
		trans[i] = imgPaletteTable[pxl];

		if (pxl == 0xff) {
			samples = 4;

			// transparent, so scan around for another color to avoid alpha fringes
			if ((i > width) && (data[i-width] != 255))
				pxl = data[i-width];
			else if ((i < s-width) && (data[i+width] != 255))
				pxl = data[i+width];
			else if ((i > 0) && (data[i-1] != 255))
				pxl = data[i-1];
			else if ((i < s-1) && (data[i+1] != 255))
				pxl = data[i+1];
			else
				pxl = 0;

			// copy rgb components
			((byte *)&trans[i])[0] = ((byte *)&imgPaletteTable[pxl])[0];
			((byte *)&trans[i])[1] = ((byte *)&imgPaletteTable[pxl])[1];
			((byte *)&trans[i])[2] = ((byte *)&imgPaletteTable[pxl])[2];
		}
	}

	Img_Upload32 (name, &trans, width, height, samples, image);
	Mem_Free (trans);
}

/*
==============================================================================

	IMAGE LOADING

==============================================================================
*/

static char	*imgBareName;

/*
================
Img_FindSlot
================
*/
static inline image_t *Img_FindSlot (void) {
	int			i;
	image_t		*image = NULL;

	// find a free imgTextures spot
	for (i=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (!image->iRegistrationFrame) {
			image->texNum = i + 1;
			//orlyqglGenTextures (1, &image->texNum);
			break;
		}
	}

	// none found, create a new spot
	if (i == imgNumTextures) {
		if (imgNumTextures >= MAX_IMAGES)
			Com_Error (ERR_DROP, "Img_FindSlot: MAX_IMAGES");

		image = &imgTextures[imgNumTextures++];
		//orlyqglGenTextures (1, &image->texNum);
		image->texNum = imgNumTextures;
	}

	return image;
}


/*
================
Img_FindImage
================
*/
static inline image_t *Img_FindImage (const char *name, int flags) {
	image_t	*image;
	uLong	hash;
	int		len;

	// check the length
	len = Q_strlen (name);
	if (len < 5) {
		Com_Printf (0, S_COLOR_RED "Img_RegisterImage: Image name too short! %s\n", name);
		return NULL;
	}
	if (len >= MAX_QPATH) {
		Com_Printf (0, S_COLOR_RED "Img_RegisterImage: Image name too long! %s\n", name);
		return NULL;
	}

	// fix/strip barename
	imgBareName = Q_FixPathName (name, qTrue);

	// calculate hash
	hash = CalcHash (imgBareName, MAX_IMAGE_HASH);

	// look for it
	if (flags == 0) {
		for (image=imgHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->iRegistrationFrame)
				continue;

			// hash optimized
			if (!strcmp (imgBareName, image->bareName)) {
				// bump and return
				image->iRegistrationFrame = imgRegistrationFrame;
				return image;
			}
		}
	}
	else {
		for (image=imgHashTree[hash] ; image ; image=image->hashNext) {
			if (!image->iRegistrationFrame)
				continue;

			// hash optimized
			if (((image->flags == flags) || (image->upFlags == flags))
			&& !strcmp (imgBareName, image->bareName)) {
				// bump and return
				image->iRegistrationFrame = imgRegistrationFrame;
				return image;
			}
		}
	}

	return NULL;
}


/*
================
Img_LoadPic

This is also used as an entry point for the generated r_NoTexture
================
*/
image_t *Img_LoadPic (char *name, byte **pic, int width, int height, int flags, int samples, qBool upload8) {
	image_t		*image;

	// get a free image spot
	image = Img_FindSlot ();

	if (Q_strlen (name) >= sizeof (image->name))
		Com_Error (ERR_DROP, "Img_LoadPic: \"%s\" is too long", name);

	Q_strncpyz (image->name, name, sizeof (image->name));
	if (imgBareName) {
		Q_strncpyz (image->bareName, imgBareName, sizeof (image->bareName));
		imgBareName = NULL;
	}
	else
		Q_strncpyz (image->bareName, Q_FixPathName (name, qTrue), sizeof (image->bareName));

	image->width = width;
	image->height = height;

	// texture scaling, hacky special case!
	if (!upload8 && (flags & IT_TEXTURE) && !!(strcmp (name + Q_strlen (name) - 4, ".wal"))) {
		char		newName[MAX_QPATH];
		walTex_t	*mt;

		Q_snprintfz (newName, sizeof (newName), "%s.wal", image->bareName);
		FS_LoadFile (newName, (void **)&mt);
		if (mt) {
			image->width = LittleLong (mt->width);
			image->height = LittleLong (mt->height);
			
			FS_FreeFile (mt);
		}
	}

	// reset
	image->upFlags = image->flags = flags;
	image->iRegistrationFrame = imgRegistrationFrame;
	image->upWidth = width;
	image->upHeight = height;
	image->format = 0;
	image->hashValue = CalcHash (image->bareName, MAX_IMAGE_HASH);

	// upload
	GL_Bind (image);
	if (upload8)
		Img_Upload8 (name, *pic, width, height, image);
	else
		Img_Upload32 (name, (uInt **)pic, width, height, samples, image);

	// link it in
	image->hashNext = imgHashTree[image->hashValue];
	imgHashTree[image->hashValue] = image;

	return image;
}


/*
===============
Img_RegisterCubeMap

Finds or loads the given cubemap image
Static because Img_RegisterImage uses it if it's passed the IT_CUBEMAP flag
===============
*/
static inline image_t *Img_RegisterCubeMap (char *name, int flags) {
	image_t		*image;
	int			i, j;
	int			samples;
	byte		*pic[6];
	int			width, height;
	int			firstSize, firstSamples;
	char		loadName[MAX_QPATH];

	// make sure we have this
	flags |= IT_CUBEMAP;

	// check the name
	if (!name || !name[0])
		return NULL;

	// see if it's already loaded
	image = Img_FindImage (name, flags);
	if (image)
		return image;

	// not found -- load the pic from disk
	for (i=0 ; i<6 ; i++) {
		pic[i] = NULL;

		// PNG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.png", imgBareName, cubeMap_NameSuffix[i]);
		samples = Img_LoadPNG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// TGA
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.tga", imgBareName, cubeMap_NameSuffix[i]);
		samples = Img_LoadTGA (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;

		// JPG
		Q_snprintfz (loadName, sizeof (loadName), "%s_%s.jpg", imgBareName, cubeMap_NameSuffix[i]);
		samples = Img_LoadJPG (loadName, &pic[i], &width, &height);
		if (pic[i])
			goto checkPic;
		break;

checkPic:
		if (width != height) {
			Com_Printf (0, S_COLOR_YELLOW "Img_RegisterCubeMap: %s is not square!\n", loadName);
			goto emptyTrash;
		}

		if (!i) {
			firstSize = width;
			firstSamples = samples;
		}
		else {
			if (firstSize != width) {
				Com_Printf (0, S_COLOR_YELLOW "Img_RegisterCubeMap: Size mismatch with previous in %s! Halting!\n", loadName);
				break;
			}

			if (firstSamples != samples) {
				Com_Printf (0, S_COLOR_YELLOW "Img_RegisterCubeMap: Sample mismatch with previous in %s! Halting!\n", loadName);
				break;
			}
		}
	}

	if (i != 6) {
		Com_Printf (0, S_COLOR_YELLOW "Img_RegisterCubeMap: Unable to find all of the sides, aborting!\n");
		goto emptyTrash;
	}
	else
		image = Img_LoadPic (loadName, pic, width, height, flags, samples, qFalse);

emptyTrash:
	for (j=0 ; j<6 ; j++) {
		if (pic[j])
			Mem_Free (pic[j]);
	}

	return image;
}


/*
===============
Img_RegisterImage

Finds or loads the given image
===============
*/
image_t	*Img_RegisterImage (char *name, int flags) {
	image_t		*image;
	byte		*pic;
	int			width, height, samples;
	char		loadName[MAX_QPATH];
	qBool		upload8;

	// cubemap stuff
	if ((flags & IT_CUBEMAP) && glConfig.extArbTexCubeMap)
		return Img_RegisterCubeMap (name, flags);
	flags &= ~IT_CUBEMAP;

	// check the name
	if (!name || !name[0])
		return NULL;

	// see if it's already loaded
	image = Img_FindImage (name, flags);
	if (image)
		return image;

	// not found -- load the pic from disk
	upload8 = qFalse;

	// PNG
	Q_snprintfz (loadName, sizeof (loadName), "%s.png", imgBareName);
	samples = Img_LoadPNG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// TGA
	Q_snprintfz (loadName, sizeof (loadName), "%s.tga", imgBareName);
	samples = Img_LoadTGA (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	// JPG
	Q_snprintfz (loadName, sizeof (loadName), "%s.jpg", imgBareName);
	samples = Img_LoadJPG (loadName, &pic, &width, &height);
	if (pic && samples)
		goto loadImage;

	pic = NULL;
	upload8 = qTrue;
	samples = 3;
	if (!(strcmp (name + Q_strlen (name) - 4, ".wal"))) {
		// WAL
		Q_snprintfz (loadName, sizeof (loadName), "%s.wal", imgBareName);
		image = Img_LoadWal (loadName, flags);
		if (image)
			goto emptyTrash;
	}
	else {
		// PCX
		Q_snprintfz (loadName, sizeof (loadName), "%s.pcx", imgBareName);
		Img_LoadPCX (loadName, &pic, NULL, &width, &height);
		if (pic)
			goto loadImage;
	}

	// nothing found!
	image = NULL;
	goto emptyTrash;

loadImage:
	image = Img_LoadPic (loadName, &pic, width, height, flags, samples, upload8);

emptyTrash:
	if (pic)
		Mem_Free (pic);

	return image;
}


/*
================
Img_BeginRegistration
================
*/
void Img_BeginRegistration (void) {
	imgRegistrationFrame++;
}


/*
================
Img_EndRegistration

Any image that was not touched on this registration sequence will be freed
================
*/
void Img_EndRegistration (void) {
	image_t	*hashImg;
	image_t	**prev;
	image_t	*image;
	int		i;

	for (i=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (image->iRegistrationFrame == imgRegistrationFrame)
			continue;		// used this sequence
		if (!image->iRegistrationFrame)
			continue;		// free image_t slot
		if (image->flags & (IF_NOFLUSH|IF_NOMIPMAP)) {
			image->iRegistrationFrame = imgRegistrationFrame;
			continue;		// don't free
		}

		// de-link it
		prev = &imgHashTree[image->hashValue];
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

		// free it
		qglDeleteTextures (1, &image->texNum);
		memset (image, 0, sizeof (*image));
	}
}


/*
===============
Img_GetSize
===============
*/
void Img_GetSize (image_t *image, int *width, int *height) {
	if (!image) {
		if (width)
			*width = 0;
		if (height)
			*height = 0;
	}
	else {
		if (width)
			*width = image->width;
		if (height)
			*height = image->height;
	}
}

/*
==============================================================================

	CONSOLE FUNCTIONS

==============================================================================
*/

/*
===============
Img_ImageList_f
===============
*/
static void Img_ImageList_f (void) {
	uInt		i, imgs, mips;
	image_t		*image;
	uInt		tempWidth, tempHeight;
	uInt		texels;

	Com_Printf (0, "-----------------------------------------------------\n");
	Com_Printf (0, "Tex# T  Format GIMC Width Height Name\n");
	Com_Printf (0, "---- -- ------ ---- ----- ------ --------------------\n");

	for (i=0, imgs=0, mips=0, texels=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;		// free imgTextures slot

		Com_Printf (0, "%4d ", image->texNum);

		if (image->upFlags & IT_CUBEMAP)		Com_Printf (0, "CM ");
		else if (image->upFlags & IF_NOMIPMAP)	Com_Printf (0, "2D ");
		else									Com_Printf (0, "   ");

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

		Com_Printf (0, "%s", (image->upFlags & IF_NOGAMMA)	? "-" : "G");
		Com_Printf (0, "%s", (image->upFlags & IF_NOINTENS)	? "-" : "I");
		Com_Printf (0, "%s", (image->upFlags & IF_NOMIPMAP)	? "-" : "M");
		Com_Printf (0, "%s", (image->upFlags & IF_CLAMP)	? "C" : "-");

		Com_Printf (0, " %5i  %5i %s\n", image->upWidth, image->upHeight, image->name);

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

	Com_Printf (0, "-----------------------------------------------------\n");
	Com_Printf (0, "Total images: %d (with mips: %d)\n", imgs, imgs + mips);
	Com_Printf (0, "Texel count (including mips): %d\n", texels);
	Com_Printf (0, "-----------------------------------------------------\n");
}


/*
================== 
Img_ScreenShot_f
================== 
*/
static void Img_ScreenShot_f (void) {
	if (!Q_stricmp (Cmd_Argv (1), "tga"))
		Img_TGAScreenShot ();
	else if (!Q_stricmp (Cmd_Argv (1), "png"))
		Img_PNGScreenShot ();
	else if (!Q_stricmp (Cmd_Argv (1), "jpg"))
		Img_JPGScreenShot ();
	else {
		if (!Q_stricmp (gl_screenshot->string, "png"))
			Img_PNGScreenShot ();
		else if (!Q_stricmp (gl_screenshot->string, "jpg"))
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
static void Img_InitSpecialTextures (void) {
	int		x, y;
	byte	*data;

	/*
	** r_NoTexture
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

	memset (&r_NoTexture, 0, sizeof (r_NoTexture));
	r_NoTexture = Img_LoadPic ("***r_NoTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3, qFalse);

	/*
	** r_WhiteTexture
	*/
	memset (data, 255, 8 * 8 * 4);
	memset (&r_WhiteTexture, 0, sizeof (r_WhiteTexture));
	r_WhiteTexture = Img_LoadPic ("***r_WhiteTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3, qFalse);

	/*
	** r_BlackTexture
	*/
	memset (data, 0, 8 * 8 * 4);
	memset (&r_BlackTexture, 0, sizeof (r_BlackTexture));
	r_BlackTexture = Img_LoadPic ("***r_BlackTexture***", &data, 8, 8, IF_NOFLUSH|IF_NOGAMMA|IF_NOINTENS, 3, qFalse);

	Mem_Free (data);

	/*
	** r_CinTexture
	** Reserve a texNum for cinematics
	*/
	data = Mem_PoolAlloc (256 * 256 * 4, MEMPOOL_IMAGESYS, 0);
	memset (data, 0, 256 * 256 * 4);
	memset (&r_CinTexture, 0, sizeof (r_CinTexture));
	r_CinTexture = Img_LoadPic ("***r_CinTexture***", &data, 256, 256, IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP|IF_NOINTENS|IF_NOGAMMA, 3, qFalse);
	Mem_Free (data);

	/*
	** r_BloomTexture
	** For full-scene bloom effect
	*/
	data = Mem_PoolAlloc (32 * 32 * 4, MEMPOOL_IMAGESYS, 0);
	memset (data, 0, 32 * 32 * 4);
	memset (&r_BloomTexture, 0, sizeof (r_BloomTexture));
	r_BloomTexture = Img_LoadPic ("***r_BloomTexture***", &data, 32, 32, IF_NOMIPMAP|IF_NOPICMIP|IF_NOINTENS|IF_NOGAMMA, 3, qFalse);
	Mem_Free (data);
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
	gl_picmip			= Cvar_Get ("gl_picmip",			"0",						CVAR_LATCH_VIDEO);
	gl_screenshot		= Cvar_Get ("gl_screenshot",		"tga",						CVAR_ARCHIVE);

	gl_texturebits		= Cvar_Get ("gl_texturebits",		"default",					CVAR_ARCHIVE|CVAR_LATCH_VIDEO);
	gl_texturemode		= Cvar_Get ("gl_texturemode",		"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE);

	vid_gamma			= Cvar_Get ("vid_gamma",			"1.0",						CVAR_ARCHIVE);
	vid_gammapics		= Cvar_Get ("vid_gammapics",		"0",						CVAR_ARCHIVE|CVAR_LATCH_VIDEO);

	Cmd_AddCommand ("imagelist",	Img_ImageList_f,		"Prints out a list of the currently loaded textures");
	Cmd_AddCommand ("screenshot",	Img_ScreenShot_f,		"Takes a screenshot");
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
	int		i, j;
	float	gam;
	int		red, green, blue;
	uInt	v;
	byte	*pal;

	Com_Printf (0, "\n--------- Image Initialization ---------\n");

	Img_Register ();

	imgNumTextures = 0;
	imgRegistrationFrame = 1;

	// set the initial state
	glState.texUnit = 0;
	VectorSet (glState.texNums, 0, 0, 0);
	VectorSet (glState.texEnvModes, 0, 0, 0);

	GL_TextureMode (qTrue);
	GL_TextureBits (qTrue);

	GL_SelectTexture (0);
	GL_TexEnv (GL_MODULATE);

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

	// get the palette
	Img_LoadPCX ("pics/colormap.pcx", NULL, &pal, NULL, NULL);
	if (!pal)
		pal = img_DefaultPal;

	for (i=0 ; i<256 ; i++) {
		red = pal[i*3+0];
		green = pal[i*3+1];
		blue = pal[i*3+2];
		
		v = (255<<24) + (red<<0) + (green<<8) + (blue<<16);
		imgPaletteTable[i] = LittleLong (v);
	}

	imgPaletteTable[255] &= LittleLong (0xffffff);	// 255 is transparent

	if (pal != img_DefaultPal)
		Mem_Free (pal);

	// set up the gamma and intensity ramps
	if (intensity->value < 1)
		Cvar_VariableForceSetValue (intensity, 1);

	glState.invIntens = 1.0 / intensity->value;

	// hack! because voodoo's are nasty bright
	if (glConfig.rendType & GLREND_VOODOO)
		gam = 1.0f;
	else
		gam = vid_gamma->value;

	// gamma
	if (gam == 1) {
		for (i=0 ; i<256 ; i++)
			imgGammaTable[i] = i;
	}
	else {
		for (i=0 ; i<256 ; i++)
			imgGammaTable[i] = clamp (255 * pow ((i + 0.5)*0.0039138943248532289628180039138943 , gam) + 0.5, 0, 255);
	}

	// intensity (eww)
	for (i=0 ; i<256 ; i++) {
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		imgIntensityTable[i] = j;
	}

	// load up special textures
	Img_InitSpecialTextures ();

	// check memory integrity
	Mem_CheckPoolIntegrity (MEMPOOL_IMAGESYS);

	Com_Printf (0, "----------------------------------------\n");
}


/*
===============
Img_Shutdown
===============
*/
void Img_Shutdown (qBool full) {
	int		i;
	image_t	*image;

	if (!full) {
		// unregister commands
		Img_Unregister ();
	}

	// free loaded textures
	for (i=0, image=imgTextures ; i<imgNumTextures ; i++, image++) {
		if (!image->iRegistrationFrame)
			continue;	// free imgTextures slot

		// free it
		qglDeleteTextures (1, &image->texNum);
	}

	imgNumTextures = 0;
	imgBareName = NULL;
	memset (imgTextures, 0, sizeof (imgTextures));
	memset (imgHashTree, 0, sizeof (imgHashTree));

	// free memory
	Mem_FreePool (MEMPOOL_IMAGESYS);
	imgResamp = NULL;
}
