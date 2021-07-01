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
// cl_console.c
//

#include "cl_local.h"

#define CON_TEXTSIZE	65536
#define CON_MAXTIMES	128

#define CON_NOTIFYTIMES	( clamp (con_notifylines->integer, 1, CON_MAXTIMES) )

typedef struct console_s {
	qBool	initialized;

	char	text[CON_TEXTSIZE];		// console text
	float	times[CON_MAXTIMES];	// cls.realTime time the line was generated
									// for transparent notify lines

	int		orMask;

	qBool	carriageReturn;			// last newline was '\r'
	int		xOffset;

	int		lastColor;				// color before last newline
	int		lastStyle;				// style before last newline
	int		currentLine;			// line where next message will be printed
	int		display;				// bottom of console displays this line

	float	visLines;
	int		totalLines;				// total lines in console scrollback
	int		lineWidth;				// characters across screen

	float	currentDrop;			// aproaches con_DropHeight at scr_conspeed->value
	float	dropHeight;				// 0.0 to 1.0 lines of console to display
} console_t;

console_t	cl_console;
console_t	cl_chatConsole;

/*
================
CL_ClearNotifyLines
================
*/
void CL_ClearNotifyLines (void)
{
	memset (&cl_console.times, 0, sizeof (cl_console.times));
	memset (&cl_chatConsole.times, 0, sizeof (cl_chatConsole.times));
}


/*
================
CL_ConsoleClose
================
*/
void CL_ConsoleClose (void)
{
	cl_console.currentDrop = 0;
	cl_chatConsole.currentDrop = 0;

	Key_ClearTyping ();
	CL_ClearNotifyLines ();
	Key_ClearStates ();
}


/*
================
CL_MoveConsoleDisplay
================
*/
void CL_MoveConsoleDisplay (int value)
{
	cl_console.display += value;
	if (cl_console.display > cl_console.currentLine)
		cl_console.display = cl_console.currentLine;
}


/*
================
CL_SetConsoleDisplay
================
*/
void CL_SetConsoleDisplay (qBool top)
{
	cl_console.display = cl_console.currentLine;
	if (top)
		cl_console.display -= cl_console.totalLines + 10;
}


/*
================
CL_ConsoleCheckResize

If the line width has changed, reformat the buffer.
================
*/
static void CL_ResizeConsole (console_t *console)
{
	int		i, j, width, oldwidth, oldtotalLines, numlines, numchars;
	char	tempbuf[CON_TEXTSIZE];

	if (cls.glConfig.vidWidth < 1) {
		// Video hasn't been initialized yet
		width = (512 / 8) - 2;
		console->lineWidth = width;
		console->totalLines = CON_TEXTSIZE / console->lineWidth;
		memset (console->text, ' ', CON_TEXTSIZE);
	}
	else {
		width = (cls.glConfig.vidWidth / FT_SIZE) - 2;
		if (width == console->lineWidth)
			return;

		oldwidth = console->lineWidth;
		console->lineWidth = width;
		oldtotalLines = console->totalLines;
		console->totalLines = CON_TEXTSIZE / console->lineWidth;
		numlines = oldtotalLines;

		if (console->totalLines < numlines)
			numlines = console->totalLines;

		numchars = oldwidth;
	
		if (console->lineWidth < numchars)
			numchars = console->lineWidth;

		memcpy (tempbuf, console->text, CON_TEXTSIZE);
		memset (console->text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++) {
			for (j=0 ; j<numchars ; j++) {
				console->text[(console->totalLines - 1 - i) * console->lineWidth + j] =
					tempbuf[((console->currentLine - i + oldtotalLines) % oldtotalLines) * oldwidth + j];
			}
		}

		CL_ClearNotifyLines ();
	}

	console->currentLine = max (1, console->totalLines - 1);
	console->display = console->currentLine;
}

void CL_ConsoleCheckResize (void)
{
	CL_ResizeConsole (&cl_console);
	CL_ResizeConsole (&cl_chatConsole);
}


/*
===============
CL_ConsoleLinefeed
===============
*/
static void CL_ConsoleLinefeed (console_t *console, qBool skipNotify)
{
	// Line feed
	if (console->display == console->currentLine)
		console->display++;

	console->currentLine++;
	memset (&console->text[(console->currentLine%console->totalLines)*console->lineWidth], ' ', console->lineWidth);

	// Set the time for notify lines
	console->times[console->currentLine % CON_MAXTIMES] = (skipNotify) ? 0 : cls.realTime;
}


/*
================
CL_ConsolePrintf

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
static void CL_PrintToConsole (console_t *console, comPrint_t flags, const char *txt)
{
	int			y, l;
	int			orMask;

	if (!console->initialized)
		return;

	if (txt[0] == 1 || txt[0] == 2) {
		orMask = 128;
		txt++;
	}
	else
		orMask = 0;

	while (*txt) {
		// Count word length
		for (l=0 ; l<console->lineWidth ; l++) {
			if (txt[l] <= ' ')
				break;
		}

		// Word wrap
		if (l != console->lineWidth && console->xOffset + l > console->lineWidth)
			console->xOffset = 0;

		if (console->carriageReturn) {
			console->currentLine = max (1, console->currentLine - 1);
			console->carriageReturn = qFalse;
		}

		if (!console->xOffset) {
			// Feed a line
			CL_ConsoleLinefeed (console, (flags & PRNT_CONSOLE));

			y = console->currentLine % console->totalLines;
			if (console->lastColor != -1 && y*console->lineWidth < CON_TEXTSIZE-2) {
				console->text[y*console->lineWidth] = Q_COLOR_ESCAPE;
				console->text[y*console->lineWidth+1] = '0' + console->lastColor;
				console->xOffset += 2;
			}

			if (console->lastStyle != -1 && y*console->lineWidth+console->xOffset < CON_TEXTSIZE-2) {
				console->text[y*console->lineWidth+console->xOffset] = Q_COLOR_ESCAPE;
				console->text[y*console->lineWidth+console->xOffset+1] = console->lastStyle;
				console->xOffset += 2;
			}
		}

		switch (*txt) {
		case '\n':
			console->lastColor = -1;
			console->lastStyle = -1;
			console->xOffset = 0;
			break;

		case '\r':
			console->lastColor = -1;
			console->lastStyle = -1;
			console->xOffset = 0;
			console->carriageReturn = qTrue;
			break;

		default:
			// Display character and advance
			y = console->currentLine % console->totalLines;
			console->text[y*console->lineWidth+console->xOffset] = *txt | orMask | console->orMask;
			if (++console->xOffset >= console->lineWidth)
				console->xOffset = 0;

			// Get old color/style codes
			if (Q_IsColorString (txt)) {
				switch ((*(txt+1)) & 127) {
				case 's':
				case 'S':
					if (console->lastStyle == 'S')
						console->lastStyle = -1;
					else
						console->lastStyle = 'S';
					break;

				case 'r':
				case 'R':
					console->lastStyle = -1;
					console->lastColor = -1;
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
					console->lastColor = Q_StrColorIndex (*(txt+1));
					break;

				default:
					console->lastColor = Q_StrColorIndex (COLOR_WHITE);
				}
			}
			break;
		}

		txt++;
	}
}

void CL_ConsolePrintf (comPrint_t flags, const char *txt)
{
	// Error/warning color coding
	if (flags & PRNT_ERROR)
		CL_PrintToConsole (&cl_console, flags, S_COLOR_RED);
	else if (flags & PRNT_WARNING)
		CL_PrintToConsole (&cl_console, flags, S_COLOR_YELLOW);

	// High-bit character list
	if (flags & PRNT_CHATHUD) {
		cl_console.orMask = 128;
		cl_chatConsole.orMask = 128;

		// Regular console
		if (con_chatHud->integer == 2)
			flags |= PRNT_CONSOLE;
	}

	// Regular console
	CL_PrintToConsole (&cl_console, flags, txt);

	// Chat console
	if (flags & PRNT_CHATHUD) {
		CL_PrintToConsole (&cl_chatConsole, flags, txt);

		cl_console.orMask = 0;
		cl_chatConsole.orMask = 0;
	}
}

/*
==============================================================================

	CONSOLE COMMANDS

==============================================================================
*/

/*
================
CL_ClearConsoleText_f
================
*/
static void CL_ClearConsoleText_f (void)
{
	// Reset line locations and clear the buffers
	cl_console.currentLine = max (1, cl_console.totalLines - 1);
	cl_console.display = cl_console.currentLine;
	memset (cl_console.text, ' ', CON_TEXTSIZE);

	cl_chatConsole.currentLine = max (1, cl_chatConsole.totalLines - 1);
	cl_chatConsole.display = cl_chatConsole.currentLine;
	memset (cl_chatConsole.text, ' ', CON_TEXTSIZE);
}


/*
================
CL_ConsoleDump_f

Save the console contents out to a file
================
*/
static void CL_ConsoleDump_f (void)
{
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf (0, "usage: condump <filename>\n");
		return;
	}

	if (strstr (Cmd_Argv (1), "."))
		Q_snprintfz (name, sizeof (name), "%s/condumps/%s", FS_Gamedir(), Cmd_Argv (1));
	else
		Q_snprintfz (name, sizeof (name), "%s/condumps/%s.txt", FS_Gamedir(), Cmd_Argv (1));

	Com_Printf (0, "Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f) {
		Com_Printf (PRNT_ERROR, "ERROR: couldn't open.\n");
		return;
	}

	// Skip empty lines
	for (l=cl_console.currentLine-cl_console.totalLines+1 ; l<=cl_console.currentLine ; l++) {
		line = cl_console.text + (l%cl_console.totalLines)*cl_console.lineWidth;
		for (x=0 ; x<cl_console.lineWidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != cl_console.lineWidth)
			break;
	}

	// Write the remaining lines
	buffer[cl_console.lineWidth] = 0;
	for ( ; l<=cl_console.currentLine ; l++) {
		line = cl_console.text + (l % cl_console.totalLines) * cl_console.lineWidth;
		strncpy (buffer, line, cl_console.lineWidth);
		for (x=cl_console.lineWidth-1 ; x>=0 ; x--) {
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}

		// Handle high-bit text
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}


/*
================
CL_ToggleConsole_f
================
*/
void CL_ToggleConsole_f (void)
{
	static keyDest_t	oldKD;

	// Kill the loading plaque
	SCR_EndLoadingPlaque ();

	if (cl.attractLoop) {
		Cbuf_AddText ("killserver\n");
		return;
	}

	Key_ClearTyping ();
	CL_ClearNotifyLines ();
	Key_ClearStates ();

	if (Key_GetDest () == KD_CONSOLE) {
		if (oldKD == KD_MESSAGE)
			Key_SetDest (KD_GAME);
		else
			Key_SetDest (oldKD);
	}
	else {
		oldKD = Key_GetDest ();
		Key_SetDest (KD_CONSOLE);
	}
}


/*
================
CL_ToggleChat_f
================
*/
static void CL_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (Key_GetDest () == KD_CONSOLE) {
		if (Com_ClientState () == CA_ACTIVE) {
			CL_CGModule_ForceMenuOff ();
			Key_SetDest (KD_GAME);
		}
	}
	else
		Key_SetDest (KD_CONSOLE);

	CL_ClearNotifyLines ();
}


/*
================
CL_MessageMode_f
================
*/
static void CL_MessageMode_f (void)
{
	if (Com_ClientState () != CA_ACTIVE)
		return;

	key_chatTeam = qFalse;
	Key_SetDest (KD_MESSAGE);
}


/*
================
CL_MessageMode2_f
================
*/
static void CL_MessageMode2_f (void)
{
	if (Com_ClientState () != CA_ACTIVE)
		return;

	key_chatTeam = qTrue;
	Key_SetDest (KD_MESSAGE);
}

/*
==============================================================================

	INIT / SHUTDOWN

==============================================================================
*/

/*
================
CL_ConsoleInit
================
*/
void CL_ConsoleInit (void)
{
	if (dedicated->integer)
		return;

	Cmd_AddCommand ("toggleconsole",	CL_ToggleConsole_f,		"Toggles displaying the console");
	Cmd_AddCommand ("togglechat",		CL_ToggleChat_f,		"");
	Cmd_AddCommand ("messagemode",		CL_MessageMode_f,		"");
	Cmd_AddCommand ("messagemode2",		CL_MessageMode2_f,		"");
	Cmd_AddCommand ("clear",			CL_ClearConsoleText_f,	"Clears the console buffer");
	Cmd_AddCommand ("condump",			CL_ConsoleDump_f,		"Dumps the content of the console to file");

	// Setup the console
	memset (&cl_console, 0, sizeof (console_t));
	cl_console.lineWidth = -1;
	cl_console.totalLines = -1;
	cl_console.lastColor = -1;
	cl_console.lastStyle = -1;

	// Setup the chat console
	memset (&cl_chatConsole, 0, sizeof (console_t));
	cl_chatConsole.lineWidth = -1;
	cl_chatConsole.totalLines = -1;
	cl_chatConsole.lastColor = -1;
	cl_chatConsole.lastStyle = -1;

	// Done
	CL_ConsoleCheckResize ();

	cl_console.initialized = qTrue;
	cl_chatConsole.initialized = qTrue;
}

/*
==============================================================================

	DRAWING

==============================================================================
*/

static int Q_ColorStrLastColor (char *s, int byteOfs)
{
	char	*end;
	int		lastClrIndex = Q_StrColorIndex (COLOR_WHITE);

	end = s + (byteOfs - 1);	// don't check last byte

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

static int Q_ColorStrLastStyle (char *s, int byteOfs)
{
	char	*end;
	int		lastStyle = 0;

	end = s + (byteOfs - 1);	// don't check last byte

	for ( ; *s && s<end ; s++) {
		if (Q_IsColorString (s)) {
			if ((toupper(s[1]) & 127) == 'S') {
				if (lastStyle & FS_SHADOW)
					lastStyle = 0;
				else
					lastStyle |= FS_SHADOW;
			}
			else if ((toupper(s[1]) & 127) == 'R')
				lastStyle = 0;

			s++;
		}
	}

	return lastStyle;
}

/*
================
CL_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
static void CL_DrawInput (void)
{
	char	*text;
	int		lastColor, lastFlags;
	int		colorLinePos;
	int		byteOfs;
	int		byteLen;

	if (Key_GetDest () == KD_MENU)
		return;

	// Don't draw anything (always draw if not active)
	if (Key_GetDest () != KD_CONSOLE && Com_ClientState () == CA_ACTIVE)
		return;

	text = key_consoleBuffer[key_consoleEditLine];

	// Convert byte offset to visible character count
	colorLinePos = Q_ColorCharCount (text, key_consoleLinePos);

	// Prestep if horizontally scrolling
	if (colorLinePos >= cl_console.lineWidth + 1) {
		byteOfs = Q_ColorCharOffset (text, colorLinePos - cl_console.lineWidth);
		lastColor = Q_ColorStrLastColor (text, byteOfs);
		lastFlags = Q_ColorStrLastStyle (text, byteOfs);
		text += byteOfs;
		colorLinePos = cl_console.lineWidth;
	}
	else {
		lastColor = Q_StrColorIndex (COLOR_WHITE);
		lastFlags = 0;
	}

	byteLen = Q_ColorCharOffset (text, cl_console.lineWidth);

	// Draw it
	R_DrawStringLen (NULL, 8, cl_console.visLines - (FT_SIZE * 2), lastFlags, FT_SCALE, text, byteLen, strColorTable[lastColor]);

	// Add the cursor
	if (((int)(cls.realTime>>8))&1)
		R_DrawChar (NULL, 8 + ((colorLinePos - (key_insertOn ? 0.3 : 0)) * FT_SIZE), cl_console.visLines - (FT_SIZE * 2),
					0, FT_SCALE, key_insertOn ? '|' : 11, colorWhite);
}


/*
================
CL_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
static void CL_DrawNotify (void)
{
	int		v, i, time;
	int		notifyLines;
	int		totalLines;
	char	*str;
	float	newScale = FT_SCALE;
	float	newSize = FT_SIZE;
	vec4_t	color;
	Vector4Copy (colorWhite, color);

	// Make larger if desired
	if (con_notifylarge->integer) {
		newScale *= 1.25;
		newSize *= 1.25;
	}

	// Render notify lines
	if (con_notifylines->integer) {
		for (totalLines=0, i=cl_console.currentLine ; i>cl_console.currentLine-CON_MAXTIMES+1 ; i--) {
			if (totalLines >= CON_NOTIFYTIMES)
				break;

			time = cl_console.times[i % CON_MAXTIMES];
			if (time == 0)
				continue;
			time = cls.realTime - time;
			if (time > con_notifytime->value * 1000)
				continue;
			totalLines++;
		}

		for (v=newSize*(totalLines-1), notifyLines=0, i=cl_console.currentLine ; i>cl_console.currentLine-CON_MAXTIMES+1 ; i--) {
			if (notifyLines >= CON_NOTIFYTIMES)
				break;

			time = cl_console.times[i % CON_MAXTIMES];
			if (time == 0)
				continue;
			time = cls.realTime - time;
			if (time > con_notifytime->value * 1000)
				continue;
			notifyLines++;

			if (con_notifyfade->integer)
				color[3] = 0.25 + 0.75 * (con_notifytime->value - (time * 0.001)) / con_notifytime->value;

			R_DrawStringLen (NULL, 8, v, 0, newScale, cl_console.text + (i % cl_console.totalLines)*cl_console.lineWidth, cl_console.lineWidth, color);

			v -= newSize;
		}
		v = newSize*totalLines;
	}
	else {
		v = 0;
	}

	//
	// Print messagemode input
	//
	if (Key_GetDest () == KD_MESSAGE) {
		int		skip;
		int		lastColor, lastFlags;
		int		colorLinePos;
		int		byteOfs;

		if (key_chatTeam)
			skip = R_DrawString (NULL, 4, v, 0, newScale, "say_team:", colorWhite) + 1;
		else
			skip = R_DrawString (NULL, 4, v, 0, newScale, "say:", colorWhite) + 1;

		str = key_chatBuffer[key_chatEditLine];

		// Convert byte offset to visible character count
		colorLinePos = Q_ColorCharCount (str, key_chatLinePos) + skip + 1;

		// Prestep if horizontally scrolling
		if (colorLinePos >= (int)(cls.glConfig.vidWidth/newSize)) {
			byteOfs = Q_ColorCharOffset (str, colorLinePos - (int)(cls.glConfig.vidWidth/newSize));

			lastColor = Q_ColorStrLastColor (str, byteOfs);
			lastFlags = Q_ColorStrLastStyle (str, byteOfs);

			str += byteOfs;
		}
		else {
			lastColor = Q_StrColorIndex (COLOR_WHITE);
			lastFlags = 0;
		}

		R_DrawString (NULL, skip * newSize, v, lastFlags, newScale, str, strColorTable[lastColor]);

		// Add cursor
		if (((int)(cls.realTime>>8))&1) {
			int charCount = Q_ColorCharCount (key_chatBuffer[key_chatEditLine], key_chatLinePos) + skip;
			if (charCount > (int)(cls.glConfig.vidWidth/newSize) - 1)
				charCount = (int)(cls.glConfig.vidWidth/newSize) - 1;

			R_DrawChar (NULL, ((charCount - (key_insertOn ? 0.3 : 0)) * newSize), v, 0, newScale,
						key_insertOn ? '|' : 11, colorWhite);
		}

		v += newSize;
	}
}


/*
==================
CL_DrawChatHud
==================
*/
static void CL_DrawChatHud (void)
{
	int		totalLines, v, i;
	char	*text;

	if (Com_ClientState () != CA_ACTIVE)
		return;
	if (Key_GetDest () == KD_MENU)
		return;
	if (cls.mapLoading)
		return;

	if (con_chatHudPosY->value < 0)
		v = cls.glConfig.vidHeight - (FT_SIZE * -con_chatHudPosY->value);
	else
		v = cls.glConfig.vidHeight + (FT_SIZE * con_chatHudPosY->value);

	totalLines = 0;
	for (i=cl_chatConsole.currentLine ; i>cl_chatConsole.currentLine-CON_MAXTIMES+1 ; i--) {
		if (totalLines == con_chatHudLines->integer)
			break;

		text = cl_chatConsole.text + (i % cl_chatConsole.totalLines)*cl_chatConsole.lineWidth;
		R_DrawStringLen (NULL, con_chatHudPosX->value, v, con_chatHudShadow->integer ? FS_SHADOW : 0, FT_SCALE, text, cl_chatConsole.lineWidth, colorWhite);

		totalLines++;
		v -= FT_SIZE;
	}
}


/*
==================
CL_RunConsole

Scroll it up or down
==================
*/
static void CL_RunConsole (void)
{
	CL_ConsoleCheckResize ();

	// Decide on the height of the console
	if (Key_GetDest () == KD_CONSOLE)
		cl_console.dropHeight = con_drop->value;
	else
		cl_console.dropHeight = 0;			// none visible
	
	if (cl_console.dropHeight < cl_console.currentDrop) {
		cl_console.currentDrop -= scr_conspeed->value*cls.refreshFrameTime;
		if (cl_console.dropHeight > cl_console.currentDrop)
			cl_console.currentDrop = cl_console.dropHeight;

	}
	else if (cl_console.dropHeight > cl_console.currentDrop) {
		cl_console.currentDrop += scr_conspeed->value*cls.refreshFrameTime;
		if (cl_console.dropHeight < cl_console.currentDrop)
			cl_console.currentDrop = cl_console.dropHeight;
	}
}


/*
================
CL_DrawConsole
================
*/
void CL_DrawConsole (void)
{
	float			frac = 0.0f;
	int				i, row;
	float			x, y, lines;
	vec4_t			conColor;
	char			version[32];

	// Advance for next frame
	CL_RunConsole ();

	// Draw the chat hud
	if (con_chatHud->integer && con_chatHudLines->integer > 0)
		CL_DrawChatHud ();

	if (cl_console.currentDrop)
		frac = cl_console.currentDrop;
	else if (Com_ClientState () == CA_ACTIVE && (Key_GetDest () == KD_GAME || Key_GetDest () == KD_MESSAGE) && !cls.mapLoading) {
		CL_DrawNotify ();	// Only draw notify in game
		return;
	}

	// Check if it's even visible
	cl_console.visLines = cls.glConfig.vidHeight * clamp (frac, 0, 1);
	if (cl_console.visLines <= 0)
		return;

	// Draw the background
	conColor[0] = colorWhite[0];
	conColor[1] = colorWhite[1];
	conColor[2] = colorWhite[2];
	conColor[3] = con_alpha->value;
	R_DrawPic (clMedia.consoleShader, 0, 0, -(cls.glConfig.vidHeight - cls.glConfig.vidHeight*frac), cls.glConfig.vidWidth, cls.glConfig.vidHeight, 0, 0, 1, 1, conColor);

	// Version
	Q_snprintfz (version, sizeof (version), "EGL v%s", EGL_VERSTR);
	R_DrawString (NULL, cls.glConfig.vidWidth - (strlen (version) * FT_SIZE) - (FT_SHAOFFSET * 2),
				cl_console.visLines - (FT_SIZE + (FT_SHAOFFSET * 2)),
				FS_SHADOW, FT_SCALE, version, colorCyan);

	// Time if desired
	if (con_clock->integer) {
		char		clockStr[16];
		time_t		ctime;
		struct tm	*ltime;

		ctime = time (NULL);
		ltime = localtime (&ctime);
		strftime (clockStr, sizeof (clockStr), "%I:%M %p", ltime);

		// Kill the initial zero
		if (clockStr[0] == '0') {
			for (i=0 ; i<(int)(strlen (clockStr) + 1) ; i++)
				clockStr[i] = clockStr[i+1];
			clockStr[i+1] = '\0';
		}

		R_DrawString (NULL, cls.glConfig.vidWidth - (strlen (clockStr) * FT_SIZE) - (FT_SHAOFFSET * 2),
					cl_console.visLines - ((FT_SIZE + (FT_SHAOFFSET * 2)) * 2),
					FS_SHADOW, FT_SCALE, clockStr, colorCyan);
	}

	// Draw the console text
	y = cl_console.visLines - (FT_SIZE * 3);	// start point (from the bottom)
	lines = (y / FT_SIZE);				// lines of text to draw

	// Draw arrows to show the buffer is backscrolled
	if (cl_console.display != cl_console.currentLine) {
		for (x=0 ; x<((cl_console.lineWidth+1)*(FT_SIZE)) ; x+=(FT_SIZE*2))
			R_DrawChar (NULL, x + (FT_SIZE * 0.5), y, FS_SHADOW, FT_SCALE, '^', colorRed);

		y -= (FT_SIZE + FT_SHAOFFSET);
		lines--;
	}

	// Draw the print, from the bottom up
	for (i=0, row=cl_console.display ; i<lines ; i++, y -= (FT_SIZE + FT_SHAOFFSET), row--) {
		if (row < 0)
			break;
		if (cl_console.currentLine - row >= cl_console.totalLines)
			break;	// past scrollback wrap point
		if (y < -(FT_SIZE + FT_SHAOFFSET))
			break;

		R_DrawStringLen (NULL, 8, y, 0, FT_SCALE, cl_console.text + (row % cl_console.totalLines) * cl_console.lineWidth, cl_console.lineWidth, colorWhite);
	}

	// Draw the input prompt, user text, and cursor if desired
	CL_DrawInput ();
}
