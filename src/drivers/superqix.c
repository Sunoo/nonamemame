/***************************************************************************

Super QIX memory map (preliminary)

driver by Mirko Buffoni

CPU:
0000-7fff ROM
8000-bfff BANK 0-1-2-3	(All banks except 2 contain code)

Notes:
- the original doesn't work due to protection. There is an unknown ROM: code
  for a mcu?

sqixa readme contains the following information
THERE IS ALSO AN 8751H AT LOCATION 3L  ( NOT READ )

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern WRITE8_HANDLER( superqix_videoram_w );
extern WRITE8_HANDLER( superqix_colorram_w );
extern READ8_HANDLER( superqix_bitmapram_r );
extern WRITE8_HANDLER( superqix_bitmapram_w );
extern READ8_HANDLER( superqix_bitmapram2_r );
extern WRITE8_HANDLER( superqix_bitmapram2_w );
extern WRITE8_HANDLER( superqix_0410_w );
extern WRITE8_HANDLER( superqix_flipscreen_w );

extern VIDEO_START( superqix );
extern VIDEO_UPDATE( superqix );


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xe0ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe100, 0xe7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe800, 0xebff) AM_WRITE(superqix_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xec00, 0xefff) AM_WRITE(superqix_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_READ(paletteram_r)
	AM_RANGE(0x0401, 0x0401) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x0405, 0x0405) AM_READ(AY8910_read_port_1_r)
	AM_RANGE(0x0418, 0x0418) AM_READ(input_port_4_r)
	AM_RANGE(0x0800, 0x77ff) AM_READ(superqix_bitmapram_r)
	AM_RANGE(0x8800, 0xf7ff) AM_READ(superqix_bitmapram2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_WRITE(paletteram_BBGGRRII_w)
	AM_RANGE(0x0402, 0x0402) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0403, 0x0403) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0406, 0x0406) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x0407, 0x0407) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x0408, 0x0408) AM_WRITE(superqix_flipscreen_w)
	AM_RANGE(0x0410, 0x0410) AM_WRITE(superqix_0410_w)	/* ROM bank, NMI enable, tile bank */
	AM_RANGE(0x0800, 0x77ff) AM_WRITE(superqix_bitmapram_w)
	AM_RANGE(0x8800, 0xf7ff) AM_WRITE(superqix_bitmapram2_w)
ADDRESS_MAP_END



INPUT_PORTS_START( superqix )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )	/* ??? */
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(    0x80, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME("Freeze???")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 50000" )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x04, "50000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Fill Area" )
	PORT_DIPSETTING(    0x80, "70%" )
	PORT_DIPSETTING(    0xc0, "75%" )
	PORT_DIPSETTING(    0x40, "80%" )
	PORT_DIPSETTING(    0x00, "85%" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	1024,
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,   0, 16 },	/* Chars */
	{ REGION_GFX2, 0x00000, &charlayout,   0, 16 },	/* Background tiles */
	{ REGION_GFX2, 0x08000, &charlayout,   0, 16 },
	{ REGION_GFX2, 0x10000, &charlayout,   0, 16 },
	{ REGION_GFX2, 0x18000, &charlayout,   0, 16 },
	{ REGION_GFX3, 0x00000, &spritelayout, 0, 16 },	/* Sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz??? */
	{ 25, 25 },
	{ input_port_0_r, input_port_3_r },		/* port Aread */
	{ input_port_1_r, input_port_2_r },		/* port Bread */
	{ 0 },	/* port Awrite */
	{ 0 }	/* port Bwrite */
};

INTERRUPT_GEN( sqix_interrupt )
{
	static int loop=0;

	loop++;

	if(loop>2) {
		if(loop==6) loop=0;
		nmi_line_pulse();
	}
}

static MACHINE_DRIVER_START( superqix )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz ? */
//			10000000,	/* 10 MHz ? */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
//	MDRV_CPU_VBLANK_INT(nmi_line_pulse,3)	/* ??? */
	MDRV_CPU_VBLANK_INT(sqix_interrupt,6)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(superqix)
	MDRV_VIDEO_UPDATE(superqix)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sqix )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sq01.97",      0x00000, 0x08000, CRC(0888b7de) SHA1(de3e4637436de185f43d2ad4186d4cfdcd4d33d9) )
	ROM_LOAD( "sq02.96",      0x10000, 0x10000, CRC(9c23cb64) SHA1(7e04cb18cabdc0031621162cbc228cd95875a022) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sq04.2",       0x00000, 0x08000, CRC(f815ef45) SHA1(4189d455b6ccf3ae922d410fb624c4665203febf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sq03.3",       0x00000, 0x10000, CRC(6e8b6a67) SHA1(c71117cc880a124c46397c446d1edc1cbf681200) )
	ROM_LOAD( "sq06.14",      0x10000, 0x10000, CRC(38154517) SHA1(703ad4cfe54a4786c67aedcca5998b57f39fd857) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sq05.1",       0x00000, 0x10000, CRC(df326540) SHA1(1fe025edcd38202e24c4e1005f478b6a88533453) )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* Unknown (protection related?) */
	ROM_LOAD( "sq07.108",     0x00000, 0x1000, CRC(071a598c) SHA1(2726705c3b82f5703e856261cdec5e86d7e1994e) )	// FIXED BITS (xxxx1xxx)
ROM_END

ROM_START( sqixa )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "bo301-1",      0x00000, 0x08000, CRC(ad614117) SHA1(c461f00a2aecde1bc3860c15a3c31091b14665a2) )
	ROM_LOAD( "sq02.96",      0x10000, 0x10000, CRC(9c23cb64) SHA1(7e04cb18cabdc0031621162cbc228cd95875a022) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sq04.2",       0x00000, 0x08000, CRC(f815ef45) SHA1(4189d455b6ccf3ae922d410fb624c4665203febf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sq03.3",       0x00000, 0x10000, CRC(6e8b6a67) SHA1(c71117cc880a124c46397c446d1edc1cbf681200) )
	ROM_LOAD( "sq06.14",      0x10000, 0x10000, CRC(38154517) SHA1(703ad4cfe54a4786c67aedcca5998b57f39fd857) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sq05.1",       0x00000, 0x10000, CRC(df326540) SHA1(1fe025edcd38202e24c4e1005f478b6a88533453) )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* Unknown (protection related?) */
	ROM_LOAD( "sq07.108",     0x00000, 0x1000, CRC(071a598c) SHA1(2726705c3b82f5703e856261cdec5e86d7e1994e) )	// FIXED BITS (xxxx1xxx)
ROM_END

ROM_START( sqixbl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu.2",        0x00000, 0x08000, CRC(682e28e3) SHA1(fe9221d26d7397be5a0fc8fdc51672b5924f3cf2) )
	ROM_LOAD( "sq02.96",      0x10000, 0x10000, CRC(9c23cb64) SHA1(7e04cb18cabdc0031621162cbc228cd95875a022) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sq04.2",       0x00000, 0x08000, CRC(f815ef45) SHA1(4189d455b6ccf3ae922d410fb624c4665203febf) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sq03.3",       0x00000, 0x10000, CRC(6e8b6a67) SHA1(c71117cc880a124c46397c446d1edc1cbf681200) )
	ROM_LOAD( "sq06.14",      0x10000, 0x10000, CRC(38154517) SHA1(703ad4cfe54a4786c67aedcca5998b57f39fd857) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sq05.1",       0x00000, 0x10000, CRC(df326540) SHA1(1fe025edcd38202e24c4e1005f478b6a88533453) )
ROM_END

ROM_START( perestrf )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	/* 0x8000 - 0x10000 in the rom is empty anyway */
	ROM_LOAD( "rom1.bin",        0x00000, 0x20000, CRC(0cbf96c1) SHA1(cf2b1367887d1b8812a56aa55593e742578f220c) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",       0x00000, 0x10000, CRC(c56122a8) SHA1(1d24b2f0358e14aca5681f92175869224584a6ea) ) /* both halves identical */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2.bin",       0x00000, 0x20000, CRC(36f93701) SHA1(452cb23efd955c6c155cef2b1b650e253e195738) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rom3.bin",       0x00000, 0x10000, CRC(00c91d5a) SHA1(fdde56d3689a47e6bfb296e442207b93b887ec7a) )
ROM_END

ROM_START( perestro )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 64k for code */
	/* 0x8000 - 0x10000 in the rom is empty anyway */
	ROM_LOAD( "rom1.bin",        0x00000, 0x20000, CRC(0cbf96c1) SHA1(cf2b1367887d1b8812a56aa55593e742578f220c) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",       0x00000, 0x10000, CRC(c56122a8) SHA1(1d24b2f0358e14aca5681f92175869224584a6ea) ) /* both halves identical */

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2.bin",       0x00000, 0x20000, CRC(36f93701) SHA1(452cb23efd955c6c155cef2b1b650e253e195738) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rom3a.bin",       0x00000, 0x10000, CRC(7a2a563f) SHA1(e3654091b858cc80ec1991281447fc3622a0d4f9) )
ROM_END

static DRIVER_INIT(perestro)
{
	data8_t *src;
	int len;
	data8_t temp[16];
	int i,j;

	/* decrypt program code; the address lines are shuffled around in a non-trivial way */
	src = memory_region(REGION_CPU1);
	len = memory_region_length(REGION_CPU1);
	for (i = 0;i < len;i += 16)
	{
		memcpy(temp,&src[i],16);
		for (j = 0;j < 16;j++)
		{
			static int convtable[16] =
			{
				0xc, 0x9, 0xb, 0xa,
				0x8, 0xd, 0xf, 0xe,
				0x4, 0x1, 0x3, 0x2,
				0x0, 0x5, 0x7, 0x6
			};

			src[i+j] = temp[convtable[j]];
		}
	}

	/* decrypt gfx ROMs; simple bit swap on the address lines */
	src = memory_region(REGION_GFX1);
	len = memory_region_length(REGION_GFX1);
	for (i = 0;i < len;i += 16)
	{
		memcpy(temp,&src[i],16);
		for (j = 0;j < 16;j++)
		{
			src[i+j] = temp[BITSWAP8(j,7,6,5,4,3,2,0,1)];
		}
	}

	src = memory_region(REGION_GFX2);
	len = memory_region_length(REGION_GFX2);
	for (i = 0;i < len;i += 16)
	{
		memcpy(temp,&src[i],16);
		for (j = 0;j < 16;j++)
		{
			src[i+j] = temp[BITSWAP8(j,7,6,5,4,0,1,2,3)];
		}
	}

	src = memory_region(REGION_GFX3);
	len = memory_region_length(REGION_GFX3);
	for (i = 0;i < len;i += 16)
	{
		memcpy(temp,&src[i],16);
		for (j = 0;j < 16;j++)
		{
			src[i+j] = temp[BITSWAP8(j,7,6,5,4,1,0,3,2)];
		}
	}
}


GAMEX(1987, sqix,     0,        superqix, superqix, 0,        ROT90, "Taito", "Super Qix (set 1)", GAME_NOT_WORKING )
GAMEX(1987, sqixa,    sqix,     superqix, superqix, 0,        ROT90, "Taito", "Super Qix (set 2)", GAME_NOT_WORKING )
GAME( 1987, sqixbl,   sqix,     superqix, superqix, 0,        ROT90, "bootleg", "Super Qix (bootleg)" )
GAME( 1994, perestro, 0,        superqix, superqix, perestro, ROT90, "Promat", "Perestroika Girls" )
GAME( 1993, perestrf, perestro, superqix, superqix, perestro, ROT90, "Promat (Fuuki license)", "Perestroika Girls (Fuuki license)" )
