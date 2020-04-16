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
// rf_image.h
//

/*
=============================================================================

	IMAGING

=============================================================================
*/

enum { // texUnit_t
	TEXUNIT0,
	TEXUNIT1,
	TEXUNIT2,
	TEXUNIT3,
	TEXUNIT4,
	TEXUNIT5,
	TEXUNIT6,
	TEXUNIT7,

	MAX_TEXUNITS
};

enum { // texFlags_t
	IT_3D				= 1 << 0,		// 3d texture
	IT_CUBEMAP			= 1 << 1,		// it's a cubemap env base image
	IT_LIGHTMAP			= 1 << 2,		// lightmap texture

	IF_CLAMP			= 1 << 3,		// texcoords edge clamped
	IF_NOCOMPRESS		= 1 << 4,		// no texture compression
	IF_NOGAMMA			= 1 << 5,		// not affected by vid_gama
	IF_NOINTENS			= 1 << 6,		// not affected by intensity
	IF_NOMIPMAP_LINEAR	= 1 << 7,		// not mipmapped, linear filtering
	IF_NOMIPMAP_NEAREST	= 1 << 8,		// not mipmapped, nearest filtering
	IF_NOPICMIP			= 1 << 9,		// not affected by gl_picmip
	IF_NOFLUSH			= 1 << 10,		// do not flush at the end of registration (internal only)
	IF_NOALPHA			= 1 << 11,		// force alpha to 255
	IF_NORGB			= 1 << 12,		// force rgb to 255 255 255
};

typedef struct image_s {
	char					name[MAX_QPATH];				// game path, including extension
	char					bareName[MAX_QPATH];			// filename only, as called when searching

	int						width,		upWidth;			// source image
	int						height,		upHeight;			// after power of two and picmip
	int						depth,		upDepth;			// for 3d textures
	texFlags_t				flags;

	int						tcWidth,	tcHeight;			// special case for high-res texture scaling

	int						target;							// destination for binding
	int						format;							// uploaded texture color components

	uint32					touchFrame;						// free if this doesn't match the current frame
	uint32					texNum;							// gl texture binding, r_numImages + 1, can't be 0
	uint32					visFrame;						// frame this texture was last used in

	uint32					hashValue;						// calculated hash value
	struct image_s			*hashNext;						// hash image tree
} image_t;

#define MAX_IMAGES			1024			// maximum local images

#define FOGTEX_WIDTH		256
#define FOGTEX_HEIGHT		32

#define Q2LIGHTMAP_WIDTH	256				// lightmaps are square
#define R_MAX_LIGHTMAPS		128				// maximum local lightmap textures
extern image_t		*r_lmTextures[R_MAX_LIGHTMAPS];

extern const char	*r_skyNameSuffix[6];
extern const char	*r_cubeMapSuffix[6];

//
// rf_image.c
//

void	GL_TextureBits (qBool verbose, qBool verboseOnly);
void	GL_TextureMode (qBool verbose, qBool verboseOnly);

void	GL_ResetAnisotropy (void);

#define R_Load2DImage(name,pic,width,height,flags,samples)			R_LoadImage((name),NULL,(pic),(width),(height),1,(flags),(samples),qFalse,qFalse)
#define R_Load3DImage(name,pic,width,height,depth,flags,samples)	R_LoadImage((name),NULL,(pic),(width),(height),(depth),(flags),(samples),qFalse,qFalse)

image_t	*R_LoadImage (char *name, const char *bareName, byte **pic, int width, int height, int depth, texFlags_t flags, int samples, qBool upload8, qBool isPCX);

#define R_TouchImage(img) ((img)->touchFrame = ri.reg.registerFrame, ri.reg.imagesTouched++)

image_t	*R_RegisterImage (char *name, texFlags_t flags);

void	R_BeginImageRegistration (void);
void	R_EndImageRegistration (void);

void	R_UpdateGammaRamp (void);

void	R_ImageInit (void);
void	R_ImageShutdown (void);
