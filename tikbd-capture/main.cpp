//
//  main.cpp
//  tikbd-capture
//
//  Created by Erik Piehl on 9.7.2022.
//

#include "../include/SDL2/SDL.h"
#include <iostream>
#include <map>
#include <list>
#include <string>
#include "serialport.hpp"
#include "serialprotocol.h"
#include <unistd.h>
#include <dirent.h>

#include "paste.h"

#define SERIAL_DEVICE "/dev/cu.usbmodem1362203"
// #define SERIAL_DEVICE "/dev/cu.usbserial-AH02Z7BM"
// #define SERIAL_DEVICE "/dev/cu.usbmodem1362403"

SDL_Window* window = nullptr;
int width = 320;
int height = 240;
const char *mytitle = "TIBKD-Capture";
bool running = true;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
SDL_Texture *current = nullptr;
bool cmd_down = false;

char paste_buf[16384];
std::list<uint8_t> paste_keys;  // Here are the keypresses to be pasted.
int original_queue_len = 0;

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
  {SDL_SCANCODE_BACKSPACE, TI_BACKSPACE },
  
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

void insert_key_up_down(char c) {
  paste_keys.push_back(SERIAL_KEYDOWN);
  paste_keys.push_back(c);
  paste_keys.push_back(SERIAL_KEYUP);
  paste_keys.push_back(c);
}

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

void insert_fctn_key_up_down(char c) {
  paste_keys.push_back(SERIAL_KEYDOWN);
  paste_keys.push_back(TI_FCTN);
  
  paste_keys.push_back(SERIAL_KEYDOWN);
  paste_keys.push_back(c);
  paste_keys.push_back(SERIAL_KEYUP);
  paste_keys.push_back(c);
  
  paste_keys.push_back(SERIAL_KEYUP);
  paste_keys.push_back(TI_FCTN);
}


bool queue_ascii_paste(char c) {
  if(c >= '0' && c <= '9') {
    insert_key_up_down(c);
    return true;
  }
  if(c >= 'a' && c <= 'z') {
    insert_key_up_down(c - ('a' - 'A'));
    return true;
  }
  if(c >= 'A' && c <= 'Z') {
    insert_shifted_key_up_down(c);
    return true;
  }
  switch(c) {
    case '.':
    case ',':
    case ';':
    case ' ':
    case '/':
    case '=':
      insert_key_up_down(c);
      return true;
    case '<': insert_shifted_key_up_down(','); return true;
    case '>': insert_shifted_key_up_down('.'); return true;
    case ':': insert_shifted_key_up_down(';'); return true;
    case '-': insert_shifted_key_up_down('/'); return true;
    case '+': insert_shifted_key_up_down('='); return true;
    case '!': insert_shifted_key_up_down('1'); return true;
    case '@': insert_shifted_key_up_down('2'); return true;
    case '#': insert_shifted_key_up_down('3'); return true;
    case '$': insert_shifted_key_up_down('4'); return true;
    case '%': insert_shifted_key_up_down('5'); return true;
    case '^': insert_shifted_key_up_down('6'); return true;
    case '&': insert_shifted_key_up_down('7'); return true;
    case '*': insert_shifted_key_up_down('8'); return true;
    case '(': insert_shifted_key_up_down('9'); return true;
    case ')': insert_shifted_key_up_down('0'); return true;
    case '\n': insert_key_up_down('\r'); return true;
    case '\r': return true;
    case '_': insert_fctn_key_up_down('U'); return true;
    case '"': insert_fctn_key_up_down('P'); return true;
  }
  return false; // Don't know what to do here.
}

uint32_t ep_timer_callback(Uint32 interval, void *param) {
  SDL_Event event;
  SDL_UserEvent userevent;

  userevent.type = SDL_USEREVENT;
  userevent.code = 0;
  userevent.data1 = NULL;
  userevent.data2 = NULL;

  event.type = SDL_USEREVENT;
  event.user = userevent;
  static int count = 0;
  std::cout << count++ << std::endl;

  SDL_PushEvent(&event);
  return(interval);
}

void my_render_copy() {
  SDL_SetRenderTarget(renderer, nullptr);  // test - render to window
  int ww = width, hh = height;
  SDL_GetRendererOutputSize(renderer, &ww, &hh);
  SDL_Rect dest;
  dest.x = dest.y = 0;
  dest.w = ww;
  dest.h = hh;
  SDL_RenderCopy(renderer, current, nullptr, &dest); // test
  SDL_RenderPresent(renderer);
}

void handle_event(SDL_Event &event) {
  switch (event.type) {
    case SDL_USEREVENT:
      break;
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
    case SDL_KEYDOWN:
      // fall-though
    case SDL_KEYUP:
      {
        if(event.key.keysym.sym == SDLK_LGUI)
          cmd_down = (event.type == SDL_KEYDOWN);
        
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
          
          // Check for PASTE
          
          if(event.key.keysym.sym == SDLK_PASTE || (event.key.keysym.sym == SDLK_v && cmd_down)) {
            // Let's try to do paste!
            int r = get_paste_data(paste_buf, sizeof(paste_buf));
            if(r > 0) {
              std::cout << "PASTE(" << r << "): " << paste_buf << std::endl;
              for(char *p = paste_buf; *p && p-paste_buf < sizeof(paste_buf); p++)
                queue_ascii_paste(*p);
              original_queue_len = paste_keys.size();
              std::cout << "Original len: " << original_queue_len << std::endl;
            } else
              std::cout << "PASTE returned " << r << std::endl;
          }
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
        
        my_render_copy();
        
        if(cmd_down)
          break;  // If command key is held down, don't send anything here.

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
              paste_keys.clear();
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

std::list<std::string> devnames;

int main(int argc, const char * argv[]) {
  
  const char *dev = SERIAL_DEVICE;
  if(argc > 1)
    dev = argv[1];
  serial_port = open_serial_port(dev);
  if(serial_port < 0) {
    std::cerr << "unable to open serial port:" << dev << std::endl;
    
    // Iterate though the directory
    DIR *dir = opendir("/dev/");
    const char *devname_prefix ="cu.usbmodem";
    if(dir) {
      struct dirent *de = nullptr;
      while((de = readdir(dir)) != nullptr) {
        if(strncmp(de->d_name, devname_prefix, strlen(devname_prefix)) == 0) {
          devnames.push_back(de->d_name);
          
        }
      }
      closedir(dir);
      
      std::cout << "Device list has " << devnames.size() << " entries" << std::endl;
      for(auto i=devnames.begin(); i!=devnames.end(); i++)
        std::cout << "Found " << *i << std::endl;
      
      // Try first device in the list
      std::string name = "/dev/" + *devnames.begin();
      serial_port = open_serial_port(name.c_str());
      if(serial_port >= 0) {
        std::cout << "Successfully opened " << name << std::endl;
      } else {
        std::cerr << "Unable to open " << name << std::endl;
        return 2;
      }

    } else {
      std::cerr << "unable to open directory to search serial ports" << std::endl;
      return 1;
    }
  } else {
    std::cout << "Successfully opened " << dev << std::endl;
  }
  
  

  
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
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
  
  SDL_TimerID timer = 0; // SDL_AddTimer(500, ep_timer_callback, nullptr);
  
  SDL_Event event;
  Uint64 last_time = 0;
  
  while (running) {
     while(SDL_PollEvent(&event)) { // used to be WaitEvent
      handle_event(event);
     }
    if(!running)
      break;
    SDL_Delay(16);
    Uint64 ticks = SDL_GetTicks64();
    if(ticks >= last_time+50) {
      last_time = ticks;
      // If we are pasting, feed a character to the TI.
      if(paste_keys.size()) {
        uint8_t ch[2];
        ch[0] = paste_keys.front();
        paste_keys.pop_front();
        ch[1] = paste_keys.front();
        paste_keys.pop_front();

        if(serial_port >= 0)
          write(serial_port, ch, sizeof(ch));
      }
      // Render a bar showing how many characters still are pending in the paste list.
      if(original_queue_len && paste_keys.size()) {
        int len = paste_keys.size();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
        SDL_SetRenderTarget(renderer, current);
        SDL_RenderClear(renderer);
        const int max_width = 200;
        SDL_Rect r = { 0, 0, max_width, 20 };
        // Erase indicator
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
        SDL_RenderFillRect(renderer, &r);
        // Draw indicator
        r.w = max_width*len/original_queue_len;
        std::cout << r.w << std::endl;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0xff, 0xff);
        SDL_RenderFillRect(renderer, &r);
        SDL_RenderPresent(renderer);
      }
    }
  }
  
  if(timer)
    SDL_RemoveTimer(timer);
  SDL_DestroyTexture(texture);
  SDL_DestroyTexture(current);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  
  close_serial_port(serial_port);
  return 0;
}
