/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  layout.c

  MAME specific TreeView definitions (and maybe more in the future)

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>  /* for sprintf */
#include <stdlib.h> /* For malloc and free */
#include <string.h>

#include "bitmask.h"
#include "TreeView.h"
#include "M32Util.h"
#include "resource.h"
#include "directories.h"
#include "options.h"
#include "splitters.h"
#include "help.h"
#include "audit32.h"
#include "screenshot.h"
#include "win32ui.h"
#include "properties.h"

static BOOL FilterAvailable(int driver_index);

FOLDERDATA g_folderData[] =
{
	{"All Games",       "allgames",          FOLDER_ALLGAMES,     IDI_FOLDER,				0,             0,            NULL,                       NULL,              TRUE },
	{"Available",       "available",         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   F_UNAVAILABLE,NULL,                       FilterAvailable,              TRUE },
	{"Unavailable",     "unavailable",       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,	F_UNAVAILABLE, F_AVAILABLE,  NULL,                       FilterAvailable,              FALSE },
	{"Working",         "working",           FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,     F_NONWORKING, NULL,                       DriverIsBroken,    FALSE },
	{"Non-Working",     "nonworking",        FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING,  F_WORKING,    NULL,                       DriverIsBroken,    TRUE },
	{"Imperfect",       "imperfect",         FOLDER_DEFICIENCY,   IDI_FOLDER_DEFICIENCY,    0,             0,            CreateDeficiencyFolders },
	{"Manufacturer",    "manufacturer",      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            CreateManufacturerFolders },
	{"Year",            "year",              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            CreateYearFolders },
	{"Source",          "source",            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            CreateSourceFolders },
	{"Processor",       "cpu",               FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,            CreateCPUFolders },
	{"Sound",           "sound",             FOLDER_SND,          IDI_FOLDER_SOUND,         0,             0,            CreateSoundFolders },
	{"Orientation",     "orientation",       FOLDER_ORIENTATION,  IDI_FOLDER_ORIENTATION,   0,             0,            CreateOrientationFolders },
	{"Bios",            "bios",              FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,             0,            CreateBiosFolders },
	{"Screen",          "screen",            FOLDER_SCREEN,       IDI_FOLDER_SCREEN,        0,             0,            CreateScreenFolders },
	{"Colors",          "colors",            FOLDER_COLORS,       IDI_FOLDER_COLORS,        0,             0,            CreateColorsFolders },
	{"Refresh",         "refresh",           FOLDER_REFRESH,      IDI_FOLDER_REFRESH,       0,             0,            CreateRefreshFolders },
	{"Originals",       "originals",         FOLDER_ORIGINAL,     IDI_ORIGINALS,            F_ORIGINALS,   F_CLONES,     NULL,                       DriverIsClone,     FALSE },
	{"Clones",          "clones",            FOLDER_CLONES,       IDI_CLONES,               F_CLONES,      F_ORIGINALS,  NULL,                       DriverIsClone,     TRUE },
	{"Locked",          "locked",            FOLDER_LOCKED,       IDI_LOCK,                 0,             0,            NULL,                       GameIsLocked,      TRUE },
	{"Unlocked",        "unlocked",          FOLDER_UNLOCKED,     IDI_UNLOCK,               0,             0,            NULL,                       GameIsLocked,      FALSE },
	{"Raster",          "raster",            FOLDER_RASTER,       IDI_RASTER,               F_RASTER,      F_VECTOR,     NULL,                       DriverIsVector,    FALSE },
	{"Vector",          "vector",            FOLDER_VECTOR,       IDI_VECTOR,               F_VECTOR,      F_RASTER,     NULL,                       DriverIsVector,    TRUE },
 	{"Multi-Monitor",   "multimon",          FOLDER_MULTIMON,     IDI_MULTIMON,             0,             0,            NULL,                       DriverIsMultiMon,  TRUE },
	{"Trackball",       "trackball",         FOLDER_TRACKBALL,    IDI_TRACKBALL,            0,             0,            NULL,                       DriverUsesTrackball,	TRUE },
	{"Lightgun",        "lightgun",          FOLDER_LIGHTGUN,     IDI_LIGHTGUN,             0,             0,            NULL,                       DriverUsesLightGun,	TRUE },
	{"Stereo",          "stereo",            FOLDER_STEREO,       IDI_STEREO,               0,             0,            NULL,                       DriverIsStereo,    TRUE },
	{"Samples",         "samples",           FOLDER_SAMPLES,      IDI_SAMPLES,              0,             0,            NULL,                       DriverUsesSamples,    TRUE },
	{"CHD",             "harddisk",          FOLDER_HARDDISK,     IDI_HARDDISK,             0,             0,            NULL,                       DriverIsHarddisk,  TRUE },
	{"Search result",   "findgame",          FOLDER_FINDGAME,     IDI_FINDGAME,             0,             0,            NULL,                       GameIsFound,  TRUE },
	{ NULL }
};

/* list of filter/control Id pairs */
FILTER_ITEM g_filterList[] =
{
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, TRUE },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, TRUE },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, FALSE },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, FALSE },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, TRUE },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, FALSE },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, FALSE },
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, TRUE },
	{ 0 }
};

DIRECTORYINFO g_directoryInfo[] =
{
	{ "ROMs",                  GetRomDirs,      SetRomDirs,      TRUE,  DIRDLG_ROMS },
	{ "Samples",               GetSampleDirs,   SetSampleDirs,   TRUE,  DIRDLG_SAMPLES },
	{ "Ini Files",             GetIniDir,       SetIniDir,       FALSE, DIRDLG_INI },
	{ "Config",                GetCfgDir,       SetCfgDir,       FALSE, DIRDLG_CFG },
	{ "High Scores",           GetHiDir,        SetHiDir,        FALSE, DIRDLG_HI },
	{ "Snapshots",             GetImgDir,       SetImgDir,       FALSE, DIRDLG_IMG },
	{ "Input files (*.inp)",   GetInpDir,       SetInpDir,       FALSE, DIRDLG_INP },
	{ "State",                 GetStateDir,     SetStateDir,     FALSE, 0 },
	{ "Artwork",               GetArtDir,       SetArtDir,       FALSE, 0 },
	{ "Memory Card",           GetMemcardDir,   SetMemcardDir,   FALSE, 0 },
	{ "Flyers",                GetFlyerDir,     SetFlyerDir,     FALSE, 0 },
	{ "Cabinets",              GetCabinetDir,   SetCabinetDir,   FALSE, 0 },
	{ "Marquees",              GetMarqueeDir,   SetMarqueeDir,   FALSE, 0 },
	{ "Titles",                GetTitlesDir,    SetTitlesDir,    FALSE, 0 },
	{ "Control Panels",        GetControlPanelDir,SetControlPanelDir, FALSE, 0 },
	{ "NVRAM",                 GetNvramDir,     SetNvramDir,     FALSE, 0 },
	{ "Controller Files",      GetCtrlrDir,     SetCtrlrDir,     FALSE, DIRDLG_CTRLR },
	{ "Hard Drive Difference", GetDiffDir,      SetDiffDir,      FALSE, 0 },
	{ "Icons",                 GetIconsDir,     SetIconsDir,     FALSE, 0 },
	{ "Background Images",     GetBgDir,        SetBgDir,        FALSE, 0 },
	{ "PCB Infos",             GetPcbInfosDir,  SetPcbInfosDir,  FALSE, 0 },
	{ NULL }
};

const SPLITTERINFO g_splitterInfo[] =
{
	{ 0.25,	IDC_SPLITTER,	IDC_TREE,	IDC_LIST,		AdjustSplitter1Rect },
	{ 0.5,	IDC_SPLITTER2,	IDC_LIST,	IDC_SSFRAME,	AdjustSplitter2Rect },
	{ -1 }
};

const MAMEHELPINFO g_helpInfo[] =
{
	{ ID_HELP_CONTENTS,		TRUE,	MAME32HELP "::/html/mame32_overview.htm" },
	{ ID_HELP_WHATS_NEW32,	TRUE,	MAME32HELP "::/html/mame32_changes.htm" },
	{ ID_HELP_TROUBLE,		TRUE,	MAME32HELP "::/html/mame32_support.htm" },
	{ ID_HELP_RELEASE,		FALSE,	"windows.txt" },
	{ ID_HELP_WHATS_NEW,	TRUE,	MAME32HELP "::/docs/whatsnew.txt" },
	{ -1 }
};

const PROPERTYSHEETINFO g_propSheets[] =
{
	{ FALSE,	NULL,					IDD_PROP_GAME,			GamePropertiesDialogProc },
	{ FALSE,	NULL,					IDD_PROP_AUDIT,			GameAuditDialogProc },
	{ TRUE,		NULL,					IDD_PROP_DISPLAY,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_ADVANCED,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_DIRECT3D,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_SOUND,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_INPUT,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_MISC,			GameOptionsProc },
	{ TRUE,		PropSheetFilter_Vector,	IDD_PROP_VECTOR,		GameOptionsProc },
	{ FALSE }
};

const ICONDATA g_iconData[] =
{
	{ IDI_WIN_NOROMS,			"noroms" },
	{ IDI_WIN_ROMS,				"roms" },
	{ IDI_WIN_UNKNOWN,			"unknown" },
	{ IDI_WIN_CLONE,			"clone" },
	{ IDI_WIN_REDX,				"warning" },
	{ 0 }
};

const char g_szPlayGameString[] = "&Play %s";
const char g_szGameCountString[] = "%d games";
const char g_szHistoryFileName[] = "history.dat";
const char g_szMameInfoFileName[] = "mameinfo.dat";

static BOOL FilterAvailable(int driver_index)
{
	return IsAuditResultYes(GetRomAuditResults(driver_index));
}
