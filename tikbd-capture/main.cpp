//
//  main.cpp
//  tikbd-capture
//
//  Created by Erik Piehl on 9.7.2022.
//

#include "../include/SDL2/SDL.h"
#include <iostream>

SDL_Window* window = nullptr;
int width = 320;
int height = 240;
const char *mytitle = "TIBKD-Capture";
bool running = true;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
SDL_Texture *current = nullptr;

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
  return 0;
}
