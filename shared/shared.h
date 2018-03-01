/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or v

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// shared.h
// Included first by ALL program modules
//

#ifndef __SHARED_H__
#define __SHARED_H__

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>

// =========================================================================

//R1Q2 SPECIFC
#define SVF_NOPREDICTION		0x00000008
//R1Q2 SPECIFC

// =========================================================================
// Windows
//
#ifdef _WIN32

# pragma intrinsic(memcmp)

# define NO_RETURN __declspec(noreturn)

# define GL_DRIVERNAME		"opengl32.dll"
# define AL_DRIVERNAME		"openal32.dll"

# define HAVE___INLINE
# define HAVE__SNPRINTF
# define HAVE__STRICMP
# define HAVE__VSNPRINTF

# define BUILDSTRING		"Win32"

# ifdef NDEBUG
#  ifdef _M_IX86
#   define CPUSTRING		"x86"
#  elif _M_X64
#   define CPUSTRING		"x64"
#  elif defined _M_ALPHA
#   define CPUSTRING		"AXP"
#  endif
# else // NDEBUG
#  ifdef _M_IX86
#   define CPUSTRING		"x86 Debug"
#  elif _M_X64
#   define CPUSTRING		"x64 Debug"
#  elif defined _M_ALPHA
#   define CPUSTRING		"AXP Debug"
#  endif
# endif // NDEBUG

# define strdup _strdup

// =========================================================================
// Generic Unix
//
#elif defined __unix__
#  define GL_DRIVERNAME		"libGL.so.1"
#  define AL_DRIVERNAME		"libopenal.so.0"
#  define GL_FORCEFINISH

#  define NO_RETURN __attribute__((noreturn))

#  define HAVE_INLINE
#  define HAVE_STRCASECMP

//
// Linux
//
# if defined __linux__
#  include <stdint.h>

#  define BUILDSTRING		"Linux"

#  ifdef NDEBUG
#   ifdef __i386__
#    define CPUSTRING		"i386"
#   elif defined(__alpha__)
#    define CPUSTRING		"AXP"
#   endif
#  else // NDEBUG
#   ifdef __i386__
#    define CPUSTRING		"i386 Debug"
#   elif defined(__alpha__)
#    define CPUSTRING		"AXP Debug"
#   endif
#  endif // NDEBUG

//
// FreeBSD
//
# elif defined __FreeBSD__
#  include <machine/param.h>
#  if __FreeBSD_version < 500000
#   include <inttypes.h>
#  else
#   include <stdint.h>
#  endif

#  define NO_RETURN __attribute__((noreturn))

#  define BUILDSTRING		"FreeBSD"

#  ifdef NDEBUG
#   ifdef __i386__
#    define CPUSTRING		"i386"
#   elif defined(__alpha__)
#    define CPUSTRING		"AXP"
#   endif
#  else // NDEBUG
#   ifdef __i386__
#    define CPUSTRING		"i386 Debug"
#   elif defined(__alpha__)
#    define CPUSTRING		"AXP Debug"
#   endif
#  endif // NDEBUG

# endif

#endif	// __unix__

typedef int16_t				int16;
typedef int32_t				int32;
typedef int64_t				int64;
typedef uint16_t			uint16;
typedef uint32_t			uint32;
typedef uint64_t			uint64;

// =========================================================================

#ifdef HAVE___INLINE
# ifndef inline
#  define inline __inline
# endif
#elif !defined(HAVE_INLINE)
# ifndef inline
#  define inline
# endif
#endif

#ifdef HAVE__SNPRINTF
# ifndef snprintf 
#  define snprintf _snprintf
# endif
#endif

#ifdef HAVE_STRCASECMP
# ifndef Q_stricmp
#  define Q_stricmp(s1,s2) strcasecmp ((s1), (s2))
# endif
# ifndef Q_strnicmp
#  define Q_strnicmp(s1,s2,n) strncasecmp ((s1), (s2), (n))
# endif
#endif

#ifdef HAVE__STRICMP
# ifndef Q_stricmp
#  define Q_stricmp(s1, s2) _stricmp((s1), (s2))
# endif
# ifndef Q_strnicmp
#  define Q_strnicmp(s1, s2, n) _strnicmp((s1), (s2), (n))
# endif
#endif

#ifdef HAVE__VSNPRINTF
# ifndef vsnprintf
#  define vsnprintf _vsnprintf
# endif
#endif

// =========================================================================

#if (defined(_M_IX86) || defined(__i386__)) && !defined(C_ONLY) && !defined(__unix__) // FIXME: make this work with unix
# define id386
#else
# ifdef id386
#  undef id386
# endif
#endif

#ifndef BUILDSTRING
#error No build string, need to fix
#endif

#ifndef CPUSTRING
#error No CPU string, need to fix
#endif

// =========================================================================

typedef unsigned char			byte;
typedef enum {qFalse, qTrue}	qBool;

/*
=============================================================================

	PROTOCOL

=============================================================================
*/

#define ORIGINAL_PROTOCOL_VERSION		34

#define ENHANCED_PROTOCOL_VERSION		35
#define ENHANCED_COMPATIBILITY_NUMBER	1905

#define MINOR_VERSION_R1Q2_BASE			1903
#define MINOR_VERSION_R1Q2_UCMD_UPDATES	1904
#define	MINOR_VERSION_R1Q2_32BIT_SOLID	1905

//
// server to client
// note: ONLY add things to the bottom, to keep Quake2 compatibility
//
enum svcTypes {
	SVC_BAD,

	//
	// these ops are known to the game dll
	//
	SVC_MUZZLEFLASH,
	SVC_MUZZLEFLASH2,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,

	//
	// the rest are private to the client and server (you can not modify their order!)
	//
	SVC_NOP,
	SVC_DISCONNECT,
	SVC_RECONNECT,
	SVC_SOUND,					// <see code>
	SVC_PRINT,					// [byte] id [string] null terminated string
	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated
	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,
	SVC_CENTERPRINT,			// [string] to put in center of the screen
	SVC_DOWNLOAD,				// [short] size [size bytes]
	SVC_PLAYERINFO,				// variable
	SVC_PACKETENTITIES,			// [...]
	SVC_DELTAPACKETENTITIES,	// [...]
	SVC_FRAME,

	SVC_ZPACKET,				// new for ENHANCED_PROTOCOL_VERSION
	SVC_ZDOWNLOAD,				// new for ENHANCED_PROTOCOL_VERSION

	SVC_MAX
};

//
// game print flags
//
enum gamePrint {
	PRINT_LOW,				// pickup messages
	PRINT_MEDIUM,			// death messages
	PRINT_HIGH,				// critical messages
	PRINT_CHAT				// chat messages
};

//
// destination class for gi.multicast()
//
typedef enum {
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multiCast_t;

//
// client connection state
//
typedef enum caState_s {
	CA_UNINITIALIZED,	// initial state
	CA_DISCONNECTED,	// not talking to a server
	CA_CONNECTING,		// sending request packets to the server
	CA_CONNECTED,		// netChan_t established, waiting for svc_serverdata
	CA_ACTIVE			// game views should be displayed
} caState_t;

//
// server state
//
typedef enum ssState_s {
	SS_DEAD,			// no map loaded
	SS_LOADING,			// spawning level edicts
	SS_GAME,			// actively running
	SS_CINEMATIC,		// playing a cinematic
	SS_DEMO,			// playing a demo
	SS_PIC				// just showing a pic
} ssState_t;

/*
==============================================================================

	MATHLIB
 
==============================================================================
*/

typedef byte	bvec2_t[2];
typedef byte	bvec3_t[3];
typedef byte	bvec4_t[4];

typedef double	dvec2_t[2];
typedef double	dvec3_t[3];
typedef double	dvec4_t[4];

typedef int32	index_t;
typedef int32	ivec2_t[2];
typedef int32	ivec3_t[3];
typedef int32	ivec4_t[4];

typedef int16	svec2_t[2];
typedef int16	svec3_t[3];
typedef int16	svec4_t[4];

typedef float	vec2_t[2];
typedef float	vec3_t[3];
typedef float	vec4_t[4];

typedef float	mat_t;
typedef	float	quat_t[4];
typedef float	mat3x3_t[3][3];
typedef float	mat4x4_t[16];

// ===========================================================================

extern vec2_t	vec2Origin;
extern vec3_t	vec3Origin;
extern vec4_t	vec4Origin;
extern mat4x4_t	mat4x4Identity;
extern mat3x3_t	axisIdentity;
extern quat_t	quatIdentity;

// ===========================================================================

#ifndef M_PI
# define M_PI			3.14159265358979323846f		// matches value in gcc v2 math.h
#endif

// angle indexes
enum {
	PITCH,		// up / down
	YAW,		// left / right
	ROLL		// fall over
};

#define DEG2RAD(v) ((v) * (M_PI / 180.0f))
#define RAD2DEG(v) ((v) * (180.0f / M_PI))

#define ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)	((x)*(360.0f/65536))

#define ANGLE2BYTE(x)	((int)((x)*256/360) & 255)
#define BYTE2ANGLE(x)	((x)*(360.0f/256))

#ifndef max
# define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
# define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#define bound(a,b,c)	((a) >= (c) ? (a) : (b) < (a) ? (a) : (b) > (c) ? (c) : (b))
#define clamp(a,b,c)	((b) >= (c) ? (b) : (a) < (b) ? (b) : (a) > (c) ? (c) : (a))
#define clampl(a,b,c)	((b) >= (c) ? (a)=(b) : (a) < (b) ? (a)=(b) : (a) > (c) ? (a)=(c) : (a)=(a))

// ===========================================================================

void	seedMT (uint32 seed);
uint32	randomMT (void);

#define frand() (randomMT() * 0.00000000023283064365386962890625f)	// 0 to 1
#define crand() (((int)randomMT() - 0x7FFFFFFF) * 0.000000000465661287307739257812f)	// -1 to 1

// ===========================================================================

#define LARGE_EPSILON		0.1
#define SMALL_EPSILON		0.01
#define TINY_EPSILON		0.001

// ===========================================================================

#define NUMVERTEXNORMALS	162
extern vec3_t	m_byteDirs[NUMVERTEXNORMALS];

byte		DirToByte (vec3_t dirVec);
void		ByteToDir (byte dirByte, vec3_t dirVec);

// ===========================================================================

byte		FloatToByte (float x);

float		ColorNormalizef (const float *in, float *out);
float		ColorNormalizeb (const float *in, byte *out);

// ===========================================================================

#define Vec2Add(a,b,out)		((out)[0]=(a)[0]+(b)[0],(out)[1]=(a)[1]+(b)[1])
#define Vec2Clear(in)			(*(int *)&(in)[0]=0,*(int *)&(in)[1]=0)
#define Vec2Compare(v1,v2)		((v1)[0]==(v2)[0] && (v1)[1]==(v2)[1])
#define Vec2Copy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1])
#define Vec2Dist(v1,v2)			(sqrt((((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1]))))
#define Vec2Inverse(v)			((v)[0]=-(v)[0],(v)[1]=-(v)[1])
#define Vec2MA(v,s,b,o)			((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s))
#define Vec2Negate(a,b)			((b)[0]=-(a)[0],(b)[1]=-(a)[1])
#define Vec2Scale(in,s,out)		((out)[0]=(in)[0]*(s),(out)[1]=(in)[1]*(s))
#define Vec2Set(v,x,y)			((v)[0]=(x),(v)[1]=(y))
#define Vec2Subtract(a,b,c)		((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1])

#define CrossProduct(v1,v2,cr)	((cr)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1],(cr)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2],(cr)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0])
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])

#define Vec3Add(a,b,out)		((out)[0]=(a)[0]+(b)[0],(out)[1]=(a)[1]+(b)[1],(out)[2]=(a)[2]+(b)[2])
#define Vec3Average(a,b,c)		((c)[0]=((a)[0]+(b)[0])*0.5f,(c)[1]=((a)[1]+(b)[1])*0.5f, (c)[2]=((a)[2]+(b)[2])*0.5f)
#define Vec3Clear(a)			(*(int *)&(a)[0]=0,*(int *)&(a)[1]=0,*(int *)&(a)[2]=0)
#define Vec3Compare(v1,v2)		((v1)[0]==(v2)[0] && (v1)[1]==(v2)[1] && (v1)[2]==(v2)[2])
#define Vec3Copy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define Vec3Dist(v1,v2)			(sqrt((((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1])+((v1)[2]-(v2)[2])*((v1)[2]-(v2)[2]))))
#define Vec3Inverse(v)			((v)[0]=-(v)[0],(v)[1]=-(v)[1],(v)[2]=-(v)[2])
#define Vec3Length(v)			(sqrt(DotProduct((v),(v))))
#define Vec3MA(v,s,b,o)			((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define Vec3Negate(a,b)			((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define Vec3Scale(in,s,out)		((out)[0]=(in)[0]*(s),(out)[1]=(in)[1]*(s),(out)[2]=(in)[2]*(s))
#define Vec3Set(v,x,y,z)		((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))
#define Vec3Subtract(a,b,c)		((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])

#define Vec4Add(a,b,c)			((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2],(c)[3]=(a)[3]+(b)[3])
#define Vec4Average(a,b,c)		((c)[0]=((a)[0]+(b)[0])*0.5f,(c)[1]=((a)[1]+(b)[1])*0.5f,(c)[2]=((a)[2]+(b)[2])*0.5f,(c)[3]=((a)[3]+(b)[3])*0.5f)
#define Vec4Clear(a)			(*(int *)&(a)[0]=0,*(int *)&(a)[1]=0,*(int *)&(a)[2]=0,*(int *)&(a)[3]=0)
#define Vec4Copy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define Vec4Identity(a)			((a)[0]=(a)[1]=(a)[2]=(a)[3]=0)
#define Vec4MA(v,s,b,o)			((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s),(o)[3]=(v)[3]+(b)[3]*(s))
#define Vec4Negate(a,b)			((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2],(b)[3]=-(a)[3])
#define Vec4Scale(in,s,out)		((out)[0]=(in)[0]*(s),(out)[1]=(in)[1]*(s),(out)[2]=(in)[2]*(s),(out)[3]=(in)[3]*(s))
#define Vec4Set(v,u,x,y,z)		((v)[0]=(u),(v)[1]=(x),(v)[2]=(y),(v)[3]=(z))
#define Vec4Subtract(a,b,c)		((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2],(c)[3]=(a)[3]-(b)[3])

// ===========================================================================

#define Q_rint(x)	((x) < 0 ? ((int)((x)-0.5f)) : ((int)((x)+0.5f)))

float	Q_RSqrtf (float number);
double	Q_RSqrtd (double number);
int		Q_log2 (int val);
void	Q_NearestPow (int *var, qBool oneLess);

float	Q_CalcFovY (float fovX, float width, float height);

// ===========================================================================

void		NormToLatLong (vec3_t normal, byte latLong[2]);
void		MakeNormalVectorsf (vec3_t forward, vec3_t right, vec3_t up);
void		MakeNormalVectorsd (dvec3_t forward, dvec3_t right, dvec3_t up);
void		PerpendicularVector (vec3_t src, vec3_t dst);
void		RotatePointAroundVector (vec3_t dst, vec3_t dir, vec3_t point, float degrees);
float		VectorNormalizef (vec3_t in, vec3_t out);
double		VectorNormalized (dvec3_t in, dvec3_t out);
float		VectorNormalizeFastf (vec3_t v);
double		VectorNormalizeFastd (dvec3_t v);

//
// m_angles.c
//
float		AngleModf (float a);
void		Angles_Matrix3 (vec3_t angles, mat3x3_t axis);
void		Angles_Vectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float		LerpAngle (float a1, float a2, float frac);
void		VecToAngles (vec3_t vec, vec3_t angles);
void		VecToAngleRolled (vec3_t value, float angleYaw, vec3_t angles);
float		VecToYaw (vec3_t vec);

//
// m_bounds.c
//
void		AddBoundsTo2DBounds (vec2_t inMins, vec2_t inMaxs, vec2_t outMins, vec2_t outMaxs);
void		AddPointTo2DBounds (vec2_t v, vec2_t mins, vec2_t maxs);
void		Clear2DBounds (vec2_t mins, vec2_t maxs);

void		AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);
qBool		BoundsAndSphereIntersect (const vec3_t mins, const vec3_t maxs, const vec3_t centre, float radius);
qBool		BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
void		ClearBounds (vec3_t mins, vec3_t maxs);
void		MinMins (vec3_t a, vec3_t b, vec3_t out);
void		MaxMaxs (vec3_t a, vec3_t b, vec3_t out);
float		RadiusFromBounds (vec3_t mins, vec3_t maxs);

//
// m_mat3.c
//
void		Matrix3_Angles (mat3x3_t mat, vec3_t angles);
qBool		Matrix3_Compare (mat3x3_t a, mat3x3_t b);
void		Matrix3_Copy (mat3x3_t in, mat3x3_t out);
void		Matrix3_FromPoints (vec3_t v1, vec3_t v2, vec3_t v3, mat3x3_t m);
void		Matrix3_Identity (mat3x3_t mat);
void		Matrix3_Matrix4 (mat3x3_t in, vec3_t origin, mat4x4_t out);
void		Matrix3_Multiply (mat3x3_t in1, mat3x3_t in2, mat3x3_t out);
void		Matrix3_Quat (mat3x3_t m, quat_t q);
void		Matrix3_Rotate (mat3x3_t a, mat_t angle, mat_t x, mat_t y, mat_t z);
void		Matrix3_TransformVector (mat3x3_t m, vec3_t v, vec3_t out);
void		Matrix3_Transpose (mat3x3_t in, mat3x3_t out);

//
// m_mat4.c
//
qBool		Matrix4_Compare (mat4x4_t a, mat4x4_t b);
void		Matrix4_Copy (mat4x4_t a, mat4x4_t b);
void		Matrix4_Identity (mat4x4_t mat);
void		Matrix4_Matrix3 (mat4x4_t in, mat3x3_t out);
void		Matrix4_Multiply (mat4x4_t a, mat4x4_t b, mat4x4_t product);
void		Matrix4_Multiply_Vec3 (mat4x4_t m, vec3_t v, vec3_t out);
void		Matrix4_Multiply_Vec4 (mat4x4_t m, vec4_t v, vec4_t out);
void		Matrix4_MultiplyFast (mat4x4_t a, mat4x4_t b, mat4x4_t product);
void		Matrix4_MultiplyFast2 (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out);
void		Matrix4_Rotate (mat4x4_t a, float angle, float x, float y, float z);
void		Matrix4_Scale (mat4x4_t m, float x, float y, float z);
void		Matrix4_Translate (mat4x4_t m, float x, float y, float z);
void		Matrix4_Transpose (mat4x4_t m, mat4x4_t ret);

//
// m_quat.c
//
void		Quat_ConcatTransforms (quat_t q1, vec3_t v1, quat_t q2, vec3_t v2, quat_t q, vec3_t v);
void		Quat_Copy (quat_t q1, quat_t q2);
void		Quat_Conjugate (quat_t q1, quat_t q2);
void		Quat_Identity (quat_t q);
mat_t		Quat_Inverse (quat_t q1, quat_t q2);
mat_t		Quat_Normalize (quat_t q);
void		Quat_Lerp (quat_t q1, quat_t q2, mat_t t, quat_t out);
void		Quat_Matrix3 (quat_t q, mat3x3_t m);
void		Quat_Multiply (quat_t q1, quat_t q2, quat_t out);
void		Quat_TransformVector (quat_t q, vec3_t v, vec3_t out);

/*
==============================================================================

	PARSING
 
==============================================================================
*/

#define MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS	256		// max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS		512		// max length of an individual token

char		*Com_Parse (char **dataPtr);
void		Com_DefaultExtension (char *path, char *extension, size_t size);
void		Com_FileBase (char *in, char *out);
void		Com_FileExtension (char *path, char *out, size_t size);
void		Com_FilePath (char *path, char *out, size_t size);
void		Com_NormalizePath (char *dest, size_t size, const char *source);
char		*Com_SkipPath (char *pathname);
void		Com_SkipRestOfLine (char **dataPtr);
char		*Com_SkipWhiteSpace (char *dataPtr, qBool *hasNewLines);
void		Com_StripExtension (char *dest, size_t size, char *src);
void		Com_StripPadding (char *in, char *dest);

/*
==============================================================================

	COLOR STRING HANDLING
 
==============================================================================
*/

extern vec4_t	Q_colorBlack;
extern vec4_t	Q_colorRed;
extern vec4_t	Q_colorGreen;
extern vec4_t	Q_colorYellow;
extern vec4_t	Q_colorBlue;
extern vec4_t	Q_colorCyan;
extern vec4_t	Q_colorMagenta;
extern vec4_t	Q_colorWhite;

extern vec4_t	Q_colorLtGrey;
extern vec4_t	Q_colorMdGrey;
extern vec4_t	Q_colorDkGrey;

extern vec4_t	Q_strColorTable[10];

#define COLOR_ESCAPE	'^'

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define COLOR_GREY		'8'

#define STYLE_ITALIC	'I'
#define STYLE_RETURN	'R'
#define STYLE_SHADOW	'S'

#define S_COLOR_ESCAPE	"^"

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_GREY	"^8"

#define S_STYLE_ITALIC	"^I"
#define S_STYLE_RETURN	"^R"
#define S_STYLE_SHADOW	"^S"

#define Q_StrColorIndex(c)	(((c & 127) - '0') % 9)

qBool		Q_IsColorString (const char *p);
size_t		Q_ColorCharCount (const char *s, size_t endPos);
size_t		Q_ColorCharOffset (const char *s, size_t charCount);
int			Q_ColorStrLastColor (char *s, size_t byteOfs);
int			Q_ColorStrLastStyle (char *s, size_t byteOfs);

#define COLOR_R(rgba)		((rgba) & 0xFF)
#define COLOR_G(rgba)		(((rgba) >> 8) & 0xFF)
#define COLOR_B(rgba)		(((rgba) >> 16) & 0xFF)
#define COLOR_A(rgba)		(((rgba) >> 24) & 0xFF)
#define COLOR_RGB(r,g,b)	(((r) << 0)|((g) << 8)|((b) << 16))
#define COLOR_RGBA(r,g,b,a) (((r) << 0)|((g) << 8)|((b) << 16)|((a) << 24))

/*
==============================================================================

	STRING RELATED FUNCTIONS

==============================================================================
*/

void	Q_snprintfz (char *dest, size_t size, const char *fmt, ...);
void	Q_strcatz (char *dst, const char *src, size_t dstSize);
void	Q_strncpyz (char *dest, const char *src, size_t size);

char	*Q_strlwr (char *s);

// ===========================================================================

int		Q_WildcardMatch (const char *filter, const char *string, int ignoreCase);
char	*Q_VarArgs (char *format, ...);

/*
==============================================================================

	INFO STRINGS

==============================================================================
*/

#define MAX_INFO_KEY		64
#define MAX_INFO_VALUE		64
#define MAX_INFO_STRING		512

void	Info_Print (char *s);
char	*Info_ValueForKey (char *s, char *key);
void	Info_RemoveKey (char *s, char *key);
void	Info_SetValueForKey (char *s, char *key, char *value);
qBool	Info_Validate (char *s);

/*
==============================================================================

	BYTE ORDER FUNCTIONS
 
==============================================================================
*/

float		(*LittleFloat) (float f);
int			(*LittleLong) (int l);
int16		(*LittleShort) (int16 s);
float		(*BigFloat) (float f);
int			(*BigLong) (int l);
int16		(*BigShort) (int16 s);

void		Swap_Init (void);

/*
==============================================================================

	NON-PORTABLE SYSTEM SERVICES

==============================================================================
*/

typedef uint32				fileHandle_t;

#define MAX_QEXT			16		// max length of a quake game pathname extension
#define MAX_QPATH			64		// max length of a quake game pathname
#define MAX_OSPATH			128		// max length of a filesystem pathname

// directory searching
#define SFF_ARCH	0x01
#define SFF_HIDDEN	0x02
#define SFF_RDONLY	0x04
#define SFF_SUBDIR	0x08
#define SFF_SYSTEM	0x10

// these are used for FS_OpenFile
typedef enum fsOpenMode_s {
	FS_MODE_READ_BINARY,
	FS_MODE_WRITE_BINARY,
	FS_MODE_APPEND_BINARY,

	FS_MODE_WRITE_TEXT,
	FS_MODE_APPEND_TEXT
} fsOpenMode_t;

// these are used for FS_Seek
typedef enum fsSeekOrigin_s {
	FS_SEEK_SET,
	FS_SEEK_CUR,
	FS_SEEK_END
} fsSeekOrigin_t;

// for FS_FindFiles
#define FS_MAX_FINDFILES	65536

//
// this is only here so the functions in shared/ can link
//
#define MAX_COMPRINT 4096

// Com_Printf
typedef enum {
	PRNT_WARNING			= 1 << 0,
	PRNT_ERROR				= 1 << 1,
	PRNT_CONSOLE			= 1 << 2,
	PRNT_CHATHUD			= 1 << 3
} comPrint_t;
void	Com_Printf (comPrint_t flags, char *fmt, ...);
void	Com_DevPrintf (comPrint_t flags, char *fmt, ...);

// Com_Error
typedef enum {
	ERR_FATAL,				// exit the entire game with a popup window
	ERR_DROP,				// print to console and disconnect from game
	ERR_DISCONNECT			// don't kill server
} comError_t;
NO_RETURN void	Com_Error (comError_t code, char *fmt, ...);

//
// styles for R_DrawString/Char
//
enum {
	FS_ALIGN_CENTER			= 1 << 0,
	FS_ALIGN_RIGHT			= 1 << 1,
	FS_ITALIC				= 1 << 2,
	FS_SECONDARY			= 1 << 3,
	FS_SHADOW				= 1 << 4,
	FS_SQUARE				= 1 << 5,	// Force the width/height to the character width/height value that's largest
};

/*
==============================================================================

	CVARS

	Console variables
	Do NOT modify struct fields, use the functions
==============================================================================
*/

enum {
	CVAR_ARCHIVE		= 1 << 0,	// saved to config
	CVAR_USERINFO		= 1 << 1,	// added to userinfo  when changed
	CVAR_SERVERINFO		= 1 << 2,	// added to serverinfo when changed
	CVAR_READONLY		= 1 << 3,	// can only be changed when forced through code
	CVAR_LATCH_SERVER	= 1 << 4,	// delay changes until server restart
	CVAR_LATCH_VIDEO	= 1 << 5,	// delay changes until video restart
	CVAR_LATCH_AUDIO	= 1 << 6,	// delay changes until audio restart
	CVAR_RESET_GAMEDIR	= 1 << 7,	// reset game dir when this cvar is modified
	CVAR_CHEAT			= 1 << 8,	// clamp to the default value when cheats are off
};

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cVar_s {
	char			*name;
	char			*string;
	char			*latchedString;	// for CVAR_LATCH vars
	int				flags;
	qBool			modified;		// set each time the cvar is changed
	float			floatVal;

	int				intVal;
	char			*defaultString;

	struct cVar_s	*hashNext;
} cVar_t;

/*
==============================================================================

	CONTENTS/SURFACE FLAGS

==============================================================================
*/

//
// lower bits are stronger, and will eat weaker brushes completely
//
#define CONTENTS_SOLID			1		// an eye is never valid in a solid
#define CONTENTS_WINDOW			2		// translucent, but not watery
#define CONTENTS_AUX			4
#define CONTENTS_LAVA			8
#define CONTENTS_SLIME			16
#define CONTENTS_WATER			32
#define CONTENTS_MIST			64

// Q3BSP
#define CONTENTS_FOG			64
// !Q3BSP

//
// remaining contents are non-visible, and don't eat brushes
//
#define CONTENTS_AREAPORTAL		0x8000

#define CONTENTS_PLAYERCLIP		0x10000
#define CONTENTS_MONSTERCLIP	0x20000

//
// currents can be added to any other contents, and may be mixed
//
#define CONTENTS_CURRENT_0		0x40000
#define CONTENTS_CURRENT_90		0x80000
#define CONTENTS_CURRENT_180	0x100000
#define CONTENTS_CURRENT_270	0x200000
#define CONTENTS_CURRENT_UP		0x400000
#define CONTENTS_CURRENT_DOWN	0x800000

#define CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity
#define CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define CONTENTS_DEADMONSTER	0x4000000
#define CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define CONTENTS_LADDER			0x20000000

// Q3BSP
#define Q3CNTNTS_TELEPORTER		0x40000
#define Q3CNTNTS_JUMPPAD		0x80000
#define Q3CNTNTS_CLUSTERPORTAL	0x100000
#define Q3CNTNTS_DONOTENTER		0x200000

#define Q3CNTNTS_ORIGIN			0x1000000	// removed before bsping an entity

#define Q3CNTNTS_BODY			0x2000000	// should never be on a brush, only in game
#define Q3CNTNTS_CORPSE			0x4000000
#define Q3CNTNTS_DETAIL			0x8000000	// brushes not used for the bsp
#define Q3CNTNTS_STRUCTURAL		0x10000000	// brushes used for the bsp
#define Q3CNTNTS_TRANSLUCENT	0x20000000	// don't consume surface fragments inside
#define Q3CNTNTS_TRIGGER		0x40000000
#define Q3CNTNTS_NODROP			0x80000000	// don't leave bodies or items (death fog, lava)
// !Q3BSP

//
// content masks
//
#define MASK_ALL				(-1)
#define MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)


#define SURF_TEXINFO_LIGHT		0x1		// value will hold the light strength
#define SURF_TEXINFO_SLICK		0x2		// effects game physics
#define SURF_TEXINFO_SKY		0x4		// don't draw, but add to skybox
#define SURF_TEXINFO_WARP		0x8		// turbulent water warp
#define SURF_TEXINFO_TRANS33	0x10
#define SURF_TEXINFO_TRANS66	0x20
#define SURF_TEXINFO_FLOWING	0x40	// scroll towards angle
#define SURF_TEXINFO_NODRAW		0x80	// don't bother referencing the texture

#define SURF_TEXINFO_HINT		0x100	// these aren't known to the engine I believe
#define SURF_TEXINFO_SKIP		0x200	// only the compiler uses them

// Q3BSP
#define SHREF_NODAMAGE			0x1		// never give falling damage
#define SHREF_SLICK				0x2		// effects game physics
#define SHREF_SKY				0x4		// lighting from environment map
#define SHREF_LADDER			0x8
#define SHREF_NOIMPACT			0x10	// don't make missile explosions
#define SHREF_NOMARKS			0x20	// don't leave missile marks
#define SHREF_FLESH				0x40	// make flesh sounds and effects
#define SHREF_NODRAW			0x80	// don't generate a drawsurface at all
#define SHREF_HINT				0x100	// make a primary bsp splitter
#define SHREF_SKIP				0x200	// completely ignore, allowing non-closed brushes
#define SHREF_NOLIGHTMAP		0x400	// surface doesn't need a lightmap
#define SHREF_POINTLIGHT		0x800	// generate lighting info at vertexes
#define SHREF_METALSTEPS		0x1000	// clanking footsteps
#define SHREF_NOSTEPS			0x2000	// no footstep sounds
#define SHREF_NONSOLID			0x4000	// don't collide against curves with this set
#define SHREF_LIGHTFILTER		0x8000	// act as a light filter during q3map -light
#define SHREF_ALPHASHADOW		0x10000	// do per-pixel light shadow casting in q3map
#define SHREF_NODLIGHT			0x20000	// never add dynamic lights
#define SHREF_DUST				0x40000 // leave a dust trail when walking on this surface
// !Q3BSP

//
// gi.BoxEdicts() can return a list of either solid or trigger entities
//
#define AREA_SOLID				1
#define AREA_TRIGGERS			2

/*
==============================================================================

	CMODEL/PLANE

==============================================================================
*/

// 0-2 are axial planes
#define PLANE_X			0
#define PLANE_Y			1
#define PLANE_Z			2
#define PLANE_NON_AXIAL	3

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX		3
#define PLANE_ANYY		4
#define PLANE_ANYZ		5

typedef struct cBspPlane_s {
	vec3_t			normal;
	float			dist;
	byte			type;			// for fast side tests
	byte			signBits;		// signx + (signy<<1) + (signz<<1)
} cBspPlane_t;

typedef struct cBspSurface_s {
	char			name[16];
	int				flags;
	int				value;

	// Q3BSP
	int				contents;
	char			rname[MAX_QPATH];
} cBspSurface_t;

// A trace is returned when a box is swept through the world
typedef struct trace_s {
	qBool			allSolid;	// if true, plane is not valid
	qBool			startSolid;	// if true, the initial point was in a solid area
	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endPos;		// final position
	cBspPlane_t		plane;		// surface normal at impact
	cBspSurface_t	*surface;	// surface hit
	int				contents;	// contents on other side of surface hit
	struct edict_s	*ent;		// not set by CM_*() functions
} trace_t;

//
// m_plane.c
//
#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)
#define BOX_ON_PLANE_SIDE(mins, maxs, p)			\
	(((p)->type < 3)? (								\
		((p)->dist <= (mins)[(p)->type])? 1 :		\
		(											\
			((p)->dist >= (maxs)[(p)->type])? 2 : 3	\
		)											\
	) : BoxOnPlaneSide((mins), (maxs), (p)))

int		BoxOnPlaneSide (vec3_t mins, vec3_t maxs, cBspPlane_t *plane);
int		PlaneTypeForNormal (vec3_t normal);
void	CategorizePlane (cBspPlane_t *plane);
void	PlaneFromPoints (vec3_t verts[3], cBspPlane_t *plane);
qBool	ComparePlanes (const vec3_t p1normal, float p1dist, const vec3_t p2normal, float p2dist);
void	SnapVector (vec3_t normal);
void	ProjectPointOnPlane (vec3_t dst, vec3_t p, vec3_t normal);
int		SignbitsForPlane (cBspPlane_t *out);

/*
==============================================================================

	PREDICTION

==============================================================================
*/

// pMoveState_t is the information necessary for client side movement prediction
enum {
	// can accelerate and turn
	PMT_NORMAL,
	PMT_SPECTATOR,
	// no acceleration or turning
	PMT_DEAD,
	PMT_GIB,		// different bounding box
	PMT_FREEZE
};

// pmove->pmFlags
enum {
	PMF_DUCKED			= 1 << 0,
	PMF_JUMP_HELD		= 1 << 1,
	PMF_ON_GROUND		= 1 << 2,
	PMF_TIME_WATERJUMP	= 1 << 3,	// pm_time is waterjump
	PMF_TIME_LAND		= 1 << 4,	// pm_time is time before rejump
	PMF_TIME_TELEPORT	= 1 << 5,	// pm_time is non-moving time
	PMF_NO_PREDICTION	= 1 << 6	// temporarily disables prediction (used for grappling hook)
};

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct pMoveState_s {
	int				pmType;

	int16			origin[3];		// 12.3
	int16			velocity[3];	// 12.3
	byte			pmFlags;		// ducked, jump_held, etc
	byte			pmTime;			// each unit = 8 ms
	int16			gravity;
	int16			deltaAngles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
} pMoveState_t;

//
// button bits
//
#define BUTTON_ATTACK		1
#define BUTTON_USE			2

//stolen for r1q2 in the name of bandwidth
#define	BUTTON_UCMD_DBLFORWARD	4
#define BUTTON_UCMD_DBLSIDE		8
#define	BUTTON_UCMD_DBLUP		16

#define BUTTON_UCMD_DBL_ANGLE1	32
#define BUTTON_UCMD_DBL_ANGLE2	64

#define BUTTON_ANY			128			// any key whatsoever

// userCmd_t is sent to the server each client frame
typedef struct userCmd_s {
	byte		msec;
	byte		buttons;

	int16		angles[3];

	int16		forwardMove;
	int16		sideMove;
	int16		upMove;

	byte		impulse;		// remove?
	byte		lightLevel;		// light level the player is standing on
} userCmd_t;


#define MAXTOUCH	32
typedef struct pMove_s {
	// state (in / out)
	pMoveState_t	state;

	// command (in)
	userCmd_t		cmd;
	qBool			snapInitial;	// if s has been changed outside pmove

	// results (out)
	int				numTouch;
	struct edict_s	*touchEnts[MAXTOUCH];

	vec3_t			viewAngles;			// clamped
	float			viewHeight;

	vec3_t			mins, maxs;			// bounding box size

	struct edict_s	*groundEntity;
	int				waterType;
	int				waterLevel;

	// callbacks to test the world
	trace_t			(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int				(*pointContents) (vec3_t point);
} pMove_t;

typedef struct pMoveNew_s {
	// state (in / out)
	pMoveState_t	state;

	// command (in)
	userCmd_t		cmd;
	qBool			snapInitial;	// if s has been changed outside pmove

	// results (out)
	int				numTouch;
	struct edict_s	*touchEnts[MAXTOUCH];

	vec3_t			viewAngles;			// clamped
	float			viewHeight;

	vec3_t			mins, maxs;			// bounding box size

	struct edict_s	*groundEntity;
	int				waterType;
	int				waterLevel;

	// callbacks to test the world
	trace_t			(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int				(*pointContents) (vec3_t point);

	float			multiplier;
	qBool			strafeHack;
	qBool			step;
} pMoveNew_t;

/*
==============================================================================

	ENTITY FX

==============================================================================
*/

// entityState_t->effects
// Effects are things handled on the client side (lights, particles, frame
// animations) that happen constantly on the given entity. An entity that has
// effects will be sent to the client even if it has a zero index model.
#define EF_ROTATE			0x00000001		// rotate (bonus items)
#define EF_GIB				0x00000002		// leave a trail
#define EF_BLASTER			0x00000008		// redlight + trail
#define EF_ROCKET			0x00000010		// redlight + trail
#define EF_GRENADE			0x00000020
#define EF_HYPERBLASTER		0x00000040
#define EF_BFG				0x00000080
#define EF_COLOR_SHELL		0x00000100
#define EF_POWERSCREEN		0x00000200
#define EF_ANIM01			0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define EF_ANIM23			0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define EF_FLIES			0x00004000
#define EF_QUAD				0x00008000
#define EF_PENT				0x00010000
#define EF_TELEPORTER		0x00020000		// particle fountain
#define EF_FLAG1			0x00040000
#define EF_FLAG2			0x00080000

// RAFAEL
#define EF_IONRIPPER		0x00100000
#define EF_GREENGIB			0x00200000
#define EF_BLUEHYPERBLASTER 0x00400000
#define EF_SPINNINGLIGHTS	0x00800000
#define EF_PLASMA			0x01000000
#define EF_TRAP				0x02000000

// ROGUE
#define EF_TRACKER			0x04000000
#define EF_DOUBLE			0x08000000
#define EF_SPHERETRANS		0x10000000
#define EF_TAGTRAIL			0x20000000
#define EF_HALF_DAMAGE		0x40000000
#define EF_TRACKERTRAIL		0x80000000

/*
==============================================================================

	RENDERFX

==============================================================================
*/

// entityState_t->renderfx flags
#define RF_MINLIGHT			1		// allways have some light (viewmodel)
#define RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define RF_WEAPONMODEL		4		// only draw through eyes
#define RF_FULLBRIGHT		8		// allways draw full intensity
#define RF_DEPTHHACK		16		// for view weapon Z crunching
#define RF_TRANSLUCENT		32
#define RF_FRAMELERP		64
#define RF_BEAM				128
#define RF_CUSTOMSKIN		256		// skin is an index in image_precache
#define RF_GLOW				512		// pulse lighting for bonus items

#define RF_SHELL_RED		1024
#define RF_SHELL_GREEN		2048
#define RF_SHELL_BLUE		4096

#define RF_IR_VISIBLE		0x00008000		// 32768
#define RF_SHELL_DOUBLE		0x00010000		// 65536
#define RF_SHELL_HALF_DAM	0x00020000
#define RF_USE_DISGUISE		0x00040000

#define RF_NOSHADOW			0x00080000
#define RF_CULLHACK			0x00100000
#define RF_FORCENOLOD		0x00200000
#define RF_SHELLMASK		(RF_SHELL_HALF_DAM|RF_SHELL_DOUBLE|RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE)

/*
==============================================================================

	MUZZLE FLASHES

==============================================================================
*/

// muzzle flashes / player effects
enum {
	MZ_BLASTER,
	MZ_MACHINEGUN,
	MZ_SHOTGUN,
	MZ_CHAINGUN1,
	MZ_CHAINGUN2,
	MZ_CHAINGUN3,
	MZ_RAILGUN,
	MZ_ROCKET,
	MZ_GRENADE,
	MZ_LOGIN,
	MZ_LOGOUT,
	MZ_RESPAWN,
	MZ_BFG,
	MZ_SSHOTGUN,
	MZ_HYPERBLASTER,
	MZ_ITEMRESPAWN,

	// RAFAEL
	MZ_IONRIPPER,
	MZ_BLUEHYPERBLASTER,
	MZ_PHALANX,
	MZ_SILENCED			= 128,		// bit flag ORed with one of the above numbers

	// ROGUE
	MZ_ETF_RIFLE		= 30,
	MZ_UNUSED,
	MZ_SHOTGUN2,
	MZ_HEATBEAM,
	MZ_BLASTER2,
	MZ_TRACKER,
	MZ_NUKE1,
	MZ_NUKE2,
	MZ_NUKE4,
	MZ_NUKE8
	// ROGUE
};

// monster muzzle flashes

extern	vec3_t dumb_and_hacky_monster_MuzzFlashOffset [];

enum {
	MZ2_TANK_BLASTER_1				= 1,
	MZ2_TANK_BLASTER_2,
	MZ2_TANK_BLASTER_3,
	MZ2_TANK_MACHINEGUN_1,
	MZ2_TANK_MACHINEGUN_2,
	MZ2_TANK_MACHINEGUN_3,
	MZ2_TANK_MACHINEGUN_4,
	MZ2_TANK_MACHINEGUN_5,
	MZ2_TANK_MACHINEGUN_6,
	MZ2_TANK_MACHINEGUN_7,
	MZ2_TANK_MACHINEGUN_8,
	MZ2_TANK_MACHINEGUN_9,
	MZ2_TANK_MACHINEGUN_10,
	MZ2_TANK_MACHINEGUN_11,
	MZ2_TANK_MACHINEGUN_12,
	MZ2_TANK_MACHINEGUN_13,
	MZ2_TANK_MACHINEGUN_14,
	MZ2_TANK_MACHINEGUN_15,
	MZ2_TANK_MACHINEGUN_16,
	MZ2_TANK_MACHINEGUN_17,
	MZ2_TANK_MACHINEGUN_18,
	MZ2_TANK_MACHINEGUN_19,
	MZ2_TANK_ROCKET_1,
	MZ2_TANK_ROCKET_2,
	MZ2_TANK_ROCKET_3,

	MZ2_INFANTRY_MACHINEGUN_1,
	MZ2_INFANTRY_MACHINEGUN_2,
	MZ2_INFANTRY_MACHINEGUN_3,
	MZ2_INFANTRY_MACHINEGUN_4,
	MZ2_INFANTRY_MACHINEGUN_5,
	MZ2_INFANTRY_MACHINEGUN_6,
	MZ2_INFANTRY_MACHINEGUN_7,
	MZ2_INFANTRY_MACHINEGUN_8,
	MZ2_INFANTRY_MACHINEGUN_9,
	MZ2_INFANTRY_MACHINEGUN_10,
	MZ2_INFANTRY_MACHINEGUN_11,
	MZ2_INFANTRY_MACHINEGUN_12,
	MZ2_INFANTRY_MACHINEGUN_13,

	MZ2_SOLDIER_BLASTER_1,
	MZ2_SOLDIER_BLASTER_2,
	MZ2_SOLDIER_SHOTGUN_1,
	MZ2_SOLDIER_SHOTGUN_2,
	MZ2_SOLDIER_MACHINEGUN_1,
	MZ2_SOLDIER_MACHINEGUN_2,

	MZ2_GUNNER_MACHINEGUN_1,
	MZ2_GUNNER_MACHINEGUN_2,
	MZ2_GUNNER_MACHINEGUN_3,
	MZ2_GUNNER_MACHINEGUN_4,
	MZ2_GUNNER_MACHINEGUN_5,
	MZ2_GUNNER_MACHINEGUN_6,
	MZ2_GUNNER_MACHINEGUN_7,
	MZ2_GUNNER_MACHINEGUN_8,
	MZ2_GUNNER_GRENADE_1,
	MZ2_GUNNER_GRENADE_2,
	MZ2_GUNNER_GRENADE_3,
	MZ2_GUNNER_GRENADE_4,

	MZ2_CHICK_ROCKET_1,

	MZ2_FLYER_BLASTER_1,
	MZ2_FLYER_BLASTER_2,

	MZ2_MEDIC_BLASTER_1,

	MZ2_GLADIATOR_RAILGUN_1,

	MZ2_HOVER_BLASTER_1,

	MZ2_ACTOR_MACHINEGUN_1,

	MZ2_SUPERTANK_MACHINEGUN_1,
	MZ2_SUPERTANK_MACHINEGUN_2,
	MZ2_SUPERTANK_MACHINEGUN_3,
	MZ2_SUPERTANK_MACHINEGUN_4,
	MZ2_SUPERTANK_MACHINEGUN_5,
	MZ2_SUPERTANK_MACHINEGUN_6,
	MZ2_SUPERTANK_ROCKET_1,
	MZ2_SUPERTANK_ROCKET_2,
	MZ2_SUPERTANK_ROCKET_3,

	MZ2_BOSS2_MACHINEGUN_L1,
	MZ2_BOSS2_MACHINEGUN_L2,
	MZ2_BOSS2_MACHINEGUN_L3,
	MZ2_BOSS2_MACHINEGUN_L4,
	MZ2_BOSS2_MACHINEGUN_L5,
	MZ2_BOSS2_ROCKET_1,
	MZ2_BOSS2_ROCKET_2,
	MZ2_BOSS2_ROCKET_3,
	MZ2_BOSS2_ROCKET_4,

	MZ2_FLOAT_BLASTER_1,

	MZ2_SOLDIER_BLASTER_3,
	MZ2_SOLDIER_SHOTGUN_3,
	MZ2_SOLDIER_MACHINEGUN_3,
	MZ2_SOLDIER_BLASTER_4,
	MZ2_SOLDIER_SHOTGUN_4,
	MZ2_SOLDIER_MACHINEGUN_4,
	MZ2_SOLDIER_BLASTER_5,
	MZ2_SOLDIER_SHOTGUN_5,
	MZ2_SOLDIER_MACHINEGUN_5,
	MZ2_SOLDIER_BLASTER_6,
	MZ2_SOLDIER_SHOTGUN_6,
	MZ2_SOLDIER_MACHINEGUN_6,
	MZ2_SOLDIER_BLASTER_7,
	MZ2_SOLDIER_SHOTGUN_7,
	MZ2_SOLDIER_MACHINEGUN_7,
	MZ2_SOLDIER_BLASTER_8,
	MZ2_SOLDIER_SHOTGUN_8,
	MZ2_SOLDIER_MACHINEGUN_8,

	// --- Xian shit below ---
	MZ2_MAKRON_BFG,
	MZ2_MAKRON_BLASTER_1,
	MZ2_MAKRON_BLASTER_2,
	MZ2_MAKRON_BLASTER_3,
	MZ2_MAKRON_BLASTER_4,
	MZ2_MAKRON_BLASTER_5,
	MZ2_MAKRON_BLASTER_6,
	MZ2_MAKRON_BLASTER_7,
	MZ2_MAKRON_BLASTER_8,
	MZ2_MAKRON_BLASTER_9,
	MZ2_MAKRON_BLASTER_10,
	MZ2_MAKRON_BLASTER_11,
	MZ2_MAKRON_BLASTER_12,
	MZ2_MAKRON_BLASTER_13,
	MZ2_MAKRON_BLASTER_14,
	MZ2_MAKRON_BLASTER_15,
	MZ2_MAKRON_BLASTER_16,
	MZ2_MAKRON_BLASTER_17,
	MZ2_MAKRON_RAILGUN_1,
	MZ2_JORG_MACHINEGUN_L1,
	MZ2_JORG_MACHINEGUN_L2,
	MZ2_JORG_MACHINEGUN_L3,
	MZ2_JORG_MACHINEGUN_L4,
	MZ2_JORG_MACHINEGUN_L5,
	MZ2_JORG_MACHINEGUN_L6,
	MZ2_JORG_MACHINEGUN_R1,
	MZ2_JORG_MACHINEGUN_R2,
	MZ2_JORG_MACHINEGUN_R3,
	MZ2_JORG_MACHINEGUN_R4,
	MZ2_JORG_MACHINEGUN_R5,
	MZ2_JORG_MACHINEGUN_R6,
	MZ2_JORG_BFG_1,
	MZ2_BOSS2_MACHINEGUN_R1,
	MZ2_BOSS2_MACHINEGUN_R2,
	MZ2_BOSS2_MACHINEGUN_R3,
	MZ2_BOSS2_MACHINEGUN_R4,
	MZ2_BOSS2_MACHINEGUN_R5,

	// ROGUE
	MZ2_CARRIER_MACHINEGUN_L1,
	MZ2_CARRIER_MACHINEGUN_R1,
	MZ2_CARRIER_GRENADE,
	MZ2_TURRET_MACHINEGUN,
	MZ2_TURRET_ROCKET,
	MZ2_TURRET_BLASTER,
	MZ2_STALKER_BLASTER,
	MZ2_DAEDALUS_BLASTER,
	MZ2_MEDIC_BLASTER_2,
	MZ2_CARRIER_RAILGUN,
	MZ2_WIDOW_DISRUPTOR,
	MZ2_WIDOW_BLASTER,
	MZ2_WIDOW_RAIL,
	MZ2_WIDOW_PLASMABEAM,		// PMM - not used
	MZ2_CARRIER_MACHINEGUN_L2,
	MZ2_CARRIER_MACHINEGUN_R2,
	MZ2_WIDOW_RAIL_LEFT,
	MZ2_WIDOW_RAIL_RIGHT,
	MZ2_WIDOW_BLASTER_SWEEP1,
	MZ2_WIDOW_BLASTER_SWEEP2,
	MZ2_WIDOW_BLASTER_SWEEP3,
	MZ2_WIDOW_BLASTER_SWEEP4,
	MZ2_WIDOW_BLASTER_SWEEP5,
	MZ2_WIDOW_BLASTER_SWEEP6,
	MZ2_WIDOW_BLASTER_SWEEP7,
	MZ2_WIDOW_BLASTER_SWEEP8,
	MZ2_WIDOW_BLASTER_SWEEP9,
	MZ2_WIDOW_BLASTER_100,
	MZ2_WIDOW_BLASTER_90,
	MZ2_WIDOW_BLASTER_80,
	MZ2_WIDOW_BLASTER_70,
	MZ2_WIDOW_BLASTER_60,
	MZ2_WIDOW_BLASTER_50,
	MZ2_WIDOW_BLASTER_40,
	MZ2_WIDOW_BLASTER_30,
	MZ2_WIDOW_BLASTER_20,
	MZ2_WIDOW_BLASTER_10,
	MZ2_WIDOW_BLASTER_0,
	MZ2_WIDOW_BLASTER_10L,
	MZ2_WIDOW_BLASTER_20L,
	MZ2_WIDOW_BLASTER_30L,
	MZ2_WIDOW_BLASTER_40L,
	MZ2_WIDOW_BLASTER_50L,
	MZ2_WIDOW_BLASTER_60L,
	MZ2_WIDOW_BLASTER_70L,
	MZ2_WIDOW_RUN_1,
	MZ2_WIDOW_RUN_2,
	MZ2_WIDOW_RUN_3,
	MZ2_WIDOW_RUN_4,
	MZ2_WIDOW_RUN_5,
	MZ2_WIDOW_RUN_6,
	MZ2_WIDOW_RUN_7,
	MZ2_WIDOW_RUN_8,
	MZ2_CARRIER_ROCKET_1,
	MZ2_CARRIER_ROCKET_2,
	MZ2_CARRIER_ROCKET_3,
	MZ2_CARRIER_ROCKET_4,
	MZ2_WIDOW2_BEAMER_1,
	MZ2_WIDOW2_BEAMER_2,
	MZ2_WIDOW2_BEAMER_3,
	MZ2_WIDOW2_BEAMER_4,
	MZ2_WIDOW2_BEAMER_5,
	MZ2_WIDOW2_BEAM_SWEEP_1,
	MZ2_WIDOW2_BEAM_SWEEP_2,
	MZ2_WIDOW2_BEAM_SWEEP_3,
	MZ2_WIDOW2_BEAM_SWEEP_4,
	MZ2_WIDOW2_BEAM_SWEEP_5,
	MZ2_WIDOW2_BEAM_SWEEP_6,
	MZ2_WIDOW2_BEAM_SWEEP_7,
	MZ2_WIDOW2_BEAM_SWEEP_8,
	MZ2_WIDOW2_BEAM_SWEEP_9,
	MZ2_WIDOW2_BEAM_SWEEP_10,
	MZ2_WIDOW2_BEAM_SWEEP_11
	// ROGUE
};

/*
==============================================================================

	TEMP ENTITY EVENTS

==============================================================================
*/

// Temp entity events are for things that happen at a location seperate from
// any existing entity. Temporary entity messages are explicitly constructed
// and broadcast.
enum {
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,

	//ROGUE
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE
	//ROGUE
};

// TE_SPLASH effects
enum {
	SPLASH_UNKNOWN,
	SPLASH_SPARKS,
	SPLASH_BLUE_WATER,
	SPLASH_BROWN_WATER,
	SPLASH_SLIME,
	SPLASH_LAVA,
	SPLASH_BLOOD
};

/*
==============================================================================

	SOUND

==============================================================================
*/

//
// sound channels
// channel 0 never willingly overrides other channels (1-7) allways override
// a playing sound on that channel
//
typedef enum entChannel_s {
	CHAN_AUTO,
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,

	// modifier flags
	CHAN_NO_PHS_ADD		= 8,	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
	CHAN_RELIABLE		= 16	// send by reliable message, not datagram
} entChannel_t;

//
// sound attenuation values
//
enum {
	ATTN_NONE,				// full volume the entire level
	ATTN_NORM,
	ATTN_IDLE,
	ATTN_STATIC				// diminish very rapidly with distance
};

/*
==============================================================================

	DEATHMATCH FLAGS

==============================================================================
*/

// dmflags->floatVal flags
#define DF_NO_HEALTH		0x00000001	// 1
#define DF_NO_ITEMS			0x00000002	// 2
#define DF_WEAPONS_STAY		0x00000004	// 4
#define DF_NO_FALLING		0x00000008	// 8
#define DF_INSTANT_ITEMS	0x00000010	// 16
#define DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

#define DF_QUADFIRE_DROP	0x00010000	// 65536

#define DF_NO_MINES			0x00020000
#define DF_NO_STACK_DOUBLE	0x00040000
#define DF_NO_NUKES			0x00080000
#define DF_NO_SPHERES		0x00100000

/*
==============================================================================

	CONFIG STRINGS

==============================================================================
*/

// per-level limits
#define MAX_CS_CLIENTS		256		// absolute limit
#define MAX_CS_EDICTS		1024	// must change protocol to increase more
#define MAX_CS_LIGHTSTYLES	256
#define MAX_CS_MODELS		256		// these are sent over the net as bytes
#define MAX_CS_SOUNDS		256		// so they cannot be blindly increased
#define MAX_CS_IMAGES		256
#define MAX_CS_ITEMS		256
#define MAX_CS_GENERAL		(MAX_CS_CLIENTS*2)	// general config strings

#define Q2BSP_MAX_AREAS		256
#define MAX_AREA_BITS		(Q2BSP_MAX_AREAS/8)

// config strings are a general means of communication from the server to all
// connected clients. Each config string can be at most MAX_CFGSTRLEN characters.
#define CS_NAME				0
#define CS_CDTRACK			1
#define CS_SKY				2
#define CS_SKYAXIS			3		// %f %f %f format
#define CS_SKYROTATE		4
#define CS_STATUSBAR		5		// display program string

#define CS_AIRACCEL			29		// air acceleration control
#define CS_MAXCLIENTS		30
#define CS_MAPCHECKSUM		31		// for catching cheater maps

#define CS_MODELS			32
#define CS_SOUNDS			(CS_MODELS+MAX_CS_MODELS)
#define CS_IMAGES			(CS_SOUNDS+MAX_CS_SOUNDS)
#define CS_LIGHTS			(CS_IMAGES+MAX_CS_IMAGES)
#define CS_ITEMS			(CS_LIGHTS+MAX_CS_LIGHTSTYLES)
#define CS_PLAYERSKINS		(CS_ITEMS+MAX_CS_ITEMS)
#define CS_GENERAL			(CS_PLAYERSKINS+MAX_CS_CLIENTS)

#define MAX_CFGSTRINGS		(CS_GENERAL+MAX_CS_GENERAL)
#define MAX_CFGSTRLEN		64

/*
==============================================================================

	ENTITY STATE

==============================================================================
*/

// entityState_t->event values
// ertity events are for effects that take place reletive to an existing
// entities origin.  Very network efficient. All muzzle flashes really should
// be converted to events...
enum {
	EV_NONE				= 0,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
};

// entityState_t is the information conveyed from the server in an update
// message about entities that the client will need to render in some way
typedef struct entityState_s {
	int				number;		// edict index

	vec3_t			origin;		// entity origin or RF_BEAM start origin
	vec3_t			angles;
	vec3_t			oldOrigin;	// for interpolation or RF_BEAM end origin

	// weapons, CTF flags, etc
	int				modelIndex;
	int				modelIndex2;
	int				modelIndex3;
	int				modelIndex4;

	int				frame;		// also RF_BEAM's size
	int				skinNum;	// also RF_BEAM color index

	uint32			effects;	// PGM - we're filling it, so it needs to be uint32
	int				renderFx;
	int				solid;		// for client side prediction, 8*(bits 0-4) is x/y radius
								// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
								// gi.linkentity sets this properly
	int				sound;		// for looping sounds, to guarantee shutoff
	int				event;		// impulse events -- muzzle flashes, footsteps, etc
								// events only go out for a single frame, they
								// are automatically cleared each frame
	vec3_t			velocity;	// for new ENHANCED_PROTOCOL_VERSION
} entityState_t;

typedef struct entityStateOld_s {
	int				number;		// edict index

	vec3_t			origin;		// entity origin or RF_BEAM start origin
	vec3_t			angles;
	vec3_t			oldOrigin;	// for interpolation or RF_BEAM end origin

	// weapons, CTF flags, etc
	int				modelIndex;
	int				modelIndex2;
	int				modelIndex3;
	int				modelIndex4;

	int				frame;		// also RF_BEAM's size
	int				skinNum;	// also RF_BEAM color index

	uint32			effects;	// PGM - we're filling it, so it needs to be uint32
	int				renderFx;
	int				solid;		// for client side prediction, 8*(bits 0-4) is x/y radius
								// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
								// gi.linkentity sets this properly
	int				sound;		// for looping sounds, to guarantee shutoff
	int				event;		// impulse events -- muzzle flashes, footsteps, etc
								// events only go out for a single frame, they
								// are automatically cleared each frame
} entityStateOld_t;

/*
==============================================================================

	PLAYER STATE

==============================================================================
*/

// playerState->stats[] indexes
enum {
	STAT_HEALTH_ICON		= 0,
	STAT_HEALTH,
	STAT_AMMO_ICON,
	STAT_AMMO,
	STAT_ARMOR_ICON,
	STAT_ARMOR,
	STAT_SELECTED_ICON,
	STAT_PICKUP_ICON,
	STAT_PICKUP_STRING,
	STAT_TIMER_ICON,
	STAT_TIMER,
	STAT_HELPICON,
	STAT_SELECTED_ITEM,
	STAT_LAYOUTS,
	STAT_FRAGS,
	STAT_FLASHES,					// cleared each frame, 1 = health, 2 = armor
	STAT_CHASE,
	STAT_SPECTATOR,

	MAX_STATS				= 32
};

// playerState_t->rdFlags
enum {
	RDF_UNDERWATER		= 1 << 0,		// warp the screen as apropriate
	RDF_NOWORLDMODEL	= 1 << 1,		// used for player configuration screen
	RDF_IRGOGGLES		= 1 << 2,
	RDF_UVGOGGLES		= 1 << 3,

	RDF_OLDAREABITS		= 1 << 4,
};

// playerState_t is the information needed in addition to pMoveState_t to
// rendered a view.  There will only be 10 playerState_t sent each second, but
// the number of pMoveState_t changes will be reletive to client frame rates
typedef struct playerStateNew_s {
	pMoveState_t	pMove;				// for prediction

	// these fields do not need to be communicated bit-precise
	vec3_t			viewAngles;			// for fixed views
	vec3_t			viewOffset;			// add to pmovestate->origin
	vec3_t			kickAngles;			// add to view direction to get render angles
										// set by weapon kicks, pain effects, etc
	vec3_t			gunAngles;
	vec3_t			gunOffset;
	int				gunIndex;
	int				gunFrame;

	float			viewBlend[4];		// rgba full screen effect
	
	float			fov;				// horizontal field of view

	int				rdFlags;			// refdef flags

	int16			stats[MAX_STATS];	// fast status bar updates

	vec3_t			mins;
	vec3_t			maxs;
} playerStateNew_t;

typedef struct playerState_s {
	pMoveState_t	pMove;				// for prediction

	// these fields do not need to be communicated bit-precise
	vec3_t			viewAngles;			// for fixed views
	vec3_t			viewOffset;			// add to pmovestate->origin
	vec3_t			kickAngles;			// add to view direction to get render angles
										// set by weapon kicks, pain effects, etc
	vec3_t			gunAngles;
	vec3_t			gunOffset;
	int				gunIndex;
	int				gunFrame;

	float			viewBlend[4];		// rgba full screen effect
	
	float			fov;				// horizontal field of view

	int				rdFlags;			// refdef flags

	int16			stats[MAX_STATS];	// fast status bar updates
} playerState_t;

#endif // __SHARED_H__
