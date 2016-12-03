#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

constexpr unsigned WinWidth = 288, WinHeight = 512;

struct Image
{
    SDL_Surface* image;
    short x, y;
    Image(std::string file, short x, short y) { image = SDL_DisplayFormat(IMG_Load(file.c_str()));
                                                this->x = x;
                                                this->y = y; }
    ~Image() { SDL_FreeSurface(image); }
};
template<typename T, typename... R>
static void render(SDL_Surface* sdl, T* i, R&&... r)
{
    render(sdl, i);
    render(sdl, std::forward<R>(r)...);
}
template<>
inline void render<Image>(SDL_Surface* sdl, Image* i)
{
    SDL_Rect r{i->x, i->y, 0, 0};
    SDL_BlitSurface(i->image, nullptr, sdl, &r);
}

int main()
{
    SDL_Surface* sdl = SDL_SetVideoMode(WinWidth, WinHeight, 32, 0);

    Image bgnd("assets/bg1.png", 0, 0);
    Image bgnd2("assets/bg1.png", 288, 0);

    bool quit = false;
    unsigned delta{}, lastFrame{};
    while (!quit) {
        // calculate delta time vars
        unsigned currentFrame = SDL_GetTicks();
        delta = currentFrame - lastFrame;
        lastFrame = currentFrame;

        bgnd.x -= delta / 2;
        bgnd2.x -= delta / 2;
        if (bgnd.x < -288) bgnd.x = 287;
        if (bgnd2.x < -288) bgnd2.x = 287;

        render(sdl, &bgnd, &bgnd2);

        for (SDL_Event e; SDL_PollEvent(&e); ) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
        }
        SDL_Flip(sdl);
        SDL_FillRect(sdl, nullptr, 0x0);
    }

    SDL_Quit();
}
