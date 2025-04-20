#include "SDL.h"

int main(int, char **)
{
    int quit = 0;
    SDL_Event event;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindow("Hello World!",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    while (!quit)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
            quit = 1;
    }
    SDL_Quit();
    return 0;
}
