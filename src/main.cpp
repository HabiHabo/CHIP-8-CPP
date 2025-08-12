#include "tinyfiledialogs.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <fstream>

#include "chip8_core/core.h"
#include <vector>

// For building, use this in terminal in the src folder:
// g++ main.cpp -o main `sdl2-config --cflags --libs`

//Then run this:
//./main

const uint32_t SCALE = 15;
const uint32_t WINDOW_WIDTH = SCREEN_WIDTH * SCALE;
const uint32_t WINDOW_HEIGHT = SCREEN_HEIGHT * SCALE;
const size_t TICKS_PER_FRAME = 10;

void draw_screen(Emu& emu, SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const bool* screen_buf = emu.get_display();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        bool pixel = screen_buf[i];
        if (pixel){
            uint32_t x = i % SCREEN_WIDTH;
            uint32_t y = i / SCREEN_WIDTH;
            SDL_Rect rect;
            rect.x = x * SCALE;
            rect.y = y * SCALE;
            rect.w = SCALE;
            rect.h = SCALE;
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_RenderPresent(renderer);
}

int key2btn(SDL_Keycode key) {
    switch (key) {
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;
        default: return -1;
    }
}

int main() {
    // Optional: file filters (NULL or empty means all files)
    const char* filters[] = { "*.ch8" };

    const char* path = tinyfd_openFileDialog(
        "Select a file",   // Dialog title
        "",                // Default path (empty for none)
        1,                 // Number of filter patterns
        filters,           // Filter patterns
        "Chip8 ROMs",// Description of filters
        0                  // Allow multiple selections? (0 = no)
    );

    if (path) {
        std::cout << "File selected: " << path << std::endl;
    } else {
        std::cout << "No file selected." << std::endl;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return 1;
    }

    // Create a window (position centered, OpenGL-capable)
    SDL_Window* window = SDL_CreateWindow(
        "Chip-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Create a renderer with VSync enabled
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Clear the screen to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    Emu chip8;

    std::ifstream rom(path, std::ios::binary);
    if (!rom.is_open()) {
        std::cerr << "Did not find file: " << path << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<uint8_t> buffer(
        (std::istreambuf_iterator<char>(rom)),
        std::istreambuf_iterator<char>()
    );

    chip8.load(buffer.data(), buffer.size());

    bool running = true;
    SDL_Event evt;

    while (running) {
        // Event pump
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    if (evt.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else {
                        int k = key2btn(evt.key.keysym.sym);
                        if (k != -1) chip8.keypress(k, true);
                    }
                    break;

                case SDL_KEYUP:
                    {
                        int k = key2btn(evt.key.keysym.sym);
                        if (k != -1) chip8.keypress(k, false);
                    }
                    break;
            }
        }

        // Emulation steps
        for (int i = 0; i < TICKS_PER_FRAME; i++) {
            chip8.tick();
        }
        chip8.tick_timers();

        // Drawing
        draw_screen(chip8, renderer);
    }

    
    return 0;
}