/*************************************************************************

	Venture Line Super Rider driver

**************************************************************************/

/*----------- defined in vidhrdw/suprridr.c -----------*/

data8_t *suprridr_fgram;
data8_t *suprridr_bgram;

VIDEO_START( suprridr );
PALETTE_INIT( suprridr );

WRITE_HANDLER( suprridr_flipx_w );
WRITE_HANDLER( suprridr_flipy_w );
WRITE_HANDLER( suprridr_fgdisable_w );
WRITE_HANDLER( suprridr_fgscrolly_w );
WRITE_HANDLER( suprridr_bgscrolly_w );

WRITE_HANDLER( suprridr_fgram_w );
WRITE_HANDLER( suprridr_bgram_w );

VIDEO_UPDATE( suprridr );
