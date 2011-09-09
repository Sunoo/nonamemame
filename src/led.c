/***************************************************************************

  led.c - Controls keyboard LEDs accurately and reliably
  
  code for linux also exists, but has not been tested

  WIN32 code will only compile with MS Visual C++

 ***************************************************************************/

#include "led.h"

#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <winioctl.h>

int FlashKeyboardLight(HANDLE hKbdDev, UINT LedFlag, int Toggle)
{

	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
	KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
	UINT				LedFlagsMask;
	ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
	ULONG				ReturnedLength; // Number of bytes returned in output buffer

	InputBuffer.UnitId = 0;
	OutputBuffer.UnitId = 0;

	// Preserve current indicators' state
	//
	if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
				&InputBuffer, DataLength,
				&OutputBuffer, DataLength,
				&ReturnedLength, NULL))
		return GetLastError();

	// Mask bit for light to be manipulated
	//
	LedFlagsMask = (OutputBuffer.LedFlags & (~LedFlag));

	// Set toggle variable to reflect current state.
	//
	
	InputBuffer.LedFlags = (unsigned short) (LedFlagsMask | (LedFlag * Toggle));
	
	if (!DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
		&InputBuffer, DataLength,
		NULL,	0,	&ReturnedLength, NULL))
		return GetLastError();

	return 0;

}


HANDLE OpenKeyboardDevice(int *ErrorNumber)
{

	HANDLE	hndKbdDev;
	int		*LocalErrorNumber;
	int		Dummy;

	if (ErrorNumber == NULL)
		LocalErrorNumber = &Dummy;
	else
		LocalErrorNumber = ErrorNumber;

	*LocalErrorNumber = 0;
	
	if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
				"\\Device\\KeyboardClass0"))
	{
		*LocalErrorNumber = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

	hndKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
				NULL,	OPEN_EXISTING,	0,	NULL);
	
	if (hndKbdDev == INVALID_HANDLE_VALUE)
		*LocalErrorNumber = GetLastError();

	return hndKbdDev;

}

int CloseKeyboardDevice(HANDLE hndKbdDev)
{

	int e = 0;

	if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		e = GetLastError();

	if (!CloseHandle(hndKbdDev))					
		e = GetLastError();

	return e;
}

#endif

// end Win32 specific code

// changes keyboard LED lights to simulate the skill LEDs in Space Ace
// changed_led (false, false, false) set all 3 lights to off
// changed_led (true, true, true) sets all 3 lights to on.

void change_led(int num_lock, int caps_lock, int scroll_lock)
{

	// are the LED's enabled
#ifdef WIN32

		HANDLE			hndKbdDev;

		hndKbdDev = OpenKeyboardDevice(NULL);

		FlashKeyboardLight(hndKbdDev, KEYBOARD_SCROLL_LOCK_ON, scroll_lock);
		FlashKeyboardLight(hndKbdDev, KEYBOARD_NUM_LOCK_ON, num_lock);
		FlashKeyboardLight(hndKbdDev, KEYBOARD_CAPS_LOCK_ON, caps_lock);

		CloseKeyboardDevice(hndKbdDev);				

#endif

#ifdef LINUX
		int keyval = 0;	// holds LED bit values
		int fd = open("/dev/console", O_NOCTTY);
	
		// if we were able to open console ok (must be root)
		if (fd != -1)
		{
			if (num_lock) keyval |= LED_NUM;
			if (scroll_lock) keyval |= LED_SCR;
			if (caps_lock) keyval |= LED_CAP;

			// set the LED's here and check for error
			if (ioctl(fd, KDSETLED, keyval) == -1)
			{
				perror("Could not set keyboard leds because");
			}
			close(fd);
		}
		else
		{
			perror("Could not open /dev/console, are you root? Error is ");
		}
#endif
}




