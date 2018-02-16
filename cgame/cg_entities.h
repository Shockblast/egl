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
// cg_entities.h
//

/*
=============================================================================

	ENTITY

=============================================================================
*/

typedef struct cgEntity_s {
	entityState_t	baseLine;		// delta from this if not from a previous frame
	entityState_t	current;
	entityState_t	prev;			// will always be valid, but might just be a copy of current

	int				serverFrame;		// if not current, this ent isn't in the frame

	vec3_t			lerpOrigin;		// for trails (variable hz)

	int				flyStopTime;

	qBool			muzzleOn;
	int				muzzType;
	qBool			muzzSilenced;
	qBool			muzzVWeap;
} cgEntity_t;

extern cgEntity_t		cg_entityList[MAX_CS_EDICTS];

// the cg_parseEntities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
extern entityState_t	cg_parseEntities[MAX_PARSE_ENTITIES];

//
// cg_entities.c
//

void		CG_BeginFrameSequence (frame_t frame);
void		CG_NewPacketEntityState (int entNum, entityState_t state);
void		CG_EndFrameSequence (int numEntities);

void		CG_AddEntities (void);
void		CG_ClearEntities (void);

void		CG_GetEntitySoundOrigin (int entNum, vec3_t origin, vec3_t velocity);

//
// cg_localents.c
//

typedef enum leType_s {
	LE_MGSHELL,
	LE_SGSHELL,
} leType_t;

qBool		CG_SpawnLocalEnt (float org0,					float org1,						float org2,
							float vel0,						float vel1,						float vel2,
							float angle0,					float angle1,					float angle2,
							float avel0,					float avel1,					float avel2,
							float parm0,					float parm1,					float parm2,
							leType_t type);

void		CG_ClearLocalEnts (void);
void		CG_AddLocalEnts (void);

//
// cg_tempents.c
//

void		CG_ExploRattle (vec3_t org, float scale);

void		CG_AddTempEnts (void);
void		CG_ClearTempEnts (void);
void		CG_ParseTempEnt (void);

//
// cg_weapon.c
//

void		CG_AddViewWeapon (void);

void		CG_WeapRegister (void);
void		CG_WeapUnregister (void);
