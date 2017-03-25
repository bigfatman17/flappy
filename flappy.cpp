#include <iostream>
#include <array>
#include <cmath>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

constexpr unsigned WinWidth = 288, WinHeight = 512;

static constexpr float BgndVelocity = 10;

static constexpr float BirdAcceleration =  600,
                       BirdJumpVel      = -200,
                       BirdJumpRot      = - 18,
                       BirdRotAccel     =  100, // Having the bird's rotation accelerate felt more natural while testing.
                       BirdAnimSpeed    =   25;

static constexpr float PipeVelocity =  70,
                       PipeSpacing  = 100,
                       PipeBounds   = 200;

// This feature is included in C++17, but not supported by GCC yet.
template<typename T>
static constexpr T clamp(const T& n, const T& lo, const T& hi) { return std::max(lo, std::min(n, hi)); }

struct Texture
{
    std::shared_ptr<SDL_Texture> tex;
    unsigned short w, h;
    Texture(SDL_Renderer* sdl, std::string file, unsigned short w = 0, unsigned short h = 0)
        : tex(SDL_CreateTextureFromSurface(sdl, IMG_Load(file.c_str())), [](SDL_Texture* x) { SDL_DestroyTexture(x); }), w(w), h(h)
    {
        if (w == 0 && h == 0) {
            int w, h;
            SDL_QueryTexture(tex.get(), nullptr, nullptr, &w, &h);
            this->w = w; this->h = h;
        }
    }
};
struct Image
{
    Texture tex;
    unsigned short w() const { return tex.w; }
    unsigned short h() const { return tex.h; }
    unsigned short& w() { return tex.w; }
    unsigned short& h() { return tex.h; }
    float x{}, y{}, rot{};
};
enum AnimType { NORMAL, YOYO };
template<unsigned n>
struct Animation
{
    AnimType type;
    float speed;
    std::array<Texture, n> texs;
    float x{}, y{}, rot{};

    unsigned short w() const { return current.w; }
    unsigned short h() const { return current.h; }
    unsigned short& w() { return current.w; }
    unsigned short& h() { return current.h; }

    Texture current = texs[0];
    void update(float delta)
    {
        static float pos{};
        switch (type) {
            case NORMAL:
                pos = pos >= n ? 0 : pos + speed * delta;
                break;
            case YOYO:
                static bool dir{};
                if (dir) { pos -= speed * delta; dir = !(pos <= 0); }
                else { pos += speed * delta; dir = pos >= n; }
                break;
        };
        current = texs[clamp(pos, 0.f, (float)n - 1)];
    };
};
inline void render(SDL_Renderer* sdl, const Image& i)
{
    SDL_Rect r{static_cast<short>(i.x),
               static_cast<short>(i.y),
               i.w(), i.h()};
    SDL_RenderCopyEx(sdl, i.tex.tex.get(), nullptr, &r, i.rot, nullptr, SDL_FLIP_NONE);
}
template<unsigned n>
inline void render(SDL_Renderer* sdl, const Animation<n>& i)
{
    SDL_Rect r{static_cast<short>(i.x),
               static_cast<short>(i.y),
               i.w(), i.h()};
    SDL_RenderCopyEx(sdl, i.current.tex.get(), nullptr, &r, i.rot, nullptr, SDL_FLIP_NONE);
}
template<typename T, typename... R>
static void render(SDL_Renderer* sdl, const T& i, R&&... r)
{
    render(sdl, i);
    render(sdl, std::forward<R>(r)...);
}

struct Bird
{
    Animation<3> img;
    float vel{}, rotvel{};
    float& rot() { return img.rot; }

    void update(float delta) {
        vel += BirdAcceleration * delta;
        rotvel += BirdRotAccel * delta;
        img.y += vel * delta;
        rot() += vel >= 0 && rot() < 90 ? rotvel * delta : 0;
        img.update(delta);
    }
};

struct Pipe
{
    Image up, down;
    Pipe(Image up, Image down) : up(up), down(down) { this->up.y = WinHeight; this->down.y = -this->down.h(); }
    void update(float delta)
    {
        up.x -= PipeVelocity * delta; down.x -= PipeVelocity * delta;
        if (up.x < (int)-WinWidth / 2) {
            // generate new position
            up.x = WinWidth; down.x = WinWidth;
            float pos = std::fmod(std::rand(), ((WinHeight - PipeBounds) - PipeSpacing)) + PipeBounds;
            up.y = pos;
            down.y = pos - PipeSpacing - down.h();
        }
    }
};

int main()
{
    SDL_Window* win = SDL_CreateWindow("flappy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WinWidth, WinHeight, 0);
    SDL_Renderer* sdl = SDL_CreateRenderer(win, -1, 0);

    std::srand(std::time(nullptr));

    Image bgnd{Texture(sdl, "assets/bg1.png")};
    Image bgnd2 = bgnd; bgnd2.x = WinWidth;
    Bird bird{Animation<3>{ YOYO, BirdAnimSpeed, { Texture(sdl, "assets/bird1.png"),
                                                   Texture(sdl, "assets/bird2.png"),
                                                   Texture(sdl, "assets/bird3.png") }, WinWidth / 2 - 34 / 2, WinHeight / 2 - 24 / 2}};

    Pipe pipe(Image{Texture(sdl, "assets/pipeup.png")}, Image{Texture(sdl, "assets/pipedown.png")}), pipe2 = pipe, pipe3 = pipe;
    pipe.up.x = pipe.up.w(); pipe.down.x = pipe.down.w();
    pipe2.up.x = pipe2.up.w() + WinWidth / 2; pipe2.down.x = pipe2.down.w() + WinWidth / 2;
    pipe3.up.x = pipe3.up.w() + WinWidth; pipe3.down.x = pipe3.down.w() + WinWidth;

    //collision checking
    auto coll = [](Bird bird, Pipe pipe)
    {
        return bird.img.x + bird.img.w() > pipe.up.x && bird.img.x < pipe.up.x + pipe.up.w()
            && (bird.img.y < pipe.down.y + pipe.down.h() || bird.img.y + bird.img.h() > pipe.up.y);
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
            if (bgnd.x < -bgnd.w()) bgnd.x = WinWidth - 1;
            if (bgnd2.x < -bgnd2.w()) bgnd2.x = WinWidth - 1;
            pipe.update(delta); pipe2.update(delta); pipe3.update(delta);
            lose = coll(bird, pipe) || coll(bird, pipe2) || coll(bird, pipe3);
        }

        render(sdl, bgnd, bgnd2, pipe.up, pipe.down, pipe2.up, pipe2.down, pipe3.up, pipe3.down, bird.img);

        for (SDL_Event e; SDL_PollEvent(&e); ) {
            if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) || e.type == SDL_QUIT)
                quit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && !lose) {
                bird.vel = BirdJumpVel;
                bird.rot() = BirdJumpRot;
                bird.rotvel = 0;
            }
        }

        SDL_RenderPresent(sdl);
    }

    SDL_DestroyWindow(win); SDL_DestroyRenderer(sdl);
    SDL_Quit();
}
