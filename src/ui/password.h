/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2004 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef PASSWORD_H
#define PASSWORD_H


/*************************************************************
 * Compilation options for the password management behaviour *
 * for both GUI and command line running                     *
 *************************************************************/


/* DEFAULT_PWD_VALUE
 * This is the default value of the password when MIM32 has to
 * restore it when it is found missing (1st MIM32 run, or deletion
 * of ini file), or corrupt.
 * This value is case sensitive and its length shall be greater or
 * equal to 3 car, and lower and equal to 10 car.
 */
#define DEFAULT_PWD_VALUE			"noname"


/* DEFAULT_PWD_STATE
 * This define indicates whether the default password is enabled
 * when MIM32 has to restore it when it is found missing (1st
 * MIM32 run, or deletion of ini file), or corrupt.
 * When set to FALSE, it is disabled, i.e. MIM32 will work as if
 * there was no password until it is enabled from option menu.
 */
#define DEFAULT_PWD_ENABLED			FALSE


/* LIST_FILENAME
 * This is the name of the lock/unlock games list file.
 * If this name changes from a previous MIM32 version to a new one,
 * the previous file can be simply renamed, otherwise a new list
 * will be created with default values.
 */
#define LIST_FILENAME				"games.lst"


/* ALL_GAMES_LOCKED
 * This define indicates whether the games are locked when MIM32
 * has to restore the locked game list it when it is found missing (1st
 * MIM32 run, or deletion of file), or corrupt.
 * When set to FALSE, it is disabled, i.e. all games will be playable
 * whatever the password state (enabled or disabled) 
 */
#define ALL_GAMES_LOCKED			FALSE


/* NEW_GAMES_LOCKED
 * This define indicates whether the new games that have to be added
 * to the existing list are locked or not.
 * It occurs when you upgrade MIM32 to a newer version that supports
 * new games.
 * Previous games are obviously kept in the same state.
 * When set to FALSE, it is disabled, i.e. all new games will be
 * playable whatever the password state (enabled or disabled) 
 */
#define NEW_GAMES_LOCKED			FALSE



#endif /* PASSWORD_H */
