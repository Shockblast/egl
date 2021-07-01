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
// cg_muzzleflash.c
//

#include "cg_local.h"

/*
==============
CG_ParseMuzzleFlash
==============
*/
void CG_ParseMuzzleFlash (void) {
	cgDLight_t	*dl;
	cgEntity_t	*pl;
	vec3_t		fv, rv;
	int			silenced;
	float		volume;
	int			entNum, flashNum;

	entNum = cgi.MSG_ReadShort ();
	if ((entNum < 1) || (entNum >= MAX_CS_EDICTS))
		Com_Error (ERR_DROP, "CG_ParseMuzzleFlash: bad entity");
	flashNum = cgi.MSG_ReadByte ();

	silenced = flashNum & MZ_SILENCED;
	flashNum &= ~MZ_SILENCED;

	pl = &cgEntities[entNum];

	dl = CG_AllocDLight (entNum);
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
	}
	else {
		pl->muzzSilenced = qFalse;
		volume = 1;
		dl->radius = 200 + (frand () * 32);
	}

	dl->minlight = 32;
	dl->die = cgs.realTime;

	switch (flashNum) {
	// MZ_BLASTER
	case MZ_BLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLASTER;
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.blasterFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_BLUEHYPERBLASTER
	case MZ_BLUEHYPERBLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLUEHYPERBLASTER;
		VectorSet (dl->color, 0, 0, 1);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.hyperBlasterFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_HYPERBLASTER
	case MZ_HYPERBLASTER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_HYPERBLASTER;
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.hyperBlasterFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_MACHINEGUN
	case MZ_MACHINEGUN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_MACHINEGUN;

		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0);
		break;

	// MZ_SHOTGUN
	case MZ_SHOTGUN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SHOTGUN;

		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.shotgunFireSfx, volume, ATTN_NORM, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_AUTO, cgMedia.mz.shotgunReloadSfx, volume, ATTN_NORM, 0.1);
		break;

	// MZ_SSHOTGUN
	case MZ_SSHOTGUN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SSHOTGUN;

		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.superShotgunFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_CHAINGUN1
	case MZ_CHAINGUN1:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN1;

		VectorSet (dl->color, 1, 0.25, 0);
		dl->radius = 200 + (rand()&31);

		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0);
		break;

	// MZ_CHAINGUN2
	case MZ_CHAINGUN2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN2;

		VectorSet (dl->color, 1, 0.5, 0);
		dl->radius = 225 + (rand()&31);
		dl->die = cgs.realTime + 0.1;	// long delay

		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0.05);
		break;

	// MZ_CHAINGUN3
	case MZ_CHAINGUN3:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_CHAINGUN3;

		VectorSet (dl->color, 1, 1, 0);
		dl->radius = 250 + (rand()&31);
		dl->die = cgs.realTime  + 0.1;	// long delay

		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0.033);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.machineGunSfx[(rand () % 5)], volume, ATTN_NORM, 0.066);
		break;

	// MZ_RAILGUN
	case MZ_RAILGUN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_RAILGUN;

		VectorSet (dl->color, 0.5, 0.5, 1);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.railgunFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_ROCKET
	case MZ_ROCKET:
		CG_RocketFireParticles (pl->current.origin, pl->current.angles);

		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_ROCKET;

		VectorSet (dl->color, 1, 0.5, 0.2);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.rocketFireSfx, volume, ATTN_NORM, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_AUTO, cgMedia.mz.rocketReloadSfx, volume, ATTN_NORM, 0.1);
		break;

	// MZ_GRENADE
	case MZ_GRENADE:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_GRENADE;

		VectorSet (dl->color, 1, 0.5, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.grenadeFireSfx, volume, ATTN_NORM, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_AUTO, cgMedia.mz.grenadeReloadSfx, volume, ATTN_NORM, 0.1);
		break;

	// MZ_BFG
	case MZ_BFG:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BFG;

		VectorSet (dl->color, 0, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.bfgFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_LOGIN
	case MZ_LOGIN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_LOGIN;

		VectorSet (dl->color, 0, 1, 0);
		dl->die = cgs.realTime + 1.0;
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.grenadeFireSfx, 1, ATTN_NORM, 0);
		CG_LogoutEffect (pl->current.origin, flashNum);
		break;

	// MZ_LOGOUT
	case MZ_LOGOUT:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_LOGOUT;

		VectorSet (dl->color, 1, 0, 0);
		dl->die = cgs.realTime + 1.0;
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.grenadeFireSfx, 1, ATTN_NORM, 0);
		CG_LogoutEffect (pl->current.origin, flashNum);
		break;

	// MZ_RESPAWN
	case MZ_RESPAWN:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_RESPAWN;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cgs.realTime + 1.0;
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.grenadeFireSfx, 1, ATTN_NORM, 0);
		CG_LogoutEffect (pl->current.origin, flashNum);
		break;

	// RAFAEL
	// MZ_PHALANX
	case MZ_PHALANX:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_PHALANX;

		VectorSet (dl->color, 1, 0.5, 0.5);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.phalanxFireSfx, volume, ATTN_NORM, 0);
		break;
	// RAFAEL

	// MZ_IONRIPPER
	case MZ_IONRIPPER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_IONRIPPER;

		VectorSet (dl->color, 1, 0.5, 0.5);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.ionRipperFireSfx, volume, ATTN_NORM, 0);
		break;

	// PGM
	// MZ_ETF_RIFLE
	case MZ_ETF_RIFLE:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_ETF_RIFLE;

		VectorSet (dl->color, 0.9, 0.7, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.etfRifleFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_SHOTGUN2
	case MZ_SHOTGUN2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_SHOTGUN2;

		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.shotgun2FireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_HEATBEAM
	case MZ_HEATBEAM:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_HEATBEAM;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cgs.realTime + 100;
		break;

	// MZ_BLASTER2
	case MZ_BLASTER2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_BLASTER2;

		VectorSet (dl->color, 0, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.blasterFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_TRACKER
	case MZ_TRACKER:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_TRACKER;

		VectorSet (dl->color, -1, -1, -1);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.trackerFireSfx, volume, ATTN_NORM, 0);
		break;

	// MZ_NUKE1
	case MZ_NUKE1:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE1;

		VectorSet (dl->color, 1, 0, 0);
		dl->die = cgs.realTime + 100;
		break;

	// MZ_NUKE2
	case MZ_NUKE2:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE2;

		VectorSet (dl->color, 1, 1, 0);
		dl->die = cgs.realTime + 100;
		break;

	// MZ_NUKE4
	case MZ_NUKE4:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE4;

		VectorSet (dl->color, 0, 0, 1);
		dl->die = cgs.realTime + 100;
		break;

	// MZ_NUKE8
	case MZ_NUKE8:
		pl->muzzleOn = qTrue;
		pl->muzzType = MZ_NUKE8;

		VectorSet (dl->color, 0, 1, 1);
		dl->die = cgs.realTime + 100;
		break;
	// PGM
	}
}


/*
==============
CG_ParseMuzzleFlash2
==============
*/
void CG_ParseMuzzleFlash2 (void) {
	cgDLight_t	*dl;
	vec3_t		origin, forward, right;
	int			entNum, flashNum;

	entNum = cgi.MSG_ReadShort ();
	if ((entNum < 1) || (entNum >= MAX_CS_EDICTS))
		Com_Error (ERR_DROP, "CG_ParseMuzzleFlash2: bad entity");
	flashNum = cgi.MSG_ReadByte ();

	// locate the origin
	AngleVectors (cgEntities[entNum].current.angles, forward, right, NULL);
	origin[0] = cgEntities[entNum].current.origin[0] + forward[0] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][0] + right[0] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][1];
	origin[1] = cgEntities[entNum].current.origin[1] + forward[1] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][0] + right[1] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][1];
	origin[2] = cgEntities[entNum].current.origin[2] + forward[2] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][0] + right[2] * dumb_and_hacky_monster_MuzzFlashOffset[flashNum][1] + dumb_and_hacky_monster_MuzzFlashOffset[flashNum][2];

	dl = CG_AllocDLight (entNum);
	VectorCopy (origin,  dl->origin);
	dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cgs.realTime;

	switch (flashNum) {
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.machGunSfx, 1, ATTN_NORM, 0);
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.soldierMachGunSfx, 1, ATTN_NORM, 0);
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.gunnerMachGunSfx, 1, ATTN_NORM, 0);
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.machGunSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		VectorSet (dl->color, 1, 1, 0);
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.machGunSfx, 1, ATTN_NONE, 0);
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
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.soldierBlasterSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.flyerBlasterSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_MEDIC_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.medicBlasterSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_HOVER_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.hoverBlasterSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_FLOAT_BLASTER_1:
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.floatBlasterSfx, 1, ATTN_NORM, 0);
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
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.soldierShotgunSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		VectorSet (dl->color, 1, 1, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.tankBlasterSfx, 1, ATTN_NORM, 0);
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.tankMachGunSfx[(rand () % 5)], 1, ATTN_NORM, 0);
		break;

	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:			// PGM
		VectorSet (dl->color, 1, 0.5, 0.2);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.chicRocketSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		VectorSet (dl->color, 1, 0.5, 0.2);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.tankRocketSfx, 1, ATTN_NORM, 0);
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
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.superTankRocketSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		VectorSet (dl->color, 1, 0.5, 0);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.gunnerGrenadeSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_GLADIATOR_RAILGUN_1:
	case MZ2_CARRIER_RAILGUN:	// PMM
	case MZ2_WIDOW_RAIL:		// PMM
		VectorSet (dl->color, 0.5, 0.5, 1);
		break;

	// --- Xian's shit starts ---
	case MZ2_MAKRON_BFG:
		VectorSet (dl->color, 0.5, 1, 0.5);
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
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.makronBlasterSfx, 1, ATTN_NORM, 0);
		break;
	
	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		VectorSet (dl->color, 1, 1, 0);
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.jorgMachGunSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		VectorSet (dl->color, 1, 1, 0);
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
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
		CG_ParticleEffect (origin, vec3Origin, 0, 40);
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
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz2.tankBlasterSfx, 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_DISRUPTOR:
		VectorSet (dl->color, -1, -1, -1);
		cgi.Snd_StartSound (NULL, entNum, CHAN_WEAPON, cgMedia.mz.trackerFireSfx, 1, ATTN_NORM, 0);
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
		dl->die = cgs.realTime + 200;
		break;
	// ROGUE

	// --- Xian's shit ends ---
	}
}
