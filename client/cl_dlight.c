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

// cl_dlight.c
// - dynamic lights

#include "cl_local.h"

/*
==============
CL_ParseMuzzleFlash
==============
*/
void CL_ParseMuzzleFlash (void) {
	vec3_t		fv, rv;
	cDLight_t	*dl;
	centity_t	*pl;
	int			ent, weapon;
	int			silenced;
	float		volume;
	char		soundname[64];

	ent = MSG_ReadShort (&net_Message);
	if ((ent < 1) || (ent >= MAX_EDICTS))
		Com_Error (ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte (&net_Message);
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_Entities[ent];

	dl = CL_AllocDLight (ent);
	VectorCopy (pl->current.origin, dl->origin);
	AngleVectors (pl->current.angles, fv, rv, NULL);
	VectorMA (dl->origin, 18, fv, dl->origin);
	VectorMA (dl->origin, 16, rv, dl->origin);

	pl->muzzleOn = qFalse;
	pl->muzzType = -1;
	pl->muzzVWeap = qFalse;
	if (silenced) {
		pl->muzzSilenced = qTrue;
		volume = 0.2;
		dl->radius = 100 + (frand () * 32);
	} else {
		pl->muzzSilenced = qFalse;
		volume = 1;
		dl->radius = 200 + (frand () * 32);
	}

	dl->minlight = 32;
	dl->die = cls.realTime;

	switch (weapon) {
	// MZ_BLASTER
	case MZ_BLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLASTER;
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_BLUEHYPERBLASTER
	case MZ_BLUEHYPERBLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLUEHYPERBLASTER;
		VectorSet (dl->color, 0, 0, 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_HYPERBLASTER
	case MZ_HYPERBLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_HYPERBLASTER;
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_MACHINEGUN
	case MZ_MACHINEGUN:
		CL_BulletShell (pl->current.origin, 1);

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_MACHINEGUN;

		VectorSet (dl->color, 1, 1, 0);
		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0);
		break;

	// MZ_SHOTGUN
	case MZ_SHOTGUN:
		CL_ShotgunShell (pl->current.origin, 1);

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SHOTGUN;

		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		Snd_StartSound (NULL, ent, CHAN_AUTO,   Snd_RegisterSound ("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;

	// MZ_SSHOTGUN
	case MZ_SSHOTGUN:
		CL_ShotgunShell (pl->current.origin, (1+(rand()%2)));

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SSHOTGUN;

		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_CHAINGUN1
	case MZ_CHAINGUN1:
		CL_BulletShell (pl->current.origin, (1+(rand()%2)));

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN1;

		VectorSet (dl->color, 1, 0.25, 0);
		dl->radius = 200 + (rand()&31);

		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0);
		break;

	// MZ_CHAINGUN2
	case MZ_CHAINGUN2:
		CL_BulletShell (pl->current.origin, (1 + (rand () % 2)));

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN2;

		VectorSet (dl->color, 1, 0.5, 0);
		dl->radius = 225 + (rand()&31);
		dl->die = cls.realTime + 0.1;	// long delay

		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0.05);
		break;

	// MZ_CHAINGUN3
	case MZ_CHAINGUN3:
		CL_BulletShell (pl->current.origin, (1+(rand()%2)));

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN3;

		VectorSet (dl->color, 1, 1, 0);
		dl->radius = 250 + (rand()&31);
		dl->die = cls.realTime  + 0.1;	// long delay

		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0.033);
		Com_sprintf(soundname, sizeof (soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), volume, ATTN_NORM, 0.066);
		break;

	// MZ_RAILGUN
	case MZ_RAILGUN:
		CL_RailShell (pl->current.origin, 1);

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_RAILGUN;

		VectorSet (dl->color, 0.5, 0.5, 1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_ROCKET
	case MZ_ROCKET:
		CL_RocketFireParticles (pl->current.origin, pl->current.angles);

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_ROCKET;

		VectorSet (dl->color, 1, 0.5, 0.2);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		Snd_StartSound (NULL, ent, CHAN_AUTO,   Snd_RegisterSound ("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;

	// MZ_GRENADE
	case MZ_GRENADE:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_GRENADE;

		VectorSet (dl->color, 1, 0.5, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		Snd_StartSound (NULL, ent, CHAN_AUTO,   Snd_RegisterSound ("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;

	// MZ_BFG
	case MZ_BFG:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BFG;

		VectorSet (dl->color, 0, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_LOGIN
	case MZ_LOGIN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_LOGIN;

		VectorSet (dl->color, 0, 1, 0);
		dl->die = cls.realTime + 1.0;
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect (pl->current.origin, weapon);
		break;

	// MZ_LOGOUT
	case MZ_LOGOUT:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_LOGOUT;

		VectorSet (dl->color, 1, 0, 0);
		dl->die = cls.realTime + 1.0;
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect (pl->current.origin, weapon);
		break;

	// MZ_RESPAWN
	case MZ_RESPAWN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_RESPAWN;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cls.realTime + 1.0;
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect (pl->current.origin, weapon);
		break;

	// RAFAEL
	// MZ_PHALANX
	case MZ_PHALANX:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_PHALANX;

		VectorSet (dl->color, 1, 0.5, 0.5);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
	// RAFAEL

	// MZ_IONRIPPER
	case MZ_IONRIPPER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_IONRIPPER;

		VectorSet (dl->color, 1, 0.5, 0.5);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

	// PGM
	// MZ_ETF_RIFLE
	case MZ_ETF_RIFLE:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_ETF_RIFLE;

		VectorSet (dl->color, 0.9, 0.7, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_SHOTGUN2
	case MZ_SHOTGUN2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SHOTGUN2;

		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_HEATBEAM
	case MZ_HEATBEAM:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_HEATBEAM;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cls.realTime + 100;
		break;

	// MZ_BLASTER2
	case MZ_BLASTER2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLASTER2;

		VectorSet (dl->color, 0, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_TRACKER
	case MZ_TRACKER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_TRACKER;

		VectorSet (dl->color, -1, -1, -1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;

	// MZ_NUKE1
	case MZ_NUKE1:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE1;

		VectorSet (dl->color, 1, 0, 0);
		dl->die = cls.realTime + 100;
		break;

	// MZ_NUKE2
	case MZ_NUKE2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE2;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cls.realTime + 100;
		break;

	// MZ_NUKE4
	case MZ_NUKE4:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE4;

		VectorSet (dl->color, 0, 0, 1);
		dl->die = cls.realTime + 100;
		break;

	// MZ_NUKE8
	case MZ_NUKE8:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE8;

		VectorSet (dl->color, 0, 1, 1);
		dl->die = cls.realTime + 100;
		break;
	// PGM
	}
}


/*
==============
CL_ParseMuzzleFlash2
==============
*/
void CL_ParseMuzzleFlash2 (void) {
	int			ent;
	vec3_t		origin;
	int			flash_number;
	cDLight_t	*dl;
	vec3_t		forward, right;
	char		soundname[64];

	ent = MSG_ReadShort (&net_Message);
	if ((ent < 1) || (ent >= MAX_EDICTS))
		Com_Error (ERR_DROP, "CL_ParseMuzzleFlash2: bad entity");

	flash_number = MSG_ReadByte (&net_Message);

	// locate the origin
	AngleVectors (cl_Entities[ent].current.angles, forward, right, NULL);
	origin[0] = cl_Entities[ent].current.origin[0] + forward[0] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][0] + right[0] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][1];
	origin[1] = cl_Entities[ent].current.origin[1] + forward[1] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][0] + right[1] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][1];
	origin[2] = cl_Entities[ent].current.origin[2] + forward[2] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][0] + right[2] * dumb_and_hacky_monster_MuzzFlashOffset[flash_number][1] + dumb_and_hacky_monster_MuzzFlashOffset[flash_number][2];

	dl = CL_AllocDLight (ent);
	VectorCopy (origin,  dl->origin);
	dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cls.realTime;

	switch (flash_number) {
	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:			// PGM
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:			// PGM
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MEDIC_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_HOVER_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLOAT_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Com_sprintf(soundname, sizeof (soundname), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound (soundname), 1, ATTN_NORM, 0);
		break;

	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:			// PGM
		VectorSet (dl->color, 1, 0.5, 0.2);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		VectorSet (dl->color, 1, 0.5, 0.2);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
//	case MZ2_CARRIER_ROCKET_2:
//	case MZ2_CARRIER_ROCKET_3:
//	case MZ2_CARRIER_ROCKET_4:
		VectorSet (dl->color, 1, 0.5, 0.2);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		VectorSet (dl->color, 1, 0.5, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GLADIATOR_RAILGUN_1:
	case MZ2_CARRIER_RAILGUN:	// PMM
	case MZ2_WIDOW_RAIL:		// PMM
		VectorSet (dl->color, 0.5, 0.5, 1);
		break;

	// --- Xian's shit starts ---
	case MZ2_MAKRON_BFG:
		VectorSet (dl->color, 0.5, 1, 0.5);
		//Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		VectorSet (dl->color, 1, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;
	
	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		break;

	case MZ2_JORG_BFG_1:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		break;

	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case MZ2_CARRIER_MACHINEGUN_R2:			// PMM
		VectorSet (dl->color, 1, 1, 0);
		CL_ParticleEffect (origin, vec3Identity, 0, 40);
		break;

	// ROGUE
	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		VectorSet (dl->color, 0, 1, 0);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_DISRUPTOR:
		VectorSet (dl->color, -1, -1, -1);
		Snd_StartSound (NULL, ent, CHAN_WEAPON, Snd_RegisterSound ("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = 300 + (rand()&100);
		VectorSet (dl->color, 1, 1, 0);
		dl->die = cls.realTime + 200;
		break;
	// ROGUE

	// --- Xian's shit ends ---
	}
}

/*
==============================================================

	DLIGHT STYLE MANAGEMENT

==============================================================
*/

typedef struct {
	int		length;
	float	value[3];
	float	map[MAX_QPATH];
} cLightStyle_t;

cLightStyle_t	cl_LightStyles[MAX_LIGHTSTYLES];
int				cl_LSLastOfs;

/*
================
CL_ClearLightStyles
================
*/
void CL_ClearLightStyles (void) {
	memset (cl_LightStyles, 0, sizeof (cl_LightStyles));
	cl_LSLastOfs = -1;
}


/*
================
CL_RunLightStyles
================
*/
void CL_RunLightStyles (void) {
	int				ofs;
	int				i;
	cLightStyle_t	*ls;

	ofs = cls.realTime * 0.01;
	if (ofs == cl_LSLastOfs)
		return;
	cl_LSLastOfs = ofs;

	for (i=0, ls=cl_LightStyles ; i<MAX_LIGHTSTYLES ; i++, ls++) {
		if (!ls->length) {
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}

		if (ls->length == 1)
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs%ls->length];
	}
}


/*
================
CL_SetLightstyle
================
*/
void CL_SetLightstyle (int i) {
	char	*s;
	int		j, k;

	s = cl.configStrings[i+CS_LIGHTS];

	j = strlen (s);
	if (j >= MAX_QPATH)
		Com_Error (ERR_DROP, "svc_lightstyle length=%i", j);

	cl_LightStyles[i].length = j;

	for (k=0 ; k<j ; k++)
		cl_LightStyles[i].map[k] = (float)(s[k]-'a')/(float)('m'-'a');
}


/*
================
CL_AddLightStyles
================
*/
void CL_AddLightStyles (void) {
	int				i;
	cLightStyle_t	*ls;

	for (i=0, ls=cl_LightStyles ; i<MAX_LIGHTSTYLES ; i++, ls++)
		R_AddLightStyle (i, ls->value[0], ls->value[1], ls->value[2]);
}

/*
==============================================================

	DLIGHT MANAGEMENT

==============================================================
*/

cDLight_t		cl_DLights[MAX_DLIGHTS];

/*
================
CL_ClearDLights
================
*/
void CL_ClearDLights (void) {
	memset (cl_DLights, 0, sizeof (cl_DLights));
}


/*
===============
CL_AllocDLight
===============
*/
cDLight_t *CL_AllocDLight (int key) {
	int		i;
	cDLight_t	*dl;

	// first look for an exact key match
	if (key) {
		dl = cl_DLights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
			if (dl->key == key) {
				memset (dl, 0, sizeof (*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_DLights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (dl->die < cls.realTime) {
			memset (dl, 0, sizeof (*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_DLights[0];
	memset (dl, 0, sizeof (*dl));
	dl->key = key;
	return dl;
}


/*
===============
CL_RunDLights
===============
*/
void CL_RunDLights (void) {
	int			i;
	cDLight_t	*dl;

	dl = cl_DLights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;
		
		if (dl->die < cls.realTime) {
			dl->radius = 0;
			return;
		}
		dl->radius -= cls.frameTime*dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_AddDLights
===============
*/
void CL_AddDLights (void) {
	int			i;
	cDLight_t	*dl;

	dl = cl_DLights;

	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++) {
		if (!dl->radius)
			continue;
		R_AddLight (dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}


/*
===============
CL_Flashlight
===============
*/
void CL_Flashlight (int ent, vec3_t pos) {
	cDLight_t	*dl;

	dl = CL_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cls.realTime + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}


/*
===============
CL_ColorFlash

flash of light
===============
*/
void FASTCALL CL_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b) {
	cDLight_t	*dl;

	dl = CL_AllocDLight (ent);
	VectorCopy (pos, dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cls.realTime + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}
