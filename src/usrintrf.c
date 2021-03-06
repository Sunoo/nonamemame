/*********************************************************************

	usrintrf.c

	Functions used to handle MAME's user interface.

*********************************************************************/

#include "driver.h"
#include "info.h"
#include "vidhrdw/vector.h"
#include "datafile.h"
#include <windows.h>
#include <stdarg.h>
#include <math.h>
#include "ui_text.h"
#include "state.h"
#include "ui_pal.h"

#ifdef MESS
  #include "mess.h"
#include "mesintrf.h"
#include "inputx.h"
#endif



/***************************************************************************

	Externals

***************************************************************************/

/* Variables for stat menu */
extern char build_version[];
extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];

/* MARTINEZ.F 990207 Memory Card */
#ifndef MESS
#ifndef TINY_COMPILE
#ifndef MMSND
int 		memcard_menu(struct mame_bitmap *bitmap, int);
extern int	mcd_action;
extern int	mcd_number;
extern int	memcard_status;
extern int	memcard_number;
extern int	memcard_manager;
extern const struct GameDriver driver_neogeo;
#endif
#endif
#endif

#if defined(__sgi) && !defined(MESS)
static int game_paused = 0; /* not zero if the game is paused */
#endif

extern int neogeo_memcard_load(int);
extern void neogeo_memcard_save(void);
extern void neogeo_memcard_eject(void);
extern int neogeo_memcard_create(int);
/* MARTINEZ.F 990207 Memory Card End */


#ifdef CMD_LIST
static int command_lastselected;
static char *commandlist_buf = NULL;
static int command_scroll = 0;
static int command_load = 1;
static int command_sc = 0;
static int command_sel = 1;
#endif /* CMD_LIST */


static int lhw, rhw, uaw, daw, law, raw;
static int scroll_reset;
static int displaymessagewindow_under;

#ifdef CMD_LIST
static int display_scroll_message (struct mame_bitmap *bitmap, int *scroll, int width, int height, char *buf);
#else /* CMD_LIST */
static void display_scroll_message (struct mame_bitmap *bitmap, int *scroll, int width, int height, char *buf);
#endif /* CMD_LIST */

#define ROWMARGIN 2
#define ROWHEIGHT       (uirotcharheight + ROWMARGIN)
#define MENUROWHEIGHT   (uirotcharheight + uirotcharheight / 6)

static int wordwrap_text_buffer(char *buffer, int maxwidth);
static int count_lines_in_buffer (char *buffer);

#ifdef UI_COLOR_DISPLAY
#define DRAWBOX_MARGIN 2
#endif /* UI_COLOR_DISPLAY */




/***************************************************************************

	Local variables

***************************************************************************/

static struct GfxElement *uirotfont;

/* raw coordinates, relative to the real scrbitmap */
static struct rectangle uirawbounds;
static int uirawcharwidth, uirawcharheight;

/* rotated coordinates, easier to deal with */
static struct rectangle uirotbounds;
static int uirotwidth, uirotheight;
int uirotcharwidth, uirotcharheight;

static int setup_selected;
static int osd_selected;
static int jukebox_selected;
static int single_step;

static int showfps;
static int showfpstemp;
int showinput = 1;

static int show_profiler;

UINT8 ui_dirty;

/*start MAME:analog+*/
/* from os/input.c */
extern int switchmice;	// add menu item to UI: player to mouse mapping
extern int switchaxes;	// add menu item to UI: assignable mouse axes (more detail than switchmice)
/* from inptport.c */
extern int joymouse;	// option to simulate trackball with analog joystick
/*start MAME:analog+*/

/***************************************************************************

	Font data

***************************************************************************/

static const UINT8 uifontdata[] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* [ 0- 1] */
	0x7c,0x80,0x98,0x90,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x44,0x04,0xf4,0x04,0xf8,	/* [ 2- 3] tape pos 1 */
	0x7c,0x80,0x98,0x88,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x24,0x04,0xf4,0x04,0xf8,	/* [ 4- 5] tape pos 2 */
	0x7c,0x80,0x88,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x24,0x64,0x04,0xf4,0x04,0xf8,	/* [ 6- 7] tape pos 3 */
	0x7c,0x80,0x90,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x44,0x64,0x04,0xf4,0x04,0xf8,	/* [ 8- 9] tape pos 3 */
	0x30,0x48,0x84,0xb4,0xb4,0x84,0x48,0x30,0x30,0x48,0x84,0x84,0x84,0x84,0x48,0x30,	/* [10-11] */
	0x00,0xfc,0x84,0x8c,0xd4,0xa4,0xfc,0x00,0x00,0xfc,0x84,0x84,0x84,0x84,0xfc,0x00,	/* [12-13] */
	0x00,0x38,0x7c,0x7c,0x7c,0x38,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,	/* [14-15] circle & bullet */
	0x80,0xc0,0xe0,0xf0,0xe0,0xc0,0x80,0x00,0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04,0x00,	/* [16-17] R/L triangle */
	0x20,0x70,0xf8,0x20,0x20,0xf8,0x70,0x20,0x48,0x48,0x48,0x48,0x48,0x00,0x48,0x00,
	0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,
	0x70,0xd8,0xe8,0xe8,0xf8,0xf8,0x70,0x00,0x1c,0x7c,0x74,0x44,0x44,0x4c,0xcc,0xc0,
	0x20,0x70,0xf8,0x70,0x70,0x70,0x70,0x00,0x70,0x70,0x70,0x70,0xf8,0x70,0x20,0x00,
	0x00,0x10,0xf8,0xfc,0xf8,0x10,0x00,0x00,0x00,0x20,0x7c,0xfc,0x7c,0x20,0x00,0x00,
	0xb0,0x54,0xb8,0xb8,0x54,0xb0,0x00,0x00,0x00,0x28,0x6c,0xfc,0x6c,0x28,0x00,0x00,
	0x00,0x30,0x30,0x78,0x78,0xfc,0x00,0x00,0xfc,0x78,0x78,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
	0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
	0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
	0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
	0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
	0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
	0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
	0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
	0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
	0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
	0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
	0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
	0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
	0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
	0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
	0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
	0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
	0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
	0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
	0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
	0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
	0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
	0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
	0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
	0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
	0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
	0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
	0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
	0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
	0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
	0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
	0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
	0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
	0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
	0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
	0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x40,0x0c,0x10,0x38,0x10,0x20,0x20,0xc0,0x00,
	0x00,0x00,0x00,0x00,0x00,0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0xa8,0x00,
	0x70,0xa8,0xf8,0x20,0x20,0x20,0x20,0x00,0x70,0xa8,0xf8,0x20,0x20,0xf8,0xa8,0x70,
	0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x44,0xa8,0x50,0x20,0x68,0xd4,0x28,0x00,
	0x88,0x70,0x88,0x60,0x30,0x88,0x70,0x00,0x00,0x10,0x20,0x40,0x20,0x10,0x00,0x00,
	0x78,0xa0,0xa0,0xb0,0xa0,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x20,0x00,0x00,0x00,0x00,0x00,
	0x10,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x28,0x50,0x50,0x00,0x00,0x00,0x00,0x00,
	0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x00,0x00,
	0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x00,0x00,0x00,0x00,
	0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x00,0xf4,0x5c,0x54,0x54,0x00,0x00,0x00,0x00,
	0x88,0x70,0x78,0x80,0x70,0x08,0xf0,0x00,0x00,0x40,0x20,0x10,0x20,0x40,0x00,0x00,
	0x00,0x00,0x70,0xa8,0xb8,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x00,0x20,0x70,0xa8,0xa0,0xa8,0x70,0x20,0x30,0x48,0x40,0xe0,0x40,0x48,0xf0,0x00,
	0x00,0x48,0x30,0x48,0x48,0x30,0x48,0x00,0x88,0x88,0x50,0xf8,0x20,0xf8,0x20,0x00,
	0x20,0x20,0x20,0x00,0x20,0x20,0x20,0x00,0x78,0x80,0x70,0x88,0x70,0x08,0xf0,0x00,
	0xd8,0xd8,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,
	0x60,0x10,0x70,0x90,0x70,0x00,0x00,0x00,0x00,0x28,0x50,0xa0,0x50,0x28,0x00,0x00,
	0x00,0x00,0x00,0xf8,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,
	0x30,0x48,0xb4,0xb4,0xa4,0xb4,0x48,0x30,0x7c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x60,0x90,0x90,0x60,0x00,0x00,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0xf8,0x00,
	0x60,0x90,0x20,0x40,0xf0,0x00,0x00,0x00,0x60,0x90,0x20,0x90,0x60,0x00,0x00,0x00,
	0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0xc8,0xb0,0x80,
	0x78,0xd0,0xd0,0xd0,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x00,0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00,
	0x20,0x50,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x50,0x28,0x50,0xa0,0x00,0x00,
	0x40,0x48,0x50,0x28,0x58,0xa8,0x38,0x08,0x40,0x48,0x50,0x28,0x44,0x98,0x20,0x3c,
	0xc0,0x28,0xd0,0x28,0xd8,0xa8,0x38,0x08,0x20,0x00,0x20,0x40,0x80,0x88,0x70,0x00,
	0x40,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,0x10,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x68,0xb0,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x50,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x20,0x50,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x78,0xa0,0xa0,0xf0,0xa0,0xa0,0xb8,0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x08,0x70,
	0x40,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x10,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x70,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x50,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x40,0x20,0x70,0x20,0x20,0x20,0x70,0x00,0x10,0x20,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x00,0x70,0x20,0x20,0x20,0x70,0x00,0x50,0x00,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x48,0x48,0xe8,0x48,0x48,0x70,0x00,0x68,0xb0,0x88,0xc8,0xa8,0x98,0x88,0x00,
	0x40,0x20,0x70,0x88,0x88,0x88,0x70,0x00,0x10,0x20,0x70,0x88,0x88,0x88,0x70,0x00,
	0x70,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x68,0xb0,0x70,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,
	0x00,0x74,0x88,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x88,0x70,0x00,
	0x10,0x20,0x88,0x88,0x88,0x88,0x70,0x00,0x70,0x00,0x88,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0xa8,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x60,0x90,0x90,0xb0,0x88,0x88,0xb0,0x00,
	0x40,0x20,0x70,0x08,0x78,0x88,0x78,0x00,0x10,0x20,0x70,0x08,0x78,0x88,0x78,0x00,
	0x70,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x68,0xb0,0x70,0x08,0x78,0x88,0x78,0x00,
	0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x20,0x50,0x70,0x08,0x78,0x88,0x78,0x00,
	0x00,0x00,0xf0,0x28,0x78,0xa0,0x78,0x00,0x00,0x00,0x70,0x88,0x80,0x78,0x08,0x70,
	0x40,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,0x10,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x50,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x40,0x20,0x00,0x60,0x20,0x20,0x70,0x00,0x10,0x20,0x00,0x60,0x20,0x20,0x70,0x00,
	0x20,0x50,0x00,0x60,0x20,0x20,0x70,0x00,0x50,0x00,0x00,0x60,0x20,0x20,0x70,0x00,
	0x50,0x60,0x10,0x78,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0xf0,0x88,0x88,0x88,0x00,
	0x40,0x20,0x00,0x70,0x88,0x88,0x70,0x00,0x10,0x20,0x00,0x70,0x88,0x88,0x70,0x00,
	0x20,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0x70,0x88,0x88,0x70,0x00,
	0x00,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x00,0x20,0x00,0xf8,0x00,0x20,0x00,0x00,
	0x00,0x00,0x68,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x98,0x68,0x00,
	0x10,0x20,0x88,0x88,0x88,0x98,0x68,0x00,0x70,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x10,0x20,0x88,0x88,0x88,0x78,0x08,0x70,
	0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x50,0x00,0x88,0x88,0x88,0x78,0x08,0x70
};

static const struct GfxLayout uifontlayout =
{
	6,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};




#if 0
#pragma mark UTILITIES
#endif

/*-------------------------------------------------
	ui_markdirty - mark a raw rectangle dirty
-------------------------------------------------*/

INLINE void ui_markdirty(const struct rectangle *rect)
{
	artwork_mark_ui_dirty(rect->min_x, rect->min_y, rect->max_x, rect->max_y);
	ui_dirty = 5;
}



/*-------------------------------------------------
	ui_raw2rot_rect - convert a rect from raw
	coordinates to rotated coordinates
-------------------------------------------------*/

static void ui_raw2rot_rect(struct rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}
}



/*-------------------------------------------------
	ui_rot2raw_rect - convert a rect from rotated
	coordinates to raw coordinates
-------------------------------------------------*/

static void ui_rot2raw_rect(struct rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}
}



/*-------------------------------------------------
	set_ui_visarea - called by the OSD code to
	set the UI's domain
-------------------------------------------------*/

void set_ui_visarea(int xmin, int ymin, int xmax, int ymax)
{
	/* fill in the rect */
	uirawbounds.min_x = xmin;
	uirawbounds.min_y = ymin;
	uirawbounds.max_x = xmax;
	uirawbounds.max_y = ymax;

	/* orient it */
	uirotbounds = uirawbounds;
	ui_raw2rot_rect(&uirotbounds);

	/* make some easier-to-access globals */
	uirotwidth = uirotbounds.max_x - uirotbounds.min_x + 1;
	uirotheight = uirotbounds.max_y - uirotbounds.min_y + 1;

	/* remove me */
	Machine->uiwidth = uirotbounds.max_x - uirotbounds.min_x + 1;
	Machine->uiheight = uirotbounds.max_y - uirotbounds.min_y + 1;
	Machine->uixmin = uirotbounds.min_x;
	Machine->uiymin = uirotbounds.min_y;

	/* rebuild the font */
	uifont_buildfont();

	uirotfont = Machine->uirotfont;
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		uirotcharwidth = uirawcharheight = Machine->uifontwidth;
		uirotcharheight = uirawcharwidth = Machine->uifontheight;
	}
	else
	{
		uirotcharwidth = uirawcharwidth = Machine->uifontwidth;
		uirotcharheight = uirawcharheight = Machine->uifontheight;
	}
}



/*-------------------------------------------------
	erase_screen - erase the screen
-------------------------------------------------*/

static void erase_screen(struct mame_bitmap *bitmap)
{
	fillbitmap(bitmap, get_black_pen(), NULL);
	schedule_full_refresh();
}



#if 0
#pragma mark -
#pragma mark FONTS & TEXT
#endif

#if 0
/*-------------------------------------------------
	builduifont - build the user interface fonts
-------------------------------------------------*/

struct GfxElement *builduifont(void)
{
	struct GfxLayout layout = uifontlayout;
	UINT32 tempoffset[MAX_GFX_SIZE];
	struct GfxElement *font;
	int temp, i;

	/* free any existing fonts */
	if (Machine->uifont)
		freegfx(Machine->uifont);
	if (uirotfont)
		freegfx(uirotfont);

	/* first decode a straight on version for games */
	Machine->uifont = font = decodegfx(uifontdata, &layout);
	Machine->uifontwidth = layout.width;
	Machine->uifontheight = layout.height;

	/* pixel double horizontally */
	if (uirotwidth >= 420)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i*2+0] = layout.xoffset[i*2+1] = tempoffset[i];
		layout.width *= 2;
	}

	/* pixel double vertically */
	if (uirotheight >= 420)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i*2+0] = layout.yoffset[i*2+1] = tempoffset[i];
		layout.height *= 2;
	}

	/* apply swappage */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		memcpy(layout.xoffset, layout.yoffset, sizeof(layout.xoffset));
		memcpy(layout.yoffset, tempoffset, sizeof(layout.yoffset));

		temp = layout.width;
		layout.width = layout.height;
		layout.height = temp;
	}

	/* apply xflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i] = tempoffset[layout.width - 1 - i];
	}

	/* apply yflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i] = tempoffset[layout.height - 1 - i];
	}

	/* decode rotated font */
	uirotfont = decodegfx(uifontdata, &layout);

	/* set the raw and rotated character width/height */
	uirawcharwidth = layout.width;
	uirawcharheight = layout.height;
	uirotcharwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.height : layout.width;
	uirotcharheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.width : layout.height;

	/* set up the bogus colortable */
	if (font)
	{
		static pen_t colortable[2*2];

		/* colortable will be set at run time */
		font->colortable = colortable;
		font->total_colors = 2;
		uirotfont->colortable = colortable;
		uirotfont->total_colors = 2;
	}

	return font;
}



/*-------------------------------------------------
	ui_drawchar - draw a rotated character
-------------------------------------------------*/

void ui_drawchar(struct mame_bitmap *dest, int ch, int color, int sx, int sy)
{
	struct rectangle bounds;

#ifdef MESS
	extern int skip_this_frame;
	skip_this_frame = 0;
#endif /* MESS */

	/* construct a rectangle in rotated coordinates, then transform it */
	bounds.min_x = sx + uirotbounds.min_x;
	bounds.min_y = sy + uirotbounds.min_y;
	bounds.max_x = bounds.min_x + uirotcharwidth - 1;
	bounds.max_y = bounds.min_y + uirotcharheight - 1;
	ui_rot2raw_rect(&bounds);

	/* now render */
	drawgfx(dest, uirotfont, ch, color, 0, 0, bounds.min_x, bounds.min_y, &uirawbounds, TRANSPARENCY_NONE, 0);

	/* mark dirty */
	ui_markdirty(&bounds);
}
#endif



/*-------------------------------------------------
	ui_text_ex - draw a string to the screen
-------------------------------------------------*/

static void ui_text_ex(struct mame_bitmap *bitmap, const char *buf_begin, const char *buf_end, int x, int y, int color)
{
	struct rectangle bounds;
	int len;
	char buf[256];

	memset(buf, 0, 256);

	len = (buf_end - buf_begin) + 1;
	if (*buf_end == ' ')
		len--;

	memcpy(buf, buf_begin, len);

	bounds.min_x = uirotbounds.min_x + x;
	bounds.min_y = uirotbounds.min_y + x;
	bounds.max_x = bounds.min_x;
	bounds.max_y = bounds.min_y;
	ui_rot2raw_rect(&bounds);

	uifont_drawfont(bitmap, buf, x, y, color);
}



/*-------------------------------------------------
	ui_text - draw a string to the screen
-------------------------------------------------*/

#ifdef UI_COLOR_DISPLAY
void ui_text(struct mame_bitmap *bitmap, const char *buf, int x, int y)
{
	struct rectangle bounds;
	unsigned char *c = (unsigned char *)buf;
	int len = 0;

	while (*c)
	{
		int increment;
		unsigned short code;

		increment = uifont_decodechar((unsigned char *)c, &code);
		len += (code < 256) ? 1 : 2;
		c += increment;
	}

	bounds.min_x = uirotbounds.min_x + x;
	bounds.min_y = uirotbounds.min_y + y;
	bounds.max_x = bounds.min_x + uirotcharwidth * len;
	bounds.max_y = bounds.min_y + uirotcharheight;
	ui_rot2raw_rect(&bounds);

	fillbitmap(bitmap, Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND], &bounds);

	ui_text_ex(bitmap, buf, buf + strlen(buf), x, y, UI_COLOR_NORMAL);
}

#else /* UI_COLOR_DISPLAY */
void ui_text(struct mame_bitmap *bitmap,const char *buf,int x,int y)
{
	ui_text_ex(bitmap, buf, buf + strlen(buf), x, y, UI_COLOR_NORMAL);
}
#endif /* UI_COLOR_DISPLAY */

void ui_text_nobk(struct mame_bitmap *bitmap, const char *buf, int x, int y)
{
	ui_text_ex(bitmap, buf, buf + strlen(buf), x, y, UI_COLOR_NORMAL);
}



/*-------------------------------------------------
	displaytext - display a series of text lines
-------------------------------------------------*/

void displaytext(struct mame_bitmap *bitmap, const struct DisplayText *dt)
{
#if 1
   /* loop until we run out of descriptors */
   for ( ; dt->text; dt++)
   {
      ui_text_ex(bitmap, dt->text, dt->text + strlen(dt->text), dt->x, dt->y, dt->color);
   }
#else
	/* loop until we run out of descriptors */
	for ( ; dt->text; dt++)
#ifdef UI_COLOR_DISPLAY
		ui_text_nobk(bitmap, dt->text, dt->x, dt->y);
#else /* UI_COLOR_DISPLAY */
		ui_text(bitmap, dt->text, dt->x, dt->y);
#endif /* UI_COLOR_DISPLAY */
#endif
}



/*-------------------------------------------------
	multiline_extract - extract one line from a
	multiline buffer; return the number of
	characters in the line; pbegin points to the
	start of the next line
-------------------------------------------------*/

static unsigned multiline_extract(const char **pbegin, const char *end, unsigned maxchars)
{
	const char *begin = *pbegin;
	unsigned numchars = 0;

	/* loop until we hit the end or max out */
	while (begin != end && numchars < maxchars)
	{
		/* if we hit an EOL, strip it and return the current count */
		if (*begin == '\n')
		{
			*pbegin = begin + 1; /* strip final space */
			return numchars;
		}

		/* if we hit a space, word wrap */
		else if (*begin == ' ')
		{
			/* find the end of this word */
			const char* word_end = begin + 1;
			while (word_end != end && *word_end != ' ' && *word_end != '\n')
				++word_end;

			/* if that pushes us past the max, truncate here */
			if (numchars + word_end - begin > maxchars)
			{
				/* if we have at least one character, strip the space */
				if (numchars)
				{
					*pbegin = begin + 1;
					return numchars;
				}

				/* otherwise, take as much as we can */
				else
				{
					*pbegin = begin + maxchars;
					return maxchars;
				}
			}

			/* advance to the end of this word */
			numchars += word_end - begin;
			begin = word_end;
		}

		/* for all other chars, just increment */
		else
		{
			++numchars;
			++begin;
		}
	}

	/* make sure we always make forward progress */
	if (begin != end && (*begin == '\n' || *begin == ' '))
		++begin;
	*pbegin = begin;
	return numchars;
}



/*-------------------------------------------------
	multiline_size - compute the output size of a
	multiline string
-------------------------------------------------*/

static void multiline_size(int *dx, int *dy, const char *begin, const char *end, unsigned maxchars)
{
	unsigned rows = 0;
	unsigned cols = 0;

	/* extract lines until the end, counting rows and tracking the max columns */
	while (begin != end)
	{
		unsigned len;
		len = multiline_extract(&begin, end, maxchars);
		if (len > cols)
			cols = len;
		++rows;
	}

	/* return the final result scaled by the char size */
	*dx = cols * uirotcharwidth;
	*dy = (rows - 1) * ROWHEIGHT + uirotcharheight;
}



/*-------------------------------------------------
	multilinebox_size - compute the output size of
	a multiline string with box
-------------------------------------------------*/

static void multilinebox_size(int *dx, int *dy, const char *begin, const char *end, unsigned maxchars)
{
	/* standard computation, plus an extra char width and height */
	multiline_size(dx, dy, begin, end, maxchars);
	*dx += uirotcharwidth;
	*dy += uirotcharheight;
}



/*-------------------------------------------------
	ui_multitext_ex - display a multiline string
-------------------------------------------------*/

static void ui_multitext_ex(struct mame_bitmap *bitmap, const char *begin, const char *end, unsigned maxchars, int x, int y, int color)
{
	/* extract lines until the end */
	while (begin != end)
	{
		const char *line_begin = begin;
		unsigned len = multiline_extract(&begin, end, maxchars);
		ui_text_ex(bitmap, line_begin, line_begin + len, x, y, color);
		y += ROWHEIGHT;
	}
}



/*-------------------------------------------------
	ui_multitextbox_ex - display a multiline
	string with box
-------------------------------------------------*/

static void ui_multitextbox_ex(struct mame_bitmap *bitmap, const char *begin, const char *end, unsigned maxchars, int x, int y, int dx, int dy, int color)
{
	/* draw the box first */
	ui_drawbox(bitmap, x, y, dx, dy);

	/* indent by half a character */
	x += uirotcharwidth/2;
	y += uirotcharheight/2;

	/* draw the text */
	ui_multitext_ex(bitmap, begin, end, maxchars, x, y, color);
}



/*-------------------------------------------------
	ui_drawbox - draw a black box with white border
-------------------------------------------------*/

void ui_drawbox(struct mame_bitmap *bitmap, int leftx, int topy, int width, int height)
{
	struct rectangle bounds, tbounds;

#ifdef UI_COLOR_DISPLAY
#define DRAWHORIZ(loffs, roffs, y, c) \
	tbounds.min_x = bounds.min_x + loffs; \
	tbounds.max_x = bounds.max_x - roffs; \
	tbounds.min_y = tbounds.max_y = y; \
	ui_rot2raw_rect(&tbounds); \
	fillbitmap(bitmap, c, &tbounds)

#define DRAWVERT(toffs, boffs, x, c) \
	tbounds.min_y = bounds.min_y + toffs; \
	tbounds.max_y = bounds.max_y - boffs; \
	tbounds.min_x = tbounds.max_x = x; \
	ui_rot2raw_rect(&tbounds); \
	fillbitmap(bitmap, c, &tbounds)

	leftx  -= DRAWBOX_MARGIN;
	topy   -= DRAWBOX_MARGIN;
	width  += DRAWBOX_MARGIN * 2;
	height += DRAWBOX_MARGIN * 2;

	/* make a rect and clip it */
	bounds.min_x = uirotbounds.min_x + leftx;
	bounds.min_y = uirotbounds.min_y + topy;
	bounds.max_x = bounds.min_x + width - 1;
	bounds.max_y = bounds.min_y + height - 1;
	sect_rect(&bounds, &uirotbounds);

	/* top edge */
	DRAWHORIZ(0, 0, bounds.min_y,   Machine->uifont->colortable[SYSTEM_COLOR_FRAMELIGHT]);
	DRAWHORIZ(1, 1, bounds.min_y+1, Machine->uifont->colortable[SYSTEM_COLOR_FRAMEDARK]);

	/* bottom edge */
	DRAWHORIZ(1, 1, bounds.max_y-1, Machine->uifont->colortable[SYSTEM_COLOR_FRAMEDARK]);
	DRAWHORIZ(0, 0, bounds.max_y,   Machine->uifont->colortable[SYSTEM_COLOR_FRAMELIGHT]);

	/* left edge */
	DRAWVERT(1, 1, bounds.min_x,   Machine->uifont->colortable[SYSTEM_COLOR_FRAMELIGHT]);
	DRAWVERT(2, 2, bounds.min_x+1, Machine->uifont->colortable[SYSTEM_COLOR_FRAMEDARK]);

	/* right edge */
	DRAWVERT(2, 2, bounds.max_x-1, Machine->uifont->colortable[SYSTEM_COLOR_FRAMEDARK]);
	DRAWVERT(1, 1, bounds.max_x,   Machine->uifont->colortable[SYSTEM_COLOR_FRAMELIGHT]);

	/* fill in the middle with black */
	tbounds = bounds;
	tbounds.min_x += DRAWBOX_MARGIN;
	tbounds.min_y += DRAWBOX_MARGIN;
	tbounds.max_x -= DRAWBOX_MARGIN;
	tbounds.max_y -= DRAWBOX_MARGIN;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND], &tbounds);

#else /* UI_COLOR_DISPLAY */
	pen_t black, white;

	/* make a rect and clip it */
	bounds.min_x = uirotbounds.min_x + leftx;
	bounds.min_y = uirotbounds.min_y + topy;
	bounds.max_x = bounds.min_x + width - 1;
	bounds.max_y = bounds.min_y + height - 1;
	sect_rect(&bounds, &uirotbounds);

	/* pick colors from the colortable */
	black = uirotfont->colortable[0];
	white = uirotfont->colortable[1];

	/* top edge */
	tbounds = bounds;
	tbounds.max_y = tbounds.min_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* bottom edge */
	tbounds = bounds;
	tbounds.min_y = tbounds.max_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* left edge */
	tbounds = bounds;
	tbounds.max_x = tbounds.min_x;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* right edge */
	tbounds = bounds;
	tbounds.min_x = tbounds.max_x;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* fill in the middle with black */
	tbounds = bounds;
	tbounds.min_x++;
	tbounds.min_y++;
	tbounds.max_x--;
	tbounds.max_y--;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, black, &tbounds);
#endif /* UI_COLOR_DISPLAY */

	/* mark things dirty */
	ui_rot2raw_rect(&bounds);
	ui_markdirty(&bounds);
}



/*-------------------------------------------------
	drawbar - draw a thermometer bar
-------------------------------------------------*/

static void drawbar(struct mame_bitmap *bitmap, int leftx, int topy, int width, int height, int percentage, int default_percentage)
{
	struct rectangle bounds, tbounds;
	UINT32 black, white;

	/* make a rect and orient/clip it */
	bounds.min_x = uirotbounds.min_x + leftx;
	bounds.min_y = uirotbounds.min_y + topy;
	bounds.max_x = bounds.min_x + width - 1;
	bounds.max_y = bounds.min_y + height - 1;
	sect_rect(&bounds, &uirotbounds);

	/* pick colors from the colortable */
	black = uirotfont->colortable[0];
	white = uirotfont->colortable[1];

	/* make a rect and orient/clip it */
	bounds.min_x = uirotbounds.min_x + leftx;
	bounds.min_y = uirotbounds.min_y + topy;
	bounds.max_x = bounds.min_x + width - 1;
	bounds.max_y = bounds.min_y + height - 1;
	sect_rect(&bounds, &uirotbounds);

	/* pick colors from the colortable */
	black = uirotfont->colortable[0];
	white = uirotfont->colortable[1];

	/* draw the top default percentage marker */
	tbounds = bounds;
	tbounds.min_x += (width - 1) * default_percentage / 100;
	tbounds.max_x = tbounds.min_x;
	tbounds.max_y = tbounds.min_y + height / 8;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* draw the bottom default percentage marker */
	tbounds = bounds;
	tbounds.min_x += (width - 1) * default_percentage / 100;
	tbounds.max_x = tbounds.min_x;
	tbounds.min_y = tbounds.max_y - height / 8;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* draw the top line of the bar */
	tbounds = bounds;
	tbounds.min_y += height / 8;
	tbounds.max_y = tbounds.min_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* draw the bottom line of the bar */
	tbounds = bounds;
	tbounds.max_y -= height / 8;
	tbounds.min_y = tbounds.max_y;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* fill in the percentage */
	tbounds = bounds;
	tbounds.max_x = tbounds.min_x + (width - 1) * percentage / 100;
	tbounds.min_y += height / 8;
	tbounds.max_y -= height / 8;
	ui_rot2raw_rect(&tbounds);
	fillbitmap(bitmap, white, &tbounds);

	/* mark things dirty */
	ui_rot2raw_rect(&bounds);
	ui_markdirty(&bounds);
}



void ui_displaymenu(struct mame_bitmap *bitmap,const char **items,const char **subitems,char *flag,int selected,int arrowize_subitem)
{
	struct DisplayText dt[256];
	int curr_dt, curr_y;
#ifdef UI_COLOR_DISPLAY
	struct rectangle bounds;
#else /* UI_COLOR_DISPLAY */
	const char *lefthilight = ui_getstring (UI_lefthilight);
	const char *righthilight = ui_getstring (UI_righthilight);
#endif /* UI_COLOR_DISPLAY */
	const char *uparrow = ui_getstring (UI_uparrow);
	const char *downarrow = ui_getstring (UI_downarrow);
	const char *leftarrow = ui_getstring (UI_leftarrow);
	const char *rightarrow = ui_getstring (UI_rightarrow);
	int i,count,len,maxlen,highlen;
	int leftoffs,topoffs,visible,topitem;
	int selected_long;


	i = 0;
	maxlen = 0;
#ifdef UI_COLOR_DISPLAY
	highlen = (uirotwidth - 2 * DRAWBOX_MARGIN) / uirotcharwidth;
#else /* UI_COLOR_DISPLAY */
	highlen = uirotwidth / uirotcharwidth;
#endif /* UI_COLOR_DISPLAY */
	while (items[i])
	{
		len = (raw+law+1) + strlen(items[i]);
		if (subitems && subitems[i])
			len += (lhw+rhw) + strlen(subitems[i]);
		if (len > maxlen)
		{
			if (len <= highlen)
			maxlen = len;
			else
				maxlen = highlen;
		}
		i++;
	}
	count = i;

	visible = (uirotheight / MENUROWHEIGHT) - 1;
	topitem = 0;
	if (visible > count) visible = count;
	else
	{
		topitem = selected - visible / 2;
		if (topitem < 0) topitem = 0;
		if (topitem > count - visible) topitem = count - visible;
	}

	leftoffs = (uirotwidth - maxlen * uirotcharwidth) / 2;
	topoffs = (uirotheight - visible * MENUROWHEIGHT) / 2;

	/* black background */
	ui_drawbox(bitmap,leftoffs,topoffs - ROWMARGIN,maxlen * uirotcharwidth,visible * MENUROWHEIGHT + ROWMARGIN*2);

	selected_long = 0;
	curr_dt = 0;
	for (i = 0;i < visible;i++)
	{
		int item = i + topitem;

		curr_y = topoffs + i * MENUROWHEIGHT + (uirotcharheight / 8);

		if (i == 0 && item > 0)
		{
			dt[curr_dt].text = uparrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * uaw) / 2;
			dt[curr_dt].y = curr_y;
			curr_dt++;
		}
		else if (i == visible - 1 && item < count - 1)
		{
			dt[curr_dt].text = downarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * daw) / 2;
			dt[curr_dt].y = curr_y;
			curr_dt++;
		}
		else
		{
			if (subitems && subitems[item])
			{
				int sublen;
				len = strlen(items[item]);
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = UI_COLOR_NORMAL;
				dt[curr_dt].x = leftoffs + uirotcharwidth/2 + uirotcharwidth * raw;
				dt[curr_dt].y = curr_y;
				curr_dt++;
				sublen = strlen(subitems[item]);
				if (sublen > maxlen-(raw+lhw+rhw+2)-len)
				{
					dt[curr_dt].text = "...";
					sublen = strlen(dt[curr_dt].text);
					if (item == selected)
						selected_long = 1;
				} else {
					dt[curr_dt].text = subitems[item];
				}
				/* If this item is flagged, draw it in inverse print */
				dt[curr_dt].color = (flag && flag[item]) ? UI_COLOR_INVERSE : UI_COLOR_NORMAL;
				dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-rhw-sublen) - uirotcharwidth/2;
				dt[curr_dt].y = curr_y;
				curr_dt++;
			}
			else
			{
				dt[curr_dt].text = items[item];
				dt[curr_dt].color = UI_COLOR_NORMAL;
				dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(items[item])) / 2;
				dt[curr_dt].y = curr_y;
				curr_dt++;
			}
		}
	}

	i = selected - topitem;

	curr_y = topoffs + i * MENUROWHEIGHT + (uirotcharheight / 8);

	if (subitems && subitems[selected] && arrowize_subitem)
	{
		if (arrowize_subitem & 1)
		{
			int sublen;

			len = strlen(items[selected]);

			dt[curr_dt].text = leftarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;

			sublen = strlen(subitems[selected]);
			if (sublen > maxlen-(raw+lhw+rhw+2)-len)
				sublen = strlen("...");

			dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-(lhw+law)-sublen) - uirotcharwidth/2 - raw;
			dt[curr_dt].y = curr_y;
			curr_dt++;
		}
		if (arrowize_subitem & 2)
		{
			dt[curr_dt].text = rightarrow;
			dt[curr_dt].color = UI_COLOR_NORMAL;
			dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-raw) - uirotcharwidth/2;
			dt[curr_dt].y = curr_y;
			curr_dt++;
		}
	}
#ifndef UI_COLOR_DISPLAY
	else
	{
		dt[curr_dt].text = righthilight;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = leftoffs + uirotcharwidth * (maxlen-rhw) - uirotcharwidth/2;
		dt[curr_dt].y = curr_y;
		curr_dt++;
	}
	dt[curr_dt].text = lefthilight;
	dt[curr_dt].color = UI_COLOR_NORMAL;
	dt[curr_dt].x = leftoffs + uirotcharwidth/2;
	dt[curr_dt].y = curr_y;
	curr_dt++;
#endif /* !UI_COLOR_DISPLAY */

	dt[curr_dt].text = 0;	/* terminate array */

#ifdef UI_COLOR_DISPLAY
	bounds.min_x = uirotbounds.min_x + leftoffs;
	bounds.min_y = uirotbounds.min_y + curr_y - 1;
	bounds.max_x = bounds.min_x + maxlen * uirotcharwidth - 1;
	bounds.max_y = bounds.min_y + ROWHEIGHT - 1;

	ui_rot2raw_rect(&bounds);
	fillbitmap(bitmap, Machine->uifont->colortable[CURSOR_COLOR], &bounds);
#endif /* UI_COLOR_DISPLAY */

	displaytext(bitmap,dt);

	if (selected_long)
	{
		int long_dx;
		int long_dy;
		int long_x;
		int long_y;
		unsigned long_max;

#ifdef UI_COLOR_DISPLAY
		long_max = (uirotwidth / uirotcharwidth) - DRAWBOX_MARGIN * 2;
#else /* UI_COLOR_DISPLAY */
		long_max = (uirotwidth / uirotcharwidth) - 2;
#endif /* UI_COLOR_DISPLAY */
		multilinebox_size(&long_dx,&long_dy,subitems[selected],subitems[selected] + strlen(subitems[selected]), long_max);

		long_x = uirotwidth - long_dx;
		long_y = topoffs + (i+1) * MENUROWHEIGHT;
#ifdef UI_COLOR_DISPLAY
		long_x -= DRAWBOX_MARGIN;
#endif /* UI_COLOR_DISPLAY */

		/* if too low display up */
		if (long_y + long_dy > uirotheight)
			long_y = topoffs + i * MENUROWHEIGHT - long_dy;

		ui_multitextbox_ex(bitmap,subitems[selected],subitems[selected] + strlen(subitems[selected]), long_max, long_x,long_y,long_dx,long_dy, UI_COLOR_NORMAL);
	}
}



int ui_displaymessagewindow(struct mame_bitmap *bitmap,const char *text)
{
	struct DisplayText dt[256];
	int curr_dt;
	char *c,*c2;
	int i,maxlen,lines;
	char textcopy[2048];
	int leftoffs,topoffs;
	int maxcols,maxrows;

#ifdef UI_COLOR_DISPLAY
	maxcols = ((uirotwidth - 2 * DRAWBOX_MARGIN) / uirotcharwidth) - 1;
	maxrows = ((uirotheight - 2 * DRAWBOX_MARGIN) / ROWHEIGHT) - 1;
#else /* UI_COLOR_DISPLAY */
	maxcols = (uirotwidth / uirotcharwidth) - 1;
	maxrows = (uirotheight / ROWHEIGHT) - 1;
#endif /* UI_COLOR_DISPLAY */

	if (maxcols < 0)
		maxcols = 0;
	if (maxrows < 0)
		maxrows = 0;

	strcpy(textcopy, text);
	maxlen = wordwrap_text_buffer(textcopy, maxcols);
	lines  = count_lines_in_buffer(textcopy) + 1;

	if (lines > maxrows)
	{
		static int scroll = 0;

		strcat(textcopy, "\n");
		if (scroll_reset)
		{
			scroll = 0;
			scroll_reset = 0;
		}

		display_scroll_message(bitmap, &scroll, maxlen, maxrows, textcopy);

		if ((scroll > 0) && input_ui_pressed_repeat(IPT_UI_UP,4))
		{
			if (scroll == 2) scroll = 0;	/* 1 would be the same as 0, but with arrow on top */
			else scroll--;
			return -1;
		}
		else if (input_ui_pressed_repeat(IPT_UI_DOWN,4))
			{
			if (scroll == 0) scroll = 2;	/* 1 would be the same as 0, but with arrow on top */
			else scroll++;
			return -1;
		}
				else
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			scroll = 0;
			return 1;
			}
		else
		if (input_ui_pressed(IPT_UI_CANCEL))
		{
			scroll = 0;
			return 2;
			}
		else
		if (input_ui_pressed(IPT_UI_CONFIGURE))
		{
			scroll = 0;
			return 3;
		}

		return 0;
	}

	maxlen += 1;

	leftoffs = (uirotwidth - uirotcharwidth * maxlen) / 2;
	if (leftoffs < 0) leftoffs = 0;

	if (!displaymessagewindow_under)
		topoffs = (uirotheight - (lines * ROWHEIGHT + ROWMARGIN)) / 2;
	else
		topoffs = uirotheight - ((lines + 1) * ROWHEIGHT + ROWMARGIN);
	if (topoffs < 0) topoffs = 0;

	/* black background */
	ui_drawbox(bitmap,leftoffs,topoffs,maxlen * uirotcharwidth,lines * ROWHEIGHT + ROWMARGIN);

	curr_dt = 0;
	c = textcopy;
	i = 0;
	while (*c)
	{
		c2 = c;
		while (*c && *c != '\n')
			c++;

		if (*c == '\n')
		{
			*c = '\0';
			c++;
		}

		if (*c2 == '\t')    /* center text */
		{
			c2++;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * (c - c2)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + uirotcharwidth/2;

		dt[curr_dt].text = c2;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].y = topoffs + i * ROWHEIGHT + ROWMARGIN;
		curr_dt++;

		i++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(bitmap,dt);

	return 0;
}




int ui_displaymessagewindow_under(struct mame_bitmap *bitmap,const char *text)
{
	int result;

	displaymessagewindow_under = 1;
	result = ui_displaymessagewindow(bitmap, text);
	displaymessagewindow_under = 0;

	return result;
}



static void showcharset(struct mame_bitmap *bitmap)
{
#ifdef UI_COLOR_DISPLAY
#define ui_text	ui_text_nobk
#endif /* UI_COLOR_DISPLAY */

	int i;
	char buf[80];
	int mode,bank,color,firstdrawn;
	int palpage;
	int changed;
	int total_colors = 0;
	pen_t *colortable = NULL;
	int cpx=0,cpy,skip_chars=0,skip_tmap=0;
	int tilemap_xpos = 0;
	int tilemap_ypos = 0;

	mode = 0;
	bank = 0;
	color = 0;
	firstdrawn = 0;
	palpage = 0;

	changed = 1;

	do
	{
		static const struct rectangle fullrect = { 0, 10000, 0, 10000 };

		/* mark the whole thing dirty */
		ui_markdirty(&fullrect);

		switch (mode)
		{
			case 0: /* palette or clut */
			{
				if (bank == 0)	/* palette */
				{
					total_colors = Machine->drv->total_colors;
					colortable = Machine->pens;
					strcpy(buf,"PALETTE");
				}
				else if (bank == 1)	/* clut */
				{
					total_colors = Machine->drv->color_table_len;
					colortable = Machine->remapped_colortable;
					strcpy(buf,"CLUT");
				}
				else
				{
					buf[0] = 0;
					total_colors = 0;
					colortable = 0;
				}

				/*if (changed) -- temporary */
				{
					erase_screen(bitmap);

					if (total_colors)
					{
						int sx,sy,colors;
						int column_heading_max;
						struct bounds;

						colors = total_colors - 256 * palpage;
						if (colors > 256) colors = 256;

						/* min(colors, 16) */
						if (colors < 16)
							column_heading_max = colors;
						else
							column_heading_max = 16;

						for (i = 0;i < column_heading_max;i++)
						{
							char bf[40];

							sx = 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							sprintf(bf,"%X",i);
							ui_text(bitmap,bf,sx,2*uirotcharheight);
							if (16*i < colors)
							{
								sy = 3*uirotcharheight + (uirotcharheight)*(i % 16);
								sprintf(bf,"%3X",i+16*palpage);
								ui_text(bitmap,bf,0,sy);
							}
						}

						for (i = 0;i < colors;i++)
						{
							struct rectangle bounds;
							bounds.min_x = uirotbounds.min_x + 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							bounds.min_y = uirotbounds.min_y + 2*uirotcharheight + (uirotcharheight)*(i / 16) + uirotcharheight;
							bounds.max_x = bounds.min_x + uirotcharwidth*4/3 - 1;
							bounds.max_y = bounds.min_y + uirotcharheight - 1;
							ui_rot2raw_rect(&bounds);
							fillbitmap(bitmap, colortable[i + 256*palpage], &bounds);
						}
					}
					else
						ui_text(bitmap,"N/A",3*uirotcharwidth,2*uirotcharheight);

					ui_text(bitmap,buf,0,0);
					changed = 0;
				}

				break;
			}
			case 1: /* characters */
			{
				int crotwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->height : Machine->gfx[bank]->width;
				int crotheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->width : Machine->gfx[bank]->height;
				cpx = uirotwidth / crotwidth;
				if (cpx == 0) cpx = 1;
				cpy = (uirotheight - uirotcharheight) / crotheight;
				if (cpy == 0) cpy = 1;
				skip_chars = cpx * cpy;
				/*if (changed) -- temporary */
				{
					int flipx,flipy;
					int lastdrawn=0;

					erase_screen(bitmap);

					/* validity check after char bank change */
					if (firstdrawn >= Machine->gfx[bank]->total_elements)
					{
						firstdrawn = Machine->gfx[bank]->total_elements - skip_chars;
						if (firstdrawn < 0) firstdrawn = 0;
					}

					flipx = 0;
					flipy = 0;

					for (i = 0; i+firstdrawn < Machine->gfx[bank]->total_elements && i<cpx*cpy; i++)
					{
						struct rectangle bounds;
						bounds.min_x = (i % cpx) * crotwidth + uirotbounds.min_x;
						bounds.min_y = uirotcharheight + (i / cpx) * crotheight + uirotbounds.min_y;
						bounds.max_x = bounds.min_x + crotwidth - 1;
						bounds.max_y = bounds.min_y + crotheight - 1;
						ui_rot2raw_rect(&bounds);

						drawgfx(bitmap,Machine->gfx[bank],
								i+firstdrawn,color,  /*sprite num, color*/
								flipx,flipy,bounds.min_x,bounds.min_y,
								0,Machine->gfx[bank]->colortable ? TRANSPARENCY_NONE : TRANSPARENCY_NONE_RAW,0);

						lastdrawn = i+firstdrawn;
					}

					sprintf(buf,"GFXSET %d COLOR %2X CODE %X-%X",bank,color,firstdrawn,lastdrawn);
					ui_text(bitmap,buf,0,0);
					changed = 0;
				}

				break;
			}
			case 2: /* Tilemaps */
			{
				/*if (changed) -- temporary */
				{
					UINT32 tilemap_width, tilemap_height;
					tilemap_nb_size (bank, &tilemap_width, &tilemap_height);
					while (tilemap_xpos < 0)
						tilemap_xpos += tilemap_width;
					tilemap_xpos %= tilemap_width;

					while (tilemap_ypos < 0)
						tilemap_ypos += tilemap_height;
					tilemap_ypos %= tilemap_height;

					erase_screen(bitmap);
					tilemap_nb_draw (bitmap, bank, tilemap_xpos, tilemap_ypos);
					sprintf(buf, "TILEMAP %d (%dx%d)  X:%d  Y:%d", bank, tilemap_width, tilemap_height, tilemap_xpos, tilemap_ypos);
					ui_text(bitmap,buf,0,0);
					changed = 0;
					skip_tmap = 0;
				}
				break;
			}
		}

		update_video_and_audio();

		if (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL))
		{
			skip_chars = cpx;
			skip_tmap = 8;
		}
		if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		{
			skip_chars = 1;
			skip_tmap = 1;
		}


		if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
		{
			int next_bank, next_mode;
			int jumped;

			next_mode = mode;
			next_bank = bank+1;
			do {
				jumped = 0;
				switch (next_mode)
				{
					case 0:
						if (next_bank == 2 || Machine->drv->color_table_len == 0)
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 1:
						if (next_bank == MAX_GFX_ELEMENTS || !Machine->gfx[next_bank])
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 2:
						if (next_bank == tilemap_count())
							next_mode = -1;
						break;
				}
			}	while (jumped);
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
//				firstdrawn = 0;
				changed = 1;
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
		{
			int next_bank, next_mode;

			next_mode = mode;
			next_bank = bank-1;
			while(next_bank < 0 && next_mode >= 0)
			{
				next_mode = next_mode - 1;
				switch (next_mode)
				{
					case 0:
						if (Machine->drv->color_table_len == 0)
							next_bank = 0;
						else
							next_bank = 1;
						break;
					case 1:
						next_bank = MAX_GFX_ELEMENTS-1;
						while (next_bank >= 0 && !Machine->gfx[next_bank])
							next_bank--;
						break;
					case 2:
						next_bank = tilemap_count() - 1;
						break;
				}
			}
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
//				firstdrawn = 0;
				changed = 1;
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGDN,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (256 * (palpage + 1) < total_colors)
					{
						palpage++;
						changed = 1;
					}
					break;
				}
				case 1:
				{
					if (firstdrawn + skip_chars < Machine->gfx[bank]->total_elements)
					{
						firstdrawn += skip_chars;
						changed = 1;
					}
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos -= skip_tmap;
					else
						tilemap_ypos -= bitmap->height/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGUP,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (palpage > 0)
					{
						palpage--;
						changed = 1;
					}
					break;
				}
				case 1:
				{
					firstdrawn -= skip_chars;
					if (firstdrawn < 0) firstdrawn = 0;
					changed = 1;
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos += skip_tmap;
					else
						tilemap_ypos += bitmap->height/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_D,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos -= skip_tmap;
					else
						tilemap_xpos -= bitmap->width/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_G,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos += skip_tmap;
					else
						tilemap_xpos += bitmap->width/4;
					changed = 1;
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_UP,6))
		{
			switch (mode)
			{
				case 1:
				{
					if (color < Machine->gfx[bank]->total_colors - 1)
					{
						color++;
						changed = 1;
					}
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_DOWN,6))
		{
			switch (mode)
			{
				case 0:
					break;
				case 1:
				{
					if (color > 0)
					{
						color--;
						changed = 1;
					}
				}
			}
		}

		if (input_ui_pressed(IPT_UI_SNAPSHOT))
			save_screen_snapshot(bitmap);
	} while (!input_ui_pressed(IPT_UI_SHOW_GFX) &&
			!input_ui_pressed(IPT_UI_CANCEL));

	schedule_full_refresh();
#ifdef UI_COLOR_DISPLAY
#undef ui_text
#endif /* UI_COLOR_DISPLAY */
}



static int switchmenu(struct mame_bitmap *bitmap, int selected, UINT32 switch_name, UINT32 switch_setting)
{
	const char *menu_item[128];
	const char *menu_subitem[128];
	struct InputPort *entry[128];
	char flag[40];
	int i,sel;
	struct InputPort *in;
	int total;
	int arrowize;


	sel = selected - 1;


	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		if (in->type == switch_name && input_port_active(in))
		{
			entry[total] = in;
			menu_item[total] = input_port_name(in);

			total++;
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_item[total + 1] = 0;	/* terminate array */
	total++;


	for (i = 0;i < total;i++)
	{
		flag[i] = 0; /* TODO: flag the dip if it's not the real default */
		if (i < total - 1)
		{
			in = entry[i] + 1;
			while (in->type == switch_setting &&
					in->default_value != entry[i]->default_value)
				in++;

			if (in->type != switch_setting)
				menu_subitem[i] = ui_getstring (UI_INVALID);
			else menu_subitem[i] = input_port_name(in);
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	arrowize = 0;
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while (in->type == switch_setting &&
				in->default_value != entry[sel]->default_value)
			in++;

		if (in->type != switch_setting)
			/* invalid setting: revert to a valid one */
			arrowize |= 1;
		else
		{
			if ((in-1)->type == switch_setting)
				arrowize |= 1;
		}
	}
	if (sel < total - 1)
	{
		in = entry[sel] + 1;
		while (in->type == switch_setting &&
				in->default_value != entry[sel]->default_value)
			in++;

		if (in->type != switch_setting)
			/* invalid setting: revert to a valid one */
			arrowize |= 2;
		else
		{
			if ((in+1)->type == switch_setting)
				arrowize |= 2;
		}
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while (in->type == switch_setting &&
					in->default_value != entry[sel]->default_value)
				in++;

			if (in->type != switch_setting)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if ((in+1)->type == switch_setting)
					entry[sel]->default_value = (in+1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if (sel < total - 1)
		{
			in = entry[sel] + 1;
			while (in->type == switch_setting &&
					in->default_value != entry[sel]->default_value)
				in++;

			if (in->type != switch_setting)
				/* invalid setting: revert to a valid one */
				entry[sel]->default_value = (entry[sel]+1)->default_value & entry[sel]->mask;
			else
			{
				if ((in-1)->type == switch_setting)
					entry[sel]->default_value = (in-1)->default_value & entry[sel]->mask;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}



static int setdipswitches(struct mame_bitmap *bitmap, int selected)
{
	return switchmenu(bitmap, selected, IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
}



#ifdef MESS
static int setconfiguration(struct mame_bitmap *bitmap, int selected)
{
	return switchmenu(bitmap, selected, IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
}



static int setcategories(struct mame_bitmap *bitmap, int selected)
{
	return switchmenu(bitmap, selected, IPT_CATEGORY_NAME, IPT_CATEGORY_SETTING);
}
#endif /* MESS */



/* This flag is used for record OR sequence of key/joy */
/* when is !=0 the first sequence is record, otherwise the first free */
/* it's used by setdefkeysettings, setdefjoysettings, setkeysettings, setjoysettings */
static int record_first_insert = 1;

static char menu_subitem_buffer[500][96];

static int setdefcodesettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[500];
	const char *menu_subitem[500];
	InputSeq *entry[500];
	char flag[500];
	int i,sel;
	struct ipd *in;
	int total;
	int visible;
	extern struct ipd inputport_defaults[];
	static int counter = 0;
	static int fast = 4;

	sel = selected - 1;
	visible = (uirotheight / MENUROWHEIGHT) - 3;


	if (Machine->input_ports == 0)
		return 0;

	in = inputport_defaults;

	total = 0;
	while (in->type != IPT_END)
	{
		if (in->name != 0  && in->type != IPT_UNKNOWN && in->type != IPT_OSD_RESERVED)
		{
			int seq_count = (in->type > IPT_ANALOG_START && in->type < IPT_ANALOG_END) ? 2 : 1;
			int j;
			for (j = 0; j < seq_count; j++)
			{
				entry[total] = &in->seq[j];
				menu_item[total] = in->name;
				if (j == 1 && in->type >= IPT_PEDAL && in->type <= IPT_PEDAL2)
					menu_item[total] = "Auto Release <Y/N>";

				total++;
			}
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	for (i = 0;i < total;i++)
	{
		if (i < total - 1)
		{
			seq_name(entry[i],menu_subitem_buffer[i],sizeof(menu_subitem_buffer[0]));
			menu_subitem[i] = menu_subitem_buffer[i];
		} else
			menu_subitem[i] = 0;	/* no subitem */
		flag[i] = 0;
	}

	if (sel > SEL_MASK)   /* are we waiting for a new key? */
	{
		int ret;

		menu_subitem[sel & SEL_MASK] = "    ";
		ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel & SEL_MASK,3);

		ret = seq_read_async(entry[sel & SEL_MASK],record_first_insert);

		if (ret >= 0)
		{
			sel &= SEL_MASK;

			if (ret > 0 || seq_get_1(entry[sel]) == CODE_NONE)
			{
				seq_set_1(entry[sel],CODE_NONE);
				ret = 1;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();

			record_first_insert = ret != 0;
		}

		init_analog_seq();

		return sel + 1;
	}

/*start MAME:analog+*/
	init_analog_seq();
/*end MAME:analog+  */

	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,fast))
	{
		sel = (sel + 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		sel = (sel + total - 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			seq_read_async_start();

			sel |= 1 << SEL_BITS;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		/* tell updatescreen() to clean after us */
		schedule_full_refresh();

		record_first_insert = 1;
	}

	return sel + 1;
}



static int setcodesettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[500];
	const char *menu_subitem[500];
	InputSeq *seq[500];
	char flag[500];
	int i,sel;
	struct InputPort *in;
	int total;
	int visible;
	static int counter = 0;
	static int fast = 4;


	sel = selected - 1;
	visible = (uirotheight / MENUROWHEIGHT) - 3;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	total = 0;
	while (in->type != IPT_END)
	{
		for (i = 0; i < input_port_seq_count(in); i++)
		{
			if (input_port_name(in) != 0 && seq_get_1(&in->seq[i]) != CODE_NONE
				&& (in->type != IPT_UNKNOWN)
#ifdef MESS
				&& ((in->category == 0) || input_category_active(in->category))
#endif /* MESS */
				&& in->type != IPT_OSD_RESERVED)
			{
				seq[total] = &in->seq[i];
				menu_item[total] = input_port_name(in);
				if (i == 1 && in->type >= IPT_PEDAL && in->type <= IPT_PEDAL2)
					menu_item[total] = "Auto Release <Y/N>";

				seq_name(input_port_seq(in, i), menu_subitem_buffer[total], sizeof(menu_subitem_buffer[0]));
				menu_subitem[total] = menu_subitem_buffer[total];

				/* If the key isn't the default, flag it */
				if (seq_get_1(seq[total]) != CODE_DEFAULT)
					flag[total] = 1;
				else
					flag[total] = 0;

				total++;
			}
		}

		in++;
	}

	if (total == 0) return 0;

	menu_item[total] = ui_getstring (UI_returntomain);
	menu_subitem[total] = 0;	/* no subitem */
	menu_item[total + 1] = 0;	/* terminate array */
	total++;

	if (sel > SEL_MASK)   /* are we waiting for a new key? */
	{
		int ret;

		menu_subitem[sel & SEL_MASK] = "    ";
		ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel & SEL_MASK,3);

		ret = seq_read_async(seq[sel & SEL_MASK],record_first_insert);

		if (ret >= 0)
		{
			sel &= SEL_MASK;

			if (ret > 0 || seq_get_1(seq[sel]) == CODE_NONE)
			{
				seq_set_1(seq[sel], CODE_DEFAULT);
				ret = 1;
			}

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();

			record_first_insert = ret != 0;
		}

		init_analog_seq();

		return sel + 1;
	}


	ui_displaymenu(bitmap,menu_item,menu_subitem,flag,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,fast))
	{
		sel = (sel + 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
	{
		sel = (sel + total - 1) % total;
		record_first_insert = 1;
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total - 1) sel = -1;
		else
		{
			seq_read_async_start();

			sel |= 1 << SEL_BITS;	/* we'll ask for a key */

			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();

		record_first_insert = 1;
	}

	return sel + 1;
}


static int calibratejoysticks(struct mame_bitmap *bitmap,int selected)
{
	const char *msg;
	static char buf[2048];
	int sel;
	static int calibration_started = 0;

	sel = selected - 1;

	if (calibration_started == 0)
	{
		osd_joystick_start_calibration();
		calibration_started = 1;
		strcpy (buf, "");
	}

	if (sel > SEL_MASK) /* Waiting for the user to acknowledge joystick movement */
	{
		if (input_ui_pressed(IPT_UI_CANCEL))
		{
			calibration_started = 0;
			sel = -1;
		}
		else if (input_ui_pressed(IPT_UI_SELECT))
		{
			osd_joystick_calibrate();
			sel &= SEL_MASK;
		}

		ui_displaymessagewindow(bitmap,buf);
	}
	else
	{
		msg = osd_joystick_calibrate_next();
		schedule_full_refresh();
		if (msg == 0)
		{
			calibration_started = 0;
			osd_joystick_end_calibration();
			sel = -1;
		}
		else
		{
			strcpy (buf, msg);
			ui_displaymessagewindow(bitmap,buf);
			sel |= 1 << SEL_BITS;
		}
	}

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


static int settraksettings(struct mame_bitmap *bitmap,int selected)
{
	const char *menu_item[40];
	const char *menu_subitem[40];
	struct InputPort *entry[40];
	int i,sel;
	struct InputPort *in;
	int total,total2;
	int arrowize;


	sel = selected - 1;


	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	/* Count the total number of analog controls */
	total = 0;
	while (in->type != IPT_END)
	{
		if (in->type > IPT_ANALOG_START && in->type < IPT_ANALOG_END)
		{
			entry[total] = in;
			total++;
		}
		in++;
	}

	if (total == 0) return 0;

	/* Each analog control has 4 entries - key & joy delta, reverse, sensitivity, center */

#define ENTRIES 4

	total2 = total * ENTRIES;

	menu_item[total2] = ui_getstring (UI_returntomain);
	menu_item[total2 + 1] = 0;	/* terminate array */
	total2++;

	arrowize = 0;
	for (i = 0;i < total2;i++)
	{
		if (i < total2 - 1)
		{
			char label[30][40];
			char setting[30][40];
			int sensitivity,delta;
			int reverse;
			int center;

			strcpy (label[i], input_port_name(entry[i/ENTRIES]));
			sensitivity = entry[i/ENTRIES]->u.analog.sensitivity;
			delta = entry[i/ENTRIES]->u.analog.delta;
			reverse = entry[i/ENTRIES]->u.analog.reverse;

			strcat (label[i], " ");
			switch (i%ENTRIES)
			{
				case 0:
					strcat (label[i], ui_getstring (UI_keyjoyspeed));
					sprintf(setting[i],"%d",delta);
					if (i == sel) arrowize = 3;
					break;
				case 1:
					strcat (label[i], ui_getstring (UI_reverse));
					if (reverse)
						strcpy(setting[i],ui_getstring (UI_on));
					else
						strcpy(setting[i],ui_getstring (UI_off));
					if (i == sel) arrowize = 3;
					break;
				case 2:
					strcat (label[i], ui_getstring (UI_sensitivity));
					sprintf(setting[i],"%3d%%",sensitivity);
					if (i == sel) arrowize = 3;
					break;
				case 3:
					strcat (label[i], ui_getstring (UI_center));
					if (center)
						// sprintf(setting[i],ui_getstring (UI_on));
						strcpy(setting[i],ui_getstring (UI_on));
					else
						// sprintf(setting[i],ui_getstring (UI_off));
						strcpy(setting[i],ui_getstring (UI_off));
					if (i == sel) arrowize = 3;
					break;
			}

			menu_item[i] = label[i];
			menu_subitem[i] = setting[i];

			in++;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,0,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total2 - 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if(sel != total2 - 1)
		{
			if ((sel % ENTRIES) == 0)
			{
				/* keyboard/joystick delta */
				int val = entry[sel/ENTRIES]->u.analog.delta;

				val --;
				if (val < 1) val = 1;
				entry[sel/ENTRIES]->u.analog.delta = val;
			}
			else if ((sel % ENTRIES) == 1)
			{
				/* reverse */
				entry[sel/ENTRIES]->u.analog.reverse ^= 1;
			}
			else if ((sel % ENTRIES) == 2)
			{
				/* sensitivity */
				int val = entry[sel/ENTRIES]->u.analog.sensitivity;

				val --;
				if (val < 1) val = 1;
				entry[sel/ENTRIES]->u.analog.sensitivity = val;
			}
			else if ((sel % ENTRIES) == 3)
			/* center */
			{
//REMOVED FOR MAME0.84u1
/*
				int center = entry[sel/ENTRIES]->type & IPF_CENTER;
				if (center)
					center=0;
				else
					center=IPF_CENTER;
				entry[sel/ENTRIES]->type &= ~IPF_CENTER;
				entry[sel/ENTRIES]->type |= center;
*/
			}
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if(sel != total2 - 1)
		{
			if ((sel % ENTRIES) == 0)
			{
				/* keyboard/joystick delta */
				int val = entry[sel/ENTRIES]->u.analog.delta;

				val ++;
				if (val > 255) val = 255;
				entry[sel/ENTRIES]->u.analog.delta = val;
			}
			else if ((sel % ENTRIES) == 1)
			{
				/* reverse */
				entry[sel/ENTRIES]->u.analog.reverse ^= 1;
			}
			else if ((sel % ENTRIES) == 2)
			{
				/* sensitivity */
				int val = entry[sel/ENTRIES]->u.analog.sensitivity;

				val ++;
				if (val > 255) val = 255;
				entry[sel/ENTRIES]->u.analog.sensitivity = val;
			}
			else if ((sel % ENTRIES) == 3)
			/* center */
			{
//REMOVED FOR MAME0.84u1
/*
				int center = entry[sel/ENTRIES]->type & IPF_CENTER;
				if (center)
					center=0;
				else
					center=IPF_CENTER;
				entry[sel/ENTRIES]->type &= ~IPF_CENTER;
				entry[sel/ENTRIES]->type |= center;
*/
			}
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total2 - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
/*start MAME:analog+*/
#undef ENTRIES
/*end MAME:analog+  */
}

/*start MAME:analog+*/
static int setplayermousesettings(struct mame_bitmap *bitmap,int selected)
{
#define MAX_LENGTH 40
	const char *menu_item[MAX_LENGTH];
	const char *menu_subitem[MAX_LENGTH];
	int i,sel;
	struct InputPort *in;
	int nummice;
	int total,total2;
	int arrowize;

	sel = selected - 1;

	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	/* Count the number of players with analog controls */
	total = 0;
//REMOVED FOR MAME0.84u1
/*
	while (in->type != IPT_END)
	{
		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
				&& !(!options.cheat && (in->type & IPF_CHEAT)))
		{
			switch (in->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (total<1) total = 1;
					break;
				case IPF_PLAYER2:
					if (total<2) total = 2;
					break;
				case IPF_PLAYER3:
					if (total<3) total = 3;
					break;
				case IPF_PLAYER4:
					if (total<4) total = 4;
					break;
			}
		}
		in++;
	}
*/
	if (total == 0) return 0;

	/* Count number of mice connected */
	nummice = osd_numbermice();

	if (nummice == 0) return 0;

	/* Each player's mouse analog control has 1 entry - mouse number */
	/* see setplayermouseaxissettings() to switch axes too */
#define ENTRIES 1

	total2 = total * ENTRIES;

	menu_item[total2] = ui_getstring (UI_returntomain);
	menu_item[total2 + 1] = 0;	/* terminate array */
	total2++;

	arrowize = 0;
	for (i = 0;i < total2;i++)
	{
		if (i < total2 - 1)
		{
			char label[10*ENTRIES][40];
			char setting[10*ENTRIES][40];
			int mouse;

			sprintf (label[i], "Player %d",i/ENTRIES);	// should get string from elsewhere
			mouse = osd_getplayer_mouse(i/ENTRIES);

			strcat (label[i], " ");
			switch (i%ENTRIES)
			{
				case 0:
					strcat (label[i], ui_getstring (UI_switchmouse));
					osd_getmousename(setting[i], mouse);
					if (i == sel) arrowize = 3;
					break;
			}

			menu_item[i] = label[i];
			menu_subitem[i] = setting[i];

//			in++;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,0,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total2 - 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if ((sel % ENTRIES) == 0)
		/* set mouse */
		{
			int mouse = osd_getplayer_mouse(sel/ENTRIES);

			mouse--;
			if (mouse < -1) mouse = nummice-1;
			osd_setplayer_mouse(sel/ENTRIES, mouse);
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if ((sel % ENTRIES) == 0)
		/* set mouse */
		{
			int mouse = osd_getplayer_mouse(sel/ENTRIES);

			mouse++;
			if (mouse >= nummice) mouse = -1;
			osd_setplayer_mouse(sel/ENTRIES, mouse);
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total2 - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
#undef ENTRIES
}

static int setplayermouseaxessettings(struct mame_bitmap *bitmap,int selected)
{
#define MAX_LENGTH 40
	const char *menu_item[MAX_LENGTH];
	const char *menu_subitem[MAX_LENGTH];
	int i,sel;
	struct InputPort *in;
	int nummice;
	int total,total2;
	int arrowize;

	sel = selected - 1;

	if (Machine->input_ports == 0)
		return 0;

	in = Machine->input_ports;

	/* Count the number of players with analog controls */
	total = 0;
//REMOVED FOR MAME0.84u1
/*
	while (in->type != IPT_END)
	{
		if (((in->type & 0xff) > IPT_ANALOG_START) && ((in->type & 0xff) < IPT_ANALOG_END)
				&& !(!options.cheat && (in->type & IPF_CHEAT)))
		{
			switch (in->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (total<1) total = 1;
					break;
				case IPF_PLAYER2:
					if (total<2) total = 2;
					break;
				case IPF_PLAYER3:
					if (total<3) total = 3;
					break;
				case IPF_PLAYER4:
					if (total<4) total = 4;
					break;
				case IPF_PLAYER5:
					if (total<5) total = 5;
					break;
				case IPF_PLAYER6:
					if (total<6) total = 6;
					break;
				case IPF_PLAYER7:
					if (total<7) total = 7;
					break;
				case IPF_PLAYER8:
					if (total<8) total = 8;
					break;
			}
		}
		in++;
	}
*/
	if (total == 0) return 0;

	/* Count number of mice connected */
	nummice = osd_numbermice();

	if (nummice == 0) return 0;

	/* Each player's mouse control has 2 entries - x-axis, y-axis */
	// will change to add Z axis
#define ENTRIES 2

	total2 = total * ENTRIES;

	menu_item[total2] = ui_getstring (UI_returntomain);
	menu_item[total2 + 1] = 0;	/* terminate array */
	total2++;

	arrowize = 0;
	for (i = 0;i < total2;i++)
	{
		if (i < total2 - 1)
		{
			char label[30][MAX_LENGTH];
			char setting[30][MAX_LENGTH];
			int mouse, mouseaxis;
			const char *mouseaxisname;
			const char *XAXISNAME = ui_getstring( UI_Xaxis ), *YAXISNAME = ui_getstring( UI_Yaxis );
//			const char *ZAXISNAME = ui_getstring( UI_Zaxis );

			sprintf (label[i], "Player %d",i/ENTRIES+1);	// should get string from elsewhere
			mouse = osd_getplayer_mousesplit(i/ENTRIES, i%ENTRIES);
			mouseaxis = osd_getplayer_mouseaxis(i/ENTRIES, i%ENTRIES);
			mouseaxisname = mouseaxis ? YAXISNAME : XAXISNAME;

			strcat (label[i], " ");
			switch (i%ENTRIES)
			{
				case 0:
					strcat (label[i], "X axis ");
					strcat (label[i], ui_getstring (UI_mouse));
					osd_getmousename(setting[i], mouse);
					if (mouse != -1)
						strcat (setting[i], mouseaxisname);
					if (i == sel) arrowize = 3;
					break;
				case 1:
					strcat (label[i], "Y axis ");
					strcat (label[i], ui_getstring (UI_mouse));
					osd_getmousename(setting[i], mouse);
					if (mouse != -1)
						strcat (setting[i], mouseaxisname);
					if (i == sel) arrowize = 3;
					break;

				case 2:		// Not used, yet
					strcat (label[i], "Z axis ");
					strcat (label[i], ui_getstring (UI_mouse));
					osd_getmousename(setting[i], mouse);
					if (mouse != -1)
						strcat (setting[i], mouseaxisname);
					if (i == sel) arrowize = 3;
					break;
			}

			menu_item[i] = label[i];
			menu_subitem[i] = setting[i];

//			in++;
		}
		else menu_subitem[i] = 0;	/* no subitem */
	}

	ui_displaymenu(bitmap,menu_item,menu_subitem,0,sel,arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + total2 - 1) % total2;

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		int mouse = osd_getplayer_mousesplit(sel/ENTRIES, sel%ENTRIES);
		int axis = osd_getplayer_mouseaxis(sel/ENTRIES, sel%ENTRIES);

		if (mouse != -1)
		{
			axis--;
			if (axis < 0)
			{
				axis = 1;
				mouse--;
			}
		}
		else mouse = nummice-1;
		osd_setplayer_mouseaxis(sel/ENTRIES, sel%ENTRIES, mouse, axis);
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		int mouse = osd_getplayer_mousesplit(sel/ENTRIES, sel%ENTRIES);
		int axis = osd_getplayer_mouseaxis(sel/ENTRIES, sel%ENTRIES);

		axis++;
		if (axis > 1)
		{
			axis = 0;
			mouse++;
		}
		if (mouse >= nummice)
		{
			mouse = -1;
			axis = 1;
		}
		osd_setplayer_mouseaxis(sel/ENTRIES, sel%ENTRIES, mouse, axis);
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == total2 - 1) sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
#undef ENTRIES
}
/*end MAME:analog+  */

#ifndef MESS
static int mame_stats(struct mame_bitmap *bitmap,int selected)
{
	char temp[10];
	char buf[2048];
	int sel, i;


	sel = selected - 1;

	buf[0] = 0;

	if (dispensed_tickets)
	{
		strcat(buf, ui_getstring (UI_tickets));
		strcat(buf, ": ");
		sprintf(temp, "%d\n\n", dispensed_tickets);
		strcat(buf, temp);
	}

	for (i=0; i<COIN_COUNTERS; i++)
	{
		strcat(buf, ui_getstring (UI_coin));
		sprintf(temp, " %c: ", i+'A');
		strcat(buf, temp);
		if (!coins[i])
			strcat (buf, ui_getstring (UI_NA));
		else
		{
			sprintf (temp, "%d", coins[i]);
			strcat (buf, temp);
		}
		if (coinlockedout[i])
		{
			strcat(buf, " ");
			strcat(buf, ui_getstring (UI_locked));
			strcat(buf, "\n");
		}
		else
		{
			strcat(buf, "\n");
		}
	}

	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t");
		strcat(buf,ui_getstring (UI_lefthilight));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_returntomain));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_righthilight));

		ui_displaymessagewindow(bitmap,buf);

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}
#endif

int showcopyright(struct mame_bitmap *bitmap)
{
#ifdef UI_COLOR_DISPLAY
	int back = Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND];
#endif /* UI_COLOR_DISPLAY */
	int done;
	char buf[1000];
	char buf2[256];

	strcpy (buf, ui_getstring(UI_copyright1));
	strcat (buf, "\n\n");
	sprintf(buf2, ui_getstring(UI_copyright2), Machine->gamedrv->description);
	strcat (buf, buf2);
	strcat (buf, "\n\n");
	strcat (buf, ui_getstring(UI_copyright3));

	setup_selected = -1;////
	done = 0;

	do
	{
#ifdef UI_COLOR_DISPLAY
		Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = Machine->uifont->colortable[FONT_COLOR_BLANK];
#endif /* UI_COLOR_DISPLAY */
		erase_screen(bitmap);
		ui_drawbox(bitmap,0,0,uirotwidth,uirotheight);
#ifdef UI_COLOR_DISPLAY
		Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = back;
#endif /* UI_COLOR_DISPLAY */
		ui_displaymessagewindow(bitmap,buf);

		update_video_and_audio();
		if (input_ui_pressed(IPT_UI_CANCEL))
		{
			setup_selected = 0;////
			return 1;
		}
		if (code_pressed_memory(KEYCODE_O) ||
				input_ui_pressed(IPT_UI_LEFT))
			done = 1;
		if (done == 1 && (code_pressed_memory(KEYCODE_K) ||
				input_ui_pressed(IPT_UI_RIGHT)))
			done = 2;
	} while (done < 2);

	setup_selected = 0;////
	erase_screen(bitmap);
	update_video_and_audio();

	return 0;
}

static int displaygameinfo(struct mame_bitmap *bitmap,int selected)
{
	int res;
	int i;
	char buf[4096];
	char buf2[256];
	int sel;


	sel = selected - 1;


	sprintf(buf,"%s\n%s %s\n\n%s:\n",
            Machine->gamedrv->description,
		Machine->gamedrv->year,
            Machine->gamedrv->manufacturer,
		ui_getstring (UI_cpu));
	i = 0;
	while (i < MAX_CPU && Machine->drv->cpu[i].cpu_type)
	{

		if (Machine->drv->cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)],"%s %d.%06d MHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000000,
					Machine->drv->cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)],"%s %d.%03d kHz",
					cputype_name(Machine->drv->cpu[i].cpu_type),
					Machine->drv->cpu[i].cpu_clock / 1000,
					Machine->drv->cpu[i].cpu_clock % 1000);

		if (Machine->drv->cpu[i].cpu_flags & CPU_AUDIO_CPU)
		{
			sprintf (buf2, " (%s)", ui_getstring (UI_sound_lc));
			strcat(buf, buf2);
		}

		strcat(buf,"\n");

		i++;
	}

	sprintf (buf2, "\n%s", ui_getstring (UI_sound));
	strcat (buf, buf2);
	if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
		sprintf(&buf[strlen(buf)]," (%s)", ui_getstring (UI_stereo));
	strcat(buf,":\n");

	i = 0;
	while (i < MAX_SOUND && Machine->drv->sound[i].sound_type)
	{
		if (sound_num(&Machine->drv->sound[i]))
			sprintf(&buf[strlen(buf)],"%dx",sound_num(&Machine->drv->sound[i]));

		sprintf(&buf[strlen(buf)],"%s",sound_name(&Machine->drv->sound[i]));

		if (sound_clock(&Machine->drv->sound[i]))
		{
			if (sound_clock(&Machine->drv->sound[i]) >= 1000000)
				sprintf(&buf[strlen(buf)]," %d.%06d MHz",
						sound_clock(&Machine->drv->sound[i]) / 1000000,
						sound_clock(&Machine->drv->sound[i]) % 1000000);
			else
				sprintf(&buf[strlen(buf)]," %d.%03d kHz",
						sound_clock(&Machine->drv->sound[i]) / 1000,
						sound_clock(&Machine->drv->sound[i]) % 1000);
		}

		strcat(buf,"\n");

		i++;
	}

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		sprintf(&buf[strlen(buf)],"\n%s\n", ui_getstring (UI_vectorgame));
	else
	{
		sprintf(&buf[strlen(buf)],"\n%s:\n", ui_getstring (UI_screenres));
		sprintf(&buf[strlen(buf)],"%d x %d (%s) %f Hz\n",
				Machine->visible_area.max_x - Machine->visible_area.min_x + 1,
				Machine->visible_area.max_y - Machine->visible_area.min_y + 1,
				(Machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
				Machine->refresh_rate);
#if 0
		{
			int pixelx,pixely,tmax,tmin,rem;

			pixelx = 4 * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
			pixely = 3 * (Machine->visible_area.max_x - Machine->visible_area.min_x + 1);

			/* calculate MCD */
			if (pixelx >= pixely)
			{
				tmax = pixelx;
				tmin = pixely;
			}
			else
			{
				tmax = pixely;
				tmin = pixelx;
			}
			while ( (rem = tmax % tmin) )
			{
				tmax = tmin;
				tmin = rem;
			}
			/* tmin is now the MCD */

			pixelx /= tmin;
			pixely /= tmin;

			sprintf(&buf[strlen(buf)],"pixel aspect ratio %d:%d\n",
					pixelx,pixely);
		}
		sprintf(&buf[strlen(buf)],"%d colors ",Machine->drv->total_colors);
#endif
	}


	if (sel == -1)
	{
#ifdef UI_COLOR_DISPLAY
		int back = Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND];

		Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = Machine->uifont->colortable[FONT_COLOR_BLANK];
#endif /* UI_COLOR_DISPLAY */
		/* startup info, print MAME version and ask for any key */

		sprintf (buf2, "\n\t%s ", ui_getstring (UI_mame));	/* \t means that the line will be centered */
		strcat(buf, buf2);

		strcat(buf,build_version);
		sprintf (buf2, "\n\t%s", ui_getstring (UI_selectkey));
		strcat(buf,buf2);
		ui_drawbox(bitmap,0,0,uirotwidth,uirotheight);

#ifdef UI_COLOR_DISPLAY
		Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = back;
#endif /* UI_COLOR_DISPLAY */

		sel = 0;

		res = ui_displaymessagewindow(bitmap,buf);

		if (res >= 0)
		{
			if (res == 1 || input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

			if (res == 2 || input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

			if (res == 3 || input_ui_pressed(IPT_UI_CONFIGURE))
				sel = -2;
		}
	}
	else
	{
		/* menu system, use the normal menu keys */
		strcat(buf,"\n\t");
		strcat(buf,ui_getstring (UI_lefthilight));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_returntomain));
		strcat(buf," ");
		strcat(buf,ui_getstring (UI_righthilight));

		res = ui_displaymessagewindow(bitmap,buf);
		if (res >= 0)
		{
			if (res == 1 || input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

			if (res == 2 || input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

			if (res == 3 || input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}
	}

	if (sel == -1 || sel == -2)
	{
		scroll_reset = 1;
		schedule_full_refresh();
	}

	return sel + 1;
}


int showgamewarnings(struct mame_bitmap *bitmap)
{
	int i;
	char buf[4096];

	if (Machine->gamedrv->flags &
			(GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_WRONG_COLORS | GAME_IMPERFECT_COLORS |
			  GAME_NO_SOUND | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL))
	{
		int res;
#ifdef UI_COLOR_DISPLAY
		int back = Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND];
#endif /* UI_COLOR_DISPLAY */
		int done;

		strcpy(buf, ui_getstring (UI_knownproblems));
		strcat(buf, "\n\n");

#ifdef MESS
		if (Machine->gamedrv->flags & GAME_COMPUTER)
		{
			strcpy(buf, ui_getstring (UI_comp1));
			strcat(buf, "\n\n");
			strcat(buf, ui_getstring (UI_comp2));
			strcat(buf, "\n");
		}
#endif

		if (Machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
		{
			strcat(buf, ui_getstring (UI_imperfectcolors));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & GAME_WRONG_COLORS)
		{
			strcat(buf, ui_getstring (UI_wrongcolors));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & GAME_IMPERFECT_GRAPHICS)
		{
			strcat(buf, ui_getstring (UI_imperfectgraphics));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & GAME_IMPERFECT_SOUND)
		{
			strcat(buf, ui_getstring (UI_imperfectsound));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & GAME_NO_SOUND)
		{
			strcat(buf, ui_getstring (UI_nosound));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & GAME_NO_COCKTAIL)
		{
			strcat(buf, ui_getstring (UI_nococktail));
			strcat(buf, "\n");
		}

		if (Machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
		{
			const struct GameDriver *maindrv;
			int foundworking;

			if (Machine->gamedrv->flags & GAME_NOT_WORKING)
			{
				strcpy(buf, ui_getstring (UI_brokengame));
				strcat(buf, "\n");
			}
			if (Machine->gamedrv->flags & GAME_UNEMULATED_PROTECTION)
			{
				strcat(buf, ui_getstring (UI_brokenprotection));
				strcat(buf, "\n");
			}

			if (Machine->gamedrv->clone_of && !(Machine->gamedrv->clone_of->flags & NOT_A_DRIVER))
				maindrv = Machine->gamedrv->clone_of;
			else maindrv = Machine->gamedrv;

			foundworking = 0;
			i = 0;
			while (drivers[i])
			{
				if (drivers[i] == maindrv || drivers[i]->clone_of == maindrv)
				{
					if ((drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
					{
						if (foundworking == 0)
						{
							strcat(buf,"\n\n");
							strcat(buf, ui_getstring (UI_workingclones));
							strcat(buf,"\n\n");
						}
						foundworking = 1;

						sprintf(&buf[strlen(buf)],"%s\n",drivers[i]->name);
					}
				}
				i++;
			}
		}

		strcat(buf,"\n\n");
		strcat(buf,ui_getstring (UI_typeok));

		done = 0;
		res = 0;
		do
		{
#ifdef UI_COLOR_DISPLAY
			Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = Machine->uifont->colortable[FONT_COLOR_BLANK];
#endif /* UI_COLOR_DISPLAY */
			erase_screen(bitmap);
			ui_drawbox(bitmap,0,0,uirotwidth,uirotheight);
#ifdef UI_COLOR_DISPLAY
			Machine->uifont->colortable[SYSTEM_COLOR_BACKGROUND] = back;
#endif /* UI_COLOR_DISPLAY */
			res = ui_displaymessagewindow(bitmap,buf);
			update_video_and_audio();
			if (res == 2 || input_ui_pressed(IPT_UI_CANCEL)) // canceled
			{
				return 1;
			}
			else if (res >= 0)
			{
				if (code_pressed_memory(KEYCODE_O)
				||  input_ui_pressed(IPT_UI_LEFT))
				done = 1;
				if (done == 1 && (code_pressed_memory(KEYCODE_K)
				||  input_ui_pressed(IPT_UI_RIGHT)))
				done = 2;
			}
		} while (done < 2);

		scroll_reset = 1;
	}

	erase_screen(bitmap);
	update_video_and_audio();

	return 0;
}


int showgameinfo(struct mame_bitmap *bitmap)
{
	/* clear the input memory */
	while (code_read_async() != CODE_NONE) {};

	while (displaygameinfo(bitmap,0) == 1)
	{
		update_video_and_audio();
	}

	#ifdef MESS
	while (displayimageinfo(bitmap,0) == 1)
	{
		update_video_and_audio();
	}
	#endif

	/* clear the input memory */
	while (code_read_async() != CODE_NONE) {};

	erase_screen(bitmap);
	/* make sure that the screen is really cleared, in case autoframeskip kicked in */
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();

	return 0;
}

/* Word-wraps the text in the specified buffer to fit in maxwidth characters per line.
   The contents of the buffer are modified.
   Known limitations: Words longer than maxwidth cause the function to fail. */
static int wordwrap_text_buffer(char *buffer, int maxwidth)
{
	char *c, *c2;
	char textcopy[256 * 1024 + 256];
	unsigned short code;
	int i, increment, wrapped;
	int len, maxlen;
	char *wrap_c = NULL;
	char *wrap_c2 = NULL;
	int wrap_w;
	int wrap_len = 0;
	int last_is_space = 0;

	if (maxwidth <= 0)
		{
		*buffer = '\0';
		return 0;
		}

	/* copy text, calculate max len, wrap long lines to fit */
	len    = 0;
	maxlen = 0;
	wrapped = i = 0;
	c = buffer;
	c2 = textcopy;
	memset(textcopy, 0, sizeof textcopy);

	while (*c)
		{
		if (wrapped)
			{
			last_is_space = 0;
			wrap_w = 0;
			}
		else
			wrap_w = i;

		wrapped = 0;
		increment = uifont_decodechar((unsigned char *)c, &code);

		if (*c == '\n')
		{
			*c2++ = *c++;
			wrapped = 1;
		}
		else
		{
			i = (code < 256) ? 1 : 2;
			if (i > 1 || wrap_w > 1)
				wrap_c = NULL;

			if (wrap_c == NULL)
			{
				wrap_len = len;
				wrap_c = c;
				wrap_c2 = c2;
			}
			else if (len > 0 && *c == ' ')
	{
				if (!last_is_space)
		{
					wrap_c2 = c2;
					wrap_len = len;
				}
				wrap_c = c;
		}

			last_is_space = (*c == ' ');

			if (len + i > maxwidth)
		{
				if (!wrap_len || !wrap_c)
			{
					wrap_len = len;
					wrap_c = c;
					wrap_c2 = c2;
			}

				len = wrap_len;
				c = wrap_c;
				c2 = wrap_c2;

				while (*c == ' ')
					c++;

				*c2++ = '\n';
				wrapped = 1;
		}
		else
			{
				len += i;

				while (increment)
				{
					*c2++ = *c++;
					increment--;
	}
			}
		}

		if (wrapped)
		{
			if (len > maxlen)
				maxlen = len;

			len = 0;
			wrap_c = NULL;
			wrap_w = 0;
		}
	}

	if (len > maxlen)
		maxlen = len;

	*c2 = '\0';

	strcpy(buffer, textcopy);

	return maxlen;
}

static int count_lines_in_buffer (char *buffer)
{
	int lines = 0;
	char c;

	while ( (c = *buffer++) )
		if (c == '\n') lines++;

	return lines;
}

/* Display lines from buffer, starting with line 'scroll', in a width x height text window */
#ifdef CMD_LIST
static int display_scroll_message (struct mame_bitmap *bitmap, int *scroll, int width, int height, char *buf)
#else /* CMD_LIST */
static void display_scroll_message (struct mame_bitmap *bitmap, int *scroll, int width, int height, char *buf)
#endif /* CMD_LIST */
{
	struct DisplayText dt[256];
	int curr_dt = 0;
	const char *uparrow = ui_getstring (UI_uparrow);
	const char *downarrow = ui_getstring (UI_downarrow);
	char textcopy[8192];
	char *copy;
	int leftoffs,topoffs;
	int first = *scroll;
	int buflines,showlines;
	int i;
#ifdef CMD_LIST
	int ret;
#endif /* CMD_LIST */


	/* draw box */
	leftoffs = (uirotwidth - uirotcharwidth * (width + 1)) / 2;
	if (leftoffs < 0) leftoffs = 0;
	topoffs = (uirotheight - (height * ROWHEIGHT + ROWMARGIN)) / 2;
	ui_drawbox(bitmap,leftoffs,topoffs,(width + 1) * uirotcharwidth,height * ROWHEIGHT + ROWMARGIN);

	buflines = count_lines_in_buffer (buf);
	if (first > 0)
	{
		if (buflines <= height)
			first = 0;
		else
		{
			height--;
			if (first > (buflines - height))
				first = buflines - height;
		}
		*scroll = first;
	}

	if (first != 0)
	{
		/* indicate that scrolling upward is possible */
		dt[curr_dt].text = uparrow;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(uparrow)) / 2;
		dt[curr_dt].y = topoffs + curr_dt*ROWHEIGHT + ROWMARGIN;
		curr_dt++;
	}

	if ((buflines - first) > height)
		showlines = height - 1;
	else
		showlines = height;

#ifdef CMD_LIST
	ret = buflines;

	if (first)
		ret |= SCR_PREV_PAGE;

	if (showlines == (height - 1))
		ret |= SCR_NEXT_PAGE;
#endif /* CMD_LIST */

	/* skip to first line */
	while (first > 0)
	{
		char c;

		while ( (c = *buf++) )
		{
			if (c == '\n')
			{
				first--;
				break;
			}
		}
	}

	/* copy 'showlines' lines from buffer, starting with line 'first' */
	copy = textcopy;
	for (i = 0; i < showlines; i++)
	{
		char *copystart = copy;

		while (*buf && *buf != '\n')
		{
			*copy = *buf;
			copy++;
			buf++;
		}
		*copy = '\0';
		copy++;
		if (*buf == '\n')
			buf++;

		if (*copystart == '\t') /* center text */
		{
			copystart++;
			dt[curr_dt].x = (uirotwidth - uirotcharwidth * (copy - copystart)) / 2;
		}
		else
			dt[curr_dt].x = leftoffs + uirotcharwidth/2;

		dt[curr_dt].text = copystart;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].y = topoffs + curr_dt*ROWHEIGHT + ROWMARGIN;
		curr_dt++;
	}

	if (showlines == (height - 1))
	{
		/* indicate that scrolling downward is possible */
		dt[curr_dt].text = downarrow;
		dt[curr_dt].color = UI_COLOR_NORMAL;
		dt[curr_dt].x = (uirotwidth - uirotcharwidth * strlen(downarrow)) / 2;
		dt[curr_dt].y = topoffs + curr_dt*ROWHEIGHT + ROWMARGIN;
		curr_dt++;
	}

	dt[curr_dt].text = 0;	/* terminate array */

	displaytext(bitmap,dt);

#ifdef CMD_LIST
	return ret;
#endif /* CMD_LIST */
}


#ifdef MASH_DATAFILE
enum {
	DATAFILE_MAMEINFO,
	DATAFILE_DRIVINFO,
	DATAFILE_HISTORY
};


/* Display text entry for current driver from history.dat and mameinfo.dat. */
static int displaydatinfo (struct mame_bitmap *bitmap, int selected, int dattype)
#else /* MASH_DATAFILE */
static int displayhistory (struct mame_bitmap *bitmap, int selected)
#endif /* MASH_DATAFILE */
{
	static int scroll = 0;
	static int counter = 0;
	static int fast = 4;
	static char *buf = 0;
	int maxcols,maxrows;
	int sel;
	int bufsize = 256 * 1024; // 256KB of history.dat buffer, enough for everything

	sel = selected - 1;


	maxcols = (uirotwidth / uirotcharwidth) - 1;
	maxrows = (uirotheight / ROWHEIGHT) - 1;
	maxcols -= 1;
	maxrows -= 2;

	if (!buf)
	{
		/* allocate a buffer for the text */
		buf = malloc (bufsize);

		if (buf)
		{
			/* Disable sound to prevent strange sound*/
			osd_sound_enable(0);

			/* try to load entry */
#ifdef MASH_DATAFILE
			if ((dattype == DATAFILE_MAMEINFO   && (load_driver_mameinfo (Machine->gamedrv, buf, bufsize) == 0))
			||  (dattype == DATAFILE_DRIVINFO   && (load_driver_drivinfo (Machine->gamedrv, buf, bufsize) == 0))
			||  (dattype == DATAFILE_HISTORY    && (load_driver_history  (Machine->gamedrv, buf, bufsize) == 0)))
#else /* MASH_DATAFILE */
			if (load_driver_history (Machine->gamedrv, buf, bufsize) == 0)
#endif /* MASH_DATAFILE */
			{
				scroll = 0;
				wordwrap_text_buffer (buf, maxcols);
				strcat(buf,"\n\t");
				strcat(buf,ui_getstring (UI_lefthilight));
				strcat(buf," ");
				strcat(buf,ui_getstring (UI_returntoprior));
				strcat(buf," ");
				strcat(buf,ui_getstring (UI_righthilight));
				strcat(buf,"\n");
			}
			else
			{
				free (buf);
				buf = 0;
			}

			osd_sound_enable(1);

		}
	}

	{
		if (buf)
			display_scroll_message (bitmap, &scroll, maxcols, maxrows, buf);
		else
		{
			char msg[80];

			strcpy(msg,"\t");

			#ifndef MESS
#ifdef MASH_DATAFILE
			switch (dattype) {
			case DATAFILE_MAMEINFO:
				strcat(msg,ui_getstring(UI_mameinfomissing));
				break;
			case DATAFILE_DRIVINFO:
				strcat(msg,ui_getstring(UI_drivinfomissing));
				break;
			case DATAFILE_HISTORY:
				strcat(msg,ui_getstring(UI_historymissing));
				break;
			}
#else /* MASH_DATAFILE */
				strcat(msg,ui_getstring(UI_historymissing));
#endif /* MASH_DATAFILE */
			#else
			strcat(msg,ui_getstring(UI_historymissing));
			#endif

			strcat(msg,"\n\n\t");
			strcat(msg,ui_getstring (UI_lefthilight));
			strcat(msg," ");
			strcat(msg,ui_getstring (UI_returntoprior));
			strcat(msg," ");
			strcat(msg,ui_getstring (UI_righthilight));
			ui_displaymessagewindow(bitmap,msg);
		}

		if ((scroll > 0) && input_ui_pressed_repeat(IPT_UI_UP,fast))
		{
			if (scroll == 2) scroll = 0;	/* 1 would be the same as 0, but with arrow on top */
			else scroll--;
		}

		if (input_ui_pressed_repeat(IPT_UI_DOWN,fast))
		{
			if (scroll == 0) scroll = 2;	/* 1 would be the same as 0, but with arrow on top */
			else scroll++;
		}

		if (input_ui_pressed_repeat(IPT_UI_PAN_UP, fast))
		{
			scroll -= maxrows - 2;
			if (scroll < 0) scroll = 0;
		}

		if (input_ui_pressed_repeat(IPT_UI_PAN_DOWN, fast))
		{
			scroll += maxrows - 2;
		}
//REMOVED FOR MAME0.84u1
/*
		if (seq_pressed(input_port_type_seq(IPT_UI_UP)) | seq_pressed(input_port_type_seq(IPT_UI_DOWN)))
		{
			if (++counter == 25)
			{
				fast--;
				if (fast < 1) fast = 0;
				counter = 0;
			}
		}
		else fast = 4;
*/
		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}

	if (sel == -1 || sel == -2)
		{
		schedule_full_refresh();

		/* force buffer to be recreated */
		if (buf)
		{
			free (buf);
			buf = 0;
		}
	}

	return sel + 1;

}


#ifdef CMD_LIST
static void flash_all_values_and_buffers(void)
{
	setup_selected    = 0;
	osd_selected      = 0;
	single_step       = 0;

	scroll_reset      = 1;

	if (commandlist_buf)
	{
		free(commandlist_buf);
		commandlist_buf = NULL;
		}
	command_scroll = 0;
	command_lastselected = 0;

	command_sc = 0;
	command_sel = 1;
	command_load = 1;
}

/* Display text entry for current driver from command.dat. */
static int displaycommand(struct mame_bitmap *bitmap, int selected, int shortcut)
{
	int sel, ret = 0, buflines = 0, visible = 0;
	static int maxcols = 0, maxrows = 0;

	sel = selected - 1;

	if (!commandlist_buf)
		{
		maxcols = (uirotwidth / uirotcharwidth) - 2;
		maxrows = (uirotheight / ROWHEIGHT) - 2;

		/* allocate a buffer for the text */
		if ((commandlist_buf = malloc(65536)))
		{
			memset(commandlist_buf, 0, 65536);

			/* try to load entry */
			if (!load_driver_command_ex (Machine->gamedrv, commandlist_buf, 65536, sel))
			{
				int height = 1;
				static int presel = -1;
				char *c;

				if( presel != sel )
				{
					command_scroll = 0;
					presel = sel;
				}

				convert_command_move(commandlist_buf);

				strcat(commandlist_buf,"\n\t");
				strcat(commandlist_buf,ui_getstring(UI_lefthilight));
				strcat(commandlist_buf," ");
				strcat(commandlist_buf,ui_getstring(UI_returntoprior));
				strcat(commandlist_buf," ");
				strcat(commandlist_buf,ui_getstring(UI_righthilight));
				strcat(commandlist_buf,"\n");

				c = commandlist_buf;

				while(*c)
					if (*c++ == '\n') height++;

				if (height <= maxrows)
					maxrows = height - 1;
			}
			else
			{
				free(commandlist_buf);
				commandlist_buf = 0;
			}
		}
	}

	if (commandlist_buf)
	{
		ret = display_scroll_message(bitmap, &command_scroll, maxcols, maxrows, commandlist_buf);

		buflines = ret & SCR_PAGE_MASK;
		visible  = maxrows - ((ret & SCR_PREV_PAGE) != 0) - ((ret & SCR_NEXT_PAGE) != 0);
	}
	else
	{
		char msg[80];

		strcpy(msg, "\t");
		strcat(msg, ui_getstring(UI_commandmissing));
		strcat(msg, "\n\n\t");
		strcat(msg, ui_getstring(UI_lefthilight));
		strcat(msg, " ");
		if (shortcut)
			strcat(msg, ui_getstring(UI_returntogame));
		else
			strcat(msg, ui_getstring(UI_returntoprior));
		strcat(msg, " ");
		strcat(msg, ui_getstring(UI_righthilight));
		ui_displaymessagewindow(bitmap, msg);
	}

	if ((command_scroll > 0) && input_ui_pressed_repeat(IPT_UI_UP,4))
	{
		if (command_scroll == 2) command_scroll = 0;	/* 1 would be the same as 0, but with arrow on top */
		else command_scroll--;
	}

	if (input_ui_pressed_repeat(IPT_UI_DOWN,4))
	{
		if (command_scroll == 0) command_scroll = 2;	/* 1 would be the same as 0, but with arrow on top */
		else command_scroll++;
	}

	if ((ret & SCR_PREV_PAGE) && input_ui_pressed_repeat(IPT_UI_LEFT,4))
	{
		if (command_scroll == maxrows - 1)
			command_scroll = 0;
		else if (command_scroll - visible < 0)
			command_scroll = 0;
		else
			command_scroll -= visible;
	}

	if ((ret & SCR_NEXT_PAGE) && input_ui_pressed_repeat(IPT_UI_RIGHT,4))
	{
		if (command_scroll >= buflines - visible)
			command_scroll = buflines - visible;
		else
			command_scroll += visible;
		}

		if (input_ui_pressed(IPT_UI_SELECT))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE) && shortcut == 0)
			sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();

		/* force buffer to be recreated */
		if (commandlist_buf)
		{
			free(commandlist_buf);
			commandlist_buf = 0;
		}
		}

	return sel + 1;
}


static int displaycommand_ex(struct mame_bitmap *bitmap, int selected, int shortcut)
{
	static const char *menu_item[64];
	static int total = 0;
	int sel, visible;

	sel = selected - 1;
	visible = (uirotheight / ROWHEIGHT) - 3;

	/* If a submenu has been selected, go there */
	if (command_lastselected)
	{
		command_lastselected = displaycommand(bitmap, (sel + 1), shortcut);

		if (command_lastselected == -1)
		{
			sel = -2;
			command_lastselected = 1;
			schedule_full_refresh();
	}

	return sel + 1;
	}

	/* No submenu active, display the main command index menu */
	if (total == -1)
	{
		char msg[80];

		strcpy(msg, "\t");
		strcat(msg, ui_getstring(UI_commandmissing));
		strcat(msg, "\n\n\t");
		strcat(msg, ui_getstring(UI_lefthilight));
		strcat(msg, " ");
		if (shortcut)
			strcat(msg, ui_getstring(UI_returntogame));
		else
			strcat(msg, ui_getstring(UI_returntoprior));
		strcat(msg, " ");
		strcat(msg, ui_getstring(UI_righthilight));
		ui_displaymessagewindow(bitmap, msg);
	}
	else
		{
		if (total == 0 || command_load)
			{
			total = command_sub_menu(Machine->gamedrv, menu_item);
			command_load = 0;

			if (total > 0)
			{
				if (shortcut)
					menu_item[total++] = ui_getstring(UI_returntogame);
				else
					menu_item[total++] = ui_getstring(UI_returntoprior);
				menu_item[total] = 0;
			}
			else
			{
				char msg[80];

				total = -1;
				strcpy(msg, "\t");
				strcat(msg, ui_getstring(UI_commandmissing));
				strcat(msg, "\n\n\t");
				strcat(msg, ui_getstring(UI_lefthilight));
				strcat(msg, " ");
				if (shortcut)
					strcat(msg, ui_getstring(UI_returntogame));
				else
					strcat(msg, ui_getstring(UI_returntoprior));
				strcat(msg, " ");
				strcat(msg, ui_getstring(UI_righthilight));
				ui_displaymessagewindow(bitmap, msg);
		}
	}
		else
		{
			if (shortcut)
				menu_item[total - 1] = ui_getstring(UI_returntogame);
			else
				menu_item[total - 1] = ui_getstring(UI_returntoprior);

			ui_displaymenu(bitmap,menu_item,0,0,sel,0);
		}

		if (total > 0)
		{
			if (input_ui_pressed_repeat(IPT_UI_DOWN,4))
				sel = (sel + 1) % total;

			if (input_ui_pressed_repeat(IPT_UI_UP,4))
				sel = (sel + total - 1) % total;

			if (input_ui_pressed_repeat(IPT_UI_LEFT,4))
			{
				sel -= visible;
				if (sel < 0)	sel = 0;
		}

			if (input_ui_pressed_repeat(IPT_UI_RIGHT,4))
		{
				sel += visible;
				if (sel >= total)	sel = total - 1;
			}
		}
		}

		if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == (total - 1) || total == -1) // return to main
		{
			command_lastselected = 0;
			sel = -1;
		}
		else
		{
			command_lastselected = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

	/* The UI key takes us all the way back out */
	if (input_ui_pressed(IPT_UI_CONFIGURE) && shortcut == 0)
			sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
		total = 0;
	}

	return sel + 1;
}
#endif /* CMD_LIST */


#ifndef MESS
#ifndef TINY_COMPILE
#ifndef MMSND
int memcard_menu(struct mame_bitmap *bitmap, int selection)
{
	int sel;
	int menutotal = 0;
	const char *menuitem[10];
	char buf[256];
	char buf2[256];

	sel = selection - 1 ;

	sprintf(buf, "%s %03d", ui_getstring (UI_loadcard), mcd_number);
	menuitem[menutotal++] = buf;
	menuitem[menutotal++] = ui_getstring (UI_ejectcard);
	menuitem[menutotal++] = ui_getstring (UI_createcard);
#ifdef MESS
	menuitem[menutotal++] = ui_getstring (UI_resetcard);
#endif
	menuitem[menutotal++] = ui_getstring (UI_returntomain);
	menuitem[menutotal] = 0;

	if (mcd_action!=0)
	{
		strcpy (buf2, "\n");

		switch(mcd_action)
		{
			case 1:
				strcat (buf2, ui_getstring (UI_loadfailed));
				break;
			case 2:
				strcat (buf2, ui_getstring (UI_loadok));
				break;
			case 3:
				strcat (buf2, ui_getstring (UI_cardejected));
				break;
			case 4:
				strcat (buf2, ui_getstring (UI_cardcreated));
				break;
			case 5:
				strcat (buf2, ui_getstring (UI_cardcreatedfailed));
				strcat (buf2, "\n");
				strcat (buf2, ui_getstring (UI_cardcreatedfailed2));
				break;
			default:
				strcat (buf2, ui_getstring (UI_carderror));
				break;
		}

		strcat (buf2, "\n\n");
		ui_displaymessagewindow(bitmap,buf2);
		if (input_ui_pressed(IPT_UI_SELECT))
			mcd_action = 0;
	}
	else
	{
		ui_displaymenu(bitmap,menuitem,0,0,sel,0);

		if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
			mcd_number = (mcd_number + 1) % 1000;

		if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
			mcd_number = (mcd_number + 999) % 1000;

		if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
			sel = (sel + 1) % menutotal;

		if (input_ui_pressed_repeat(IPT_UI_UP,8))
			sel = (sel + menutotal - 1) % menutotal;

		if (input_ui_pressed(IPT_UI_SELECT))
		{
			switch(sel)
			{
			case 0:
				neogeo_memcard_eject();
				if (neogeo_memcard_load(mcd_number))
				{
					memcard_status=1;
					memcard_number=mcd_number;
					mcd_action = 2;
				}
				else
					mcd_action = 1;
				break;
			case 1:
				neogeo_memcard_eject();
				mcd_action = 3;
				break;
			case 2:
				if (neogeo_memcard_create(mcd_number))
					mcd_action = 4;
				else
					mcd_action = 5;
				break;
#ifdef MESS
			case 3:
				memcard_manager=1;
				sel=-2;
				machine_reset();
				break;
			case 4:
				sel=-1;
				break;
#else
			case 3:
				sel=-1;
				break;
#endif


			}
		}

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;

		if (sel == -1 || sel == -2)
		{
			schedule_full_refresh();
		}
	}

	return sel + 1;
}
#endif
#endif
#endif


static int document_menu(struct mame_bitmap *bitmap, int selected)
{
	enum {
#ifndef MESS
#ifdef MASH_DATAFILE
		UI_MAMEINFO, UI_DRIVINFO,
#endif /* MASH_DATAFILE */
#else
		UI_IMAGEINFO,
#endif
#ifdef CMD_LIST
		UI_COMMAND,
#endif /* CMD_LIST */
		UI_HISTORY, UI_RETURNTOMAIN
	};

	const char *menuitem[10];
	int menuaction[10];
	int menutotal = 0;
	int sel,res=-1;
	static int menu_lastselected = 0;

	if (selected == -1)
		sel = menu_lastselected;
	else sel = selected - 1;

#ifndef MESS
	menuitem[menutotal] = ui_getstring (UI_history); menuaction[menutotal++] = UI_HISTORY;
#ifdef MASH_DATAFILE
	menuitem[menutotal] = ui_getstring (UI_mameinfo); menuaction[menutotal++] = UI_MAMEINFO;
	menuitem[menutotal] = ui_getstring (UI_drivinfo); menuaction[menutotal++] = UI_DRIVINFO;
#endif /* MASH_DATAFILE */
#ifdef CMD_LIST
	menuitem[menutotal] = ui_getstring (UI_command); menuaction[menutotal++] = UI_COMMAND;
#endif /* CMD_LIST */
#else
	menuitem[menutotal] = ui_getstring (UI_imageinfo); menuaction[menutotal++] = UI_IMAGEINFO;
	menuitem[menutotal] = ui_getstring (UI_history); menuaction[menutotal++] = UI_HISTORY;
#endif
	menuitem[menutotal] = ui_getstring(UI_returntomain); menuaction[menutotal++] = UI_RETURNTOMAIN;
	menuitem[menutotal] = 0; /* terminate array */

	if (sel > SEL_MASK)
	{
		switch (menuaction[sel & SEL_MASK])
		{
			case UI_HISTORY:
#ifdef MASH_DATAFILE
				res = displaydatinfo(bitmap, sel >> SEL_BITS, DATAFILE_HISTORY);
#else /* MASH_DATAFILE */
				res = displayhistory(bitmap, sel >> SEL_BITS);
#endif /* MASH_DATAFILE */
				break;
#ifndef MESS
#ifdef MASH_DATAFILE
			case UI_MAMEINFO:
				res = displaydatinfo(bitmap, sel >> SEL_BITS, DATAFILE_MAMEINFO);
				break;
			case UI_DRIVINFO:
				res = displaydatinfo(bitmap, sel >> SEL_BITS, DATAFILE_DRIVINFO);
				break;
#endif /* MASH_DATAFILE */
#else
			case UI_IMAGEINFO:
				res = displayimageinfo(bitmap, sel >> SEL_BITS);
				break;
#endif
#ifdef CMD_LIST
			case UI_COMMAND:
				res = displaycommand_ex(bitmap, sel >> SEL_BITS, 0);
				break;
#endif /* CMD_LIST */
		}

		if (res == -1)
		{
			menu_lastselected = sel;
			sel = -2;
		}
		else
			sel = (sel & SEL_MASK) | (res << SEL_BITS);

		return sel + 1;
	}

	ui_displaymenu(bitmap,menuitem,0,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % menutotal;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + menutotal - 1) % menutotal;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		switch (menuaction[sel])
		{
			#ifndef MESS
#ifdef MASH_DATAFILE
			case UI_MAMEINFO:
			case UI_DRIVINFO:
#endif /* MASH_DATAFILE */
			#else
			case UI_IMAGEINFO:
			#endif
			case UI_HISTORY:
#ifdef CMD_LIST
			case UI_COMMAND:
#endif /* CMD_LIST */
				sel |= 1 << SEL_BITS;
				schedule_full_refresh();
				break;

			default:
				if (sel == menutotal - 1)
				{
					menu_lastselected = 0;
					sel = -1;
				}
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
	{
		menu_lastselected = 0;
		sel = -1;
	}

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


enum { UI_SWITCH = 0,UI_DEFCODE,UI_CODE,
		UI_ANALOG,UI_CALIBRATE,
#ifndef MESS
		UI_STATS,UI_GAMEINFO,
#else
		UI_GAMEINFO,UI_FILEMANAGER,UI_TAPECONTROL,
#endif
		UI_GAMEDOCS,UI_CHEAT,
/*start MAME:analog+*/
		UI_MOUSECNTL,UI_MOUSEAXESCNTL,
/*end MAME:analog+  */
		UI_RESET,UI_MEMCARD,
		UI_EXIT };


#define MAX_SETUPMENU_ITEMS 20
static const char *menu_item[MAX_SETUPMENU_ITEMS];
static int menu_action[MAX_SETUPMENU_ITEMS];
static int menu_total;

static void append_menu(int uistring, int action)
{
	menu_item[menu_total] = ui_getstring(uistring);
	menu_action[menu_total++] = action;
}


static int has_dipswitches(void)
{
	struct InputPort *in;
	int num;

	/* Determine if there are any dip switches */
	num = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (in->type == IPT_DIPSWITCH_NAME && input_port_active(in))
			num++;
	}
	return num > 0;
}


static int has_analog(void)
{
	struct InputPort *in;
	int num;

	/* Determine if there are any analog controls */
	num = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
 	{
		if (in->type > IPT_ANALOG_START && in->type < IPT_ANALOG_END && input_port_active(in))
			num++;
/*start MAME:analog+*/
		/* Determine if there are any mice controls */
		if (osd_numbermice() && num)
		{
			if (switchaxes)
			{
				menu_item[menu_total] = ui_getstring (UI_mouseaxescontrols); menu_action[menu_total++] = UI_MOUSEAXESCNTL;
			}
			else if (switchmice)
			{
				menu_item[menu_total] = ui_getstring (UI_mousecontrols); menu_action[menu_total++] = UI_MOUSECNTL;
			}
		}
/*end MAME:analog+  */
	}
	return num > 0;
}


#ifdef MESS
static int has_configurables(void)
{
	struct InputPort *in;
	int num;

	num = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (in->type == IPT_CONFIG_NAME && input_port_active(in))
			num++;
	}
	return num > 0;
}


static int has_categories(void)
{
	struct InputPort *in;
	int num;

	num = 0;
	for (in = Machine->input_ports; in->type != IPT_END; in++)
	{
		if (in->category > 0 && input_port_active(in))
			num++;
	}
	return num > 0;
}
#endif /* MESS */


static void setup_menu_init(void)
{
	menu_total = 0;

	append_menu(UI_inputgeneral, UI_DEFCODE);
	append_menu(UI_inputspecific, UI_CODE);

	if (has_dipswitches())
		append_menu(UI_dipswitches, UI_SWITCH);

#ifdef MESS
	if (has_configurables())
		append_menu(UI_configuration, UI_CONFIGURATION);
	if (has_categories())
		append_menu(UI_categories, UI_CATEGORIES);
#endif /* MESS */


	if (has_analog())
		append_menu(UI_analogcontrols, UI_ANALOG);
  
  	/* Joystick calibration possible? */
  	if ((osd_joystick_needs_calibration()) != 0)
	{
		append_menu(UI_calibrate, UI_CALIBRATE);
	}

#ifndef MESS
	append_menu(UI_bookkeeping, UI_STATS);
	append_menu(UI_gameinfo, UI_GAMEINFO);
//	append_menu(UI_history, UI_HISTORY);
#else /* MESS */
	append_menu(UI_imageinfo, UI_IMAGEINFO);
	append_menu(UI_filemanager, UI_FILEMANAGER);
#if HAS_WAVE
	append_menu(UI_tapecontrol, UI_TAPECONTROL);
#endif /* HAS_WAVE */
	append_menu(UI_history, UI_HISTORY);
#endif /* !MESS */

	if (options.cheat)
	{
		append_menu(UI_cheat, UI_CHEAT);
	}

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef MMSND
	if (Machine->gamedrv->clone_of == &driver_neogeo ||
			(Machine->gamedrv->clone_of &&
				Machine->gamedrv->clone_of->clone_of == &driver_neogeo))
	{
		append_menu(UI_memorycard, UI_MEMCARD);
	}
#endif
#endif
#endif

	append_menu(UI_resetgame, UI_RESET);
	append_menu(UI_returntogame, UI_EXIT);
	menu_item[menu_total] = 0; /* terminate array */
}


static int setup_menu(struct mame_bitmap *bitmap, int selected)
{
	int sel,res=-1;
	static int menu_lastselected = 0;


	if (selected == -1)
		sel = menu_lastselected;
	else sel = selected - 1;

	if (sel > SEL_MASK)
	{
		switch (menu_action[sel & SEL_MASK])
		{
			case UI_SWITCH:
				res = setdipswitches(bitmap, sel >> SEL_BITS);
				break;
			case UI_DEFCODE:
				res = setdefcodesettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_CODE:
				res = setcodesettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_ANALOG:
				res = settraksettings(bitmap, sel >> SEL_BITS);
				break;
/*start MAME:analog+*/
			case UI_MOUSECNTL:
				res = setplayermousesettings(bitmap, sel >> SEL_BITS);
				break;
			case UI_MOUSEAXESCNTL:
				res = setplayermouseaxessettings(bitmap, sel >> SEL_BITS);
				break;
/*end MAME:analog+  */
			case UI_CALIBRATE:
				res = calibratejoysticks(bitmap, sel >> SEL_BITS);
				break;
#ifndef MESS
			case UI_STATS:
				res = mame_stats(bitmap, sel >> SEL_BITS);
				break;
			case UI_GAMEINFO:
				res = displaygameinfo(bitmap, sel >> SEL_BITS);
				break;
#endif
#ifdef MESS
			case UI_IMAGEINFO:
				res = displayimageinfo(bitmap, sel >> SEL_BITS);
				break;
			case UI_FILEMANAGER:
				res = filemanager(bitmap, sel >> SEL_BITS);
				break;
#if HAS_WAVE
			case UI_TAPECONTROL:
				res = tapecontrol(bitmap, sel >> SEL_BITS);
				break;
#endif /* HAS_WAVE */
			case UI_CONFIGURATION:
				res = setconfiguration(bitmap, sel >> SEL_BITS);
				break;
			case UI_CATEGORIES:
				res = setcategories(bitmap, sel >> SEL_BITS);
				break;
#endif /* MESS */
			case UI_GAMEDOCS:
				res = document_menu(bitmap, sel >> SEL_BITS);
				break;
			case UI_CHEAT:
				res = cheat_menu(bitmap, sel >> SEL_BITS);
				break;
#ifndef MESS
#ifndef TINY_COMPILE
#ifndef MMSND
			case UI_MEMCARD:
				res = memcard_menu(bitmap, sel >> SEL_BITS);
				break;
#endif
#endif
#endif
		}

		if (res == -1)
		{
			menu_lastselected = sel;
			sel = -1;
		}
		else
			sel = (sel & SEL_MASK) | (res << SEL_BITS);

/*start MAME:analog+*/
		init_analog_seq();
/*end MAME:analog+  */

		return sel + 1;
	}


	ui_displaymenu(bitmap,menu_item,0,0,sel,0);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % menu_total;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + menu_total - 1) % menu_total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		switch (menu_action[sel])
		{
			case UI_SWITCH:
			case UI_DEFCODE:
			case UI_CODE:
			case UI_ANALOG:
			case UI_CALIBRATE:
			#ifndef MESS
/*start MAME:analog+*/
			case UI_MOUSECNTL:
			case UI_MOUSEAXESCNTL:
/*end MAME:analog+  */
			case UI_STATS:
			case UI_GAMEINFO:
#else
			case UI_GAMEINFO:
			case UI_FILEMANAGER:
			case UI_TAPECONTROL:
			case UI_CONFIGURATION:
			case UI_CATEGORIES:
#endif /* !MESS */
			case UI_GAMEDOCS:
			case UI_CHEAT:
			case UI_MEMCARD:
				sel |= 1 << SEL_BITS;
				schedule_full_refresh();
				break;

			case UI_RESET:
				machine_reset();
				break;

			case UI_EXIT:
				menu_lastselected = 0;
				sel = -1;
				break;
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL) ||
			input_ui_pressed(IPT_UI_CONFIGURE))
	{
		menu_lastselected = sel;
		sel = -1;
	}

	if (sel == -1)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}



/*********************************************************************

  start of On Screen Display handling

*********************************************************************/

static void displayosd(struct mame_bitmap *bitmap,const char *text,int percentage,int default_percentage)
{
	struct DisplayText dt[2];
	int avail;


	avail = (uirotwidth / uirotcharwidth) * 19 / 20;

	ui_drawbox(bitmap,(uirotwidth - uirotcharwidth * avail) / 2,
			(uirotheight - 7*uirotcharheight/2),
			avail * uirotcharwidth,
			3*uirotcharheight);

	avail--;

	drawbar(bitmap,(uirotwidth - uirotcharwidth * avail) / 2,
			(uirotheight - 3*uirotcharheight),
			avail * uirotcharwidth,
			uirotcharheight,
			percentage,default_percentage);

	dt[0].text = text;
	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = (uirotwidth - uirotcharwidth * strlen(text)) / 2;
	dt[0].y = (uirotheight - 2*uirotcharheight) + 2;
	dt[1].text = 0; /* terminate array */
	displaytext(bitmap,dt);
}

static void onscrd_adjuster(struct mame_bitmap *bitmap,int increment,int arg)
{
	struct InputPort *in = &Machine->input_ports[arg];
	char buf[80];
	int value;

	if (increment)
	{
		value = in->default_value & 0xff;
		value += increment;
		if (value > 100) value = 100;
		if (value < 0) value = 0;
		in->default_value = (in->default_value & ~0xff) | value;
	}
	value = in->default_value & 0xff;

	sprintf(buf,"%s %d%%",in->name,value);

	displayosd(bitmap,buf,value,in->default_value >> 8);
}

static void onscrd_volume(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[20];
	int attenuation;

	if (increment)
	{
		attenuation = osd_get_mastervolume();
		attenuation += increment;
		if (attenuation > 0) attenuation = 0;
		if (attenuation < -32) attenuation = -32;
		osd_set_mastervolume(attenuation);
	}
	attenuation = osd_get_mastervolume();

	sprintf(buf,"%s %3ddB", ui_getstring (UI_volume), attenuation);
	displayosd(bitmap,buf,100 * (attenuation + 32) / 32,100);
}

static void onscrd_mixervol(struct mame_bitmap *bitmap,int increment,int arg)
{
	static void *driver = 0;
	char buf[40];
	int volume,ch;
	int doallchannels = 0;
	int proportional = 0;


	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		doallchannels = 1;
	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;
	if (code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_RALT))
		proportional = 1;

	if (increment)
	{
		if (proportional)
		{
			static int old_vol[MIXER_MAX_CHANNELS];
			float ratio = 1.0;
			int overflow = 0;

			if (driver != Machine->drv)
			{
				driver = (void *)Machine->drv;
				for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
					old_vol[ch] = mixer_get_mixing_level(ch);
			}

			volume = mixer_get_mixing_level(arg);
			if (old_vol[arg])
				ratio = (float)(volume + increment) / (float)old_vol[arg];

			for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
			{
				if (mixer_get_name(ch) != 0)
				{
					volume = ratio * old_vol[ch];
					if (volume < 0 || volume > 100)
						overflow = 1;
				}
			}

			if (!overflow)
			{
				for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
				{
					volume = ratio * old_vol[ch];
					mixer_set_mixing_level(ch,volume);
				}
			}
		}
		else
		{
			driver = 0; /* force reset of saved volumes */

			volume = mixer_get_mixing_level(arg);
			volume += increment;
			if (volume > 100) volume = 100;
			if (volume < 0) volume = 0;

			if (doallchannels)
			{
				for (ch = 0;ch < MIXER_MAX_CHANNELS;ch++)
					mixer_set_mixing_level(ch,volume);
			}
			else
				mixer_set_mixing_level(arg,volume);
		}
	}
	volume = mixer_get_mixing_level(arg);

	if (proportional)
		sprintf(buf,"%s %s %3d%%", ui_getstring (UI_allchannels), ui_getstring (UI_relative), volume);
	else if (doallchannels)
		sprintf(buf,"%s %s %3d%%", ui_getstring (UI_allchannels), ui_getstring (UI_volume), volume);
	else
		sprintf(buf,"%s %s %3d%%",mixer_get_name(arg), ui_getstring (UI_volume), volume);
	displayosd(bitmap,buf,volume,mixer_get_default_mixing_level(arg));
}

static void onscrd_brightness(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[20];
	double brightness;


	if (increment)
	{
		brightness = palette_get_global_brightness();
		brightness += 0.05 * increment;
		if (brightness < 0.1) brightness = 0.1;
		if (brightness > 1.0) brightness = 1.0;
		palette_set_global_brightness(brightness);
	}
	brightness = palette_get_global_brightness();

	sprintf(buf,"%s %3d%%", ui_getstring (UI_brightness), (int)(brightness * 100));
	displayosd(bitmap,buf,brightness*100,100);
}

static void onscrd_gamma(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[20];
	double gamma_correction;

	if (increment)
	{
		gamma_correction = palette_get_global_gamma();

		gamma_correction += 0.05 * increment;
		if (gamma_correction < 0.5) gamma_correction = 0.5;
		if (gamma_correction > 2.0) gamma_correction = 2.0;

		palette_set_global_gamma(gamma_correction);
	}
	gamma_correction = palette_get_global_gamma();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_gamma), gamma_correction);
	displayosd(bitmap,buf,100*(gamma_correction-0.5)/(2.0-0.5),100*(1.0-0.5)/(2.0-0.5));
}

static void onscrd_vector_flicker(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[1000];
	float flicker_correction;

	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;

	if (increment)
	{
		flicker_correction = vector_get_flicker();

		flicker_correction += increment;
		if (flicker_correction < 0.0) flicker_correction = 0.0;
		if (flicker_correction > 100.0) flicker_correction = 100.0;

		vector_set_flicker(flicker_correction);
	}
	flicker_correction = vector_get_flicker();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_vectorflicker), flicker_correction);
	displayosd(bitmap,buf,flicker_correction,0);
}

static void onscrd_vector_intensity(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[30];
	float intensity_correction;

	if (increment)
	{
		intensity_correction = vector_get_intensity();

		intensity_correction += 0.05 * increment;
		if (intensity_correction < 0.5) intensity_correction = 0.5;
		if (intensity_correction > 3.0) intensity_correction = 3.0;

		vector_set_intensity(intensity_correction);
	}
	intensity_correction = vector_get_intensity();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_vectorintensity), intensity_correction);
	displayosd(bitmap,buf,100*(intensity_correction-0.5)/(3.0-0.5),100*(1.5-0.5)/(3.0-0.5));
}


static void onscrd_overclock(struct mame_bitmap *bitmap,int increment,int arg)
{
	char buf[30];
	double overclock;
	int cpu, doallcpus = 0, oc;

	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		doallcpus = 1;
	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;
	if( increment )
	{
		overclock = cpunum_get_clockscale(arg);
		overclock += 0.01 * increment;
		if (overclock < 0.01) overclock = 0.01;
		if (overclock > 2.0) overclock = 2.0;
		if( doallcpus )
			for( cpu = 0; cpu < cpu_gettotalcpu(); cpu++ )
				cpunum_set_clockscale(cpu, overclock);
		else
			cpunum_set_clockscale(arg, overclock);
	}

	oc = 100 * cpunum_get_clockscale(arg) + 0.5;

	if( doallcpus )
		sprintf(buf,"%s %s %3d%%", ui_getstring (UI_allcpus), ui_getstring (UI_overclock), oc);
	else
		sprintf(buf,"%s %s%d %3d%%", ui_getstring (UI_overclock), ui_getstring (UI_cpu), arg, oc);
	displayosd(bitmap,buf,oc/2,100/2);
}

#define MAX_OSD_ITEMS 30
static void (*onscrd_fnc[MAX_OSD_ITEMS])(struct mame_bitmap *bitmap,int increment,int arg);
static int onscrd_arg[MAX_OSD_ITEMS];
static int onscrd_total_items;

static void onscrd_init(void)
{
	struct InputPort *in;
	int item,ch;

	item = 0;

	if (Machine->sample_rate)
	{
		onscrd_fnc[item] = onscrd_volume;
		onscrd_arg[item] = 0;
		item++;

		for (ch = 0;ch < MIXER_MAX_CHANNELS;ch++)
		{
			if (mixer_get_name(ch) != 0)
			{
				onscrd_fnc[item] = onscrd_mixervol;
				onscrd_arg[item] = ch;
				item++;
			}
		}
	}

	for (in = Machine->input_ports; in && in->type != IPT_END; in++)
		if ((in->type & 0xff) == IPT_ADJUSTER)
		{
			onscrd_fnc[item] = onscrd_adjuster;
			onscrd_arg[item] = in - Machine->input_ports;
			item++;
		}

	if (options.cheat)
	{
		for (ch = 0;ch < cpu_gettotalcpu();ch++)
		{
			onscrd_fnc[item] = onscrd_overclock;
			onscrd_arg[item] = ch;
			item++;
		}
	}

	onscrd_fnc[item] = onscrd_brightness;
	onscrd_arg[item] = 0;
	item++;

	onscrd_fnc[item] = onscrd_gamma;
	onscrd_arg[item] = 0;
	item++;

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		onscrd_fnc[item] = onscrd_vector_flicker;
		onscrd_arg[item] = 0;
		item++;

		onscrd_fnc[item] = onscrd_vector_intensity;
		onscrd_arg[item] = 0;
		item++;
	}

	onscrd_total_items = item;
}

static int on_screen_display(struct mame_bitmap *bitmap, int selected)
{
	int increment,sel;
	static int lastselected = 0;


	if (selected == -1)
		sel = lastselected;
	else sel = selected - 1;

	increment = 0;
	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
		increment = -1;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
		increment = 1;
	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % onscrd_total_items;
	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + onscrd_total_items - 1) % onscrd_total_items;

	(*onscrd_fnc[sel])(bitmap,increment,onscrd_arg[sel]);

	lastselected = sel;

	if (input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
	{
		sel = -1;

		schedule_full_refresh();
	}

	return sel + 1;
}

/*********************************************************************

  end of On Screen Display handling

*********************************************************************/


static void displaymessage(struct mame_bitmap *bitmap,const char *text)
{
	struct DisplayText dt[2];
	int avail;


	if (uirotwidth < uirotcharwidth * strlen(text))
	{
		ui_displaymessagewindow(bitmap,text);
		return;
	}

	avail = strlen(text)+2;

	ui_drawbox(bitmap,(uirotwidth - uirotcharwidth * avail) / 2,
			uirotheight - 3*uirotcharheight,
			avail * uirotcharwidth,
			2*uirotcharheight);

	dt[0].text = text;
	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = (uirotwidth - uirotcharwidth * strlen(text)) / 2;
	dt[0].y = uirotheight - 5*uirotcharheight/2;
	dt[1].text = 0; /* terminate array */
	displaytext(bitmap,dt);
}


static char messagetext[200];
static int messagecounter;

void CLIB_DECL usrintf_showmessage(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(messagetext,text,arg);
	va_end(arg);
	messagecounter = 2 * Machine->refresh_rate;
}

void CLIB_DECL usrintf_showmessage_secs(int seconds, const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(messagetext,text,arg);
	va_end(arg);
	messagecounter = seconds * Machine->refresh_rate;
}

void do_loadsave(struct mame_bitmap *bitmap, int request_loadsave)
{
	int file = 0;

	mame_pause(1);

	do
	{
		InputCode code;

		if (request_loadsave == LOADSAVE_SAVE)
			displaymessage(bitmap, "Select position to save to");
		else
			displaymessage(bitmap, "Select position to load from");

		update_video_and_audio();
		reset_partial_updates();

		if (input_ui_pressed(IPT_UI_CANCEL))
			break;

		code = code_read_async();
		if (code != CODE_NONE)
		{
			if (code >= KEYCODE_A && code <= KEYCODE_Z)
				file = 'a' + (code - KEYCODE_A);
			else if (code >= KEYCODE_0 && code <= KEYCODE_9)
				file = '0' + (code - KEYCODE_0);
			else if (code >= KEYCODE_0_PAD && code <= KEYCODE_9_PAD)
				file = '0' + (code - KEYCODE_0);
		}
	}
	while (!file);

	mame_pause(0);

	if (file > 0)
	{
		if (request_loadsave == LOADSAVE_SAVE)
			usrintf_showmessage("Save to position %c", file);
		else
			usrintf_showmessage("Load from position %c", file);
		cpu_loadsave_schedule(request_loadsave, file);
	}
	else
	{
		if (request_loadsave == LOADSAVE_SAVE)
			usrintf_showmessage("Save cancelled");
		else
			usrintf_showmessage("Load cancelled");
	}
}


void ui_show_fps_temp(double seconds)
{
	if (!showfps)
		showfpstemp = (int)(seconds * Machine->refresh_rate);
}

void ui_show_fps_set(int show)
{
	if (show)
	{
		showfps = 1;
	}
	else
	{
		showfps = 0;
		showfpstemp = 0;
		schedule_full_refresh();
	}
}

int ui_show_fps_get(void)
{
	return showfps || showfpstemp;
}

void ui_show_profiler_set(int show)
{
	if (show)
	{
		show_profiler = 1;
		profiler_start();
	}
	else
	{
		show_profiler = 0;
		profiler_stop();
		schedule_full_refresh();
	}
}

int ui_show_profiler_get(void)
{
	return show_profiler;
}

void ui_display_fps(struct mame_bitmap *bitmap)
{
	const char *text, *end;
	char textbuf[256];
	int done = 0;
	int y = 0;

	/* if we're not currently displaying, skip it */
	if (!showfps && !showfpstemp)
		return;

	/* get the current FPS text */
	text = osd_get_fps_text(mame_get_performance_info());

	/* loop over lines */
	while (!done)
	{
		/* find the end of this line and copy it to the text buf */
		end = strchr(text, '\n');
		if (end)
		{
			memcpy(textbuf, text, end - text);
			textbuf[end - text] = 0;
			text = end + 1;
		}
		else
		{
			strcpy(textbuf, text);
			done = 1;
		}

		/* render */
		ui_text(bitmap, textbuf, uirotwidth - strlen(textbuf) * uirotcharwidth, y);
		y += uirotcharheight;
	}

	/* update the temporary FPS display state */
	if (showfpstemp)
	{
		showfpstemp--;
		if (!showfps && showfpstemp == 0)
			schedule_full_refresh();
	}
}

int handle_user_interface(struct mame_bitmap *bitmap)
{
#ifdef MESS
	extern int mess_pause_for_ui;
#endif

	/* if the user pressed F12, save the screen to a file */
	if (input_ui_pressed(IPT_UI_SNAPSHOT))
		save_screen_snapshot(bitmap);


	/* This call is for the cheat, it must be called once a frame */
	if (options.cheat) DoCheat(bitmap);

	/* if the user pressed ESC, stop the emulation */
	/* but don't quit if the setup menu is on screen */
	if (setup_selected == 0 && input_ui_pressed(IPT_UI_CANCEL))
		return 1;

	if (setup_selected == 0 && input_ui_pressed(IPT_UI_CONFIGURE))
	{
		setup_selected = -1;
		if (osd_selected != 0)
		{
			osd_selected = 0;	/* disable on screen display */
			schedule_full_refresh();
		}
	}
	if (setup_selected != 0) setup_selected = setup_menu(bitmap, setup_selected);

#ifdef MAME_DEBUG
	if (!mame_debug)
#endif
		if (osd_selected == 0 && input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
		{
			osd_selected = -1;
			if (setup_selected != 0)
			{
				setup_selected = 0; /* disable setup menu */
				schedule_full_refresh();
			}
		}
	if (osd_selected != 0) osd_selected = on_screen_display(bitmap, osd_selected);


#if 0
	if (code_pressed_memory(KEYCODE_BACKSPACE))
	{
		if (jukebox_selected != -1)
		{
			jukebox_selected = -1;
			cpu_halt(0,1);
		}
		else
		{
			jukebox_selected = 0;
			cpu_halt(0,0);
		}
	}

	if (jukebox_selected != -1)
	{
		char buf[40];
		watchdog_reset_w(0,0);
		if (code_pressed_memory(KEYCODE_LCONTROL))
		{
#include "cpu/z80/z80.h"
			soundlatch_w(0,jukebox_selected);
			cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
		}
		if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
		{
			jukebox_selected = (jukebox_selected + 1) & 0xff;
		}
		if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
		{
			jukebox_selected = (jukebox_selected - 1) & 0xff;
		}
		if (input_ui_pressed_repeat(IPT_UI_UP,8))
		{
			jukebox_selected = (jukebox_selected + 16) & 0xff;
		}
		if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		{
			jukebox_selected = (jukebox_selected - 16) & 0xff;
		}
		sprintf(buf,"sound cmd %02x",jukebox_selected);
		displaymessage(buf);
	}
#endif


	/* if the user pressed F3, reset the emulation */
	if (input_ui_pressed(IPT_UI_RESET_MACHINE))
		machine_reset();

	if (input_ui_pressed(IPT_UI_SAVE_STATE))
		do_loadsave(bitmap, LOADSAVE_SAVE);

	if (input_ui_pressed(IPT_UI_LOAD_STATE))
		do_loadsave(bitmap, LOADSAVE_LOAD);

#ifndef MESS
	if (single_step || input_ui_pressed(IPT_UI_PAUSE)) /* pause the game */
	{
#else
	if (setup_selected)
		mess_pause_for_ui = 1;

	if (single_step || input_ui_pressed(IPT_UI_PAUSE) || mess_pause_for_ui) /* pause the game */
	{
#endif
/*		osd_selected = 0;	   disable on screen display, since we are going   */
							/* to change parameters affected by it */

		if (single_step == 0)
			mame_pause(1);

		while (!input_ui_pressed(IPT_UI_PAUSE))
		{
			profiler_mark(PROFILER_VIDEO);
			if (osd_skip_this_frame() == 0)
			{
				/* keep calling vh_screenrefresh() while paused so we can stuff */
				/* debug code in there */
				draw_screen();
			}
			profiler_mark(PROFILER_END);

			if (input_ui_pressed(IPT_UI_SNAPSHOT))
				save_screen_snapshot(bitmap);

			if (input_ui_pressed(IPT_UI_SAVE_STATE))
				do_loadsave(bitmap, LOADSAVE_SAVE);

			if (input_ui_pressed(IPT_UI_LOAD_STATE))
				do_loadsave(bitmap, LOADSAVE_LOAD);

			/* if the user pressed F4, show the character set */
			if (input_ui_pressed(IPT_UI_SHOW_GFX))
				showcharset(bitmap);

			if (setup_selected == 0 && input_ui_pressed(IPT_UI_CANCEL))
				return 1;

			if (setup_selected == 0 && input_ui_pressed(IPT_UI_CONFIGURE))
			{
				setup_selected = -1;
				if (osd_selected != 0)
				{
					osd_selected = 0;	/* disable on screen display */
					schedule_full_refresh();
				}
			}
			if (setup_selected != 0) setup_selected = setup_menu(bitmap, setup_selected);

#ifdef MAME_DEBUG
			if (!mame_debug)
#endif
				if (osd_selected == 0 && input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
				{
					osd_selected = -1;
					if (setup_selected != 0)
					{
						setup_selected = 0; /* disable setup menu */
						schedule_full_refresh();
					}
				}
			if (osd_selected != 0) osd_selected = on_screen_display(bitmap, osd_selected);

			if (options.cheat) DisplayWatches(bitmap);

			/* show popup message if any */
			if (messagecounter > 0)
			{
				displaymessage(bitmap, messagetext);

				if (--messagecounter == 0)
					schedule_full_refresh();
			}

			/* show FPS display? */
			if (input_ui_pressed(IPT_UI_SHOW_FPS))
			{
				/* if we're temporarily on, turn it off immediately */
				if (showfpstemp)
				{
					showfpstemp = 0;
					schedule_full_refresh();
				}

				/* otherwise, just toggle; force a refresh if going off */
				else
				{
					showfps ^= 1;
					if (!showfps)
						schedule_full_refresh();
				}
			}

			/* add the FPS counter */
			ui_display_fps(bitmap);


			update_video_and_audio();
			reset_partial_updates();

#ifdef MESS
			if (!setup_selected && mess_pause_for_ui)
			{
				mess_pause_for_ui = 0;
				break;
			}
#endif /* MESS */
		}

		if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
			single_step = 1;
		else
		{
			single_step = 0;
			mame_pause(0);
		}

		schedule_full_refresh();
	}

#if defined(__sgi) && !defined(MESS)
	game_paused = 0;
#endif

	/* show popup message if any */
	if (messagecounter > 0)
	{
		displaymessage(bitmap, messagetext);

		if (--messagecounter == 0)
			schedule_full_refresh();
	}


	if (input_ui_pressed(IPT_UI_SHOW_PROFILER))
	{
		ui_show_profiler_set(!ui_show_profiler_get());
	}

	if (show_profiler) profiler_show(bitmap);


	/* show FPS display? */
	if (input_ui_pressed(IPT_UI_SHOW_FPS))
	{
		/* toggle fps */
		ui_show_fps_set(!ui_show_fps_get());
	}


	/* if the user pressed F4, show the character set */
	if (input_ui_pressed(IPT_UI_SHOW_GFX))
	{
		osd_sound_enable(0);

		showcharset(bitmap);

		osd_sound_enable(1);
	}

	/* if the user pressed F1 and this is a lightgun game, toggle the crosshair */
	if (input_ui_pressed(IPT_UI_TOGGLE_CROSSHAIR))
	{
		drawgfx_toggle_crosshair();
	}

	/* add the FPS counter */
	ui_display_fps(bitmap);

	return 0;
}


void init_user_interface(void)
{
	extern int snapno;	/* in common.c */

	snapno = 0; /* reset snapshot counter */

	/* clear the input memory */
	while (code_read_async() != CODE_NONE) {};


	setup_menu_init();
	setup_selected = 0;

	onscrd_init();
	osd_selected = 0;

	jukebox_selected = -1;

	single_step = 0;
	scroll_reset = 1;

	displaymessagewindow_under = 0;

	lhw = strlen(ui_getstring(UI_lefthilight));
	rhw = strlen(ui_getstring(UI_righthilight));
	uaw = strlen(ui_getstring(UI_uparrow));
	daw = strlen(ui_getstring(UI_downarrow));
	law = strlen(ui_getstring(UI_leftarrow));
	raw = strlen(ui_getstring(UI_rightarrow));
#ifdef CMD_LIST
	flash_all_values_and_buffers();
#endif /* CMD_LIST */
}

int onscrd_active(void)
{
	return osd_selected;
}

int setup_active(void)
{
	return setup_selected;
}

#if defined(__sgi) && !defined(MESS)

/* Return if the game is paused or not */
int is_game_paused(void)
{
	return game_paused;
}

#endif

