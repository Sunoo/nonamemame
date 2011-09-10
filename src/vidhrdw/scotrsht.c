#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *scotrsht_scroll;

static struct tilemap *bg_tilemap;
static int scotrsht_charbank = 0;

/* Similar as Iron Horse */
PALETTE_INIT( scotrsht )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the character lookup table */


	/* there are eight 32 colors palette banks; sprites use colors 0-15 and */
	/* characters 16-31 of each bank. */
	for (i = 0;i < TOTAL_COLORS(0)/8;i++)
	{
		int j;


		for (j = 0;j < 8;j++)
			COLOR(0,i + j * TOTAL_COLORS(0)/8) = (*color_prom & 0x0f) + 16 * j + 16 + 16;

		color_prom++;
	}

	for (i = 0;i < TOTAL_COLORS(1)/8;i++)
	{
		int j;


		for (j = 0;j < 8;j++)
			COLOR(1,i + j * TOTAL_COLORS(1)/8) = (*color_prom & 0x0f) + 32 * j;

		color_prom++;
	}
}

WRITE8_HANDLER( scotrsht_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( scotrsht_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( scotrsht_charbank_w )
{
	if (scotrsht_charbank != (data & 0x01))
	{
		scotrsht_charbank = data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	/* other bits unknown */
}


static void scotrsht_get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + (scotrsht_charbank << 9) + ((attr & 0x40) << 2);
	int color = attr & 0x0f;
	int flag = 0;

	if(attr & 0x10)
		flag |= TILE_FLIPX;

	if(attr & 0x20)
		flag |= TILE_FLIPY;

	/* wrong! */
	if((code & 300) == 0x300)
		color += 0;
	else if((code & 200) == 0x200)
		color += 0;
	else if((code & 0x100) == 0x100)
		color += 0x60;
	else
		color += 0x70;


	SET_TILE_INFO(0, code, color, flag)
}

/* Same as Jailbreak */
void scotrsht_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i;

	for (i = 0; i < spriteram_size; i += 4)
	{
		int attr = spriteram[i + 1];	// attributes = ?tyxcccc
		int code = spriteram[i] + ((attr & 0x40) << 2);
		int color = attr & 0x0f;
		int flipx = attr & 0x10;
		int flipy = attr & 0x20;
		int sx = spriteram[i + 2] - ((attr & 0x80) << 1);
		int sy = spriteram[i + 3];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy,
			sx, sy, cliprect, TRANSPARENCY_COLOR, 0);
	}
}

VIDEO_START( scotrsht )
{
	bg_tilemap = tilemap_create(scotrsht_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);

	tilemap_set_scroll_cols(bg_tilemap, 64);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( scotrsht )
{
	int col;

	for (col = 0; col < 32; col++)
		tilemap_set_scrolly(bg_tilemap, col, scotrsht_scroll[col]);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	scotrsht_draw_sprites(bitmap, cliprect);
}
