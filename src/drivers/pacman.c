/***************************************************************************

	Namco PuckMan

    driver by Nicola Salmoria and many others

    Games supported:
		* PuckMan
		* Pac-Man Plus
		* Ms. Pac-Man
		* Crush Roller
		* Ponpoko
		* Eyes
		* Mr. TNT
		* Gorkans
		* Lizard Wizard
		* The Glob
		* Dream Shopper
		* Van Van Car
		* Ali Baba and 40 Thieves
		* Jump Shot
		* Shoot the Bull
		* Big Bucks
		* Driving Force
		* Eight Ball Action
		* Porky
		* MTV Rock-N-Roll Trivia (Part 2)

	Known issues:
		* mystery items in Ali Baba don't work correctly because of protection

****************************************************************************

	Pac-Man memory map (preliminary)

	0000-3fff ROM
	4000-43ff Video RAM
	4400-47ff Color RAM
	4800-4bff RAM (Dream Shopper only)
	4c00-4fff RAM
	8000-9fff ROM (Ms Pac-Man and Ponpoko only)
	a000-bfff ROM (Ponpoko only)

	memory mapped ports:

	read:
	5000      IN0
	5040      IN1
	5080      DSW 1
	50c0	  DSW 2 (Ponpoko only)
	see the input_ports definition below for details on the input bits

	write:
	4ff0-4fff 8 pairs of two bytes:
	          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
			  X flip (bit 1); the second byte the color
	5000      interrupt enable
	5001      sound enable
	5002      ????
	5003      flip screen
	5004      1 player start lamp
	5005      2 players start lamp
	5006      coin lockout
	5007      coin counter
	5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
	5045      sound voice 1 waveform (nibble)
	5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
	504a      sound voice 2 waveform (nibble)
	504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
	504f      sound voice 3 waveform (nibble)
	5050-5054 sound voice 1 frequency (nibbles)
	5055      sound voice 1 volume (nibble)
	5056-5059 sound voice 2 frequency (nibbles)
	505a      sound voice 2 volume (nibble)
	505b-505e sound voice 3 frequency (nibbles)
	505f      sound voice 3 volume (nibble)
	5060-506f Sprite coordinates, x/y pairs for 8 sprites
	50c0      Watchdog reset

	I/O ports:
	OUT on port $0 sets the interrupt vector


****************************************************************************

	Make Trax protection description:

	Make Trax has a "Special" chip that it uses for copy protection.
	The following chart shows when reads and writes may occur:

	AAAAAAAA AAAAAAAA
	11111100 00000000  <- address bits
	54321098 76543210
	xxx1xxxx 01xxxxxx - read data bits 4 and 7
	xxx1xxxx 10xxxxxx - read data bits 6 and 7
	xxx1xxxx 11xxxxxx - read data bits 0 through 5

	xxx1xxxx 00xxx100 - write to Special
	xxx1xxxx 00xxx101 - write to Special
	xxx1xxxx 00xxx110 - write to Special
	xxx1xxxx 00xxx111 - write to Special

	In practical terms, it reads from Special when it reads from
	location $5040-$50FF.  Note that these locations overlap our
	inputs and Dip Switches.  Yuk.

	I don't bother trapping the writes right now, because I don't
	know how to interpret them.  However, comparing against Crush
	Roller gives most of the values necessary on the reads.

	Instead of always reading from $5040, $5080, and $50C0, the Make
	Trax programmers chose to read from a wide variety of locations,
	probably to make debugging easier.  To us, it means that for the most
	part we can just assign a specific value to return for each address and
	we'll be OK.  This falls apart for the following addresses:  $50C0, $508E,
	$5090, and $5080.  These addresses should return multiple values.  The other
	ugly thing happening is in the ROMs at $3AE5.  It keeps checking for
	different values of $50C0 and $5080, and weird things happen if it gets
	the wrong values.  The only way I've found around these is to patch the
	ROMs using the same patches Crush Roller uses.  The only thing to watch
	with this is that changing the ROMs will break the beginning checksum.
	That's why we use the rom opcode decode function to do our patches.

	Incidentally, there are extremely few differences between Crush Roller
	and Make Trax.  About 98% of the differences appear to be either unused
	bytes, the name of the game, or code related to the protection.  I've
	only spotted two or three actual differences in the games, and they all
	seem minor.

	If anybody cares, here's a list of disassembled addresses for every
	read and write to the Special chip (not all of the reads are
	specifically for checking the Special bits, some are for checking
	player inputs and Dip Switches):

	Writes: $0084, $012F, $0178, $023C, $0C4C, $1426, $1802, $1817,
		$280C, $2C2E, $2E22, $3205, $3AB7, $3ACC, $3F3D, $3F40,
		$3F4E, $3F5E
	Reads:  $01C8, $01D2, $0260, $030E, $040E, $0416, $046E, $0474,
		$0560, $0568, $05B0, $05B8, $096D, $0972, $0981, $0C27,
		$0C2C, $0F0A, $10B8, $10BE, $111F, $1127, $1156, $115E,
		$11E3, $11E8, $18B7, $18BC, $18CA, $1973, $197A, $1BE7,
		$1C06, $1C9F, $1CAA, $1D79, $213D, $2142, $2389, $238F,
		$2AAE, $2BF4, $2E0A, $39D5, $39DA, $3AE2, $3AEA, $3EE0,
		$3EE9, $3F07, $3F0D

****************************************************************************

Todo: fix the sets according to this

Puckman is labeled wrong.  Puckman set 1 is
likely a bootleg since the protection is patched out, set 2 would likely
be correct if the roms were split differently.  Nicola had said that he
had a readme that mentioned 2k roms, which is my understanding.
Although the board will accept either, it is likely they were all 2k or
all 4k, not mixed.  Also the set labeled "harder?" is not:

Comparing files npacmod.6j and NAMCOPAC.6J

00000031: AF 25    ;3031 is sub for fail rom check.
00000032: C3 7C ;301c is sub for pass rom check
00000033: 1C E6    ;so it now clears the sum (reg A) and
00000034: 30 F0    ;jumps to pass if it fails rom check.

000007F8: 31 30  c 1981 / c 1980

0000080B: 40 4E  ghost / nickname
0000080C: 47 49
0000080D: 48 43
0000080E: 4F 4B
0000080F: 53 4E
00000810: 54 41
00000811: 40 4D
00000812: 40 45

00000FFF: 00 F1  checksum

Dave Widel

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "pacman.h"
#include "pengo.h"
#include "cpu/s2650/s2650.h"

static UINT8 speedcheat = 0;	/* a well known hack allows to make Pac Man run at four times */
								/* his usual speed. When we start the emulation, we check if the */
								/* hack can be applied, and set this flag accordingly. */



/*************************************
 *
 *	Machine init
 *
 *************************************/

MACHINE_INIT( pacman )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if ((RAM[0x180b] == 0xbe && RAM[0x1ffd] == 0x00) ||
			(RAM[0x180b] == 0x01 && RAM[0x1ffd] == 0xbd))
		speedcheat = 1;
	else
		speedcheat = 0;
}


MACHINE_INIT( pacplus )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if ((RAM[0x182d] == 0xbe && RAM[0x1ffd] == 0xff) ||
			(RAM[0x182d] == 0x01 && RAM[0x1ffd] == 0xbc))
		speedcheat = 1;
	else
		speedcheat = 0;
}


MACHINE_INIT( mschamp )
{
	data8_t *rom = memory_region(REGION_CPU1) + 0x10000;
	int bankaddr = ((readinputport(3) & 1) * 0x8000);

	cpu_setbank(1,&rom[bankaddr]);
	cpu_setbank(2,&rom[bankaddr+0x4000]);
}

MACHINE_INIT( piranha )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if ((RAM[0x180b] == 0xbe && RAM[0x1ffd] == 0x00) ||
			(RAM[0x180b] == 0x01 && RAM[0x1ffd] == 0xbd))
		speedcheat = 1;
	else
	speedcheat = 0;
}

/*************************************
 *
 *	Interrupts
 *
 *************************************/

static INTERRUPT_GEN( pacman_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputportbytag("FAKE") & 1)	/* check status of the fake dip switch */
		{
			/* activate the cheat */
			RAM[0x180b] = 0x01;
			RAM[0x1ffd] = 0xbd;
		}
		else
		{
			/* remove the cheat */
			RAM[0x180b] = 0xbe;
			RAM[0x1ffd] = 0x00;
		}
	}

	irq0_line_hold();
}


static INTERRUPT_GEN( pacplus_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputportbytag("FAKE") & 1)	/* check status of the fake dip switch */
		{
			/* activate the cheat */
			RAM[0x182d] = 0x01;
			RAM[0x1ffd] = 0xbc;
		}
		else
		{
			/* remove the cheat */
			RAM[0x182d] = 0xbe;
			RAM[0x1ffd] = 0xff;
		}
	}

	irq0_line_hold();
}


static INTERRUPT_GEN( mspacman_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputportbytag("FAKE") & 1)	/* check status of the fake dip switch */
		{
			/* activate the cheat */
			RAM[0x1180b] = 0x01;
			RAM[0x11ffd] = 0xbd;
		}
		else
		{
			/* remove the cheat */
			RAM[0x1180b] = 0xbe;
			RAM[0x11ffd] = 0x00;
		}
	}

	irq0_line_hold();
}

/*
   The piranha board has a sync bus controler card similar to Midway's pacman. It
   stores the LSB of the interupt vector using port 00 but it alters the byte to prevent
   it from running on normal pacman hardware and vice versa. I wrote a program to print
   out the even numbers and the vectors they convert to.  Thanks to Dave France for
   burning the roms.  The numbers that didn't print here convert to odd numbers.  It's
   slightly possible some numbers listed converted to odd numbers and coincidentally
   printed a valid even number.  Since it only uses 2 vectors($fa,$fc) I didn't complete
   the table or attempt to find the algorythm.

   David Widel
   d_widel@hotmail.com
  out vec  out vec  out vec  out vec
  c0 44    80 04    40 44    00 04
  c2 40    82 00    42 C4    02 84
  c4 C4    84 84    44 C4    04 00
  c6 40    86 00
  c8 4C    88 0C    48 4C    08 0C
  ca 48    8A 08    4A CC    0A 8C
  cc CC    8C 8C    4C 48    0C 08
  ce 48    8E 08
  d0 54    90 14    50 54    10 14
  d2 50    92 10    52 D4    12 94
  d4 D4    94 94    54 50    14 10
  d6 50    96 10
  d8 5C    98 1C    58 5C    18 1C
  da 58    9A 18    5A DC    1A 9C
  dc DC    9C 9C    5C 58    1C 18
  de 58    9E 18
  e0 64    a0 24    60 64    20 24
  e2 60    a2 20    62 E4    22 A4
  e4 E4    a4 A4    64 60    24 20
  e6 60    a6 20
  e8 6C    a8 2C    68 6C    28 2C
  ea 68    aA 28    6A EC    2A AC
  ec EC    aC AC    6C 68    2C 28
  ee 68    aE 28
  f0 74    b0 34    70 74    30 34
  f2 70    b2 30    72 F4    32 84
  f4 F4    b4 B4    74 70    34 30
  f6 70    b6 30
  f8 7C    b8 3C    78 7C    38 3C
  fa 78    bA 38    7A FC    3A BC
  fc FC    bC BC    7C 78    3C 38
  fe 78    bE 38


Naughty Mouse uses the same board as Piranha with a different pal to encrypt the vectors.
Incidentally we don't know the actual name of this game.  Other than the word naughty at
the top of the playfield there's no name.  It shares some character data with the missing
game Woodpecker, they may be related.

I haven't examined the code thoroughly but what I
did look at(sprite buffer), was copied from Pacman.  The addresses for the variables seem
to be the same as well.
*/

static WRITE8_HANDLER( piranha_interrupt_vector_w)
{
	if (data==0xfa) data=0x78;
	if (data==0xfc) data=0xfc;
	cpunum_set_input_line_vector( 0, 0, data );
}


static WRITE8_HANDLER( nmouse_interrupt_vector_w)
{
	if (data==0xbf) data=0x3c;
	if (data==0xc6) data=0x40;
	if (data==0xfc) data=0xfc;
	cpunum_set_input_line_vector( 0, 0, data );
}


/*************************************
 *
 *	LEDs/coin counters
 *
 *************************************/

static WRITE8_HANDLER( pacman_leds_w )
{
	set_led_status(offset,data & 1);
}


static WRITE8_HANDLER( pacman_coin_counter_w )
{
	coin_counter_w(offset,data & 1);
}


static WRITE8_HANDLER( pacman_coin_lockout_global_w )
{
	coin_lockout_global_w(~data & 0x01);
}


/*************************************
 *
 *	Ali Baba sound
 *
 *************************************/

static WRITE8_HANDLER( alibaba_sound_w )
{
	/* since the sound region in Ali Baba is not contiguous, translate the
	   offset into the 0-0x1f range */
 	if (offset < 0x10)
		pengo_sound_w(offset, data);
	else if (offset < 0x20)
		spriteram_2[offset - 0x10] = data;
	else
		pengo_sound_w(offset - 0x10, data);
}


static READ8_HANDLER( alibaba_mystery_1_r )
{
	// The return value determines what the mystery item is.  Each bit corresponds
	// to a question mark

	return rand() & 0x0f;
}


static READ8_HANDLER( alibaba_mystery_2_r )
{
	static int mystery = 0;

	// The single bit return value determines when the mystery is lit up.
	// This is certainly wrong

	mystery++;
	return (mystery >> 10) & 1;
}



/*************************************
 *
 *	Make Trax input handlers
 *
 *************************************/

static READ8_HANDLER( maketrax_special_port2_r )
{
	int data = input_port_2_r(offset);
	int pc = activecpu_get_previouspc();

	if ((pc == 0x1973) || (pc == 0x2389)) return data | 0x40;

	switch (offset)
	{
		case 0x01:
		case 0x04:
			data |= 0x40; break;
		case 0x05:
			data |= 0xc0; break;
		default:
			data &= 0x3f; break;
	}

	return data;
}


static READ8_HANDLER( maketrax_special_port3_r )
{
	int pc = activecpu_get_previouspc();

	if (pc == 0x040e) return 0x20;

	if ((pc == 0x115e) || (pc == 0x3ae2)) return 0x00;

	switch (offset)
	{
		case 0x00:
			return 0x1f;
		case 0x09:
			return 0x30;
		case 0x0c:
			return 0x00;
		default:
			return 0x20;
	}
}

static READ8_HANDLER( korosuke_special_port2_r )
{
	int data = input_port_2_r(offset);
	int pc = activecpu_get_previouspc();

	if ((pc == 0x196e) || (pc == 0x2387)) return data | 0x40;

	switch (offset)
	{
		case 0x01:
		case 0x04:
			data |= 0x40; break;
		case 0x05:
			data |= 0xc0; break;
		default:
			data &= 0x3f; break;
	}

	return data;
}

static READ8_HANDLER( korosuke_special_port3_r )
{
	int pc = activecpu_get_previouspc();

	if (pc == 0x0445) return 0x20;

	if ((pc == 0x115b) || (pc == 0x3ae6)) return 0x00;

	switch (offset)
	{
		case 0x00:
			return 0x1f;
		case 0x09:
			return 0x30;
		case 0x0c:
			return 0x00;
		default:
			return 0x20;
	}
}

/*************************************
 *
 *	Zola kludge
 *
 *************************************/

static READ8_HANDLER( mschamp_kludge_r )
{
	static UINT8 counter;
	return counter++;
}

/************************************
 *
 *	Big Bucks questions roms handlers
 *
 ************************************/

static int bigbucks_bank = 0;

static WRITE8_HANDLER( bigbucks_bank_w )
{
	bigbucks_bank = data;
}

static READ8_HANDLER( bigbucks_question_r )
{

	UINT8 *question = memory_region(REGION_USER1);
	UINT8 ret;

	ret = question[(bigbucks_bank << 16) | (offset ^ 0xffff)];

	return ret;
}

/************************************
 *
 *	S2650 cpu based games
 *
 ************************************/

static INTERRUPT_GEN( s2650_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x03);
}

static READ8_HANDLER( s2650_mirror_r )
{
	return program_read_byte(0x1000 + offset);
}

static WRITE8_HANDLER( s2650_mirror_w )
{
	program_write_byte(0x1000 + offset, data);
}

static READ8_HANDLER( drivfrcp_port1_r )
{
	switch (activecpu_get_pc())
	{
		case 0x0030:
		case 0x0291:
			return 0x01;
	}

    return 0;
}

static READ8_HANDLER( _8bpm_port1_r )
{
	switch (activecpu_get_pc())
	{
		case 0x0030:
		case 0x0466:
			return 0x01;
	}

    return 0;
}

static READ8_HANDLER( porky_port1_r )
{
	switch (activecpu_get_pc())
	{
		case 0x0034:
			return 0x01;
	}

    return 0;
}


/************************************
 *
 *	Rock-N-Roll Trivia (Part 2)
 *  questions roms and protection
 *	handlers
 *
 ************************************/

static data8_t *rocktrv2_prot_data, rocktrv2_question_bank = 0;

static READ8_HANDLER( rocktrv2_prot1_data_r )
{
	return rocktrv2_prot_data[0] >> 4;
}

static READ8_HANDLER( rocktrv2_prot2_data_r )
{
	return rocktrv2_prot_data[1] >> 4;
}

static READ8_HANDLER( rocktrv2_prot3_data_r )
{
	return rocktrv2_prot_data[2] >> 4;
}

static READ8_HANDLER( rocktrv2_prot4_data_r )
{
	return rocktrv2_prot_data[3] >> 4;
}

static WRITE8_HANDLER( rocktrv2_prot_data_w )
{
	rocktrv2_prot_data[offset] = data;
}

static WRITE8_HANDLER( rocktrv2_question_bank_w )
{
	rocktrv2_question_bank = data;
}

static READ8_HANDLER( rocktrv2_question_r )
{
	UINT8 *question = memory_region(REGION_USER1);

	return question[offset | (rocktrv2_question_bank * 0x8000)];
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)	/* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)	/* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50ff) AM_READ(input_port_3_r)	/* DSW2 */
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_ROM)	/* Ms. Pac-Man / Ponpoko only */
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4c00, 0x4fef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4ff0, 0x4fff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(pengo_sound_enable_w)
	AM_RANGE(0x5002, 0x5002) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pengo_flipscreen_w)
 	AM_RANGE(0x5004, 0x5005) AM_WRITE(pacman_leds_w)
// 	AM_RANGE(0x5006, 0x5006) AM_WRITE(pacman_coin_lockout_global_w)	this breaks many games
 	AM_RANGE(0x5007, 0x5007) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(pengo_sound_w) AM_BASE(&pengo_soundregs)
	AM_RANGE(0x5060, 0x506f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_ROM)	/* Ms. Pac-Man / Ponpoko only */
	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(videoram_w) /* mirror address for video ram, */
	AM_RANGE(0xc400, 0xc7ef) AM_WRITE(colorram_w) /* used to display HIGH SCORE and CREDITS */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(MWA8_NOP)	/* Eyes writes to this location to simplify code */
ADDRESS_MAP_END


static ADDRESS_MAP_START( mschamp_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK1)		/* By Sil: Zola/Ms. Champ */
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)		/* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)		/* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK2)		/* By Sil: Zola/Ms. Champ */
ADDRESS_MAP_END


static ADDRESS_MAP_START( mspacman_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)	/* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)	/* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50ff) AM_READ(input_port_3_r)	/* DSW2 */
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mspacman_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_BANK1)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4c00, 0x4fef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4ff0, 0x4fff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(pengo_sound_enable_w)
	AM_RANGE(0x5002, 0x5002) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pengo_flipscreen_w)
 	AM_RANGE(0x5004, 0x5005) AM_WRITE(pacman_leds_w)
	AM_RANGE(0x5006, 0x5006) AM_WRITE(mspacman_activate_rom)	/* Not actually, just handy */
// 	AM_RANGE(0x5006, 0x5006) AM_WRITE(pacman_coin_lockout_global_w)	this breaks many games
 	AM_RANGE(0x5007, 0x5007) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(pengo_sound_w) AM_BASE(&pengo_soundregs)
	AM_RANGE(0x5060, 0x506f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_BANK1)	/* Ms. Pac-Man / Ponpoko only */
	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(videoram_w) /* mirror address for video ram, */
	AM_RANGE(0xc400, 0xc7ef) AM_WRITE(colorram_w) /* used to display HIGH SCORE and CREDITS */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(MWA8_NOP)	/* Eyes writes to this location to simplify code */
ADDRESS_MAP_END


static ADDRESS_MAP_START( alibaba_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)	/* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)	/* including sprite codes at 4ef0-4eff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50c0) AM_READ(alibaba_mystery_1_r)
	AM_RANGE(0x50c1, 0x50c1) AM_READ(alibaba_mystery_2_r)
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x9000, 0x93ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xa7ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( alibaba_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4ef0, 0x4eff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(watchdog_reset_w)
 	AM_RANGE(0x5004, 0x5005) AM_WRITE(pacman_leds_w)
 	AM_RANGE(0x5006, 0x5006) AM_WRITE(pacman_coin_lockout_global_w)
 	AM_RANGE(0x5007, 0x5007) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x5040, 0x506f) AM_WRITE(alibaba_sound_w) AM_BASE(&pengo_soundregs)  /* the sound region is not contiguous */
	AM_RANGE(0x5060, 0x506f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2) /* actually at 5050-505f, here to point to free RAM */
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(pengo_sound_enable_w)
	AM_RANGE(0x50c1, 0x50c1) AM_WRITE(pengo_flipscreen_w)
	AM_RANGE(0x50c2, 0x50c2) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x9000, 0x93ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(videoram_w) /* mirror address for video ram, */
	AM_RANGE(0xc400, 0xc7ef) AM_WRITE(colorram_w) /* used to display HIGH SCORE and CREDITS */
ADDRESS_MAP_END


static ADDRESS_MAP_START( theglobp_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)	/* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)	/* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50ff) AM_READ(input_port_3_r)	/* DSW2 */
ADDRESS_MAP_END


static ADDRESS_MAP_START( vanvan_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM)	/* video and color RAM */
	AM_RANGE(0x4800, 0x4fff) AM_READ(MRA8_RAM)	/* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x5000) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x5040) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x5080) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50c0) AM_READ(input_port_3_r)	/* DSW2 */
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( vanvan_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4800, 0x4fef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4ff0, 0x4fff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(vanvan_bgcolor_w)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pengo_flipscreen_w)
	AM_RANGE(0x5005, 0x5006) AM_WRITE(MWA8_NOP)	/* always written together with 5001 */
 	AM_RANGE(0x5007, 0x5007) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x5060, 0x506f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0x5080, 0x5080) AM_WRITE(MWA8_NOP)	/* ??? toggled before reading 5000 */
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xb800, 0xb87f) AM_WRITE(MWA8_NOP)	/* probably a leftover from development: the Sanritsu version */
									/* writes the color lookup table here, while the Karateko version */
									/* writes garbage. */
ADDRESS_MAP_END


static ADDRESS_MAP_START( acitya_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0x47ff) AM_READ(MRA8_RAM) /* video and color RAM */
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM) /* including sprite codes at 4ff0-4fff */
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r) /* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r) /* IN1 */
	AM_RANGE(0x5080, 0x50bf) AM_READ(input_port_2_r) /* DSW1 */
	AM_RANGE(0x50c0, 0x50ff) AM_READ(input_port_3_r) /* DSW2 */
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_ROM) /* Ms. Pac-Man / Ponpoko only */
ADDRESS_MAP_END


static ADDRESS_MAP_START( bigbucks_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4c00, 0x4fbf) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(pengo_sound_enable_w)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pengo_flipscreen_w)
	AM_RANGE(0x5007, 0x5007) AM_WRITE(MWA8_NOP) //?
	AM_RANGE(0x5040, 0x505f) AM_WRITE(pengo_sound_w) AM_BASE(&pengo_soundregs)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x5100, 0x5100) AM_WRITE(MWA8_NOP) //?
	AM_RANGE(0x6000, 0x6000) AM_WRITE(bigbucks_bank_w)
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( s2650games_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1500, 0x1500) AM_READ(input_port_0_r)
	AM_RANGE(0x1540, 0x1540) AM_READ(input_port_1_r)
	AM_RANGE(0x1580, 0x1580) AM_READ(input_port_2_r)
	AM_RANGE(0x1c00, 0x1fef) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_READ(s2650_mirror_r)
	AM_RANGE(0x4000, 0x4fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_READ(s2650_mirror_r)
	AM_RANGE(0x6000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_READ(s2650_mirror_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( s2650games_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1000, 0x13ff) AM_WRITE(s2650games_colorram_w)
	AM_RANGE(0x1400, 0x141f) AM_WRITE(s2650games_scroll_w)
	AM_RANGE(0x1420, 0x148f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1490, 0x149f) AM_WRITE(MWA8_RAM) AM_BASE(&sprite_bank)
	AM_RANGE(0x14a0, 0x14bf) AM_WRITE(s2650games_tilesbank_w) AM_BASE(&tiles_bankram)
	AM_RANGE(0x14c0, 0x14ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1500, 0x1502) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x1503, 0x1503) AM_WRITE(s2650games_flipscreen_w)
	AM_RANGE(0x1504, 0x1506) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x1507, 0x1507) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x1508, 0x155f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1560, 0x156f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2)
	AM_RANGE(0x1570, 0x157f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1586, 0x1587) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x15c0, 0x15c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x15c7, 0x15c7) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1bff) AM_WRITE(s2650games_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1c00, 0x1fef) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1ff0, 0x1fff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x2000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x3fff) AM_WRITE(s2650_mirror_w)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE(s2650_mirror_w)
	AM_RANGE(0x6000, 0x6fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7000, 0x7fff) AM_WRITE(s2650_mirror_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( rocktrv2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_READWRITE(MRA8_RAM, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4400, 0x47ff) AM_READWRITE(MRA8_RAM, colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4c00, 0x4fff) AM_RAM
	AM_RANGE(0x5000, 0x5000) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x5040, 0x507f) AM_READ(input_port_1_r)	/* IN1 */
	AM_RANGE(0x5080, 0x5080) AM_READ(input_port_2_r)	/* DSW1 */
	AM_RANGE(0x50c0, 0x50c0) AM_READ(input_port_3_r)	/* DSW2 */
	AM_RANGE(0x5000, 0x5000) AM_WRITE(interrupt_enable_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(pengo_sound_enable_w)
	AM_RANGE(0x5003, 0x5003) AM_WRITE(pengo_flipscreen_w)
	AM_RANGE(0x5007, 0x5007) AM_WRITE(pacman_coin_counter_w)
	AM_RANGE(0x5040, 0x505f) AM_WRITE(pengo_sound_w) AM_BASE(&pengo_soundregs)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x5fe0, 0x5fe3) AM_WRITE(rocktrv2_prot_data_w) AM_BASE(&rocktrv2_prot_data)
	AM_RANGE(0x5fe0, 0x5fe0) AM_READ(rocktrv2_prot1_data_r)
	AM_RANGE(0x5fe4, 0x5fe4) AM_READ(rocktrv2_prot2_data_r)
	AM_RANGE(0x5fe8, 0x5fe8) AM_READ(rocktrv2_prot3_data_r)
	AM_RANGE(0x5fec, 0x5fec) AM_READ(rocktrv2_prot4_data_r)
	AM_RANGE(0x5ff0, 0x5ff0) AM_WRITE(rocktrv2_question_bank_w)
	AM_RANGE(0x5fff, 0x5fff) AM_READ(input_port_3_r) /* DSW2 mirrored */
	AM_RANGE(0x6000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_READ(rocktrv2_question_r)
ADDRESS_MAP_END

/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(interrupt_vector_w)	/* Pac-Man only */
ADDRESS_MAP_END

static ADDRESS_MAP_START( vanvan_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(SN76496_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(SN76496_1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dremshpr_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x06, 0x06) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( piranha_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(piranha_interrupt_vector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( nmouse_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(nmouse_interrupt_vector_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( theglobp_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(theglobp_decrypt_rom)	/* Switch protection logic */
ADDRESS_MAP_END

static ADDRESS_MAP_START( acitya_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(acitya_decrypt_rom) /* Switch protection logic */
ADDRESS_MAP_END

static ADDRESS_MAP_START( mschamp_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(mschamp_kludge_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bigbucks_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READ(bigbucks_question_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( drivfrcp_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(MRA8_NOP)
	AM_RANGE(0x01, 0x01) AM_READ(drivfrcp_port1_r)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( _8bpm_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(MRA8_NOP)
	AM_RANGE(0x01, 0x01) AM_READ(_8bpm_port1_r)
	AM_RANGE(0xe0, 0xe0) AM_READ(MRA8_NOP)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( porky_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(porky_port1_r)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( s2650games_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(S2650_DATA_PORT, S2650_DATA_PORT) AM_WRITE(SN76496_0_w)
ADDRESS_MAP_END


/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( pacman )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(1) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(1) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(1) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(1) PORT_4WAY
	PORT_DIPNAME(0x10, 0x10, "Rack Test" )	PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(2) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(2) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Ghost Names" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Alternate" )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("FAKE")
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_DIPNAME(0x01, 0x00, "Speedup Cheat" ) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON1)
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END


/* Ms. Pac-Man input ports are identical to Pac-Man, the only difference is */
/* the missing Ghost Names dip switch. */
INPUT_PORTS_START( mspacman )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(    0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("FAKE")
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BIT(    0x01, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Speedup Cheat") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON1) PORT_CODE(JOYCODE_MOUSE_1_BUTTON1) PORT_CHEAT
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END


/* Same as 'mspacman', but no fake input port */
INPUT_PORTS_START( mspacpls )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(    0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )	// Also invincibility when playing
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )	// Also speed-up when playing
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( mschamp )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(    0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x01, 0x01, "Game" )
	PORT_DIPSETTING(    0x01, "Champion Edition" )
	PORT_DIPSETTING(    0x00, "Super Zola Pac Gal" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( maketrax )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, "First Pattern" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Teleport Holes" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( korosuke )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, "First Pattern" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Teleport Holes" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
 	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection */

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( mbrush )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( paintrlr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )  /* Protection in Make Trax */

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( ponpoko )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	/* The 2nd player controls are used even in upright mode */
	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x0f, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, "A 3/1 B 3/1" )
	PORT_DIPSETTING(    0x0e, "A 3/1 B 1/2" )
	PORT_DIPSETTING(    0x0f, "A 3/1 B 1/4" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x0d, "A 2/1 B 1/1" )
	PORT_DIPSETTING(    0x07, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x0b, "A 2/1 B 1/5" )
	PORT_DIPSETTING(    0x0c, "A 2/1 B 1/6" )
	PORT_DIPSETTING(    0x01, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 4/5" )
	PORT_DIPSETTING(    0x05, "A 1/1 B 2/3" )
	PORT_DIPSETTING(    0x0a, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x08, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x09, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x03, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )  /* Most likely unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )  /* Most likely unused */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )  /* Most likely unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( eyes )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "50000" )
	PORT_DIPSETTING(    0x20, "75000" )
	PORT_DIPSETTING(    0x10, "100000" )
	PORT_DIPSETTING(    0x00, "125000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )  /* Not accessed */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( mrtnt )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "75000" )
	PORT_DIPSETTING(    0x20, "100000" )
	PORT_DIPSETTING(    0x10, "125000" )
	PORT_DIPSETTING(    0x00, "150000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( lizwiz )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "75000" )
	PORT_DIPSETTING(    0x20, "100000" )
	PORT_DIPSETTING(    0x10, "125000" )
	PORT_DIPSETTING(    0x00, "150000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( theglobp )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x1c, "Easiest" )
	PORT_DIPSETTING(    0x18, "Very Easy" )
	PORT_DIPSETTING(    0x14, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x0c, "Difficult" )
	PORT_DIPSETTING(    0x08, "Very Difficult" )
	PORT_DIPSETTING(    0x04, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( vanvan )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20k and 100k" )
	PORT_DIPSETTING(    0x04, "40k and 140k" )
	PORT_DIPSETTING(    0x00, "70k and 200k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )

	/* When all DSW2 are ON, there is no sprite collision detection */
	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT(    0x02, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Invulnerability") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// Missile effect
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )		// Killer car is destroyed
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )			// Killer car is not destroyed
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( vanvank )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20k and 100k" )
	PORT_DIPSETTING(    0x04, "40k and 140k" )
	PORT_DIPSETTING(    0x00, "70k and 200k" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )

	/* When all DSW2 are ON, there is no sprite collision detection */
	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_BIT(    0x02, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Invulnerability") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		// Missile effect
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )		// Killer car is destroyed
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )			// Killer car is not destroyed
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( dremshpr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x00, "70000" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )

	PORT_START_TAG("DSW 2")
  //PORT_BIT(    0x01, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Invulnerability") PORT_CHEAT
  //PORT_DIPSETTING(    0x00, DEF_STR( Off ) )		/* turning this on crashes puts the */
  //PORT_DIPSETTING(    0x01, DEF_STR( On ) )       /* emulated machine in an infinite loop once in a while */
//	PORT_DIPNAME( 0xff, 0x00, DEF_STR( Unused ) )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( alibaba )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( jumpshot )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1  ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, "Time"  )
//	PORT_DIPSETTING(    0x00,  "2 Minutes"  )
	PORT_DIPSETTING(    0x02,  "2 Minutes" )
	PORT_DIPSETTING(    0x03,  "3 Minutes" )
	PORT_DIPSETTING(    0x01,  "4 Minutes"  )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "2 Players Game" )
	PORT_DIPSETTING(    0x20, "1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Credits" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



INPUT_PORTS_START( shootbul )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0f, 0x0f, IPT_TRACKBALL_X  ) PORT_MINMAX(0,0) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0f, 0x0f, IPT_TRACKBALL_Y ) PORT_MINMAX(0,0) PORT_SENSITIVITY(50) PORT_KEYDELTA(25) PORT_REVERSE
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x07, 0x07, "Time"  )
	PORT_DIPSETTING(    0x01, "Short")
	PORT_DIPSETTING(    0x07, "Average" )
	PORT_DIPSETTING(    0x03, "Long" )
	PORT_DIPSETTING(    0x05, "Longer" )
	PORT_DIPSETTING(    0x06, "Longest" )
	PORT_DIPNAME( 0x08, 0x08, "Title Page Sounds"  )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

// New Atlantic City Action / Board Walk Casino Inputs //
// Annoyingly enough, you can't get into service mode on bwcasino if the
// cocktail mode is set. To test player 2's inputs, select Upright Mode on
// the dipswitches, and enter test mode. Now select cocktail mode and you
// can test everything. Wierd.

INPUT_PORTS_START( bwcasino )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_COCKTAIL PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_COCKTAIL PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1e, 0x1e, "Hands per Game" )
	PORT_DIPSETTING(    0x1e, "3" )
	PORT_DIPSETTING(    0x1c, "4" )
	PORT_DIPSETTING(    0x1a, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x16, "7" )
	PORT_DIPSETTING(    0x14, "8" )
	PORT_DIPSETTING(    0x12, "9" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x0e, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0a, "13" )
	PORT_DIPSETTING(    0x08, "14" )
	PORT_DIPSETTING(    0x06, "15" )
	PORT_DIPSETTING(    0x04, "16" )
	PORT_DIPSETTING(    0x02, "17" )
	PORT_DIPSETTING(    0x00, "18" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

// ATLANTIC CITY ACTION (acitya)
// Unlike "Boardwalk Casino", "Atlantic City Action" does not appear to
// have a cocktail mode, and uses service button connected differently to
// "Boardwalk"
INPUT_PORTS_START( acitya )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1e, 0x1e, "Hands per Game" )
	PORT_DIPSETTING(    0x1e, "3" )
	PORT_DIPSETTING(    0x1c, "4" )
	PORT_DIPSETTING(    0x1a, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x16, "7" )
	PORT_DIPSETTING(    0x14, "8" )
	PORT_DIPSETTING(    0x12, "9" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x0e, "11" )
	PORT_DIPSETTING(    0x0c, "12" )
	PORT_DIPSETTING(    0x0a, "13" )
	PORT_DIPSETTING(    0x08, "14" )
	PORT_DIPSETTING(    0x06, "15" )
	PORT_DIPSETTING(    0x04, "16" )
	PORT_DIPSETTING(    0x02, "17" )
	PORT_DIPSETTING(    0x00, "18" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( nmouse )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT(    0x10, 0x10, IPT_DIPSWITCH_NAME ) PORT_NAME("Rack Test") PORT_CODE(KEYCODE_F1) PORT_CHEAT
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPSETTING(    0x20, "15000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END


INPUT_PORTS_START( bigbucks )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x10, "Enable Category Adult Affairs" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, "Time to bet / answer" )
	PORT_DIPSETTING(    0x00, "15 sec. / 10 sec." )
	PORT_DIPSETTING(    0x01, "20 sec. / 15 sec." )
	PORT_DIPNAME( 0x02, 0x00, "Continue if player busts" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Show correct answer" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( drivfrcp )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("Sense")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( 8bpm )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Start 1 / P1 Button 1")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Start 2 / P1 Button 1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("Sense")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( porky )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("Sense")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( rocktrv2 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x1c, 0x10, "Questions Per Game" )
	PORT_DIPSETTING(    0x1c, "2" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x14, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x04, "8" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x60, 0x60, "Clock Speed" )
	PORT_DIPSETTING(    0x60, "Beginner" )
	PORT_DIPSETTING(    0x40, "Intermed" )
	PORT_DIPSETTING(    0x20, "Pro" )
	PORT_DIPSETTING(    0x00, "Super - Pro" )
	PORT_DIPNAME( 0x80, 0x80,"Freeze Image" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x01, 0x01, "Mode" )
	PORT_DIPSETTING(    0x01, "Amusement" )
	PORT_DIPSETTING(    0x00, "Credit" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x04, "K.O. Switch" )
	PORT_DIPSETTING(    0x04, "Auto" )
	PORT_DIPSETTING(    0x00, "Manual" )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x70, "10000" )
	PORT_DIPSETTING(    0x60, "17500" )
	PORT_DIPSETTING(    0x50, "25000" )
	PORT_DIPSETTING(    0x40, "32500" )
	PORT_DIPSETTING(    0x30, "40000" )
	PORT_DIPSETTING(    0x20, "47500" )
	PORT_DIPSETTING(    0x10, "55000" )
	PORT_DIPSETTING(    0x00, "62500" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown )  )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
    2,  /* 2 bits per pixel */
    { 0, 4 },   /* the two bitplanes for 4 pixels are packed into one byte */
    { 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 }, /* bits are packed in groups of four */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo mschampgfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tilelayout,   0, 32 },
	{ REGION_GFX1, 0x1000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo s2650games_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tilelayout,      0, 32 },
    { REGION_GFX1, 0x0000, &spritelayout,    0, 32 },
    { -1 } /* end of array */
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	100,		/* playback volume */
	REGION_SOUND1	/* memory region */
};


static struct SN76496interface sn76496_interface =
{
	2,
	{ 1789750, 1789750 },	/* 1.78975 MHz ? */
	{ 75, 75 }
};


static struct AY8910interface dremshpr_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz ??? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct SN76496interface sn76489_interface =
{
	1,	/* 1 chip */
	{ 18432000/6/2/3 },	/* ? MHz */
	{ 100 }
};


/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( pacman )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(0,writeport)
	MDRV_CPU_VBLANK_INT(pacman_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(pacman)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(4*32)

	MDRV_PALETTE_INIT(pacman)
	MDRV_VIDEO_START(pacman)
	MDRV_VIDEO_UPDATE(pengo)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("namco", NAMCO, namco_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pacplus )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(pacplus_interrupt,1)

	MDRV_MACHINE_INIT(pacplus)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mspacman )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mspacman_readmem,mspacman_writemem)
	MDRV_CPU_VBLANK_INT(mspacman_interrupt,1)

	MDRV_MACHINE_INIT(mspacman)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mspacpls )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_INIT(NULL)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mschamp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mschamp_readmem,writemem)
	MDRV_CPU_IO_MAP(mschamp_readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_INIT(mschamp)

	/* video hardware */
	MDRV_GFXDECODE(mschampgfxdecodeinfo)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( theglobp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(theglobp_readmem,writemem)
	MDRV_CPU_IO_MAP(theglobp_readport,writeport)

	MDRV_MACHINE_INIT(theglobp)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vanvan )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(vanvan_readmem,vanvan_writemem)
	MDRV_CPU_IO_MAP(0,vanvan_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_MACHINE_INIT(NULL)

	/* video hardware */
	MDRV_VISIBLE_AREA(2*8, 34*8-1, 0*8, 28*8-1)
	MDRV_VIDEO_UPDATE(vanvan)

	/* sound hardware */
	MDRV_SOUND_REPLACE("namco", SN76496, sn76496_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dremshpr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(0,dremshpr_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_MACHINE_INIT(NULL)

	/* sound hardware */
	MDRV_SOUND_REPLACE("namco", AY8910, dremshpr_ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( alibaba )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(alibaba_readmem,alibaba_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_INIT(NULL)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( piranha )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(0,piranha_writeport)

	MDRV_MACHINE_INIT(piranha)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nmouse )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(0,nmouse_writeport)

	MDRV_MACHINE_INIT(NULL)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( acitya )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(acitya_readmem,writemem)
	MDRV_CPU_IO_MAP(acitya_readport,writeport)

	MDRV_MACHINE_INIT(acitya)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bigbucks )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_PROGRAM_MAP(readmem,bigbucks_writemem)
	MDRV_CPU_IO_MAP(bigbucks_readport,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,20)

	MDRV_MACHINE_INIT(NULL)

	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( s2650games )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_REMOVE("main")
	MDRV_CPU_ADD_TAG("main", S2650, 18432000/6/2/3)	/* ??? */
	MDRV_CPU_PROGRAM_MAP(s2650games_readmem,s2650games_writemem)
	MDRV_CPU_VBLANK_INT(s2650_interrupt,1)

	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(s2650games_gfxdecodeinfo)

	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_START(s2650games)
	MDRV_VIDEO_UPDATE(s2650games)

	/* sound hardware */
	MDRV_SOUND_REPLACE("namco", SN76496, sn76489_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( drivfrcp )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(s2650games)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(drivfrcp_readport,s2650games_writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 8bpm )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(s2650games)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(_8bpm_readport,s2650games_writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( porky )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(s2650games)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(porky_readport,s2650games_writeport)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rocktrv2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(pacman)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(rocktrv2_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_MACHINE_INIT(NULL)

	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( puckman )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3) SHA1(87117ba5082cd7a615b4ec7c02dd819003fbd669) )
	ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(39d1fc83) SHA1(326dbbf94c6fa2e96613dedb53702f8832b47d59) )
	ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(02083b03) SHA1(7e1945f6eb51f2e51806d0439f975f7a2889b9b8) )
	ROM_LOAD( "namcopac.6j",  0x3000, 0x1000, CRC(7a36fe55) SHA1(01b4c38108d9dc4e48da4f8d685248e1e6821377) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) SHA1(06ef227747a440831c9a3a613b76693d52a2f0a9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( puckmod )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3) SHA1(87117ba5082cd7a615b4ec7c02dd819003fbd669) )
	ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(39d1fc83) SHA1(326dbbf94c6fa2e96613dedb53702f8832b47d59) )
	ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(02083b03) SHA1(7e1945f6eb51f2e51806d0439f975f7a2889b9b8) )
	ROM_LOAD( "npacmod.6j",   0x3000, 0x1000, CRC(7d98d5f5) SHA1(39939bcd6fb785d0d06fd29f0287158ab1267dfc) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) SHA1(06ef227747a440831c9a3a613b76693d52a2f0a9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( puckmana )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) SHA1(e87e059c5be45753f7e9f33dff851f16d6751181) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) SHA1(8e47e8c2c4d6117d174cdac150392042d3e0a881) )
	ROM_LOAD( "prg7",         0x3000, 0x0800, CRC(b6289b26) SHA1(d249fa9cdde774d5fee7258147cd25fa3f4dc2b3) )
	ROM_LOAD( "prg8",         0x3800, 0x0800, CRC(17a88c13) SHA1(eb462de79f49b7aa8adb0cc6d31535b10550c0ce) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chg1",         0x0000, 0x0800, CRC(2066a0b7) SHA1(6d4ccc27d6be185589e08aa9f18702b679e49a4a) )
	ROM_LOAD( "chg2",         0x0800, 0x0800, CRC(3591b89d) SHA1(79bb456be6c39c1ccd7d077fbe181523131fb300) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( pacman )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) SHA1(e87e059c5be45753f7e9f33dff851f16d6751181) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) SHA1(8e47e8c2c4d6117d174cdac150392042d3e0a881) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) SHA1(d4a70d56bb01d27d094d73db8667ffb00ca69cb9) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) SHA1(06ef227747a440831c9a3a613b76693d52a2f0a9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( pacmod )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacmanh.6e",   0x0000, 0x1000, CRC(3b2ec270) SHA1(48fc607ad8d86249948aa377c677ae44bb8ad3da) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pacmanh.6h",   0x2000, 0x1000, CRC(18811780) SHA1(ab34acaa3dbcafe8b20c2197f36641e471984487) )
	ROM_LOAD( "pacmanh.6j",   0x3000, 0x1000, CRC(5c96a733) SHA1(22ae15a6f088e7296f77c7487a350c4bd102f00e) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacmanh.5e",   0x0000, 0x1000, CRC(299fb17a) SHA1(ad97adc2122482a9018bacd137df9d8f409ddf85) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( hangly )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) SHA1(d63eaebd85e10aa6c27bb7f47642dd403eeb6934) )
	ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) SHA1(cedddc5194589039dd8b64f07ab6320d7d4f55f9) )
	ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) SHA1(bd42e68b29b4d654dc817782ba00db69b7d2dfe2) )
	ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) SHA1(0a7ac0e59d4d26fe52a2f4196c9f19e5ab677c87) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) SHA1(06ef227747a440831c9a3a613b76693d52a2f0a9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( hangly2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) SHA1(d63eaebd85e10aa6c27bb7f47642dd403eeb6934) )
	ROM_LOAD( "hangly2.6f",   0x1000, 0x0800, CRC(5ba228bb) SHA1(b0e902cdf98bee72d6ec8069eec96adce3245074) )
	ROM_LOAD( "hangly2.6m",   0x1800, 0x0800, CRC(baf5461e) SHA1(754586a6449fd54a342f260e572c1cd60ab70815) )
	ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) SHA1(bd42e68b29b4d654dc817782ba00db69b7d2dfe2) )
	ROM_LOAD( "hangly2.6j",   0x3000, 0x0800, CRC(51305374) SHA1(6197b606a0eedb11135d9f4f7a89aecc23fb2d33) )
	ROM_LOAD( "hangly2.6p",   0x3800, 0x0800, CRC(427c9d4d) SHA1(917bc3d571cbdd24d88327ecabfb5b3f6d39af0a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacmanh.5e",   0x0000, 0x1000, CRC(299fb17a) SHA1(ad97adc2122482a9018bacd137df9d8f409ddf85) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( hangly3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hm1.6e",   0x0000, 0x0800, CRC(9d027c4a) SHA1(88e094880057451a75cdc2ce9477403021813982) )
	ROM_LOAD( "hm5.6k",	  0x0800, 0x0800, CRC(194c7189) SHA1(fd423bac2810015313841c7b935054565390fbd0) )
	ROM_LOAD( "hangly2.6f",   0x1000, 0x0800, CRC(5ba228bb) SHA1(b0e902cdf98bee72d6ec8069eec96adce3245074) ) // hm2.6f
	ROM_LOAD( "hangly2.6m",   0x1800, 0x0800, CRC(baf5461e) SHA1(754586a6449fd54a342f260e572c1cd60ab70815) ) // hm6.6m
	ROM_LOAD( "hm3.6h",   0x2000, 0x0800, CRC(08419c4a) SHA1(7e5001adad401080c788737c1d2349f218750442) )
	ROM_LOAD( "hm7.6n",   0x2800, 0x0800, CRC(ab74b51f) SHA1(1bce8933ed7807eb7aca9670df8994f8d1a8b5b7) )
	ROM_LOAD( "hm4.6j",   0x3000, 0x0800, CRC(5039b082) SHA1(086a6ac4742734167d283b1121fce29d8ac4a6cd) )
	ROM_LOAD( "hm8.6p",   0x3800, 0x0800, CRC(931770d7) SHA1(78fcf88e07ec5126c12c3297b62ca388809e947c) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hm9.5e", 	0x0000, 0x0800, CRC(5f4be3cc) SHA1(eeb0e1e44549b99eab481d9ac016b4359e19fe30) )
	ROM_LOAD( "hm11.5h",    0x0800, 0x0800, CRC(3591b89d) SHA1(79bb456be6c39c1ccd7d077fbe181523131fb300) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hm10.5f", 	0x0000, 0x0800, CRC(9e39323a) SHA1(be933e691df4dbe7d12123913c3b7b7b585b7a35) )
	ROM_LOAD( "hm12.5j", 	0x0800, 0x0800, CRC(1b1d9096) SHA1(53771c573051db43e7185b1d188533056290a620) )


	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( newpuckx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5) SHA1(1481a4f083b563350744f9d25b1bcd28073875d6) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8) SHA1(119aab12a9e1052c7b9a1f81e563740b41429a8c) )
	ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(2e64a3ba) SHA1(f86a921173f32211b18d023c2701664d13ae23be) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) SHA1(06ef227747a440831c9a3a613b76693d52a2f0a9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) SHA1(4a937ac02216ea8c96477d4a15522070507fb599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( pacheart )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.6e",         0x0000, 0x0800, CRC(d844b679) SHA1(c4486198b3126bb8e05a308c53787e51065f77ae) )
	ROM_LOAD( "pacheart.pg2", 0x0800, 0x0800, CRC(b9152a38) SHA1(b6be2cb6bc7dd123503eb6bf1165dd1c99456813) )
	ROM_LOAD( "2.6f",         0x1000, 0x0800, CRC(7d177853) SHA1(9b5ddaaa8b564654f97af193dbcc29f81f230a25) )
	ROM_LOAD( "pacheart.pg4", 0x1800, 0x0800, CRC(842d6574) SHA1(40e32d09cc8d701eb318716493a68cf3f95d3d6d) )
	ROM_LOAD( "3.6h",         0x2000, 0x0800, CRC(9045a44c) SHA1(a97d7016effbd2ace9a7d92ceb04a6ce18fb42f9) )
	ROM_LOAD( "7.6n",         0x2800, 0x0800, CRC(888f3c3e) SHA1(c2b5917bf13071131dd53ea76f0da86706db2d80) )
	ROM_LOAD( "pacheart.pg7", 0x3000, 0x0800, CRC(f5265c10) SHA1(9a320790d7a03fd6192a92d30b3e9c754bbc6a9d) )
	ROM_LOAD( "pacheart.pg8", 0x3800, 0x0800, CRC(1a21a381) SHA1(d5367a327d19fb57ba5e484bd4fda1b10953c040) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacheart.ch1", 0x0000, 0x0800, CRC(c62bbabf) SHA1(f6f28ae33c2ab274105283b22b49ad243780a95e) )
	ROM_LOAD( "chg2",         0x0800, 0x0800, CRC(3591b89d) SHA1(79bb456be6c39c1ccd7d077fbe181523131fb300) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacheart.ch3", 0x0000, 0x0800, CRC(ca8c184c) SHA1(833aa845824ed80777b62f03df36a920ad7c3656) )
	ROM_LOAD( "pacheart.ch4", 0x0800, 0x0800, CRC(1b1d9096) SHA1(53771c573051db43e7185b1d188533056290a620) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END


ROM_START( joyman )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1.6e",         0x0000, 0x0800, CRC(d844b679) SHA1(c4486198b3126bb8e05a308c53787e51065f77ae) )
	ROM_LOAD( "5.6k",         0x0800, 0x0800, CRC(ab9c8f29) SHA1(3753b8609c30d85d89acf745cf9303b77be440fd) )
	ROM_LOAD( "2.6f",         0x1000, 0x0800, CRC(7d177853) SHA1(9b5ddaaa8b564654f97af193dbcc29f81f230a25) )
	ROM_LOAD( "6.6m",         0x1800, 0x0800, CRC(b3c8d32e) SHA1(8b336fca1300820308cd5c4efc60bf2ba4199302) )
	ROM_LOAD( "3.6h",         0x2000, 0x0800, CRC(9045a44c) SHA1(a97d7016effbd2ace9a7d92ceb04a6ce18fb42f9) )
	ROM_LOAD( "7.6n",         0x2800, 0x0800, CRC(888f3c3e) SHA1(c2b5917bf13071131dd53ea76f0da86706db2d80) )
	ROM_LOAD( "4.6j",         0x3000, 0x0800, CRC(00b553f8) SHA1(57f2e4a6da9f00935fead447b2123a8b95e5d672) )
	ROM_LOAD( "8.6p",         0x3800, 0x0800, CRC(5d5ce992) SHA1(ced7ed39cfc7ec7b2c0459e275577976109ee82f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9.5e",  0x0000, 0x0800, CRC(39b557bc) SHA1(0f602ec84cb25fced89699e430b95b5ae93c83bd) )
	ROM_LOAD( "11.5h", 0x0800, 0x0800, CRC(33e0289e) SHA1(c1b910bdc61e560a8c34298deb11401f718e7330) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10.5f", 0x0000, 0x0800, CRC(338771a6) SHA1(7cd68cc428986255d0de29aae894900519e7fda5) )
	ROM_LOAD( "12.5j", 0x0800, 0x0800, CRC(f4f0add5) SHA1(d71c54ef55a755ec1316623d183b4f615ef7c055) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END


ROM_START( piranha )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "pir1.bin", 0x0000, 0x0800, CRC(69a3e6ea) SHA1(c54e5d039a03d3cbee7a5e21bf1e23f4fd913ea6) )
	ROM_LOAD( "pir5.bin", 0x0800, 0x0800, CRC(245e753f) SHA1(4c1183b8449e4e7995f81079953fe0e251251c60) )
	ROM_LOAD( "pir2.bin", 0x1000, 0x0800, CRC(62cb6954) SHA1(0e01c8463b130ab5518ce23368ad028c86cd0a32) )
	ROM_LOAD( "pir6.bin", 0x1800, 0x0800, CRC(cb0700bc) SHA1(1f5e91791ea25eb58d26b9627e98e0b6c1d9becf) )
	ROM_LOAD( "pir3.bin", 0x2000, 0x0800, CRC(843fbfe5) SHA1(6671a3c55ef70447f2a127438e0c39857f8bf6b1) )
	ROM_LOAD( "pir7.bin", 0x2800, 0x0800, CRC(73084d5e) SHA1(cb04a4c9dbf1672ddf478d2fe92b0ffd0159bb9e) )
	ROM_LOAD( "pir4.bin", 0x3000, 0x0800, CRC(4cdf6704) SHA1(97af8bbd08896dffd73e359ec46843dd673c4c9c) )
	ROM_LOAD( "pir8.bin", 0x3800, 0x0800, CRC(b86fedb3) SHA1(f5eaf7ccc1ecaa2417bcc077561efca8e7cb691a) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE)
	ROM_LOAD( "pir9.bin", 0x0000, 0x0800, CRC(0f19eb28) SHA1(0335189a06be01b97ca376d3682ed54df9b121e8) )
	ROM_LOAD( "pir11.bin", 0x0800, 0x0800, CRC(5f8bdabe) SHA1(eb6a0515a381a885b087d165aaefb0277a223715) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pir10.bin", 0x0000, 0x0800, CRC(d19399fb) SHA1(c0a75a08f77adb9d0010511c4b6ea99324c33c50) )
	ROM_LOAD( "pir12.bin", 0x0800, 0x0800, CRC(cfb4403d) SHA1(1642a4917be0621ebf5f705c7f68a2b75d1c78d3) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "piranha.4a", 0x0020, 0x0100, CRC(08c9447b) SHA1(5e4fbfcc7179fc4b1436af9bb709ffc381479315) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( piranhao )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "p1.bin", 0x0000, 0x0800, CRC(c6ce1bfc) SHA1(da145d67331cee292654a185fb09e773dd9d40cd) )
	ROM_LOAD( "p5.bin", 0x0800, 0x0800, CRC(a2655a33) SHA1(2253dcf5c8cbe278118aa1569cf456b13d8cf029) )
	ROM_LOAD( "pir2.bin", 0x1000, 0x0800, CRC(62cb6954) SHA1(0e01c8463b130ab5518ce23368ad028c86cd0a32) )
	ROM_LOAD( "pir6.bin", 0x1800, 0x0800, CRC(cb0700bc) SHA1(1f5e91791ea25eb58d26b9627e98e0b6c1d9becf) )
	ROM_LOAD( "pir3.bin", 0x2000, 0x0800, CRC(843fbfe5) SHA1(6671a3c55ef70447f2a127438e0c39857f8bf6b1) )
	ROM_LOAD( "pir7.bin", 0x2800, 0x0800, CRC(73084d5e) SHA1(cb04a4c9dbf1672ddf478d2fe92b0ffd0159bb9e) )
	ROM_LOAD( "p4.bin", 0x3000, 0x0800, CRC(9363a4d1) SHA1(4cb4a86d92a1f9bf233cac01aa266485a8bb7a34) )
	ROM_LOAD( "p8.bin", 0x3800, 0x0800, CRC(2769979c) SHA1(581592da26199b325de51791ddab66b474ab0413) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "p9.bin", 0x0000, 0x0800, CRC(94eb7563) SHA1(c99741ce1aebdfb89628fbfaecf5ae6b2719a0ca) )
	ROM_LOAD( "p11.bin", 0x0800, 0x0800, CRC(a3606973) SHA1(72297e1a33102c6a48b4c65f2a0b9bfc75a2df36) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "p10.bin", 0x0000, 0x0800, CRC(84165a2c) SHA1(95b24620fbf9bd0ec4dd2aeeb6d9305bd475dce2) )
	ROM_LOAD( "p12.bin", 0x0800, 0x0800, CRC(2699ba9e) SHA1(b91ff586defe65b200bea5ade7374c2c7579cd80) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "piranha.4a", 0x0020, 0x0100, CRC(08c9447b) SHA1(5e4fbfcc7179fc4b1436af9bb709ffc381479315) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( piranhah )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pr1.cpu",      0x0000, 0x1000, CRC(bc5ad024) SHA1(a3ed781b514a1068b24a7146a28f0a2adfaa2719) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pr3.cpu",      0x2000, 0x1000, CRC(473c379d) SHA1(6e7985367c3e544b4cb98ba8291908df88eafe7f) )
	ROM_LOAD( "pr4.cpu",      0x3000, 0x1000, CRC(63fbf895) SHA1(d328bf3b8f307fb774614834edec211117148e64) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pr5.cpu",      0x0000, 0x0800, CRC(3fc4030c) SHA1(5e45f0c19cf96daa17afd2fa1c628d7ac7f4a79c) )
	ROM_LOAD( "pr7.cpu",      0x0800, 0x0800, CRC(30b9a010) SHA1(b0ba8b6cd430feb32d11d092e1959b9f5d240f1b) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pr6.cpu",      0x0000, 0x0800, CRC(f3e9c9d5) SHA1(709a75b2457f21f0f1a3d9e7f4c8579468ee5cad) )
	ROM_LOAD( "pr8.cpu",      0x0800, 0x0800, CRC(133d720d) SHA1(8af75ed9e115a996379acedd44d0c09332ec5a03) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( pacplus )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68) SHA1(8531c54ca6b0de0ea4ccc34e0e801ba9847e75bc) )
	ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556) SHA1(8ba97215bdb75f0e70eb8d3223847efe4dc4fb48) )
	ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430) SHA1(4e8613d51a80cf106f883db79685e1e22541da45) )
	ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b) SHA1(b956ae5d66683aab74b90469ad36b5bb361d677e) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(022c35da) SHA1(57d7d723c7b029e3415801f4ce83469ec97bb8a1) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(4de65cdd) SHA1(9c0699204484be819b77f0b212c792fe9e9fae5d) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(063dd53a) SHA1(2e43b46ec3b101d1babab87cdaddfa944116ec06) )
	ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(e271a166) SHA1(cf006536215a7a1d488eebc1d8a2e2a8134ce1a6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mspacman )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code+64k for decrypted code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) SHA1(e87e059c5be45753f7e9f33dff851f16d6751181) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) SHA1(8e47e8c2c4d6117d174cdac150392042d3e0a881) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) SHA1(d4a70d56bb01d27d094d73db8667ffb00ca69cb9) )
	ROM_LOAD( "u5",           0x8000, 0x0800, CRC(f45fbbcd) SHA1(b26cc1c8ee18e9b1daa97956d2159b954703a0ec) )
	ROM_LOAD( "u6",           0x9000, 0x1000, CRC(a90e7000) SHA1(e4df96f1db753533f7d770aa62ae1973349ea4cf) )
	ROM_LOAD( "u7",           0xb000, 0x1000, CRC(c82cd714) SHA1(1d8ac7ad03db2dc4c8c18ade466e12032673f874) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mspacmab )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) SHA1(bc2247ec946b639dd1f00bfc603fa157d0baaa97) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) SHA1(13ea0c343de072508908be885e6a2a217bbb3047) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) SHA1(5ea4d907dbb2690698db72c4e0b5be4d3e9a7786) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) SHA1(3022a408118fa7420060e32a760aeef15b8a96cf) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) SHA1(fed6e9a2b210b07e7189a18574f6b8c4ec5bb49b) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) SHA1(387010a0c76319a1eab61b54c9bcb5c66c4b67a1) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mspacmat )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code+64k for decrypted code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) SHA1(e87e059c5be45753f7e9f33dff851f16d6751181) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) SHA1(674d3a7f00d8be5e38b1fdc208ebef5a92d38329) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) SHA1(8e47e8c2c4d6117d174cdac150392042d3e0a881) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) SHA1(d4a70d56bb01d27d094d73db8667ffb00ca69cb9) )
	ROM_LOAD( "u5",           0x8000, 0x0800, CRC(f45fbbcd) SHA1(b26cc1c8ee18e9b1daa97956d2159b954703a0ec) )
	ROM_LOAD( "u6pacatk",     0x9000, 0x1000, CRC(f6d83f4d) SHA1(6135b187d6b968554d08f2ac00d3a3313efb8638) )
	ROM_LOAD( "u7",           0xb000, 0x1000, CRC(c82cd714) SHA1(1d8ac7ad03db2dc4c8c18ade466e12032673f874) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mspacpls )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) SHA1(bc2247ec946b639dd1f00bfc603fa157d0baaa97) )
	ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31) SHA1(6ff73e4da4910bcd2ca3aa299d8ffad23f8abf79) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) SHA1(5ea4d907dbb2690698db72c4e0b5be4d3e9a7786) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) SHA1(3022a408118fa7420060e32a760aeef15b8a96cf) )
	ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954) SHA1(ee5b266b1cc178df31fc1da5f66ef4911c653dda) )
	ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308) SHA1(c1ba630cb8fb665c4881a6cce9d3b0d4300bd0eb) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( pacgal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) SHA1(bc2247ec946b639dd1f00bfc603fa157d0baaa97) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) SHA1(13ea0c343de072508908be885e6a2a217bbb3047) )
	ROM_LOAD( "pacman.7fh",   0x2000, 0x1000, CRC(513f4d5c) SHA1(ae011b89422bd8cbb80389814500bc1427f6ecb2) )
	ROM_LOAD( "pacman.7hj",   0x3000, 0x1000, CRC(70694c8e) SHA1(d0d02f0997b44e1ba5ea27fc3f7af1b956e2a687) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) SHA1(fed6e9a2b210b07e7189a18574f6b8c4ec5bb49b) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) SHA1(387010a0c76319a1eab61b54c9bcb5c66c4b67a1) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5ef",   0x0000, 0x0800, CRC(65a3ee71) SHA1(cbbf700eefba2a5bf158983f2ca9688b7c6f5d2b) )
	ROM_LOAD( "pacman.5hj",   0x0800, 0x0800, CRC(50c7477d) SHA1(c04ec282a8cb528df5e38ad750d12ee71612695d) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s129.4a",    0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mschamp )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "pm4.bin", 0x10000, 0x10000, CRC(7d6b6303) SHA1(65ad72a9188422653c02a48c07ed2661e1e36961) )	/* banked */

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pm5.bin", 0x0000, 0x0800, CRC(7fe6b9e2) SHA1(bfd0d84c7ef909ae078d8f60340682b3ff230aa6) )
	ROM_CONTINUE(        0x1000, 0x0800 )
	ROM_CONTINUE(        0x0800, 0x0800 )
	ROM_CONTINUE(        0x1800, 0x0800 )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )
ROM_END


ROM_START( crush )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for opcode copy to hack protection */
	ROM_LOAD( "crushkrl.6e",  0x0000, 0x1000, CRC(a8dd8f54) SHA1(4e3a973ea74a9e145c6997513b98fc80aa478442) )
	ROM_LOAD( "crushkrl.6f",  0x1000, 0x1000, CRC(91387299) SHA1(3ad8c28e02c45667e32860953b157832445a82c8) )
	ROM_LOAD( "crushkrl.6h",  0x2000, 0x1000, CRC(d4455f27) SHA1(53f8ffc28be664fa8a2d756b4c70045a3f041bea) )
	ROM_LOAD( "crushkrl.6j",  0x3000, 0x1000, CRC(d59fc251) SHA1(024605e4485b0ac826217256e5356ed9a6c8ef34) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5e",  0x0000, 0x1000, CRC(91bad2da) SHA1(096197d0cb6d55bf72b5be045224f4bd6a9cfa1b) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5f",  0x0000, 0x1000, CRC(aea79f55) SHA1(279021e6771dfa5bd0b7c557aae44434286d91b7) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( crush2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tp1",          0x0000, 0x0800, CRC(f276592e) SHA1(68ebb7d9f70af868d99ec42c26bc55a54ba1f22c) )
	ROM_LOAD( "tp5a",         0x0800, 0x0800, CRC(3d302abe) SHA1(8ca5cd82d099b55e20f785489158231a1d99a430) )
	ROM_LOAD( "tp2",          0x1000, 0x0800, CRC(25f42e70) SHA1(66de8203c364fd90e8a2b5749c2e40665b2f5830) )
	ROM_LOAD( "tp6",          0x1800, 0x0800, CRC(98279cbe) SHA1(84b5e64bdbc25afab9b6f53e1719640e21a6feba) )
	ROM_LOAD( "tp3",          0x2000, 0x0800, CRC(8377b4cb) SHA1(f828a177f22db9093a00c31e39e16214ce0dc6de) )
	ROM_LOAD( "tp7",          0x2800, 0x0800, CRC(d8e76c8c) SHA1(7c3d7eb07b9256130141f71eba722f7823fd4c32) )
	ROM_LOAD( "tp4",          0x3000, 0x0800, CRC(90b28fa3) SHA1(ff58d2dfb016397daabe2996bc3a7b63d28a4cca) )
	ROM_LOAD( "tp8",          0x3800, 0x0800, CRC(10854e1b) SHA1(b3b9066d9a43796185c00ae12f7bb2bbf42e3a07) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tpa",          0x0000, 0x0800, CRC(c7617198) SHA1(95b204af0345163f93811cc770ee0ca2851a39c1) )
	ROM_LOAD( "tpc",          0x0800, 0x0800, CRC(e129d76a) SHA1(c9256795c6d0929ade1f24b372dadc2a2b88d897) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tpb",          0x0000, 0x0800, CRC(d1899f05) SHA1(dce755511b6262b984a2bca329f454892e486a09) )
	ROM_LOAD( "tpd",          0x0800, 0x0800, CRC(d35d1caf) SHA1(65dd7861e05651485626465dc97215fed58db551) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( crush3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "unkmol.4e",    0x0000, 0x0800, CRC(49150ddf) SHA1(5a20464a40d1d48606664779c85a7679073d7954) )
	ROM_LOAD( "unkmol.6e",    0x0800, 0x0800, CRC(21f47e17) SHA1(1194b5e8b0cce1f480acda3cb6c1fc65988bdc80) )
	ROM_LOAD( "unkmol.4f",    0x1000, 0x0800, CRC(9b6dd592) SHA1(6bb1b7ed95a7a8682a6ab58fa9f02c34beea8cd4) )
	ROM_LOAD( "unkmol.6f",    0x1800, 0x0800, CRC(755c1452) SHA1(a2da17ed0e526dad4d53d332467a3dfd3b2a8cab) )
	ROM_LOAD( "unkmol.4h",    0x2000, 0x0800, CRC(ed30a312) SHA1(15855904422eb603e5c5465bd038a3e8c666c10d) )
	ROM_LOAD( "unkmol.6h",    0x2800, 0x0800, CRC(fe4bb0eb) SHA1(70e480a75421ee0832456f1d30bf45a702192625) )
	ROM_LOAD( "unkmol.4j",    0x3000, 0x0800, CRC(072b91c9) SHA1(808df98c0cfd2367a39e06f30f920fd14887d922) )
	ROM_LOAD( "unkmol.6j",    0x3800, 0x0800, CRC(66fba07d) SHA1(4944d69a38fd823dad38b70433848017ae7027d7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "unkmol.5e",    0x0000, 0x0800, CRC(338880a0) SHA1(beba1c71291394442b04fa5f4e1b833d7cf0fa8a) )
	ROM_LOAD( "unkmol.5h",    0x0800, 0x0800, CRC(4ce9c81f) SHA1(90a695ce4a45bde62bdbf09724a3ec6b45674660) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "unkmol.5f",    0x0000, 0x0800, CRC(752e3780) SHA1(5730ebd8091eba5ac32ddd9db2f42d718b088753) )
	ROM_LOAD( "unkmol.5j",    0x0800, 0x0800, CRC(6e00d2ac) SHA1(aa3f1f3a3b6899bea717d97e4817b13159e552e5) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( maketrax )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for opcode copy to hack protection */
	ROM_LOAD( "maketrax.6e",  0x0000, 0x1000, CRC(0150fb4a) SHA1(ba41582d5432670654479b4bf6d938d2168858af) )
	ROM_LOAD( "maketrax.6f",  0x1000, 0x1000, CRC(77531691) SHA1(68a450bcc8d832368d0f1cb2815cb5c03451796e) )
	ROM_LOAD( "maketrax.6h",  0x2000, 0x1000, CRC(a2cdc51e) SHA1(80d80235cda3ce19c1dbafacf3d47b1325ad4728) )
	ROM_LOAD( "maketrax.6j",  0x3000, 0x1000, CRC(0b4b5e0a) SHA1(621aece612df612065f776696956ef3671421fac) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5e",  0x0000, 0x1000, CRC(91bad2da) SHA1(096197d0cb6d55bf72b5be045224f4bd6a9cfa1b) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5f",  0x0000, 0x1000, CRC(aea79f55) SHA1(279021e6771dfa5bd0b7c557aae44434286d91b7) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( maketrxb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for opcode copy to hack protection */
	ROM_LOAD( "maketrax.6e",  0x0000, 0x1000, CRC(0150fb4a) SHA1(ba41582d5432670654479b4bf6d938d2168858af) )
	ROM_LOAD( "maketrax.6f",  0x1000, 0x1000, CRC(77531691) SHA1(68a450bcc8d832368d0f1cb2815cb5c03451796e) )
	ROM_LOAD( "maketrxb.6h",  0x2000, 0x1000, CRC(6ad342c9) SHA1(5469f3952adc682725a71602b4a00a7751e48a99) )
	ROM_LOAD( "maketrxb.6j",  0x3000, 0x1000, CRC(be27f729) SHA1(0f7b873d33f751fa2fc54f9eede0598cb7d7f3c8) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5e",  0x0000, 0x1000, CRC(91bad2da) SHA1(096197d0cb6d55bf72b5be045224f4bd6a9cfa1b) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maketrax.5f",  0x0000, 0x1000, CRC(aea79f55) SHA1(279021e6771dfa5bd0b7c557aae44434286d91b7) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( korosuke )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for opcode copy to hack protection */
	ROM_LOAD( "kr.6e",        0x0000, 0x1000, CRC(69f6e2da) SHA1(5f06523122d81a079bed080a16b44adb90aa95ad) )
	ROM_LOAD( "kr.6f",        0x1000, 0x1000, CRC(abf34d23) SHA1(6ae16fb8208037fd8b752076dd97e3da09e5cb8f) )
	ROM_LOAD( "kr.6h",        0x2000, 0x1000, CRC(76a2e2e2) SHA1(570aaed91279caab9274024e5a6176bdfe85bedd) )
	ROM_LOAD( "kr.6j",        0x3000, 0x1000, CRC(33e0e3bb) SHA1(43f5da486b9c44b0e4e8c909000786ee8ffee87f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kr.5e",        0x0000, 0x1000, CRC(e0380be8) SHA1(96eb7c5ef91342be67bd2a6c4958412d2572ba2a) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "kr.5f",        0x0000, 0x1000, CRC(63fec9ee) SHA1(7d136362e08cceba9395c2c469d8fec451c5e396) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mbrush )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mbrush.6e",    0x0000, 0x1000, CRC(750fbff7) SHA1(986d20010d4fdd4bac916ac6b3a01bcd09d695ea) )
	ROM_LOAD( "mbrush.6f",    0x1000, 0x1000, CRC(27eb4299) SHA1(af2d7fdedcea766045fc2f20ae383024d1c35731) )
	ROM_LOAD( "mbrush.6h",    0x2000, 0x1000, CRC(d297108e) SHA1(a5bd11f26ba82b66a93d07e8cbc838ad9bd01413) )
	ROM_LOAD( "mbrush.6j",    0x3000, 0x1000, CRC(6fd719d0) SHA1(3de00981264cef24dc2c6277192e071144da2a88) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tpa",          0x0000, 0x0800, CRC(c7617198) SHA1(95b204af0345163f93811cc770ee0ca2851a39c1) )
	ROM_LOAD( "mbrush.5h",    0x0800, 0x0800, CRC(c15b6967) SHA1(d8f16e2d6af5bf0f610d1e23614c531f67490da9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbrush.5f",    0x0000, 0x0800, CRC(d5bc5cb8) SHA1(269b82ae2b838c72ae06bff77412f22bb779ad2e) )  /* copyright sign was removed */
	ROM_LOAD( "tpd",          0x0800, 0x0800, CRC(d35d1caf) SHA1(65dd7861e05651485626465dc97215fed58db551) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( paintrlr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "paintrlr.1",   0x0000, 0x0800, CRC(556d20b5) SHA1(c0a74def85bca108fc56726d22bbea1fc051e1ff) )
	ROM_LOAD( "paintrlr.5",   0x0800, 0x0800, CRC(4598a965) SHA1(866dbe7c0dbca10c5d5ec3efa3c79fb1ff1c5b56) )
	ROM_LOAD( "paintrlr.2",   0x1000, 0x0800, CRC(2da29c81) SHA1(e77f84e2f3136a116b75b40869e7f59404b0dbab) )
	ROM_LOAD( "paintrlr.6",   0x1800, 0x0800, CRC(1f561c54) SHA1(ef1159f2203ff6b5c17e3a79f32e8cafb12a49f7) )
	ROM_LOAD( "paintrlr.3",   0x2000, 0x0800, CRC(e695b785) SHA1(bc627a1a03d2e701fa4051acee469a4516cfb5bf) )
	ROM_LOAD( "paintrlr.7",   0x2800, 0x0800, CRC(00e6eec0) SHA1(e98850cf6e1762d08225a95f26a26766f8fa7303) )
	ROM_LOAD( "paintrlr.4",   0x3000, 0x0800, CRC(0fd5884b) SHA1(fa9614b625b3d71a6e9d5f883da625ad88e3eb5e) )
	ROM_LOAD( "paintrlr.8",   0x3800, 0x0800, CRC(4900114a) SHA1(47aee5bad136c19b203958b7ddac583d45018249) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tpa",          0x0000, 0x0800, CRC(c7617198) SHA1(95b204af0345163f93811cc770ee0ca2851a39c1) )
	ROM_LOAD( "mbrush.5h",    0x0800, 0x0800, CRC(c15b6967) SHA1(d8f16e2d6af5bf0f610d1e23614c531f67490da9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mbrush.5f",    0x0000, 0x0800, CRC(d5bc5cb8) SHA1(269b82ae2b838c72ae06bff77412f22bb779ad2e) )  /* copyright sign was removed */
	ROM_LOAD( "tpd",          0x0800, 0x0800, CRC(d35d1caf) SHA1(65dd7861e05651485626465dc97215fed58db551) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "2s140.4a",     0x0020, 0x0100, CRC(63efb927) SHA1(5c144a613fc4960a1dfd7ead89e7fee258a63171) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( ponpoko )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ppokoj1.bin",  0x0000, 0x1000, CRC(ffa3c004) SHA1(d9e3186dcd4eb94d02bd24ad56030b248721537f) )
	ROM_LOAD( "ppokoj2.bin",  0x1000, 0x1000, CRC(4a496866) SHA1(4b8bd13e58040c30ca032b54fb47d889677e8c6f) )
	ROM_LOAD( "ppokoj3.bin",  0x2000, 0x1000, CRC(17da6ca3) SHA1(1a57767557c13fa3d08e4451fb9fb1f7219b26ef) )
	ROM_LOAD( "ppokoj4.bin",  0x3000, 0x1000, CRC(9d39a565) SHA1(d4835ee97c9b3c63504d8b576a11f0a3a97057ec) )
	ROM_LOAD( "ppoko5.bin",   0x8000, 0x1000, CRC(54ca3d7d) SHA1(b54299b00573fbd6d3278586df0c12c09235615d) )
	ROM_LOAD( "ppoko6.bin",   0x9000, 0x1000, CRC(3055c7e0) SHA1(ab3fb9c8846effdcea0569d08a84c5fa19057a8f) )
	ROM_LOAD( "ppoko7.bin",   0xa000, 0x1000, CRC(3cbe47ca) SHA1(577c79c016be26a9fc7895cef0f30bf3f0b15097) )
	ROM_LOAD( "ppokoj8.bin",  0xb000, 0x1000, CRC(04b63fc6) SHA1(9b86ae34aaefa2813d29a4f7b24cee40eadcc6a1) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ppoko9.bin",   0x0000, 0x1000, CRC(b73e1a06) SHA1(f1229e804eb15827b71f0e769a8c9e496c6d1de7) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ppoko10.bin",  0x0000, 0x1000, CRC(62069b5d) SHA1(1b58ad1c2cc2d12f4e492fdd665b726d50c80364) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( ponpokov )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ppoko1.bin",   0x0000, 0x1000, CRC(49077667) SHA1(3e760cd4dbe5913e58d786caf510237ff635c775) )
	ROM_LOAD( "ppoko2.bin",   0x1000, 0x1000, CRC(5101781a) SHA1(a82fbd2418ac7866f9463092e9dd37fd7ba9b694) )
	ROM_LOAD( "ppoko3.bin",   0x2000, 0x1000, CRC(d790ed22) SHA1(2d32f91f6993232db40b44b35bd2503d85e5c874) )
	ROM_LOAD( "ppoko4.bin",   0x3000, 0x1000, CRC(4e449069) SHA1(d5e6e346f80e66eb0db530de9721d9b6f22e86ae) )
	ROM_LOAD( "ppoko5.bin",   0x8000, 0x1000, CRC(54ca3d7d) SHA1(b54299b00573fbd6d3278586df0c12c09235615d) )
	ROM_LOAD( "ppoko6.bin",   0x9000, 0x1000, CRC(3055c7e0) SHA1(ab3fb9c8846effdcea0569d08a84c5fa19057a8f) )
	ROM_LOAD( "ppoko7.bin",   0xa000, 0x1000, CRC(3cbe47ca) SHA1(577c79c016be26a9fc7895cef0f30bf3f0b15097) )
	ROM_LOAD( "ppoko8.bin",   0xb000, 0x1000, CRC(b39be27d) SHA1(c299d22d26da68bec8fc53c898523135ec4016fa) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ppoko9.bin",   0x0000, 0x1000, CRC(b73e1a06) SHA1(f1229e804eb15827b71f0e769a8c9e496c6d1de7) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ppoko10.bin",  0x0000, 0x1000, CRC(62069b5d) SHA1(1b58ad1c2cc2d12f4e492fdd665b726d50c80364) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( eyes )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "d7",           0x0000, 0x1000, CRC(3b09ac89) SHA1(a8f1c918da74495bb73172f39364dada38ae4713) )
	ROM_LOAD( "e7",           0x1000, 0x1000, CRC(97096855) SHA1(10d3b164bbbe5eee86e881a1434f0c114ee8adff) )
	ROM_LOAD( "f7",           0x2000, 0x1000, CRC(731e294e) SHA1(96c0308c146dbd85e244c4530af9ae8df78c86de) )
	ROM_LOAD( "h7",           0x3000, 0x1000, CRC(22f7a719) SHA1(eb000b606ecedd52bebbb232e661fb1ef205f8b0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "d5",           0x0000, 0x1000, CRC(d6af0030) SHA1(652b779533e3f00e81cc102b78d367d503b06f33) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e5",           0x0000, 0x1000, CRC(a42b5201) SHA1(2e5cede3b6039c7bd5230de27d02aaa3f35a7b64) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s129.4a",    0x0020, 0x0100, CRC(d8d78829) SHA1(19820d1651423210083a087fb70ebea73ad34951) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( eyes2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g38201.7d",    0x0000, 0x1000, CRC(2cda7185) SHA1(7ec3ee9bb90e6a1d83ad3aa12fd62184e07b1399) )
	ROM_LOAD( "g38202.7e",    0x1000, 0x1000, CRC(b9fe4f59) SHA1(2d97dc1a0458b406ca0c50d6b8bd0dbe58d21464) )
	ROM_LOAD( "g38203.7f",    0x2000, 0x1000, CRC(d618ba66) SHA1(76d93d8bc09bafac464ebfd002869e21535a365b) )
	ROM_LOAD( "g38204.7h",    0x3000, 0x1000, CRC(cf038276) SHA1(bcf4e129a151e2245e630cf865ce6cb009b405a5) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g38205.5d",    0x0000, 0x1000, CRC(03b1b4c7) SHA1(a90b2fbaee2888ee4f0bcdf80a069c8594ef5ea1) )  /* this one has a (c) sign */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e5",           0x0000, 0x1000, CRC(a42b5201) SHA1(2e5cede3b6039c7bd5230de27d02aaa3f35a7b64) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s129.4a",    0x0020, 0x0100, CRC(d8d78829) SHA1(19820d1651423210083a087fb70ebea73ad34951) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( mrtnt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tnt.1",        0x0000, 0x1000, CRC(0e836586) SHA1(5037b7c618f05bc3d6a33694729ae575b9aa7dbb) )
	ROM_LOAD( "tnt.2",        0x1000, 0x1000, CRC(779c4c5b) SHA1(5ecac4f5b64b306c73d8f57d5260b586789b3055) )
	ROM_LOAD( "tnt.3",        0x2000, 0x1000, CRC(ad6fc688) SHA1(e5729e4e42a5b9b3a26de8a44b3a78b49c8b1d8e) )
	ROM_LOAD( "tnt.4",        0x3000, 0x1000, CRC(d77557b3) SHA1(689746653b1e19fbcddd0d71db2b86d1019235aa) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tnt.5",        0x0000, 0x1000, CRC(3038cc0e) SHA1(f8f5927ea4cbfda8fa7546abd766ba2e8b020004) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tnt.6",        0x0000, 0x1000, CRC(97634d8b) SHA1(4c0fa4bc44bbb4b4614b5cc05e811c469c0e78e8) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "mrtnt08.bin",  0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "mrtnt04.bin",  0x0020, 0x0100, NO_DUMP )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( gorkans )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "gorkans8.rom",        0x0000, 0x0800, CRC(55100b18) SHA1(8f657c1b2865987b60d95960c5297a82bb1cc6e0) )
	ROM_LOAD( "gorkans4.rom",        0x0800, 0x0800, CRC(b5c604bf) SHA1(0f3608d630fba9d4734a3ef30199a5d1a067cdff) )
 	ROM_LOAD( "gorkans7.rom",        0x1000, 0x0800, CRC(b8c6def4) SHA1(58ac78fc5b3559ef771ca708a79089b7a00cf6b8) )
	ROM_LOAD( "gorkans3.rom",        0x1800, 0x0800, CRC(4602c840) SHA1(c77de0e991c44c2ee8a4537e264ac8fbb1b4b7db) )
	ROM_LOAD( "gorkans6.rom",        0x2000, 0x0800, CRC(21412a62) SHA1(ece44c3204cf182db23b594ebdc051b51340ba2b) )
	ROM_LOAD( "gorkans2.rom",        0x2800, 0x0800, CRC(a013310b) SHA1(847ba7ca033eaf49245bef49d6513619edec3472) )
	ROM_LOAD( "gorkans5.rom",        0x3000, 0x0800, CRC(122969b2) SHA1(0803e1ec5e5ed742ea83ff156ae75a2d48530f71) )
	ROM_LOAD( "gorkans1.rom",        0x3800, 0x0800, CRC(f2524b11) SHA1(1216b963e73c1de63cc323e361875f6810d83a05) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gorkgfx4.rom",        0x0000, 0x0800, CRC(39cd0dbc) SHA1(8d6882dad94b26da8f0737e7f7f99946fe273f1b) )
	ROM_LOAD( "gorkgfx2.rom",        0x0800, 0x0800, CRC(33d52535) SHA1(e78ac5afa1ce996c41005c619ba2d2aa718497fc) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gorkgfx3.rom",        0x0000, 0x0800, CRC(4b6b7970) SHA1(1d8b65cad0b834fb920135fc907432042bc83db2) )
	ROM_LOAD( "gorkgfx1.rom",        0x0800, 0x0800, CRC(ed70bb3c) SHA1(7e51ddcf496f3b80fe186acc8bc6a0e574340346) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "gorkprom.4",   0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "gorkprom.1",   0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "gorkprom.3",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "gorkprom.2"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( eggor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.bin",        0x0000, 0x0800, CRC(818ed154) SHA1(8c0f555a3ab1d20a2c284d721b31278a0ddf9e51) )
	ROM_LOAD( "5.bin",        0x0800, 0x0800, CRC(a4b21d93) SHA1(923b7a06f9146c7bcda4cdb16b15d2bbbec95eab) )
	ROM_LOAD( "2.bin",        0x1000, 0x0800, CRC(5d7a23ed) SHA1(242fd973b0bde91c38e1f5e7f6c53d737019ec9c) )
	ROM_LOAD( "6.bin",        0x1800, 0x0800, CRC(e9dbca8d) SHA1(b66783d68df778910cc190159aba07b476ff01af) )
	ROM_LOAD( "3.bin",        0x2000, 0x0800, CRC(4318ab85) SHA1(eda9bb1bb8102e1c2cf838d0682732a45609f430) )
	ROM_LOAD( "7.bin",        0x2800, 0x0800, CRC(03214d7f) SHA1(0e1b602fbdedfe81452109912fed006653bdc455) )
	ROM_LOAD( "4.bin",        0x3000, 0x0800, CRC(dc805be4) SHA1(18604b221cd8af23ff8a05c954a42c3aa9e1948a) )
	ROM_LOAD( "8.bin",        0x3800, 0x0800, CRC(f9ae204b) SHA1(53022d2d7b83f44c46fdcca454815cf1f65c34d1) )


	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9.bin",        0x0000, 0x0800, CRC(96ad8626) SHA1(f003a6e1b00a51bfe326eac18658fafd58c88f88) )
	ROM_LOAD( "11.bin",       0x0800, 0x0800, CRC(cc324017) SHA1(ea96572e3e24714033688fe7ca99af2fc707c1d3) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "10.bin",        0x0000, 0x0800, CRC(7c97f513) SHA1(6f78c7cde321ea6ac51d08d0e3620653d0af87db) )
	ROM_LOAD( "12.bin",        0x0800, 0x0800, CRC(2e930602) SHA1(4012ec0cc542061b27b9b508bedde3f2ffc11838) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	/* the board was stripped of its proms, these are the standard ones from Pacman, they look reasonable
	   but without another board its impossible to say if they are actually good */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, BAD_DUMP CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, BAD_DUMP CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( lizwiz )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6e.cpu",       0x0000, 0x1000, CRC(32bc1990) SHA1(467c9d70e07f403b6b9118aebe4e6d0abb40a5c1) )
	ROM_LOAD( "6f.cpu",       0x1000, 0x1000, CRC(ef24b414) SHA1(12fce48008c4f9387df0c84f3b0d7c5a1b35d898) )
	ROM_LOAD( "6h.cpu",       0x2000, 0x1000, CRC(30bed83d) SHA1(8c2458f98320c6887580c71632b544da0a582ba2) )
	ROM_LOAD( "6j.cpu",       0x3000, 0x1000, CRC(dd09baeb) SHA1(f91447ec1f06bf95106e6872d80dcb82e1d42ffb) )
	ROM_LOAD( "wiza",         0x8000, 0x1000, CRC(f6dea3a6) SHA1(ec0b123fd2e6de6681ca14f35fda249b2c2ec44f) )
	ROM_LOAD( "wizb",         0x9000, 0x1000, CRC(f27fb5a8) SHA1(3ea384a1064302709d97fc16b347d3c012e90ac7) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e.cpu",       0x0000, 0x1000, CRC(45059e73) SHA1(c960cd5720bfa21db73e1000fe8be7d5baf2a3a1) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu",       0x0000, 0x1000, CRC(d2469717) SHA1(194c8f816e5ff7614b3db4f355223667105738fa) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "7f.cpu",       0x0000, 0x0020, CRC(7549a947) SHA1(4f2c3e7d6c38f0b9a90317f91feb3f86c9a0d0a5) )
	ROM_LOAD( "4a.cpu",       0x0020, 0x0100, CRC(5fdca536) SHA1(3a09b29374031aaa3722932aff974a467b3bb201) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( theglobp )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "glob.u2",      0x0000, 0x2000, CRC(829d0bea) SHA1(89f52b459a03fb40b9bbd97ac8a292f7ead6faba) )
	ROM_LOAD( "glob.u3",      0x2000, 0x2000, CRC(31de6628) SHA1(35a47dcf34efd74b5b2fda137e06a3dcabd74854) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "glob.5e",      0x0000, 0x1000, CRC(53688260) SHA1(9ce0d1d67d12743b69e8190bf7506b00b2f02955) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "glob.5f",      0x0000, 0x1000, CRC(051f59c7) SHA1(e1e1322686997e5bcdac164704b328cce352ae42) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "glob.7f",      0x0000, 0x0020, CRC(1f617527) SHA1(448845cab63800a05fcb106897503d994377f78f) )
	ROM_LOAD( "glob.4a",      0x0020, 0x0100, CRC(28faa769) SHA1(7588889f3102d4e0ca7918f536556209b2490ea1) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( beastf )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bf-u2.bin",    0x0000, 0x2000, CRC(3afc517b) SHA1(5b74bca9e9cd4d8bcf94a340f8f0e53fe1dcfc1d) )
	ROM_LOAD( "bf-u3.bin",    0x2000, 0x2000, CRC(8dbd76d0) SHA1(058c01e87ad583eb99d5043a821e6c68f1b30267) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "beastf.5e",    0x0000, 0x1000, CRC(5654dc34) SHA1(fc2336b951a3ab48d4fc4f36a8dd80e79e8ca1a0) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "beastf.5f",    0x0000, 0x1000, CRC(1b30ca61) SHA1(8495d8a280346246f00c4f1dc42ab5a2a02c5863) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "glob.7f",      0x0000, 0x0020, CRC(1f617527) SHA1(448845cab63800a05fcb106897503d994377f78f) )
	ROM_LOAD( "glob.4a",      0x0020, 0x0100, CRC(28faa769) SHA1(7588889f3102d4e0ca7918f536556209b2490ea1) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( vanvan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "van-1.50",     0x0000, 0x1000, CRC(cf1b2df0) SHA1(938b4434c0129cf9151f829901d00e47dca68956) )
	ROM_LOAD( "van-2.51",     0x1000, 0x1000, CRC(df58e1cb) SHA1(5e0fc713b50d46c7650d6564c20882891864cdc5) )
	ROM_LOAD( "van-3.52",     0x2000, 0x1000, CRC(15571e24) SHA1(d259d81fce16e151b32ac81f94a13b7044fdef95) )
	ROM_LOAD( "van-4.53",     0x3000, 0x1000, CRC(b724cbe0) SHA1(5fe1d3b81d07c538c31daf6522b26bbf35cfc512) )
	ROM_LOAD( "van-5.39",     0x8000, 0x1000, CRC(db67414c) SHA1(19eba21dfea24507b386ea1b5ce737c5822b0696) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "van-20.18",    0x0000, 0x1000, CRC(60efbe66) SHA1(ac398f77bfeab3d18ffd496e117825bfbeed4b62) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "van-21.19",    0x0000, 0x1000, CRC(5dd53723) SHA1(f75c869ac364f477d532e695347ceb5e281f9efa) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1.6",     0x0000, 0x0020, CRC(ce1d9503) SHA1(b829bed78c02d9998c1aecb8f6813e90b417a7f2) )
	ROM_LOAD( "6301-1.37",    0x0020, 0x0100, CRC(4b803d9f) SHA1(59b7f2e22c4e0b20ac3b12d88996a6dfeebc5933) )
ROM_END

ROM_START( vanvank )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "van1.bin",	  0x0000, 0x1000, CRC(00f48295) SHA1(703fab63760cadcce042b491d7d1d45301319158) )
	ROM_LOAD( "van-2.51",     0x1000, 0x1000, CRC(df58e1cb) SHA1(5e0fc713b50d46c7650d6564c20882891864cdc5) )
	ROM_LOAD( "van-3.52",     0x2000, 0x1000, CRC(15571e24) SHA1(d259d81fce16e151b32ac81f94a13b7044fdef95) )
	ROM_LOAD( "van4.bin",     0x3000, 0x1000, CRC(f8b37ed5) SHA1(34f844be891dfa5f6a1160de6f428e9dacd618a8) )
	ROM_LOAD( "van5.bin",     0x8000, 0x1000, CRC(b8c1e089) SHA1(c614fb9159210f6cf68f5085bfebd928caded91c) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "van-20.18",    0x0000, 0x1000, CRC(60efbe66) SHA1(ac398f77bfeab3d18ffd496e117825bfbeed4b62) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "van-21.19",    0x0000, 0x1000, CRC(5dd53723) SHA1(f75c869ac364f477d532e695347ceb5e281f9efa) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1.6",     0x0000, 0x0020, CRC(ce1d9503) SHA1(b829bed78c02d9998c1aecb8f6813e90b417a7f2) )
	ROM_LOAD( "6301-1.37",    0x0020, 0x0100, CRC(4b803d9f) SHA1(59b7f2e22c4e0b20ac3b12d88996a6dfeebc5933) )
ROM_END

ROM_START( dremshpr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "red_1.50",	  0x0000, 0x1000, CRC(830c6361) SHA1(a28c517a9b7f509e0dedacea64b9740335315457) )
	ROM_LOAD( "red_2.51",     0x1000, 0x1000, CRC(d22551cc) SHA1(2c513908899b618f0c0a0c3e48c4a4aad90f627e) )
	ROM_LOAD( "red_3.52",     0x2000, 0x1000, CRC(0713a34a) SHA1(37733b557e6afe116f5d3c8bc918f59124a8229d) )
	ROM_LOAD( "red_4.53",     0x3000, 0x1000, CRC(f38bcaaa) SHA1(cdebeaf5b77ac5a8b4668cff97b6351e075b392b) )
	ROM_LOAD( "red_5.39",     0x8000, 0x1000, CRC(6a382267) SHA1(7d6a1c75de8a6eb714ba9a18dd3c497832145bcc) )
	ROM_LOAD( "red_6.40",     0x9000, 0x1000, CRC(4cf8b121) SHA1(04162b41e747dfa442b958bd360e49993c5c4162) )
	ROM_LOAD( "red_7.41",     0xa000, 0x1000, CRC(bd4fc4ba) SHA1(50a5858acde5fd4b3476f5502141e7d492c3af9f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "red-20.18",    0x0000, 0x1000, CRC(2d6698dc) SHA1(5f5e54fdcff53c6ba783d585cd994cf563c53613) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "red-21.19",    0x0000, 0x1000, CRC(38c9ce9b) SHA1(c719bcd77549228e72ad9bcc42f5db0070ec5dca) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1.6",     0x0000, 0x0020, CRC(ce1d9503) SHA1(b829bed78c02d9998c1aecb8f6813e90b417a7f2) )
	ROM_LOAD( "6301-1.37",    0x0020, 0x0100, CRC(39d6fb5c) SHA1(848f9cd02f90006e8a2aae3693b57ae391cf498b) )
ROM_END


ROM_START( alibaba )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6e",           0x0000, 0x1000, CRC(38d701aa) SHA1(4e886a4a17f441f6d1d213c4c433b40dd38eefbc) )
	ROM_LOAD( "6f",           0x1000, 0x1000, CRC(3d0e35f3) SHA1(6b9a1fd11db9f521417566ae4c7065151aa272b5) )
	ROM_LOAD( "6h",           0x2000, 0x1000, CRC(823bee89) SHA1(5381a4fcbc9fa97574c6df2978c7500164df75e5) )
	ROM_LOAD( "6k",           0x3000, 0x1000, CRC(474d032f) SHA1(4516a60ec83e3c3388cd56f538f49afc86a50983) )
	ROM_LOAD( "6l",           0x8000, 0x1000, CRC(5ab315c1) SHA1(6f3507ad10432f9123150b8bc1d0fb52372a412b) )
	ROM_LOAD( "6m",           0xa000, 0x0800, CRC(438d0357) SHA1(7caaf668906b76d4947e988c444723b33f8e9726) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x0800, CRC(85bcb8f8) SHA1(986170627953582b1e6fbca59e5c15cf8c4de9c7) )
	ROM_LOAD( "5h",           0x0800, 0x0800, CRC(38e50862) SHA1(094d090bd0563f75d6ff1bfe2302cae941a89504) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, CRC(b5715c86) SHA1(ed6aee778295b0182d32846b5e41776b5b15420c) )
	ROM_LOAD( "5k",           0x0800, 0x0800, CRC(713086b3) SHA1(a1609bae637207a82920678f05bcc10a5ff096de) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.e7",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s129.a4",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */

	// unknown, used for the mistery items ?
	// 1ST AND 2ND HALF IDENTICAL
	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD( "7.p6",         0x000, 0x1000, CRC(d8eb7cbd) SHA1(fe482d36d81de5c5034e18b91bfa6b43111e683e) )
ROM_END


ROM_START( jumpshot )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "6e",           0x0000, 0x1000, CRC(f00def9a) SHA1(465a7f368e61a1e6614d6eab0fa2c6319920eaa5) )
	ROM_LOAD( "6f",           0x1000, 0x1000, CRC(f70deae2) SHA1(a8a8369e865b62cb9ed66d3de2396c6a5fced549) )
	ROM_LOAD( "6h",           0x2000, 0x1000, CRC(894d6f68) SHA1(8693ffc29587cdd1be0b42cede53f8f450a2c7fa) )
	ROM_LOAD( "6j",           0x3000, 0x1000, CRC(f15a108a) SHA1(db5c8394f688c6f889cadddeeae4fbca63c29a4c) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(d9fa90f5) SHA1(3c37fe077a77baa802230dddbc4bb2c05985d2bb) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(2ec711c1) SHA1(fcc3169f48eb7d4af533ad0169701e4230ff5a1f) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "prom.7f",      0x0000, 0x0020, CRC(872b42f3) SHA1(bbcd392ba3d2a5715e92fa0f7a7cf1e7e6e655a2) )
	ROM_LOAD( "prom.4a",      0x0020, 0x0100, CRC(0399f39f) SHA1(e98f08da4666cab44e01acb760a1bd2fc858bc0d) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END


ROM_START( shootbul )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "sb6e.cpu",     0x0000, 0x1000, CRC(25daa5e9) SHA1(8257de5e0e62235d05d74b53e5fc716e85cb05b9) )
	ROM_LOAD( "sb6f.cpu",     0x1000, 0x1000, CRC(92144044) SHA1(905a354a806da47ab40577171acdac7db635d102) )
	ROM_LOAD( "sb6h.cpu",     0x2000, 0x1000, CRC(43b7f99d) SHA1(6372763fbbca3581376204c5e58ceedd3f47fc60) )
	ROM_LOAD( "sb6j.cpu",     0x3000, 0x1000, CRC(bc4d3bbf) SHA1(2fa15b339166b9a5bf711b58a1705bc0b9e528e2) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sb5e.cpu",     0x0000, 0x1000, CRC(07c6c5aa) SHA1(cbe99ece795f29fdeef374cbf9b1f45ff065e803) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sb5f.cpu",     0x0000, 0x1000, CRC(eaec6837) SHA1(ff21b0fd5381afb1ba7f5920132006ee8e6d10eb) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "7f.rom",       0x0000, 0x0020, CRC(ec578b98) SHA1(196da49cc260f967ec5f01bc3c75b11077c85998) )
	ROM_LOAD( "4a.rom",       0x0020, 0x0100, CRC(81a6b30f) SHA1(60c767fd536c325151a2b759fdbce4ba41e0c78f) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( dderby )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "boot1", 0x0000, 0x1000, CRC(6f373bd4) )
	ROM_LOAD( "boot2", 0x1000, 0x1000, CRC(2fbf16bf) )
	ROM_LOAD( "boot3", 0x2000, 0x1000, CRC(6e16cd16) )
	ROM_LOAD( "boot4", 0x3000, 0x1000, CRC(f7e09874) )
	ROM_LOAD( "boot5", 0x8000, 0x1000, CRC(f154670a) )
	ROM_LOAD( "boot6", 0x9000, 0x1000, CRC(f154670a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e", 0x0000, 0x1000, CRC(7e2c0a53) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f", 0x0000, 0x1000, CRC(cb2dd072) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) ) /* timing - not used */
ROM_END

ROM_START( bace ) 
ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */ 
ROM_LOAD( "boot1", 0x0000, 0x1000, CRC(8b60ff7c) ) 
ROM_LOAD( "boot2", 0x1000, 0x1000, CRC(25d8361a) ) 
ROM_LOAD( "boot3", 0x2000, 0x1000, CRC(fc38d994) ) 
ROM_LOAD( "boot4", 0x3000, 0x1000, CRC(5853f341) ) 
ROM_LOAD( "boot5", 0x8000, 0x1000, CRC(f154670a) ) 
ROM_LOAD( "boot6", 0x9000, 0x1000, CRC(f154670a) ) 
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) 
ROM_LOAD( "5e", 0x0000, 0x1000, CRC(6da99c7b) ) 
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE ) 
ROM_LOAD( "5f", 0x0000, 0x1000, CRC(b81cdc64) ) 
ROM_REGION( 0x0120, REGION_PROMS, 0 ) 
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) ) 
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) ) 
ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */ 
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) ) 
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) ) /* timing - not used */ 
ROM_END


ROM_START( acitya )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "aca_u2.bin",   0x0000, 0x2000, CRC(261c2fdc) SHA1(b4e7e6c8d8e401c7e4673213074802a73b9886a2) )
	ROM_LOAD( "aca_u3.bin",   0x2000, 0x2000, CRC(05fab4ca) SHA1(5172229eda25920eeaa6d9f610f2bcfa674979b7) )


	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "aca_5e.bin",   0x0000, 0x1000, CRC(7f2dd2c9) SHA1(aa7ea70355904989b99d568d1e055e8272cfa8ca) )

 	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "aca_5e.bin",   0x0000, 0x1000, CRC(7f2dd2c9) SHA1(aa7ea70355904989b99d568d1e055e8272cfa8ca) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "aca_7f.bin",   0x0000, 0x0020, CRC(133bb744) SHA1(da4074f3ea30202973f0b6c9ad05a992bb44eafd) )
	ROM_LOAD( "aca_4a.bin",   0x0020, 0x0100, CRC(8e29208f) SHA1(a30a405fbd43d27a8d403f6c3545178564dede5d) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END

ROM_START( bwcasino )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "bwc_u2.bin",   0x0000, 0x2000, CRC(e2eea868) SHA1(9e9dae02ab746ef48981f42a75c192c5aae0ffee) )
	ROM_LOAD( "bwc_u3.bin",   0x2000, 0x2000, CRC(a935571e) SHA1(ab4f53be2544593fc8eb4c4bcccdec4191c0c626) )


	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bwc_5e.bin",   0x0000, 0x1000, CRC(e334c01e) SHA1(cc6e50e3cf51eb8b7b27aa7351733954da8128ff) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bwc_5e.bin",   0x0000, 0x1000, CRC(e334c01e) SHA1(cc6e50e3cf51eb8b7b27aa7351733954da8128ff) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "aca_7f.bin",   0x0000, 0x0020, CRC(133bb744) SHA1(da4074f3ea30202973f0b6c9ad05a992bb44eafd) )
	ROM_LOAD( "aca_4a.bin",   0x0020, 0x0100, CRC(8e29208f) SHA1(a30a405fbd43d27a8d403f6c3545178564dede5d) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( newpuc2 )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "6e.cpu", 0x0000, 0x0800, CRC(69496a98) SHA1(2934051d6305cc3654951bc1aacf2b8902f463fe) )
	ROM_LOAD( "6k.cpu", 0x0800, 0x0800, CRC(158fc01c) SHA1(2f7a1e24d259fdc716ef8e7354a87780595f3c4e) )
	ROM_LOAD( "6f.cpu", 0x1000, 0x0800, CRC(7d177853) SHA1(9b5ddaaa8b564654f97af193dbcc29f81f230a25) )
	ROM_LOAD( "6m.cpu", 0x1800, 0x0800, CRC(70810ccf) SHA1(3941678606aab1e53356a6781e24d84e83cc88ce) )
	ROM_LOAD( "6h.cpu", 0x2000, 0x0800, CRC(81719de8) SHA1(e886d04ac0e20562a4bd2df7676bdf9aa98665d7) )
	ROM_LOAD( "6n.cpu", 0x2800, 0x0800, CRC(3f250c58) SHA1(53bf2270c26f10f7e97960cd4c96e09e16b9bdf3) )
	ROM_LOAD( "6j.cpu", 0x3000, 0x0800, CRC(e6675736) SHA1(85d0bb79bc96acbc67fcb70ff4d453c870a6c8ea) )
	ROM_LOAD( "6p.cpu", 0x3800, 0x0800, CRC(1f81e765) SHA1(442d8a82e79ae842f1ffb46369c632c1d0b83161) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE)
	ROM_LOAD( "5e.cpu", 0x0000, 0x0800, CRC(2066a0b7) SHA1(6d4ccc27d6be185589e08aa9f18702b679e49a4a) )
	ROM_LOAD( "5h.cpu", 0x0800, 0x0800, CRC(777c70d3) SHA1(ed5ccbeb1102ec9f837577de3aa51317c32520d6) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu", 0x0000, 0x0800, CRC(ca8c184c) SHA1(833aa845824ed80777b62f03df36a920ad7c3656) )
	ROM_LOAD( "5j.cpu", 0x0800, 0x0800, CRC(7dc75a81) SHA1(d3fe1cad3b594052d8367685febb2335b0ad62f4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )   /*timing - not used */
ROM_END


ROM_START( newpuc2b )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "np2b1.bin", 0x0000, 0x0800, CRC(9d027c4a) SHA1(88e094880057451a75cdc2ce9477403021813982) )
	ROM_LOAD( "6k.cpu", 0x0800, 0x0800, CRC(158fc01c) SHA1(2f7a1e24d259fdc716ef8e7354a87780595f3c4e) )
	ROM_LOAD( "6f.cpu", 0x1000, 0x0800, CRC(7d177853) SHA1(9b5ddaaa8b564654f97af193dbcc29f81f230a25) )
	ROM_LOAD( "6m.cpu", 0x1800, 0x0800, CRC(70810ccf) SHA1(3941678606aab1e53356a6781e24d84e83cc88ce) )
	ROM_LOAD( "np2b3.bin", 0x2000, 0x0800, CRC(f5e4b2b1) SHA1(68464f61cc50931f6cd4bb493dd703c139500825) )
	ROM_LOAD( "6n.cpu", 0x2800, 0x0800, CRC(3f250c58) SHA1(53bf2270c26f10f7e97960cd4c96e09e16b9bdf3) )
	ROM_LOAD( "np2b4.bin", 0x3000, 0x0800, CRC(f068e009) SHA1(a30763935e116559d535654827230bb21a5734bb) )
	ROM_LOAD( "np2b8.bin", 0x3800, 0x0800, CRC(1fadcc2f) SHA1(2d636cfc2b52b671ac5a26a03b1195e2cf8d4718) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE)
	ROM_LOAD( "5e.cpu", 0x0000, 0x0800, CRC(2066a0b7) SHA1(6d4ccc27d6be185589e08aa9f18702b679e49a4a) )
	ROM_LOAD( "5h.cpu", 0x0800, 0x0800, CRC(777c70d3) SHA1(ed5ccbeb1102ec9f837577de3aa51317c32520d6) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu", 0x0000, 0x0800, CRC(ca8c184c) SHA1(833aa845824ed80777b62f03df36a920ad7c3656) )
	ROM_LOAD( "5j.cpu", 0x0800, 0x0800, CRC(7dc75a81) SHA1(d3fe1cad3b594052d8367685febb2335b0ad62f4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )   /*timing - not used */
ROM_END

ROM_START( nmouse )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "naumouse.d7", 0x0000, 0x0800, CRC(e447ecfa) SHA1(45bce93f4a4e1c9994fb6b0c81691a14cae43ae5) )
	ROM_LOAD( "naumouse.d6", 0x0800, 0x0800, CRC(2e6f13d9) SHA1(1278bd1ddd84ac5b956cb4d25c151871fab2b1d9) )
	ROM_LOAD( "naumouse.e7", 0x1000, 0x0800, CRC(44a80f97) SHA1(d06ffd96c72c3c8a3c71df564e8f5f9fb289b398) )
	ROM_LOAD( "naumouse.e6", 0x1800, 0x0800, CRC(9c7a46bd) SHA1(04771a99295fc6d3c41807e2c4437ff4e7e4ba4a) )
	ROM_LOAD( "naumouse.h7", 0x2000, 0x0800, CRC(5bc94c5d) SHA1(9238add33bbde151532b7ce3917566d9b4f67c62) )
	ROM_LOAD( "naumouse.h6", 0x2800, 0x0800, CRC(1af29e22) SHA1(628291aa97f5f88793f624af66a0c2b021328ef9) )
	ROM_LOAD( "naumouse.j7", 0x3000, 0x0800, CRC(cc3be185) SHA1(92fdc87256d16c4e400da83e3ca2786012766767) )
	ROM_LOAD( "naumouse.j6", 0x3800, 0x0800, CRC(66b3e5dc) SHA1(0ca7e67ef0ff908bb9953399f024e8b1aaf74e55) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE)
	ROM_LOAD( "naumouse.d5", 0x0000, 0x0800, CRC(2ea7cc3f) SHA1(ffeca1c382a7ae0cd898eab2905a0e8e96b95bee) )
	ROM_LOAD( "naumouse.h5", 0x0800, 0x0800, CRC(0511fcea) SHA1(52a498ca024b5c758ad0c978d3f67cdbbf2c56d3) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "naumouse.e5", 0x0000, 0x0800, CRC(f5a627cd) SHA1(2b8bc6d29e2aead924423a232c130151c8a8ebe5) )
	ROM_LOAD( "naumouse.j5", 0x0800, 0x0800, CRC(65f2580e) SHA1(769905837b98736ef2bfcaafa7820083dad80c57) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "naumouse.a4", 0x0020, 0x0100, CRC(d8772167) SHA1(782fa53f0de7262924a92d75f12a42bc4e44c812) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )   /*timing - not used */
ROM_END

ROM_START( nmouseb )
	ROM_REGION( 0x10000, REGION_CPU1,0 )     /* 64k for code */
	ROM_LOAD( "naumouse.d7", 0x0000, 0x0800, CRC(e447ecfa) SHA1(45bce93f4a4e1c9994fb6b0c81691a14cae43ae5) )
	ROM_LOAD( "naumouse.d6", 0x0800, 0x0800, CRC(2e6f13d9) SHA1(1278bd1ddd84ac5b956cb4d25c151871fab2b1d9) )
	ROM_LOAD( "naumouse.e7", 0x1000, 0x0800, CRC(44a80f97) SHA1(d06ffd96c72c3c8a3c71df564e8f5f9fb289b398) )
	ROM_LOAD( "naumouse.e6", 0x1800, 0x0800, CRC(9c7a46bd) SHA1(04771a99295fc6d3c41807e2c4437ff4e7e4ba4a) )
	ROM_LOAD( "snatch2.bin", 0x2000, 0x0800, CRC(405aa389) SHA1(687c82d94309c4ed83b72d656dbe7068b1de1b44) )
	ROM_LOAD( "snatch6.bin", 0x2800, 0x0800, CRC(f58e7df4) SHA1(a0853374a2a8c3ab572154d12e2e6297c97bd8b9) )
	ROM_LOAD( "snatch3.bin", 0x3000, 0x0800, CRC(06fb18ec) SHA1(ad57ffdb0fc5acdddeb85c4ce3ad618124fd7a6d) )
	ROM_LOAD( "snatch7.bin", 0x3800, 0x0800, CRC(d187b82b) SHA1(db739d5894a7fbfbc2e384ee1bdfe170935b2df7) )

	ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE)
	ROM_LOAD( "naumouse.d5", 0x0000, 0x0800, CRC(2ea7cc3f) SHA1(ffeca1c382a7ae0cd898eab2905a0e8e96b95bee) )
	ROM_LOAD( "naumouse.h5", 0x0800, 0x0800, CRC(0511fcea) SHA1(52a498ca024b5c758ad0c978d3f67cdbbf2c56d3) )

	ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "naumouse.e5", 0x0000, 0x0800, CRC(f5a627cd) SHA1(2b8bc6d29e2aead924423a232c130151c8a8ebe5) )
	ROM_LOAD( "snatch11.bin", 0x0800, 0x0800, CRC(330230a5) SHA1(3de4e454dd51b2ef05b5e1c74c8d12f8cb3f42ef) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "naumouse.a4", 0x0020, 0x0100, CRC(d8772167) SHA1(782fa53f0de7262924a92d75f12a42bc4e44c812) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )   /*timing - not used */
ROM_END


ROM_START( bigbucks )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p.rom",        0x0000, 0x4000, CRC(eea6c1c9) SHA1(eaea4ffbcdfbb38364887830fd00ac87fe838006) )
	ROM_LOAD( "m.rom",        0x8000, 0x2000, CRC(bb8f7363) SHA1(11ebdb1a3c589515240d006646f2fb3ead06bdcf) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e.cpu",       0x0000, 0x1000, CRC(18442c37) SHA1(fac445d15731532364315852492b48470039c0ca) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	/* Not Used */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */

	ROM_REGION( 0x60000, REGION_USER1, 0 )	/* Question ROMs */
	ROM_LOAD( "rom1.rom",     0x00000, 0x8000, CRC(90b7785f) SHA1(7fc5aa2be868b87ffb9e957c204dabf1508e212e) )
	ROM_LOAD( "rom2.rom",     0x08000, 0x8000, CRC(60172d77) SHA1(92cb2312c6f3395f7ddb45e58695dd000d6ec756) )
	ROM_LOAD( "rom3.rom",     0x10000, 0x8000, CRC(a2207320) SHA1(18ad94b62e7e611ab8a1cbf2d2c6576b8840da2f) )
	ROM_LOAD( "rom4.rom",     0x18000, 0x8000, CRC(5a74c1f9) SHA1(0c55a27a492099ac98daefe0c199761ab1ccce82) )
	ROM_LOAD( "rom5.rom",     0x20000, 0x8000, CRC(93bc1080) SHA1(53e40b8bbc82b3be044f8a39b71fbb811e9263d8) )
	ROM_LOAD( "rom6.rom",     0x28000, 0x8000, CRC(eea2423f) SHA1(34de5495061be7d498773f9a723052c4f13d4a0c) )
	ROM_LOAD( "rom7.rom",     0x30000, 0x8000, CRC(96694055) SHA1(64ebbd85c2a60936f60345b5d573cd9eda196d3f) )
	ROM_LOAD( "rom8.rom",     0x38000, 0x8000, CRC(e68ebf8e) SHA1(cac17ac0231a0526e7f4a58bcb2cae3d05727ee6) )
	ROM_LOAD( "rom9.rom",     0x40000, 0x8000, CRC(fd20921d) SHA1(eedf93841b5ebe9afc4184e089d6694bbdb64445) )
	ROM_LOAD( "rom10.rom",    0x48000, 0x8000, CRC(5091b951) SHA1(224b65795d11599cdbd78e984ac2c71e8847041c) )
	ROM_LOAD( "rom11.rom",    0x50000, 0x8000, CRC(705128db) SHA1(92d45bfd09f61a1a3ac46c2e0ec1f634f04cf049) )
	ROM_LOAD( "rom12.rom",    0x58000, 0x8000, CRC(74c776e7) SHA1(03860d90461b01df4b734b784dddb20843ba811a) )
ROM_END


ROM_START( drivfrcp )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "drivforc.1",   0x0000, 0x1000, CRC(10b59d27) SHA1(fa09f3b95319a3487fa54b72198f41211663e087) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x4000, 0x1000 )
	ROM_CONTINUE(			  0x6000, 0x1000 )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "drivforc.2",   0x0000, 0x1000, CRC(56331cb5) SHA1(520f2a18ebbdfb093c8e4d144749a3f5cbce19bf) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x1000, 0x1000 )
	ROM_CONTINUE(			  0x3000, 0x1000 )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "drivforc.pr1", 0x0000, 0x0020, CRC(045aa47f) SHA1(ea9034f441937df43a7c0bdb502165fb27d06635) )
	ROM_LOAD( "drivforc.pr2", 0x0020, 0x0100, CRC(9e6d2f1d) SHA1(7bcbcd4c0a40264c3b0667fc6a39ed4f2a86cafe) )
ROM_END


ROM_START( 8bpm )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "8bpmp.bin",    0x0000, 0x1000, CRC(b4f7eba7) SHA1(9b15543895c70f5ee2b4f91b8af78a884453e4f1) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x4000, 0x1000 )
	ROM_CONTINUE(			  0x6000, 0x1000 )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8bpmc.bin",    0x0000, 0x1000, CRC(1c894a6d) SHA1(04e5c548290095d1d0f873b6c2e639e6dbe8ff35) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x1000, 0x1000 )
	ROM_CONTINUE(			  0x3000, 0x1000 )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "8bpm.pr1", 0x0000, 0x0020, NO_DUMP )
	ROM_LOAD( "8bpm.pr2", 0x0020, 0x0100, NO_DUMP )
	/* used only to display something until we have the dumps */
	ROM_LOAD( "drivforc.pr1", 0x0000, 0x0020, CRC(045aa47f) SHA1(ea9034f441937df43a7c0bdb502165fb27d06635) )
	ROM_LOAD( "drivforc.pr2", 0x0020, 0x0100, CRC(9e6d2f1d) SHA1(7bcbcd4c0a40264c3b0667fc6a39ed4f2a86cafe) )
ROM_END


ROM_START( porky )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )	/* 32k for code */
	ROM_LOAD( "pp",			  0x0000, 0x1000, CRC(00592624) SHA1(41e554178a89b95bed1f570fab28e2a04f7a68d6) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x4000, 0x1000 )
	ROM_CONTINUE(			  0x6000, 0x1000 )

	/* what is it used for ? */
	ROM_REGION( 0x4000, REGION_USER1, 0 )
	ROM_LOAD( "ps",			  0x0000, 0x4000, CRC(2efb9861) SHA1(8c5a23ed15bd985af78a54d2121fe058e53703bb) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pc",			  0x0000, 0x1000, CRC(a20e3d39) SHA1(3762289a495d597d6b9540ea7fa663128a9d543c) )
	ROM_CONTINUE(			  0x2000, 0x1000 )
	ROM_CONTINUE(			  0x1000, 0x1000 )
	ROM_CONTINUE(			  0x3000, 0x1000 )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "7f",			  0x0000, 0x0020, CRC(98bce7cc) SHA1(e461862ccaf7526421631ac6ebb9b09cd0bc9909) )
	ROM_LOAD( "4a",			  0x0020, 0x0100, CRC(30fe0266) SHA1(5081a19ceaeb937ee1378f3374e9d5949d17c3e8) )
ROM_END


ROM_START( abortman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "namcopac.6e", 0x0000, 0x1000, CRC(fee263b3) )
ROM_LOAD( "namcopac.6f", 0x1000, 0x1000, CRC(39d1fc83) )
ROM_LOAD( "namcopac.6h", 0x2000, 0x1000, CRC(02083b03) )
ROM_LOAD( "namcopac.6j", 0x3000, 0x1000, CRC(7a36fe55) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e", 0x0000, 0x1000, CRC(1c4ef687) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f", 0x0000, 0x1000, CRC(38a22ac4) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) )
ROM_END
 
ROM_START( baby2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e", 0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f", 0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h", 0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j", 0x3000, 0x1000, CRC(260b87b8) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e", 0x0000, 0x1000, CRC(6f7d8d57) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f", 0x0000, 0x1000, CRC(b6d77a1e) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( baby3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e", 0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f", 0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h", 0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j", 0x3000, 0x1000, CRC(6c0e22c8) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e", 0x0000, 0x1000, CRC(22174b65) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f", 0x0000, 0x1000, CRC(b6d77a1e) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( baby4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e", 0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f", 0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h", 0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j", 0x3000, 0x1000, CRC(f8cd7ebb) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e", 0x0000, 0x1000, CRC(22174b65) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f", 0x0000, 0x1000, CRC(b6d77a1e) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( brakman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(2d2af2e5) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(a004abe7) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(30ed6264) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( bucaneer )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "6e.cpu",      0x0000, 0x1000, CRC(fee263b3) )
ROM_LOAD( "6f.cpu",    0x1000, 0x1000, CRC(39d1fc83) )
ROM_LOAD( "6h.cpu",      0x2000, 0x1000, CRC(197443f8) )
ROM_LOAD( "6k.cpu",      0x3000, 0x1000, CRC(c4d9169a) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e.cpu",      0x0000, 0x0800, CRC(4060c077) )
ROM_LOAD( "5h.cpu",      0x0800, 0x0800, CRC(e3861283) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f.cpu",      0x0000, 0x0800, CRC(09f66dec) )
ROM_LOAD( "5k.cpu",      0x0800, 0x0800, CRC(653314e7) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	
ROM_END

ROM_START( caterpil )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(b53c5650) )
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(53845efb) )
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(a474357e) )
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(5df0ea3b) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",    0x0000, 0x1000, CRC(0387907d) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",    0x0000, 0x1000, CRC(aa450714) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( chtmsatk )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954) )
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	
ROM_END

ROM_START( chtmspa )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0af09d31) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( chtpac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(72cea888) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( chtpman2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5) )
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(72cea888) )
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8) )
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(0e3bdbde) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",    0x0000, 0x1000, CRC(0c944964) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",    0x0000, 0x1000, CRC(958fedf9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( chtpop )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(72cea888) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(af2b610b) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( chtpuck )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5) )
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(72cea888) )
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8) )
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(2e64a3ba) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",    0x0000, 0x1000, CRC(0c944964) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",    0x0000, 0x1000, CRC(958fedf9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( cookiem )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(3c95b06b) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(527859fb) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(66e397fd) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(a96761a6) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(ffb4de27) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(1b5875ac) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( crashh )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(04353b41) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(e03205c0) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(b0fa8e46) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(bfa4d2fe) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(12f2f224) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(817d94e3) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(0a25969b) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(447ea79c) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(2bc5d339) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( crazypac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(9f754a51) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( dizzy )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(bf05bf15) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(f999a8d9) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(7cb7b9b1) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( eltonpac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9c093e5b) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2825641) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( europac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(a4e82007) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(00208da4) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(0d36dabf) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( fasthear )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(a8d6c227) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(28eb876c) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(50fc9966) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( fastmspa )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(a8d6c227) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( fastplus )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
/*	ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31) ) */
ROM_LOAD( "boot2",   	  0x1000, 0x1000, CRC(a8d6c227) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954) )
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( faststrm )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(a8d6c227) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(b2940b89) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(5c65865f) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( fpnleash )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(eddb20e7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(57fe8b4d) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(94c63bb1) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(fe7734d5) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(ef155ffb) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(70d15899) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "flat.5e",           0x0000, 0x1000, CRC(1f69d1d0) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(98d3d364) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(357c2523) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(5ff3b85a) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( fstmsatk )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(a8d6c227) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954) )
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	
ROM_END

ROM_START( hanglyad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(93fd3682) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	
ROM_END

ROM_START( hanglyb )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "01",           0x0000, 0x0800, CRC(9d027c4a) )
ROM_LOAD( "05",           0x0800, 0x0800, CRC(194c7189) )
ROM_LOAD( "02",           0x1000, 0x0800, CRC(5ba228bb) )
ROM_LOAD( "06",           0x1800, 0x0800, CRC(70810ccf) )
ROM_LOAD( "03",           0x2000, 0x0800, CRC(08419c4a) )
ROM_LOAD( "07",           0x2800, 0x0800, CRC(3f250c58) )
ROM_LOAD( "04",           0x3000, 0x0800, CRC(603b70b7) )
ROM_LOAD( "08",           0x3800, 0x0800, CRC(71293b1f) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "09",           0x0000, 0x0800, CRC(c62bbabf) )
ROM_LOAD( "11",           0x0800, 0x0800, CRC(777c70d3) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "10",         0x0000, 0x0800, CRC(ca8c184c) )
ROM_LOAD( "12",           0x0800, 0x0800, CRC(7dc75a81) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( heartbn2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(28eb876c) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(50fc9966) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( heartbrn )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5431d4c4) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(ceb50654) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( hearts )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5be0a4e4) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( hm1000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(16d9f7ce) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(fd06d923) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hm2000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(cba39624) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(a2b31528) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmba5000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5ebb61c5) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(d9a055aa) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmba7000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7961ed4e) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(417080fd) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmbabymz )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9a8cd8a1) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(f0bd55bd) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmbluep )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(16d9f7ce) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ceb721be) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmgrenp )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5807a438) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(1fcc0cf2) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hmplus )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(022c35da) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(7a444bda) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jacman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(1f52ef8b) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(809bd73e) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(0509d3e6) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( mazeman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5) )
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(77e44852) )
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(6d16a528) )
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(3c3ff208) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",    0x0000, 0x1000, CRC(38870a32) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",    0x0000, 0x1000, CRC(1fd18b80) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( mrpacman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(65c0526c) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(4078d9b2) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(c1637f1c) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(83e6a3a9) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(045dadf0) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )
ROM_END

ROM_START( ms1000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(22a10392) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(adde7864) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(5cb008a9) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( ms2000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(a72e49df ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(2bcce741 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(4b40032a ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( ms2600 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(c2a293df ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(4852db05 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(f32b076f ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(6b07b97a ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(f844608b ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( ms3000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(46790576 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(1708299b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(cf4798e3 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( ms4000p )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(ff418db8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(05ed8a9f ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(c0b58e02 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( ms5000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(a72e49df ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(fd1bd093 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(07d39d56 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( ms5000p )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(d7ff3b21 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(4fd847aa ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(50d5fa64 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msatk2ad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "mspacatk.1",   0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "mspacatk.3",   0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "mspacatk.4",   0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(5a72a7a6 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(bd0c3e94 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(dc70ed2e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(643d5523 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msatkad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(dc70ed2e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(643d5523 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( msbaby )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(17aea21d ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(0c08aa7f ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(e28aaa1f ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(e764738d ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(3d316ad0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msbaby1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(17aea21d ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(0c08aa7f ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(e28aaa1f ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(e764738d ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(3d316ad0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msberzk )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(795b1d20 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(9fc2a679 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(6749a32d ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(92692429 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(7c32ed2e ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mscrzyma )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(f2b17834 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(43936999 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(a15b524e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(2bcce741 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(2a9d6f77 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( mscrzymp )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(db440183 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(43936999 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(a15b524e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(d43a7194 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(7af54907 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msdroid )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(984b3e88 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(97f5dd55 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(c0e47314 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msdstorm )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(b2940b89 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(5c65865f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mselton )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(30d0b19f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msextra )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(b13d0521 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(b656c0c5 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(c5d7017b ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msextrap )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(728f7465 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(1a2fa3e2 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(5a400cbc ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msf1pac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e255428b ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(c7183fd2 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(a355e061 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msgrkatk )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(7143729f ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(b6526896 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(043b1c9b ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msgrkb )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(7143729f ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(b6526896 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(043b1c9b ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mshangly )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( mshearts )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(0b5cf081 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msindy )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(8e234b3a ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(c7183fd2 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(a355e061 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msmini )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(7787a3f6 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(98ab3c44 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( msminia )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(7787a3f6 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(98ab3c44 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( msmspac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(1c11c60f ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(a0ffcd08 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msmulti )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(cfb721a8 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(e55d5230 ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(29ba4b53 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(5a4b59dc ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(3b939b12 ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(46cdda4a ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msnes4a )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(d16b9605 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(e6b58707 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(22e5b64e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes62 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(52120bc9 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(8fd36e02 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(d3a28769 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes63 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(f4ea7d2c ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(1ea43bba ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(d3a28769 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes6m )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(bc091971 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(23ebe9a1 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(3abb116b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes6m2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(b8f8edda ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(5d7888bc ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(3abb116b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes6m3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(20833779 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(a047e68f ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(3abb116b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msnes6m4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(23c0db46 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(daafdb39 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(22e5b64e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspac6m )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "1.cpu",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "2.cpu",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "3.cpu",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "4.cpu",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "52.cpu",       0x8000, 0x1000, CRC(d0bd89d9 ))
ROM_LOAD( "62.cpu",       0x9000, 0x1000, CRC(c5662407 ))
ROM_REGION( 0x2000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "7.cpu",           0x0000, 0x0800, CRC(2850148a ))
ROM_LOAD( "9.cpu",           0x0800, 0x0800, CRC(6c3c6ebb ))
ROM_REGION( 0x2000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "8.cpu",           0x0000, 0x0800, CRC(5596b345 ))
ROM_LOAD( "10.cpu",          0x0800, 0x0800, CRC(50c7477d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "pacplus.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(c3dca510 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(57cb31e3 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacat2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "mspacatk.1",   0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "mspacatk.3",   0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "mspacatk.4",   0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(5a72a7a6 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(bd0c3e94 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacatb )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5b",   0x8000, 0x1000, CRC(5a72a7a6 ))
ROM_LOAD( "mspacatk.6b",   0x9000, 0x1000, CRC(7a8c4dfe ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacdel )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(af8c63ad ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(605becd7 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(435de4cb ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( mspacnes )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(43936999 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(a15b524e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(d3a28769 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(6a4db98d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacp )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(203dcba5 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(cd4afa90 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(58ea207f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( mspacren )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(efe80423 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(f513ae17 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(ce3b842b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(31b134a0 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(022f61d1 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(7b6aa3c9 ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(64fe1dbf ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspacrip )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(5fb1ee61 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5aa9eb83 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(ceb3785d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mspamsba )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(8b3f8e44 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(aa7115b8 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(f94b0961 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( mspc6mad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(b556fe78 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(4dbe1939 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(92ea9860 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(713cc17e ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( msplus )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(203dcba5 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(cd4afa90 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(58ea207f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msrumble )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(15b02313 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(e2473a86 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(17abb097 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( mssilad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(79d241dd ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(2e2fcde0 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(22c6d19a ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(dc70ed2e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(643d5523 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msstrmaz )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(ef9cce5a ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(b8f8edda ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(5d7888bc ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(14fd0ad7 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(67fb6254 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( msultra )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(36b5e9fd ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(cf609615 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(007cd365 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(508af5b0 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(db5a4630 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msvctr6m )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(513f4d5c ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(e21c81ff ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(d0bd89d9 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(c5662407 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(64b35a5a ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(57cb31e3 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( msvectr )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(56560aef ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(57cb31e3 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( msyakman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(827892bb ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(1108ba96 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( namcosil )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3 ))
ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(c5ec2352 ))
ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(02083b03 ))
ROM_LOAD( "namcopac.6j",  0x3000, 0x1000, CRC(57a07f6e ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "namcopac.5e",  0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "namcopac.5f",  0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( newpuck2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5 ))
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8 ))
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(0e3bdbde ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",   0x0000,0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",   0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pac2600 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(0a27d11d ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(ab23fe0c ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(87d8f5ce ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(91bd671f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END


ROM_START( pacatk )
ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64k for code+64k for decrypted code */
ROM_LOAD( "pacman.6e", 0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f", 0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h", 0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j", 0x3000, 0x1000, CRC(817d94e3 ))
ROM_LOAD( "u5", 0x8000, 0x0800, CRC(f45fbbcd ))
ROM_LOAD( "u6pacatk", 0x9000, 0x1000, CRC(a35b3788 ))
ROM_LOAD( "u7", 0xb000, 0x1000, CRC(c82cd714 ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e", 0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f", 0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound PROMs */
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66 )) /* timing - not used */
ROM_END

ROM_START( pacbaby )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac.6j",    0x3000, 0x1000, CRC(4e4ac798 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacbaby2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac2.6j",  0x3000, 0x1000, CRC(742453ba ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacbaby3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac3.6j",    0x3000, 0x1000, CRC(b82f2b73 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacbell )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(4b428215 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacelec )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(9e5c763d ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacinvis )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(1b4e96dc ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacjail )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(cc31f185 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacjr1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(fcfb21a3 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( pacjr2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(cc26c905 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( pacjr3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(d4d8cb9b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( pacjr4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(42ca024b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( pacm255 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacman3d )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(96364259 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(959e930e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(aa203d45 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(d1830540 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacman6 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(de753fd9 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(46cdda4a ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmar )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "pacman.6e",   0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",   0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",   0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",   0x3000, 0x1000, CRC(b208c7dc ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",   0x0000, 0x1000, CRC(7ccdfa59 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",   0x0000, 0x1000, CRC(51af9fc5 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacman.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacman.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( pacmini )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(5e04f9c5 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5520b393 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(30e6024c ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmini2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(092dd5fa ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(130eff7b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(30e6024c ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmn6m2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(aadee235 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(60ada878 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacms1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c5da3887 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacms2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(b0a107ea ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacms3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c7d8325a ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacms4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(7b63ff9b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmsa1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(07163b97 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmsa2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(15e83c24 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmsa3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(5762f9cf ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmsa4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(d9c2a669 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7394868b ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(ce1a3264 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( pacmulti )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacmulti.6e",    0x0000, 0x1000, CRC(cfb721a8 ) )
ROM_LOAD( "pacmulti.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacmulti.6h",    0x2000, 0x1000, CRC(e55d5230 ) )
ROM_LOAD( "pacmulti.6j",    0x3000, 0x1000, CRC(29ba4b53 ) )
ROM_LOAD( "pacmulti.8",     0x8000, 0x1000, CRC(5a4b59dc ) )
ROM_LOAD( "pacmulti.9",     0x9000, 0x1000, CRC(3b939b12 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacmulti.5e",    0x0000, 0x1000, CRC(46cdda4a ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacmulti.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacn255 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3 ) )
ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(39d1fc83 ) )
ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(6dac9e8f ) )
ROM_LOAD( "namcopac.6j",  0x3000, 0x1000, CRC(7a36fe55 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacplusc )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacplusc.rom",   0x0000, 0x4000, CRC(a0d25591 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",    0x0000, 0x1000, CRC(022c35da ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",    0x0000, 0x1000, CRC(4de65cdd ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",    0x0000, 0x0020, CRC(063dd53a ) )
ROM_LOAD( "pacplus.4a",    0x0020, 0x0100, CRC(e271a166 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacpopey )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(af2b610b ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pacshuf )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(55cc4d87 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacspeed )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(0e7946d1 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacstrm )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(720dc3ee ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(b2940b89 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(5c65865f ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pacweird )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ) )
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ) )
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(92fac825 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(11077f3e ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(7c213ab4 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pcrunchy )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c21c6c52 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pengman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(37e7f3ba ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(02f5f5e6 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(5cd3db2e ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( petshop )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(75230eef ) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(ac1c180b ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(ce314b46 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( piranha2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(fe8b786d ) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(bef08ea4 ) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(fd3df829 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(8f0fa58f ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(42153903 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm1000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(16d9f7ce ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(fd06d923 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm2000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(cba39624 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(a2b31528 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm3000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(adb41be2 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(1fac6af1 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm4000p )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(adb41be2 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(74f6f4d3 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm5000p )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(5865ce98 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(81cf02e1 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(063dd53a ) )
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(e271a166 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm6000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(89945690 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(9268eb5a ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm7000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(c181ad5d ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(bd31be36 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pm7000p )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(c181ad5d ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(3d5861a5 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(dd2eb43d ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(ff3f4866 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5b794d59 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(59a9362c ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmad_a )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(dd2eb43d ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(24e43519 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmad00 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(dd2eb43d ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(5f1aea94 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(315f7131 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(157a6c0f ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmad6m )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ) )
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ) )
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(d6a0d7f2 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(93fd3682 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(c2310808 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmada )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(dd2eb43d ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(24e43519 ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9f3c32d4 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c2310808 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmanalt )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "namcopac.6e",    0x0000, 0x1000, CRC(fee263b3 ) )
ROM_LOAD( "namcopac.6f",    0x1000, 0x1000, CRC(c5ec2352 ) )
ROM_LOAD( "namcopac.6h",    0x2000, 0x1000, CRC(02083b03 ) )
ROM_LOAD( "namcopac.6j",    0x3000, 0x1000, CRC(57a07f6e ) )
/*
ROM_REGION_DISPOSE(0x2000)
ROM_LOAD( "namcopac.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_LOAD( "namcopac.5f",    0x1000, 0x1000, CRC(958fedf9 ) )
*/
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "namcopac.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "namcopac.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmanaltm )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(fee263b3 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(c5ec2352 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(02083b03 ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(57a07f6e ) )
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ) )
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )
ROM_END

ROM_START( pmba2000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(f7d647d8 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(ff6308bb ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(c8b456b5 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmba3000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(08f7a148 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(5ebb61c5 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(d9a055aa ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmba4000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(5ebb61c5 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(d9a055aa ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmba6000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(bdb3d11a ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(7961ed4e ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(417080fd ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(063dd53a ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(e271a166 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmba8000 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(7961ed4e ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(417080fd ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(063dd53a ) )
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(e271a166 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmbamaz )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ) )
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ) )
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ) )
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(bf5e4b89 ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(9a8cd8a1 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(13aeaac4 ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmbaplus )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ) )
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ) )
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ) )
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(3e5a31c2 ) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(3c099bbd ) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ) )
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ) )	/* timing - not used */
ROM_END

ROM_START( pmbluep )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(16d9f7ce ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(2da4dec7 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmdeluxe )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(cba39624 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(ab172fc9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmextra )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(3ee34b66 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(edf92a4c ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmextrap )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(3ee34b66 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(34ff1749 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmfever )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(f9b955e1 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(8ddf8eb6 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmfeverp )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(5ebb61c5 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(d9a055aa ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pmgrenp )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, CRC(5807a438 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",   0x0000, 0x1000, CRC(1545b3c8 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( pplusad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",    0x0000, 0x1000, CRC(ce684d85 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",    0x0000, 0x1000, CRC(0b86d002 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "pacplus.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( puckman2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5 ))
ROM_LOAD( "puckman.6f",   0x1000, 0x1000, CRC(720dc3ee ))
ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8 ))
ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(0e3bdbde ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "puckman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( puckren )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(0f5ecf67 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(70ea1149 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(c6bb6ea6 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(dc90f1dc ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(a502e25f ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(18d44098 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(7b6aa3c9 ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(64fe1dbf ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( puckrenc )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(0f5ecf67 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(8a75a7e2 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(c6bb6ea6 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(dc90f1dc ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(a502e25f ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(18d44098 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(7b6aa3c9 ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(64fe1dbf ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( punleash )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(eddb20e7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(57fe8b4d ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(94c63bb1 ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(fe7734d5 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(ef155ffb ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(70d15899 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(713a4ba4 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(98d3d364 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(357c2523 ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(5ff3b85a ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( roboman )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(a909673e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(e386213e ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(d8db5788 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( snakeyes )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(7e506e78 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(46cdda4a ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( snowpac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(4f9e1d02 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(2cc60a94 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(41ba87bd ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( sueworld )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(677adea7 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(6c975345 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(2c809842 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( sumelton )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c1fe2c7d ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(10894f38 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(debef8a9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( tbone )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(e7377f2d ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(afd44c48 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( ultra2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(83da903e ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( ultrapac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(84c6c23b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(be212894 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(d46efbcf ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vcrunchy )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c21c6c52 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vecbaby )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac.6j",    0x3000, 0x1000, CRC(4e4ac798 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vecbaby2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac2.6j",    0x3000, 0x1000, CRC(742453ba ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vecbaby3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "babypac3.6j",    0x3000, 0x1000, CRC(b82f2b73 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectplus )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68 ))
ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556 ))
ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430 ))
ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5e",    0x0000, 0x1000, CRC(330abcc6 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacplus.5f",    0x0000, 0x1000, CRC(61da12cf ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "pacplus.7f",    0x0000, 0x0020, CRC(063dd53a ))
ROM_LOAD( "pacplus.4a",    0x0020, 0x0100, CRC(e271a166 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectr6m )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(de753fd9 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(f7be09ba ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectr6tb )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman6.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman6.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman6.6h",    0x2000, 0x1000, CRC(aee79ea1 ))
ROM_LOAD( "pacman6.6j",    0x3000, 0x1000, CRC(581ae70b ))
ROM_LOAD( "pacmaps.rom",   0x8000, 0x1000, CRC(bdbbb207 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5e",    0x0000, 0x1000, CRC(f7be09ba ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman6.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectratk )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "mspacatk.1",   0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31 ))
ROM_LOAD( "mspacatk.3",   0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "mspacatk.4",   0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954 ))
ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308 ))
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(56560aef ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(57cb31e3 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectrpac )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vectxens )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(64a10b6c ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vhangly )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a ))
ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586 ))
ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f ))
ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

ROM_START( vpacbell )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(4b428215 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacelec )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(9e5c763d ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacjail )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(cc31f185 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacms1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c5da3887 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacms2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(b0a107ea ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacms3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(c7d8325a ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacms4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(7b63ff9b ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacmsa1 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(07163b97 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacmsa2 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(15e83c24 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacmsa3 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(5762f9cf ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacmsa4 )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(d9c2a669 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(035f0597 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(306b24f0 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpacshuf )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(55cc4d87 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( vpspeed )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(d3640977 ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(0e7946d1 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(936d0475 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(022d0686 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( xensad )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(64a10b6c ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(315f7131 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(157a6c0f ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( xensrev )
ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10 ))
ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4 ))
ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb ))
ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(64a10b6c ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(40e3522d ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))
ROM_END

ROM_START( lazybug )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) SHA1(bc2247ec946b639dd1f00bfc603fa157d0baaa97) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) SHA1(13ea0c343de072508908be885e6a2a217bbb3047) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) SHA1(5ea4d907dbb2690698db72c4e0b5be4d3e9a7786) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) SHA1(3022a408118fa7420060e32a760aeef15b8a96cf) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) SHA1(fed6e9a2b210b07e7189a18574f6b8c4ec5bb49b) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) SHA1(387010a0c76319a1eab61b54c9bcb5c66c4b67a1) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( wavybug )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) SHA1(bc2247ec946b639dd1f00bfc603fa157d0baaa97) )
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) SHA1(13ea0c343de072508908be885e6a2a217bbb3047) )
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) SHA1(5ea4d907dbb2690698db72c4e0b5be4d3e9a7786) )
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) SHA1(3022a408118fa7420060e32a760aeef15b8a96cf) )
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) SHA1(fed6e9a2b210b07e7189a18574f6b8c4ec5bb49b) )
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) SHA1(387010a0c76319a1eab61b54c9bcb5c66c4b67a1) )
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01) SHA1(5e8b472b615f12efca3fe792410c23619f067845) )
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909) SHA1(fd6a1dde780b39aea76bf1c4befa5882573c2ef4) )
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) SHA1(8d0268dee78e47c712202b0ec4f1f51109b1f2a5) )
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) SHA1(19097b5f60d1030f8b82d9f1d3a241f93e5c75d6) )
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( zolamaze )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	
ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7 ))
ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e ))
ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b ))
ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6 ))
ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165 ))
ROM_REGION( 0x1000, REGION_GFX1 , ROMREGION_DISPOSE )
ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5c281d01 ))
ROM_REGION( 0x1000, REGION_GFX2 , ROMREGION_DISPOSE )
ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END


ROM_START( horizpac )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(7CC1979D) SHA1(557346859DCFCE806E83DCF0DA1FE60DA20FED81) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(14202B73) SHA1(136B860D6AC8EFB9C83B67F77FBB47C3014D4ADC) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821EE0B) SHA1(5EA4D907DBB2690698DB72C4E0B5BE4D3E9A7786) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(9A17E8DA) SHA1(2696643705878207952C4559C176D486BD0027C2) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8C3E6DE6) SHA1(FED6E9A2B210B07E7189A18574F6B8C4EC5BB49B) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368CB165) SHA1(387010A0C76319A1EAB61B54C9BCB5C66C4B67A1) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(0572335D) SHA1(F2D8BB3124970B298026E841E057FBCC7CDFC4B1) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(4198511D) SHA1(B3808062835AA0CC05FA19A678A5C8AB8FB646D7) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( at )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(0EC135BE) SHA1(F2BA6F8EE4256420E6149853A34A2B11A3545EEA) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1B95F321) SHA1(F9D76544CCE43CE779A1BF59A01AC00297BAA82C) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(5892650A) SHA1(7C63F42EA3F75FAA15C6F5864569CB6A8F8C7C79) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(8612016A) SHA1(4E144069F194D240E503B24DF9B10BCDE68B2C6C) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(EEB359BF) SHA1(36F153B5CE53475FD144AF50E4BD67B0E0B3A01F) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958FEDF9) SHA1(4A937AC02216EA8C96477D4A15522070507FB599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( seq1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(7063B724) SHA1(3A291D26BDFBF5C895D5F6AA70FC164299E8D9F1) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(95DB4723) SHA1(A0C47EA05E14CEBC6493705CD2D46D1E3D12B23A) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(EEB359BF) SHA1(36F153B5CE53475FD144AF50E4BD67B0E0B3A01F) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958FEDF9) SHA1(4A937AC02216EA8C96477D4A15522070507FB599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( pachello )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(A8227949) SHA1(793B7BA579C7E4771094D281C2589435A1BF2FDA) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(D5690D97) SHA1(51A4D63265F1D848359CA8C78BA111D72A3BC2CF) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(EEB359BF) SHA1(36F153B5CE53475FD144AF50E4BD67B0E0B3A01F) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958FEDF9) SHA1(4A937AC02216EA8C96477D4A15522070507FB599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( pacmatri )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(29B0497E) SHA1(E45B225AABDF2F0549718885C02AE8A8EEF3BAEB) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(EAA7B145) SHA1(4C0ABF30F2C962B6EB2BDDA833236B9D58544A89) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(EEB359BF) SHA1(36F153B5CE53475FD144AF50E4BD67B0E0B3A01F) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958FEDF9) SHA1(4A937AC02216EA8C96477D4A15522070507FB599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( alpaca7 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(AF4AFCBF) SHA1(F11E2FE309818B41CB2A28408B06D18419879C09) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(3E879F02) SHA1(0B084DD449E57476231E59F15F85A209A919959C) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(A6103847) SHA1(4376996FF8C19AFD65F1757CE159B70071A4BD3B) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( alpaca8 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(86027944) SHA1(C47FC62522A3BAE0D49F4B68C218F73C43ED19B5) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(C71C0011) SHA1(1CEAF73DF40E531DF3BFB26B4FB7CD95FB7BFF1D) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(3E879F02) SHA1(0B084DD449E57476231E59F15F85A209A919959C) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(856E53AE) SHA1(95460212107B3371600569DBD4DA482EC631ABDB) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(2BC5D339) SHA1(446E234DF94D9EF34C3191877BB33DD775ACFDF5) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( pacsnoop )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(FA536307) SHA1(55742123DE6EF87E1DEEF31DC75744B98129AA9E) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(30EA615C) SHA1(AFB953167AA8060DC60173F3DE065C07FD933CC1) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(25DA6440) SHA1(93414E82442044324AC5475A45FB124DA70E5D6F) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(A9594951) SHA1(5C4FA744FDCC3B0EAD7900B079A13297D16A996F) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, CRC(0C944964) SHA1(06EF227747A440831C9A3A613B76693D52A2F0A9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f",    0x0000, 0x1000, CRC(958FEDF9) SHA1(4A937AC02216EA8C96477D4A15522070507FB599) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END


ROM_START( ramsnoop )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(A08529FA) SHA1(E448E3A2BED42FA9A2C6435849426E6A2CCF5290) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(27B70DD6) SHA1(E52C724C7909C2B3B8EA44C2D3540BE9D23C5E03) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(812691A0) SHA1(8FF6D1C2697732C0466BCB37A7F36392980B4A99) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(9E7849E3) SHA1(8C574E42BFFC6B88D5B5DF3C6655F792B2FF5C5C) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8C3E6DE6) SHA1(FED6E9A2B210B07E7189A18574F6B8C4EC5BB49B) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368CB165) SHA1(387010A0C76319A1EAB61B54C9BCB5C66C4B67A1) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e",           0x0000, 0x1000, CRC(5C281D01) SHA1(5E8B472B615F12EFCA3FE792410C23619F067845) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x1000, CRC(615AF909) SHA1(FD6A1DDE780B39AEA76BF1C4BEFA5882573C2EF4) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2FC650BD) SHA1(8D0268DEE78E47C712202B0EC4F1F51109B1F2A5) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3EB3A8E4) SHA1(19097B5F60D1030F8B82D9F1D3A241F93E5C75D6) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(A9CC86BF) SHA1(BBCEC0570AECEB582FF8238A4BC8546A23430081) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245B66) SHA1(0C4D0BEE858B97632411C440BEA6948A74759746) )	/* timing - not used */
ROM_END



ROM_START( rocktrv2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.aux",        0x0000, 0x4000, CRC(d182947b) SHA1(b778658386b2ed7c9f518cf20d7805ea62ae727b) )
	ROM_LOAD( "2.aux",        0x6000, 0x2000, CRC(27a7461d) SHA1(0cbd4a03dcff352fbd6b9a9009dc908e34553ee2) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5e.cpu",       0x0000, 0x1000, CRC(0a6cc43b) SHA1(a773bf3dda326797d63ceb908ad4d48f516bcea0) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	/* Not Used */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "7f.cpu",       0x0000, 0x0020, CRC(7549a947) SHA1(4f2c3e7d6c38f0b9a90317f91feb3f86c9a0d0a5) )
	ROM_LOAD( "4a.cpu",       0x0020, 0x0100, CRC(ddd5d88e) SHA1(f28e1d90bb495001c30c63b0ef2eec45de568174) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "82s126.3m"  ,  0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */

	ROM_REGION( 0x40000, REGION_USER1, 0 )	/* Question ROMs */
	ROM_LOAD( "3.aux",        0x00000, 0x4000, CRC(5b117ca6) SHA1(08d625312a751b99e132b90dcf8274d0ff2aecf2) )
	ROM_LOAD( "4.aux",        0x04000, 0x4000, CRC(81bfd4c3) SHA1(300cb4a38d3a1234bfc793f0574527033697f5a2) )
	ROM_LOAD( "5.aux",        0x08000, 0x4000, CRC(e976423c) SHA1(53a7f100943313014285ce09c03bd3eabd1388b0) )
	ROM_LOAD( "6.aux",        0x0c000, 0x4000, CRC(425946bf) SHA1(c8b0ba85bbba2f2c33f4ba069bf2fbb9692281d8) )
	ROM_LOAD( "7.aux",        0x10000, 0x4000, CRC(7056fc8f) SHA1(99c18ba4cd4d45531066069d2fd5018177072d5b) )
	ROM_LOAD( "8.aux",        0x14000, 0x4000, CRC(8b86464f) SHA1(7827df4c763fe078d3844eafab728e9400275049) )
	ROM_LOAD( "9.aux",        0x18000, 0x4000, CRC(17d8eba4) SHA1(806593824868e266c776e2e49cebb60dd6f8302e) )
	ROM_LOAD( "10.aux",       0x1c000, 0x4000, CRC(398c8eb4) SHA1(2cbbb11e255b84a54621f5fccfa8354bf925f1df) )
	ROM_LOAD( "11.aux",       0x20000, 0x4000, CRC(7f376424) SHA1(72ba5b01053c0c568562ba7a1257252c47736a3c) )
	ROM_LOAD( "12.aux",       0x24000, 0x4000, BAD_DUMP CRC(8d5bbf81) SHA1(0ebc9afbe6df6d60cf8797e246dda45694dca89e) )
	ROM_LOAD( "13.aux",       0x28000, 0x4000, CRC(99fe2c21) SHA1(9ff29cb2b74a16f5249677172b9d96e11241032e) )
	ROM_LOAD( "14.aux",       0x2c000, 0x4000, CRC(df4cf5e7) SHA1(1228a31b9053ade416a33f699f3f5513d1e47b24) )
	ROM_LOAD( "15.aux",       0x30000, 0x4000, CRC(2a32de26) SHA1(5892d4aea590d109339a66d15ebedaa04629fa7e) )
	ROM_LOAD( "16.aux",       0x34000, 0x4000, CRC(fcd42187) SHA1(e99e1f281eff2f6f42440f30bcb7a5efe34590fd) )
	ROM_LOAD( "17.aux",       0x38000, 0x4000, CRC(24d5c388) SHA1(f7039d84b3cbf00884e87ea7221f1b608a7d879e) )
	ROM_LOAD( "18.aux",       0x3c000, 0x4000, CRC(feb195fd) SHA1(5677d31e526cc7752254e9af0d694f05bc6bc907) )
ROM_END

/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void maketrax_rom_decode(void)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	/* patch protection using a copy of the opcodes so ROM checksum */
	/* tests will not fail */
	memory_set_opcode_base(0,rom+diff);

	memcpy(rom+diff,rom,diff);

	rom[0x0415 + diff] = 0xc9;
	rom[0x1978 + diff] = 0x18;
	rom[0x238e + diff] = 0xc9;
	rom[0x3ae5 + diff] = 0xe6;
	rom[0x3ae7 + diff] = 0x00;
	rom[0x3ae8 + diff] = 0xc9;
	rom[0x3aed + diff] = 0x86;
	rom[0x3aee + diff] = 0xc0;
	rom[0x3aef + diff] = 0xb0;
}

static DRIVER_INIT( maketrax )
{
	/* set up protection handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5080, 0x50bf, 0, 0, maketrax_special_port2_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x50c0, 0x50ff, 0, 0, maketrax_special_port3_r);

	maketrax_rom_decode();
}

static void korosuke_rom_decode(void)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	/* patch protection using a copy of the opcodes so ROM checksum */
	/* tests will not fail */
	memory_set_opcode_base(0,rom+diff);

	memcpy(rom+diff,rom,diff);

	rom[0x044c + diff] = 0xc9;
	rom[0x1973 + diff] = 0x18;
	rom[0x238c + diff] = 0xc9;
	rom[0x3ae9 + diff] = 0xe6;	// not changed
	rom[0x3aeb + diff] = 0x00;
	rom[0x3aec + diff] = 0xc9;
	rom[0x3af1 + diff] = 0x86;
	rom[0x3af2 + diff] = 0xc0;
	rom[0x3af3 + diff] = 0xb0;
}

static DRIVER_INIT( korosuke )
{
	/* set up protection handlers */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5080, 0x5080, 0, 0, korosuke_special_port2_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x50c0, 0x50ff, 0, 0, korosuke_special_port3_r);

	korosuke_rom_decode();
}

static DRIVER_INIT( ponpoko )
{
	int i, j;
	unsigned char *RAM, temp;

	/* The gfx data is swapped wrt the other Pac-Man hardware games. */
	/* Here we revert it to the usual format. */

	/* Characters */
	RAM = memory_region(REGION_GFX1);
	for (i = 0;i < memory_region_length(REGION_GFX1);i += 0x10)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}

	/* Sprites */
	RAM = memory_region(REGION_GFX2);
	for (i = 0;i < memory_region_length(REGION_GFX2);i += 0x20)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x18];
			RAM[i+j+0x18] = RAM[i+j+0x10];
			RAM[i+j+0x10] = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}
}

static void eyes_decode(unsigned char *data)
{
	int j;
	unsigned char swapbuffer[8];

	for (j = 0; j < 8; j++)
	{
		swapbuffer[j] = data[(j >> 2) + (j & 2) + ((j & 1) << 2)];
	}

	for (j = 0; j < 8; j++)
	{
		char ch = swapbuffer[j];

		data[j] = (ch & 0x80) | ((ch & 0x10) << 2) |
					 (ch & 0x20) | ((ch & 0x40) >> 2) | (ch & 0x0f);
	}
}

static DRIVER_INIT( eyes )
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	/* Data lines D3 and D5 swapped */
	RAM = memory_region(REGION_CPU1);
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] =  (RAM[i] & 0xc0) | ((RAM[i] & 0x08) << 2) |
				  (RAM[i] & 0x10) | ((RAM[i] & 0x20) >> 2) | (RAM[i] & 0x07);
	}


	/* Graphics ROMs */

	/* Data lines D4 and D6 and address lines A0 and A2 are swapped */
	RAM = memory_region(REGION_GFX1);
	for (i = 0;i < memory_region_length(REGION_GFX1);i += 8)
		eyes_decode(&RAM[i]);
	RAM = memory_region(REGION_GFX2);
	for (i = 0;i < memory_region_length(REGION_GFX2);i += 8)
		eyes_decode(&RAM[i]);
}


static DRIVER_INIT( pacplus )
{
	pacplus_decode();
}

static DRIVER_INIT( jumpshot )
{
	jumpshot_decode();
}

static DRIVER_INIT( 8bpm )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	int i;

	/* Data lines D0 and D6 swapped */
	for( i = 0; i < 0x8000; i++ )
	{
		RAM[i] = BITSWAP8(RAM[i],7,0,5,4,3,2,1,6);
	}
}

static DRIVER_INIT( porky )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	int i;

	/* Data lines D0 and D4 swapped */
	for(i = 0; i < 0x8000; i++)
	{
		RAM[i] = BITSWAP8(RAM[i],7,6,5,0,3,2,1,4);
	}

}

static DRIVER_INIT( dremshpr )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4bff, 0, 0, MRA8_RAM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4bff, 0, 0, MWA8_RAM);
}

static DRIVER_INIT( rocktrv2 )
{
	/* hack to pass the rom check for the bad rom */
	UINT8 *ROM = memory_region(REGION_CPU1);

	ROM[0x7ffe] = 0xa7;
	ROM[0x7fee] = 0x6d;
}


/*************************************
 *
 *	Game drivers
 *
 *************************************/

/*          rom       parent    machine   inp       init */
GAME( 1980, puckman,  0,        pacman,   pacman,   0,        ROT90,  "Namco", "PuckMan (Japan set 1)" )
GAME( 1980, puckmana, puckman,  pacman,   pacman,   0,        ROT90,  "Namco", "PuckMan (Japan set 2)" )
GAME( 1980, pacman,   puckman,  pacman,   pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Midway)" )
GAME( 1981, puckmod,  puckman,  pacman,   pacman,   0,        ROT90,  "Namco", "PuckMan (harder?)" )
GAME( 1981, pacmod,   puckman,  pacman,   pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Midway, harder)" )
GAME( 1981, hangly,   puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Hangly-Man (set 1)" )
GAME( 1981, hangly2,  puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Hangly-Man (set 2)" )
GAME( 1981, hangly3,  puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Hangly-Man (set 3)" )
GAME( 1980, newpuckx, puckman,  pacman,   pacman,   0,        ROT90,  "hack", "New Puck-X" )
GAME( 1981, pacheart, puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Pac-Man (Hearts)" )
GAME( 1982, joyman,   puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Joyman" )
GAME( 1980, newpuc2,  puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Newpuc2" )
GAME( 1980, newpuc2b, puckman,  pacman,   pacman,   0,        ROT90,  "hack", "Newpuc2 (set 2)" )
GAME( 1981, piranha,  puckman,  piranha,  mspacman, eyes,     ROT90,  "GL (US Billiards License)", "Piranha" )
GAME( 1981, piranhao, puckman,  piranha,  mspacman, eyes,     ROT90,  "GL (US Billiards License)", "Piranha (older)" )
GAME( 1981, piranhah, puckman,  pacman,   mspacman, 0,        ROT90,  "hack", "Piranha (hack)" )
GAME( 1981, nmouse,   0	     ,  nmouse ,  nmouse,   eyes,     ROT90,  "Amenip (Palcom Queen River)", "Naughty Mouse (set 1)" )
GAME( 1981, nmouseb,  nmouse ,  nmouse ,  nmouse,   eyes,     ROT90,  "Amenip Nova Games Ltd.", "Naughty Mouse (set 2)" )
GAME( 1982, pacplus,  0,        pacplus,  pacman,   pacplus,  ROT90,  "[Namco] (Midway license)", "Pac-Man Plus" )
GAME( 1981, mspacman, 0,        mspacman, mspacman, 0,        ROT90,  "Midway", "Ms. Pac-Man" )
GAME( 1981, mspacmab, mspacman, pacman,   mspacman, 0,        ROT90,  "bootleg", "Ms. Pac-Man (bootleg)" )
GAME( 1981, mspacmat, mspacman, mspacman, mspacman, 0,        ROT90,  "hack", "Ms. Pac Attack" )
GAME( 1981, mspacpls, mspacman, mspacpls, mspacpls, 0,        ROT90,  "hack", "Ms. Pac-Man Plus" )
GAME( 1981, pacgal,   mspacman, pacman,   mspacman, 0,        ROT90,  "hack", "Pac-Gal" )
GAME( 1995, mschamp,  mspacman, mschamp,  mschamp,  0,        ROT90,  "hack", "Ms. Pacman Champion Edition / Super Zola Pac Gal" )
GAME( 1981, crush,    0,        pacman,   maketrax, maketrax, ROT90,  "Kural Samno Electric", "Crush Roller (Kural Samno)" )
GAME( 1981, crush2,   crush,    pacman,   maketrax, 0,        ROT90,  "Kural Esco Electric", "Crush Roller (Kural Esco - bootleg?)" )
GAME( 1981, crush3,   crush,    pacman,   maketrax, eyes,     ROT90,  "Kural Electric", "Crush Roller (Kural - bootleg?)" )
GAME( 1981, maketrax, crush,    pacman,   maketrax, maketrax, ROT270, "[Kural] (Williams license)", "Make Trax (set 1)" )
GAME( 1981, maketrxb, crush,    pacman,   maketrax, maketrax, ROT270, "[Kural] (Williams license)", "Make Trax (set 2)" )
GAME( 1981, korosuke, crush,    pacman,   korosuke, korosuke, ROT90,  "Kural Electric", "Korosuke Roller" )
GAME( 1981, mbrush,   crush,    pacman,   mbrush,   0,        ROT90,  "bootleg", "Magic Brush" )
GAME( 1981, paintrlr, crush,    pacman,   paintrlr, 0,        ROT90,  "bootleg", "Paint Roller" )
GAME( 1982, ponpoko,  0,        pacman,   ponpoko,  ponpoko,  ROT0,   "Sigma Enterprises Inc.", "Ponpoko" )
GAME( 1982, ponpokov, ponpoko,  pacman,   ponpoko,  ponpoko,  ROT0,   "Sigma Enterprises Inc. (Venture Line license)", "Ponpoko (Venture Line)" )
GAME( 1982, eyes,     0,        pacman,   eyes,     eyes,     ROT90,  "Digitrex Techstar (Rock-ola license)", "Eyes (Digitrex Techstar)" )
GAME( 1982, eyes2,    eyes,     pacman,   eyes,     eyes,     ROT90,  "Techstar (Rock-ola license)", "Eyes (Techstar)" )
GAMEX(1983, mrtnt,    0,        pacman,   mrtnt,    eyes,     ROT90,  "Telko", "Mr. TNT", GAME_WRONG_COLORS )
GAME( 1983, gorkans,  mrtnt,    pacman,   mrtnt,    0,        ROT90,  "Techstar", "Gorkans" )
GAMEX(1983, eggor,    0,        pacman,   mrtnt,    eyes,     ROT90,  "Telko", "Eggor", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND  )
GAME( 1985, lizwiz,   0,        pacman,   lizwiz,   0,        ROT90,  "Techstar (Sunn license)", "Lizard Wizard" )
GAME( 1983, theglobp, suprglob, theglobp, theglobp, 0,        ROT90,  "Epos Corporation", "The Glob (Pac-Man hardware)" )
GAME( 1984, beastf,   suprglob, theglobp, theglobp, 0,        ROT90,  "Epos Corporation", "Beastie Feastie" )
GAME( 1983, bwcasino, 0,        acitya,   bwcasino, 0,        ROT90,  "Epos Corporation", "Boardwalk Casino" )
GAME( 1982, dremshpr, 0,        dremshpr, dremshpr, dremshpr, ROT270, "Sanritsu", "Dream Shopper" )
GAME( 1983, acitya,   bwcasino, acitya,   acitya,   0,        ROT90,  "Epos Corporation", "Atlantic City Action" )
GAME( 1983, vanvan,   0,        vanvan,   vanvan,   0,        ROT270, "Sanritsu", "Van-Van Car" )
GAME( 1983, vanvank,  vanvan,   vanvan,   vanvank,  0,        ROT270, "Karateco", "Van-Van Car (Karateco)" )
GAMEX(1982, alibaba,  0,        alibaba,  alibaba,  0,        ROT90,  "Sega", "Ali Baba and 40 Thieves", GAME_UNEMULATED_PROTECTION )
GAME( 1985, jumpshot, 0,        pacman,   jumpshot, jumpshot, ROT90,  "Bally Midway", "Jump Shot" )
GAME( 1985, shootbul, 0,        pacman,   shootbul, jumpshot, ROT90,  "Bally Midway", "Shoot the Bull" )
GAME( 2003, dderby,   0,        pacman,   mspacman, 0,        ROT90,  "D.Widel", "Death Derby (PacMan HW)" )
GAME( 2003, bace, 0, pacman, mspacman, 0, ROT90, "D.Widel", "Balloon Ace (PacMan HW)" )
GAME( 1986, bigbucks, 0,	bigbucks, bigbucks, 0,        ROT90,  "Dynasoft Inc.", "Big Bucks" )
GAME( 1984, drivfrcp, 0,        drivfrcp, drivfrcp, 0,        ROT90,  "Shinkai Inc. (Magic Eletronics Inc. licence)", "Driving Force (Pac-Man conversion)" )
GAMEX(1985, 8bpm,	  8ballact,	8bpm,	  8bpm,		8bpm,     ROT90,  "Seatongrove Ltd (Magic Eletronics USA licence)", "Eight Ball Action (Pac-Man conversion)", GAME_WRONG_COLORS )
GAMEX(1985, porky,	  0,        porky,	  porky,	porky,    ROT90,  "Shinkai Inc. (Magic Eletronics Inc. licence)", "Porky", GAME_NO_SOUND )
GAME( 1986, rocktrv2, 0,		rocktrv2, rocktrv2, rocktrv2, ROT90,  "Triumph Software Inc.", "MTV Rock-N-Roll Trivia (Part 2)" )

GAME( 2001, abortman, puckman, pacman, pacman, 0, ROT90, "Paul Copeland", "Abortman" )
GAME( 2000, baby2, puckman, pacman, pacman, 0, ROT90, "T-Bone", "Baby Pacman 2 (Alt)" )
GAME( 2000, baby3, puckman, pacman, pacman, 0, ROT90, "T-Bone", "Baby Pacman 3 (Alt)" )
GAME( 2000, baby4, puckman, pacman, pacman, 0, ROT90, "T-Bone", "Pacman (Baby Maze 4)" )
GAME( 2000, brakman,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Brakman" )
GAME (1981, bucaneer, puckman,  pacman,   mspacman, 0,        ROT90,  "PacMAME", "Bucaneer" )
GAME( 2000, caterpil, puckman,   pacman,  pacman,   0,        ROT90,  "Phi", "Caterpillar" )
GAME( 2000, chtmsatk, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Plus (Cheat)" )
GAME( 2000, chtmspa,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man (Cheat)" )
GAME( 2000, chtpac,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Cheat)" )
GAME( 2000, chtpman2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Puck-Man II (Cheat)" )
GAME( 2000, chtpop,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man / Popeye (Cheat)" )
GAME( 2000, chtpuck,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Puck-Man (Cheat)" )
GAME( 2001, cookiem,  mspacman, pacman,  mspacman, 0,        ROT90,  "Nic", "Cookie-Mon!" )
GAME( 2002, crashh,    mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Crash" )
GAME( 2000, crazypac, puckman,   pacman,  pacman,   0,        ROT90,  "Tim Appleton a.k.a. medragon", "Crazy Pac" )
GAME( 2000, dizzy,    puckman,   pacman,  pacman,   0,        ROT90,  "Tim Appleton a.k.a. medragon", "Dizzy Ghost - A Reversal of Roles" )
GAME( 2000, eltonpac, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Elton Pac" )
GAME( 2000, europac,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Euro Pac" )
GAME( 2000, fasthear, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Heart Burn (Fast)" )
GAME( 2000, fastmspa, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man (Fast)" )
GAME( 2000, fastplus, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Plus (Fast)" )
GAME( 2000, faststrm, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Desert Storm (Fast)" )
GAME( 2000, fpnleash, mspacman, pacman,  mspacman, 0,        ROT90,  "Peter Storey", "Pacman Unleashed (Flat)" )
GAME( 2000, fstmsatk, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Attack (Fast)" )
GAME( 2000, hanglyad, puckman,   pacman,  pacman,   0,        ROT90,  "hack", "Hangly-Man (set 1) After Dark" )
GAME( 1981, hanglyb,  puckman,   pacman,  pacman,   0,        ROT90,  "hack", "Hangly-Man (Hearts)" )
GAME( 2000, heartbn2, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Heart Burn" )
GAME( 2000, heartbrn, mspacman, pacman,  mspacman, 0,        ROT90,  "TwoBit Score", "Ms. Heart Burn (TwoBit Score)" )
GAME( 2000, hearts,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Hearts Bootleg)" )
GAME (2001, hm1000,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man 1000" )
GAME (2001, hm2000,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man 2000" )
GAME (2001, hmba5000, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man Babies 5000" )
GAME (2001, hmba7000, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man Babies 7000" )
GAME (2001, hmbabymz, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man BabiesMaze" )
GAME (2001, hmbluep,  puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man Blue Plus" )
GAME (2001, hmgrenp,  puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man Green Plus" )
GAME (2001, hmplus,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Hangly Man Plus" )
GAME( 2001, jacman,   puckman,  pacman,   pacman,   0,        ROT90,  "Brent Cobb", "Jacman" )
GAME( 2000, mazeman,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Maze Man" )
GAME( 2000, mrpacman, mspacman, pacman,  mspacman, 0,        ROT90,  "Tim Appleton a.k.a. medragon", "Mr. Pac- Man - Another Kind of Role Reversal" )
GAME (2001, ms1000,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 1000" )
GAME (2001, ms2000,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 2000" )
GAME( 2000, ms2600,   mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman 2600" )
GAME (2001, ms3000,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 3000" )
GAME (2001, ms4000p,  mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 4000 Plus" )
GAME (2001, ms5000,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 5000" )
GAME (2001, ms5000p,  mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman 5000 Plus" )
GAME( 2000, msatk2ad, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Plus / Attack After Dark" )
GAME( 2000, msatkad,  mspacman, pacman,  mspacman, 0,        ROT90,  "hack", "Ms. Pac-Man Plus After Dark" )
GAME( 2000, msbaby,   mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Baby Pacman" )
GAME( 2000, msbaby1,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Baby Pacman (Alt)" )
GAME( 2000, msberzk,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Berzerk" )
GAME (2001, mscrzyma, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Crazy Mazes" )
GAME (2001, mscrzymp, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Crazy Mazes Plus" )
GAME( 2001, msdroid,  mspacman, pacman,  mspacman, 0,        ROT90,  "Grendal74", "Ms. Pacman Android" )
GAME( 2000, msdstorm, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Desert Storm" )
GAME( 2000, mselton,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Elton" )
GAME (2001, msextra,  mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Extra" )
GAME (2001, msextrap, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Extra Plus" )
GAME( 2000, msf1pac,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. F1 Pac-Man" )
GAME( 2001, msgrkatk, mspacman, pacman,  mspacman, 0,        ROT90,  "Grendal74", "Ms. Pacman Greek Bootleg" )
GAME( 2001, msgrkb,   mspacman, pacman,  mspacman, 0,        ROT90,  "Grendal74", "Ms. Pacman Greek" )
GAME( 2000, mshangly, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Hangly Man" )
GAME( 2000, mshearts, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Hearts" )
GAME( 2000, msindy,   mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Indy" )
GAME( 2000, msmini,   mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pac-Mini" )
GAME( 2000, msminia,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pac-Attack Mini" )
GAME (2001, msmspac,  mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Ms. Pac" )
GAME (1999, msmulti,  mspacman, pacman,   mspacman, 0,        ROT90,  "Hack", "Ms. Pacman Multi" )
GAME( 2000, msnes4a,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman SNES (Tall Alternate)" )
GAME( 2000, msnes62,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman NES (Set 2)" )
GAME( 2000, msnes63,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman NES (Set 3)" )
GAME( 2000, msnes6m,  mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman NES" )
GAME( 2000, msnes6m2, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman SNES (New Mazes)" )
GAME( 2000, msnes6m3, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman SNES (Regular / Tall" )
GAME( 2000, msnes6m4, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman SNES (Regular)" )
GAME( 2000, mspac6m,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man 6M (Six Maze)" )
GAME( 2000, mspacad,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man After Dark" )
GAME( 2000, mspacat2, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Plus / Attack" )
GAME( 2000, mspacatb, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man Plus (Set B)" )
GAME (2001, mspacdel, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Deluxe" )
GAME (1998, mspacnes, mspacman, pacman,   mspacman, 0,        ROT90,  "T-Bone hack", "Ms. Pac-Man NES" )
GAME (2001, mspacp,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Plus" )
GAME( 2000, mspacren, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman Renaissance" )
GAME( 2000, mspacrip, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Mortem" )
GAME (2001, mspamsba, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Ms. Babies" )
GAME( 2000, mspc6mad, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Ms. Pacman After Dark (6 Mazes)" )
GAME (2001, msplus,   mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Plus" )
GAME( 2000, msrumble, mspacman, pacman,  mspacman, 0,        ROT90,  "Tim Appleton a.k.a. medragon", "Ms. Pac Rumble" )
GAME( 2000, mssilad,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pac-Man After Dark (Sil)" )
GAME (2001, msstrmaz, mspacman, pacman,   mspacman, 0,        ROT90,  "Blue Justice", "Ms. Pacman Strange Mazes" )
GAME( 2000, msultra,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacFan", "Ms. Ultra Pacman" )
GAME( 2000, msvctr6m, mspacman, pacman,  mspacman, 0,        ROT90,  "T-Bone", "Vector Ms. Pacman (6 Mazes)" )
GAME( 2000, msvectr,  mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Vector Ms. Pac-Man" )
GAME( 2000, msyakman, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Yak Man" )
GAME (1998, namcosil, puckman,   pacman,   pacman,   0,        ROT90,  "Sil hack", "Pac-Man (Namco) (Sil hack)")
GAME (1998, newpuck2, puckman,   pacman,   pacman,   0,        ROT90,  "Scotty hack", "New Puck-2")
GAME( 2000, pac2600,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac 2600" )
GAME (2000, pacatk,   mspacman, mspacman, mspacman, 0,        ROT90,  "David Widel", "Ms. Pac-Man Plus (Fruit)" )
GAME( 2000, pacbaby,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Baby Maze 1)" )
GAME( 2000, pacbaby2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Baby Maze 2)" )
GAME( 2000, pacbaby3, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Baby Maze 3)" )
GAME( 2000, pacbell,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Bell)" )
GAME( 2000, pacelec,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Electric Cowboy)" )
GAME( 2000, pacinvis, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Original Inviso)" )
GAME( 2000, pacjail,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Jail)" )
GAME( 2000, pacjr1,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman Jr. (Maze 1)" )
GAME( 2000, pacjr2,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman Jr. (Maze 2)" )
GAME( 2000, pacjr3,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman Jr. (Maze 3)" )
GAME( 2000, pacjr4,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman Jr. (Maze 4)" )
GAME( 2000, pacm255,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Midway 255th Maze)" )
GAME( 2000, pacman3d, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man 3D" )
GAME( 2000, pacman6,  mspacman, pacman,  mspacman, 0,        ROT90,  "Sil", "Pacman 6" )
GAME( 2001, pacmar,   puckman,  pacman,   pacman,   0,        ROT90,  "PacMAME", "Mario Pacman" )
GAME( 2000, pacmini,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Mini Pac-Man" )
GAME( 2000, pacmini2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Mini Pac-Man 2" )
GAME( 2000, pacmn6m2, puckman,   pacman,  pacman,   0,        ROT90,  "Sil", "Pacman 2000 (Set 2)" )
GAME( 2000, pacms1,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Maze 1)" )
GAME( 2000, pacms2,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Maze 2)" )
GAME( 2000, pacms3,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Maze 3)" )
GAME( 2000, pacms4,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Maze 4)" )
GAME( 2000, pacmsa1,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Attack 1)" )
GAME( 2000, pacmsa2,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Attack 2)" )
GAME( 2000, pacmsa3,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Attack 3)" )
GAME( 2000, pacmsa4,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Ms. Pacman Attack 4)" )
GAME( 2000, pacmulti, puckman,   pacman,  pacman,   0,        ROT90,  "Sil", "PacMulti (Pacman)" )
GAME( 2000, pacn255,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Namco 255th Maze)" )
GAME( 2000, pacplusc, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man Plus (Unencrypted)" )
GAME (1998, pacpopey, puckman,   pacman,   pacman,   0,        ROT90,  "hack", "Pac-Man (Popeye)")
GAME( 2000, pacshuf,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Shuffle)" )
GAME( 2000, pacspeed, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Speedy)" )
GAME( 2000, pacstrm,  puckman,   pacman,  pacman,   0,        ROT90,  "PacMAME", "Desert Storm Pac-Man" )
GAME( 2000, pacweird, puckman,   pacman,  pacman,   0,        ROT90,  "Staizitto", "Pacman (Six Map Weird)" )
GAME( 2000, pcrunchy, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pacman (Crunchy)" )
GAME( 2000, pengman,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pengo Man" )
GAME (2001, petshop,  mspacman, pacman,   mspacman, 0,        ROT90,  "Hack", "Pet Shop Freak-Out!")
GAME (2002, piranha2, mspacman, pacman,   mspacman, 0,        ROT90,  "Hack", "Piranha 2 Revenge!")
GAME (2001, pm1000,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man 1000" )
GAME (2001, pm2000,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man 2000" )
GAME (2001, pm3000,   puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man 3000" )
GAME( 2001, pm4000p,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man 4000 Plus" )
GAME( 2001, pm5000p,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man 5000 Plus" )
GAME( 2001, pm6000,   puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man 6000" )
GAME( 2001, pm7000,   puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man 7000" )
GAME( 2001, pm7000p,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man 7000 Plus" )
GAME( 2000, pmad,     puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man After Dark" )
GAME (1998, pmad_a,   puckman,   pacman,   pacman,   0,        ROT90,  "SirScotty, Sil, Jerry Lawrence hack", "Pac-Man After Dark (version 2)")
GAME( 2000, pmad00,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man After Dark 2000" )
GAME( 2000, pmad6m,   puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Pacman 2000 After Dark" )
GAME( 2000, pmada,    puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man After Dark (Alternate)" )
GAME( 2000, pmanalt,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man Vertical Tunnels (Cheat)" )
GAME( 2000, pmanaltm, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Pac-Man (Midway Vertical Tunnels)" )
GAME (2001, pmba2000, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man Babies 2000" )
GAME (2001, pmba3000, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man Babies 3000" )
GAME( 2001, pmba4000, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Babies 4000" )
GAME (2001, pmba6000, puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man Babies 6000" )
GAME( 2001, pmba8000, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Babies 8000" )
GAME (2001, pmbamaz,  puckman,  pacman,   pacman,   0,        ROT90,  "Blue Justice", "Pac Man Babies Maze" )
GAME( 2001, pmbaplus, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Babies Plus" )
GAME( 2001, pmbluep,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Blue Plus" )
GAME( 2001, pmdeluxe, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Deluxe" )
GAME( 2001, pmextra,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Extra" )
GAME( 2001, pmextrap, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Extra Plus" )
GAME( 2001, pmfever,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Fever" )
GAME( 2001, pmfeverp, puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Fever Plus" )
GAME( 2001, pmgrenp,  puckman,        pacman,   pacman,   pacplus,  ROT90,  "Blue Justice", "Pac Man Green Plus" )
GAME( 2000, pplusad,  puckman,   pacman,  pacman,   pacplus,  ROT90,  "[Namco] (Midway license)", "Pac-Man Plus After Dark" )
GAME( 2000, puckman2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Puck-Man II" )
GAME( 2000, puckren,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Puckman Renaissance" )
GAME( 2000, puckrenc, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Puckman Renaissance (Cheat)" )
GAME( 2000, punleash, mspacman, pacman,  mspacman, 0,        ROT90,  "Peter Storey", "Pacman Unleashed" )
GAME( 2000, roboman,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Robo Man" )
GAME( 2000, snakeyes, puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Pacman 2000 (Snake Eyes Mazes)")
GAME( 2000, snowpac,  puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Snowy Day Pacman" )
GAME( 2000, sueworld, mspacman, pacman,  mspacman, 0,        ROT90,  "PacFay", "Sue's World" )
GAME( 2000, sumelton, puckman,   pacman,  pacman,   0,        ROT90,  "Staizitto", "Summertime Elton" )
GAME( 2000, tbone,    puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Pacman 2000 (T-Bone Mazes)")
GAME( 2000, ultra2,   puckman,   pacman,  pacman,   0,        ROT90,  "PacMAME", "Ultra Pacman (TwoBit Score Mazes)" )
GAME( 2000, ultrapac, puckman,   pacman,  pacman,   0,        ROT90,  "PacFan", "UltraPac" )
GAME( 2000, vcrunchy, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Crunchy)" )
GAME( 2000, vecbaby,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Baby Maze 1)" )
GAME( 2000, vecbaby2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Baby Maze 2)" )
GAME( 2000, vecbaby3, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Baby Maze 3)" )
GAME( 2000, vectplus, puckman,   pacman,  pacman,   pacplus,  ROT90,  "[Namco] (Midway license)", "Vector Pac-Man Plus" )
GAME( 2000, vectr6m,  puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Vector Pacman 2000")
GAME( 2000, vectr6tb, puckman,   pacman,  pacman,   0,        ROT90,  "T-Bone", "Vector Pacman 2000 (T-Bone Mazes)")
GAME( 2000, vectratk, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Vector Attack" )
GAME( 2000, vectrpac, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pac-Man" )
GAME( 2000, vectxens, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Xens Revenge" )
GAME( 2000, vhangly,  puckman,   pacman,  pacman,   0,        ROT90,  "hack", "Vector Hangly-Man (set 1)" )
GAME( 2000, vpacbell, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Bell)" )
GAME( 2000, vpacelec, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Electric Cowboy)" )
GAME( 2000, vpacjail, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Jail)" )
GAME( 2000, vpacms1,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Maze 1)" )
GAME( 2000, vpacms2,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Maze 2)" )
GAME( 2000, vpacms3,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Maze 3)" )
GAME( 2000, vpacms4,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Maze 4)" )
GAME( 2000, vpacmsa1, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Attack 1)" )
GAME( 2000, vpacmsa2, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Attack 2)" )
GAME( 2000, vpacmsa3, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Attack 3)" )
GAME( 2000, vpacmsa4, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Ms. Pacman Attack 4)" )
GAME( 2000, vpacshuf, puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Shuffle)" )
GAME( 2000, vpspeed,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Vector Pacman (Speedy)" )
GAME( 2000, xensad,   puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Xens Revenge After Dark" )
GAME( 2000, xensrev,  puckman,   pacman,  pacman,   0,        ROT90,  "[Namco] (Midway license)", "Xens Revenge" )
GAME( 1981, lazybug, mspacman, pacman,   mspacman, 0,        ROT90,  "bootleg", "Lazy Bug" )
GAME( 1981, wavybug, mspacman, pacman,   mspacman, 0,        ROT90,  "bootleg", "Wavv Bug" )
GAME( 2000, zolamaze, mspacman, pacman,  mspacman, 0,        ROT90,  "PacMAME", "Ms. Pacman (Zola Mazes)" )
GAME( 2004, horizpac, mspacman, pacman,   mspacman, 0,        ROT0,  "Jerronimo", "Horiz Pac" )
GAME( 2003, at, 0, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Sequencer and Music Player" )
GAME( 2003, seq1, 0, pacman,   pacman, 0,        ROT90,  "Jerronimo", "16 Step Simple Sequencer" )
GAME( 200?, pachello, 0, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Hello, World!" )
GAME( 2001, pacmatri, 0, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Matrix Effect" )
GAME( 2003, alpaca7, alpaca8, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Alpaca v0.7 (Pacman Hardware)" )
GAME( 2003, alpaca8, 0, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Alpaca v0.8 (Pacman Hardware)" )
GAME( 2004, pacsnoop, puckman, pacman,   pacman, 0,        ROT90,  "Jerronimo", "Pac Snoop v1.3" )
GAME( 2004, ramsnoop, mspacman, pacman,   mspacman, 0,        ROT90,  "Jerronimo", "Miss Snoop v1.3" )




// Ms. Pacman II?

ROM_START( mspacii )
ROM_REGION( 0x10000, REGION_CPU1, 0 )   /* 8 banks of 64k for code */
ROM_LOAD( "msp2p36e.bin",  0x0000, 0x1000, CRC(df673b57 ))
ROM_LOAD( "msp2p46f.bin",  0x1000, 0x1000, CRC(7591f606 ))
ROM_LOAD( "msp2p56h.bin",  0x2000, 0x1000, CRC(c8ef1a7f ))
ROM_LOAD( "msp2p66j.bin",  0x3000, 0x1000, CRC(165a9dd8 ))
ROM_LOAD( "msp2p7s1.bin",  0x8000, 0x1000, CRC(fbbc3d2e ))
ROM_LOAD( "msp2p8s2.bin",  0x9000, 0x1000, CRC(aba3096d ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "msp2p15e.bin",  0x0000, 0x1000, CRC(04333722 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "msp2p25f.bin",  0x0000, 0x1000, CRC(615af909 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",     0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",     0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )  
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	
ROM_END

/*          rom       parent    machine   inp       init */
GAME( 1998, mspacii,  0,        pacman,   mspacman,   0,            ROT90,  "[Midway] Orca?", "Ms. Pacman II" )



/*
Original, Encrypted Piranha set

This set is believed to be the "original" piranha set and uses
the same encryption as "eyes": It uses a slightly different CLUT prom (.4a)
than pacman, and the title screen is pretty much different to the piranha
set in MAME. Its easier to see how this set was taken for a new game
by arcade owners than the one existing in MAME

There is some protection which is not emulated, and is patched out in
init_newpir
*/

// This init is the same as for eyes, but with the code patch at 0x030fb
static void init_newpir(void)
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	/* Data lines D3 and D5 swapped */
	RAM = memory_region(REGION_CPU1);
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] =  (RAM[i] & 0xc0) | ((RAM[i] & 0x08) << 2) |
			(RAM[i] & 0x10) | ((RAM[i] & 0x20) >> 2) | (RAM[i] & 0x07);
	}

	/* Apply Code Patch to clear protection */
	RAM[0x30fb]=0xc3;
	RAM[0x30fc]=0x74;
	RAM[0x30fd]=0x31;

	/* Graphics ROMs */

	/* Data lines D4 and D6 and address lines A0 and A2 are swapped */
	RAM = memory_region(REGION_GFX1);
	for (i = 0;i < memory_region_length(REGION_GFX1);i += 8)
		eyes_decode(&RAM[i]);
	RAM = memory_region(REGION_GFX2);
	for (i = 0;i < memory_region_length(REGION_GFX2);i += 8)
		eyes_decode(&RAM[i]);
}

ROM_START( newpir )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "1.bin",        0x0000, 0x0800, CRC(69a3e6ea ))
ROM_LOAD( "5.bin",        0x0800, 0x0800, CRC(245e753f ))
ROM_LOAD( "2.bin",        0x1000, 0x0800, CRC(62cb6954 ))
ROM_LOAD( "6.bin",        0x1800, 0x0800, CRC(cb0700bc ))
ROM_LOAD( "3.bin",        0x2000, 0x0800, CRC(843fbfe5 ))
ROM_LOAD( "7.bin",        0x2800, 0x0800, CRC(73084d5e ))
ROM_LOAD( "4.bin",        0x3000, 0x0800, CRC(4cdf6704 ))
ROM_LOAD( "8.bin",        0x3800, 0x0800, CRC(b86fedb3 ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "9.bin",        0x0000, 0x0800, CRC(0f19eb28 ))
ROM_LOAD( "11.bin",       0x0800, 0x0800, CRC(5f8bdabe ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "10.bin",       0x0000, 0x0800, CRC(d19399fb ))
ROM_LOAD( "12.bin",       0x0800, 0x0800, CRC(cfb4403d ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(08c9447b ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

ROM_START( newpirb )
ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
ROM_LOAD( "pir1.bin",        0x0000, 0x0800, CRC(c6ce1bfc ))
ROM_LOAD( "pir5.bin",        0x0800, 0x0800, CRC(a2655a33 ))
ROM_LOAD( "pir2.bin",        0x1000, 0x0800, CRC(62cb6954 ))
ROM_LOAD( "pir6.bin",        0x1800, 0x0800, CRC(cb0700bc ))
ROM_LOAD( "pir3.bin",        0x2000, 0x0800, CRC(843fbfe5 ))
ROM_LOAD( "pir7.bin",        0x2800, 0x0800, CRC(73084d5e ))
ROM_LOAD( "pir4.bin",        0x3000, 0x0800, CRC(9363a4d1 ))
ROM_LOAD( "pir8.bin",        0x3800, 0x0800, CRC(2769979c ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "pir9.bin",        0x0000, 0x0800, CRC(94eb7563 ))
ROM_LOAD( "pir11.bin",       0x0800, 0x0800, CRC(a3606973 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "pir10.bin",       0x0000, 0x0800, CRC(84165a2c ))
ROM_LOAD( "pir12.bin",       0x0800, 0x0800, CRC(2699ba9e ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "piranha.4a",    0x0020, 0x0100, CRC(08c9447b ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

/*          rom       parent    machine   inp       init */
GAME( 1981, newpir,   puckman,  pacman,   mspacman, newpir,        ROT90,  "hack", "Piranha (Original - Encrypted)" )
GAME( 1981, newpirb,  puckman,  pacman,   mspacman, newpir,        ROT90,  "hack", "Piranha (Original - Encrypted/Older)" )

ROM_START( test )
ROM_REGION( 0x10000, REGION_CPU1, 0 )   /* 64k for code */
ROM_LOAD( "test.rom",     0x0000, 0x1000, CRC(fb645998 ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "test.5e",      0x0000, 0x1000, CRC(0c944964 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "test.5f",      0x0000, 0x1000, CRC(958fedf9 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66 ))	/* timing - not used */
ROM_END

GAME( 1999, test,      puckman,   pacman,   mspacman, 0, ROT90, "David Caldwell", "Pacman Hardware Test" )




// New-Puck-2 Set 2
INPUT_PORTS_START( nwpuc2b )
PORT_START	/* IN0 */
PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
PORT_DIPSETTING(    0x00, DEF_STR( On ) )
PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

PORT_START	/* IN1 */
PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
PORT_DIPNAME(0x80, 0x80, DEF_STR( Cabinet ) )
PORT_DIPSETTING(   0x80, DEF_STR( Upright ) )
PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ) )

PORT_START	/* DSW 1 */
PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
PORT_DIPSETTING(    0x00, "1" )
PORT_DIPSETTING(    0x04, "2" )
PORT_DIPSETTING(    0x08, "3" )
PORT_DIPSETTING(    0x0c, "5" )
PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
PORT_DIPSETTING(    0x00, "30000" )
PORT_DIPSETTING(    0x10, "60000" )
PORT_DIPSETTING(    0x20, "90000" )
PORT_DIPSETTING(    0x30, "None" )
PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
PORT_DIPSETTING(    0x40, "Normal" )
PORT_DIPSETTING(    0x00, "Hard" )
PORT_DIPNAME( 0x80, 0x80, "Ghost Names" )
PORT_DIPSETTING(    0x80, "Normal" )
PORT_DIPSETTING(    0x00, "Alternate" )

PORT_START	/* DSW 2 */
PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

PORT_START	/* FAKE */
/* This fake input port is used to get the status of the fire button */
/* and activate the speedup cheat if it is. */
PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", KEYCODE_LCONTROL, JOYCODE_1_BUTTON1 )
PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END

ROM_START( nwpuc2b )
ROM_REGION( 0x10000, REGION_CPU1,0 )
ROM_LOAD( "nwpuc2b.6e", 0x0000, 0x1000, CRC(532bd09f ))
ROM_LOAD( "nwpuc2b.6f", 0x1000, 0x1000, CRC(b9062f57 ))
ROM_LOAD( "nwpuc2b.6h", 0x2000, 0x1000, CRC(519b3c57 ))
ROM_LOAD( "nwpuc2b.6j", 0x3000, 0x1000, CRC(154cc118 ))
ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
ROM_LOAD( "nwpuc2b.5e", 0x0000, 0x1000, CRC(58357313 ))
ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
ROM_LOAD( "nwpuc2b.5f", 0x0000, 0x1000, CRC(4b044c87 ))
ROM_REGION( 0x0120, REGION_PROMS, 0 )
ROM_LOAD( "82s123.7f", 0x0000, 0x0020, CRC(2fc650bd ))
ROM_LOAD( "82s126.4a", 0x0020, 0x0100, CRC(3eb3a8e4 ))
ROM_REGION( 0x0200, REGION_SOUND1, 0 )     /* sound PROMs */
ROM_LOAD( "82s126.1m", 0x0000, 0x0100, CRC(a9cc86bf ))
ROM_LOAD( "82s126.3m", 0x0100, 0x0100, CRC(77245b66 ))
ROM_END

/*          rom       parent     machine   inp       init */
GAME( 1981, nwpuc2b,  puckman,   pacman,   nwpuc2b,  0, ROT90, "[hack]", "New Puckman 2 (Set 2)" )
