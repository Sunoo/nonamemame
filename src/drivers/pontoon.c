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
- Work needs to be done to find the hopper overflow sensors, as fruit machine emulation precedent suggests
that these switches be set 'always on'

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/pontoon.c"


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

static ADDRESS_MAP_START( lcreadmem , ADDRESS_SPACE_PROGRAM, 8  )
	AM_RANGE(0x0000, 0x5fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9000, 0x97ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r)
	AM_RANGE(0xa002, 0xa002) AM_READ(input_port_2_r)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lcwritemem, ADDRESS_SPACE_PROGRAM, 8  )
AM_RANGE(0x0000, 0x5fff) AM_WRITE(MWA8_ROM)
AM_RANGE(0x6000, 0x67ff) AM_WRITE(MWA8_RAM)AM_BASE(&generic_nvram)AM_SIZE(&generic_nvram_size)
AM_RANGE(0x9000, 0x93ff) AM_WRITE(videoram_w)AM_BASE (&videoram) AM_SIZE(&videoram_size)
AM_RANGE(0x9400, 0x97ff) AM_WRITE(colorram_w)AM_BASE (&colorram)
AM_RANGE(0xa001, 0xa001) AM_WRITE(MWA8_NOP ) /* outputs */
AM_RANGE(0xa002, 0xa002) AM_WRITE(MWA8_NOP ) /* outputs */
AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_ROM )
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
	PORT_BITX(0x08, IP_ACTIVE_LOW,  0, "Clear Stats", KEYCODE_F7, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  0, "Call Attendant", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  0, "Reset Hopper", KEYCODE_9, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,  IPT_UNUSED ) /*  ne donne rien */

	PORT_START      /* IN1 */
	PORT_BIT(0x07, IP_ACTIVE_LOW,  IPT_UNUSED ) /* ne donne rien */
	PORT_BITX(0x08, IP_ACTIVE_LOW,  IPT_BUTTON1, "Bonus Game", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_BUTTON2, "Stand",      IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW,  IPT_BUTTON3, "Hit",        IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON5)
	PORT_BITX(0x80, IP_ACTIVE_LOW,  IPT_BUTTON4, "Pay Out",    IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )   /* Token Drop */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )	/* Token In */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )	/* Overflow - Set to be high permanently (full hopper)*/ 
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON6, "Coin Out sensor (1 per press)", KEYCODE_ENTER, IP_JOY_NONE )
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


INPUT_PORTS_START( lvcards )
	PORT_START
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "Black", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Red", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Analyzer", KEYCODE_0, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Bet", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Deal / Double", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Hold 5", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Hold 4", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Hold 3", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Hold 2", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Hold 1", KEYCODE_A, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Cancel / Take Score", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* dsw0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Reset?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x07, 0x07, "1 COIN = " )
	PORT_DIPSETTING(    0x06, "5$" )
	PORT_DIPSETTING(    0x05, "10$" )
	PORT_DIPSETTING(    0x04, "15$" )
	PORT_DIPSETTING(    0x03, "20$" )
	PORT_DIPSETTING(    0x07, "25$" )
	PORT_DIPSETTING(    0x02, "50$" )
	PORT_DIPSETTING(    0x01, "75$" )
	PORT_DIPSETTING(    0x00, "100$" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( lvpoker )
    PORT_START
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "Black", KEYCODE_X, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Red", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Error Reset", KEYCODE_2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Memory Reset", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Analyzer", KEYCODE_0, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "Payout", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Bet", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Deal / Double", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Hold 5", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Hold 4", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Hold 3", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Hold 2", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Hold 1", KEYCODE_A, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Coinout Sensor", KEYCODE_4, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN3, 3 ) /* TOKEN IN */
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 3 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Cancel / Take Score", KEYCODE_SPACE, IP_JOY_NONE )
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* dsw0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Enable Memory Reset Key" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

    PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

static struct GfxLayout lccharlayout =
{
	8,8,    /* 8*8 characters */
	2048,   /* 2048 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{4,0,12,8,20,16,28,24},
	{32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7},
	32*8
};

static struct GfxDecodeInfo lcgfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lccharlayout, 0, 16 },
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


static struct AY8910interface lcay8910_interface =
{
	1,	 /* 1 chip */
	18432000/12, /* 1.536 MHz */
	{ 25, 25 },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 },
};

static MACHINE_DRIVER_START( lvcards )

	/* basic machine hardware */
 	MDRV_CPU_ADD(Z80, 18432000/6)	/*3.072 MHz*/

	MDRV_CPU_PROGRAM_MAP(lcreadmem,lcwritemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(8*0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(lcgfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
MDRV_COLORTABLE_LENGTH(256)
	MDRV_NVRAM_HANDLER(pontoon)

MDRV_PALETTE_INIT(lvcards)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(pontoon)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, lcay8910_interface)
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

ROM_START( lvcards )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "lc1.bin", 0x0000,  0x4000, CRC(0c5fbf05) SHA1(bf996bdccfc5748cee91d004f2b1da10bcd8e329))
	ROM_LOAD( "lc2.bin", 0x4000,  0x2000, CRC(deb54548) SHA1(a245898635c5cd3c26989c2bba89bb71edacd906))
	ROM_LOAD( "lc3.bin", 0xc000,  0x2000, CRC(45c2bea9) SHA1(3a33501824769656aa87649c3fd0a8b8a4d83f3c))

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* gfx */
	ROM_LOAD( "lc4.bin", 0x0000,  0x2000, CRC(dd705389) SHA1(271c11c2bd9affd976d65e318fd9fb01dbdde040))
	ROM_CONTINUE(      0x8000,  0x2000 )
	ROM_LOAD( "lc5.bin", 0x2000,  0x2000, CRC(ddd1e3e5) SHA1(b7e8ccaab318b61b91eae4eee9e04606f9717037))
	ROM_CONTINUE(      0xa000,  0x2000 )
	ROM_LOAD( "lc6.bin", 0x4000,  0x2000,BAD_DUMP  CRC(2991a6ec) SHA1(b2c32550884b7b708db48bb7f0854bbad504417d)) //Tiles are corrupt, making some cards unreadable
	ROM_RELOAD(        0xc000,  0x2000 )
	ROM_LOAD( "lc7.bin", 0x6000,  0x2000, CRC(f1b84c56) SHA1(6834139400bf8aa8db17f65bfdcbdcb2433d5fdc))
	ROM_RELOAD(        0xe000,  0x2000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "3.7c", 0x0000,  0x0100, CRC(0c2ead4a) SHA1(e8e29e21366622c9bf3ee5e807c83b5cd7da8e3e))
	ROM_LOAD( "2.7b", 0x0100,  0x0100, CRC(f4bc51e2) SHA1(38293c1117d6f3076081b6f2da3f42819558b04f))
	ROM_LOAD( "1.7a", 0x0200,  0x0100, CRC(e40f2363) SHA1(cea598b6ed037dd3b4306c2ca3b0b4d5197d42a4)) 
ROM_END

ROM_START( lvpoker )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "lp1.bin", 0x0000,  0x4000, CRC(ecfefa91) SHA1(7f6e0f30a2c4299797a8066419bf247b2900e312))
	ROM_LOAD( "lp2.bin", 0x4000,  0x2000, CRC(06d5484f) SHA1(326756a03eaeefc944428c7e011fcdc128aa415a))
	ROM_LOAD( "lp3.bin", 0xc000,  0x2000, CRC(05e17de8) SHA1(76b38e414f225789de8af9ca0556008e17285ffe))

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* gfx */
	ROM_LOAD( "lp4.bin", 0x0000,  0x2000, CRC(04fd2a6b) SHA1(33fb42f54646dc91f5aca1c55cfc932fa04f5d77))
	ROM_CONTINUE(      0x8000,  0x2000 )
	ROM_LOAD( "lp5.bin", 0x2000,  0x2000, CRC(9b5b531c) SHA1(1ce700361ea39a15c9c62fc0fa61df0cda62a340))
	ROM_CONTINUE(      0xa000,  0x2000 )
	ROM_LOAD( "lc6.bin", 0x4000,  0x2000,BAD_DUMP CRC(2991a6ec) SHA1(b2c32550884b7b708db48bb7f0854bbad504417d) ) //Tiles are corrupt, making some cards unreadable
	ROM_RELOAD(        0xc000,  0x2000 )
	ROM_LOAD( "lp7.bin", 0x6000,  0x2000, CRC(217e9cfb) SHA1(3e7d76db5195c599c2bf186ae6616b29bc0fd337))
	ROM_RELOAD(        0xe000,  0x2000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "3.7c", 0x0000,  0x0100, CRC(0c2ead4a) SHA1(e8e29e21366622c9bf3ee5e807c83b5cd7da8e3e))
	ROM_LOAD( "2.7b", 0x0100,  0x0100, CRC(f4bc51e2) SHA1(38293c1117d6f3076081b6f2da3f42819558b04f))
	ROM_LOAD( "1.7a", 0x0200,  0x0100, CRC(e40f2363) SHA1(cea598b6ed037dd3b4306c2ca3b0b4d5197d42a4))
ROM_END

GAME( 1985, lvcards, 0,       lvcards, lvcards, 0, ROT0, "Tehkan", "Lovely Cards" )
GAME( 1985, lvpoker, lvcards, lvcards, lvpoker, 0, ROT0, "Tehkan", "Lovely Poker [BET]" )
GAME( 1985, pontoon, 0, pontoon, pontoon, 0, ROT0, "Tehkan", "Pontoon (Tehkan)" )
