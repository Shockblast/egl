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
// gui_events.c
//

#include "gui_local.h"

/*
==================
GUI_RunEvent

Processes event actions.
==================
*/
static void GUI_RunEvent (gui_t *gui, guiEvent_t *event)
{
	evAction_t	*action;
	uint32		i;

	for (i=0, action=event->actionList ; i<event->numActions ; action++, i++) {
		switch (action->type) {
		case WEA_COMMAND:
		case WEA_IF:
		case WEA_LOCAL_SOUND:
		case WEA_LOCK_CURSOR:
		case WEA_RESET_TIME:
		case WEA_SET:
		case WEA_SET_FOCUS:
		case WEA_STOP_TRANSITIONS:
		case WEA_TRANSITION:
			break;
		}
	}
}

/*
=============================================================================

	EVENT QUEUE

	Events can be saved here as "triggers", so that on the next render call
	they will trigger the appropriate event.
=============================================================================
*/

/*
==================
GUI_FireTriggers

Triggers any queued events, along with the per-frame and time-based events.
==================
*/
void GUI_FireTriggers (gui_t *gui)
{
	gui_t		*child;
	guiEvent_t	*event;
	uint32		i;

	// Process events
	for (i=0, event=gui->eventList ; i<gui->numEvents ; event++, i++) {
		switch (event->type) {
		case WEV_FRAME:
			// This is triggered every frame
			GUI_RunEvent (gui, event);
			break;

		case WEV_TIME:
			if (event->intArgs[0][0] == Sys_Milliseconds ())
				GUI_RunEvent (gui, event);
			break;
		}
	}

	// Fire queued triggers
	for (i=0 ; i<gui->numTriggers ; i++)
		;

	// Recurse down the children
	for (i=0, child=gui->childList ; i<gui->numChildren ; child++, i++)
		GUI_FireTriggers (child);
}


/*
==================
GUI_QueueTrigger
==================
*/
void GUI_QueueTrigger (gui_t *gui, guiEventType_t trigger)
{
}
