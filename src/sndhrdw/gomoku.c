/***************************************************************************

	Gomoku sound driver (quick hack of the Wiping sound driver)

***************************************************************************/

#include "driver.h"


/* 4 voices max */
#define MAX_VOICES 8


static const int samplerate = 48000;
static const int defgain = 48;


/* this structure defines the parameters for a channel */
typedef struct
{
	int frequency;
	int counter;
	int volume;
	const unsigned char *wave;
	int oneshot;
	int oneshotplaying;
} sound_channel;


/* globals available to everyone */
unsigned char *gomoku_soundregs;
unsigned char *gomoku_wavedata;

/* data about the sound system */
static sound_channel channel_list[MAX_VOICES];
static sound_channel *last_channel;

/* global sound parameters */
static const unsigned char *sound_prom, *sound_rom;
static int num_voices;
static int sound_enable;
static int stream;

/* mixer tables and internal buffers */
static INT16 *mixer_table;
static INT16 *mixer_lookup;
static short *mixer_buffer;
static short *mixer_buffer_2;



/* build a table to divide by the number of voices; gain is specified as gain*16 */
static int make_mixer_table(int voices, int gain)
{
	int count = voices * 128;
	int i;

	/* allocate memory */
	mixer_table = auto_malloc(256 * voices * sizeof(INT16));
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
	mixer_lookup = mixer_table + (128 * voices);

	/* fill in the table - 16 bit case */
	for (i = 0; i < count; i++)
	{
		int val = i * gain * 16 / voices;
		if (val > 32767) val = 32767;
		mixer_lookup[ i] = val;
		mixer_lookup[-i] = -val;
	}

	return 0;
}


/* generate sound to the mix buffer in mono */
static void gomoku_update_mono(int ch, INT16 *buffer, int length)
{
	sound_channel *voice;
	short *mix;
	int i;

	/* if no sound, we're done */
	if (sound_enable == 0)
	{
		memset(buffer, 0, length * 2);
		return;
	}

	/* zap the contents of the mixer buffer */
	memset(mixer_buffer, 0, length * sizeof(short));

	/* loop over each voice and add its contribution */
	for (voice = channel_list; voice < last_channel; voice++)
	{
		int f = 16*voice->frequency;
		int v = voice->volume;

		/* only update if we have non-zero volume and frequency */
		if (v && f)
		{
			const unsigned char *w = voice->wave;
			int c = voice->counter;

			mix = mixer_buffer;

			/* add our contribution */
			for (i = 0; i < length; i++)
			{
				int offs;

				c += f;

				if (voice->oneshot)
				{
					if (voice->oneshotplaying)
					{
						offs = (c >> 15);
						if (w[offs>>1] == 0xff)
						{
							voice->oneshotplaying = 0;
						}

						if (voice->oneshotplaying)
						{
							/* use full byte, first the high 4 bits, then the low 4 bits */
							if (offs & 1)
								*mix++ += ((w[offs>>1] & 0x0f) - 8) * v;
							else
								*mix++ += (((w[offs>>1]>>4) & 0x0f) - 8) * v;
						}
					}
				}
				else
				{
					offs = (c >> 15) & 0x1f;

					/* use full byte, first the high 4 bits, then the low 4 bits */
					if (offs & 1)
						*mix++ += ((w[offs>>1] & 0x0f) - 8) * v;
					else
						*mix++ += (((w[offs>>1]>>4) & 0x0f) - 8) * v;
				}
			}

			/* update the counter for this voice */
			voice->counter = c;
		}
	}

	/* mix it down */
	mix = mixer_buffer;
	for (i = 0; i < length; i++)
		*buffer++ = mixer_lookup[*mix++];
}



int gomoku_sh_start(const struct MachineSound *msound)
{
	const char *mono_name = "gomoku";
	sound_channel *voice;

	/* get stream channels */
	stream = stream_init(mono_name, 100, samplerate, 0, gomoku_update_mono);

	/* allocate a pair of buffers to mix into - 1 second's worth should be more than enough */
	if ((mixer_buffer = auto_malloc(2 * sizeof(short) * samplerate)) == 0)
		return 1;
	mixer_buffer_2 = mixer_buffer + samplerate;

	/* build the mixer table */
	if (make_mixer_table(8, defgain))
		return 1;

	/* extract globals from the interface */
	num_voices = 4;
	last_channel = channel_list + num_voices;

	sound_rom = memory_region(REGION_SOUND1);
	sound_prom = memory_region(REGION_SOUND1);

	/* start with sound enabled, many games don't have a sound enable register */
	sound_enable = 1;

	/* reset all the voices */
	for (voice = channel_list; voice < last_channel; voice++)
	{
		voice->frequency = 0;
		voice->volume = 0;
		voice->wave = &sound_prom[0];
		voice->counter = 0;
	}

	return 0;
}


void gomoku_sh_stop(void)
{
}


/********************************************************************************/

WRITE8_HANDLER( gomoku_sound_w )
{
	sound_channel *voice;
	int base;

	/* update the streams */
	stream_update(stream, 0);

	/* set the register */
	gomoku_soundregs[offset] = data;

	/* recompute all the voice parameters */
	if (offset <= 0x1f)
	{
		for (base = 0, voice = channel_list; voice < last_channel; voice++, base += 8)
		{
			voice->frequency = gomoku_soundregs[0x02 + base] & 0x0f;
			voice->frequency = voice->frequency * 16 + ((gomoku_soundregs[0x01 + base]) & 0x0f);
			voice->frequency = voice->frequency * 16 + ((gomoku_soundregs[0x00 + base]) & 0x0f);

			voice->volume = gomoku_soundregs[0x806 + base] & 0x0f;
			if (gomoku_soundregs[0x800 + base] & 0xf0)
			{
				voice->wave = &sound_rom[128 * (16 * (gomoku_soundregs[0x805 + base] & 0x0f)
						+ (gomoku_soundregs[0x805 + base] & 0x0f))];
				voice->oneshot = 1;
			}
			else
			{
				voice->wave = &sound_rom[16 * (gomoku_soundregs[0x6 + base] & 0x0f)];
				voice->oneshot = 0;
			}
		}
	}
	else if (offset >= 0x800)
	{
		voice = &channel_list[(offset & 0x1f) / 8];
		if (voice->oneshot)
		{
			voice->counter = 0;
			voice->oneshotplaying = 1;
		}
	}
}
