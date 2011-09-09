#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *text_layer;
static struct tilemap *back_layer;
static struct tilemap *mid_layer;
static struct tilemap *fore_layer;

UINT32 *back_ram, *mid_ram, *fore_ram, *scroll_ram;
UINT32 bg_2layer;

static UINT32 layer_bank;

READ32_HANDLER( spi_layer_bank_r )
{
	return layer_bank;
}

WRITE32_HANDLER( spi_layer_bank_w )
{
	COMBINE_DATA( &layer_bank );
}

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri_mask)
{
	int xpos, ypos, tile_num, color;
	int width, height;
	int flip_x = 0, flip_y = 0;
	int a;
	int x,y;
	const struct GfxElement *gfx = Machine->gfx[2];

	for( a = 0x400 - 2; a >= 0; a -= 2 ) {
		tile_num = (spriteram32[a + 0] >> 16) & 0xffff;

		if( !tile_num )
			continue;

		xpos = spriteram32[a + 1] & 0x3fff;
		if( xpos & 0x2000 )
			xpos |= 0xffffc000;
		ypos = (spriteram32[a + 1] >> 16) & 0x3fff;
		if( ypos & 0x2000 )
			ypos |= 0xffffc000;
		color = (spriteram32[a + 0] & 0x3f);

		width = ((spriteram32[a + 0] >> 8) & 0x7) + 1;
		height = ((spriteram32[a + 0] >> 12) & 0x7) + 1;

		for( x=0; x < width*16; x += 16 ) {
			for( y=0; y < height*16; y += 16 ) {			
				drawgfx(
					bitmap,
					gfx,
					tile_num,
					color, flip_x, flip_y, xpos + x, ypos + y,
					cliprect,
					TRANSPARENCY_PEN, 63
					);

				tile_num++;
			}
		}
	}
}

WRITE32_HANDLER( spi_paletteram32_xBBBBBGGGGGRRRRR_w )
{
	int r1,g1,b1,r2,g2,b2;
	COMBINE_DATA(&paletteram32[offset]);

	b1 = ((paletteram32[offset] & 0x7c000000) >>26) << 3;
	g1 = ((paletteram32[offset] & 0x03e00000) >>21) << 3;
	r1 = ((paletteram32[offset] & 0x001f0000) >>16) << 3;
	b2 = ((paletteram32[offset] & 0x00007c00) >>10) << 3;
	g2 = ((paletteram32[offset] & 0x000003e0) >>5) << 3;
	r2 = ((paletteram32[offset] & 0x0000001f) >>0) << 3;

	palette_set_color( (offset * 2), r2, g2, b2 );
	palette_set_color( (offset * 2) + 1, r1, g1, b1 );
}



WRITE32_HANDLER( spi_textlayer_w )
{
	COMBINE_DATA(&videoram32[offset]);
	tilemap_mark_tile_dirty( text_layer, (offset * 2) );
	tilemap_mark_tile_dirty( text_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_back_layer_w )
{
	COMBINE_DATA(&back_ram[offset]);
	tilemap_mark_tile_dirty( back_layer, (offset * 2) );
	tilemap_mark_tile_dirty( back_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_mid_layer_w )
{
	COMBINE_DATA(&mid_ram[offset]);
	tilemap_mark_tile_dirty( mid_layer, (offset * 2) );
	tilemap_mark_tile_dirty( mid_layer, (offset * 2) + 1 );
}

WRITE32_HANDLER( spi_fore_layer_w )
{
	COMBINE_DATA(&fore_ram[offset]);
	tilemap_mark_tile_dirty( fore_layer, (offset * 2) );
	tilemap_mark_tile_dirty( fore_layer, (offset * 2) + 1 );
}

static void get_text_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (videoram32[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0, tile, color/2, 0)
}

static void get_back_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (back_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0x1fff;
	color = 0;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_mid_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (mid_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0x1fff;
	color = 0;
	tile |= 0x2000;

	SET_TILE_INFO(1, tile, color, 0)
}

static void get_fore_tile_info( int tile_index )
{
	int offs = tile_index / 2;
	int tile = (fore_ram[offs] >> ((tile_index & 0x1) ? 16 : 0)) & 0xffff;
	int color = (tile >> 12) & 0xf;

	tile &= 0x1fff;
	color = 0;
	if( bg_2layer ) {
		tile |= 0x2000;
	} else {
		tile |= 0x4000;
	}
	tile |= ((layer_bank >> 27) & 0x1) << 13;

	SET_TILE_INFO(1, tile, color, 0)
}

VIDEO_START( spi )
{
	text_layer	= tilemap_create( get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32 );
	back_layer	= tilemap_create( get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 16,16,32,32 );
	mid_layer	= tilemap_create( get_mid_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );
	fore_layer	= tilemap_create( get_fore_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 16,16,32,32 );

	tilemap_set_transparent_pen(text_layer, 63);
	tilemap_set_transparent_pen(mid_layer, 63);
	tilemap_set_transparent_pen(fore_layer, 63);
	return 0;
}

static void set_scroll(struct tilemap *layer, int scroll)
{
	int x = scroll_ram[scroll] & 0xffff;
	int y = (scroll_ram[scroll] >> 16) & 0xffff;
	tilemap_set_scrollx(layer, 0, x);
	tilemap_set_scrolly(layer, 0, y);
}

VIDEO_UPDATE( spi )
{
	set_scroll(back_layer, 1);
	set_scroll(mid_layer, 2);
	set_scroll(fore_layer, 3);

	tilemap_draw(bitmap, cliprect, back_layer, 0,0);
	if( !bg_2layer )
		tilemap_draw(bitmap, cliprect, mid_layer, 0,0);
	tilemap_draw(bitmap, cliprect, fore_layer, 0,0);

	draw_sprites(bitmap, cliprect, 0);

	tilemap_draw(bitmap, cliprect, text_layer, 0,0);
}
