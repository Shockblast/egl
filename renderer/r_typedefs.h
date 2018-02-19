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
// r_typedefs.h
// Refresh type definitions
//

//
// Rendering system
//
typedef byte			rendererClass_t;

//
// Rendering backend
//
typedef char			meshType_t;
typedef uint16			meshFeatures_t;

//
// Image subsystem
//
typedef char			texUnit_t;
typedef uint16			texFlags_t;

//
// Material subsystem
//
typedef uint16			matBaseFlags_t;
typedef char			matSortKey_t;
typedef char			matCullType_t;
typedef char			matDeformvType_t;
typedef char			matPathType_t;
typedef char			matRegType_t;
typedef int				matSurfParams_t;
typedef char			matTableFunc_t;

typedef char			matPassAlphaFunc_t;
typedef char			matPassAlphaGen_t;
typedef uint16			matPassFlags_t;
typedef char			matPassRGBGen_t;
typedef char			matPassTcGen_t;
typedef char			matPassTcMod_t;

//
// Model subsystem
//
typedef char			modelType_t;
