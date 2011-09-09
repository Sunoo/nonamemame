/* Super Derby - Playmark */

/*

Super Derby
Playmark '96

mc 68k 12mhz
OKI m6295

2x GM 76c88al (8kx8 6264) near program ROMs
2x 6264 near gfx ROMs
16k nonvolatile SRAM (Dallas 1220Y)
two FPGA chips - one labelled 'Playmark 010412'
4x hm3-65728bk-5
--
21.bin 6F9F2F2B - near OKI (samples?)

22.bin A319F1E0 - program code
23.bin 1D6E2321 /

24.bin 93C917DF  - gfx
25.bin 7BA485CD
26.bin BEABE4F7
27.bin 672CE5DF
28.bin 39CA3B52
--

*/


/* working notes:

apparently this pcb doesn't contain a pic so it should be possible to emulate sound
this can almost certainly be merged with playmark.c
probably needs eeprom support adding

Stephh's notes :

  - The game is playable, but :

      * it isn't possible to decrease the bet (but it might be an ingame "feature")
      * it isn't possible to insert a note
      * it isn't possible to exchange the winning points against tickets or cash

  - The settings can be modified in the "test mode", but there aren't mapped to
    input ports.


TO DO :

  - figure out the reads from 0x308002.w and 0x30800e.w (see sderby_input_r read handler)
  - add sound support (by default, demo sounds are OFF, so change this in the "test mode")

*/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *sderby_fg_videoram;
data16_t *sderby_md_videoram;
data16_t *sderby_videoram;

WRITE16_HANDLER( sderby_videoram_w );
WRITE16_HANDLER( sderby_md_videoram_w );
WRITE16_HANDLER( sderby_fg_videoram_w );
VIDEO_START( sderby );
VIDEO_UPDATE( sderby );
WRITE16_HANDLER( sderby_scroll_w );


static READ16_HANDLER ( sderby_input_r )
{
	switch (offset)
	{
		case 0x00 >> 1:
			return readinputport(0);
		case 0x02 >> 1:
			return 0xffff;			// to avoid game to reset (needs more work)
	}

	logerror("sderby_input_r : offset = %x - PC = %06x\n",offset*2,activecpu_get_pc());

	return 0xffff;
}

static ADDRESS_MAP_START( sderby_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x200fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x308000, 0x30800d) AM_READ(sderby_input_r)
	AM_RANGE(0x30800e, 0x30800f) AM_READ(OKIM6295_status_0_lsb_r)
	AM_RANGE(0xd00000, 0xd001ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xf00000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sderby_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100fff) AM_WRITE(sderby_videoram_w) AM_BASE(&sderby_videoram) // bg
	AM_RANGE(0x101000, 0x101fff) AM_WRITE(sderby_md_videoram_w) AM_BASE(&sderby_md_videoram) // mid
	AM_RANGE(0x102000, 0x103fff) AM_WRITE(sderby_fg_videoram_w) AM_BASE(&sderby_fg_videoram) // fg
	AM_RANGE(0x104000, 0x10400b) AM_WRITE(sderby_scroll_w)

	AM_RANGE(0x10400c, 0x10400d) AM_WRITE(MWA16_NOP)	// ??? - check code at 0x000456 (executed once at startup)
	AM_RANGE(0x10400e, 0x10400f) AM_WRITE(MWA16_NOP)	// ??? - check code at 0x000524 (executed once at startup)

	AM_RANGE(0x200000, 0x200fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)

	AM_RANGE(0x308008, 0x308009) AM_WRITE(MWA16_NOP)	// ???
	AM_RANGE(0x30800e, 0x30800f) AM_WRITE(OKIM6295_data_0_lsb_w)

	AM_RANGE(0x380000, 0x380fff) AM_WRITE(paletteram16_RRRRRGGGGGBBBBBx_word_w) AM_BASE(&paletteram16)

	AM_RANGE(0x500000, 0x500001) AM_WRITE(MWA16_NOP)	// ??? - check code at 0x00042e (executed once at startup)

	AM_RANGE(0xd00000, 0xd001ff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0xf00000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END


INPUT_PORTS_START( sderby )
	PORT_START	/* 0x308000.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )		// "Bet"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )		// "Collect" ?
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )		// Adds n credits depending on settings
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x1000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )		// check code at 0x00765e
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(4,5),RGN_FRAC(3,5),RGN_FRAC(2,5),RGN_FRAC(1,5),RGN_FRAC(0,5) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(4,5),RGN_FRAC(3,5),RGN_FRAC(2,5),RGN_FRAC(1,5),RGN_FRAC(0,5) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	 128+0, 128+1,128+2,128+3,128+4,128+5,128+6,128+7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	64+0*8,64+1*8,64+2*8,64+3*8,64+4*8,64+5*8,64+6*8,64+7*8

	 },

	256,
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout,   0x000, 256  }, /* sprites */
	{ REGION_GFX1, 0, &tiles16x16_layout,   0x000, 256  }, /* sprites */

	{ -1 } /* end of array */
};

static struct OKIM6295interface okim6295_interface =
{
	1,  /* 1 chip */
	{ 8000 },
	{ REGION_SOUND1 },
	{ 100 }
};


static MACHINE_DRIVER_START( sderby )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(sderby_readmem,sderby_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(4*8, 44*8-1, 3*8, 33*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(sderby)
	MDRV_VIDEO_UPDATE(sderby)

	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( sderby )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "22.bin", 0x00000, 0x20000, CRC(a319f1e0) SHA1(d932cc7e990aa87308dcd9ffa5af2aaea333aa9a) )
	ROM_LOAD16_BYTE( "23.bin", 0x00001, 0x20000, CRC(1d6e2321) SHA1(3bb32021cc9ee6bd6d1fd79a89159fef70f34f41) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "21.bin", 0x00000, 0x80000, CRC(6f9f2f2b) SHA1(9778439979bc21b3e49f0c16353488a33b93c01b) )

	ROM_REGION( 0xa0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "24.bin", 0x00000, 0x20000, CRC(93c917df) SHA1(dc2fa5e29749ec92871c66146c0412a23f47e316) )
	ROM_LOAD( "25.bin", 0x20000, 0x20000, CRC(7ba485cd) SHA1(b0170614d713af9d1556251c76ae762de872abe6) )
	ROM_LOAD( "26.bin", 0x40000, 0x20000, CRC(beabe4f7) SHA1(a5615450fae930cb2408f201a9faa12551de0d70) )
	ROM_LOAD( "27.bin", 0x60000, 0x20000, CRC(672ce5df) SHA1(cdf3af842cbcbf53cc73d9986744dc9cfa92c71a) )
	ROM_LOAD( "28.bin", 0x80000, 0x20000, CRC(39ca3b52) SHA1(9a03e73d88a1551cd3cfe616ab71e67dced1272a) )
ROM_END

GAME( 1996, sderby, 0, sderby, sderby, 0, ROT0, "Playmark", "Super Derby" )
