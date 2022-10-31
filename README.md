# tikbd-capture

Capture keypresses using SDL2 and send them over serial port to a NUCLEO board connected to TI-99/4A.
This is the host (mac) side program I use to allow me to use my Mac's keyboard with the TI.

Debugged and built with xcode.

Only tested on macos, although it should perhahps work on Linux if the serial port is provided as a command line parameter. At least
if the copy paste code is removed, I don't think I tested the copy-paste code much anyway. Might not work.

On the Mac it searches for serial ports /dev/cu.usbmodem* and chooses one of them if no command line argument provided.

I have used a NUCLEO-F767 board to connect to the keyboard connector of the TI-99/4A. Total overkill, but I had these handy.

## Building it
Simply using the xcode project.

I have copied into the local directory include/SDL2 the SDL2 headers. SDL2 was installed with homebrew.
On my system this stuff came from 
  - /opt/homebrew/Cellar/sdl2/2.24.1/include/SDL2 
  - /opt/homebrew/Cellar/sdl2_ttf/2.20.1/include/SDL2

I know it's not good practice to copy these locally into the project, but I went with that in order not fiddle with xcode project settings.
In the same vain I created a local lib directory inside the project folder and the used libraries are there:

```
libSDL2-2.0.0.dylib
libSDL2.a
libSDL2.dylib
libSDL2_test.a
libSDL2_ttf-2.0.0.dylib
libSDL2_ttf.a
libSDL2_ttf.dylib
libSDL2main.a
```
## Protocol
Use the source, luke. Look at main.cpp where all the action is. 
In principle key presses are sent as two byte commands, first SERIAL_KEYDOWN ('1') followed by the keycode. 
When the key is released SERIAL_KEYUP ('0') is sent followed by the keycode. These command bytes are on purpose selected 
so that they're easy to enter with a terminal program. Useful when debugging the receiving end firmware.

Perhaps the following sequence explains it, this snippet 
is used in the copy-paste code (which might be broken but for the purposes of documenting the way keys are sent this should be helpful):

```c++
void insert_shifted_key_up_down(char c) {
  paste_keys.push_back(SERIAL_KEYDOWN);
  paste_keys.push_back(TI_SHIFT);
  
  paste_keys.push_back(SERIAL_KEYDOWN);
  paste_keys.push_back(c);
  paste_keys.push_back(SERIAL_KEYUP);
  paste_keys.push_back(c);
  
  paste_keys.push_back(SERIAL_KEYUP);
  paste_keys.push_back(TI_SHIFT);
}
```

