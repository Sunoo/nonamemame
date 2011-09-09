/*************************************************************************

	Sega Pengo

**************************************************************************/

/*----------- defined in vidhrdw/pengo.c -----------*/

PALETTE_INIT( pacman );
PALETTE_INIT( pengo );
//PacMAME
PALETTE_INIT( multipac );

VIDEO_START( pacman );
VIDEO_START( pengo );
//PacMAME
VIDEO_START( pacmanx );

WRITE_HANDLER( pengo_gfxbank_w );
WRITE_HANDLER( pengo_flipscreen_w );
//PacMAME
WRITE_HANDLER( pacmanx_flipscreen_w );

VIDEO_UPDATE( pengo );
//PacMAME
VIDEO_UPDATE( pacmanx );

WRITE_HANDLER( vanvan_bgcolor_w );
VIDEO_UPDATE( vanvan );
