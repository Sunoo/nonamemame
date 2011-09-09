/*
 *	Beatmania DJ Main Board (GX753)
 *
 *	Product numbers:
 *	GQ753 beatmania (first release in 1997)
 *	Gx853 beatmania 2nd MIX (1998)
 *	Gx825 beatmania 3rd MIX
 *	Gx858 beatmania complete MIX (1999)
 *	Gx847 beatmania 4th MIX
 *	Gx981 beatmania 5th MIX
 *	Gx993 beatmania Club MIX (2000)
 *	Gx988 beatmania complete MIX 2
 *	GxA05 beatmania CORE REMIX
 *
 *	Gx803 Pop'n Music 1 (1998)
 *	Gx831 Pop'n Music 2
 *	Gx980 Pop'n Music 3 (1999)
 *
 *	Gx970 Pop'n Stage EX (1999)
 *
 *	Chips:
 *	15a:	MC68EC020FG25
 *	25b:	001642
 *	18d:	055555 (priority encoder)
 *	 5f:	056766 (splites?)
 *	18f:	056832 (tiles)
 *	22f:	058143 = 054156 (tiles)
 *	12j:	058141 = 054539 (x2) (2 sound chips in one)
 *
 *	TODO:
 *	- keep sync between objects and background sound.
 *	- correct background color
 *	- correct FPS
 *	- volum control
 *
 */

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "machine/idectrl.h"
#include "sound/k054539.h"
#include "vidhrdw/konamiic.h"

// debug purpose only
// to check synchronous between objects and background sound
//#define AUTOPLAY_CHEAT


extern data32_t *djmain_obj_ram;

#ifdef AUTOPLAY_CHEAT
data16_t autokey;
int autoscratch[2];
#endif /* AUTOPLAY_CHEAT */

VIDEO_UPDATE( djmain );
VIDEO_START( djmain );


static int sndram_bank;
static data8_t *sndram;

static int scratch_select;
static data8_t scratch_data[2];

static int pending_vb_int;
static data16_t v_ctrl;
static data32_t obj_regs[0xa0/4];

#define DISABLE_VB_INT	(!(v_ctrl & 0x8000))


/*************************************
 *
 *	68k CPU memory handlers
 *
 *************************************/

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

 	r = (data >>  0) & 0xff;
	g = (data >>  8) & 0xff;
	b = (data >> 16) & 0xff;

	// *KLUDGE* fix background color of tile
	if ((offset & 15) == 0)
	{
		if (r != g || g != b)
			r = g = b = 0;
	}

	palette_set_color(offset, r, g, b);
}


//---------

static void sndram_set_bank(void)
{
	sndram = memory_region(REGION_SOUND1) + 0x80000 * sndram_bank;
}

static WRITE32_HANDLER( sndram_bank_w )
{
	if (ACCESSING_MSW32)
	{
		sndram_bank = (data >> 16) & 0x1f;
		sndram_set_bank();
	}
}

static READ32_HANDLER( sndram_r )
{
	data32_t data = 0;

	if ((mem_mask & 0xff000000) == 0)
		data |= sndram[offset * 4] << 24;

	if ((mem_mask & 0x00ff0000) == 0)
		data |= sndram[offset * 4 + 1] << 16;

	if ((mem_mask & 0x0000ff00) == 0)
		data |= sndram[offset * 4 + 2] << 8;

	if ((mem_mask & 0x000000ff) == 0)
		data |= sndram[offset * 4 + 3];

	return data;
}

static WRITE32_HANDLER( sndram_w )
{
	if ((mem_mask & 0xff000000) == 0)
		sndram[offset * 4] = data >> 24;

	if ((mem_mask & 0x00ff0000) == 0)
		sndram[offset * 4 + 1] = data >> 16;

	if ((mem_mask & 0x0000ff00) == 0)
		sndram[offset * 4 + 2] = data >> 8;

	if ((mem_mask & 0x000000ff) == 0)
		sndram[offset * 4 + 3] = data;
}


//---------

static READ16_HANDLER( dual539_16_r )
{
	data16_t ret = 0;

	if (ACCESSING_LSB16)
		ret |= K054539_1_r(offset);
	if (ACCESSING_MSB16)
		ret |= K054539_0_r(offset)<<8;

	return ret;
}

static WRITE16_HANDLER( dual539_16_w )
{
	if (ACCESSING_LSB16)
		K054539_1_w(offset, data);
	if (ACCESSING_MSB16)
		K054539_0_w(offset, data>>8);
}

static READ32_HANDLER( dual539_r )
{
	data32_t data = 0;

	if (~mem_mask & 0xffff0000)
		data |= dual539_16_r(offset * 2, mem_mask >> 16) << 16;
	if (~mem_mask & 0x0000ffff)
		data |= dual539_16_r(offset * 2 + 1, mem_mask);

	return data;
}

static WRITE32_HANDLER( dual539_w )
{
	if (~mem_mask & 0xffff0000)
		dual539_16_w(offset * 2, data >> 16, mem_mask >> 16);
	if (~mem_mask & 0x0000ffff)
		dual539_16_w(offset * 2 + 1, data, mem_mask);
}


//---------

static READ32_HANDLER( obj_ctrl_r )
{
	// read obj_regs[0x0c/4]: unknown
	// read obj_regs[0x24/4]: unknown

	return obj_regs[offset];
}

static WRITE32_HANDLER( obj_ctrl_w )
{
	// write obj_regs[0x28/4]: bank for rom readthrough

	COMBINE_DATA(&obj_regs[offset]);
}

static READ32_HANDLER( obj_rom_r )
{
	data8_t *mem8 = memory_region(REGION_GFX1);
	int bank = obj_regs[0x28/4] >> 16;

	offset += bank * 0x200;
	offset *= 4;

	if (~mem_mask & 0x0000ffff)
		offset += 2;

	if (~mem_mask & 0xff00ff00)
		offset++;

	return mem8[offset] * 0x01010101;
}


//---------

static WRITE32_HANDLER( v_ctrl_w )
{
	if (ACCESSING_MSW32)
	{
		data >>= 16;
		mem_mask >>= 16;
		COMBINE_DATA(&v_ctrl);

		if (pending_vb_int && !DISABLE_VB_INT)
		{
			pending_vb_int = 0;
			cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
		}
	}
}

static READ32_HANDLER( v_rom_r )
{
	data8_t *mem8 = memory_region(REGION_GFX2);
	int bank = K056832_word_r(0x34/2, 0xffff);

	offset *= 2;

	if (!ACCESSING_MSB32)
		offset += 1;

	offset += bank * 0x800 * 4;

	if (v_ctrl & 0x020)
		offset += 0x800 * 2;

	return mem8[offset] * 0x01010000;
}


//---------

static READ32_HANDLER( inp1_r )
{
	data32_t result = (input_port_5_r(0)<<24) | (input_port_2_r(0)<<16) | (input_port_1_r(0)<<8) | input_port_0_r(0);

#ifdef AUTOPLAY_CHEAT
       result &= ~autokey;
#endif /* AUTOPLAY_CHEAT */

	return result;
}

static READ32_HANDLER( inp2_r )
{
	return (input_port_3_r(0)<<24) | (input_port_4_r(0)<<16) | 0xffff;
}

static READ32_HANDLER( scratch_r )
{
	data32_t result = 0;

	if (!(mem_mask & 0x0000ff00))
	{
#ifdef AUTOPLAY_CHEAT
		if (autokey & (scratch_select == 0 ? 0x0020 : 0x0800))
			scratch_data[scratch_select] += autoscratch[scratch_select];
#endif /* AUTOPLAY_CHEAT */

		if (input_port_6_r(0) & (1 << scratch_select))
			scratch_data[scratch_select]++;
		if (input_port_6_r(0) & (4 << scratch_select))
			scratch_data[scratch_select]--;

		result |= scratch_data[scratch_select] << 8;
	}

	return result;
}

static WRITE32_HANDLER( scratch_w )
{
	if (!(mem_mask & 0x00ff0000))
		scratch_select = (data >> 19) & 1;
}

//---------

#define IDE_STD_OFFSET	(0x1f0/2)
#define IDE_ALT_OFFSET	(0x3f6/2)

static READ32_HANDLER( ide_std_r )
{
	if (ACCESSING_LSB32)
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x00ff) >> 8;
	else
		return ide_controller16_0_r(IDE_STD_OFFSET + offset, 0x0000) << 16;
}

static WRITE32_HANDLER( ide_std_w )
{
	if (ACCESSING_LSB32)
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data << 8, 0x00ff);
	else
		ide_controller16_0_w(IDE_STD_OFFSET + offset, data >> 16, 0x0000);
}


static READ32_HANDLER( ide_alt_r )
{
	if (offset == 0)
		return ide_controller16_0_r(IDE_ALT_OFFSET, 0xff00) << 24;

	return 0;
}

static WRITE32_HANDLER( ide_alt_w )
{
	if (offset == 0 && !(mem_mask & 0x00ff0000))
		ide_controller16_0_w(IDE_ALT_OFFSET, data >> 24, 0xff00);
}



//---------

static WRITE32_HANDLER( unknown590000_w )
{
	//logerror("%08X: unknown590000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknown5d0000_w )
{
	//logerror("%08X: unknown5d0000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknown5d2000_w )
{
	// neon light??
	//logerror("%08X: unknown5d2000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknown802000_w )
{
	//logerror("%08X: unknown802000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}

static WRITE32_HANDLER( unknownc02000_w )
{
	//logerror("%08X: unknownc02000 write %08X: %08X & %08X\n", activecpu_get_previouspc(), offset, data, ~mem_mask);
}



/*************************************
 *
 *	Interrupt handlers
 *
 *************************************/

static INTERRUPT_GEN( vb_interrupt )
{
#ifdef AUTOPLAY_CHEAT
#define KEY_PRESS_TIME	64

	extern const struct GameDriver driver_bmcompmx;

	static int counter[12];
	static int target[12];

	int scratch_x1 = 96;
	int scratch_x2 = 464;
	int scratch_y = 302;
	int i;

	if (Machine->gamedrv == &driver_bmcompmx)
	{
		scratch_x1 = 88;
		scratch_x2 = 456;
		scratch_y = 312;
	}

	autokey = 0;

	for (i = 0; i < 12; i++)
	{
		if (counter[i] > 1)
			counter[i]--;
		else
			counter[i] = 0;
	}

	if (input_port_7_r(0) & 0x01)
	{
		for (i = 0; i < 0x800; i += 4)
		{
			int key = -1;
			int x, y;

			if (!(djmain_obj_ram[i] & 0x00008000))
				continue;
			if ((djmain_obj_ram[i + 2] & 0x000f0000) == 0x000d0000)
				continue;

			x = djmain_obj_ram[i + 1] & 0xffff;
			y = djmain_obj_ram[i + 1] >> 16;

			// find target sprite (scratch)
			if (y == scratch_y - 1 || y == scratch_y - 2)
			{
				if (x == scratch_x1)
					target[5] = i;
				else if (x == scratch_x2)
					target[11] = i;
			}

			// turn table?
			if (y >= scratch_y)
			{
				if (x == scratch_x1)
					key = 5;
				else if (x == scratch_x2)
					key = 11;

				if (target[key])
				{
					y = djmain_obj_ram[target[key] + 1] >> 16;
					if (y == scratch_y - 1 || y == scratch_y - 2)
						continue;

					counter[key] = KEY_PRESS_TIME;
					target[key] = 0;

					if (autoscratch[(key > 5)] > 0)
						autoscratch[(key > 5)] = -8;
					else
						autoscratch[(key > 5)] = 8;
				}
			}

			// find target sprite (key)
			if (y == 309 || y == 310 || y == 311)
			{
				if (x >= 40 && x <= 72 && (x & 7) == 0)
					target[4 - (x - 40) / 8] = i;
				else if (x >= 408 && x <= 440 && (x & 7) == 0)
					target[6 + (x - 408) / 8] = i;

			}

			// press key?
			if (y == 312)
			{
				if (x >= 40 && x <= 72 && (x & 7) == 0)
					key = 4 - (x - 40) / 8;
				else if (x >= 408 && x <= 440 && (x & 7) == 0)
					key = 6 + (x - 408) / 8;

				if (target[key])
				{
					y = djmain_obj_ram[target[key] + 1] >> 16;
					if (y == 310 || y == 311)
						continue;

					counter[key] = KEY_PRESS_TIME;
					target[key] = 0;
				}
			}
		}

		for (i = 0; i < 12; i++)
			if (counter[i] > KEY_PRESS_TIME - 2)
				autokey |= 1 << i;
	}
#endif /* AUTOPLAY_CHEAT */

	pending_vb_int = 0;

	if (DISABLE_VB_INT)
	{
		pending_vb_int = 1;
		return;
	}

	//logerror("V-Blank interrupt\n");
	cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
}


static void ide_interrupt(int state)
{
	if (state != CLEAR_LINE)
	{
		//logerror("IDE interrupt asserted\n");
		cpu_set_irq_line(0, MC68000_IRQ_1, HOLD_LINE);
	}
	else
	{
		//logerror("IDE interrupt cleared\n");
		cpu_set_irq_line(0, MC68000_IRQ_1, CLEAR_LINE);
	}
}




/*************************************
 *
 *	Memory definitions
 *
 *************************************/

static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x0fffff, MRA32_ROM },		// PRG ROM
	{ 0x400000, 0x40ffff, MRA32_RAM },		// WORK RAM
	{ 0x480000, 0x48443f, paletteram32_r },		// COLOR RAM (tilemap)
	{ 0x500000, 0x57ffff, sndram_r },		// SOUND RAM
	{ 0x580000, 0x58003f, K056832_long_r },		// VIDEO REG (tilemap)
	{ 0x5b0000, 0x5b04ff, dual539_r },		// SOUND regs
	{ 0x5c0000, 0x5c0003, inp1_r },			// input port
	{ 0x5c8000, 0x5c8003, inp2_r },			// input port
	{ 0x5e0000, 0x5e0003, scratch_r },		// scratch input port
	{ 0x600000, 0x601fff, v_rom_r },		// VIDEO ROM readthrough (for POST)
	{ 0x801000, 0x8017ff, MRA32_RAM },		// OBJECT RAM
	{ 0x803000, 0x80309f, obj_ctrl_r },		// OBJECT REGS
	{ 0x803800, 0x803fff, obj_rom_r },		// OBJECT ROM readthrough (for POST)
	{ 0xc00000, 0xc01fff, K056832_ram_long_r },	// VIDEO RAM (tilemap) (beatmania)
	{ 0xd00000, 0xd0000f, ide_std_r },		// IDE control regs (hiphopmania)
	{ 0xd4000c, 0xd4000f, ide_alt_r },		// IDE status control reg (hiphopmania)
	{ 0xe00000, 0xe01fff, K056832_ram_long_r },	// VIDEO RAM (tilemap) (hiphopmania)
	{ 0xf00000, 0xf0000f, ide_std_r },		// IDE control regs (beatmania)
	{ 0xf4000c, 0xf4000f, ide_alt_r },		// IDE status control reg (beatmania)
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0x000000, 0x0fffff, MWA32_ROM },		// PRG ROM
	{ 0x400000, 0x40ffff, MWA32_RAM },		// WORK RAM
	{ 0x480000, 0x48443f, paletteram32_w, &paletteram32 },	// COLOR RAM
	{ 0x500000, 0x57ffff, sndram_w },		// SOUND RAM
	{ 0x580000, 0x58003f, K056832_long_w },		// VIDEO REG (tilemap)
	{ 0x590000, 0x590007, unknown590000_w },	// ??
	{ 0x5a0000, 0x5a005f, K055555_long_w },		// 055555: priority encoder
	{ 0x5b0000, 0x5b04ff, dual539_w },		// SOUND regs
	{ 0x5d0000, 0x5d0003, unknown5d0000_w },	// ??
	{ 0x5d2000, 0x5d2003, unknown5d2000_w },	// ??
	{ 0x5d4000, 0x5d4003, v_ctrl_w },		// VIDEO control
	{ 0x5d6000, 0x5d6003, sndram_bank_w },		// SOUND RAM bank
	{ 0x5e0000, 0x5e0003, scratch_w },		// scratch input port
	{ 0x801000, 0x8017ff, MWA32_RAM, &djmain_obj_ram },	// OBJECT RAM
	{ 0x802000, 0x802fff, unknown802000_w },	// ??
	{ 0x803000, 0x80309f, obj_ctrl_w },		// OBJECT REGS
	{ 0xc00000, 0xc01fff, K056832_ram_long_w },	// VIDEO RAM (tilemap) (beatmania)
	{ 0xc02000, 0xc02047, unknownc02000_w },	// ??
	{ 0xd00000, 0xd0000f, ide_std_w },		// IDE control regs (hiphopmania)
	{ 0xd4000c, 0xd4000f, ide_alt_w },		// IDE status control reg (hiphopmania)
	{ 0xe00000, 0xe01fff, K056832_ram_long_w },	// VIDEO RAM (tilemap) (hiphopmania)
	{ 0xf00000, 0xf0000f, ide_std_w },		// IDE control regs (beatmania)
	{ 0xf4000c, 0xf4000f, ide_alt_w },		// IDE status control reg (beatmania)
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( djmain )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )	// EFFECT
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	//PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )		// TEST SW
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST SW
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )	// SERVICE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )	// RESET SW
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3: DSW 1 */
	PORT_DIPNAME( 0x80, 0x80, "DSW 1 - 1" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW 1 - 2" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW 1 - 3" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 1 - 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 1 - 5" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 1 - 6" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 1 - 7" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 1 - 8" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN4: DSW 2 */
	PORT_DIPNAME( 0x80, 0x80, "DSW 2 - 1" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW 2 - 2" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW 2 - 3" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 2 - 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 2 - 5" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 2 - 6" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 2 - 7" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 2 - 8" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN5: DSW 3 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "DSW 3 - 1" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 3 - 2" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 3 - 3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 3 - 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 3 - 5" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 3 - 6" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN6: fake port for scratch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1 )	// +R
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2 )	// +R
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER1 )	// -L
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER2 )	// -L

#ifdef AUTOPLAY_CHEAT
	PORT_START      /* IN7: autoplay cheat */
	PORT_BITX(0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Auto Play", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(0x01, DEF_STR( On ) )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )
#endif /* AUTOPLAY_CHEAT */
INPUT_PORTS_END

INPUT_PORTS_START( popn )
	PORT_START      /* IN0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 | IPF_PLAYER1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON9 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 */
	//PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )		// TEST SW
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// TEST SW
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )	// SERVICE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )	// RESET SW
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3: DSW 1 */
	PORT_DIPNAME( 0x80, 0x80, "DSW 1 - 1" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW 1 - 2" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW 1 - 3" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 1 - 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 1 - 5" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 1 - 6" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 1 - 7" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 1 - 8" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN4: DSW 2 */
	PORT_DIPNAME( 0x80, 0x80, "DSW 2 - 1" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW 2 - 2" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW 2 - 3" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 2 - 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 2 - 5" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 2 - 6" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 2 - 7" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 2 - 8" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN5: DSW 3 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "DSW 3 - 1" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW 3 - 2" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW 3 - 3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW 3 - 4" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW 3 - 5" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DSW 3 - 6" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout spritelayout =
{
	16, 16,	/* 16x16 characters */
	0x200000 / 128,	/* 16384 characters */
	4,	/* bit planes */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24,
	  4+256, 0+256, 12+256, 8+256, 20+256, 16+256, 28+256, 24+256 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  0*32+512, 1*32+512, 2*32+512, 3*32+512, 4*32+512, 5*32+512, 6*32+512, 7*32+512 },
	16*16*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,  (0x4440/4)/16 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	IDE interfaces
 *
 *************************************/

static struct ide_interface ide_intf =
{
	ide_interrupt,
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};



/*************************************
 *
 *	Machine-specific init
 *
 *************************************/

static MACHINE_INIT( djmain )
{
	/* reset sound ram bank */
	sndram_bank = 0;
	sndram_set_bank();

	/* reset the IDE controller */
	ide_controller_reset(0);
}



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( djmain )

	/* basic machine hardware */
	//MDRV_CPU_ADD(M68EC020, 32000000*3/4)	/* 24.000 MHz ? */
	//MDRV_CPU_ADD(M68EC020, 18432000)	/* 18.432 MHz ? */
	MDRV_CPU_ADD(M68EC020, 32000000/2)	/* 16.000 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vb_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(djmain)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(12, 512-12-1, 0, 384-1)
	MDRV_PALETTE_LENGTH(0x4440/4)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_START(djmain)
	MDRV_VIDEO_UPDATE(djmain)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( bmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858jab01.6a", 0x000000, 0x80000, CRC(92841EB5) SHA1(3a9d90a9c4b16cb7118aed2cadd3ab32919efa96) )
	ROM_LOAD16_BYTE( "858jab02.8a", 0x000001, 0x80000, CRC(7B19969C) SHA1(3545acabbf53bacc5afa72a3c5af3cd648bc2ae1) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPLITE */
	ROM_LOAD16_BYTE( "858jaa03.19a", 0x000000, 0x80000, CRC(8559F457) SHA1(133092994087864a6c29e9d51dcdbef2e2c2a123) )
	ROM_LOAD16_BYTE( "858jaa04.20a", 0x000001, 0x80000, CRC(770824D3) SHA1(5c21bc39f8128957d76be85bc178c96976987f5f) )
	ROM_LOAD16_BYTE( "858jaa05.22a", 0x100000, 0x80000, CRC(9CE769DA) SHA1(1fe2999f786effdd5e3e74475e8431393eb9403d) )
	ROM_LOAD16_BYTE( "858jaa06.24a", 0x100001, 0x80000, CRC(0CDE6584) SHA1(fb58d2b4f58144b71703431740c0381bb583f581) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858jaa07.22d", 0x000000, 0x80000, CRC(7D183F46) SHA1(7a1b0ccb0407b787af709bdf038d886727199e4e) )
	ROM_LOAD16_BYTE( "858jaa08.23d", 0x000001, 0x80000, CRC(C731DC8F) SHA1(1a937d76c02711b7f73743c9999456d4408ad284) )
	ROM_LOAD16_BYTE( "858jaa09.25d", 0x100000, 0x80000, CRC(0B4AD843) SHA1(c01e15053dd1975dc68db9f4e6da47062d8f9b54) )
	ROM_LOAD16_BYTE( "858jaa10.27d", 0x100001, 0x80000, CRC(00B124EE) SHA1(435d28a327c2707833a8ddfe841104df65ffa3f8) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11.chd", 0, MD5(e7b26f6f03f807a32b2e5e291324d582) )
ROM_END

ROM_START( hmcompmx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "858uab01.6a", 0x000000, 0x80000, CRC(F9C16675) SHA1(f2b50a3544f43af6fd987256a8bd4125b95749ef) )
	ROM_LOAD16_BYTE( "858uab02.8a", 0x000001, 0x80000, CRC(4E8F1E78) SHA1(88d654de4377b584ff8a5e1f8bc81ffb293ec8a5) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPLITE */
	ROM_LOAD16_BYTE( "858uaa03.19a", 0x000000, 0x80000, CRC(52B51A5E) SHA1(9f01e2fcbe5a9d7f80b377c5e10f18da2c9dcc8e) )
	ROM_LOAD16_BYTE( "858uaa04.20a", 0x000001, 0x80000, CRC(A336CEE9) SHA1(0e62c0c38d86868c909b4c1790fbb7ecb2de137d) )
	ROM_LOAD16_BYTE( "858uaa05.22a", 0x100000, 0x80000, CRC(2E14CF83) SHA1(799b2162f7b11678d1d260f7e1eb841abda55a60) )
	ROM_LOAD16_BYTE( "858uaa06.24a", 0x100001, 0x80000, CRC(2BE07788) SHA1(5cc2408f907ca6156efdcbb2c10a30e9b81797f8) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "858uaa07.22d", 0x000000, 0x80000, CRC(9D7C8EA0) SHA1(5ef773ade7ab12a5dc10484e8b7711c9d76fe2a1) )
	ROM_LOAD16_BYTE( "858uaa08.23d", 0x000001, 0x80000, CRC(F21C3F45) SHA1(1d7ff2c4161605b382d07900142093192aa93a48) )
	ROM_LOAD16_BYTE( "858uaa09.25d", 0x100000, 0x80000, CRC(99519886) SHA1(664f6bd953201a6e2fc123cb8b3facf72766107d) )
	ROM_LOAD16_BYTE( "858uaa10.27d", 0x100001, 0x80000, CRC(20AA7145) SHA1(eeff87eb9a9864985d751f45e843ee6e73db8cfd) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "858jaa11.chd", 0, MD5(e7b26f6f03f807a32b2e5e291324d582) )
ROM_END

ROM_START( bmcompm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "988jaa01.6a", 0x000000, 0x80000, CRC(31BE1D4C) SHA1(ab8c2b4a2b48e3b2b549022f65afb206ab125680) )
	ROM_LOAD16_BYTE( "988jaa02.8a", 0x000001, 0x80000, CRC(0413DE32) SHA1(f819e8756e2000de5df61ad42ac01de14b7330f9) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPLITE */
	ROM_LOAD16_BYTE( "988jaa03.19a", 0x000000, 0x80000, CRC(C0AD86D4) SHA1(6aca5bf3fbc0bd69116e442053840660eeff0239) )
	ROM_LOAD16_BYTE( "988jaa04.20a", 0x000001, 0x80000, CRC(84801A50) SHA1(8700e4fb56941b87f8333e72e2a1c7ac9e322312) )
	ROM_LOAD16_BYTE( "988jaa05.22a", 0x100000, 0x80000, CRC(0DDF7D6D) SHA1(aa110ab64c2fbf427796dff3a817b57cf6a9440d) )
	ROM_LOAD16_BYTE( "988jaa06.24a", 0x100001, 0x80000, CRC(2A87F69E) SHA1(fe84bb50864467a83d06d34a18123ab11fb55781) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "988jaa07.22d", 0x000000, 0x80000, CRC(9E57FE24) SHA1(40bd0428227e46ebe365f2f6821b08182a0ce698) )
	ROM_LOAD16_BYTE( "988jaa08.23d", 0x000001, 0x80000, CRC(BF604CA4) SHA1(6abc81d5d9084fcf59f70a6bd57e1b36041a1072) )
	ROM_LOAD16_BYTE( "988jaa09.25d", 0x100000, 0x80000, CRC(8F3BAE7F) SHA1(c4dac14f6c7f75a2b19153e05bfe969e9eb4aca0) )
	ROM_LOAD16_BYTE( "988jaa10.27d", 0x100001, 0x80000, CRC(248BF0EE) SHA1(d89205ed57e771401bfc2c24043d200ecbd0b7fc) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "988jaa11.chd", 0, MD5(cc21d58d6bee58f1c4baf08f345fe2c5) )
ROM_END

ROM_START( hmcompm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "988uaa01.6a", 0x000000, 0x80000, CRC(5E5CC6C0) SHA1(0e7cd601d4543715cbc9f65e6fd48837179c962a) )
	ROM_LOAD16_BYTE( "988uaa02.8a", 0x000001, 0x80000, CRC(E262984A) SHA1(f47662e40f91f2addb1a4b649923c1d0ee017341) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPLITE */
	ROM_LOAD16_BYTE( "988uaa03.19a", 0x000000, 0x80000, CRC(D0F204C8) SHA1(866baac5a6d301d5b9cf0c14e9937ee5f435db77) )
	ROM_LOAD16_BYTE( "988uaa04.20a", 0x000001, 0x80000, CRC(74C6B3ED) SHA1(7d9b064bab3f29fc6435f6430c71208abbf9d861) )
	ROM_LOAD16_BYTE( "988uaa05.22a", 0x100000, 0x80000, CRC(6B9321CB) SHA1(449e5f85288a8c6724658050fa9521c7454a1e46) )
	ROM_LOAD16_BYTE( "988uaa06.24a", 0x100001, 0x80000, CRC(DA6E0C1E) SHA1(4ef37db6c872bccff8c27fc53cccc0b269c7aee4) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "988uaa07.22d", 0x000000, 0x80000, CRC(9217870D) SHA1(d0536a8a929c41b49cdd053205165bfb8150e0c5) )
	ROM_LOAD16_BYTE( "988uaa08.23d", 0x000001, 0x80000, CRC(77777E59) SHA1(33b5508b961a04b82c9967a3326af6bbd838b85e) )
	ROM_LOAD16_BYTE( "988uaa09.25d", 0x100000, 0x80000, CRC(C2AD6810) SHA1(706388c5acf6718297fd90e10f8a673463a0893b) )
	ROM_LOAD16_BYTE( "988uaa10.27d", 0x100001, 0x80000, CRC(DAB0F3C9) SHA1(6fd899e753e32f60262c54ab8553c686c7ef28de) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "988jaa11.chd", 0, MD5(cc21d58d6bee58f1c4baf08f345fe2c5) )
ROM_END

ROM_START( bmcorerm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* MC68EC020FG25 MPU */
	ROM_LOAD16_BYTE( "a05jaa01.6a", 0x000000, 0x80000, CRC(CD6F1FC5) SHA1(237cbc17a693efb6bffffd6afb24f0944c29330c) )
	ROM_LOAD16_BYTE( "a05jaa02.8a", 0x000001, 0x80000, CRC(FE07785E) SHA1(14c652008cb509b5206fb515aad7dfe36a6fe6f4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0)		/* SPLITE */
	ROM_LOAD16_BYTE( "a05jaa03.19a", 0x000000, 0x80000, CRC(8B88932A) SHA1(df20f8323adb02d07b835da98f4a29b3142175c9) )
	ROM_LOAD16_BYTE( "a05jaa04.20a", 0x000001, 0x80000, CRC(CC72629F) SHA1(f95d06f409c7d6422d66a55c0452eb3feafc6ef0) )
	ROM_LOAD16_BYTE( "a05jaa05.22a", 0x100000, 0x80000, CRC(E241B22B) SHA1(941a76f6ac821e0984057ec7df7862b12fa657b8) )
	ROM_LOAD16_BYTE( "a05jaa06.24a", 0x100001, 0x80000, CRC(77EB08A3) SHA1(fd339aaec06916abfc928e850e33480707b5450d) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )		/* TILEMAP */
	ROM_LOAD16_BYTE( "a05jaa07.22d", 0x000000, 0x80000, CRC(4D79646D) SHA1(5f1237bbd3cb09b27babf1c5359ef6c0d80ae3a9) )
	ROM_LOAD16_BYTE( "a05jaa08.23d", 0x000001, 0x80000, CRC(F067494F) SHA1(ef031b5501556c1aa047a51604a44551b35a8b99) )
	ROM_LOAD16_BYTE( "a05jaa09.25d", 0x100000, 0x80000, CRC(1504D62C) SHA1(3c31c6625bc089235a96fe21021239f2d0c0f6e1) )
	ROM_LOAD16_BYTE( "a05jaa10.27d", 0x100001, 0x80000, CRC(99D75C36) SHA1(9599420863aa0a9492d3caeb03f8ac5fd4c3cdb2) )

	DISK_REGION( REGION_DISKS )			/* IDE HARD DRIVE */
	DISK_IMAGE( "a05jaa11.chd", 0, MD5(180f7b1b2145fab2d2ba717780f2ca26) )
ROM_END





/*************************************
 *
 *	Driver-specific init
 *
 *************************************/

static DRIVER_INIT( djmain )
{
	if (new_memory_region(REGION_SOUND1, 0x80000 * 32, 0))
		return;

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);

	state_save_register_int   ("djmain", 0, "sndram_bank",    &sndram_bank);
	state_save_register_UINT8 ("djmain", 0, "sndram",         memory_region(REGION_SOUND1), 0x80000 * 32);
	state_save_register_int   ("djmain", 0, "pending_vb_int", &pending_vb_int);
	state_save_register_UINT16("djmain", 0, "v_ctrl",         &v_ctrl,  1);
	state_save_register_UINT32("djmain", 0, "obj_regs",       obj_regs, sizeof (obj_regs) / sizeof (UINT32));

	state_save_register_func_postload(sndram_set_bank);
}

static UINT8 beatmania_master_password[2 + 32] =
{
	0x01, 0x00,
	0x4d, 0x47, 0x43, 0x28, 0x4b, 0x29, 0x4e, 0x4f,
	0x4d, 0x41, 0x20, 0x49, 0x4c, 0x41, 0x20, 0x4c,
	0x49, 0x52, 0x48, 0x47, 0x53, 0x54, 0x52, 0x20,
	0x53, 0x45, 0x52, 0x45, 0x45, 0x56, 0x2e, 0x44
};

static DRIVER_INIT( hmcompmx )
{
	static UINT8 hmcompmx_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3a, 0x34, 0x38, 0x2a,
		0x5a, 0x4d, 0x78, 0x3e, 0x74, 0x61, 0x6c, 0x0a,
		0x7a, 0x63, 0x19, 0x77, 0x73, 0x7d, 0x0d, 0x12,
		0x6b, 0x09, 0x02, 0x0f, 0x05, 0x00, 0x7d, 0x1b
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompmx_user_password);
}

static DRIVER_INIT( bmcompm2 )
{
	static UINT8 bmcompm2_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x3a, 0x20, 0x31, 0x3e, 0x46, 0x2c, 0x35, 0x46,
		0x48, 0x51, 0x6f, 0x3e, 0x73, 0x6b, 0x68, 0x0a,
		0x60, 0x71, 0x19, 0x6f, 0x70, 0x68, 0x07, 0x62,
		0x6b, 0x0d, 0x71, 0x0f, 0x1d, 0x10, 0x7d, 0x7a
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmcompm2_user_password);
}

static DRIVER_INIT( hmcompm2 )
{
	static UINT8 hmcompm2_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x3b, 0x39, 0x24, 0x3e, 0x4e, 0x59, 0x5c, 0x32,
		0x3b, 0x4c, 0x72, 0x57, 0x69, 0x04, 0x79, 0x65,
		0x76, 0x10, 0x6a, 0x77, 0x1f, 0x65, 0x0a, 0x16,
		0x09, 0x68, 0x71, 0x0b, 0x77, 0x15, 0x17, 0x1e
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, hmcompm2_user_password);
}

static DRIVER_INIT( bmcorerm )
{
	static UINT8 bmcorerm_user_password[2 + 32] =
	{
		0x00, 0x00,
		0x44, 0x42, 0x56, 0x4b, 0x3f, 0x4d, 0x4a, 0x27,
		0x5a, 0x52, 0x0c, 0x3e, 0x6a, 0x04, 0x63, 0x6f,
		0x72, 0x64, 0x72, 0x7f, 0x1f, 0x73, 0x17, 0x04,
		0x05, 0x09, 0x14, 0x0d, 0x7a, 0x74, 0x7d, 0x7a
	};

	init_djmain();

	ide_set_master_password(0, beatmania_master_password);
	ide_set_user_password(0, bmcorerm_user_password);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1999, bmcompmx, 0,        djmain,   djmain,   djmain,   ROT0, "Konami", "Beatmania Complete MIX" )
GAME( 1999, hmcompmx, bmcompmx, djmain,   djmain,   hmcompmx, ROT0, "Konami", "Hiphopmania Complete MIX" )
GAME( 2000, bmcompm2, 0,        djmain,   djmain,   bmcompm2, ROT0, "Konami", "Beatmania Complete MIX 2" )
GAME( 2000, hmcompm2, bmcompm2, djmain,   djmain,   hmcompm2, ROT0, "Konami", "Hiphopmania Complete MIX 2" )
GAME( 2000, bmcorerm, 0,        djmain,   djmain,   bmcorerm, ROT0, "Konami", "Beatmania CORE REMIX" )
