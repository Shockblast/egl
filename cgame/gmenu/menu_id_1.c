#include "../cg_local.h"
#include "../menu/m_local.h"

//
//
//
// NOTE: THIS FILE IS PROTOTYPING, IT HAS TO BE CLEANED UP BY SPLITTING IN SEPARATE FILES.
// AND ALSO BY GENERIZING THIS SYSTEM SO IT CAN ACTIVATE REGISTERED MENU_IDS.
//
//
//

typedef struct g_messagelist_s
{
	char			*message;
} g_messagelist_t;
typedef struct g_optionlist_s
{
	char			*message;

	qBool(*option_use)(char idx);
} g_optionlist_t;

// TODO: Clean up.
#define MENU_OPTION_LIMIT 4

typedef struct m_menu_id_1_s {
	// Menu items
	uiFrameWork_t		frameWork;

	uiAction_t			message_text;
	uiAction_t			option_actions[MENU_OPTION_LIMIT]; // LIMIT = 4..

	// These are important, since they supply the data.
	// NOTE: menu_id is not here, it is elsewhere since it is
	// used with registering etc.
	char				message_id;
	char				optionlist_id;
} m_menu_id_1_t;

// Vars, temp too...
static m_menu_id_1_t m_menu_id_1;
qBool menu_init;

// These are important, should be redefined in each file for code to be generic.
g_messagelist_t m_menu_messages[] = {
	{
		"I'm just a boring text message. ID #0",
	},
	{
		"The sound of silence bro, really. ID #1",
	},
	{
		NULL
	}
};

g_optionlist_t m_menu_options[] = {
	{
		"Option A may be okay. ID #0",
		NULL
	},
	{
		"Option B makes me pee. ID #1",
		NULL
	},
	{
		"Option C is not for me. ID #2",
		NULL
	},
	{
		"Option D is the death of me. ID #3",
		NULL
	}
};

static void menu_id_1_no(void *unused) {
	Com_Printf(PRNT_CHATHUD, "NO");
	UI_PopInterface();
}
static void menu_id_1_action(void *self) {
	uiAction_t *a = (uiAction_t *)self;

	cgi.Cbuf_AddText(Q_VarArgs("menu_option %i\n", a->generic.localData[0]));
	cgi.Cbuf_Execute();

	Com_Printf(PRNT_CHATHUD, "YES");
	Com_Printf(PRNT_CHATHUD, Q_VarArgs("menu_option %i\n", a->generic.localData[0]));
	UI_PopInterface();
}

void menu_id_1_init() {
	menu_init = qTrue;

	// Init menu...
	UI_StartFramework(&m_menu_id_1.frameWork, FWF_CENTERHEIGHT);

	//
	// Generate message.
	//
	// TODO: Do out of bounds error checking... 
	// If oob, make it say so which message ID is the cause: "Out of bounds, message_id: #.."
	m_menu_id_1.message_text.generic.type = UITYPE_ACTION;
	m_menu_id_1.message_text.generic.flags = UIF_NOSELECT | UIF_CENTERED | UIF_LARGE | UIF_SHADOW;
	m_menu_id_1.message_text.generic.name = m_menu_messages[m_menu_id_1.message_id].message;
	UI_AddItem(&m_menu_id_1.frameWork, &m_menu_id_1.message_text);

	//
	// Generate menu options depending on which ID.
	//
	// TODO: Add the room for a space (empty option)
	// TODO: Make sure it is based on size maybe instead of a fixed limit.
	// TODO: Make sure it does out of bounds checking.
	// TODO: Maybe allow a custom callback?
	int i = 0;
	for (i = 0; i < MENU_OPTION_LIMIT; i++) {
		// Add if text is available, otherwise break out.
		if (m_menu_options[i].message == NULL) {
			break;
		}

		m_menu_id_1.option_actions[i].generic.type = UITYPE_ACTION;
		m_menu_id_1.option_actions[i].generic.flags = UIF_CENTERED;
		m_menu_id_1.option_actions[i].generic.name = m_menu_options[i].message;
		m_menu_id_1.option_actions[i].generic.callBack = menu_id_1_action;
		m_menu_id_1.option_actions[i].generic.localData[0] = i;
		UI_AddItem(&m_menu_id_1.frameWork, &m_menu_id_1.option_actions[i]);
	}

	UI_FinishFramework(&m_menu_id_1.frameWork, qTrue);
}

static void menu_id_1_draw(void) {
	float	y;
	y = 0;
	// Initialize if necessary
	if (!m_menu_id_1.frameWork.initialized)
		menu_id_1_init();

	// Dynamically position
	m_menu_id_1.frameWork.x = cg.refConfig.vidWidth * 0.5f;
	m_menu_id_1.frameWork.y = 0;

	m_menu_id_1.message_text.generic.x = 0;
	m_menu_id_1.message_text.generic.y = 0;

	int i = 0;
	for (i = 0; i < MENU_OPTION_LIMIT; i++) {
		// Add if text is available, otherwise break out.
		if (m_menu_options[i].message == NULL) {
			break;
		}

		m_menu_id_1.option_actions[i].generic.x = 0;
		m_menu_id_1.option_actions[i].generic.y = y += UIFT_SIZEINCLG;
	}

	// Render
	UI_DrawInterface(&m_menu_id_1.frameWork);
}

static struct sfx_s *menu_id_1_close(void)
{
	return uiMedia.sounds.menuOut;
}

// Activate this menu.
void menu_id_1_activate(char menu_id, char message_id, char optionlist_id) {
	// This is temp...
	if (menu_init == qFalse) {
		menu_id_1_init();
	}

	// Store IDs for this menu functionality.
	m_menu_id_1.message_id = message_id;
	m_menu_id_1.optionlist_id = optionlist_id;

	// Push menu.
	M_PushMenu(&m_menu_id_1.frameWork, menu_id_1_draw, menu_id_1_close, M_KeyHandler);
}