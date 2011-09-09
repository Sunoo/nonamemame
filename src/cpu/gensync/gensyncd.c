#include <stdio.h>
#include <string.h>
#include "gensync.h"

unsigned gensyncd(char *buffer, unsigned PC)
{
	int hblank = activecpu_get_reg(GS_HBLANK);
	int hsync = activecpu_get_reg(GS_HSYNC);
	int vblank = activecpu_get_reg(GS_VBLANK);
	int vsync = activecpu_get_reg(GS_VSYNC);

	if( hblank )
	{
		buffer += sprintf(buffer, " HB");
		if( hsync )
			buffer += sprintf(buffer, " HS");
    }
	else
		buffer += sprintf(buffer, " X:%03X", activecpu_get_reg(GS_X));

    if( vblank )
    {
		buffer += sprintf(buffer, " VB");
		if( vsync )
			buffer += sprintf(buffer, " VS");
    }
	else
		buffer += sprintf(buffer, " Y:%03X", activecpu_get_reg(GS_Y));

    return 1;
}

