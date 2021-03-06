/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  win32ui.c

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/
#ifdef _MSC_VER
#ifndef NONAMELESSUNION
#define NONAMELESSUNION 
#endif
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string.h>
#include <sys/stat.h>
#include <wingdi.h>
#include <time.h>

#include <driver.h>
#include <osdepend.h>
#include <unzip.h>

#include "resource.h"
#include "resource.hm"

#include "screenshot.h"
#include "mame32.h"
#include "M32Util.h"
#include "file.h"
#include "audit32.h"
#include "Directories.h"
#include "Properties.h"
#include "ColumnEdit.h"
#include "bitmask.h"
#include "TreeView.h"
#include "Splitters.h"
#include "help.h"
#include "history.h"
#include "options.h"
#include "dialogs.h"
#include "state.h"

#include "DirectDraw.h"
#include "DirectInput.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */

#include "password.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif

#if defined(__GNUC__)

/* fix warning: cast does not match function type */
#undef ListView_GetImageList
#define ListView_GetImageList(w,i) (HIMAGELIST)(LRESULT)(int)SendMessage((w),LVM_GETIMAGELIST,(i),0)

#undef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)(LRESULT)(int)SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))

#undef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
    SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))

#undef ListView_GetHeader
#define ListView_GetHeader(w) (HWND)(LRESULT)(int)SNDMSG((w),LVM_GETHEADER,0,0)

#define HDM_SETIMAGELIST        (HDM_FIRST + 8)
#define Header_SetImageList(h,i) (HIMAGELIST)(LRESULT)(int)SNDMSG((h), HDM_SETIMAGELIST, 0, (LPARAM)i)


#endif /* defined(__GNUC__) */

#ifndef HDF_SORTUP
#define HDF_SORTUP 0x400
#endif

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x200
#endif

#define MM_PLAY_GAME (WM_APP + 15000)

#define JOYGUI_MS 100

#define JOYGUI_TIMER 1
#define SCREENSHOT_TIMER 2

/* Max size of a sub-menu */
#define DBU_MIN_WIDTH  292
#define DBU_MIN_HEIGHT 190

int MIN_WIDTH  = DBU_MIN_WIDTH;
int MIN_HEIGHT = DBU_MIN_HEIGHT;

/* Max number of bkground picture files */
#define MAX_BGFILES 100

/* Max car for a password */
#define MAX_PWD_LEN 10

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif

#define STATESAVE_VERSION 1

typedef BOOL (WINAPI *common_file_dialog_proc)(LPOPENFILENAME lpofn);

/***************************************************************************
 externally defined global variables
 ***************************************************************************/
extern const ICONDATA g_iconData[];
extern const char g_szPlayGameString[];
extern const char g_szGameCountString[];

/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL             Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);
static void             Win32UI_exit(void);

static BOOL             PumpMessage(void);
static void             OnIdle(void);
static void             OnSize(HWND hwnd, UINT state, int width, int height);
static long WINAPI      MameWindowProc(HWND hwnd,UINT message,UINT wParam,LONG lParam);

static void             SetView(int menu_id,int listview_style);
static void             ResetListView(void);
static void             UpdateGameList(void);
static void             DestroyIcons(void);
static void             ReloadIcons(void);
static void             PollGUIJoystick(void);
static void             PressKey(HWND hwnd,UINT vk);
static BOOL             MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static void             KeyboardKeyDown(int syskey, int vk_code, int special);
static void             KeyboardKeyUp(int syskey, int vk_code, int special);
static void             KeyboardStateClear(void);

static void             UpdateStatusBarIcons(int game);
static void             UpdateStatusBar(void);
static BOOL             PickerHitTest(HWND hWnd);
static BOOL             MamePickerNotify(NMHDR *nm);
static BOOL             TreeViewNotify(NMHDR *nm);
// "tab" = value in options.h (TAB_SCREENSHOT, for example)
// "tab index" = index in the ui tab control
static int GetTabFromTabIndex(int tab_index);
static int CalculateCurrentTabIndex(void);
static void CalculateNextTab(void);

static BOOL             TabNotify(NMHDR *nm);

static void             ResetBackground(char *szFile);
static void				RandomSelectBackground(void);
static void             LoadBackgroundBitmap(void);

static int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2);
static void             ResetTabControl(void);
static int CALLBACK     ListCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem);
static int              BasicCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem);

static void             DisableSelection(void);
static void             EnableSelection(int nGame);

static int              GetSelectedPick(void);
static int              GetSelectedPickItem(void);
static HICON			GetSelectedPickItemIcon(void);
static void             SetSelectedPick(int new_index);
static void             SetSelectedPickItem(int val);
static void             SetRandomPickItem(void);

static LPTREEFOLDER     GetSelectedFolder(void);
static HICON			GetSelectedFolderIcon(void);

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK PasswordDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK BackgroundDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK FindGameDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK BackMainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static BOOL             SelectLanguageFile(HWND hWnd, TCHAR* filename);
static void             MamePlayRecordGame(void);
static void             MamePlayBackGame(void);
static void				MameLoadState(void);
static BOOL             CommonFileDialog(common_file_dialog_proc cfd,char *filename, BOOL bZip, BOOL bInp);
static void             MamePlayGame(void);
static void             MamePlayGameWithOptions(int nGame);
static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static int UpdateLoadProgress(const char* name, const struct rom_load_data *romdata);
static void SortListView(void);
static BOOL GameCheck(void);
static BOOL FolderCheck(void);

static void             ToggleScreenShot(void);
static void             AdjustMetrics(void);
static void             EnablePlayOptions(int nIndex, options_type *o);

/* for header */
static LRESULT CALLBACK ListViewWndProc(HWND, UINT, WPARAM, LPARAM);
static BOOL ListCtrlOnErase(HWND hWnd, HDC hDC);
static BOOL ListCtrlOnPaint(HWND hWnd, UINT uMsg);
static void ResetHeaderSortIcon(void);

/* Icon routines */
static DWORD            GetShellLargeIconSize(void);
static DWORD            GetShellSmallIconSize(void);
static void             CreateIcons(void);
static int              GetIconForDriver(int nItem);
static void             AddDriverIcon(int nItem,int default_icon_index);

// Context Menu handlers
static void             UpdateMenu(HMENU hMenu);
static void InitTreeContextMenu(HMENU hTreeMenu);
static void InitSystrayContextMenu(HMENU hSystrayMenu);
static void ToggleShowFolder(int folder);
static BOOL             HandleContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HeaderOnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleTreeContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleScreenShotContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleSystemTrayContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);

static void             InitListView(void);
/* Re/initialize the ListView header columns */
static void ResetColumnDisplay(BOOL first_time);
static int GetRealColumnFromViewColumn(int view_column);
static int GetViewColumnFromRealColumn(int real_column);

/* Custom Draw item */
static void             DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
static int              GetNumColumns(HWND hWnd);

static void             CopyToolTipText (LPTOOLTIPTEXT lpttt);

static void             ProgressBarShow(void);
static void             ProgressBarHide(void);
static void             ResizeProgressBar(void);
static void             ProgressBarStep(void);
static void              ProgressBarStepParam(int iGameIndex, int nGameCount);

static HWND             InitProgressBar(HWND hParent);
static HWND             InitToolbar(HWND hParent);
static HWND             InitStatusBar(HWND hParent);

static LRESULT          Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);

static void             UpdateHistory(void);


void RemoveCurrentGameCustomFolder(void);
void RemoveGameCustomFolder(int driver_index);

void BeginListViewDrag(NM_LISTVIEW *pnmv);
void MouseMoveListViewDrag(POINTS pt);
void ButtonUpListViewDrag(POINTS p);

void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height);

BOOL MouseHasBeenMoved(void);
void SwitchFullScreenMode(void);

// Game Window Communication Functions
void SendMessageToProcess(LPPROCESS_INFORMATION lpProcessInformation,
						  UINT Msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam);
void SendIconToProcess(LPPROCESS_INFORMATION lpProcessInformation, int nGameIndex);

static void SaveGameListToFile(char *szFile, int Formatted);

static void SetGameLockUnlock(int game, BOOL locked);
static void SetFolderLockUnlock(LPTREEFOLDER lpFolder, BOOL bLock);
static void CreateDefaultLockUnlockList(void);
static BOOL CreateAndLoadLockUnlockList(void);
static void SaveAndDestroyLockUnlockList(void);
static void CheckPassword(char *Exe, char *Pwd);
static BOOL GetGameLockByDriverName(char *DriverName);

static void CreateBackgroundMain(HINSTANCE hInstance, BOOL ForCreate );
static int GetGamesFound(char *Pattern, int Criteria);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	BYTE    bWidth;               /* Width of the image */
	BYTE    bHeight;              /* Height of the image (times 2) */
	BYTE    bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	BYTE    bReserved;            /* Reserved */
	WORD    wPlanes;              /* Color Planes */
	WORD    wBitCount;            /* Bits per pixel */
	DWORD   dwBytesInRes;         /* how many bytes in this resource? */
	DWORD   dwImageOffset;        /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; /* Width, Height and bpp */
	LPBYTE          lpBits;                /* ptr to DIB bits */
	DWORD           dwNumBytes;            /* how many bytes? */
	LPBITMAPINFO    lpbi;                  /* ptr to header */
	LPBYTE          lpXOR;                 /* ptr to XOR image bits */
	LPBYTE          lpAND;                 /* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

/* Which edges of a control are anchored to the corresponding side of the parent window */
#define RA_LEFT     0x01
#define RA_RIGHT    0x02
#define RA_TOP      0x04
#define RA_BOTTOM   0x08
#define RA_ALL      0x0F

#define RA_END  0
#define RA_ID   1
#define RA_HWND 2

typedef struct
{
	int         type;       /* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
							   to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int     id;         /* Window control id */
		HWND    hwnd;       /* Window handle */
	} u;
	int         action;     /* What to do when control is resized */
	void        *subwindow; /* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT        rect;       /* Client rect of window; must be initialized before first resize */
	ResizeItem* items;      /* Array of subitems to be resized */
} Resize;

static void             ResizeWindow(HWND hParent, Resize *r);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

#define MAX_MENUS 3

#define SPLITTER_WIDTH	4
#define MIN_VIEW_WIDTH	10

// Struct needed for Game Window Communication

typedef struct
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
} FINDWINDOWHANDLE;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND   hMain  = NULL;
static HACCEL hAccel = NULL;

static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;
static HWND hTabCtrl  = NULL;

static BOOL g_bAbortLoading = FALSE; /* doesn't work right */
static BOOL g_bCloseLoading = FALSE;

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     /* Font for list view */

static int game_count = 0;

/* global data--know where to send messages */
static BOOL in_emulation;

/* idle work at startup */
static BOOL idle_work;

static int  game_index;
static int  progBarStep;

static BOOL bDoGameCheck = FALSE;

/* current menu check for listview */
static int current_view_id;

/* Tree control variables */
static BOOL bShowTree      = 1;
static BOOL bShowToolBar   = 1;
static BOOL bShowStatusBar = 1;
static BOOL bShowTabCtrl   = 1;
static BOOL bProgressShown = FALSE;
static BOOL bListReady     = FALSE;
static BOOL bSortTree      = 1;

/* Password / locks management */
static BOOL bPwdEnabled    = TRUE;
static BOOL bPwdVerified   = FALSE;
static BOOL bUseLockGUI    = FALSE;

/* use a joystick subsystem in the gui? */
static struct OSDJoystick* g_pJoyGUI = NULL;

/* store current keyboard state (in internal codes) here */
static InputCode keyboard_state[ __code_key_last ]; /* __code_key_last #defines the number of internal key_codes */

/* table copied from windows/inputs.c */
// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3

// master translation table
static int key_trans_table[][4] =
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
	{ KEYCODE_MENU, 		DIK_APPS,			VK_APPS, 		0 }
};


typedef struct
{
	char		name[40];	    // functionality name (optional)
	InputSeq	is;				// the input sequence (the keys pressed)
	UINT		func_id;        // the identifier
	InputSeq* (*getiniptr)(void);// pointer to function to get the value from .ini file
} GUISequence;

static GUISequence GUISequenceControl[]=
{
	{"gui_key_up",                SEQ_DEF_0,    ID_UI_UP,           Get_ui_key_up },
	{"gui_key_down",              SEQ_DEF_0,    ID_UI_DOWN,         Get_ui_key_down },
	{"gui_key_left",              SEQ_DEF_0,    ID_UI_LEFT,         Get_ui_key_left },
	{"gui_key_right",             SEQ_DEF_0,    ID_UI_RIGHT,        Get_ui_key_right },
	{"gui_key_start",             SEQ_DEF_0,    ID_UI_START,        Get_ui_key_start },
	{"gui_key_pgup",              SEQ_DEF_0,    ID_UI_PGUP,         Get_ui_key_pgup },
	{"gui_key_pgdwn",             SEQ_DEF_0,    ID_UI_PGDOWN,       Get_ui_key_pgdwn },
	{"gui_key_home",              SEQ_DEF_0,    ID_UI_HOME,         Get_ui_key_home },
	{"gui_key_end",               SEQ_DEF_0,    ID_UI_END,          Get_ui_key_end },
	{"gui_key_ss_change",         SEQ_DEF_0,    IDC_SSFRAME,        Get_ui_key_ss_change },
	{"gui_key_history_up",        SEQ_DEF_0,    ID_UI_HISTORY_UP,   Get_ui_key_history_up },
	{"gui_key_history_down",      SEQ_DEF_0,    ID_UI_HISTORY_DOWN, Get_ui_key_history_down },
	
	{"gui_key_context_filters",    SEQ_DEF_0,    ID_CONTEXT_FILTERS,       Get_ui_key_context_filters },
	{"gui_key_select_random",      SEQ_DEF_0,    ID_CONTEXT_SELECT_RANDOM, Get_ui_key_select_random },
	{"gui_key_game_audit",         SEQ_DEF_0,    ID_GAME_AUDIT,            Get_ui_key_game_audit },
	{"gui_key_game_properties",    SEQ_DEF_0,    ID_GAME_PROPERTIES,       Get_ui_key_game_properties },
	{"gui_key_help_contents",      SEQ_DEF_0,    ID_HELP_CONTENTS,         Get_ui_key_help_contents },
	{"gui_key_update_gamelist",    SEQ_DEF_0,    ID_UPDATE_GAMELIST,       Get_ui_key_update_gamelist },
	{"gui_key_view_folders",       SEQ_DEF_0,    ID_VIEW_FOLDERS,          Get_ui_key_view_folders },
	{"gui_key_view_fullscreen",    SEQ_DEF_0,    ID_VIEW_FULLSCREEN,       Get_ui_key_view_fullscreen },
	{"gui_key_view_pagetab",       SEQ_DEF_0,    ID_VIEW_PAGETAB,          Get_ui_key_view_pagetab },
	{"gui_key_view_picture_area",  SEQ_DEF_0,    ID_VIEW_PICTURE_AREA,     Get_ui_key_view_picture_area },
	{"gui_key_view_status",        SEQ_DEF_0,    ID_VIEW_STATUS,           Get_ui_key_view_status },
	{"gui_key_view_toolbars",      SEQ_DEF_0,    ID_VIEW_TOOLBARS,         Get_ui_key_view_toolbars },

	{"gui_key_view_tab_cabinet",     SEQ_DEF_0,  ID_VIEW_TAB_CABINET,       Get_ui_key_view_tab_cabinet },
	{"gui_key_view_tab_cpanel",      SEQ_DEF_0,  ID_VIEW_TAB_CONTROL_PANEL, Get_ui_key_view_tab_cpanel },
	{"gui_key_view_tab_flyer",       SEQ_DEF_0,  ID_VIEW_TAB_FLYER,         Get_ui_key_view_tab_flyer },
	{"gui_key_view_tab_history",     SEQ_DEF_0,  ID_VIEW_TAB_HISTORY,       Get_ui_key_view_tab_history },
	{"gui_key_view_tab_marquee",     SEQ_DEF_0,  ID_VIEW_TAB_MARQUEE,       Get_ui_key_view_tab_marquee },
	{"gui_key_view_tab_screenshot",  SEQ_DEF_0,  ID_VIEW_TAB_SCREENSHOT,    Get_ui_key_view_tab_screenshot },
	{"gui_key_view_tab_title",       SEQ_DEF_0,  ID_VIEW_TAB_TITLE,         Get_ui_key_view_tab_title },
	{"gui_key_quit",                 SEQ_DEF_0,  ID_FILE_EXIT,              Get_ui_key_quit },
};


#define NUM_GUI_SEQUENCES (sizeof(GUISequenceControl) / sizeof(GUISequenceControl[0]))


static UINT    lastColumnClick   = 0;
static WNDPROC g_lpListViewWndProc = NULL;
static WNDPROC g_lpHistoryWndProc = NULL;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

/* Tool and Status bar variables */
static HWND hStatusBar = 0;
static HWND hToolBar   = 0;

/* Column Order as Displayed */
static BOOL oldControl = FALSE;
static BOOL xpControl = FALSE;
static int  realColumn[COLUMN_MAX];

/* Used to recalculate the main window layout */
static int  bottomMargin;
static int  topMargin;
static int  have_history = FALSE;

static BOOL have_selection = FALSE;

static HBITMAP hMissing_bitmap;

/* Icon variables */
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static HIMAGELIST   hHeaderImages = NULL;
static HIMAGELIST   hPwdIconList = NULL;
static int          *icon_index = NULL; /* for custom per-game icons */
       char			*games_found = NULL; /* list of games found */

static TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,    TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_PICTURE_AREA,TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_LARGE_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_SMALL_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{4, ID_VIEW_LIST_MENU,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{5, ID_VIEW_DETAIL,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 5},
	{6, ID_VIEW_GROUPED, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 6},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{7, ID_HELP_ABOUT,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7},
	{8, ID_HELP_CONTENTS,   TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 8}
};

#define NUM_TOOLBUTTONS (sizeof(tbb) / sizeof(tbb[0]))

#define NUM_TOOLTIPS 8

static char szTbStrings[NUM_TOOLTIPS + 1][30] =
{
	"Toggle Folder List",
	"Toggle Screen Shot",
	"Large Icons",
	"Small Icons",
	"List",
	"Details",
	"Grouped",
	"About",
	"Help"
};

static int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_LARGE_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
	ID_VIEW_GROUPED,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    RA_RIGHT | RA_TOP,                 NULL },
	{ RA_END,  { 0 },            0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

/* our dialog/configured options */
static options_type playing_game_options;

/* last directory for common file dialogs */
static char last_directory[MAX_PATH];

/* system-wide window message sent out with an ATOM of the current game name
   each time it changes */
static UINT g_mame32_message = 0;
static BOOL g_bDoBroadcast   = FALSE;

static BOOL use_gui_romloading = FALSE;

static BOOL g_listview_dragging = FALSE;
HIMAGELIST himl_drag;
int game_dragged; /* which game started the drag */
HTREEITEM prev_drag_drop_target; /* which tree view item we're currently highlighting */

static BOOL g_in_treeview_edit = FALSE;

typedef struct
{
    const char *name;
    int index;
} driver_data_type;
static driver_data_type *sorted_drivers;

static char * g_pRecordName = NULL;
static char * g_pPlayBkName = NULL;
static char * g_pSaveStateName = NULL;
static char * override_playback_directory = NULL;
static char * override_savestate_directory = NULL;

static char * tcLockUnlockList = NULL;

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Icon displayed in system tray */
static BOOL		bIsWindowsVisible = TRUE;
static UINT		uShellIconMsg = 0;
static NOTIFYICONDATA	MameIcon;

/* Background Image handles also accessed from TreeView.c */
static HPALETTE         hPALbg   = 0;
static HBITMAP          hBackground  = 0;
static MYBITMAPINFO     bmDesc;

static HWND             hBackMain = NULL;
static HBITMAP          hSplashBmp = 0;
static HDC              hMemoryDC;


/* List view Column text */
const char* column_names[COLUMN_MAX] =
{
	"Game",
	"ROMs",
	"Samples",
	"Directory",
	"Type",
	"Trackball",
	"Played",
	"Manufacturer",
	"Year",
	"Clone Of",
	"Source",
	"Play Time"
};

/* a tiny compile is without Neogeo games */
#if (defined(NEOFREE) || defined(TINY_COMPILE)) && !defined(NEOMAME)
struct GameDriver driver_neogeo =
{
	__FILE__,
	0,
	"Neo-Geo Fake driver",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NOT_A_DRIVER,
};
#else
extern struct GameDriver driver_neogeo;
#endif

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) \
    SendMessage(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) \
    SendMessage(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

/***************************************************************************
    External functions
 ***************************************************************************/

static void CreateCommandLine(int nGameIndex, char* pCmdLine)
{
	char pModule[_MAX_PATH];
	options_type* pOpts;
	LPTREEFOLDER folder;

	// this command line can grow too long for win9x, so we try to not send
	// some default values

	GetModuleFileName(GetModuleHandle(NULL), pModule, _MAX_PATH);

	folder = GetFolderByName(FOLDER_SOURCE-1, (char*)GetDriverFilename(nGameIndex) );
	if( folder )
	{
		pOpts = GetGameOptions(nGameIndex, folder->m_nFolderId);
	}
	else
	{
		pOpts = GetGameOptions(nGameIndex, -1);
	}

	sprintf(pCmdLine, "%s %s", pModule, drivers[nGameIndex]->name);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -rp \"%s\"",            GetRomDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -sp \"%s\"",         GetSampleDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -inipath \"%s\"",			GetIniDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cfg_directory \"%s\"",      GetCfgDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -nvram_directory \"%s\"",    GetNvramDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -memcard_directory \"%s\"",  GetMemcardDir());

	if (override_playback_directory != NULL)
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -input_directory \"%s\"",override_playback_directory);
	}
	else
		sprintf(&pCmdLine[strlen(pCmdLine)], " -input_directory \"%s\"",GetInpDir());

	sprintf(&pCmdLine[strlen(pCmdLine)], " -hiscore_directory \"%s\"",  GetHiDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -state_directory \"%s\"",    GetStateDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -artwork_directory \"%s\"",	GetArtDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -snapshot_directory \"%s\"", GetImgDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -diff_directory \"%s\"",     GetDiffDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cheat_file \"%s\"",         GetCheatFileName());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -history_file \"%s\"",       GetHistoryFileName());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -mameinfo_file \"%s\"",      GetMAMEInfoFileName());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -ctrlr_directory \"%s\"",    GetCtrlrDir());

	/* video */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%safs",                     pOpts->autoframeskip   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fs %d",                     pOpts->frameskip);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%swaitvsync",               pOpts->wait_vsync      ? "" : "no");
	if (pOpts->use_triplebuf)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%stb",                  pOpts->use_triplebuf ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sw",                       pOpts->window_mode     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdd",                      pOpts->use_ddraw       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%shws",                     pOpts->ddraw_stretch   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -r %s",                      pOpts->resolution);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -refresh %d",                pOpts->gfx_refresh);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssl",                      pOpts->scanlines       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sswitchres",               pOpts->switchres       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sswitchbpp",               pOpts->switchbpp       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smax",                     pOpts->maximize        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ska",                      pOpts->keepaspect      ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smatchrefresh",            pOpts->matchrefresh    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssyncrefresh",             pOpts->syncrefresh     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sthrottle",                pOpts->throttle        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -fsb %f",                    pOpts->gfx_brightness);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -ftr %d",                    pOpts->frames_to_display);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -effect %s",                 pOpts->effect);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -screen_aspect %s",          pOpts->aspect);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -cs %s",                     GetCleanStretchShortName(pOpts->clean_stretch));
	sprintf(&pCmdLine[strlen(pCmdLine)], " -zoom %i",                   pOpts->zoom);

	// d3d
	if (pOpts->use_d3d)
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -d3d");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -flt %i",pOpts->d3d_filter);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sd3dtexmanage",pOpts->d3d_texture_management ? "" : "no");
		if (pOpts->d3d_effect != D3D_EFFECT_NONE)
			sprintf(&pCmdLine[strlen(pCmdLine)], " -d3deffect %s",GetD3DEffectShortName(pOpts->d3d_effect));
		sprintf(&pCmdLine[strlen(pCmdLine)], " -d3dprescale %s",GetD3DPrescaleShortName(pOpts->d3d_prescale));

		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sd3deffectrotate",pOpts->d3d_rotate_effects ? "" : "no");

		if (pOpts->d3d_scanlines_enable)
			sprintf(&pCmdLine[strlen(pCmdLine)], " -d3dscan %i",pOpts->d3d_scanlines);
		if (pOpts->d3d_feedback_enable)
			sprintf(&pCmdLine[strlen(pCmdLine)], " -d3dfeedback %i",pOpts->d3d_feedback);
	}
	/* input */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smouse",                   pOpts->use_mouse       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sjoy",                     pOpts->use_joystick    ? "" : "no");
	if (pOpts->use_joystick) {
		sprintf(&pCmdLine[strlen(pCmdLine)], " -a2d %f",                pOpts->f_a2d);
	}
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssteady",                  pOpts->steadykey       ? "" : "no");

	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sgun",!pOpts->window_mode && pOpts->lightgun ? "" : "no");
	if (pOpts->dual_lightgun)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdual",pOpts->dual_lightgun ? "" : "no");
	if (pOpts->offscreen_reload)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sreload",pOpts->offscreen_reload ? "" : "no");

	sprintf(&pCmdLine[strlen(pCmdLine)], " -ctrlr \"%s\"",              pOpts->ctrlr);
	
	/* core video */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -bright %f",                 pOpts->f_bright_correct); 
	sprintf(&pCmdLine[strlen(pCmdLine)], " -pause_brightness %f",       pOpts->f_pause_bright); 

	if (pOpts->norotate)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%snorotate",pOpts->norotate ? "" : "no");
	if (pOpts->ror)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sror",pOpts->ror ? "" : "no");
	if (pOpts->rol)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%srol",pOpts->rol ? "" : "no");
	if (pOpts->auto_ror)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -autoror");
	if (pOpts->auto_rol)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -autorol");
	if (pOpts->flipx)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipx",pOpts->flipx ? "" : "no");
	if (pOpts->flipy)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipy",pOpts->flipy ? "" : "no");
	if (strcmp(pOpts->debugres,"auto") != 0)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -dr %s",       pOpts->debugres); 
	sprintf(&pCmdLine[strlen(pCmdLine)], " -gamma %f",                  pOpts->f_gamma_correct);

	/* vector */
	if (DriverIsVector(nGameIndex))
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%saa",                      pOpts->antialias       ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%stl",                       pOpts->translucency    ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -beam %f",                   pOpts->f_beam);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -flicker %f",                pOpts->f_flicker);
		sprintf(&pCmdLine[strlen(pCmdLine)], " -intensity %f",              pOpts->f_intensity);
	}
	/* sound */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -sr %d",                     pOpts->samplerate);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssamples",                 pOpts->use_samples     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sresamplefilter",          pOpts->use_filter      ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssound",                   pOpts->enable_sound    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -vol %d",                    pOpts->attenuation);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -audio_latency %i",          pOpts->audio_latency);
	/* misc artwork options */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sart",                     pOpts->use_artwork     ? "" : "no");

	if (pOpts->use_artwork == TRUE)
	{
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sbackdrop",            pOpts->backdrops       ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%soverlay",             pOpts->overlays        ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sbezel",               pOpts->bezels          ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sartcrop",             pOpts->artwork_crop    ? "" : "no");
		sprintf(&pCmdLine[strlen(pCmdLine)], " -artres %d",             pOpts->artres);
	}

	/* misc */
	if (pOpts->cheat)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sc",                   pOpts->cheat ? "" : "no");
	if (pOpts->mame_debug)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -%sd",               pOpts->mame_debug ? "" : "no");
	if (g_pPlayBkName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -pb \"%s\"",             g_pPlayBkName);
	if (g_pRecordName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -rec \"%s\"",            g_pRecordName);
	if (g_pSaveStateName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -state \"%s\"",          g_pSaveStateName);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%slog",                     pOpts->errorlog        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssleep",                   pOpts->sleep           ? "" : "no");
	if (pOpts->old_timing)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -rdtsc");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sleds",                    pOpts->leds            ? "" : "no");

	if (DriverHasOptionalBIOS(nGameIndex))
		sprintf(&pCmdLine[strlen(pCmdLine)], " -bios %i",pOpts->bios);		

	if (GetSkipDisclaimer())
		sprintf(&pCmdLine[strlen(pCmdLine)]," -skip_disclaimer");
	if (GetSkipGameInfo())
		sprintf(&pCmdLine[strlen(pCmdLine)]," -skip_gameinfo");
	if (GetHighPriority() == TRUE)
		sprintf(&pCmdLine[strlen(pCmdLine)]," -high_priority");

	dprintf("Launching MAME32:");
	dprintf("%s",pCmdLine);

	// Add password
	strcat(pCmdLine, " Pwd=");
	GetPassword(&pCmdLine[strlen(pCmdLine)]);
}

static BOOL WaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG   msg;

	while (1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE;
		}
	}
	return FALSE;
}

static int RunMAME(int nGameIndex)
{
	DWORD               dwExitCode = 0;
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
	char pCmdLine[2048];
	time_t start, end;
	double elapsedtime;
	
	CreateCommandLine(nGameIndex, pCmdLine);

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	if (!CreateProcess(NULL,
						pCmdLine,
						NULL,		  /* Process handle not inheritable. */
						NULL,		  /* Thread handle not inheritable. */
						TRUE,		  /* Handle inheritance.  */
						0,			  /* Creation flags. */
						NULL,		  /* Use parent's environment block.  */
						NULL,		  /* Use parent's starting directory.  */
						&si,		  /* STARTUPINFO */
						&pi))		  /* PROCESS_INFORMATION */
	{
		dprintf("CreateProcess failed.");
		dwExitCode = GetLastError();
	}
	else
	{

		ShowWindow(hMain, SW_HIDE);
		SendIconToProcess(&pi, nGameIndex);

		time(&start);

		// Wait until child process exits.
		WaitWithMessageLoop(pi.hProcess);

		GetExitCodeProcess(pi.hProcess, &dwExitCode);

		time(&end);
		elapsedtime = end - start;
		if( dwExitCode == 0 )
		{
			/*Check the exitcode before incrementing Playtime*/
			IncrementPlayTime(nGameIndex, elapsedtime);
			ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		}

		if ( bIsWindowsVisible )
		ShowWindow(hMain, SW_SHOW);

		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return dwExitCode;
}

int Mame32Main(HINSTANCE    hInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	MSG 	msg;

	dprintf("MAME32 starting");

	options.gui_host = 1;
	use_gui_romloading = TRUE;

	if (__argc != 1)
	{
		/* Rename main because gcc will use it instead of WinMain even with -mwindows */
		extern int DECL_SPEC main_(int, char**);

		// Get number of games
		game_count = 0;
		while (drivers[game_count] != 0)
			game_count++;

		// Last argument may contain password
		if ( strncmp(__argv[__argc-1], "Pwd=", 4) == 0 )
		{
			CheckPassword(__argv[0], __argv[__argc-1]+4);

			// Remove password from command line
			__argc--;
		}
		else
		{
			CheckPassword(__argv[0], NULL);
		}

		exit(main_(__argc, __argv));
	}
	if (!Win32UI_init(hInstance, lpCmdLine, nCmdShow))
		return 1;

	/*
		Simplified MFC Run() alg. See mfc/src/thrdcore.cpp.
	*/
	for (;;)
	{
		// phase1: check to see if we can do idle work
		while (idle_work && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			// call OnIdle while idle_work
			OnIdle();
		}

		// phase2: pump messages while available
		do
		{
			// pump message, but quit on WM_QUIT
			if (!PumpMessage())
			{
				Win32UI_exit();
				return msg.wParam;
			}

		}
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
	}

	return 0;
}


HWND GetMainWindow(void)
{
	return hMain;
}

HWND GetTreeView(void)
{
	return hTreeView;
}

int GetNumColumns(HWND hWnd)
{
	int  nColumn = 0;
	int  i;
	HWND hwndHeader;
	int  shown[COLUMN_MAX];

	GetColumnShown(shown);
	hwndHeader = ListView_GetHeader(hwndList);

	if (oldControl || (nColumn = Header_GetItemCount(hwndHeader)) < 1)
	{
		nColumn = 0;
		for (i = 0; i < COLUMN_MAX ; i++ )
		{
			if (shown[i])
				nColumn++;
		}
	}
	return nColumn;
}

void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX];
	int nColumnMax;
	int i;

	nColumnMax = GetNumColumns(hwndList);

	/* Get the Column Order and save it */
	if (!oldControl)
	{
		ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++)
		{
			order[i] = GetRealColumnFromViewColumn(tmpOrder[i]);
		}
	}
}

/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	ICONIMAGE           IconImage;
	LPICONDIRENTRY      lpIDE = NULL;
	UINT                nNumImages;
	UINT                nBufferIndex = 0;
	HICON               hIcon = NULL;

	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it 'reserved' ?	 (ie 0) */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Then that's the number of images in the ICO file */
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if ( nNumImages < 1 )
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
	{
		return NULL;
	}

	/* Assign icon directory entries from buffer */
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if ( lpIDE->dwImageOffset > nBufferSize )
	{
		return NULL;
	}

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
	{
		return NULL;
	}

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;

	hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000,
			(*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biWidth, (*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biHeight/2, 0 );

	/* It failed, odds are good we're on NT so try the non-Ex way */
	if (hIcon == NULL)
	{
		/* We would break on NT if we try with a 16bpp image */
		if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		{
			hIcon = CreateIconFromResource(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000);
		}
	}
	return hIcon;
}

HICON LoadIconFromFile(const char *iconname)
{
	HICON       hIcon = 0;
	struct stat file_stat;
	char        tmpStr[MAX_PATH];
	char        tmpIcoName[MAX_PATH];
	const char* sDirName = GetImgDir();
	PBYTE       bufferPtr;
	UINT        bufferLen;

	sprintf(tmpStr, "%s/%s.ico", GetIconsDir(), iconname);
	if (stat(tmpStr, &file_stat) != 0
	|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
	{
		sprintf(tmpStr, "%s/%s.ico", sDirName, iconname);
		if (stat(tmpStr, &file_stat) != 0
		|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
		{
			sprintf(tmpStr, "%s/icons.zip", GetIconsDir());
			sprintf(tmpIcoName, "%s.ico", iconname);
			if (stat(tmpStr, &file_stat) == 0)
			{
				if (load_zipped_file(OSD_FILETYPE_ICON,0,tmpStr, tmpIcoName, &bufferPtr, &bufferLen) == 0)
				{
					hIcon = FormatICOInMemoryToHICON(bufferPtr, bufferLen);
					free(bufferPtr);
				}
			}
		}
	}
	return hIcon;
}

/* Return the number of games currently displayed */
int GetNumGames()
{
	return game_count;
}

/* Sets the treeview and listviews sizes in accordance with their visibility and the splitters */
static void ResizeTreeAndListViews(BOOL bResizeHidden)
{
	int i;
	int nLastWidth = 0;
	int nLastWidth2 = 0;
	int nLeftWindowWidth;
	RECT rect;
	BOOL bVisible;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	/* Tree control */
	ShowWindow(GetDlgItem(hMain, IDC_TREE), bShowTree ? SW_SHOW : SW_HIDE);

	for (i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bVisible = GetWindowLong(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE ? TRUE : FALSE;
		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH/2 - nLastWidth;

			/* special case for the rightmost pane when the screenshot is gone */
			if (!GetShowScreenShot() && !g_splitterInfo[i+1].nSplitterWindow)
				nLeftWindowWidth = rect.right - nLastWidth;

			/* woah?  are we overlapping ourselves? */
			if (nLeftWindowWidth < MIN_VIEW_WIDTH)
			{
				nLastWidth = nLastWidth2;
				nLeftWindowWidth = nSplitterOffset[i] - MIN_VIEW_WIDTH - (SPLITTER_WIDTH*3/2) - nLastWidth;
				i--;
			}

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 2,
				nLeftWindowWidth, (rect.bottom - rect.top) - 4 , TRUE);

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 2,
				SPLITTER_WIDTH, (rect.bottom - rect.top) - 4, TRUE);
		}

		if (bVisible)
		{
			nLastWidth2 = nLastWidth;
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH; 
		}
	}
}

/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot(void)
{
	RECT rect;
	int  nWidth;
	RECT fRect;
	POINT p = {0, 0};

	/* first time through can't do this stuff */
	if (hwndList == NULL)
		return;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowScreenShot())
	{
		nWidth = nSplitterOffset[GetSplitterCount() - 1];
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_CHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_CHECKED);
	}
	else
	{
		nWidth = rect.right;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
	}

	ResizeTreeAndListViews(FALSE);

	FreeScreenShot();

	if (have_selection)
	{
		// load and set image, or empty it if we don't have one
		LoadScreenShot(GetSelectedPickItem(), GetCurrentTab());
	}

	// figure out if we have a history or not, to place our other windows properly
	UpdateHistory();

	// setup the picture area

	if (GetShowScreenShot())
	{
		DWORD dwStyle;
		DWORD dwStyleEx;
		BOOL showing_history;

		ClientToScreen(hMain, &p);
		GetWindowRect(GetDlgItem(hMain, IDC_SSFRAME), &fRect);
		OffsetRect(&fRect, -p.x, -p.y);

		// show history on this tab IFF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		showing_history = (have_history && (GetCurrentTab() == GetHistoryTab() || GetHistoryTab() == TAB_ALL ) &&
						   GetShowTab(TAB_HISTORY) == FALSE);
		CalculateBestScreenShotRect(GetDlgItem(hMain, IDC_SSFRAME), &rect,showing_history);
			
		dwStyle   = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_STYLE);
		dwStyleEx = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_EXSTYLE);
			
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);
		MoveWindow(GetDlgItem(hMain, IDC_SSPICTURE),
				   fRect.left  + rect.left,
				   fRect.top   + rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top,
				   TRUE);
		
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),
				   (GetCurrentTab() != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_SHOW);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),bShowTabCtrl ? SW_SHOW : SW_HIDE);

		InvalidateRect(GetDlgItem(hMain,IDC_SSPICTURE),NULL,FALSE);
	}
	else
	{
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),SW_HIDE);
	}

}

void ResizePickerControls(HWND hWnd)
{
	RECT frameRect;
	RECT rect, sRect;
	int  nListWidth, nScreenShotWidth;
	static BOOL firstTime = TRUE;
	int  doSSControls = TRUE;
	int i, nSplitterCount;

	nSplitterCount = GetSplitterCount();

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc the display sizes based on g_splitterInfo */
	if (firstTime)
	{
		RECT rWindow;

		for (i = 0; i < nSplitterCount; i++)
			nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;

		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		/*buttonMargin = (sRect.bottom + 4); */

		firstTime = FALSE;
	}
	else
	{
		doSSControls = GetShowScreenShot();
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, TRUE);

	ResizeTreeAndListViews(TRUE);

	nListWidth = nSplitterOffset[nSplitterCount-1];
	nScreenShotWidth = (rect.right - nListWidth) - 4;

	/* Screen shot Page tab control */
	if (bShowTabCtrl)
	{
		MoveWindow(GetDlgItem(hWnd, IDC_SSTAB), nListWidth + 4, rect.top + 2,
			nScreenShotWidth - 2, rect.top + 20, doSSControls);
		rect.top += 20;
	}

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2,
		nScreenShotWidth - 2, (rect.bottom - rect.top) - 4, doSSControls);

	/* The screen shot controls */
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);

	/* Text control - game history */
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + (nScreenShotWidth - 22);

	
	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 14;
		sRect.bottom = (rect.bottom - rect.top) - 30;   
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + 264;
		sRect.bottom = (rect.bottom - rect.top) - 278;
	}

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY),
		sRect.left, sRect.top,
		sRect.right - sRect.left, sRect.bottom, doSSControls);

	/* the other screen shot controls will be properly placed in UpdateScreenshot() */

}

char *ModifyThe(const char *str)
{
	static int  bufno = 0;
	static char buffer[4][255];

	if (strncmp(str, "The ", 4) == 0)
	{
		char *s, *p;
		char temp[255];

		strcpy(temp, &str[4]);

		bufno = (bufno + 1) % 4;

		s = buffer[bufno];

		/* Check for version notes in parens */
		p = strchr(temp, '(');
		if (p)
		{
			p[-1] = '\0';
		}

		strcpy(s, temp);
		strcat(s, ", The");

		if (p)
		{
			strcat(s, " ");
			strcat(s, p);
		}

		return s;
	}
	return (char *)str;
}

HBITMAP GetBackgroundBitmap(void)
{
	return hBackground;
}

HPALETTE GetBackgroundPalette(void)
{
	return hPALbg;
}

MYBITMAPINFO * GetBackgroundInfo(void)
{
	return &bmDesc;
}

int GetMinimumScreenShotWindowWidth(void)
{
	BITMAP bmp;
	GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

	return bmp.bmWidth + 6; // 6 is for a little breathing room
}


int GetDriverIndex(const struct GameDriver *driver)
{
	return GetGameNameIndex(driver->name);
}

int GetGameNameIndex(const char *name)
{
    driver_data_type *driver_index_info;
	driver_data_type key;
	key.name = name;

	// uses our sorted array of driver names to get the index in log time
	driver_index_info = bsearch(&key,sorted_drivers,game_count,sizeof(driver_data_type),
								DriverDataCompareFunc);

	if (driver_index_info == NULL)
		return -1;

	return driver_index_info->index;

}

int GetIndexFromSortedIndex(int sorted_index)
{
	return sorted_drivers[sorted_index].index;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

// used for our sorted array of game names
int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2)
{
    return strcmp( ((driver_data_type *)arg1)->name, ((driver_data_type *)arg2)->name );
}

static void ResetTabControl(void)
{
	TC_ITEM tci;
	int i;

	TabCtrl_DeleteAllItems(hTabCtrl);

	ZeroMemory(&tci, sizeof(TC_ITEM));

	tci.mask = TCIF_TEXT;
	tci.cchTextMax = 20;

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (GetShowTab(i))
		{
			tci.pszText = (char *)GetImageTabLongName(i);
			TabCtrl_InsertItem(hTabCtrl, i, &tci);
		}
	}
}

static void ResetBackground(char *szFile)
{
	char szDestFile[MAX_PATH];

	/* The MAME core load the .png file first, so we only need replace this file */
	sprintf(szDestFile, "%s\\bkground.png", GetBgDir());
	SetFileAttributes(szDestFile, FILE_ATTRIBUTE_NORMAL);
	CopyFileA(szFile, szDestFile, FALSE);
}

static void RandomSelectBackground(void)
{
	struct _finddata_t c_file;
	long hFile;
	char szFile[MAX_PATH];
	int count=0;
	const char *szDir=GetBgDir();
	char *buf=malloc(_MAX_FNAME * MAX_BGFILES);

	if (buf == NULL)
		return;

	sprintf(szFile, "%s\\*.bmp", szDir);
	hFile = _findfirst(szFile, &c_file);
	if (hFile != -1L)
	{
		int Done = 0;
		while (!Done && count < MAX_BGFILES)
		{
			memcpy(buf + count * _MAX_FNAME, c_file.name, _MAX_FNAME);
			count++;
			Done = _findnext(hFile, &c_file);
		}
		_findclose(hFile);
	}
	sprintf(szFile, "%s\\*.png", szDir);
	hFile = _findfirst(szFile, &c_file);
	if (hFile != -1L)
	{
		int Done = 0;
		while (!Done && count < MAX_BGFILES)
		{
			memcpy(buf + count * _MAX_FNAME, c_file.name, _MAX_FNAME);
			count++;
			Done = _findnext(hFile, &c_file);
		}
		_findclose(hFile);
	}

	if (count)
	{
		srand( (unsigned)time( NULL ) );
		sprintf(szFile, "%s\\%s", szDir, buf + (rand() % count) * _MAX_FNAME);
		ResetBackground(szFile);
	}

	free(buf);
}

void SetMainTitle(void)
{
	char version[50];
	char buffer[100];

	sscanf(build_version,"%s",version);
	sprintf(buffer,"%s %s",MAME32NAME,version);
	SetWindowText(hMain,buffer);
}

static BOOL Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS	wndclass;
	RECT		rect;
	int i, nSplitterCount;
	extern FOLDERDATA g_folderData[];
	extern FILTER_ITEM g_filterList[];
	extern const char g_szHistoryFileName[];
	extern const char g_szMameInfoFileName[];
	extern const char *history_filename;
	extern const char *mameinfo_filename;
	LONG common_control_version = GetCommonControlVersion();

	CreateBackgroundMain(hInstance, TRUE);

	srand((unsigned)time(NULL));

	game_count = 0;
	while (drivers[game_count] != 0)
		game_count++;

	/* custom per-game icons */
	icon_index = malloc(sizeof(int) * game_count);
	ZeroMemory(icon_index,sizeof(int) * game_count);

	/* none of games are "found" */
	games_found = malloc(sizeof(char) * game_count);
	ZeroMemory(games_found,sizeof(char) * game_count);

	/* sorted list of drivers by name */
	sorted_drivers = (driver_data_type *)malloc(sizeof(driver_data_type) * game_count);
    for (i=0;i<game_count;i++)
    {
        sorted_drivers[i].name = drivers[i]->name;
        sorted_drivers[i].index = i;
    }
    qsort(sorted_drivers,game_count,sizeof(driver_data_type),DriverDataCompareFunc );

	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MIM32_ICON));
	wndclass.hCursor	   = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = "MainClass";

	RegisterClass(&wndclass);

	InitCommonControls();

	// Are we using an Old comctl32.dll?
	dprintf("common controlversion %i %i",common_control_version >> 16,
			common_control_version & 0xffff);
			 
	oldControl = (common_control_version < PACKVERSION(4,71));
	xpControl = (common_control_version >= PACKVERSION(6,0));
	if (oldControl)
	{
		char buf[] = MAME32NAME " has detected an old version of comctl32.dll\n\n"
					 "Game Properties, many configuration options and\n"
					 "features are not available without an updated DLL\n\n"
					 "Please install the common control update found at:\n\n"
					 "http://www.microsoft.com/msdownload/ieplatform/ie/comctrlx86.asp\n\n"
					 "Would you like to continue without using the new features?\n";

		if (IDNO == MessageBox(0, buf, MAME32NAME " Outdated comctl32.dll Warning", MB_YESNO | MB_ICONWARNING))
			return FALSE;
    }

	dprintf("about to init options");
	if (!OptionsInit())
		return FALSE;
	dprintf("options loaded");

	g_mame32_message = RegisterWindowMessage(MAME32NAME);
	g_bDoBroadcast = GetBroadcast();

	HelpInit();

	strcpy(last_directory,GetInpDir());
	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);
	SetMainTitle();
	hTabCtrl = GetDlgItem(hMain, IDC_SSTAB);
	ResetTabControl();

	/* subclass history window */
	g_lpHistoryWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_WNDPROC, (LONG)HistoryWndProc);

	/* subclass picture frame area */
	g_lpPictureFrameWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_SSFRAME), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_SSFRAME), GWL_WNDPROC, (LONG)PictureFrameWndProc);

	/* subclass picture area */
	g_lpPictureWndProc = (WNDPROC)(LONG)(int)GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_WNDPROC);
	SetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_WNDPROC, (LONG)PictureWndProc);

	/* Load the pic for the default screenshot. */
	hMissing_bitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT));

	/* Stash hInstance for later use */
	hInst = hInstance;

	hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	/* In order to handle 'Large Fonts' as the Windows
	 * default setting, we need to make the dialogs small
	 * enough to fit in our smallest window size with
	 * large fonts, then resize the picker, tab and button
	 * controls to fill the window, no matter which font
	 * is currently set.  This will still look like bad
	 * if the user uses a bigger default font than 125%
	 * (Large Fonts) on the Windows display setting tab.
	 *
	 * NOTE: This has to do with Windows default font size
	 * settings, NOT our picker font size.
	 */

	GetClientRect(hMain, &rect);

	hTreeView = GetDlgItem(hMain, IDC_TREE);
	hwndList  = GetDlgItem(hMain, IDC_LIST);

	history_filename = g_szHistoryFileName;
	mameinfo_filename = g_szMameInfoFileName;

	if (!InitSplitters())
		return FALSE;

	nSplitterCount = GetSplitterCount();
	for (i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd;
		HWND hWndLeft;
		HWND hWndRight;

		hWnd = GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow);
		hWndLeft = GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow);
		hWndRight = GetDlgItem(hMain, g_splitterInfo[i].nRightWindow);

		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hMain);

	/* Reset the font */
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		hFont = CreateFontIndirect(&logfont);
		if (hFont != NULL)
		{
			if (hwndList != NULL)
			{
				HWND    hwndHeaderCtrl  = NULL;
				HFONT   hHeaderCtrlFont = NULL;

				hwndHeaderCtrl = GetDlgItem(hwndList, 0);
				if (hwndHeaderCtrl)
				{
					hHeaderCtrlFont = GetWindowFont( hwndHeaderCtrl);
				}

    			SetWindowFont(hwndList, hFont, FALSE);

				/* Reset header ctrl font back to original font */

				if (hHeaderCtrlFont)
				{
					SetWindowFont(hwndHeaderCtrl, hHeaderCtrlFont, TRUE);
				}
			}

			if (hTreeView != NULL)
			{
				SetWindowFont(hTreeView, hFont, FALSE);
			}

			SetWindowFont(GetDlgItem(hMain, IDC_HISTORY), hFont, FALSE);
		}
	}

	TabCtrl_SetCurSel(hTabCtrl, CalculateCurrentTabIndex());

	bDoGameCheck = GetGameCheck();
	idle_work    = TRUE;
	game_index   = 0;

	bShowTree      = GetShowFolderList();
	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl   = GetShowTabCtrl();

	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);

	bSortTree      = GetSortTree();
	bPwdEnabled    = GetPassword(NULL);
  	bPwdVerified   = FALSE;
  	bUseLockGUI    = TRUE;

	if ( ! CreateAndLoadLockUnlockList() )
	{
		// Create a default list with locked games
		CreateDefaultLockUnlockList();

#if (ALL_GAMES_LOCKED == FALSE)
		MessageBox(GetMainWindow(), "Locked games list is either missing or corrupt.\nAll games will be considered as unlocked." , MAME32NAME, MB_OK | MB_ICONWARNING);
#else
		MessageBox(GetMainWindow(), "Unlocked games list is either missing or corrupt.\nAll games will be considered as locked." , MAME32NAME, MB_OK | MB_ICONWARNING);
#endif
	}

	if (oldControl)
	{
		EnableMenuItem(GetMenu(hMain), ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_GAME_PROPERTIES,	MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	/* Init DirectDraw */
	if (!DirectDraw_Initialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	if (GetRandomBackground())
		RandomSelectBackground();

	LoadBackgroundBitmap();

	dprintf("about to init tree");
	InitTree(g_folderData, g_filterList);
	dprintf("did init tree");
	
	PropertiesInit();

	/* Initialize listview columns */
	InitListView();
	SetFocus(hwndList);

	/* Init DirectInput */
	if (!DirectInputInitialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	AdjustMetrics();
	UpdateScreenShot();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TAB_KEYS));

	/* clear keyboard state */
	KeyboardStateClear();

	for (i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		InputSeq *is1;
		InputSeq *is2;
		is1 = &(GUISequenceControl[i].is);
		is2 = GUISequenceControl[i].getiniptr();
		seq_copy(is1, is2);
		//dprintf("seq =%s is: %4i %4i %4i %4i\n",GUISequenceControl[i].name, (*is1)[0], (*is1)[1], (*is1)[2], (*is1)[3]);
	}

	if (GetJoyGUI() == TRUE)
	{
		g_pJoyGUI = &DIJoystick;
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	if (GetHideMouseOnStartup())
	{    
		/*  For some reason the mouse is centered when a game is exited, which of
			course causes a WM_MOUSEMOVE event that shows the mouse. So we center
			it now, before the startup coords are initilized, and that way the mouse
			will still be hidden when exiting from a game (i hope) :)
		*/      
		SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2);
		
		// Then hide it
		ShowCursor(FALSE);
	}

	dprintf("about to show window");

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
	{
		nCmdShow = SW_RESTORE;
	}

	if (GetRunFullScreen())
	{ 
		LONG lMainStyle;

		// Remove menu
		SetMenu(hMain,NULL); 

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		nCmdShow = SW_MAXIMIZE;
	}

	// Hide splash screen
	ShowWindow(hBackMain, SW_HIDE);

	ShowWindow(hMain, nCmdShow);

	
	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS :
		SetView(ID_VIEW_LARGE_ICON,LVS_ICON);
		break;
	case VIEW_SMALL_ICONS :
		SetView(ID_VIEW_SMALL_ICON,LVS_SMALLICON);
		break;
	case VIEW_INLIST :
		SetView(ID_VIEW_LIST_MENU,LVS_LIST);
		break;
	case VIEW_REPORT :
		SetView(ID_VIEW_DETAIL,LVS_REPORT);
		break;
	case VIEW_GROUPED :
	default :
		SetView(ID_VIEW_GROUPED,LVS_REPORT);
		break;
	}

	if (GetCycleScreenshot() > 0)
	{
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to Seconds
	}

	Shell_NotifyIcon( NIM_ADD, &MameIcon );

	return TRUE;
}

static void Win32UI_exit()
{
    Shell_NotifyIcon( NIM_DELETE, &MameIcon );

	CreateBackgroundMain(hInst, FALSE);

    if (g_bDoBroadcast == TRUE)
    {
        ATOM a = GlobalAddAtom("");
        SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
        GlobalDeleteAtom(a);
    }

	if (g_pJoyGUI != NULL)
		g_pJoyGUI->exit();

	/* Free GDI resources */

	if (hMissing_bitmap)
	{
		DeleteObject(hMissing_bitmap);
		hMissing_bitmap = NULL;
	}

	if (hBackground)
	{
		DeleteObject(hBackground);
		hBackground = NULL;
	}
	
	if (hPALbg)
	{
		DeleteObject(hPALbg);
		hPALbg = NULL;
	}
	
	if (hFont)
	{
		DeleteObject(hFont);
		hFont = NULL;
	}
	
	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	if (icon_index != NULL)
	{
		free(icon_index);
		icon_index = NULL;
	}

	if (games_found != NULL)
	{
		free(games_found);
		games_found = NULL;
	}

	DirectInputClose();
	DirectDraw_Close();

	SaveAndDestroyLockUnlockList();

	SetSavedFolderID(GetCurrentFolderID());

	SaveOptions();

	FreeFolders();
	SetViewMode(current_view_id - ID_VIEW_LARGE_ICON);

    /* DestroyTree(hTreeView); */

	FreeScreenShot();

	OptionsExit();

	HelpExit();
}

static long WINAPI MameWindowProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	MINMAXINFO	*mminfo;
	int 		i;

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
		if (hBackground && (HWND)lParam == GetDlgItem(hMain, IDC_HISTORY))
		{
			static HBRUSH hBrush=0;
			HDC hDC=(HDC)wParam;
			LOGBRUSH lb;

			if (hBrush)
				DeleteObject(hBrush);
			lb.lbStyle  = BS_HOLLOW;
			hBrush = CreateBrushIndirect(&lb);
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetListFontColor());
			return (LRESULT) hBrush;
		}
		break;

	case WM_INITDIALOG:
		/* Initialize info for resizing subitems */
		GetClientRect(hWnd, &main_resize.rect);
		return TRUE;

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETTINGCHANGE:
		AdjustMetrics();
		return 0;

	case WM_SIZE:
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(wParam));
		return TRUE;

	case WM_MENUSELECT:
		return Statusbar_MenuSelect(hWnd, wParam, lParam);

	case MM_PLAY_GAME:
		MamePlayGame();
		return TRUE;

	case WM_INITMENUPOPUP:
		UpdateMenu(GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if (HandleTreeContextMenu(hWnd, wParam, lParam)
		||	HandleScreenShotContextMenu(hWnd, wParam, lParam)
		||	HandleContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;

	case WM_COMMAND:
		return MameCommand(hWnd,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

	case WM_GETMINMAXINFO:
		/* Don't let the window get too small; it can break resizing */
		mminfo = (MINMAXINFO *) lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case JOYGUI_TIMER:
			PollGUIJoystick();
			break;
		case SCREENSHOT_TIMER:
			CalculateNextTab();
			UpdateScreenShot();
			TabCtrl_SetCurSel(hTabCtrl, CalculateCurrentTabIndex());
			break;
		default:
			break;
		}
		return TRUE;

	case WM_CLOSE:
		{
			/* save current item */
			RECT rect;
			AREA area;
			int widths[COLUMN_MAX];
			int order[COLUMN_MAX];
			int shown[COLUMN_MAX];
			int tmpOrder[COLUMN_MAX];
			int nItem;
			int nColumnMax = 0;
			WINDOWPLACEMENT wndpl;
			UINT state;

			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hMain, &wndpl);
			state = wndpl.showCmd;

			/* Restore the window before we attempt to save parameters,
			 * This fixed the lost window on startup problem, among other problems
			 */
			if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED || state == SW_MAXIMIZE)
			{
				if( wndpl.flags & WPF_RESTORETOMAXIMIZED || state == SW_MAXIMIZE)
					state = SW_MAXIMIZE;
				else
				{
					state = SW_RESTORE;
					ShowWindow(hWnd, SW_RESTORE);
				}
			}
			for (i = 0; i < GetSplitterCount(); i++)
				SetSplitterPos(i, nSplitterOffset[i]);
			SetWindowState(state);

			GetColumnOrder(order);
			GetColumnShown(shown);
			GetColumnWidths(widths);

			// switch the list view to LVS_REPORT style so column widths reported correctly
			SetWindowLong(hwndList,GWL_STYLE,
						  (GetWindowLong(hwndList, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_REPORT);

			nColumnMax = GetNumColumns(hwndList);

			if (oldControl)
			{
				for (i = 0; i < nColumnMax; i++)
				{
					widths[GetRealColumnFromViewColumn(i)] = ListView_GetColumnWidth(hwndList, i);
				}
			}
			else
			{
				/* Get the Column Order and save it */
				ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

				for (i = 0; i < nColumnMax; i++)
				{
					widths[GetRealColumnFromViewColumn(i)] = ListView_GetColumnWidth(hwndList, i);
					order[i] = GetRealColumnFromViewColumn(tmpOrder[i]);
				}
			}

			SetColumnWidths(widths);
			SetColumnOrder(order);

			GetWindowRect(hWnd, &rect);
			area.x		= rect.left;
			area.y		= rect.top;
			area.width	= rect.right  - rect.left;
			area.height = rect.bottom - rect.top;
			SetWindowArea(&area);

			/* Save the users current game options and default game */
			nItem = GetSelectedPickItem();
			SetDefaultGame(ModifyThe(drivers[nItem]->name));

			/* hide window to prevent orphan empty rectangles on the taskbar */
			/* ShowWindow(hWnd,SW_HIDE); */
            DestroyWindow( hWnd );

			/* PostQuitMessage(0); */
			break;
		}

	case WM_DESTROY:
        PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

		/*
		  Check to see if the mouse has been moved by the user since
		  startup. I'd like this checking to be done only in the
		  main WinProc (here), but somehow the WM_MOUSEDOWN messages
		  are eaten up before they reach MameWindowProc. That's why
		  there is one check for each of the subclassed windows too.
    
		  POSSIBLE BUGS:
		  I've included this check in the subclassed windows, but a 
		  mose move in either the title bar, the menu, or the
		  toolbar will not generate a WM_MOUSEOVER message. At least
		  not one that I know how to pick up. A solution could maybe
		  be to subclass those too, but that's too much work :)
		*/
		
	case WM_MOUSEMOVE:
	{
		if (MouseHasBeenMoved())
			ShowCursor(TRUE);
		
		if (g_listview_dragging)
			MouseMoveListViewDrag(MAKEPOINTS(lParam));
		else
			/* for splitters */
			OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;
	}

	case WM_LBUTTONUP:
	    if (g_listview_dragging)
		    ButtonUpListViewDrag(MAKEPOINTS(lParam));
		else
		   /* for splitters */
		   OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_NOTIFY:
		/* Where is this message intended to go */
		{
			LPNMHDR lpNmHdr = (LPNMHDR)lParam;

			/* Fetch tooltip text */
			if (lpNmHdr->code == TTN_NEEDTEXT)
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				CopyToolTipText(lpttt);
			}
			if (lpNmHdr->hwndFrom == hwndList)
				return MamePickerNotify(lpNmHdr);
			if (lpNmHdr->hwndFrom == hTreeView)
				return TreeViewNotify(lpNmHdr);
			if (lpNmHdr->hwndFrom == hTabCtrl)
				return TabNotify(lpNmHdr);
			
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
			switch (lpDis->CtlID)
			{
			case IDC_LIST:
				DrawItem(lpDis);
				break;
			}
		}
		break;

	default:

		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: /* List-View controls use this message for searching the items "as user types" */
			//MessageBox(NULL,"wm_char message arrived","TitleBox",MB_OK);
			return TRUE;

		case WM_KEYDOWN:
			KeyboardKeyDown(0, wParam, lParam);
			return TRUE;

		case WM_KEYUP:
			KeyboardKeyUp(0, wParam, lParam);
			return TRUE;

		case WM_SYSKEYDOWN:
			KeyboardKeyDown(1, wParam, lParam);
			return TRUE;

		case WM_SYSKEYUP:
			KeyboardKeyUp(1, wParam, lParam);
			return TRUE;
	}

	return FALSE;	/* message not processed */
}

static BOOL PumpMessage()
{
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
    {
		return FALSE;
    }

	if (IsWindow(hMain))
	{
		BOOL absorbed_key = FALSE;
		if (GetKeyGUI())
			absorbed_key = HandleKeyboardGUIMessage(msg.hwnd, msg.message, 
													msg.wParam, msg.lParam);
		else
			absorbed_key = TranslateAccelerator(hMain, hAccel, &msg);

		if (!absorbed_key)
		{
			if (!IsDialogMessage(hMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return TRUE;
}

static void SortListView(void)
{
	LV_FINDINFO lvfi;

	ListView_SortItems(hwndList, ListCompareFunc, GetSortColumn());

	ResetHeaderSortIcon();

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = GetSelectedPickItem();
	ListView_EnsureVisible(hwndList, ListView_FindItem(hwndList, -1, &lvfi), FALSE);
}

static BOOL FolderCheck(void)
{
	
	char *pDescription = NULL;
	int nGameIndex = 0;
	int i=0;
	int iStep = 0;
	LV_FINDINFO lvfi;
	int nCount = ListView_GetItemCount(hwndList);
	BOOL changed = FALSE;

	MSG msg;
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		SetRomAuditResults(nGameIndex, UNKNOWN);
		SetSampleAuditResults(nGameIndex, UNKNOWN);
	}
	if( nCount > 0)
		ProgressBarShow();
	else
		return FALSE;
	if( nCount < 100 )
		iStep = 100 / nCount;
	else
		iStep = nCount/100;
	UpdateListView();
	UpdateWindow(hMain);
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		if (GetRomAuditResults(nGameIndex) == UNKNOWN)
		{
			Mame32VerifyRomSet(nGameIndex);
			changed = TRUE;
		}

		if (GetSampleAuditResults(nGameIndex) == UNKNOWN)
		{
			Mame32VerifySampleSet(nGameIndex);
			changed = TRUE;
		}

		lvfi.flags	= LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		i = ListView_FindItem(hwndList, -1, &lvfi);
		if (changed && i != -1);
		{
			ListView_RedrawItems(hwndList, i, i);
			while( PeekMessage( &msg, hwndList, 0, 0, PM_REMOVE ) != 0)
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			}
		}
		changed = FALSE;
		if ((i % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	pDescription = ModifyThe(drivers[GetSelectedPickItem()]->description);
	SetStatusBarText(0, pDescription);
	UpdateStatusBar();
	return TRUE;
}

static BOOL GameCheck(void)
{
	LV_FINDINFO lvfi;
	int i;
	BOOL changed = FALSE;

	if (game_index == 0)
		ProgressBarShow();

	if (game_index >= game_count)
	{
		bDoGameCheck = FALSE;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		return FALSE;
	}

	if (GetRomAuditResults(game_index) == UNKNOWN)
	{
		Mame32VerifyRomSet(game_index);
		changed = TRUE;
	}

	if (GetSampleAuditResults(game_index) == UNKNOWN)
	{
		Mame32VerifySampleSet(game_index);
		changed = TRUE;
	}

	lvfi.flags	= LVFI_PARAM;
	lvfi.lParam = game_index;

	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (changed && i != -1);
		ListView_RedrawItems(hwndList, i, i);
	if ((game_index % progBarStep) == 0)
		ProgressBarStep();
	game_index++;

	return changed;
}

static void OnIdle()
{
	static int bFirstTime = TRUE;
	static int bResetList = TRUE;

	char *pDescription;
	int driver_index;

	if (bFirstTime)
	{
		bResetList = FALSE;
		bFirstTime = FALSE;
	}
	if (bDoGameCheck)
	{
		bResetList |= GameCheck();
		return;
	}
	// NPW 17-Jun-2003 - Commenting this out because it is redundant
	// and it causes the game to reset back to the original game after an F5 
	// refresh
	//driver_index = GetGameNameIndex(GetDefaultGame());
	//SetSelectedPickItem(driver_index);

	// in case it's not found, get it back
	driver_index = GetSelectedPickItem();

	pDescription = ModifyThe(drivers[driver_index]->description);
	SetStatusBarText(0, pDescription);
	idle_work = FALSE;
	UpdateStatusBar();
	bFirstTime = TRUE;
}

static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static BOOL firstTime = TRUE;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (firstTime == FALSE)
		OnSizeSplitter(hMain);
	//firstTime = FALSE;
	/* Update the splitters structures as appropriate */
	RecalcSplitters();
	if (firstTime == FALSE)
		ResizePickerControls(hMain);
	firstTime = FALSE;
	UpdateScreenShot();
}

static void ResizeWindow(HWND hParent, Resize *r)
{
	int cmkindex = 0, dx, dy, dx1, dtempx;
	HWND hControl;
	RECT parent_rect, rect;
	ResizeItem *ri;
	POINT p = {0, 0};

	if (hParent == NULL)
		return;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	//dx = parent_rect.right - r->rect.right;
	dtempx = parent_rect.right - r->rect.right;
	dy = parent_rect.bottom - r->rect.bottom;
	dx = dtempx/2;
	dx1 = dtempx - dx;
	ClientToScreen(hParent, &p);

	while (r->items[cmkindex].type != RA_END)
	{
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;

		MoveWindow(hControl, rect.left, rect.top,
				   (rect.right - rect.left),
				   (rect.bottom - rect.top), TRUE);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow != NULL)
			ResizeWindow(hControl, ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}

static void ProgressBarShow()
{
	RECT rect;
	int  widths[2] = {150, -1};

	if (game_count < 100)
		progBarStep = 100 / game_count;
	else
		progBarStep = (game_count / 100);

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, game_count));
	SendMessage(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);

	StatusBar_GetItemRect(hStatusBar, 1, &rect);

	MoveWindow(hProgWnd, rect.left, rect.top,
			   rect.right - rect.left,
			   rect.bottom - rect.top, TRUE);

	bProgressShown = TRUE;
}

static void ProgressBarHide()
{
	RECT rect;
	int  widths[4];
	HDC  hDC;
	SIZE size;
	int  numParts = 4;

	if (hProgWnd == NULL)
	{
		return;
	}

    hDC = GetDC(hProgWnd);

	ShowWindow(hProgWnd, SW_HIDE);

	GetTextExtentPoint32(hDC, "MMX", 3 , &size);
	widths[3] = size.cx;
	GetTextExtentPoint32(hDC, "MMMM games", 10, &size);
	widths[2] = size.cx;
	//Just specify 24 instead of 30, gives us sufficient space to display the message, and saves some space
	GetTextExtentPoint32(hDC, "Screen flip support is missing", 24, &size);
	widths[1] = size.cx;

	ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
	UpdateStatusBar();

	bProgressShown = FALSE;
}

static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top, TRUE);
	}
	else
	{
		ProgressBarHide();
	}
}

static void ProgressBarStepParam(int iGameIndex, int nGameCount)
{
	SetStatusBarTextF(0, "Game search %d%% complete",
			((iGameIndex + 1) * 100) / nGameCount);
	if (iGameIndex == 0)
		ShowWindow(hProgWnd, SW_SHOW);
	SendMessage(hProgWnd, PBM_STEPIT, 0, 0);
}

static void ProgressBarStep()
{
	ProgressBarStepParam(game_index, game_count);
}

static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return CreateWindowEx(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			"Progress Bar",
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right	- rect.left,
			rect.bottom - rect.top,
			hParent,
			NULL,
			hInst,
			NULL);
}

static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int   i;
	int   iButton = lpttt->hdr.idFrom;
	static char String[1024];
	BOOL bConverted = FALSE;
	//LPSTR pDest = lpttt->lpszText;
	/* Map command ID to string index */
	for (i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = TRUE;
			break;
		}
	}
	if( bConverted )
	{
		/* Check for valid parameter */
		if(iButton > NUM_TOOLTIPS)
		{
			strcpy(String,"Invalid Button Index");
		}
		else
		{
			strcpy(String,szTbStrings[iButton]);
		}
	}
	else if ( iButton <= 2 )
	{
		//Statusbar (part #0, #1 and #2)
	    SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 140);
		if( iButton != 1)
			SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)iButton, (LPARAM)(LPSTR) &String );
		else
			//for first pane we get the Status directly, to get the line breaks
			strcpy(String, GameInfoStatus(GetSelectedPickItem(), FALSE) );
	}
	else if ( iButton == 3 )
	{
		//Statusbar (part #3)
		if ( bPwdEnabled )
		{
			if ( bPwdVerified )
			{
				strcpy(String, "Password is enabled and verified");
			}
			else
			{
			    strcpy(String, "Password is enabled but not verified");
			}
		}
	}
	else
		strcpy(String,"Invalid Button Index");

	//strcpy(pDest, (LPCTSTR)&String);
	lpttt->lpszText = String;
}

static HWND InitToolbar(HWND hParent)
{
	return CreateToolbarEx(hParent,
						   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						   CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						   1,
						   8,
						   hInst,
						   IDB_TOOLBAR,
						   tbb,
						   NUM_TOOLBUTTONS,
						   16,
						   16,
						   0,
						   0,
						   sizeof(TBBUTTON));
}

static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_UI_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
							  CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS,
							  "Ready",
							  hParent,
							  2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags	= (UINT)HIWORD(wParam);
	HMENU hMainMenu = NULL;
	int   iMenu 	= 0;

	/* Handle non-system popup menu descriptions. */
	if (  (fuFlags & MF_POPUP)
	&&	(!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		/* Display helpful text in status bar */
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst,
				 hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst,
				 hStatusBar, &nZero);
	}

	return 0;
}

static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int 		 games_shown = 0;
	int 		 i = -1;

	if (!lpFolder)
		return;

	while (1)
	{
		i = FindGame(lpFolder,i+1);
		if (i == -1)
			break;

		if (!GameFiltered(i, lpFolder->m_dwFlags))
			games_shown++;
	}

	/* Show number of games in the current 'View' in the status bar */
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	i = GetSelectedPickItem();

	if (games_shown == 0)
	{
		DisableSelection();
	}
	else
	{
		const char* pStatus = GameInfoStatus(i, FALSE);
		SetStatusBarText(1, pStatus);

		// Game state
		if ( !GameIsLocked(i) && !bPwdEnabled )
		{
		    	SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)NULL);
		}
		else
		{
			HICON PwdIcon = ImageList_GetIcon(hPwdIconList, (GameIsLocked(i)?(bPwdEnabled?0:2):1), ILD_TRANSPARENT);
			SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)PwdIcon);
		}

		UpdateStatusBarIcons(i);
	}
}

static void UpdateStatusBarIcons(int game)
{
	HICON PwdIcon;
	
	// Game state
	if (game == -1)
	{
    	SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)NULL);
	}
	else
	{
		if ( !GameIsLocked(game) && !bPwdEnabled )
		{
	    	SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)NULL);
		}
		else
		{
			PwdIcon = ImageList_GetIcon(hPwdIconList, (GameIsLocked(game)?(bPwdEnabled?0:2):1), ILD_TRANSPARENT);
   	 	SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)PwdIcon);
		}
	}

	// Password state
	if ( bPwdEnabled )
	{
		PwdIcon = ImageList_GetIcon(hPwdIconList, (bPwdVerified?1:0), ILD_TRANSPARENT);
    	SendMessage(hStatusBar, SB_SETICON, (WPARAM)3, (LPARAM)(HICON)PwdIcon);
	}
	else
	{
    	SendMessage(hStatusBar, SB_SETICON, (WPARAM)3, (LPARAM)(HICON)NULL);
	}
}

static void UpdateHistory(void)
{
	LOGFONT font;
	HDC hDC;
	RECT rect;
	TEXTMETRIC     tm ;
	int nLines, nLineHeight;
	//DWORD dwStyle = GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = FALSE;

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(GetSelectedPickItem());

		have_history = (histText && histText[0]) ? TRUE : FALSE;
		Edit_SetText(GetDlgItem(hMain, IDC_HISTORY), histText);
	}

	if (have_history && GetShowScreenShot()
		&& ((GetCurrentTab() == TAB_HISTORY) || 
			(GetCurrentTab() == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ))
	{
		Edit_GetRect(GetDlgItem(hMain, IDC_HISTORY),&rect);
		nLines = Edit_GetLineCount(GetDlgItem(hMain, IDC_HISTORY) );
		GetListFont( &font);
		hDC = GetDC(GetDlgItem(hMain, IDC_HISTORY));
		GetTextMetrics (hDC, &tm);
		nLineHeight = tm.tmHeight - tm.tmInternalLeading;
		if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) )
		{
			//more than one Page, so show Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY), SB_VERT, 0, nLines, TRUE); 
		}
		else
		{
			//hide Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY),SB_VERT, 0, 0, TRUE);

		}
 		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_SHOW);
	}
	else
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_HIDE);

}


static void DisableSelection()
{
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);
	BOOL			prev_have_selection = have_selection;

	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = (char *)"&Play";
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES,	   MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_LOCK, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_UNLOCK, MF_GRAYED);
	EnableMenuItem(hMenu, ID_VIEW_PCBINFO, MF_GRAYED);

	SetStatusBarText(0, "No Selection");
	SetStatusBarText(1, "");
	SetStatusBarText(3, "");
	SendMessage(hStatusBar, SB_SETICON, (WPARAM)1, (LPARAM)(HICON)NULL);

	UpdateStatusBarIcons(-1);

	have_selection = FALSE;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();

	sprintf(MameIcon.szTip, MAME32NAME);
	Shell_NotifyIcon( NIM_MODIFY, &MameIcon );
}

static void EnableSelection(int nGame)
{
	char			buf[200];
	const char *	pText;
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);

	snprintf(buf, sizeof(buf) / sizeof(buf[0]), g_szPlayGameString, ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));
	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	pText = ModifyThe(drivers[nGame]->description);
	SetStatusBarText(0, pText);
	/* Add this game's status to the status bar */
	pText = GameInfoStatus(nGame, FALSE);
	SetStatusBarText(1, pText);
	SetStatusBarText(3, "");

	UpdateStatusBarIcons(nGame);

	/* If doing updating game status and the game name is NOT pacman.... */

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_ENABLED);
	EnableMenuItem(hMenu, ID_GAME_LOCK, MF_ENABLED);
	EnableMenuItem(hMenu, ID_GAME_UNLOCK, MF_ENABLED);
	EnableMenuItem(hMenu, ID_VIEW_PCBINFO, MF_ENABLED);

	if (!oldControl)
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	if (bProgressShown && bListReady == TRUE)
	{
		SetDefaultGame(ModifyThe(drivers[nGame]->name));
	}

	sprintf(MameIcon.szTip, "%s [%.50s]", MAME32NAME, ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));
	Shell_NotifyIcon( NIM_MODIFY, &MameIcon );

	have_selection = TRUE;

	UpdateScreenShot();
}

void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;
	HDC 		hDC = GetDC(hWnd);
	int 		i, j;
	HDC 		htempDC;
	HBITMAP 	oldBitmap;

	/* x and y are offsets within the background image that should be at 0,0 in hWnd */

	/* So we don't paint over the control's border */
	GetClientRect(hWnd, &rcClient);

	htempDC = CreateCompatibleDC(hDC);
	oldBitmap = SelectObject(htempDC, hBackground);

	if (hRgn == NULL)
	{
		/* create a region of our client area */
		rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);
	}
	else
	{
		/* use the passed in region */
		SelectClipRgn(hDC, hRgn);
	}

	hPAL = GetBackgroundPalette();
	if (hPAL == NULL)
		hPAL = CreateHalftonePalette(hDC);

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, FALSE);
		RealizePalette(htempDC);
	}

	for (i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);
	
	SelectObject(htempDC, oldBitmap);
	DeleteDC(htempDC);
	
	if (GetBackgroundPalette() == NULL)
	{
		DeleteObject(hPAL);
		hPAL = NULL;
	}

	ReleaseDC(hWnd, hDC);
}

static LPCSTR GetCloneParentName(int nItem)
{
	if (DriverIsClone(nItem) == TRUE)
		return ModifyThe(drivers[nItem]->clone_of->description);
	return "";
}

static BOOL PickerHitTest(HWND hWnd)
{
	RECT			rect;
	POINTS			p;
	DWORD			res = GetMessagePos();
	LVHITTESTINFO	htInfo;

    ZeroMemory(&htInfo,sizeof(LVHITTESTINFO));
	p = MAKEPOINTS(res);
	GetWindowRect(hWnd, &rect);
	htInfo.pt.x = p.x - rect.left;
	htInfo.pt.y = p.y - rect.top;
	ListView_HitTest(hWnd, &htInfo);

	return (! (htInfo.flags & LVHT_NOWHERE));
}

static BOOL MamePickerNotify(NMHDR *nm)
{
	NM_LISTVIEW *pnmv;
	static int nLastItem  = -1;

	switch (nm->code)
	{
	case NM_RCLICK:
	case NM_CLICK:
		/* don't allow selection of blank spaces in the listview */
		if (!PickerHitTest(hwndList))
		{
			/* we have no current game selected */
			if (nLastItem != -1)
				SetSelectedPickItem(nLastItem);
			return TRUE;
		}
		break;

	case NM_DBLCLK:
		/* Check here to make sure an item was selected */
		if (!PickerHitTest(hwndList))
		{
			if (nLastItem != -1);
				SetSelectedPickItem(nLastItem);
			return TRUE;
		}
		else
			MamePlayGame();
		return TRUE;

	case LVN_GETDISPINFO:
		{
			LV_DISPINFO* pDispInfo = (LV_DISPINFO*)nm;
			int nItem = pDispInfo->item.lParam;

			if (pDispInfo->item.mask & LVIF_IMAGE)
			{
			   pDispInfo->item.iImage = GetIconForDriver(nItem);
			}

			if (pDispInfo->item.mask & LVIF_STATE)
				pDispInfo->item.state = 0;

			if (pDispInfo->item.mask & LVIF_TEXT)
			{
				switch (GetRealColumnFromViewColumn(pDispInfo->item.iSubItem))
				{
				case COLUMN_GAMES:
					/* Driver description */
					pDispInfo->item.pszText = (char *)ModifyThe(drivers[nItem]->description);
					break;

				case COLUMN_ROMS:
					/* Has Roms */
					pDispInfo->item.pszText = (char *)GetAuditString(GetRomAuditResults(nItem));
					break;

				case COLUMN_SAMPLES:
					/* Samples */
					if (DriverUsesSamples(nItem))
					{
						pDispInfo->item.pszText = 
							(char *)GetAuditString(GetSampleAuditResults(nItem));
					}
					else
					{
						pDispInfo->item.pszText = (char *)"";
					}
					break;

				case COLUMN_DIRECTORY:
					/* Driver name (directory) */
					pDispInfo->item.pszText = (char*)drivers[nItem]->name;
					break;

                case COLUMN_SRCDRIVERS:
					/* Source drivers */
					pDispInfo->item.pszText = (char *)GetDriverFilename(nItem);
					break;

                case COLUMN_PLAYTIME:
					/* Source drivers */
					GetTextPlayTime(nItem, pDispInfo->item.pszText);
					break;

				case COLUMN_TYPE:
                {
                    struct InternalMachineDriver drv;
                    expand_machine_driver(drivers[nItem]->drv,&drv);

					/* Vector/Raster */
					if (drv.video_attributes & VIDEO_TYPE_VECTOR)
						pDispInfo->item.pszText = (char *)"Vector";
					else
						pDispInfo->item.pszText = (char *)"Raster";
					break;
                }
				case COLUMN_TRACKBALL:
					/* Trackball */
					if (DriverUsesTrackball(nItem))
						pDispInfo->item.pszText = (char *)"Yes";
					else
						pDispInfo->item.pszText = (char *)"No";
					break;

				case COLUMN_PLAYED:
					/* times played */
					{
						static char buf[100];
						sprintf(buf, "%i", GetPlayCount(nItem));
						pDispInfo->item.pszText = buf;
					}
					break;

				case COLUMN_MANUFACTURER:
					/* Manufacturer */
					pDispInfo->item.pszText = (char *)drivers[nItem]->manufacturer;
					break;

				case COLUMN_YEAR:
					/* Year */
					pDispInfo->item.pszText = (char *)drivers[nItem]->year;
					break;

				case COLUMN_CLONE:
					pDispInfo->item.pszText = (char *)GetCloneParentName(nItem);
					break;

				}
			}
		}
		return TRUE;

	case LVN_ITEMCHANGED :
		pnmv = (NM_LISTVIEW *)nm;

		if ( (pnmv->uOldState & LVIS_SELECTED)
		&&	!(pnmv->uNewState & LVIS_SELECTED))
		{
			if (pnmv->lParam != -1)
				nLastItem = pnmv->lParam;
			/* leaving item */
			/* printf("leaving %s\n",drivers[pnmv->lParam]->name); */
		}

		if (!(pnmv->uOldState & LVIS_SELECTED)
		&&	 (pnmv->uNewState & LVIS_SELECTED))
		{
			/* printf("entering %s\n",drivers[pnmv->lParam]->name); */
			if (g_bDoBroadcast == TRUE)
			{
				ATOM a = GlobalAddAtom(drivers[pnmv->lParam]->description);
				SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
				GlobalDeleteAtom(a);
			}

			EnableSelection(pnmv->lParam);
		}
		return TRUE;

	case LVN_COLUMNCLICK :
		pnmv = (NM_LISTVIEW *)nm;
		// if clicked on the same column we're sorting by, reverse it
		if (GetSortColumn() == GetRealColumnFromViewColumn(pnmv->iSubItem))
			SetSortReverse(!GetSortReverse());
		else
			SetSortReverse(FALSE);
		SetSortColumn(GetRealColumnFromViewColumn(pnmv->iSubItem));
		SortListView();
		return TRUE;

	case LVN_BEGINDRAG :
		pnmv = (NM_LISTVIEW *)nm;
		BeginListViewDrag(pnmv);
		break;

	}
	return FALSE;
}

static BOOL TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
	case TVN_SELCHANGED :
	{
		HTREEITEM hti = TreeView_GetSelection(hTreeView);
		TVITEM	  tvi;

		tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
		tvi.hItem = hti;
		
		if (TreeView_GetItem(hTreeView, &tvi))
		{
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			if (bListReady)
			{
				ResetListView();
				UpdateScreenShot();
			}
		}
		return TRUE;
	}
	case TVN_BEGINLABELEDIT :
	{
		TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
		LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

		if (folder->m_dwFlags & F_CUSTOM)
		{
			// user can edit custom folder names
			g_in_treeview_edit = TRUE;
			return FALSE;
		}
		// user can't edit built in folder names
		return TRUE;
	}
	case TVN_ENDLABELEDIT :
	{
		TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
		LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

		g_in_treeview_edit = FALSE;

		if (ptvdi->item.pszText == NULL || strlen(ptvdi->item.pszText) == 0)
			return FALSE;

		return TryRenameCustomFolder(folder,ptvdi->item.pszText);
	}
	}
	return FALSE;
}

static int GetTabFromTabIndex(int tab_index)
{
	int shown_tabs = -1;
	int i;

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (GetShowTab(i))
		{
			shown_tabs++;
			if (shown_tabs == tab_index)
				return i;
		}
	}
	dprintf("invalid tab index %i",tab_index);
	return 0;
}

static int CalculateCurrentTabIndex(void)
{
	int shown_tabs = 0;
	int i;

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (GetCurrentTab() == i)
			break;

		if (GetShowTab(i))
			shown_tabs++;

	}
	
	return shown_tabs;
}

static void CalculateNextTab(void)
{
	int i;

	// at most loop once through all options
	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		SetCurrentTab((GetCurrentTab() + 1) % MAX_TAB_TYPES);

		if (GetShowTab(GetCurrentTab()))
		{
			// this tab is being shown, so we're all set
			return;
		}
	}
}

static BOOL TabNotify(NMHDR *nm)
{
	switch (nm->code)
	{
	case TCN_SELCHANGE:
		SetCurrentTab(GetTabFromTabIndex(TabCtrl_GetCurSel(hTabCtrl)));
		UpdateScreenShot();
		return TRUE;
	}
	return FALSE;
}

static BOOL HeaderOnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	/* Right button was clicked on header */
	POINT	pt,client_pt;
	RECT	rcCol;
	int 	cmkindex;
	int 	i;
	BOOL	found = FALSE;
	HWND	hwndHeader;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_LIST))
		return FALSE;

	hwndHeader = ListView_GetHeader(hwndList);

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	client_pt = pt;
	ScreenToClient(hwndHeader, &client_pt);

	/* Determine the column index */
	for (i = 0; Header_GetItemRect(hwndHeader, i, &rcCol); i++)
	{
		if (PtInRect(&rcCol, client_pt))
		{
			cmkindex = i;
			found = TRUE;
			break;
		}
	}

	/* Do the context menu */
	if (found)
	{
		hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
		hMenu = GetSubMenu(hMenuLoad, 0);
		lastColumnClick = i;
		TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

		DestroyMenu(hMenuLoad);

		return TRUE;
	}
	return FALSE;
}

char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
	   for setting text of window controls that don't allow us to disable
	   the ampersand underlining.
	 */
	/* returns a static buffer--use before calling again */

	static char buf[200];
	char *ptr;

	ptr = buf;
	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;
		*ptr++ = *s++;
	}
	*ptr = 0;

	return buf;
}

static int GUI_seq_pressed(InputSeq* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<SEQ_MAX;++j)
	{
		switch ((*code)[j])
		{
			case CODE_NONE :
				return res && count;
			case CODE_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case CODE_NOT :
				invert = !invert;
				break;
			default:
				if (res)
				{
					int pressed = keyboard_state[(*code)[j]];
					if ((pressed != 0) == invert)
						res = 0;
				}
				invert = 0;
				++count;
		}
	}
	return res && count;
}

static void check_for_GUI_action(void)
{
	int i;

	for (i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		InputSeq *is = &(GUISequenceControl[i].is);

		if (GUI_seq_pressed(is))
		{
			dprintf("seq =%s pressed\n", GUISequenceControl[i].name);
			switch (GUISequenceControl[i].func_id)
			{
			case ID_GAME_AUDIT:
			case ID_GAME_PROPERTIES:
			case ID_CONTEXT_FILTERS:
			case ID_UI_START:
				KeyboardStateClear(); /* beacuse whe won't receive KeyUp mesage when we loose focus */
				break;
			default:
				break;
			}
			SendMessage(hMain, WM_COMMAND, GUISequenceControl[i].func_id, 0);
		}
	}
}

static void KeyboardStateClear(void)
{
	int i;

	for (i = 0; i < __code_key_last; i++)
		keyboard_state[i] = 0;

	dprintf("keyboard gui state cleared.\n");
}


static void KeyboardKeyDown(int syskey, int vk_code, int special)
{
	int i, found = 0;
	InputCode icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_RALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_RCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_RSHIFT;
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_LALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_LCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_LSHIFT;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < __code_key_last; i++)
		{
			if ( vk_code == key_trans_table[i][VIRTUAL_KEY])
			{
				icode = key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code pressed not found =  %i\n",vk_code);
		//MessageBox(NULL,"keydown message arrived not processed","TitleBox",MB_OK);
		return;
	}
	dprintf("VK_code pressed found =  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special);
	keyboard_state[icode] = 1;
	check_for_GUI_action();
}

static void KeyboardKeyUp(int syskey, int vk_code, int special)
{
	int i, found = 0;
	InputCode icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_RALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_RCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_RSHIFT;
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = KEYCODE_LALT;
				break;
			case VK_CONTROL:
				icode = KEYCODE_LCONTROL;
				break;
			case VK_SHIFT:
				icode = KEYCODE_LSHIFT;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < __code_key_last; i++)
		{
			if (vk_code == key_trans_table[i][VIRTUAL_KEY])
			{
				icode = key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code released not found =  %i",vk_code);
		//MessageBox(NULL,"keyup message arrived not processed","TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = 0;
	dprintf("VK_code released found=  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special );
	check_for_GUI_action();
}

static void PollGUIJoystick()
{
	// For the exec timer, will keep track of how long the button has been pressed  
	static int exec_counter = 0;
  
	if (in_emulation)
		return;

	if (g_pJoyGUI == NULL)
		return;

	g_pJoyGUI->poll_joysticks();


	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2),GetUIJoyUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_UP, 0);
	}
	
	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2),GetUIJoyDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_DOWN, 0);
	}
	
	// User pressed LEFT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2),GetUIJoyLeft(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_LEFT, 0);
	}

	// User pressed RIGHT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2),GetUIJoyRight(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_RIGHT, 0);
	}

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2),GetUIJoyStart(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_START, 0);
	}

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2),GetUIJoyPageUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGUP, 0);
	}

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2),GetUIJoyPageDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);
	}

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2),GetUIJoyHome(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HOME, 0);
	}

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2),GetUIJoyEnd(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_END, 0);
	}

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2),GetUIJoySSChange(3))))
	{
		SendMessage(hMain, WM_COMMAND, IDC_SSFRAME, 0);
	}
	
	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2),GetUIJoyHistoryUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);
	}
	
	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2),GetUIJoyHistoryDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);
	}
  
  // User pressed EXECUTE COMMANDLINE
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyExec(0), GetUIJoyExec(1), GetUIJoyExec(2),GetUIJoyExec(3))))
	{
		if (++exec_counter >= GetExecWait()) // Button has been pressed > exec timeout
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			
			// Reset counter
			exec_counter = 0;

			ZeroMemory( &si, sizeof(si) );
			ZeroMemory( &pi, sizeof(pi) );
			si.dwFlags = STARTF_FORCEONFEEDBACK;
			si.cb = sizeof(si);
			
			CreateProcess(NULL, GetExecCommand(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else
	{
		// Button has been released within the timeout period, reset the counter
		exec_counter = 0;
	}
}

static void PressKey(HWND hwnd, UINT vk)
{
	SendMessage(hwnd, WM_KEYDOWN, vk, 0);
	SendMessage(hwnd, WM_KEYUP,   vk, 0xc0000000);
}

static void SetView(int menu_id, int listview_style)
{
	BOOL force_reset = FALSE;

	// first uncheck previous menu item, check new one
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_LARGE_ICON, ID_VIEW_GROUPED, menu_id, MF_CHECKED);
	ToolBar_CheckButton(hToolBar, menu_id, MF_CHECKED);

	SetWindowLong(hwndList, GWL_STYLE,
				  (GetWindowLong(hwndList, GWL_STYLE) & ~LVS_TYPEMASK) | listview_style );

	if (current_view_id == ID_VIEW_GROUPED || menu_id == ID_VIEW_GROUPED)
	{
		// this changes the sort order, so redo everything
		force_reset = TRUE;
	}

	current_view_id = menu_id;
	SetViewMode(menu_id - ID_VIEW_LARGE_ICON);

	if (force_reset)
		ResetListView();

	if ( (GetViewMode() == VIEW_REPORT) || (GetViewMode() == VIEW_GROUPED) )
		EnableMenuItem(GetMenu(hMain), ID_FILE_GAMELIST, MF_ENABLED);
	else
		EnableMenuItem(GetMenu(hMain), ID_FILE_GAMELIST, MF_GRAYED);
}

static void ResetListView()
{
	int 	i;
	int 	current_game;
	LV_ITEM lvi;
	BOOL	no_selection = FALSE;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (!lpFolder)
    {
		return;
    }

	/* If the last folder was empty, no_selection is TRUE */
	if (have_selection == FALSE)
    {
		no_selection = TRUE;
    }

	current_game = GetSelectedPickItem();

	SetWindowRedraw(hwndList,FALSE);

	ListView_DeleteAllItems(hwndList);

	// hint to have it allocate it all at once
	ListView_SetItemCount(hwndList,game_count);

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;

	i = -1;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem	 = i;
			lvi.iSubItem = 0;
			lvi.lParam	 = i;
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iImage	 = I_IMAGECALLBACK;
			ListView_InsertItem(hwndList, &lvi);
		}
	} while (i != -1);

	SortListView();

	if (bListReady)
	{
	    /* If last folder was empty, select the first item in this folder */
	    if (no_selection)
		    SetSelectedPick(0);
		else
		    SetSelectedPickItem(current_game);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
		 We now need to set the ViewMode for SmallIcon again,
		 for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_SMALL_ICONS)
		SetView(ID_VIEW_SMALL_ICON,LVS_SMALLICON);

	SetWindowRedraw(hwndList, TRUE);

	UpdateStatusBar();

}

static void UpdateGameList()
{
	int i;

	for (i = 0; i < game_count; i++)
	{
		SetRomAuditResults(i, UNKNOWN);
		SetSampleAuditResults(i, UNKNOWN);
	}

	idle_work	 = TRUE;
	bDoGameCheck = TRUE;
	game_index	 = 0;

	ReloadIcons();

	// Let REFRESH also load new background if found
	LoadBackgroundBitmap();
	InvalidateRect(hMain,NULL,TRUE);
	ResetListView();
}

static void PickFont(void)
{
	LOGFONT font;
	CHOOSEFONT cf;

	GetListFont(&font);
	font.lfQuality = DEFAULT_QUALITY;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.rgbColors   = GetListFontColor();
	cf.Flags	   = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		DeleteObject(hFont);
	hFont = CreateFontIndirect(&font);
	if (hFont != NULL)
	{
        HWND    hwndHeaderCtrl = NULL;
        HFONT   hHeaderCtrlFont = NULL;

		COLORREF textColor = cf.rgbColors;

		if (textColor == RGB(255,255,255))
        {
			textColor = RGB(240, 240, 240);
        }

        hwndHeaderCtrl = GetDlgItem( hwndList, 0 );
        if ( hwndHeaderCtrl )
        {
            hHeaderCtrlFont = GetWindowFont( hwndHeaderCtrl );
        }

		SetWindowFont(hwndList, hFont, TRUE);
		SetWindowFont(hTreeView, hFont, TRUE);

        /* Reset header ctrl font back to original font */

        if ( hHeaderCtrlFont )
        {
            SetWindowFont( hwndHeaderCtrl, hHeaderCtrlFont, TRUE );
        }

		ListView_SetTextColor(hwndList, textColor);
		TreeView_SetTextColor(hTreeView,textColor);
		SetListFontColor(cf.rgbColors);
		SetWindowFont(GetDlgItem(hMain, IDC_HISTORY), hFont, FALSE);
		ResetListView();
	}
}

static void PickCloneColor(void)
{
	CHOOSECOLOR cc;
	COLORREF choice_colors[16];
	int i;

	for (i=0;i<16;i++)
		choice_colors[i] = RGB(0,0,0);

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner   = hMain;
	cc.rgbResult   = GetListCloneColor();
	cc.lpCustColors = choice_colors;
	cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
	if (!ChooseColor(&cc))
		return;

	SetListCloneColor(cc.rgbResult);
	InvalidateRect(hwndList,NULL,FALSE);
}

static BOOL MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	int i;
	LPTREEFOLDER folder;
	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD:
		MamePlayRecordGame();
		return TRUE;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
		return TRUE;

	case ID_FILE_LOADSTATE :
		MameLoadState();
		return TRUE;

	case ID_FILE_AUDIT:
		AuditDialog(hMain);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_GAMELIST:
	{
	   	BOOL bIsTrue = FALSE;
		OPENFILENAMEA OpenFileName;
		char szFile[MAX_PATH] = "\0";
		char szCurDir[MAX_PATH] = "\1";

                // Save current directory (avoids mame file creation further failure)
		if ( GetCurrentDirectory(MAX_PATH, szCurDir) > MAX_PATH )
		{
		   // Path too large
		   szCurDir[0] = 0;
		}

		OpenFileName.lStructSize       = sizeof(OPENFILENAME);
		OpenFileName.hwndOwner         = hMain;
		OpenFileName.hInstance         = 0;
		OpenFileName.lpstrFilter       = "Formatted text file (*.txt, *.*)\0*.txt;*.*\0Tabuled text file (*.txt, *.*)\0*.txt;*.*\0";
		OpenFileName.lpstrCustomFilter = NULL;
		OpenFileName.nMaxCustFilter    = 0;
		OpenFileName.nFilterIndex      = 1;
		OpenFileName.lpstrFile         = szFile;
		OpenFileName.nMaxFile          = sizeof(szFile);
		OpenFileName.lpstrFileTitle    = NULL;
		OpenFileName.nMaxFileTitle     = 0;
		OpenFileName.lpstrInitialDir   = NULL;
		OpenFileName.lpstrTitle        = "Pick a file name to save the game list";
		OpenFileName.nFileOffset       = 0;
		OpenFileName.nFileExtension    = 0;
		OpenFileName.lpstrDefExt       = NULL;
		OpenFileName.lCustData         = 0;
		OpenFileName.lpfnHook          = NULL;
		OpenFileName.lpTemplateName    = NULL;
		OpenFileName.Flags             = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;


		while( ! bIsTrue )
		{
			if ( GetOpenFileNameA(&OpenFileName) )
			{
				if ( GetFileAttributes(szFile) != -1 )
				{
					if ( MessageBox(GetMainWindow(), "File already exists, overwrite ?" , MAME32NAME, MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK )
						continue;
					else
						bIsTrue = TRUE;
	
					SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
				}
	
				SaveGameListToFile(szFile, (OpenFileName.nFilterIndex==2 ? 0 : 1));
	    
				bIsTrue = TRUE;
			}
			else
				break;
		}

		// Restore current file path
		if ( szCurDir[0] != 0 )
			SetCurrentDirectory(szCurDir);
		
		if ( bIsTrue )
			return TRUE;

		break;
	}

	case ID_FILE_FIND_GAME:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FIND_GAME), hMain, FindGameDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_EXIT:
		PostMessage(hMain, WM_CLOSE, 0, 0);
		return TRUE;

	case ID_VIEW_LARGE_ICON:
		SetView(ID_VIEW_LARGE_ICON, LVS_ICON);
		return TRUE;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON, LVS_SMALLICON);
		ResetListView();
		return TRUE;

	case ID_VIEW_LIST_MENU:
		SetView(ID_VIEW_LIST_MENU, LVS_LIST);
		return TRUE;

	case ID_VIEW_DETAIL:
		SetView(ID_VIEW_DETAIL, LVS_REPORT);
		return TRUE;

	case ID_VIEW_GROUPED:
		SetView(ID_VIEW_GROUPED, LVS_REPORT);
		return TRUE;

	/* Arrange Icons submenu */
	case ID_VIEW_BYGAME:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_GAMES);
		SortListView();
		break;

	case ID_VIEW_BYDIRECTORY:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_DIRECTORY);
		SortListView();
		break;

	case ID_VIEW_BYMANUFACTURER:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_MANUFACTURER);
		SortListView();
		break;

	case ID_VIEW_BYTIMESPLAYED:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_PLAYED);
		SortListView();
		break;

	case ID_VIEW_BYTYPE:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_TYPE);
		SortListView();
		break;

	case ID_VIEW_BYYEAR:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_YEAR);
		SortListView();
		break;

	case ID_VIEW_FOLDERS:
		bShowTree = !bShowTree;
		SetShowFolderList(bShowTree);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_PAGETAB:
		bShowTabCtrl = !bShowTabCtrl;
		SetShowTabCtrl(bShowTabCtrl);
		ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		InvalidateRect(hMain,NULL,TRUE);
		break;

		/*
		  Switches to fullscreen mode. No check mark handeling 
		  for this item cause in fullscreen mode the menu won't
		  be visible anyways.
		*/    
	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_SORT_TREE:
	{
		int current_id = GetCurrentFolderID();
		
		bSortTree = !bSortTree;
		SetSortTree(bSortTree);
//		CheckMenuItem(GetMenu(hMain), ID_SORT_TREE, (bSortTree) ? MF_CHECKED : MF_UNCHECKED);

		SetWindowRedraw(hwndList,FALSE);

		ResetTreeViewFolders();
		SelectTreeViewFolder(current_id);

		SetWindowRedraw(hwndList,TRUE);
		
		break;
	}

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (!oldControl)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItem(), GetSelectedPickItemIcon(), AUDIT_PAGE, -1, SRC_GAME);
			SaveGameOptions(GetSelectedPickItem());
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
	   break;

	case ID_GAME_LOCK:
		SetGameLockUnlock(GetSelectedPickItem(), TRUE);
		GameIsLocked(-1);
		UpdateStatusBarIcons(GetSelectedPickItem());
		ResetListView();
		break;

	case ID_GAME_UNLOCK:
		if ( bPwdVerified || !bPwdEnabled )
		{
			SetGameLockUnlock(GetSelectedPickItem(), FALSE);
			GameIsLocked(-1);
			UpdateStatusBarIcons(GetSelectedPickItem());
			ResetListView();
		}
		else
		{
			MessageBox(GetMainWindow(), "Password must be verified or disabled to unlock this game.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION);
		}
		break;

	case ID_FOLDER_LOCK:
		SetFolderLockUnlock(GetCurrentFolder(), TRUE);
		GameIsLocked(-1);
		UpdateStatusBarIcons(GetSelectedPickItem());
		break;

	case ID_FOLDER_UNLOCK:
		if ( bPwdVerified || !bPwdEnabled )
		{
			SetFolderLockUnlock(GetCurrentFolder(), FALSE);
			GameIsLocked(-1);
			UpdateStatusBarIcons(GetSelectedPickItem());
		}
		else
		{
			MessageBox(GetMainWindow(), "Password must be verified or disabled to unlock folder games.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION);
		}
		break;

	/* ListView Context Menu */
	case ID_CONTEXT_ADD_CUSTOM:
	{
	    int  nResult;

		nResult = DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CUSTOM_FILE),
								 hMain,AddCustomFileDialogProc,GetSelectedPickItem());
		SetFocus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
	    RemoveCurrentGameCustomFolder();
		break;
	}

	/* Tree Context Menu */
	case ID_CONTEXT_FILTERS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == TRUE)
			ResetListView();
		SetFocus(hwndList);
		return TRUE;

		// ScreenShot Context Menu
		// select current tab
	case ID_VIEW_TAB_SCREENSHOT :
	case ID_VIEW_TAB_FLYER :
	case ID_VIEW_TAB_CABINET :
	case ID_VIEW_TAB_MARQUEE :
	case ID_VIEW_TAB_TITLE :
	case ID_VIEW_TAB_CONTROL_PANEL :
	case ID_VIEW_TAB_HISTORY:
		if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == FALSE)
			break;

		SetCurrentTab(id - ID_VIEW_TAB_SCREENSHOT);
		UpdateScreenShot();
		TabCtrl_SetCurSel(hTabCtrl, CalculateCurrentTabIndex());
		break;

	case ID_VIEW_PCBINFO:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PCBINFO), hMain, PCBInfoDialogProc);
		SetFocus(hwndList);
		return TRUE;
		break;

		// toggle tab's existence
	case ID_TOGGLE_TAB_SCREENSHOT :
	case ID_TOGGLE_TAB_FLYER :
	case ID_TOGGLE_TAB_CABINET :
	case ID_TOGGLE_TAB_MARQUEE :
	case ID_TOGGLE_TAB_TITLE :
	case ID_TOGGLE_TAB_CONTROL_PANEL :
	case ID_TOGGLE_TAB_HISTORY :
	{
		int toggle_flag = id - ID_TOGGLE_TAB_SCREENSHOT;

		if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == FALSE)
		{
			// attempt to hide the last tab
			// should show error dialog? hide picture area? or ignore?
			break;
		}

		SetShowTab(toggle_flag,!GetShowTab(toggle_flag));

		ResetTabControl();

		if (GetCurrentTab() == toggle_flag && GetShowTab(toggle_flag) == FALSE)
		{
			// we're deleting the tab we're on, so go to the next one
			CalculateNextTab();
		}


		// Resize the controls in case we toggled to another history
		// mode (and the history control needs resizing).

		ResizePickerControls(hMain);
		UpdateScreenShot();
    
		TabCtrl_SetCurSel(hTabCtrl, CalculateCurrentTabIndex());

		break;
	}

	/* Header Context Menu */
	case ID_SORT_ASCENDING:
		SetSortReverse(FALSE);
		SetSortColumn(GetRealColumnFromViewColumn(lastColumnClick));
		SortListView();
		break;

	case ID_SORT_DESCENDING:
		SetSortReverse(TRUE);
		SetSortColumn(GetRealColumnFromViewColumn(lastColumnClick));
		SortListView();
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == TRUE)
			ResetColumnDisplay(FALSE);
		SetFocus(hwndList);
		return TRUE;

	/* View Menu */
	case ID_VIEW_LINEUPICONS:
		ResetListView();
		break;

	case ID_GAME_PROPERTIES:
		if (!oldControl)
		{
			//folder = GetSelectedFolder();
			folder = GetFolderByName(FOLDER_SOURCE-1, (char*)GetDriverFilename(GetSelectedPickItem()) );
			InitPropertyPage(hInst, hwnd, GetSelectedPickItem(), GetSelectedPickItemIcon(), folder->m_nFolderId, SRC_GAME);
			SaveGameOptions(GetSelectedPickItem());
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_PROPERTIES:
		if (!oldControl)
		{
			folder = GetSelectedFolder();
			InitPropertyPage(hInst, hwnd, folder->m_nFolderId, GetSelectedFolderIcon(), GetSelectedPickItem(), SRC_FOLDER);
			SaveFolderOptions(folder->m_nFolderId, GetSelectedPickItem() );
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;
	case ID_FOLDER_AUDIT:
		FolderCheck();
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_VIEW_PICTURE_AREA :
		ToggleScreenShot();
		break;

	case ID_UPDATE_GAMELIST:
		UpdateGameList();
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		return TRUE;

	case ID_OPTIONS_CLONE_COLOR:
		PickCloneColor();
		return TRUE;

	case ID_OPTIONS_DEFAULTS:
		/* Check the return value to see if changes were applied */
		if (!oldControl)
		{
			InitDefaultPropertyPage(hInst, hwnd);
			SaveDefaultOptions();
		}
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_DIR:
		{
			int  nResult;
			BOOL bUpdateRoms;
			BOOL bUpdateSamples;

			nResult = DialogBox(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_DIRECTORIES),
								hMain,
								DirectoriesDialogProc);

			SaveDefaultOptions();

			bUpdateRoms    = ((nResult & DIRDLG_ROMS)	 == DIRDLG_ROMS)	? TRUE : FALSE;
			bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? TRUE : FALSE;

			/* update game list */
			if (bUpdateRoms == TRUE || bUpdateSamples == TRUE)
				UpdateGameList();

			SetFocus(hwndList);
		}
		return TRUE;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (DialogBox(GetModuleHandle(NULL),
					  MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == TRUE)
        {
			// these may have been changed
			SaveDefaultOptions();
            DestroyWindow(hwnd);
			PostQuitMessage(0);
        }
		return TRUE;

	case ID_OPTIONS_INTERFACE:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INTERFACE_OPTIONS),
				  hMain, InterfaceDialogProc);
		SaveDefaultOptions();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
		{
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds
		}

		return TRUE;

	case ID_OPTIONS_BG:
		DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_BACKGROUND),
				  hMain,
				  BackgroundDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_LANGUAGE:
		DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_LANGUAGE),
				  hMain,
				  LanguageDialogProc);
		return TRUE;

	case ID_OPTIONS_PASSWORD:
		{
			char szPassword[MAX_PWD_LEN+1];

			GetPassword(szPassword);

			if ( szPassword[0] == '\0' )
			{
				MessageBox(GetMainWindow(), "Password is corrupt.\nDefault password value restored.", MAME32NAME, MB_OK | MB_ICONWARNING);
				bPwdVerified = FALSE;
			}
			else
			{
				DialogBox(GetModuleHandle(NULL),
						  MAKEINTRESOURCE(IDD_PASSWORD),
						  hMain,
						  PasswordDialogProc);
		}

		return TRUE;
		}

	case ID_HELP_ABOUT:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT),
				  hMain, AboutDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case IDOK :
		/* cmk -- might need to check more codes here, not sure */
		if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
		{
			/* enter key */
			if (g_in_treeview_edit)
			{
				TreeView_EndEditLabelNow(hTreeView, FALSE);
				return TRUE;
			}
			else 
				if (have_selection)
					MamePlayGame();
		}
		break;

	case IDCANCEL : /* esc key */
		if (g_in_treeview_edit)
			TreeView_EndEditLabelNow(hTreeView, TRUE);
		break;

	case IDC_PLAY_GAME :
		if (have_selection)
			MamePlayGame();
		break;

	case ID_UI_START:
		SetFocus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		SetSelectedPick(GetSelectedPick() - 1);
 		break;

	case ID_UI_DOWN:
		SetSelectedPick(GetSelectedPick() + 1);
 		break;

	case ID_UI_PGUP:
		SetSelectedPick(GetSelectedPick() - ListView_GetCountPerPage(hwndList));
 		break;

	case ID_UI_PGDOWN:
		if ( (GetSelectedPick() + ListView_GetCountPerPage(hwndList)) < ListView_GetItemCount(hwndList) )
			SetSelectedPick( GetSelectedPick() + ListView_GetCountPerPage(hwndList) );
		else
			SetSelectedPick( ListView_GetItemCount(hwndList)-1 );
 		break;

	case ID_UI_HOME:
		SetSelectedPick(0);
 		break;

	case ID_UI_END:
		SetSelectedPick( ListView_GetItemCount(hwndList)-1 );
		break;
	case ID_UI_LEFT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINELEFT, 0);
 		break;

	case ID_UI_RIGHT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINERIGHT, 0);
 		break;
	case ID_UI_HISTORY_UP:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEUP, 0);
		}
 		break;

	case ID_UI_HISTORY_DOWN:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
		}
 		break;

	case IDC_SSFRAME:
		CalculateNextTab();
		UpdateScreenShot();
		TabCtrl_SetCurSel(hTabCtrl, CalculateCurrentTabIndex());
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;

	case ID_CONTEXT_RESET_PLAYTIME:
		ResetPlayTime( GetSelectedPickItem() );
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RESET_PLAYCOUNT:
		ResetPlayCount( GetSelectedPickItem() );
		ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RENAME_CUSTOM :
		TreeView_EditLabel(hTreeView,TreeView_GetSelection(hTreeView));
		break;

	default:
		if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
		{
			ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
			break;
		}
		for (i = 0; g_helpInfo[i].nMenuItem > 0; i++)
		{
			if (g_helpInfo[i].nMenuItem == id)
			{
				if (g_helpInfo[i].bIsHtmlHelp)
					HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
				else
					DisplayTextFile(hMain, g_helpInfo[i].lpFile);
				return FALSE;
			}
		}
		break;
	}

	return FALSE;
}

static void LoadBackgroundBitmap()
{
	HGLOBAL hDIBbg;

	if (hBackground)
	{
		DeleteObject(hBackground);
		hBackground = 0;
	}

	if (hPALbg)
	{
		DeleteObject(hPALbg);
		hPALbg = 0;
	}

	if ( GetBackground() == 0 )
	{
		if (LoadDIB("bkground", &hDIBbg, &hPALbg, BACKGROUND))
		{
			HDC hDC = GetDC(hwndList);
			hBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
			GlobalFree(hDIBbg);
			ReleaseDC(hwndList, hDC);
		}
		else
		{
			MessageBox(GetMainWindow(), "Unable to load background image file.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION );
		}
	}
	else
	{
		// No background available, get default internal background
		HDC hDC = GetDC(hwndList);
		HBITMAP hBMP = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(GetBackground()), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION|LR_SHARED);
		BITMAPINFO *pbmi;
		BITMAPINFOHEADER *pbmih;
		BITMAP bmp;
		WORD cClrBits;

		// Retreive the bitmap infos
		GetObject(hBMP, sizeof(BITMAP), (LPSTR)&bmp);
		
		// Compute the bits per colors
		cClrBits = bmp.bmPlanes * bmp.bmBitsPixel;
		
		// Adjust to the upper common size
		if (cClrBits == 1) 
			cClrBits = 1; 
		else if (cClrBits <= 4) 
			cClrBits = 4; 
		else if (cClrBits <= 8) 
			cClrBits = 8; 
		else if (cClrBits <= 16) 
			cClrBits = 16; 
		else if (cClrBits <= 24) 
			cClrBits = 24; 
		else
			cClrBits = 32; 
		
		// Alocate a buffer to store the bitmap infos
		if ( cClrBits == 24 )
		{
			// There is no palette with 24 bits per pixel (1 byte = 1 color)
			pbmi = (PBITMAPINFO)LocalAlloc( LMEM_FIXED, sizeof(BITMAPINFOHEADER) );
	}
	else
	{
			pbmi = (PBITMAPINFO)LocalAlloc( LMEM_FIXED, sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * (1<< cClrBits)) );
	}

		if ( pbmi != NULL )
	{
			// Gain some code source size
			pbmih = (BITMAPINFOHEADER *)&pbmi->bmiHeader;
			
			// Fill the structure with the appropriate data
			pbmih->biSize = sizeof(BITMAPINFOHEADER);
			pbmih->biWidth = bmp.bmWidth;
			pbmih->biHeight = bmp.bmHeight;
			pbmih->biPlanes = bmp.bmPlanes;
			pbmih->biBitCount = bmp.bmBitsPixel;
			pbmih->biCompression = BI_RGB;
			pbmih->biSizeImage = (pbmih->biWidth * cClrBits) * pbmih->biHeight;
			pbmih->biXPelsPerMeter = 0;
			pbmih->biYPelsPerMeter = 0;
			pbmih->biClrUsed = ( (cClrBits < 24) ? (1 << cClrBits) : 0);
			pbmih->biClrImportant = 0;
			
			// For future use
			bmDesc.bmWidth  = bmp.bmWidth;
			bmDesc.bmHeight = bmp.bmHeight;
			bmDesc.bmColors = (bmp.bmBitsPixel < 24) ? 1 << bmp.bmBitsPixel : 0;
			
			// Get the bitmap color palette
			GetDIBits(hDC, hBMP, 0, bmp.bmHeight, NULL, pbmi, DIB_RGB_COLORS);
			
			// Draw the bitmap
			hBackground = CreateDIBitmap( hDC, pbmih, (LONG)CBM_INIT, bmp.bmBits, pbmi, DIB_RGB_COLORS );
			
			LocalFree(pbmih);
		}
		
		DeleteObject(hBMP);
		ReleaseDC(hwndList, hDC);
	}
}

/* Initialize the Picker and List controls */
static void InitListView()
{
	int order[COLUMN_MAX];
	int shown[COLUMN_MAX];
	int i;

	/* subclass the list view */
	g_lpListViewWndProc = (WNDPROC)(LONG)(int)GetWindowLong(hwndList, GWL_WNDPROC);
	SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListViewWndProc);

	/* Disabled column customize with old Control */
	if (oldControl)
	{
		for (i = 0; i < COLUMN_MAX; i++)
		{
			order[i] = i;
			shown[i] = TRUE;
		}
		SetColumnOrder(order);
		SetColumnShown(shown);
	}

	CreateIcons();
	GetColumnOrder(realColumn);

	ResetColumnDisplay(TRUE);

	// Allow selection to change the default saved game
	bListReady = TRUE;
}

/* Re/initialize the ListControl Columns */
static void ResetColumnDisplay(BOOL first_time)
{
	LV_COLUMN   lvc;
	int         i;
	int         nColumn;
	int         widths[COLUMN_MAX];
	int         order[COLUMN_MAX];
	int         shown[COLUMN_MAX];
	int shown_columns;
	int driver_index;

	GetColumnWidths(widths);
	GetColumnOrder(order);
	GetColumnShown(shown);

	if (!first_time)
	{
		DWORD style = GetWindowLong(hwndList, GWL_STYLE);

		// switch the list view to LVS_REPORT style so column widths reported correctly
		SetWindowLong(hwndList,GWL_STYLE,
					  (GetWindowLong(hwndList, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_REPORT);

		nColumn = GetNumColumns(hwndList);
		// The first time thru this won't happen, on purpose
		// because the column widths will be in the negative millions,
		// since it's been created but not setup properly yet
		for (i = 0; i < nColumn; i++)
		{
			widths[GetRealColumnFromViewColumn(i)] = ListView_GetColumnWidth(hwndList, i);
		}

		SetColumnWidths(widths);

		SetWindowLong(hwndList,GWL_STYLE,style);

		for (i = nColumn; i > 0; i--)
		{
			ListView_DeleteColumn(hwndList, i - 1);
		}
	}

	ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP |
								  LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD | LVS_EX_LABELTIP);


	nColumn = 0;
	for (i = 0; i < COLUMN_MAX; i++)
	{
		if (shown[order[i]])
		{
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
			lvc.pszText = (char *)column_names[order[i]];
			lvc.iSubItem = nColumn;
			lvc.cx = widths[order[i]];
			lvc.fmt = LVCFMT_LEFT;
			ListView_InsertColumn(hwndList, nColumn, &lvc);
			realColumn[nColumn] = order[i];
			nColumn++;
		}
	}
	
	shown_columns = nColumn;

	/* Fill this in so we can still sort on columns NOT shown */
	for (i = 0; i < COLUMN_MAX && nColumn < COLUMN_MAX; i++)
	{
		if (!shown[order[i]])
		{
			realColumn[nColumn] = order[i];
			nColumn++;
		}
	}

	if (GetListFontColor() == RGB(255, 255, 255))
		ListView_SetTextColor(hwndList, RGB(240, 240, 240));
	else
		ListView_SetTextColor(hwndList, GetListFontColor());

	ResetListView();

	driver_index = GetGameNameIndex(GetDefaultGame());
	SetSelectedPickItem(driver_index);
}

static int GetRealColumnFromViewColumn(int view_column)
{
	return realColumn[view_column];
}

static int GetViewColumnFromRealColumn(int real_column)
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
	{
		if (realColumn[i] == real_column)
			return i;
	}

	// major error, shouldn't be possible, but no good way to warn
	return 0;
}


static void AddDriverIcon(int nItem,int default_icon_index)
{
	HICON hIcon = 0;

	/* if already set to rom or clone icon, we've been here before */
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)drivers[nItem]->name);
	if (hIcon == NULL && drivers[nItem]->clone_of != NULL)
	{
		hIcon = LoadIconFromFile((char *)drivers[nItem]->clone_of->name);
		if (hIcon == NULL && drivers[nItem]->clone_of->clone_of != NULL)
			hIcon = LoadIconFromFile((char *)drivers[nItem]->clone_of->clone_of->name);
	}

	if (hIcon != NULL)
	{
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		if (nIconPos != -1)
			icon_index[nItem] = nIconPos;
	}
	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}

static void DestroyIcons(void)
{
	if (hSmall != NULL)
	{
		ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index != NULL)
	{
		int i;
		for (i=0;i<game_count;i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge != NULL)
	{
		ImageList_Destroy(hLarge);
		hLarge = NULL;
	}

	if (hHeaderImages != NULL)
	{
		ImageList_Destroy(hHeaderImages);
		hHeaderImages = NULL;
	}

	if (hPwdIconList != NULL)
	{
		ImageList_Destroy(hPwdIconList);
		hPwdIconList = NULL;
	}

}

static void ReloadIcons(void)
{
	HICON hIcon;
	INT i;

	// clear out all the images
	ImageList_Remove(hSmall,-1);
	ImageList_Remove(hLarge,-1);

	if (icon_index != NULL)
	{
		for (i=0;i<game_count;i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	for (i = 0; g_iconData[i].icon_name; i++)
	{
		hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);
		if (hIcon == NULL)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(g_iconData[i].resource));

		ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
	}
}

static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize, dwLength = 512, dwType = REG_SZ;
	TCHAR  szBuffer[512];
	HKEY   hKey;

	/* Get the Key */
	RegOpenKey(HKEY_CURRENT_USER, "Control Panel\\desktop\\WindowMetrics", &hKey);
	/* Save the last size */
	RegQueryValueEx(hKey, "Shell Icon Size", NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	dwSize = atol(szBuffer);
	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}

static DWORD GetShellSmallIconSize(void)
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize < 48)
	{
		if (dwSize < 32)
			dwSize = 16;
		else
			dwSize = 32;
	}
	else
	{
		dwSize = 48;
	}
	return dwSize;
}

// create iconlist for Listview control
static void CreateIcons(void)
{
	HWND header;
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	int icon_count;
	DWORD dwStyle;

	icon_count = 0;
	while(g_iconData[icon_count].icon_name)
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list
	//
	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	dwStyle = GetWindowLong(hwndList,GWL_STYLE);
	SetWindowLong(hwndList,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(GetShellSmallIconSize(),GetShellSmallIconSize(),
							  ILC_COLORDDB | ILC_MASK, icon_count, 500);
	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize,
							  ILC_COLORDDB | ILC_MASK, icon_count, 500);

	ReloadIcons();

	// Associate the image lists with the list view control.
	ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
	ListView_SetImageList(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	SetWindowLong(hwndList,GWL_STYLE,dwStyle);

	hHeaderImages = ImageList_Create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_UP));
	ImageList_AddIcon(hHeaderImages,hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_DOWN));
	ImageList_AddIcon(hHeaderImages,hIcon);

	header = ListView_GetHeader(hwndList);
	Header_SetImageList(header,hHeaderImages);

	// Icons for password state
	hPwdIconList = ImageList_Create(GetShellSmallIconSize(),GetShellSmallIconSize(), ILC_COLORDDB | ILC_MASK, 2, 2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_LOCK));
	ImageList_AddIcon(hPwdIconList, hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_UNLOCK));
	ImageList_AddIcon(hPwdIconList, hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_KEY));
	ImageList_AddIcon(hPwdIconList, hIcon);

}

/* Sort subroutine to sort the list control */
static int CALLBACK ListCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem)
{
	if (GetViewMode() != VIEW_GROUPED)
		return BasicCompareFunc(index1,index2,sort_subitem);

	/* do our fancy compare, with clones grouped under parents
	 * if games are both parents, basic sort on that
     */
	if (DriverIsClone(index1) == FALSE && DriverIsClone(index2) == FALSE)
	{
		return BasicCompareFunc(index1,index2,sort_subitem);
	}

	/* if both are clones */
	if (DriverIsClone(index1) && DriverIsClone(index2))
	{
		/* same parent? sort on children */
		if (drivers[index1]->clone_of == drivers[index2]->clone_of)
		{
			return BasicCompareFunc(index1,index2,sort_subitem);
		}

		/* different parents, so sort on parents */
		return BasicCompareFunc(GetDriverIndex(drivers[index1]->clone_of),
								GetDriverIndex(drivers[index2]->clone_of),sort_subitem);
	}

	/* one clone, one non-clone */
	if (DriverIsClone(index1))
	{
		/* first one is the clone */

		/* if this is a clone and its parent, put clone after */
		if (drivers[index1]->clone_of == drivers[index2])
			return 1;
		
		/* otherwise, compare clone's parent with #2 */
		return BasicCompareFunc(GetDriverIndex(drivers[index1]->clone_of),index2,sort_subitem);
	}

	/* second one is the clone */

	/* if this is a clone and its parent, put clone after */
	if (drivers[index1] == drivers[index2]->clone_of)
		return -1;
		
	/* otherwise, compare clone's parent with #1 */
	return BasicCompareFunc(index1,GetDriverIndex(drivers[index2]->clone_of),sort_subitem);
}

static int BasicCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem)
{
	int value;
	const char *name1 = NULL;
	const char *name2 = NULL;
	int nTemp1, nTemp2;

#ifdef DEBUG
	if (strcmp(drivers[index1]->name,"1941") == 0 && strcmp(drivers[index2]->name,"1942") == 0)
	{
		dprintf("comparing 1941, 1942");
	}

	if (strcmp(drivers[index1]->name,"1942") == 0 && strcmp(drivers[index2]->name,"1941") == 0)
	{
		dprintf("comparing 1942, 1941");
	}
#endif

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
		value = stricmp(ModifyThe(drivers[index1]->description),
						ModifyThe(drivers[index2]->description));
		break;

	case COLUMN_ROMS:
		nTemp1 = GetRomAuditResults(index1);
		nTemp2 = GetRomAuditResults(index2);

		if (IsAuditResultKnown(nTemp1) == FALSE && IsAuditResultKnown(nTemp2) == FALSE)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		if (IsAuditResultKnown(nTemp1) == FALSE)
		{
			value = 1;
			break;
		}

		if (IsAuditResultKnown(nTemp2) == FALSE)
		{
			value = -1;
			break;
		}

		// ok, both are known

		if (IsAuditResultYes(nTemp1) && IsAuditResultYes(nTemp2))
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);
		
		if (IsAuditResultNo(nTemp1) && IsAuditResultNo(nTemp2))
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		if (IsAuditResultYes(nTemp1) && IsAuditResultNo(nTemp2))
			value = -1;
		else
			value = 1;
		break;

	case COLUMN_SAMPLES:
		nTemp1 = -1;
		if (DriverUsesSamples(index1))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp1 = 1;
				else 
					nTemp1 = 0;
			}
			else
				nTemp1 = 2;
		}

		nTemp2 = -1;
		if (DriverUsesSamples(index2))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp2 = 1;
				else 
					nTemp2 = 0;
			}
			else
				nTemp2 = 2;
		}

		if (nTemp1 == nTemp2)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = nTemp2 - nTemp1;
		break;

	case COLUMN_DIRECTORY:
		value = stricmp(drivers[index1]->name, drivers[index2]->name);
		break;

   	case COLUMN_SRCDRIVERS:
		if (stricmp(drivers[index1]->source_file+12, drivers[index2]->source_file+12) == 0)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = stricmp(drivers[index1]->source_file+12, drivers[index2]->source_file+12);
		break;
	case COLUMN_PLAYTIME:
	   value = GetPlayTime(index1) - GetPlayTime(index2);
	   if (value == 0)
		  return BasicCompareFunc(index1, index2, COLUMN_GAMES);

	   break;

	case COLUMN_TYPE:
    {
        struct InternalMachineDriver drv1,drv2;
        expand_machine_driver(drivers[index1]->drv,&drv1);
        expand_machine_driver(drivers[index2]->drv,&drv2);

		if ((drv1.video_attributes & VIDEO_TYPE_VECTOR) ==
			(drv2.video_attributes & VIDEO_TYPE_VECTOR))
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = (drv1.video_attributes & VIDEO_TYPE_VECTOR) -
				(drv2.video_attributes & VIDEO_TYPE_VECTOR);
		break;
    }
	case COLUMN_TRACKBALL:
		if (DriverUsesTrackball(index1) == DriverUsesTrackball(index2))
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = DriverUsesTrackball(index1) - DriverUsesTrackball(index2);
		break;

	case COLUMN_PLAYED:
	   value = GetPlayCount(index1) - GetPlayCount(index2);
	   if (value == 0)
		  return BasicCompareFunc(index1, index2, COLUMN_GAMES);

	   break;

	case COLUMN_MANUFACTURER:
		if (stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer) == 0)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer);
		break;

	case COLUMN_YEAR:
		if (stricmp(drivers[index1]->year, drivers[index2]->year) == 0)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		value = stricmp(drivers[index1]->year, drivers[index2]->year);
		break;

	case COLUMN_CLONE:
		name1 = GetCloneParentName(index1);
		name2 = GetCloneParentName(index2);

		if (*name1 == '\0')
			name1 = NULL;
		if (*name2 == '\0')
			name2 = NULL;

		if (name1 == name2)
			return BasicCompareFunc(index1, index2, COLUMN_GAMES);

		if (name2 == NULL)
			value = -1;
		else if (name1 == NULL)
			value = 1;
		else
			value = stricmp(name1, name2);
		break;

	default :
		return BasicCompareFunc(index1, index2, COLUMN_GAMES);
	}

	if (GetSortReverse())
		value = -value;

#ifdef DEBUG
	if ((strcmp(drivers[index1]->name,"1941") == 0 && strcmp(drivers[index2]->name,"1942") == 0) ||
		(strcmp(drivers[index1]->name,"1942") == 0 && strcmp(drivers[index2]->name,"1941") == 0))
		dprintf("result: %i",value);
#endif

	return value;
}

static int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}

static int GetSelectedPickItem()
{
	/* returns lParam (game index) of listview selected item */
	LV_ITEM lvi;

	lvi.iItem = GetSelectedPick();
	if (lvi.iItem == -1)
		return 0;

	lvi.iSubItem = 0;
	lvi.mask	 = LVIF_PARAM;
	ListView_GetItem(hwndList, &lvi);

	return lvi.lParam;
}

static HICON GetSelectedPickItemIcon()
{
	LV_ITEM lvi;

	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	ListView_GetItem(hwndList, &lvi);

	return ImageList_GetIcon(hLarge, lvi.iImage, ILD_TRANSPARENT);
}

static void SetSelectedPick(int new_index)
{
	if (new_index < 0)
		new_index = 0;

	ListView_SetItemState(hwndList, new_index, LVIS_FOCUSED | LVIS_SELECTED,
						  LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hwndList, new_index, FALSE);
}

static void SetSelectedPickItem(int val)
{
	int i;
	LV_FINDINFO lvfi;

	if (val < 0)
		return;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = val;
	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (i == -1)
	{
		POINT p = {0,0};
		lvfi.flags = LVFI_NEARESTXY;
		lvfi.pt = p;
		i = ListView_FindItem(hwndList, -1, &lvfi);
	}
	SetSelectedPick((i == -1) ? 0 : i);
}

static void SetRandomPickItem()
{
	int nListCount;

	nListCount = ListView_GetItemCount(hwndList);

	if (nListCount > 0)
	{
		SetSelectedPick(rand() % nListCount);
	}
}

static BOOL CommonFileDialog(common_file_dialog_proc cfd, char *filename, BOOL bZip, BOOL bInp)
{
	BOOL success;
	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hMain;
	of.hInstance         = NULL;
	if( bInp )
	{
		if (bZip == TRUE)
			of.lpstrFilter   = MAMENAME " input files (*.inp,*.zip)\0*.inp;*.zip\0All files (*.*)\0*.*\0";
		else
			of.lpstrFilter   = MAMENAME " input files (*.inp)\0*.inp;\0All files (*.*)\0*.*\0";
	}
	else
	{
		of.lpstrFilter   = MAMENAME " savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0";
	}
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	if( bInp )
		of.lpstrInitialDir   = last_directory;
	else
		of.lpstrInitialDir = GetStateDir();
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	if( bInp )
		of.lpstrDefExt       = "inp";
	else
		of.lpstrDefExt       = "sta";
	of.lpstrDefExt       = NULL;
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	success = cfd(&of);
	if (success)
	{
		/*GetDirectory(filename,last_directory,sizeof(last_directory));*/
	}

	return success;
}

static BOOL SelectLanguageFile(HWND hWnd, TCHAR* filename)
{
	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hWnd;
	of.hInstance         = NULL;
	of.lpstrFilter       = MAMENAME " Language files (*.lng)\0*.lng\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	of.lpstrInitialDir   = NULL;
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	of.lpstrDefExt       = "lng";
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	return GetOpenFileName(&of);
}

static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR pLangFile[MAX_PATH];
	DWORD dwHelpIDs[] = { IDC_LANGUAGECHECK, HIDC_LANGUAGECHECK,
						  IDC_LANGUAGEEDIT,  HIDC_LANGUAGEEDIT,
						  0, 0};

	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			const char* pLang = GetLanguage();
			if (pLang == NULL || *pLang == '\0')
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), FALSE);
			}
			else
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLang);
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), TRUE);
			}

			return TRUE;
		}

	case WM_HELP:
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, (DWORD)dwHelpIDs);
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, (DWORD)dwHelpIDs);
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			SetLanguage(pLangFile);

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		case IDC_LANGUAGECHECK:
		{
			BOOL bCheck = Button_GetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK));
			EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   bCheck);
			EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), bCheck);
			if (bCheck == FALSE)
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
			return TRUE;
		}

		case IDC_LANGUAGEBROWSE:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			if (SelectLanguageFile(hDlg, pLangFile) == TRUE)
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile);
			break;
		}
		break;
	}
	return 0;
}

static INT_PTR CALLBACK PasswordDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_CURRENT),   FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_NEW), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_RETYPE), FALSE);

			Edit_LimitText(GetDlgItem(hDlg, IDC_PWD_CURRENT), MAX_PWD_LEN);
			Edit_LimitText(GetDlgItem(hDlg, IDC_PWD_NEW), MAX_PWD_LEN);
			Edit_LimitText(GetDlgItem(hDlg, IDC_PWD_RETYPE), MAX_PWD_LEN);

			if ( bPwdEnabled )
			{
				Static_SetText(GetDlgItem(hDlg, IDC_PWD_INFOS), "Password is enabled. If you are running " MAME32NAME " for the first time, you should replace the default password with a new one.");
			}
			else
			{
				Static_SetText(GetDlgItem(hDlg, IDC_PWD_INFOS), "Password is disabled." );
			}

			return TRUE;
		}

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
		{
			HICON PwdIcon;

			char szPassword[MAX_PWD_LEN+1];
			char EditPwd[MAX_PWD_LEN+1];
			char NewPwd[MAX_PWD_LEN+1];

			GetPassword(szPassword);

			// Enable password
			if ( Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_PWD)) )
			{
				bPwdEnabled = TRUE;
				SetPassword(NULL, TRUE);
				UpdateStatusBarIcons(GetSelectedPickItem());
				MessageBox(GetMainWindow(), "Password enabled.", MAME32NAME, MB_OK | MB_ICONINFORMATION);
			}
			// Disable password
			else if ( Button_GetCheck(GetDlgItem(hDlg, IDC_DISABLE_PWD)) )
			{
				Edit_GetText(GetDlgItem(hDlg, IDC_PWD_CURRENT), EditPwd, MAX_PWD_LEN);
				
				if ( !bPwdVerified && strcmp(szPassword, EditPwd) )
				{
					if ( strlen(EditPwd) != 0 )
					{
						MessageBox(GetMainWindow(), "Wrong password.\nPassword must be verified before disabled.", MAME32NAME, MB_OK | MB_ICONERROR);
					}
					else
					{
						MessageBox(GetMainWindow(), "Password must be verifed before disabled.", MAME32NAME, MB_OK | MB_ICONWARNING);
					}

					SetFocus(GetDlgItem(hDlg,IDC_PWD_CURRENT));
					return 0;
				}
				else
				{
					bPwdEnabled = FALSE;
					SetPassword(NULL, FALSE);
					UpdateStatusBarIcons(GetSelectedPickItem());
					MessageBox(GetMainWindow(), "Password disabled.", MAME32NAME, MB_OK | MB_ICONINFORMATION);
				}
			}
			// Verify password
			else if ( Button_GetCheck(GetDlgItem(hDlg, IDC_VERIFY_PWD)) )
			{
				Edit_GetText(GetDlgItem(hDlg, IDC_PWD_CURRENT), EditPwd, MAX_PWD_LEN);
				
				if ( (strlen(szPassword) == 0) || strcmp(szPassword, EditPwd) )
				{
					bPwdVerified = FALSE;
					PwdIcon = ImageList_GetIcon(hPwdIconList, 0, ILD_TRANSPARENT);
					MessageBox(GetMainWindow(), "Wrong password.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION);
					SendMessage(hStatusBar, SB_SETICON, (WPARAM)3, (LPARAM)(HICON)PwdIcon);
				}
				else
				{
					bPwdVerified = TRUE;
					PwdIcon = ImageList_GetIcon(hPwdIconList, 1, ILD_TRANSPARENT);
					SendMessage(hStatusBar, SB_SETICON, (WPARAM)3, (LPARAM)(HICON)PwdIcon);
					MessageBox(GetMainWindow(), "Password successful verifed.", MAME32NAME, MB_OK | MB_ICONINFORMATION);
				}
			}
			// Change password
			else if ( Button_GetCheck(GetDlgItem(hDlg, IDC_CHANGE_PWD)) )
			{
				Edit_GetText(GetDlgItem(hDlg, IDC_PWD_CURRENT), EditPwd, MAX_PWD_LEN);
				
				if ( !bPwdVerified && ((strlen(szPassword) == 0) || strcmp(szPassword, EditPwd)) )
				{
					if ( strlen(EditPwd) != 0 )
					{
						MessageBox(GetMainWindow(), "Wrong password.\nPassword must be verified before changed.", MAME32NAME, MB_OK | MB_ICONERROR);
					}
					else
					{
						MessageBox(GetMainWindow(), "Password must be verifed before changed.", MAME32NAME, MB_OK | MB_ICONWARNING);
					}
					SetFocus(GetDlgItem(hDlg,IDC_PWD_CURRENT));
					return 0;
				}
				else
				{
					bPwdVerified = TRUE;
					
					Edit_GetText(GetDlgItem(hDlg, IDC_PWD_NEW), EditPwd, MAX_PWD_LEN);
					Edit_GetText(GetDlgItem(hDlg, IDC_PWD_RETYPE), NewPwd, MAX_PWD_LEN);
					
					if ( (strlen(EditPwd) < 3) || (strlen(NewPwd) < 3) )
					{
						MessageBox(GetMainWindow(), "Password must be at least 3 characters.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_PWD_NEW));
						return 0;
					}
					else if ( strcmp(EditPwd, NewPwd) )
					{
						MessageBox(GetMainWindow(), "New password doesn't match with re-type.", MAME32NAME, MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_PWD_RETYPE));
						return 0;
					}
					
					SetPassword(NewPwd, GetPassword(NULL));
					if ( bPwdEnabled )
					{
						PwdIcon = ImageList_GetIcon(hPwdIconList, 1, ILD_TRANSPARENT);
						SendMessage(hStatusBar, SB_SETICON, (WPARAM)3, (LPARAM)(HICON)PwdIcon);
					}
					MessageBox(GetMainWindow(), "Password successful changed.", MAME32NAME, MB_OK | MB_ICONINFORMATION);
				}
			}
		}

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		case IDC_ENABLE_PWD:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_CURRENT),   FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_NEW), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_RETYPE), FALSE);
			
			break;
		}

		case IDC_DISABLE_PWD:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_CURRENT),   (bPwdVerified?FALSE:TRUE));
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_NEW), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_RETYPE), FALSE);

			if ( ! bPwdVerified )
			{
				SetFocus(GetDlgItem(hDlg,IDC_PWD_CURRENT));
			}

			break;
		}

		case IDC_VERIFY_PWD:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_CURRENT),   TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_NEW), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_RETYPE), FALSE);
			
			SetFocus(GetDlgItem(hDlg,IDC_PWD_CURRENT));
			break;
		}

		case IDC_CHANGE_PWD:
		{
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_CURRENT),   (bPwdVerified?FALSE:TRUE));
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_NEW), TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_PWD_RETYPE), TRUE);
			
			if ( ! bPwdVerified )
			{
				SetFocus(GetDlgItem(hDlg,IDC_PWD_CURRENT));
			}
			else
			{
				SetFocus(GetDlgItem(hDlg,IDC_PWD_NEW));
			}

			break;
		}
		}			
		break;
	}
	return 0;
}

static INT_PTR CALLBACK BackgroundDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static char szFile[MAX_PATH] = "\0";

	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			HBITMAP hBmp;
			
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_THUMB1), IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_BITMAP1), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_THUMB2), IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_BITMAP2), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_THUMB3), IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_BITMAP3), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_THUMB4), IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_BITMAP4), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
						
			EnableWindow(GetDlgItem(hDlg, IDC_BKG_EDIT),   FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_BKG), FALSE);

			Edit_LimitText(GetDlgItem(hDlg, IDC_BKG_EDIT), MAX_PATH-1);
			Edit_SetText(GetDlgItem(hDlg, IDC_BKG_EDIT), "");

			switch(GetBackground())
			{
				case 0:
					Button_SetCheck(GetDlgItem(hDlg, IDC_IMAGE_CUSTOM), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BKG_EDIT),   TRUE);
					strcpy(szFile, GetBgDir());
					strcat(szFile, "\\bkground.png");
					Edit_SetText(GetDlgItem(hDlg, IDC_BKG_EDIT), szFile);
					break;
				case IDB_BKGND_SAND:
					Button_SetCheck(GetDlgItem(hDlg, IDC_IMAGE_1), TRUE);
					break;
				case IDB_BKGND_GREY:
					Button_SetCheck(GetDlgItem(hDlg, IDC_IMAGE_2), TRUE);
					break;
				case IDB_BKGND_BKGND16:
					Button_SetCheck(GetDlgItem(hDlg, IDC_IMAGE_3), TRUE);
					break;
				case IDB_BKGND_BKGND256:
					Button_SetCheck(GetDlgItem(hDlg, IDC_IMAGE_4), TRUE);
					break;
			}

			return TRUE;
		}

	case WM_COMMAND:
	
		if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_CUSTOM)) )
		{
			EnableWindow(GetDlgItem(hDlg, IDC_BKG_EDIT),   TRUE);
			EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_BKG), TRUE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg, IDC_BKG_EDIT),   FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_BKG), FALSE);
		}

		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			// Internal image #1
			if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_1)) )
			{
				SetBackground(IDB_BKGND_SAND);
			}
			// Internal image #2
			if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_2)) )
			{
				SetBackground(IDB_BKGND_GREY);
			}
			// Internal image #3
			if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_3)) )
			{
				SetBackground(IDB_BKGND_BKGND16);
			}
			// Internal image #4
			if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_4)) )
			{
				SetBackground(IDB_BKGND_BKGND256);
			}
			else if ( Button_GetCheck(GetDlgItem(hDlg, IDC_IMAGE_CUSTOM)) )
			{
				Edit_GetText(GetDlgItem(hDlg, IDC_BKG_EDIT), szFile, MAX_PATH);

				if ( *szFile != '\0' );
				{
					ResetBackground(szFile);
					SetBackground(0);
				}
			}

			// Set new background image			
			LoadBackgroundBitmap();
			InvalidateRect(hMain, NULL, TRUE);

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		case IDC_BROWSE_BKG:
		{
			OPENFILENAMEA OpenFileName;

			OpenFileName.lStructSize       = sizeof(OPENFILENAME);
			OpenFileName.hwndOwner         = hMain;
			OpenFileName.hInstance         = 0;
			OpenFileName.lpstrFilter       = "Image Files (*.png, *.bmp)\0*.PNG;*.BMP\0";
			OpenFileName.lpstrCustomFilter = NULL;
			OpenFileName.nMaxCustFilter    = 0;
			OpenFileName.nFilterIndex      = 1;
			OpenFileName.lpstrFile         = szFile;
			OpenFileName.nMaxFile          = sizeof(szFile);
			OpenFileName.lpstrFileTitle    = NULL;
			OpenFileName.nMaxFileTitle     = 0;
			OpenFileName.lpstrInitialDir   = GetBgDir();
			OpenFileName.lpstrTitle        = "Select a Background Image";
			OpenFileName.nFileOffset       = 0;
			OpenFileName.nFileExtension    = 0;
			OpenFileName.lpstrDefExt       = NULL;
			OpenFileName.lCustData         = 0;                                                 
			OpenFileName.lpfnHook 		   = NULL;
			OpenFileName.lpTemplateName    = NULL;                                    
			OpenFileName.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			if (GetOpenFileNameA(&OpenFileName))
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_BKG_EDIT), szFile);
			}

			break;
		}
		}			
		break;
	}
	return 0;
}


static INT_PTR CALLBACK FindGameDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static char szPattern[50] = "\0";
	static int iCriteria = 1;
	
	char Tmp[50];

	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			Edit_LimitText(GetDlgItem(hDlg, IDC_FIND_PATTERN), 50-1);
			Edit_SetText(GetDlgItem(hDlg, IDC_FIND_PATTERN), szPattern);

			if (iCriteria & 1)
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_FIND_NAME), TRUE);
			}
			
			if (iCriteria & 2)
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_FIND_DRIVER), TRUE);
			}

			if (iCriteria & 4)
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_FIND_MANUFACTURER), TRUE);
			}

			return TRUE;
		}

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:

			iCriteria = 0;

			if (Button_GetCheck(GetDlgItem(hDlg, IDC_FIND_NAME)) == TRUE)
			{
				iCriteria |= 1;
			}
			
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_FIND_DRIVER)) == TRUE)
			{
				iCriteria |= 2;
			}
			
			if (Button_GetCheck(GetDlgItem(hDlg, IDC_FIND_MANUFACTURER)) == TRUE)
			{
				iCriteria |= 4;
			}
			
			if (iCriteria == 0)
			{
				MessageBox(GetMainWindow(), "At least one criteria must be checked.", MAME32NAME, MB_OK | MB_ICONWARNING);
				iCriteria = 1;
				SetFocus(hDlg);
				return TRUE;
			}

			Edit_GetText(GetDlgItem(hDlg, IDC_FIND_PATTERN), szPattern, 50-1);
			
			if (strlen(szPattern) == 0)
			{
				MessageBox(GetMainWindow(), "Pattern must contain at least one car.", MAME32NAME, MB_OK | MB_ICONWARNING);
				SetFocus(hDlg);
				return TRUE;
			}

			// Go !
			sprintf(Tmp, "Search result : %d game(s) found.", GetGamesFound(szPattern, iCriteria));

			Static_SetText(GetDlgItem(hDlg, IDC_FIND_RESULT), Tmp);

			// Update folder
			{
				LPTREEFOLDER FolderFindGame = GetFolderByID(FOLDER_FINDGAME);
				int i;
				
				for (i=0; i<game_count; i++)
				{
					if ( games_found[i] )
					{
						AddGame(FolderFindGame, i);
					}
					else
					{
						RemoveGame(FolderFindGame, i);
					}
				}
			}

			return TRUE;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
	}
	return 0;
}


void SetStatusBarText(int part_index, const char *message)
{
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM) part_index, (LPARAM) message);
}

void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	SetStatusBarText(part_index, buf);
}

static void MameMessageBox(const char *fmt, ...)
{
	char buf[2048];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	MessageBox(GetMainWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);
	va_end(va);
}

static LRESULT CALLBACK BackMainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle system tray icon messages
	if ( uMsg == uShellIconMsg )
	{
		if ( ! in_emulation )
		{
			switch( (UINT)lParam )
			{
				case WM_RBUTTONUP:
					SetForegroundWindow(hWnd);
					if ( HandleSystemTrayContextMenu(hWnd, wParam, TRUE) )
					{
						return FALSE;
					}
					break;

				case WM_LBUTTONDBLCLK:
					if ( have_selection )
					{
						MamePlayGame();
					}
					break;
			}
		}

		return TRUE;
	}

	switch (uMsg)
	{
		case WM_ACTIVATE:
			if ( LOWORD(wParam) == WA_INACTIVE )
			{
				HandleSystemTrayContextMenu(hWnd, wParam, FALSE);
				return FALSE;
			}
		break;

		case WM_ERASEBKGND:
		{
			BITMAP Bitmap;
			GetObject(hSplashBmp, sizeof(BITMAP), &Bitmap);
			BitBlt((HDC)wParam, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, hMemoryDC, 0, 0, SRCCOPY);
			break;
		}

		case WM_COMMAND:
			switch( GET_WM_COMMAND_ID(wParam, lParam) )
			{
				case ID_FILE_EXIT:
					SetWindowState(SW_HIDE);

				case ID_FILE_PLAY:
				case ID_CONTEXT_SELECT_RANDOM:
					return MameCommand(hMain,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

				case ID_CONTEXT_SELECT_PREVIOUS:
					SetSelectedPick(GetSelectedPick() - 1);
			 		break;

				case ID_CONTEXT_SELECT_NEXT:
					SetSelectedPick(GetSelectedPick() + 1);
			 		break;

				case ID_CONTEXT_SHOW_MAIN:
					bIsWindowsVisible = TRUE;
					SetWindowState(SW_SHOW);
					ShowWindow(hMain, SW_SHOW);
//					SetForegroundWindow(hMain);
					SetFocus(hwndList);
			 		break;
				
				case ID_CONTEXT_HIDE_MAIN:
					bIsWindowsVisible = FALSE;
					SetWindowState(SW_HIDE);
					ShowWindow(hMain, SW_HIDE);
			 		break;

				default:
					if (GET_WM_COMMAND_ID(wParam, lParam) >= ID_CONTEXT_SELECT_CHOICE_START && GET_WM_COMMAND_ID(wParam, lParam) < ID_CONTEXT_SELECT_CHOICE_END)
					{
						SetSelectedPick(GET_WM_COMMAND_ID(wParam, lParam) - ID_CONTEXT_SELECT_CHOICE_START);
					}
			 		break;
			}

		default:
			return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }

    return FALSE;
}

static void MamePlayBackGame()
{
	int nGame;
	char filename[MAX_PATH];

	*filename = 0;

	nGame = GetSelectedPickItem();
	if (nGame != -1)
		strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetOpenFileName, filename, TRUE, TRUE))
	{
		mame_file* pPlayBack;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char bare_fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		char path[MAX_PATH];
		char fname[MAX_PATH];

		_splitpath(filename, drive, dir, bare_fname, ext);

		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		set_pathlist(FILETYPE_INPUTLOG,path);
		pPlayBack = mame_fopen(fname,NULL,FILETYPE_INPUTLOG,0);
		if (pPlayBack == NULL)
		{
			MameMessageBox("Could not open '%s' as a valid input file.", filename);
			return;
		}

		// check for game name embedded in .inp header
		if (pPlayBack)
		{
			INP_HEADER inp_header;

			// read playback header
			mame_fread(pPlayBack, &inp_header, sizeof(INP_HEADER));

			if (!isalnum(inp_header.name[0])) // If first byte is not alpha-numeric
				mame_fseek(pPlayBack, 0, SEEK_SET); // old .inp file - no header
			else
			{
				int i;
				for (i = 0; drivers[i] != 0; i++) // find game and play it
				{
					if (strcmp(drivers[i]->name, inp_header.name) == 0)
					{
						nGame = i;
						break;
					}
				}
			}
		}
		mame_fclose(pPlayBack);

		g_pPlayBkName = fname;
		override_playback_directory = path;
		MamePlayGameWithOptions(nGame);
		g_pPlayBkName = NULL;
		override_playback_directory = NULL;
	}
}

static void MameLoadState()
{
	int nGame;
	char filename[MAX_PATH];
	char selected_filename[MAX_PATH];

	*filename = 0;

	nGame = GetSelectedPickItem();
	if (nGame != -1)
	{
		strcpy(filename, drivers[nGame]->name);
		strcpy(selected_filename, drivers[nGame]->name);
	}
	if (CommonFileDialog(GetOpenFileName, filename, FALSE, FALSE))
	{
		mame_file* pSaveState;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char ext[_MAX_EXT];

		char path[MAX_PATH];
		char fname[MAX_PATH];
		char bare_fname[_MAX_FNAME];
		char *state_fname;
		int rc;

		_splitpath(filename, drive, dir, bare_fname, ext);

		// parse path
		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

#ifdef MESS
		{
			state_fname = filename;
		}
#else // !MESS
		{
			char *cPos=0;
			int  iPos=0;
			char romname[MAX_PATH];

			cPos = strchr(bare_fname, '-' );
			iPos = cPos ? cPos - bare_fname : strlen(bare_fname);
			strncpy(romname, bare_fname, iPos );
			romname[iPos] = '\0';
			if( strcmp(selected_filename,romname) != 0 )
			{
				MameMessageBox("'%s' is not a valid savestate file for game '%s'.", filename, selected_filename);
				return;
			}
			set_pathlist(FILETYPE_STATE,path);
			state_fname = fname;
		}
#endif // MESS

		pSaveState = mame_fopen(NULL,state_fname,FILETYPE_STATE,0);
		if (pSaveState == NULL)
		{
			MameMessageBox("Could not open '%s' as a valid savestate file.", filename);
			return;
		}

		// call the MAME core function to check the save state file
		rc = state_save_check_file(pSaveState, selected_filename, MameMessageBox);
		mame_fclose(pSaveState);
		if (rc)
			return;

#ifdef MESS
		g_pSaveStateName = state_fname;
#else
		{
			char *cPos;
			cPos = strrchr(bare_fname, '-' );
			cPos = cPos+1;
			if( strlen(cPos) >0)
			{
				g_pSaveStateName = cPos;
				override_savestate_directory = path;
			}
		}
#endif

		MamePlayGameWithOptions(nGame);
		g_pSaveStateName = NULL;
		override_savestate_directory = NULL;
	}
}

static void MamePlayRecordGame()
{
	int  nGame;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = GetSelectedPickItem();
	strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetSaveFileName, filename, FALSE, TRUE))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		g_pRecordName = fname;
		override_playback_directory = path;
		MamePlayGameWithOptions(nGame);
		g_pRecordName = NULL;
		override_playback_directory = NULL;
	}
}

static void MamePlayGame()
{
	int i, nGame;

	const char *TestDrivers[13] =
	{
		"cthd2003",
		"kof2001",
		"kof2002",
		"kof2003",
		"matri",
		"mslug4",
		"mslug5",
		"pnyaa",
		"rotd",
		"samsho5",
		"sengoku3",
		"svcchaos",
		"zupapa"
	};

	nGame = GetSelectedPickItem();

	if ( ! GetShowRecent() )
	{
		for ( i=0; i<13; i++ )
		{
			if ( stricmp(TestDrivers[i], drivers[nGame]->name) == 0 )
			{
				MessageBox(GetMainWindow(), "This game is too recent to be emulated.\nIt is listed only for debug purpose.", "Game is locked", MB_OK | MB_ICONEXCLAMATION);
				
				return;
			}
		}
	}

	if ( bPwdEnabled )
	{
		if ( !bPwdVerified && GameIsLocked(nGame) )
		{
			MessageBox(GetMainWindow(), "Please verify password to play this game.", "Game is locked", MB_OK | MB_ICONEXCLAMATION);
			
			return;
		}
	}

	g_pPlayBkName = NULL;
	g_pRecordName = NULL;

	MamePlayGameWithOptions(nGame);
}

static void MamePlayGameWithOptions(int nGame)
{
	memcpy(&playing_game_options, GetGameOptions(nGame, -1), sizeof(options_type));

	/* Deal with options that can be disabled. */
	EnablePlayOptions(nGame, &playing_game_options);

	if (g_pJoyGUI != NULL)
		KillTimer(hMain, JOYGUI_TIMER);
	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	g_bAbortLoading = FALSE;

	sprintf( MameIcon.szTip, "%s game running ...", MAME32NAME );
	Shell_NotifyIcon( NIM_MODIFY, &MameIcon );

	in_emulation = TRUE;

	if (RunMAME(nGame) == 0)
	{
	   IncrementPlayCount(nGame);
	   ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
	}
	else
	{
		ShowWindow(hMain, SW_SHOW);
	}

	in_emulation = FALSE;

	// re-sort if sorting on # of times played
	if (GetSortColumn() == COLUMN_PLAYED)
		SortListView();

	UpdateStatusBar();

	sprintf(MameIcon.szTip, "%s [%.50s]", MAME32NAME, ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));
	Shell_NotifyIcon( NIM_MODIFY, &MameIcon );

	if ( bIsWindowsVisible )
	{
	ShowWindow(hMain, SW_SHOW);
	SetFocus(hwndList);
	}

	if (g_pJoyGUI != NULL)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
}

/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot(void)
{
	BOOL showScreenShot = GetShowScreenShot();

	SetShowScreenShot((showScreenShot) ? FALSE : TRUE);
	UpdateScreenShot();

	/* Redraw list view */
	if (hBackground != NULL && showScreenShot)
		InvalidateRect(hwndList, NULL, FALSE);
}

static void AdjustMetrics(void)
{
	HDC hDC;
	TEXTMETRIC tm;
	int xtraX, xtraY;
	AREA area;
	int  offX, offY;
	int  maxX, maxY;
	COLORREF textColor;

	/* WM_SETTINGCHANGE also */
	xtraX  = GetSystemMetrics(SM_CXFIXEDFRAME); /* Dialog frame width */
	xtraY  = GetSystemMetrics(SM_CYFIXEDFRAME); /* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE);	/* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION);	/* Caption Height */
	maxX   = GetSystemMetrics(SM_CXSCREEN); 	/* Screen Width */
	maxY   = GetSystemMetrics(SM_CYSCREEN); 	/* Screen Height */

	hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);

	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight 	  / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	ListView_SetBkColor(hwndList, GetSysColor(COLOR_WINDOW));
	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	ListView_SetTextColor(hwndList, textColor);
	TreeView_SetBkColor(hTreeView, GetSysColor(COLOR_WINDOW));
	TreeView_SetTextColor(hTreeView, textColor);
	GetWindowArea(&area);

	offX = area.x + area.width;
	offY = area.y + area.height;

	if (offX > maxX)
	{
		offX = maxX;
		area.x = (offX - area.width > 0) ? (offX - area.width) : 0;
	}
	if (offY > maxY)
	{
		offY = maxY;
		area.y = (offY - area.height > 0) ? (offY - area.height) : 0;
	}

	SetWindowArea(&area);
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW);
}


/* Adjust options - tune them to the currently selected game */
static void EnablePlayOptions(int nIndex, options_type *o)
{
	if (!DIJoystick.Available())
		o->use_joystick = FALSE;
}

static int FindIconIndex(int nIconResource)
{
	int i;
	for(i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

static int GetIconForDriver(int nItem)
{
	int iconRoms;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
		if (IsAuditResultKnown(audit_result) == FALSE)
			return 2;

		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
	}
	else
		iconRoms = 1;

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	/* these are indices into icon_names, which maps into our image list
	 * also must match IDI_WIN_NOROMS + iconRoms
     */

	// Show Red-X if the ROMs are present and flagged as NOT WORKING
	if (iconRoms == 1 && DriverIsBroken(nItem))
		iconRoms = FindIconIndex(IDI_WIN_REDX);

	// show clone icon if we have roms and game is working
	if (iconRoms == 1 && DriverIsClone(nItem))
		iconRoms = FindIconIndex(IDI_WIN_CLONE);

	// if we have the roms, then look for a custom per-game icon to override
	if (iconRoms == 1 || iconRoms == 3)
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}

static BOOL HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hTreeMenu;
	HMENU hMenu;
	TVHITTESTINFO hti;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	/* select the item that was right clicked or shift-F10'ed */
	hti.pt = pt;
	ScreenToClient(hTreeView,&hti.pt);
	TreeView_HitTest(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		TreeView_SelectItem(hTreeView,hti.hItem);

	hTreeMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	hMenu = GetSubMenu(hTreeMenu, 0);
	
	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hTreeMenu);

	return TRUE;
}

static BOOL HandleContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_LIST))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	if ((current_view_id == ID_VIEW_DETAIL || current_view_id == ID_VIEW_GROUPED)
		&&	HeaderOnContextMenu(hWnd, wParam, lParam) == TRUE)
	{
		return TRUE;
	}

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static BOOL HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_SSPICTURE) && (HWND)wParam != GetDlgItem(hWnd, IDC_SSFRAME))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SCREENSHOT));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static BOOL HandleSystemTrayContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	static HMENU hMenuLoad = NULL;
	HMENU hMenu;
	POINT pt;

	if ( lParam )
	{
		GetCursorPos(&pt);

		hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SYSTEMTRAY));
		InitSystrayContextMenu(hMenuLoad);
		hMenu = GetSubMenu(hMenuLoad, 0);

		UpdateMenu(hMenu);
		TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);
		DestroyMenu(hMenuLoad);
		hMenuLoad = NULL;
	}
	else if ( hMenuLoad != NULL )
	{
		DestroyMenu(hMenuLoad);
		hMenuLoad = NULL;
	}

	return TRUE;
}

static void UpdateMenu(HMENU hMenu)
{
	char			buf[200];
	MENUITEMINFO	mItem;
	int 			nGame = GetSelectedPickItem();
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int i;
	const char *pParent;

	if (have_selection)
	{
		snprintf(buf, sizeof(buf), g_szPlayGameString,
				 ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));

		mItem.cbSize	 = sizeof(mItem);
		mItem.fMask 	 = MIIM_TYPE;
		mItem.fType 	 = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch		 = strlen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mItem);

		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
		
		if ( GetSelectedPick() == 0 )
		{
			EnableMenuItem(hMenu, ID_CONTEXT_SELECT_PREVIOUS, MF_GRAYED);
		}
		else if ( GetSelectedPick() == (ListView_GetItemCount(hwndList)-1) )
		{
			EnableMenuItem(hMenu, ID_CONTEXT_SELECT_NEXT, MF_GRAYED);
		}
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, 			MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_PREVIOUS,MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_NEXT,	MF_GRAYED);
	}

	if (oldControl)
	{
		EnableMenuItem(hMenu, ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,  MF_GRAYED);
		EnableMenuItem(hMenu, ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
	    EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
	    EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if( (strcmp(lpFolder->m_lpTitle, "Vector" ) == 0) ||
		(pParent && (strcmp(pParent, "Source" ) == 0) ) )
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	CheckMenuRadioItem(hMenu, ID_VIEW_TAB_SCREENSHOT, ID_VIEW_TAB_HISTORY,
					   ID_VIEW_TAB_SCREENSHOT + GetCurrentTab(), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_UNCHECKED);

	// set whether the tree folder is sorted by alphabetic order
	if (bSortTree)
		CheckMenuItem(hMenu,ID_SORT_TREE,MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu,ID_SORT_TREE,MF_BYCOMMAND | MF_UNCHECKED);

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items 
		if (GetShowTab(i))
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (i=0;i<MAX_FOLDERS;i++)
	{
		// Hide Unavailable folder option if not allowed (Mame licence requirement)
		if ((i == FOLDER_UNAVAILABLE) && (!GetShowUnavailableFolder()))
			EnableMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_GRAYED);
		
		if (GetShowFolder(i))
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

}

void InitTreeContextMenu(HMENU hTreeMenu)
{
	MENUITEMINFO mii;
	HMENU hMenu;
	int i;
	extern FOLDERDATA g_folderData[];

	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	hMenu = GetSubMenu(hTreeMenu, 0);

	if (GetMenuItemInfo(hMenu,4,TRUE,&mii) == FALSE)
	{
		dprintf("can't find show folders context menu");
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		dprintf("can't find submenu for show folders context menu");
		return;
	}

	hMenu = mii.hSubMenu;
	
	for (i=0;g_folderData[i].m_lpTitle != NULL;i++)
	{
		mii.fMask = MIIM_TYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.dwTypeData = (char *)g_folderData[i].m_lpTitle;
		mii.cch = strlen(mii.dwTypeData);
		mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;


		// menu in resources has one empty item (needed for the submenu to setup properly)
		// so overwrite this one, append after
		if (i == 0)
			SetMenuItemInfo(hMenu,ID_CONTEXT_SHOW_FOLDER_START,FALSE,&mii);
		else
			InsertMenuItem(hMenu,i,FALSE,&mii);
	}

}

void InitSystrayContextMenu(HMENU hSystrayMenu)
{
	MENUITEMINFO mii;
	HMENU hMenu;
	int iItem = 0;
    LV_ITEM lvi;

	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	hMenu = GetSubMenu(hSystrayMenu, 0);

	if (GetMenuItemInfo(hMenu,5,TRUE,&mii) == FALSE)
	{
		dprintf("can't find select choice context menu");
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		dprintf("can't find submenu for select choice context menu");
		return;
	}

	hMenu = mii.hSubMenu;
	
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL);
    while( lvi.iItem != -1 )
	{
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;

        if ( ListView_GetItem(hwndList, &lvi) )
        {
			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ModifyThe(drivers[lvi.lParam]->description);
			mii.cch = strlen(mii.dwTypeData);
			mii.wID = ID_CONTEXT_SELECT_CHOICE_START + lvi.iItem;

			// menu in resources has one empty item (needed for the submenu to setup properly)
			// so overwrite this one, append after
			if (iItem == 0)
				SetMenuItemInfo(hMenu,ID_CONTEXT_SELECT_CHOICE_START,FALSE,&mii);
			else
				InsertMenuItem(hMenu,iItem,FALSE,&mii);
				
			iItem++;
		}

        lvi.iItem = ListView_GetNextItem(hwndList, lvi.iItem, LVNI_BELOW);
	}
}

void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();

	SetWindowRedraw(hwndList,FALSE);

	SetShowFolder(folder,!GetShowFolder(folder));

	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);

	SetWindowRedraw(hwndList,TRUE);
}

/* Add ... to Items in ListView if needed */
static LPCTSTR MakeShortString(HDC hDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const CHAR szThreeDots[] = "...";
	static CHAR szShort[MAX_PATH];
	int nStringLen = lstrlen(lpszLong);
	int nAddLen;
	SIZE size;
	int i;

	GetTextExtentPoint32(hDC, lpszLong, nStringLen, &size);
	if (nStringLen == 0 || size.cx + nOffset <= nColumnLen)
		return lpszLong;

	lstrcpy(szShort, lpszLong);
	GetTextExtentPoint32(hDC, szThreeDots, sizeof(szThreeDots), &size);
	nAddLen = size.cx;

	for (i = nStringLen - 1; i > 0; i--)
	{
		szShort[i] = 0;
		GetTextExtentPoint32(hDC, szShort, i, &size);
		if (size.cx + nOffset + nAddLen <= nColumnLen)
			break;
	}

	lstrcat(szShort, szThreeDots);

	return szShort;
}

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hBackground)
	{
		switch (uMsg)
		{
	    case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}
    
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
		{
			POINT p = { 0, 0 };
  			
			/* get base point of background bitmap */
			MapWindowPoints(hWnd,hTreeView,&p,1);
			PaintBackgroundImage(hWnd, NULL, p.x, p.y);
			/* to ensure our parent procedure repaints the whole client area */
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		}
		}
	}
	return CallWindowProc(g_lpHistoryWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
    {
		if (MouseHasBeenMoved())
			ShowCursor(TRUE);
		break;
    }
  
    case WM_NCHITTEST :
	{
		POINT pt;
		RECT  rect;
		HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		GetWindowRect(hHistory, &rect);
		// check if they clicked on the picture area (leave 6 pixel no man's land
		// by the history window to reduce mistaken clicks)
		// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback

		if (have_history &&        
			( ( (GetCurrentTab() == TAB_HISTORY) || 
			 (GetCurrentTab() == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ) &&
//			  (rect.top - 6) < pt.y && pt.y < (rect.bottom + 6) ) )
			  		PtInRect( &rect, pt ) ) )

		{
			return HTTRANSPARENT;
		}
		else
		{
			return HTCLIENT;
		}
	}
	break;
	case WM_CONTEXTMENU:
		if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;
	}

	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND :
			return TRUE;
		case WM_PAINT :
		{
			RECT rect,nodraw_rect;
			HRGN region,nodraw_region;
			POINT p = { 0, 0 };

			/* get base point of background bitmap */
			MapWindowPoints(hWnd,hTreeView,&p,1);

			/* get big region */
			GetClientRect(hWnd,&rect);
			region = CreateRectRgnIndirect(&rect);

			if (IsWindowVisible(GetDlgItem(hMain,IDC_HISTORY)))
			{
				/* don't draw over this window */
				GetWindowRect(GetDlgItem(hMain,IDC_HISTORY),&nodraw_rect);
				MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
				nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
				CombineRgn(region,region,nodraw_region,RGN_DIFF);
				DeleteObject(nodraw_region);
			}
			if (IsWindowVisible(GetDlgItem(hMain,IDC_SSPICTURE)))
			{
				/* don't draw over this window */
				GetWindowRect(GetDlgItem(hMain,IDC_SSPICTURE),&nodraw_rect);
				MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
				nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
				CombineRgn(region,region,nodraw_region,RGN_DIFF);
				DeleteObject(nodraw_region);
			}

			PaintBackgroundImage(hWnd,region,p.x,p.y);

			DeleteObject(region);

			/* to ensure our parent procedure repaints the whole client area */
			InvalidateRect(hWnd, NULL, FALSE);

			break;
		}
		}
	}
	return CallWindowProc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND :
		return TRUE;
	case WM_PAINT :
	{
		PAINTSTRUCT ps;
		HDC	hdc,hdc_temp;
		RECT rect;
		HBITMAP old_bitmap;

		int width,height;

		hdc = BeginPaint(hWnd,&ps);

		hdc_temp = CreateCompatibleDC(hdc);
		if (ScreenShotLoaded())
		{
			width = GetScreenShotWidth();
			height = GetScreenShotHeight();

			old_bitmap = SelectObject(hdc_temp,GetScreenShotHandle());
		}
		else
		{
			BITMAP bmp;

			GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);
			width = bmp.bmWidth;
			height = bmp.bmHeight;

			old_bitmap = SelectObject(hdc_temp,hMissing_bitmap);
		}

		GetClientRect(hWnd,&rect);
		SetStretchBltMode(hdc,STRETCH_HALFTONE);
		StretchBlt(hdc,0,0,rect.right-rect.left,rect.bottom-rect.top,
				   hdc_temp,0,0,width,height,SRCCOPY);
		SelectObject(hdc_temp,old_bitmap);
		DeleteDC(hdc_temp);

		EndPaint(hWnd,&ps);

		return TRUE;
	}
	}

	return CallWindowProc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}

/* DrawItem for ListView */
static void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HDC         hDC = lpDrawItemStruct->hDC;
	RECT        rcItem = lpDrawItemStruct->rcItem;
	UINT        uiFlags = ILD_TRANSPARENT;
	HIMAGELIST  hImageList;
	int         nItem = lpDrawItemStruct->itemID;
	COLORREF    clrTextSave = 0;
	COLORREF    clrBkSave = 0;
	COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
	static CHAR szBuff[MAX_PATH];
	BOOL        bFocus = (GetFocus() == hwndList);
	LPCTSTR     pszText;
	UINT        nStateImageMask;
	BOOL        bSelected;
	LV_COLUMN   lvc;
	LV_ITEM     lvi;
	RECT        rcAllLabels;
	RECT        rcLabel;
	RECT        rcIcon;
	int         offset;
	SIZE        size;
	int         i, j;
	int         nColumn;
	int         nColumnMax = 0;
	int         order[COLUMN_MAX];
	BOOL draw_as_clone;
	int indent_space;
	BOOL		bParentFound = FALSE;
	BOOL		bColourClone = FALSE;

	nColumnMax = GetNumColumns(hwndList);

	if (oldControl)
	{
		GetColumnOrder(order);
	}
	else
	{
		/* Get the Column Order and save it */
		ListView_GetColumnOrderArray(hwndList, nColumnMax, order);

		/* Disallow moving column 0 */
		if (order[0] != 0)
		{
			for (i = 0; i < nColumnMax; i++)
			{
				if (order[i] == 0)
				{
					order[i] = order[0];
					order[0] = 0;
				}
			}
			ListView_SetColumnOrderArray(hwndList, nColumnMax, order);
		}
	}

	/* Labels are offset by a certain amount */
	/* This offset is related to the width of a space character */
	GetTextExtentPoint32(hDC, " ", 1 , &size);
	offset = size.cx;

	lvi.mask	   = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem	   = nItem;
	lvi.iSubItem   = order[0];
	lvi.pszText    = szBuff;
	lvi.cchTextMax = sizeof(szBuff);
	lvi.stateMask  = 0xFFFF;	   /* get all state flags */
	ListView_GetItem(hwndList, &lvi);

	bSelected = ((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED)
		&& ((bFocus) || (GetWindowLong(hwndList, GWL_STYLE) & LVS_SHOWSELALWAYS))));

	/* figure out if we indent and draw grayed */
	draw_as_clone = (GetViewMode() == VIEW_GROUPED && DriverIsClone(lvi.lParam));

	/* only indent if parent is also in this view */
	if( DriverIsClone(lvi.lParam) && draw_as_clone )
	{
		char cDesc[256];
		char *pDesc2 = NULL;
		for( i=0;i<ListView_GetItemCount(hwndList);i++)
		{
			ListView_GetItemText(hwndList,i,0,cDesc, 255 );
			pDesc2 = ModifyThe(drivers[lvi.lParam]->clone_of->description);
			if( strcmp(pDesc2, cDesc ) == 0 )
			{
				bParentFound = TRUE;
				break;
			}
		}
	}
	if( GetOffsetClones() )
	{
		if( ! bParentFound && draw_as_clone)
		{
			/*Reset it, as no Parent is there*/
			draw_as_clone = FALSE;
			bColourClone = TRUE;
		}
		else
		{
			bParentFound = FALSE;
		}
	}

	ListView_GetItemRect(hwndList, nItem, &rcAllLabels, LVIR_BOUNDS);

	ListView_GetItemRect(hwndList, nItem, &rcLabel, LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;

	if (hBackground != NULL)
	{
		RECT		rcClient;
		HRGN		rgnBitmap;
		RECT		rcTmpBmp = rcItem;
		RECT		rcFirstItem;
		HPALETTE	hPAL;
		HDC 		htempDC;
		HBITMAP 	oldBitmap;

		htempDC = CreateCompatibleDC(hDC);

		oldBitmap = SelectObject(htempDC, hBackground);

		GetClientRect(hwndList, &rcClient);
		rcTmpBmp.right = rcClient.right;
		/* We also need to check whether it is the last item
		   The update region has to be extended to the bottom if it is */
		if (nItem == ListView_GetItemCount(hwndList) - 1)
			rcTmpBmp.bottom = rcClient.bottom;

		rgnBitmap = CreateRectRgnIndirect(&rcTmpBmp);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);

		hPAL = GetBackgroundPalette();
		if (hPAL == NULL)
			hPAL = CreateHalftonePalette(hDC);

		if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(htempDC, hPAL, FALSE);
			RealizePalette(htempDC);
		}

		ListView_GetItemRect(hwndList, 0, &rcFirstItem, LVIR_BOUNDS);

		for (i = rcFirstItem.left; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = rcFirstItem.top; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

		SelectObject(htempDC, oldBitmap);
		DeleteDC(htempDC);

		if (GetBackgroundPalette() == NULL)
		{
			DeleteObject(hPAL);
			hPAL = NULL;
		}
	}

	indent_space = 0;

	if (draw_as_clone)
	{
		RECT rect;
		ListView_GetItemRect(hwndList, nItem, &rect, LVIR_ICON);

		/* indent width of icon + the space between the icon and text
		 * so left of clone icon starts at text of parent
         */
		indent_space = rect.right - rect.left + offset;
	}

	rcAllLabels.left += indent_space;

	if (bSelected)
	{
		HBRUSH hBrush;
		HBRUSH hOldBrush;

		if (bFocus)
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		}

		hOldBrush = SelectObject(hDC, hBrush);
		FillRect(hDC, &rcAllLabels, hBrush);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}
	else
	{
		if (hBackground == NULL)
		{
			HBRUSH hBrush;
			
			hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			FillRect(hDC, &rcAllLabels, hBrush);
			DeleteObject(hBrush);
		}
		
		if( GetOffsetClones() )
		{
			if ( draw_as_clone || bColourClone)
				clrTextSave = SetTextColor(hDC,GetListCloneColor());
			else
				clrTextSave = SetTextColor(hDC,GetListFontColor());
		}
		else
		{
			if ( draw_as_clone )
				clrTextSave = SetTextColor(hDC,GetListCloneColor());
			else
				clrTextSave = SetTextColor(hDC,GetListFontColor());
		}

		clrBkSave = SetBkColor(hDC,GetSysColor(COLOR_WINDOW));
	}


	if (lvi.state & LVIS_CUT)
	{
		clrImage = GetSysColor(COLOR_WINDOW);
		uiFlags |= ILD_BLEND50;
	}
	else
	if (bSelected)
	{
		if (bFocus)
			clrImage = GetSysColor(COLOR_HIGHLIGHT);
		else
			clrImage = GetSysColor(COLOR_BTNFACE);

		uiFlags |= ILD_BLEND50;
	}

	nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;

	if (nStateImageMask)
	{
		int nImage = (nStateImageMask >> 12) - 1;
		hImageList = ListView_GetImageList(hwndList, LVSIL_STATE);
		if (hImageList)
			ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
	}

	ListView_GetItemRect(hwndList, nItem, &rcIcon, LVIR_ICON);

	rcIcon.left += indent_space;

	ListView_GetItemRect(hwndList, nItem, &rcItem, LVIR_LABEL);

	hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);
	if (hImageList)
	{
		UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
		if (rcIcon.left + 16 + indent_space < rcItem.right)
		{
			ImageList_DrawEx(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top, 16, 16,
							 GetSysColor(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
		}
	}

	ListView_GetItemRect(hwndList, nItem, &rcItem, LVIR_LABEL);

	pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2*offset + indent_space);

	rcLabel = rcItem;
	rcLabel.left  += offset + indent_space;
	rcLabel.right -= offset;

	DrawText(hDC, pszText, -1, &rcLabel,
			 DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

	for (nColumn = 1; nColumn < nColumnMax; nColumn++)
	{
		int 	nRetLen;
		UINT	nJustify;
		LV_ITEM lvItem;

		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		ListView_GetColumn(hwndList, order[nColumn], &lvc);

		lvItem.mask 	  = LVIF_TEXT;
		lvItem.iItem	  = nItem;
		lvItem.iSubItem   = order[nColumn];
		lvItem.pszText	  = szBuff;
		lvItem.cchTextMax = sizeof(szBuff);

		if (ListView_GetItem(hwndList, &lvItem) == FALSE)
			continue;

		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;

		nRetLen = strlen(szBuff);
		if (nRetLen == 0)
			continue;

		pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2 * offset);

		nJustify = DT_LEFT;

		if (pszText == szBuff)
		{
			switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
			{
			case LVCFMT_RIGHT:
				nJustify = DT_RIGHT;
				break;

			case LVCFMT_CENTER:
				nJustify = DT_CENTER;
				break;

			default:
				break;
			}
		}

		rcLabel = rcItem;
		rcLabel.left  += offset;
		rcLabel.right -= offset;
		DrawText(hDC, pszText, -1, &rcLabel,
				 nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
	}

	if (lvi.state & LVIS_FOCUSED && bFocus)
		DrawFocusRect(hDC, &rcAllLabels);

	SetTextColor(hDC, clrTextSave);
	SetBkColor(hDC, clrBkSave);
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK ListViewWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hBackground != NULL)
	{
		switch (uMsg)
		{
	    case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}

		case WM_ERASEBKGND:
			return ListCtrlOnErase(hwnd, (HDC)wParam);

		case WM_PAINT:
			if (current_view_id != ID_VIEW_DETAIL && current_view_id != ID_VIEW_GROUPED)
				if (ListCtrlOnPaint(hwnd, uMsg) == 0)
					return 0;
			break;

		case WM_NOTIFY:
		{
			HD_NOTIFY *pHDN = (HD_NOTIFY*)lParam;
			
			/*
			  This code is for using bitmap in the background
			  Invalidate the right side of the control when a column is resized			
			*/
			if (pHDN->hdr.code == HDN_ITEMCHANGINGW || pHDN->hdr.code == HDN_ITEMCHANGINGA)
			{
				RECT rcClient;
				DWORD dwPos;
				POINT pt;
					
				dwPos = GetMessagePos();
				pt.x = LOWORD(dwPos);
				pt.y = HIWORD(dwPos);
					
				GetClientRect(hwnd, &rcClient);
				ScreenToClient(hwnd, &pt);
				rcClient.left = pt.x;
				InvalidateRect(hwnd, &rcClient, FALSE);
			}
			break;
		}
		}
	}
#if 0
	switch (uMsg)
	{
		case WM_CHAR: /* List-View controls use this message for searching the items "as user types" */
			//MessageBox(NULL,"wm_char message arrived","TitleBox",MB_OK);
			return 0;
	}
#endif
	/* message not handled */
	return CallWindowProc(g_lpListViewWndProc, hwnd, uMsg, wParam, lParam);
}

static BOOL ListCtrlOnErase(HWND hWnd, HDC hDC)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;

	int 		i, j;
	HDC 		htempDC;
	POINT		ptOrigin;
	POINT		pt = {0,0};
	HBITMAP 	hOldBitmap;

	// this does not draw the background properly in report view

	GetClientRect(hWnd, &rcClient);

	htempDC = CreateCompatibleDC(hDC);
	hOldBitmap = SelectObject(htempDC, hBackground);

	rgnBitmap = CreateRectRgnIndirect(&rcClient);
	SelectClipRgn(hDC, rgnBitmap);
	DeleteObject(rgnBitmap);

	hPAL = (!hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, FALSE);
		RealizePalette(htempDC);
	}

	/* Get x and y offset */
	MapWindowPoints(hWnd,hTreeView,&pt,1);
	GetDCOrgEx(hDC, &ptOrigin);
	ptOrigin.x -= pt.x;
	ptOrigin.y -= pt.y;
	ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
	ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

	for (i = ptOrigin.x; i < rcClient.right; i += bmDesc.bmWidth)
		for (j = ptOrigin.y; j < rcClient.bottom; j += bmDesc.bmHeight)
			BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

	SelectObject(htempDC, hOldBitmap);
	DeleteDC(htempDC);

	if (!bmDesc.bmColors)
	{
		DeleteObject(hPAL);
		hPAL = 0;
	}

	return TRUE;
}

static BOOL ListCtrlOnPaint(HWND hWnd, UINT uMsg)
{
	PAINTSTRUCT ps;
	HDC 		hDC;
	RECT		rcClip, rcClient;
	HDC 		memDC;
	HBITMAP 	bitmap;
	HBITMAP 	hOldBitmap;

	hDC = BeginPaint(hWnd, &ps);
	rcClient = ps.rcPaint;
	GetClipBox(hDC, &rcClip);
	GetClientRect(hWnd, &rcClient);
	/* Create a compatible memory DC */
	memDC = CreateCompatibleDC(hDC);

	/* Select a compatible bitmap into the memory DC */
	bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
									rcClient.bottom - rcClient.top );
	hOldBitmap = SelectObject(memDC, bitmap);

	BitBlt(memDC, rcClip.left, rcClip.top,
		   rcClip.right - rcClip.left, rcClip.bottom - rcClip.top,
		   hDC, rcClip.left, rcClip.top, SRCCOPY);

	/* First let the control do its default drawing. */
	CallWindowProc(g_lpListViewWndProc, hWnd, uMsg, (WPARAM)memDC, 0);
	/* Draw bitmap in the background */
	if( hBackground )
	{
		HPALETTE	hPAL;
		HDC maskDC;
		HBITMAP maskBitmap;
		HDC tempDC;
		HDC imageDC;
		HBITMAP bmpImage;
		HBITMAP hOldBmpImage;
		HBITMAP hOldMaskBitmap;
		HBITMAP hOldHBitmap;
		int i, j;
		POINT ptOrigin;
		POINT pt = {0,0};

		/* Now create a mask */
		maskDC = CreateCompatibleDC(hDC);

		/* Create monochrome bitmap for the mask */
		maskBitmap = CreateBitmap(rcClient.right  - rcClient.left,
								  rcClient.bottom - rcClient.top,
								  1, 1, NULL );

		hOldMaskBitmap = SelectObject(maskDC, maskBitmap);
		SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

		/* Create the mask from the memory DC */
		BitBlt(maskDC, 0, 0, rcClient.right - rcClient.left,
			   rcClient.bottom - rcClient.top, memDC,
			   rcClient.left, rcClient.top, SRCCOPY);

		tempDC = CreateCompatibleDC(hDC);
		hOldHBitmap = SelectObject(tempDC, hBackground);

		imageDC = CreateCompatibleDC(hDC);
		bmpImage = CreateCompatibleBitmap(hDC,
										  rcClient.right  - rcClient.left,
										  rcClient.bottom - rcClient.top);
		hOldBmpImage = SelectObject(imageDC, bmpImage);

		hPAL = (! hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(hDC, hPAL, FALSE);
			RealizePalette(hDC);
			SelectPalette(imageDC, hPAL, FALSE);
		}

		/* Get x and y offset */
		ClientToScreen(hWnd, &pt);
		GetDCOrgEx(hDC, &ptOrigin);
		ptOrigin.x -= pt.x;
		ptOrigin.y -= pt.y;
		ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
		ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

		/* Draw bitmap in tiled manner to imageDC */
		for (i = ptOrigin.x; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = ptOrigin.y; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(imageDC,  i, j, bmDesc.bmWidth, bmDesc.bmHeight,
					   tempDC, 0, 0, SRCCOPY);

		/*
		   Set the background in memDC to black. Using SRCPAINT with black
		   and any other color results in the other color, thus making black
		   the transparent color
		*/
		SetBkColor(memDC, RGB(0, 0, 0));
		SetTextColor(memDC, RGB(255, 255, 255));
		BitBlt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		/* Set the foreground to black. See comment above. */
		SetBkColor(imageDC, RGB(255, 255, 255));
		SetTextColor(imageDC, RGB(0, 0, 0));
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		/* Combine the foreground with the background */
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   memDC, rcClip.left, rcClip.top, SRCPAINT);

		/* Draw the final image to the screen */
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   imageDC, rcClip.left, rcClip.top, SRCCOPY);

		SelectObject(maskDC, hOldMaskBitmap);
		SelectObject(tempDC, hOldHBitmap);
		SelectObject(imageDC, hOldBmpImage);

		DeleteDC(maskDC);
		DeleteDC(imageDC);
		DeleteDC(tempDC);
		DeleteObject(bmpImage);
		DeleteObject(maskBitmap);
		if (!hPALbg)
		{
			DeleteObject(hPAL);
		}
	}
	SelectObject(memDC, hOldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	EndPaint(hWnd, &ps);
	return 0;
}

// put the arrow on the new sort column
static void ResetHeaderSortIcon(void)
{
	HWND header = ListView_GetHeader(hwndList);
	HD_ITEM hdi;
	int i;

	// take arrow off non-current columns
	hdi.mask = HDI_FORMAT;
	hdi.fmt = HDF_STRING;
	for (i=0;i<COLUMN_MAX;i++)
	{
		if (i != GetSortColumn())
			Header_SetItem(header,GetViewColumnFromRealColumn(i),&hdi);
	}

	if (xpControl)
	{
		// use built in sort arrows
		hdi.mask = HDI_FORMAT;
		hdi.fmt = HDF_STRING | (GetSortReverse() ? HDF_SORTDOWN : HDF_SORTUP);
	}
	else
	{
		// put our arrow icon next to the text
		hdi.mask = HDI_FORMAT | HDI_IMAGE;
		hdi.fmt = HDF_STRING | HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
		hdi.iImage = GetSortReverse() ? 1 : 0;
	}
	Header_SetItem(header,GetViewColumnFromRealColumn(GetSortColumn()),&hdi);
}

// replaces function in src/windows/fileio.c:

int osd_display_loading_rom_message(const char *name,struct rom_load_data *romdata)
{
	int retval;

	if (use_gui_romloading)
	{
		options.gui_host = 1;
		retval = UpdateLoadProgress(name,romdata);
	}
	else
	{
		if (name)
			fprintf (stdout, "loading %-12s\r", name);
		else
			fprintf (stdout, "                    \r");
		fflush (stdout);
		retval = 0;
	}
	
	return retval;
}

static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG :
	{
		char buf[256];
		
		sprintf(buf, "Loading %s", Machine->gamedrv->description);
		SetWindowText(hDlg, buf);
		
		g_bCloseLoading = FALSE;
		g_bAbortLoading = FALSE;

		return 1;
	}

	case WM_CLOSE:
		if (!g_bCloseLoading)
			g_bAbortLoading = TRUE;
		EndDialog(hDlg, 0);
		return 1;

	case WM_COMMAND:
	   if (LOWORD(wParam) == IDCANCEL)
	   {
		   g_bAbortLoading = TRUE;
		   EndDialog(hDlg, IDCANCEL);
		   return 1;
	   }
	   if (LOWORD(wParam) == IDOK)
	   {
		   g_bCloseLoading = TRUE;
		   EndDialog(hDlg, IDOK);
		   return 1;
	   }
	}
	return 0;
}

int UpdateLoadProgress(const char* name, const struct rom_load_data *romdata)
{
	static HWND hWndLoad = 0;
	MSG Msg;

	int current = romdata->romsloaded;
	int total = romdata->romstotal;

	// Check lock if command line
	if ( (! bUseLockGUI) && bPwdEnabled )
	{
		if ( !bPwdVerified && GetGameLockByDriverName((char *)Machine->gamedrv->name) )
		{
			char Tmp[200];
			sprintf(Tmp, "%s [%s]\nPlease verify password to play this game.", Machine->gamedrv->description, Machine->gamedrv->name);
			MessageBox(NULL, Tmp, "Game is locked", MB_OK | MB_ICONEXCLAMATION);
			exit(1);
		}
	}

	if (hWndLoad == NULL)
	{
		hWndLoad = CreateDialog(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_LOAD_PROGRESS),
								hMain,
								LoadProgressDialogProc);

		EnableWindow(GetDlgItem(hWndLoad,IDOK),FALSE);
		EnableWindow(GetDlgItem(hWndLoad,IDCANCEL),TRUE);

		ShowWindow(hWndLoad,SW_SHOW);
	}

	SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_STATUS),
				  ConvertToWindowsNewlines(romdata->errorbuf));

	SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, total));
	SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETPOS, current, 0);

	if (name == NULL)
	{
		// final call to us
		SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_ROMNAME), "");
		if (romdata->errors > 0 || romdata->warnings > 0)
		{
			
			/*
			  Shows the load progress dialog if there is an error while
			  loading the game.
			*/

			ShowWindow(hWndLoad, SW_SHOW);
      
			EnableWindow(GetDlgItem(hWndLoad,IDOK),TRUE);
			if (romdata->errors != 0)
				EnableWindow(GetDlgItem(hWndLoad,IDCANCEL),FALSE);
			SetFocus(GetDlgItem(hWndLoad,IDOK));
			if (romdata->errors)
				SetWindowText(GetDlgItem(hWndLoad,IDC_ERROR_TEXT),
							  "ERROR: required files are missing, the game cannot be run.");
			else
				SetWindowText(GetDlgItem(hWndLoad,IDC_ERROR_TEXT),
							  "WARNING: the game might not run correctly.");
		}
	}
	else
		SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_ROMNAME), name);

	if (name == NULL && (romdata->errors > 0 || romdata->warnings > 0))
	{
		while (GetMessage(&Msg, NULL, 0, 0))
		{
			if (!IsDialogMessage(hWndLoad, &Msg))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}

			// make sure the user clicks-through on an error/warning
			if (g_bCloseLoading || g_bAbortLoading)
				break;
		}

	}

	if (name == NULL)
		DestroyWindow(hWndLoad);

	// take care of any pending messages
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!IsDialogMessage(hWndLoad, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	// if abort with a warning, gotta exit abruptly
	if (name == NULL && g_bAbortLoading && romdata->errors == 0)
		exit(1);

	// if abort along the way, tell 'em
	if (g_bAbortLoading && name != NULL)
	{
		exit(1);
	}

	return 0;
}

void RemoveCurrentGameCustomFolder(void)
{
    RemoveGameCustomFolder(GetSelectedPickItem());
}

void RemoveGameCustomFolder(int driver_index)
{
    int i;
	TREEFOLDER **folders;
	int num_folders;

	GetFolders(&folders,&num_folders);
	
	for (i=0;i<num_folders;i++)
	{
	    if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
		    int current_pick_index;

		    RemoveFromCustomFolder(folders[i],driver_index);

			if (driver_index == GetSelectedPickItem())
			{
			   /* if we just removed the current game,
				  move the current selection so that when we rebuild the listview it
				  leaves the cursor on next or previous one */
			
			   current_pick_index = GetSelectedPick();
			   SetSelectedPick(GetSelectedPick() + 1);
			   if (current_pick_index == GetSelectedPick()) /* we must have deleted the last item */
				  SetSelectedPick(GetSelectedPick() - 1);
			}

			ResetListView();
			return;
		}
	}
	MessageBox(GetMainWindow(), "Error searching for custom folder", MAME32NAME, MB_OK | MB_ICONERROR);

}


void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
    LV_ITEM lvi;
	POINT pt;

	lvi.iItem = pnmv->iItem;
	lvi.mask	 = LVIF_PARAM;
	ListView_GetItem(hwndList, &lvi);

	game_dragged = lvi.lParam;

	pt.x = 0;
	pt.y = 0;

	/* Tell the list view control to create an image to use 
	   for dragging. */
    himl_drag = ListView_CreateDragImage(hwndList,pnmv->iItem,&pt);
 
    /* Start the drag operation. */
    ImageList_BeginDrag(himl_drag, 0, 0, 0); 

	pt = pnmv->ptAction;
	ClientToScreen(hwndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(),pt.x,pt.y);

    /* Hide the mouse cursor, and direct mouse input to the 
	   parent window. */
    SetCapture(hMain);

	prev_drag_drop_target = NULL;

    g_listview_dragging = TRUE; 

}

void MouseMoveListViewDrag(POINTS p)
{
   HTREEITEM htiTarget;
   TV_HITTESTINFO tvht;

   POINT pt;
   pt.x = p.x;
   pt.y = p.y;

   ClientToScreen(hMain,&pt);

   ImageList_DragMove(pt.x,pt.y);

   MapWindowPoints(GetDesktopWindow(),hTreeView,&pt,1);

   tvht.pt = pt;
   htiTarget = TreeView_HitTest(hTreeView,&tvht);

   if (htiTarget != prev_drag_drop_target)
   {
	   ImageList_DragShowNolock(FALSE);
	   if (htiTarget != NULL)
		   TreeView_SelectDropTarget(hTreeView,htiTarget);
	   else
		   TreeView_SelectDropTarget(hTreeView,NULL);
	   ImageList_DragShowNolock(TRUE);
	   
	   prev_drag_drop_target = htiTarget;
   }
}

void ButtonUpListViewDrag(POINTS p)
{
    POINT pt;
    HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	TVITEM tvi;
	
	ReleaseCapture();

    ImageList_DragLeave(hwndList);
    ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	TreeView_SelectDropTarget(hTreeView,NULL);

	g_listview_dragging = FALSE;

	/* see where the game was dragged */

	pt.x = p.x;
	pt.y = p.y;

	MapWindowPoints(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);
	if (htiTarget == NULL)
	{
	   LVHITTESTINFO lvhtti;
	   LPTREEFOLDER folder;
	   RECT rcList;

	   /* the user dragged a game onto something other than the treeview */
	   /* try to remove if we're in a custom folder */

	   /* see if it was dragged within the list view; if so, ignore */

	   MapWindowPoints(hTreeView,hwndList,&pt,1);
	   lvhtti.pt = pt;
	   GetWindowRect(hwndList, &rcList);
	   ClientToScreen(hwndList, &pt);
	   if( PtInRect(&rcList, pt) != 0 )
		   return;

	   folder = GetCurrentFolder();
	   if (folder->m_dwFlags & F_CUSTOM)
	   {
		   /* dragged out of a custom folder, so let's remove it */
		   RemoveCurrentGameCustomFolder();
	   }
	   return;
	}


	tvi.lParam = 0;
	tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (TreeView_GetItem(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder,game_dragged);
	}

}

static LPTREEFOLDER GetSelectedFolder(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	htree = TreeView_GetSelection(hTreeView);
	if(htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}
	return NULL;
}

static HICON GetSelectedFolderIcon(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	HIMAGELIST hSmall_icon;
	LPTREEFOLDER folder;
	htree = TreeView_GetSelection(hTreeView);

	if (htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);
		
		folder = (LPTREEFOLDER)tvi.lParam;
		//hSmall_icon = TreeView_GetImageList(hTreeView,(int)tvi.iImage);
		hSmall_icon = NULL;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return NULL;
}

/* Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	if( (GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_DETAILS ) )
		ListView_RedrawItems(hwndList,ListView_GetTopIndex(hwndList),
							 ListView_GetTopIndex(hwndList)+ ListView_GetCountPerPage(hwndList) );
}

void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height)
{
	int 	destX, destY;
	int 	destW, destH;
	RECT	rect;
	/* for scaling */		 
	int x, y;
	int rWidth, rHeight;
	double scale;
	BOOL bReduce = FALSE;

	GetClientRect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;
		GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}

	rWidth	= (rect.right  - rect.left);
	rHeight = (rect.bottom - rect.top);

	/* Limit the screen shot to max height of 264 */
	if (restrict_height == TRUE && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	/* If the bitmap does NOT fit in the screenshot area */
	if (x > rWidth - 10 || y > rHeight - 10)
	{
		rect.right	-= 10;
		rect.bottom -= 10;
		rWidth	-= 10;
		rHeight -= 10;
		bReduce = TRUE;
		/* Try to scale it properly */
		/*	assumes square pixels, doesn't consider aspect ratio */
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		/* If it's still too big, scale again */
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth	/ destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		if (GetStretchScreenShotLarger())
		{
			rect.right	-= 10;
			rect.bottom -= 10;
			rWidth	-= 10;
			rHeight -= 10;
			bReduce = TRUE;
			// Try to scale it properly
			// assumes square pixels, doesn't consider aspect ratio
			if (x < y)
				scale = (double)rWidth / x;
			else
				scale = (double)rHeight / y;
			
			destW = (int)(x * scale);
			destH = (int)(y * scale);
			
			// If it's too big, scale again
			if (destW > rWidth || destH > rHeight)
			{
				if (destW > rWidth)
					scale = (double)rWidth	/ destW;
				else
					scale = (double)rHeight / destH;
				
				destW = (int)(destW * scale);
				destH = (int)(destH * scale);
			}
		}
		else
		{
			// Use the bitmaps size if it fits
			destW = x;
			destH = y;
		}

	}


	destX = ((rWidth  - destW) / 2);
	destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}

	pRect->left   = destX;
	pRect->top	  = destY;
	pRect->right  = destX + destW;
	pRect->bottom = destY + destH;
}

/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically. 
*/

void SwitchFullScreenMode(void)
{
	LONG lMainStyle;
	
	if (GetRunFullScreen())
	{
		// Return to normal

		// Restore the menu
		SetMenu(hMain, LoadMenu(hInst,MAKEINTRESOURCE(IDR_UI_MENU)));
		
		// Refresh the checkmarks
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, GetShowFolderList() ? MF_CHECKED : MF_UNCHECKED); 
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);    
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);
		
		// Add frame to dialog again
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle | WS_BORDER;
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		
		// Show the window maximized
		if( GetWindowState() == SW_MAXIMIZE )
		{
			ShowWindow(hMain, SW_NORMAL);
			ShowWindow(hMain, SW_MAXIMIZE);
		}
		else
			ShowWindow(hMain, SW_RESTORE);

		SetRunFullScreen(FALSE);
	}
	else
	{
		// Set to fullscreen
		
		// Remove menu
		SetMenu(hMain,NULL); 

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		if( IsMaximized(hMain) )
		{
			ShowWindow(hMain, SW_NORMAL);
			SetWindowState( SW_MAXIMIZE );
		}
		ShowWindow(hMain, SW_MAXIMIZE);
		
		SetRunFullScreen(TRUE);
	}
}

/*
  Checks to see if the mouse has been moved since this func
  was first called (which is at startup). The reason for 
  storing the startup coordinates of the mouse is that when
  a window is created it generates WM_MOUSEOVER events, even
  though the user didn't actually move the mouse. So we need
  to know when the WM_MOUSEOVER event is user-triggered.

  POSSIBLE BUGS:
  Gets polled at every WM_MOUSEMOVE so it might cause lag,
  but there's probably another way to code this that's 
  way better?
  
*/

BOOL MouseHasBeenMoved(void)
{
    static int mouse_x = -1;
    static int mouse_y = -1;
	POINT p;

	GetCursorPos(&p);

    if (mouse_x == -1) // First time
    {
		mouse_x = p.x;
		mouse_y = p.y;
    }
	
	return (p.x != mouse_x || p.y != mouse_y);       
}


void SendIconToProcess(LPPROCESS_INFORMATION pi, int nGameIndex)
{
	HICON hIcon; 
	hIcon = LoadIconFromFile(drivers[nGameIndex]->name); 
	if( hIcon == NULL ) 
	{ 
		//Check if clone, if so try parent icon first 
		if( DriverIsClone(nGameIndex) ) 
		{ 
			hIcon = LoadIconFromFile(drivers[nGameIndex]->clone_of->name); 
			if( hIcon == NULL) 
			{ 
				hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MIM32_ICON)); 
			} 
		} 
		else 
		{ 
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MIM32_ICON)); 
		} 
	} 
	WaitForInputIdle( pi->hProcess, INFINITE ); 
	SendMessageToProcess( pi, WM_SETICON, ICON_SMALL, (LPARAM)hIcon ); 
	SendMessageToProcess( pi, WM_SETICON, ICON_BIG, (LPARAM)hIcon ); 
}
/*
	The following two functions enable us to send Messages to the Game Window
*/

void SendMessageToProcess(LPPROCESS_INFORMATION lpProcessInformation, 
                                      UINT Msg, WPARAM wParam, LPARAM lParam)
{
	FINDWINDOWHANDLE fwhs;
	fwhs.ProcessInfo = lpProcessInformation;
	fwhs.hwndFound  = NULL;

	EnumWindows(EnumWindowCallBack, (LPARAM)&fwhs);

	SendMessage(fwhs.hwndFound, Msg, wParam, lParam);
	//Fix for loosing Focus, we reset the Focus on our own Main window
	SendMessage(fwhs.hwndFound, WM_KILLFOCUS,(WPARAM) hMain, (LPARAM) NULL);
}

BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam) 
{ 
	FINDWINDOWHANDLE * pfwhs = (FINDWINDOWHANDLE * )lParam; 
	DWORD ProcessId; 
	char buffer[MAX_PATH]; 

	GetWindowThreadProcessId(hwnd, &ProcessId);

	// cmk--I'm not sure I believe this note is necessary
	// note: In order to make sure we have the MainFrame, verify that the title 
	// has Zero-Length. Under Windows 98, sometimes the Client Window ( which doesn't 
	// have a title ) is enumerated before the MainFrame 

	GetWindowText(hwnd, buffer, sizeof(buffer));
	if (ProcessId  == pfwhs->ProcessInfo->dwProcessId &&
		 strncmp(buffer,MAMENAME,strlen(MAMENAME)) == 0) 
	{ 
		pfwhs->hwndFound = hwnd; 
		return FALSE; 
	} 
	else 
	{ 
		// Keep enumerating 
		return TRUE; 
	} 
}

static void SaveGameListToFile(char *szFile, int Formatted)
{
    int Order[COLUMN_MAX];
    int Size[COLUMN_MAX] = {70, 4, 8, 9, 6, 9, 6, 30, 4, 70, 12, 9};
    int nColumnMax = GetNumColumns(hwndList);
    int i, j = 0;

    const char *Filters[8] = { "Clones", "Non-Working", "Unavailable", "Vector", "Raster", "Originals", "Working", "Available" };
    char *CrLf;
    char Buf[300];

    LPTREEFOLDER lpFolder = GetCurrentFolder();
    LV_ITEM lvi;

    FILE *fp = fopen(szFile, "wt");

    // No interline with tabuled format
    if ( Formatted )
        CrLf = (char *)"\r\n\r\n";
    else
        CrLf = (char *)"\r\n";
    
    if (fp == NULL)
    {
        MessageBox(GetMainWindow(), "Error : unable to access file", MAME32NAME, MB_OK | MB_ICONERROR);
        return;
    }

    GetRealColumnOrder(Order);

    // Title
    sprintf( Buf, "%s %s.%s", MAME32NAME, GetVersionString(), CrLf );
    fwrite( Buf, strlen(Buf), 1, fp);
    sprintf( Buf, "This is the current list of games, as displayed in the GUI (%s view mode).%s", ((GetViewMode() == VIEW_GROUPED)?"grouped":"detail"), CrLf );
    fwrite( Buf, strlen(Buf), 1, fp);

    // Current folder
    sprintf( Buf, "Current folder : <" );
    if ( lpFolder->m_nParent != -1 )
    {
        // Shows only 2 levels (last and previous)
        LPTREEFOLDER lpF = GetFolder( lpFolder->m_nParent );

        if ( lpF->m_nParent == -1 )
        {
            strcat( Buf, "\\" );
        }
        strcat( Buf, lpF->m_lpTitle );
        strcat( Buf, "\\" );
    }
    else
    {
        strcat( Buf, "\\" );
    }
    sprintf( &Buf[strlen(Buf)], "%s>%s.%s", lpFolder->m_lpTitle, (lpFolder->m_dwFlags&F_CUSTOM?" (custom folder)":""), CrLf );
    fwrite( Buf, strlen(Buf), 1, fp);

    // Filters
    sprintf(Buf, "Additional excluding filter(s) : ");
    for (i=0,j=0; i<8; i++ )
    {
        if ( lpFolder->m_dwFlags & (1<<i) )
        {
            if ( j > 0)
            {
                strcat( Buf, ", ");
            }

            strcat(Buf, Filters[i]);

            j++;
        }
    }
    if ( j == 0)
    {
        strcat(Buf, "none");
    }
    strcat(Buf, ".");
    strcat(Buf, CrLf);
    fwrite( Buf, strlen(Buf), 1, fp);

    // Sorting
    if ( GetSortColumn() > 0 )
    {
        sprintf( Buf, "Sorted by <%s> descending order", column_names[GetSortColumn()] );
    }
    else
    {
        sprintf( Buf, "Sorted by <%s> ascending order", column_names[-GetSortColumn()] );
    }
    sprintf( &Buf[strlen(Buf)], ", %d game(s) found.%s", ListView_GetItemCount(hwndList), CrLf );
    fwrite( Buf, strlen(Buf), 1, fp);

    if ( Formatted )
    {
        // Separator
        memset( Buf, '-', sizeof(Buf) );
        Buf[0] = '+';
        for (i=0,j=0; i<nColumnMax; i++ )
        {
            j += Size[Order[i]]+3;
            Buf[j] = '+';
        }
        Buf[j+1] = '\0';
        strcat( Buf, "\r\n");
        fwrite( Buf, strlen(Buf), 1, fp);

        // Left side of columns title
        Buf[0] = '|';
        Buf[1] = '\0';
    }
    else
        Buf[0] = '\0';

    // Title of columns
    for (i=0; i<nColumnMax; i++ )
    {
        if ( Formatted )
            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], column_names[Order[i]] );
        else
        {
            if ( i )
                sprintf( &Buf[strlen(Buf)], "\t%s", column_names[Order[i]] );
            else
                sprintf( &Buf[strlen(Buf)], "%s", column_names[Order[i]] );
        }
    }
    strcat( Buf, "\r\n");
    fwrite( Buf, strlen(Buf), 1, fp);

    // Separator
    if ( Formatted )
    {
        memset( Buf, '-', sizeof(Buf) );
        Buf[0] = '+';
        for (i=0,j=0; i<nColumnMax; i++ )
        {
            j += Size[Order[i]]+3;
            Buf[j] = '+';
        }
        Buf[j+1] = '\0';
        strcat( Buf, "\r\n");
        fwrite( Buf, strlen(Buf), 1, fp);
    }

    // Games
    lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL);
    while( lvi.iItem != -1 )
    {
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;

        if ( ListView_GetItem(hwndList, &lvi) )
        {
            if ( Formatted )
            {
                Buf[0] = '|';
                Buf[1] = '\0';
            }
            else
                Buf[0] = '\0';

            // lvi.lParam contains the absolute game index
            for (i=0; i<nColumnMax; i++ )
            {
                if ((i > 1) && (! Formatted))
                    strcat(&Buf[strlen(Buf)], "\t");

                switch( Order[i] )
                {
                    case  0: // Name
                        if ( Formatted )
                        {
                            if ( DriverIsClone(lvi.lParam) && (GetViewMode() == VIEW_GROUPED) )
                            {
                                sprintf( &Buf[strlen(Buf)], "    %-*.*s |", Size[Order[i]]-3, Size[Order[i]]-3, ModifyThe(drivers[lvi.lParam]->description) );
                            }
                            else
                            {
                                sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], ModifyThe(drivers[lvi.lParam]->description) );
                            }
                        }
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", ModifyThe(drivers[lvi.lParam]->description) );
                        break;

                    case  1: // ROMs
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], (GetRomAuditResults(lvi.lParam)==TRUE?"no":"yes") );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", (GetRomAuditResults(lvi.lParam)==TRUE?"no":"yes") );
                        break;

                    case  2: // Samples
                        if (DriverUsesSamples(lvi.lParam))
                        {
                            if ( Formatted )
                                sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], (GetSampleAuditResults(lvi.lParam)?"no":"yes") );
                            else
                                sprintf( &Buf[strlen(Buf)], "%s", (GetSampleAuditResults(lvi.lParam)?"no":"yes") );
                        }
                        else
                        {
                            if ( Formatted )
                                sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], "" );
                        }
                        break;

                    case  3: // Directory
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], drivers[lvi.lParam]->name );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", drivers[lvi.lParam]->name );
                        break;

                    case  4: // Type
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], (DriverIsVector(lvi.lParam)?"Vector":"Raster") );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", (DriverIsVector(lvi.lParam)?"Vector":"Raster") );
                        break;

                    case  5: // Trackball
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], (DriverUsesTrackball(lvi.lParam)?"yes":"no") );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", (DriverUsesTrackball(lvi.lParam)?"yes":"no") );
                        break;

                    case  6: // Played
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*d |", Size[Order[i]], GetPlayCount(lvi.lParam) );
                        else
                            sprintf( &Buf[strlen(Buf)], "%d", GetPlayCount(lvi.lParam) );
                        break;

                    case  7: // Manufacturer
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], drivers[lvi.lParam]->manufacturer );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", drivers[lvi.lParam]->manufacturer );
                        break;

                    case  8: // Year
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], drivers[lvi.lParam]->year );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", drivers[lvi.lParam]->year );
                        break;

                    case  9: // Clone of
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], GetCloneParentName(lvi.lParam) );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", GetCloneParentName(lvi.lParam) );
                        break;

                    case 10: // Source
                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], GetDriverFilename(lvi.lParam) );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", GetDriverFilename(lvi.lParam) );
                        break;

                    case 11: // Play time
                    {
                        char Tmp[20];
                        
                        GetTextPlayTime(lvi.lParam, Tmp);

                        if ( Formatted )
                            sprintf( &Buf[strlen(Buf)], " %-*.*s |", Size[Order[i]], Size[Order[i]], Tmp );
                        else
                            sprintf( &Buf[strlen(Buf)], "%s", Tmp );

                        break;
                    }
                }
            }
            strcat( Buf, "\r\n");
            fwrite( Buf, strlen(Buf), 1, fp);
        }

        lvi.iItem = ListView_GetNextItem(hwndList, lvi.iItem, LVNI_BELOW);
    }

    // Last separator
    if ( Formatted && (ListView_GetItemCount(hwndList) > 0) )
    {
        memset( Buf, '-', sizeof(Buf) );
        Buf[0] = '+';
        for (i=0,j=0; i<nColumnMax; i++ )
        {
            j += Size[Order[i]]+3;
            Buf[j] = '+';
        }
        Buf[j+1] = '\0';
        strcat( Buf, "\r\n");
        fwrite( Buf, strlen(Buf), 1, fp);
    }

    fclose(fp);
    
    SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)"File created");
}

BOOL GameIsLocked(int game)
{
	static int iGame = -1;
	static BOOL bLock = FALSE;
	char *p = tcLockUnlockList;
	char szGame[10];

	// Reinitialise memory
	if ( game == -1 )
	{
		iGame = -1;
		bLock = FALSE;
		return FALSE;
	}

	// Game hasn't changed since last call
	if ( game == iGame )
	{
		return bLock;
	}
	
	iGame = game;

	while( *p != 0 )
	{
		memcpy(szGame, &p[2], p[1]-'0');
		szGame[(int)p[1] - '0'] = '\0';

		if ( strcmp(drivers[game]->name, szGame) == 0 )
		{
			bLock = (p[0] == '*' ? TRUE : FALSE);
			
			// Locked game
			return bLock;
		}

		// Next game in list
		p += 2 + p[1] - '0';
	}

#if (NEW_GAMES_LOCKED == FALSE)
	// Game is not found, consider it as unlocked
	bLock = FALSE;
#else
	// Game is not found, consider it as locked
	bLock = TRUE;
#endif

	return bLock;
}

static void SetGameLockUnlock(int game, BOOL locked)
{
	char *p = tcLockUnlockList;
	char szGame[10];
	int num_folders = 0;

	while( *p != 0 )
	{
		memcpy(szGame, &p[2], p[1]-'0');
		szGame[(int)p[1] - '0'] = '\0';

		if ( strcmp(drivers[game]->name, szGame) == 0 )
		{
			// If game state changes, update folders
			if ( ((p[0] == '*') && !locked) || ((p[0] != '*') && locked) )
			{
				num_folders = 1;
			}
			
			p[0] = (locked ? '*' : ' ');
			
			break;
		}

		// Next game in list
		p += 2 + p[1] - '0';
	}

	// Game is not found, add it to list
	if ( *p == '\0' )
	{
		// By default, new games are unlocked -> update folder
		if ( locked )
		{
			num_folders = 1;
		}

		p[0] = (locked ? '*' : ' ');
		p[1] = '0' + strlen(drivers[game]->name);
		memcpy(&p[2], drivers[game]->name, strlen(drivers[game]->name) );
		p[2+(int)p[1]-'0'] = '\0';
	}

	// Update lock/unlock folders
	if ( num_folders )
	{
		LPTREEFOLDER FolderLocked = GetFolderByID(FOLDER_LOCKED);
		LPTREEFOLDER FolderUnlocked = GetFolderByID(FOLDER_UNLOCKED);

		if ( locked )
		{
			AddGame(FolderLocked, game);
			RemoveGame(FolderUnlocked, game);
		}
		else
		{
			RemoveGame(FolderLocked, game);
			AddGame(FolderUnlocked, game);
		}
	}
}

static void SetFolderLockUnlock(LPTREEFOLDER lpFolder, BOOL bLock)
{
	int i, j;
	int NbGames = 0;
	char *pGameList = (char *)malloc(game_count);
	TREEFOLDER **folders;
	int num_folders;

	GetFolders(&folders, &num_folders);

	ZeroMemory(pGameList, game_count);

	// Scan current folder
	i = -1;
	while( (i = FindGame(lpFolder, i+1)) != -1 )
	{
		if ( GameFiltered(i, lpFolder->m_dwFlags) )
		{
			continue;
		}

		pGameList[i] = 1;
		NbGames++;
	}

	// Scan child folder(s)
	for ( j=0; j<num_folders; j++ )
	{
		if ( GetFolder(folders[j]->m_nParent) == lpFolder )
		{
			i = -1;
			while( (i = FindGame(folders[j], i+1)) != -1 )
			{
				if ( GameFiltered(i, folders[j]->m_dwFlags) )
				{
					continue;
				}
		
				pGameList[i] = 1;
				NbGames++;
			}
		}
	}

	// Lock/unlock games found
	for (i=0; i<game_count; i++ )
	{
		if ( pGameList[i] )
		{
			SetGameLockUnlock(i, bLock );
		}
	}

	ResetListView();
	
	sprintf( pGameList, "Done, %d game(s) %s.", NbGames, (bLock?"locked":"unlocked"));

	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)pGameList);

	free(pGameList);
}

static void CreateDefaultLockUnlockList(void)
{
	char *p = tcLockUnlockList;
	int i;

	// Create a default list with locked games
	for ( i=0; i<game_count; i++ )
	{
#if (ALL_GAMES_LOCKED == FALSE)
			p[0] = ' ';
#else
			p[0] = '*';
#endif
			p[1] = '0' + strlen(drivers[i]->name);
			memcpy(&p[2], drivers[i]->name, strlen(drivers[i]->name));
			
			p += 2 + p[1] - '0';
	}
	
	*p = '\0';
}

static BOOL CreateAndLoadLockUnlockList(void)
{
	long filelen;
	long ListLen;
	FILE *fList;
	char *pList;
	char *p = (char *)&filelen;
	int i;
	
	// Allocate for the largest list (10 = 1+1+8)
	tcLockUnlockList = (char *)malloc(10*game_count+1);

	fList = fopen(LIST_FILENAME, "rb");

	if ( fList == NULL )
	{
		return FALSE;
	}

	fseek(fList, 0, SEEK_END);
	ListLen = ftell(fList);
	fseek(fList, 0, SEEK_SET);
	
	if ( ListLen < sizeof(filelen) )
	{
		fclose(fList);
		return FALSE;
	}

	fread(&filelen, sizeof(filelen), 1, fList);
	ListLen -= sizeof(filelen);

	pList = (char *)malloc(ListLen);

	if ( fread(pList, ListLen, 1, fList) != 1 )
	{
		free(pList);
		fclose(fList);
		return FALSE;
	}

	fclose(fList);

	// Decode length
	for ( i=0; i<sizeof(filelen)-1; i++ )
	{
		p[i] ^= p[i+1];
	}
	p[i] ^= pList[0];

	// Decode list
	for ( i=0; i<(int)ListLen-1; i++ )
	{
		pList[i] ^= pList[i+1];
	}
	pList[i] ^= 0xA5;

	filelen ^= (ListLen+sizeof(filelen)) ^ 0x21041971;

	// File is corrupt
	if ( filelen != 0 )
	{
		free(pList);
		return FALSE;
	}

	// If a list is found, it replaces the default list
	ZeroMemory(tcLockUnlockList, 10*game_count+1);
	memcpy(tcLockUnlockList, pList, ListLen);
	tcLockUnlockList[ListLen] = '\0';

	free(pList);

	return TRUE;
}

// This function is only used with command line
static BOOL GetGameLockByDriverName(char *DriverName)
{
	BOOL bLock;
	char *p;
	char szGame[10];

#if (NEW_GAMES_LOCKED == FALSE)
	bLock = FALSE;
#else
	bLock = TRUE;
#endif
	
	if ( ! CreateAndLoadLockUnlockList() )
	{
		// Unable to get game lock state -> game is locked
		free(tcLockUnlockList);
#if (ALL_GAMES_LOCKED == FALSE)
		return FALSE;
#else
		return TRUE;
#endif
	}

	// List is now in memory, search for selected driver
	p = tcLockUnlockList;
	while( *p != 0 )
	{
		memcpy(szGame, &p[2], p[1]-'0');
		szGame[(int)p[1] - '0'] = '\0';

		// Game found
		if ( strcmp(DriverName, szGame) == 0 )
		{
			bLock = (p[0] == '*' ? TRUE : FALSE);
			break;
		}

		// Next game in list
		p += 2 + p[1] - '0';
	}

	// Don't need the list anymore	
	free(tcLockUnlockList);
	return bLock;
}

static void SaveAndDestroyLockUnlockList(void)
{
	char chk = 0xA5;
	long filelen;
	FILE *fList;
	char *p = (char *)&filelen;
	int ListLen = strlen(tcLockUnlockList);
	int i = strlen(tcLockUnlockList);

	// Encode list
	while( i-- )
	{
		tcLockUnlockList[i] ^= chk;
		chk = tcLockUnlockList[i];
	}

	filelen = (sizeof(filelen) + ListLen) ^ 0x21041971;

	// Encode length
	i = sizeof(filelen);
	while( i-- )
	{
		p[i] ^= chk;
		chk = p[i];
	}

	fList = fopen(LIST_FILENAME, "wb");

	if ( fList != NULL )
	{
		fwrite(p, sizeof(filelen), 1, fList);
		fwrite(tcLockUnlockList, ListLen, 1, fList);
		fclose(fList);
	}
	
	free(tcLockUnlockList);
}

static void CheckPassword(char *Exe, char *Pwd)
{
	char buffer[512];
	FILE *fptr;
	char *p;
	BOOL bFound = FALSE;

	strcpy(buffer, Exe);
	p = buffer+strlen(Exe);
	
	while( (*p != '\\') && (*p != ':') )
	{
		*p = '\0';

		if ( p == buffer )
		{
			break;
		}

		p--;
	}

	// Warning : the ini filename is defined in options.c too
	strcat(p, MAME32NAME "ui.ini");

	fptr = fopen(buffer,"rt");

	if (fptr != NULL)
	{
		// Quick parsing of ini file
		while ( fgets(buffer, sizeof(buffer), fptr) != NULL )
		{
			char *key, *value_str;

			if (buffer[0] == '\0' || buffer[0] == '#')
				continue;

			// we're guaranteed that strlen(buffer) >= 1 now
			buffer[strlen(buffer)-1] = '\0';

			ParseKeyValueStrings(buffer, &key, &value_str);

			if (key == NULL || value_str == NULL)
				continue;

			if ( strcmp(key, "password") == 0 )
			{
				PasswordDecodeString(value_str, &p);

				if ( *p != '*' )
				{
					bPwdVerified = TRUE;
				}
				else
				{
					if ( Pwd != NULL )
					{
						if ( strcmp(p+1, Pwd) == 0 )
						{
							bPwdVerified = TRUE;
						}
					}
				}
				
				bFound = TRUE;
				free(p);
				break;
			}
		}
		
#if (DEFAULT_PWD_ENABLED == FALSE)
		if ( !bFound )
		{
			// No password entry in ini file
			bPwdEnabled = TRUE;
		}
#endif
		
		fclose(fptr);
	}
#if (DEFAULT_PWD_ENABLED == FALSE)
	else
	{
		// No ini file
		bPwdEnabled = FALSE;
	}
#endif
}

BOOL GameIsFound(int game)
{
	return (games_found[game]==0 ? FALSE : TRUE);
}

static void CreateBackgroundMain(HINSTANCE hInstance, BOOL ForCreate )
{
	static HDC  hSplashDC = 0;

	if ( ForCreate )
	{
		WNDCLASSEX BackMainClass;
	
		BackMainClass.cbSize           = sizeof(WNDCLASSEX);
		BackMainClass.style            = 0;
		BackMainClass.lpfnWndProc      = (WNDPROC)BackMainWndProc;
		BackMainClass.cbClsExtra       = 0;
		BackMainClass.cbWndExtra       = 0;
		BackMainClass.hInstance        = hInstance;
		BackMainClass.hIcon            = NULL;
		BackMainClass.hIconSm          = NULL;
		BackMainClass.hCursor          = NULL;
		BackMainClass.hbrBackground    = NULL;
		BackMainClass.lpszMenuName     = NULL;
		BackMainClass.lpszClassName    = "BackMainWindowClass";

		if ( RegisterClassEx(&BackMainClass) )
		{
			BITMAP Bitmap;
			RECT DesktopRect;
			GetWindowRect(GetDesktopWindow(), &DesktopRect);

			hSplashBmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SPLASH));
			GetObject(hSplashBmp, sizeof(BITMAP), &Bitmap);

			hBackMain = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			"BackMainWindowClass",
			"Main Backgound windows",
			WS_POPUP,
			(DesktopRect.right - Bitmap.bmWidth) / 2,
			(DesktopRect.bottom - Bitmap.bmHeight) / 2,
			Bitmap.bmWidth,
			Bitmap.bmHeight,
			NULL,
			NULL,
			hInstance,
			NULL);

			hSplashDC = GetDC(hBackMain);
			hMemoryDC = CreateCompatibleDC(hSplashDC);
			SelectObject(hMemoryDC, (HGDIOBJ)hSplashBmp);

			ShowWindow(hBackMain, SW_SHOW);
			UpdateWindow(hBackMain);

			uShellIconMsg = RegisterWindowMessage( "Mame32 Shell Icon" );
			MameIcon.cbSize = sizeof(NOTIFYICONDATA);
			MameIcon.hWnd = hBackMain;
			MameIcon.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SYSTRAY));
			MameIcon.uID = 1;
			MameIcon.uCallbackMessage = uShellIconMsg;
			MameIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		}
	}
	else
	{
		if ( hBackMain )
		{
			DeleteObject(hSplashBmp);
			ReleaseDC(hBackMain, hSplashDC);
			ReleaseDC(hBackMain, hMemoryDC);
			DestroyWindow(hBackMain);
		}
	}
}

static int GetGamesFound(char *Pattern, int Criteria)
{
	char *pDescription = NULL;
	int nGameIndex = 0;
	int i=0;
	int iStep = 0;
	LV_FINDINFO lvfi;
	int nCount = ListView_GetItemCount(hwndList);
	BOOL changed = FALSE;

	MSG msg;
	
	BOOL bFound;
	int iFound = 0;

	ZeroMemory(games_found,sizeof(char) * game_count);

	if( nCount > 0)
		ProgressBarShow();
	else
		return 0;

	if( nCount < 100 )
		iStep = 100 / nCount;
	else
		iStep = nCount/100;

	UpdateListView();
	UpdateWindow(hMain);
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		
		bFound = FALSE;

		if (Criteria & 1)
		{
			if (CompareStringWithWildcards((char *)drivers[nGameIndex]->description, Pattern))
			{
				bFound = TRUE;
			}
		}

		if (Criteria & 2)
		{
			if (CompareStringWithWildcards((char *)drivers[nGameIndex]->name, Pattern))
			{
				bFound = TRUE;
			}
		}

		if (Criteria & 4)
		{
			if (CompareStringWithWildcards((char *)drivers[nGameIndex]->manufacturer, Pattern))
			{
				bFound = TRUE;
			}
		}

		if (bFound)
		{
	 		games_found[nGameIndex] = 1;
	 		iFound++;
	 	}

		lvfi.flags	= LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		i = ListView_FindItem(hwndList, -1, &lvfi);
		if (changed && i != -1);
		{
			ListView_RedrawItems(hwndList, i, i);
			while( PeekMessage( &msg, hwndList, 0, 0, PM_REMOVE ) != 0)
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			}
		}
		changed = FALSE;
		if ((i % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	pDescription = ModifyThe(drivers[GetSelectedPickItem()]->description);
	SetStatusBarText(0, pDescription);
	UpdateStatusBar();
	return iFound;
}

/* End of source file */
