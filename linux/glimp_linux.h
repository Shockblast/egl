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
// glimp_linux.h
//

#ifndef __linux__
#  error You should not be including this file on this platform
#endif // __linux__

#ifndef __GLIMP_LINUX_H__
#define __GLIMP_LINUX_H__

#define APPLICATION "EGL" EGL_VERSTR

typedef struct glxState_s {
	qBool		active;

	void		*OpenGLLib;		// OpenGL library handle

	FILE		*oglLogFP;		// for gl_log logging
} glxState_t;

extern glxState_t	glxState;

#endif	// __GLIMP_LINUX_H__
