//
//  serialprotocol.h
//  tikbd-capture
//
//  Created by Erik Piehl on 10.7.2022.
//

#ifndef serialprotocol_h
#define serialprotocol_h

// TI specific keycodes for a few keys, selected not to collide with SDL key codes
// used here.
#define TI_SHIFT        SDL_SCANCODE_LSHIFT   // 225
#define TI_CTRL         SDL_SCANCODE_LCTRL    // 224
#define TI_FCTN         SDL_SCANCODE_LALT     // 226
#define TI_LEFT         128
#define TI_RIGHT        129
#define TI_UP           130
#define TI_DOWN         131
#define TI_BACKSPACE    132
#define TI_ALPHA_LOCK   133


// My command codes. Transfer format contains two bytes:
// one of the below, followed by the keycode.
#define SERIAL_KEYDOWN  '1'
#define SERIAL_KEYUP    '0'
#define SERIAL_ALLUP    '2'

#endif /* serialprotocol_h */
