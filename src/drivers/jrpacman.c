/* It is likely that most of the GFX roms have a 5 at the beginning of the CRC. Check and fix */

/***************************************************************************

	Bally/Midway Jr. Pac-Man

    Games supported:
		* Jr. Pac-Man
	
	Known issues:
		* none

****************************************************************************

	Jr. Pac Man memory map (preliminary)

	0000-3fff ROM
	4000-47ff Video RAM (also color RAM)
	4800-4fff RAM
	8000-dfff ROM

	memory mapped ports:

	read:
	5000      IN0
	5040      IN1
	5080      DSW1

	*
	 * IN0 (all bits are inverted)
	 * bit 7 : CREDIT
	 * bit 6 : COIN 2
	 * bit 5 : COIN 1
	 * bit 4 : RACK TEST
	 * bit 3 : DOWN player 1
	 * bit 2 : RIGHT player 1
	 * bit 1 : LEFT player 1
	 * bit 0 : UP player 1
	 *
	*
	 * IN1 (all bits are inverted)
	 * bit 7 : TABLE or UPRIGHT cabinet select (1 = UPRIGHT)
	 * bit 6 : START 2
	 * bit 5 : START 1
	 * bit 4 : TEST SWITCH
	 * bit 3 : DOWN player 2 (TABLE only)
	 * bit 2 : RIGHT player 2 (TABLE only)
	 * bit 1 : LEFT player 2 (TABLE only)
	 * bit 0 : UP player 2 (TABLE only)
	 *
	*
	 * DSW1 (all bits are inverted)
	 * bit 7 :  ?
	 * bit 6 :  difficulty level
	 *                       1 = Normal  0 = Harder
	 * bit 5 :\ bonus pac at xx000 pts
	 * bit 4 :/ 00 = 10000  01 = 15000  10 = 20000  11 = 30000
	 * bit 3 :\ nr of lives
	 * bit 2 :/ 00 = 1  01 = 2  10 = 3  11 = 5
	 * bit 1 :\ play mode
	 * bit 0 :/ 00 = free play   01 = 1 coin 1 credit
	 *          10 = 1 coin 2 credits   11 = 2 coins 1 credit
	 *

	write:
	4ff2-4ffd 6 pairs of two bytes:
	          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
			  X flip (bit 1); the second byte the color
	5000      interrupt enable
	5001      sound enable
	5002      unused
	5003      flip screen
	5004      unused
	5005      unused
	5006      unused
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
	5062-506d Sprite coordinates, x/y pairs for 6 sprites
	5070      palette bank
	5071      colortable bank
	5073      background priority over sprites
	5074      char gfx bank
	5075      sprite gfx bank
	5080      scroll
	50c0      Watchdog reset

	I/O ports:
	OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "jrpacman.h"


static UINT8 speedcheat = 0;	/* a well known hack allows to make JrPac Man run at four times */
				/* his usual speed. When we start the emulation, we check if the */
				/* hack can be applied, and set this flag accordingly. */



/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( jrpacman )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if (RAM[0x180b] == 0xbe || RAM[0x180b] == 0x01)
		speedcheat = 1;
	else speedcheat = 0;
}



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static INTERRUPT_GEN( jrpacman_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputport(3) & 1)	/* check status of the fake dip switch */
		{
			/* activate the cheat */
			RAM[0x180b] = 0x01;
		}
		else
		{
			/* remove the cheat */
			RAM[0x180b] = 0xbe;
		}
	}
	irq0_line_hold();
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },	/* including video and color RAM */
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ 0x8000, 0xdfff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, jrpacman_videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fef, MWA_RAM },
	{ 0x4ff0, 0x4fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5003, 0x5003, jrpacman_flipscreen_w },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x5060, 0x506f, MWA_RAM, &spriteram_2 },
	{ 0x5070, 0x5070, jrpacman_palettebank_w, &jrpacman_palettebank },
	{ 0x5071, 0x5071, jrpacman_colortablebank_w, &jrpacman_colortablebank },
	{ 0x5073, 0x5073, MWA_RAM, &jrpacman_bgpriority },
	{ 0x5074, 0x5074, jrpacman_charbank_w, &jrpacman_charbank },
	{ 0x5075, 0x5075, MWA_RAM, &jrpacman_spritebank },
	{ 0x5080, 0x5080, MWA_RAM, &jrpacman_scroll },
	{ 0x50c0, 0x50c0, MWA_NOP },
	{ 0x8000, 0xdfff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Main CPU port handlers
 *
 *************************************/

static PORT_WRITE_START( writeport )
	{ 0, 0, interrupt_vector_w },
PORT_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( jrpacman )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

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

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};


static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 128 },
	{ REGION_GFX2, 0, &spritelayout,    0, 128 },
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

static MACHINE_DRIVER_START( jrpacman )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(0,writeport)
	MDRV_CPU_VBLANK_INT(jrpacman_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(jrpacman)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(128*4)

	MDRV_PALETTE_INIT(jrpacman)
	MDRV_VIDEO_START(jrpacman)
	MDRV_VIDEO_UPDATE(jrpacman)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO, namco_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( jrpacman )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) SHA1(5ea34621213c649ca2848ab31aab2cbe751723d4) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) SHA1(8294e9e79f8fd19a419431fa690e6ac4a1302f58) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) SHA1(b84b34560b9aae18b24274712b052283faa01730) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) SHA1(07d912a61824323c8fc1b8bd0da89172d4f70b91) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) SHA1(18bd4d5381656120e4242811006c20776774de4d) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(0527ff9b) SHA1(37fe3176b0d125b7d629e108e7ebdc1196e4a132) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(73477193) SHA1(f00a488958ea0438642d345693787bdf771219ad) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) SHA1(62cf15513934d34641433c891a7f73bef82e2fb1) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )	/* timing - not used */
ROM_END

ROM_START( jrfast )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
/*
	ROM_LOAD( "jppac1.bin",   0x0000, 0x4000, CRC(792b5ab8) )
*/
	ROM_LOAD( "jppac2.bin",   0x0000, 0x4000, CRC(f3bcb240) )

	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(0527ff9b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(73477193) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( fastjr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(0527ff9b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(73477193) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jrvectr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(4876ad83) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(c0b35564) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */

ROM_END

ROM_START( jrpacad )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(51ecf940c) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(c0b35564) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */

ROM_END

ROM_START( jrhearts )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(50527ff9b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(573477193) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END
// End Emu Plus
ROM_START( jr1000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(58f93c273) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5eac97aec) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(52784936b) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5a26f1a49) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2001 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(59171f08f) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(53164b853) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2001p)
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5d56cb9c5) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(51b534804) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2002 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5e254096c) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5e3240bec) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2002p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5ad1d0360) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5160321a1) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2003 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5ffe21f5) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(51fd35d7a) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2003p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5b24e9383) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(565ba2fb7) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2004 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(53c20cf84) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5f08575ab) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2004p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(56e1db602) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5ab075ebe) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2005 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(542eb8848) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5502d286c) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr2005p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(54d03cd0a) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(551cbce81) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr3000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(58f93c273) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5a8f7b5c) )
	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr4000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5c5f05f11) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5944add80) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr5000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(586979e1) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5a0fce81b) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr6000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(544bd3c24) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(583c8ff32) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr7000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(53dca34d5) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5202f2f37) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr7000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(53b3e60fe) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(589b17c92) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr8000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(59653f563) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5b1791f8) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr8000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5913a0bea) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5aed01226) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jr9000p )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(5b2653891) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(587f45bd7) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jrdeluxe )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(590517001) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(5ef042965) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jrpacjr)
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(554e6c297) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(55b34dd98) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jrpacjrp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(554e6c297) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(51ee279ef) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END

ROM_START( jrpacp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, CRC(e3fa972e) )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, CRC(ec889e94) )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, CRC(35f1fc6e) )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, CRC(9737099e) )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, CRC(5252dd97) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, CRC(54fe4238e) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "jrp2e.bin",    0x0000, 0x2000, CRC(55993c0fa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "jrprom.9e",    0x0000, 0x0100, CRC(029d35c4) ) /* palette low bits */
	ROM_LOAD( "jrprom.9f",    0x0100, 0x0100, CRC(eee34a79) ) /* palette high bits */
	ROM_LOAD( "jrprom.9p",    0x0200, 0x0100, CRC(9f6ea9d8) ) /* color lookup table */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )	/* sound prom */
	ROM_LOAD( "jrprom.7p",    0x0000, 0x0100, CRC(a9cc86bf) )
	ROM_LOAD( "jrprom.5s",    0x0100, 0x0100, CRC(77245b66) )	/* timing - not used */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( jrpacman )
{
	/* The encryption PALs garble bits 0, 2 and 7 of the ROMs. The encryption */
	/* scheme is complex (basically it's a state machine) and can only be */
	/* faithfully emulated at run time. To avoid the performance hit that would */
	/* cause, here we have a table of the values which must be XORed with */
	/* each memory region to obtain the decrypted bytes. */
	/* Decryption table provided by David Caldwell (david@indigita.com) */
	/* For an accurate reproduction of the encryption, see jrcrypt.c */
	struct {
	    int count;
	    int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
	    { 0,0 }
	};
	int i,j,A;
	unsigned char *RAM = memory_region(REGION_CPU1);


	A = 0;
	i = 0;
	while (table[i].count)
	{
		for (j = 0;j < table[i].count;j++)
		{
			RAM[A] ^= table[i].value;
			A++;
		}
		i++;
	}
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, jrpacman, 0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man" )
/* Misfited */
GAME( 1983, jrfast,   0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man (Fast)" )
GAME( 1983, fastjr,   0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man (Fast)" )
GAME( 1983, jrpacad,  0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man After Dark" )
GAME( 1983, jrvectr,  0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man Vector" )
GAME( 2000, jrhearts, 0, jrpacman, jrpacman, jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man Hearts" )
GAME( 2000, jr1000,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 1000" )
GAME( 2000, jr2000,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2000" )
GAME( 2000, jr2001,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2001" )
GAME( 2000, jr2001p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2001 Plus" )
GAME( 2000, jr2002,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2002" )
GAME( 2000, jr2002p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2002 Plus" )
GAME( 2000, jr2003,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2003" )
GAME( 2000, jr2003p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2003 Plus" )
GAME( 2000, jr2004,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2004" )
GAME( 2000, jr2004p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2004 Plus" )
GAME( 2000, jr2005,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2005" )
GAME( 2000, jr2005p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 2005 Plus" )
GAME( 2000, jr3000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 3000 Plus" )
GAME( 2000, jr4000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 4000 Plus" )
GAME( 2000, jr5000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 5000 Plus" )
GAME( 2000, jr6000,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 6000" )
GAME( 2000, jr7000,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 7000" )
GAME( 2000, jr7000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 7000 Plus" )
GAME( 2000, jr8000,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 8000" )
GAME( 2000, jr8000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 8000 Plus" )
GAME( 2000, jr9000p,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man 9000 Plus" )
GAME( 2000, jrdeluxe, 0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man Deluxe" )
GAME( 2000, jrpacjr,  0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man Junior" )
GAME( 2000, jrpacjrp, 0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man Junior Plus" )
GAME( 2000, jrpacp,   0, jrpacman, jrpacman, jrpacman, ROT90, "Blue Justice", "Jr. Pac-Man Plus" )
