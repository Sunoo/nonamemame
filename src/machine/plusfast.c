/* PacMAME */
/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

static UINT8 speedcheat = 0;	/* a well known hack allows to make Pac Man run at four times */
					/* his usual speed. When we start the emulation, we check if the */
					/* hack can be applied, and set this flag accordingly. */

MACHINE_INIT( plusfast )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if ((RAM[0x182d] == 0xbe && RAM[0x1ffd] == 0xff) ||
			(RAM[0x182d] == 0x01 && RAM[0x1ffd] == 0xbe))
		speedcheat = 1;
	else
		speedcheat = 0;
}

static INTERRUPT_GEN( plusfast_interrupt )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* speed up cheat */
	if (speedcheat)
	{
		if (readinputport(4) & 1)	/* check status of the fake dip switch */
		{
			/* activate the cheat */
			RAM[0x182d] = 0x01;
			RAM[0x1ffd] = 0xbe;
		}
		else
		{
			/* remove the cheat */
			RAM[0x182d] = 0xbe;
			RAM[0x1ffd] = 0xff;
		}
	}

	irq0_line_hold();
}
/* End PacMAME */
