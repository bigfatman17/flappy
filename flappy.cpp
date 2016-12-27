#include <cmath>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

constexpr unsigned WinWidth = 288, WinHeight = 512;
static constexpr float BgndVelocity = 10, BirdAcceleration = 600, BirdJump = -200, PipeVelocity = 70,
                       PipeSpacing = 100, PipeBounds = 200;

struct Image
{
    std::shared_ptr<SDL_Texture> image;
    float x, y;
    unsigned short w, h;
    Image(SDL_Renderer* sdl, std::string file,
          unsigned short w, unsigned short h,
          float x = 0, float y = 0)
          : image(SDL_CreateTextureFromSurface(sdl, IMG_Load(file.c_str())), [](SDL_Texture* x) { SDL_DestroyTexture(x); }),
            x(x), y(y), w(w), h(h) {}
};
template<typename T, typename... R>
static void render(SDL_Renderer* sdl, T i, R&&... r)
{
    render(sdl, i);
    render(sdl, std::forward<R>(r)...);
}
template<>
inline void render<Image>(SDL_Renderer* sdl, Image i)
{
    SDL_Rect r{static_cast<short>(i.x),
               static_cast<short>(i.y),
               i.w, i.h};
    SDL_RenderCopy(sdl, i.image.get(), nullptr, &r);
}

struct Bird
{
    Image img;
    float vel{};

    Bird(Image img) : img(img) {}

    void update(float delta) {
        vel += BirdAcceleration * delta;
        img.y += vel * delta;
    }
};

struct Pipe
{
    Image up, down;
    Pipe(Image up, Image down) : up(up), down(down) { this->up.y = WinHeight; this->down.y = -this->down.h; }
    void update(float delta)
    {
        up.x -= PipeVelocity * delta; down.x -= PipeVelocity * delta;
        if (up.x < (int)-WinWidth / 2) {
            // generate new position
            up.x = WinWidth; down.x = WinWidth;
            float pos = std::fmod(std::rand(), ((WinHeight - PipeBounds) - PipeBounds)) + PipeBounds;
            up.y = pos;
            down.y = pos - PipeSpacing - down.h;
        }
    }
};

int main()
{
    SDL_Window* win = SDL_CreateWindow("flappy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WinWidth, WinHeight, 0);
    SDL_Renderer* sdl = SDL_CreateRenderer(win, -1, 0);

    std::srand(std::time(nullptr));

    Image bgnd(sdl, "assets/bg1.png", 288, 512);
    Image bgnd2 = bgnd; bgnd2.x = WinWidth;
    Bird bird(Image(sdl, "assets/bird2.png", 34, 24, WinWidth / 2 - 34 / 2, WinHeight / 2 - 24 / 2));

    Pipe pipe(Image(sdl, "assets/pipeup.png", 52, 320), Image(sdl, "assets/pipedown.png", 52, 320)), pipe2 = pipe, pipe3 = pipe;
    pipe.up.x = pipe.up.w; pipe.down.x = pipe.down.w;
    pipe2.up.x = pipe2.up.w + WinWidth / 2; pipe2.down.x = pipe2.down.w + WinWidth / 2;
    pipe3.up.x = pipe3.up.w + WinWidth; pipe3.down.x = pipe3.down.w + WinWidth;

    //collision checking
    auto coll = [](Bird bird, Pipe pipe)
    {
        return bird.img.x + bird.img.w > pipe.up.x && bird.img.x < pipe.up.x + pipe.up.w
            && (bird.img.y < pipe.down.y + pipe.down.h || bird.img.y + bird.img.h > pipe.up.y);
    };

    bool quit{}, lose{};
    float delta{};
    auto startTime = std::chrono::system_clock::now();
    while (!quit) {
        SDL_RenderClear(sdl);

        auto now = std::chrono::system_clock::now();
        delta = std::chrono::duration<float>(now - startTime).count();
        startTime = now;

        bird.update(delta);
        if (!lose) {
            bgnd.x -= BgndVelocity * delta; bgnd2.x -= BgndVelocity * delta;
            if (bgnd.x < -bgnd.w) bgnd.x = WinWidth - 1;
            if (bgnd2.x < -bgnd2.w) bgnd2.x = WinWidth - 1;
            pipe.update(delta); pipe2.update(delta); pipe3.update(delta);
            lose = coll(bird, pipe) || coll(bird, pipe2) || coll(bird, pipe3);
        }

        render(sdl, bgnd, bgnd2, pipe.up, pipe.down, pipe2.up, pipe2.down, pipe3.up, pipe3.down, bird.img);

        for (SDL_Event e; SDL_PollEvent(&e); ) {
            if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) || e.type == SDL_QUIT)
                quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && !lose)
                bird.vel = -200;
        }

        SDL_RenderPresent(sdl);
    }

    SDL_DestroyWindow(win); SDL_DestroyRenderer(sdl);
    SDL_Quit();
}
