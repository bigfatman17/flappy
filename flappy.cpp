#include <cstdlib>
#include <ctime>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

constexpr unsigned WinWidth = 288, WinHeight = 512;
// NOTE: Despite the names, these variables represent the frame number when their items should move (except for PipeSpacing and PipeBounds).
// Thus, by lowering these values, the item will traverse faster.
// Everything is based on 60 frames per second.
static constexpr unsigned BgndVelocity = 10, BirdAcceleration = 5, PipeVelocity = 2, PipeSpacing = 100, PipeBounds = 200;

class Texture
{
    SDL_Texture* s;
public:
    SDL_Texture* S() const { return s; }
    Texture(SDL_Renderer* sdl, std::string file) { s = SDL_CreateTextureFromSurface(sdl, IMG_Load(file.c_str())); }
    Texture(const Texture& o) { s = o.s; }
    Texture(Texture&& o)      { s = o.s; o.s = nullptr; }
    Texture& operator=(Texture o) { std::swap(s, o.s); return *this; }
    ~Texture() { SDL_DestroyTexture(s); }
};
struct Image
{
    Texture image;
    short x, y;
    unsigned short w, h;
    Image(SDL_Renderer* sdl, std::string file,
          unsigned short w, unsigned short h,
          short x = 0, short y = 0) : image(sdl, file) { this->w = w; this->h = h; this->x = x; this->y = y; }
};
template<typename T, typename... R>
static void render(SDL_Renderer* sdl, const T* i, R&&... r)
{
    render(sdl, i);
    render(sdl, std::forward<R>(r)...);
}
template<>
inline void render<Image>(SDL_Renderer* sdl, const Image* i)
{
    SDL_Rect r{i->x, i->y, i->w, i->h};
    SDL_RenderCopy(sdl, i->image.S(), nullptr, &r);
}

struct Bird
{
    Image* img;
    unsigned vel{};

    Bird(Image* img) { this->img = img; }

    void update(unsigned frame) { if (!(frame % BirdAcceleration)) { vel++; img->y += vel; } }
};

struct Pipe
{
    Image* up, *down;
    Pipe(Image* up, Image* down) { this->up = up; this->down = down; this->up->y = WinHeight; this->down->y = -this->down->h; }
    void update(unsigned frame)
    {
        if (!(frame % PipeVelocity)) { up->x--; down->x--; }
        if (up->x < (int)-WinWidth / 2) {
            // generate new position
            up->x = WinWidth; down->x = WinWidth;
            unsigned pos = std::rand() % ((WinHeight - PipeBounds) - PipeBounds) + PipeBounds;
            up->y = pos;
            down->y = pos - PipeSpacing - down->h;
        }
    }
};

int main()
{
    SDL_Window* win = SDL_CreateWindow("flappy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WinWidth, WinHeight, 0);
    SDL_Renderer* sdl = SDL_CreateRenderer(win, -1, 0);

    std::srand(std::time(nullptr));

    Image bgnd(sdl, "assets/bg1.png", 288, 512);
    Image bgnd2(sdl, "assets/bg1.png", 288, 512, WinWidth, 0);
    Bird bird(new Image(sdl, "assets/bird2.png", 34, 24, WinWidth / 2 - 34 / 2, WinHeight / 2 - 24 / 2));

    Pipe pipe(new Image(sdl, "assets/pipeup.png", 52, 320), new Image(sdl, "assets/pipedown.png", 52, 320));
    pipe.up->x = pipe.up->w; pipe.down->x = pipe.down->w;
    Pipe pipe2(new Image(sdl, "assets/pipeup.png", 52, 320), new Image(sdl, "assets/pipedown.png", 52, 320));
    pipe2.up->x = pipe2.up->w + WinWidth / 2; pipe2.down->x = pipe2.down->w + WinWidth / 2;
    Pipe pipe3(new Image(sdl, "assets/pipeup.png", 52, 320), new Image(sdl, "assets/pipedown.png", 52, 320));
    pipe3.up->x = pipe3.up->w + WinWidth; pipe3.down->x = pipe3.down->w + WinWidth;

    //collision checking
    auto coll = [](Bird bird, Pipe pipe)
    {
        return bird.img->x + bird.img->w > pipe.up->x && bird.img->x < pipe.up->x + pipe.up->w
            && (bird.img->y < pipe.down->y + pipe.down->h || bird.img->y + bird.img->h > pipe.up->y);
    };

    bool quit{}, lose{};
    unsigned frame{}, startTicks = SDL_GetTicks();
    while (!quit) {
        unsigned ticks = SDL_GetTicks() - startTicks;
        frame++;

        bird.update(frame);
        if (!lose) {
            if (!(frame % BgndVelocity)) { bgnd.x--; bgnd2.x--; }
            if (bgnd.x < -bgnd.w) bgnd.x = WinWidth - 1;
            if (bgnd2.x < -bgnd2.w) bgnd2.x = WinWidth - 1;
            pipe.update(frame); pipe2.update(frame); pipe3.update(frame);
            lose = coll(bird, pipe) || coll(bird, pipe2) || coll(bird, pipe3);
        }

        render(sdl, &bgnd, &bgnd2, pipe.up, pipe.down, pipe2.up, pipe2.down, pipe3.up, pipe3.down, bird.img);

        for (SDL_Event e; SDL_PollEvent(&e); ) {
            if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) || e.type == SDL_QUIT)
                quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && !lose)
                bird.vel = -10;
        }

        frame = frame > 60 ? 0 : frame;
        if (ticks < 1000 / 150) {
            SDL_Delay(1000 / 150 - ticks);
            startTicks = SDL_GetTicks();
        }

        SDL_RenderPresent(sdl);
        SDL_RenderClear(sdl);
    }

    SDL_DestroyWindow(win); SDL_DestroyRenderer(sdl);
    SDL_Quit();
}
