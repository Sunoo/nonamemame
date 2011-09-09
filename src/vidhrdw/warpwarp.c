/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"



int geebee_handleoverlay;
int geebee_bgw;
int warpwarp_ball_on;
int warpwarp_ball_h,warpwarp_ball_v;
int warpwarp_ball_sizex, warpwarp_ball_sizey;


static unsigned char geebee_palette[] =
{
	0x00,0x00,0x00, /* black */
	0xff,0xff,0xff, /* white */
	0x7f,0x7f,0x7f  /* grey  */
};

static unsigned short geebee_colortable[] =
{
	 0, 1,
	 1, 0,
	 0, 2,
	 2, 0
};

static unsigned short navarone_colortable[] =
{
	 0, 2,
	 2, 0,
};


/* Initialise the palette */
PALETTE_INIT( geebee )
{
	int i;
	for (i = 0; i < sizeof(geebee_palette)/3; i++)
		palette_set_color(i,geebee_palette[i*3+0],geebee_palette[i*3+1],geebee_palette[i*3+2]);
	memcpy(colortable, geebee_colortable, sizeof (geebee_colortable));
}

/* Initialise the palette */
PALETTE_INIT( navarone )
{
	int i;
	for (i = 0; i < sizeof(geebee_palette)/3; i++)
		palette_set_color(i,geebee_palette[i*3+0],geebee_palette[i*3+1],geebee_palette[i*3+2]);
	memcpy(colortable, navarone_colortable, sizeof (navarone_colortable));
}


/***************************************************************************

  Warp Warp doesn't use PROMs - the 8-bit code is directly converted into a
  color.

  The color RAM is connected to the RGB output this way (I think - schematics
  are fuzzy):

  bit 7 -- 300 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 300 ohm resistor  -- GREEN
        -- 820 ohm resistor  -- GREEN
        -- 1.6kohm resistor  -- GREEN
        -- 300 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
  bit 0 -- 1.6kohm resistor  -- RED

  Moreover, the bullet is pure white, obtained with three 220 ohm resistors.

***************************************************************************/
PALETTE_INIT( warpwarp )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		r = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;
		/* green component */
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		g = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		b = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;

		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < Machine->drv->color_table_len;i += 2)
	{
		colortable[i] = 0;			/* black background */
		colortable[i + 1] = i / 2;	/* colored foreground */
	}
}



INLINE void geebee_plot(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int x, int y, int pen)
{
	if (x >= cliprect->min_x && x <= cliprect->max_x && y >= cliprect->min_y && y <= cliprect->max_y)
		plot_pixel(bitmap,x,y,pen);
}

static void draw_ball(struct mame_bitmap *bitmap, const struct rectangle *cliprect,int color)
{
	if (warpwarp_ball_on)
	{
		int x = 256+8 - warpwarp_ball_h;
		int y = 256 - warpwarp_ball_v;
		int i,j;

		for (i = warpwarp_ball_sizey;i > 0;i--)
		{
			for (j = warpwarp_ball_sizex;j > 0;j--)
			{
				geebee_plot(bitmap, cliprect, x-j, y-i, color);
			}
		}
	}
}

VIDEO_UPDATE( geebee )
{
	int offs;

	/* use an overlay only in upright mode */
	if (geebee_handleoverlay)
		artwork_show(OVERLAY_TAG, (readinputport(2) & 0x01) == 0);

	if (get_vh_global_attribute_changed())
        memset(dirtybuffer, 1, videoram_size);

	for (offs = 0; offs < videoram_size; offs++)
	{
		if( dirtybuffer[offs] )
		{
			int mx,my,sx,sy,code,color;

			dirtybuffer[offs] = 0;

			mx = offs % 32;
			my = offs / 32;

			if (my == 0)
			{
				sx = 33;
				sy = mx;
			}
			else if (my == 1)
			{
				sx = 0;
				sy = mx;
			}
			else
			{
				sx = mx+1;
				sy = my;
			}

			if (flip_screen)
			{
				sx = 33 - sx;
				sy = 31 - sy;
			}

			code = videoram[offs];
			color = (geebee_bgw & 1) | ((code & 0x80) >> 6);
			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					color,
					flip_screen,flip_screen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);

	draw_ball(bitmap,cliprect,1);
}



VIDEO_UPDATE( warpwarp )
{
	int offs;

	if (get_vh_global_attribute_changed())
        memset(dirtybuffer, 1, videoram_size);

	for (offs = 0; offs < videoram_size; offs++)
	{
		if (dirtybuffer[offs])
		{
			int mx,my,sx,sy;

			dirtybuffer[offs] = 0;

			mx = offs % 32;
			my = offs / 32;

			if (my == 0)
			{
				sx = 33;
				sy = mx;
			}
			else if (my == 1)
			{
				sx = 0;
				sy = mx;
			}
			else
			{
				sx = mx + 1;
				sy = my;
			}

			if (flip_screen)
			{
				sx = 33 - sx;
				sy = 31 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					flip_screen,flip_screen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);

	draw_ball(bitmap,cliprect,0xf6);
}
