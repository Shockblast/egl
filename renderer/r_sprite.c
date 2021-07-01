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
// r_sprite.c
// Sp2 sprite model rendering
//

#include "r_local.h"

/*
=================
R_DrawSpriteModel
=================
*/
void R_DrawSpriteModel (entity_t *ent) {
	mSpriteModel_t	*spriteModel;
	mSpriteFrame_t	*spriteFrame;
	shader_t		*shader;
	shaderStage_t	*stage;
	float			scaleAlpha;
	vec3_t			point;

	spriteModel = ent->model->spriteModel;
	spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];

	// culling
	if (R_CullSphere (ent->origin, spriteFrame->radius))
		return;

	glSpeeds.numEntities++;

	scaleAlpha = ent->color[3];
	shader = spriteFrame->shader;

	// rendering
	R_LoadModelIdentity ();
	if (!shader || !r_shaders->integer) {
		if ((scaleAlpha != 1.0f) && (!(ent->flags & RF_TRANSLUCENT))) qglEnable (GL_BLEND);
		if (scaleAlpha == 1.0f) qglEnable (GL_ALPHA_TEST);
		qglColor4f (1, 1, 1, scaleAlpha);

		GL_Bind (0, spriteFrame->skin);

		qglBegin (GL_QUADS);

		qglTexCoord2f (0, 1);
		VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
		VectorMA (point, -spriteFrame->originX, v_Right, point);
		qglVertex3fv (point);

		qglTexCoord2f (0, 0);
		VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
		VectorMA (point, -spriteFrame->originX, v_Right, point);
		qglVertex3fv (point);

		qglTexCoord2f (1, 0);
		VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
		VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
		qglVertex3fv (point);

		qglTexCoord2f (1, 1);
		VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
		VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
		qglVertex3fv (point);

		qglEnd ();

		if ((scaleAlpha != 1.0f) && (!(ent->flags & RF_TRANSLUCENT))) qglDisable (GL_BLEND);
		if (scaleAlpha == 1.0f) qglDisable (GL_ALPHA_TEST);
	} else {
		vec3_t		shadelight, normal = { 0, 1, 0 };
		image_t		*stageImage;
		vec2_t		coords, scrollCoords;
		float		alpha;

		R_LightPoint (ent->origin, shadelight);

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
			RS_BlendFunc (stage, (ent->flags&RF_TRANSLUCENT)?qTrue:qFalse, (ent->flags&RF_TRANSLUCENT)?qFalse:qTrue);
			RS_AlphaShift (stage, &alpha);
			alpha = (alpha*scaleAlpha);

			if ((alpha < 1) && (!(ent->flags&RF_TRANSLUCENT)))
				qglDepthMask (GL_FALSE);

			if (stage->alphaMask)
				qglEnable (GL_ALPHA_TEST);

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

			if (stage->lightMap || stage->lightMapOnly)
				qglColor4f (shadelight[0], shadelight[1], shadelight[2], alpha);
			else
				qglColor4f (1, 1, 1, alpha);

			qglBegin (GL_QUADS);

			coords[0] = 0; coords[1] = 1;
			VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
			VectorMA (point, -spriteFrame->originX, v_Right, point);
			if (stage->envMap)
				RS_SetEnvmap (ent->origin, axisIdentity, point, normal, coords);
			RS_SetTexcoords2D (stage, coords);
			qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
			qglVertex3fv (point);

			coords[0] = 0; coords[1] = 0;
			VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
			VectorMA (point, -spriteFrame->originX, v_Right, point);
			if (stage->envMap)
				RS_SetEnvmap (ent->origin, axisIdentity, point, normal, coords);
			RS_SetTexcoords2D (stage, coords);
			qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
			qglVertex3fv (point);

			coords[0] = 1; coords[1] = 0;
			VectorMA (ent->origin, spriteFrame->height - spriteFrame->originY, v_Up, point);
			VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
			if (stage->envMap)
				RS_SetEnvmap (ent->origin, axisIdentity, point, normal, coords);
			RS_SetTexcoords2D (stage, coords);
			qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
			qglVertex3fv (point);

			coords[0] = 1; coords[1] = 1;
			VectorMA (ent->origin, -spriteFrame->originY, v_Up, point);
			VectorMA (point, spriteFrame->width - spriteFrame->originX, v_Right, point);
			if (stage->envMap)
				RS_SetEnvmap (ent->origin, axisIdentity, point, normal, coords);
			RS_SetTexcoords2D (stage, coords);
			qglTexCoord2f (coords[0]+scrollCoords[0], coords[1]+scrollCoords[1]);
			qglVertex3fv (point);

			qglEnd ();

			if (stage->cubeMap) {
				qglDisable (GL_TEXTURE_GEN_S);
				qglDisable (GL_TEXTURE_GEN_T);
				qglDisable (GL_TEXTURE_GEN_R);

				qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
			}

			if (stage->alphaMask)
				qglDisable (GL_ALPHA_TEST);

			if (!(ent->flags&RF_TRANSLUCENT)) {
				qglDisable (GL_BLEND);
				qglDepthMask (GL_TRUE);
			}

			glSpeeds.shaderPasses++;
			stage = stage->next;
		} while (stage);
		glSpeeds.shaderCount++;
	}

	GL_BlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
