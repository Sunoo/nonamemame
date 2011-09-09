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

#define MAX_TOKEN_LENGTH        1024


/****************************************************************************
 *      datafile constants
 ****************************************************************************/
#define MAX_DATAFILE_ENTRIES 5000
#define MAX_MENUIDX_ENTRIES 64
#define DATAFILE_TAG '$'

const char *DATAFILE_TAG_KEY = "$info";
const char *DATAFILE_TAG_BIO = "$bio";
const char *DATAFILE_TAG_MAME = "$mame";
#ifdef MASH_DATAFILE
const char *DATAFILE_TAG_DRIV = "$drv";
#endif /* MASH_DATAFILE */

#ifdef CMD_LIST
const char *DATAFILE_TAG_COMMAND = "$cmd";
const char *DATAFILE_TAG_END = "$end";
#endif /* CMD_LIST */

const char *history_filename = NULL;
const char *mameinfo_filename = NULL;
#ifdef CMD_LIST
const char *command_filename = NULL;
#endif /* CMD_LIST */

#ifdef MASH_DATAFILE
static struct tDatafileIndex *hist_idx = 0;
static struct tDatafileIndex *hist2_idx = 0;
static struct tDatafileIndex *mame_idx = 0;
static struct tDatafileIndex *driv_idx = 0;
#endif /* MASH_DATAFILE */

#ifdef CMD_LIST
static struct tDatafileIndex *cmnd_idx = 0;
static struct tDatafileIndex *cmnd2_idx = 0;
static struct tMenuIndex *menu_idx = 0;

static int mame32jp_wrap;
#endif /* CMD_LIST */

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
#ifdef CMD_LIST
	UINT8 space, character;

	if (mame32jp_wrap)
	{
		space     = '\t';
		character = ' ' - 1;
	}
	else
	{
		space     = ' ';
		character = ' ';
	}
#endif /* CMD_LIST */

        while (1)
        {
                bData = mame_fgetc(fp);                                  /* Get next character */

                /* If we're at the end of the file, bail out */

                if (mame_feof(fp))
                        return(TOKEN_INVALID);

                /* If it's not whitespace, then let's start eating characters */

#ifdef CMD_LIST
		if (space != bData && '\t' != bData)
#else /* CMD_LIST */
                if (' ' != bData && '\t' != bData)
#endif /* CMD_LIST */
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

#ifdef CMD_LIST
			if (bData > character)
#else /* CMD_LIST */
                        if (bData > ' ')
#endif /* CMD_LIST */
                        {
                                dwLength = 0;                   /* Assume we're 0 length to start with */

                                /* Loop until we've hit something we don't understand */

                                while (bData != ',' &&
                                                 bData != '=' &&
#ifdef CMD_LIST
						 bData != space &&
#else /* CMD_LIST */
                                                 bData != ' ' &&
#endif /* CMD_LIST */
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
#ifdef CMD_LIST
					if (mame32jp_wrap)
					{
						mame_ungetc(bData, fp);

						*pbTokenPtr++ = bData;	/* A real carriage return (hard) */
						*pbTokenPtr = '\0';

						return(TOKEN_LINEBREAK);
					}
					else
					{
#endif /* CMD_LIST */
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
#ifdef CMD_LIST
				}
#endif /* CMD_LIST */
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
 *	Returns 0 on error, or the number of index entries created.
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
						/* done = 1;  Not done, as we must process other clones in list */
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

#ifdef CMD_LIST
/**************************************************************************
 *	index_menuidx
 *	
 *
 *      Returns 0 on error, or the number of index entries created.
 **************************************************************************/
static int index_menuidx (const struct GameDriver *drv, struct tDatafileIndex *d_idx, struct tMenuIndex **_index)
{
	struct tMenuIndex *m_idx;
	int m_count = 0;
        UINT32 token = TOKEN_SYMBOL;

	long tell;
	long cmdtag_offset = 0;
	char *s;

	mame32jp_wrap = 1;

	/* find driver in datafile index */
	while (d_idx->driver)
        {
		if (d_idx->driver == drv || d_idx->driver == drv->clone_of) break;
		d_idx++;
	}
	if (d_idx->driver == 0) return 0;	/* driver not found in Data_file_index */

	/* seek to correct point in datafile */
	if (ParseSeek (d_idx->offset, SEEK_SET)) return 0;

	/* allocate index */
	m_idx = *_index = malloc(MAX_MENUIDX_ENTRIES * sizeof (struct tMenuIndex));
	if (NULL == m_idx) return 0;

	/* loop through between $cmd= */
	token = GetNextToken ((UINT8 **)&s, &tell);
	while ((m_count < (MAX_MENUIDX_ENTRIES - 1)) && TOKEN_INVALID != token)
	{
                if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
			break;

		/* DATAFILE_TAG_COMMAND identifies the driver */
		if (!ci_strncmp (DATAFILE_TAG_COMMAND, s, strlen (DATAFILE_TAG_COMMAND)))
                {
			cmdtag_offset = tell;
                        token = GetNextToken ((UINT8 **)&s, &tell);

                        if (TOKEN_EQUALS == token)
                        {
                                token = GetNextToken ((UINT8 **)&s, &tell);
				m_idx->menuitem = malloc(strlen(s)+1);
				strcpy(m_idx->menuitem, s);

				m_idx->offset = cmdtag_offset;

				m_idx++;
				m_count++;
			}
			else
									{
				while (TOKEN_SYMBOL != token)
										token = GetNextToken ((UINT8 **)&s, &tell);

				m_idx->menuitem = malloc( strlen( s ) + 1 );
				strcpy( m_idx->menuitem, s );

				m_idx->offset = cmdtag_offset;

				m_idx++;
				m_count++;
                        }
                }
		token = GetNextToken ((UINT8 **)&s, &tell);
        }

        /* mark end of index */
	m_idx->offset = 0L;
	m_idx->menuitem = NULL;

	return m_count;
}
#endif /* CMD_LIST */

#ifdef MASH_DATAFILE
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
						/* done = 1;  Not done, as we must process other clones in list */
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
#endif /* MASH_DATAFILE */

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
#ifdef CMD_LIST
	int first = 1;
#endif /* CMD_LIST */

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
#ifdef CMD_LIST
			if (!mame32jp_wrap && TOKEN_LINEBREAK != token)
#else /* CMD_LIST */
                        if (TOKEN_LINEBREAK != token)
#endif /* CMD_LIST */
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
				{
                                        found = 1;
#ifdef CMD_LIST
					if (first && mame32jp_wrap)
					{
						mame_fseek(fp, 2l, SEEK_CUR);
						first = 0;
					}
#endif /* CMD_LIST */
				}
                                else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
                                        break; /* error: tag missing */
                        }
                }
        }
        return (!found);
}

#ifdef CMD_LIST
/**************************************************************************
 *	load_datafile_text_ex
 *
 *	Loads text field for a driver into the buffer specified. Specify the
 *	driver, a pointer to the buffer, the buffer size, the index created by
 *	index_datafile(), and the desired text field (e.g., DATAFILE_TAG_BIO).
 *
 *	Returns 0 if successful.
 **************************************************************************/
static int load_datafile_text_ex (const struct GameDriver *drv, char *buffer, int bufsize,
	const char *tag, struct tMenuIndex *m_idx, const int menu_sel)
{
	int	offset = 0;
	int found = 0;
	UINT32	token = TOKEN_SYMBOL;
	UINT32 	prev_token = TOKEN_SYMBOL;
	int first = 1;

	*buffer = '\0';

	/* seek to correct point in datafile */
	if (ParseSeek ((m_idx + menu_sel)->offset, SEEK_SET)) return 1;

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
			if (TOKEN_SYMBOL == token && !ci_strncmp (DATAFILE_TAG_END, s, strlen (DATAFILE_TAG_END))) break;

			prev_token = token;

			/* translate platform-specific linebreaks to '\n' */
			if (TOKEN_LINEBREAK == token)
				strcpy (s, "\n");

			/* append a space to words */
			if (!mame32jp_wrap && TOKEN_LINEBREAK != token)
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
				{
					found = 1;
					token = GetNextToken ((UINT8 **)&s, &tell);
					if ( TOKEN_EQUALS == token )
					{
						token = GetNextToken ((UINT8 **)&s, &tell);
						token = GetNextToken ((UINT8 **)&s, &tell);
					}

					if (first) first = 0;
				}
				else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
					break; /* error: tag missing */
			}
		}
	}
	return (!found);
}
#endif /* CMD_LIST */

#ifdef MASH_DATAFILE
static int load_drivfile_text (const struct GameDriver *drv, char *buffer, int bufsize,
	struct tDatafileIndex *idx, const char *tag)
{
	int     offset = 0;
	int     found = 0;
	UINT32  token = TOKEN_SYMBOL;
	UINT32  prev_token = TOKEN_SYMBOL;
	struct tDatafileIndex *idx_save = idx;

	*buffer = '\0';

	/* find driver in datafile index */
	while (idx->driver)
	{
		if (idx->driver->source_file == drv->source_file) break;
		idx++;
	}
	// MSVC doesn't work above, retry but slow. :-(
	if (idx->driver == 0)
	{
		idx = idx_save;

		while (idx->driver)
		{
			if (!strcmp(idx->driver->source_file, drv->source_file))
				break;
			idx++;
		}
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
#endif /* MASH_DATAFILE */

/**************************************************************************
 *	load_driver_history
 *	Load history text for the specified driver into the specified buffer.
 *	Combines $bio field of HISTORY.DAT with $mame field of MAMEINFO.DAT.
 *
 *	Returns 0 if successful.
 *
 *	NOTE: For efficiency the indices are never freed (intentional leak).
 **************************************************************************/
int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize)
{
#ifdef MASH_DATAFILE
	int history = 0, history2 = 0;
#else /* MASH_DATAFILE */
	static struct tDatafileIndex *hist_idx = 0, *hist2_idx = 0;
	static struct tDatafileIndex *mame_idx = 0;
	int history = 0, history2 = 0, mameinfo = 0;
#endif /* MASH_DATAFILE */
	int err;
	char localhistory_filename[80];
	static int oldLangCode = -1;

	*buffer = 0;

#ifdef CMD_LIST
	mame32jp_wrap = 0;
#endif /* CMD_LIST */

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
				int len = strlen (buffer);
				err = load_datafile_text (gdrv, buffer+len, bufsize-len,
                                                                                  hist_idx, DATAFILE_TAG_BIO);
                                gdrv = gdrv->clone_of;
                        } while (err && gdrv);

                        if (err) history = 0;
                }
                ParseClose ();
        }

	/* try to open history2 datafile */
	if (ParseOpen (localhistory_filename))
	{
		/* create index if necessary */
		if (!hist2_idx || oldLangCode != options.langcode)
		{
			oldLangCode = options.langcode;
			if (hist2_idx)
			{
				free(hist2_idx);
				hist2_idx = 0;
			}
			history2 = (index_datafile (&hist2_idx) != 0);
		}
		else
			history2 = 1;

		/* load history text */
		if (hist2_idx)
		{
			int len = strlen (buffer);
			err = load_datafile_text (drv, buffer+len, bufsize-len,
			                          hist2_idx, DATAFILE_TAG_BIO);
			if (err && drv->clone_of)
				err = load_datafile_text (drv->clone_of, buffer+len, bufsize-len,
				                          hist2_idx, DATAFILE_TAG_BIO);

			if (err) history2 = 0;
		}
		ParseClose ();
	}
	
#ifdef MASH_DATAFILE
	return (history == 0 && history2 == 0);
#else /* MASH_DATAFILE */
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

	return (history == 0 && history2 == 0 && mameinfo == 0);
#endif /* MASH_DATAFILE */
}

#ifdef MASH_DATAFILE
int load_driver_mameinfo (const struct GameDriver *drv, char *buffer, int bufsize)
{
	const struct RomModule *region, *rom, *chunk;
	struct InternalMachineDriver game;
	int mameinfo = 0;
	int err;
	int	i;

	*buffer = 0;

	strcat(buffer, "MAMEInfo:\n");
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
	int drivinfo = 0;
	int err;
	// int i;

	*buffer = 0;

	/* Print source code file */
	sprintf (buffer, "\nSOURCE: %s\n", drv->source_file+12);

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

	return (drivinfo == 0);

}

#endif /* MASH_DATAFILE */

#ifdef CMD_LIST
/**************************************************************************
 *	load_driver_command
 **************************************************************************/
int load_driver_command (const struct GameDriver *drv, char *buffer, int bufsize)
{
	int command = 0;
	int err = 0;
	char localcommand_filename[80];

	*buffer = 0;
	mame32jp_wrap = 1;

	if(!command_filename)
		command_filename = "command.dat";

	/* try to open local command datafile */
	if (ParseOpen (localcommand_filename))
	{
		/* create index if necessary */
		if (cmnd2_idx)
			command = 1;
		else
			command = (index_datafile (&cmnd2_idx) != 0);

		/* load informational text (append) */
		if (cmnd2_idx)
		{
			const struct GameDriver *gdrv;

			gdrv = drv;
			do
	{
				err = load_datafile_text (gdrv, buffer, bufsize,
				                          cmnd2_idx, DATAFILE_TAG_COMMAND);
				gdrv = gdrv->clone_of;
			} while (err && gdrv);

			if (err) command = 0;
		}
		ParseClose ();
	}

	if (command)
		return (command == 0);

	/* try to open command datafile */
	if (ParseOpen (command_filename))
	{
		/* create index if necessary */
		if (cmnd_idx)
			command = 1;
		else
			command = (index_datafile (&cmnd_idx) != 0);

		/* load informational text (append) */
		if (cmnd_idx)
		{
			const struct GameDriver *gdrv;

			gdrv = drv;
			do
			{
				err = load_datafile_text (gdrv, buffer, bufsize,
				                          cmnd_idx, DATAFILE_TAG_COMMAND);
				gdrv = gdrv->clone_of;
			} while (err && gdrv);

			if (err) command = 0;
		}
		ParseClose ();
	}

	return (command == 0);
}


/**************************************************************************
 *	load_driver_command_ex
 **************************************************************************/
int load_driver_command_ex (const struct GameDriver *drv, char *buffer, int bufsize, const int menu_sel)
{
	int command = 0;
	int err = 0;
	char localcommand_filename[80];

	*buffer = 0;
	mame32jp_wrap = 1;

	if(!command_filename)
		command_filename = "command.dat";

	/* try to open local command datafile */
	if (ParseOpen (localcommand_filename))
		{
		/* create index if necessary */
		if (cmnd2_idx)
			command = 1;
		else
			command = (index_datafile (&cmnd2_idx) != 0);

		/* create menu_index if necessary */
		if (menu_idx)
			command = 1;
		else
			command = (index_menuidx (drv, cmnd2_idx, &menu_idx) != 0);

		/* load informational text (append) */
		if (cmnd2_idx)
		{
			const struct GameDriver *gdrv;

			gdrv = drv;
			do
			{
				err = load_datafile_text_ex (gdrv, buffer, bufsize,
				                             DATAFILE_TAG_COMMAND, menu_idx, menu_sel);
				gdrv = gdrv->clone_of;
			} while (err && gdrv);

			if (err) command = 0;
		}
		ParseClose ();
			}

	if (command)
		return (command == 0);

	/* try to open command datafile */
	if (ParseOpen (command_filename))
		{
		/* create index if necessary */
		if (cmnd_idx)
			command = 1;
		else
			command = (index_datafile (&cmnd_idx) != 0);

		/* create menu_index if necessary */
		if (menu_idx)
			command = 1;
		else
			command = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);

		/* load informational text (append) */
		if (cmnd_idx)
		{
			const struct GameDriver *gdrv;

			gdrv = drv;
			do
				{
				err = load_datafile_text_ex (gdrv, buffer, bufsize,
				                             DATAFILE_TAG_COMMAND, menu_idx, menu_sel);
				gdrv = gdrv->clone_of;
			} while (err && gdrv);

			if (err) command = 0;
			}
		ParseClose ();
		}

	return (command == 0);
}


/**************************************************************************
 *	command_sub_menu
 **************************************************************************/
UINT8 command_sub_menu(const struct GameDriver *drv, const char *menu_item[])
{
	int command = 0;
	UINT8 total = 0;
	static const struct GameDriver *last_drv = 0;
	char localcommand_filename[80];

	if(!command_filename)
		command_filename = "command.dat";

	/* try to open local command datafile */
	if (ParseOpen (localcommand_filename))
					{
		/* create index if necessary */
		if (cmnd2_idx)
			command = 1;
				else
			command = (index_datafile (&cmnd2_idx) != 0);

		command = (index_menuidx (drv, cmnd2_idx, &menu_idx) != 0);
		}
	ParseClose ();

	if(!menu_idx || command == FALSE)
			{ 
		/* try to open command datafile */
		if (ParseOpen (command_filename))
				{
			/* create index if necessary */
			if (cmnd_idx)
				command = 1;
	else
				command = (index_datafile (&cmnd_idx) != 0);

			command = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);
	}
		ParseClose ();
	}

	if(menu_idx && command == TRUE)
	{
		struct tMenuIndex *m_idx = menu_idx;

		while(m_idx->menuitem != NULL)
		{
			menu_item[total++] = m_idx->menuitem;
			m_idx++;
		}
		last_drv = drv;
		return total;
		}
		else
	return 0;
}
#endif /* CMD_LIST */
