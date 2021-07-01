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

// QGL.H

#ifndef __QGL_H__
#define __QGL_H__

#ifdef _WIN32
# include <windows.h>
#endif

#include <GL/gl.h>

#ifdef __linux__
# include <GL/glx.h>
#endif

#ifndef APIENTRY
# define APIENTRY
#endif
#ifndef APIENTRYP
# define APIENTRYP APIENTRY *
#endif

extern void		(APIENTRYP qglAccum) (GLenum op, GLfloat value);
extern void		(APIENTRYP qglAlphaFunc) (GLenum func, GLclampf ref);
extern GLboolean (APIENTRYP qglAreTexturesResident) (GLsizei n, const GLuint *textures, GLboolean *residences);
extern void		(APIENTRYP qglArrayElement) (GLint i);
extern void		(APIENTRYP qglBegin) (GLenum mode);
extern void		(APIENTRYP qglBindTexture) (GLenum target, GLuint texture);
extern void		(APIENTRYP qglBitmap) (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void		(APIENTRYP qglBlendFunc) (GLenum sfactor, GLenum dfactor);
extern void		(APIENTRYP qglCallList) (GLuint list);
extern void		(APIENTRYP qglCallLists) (GLsizei n, GLenum type, const GLvoid *lists);
extern void		(APIENTRYP qglClear) (GLbitfield mask);
extern void		(APIENTRYP qglClearAccum) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void		(APIENTRYP qglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void		(APIENTRYP qglClearDepth) (GLclampd depth);
extern void		(APIENTRYP qglClearIndex) (GLfloat c);
extern void		(APIENTRYP qglClearStencil) (GLint s);
extern void		(APIENTRYP qglClipPlane) (GLenum plane, const GLdouble *equation);
extern void		(APIENTRYP qglColor3b) (GLbyte red, GLbyte green, GLbyte blue);
extern void		(APIENTRYP qglColor3bv) (const GLbyte *v);
extern void		(APIENTRYP qglColor3d) (GLdouble red, GLdouble green, GLdouble blue);
extern void		(APIENTRYP qglColor3dv) (const GLdouble *v);
extern void		(APIENTRYP qglColor3f) (GLfloat red, GLfloat green, GLfloat blue);
extern void		(APIENTRYP qglColor3fv) (const GLfloat *v);
extern void		(APIENTRYP qglColor3i) (GLint red, GLint green, GLint blue);
extern void		(APIENTRYP qglColor3iv) (const GLint *v);
extern void		(APIENTRYP qglColor3s) (GLshort red, GLshort green, GLshort blue);
extern void		(APIENTRYP qglColor3sv) (const GLshort *v);
extern void		(APIENTRYP qglColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
extern void		(APIENTRYP qglColor3ubv) (const GLubyte *v);
extern void		(APIENTRYP qglColor3ui) (GLuint red, GLuint green, GLuint blue);
extern void		(APIENTRYP qglColor3uiv) (const GLuint *v);
extern void		(APIENTRYP qglColor3us) (GLushort red, GLushort green, GLushort blue);
extern void		(APIENTRYP qglColor3usv) (const GLushort *v);
extern void		(APIENTRYP qglColor4b) (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void		(APIENTRYP qglColor4bv) (const GLbyte *v);
extern void		(APIENTRYP qglColor4d) (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void		(APIENTRYP qglColor4dv) (const GLdouble *v);
extern void		(APIENTRYP qglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void		(APIENTRYP qglColor4fv) (const GLfloat *v);
extern void		(APIENTRYP qglColor4i) (GLint red, GLint green, GLint blue, GLint alpha);
extern void		(APIENTRYP qglColor4iv) (const GLint *v);
extern void		(APIENTRYP qglColor4s) (GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void		(APIENTRYP qglColor4sv) (const GLshort *v);
extern void		(APIENTRYP qglColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void		(APIENTRYP qglColor4ubv) (const GLubyte *v);
extern void		(APIENTRYP qglColor4ui) (GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void		(APIENTRYP qglColor4uiv) (const GLuint *v);
extern void		(APIENTRYP qglColor4us) (GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void		(APIENTRYP qglColor4usv) (const GLushort *v);
extern void		(APIENTRYP qglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void		(APIENTRYP qglColorMaterial) (GLenum face, GLenum mode);
extern void		(APIENTRYP qglColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglCopyPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void		(APIENTRYP qglCopyTexImage1D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void		(APIENTRYP qglCopyTexImage2D) (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void		(APIENTRYP qglCopyTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void		(APIENTRYP qglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void		(APIENTRYP qglCullFace) (GLenum mode);
extern void		(APIENTRYP qglDeleteLists) (GLuint list, GLsizei range);
extern void		(APIENTRYP qglDeleteTextures) (GLsizei n, const GLuint *textures);
extern void		(APIENTRYP qglDepthFunc) (GLenum func);
extern void		(APIENTRYP qglDepthMask) (GLboolean flag);
extern void		(APIENTRYP qglDepthRange) (GLclampd zNear, GLclampd zFar);
extern void		(APIENTRYP qglDisable) (GLenum cap);
extern void		(APIENTRYP qglDisableClientState) (GLenum array);
extern void		(APIENTRYP qglDrawArrays) (GLenum mode, GLint first, GLsizei count);
extern void		(APIENTRYP qglDrawBuffer) (GLenum mode);
extern void		(APIENTRYP qglDrawElements) (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void		(APIENTRYP qglDrawPixels) (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRYP qglEdgeFlag) (GLboolean flag);
extern void		(APIENTRYP qglEdgeFlagPointer) (GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglEdgeFlagv) (const GLboolean *flag);
extern void		(APIENTRYP qglEnable) (GLenum cap);
extern void		(APIENTRYP qglEnableClientState) (GLenum array);
extern void		(APIENTRYP qglEnd) (void);
extern void		(APIENTRYP qglEndList) (void);
extern void		(APIENTRYP qglEvalCoord1d) (GLdouble u);
extern void		(APIENTRYP qglEvalCoord1dv) (const GLdouble *u);
extern void		(APIENTRYP qglEvalCoord1f) (GLfloat u);
extern void		(APIENTRYP qglEvalCoord1fv) (const GLfloat *u);
extern void		(APIENTRYP qglEvalCoord2d) (GLdouble u, GLdouble v);
extern void		(APIENTRYP qglEvalCoord2dv) (const GLdouble *u);
extern void		(APIENTRYP qglEvalCoord2f) (GLfloat u, GLfloat v);
extern void		(APIENTRYP qglEvalCoord2fv) (const GLfloat *u);
extern void		(APIENTRYP qglEvalMesh1) (GLenum mode, GLint i1, GLint i2);
extern void		(APIENTRYP qglEvalMesh2) (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void		(APIENTRYP qglEvalPoint1) (GLint i);
extern void		(APIENTRYP qglEvalPoint2) (GLint i, GLint j);
extern void		(APIENTRYP qglFeedbackBuffer) (GLsizei size, GLenum type, GLfloat *buffer);
extern void		(APIENTRYP qglFinish) (void);
extern void		(APIENTRYP qglFlush) (void);
extern void		(APIENTRYP qglFogf) (GLenum pname, GLfloat param);
extern void		(APIENTRYP qglFogfv) (GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglFogi) (GLenum pname, GLint param);
extern void		(APIENTRYP qglFogiv) (GLenum pname, const GLint *params);
extern void		(APIENTRYP qglFrontFace) (GLenum mode);
extern void		(APIENTRYP qglFrustum) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern GLuint	(APIENTRYP qglGenLists) (GLsizei range);
extern void		(APIENTRYP qglGenTextures) (GLsizei n, GLuint *textures);
extern void		(APIENTRYP qglGetBooleanv) (GLenum pname, GLboolean *params);
extern void		(APIENTRYP qglGetClipPlane) (GLenum plane, GLdouble *equation);
extern void		(APIENTRYP qglGetDoublev) (GLenum pname, GLdouble *params);
extern GLenum	(APIENTRYP qglGetError) (void);
extern void		(APIENTRYP qglGetFloatv) (GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetIntegerv) (GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetLightfv) (GLenum light, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetLightiv) (GLenum light, GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetMapdv) (GLenum target, GLenum query, GLdouble *v);
extern void		(APIENTRYP qglGetMapfv) (GLenum target, GLenum query, GLfloat *v);
extern void		(APIENTRYP qglGetMapiv) (GLenum target, GLenum query, GLint *v);
extern void		(APIENTRYP qglGetMaterialfv) (GLenum face, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetMaterialiv) (GLenum face, GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetPixelMapfv) (GLenum map, GLfloat *values);
extern void		(APIENTRYP qglGetPixelMapuiv) (GLenum map, GLuint *values);
extern void		(APIENTRYP qglGetPixelMapusv) (GLenum map, GLushort *values);
extern void		(APIENTRYP qglGetPointerv) (GLenum pname, GLvoid* *params);
extern void		(APIENTRYP qglGetPolygonStipple) (GLubyte *mask);
extern const GLubyte * (APIENTRYP qglGetString) (GLenum name);
extern void		(APIENTRYP qglGetTexEnvfv) (GLenum target, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetTexEnviv) (GLenum target, GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetTexGendv) (GLenum coord, GLenum pname, GLdouble *params);
extern void		(APIENTRYP qglGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetTexGeniv) (GLenum coord, GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void		(APIENTRYP qglGetTexLevelParameterfv) (GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
extern void		(APIENTRYP qglGetTexParameterfv) (GLenum target, GLenum pname, GLfloat *params);
extern void		(APIENTRYP qglGetTexParameteriv) (GLenum target, GLenum pname, GLint *params);
extern void		(APIENTRYP qglHint) (GLenum target, GLenum mode);
extern void		(APIENTRYP qglIndexMask) (GLuint mask);
extern void		(APIENTRYP qglIndexPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglIndexd) (GLdouble c);
extern void		(APIENTRYP qglIndexdv) (const GLdouble *c);
extern void		(APIENTRYP qglIndexf) (GLfloat c);
extern void		(APIENTRYP qglIndexfv) (const GLfloat *c);
extern void		(APIENTRYP qglIndexi) (GLint c);
extern void		(APIENTRYP qglIndexiv) (const GLint *c);
extern void		(APIENTRYP qglIndexs) (GLshort c);
extern void		(APIENTRYP qglIndexsv) (const GLshort *c);
extern void		(APIENTRYP qglIndexub) (GLubyte c);
extern void		(APIENTRYP qglIndexubv) (const GLubyte *c);
extern void		(APIENTRYP qglInitNames) (void);
extern void		(APIENTRYP qglInterleavedArrays) (GLenum format, GLsizei stride, const GLvoid *pointer);
extern GLboolean (APIENTRYP qglIsEnabled) (GLenum cap);
extern GLboolean (APIENTRYP qglIsList) (GLuint list);
extern GLboolean (APIENTRYP qglIsTexture) (GLuint texture);
extern void		(APIENTRYP qglLightModelf) (GLenum pname, GLfloat param);
extern void		(APIENTRYP qglLightModelfv) (GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglLightModeli) (GLenum pname, GLint param);
extern void		(APIENTRYP qglLightModeliv) (GLenum pname, const GLint *params);
extern void		(APIENTRYP qglLightf) (GLenum light, GLenum pname, GLfloat param);
extern void		(APIENTRYP qglLightfv) (GLenum light, GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglLighti) (GLenum light, GLenum pname, GLint param);
extern void		(APIENTRYP qglLightiv) (GLenum light, GLenum pname, const GLint *params);
extern void		(APIENTRYP qglLineStipple) (GLint factor, GLushort pattern);
extern void		(APIENTRYP qglLineWidth) (GLfloat width);
extern void		(APIENTRYP qglListBase) (GLuint base);
extern void		(APIENTRYP qglLoadIdentity) (void);
extern void		(APIENTRYP qglLoadMatrixd) (const GLdouble *m);
extern void		(APIENTRYP qglLoadMatrixf) (const GLfloat *m);
extern void		(APIENTRYP qglLoadName) (GLuint name);
extern void		(APIENTRYP qglLogicOp) (GLenum opcode);
extern void		(APIENTRYP qglMap1d) (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void		(APIENTRYP qglMap1f) (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void		(APIENTRYP qglMap2d) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void		(APIENTRYP qglMap2f) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void		(APIENTRYP qglMapGrid1d) (GLint un, GLdouble u1, GLdouble u2);
extern void		(APIENTRYP qglMapGrid1f) (GLint un, GLfloat u1, GLfloat u2);
extern void		(APIENTRYP qglMapGrid2d) (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void		(APIENTRYP qglMapGrid2f) (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void		(APIENTRYP qglMaterialf) (GLenum face, GLenum pname, GLfloat param);
extern void		(APIENTRYP qglMaterialfv) (GLenum face, GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglMateriali) (GLenum face, GLenum pname, GLint param);
extern void		(APIENTRYP qglMaterialiv) (GLenum face, GLenum pname, const GLint *params);
extern void		(APIENTRYP qglMatrixMode) (GLenum mode);
extern void		(APIENTRYP qglMultMatrixd) (const GLdouble *m);
extern void		(APIENTRYP qglMultMatrixf) (const GLfloat *m);
extern void		(APIENTRYP qglNewList) (GLuint list, GLenum mode);
extern void		(APIENTRYP qglNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
extern void		(APIENTRYP qglNormal3bv) (const GLbyte *v);
extern void		(APIENTRYP qglNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
extern void		(APIENTRYP qglNormal3dv) (const GLdouble *v);
extern void		(APIENTRYP qglNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
extern void		(APIENTRYP qglNormal3fv) (const GLfloat *v);
extern void		(APIENTRYP qglNormal3i) (GLint nx, GLint ny, GLint nz);
extern void		(APIENTRYP qglNormal3iv) (const GLint *v);
extern void		(APIENTRYP qglNormal3s) (GLshort nx, GLshort ny, GLshort nz);
extern void		(APIENTRYP qglNormal3sv) (const GLshort *v);
extern void		(APIENTRYP qglNormalPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void		(APIENTRYP qglPassThrough) (GLfloat token);
extern void		(APIENTRYP qglPixelMapfv) (GLenum map, GLsizei mapsize, const GLfloat *values);
extern void		(APIENTRYP qglPixelMapuiv) (GLenum map, GLsizei mapsize, const GLuint *values);
extern void		(APIENTRYP qglPixelMapusv) (GLenum map, GLsizei mapsize, const GLushort *values);
extern void		(APIENTRYP qglPixelStoref) (GLenum pname, GLfloat param);
extern void		(APIENTRYP qglPixelStorei) (GLenum pname, GLint param);
extern void		(APIENTRYP qglPixelTransferf) (GLenum pname, GLfloat param);
extern void		(APIENTRYP qglPixelTransferi) (GLenum pname, GLint param);
extern void		(APIENTRYP qglPixelZoom) (GLfloat xfactor, GLfloat yfactor);
extern void		(APIENTRYP qglPointSize) (GLfloat size);
extern void		(APIENTRYP qglPolygonMode) (GLenum face, GLenum mode);
extern void		(APIENTRYP qglPolygonOffset) (GLfloat factor, GLfloat units);
extern void		(APIENTRYP qglPolygonStipple) (const GLubyte *mask);
extern void		(APIENTRYP qglPopAttrib) (void);
extern void		(APIENTRYP qglPopClientAttrib) (void);
extern void		(APIENTRYP qglPopMatrix) (void);
extern void		(APIENTRYP qglPopName) (void);
extern void		(APIENTRYP qglPrioritizeTextures) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern void		(APIENTRYP qglPushAttrib) (GLbitfield mask);
extern void		(APIENTRYP qglPushClientAttrib) (GLbitfield mask);
extern void		(APIENTRYP qglPushMatrix) (void);
extern void		(APIENTRYP qglPushName) (GLuint name);
extern void		(APIENTRYP qglRasterPos2d) (GLdouble x, GLdouble y);
extern void		(APIENTRYP qglRasterPos2dv) (const GLdouble *v);
extern void		(APIENTRYP qglRasterPos2f) (GLfloat x, GLfloat y);
extern void		(APIENTRYP qglRasterPos2fv) (const GLfloat *v);
extern void		(APIENTRYP qglRasterPos2i) (GLint x, GLint y);
extern void		(APIENTRYP qglRasterPos2iv) (const GLint *v);
extern void		(APIENTRYP qglRasterPos2s) (GLshort x, GLshort y);
extern void		(APIENTRYP qglRasterPos2sv) (const GLshort *v);
extern void		(APIENTRYP qglRasterPos3d) (GLdouble x, GLdouble y, GLdouble z);
extern void		(APIENTRYP qglRasterPos3dv) (const GLdouble *v);
extern void		(APIENTRYP qglRasterPos3f) (GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRYP qglRasterPos3fv) (const GLfloat *v);
extern void		(APIENTRYP qglRasterPos3i) (GLint x, GLint y, GLint z);
extern void		(APIENTRYP qglRasterPos3iv) (const GLint *v);
extern void		(APIENTRYP qglRasterPos3s) (GLshort x, GLshort y, GLshort z);
extern void		(APIENTRYP qglRasterPos3sv) (const GLshort *v);
extern void		(APIENTRYP qglRasterPos4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void		(APIENTRYP qglRasterPos4dv) (const GLdouble *v);
extern void		(APIENTRYP qglRasterPos4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void		(APIENTRYP qglRasterPos4fv) (const GLfloat *v);
extern void		(APIENTRYP qglRasterPos4i) (GLint x, GLint y, GLint z, GLint w);
extern void		(APIENTRYP qglRasterPos4iv) (const GLint *v);
extern void		(APIENTRYP qglRasterPos4s) (GLshort x, GLshort y, GLshort z, GLshort w);
extern void		(APIENTRYP qglRasterPos4sv) (const GLshort *v);
extern void		(APIENTRYP qglReadBuffer) (GLenum mode);
extern void		(APIENTRYP qglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void		(APIENTRYP qglRectd) (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void		(APIENTRYP qglRectdv) (const GLdouble *v1, const GLdouble *v2);
extern void		(APIENTRYP qglRectf) (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void		(APIENTRYP qglRectfv) (const GLfloat *v1, const GLfloat *v2);
extern void		(APIENTRYP qglRecti) (GLint x1, GLint y1, GLint x2, GLint y2);
extern void		(APIENTRYP qglRectiv) (const GLint *v1, const GLint *v2);
extern void		(APIENTRYP qglRects) (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void		(APIENTRYP qglRectsv) (const GLshort *v1, const GLshort *v2);
extern GLint	(APIENTRYP qglRenderMode) (GLenum mode);
extern void		(APIENTRYP qglRotated) (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void		(APIENTRYP qglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRYP qglScaled) (GLdouble x, GLdouble y, GLdouble z);
extern void		(APIENTRYP qglScalef) (GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRYP qglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
extern void		(APIENTRYP qglSelectBuffer) (GLsizei size, GLuint *buffer);
extern void		(APIENTRYP qglShadeModel) (GLenum mode);
extern void		(APIENTRYP qglStencilFunc) (GLenum func, GLint ref, GLuint mask);
extern void		(APIENTRYP qglStencilMask) (GLuint mask);
extern void		(APIENTRYP qglStencilOp) (GLenum fail, GLenum zfail, GLenum zpass);
extern void		(APIENTRYP qglTexCoord1d) (GLdouble s);
extern void		(APIENTRYP qglTexCoord1dv) (const GLdouble *v);
extern void		(APIENTRYP qglTexCoord1f) (GLfloat s);
extern void		(APIENTRYP qglTexCoord1fv) (const GLfloat *v);
extern void		(APIENTRYP qglTexCoord1i) (GLint s);
extern void		(APIENTRYP qglTexCoord1iv) (const GLint *v);
extern void		(APIENTRYP qglTexCoord1s) (GLshort s);
extern void		(APIENTRYP qglTexCoord1sv) (const GLshort *v);
extern void		(APIENTRYP qglTexCoord2d) (GLdouble s, GLdouble t);
extern void		(APIENTRYP qglTexCoord2dv) (const GLdouble *v);
extern void		(APIENTRYP qglTexCoord2f) (GLfloat s, GLfloat t);
extern void		(APIENTRYP qglTexCoord2fv) (const GLfloat *v);
extern void		(APIENTRYP qglTexCoord2i) (GLint s, GLint t);
extern void		(APIENTRYP qglTexCoord2iv) (const GLint *v);
extern void		(APIENTRYP qglTexCoord2s) (GLshort s, GLshort t);
extern void		(APIENTRYP qglTexCoord2sv) (const GLshort *v);
extern void		(APIENTRYP qglTexCoord3d) (GLdouble s, GLdouble t, GLdouble r);
extern void		(APIENTRYP qglTexCoord3dv) (const GLdouble *v);
extern void		(APIENTRYP qglTexCoord3f) (GLfloat s, GLfloat t, GLfloat r);
extern void		(APIENTRYP qglTexCoord3fv) (const GLfloat *v);
extern void		(APIENTRYP qglTexCoord3i) (GLint s, GLint t, GLint r);
extern void		(APIENTRYP qglTexCoord3iv) (const GLint *v);
extern void		(APIENTRYP qglTexCoord3s) (GLshort s, GLshort t, GLshort r);
extern void		(APIENTRYP qglTexCoord3sv) (const GLshort *v);
extern void		(APIENTRYP qglTexCoord4d) (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void		(APIENTRYP qglTexCoord4dv) (const GLdouble *v);
extern void		(APIENTRYP qglTexCoord4f) (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void		(APIENTRYP qglTexCoord4fv) (const GLfloat *v);
extern void		(APIENTRYP qglTexCoord4i) (GLint s, GLint t, GLint r, GLint q);
extern void		(APIENTRYP qglTexCoord4iv) (const GLint *v);
extern void		(APIENTRYP qglTexCoord4s) (GLshort s, GLshort t, GLshort r, GLshort q);
extern void		(APIENTRYP qglTexCoord4sv) (const GLshort *v);
extern void		(APIENTRYP qglTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglTexEnvf) (GLenum target, GLenum pname, GLfloat param);
extern void		(APIENTRYP qglTexEnvfv) (GLenum target, GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglTexEnvi) (GLenum target, GLenum pname, GLint param);
extern void		(APIENTRYP qglTexEnviv) (GLenum target, GLenum pname, const GLint *params);
extern void		(APIENTRYP qglTexGend) (GLenum coord, GLenum pname, GLdouble param);
extern void		(APIENTRYP qglTexGendv) (GLenum coord, GLenum pname, const GLdouble *params);
extern void		(APIENTRYP qglTexGenf) (GLenum coord, GLenum pname, GLfloat param);
extern void		(APIENTRYP qglTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglTexGeni) (GLenum coord, GLenum pname, GLint param);
extern void		(APIENTRYP qglTexGeniv) (GLenum coord, GLenum pname, const GLint *params);
extern void		(APIENTRYP qglTexImage1D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRYP qglTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRYP qglTexParameterf) (GLenum target, GLenum pname, GLfloat param);
extern void		(APIENTRYP qglTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
extern void		(APIENTRYP qglTexParameteri) (GLenum target, GLenum pname, GLint param);
extern void		(APIENTRYP qglTexParameteriv) (GLenum target, GLenum pname, const GLint *params);
extern void		(APIENTRYP qglTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRYP qglTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void		(APIENTRYP qglTranslated) (GLdouble x, GLdouble y, GLdouble z);
extern void		(APIENTRYP qglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRYP qglVertex2d) (GLdouble x, GLdouble y);
extern void		(APIENTRYP qglVertex2dv) (const GLdouble *v);
extern void		(APIENTRYP qglVertex2f) (GLfloat x, GLfloat y);
extern void		(APIENTRYP qglVertex2fv) (const GLfloat *v);
extern void		(APIENTRYP qglVertex2i) (GLint x, GLint y);
extern void		(APIENTRYP qglVertex2iv) (const GLint *v);
extern void		(APIENTRYP qglVertex2s) (GLshort x, GLshort y);
extern void		(APIENTRYP qglVertex2sv) (const GLshort *v);
extern void		(APIENTRYP qglVertex3d) (GLdouble x, GLdouble y, GLdouble z);
extern void		(APIENTRYP qglVertex3dv) (const GLdouble *v);
extern void		(APIENTRYP qglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
extern void		(APIENTRYP qglVertex3fv) (const GLfloat *v);
extern void		(APIENTRYP qglVertex3i) (GLint x, GLint y, GLint z);
extern void		(APIENTRYP qglVertex3iv) (const GLint *v);
extern void		(APIENTRYP qglVertex3s) (GLshort x, GLshort y, GLshort z);
extern void		(APIENTRYP qglVertex3sv) (const GLshort *v);
extern void		(APIENTRYP qglVertex4d) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void		(APIENTRYP qglVertex4dv) (const GLdouble *v);
extern void		(APIENTRYP qglVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void		(APIENTRYP qglVertex4fv) (const GLfloat *v);
extern void		(APIENTRYP qglVertex4i) (GLint x, GLint y, GLint z, GLint w);
extern void		(APIENTRYP qglVertex4iv) (const GLint *v);
extern void		(APIENTRYP qglVertex4s) (GLshort x, GLshort y, GLshort z, GLshort w);
extern void		(APIENTRYP qglVertex4sv) (const GLshort *v);
extern void		(APIENTRYP qglVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void		(APIENTRYP qglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);

#ifdef _WIN32

//
// win32
//

#ifndef WINAPI
# define WINAPI APIENTRY
#endif
#ifndef WINAPIP
# define WINAPIP WINAPI *
#endif

extern int		(WINAPIP qwglChoosePixelFormat) (HDC, CONST PIXELFORMATDESCRIPTOR *);
extern int		(WINAPIP qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
extern int		(WINAPIP qwglGetPixelFormat) (HDC);
extern BOOL		(WINAPIP qwglSetPixelFormat) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
extern BOOL		(WINAPIP qwglSwapBuffers) (HDC);

extern BOOL		(WINAPIP qwglCopyContext) (HGLRC, HGLRC, UINT);
extern HGLRC	(WINAPIP qwglCreateContext) (HDC);
extern HGLRC	(WINAPIP qwglCreateLayerContext) (HDC, int);
extern BOOL		(WINAPIP qwglDeleteContext) (HGLRC);
extern HGLRC	(WINAPIP qwglGetCurrentContext) (VOID);
extern HDC		(WINAPIP qwglGetCurrentDC) (VOID);
extern PROC		(WINAPIP qwglGetProcAddress) (LPCSTR);
extern BOOL		(WINAPIP qwglMakeCurrent) (HDC, HGLRC);
extern BOOL		(WINAPIP qwglShareLists) (HGLRC, HGLRC);
extern BOOL		(WINAPIP qwglUseFontBitmaps) (HDC, DWORD, DWORD, DWORD);

extern BOOL		(WINAPIP qwglUseFontOutlines) (HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);

extern BOOL		(WINAPIP qwglDescribeLayerPlane) (HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
extern int		(WINAPIP qwglSetLayerPaletteEntries) (HDC, int, int, int, CONST COLORREF *);
extern int		(WINAPIP qwglGetLayerPaletteEntries) (HDC, int, int, int, COLORREF *);
extern BOOL		(WINAPIP qwglRealizeLayerPalette) (HDC, int, BOOL);
extern BOOL		(WINAPIP qwglSwapLayerBuffers) (HDC, UINT);

extern BOOL		(WINAPIP qwglSwapIntervalEXT) (int interval);

extern const char * (WINAPIP qwglGetExtensionsStringARB) (HDC);
extern BOOL		(WINAPIP qwglGetPixelFormatAttribivARB) (HDC, int, int, UINT, const int *, int *);
extern BOOL		(WINAPIP qwglGetPixelFormatAttribfvARB) (HDC, int, int, UINT, const int *, FLOAT *);
extern BOOL		(WINAPIP qwglChoosePixelFormatARB) (HDC, const int *, const FLOAT *, UINT, int *, UINT *);

extern const char * (WINAPIP qwglGetExtensionsStringEXT) (void);
extern BOOL		(WINAPIP qwglGetPixelFormatAttribivEXT) (HDC, int, int, UINT, int *, int *);
extern BOOL		(WINAPIP qwglGetPixelFormatAttribfvEXT) (HDC, int, int, UINT, int *, FLOAT *);
extern BOOL		(WINAPIP qwglChoosePixelFormatEXT) (HDC, const int *, const FLOAT *, UINT, int *, UINT *);

extern BOOL		(WINAPIP qwglGetDeviceGammaRamp3DFX) (HDC, WORD *);
extern BOOL		(WINAPIP qwglSetDeviceGammaRamp3DFX) (HDC, WORD *);
extern BOOL		(WINAPIP qwglGetDeviceGammaRampEXT) (unsigned char *pRed, unsigned char *pGreen, unsigned char *pBlue);
extern BOOL		(WINAPIP qwglSetDeviceGammaRampEXT) (const unsigned char *pRed, const unsigned char *pGreen, const unsigned char *pBlue);

#endif // _WIN32



#ifdef __linux__

//
// linux
//

//GLX Functions
extern XVisualInfo * (*qglXChooseVisual) (Display *dpy, int screen, int *attribList);
extern GLXContext (*qglXCreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
extern void		(*qglXDestroyContext) (Display *dpy, GLXContext ctx);
extern Bool		(*qglXMakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void		(*qglXCopyContext) (Display *dpy, GLXContext src, GLXContext dst, GLuint mask);
extern void		(*qglXSwapBuffers) (Display *dpy, GLXDrawable drawable);

#endif // linux



//
// extensions
//

extern void		(APIENTRYP qglMTexCoord2f) (GLenum, GLfloat, GLfloat);
extern void		(APIENTRYP qglSelectTextureSGIS) (GLenum);
extern void		(APIENTRYP qglActiveTextureARB) (GLenum);
extern void		(APIENTRYP qglClientActiveTextureARB) (GLenum);

extern void		(APIENTRYP qglLockArraysEXT) (int , int);
extern void		(APIENTRYP qglUnlockArraysEXT) (void);

extern void		(APIENTRYP qglDrawRangeElementsEXT) (GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);

extern void		(APIENTRYP qglBindBufferARB) (GLenum target, GLuint buffer);
extern void		(APIENTRYP qglDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
extern void		(APIENTRYP qglGenBuffersARB) (GLsizei n, GLuint *buffers);
extern GLboolean (APIENTRYP qglIsBufferARB) (GLuint buffer);
extern void		*(APIENTRYP qglMapBufferARB) (GLenum target, GLenum access);
extern GLboolean (APIENTRYP qglUnmapBufferARB) (GLenum target);
extern void		(APIENTRYP qglBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
extern void		(APIENTRYP qglBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

//
// extension constants
//

#define GL_TEXTURE0_SGIS					0x835E
#define GL_TEXTURE1_SGIS					0x835F

//
// implementation specific functions
//

qBool	QGL_Init (const char *dllName);
void	QGL_Shutdown (void);

void	QGL_ToggleLogging (void);
void	QGL_LogBeginFrame (void);
void	QGL_LogEndFrame (void);

void	*QGL_GetProcAddress (const GLubyte *procName);

#endif // __QGL_H__
