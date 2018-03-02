#include "g_local.h"

// On Selected callback.
void gmenu_selected(edict_t *ent, int option_id) {
	gi.cprintf(ent, PRINT_HIGH, "Menu_id: %d, Selected option_id: %i\n", ent->menu_id, option_id);
}

// Call whichever callback the func_menu had set for its fire function.

// Use function, setup activator, activate!
void func_menu_use(edict_t *self, edict_t *other, edict_t *activator) {
	// Best set this just to be sure.
	self->activator = activator;

	// Write out our menu function network message.
	gi.WriteByte(SVC_FUNC_MENU);
	gi.WriteByte(1); // Menu ID.
	gi.WriteByte(0); // Message ID.
	gi.WriteByte(0); // Option list ID.
	gi.unicast(self->activator, qTrue);
}

// Function menu entity spawn.
void func_menu(edict_t *ent) {
	// Setup use function.
	ent->use = func_menu_use;

	// Not sure if this is required... but do it anyway for now.
	ent->movetype = MOVETYPE_STOP;
	ent->solid = SOLID_NOT;
	gi.linkentity(ent);
}