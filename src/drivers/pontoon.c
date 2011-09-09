/***************************************************************************

Pontoon Memory Map (preliminary)

driver by Zsolt Vasvari

Enter switch test mode by holding down the Hit key, and when the
crosshatch pattern comes up, release it and push it again.

After you get into Check Mode (F2), press the Hit key to switch pages.


Memory Mapped:

0000-5fff   R	ROM
6000-67ff   RW  Battery Backed RAM
8000-83ff   RW  Video RAM
8400-87ff   RW  Color RAM
				Bits 0-3 - color
 				Bits 4-5 - character bank
				Bit  6   - flip x
				Bit  7   - Is it used?
a000		R	Input Port 0
a001		R	Input Port 1
a002		R	Input Port 2
a001		 W  Control Port 0
a002		 W  Control Port 1

I/O Ports:
00			RW  YM2149 Data	Port
				Port A - DSW #1
				Port B - DSW #2
01           W  YM2149 Control Port


TODO:

- What do the control ports do?
- CPU speed/ YM2149 frequencies
- Input ports need to be cleaned up


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



PALETTE_INIT( pontoon );
VIDEO_UPDATE( pontoon );


static unsigned char *nvram;
static size_t nvram_size;

static NVRAM_HANDLER( pontoon )
{
	if (read_or_write)
		mame_fwrite(file,nvram,nvram_size);
	else
	{
		if (file)
			mame_fread(file,nvram,nvram_size);
		else
			memset(nvram,0xff,nvram_size);
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)
	AM_RANGE(0xa002, 0xa002) AM_READ(input_port_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(MWA8_RAM) AM_BASE(&nvram) AM_SIZE(&nvram_size)
	AM_RANGE(0x8000, 0x83ff) AM_WRITE(videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x8400, 0x87ff) AM_WRITE(colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xa001, 0xa002) AM_WRITE(MWA8_NOP)			  /* Probably lights and stuff */
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( pontoon )
	PORT_START      /* IN0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN3 )
	PORT_BITX(0x04, IP_ACTIVE_LOW,  0, "Reset All", KEYCODE_F5, IP_JOY_NONE )  /* including battery backed RAM */
	PORT_BITX(0x08, IP_ACTIVE_LOW,  0, "Clear Stats", KEYCODE_F6, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Call Attendant", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  0, "Reset Hopper", KEYCODE_7, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,  IPT_UNUSED ) /*  ne donne rien */

	PORT_START      /* IN1 */
	PORT_BIT(0x07, IP_ACTIVE_LOW,  IPT_UNUSED ) /* ne donne rien */
	PORT_BITX(0x08, IP_ACTIVE_LOW,  IPT_BUTTON1, "Bonus Game", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_BUTTON2, "Stand",      IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_BUTTON3, "Hit",        IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON5 )
	PORT_BITX(0x80, IP_ACTIVE_LOW,  IPT_BUTTON4, "Pay Out",    IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )   /* Token Drop */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )	/* Token In */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Overflow */
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Coin Out", KEYCODE_8, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Winning Percentage" )
	PORT_DIPSETTING(    0x06, "70%" )
	PORT_DIPSETTING(    0x05, "74%" )
	PORT_DIPSETTING(    0x04, "78%" )
	PORT_DIPSETTING(    0x03, "82%" )
	PORT_DIPSETTING(    0x02, "86%" )
	PORT_DIPSETTING(    0x07, "90%" )
	PORT_DIPSETTING(    0x01, "94%" )
	PORT_DIPSETTING(    0x00, "98%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )   /* Doesn't appear on the test menu */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )   /* Doesn't appear on the test menu */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )   /* Doesn't appear on the test menu */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x20, "Payment Method" )
	PORT_DIPSETTING(    0x00, "Credit In/Coin Out" )
	PORT_DIPSETTING(    0x20, "Coin In/Coin Out" )
	PORT_DIPSETTING(    0x40, "Credit In/Credit Out" )
  /*PORT_DIPSETTING(    0x60, "Credit In/Coin Out" ) */
	PORT_DIPNAME( 0x80, 0x80, "Reset All Switch" )
	PORT_DIPSETTING(    0x80, "Disable" )
	PORT_DIPSETTING(    0x00, "Enable" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Coin C" )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 2 chips */
	4608000,	/* 18.432000 / 4 (???) */
	{ 50 },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( pontoon )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4608000)	/* 18.432000 / 4 (???) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)  /* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)
	MDRV_NVRAM_HANDLER(pontoon)

	MDRV_PALETTE_INIT(pontoon)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(pontoon)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pontoon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )         /* 64k for code */
	ROM_LOAD( "ponttekh.001",   0x0000, 0x4000, CRC(1f8c1b38) SHA1(3776ddd695741223bd9ad41f74187bff31f2cd3b) )
	ROM_LOAD( "ponttekh.002",   0x4000, 0x2000, CRC(befb4f48) SHA1(8ca146c8b52afab5deb6f0ff52bdbb2b1ff3ded7) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ponttekh.003",   0x0000, 0x2000, CRC(a6a91b3d) SHA1(d180eabe67efd3fd1205570b661a74acf7ed93b3) )
	ROM_LOAD( "ponttekh.004",   0x2000, 0x2000, CRC(976ed924) SHA1(4d305694b3e157411068baf3052e3aac7d0b32d5) )
	ROM_LOAD( "ponttekh.005",   0x4000, 0x2000, CRC(2b8e8ca7) SHA1(dd86d3b4fd1627bdaa0603ffd2f1bc2953bc51f8) )
	ROM_LOAD( "ponttekh.006",   0x6000, 0x2000, CRC(6bc23965) SHA1(b73a584fc5b2dd9436bbb8bc1620f5a51d351aa8) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "pon24s10.003",   0x0000, 0x0100, CRC(4623b7f3) SHA1(55948753dec09d0a476b90ca75e7e092ce0f68ee) )  /* red component */
	ROM_LOAD( "pon24s10.002",   0x0100, 0x0100, CRC(117e1b67) SHA1(b753137878fe5cd650722cf526cd4929821240a8) )  /* green component */
	ROM_LOAD( "pon24s10.001",   0x0200, 0x0100, CRC(c64ecee8) SHA1(80c9ec21e135235f7f2d41ce7900cf3904123823) )  /* blue component */
ROM_END

GAME( 1985, pontoon, 0, pontoon, pontoon, 0, ROT0, "Tehkan", "Pontoon" )
