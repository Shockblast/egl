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

#include "ui_local.h"

/*
=============================================================================

	CREDITS MENU

=============================================================================
*/

typedef struct {
	uiFrameWork_t		framework;

	// kthx do me
//	menuaction_t		eglcredits_action;
//	menuaction_t		quake2credits_action;
//	menuaction_t		roguecredits_action;
//	menuaction_t		xatrixcredits_action;

//	menuaction_t		back_action;
} m_creditsmenu_t;

m_creditsmenu_t		m_credits_menu;

static int			credits_start_time;
static const char	**credits;
static char			*creditsIndex[256];
static char			*creditsBuffer;

static const char *idcredits[] = {
	S_COLOR_BLUE"QUAKE II BY ID SOFTWARE",
	"",
	S_COLOR_BLUE"PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	S_COLOR_BLUE"ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	S_COLOR_BLUE"LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	S_COLOR_BLUE"BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	S_COLOR_BLUE"SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	S_COLOR_BLUE"ADDITIONAL SUPPORT",
	"",
	S_COLOR_BLUE"LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	S_COLOR_BLUE"CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	S_COLOR_BLUE"SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	S_COLOR_BLUE"THANKS TO ACTIVISION",
	S_COLOR_BLUE"IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char *xatcredits[] = {
	S_COLOR_BLUE"QUAKE II MISSION PACK: THE RECKONING",
	S_COLOR_BLUE"BY",
	S_COLOR_BLUE"XATRIX ENTERTAINMENT, INC.",
	"",
	S_COLOR_BLUE"DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	S_COLOR_BLUE"PRODUCED BY",
	"Greg Goodrich",
	"",
	S_COLOR_BLUE"PROGRAMMING",
	"Rafael Paiz",
	"",
	S_COLOR_BLUE"LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	S_COLOR_BLUE"LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	S_COLOR_BLUE"ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	S_COLOR_BLUE"COMPUTER GRAPHICS SUPERVISOR AND",
	S_COLOR_BLUE"CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	S_COLOR_BLUE"SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	S_COLOR_BLUE"CHARACTER ANIMATION AND",
	S_COLOR_BLUE"MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	S_COLOR_BLUE"ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	S_COLOR_BLUE"INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	S_COLOR_BLUE"ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	S_COLOR_BLUE"3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	S_COLOR_BLUE"ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	S_COLOR_BLUE"ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	S_COLOR_BLUE"PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	S_COLOR_BLUE"SOUND DESIGN",
	"Gary Bradfield",
	"",
	S_COLOR_BLUE"MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	S_COLOR_BLUE"SPECIAL THANKS",
	S_COLOR_BLUE"TO",
	S_COLOR_BLUE"OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	S_COLOR_BLUE"THANKS TO ACTIVISION",
	S_COLOR_BLUE"IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	S_COLOR_BLUE"AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	S_COLOR_BLUE"THANKS TO INTERGRAPH COMPUTER SYTEMS",
	S_COLOR_BLUE"IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char *roguecredits[] = {
	S_COLOR_BLUE"QUAKE II MISSION PACK 2: GROUND ZERO",
	S_COLOR_BLUE"BY",
	S_COLOR_BLUE"ROGUE ENTERTAINMENT, INC.",
	"",
	S_COLOR_BLUE"PRODUCED BY",
	"Jim Molinets",
	"",
	S_COLOR_BLUE"PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	S_COLOR_BLUE"LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	S_COLOR_BLUE"ART DIRECTION",
	"Rich Fleider",
	"",
	S_COLOR_BLUE"ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	S_COLOR_BLUE"ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	S_COLOR_BLUE"ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	S_COLOR_BLUE"SOUND",
	"James Grunke",
	"",
	S_COLOR_BLUE"GROUND ZERO THEME",
	S_COLOR_BLUE"AND",
	S_COLOR_BLUE"MUSIC BY",
	"Sonic Mayhem",
	"",
	S_COLOR_BLUE"VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	S_COLOR_BLUE"SPECIAL THANKS",
	S_COLOR_BLUE"TO",
	S_COLOR_BLUE"OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	S_COLOR_BLUE"THANKS TO ACTIVISION",
	S_COLOR_BLUE"IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	S_COLOR_BLUE"AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};


/*
=============
CreditsMenu_Init
=============
*/
void CreditsMenu_Init (void) {
	int		n, count;
	char	*p;

	creditsBuffer = NULL;
	count = uii.FS_LoadFile ("credits", &creditsBuffer);

	if (count != -1) {
		p = creditsBuffer;
		for (n=0 ; n<255 ; n++) {
			creditsIndex[n] = p;
			while ((*p != '\r') && (*p != '\n')) {
				p++;
				if (--count == 0)
					break;
			}

			if (*p == '\r') {
				*p++ = 0;
				if (--count == 0)
					break;
			}

			*p++ = 0;
			if (--count == 0)
				break;
		}
		creditsIndex[++n] = 0;
		credits = creditsIndex;
	} else {
		if (uii.FS_CurrGame ("xatrix"))
			credits = xatcredits;
		else if (uii.FS_CurrGame ("rogue"))
			credits = roguecredits;
		else
			credits = idcredits;
	}

	credits_start_time = uis.time;
}


/*
=============
CreditsMenu_Draw
=============
*/
void CreditsMenu_Draw (void) {
	int		i;
	float	y;

	/* draw the credits */
	for (i=0, y=uis.vidHeight - ((uis.time - credits_start_time) / 40.0F) ; credits[i] && (y < uis.vidHeight) ; y += (UIFT_SIZEINCMED), i++) {
		if (y <= -UIFT_SIZEMED)
			continue;

		UI_DrawString ((uis.vidWidth - (Q_ColorCharCount (credits[i], strlen (credits[i])) * UIFT_SIZEMED)) * 0.5,
			y, 0, UIFT_SCALEMED, (char *)credits[i], colorWhite);
	}

	if (y < 0)
		credits_start_time = uis.time;
}


/*
=============
CreditsMenu_Key
=============
*/
const char *CreditsMenu_Key (int key) {
	switch (key) {
	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_ESCAPE:
		if (creditsBuffer)
			uii.FS_FreeFile (creditsBuffer);

		UI_PopMenu ();
		break;
	}

	return NULL;
}


/*
=============
UI_CreditsMenu_f
=============
*/
void UI_CreditsMenu_f (void) {
	CreditsMenu_Init ();
	UI_PushMenu (&m_credits_menu.framework, CreditsMenu_Draw, CreditsMenu_Key);
}
