#ifndef LED_H
#define LED_H

#ifdef WIN32

#include <windows.h>

// (some definitions borrowed from ntkbdio.h)

//
// Define the keyboard indicators.
//

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;		// Unit identifier.
    USHORT LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

#endif

void change_led(bool, bool, bool);

#ifdef WIN32

int FlashKeyboardLight(HANDLE hKbdDev, UINT LightFlag, BOOL);
HANDLE OpenKeyboardDevice(int *ErrorNumber);
int CloseKeyboardDevice(HANDLE hndKbdDev);

#endif
#endif
