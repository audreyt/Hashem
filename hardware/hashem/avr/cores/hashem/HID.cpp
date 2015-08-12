

/* Copyright (c) 2011, Peter Barrett  
**  
** Sleep/Wakeup/SystemControl support added by Michael Dreher
**
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
** 
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "USBAPI.h"
#include "USBDesc.h"
#include "HIDTables.h"

#if defined(USBCON)
#ifdef HID_ENABLED
#define HID_MOUSE_ABS_ENABLED
//#define RAWHID_ENABLED

//	Singletons for mouse and keyboard

Mouse_ Mouse;
Keyboard_ Keyboard;

//================================================================================
//================================================================================

//	HID report descriptor

#define LSB(_x) ((_x) & 0xFF)
#define MSB(_x) ((_x) >> 8)

#define RAWHID_USAGE_PAGE	0xFFC0
#define RAWHID_USAGE		0x0C00
#define RAWHID_TX_SIZE 64
#define RAWHID_RX_SIZE 64

#define HID_REPORTID_KEYBOARD (1)
#define HID_REPORTID_MOUSE (2)
#define HID_REPORTID_MOUSE_ABS (3)
#define HID_REPORTID_SYSTEMCONTROL (4)
#define HID_REPORTID_CONSUMERCONTROL (5)
#define HID_REPORTID_RAWHID (6)

#define HID_REPORT_KEYBOARD    /*  Keyboard */ \
    0x05, 0x01,                      /* USAGE_PAGE (Generic Desktop)	  47 */ \
    0x09, 0x06,                      /* USAGE (Keyboard) */ \
    0xa1, 0x01,                      /* COLLECTION (Application) */ \
    0x85, HID_REPORTID_KEYBOARD,     /*   REPORT_ID */ \
    0x05, 0x07,                      /*   USAGE_PAGE (Keyboard) */ \
\
      /* Keyboard Modifiers (shift, alt, ...) */ \
    0x19, 0xe0,                      /*   USAGE_MINIMUM (Keyboard LeftControl) */ \
    0x29, 0xe7,                      /*   USAGE_MAXIMUM (Keyboard Right GUI) */ \
    0x15, 0x00,                      /*   LOGICAL_MINIMUM (0) */ \
    0x25, 0x01,                      /*   LOGICAL_MAXIMUM (1) */ \
    0x75, 0x01,                      /*   REPORT_SIZE (1) */ \
\
    0x95, 0x08,                      /*   REPORT_COUNT (8) */ \
    0x81, 0x02,                      /*   INPUT (Data,Var,Abs) */ \
    0x95, 0x01,                      /*   REPORT_COUNT (1) */ \
    0x75, 0x08,                      /*   REPORT_SIZE (8) */ \
    0x81, 0x03,                      /*   INPUT (Cnst,Var,Abs) */ \
\
      /* Keyboard keys */ \
    0x95, 0x06,                      /*   REPORT_COUNT (6) */ \
    0x75, 0x08,                      /*   REPORT_SIZE (8) */ \
    0x15, 0x00,                      /*   LOGICAL_MINIMUM (0) */ \
    0x26, 0xDF, 0x00,                /*   LOGICAL_MAXIMUM (239) */ \
    0x05, 0x07,                      /*   USAGE_PAGE (Keyboard) */ \
    0x19, 0x00,                      /*   USAGE_MINIMUM (Reserved (no event indicated)) */ \
    0x29, 0xDF,                      /*   USAGE_MAXIMUM (Left Control - 1) */ \
    0x81, 0x00,                      /*   INPUT (Data,Ary,Abs) */ \
    0xc0                            /* END_COLLECTION */

#define HID_REPORT_MOUSE_ABSOLUTE /*  Mouse absolute */ \
    0x05, 0x01,                      /* USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x02,                      /* USAGE (Mouse) */ \
    0xa1, 0x01,                      /* COLLECTION (Application) */ \
    0x09, 0x01,                      /*   USAGE (Pointer) */ \
    0xa1, 0x00,                      /*   COLLECTION (Physical) */ \
    0x85, HID_REPORTID_MOUSE_ABS,    /*     REPORT_ID */ \
    0x05, 0x09,                      /*     USAGE_PAGE (Button) */ \
    0x19, 0x01,                      /*     USAGE_MINIMUM (Button 1) */ \
    0x29, 0x03,                      /*     USAGE_MAXIMUM (Button 3) */ \
    0x15, 0x00,                      /*     LOGICAL_MINIMUM (0) */ \
    0x25, 0x01,                      /*     LOGICAL_MAXIMUM (1) */ \
    0x95, 0x03,                      /*     REPORT_COUNT (3) */ \
    0x75, 0x01,                      /*     REPORT_SIZE (1) */ \
    0x81, 0x02,                      /*     INPUT (Data,Var,Abs) */ \
    0x95, 0x01,                      /*     REPORT_COUNT (1) */ \
    0x75, 0x05,                      /*     REPORT_SIZE (5) */ \
    0x81, 0x03,                      /*     INPUT (Cnst,Var,Abs) */ \
    0x05, 0x01,                      /*     USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x30,                      /*     USAGE (X) */ \
    0x09, 0x31,                      /*     USAGE (Y) */ \
    0x15, 0x00,                      /*     LOGICAL_MINIMUM (0) */ \
    0x26, 0xff, 0x7f,                /*     LOGICAL_MAXIMUM (32767) */ \
    0x75, 0x10,                      /*     REPORT_SIZE (16) */ \
    0x95, 0x02,                      /*     REPORT_COUNT (2) */ \
    0x81, 0x02,                      /*     INPUT (Data,Var,Abs) */ \
    0xc0,                            /*   END_COLLECTION */ \
    0xc0                            /* END_COLLECTION */

#define HID_REPORT_MOUSE_RELATIVE /*  Mouse relative */ \
    0x05, 0x01,                      /* USAGE_PAGE (Generic Desktop)	  54 */ \
    0x09, 0x02,                      /* USAGE (Mouse) */ \
    0xa1, 0x01,                      /* COLLECTION (Application) */ \
    0x09, 0x01,                      /*   USAGE (Pointer) */ \
    0xa1, 0x00,                      /*   COLLECTION (Physical) */ \
    0x85, HID_REPORTID_MOUSE,        /*     REPORT_ID */ \
    0x05, 0x09,                      /*     USAGE_PAGE (Button) */ \
    0x19, 0x01,                      /*     USAGE_MINIMUM (Button 1) */ \
    0x29, 0x05,                      /*     USAGE_MAXIMUM (Button 5) */ \
    0x15, 0x00,                      /*     LOGICAL_MINIMUM (0) */ \
    0x25, 0x01,                      /*     LOGICAL_MAXIMUM (1) */ \
    0x95, 0x05,                      /*     REPORT_COUNT (5) */ \
    0x75, 0x01,                      /*     REPORT_SIZE (1) */ \
    0x81, 0x02,                      /*     INPUT (Data,Var,Abs) */ \
    0x95, 0x01,                      /*     REPORT_COUNT (1) */ \
    0x75, 0x03,                      /*     REPORT_SIZE (3) */ \
    0x81, 0x01,                      /*     INPUT (Cnst,Var,Abs) */ \
    0x05, 0x01,                      /*     USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x30,                      /*     USAGE (X) */ \
    0x09, 0x31,                      /*     USAGE (Y) */ \
    0x09, 0x38,                      /*     USAGE (Wheel) */ \
    0x15, 0x81,                      /*     LOGICAL_MINIMUM (-127) */ \
    0x25, 0x7f,                      /*     LOGICAL_MAXIMUM (127) */ \
    0x75, 0x08,                      /*     REPORT_SIZE (8) */ \
    0x95, 0x03,                      /*     REPORT_COUNT (3) */ \
    0x81, 0x06,                      /*     INPUT (Data,Var,Rel) */ \
    0xc0,                            /*   END_COLLECTION */ \
    0xc0                            /* END_COLLECTION */ 

#define HID_REPORT_SYSTEMCONTROL /*  System Control (Power Down, Sleep, Wakeup, ...) */ \
    0x05, 0x01,                          /* USAGE_PAGE (Generic Desktop) */ \
    0x09, 0x80,                          /* USAGE (System Control) */ \
    0xa1, 0x01,                          /* COLLECTION (Application) */ \
    0x85, HID_REPORTID_SYSTEMCONTROL,    /* REPORT_ID */ \
    0x09, 0x81,                          /*   USAGE (System Power Down) */ \
    0x09, 0x82,                          /*   USAGE (System Sleep) */ \
    0x09, 0x83,                          /*   USAGE (System Wakeup) */ \
    0x09, 0x8E,                          /*   USAGE (System Cold Restart) */ \
    0x09, 0x8F,                          /*   USAGE (System Warm Restart) */ \
    0x09, 0xA0,                          /*   USAGE (System Dock) */ \
    0x09, 0xA1,                          /*   USAGE (System Undock) */ \
    0x09, 0xA7,                          /*   USAGE (System Speaker Mute) */ \
    0x09, 0xA8,                          /*   USAGE (System Hibernate) */ \
      /* although these display usages are not that important, they don't cost */ \
      /* much more than declaring the otherwise necessary constant fill bits */ \
    0x09, 0xB0,                          /*   USAGE (System Display Invert) */ \
    0x09, 0xB1,                          /*   USAGE (System Display Internal) */ \
    0x09, 0xB2,                          /*   USAGE (System Display External) */ \
    0x09, 0xB3,                          /*   USAGE (System Display Both) */ \
    0x09, 0xB4,                          /*   USAGE (System Display Dual) */ \
    0x09, 0xB5,                          /*   USAGE (System Display Toggle Intern/Extern) */ \
    0x09, 0xB6,                          /*   USAGE (System Display Swap) */ \
    0x15, 0x00,                          /*   LOGICAL_MINIMUM (0) */ \
    0x25, 0x01,                          /*   LOGICAL_MAXIMUM (1) */ \
    0x75, 0x01,                          /*   REPORT_SIZE (1) */ \
    0x95, 0x10,                          /*   REPORT_COUNT (16) */ \
    0x81, 0x02,                          /*   INPUT (Data,Var,Abs) */ \
    0xc0                                /* END_COLLECTION */

#define HID_REPORT_CONSUMERCONTROL      /* Consumer Control (Sound/Media keys) */ \
    0x05, 0x0c,                          /* USAGE_PAGE (Consumer Devices)  */ \
    0x09, 0x01,                          /* USAGE (Consumer Control)  */ \
    0xa1, 0x01,                          /* COLLECTION (Application)  */ \
    0x85, HID_REPORTID_CONSUMERCONTROL,  /* REPORT_ID */ \
    0x15, 0x00,                          /* LOGICAL_MINIMUM (0)  */ \
    0x25, 0x01,                          /* LOGICAL_MAXIMUM (1)  */ \
    0x75, 0x01,                          /* REPORT_SIZE (1)  */ \
    0x95, 0x08,                          /* REPORT_COUNT (8)  */ \
    0x09, 0xe2,                          /* USAGE (Mute) 0x01  */ \
    0x09, 0xe9,                          /* USAGE (Volume Up) 0x02  */ \
    0x09, 0xea,                          /* USAGE (Volume Down) 0x03  */ \
    0x09, 0xcd,                          /* USAGE (Play/Pause) 0x04  */ \
    0x09, 0xb7,                          /* USAGE (Stop) 0x05  */ \
    0x09, 0xb6,                          /* USAGE (Scan Previous Track) 0x06  */ \
    0x09, 0xb5,                          /* USAGE (Scan Next Track) 0x07  */ \
    0x09, 0xb8,                          /* USAGE (Eject) 0x08 */ \
    0x81, 0x02,                          /* INPUT (Data,Var,Abs) */ \
    0xc0 

#define HID_REPORT_RAWHID /*    RAW HID */ \
    0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),      /* 30 */ \
    0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE), \
\
    0xA1, 0x01,                  /* Collection 0x01 */ \
    0x85, HID_REPORTID_RAWHID,   /* REPORT_ID */ \
    0x75, 0x08,                  /* report size = 8 bits */ \
    0x15, 0x00,                  /* logical minimum = 0 */ \
    0x26, 0xFF, 0x00,            /* logical maximum = 255 */ \
\
    0x95, 64,                    /* report count TX */ \
    0x09, 0x01,                  /* usage */ \
    0x81, 0x02,                  /* Input (array) */ \
\
    0x95, 64,                    /* report count RX */ \
    0x09, 0x02,                  /* usage */ \
    0x91, 0x02,                  /* Output (array) */ \
    0xC0                         /* end collection */ 

extern const u8 _hidReportDescriptor[] PROGMEM;
const u8 _hidReportDescriptor[] = {

    HID_REPORT_KEYBOARD,
#if RAWHID_ENABLED
    HID_REPORT_RAWHID
#endif
};

extern const HIDDescriptor _hidInterface PROGMEM;
const HIDDescriptor _hidInterface =
{
	D_INTERFACE(HID_INTERFACE,1,3,0,0),
	D_HIDREPORT(sizeof(_hidReportDescriptor)),
	D_ENDPOINT(USB_ENDPOINT_IN (HID_ENDPOINT_INT),USB_ENDPOINT_TYPE_INTERRUPT,0x40,0x01)
};

//================================================================================
//================================================================================
//	Driver

u8 _hid_protocol = 1;
u8 _hid_idle = 1;

#define WEAK __attribute__ ((weak))

int WEAK HID_GetInterface(u8* interfaceNum)
{
	interfaceNum[0] += 1;	// uses 1
	return USB_SendControl(TRANSFER_PGM,&_hidInterface,sizeof(_hidInterface));
}

int WEAK HID_GetDescriptor(int /* i */)
{
	return USB_SendControl(TRANSFER_PGM,_hidReportDescriptor,sizeof(_hidReportDescriptor));
}

void WEAK HID_SendReport(u8 id, const void* data, int len)
{
	USB_Send(HID_TX, &id, 1);
	USB_Send(HID_TX | TRANSFER_RELEASE,data,len);
}

bool WEAK HID_Setup(Setup& setup)
{
	u8 r = setup.bRequest;
	u8 requestType = setup.bmRequestType;
	if (REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType)
	{
		if (HID_GET_REPORT == r)
		{
			//HID_GetReport();
			return true;
		}
		if (HID_GET_PROTOCOL == r)
		{
			//Send8(_hid_protocol);	// TODO
			return true;
		}
	}
	
	if (REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType)
	{
		if (HID_SET_PROTOCOL == r)
		{
			_hid_protocol = setup.wValueL;
			return true;
		}

		if (HID_SET_IDLE == r)
		{
			_hid_idle = setup.wValueL;
			return true;
		}
	}
	return false;
}

//================================================================================
//================================================================================
//	Mouse

Mouse_::Mouse_(void) : _buttons(0)
{
}

void Mouse_::begin(void) 
{
}

void Mouse_::end(void) 
{
}

void Mouse_::click(uint8_t b)
{
	_buttons = b;
	move(0,0,0);
	_buttons = 0;
	move(0,0,0);
}

void Mouse_::move(signed char x, signed char y, signed char wheel)
{
	u8 m[4];
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	HID_SendReport(HID_REPORTID_MOUSE,m,sizeof(m));
}

// X and Y have the range of 0 to 32767. 
// The USB Host will convert them to pixels on the screen.
//
//   x=0,y=0 is top-left corner of the screen
//   x=32767,y=0 is the top right corner
//   x=32767,y=32767 is the bottom right corner
//   x=0,y=32767 is the bottom left corner
//
//   When converting these coordinates to pixels on screen, Mac OS X's
//   default HID driver maps the inner 85% of the coordinate space to
//   the screen's physical dimensions. This means that any value between
//   0 and 2293 or 30474 and 32767 will move the mouse to the screen 
//   edge on a Mac
//
//   For details, see:
//   http://lists.apple.com/archives/usb/2011/Jun/msg00032.html


void Mouse_::moveAbsolute(uint16_t x, uint16_t y)
{
	u8 m[5];
	m[0] = _buttons; 
	m[1] = LSB(x);
	m[2] = MSB(x);
	m[3] = LSB(y);
	m[4] = MSB(y);
	HID_SendReport(HID_REPORTID_MOUSE_ABS,m,sizeof(m));
}

void Mouse_::buttons(uint8_t b)
{
	if (b != _buttons)
	{
		_buttons = b;
		move(0,0,0);
	}
}

void Mouse_::press(uint8_t b) 
{
	buttons(_buttons | b);
}

void Mouse_::release(uint8_t b)
{
	buttons(_buttons & ~b);
}

bool Mouse_::isPressed(uint8_t b)
{
	if ((b & _buttons) > 0) 
		return true;
	return false;
}

//================================================================================
//================================================================================
//	Keyboard

Keyboard_::Keyboard_(void) 
{
}

void Keyboard_::begin(void) 
{
}

void Keyboard_::end(void) 
{
}

void Keyboard_::sendReport(KeyReport* keys)
{
	HID_SendReport(HID_REPORTID_KEYBOARD,keys,sizeof(*keys));
}

extern
const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK  
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT 
	0x00,             // FF 
	0x00,             // CR 
	0x00,             // SO 
	0x00,             // SI 
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM 
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS 
	0x00,             // GS 
	0x00,             // RS 
	0x00,             // US 

	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // 
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};

uint8_t USBPutChar(uint8_t c);


 // pressKeycode() adds the specified key (printing, non-printing, or modifier)
 // to the persistent key report and sends the report.  Because of the way 
 // USB HID works, the host acts like the key remains pressed until we 
 // call releaseKeycode(), releaseAll(), or otherwise clear the report and resend.
 size_t Keyboard_::pressKeycode(uint8_t k)
 {
    if (!addKeycodeToReport(k)) {
        return 0;
    }
 }

 size_t Keyboard_::addKeycodeToReport(uint8_t k)
 {
 	uint8_t index = 0;
 	uint8_t done = 0;
 	
 	if ((k >= HID_KEYBOARD_LEFT_CONTROL) && (k <= HID_KEYBOARD_RIGHT_GUI)) {
 		// it's a modifier key
 		_keyReport.modifiers |= (0x01 << (k - HID_KEYBOARD_LEFT_CONTROL));
 	} else {
 		// it's some other key:
 		// Add k to the key report only if it's not already present
 		// and if there is an empty slot.
 		for (index = 0; index < KEYREPORT_KEYCOUNT; index++) {
 			if (_keyReport.keys[index] != k) { // is k already in list?
 				if (0 == _keyReport.keys[index]) { // have we found an empty slot?
 					_keyReport.keys[index] = k;
 					done = 1;
 					break;
 				}
 			} else {
 				done = 1;
 				break;
 			}
 			
 		}
 		
 		// use separate variable to check if slot was found
 		// for style reasons - we do not know how the compiler
 		// handles the for() index when it leaves the loop
 		if (0 == done) {
 			setWriteError();
 			return 0;
 		}
 	}
 	
 	return 1;
 }
 
 // press() transforms the given key to the actual keycode and calls
 // pressKeycode() to actually press this key.
 //
 size_t Keyboard_::press(uint8_t k) 
 {
 	if (k >= KEY_RIGHT_GUI + 1) {
 		// it's a non-printing key (not a modifier)
 		k = k - (KEY_RIGHT_GUI + 1);
 	} else {
 		if (k >= KEY_LEFT_CTRL) {
 			// it's a modifier key
 			k = k - KEY_LEFT_CTRL + HID_KEYBOARD_LEFT_CONTROL;
 		} else {
 			k = pgm_read_byte(_asciimap + k);
 			if (k) {
 				if (k & SHIFT) {
 					// it's a capital letter or other character reached with shift
 					// the left shift modifier
 					addKeycodeToReport(HID_KEYBOARD_LEFT_SHIFT);
 					k = k ^ SHIFT;
 				}
 			} else {
 				return 0;
 			}
 		}
 	}
 	
 	pressKeycode(k);
 	return 1;
 }
 
// System Control
//  k is one of the SYSTEM_CONTROL defines which come from the HID usage table "Generic Desktop Page (0x01)"
// in "HID Usage Tables" (HUT1_12v2.pdf)
size_t Keyboard_::systemControl(uint8_t k)
{
	if(k <= 16)
	{
		u16 mask = 0;
		u8 m[2];

		if(k > 0)
		{
			mask = 1 << (k - 1);
		}

		m[0] = LSB(mask);
		m[1] = MSB(mask);
		HID_SendReport(HID_REPORTID_SYSTEMCONTROL,m,sizeof(m));

		// these are all OSCs, so send a clear to make it possible to send it again later
		m[0] = 0;
		m[1] = 0;
		HID_SendReport(HID_REPORTID_SYSTEMCONTROL,m,sizeof(m));
		return 1;
	}
	else
	{
		setWriteError();
		return 0;
	}
}

// Consumer Control
//  k is one of the CONSUMER_CONTROL defines which come from the HID usage table "Consumer Devices Page (0x0c)"
// in "HID Usage Tables" (HUT1_12v2.pdf)
size_t Keyboard_::consumerControl(uint8_t k) 
{
	if(k <= 8)
	{
		u16 mask = 0;
		u8 m[2];

		if(k > 0)
		{
			mask = 1 << (k - 1);
		}

		m[0] = LSB(mask);
		m[1] = MSB(mask);
		HID_SendReport(HID_REPORTID_CONSUMERCONTROL,m,sizeof(m));

		// these are all OSCs, so send a clear to make it possible to send it again later
		m[0] = 0;
		m[1] = 0;
		HID_SendReport(HID_REPORTID_CONSUMERCONTROL,m,sizeof(m));
		return 1;
	}
	else
	{
		setWriteError();
		return 0;
	}
}

// releaseKeycode() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
// When send is set to FALSE (= 0) no sendReport() is executed. This comes in
// handy when combining key releases (e.g. SHIFT+A).
size_t Keyboard_::releaseKeycode(uint8_t k)
{
	if (!removeKeycodeFromReport(k)) {
		return 0;
	}
}

size_t Keyboard_::removeKeycodeFromReport(uint8_t k)
{
	uint8_t indexA;
	uint8_t indexB;
	uint8_t count;
	
	if ((k >= HID_KEYBOARD_LEFT_CONTROL) && (k <= HID_KEYBOARD_RIGHT_GUI)) {
		// it's a modifier key
		_keyReport.modifiers = _keyReport.modifiers & (~(0x01 << (k - HID_KEYBOARD_LEFT_CONTROL)));
	} else {
		// it's some other key:
		// Test the key report to see if k is present.  Clear it if it exists.
		// Check all positions in case the key is present more than once (which it shouldn't be)
		for (indexA = 0; indexA < KEYREPORT_KEYCOUNT; indexA++) {
			if (_keyReport.keys[indexA] == k) {
				_keyReport.keys[indexA] = 0;
			}
		}
		
		// finally rearrange the keys list so that the free (= 0x00) are at the
		// end of the keys list - some implementations stop for keys at the
		// first occurence of an 0x00 in the keys list
		// so (0x00)(0x01)(0x00)(0x03)(0x02)(0x00) becomes 
		//    (0x01)(0x03)(0x02)(0x00)(0x00)(0x00)
		count = 0; // holds the number of zeros we've found
		indexA = 0;
		while ((indexA + count) < KEYREPORT_KEYCOUNT) {
			if (0 == _keyReport.keys[indexA]) {
				count++; // one more zero
				for (indexB = indexA; indexB < KEYREPORT_KEYCOUNT-count; indexB++) {
					_keyReport.keys[indexB] = _keyReport.keys[indexB+1];
				}
				_keyReport.keys[KEYREPORT_KEYCOUNT-count] = 0;
			} else {
				indexA++; // one more non-zero
			}
		}
	}		
	
	return 1;
}

// release() transforms the given key to the actual keycode and calls
// releaseKeycode() to actually release this key.
//
size_t Keyboard_::release(uint8_t k) 
{
	uint8_t i;
	
	if (k >= KEY_RIGHT_GUI + 1) {
		// it's a non-printing key (not a modifier)
		k = k - (KEY_RIGHT_GUI + 1);
	} else {
		if (k >= KEY_LEFT_CTRL) {
			// it's a modifier key
			k = k - KEY_LEFT_CTRL + HID_KEYBOARD_LEFT_CONTROL;
		} else {
			k = pgm_read_byte(_asciimap + k);
			if (k) {
				if ((k & SHIFT)) {
					// it's a capital letter or other character reached with shift
					// the left shift modifier
					removeKeycodeFromReport(HID_KEYBOARD_LEFT_SHIFT);
					k = k ^ SHIFT;
				}
			} else {
				return 0;
			}
		}
	}
	
	releaseKeycode(k);
	return 1;
}

void Keyboard_::releaseAll(void)
{
    memset(&_keyReport, 0x00, sizeof(_keyReport));
    sendCurrentReport();
}

void Keyboard_::sendCurrentReport(void) {
	sendReport(&_keyReport);
}

size_t Keyboard_::write(uint8_t c)
{	
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t Keyboard_::writeKeycode(uint8_t c)
{	
	uint8_t p = pressKeycode(c);		// Keydown
	releaseKeycode(c);		// Keyup
	return (p);					// just return the result of pressKeycode() since release() almost always returns 1
}

#endif

#endif /* if defined(USBCON) */
