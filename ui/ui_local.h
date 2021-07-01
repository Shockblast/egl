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

#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../shared/shared.h"
#include "../client/cl_local.h"
#include "ui_api.h"
#include "ui_common.h"

extern ui_import_t uii;

/*
=============================================================================

	UI STATE

=============================================================================
*/

typedef struct {
	int			time;

	int			vidWidth;
	int			vidHeight;

	int			cursorX;
	int			cursorY;

	int			connectionState;
	int			serverState;
} uiState_t;

extern uiState_t	uis;

/*
=============================================================================

	UI MEDIA

=============================================================================
*/

#define MAINMENU_CURSOR_NUMFRAMES	15

typedef struct {
	// background images
	image_t		*bgBig;

	// cursor image
	image_t		*cursorImage;

	// menu banners
	image_t		*addressBookBanner;
	image_t		*multiplayerBanner;
	image_t		*startServerBanner;
	image_t		*joinServerBanner;
	image_t		*optionsBanner;
	image_t		*gameBanner;
	image_t		*loadGameBanner;
	image_t		*saveGameBanner;
	image_t		*videoBanner;
	image_t		*quitBanner;

	// main menu items
	image_t		*mainCursors[MAINMENU_CURSOR_NUMFRAMES];
	image_t		*mainPlaque;
	image_t		*mainLogo;

	image_t		*mainGame;
	image_t		*mainMultiplayer;
	image_t		*mainOptions;
	image_t		*mainVideo;
	image_t		*mainQuit;

	image_t		*mainGameSel;
	image_t		*mainMultiplayerSel;
	image_t		*mainOptionsSel;
	image_t		*mainVideoSel;
	image_t		*mainQuitSel;
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
	UIF_NOSELBAR		= 1 << 10
};

//
// misc
//
#define	MAX_UI_DEPTH		12

#define RCOLUMN_OFFSET		(UIFT_SIZE*2)
#define LCOLUMN_OFFSET		(-(RCOLUMN_OFFSET))

#define MAX_LOCAL_SERVERS	8
#define	MAX_SAVEGAMES		16

#define SLIDER_RANGE		10

/*
=============================================================================

	SCALING

=============================================================================
*/

#define UI_SCALE			(uis.vidWidth / 640.0)

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

#define UI_HOVERALPHA			(clamp (0.05 * sin (uis.time*2) + 0.6, 0.6, 0.99))
#define UIFT_ALPHA(i)			((i == NULL) ? 1 : ((ui_CursorItem && (ui_CursorItem == i)) || (ui_SelItem && (ui_SelItem == i))) ? UI_HOVERALPHA : 1)

/*
=============================================================================

	STRUCTS

=============================================================================
*/

typedef struct uiFrameWork_s {
	float			x;
	float			y;
	int				cursor;

	int				nitems;
	int				nslots;
	void			*items[64];

	char			*statusbar;

	void			(*cursordraw) (struct uiFrameWork_s *m);
	
} uiFrameWork_t;

typedef struct {
	int				type;

	char			*name;

	float			x;
	float			y;

	uiFrameWork_t	*parent;
	int				cursor_offset;
	int				localdata[4];
	unsigned		flags;

	char			*statusbar;
	int				tl[2];		// top left for mouse collision
	int				br[2];		// bottom right for mouse collision

	void (*callback)		(void *self);
	void (*statusbarfunc)	(void *self);
	void (*ownerdraw)		(void *self);
	void (*cursordraw)		(void *self);
} uiCommon_t;

typedef struct {
	uiCommon_t		generic;
} uiAction_t;

typedef struct {
	uiCommon_t		generic;

	char			buffer[80];
	int				cursor;

	int				length;
	int				visible_length;
	int				visible_offset;
} uiField_t;

typedef struct {
	uiCommon_t		generic;

	int				curvalue;

	char			**itemnames;
} uiList_t;

typedef struct {
	uiCommon_t		generic;

	image_t			*image;
	image_t			*selImage;
} uiImage_t;

typedef struct {
	uiCommon_t		generic;

	float			minvalue;
	float			maxvalue;
	float			curvalue;

	float			range;
} uiSlider_t;

typedef struct {
	uiFrameWork_t	*m;
	void			(*draw) (void);
	const char		*(*key) (int k);
} uiLayer_t;

/*
=============================================================================

	MISCELLANEOUS

=============================================================================
*/

// play after drawing a frame, so caching won't disrupt the sound
extern qBool	ui_EnterSound;

extern char		*ui_InSFX;
extern char		*ui_MoveSFX;
extern char		*ui_OutSFX;

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

//
// ui_api.c
//

void	UI_DrawChar (float x, float y, int flags, float scale, int num, vec4_t color);
int		UI_DrawString (float x, float y, int flags, float scale, char *string, vec4_t color);
void	UI_DrawStretchPic (char *pic, float x, float y, int w, int h, vec4_t color);
void	UI_DrawPic (image_t *image, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	UI_DrawFill (float x, float y, int w, int h, vec4_t color);

void	UI_ImgGetPicSize (int *w, int *h, char *pic);

//
// ui_backend.c
//

extern char				**mapnames;
extern int				nummaps;
extern uiFrameWork_t	*ui_ActiveUI;

void	(*UI_DrawFunc) (void);

void	UI_Init (int vidWidth, int vidHeight);
void	UI_Shutdown (void);

void	UI_PushMenu (uiFrameWork_t *m, void (*draw) (void), const char *(*key) (int k));
void	UI_PopMenu (void);
void	UI_ForceMenuOff (void);

void	UI_Center (uiFrameWork_t *menu);

void	UI_AddItem (uiFrameWork_t *menu, void *item);
void	UI_AdjustTextCursor (uiFrameWork_t *menu, int dir);
void	*UI_ItemAtCursor (uiFrameWork_t *m);
qBool	UI_SelectItem (uiFrameWork_t *s);
void	UI_SetStatusBar (uiFrameWork_t *s, char *string);
qBool	UI_SlideItem (uiFrameWork_t *s, int dir);

//
// ui_cursor.c
//

void	UI_CursorInit (void);
void	UI_DrawMouseCursor (void);
void	UI_UpdateMousePos (void);
void	UI_MoveMouse (int mx, int my);
void	UI_Setup (uiFrameWork_t *menu);

extern qBool	ui_CursorLock;
void			(*ui_CursorItem);
void			(*ui_MouseItem);
void			(*ui_SelItem);

//
// ui_draw.c
//

void	UI_Banner (uiImage_t *item, image_t *image, float *y);

void	FASTCALL UI_DrawTextBox (float x, float y, float scale, int width, int lines);

void	UI_Draw (uiFrameWork_t *menu);
void	UI_DrawStatusBar (char *string);

void	UI_Refresh (int time, int connectionState, int serverState, int vidWidth, int vidHeight, qBool backGround);

//
// ui_joinserver.c
//

void	UI_AddToServerList (netAdr_t adr, char *info);

//
// ui_keys.c
//

const char	*(*UI_KeyFunc) (int key);

void		UI_Keydown (int key);
const char	*DefaultMenu_Key (uiFrameWork_t *m, int key);
qBool		Field_Key (uiField_t *field, int key);

//
// ui_loadgame.c
//

void	Create_Savestrings (void);
extern char		ui_SaveStrings[MAX_SAVEGAMES][32];

//
// ui_quit.c
//

#endif // __UI_LOCAL_H__
