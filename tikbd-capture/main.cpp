//
//  main.cpp
//  tikbd-capture
//
//  Created by Erik Piehl on 9.7.2022.
//

#include "../include/SDL2/SDL.h"
#include <iostream>
#include <map>
#include "serialport.hpp"
#include "serialprotocol.h"
#include <unistd.h>

SDL_Window* window = nullptr;
int width = 320;
int height = 240;
const char *mytitle = "TIBKD-Capture";
bool running = true;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
SDL_Texture *current = nullptr;

int serial_port = -1;

std::map<int, std::string> tikey_to_string =  {
  { TI_SHIFT, "Shift" },
  { TI_ALPHA_LOCK, "Alpha Lock" },
  { TI_CTRL,    "CTRL "},
  { TI_FCTN,    "FCTN"},
  { TI_LEFT ,   "Left" },
  { TI_RIGHT,   "Right" },
  { TI_UP   ,   "Up" },
  { TI_DOWN ,   "Down" },
};

std::map<int,int> sdl_to_tikey = {
  // arrow keys and convenience keys
  {SDL_SCANCODE_DOWN,   TI_DOWN },
  {SDL_SCANCODE_UP,     TI_UP },
  {SDL_SCANCODE_LEFT,   TI_LEFT },
  {SDL_SCANCODE_RIGHT,  TI_RIGHT },
  {SDL_SCANCODE_TAB,    SDL_SCANCODE_TAB },
  
  // ESC all keys up
  { SDL_SCANCODE_ESCAPE,   SDL_SCANCODE_ESCAPE },
  
  // First row
  { SDL_SCANCODE_1, '1' },
  { SDL_SCANCODE_2, '2' },
  { SDL_SCANCODE_3, '3' },
  { SDL_SCANCODE_4, '4' },
  { SDL_SCANCODE_5, '5' },
  { SDL_SCANCODE_6, '6' },
  { SDL_SCANCODE_7, '7' },
  { SDL_SCANCODE_8, '8' },
  { SDL_SCANCODE_9, '9' },
  { SDL_SCANCODE_0, '0' },
  { SDL_SCANCODE_MINUS, '=' },
  
  // alphakeys
  { SDL_SCANCODE_A,     'A'},
  { SDL_SCANCODE_B,     'B'},
  { SDL_SCANCODE_C,     'C'},
  { SDL_SCANCODE_D,     'D'},
  { SDL_SCANCODE_E,     'E'},
  { SDL_SCANCODE_F,     'F'},
  { SDL_SCANCODE_G,     'G'},
  { SDL_SCANCODE_H,     'H'},
  { SDL_SCANCODE_I,     'I'},
  { SDL_SCANCODE_J,     'J'},
  { SDL_SCANCODE_K,     'K'},
  { SDL_SCANCODE_L,     'L'},
  { SDL_SCANCODE_M,     'M'},
  { SDL_SCANCODE_N,     'N'},
  { SDL_SCANCODE_O,     'O'},
  { SDL_SCANCODE_P,     'P'},
  { SDL_SCANCODE_Q,     'Q'},
  { SDL_SCANCODE_R,     'R'},
  { SDL_SCANCODE_S,     'S'},
  { SDL_SCANCODE_T,     'T'},
  { SDL_SCANCODE_U,     'U'},
  { SDL_SCANCODE_V,     'V'},
  { SDL_SCANCODE_W,     'W'},
  { SDL_SCANCODE_X,     'X'},
  { SDL_SCANCODE_Y,     'Y'},
  { SDL_SCANCODE_Z,     'Z'},
  // rightmost keys in alpha rows
  { SDL_SCANCODE_LEFTBRACKET,     '/' }, // keep as is
  { SDL_SCANCODE_SEMICOLON, ';' },
  { SDL_SCANCODE_RETURN,    '\r' },
  
  // row 4 non alpha keys
  { SDL_SCANCODE_LSHIFT,  TI_SHIFT },
  { SDL_SCANCODE_COMMA,   ',' },
  { SDL_SCANCODE_PERIOD,  '.' },
  { SDL_SCANCODE_RSHIFT,  TI_SHIFT },

  // bottom row
  { SDL_SCANCODE_CAPSLOCK,  TI_ALPHA_LOCK },
  { SDL_SCANCODE_LCTRL,     TI_CTRL },
  { SDL_SCANCODE_SPACE,     ' '},
  { SDL_SCANCODE_LALT,      TI_FCTN},
  { SDL_SCANCODE_RALT,      TI_FCTN},
};

void handle_event(SDL_Event &event) {
  switch (event.type) {
    case SDL_QUIT:
      running = false;
      break;
    case SDL_WINDOWEVENT:
      // if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
      //        blit_renderer->resize(event.window.data1, event.window.data2);
      // }
      break;
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      //blit_input->handle_mouse(event.button.button, event.type == SDL_MOUSEBUTTONDOWN, event.button.x, event.button.y);
      break;
    case SDL_MOUSEMOTION:
      // if (event.motion.state & SDL_BUTTON_LMASK) {
      //        blit_input->handle_mouse(SDL_BUTTON_LEFT, event.motion.state & SDL_MOUSEBUTTONDOWN, event.motion.x, event.motion.y);
      // }
      break;
    case SDL_KEYDOWN: // fall-though
    case SDL_KEYUP:
      {
        // if (!blit_input->handle_keyboard(event.key.keysym.sym, event.type == SDL_KEYDOWN)) {
        // }
        // https://wiki.libsdl.org/SDL_RenderCopy
        SDL_SetRenderTarget(renderer, current);
        static bool init = false;
        if(init) {
          init = false;
          SDL_SetRenderDrawColor(renderer, 0, 0, 128, 0);
          SDL_RenderClear(renderer);
        }
        int y = event.key.keysym.scancode;
        static int toggler = 0;
        if(event.type == SDL_KEYDOWN) {
          switch(toggler) {
            case 0: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0); break;
            case 1: SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0); break;
            case 2: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 0); break;
            default:
              break;
          }
          if(++toggler == 3)
            toggler = 0;
        } else {
          // KEYUP
          SDL_SetRenderDrawColor(renderer, 128, 128, 128,0);
        }
        SDL_RenderDrawLine(renderer, 0, y, width-1, y);
        // Not sure if the RenderDrawLine works for textures. F... it. Let's just lock and draw manually.
        uint8_t *p = nullptr;
        int pitch;
        SDL_LockTexture(current, nullptr, (void **)&p, &pitch);
        if(p != nullptr) {
          uint8_t r,g,b;
          if(event.type == SDL_KEYDOWN) {
            r = 255; g=0; b=0;
          } else {
            r = g = b = 128;
          }
          uint8_t *d = p + pitch*y;
          for(int x=0; x<width; x++) {
            *d++ = r;
            *d++ = g;
            *d++ = b;
          }
        }
        SDL_UnlockTexture(current);

        SDL_SetRenderTarget(renderer, nullptr);  // test - render to window
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 0);
        SDL_RenderClear(renderer);
        
        int ww = width, hh = height;
        SDL_GetRendererOutputSize(renderer, &ww, &hh);
        SDL_Rect dest;
        dest.x = dest.y = 0;
        dest.w = ww;
        dest.h = hh;
        SDL_RenderCopy(renderer, current, nullptr, &dest); // test
        SDL_RenderPresent(renderer);

        int k = event.key.keysym.scancode;
        auto i = sdl_to_tikey.find(k);
        if(i != sdl_to_tikey.end()) {
          // The key we are processing here is a key beloning to the TI keyboard.
          // The converted keycode is in i->second.
          // It should be sent to the TI along with the KEYUP or KEYDOWN information.
          std::string s;
          auto j = tikey_to_string.find(i->second);
          if(j != tikey_to_string.end())
            s = j->second;
          else
            s = i->second;
          std::cout << "SDL " << i->first << " to " << i->second << " " << s << std::endl;
          
          // send over serial.
          if(serial_port != -1) {
            uint8_t buf[2];
            if(i->second == SDL_SCANCODE_ESCAPE) {
              buf[0] = SERIAL_ALLUP;
              buf[1] = i->second;
            } else {
              buf[0] = event.type == SDL_KEYDOWN ? SERIAL_KEYDOWN : SERIAL_KEYUP;
              buf[1] = i->second;
            }
            write(serial_port, buf, sizeof(buf));

            // check if we can read something from the serial port.
            uint8_t rxbuf[40];
            int rd = read(serial_port, rxbuf, sizeof(rxbuf));
            if(rd > 0) {
              // We did receive some data. Anyway let's just throw it away.
              std::cout << "received: " << rd << " bytes" << std::endl;
            }
          }
        }

        std::cout << (event.type == SDL_KEYDOWN ? "keydown " : "keyup ") << "scancode: " << event.key.keysym.scancode << std::endl;
      }
      break;
    case SDL_RENDER_TARGETS_RESET:
      std::cout << "Targets reset" << std::endl;
      break;
    case SDL_RENDER_DEVICE_RESET:
      std::cout << "Device reset" << std::endl;
      break;
    default:
//      if(event.type == System::loop_event) {
//        blit_renderer->update(blit_system);
//        blit_system->notify_redraw();
//        blit_renderer->present();
//      } else
//      if (event.type == System::timer_event) {
        switch(event.user.code) {
          case 0:
            SDL_SetWindowTitle(window, mytitle);
            break;
          case 1:
            SDL_SetWindowTitle(window, (std::string(mytitle) + " [SLOW]").c_str());
            break;
          case 2:
            SDL_SetWindowTitle(window, (std::string(mytitle) + " [FROZEN]").c_str());
            break;
        }
//      }
      break;
    }
}

int main(int argc, const char * argv[]) {
  
  // serial_port = open_serial_port("/dev/cu.usbserial-AH02Z7BM");
  serial_port = open_serial_port("/dev/cu.usbmodem1362403");
  if(serial_port < 0) {
    std::cerr << "unable to open serial port." << std::endl;
    return 1;
  }
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
          std::cerr << "could not initialize SDL2: " << SDL_GetError() << std::endl;
          return 1;
  }

  window = SDL_CreateWindow(
          mytitle,
          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
          width*2, height*2,
          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );

  if (window == nullptr) {
          std::cerr << "could not create window: " << SDL_GetError() << std::endl;
          return 1;
  }
  
  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == nullptr) {
    std::cerr << "could not create renderer: " << SDL_GetError() << std::endl;
    return 1;
  }
  
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
  if(texture == nullptr) {
    std::cerr << "could not create texture: " << SDL_GetError() << std::endl;
    return 1;
  }
  
  current = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
  if(current == nullptr) {
    std::cerr << "could not create texture: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Clear the window.
  SDL_SetRenderTarget(renderer, texture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
  
  
  SDL_SetWindowMinimumSize(window, width, height);
  
  SDL_Event event;
  while (running && SDL_WaitEvent(&event)) {
          handle_event(event);
  }
  
  SDL_DestroyTexture(texture);
  SDL_DestroyTexture(current);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  close_serial_port(serial_port);
  return 0;
}
