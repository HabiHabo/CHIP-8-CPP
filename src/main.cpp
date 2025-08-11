#include <SDL2/SDL.h>
#include <iostream>

// For building, use this in terminal in the src folder:
// g++ main.cpp -o main `sdl2-config --cflags --libs`

//Then run this:
//./main

int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "An SDL2 window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cout << "Could not create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cout << "Could not create renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set draw color to black (RGBA)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    // Clear the screen with the draw color (black)
    SDL_RenderClear(renderer);

    // Present the renderer to update the window
    SDL_RenderPresent(renderer);

    SDL_Delay(3000);

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
