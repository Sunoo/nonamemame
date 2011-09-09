//============================================================
//
//	input.c - Win32 implementation of MAME input routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#ifdef WINXPANANLOG
#ifndef WINUI
#define _WIN32_WINNT 0x501
#endif /* WINUI */
#endif /* WINXPANANLOG */
#include <windows.h>
#include <conio.h>
#include <winioctl.h>

// undef WINNT for dinput.h to prevent duplicate definition
#undef WINNT
#include <dinput.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "rc.h"
#include "input.h"
#ifdef WINXPANANLOG
/* <jake> */
#include "raw_mouse.h"
/* </jake> */
#endif /* WINXPANANLOG */

//#include <stdlib.h>
//int snprintf (char *str, size_t count, const char *fmt, ...);	// from snprintf.c


//============================================================
//	IMPORTS
//============================================================

extern int verbose;
extern int win_physical_width;
extern int win_physical_height;
extern int win_window_mode;


//============================================================
//	PARAMETERS
//============================================================

#define MAX_KEYBOARDS		1
/*start MAME:analog+*/
#define MAX_MICE			MAX_PLAYERS + 1	// The extra one is the "system mouse"
#define MAX_JOYSTICKS		MAX_PLAYERS		// now that MAX_PLAYERS is 8
/*end MAME:analog+  */

#define MAX_KEYS			256

#define MAX_JOY				256
#define MAX_AXES			8
#define MAX_BUTTONS			32
#define MAX_POV				4



//============================================================
//	MACROS
//============================================================

#define STRUCTSIZE(x)		((dinput_version == 0x0300) ? sizeof(x##_DX3) : sizeof(x))

#define ELEMENTS(x)			(sizeof(x) / sizeof((x)[0]))



//============================================================
//	GLOBAL VARIABLES
//============================================================

UINT8						win_trying_to_quit;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectInput variables
static LPDIRECTINPUT		dinput;
static int					dinput_version;

// global states
static int					input_paused;
static cycles_t				last_poll;

// Controller override options
//static int					hotrod;
//static int					hotrodse;
static float				a2d_deadzone;
static int					use_mouse;
static int					use_joystick;
static int					use_lightgun;
static int					use_lightgun_dual;
static int					use_lightgun_reload;
static int					use_keyboard_leds;
static const char *			ledmode;
static int					steadykey;
static const char*			ctrlrtype;
static const char*			ctrlrname;
static const char*			trackball_ini;
static const char*			paddle_ini;
static const char*			dial_ini;
static const char*			ad_stick_ini;
static const char*			pedal_ini;
static const char*			lightgun_ini;

// this is used for the ipdef_custom_rc_func
static struct ipd 			*ipddef_ptr = NULL;

static int					num_osd_ik = 0;
static int					size_osd_ik = 0;

/*start MAME:analog+*/
//int							analog_pedal;
static int					singlemouse;
int							switchmice;
int							switchaxes;
int							splitmouse;
int							resetmouse;
#ifdef WINXPANANLOG
/* <jake> */
static int use_rawmouse_winxp;
static int acquire_raw_mouse;
/* </jake> */
#endif /* WINXPANANLOG */

// two different multi-lightgun options
static int					use_lightgun2a;
static int					use_lightgun2b;

// debug, should be moved back to function after debugging finished
static int initlightgun2[MAX_PLAYERS]= {2,2,2,2,2,2,2,2};
static int maxdeltax[MAX_PLAYERS]={1,1,1,1,1,1,1,1}, maxdeltay[MAX_PLAYERS]={1,1,1,1,1,1,1,1};
//static int minmx[4], minmy[4], maxmx[4], maxmy[4];
static int lg_center_x[MAX_PLAYERS], lg_center_y[MAX_PLAYERS];
/*end MAME:analog+  */

// keyboard states
static int					keyboard_count;
static LPDIRECTINPUTDEVICE	keyboard_device[MAX_KEYBOARDS];
static LPDIRECTINPUTDEVICE2	keyboard_device2[MAX_KEYBOARDS];
static DIDEVCAPS			keyboard_caps[MAX_KEYBOARDS];
static BYTE					keyboard_state[MAX_KEYBOARDS][MAX_KEYS];

// additional key data
static INT8					oldkey[MAX_KEYS];
static INT8					currkey[MAX_KEYS];

// mouse states
static int					mouse_active;
static int					mouse_count;
static LPDIRECTINPUTDEVICE	mouse_device[MAX_MICE];
static LPDIRECTINPUTDEVICE2	mouse_device2[MAX_MICE];
static DIDEVCAPS			mouse_caps[MAX_MICE];
static DIMOUSESTATE			mouse_state[MAX_MICE];
static int					lightgun_count;
static POINT				lightgun_dual_player_pos[4];
static int					lightgun_dual_player_state[4];

// joystick states
static int					joystick_count;
static LPDIRECTINPUTDEVICE	joystick_device[MAX_JOYSTICKS];
static LPDIRECTINPUTDEVICE2	joystick_device2[MAX_JOYSTICKS];
static DIDEVCAPS			joystick_caps[MAX_JOYSTICKS];
static DIJOYSTATE			joystick_state[MAX_JOYSTICKS];
static DIPROPRANGE			joystick_range[MAX_JOYSTICKS][MAX_AXES];


/*start MAME:analog+*/
static int					joystick_setrange[MAX_JOYSTICKS][MAX_AXES];  // boolean if range was set

// mappable mouse axes stuff
static int num_axis_per_mouse[MAX_MICE];

struct Mouse_Axes
{
	int xmouse;
	int xaxis;
	int ymouse;
	int yaxis;
};

// analog tables for the mice
static struct Mouse_Axes os_player_mouse_axes[MAX_PLAYERS];

// set with following const, edit them as needed
// TODO: do this in the UI
const int mouse_map_default[MAX_PLAYERS] = {0,1,2,3,4,5,6,7};	// default mame mapping
const int mouse_map_split[MAX_PLAYERS]	= {1,1,2,2,3,3,4,4};	// split mice default mapping
const int mouse_map_usbs[MAX_PLAYERS]	= {1,2,3,4,5,6,7,0};	// if usb is present, map it to p1 and "system mouse" to p4
const int mouse_map_8usbs[MAX_PLAYERS]	= {1,2,3,4,5,6,7,8};	// 4 usb present, map only to them and no "system mouse"
/*end MAME:analog+  */

#ifdef WINXPANANLOG
/* <jake> */
// Storage for values to be read at the beginning of the input read cycle - the "+1" is to include the sys mouse
static ULONG raw_mouse_delta_x[MAX_PLAYERS + 1] = {0,0,0,0,0,0,0,0,0};
static ULONG raw_mouse_delta_y[MAX_PLAYERS + 1] = {0,0,0,0,0,0,0,0,0};
static ULONG raw_mouse_delta_z[MAX_PLAYERS + 1] = {0,0,0,0,0,0,0,0,0};
/* </jake> */

#endif /* WINXPANANLOG */
// led states
static int original_leds;
static HANDLE hKbdDev;
static int ledmethod;

//============================================================
//	OPTIONS
//============================================================

// prototypes
static int decode_ledmode(struct rc_option *option, const char *arg, int priority);

// global input options
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input device options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
//	{ "hotrod", NULL, rc_bool, &hotrod, "0", 0, 0, NULL, "preconfigure for hotrod" },
//	{ "hotrodse", NULL, rc_bool, &hotrodse, "0", 0, 0, NULL, "preconfigure for hotrod se" },
	{ "mouse", NULL, rc_bool, &use_mouse, "0", 0, 0, NULL, "enable mouse input" },
	{ "joystick", "joy", rc_bool, &use_joystick, "0", 0, 0, NULL, "enable joystick input" },
	{ "lightgun", "gun", rc_bool, &use_lightgun, "0", 0, 0, NULL, "enable lightgun input" },
	{ "dual_lightgun", "dual", rc_bool, &use_lightgun_dual, "0", 0, 0, NULL, "enable dual lightgun input" },
	{ "offscreen_reload", "reload", rc_bool, &use_lightgun_reload, "0", 0, 0, NULL, "offscreen shots reload" },
	{ "steadykey", "steady", rc_bool, &steadykey, "0", 0, 0, NULL, "enable steadykey support" },
	{ "keyboard_leds", "leds", rc_bool, &use_keyboard_leds, "1", 0, 0, NULL, "enable keyboard LED emulation" },
	{ "led_mode", NULL, rc_string, &ledmode, "ps/2", 0, 0, decode_ledmode, "LED mode (ps/2|usb)" },
	{ "a2d_deadzone", "a2d", rc_float, &a2d_deadzone, "0.3", 0.0, 1.0, NULL, "minimal analog value for digital input" },
	{ "ctrlr", NULL, rc_string, &ctrlrtype, 0, 0, 0, NULL, "preconfigure for specified controller" },
/*start MAME:analog+*/
	{ "Analog+ options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
//	{ "analogpedal", "anapedal", rc_bool, &analog_pedal, "1", 0, 0, NULL, "enable analog pedal" },
	{ "singlemouse", "onemouse", rc_bool, &singlemouse, "0", 0, 0, NULL, "allow only one mouse device" },
	{ "switchablemice", "switchmice", rc_bool, &switchmice, "0", 0, 0, NULL, "enable switching mouse -> player" },
	{ "switchmiceaxes", "switchaxes", rc_bool, &switchaxes, "0", 0, 0, NULL, "enable assigning mouse axis -> player axis" },
	{ "splitmouseaxes", "splitmouse", rc_bool, &splitmouse, "0", 0, 0, NULL, "automatically map one mouse axis per player from commandline/ini" },
	{ "resetmouseaxes", "resetmouse", rc_bool, &resetmouse, "0", 0, 0, NULL, "reset analog+ mouse settings for this game" },
	{ "lightgun2a", "gun2a", rc_bool, &use_lightgun2a, "0", 0, 0, NULL, "use 2 USB mouse lightguns with relative mode" },
	{ "lightgun2b", "gun2b", rc_bool, &use_lightgun2b, "0", 0, 0, NULL, "use 2 USB mouse lightguns with absolute mode" },
#ifdef WINXPANANLOG
	/* <jake> */
	{ "multimouse_winxp", "multimousexp", rc_bool, &use_rawmouse_winxp, "0", 0, 0, NULL, "use multiple mice (WinXP only) with rawmouse, -mouse required" },
	/* </jake> */
#endif /* WINXPANANLOG */
/*end MAME:analog+  */
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

struct rc_option *ctrlr_input_opts = NULL;

struct rc_option ctrlr_input_opts2[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "ctrlrname", NULL, rc_string, &ctrlrname, 0, 0, 0, NULL, "name of controller" },
	{ "trackball_ini", NULL, rc_string, &trackball_ini, 0, 0, 0, NULL, "ctrlr opts if game has TRACKBALL input" },
	{ "paddle_ini", NULL, rc_string, &paddle_ini, 0, 0, 0, NULL, "ctrlr opts if game has PADDLE input" },
	{ "dial_ini", NULL, rc_string, &dial_ini, 0, 0, 0, NULL, "ctrlr opts if game has DIAL input" },
	{ "ad_stick_ini", NULL, rc_string, &ad_stick_ini, 0, 0, 0, NULL, "ctrlr opts if game has AD STICK input" },
	{ "lightgun_ini", NULL, rc_string, &lightgun_ini, 0, 0, 0, NULL, "ctrlr opts if game has LIGHTGUN input" },
	{ "pedal_ini", NULL, rc_string, &pedal_ini, 0, 0, 0, NULL, "ctrlr opts if game has PEDAL input" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};


//============================================================
//	decode_cleanstretch
//============================================================

static int decode_ledmode(struct rc_option *option, const char *arg, int priority)
{
	if( strcmp( arg, "ps/2" ) != 0 &&
		strcmp( arg, "usb" ) != 0 )
	{
		fprintf(stderr, "error: invalid value for led_mode: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

//============================================================
//	PROTOTYPES
//============================================================

static void updatekeyboard(void);
static void init_keylist(void);
static void init_joylist(void);

/*start MAME:analog+*/
//static void init_analogjoy_arrays(void);
static void init_mouse_arrays(const int playermousedefault[]);
/*end MAME:analog+  */


//============================================================
//	KEYBOARD LIST
//============================================================

// this will be filled in dynamically
static struct KeyboardInfo keylist[MAX_KEYS];

// macros for building/mapping keycodes
#define KEYCODE(dik, vk, ascii)		((dik) | ((vk) << 8) | ((ascii) << 16))
#define DICODE(keycode)				((keycode) & 0xff)
#define VKCODE(keycode)				(((keycode) >> 8) & 0xff)
#define ASCIICODE(keycode)			(((keycode) >> 16) & 0xff)

// master translation table
const int win_key_trans_table[][4] =
{
	// MAME key				dinput key			virtual key		ascii
	{ KEYCODE_ESC, 			DIK_ESCAPE,			VK_ESCAPE,	 	27 },
	{ KEYCODE_1, 			DIK_1,				'1',			'1' },
	{ KEYCODE_2, 			DIK_2,				'2',			'2' },
	{ KEYCODE_3, 			DIK_3,				'3',			'3' },
	{ KEYCODE_4, 			DIK_4,				'4',			'4' },
	{ KEYCODE_5, 			DIK_5,				'5',			'5' },
	{ KEYCODE_6, 			DIK_6,				'6',			'6' },
	{ KEYCODE_7, 			DIK_7,				'7',			'7' },
	{ KEYCODE_8, 			DIK_8,				'8',			'8' },
	{ KEYCODE_9, 			DIK_9,				'9',			'9' },
	{ KEYCODE_0, 			DIK_0,				'0',			'0' },
	{ KEYCODE_MINUS, 		DIK_MINUS, 			0xbd,			'-' },
	{ KEYCODE_EQUALS, 		DIK_EQUALS,		 	0xbb,			'=' },
	{ KEYCODE_BACKSPACE,	DIK_BACK, 			VK_BACK, 		8 },
	{ KEYCODE_TAB, 			DIK_TAB, 			VK_TAB, 		9 },
	{ KEYCODE_Q, 			DIK_Q,				'Q',			'Q' },
	{ KEYCODE_W, 			DIK_W,				'W',			'W' },
	{ KEYCODE_E, 			DIK_E,				'E',			'E' },
	{ KEYCODE_R, 			DIK_R,				'R',			'R' },
	{ KEYCODE_T, 			DIK_T,				'T',			'T' },
	{ KEYCODE_Y, 			DIK_Y,				'Y',			'Y' },
	{ KEYCODE_U, 			DIK_U,				'U',			'U' },
	{ KEYCODE_I, 			DIK_I,				'I',			'I' },
	{ KEYCODE_O, 			DIK_O,				'O',			'O' },
	{ KEYCODE_P, 			DIK_P,				'P',			'P' },
	{ KEYCODE_OPENBRACE,	DIK_LBRACKET, 		0xdb,			'[' },
	{ KEYCODE_CLOSEBRACE,	DIK_RBRACKET, 		0xdd,			']' },
	{ KEYCODE_ENTER, 		DIK_RETURN, 		VK_RETURN, 		13 },
	{ KEYCODE_LCONTROL, 	DIK_LCONTROL, 		VK_CONTROL, 	0 },
	{ KEYCODE_A, 			DIK_A,				'A',			'A' },
	{ KEYCODE_S, 			DIK_S,				'S',			'S' },
	{ KEYCODE_D, 			DIK_D,				'D',			'D' },
	{ KEYCODE_F, 			DIK_F,				'F',			'F' },
	{ KEYCODE_G, 			DIK_G,				'G',			'G' },
	{ KEYCODE_H, 			DIK_H,				'H',			'H' },
	{ KEYCODE_J, 			DIK_J,				'J',			'J' },
	{ KEYCODE_K, 			DIK_K,				'K',			'K' },
	{ KEYCODE_L, 			DIK_L,				'L',			'L' },
	{ KEYCODE_COLON, 		DIK_SEMICOLON,		0xba,			';' },
	{ KEYCODE_QUOTE, 		DIK_APOSTROPHE,		0xde,			'\'' },
	{ KEYCODE_TILDE, 		DIK_GRAVE, 			0xc0,			'`' },
	{ KEYCODE_LSHIFT, 		DIK_LSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_BACKSLASH,	DIK_BACKSLASH, 		0xdc,			'\\' },
	{ KEYCODE_Z, 			DIK_Z,				'Z',			'Z' },
	{ KEYCODE_X, 			DIK_X,				'X',			'X' },
	{ KEYCODE_C, 			DIK_C,				'C',			'C' },
	{ KEYCODE_V, 			DIK_V,				'V',			'V' },
	{ KEYCODE_B, 			DIK_B,				'B',			'B' },
	{ KEYCODE_N, 			DIK_N,				'N',			'N' },
	{ KEYCODE_M, 			DIK_M,				'M',			'M' },
	{ KEYCODE_COMMA, 		DIK_COMMA,			0xbc,			',' },
	{ KEYCODE_STOP, 		DIK_PERIOD, 		0xbe,			'.' },
	{ KEYCODE_SLASH, 		DIK_SLASH, 			0xbf,			'/' },
	{ KEYCODE_RSHIFT, 		DIK_RSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_ASTERISK, 	DIK_MULTIPLY, 		VK_MULTIPLY,	'*' },
	{ KEYCODE_LALT, 		DIK_LMENU, 			VK_MENU, 		0 },
	{ KEYCODE_SPACE, 		DIK_SPACE, 			VK_SPACE,		' ' },
	{ KEYCODE_CAPSLOCK, 	DIK_CAPITAL, 		VK_CAPITAL, 	0 },
	{ KEYCODE_F1, 			DIK_F1,				VK_F1, 			0 },
	{ KEYCODE_F2, 			DIK_F2,				VK_F2, 			0 },
	{ KEYCODE_F3, 			DIK_F3,				VK_F3, 			0 },
	{ KEYCODE_F4, 			DIK_F4,				VK_F4, 			0 },
	{ KEYCODE_F5, 			DIK_F5,				VK_F5, 			0 },
	{ KEYCODE_F6, 			DIK_F6,				VK_F6, 			0 },
	{ KEYCODE_F7, 			DIK_F7,				VK_F7, 			0 },
	{ KEYCODE_F8, 			DIK_F8,				VK_F8, 			0 },
	{ KEYCODE_F9, 			DIK_F9,				VK_F9, 			0 },
	{ KEYCODE_F10, 			DIK_F10,			VK_F10, 		0 },
	{ KEYCODE_NUMLOCK, 		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ KEYCODE_SCRLOCK, 		DIK_SCROLL,			VK_SCROLL, 		0 },
	{ KEYCODE_7_PAD, 		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ KEYCODE_8_PAD, 		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ KEYCODE_9_PAD, 		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ KEYCODE_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT, 	0 },
	{ KEYCODE_4_PAD, 		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ KEYCODE_5_PAD, 		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ KEYCODE_6_PAD, 		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ KEYCODE_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ KEYCODE_1_PAD, 		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ KEYCODE_2_PAD, 		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ KEYCODE_3_PAD, 		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ KEYCODE_0_PAD, 		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ KEYCODE_DEL_PAD, 		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ KEYCODE_F11, 			DIK_F11,			VK_F11, 		0 },
	{ KEYCODE_F12, 			DIK_F12,			VK_F12, 		0 },
	{ KEYCODE_OTHER, 		DIK_F13,			VK_F13, 		0 },
	{ KEYCODE_OTHER, 		DIK_F14,			VK_F14, 		0 },
	{ KEYCODE_OTHER, 		DIK_F15,			VK_F15, 		0 },
	{ KEYCODE_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN, 		0 },
	{ KEYCODE_RCONTROL, 	DIK_RCONTROL,		VK_CONTROL, 	0 },
	{ KEYCODE_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE, 		0 },
	{ KEYCODE_PRTSCR, 		DIK_SYSRQ, 			0, 				0 },
	{ KEYCODE_RALT, 		DIK_RMENU,			VK_MENU, 		0 },
	{ KEYCODE_HOME, 		DIK_HOME,			VK_HOME, 		0 },
	{ KEYCODE_UP, 			DIK_UP,				VK_UP, 			0 },
	{ KEYCODE_PGUP, 		DIK_PRIOR,			VK_PRIOR, 		0 },
	{ KEYCODE_LEFT, 		DIK_LEFT,			VK_LEFT, 		0 },
	{ KEYCODE_RIGHT, 		DIK_RIGHT,			VK_RIGHT, 		0 },
	{ KEYCODE_END, 			DIK_END,			VK_END, 		0 },
	{ KEYCODE_DOWN, 		DIK_DOWN,			VK_DOWN, 		0 },
	{ KEYCODE_PGDN, 		DIK_NEXT,			VK_NEXT, 		0 },
	{ KEYCODE_INSERT, 		DIK_INSERT,			VK_INSERT, 		0 },
	{ KEYCODE_DEL, 			DIK_DELETE,			VK_DELETE, 		0 },
	{ KEYCODE_LWIN, 		DIK_LWIN,			VK_LWIN, 		0 },
	{ KEYCODE_RWIN, 		DIK_RWIN,			VK_RWIN, 		0 },
	{ KEYCODE_MENU, 		DIK_APPS,			VK_APPS, 		0 },
	{ -1 }
};



//============================================================
//	JOYSTICK LIST
//============================================================

// this will be filled in dynamically
static struct JoystickInfo joylist[MAX_JOY];

// macros for building/mapping keycodes
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12))
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define JOYTYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

// joystick types
#define JOYTYPE_AXIS_NEG			0
#define JOYTYPE_AXIS_POS			1
#define JOYTYPE_POV_UP				2
#define JOYTYPE_POV_DOWN			3
#define JOYTYPE_POV_LEFT			4
#define JOYTYPE_POV_RIGHT			5
#define JOYTYPE_BUTTON				6
#define JOYTYPE_MOUSEBUTTON			7

// master translation table
static int joy_trans_table[][2] =
{
	// internal code					MAME code
	{ JOYCODE(0, JOYTYPE_AXIS_NEG, 0),	JOYCODE_1_LEFT },
	{ JOYCODE(0, JOYTYPE_AXIS_POS, 0),	JOYCODE_1_RIGHT },
	{ JOYCODE(0, JOYTYPE_AXIS_NEG, 1),	JOYCODE_1_UP },
	{ JOYCODE(0, JOYTYPE_AXIS_POS, 1),	JOYCODE_1_DOWN },
	{ JOYCODE(0, JOYTYPE_BUTTON, 0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 1),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 2),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 3),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 4),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 5),	JOYCODE_1_BUTTON6 },

	{ JOYCODE(1, JOYTYPE_AXIS_NEG, 0),	JOYCODE_2_LEFT },
	{ JOYCODE(1, JOYTYPE_AXIS_POS, 0),	JOYCODE_2_RIGHT },
	{ JOYCODE(1, JOYTYPE_AXIS_NEG, 1),	JOYCODE_2_UP },
	{ JOYCODE(1, JOYTYPE_AXIS_POS, 1),	JOYCODE_2_DOWN },
	{ JOYCODE(1, JOYTYPE_BUTTON, 0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 1),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 2),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 3),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 4),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 5),	JOYCODE_2_BUTTON6 },

	{ JOYCODE(2, JOYTYPE_AXIS_NEG, 0),	JOYCODE_3_LEFT },
	{ JOYCODE(2, JOYTYPE_AXIS_POS, 0),	JOYCODE_3_RIGHT },
	{ JOYCODE(2, JOYTYPE_AXIS_NEG, 1),	JOYCODE_3_UP },
	{ JOYCODE(2, JOYTYPE_AXIS_POS, 1),	JOYCODE_3_DOWN },
	{ JOYCODE(2, JOYTYPE_BUTTON, 0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 1),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 2),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 3),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 4),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 5),	JOYCODE_3_BUTTON6 },

	{ JOYCODE(3, JOYTYPE_AXIS_NEG, 0),	JOYCODE_4_LEFT },
	{ JOYCODE(3, JOYTYPE_AXIS_POS, 0),	JOYCODE_4_RIGHT },
	{ JOYCODE(3, JOYTYPE_AXIS_NEG, 1),	JOYCODE_4_UP },
	{ JOYCODE(3, JOYTYPE_AXIS_POS, 1),	JOYCODE_4_DOWN },
	{ JOYCODE(3, JOYTYPE_BUTTON, 0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 1),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 2),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 3),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 4),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 },

	{ JOYCODE(4, JOYTYPE_AXIS_NEG, 0),	JOYCODE_5_LEFT },
	{ JOYCODE(4, JOYTYPE_AXIS_POS, 0),	JOYCODE_5_RIGHT },
	{ JOYCODE(4, JOYTYPE_AXIS_NEG, 1),	JOYCODE_5_UP },
	{ JOYCODE(4, JOYTYPE_AXIS_POS, 1),	JOYCODE_5_DOWN },
	{ JOYCODE(4, JOYTYPE_BUTTON, 0),	JOYCODE_5_BUTTON1 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 1),	JOYCODE_5_BUTTON2 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 2),	JOYCODE_5_BUTTON3 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 3),	JOYCODE_5_BUTTON4 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 4),	JOYCODE_5_BUTTON5 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 5),	JOYCODE_5_BUTTON6 },

	{ JOYCODE(5, JOYTYPE_AXIS_NEG, 0),	JOYCODE_6_LEFT },
	{ JOYCODE(5, JOYTYPE_AXIS_POS, 0),	JOYCODE_6_RIGHT },
	{ JOYCODE(5, JOYTYPE_AXIS_NEG, 1),	JOYCODE_6_UP },
	{ JOYCODE(5, JOYTYPE_AXIS_POS, 1),	JOYCODE_6_DOWN },
	{ JOYCODE(5, JOYTYPE_BUTTON, 0),	JOYCODE_6_BUTTON1 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 1),	JOYCODE_6_BUTTON2 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 2),	JOYCODE_6_BUTTON3 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 3),	JOYCODE_6_BUTTON4 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 4),	JOYCODE_6_BUTTON5 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 5),	JOYCODE_6_BUTTON6 },

	{ JOYCODE(6, JOYTYPE_AXIS_NEG, 0),	JOYCODE_7_LEFT },
	{ JOYCODE(6, JOYTYPE_AXIS_POS, 0),	JOYCODE_7_RIGHT },
	{ JOYCODE(6, JOYTYPE_AXIS_NEG, 1),	JOYCODE_7_UP },
	{ JOYCODE(6, JOYTYPE_AXIS_POS, 1),	JOYCODE_7_DOWN },
	{ JOYCODE(6, JOYTYPE_BUTTON, 0),	JOYCODE_7_BUTTON1 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 1),	JOYCODE_7_BUTTON2 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 2),	JOYCODE_7_BUTTON3 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 3),	JOYCODE_7_BUTTON4 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 4),	JOYCODE_7_BUTTON5 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 5),	JOYCODE_7_BUTTON6 },

	{ JOYCODE(7, JOYTYPE_AXIS_NEG, 0),	JOYCODE_8_LEFT },
	{ JOYCODE(7, JOYTYPE_AXIS_POS, 0),	JOYCODE_8_RIGHT },
	{ JOYCODE(7, JOYTYPE_AXIS_NEG, 1),	JOYCODE_8_UP },
	{ JOYCODE(7, JOYTYPE_AXIS_POS, 1),	JOYCODE_8_DOWN },
	{ JOYCODE(7, JOYTYPE_BUTTON, 0),	JOYCODE_8_BUTTON1 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 1),	JOYCODE_8_BUTTON2 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 2),	JOYCODE_8_BUTTON3 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 3),	JOYCODE_8_BUTTON4 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 4),	JOYCODE_8_BUTTON5 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 5),	JOYCODE_8_BUTTON6 },

	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_SYS_BUTTON1 },
	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_SYS_BUTTON2 },
	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_SYS_BUTTON3 },
	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_SYS_BUTTON4 },

	{ JOYCODE(0, JOYTYPE_BUTTON, 6),	JOYCODE_1_BUTTON7 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 7),	JOYCODE_1_BUTTON8 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 8),	JOYCODE_1_BUTTON9 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 9),	JOYCODE_1_BUTTON10 },

	{ JOYCODE(1, JOYTYPE_BUTTON, 6),	JOYCODE_2_BUTTON7 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 7),	JOYCODE_2_BUTTON8 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 8),	JOYCODE_2_BUTTON9 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 9),	JOYCODE_2_BUTTON10 },

	{ JOYCODE(2, JOYTYPE_BUTTON, 6),	JOYCODE_3_BUTTON7 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 7),	JOYCODE_3_BUTTON8 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 8),	JOYCODE_3_BUTTON9 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 9),	JOYCODE_3_BUTTON10 },

	{ JOYCODE(3, JOYTYPE_BUTTON, 6),	JOYCODE_4_BUTTON7 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 7),	JOYCODE_4_BUTTON8 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 8),	JOYCODE_4_BUTTON9 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 9),	JOYCODE_4_BUTTON10 },

	{ JOYCODE(4, JOYTYPE_BUTTON, 6),	JOYCODE_5_BUTTON7 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 7),	JOYCODE_5_BUTTON8 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 8),	JOYCODE_5_BUTTON9 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 9),	JOYCODE_5_BUTTON10 },

	{ JOYCODE(5, JOYTYPE_BUTTON, 6),	JOYCODE_6_BUTTON7 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 7),	JOYCODE_6_BUTTON8 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 8),	JOYCODE_6_BUTTON9 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 9),	JOYCODE_6_BUTTON10 },

	{ JOYCODE(6, JOYTYPE_BUTTON, 6),	JOYCODE_7_BUTTON7 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 7),	JOYCODE_7_BUTTON8 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 8),	JOYCODE_7_BUTTON9 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 9),	JOYCODE_7_BUTTON10 },

	{ JOYCODE(7, JOYTYPE_BUTTON, 6),	JOYCODE_8_BUTTON7 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 7),	JOYCODE_8_BUTTON8 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 8),	JOYCODE_8_BUTTON9 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 9),	JOYCODE_8_BUTTON10 },

	{ JOYCODE(1, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_1_BUTTON1 },
	{ JOYCODE(1, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_1_BUTTON2 },
	{ JOYCODE(1, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_1_BUTTON3 },
	{ JOYCODE(1, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_1_BUTTON4 },

	{ JOYCODE(2, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_2_BUTTON1 },
	{ JOYCODE(2, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_2_BUTTON2 },
	{ JOYCODE(2, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_2_BUTTON3 },
	{ JOYCODE(2, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_2_BUTTON4 },

	{ JOYCODE(3, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_3_BUTTON1 },
	{ JOYCODE(3, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_3_BUTTON2 },
	{ JOYCODE(3, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_3_BUTTON3 },
	{ JOYCODE(3, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_3_BUTTON4 },

	{ JOYCODE(4, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_4_BUTTON1 },
	{ JOYCODE(4, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_4_BUTTON2 },
	{ JOYCODE(4, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_4_BUTTON3 },
	{ JOYCODE(4, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_4_BUTTON4 },

	{ JOYCODE(5, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_5_BUTTON1 },
	{ JOYCODE(5, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_5_BUTTON2 },
	{ JOYCODE(5, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_5_BUTTON3 },
	{ JOYCODE(5, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_5_BUTTON4 },

	{ JOYCODE(6, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_6_BUTTON1 },
	{ JOYCODE(6, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_6_BUTTON2 },
	{ JOYCODE(6, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_6_BUTTON3 },
	{ JOYCODE(6, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_6_BUTTON4 },

	{ JOYCODE(7, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_7_BUTTON1 },
	{ JOYCODE(7, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_7_BUTTON2 },
	{ JOYCODE(7, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_7_BUTTON3 },
	{ JOYCODE(7, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_7_BUTTON4 },

	{ JOYCODE(8, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_8_BUTTON1 },
	{ JOYCODE(8, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_8_BUTTON2 },
	{ JOYCODE(8, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_8_BUTTON3 },
	{ JOYCODE(8, JOYTYPE_MOUSEBUTTON, 3), 	JOYCODE_MOUSE_8_BUTTON4 },
};



//============================================================
//	enum_keyboard_callback
//============================================================

static BOOL CALLBACK enum_keyboard_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	HRESULT result;

	// if we're not out of mice, log this one
	if (keyboard_count >= MAX_KEYBOARDS)
		goto out_of_keyboards;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &keyboard_device[keyboard_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(keyboard_device[keyboard_count], &IID_IDirectInputDevice2, (void **)&keyboard_device2[keyboard_count]);
	if (result != DI_OK)
		keyboard_device2[keyboard_count] = NULL;

	// get the caps
	keyboard_caps[keyboard_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(keyboard_device[keyboard_count], &keyboard_caps[keyboard_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(keyboard_device[keyboard_count], &c_dfDIKeyboard);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	result = IDirectInputDevice_SetCooperativeLevel(keyboard_device[keyboard_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	keyboard_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_get_caps:
	if (keyboard_device2[keyboard_count])
		IDirectInputDevice_Release(keyboard_device2[keyboard_count]);
	IDirectInputDevice_Release(keyboard_device[keyboard_count]);
cant_create_device:
out_of_keyboards:
	return DIENUM_CONTINUE;
}



//============================================================
//	enum_mouse_callback
//============================================================

static BOOL CALLBACK enum_mouse_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result;

	// if we're not out of mice, log this one
	if (mouse_count >= MAX_MICE)
		goto out_of_mice;

	// init for this mouse
	num_axis_per_mouse[mouse_count] = 0;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &mouse_device[mouse_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(mouse_device[mouse_count], &IID_IDirectInputDevice2, (void **)&mouse_device2[mouse_count]);
	if (result != DI_OK)
		mouse_device2[mouse_count] = NULL;

	// get the caps
	mouse_caps[mouse_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(mouse_device[mouse_count], &mouse_caps[mouse_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// get number of axes used by the mouse
	num_axis_per_mouse[mouse_count] = mouse_caps[mouse_count].dwAxes;

	// set relative or absolute mode
	// note: does not have effect in winMe!!!
	// hacked it into pause function, where it does take effect
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	if (use_lightgun || use_lightgun2b)
	{
		if (verbose)
			printf("mouse/lightgun absolute mode\n");
		value.dwData = DIPROPAXISMODE_ABS;
	}
	else
	{
		if (verbose)
			printf("mouse/lightgun relative mode\n");
		value.dwData = DIPROPAXISMODE_REL;
	}
	result = IDirectInputDevice_SetProperty(mouse_device[mouse_count], DIPROP_AXISMODE, &value.diph);
	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(mouse_device[mouse_count], &c_dfDIMouse);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
/*start MAME:analog+*/
	if ( use_lightgun && !(use_lightgun2a) && !(use_lightgun2b) )
/*end MAME:analog+  */
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	else
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);

	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	if (use_lightgun||use_lightgun2a||use_lightgun2b)
		lightgun_count++;
	mouse_count++;
/*start MAME:analog+*/
	if (singlemouse)		// Assuming the first mouse enum'ed always is the sysmouse:
		return DIENUM_STOP;	// why continue looking for more mice if you only want one?
	else
/*end MAME:analog+  */
		return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (mouse_device2[mouse_count])
		IDirectInputDevice_Release(mouse_device2[mouse_count]);
	IDirectInputDevice_Release(mouse_device[mouse_count]);
cant_create_device:
out_of_mice:
	return DIENUM_CONTINUE;
}



//============================================================
//	enum_joystick_callback
//============================================================

static BOOL CALLBACK enum_joystick_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result = DI_OK;
	DWORD flags;

	// if we're not out of joysticks, log this one
	if (joystick_count >= MAX_JOYSTICKS)
		goto out_of_joysticks;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &joystick_device[joystick_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(joystick_device[joystick_count], &IID_IDirectInputDevice2, (void **)&joystick_device2[joystick_count]);
	if (result != DI_OK)
		joystick_device2[joystick_count] = NULL;

	// get the caps
	joystick_caps[joystick_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(joystick_device[joystick_count], &joystick_caps[joystick_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set absolute mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_ABS;
	result = IDirectInputDevice_SetProperty(joystick_device[joystick_count], DIPROP_AXISMODE, &value.diph);
 	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(joystick_device[joystick_count], &c_dfDIJoystick);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
#if HAS_WINDOW_MENU
	flags = DISCL_BACKGROUND | DISCL_EXCLUSIVE;
#else
	flags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
#endif
	result = IDirectInputDevice_SetCooperativeLevel(joystick_device[joystick_count], win_video_window,
					flags);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	joystick_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (joystick_device2[joystick_count])
		IDirectInputDevice_Release(joystick_device2[joystick_count]);
	IDirectInputDevice_Release(joystick_device[joystick_count]);
cant_create_device:
out_of_joysticks:
	return DIENUM_CONTINUE;
}

#ifdef WINXPANANLOG
/* <jake> */
//============================================================
//	process_raw_mouse_wm_input()
//============================================================
void process_rawmouse_wm_input(LPARAM lparam, int center_cursor_pos_x, int center_cursor_pos_y)
{
	// called when WM_INPUT message received in window.c
	//   to in order to stay informed of the current mouse status
#endif /* WINXPANANLOG */

#ifdef WINXPANANLOG
	if (use_rawmouse_winxp) add_to_raw_mouse_x_and_y((HRAWINPUT)lparam);

	// Hide the cursor smack in the center of the screen (hides the mouse in windowed mode)
	if (acquire_raw_mouse) SetCursorPos(center_cursor_pos_x, center_cursor_pos_y);

}
/* </jake> */
#endif /* WINXPANANLOG */

//============================================================
//	win_init_input
//============================================================

int win_init_input(void)
{
	HRESULT result;

	// first attempt to initialize DirectInput
	dinput_version = DIRECTINPUT_VERSION;
	result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
	if (result != DI_OK)
	{
		dinput_version = 0x0300;
		result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
		if (result != DI_OK)
			goto cant_create_dinput;
	}

	if (verbose)
		fprintf(stderr, "Using DirectInput %d\n", dinput_version >> 8);

	// initialize keyboard devices
	keyboard_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_KEYBOARD, enum_keyboard_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_keyboard;

	// initialize mouse devices
	lightgun_count = 0;
	mouse_count = 0;
	lightgun_dual_player_state[0]=lightgun_dual_player_state[1]=0;
	lightgun_dual_player_state[2]=lightgun_dual_player_state[3]=0;
#ifdef WINXPANANLOG

	/* <jake> */
	if (use_rawmouse_winxp) {
		if (!init_raw_mouse(1, 0, !singlemouse))  // only include individual mice when singlemouse is "off"
			goto cant_init_mouse;  // sysmouse yes, rdpmouse no, individualmice no

		mouse_count = raw_mouse_count;
		if (use_lightgun) lightgun_count = mouse_count;
	}
	else { // Use directinput for the mice
	/* </jake> */

#endif /* WINXPANANLOG */
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_MOUSE, enum_mouse_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_mouse;
#ifdef WINXPANANLOG
	/* <jake> */
	}
	/* </jake> */

#endif /* WINXPANANLOG */
/*start MAME:analog+*/
	if (splitmouse)
		init_mouse_arrays(mouse_map_split);		// handle split mice
	else if (mouse_count >= 9)
		init_mouse_arrays(mouse_map_8usbs);		// handle 8 usb mice
	else if (mouse_count >= 2)
		init_mouse_arrays(mouse_map_usbs);		// handle 1-3 usb mice
	else
		init_mouse_arrays(mouse_map_default);	// else do the normal mapping
/*end MAME:analog+  */

	// initialize joystick devices
	joystick_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_JOYSTICK, enum_joystick_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_joystick;

	// init the keyboard list
	init_keylist();

	// init the joystick list
	init_joylist();

	// print the results
	if (verbose)
/*start MAME:analog+*/
	{
		if (singlemouse)
			fprintf(stderr, "Single Mouse option set\n");
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d Lightguns=%d\n", keyboard_count, mouse_count, joystick_count, lightgun_count);
	}
/*end MAME:analog+  */
	return 0;

cant_init_joystick:
cant_init_mouse:
cant_init_keyboard:
	IDirectInput_Release(dinput);
cant_create_dinput:
	dinput = NULL;
	return 1;
}



//============================================================
//	win_shutdown_input
//============================================================

void win_shutdown_input(void)
{
	int i;

/* lightgun2a/b debug */
	DIPROPDWORD value;
	HRESULT result = DI_OK;

	if (verbose)
	{
		if (use_lightgun2a || use_lightgun2b)
		{
			if (use_lightgun2a)
			{
				fprintf (stdout, "final values for -lightgun2a (relative directX)\n");
				for (i = 0; i < MAX_PLAYERS; i++)
					fprintf (stdout, "Player %d max delta x,y ( %d , %d )\n", i, maxdeltax[i], maxdeltay[i]);
			}
			else if (use_lightgun2b)
			{
				fprintf (stdout, "final values for -lightgun2b (absolute directX)\n");
				for (i = 0; i < MAX_PLAYERS; i++)
					fprintf (stdout, "Player %d center x,y ( %d , %d )\n", i, lg_center_x[i], lg_center_y[i]);
			}
			for (i = 0; i<mouse_count; i++)
			{
				// get relative or absolute mode
				value.diph.dwSize = sizeof(DIPROPDWORD);
				value.diph.dwHeaderSize = sizeof(value.diph);
				value.diph.dwObj = 0;
				value.diph.dwHow = DIPH_DEVICE;
				result = IDirectInputDevice_GetProperty(mouse_device[i], DIPROP_AXISMODE, &value.diph);
				if (result == DI_OK)
				{
					fprintf(stdout, "Player %d mouse type %s\n", i, (value.dwData == DIPROPAXISMODE_REL) ? "Relative" : "Absolute");
	//				if (value.dwData == DIPROPAXISMODE_REL)
	//					fprintf(stdout, "Player %d mouse type Relative\n", i);
	//				else if (value.dwData == DIPROPAXISMODE_ABS)
	//					fprintf(stdout, "Player %d mouse type Absolute\n", i);
	//				else
	//					fprintf(stdout, "player %i mouse unknown rel/abs type", i);
				}
			}
		}
	}
/* end lightgun2a/b debug output */

	// release all our keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		IDirectInputDevice_Release(keyboard_device[i]);
		if (keyboard_device2[i])
			IDirectInputDevice_Release(keyboard_device2[i]);
		keyboard_device2[i]=0;
	}

	for (i = 0; i < joystick_count; i++)
	{
	}

	// release all our joysticks
	for (i = 0; i < joystick_count; i++)
	{
		// restore all joysticks original range before releasing
		for (int j=0; j < MAX_AXES; j++)
		{
			joystick_range[i][j].diph.dwSize = sizeof(DIPROPRANGE);
			joystick_range[i][j].diph.dwHeaderSize = sizeof(joystick_range[i][j].diph);
			joystick_range[i][j].diph.dwObj = offsetof(DIJOYSTATE, lX) + j * sizeof(LONG);
			joystick_range[i][j].diph.dwHow = DIPH_BYOFFSET;
			result = IDirectInputDevice_SetProperty(joystick_device[i], DIPROP_RANGE, &joystick_range[i][j].diph);
		}
		IDirectInputDevice_Release(joystick_device[i]);
		if (joystick_device2[i])
			IDirectInputDevice_Release(joystick_device2[i]);
		joystick_device2[i]=0;
	}

	// release all our mice

#ifdef WINXPANANLOG
  	/* <jake> */
	if (use_rawmouse_winxp)
	{
		destroy_raw_mouse();
	}
	else
	{
	/* </jake> */
#endif /* WINXPANANLOG */

	for (i = 0; i < mouse_count; i++)
	{
		IDirectInputDevice_Release(mouse_device[i]);
		if (mouse_device2[i])
			IDirectInputDevice_Release(mouse_device2[i]);
		mouse_device2[i]=0;
	}

#ifdef WINXPANANLOG
	/* <jake> */
	}
	/* </jake> */
#endif /* WINXPANANLOG */
	// release DirectInput
	if (dinput)
		IDirectInput_Release(dinput);
	dinput = NULL;
}



//============================================================
//	win_pause_input
//============================================================

void win_pause_input(int paused)
{
	int i;

	// if paused, unacquire all devices
	if (paused)
	{
		// unacquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Unacquire(keyboard_device[i]);

#ifdef WINXPANANLOG
		/* <jake> */
		if (!use_rawmouse_winxp) {
		/* </jake> */

#endif /* WINXPANANLOG */
		// unacquire all our mice
		for (i = 0; i < mouse_count; i++)
			IDirectInputDevice_Unacquire(mouse_device[i]);
#ifdef WINXPANANLOG

		/* <jake> */
		}
		else {
		  acquire_raw_mouse = 0;
		}
		/* </jake> */

#endif /* WINXPANANLOG */
	}

	// otherwise, reacquire all devices
	else
	{
		// acquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Acquire(keyboard_device[i]);

#ifdef WINXPANANLOG
		/* <jake> */
		if (!use_rawmouse_winxp) {
 	        /* </jake> */

#endif /* WINXPANANLOG */
		// acquire all our mice if active
		if (mouse_active && !win_has_menu())
		{
/*start MAME:analog+*/
			for (i = 0; i < mouse_count && (use_mouse||use_lightgun||use_lightgun2a||use_lightgun2b); i++)
			{
				if (use_lightgun2b)		// set abs mode not working in enum location
				{						// placement here is a hack, but seems to work
					DIPROPDWORD value;

					IDirectInputDevice_Unacquire(mouse_device[i]);

					// set absolute mode
					value.diph.dwSize = sizeof(DIPROPDWORD);
					value.diph.dwHeaderSize = sizeof(value.diph);
					value.diph.dwObj = 0;
					value.diph.dwHow = DIPH_DEVICE;
					value.dwData = DIPROPAXISMODE_ABS;

					IDirectInputDevice_SetProperty(mouse_device[i], DIPROP_AXISMODE, &value.diph);
				}
/*end MAME:analog+  */
				IDirectInputDevice_Acquire(mouse_device[i]);
			}

/*start MAME:analog+*/
// according to MS dX SDK help, need to init directX lightguns after every pause
			for (i = 0; i<MAX_PLAYERS && (use_lightgun2a||use_lightgun2b);i++)
				if (!initlightgun2[i]) initlightgun2[i] = 1;
/*end MAME:analog+  */
		}
#ifdef WINXPANANLOG

		/* <jake> */
		}
		else {
		        if (mouse_active && !win_has_menu()) acquire_raw_mouse = 1;
		}
		/* </jake> */

#endif /* WINXPANANLOG */
	}

	// set the paused state
	input_paused = paused;
	win_update_cursor_state();
}



//============================================================
//	win_poll_input
//============================================================

void win_poll_input(void)
{
	HWND focus = GetFocus();
	HRESULT result = 1;
	int i, j;

	// remember when this happened
	last_poll = osd_cycles();

	// periodically process events, in case they're not coming through
	win_process_events_periodic();

	// if we don't have focus, turn off all keys
	if (!focus)
	{
		memset(&keyboard_state[0][0], 0, sizeof(keyboard_state[i]));
		updatekeyboard();
		return;
	}

	// poll all keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		// first poll the device
		if (keyboard_device2[i])
			IDirectInputDevice2_Poll(keyboard_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);

		// handle lost inputs here
		if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
		{
			result = IDirectInputDevice_Acquire(keyboard_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);
		}

		// convert to 0 or 1
		if (result == DI_OK)
			for (j = 0; j < sizeof(keyboard_state[i]); j++)
				keyboard_state[i][j] >>= 7;
	}

	// if we couldn't poll the keyboard that way, poll it via GetAsyncKeyState
	if (result != DI_OK)
		for (i = 0; keylist[i].code; i++)
		{
			int dik = DICODE(keylist[i].code);
			int vk = VKCODE(keylist[i].code);

			// if we have a non-zero VK, query it
			if (vk)
				keyboard_state[0][dik] = (GetAsyncKeyState(vk) >> 15) & 1;
		}

	// update the lagged keyboard
	updatekeyboard();

	// if the debugger is up and visible, don't bother with the rest
	if (win_debug_window != NULL && IsWindowVisible(win_debug_window))
		return;

	// poll all joysticks
	for (i = 0; i < joystick_count && use_joystick; i++)
	{
		// first poll the device
		if (joystick_device2[i])
			IDirectInputDevice2_Poll(joystick_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);

		// handle lost inputs here
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
		{
			result = IDirectInputDevice_Acquire(joystick_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);
		}
	}

#ifdef WINXPANANLOG
	/* <jake> */  // For rawmouse, this processing is handled with WM_INPUT from window.c
	if (!use_rawmouse_winxp) {
	/* </jake> */

#endif /* WINXPANANLOG */
	// poll all our mice if active
	if (mouse_active && !win_has_menu() && (use_mouse||use_lightgun||use_lightgun2a||use_lightgun2b))
		for (i = 0; i < mouse_count; i++)
		{
			// first poll the device
			if (mouse_device2[i])
				IDirectInputDevice2_Poll(mouse_device2[i]);

			// get the state
			result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);

			// handle lost inputs here
			if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
			{
				result = IDirectInputDevice_Acquire(mouse_device[i]);
				if (result == DI_OK)
					result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);
			}
		}
#ifdef WINXPANANLOG
	/* <jake> */
	}
	/* </jake> */
#endif /* WINXPANANLOG */
}



//============================================================
//	is_mouse_captured
//============================================================

int win_is_mouse_captured(void)
{
	return (!input_paused && mouse_active && (mouse_count > 0) && (use_mouse||use_lightgun||use_lightgun2a||use_lightgun2b) && !win_has_menu());
}



//============================================================
//	osd_get_key_list
//============================================================

const struct KeyboardInfo *osd_get_key_list(void)
{
	return keylist;
}



//============================================================
//	updatekeyboard
//============================================================

// since the keyboard controller is slow, it is not capable of reporting multiple
// key presses fast enough. We have to delay them in order not to lose special moves
// tied to simultaneous button presses.

static void updatekeyboard(void)
{
	int i, changed = 0;

	// see if any keys have changed state
	for (i = 0; i < MAX_KEYS; i++)
		if (keyboard_state[0][i] != oldkey[i])
		{
			changed = 1;

			// keypress was missed, turn it on for one frame
			if (keyboard_state[0][i] == 0 && currkey[i] == 0)
				currkey[i] = -1;
		}

	// if keyboard state is stable, copy it over
	if (!changed)
		memcpy(currkey, &keyboard_state[0][0], sizeof(currkey));

	// remember the previous state
	memcpy(oldkey, &keyboard_state[0][0], sizeof(oldkey));
}



//============================================================
//	osd_is_key_pressed
//============================================================

int osd_is_key_pressed(int keycode)
{
	int dik = DICODE(keycode);

	// make sure we've polled recently
	if (osd_cycles() > last_poll + osd_cycles_per_second()/4)
		win_poll_input();

	// special case: if we're trying to quit, fake up/down/up/down
	if (dik == DIK_ESCAPE && win_trying_to_quit)
	{
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	// if the video window isn't visible, we have to get our events from the console
	if (!win_video_window || !IsWindowVisible(win_video_window))
	{
		// warning: this code relies on the assumption that when you're polling for
		// keyboard events before the system is initialized, they are all of the
		// "press any key" to continue variety
		int result = _kbhit();
		if (result)
			_getch();
		return result;
	}

	// otherwise, just return the current keystate
	if (steadykey)
		return currkey[dik];
	else
		return keyboard_state[0][dik];
}



//============================================================
//	osd_readkey_unicode
//============================================================

int osd_readkey_unicode(int flush)
{
#if 0
	if (flush) clear_keybuf();
	if (keypressed())
		return ureadkey(NULL);
	else
		return 0;
#endif
	return 0;
}



//============================================================
//	init_keylist
//============================================================

static void init_keylist(void)
{
	int keycount = 0, key;
	struct ik *temp;

	// iterate over all possible keys
	for (key = 0; key < MAX_KEYS; key++)
	{
		DIDEVICEOBJECTINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
		result = IDirectInputDevice_GetObjectInfo(keyboard_device[0], &instance, key, DIPH_BYOFFSET);
		if (result == DI_OK)
		{
			// if it worked, assume we have a valid key

			// copy the name
			char *namecopy = malloc(strlen(instance.tszName) + 1);
			if (namecopy)
			{
				unsigned code, standardcode;
				int entry;

				// find the table entry, if there is one
				for (entry = 0; win_key_trans_table[entry][0] >= 0; entry++)
					if (win_key_trans_table[entry][DI_KEY] == key)
						break;

				// compute the code, which encodes DirectInput, virtual, and ASCII codes
				code = KEYCODE(key, 0, 0);
				standardcode = KEYCODE_OTHER;
				if (entry < win_key_trans_table[entry][0] >= 0)
				{
					code = KEYCODE(key, win_key_trans_table[entry][VIRTUAL_KEY], win_key_trans_table[entry][ASCII_KEY]);
					standardcode = win_key_trans_table[entry][MAME_KEY];
				}

				// fill in the key description
				keylist[keycount].name = strcpy(namecopy, instance.tszName);
				keylist[keycount].code = code;
				keylist[keycount].standardcode = standardcode;
				keycount++;

				// make sure we have enough room for the new entry and the terminator (2 more)
				if ((num_osd_ik + 2) > size_osd_ik)
				{
					// attempt to allocate 16 more
					temp = realloc (osd_input_keywords, (size_osd_ik + 16)*sizeof (struct ik));

					// if the realloc was successful
					if (temp)
					{
						// point to the new buffer and increase the size indicator
						osd_input_keywords =  temp;
						size_osd_ik += 16;
					}
				}

				// if we have enough room for the new entry and the terminator
				if ((num_osd_ik + 2) <= size_osd_ik)
				{
					const char *src;
					char *dst;

					osd_input_keywords[num_osd_ik].name = malloc (strlen(instance.tszName) + 4 + 1);

					src = instance.tszName;
					dst = (char *)osd_input_keywords[num_osd_ik].name;

					strcpy (dst, "Key_");
					dst += strlen(dst);

					// copy name converting all spaces to underscores
					while (*src != 0)
					{
						if (*src == ' ')
							*dst++ = '_';
						else
							*dst++ = *src;
						src++;
					}
					*dst = 0;

					osd_input_keywords[num_osd_ik].type = IKT_OSD_KEY;
					osd_input_keywords[num_osd_ik].val = code;

					num_osd_ik++;

					// indicate end of list
					osd_input_keywords[num_osd_ik].name = 0;
				}
			}
		}
	}

	// terminate the list
	memset(&keylist[keycount], 0, sizeof(keylist[keycount]));
}



//============================================================
//	add_joylist_entry
//============================================================

static void add_joylist_entry(const char *name, int code, int *joycount)
{
	int standardcode = JOYCODE_OTHER;
 	struct ik *temp;

	// copy the name
	char *namecopy = malloc(strlen(name) + 1);
	if (namecopy)
	{
		int entry;

		// find the table entry, if there is one
		for (entry = 0; entry < ELEMENTS(joy_trans_table); entry++)
			if (joy_trans_table[entry][0] == code)
				break;

		// fill in the joy description
		joylist[*joycount].name = strcpy(namecopy, name);
		joylist[*joycount].code = code;
		if (entry < ELEMENTS(joy_trans_table))
			standardcode = joy_trans_table[entry][1];
		joylist[*joycount].standardcode = standardcode;
		*joycount += 1;

		// make sure we have enough room for the new entry and the terminator (2 more)
		if ((num_osd_ik + 2) > size_osd_ik)
		{
			// attempt to allocate 16 more
			temp = realloc (osd_input_keywords, (size_osd_ik + 16)*sizeof (struct ik));

			// if the realloc was successful
			if (temp)
			{
				// point to the new buffer and increase the size indicator
				osd_input_keywords =  temp;
				size_osd_ik += 16;
			}
		}

		// if we have enough room for the new entry and the terminator
		if ((num_osd_ik + 2) <= size_osd_ik)
		{
			const char *src;
			char *dst;

			osd_input_keywords[num_osd_ik].name = malloc (strlen(name) + 1);

			src = name;
			dst = (char *)osd_input_keywords[num_osd_ik].name;

			// copy name converting all spaces to underscores
			while (*src != 0)
			{
				if (*src == ' ')
					*dst++ = '_';
				else
					*dst++ = *src;
				src++;
			}
			*dst = 0;

			osd_input_keywords[num_osd_ik].type = IKT_OSD_JOY;
			osd_input_keywords[num_osd_ik].val = code;

			num_osd_ik++;

			// indicate end of list
			osd_input_keywords[num_osd_ik].name = 0;
		}
	}
}



//============================================================
//	init_joylist
//============================================================

static void init_joylist(void)
{
	int mouse, stick, axis, button, pov;
	char tempname[MAX_PATH];
	int joycount = 0;

	// first of all, map mouse buttons
//	for (mouse = 0; (mouse < mouse_count) && (use_mouse || use_lightgun||use_lightgun2a||use_lightgun2b); mouse++)
	for (mouse = 0; mouse < mouse_count; mouse++)
		for (button = 0; button < 4; button++)
		{
#ifdef WINXPANANLOG

			/* <jake> */
			if(use_rawmouse_winxp) {
				if (mouse_count > 1) {
					wsprintf(tempname, "Mouse %d %s", mouse, get_raw_mouse_button_name(mouse, button));
				}
				else
					sprintf(tempname, "Mouse Button %i", button);

					if (mouse != 0) // Avoid using the sysmouse buttons for every user
						add_joylist_entry(tempname, JOYCODE(mouse, JOYTYPE_MOUSEBUTTON, button), &joycount);
			}
			else { // using directx
			/* </jake> */

#endif /* WINXPANANLOG */
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(mouse_device[mouse], &instance, offsetof(DIMOUSESTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add mouse number to the name
				if (mouse_count > 1)
					if (mouse == 0)
						sprintf(tempname, "SysMouse %s", instance.tszName);
					else
						sprintf(tempname, "Mouse %d %s", mouse, instance.tszName);
				else
					sprintf(tempname, "Mouse %s", instance.tszName);
				add_joylist_entry(tempname, JOYCODE(mouse, JOYTYPE_MOUSEBUTTON, button), &joycount);
			}
#ifdef WINXPANANLOG

			/* <jake> */
			}
			/* </jake> */

#endif /* WINXPANANLOG */
		}

	// now map joysticks
//	for (stick = 0; (stick < joystick_count) && use_joystick; stick++)	// loop over all joystick deviceses
	for (stick = 0; stick < joystick_count; stick++)	// loop over all joystick devices
	{
		// loop over all axes
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;
			DIPROPRANGE temprange;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add negative value
				sprintf(tempname, "J%d %s -", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_AXIS_NEG, axis), &joycount);

				// add positive value
				sprintf(tempname, "J%d %s +", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_AXIS_POS, axis), &joycount);

				// get the axis range while we're here
				joystick_range[stick][axis].diph.dwSize = sizeof(DIPROPRANGE);
				joystick_range[stick][axis].diph.dwHeaderSize = sizeof(joystick_range[stick][axis].diph);
				joystick_range[stick][axis].diph.dwObj = offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG);
				joystick_range[stick][axis].diph.dwHow = DIPH_BYOFFSET;
				result = IDirectInputDevice_GetProperty(joystick_device[stick], DIPROP_RANGE, &joystick_range[stick][axis].diph);

/*start MAME:analog+*/
				// try to set range to -128 to 128
				temprange.diph.dwSize       = sizeof(DIPROPRANGE);
				temprange.diph.dwHeaderSize = sizeof(temprange.diph);
				temprange.diph.dwObj        = offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG);
				temprange.diph.dwHow        = DIPH_BYOFFSET;
				temprange.lMin              = -128;
				temprange.lMax              = +128;
				result = IDirectInputDevice_SetProperty(joystick_device[stick], DIPROP_RANGE, &temprange.diph);
				joystick_setrange[stick][axis] = (result == DI_OK);
				if (verbose)
				{
					if (joystick_setrange[stick][axis])
						fprintf(stderr, "stick%d, axis%d: set\n", stick, axis) ;
					else
						fprintf(stderr, "stick%d, axis%d: could not set\n", stick, axis) ;
				}
/*end MAME:analog+  */
			}
		}

		// loop over all buttons
		for (button = 0; button < MAX_BUTTONS; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// make the name for this item
				sprintf(tempname, "J%d %s", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_BUTTON, button), &joycount);
			}
		}

		// check POV hats
		for (pov = 0; pov < MAX_POV; pov++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgdwPOV[pov]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add up direction
				sprintf(tempname, "J%d %s U", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_UP, pov), &joycount);

				// add down direction
				sprintf(tempname, "J%d %s D", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_DOWN, pov), &joycount);

				// add left direction
				sprintf(tempname, "J%d %s L", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_LEFT, pov), &joycount);

				// add right direction
				sprintf(tempname, "J%d %s R", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_RIGHT, pov), &joycount);
			}
		}

/*start MAME:analog+*/
		/* init analog_post_player to default */
//		init_analogjoy_arrays();
/*end MAME:analog+  */
	}

	// terminate array
	memset(&joylist[joycount], 0, sizeof(joylist[joycount]));
}



//============================================================
//	osd_get_joy_list
//============================================================

const struct JoystickInfo *osd_get_joy_list(void)
{
	return joylist;
}

#ifdef WINXPANANLOG
/* <jake> */
//============================================================
//	osd_new_input_read_cycle
//============================================================
#endif /* WINXPANANLOG */

#ifdef WINXPANANLOG
void osd_new_input_read_cycle(void)
{
	// Do any processing needed at the beginning of an input read cycle
	int i;

	// refresh the local copy of raw mouse x and y deltas
	// When the get_raw_mouse_n_delta() is called, the
	//    the mouse delta is reset to 0.  Therefore I need to
	//    cache the values at the beginning in case multiple
	//    players wish to share a single raw_mouse input.
	if (use_rawmouse_winxp) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			raw_mouse_delta_x[i] = get_raw_mouse_x_delta(i);
			raw_mouse_delta_y[i] = get_raw_mouse_y_delta(i);
			raw_mouse_delta_z[i] = get_raw_mouse_z_delta(i);
		}
	}
}
/* </jake> */
#endif /* WINXPANANLOG */

//============================================================
//	osd_is_joy_pressed
//============================================================

int osd_is_joy_pressed(int joycode)
{
	int joyindex = JOYINDEX(joycode);
	int joytype = JOYTYPE(joycode);
	int joynum = JOYNUM(joycode);
	DWORD pov;

	// switch off the type
	switch (joytype)
	{
		case JOYTYPE_MOUSEBUTTON:
/*Analog+: back out official mame's core reload so the game driver reload works*/
#if 0
			/* ActLabs lightgun - remap button 2 (shot off-screen) as button 1 */
			if (use_lightgun_dual && joyindex<4) {
				if (use_lightgun_reload && joynum==0) {
					if (joyindex==0 && lightgun_dual_player_state[1])
						return 1;
					if (joyindex==1 && lightgun_dual_player_state[1])
						return 0;
					if (joyindex==2 && lightgun_dual_player_state[3])
						return 1;
					if (joyindex==2 && lightgun_dual_player_state[3])
						return 0;
				}
				return lightgun_dual_player_state[joyindex];
			}

			if (use_lightgun) {
				if (use_lightgun_reload && joynum==0) {
					if (joyindex==0 && (mouse_state[0].rgbButtons[1]&0x80))
						return 1;
					if (joyindex==1 && (mouse_state[0].rgbButtons[1]&0x80))
						return 0;
				}
			}
#endif
/*end Analog+: back out*/
#ifdef WINXPANANLOG
		  /* <jake> */
		  if (use_rawmouse_winxp) {
				return is_raw_mouse_button_pressed(joynum, joyindex);
		  }
		  else
		  /* </jake> */
#endif /* WINXPANANLOG */

			return mouse_state[joynum].rgbButtons[joyindex] >> 7;

		case JOYTYPE_BUTTON:
			return joystick_state[joynum].rgbButtons[joyindex] >> 7;

		case JOYTYPE_AXIS_POS:
		case JOYTYPE_AXIS_NEG:
		{
			LONG top, bottom, middle;
			LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
			if (joystick_setrange[joynum][joyindex])
			{
				top = 128;
				bottom = -128;
				middle = 0;
			}
			else
			{
				top = joystick_range[joynum][joyindex].lMax;
				bottom = joystick_range[joynum][joyindex].lMin;
				middle = (top + bottom) / 2;
			}

			// watch for movement greater "a2d_deadzone" along either axis
			// FIXME in the two-axis joystick case, we need to find out
			// the angle. Anything else is unprecise.
			if (joytype == JOYTYPE_AXIS_POS)
				return (val > middle + ((top - middle) * a2d_deadzone));
			else
				return (val < middle - ((middle - bottom) * a2d_deadzone));
		}

		// anywhere from 0-45 (315) deg to 0+45 (45) deg
		case JOYTYPE_POV_UP:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 31500 || pov <= 4500));

		// anywhere from 90-45 (45) deg to 90+45 (135) deg
		case JOYTYPE_POV_RIGHT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 4500 && pov <= 13500));

		// anywhere from 180-45 (135) deg to 180+45 (225) deg
		case JOYTYPE_POV_DOWN:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 13500 && pov <= 22500));

		// anywhere from 270-45 (225) deg to 270+45 (315) deg
		case JOYTYPE_POV_LEFT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 22500 && pov <= 31500));

	}

	// keep the compiler happy
	return 0;
}



//============================================================
//	osd_analogjoy_read
//============================================================

void osd_analogjoy_read(int player, int analog_axis[], InputCode analogjoy_input[])
{
	LONG top, bottom, middle;
	int i;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && use_mouse && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	for (i=0; i<MAX_ANALOG_AXES; i++)
	{
		int joyindex, joynum;

		analog_axis[i] = 0;

		if (analogjoy_input[i] == CODE_NONE || !use_joystick)
			continue;

		joyindex = JOYINDEX( analogjoy_input[i] );
		joynum = JOYNUM( analogjoy_input[i] );

		if (joystick_setrange[joynum][joyindex])
		{
			analog_axis[i] = ((LONG *)&joystick_state[joynum].lX)[joyindex];
			if (analog_axis[i] < -128) { analog_axis[i] = -128; fprintf(stderr, "RangeSet but too low\n"); }
			if (analog_axis[i] >  128) { analog_axis[i] =  128; fprintf(stderr, "RangeSet but too high\n"); }
		}
		else
		{
			top = joystick_range[joynum][joyindex].lMax;
			bottom = joystick_range[joynum][joyindex].lMin;
			middle = (top + bottom) / 2;
			analog_axis[i] = (((LONG *)&joystick_state[joynum].lX)[joyindex] - middle) * 257 / (top - bottom);
			if (analog_axis[i] < -128) analog_axis[i] = -128;
			if (analog_axis[i] >  128) analog_axis[i] =  128;
		}
		if (!osd_isNegativeSemiAxis(analogjoy_input[i]))
			analog_axis[i] = -analog_axis[i];
	}
}


int osd_is_joystick_axis_code(int joycode)
{
	switch (JOYTYPE( joycode ))
	{
		case JOYTYPE_AXIS_POS:
		case JOYTYPE_AXIS_NEG:
			return 1;
		default:
			return 0;
	}
	return 0;
}


//============================================================
//	osd_lightgun_read
//============================================================

void input_mouse_button_down(int button, int x, int y)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=1;
	lightgun_dual_player_pos[button].x=x;
	lightgun_dual_player_pos[button].y=y;

	//logerror("mouse %d at %d %d\n",button,x,y);
}

void input_mouse_button_up(int button)
{
	if (!use_lightgun_dual)
		return;

	lightgun_dual_player_state[button]=0;
}

void osd_lightgun_read(int player,int *deltax,int *deltay)
{
	POINT point;

#ifdef WINXPANANLOG
	/* <jake> */
	if (use_rawmouse_winxp) {
		osd_raw_mouse_lightgun_read(player,deltax,deltay);
		return;
	}
	/* </jake> */

#endif /* WINXPANANLOG */
	if (use_lightgun2b)
	{
		osd_lightgun_read2b(player,deltax,deltay);
		return;
	}

	// if the mouse isn't yet active, make it so
	if (!mouse_active && (use_mouse||use_lightgun) && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// if out of range, skip it
	if (!use_lightgun || !win_physical_width || !win_physical_height || player >= (lightgun_count + use_lightgun_dual))
	{
		*deltax = *deltay = 0;
		return;
	}

	// Warning message to users - design wise this probably isn't the best function to put this in...
	if (win_window_mode)
		usrintf_showmessage("Lightgun not supported in windowed mode");

	// Hack - if button 2 is pressed on lightgun, then return 0,0 (off-screen) to simulate reload
	if (use_lightgun_reload)
	{
		int return_offscreen=0;

		// In dualmode we need to use the buttons returned from Windows messages
		if (use_lightgun_dual)
		{
			if (player==0 && lightgun_dual_player_state[1])
				return_offscreen=1;

			if (player==1 && lightgun_dual_player_state[3])
				return_offscreen=1;
		}
		else
		{
			if (mouse_state[0].rgbButtons[1]&0x80)
				return_offscreen=1;
		}

		if (return_offscreen)
		{
			*deltax = -128;
			*deltay = -128;
			return;
		}
	}

	// Act-Labs dual lightgun - _only_ works with Windows messages for input location
	if (use_lightgun_dual)
	{
		if (player==0)
		{
			point.x=lightgun_dual_player_pos[0].x; // Button 0 is player 1
			point.y=lightgun_dual_player_pos[0].y; // Button 0 is player 1
		}
		else if (player==1)
		{
			point.x=lightgun_dual_player_pos[2].x; // Button 2 is player 2
			point.y=lightgun_dual_player_pos[2].y; // Button 2 is player 2
		}
		else
		{
			point.x=point.y=0;
		}

		// Map absolute pixel values into -128 -> 128 range
		*deltax = (point.x * 256 + win_physical_width/2) / (win_physical_width-1) - 128;
		*deltay = (point.y * 256 + win_physical_height/2) / (win_physical_height-1) - 128;
	}
	else
	{
		// I would much prefer to use DirectInput to read the gun values but there seem to be
		// some problems...  DirectInput (8.0 tested) on Win98 returns garbage for both buffered
		// and immediate, absolute and relative axis modes.  Win2k (DX 8.1) returns good data
		// for buffered absolute reads, but WinXP (8.1) returns garbage on all modes.  DX9 betas
		// seem to exhibit the same behaviour.  I have no idea of the cause of this, the only
		// consistent way to read the location seems to be the Windows system call GetCursorPos
		// which requires the application have non-exclusive access to the mouse device
		//
		GetCursorPos(&point);

		// Map absolute pixel values into -128 -> 128 range
		*deltax = (point.x * 256 + win_physical_width/2) / (win_physical_width-1) - 128;
		*deltay = (point.y * 256 + win_physical_height/2) / (win_physical_height-1) - 128;
	}

	if (*deltax < -128) *deltax = -128;
	if (*deltax > 128) *deltax = 128;
	if (*deltay < -128) *deltay = -128;
	if (*deltay > 128) *deltay = 128;
}


//============================================================
//	osd_lightgun_read2a
//============================================================

	// This is an alpha test for using directX relative mouse values
	// mame core should treat deltax & deltay values like
	// a normal mouse delta values
	// Compare to osd_trackball_read() function

void osd_lightgun_read2a(int player,int *deltax,int *deltay)
{
//	static int maxdeltax[]={1,1,1,1}, maxdeltay[]={1,1,1,1};  //currently global for debug
//	static int sxpre=0, sypre=0;
	int sx=0, sy=0;							// temp raw mouse vaules
	int mousex,mousey;
	int axisx, axisy;

	*deltax = 0;
	*deltay = 0;

	if (!use_lightgun2a )
		return ;

	// if the mouse isn't yet active, make it so
	if (!mouse_active)
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// init values to get center
	if (initlightgun2[player])
	{								// resetting maxdelta
		maxdeltax[player] = 1;
		maxdeltay[player] = 1;

		if (verbose)						// debugging output
		{
			fprintf (stdout, "p%d init delta (%d,%d)\n", player, maxdeltax[player], maxdeltay[player]);
			if (player == 3) fprintf (stdout, "\n");
		}

		initlightgun2[player]--;
	}

	// for switchable mice and switch mouse axis
	mousex = os_player_mouse_axes[player].xmouse;
	mousey = os_player_mouse_axes[player].ymouse;
	axisx = os_player_mouse_axes[player].xaxis;
	axisy = os_player_mouse_axes[player].yaxis;

	// get the latest lightgun/mouse info if mouse exists
	if ((mousex >= 0) && (mousex < mouse_count))
	{
		switch (axisx)
		{
			case X_AXIS: sx = mouse_state[mousex].lX;	break;
			case Y_AXIS: sx = mouse_state[mousex].lY;	break;
			case Z_AXIS: sx = mouse_state[mousex].lZ;	break;

			default:	sx = 0;	break;	/* never should happen, but JIC */
		}
	}
	if ((mousey >= 0) && (mousey < mouse_count))
	{
		switch (axisy)
		{
			case X_AXIS: sy = mouse_state[mousex].lX;	break;
			case Y_AXIS: sy = mouse_state[mousex].lY;	break;
			case Z_AXIS: sy = mouse_state[mousex].lZ;	break;

			default:	sy = 0;	break;	/* never should happen, but JIC */
		}
	}

	// need max delta to scale correctly
	if (abs(sx)>maxdeltax[player])
	{
		maxdeltax[player] = abs(sx);
	}
	if (abs(sy)>maxdeltay[player])
	{
		maxdeltay[player] = abs(sy);
	}

	// Map reletive pixel values for mame
	// I think it should be -256 to 256, corner to corner. I need to check up on this
//	*deltax=sx*256/maxdeltax[player];
//	*deltay=sy*256/maxdeltay[player];
	*deltax=sx*512/0x7fff;				// max delta ~32000 (0x7fff), so testing constant number
	*deltay=sy*512/0x7fff;
}

//============================================================
//	osd_lightgun_read2b
//============================================================

	// This is an alpha test for using directX absolute mouse values
	// mame core should treat deltax & deltay values like
	// a lightgun's delta values
	// Compare to osd_lightgun_read() function

void osd_lightgun_read2b(int player,int *deltax,int *deltay)
{
//	static int initlightgun2[]= {1,1,1,1};  // global for debugging
//	static int lg_center_x[4], lg_center_y[4];	// global for debug
	static int minmx[MAX_PLAYERS], minmy[MAX_PLAYERS];		//remove this after debugging
	static int maxmx[MAX_PLAYERS], maxmy[MAX_PLAYERS];		//remove this after debugging
	static int presx[MAX_PLAYERS], presy[MAX_PLAYERS];		// saving last value to skip repeating math
	const int rangemx=0xFFFF, rangemy=0xFFFF;	// total range (x,y), works on my winME
	int p=0;							// debug boolean
	int sx=0, sy=0, changed=0;			// temp raw mouse vaules, and if different from last time boolean
	int mousex,mousey;					// mouse # that controls this game axis
	int axisx, axisy;					// mouse axis # that controls this game axis

	if (!use_lightgun2b)
		return ;			// don't really need this ATM, but leaving it in for error protection

	// if the mouse isn't yet active, make it so
	if (!mouse_active)
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// for switchable mice and switch mouse axis
	mousex = os_player_mouse_axes[player].xmouse;
	mousey = os_player_mouse_axes[player].ymouse;
	axisx = os_player_mouse_axes[player].xaxis;
	axisy = os_player_mouse_axes[player].yaxis;

	// get the latest lightgun/mouse info if mouse exists
	if ((mousex >= 0) && (mousex < mouse_count))
	{
		switch (axisx)
		{
			case X_AXIS: sx = mouse_state[mousex].lX;	break;
			case Y_AXIS: sx = mouse_state[mousex].lY;	break;
			case Z_AXIS: sx = mouse_state[mousex].lZ;	break;

			default:	sx = 0;	break;	/* never should happen, but JIC */
		}
	}
	if ((mousey >= 0) && (mousey < mouse_count))
	{
		switch (axisy)
		{
			case X_AXIS: sy = mouse_state[mousex].lX;	break;
			case Y_AXIS: sy = mouse_state[mousex].lY;	break;
			case Z_AXIS: sy = mouse_state[mousex].lZ;	break;

			default:	sy = 0;	break;	/* never should happen, but JIC */
		}
	}

	// init values to get center
	if (initlightgun2[player])
	{																// get default center values, I hope
		lg_center_x[player] = (mousex == 0) ? sx : rangemx/2;		// system mouse has unknown center
		lg_center_y[player] = (mousey == 0) ? sy : rangemy/2;		// USB mice in winMe center = 0x7FFF

		if (verbose)						// debugging output
		{
			fprintf (stdout, "p%d init center (%d,%d)\n", player, lg_center_x[player], lg_center_y[player]);
			if (player == 3) fprintf (stdout, "\n");
		}

		presx[player] = sx+1; presy[player] = sy+1;
		minmx[player]=maxmx[player]=sx;
		minmy[player]=maxmy[player]=sy;
		initlightgun2[player]--;
	}

// debug: store old value so only fprintfs when changed
//	doesn't seem to be working ?
	if ( (sx == presx[player]) && (sy == presy[player]) )
		return ;

	presx[player] = sx; presy[player] = sy;
	*deltax = 0;
	*deltay = 0;

	if (minmx[player] > sx) minmx[player] = sx;			//storing min & max for debugging
	else if (maxmx[player] < sx) maxmx[player] = sx;
	if (minmy[player] > sy) minmy[player] = sy;
	else if (maxmy[player] < sy) maxmy[player] = sy;

	changed = 1;		// old code.  leaving in because may need later

	// Map absolute pixel values into -128 -> 128 range
	// I hope that's what I'm doing
	sx = sx - ( lg_center_x[player] - (rangemx / 2) );
	sy = sy - ( lg_center_y[player] - (rangemy / 2) );

// this should be combined with last two lines on sy & sx, but separated for debugging for now
	*deltax=((sx*257)/rangemx) - 128;
	*deltay=((sy*257)/rangemy) - 128;

	if (*deltax > 128) *deltax=128;
	if (*deltax < -128) *deltay=-128;
	if (*deltay > 128) *deltay=128;
	if (*deltay < -128) *deltay=-128;

	if (verbose && (p || (changed && (player == 0))))	// debug output
	{
		fprintf (stdout, "p%d center (%d,%d)\t min, max x (%d - %d)  y (%d - %d)\t", player, \
						lg_center_x[player], lg_center_y[player], \
						minmx[player], maxmx[player],minmy[player], maxmy[player]);
		fprintf (stdout, "p%d range (%d,%d)\ns# (%d,%d)\t", player, rangemx, rangemy, sx, sy);
		fprintf (stdout, "p%d delta (%d,%d)\n\n", player, *deltax, *deltay);
	}
}
#ifdef WINXPANANLOG

/* <jake> */
//============================================================
//	osd_raw_mouse_lightgun_read
//============================================================

void osd_raw_mouse_lightgun_read(int player,int *deltax,int *deltay) {

	int mousex,mousey;
	int axisx, axisy;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && (use_mouse||use_lightgun) && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// if out of range, skip it
	if (!use_lightgun || !win_physical_width || !win_physical_height || player >= lightgun_count)
	{
		*deltax = *deltay = 0;
		return;
	}

	// for switchable mice and switch mouse axis
	mousex = os_player_mouse_axes[player].xmouse;
	mousey = os_player_mouse_axes[player].ymouse;
	axisx = os_player_mouse_axes[player].xaxis;
	axisy = os_player_mouse_axes[player].yaxis;

	// get the latest lightgun/mouse info if mouse exists
	if ((mousex >= 0) && (mousex < mouse_count))
	{
		switch (axisx)
		{
			case X_AXIS: *deltax = raw_mouse_delta_x[mousex];	break;
			case Y_AXIS: *deltax = raw_mouse_delta_y[mousex];	break;
			case Z_AXIS: *deltax = raw_mouse_delta_z[mousex];	break;

			default:	*deltax = 0;	break;	/* never should happen, but JIC */
		}
		*deltax = (*deltax/256) - 128;
	}
	if ((mousey >= 0) && (mousey < mouse_count)) {
		switch (axisy)
		{
			case X_AXIS: *deltay = raw_mouse_delta_x[mousey];	break;
			case Y_AXIS: *deltay = raw_mouse_delta_y[mousey];	break;
			case Z_AXIS: *deltay = raw_mouse_delta_z[mousey];	break;

			default:	*deltay = 0;	break;	/* never should happen, but JIC */
		}
		*deltay = (*deltay/256) - 128;
	}

	if (*deltax < -128) *deltax = -128;
	if (*deltax > 128) *deltax = 128;
	if (*deltay < -128) *deltay = -128;
	if (*deltay > 128) *deltay = 128;
}
/* </jake> */

#endif /* WINXPANANLOG */
/*end Mame:Analog+*/

//============================================================
//	osd_trak_read
//============================================================

void osd_trak_read(int player, int *deltax, int *deltay)
{
/*start MAME:analog+*/
	int mousex,mousey;
	int axisx, axisy;

	if (use_lightgun2a)
	{
		osd_lightgun_read2a(player,deltax,deltay);
		return;
	}
/*end MAME:analog+  */

	*deltax = 0;
	*deltay = 0;

	if (!use_mouse)
		return ;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

/*start MAME:analog+*/
	mousex = os_player_mouse_axes[player].xmouse;
	mousey = os_player_mouse_axes[player].ymouse;
	axisx = os_player_mouse_axes[player].xaxis;
	axisy = os_player_mouse_axes[player].yaxis;

	// return the latest mouse info if mouse exists
	if ((mousex >= 0) && (mousex < mouse_count))
	{
#ifdef WINXPANANLOG

	/* <jake> */
		if (use_rawmouse_winxp) {
		        switch (axisx)
		        {
			        case X_AXIS: *deltax = raw_mouse_delta_x[mousex];	break;
			        case Y_AXIS: *deltax = raw_mouse_delta_y[mousex];	break;
			        case Z_AXIS: *deltax = raw_mouse_delta_z[mousex];	break;

			        default:	*deltax = 0;	break;	/* never should happen, but JIC */
		        }
		}
		else  // using direct x
	/* </jake> */

#endif /* WINXPANANLOG */
		switch (axisx)
		{
			case X_AXIS: *deltax = mouse_state[mousex].lX;	break;
			case Y_AXIS: *deltax = mouse_state[mousex].lY;	break;
			case Z_AXIS: *deltax = mouse_state[mousex].lZ;	break;

			default:	*deltax = 0;	break;	/* never should happen, but JIC */
		}
	}
	if ((mousey >= 0) && (mousey < mouse_count))
	{
#ifdef WINXPANANLOG

	/* <jake> */
		if (use_rawmouse_winxp) {
		        switch (axisy)
		        {
			        case X_AXIS: *deltay = raw_mouse_delta_x[mousey];	break;
			        case Y_AXIS: *deltay = raw_mouse_delta_y[mousey];	break;
			        case Z_AXIS: *deltay = raw_mouse_delta_z[mousey];	break;

			        default:	*deltay = 0;	break;	/* never should happen, but JIC */
		        }
		}
		else
	/* </jake> */

#endif /* WINXPANANLOG */
		switch (axisy)
		{
			case X_AXIS: *deltay = mouse_state[mousex].lX;	break;
			case Y_AXIS: *deltay = mouse_state[mousex].lY;	break;
			case Z_AXIS: *deltay = mouse_state[mousex].lZ;	break;

			default:	*deltay = 0;	break;	/* never should happen, but JIC */
		}
	}
/*end MAME:analog+  */
}



//============================================================
//	osd_joystick_needs_calibration
//============================================================

int osd_joystick_needs_calibration(void)
{
	return 0;
}



//============================================================
//	osd_joystick_start_calibration
//============================================================

void osd_joystick_start_calibration(void)
{
}



//============================================================
//	osd_joystick_calibrate_next
//============================================================

const char *osd_joystick_calibrate_next(void)
{
	return 0;
}



//============================================================
//	osd_joystick_calibrate
//============================================================

void osd_joystick_calibrate(void)
{
}



//============================================================
//	osd_joystick_end_calibration
//============================================================

void osd_joystick_end_calibration(void)
{
}



//============================================================
//	osd_customize_inputport_defaults
//============================================================

extern struct rc_struct *rc;

void process_ctrlr_file(struct rc_struct *iptrc, const char *ctype, const char *filename)
{
	mame_file *f;

	// open the specified controller type/filename
	f = mame_fopen (ctype, filename, FILETYPE_CTRLR, 0);

	if (f)
	{
		if (verbose)
		{
			if (ctype)
				fprintf (stderr, "trying to parse ctrlr file %s/%s.ini\n", ctype, filename);
			else
				fprintf (stderr, "trying to parse ctrlr file %s.ini\n", filename);
		}

		// process this file
		if(osd_rc_read(iptrc, f, filename, 1, 1))
		{
			if (verbose)
			{
				if (ctype)
					fprintf (stderr, "problem parsing ctrlr file %s/%s.ini\n", ctype, filename);
				else
					fprintf (stderr, "problem parsing ctrlr file %s.ini\n", filename);
			}
		}
	}

	// close the file
	if (f)
		mame_fclose (f);
}

void process_ctrlr_game(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
{
	// recursive call to process parents first
	if (drv->clone_of)
		process_ctrlr_game (iptrc, ctype, drv->clone_of);

	// now process this game
	if (drv->name && *(drv->name) != 0)
		process_ctrlr_file (iptrc, ctype, drv->name);
}

void process_ctrlr_orient(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
{
	/* if this is a vertical game, parse vertical.ini else horizont.ini */	
	if (drv->flags & ORIENTATION_SWAP_XY) {		
		process_ctrlr_file (iptrc, ctype, "vertical");
	} else {		
		process_ctrlr_file (iptrc, ctype, "horizont");
	}
}

void process_ctrlr_players(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
{
	char buffer[128];
	const struct InputPortTiny *input = drv->input_ports;
	int nplayer=0;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		switch (input->type & IPF_PLAYERMASK)
		{
			case IPF_PLAYER1:
				if (nplayer<1) nplayer = 1;
				break;
			case IPF_PLAYER2:
				if (nplayer<2) nplayer = 2;
				break;
			case IPF_PLAYER3:
				if (nplayer<3) nplayer = 3;
				break;
			case IPF_PLAYER4:
				if (nplayer<4) nplayer = 4;
				break;
		}
		++input;
	}
	sprintf(buffer, "player%d", nplayer);
	process_ctrlr_file (iptrc, ctype, buffer);
}

void process_ctrlr_buttons(struct rc_struct *iptrc, const char *ctype, const
struct GameDriver *drv)
{
 char buffer[128];
 const struct InputPortTiny *input = drv->input_ports;
 int no_buttons=0;

 while ((input->type & ~IPF_MASK) != IPT_END)
 {
  switch (input->type & ~IPF_MASK)
  {
  case IPT_BUTTON1:
   if (no_buttons<1) no_buttons = 1;
   break;
  case IPT_BUTTON2:
   if (no_buttons<2) no_buttons = 2;
   break;
  case IPT_BUTTON3:
   if (no_buttons<3) no_buttons = 3;
   break;
  case IPT_BUTTON4:
   if (no_buttons<4) no_buttons = 4;
   break;
  case IPT_BUTTON5:
   if (no_buttons<5) no_buttons = 5;
   break;
  case IPT_BUTTON6:
   if (no_buttons<6) no_buttons = 6;
   break;
  case IPT_BUTTON7:
   if (no_buttons<7) no_buttons = 7;
   break;
  case IPT_BUTTON8:
   if (no_buttons<8) no_buttons = 8;
   break;
  }
  input++;
 }

 sprintf(buffer, "button%d", no_buttons);
 process_ctrlr_file (iptrc, ctype, buffer);
}

// nice hack: load source_file.ini (omit if referenced later any)
void process_ctrlr_system(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
{
	char buffer[128];
	const struct GameDriver *tmp_gd;

	sprintf(buffer, "%s", drv->source_file+12);
	buffer[strlen(buffer) - 2] = 0;

	tmp_gd = drv;
	while (tmp_gd != NULL)
	{
		if (strcmp(tmp_gd->name, buffer) == 0) break;
		tmp_gd = tmp_gd->clone_of;
	}

	// not referenced later, so load it here
	if (tmp_gd == NULL)
		// now process this system
		process_ctrlr_file (iptrc, ctype, buffer);
}

static int ipdef_custom_rc_func(struct rc_option *option, const char *arg, int priority)
{
	struct ik *pinput_keywords = (struct ik *)option->dest;
	struct ipd *idef = ipddef_ptr;

	// only process the default definitions if the input port definitions
	// pointer has been defined
	if (idef)
	{
		// if a keycode was re-assigned
		if (pinput_keywords->type == IKT_STD)
		{
			InputSeq is;

			// get the new keycode
			seq_set_string (&is, arg);

			// was a sequence was assigned to a keycode? - not valid!
			if (is[1] != CODE_NONE)
			{
				fprintf(stderr, "error: can't map \"%s\" to \"%s\"\n",pinput_keywords->name,arg);
			}

			// for all definitions
			while (idef->type != IPT_END)
			{
				int j;

				// reassign all matching keystrokes to the given argument
				for (j = 0; j < SEQ_MAX; j++)
				{
					// if the keystroke matches
					if (idef->seq[j] == pinput_keywords->val)
					{
						// re-assign
						idef->seq[j] = is[0];
					}
				}
				// move to the next definition
				idef++;
			}
		}

		// if an input definition was re-defined
		else if (pinput_keywords->type == IKT_IPT ||
                 pinput_keywords->type == IKT_IPT_EXT)
		{
			// loop through all definitions
			while (idef->type != IPT_END)
			{
				// if the definition matches
				if (idef->type == pinput_keywords->val)
				{
                    if (pinput_keywords->type == IKT_IPT_EXT)
                        idef++;
					seq_set_string(&idef->seq, arg);
					// and abort (there shouldn't be duplicate definitions)
					break;
				}

				// move to the next definition
				idef++;
			}
		}
	}

	return 0;
}


void osd_customize_inputport_defaults(struct ipd *defaults)
{
	static InputSeq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	UINT32 next_reserved = IPT_OSD_1;
	struct ipd *idef = defaults;
	int i;

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// if the type is OSD reserved
		if (idef->type == IPT_OSD_RESERVED)
		{
			// process the next reserved entry
			switch (next_reserved)
			{
				// OSD_1 is alt-enter for fullscreen
				case IPT_OSD_1:
					idef->type = next_reserved;
					idef->name = "Toggle fullscreen";
					seq_set_2 (&idef->seq, KEYCODE_LALT, KEYCODE_ENTER);
				break;

#ifdef MESS
				case IPT_OSD_2:
					if (options.disable_normal_ui)
					{
						idef->type = next_reserved;
						idef->name = "Toggle menubar";
						seq_set_1 (&idef->seq, KEYCODE_SCRLOCK);
					}
				break;
#endif /* MESS */

				default:
				break;
			}
			next_reserved++;
		}

		// disable the config menu if the ALT key is down
		// (allows ALT-TAB to switch between windows apps)
		if (idef->type == IPT_UI_CONFIGURE)
		{
			seq_copy(&idef->seq, &no_alt_tab_seq);
		}

#ifdef MESS
		if (idef->type == IPT_UI_THROTTLE)
		{
			static InputSeq empty_seq = SEQ_DEF_0;
			seq_copy(&idef->seq, &empty_seq);
		}
#endif /* MESS */

		// Dual lightguns - remap default buttons to suit
		if (use_lightgun && use_lightgun_dual) {
			static InputSeq p1b2 = SEQ_DEF_3(KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2);
			static InputSeq p1b3 = SEQ_DEF_3(KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3);
			static InputSeq p2b1 = SEQ_DEF_5(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1, CODE_OR, JOYCODE_MOUSE_1_BUTTON3);
			static InputSeq p2b2 = SEQ_DEF_3(KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2);

			if (idef->type == (IPT_BUTTON2 | IPF_PLAYER1))
				seq_copy(&idef->seq, &p1b2);
			if (idef->type == (IPT_BUTTON3 | IPF_PLAYER1))
				seq_copy(&idef->seq, &p1b3);
			if (idef->type == (IPT_BUTTON1 | IPF_PLAYER2))
				seq_copy(&idef->seq, &p2b1);
			if (idef->type == (IPT_BUTTON2 | IPF_PLAYER2))
				seq_copy(&idef->seq, &p2b2);
		}

		// find the next one
		idef++;
	}

#if 0
	// if a controller type hasn't been specified
	if (ctrlrtype == NULL || *ctrlrtype == 0 || (stricmp(ctrlrtype,"Standard") == 0))
	{
		// default to the legacy controller types if selected
		if (hotrod)
			ctrlrtype = "hotrod";

		if (hotrodse)
			ctrlrtype = "hotrodse";
	}
#endif

	// create a structure for the input port options
	if (!(ctrlr_input_opts = calloc (num_ik+num_osd_ik+1, sizeof(struct rc_option))))
	{
		fprintf(stderr, "error on ctrlr_input_opts creation\n");
		exit(1);
	}

	// Populate the structure with the input_keywords.
	// For all, use the ipdef_custom_rc_func callback.
	// Also, reference the original ik structure.
	for (i=0; i<num_ik+num_osd_ik; i++)
	{
		if (i < num_ik)
		{
	   		ctrlr_input_opts[i].name = input_keywords[i].name;
			ctrlr_input_opts[i].dest = (void *)&input_keywords[i];
		}
		else
		{
	   		ctrlr_input_opts[i].name = osd_input_keywords[i-num_ik].name;
			ctrlr_input_opts[i].dest = (void *)&osd_input_keywords[i-num_ik];
		}
		ctrlr_input_opts[i].shortname = NULL;
		ctrlr_input_opts[i].type = rc_use_function;
		ctrlr_input_opts[i].deflt = NULL;
		ctrlr_input_opts[i].min = 0.0;
		ctrlr_input_opts[i].max = 0.0;
		ctrlr_input_opts[i].func = ipdef_custom_rc_func;
		ctrlr_input_opts[i].help = NULL;
		ctrlr_input_opts[i].priority = 0;
	}

	// add an end-of-opts indicator
	ctrlr_input_opts[i].type = rc_end;

	if (rc_register(rc, ctrlr_input_opts))
	{
		fprintf (stderr, "error on registering ctrlr_input_opts\n");
		exit(1);
	}

	if (rc_register(rc, ctrlr_input_opts2))
	{
		fprintf (stderr, "error on registering ctrlr_input_opts2\n");
		exit(1);
	}

	// set a static variable for the ipdef_custom_rc_func callback
	ipddef_ptr = defaults;

	// process the main platform-specific default file
	process_ctrlr_file (rc, NULL, "windows");

	// if a custom controller has been selected
	if (ctrlrtype && *ctrlrtype != 0 && (stricmp(ctrlrtype,"Standard") != 0))
	{
		const struct InputPortTiny* input = Machine->gamedrv->input_ports;
		int paddle = 0, dial = 0, trackball = 0, adstick = 0, pedal = 0, lightgun = 0;

		// process the controller-specific default file
		process_ctrlr_file (rc, ctrlrtype, "default");

		// process the system-specific files for this controller
		process_ctrlr_system (rc, ctrlrtype, Machine->gamedrv);

		// process the game-specific files for this controller
		process_ctrlr_game (rc, ctrlrtype, Machine->gamedrv);

		// process the orientation-specific files for this controller
		process_ctrlr_orient (rc, ctrlrtype, Machine->gamedrv);

		// process the player-specific files for this controller
		process_ctrlr_players (rc, ctrlrtype, Machine->gamedrv);

		// process the button-specific files for this controller
		process_ctrlr_buttons (rc, ctrlrtype, Machine->gamedrv);

		while ((input->type & ~IPF_MASK) != IPT_END)
		{
			switch (input->type & ~IPF_MASK)
			{
				case IPT_PADDLE:
				case IPT_PADDLE_V:
					if (!paddle)
					{
						if ((paddle_ini != NULL) && (*paddle_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, paddle_ini);
						paddle = 1;
					}
					break;

				case IPT_DIAL:
				case IPT_DIAL_V:
					if (!dial)
					{
						if ((dial_ini != NULL) && (*dial_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, dial_ini);
						dial = 1;
					}
					break;

				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					if (!trackball)
					{
						if ((trackball_ini != NULL) && (*trackball_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, trackball_ini);
						trackball = 1;
					}
					break;

				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
				case IPT_AD_STICK_Z:
					if (!adstick)
					{
						if ((ad_stick_ini != NULL) && (*ad_stick_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, ad_stick_ini);
						adstick = 1;
					}
					break;

				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					if (!lightgun)
					{
						if ((lightgun_ini != NULL) && (*lightgun_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, lightgun_ini);
						lightgun = 1;
					}
					break;

				case IPT_PEDAL:
				case IPT_PEDAL2:
					if (!pedal)
					{
						if ((pedal_ini != NULL) && (*pedal_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, pedal_ini);
						pedal = 1;
					}
					break;

			}
			++input;
		}
	}

	// print the results
	if (verbose)
	{
		if (ctrlrname)
			fprintf (stderr,"\"%s\" controller support enabled\n",ctrlrname);

		fprintf(stderr, "Mouse support %sabled\n",use_mouse ? "en" : "dis");
		fprintf(stderr, "Joystick support %sabled\n",use_joystick ? "en" : "dis");
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d Lightguns=%d(%d,%d)\n",
			keyboard_count,
			use_mouse ? mouse_count : 0,
			use_joystick ? joystick_count : 0,
			use_lightgun ? lightgun_count : 0,
			use_lightgun2a ? lightgun_count : 0,
			use_lightgun2b ? lightgun_count : 0);
	}

	init_analog_seq();
}



//============================================================
//	osd_get_leds
//============================================================

int osd_get_leds(void)
{
	int result = 0;

	if (!use_keyboard_leds)
		return 0;

	// if we're on Win9x, use GetKeyboardState
	if( ledmethod == 0 )
	{
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// set the numlock bit
		result |= (key_states[VK_NUMLOCK] & 1);
		result |= (key_states[VK_CAPITAL] & 1) << 1;
		result |= (key_states[VK_SCROLL] & 1) << 2;
	}
	else if( ledmethod == 1 ) // WinNT/2K/XP, use GetKeyboardState
	{
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// set the numlock bit
		result |= (key_states[VK_NUMLOCK] & 1);
		result |= (key_states[VK_CAPITAL] & 1) << 1;
		result |= (key_states[VK_SCROLL] & 1) << 2;
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer

		// Address first keyboard
		OutputBuffer.UnitId = 0;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
						NULL, 0,
						&OutputBuffer, DataLength,
						&ReturnedLength, NULL);

		// Demangle lights to match 95/98
		if (OutputBuffer.LedFlags & KEYBOARD_NUM_LOCK_ON) result |= 0x1;
		if (OutputBuffer.LedFlags & KEYBOARD_CAPS_LOCK_ON) result |= 0x2;
		if (OutputBuffer.LedFlags & KEYBOARD_SCROLL_LOCK_ON) result |= 0x4;
	}

	return result;
}



//============================================================
//	osd_set_leds
//============================================================

void osd_set_leds(int state)
{
	if (!use_keyboard_leds)
		return;

	// if we're on Win9x, use SetKeyboardState
	if( ledmethod == 0 )
	{
		// thanks to Lee Taylor for the original version of this code
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// mask states and set new states
		key_states[VK_NUMLOCK] = (key_states[VK_NUMLOCK] & ~1) | ((state >> 0) & 1);
		key_states[VK_CAPITAL] = (key_states[VK_CAPITAL] & ~1) | ((state >> 1) & 1);
		key_states[VK_SCROLL] = (key_states[VK_SCROLL] & ~1) | ((state >> 2) & 1);

		SetKeyboardState(&key_states[0]);
	}
	else if( ledmethod == 1 ) // WinNT/2K/XP, use keybd_event()
	{
		int k;
		BYTE keyState[ 256 ];
		const BYTE vk[ 3 ] = { VK_NUMLOCK, VK_CAPITAL, VK_SCROLL };

		GetKeyboardState( (LPBYTE)&keyState );
		for( k = 0; k < 3; k++ )
		{
			if( (  ( ( state >> k ) & 1 ) && !( keyState[ vk[ k ] ] & 1 ) ) ||
				( !( ( state >> k ) & 1 ) &&  ( keyState[ vk[ k ] ] & 1 ) ) )
			{
				// Simulate a key press
				keybd_event( vk[ k ], 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0 );

				// Simulate a key release
				keybd_event( vk[ k ], 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0 );
			}
		}
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer
		UINT				LedFlags=0;

		// Demangle lights to match 95/98
		if (state & 0x1) LedFlags |= KEYBOARD_NUM_LOCK_ON;
		if (state & 0x2) LedFlags |= KEYBOARD_CAPS_LOCK_ON;
		if (state & 0x4) LedFlags |= KEYBOARD_SCROLL_LOCK_ON;

		// Address first keyboard
		InputBuffer.UnitId = 0;
		InputBuffer.LedFlags = LedFlags;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
						&InputBuffer, DataLength,
						NULL, 0,
						&ReturnedLength, NULL);
	}

	return;
}



//============================================================
//	start_led
//============================================================

void start_led(void)
{
	OSVERSIONINFO osinfo = { sizeof(OSVERSIONINFO) };

	if (!use_keyboard_leds)
		return;

	// retrive windows version
	GetVersionEx(&osinfo);

	if ( osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		// 98
		ledmethod = 0;
	}
	else if( strcmp( ledmode, "usb" ) == 0 )
	{
		// nt/2k/xp
		ledmethod = 1;
	}
	else
	{
		// nt/2k/xp
		int error_number;

		ledmethod = 2;

		if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
					"\\Device\\KeyboardClass0"))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}

		hKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
					NULL,	OPEN_EXISTING,	0,	NULL);

		if (hKbdDev == INVALID_HANDLE_VALUE)
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	// remember the initial LED states
	original_leds = osd_get_leds();

	return;
}



//============================================================
//	stop_led
//============================================================

void stop_led(void)
{
	int error_number = 0;

	if (!use_keyboard_leds)
		return;

	// restore the initial LED states
	osd_set_leds(original_leds);

	if( ledmethod == 0 )
	{
	}
	else if( ledmethod == 1 )
	{
	}
	else
	{
		// nt/2k/xp
		if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}

		if (!CloseHandle(hKbdDev))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	return;
}


/*start MAME:analog+*/
// init's os_player_mouse_axes
static void init_mouse_arrays(const int playermousedefault[])
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		os_player_mouse_axes[i].xmouse = playermousedefault[i];
		os_player_mouse_axes[i].ymouse = playermousedefault[i];
		if (splitmouse)
		{
			os_player_mouse_axes[i].xaxis = i % 2;
			os_player_mouse_axes[i].yaxis = i % 2;	// Y axis should not be used, but mapped same axis as X
		}
		else
		{
			os_player_mouse_axes[i].xaxis = X_AXIS;
			os_player_mouse_axes[i].yaxis = Y_AXIS;
		}
	}
}


/* osd UI input functions */
/* trackballs / mice */
int osd_numbermice(void)	// return # of mice connected
{
	if (!use_mouse && !use_lightgun && !use_lightgun2a && !use_lightgun2b)
		return 0;			// if all mouse type inputs are disabled, return zero
	return mouse_count;		// mouse[0] is sysmouse
}

char *osd_getmousename(char *name, int mouse)	// gets name to be displayed
{												// note mouse, !player's mouse
	if ((mouse < mouse_count) && (mouse > 0))
		sprintf(name, "mouse %d", mouse);
	else if (mouse == 0)
		sprintf(name, "sysmouse");
	else
		sprintf(name, "no mouse");

	return name;
}

int osd_getplayer_mouse(int player)				// returns mouse # of player
{
	return os_player_mouse_axes[player].xmouse;
}

int osd_setplayer_mouse(int player, int mouse)	// sets player's mouse
{
	if (mouse >= mouse_count)
		return 1;								// returns 1 if error

	os_player_mouse_axes[player].xmouse = mouse;
	os_player_mouse_axes[player].ymouse = mouse;
	return 0;
}

/* returns mouse # used for player and game axis */
int osd_getplayer_mousesplit(int player, int axis)
{
	if (axis == 0)
		return os_player_mouse_axes[player].xmouse;
	else
		return os_player_mouse_axes[player].ymouse;
}

int osd_setplayer_mousesplit(int player, int playeraxis, int mouse)
{												// sets player's mouse
	if (mouse >= mouse_count)
		return 1;								// returns 1 if error

	if (playeraxis == 0)
		os_player_mouse_axes[player].xmouse = mouse;
	else
		os_player_mouse_axes[player].ymouse = mouse;
	return 0;
}

/* mouse axes settings */
int osd_getnummouseaxes(void)			// returns number of mappable axes
{
	return 2;							// defaults to 2 right now
}


int osd_getplayer_mouseaxis(int player, int gameaxis)
{
	if (gameaxis == 0)
		return os_player_mouse_axes[player].xaxis;
	else
		return os_player_mouse_axes[player].yaxis;
}

int osd_setplayer_mouseaxis(int player, int playeraxis, int mouse, int axis)
{
	switch (playeraxis)	/* switch in case add z-axis */
	{
		case 0:
			os_player_mouse_axes[player].xmouse = mouse;
			os_player_mouse_axes[player].xaxis  = axis;
			break;
		case 1:
			os_player_mouse_axes[player].ymouse = mouse;
			os_player_mouse_axes[player].yaxis  = axis;
			break;
		default:
			/* error */
			break;
	}
	return 0;
}

#if 0 //notused?
int osd_getplayermouseXaxis(int player)
{
	return osd_getplayer_mouseaxis(player, 0);
}
int osd_getplayermouseYaxis(int player)
{
	return osd_getplayer_mouseaxis(player, 1);
}
#endif


int osd_isNegativeSemiAxis(InputCode code)
{
	return ( JOYTYPE( code ) == JOYTYPE_AXIS_NEG );
//	return (is_joycode(code) && JOYTYPE(code) == JOYTYPE_AXIS_NEG);
}

char* osd_getjoyaxisname(char *name, int joynum, int axis)
#define MAX_LENGTH 40
{
	if ((joynum < joystick_count) && (joynum >= 0))
	{
		DIDEVICEOBJECTINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
		result = IDirectInputDevice_GetObjectInfo(joystick_device[joynum], &instance, offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG), DIPH_BYOFFSET);

		if (result == DI_OK)
			snprintf(name, 10, "%s ", instance.tszName);
//			strncpy(name, instance.tszName, 10);
		else
			sprintf(name, " ");
	}
	else
	{
		sprintf(name, " ");
	}
	return name;
}

char *osd_getjoyname(char *name, int joynum)	// gets name to be displayed
{
	if ((joynum < joystick_count) && (joynum >= 0))
	{
		DIDEVICEINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEINSTANCE);
		result = IDirectInputDevice_GetDeviceInfo(joystick_device[joynum], &instance);

		if (FAILED(result))
			sprintf(name, "Joystick %d", joynum+1);
		else
			snprintf(name, 28, "%s ", instance.tszProductName);
//			strncpy(name, instance.tszInstanceName, 25);
	}
	else
	{
		sprintf(name, "No joystick");
	}
	return name;
}

#if 0
int ui_inputtoanalogsettings()
{
}

int ui_analogtoinputsettings()
{
}
#endif
/*end MAME:analog+  */

