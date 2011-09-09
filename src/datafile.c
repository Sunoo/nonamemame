/****************************************************************************
 *      datafile.c
 *      History database engine
 *
 *      Token parsing by Neil Bradley
 *      Modifications and higher-level functions by John Butler
 ****************************************************************************/

#include <assert.h>
#include <ctype.h>
#include "osd_cpu.h"
#include "driver.h"
#include "datafile.h"

/****************************************************************************
 *      token parsing constants
 ****************************************************************************/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define CR      0x0d    /* '\n' and '\r' meanings are swapped in some */
#define LF      0x0a    /*     compilers (e.g., Mac compilers) */

enum
{
        TOKEN_COMMA,
        TOKEN_EQUALS,
        TOKEN_SYMBOL,
        TOKEN_LINEBREAK,
        TOKEN_INVALID=-1
};

#define MAX_TOKEN_LENGTH        256


/****************************************************************************
 *      datafile constants
 ****************************************************************************/
#define MAX_DATAFILE_ENTRIES 5000
#define DATAFILE_TAG '$'

const char *DATAFILE_TAG_KEY = "$info";
const char *DATAFILE_TAG_BIO = "$bio";
const char *DATAFILE_TAG_MAME = "$mame";
const char *DATAFILE_TAG_DRIV = "$drv";

const char *history_filename = NULL;
const char *mameinfo_filename = NULL;


/****************************************************************************
 *      private data for parsing functions
 ****************************************************************************/
static mame_file *fp;                                       /* Our file pointer */
static long dwFilePos;                                          /* file position */
static UINT8 bToken[MAX_TOKEN_LENGTH];          /* Our current token */

/* an array of driver name/drivers array index sorted by driver name
   for fast look up by name */
typedef struct
{
    const char *name;
    int index;
} driver_data_type;
static driver_data_type *sorted_drivers = NULL;
static int num_games;


/**************************************************************************
 **************************************************************************
 *
 *              Parsing functions
 *
 **************************************************************************
 **************************************************************************/

/*
 * DriverDataCompareFunc -- compare function for GetGameNameIndex
 */
static int CLIB_DECL DriverDataCompareFunc(const void *arg1,const void *arg2)
{
    return strcmp( ((driver_data_type *)arg1)->name, ((driver_data_type *)arg2)->name );
}

/*
 * GetGameNameIndex -- given a driver name (in lowercase), return
 * its index in the main drivers[] array, or -1 if it's not found.
 */
static int GetGameNameIndex(const char *name)
{
    driver_data_type *driver_index_info;
	driver_data_type key;
	key.name = name;

	if (sorted_drivers == NULL)
	{
		/* initialize array of game names/indices */
		int i;
		num_games = 0;
		while (drivers[num_games] != NULL)
			num_games++;

		sorted_drivers = (driver_data_type *)malloc(sizeof(driver_data_type) * num_games);
		for (i=0;i<num_games;i++)
		{
			sorted_drivers[i].name = drivers[i]->name;
			sorted_drivers[i].index = i;
		}
		qsort(sorted_drivers,num_games,sizeof(driver_data_type),DriverDataCompareFunc);
	}

	/* uses our sorted array of driver names to get the index in log time */
	driver_index_info = bsearch(&key,sorted_drivers,num_games,sizeof(driver_data_type),
								DriverDataCompareFunc);

	if (driver_index_info == NULL)
		return -1;

	return driver_index_info->index;

}


/****************************************************************************
 *      Create an array with sorted sourcedrivers for the function
 *      index_datafile_drivinfo to speed up the datafile access
 ****************************************************************************/

typedef struct
{
    const char *srcdriver;
    int index;
} srcdriver_data_type;
static srcdriver_data_type *sorted_srcdrivers = NULL;
static int num_games;


static int SrcDriverDataCompareFunc(const void *arg1,const void *arg2)
{
    return strcmp( ((srcdriver_data_type *)arg1)->srcdriver, ((srcdriver_data_type *)arg2)->srcdriver );
}


static int GetSrcDriverIndex(const char *srcdriver)
{
    srcdriver_data_type *srcdriver_index_info;
	srcdriver_data_type key;
	key.srcdriver = srcdriver;

	if (sorted_srcdrivers == NULL)
	{
		/* initialize array of game names/indices */
		int i;
		num_games = 0;
		while (drivers[num_games] != NULL)
			num_games++;

		sorted_srcdrivers = (srcdriver_data_type *)malloc(sizeof(srcdriver_data_type) * num_games);
		for (i=0;i<num_games;i++)
		{
			sorted_srcdrivers[i].srcdriver = drivers[i]->source_file+12;
			sorted_srcdrivers[i].index = i;
		}
		qsort(sorted_srcdrivers,num_games,sizeof(srcdriver_data_type),SrcDriverDataCompareFunc);
	}

	srcdriver_index_info = bsearch(&key,sorted_srcdrivers,num_games,sizeof(srcdriver_data_type),
								SrcDriverDataCompareFunc);

	if (srcdriver_index_info == NULL)
		return -1;

	return srcdriver_index_info->index;

}


/****************************************************************************
 *      GetNextToken - Pointer to the token string pointer
 *                                 Pointer to position within file
 *
 *      Returns token, or TOKEN_INVALID if at end of file
 ****************************************************************************/
static UINT32 GetNextToken(UINT8 **ppszTokenText, long *pdwPosition)
{
        UINT32 dwLength;                                                /* Length of symbol */
        long dwPos;                                                             /* Temporary position */
        UINT8 *pbTokenPtr = bToken;                             /* Point to the beginning */
        UINT8 bData;                                                    /* Temporary data byte */

        while (1)
        {
                bData = mame_fgetc(fp);                                  /* Get next character */

                /* If we're at the end of the file, bail out */

                if (mame_feof(fp))
                        return(TOKEN_INVALID);

                /* If it's not whitespace, then let's start eating characters */

                if (' ' != bData && '\t' != bData)
                {
                        /* Store away our file position (if given on input) */

                        if (pdwPosition)
                                *pdwPosition = dwFilePos;

                        /* If it's a separator, special case it */

                        if (',' == bData || '=' == bData)
                        {
                                *pbTokenPtr++ = bData;
                                *pbTokenPtr = '\0';
                                ++dwFilePos;

                                if (',' == bData)
                                        return(TOKEN_COMMA);
                                else
                                        return(TOKEN_EQUALS);
                        }

                        /* Otherwise, let's try for a symbol */

                        if (bData > ' ')
                        {
                                dwLength = 0;                   /* Assume we're 0 length to start with */

                                /* Loop until we've hit something we don't understand */

                                while (bData != ',' &&
                                                 bData != '=' &&
                                                 bData != ' ' &&
                                                 bData != '\t' &&
                                                 bData != '\n' &&
                                                 bData != '\r' &&
                                                 mame_feof(fp) == 0)
                                {
                                        ++dwFilePos;
                                        *pbTokenPtr++ = bData;  /* Store our byte */
                                        ++dwLength;
                                        assert(dwLength < MAX_TOKEN_LENGTH);
                                        bData = mame_fgetc(fp);
                                }

                                /* If it's not the end of the file, put the last received byte */
                                /* back. We don't want to touch the file position, though if */
                                /* we're past the end of the file. Otherwise, adjust it. */

                                if (0 == mame_feof(fp))
                                {
                                        mame_ungetc(bData, fp);
                                }

                                /* Null terminate the token */

                                *pbTokenPtr = '\0';

                                /* Connect up the */

                                if (ppszTokenText)
                                        *ppszTokenText = bToken;

                                return(TOKEN_SYMBOL);
                        }

                        /* Not a symbol. Let's see if it's a cr/cr, lf/lf, or cr/lf/cr/lf */
                        /* sequence */

                        if (LF == bData)
                        {
                                /* Unix style perhaps? */

                                bData = mame_fgetc(fp);          /* Peek ahead */
                                mame_ungetc(bData, fp);          /* Force a retrigger if subsequent LF's */

                                if (LF == bData)                /* Two LF's in a row - it's a UNIX hard CR */
                                {
                                        ++dwFilePos;
                                        *pbTokenPtr++ = bData;  /* A real linefeed */
                                        *pbTokenPtr = '\0';
                                        return(TOKEN_LINEBREAK);
                                }

                                /* Otherwise, fall through and keep parsing. */

                        }
                        else
                        if (CR == bData)                /* Carriage return? */
                        {
                                /* Figure out if it's Mac or MSDOS format */

                                ++dwFilePos;
                                bData = mame_fgetc(fp);          /* Peek ahead */

                                /* We don't need to bother with EOF checking. It will be 0xff if */
                                /* it's the end of the file and will be caught by the outer loop. */

                                if (CR == bData)                /* Mac style hard return! */
                                {
                                        /* Do not advance the file pointer in case there are successive */
                                        /* CR/CR sequences */

                                        /* Stuff our character back upstream for successive CR's */

                                        mame_ungetc(bData, fp);

                                        *pbTokenPtr++ = bData;  /* A real carriage return (hard) */
                                        *pbTokenPtr = '\0';
                                        return(TOKEN_LINEBREAK);
                                }
                                else
                                if (LF == bData)        /* MSDOS format! */
                                {
                                        ++dwFilePos;                    /* Our file position to reset to */
                                        dwPos = dwFilePos;              /* Here so we can reposition things */

                                        /* Look for a followup CR/LF */

                                        bData = mame_fgetc(fp);  /* Get the next byte */

                                        if (CR == bData)        /* CR! Good! */
                                        {
                                                bData = mame_fgetc(fp);  /* Get the next byte */

                                                /* We need to do this to pick up subsequent CR/LF sequences */

                                                mame_fseek(fp, dwPos, SEEK_SET);

                                                if (pdwPosition)
                                                        *pdwPosition = dwPos;

                                                if (LF == bData)        /* LF? Good! */
                                                {
                                                        *pbTokenPtr++ = '\r';
                                                        *pbTokenPtr++ = '\n';
                                                        *pbTokenPtr = '\0';

                                                        return(TOKEN_LINEBREAK);
                                                }
                                        }
                                        else
                                        {
                                                --dwFilePos;
                                                mame_ungetc(bData, fp);  /* Put the character back. No good */
                                        }
                                }
                                else
                                {
                                        --dwFilePos;
                                        mame_ungetc(bData, fp);  /* Put the character back. No good */
                                }

                                /* Otherwise, fall through and keep parsing */
                        }
                }

                ++dwFilePos;
        }
}


/****************************************************************************
 *      ParseClose - Closes the existing opened file (if any)
 ****************************************************************************/
static void ParseClose(void)
{
        /* If the file is open, time for fclose. */

        if (fp)
        {
                mame_fclose(fp);
        }

        fp = NULL;
}


/****************************************************************************
 *      ParseOpen - Open up file for reading
 ****************************************************************************/
static UINT8 ParseOpen(const char *pszFilename)
{
        /* Open file up in binary mode */

        fp = mame_fopen (NULL, pszFilename, FILETYPE_HISTORY, 0);

        /* If this is NULL, return FALSE. We can't open it */

        if (NULL == fp)
        {
                return(FALSE);
        }

        /* Otherwise, prepare! */

        dwFilePos = 0;
        return(TRUE);
}


/****************************************************************************
 *      ParseSeek - Move the file position indicator
 ****************************************************************************/
static UINT8 ParseSeek(long offset, int whence)
{
        int result = mame_fseek(fp, offset, whence);

        if (0 == result)
        {
                dwFilePos = mame_ftell(fp);
        }
        return (UINT8)result;
}



/**************************************************************************
 **************************************************************************
 *
 *              Datafile functions
 *
 **************************************************************************
 **************************************************************************/


/**************************************************************************
 *      ci_strcmp - case insensitive string compare
 *
 *      Returns zero if s1 and s2 are equal, ignoring case
 **************************************************************************/
static int ci_strcmp (const char *s1, const char *s2)
{
        int c1, c2;

        while ((c1 = tolower(*s1)) == (c2 = tolower(*s2)))
        {
                if (!c1)
                        return 0;

                s1++;
                s2++;
        }

        return (c1 - c2);
}


/**************************************************************************
 *      ci_strncmp - case insensitive character array compare
 *
 *      Returns zero if the first n characters of s1 and s2 are equal,
 *      ignoring case.
 **************************************************************************/
static int ci_strncmp (const char *s1, const char *s2, int n)
{
        int c1, c2;

        while (n)
        {
                if ((c1 = tolower (*s1)) != (c2 = tolower (*s2)))
                        return (c1 - c2);
                else if (!c1)
                        break;
                --n;

                s1++;
                s2++;
        }
        return 0;
}


/**************************************************************************
 *      index_datafile
 *      Create an index for the records in the currently open datafile.
 *
 *      Returns 0 on error, or the number of index entries created.
 **************************************************************************/
static int index_datafile (struct tDatafileIndex **_index)
{
        struct tDatafileIndex *idx;
        int count = 0;
        UINT32 token = TOKEN_SYMBOL;

        /* rewind file */
        if (ParseSeek (0L, SEEK_SET)) return 0;

        /* allocate index */
        idx = *_index = malloc (MAX_DATAFILE_ENTRIES * sizeof (struct tDatafileIndex));
        if (NULL == idx) return 0;

        /* loop through datafile */
        while ((count < (MAX_DATAFILE_ENTRIES - 1)) && TOKEN_INVALID != token)
        {
                long tell;
                char *s;

                token = GetNextToken ((UINT8 **)&s, &tell);
                if (TOKEN_SYMBOL != token) continue;

                /* DATAFILE_TAG_KEY identifies the driver */
                if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
                {
                        token = GetNextToken ((UINT8 **)&s, &tell);
                        if (TOKEN_EQUALS == token)
                        {
                                int done = 0;

                                token = GetNextToken ((UINT8 **)&s, &tell);
                                while (!done && TOKEN_SYMBOL == token)
                                {
									int game_index;
									char *p;
									for (p = s; *p; p++)
										*p = tolower(*p);

									game_index = GetGameNameIndex(s);
									if (game_index >= 0)
									{
										idx->driver = drivers[game_index];
										idx->offset = tell;
										idx++;
										count++;
										done = 1;
										break;
									}

									if (!done)
									{
										token = GetNextToken ((UINT8 **)&s, &tell);

										if (TOKEN_COMMA == token)
											token = GetNextToken ((UINT8 **)&s, &tell);
										else
											done = 1; /* end of key field */
									}
                                }
                        }
                }
        }

        /* mark end of index */
        idx->offset = 0L;
        idx->driver = 0;
        return count;
}

static int index_datafile_drivinfo (struct tDatafileIndex **_index)
{
	struct tDatafileIndex *idx;
	int count = 0;
	UINT32 token = TOKEN_SYMBOL;

	/* rewind file */
	if (ParseSeek (0L, SEEK_SET)) return 0;

	/* allocate index */
	idx = *_index = malloc (MAX_DATAFILE_ENTRIES * sizeof (struct tDatafileIndex));
	if (NULL == idx) return 0;

	/* loop through datafile */
	while ((count < (MAX_DATAFILE_ENTRIES - 1)) && TOKEN_INVALID != token)
	{
		long tell;
		char *s;

		token = GetNextToken ((UINT8 **)&s, &tell);
		if (TOKEN_SYMBOL != token) continue;

		/* DATAFILE_TAG_KEY identifies the driver */
		if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
		{
			token = GetNextToken ((UINT8 **)&s, &tell);
			if (TOKEN_EQUALS == token)
			{
				int done = 0;

				token = GetNextToken ((UINT8 **)&s, &tell);
				while (!done && TOKEN_SYMBOL == token)
				{
					int src_index;
					strlwr(s);
					src_index = GetSrcDriverIndex(s);
					if (src_index >= 0)
					{
						idx->driver = drivers[src_index];
						idx->offset = tell;
						idx++;
						count++;
						done = 1;
						break;
					}

					if (!done)
					{
						token = GetNextToken ((UINT8 **)&s, &tell);
						if (TOKEN_COMMA == token)
							token = GetNextToken ((UINT8 **)&s, &tell);
						else
							done = 1; /* end of key field */
					}
				}
			}
		}
	}

	/* mark end of index */
	idx->offset = 0L;
	idx->driver = 0;
	return count;
}

/**************************************************************************
 *      load_datafile_text
 *
 *      Loads text field for a driver into the buffer specified. Specify the
 *      driver, a pointer to the buffer, the buffer size, the index created by
 *      index_datafile(), and the desired text field (e.g., DATAFILE_TAG_BIO).
 *
 *      Returns 0 if successful.
 **************************************************************************/
static int load_datafile_text (const struct GameDriver *drv, char *buffer, int bufsize,
        struct tDatafileIndex *idx, const char *tag)
{
        int     offset = 0;
        int found = 0;
        UINT32  token = TOKEN_SYMBOL;
        UINT32  prev_token = TOKEN_SYMBOL;

        *buffer = '\0';

        /* find driver in datafile index */
        while (idx->driver)
        {

                if (idx->driver == drv) break;

                idx++;
        }
        if (idx->driver == 0) return 1; /* driver not found in index */

        /* seek to correct point in datafile */
        if (ParseSeek (idx->offset, SEEK_SET)) return 1;

        /* read text until buffer is full or end of entry is encountered */
        while (TOKEN_INVALID != token)
        {
                char *s;
                int len;
                long tell;

                token = GetNextToken ((UINT8 **)&s, &tell);
                if (TOKEN_INVALID == token) continue;

                if (found)
                {
                        /* end entry when a tag is encountered */
                        if (TOKEN_SYMBOL == token && DATAFILE_TAG == s[0] && TOKEN_LINEBREAK == prev_token) break;

                        prev_token = token;

                        /* translate platform-specific linebreaks to '\n' */
                        if (TOKEN_LINEBREAK == token)
                                strcpy (s, "\n");

                        /* append a space to words */
                        if (TOKEN_LINEBREAK != token)
                                strcat (s, " ");

                        /* remove extraneous space before commas */
                        if (TOKEN_COMMA == token)
                        {
                                --buffer;
                                --offset;
                                *buffer = '\0';
                        }

                        /* Get length of text to add to the buffer */
                        len = strlen (s);

                        /* Check for buffer overflow */
                        /* For some reason we can get a crash if we try */
                        /* to use the last 30 characters of the buffer  */
                        if ((bufsize - offset) - len <= 45)
                        {
                            strcpy (s, " ...[TRUNCATED]");
                            len = strlen(s);
                            strcpy (buffer, s);
                            buffer += len;
                            offset += len;
                            break;
                        }

                        /* add this word to the buffer */
                        strcpy (buffer, s);
                        buffer += len;
                        offset += len;
                }
                else
                {
                        if (TOKEN_SYMBOL == token)
                        {
                                /* looking for requested tag */
                                if (!ci_strncmp (tag, s, strlen (tag)))
                                        found = 1;
                                else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
                                        break; /* error: tag missing */
                        }
                }
        }
        return (!found);
}

static int load_drivfile_text (const struct GameDriver *drv, char *buffer, int bufsize,
	struct tDatafileIndex *idx, const char *tag)
{
	int	offset = 0;
	int found = 0;
	UINT32	token = TOKEN_SYMBOL;
	UINT32 	prev_token = TOKEN_SYMBOL;

	*buffer = '\0';

	/* find driver in datafile index */
	while (idx->driver)
	{
		if (idx->driver->source_file == drv->source_file) break;
		idx++;
	}
	if (idx->driver == 0) return 1;	/* driver not found in index */

	/* seek to correct point in datafile */
	if (ParseSeek (idx->offset, SEEK_SET)) return 1;

	/* read text until buffer is full or end of entry is encountered */
	while (TOKEN_INVALID != token)
	{
		char *s;
		int len;
		long tell;

		token = GetNextToken ((UINT8 **)&s, &tell);
		if (TOKEN_INVALID == token) continue;

		if (found)
		{
			/* end entry when a tag is encountered */
			if (TOKEN_SYMBOL == token && DATAFILE_TAG == s[0] && TOKEN_LINEBREAK == prev_token) break;

			prev_token = token;

			/* translate platform-specific linebreaks to '\n' */
			if (TOKEN_LINEBREAK == token)
				strcpy (s, "\n");

			/* append a space to words */
			if (TOKEN_LINEBREAK != token)
				strcat (s, " ");

			/* remove extraneous space before commas */
			if (TOKEN_COMMA == token)
			{
				--buffer;
				--offset;
				*buffer = '\0';
			}

			/* add this word to the buffer */
			len = strlen (s);
			if ((len + offset) >= bufsize) break;
			strcpy (buffer, s);
			buffer += len;
			offset += len;
		}
		else
		{
			if (TOKEN_SYMBOL == token)
			{
				/* looking for requested tag */
				if (!ci_strncmp (tag, s, strlen (tag)))
					found = 1;
				else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
					break; /* error: tag missing */
			}
		}
	}
	return (!found);
}

/**************************************************************************
 *      load_driver_history
 *      Load history text for the specified driver into the specified buffer.
 *      Combines $bio field of HISTORY.DAT with $mame field of MAMEINFO.DAT.
 *
 *      Returns 0 if successful.
 *
 *      NOTE: For efficiency the indices are never freed (intentional leak).
 **************************************************************************/
int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize)
{
        static struct tDatafileIndex *hist_idx = 0;
	int history = 0;
        int err;

        *buffer = 0;


        if(!history_filename)
                history_filename = "history.dat";

        /* try to open history datafile */
        if (ParseOpen (history_filename))
        {
                /* create index if necessary */
                if (hist_idx)
                        history = 1;
                else
                        history = (index_datafile (&hist_idx) != 0);

                /* load history text */
                if (hist_idx)
                {
                        const struct GameDriver *gdrv;

                        gdrv = drv;
                        do
                        {
                                err = load_datafile_text (gdrv, buffer, bufsize,
                                                                                  hist_idx, DATAFILE_TAG_BIO);
                                gdrv = gdrv->clone_of;
                        } while (err && gdrv);

                        if (err) history = 0;
                }
                ParseClose ();
        }

	return (history == 0);
}

int load_driver_mameinfo (const struct GameDriver *drv, char *buffer, int bufsize)
{
	static struct tDatafileIndex *mame_idx = 0;
	const struct RomModule *region, *rom, *chunk;
	struct InternalMachineDriver game;
	int mameinfo = 0;
	int err;
	int	i;

	*buffer = 0;

	expand_machine_driver(drv->drv, &game);



	/* List the game info 'flags' */
	if (drv->flags &
			    ( GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS |
				 GAME_IMPERFECT_COLORS | GAME_NO_SOUND | GAME_IMPERFECT_SOUND | GAME_NO_COCKTAIL) ||
		 game.video_attributes & VIDEO_DUAL_MONITOR)
	{
		strcat(buffer, "GAME: ");
		strcat(buffer, drv->description);
		strcat(buffer, "\n");

		if (drv->flags & GAME_NOT_WORKING)
			strcat(buffer, "THIS GAME DOESN'T WORK PROPERLY\n");

		if (drv->flags & GAME_UNEMULATED_PROTECTION)
			strcat(buffer, "The game has protection which isn't fully emulated.\n");

		if (drv->flags & GAME_IMPERFECT_GRAPHICS)
			strcat(buffer, "The video emulation isn't 100% accurate.\n");

		if (drv->flags & GAME_WRONG_COLORS)
			strcat(buffer, "The colors are completely wrong.\n");

		if (drv->flags & GAME_IMPERFECT_COLORS)
			strcat(buffer, "The colors aren't 100% accurate.\n");

		if (drv->flags & GAME_NO_SOUND)
			strcat(buffer, "The game lacks sound.\n");

		if (drv->flags & GAME_IMPERFECT_SOUND)
			strcat(buffer, "The sound emulation isn't 100% accurate.\n");

		if (drv->flags & GAME_NO_COCKTAIL)
			strcat(buffer, "Screen flipping in cocktail mode is not supported.\n");

		if (game.video_attributes & VIDEO_DUAL_MONITOR)
			strcat(buffer, "The game use two or more monitors.\n");

		strcat(buffer, "\n");
	}	

        if(!mameinfo_filename)
                mameinfo_filename = "mameinfo.dat";

        /* try to open mameinfo datafile */
        if (ParseOpen (mameinfo_filename))
        {
                /* create index if necessary */
                if (mame_idx)
                        mameinfo = 1;
                else
                        mameinfo = (index_datafile (&mame_idx) != 0);

                /* load informational text (append) */
                if (mame_idx)
                {
                        int len = strlen (buffer);
                        const struct GameDriver *gdrv;

                        gdrv = drv;
                        do
                        {
                                err = load_datafile_text (gdrv, buffer+len, bufsize-len,
                                                                                  mame_idx, DATAFILE_TAG_MAME);
                                gdrv = gdrv->clone_of;
                        } while (err && gdrv);

                        if (err) mameinfo = 0;
                }
                ParseClose ();
        }

	strcat(buffer, "\nROM REGION:\n");
	for (region = rom_first_region(drv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			char name[100];
			int length;

			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			sprintf(name," %-12s ",ROM_GETNAME(rom));
			strcat(buffer, name);
			sprintf(name,"%6x ",length);
			strcat(buffer, name);
			switch (ROMREGION_GETTYPE(region))
			{
			case REGION_CPU1: strcat(buffer, "cpu1"); break;
			case REGION_CPU2: strcat(buffer, "cpu2"); break;
			case REGION_CPU3: strcat(buffer, "cpu3"); break;
			case REGION_CPU4: strcat(buffer, "cpu4"); break;
			case REGION_CPU5: strcat(buffer, "cpu5"); break;
			case REGION_CPU6: strcat(buffer, "cpu6"); break;
			case REGION_CPU7: strcat(buffer, "cpu7"); break;
			case REGION_CPU8: strcat(buffer, "cpu8"); break;
			case REGION_GFX1: strcat(buffer, "gfx1"); break;
			case REGION_GFX2: strcat(buffer, "gfx2"); break;
			case REGION_GFX3: strcat(buffer, "gfx3"); break;
			case REGION_GFX4: strcat(buffer, "gfx4"); break;
			case REGION_GFX5: strcat(buffer, "gfx5"); break;
			case REGION_GFX6: strcat(buffer, "gfx6"); break;
			case REGION_GFX7: strcat(buffer, "gfx7"); break;
			case REGION_GFX8: strcat(buffer, "gfx8"); break;
			case REGION_PROMS: strcat(buffer, "prom"); break;
			case REGION_SOUND1: strcat(buffer, "snd1"); break;
			case REGION_SOUND2: strcat(buffer, "snd2"); break;
			case REGION_SOUND3: strcat(buffer, "snd3"); break;
			case REGION_SOUND4: strcat(buffer, "snd4"); break;
			case REGION_SOUND5: strcat(buffer, "snd5"); break;
			case REGION_SOUND6: strcat(buffer, "snd6"); break;
			case REGION_SOUND7: strcat(buffer, "snd7"); break;
			case REGION_SOUND8: strcat(buffer, "snd8"); break;
			case REGION_USER1: strcat(buffer, "usr1"); break;
			case REGION_USER2: strcat(buffer, "usr2"); break;
			case REGION_USER3: strcat(buffer, "usr3"); break;
			case REGION_USER4: strcat(buffer, "usr4"); break;
			case REGION_USER5: strcat(buffer, "usr5"); break;
			case REGION_USER6: strcat(buffer, "usr6"); break;
			case REGION_USER7: strcat(buffer, "usr7"); break;
			case REGION_USER8: strcat(buffer, "usr8"); break;
	            }

		sprintf(name," %7x\n",ROM_GETOFFSET(rom));
		strcat(buffer, name);

		}

	if (drv->clone_of && !(drv->clone_of->flags & NOT_A_DRIVER))
	{
		strcat(buffer, "\nORIGINAL:\n");
		strcat(buffer, drv->clone_of->description);
		strcat(buffer, "\n\nCLONES:\n");
		for (i = 0; drivers[i]; i++)
		{
			if (!ci_strcmp (drv->clone_of->name, drivers[i]->clone_of->name)) 
			{
				strcat(buffer, drivers[i]->description);
				strcat(buffer, "\n");
			}
		}
	}
	else
	{
		strcat(buffer, "\nORIGINAL:\n");
		strcat(buffer, drv->description);
		strcat(buffer, "\n\nCLONES:\n");
		for (i = 0; drivers[i]; i++)
		{
			if (!ci_strcmp (drv->name, drivers[i]->clone_of->name)) 
			{
				strcat(buffer, drivers[i]->description);
				strcat(buffer, "\n");
			}
		}
	}

	return (mameinfo == 0);
}

int load_driver_drivinfo (const struct GameDriver *drv, char *buffer, int bufsize)
{
	static struct tDatafileIndex *driv_idx = 0;
	int drivinfo = 0;
	int err;
	int	i;

	*buffer = 0;

	/* Print source code file */
	sprintf (buffer, "SOURCE: %s\n", drv->source_file+12);

        if(!mameinfo_filename)
                mameinfo_filename = "mameinfo.dat";

	/* Try to open mameinfo datafile - driver section*/
	if (ParseOpen (mameinfo_filename))
	{
		/* create index if necessary */
		if (driv_idx)
			drivinfo = 1;
		else
			drivinfo = (index_datafile_drivinfo (&driv_idx) != 0);

		/* load informational text (append) */
		if (driv_idx)
		{
			int len = strlen (buffer);

			err = load_drivfile_text (drv, buffer+len, bufsize-len,
									  driv_idx, DATAFILE_TAG_DRIV);

			if (err) drivinfo = 0;
		}
		ParseClose ();
	}

	strcat(buffer,"\nGAMES SUPPORTED:\n");
	for (i = 0; drivers[i]; i++)
	{
		if (!ci_strcmp (drv->source_file, drivers[i]->source_file)) 
		{
			strcat(buffer, drivers[i]->description);
			strcat(buffer,"\n");
		}
	}

	return (drivinfo == 0);

}

int load_driver_statistics (char *buffer, int bufsize)
{
	const struct RomModule *region, *rom, *chunk;
	const struct RomModule *pregion, *prom, *fprom=NULL;
	const struct InputPortTiny *inp;

	static const char *versions[] =
	{
		"0.74     September 14th 2003",
		"0.73     September  4th 2003",
		"0.72u2   August    25th 2003",
		"0.72u1   August    15th 2003",
		"0.72     August     9th 2003",
		"0.71u2   July      12th 2003",
		"0.71u1   July       8th 2003",
		"0.71     July       5th 2003",
		"0.70u5   June      29th 2003",
		"0.70u4   June      21th 2003",
		"0.70u3   June      21th 2003",
		"0.70u2   June      19th 2003",
		"0.70u1   June      12th 2003",
		"0.70     June      11th 2003",
		"0.69u3   June       5th 2003",
		"0.69b    June       2nd 2003",
		"0.69a    May       31th 2003",
		"0.69     May       24th 2003",
		"0.68     May       16th 2003",
		"0.67     April      6th 2003",
		"0.66     March     10th 2003",
		"0.65     February  10th 2003",
		"0.64     January   26th 2003",
		"0.63     January   12th 2003",
		"0.62     November  12th 2002",
		"0.61     July       4th 2002",
		"0.60     May        1st 2002",
		"0.59     March     22nd 2002",
		"0.58     February   6th 2002",
		"0.57     January    1st 2002",
		"0.56     November   1st 2001",
		"0.55     September 17th 2001",
		"0.54     August    25th 2001",
		"0.53     August    12th 2001",
		"0.37b16  July       2th 2001",
		"0.37b15  May       24th 2001",
		"0.37b14  April      7th 2001",
		"0.37b13  March     10th 2001",
		"0.37b12  February  15th 2001",
		"0.37b11  January   16th 2001",
		"0.37b10  December   6th 2000",
		"0.37b9   November   6th 2000",
		"0.37b8   October    3rd 2000",
		"0.37b7   September  5th 2000",
		"0.37b6   August    20th 2000",
		"0.37b5   July      28th 2000",
		"0.37b4   June      16th 2000",
		"0.37b3   May       27th 2000",
		"0.37b2   May        6th 2000",
		"0.37b1   April      6th 2000",
		"0.36     March     21st 2000",
		"0.36RC2  March     13th 2000",
		"0.36RC1  February  26th 2000",
		"0.36b16  February   4th 2000",
		"0.36b15  January   21st 2000",
		"0.36b14  January   11th 2000",
		"0.36b13  December  30th 1999",
		"0.36b12  December  18th 1999",
		"0.36b11  December   6th 1999",
		"0.36b10  November  20th 1999",
		"0.36b91  November  14th 1999",
		"0.36b9   November  13th 1999",
		"0.36b8   October   30th 1999",
		"0.36b7   October   17th 1999",
		"0.36b6   September 29th 1999",
		"0.36b5   September 18th 1999", 
		"0.36b4   September  4th 1999",
		"0.36b3   August    22nd 1999",
		"0.36b2   August     8th 1999",
		"0.36b1   July      19th 1999",
		"0.35fix  July       5th 1999",
		"0.35     July       4th 1999",
		"0.35RC2  June      24th 1999",
		"0.35RC1  June      13th 1999",
		"0.35b13  May       24th 1999",
		"0.35b12  May        1st 1999",
		"0.35b11  April     22nd 1999",
		"0.35b10  April      8th 1999",
		"0.35b9   March     30th 1999",
		"0.35b8   March     24th 1999",
		"0.35b7   March     18th 1999",
		"0.35b6   March     15th 1999",
		"0.35b5   March      7th 1999",
		"0.35b4   March      1st 1999",
		"0.35b3   February  15th 1999",
		"0.35b2   January   24th 1999",
		"0.35b1   January    7th 1999",
		"0.34     December  31st 1998",
		"0.34RC2  December  21st 1998",
		"0.34RC1  December  14th 1998",
		"0.34b8   November  29th 1998",
		"0.34b7   November  10th 1998",
		"0.34b6   October   28th 1998",
		"0.34b5   October   16th 1998",
		"0.34b4   October    4th 1998",
		"0.34b3   September 17th 1998",
		"0.34b2   August    30th 1998",
		"0.34b1   August    16th 1998",
		"0.33     August    10th 1998",
		"0.33RC1  July      29th 1998", 
		"0.33b7   July      21st 1998",
		"0.33b6   June      16th 1998",
		"0.33b5   June      10th 1998",
		"0.33b4   May       31st 1998",
		"0.33b3   May       17th 1998",
		"0.33b2   May        8th 1998",
		"0.33b1   May        3rd 1998",
		"0.31     April     25th 1998",
		"0.30     January    7th 1998",
		"0.29     October   20th 1997",
		"0.28     September  7th 1997",
		"0.27     August    10th 1997",
		"0.26a    July      18th 1997",
		"0.26     July      14th 1997",
		"0.25     June      28th 1997",
		"0.24     June      13th 1997",
		"0.23a    June       3rd 1997",
		"0.23     June       2nd 1997",
		"0.22     May       25th 1997",
		"0.21.5   May       16th 1997",
		"0.21     May       12th 1997",
		"0.20     May        5th 1997",
		"0.19     April     26th 1997",
		"0.18     April     20th 1997",
		"0.17     April     14th 1997",
		"0.16     April     13th 1997",
		"0.15     April      6th 1997",
		"0.14     April      2nd 1997",
		"0.13     March     30th 1997",
		"0.12     March     23rd 1997",
		"0.11     March     16th 1997",
		"0.10     March     13th 1997",
		"0.091    March      9th 1997",
		"0.09     March      9th 1997",
		"0.081    March      4th 1997",
		"0.08     March      4th 1997",
		"0.07     February  27th 1997",
		"0.06     February  23rd 1997",
		"0.05     February  20th 1997",
		"0.04     February  16th 1997",
		"0.03     February  13th 1997",
		"0.02     February   9th 1997",
		"0.01     February   5th 1997",
		0
	};

	static const char *history[] =
	{
		"0.74       686    4123  +7",
		"0.73       685    4116  +9",
		"0.72u2     683    4107  +19",
		"0.72u1     680    4088  +5",
		"0.72       679    4083  +31",
		"0.71u2     677    4052  +25",
		"0.71u1     669    4027  +3",
		"0.71       669    4024  +10",
		"0.70u5     667    4014  +8",
		"0.70u4     667    4006  +11",
		"0.70u3     666    3995  +2",
		"0.70u2     666    3993  +2",
		"0.70u1     665    3991  +1",
		"0.70       665    3990  +2",
		"0.69u3     664    3988  +10",
		"0.69b      664    3978  +7",
		"0.69a      663    3971  +7",
		"0.69       663    3964  +28",
		"0.68       657    3936  +110",
		"0.67       635    3826  +34",
		"0.66       631    3792  +43",
		"0.65       628    3749  +18",
		"0.64       626    3731  +41",
		"0.63       623    3690  +94",
		"0.62       605    3596  +127",
		"0.61       563    3469  +101",
		"0.60       543    3368  +78",
		"0.59       529    3290  +36",
		"0.58       521    3254  +25",
		"0.57       514    3229  +74",
		"0.56       502    3155  +32",
		"0.55       501    3123  +21",
		"0.54       499    3102  + 4",
		"0.53       499    3098  +90",
		"0.37b16    498    3008  +78",
		"0.37b15    478    2930  +47",
		"0.37b14    476    2883  +44",
		"0.37b13    471    2839  +66",
		"0.37b12    460    2773  +109",
		"0.37b11    448    2664  +125",
		"0.37b10    438    2539  +63",
		"0.37b9     421    2476  +65",
		"0.37b8     417    2411  +74",
		"0.37b7     407    2337  +54",
		"0.37b6     400    2283  +43",
		"0.37b5     393    2240  +83",
		"0.37b4     383    2157  +12",
		"0.37b3     382    2145  +42",
		"0.37b2     375    2103  +30",
		"0.37b1     369    2073  +25",
		"0.36       366    2048  + 0",
		"0.36RC2    366    2048  +27",
		"0.36RC1    364    2021  +21",
		"0.36b16    364    2000  +29",
		"0.36b15    366    1971  +20",
		"0.36b14    362    1951  +19",
		"0.36b13    361    1932  +20",
		"0.36b12    356    1912  +12",
		"0.36b11    353    1900  +23",
		"0.36b10    352    1877  +19",
		"0.36b91    345    1858  + 0",
		"0.36b9     345    1858  +27",
		"0.36b8     342    1831  +37",
		"0.36b7     339    1794  +29",
		"0.36b6     336    1765  +42",
		"0.36b5     330    1723  +39",
		"0.36b4     325    1684  +42",
		"0.36b3     320    1642  +41",
		"0.36b2     312    1601  +69",
		"0.36b1     297    1532  +58",
		"0.35fix    287    1474  + 0",
		"0.35       287    1474  +15",
		"0.35RC2    287    1459  +22",
		"0.35RC1    286    1437  +37",
		"0.35b13    285    1400  +58",
		"0.35b12    273    1342  +25",
		"0.35b11    268    1317  +54",
		"0.35b10    262    1263  + 9",
		"0.35b9     261    1254  +13",
		"0.35b8     258    1241  +22",
		"0.35b7     256    1219  + 2",
		"0.35b6     257    1217  +31",
		"0.35b5     255    1186  + 8",
		"0.35b4     255    1178  +30",
		"0.35b3     252    1148  +41",
		"0.35b2     251    1107  +60",
		"0.35b1     247    1047  +23",
		"0.34       243    1024  + 0",
		"0.34RC2    243    1024  + 0",
		"0.34RC1    243    1024  + 1",
		"0.34b8     244    1023  +43",
		"0.34b7     236     980  +42",
		"0.34b6     234     938  +34",
		"0.34b5     231     904  +50",
		"0.34b4     230     854  +50",
		"0.34b3     226     804  +38",
		"0.34b2     226     766  +64",
		"0.34b1     218     702  +73",
		"0.33       210     629  + 0",
		"0.33RC1    210     629  + 1",
		"0.33b7     210     628  +24",
		"0.33b6     200     604  +21",
		"0.33b5             583  +18",
		"0.33b4             565  +25",
		"0.33b3             540  +40",
		"0.33b2             500  + 7",
		"0.33b1             493  +15",
		"0.31               478  +132",
		"0.30               346  +88",
		"0.29               258  +33",
		"0.28               225  +25",
		"0.27               200  +32",
		"0.26a              168  + 1",
		"0.26               167  +21",
		"0.25               146  +12",
		"0.24               134  +10",
		"0.23a              124  + 0",
		"0.23               124  + 6",
		"0.22               118  + 6",
		"0.21.5             112  + 0",
		"0.21               112  + 6",
		"0.20               106  + 4",
		"0.19               102  + 4",
		"0.18                98  + 9",
		"0.17                89  + 4",
		"0.16                85  + 5",
		"0.15                80  + 2",
		"0.14                78  + 1",
		"0.13                77  + 1",
		"0.12                76  + 8",
		"0.11                68  + 3",
		"0.10                65  + 3",
		"0.091               62  + 0",
		"0.09                62  + 3",
		"0.081               59  + 0",
		"0.08                59  +11",
		"0.07                48  + 7",
		"0.06                41  + 7",
		"0.05                34  +15",
		"0.04                19  + 2",
		"0.03                17  + 1",
		"0.02                16  +11",
		"0.01                 5     ",
		0
	};

	static const char *newgames[] =
	{
		"1997:    + 258",
		"1998:    + 766",
		"1999:    + 908",
		"2000:    + 607",
		"2001:    + 616",
		"2002:    + 441",
		"2003:    + 527",
		0
	};


	struct snd_interface
	{
		unsigned sound_num;
		const char *name;
		int (*chips_num)(const struct MachineSound *msound);
		int (*chips_clock)(const struct MachineSound *msound);
		int (*start)(const struct MachineSound *msound);
		void (*stop)(void);
		void (*update)(void);
		void (*reset)(void);
	};

	extern struct snd_interface sndintf[];

	char name[100];
	char year[4];
	int i, n, x, y;
	int ax, ay;
	int all = 0, cl = 0, vec = 0, vecc = 0, neo = 0, neoc = 0;
	int pch = 0, pchc = 0, deco = 0, decoc = 0, cvs = 0, cvsc = 0, noyear = 0, nobutt = 0, noinput = 0;
	int vertical = 0, verticalc = 0, horizontal = 0, horizontalc = 0;
	int clone = 0, stereo = 0, stereoc = 0, dual = 0, dualc = 0;
	int sum = 0, xsum = 0, files = 0, mfiles = 0, hddisk = 0;
	int rsum = 0, ndgame = 0, ndsum = 0, gndsum = 0, gndsumc = 0, bdgame = 0, bdsum = 0, gbdsum = 0, gbdsumc = 0;
	int bitx = 0, bitc = 0, shad = 0, shadc = 0, hghs = 0, hghsc = 0;
	int rgbd = 0, rgbdc = 0;
	static int flags[20], romsize[10];
	static int numcpu[4][CPU_COUNT], numsnd[4][SOUND_COUNT], sumcpu[MAX_CPU+1], sumsound[MAX_SOUND+1];
	static int resx[400], resy[400], resnum[400];
	static int asx[30], asy[30], asnum[30];
	static int palett[200], palettnum[200], control[35];
	static int fpsnum[35];
	float fps[35];


	*buffer = 0;


	strcat(buffer, "MAME ");
	strcat(buffer, build_version);

	for (i = 0; drivers[i]; i++)
	{
		struct InternalMachineDriver drv;
		expand_machine_driver(drivers[i]->drv, &drv);

		all++;

		if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
		{
			clone = 1;
			cl++;
		}
		else
			clone = 0;


		/* Calc all graphic resolution and aspect ratio numbers */
		if (drivers[i]->flags & ORIENTATION_SWAP_XY)
		{
			x = drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1;
			y = drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1;

			ax = drv.aspect_y;
			ay = drv.aspect_x;
			if (ax == 0 && ay == 0)
			{
				ax = 3;
				ay = 4;
			}

		}
		else
		{
			x = drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1;
			y = drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1;

			ax = drv.aspect_x;
			ay = drv.aspect_y;
			if (ax == 0 && ay == 0)
			{
				ax = 4;
				ay = 3;
			}

		}

		if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		{
	 		vec++;
			if (clone) vecc++;
		}
		else
		{
			/* Store all resolutions, without the Vector games */
			for (n = 0; n < 200; n++)
			{
				if (resx[n] == x && resy[n] == y)
				{
					resnum [n]++;
					break;
				}

				if (resx[n] == 0)
				{
					resx[n] = x;
					resy[n] = y;
					resnum [n]++;
					break;
				}

			}

		}

		/* Store all aspect ratio numbers */
		for (n = 0; n < 15; n++)
		{
			if (asx[n] == ax && asy[n] == ay)
			{
				asnum [n]++;
				break;
			}

			if (asx[n] == 0)
			{
				asx[n] = ax;
				asy[n] = ay;
				asnum [n]++;
				break;
			}
		}


		/* Calc all palettesize numbers */
		x = drv.total_colors;
		for (n = 0; n < 100; n++)
		{
			if (palett[n] == x)
			{
				palettnum [n]++;
				break;
			}

			if (palett[n] == 0)
			{
				palett[n] = x;
				palettnum [n]++;
				break;
			}
		}


		if (!ci_strcmp (drivers[i]->source_file+12, "neogeo.c"))
		{
			neo++;
			if (clone) neoc++;
		}

		if (!ci_strcmp (drivers[i]->source_file+12, "playch10.c"))
		{
			pch++;
			if (clone) pchc++;
		}

		if (!ci_strcmp (drivers[i]->source_file+12, "decocass.c"))
		{
			deco++;
			if (clone) decoc++;
		}

		if (!ci_strcmp (drivers[i]->source_file+12, "cvs.c"))
		{
			cvs++;
			if (clone) cvsc++;
		}

		if (drivers[i]->flags & ORIENTATION_SWAP_XY)
		{
	 		vertical++;
			if (clone) verticalc++;
		}
		else
		{
			horizontal++;
			if (clone) horizontalc++;
		}

		if (drv.sound_attributes & SOUND_SUPPORTS_STEREO)
		{
	 		stereo++;
			if (clone) stereoc++;
		}


		/* Calc GAME INPUT and numbers */
		n = 0, x = 0, y = 0;
		inp = drivers[i]->input_ports;
		while ((inp->type & ~IPF_MASK) != IPT_END)
		{
			switch (inp->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (n<1) n = 1;
					break;
				case IPF_PLAYER2:
					if (n<2) n = 2;
					break;
				case IPF_PLAYER3:
					if (n<3) n = 3;
					break;
				case IPF_PLAYER4:
					if (n<4) n = 4;
					break;
				case IPF_PLAYER5:
					if (n<5) n = 5;
					break;
				case IPF_PLAYER6:
					if (n<6) n = 6;
					break;
				case IPF_PLAYER7:
					if (n<7) n = 7;
					break;
				case IPF_PLAYER8:
					if (n<8) n = 8;
					break;
			}
			switch (inp->type & ~IPF_MASK)
			{
				case IPT_JOYSTICK_UP:
				case IPT_JOYSTICK_DOWN:
				case IPT_JOYSTICK_LEFT:
				case IPT_JOYSTICK_RIGHT:
					if (inp->type & IPF_2WAY)
						y = 1;
					else if (inp->type & IPF_4WAY)
						y = 2;
					else
						y = 3;
					break;
				case IPT_JOYSTICKRIGHT_UP:
				case IPT_JOYSTICKRIGHT_DOWN:
				case IPT_JOYSTICKRIGHT_LEFT:
				case IPT_JOYSTICKRIGHT_RIGHT:
				case IPT_JOYSTICKLEFT_UP:
				case IPT_JOYSTICKLEFT_DOWN:
				case IPT_JOYSTICKLEFT_LEFT:
				case IPT_JOYSTICKLEFT_RIGHT:
					if (inp->type & IPF_2WAY)
						y = 4;
					else if (inp->type & IPF_4WAY)
						y = 5;
					else
						y = 6;
					break;
				case IPT_BUTTON1:
					if (x<1) x = 1;
					break;
				case IPT_BUTTON2:
					if (x<2) x = 2;
					break;
				case IPT_BUTTON3:
					if (x<3) x = 3;
					break;
				case IPT_BUTTON4:
					if (x<4) x = 4;
					break;
				case IPT_BUTTON5:
					if (x<5) x = 5;
					break;
				case IPT_BUTTON6:
					if (x<6) x = 6;
					break;
				case IPT_BUTTON7:
					if (x<7) x = 7;
					break;
				case IPT_BUTTON8:
					if (x<8) x = 8;
					break;
				case IPT_BUTTON9:
					if (x<9) x = 9;
					break;
				case IPT_BUTTON10:
					if (x<10) x = 10;
					break;
				case IPT_PADDLE:
					y = 7;
					break;
				case IPT_DIAL:
					y = 8;
					break;
				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					y = 9;
					break;
				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
					y = 10;
					break;
				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					y = 11;
					break;
			}
			++inp;
		}

		if (n) control[n]++;
		if (x) control[x+10]++;
		if (y) control[y+20]++;


		/* Calc all Frames_Per_Second numbers */
		fps[0] = drv.frames_per_second;
		for (n = 1; n < 35; n++)
		{
			if (fps[n] == fps[0])
			{
				fpsnum[n]++;
				break;
			}

			if (fpsnum[n] == 0)
			{
				fps[n] = fps[0];
				fpsnum[n]++;
				fpsnum[0]++;
				break;
			}
		}


		/* Calc number of various info 'flags' in original and clone games */
		if (ci_strcmp (drivers[i]->source_file+12, "neogeo.c"))
		{
			if (drivers[i]->flags & GAME_NOT_WORKING)
			{
	 			flags[1]++;
				if (clone) flags[11]++;
			}

			if (drivers[i]->flags & GAME_UNEMULATED_PROTECTION)
			{
	 			flags[2]++;
				if (clone) flags[12]++;
			}
			if (drivers[i]->flags & GAME_IMPERFECT_GRAPHICS)
			{
	 			flags[3]++;
				if (clone) flags[13]++;
			}
			if (drivers[i]->flags & GAME_WRONG_COLORS)
			{
	 			flags[4]++;
				if (clone) flags[14]++;
			}
			if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
			{
	 			flags[5]++;
				if (clone) flags[15]++;
			}
			if (drivers[i]->flags & GAME_NO_SOUND)
			{
	 			flags[6]++;
				if (clone) flags[16]++;
			}
			if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
			{
	 			flags[7]++;
				if (clone) flags[17]++;
			}
			if (drivers[i]->flags & GAME_NO_COCKTAIL)
			{
	 			flags[8]++;
				if (clone) flags[18]++;
			}

			if (drv.video_attributes & VIDEO_DUAL_MONITOR)
			{
	 			dual++;
				if (clone) dualc++;
			}

			if (drv.video_attributes & VIDEO_NEEDS_6BITS_PER_GUN)
			{
	 			bitx++;
				if (clone) bitc++;
			}

			if (drv.video_attributes & VIDEO_RGB_DIRECT)
			{
	 			rgbd++;
				if (clone) rgbdc++;
			}

			if (drv.video_attributes & VIDEO_HAS_SHADOWS)
			{
	 			shad++;
				if (clone) shadc++;
			}

			if (drv.video_attributes & VIDEO_HAS_HIGHLIGHTS)
			{
	 			hghs++;
				if (clone) hghsc++;
			}


			if (!clone)
			{
				/* Calc all CPU's only in ORIGINAL games */
				y = 0;
				for (x = 1; x < CPU_COUNT; x++)
				{
					for (n = 0; n < MAX_CPU; n++)
					{
						if (!ci_strcmp (cputype_name(drv.cpu[n].cpu_type), cputype_name(x)))
						{
							if (n < 4) numcpu[n][x]++;
							y++;
						}
					}

				}

				sumcpu[y]++;


				/* Calc all Sound hardware only in ORIGINAL games */
				y = 0;
				for (x = 1; x < SOUND_COUNT; x++)
				{
					for (n = 0; n < MAX_SOUND; n++)
					{
						if (!ci_strcmp (sound_name(&drv.sound[n]), sndintf[x].name))
						{
							if (n < 4)
							{
								numsnd[n][x]++;
								if(sound_num(&drv.sound[n]))
								 	numsnd[n][x] = numsnd[n][x] + sound_num(&drv.sound[n]) - 1;
							}
							y++;
						}
					}

				}

				sumsound[y]++;

			}


		}


		/* Calc number of ROM files and file size */
		for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				int length, in_parent, is_disk;
				is_disk = ROMREGION_ISDISKDATA(region);



				length = 0;
				in_parent = 0;

				for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
					length += ROM_GETLENGTH(chunk)/32;

				if (drivers[i]->clone_of)
				{
					fprom=NULL;
					for (pregion = rom_first_region(drivers[i]->clone_of); pregion; pregion = rom_next_region(pregion))
						for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
							if (hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(prom), 0))
							{
								if (!fprom || !strcmp(ROM_GETNAME(prom), name))
									fprom=prom;
								in_parent = 1;
							}

				}

				if (!is_disk)
				{
					sum += length;
					rsum += length;
					files++;

					if(in_parent)
					{
						xsum += length;
						mfiles++;
					}
				}
				else
					hddisk++;

				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
				{
					ndsum++;
					ndgame = 1;
				}

				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
				{
					bdsum++;
					bdgame = 1;
				}

			}

		if (ndgame)
		{
			gndsum++;
			if (clone) gndsumc++;
			ndgame = 0;
		}

		if (bdgame)
		{
			gbdsum++;
			if (clone) gbdsumc++;
			bdgame = 0;
		}


		rsum = rsum/32;
		if(rsum < 5)
			romsize[1]++;
		if(rsum >= 5 && rsum < 10)
			romsize[2]++;
		if(rsum >= 10 && rsum < 50)
			romsize[3]++;
		if(rsum >= 50 && rsum < 100)
			romsize[4]++;
		if(rsum >= 100 && rsum < 500)
			romsize[5]++;
		if(rsum >= 500 && rsum < 1000)
			romsize[6]++;
		if(rsum >= 1000 && rsum < 5000)
			romsize[7]++;
		if(rsum >= 5000 && rsum < 10000)
			romsize[8]++;
		if(rsum >= 10000 && rsum < 50000)
			romsize[9]++;
		if(rsum >= 50000)
			romsize[10]++;
		rsum = 0;

	}

	sum = sum/32;
	xsum = xsum/32;
	noyear = all; /* See Print Year and Games */
	noinput = all; /* See Input */
	nobutt = all; /* See Input Buttons */


	sprintf(name, "\n\n %4d GAMES (ALL)\n %4d ORIGINALS  + %4d CLNS\n %4d NEOGEO     + %4d\n", all, all-cl-neo+neoc, cl-neoc, neo-neoc, neoc);
	strcat(buffer, name);
	sprintf(name, " %4d PLAYCHOICE + %4d\n %4d DECO CASS  + %4d\n %4d CVS        + %4d\n", pch-pchc, pchc, deco-decoc, decoc, cvs-cvsc, cvsc);
	strcat(buffer, name);
	sprintf(name, " %4d RASTER     + %4d\n %4d VECTOR     + %4d\n", all-vec-cl+vecc, cl-vecc, vec-vecc, vecc);
	strcat(buffer, name);
	sprintf(name, " %4d HORIZONTAL + %4d\n %4d VERTICAL   + %4d\n", horizontal-horizontalc, horizontalc, vertical-verticalc, verticalc);
	strcat(buffer, name);
	sprintf(name, " %4d STEREO     + %4d\n", stereo-stereoc, stereoc);
	strcat(buffer, name);
	sprintf(name, " %4d HARDDISK\n", hddisk);
	strcat(buffer, name);
/*
	for (i = 0; test_drivers[i]; i++)
	sprintf(name, " %4d TESTGAMES\n", i+1);
	strcat(buffer, name);
*/

	/* Print number of various info 'flags' */
	strcat(buffer,"\n\nGAME INFO FLAGS   : ORIG CLNS\n\n");
	sprintf(name, "NON-WORKING        : %3d  %3d\n", flags[1]-flags[11], flags[11]);
	strcat(buffer, name);
	sprintf(name, "UNEMULATED PROTEC. : %3d  %3d\n", flags[2]-flags[12], flags[12]);
	strcat(buffer, name);
	sprintf(name, "IMPERFECT GRAPHICS : %3d  %3d\n", flags[3]-flags[13], flags[13]);
	strcat(buffer, name);
	sprintf(name, "WRONG COLORS       : %3d  %3d\n", flags[4]-flags[14], flags[14]);
	strcat(buffer, name);
	sprintf(name, "IMPERFECT COLORS   : %3d  %3d\n", flags[5]-flags[15], flags[15]);
	strcat(buffer, name);
	sprintf(name, "NO SOUND           : %3d  %3d\n", flags[6]-flags[16], flags[16]);
	strcat(buffer, name);
	sprintf(name, "IMPERFECT SOUND    : %3d  %3d\n", flags[7]-flags[17], flags[17]);
	strcat(buffer, name);
	sprintf(name, "NO COCKTAIL        : %3d  %3d\n", flags[8]-flags[18], flags[18]);
	strcat(buffer, name);
	sprintf(name, "NO GOOD DUMP KNOWN : %3d  %3d\n(%3d ROMS IN %d GAMES)\n", gndsum-gndsumc, gndsumc, ndsum, gndsum);
	strcat(buffer, name);
	sprintf(name, "ROM NEEDS REDUMP   : %3d  %3d\n(%3d ROMS IN %d GAMES)\n", gbdsum-gbdsumc, gbdsumc, bdsum, gbdsum);
	strcat(buffer, name);




	/* Print Year and Games - Note: Some games have no year*/
	strcat(buffer,"\n\nYEAR: ORIG  CLNS NEOGEO  ALL\n\n");
	for (x = 1972; x < 2004; x++)
	{

		all = 0; cl = 0; neo = 0; neoc = 0;

		sprintf(year,"%d",x);
		for (i = 0; drivers[i]; i++)
		{
			if (!ci_strcmp (year, drivers[i]->year))
			{ 
				all++;

				if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
					cl++;
				if (!ci_strcmp (drivers[i]->source_file+12, "neogeo.c"))
				{
					neo++;
					if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
						neoc++;
				}
			}
		}

		sprintf(name, "%d: %3d   %3d   %3d    %3d\n", x, all-cl-neo+neoc, cl-neoc, neo, all);
		strcat(buffer, name);

		noyear = noyear - all; /* Number of games with no year informations */

	}
	sprintf(name, "19??:                    %3d\n\n", noyear);
	strcat(buffer, name);


	/* Print number of ROM files and whole file size */
	if(sum > 524288)
		sprintf(name, "\nROMFILES: %d\n%d + %d MERGED ROMS\n\nFILESIZE: %d MB (UNCOMPRESSED)\n%d MB + %d MB MERGED\n", files, files-mfiles, mfiles, sum/1024, sum/1024-xsum/1024, xsum/1024);
	else
		sprintf(name, "\n\nROMFILES: %d\n%d + %d MERGED ROMS\n\nFILESIZE: %d KB (UNCOMPRESSED)\n%d KB + %d KB MERGED\n", files, files-mfiles, mfiles, sum, sum-xsum, xsum);
	strcat(buffer, name);



	/* Print the typical sizes of all supported romsets in kbytes */
	strcat(buffer,"\n\nTYPICAL ROMSET SIZE (ALL)\n");

	sprintf(name, "\n    0 -     5 KB:  %4d\n    5 -    10 KB:  %4d\n   10 -    50 KB:  %4d\n   50 -   100 KB:  %4d\n", romsize[1], romsize[2], romsize[3], romsize[4]);
	strcat(buffer, name);
	sprintf(name, "  100 -   500 KB:  %4d\n  500 -  1000 KB:  %4d\n 1000 -  5000 KB:  %4d\n 5000 - 10000 KB:  %4d\n", romsize[5], romsize[6], romsize[7], romsize[8]);
	strcat(buffer, name);
	sprintf(name, "10000 - 50000 KB:  %4d\n      > 50000 KB:  %4d\n", romsize[9], romsize[10]);
	strcat(buffer, name);



	/* Print year and the number of added games */
	strcat(buffer,"\n\nYEAR    NEW GAMES\n\n");
	for (i = 0; newgames[i]; i++)
	{
		strcat(buffer,newgames[i]);
		strcat(buffer,"\n");
	}



	/* Print all games and their maximum CPU's */
	strcat(buffer,"\n\nCPU HARDWARE (ORIGINAL GAMES)\n\n");
	for (n = 0; n < MAX_CPU+1; n++)
	{
		if (sumcpu[n])
		{
			sprintf(name, "     GAMES WITH  %d CPUs: %4d\n", n, sumcpu[n]);
			strcat(buffer, name);
		}

	}


	/* Print all used CPU's and numbers of original games they used */
	strcat(buffer,"\n     CPU:  (1)  (2)  (3)  (4)\n\n");
	all = 0;
	for (i = 1; i < CPU_COUNT; i++)
	{
		if (numcpu[0][i] || numcpu[1][i] || numcpu[2][i] || numcpu[3][i])
		{
			sprintf(name, "%8s:  %3d  %3d  %3d  %3d\n", cputype_name(i), numcpu[0][i], numcpu[1][i], numcpu[2][i], numcpu[3][i]);
			if (!ci_strcmp(cputype_name(i), "DECO CPU16"))
				sprintf(name, "%8s:  %3d  %3d  %3d  %3d\n", "DECO16", numcpu[0][i], numcpu[1][i], numcpu[2][i], numcpu[3][i]);
			if (!ci_strcmp(cputype_name(i), "TMS9980A/TMS9981"))
				sprintf(name, "%8s:  %3d  %3d  %3d  %3d\n", "TMS9980A", numcpu[0][i], numcpu[1][i], numcpu[2][i], numcpu[3][i]);
			if (!ci_strcmp(cputype_name(i), "Jaguar GPU"))
				sprintf(name, "%8s:  %3d  %3d  %3d  %3d\n", "Jag. GPU", numcpu[0][i], numcpu[1][i], numcpu[2][i], numcpu[3][i]);
			if (!ci_strcmp(cputype_name(i), "Jaguar DSP"))
				sprintf(name, "%8s:  %3d  %3d  %3d  %3d\n", "Jag. DSP", numcpu[0][i], numcpu[1][i], numcpu[2][i], numcpu[3][i]);
			strcat(buffer, name);
			all = all + numcpu[0][i] + numcpu[1][i] + numcpu[2][i] + numcpu[3][i];
		}
	}

	/* Print the number of all CPU the original games */
	sprintf(name, "\n   TOTAL: %4d\n", all);
	strcat(buffer, name);


	/* Print all games and their maximum number of sound subsystems */
	strcat(buffer,"\n\nSOUND SYSTEM (ORIGINAL GAMES)\n\n");
	for (n = 0; n < MAX_SOUND+1; n++)
	{
		if (sumsound[n])
		{
			sprintf(name, " GAMES WITH  %d SNDINTRF: %4d\n", n, sumsound[n]);
			strcat(buffer, name);
		}

	}

	/* Print all Sound hardware and numbers of games they used */
	strcat(buffer,"\n SNDINTRF: (1)  (2)  (3)  (4)\n\n");
	all = 0;
	for (i = 1; i < SOUND_COUNT; i++)
	{
		if (numsnd[0][i] || numsnd[1][i] || numsnd[2][i] || numsnd[3][i])
		{
			sprintf(name, "%9s: %3d  %3d  %3d  %3d\n", sndintf[i].name, numsnd[0][i], numsnd[1][i], numsnd[2][i], numsnd[3][i]);
			if (!ci_strcmp(sndintf[i].name, "Discrete Components"))
				sprintf(name, "%9s: %3d  %3d  %3d  %3d\n", "Discrete", numsnd[0][i], numsnd[1][i], numsnd[2][i], numsnd[3][i]);
			if (!ci_strcmp(sndintf[i].name, "GAELCO GAE1"))
				sprintf(name, "%9s: %3d  %3d  %3d  %3d\n", "GA.GAE1 ", numsnd[0][i], numsnd[1][i], numsnd[2][i], numsnd[3][i]);
			if (!ci_strcmp(sndintf[i].name, "GAELCO CG-1V"))
				sprintf(name, "%9s: %3d  %3d  %3d  %3d\n", "GA.CG-1V", numsnd[0][i], numsnd[1][i], numsnd[2][i], numsnd[3][i]);
			if (!ci_strcmp(sndintf[i].name, "Sega 315-5560"))
				sprintf(name, "%9s: %3d  %3d  %3d  %3d\n", "Sega 315", numsnd[0][i], numsnd[1][i], numsnd[2][i], numsnd[3][i]);

			strcat(buffer, name);
			all = all + numsnd[0][i] + numsnd[1][i] + numsnd[2][i] + numsnd[3][i];
		}
	}

	sprintf(name, "\n    TOTAL: %4d\n", all);
	strcat(buffer, name);


	/* Print all Input Controls and numbers of all games */
	strcat(buffer, "\n\nCABINET INPUT CONTROL: (ALL)\n\n");
	for (n = 1; n < 9; n++)
	{
		if (control[n])
		{

			sprintf(name, "     PLAYERS %d:  %4d\n", n, control[n]);
			strcat(buffer, name);
		}
	}

	strcat(buffer, "\n");
	if (control[21]) { sprintf(name, "       JOY2WAY:  %4d\n", control[21]); strcat(buffer, name); noinput -= control[21]; }
	if (control[22]) { sprintf(name, "       JOY4WAY:  %4d\n", control[22]); strcat(buffer, name); noinput -= control[22]; }
	if (control[23]) { sprintf(name, "       JOY8WAY:  %4d\n", control[23]); strcat(buffer, name); noinput -= control[23]; }
	if (control[24]) { sprintf(name, " DOUBLEJOY2WAY:  %4d\n", control[24]); strcat(buffer, name); noinput -= control[24]; }
	if (control[25]) { sprintf(name, " DOUBLEJOY4WAY:  %4d\n", control[25]); strcat(buffer, name); noinput -= control[25]; }
	if (control[26]) { sprintf(name, " DOUBLEJOY8WAY:  %4d\n", control[26]); strcat(buffer, name); noinput -= control[26]; }
	if (control[27]) { sprintf(name, "        PADDLE:  %4d\n", control[27]); strcat(buffer, name); noinput -= control[27]; }
	if (control[28]) { sprintf(name, "          DIAL:  %4d\n", control[28]); strcat(buffer, name); noinput -= control[28]; }
	if (control[29]) { sprintf(name, "     TRACKBALL:  %4d\n", control[29]); strcat(buffer, name); noinput -= control[29]; }
	if (control[30]) { sprintf(name, "         STICK:  %4d\n", control[30]); strcat(buffer, name); noinput -= control[30]; }
	if (control[31]) { sprintf(name, "      LIGHTGUN:  %4d\n", control[31]); strcat(buffer, name); noinput -= control[31]; }

	sprintf(name, "         OTHER:  %4d\n", noinput);
	strcat(buffer, name);

	strcat(buffer, "\n");
	for (n = 11; n < 21; n++)
	{
		if (control[n])
		{

			sprintf(name, "    BUTTONS%3d:  %4d\n", n-10, control[n]);
			strcat(buffer, name);
			nobutt = nobutt - control[n];			
		}
	}

	sprintf(name, "    NO BUTTONS:  %4d\n", nobutt);
	strcat(buffer, name);




	/* VIDEO_ASPECT_RATIO: Sort all games video ratio by x and y */
	i = 0;
	for (cl = 0; cl < 15; cl++)
	{
		x = 1280;
		for (n = 0; n < 15; n++)
		{
			if (asx[n] && x > asx[n])
				x = asx[n];
		}

		y = 1280;
		for (n = 0; n < 15; n++)
		{
			if (x == asx[n] && y > asy[n])
				y = asy[n];
		}

		for (n = 0; n < 15; n++)
		{
			if (x == asx[n] && y == asy[n])
			{
				/* Store all sorted resolutions in the higher array */
				asx[15+i] = asx[n];
				asy[15+i] = asy[n];
				asnum[15+i] = asnum[n];
				i++;
				asx[n] = 0, asy[n] = 0, asnum[n] = 0;
			}
		}
	}

	/* Print all VIDEO_ASPECT_RATIO's */
	strcat(buffer,"\n\nVIDEO ASPECT RATIO X:Y (ALL)\n\n");
	for (n = 15; n < 30; n++)
	{
		if (asx[n])
		{
			sprintf(name, "%4d   :%4d    | %4d\n", asx[n], asy[n], asnum[n]);
			strcat(buffer, name);
			if (i > 14)
				strcat(buffer, "\nWARNING: ASPECT RATIO number too high!\n");
		}

	}

	/* Print the video_attributes */
	strcat(buffer,"\n\nVIDEO NEEDS... : ORIG   CLNS\n\n");
	sprintf(name, "DUAL MONITOR   : %3d  + %3d\n", dual-dualc, dualc);
	strcat(buffer, name);
	sprintf(name, "24-BIT DISPLAY : %3d  + %3d\n", bitx-bitc, bitc);
	strcat(buffer, name);
	sprintf(name, "HI/TRUE BITMAP : %3d  + %3d\n", rgbd-rgbdc, rgbdc);
	strcat(buffer, name);
	sprintf(name, "SHADOWS        : %3d  + %3d\n", shad-shadc, shadc);
	strcat(buffer, name);
	sprintf(name, "HIGHLIGHTS     : %3d  + %3d\n", hghs-hghsc, hghsc);
	strcat(buffer, name);


	/* FRAMES_PER_SECOND: Sort and print all fps */
	sprintf(name,"\n\nFRAMES PER SECOND (%d): (ALL)\n\n", fpsnum[0]);
	strcat(buffer, name);
	for (y = 1; y < 35; y++)
	{
		fps[0] = 199;
		for (n = 1; n < 35; n++)
		{
			if (fpsnum[n] && fps[0] > fps[n])
				fps[0] = fps[n];
		}

		for (n = 1; n < 35; n++)  /* Print fps and number*/
		{
			if (fps[0] == fps[n])
			{
				sprintf(name, "  FPS %f:  %4d\n", fps[n], fpsnum[n]);
				strcat(buffer, name);
				fpsnum[n] = 0;
			}
		}

	}

	if (fpsnum[0] > 33)
		strcat(buffer, "\nWARNING: FPS number too high!\n");


	/* RESOLUTION: Sort all games resolutions by x and y */
	cl = 0;
	for (all = 0; all < 200; all++)
	{
		x = 999;
		for (n = 0; n < 200; n++)
		{
			if (resx[n] && x > resx[n])
				x = resx[n];
		}

		y = 999;
		for (n = 0; n < 200; n++)
		{
			if (x == resx[n] && y > resy[n])
				y = resy[n];
		}

		for (n = 0; n < 200; n++)
		{
			if (x == resx[n] && y == resy[n])
			{
				/* Store all sorted resolutions in the higher array */
				resx[200+cl] = resx[n];
				resy[200+cl] = resy[n];
				resnum[200+cl] = resnum[n];
				cl++;
				resx[n] = 0, resy[n] = 0, resnum[n] = 0;
			}
		}
	}


	/* PALETTESIZE: Sort the palettesizes */
	x = 0;
	for (y = 0; y < 100; y++)
	{
		i = 99999;
		for (n = 0; n < 100; n++)
		{
			if (palett[n] && i > palett[n])
				i = palett[n];
		}

		for (n = 0; n < 100; n++)  /* Store all sorted resolutions in the higher array */
		{
			if (i == palett[n])
			{
				palett[100+x] = palett[n];
				palettnum[100+x] = palettnum[n];
				x++;
				palett[n] = 0, palettnum[n] = 0;
			}
		}
	}

	/* RESOLUTION & PALETTESIZE: Print all resolutions and palettesizes */
	sprintf(name,"\n\nRESOLUTION & PALETTESIZE: (ALL)\n    (%d)          (%d)\n\n", cl, x);
	strcat(buffer, name);
	for (n = 100; n < 300; n++)
	{

		if (resx[n+100])
		{
			sprintf(name, "  %dx%d: %3d    ", resx[n+100], resy[n+100], resnum[n+100]);
			strcat(buffer, name);
		}

		if (palett[n])
		{
			sprintf(name, "%5d: %3d\n", palett[n], palettnum[n]);
			strcat(buffer, name);
		}
		else
			if (resx[n+100])	strcat(buffer, "\n");

	}

	if (cl > 198)
		strcat(buffer, "\nWARNING: Resolution number too high!\n");

	if (x > 98)
		strcat(buffer, "\nWARNING: Palettesize number too high!\n");


	/* MAME HISTORY - Print all MAME versions + Drivers + Supported Games (generated with MAMEDiff) */
	strcat(buffer,"\n\nVERSION - DRIVERS - SUPPORT:\n\n");
	for (i = 0; history[i]; i++)
	{
		strcat(buffer,history[i]);
		strcat(buffer,"\n");
	}


	/* Calc all MAME versions and print all version */
	for (i = 0; versions[i]; i++){}
	sprintf(name, "\n\nMAME VERSIONS (%3d)\n\n", i);
	strcat(buffer, name);

	for (i = 0; versions[i]; i++)
	{
		strcat(buffer,versions[i]);
		strcat(buffer,"\n");
	}


	/* CLEAR ALL DATA ARRAYS */
	for (i = 0; i < 400; i++)
	{
		numcpu[0][i] = 0, numcpu[1][i] = 0, numcpu[2][i] = 0, numcpu[3][i] = 0;
		numsnd[0][i] = 0, numsnd[1][i] = 0, numsnd[2][i] = 0, numsnd[3][i] = 0;
		resx[i] = 0, resy[i] = 0, resnum[i] = 0;
		palett[i] = 0, palettnum[i] = 0;
	}

	for (i = 0; i < 35; i++)
	{
		flags[i] = 0, romsize[i] = 0;
		asx[i] = 0, asy[i] = 0, asnum[i] = 0;
		control[i] = 0, fps[i] = 0, fpsnum[i] = 0;
		sumcpu[i] = 0, sumsound[i] = 0;
	}



	return 0;

}

