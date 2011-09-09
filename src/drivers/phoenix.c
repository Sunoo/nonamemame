/***************************************************************************

Phoenix hardware games

driver by Richard Davies

Note:
   pleiads is using another sound driver, sndhrdw\pleiads.c
 Andrew Scott (ascott@utkux.utcc.utk.edu)


To Do:


Survival:

- Protection.  There is a 14 pin part connected to the 8910 Port B D0 labeled DL57S22.
  There is a loop at $2002 that reads the player controls -- the game sits in this
  loop as long as Port B changes.  Also, Port B seems to invert the input bits, and
  the game checks for this at $2f32.  The game also uses the RIM instruction a lot,
  that's purpose is unclear, as the result doesn't seem to be used (even when it's
  stored, the result is never read again.)  I would think that this advances the
  protection chip somehow, but isn't RIM a read only operation?

- Check background visibile area.  When the background scrolls up, it
  currently shows below the top and bottom of the border of the play area.


Pleiads:

- Palette banking.  Controlled by 3 custom chips marked T-X, T-Y and T-Z.
  These chips are reponsible for the protection as well.

***************************************************************************/

#include "driver.h"



READ_HANDLER( phoenix_videoram_r );
WRITE_HANDLER( phoenix_videoram_w );
WRITE_HANDLER( phoenix_videoreg_w );
WRITE_HANDLER( pleiads_videoreg_w );
WRITE_HANDLER( phoenix_scroll_w );
READ_HANDLER( phoenix_input_port_0_r );
READ_HANDLER( pleiads_input_port_0_r );
READ_HANDLER( survival_input_port_0_r );
READ_HANDLER( survival_protection_r );
PALETTE_INIT( phoenix );
PALETTE_INIT( pleiads );
VIDEO_START( phoenix );
VIDEO_UPDATE( phoenix );

WRITE_HANDLER( phoenix_sound_control_a_w );
WRITE_HANDLER( phoenix_sound_control_b_w );
int phoenix_sh_start(const struct MachineSound *msound);
void phoenix_sh_stop(void);
void phoenix_sh_update(void);

WRITE_HANDLER( pleiads_sound_control_a_w );
WRITE_HANDLER( pleiads_sound_control_b_w );
int pleiads_sh_start(const struct MachineSound *msound);
void pleiads_sh_stop(void);
void pleiads_sh_update(void);


static MEMORY_READ_START( phoenix_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x7000, 0x73ff, phoenix_input_port_0_r }, /* IN0 or IN1 */
	{ 0x7800, 0x7bff, input_port_2_r }, 		/* DSW */
MEMORY_END

static MEMORY_READ_START( pleiads_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x7000, 0x73ff, pleiads_input_port_0_r }, /* IN0 or IN1 + protection */
	{ 0x7800, 0x7bff, input_port_2_r }, 		/* DSW */
MEMORY_END

static MEMORY_READ_START( survival_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x6900, 0x69ff, AY8910_read_port_0_r },
	{ 0x7000, 0x73ff, survival_input_port_0_r },/* IN0 or IN1 */
	{ 0x7800, 0x7bff, input_port_2_r },			/* DSW */
MEMORY_END


static MEMORY_WRITE_START( phoenix_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6000, 0x63ff, phoenix_sound_control_a_w },
	{ 0x6800, 0x6bff, phoenix_sound_control_b_w },
MEMORY_END

static MEMORY_WRITE_START( pleiads_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, pleiads_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6000, 0x63ff, pleiads_sound_control_a_w },
	{ 0x6800, 0x6bff, pleiads_sound_control_b_w },
MEMORY_END

static MEMORY_WRITE_START( survival_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6800, 0x68ff, AY8910_control_port_0_w },
	{ 0x6900, 0x69ff, AY8910_write_port_0_w },
MEMORY_END



INPUT_PORTS_START( phoenix )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenixa )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	/* Coinage is backwards from phoenix (Amstar) */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenixt )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenix3 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( condor )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x70, 0x30, "Fuel Consumption" )
	PORT_DIPSETTING(	0x00, "Slowest" )
	PORT_DIPSETTING(	0x10, "Slower" )
	PORT_DIPSETTING(	0x20, "Slow" )
	PORT_DIPSETTING(	0x30, "Bit Slow" )
	PORT_DIPSETTING(	0x40, "Bit Fast" )
	PORT_DIPSETTING(	0x50, "Fast" )
	PORT_DIPSETTING(	0x60, "Faster" )
	PORT_DIPSETTING(	0x70, "Fastest" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( pleiads )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	   /* Protection. See 0x0552 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( pleiadce )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	   /* Protection. See 0x0552 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "7K 70K" )
	PORT_DIPSETTING(	0x04, "8K 80K" )
	PORT_DIPSETTING(	0x08, "9K 90K" )
  /*PORT_DIPSETTING(	0x0c, "INVALID" )   Sets bonus to A000 */
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( survival )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )

	PORT_START		/* IN1 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL )	/* comes from IN0 0-2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL  )

    PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "2" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x0c, "25000" )
	PORT_DIPSETTING(	0x08, "35000" )
	PORT_DIPSETTING(	0x04, "45000" )
	PORT_DIPSETTING(	0x00, "55000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x60, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 256*8*8, 0 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo phoenix_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 16 },
	{ REGION_GFX2, 0, &charlayout, 16*4, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo pleiads_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 32 },
	{ REGION_GFX2, 0, &charlayout, 32*4, 32 },
	{ -1 } /* end of array */
};


static struct TMS36XXinterface phoenix_tms36xx_interface =
{
	1,
	{ 50 }, 		/* mixing levels */
	{ MM6221AA },	/* TMS36xx subtype(s) */
	{ 372  },		/* base frequency */
	{ {0.50,0,0,1.05,0,0} }, /* decay times of voices */
    { 0.21 },       /* tune speed (time between beats) */
};

static struct CustomSound_interface phoenix_custom_interface =
{
	phoenix_sh_start,
	phoenix_sh_stop,
	phoenix_sh_update
};

static struct TMS36XXinterface pleiads_tms36xx_interface =
{
	1,
	{ 75		},	/* mixing levels */
	{ TMS3615	},	/* TMS36xx subtype(s) */
	{ 247		},	/* base frequencies (one octave below A) */
	/*
	 * Decay times of the voices; NOTE: it's unknown if
	 * the the TMS3615 mixes more than one voice internally.
	 * A wav taken from Pop Flamer sounds like there
	 * are at least no 'odd' harmonics (5 1/3' and 2 2/3')
     */
	{ {0.33,0.33,0,0.33,0,0.33} }
};

static struct CustomSound_interface pleiads_custom_interface =
{
	pleiads_sh_start,
	pleiads_sh_stop,
	pleiads_sh_update
};

static struct AY8910interface survival_ay8910_interface =
{
	1,	/* 1 chip */
	11000000/4,
	{ 50 },
	{ 0 },
	{ survival_protection_r },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( phoenix )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8085A, 11000000/4)	/* 2.75 MHz */
	MDRV_CPU_MEMORY(phoenix_readmem,phoenix_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* frames per second, vblank duration */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 26*8-1)
	MDRV_GFXDECODE(phoenix_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16*4+16*4)

	MDRV_PALETTE_INIT(phoenix)
	MDRV_VIDEO_START(phoenix)
	MDRV_VIDEO_UPDATE(phoenix)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("tms",  TMS36XX, phoenix_tms36xx_interface)
	MDRV_SOUND_ADD_TAG("cust", CUSTOM, phoenix_custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pleiads )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(phoenix)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(pleiads_readmem,pleiads_writemem)

	/* video hardware */
	MDRV_GFXDECODE(pleiads_gfxdecodeinfo)
	MDRV_COLORTABLE_LENGTH(32*4+32*4)

	MDRV_PALETTE_INIT(pleiads)

	/* sound hardware */
	MDRV_SOUND_REPLACE("tms",  TMS36XX, pleiads_tms36xx_interface)
	MDRV_SOUND_REPLACE("cust", CUSTOM, pleiads_custom_interface)
MACHINE_DRIVER_END


/* Same as Phoenix, but uses an AY8910 and an extra visible line (column) */

static MACHINE_DRIVER_START( survival )

	/* basic machine hardware */
	MDRV_CPU_ADD(8085A,11000000/4)	/* 2.75 MHz */
	MDRV_CPU_MEMORY(survival_readmem,survival_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 26*8-1)
	MDRV_GFXDECODE(phoenix_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16*4+16*4)

	MDRV_PALETTE_INIT(phoenix)
	MDRV_VIDEO_START(phoenix)
	MDRV_VIDEO_UPDATE(phoenix)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, survival_ay8910_interface)
MACHINE_DRIVER_END


/* Uses a Z80 */
static MACHINE_DRIVER_START( condor )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(phoenix)
	MDRV_CPU_REPLACE("main", Z80, 11000000/4)	/* 2.75 MHz??? */
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( phoenix )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45",         0x0000, 0x0800, CRC(9f68086b) SHA1(fc3cef299bf03bf0586c4047c6b96ca666846220) )
	ROM_LOAD( "ic46",         0x0800, 0x0800, CRC(273a4a82) SHA1(6f3019a074e73ff50ceb92f655fcf15659f34919) )
	ROM_LOAD( "ic47",         0x1000, 0x0800, CRC(3d4284b9) SHA1(6e69f8f0d537fe89140cd95d2398531d7e93d102) )
	ROM_LOAD( "ic48",         0x1800, 0x0800, CRC(cb5d9915) SHA1(49bcf55a5721cfcc02c3b811a4b601e35ea576db) )
	ROM_LOAD( "ic49",         0x2000, 0x0800, CRC(a105e4e7) SHA1(b35142a91b6b7fdf7535202671793393c9f4685f) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) SHA1(849bf8273317cc869bdd67e50c68399ee8ece81d) )
	ROM_LOAD( "ic52",         0x3800, 0x0800, CRC(aff8e9c5) SHA1(e4164f85ec12d4d9bcbffba27ab1f51b3599f6d0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, CRC(53413e8f) SHA1(d772358505b973b10da840d204afb210c0c746ec) )
	ROM_LOAD( "ic40",         0x0800, 0x0800, CRC(0be2ba91) SHA1(af9243ee23377b632b9b7d0b84d341d06bf22480) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( phoenixa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45.k1",      0x0000, 0x0800, CRC(c7a9b499) SHA1(cda61de47956b3603ff6e48556ce528b5f45deab) )
	ROM_LOAD( "ic46.k2",      0x0800, 0x0800, CRC(d0e6ae1b) SHA1(63c6df8365dcb8befa338e8479482e34a4259abf) )
	ROM_LOAD( "ic47.k3",      0x1000, 0x0800, CRC(64bf463a) SHA1(6cd876e80b85fbac6374ea1f26620c026ba9e99a) )
	ROM_LOAD( "ic48.k4",      0x1800, 0x0800, CRC(1b20fe62) SHA1(87d2da6b9bde9049825245ca4b994fc84543e8b9) )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, CRC(1a1ce0d0) SHA1(c2825eef5d461e16ca2172daff94b3751be2f4dc) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) SHA1(849bf8273317cc869bdd67e50c68399ee8ece81d) )
	ROM_LOAD( "ic52",         0x3800, 0x0800, CRC(aff8e9c5) SHA1(e4164f85ec12d4d9bcbffba27ab1f51b3599f6d0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "phoenixc.39",  0x0000, 0x0800, CRC(bb0525ed) SHA1(86db1c7584fb3846bfd47535e1585eeb7fbbb1fe) )
	ROM_LOAD( "phoenixc.40",  0x0800, 0x0800, CRC(4178aa4f) SHA1(5350f8f62cc7c223c38008bc83140b7a19147d81) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( phoenixt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, CRC(5b8c55a8) SHA1(839c1ca9766f730ec3accd48db70f6429a9c3362) )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, CRC(dbc942fa) SHA1(9fe224e6ced407289dfa571468259a021d942b7d) )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, CRC(cbbb8839) SHA1(b7f449374cac111081559e39646f973e7e99fd64) )
	ROM_LOAD( "phoenix.48",   0x1800, 0x0800, CRC(cb65eff8) SHA1(63e674d680972d3744d66b943e8546f3b77ee6d4) )
	ROM_LOAD( "phoenix.49",   0x2000, 0x0800, CRC(c8a5d6d6) SHA1(ef6ade323544e3edd4101609138ecf35e8cb9577) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) SHA1(849bf8273317cc869bdd67e50c68399ee8ece81d) )
	ROM_LOAD( "phoenix.52",   0x3800, 0x0800, CRC(b9915263) SHA1(f61396077b23364b5b26f62c6923394d23a37eb3) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, CRC(53413e8f) SHA1(d772358505b973b10da840d204afb210c0c746ec) )
	ROM_LOAD( "ic40",         0x0800, 0x0800, CRC(0be2ba91) SHA1(af9243ee23377b632b9b7d0b84d341d06bf22480) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( phoenix3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix3.45",  0x0000, 0x0800, CRC(a362cda0) SHA1(5ab38afaf92179c965533326574c773f6a63dbbb) )
	ROM_LOAD( "phoenix3.46",  0x0800, 0x0800, CRC(5748f486) SHA1(49e6fd836d26ec24105e95227b24cf668e8a470a) )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, CRC(cbbb8839) SHA1(b7f449374cac111081559e39646f973e7e99fd64) )
	ROM_LOAD( "phoenix3.48",  0x1800, 0x0800, CRC(b5d97a4d) SHA1(d5d92c5e34431b2ded47e58608c459cc8cdd7937) )
	ROM_LOAD( "ic49",         0x2000, 0x0800, CRC(a105e4e7) SHA1(b35142a91b6b7fdf7535202671793393c9f4685f) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) SHA1(849bf8273317cc869bdd67e50c68399ee8ece81d) )
	ROM_LOAD( "phoenix3.52",  0x3800, 0x0800, CRC(d2c5c984) SHA1(a9432f9aff8a2f5ca1d347443efc008a177d8ae0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, CRC(53413e8f) SHA1(d772358505b973b10da840d204afb210c0c746ec) )
	ROM_LOAD( "ic40",         0x0800, 0x0800, CRC(0be2ba91) SHA1(af9243ee23377b632b9b7d0b84d341d06bf22480) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( phoenixc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, CRC(5b8c55a8) SHA1(839c1ca9766f730ec3accd48db70f6429a9c3362) )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, CRC(dbc942fa) SHA1(9fe224e6ced407289dfa571468259a021d942b7d) )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, CRC(cbbb8839) SHA1(b7f449374cac111081559e39646f973e7e99fd64) )
	ROM_LOAD( "phoenixc.48",  0x1800, 0x0800, CRC(5ae0b215) SHA1(f6dd86806fb9c467aaa63edf0cb4dbed9645e7c0) )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, CRC(1a1ce0d0) SHA1(c2825eef5d461e16ca2172daff94b3751be2f4dc) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) SHA1(849bf8273317cc869bdd67e50c68399ee8ece81d) )
	ROM_LOAD( "phoenixc.52",  0x3800, 0x0800, CRC(8424d7c4) SHA1(1b5fa7d8be9e8750a4148dfefc17e96c86ed084d) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "phoenixc.39",  0x0000, 0x0800, CRC(bb0525ed) SHA1(86db1c7584fb3846bfd47535e1585eeb7fbbb1fe) )
	ROM_LOAD( "phoenixc.40",  0x0800, 0x0800, CRC(4178aa4f) SHA1(5350f8f62cc7c223c38008bc83140b7a19147d81) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( condor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cond01c.bin",  0x0000, 0x0800, CRC(c0f73929) SHA1(3cecf8341a5674165d2cae9b22ea5db26a9597de) )
	ROM_LOAD( "cond02c.bin",  0x0800, 0x0800, CRC(440d56e8) SHA1(b3147d5416cec8c00c7df40b878b826434121737) )
	ROM_LOAD( "cond03c.bin",  0x1000, 0x0800, CRC(750b059b) SHA1(6fbaa2ef4c7eef6f731a73b2d33a02fff21b318a) )
	ROM_LOAD( "cond04c.bin",  0x1800, 0x0800, CRC(ca55e1dd) SHA1(f3d8de56e54ec8651ab95af90ed122096c076c65) )
	ROM_LOAD( "cond05c.bin",  0x2000, 0x0800, CRC(1ff3a982) SHA1(66fb39e7abdf7a9c6e2eb01d41cfe9429781d6aa) )
	ROM_LOAD( "cond06c.bin",  0x2800, 0x0800, CRC(8c83bff7) SHA1(3dfb090d7e3a9ae8da882b06e166c48555eaf77c) )
	ROM_LOAD( "cond07c.bin",  0x3000, 0x0800, CRC(805ec2e8) SHA1(7e56fc9990eb99512078e2b1e2874fb33b0aa05c) )
	ROM_LOAD( "cond08c.bin",  0x3800, 0x0800, CRC(1edebb45) SHA1(2fdf061ee600e27a6ed512ea61a8d78307a7fb8a) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cond11c.bin",  0x0000, 0x0800, CRC(53c52eb0) SHA1(19624ca359996b77d3c65ef78a7af90eeb092377) )
	ROM_LOAD( "cond12c.bin",  0x0800, 0x0800, CRC(eba42f0f) SHA1(378282cb2c4e10c23179ae3c605ae7bf691150f6) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( falcon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "falcon.45",    0x0000, 0x0800, CRC(80382b6c) SHA1(47e24f04b5dd8aa8258ce324a0e4ef68a75dc168) )
	ROM_LOAD( "falcon.46",    0x0800, 0x0800, CRC(6a13193b) SHA1(760347695f1abc92cfe19ea7085e5aaf2dced383) )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, CRC(cbbb8839) SHA1(b7f449374cac111081559e39646f973e7e99fd64) )
	ROM_LOAD( "falcon.48",    0x1800, 0x0800, CRC(084e9766) SHA1(844b83041c3cf60c51a045029624296f205847ab) )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, CRC(1a1ce0d0) SHA1(c2825eef5d461e16ca2172daff94b3751be2f4dc) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) SHA1(0402e5241d99759d804291998efd43f37ce99917) )
	ROM_LOAD( "falcon.51",    0x3000, 0x0800, CRC(6e82e400) SHA1(22e97f74ca7010bba4263ea844cb7b2c6da09ab7) )
	ROM_LOAD( "ic52",         0x3800, 0x0800, CRC(aff8e9c5) SHA1(e4164f85ec12d4d9bcbffba27ab1f51b3599f6d0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, CRC(53413e8f) SHA1(d772358505b973b10da840d204afb210c0c746ec) )
	ROM_LOAD( "ic40",         0x0800, 0x0800, CRC(0be2ba91) SHA1(af9243ee23377b632b9b7d0b84d341d06bf22480) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( vautour )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "vautor01.1e",  0x0000, 0x0800, CRC(cd2807ee) SHA1(79b9769f212d25b9ccb5124e2aa632c964c14a0b) )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, CRC(dbc942fa) SHA1(9fe224e6ced407289dfa571468259a021d942b7d) )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, CRC(cbbb8839) SHA1(b7f449374cac111081559e39646f973e7e99fd64) )
	ROM_LOAD( "vautor04.1j",  0x1800, 0x0800, CRC(106262eb) SHA1(1e52ca66ea3542d86f2604f5aadc854ffe22fd89) )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, CRC(1a1ce0d0) SHA1(c2825eef5d461e16ca2172daff94b3751be2f4dc) )
	ROM_LOAD( "vautor06.1h",  0x2800, 0x0800, CRC(c90e3287) SHA1(696014738d3b87acb10175b021d9fd4885904a9f) )
	ROM_LOAD( "vautor07.1m",  0x3000, 0x0800, CRC(079ac364) SHA1(55b17c069b191cd1752e78068ef683b33c094e56) )
	ROM_LOAD( "vautor08.1n",  0x3800, 0x0800, CRC(1dbd937a) SHA1(efed9adad3d639c893b33071fd86c64800a7a32f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(3c7e623f) SHA1(e7ff5fc371664af44785c079e92eeb2d8530187b) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(59916d3b) SHA1(71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vautor12.2h",  0x0000, 0x0800, CRC(8eff75c9) SHA1(d38a0e0c02ba680984dd8748a3c45ac55f81f127) )
	ROM_LOAD( "vautor11.2j",  0x0800, 0x0800, CRC(369e7476) SHA1(599d2fc3b298060d746e95c20a089ad37f685d5b) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) SHA1(57411be4c1d89677f7919ae295446da90612c8a8) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) SHA1(e2184dd495ed579f10b6da0b78379e02d7a6229f) )  /* palette high bits */
ROM_END

ROM_START( pleiads )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic47.r1",      0x0000, 0x0800, CRC(960212c8) SHA1(52a3232e99920805ce9e195b8a6338ae7044dd18) )
	ROM_LOAD( "ic48.r2",      0x0800, 0x0800, CRC(b254217c) SHA1(312a33cca09d5d2d18992f28eb051230a90db6e3) )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, CRC(87e700bb) SHA1(0f352b5461da957c564920fd1da83bc81f41ffb9) ) /* IC 49 on real board */
	ROM_LOAD( "ic48.bin",     0x1800, 0x0800, CRC(2d5198d0) SHA1(6bfdc6c965199c5d4d687fe35dda057ec38cd8e0) ) /* IC 50 on real board */
	ROM_LOAD( "ic51.r5",      0x2000, 0x0800, CRC(49c629bc) SHA1(fd7937d0c114c8d9c1efaa9918ae3df2af41f032) )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, CRC(f1a8a00d) SHA1(5c183e3a73fa882ffec3cb9219fb5619e625591a) ) /* IC 52 on real board */
	ROM_LOAD( "ic53.r7",      0x3000, 0x0800, CRC(b5f07fbc) SHA1(2ae687c84732942e69ad4dfb7a4ac1b97b77487a) )
	ROM_LOAD( "ic52.bin",     0x3800, 0x0800, CRC(b1b5a8a6) SHA1(7e4ef298c8ddefc7dc0cbf94a9c9f36a4b807ba0) ) /* IC 54 on real board */

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23.bin",     0x0000, 0x0800, CRC(4e30f9e7) SHA1(da023a94725dc40107cd97e4decfd4dc0f9f00ee) ) /* IC 45 on real board */
	ROM_LOAD( "ic24.bin",     0x0800, 0x0800, CRC(5188fc29) SHA1(421dedc674c6dde7abf01412df035a8eb8e6db9b) ) /* IC 44 on real board */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, CRC(85866607) SHA1(cd240bd056f761b2f9e2142049434f02cae3e315) ) /* IC 27 on real board */
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, CRC(a841d511) SHA1(8349008ab1d8ef08775b54170c37deb1d391fffc) ) /* IC 26 on real board */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, CRC(7a1bcb1e) SHA1(bdfab316ea26e2063879e7aa78b6ae2b55eb95c8) )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, CRC(e38eeb83) SHA1(252880d80425b2e697146e76efdc6cb9f3ba0378) )   /* palette high bits */
ROM_END

ROM_START( pleiadbl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45.bin",     0x0000, 0x0800, CRC(93fc2958) SHA1(d8723c4f4376e035e655f69352c1765fdbf4a602) )
	ROM_LOAD( "ic46.bin",     0x0800, 0x0800, CRC(e2b5b8cd) SHA1(514ab2b24fc1d6d1fd64e74470b601ba9a11f36f) )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, CRC(87e700bb) SHA1(0f352b5461da957c564920fd1da83bc81f41ffb9) )
	ROM_LOAD( "ic48.bin",     0x1800, 0x0800, CRC(2d5198d0) SHA1(6bfdc6c965199c5d4d687fe35dda057ec38cd8e0) )
	ROM_LOAD( "ic49.bin",     0x2000, 0x0800, CRC(9dc73e63) SHA1(8a2de6666fecead7071285125b16641b50249adc) )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, CRC(f1a8a00d) SHA1(5c183e3a73fa882ffec3cb9219fb5619e625591a) )
	ROM_LOAD( "ic51.bin",     0x3000, 0x0800, CRC(6f56f317) SHA1(d7e6b0b1c58b741de3504640bcc23e86d1a134a0) )
	ROM_LOAD( "ic52.bin",     0x3800, 0x0800, CRC(b1b5a8a6) SHA1(7e4ef298c8ddefc7dc0cbf94a9c9f36a4b807ba0) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23.bin",     0x0000, 0x0800, CRC(4e30f9e7) SHA1(da023a94725dc40107cd97e4decfd4dc0f9f00ee) )
	ROM_LOAD( "ic24.bin",     0x0800, 0x0800, CRC(5188fc29) SHA1(421dedc674c6dde7abf01412df035a8eb8e6db9b) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, CRC(85866607) SHA1(cd240bd056f761b2f9e2142049434f02cae3e315) )
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, CRC(a841d511) SHA1(8349008ab1d8ef08775b54170c37deb1d391fffc) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, CRC(7a1bcb1e) SHA1(bdfab316ea26e2063879e7aa78b6ae2b55eb95c8) )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, CRC(e38eeb83) SHA1(252880d80425b2e697146e76efdc6cb9f3ba0378) )   /* palette high bits */
ROM_END

ROM_START( pleiadce )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pleiades.47",  0x0000, 0x0800, CRC(711e2ba0) SHA1(62d9108b9066d3e2b99c712daf2b9412704970cc) )
	ROM_LOAD( "pleiades.48",  0x0800, 0x0800, CRC(93a36943) SHA1(7cb4a9e8b60e28415df8401373ff4e595eaab7f5) )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, CRC(87e700bb) SHA1(0f352b5461da957c564920fd1da83bc81f41ffb9) )
	ROM_LOAD( "pleiades.50",  0x1800, 0x0800, CRC(5a9beba0) SHA1(e9cf03c88d8db2a7cf97877a103cfdd1fe3f459e) )
	ROM_LOAD( "pleiades.51",  0x2000, 0x0800, CRC(1d828719) SHA1(54857a3de9f4c9c5f18b0d46cf428b4171f839e9) )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, CRC(f1a8a00d) SHA1(5c183e3a73fa882ffec3cb9219fb5619e625591a) )
	ROM_LOAD( "pleiades.53",  0x3000, 0x0800, CRC(037b319c) SHA1(2ff7a7777a63326e2abca2d1881df33a8e3f8561) )
	ROM_LOAD( "pleiades.54",  0x3800, 0x0800, CRC(ca264c7c) SHA1(3a6adfaa935a1a11cb62e73b9f43b228b711c2da) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pleiades.45",  0x0000, 0x0800, CRC(8dbd3785) SHA1(700cb9eb8ea64be99d843910cebcd29d601ab2e9) )
	ROM_LOAD( "pleiades.44",  0x0800, 0x0800, CRC(0db3e436) SHA1(cd1825775b0a10df66d2ccc01cb4b6a9a3d2141a) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, CRC(85866607) SHA1(cd240bd056f761b2f9e2142049434f02cae3e315) )
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, CRC(a841d511) SHA1(8349008ab1d8ef08775b54170c37deb1d391fffc) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, CRC(7a1bcb1e) SHA1(bdfab316ea26e2063879e7aa78b6ae2b55eb95c8) )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, CRC(e38eeb83) SHA1(252880d80425b2e697146e76efdc6cb9f3ba0378) )   /* palette high bits */
ROM_END

ROM_START( survival )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g959-32a.u45", 0x0000, 0x0800, CRC(0bc53541) )
	ROM_LOAD( "g959-33a.u46", 0x0800, 0x0800, CRC(726e9428) )
	ROM_LOAD( "g959-34a.u47", 0x1000, 0x0800, CRC(78f166ff) )
	ROM_LOAD( "g959-35a.u48", 0x1800, 0x0800, CRC(59dbe099) )
	ROM_LOAD( "g959-36a.u49", 0x2000, 0x0800, CRC(bd5e586e) )
	ROM_LOAD( "g959-37a.u50", 0x2800, 0x0800, CRC(b2de1094) )
	ROM_LOAD( "g959-38a.u51", 0x3000, 0x0800, CRC(131c4440) )
	ROM_LOAD( "g959-39a.u52", 0x3800, 0x0800, CRC(213bc910) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g959-42.u23",  0x0000, 0x0800, CRC(3d1ce38d) )
	ROM_LOAD( "g959-43.u24",  0x0800, 0x0800, CRC(cd150da9) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g959-40.u39",  0x0000, 0x0800, CRC(41dee996) )
	ROM_LOAD( "g959-41.u40",  0x0800, 0x0800, CRC(a255d6dc) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "clr.u40",      0x0000, 0x0100, CRC(b3e20669) )   /* palette low bits */
	ROM_LOAD( "clr.u41",      0x0100, 0x0100, CRC(abddf69a) )   /* palette high bits */
ROM_END

ROM_START( phoenixr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45",         0x0000, 0x0800, CRC(5b8c55a8) )
	ROM_LOAD( "ic46",         0x0800, 0x0800, CRC(5748f486) )
	ROM_LOAD( "ic47",         0x1000, 0x0800, CRC(cbbb8839) )
	ROM_LOAD( "ic48",         0x1800, 0x0800, CRC(9253e642) )
	ROM_LOAD( "ic49",         0x2000, 0x0800, CRC(a105e4e7) )
	ROM_LOAD( "ic50",         0x2800, 0x0800, CRC(ac5e9ec1) )
	ROM_LOAD( "ic51",         0x3000, 0x0800, CRC(2eab35b4) )
	ROM_LOAD( "ic52",         0x3800, 0x0800, CRC(d2c5c984) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, CRC(02bc87ea) )
	ROM_LOAD( "ic24",         0x0800, 0x0800, CRC(675388ee) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, CRC(8b1653ba) )
	ROM_LOAD( "ic40",         0x0800, 0x0800, CRC(3b811dfb) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, CRC(79350b25) )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, CRC(e176b768) )  /* palette high bits */
ROM_END


static DRIVER_INIT( survival )
{
	unsigned char *rom = memory_region(REGION_CPU1);

	rom[0x0157] = 0x21;	/* ROM check */
	rom[0x02e8] = 0x21; /* crash due to protection, it still locks up somewhere else */
}



GAME ( 1980, phoenix,  0,       phoenix,  phoenix,  0,        ROT90, "Amstar", "Phoenix (Amstar)" )
GAME ( 1980, phoenixa, phoenix, phoenix,  phoenixa, 0,        ROT90, "Amstar (Centuri license)", "Phoenix (Centuri)" )
GAME ( 1980, phoenixt, phoenix, phoenix,  phoenixt, 0,        ROT90, "Taito", "Phoenix (Taito)" )
GAME ( 1980, phoenix3, phoenix, phoenix,  phoenix3, 0,        ROT90, "bootleg", "Phoenix (T.P.N.)" )
GAME ( 1981, phoenixc, phoenix, phoenix,  phoenixt, 0,        ROT90, "bootleg?", "Phoenix (IRECSA, G.G.I Corp)" )
GAME ( 1981, condor,   phoenix, condor,   condor,   0,        ROT90, "Sidam", "Condor" )
// the following 2 were common bootlegs in england & france respectively
GAME ( 1980, falcon,   phoenix, phoenix,  phoenixt, 0,        ROT90, "bootleg", "Falcon" )
GAME ( 1980, vautour,  phoenix, phoenix,  phoenixt, 0,        ROT90, "bootleg", "Vautour (Jeutel France)" )
GAMEX( 1981, pleiads,  0,       pleiads,  pleiads,  0,        ROT90, "Tehkan", "Pleiads (Tehkan)", GAME_IMPERFECT_COLORS )
GAMEX( 1981, pleiadbl, pleiads, pleiads,  pleiads,  0,        ROT90, "bootleg", "Pleiads (bootleg)", GAME_IMPERFECT_COLORS )
GAMEX( 1981, pleiadce, pleiads, pleiads,  pleiadce, 0,        ROT90, "Tehkan (Centuri license)", "Pleiads (Centuri)", GAME_IMPERFECT_COLORS )
GAMEX( 1982, survival, 0,       survival, survival, survival, ROT90, "Rock-ola", "Survival", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 2000, phoenixr, phoenix, phoenix, phoenix, 0, ROT90, "CYBERYOGI =CO= Windler hack", "PhoenixR (German Democratic Republic version)", GAME_NO_COCKTAIL ) 
