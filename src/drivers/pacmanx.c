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
		* Lizard Wizard
		* The Glob
		* Dream Shopper
		* Van Van Car
		* Ali Baba and 40 Thieves
		* Jump Shot
		* Shoot the Bull

	Known issues:
		* mystery items in Ali Baba don't work correctly because of protection

****************************************************************************

	Pac-Man memory map (preliminary)

	0000-3fff ROM
	4000-43ff Video RAM
	4400-47ff Color RAM
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

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "pacman.h"
#include "pengo.h"



static UINT8 speedcheat = 0;	/* a well known hack allows to make Pac Man run at four times */
								/* his usual speed. When we start the emulation, we check if the */
								/* hack can be applied, and set this flag accordingly. */



/*************************************
 *
 *	Machine init
 *
 *************************************/

MACHINE_INIT( pacmanx )
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

static INTERRUPT_GEN( pacmanx_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputport(4) & 1)	/* check status of the fake dip switch */
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





/*************************************
 *
 *	LEDs/coin counters
 *
 *************************************/

static WRITE_HANDLER( pacmanx_leds_w )
{
	set_led_status(offset,data & 1);
}

// PacMAME
static WRITE_HANDLER( pacmanx_coin_counter_w )
{
	coin_counter_w(offset,data & 1);
}
// End PacMAME

static WRITE_HANDLER( pacman_coin_lockout_global_w )
{
	// PacMAME
	coin_lockout_global_w(~data & 0x01);
	//coin_lockout_global_w(offset, ~data & 0x01);
	// End PacMAME
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x4c00, 0x4fff, MRA_RAM },	/* including sprite codes at 4ff0-4fff */
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ 0x50c0, 0x50ff, input_port_3_r },	/* DSW2 */
	{ 0x8000, 0xbfff, MRA_ROM },	/* Ms. Pac-Man / Ponpoko only */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4c00, 0x4fef, MWA_RAM },
	{ 0x4ff0, 0x4fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5002, MWA_NOP },
	{ 0x5003, 0x5003, pacmanx_flipscreen_w },
 	{ 0x5004, 0x5005, pacmanx_leds_w },
// 	{ 0x5006, 0x5006, pacmanx_coin_lockout_global_w },	this breaks many games
    // PacMAME
 	{ 0x5007, 0x5007, pacmanx_coin_counter_w },
 	//{ 0x5007, 0x5007, coin_counter_w },
 	// End PacMAME
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x5060, 0x506f, MWA_RAM, &spriteram_2 },
	{ 0x50c0, 0x50c0, watchdog_reset_w },
	{ 0x8000, 0xbfff, MWA_ROM },	/* Ms. Pac-Man / Ponpoko only */
	{ 0xc000, 0xc3ff, videoram_w }, /* mirror address for video ram, */
	{ 0xc400, 0xc7ef, colorram_w }, /* used to display HIGH SCORE and CREDITS */
	{ 0xffff, 0xffff, MWA_NOP },	/* Eyes writes to this location to simplify code */
// PacMAME
MEMORY_END
//	{ -1 }	/* end of table */
//};
// End PacMAME





/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

// PacMAME
static PORT_WRITE_START( writeport )
//static struct IOWritePort writeport[] =
//{
// End PacMAME
	{ 0x00, 0x00, interrupt_vector_w },	/* Pac-Man only */
// PacMAME
PORT_END
//	{ -1 }	/* end of table */
//};
// End PacMAME


/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( pacmanx )
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

	PORT_START	/* DSW 2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", KEYCODE_LCONTROL, JOYCODE_1_BUTTON1 )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END


/* Ms. Pac-Man input ports are identical to Pac-Man, the only difference is */
/* the missing Ghost Names dip switch. */
INPUT_PORTS_START( mspacx )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

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
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW 2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", KEYCODE_LCONTROL, JOYCODE_1_BUTTON1 )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END




/*************************************
 *
 *	Graphics layouts
 *
 *************************************/


static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 128, 129, 130, 131, 256, 257, 258, 259, 384, 385, 386, 387 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8	/* every tile takes 64 bytes */
};


static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
//16,16
	64,	/* 64 sprites */
//256
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 256, 257, 258, 259, 512, 513, 514, 515, 768, 769, 770, 771,
			1024+0, 1024+1, 1024+2, 1024+3, 1024+256, 1024+257, 1024+258, 1024+259,
			1024+512, 1024+513, 1024+514, 1024+515,	1024+768, 1024+769, 1024+770, 1024+771 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,	24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8 },
	256*8	/* every sprite takes 256 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0, 32 },
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




/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( pacmanx )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18432000/6)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(0,writeport)
	MDRV_CPU_VBLANK_INT(pacmanx_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(pacmanx)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*16, 28*16)
	MDRV_VISIBLE_AREA(0*16, 36*16-1, 0*8, 28*16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(4*32)

	MDRV_PALETTE_INIT(pacman)
	MDRV_VIDEO_START(pacmanx)
	MDRV_VIDEO_UPDATE(pacmanx)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("namco", NAMCO, namco_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( pacmanx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3) )
	ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(39d1fc83) )
	ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(02083b03) )
	ROM_LOAD( "namcopac.6j",  0x3000, 0x1000, CRC(7a36fe55) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e2",   0x0000, 0x4000, CRC(a45138ce) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( npacmodx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, CRC(fee263b3) )
	ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, CRC(39d1fc83) )
	ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, CRC(02083b03) )
	ROM_LOAD( "npacmod.6j",   0x3000, 0x1000, CRC(7d98d5f5) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e2",   0x0000, 0x4000, CRC(a45138ce) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( pacmanxj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
	ROM_LOAD( "prg7",         0x3000, 0x0800, CRC(b6289b26) )
	ROM_LOAD( "prg8",         0x3800, 0x0800, CRC(17a88c13) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacmanjp.5e2", 0x0000, 0x4000, CRC(c8d9349e) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( pacmanxm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, CRC(c1e6ab10) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, CRC(bcdd1beb) )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, CRC(817d94e3) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e2",   0x0000, 0x4000, CRC(a45138ce) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( pacmodx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacmanh.6e",   0x0000, 0x1000, CRC(3b2ec270) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
	ROM_LOAD( "pacmanh.6h",   0x2000, 0x1000, CRC(18811780) )
	ROM_LOAD( "pacmanh.6j",   0x3000, 0x1000, CRC(5c96a733) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacmanh.5e2",  0x0000, 0x4000, CRC(a05bc552) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hanglyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
	ROM_LOAD( "hangly.6f",    0x1000, 0x1000, CRC(73726586) )
	ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
	ROM_LOAD( "hangly.6j",    0x3000, 0x1000, CRC(7f4147e6) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e2",   0x0000, 0x4000, CRC(a45138ce) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( hangly2x )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hangly.6e",    0x0000, 0x1000, CRC(5fe8610a) )
	ROM_LOAD( "hangly2.6f",   0x1000, 0x0800, CRC(5ba228bb) )
	ROM_LOAD( "hangly2.6m",   0x1800, 0x0800, CRC(baf5461e) )
	ROM_LOAD( "hangly.6h",    0x2000, 0x1000, CRC(4e7ef99f) )
	ROM_LOAD( "hangly2.6j",   0x3000, 0x0800, CRC(51305374) )
	ROM_LOAD( "hangly2.6p",   0x3800, 0x0800, CRC(427c9d4d) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacmanh.5e2",  0x0000, 0x4000, CRC(a05bc552) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( puckmanx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "puckman.6e",   0x0000, 0x1000, CRC(a8ae23c5) )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, CRC(1a6fb2d4) )
	ROM_LOAD( "puckman.6h",   0x2000, 0x1000, CRC(197443f8) )
	ROM_LOAD( "puckman.6j",   0x3000, 0x1000, CRC(2e64a3ba) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5e2",   0x0000, 0x4000, CRC(a45138ce) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacman.5f2",   0x0000, 0x4000, CRC(92e1b10e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( pheartx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "pacheart.pg1", 0x0000, 0x0800, CRC(d844b679) )
	ROM_LOAD( "pacheart.pg2", 0x0800, 0x0800, CRC(b9152a38) )
	ROM_LOAD( "pacheart.pg3", 0x1000, 0x0800, CRC(7d177853) )
	ROM_LOAD( "pacheart.pg4", 0x1800, 0x0800, CRC(842d6574) )
	ROM_LOAD( "pacheart.pg5", 0x2000, 0x0800, CRC(9045a44c) )
	ROM_LOAD( "pacheart.pg6", 0x2800, 0x0800, CRC(888f3c3e) )
	ROM_LOAD( "pacheart.pg7", 0x3000, 0x0800, CRC(f5265c10) )
	ROM_LOAD( "pacheart.pg8", 0x3800, 0x0800, CRC(1a21a381) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacheart.5e2", 0x0000, 0x4000, CRC(868825e6) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacheart.5f2", 0x0000, 0x4000, CRC(b7d222ba) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )  /* timing - not used */
ROM_END

ROM_START( pacplusx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, CRC(d611ef68) )
	ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, CRC(c7207556) )
	ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, CRC(ae379430) )
	ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, CRC(5a6dff7b) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacplus.5e2",  0x0000, 0x4000, CRC(06b08556) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacplus.5f2",  0x0000, 0x4000, CRC(57e9f865) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, CRC(063dd53a) )
	ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, CRC(e271a166) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( mspacx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "5e2",          0x0000, 0x4000, CRC(d9a2897d) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "5f2",          0x0000, 0x4000, CRC(6f625a9e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( mspatkx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
	ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, CRC(0af09d31) )
	ROM_LOAD( "boot3",        0x2000, 0x1000, CRC(1821ee0b) )
	ROM_LOAD( "boot4",        0x3000, 0x1000, CRC(165a9dd8) )
	ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, CRC(e6e06954) )
	ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, CRC(3b5db308) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "5e2",          0x0000, 0x4000, CRC(d9a2897d) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "5f2",          0x0000, 0x4000, CRC(6f625a9e) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, CRC(3eb3a8e4) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( pacgalx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, CRC(d16b31b7) )
	ROM_LOAD( "boot2",        0x1000, 0x1000, CRC(0d32de5e) )
	ROM_LOAD( "pacman.7fh",   0x2000, 0x1000, CRC(513f4d5c) )
	ROM_LOAD( "pacman.7hj",   0x3000, 0x1000, CRC(70694c8e) )
	ROM_LOAD( "boot5",        0x8000, 0x1000, CRC(8c3e6de6) )
	ROM_LOAD( "boot6",        0x9000, 0x1000, CRC(368cb165) )

	ROM_REGION( 0x4000, REGION_GFX1 , ROMREGION_DISPOSE )
	ROM_LOAD( "5e2",          0x0000, 0x4000, CRC(d9a2897d) )

	ROM_REGION( 0x4000, REGION_GFX2 , ROMREGION_DISPOSE )
	ROM_LOAD( "pacgal.5f2",   0x0000, 0x4000, CRC(dfb2a9e9) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, CRC(2fc650bd) )
	ROM_LOAD( "82s129.4a",    0x0020, 0x0100, CRC(63efb927) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

static void init_pacplus(void)
{
	pacplus_decode();
}
/*          rom       parent    machine   inp       init */
GAME( 1980, pacmanx,  puckman,  pacmanx,   pacmanx,   0,        ROT90,  "Namco", "PuckManX (Japan set 1)" )
GAME( 1980, pacmanxj, puckman,  pacmanx,   pacmanx,   0,        ROT90,  "Namco", "PuckManX (Japan set 2)" )
GAME( 1980, pacmanxm, puckman,  pacmanx,   pacmanx,   0,        ROT90,  "[Namco] (Midway license)", "Pac-ManX (Midway)" )
GAME( 1981, npacmodx, puckman,  pacmanx,   pacmanx,   0,        ROT90,  "Namco", "PuckManX (harder?)" )
GAME( 1981, pacmodx,  puckman,  pacmanx,   pacmanx,   0,        ROT90,  "[Namco] (Midway license)", "Pac-ManX (Midway harder)" )
GAME( 1981, hanglyx,  puckman,  pacmanx,   pacmanx,   0,        ROT90,  "hack", "Hangly-ManX (set 1)" )
GAME( 1981, hangly2x, puckman,  pacmanx,   pacmanx,   0,        ROT90,  "hack", "Hangly-ManX (set 2)" )
GAME( 1980, puckmanx, puckman,  pacmanx,   pacmanx,   0,        ROT90,  "hack", "New Puck-XX" )
GAME( 1981, pheartx,  puckman,  pacmanx,   pacmanx,   0,        ROT90,  "hack", "Pac-ManX (Hearts)" )
GAME( 1982, pacplusx, pacplus, pacmanx,   pacmanx,   pacplus,  ROT90,  "[Namco] (Midway license)", "Pac-Man PlusX" )
GAME( 1981, mspacx,   mspacman,  pacmanx,   mspacx,    0,        ROT90,  "bootleg", "Ms. Pac-ManX" )
GAME( 1981, mspatkx,  mspacman,  pacmanx,   mspacx,    0,        ROT90,  "hack", "Ms. Pac-Man PlusX" )
GAME( 1981, pacgalx,  mspacman,  pacmanx,   mspacx,    0,        ROT90,  "hack", "Pac-GalX" )
// End PacMAME
