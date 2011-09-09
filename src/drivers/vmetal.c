/*

Varia Metal

Notes:

It has Sega and Taito logos in the roms ?!

whats going on with the dipswitches
also has a strange sound chip + oki

scrolling behavior is incorrect, see background on second attract demo
tilemap priorities can change

spriteram clear or list markers? (i clear it after drawing each sprite at the moment)

cleanup


---

Excellent Systems 'Varia Metal'
board ID ES-9309B-B

main cpu 68000 @ 16Mhz

sound oki m6295 (rom VM8)
      es8712    (rom VM7)

program roms VM5 and VM6

graphics VM1-VM4

roms are 23C160 except for code and OKI 27C4001


*/

#include "driver.h"

data16_t *vmetal_texttileram;
data16_t *vmetal_mid1tileram;
data16_t *vmetal_mid2tileram;
data16_t *vmetal_tlookup;
data16_t *vmetal_videoregs;


static struct tilemap *vmetal_texttilemap;
static struct tilemap *vmetal_mid1tilemap;
static struct tilemap *vmetal_mid2tilemap;

static data16_t *varia_spriteram16;

READ16_HANDLER ( varia_crom_read )
{
	/* game reads the cgrom, result is 7772, verified to be correct on the real board */

	data8_t *cgrom = memory_region(REGION_GFX1);
	data16_t retdat;
	offset = offset << 1;
	offset |= (vmetal_videoregs[0x0ab/2]&0x7f) << 16;
	retdat = (cgrom[offset] <<8)| (cgrom[offset+1]);
	usrintf_showmessage("varia romread offset %06x data %04x",offset, retdat);

	return retdat;
}


READ16_HANDLER ( varia_random )
{
//	return rand();  // dips etc.. weird
	return 0xffff;
}



static void get_vmetal_tlookup(UINT16 data, UINT16 *tileno, UINT16 *color)
{
	int idx = ((data & 0x7fff) >> 4)*2;
	UINT32 lookup = (vmetal_tlookup[idx]<<16) | vmetal_tlookup[idx+1];
	*tileno = (data & 0xf) | ((lookup>>2) & 0xfff0);
	*color = (lookup>>20) & 0xff;
}

/* sprite format

sprites are non-tile based

4 words per sprite

 -------- --------    -------- --------    -------- --------    -------- --------
       xx xxxxxxxx           y yyyyyyyy    Ff             oo    oooooooo oooooooo


  x = xpos
  y = ypos
  f = flipy
  F = flipx
  o = offset (64 byte sprite gfxdata boundaries)




*/

static void varia_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	data16_t *finish = varia_spriteram16;
	data16_t *source = finish + 0x1000/2;

	UINT16 *destline;

	const struct GfxElement *gfx = Machine->gfx[0];

	data8_t *srcgfx;

	int gfxoffs;

	while( source>finish )
	{
		source -= 0x04;

		int x,y,xloop,yloop;
		UINT16 height, width;
		int tile;
		int flipy, flipx;
		int colour;
		x = source[0]&0x3ff;
		y = source[1]& 0x1ff;
		flipy = source[2]&0x4000;
		flipx = source[2]&0x8000;
		gfxoffs = 0;
		tile=(source[3]) | ((source[2]&0x3)<<16);
		srcgfx= gfx->gfxdata+(64*tile);
		width = (((source[2]&0x3000) >> 12)+1)*16;
		height =(((source[2]&0x0600) >> 9)+1)*16;
		colour =((source[2]&0x00f0) >> 4);

		y-=64;
		x-=64;


		for (yloop=0; yloop<height; yloop++)
		{
			UINT16 drawypos;

			if (!flipy) {drawypos = y+yloop;} else {drawypos = (y+height-1)-yloop;}

			destline = (UINT16 *)(bitmap->line[drawypos]);


			for (xloop=0; xloop<width; xloop++)
			{
				UINT16 drawxpos;
				int pixdata;
				pixdata = srcgfx[gfxoffs];

				if (!flipx) { drawxpos = x+xloop; } else { drawxpos = (x+width-1)-xloop; }

				if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x) && (drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y) && (pixdata!=15))
					destline[drawxpos] = pixdata + ((0x80+colour)*16);


				gfxoffs++;
			}


		}

		/* I see no end of list marker or register ... so I'm clearing the sprite ram after I draw each sprite */
	//	source[0] = source[1] = source[2] = source[3] = 0;
	// done in VIDEO_EOF
	}
}

WRITE16_HANDLER( vmetal_texttileram_w )
{
	COMBINE_DATA(&vmetal_texttileram[offset]);
	tilemap_mark_tile_dirty(vmetal_texttilemap,offset);
}

WRITE16_HANDLER( vmetal_mid1tileram_w )
{
	COMBINE_DATA(&vmetal_mid1tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid1tilemap,offset);
}
WRITE16_HANDLER( vmetal_mid2tileram_w )
{
	COMBINE_DATA(&vmetal_mid2tileram[offset]);
	tilemap_mark_tile_dirty(vmetal_mid2tilemap,offset);
}

WRITE16_HANDLER( paletteram16_GGGGGRRRRRBBBBBx_word_w )
{
	int r,g,b;
	UINT16 datax;
	COMBINE_DATA(&paletteram16[offset]);
	datax = paletteram16[offset];

	r = (datax >>  6) & 0x1f;
	g = (datax >> 11) & 0x1f;
	b = (datax >>  1) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}

static ADDRESS_MAP_START( varia_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x120000, 0x13ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x15ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x160000, 0x16ffff) AM_READ(varia_crom_read) // cgrom read window ..

	AM_RANGE(0x170000, 0x171fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x172000, 0x173fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x174000, 0x17ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x200000, 0x200001) AM_READ(input_port_0_word_r )
	AM_RANGE(0x200002, 0x200003) AM_READ(input_port_1_word_r )

	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_2_word_r ) // sound reads?

	/* i have no idea whats meant to be going on here .. it seems to read one bit of the dips from some of them, protection ??? */

	AM_RANGE(0x30fffe, 0x30ffff) AM_READ(varia_random )

	AM_RANGE(0x317ffe, 0x317fff) AM_READ(varia_random )
	AM_RANGE(0x31bffe, 0x31bfff) AM_READ(varia_random )
	AM_RANGE(0x31dffe, 0x31dfff) AM_READ(varia_random )
	AM_RANGE(0x31effe, 0x31efff) AM_READ(varia_random )

	AM_RANGE(0x31f7fe, 0x31f7ff) AM_READ(varia_random )
	AM_RANGE(0x31fbfe, 0x31fbff) AM_READ(varia_random )
	AM_RANGE(0x31fdfe, 0x31fdff) AM_READ(varia_random )
	AM_RANGE(0x31fefe, 0x31feff) AM_READ(varia_random )

	AM_RANGE(0x31ff7e, 0x31ff7f) AM_READ(varia_random )
	AM_RANGE(0x31ffbe, 0x31ffbf) AM_READ(varia_random )
	AM_RANGE(0x31ffde, 0x31ffdf) AM_READ(varia_random )
	AM_RANGE(0x31ffee, 0x31ffef) AM_READ(varia_random )
	AM_RANGE(0x31fffe, 0x31ffff) AM_READ(varia_random )

	AM_RANGE(0x31fff6, 0x31fff7) AM_READ(varia_random )
	AM_RANGE(0x31fffa, 0x31fffb) AM_READ(varia_random )
	AM_RANGE(0x31fffc, 0x31fffd) AM_READ(varia_random )

	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( varia_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_WRITE(vmetal_texttileram_w) AM_BASE(&vmetal_texttileram)
	AM_RANGE(0x120000, 0x13ffff) AM_WRITE(vmetal_mid1tileram_w) AM_BASE(&vmetal_mid1tileram)
	AM_RANGE(0x140000, 0x15ffff) AM_WRITE(vmetal_mid2tileram_w) AM_BASE(&vmetal_mid2tileram)

	AM_RANGE(0x170000, 0x171fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x172000, 0x173fff) AM_WRITE(paletteram16_GGGGGRRRRRBBBBBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x174000, 0x174fff) AM_WRITE(MWA16_RAM) AM_BASE(&varia_spriteram16)
	AM_RANGE(0x175000, 0x177fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x178000, 0x1787ff) AM_WRITE(MWA16_RAM) AM_BASE(&vmetal_tlookup)
	AM_RANGE(0x178800, 0x17ffff) AM_WRITE(MWA16_RAM) AM_BASE(&vmetal_videoregs)

	AM_RANGE(0x200000, 0x200001) AM_WRITE(MWA16_NOP) // sound writes?

	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM)

ADDRESS_MAP_END

INPUT_PORTS_START( varia )
	PORT_START	/* DSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )


	PORT_START		/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x0010, IP_ACTIVE_LOW )				// "Test Mode"
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown )  )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )


	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, "2" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown )  )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout char16x16layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(3,4)+4, RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+12, RGN_FRAC(3,4)+8, 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxLayout char8x8layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+12, RGN_FRAC(2,4)+8 },
	{ RGN_FRAC(1,4)+0*16,0*16, RGN_FRAC(1,4)+1*16, 1*16, RGN_FRAC(1,4)+2*16,  2*16, RGN_FRAC(1,4)+3*16, 3*16 },
	4*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char16x16layout,   0x0, 512  }, /* bg tiles */
	{ REGION_GFX1, 0, &char8x8layout,   0x0, 512  }, /* bg tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

static void get_vmetal_texttilemap_tile_info(int tile_index)
{
	UINT32 tile;
	UINT16 color, data = vmetal_texttileram[tile_index];
	int idx = ((data & 0x7fff) >> 4)*2;
	UINT32 lookup = (vmetal_tlookup[idx]<<16) | vmetal_tlookup[idx+1];
	tile = (data & 0xf) | (lookup & 0x7fff0);
	color = ((lookup>>20) & 0x1f)+0xe0;
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(1, tile, color, TILE_FLIPYX(0x0));
}


static void get_vmetal_mid1tilemap_tile_info(int tile_index)
{
	UINT16 tile, color, data = vmetal_mid1tileram[tile_index];
	get_vmetal_tlookup(data, &tile, &color);
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(0, tile, color, TILE_FLIPYX(0x0));
}
static void get_vmetal_mid2tilemap_tile_info(int tile_index)
{
	UINT16 tile, color, data = vmetal_mid2tileram[tile_index];
	get_vmetal_tlookup(data, &tile, &color);
	if (data & 0x8000) tile = 0;
	SET_TILE_INFO(0, tile, color, TILE_FLIPYX(0x0));
}

VIDEO_START(varia)
{
	vmetal_texttilemap = tilemap_create(get_vmetal_texttilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      8, 8, 256,256);
	vmetal_mid1tilemap = tilemap_create(get_vmetal_mid1tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      16,16, 256,256);
	vmetal_mid2tilemap = tilemap_create(get_vmetal_mid2tilemap_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,      16,16, 256,256);
	tilemap_set_transparent_pen(vmetal_texttilemap,15);
	tilemap_set_transparent_pen(vmetal_mid1tilemap,15);
	tilemap_set_transparent_pen(vmetal_mid2tilemap,15);

	return 0;
}

VIDEO_UPDATE(varia)
{
	fillbitmap(bitmap, get_black_pen(), cliprect);

	tilemap_set_scrollx(vmetal_mid2tilemap,0, vmetal_videoregs[0x06a/2] /*+ vmetal_videoregs[0x066/2]*/);
	tilemap_set_scrollx(vmetal_mid1tilemap,0, vmetal_videoregs[0x07a/2] /*+ vmetal_videoregs[0x076/2]*/);


	tilemap_draw(bitmap,cliprect,vmetal_mid1tilemap,0,0);

	tilemap_draw(bitmap,cliprect,vmetal_mid2tilemap,0,0);

	varia_drawsprites (bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,vmetal_texttilemap,0,0);

}

VIDEO_EOF(varia)
{
	memset(varia_spriteram16, 0, 0x1000);
}

static MACHINE_DRIVER_START( varia )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(varia_readmem,varia_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1) // also level 3
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(2048, 2048)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(varia)
	MDRV_VIDEO_UPDATE(varia)
	MDRV_VIDEO_EOF(varia)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( vmetal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "vm5.bin", 0x00001, 0x80000, CRC(43ef844e) SHA1(c673f34fcc9e406282c9008795b52d01a240099a) )
	ROM_LOAD16_BYTE( "vm6.bin", 0x00000, 0x80000, CRC(cb292ab1) SHA1(41fdfe67e6cb848542fd5aa0dfde3b1936bb3a28) )


	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "vm1.bin", 0x000000, 0x200000, CRC(b470c168) SHA1(c30462dc134da1e71a94b36ef96ecd65c325b07e) )
	ROM_LOAD( "vm2.bin", 0x200000, 0x200000, CRC(b36f8d60) SHA1(1676859d0fee4eb9897ce1601a2c9fd9a6dc4a43) )
	ROM_LOAD( "vm3.bin", 0x400000, 0x200000, CRC(00fca765) SHA1(ca9010bd7f59367e483868018db9a9abf871386e) )
	ROM_LOAD( "vm4.bin", 0x600000, 0x200000, CRC(5a25a49c) SHA1(c30781202ec882e1ec6adfb560b0a1075b3cce55) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "vm8.bin", 0x00000, 0x80000, CRC(c14c001c) SHA1(bad96b5cd40d1c34ef8b702262168ecab8192fb6) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* more samples? */
	ROM_LOAD( "vm7.bin", 0x00000, 0x200000, CRC(a88c52f1) SHA1(d74a5a11f84ba6b1042b33a2c156a1071b6fbfe1) )
ROM_END

GAMEX( 1995, vmetal, 0, varia, varia, 0, ROT270, "Excellent Systems", "Varia Metal", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )