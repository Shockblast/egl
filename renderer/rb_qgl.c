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
// rb_qgl.c
//
// This file implements the operating system binding of GL to QGL function pointers.
//
// QGL_Init () - loads libraries, assigns function pointers, etc.
// QGL_Shutdown () - unloads libraries, NULLs function pointers
// QGL_ToggleDebug () - toggles glGetError calls after every gl function call
// QGL_ToggleLogging () - toggles logging of gl function calls
// QGL_LogBeginFrame () - Prints to the log file the beginning of a frame
// QGL_LogEndFrame () - Prints to the log file the end of a frame
// QGL_GetProcAddress () - returns the address of a gl function
//

#ifdef WIN32

# include "../renderer/r_local.h"
# include "../win32/win_glimp.h"

# define LOGPROC	(glwState.oglLogFP)
# define GL_GPA(a)	(void *)GetProcAddress (glwState.hInstOpenGL, a)
# define SIG(x)		fprintf (glwState.oglLogFP, x "\n")

#elif defined __unix__

# include <dlfcn.h>
# include <GL/gl.h>
# include <GL/glx.h>
# include "../renderer/r_local.h"
# include "../unix/unix_glimp.h"

# define LOGPROC	(glxState.oglLogFP)
# define GL_GPA(a)	(void *)dlsym (glxState.OpenGLLib, a)
# define SIG(x)		fprintf (glxState.oglLogFP, x "\n")

#endif

//
// direct pointers
//
void		(APIENTRYP qglAccum) (GLenum op, GLfloat value);
void		(APIENTRYP qglAlphaFunc) (GLenum func, GLclampf ref);
GLboolean	(APIENTRYP qglAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences);
void		(APIENTRYP qglArrayElement) (GLint i);
void		(APIENTRYP qglBegin) (GLenum mode);
void		(APIENTRYP qglBindTexture) (GLenum target, GLuint texture);
void		(APIENTRYP qglBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void		(APIENTRYP qglBlendFunc) (GLenum sfactor, GLenum dfactor);
void		(APIENTRYP qglCallList) (GLuint list);
void		(APIENTRYP qglCallLists) (GLsizei n, GLenum type, const GLvoid *lists);
void		(APIENTRYP qglClear) (GLbitfield mask);
void		(APIENTRYP qglClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void		(APIENTRYP qglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void		(APIENTRYP qglClearDepth) (GLclampd depth);
void		(APIENTRYP qglClearIndex) (GLfloat c);
void		(APIENTRYP qglClearStencil) (GLint s);
void		(APIENTRYP qglClipPlane) (GLenum plane, const GLdouble *equation);
void		(APIENTRYP qglColor3b) (GLbyte red, GLbyte green, GLbyte blue);
void		(APIENTRYP qglColor3bv) (const GLbyte *v);
void		(APIENTRYP qglColor3d) (GLdouble red, GLdouble green, GLdouble blue);
void		(APIENTRYP qglColor3dv) (const GLdouble *v);
void		(APIENTRYP qglColor3f) (GLfloat red, GLfloat green, GLfloat blue);
void		(APIENTRYP qglColor3fv) (const GLfloat *v);
void		(APIENTRYP qglColor3i) (GLint red, GLint green, GLint blue);
void		(APIENTRYP qglColor3iv) (const GLint *v);
void		(APIENTRYP qglColor3s) (GLshort red, GLshort green, GLshort blue);
void		(APIENTRYP qglColor3sv) (const GLshort *v);
void		(APIENTRYP qglColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
void		(APIENTRYP qglColor3ubv) (const GLubyte *v);
void		(APIENTRYP qglColor3ui) (GLuint red, GLuint green, GLuint blue);
void		(APIENTRYP qglColor3uiv) (const GLuint *v);
void		(APIENTRYP qglColor3us) (GLushort red, GLushort green, GLushort blue);
void		(APIENTRYP qglColor3usv) (const GLushort *v);
void		(APIENTRYP qglColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void		(APIENTRYP qglColor4bv) (const GLbyte *v);
void		(APIENTRYP qglColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void		(APIENTRYP qglColor4dv) (const GLdouble *v);
void		(APIENTRYP qglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void		(APIENTRYP qglColor4fv) (const GLfloat *v);
void		(APIENTRYP qglColor4i) (GLint red, GLint green, GLint blue, GLint alpha);
void		(APIENTRYP qglColor4iv) (const GLint *v);
void		(APIENTRYP qglColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
void		(APIENTRYP qglColor4sv) (const GLshort *v);
void		(APIENTRYP qglColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void		(APIENTRYP qglColor4ubv) (const GLubyte *v);
void		(APIENTRYP qglColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
void		(APIENTRYP qglColor4uiv) (const GLuint *v);
void		(APIENTRYP qglColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
void		(APIENTRYP qglColor4usv) (const GLushort *v);
void		(APIENTRYP qglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void		(APIENTRYP qglColorMaterial) (GLenum face, GLenum mode);
void		(APIENTRYP qglColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void		(APIENTRYP qglCopyTexImage1D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void		(APIENTRYP qglCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void		(APIENTRYP qglCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void		(APIENTRYP qglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void		(APIENTRYP qglCullFace) (GLenum mode);
void		(APIENTRYP qglDeleteLists) (GLuint list, GLsizei range);
void		(APIENTRYP qglDeleteTextures) (GLsizei n, const GLuint *textures);
void		(APIENTRYP qglDepthFunc) (GLenum func);
void		(APIENTRYP qglDepthMask) (GLboolean flag);
void		(APIENTRYP qglDepthRange) (GLclampd zNear, GLclampd zFar);
void		(APIENTRYP qglDisable) (GLenum cap);
void		(APIENTRYP qglDisableClientState) (GLenum array);
void		(APIENTRYP qglDrawArrays) (GLenum mode, GLint first, GLsizei count);
void		(APIENTRYP qglDrawBuffer) (GLenum mode);
void		(APIENTRYP qglDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void		(APIENTRYP qglDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglEdgeFlag) (GLboolean flag);
void		(APIENTRYP qglEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglEdgeFlagv) (const GLboolean *flag);
void		(APIENTRYP qglEnable) (GLenum cap);
void		(APIENTRYP qglEnableClientState) (GLenum array);
void		(APIENTRYP qglEnd) (void);
void		(APIENTRYP qglEndList) (void);
void		(APIENTRYP qglEvalCoord1d) (GLdouble u);
void		(APIENTRYP qglEvalCoord1dv) (const GLdouble *u);
void		(APIENTRYP qglEvalCoord1f) (GLfloat u);
void		(APIENTRYP qglEvalCoord1fv) (const GLfloat *u);
void		(APIENTRYP qglEvalCoord2d) (GLdouble u, GLdouble v);
void		(APIENTRYP qglEvalCoord2dv) (const GLdouble *u);
void		(APIENTRYP qglEvalCoord2f) (GLfloat u, GLfloat v);
void		(APIENTRYP qglEvalCoord2fv) (const GLfloat *u);
void		(APIENTRYP qglEvalMesh1) (GLenum mode, GLint i1, GLint i2);
void		(APIENTRYP qglEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void		(APIENTRYP qglEvalPoint1) (GLint i);
void		(APIENTRYP qglEvalPoint2) (GLint i, GLint j);
void		(APIENTRYP qglFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer);
void		(APIENTRYP qglFinish) (void);
void		(APIENTRYP qglFlush) (void);
void		(APIENTRYP qglFogf) (GLenum pname, GLfloat param);
void		(APIENTRYP qglFogfv) (GLenum pname, const GLfloat *params);
void		(APIENTRYP qglFogi) (GLenum pname, GLint param);
void		(APIENTRYP qglFogiv) (GLenum pname, const GLint *params);
void		(APIENTRYP qglFrontFace) (GLenum mode);
void		(APIENTRYP qglFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint		(APIENTRYP qglGenLists) (GLsizei range);
void		(APIENTRYP qglGenTextures) (GLsizei n, GLuint *textures);
void		(APIENTRYP qglGetBooleanv) (GLenum pname, GLboolean *params);
void		(APIENTRYP qglGetClipPlane) (GLenum plane, GLdouble *equation);
void		(APIENTRYP qglGetDoublev) (GLenum pname, GLdouble *params);
GLenum		(APIENTRYP qglGetError) (void);
void		(APIENTRYP qglGetFloatv) (GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetIntegerv) (GLenum pname, GLint *params);
void		(APIENTRYP qglGetLightfv) (GLenum light, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetLightiv) (GLenum light, GLenum pname, GLint *params);
void		(APIENTRYP qglGetMapdv) (GLenum target, GLenum query, GLdouble *v);
void		(APIENTRYP qglGetMapfv) (GLenum target, GLenum query, GLfloat *v);
void		(APIENTRYP qglGetMapiv) (GLenum target, GLenum query, GLint *v);
void		(APIENTRYP qglGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetMaterialiv) (GLenum face, GLenum pname, GLint *params);
void		(APIENTRYP qglGetPixelMapfv) (GLenum map, GLfloat *values);
void		(APIENTRYP qglGetPixelMapuiv) (GLenum map, GLuint *values);
void		(APIENTRYP qglGetPixelMapusv) (GLenum map, GLushort *values);
void		(APIENTRYP qglGetPointerv) (GLenum pname, GLvoid* *params);
void		(APIENTRYP qglGetPolygonStipple) (GLubyte *mask);
const GLubyte * (APIENTRYP qglGetString) (GLenum name);
void		(APIENTRYP qglGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
void		(APIENTRYP qglGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params);
void		(APIENTRYP qglGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetTexGeniv) (GLenum coord, GLenum pname, GLint *params);
void		(APIENTRYP qglGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void		(APIENTRYP qglGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
void		(APIENTRYP qglGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);
void		(APIENTRYP qglGetTexParameteriv) (GLenum target, GLenum pname, GLint *params);
void		(APIENTRYP qglHint) (GLenum target, GLenum mode);
void		(APIENTRYP qglIndexMask) (GLuint mask);
void		(APIENTRYP qglIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglIndexd) (GLdouble c);
void		(APIENTRYP qglIndexdv) (const GLdouble *c);
void		(APIENTRYP qglIndexf) (GLfloat c);
void		(APIENTRYP qglIndexfv) (const GLfloat *c);
void		(APIENTRYP qglIndexi) (GLint c);
void		(APIENTRYP qglIndexiv) (const GLint *c);
void		(APIENTRYP qglIndexs) (GLshort c);
void		(APIENTRYP qglIndexsv) (const GLshort *c);
void		(APIENTRYP qglIndexub) (GLubyte c);
void		(APIENTRYP qglIndexubv) (const GLubyte *c);
void		(APIENTRYP qglInitNames) (void);
void		(APIENTRYP qglInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean	(APIENTRYP qglIsEnabled) (GLenum cap);
GLboolean	(APIENTRYP qglIsList) (GLuint list);
GLboolean	(APIENTRYP qglIsTexture) (GLuint texture);
void		(APIENTRYP qglLightModelf) (GLenum pname, GLfloat param);
void		(APIENTRYP qglLightModelfv) (GLenum pname, const GLfloat *params);
void		(APIENTRYP qglLightModeli) (GLenum pname, GLint param);
void		(APIENTRYP qglLightModeliv) (GLenum pname, const GLint *params);
void		(APIENTRYP qglLightf) (GLenum light, GLenum pname, GLfloat param);
void		(APIENTRYP qglLightfv) (GLenum light, GLenum pname, const GLfloat *params);
void		(APIENTRYP qglLighti) (GLenum light, GLenum pname, GLint param);
void		(APIENTRYP qglLightiv) (GLenum light, GLenum pname, const GLint *params);
void		(APIENTRYP qglLineStipple) (GLint factor, GLushort pattern);
void		(APIENTRYP qglLineWidth) (GLfloat width);
void		(APIENTRYP qglListBase) (GLuint base);
void		(APIENTRYP qglLoadIdentity) (void);
void		(APIENTRYP qglLoadMatrixd) (const GLdouble *m);
void		(APIENTRYP qglLoadMatrixf) (const GLfloat *m);
void		(APIENTRYP qglLoadName) (GLuint name);
void		(APIENTRYP qglLogicOp) (GLenum opcode);
void		(APIENTRYP qglMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void		(APIENTRYP qglMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void		(APIENTRYP qglMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void		(APIENTRYP qglMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void		(APIENTRYP qglMapGrid1d) (GLint un, GLdouble u1, GLdouble u2);
void		(APIENTRYP qglMapGrid1f) (GLint un, GLfloat u1, GLfloat u2);
void		(APIENTRYP qglMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void		(APIENTRYP qglMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void		(APIENTRYP qglMaterialf) (GLenum face, GLenum pname, GLfloat param);
void		(APIENTRYP qglMaterialfv) (GLenum face, GLenum pname, const GLfloat *params);
void		(APIENTRYP qglMateriali) (GLenum face, GLenum pname, GLint param);
void		(APIENTRYP qglMaterialiv) (GLenum face, GLenum pname, const GLint *params);
void		(APIENTRYP qglMatrixMode) (GLenum mode);
void		(APIENTRYP qglMultMatrixd) (const GLdouble *m);
void		(APIENTRYP qglMultMatrixf) (const GLfloat *m);
void		(APIENTRYP qglNewList) (GLuint list, GLenum mode);
void		(APIENTRYP qglNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
void		(APIENTRYP qglNormal3bv) (const GLbyte *v);
void		(APIENTRYP qglNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
void		(APIENTRYP qglNormal3dv) (const GLdouble *v);
void		(APIENTRYP qglNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
void		(APIENTRYP qglNormal3fv) (const GLfloat *v);
void		(APIENTRYP qglNormal3i) (GLint nx, GLint ny, GLint nz);
void		(APIENTRYP qglNormal3iv) (const GLint *v);
void		(APIENTRYP qglNormal3s) (GLshort nx, GLshort ny, GLshort nz);
void		(APIENTRYP qglNormal3sv) (const GLshort *v);
void		(APIENTRYP qglNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void		(APIENTRYP qglPassThrough) (GLfloat token);
void		(APIENTRYP qglPixelMapfv) (GLenum map, GLsizei mapsize, const GLfloat *values);
void		(APIENTRYP qglPixelMapuiv) (GLenum map, GLsizei mapsize, const GLuint *values);
void		(APIENTRYP qglPixelMapusv) (GLenum map, GLsizei mapsize, const GLushort *values);
void		(APIENTRYP qglPixelStoref) (GLenum pname, GLfloat param);
void		(APIENTRYP qglPixelStorei) (GLenum pname, GLint param);
void		(APIENTRYP qglPixelTransferf) (GLenum pname, GLfloat param);
void		(APIENTRYP qglPixelTransferi) (GLenum pname, GLint param);
void		(APIENTRYP qglPixelZoom) (GLfloat xfactor, GLfloat yfactor);
void		(APIENTRYP qglPointSize) (GLfloat size);
void		(APIENTRYP qglPolygonMode) (GLenum face, GLenum mode);
void		(APIENTRYP qglPolygonOffset) (GLfloat factor, GLfloat units);
void		(APIENTRYP qglPolygonStipple) (const GLubyte *mask);
void		(APIENTRYP qglPopAttrib) (void);
void		(APIENTRYP qglPopClientAttrib) (void);
void		(APIENTRYP qglPopMatrix) (void);
void		(APIENTRYP qglPopName) (void);
void		(APIENTRYP qglPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
void		(APIENTRYP qglPushAttrib) (GLbitfield mask);
void		(APIENTRYP qglPushClientAttrib) (GLbitfield mask);
void		(APIENTRYP qglPushMatrix) (void);
void		(APIENTRYP qglPushName) (GLuint name);
void		(APIENTRYP qglRasterPos2d) (GLdouble x, GLdouble y);
void		(APIENTRYP qglRasterPos2dv) (const GLdouble *v);
void		(APIENTRYP qglRasterPos2f) (GLfloat x, GLfloat y);
void		(APIENTRYP qglRasterPos2fv) (const GLfloat *v);
void		(APIENTRYP qglRasterPos2i) (GLint x, GLint y);
void		(APIENTRYP qglRasterPos2iv) (const GLint *v);
void		(APIENTRYP qglRasterPos2s) (GLshort x, GLshort y);
void		(APIENTRYP qglRasterPos2sv) (const GLshort *v);
void		(APIENTRYP qglRasterPos3d) (GLdouble x, GLdouble y, GLdouble z);
void		(APIENTRYP qglRasterPos3dv) (const GLdouble *v);
void		(APIENTRYP qglRasterPos3f) (GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRYP qglRasterPos3fv) (const GLfloat *v);
void		(APIENTRYP qglRasterPos3i) (GLint x, GLint y, GLint z);
void		(APIENTRYP qglRasterPos3iv) (const GLint *v);
void		(APIENTRYP qglRasterPos3s) (GLshort x, GLshort y, GLshort z);
void		(APIENTRYP qglRasterPos3sv) (const GLshort *v);
void		(APIENTRYP qglRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void		(APIENTRYP qglRasterPos4dv) (const GLdouble *v);
void		(APIENTRYP qglRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void		(APIENTRYP qglRasterPos4fv) (const GLfloat *v);
void		(APIENTRYP qglRasterPos4i) (GLint x, GLint y, GLint z, GLint w);
void		(APIENTRYP qglRasterPos4iv) (const GLint *v);
void		(APIENTRYP qglRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w);
void		(APIENTRYP qglRasterPos4sv) (const GLshort *v);
void		(APIENTRYP qglReadBuffer) (GLenum mode);
void		(APIENTRYP qglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void		(APIENTRYP qglRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void		(APIENTRYP qglRectdv) (const GLdouble *v1, const GLdouble *v2);
void		(APIENTRYP qglRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void		(APIENTRYP qglRectfv) (const GLfloat *v1, const GLfloat *v2);
void		(APIENTRYP qglRecti) (GLint x1, GLint y1, GLint x2, GLint y2);
void		(APIENTRYP qglRectiv) (const GLint *v1, const GLint *v2);
void		(APIENTRYP qglRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void		(APIENTRYP qglRectsv) (const GLshort *v1, const GLshort *v2);
GLint		(APIENTRYP qglRenderMode) (GLenum mode);
void		(APIENTRYP qglRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void		(APIENTRYP qglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRYP qglScaled) (GLdouble x, GLdouble y, GLdouble z);
void		(APIENTRYP qglScalef) (GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRYP qglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
void		(APIENTRYP qglSelectBuffer) (GLsizei size, GLuint *buffer);
void		(APIENTRYP qglShadeModel) (GLenum mode);
void		(APIENTRYP qglStencilFunc) (GLenum func, GLint ref, GLuint mask);
void		(APIENTRYP qglStencilMask) (GLuint mask);
void		(APIENTRYP qglStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
void		(APIENTRYP qglTexCoord1d) (GLdouble s);
void		(APIENTRYP qglTexCoord1dv) (const GLdouble *v);
void		(APIENTRYP qglTexCoord1f) (GLfloat s);
void		(APIENTRYP qglTexCoord1fv) (const GLfloat *v);
void		(APIENTRYP qglTexCoord1i) (GLint s);
void		(APIENTRYP qglTexCoord1iv) (const GLint *v);
void		(APIENTRYP qglTexCoord1s) (GLshort s);
void		(APIENTRYP qglTexCoord1sv) (const GLshort *v);
void		(APIENTRYP qglTexCoord2d) (GLdouble s, GLdouble t);
void		(APIENTRYP qglTexCoord2dv) (const GLdouble *v);
void		(APIENTRYP qglTexCoord2f) (GLfloat s, GLfloat t);
void		(APIENTRYP qglTexCoord2fv) (const GLfloat *v);
void		(APIENTRYP qglTexCoord2i) (GLint s, GLint t);
void		(APIENTRYP qglTexCoord2iv) (const GLint *v);
void		(APIENTRYP qglTexCoord2s) (GLshort s, GLshort t);
void		(APIENTRYP qglTexCoord2sv) (const GLshort *v);
void		(APIENTRYP qglTexCoord3d) (GLdouble s, GLdouble t, GLdouble r);
void		(APIENTRYP qglTexCoord3dv) (const GLdouble *v);
void		(APIENTRYP qglTexCoord3f) (GLfloat s, GLfloat t, GLfloat r);
void		(APIENTRYP qglTexCoord3fv) (const GLfloat *v);
void		(APIENTRYP qglTexCoord3i) (GLint s, GLint t, GLint r);
void		(APIENTRYP qglTexCoord3iv) (const GLint *v);
void		(APIENTRYP qglTexCoord3s) (GLshort s, GLshort t, GLshort r);
void		(APIENTRYP qglTexCoord3sv) (const GLshort *v);
void		(APIENTRYP qglTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void		(APIENTRYP qglTexCoord4dv) (const GLdouble *v);
void		(APIENTRYP qglTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void		(APIENTRYP qglTexCoord4fv) (const GLfloat *v);
void		(APIENTRYP qglTexCoord4i) (GLint s, GLint t, GLint r, GLint q);
void		(APIENTRYP qglTexCoord4iv) (const GLint *v);
void		(APIENTRYP qglTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q);
void		(APIENTRYP qglTexCoord4sv) (const GLshort *v);
void		(APIENTRYP qglTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglTexEnvf) (GLenum target, GLenum pname, GLfloat param);
void		(APIENTRYP qglTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
void		(APIENTRYP qglTexEnvi) (GLenum target, GLenum pname, GLint param);
void		(APIENTRYP qglTexEnviv) (GLenum target, GLenum pname, const GLint *params);
void		(APIENTRYP qglTexGend) (GLenum coord, GLenum pname, GLdouble param);
void		(APIENTRYP qglTexGendv) (GLenum coord, GLenum pname, const GLdouble *params);
void		(APIENTRYP qglTexGenf) (GLenum coord, GLenum pname, GLfloat param);
void		(APIENTRYP qglTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params);
void		(APIENTRYP qglTexGeni) (GLenum coord, GLenum pname, GLint param);
void		(APIENTRYP qglTexGeniv) (GLenum coord, GLenum pname, const GLint *params);
void		(APIENTRYP qglTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglTexParameterf) (GLenum target, GLenum pname, GLfloat param);
void		(APIENTRYP qglTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
void		(APIENTRYP qglTexParameteri) (GLenum target, GLenum pname, GLint param);
void		(APIENTRYP qglTexParameteriv) (GLenum target, GLenum pname, const GLint *params);
void		(APIENTRYP qglTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglTranslated) (GLdouble x, GLdouble y, GLdouble z);
void		(APIENTRYP qglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRYP qglVertex2d) (GLdouble x, GLdouble y);
void		(APIENTRYP qglVertex2dv) (const GLdouble *v);
void		(APIENTRYP qglVertex2f) (GLfloat x, GLfloat y);
void		(APIENTRYP qglVertex2fv) (const GLfloat *v);
void		(APIENTRYP qglVertex2i) (GLint x, GLint y);
void		(APIENTRYP qglVertex2iv) (const GLint *v);
void		(APIENTRYP qglVertex2s) (GLshort x, GLshort y);
void		(APIENTRYP qglVertex2sv) (const GLshort *v);
void		(APIENTRYP qglVertex3d) (GLdouble x, GLdouble y, GLdouble z);
void		(APIENTRYP qglVertex3dv) (const GLdouble *v);
void		(APIENTRYP qglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
void		(APIENTRYP qglVertex3fv) (const GLfloat *v);
void		(APIENTRYP qglVertex3i) (GLint x, GLint y, GLint z);
void		(APIENTRYP qglVertex3iv) (const GLint *v);
void		(APIENTRYP qglVertex3s) (GLshort x, GLshort y, GLshort z);
void		(APIENTRYP qglVertex3sv) (const GLshort *v);
void		(APIENTRYP qglVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void		(APIENTRYP qglVertex4dv) (const GLdouble *v);
void		(APIENTRYP qglVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void		(APIENTRYP qglVertex4fv) (const GLfloat *v);
void		(APIENTRYP qglVertex4i) (GLint x, GLint y, GLint z, GLint w);
void		(APIENTRYP qglVertex4iv) (const GLint *v);
void		(APIENTRYP qglVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w);
void		(APIENTRYP qglVertex4sv) (const GLshort *v);
void		(APIENTRYP qglVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

//
// win32
//

#ifdef WIN32
int			(WINAPIP qwglChoosePixelFormat) (HDC, CONST PIXELFORMATDESCRIPTOR *);
int			(WINAPIP qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int			(WINAPIP qwglGetPixelFormat) (HDC);
BOOL		(WINAPIP qwglSetPixelFormat) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
BOOL		(WINAPIP qwglSwapBuffers) (HDC);

BOOL		(WINAPIP qwglCopyContext) (HGLRC, HGLRC, UINT);
HGLRC		(WINAPIP qwglCreateContext) (HDC);
HGLRC		(WINAPIP qwglCreateLayerContext) (HDC, int);
BOOL		(WINAPIP qwglDeleteContext) (HGLRC);
HGLRC		(WINAPIP qwglGetCurrentContext) (VOID);
HDC			(WINAPIP qwglGetCurrentDC) (VOID);
PROC		(WINAPIP qwglGetProcAddress) (LPCSTR);
BOOL		(WINAPIP qwglMakeCurrent) (HDC, HGLRC);
BOOL		(WINAPIP qwglShareLists) (HGLRC, HGLRC);
BOOL		(WINAPIP qwglUseFontBitmaps) (HDC, DWORD, DWORD, DWORD);

BOOL		(WINAPIP qwglUseFontOutlines) (HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
BOOL		(WINAPIP qwglDescribeLayerPlane) (HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
int			(WINAPIP qwglSetLayerPaletteEntries) (HDC, int, int, int, CONST COLORREF *);
int			(WINAPIP qwglGetLayerPaletteEntries) (HDC, int, int, int, COLORREF *);
BOOL		(WINAPIP qwglRealizeLayerPalette) (HDC, int, BOOL);
BOOL		(WINAPIP qwglSwapLayerBuffers) (HDC, UINT);

BOOL		(WINAPIP qwglSwapIntervalEXT) (int interval);

BOOL		(WINAPIP qwglGetDeviceGammaRamp3DFX) (HDC hDC, WORD *ramp);
BOOL		(WINAPIP qwglSetDeviceGammaRamp3DFX) (HDC hDC, WORD *ramp);
#endif

//
// unix
//

#ifdef __unix__
XVisualInfo	*(*qglXChooseVisual) (Display *dpy, int screen, int *attribList);
GLXContext	(*qglXCreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
void		(*qglXDestroyContext) (Display *dpy, GLXContext ctx);
Bool	 	(*qglXMakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
void		(*qglXCopyContext) (Display *dpy, GLXContext src, GLXContext dst, GLuint mask);
void		(*qglXSwapBuffers) (Display *dpy, GLXDrawable drawable);
#endif

//
// extensions
//

void		(APIENTRYP qglSelectTextureSGIS) (GLenum texture);
void		(APIENTRYP qglActiveTextureARB) (GLenum texture);
void		(APIENTRYP qglClientActiveTextureARB) (GLenum texture);

void		(APIENTRYP qglLockArraysEXT) (int first, int count);
void		(APIENTRYP qglUnlockArraysEXT) (void);

void		(APIENTRYP qglDrawRangeElementsEXT) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

void		(APIENTRYP qglBindBufferARB) (GLenum target, GLuint buffer);
void		(APIENTRYP qglDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
void		(APIENTRYP qglGenBuffersARB) (GLsizei n, GLuint *buffers);
GLboolean	(APIENTRYP qglIsBufferARB) (GLuint buffer);
GLvoid		*(APIENTRYP qglMapBufferARB) (GLenum target, GLenum access);
GLboolean	(APIENTRYP qglUnmapBufferARB) (GLenum target);
void		(APIENTRYP qglBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
void		(APIENTRYP qglBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

void		(APIENTRYP qglVertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
void		(APIENTRYP qglEnableVertexAttribArrayARB) (GLuint index);
void		(APIENTRYP qglDisableVertexAttribArrayARB) (GLuint index);
void		(APIENTRYP qglBindProgramARB) (GLenum target, GLuint program);
void		(APIENTRYP qglDeleteProgramsARB) (GLsizei n, const GLuint *programs);
void		(APIENTRYP qglGenProgramsARB) (GLsizei n, GLuint *programs);
void		(APIENTRYP qglProgramStringARB) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
void		(APIENTRYP qglProgramEnvParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void		(APIENTRYP qglProgramEnvParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
void		(APIENTRYP qglProgramLocalParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void		(APIENTRYP qglProgramLocalParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
void		(APIENTRYP qglGetProgramivARB) (GLenum target, GLenum pname, GLint *params);

void		(APIENTRYP qglTexImage3D) (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void		(APIENTRYP qglTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);

void		(APIENTRYP qglActiveStencilFaceEXT) (GLenum face);

//
// dll pointers
//
static void		(APIENTRYP dllAccum) (GLenum op, GLfloat value);
static void		(APIENTRYP dllAlphaFunc) (GLenum func, GLclampf ref);
static GLboolean (APIENTRYP dllAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences);
static void		(APIENTRYP dllArrayElement) (GLint i);
static void		(APIENTRYP dllBegin) (GLenum mode);
static void		(APIENTRYP dllBindTexture) (GLenum target, GLuint texture);
static void		(APIENTRYP dllBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static void		(APIENTRYP dllBlendFunc) (GLenum sfactor, GLenum dfactor);
static void		(APIENTRYP dllCallList) (GLuint list);
static void		(APIENTRYP dllCallLists) (GLsizei n, GLenum type, const GLvoid *lists);
static void		(APIENTRYP dllClear) (GLbitfield mask);
static void		(APIENTRYP dllClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void		(APIENTRYP dllClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void		(APIENTRYP dllClearDepth) (GLclampd depth);
static void		(APIENTRYP dllClearIndex) (GLfloat c);
static void		(APIENTRYP dllClearStencil) (GLint s);
static void		(APIENTRYP dllClipPlane) (GLenum plane, const GLdouble *equation);
static void		(APIENTRYP dllColor3b) (GLbyte red, GLbyte green, GLbyte blue);
static void		(APIENTRYP dllColor3bv) (const GLbyte *v);
static void		(APIENTRYP dllColor3d) (GLdouble red, GLdouble green, GLdouble blue);
static void		(APIENTRYP dllColor3dv) (const GLdouble *v);
static void		(APIENTRYP dllColor3f) (GLfloat red, GLfloat green, GLfloat blue);
static void		(APIENTRYP dllColor3fv) (const GLfloat *v);
static void		(APIENTRYP dllColor3i) (GLint red, GLint green, GLint blue);
static void		(APIENTRYP dllColor3iv) (const GLint *v);
static void		(APIENTRYP dllColor3s) (GLshort red, GLshort green, GLshort blue);
static void		(APIENTRYP dllColor3sv) (const GLshort *v);
static void		(APIENTRYP dllColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
static void		(APIENTRYP dllColor3ubv) (const GLubyte *v);
static void		(APIENTRYP dllColor3ui) (GLuint red, GLuint green, GLuint blue);
static void		(APIENTRYP dllColor3uiv) (const GLuint *v);
static void		(APIENTRYP dllColor3us) (GLushort red, GLushort green, GLushort blue);
static void		(APIENTRYP dllColor3usv) (const GLushort *v);
static void		(APIENTRYP dllColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static void		(APIENTRYP dllColor4bv) (const GLbyte *v);
static void		(APIENTRYP dllColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static void		(APIENTRYP dllColor4dv) (const GLdouble *v);
static void		(APIENTRYP dllColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void		(APIENTRYP dllColor4fv) (const GLfloat *v);
static void		(APIENTRYP dllColor4i) (GLint red, GLint green, GLint blue, GLint alpha);
static void		(APIENTRYP dllColor4iv) (const GLint *v);
static void		(APIENTRYP dllColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
static void		(APIENTRYP dllColor4sv) (const GLshort *v);
static void		(APIENTRYP dllColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static void		(APIENTRYP dllColor4ubv) (const GLubyte *v);
static void		(APIENTRYP dllColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
static void		(APIENTRYP dllColor4uiv) (const GLuint *v);
static void		(APIENTRYP dllColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
static void		(APIENTRYP dllColor4usv) (const GLushort *v);
static void		(APIENTRYP dllColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void		(APIENTRYP dllColorMaterial) (GLenum face, GLenum mode);
static void		(APIENTRYP dllColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static void		(APIENTRYP dllCopyTexImage1D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
static void		(APIENTRYP dllCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static void		(APIENTRYP dllCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static void		(APIENTRYP dllCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static void		(APIENTRYP dllCullFace) (GLenum mode);
static void		(APIENTRYP dllDeleteLists) (GLuint list, GLsizei range);
static void		(APIENTRYP dllDeleteTextures) (GLsizei n, const GLuint *textures);
static void		(APIENTRYP dllDepthFunc) (GLenum func);
static void		(APIENTRYP dllDepthMask) (GLboolean flag);
static void		(APIENTRYP dllDepthRange) (GLclampd zNear, GLclampd zFar);
static void		(APIENTRYP dllDisable) (GLenum cap);
static void		(APIENTRYP dllDisableClientState) (GLenum array);
static void		(APIENTRYP dllDrawArrays) (GLenum mode, GLint first, GLsizei count);
static void		(APIENTRYP dllDrawBuffer) (GLenum mode);
static void		(APIENTRYP dllDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void		(APIENTRYP dllDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void		(APIENTRYP dllEdgeFlag) (GLboolean flag);
static void		(APIENTRYP dllEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllEdgeFlagv) (const GLboolean *flag);
static void		(APIENTRYP dllEnable) (GLenum cap);
static void		(APIENTRYP dllEnableClientState) (GLenum array);
static void		(APIENTRYP dllEnd) (void);
static void		(APIENTRYP dllEndList) (void);
static void		(APIENTRYP dllEvalCoord1d) (GLdouble u);
static void		(APIENTRYP dllEvalCoord1dv) (const GLdouble *u);
static void		(APIENTRYP dllEvalCoord1f) (GLfloat u);
static void		(APIENTRYP dllEvalCoord1fv) (const GLfloat *u);
static void		(APIENTRYP dllEvalCoord2d) (GLdouble u, GLdouble v);
static void		(APIENTRYP dllEvalCoord2dv) (const GLdouble *u);
static void		(APIENTRYP dllEvalCoord2f) (GLfloat u, GLfloat v);
static void		(APIENTRYP dllEvalCoord2fv) (const GLfloat *u);
static void		(APIENTRYP dllEvalMesh1) (GLenum mode, GLint i1, GLint i2);
static void		(APIENTRYP dllEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static void		(APIENTRYP dllEvalPoint1) (GLint i);
static void		(APIENTRYP dllEvalPoint2) (GLint i, GLint j);
static void		(APIENTRYP dllFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer);
static void		(APIENTRYP dllFinish) (void);
static void		(APIENTRYP dllFlush) (void);
static void		(APIENTRYP dllFogf) (GLenum pname, GLfloat param);
static void		(APIENTRYP dllFogfv) (GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllFogi) (GLenum pname, GLint param);
static void		(APIENTRYP dllFogiv) (GLenum pname, const GLint *params);
static void		(APIENTRYP dllFrontFace) (GLenum mode);
static void		(APIENTRYP dllFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static GLuint	(APIENTRYP dllGenLists) (GLsizei range);
static void		(APIENTRYP dllGenTextures) (GLsizei n, GLuint *textures);
static void		(APIENTRYP dllGetBooleanv) (GLenum pname, GLboolean *params);
static void		(APIENTRYP dllGetClipPlane) (GLenum plane, GLdouble *equation);
static void		(APIENTRYP dllGetDoublev) (GLenum pname, GLdouble *params);
static GLenum	(APIENTRYP dllGetError) (void);
static void		(APIENTRYP dllGetFloatv) (GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetIntegerv) (GLenum pname, GLint *params);
static void		(APIENTRYP dllGetLightfv) (GLenum light, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetLightiv) (GLenum light, GLenum pname, GLint *params);
static void		(APIENTRYP dllGetMapdv) (GLenum target, GLenum query, GLdouble *v);
static void		(APIENTRYP dllGetMapfv) (GLenum target, GLenum query, GLfloat *v);
static void		(APIENTRYP dllGetMapiv) (GLenum target, GLenum query, GLint *v);
static void		(APIENTRYP dllGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetMaterialiv) (GLenum face, GLenum pname, GLint *params);
static void		(APIENTRYP dllGetPixelMapfv) (GLenum map, GLfloat *values);
static void		(APIENTRYP dllGetPixelMapuiv) (GLenum map, GLuint *values);
static void		(APIENTRYP dllGetPixelMapusv) (GLenum map, GLushort *values);
static void		(APIENTRYP dllGetPointerv) (GLenum pname, GLvoid* *params);
static void		(APIENTRYP dllGetPolygonStipple) (GLubyte *mask);
static const GLubyte * (APIENTRYP dllGetString) (GLenum name);
static void		(APIENTRYP dllGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
static void		(APIENTRYP dllGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params);
static void		(APIENTRYP dllGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetTexGeniv) (GLenum coord, GLenum pname, GLint *params);
static void		(APIENTRYP dllGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static void		(APIENTRYP dllGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
static void		(APIENTRYP dllGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);
static void		(APIENTRYP dllGetTexParameteriv) (GLenum target, GLenum pname, GLint *params);
static void		(APIENTRYP dllHint) (GLenum target, GLenum mode);
static void		(APIENTRYP dllIndexMask) (GLuint mask);
static void		(APIENTRYP dllIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllIndexd) (GLdouble c);
static void		(APIENTRYP dllIndexdv) (const GLdouble *c);
static void		(APIENTRYP dllIndexf) (GLfloat c);
static void		(APIENTRYP dllIndexfv) (const GLfloat *c);
static void		(APIENTRYP dllIndexi) (GLint c);
static void		(APIENTRYP dllIndexiv) (const GLint *c);
static void		(APIENTRYP dllIndexs) (GLshort c);
static void		(APIENTRYP dllIndexsv) (const GLshort *c);
static void		(APIENTRYP dllIndexub) (GLubyte c);
static void		(APIENTRYP dllIndexubv) (const GLubyte *c);
static void		(APIENTRYP dllInitNames) (void);
static void		(APIENTRYP dllInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer);
static GLboolean (APIENTRYP dllIsEnabled) (GLenum cap);
static GLboolean (APIENTRYP dllIsList) (GLuint list);
static GLboolean (APIENTRYP dllIsTexture) (GLuint texture);
static void		(APIENTRYP dllLightModelf) (GLenum pname, GLfloat param);
static void		(APIENTRYP dllLightModelfv) (GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllLightModeli) (GLenum pname, GLint param);
static void		(APIENTRYP dllLightModeliv) (GLenum pname, const GLint *params);
static void		(APIENTRYP dllLightf) (GLenum light, GLenum pname, GLfloat param);
static void		(APIENTRYP dllLightfv) (GLenum light, GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllLighti) (GLenum light, GLenum pname, GLint param);
static void		(APIENTRYP dllLightiv) (GLenum light, GLenum pname, const GLint *params);
static void		(APIENTRYP dllLineStipple) (GLint factor, GLushort pattern);
static void		(APIENTRYP dllLineWidth) (GLfloat width);
static void		(APIENTRYP dllListBase) (GLuint base);
static void		(APIENTRYP dllLoadIdentity) (void);
static void		(APIENTRYP dllLoadMatrixd) (const GLdouble *m);
static void		(APIENTRYP dllLoadMatrixf) (const GLfloat *m);
static void		(APIENTRYP dllLoadName) (GLuint name);
static void		(APIENTRYP dllLogicOp) (GLenum opcode);
static void		(APIENTRYP dllMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static void		(APIENTRYP dllMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static void		(APIENTRYP dllMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static void		(APIENTRYP dllMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static void		(APIENTRYP dllMapGrid1d) (GLint un, GLdouble u1, GLdouble u2);
static void		(APIENTRYP dllMapGrid1f) (GLint un, GLfloat u1, GLfloat u2);
static void		(APIENTRYP dllMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static void		(APIENTRYP dllMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static void		(APIENTRYP dllMaterialf) (GLenum face, GLenum pname, GLfloat param);
static void		(APIENTRYP dllMaterialfv) (GLenum face, GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllMateriali) (GLenum face, GLenum pname, GLint param);
static void		(APIENTRYP dllMaterialiv) (GLenum face, GLenum pname, const GLint *params);
static void		(APIENTRYP dllMatrixMode) (GLenum mode);
static void		(APIENTRYP dllMultMatrixd) (const GLdouble *m);
static void		(APIENTRYP dllMultMatrixf) (const GLfloat *m);
static void		(APIENTRYP dllNewList) (GLuint list, GLenum mode);
static void		(APIENTRYP dllNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
static void		(APIENTRYP dllNormal3bv) (const GLbyte *v);
static void		(APIENTRYP dllNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
static void		(APIENTRYP dllNormal3dv) (const GLdouble *v);
static void		(APIENTRYP dllNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
static void		(APIENTRYP dllNormal3fv) (const GLfloat *v);
static void		(APIENTRYP dllNormal3i) (GLint nx, GLint ny, GLint nz);
static void		(APIENTRYP dllNormal3iv) (const GLint *v);
static void		(APIENTRYP dllNormal3s) (GLshort nx, GLshort ny, GLshort nz);
static void		(APIENTRYP dllNormal3sv) (const GLshort *v);
static void		(APIENTRYP dllNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void		(APIENTRYP dllPassThrough) (GLfloat token);
static void		(APIENTRYP dllPixelMapfv) (GLenum map, GLsizei mapsize, const GLfloat *values);
static void		(APIENTRYP dllPixelMapuiv) (GLenum map, GLsizei mapsize, const GLuint *values);
static void		(APIENTRYP dllPixelMapusv) (GLenum map, GLsizei mapsize, const GLushort *values);
static void		(APIENTRYP dllPixelStoref) (GLenum pname, GLfloat param);
static void		(APIENTRYP dllPixelStorei) (GLenum pname, GLint param);
static void		(APIENTRYP dllPixelTransferf) (GLenum pname, GLfloat param);
static void		(APIENTRYP dllPixelTransferi) (GLenum pname, GLint param);
static void		(APIENTRYP dllPixelZoom) (GLfloat xfactor, GLfloat yfactor);
static void		(APIENTRYP dllPointSize) (GLfloat size);
static void		(APIENTRYP dllPolygonMode) (GLenum face, GLenum mode);
static void		(APIENTRYP dllPolygonOffset) (GLfloat factor, GLfloat units);
static void		(APIENTRYP dllPolygonStipple) (const GLubyte *mask);
static void		(APIENTRYP dllPopAttrib) (void);
static void		(APIENTRYP dllPopClientAttrib) (void);
static void		(APIENTRYP dllPopMatrix) (void);
static void		(APIENTRYP dllPopName) (void);
static void		(APIENTRYP dllPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
static void		(APIENTRYP dllPushAttrib) (GLbitfield mask);
static void		(APIENTRYP dllPushClientAttrib) (GLbitfield mask);
static void		(APIENTRYP dllPushMatrix) (void);
static void		(APIENTRYP dllPushName) (GLuint name);
static void		(APIENTRYP dllRasterPos2d) (GLdouble x, GLdouble y);
static void		(APIENTRYP dllRasterPos2dv) (const GLdouble *v);
static void		(APIENTRYP dllRasterPos2f) (GLfloat x, GLfloat y);
static void		(APIENTRYP dllRasterPos2fv) (const GLfloat *v);
static void		(APIENTRYP dllRasterPos2i) (GLint x, GLint y);
static void		(APIENTRYP dllRasterPos2iv) (const GLint *v);
static void		(APIENTRYP dllRasterPos2s) (GLshort x, GLshort y);
static void		(APIENTRYP dllRasterPos2sv) (const GLshort *v);
static void		(APIENTRYP dllRasterPos3d) (GLdouble x, GLdouble y, GLdouble z);
static void		(APIENTRYP dllRasterPos3dv) (const GLdouble *v);
static void		(APIENTRYP dllRasterPos3f) (GLfloat x, GLfloat y, GLfloat z);
static void		(APIENTRYP dllRasterPos3fv) (const GLfloat *v);
static void		(APIENTRYP dllRasterPos3i) (GLint x, GLint y, GLint z);
static void		(APIENTRYP dllRasterPos3iv) (const GLint *v);
static void		(APIENTRYP dllRasterPos3s) (GLshort x, GLshort y, GLshort z);
static void		(APIENTRYP dllRasterPos3sv) (const GLshort *v);
static void		(APIENTRYP dllRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void		(APIENTRYP dllRasterPos4dv) (const GLdouble *v);
static void		(APIENTRYP dllRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void		(APIENTRYP dllRasterPos4fv) (const GLfloat *v);
static void		(APIENTRYP dllRasterPos4i) (GLint x, GLint y, GLint z, GLint w);
static void		(APIENTRYP dllRasterPos4iv) (const GLint *v);
static void		(APIENTRYP dllRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w);
static void		(APIENTRYP dllRasterPos4sv) (const GLshort *v);
static void		(APIENTRYP dllReadBuffer) (GLenum mode);
static void		(APIENTRYP dllReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static void		(APIENTRYP dllRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static void		(APIENTRYP dllRectdv) (const GLdouble *v1, const GLdouble *v2);
static void		(APIENTRYP dllRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static void		(APIENTRYP dllRectfv) (const GLfloat *v1, const GLfloat *v2);
static void		(APIENTRYP dllRecti) (GLint x1, GLint y1, GLint x2, GLint y2);
static void		(APIENTRYP dllRectiv) (const GLint *v1, const GLint *v2);
static void		(APIENTRYP dllRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static void		(APIENTRYP dllRectsv) (const GLshort *v1, const GLshort *v2);
static GLint	(APIENTRYP dllRenderMode) (GLenum mode);
static void		(APIENTRYP dllRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static void		(APIENTRYP dllRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static void		(APIENTRYP dllScaled) (GLdouble x, GLdouble y, GLdouble z);
static void		(APIENTRYP dllScalef) (GLfloat x, GLfloat y, GLfloat z);
static void		(APIENTRYP dllScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
static void		(APIENTRYP dllSelectBuffer) (GLsizei size, GLuint *buffer);
static void		(APIENTRYP dllShadeModel) (GLenum mode);
static void		(APIENTRYP dllStencilFunc) (GLenum func, GLint ref, GLuint mask);
static void		(APIENTRYP dllStencilMask) (GLuint mask);
static void		(APIENTRYP dllStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
static void		(APIENTRYP dllTexCoord1d) (GLdouble s);
static void		(APIENTRYP dllTexCoord1dv) (const GLdouble *v);
static void		(APIENTRYP dllTexCoord1f) (GLfloat s);
static void		(APIENTRYP dllTexCoord1fv) (const GLfloat *v);
static void		(APIENTRYP dllTexCoord1i) (GLint s);
static void		(APIENTRYP dllTexCoord1iv) (const GLint *v);
static void		(APIENTRYP dllTexCoord1s) (GLshort s);
static void		(APIENTRYP dllTexCoord1sv) (const GLshort *v);
static void		(APIENTRYP dllTexCoord2d) (GLdouble s, GLdouble t);
static void		(APIENTRYP dllTexCoord2dv) (const GLdouble *v);
static void		(APIENTRYP dllTexCoord2f) (GLfloat s, GLfloat t);
static void		(APIENTRYP dllTexCoord2fv) (const GLfloat *v);
static void		(APIENTRYP dllTexCoord2i) (GLint s, GLint t);
static void		(APIENTRYP dllTexCoord2iv) (const GLint *v);
static void		(APIENTRYP dllTexCoord2s) (GLshort s, GLshort t);
static void		(APIENTRYP dllTexCoord2sv) (const GLshort *v);
static void		(APIENTRYP dllTexCoord3d) (GLdouble s, GLdouble t, GLdouble r);
static void		(APIENTRYP dllTexCoord3dv) (const GLdouble *v);
static void		(APIENTRYP dllTexCoord3f) (GLfloat s, GLfloat t, GLfloat r);
static void		(APIENTRYP dllTexCoord3fv) (const GLfloat *v);
static void		(APIENTRYP dllTexCoord3i) (GLint s, GLint t, GLint r);
static void		(APIENTRYP dllTexCoord3iv) (const GLint *v);
static void		(APIENTRYP dllTexCoord3s) (GLshort s, GLshort t, GLshort r);
static void		(APIENTRYP dllTexCoord3sv) (const GLshort *v);
static void		(APIENTRYP dllTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static void		(APIENTRYP dllTexCoord4dv) (const GLdouble *v);
static void		(APIENTRYP dllTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static void		(APIENTRYP dllTexCoord4fv) (const GLfloat *v);
static void		(APIENTRYP dllTexCoord4i) (GLint s, GLint t, GLint r, GLint q);
static void		(APIENTRYP dllTexCoord4iv) (const GLint *v);
static void		(APIENTRYP dllTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q);
static void		(APIENTRYP dllTexCoord4sv) (const GLshort *v);
static void		(APIENTRYP dllTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllTexEnvf) (GLenum target, GLenum pname, GLfloat param);
static void		(APIENTRYP dllTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllTexEnvi) (GLenum target, GLenum pname, GLint param);
static void		(APIENTRYP dllTexEnviv) (GLenum target, GLenum pname, const GLint *params);
static void		(APIENTRYP dllTexGend) (GLenum coord, GLenum pname, GLdouble param);
static void		(APIENTRYP dllTexGendv) (GLenum coord, GLenum pname, const GLdouble *params);
static void		(APIENTRYP dllTexGenf) (GLenum coord, GLenum pname, GLfloat param);
static void		(APIENTRYP dllTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllTexGeni) (GLenum coord, GLenum pname, GLint param);
static void		(APIENTRYP dllTexGeniv) (GLenum coord, GLenum pname, const GLint *params);
static void		(APIENTRYP dllTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void		(APIENTRYP dllTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void		(APIENTRYP dllTexParameterf) (GLenum target, GLenum pname, GLfloat param);
static void		(APIENTRYP dllTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
static void		(APIENTRYP dllTexParameteri) (GLenum target, GLenum pname, GLint param);
static void		(APIENTRYP dllTexParameteriv) (GLenum target, GLenum pname, const GLint *params);
static void		(APIENTRYP dllTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static void		(APIENTRYP dllTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void		(APIENTRYP dllTranslated) (GLdouble x, GLdouble y, GLdouble z);
static void		(APIENTRYP dllTranslatef) (GLfloat x, GLfloat y, GLfloat z);
static void		(APIENTRYP dllVertex2d) (GLdouble x, GLdouble y);
static void		(APIENTRYP dllVertex2dv) (const GLdouble *v);
static void		(APIENTRYP dllVertex2f) (GLfloat x, GLfloat y);
static void		(APIENTRYP dllVertex2fv) (const GLfloat *v);
static void		(APIENTRYP dllVertex2i) (GLint x, GLint y);
static void		(APIENTRYP dllVertex2iv) (const GLint *v);
static void		(APIENTRYP dllVertex2s) (GLshort x, GLshort y);
static void		(APIENTRYP dllVertex2sv) (const GLshort *v);
static void		(APIENTRYP dllVertex3d) (GLdouble x, GLdouble y, GLdouble z);
static void		(APIENTRYP dllVertex3dv) (const GLdouble *v);
static void		(APIENTRYP dllVertex3f) (GLfloat x, GLfloat y, GLfloat z);
static void		(APIENTRYP dllVertex3fv) (const GLfloat *v);
static void		(APIENTRYP dllVertex3i) (GLint x, GLint y, GLint z);
static void		(APIENTRYP dllVertex3iv) (const GLint *v);
static void		(APIENTRYP dllVertex3s) (GLshort x, GLshort y, GLshort z);
static void		(APIENTRYP dllVertex3sv) (const GLshort *v);
static void		(APIENTRYP dllVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void		(APIENTRYP dllVertex4dv) (const GLdouble *v);
static void		(APIENTRYP dllVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void		(APIENTRYP dllVertex4fv) (const GLfloat *v);
static void		(APIENTRYP dllVertex4i) (GLint x, GLint y, GLint z, GLint w);
static void		(APIENTRYP dllVertex4iv) (const GLint *v);
static void		(APIENTRYP dllVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w);
static void		(APIENTRYP dllVertex4sv) (const GLshort *v);
static void		(APIENTRYP dllVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void		(APIENTRYP dllViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

//
// for logging
//
static void APIENTRY logAccum (GLenum op, GLfloat value) {
	fprintf (LOGPROC, "glAccum (%x %f)\n", op, value);
	dllAccum (op, value);
}

static void APIENTRY logAlphaFunc (GLenum func, GLclampf ref) {
	fprintf (LOGPROC, "glAlphaFunc (0x%x, %f)\n", func, ref);
	dllAlphaFunc (func, ref);
}

static GLboolean APIENTRY logAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences) {
	fprintf (LOGPROC, "glAreTexturesResident\n");
	return dllAreTexturesResident (n, textures, residences);
}

static void APIENTRY logArrayElement (GLint i) {
	fprintf (LOGPROC, "glArrayElement\n");
	dllArrayElement (i);
}

static void APIENTRY logBegin (GLenum mode) {
	fprintf (LOGPROC, "glBegin (0x%x)\n", mode);
	dllBegin (mode);
}

static void APIENTRY logBindTexture (GLenum target, GLuint texture) {
	fprintf (LOGPROC, "glBindTexture (0x%x, %u)\n", target, texture);
	dllBindTexture (target, texture);
}

static void APIENTRY logBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
	fprintf (LOGPROC, "glBitmap\n");
	dllBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
}

static void APIENTRY logBlendFunc (GLenum sfactor, GLenum dfactor) {
	fprintf (LOGPROC, "glBlendFunc (0x%x, 0x%x)\n", sfactor, dfactor);
	dllBlendFunc (sfactor, dfactor);
}

static void APIENTRY logCallList (GLuint list) {
	fprintf (LOGPROC, "glCallList (%u)\n", list);
	dllCallList (list);
}

static void APIENTRY logCallLists (GLsizei n, GLenum type, const void *lists) {
	fprintf (LOGPROC, "glCallLists\n");
	dllCallLists (n, type, lists);
}

static void APIENTRY logClear (GLbitfield mask) {
	fprintf (LOGPROC, "glClear\n");
	dllClear (mask);
}

static void APIENTRY logClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	fprintf (LOGPROC, "glClearAccum\n");
	dllClearAccum (red, green, blue, alpha);
}

static void APIENTRY logClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	fprintf (LOGPROC, "glClearColor\n");
	dllClearColor (red, green, blue, alpha);
}

static void APIENTRY logClearDepth (GLclampd depth) {
	fprintf (LOGPROC, "glClearDepth\n");
	dllClearDepth (depth);
}

static void APIENTRY logClearIndex (GLfloat c) {
	fprintf (LOGPROC, "glClearIndex\n");
	dllClearIndex (c);
}

static void APIENTRY logClearStencil (GLint s) {
	fprintf (LOGPROC, "glClearStencil\n");
	dllClearStencil (s);
}

static void APIENTRY logClipPlane (GLenum plane, const GLdouble *equation) {
	fprintf (LOGPROC, "glClipPlane\n");
	dllClipPlane (plane, equation);
}

static void APIENTRY logColor3b (GLbyte red, GLbyte green, GLbyte blue) {
	fprintf (LOGPROC, "glColor3b\n");
	dllColor3b (red, green, blue);
}

static void APIENTRY logColor3bv (const GLbyte *v) {
	fprintf (LOGPROC, "glColor3bv\n");
	dllColor3bv (v);
}

static void APIENTRY logColor3d (GLdouble red, GLdouble green, GLdouble blue) {
	fprintf (LOGPROC, "glColor3d\n");
	dllColor3d (red, green, blue);
}

static void APIENTRY logColor3dv (const GLdouble *v) {
	fprintf (LOGPROC, "glColor3dv\n");
	dllColor3dv (v);
}

static void APIENTRY logColor3f (GLfloat red, GLfloat green, GLfloat blue) {
	fprintf (LOGPROC, "glColor3f\n");
	dllColor3f (red, green, blue);
}

static void APIENTRY logColor3fv (const GLfloat *v) {
	fprintf (LOGPROC, "glColor3fv\n");
	dllColor3fv (v);
}

static void APIENTRY logColor3i (GLint red, GLint green, GLint blue) {
	fprintf (LOGPROC, "glColor3i\n");
	dllColor3i (red, green, blue);
}

static void APIENTRY logColor3iv (const GLint *v) {
	fprintf (LOGPROC, "glColor3iv\n");
	dllColor3iv (v);
}

static void APIENTRY logColor3s (GLshort red, GLshort green, GLshort blue) {
	fprintf (LOGPROC, "glColor3s\n");
	dllColor3s (red, green, blue);
}

static void APIENTRY logColor3sv (const GLshort *v) {
	fprintf (LOGPROC, "glColor3sv\n");
	dllColor3sv (v);
}

static void APIENTRY logColor3ub (GLubyte red, GLubyte green, GLubyte blue) {
	fprintf (LOGPROC, "glColor3ub\n");
	dllColor3ub (red, green, blue);
}

static void APIENTRY logColor3ubv (const GLubyte *v) {
	fprintf (LOGPROC, "glColor3ubv\n");
	dllColor3ubv (v);
}

static void APIENTRY logColor3ui (GLuint red, GLuint green, GLuint blue) {
	SIG ("glColor3ui");
	dllColor3ui (red, green, blue);
}

static void APIENTRY logColor3uiv (const GLuint *v) {
	SIG ("glColor3uiv");
	dllColor3uiv (v);
}

static void APIENTRY logColor3us (GLushort red, GLushort green, GLushort blue) {
	SIG ("glColor3us");
	dllColor3us (red, green, blue);
}

static void APIENTRY logColor3usv (const GLushort *v) {
	SIG ("glColor3usv");
	dllColor3usv (v);
}

static void APIENTRY logColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
	SIG ("glColor4b");
	dllColor4b (red, green, blue, alpha);
}

static void APIENTRY logColor4bv (const GLbyte *v) {
	SIG ("glColor4bv");
	dllColor4bv (v);
}

static void APIENTRY logColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
	SIG ("glColor4d");
	dllColor4d (red, green, blue, alpha);
}

static void APIENTRY logColor4dv (const GLdouble *v) {
	SIG ("glColor4dv");
	dllColor4dv (v);
}

static void APIENTRY logColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	fprintf (LOGPROC, "glColor4f (%f,%f,%f,%f)\n", red, green, blue, alpha);
	dllColor4f (red, green, blue, alpha);
}

static void APIENTRY logColor4fv (const GLfloat *v) {
	fprintf (LOGPROC, "glColor4fv (%f,%f,%f,%f)\n", v[0], v[1], v[2], v[3]);
	dllColor4fv (v);
}

static void APIENTRY logColor4i (GLint red, GLint green, GLint blue, GLint alpha) {
	SIG ("glColor4i");
	dllColor4i (red, green, blue, alpha);
}

static void APIENTRY logColor4iv (const GLint *v) {
	SIG ("glColor4iv");
	dllColor4iv (v);
}

static void APIENTRY logColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha) {
	SIG ("glColor4s");
	dllColor4s (red, green, blue, alpha);
}

static void APIENTRY logColor4sv (const GLshort *v) {
	SIG ("glColor4sv");
	dllColor4sv (v);
}

static void APIENTRY logColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	SIG ("glColor4b");
	dllColor4b (red, green, blue, alpha);
}

static void APIENTRY logColor4ubv (const GLubyte *v) {
	SIG ("glColor4ubv");
	dllColor4ubv (v);
}

static void APIENTRY logColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha) {
	SIG ("glColor4ui");
	dllColor4ui (red, green, blue, alpha);
}

static void APIENTRY logColor4uiv (const GLuint *v) {
	SIG ("glColor4uiv");
	dllColor4uiv (v);
}

static void APIENTRY logColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha) {
	SIG ("glColor4us");
	dllColor4us (red, green, blue, alpha);
}

static void APIENTRY logColor4usv (const GLushort *v) {
	SIG ("glColor4usv");
	dllColor4usv (v);
}

static void APIENTRY logColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	SIG ("glColorMask");
	dllColorMask (red, green, blue, alpha);
}

static void APIENTRY logColorMaterial (GLenum face, GLenum mode) {
	SIG ("glColorMaterial");
	dllColorMaterial (face, mode);
}

static void APIENTRY logColorPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	SIG ("glColorPointer");
	dllColorPointer (size, type, stride, pointer);
}

static void APIENTRY logCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
	SIG ("glCopyPixels");
	dllCopyPixels (x, y, width, height, type);
}

static void APIENTRY logCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) {
	SIG ("glCopyTexImage1D");
	dllCopyTexImage1D (target, level, internalFormat, x, y, width, border);
}

static void APIENTRY logCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	SIG ("glCopyTexImage2D");
	dllCopyTexImage2D (target, level, internalFormat, x, y, width, height, border);
}

static void APIENTRY logCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
	SIG ("glCopyTexSubImage1D");
	dllCopyTexSubImage1D (target, level, xoffset, x, y, width);
}

static void APIENTRY logCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	SIG ("glCopyTexSubImage2D");
	dllCopyTexSubImage2D (target, level, xoffset, yoffset, x, y, width, height);
}

static void APIENTRY logCullFace (GLenum mode) {
	SIG ("glCullFace");
	dllCullFace (mode);
}

static void APIENTRY logDeleteLists (GLuint list, GLsizei range) {
	SIG ("glDeleteLists");
	dllDeleteLists (list, range);
}

static void APIENTRY logDeleteTextures (GLsizei n, const GLuint *textures) {
	SIG ("glDeleteTextures");
	dllDeleteTextures (n, textures);
}

static void APIENTRY logDepthFunc (GLenum func) {
	SIG ("glDepthFunc");
	dllDepthFunc (func);
}

static void APIENTRY logDepthMask (GLboolean flag) {
	SIG ("glDepthMask");
	dllDepthMask (flag);
}

static void APIENTRY logDepthRange (GLclampd zNear, GLclampd zFar) {
	SIG ("glDepthRange");
	dllDepthRange (zNear, zFar);
}

static void APIENTRY logDisable (GLenum cap) {
	fprintf (LOGPROC, "glDisable (0x%x)\n", cap);
	dllDisable (cap);
}

static void APIENTRY logDisableClientState (GLenum array) {
	SIG ("glDisableClientState");
	dllDisableClientState (array);
}

static void APIENTRY logDrawArrays (GLenum mode, GLint first, GLsizei count) {
	SIG ("glDrawArrays");
	dllDrawArrays (mode, first, count);
}

static void APIENTRY logDrawBuffer (GLenum mode) {
	SIG ("glDrawBuffer");
	dllDrawBuffer (mode);
}

static void APIENTRY logDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {
	SIG ("glDrawElements");
	dllDrawElements (mode, count, type, indices);
}

static void APIENTRY logDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	SIG ("glDrawPixels");
	dllDrawPixels (width, height, format, type, pixels);
}

static void APIENTRY logEdgeFlag (GLboolean flag) {
	SIG ("glEdgeFlag");
	dllEdgeFlag (flag);
}

static void APIENTRY logEdgeFlagPointer (GLsizei stride, const void *pointer) {
	SIG ("glEdgeFlagPointer");
	dllEdgeFlagPointer (stride, pointer);
}

static void APIENTRY logEdgeFlagv (const GLboolean *flag) {
	SIG ("glEdgeFlagv");
	dllEdgeFlagv (flag);
}

static void APIENTRY logEnable (GLenum cap) {
	fprintf (LOGPROC, "glEnable (0x%x)\n", cap);
	dllEnable (cap);
}

static void APIENTRY logEnableClientState (GLenum array) {
	SIG ("glEnableClientState");
	dllEnableClientState (array);
}

static void APIENTRY logEnd (void) {
	SIG ("glEnd");
	dllEnd ();
}

static void APIENTRY logEndList (void) {
	SIG ("glEndList");
	dllEndList ();
}

static void APIENTRY logEvalCoord1d (GLdouble u) {
	SIG ("glEvalCoord1d");
	dllEvalCoord1d (u);
}

static void APIENTRY logEvalCoord1dv (const GLdouble *u) {
	SIG ("glEvalCoord1dv");
	dllEvalCoord1dv (u);
}

static void APIENTRY logEvalCoord1f (GLfloat u) {
	SIG ("glEvalCoord1f");
	dllEvalCoord1f (u);
}

static void APIENTRY logEvalCoord1fv (const GLfloat *u) {
	SIG ("glEvalCoord1fv");
	dllEvalCoord1fv (u);
}
static void APIENTRY logEvalCoord2d (GLdouble u, GLdouble v) {
	SIG ("glEvalCoord2d");
	dllEvalCoord2d (u, v);
}
static void APIENTRY logEvalCoord2dv (const GLdouble *u) {
	SIG ("glEvalCoord2dv");
	dllEvalCoord2dv (u);
}
static void APIENTRY logEvalCoord2f (GLfloat u, GLfloat v) {
	SIG ("glEvalCoord2f");
	dllEvalCoord2f (u, v);
}
static void APIENTRY logEvalCoord2fv (const GLfloat *u) {
	SIG ("glEvalCoord2fv");
	dllEvalCoord2fv (u);
}

static void APIENTRY logEvalMesh1 (GLenum mode, GLint i1, GLint i2) {
	SIG ("glEvalMesh1");
	dllEvalMesh1 (mode, i1, i2);
}

static void APIENTRY logEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
	SIG ("glEvalMesh2");
	dllEvalMesh2 (mode, i1, i2, j1, j2);
}

static void APIENTRY logEvalPoint1 (GLint i) {
	SIG ("glEvalPoint1");
	dllEvalPoint1 (i);
}

static void APIENTRY logEvalPoint2 (GLint i, GLint j) {
	SIG ("glEvalPoint2");
	dllEvalPoint2 (i, j);
}

static void APIENTRY logFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer) {
	SIG ("glFeedbackBuffer");
	dllFeedbackBuffer (size, type, buffer);
}

static void APIENTRY logFinish (void) {
	SIG ("glFinish");
	dllFinish ();
}

static void APIENTRY logFlush (void) {
	SIG ("glFlush");
	dllFlush ();
}

static void APIENTRY logFogf (GLenum pname, GLfloat param) {
	SIG ("glFogf");
	dllFogf (pname, param);
}

static void APIENTRY logFogfv (GLenum pname, const GLfloat *params) {
	SIG ("glFogfv");
	dllFogfv (pname, params);
}

static void APIENTRY logFogi (GLenum pname, GLint param) {
	SIG ("glFogi");
	dllFogi (pname, param);
}

static void APIENTRY logFogiv (GLenum pname, const GLint *params) {
	SIG ("glFogiv");
	dllFogiv (pname, params);
}

static void APIENTRY logFrontFace (GLenum mode) {
	SIG ("glFrontFace");
	dllFrontFace (mode);
}

static void APIENTRY logFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	SIG ("glFrustum");
	dllFrustum (left, right, bottom, top, zNear, zFar);
}

static GLuint APIENTRY logGenLists (GLsizei range) {
	SIG ("glGenLists");
	return dllGenLists (range);
}

static void APIENTRY logGenTextures (GLsizei n, GLuint *textures) {
	SIG ("glGenTextures");
	dllGenTextures (n, textures);
}

static void APIENTRY logGetBooleanv (GLenum pname, GLboolean *params) {
	SIG ("glGetBooleanv");
	dllGetBooleanv (pname, params);
}

static void APIENTRY logGetClipPlane (GLenum plane, GLdouble *equation) {
	SIG ("glGetClipPlane");
	dllGetClipPlane (plane, equation);
}

static void APIENTRY logGetDoublev (GLenum pname, GLdouble *params) {
	SIG ("glGetDoublev");
	dllGetDoublev (pname, params);
}

static GLenum APIENTRY logGetError (void) {
	SIG ("glGetError");
	return dllGetError ();
}

static void APIENTRY logGetFloatv (GLenum pname, GLfloat *params) {
	SIG ("glGetFloatv");
	dllGetFloatv (pname, params);
}

static void APIENTRY logGetIntegerv (GLenum pname, GLint *params) {
	SIG ("glGetIntegerv");
	dllGetIntegerv (pname, params);
}

static void APIENTRY logGetLightfv (GLenum light, GLenum pname, GLfloat *params) {
	SIG ("glGetLightfv");
	dllGetLightfv (light, pname, params);
}

static void APIENTRY logGetLightiv (GLenum light, GLenum pname, GLint *params) {
	SIG ("glGetLightiv");
	dllGetLightiv (light, pname, params);
}

static void APIENTRY logGetMapdv (GLenum target, GLenum query, GLdouble *v) {
	SIG ("glGetMapdv");
	dllGetMapdv (target, query, v);
}

static void APIENTRY logGetMapfv (GLenum target, GLenum query, GLfloat *v) {
	SIG ("glGetMapfv");
	dllGetMapfv (target, query, v);
}

static void APIENTRY logGetMapiv (GLenum target, GLenum query, GLint *v) {
	SIG ("glGetMapiv");
	dllGetMapiv (target, query, v);
}

static void APIENTRY logGetMaterialfv (GLenum face, GLenum pname, GLfloat *params) {
	SIG ("glGetMaterialfv");
	dllGetMaterialfv (face, pname, params);
}

static void APIENTRY logGetMaterialiv (GLenum face, GLenum pname, GLint *params) {
	SIG ("glGetMaterialiv");
	dllGetMaterialiv (face, pname, params);
}

static void APIENTRY logGetPixelMapfv (GLenum map, GLfloat *values) {
	SIG ("glGetPixelMapfv");
	dllGetPixelMapfv (map, values);
}

static void APIENTRY logGetPixelMapuiv (GLenum map, GLuint *values) {
	SIG ("glGetPixelMapuiv");
	dllGetPixelMapuiv (map, values);
}

static void APIENTRY logGetPixelMapusv (GLenum map, GLushort *values) {
	SIG ("glGetPixelMapusv");
	dllGetPixelMapusv (map, values);
}

static void APIENTRY logGetPointerv (GLenum pname, GLvoid* *params) {
	SIG ("glGetPointerv");
	dllGetPointerv (pname, params);
}

static void APIENTRY logGetPolygonStipple (GLubyte *mask) {
	SIG ("glGetPolygonStipple");
	dllGetPolygonStipple (mask);
}

static const GLubyte * APIENTRY logGetString (GLenum name) {
	SIG ("glGetString");
	return dllGetString (name);
}

static void APIENTRY logGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params){
	SIG ("glGetTexEnvfv");
	dllGetTexEnvfv (target, pname, params);
}

static void APIENTRY logGetTexEnviv (GLenum target, GLenum pname, GLint *params) {
	SIG ("glGetTexEnviv");
	dllGetTexEnviv (target, pname, params);
}

static void APIENTRY logGetTexGendv (GLenum coord, GLenum pname, GLdouble *params) {
	SIG ("glGetTexGendv");
	dllGetTexGendv (coord, pname, params);
}

static void APIENTRY logGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params) {
	SIG ("glGetTexGenfv");
	dllGetTexGenfv (coord, pname, params);
}

static void APIENTRY logGetTexGeniv (GLenum coord, GLenum pname, GLint *params) {
	SIG ("glGetTexGeniv");
	dllGetTexGeniv (coord, pname, params);
}

static void APIENTRY logGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, void *pixels) {
	SIG ("glGetTexImage");
	dllGetTexImage (target, level, format, type, pixels);
}
static void APIENTRY logGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params) {
	SIG ("glGetTexLevelParameterfv");
	dllGetTexLevelParameterfv (target, level, pname, params);
}

static void APIENTRY logGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params) {
	SIG ("glGetTexLevelParameteriv");
	dllGetTexLevelParameteriv (target, level, pname, params);
}

static void APIENTRY logGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {
	SIG ("glGetTexParameterfv");
	dllGetTexParameterfv (target, pname, params);
}

static void APIENTRY logGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {
	SIG ("glGetTexParameteriv");
	dllGetTexParameteriv (target, pname, params);
}

static void APIENTRY logHint (GLenum target, GLenum mode) {
	fprintf (LOGPROC, "glHint (0x%x, 0x%x)\n", target, mode);
	dllHint (target, mode);
}

static void APIENTRY logIndexMask (GLuint mask) {
	SIG ("glIndexMask");
	dllIndexMask (mask);
}

static void APIENTRY logIndexPointer (GLenum type, GLsizei stride, const void *pointer) {
	SIG ("glIndexPointer");
	dllIndexPointer (type, stride, pointer);
}

static void APIENTRY logIndexd (GLdouble c) {
	SIG ("glIndexd");
	dllIndexd (c);
}

static void APIENTRY logIndexdv (const GLdouble *c) {
	SIG ("glIndexdv");
	dllIndexdv (c);
}

static void APIENTRY logIndexf (GLfloat c) {
	SIG ("glIndexf");
	dllIndexf (c);
}

static void APIENTRY logIndexfv (const GLfloat *c) {
	SIG ("glIndexfv");
	dllIndexfv (c);
}

static void APIENTRY logIndexi (GLint c) {
	SIG ("glIndexi");
	dllIndexi (c);
}

static void APIENTRY logIndexiv (const GLint *c) {
	SIG ("glIndexiv");
	dllIndexiv (c);
}

static void APIENTRY logIndexs (GLshort c) {
	SIG ("glIndexs");
	dllIndexs (c);
}

static void APIENTRY logIndexsv (const GLshort *c) {
	SIG ("glIndexsv");
	dllIndexsv (c);
}

static void APIENTRY logIndexub (GLubyte c) {
	SIG ("glIndexub");
	dllIndexub (c);
}

static void APIENTRY logIndexubv (const GLubyte *c) {
	SIG ("glIndexubv");
	dllIndexubv (c);
}

static void APIENTRY logInitNames (void) {
	SIG ("glInitNames");
	dllInitNames ();
}

static void APIENTRY logInterleavedArrays (GLenum format, GLsizei stride, const void *pointer) {
	SIG ("glInterleavedArrays");
	dllInterleavedArrays (format, stride, pointer);
}

static GLboolean APIENTRY logIsEnabled (GLenum cap) {
	SIG ("glIsEnabled");
	return dllIsEnabled (cap);
}

static GLboolean APIENTRY logIsList (GLuint list) {
	SIG ("glIsList");
	return dllIsList (list);
}

static GLboolean APIENTRY logIsTexture (GLuint texture) {
	SIG ("glIsTexture");
	return dllIsTexture (texture);
}

static void APIENTRY logLightModelf (GLenum pname, GLfloat param) {
	SIG ("glLightModelf");
	dllLightModelf (pname, param);
}

static void APIENTRY logLightModelfv (GLenum pname, const GLfloat *params) {
	SIG ("glLightModelfv");
	dllLightModelfv (pname, params);
}

static void APIENTRY logLightModeli (GLenum pname, GLint param) {
	SIG ("glLightModeli");
	dllLightModeli (pname, param);

}

static void APIENTRY logLightModeliv (GLenum pname, const GLint *params) {
	SIG ("glLightModeliv");
	dllLightModeliv (pname, params);
}

static void APIENTRY logLightf (GLenum light, GLenum pname, GLfloat param) {
	SIG ("glLightf");
	dllLightf (light, pname, param);
}

static void APIENTRY logLightfv (GLenum light, GLenum pname, const GLfloat *params) {
	SIG ("glLightfv");
	dllLightfv (light, pname, params);
}

static void APIENTRY logLighti (GLenum light, GLenum pname, GLint param) {
	SIG ("glLighti");
	dllLighti (light, pname, param);
}

static void APIENTRY logLightiv (GLenum light, GLenum pname, const GLint *params) {
	SIG ("glLightiv");
	dllLightiv (light, pname, params);
}

static void APIENTRY logLineStipple (GLint factor, GLushort pattern) {
	SIG ("glLineStipple");
	dllLineStipple (factor, pattern);
}

static void APIENTRY logLineWidth (GLfloat width) {
	SIG ("glLineWidth");
	dllLineWidth (width);
}

static void APIENTRY logListBase (GLuint base) {
	SIG ("glListBase");
	dllListBase (base);
}

static void APIENTRY logLoadIdentity (void) {
	SIG ("glLoadIdentity");
	dllLoadIdentity ();
}

static void APIENTRY logLoadMatrixd (const GLdouble *m) {
	SIG ("glLoadMatrixd");
	dllLoadMatrixd (m);
}

static void APIENTRY logLoadMatrixf (const GLfloat *m){
	SIG ("glLoadMatrixf");
	dllLoadMatrixf (m);
}

static void APIENTRY logLoadName (GLuint name) {
	SIG ("glLoadName");
	dllLoadName (name);
}

static void APIENTRY logLogicOp (GLenum opcode) {
	SIG ("glLogicOp");
	dllLogicOp (opcode);
}

static void APIENTRY logMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
	SIG ("glMap1d");
	dllMap1d (target, u1, u2, stride, order, points);
}

static void APIENTRY logMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
	SIG ("glMap1f");
	dllMap1f (target, u1, u2, stride, order, points);
}

static void APIENTRY logMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
	SIG ("glMap2d");
	dllMap2d (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static void APIENTRY logMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
	SIG ("glMap2f");
	dllMap2f (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static void APIENTRY logMapGrid1d (GLint un, GLdouble u1, GLdouble u2) {
	SIG ("glMapGrid1d");
	dllMapGrid1d (un, u1, u2);
}

static void APIENTRY logMapGrid1f (GLint un, GLfloat u1, GLfloat u2) {
	SIG ("glMapGrid1f");
	dllMapGrid1f (un, u1, u2);
}

static void APIENTRY logMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
	SIG ("glMapGrid2d");
	dllMapGrid2d (un, u1, u2, vn, v1, v2);
}

static void APIENTRY logMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
	SIG ("glMapGrid2f");
	dllMapGrid2f (un, u1, u2, vn, v1, v2);
}

static void APIENTRY logMaterialf (GLenum face, GLenum pname, GLfloat param) {
	SIG ("glMaterialf");
	dllMaterialf (face, pname, param);
}

static void APIENTRY logMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {
	SIG ("glMaterialfv");
	dllMaterialfv (face, pname, params);
}

static void APIENTRY logMateriali (GLenum face, GLenum pname, GLint param) {
	SIG ("glMateriali");
	dllMateriali (face, pname, param);
}

static void APIENTRY logMaterialiv (GLenum face, GLenum pname, const GLint *params) {
	SIG ("glMaterialiv");
	dllMaterialiv (face, pname, params);
}

static void APIENTRY logMatrixMode (GLenum mode) {
	SIG ("glMatrixMode");
	dllMatrixMode (mode);
}

static void APIENTRY logMultMatrixd (const GLdouble *m) {
	SIG ("glMultMatrixd");
	dllMultMatrixd (m);
}

static void APIENTRY logMultMatrixf (const GLfloat *m) {
	SIG ("glMultMatrixf");
	dllMultMatrixf (m);
}

static void APIENTRY logNewList (GLuint list, GLenum mode) {
	SIG ("glNewList");
	dllNewList (list, mode);
}

static void APIENTRY logNormal3b (GLbyte nx, GLbyte ny, GLbyte nz) {
	SIG ("glNormal3b");
	dllNormal3b (nx, ny, nz);
}

static void APIENTRY logNormal3bv (const GLbyte *v) {
	SIG ("glNormal3bv");
	dllNormal3bv (v);
}

static void APIENTRY logNormal3d (GLdouble nx, GLdouble ny, GLdouble nz) {
	SIG ("glNormal3d");
	dllNormal3d (nx, ny, nz);
}

static void APIENTRY logNormal3dv (const GLdouble *v) {
	SIG ("glNormal3dv");
	dllNormal3dv (v);
}

static void APIENTRY logNormal3f (GLfloat nx, GLfloat ny, GLfloat nz) {
	SIG ("glNormal3f");
	dllNormal3f (nx, ny, nz);
}

static void APIENTRY logNormal3fv (const GLfloat *v) {
	SIG ("glNormal3fv");
	dllNormal3fv (v);
}
static void APIENTRY logNormal3i (GLint nx, GLint ny, GLint nz) {
	SIG ("glNormal3i");
	dllNormal3i (nx, ny, nz);
}
static void APIENTRY logNormal3iv (const GLint *v) {
	SIG ("glNormal3iv");
	dllNormal3iv (v);
}
static void APIENTRY logNormal3s (GLshort nx, GLshort ny, GLshort nz) {
	SIG ("glNormal3s");
	dllNormal3s (nx, ny, nz);
}
static void APIENTRY logNormal3sv (const GLshort *v) {
	SIG ("glNormal3sv");
	dllNormal3sv (v);
}
static void APIENTRY logNormalPointer (GLenum type, GLsizei stride, const void *pointer) {
	SIG ("glNormalPointer");
	dllNormalPointer (type, stride, pointer);
}
static void APIENTRY logOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	SIG ("glOrtho");
	dllOrtho (left, right, bottom, top, zNear, zFar);
}

static void APIENTRY logPassThrough (GLfloat token) {
	SIG ("glPassThrough");
	dllPassThrough (token);
}

static void APIENTRY logPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values) {
	SIG ("glPixelMapfv");
	dllPixelMapfv (map, mapsize, values);
}

static void APIENTRY logPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values) {
	SIG ("glPixelMapuiv");
	dllPixelMapuiv (map, mapsize, values);
}

static void APIENTRY logPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values) {
	SIG ("glPixelMapusv");
	dllPixelMapusv (map, mapsize, values);
}

static void APIENTRY logPixelStoref (GLenum pname, GLfloat param) {
	SIG ("glPixelStoref");
	dllPixelStoref (pname, param);
}

static void APIENTRY logPixelStorei (GLenum pname, GLint param) {
	SIG ("glPixelStorei");
	dllPixelStorei (pname, param);
}

static void APIENTRY logPixelTransferf (GLenum pname, GLfloat param) {
	SIG ("glPixelTransferf");
	dllPixelTransferf (pname, param);
}

static void APIENTRY logPixelTransferi (GLenum pname, GLint param) {
	SIG ("glPixelTransferi");
	dllPixelTransferi (pname, param);
}

static void APIENTRY logPixelZoom (GLfloat xfactor, GLfloat yfactor) {
	SIG ("glPixelZoom");
	dllPixelZoom (xfactor, yfactor);
}

static void APIENTRY logPointSize (GLfloat size) {
	SIG ("glPointSize");
	dllPointSize (size);
}

static void APIENTRY logPolygonMode (GLenum face, GLenum mode) {
	fprintf (LOGPROC, "glPolygonMode (0x%x, 0x%x)\n", face, mode);
	dllPolygonMode (face, mode);
}

static void APIENTRY logPolygonOffset (GLfloat factor, GLfloat units) {
	SIG ("glPolygonOffset");
	dllPolygonOffset (factor, units);
}

static void APIENTRY logPolygonStipple (const GLubyte *mask) {
	SIG ("glPolygonStipple");
	dllPolygonStipple (mask);
}

static void APIENTRY logPopAttrib (void) {
	SIG ("glPopAttrib");
	dllPopAttrib ();
}

static void APIENTRY logPopClientAttrib (void) {
	SIG ("glPopClientAttrib");
	dllPopClientAttrib ();
}

static void APIENTRY logPopMatrix (void) {
	SIG ("glPopMatrix");
	dllPopMatrix ();
}

static void APIENTRY logPopName (void) {
	SIG ("glPopName");
	dllPopName ();
}

static void APIENTRY logPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities) {
	SIG ("glPrioritizeTextures");
	dllPrioritizeTextures (n, textures, priorities);
}

static void APIENTRY logPushAttrib (GLbitfield mask) {
	SIG ("glPushAttrib");
	dllPushAttrib (mask);
}

static void APIENTRY logPushClientAttrib (GLbitfield mask) {
	SIG ("glPushClientAttrib");
	dllPushClientAttrib (mask);
}

static void APIENTRY logPushMatrix (void) {
	SIG ("glPushMatrix");
	dllPushMatrix ();
}

static void APIENTRY logPushName (GLuint name) {
	SIG ("glPushName");
	dllPushName (name);
}

static void APIENTRY logRasterPos2d (GLdouble x, GLdouble y) {
	SIG ("glRasterPot2d");
	dllRasterPos2d (x, y);
}

static void APIENTRY logRasterPos2dv (const GLdouble *v) {
	SIG ("glRasterPos2dv");
	dllRasterPos2dv (v);
}

static void APIENTRY logRasterPos2f (GLfloat x, GLfloat y) {
	SIG ("glRasterPos2f");
	dllRasterPos2f (x, y);
}

static void APIENTRY logRasterPos2fv (const GLfloat *v) {
	SIG ("glRasterPos2dv");
	dllRasterPos2fv (v);
}

static void APIENTRY logRasterPos2i (GLint x, GLint y) {
	SIG ("glRasterPos2if");
	dllRasterPos2i (x, y);
}

static void APIENTRY logRasterPos2iv (const GLint *v) {
	SIG ("glRasterPos2iv");
	dllRasterPos2iv (v);
}

static void APIENTRY logRasterPos2s (GLshort x, GLshort y) {
	SIG ("glRasterPos2s");
	dllRasterPos2s (x, y);
}

static void APIENTRY logRasterPos2sv (const GLshort *v) {
	SIG ("glRasterPos2sv");
	dllRasterPos2sv (v);
}

static void APIENTRY logRasterPos3d (GLdouble x, GLdouble y, GLdouble z) {
	SIG ("glRasterPos3d");
	dllRasterPos3d (x, y, z);
}

static void APIENTRY logRasterPos3dv (const GLdouble *v) {
	SIG ("glRasterPos3dv");
	dllRasterPos3dv (v);
}

static void APIENTRY logRasterPos3f (GLfloat x, GLfloat y, GLfloat z) {
	SIG ("glRasterPos3f");
	dllRasterPos3f (x, y, z);
}

static void APIENTRY logRasterPos3fv (const GLfloat *v) {
	SIG ("glRasterPos3fv");
	dllRasterPos3fv (v);
}

static void APIENTRY logRasterPos3i (GLint x, GLint y, GLint z) {
	SIG ("glRasterPos3i");
	dllRasterPos3i (x, y, z);
}

static void APIENTRY logRasterPos3iv (const GLint *v) {
	SIG ("glRasterPos3iv");
	dllRasterPos3iv (v);
}

static void APIENTRY logRasterPos3s (GLshort x, GLshort y, GLshort z) {
	SIG ("glRasterPos3s");
	dllRasterPos3s (x, y, z);
}

static void APIENTRY logRasterPos3sv (const GLshort *v) {
	SIG ("glRasterPos3sv");
	dllRasterPos3sv (v);
}

static void APIENTRY logRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	SIG ("glRasterPos4d");
	dllRasterPos4d (x, y, z, w);
}

static void APIENTRY logRasterPos4dv (const GLdouble *v) {
	SIG ("glRasterPos4dv");
	dllRasterPos4dv (v);
}

static void APIENTRY logRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	SIG ("glRasterPos4f");
	dllRasterPos4f (x, y, z, w);
}

static void APIENTRY logRasterPos4fv (const GLfloat *v) {
	SIG ("glRasterPos4fv");
	dllRasterPos4fv (v);
}

static void APIENTRY logRasterPos4i (GLint x, GLint y, GLint z, GLint w) {
	SIG ("glRasterPos4i");
	dllRasterPos4i (x, y, z, w);
}

static void APIENTRY logRasterPos4iv (const GLint *v) {
	SIG ("glRasterPos4iv");
	dllRasterPos4iv (v);
}

static void APIENTRY logRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	SIG ("glRasterPos4s");
	dllRasterPos4s (x, y, z, w);
}

static void APIENTRY logRasterPos4sv (const GLshort *v) {
	SIG ("glRasterPos4sv");
	dllRasterPos4sv (v);
}

static void APIENTRY logReadBuffer (GLenum mode) {
	SIG ("glReadBuffer");
	dllReadBuffer (mode);
}

static void APIENTRY logReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	SIG ("glReadPixels");
	dllReadPixels (x, y, width, height, format, type, pixels);
}

static void APIENTRY logRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
	SIG ("glRectd");
	dllRectd (x1, y1, x2, y2);
}

static void APIENTRY logRectdv (const GLdouble *v1, const GLdouble *v2) {
	SIG ("glRectdv");
	dllRectdv (v1, v2);
}

static void APIENTRY logRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	SIG ("glRectf");
	dllRectf (x1, y1, x2, y2);
}

static void APIENTRY logRectfv (const GLfloat *v1, const GLfloat *v2) {
	SIG ("glRectfv");
	dllRectfv (v1, v2);
}

static void APIENTRY logRecti (GLint x1, GLint y1, GLint x2, GLint y2) {
	SIG ("glRecti");
	dllRecti (x1, y1, x2, y2);
}

static void APIENTRY logRectiv (const GLint *v1, const GLint *v2) {
	SIG ("glRectiv");
	dllRectiv (v1, v2);
}

static void APIENTRY logRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
	SIG ("glRects");
	dllRects (x1, y1, x2, y2);
}

static void APIENTRY logRectsv (const GLshort *v1, const GLshort *v2) {
	SIG ("glRectsv");
	dllRectsv (v1, v2);
}

static GLint APIENTRY logRenderMode (GLenum mode) {
	SIG ("glRenderMode");
	return dllRenderMode (mode);
}

static void APIENTRY logRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
	SIG ("glRotated");
	dllRotated (angle, x, y, z);
}

static void APIENTRY logRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	SIG ("glRotatef");
	dllRotatef (angle, x, y, z);
}

static void APIENTRY logScaled (GLdouble x, GLdouble y, GLdouble z) {
	SIG ("glScaled");
	dllScaled (x, y, z);
}

static void APIENTRY logScalef (GLfloat x, GLfloat y, GLfloat z) {
	SIG ("glScalef");
	dllScalef (x, y, z);
}

static void APIENTRY logScissor (GLint x, GLint y, GLsizei width, GLsizei height) {
	SIG ("glScissor");
	dllScissor (x, y, width, height);
}

static void APIENTRY logSelectBuffer (GLsizei size, GLuint *buffer) {
	SIG ("glSelectBuffer");
	dllSelectBuffer (size, buffer);
}

static void APIENTRY logShadeModel (GLenum mode) {
	SIG ("glShadeModel");
	dllShadeModel (mode);
}

static void APIENTRY logStencilFunc (GLenum func, GLint ref, GLuint mask) {
	SIG ("glStencilFunc");
	dllStencilFunc (func, ref, mask);
}

static void APIENTRY logStencilMask (GLuint mask) {
	SIG ("glStencilMask");
	dllStencilMask (mask);
}

static void APIENTRY logStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
	SIG ("glStencilOp");
	dllStencilOp (fail, zfail, zpass);
}

static void APIENTRY logTexCoord1d (GLdouble s) {
	SIG ("glTexCoord1d");
	dllTexCoord1d (s);
}

static void APIENTRY logTexCoord1dv (const GLdouble *v) {
	SIG ("glTexCoord1dv");
	dllTexCoord1dv (v);
}

static void APIENTRY logTexCoord1f (GLfloat s) {
	SIG ("glTexCoord1f");
	dllTexCoord1f (s);
}

static void APIENTRY logTexCoord1fv (const GLfloat *v) {
	SIG ("glTexCoord1fv");
	dllTexCoord1fv (v);
}

static void APIENTRY logTexCoord1i (GLint s) {
	SIG ("glTexCoord1i");
	dllTexCoord1i (s);
}

static void APIENTRY logTexCoord1iv (const GLint *v) {
	SIG ("glTexCoord1iv");
	dllTexCoord1iv (v);
}

static void APIENTRY logTexCoord1s (GLshort s) {
	SIG ("glTexCoord1s");
	dllTexCoord1s (s);
}

static void APIENTRY logTexCoord1sv (const GLshort *v) {
	SIG ("glTexCoord1sv");
	dllTexCoord1sv (v);
}

static void APIENTRY logTexCoord2d (GLdouble s, GLdouble t) {
	SIG ("glTexCoord2d");
	dllTexCoord2d (s, t);
}

static void APIENTRY logTexCoord2dv (const GLdouble *v) {
	SIG ("glTexCoord2dv");
	dllTexCoord2dv (v);
}

static void APIENTRY logTexCoord2f (GLfloat s, GLfloat t) {
	SIG ("glTexCoord2f");
	dllTexCoord2f (s, t);
}

static void APIENTRY logTexCoord2fv (const GLfloat *v) {
	SIG ("glTexCoord2fv");
	dllTexCoord2fv (v);
}

static void APIENTRY logTexCoord2i (GLint s, GLint t) {
	SIG ("glTexCoord2i");
	dllTexCoord2i (s, t);
}

static void APIENTRY logTexCoord2iv (const GLint *v) {
	SIG ("glTexCoord2iv");
	dllTexCoord2iv (v);
}

static void APIENTRY logTexCoord2s (GLshort s, GLshort t) {
	SIG ("glTexCoord2s");
	dllTexCoord2s (s, t);
}

static void APIENTRY logTexCoord2sv (const GLshort *v) {
	SIG ("glTexCoord2sv");
	dllTexCoord2sv (v);
}

static void APIENTRY logTexCoord3d (GLdouble s, GLdouble t, GLdouble r) {
	SIG ("glTexCoord3d");
	dllTexCoord3d (s, t, r);
}

static void APIENTRY logTexCoord3dv (const GLdouble *v) {
	SIG ("glTexCoord3dv");
	dllTexCoord3dv (v);
}

static void APIENTRY logTexCoord3f (GLfloat s, GLfloat t, GLfloat r) {
	SIG ("glTexCoord3f");
	dllTexCoord3f (s, t, r);
}

static void APIENTRY logTexCoord3fv (const GLfloat *v) {
	SIG ("glTexCoord3fv");
	dllTexCoord3fv (v);
}

static void APIENTRY logTexCoord3i (GLint s, GLint t, GLint r) {
	SIG ("glTexCoord3i");
	dllTexCoord3i (s, t, r);
}

static void APIENTRY logTexCoord3iv (const GLint *v) {
	SIG ("glTexCoord3iv");
	dllTexCoord3iv (v);
}

static void APIENTRY logTexCoord3s (GLshort s, GLshort t, GLshort r) {
	SIG ("glTexCoord3s");
	dllTexCoord3s (s, t, r);
}

static void APIENTRY logTexCoord3sv (const GLshort *v) {
	SIG ("glTexCoord3sv");
	dllTexCoord3sv (v);
}

static void APIENTRY logTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
	SIG ("glTexCoord4d");
	dllTexCoord4d (s, t, r, q);
}

static void APIENTRY logTexCoord4dv (const GLdouble *v) {
	SIG ("glTexCoord4dv");
	dllTexCoord4dv (v);
}

static void APIENTRY logTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
	SIG ("glTexCoord4f");
	dllTexCoord4f (s, t, r, q);
}

static void APIENTRY logTexCoord4fv (const GLfloat *v) {
	SIG ("glTexCoord4fv");
	dllTexCoord4fv (v);
}

static void APIENTRY logTexCoord4i (GLint s, GLint t, GLint r, GLint q) {
	SIG ("glTexCoord4i");
	dllTexCoord4i (s, t, r, q);
}

static void APIENTRY logTexCoord4iv (const GLint *v) {
	SIG ("glTexCoord4iv");
	dllTexCoord4iv (v);
}

static void APIENTRY logTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q) {
	SIG ("glTexCoord4s");
	dllTexCoord4s (s, t, r, q);
}

static void APIENTRY logTexCoord4sv (const GLshort *v) {
	SIG ("glTexCoord4sv");
	dllTexCoord4sv (v);
}

static void APIENTRY logTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	SIG ("glTexCoordPointer");
	dllTexCoordPointer (size, type, stride, pointer);
}

static void APIENTRY logTexEnvf (GLenum target, GLenum pname, GLfloat param) {
	fprintf (LOGPROC, "glTexEnvf (0x%x, 0x%x, %f)\n", target, pname, param);
	dllTexEnvf (target, pname, param);
}

static void APIENTRY logTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) {
	SIG ("glTexEnvfv");
	dllTexEnvfv (target, pname, params);
}

static void APIENTRY logTexEnvi (GLenum target, GLenum pname, GLint param) {
	fprintf (LOGPROC, "glTexEnvi (0x%x, 0x%x, 0x%x)\n", target, pname, param);
	dllTexEnvi (target, pname, param);
}

static void APIENTRY logTexEnviv (GLenum target, GLenum pname, const GLint *params) {
	SIG ("glTexEnviv");
	dllTexEnviv (target, pname, params);
}

static void APIENTRY logTexGend (GLenum coord, GLenum pname, GLdouble param) {
	SIG ("glTexGend");
	dllTexGend (coord, pname, param);
}

static void APIENTRY logTexGendv (GLenum coord, GLenum pname, const GLdouble *params) {
	SIG ("glTexGendv");
	dllTexGendv (coord, pname, params);
}

static void APIENTRY logTexGenf (GLenum coord, GLenum pname, GLfloat param) {
	SIG ("glTexGenf");
	dllTexGenf (coord, pname, param);
}
static void APIENTRY logTexGenfv (GLenum coord, GLenum pname, const GLfloat *params) {
	SIG ("glTexGenfv");
	dllTexGenfv (coord, pname, params);
}

static void APIENTRY logTexGeni (GLenum coord, GLenum pname, GLint param) {
	SIG ("glTexGeni");
	dllTexGeni (coord, pname, param);
}

static void APIENTRY logTexGeniv (GLenum coord, GLenum pname, const GLint *params) {
	SIG ("glTexGeniv");
	dllTexGeniv (coord, pname, params);
}

static void APIENTRY logTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) {
	SIG ("glTexImage1D");
	dllTexImage1D (target, level, internalformat, width, border, format, type, pixels);
}

static void APIENTRY logTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	SIG ("glTexImage2D");
	dllTexImage2D (target, level, internalformat, width, height, border, format, type, pixels);
}

static void APIENTRY logTexParameterf (GLenum target, GLenum pname, GLfloat param) {
	fprintf (LOGPROC, "glTexParameterf (0x%x, 0x%x, %f)\n", target, pname, param);
	dllTexParameterf (target, pname, param);
}

static void APIENTRY logTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {
	SIG ("glTexParameterfv");
	dllTexParameterfv (target, pname, params);
}

static void APIENTRY logTexParameteri (GLenum target, GLenum pname, GLint param) {
	fprintf (LOGPROC, "glTexParameteri (0x%x, 0x%x, 0x%x)\n", target, pname, param);
	dllTexParameteri (target, pname, param);
}

static void APIENTRY logTexParameteriv (GLenum target, GLenum pname, const GLint *params) {
	SIG ("glTexParameteriv");
	dllTexParameteriv (target, pname, params);
}

static void APIENTRY logTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
	SIG ("glTexSubImage1D");
	dllTexSubImage1D (target, level, xoffset, width, format, type, pixels);
}

static void APIENTRY logTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	SIG ("glTexSubImage2D");
	dllTexSubImage2D (target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static void APIENTRY logTranslated (GLdouble x, GLdouble y, GLdouble z) {
	SIG ("glTranslated");
	dllTranslated (x, y, z);
}

static void APIENTRY logTranslatef (GLfloat x, GLfloat y, GLfloat z) {
	SIG ("glTranslatef");
	dllTranslatef (x, y, z);
}

static void APIENTRY logVertex2d (GLdouble x, GLdouble y) {
	SIG ("glVertex2d");
	dllVertex2d (x, y);
}

static void APIENTRY logVertex2dv (const GLdouble *v) {
	SIG ("glVertex2dv");
	dllVertex2dv (v);
}

static void APIENTRY logVertex2f (GLfloat x, GLfloat y) {
	SIG ("glVertex2f");
	dllVertex2f (x, y);
}

static void APIENTRY logVertex2fv (const GLfloat *v) {
	SIG ("glVertex2fv");
	dllVertex2fv (v);
}

static void APIENTRY logVertex2i (GLint x, GLint y) {
	SIG ("glVertex2i");
	dllVertex2i (x, y);
}

static void APIENTRY logVertex2iv (const GLint *v) {
	SIG ("glVertex2iv");
	dllVertex2iv (v);
}

static void APIENTRY logVertex2s (GLshort x, GLshort y) {
	SIG ("glVertex2s");
	dllVertex2s (x, y);
}

static void APIENTRY logVertex2sv (const GLshort *v) {
	SIG ("glVertex2sv");
	dllVertex2sv (v);
}

static void APIENTRY logVertex3d (GLdouble x, GLdouble y, GLdouble z) {
	SIG ("glVertex3d");
	dllVertex3d (x, y, z);
}

static void APIENTRY logVertex3dv (const GLdouble *v) {
	SIG ("glVertex3dv");
	dllVertex3dv (v);
}

static void APIENTRY logVertex3f (GLfloat x, GLfloat y, GLfloat z) {
	SIG ("glVertex3f");
	dllVertex3f (x, y, z);
}

static void APIENTRY logVertex3fv (const GLfloat *v) {
	SIG ("glVertex3fv");
	dllVertex3fv (v);
}

static void APIENTRY logVertex3i (GLint x, GLint y, GLint z) {
	SIG ("glVertex3i");
	dllVertex3i (x, y, z);
}

static void APIENTRY logVertex3iv (const GLint *v) {
	SIG ("glVertex3iv");
	dllVertex3iv (v);
}

static void APIENTRY logVertex3s (GLshort x, GLshort y, GLshort z) {
	SIG ("glVertex3s");
	dllVertex3s (x, y, z);
}

static void APIENTRY logVertex3sv (const GLshort *v) {
	SIG ("glVertex3sv");
	dllVertex3sv (v);
}

static void APIENTRY logVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	SIG ("glVertex4d");
	dllVertex4d (x, y, z, w);
}

static void APIENTRY logVertex4dv (const GLdouble *v) {
	SIG ("glVertex4dv");
	dllVertex4dv (v);
}

static void APIENTRY logVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	SIG ("glVertex4f");
	dllVertex4f (x, y, z, w);
}

static void APIENTRY logVertex4fv (const GLfloat *v) {
	SIG ("glVertex4fv");
	dllVertex4fv (v);
}

static void APIENTRY logVertex4i (GLint x, GLint y, GLint z, GLint w) {
	SIG ("glVertex4i");
	dllVertex4i (x, y, z, w);
}

static void APIENTRY logVertex4iv (const GLint *v) {
	SIG ("glVertex4iv");
	dllVertex4iv (v);
}

static void APIENTRY logVertex4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	SIG ("glVertex4s");
	dllVertex4s (x, y, z, w);
}

static void APIENTRY logVertex4sv (const GLshort *v) {
	SIG ("glVertex4sv");
	dllVertex4sv (v);
}

static void APIENTRY logVertexPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	SIG ("glVertexPointer");
	dllVertexPointer (size, type, stride, pointer);
}

static void APIENTRY logViewport (GLint x, GLint y, GLsizei width, GLsizei height) {
	SIG ("glViewport");
	dllViewport (x, y, width, height);
}


//
// for debugging
//
static qBool	glBegan;

static void APIENTRY dbgAccum (GLenum op, GLfloat value) {
	dllAccum (op, value);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glAccum (%x %f)", op, value));
}
static void APIENTRY dbgAlphaFunc (GLenum func, GLclampf ref) {
	dllAlphaFunc (func, ref);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glAlphaFunc (0x%x, %f)", func, ref));
}
static GLboolean APIENTRY dbgAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences) {
	GLboolean	temp = dllAreTexturesResident (n, textures, residences);

	if (!glBegan) GL_CheckForError ("glAreTexturesResident");
	return temp;
}
static void APIENTRY dbgArrayElement (GLint i) {
	dllArrayElement (i);
	if (!glBegan) GL_CheckForError ("glArrayElement");
}
static void APIENTRY dbgBegin (GLenum mode) {
	dllBegin (mode);
	glBegan = qTrue;
}
static void APIENTRY dbgBindTexture (GLenum target, GLuint texture) {
	dllBindTexture (target, texture);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glBindTexture (0x%x, %u)", target, texture));
}
static void APIENTRY dbgBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
	dllBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
	if (!glBegan) GL_CheckForError ("glBitmap");
}
static void APIENTRY dbgBlendFunc (GLenum sfactor, GLenum dfactor) {
	dllBlendFunc (sfactor, dfactor);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glBlendFunc (0x%x, 0x%x)", sfactor, dfactor));
}
static void APIENTRY dbgCallList (GLuint list) {
	dllCallList (list);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glCallList (%u)", list));
}
static void APIENTRY dbgCallLists (GLsizei n, GLenum type, const void *lists) {
	dllCallLists (n, type, lists);
	if (!glBegan) GL_CheckForError ("glCallLists");
}
static void APIENTRY dbgClear (GLbitfield mask) {
	dllClear (mask);
	if (!glBegan) GL_CheckForError ("glClear");
}
static void APIENTRY dbgClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	dllClearAccum (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glClearAccum");
}
static void APIENTRY dbgClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	dllClearColor (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glClearColor");
}
static void APIENTRY dbgClearDepth (GLclampd depth) {
	dllClearDepth (depth);
	if (!glBegan) GL_CheckForError ("glClearDepth");
}
static void APIENTRY dbgClearIndex (GLfloat c) {
	dllClearIndex (c);
	if (!glBegan) GL_CheckForError ("glClearIndex");
}
static void APIENTRY dbgClearStencil (GLint s) {
	dllClearStencil (s);
	if (!glBegan) GL_CheckForError ("glClearStencil");
}
static void APIENTRY dbgClipPlane (GLenum plane, const GLdouble *equation) {
	dllClipPlane (plane, equation);
	if (!glBegan) GL_CheckForError ("glClipPlane");
}
static void APIENTRY dbgColor3b (GLbyte red, GLbyte green, GLbyte blue) {
	dllColor3b (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3b");
}
static void APIENTRY dbgColor3bv (const GLbyte *v) {
	dllColor3bv (v);
	if (!glBegan) GL_CheckForError ("glColor3bv");
}
static void APIENTRY dbgColor3d (GLdouble red, GLdouble green, GLdouble blue) {
	dllColor3d (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3d");
}
static void APIENTRY dbgColor3dv (const GLdouble *v) {
	dllColor3dv (v);
	if (!glBegan) GL_CheckForError ("glColor3dv");
}
static void APIENTRY dbgColor3f (GLfloat red, GLfloat green, GLfloat blue) {
	dllColor3f (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3f");
}
static void APIENTRY dbgColor3fv (const GLfloat *v) {
	dllColor3fv (v);
	if (!glBegan) GL_CheckForError ("glColor3fv");
}
static void APIENTRY dbgColor3i (GLint red, GLint green, GLint blue) {
	dllColor3i (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3i");
}
static void APIENTRY dbgColor3iv (const GLint *v) {
	dllColor3iv (v);
	if (!glBegan) GL_CheckForError ("glColor3iv");
}
static void APIENTRY dbgColor3s (GLshort red, GLshort green, GLshort blue) {
	dllColor3s (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3s");
}
static void APIENTRY dbgColor3sv (const GLshort *v) {
	dllColor3sv (v);
	if (!glBegan) GL_CheckForError ("glColor3sv");
}
static void APIENTRY dbgColor3ub (GLubyte red, GLubyte green, GLubyte blue) {
	dllColor3ub (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3ub");
}
static void APIENTRY dbgColor3ubv (const GLubyte *v) {
	dllColor3ubv (v);
	if (!glBegan) GL_CheckForError ("glColor3ubv");
}
static void APIENTRY dbgColor3ui (GLuint red, GLuint green, GLuint blue) {
	dllColor3ui (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3ui");
}
static void APIENTRY dbgColor3uiv (const GLuint *v) {
	dllColor3uiv (v);
	if (!glBegan) GL_CheckForError ("glColor3uiv");
}
static void APIENTRY dbgColor3us (GLushort red, GLushort green, GLushort blue) {
	dllColor3us (red, green, blue);
	if (!glBegan) GL_CheckForError ("glColor3us");
}
static void APIENTRY dbgColor3usv (const GLushort *v) {
	dllColor3usv (v);
	if (!glBegan) GL_CheckForError ("glColor3usv");
}
static void APIENTRY dbgColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
	dllColor4b (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4b");
}
static void APIENTRY dbgColor4bv (const GLbyte *v) {
	dllColor4bv (v);
	if (!glBegan) GL_CheckForError ("glColor4bv");
}
static void APIENTRY dbgColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
	dllColor4d (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4d");
}
static void APIENTRY dbgColor4dv (const GLdouble *v) {
	dllColor4dv (v);
	if (!glBegan) GL_CheckForError ("glColor4dv");
}
static void APIENTRY dbgColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	dllColor4f (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glColor4f (%f,%f,%f,%f)", red, green, blue, alpha));
}
static void APIENTRY dbgColor4fv (const GLfloat *v) {
	dllColor4fv (v);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glColor4fv (%f,%f,%f,%f)", v[0], v[1], v[2], v[3]));
}
static void APIENTRY dbgColor4i (GLint red, GLint green, GLint blue, GLint alpha) {
	dllColor4i (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4i");
}
static void APIENTRY dbgColor4iv (const GLint *v) {
	dllColor4iv (v);
	if (!glBegan) GL_CheckForError ("glColor4iv");
}
static void APIENTRY dbgColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha) {
	dllColor4s (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4s");
}
static void APIENTRY dbgColor4sv (const GLshort *v) {
	dllColor4sv (v);
	if (!glBegan) GL_CheckForError ("glColor4sv");
}
static void APIENTRY dbgColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	dllColor4b (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4b");
}
static void APIENTRY dbgColor4ubv (const GLubyte *v) {
	dllColor4ubv (v);
	if (!glBegan) GL_CheckForError ("glColor4ubv");
}
static void APIENTRY dbgColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha) {
	dllColor4ui (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4ui");
}
static void APIENTRY dbgColor4uiv (const GLuint *v) {
	dllColor4uiv (v);
	if (!glBegan) GL_CheckForError ("glColor4uiv");
}
static void APIENTRY dbgColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha) {
	dllColor4us (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColor4us");
}
static void APIENTRY dbgColor4usv (const GLushort *v) {
	dllColor4usv (v);
	if (!glBegan) GL_CheckForError ("glColor4usv");
}
static void APIENTRY dbgColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	dllColorMask (red, green, blue, alpha);
	if (!glBegan) GL_CheckForError ("glColorMask");
}
static void APIENTRY dbgColorMaterial (GLenum face, GLenum mode) {
	dllColorMaterial (face, mode);
	if (!glBegan) GL_CheckForError ("glColorMaterial");
}
static void APIENTRY dbgColorPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	dllColorPointer (size, type, stride, pointer);
	if (!glBegan) GL_CheckForError ("glColorPointer");
}
static void APIENTRY dbgCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
	dllCopyPixels (x, y, width, height, type);
	if (!glBegan) GL_CheckForError ("glCopyPixels");
}
static void APIENTRY dbgCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) {
	dllCopyTexImage1D (target, level, internalFormat, x, y, width, border);
	if (!glBegan) GL_CheckForError ("glCopyTexImage1D");
}
static void APIENTRY dbgCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	dllCopyTexImage2D (target, level, internalFormat, x, y, width, height, border);
	if (!glBegan) GL_CheckForError ("glCopyTexImage2D");
}
static void APIENTRY dbgCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
	dllCopyTexSubImage1D (target, level, xoffset, x, y, width);
	if (!glBegan) GL_CheckForError ("glCopyTexSubImage1D");
}
static void APIENTRY dbgCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	dllCopyTexSubImage2D (target, level, xoffset, yoffset, x, y, width, height);
	if (!glBegan) GL_CheckForError ("glCopyTexSubImage2D");
}
static void APIENTRY dbgCullFace (GLenum mode) {
	dllCullFace (mode);
	if (!glBegan) GL_CheckForError ("glCullFace");
}
static void APIENTRY dbgDeleteLists (GLuint list, GLsizei range) {
	dllDeleteLists (list, range);
	if (!glBegan) GL_CheckForError ("glDeleteLists");
}
static void APIENTRY dbgDeleteTextures (GLsizei n, const GLuint *textures) {
	dllDeleteTextures (n, textures);
	if (!glBegan) GL_CheckForError ("glDeleteTextures");
}
static void APIENTRY dbgDepthFunc (GLenum func) {
	dllDepthFunc (func);
	if (!glBegan) GL_CheckForError ("glDepthFunc");
}
static void APIENTRY dbgDepthMask (GLboolean flag) {
	dllDepthMask (flag);
	if (!glBegan) GL_CheckForError ("glDepthMask");
}
static void APIENTRY dbgDepthRange (GLclampd zNear, GLclampd zFar) {
	dllDepthRange (zNear, zFar);
	if (!glBegan) GL_CheckForError ("glDepthRange");
}
static void APIENTRY dbgDisable (GLenum cap) {
	dllDisable (cap);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glDisable (0x%x)", cap));
}
static void APIENTRY dbgDisableClientState (GLenum array) {
	dllDisableClientState (array);
	if (!glBegan) GL_CheckForError ("glDisableClientState");
}
static void APIENTRY dbgDrawArrays (GLenum mode, GLint first, GLsizei count) {
	dllDrawArrays (mode, first, count);
	if (!glBegan) GL_CheckForError ("glDrawArrays");
}
static void APIENTRY dbgDrawBuffer (GLenum mode) {
	dllDrawBuffer (mode);
	if (!glBegan) GL_CheckForError ("glDrawBuffer");
}
static void APIENTRY dbgDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {
	dllDrawElements (mode, count, type, indices);
	if (!glBegan) GL_CheckForError ("glDrawElements");
}
static void APIENTRY dbgDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	dllDrawPixels (width, height, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glDrawPixels");
}
static void APIENTRY dbgEdgeFlag (GLboolean flag) {
	dllEdgeFlag (flag);
	if (!glBegan) GL_CheckForError ("glEdgeFlag");
}
static void APIENTRY dbgEdgeFlagPointer (GLsizei stride, const void *pointer) {
	dllEdgeFlagPointer (stride, pointer);
	if (!glBegan) GL_CheckForError ("glEdgeFlagPointer");
}
static void APIENTRY dbgEdgeFlagv (const GLboolean *flag) {
	dllEdgeFlagv (flag);
	if (!glBegan) GL_CheckForError ("glEdgeFlagv");
}
static void APIENTRY dbgEnable (GLenum cap) {
	dllEnable (cap);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glEnable (0x%x)", cap));
}
static void APIENTRY dbgEnableClientState (GLenum array) {
	dllEnableClientState (array);
	if (!glBegan) GL_CheckForError ("glEnableClientState");
}
static void APIENTRY dbgEnd (void) {
	dllEnd ();
	glBegan = qFalse;
}
static void APIENTRY dbgEndList (void) {
	dllEndList ();
	if (!glBegan) GL_CheckForError ("glEndList");
}
static void APIENTRY dbgEvalCoord1d (GLdouble u) {
	dllEvalCoord1d (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord1d");
}
static void APIENTRY dbgEvalCoord1dv (const GLdouble *u) {
	dllEvalCoord1dv (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord1dv");
}
static void APIENTRY dbgEvalCoord1f (GLfloat u) {
	dllEvalCoord1f (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord1f");
}
static void APIENTRY dbgEvalCoord1fv (const GLfloat *u) {
	dllEvalCoord1fv (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord1fv");
}
static void APIENTRY dbgEvalCoord2d (GLdouble u, GLdouble v) {
	dllEvalCoord2d (u, v);
	if (!glBegan) GL_CheckForError ("glEvalCoord2d");
}
static void APIENTRY dbgEvalCoord2dv (const GLdouble *u) {
	dllEvalCoord2dv (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord2dv");
}
static void APIENTRY dbgEvalCoord2f (GLfloat u, GLfloat v) {
	dllEvalCoord2f (u, v);
	if (!glBegan) GL_CheckForError ("glEvalCoord2f");
}
static void APIENTRY dbgEvalCoord2fv (const GLfloat *u) {
	dllEvalCoord2fv (u);
	if (!glBegan) GL_CheckForError ("glEvalCoord2fv");
}
static void APIENTRY dbgEvalMesh1 (GLenum mode, GLint i1, GLint i2) {
	dllEvalMesh1 (mode, i1, i2);
	if (!glBegan) GL_CheckForError ("glEvalMesh1");
}
static void APIENTRY dbgEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
	dllEvalMesh2 (mode, i1, i2, j1, j2);
	if (!glBegan) GL_CheckForError ("glEvalMesh2");
}
static void APIENTRY dbgEvalPoint1 (GLint i) {
	dllEvalPoint1 (i);
	if (!glBegan) GL_CheckForError ("glEvalPoint1");
}
static void APIENTRY dbgEvalPoint2 (GLint i, GLint j) {
	dllEvalPoint2 (i, j);
	if (!glBegan) GL_CheckForError ("glEvalPoint2");
}
static void APIENTRY dbgFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer) {
	dllFeedbackBuffer (size, type, buffer);
	if (!glBegan) GL_CheckForError ("glFeedbackBuffer");
}
static void APIENTRY dbgFinish (void) {
	dllFinish ();
	if (!glBegan) GL_CheckForError ("glFinish");
}
static void APIENTRY dbgFlush (void) {
	dllFlush ();
	if (!glBegan) GL_CheckForError ("glFlush");
}
static void APIENTRY dbgFogf (GLenum pname, GLfloat param) {
	dllFogf (pname, param);
	if (!glBegan) GL_CheckForError ("glFogf");
}
static void APIENTRY dbgFogfv (GLenum pname, const GLfloat *params) {
	dllFogfv (pname, params);
	if (!glBegan) GL_CheckForError ("glFogfv");
}
static void APIENTRY dbgFogi (GLenum pname, GLint param) {
	dllFogi (pname, param);
	if (!glBegan) GL_CheckForError ("glFogi");
}
static void APIENTRY dbgFogiv (GLenum pname, const GLint *params) {
	dllFogiv (pname, params);
	if (!glBegan) GL_CheckForError ("glFogiv");
}
static void APIENTRY dbgFrontFace (GLenum mode) {
	dllFrontFace (mode);
	if (!glBegan) GL_CheckForError ("glFrontFace");
}
static void APIENTRY dbgFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	dllFrustum (left, right, bottom, top, zNear, zFar);
	if (!glBegan) GL_CheckForError ("glFrustum");
}
static GLuint APIENTRY dbgGenLists (GLsizei range) {
	GLuint	temp = dllGenLists (range);

	if (!glBegan) GL_CheckForError ("glGenLists");
	return temp;
}
static void APIENTRY dbgGenTextures (GLsizei n, GLuint *textures) {
	dllGenTextures (n, textures);
	if (!glBegan) GL_CheckForError ("glGenTextures");
}
static void APIENTRY dbgGetBooleanv (GLenum pname, GLboolean *params) {
	dllGetBooleanv (pname, params);
	if (!glBegan) GL_CheckForError ("glGetBooleanv");
}
static void APIENTRY dbgGetClipPlane (GLenum plane, GLdouble *equation) {
	dllGetClipPlane (plane, equation);
	if (!glBegan) GL_CheckForError ("glGetClipPlane");
}
static void APIENTRY dbgGetDoublev (GLenum pname, GLdouble *params) {
	dllGetDoublev (pname, params);
	if (!glBegan) GL_CheckForError ("glGetDoublev");
}
static GLenum APIENTRY dbgGetError (void) {
	return dllGetError ();
}
static void APIENTRY dbgGetFloatv (GLenum pname, GLfloat *params) {
	dllGetFloatv (pname, params);
	if (!glBegan) GL_CheckForError ("glGetFloatv");
}
static void APIENTRY dbgGetIntegerv (GLenum pname, GLint *params) {
	dllGetIntegerv (pname, params);
	if (!glBegan) GL_CheckForError ("glGetIntegerv");
}
static void APIENTRY dbgGetLightfv (GLenum light, GLenum pname, GLfloat *params) {
	dllGetLightfv (light, pname, params);
	if (!glBegan) GL_CheckForError ("glGetLightfv");
}
static void APIENTRY dbgGetLightiv (GLenum light, GLenum pname, GLint *params) {
	dllGetLightiv (light, pname, params);
	if (!glBegan) GL_CheckForError ("glGetLightiv");
}
static void APIENTRY dbgGetMapdv (GLenum target, GLenum query, GLdouble *v) {
	dllGetMapdv (target, query, v);
	if (!glBegan) GL_CheckForError ("glGetMapdv");
}
static void APIENTRY dbgGetMapfv (GLenum target, GLenum query, GLfloat *v) {
	dllGetMapfv (target, query, v);
	if (!glBegan) GL_CheckForError ("glGetMapfv");
}
static void APIENTRY dbgGetMapiv (GLenum target, GLenum query, GLint *v) {
	dllGetMapiv (target, query, v);
	if (!glBegan) GL_CheckForError ("glGetMapiv");
}
static void APIENTRY dbgGetMaterialfv (GLenum face, GLenum pname, GLfloat *params) {
	dllGetMaterialfv (face, pname, params);
	if (!glBegan) GL_CheckForError ("glGetMaterialfv");
}
static void APIENTRY dbgGetMaterialiv (GLenum face, GLenum pname, GLint *params) {
	dllGetMaterialiv (face, pname, params);
	if (!glBegan) GL_CheckForError ("glGetMaterialiv");
}
static void APIENTRY dbgGetPixelMapfv (GLenum map, GLfloat *values) {
	dllGetPixelMapfv (map, values);
	if (!glBegan) GL_CheckForError ("glGetPixelMapfv");
}
static void APIENTRY dbgGetPixelMapuiv (GLenum map, GLuint *values) {
	dllGetPixelMapuiv (map, values);
	if (!glBegan) GL_CheckForError ("glGetPixelMapuiv");
}
static void APIENTRY dbgGetPixelMapusv (GLenum map, GLushort *values) {
	dllGetPixelMapusv (map, values);
	if (!glBegan) GL_CheckForError ("glGetPixelMapusv");
}
static void APIENTRY dbgGetPointerv (GLenum pname, GLvoid* *params) {
	dllGetPointerv (pname, params);
	if (!glBegan) GL_CheckForError ("glGetPointerv");
}
static void APIENTRY dbgGetPolygonStipple (GLubyte *mask) {
	dllGetPolygonStipple (mask);
	if (!glBegan) GL_CheckForError ("glGetPolygonStipple");
}
static const GLubyte * APIENTRY dbgGetString (GLenum name) {
	const GLubyte	*temp = dllGetString (name);

	if (!glBegan) GL_CheckForError ("glGetString");
	return temp;
}
static void APIENTRY dbgGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params){
	dllGetTexEnvfv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexEnvfv");
}
static void APIENTRY dbgGetTexEnviv (GLenum target, GLenum pname, GLint *params) {
	dllGetTexEnviv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexEnviv");
}
static void APIENTRY dbgGetTexGendv (GLenum coord, GLenum pname, GLdouble *params) {
	dllGetTexGendv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexGendv");
}
static void APIENTRY dbgGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params) {
	dllGetTexGenfv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexGenfv");
}
static void APIENTRY dbgGetTexGeniv (GLenum coord, GLenum pname, GLint *params) {
	dllGetTexGeniv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexGeniv");
}
static void APIENTRY dbgGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, void *pixels) {
	dllGetTexImage (target, level, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glGetTexImage");
}
static void APIENTRY dbgGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params) {
	dllGetTexLevelParameterfv (target, level, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexLevelParameterfv");
}
static void APIENTRY dbgGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params) {
	dllGetTexLevelParameteriv (target, level, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexLevelParameteriv");
}
static void APIENTRY dbgGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params) {
	dllGetTexParameterfv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexParameterfv");
}
static void APIENTRY dbgGetTexParameteriv (GLenum target, GLenum pname, GLint *params) {
	dllGetTexParameteriv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glGetTexParameteriv");
}
static void APIENTRY dbgHint (GLenum target, GLenum mode) {
	dllHint (target, mode);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glHint (0x%x, 0x%x)", target, mode));
}
static void APIENTRY dbgIndexMask (GLuint mask) {
	dllIndexMask (mask);
	if (!glBegan) GL_CheckForError ("glIndexMask");
}
static void APIENTRY dbgIndexPointer (GLenum type, GLsizei stride, const void *pointer) {
	dllIndexPointer (type, stride, pointer);
	if (!glBegan) GL_CheckForError ("glIndexPointer");
}
static void APIENTRY dbgIndexd (GLdouble c) {
	dllIndexd (c);
	if (!glBegan) GL_CheckForError ("glIndexd");
}
static void APIENTRY dbgIndexdv (const GLdouble *c) {
	dllIndexdv (c);
	if (!glBegan) GL_CheckForError ("glIndexdv");
}
static void APIENTRY dbgIndexf (GLfloat c) {
	dllIndexf (c);
	if (!glBegan) GL_CheckForError ("glIndexf");
}
static void APIENTRY dbgIndexfv (const GLfloat *c) {
	dllIndexfv (c);
	if (!glBegan) GL_CheckForError ("glIndexfv");
}
static void APIENTRY dbgIndexi (GLint c) {
	dllIndexi (c);
	if (!glBegan) GL_CheckForError ("glIndexi");
}
static void APIENTRY dbgIndexiv (const GLint *c) {
	dllIndexiv (c);
	if (!glBegan) GL_CheckForError ("glIndexiv");
}
static void APIENTRY dbgIndexs (GLshort c) {
	dllIndexs (c);
	if (!glBegan) GL_CheckForError ("glIndexs");
}
static void APIENTRY dbgIndexsv (const GLshort *c) {
	dllIndexsv (c);
	if (!glBegan) GL_CheckForError ("glIndexsv");
}
static void APIENTRY dbgIndexub (GLubyte c) {
	dllIndexub (c);
	if (!glBegan) GL_CheckForError ("glIndexub");
}
static void APIENTRY dbgIndexubv (const GLubyte *c) {
	dllIndexubv (c);
	if (!glBegan) GL_CheckForError ("glIndexubv");
}
static void APIENTRY dbgInitNames (void) {
	dllInitNames ();
	if (!glBegan) GL_CheckForError ("glInitNames");
}
static void APIENTRY dbgInterleavedArrays (GLenum format, GLsizei stride, const void *pointer) {
	dllInterleavedArrays (format, stride, pointer);
	if (!glBegan) GL_CheckForError ("glInterleavedArrays");
}
static GLboolean APIENTRY dbgIsEnabled (GLenum cap) {
	GLboolean	temp = dllIsEnabled (cap);

	if (!glBegan) GL_CheckForError ("glIsEnabled");
	return temp;
}
static GLboolean APIENTRY dbgIsList (GLuint list) {
	GLboolean	temp = dllIsList (list);

	if (!glBegan) GL_CheckForError ("glIsList");
	return temp;
}
static GLboolean APIENTRY dbgIsTexture (GLuint texture) {
	GLboolean	temp = dllIsTexture (texture);

	if (!glBegan) GL_CheckForError ("glIsTexture");
	return temp;
}
static void APIENTRY dbgLightModelf (GLenum pname, GLfloat param) {
	dllLightModelf (pname, param);
	if (!glBegan) GL_CheckForError ("glLightModelf");
}
static void APIENTRY dbgLightModelfv (GLenum pname, const GLfloat *params) {
	dllLightModelfv (pname, params);
	if (!glBegan) GL_CheckForError ("glLightModelfv");
}
static void APIENTRY dbgLightModeli (GLenum pname, GLint param) {
	dllLightModeli (pname, param);
	if (!glBegan) GL_CheckForError ("glLightModeli");
}
static void APIENTRY dbgLightModeliv (GLenum pname, const GLint *params) {
	dllLightModeliv (pname, params);
	if (!glBegan) GL_CheckForError ("glLightModeliv");
}
static void APIENTRY dbgLightf (GLenum light, GLenum pname, GLfloat param) {
	dllLightf (light, pname, param);
	if (!glBegan) GL_CheckForError ("glLightf");
}
static void APIENTRY dbgLightfv (GLenum light, GLenum pname, const GLfloat *params) {
	dllLightfv (light, pname, params);
	if (!glBegan) GL_CheckForError ("glLightfv");
}
static void APIENTRY dbgLighti (GLenum light, GLenum pname, GLint param) {
	dllLighti (light, pname, param);
	if (!glBegan) GL_CheckForError ("glLighti");
}
static void APIENTRY dbgLightiv (GLenum light, GLenum pname, const GLint *params) {
	dllLightiv (light, pname, params);
	if (!glBegan) GL_CheckForError ("glLightiv");
}
static void APIENTRY dbgLineStipple (GLint factor, GLushort pattern) {
	dllLineStipple (factor, pattern);
	if (!glBegan) GL_CheckForError ("glLineStipple");
}
static void APIENTRY dbgLineWidth (GLfloat width) {
	dllLineWidth (width);
	if (!glBegan) GL_CheckForError ("glLineWidth");
}
static void APIENTRY dbgListBase (GLuint base) {
	dllListBase (base);
	if (!glBegan) GL_CheckForError ("glListBase");
}
static void APIENTRY dbgLoadIdentity (void) {
	dllLoadIdentity ();
	if (!glBegan) GL_CheckForError ("glLoadIdentity");
}
static void APIENTRY dbgLoadMatrixd (const GLdouble *m) {
	dllLoadMatrixd (m);
	if (!glBegan) GL_CheckForError ("glLoadMatrixd");
}

static void APIENTRY dbgLoadMatrixf (const GLfloat *m){
	dllLoadMatrixf (m);
	if (!glBegan) GL_CheckForError ("glLoadMatrixf");
}
static void APIENTRY dbgLoadName (GLuint name) {
	dllLoadName (name);
	if (!glBegan) GL_CheckForError ("glLoadName");
}
static void APIENTRY dbgLogicOp (GLenum opcode) {
	dllLogicOp (opcode);
	if (!glBegan) GL_CheckForError ("glLogicOp");
}
static void APIENTRY dbgMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
	dllMap1d (target, u1, u2, stride, order, points);
	if (!glBegan) GL_CheckForError ("glMap1d");
}
static void APIENTRY dbgMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
	dllMap1f (target, u1, u2, stride, order, points);
	if (!glBegan) GL_CheckForError ("glMap1f");
}
static void APIENTRY dbgMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
	dllMap2d (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
	if (!glBegan) GL_CheckForError ("glMap2d");
}
static void APIENTRY dbgMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
	dllMap2f (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
	if (!glBegan) GL_CheckForError ("glMap2f");
}
static void APIENTRY dbgMapGrid1d (GLint un, GLdouble u1, GLdouble u2) {
	dllMapGrid1d (un, u1, u2);
	if (!glBegan) GL_CheckForError ("glMapGrid1d");
}
static void APIENTRY dbgMapGrid1f (GLint un, GLfloat u1, GLfloat u2) {
	dllMapGrid1f (un, u1, u2);
	if (!glBegan) GL_CheckForError ("glMapGrid1f");
}
static void APIENTRY dbgMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
	dllMapGrid2d (un, u1, u2, vn, v1, v2);
	if (!glBegan) GL_CheckForError ("glMapGrid2d");
}
static void APIENTRY dbgMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
	dllMapGrid2f (un, u1, u2, vn, v1, v2);
	if (!glBegan) GL_CheckForError ("glMapGrid2f");
}
static void APIENTRY dbgMaterialf (GLenum face, GLenum pname, GLfloat param) {
	dllMaterialf (face, pname, param);
	if (!glBegan) GL_CheckForError ("glMaterialf");
}
static void APIENTRY dbgMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {
	dllMaterialfv (face, pname, params);
	if (!glBegan) GL_CheckForError ("glMaterialfv");
}
static void APIENTRY dbgMateriali (GLenum face, GLenum pname, GLint param) {
	dllMateriali (face, pname, param);
	if (!glBegan) GL_CheckForError ("glMateriali");
}
static void APIENTRY dbgMaterialiv (GLenum face, GLenum pname, const GLint *params) {
	dllMaterialiv (face, pname, params);
	if (!glBegan) GL_CheckForError ("glMaterialiv");
}
static void APIENTRY dbgMatrixMode (GLenum mode) {
	dllMatrixMode (mode);
	if (!glBegan) GL_CheckForError ("glMatrixMode");
}
static void APIENTRY dbgMultMatrixd (const GLdouble *m) {
	dllMultMatrixd (m);
	if (!glBegan) GL_CheckForError ("glMultMatrixd");
}
static void APIENTRY dbgMultMatrixf (const GLfloat *m) {
	dllMultMatrixf (m);
	if (!glBegan) GL_CheckForError ("glMultMatrixf");
}
static void APIENTRY dbgNewList (GLuint list, GLenum mode) {
	dllNewList (list, mode);
	if (!glBegan) GL_CheckForError ("glNewList");
}
static void APIENTRY dbgNormal3b (GLbyte nx, GLbyte ny, GLbyte nz) {
	dllNormal3b (nx, ny, nz);
	if (!glBegan) GL_CheckForError ("glNormal3b");
}
static void APIENTRY dbgNormal3bv (const GLbyte *v) {
	dllNormal3bv (v);
	if (!glBegan) GL_CheckForError ("glNormal3bv");
}
static void APIENTRY dbgNormal3d (GLdouble nx, GLdouble ny, GLdouble nz) {
	dllNormal3d (nx, ny, nz);
	if (!glBegan) GL_CheckForError ("glNormal3d");
}
static void APIENTRY dbgNormal3dv (const GLdouble *v) {
	dllNormal3dv (v);
	if (!glBegan) GL_CheckForError ("glNormal3dv");
}
static void APIENTRY dbgNormal3f (GLfloat nx, GLfloat ny, GLfloat nz) {
	dllNormal3f (nx, ny, nz);
	if (!glBegan) GL_CheckForError ("glNormal3f");
}
static void APIENTRY dbgNormal3fv (const GLfloat *v) {
	dllNormal3fv (v);
	if (!glBegan) GL_CheckForError ("glNormal3fv");
}
static void APIENTRY dbgNormal3i (GLint nx, GLint ny, GLint nz) {
	dllNormal3i (nx, ny, nz);
	if (!glBegan) GL_CheckForError ("glNormal3i");
}
static void APIENTRY dbgNormal3iv (const GLint *v) {
	dllNormal3iv (v);
	if (!glBegan) GL_CheckForError ("glNormal3iv");
}
static void APIENTRY dbgNormal3s (GLshort nx, GLshort ny, GLshort nz) {
	dllNormal3s (nx, ny, nz);
	if (!glBegan) GL_CheckForError ("glNormal3s");
}
static void APIENTRY dbgNormal3sv (const GLshort *v) {
	dllNormal3sv (v);
	if (!glBegan) GL_CheckForError ("glNormal3sv");
}
static void APIENTRY dbgNormalPointer (GLenum type, GLsizei stride, const void *pointer) {
	dllNormalPointer (type, stride, pointer);
	if (!glBegan) GL_CheckForError ("glNormalPointer");
}
static void APIENTRY dbgOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	dllOrtho (left, right, bottom, top, zNear, zFar);
	if (!glBegan) GL_CheckForError ("glOrtho");
}
static void APIENTRY dbgPassThrough (GLfloat token) {
	dllPassThrough (token);
	if (!glBegan) GL_CheckForError ("glPassThrough");
}
static void APIENTRY dbgPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values) {
	dllPixelMapfv (map, mapsize, values);
	if (!glBegan) GL_CheckForError ("glPixelMapfv");
}
static void APIENTRY dbgPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values) {
	dllPixelMapuiv (map, mapsize, values);
	if (!glBegan) GL_CheckForError ("glPixelMapuiv");
}
static void APIENTRY dbgPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values) {
	dllPixelMapusv (map, mapsize, values);
	if (!glBegan) GL_CheckForError ("glPixelMapusv");
}
static void APIENTRY dbgPixelStoref (GLenum pname, GLfloat param) {
	dllPixelStoref (pname, param);
	if (!glBegan) GL_CheckForError ("glPixelStoref");
}
static void APIENTRY dbgPixelStorei (GLenum pname, GLint param) {
	dllPixelStorei (pname, param);
	if (!glBegan) GL_CheckForError ("glPixelStorei");
}
static void APIENTRY dbgPixelTransferf (GLenum pname, GLfloat param) {
	dllPixelTransferf (pname, param);
	if (!glBegan) GL_CheckForError ("glPixelTransferf");
}
static void APIENTRY dbgPixelTransferi (GLenum pname, GLint param) {
	dllPixelTransferi (pname, param);
	if (!glBegan) GL_CheckForError ("glPixelTransferi");
}
static void APIENTRY dbgPixelZoom (GLfloat xfactor, GLfloat yfactor) {
	dllPixelZoom (xfactor, yfactor);
	if (!glBegan) GL_CheckForError ("glPixelZoom");
}
static void APIENTRY dbgPointSize (GLfloat size) {
	dllPointSize (size);
	if (!glBegan) GL_CheckForError ("glPointSize");
}
static void APIENTRY dbgPolygonMode (GLenum face, GLenum mode) {
	dllPolygonMode (face, mode);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glPolygonMode (0x%x, 0x%x)", face, mode));
}
static void APIENTRY dbgPolygonOffset (GLfloat factor, GLfloat units) {
	dllPolygonOffset (factor, units);
	if (!glBegan) GL_CheckForError ("glPolygonOffset");
}
static void APIENTRY dbgPolygonStipple (const GLubyte *mask) {
	dllPolygonStipple (mask);
	if (!glBegan) GL_CheckForError ("glPolygonStipple");
}
static void APIENTRY dbgPopAttrib (void) {
	dllPopAttrib ();
	if (!glBegan) GL_CheckForError ("glPopAttrib");
}
static void APIENTRY dbgPopClientAttrib (void) {
	dllPopClientAttrib ();
	if (!glBegan) GL_CheckForError ("glPopClientAttrib");
}
static void APIENTRY dbgPopMatrix (void) {
	dllPopMatrix ();
	if (!glBegan) GL_CheckForError ("glPopMatrix");
}
static void APIENTRY dbgPopName (void) {
	dllPopName ();
	if (!glBegan) GL_CheckForError ("glPopName");
}
static void APIENTRY dbgPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities) {
	dllPrioritizeTextures (n, textures, priorities);
	if (!glBegan) GL_CheckForError ("glPrioritizeTextures");
}
static void APIENTRY dbgPushAttrib (GLbitfield mask) {
	dllPushAttrib (mask);
	if (!glBegan) GL_CheckForError ("glPushAttrib");
}
static void APIENTRY dbgPushClientAttrib (GLbitfield mask) {
	dllPushClientAttrib (mask);
	if (!glBegan) GL_CheckForError ("glPushClientAttrib");
}
static void APIENTRY dbgPushMatrix (void) {
	dllPushMatrix ();
	if (!glBegan) GL_CheckForError ("glPushMatrix");
}
static void APIENTRY dbgPushName (GLuint name) {
	dllPushName (name);
	if (!glBegan) GL_CheckForError ("glPushName");
}
static void APIENTRY dbgRasterPos2d (GLdouble x, GLdouble y) {
	dllRasterPos2d (x, y);
	if (!glBegan) GL_CheckForError ("glRasterPot2d");
}
static void APIENTRY dbgRasterPos2dv (const GLdouble *v) {
	dllRasterPos2dv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos2dv");
}
static void APIENTRY dbgRasterPos2f (GLfloat x, GLfloat y) {
	dllRasterPos2f (x, y);
	if (!glBegan) GL_CheckForError ("glRasterPos2f");
}
static void APIENTRY dbgRasterPos2fv (const GLfloat *v) {
	dllRasterPos2fv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos2dv");
}
static void APIENTRY dbgRasterPos2i (GLint x, GLint y) {
	dllRasterPos2i (x, y);
	if (!glBegan) GL_CheckForError ("glRasterPos2if");
}
static void APIENTRY dbgRasterPos2iv (const GLint *v) {
	dllRasterPos2iv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos2iv");
}
static void APIENTRY dbgRasterPos2s (GLshort x, GLshort y) {
	dllRasterPos2s (x, y);
	if (!glBegan) GL_CheckForError ("glRasterPos2s");
}
static void APIENTRY dbgRasterPos2sv (const GLshort *v) {
	dllRasterPos2sv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos2sv");
}
static void APIENTRY dbgRasterPos3d (GLdouble x, GLdouble y, GLdouble z) {
	dllRasterPos3d (x, y, z);
	if (!glBegan) GL_CheckForError ("glRasterPos3d");
}
static void APIENTRY dbgRasterPos3dv (const GLdouble *v) {
	dllRasterPos3dv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos3dv");
}
static void APIENTRY dbgRasterPos3f (GLfloat x, GLfloat y, GLfloat z) {
	dllRasterPos3f (x, y, z);
	if (!glBegan) GL_CheckForError ("glRasterPos3f");
}
static void APIENTRY dbgRasterPos3fv (const GLfloat *v) {
	dllRasterPos3fv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos3fv");
}
static void APIENTRY dbgRasterPos3i (GLint x, GLint y, GLint z) {
	dllRasterPos3i (x, y, z);
	if (!glBegan) GL_CheckForError ("glRasterPos3i");
}
static void APIENTRY dbgRasterPos3iv (const GLint *v) {
	dllRasterPos3iv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos3iv");
}
static void APIENTRY dbgRasterPos3s (GLshort x, GLshort y, GLshort z) {
	dllRasterPos3s (x, y, z);
	if (!glBegan) GL_CheckForError ("glRasterPos3s");
}
static void APIENTRY dbgRasterPos3sv (const GLshort *v) {
	dllRasterPos3sv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos3sv");
}
static void APIENTRY dbgRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	dllRasterPos4d (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glRasterPos4d");
}
static void APIENTRY dbgRasterPos4dv (const GLdouble *v) {
	dllRasterPos4dv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos4dv");
}
static void APIENTRY dbgRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	dllRasterPos4f (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glRasterPos4f");
}
static void APIENTRY dbgRasterPos4fv (const GLfloat *v) {
	dllRasterPos4fv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos4fv");
}
static void APIENTRY dbgRasterPos4i (GLint x, GLint y, GLint z, GLint w) {
	dllRasterPos4i (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glRasterPos4i");
}
static void APIENTRY dbgRasterPos4iv (const GLint *v) {
	dllRasterPos4iv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos4iv");
}
static void APIENTRY dbgRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	dllRasterPos4s (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glRasterPos4s");
}
static void APIENTRY dbgRasterPos4sv (const GLshort *v) {
	dllRasterPos4sv (v);
	if (!glBegan) GL_CheckForError ("glRasterPos4sv");
}
static void APIENTRY dbgReadBuffer (GLenum mode) {
	dllReadBuffer (mode);
	if (!glBegan) GL_CheckForError ("glReadBuffer");
}
static void APIENTRY dbgReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	dllReadPixels (x, y, width, height, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glReadPixels");
}
static void APIENTRY dbgRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
	dllRectd (x1, y1, x2, y2);
	if (!glBegan) GL_CheckForError ("glRectd");
}
static void APIENTRY dbgRectdv (const GLdouble *v1, const GLdouble *v2) {
	dllRectdv (v1, v2);
	if (!glBegan) GL_CheckForError ("glRectdv");
}
static void APIENTRY dbgRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	dllRectf (x1, y1, x2, y2);
	if (!glBegan) GL_CheckForError ("glRectf");
}
static void APIENTRY dbgRectfv (const GLfloat *v1, const GLfloat *v2) {
	dllRectfv (v1, v2);
	if (!glBegan) GL_CheckForError ("glRectfv");
}
static void APIENTRY dbgRecti (GLint x1, GLint y1, GLint x2, GLint y2) {
	dllRecti (x1, y1, x2, y2);
	if (!glBegan) GL_CheckForError ("glRecti");
}
static void APIENTRY dbgRectiv (const GLint *v1, const GLint *v2) {
	dllRectiv (v1, v2);
	if (!glBegan) GL_CheckForError ("glRectiv");
}
static void APIENTRY dbgRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
	dllRects (x1, y1, x2, y2);
	if (!glBegan) GL_CheckForError ("glRects");
}
static void APIENTRY dbgRectsv (const GLshort *v1, const GLshort *v2) {
	dllRectsv (v1, v2);
	if (!glBegan) GL_CheckForError ("glRectsv");
}
static GLint APIENTRY dbgRenderMode (GLenum mode) {
	GLint	temp = dllRenderMode (mode);

	if (!glBegan) GL_CheckForError ("glRenderMode");
	return temp;
}
static void APIENTRY dbgRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
	dllRotated (angle, x, y, z);
	if (!glBegan) GL_CheckForError ("glRotated");
}
static void APIENTRY dbgRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	dllRotatef (angle, x, y, z);
	if (!glBegan) GL_CheckForError ("glRotatef");
}
static void APIENTRY dbgScaled (GLdouble x, GLdouble y, GLdouble z) {
	dllScaled (x, y, z);
	if (!glBegan) GL_CheckForError ("glScaled");
}
static void APIENTRY dbgScalef (GLfloat x, GLfloat y, GLfloat z) {
	dllScalef (x, y, z);
	if (!glBegan) GL_CheckForError ("glScalef");
}
static void APIENTRY dbgScissor (GLint x, GLint y, GLsizei width, GLsizei height) {
	dllScissor (x, y, width, height);
	if (!glBegan) GL_CheckForError ("glScissor");
}
static void APIENTRY dbgSelectBuffer (GLsizei size, GLuint *buffer) {
	dllSelectBuffer (size, buffer);
	if (!glBegan) GL_CheckForError ("glSelectBuffer");
}
static void APIENTRY dbgShadeModel (GLenum mode) {
	dllShadeModel (mode);
	if (!glBegan) GL_CheckForError ("glShadeModel");
}
static void APIENTRY dbgStencilFunc (GLenum func, GLint ref, GLuint mask) {
	dllStencilFunc (func, ref, mask);
	if (!glBegan) GL_CheckForError ("glStencilFunc");
}
static void APIENTRY dbgStencilMask (GLuint mask) {
	dllStencilMask (mask);
	if (!glBegan) GL_CheckForError ("glStencilMask");
}
static void APIENTRY dbgStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
	dllStencilOp (fail, zfail, zpass);
	if (!glBegan) GL_CheckForError ("glStencilOp");
}
static void APIENTRY dbgTexCoord1d (GLdouble s) {
	dllTexCoord1d (s);
	if (!glBegan) GL_CheckForError ("glTexCoord1d");
}
static void APIENTRY dbgTexCoord1dv (const GLdouble *v) {
	dllTexCoord1dv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord1dv");
}
static void APIENTRY dbgTexCoord1f (GLfloat s) {
	dllTexCoord1f (s);
	if (!glBegan) GL_CheckForError ("glTexCoord1f");
}
static void APIENTRY dbgTexCoord1fv (const GLfloat *v) {
	dllTexCoord1fv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord1fv");
}
static void APIENTRY dbgTexCoord1i (GLint s) {
	dllTexCoord1i (s);
	if (!glBegan) GL_CheckForError ("glTexCoord1i");
}
static void APIENTRY dbgTexCoord1iv (const GLint *v) {
	dllTexCoord1iv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord1iv");
}
static void APIENTRY dbgTexCoord1s (GLshort s) {
	dllTexCoord1s (s);
	if (!glBegan) GL_CheckForError ("glTexCoord1s");
}
static void APIENTRY dbgTexCoord1sv (const GLshort *v) {
	dllTexCoord1sv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord1sv");
}
static void APIENTRY dbgTexCoord2d (GLdouble s, GLdouble t) {
	dllTexCoord2d (s, t);
	if (!glBegan) GL_CheckForError ("glTexCoord2d");
}
static void APIENTRY dbgTexCoord2dv (const GLdouble *v) {
	dllTexCoord2dv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord2dv");
}
static void APIENTRY dbgTexCoord2f (GLfloat s, GLfloat t) {
	dllTexCoord2f (s, t);
	if (!glBegan) GL_CheckForError ("glTexCoord2f");
}
static void APIENTRY dbgTexCoord2fv (const GLfloat *v) {
	dllTexCoord2fv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord2fv");
}
static void APIENTRY dbgTexCoord2i (GLint s, GLint t) {
	dllTexCoord2i (s, t);
	if (!glBegan) GL_CheckForError ("glTexCoord2i");
}
static void APIENTRY dbgTexCoord2iv (const GLint *v) {
	dllTexCoord2iv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord2iv");
}
static void APIENTRY dbgTexCoord2s (GLshort s, GLshort t) {
	dllTexCoord2s (s, t);
	if (!glBegan) GL_CheckForError ("glTexCoord2s");
}
static void APIENTRY dbgTexCoord2sv (const GLshort *v) {
	dllTexCoord2sv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord2sv");
}
static void APIENTRY dbgTexCoord3d (GLdouble s, GLdouble t, GLdouble r) {
	dllTexCoord3d (s, t, r);
	if (!glBegan) GL_CheckForError ("glTexCoord3d");
}
static void APIENTRY dbgTexCoord3dv (const GLdouble *v) {
	dllTexCoord3dv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord3dv");
}
static void APIENTRY dbgTexCoord3f (GLfloat s, GLfloat t, GLfloat r) {
	dllTexCoord3f (s, t, r);
	if (!glBegan) GL_CheckForError ("glTexCoord3f");
}
static void APIENTRY dbgTexCoord3fv (const GLfloat *v) {
	dllTexCoord3fv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord3fv");
}
static void APIENTRY dbgTexCoord3i (GLint s, GLint t, GLint r) {
	dllTexCoord3i (s, t, r);
	if (!glBegan) GL_CheckForError ("glTexCoord3i");
}
static void APIENTRY dbgTexCoord3iv (const GLint *v) {
	dllTexCoord3iv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord3iv");
}
static void APIENTRY dbgTexCoord3s (GLshort s, GLshort t, GLshort r) {
	dllTexCoord3s (s, t, r);
	if (!glBegan) GL_CheckForError ("glTexCoord3s");
}
static void APIENTRY dbgTexCoord3sv (const GLshort *v) {
	dllTexCoord3sv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord3sv");
}
static void APIENTRY dbgTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
	dllTexCoord4d (s, t, r, q);
	if (!glBegan) GL_CheckForError ("glTexCoord4d");
}
static void APIENTRY dbgTexCoord4dv (const GLdouble *v) {
	dllTexCoord4dv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord4dv");
}
static void APIENTRY dbgTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
	dllTexCoord4f (s, t, r, q);
	if (!glBegan) GL_CheckForError ("glTexCoord4f");
}
static void APIENTRY dbgTexCoord4fv (const GLfloat *v) {
	dllTexCoord4fv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord4fv");
}
static void APIENTRY dbgTexCoord4i (GLint s, GLint t, GLint r, GLint q) {
	dllTexCoord4i (s, t, r, q);
	if (!glBegan) GL_CheckForError ("glTexCoord4i");
}
static void APIENTRY dbgTexCoord4iv (const GLint *v) {
	dllTexCoord4iv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord4iv");
}
static void APIENTRY dbgTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q) {
	dllTexCoord4s (s, t, r, q);
	if (!glBegan) GL_CheckForError ("glTexCoord4s");
}
static void APIENTRY dbgTexCoord4sv (const GLshort *v) {
	dllTexCoord4sv (v);
	if (!glBegan) GL_CheckForError ("glTexCoord4sv");
}
static void APIENTRY dbgTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	dllTexCoordPointer (size, type, stride, pointer);
	if (!glBegan) GL_CheckForError ("glTexCoordPointer");
}
static void APIENTRY dbgTexEnvf (GLenum target, GLenum pname, GLfloat param) {
	dllTexEnvf (target, pname, param);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glTexEnvf (0x%x, 0x%x, %f)", target, pname, param));
}
static void APIENTRY dbgTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) {
	dllTexEnvfv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glTexEnvfv");
}
static void APIENTRY dbgTexEnvi (GLenum target, GLenum pname, GLint param) {
	dllTexEnvi (target, pname, param);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glTexEnvi (0x%x, 0x%x, 0x%x)", target, pname, param));
}
static void APIENTRY dbgTexEnviv (GLenum target, GLenum pname, const GLint *params) {
	dllTexEnviv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glTexEnviv");
}
static void APIENTRY dbgTexGend (GLenum coord, GLenum pname, GLdouble param) {
	dllTexGend (coord, pname, param);
	if (!glBegan) GL_CheckForError ("glTexGend");
}
static void APIENTRY dbgTexGendv (GLenum coord, GLenum pname, const GLdouble *params) {
	dllTexGendv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glTexGendv");
}
static void APIENTRY dbgTexGenf (GLenum coord, GLenum pname, GLfloat param) {
	dllTexGenf (coord, pname, param);
	if (!glBegan) GL_CheckForError ("glTexGenf");
}
static void APIENTRY dbgTexGenfv (GLenum coord, GLenum pname, const GLfloat *params) {
	dllTexGenfv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glTexGenfv");
}
static void APIENTRY dbgTexGeni (GLenum coord, GLenum pname, GLint param) {
	dllTexGeni (coord, pname, param);
	if (!glBegan) GL_CheckForError ("glTexGeni");
}
static void APIENTRY dbgTexGeniv (GLenum coord, GLenum pname, const GLint *params) {
	dllTexGeniv (coord, pname, params);
	if (!glBegan) GL_CheckForError ("glTexGeniv");
}
static void APIENTRY dbgTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) {
	dllTexImage1D (target, level, internalformat, width, border, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glTexImage1D");
}
static void APIENTRY dbgTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	dllTexImage2D (target, level, internalformat, width, height, border, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glTexImage2D");
}
static void APIENTRY dbgTexParameterf (GLenum target, GLenum pname, GLfloat param) {
	dllTexParameterf (target, pname, param);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glTexParameterf (0x%x, 0x%x, %f)", target, pname, param));
}
static void APIENTRY dbgTexParameterfv (GLenum target, GLenum pname, const GLfloat *params) {
	dllTexParameterfv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glTexParameterfv");
}
static void APIENTRY dbgTexParameteri (GLenum target, GLenum pname, GLint param) {
	dllTexParameteri (target, pname, param);
	if (!glBegan) GL_CheckForError (Q_VarArgs ("glTexParameteri (0x%x, 0x%x, 0x%x)", target, pname, param));
}
static void APIENTRY dbgTexParameteriv (GLenum target, GLenum pname, const GLint *params) {
	dllTexParameteriv (target, pname, params);
	if (!glBegan) GL_CheckForError ("glTexParameteriv");
}
static void APIENTRY dbgTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
	dllTexSubImage1D (target, level, xoffset, width, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glTexSubImage1D");
}
static void APIENTRY dbgTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	dllTexSubImage2D (target, level, xoffset, yoffset, width, height, format, type, pixels);
	if (!glBegan) GL_CheckForError ("glTexSubImage2D");
}
static void APIENTRY dbgTranslated (GLdouble x, GLdouble y, GLdouble z) {
	dllTranslated (x, y, z);
	if (!glBegan) GL_CheckForError ("glTranslated");
}
static void APIENTRY dbgTranslatef (GLfloat x, GLfloat y, GLfloat z) {
	dllTranslatef (x, y, z);
	if (!glBegan) GL_CheckForError ("glTranslatef");
}
static void APIENTRY dbgVertex2d (GLdouble x, GLdouble y) {
	dllVertex2d (x, y);
	if (!glBegan) GL_CheckForError ("glVertex2d");
}
static void APIENTRY dbgVertex2dv (const GLdouble *v) {
	dllVertex2dv (v);
	if (!glBegan) GL_CheckForError ("glVertex2dv");
}
static void APIENTRY dbgVertex2f (GLfloat x, GLfloat y) {
	dllVertex2f (x, y);
	if (!glBegan) GL_CheckForError ("glVertex2f");
}
static void APIENTRY dbgVertex2fv (const GLfloat *v) {
	dllVertex2fv (v);
	if (!glBegan) GL_CheckForError ("glVertex2fv");
}
static void APIENTRY dbgVertex2i (GLint x, GLint y) {
	dllVertex2i (x, y);
	if (!glBegan) GL_CheckForError ("glVertex2i");
}
static void APIENTRY dbgVertex2iv (const GLint *v) {
	dllVertex2iv (v);
	if (!glBegan) GL_CheckForError ("glVertex2iv");
}
static void APIENTRY dbgVertex2s (GLshort x, GLshort y) {
	dllVertex2s (x, y);
	if (!glBegan) GL_CheckForError ("glVertex2s");
}
static void APIENTRY dbgVertex2sv (const GLshort *v) {
	dllVertex2sv (v);
	if (!glBegan) GL_CheckForError ("glVertex2sv");
}
static void APIENTRY dbgVertex3d (GLdouble x, GLdouble y, GLdouble z) {
	dllVertex3d (x, y, z);
	if (!glBegan) GL_CheckForError ("glVertex3d");
}
static void APIENTRY dbgVertex3dv (const GLdouble *v) {
	dllVertex3dv (v);
	if (!glBegan) GL_CheckForError ("glVertex3dv");
}
static void APIENTRY dbgVertex3f (GLfloat x, GLfloat y, GLfloat z) {
	dllVertex3f (x, y, z);
	if (!glBegan) GL_CheckForError ("glVertex3f");
}
static void APIENTRY dbgVertex3fv (const GLfloat *v) {
	dllVertex3fv (v);
	if (!glBegan) GL_CheckForError ("glVertex3fv");
}
static void APIENTRY dbgVertex3i (GLint x, GLint y, GLint z) {
	dllVertex3i (x, y, z);
	if (!glBegan) GL_CheckForError ("glVertex3i");
}
static void APIENTRY dbgVertex3iv (const GLint *v) {
	dllVertex3iv (v);
	if (!glBegan) GL_CheckForError ("glVertex3iv");
}
static void APIENTRY dbgVertex3s (GLshort x, GLshort y, GLshort z) {
	dllVertex3s (x, y, z);
	if (!glBegan) GL_CheckForError ("glVertex3s");
}
static void APIENTRY dbgVertex3sv (const GLshort *v) {
	dllVertex3sv (v);
	if (!glBegan) GL_CheckForError ("glVertex3sv");
}
static void APIENTRY dbgVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	dllVertex4d (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glVertex4d");
}
static void APIENTRY dbgVertex4dv (const GLdouble *v) {
	dllVertex4dv (v);
	if (!glBegan) GL_CheckForError ("glVertex4dv");
}
static void APIENTRY dbgVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	dllVertex4f (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glVertex4f");
}
static void APIENTRY dbgVertex4fv (const GLfloat *v) {
	dllVertex4fv (v);
	if (!glBegan) GL_CheckForError ("glVertex4fv");
}
static void APIENTRY dbgVertex4i (GLint x, GLint y, GLint z, GLint w) {
	dllVertex4i (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glVertex4i");
}
static void APIENTRY dbgVertex4iv (const GLint *v) {
	dllVertex4iv (v);
	if (!glBegan) GL_CheckForError ("glVertex4iv");
}
static void APIENTRY dbgVertex4s (GLshort x, GLshort y, GLshort z, GLshort w) {
	dllVertex4s (x, y, z, w);
	if (!glBegan) GL_CheckForError ("glVertex4s");
}
static void APIENTRY dbgVertex4sv (const GLshort *v) {
	dllVertex4sv (v);
	if (!glBegan) GL_CheckForError ("glVertex4sv");
}
static void APIENTRY dbgVertexPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
	dllVertexPointer (size, type, stride, pointer);
	if (!glBegan) GL_CheckForError ("glVertexPointer");
}
static void APIENTRY dbgViewport (GLint x, GLint y, GLsizei width, GLsizei height) {
	dllViewport (x, y, width, height);
	if (!glBegan) GL_CheckForError ("glViewport");
}


/*
===========
QGL_Shutdown

Unloads the specified DLL then nulls out all the proc pointers.
===========
*/
void QGL_Shutdown (void)
{
#ifdef WIN32
	if (glwState.hInstOpenGL) {
		FreeLibrary (glwState.hInstOpenGL);
		glwState.hInstOpenGL = NULL;
	}
#elif defined __unix__
	/* Not unloading because of the XCloseDisplay related bug, it will hopefully be fixed someday
	if (glxState.OpenGLLib)
		dlclose (glxState.OpenGLLib);
	glxState.OpenGLLib = NULL;
	*/
#endif

	qglAccum					= NULL;
	qglAlphaFunc				= NULL;
	qglAreTexturesResident		= NULL;
	qglArrayElement				= NULL;
	qglBegin					= NULL;
	qglBindTexture				= NULL;
	qglBitmap					= NULL;
	qglBlendFunc				= NULL;
	qglCallList					= NULL;
	qglCallLists				= NULL;
	qglClear					= NULL;
	qglClearAccum				= NULL;
	qglClearColor				= NULL;
	qglClearDepth				= NULL;
	qglClearIndex				= NULL;
	qglClearStencil				= NULL;
	qglClipPlane				= NULL;
	qglColor3b					= NULL;
	qglColor3bv					= NULL;
	qglColor3d					= NULL;
	qglColor3dv					= NULL;
	qglColor3f					= NULL;
	qglColor3fv					= NULL;
	qglColor3i					= NULL;
	qglColor3iv					= NULL;
	qglColor3s					= NULL;
	qglColor3sv					= NULL;
	qglColor3ub					= NULL;
	qglColor3ubv				= NULL;
	qglColor3ui					= NULL;
	qglColor3uiv				= NULL;
	qglColor3us					= NULL;
	qglColor3usv				= NULL;
	qglColor4b					= NULL;
	qglColor4bv					= NULL;
	qglColor4d					= NULL;
	qglColor4dv					= NULL;
	qglColor4f					= NULL;
	qglColor4fv					= NULL;
	qglColor4i					= NULL;
	qglColor4iv					= NULL;
	qglColor4s					= NULL;
	qglColor4sv					= NULL;
	qglColor4ub					= NULL;
	qglColor4ubv				= NULL;
	qglColor4ui					= NULL;
	qglColor4uiv				= NULL;
	qglColor4us					= NULL;
	qglColor4usv				= NULL;
	qglColorMask				= NULL;
	qglColorMaterial			= NULL;
	qglColorPointer				= NULL;
	qglCopyPixels				= NULL;
	qglCopyTexImage1D			= NULL;
	qglCopyTexImage2D			= NULL;
	qglCopyTexSubImage1D		= NULL;
	qglCopyTexSubImage2D		= NULL;
	qglCullFace					= NULL;
	qglDeleteLists				= NULL;
	qglDeleteTextures			= NULL;
	qglDepthFunc				= NULL;
	qglDepthMask				= NULL;
	qglDepthRange				= NULL;
	qglDisable					= NULL;
	qglDisableClientState		= NULL;
	qglDrawArrays				= NULL;
	qglDrawBuffer				= NULL;
	qglDrawElements				= NULL;
	qglDrawPixels				= NULL;
	qglEdgeFlag					= NULL;
	qglEdgeFlagPointer			= NULL;
	qglEdgeFlagv				= NULL;
	qglEnable					= NULL;
	qglEnableClientState		= NULL;
	qglEnd						= NULL;
	qglEndList					= NULL;
	qglEvalCoord1d				= NULL;
	qglEvalCoord1dv				= NULL;
	qglEvalCoord1f				= NULL;
	qglEvalCoord1fv				= NULL;
	qglEvalCoord2d				= NULL;
	qglEvalCoord2dv				= NULL;
	qglEvalCoord2f				= NULL;
	qglEvalCoord2fv				= NULL;
	qglEvalMesh1				= NULL;
	qglEvalMesh2				= NULL;
	qglEvalPoint1				= NULL;
	qglEvalPoint2				= NULL;
	qglFeedbackBuffer			= NULL;
	qglFinish					= NULL;
	qglFlush					= NULL;
	qglFogf						= NULL;
	qglFogfv					= NULL;
	qglFogi						= NULL;
	qglFogiv					= NULL;
	qglFrontFace				= NULL;
	qglFrustum					= NULL;
	qglGenLists					= NULL;
	qglGenTextures				= NULL;
	qglGetBooleanv				= NULL;
	qglGetClipPlane				= NULL;
	qglGetDoublev				= NULL;
	qglGetError					= NULL;
	qglGetFloatv				= NULL;
	qglGetIntegerv				= NULL;
	qglGetLightfv				= NULL;
	qglGetLightiv				= NULL;
	qglGetMapdv					= NULL;
	qglGetMapfv					= NULL;
	qglGetMapiv					= NULL;
	qglGetMaterialfv			= NULL;
	qglGetMaterialiv			= NULL;
	qglGetPixelMapfv			= NULL;
	qglGetPixelMapuiv			= NULL;
	qglGetPixelMapusv			= NULL;
	qglGetPointerv				= NULL;
	qglGetPolygonStipple		= NULL;
	qglGetString				= NULL;
	qglGetTexEnvfv				= NULL;
	qglGetTexEnviv				= NULL;
	qglGetTexGendv				= NULL;
	qglGetTexGenfv				= NULL;
	qglGetTexGeniv				= NULL;
	qglGetTexImage				= NULL;
	qglGetTexLevelParameterfv	= NULL;
	qglGetTexLevelParameteriv	= NULL;
	qglGetTexParameterfv		= NULL;
	qglGetTexParameteriv		= NULL;
	qglHint						= NULL;
	qglIndexMask				= NULL;
	qglIndexPointer				= NULL;
	qglIndexd					= NULL;
	qglIndexdv					= NULL;
	qglIndexf					= NULL;
	qglIndexfv					= NULL;
	qglIndexi					= NULL;
	qglIndexiv					= NULL;
	qglIndexs					= NULL;
	qglIndexsv					= NULL;
	qglIndexub					= NULL;
	qglIndexubv					= NULL;
	qglInitNames				= NULL;
	qglInterleavedArrays		= NULL;
	qglIsEnabled				= NULL;
	qglIsList					= NULL;
	qglIsTexture				= NULL;
	qglLightModelf				= NULL;
	qglLightModelfv				= NULL;
	qglLightModeli				= NULL;
	qglLightModeliv				= NULL;
	qglLightf					= NULL;
	qglLightfv					= NULL;
	qglLighti					= NULL;
	qglLightiv					= NULL;
	qglLineStipple				= NULL;
	qglLineWidth				= NULL;
	qglListBase					= NULL;
	qglLoadIdentity				= NULL;
	qglLoadMatrixd				= NULL;
	qglLoadMatrixf				= NULL;
	qglLoadName					= NULL;
	qglLogicOp					= NULL;
	qglMap1d					= NULL;
	qglMap1f					= NULL;
	qglMap2d					= NULL;
	qglMap2f					= NULL;
	qglMapGrid1d				= NULL;
	qglMapGrid1f				= NULL;
	qglMapGrid2d				= NULL;
	qglMapGrid2f				= NULL;
	qglMaterialf				= NULL;
	qglMaterialfv				= NULL;
	qglMateriali				= NULL;
	qglMaterialiv				= NULL;
	qglMatrixMode				= NULL;
	qglMultMatrixd				= NULL;
	qglMultMatrixf				= NULL;
	qglNewList					= NULL;
	qglNormal3b					= NULL;
	qglNormal3bv				= NULL;
	qglNormal3d					= NULL;
	qglNormal3dv				= NULL;
	qglNormal3f					= NULL;
	qglNormal3fv				= NULL;
	qglNormal3i					= NULL;
	qglNormal3iv				= NULL;
	qglNormal3s					= NULL;
	qglNormal3sv				= NULL;
	qglNormalPointer			= NULL;
	qglOrtho					= NULL;
	qglPassThrough				= NULL;
	qglPixelMapfv				= NULL;
	qglPixelMapuiv				= NULL;
	qglPixelMapusv				= NULL;
	qglPixelStoref				= NULL;
	qglPixelStorei				= NULL;
	qglPixelTransferf			= NULL;
	qglPixelTransferi			= NULL;
	qglPixelZoom				= NULL;
	qglPointSize				= NULL;
	qglPolygonMode				= NULL;
	qglPolygonOffset			= NULL;
	qglPolygonStipple			= NULL;
	qglPopAttrib				= NULL;
	qglPopClientAttrib			= NULL;
	qglPopMatrix				= NULL;
	qglPopName					= NULL;
	qglPrioritizeTextures		= NULL;
	qglPushAttrib				= NULL;
	qglPushClientAttrib			= NULL;
	qglPushMatrix				= NULL;
	qglPushName					= NULL;
	qglRasterPos2d				= NULL;
	qglRasterPos2dv				= NULL;
	qglRasterPos2f				= NULL;
	qglRasterPos2fv				= NULL;
	qglRasterPos2i				= NULL;
	qglRasterPos2iv				= NULL;
	qglRasterPos2s				= NULL;
	qglRasterPos2sv				= NULL;
	qglRasterPos3d				= NULL;
	qglRasterPos3dv				= NULL;
	qglRasterPos3f				= NULL;
	qglRasterPos3fv				= NULL;
	qglRasterPos3i				= NULL;
	qglRasterPos3iv				= NULL;
	qglRasterPos3s				= NULL;
	qglRasterPos3sv				= NULL;
	qglRasterPos4d				= NULL;
	qglRasterPos4dv				= NULL;
	qglRasterPos4f				= NULL;
	qglRasterPos4fv				= NULL;
	qglRasterPos4i				= NULL;
	qglRasterPos4iv				= NULL;
	qglRasterPos4s				= NULL;
	qglRasterPos4sv				= NULL;
	qglReadBuffer				= NULL;
	qglReadPixels				= NULL;
	qglRectd					= NULL;
	qglRectdv					= NULL;
	qglRectf					= NULL;
	qglRectfv					= NULL;
	qglRecti					= NULL;
	qglRectiv					= NULL;
	qglRects					= NULL;
	qglRectsv					= NULL;
	qglRenderMode				= NULL;
	qglRotated					= NULL;
	qglRotatef					= NULL;
	qglScaled					= NULL;
	qglScalef					= NULL;
	qglScissor					= NULL;
	qglSelectBuffer				= NULL;
	qglShadeModel				= NULL;
	qglStencilFunc				= NULL;
	qglStencilMask				= NULL;
	qglStencilOp				= NULL;
	qglTexCoord1d				= NULL;
	qglTexCoord1dv				= NULL;
	qglTexCoord1f				= NULL;
	qglTexCoord1fv				= NULL;
	qglTexCoord1i				= NULL;
	qglTexCoord1iv				= NULL;
	qglTexCoord1s				= NULL;
	qglTexCoord1sv				= NULL;
	qglTexCoord2d				= NULL;
	qglTexCoord2dv				= NULL;
	qglTexCoord2f				= NULL;
	qglTexCoord2fv				= NULL;
	qglTexCoord2i				= NULL;
	qglTexCoord2iv				= NULL;
	qglTexCoord2s				= NULL;
	qglTexCoord2sv				= NULL;
	qglTexCoord3d				= NULL;
	qglTexCoord3dv				= NULL;
	qglTexCoord3f				= NULL;
	qglTexCoord3fv				= NULL;
	qglTexCoord3i				= NULL;
	qglTexCoord3iv				= NULL;
	qglTexCoord3s				= NULL;
	qglTexCoord3sv				= NULL;
	qglTexCoord4d				= NULL;
	qglTexCoord4dv				= NULL;
	qglTexCoord4f				= NULL;
	qglTexCoord4fv				= NULL;
	qglTexCoord4i				= NULL;
	qglTexCoord4iv				= NULL;
	qglTexCoord4s				= NULL;
	qglTexCoord4sv				= NULL;
	qglTexCoordPointer			= NULL;
	qglTexEnvf					= NULL;
	qglTexEnvfv					= NULL;
	qglTexEnvi					= NULL;
	qglTexEnviv					= NULL;
	qglTexGend					= NULL;
	qglTexGendv					= NULL;
	qglTexGenf					= NULL;
	qglTexGenfv					= NULL;
	qglTexGeni					= NULL;
	qglTexGeniv					= NULL;
	qglTexImage1D				= NULL;
	qglTexImage2D				= NULL;
	qglTexParameterf			= NULL;
	qglTexParameterfv			= NULL;
	qglTexParameteri			= NULL;
	qglTexParameteriv			= NULL;
	qglTexSubImage1D			= NULL;
	qglTexSubImage2D			= NULL;
	qglTranslated				= NULL;
	qglTranslatef				= NULL;
	qglVertex2d					= NULL;
	qglVertex2dv				= NULL;
	qglVertex2f					= NULL;
	qglVertex2fv				= NULL;
	qglVertex2i					= NULL;
	qglVertex2iv				= NULL;
	qglVertex2s					= NULL;
	qglVertex2sv				= NULL;
	qglVertex3d					= NULL;
	qglVertex3dv				= NULL;
	qglVertex3f					= NULL;
	qglVertex3fv				= NULL;
	qglVertex3i					= NULL;
	qglVertex3iv				= NULL;
	qglVertex3s					= NULL;
	qglVertex3sv				= NULL;
	qglVertex4d					= NULL;
	qglVertex4dv				= NULL;
	qglVertex4f					= NULL;
	qglVertex4fv				= NULL;
	qglVertex4i					= NULL;
	qglVertex4iv				= NULL;
	qglVertex4s					= NULL;
	qglVertex4sv				= NULL;
	qglVertexPointer			= NULL;
	qglViewport					= NULL;

#ifdef WIN32
	qwglCopyContext				= NULL;
	qwglCreateContext			= NULL;
	qwglCreateLayerContext		= NULL;
	qwglDeleteContext			= NULL;
	qwglDescribeLayerPlane		= NULL;
	qwglGetCurrentContext		= NULL;
	qwglGetCurrentDC			= NULL;
	qwglGetLayerPaletteEntries	= NULL;
	qwglGetProcAddress			= NULL;
	qwglMakeCurrent				= NULL;
	qwglRealizeLayerPalette		= NULL;
	qwglSetLayerPaletteEntries	= NULL;
	qwglShareLists				= NULL;
	qwglSwapLayerBuffers		= NULL;
	qwglUseFontBitmaps			= NULL;
	qwglUseFontOutlines			= NULL;

	qwglChoosePixelFormat		= NULL;
	qwglDescribePixelFormat		= NULL;
	qwglGetPixelFormat			= NULL;
	qwglSetPixelFormat			= NULL;
	qwglSwapBuffers				= NULL;

	qwglGetDeviceGammaRamp3DFX	= NULL;
	qwglSetDeviceGammaRamp3DFX	= NULL;

	qwglSwapIntervalEXT			= NULL;

#elif defined __unix__
	qglXChooseVisual			= NULL;
	qglXCreateContext			= NULL;
	qglXDestroyContext			= NULL;
	qglXMakeCurrent				= NULL;
	qglXCopyContext				= NULL;
	qglXSwapBuffers				= NULL;
#endif

	qglActiveTextureARB				= NULL;
	qglClientActiveTextureARB		= NULL;
	qglSelectTextureSGIS			= NULL;

	qglVertexAttribPointerARB		= NULL;
	qglEnableVertexAttribArrayARB	= NULL;
	qglDisableVertexAttribArrayARB	= NULL;
	qglBindProgramARB				= NULL;
	qglDeleteProgramsARB			= NULL;
	qglGenProgramsARB				= NULL;
	qglProgramStringARB				= NULL;
	qglProgramEnvParameter4fARB		= NULL;
	qglProgramEnvParameter4fvARB	= NULL;
	qglProgramLocalParameter4fARB	= NULL;
	qglProgramLocalParameter4fvARB	= NULL;
	qglGetProgramivARB				= NULL;

	qglBindBufferARB				= NULL;
	qglDeleteBuffersARB				= NULL;
	qglGenBuffersARB				= NULL;
	qglIsBufferARB					= NULL;
	qglMapBufferARB					= NULL;
	qglUnmapBufferARB				= NULL;
	qglBufferDataARB				= NULL;
	qglBufferSubDataARB				= NULL;

	qglLockArraysEXT				= NULL;
	qglUnlockArraysEXT				= NULL;

	qglDrawRangeElementsEXT			= NULL;

	qglTexImage3D					= NULL;
	qglTexSubImage3D				= NULL;

	qglActiveStencilFaceEXT			= NULL;
}


/*
===========
QGL_Init

This is responsible for binding our qgl function pointers to the appropriate GL stuff.
In Windows this means doing a LoadLibrary and a bunch of calls to GetProcAddress. On other
operating systems we need to do the right thing, whatever that might be.
===========
*/
qBool QGL_Init (const char *dllName)
{
	Com_Printf (0, "QGL_Init: LoadLibrary ( \"%s\" )", dllName);

#ifdef WIN32
	if (!(glwState.hInstOpenGL = LoadLibrary (dllName))) {
		char *buf = NULL;

		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						GetLastError (),
						MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR) &buf,
						0,
						NULL);
		Com_Printf (PRNT_ERROR, " failed\n");
		MessageBox (NULL, buf, "EGL Fatal Error", MB_OK|MB_ICONWARNING);

		Com_Printf (0, "QGL_Init: LoadLibrary ( \"%s\" )", GL_DRIVERNAME);
		if (!(glwState.hInstOpenGL = LoadLibrary (GL_DRIVERNAME))) {
			char *buf = NULL;

			FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							GetLastError (),
							MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
							(LPTSTR) &buf,
							0,
							NULL);
			Com_Printf (PRNT_ERROR, " failed\n");
			MessageBox (NULL, buf, "EGL Fatal Error", MB_OK|MB_ICONWARNING);

			return qFalse;
		}
	}
#elif defined __unix__
	// This is here because of a bug with XCloseDisplay
	if (glxState.OpenGLLib)
		dlclose (glxState.OpenGLLib);
	if ((glxState.OpenGLLib = dlopen (dllName, RTLD_LAZY|RTLD_GLOBAL)) == 0) {
		cVar_t	*basedir;
		char	fn[MAX_OSPATH];
		char	*path;

		// try basedir next
		basedir = Cvar_Register ("basedir", ".", CVAR_READONLY);
		path = basedir->string;
		Q_snprintfz (fn, MAX_OSPATH, "%s/%s", path, dllName);

		Com_Printf (0, "QGL_Init: LoadLibrary ( \"%s\" )", fn);
		if ((glxState.OpenGLLib = dlopen (fn, RTLD_LAZY)) == 0 ) {
			Com_Printf (0, "%s\n", dlerror());
			return qFalse;
		}
	}
#endif
	Com_Printf (0, "\n");

	qglAccum					= dllAccum						= GL_GPA ("glAccum");
	qglAlphaFunc				= dllAlphaFunc					= GL_GPA ("glAlphaFunc");
	qglAreTexturesResident		= dllAreTexturesResident		= GL_GPA ("glAreTexturesResident");
	qglArrayElement				= dllArrayElement				= GL_GPA ("glArrayElement");
	qglBegin					= dllBegin						= GL_GPA ("glBegin");
	qglBindTexture				= dllBindTexture				= GL_GPA ("glBindTexture");
	qglBitmap					= dllBitmap						= GL_GPA ("glBitmap");
	qglBlendFunc				= dllBlendFunc					= GL_GPA ("glBlendFunc");
	qglCallList					= dllCallList					= GL_GPA ("glCallList");
	qglCallLists				= dllCallLists					= GL_GPA ("glCallLists");
	qglClear					= dllClear						= GL_GPA ("glClear");
	qglClearAccum				= dllClearAccum					= GL_GPA ("glClearAccum");
	qglClearColor				= dllClearColor					= GL_GPA ("glClearColor");
	qglClearDepth				= dllClearDepth					= GL_GPA ("glClearDepth");
	qglClearIndex				= dllClearIndex					= GL_GPA ("glClearIndex");
	qglClearStencil				= dllClearStencil				= GL_GPA ("glClearStencil");
	qglClipPlane				= dllClipPlane					= GL_GPA ("glClipPlane");
	qglColor3b					= dllColor3b					= GL_GPA ("glColor3b");
	qglColor3bv					= dllColor3bv					= GL_GPA ("glColor3bv");
	qglColor3d					= dllColor3d					= GL_GPA ("glColor3d");
	qglColor3dv					= dllColor3dv					= GL_GPA ("glColor3dv");
	qglColor3f					= dllColor3f					= GL_GPA ("glColor3f");
	qglColor3fv					= dllColor3fv					= GL_GPA ("glColor3fv");
	qglColor3i					= dllColor3i					= GL_GPA ("glColor3i");
	qglColor3iv					= dllColor3iv					= GL_GPA ("glColor3iv");
	qglColor3s					= dllColor3s					= GL_GPA ("glColor3s");
	qglColor3sv					= dllColor3sv					= GL_GPA ("glColor3sv");
	qglColor3ub					= dllColor3ub					= GL_GPA ("glColor3ub");
	qglColor3ubv				= dllColor3ubv					= GL_GPA ("glColor3ubv");
	qglColor3ui					= dllColor3ui					= GL_GPA ("glColor3ui");
	qglColor3uiv				= dllColor3uiv					= GL_GPA ("glColor3uiv");
	qglColor3us					= dllColor3us					= GL_GPA ("glColor3us");
	qglColor3usv				= dllColor3usv					= GL_GPA ("glColor3usv");
	qglColor4b					= dllColor4b					= GL_GPA ("glColor4b");
	qglColor4bv					= dllColor4bv					= GL_GPA ("glColor4bv");
	qglColor4d					= dllColor4d					= GL_GPA ("glColor4d");
	qglColor4dv					= dllColor4dv					= GL_GPA ("glColor4dv");
	qglColor4f					= dllColor4f					= GL_GPA ("glColor4f");
	qglColor4fv					= dllColor4fv					= GL_GPA ("glColor4fv");
	qglColor4i					= dllColor4i					= GL_GPA ("glColor4i");
	qglColor4iv					= dllColor4iv					= GL_GPA ("glColor4iv");
	qglColor4s					= dllColor4s					= GL_GPA ("glColor4s");
	qglColor4sv					= dllColor4sv					= GL_GPA ("glColor4sv");
	qglColor4ub					= dllColor4ub					= GL_GPA ("glColor4ub");
	qglColor4ubv				= dllColor4ubv					= GL_GPA ("glColor4ubv");
	qglColor4ui					= dllColor4ui					= GL_GPA ("glColor4ui");
	qglColor4uiv				= dllColor4uiv					= GL_GPA ("glColor4uiv");
	qglColor4us					= dllColor4us					= GL_GPA ("glColor4us");
	qglColor4usv				= dllColor4usv					= GL_GPA ("glColor4usv");
	qglColorMask				= dllColorMask					= GL_GPA ("glColorMask");
	qglColorMaterial			= dllColorMaterial				= GL_GPA ("glColorMaterial");
	qglColorPointer				= dllColorPointer				= GL_GPA ("glColorPointer");
	qglCopyPixels				= dllCopyPixels					= GL_GPA ("glCopyPixels");
	qglCopyTexImage1D			= dllCopyTexImage1D				= GL_GPA ("glCopyTexImage1D");
	qglCopyTexImage2D			= dllCopyTexImage2D				= GL_GPA ("glCopyTexImage2D");
	qglCopyTexSubImage1D		= dllCopyTexSubImage1D			= GL_GPA ("glCopyTexSubImage1D");
	qglCopyTexSubImage2D		= dllCopyTexSubImage2D			= GL_GPA ("glCopyTexSubImage2D");
	qglCullFace					= dllCullFace					= GL_GPA ("glCullFace");
	qglDeleteLists				= dllDeleteLists				= GL_GPA ("glDeleteLists");
	qglDeleteTextures			= dllDeleteTextures				= GL_GPA ("glDeleteTextures");
	qglDepthFunc				= dllDepthFunc					= GL_GPA ("glDepthFunc");
	qglDepthMask				= dllDepthMask					= GL_GPA ("glDepthMask");
	qglDepthRange				= dllDepthRange					= GL_GPA ("glDepthRange");
	qglDisable					= dllDisable					= GL_GPA ("glDisable");
	qglDisableClientState		= dllDisableClientState			= GL_GPA ("glDisableClientState");
	qglDrawArrays				= dllDrawArrays					= GL_GPA ("glDrawArrays");
	qglDrawBuffer				= dllDrawBuffer					= GL_GPA ("glDrawBuffer");
	qglDrawElements				= dllDrawElements				= GL_GPA ("glDrawElements");
	qglDrawPixels				= dllDrawPixels					= GL_GPA ("glDrawPixels");
	qglEdgeFlag					= dllEdgeFlag					= GL_GPA ("glEdgeFlag");
	qglEdgeFlagPointer			= dllEdgeFlagPointer			= GL_GPA ("glEdgeFlagPointer");
	qglEdgeFlagv				= dllEdgeFlagv					= GL_GPA ("glEdgeFlagv");
	qglEnable					= dllEnable						= GL_GPA ("glEnable");
	qglEnableClientState		= dllEnableClientState			= GL_GPA ("glEnableClientState");
	qglEnd						= dllEnd						= GL_GPA ("glEnd");
	qglEndList					= dllEndList					= GL_GPA ("glEndList");
	qglEvalCoord1d				= dllEvalCoord1d				= GL_GPA ("glEvalCoord1d");
	qglEvalCoord1dv				= dllEvalCoord1dv				= GL_GPA ("glEvalCoord1dv");
	qglEvalCoord1f				= dllEvalCoord1f				= GL_GPA ("glEvalCoord1f");
	qglEvalCoord1fv				= dllEvalCoord1fv				= GL_GPA ("glEvalCoord1fv");
	qglEvalCoord2d				= dllEvalCoord2d				= GL_GPA ("glEvalCoord2d");
	qglEvalCoord2dv				= dllEvalCoord2dv				= GL_GPA ("glEvalCoord2dv");
	qglEvalCoord2f				= dllEvalCoord2f				= GL_GPA ("glEvalCoord2f");
	qglEvalCoord2fv				= dllEvalCoord2fv				= GL_GPA ("glEvalCoord2fv");
	qglEvalMesh1				= dllEvalMesh1					= GL_GPA ("glEvalMesh1");
	qglEvalMesh2				= dllEvalMesh2					= GL_GPA ("glEvalMesh2");
	qglEvalPoint1				= dllEvalPoint1					= GL_GPA ("glEvalPoint1");
	qglEvalPoint2				= dllEvalPoint2					= GL_GPA ("glEvalPoint2");
	qglFeedbackBuffer			= dllFeedbackBuffer				= GL_GPA ("glFeedbackBuffer");
	qglFinish					= dllFinish						= GL_GPA ("glFinish");
	qglFlush					= dllFlush						= GL_GPA ("glFlush");
	qglFogf						= dllFogf						= GL_GPA ("glFogf");
	qglFogfv					= dllFogfv						= GL_GPA ("glFogfv");
	qglFogi						= dllFogi						= GL_GPA ("glFogi");
	qglFogiv					= dllFogiv						= GL_GPA ("glFogiv");
	qglFrontFace				= dllFrontFace					= GL_GPA ("glFrontFace");
	qglFrustum					= dllFrustum					= GL_GPA ("glFrustum");
	qglGenLists					= dllGenLists					= GL_GPA ("glGenLists");
	qglGenTextures				= dllGenTextures				= GL_GPA ("glGenTextures");
	qglGetBooleanv				= dllGetBooleanv				= GL_GPA ("glGetBooleanv");
	qglGetClipPlane				= dllGetClipPlane				= GL_GPA ("glGetClipPlane");
	qglGetDoublev				= dllGetDoublev					= GL_GPA ("glGetDoublev");
	qglGetError					= dllGetError					= GL_GPA ("glGetError");
	qglGetFloatv				= dllGetFloatv					= GL_GPA ("glGetFloatv");
	qglGetIntegerv				= dllGetIntegerv				= GL_GPA ("glGetIntegerv");
	qglGetLightfv				= dllGetLightfv					= GL_GPA ("glGetLightfv");
	qglGetLightiv				= dllGetLightiv					= GL_GPA ("glGetLightiv");
	qglGetMapdv					= dllGetMapdv					= GL_GPA ("glGetMapdv");
	qglGetMapfv					= dllGetMapfv					= GL_GPA ("glGetMapfv");
	qglGetMapiv					= dllGetMapiv					= GL_GPA ("glGetMapiv");
	qglGetMaterialfv			= dllGetMaterialfv				= GL_GPA ("glGetMaterialfv");
	qglGetMaterialiv			= dllGetMaterialiv				= GL_GPA ("glGetMaterialiv");
	qglGetPixelMapfv			= dllGetPixelMapfv				= GL_GPA ("glGetPixelMapfv");
	qglGetPixelMapuiv			= dllGetPixelMapuiv				= GL_GPA ("glGetPixelMapuiv");
	qglGetPixelMapusv			= dllGetPixelMapusv				= GL_GPA ("glGetPixelMapusv");
	qglGetPointerv				= dllGetPointerv				= GL_GPA ("glGetPointerv");
	qglGetPolygonStipple		= dllGetPolygonStipple			= GL_GPA ("glGetPolygonStipple");
	qglGetString				= dllGetString					= GL_GPA ("glGetString");
	qglGetTexEnvfv				= dllGetTexEnvfv				= GL_GPA ("glGetTexEnvfv");
	qglGetTexEnviv				= dllGetTexEnviv				= GL_GPA ("glGetTexEnviv");
	qglGetTexGendv				= dllGetTexGendv				= GL_GPA ("glGetTexGendv");
	qglGetTexGenfv				= dllGetTexGenfv				= GL_GPA ("glGetTexGenfv");
	qglGetTexGeniv				= dllGetTexGeniv				= GL_GPA ("glGetTexGeniv");
	qglGetTexImage				= dllGetTexImage				= GL_GPA ("glGetTexImage");
	qglGetTexLevelParameterfv	= dllGetTexLevelParameterfv		= GL_GPA ("glGetLevelParameterfv");
	qglGetTexLevelParameteriv	= dllGetTexLevelParameteriv		= GL_GPA ("glGetLevelParameteriv");
	qglGetTexParameterfv		= dllGetTexParameterfv			= GL_GPA ("glGetTexParameterfv");
	qglGetTexParameteriv		= dllGetTexParameteriv			= GL_GPA ("glGetTexParameteriv");
	qglHint						= dllHint						= GL_GPA ("glHint");
	qglIndexMask				= dllIndexMask					= GL_GPA ("glIndexMask");
	qglIndexPointer				= dllIndexPointer				= GL_GPA ("glIndexPointer");
	qglIndexd					= dllIndexd						= GL_GPA ("glIndexd");
	qglIndexdv					= dllIndexdv					= GL_GPA ("glIndexdv");
	qglIndexf					= dllIndexf						= GL_GPA ("glIndexf");
	qglIndexfv					= dllIndexfv					= GL_GPA ("glIndexfv");
	qglIndexi					= dllIndexi						= GL_GPA ("glIndexi");
	qglIndexiv					= dllIndexiv					= GL_GPA ("glIndexiv");
	qglIndexs					= dllIndexs						= GL_GPA ("glIndexs");
	qglIndexsv					= dllIndexsv					= GL_GPA ("glIndexsv");
	qglIndexub					= dllIndexub					= GL_GPA ("glIndexub");
	qglIndexubv					= dllIndexubv					= GL_GPA ("glIndexubv");
	qglInitNames				= dllInitNames					= GL_GPA ("glInitNames");
	qglInterleavedArrays		= dllInterleavedArrays			= GL_GPA ("glInterleavedArrays");
	qglIsEnabled				= dllIsEnabled					= GL_GPA ("glIsEnabled");
	qglIsList					= dllIsList						= GL_GPA ("glIsList");
	qglIsTexture				= dllIsTexture					= GL_GPA ("glIsTexture");
	qglLightModelf				= dllLightModelf				= GL_GPA ("glLightModelf");
	qglLightModelfv				= dllLightModelfv				= GL_GPA ("glLightModelfv");
	qglLightModeli				= dllLightModeli				= GL_GPA ("glLightModeli");
	qglLightModeliv				= dllLightModeliv				= GL_GPA ("glLightModeliv");
	qglLightf					= dllLightf						= GL_GPA ("glLightf");
	qglLightfv					= dllLightfv					= GL_GPA ("glLightfv");
	qglLighti					= dllLighti						= GL_GPA ("glLighti");
	qglLightiv					= dllLightiv					= GL_GPA ("glLightiv");
	qglLineStipple				= dllLineStipple				= GL_GPA ("glLineStipple");
	qglLineWidth				= dllLineWidth					= GL_GPA ("glLineWidth");
	qglListBase					= dllListBase					= GL_GPA ("glListBase");
	qglLoadIdentity				= dllLoadIdentity				= GL_GPA ("glLoadIdentity");
	qglLoadMatrixd				= dllLoadMatrixd				= GL_GPA ("glLoadMatrixd");
	qglLoadMatrixf				= dllLoadMatrixf				= GL_GPA ("glLoadMatrixf");
	qglLoadName					= dllLoadName					= GL_GPA ("glLoadName");
	qglLogicOp					= dllLogicOp					= GL_GPA ("glLogicOp");
	qglMap1d					= dllMap1d						= GL_GPA ("glMap1d");
	qglMap1f					= dllMap1f						= GL_GPA ("glMap1f");
	qglMap2d					= dllMap2d						= GL_GPA ("glMap2d");
	qglMap2f					= dllMap2f						= GL_GPA ("glMap2f");
	qglMapGrid1d				= dllMapGrid1d					= GL_GPA ("glMapGrid1d");
	qglMapGrid1f				= dllMapGrid1f					= GL_GPA ("glMapGrid1f");
	qglMapGrid2d				= dllMapGrid2d					= GL_GPA ("glMapGrid2d");
	qglMapGrid2f				= dllMapGrid2f					= GL_GPA ("glMapGrid2f");
	qglMaterialf				= dllMaterialf					= GL_GPA ("glMaterialf");
	qglMaterialfv				= dllMaterialfv					= GL_GPA ("glMaterialfv");
	qglMateriali				= dllMateriali					= GL_GPA ("glMateriali");
	qglMaterialiv				= dllMaterialiv					= GL_GPA ("glMaterialiv");
	qglMatrixMode				= dllMatrixMode					= GL_GPA ("glMatrixMode");
	qglMultMatrixd				= dllMultMatrixd				= GL_GPA ("glMultMatrixd");
	qglMultMatrixf				= dllMultMatrixf				= GL_GPA ("glMultMatrixf");
	qglNewList					= dllNewList					= GL_GPA ("glNewList");
	qglNormal3b					= dllNormal3b					= GL_GPA ("glNormal3b");
	qglNormal3bv				= dllNormal3bv					= GL_GPA ("glNormal3bv");
	qglNormal3d					= dllNormal3d					= GL_GPA ("glNormal3d");
	qglNormal3dv				= dllNormal3dv					= GL_GPA ("glNormal3dv");
	qglNormal3f					= dllNormal3f					= GL_GPA ("glNormal3f");
	qglNormal3fv				= dllNormal3fv					= GL_GPA ("glNormal3fv");
	qglNormal3i					= dllNormal3i					= GL_GPA ("glNormal3i");
	qglNormal3iv				= dllNormal3iv					= GL_GPA ("glNormal3iv");
	qglNormal3s					= dllNormal3s					= GL_GPA ("glNormal3s");
	qglNormal3sv				= dllNormal3sv					= GL_GPA ("glNormal3sv");
	qglNormalPointer			= dllNormalPointer				= GL_GPA ("glNormalPointer");
	qglOrtho					= dllOrtho						= GL_GPA ("glOrtho");
	qglPassThrough				= dllPassThrough				= GL_GPA ("glPassThrough");
	qglPixelMapfv				= dllPixelMapfv					= GL_GPA ("glPixelMapfv");
	qglPixelMapuiv				= dllPixelMapuiv				= GL_GPA ("glPixelMapuiv");
	qglPixelMapusv				= dllPixelMapusv				= GL_GPA ("glPixelMapusv");
	qglPixelStoref				= dllPixelStoref				= GL_GPA ("glPixelStoref");
	qglPixelStorei				= dllPixelStorei				= GL_GPA ("glPixelStorei");
	qglPixelTransferf			= dllPixelTransferf				= GL_GPA ("glPixelTransferf");
	qglPixelTransferi			= dllPixelTransferi				= GL_GPA ("glPixelTransferi");
	qglPixelZoom				= dllPixelZoom					= GL_GPA ("glPixelZoom");
	qglPointSize				= dllPointSize					= GL_GPA ("glPointSize");
	qglPolygonMode				= dllPolygonMode				= GL_GPA ("glPolygonMode");
	qglPolygonOffset			= dllPolygonOffset				= GL_GPA ("glPolygonOffset");
	qglPolygonStipple			= dllPolygonStipple				= GL_GPA ("glPolygonStipple");
	qglPopAttrib				= dllPopAttrib					= GL_GPA ("glPopAttrib");
	qglPopClientAttrib			= dllPopClientAttrib			= GL_GPA ("glPopClientAttrib");
	qglPopMatrix				= dllPopMatrix					= GL_GPA ("glPopMatrix");
	qglPopName					= dllPopName					= GL_GPA ("glPopName");
	qglPrioritizeTextures		= dllPrioritizeTextures			= GL_GPA ("glPrioritizeTextures");
	qglPushAttrib				= dllPushAttrib					= GL_GPA ("glPushAttrib");
	qglPushClientAttrib			= dllPushClientAttrib			= GL_GPA ("glPushClientAttrib");
	qglPushMatrix				= dllPushMatrix					= GL_GPA ("glPushMatrix");
	qglPushName					= dllPushName					= GL_GPA ("glPushName");
	qglRasterPos2d				= dllRasterPos2d				= GL_GPA ("glRasterPos2d");
	qglRasterPos2dv				= dllRasterPos2dv				= GL_GPA ("glRasterPos2dv");
	qglRasterPos2f				= dllRasterPos2f				= GL_GPA ("glRasterPos2f");
	qglRasterPos2fv				= dllRasterPos2fv				= GL_GPA ("glRasterPos2fv");
	qglRasterPos2i				= dllRasterPos2i				= GL_GPA ("glRasterPos2i");
	qglRasterPos2iv				= dllRasterPos2iv				= GL_GPA ("glRasterPos2iv");
	qglRasterPos2s				= dllRasterPos2s				= GL_GPA ("glRasterPos2s");
	qglRasterPos2sv				= dllRasterPos2sv				= GL_GPA ("glRasterPos2sv");
	qglRasterPos3d				= dllRasterPos3d				= GL_GPA ("glRasterPos3d");
	qglRasterPos3dv				= dllRasterPos3dv				= GL_GPA ("glRasterPos3dv");
	qglRasterPos3f				= dllRasterPos3f				= GL_GPA ("glRasterPos3f");
	qglRasterPos3fv				= dllRasterPos3fv				= GL_GPA ("glRasterPos3fv");
	qglRasterPos3i				= dllRasterPos3i				= GL_GPA ("glRasterPos3i");
	qglRasterPos3iv				= dllRasterPos3iv				= GL_GPA ("glRasterPos3iv");
	qglRasterPos3s				= dllRasterPos3s				= GL_GPA ("glRasterPos3s");
	qglRasterPos3sv				= dllRasterPos3sv				= GL_GPA ("glRasterPos3sv");
	qglRasterPos4d				= dllRasterPos4d				= GL_GPA ("glRasterPos4d");
	qglRasterPos4dv				= dllRasterPos4dv				= GL_GPA ("glRasterPos4dv");
	qglRasterPos4f				= dllRasterPos4f				= GL_GPA ("glRasterPos4f");
	qglRasterPos4fv				= dllRasterPos4fv				= GL_GPA ("glRasterPos4fv");
	qglRasterPos4i				= dllRasterPos4i				= GL_GPA ("glRasterPos4i");
	qglRasterPos4iv				= dllRasterPos4iv				= GL_GPA ("glRasterPos4iv");
	qglRasterPos4s				= dllRasterPos4s				= GL_GPA ("glRasterPos4s");
	qglRasterPos4sv				= dllRasterPos4sv				= GL_GPA ("glRasterPos4sv");
	qglReadBuffer				= dllReadBuffer					= GL_GPA ("glReadBuffer");
	qglReadPixels				= dllReadPixels					= GL_GPA ("glReadPixels");
	qglRectd					= dllRectd						= GL_GPA ("glRectd");
	qglRectdv					= dllRectdv						= GL_GPA ("glRectdv");
	qglRectf					= dllRectf						= GL_GPA ("glRectf");
	qglRectfv					= dllRectfv						= GL_GPA ("glRectfv");
	qglRecti					= dllRecti						= GL_GPA ("glRecti");
	qglRectiv					= dllRectiv						= GL_GPA ("glRectiv");
	qglRects					= dllRects						= GL_GPA ("glRects");
	qglRectsv					= dllRectsv						= GL_GPA ("glRectsv");
	qglRenderMode				= dllRenderMode					= GL_GPA ("glRenderMode");
	qglRotated					= dllRotated					= GL_GPA ("glRotated");
	qglRotatef					= dllRotatef					= GL_GPA ("glRotatef");
	qglScaled					= dllScaled						= GL_GPA ("glScaled");
	qglScalef					= dllScalef						= GL_GPA ("glScalef");
	qglScissor					= dllScissor					= GL_GPA ("glScissor");
	qglSelectBuffer				= dllSelectBuffer				= GL_GPA ("glSelectBuffer");
	qglShadeModel				= dllShadeModel					= GL_GPA ("glShadeModel");
	qglStencilFunc				= dllStencilFunc				= GL_GPA ("glStencilFunc");
	qglStencilMask				= dllStencilMask				= GL_GPA ("glStencilMask");
	qglStencilOp				= dllStencilOp					= GL_GPA ("glStencilOp");
	qglTexCoord1d				= dllTexCoord1d					= GL_GPA ("glTexCoord1d");
	qglTexCoord1dv				= dllTexCoord1dv				= GL_GPA ("glTexCoord1dv");
	qglTexCoord1f				= dllTexCoord1f					= GL_GPA ("glTexCoord1f");
	qglTexCoord1fv				= dllTexCoord1fv				= GL_GPA ("glTexCoord1fv");
	qglTexCoord1i				= dllTexCoord1i					= GL_GPA ("glTexCoord1i");
	qglTexCoord1iv				= dllTexCoord1iv				= GL_GPA ("glTexCoord1iv");
	qglTexCoord1s				= dllTexCoord1s					= GL_GPA ("glTexCoord1s");
	qglTexCoord1sv				= dllTexCoord1sv				= GL_GPA ("glTexCoord1sv");
	qglTexCoord2d				= dllTexCoord2d					= GL_GPA ("glTexCoord2d");
	qglTexCoord2dv				= dllTexCoord2dv				= GL_GPA ("glTexCoord2dv");
	qglTexCoord2f				= dllTexCoord2f					= GL_GPA ("glTexCoord2f");
	qglTexCoord2fv				= dllTexCoord2fv				= GL_GPA ("glTexCoord2fv");
	qglTexCoord2i				= dllTexCoord2i					= GL_GPA ("glTexCoord2i");
	qglTexCoord2iv				= dllTexCoord2iv				= GL_GPA ("glTexCoord2iv");
	qglTexCoord2s				= dllTexCoord2s					= GL_GPA ("glTexCoord2s");
	qglTexCoord2sv				= dllTexCoord2sv				= GL_GPA ("glTexCoord2sv");
	qglTexCoord3d				= dllTexCoord3d					= GL_GPA ("glTexCoord3d");
	qglTexCoord3dv				= dllTexCoord3dv				= GL_GPA ("glTexCoord3dv");
	qglTexCoord3f				= dllTexCoord3f					= GL_GPA ("glTexCoord3f");
	qglTexCoord3fv				= dllTexCoord3fv				= GL_GPA ("glTexCoord3fv");
	qglTexCoord3i				= dllTexCoord3i					= GL_GPA ("glTexCoord3i");
	qglTexCoord3iv				= dllTexCoord3iv				= GL_GPA ("glTexCoord3iv");
	qglTexCoord3s				= dllTexCoord3s					= GL_GPA ("glTexCoord3s");
	qglTexCoord3sv				= dllTexCoord3sv				= GL_GPA ("glTexCoord3sv");
	qglTexCoord4d				= dllTexCoord4d					= GL_GPA ("glTexCoord4d");
	qglTexCoord4dv				= dllTexCoord4dv				= GL_GPA ("glTexCoord4dv");
	qglTexCoord4f				= dllTexCoord4f					= GL_GPA ("glTexCoord4f");
	qglTexCoord4fv				= dllTexCoord4fv				= GL_GPA ("glTexCoord4fv");
	qglTexCoord4i				= dllTexCoord4i					= GL_GPA ("glTexCoord4i");
	qglTexCoord4iv				= dllTexCoord4iv				= GL_GPA ("glTexCoord4iv");
	qglTexCoord4s				= dllTexCoord4s					= GL_GPA ("glTexCoord4s");
	qglTexCoord4sv				= dllTexCoord4sv				= GL_GPA ("glTexCoord4sv");
	qglTexCoordPointer			= dllTexCoordPointer			= GL_GPA ("glTexCoordPointer");
	qglTexEnvf					= dllTexEnvf					= GL_GPA ("glTexEnvf");
	qglTexEnvfv					= dllTexEnvfv					= GL_GPA ("glTexEnvfv");
	qglTexEnvi					= dllTexEnvi					= GL_GPA ("glTexEnvi");
	qglTexEnviv					= dllTexEnviv					= GL_GPA ("glTexEnviv");
	qglTexGend					= dllTexGend					= GL_GPA ("glTexGend");
	qglTexGendv					= dllTexGendv					= GL_GPA ("glTexGendv");
	qglTexGenf					= dllTexGenf					= GL_GPA ("glTexGenf");
	qglTexGenfv					= dllTexGenfv					= GL_GPA ("glTexGenfv");
	qglTexGeni					= dllTexGeni					= GL_GPA ("glTexGeni");
	qglTexGeniv					= dllTexGeniv					= GL_GPA ("glTexGeniv");
	qglTexImage1D				= dllTexImage1D					= GL_GPA ("glTexImage1D");
	qglTexImage2D				= dllTexImage2D					= GL_GPA ("glTexImage2D");
	qglTexParameterf			= dllTexParameterf				= GL_GPA ("glTexParameterf");
	qglTexParameterfv			= dllTexParameterfv				= GL_GPA ("glTexParameterfv");
	qglTexParameteri			= dllTexParameteri				= GL_GPA ("glTexParameteri");
	qglTexParameteriv			= dllTexParameteriv				= GL_GPA ("glTexParameteriv");
	qglTexSubImage1D			= dllTexSubImage1D				= GL_GPA ("glTexSubImage1D");
	qglTexSubImage2D			= dllTexSubImage2D				= GL_GPA ("glTexSubImage2D");
	qglTranslated				= dllTranslated					= GL_GPA ("glTranslated");
	qglTranslatef				= dllTranslatef					= GL_GPA ("glTranslatef");
	qglVertex2d					= dllVertex2d					= GL_GPA ("glVertex2d");
	qglVertex2dv				= dllVertex2dv					= GL_GPA ("glVertex2dv");
	qglVertex2f					= dllVertex2f					= GL_GPA ("glVertex2f");
	qglVertex2fv				= dllVertex2fv					= GL_GPA ("glVertex2fv");
	qglVertex2i					= dllVertex2i					= GL_GPA ("glVertex2i");
	qglVertex2iv				= dllVertex2iv					= GL_GPA ("glVertex2iv");
	qglVertex2s					= dllVertex2s					= GL_GPA ("glVertex2s");
	qglVertex2sv				= dllVertex2sv					= GL_GPA ("glVertex2sv");
	qglVertex3d					= dllVertex3d					= GL_GPA ("glVertex3d");
	qglVertex3dv				= dllVertex3dv					= GL_GPA ("glVertex3dv");
	qglVertex3f					= dllVertex3f					= GL_GPA ("glVertex3f");
	qglVertex3fv				= dllVertex3fv					= GL_GPA ("glVertex3fv");
	qglVertex3i					= dllVertex3i					= GL_GPA ("glVertex3i");
	qglVertex3iv				= dllVertex3iv					= GL_GPA ("glVertex3iv");
	qglVertex3s					= dllVertex3s					= GL_GPA ("glVertex3s");
	qglVertex3sv				= dllVertex3sv					= GL_GPA ("glVertex3sv");
	qglVertex4d					= dllVertex4d					= GL_GPA ("glVertex4d");
	qglVertex4dv				= dllVertex4dv					= GL_GPA ("glVertex4dv");
	qglVertex4f					= dllVertex4f					= GL_GPA ("glVertex4f");
	qglVertex4fv				= dllVertex4fv					= GL_GPA ("glVertex4fv");
	qglVertex4i					= dllVertex4i					= GL_GPA ("glVertex4i");
	qglVertex4iv				= dllVertex4iv					= GL_GPA ("glVertex4iv");
	qglVertex4s					= dllVertex4s					= GL_GPA ("glVertex4s");
	qglVertex4sv				= dllVertex4sv					= GL_GPA ("glVertex4sv");
	qglVertexPointer			= dllVertexPointer				= GL_GPA ("glVertexPointer");
	qglViewport					= dllViewport					= GL_GPA ("glViewport");

#ifdef WIN32
	qwglCopyContext				= GL_GPA ("wglCopyContext");
	qwglCreateContext			= GL_GPA ("wglCreateContext");
	qwglCreateLayerContext		= GL_GPA ("wglCreateLayerContext");
	qwglDeleteContext			= GL_GPA ("wglDeleteContext");
	qwglDescribeLayerPlane		= GL_GPA ("wglDescribeLayerPlane");
	qwglGetCurrentContext		= GL_GPA ("wglGetCurrentContext");
	qwglGetCurrentDC			= GL_GPA ("wglGetCurrentDC");
	qwglGetLayerPaletteEntries	= GL_GPA ("wglGetLayerPaletteEntries");
	qwglGetProcAddress			= GL_GPA ("wglGetProcAddress");
	qwglMakeCurrent				= GL_GPA ("wglMakeCurrent");
	qwglRealizeLayerPalette		= GL_GPA ("wglRealizeLayerPalette");
	qwglSetLayerPaletteEntries	= GL_GPA ("wglSetLayerPaletteEntries");
	qwglShareLists				= GL_GPA ("wglShareLists");
	qwglSwapLayerBuffers		= GL_GPA ("wglSwapLayerBuffers");
	qwglUseFontBitmaps			= GL_GPA ("wglUseFontBitmapsA");
	qwglUseFontOutlines			= GL_GPA ("wglUseFontOutlinesA");

	qwglChoosePixelFormat		= GL_GPA ("wglChoosePixelFormat");
	qwglDescribePixelFormat		= GL_GPA ("wglDescribePixelFormat");
	qwglGetPixelFormat			= GL_GPA ("wglGetPixelFormat");
	qwglSetPixelFormat			= GL_GPA ("wglSetPixelFormat");
	qwglSwapBuffers				= GL_GPA ("wglSwapBuffers");

	qwglGetDeviceGammaRamp3DFX	= NULL;
	qwglSetDeviceGammaRamp3DFX	= NULL;

	qwglSwapIntervalEXT			= NULL;
#elif defined __unix__
	qglXChooseVisual			= GL_GPA ("glXChooseVisual");
	qglXCreateContext			= GL_GPA ("glXCreateContext");
	qglXDestroyContext			= GL_GPA ("glXDestroyContext");
	qglXMakeCurrent				= GL_GPA ("glXMakeCurrent");
	qglXCopyContext				= GL_GPA ("glXCopyContext");
	qglXSwapBuffers				= GL_GPA ("glXSwapBuffers");
#endif

	qglActiveTextureARB				= NULL;
	qglClientActiveTextureARB		= NULL;
	qglSelectTextureSGIS			= NULL;

	qglVertexAttribPointerARB		= NULL;
	qglEnableVertexAttribArrayARB	= NULL;
	qglDisableVertexAttribArrayARB	= NULL;
	qglBindProgramARB				= NULL;
	qglDeleteProgramsARB			= NULL;
	qglGenProgramsARB				= NULL;
	qglProgramStringARB				= NULL;
	qglProgramEnvParameter4fARB		= NULL;
	qglProgramEnvParameter4fvARB	= NULL;
	qglProgramLocalParameter4fARB	= NULL;
	qglProgramLocalParameter4fvARB	= NULL;
	qglGetProgramivARB				= NULL;

	qglBindBufferARB				= NULL;
	qglDeleteBuffersARB				= NULL;
	qglGenBuffersARB				= NULL;
	qglIsBufferARB					= NULL;
	qglMapBufferARB					= NULL;
	qglUnmapBufferARB				= NULL;
	qglBufferDataARB				= NULL;
	qglBufferSubDataARB				= NULL;

	qglLockArraysEXT				= NULL;
	qglUnlockArraysEXT				= NULL;

	qglDrawRangeElementsEXT			= NULL;

	qglTexImage3D					= NULL;
	qglTexSubImage3D				= NULL;

	qglActiveStencilFaceEXT			= NULL;

	return qTrue;
}


/*
===========
QGL_ToggleDebug
===========
*/
void QGL_ToggleDebug (void)
{
	if (qgl_debug->intVal) {
		qglAccum					= dbgAccum;
		qglAlphaFunc				= dbgAlphaFunc;
		qglAreTexturesResident		= dbgAreTexturesResident;
		qglArrayElement				= dbgArrayElement;
		qglBegin					= dbgBegin;
		qglBindTexture				= dbgBindTexture;
		qglBitmap					= dbgBitmap;
		qglBlendFunc				= dbgBlendFunc;
		qglCallList					= dbgCallList;
		qglCallLists				= dbgCallLists;
		qglClear					= dbgClear;
		qglClearAccum				= dbgClearAccum;
		qglClearColor				= dbgClearColor;
		qglClearDepth				= dbgClearDepth;
		qglClearIndex				= dbgClearIndex;
		qglClearStencil				= dbgClearStencil;
		qglClipPlane				= dbgClipPlane;
		qglColor3b					= dbgColor3b;
		qglColor3bv					= dbgColor3bv;
		qglColor3d					= dbgColor3d;
		qglColor3dv					= dbgColor3dv;
		qglColor3f					= dbgColor3f;
		qglColor3fv					= dbgColor3fv;
		qglColor3i					= dbgColor3i;
		qglColor3iv					= dbgColor3iv;
		qglColor3s					= dbgColor3s;
		qglColor3sv					= dbgColor3sv;
		qglColor3ub					= dbgColor3ub;
		qglColor3ubv				= dbgColor3ubv;
		qglColor3ui					= dbgColor3ui;
		qglColor3uiv				= dbgColor3uiv;
		qglColor3us					= dbgColor3us;
		qglColor3usv				= dbgColor3usv;
		qglColor4b					= dbgColor4b;
		qglColor4bv					= dbgColor4bv;
		qglColor4d					= dbgColor4d;
		qglColor4dv					= dbgColor4dv;
		qglColor4f					= dbgColor4f;
		qglColor4fv					= dbgColor4fv;
		qglColor4i					= dbgColor4i;
		qglColor4iv					= dbgColor4iv;
		qglColor4s					= dbgColor4s;
		qglColor4sv					= dbgColor4sv;
		qglColor4ub					= dbgColor4ub;
		qglColor4ubv				= dbgColor4ubv;
		qglColor4ui					= dbgColor4ui;
		qglColor4uiv				= dbgColor4uiv;
		qglColor4us					= dbgColor4us;
		qglColor4usv				= dbgColor4usv;
		qglColorMask				= dbgColorMask;
		qglColorMaterial			= dbgColorMaterial;
		qglColorPointer				= dbgColorPointer;
		qglCopyPixels				= dbgCopyPixels;
		qglCopyTexImage1D			= dbgCopyTexImage1D;
		qglCopyTexImage2D			= dbgCopyTexImage2D;
		qglCopyTexSubImage1D		= dbgCopyTexSubImage1D;
		qglCopyTexSubImage2D		= dbgCopyTexSubImage2D;
		qglCullFace					= dbgCullFace;
		qglDeleteLists				= dbgDeleteLists;
		qglDeleteTextures			= dbgDeleteTextures;
		qglDepthFunc				= dbgDepthFunc;
		qglDepthMask				= dbgDepthMask;
		qglDepthRange				= dbgDepthRange;
		qglDisable					= dbgDisable;
		qglDisableClientState		= dbgDisableClientState;
		qglDrawArrays				= dbgDrawArrays;
		qglDrawBuffer				= dbgDrawBuffer;
		qglDrawElements				= dbgDrawElements;
		qglDrawPixels				= dbgDrawPixels;
		qglEdgeFlag					= dbgEdgeFlag;
		qglEdgeFlagPointer			= dbgEdgeFlagPointer;
		qglEdgeFlagv				= dbgEdgeFlagv;
		qglEnable					= dbgEnable;
		qglEnableClientState		= dbgEnableClientState;
		qglEnd						= dbgEnd;
		qglEndList					= dbgEndList;
		qglEvalCoord1d				= dbgEvalCoord1d;
		qglEvalCoord1dv				= dbgEvalCoord1dv;
		qglEvalCoord1f				= dbgEvalCoord1f;
		qglEvalCoord1fv				= dbgEvalCoord1fv;
		qglEvalCoord2d				= dbgEvalCoord2d;
		qglEvalCoord2dv				= dbgEvalCoord2dv;
		qglEvalCoord2f				= dbgEvalCoord2f;
		qglEvalCoord2fv				= dbgEvalCoord2fv;
		qglEvalMesh1				= dbgEvalMesh1;
		qglEvalMesh2				= dbgEvalMesh2;
		qglEvalPoint1				= dbgEvalPoint1;
		qglEvalPoint2				= dbgEvalPoint2;
		qglFeedbackBuffer			= dbgFeedbackBuffer;
		qglFinish					= dbgFinish;
		qglFlush					= dbgFlush;
		qglFogf						= dbgFogf;
		qglFogfv					= dbgFogfv;
		qglFogi						= dbgFogi;
		qglFogiv					= dbgFogiv;
		qglFrontFace				= dbgFrontFace;
		qglFrustum					= dbgFrustum;
		qglGenLists					= dbgGenLists;
		qglGenTextures				= dbgGenTextures;
		qglGetBooleanv				= dbgGetBooleanv;
		qglGetClipPlane				= dbgGetClipPlane;
		qglGetDoublev				= dbgGetDoublev;
		qglGetError					= dbgGetError;
		qglGetFloatv				= dbgGetFloatv;
		qglGetIntegerv				= dbgGetIntegerv;
		qglGetLightfv				= dbgGetLightfv;
		qglGetLightiv				= dbgGetLightiv;
		qglGetMapdv					= dbgGetMapdv;
		qglGetMapfv					= dbgGetMapfv;
		qglGetMapiv					= dbgGetMapiv;
		qglGetMaterialfv			= dbgGetMaterialfv;
		qglGetMaterialiv			= dbgGetMaterialiv;
		qglGetPixelMapfv			= dbgGetPixelMapfv;
		qglGetPixelMapuiv			= dbgGetPixelMapuiv;
		qglGetPixelMapusv			= dbgGetPixelMapusv;
		qglGetPointerv				= dbgGetPointerv;
		qglGetPolygonStipple		= dbgGetPolygonStipple;
		qglGetString				= dbgGetString;
		qglGetTexEnvfv				= dbgGetTexEnvfv;
		qglGetTexEnviv				= dbgGetTexEnviv;
		qglGetTexGendv				= dbgGetTexGendv;
		qglGetTexGenfv				= dbgGetTexGenfv;
		qglGetTexGeniv				= dbgGetTexGeniv;
		qglGetTexImage				= dbgGetTexImage;
		qglGetTexLevelParameterfv	= dbgGetTexLevelParameterfv;
		qglGetTexLevelParameteriv	= dbgGetTexLevelParameteriv;
		qglGetTexParameterfv		= dbgGetTexParameterfv;
		qglGetTexParameteriv		= dbgGetTexParameteriv;
		qglHint						= dbgHint;
		qglIndexMask				= dbgIndexMask;
		qglIndexPointer				= dbgIndexPointer;
		qglIndexd					= dbgIndexd;
		qglIndexdv					= dbgIndexdv;
		qglIndexf					= dbgIndexf;
		qglIndexfv					= dbgIndexfv;
		qglIndexi					= dbgIndexi;
		qglIndexiv					= dbgIndexiv;
		qglIndexs					= dbgIndexs;
		qglIndexsv					= dbgIndexsv;
		qglIndexub					= dbgIndexub;
		qglIndexubv					= dbgIndexubv;
		qglInitNames				= dbgInitNames;
		qglInterleavedArrays		= dbgInterleavedArrays;
		qglIsEnabled				= dbgIsEnabled;
		qglIsList					= dbgIsList;
		qglIsTexture				= dbgIsTexture;
		qglLightModelf				= dbgLightModelf;
		qglLightModelfv				= dbgLightModelfv;
		qglLightModeli				= dbgLightModeli;
		qglLightModeliv				= dbgLightModeliv;
		qglLightf					= dbgLightf;
		qglLightfv					= dbgLightfv;
		qglLighti					= dbgLighti;
		qglLightiv					= dbgLightiv;
		qglLineStipple				= dbgLineStipple;
		qglLineWidth				= dbgLineWidth;
		qglListBase					= dbgListBase;
		qglLoadIdentity				= dbgLoadIdentity;
		qglLoadMatrixd				= dbgLoadMatrixd;
		qglLoadMatrixf				= dbgLoadMatrixf;
		qglLoadName					= dbgLoadName;
		qglLogicOp					= dbgLogicOp;
		qglMap1d					= dbgMap1d;
		qglMap1f					= dbgMap1f;
		qglMap2d					= dbgMap2d;
		qglMap2f					= dbgMap2f;
		qglMapGrid1d				= dbgMapGrid1d;
		qglMapGrid1f				= dbgMapGrid1f;
		qglMapGrid2d				= dbgMapGrid2d;
		qglMapGrid2f				= dbgMapGrid2f;
		qglMaterialf				= dbgMaterialf;
		qglMaterialfv				= dbgMaterialfv;
		qglMateriali				= dbgMateriali;
		qglMaterialiv				= dbgMaterialiv;
		qglMatrixMode				= dbgMatrixMode;
		qglMultMatrixd				= dbgMultMatrixd;
		qglMultMatrixf				= dbgMultMatrixf;
		qglNewList					= dbgNewList;
		qglNormal3b					= dbgNormal3b;
		qglNormal3bv				= dbgNormal3bv;
		qglNormal3d					= dbgNormal3d;
		qglNormal3dv				= dbgNormal3dv;
		qglNormal3f					= dbgNormal3f;
		qglNormal3fv				= dbgNormal3fv;
		qglNormal3i					= dbgNormal3i;
		qglNormal3iv				= dbgNormal3iv;
		qglNormal3s					= dbgNormal3s;
		qglNormal3sv				= dbgNormal3sv;
		qglNormalPointer			= dbgNormalPointer;
		qglOrtho					= dbgOrtho;
		qglPassThrough				= dbgPassThrough;
		qglPixelMapfv				= dbgPixelMapfv;
		qglPixelMapuiv				= dbgPixelMapuiv;
		qglPixelMapusv				= dbgPixelMapusv;
		qglPixelStoref				= dbgPixelStoref;
		qglPixelStorei				= dbgPixelStorei;
		qglPixelTransferf			= dbgPixelTransferf;
		qglPixelTransferi			= dbgPixelTransferi;
		qglPixelZoom				= dbgPixelZoom;
		qglPointSize				= dbgPointSize;
		qglPolygonMode				= dbgPolygonMode;
		qglPolygonOffset			= dbgPolygonOffset;
		qglPolygonStipple			= dbgPolygonStipple;
		qglPopAttrib				= dbgPopAttrib;
		qglPopClientAttrib			= dbgPopClientAttrib;
		qglPopMatrix				= dbgPopMatrix;
		qglPopName					= dbgPopName;
		qglPrioritizeTextures		= dbgPrioritizeTextures;
		qglPushAttrib				= dbgPushAttrib;
		qglPushClientAttrib			= dbgPushClientAttrib;
		qglPushMatrix				= dbgPushMatrix;
		qglPushName					= dbgPushName;
		qglRasterPos2d				= dbgRasterPos2d;
		qglRasterPos2dv				= dbgRasterPos2dv;
		qglRasterPos2f				= dbgRasterPos2f;
		qglRasterPos2fv				= dbgRasterPos2fv;
		qglRasterPos2i				= dbgRasterPos2i;
		qglRasterPos2iv				= dbgRasterPos2iv;
		qglRasterPos2s				= dbgRasterPos2s;
		qglRasterPos2sv				= dbgRasterPos2sv;
		qglRasterPos3d				= dbgRasterPos3d;
		qglRasterPos3dv				= dbgRasterPos3dv;
		qglRasterPos3f				= dbgRasterPos3f;
		qglRasterPos3fv				= dbgRasterPos3fv;
		qglRasterPos3i				= dbgRasterPos3i;
		qglRasterPos3iv				= dbgRasterPos3iv;
		qglRasterPos3s				= dbgRasterPos3s;
		qglRasterPos3sv				= dbgRasterPos3sv;
		qglRasterPos4d				= dbgRasterPos4d;
		qglRasterPos4dv				= dbgRasterPos4dv;
		qglRasterPos4f				= dbgRasterPos4f;
		qglRasterPos4fv				= dbgRasterPos4fv;
		qglRasterPos4i				= dbgRasterPos4i;
		qglRasterPos4iv				= dbgRasterPos4iv;
		qglRasterPos4s				= dbgRasterPos4s;
		qglRasterPos4sv				= dbgRasterPos4sv;
		qglReadBuffer				= dbgReadBuffer;
		qglReadPixels				= dbgReadPixels;
		qglRectd					= dbgRectd;
		qglRectdv					= dbgRectdv;
		qglRectf					= dbgRectf;
		qglRectfv					= dbgRectfv;
		qglRecti					= dbgRecti;
		qglRectiv					= dbgRectiv;
		qglRects					= dbgRects;
		qglRectsv					= dbgRectsv;
		qglRenderMode				= dbgRenderMode;
		qglRotated					= dbgRotated;
		qglRotatef					= dbgRotatef;
		qglScaled					= dbgScaled;
		qglScalef					= dbgScalef;
		qglScissor					= dbgScissor;
		qglSelectBuffer				= dbgSelectBuffer;
		qglShadeModel				= dbgShadeModel;
		qglStencilFunc				= dbgStencilFunc;
		qglStencilMask				= dbgStencilMask;
		qglStencilOp				= dbgStencilOp;
		qglTexCoord1d				= dbgTexCoord1d;
		qglTexCoord1dv				= dbgTexCoord1dv;
		qglTexCoord1f				= dbgTexCoord1f;
		qglTexCoord1fv				= dbgTexCoord1fv;
		qglTexCoord1i				= dbgTexCoord1i;
		qglTexCoord1iv				= dbgTexCoord1iv;
		qglTexCoord1s				= dbgTexCoord1s;
		qglTexCoord1sv				= dbgTexCoord1sv;
		qglTexCoord2d				= dbgTexCoord2d;
		qglTexCoord2dv				= dbgTexCoord2dv;
		qglTexCoord2f				= dbgTexCoord2f;
		qglTexCoord2fv				= dbgTexCoord2fv;
		qglTexCoord2i				= dbgTexCoord2i;
		qglTexCoord2iv				= dbgTexCoord2iv;
		qglTexCoord2s				= dbgTexCoord2s;
		qglTexCoord2sv				= dbgTexCoord2sv;
		qglTexCoord3d				= dbgTexCoord3d;
		qglTexCoord3dv				= dbgTexCoord3dv;
		qglTexCoord3f				= dbgTexCoord3f;
		qglTexCoord3fv				= dbgTexCoord3fv;
		qglTexCoord3i				= dbgTexCoord3i;
		qglTexCoord3iv				= dbgTexCoord3iv;
		qglTexCoord3s				= dbgTexCoord3s;
		qglTexCoord3sv				= dbgTexCoord3sv;
		qglTexCoord4d				= dbgTexCoord4d;
		qglTexCoord4dv				= dbgTexCoord4dv;
		qglTexCoord4f				= dbgTexCoord4f;
		qglTexCoord4fv				= dbgTexCoord4fv;
		qglTexCoord4i				= dbgTexCoord4i;
		qglTexCoord4iv				= dbgTexCoord4iv;
		qglTexCoord4s				= dbgTexCoord4s;
		qglTexCoord4sv				= dbgTexCoord4sv;
		qglTexCoordPointer			= dbgTexCoordPointer;
		qglTexEnvf					= dbgTexEnvf;
		qglTexEnvfv					= dbgTexEnvfv;
		qglTexEnvi					= dbgTexEnvi;
		qglTexEnviv					= dbgTexEnviv;
		qglTexGend					= dbgTexGend;
		qglTexGendv					= dbgTexGendv;
		qglTexGenf					= dbgTexGenf;
		qglTexGenfv					= dbgTexGenfv;
		qglTexGeni					= dbgTexGeni;
		qglTexGeniv					= dbgTexGeniv;
		qglTexImage1D				= dbgTexImage1D;
		qglTexImage2D				= dbgTexImage2D;
		qglTexParameterf			= dbgTexParameterf;
		qglTexParameterfv			= dbgTexParameterfv;
		qglTexParameteri			= dbgTexParameteri;
		qglTexParameteriv			= dbgTexParameteriv;
		qglTexSubImage1D			= dbgTexSubImage1D;
		qglTexSubImage2D			= dbgTexSubImage2D;
		qglTranslated				= dbgTranslated;
		qglTranslatef				= dbgTranslatef;
		qglVertex2d					= dbgVertex2d;
		qglVertex2dv				= dbgVertex2dv;
		qglVertex2f					= dbgVertex2f;
		qglVertex2fv				= dbgVertex2fv;
		qglVertex2i					= dbgVertex2i;
		qglVertex2iv				= dbgVertex2iv;
		qglVertex2s					= dbgVertex2s;
		qglVertex2sv				= dbgVertex2sv;
		qglVertex3d					= dbgVertex3d;
		qglVertex3dv				= dbgVertex3dv;
		qglVertex3f					= dbgVertex3f;
		qglVertex3fv				= dbgVertex3fv;
		qglVertex3i					= dbgVertex3i;
		qglVertex3iv				= dbgVertex3iv;
		qglVertex3s					= dbgVertex3s;
		qglVertex3sv				= dbgVertex3sv;
		qglVertex4d					= dbgVertex4d;
		qglVertex4dv				= dbgVertex4dv;
		qglVertex4f					= dbgVertex4f;
		qglVertex4fv				= dbgVertex4fv;
		qglVertex4i					= dbgVertex4i;
		qglVertex4iv				= dbgVertex4iv;
		qglVertex4s					= dbgVertex4s;
		qglVertex4sv				= dbgVertex4sv;
		qglVertexPointer			= dbgVertexPointer;
		qglViewport					= dbgViewport;
	}
	else {
		qglAccum					= dllAccum;
		qglAlphaFunc				= dllAlphaFunc;
		qglAreTexturesResident		= dllAreTexturesResident;
		qglArrayElement				= dllArrayElement;
		qglBegin					= dllBegin;
		qglBindTexture				= dllBindTexture;
		qglBitmap					= dllBitmap;
		qglBlendFunc				= dllBlendFunc;
		qglCallList					= dllCallList;
		qglCallLists				= dllCallLists;
		qglClear					= dllClear;
		qglClearAccum				= dllClearAccum;
		qglClearColor				= dllClearColor;
		qglClearDepth				= dllClearDepth;
		qglClearIndex				= dllClearIndex;
		qglClearStencil				= dllClearStencil;
		qglClipPlane				= dllClipPlane;
		qglColor3b					= dllColor3b;
		qglColor3bv					= dllColor3bv;
		qglColor3d					= dllColor3d;
		qglColor3dv					= dllColor3dv;
		qglColor3f					= dllColor3f;
		qglColor3fv					= dllColor3fv;
		qglColor3i					= dllColor3i;
		qglColor3iv					= dllColor3iv;
		qglColor3s					= dllColor3s;
		qglColor3sv					= dllColor3sv;
		qglColor3ub					= dllColor3ub;
		qglColor3ubv				= dllColor3ubv;
		qglColor3ui					= dllColor3ui;
		qglColor3uiv				= dllColor3uiv;
		qglColor3us					= dllColor3us;
		qglColor3usv				= dllColor3usv;
		qglColor4b					= dllColor4b;
		qglColor4bv					= dllColor4bv;
		qglColor4d					= dllColor4d;
		qglColor4dv					= dllColor4dv;
		qglColor4f					= dllColor4f;
		qglColor4fv					= dllColor4fv;
		qglColor4i					= dllColor4i;
		qglColor4iv					= dllColor4iv;
		qglColor4s					= dllColor4s;
		qglColor4sv					= dllColor4sv;
		qglColor4ub					= dllColor4ub;
		qglColor4ubv				= dllColor4ubv;
		qglColor4ui					= dllColor4ui;
		qglColor4uiv				= dllColor4uiv;
		qglColor4us					= dllColor4us;
		qglColor4usv				= dllColor4usv;
		qglColorMask				= dllColorMask;
		qglColorMaterial			= dllColorMaterial;
		qglColorPointer				= dllColorPointer;
		qglCopyPixels				= dllCopyPixels;
		qglCopyTexImage1D			= dllCopyTexImage1D;
		qglCopyTexImage2D			= dllCopyTexImage2D;
		qglCopyTexSubImage1D		= dllCopyTexSubImage1D;
		qglCopyTexSubImage2D		= dllCopyTexSubImage2D;
		qglCullFace					= dllCullFace;
		qglDeleteLists				= dllDeleteLists;
		qglDeleteTextures			= dllDeleteTextures;
		qglDepthFunc				= dllDepthFunc;
		qglDepthMask				= dllDepthMask;
		qglDepthRange				= dllDepthRange;
		qglDisable					= dllDisable;
		qglDisableClientState		= dllDisableClientState;
		qglDrawArrays				= dllDrawArrays;
		qglDrawBuffer				= dllDrawBuffer;
		qglDrawElements				= dllDrawElements;
		qglDrawPixels				= dllDrawPixels;
		qglEdgeFlag					= dllEdgeFlag;
		qglEdgeFlagPointer			= dllEdgeFlagPointer;
		qglEdgeFlagv				= dllEdgeFlagv;
		qglEnable					= dllEnable;
		qglEnableClientState		= dllEnableClientState;
		qglEnd						= dllEnd;
		qglEndList					= dllEndList;
		qglEvalCoord1d				= dllEvalCoord1d;
		qglEvalCoord1dv				= dllEvalCoord1dv;
		qglEvalCoord1f				= dllEvalCoord1f;
		qglEvalCoord1fv				= dllEvalCoord1fv;
		qglEvalCoord2d				= dllEvalCoord2d;
		qglEvalCoord2dv				= dllEvalCoord2dv;
		qglEvalCoord2f				= dllEvalCoord2f;
		qglEvalCoord2fv				= dllEvalCoord2fv;
		qglEvalMesh1				= dllEvalMesh1;
		qglEvalMesh2				= dllEvalMesh2;
		qglEvalPoint1				= dllEvalPoint1;
		qglEvalPoint2				= dllEvalPoint2;
		qglFeedbackBuffer			= dllFeedbackBuffer;
		qglFinish					= dllFinish;
		qglFlush					= dllFlush;
		qglFogf						= dllFogf;
		qglFogfv					= dllFogfv;
		qglFogi						= dllFogi;
		qglFogiv					= dllFogiv;
		qglFrontFace				= dllFrontFace;
		qglFrustum					= dllFrustum;
		qglGenLists					= dllGenLists;
		qglGenTextures				= dllGenTextures;
		qglGetBooleanv				= dllGetBooleanv;
		qglGetClipPlane				= dllGetClipPlane;
		qglGetDoublev				= dllGetDoublev;
		qglGetError					= dllGetError;
		qglGetFloatv				= dllGetFloatv;
		qglGetIntegerv				= dllGetIntegerv;
		qglGetLightfv				= dllGetLightfv;
		qglGetLightiv				= dllGetLightiv;
		qglGetMapdv					= dllGetMapdv;
		qglGetMapfv					= dllGetMapfv;
		qglGetMapiv					= dllGetMapiv;
		qglGetMaterialfv			= dllGetMaterialfv;
		qglGetMaterialiv			= dllGetMaterialiv;
		qglGetPixelMapfv			= dllGetPixelMapfv;
		qglGetPixelMapuiv			= dllGetPixelMapuiv;
		qglGetPixelMapusv			= dllGetPixelMapusv;
		qglGetPointerv				= dllGetPointerv;
		qglGetPolygonStipple		= dllGetPolygonStipple;
		qglGetString				= dllGetString;
		qglGetTexEnvfv				= dllGetTexEnvfv;
		qglGetTexEnviv				= dllGetTexEnviv;
		qglGetTexGendv				= dllGetTexGendv;
		qglGetTexGenfv				= dllGetTexGenfv;
		qglGetTexGeniv				= dllGetTexGeniv;
		qglGetTexImage				= dllGetTexImage;
		qglGetTexLevelParameterfv	= dllGetTexLevelParameterfv;
		qglGetTexLevelParameteriv	= dllGetTexLevelParameteriv;
		qglGetTexParameterfv		= dllGetTexParameterfv;
		qglGetTexParameteriv		= dllGetTexParameteriv;
		qglHint						= dllHint;
		qglIndexMask				= dllIndexMask;
		qglIndexPointer				= dllIndexPointer;
		qglIndexd					= dllIndexd;
		qglIndexdv					= dllIndexdv;
		qglIndexf					= dllIndexf;
		qglIndexfv					= dllIndexfv;
		qglIndexi					= dllIndexi;
		qglIndexiv					= dllIndexiv;
		qglIndexs					= dllIndexs;
		qglIndexsv					= dllIndexsv;
		qglIndexub					= dllIndexub;
		qglIndexubv					= dllIndexubv;
		qglInitNames				= dllInitNames;
		qglInterleavedArrays		= dllInterleavedArrays;
		qglIsEnabled				= dllIsEnabled;
		qglIsList					= dllIsList;
		qglIsTexture				= dllIsTexture;
		qglLightModelf				= dllLightModelf;
		qglLightModelfv				= dllLightModelfv;
		qglLightModeli				= dllLightModeli;
		qglLightModeliv				= dllLightModeliv;
		qglLightf					= dllLightf;
		qglLightfv					= dllLightfv;
		qglLighti					= dllLighti;
		qglLightiv					= dllLightiv;
		qglLineStipple				= dllLineStipple;
		qglLineWidth				= dllLineWidth;
		qglListBase					= dllListBase;
		qglLoadIdentity				= dllLoadIdentity;
		qglLoadMatrixd				= dllLoadMatrixd;
		qglLoadMatrixf				= dllLoadMatrixf;
		qglLoadName					= dllLoadName;
		qglLogicOp					= dllLogicOp;
		qglMap1d					= dllMap1d;
		qglMap1f					= dllMap1f;
		qglMap2d					= dllMap2d;
		qglMap2f					= dllMap2f;
		qglMapGrid1d				= dllMapGrid1d;
		qglMapGrid1f				= dllMapGrid1f;
		qglMapGrid2d				= dllMapGrid2d;
		qglMapGrid2f				= dllMapGrid2f;
		qglMaterialf				= dllMaterialf;
		qglMaterialfv				= dllMaterialfv;
		qglMateriali				= dllMateriali;
		qglMaterialiv				= dllMaterialiv;
		qglMatrixMode				= dllMatrixMode;
		qglMultMatrixd				= dllMultMatrixd;
		qglMultMatrixf				= dllMultMatrixf;
		qglNewList					= dllNewList;
		qglNormal3b					= dllNormal3b;
		qglNormal3bv				= dllNormal3bv;
		qglNormal3d					= dllNormal3d;
		qglNormal3dv				= dllNormal3dv;
		qglNormal3f					= dllNormal3f;
		qglNormal3fv				= dllNormal3fv;
		qglNormal3i					= dllNormal3i;
		qglNormal3iv				= dllNormal3iv;
		qglNormal3s					= dllNormal3s;
		qglNormal3sv				= dllNormal3sv;
		qglNormalPointer			= dllNormalPointer;
		qglOrtho					= dllOrtho;
		qglPassThrough				= dllPassThrough;
		qglPixelMapfv				= dllPixelMapfv;
		qglPixelMapuiv				= dllPixelMapuiv;
		qglPixelMapusv				= dllPixelMapusv;
		qglPixelStoref				= dllPixelStoref;
		qglPixelStorei				= dllPixelStorei;
		qglPixelTransferf			= dllPixelTransferf;
		qglPixelTransferi			= dllPixelTransferi;
		qglPixelZoom				= dllPixelZoom;
		qglPointSize				= dllPointSize;
		qglPolygonMode				= dllPolygonMode;
		qglPolygonOffset			= dllPolygonOffset;
		qglPolygonStipple			= dllPolygonStipple;
		qglPopAttrib				= dllPopAttrib;
		qglPopClientAttrib			= dllPopClientAttrib;
		qglPopMatrix				= dllPopMatrix;
		qglPopName					= dllPopName;
		qglPrioritizeTextures		= dllPrioritizeTextures;
		qglPushAttrib				= dllPushAttrib;
		qglPushClientAttrib			= dllPushClientAttrib;
		qglPushMatrix				= dllPushMatrix;
		qglPushName					= dllPushName;
		qglRasterPos2d				= dllRasterPos2d;
		qglRasterPos2dv				= dllRasterPos2dv;
		qglRasterPos2f				= dllRasterPos2f;
		qglRasterPos2fv				= dllRasterPos2fv;
		qglRasterPos2i				= dllRasterPos2i;
		qglRasterPos2iv				= dllRasterPos2iv;
		qglRasterPos2s				= dllRasterPos2s;
		qglRasterPos2sv				= dllRasterPos2sv;
		qglRasterPos3d				= dllRasterPos3d;
		qglRasterPos3dv				= dllRasterPos3dv;
		qglRasterPos3f				= dllRasterPos3f;
		qglRasterPos3fv				= dllRasterPos3fv;
		qglRasterPos3i				= dllRasterPos3i;
		qglRasterPos3iv				= dllRasterPos3iv;
		qglRasterPos3s				= dllRasterPos3s;
		qglRasterPos3sv				= dllRasterPos3sv;
		qglRasterPos4d				= dllRasterPos4d;
		qglRasterPos4dv				= dllRasterPos4dv;
		qglRasterPos4f				= dllRasterPos4f;
		qglRasterPos4fv				= dllRasterPos4fv;
		qglRasterPos4i				= dllRasterPos4i;
		qglRasterPos4iv				= dllRasterPos4iv;
		qglRasterPos4s				= dllRasterPos4s;
		qglRasterPos4sv				= dllRasterPos4sv;
		qglReadBuffer				= dllReadBuffer;
		qglReadPixels				= dllReadPixels;
		qglRectd					= dllRectd;
		qglRectdv					= dllRectdv;
		qglRectf					= dllRectf;
		qglRectfv					= dllRectfv;
		qglRecti					= dllRecti;
		qglRectiv					= dllRectiv;
		qglRects					= dllRects;
		qglRectsv					= dllRectsv;
		qglRenderMode				= dllRenderMode;
		qglRotated					= dllRotated;
		qglRotatef					= dllRotatef;
		qglScaled					= dllScaled;
		qglScalef					= dllScalef;
		qglScissor					= dllScissor;
		qglSelectBuffer				= dllSelectBuffer;
		qglShadeModel				= dllShadeModel;
		qglStencilFunc				= dllStencilFunc;
		qglStencilMask				= dllStencilMask;
		qglStencilOp				= dllStencilOp;
		qglTexCoord1d				= dllTexCoord1d;
		qglTexCoord1dv				= dllTexCoord1dv;
		qglTexCoord1f				= dllTexCoord1f;
		qglTexCoord1fv				= dllTexCoord1fv;
		qglTexCoord1i				= dllTexCoord1i;
		qglTexCoord1iv				= dllTexCoord1iv;
		qglTexCoord1s				= dllTexCoord1s;
		qglTexCoord1sv				= dllTexCoord1sv;
		qglTexCoord2d				= dllTexCoord2d;
		qglTexCoord2dv				= dllTexCoord2dv;
		qglTexCoord2f				= dllTexCoord2f;
		qglTexCoord2fv				= dllTexCoord2fv;
		qglTexCoord2i				= dllTexCoord2i;
		qglTexCoord2iv				= dllTexCoord2iv;
		qglTexCoord2s				= dllTexCoord2s;
		qglTexCoord2sv				= dllTexCoord2sv;
		qglTexCoord3d				= dllTexCoord3d;
		qglTexCoord3dv				= dllTexCoord3dv;
		qglTexCoord3f				= dllTexCoord3f;
		qglTexCoord3fv				= dllTexCoord3fv;
		qglTexCoord3i				= dllTexCoord3i;
		qglTexCoord3iv				= dllTexCoord3iv;
		qglTexCoord3s				= dllTexCoord3s;
		qglTexCoord3sv				= dllTexCoord3sv;
		qglTexCoord4d				= dllTexCoord4d;
		qglTexCoord4dv				= dllTexCoord4dv;
		qglTexCoord4f				= dllTexCoord4f;
		qglTexCoord4fv				= dllTexCoord4fv;
		qglTexCoord4i				= dllTexCoord4i;
		qglTexCoord4iv				= dllTexCoord4iv;
		qglTexCoord4s				= dllTexCoord4s;
		qglTexCoord4sv				= dllTexCoord4sv;
		qglTexCoordPointer			= dllTexCoordPointer;
		qglTexEnvf					= dllTexEnvf;
		qglTexEnvfv					= dllTexEnvfv;
		qglTexEnvi					= dllTexEnvi;
		qglTexEnviv					= dllTexEnviv;
		qglTexGend					= dllTexGend;
		qglTexGendv					= dllTexGendv;
		qglTexGenf					= dllTexGenf;
		qglTexGenfv					= dllTexGenfv;
		qglTexGeni					= dllTexGeni;
		qglTexGeniv					= dllTexGeniv;
		qglTexImage1D				= dllTexImage1D;
		qglTexImage2D				= dllTexImage2D;
		qglTexParameterf			= dllTexParameterf;
		qglTexParameterfv			= dllTexParameterfv;
		qglTexParameteri			= dllTexParameteri;
		qglTexParameteriv			= dllTexParameteriv;
		qglTexSubImage1D			= dllTexSubImage1D;
		qglTexSubImage2D			= dllTexSubImage2D;
		qglTranslated				= dllTranslated;
		qglTranslatef				= dllTranslatef;
		qglVertex2d					= dllVertex2d;
		qglVertex2dv				= dllVertex2dv;
		qglVertex2f					= dllVertex2f;
		qglVertex2fv				= dllVertex2fv;
		qglVertex2i					= dllVertex2i;
		qglVertex2iv				= dllVertex2iv;
		qglVertex2s					= dllVertex2s;
		qglVertex2sv				= dllVertex2sv;
		qglVertex3d					= dllVertex3d;
		qglVertex3dv				= dllVertex3dv;
		qglVertex3f					= dllVertex3f;
		qglVertex3fv				= dllVertex3fv;
		qglVertex3i					= dllVertex3i;
		qglVertex3iv				= dllVertex3iv;
		qglVertex3s					= dllVertex3s;
		qglVertex3sv				= dllVertex3sv;
		qglVertex4d					= dllVertex4d;
		qglVertex4dv				= dllVertex4dv;
		qglVertex4f					= dllVertex4f;
		qglVertex4fv				= dllVertex4fv;
		qglVertex4i					= dllVertex4i;
		qglVertex4iv				= dllVertex4iv;
		qglVertex4s					= dllVertex4s;
		qglVertex4sv				= dllVertex4sv;
		qglVertexPointer			= dllVertexPointer;
		qglViewport					= dllViewport;
	}
}


/*
===========
QGL_ToggleLogging
===========
*/
void QGL_ToggleLogging (void)
{	
	if (gl_log->intVal) {
		if (!LOGPROC) {
			struct tm	*newtime;
			time_t		aclock;
			char		buffer[1024];

			time (&aclock);
			newtime = localtime (&aclock);

			asctime (newtime);

			Q_snprintfz (buffer, sizeof (buffer), "%s/egl_opengl.log", FS_Gamedir ()); 
			LOGPROC = fopen (buffer, "wt");

			fprintf (LOGPROC, "%s\n", asctime (newtime));
		}

		qglAccum					= logAccum;
		qglAlphaFunc				= logAlphaFunc;
		qglAreTexturesResident		= logAreTexturesResident;
		qglArrayElement				= logArrayElement;
		qglBegin					= logBegin;
		qglBindTexture				= logBindTexture;
		qglBitmap					= logBitmap;
		qglBlendFunc				= logBlendFunc;
		qglCallList					= logCallList;
		qglCallLists				= logCallLists;
		qglClear					= logClear;
		qglClearAccum				= logClearAccum;
		qglClearColor				= logClearColor;
		qglClearDepth				= logClearDepth;
		qglClearIndex				= logClearIndex;
		qglClearStencil				= logClearStencil;
		qglClipPlane				= logClipPlane;
		qglColor3b					= logColor3b;
		qglColor3bv					= logColor3bv;
		qglColor3d					= logColor3d;
		qglColor3dv					= logColor3dv;
		qglColor3f					= logColor3f;
		qglColor3fv					= logColor3fv;
		qglColor3i					= logColor3i;
		qglColor3iv					= logColor3iv;
		qglColor3s					= logColor3s;
		qglColor3sv					= logColor3sv;
		qglColor3ub					= logColor3ub;
		qglColor3ubv				= logColor3ubv;
		qglColor3ui					= logColor3ui;
		qglColor3uiv				= logColor3uiv;
		qglColor3us					= logColor3us;
		qglColor3usv				= logColor3usv;
		qglColor4b					= logColor4b;
		qglColor4bv					= logColor4bv;
		qglColor4d					= logColor4d;
		qglColor4dv					= logColor4dv;
		qglColor4f					= logColor4f;
		qglColor4fv					= logColor4fv;
		qglColor4i					= logColor4i;
		qglColor4iv					= logColor4iv;
		qglColor4s					= logColor4s;
		qglColor4sv					= logColor4sv;
		qglColor4ub					= logColor4ub;
		qglColor4ubv				= logColor4ubv;
		qglColor4ui					= logColor4ui;
		qglColor4uiv				= logColor4uiv;
		qglColor4us					= logColor4us;
		qglColor4usv				= logColor4usv;
		qglColorMask				= logColorMask;
		qglColorMaterial			= logColorMaterial;
		qglColorPointer				= logColorPointer;
		qglCopyPixels				= logCopyPixels;
		qglCopyTexImage1D			= logCopyTexImage1D;
		qglCopyTexImage2D			= logCopyTexImage2D;
		qglCopyTexSubImage1D		= logCopyTexSubImage1D;
		qglCopyTexSubImage2D		= logCopyTexSubImage2D;
		qglCullFace					= logCullFace;
		qglDeleteLists				= logDeleteLists;
		qglDeleteTextures			= logDeleteTextures;
		qglDepthFunc				= logDepthFunc;
		qglDepthMask				= logDepthMask;
		qglDepthRange				= logDepthRange;
		qglDisable					= logDisable;
		qglDisableClientState		= logDisableClientState;
		qglDrawArrays				= logDrawArrays;
		qglDrawBuffer				= logDrawBuffer;
		qglDrawElements				= logDrawElements;
		qglDrawPixels				= logDrawPixels;
		qglEdgeFlag					= logEdgeFlag;
		qglEdgeFlagPointer			= logEdgeFlagPointer;
		qglEdgeFlagv				= logEdgeFlagv;
		qglEnable					= logEnable;
		qglEnableClientState		= logEnableClientState;
		qglEnd						= logEnd;
		qglEndList					= logEndList;
		qglEvalCoord1d				= logEvalCoord1d;
		qglEvalCoord1dv				= logEvalCoord1dv;
		qglEvalCoord1f				= logEvalCoord1f;
		qglEvalCoord1fv				= logEvalCoord1fv;
		qglEvalCoord2d				= logEvalCoord2d;
		qglEvalCoord2dv				= logEvalCoord2dv;
		qglEvalCoord2f				= logEvalCoord2f;
		qglEvalCoord2fv				= logEvalCoord2fv;
		qglEvalMesh1				= logEvalMesh1;
		qglEvalMesh2				= logEvalMesh2;
		qglEvalPoint1				= logEvalPoint1;
		qglEvalPoint2				= logEvalPoint2;
		qglFeedbackBuffer			= logFeedbackBuffer;
		qglFinish					= logFinish;
		qglFlush					= logFlush;
		qglFogf						= logFogf;
		qglFogfv					= logFogfv;
		qglFogi						= logFogi;
		qglFogiv					= logFogiv;
		qglFrontFace				= logFrontFace;
		qglFrustum					= logFrustum;
		qglGenLists					= logGenLists;
		qglGenTextures				= logGenTextures;
		qglGetBooleanv				= logGetBooleanv;
		qglGetClipPlane				= logGetClipPlane;
		qglGetDoublev				= logGetDoublev;
		qglGetError					= logGetError;
		qglGetFloatv				= logGetFloatv;
		qglGetIntegerv				= logGetIntegerv;
		qglGetLightfv				= logGetLightfv;
		qglGetLightiv				= logGetLightiv;
		qglGetMapdv					= logGetMapdv;
		qglGetMapfv					= logGetMapfv;
		qglGetMapiv					= logGetMapiv;
		qglGetMaterialfv			= logGetMaterialfv;
		qglGetMaterialiv			= logGetMaterialiv;
		qglGetPixelMapfv			= logGetPixelMapfv;
		qglGetPixelMapuiv			= logGetPixelMapuiv;
		qglGetPixelMapusv			= logGetPixelMapusv;
		qglGetPointerv				= logGetPointerv;
		qglGetPolygonStipple		= logGetPolygonStipple;
		qglGetString				= logGetString;
		qglGetTexEnvfv				= logGetTexEnvfv;
		qglGetTexEnviv				= logGetTexEnviv;
		qglGetTexGendv				= logGetTexGendv;
		qglGetTexGenfv				= logGetTexGenfv;
		qglGetTexGeniv				= logGetTexGeniv;
		qglGetTexImage				= logGetTexImage;
		qglGetTexLevelParameterfv	= logGetTexLevelParameterfv;
		qglGetTexLevelParameteriv	= logGetTexLevelParameteriv;
		qglGetTexParameterfv		= logGetTexParameterfv;
		qglGetTexParameteriv		= logGetTexParameteriv;
		qglHint						= logHint;
		qglIndexMask				= logIndexMask;
		qglIndexPointer				= logIndexPointer;
		qglIndexd					= logIndexd;
		qglIndexdv					= logIndexdv;
		qglIndexf					= logIndexf;
		qglIndexfv					= logIndexfv;
		qglIndexi					= logIndexi;
		qglIndexiv					= logIndexiv;
		qglIndexs					= logIndexs;
		qglIndexsv					= logIndexsv;
		qglIndexub					= logIndexub;
		qglIndexubv					= logIndexubv;
		qglInitNames				= logInitNames;
		qglInterleavedArrays		= logInterleavedArrays;
		qglIsEnabled				= logIsEnabled;
		qglIsList					= logIsList;
		qglIsTexture				= logIsTexture;
		qglLightModelf				= logLightModelf;
		qglLightModelfv				= logLightModelfv;
		qglLightModeli				= logLightModeli;
		qglLightModeliv				= logLightModeliv;
		qglLightf					= logLightf;
		qglLightfv					= logLightfv;
		qglLighti					= logLighti;
		qglLightiv					= logLightiv;
		qglLineStipple				= logLineStipple;
		qglLineWidth				= logLineWidth;
		qglListBase					= logListBase;
		qglLoadIdentity				= logLoadIdentity;
		qglLoadMatrixd				= logLoadMatrixd;
		qglLoadMatrixf				= logLoadMatrixf;
		qglLoadName					= logLoadName;
		qglLogicOp					= logLogicOp;
		qglMap1d					= logMap1d;
		qglMap1f					= logMap1f;
		qglMap2d					= logMap2d;
		qglMap2f					= logMap2f;
		qglMapGrid1d				= logMapGrid1d;
		qglMapGrid1f				= logMapGrid1f;
		qglMapGrid2d				= logMapGrid2d;
		qglMapGrid2f				= logMapGrid2f;
		qglMaterialf				= logMaterialf;
		qglMaterialfv				= logMaterialfv;
		qglMateriali				= logMateriali;
		qglMaterialiv				= logMaterialiv;
		qglMatrixMode				= logMatrixMode;
		qglMultMatrixd				= logMultMatrixd;
		qglMultMatrixf				= logMultMatrixf;
		qglNewList					= logNewList;
		qglNormal3b					= logNormal3b;
		qglNormal3bv				= logNormal3bv;
		qglNormal3d					= logNormal3d;
		qglNormal3dv				= logNormal3dv;
		qglNormal3f					= logNormal3f;
		qglNormal3fv				= logNormal3fv;
		qglNormal3i					= logNormal3i;
		qglNormal3iv				= logNormal3iv;
		qglNormal3s					= logNormal3s;
		qglNormal3sv				= logNormal3sv;
		qglNormalPointer			= logNormalPointer;
		qglOrtho					= logOrtho;
		qglPassThrough				= logPassThrough;
		qglPixelMapfv				= logPixelMapfv;
		qglPixelMapuiv				= logPixelMapuiv;
		qglPixelMapusv				= logPixelMapusv;
		qglPixelStoref				= logPixelStoref;
		qglPixelStorei				= logPixelStorei;
		qglPixelTransferf			= logPixelTransferf;
		qglPixelTransferi			= logPixelTransferi;
		qglPixelZoom				= logPixelZoom;
		qglPointSize				= logPointSize;
		qglPolygonMode				= logPolygonMode;
		qglPolygonOffset			= logPolygonOffset;
		qglPolygonStipple			= logPolygonStipple;
		qglPopAttrib				= logPopAttrib;
		qglPopClientAttrib			= logPopClientAttrib;
		qglPopMatrix				= logPopMatrix;
		qglPopName					= logPopName;
		qglPrioritizeTextures		= logPrioritizeTextures;
		qglPushAttrib				= logPushAttrib;
		qglPushClientAttrib			= logPushClientAttrib;
		qglPushMatrix				= logPushMatrix;
		qglPushName					= logPushName;
		qglRasterPos2d				= logRasterPos2d;
		qglRasterPos2dv				= logRasterPos2dv;
		qglRasterPos2f				= logRasterPos2f;
		qglRasterPos2fv				= logRasterPos2fv;
		qglRasterPos2i				= logRasterPos2i;
		qglRasterPos2iv				= logRasterPos2iv;
		qglRasterPos2s				= logRasterPos2s;
		qglRasterPos2sv				= logRasterPos2sv;
		qglRasterPos3d				= logRasterPos3d;
		qglRasterPos3dv				= logRasterPos3dv;
		qglRasterPos3f				= logRasterPos3f;
		qglRasterPos3fv				= logRasterPos3fv;
		qglRasterPos3i				= logRasterPos3i;
		qglRasterPos3iv				= logRasterPos3iv;
		qglRasterPos3s				= logRasterPos3s;
		qglRasterPos3sv				= logRasterPos3sv;
		qglRasterPos4d				= logRasterPos4d;
		qglRasterPos4dv				= logRasterPos4dv;
		qglRasterPos4f				= logRasterPos4f;
		qglRasterPos4fv				= logRasterPos4fv;
		qglRasterPos4i				= logRasterPos4i;
		qglRasterPos4iv				= logRasterPos4iv;
		qglRasterPos4s				= logRasterPos4s;
		qglRasterPos4sv				= logRasterPos4sv;
		qglReadBuffer				= logReadBuffer;
		qglReadPixels				= logReadPixels;
		qglRectd					= logRectd;
		qglRectdv					= logRectdv;
		qglRectf					= logRectf;
		qglRectfv					= logRectfv;
		qglRecti					= logRecti;
		qglRectiv					= logRectiv;
		qglRects					= logRects;
		qglRectsv					= logRectsv;
		qglRenderMode				= logRenderMode;
		qglRotated					= logRotated;
		qglRotatef					= logRotatef;
		qglScaled					= logScaled;
		qglScalef					= logScalef;
		qglScissor					= logScissor;
		qglSelectBuffer				= logSelectBuffer;
		qglShadeModel				= logShadeModel;
		qglStencilFunc				= logStencilFunc;
		qglStencilMask				= logStencilMask;
		qglStencilOp				= logStencilOp;
		qglTexCoord1d				= logTexCoord1d;
		qglTexCoord1dv				= logTexCoord1dv;
		qglTexCoord1f				= logTexCoord1f;
		qglTexCoord1fv				= logTexCoord1fv;
		qglTexCoord1i				= logTexCoord1i;
		qglTexCoord1iv				= logTexCoord1iv;
		qglTexCoord1s				= logTexCoord1s;
		qglTexCoord1sv				= logTexCoord1sv;
		qglTexCoord2d				= logTexCoord2d;
		qglTexCoord2dv				= logTexCoord2dv;
		qglTexCoord2f				= logTexCoord2f;
		qglTexCoord2fv				= logTexCoord2fv;
		qglTexCoord2i				= logTexCoord2i;
		qglTexCoord2iv				= logTexCoord2iv;
		qglTexCoord2s				= logTexCoord2s;
		qglTexCoord2sv				= logTexCoord2sv;
		qglTexCoord3d				= logTexCoord3d;
		qglTexCoord3dv				= logTexCoord3dv;
		qglTexCoord3f				= logTexCoord3f;
		qglTexCoord3fv				= logTexCoord3fv;
		qglTexCoord3i				= logTexCoord3i;
		qglTexCoord3iv				= logTexCoord3iv;
		qglTexCoord3s				= logTexCoord3s;
		qglTexCoord3sv				= logTexCoord3sv;
		qglTexCoord4d				= logTexCoord4d;
		qglTexCoord4dv				= logTexCoord4dv;
		qglTexCoord4f				= logTexCoord4f;
		qglTexCoord4fv				= logTexCoord4fv;
		qglTexCoord4i				= logTexCoord4i;
		qglTexCoord4iv				= logTexCoord4iv;
		qglTexCoord4s				= logTexCoord4s;
		qglTexCoord4sv				= logTexCoord4sv;
		qglTexCoordPointer			= logTexCoordPointer;
		qglTexEnvf					= logTexEnvf;
		qglTexEnvfv					= logTexEnvfv;
		qglTexEnvi					= logTexEnvi;
		qglTexEnviv					= logTexEnviv;
		qglTexGend					= logTexGend;
		qglTexGendv					= logTexGendv;
		qglTexGenf					= logTexGenf;
		qglTexGenfv					= logTexGenfv;
		qglTexGeni					= logTexGeni;
		qglTexGeniv					= logTexGeniv;
		qglTexImage1D				= logTexImage1D;
		qglTexImage2D				= logTexImage2D;
		qglTexParameterf			= logTexParameterf;
		qglTexParameterfv			= logTexParameterfv;
		qglTexParameteri			= logTexParameteri;
		qglTexParameteriv			= logTexParameteriv;
		qglTexSubImage1D			= logTexSubImage1D;
		qglTexSubImage2D			= logTexSubImage2D;
		qglTranslated				= logTranslated;
		qglTranslatef				= logTranslatef;
		qglVertex2d					= logVertex2d;
		qglVertex2dv				= logVertex2dv;
		qglVertex2f					= logVertex2f;
		qglVertex2fv				= logVertex2fv;
		qglVertex2i					= logVertex2i;
		qglVertex2iv				= logVertex2iv;
		qglVertex2s					= logVertex2s;
		qglVertex2sv				= logVertex2sv;
		qglVertex3d					= logVertex3d;
		qglVertex3dv				= logVertex3dv;
		qglVertex3f					= logVertex3f;
		qglVertex3fv				= logVertex3fv;
		qglVertex3i					= logVertex3i;
		qglVertex3iv				= logVertex3iv;
		qglVertex3s					= logVertex3s;
		qglVertex3sv				= logVertex3sv;
		qglVertex4d					= logVertex4d;
		qglVertex4dv				= logVertex4dv;
		qglVertex4f					= logVertex4f;
		qglVertex4fv				= logVertex4fv;
		qglVertex4i					= logVertex4i;
		qglVertex4iv				= logVertex4iv;
		qglVertex4s					= logVertex4s;
		qglVertex4sv				= logVertex4sv;
		qglVertexPointer			= logVertexPointer;
		qglViewport					= logViewport;
	}
	else {
		qglAccum					= dllAccum;
		qglAlphaFunc				= dllAlphaFunc;
		qglAreTexturesResident		= dllAreTexturesResident;
		qglArrayElement				= dllArrayElement;
		qglBegin					= dllBegin;
		qglBindTexture				= dllBindTexture;
		qglBitmap					= dllBitmap;
		qglBlendFunc				= dllBlendFunc;
		qglCallList					= dllCallList;
		qglCallLists				= dllCallLists;
		qglClear					= dllClear;
		qglClearAccum				= dllClearAccum;
		qglClearColor				= dllClearColor;
		qglClearDepth				= dllClearDepth;
		qglClearIndex				= dllClearIndex;
		qglClearStencil				= dllClearStencil;
		qglClipPlane				= dllClipPlane;
		qglColor3b					= dllColor3b;
		qglColor3bv					= dllColor3bv;
		qglColor3d					= dllColor3d;
		qglColor3dv					= dllColor3dv;
		qglColor3f					= dllColor3f;
		qglColor3fv					= dllColor3fv;
		qglColor3i					= dllColor3i;
		qglColor3iv					= dllColor3iv;
		qglColor3s					= dllColor3s;
		qglColor3sv					= dllColor3sv;
		qglColor3ub					= dllColor3ub;
		qglColor3ubv				= dllColor3ubv;
		qglColor3ui					= dllColor3ui;
		qglColor3uiv				= dllColor3uiv;
		qglColor3us					= dllColor3us;
		qglColor3usv				= dllColor3usv;
		qglColor4b					= dllColor4b;
		qglColor4bv					= dllColor4bv;
		qglColor4d					= dllColor4d;
		qglColor4dv					= dllColor4dv;
		qglColor4f					= dllColor4f;
		qglColor4fv					= dllColor4fv;
		qglColor4i					= dllColor4i;
		qglColor4iv					= dllColor4iv;
		qglColor4s					= dllColor4s;
		qglColor4sv					= dllColor4sv;
		qglColor4ub					= dllColor4ub;
		qglColor4ubv				= dllColor4ubv;
		qglColor4ui					= dllColor4ui;
		qglColor4uiv				= dllColor4uiv;
		qglColor4us					= dllColor4us;
		qglColor4usv				= dllColor4usv;
		qglColorMask				= dllColorMask;
		qglColorMaterial			= dllColorMaterial;
		qglColorPointer				= dllColorPointer;
		qglCopyPixels				= dllCopyPixels;
		qglCopyTexImage1D			= dllCopyTexImage1D;
		qglCopyTexImage2D			= dllCopyTexImage2D;
		qglCopyTexSubImage1D		= dllCopyTexSubImage1D;
		qglCopyTexSubImage2D		= dllCopyTexSubImage2D;
		qglCullFace					= dllCullFace;
		qglDeleteLists				= dllDeleteLists;
		qglDeleteTextures			= dllDeleteTextures;
		qglDepthFunc				= dllDepthFunc;
		qglDepthMask				= dllDepthMask;
		qglDepthRange				= dllDepthRange;
		qglDisable					= dllDisable;
		qglDisableClientState		= dllDisableClientState;
		qglDrawArrays				= dllDrawArrays;
		qglDrawBuffer				= dllDrawBuffer;
		qglDrawElements				= dllDrawElements;
		qglDrawPixels				= dllDrawPixels;
		qglEdgeFlag					= dllEdgeFlag;
		qglEdgeFlagPointer			= dllEdgeFlagPointer;
		qglEdgeFlagv				= dllEdgeFlagv;
		qglEnable					= dllEnable;
		qglEnableClientState		= dllEnableClientState;
		qglEnd						= dllEnd;
		qglEndList					= dllEndList;
		qglEvalCoord1d				= dllEvalCoord1d;
		qglEvalCoord1dv				= dllEvalCoord1dv;
		qglEvalCoord1f				= dllEvalCoord1f;
		qglEvalCoord1fv				= dllEvalCoord1fv;
		qglEvalCoord2d				= dllEvalCoord2d;
		qglEvalCoord2dv				= dllEvalCoord2dv;
		qglEvalCoord2f				= dllEvalCoord2f;
		qglEvalCoord2fv				= dllEvalCoord2fv;
		qglEvalMesh1				= dllEvalMesh1;
		qglEvalMesh2				= dllEvalMesh2;
		qglEvalPoint1				= dllEvalPoint1;
		qglEvalPoint2				= dllEvalPoint2;
		qglFeedbackBuffer			= dllFeedbackBuffer;
		qglFinish					= dllFinish;
		qglFlush					= dllFlush;
		qglFogf						= dllFogf;
		qglFogfv					= dllFogfv;
		qglFogi						= dllFogi;
		qglFogiv					= dllFogiv;
		qglFrontFace				= dllFrontFace;
		qglFrustum					= dllFrustum;
		qglGenLists					= dllGenLists;
		qglGenTextures				= dllGenTextures;
		qglGetBooleanv				= dllGetBooleanv;
		qglGetClipPlane				= dllGetClipPlane;
		qglGetDoublev				= dllGetDoublev;
		qglGetError					= dllGetError;
		qglGetFloatv				= dllGetFloatv;
		qglGetIntegerv				= dllGetIntegerv;
		qglGetLightfv				= dllGetLightfv;
		qglGetLightiv				= dllGetLightiv;
		qglGetMapdv					= dllGetMapdv;
		qglGetMapfv					= dllGetMapfv;
		qglGetMapiv					= dllGetMapiv;
		qglGetMaterialfv			= dllGetMaterialfv;
		qglGetMaterialiv			= dllGetMaterialiv;
		qglGetPixelMapfv			= dllGetPixelMapfv;
		qglGetPixelMapuiv			= dllGetPixelMapuiv;
		qglGetPixelMapusv			= dllGetPixelMapusv;
		qglGetPointerv				= dllGetPointerv;
		qglGetPolygonStipple		= dllGetPolygonStipple;
		qglGetString				= dllGetString;
		qglGetTexEnvfv				= dllGetTexEnvfv;
		qglGetTexEnviv				= dllGetTexEnviv;
		qglGetTexGendv				= dllGetTexGendv;
		qglGetTexGenfv				= dllGetTexGenfv;
		qglGetTexGeniv				= dllGetTexGeniv;
		qglGetTexImage				= dllGetTexImage;
		qglGetTexLevelParameterfv	= dllGetTexLevelParameterfv;
		qglGetTexLevelParameteriv	= dllGetTexLevelParameteriv;
		qglGetTexParameterfv		= dllGetTexParameterfv;
		qglGetTexParameteriv		= dllGetTexParameteriv;
		qglHint						= dllHint;
		qglIndexMask				= dllIndexMask;
		qglIndexPointer				= dllIndexPointer;
		qglIndexd					= dllIndexd;
		qglIndexdv					= dllIndexdv;
		qglIndexf					= dllIndexf;
		qglIndexfv					= dllIndexfv;
		qglIndexi					= dllIndexi;
		qglIndexiv					= dllIndexiv;
		qglIndexs					= dllIndexs;
		qglIndexsv					= dllIndexsv;
		qglIndexub					= dllIndexub;
		qglIndexubv					= dllIndexubv;
		qglInitNames				= dllInitNames;
		qglInterleavedArrays		= dllInterleavedArrays;
		qglIsEnabled				= dllIsEnabled;
		qglIsList					= dllIsList;
		qglIsTexture				= dllIsTexture;
		qglLightModelf				= dllLightModelf;
		qglLightModelfv				= dllLightModelfv;
		qglLightModeli				= dllLightModeli;
		qglLightModeliv				= dllLightModeliv;
		qglLightf					= dllLightf;
		qglLightfv					= dllLightfv;
		qglLighti					= dllLighti;
		qglLightiv					= dllLightiv;
		qglLineStipple				= dllLineStipple;
		qglLineWidth				= dllLineWidth;
		qglListBase					= dllListBase;
		qglLoadIdentity				= dllLoadIdentity;
		qglLoadMatrixd				= dllLoadMatrixd;
		qglLoadMatrixf				= dllLoadMatrixf;
		qglLoadName					= dllLoadName;
		qglLogicOp					= dllLogicOp;
		qglMap1d					= dllMap1d;
		qglMap1f					= dllMap1f;
		qglMap2d					= dllMap2d;
		qglMap2f					= dllMap2f;
		qglMapGrid1d				= dllMapGrid1d;
		qglMapGrid1f				= dllMapGrid1f;
		qglMapGrid2d				= dllMapGrid2d;
		qglMapGrid2f				= dllMapGrid2f;
		qglMaterialf				= dllMaterialf;
		qglMaterialfv				= dllMaterialfv;
		qglMateriali				= dllMateriali;
		qglMaterialiv				= dllMaterialiv;
		qglMatrixMode				= dllMatrixMode;
		qglMultMatrixd				= dllMultMatrixd;
		qglMultMatrixf				= dllMultMatrixf;
		qglNewList					= dllNewList;
		qglNormal3b					= dllNormal3b;
		qglNormal3bv				= dllNormal3bv;
		qglNormal3d					= dllNormal3d;
		qglNormal3dv				= dllNormal3dv;
		qglNormal3f					= dllNormal3f;
		qglNormal3fv				= dllNormal3fv;
		qglNormal3i					= dllNormal3i;
		qglNormal3iv				= dllNormal3iv;
		qglNormal3s					= dllNormal3s;
		qglNormal3sv				= dllNormal3sv;
		qglNormalPointer			= dllNormalPointer;
		qglOrtho					= dllOrtho;
		qglPassThrough				= dllPassThrough;
		qglPixelMapfv				= dllPixelMapfv;
		qglPixelMapuiv				= dllPixelMapuiv;
		qglPixelMapusv				= dllPixelMapusv;
		qglPixelStoref				= dllPixelStoref;
		qglPixelStorei				= dllPixelStorei;
		qglPixelTransferf			= dllPixelTransferf;
		qglPixelTransferi			= dllPixelTransferi;
		qglPixelZoom				= dllPixelZoom;
		qglPointSize				= dllPointSize;
		qglPolygonMode				= dllPolygonMode;
		qglPolygonOffset			= dllPolygonOffset;
		qglPolygonStipple			= dllPolygonStipple;
		qglPopAttrib				= dllPopAttrib;
		qglPopClientAttrib			= dllPopClientAttrib;
		qglPopMatrix				= dllPopMatrix;
		qglPopName					= dllPopName;
		qglPrioritizeTextures		= dllPrioritizeTextures;
		qglPushAttrib				= dllPushAttrib;
		qglPushClientAttrib			= dllPushClientAttrib;
		qglPushMatrix				= dllPushMatrix;
		qglPushName					= dllPushName;
		qglRasterPos2d				= dllRasterPos2d;
		qglRasterPos2dv				= dllRasterPos2dv;
		qglRasterPos2f				= dllRasterPos2f;
		qglRasterPos2fv				= dllRasterPos2fv;
		qglRasterPos2i				= dllRasterPos2i;
		qglRasterPos2iv				= dllRasterPos2iv;
		qglRasterPos2s				= dllRasterPos2s;
		qglRasterPos2sv				= dllRasterPos2sv;
		qglRasterPos3d				= dllRasterPos3d;
		qglRasterPos3dv				= dllRasterPos3dv;
		qglRasterPos3f				= dllRasterPos3f;
		qglRasterPos3fv				= dllRasterPos3fv;
		qglRasterPos3i				= dllRasterPos3i;
		qglRasterPos3iv				= dllRasterPos3iv;
		qglRasterPos3s				= dllRasterPos3s;
		qglRasterPos3sv				= dllRasterPos3sv;
		qglRasterPos4d				= dllRasterPos4d;
		qglRasterPos4dv				= dllRasterPos4dv;
		qglRasterPos4f				= dllRasterPos4f;
		qglRasterPos4fv				= dllRasterPos4fv;
		qglRasterPos4i				= dllRasterPos4i;
		qglRasterPos4iv				= dllRasterPos4iv;
		qglRasterPos4s				= dllRasterPos4s;
		qglRasterPos4sv				= dllRasterPos4sv;
		qglReadBuffer				= dllReadBuffer;
		qglReadPixels				= dllReadPixels;
		qglRectd					= dllRectd;
		qglRectdv					= dllRectdv;
		qglRectf					= dllRectf;
		qglRectfv					= dllRectfv;
		qglRecti					= dllRecti;
		qglRectiv					= dllRectiv;
		qglRects					= dllRects;
		qglRectsv					= dllRectsv;
		qglRenderMode				= dllRenderMode;
		qglRotated					= dllRotated;
		qglRotatef					= dllRotatef;
		qglScaled					= dllScaled;
		qglScalef					= dllScalef;
		qglScissor					= dllScissor;
		qglSelectBuffer				= dllSelectBuffer;
		qglShadeModel				= dllShadeModel;
		qglStencilFunc				= dllStencilFunc;
		qglStencilMask				= dllStencilMask;
		qglStencilOp				= dllStencilOp;
		qglTexCoord1d				= dllTexCoord1d;
		qglTexCoord1dv				= dllTexCoord1dv;
		qglTexCoord1f				= dllTexCoord1f;
		qglTexCoord1fv				= dllTexCoord1fv;
		qglTexCoord1i				= dllTexCoord1i;
		qglTexCoord1iv				= dllTexCoord1iv;
		qglTexCoord1s				= dllTexCoord1s;
		qglTexCoord1sv				= dllTexCoord1sv;
		qglTexCoord2d				= dllTexCoord2d;
		qglTexCoord2dv				= dllTexCoord2dv;
		qglTexCoord2f				= dllTexCoord2f;
		qglTexCoord2fv				= dllTexCoord2fv;
		qglTexCoord2i				= dllTexCoord2i;
		qglTexCoord2iv				= dllTexCoord2iv;
		qglTexCoord2s				= dllTexCoord2s;
		qglTexCoord2sv				= dllTexCoord2sv;
		qglTexCoord3d				= dllTexCoord3d;
		qglTexCoord3dv				= dllTexCoord3dv;
		qglTexCoord3f				= dllTexCoord3f;
		qglTexCoord3fv				= dllTexCoord3fv;
		qglTexCoord3i				= dllTexCoord3i;
		qglTexCoord3iv				= dllTexCoord3iv;
		qglTexCoord3s				= dllTexCoord3s;
		qglTexCoord3sv				= dllTexCoord3sv;
		qglTexCoord4d				= dllTexCoord4d;
		qglTexCoord4dv				= dllTexCoord4dv;
		qglTexCoord4f				= dllTexCoord4f;
		qglTexCoord4fv				= dllTexCoord4fv;
		qglTexCoord4i				= dllTexCoord4i;
		qglTexCoord4iv				= dllTexCoord4iv;
		qglTexCoord4s				= dllTexCoord4s;
		qglTexCoord4sv				= dllTexCoord4sv;
		qglTexCoordPointer			= dllTexCoordPointer;
		qglTexEnvf					= dllTexEnvf;
		qglTexEnvfv					= dllTexEnvfv;
		qglTexEnvi					= dllTexEnvi;
		qglTexEnviv					= dllTexEnviv;
		qglTexGend					= dllTexGend;
		qglTexGendv					= dllTexGendv;
		qglTexGenf					= dllTexGenf;
		qglTexGenfv					= dllTexGenfv;
		qglTexGeni					= dllTexGeni;
		qglTexGeniv					= dllTexGeniv;
		qglTexImage1D				= dllTexImage1D;
		qglTexImage2D				= dllTexImage2D;
		qglTexParameterf			= dllTexParameterf;
		qglTexParameterfv			= dllTexParameterfv;
		qglTexParameteri			= dllTexParameteri;
		qglTexParameteriv			= dllTexParameteriv;
		qglTexSubImage1D			= dllTexSubImage1D;
		qglTexSubImage2D			= dllTexSubImage2D;
		qglTranslated				= dllTranslated;
		qglTranslatef				= dllTranslatef;
		qglVertex2d					= dllVertex2d;
		qglVertex2dv				= dllVertex2dv;
		qglVertex2f					= dllVertex2f;
		qglVertex2fv				= dllVertex2fv;
		qglVertex2i					= dllVertex2i;
		qglVertex2iv				= dllVertex2iv;
		qglVertex2s					= dllVertex2s;
		qglVertex2sv				= dllVertex2sv;
		qglVertex3d					= dllVertex3d;
		qglVertex3dv				= dllVertex3dv;
		qglVertex3f					= dllVertex3f;
		qglVertex3fv				= dllVertex3fv;
		qglVertex3i					= dllVertex3i;
		qglVertex3iv				= dllVertex3iv;
		qglVertex3s					= dllVertex3s;
		qglVertex3sv				= dllVertex3sv;
		qglVertex4d					= dllVertex4d;
		qglVertex4dv				= dllVertex4dv;
		qglVertex4f					= dllVertex4f;
		qglVertex4fv				= dllVertex4fv;
		qglVertex4i					= dllVertex4i;
		qglVertex4iv				= dllVertex4iv;
		qglVertex4s					= dllVertex4s;
		qglVertex4sv				= dllVertex4sv;
		qglVertexPointer			= dllVertexPointer;
		qglViewport					= dllViewport;
	}
}


/*
===========
QGL_LogBeginFrame
===========
*/
void QGL_LogBeginFrame (void)
{
	if (!gl_log->intVal)
		return;

	fprintf (LOGPROC, "*** R_BeginFrame ***\n");
}


/*
===========
QGL_LogEndFrame
===========
*/
void QGL_LogEndFrame (void)
{
	if (!gl_log->intVal)
		return;

	fprintf (LOGPROC, "*** R_EndFrame ***\n");
}


/*
===========
QGL_GetProcAddress
===========
*/
void *QGL_GetProcAddress (const char *procName)
{
#ifdef WIN32
	return qwglGetProcAddress ((LPCSTR) procName);
#elif defined __unix__
	if (glxState.OpenGLLib)
		return (void *)dlsym (glxState.OpenGLLib, procName);

	return NULL;
#endif
}
