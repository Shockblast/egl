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
Foundation, Inc., 59 Temple Place - Suite 330, v
*/

//
// console.c
//

#include "cl_local.h"

console_t	con;

cVar_t		*con_notifyfade;
cVar_t		*con_notifylarge;
cVar_t		*con_notifylines;
cVar_t		*con_notifytime;

#define CON_NOTIFYTIMES	( clamp (con_notifylines->integer, 1, CON_MAXTIMES) )


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void) {
	int		i;

	for (i=0 ; i<CON_MAXTIMES ; i++)
		con.times[i] = 0;
}


/*
================
Con_Close
================
*/
void Con_Close (void) {
	con.currentDrop = 0;

	Key_ClearTyping ();
	Con_ClearNotify ();
	Key_ClearStates ();
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void) {
	int		i, j, width, oldwidth, oldtotalLines, numlines, numchars;
	char	tempbuf[CON_TEXTSIZE];

	if (vidDef.width < 1) {
		// video hasn't been initialized yet
		width = (512 / 8) - 2;
		con.lineWidth = width;
		con.totalLines = CON_TEXTSIZE / con.lineWidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	} else {
		width = (vidDef.width / FT_SIZE) - 2;
		if (width == con.lineWidth)
			return;

		oldwidth = con.lineWidth;
		con.lineWidth = width;
		oldtotalLines = con.totalLines;
		con.totalLines = CON_TEXTSIZE / con.lineWidth;
		numlines = oldtotalLines;

		if (con.totalLines < numlines)
			numlines = con.totalLines;

		numchars = oldwidth;
	
		if (con.lineWidth < numchars)
			numchars = con.lineWidth;

		memcpy (tempbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++) {
			for (j=0 ; j<numchars ; j++) {
				con.text[(con.totalLines - 1 - i) * con.lineWidth + j] =
					tempbuf[((con.currentLine - i + oldtotalLines) % oldtotalLines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.currentLine = max (1, con.totalLines - 1);
	con.display = con.currentLine;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void) {
	if (con.display == con.currentLine)
		con.display++;

	con.currentLine++;
	memset (&con.text[(con.currentLine%con.totalLines)*con.lineWidth], ' ', con.lineWidth);
}


/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (int flags, const char *txt) {
	int			y, l;
	static int	cr, xOffset = 0;
	static int	lastColor, lastStyle;
	int			orMask;

	if (!con.initialized)
		return;

	lastColor = -1;
	lastStyle = -1;

	if ((txt[0] == 1) || (txt[0] == 2)) {
		orMask = 128;
		txt++;
	} else
		orMask = 0;

	while (*txt) {
		// count word length
		for (l=0 ; l<con.lineWidth ; l++) {
			if (txt[l] <= ' ')
				break;
		}

		// word wrap
		if ((l != con.lineWidth) && (xOffset + l > con.lineWidth))
			xOffset = 0;

		if (cr) {
			con.currentLine = max (1, con.currentLine - 1);
			cr = qFalse;
		}

		if (!xOffset) {
			Con_Linefeed ();

			// mark time for notify lines
			con.times[con.currentLine % CON_MAXTIMES] = (flags & PRNT_CONSOLE) ? 0 : cls.realTime;

			y = con.currentLine % con.totalLines;
			if ((lastColor != -1) && (y*con.lineWidth < CON_TEXTSIZE-2)) {
				con.text[y*con.lineWidth] = Q_COLOR_ESCAPE;
				con.text[y*con.lineWidth+1] = '0' + lastColor;
				xOffset += 2;
			}

			if ((lastStyle != -1) && (y*con.lineWidth+xOffset < CON_TEXTSIZE-2)) {
				con.text[y*con.lineWidth+xOffset] = Q_COLOR_ESCAPE;
				con.text[y*con.lineWidth+xOffset+1] = lastStyle;
				xOffset += 2;
			}
		}

		switch (*txt) {
		case '\n':
			lastColor = -1;
			lastStyle = -1;
			xOffset = 0;
			break;

		case '\r':
			lastColor = -1;
			lastStyle = -1;
			xOffset = 0;
			cr = qTrue;
			break;

		default:
			// display character and advance
			y = con.currentLine % con.totalLines;
			con.text[y*con.lineWidth+xOffset] = *txt | orMask | con.orMask;
			if (++xOffset >= con.lineWidth)
				xOffset = 0;

			// get old color/style codes
			if (Q_IsColorString (txt)) {
				switch ((*(txt+1)) & 127) {
				case 's':
				case 'S':
					if (lastStyle == 'S')
						lastStyle = -1;
					else
						lastStyle = 'S';
					break;

				case 'r':
				case 'R':
					lastStyle = -1;
					lastColor = -1;
					break;

				case COLOR_BLACK:
				case COLOR_RED:
				case COLOR_GREEN:
				case COLOR_YELLOW:
				case COLOR_BLUE:
				case COLOR_CYAN:
				case COLOR_MAGENTA:
				case COLOR_WHITE:
				case COLOR_GREY:
					lastColor = Q_StrColorIndex (*(txt+1));
					break;

				default:
					lastColor = Q_StrColorIndex (COLOR_WHITE);
				}
			}
			break;
		}

		txt++;
	}
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
================
Con_ClearText_f
================
*/
void Con_ClearText_f (void) {
	memset (con.text, ' ', CON_TEXTSIZE);
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void) {
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: condump <filename>\n");
		return;
	}

	Q_snprintfz (name, sizeof (name), "%s/condumps/%s.txt", FS_Gamedir(), Cmd_Argv (1));

	Com_Printf (0, "Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f) {
		Com_Printf (0, S_COLOR_RED "ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l=con.currentLine-con.totalLines+1 ; l<=con.currentLine ; l++) {
		line = con.text + (l%con.totalLines)*con.lineWidth;
		for (x=0 ; x<con.lineWidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con.lineWidth)
			break;
	}

	// write the remaining lines
	buffer[con.lineWidth] = 0;
	for ( ; l<=con.currentLine ; l++) {
		line = con.text + (l % con.totalLines) * con.lineWidth;
		strncpy (buffer, line, con.lineWidth);
		for (x=con.lineWidth-1 ; x>=0 ; x--) {
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// kill the loading plaque
	SCR_EndLoadingPlaque ();

	// if playing a cinematic, kill it
	if (cl.attractLoop) {
		Cbuf_AddText ("killserver\n");
		return;
	}

	// if at the menu, close it
	if (Key_KeyDest (KD_MENU)) {
		CL_UIModule_ForceMenuOff ();
		return;
	}

	// if at the console open the menu
	if (cls.connState == CA_DISCONNECTED) {
		CL_UIModule_MainMenu ();
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();
	Key_ClearStates ();

	if (Key_KeyDest (KD_CONSOLE))
		CL_UIModule_ForceMenuOff ();
	else {
		CL_UIModule_ForceMenuOff ();

		Key_SetKeyDest (KD_CONSOLE);
	}
}


/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void) {
	Key_ClearTyping ();

	if (Key_KeyDest (KD_CONSOLE)) {
		if (cls.connState == CA_ACTIVE) {
			CL_UIModule_ForceMenuOff ();
			Key_SetKeyDest (KD_GAME);
		}
	} else
		Key_SetKeyDest (KD_CONSOLE);
	
	Con_ClearNotify ();
}


/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	keyChatTeam = qFalse;
	Key_SetKeyDest (KD_MESSAGE);
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	keyChatTeam = qTrue;
	Key_SetKeyDest (KD_MESSAGE);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
================
Con_Init
================
*/
void Con_Init (void) {
	// cvars needed for dedicated mode
	con_notifylines = Cvar_Get ("con_notifylines", "4", CVAR_ARCHIVE);

	if (dedicated->integer)
		return;

	con_notifyfade			= Cvar_Get ("con_notifyfade",		"1",	CVAR_ARCHIVE);
	con_notifylarge			= Cvar_Get ("con_notifylarge",		"0",	CVAR_ARCHIVE);
	con_notifytime			= Cvar_Get ("con_notifytime",		"3",	CVAR_ARCHIVE);

	Cmd_AddCommand ("toggleconsole",	Con_ToggleConsole_f,	"Toggles displaying the console");
	Cmd_AddCommand ("togglechat",		Con_ToggleChat_f,		"");
	Cmd_AddCommand ("messagemode",		Con_MessageMode_f,		"");
	Cmd_AddCommand ("messagemode2",		Con_MessageMode2_f,		"");
	Cmd_AddCommand ("clear",			Con_ClearText_f,		"Clears the console buffer");
	Cmd_AddCommand ("condump",			Con_Dump_f,				"Dumps the content of the console to file");

	con.lineWidth = -1;
	con.totalLines = -1;
	con.orMask = 0;

	Con_CheckResize ();

	con.initialized = qTrue;
}

/*
==============================================================================

	DRAWING

==============================================================================
*/

int Q_ColorStrLastColor (char *s, int byteofs) {
	char	*end;
	int		lastClrIndex = Q_StrColorIndex (COLOR_WHITE);

	end = s + (byteofs - 1);	// don't check last byte

	for ( ; *s && s<end ; s++) {
		if (Q_IsColorString (s)) {
			if (((s[1] & 127) >= COLOR_BLACK) && ((s[1] & 127) <= COLOR_GREY))
				lastClrIndex = Q_StrColorIndex (s[1]);
			else if ((toupper (s[1]) & 127) == 'R')
				lastClrIndex = Q_StrColorIndex (COLOR_WHITE);
			s++;
		}
	}

	return lastClrIndex;
}

int Q_ColorStrLastStyle (char *s, int byteofs) {
	char	*end;
	int		lastStyle = 0;

	end = s + (byteofs - 1);	// don't check last byte

	for ( ; *s && s<end ; s++) {
		if (Q_IsColorString (s)) {
			if ((toupper(s[1]) & 127) == 'S') {
				if (lastStyle & FS_SHADOW)
					lastStyle = 0;
				else
					lastStyle |= FS_SHADOW;
			} else if ((toupper(s[1]) & 127) == 'R')
				lastStyle = 0;

			s++;
		}
	}

	return lastStyle;
}

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void) {
	char	*text;
	int		lastColor, lastFlags;
	int		colorlinepos;
	int		byteofs;
	int		bytelen;

	if (Key_KeyDest (KD_MENU))
		return;

	// don't draw anything (always draw if not active)
	if (!Key_KeyDest (KD_CONSOLE) && (cls.connState == CA_ACTIVE))
		return;

	text = keyConsoleBuffer[keyConsoleEditLine];

	// convert byte offset to visible character count
	colorlinepos = Q_ColorCharCount (text, keyConsoleLinePos);

	// prestep if horizontally scrolling
	if (colorlinepos >= con.lineWidth + 1) {
		byteofs = Q_ColorCharOffset (text, colorlinepos - con.lineWidth);
		lastColor = Q_ColorStrLastColor (text, byteofs);
		lastFlags = Q_ColorStrLastStyle (text, byteofs);
		text += byteofs;
		colorlinepos = con.lineWidth;
	} else {
		lastColor = Q_StrColorIndex (COLOR_WHITE);
		lastFlags = 0;
	}

	bytelen = Q_ColorCharOffset (text, con.lineWidth);

	// draw it
	Draw_StringLen (NULL, 8, con.visLines - (FT_SIZE * 2), lastFlags, FT_SCALE, text, bytelen, strColorTable[lastColor]);

	// add the cursor
	if (((int)(cls.realTime>>8))&1)
		Draw_Char (NULL, 8 + ((colorlinepos - (keyInsertOn ? 0.3 : 0)) * FT_SIZE), con.visLines - (FT_SIZE * 2),
					0, FT_SCALE, keyInsertOn ? '|' : 11, colorWhite);
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void) {
	int		v, i, time;
	int		notifyLines;
	int		totalLines;
	char	*str;
	float	newScale = FT_SCALE;
	float	newSize = FT_SIZE;
	vec4_t	color;
	Vector4Copy (colorWhite, color);

	// make larger if desired
	if (con_notifylarge->integer) {
		newScale *= 1.25;
		newSize *= 1.25;
	}

	//
	// render notify lines
	//
	for (totalLines=0, i=con.currentLine ; i>con.currentLine-CON_MAXTIMES+1 ; i--) {
		if (totalLines > CON_NOTIFYTIMES)
			break;

		time = con.times[i % CON_MAXTIMES];
		if (time == 0)
			continue;
		time = cls.realTime - time;
		if (time > con_notifytime->value * 1000)
			continue;
		totalLines++;
	}

	for (v=newSize*(totalLines-1), notifyLines=0, i=con.currentLine ; i>con.currentLine-CON_MAXTIMES+1 ; i--) {
		if (notifyLines > CON_NOTIFYTIMES)
			break;

		time = con.times[i % CON_MAXTIMES];
		if (time == 0)
			continue;
		time = cls.realTime - time;
		if (time > con_notifytime->value * 1000)
			continue;
		notifyLines++;

		if (con_notifyfade->integer)
			color[3] = 0.25 + 0.75 * (con_notifytime->value - (time * 0.001)) / con_notifytime->value;

		Draw_StringLen (NULL, 8, v, 0, newScale, con.text + (i % con.totalLines)*con.lineWidth, con.lineWidth, color);

		v -= newSize;
	}
	v=newSize*totalLines;

	//
	// print messagemode input
	//
	if (Key_KeyDest (KD_MESSAGE)) {
		int		skip;
		int		lastColor, lastFlags;
		int		colorlinepos;
		int		byteofs;

		if (keyChatTeam)
			skip = Draw_String (NULL, 4, v, 0, newScale, "say_team:", colorWhite) + 1;
		else
			skip = Draw_String (NULL, 4, v, 0, newScale, "say:", colorWhite) + 1;

		str = keyChatBuffer[keyChatEditLine];

		// convert byte offset to visible character count
		colorlinepos = Q_ColorCharCount (str, keyChatLinePos) + skip + 1;

		// prestep if horizontally scrolling
		if (colorlinepos >= (int)(vidDef.width/newSize)) {
			byteofs = Q_ColorCharOffset (str, colorlinepos - (int)(vidDef.width/newSize));

			lastColor = Q_ColorStrLastColor (str, byteofs);
			lastFlags = Q_ColorStrLastStyle (str, byteofs);

			str += byteofs;
		} else {
			lastColor = Q_StrColorIndex (COLOR_WHITE);
			lastFlags = 0;
		}

		Draw_String (NULL, skip * newSize, v, lastFlags, newScale, str, strColorTable[lastColor]);

		// add cursor
		if (((int)(cls.realTime>>8))&1) {
			int charCount = Q_ColorCharCount (keyChatBuffer[keyChatEditLine], keyChatLinePos) + skip;
			if (charCount > (int)(vidDef.width/newSize) - 1)
				charCount = (int)(vidDef.width/newSize) - 1;

			Draw_Char (NULL, ((charCount - (keyInsertOn ? 0.3 : 0)) * newSize), v, 0, newScale,
						keyInsertOn ? '|' : 11, colorWhite);
		}

		v += newSize;
	}
}


/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	Con_CheckResize ();

	// decide on the height of the console
	if (Key_KeyDest (KD_CONSOLE))
		con.dropHeight = con_drop->value;
	else
		con.dropHeight = 0;			// none visible
	
	if (con.dropHeight < con.currentDrop) {
		con.currentDrop -= scr_conspeed->value*cls.frameTime;
		if (con.dropHeight > con.currentDrop)
			con.currentDrop = con.dropHeight;

	} else if (con.dropHeight > con.currentDrop) {
		con.currentDrop += scr_conspeed->value*cls.frameTime;
		if (con.dropHeight < con.currentDrop)
			con.currentDrop = con.dropHeight;
	}
}


/*
================
Con_DrawConsole
================
*/
void Con_DrawConsole (void) {
	float			frac;
	int				i, j, row;
	float			x, y, lines;
	vec4_t			conColor;
	char			version[32];

	frac = 0.0;
	if ((cls.connState == CA_DISCONNECTED) || (cls.connState == CA_CONNECTING)) {
		// forced full screen console
		Draw_Fill (0, 0, vidDef.width, vidDef.height, colorBlack);
		frac = 1.0;
	} else if ((cls.connState != CA_ACTIVE) || !cl.refreshPrepped) {
		// connected, but can't render
		Draw_Fill (0, 0, vidDef.width, vidDef.height, colorBlack);
		frac = con_drop->value;
	} else {
		if (con.currentDrop)
			frac = con.currentDrop;
		else {
			if (Key_KeyDest (KD_GAME) || Key_KeyDest (KD_MESSAGE))
				Con_DrawNotify ();	// only draw notify in game
			return;
		}
	}

	con.visLines = vidDef.height * clamp (frac, 0, 1);
	if (con.visLines <= 0)
		return;

	// draw the background
	Vector4Copy (colorWhite, conColor);
	conColor[3] = con_alpha->value;
	Draw_Pic (clMedia.consoleImage, 0, -(vidDef.height - vidDef.height*frac), vidDef.width, vidDef.height, 0, 0, 1, 1, conColor);

	// version
	Q_snprintfz (version, sizeof (version), "EGL v%s", EGL_VERSTR);
	Draw_String (NULL, vidDef.width - (strlen (version) * FT_SIZE) - (FT_SHAOFFSET * 2),
				con.visLines - (FT_SIZE + (FT_SHAOFFSET * 2)),
				FS_SHADOW, FT_SCALE, version, colorCyan);

	// time if desired
	if (con_clock->integer) {
		char		clockStr[16];
		time_t		ctime;
		struct tm	*ltime;

		ctime = time (NULL);
		ltime = localtime (&ctime);
		strftime (clockStr, sizeof (clockStr), "%I:%M %p", ltime);

		// kill the initial zero
		if (clockStr[0] == '0') {
			for (i=0 ; i<(strlen (clockStr) + 1) ; i++)
				clockStr[i] = clockStr[i+1];
			clockStr[i+1] = '\0';
		}

		Draw_String (NULL, vidDef.width - (strlen (clockStr) * FT_SIZE) - (FT_SHAOFFSET * 2),
					con.visLines - ((FT_SIZE + (FT_SHAOFFSET * 2)) * 2),
					FS_SHADOW, FT_SCALE, clockStr, colorCyan);
	}

	// draw the console text
	y = con.visLines - (FT_SIZE * 3);	// start point (from the bottom)
	lines = (y / FT_SIZE);				// lines of text to draw

	// draw arrows to show the buffer is backscrolled
	if (con.display != con.currentLine) {
		for (x=0 ; x<((con.lineWidth+1)*(FT_SIZE)) ; x+=(FT_SIZE*2))
			Draw_Char (NULL, x + (FT_SIZE * 0.5), y, FS_SHADOW, FT_SCALE, '^', colorRed);

		y -= (FT_SIZE + FT_SHAOFFSET);
		lines--;
	}

	// pull the buffer up a spot if downloading a file, so the download bar has room
	if (cls.downloadFile) {
		y -= (FT_SIZE + FT_SHAOFFSET);
		lines--;
	}

	// draw the print, from the bottom up
	for (i=0, row=con.display ; i<lines ; i++, y -= (FT_SIZE + FT_SHAOFFSET), row--) {
		if (row < 0)
			break;
		if (con.currentLine - row >= con.totalLines)
			break;	// past scrollback wrap point
		if (y < -(FT_SIZE + FT_SHAOFFSET))
			break;

		Draw_StringLen (NULL, 8, y, 0, FT_SCALE, con.text + (row % con.totalLines) * con.lineWidth, con.lineWidth, colorWhite);
	}

	// figure out width & draw the download bar
	if (cls.downloadFile) {
		char	dlbar[1024];
		char	*text;
		float	n;

		if ((text = strrchr (cls.downloadName, '/')) != NULL)
			text++;
		else
			text = cls.downloadName;

		x = con.lineWidth - ((con.lineWidth * 7) / 40);
		y = x - strlen(text) - FT_SIZE;
		i = con.lineWidth/3;

		if (strlen (text) > i) {
			y = x - i - 11;
			strncpy (dlbar, text, i);
			dlbar[i] = 0;
			strcat (dlbar, "...");
		} else
			strcpy (dlbar, text);

		strcat (dlbar, ": ");
		i = strlen (dlbar);
		dlbar[i++] = '\x80';

		// where's the dot go?
		if (cls.downloadPercent == 0)
			n = 0;
		else
			n = y * cls.downloadPercent / 100;
			
		for (j=0 ; j<y ; j++) {
			if (j == (int)n)
				dlbar[i++] = '\x83';
			else
				dlbar[i++] = '\x81';
		}

		dlbar[i++] = '\x82';
		dlbar[i] = 0;

		Q_snprintfz (dlbar + strlen(dlbar), sizeof (dlbar), " %02d%%", cls.downloadPercent);

		// draw it
		Draw_String (NULL, 8, con.visLines - (14*FT_SCALE), FS_SHADOW, FT_SCALE, dlbar, colorWhite);

		con.visLines -= (FT_SIZE + FT_SHAOFFSET);
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}
