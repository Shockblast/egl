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
// ui_local.h
//

#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../shared/shared.h"
#include "../cgame/cgref.h"
#include "ui_api.h"
#include "ui_common.h"
#include "keycodes.h"

extern uiImport_t uii;

/*
==================================================================

	API

==================================================================
*/

#define UI_MemAlloc(size)	uii.Mem_Alloc((size),__FILE__,__LINE__)
#define UI_MemFree(ptr)		uii.Mem_Free((ptr),__FILE__,__LINE__)

/*
=============================================================================

	UI STATE

=============================================================================
*/

typedef struct uiState_s {
	int			time;

	int			vidWidth;
	int			vidHeight;

	int			cursorX;
	int			cursorY;

	int			connectState;
	int			serverState;
} uiState_t;

extern uiState_t	uis;

/*
=============================================================================

	UI MEDIA

=============================================================================
*/

// sounds
typedef struct uiSoundMedia_s {
	struct sfx_s	*menuIn;
	struct sfx_s	*menuMove;
	struct sfx_s	*menuOut;
} uiSoundMedia_t;

// menu banners
typedef struct uiBannerMedia_s {
	struct image_s	*addressBook;
	struct image_s	*multiplayer;
	struct image_s	*startServer;
	struct image_s	*joinServer;
	struct image_s	*options;
	struct image_s	*game;
	struct image_s	*loadGame;
	struct image_s	*saveGame;
	struct image_s	*video;
	struct image_s	*quit;
} uiBannerMedia_t;

// menu media
#define MAINMENU_CURSOR_NUMFRAMES	15
typedef struct uiMenuMedia_s {
	struct image_s	*mainCursors[MAINMENU_CURSOR_NUMFRAMES];
	struct image_s	*mainPlaque;
	struct image_s	*mainLogo;

	struct image_s	*mainGame;
	struct image_s	*mainMultiplayer;
	struct image_s	*mainOptions;
	struct image_s	*mainVideo;
	struct image_s	*mainQuit;

	struct image_s	*mainGameSel;
	struct image_s	*mainMultiplayerSel;
	struct image_s	*mainOptionsSel;
	struct image_s	*mainVideoSel;
	struct image_s	*mainQuitSel;
} uiMenuMedia_t;

// ==========================================================================

typedef struct uiMedia_s {
	qBool				initialized;

	// sounds
	uiSoundMedia_t		sounds;

	// generic textures
	struct image_s		*whiteTexture;

	// chars image
	struct image_s		*uiCharsImage;

	// background images
	struct image_s		*bgBig;

	// cursor image
	struct image_s		*cursorImage;

	// menu items
	uiBannerMedia_t		banners;
	uiMenuMedia_t		menus;
} uiMedia_t;

extern uiMedia_t	uiMedia;

/*
=============================================================================

	MACROS

=============================================================================
*/

#define MAXMENUITEMS		128

//
// types
//
enum {
	UITYPE_ACTION,
	UITYPE_FIELD,
	UITYPE_IMAGE,
	UITYPE_SLIDER,
	UITYPE_SPINCONTROL
};

//
// flags
//
enum {
	UIF_LEFT_JUSTIFY	= 1 << 0,
	UIF_CENTERED		= 1 << 1,

	UIF_NUMBERSONLY		= 1 << 2,

	UIF_SHADOW			= 1 << 3,

	UIF_MEDIUM			= 1 << 4,
	UIF_LARGE			= 1 << 5,
	UIF_NOTOOLTIP		= 1 << 6,

	UIF_NOSELECT		= 1 << 7,
	UIF_DBLCLICK		= 1 << 8,
	UIF_SELONLY			= 1 << 9,
	UIF_NOSELBAR		= 1 << 10,
	UIF_FORCESELBAR		= 1 << 11
};

//
// misc
//
#define	MAX_UI_DEPTH		12

#define RCOLUMN_OFFSET		(UIFT_SIZE*2)
#define LCOLUMN_OFFSET		(-(RCOLUMN_OFFSET))

#define MAX_LOCAL_SERVERS	24
#define MAX_ADDRBOOK_SAVES	8
#define	MAX_SAVEGAMES		16

#define SLIDER_RANGE		10

/*
=============================================================================

	SCALING

	Generally we scale to 640x480
=============================================================================
*/

#define UI_HSCALE			((float)(uis.vidWidth / 640.0))
#define UI_VSCALE			((float)(uis.vidHeight / 480.0))

#define UI_SCALE			((float)(uis.vidWidth / 640.0))

#define UIFT_SCALE			(UI_SCALE)
#define UIFT_SIZE			(8 * UIFT_SCALE)
#define UIFT_SIZEINC		(UIFT_SIZE + FT_SHAOFFSET)

#define UIFT_SCALEMED		(UI_SCALE*1.25)
#define UIFT_SIZEMED		(8 * UIFT_SCALEMED)
#define UIFT_SIZEINCMED		(UIFT_SIZEMED + FT_SHAOFFSET)

#define UIFT_SCALELG		(UI_SCALE*1.5)
#define UIFT_SIZELG			(8 * UIFT_SCALELG)
#define UIFT_SIZEINCLG		(UIFT_SIZELG + FT_SHAOFFSET)

#define UISCALE_TYPE(flags)		((flags&UIF_LARGE) ? UIFT_SCALELG	: (flags&UIF_MEDIUM) ? UIFT_SCALEMED	: UIFT_SCALE)
#define UISIZE_TYPE(flags)		((flags&UIF_LARGE) ? UIFT_SIZELG	: (flags&UIF_MEDIUM) ? UIFT_SIZEMED		: UIFT_SIZE)
#define UISIZEINC_TYPE(flags)	((flags&UIF_LARGE) ? UIFT_SIZEINCLG	: (flags&UIF_MEDIUM) ? UIFT_SIZEINCMED	: UIFT_SIZEINC)

/*
=============================================================================

	STRUCTS

=============================================================================
*/

typedef struct uiFrameWork_s {
	float					x, y;
	int						cursor;

	int						numItems;
	void					*items[64];

	char					*statusBar;

	void					(*cursorDraw) (struct uiFrameWork_s *m);
	
} uiFrameWork_t;

typedef struct uiCommon_s {
	int						type;

	char					*name;

	float					x;
	float					y;

	uiFrameWork_t			*parent;
	int						cursorOffset;
	int						localData[4];
	uInt					flags;

	char					*statusBar;
	int						tl[2];		// top left for mouse collision
	int						br[2];		// bottom right for mouse collision

	void (*callBack)		(void *self);

	void (*statusBarFunc)	(void *self);
	void (*ownerDraw)		(void *self);
	void (*cursorDraw)		(void *self);
} uiCommon_t;

typedef struct uiAction_s {
	uiCommon_t				generic;
} uiAction_t;

typedef struct uiField_s {
	uiCommon_t				generic;

	char					buffer[80];
	int						cursor;

	int						length;
	int						visibleLength;
	int						visibleOffset;
} uiField_t;

typedef struct uiList_s {
	uiCommon_t				generic;

	int						curValue;

	char					**itemNames;
} uiList_t;

typedef struct uiImage_s {
	uiCommon_t				generic;

	struct image_s			*image;
	struct image_s			*selImage;

	int						width;
	int						height;
} uiImage_t;

typedef struct uiSlider_s {
	uiCommon_t				generic;

	float					minValue;
	float					maxValue;
	float					curValue;

	float					range;
} uiSlider_t;

/*
=============================================================================

	CURSOR

=============================================================================
*/

extern cVar_t	*ui_jsMenuPage;
extern cVar_t	*ui_jsSortItem;
extern cVar_t	*ui_jsSortMethod;

extern cVar_t	*ui_sensitivity;

extern qBool	uiCursorLock;
void			(*uiCursorItem);
void			(*uiMouseItem);
void			(*uiSelItem);

void	UI_CursorInit (void);
void	UI_DrawMouseCursor (void);
void	UI_UpdateMousePos (void);
void	UI_MoveMouse (int x, int y);
void	UI_SetCursorPos (int x, int y);
void	UI_Setup (uiFrameWork_t *menu);

/*
=============================================================================

	MENUS

=============================================================================
*/

void UI_MainMenu_f (void);

void UI_GameMenu_f (void);
	void UI_LoadGameMenu_f (void);
	void UI_SaveGameMenu_f (void);
	void UI_CreditsMenu_f (void);

void UI_MultiplayerMenu_f (void);
		void UI_DLOptionsMenu_f (void);
	void UI_JoinServerMenu_f (void);
		void UI_AddressBookMenu_f (void);
	void UI_PlayerConfigMenu_f (void);
	void UI_StartServerMenu_f (void);
		void UI_DMFlagsMenu_f (void);

void UI_OptionsMenu_f (void);
	void UI_ControlsMenu_f (void);
	void UI_EffectsMenu_f (void);
	void UI_FogMenu_f (void);
	void UI_GloomMenu_f (void);
	void UI_HUDMenu_f (void);
	void UI_InputMenu_f (void);
	void UI_MiscMenu_f (void);
	void UI_ScreenMenu_f (void);
	void UI_SoundMenu_f (void);

void UI_VideoMenu_f (void);
	void UI_GLExtsMenu_f (void);
	void UI_VIDSettingsMenu_f (void);

void UI_QuitMenu_f (void);

/*
=============================================================================

	MISCELLANEOUS

=============================================================================
*/

// play after drawing a frame, so caching won't disrupt the sound
extern qBool	uiEnterSound;

//
// ui_api.c
//

void	UI_DrawChar (float x, float y, int flags, float scale, int num, vec4_t color);
int		UI_DrawString (float x, float y, int flags, float scale, char *string, vec4_t color);
void	UI_DrawStretchPic (char *pic, float x, float y, int w, int h, vec4_t color);
void	UI_DrawPic (struct image_s *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	UI_DrawFill (float x, float y, int w, int h, vec4_t color);

//
// ui_backend.c
//

extern uiFrameWork_t	*uiActiveUI;

void	(*UI_DrawFunc) (void);

void	UI_InitSoundMedia (void);

void	UI_Init (int vidWidth, int vidHeight, int oldCursorX, int oldCursorY);
void	UI_Shutdown (qBool full);

void	UI_PushMenu (uiFrameWork_t *m, void (*draw) (void), struct sfx_s *(*key) (int k));
void	UI_PopMenu (void);
void	UI_ForceMenuOff (void);

void	UI_Center (uiFrameWork_t *menu);

void	UI_AddItem (uiFrameWork_t *menu, void *item);
void	UI_AdjustTextCursor (uiFrameWork_t *menu, int dir);
void	*UI_ItemAtCursor (uiFrameWork_t *m);
qBool	UI_SelectItem (uiFrameWork_t *s);
void	UI_SetStatusBar (uiFrameWork_t *s, char *string);
qBool	UI_SlideItem (uiFrameWork_t *s, int dir);

char	*UI_StrDup (const char *in);

//
// ui_draw.c
//

void	UI_Banner (uiImage_t *item, struct image_s *image, float *y);

void	__fastcall UI_DrawTextBox (float x, float y, float scale, int width, int lines);

void	UI_Draw (uiFrameWork_t *menu);
void	UI_DrawStatusBar (char *string);

void	UI_Refresh (int time, int connectState, int serverState, int vidWidth, int vidHeight, qBool fullScreen);

//
// ui_joinserver.c
//

void	UI_FreeServerList (void);

qBool	UI_ParseServerInfo (char *adr, char *info);
qBool	UI_ParseServerStatus (char *adr, char *info);

//
// ui_keys.c
//

struct	sfx_s *(*UI_KeyFunc) (int key);

void	UI_Keydown (int key);
struct	sfx_s *DefaultMenu_Key (uiFrameWork_t *m, int key);
qBool	Field_Key (uiField_t *field, int key);

//
// ui_loadgame.c
//

extern char		uiSaveStrings[MAX_SAVEGAMES][32];

void	Create_Savestrings (void);

//
// ui_loadscreen.c
//

void	UI_DrawConnectScreen (char *serverName, int connectCount, qBool backGround, qBool cGameOn);

//
// ui_startserver.c
//

void	UI_FreeMapNames (void);

#endif // __UI_LOCAL_H__
