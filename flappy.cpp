#include <cstdlib>
#include <ctime>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

constexpr unsigned WinWidth = 288, WinHeight = 512;
// NOTE: Despite the names, these variables represent the frame number when their items should move (except for PipeSpacing and PipeBounds).
// Thus, by lowering these values, the item will traverse faster.
// Everything is based on 60 frames per second.
static constexpr unsigned BgndVelocity = 10, BirdAcceleration = 5, PipeVelocity = 2, PipeSpacing = 100, PipeBounds = 200;

class Surface
{
    SDL_Surface* s;
public:
    SDL_Surface* S() const { return s; }
    Surface(std::string file) { s = IMG_Load(file.c_str()); }
    Surface(const Surface& o) { s = SDL_ConvertSurface(o.s, o.s->format, 0); }
    Surface(Surface&& o)      { swap(*this, o); }
    friend void swap(Surface& a, Surface& b) { using std::swap; swap(a.s, b.s); }
    Surface& operator=(Surface o) { swap(*this, o); return *this; }
    ~Surface() { SDL_FreeSurface(s); }
};
struct Image
{
    Surface image;
    short x, y;
    unsigned short w, h;
    //centers if no coordinates are given
    Image(std::string file) : image(file)                   { w = image.S()->w; h = image.S()->h;
                                                              x = WinWidth / 2 - w / 2;
                                                              y = WinWidth / 2 - h / 2; }
    Image(std::string file, short x, short y) : image(file) { w = image.S()->w; h = image.S()->h;
                                                              this->x = x; this->y = y; }
};
template<typename T, typename... R>
static void render(SDL_Surface* sdl, const T* i, R&&... r)
{
    render(sdl, i);
    render(sdl, std::forward<R>(r)...);
}
template<>
inline void render<Image>(SDL_Surface* sdl, const Image* i)
{
    SDL_Rect r{i->x, i->y, 0, 0};
    SDL_BlitSurface(i->image.S(), nullptr, sdl, &r);
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
    SDL_Surface* sdl = SDL_SetVideoMode(WinWidth, WinHeight, 32, 0);

    std::srand(std::time(nullptr));

    Image bgnd("assets/bg1.png", 0, 0);
    Image bgnd2("assets/bg1.png", WinWidth, 0);
    Bird bird(new Image("assets/bird2.png"));

    Pipe pipe(new Image("assets/pipeup.png"), new Image("assets/pipedown.png"));
    pipe.up->x = pipe.up->w; pipe.down->x = pipe.down->w;
    Pipe pipe2(new Image("assets/pipeup.png"), new Image("assets/pipedown.png"));
    pipe2.up->x = pipe2.up->w + WinWidth / 2; pipe2.down->x = pipe2.down->w + WinWidth / 2;
    Pipe pipe3(new Image("assets/pipeup.png"), new Image("assets/pipedown.png"));
    pipe3.up->x = pipe3.up->w + WinWidth; pipe3.down->x = pipe3.down->w + WinWidth;

    //collision checking
    auto coll = [](Bird bird, Pipe pipe)
    {
        return bird.img->x + bird.img->w > pipe.up->x && bird.img->x < pipe.up->x + pipe.up->w
            && (bird.img->y < pipe.down->y + pipe.down->h || bird.img->y + bird.img->h > pipe.up->y);
    };

    bool quit{}, lose{};
    unsigned frame{};
    while (!quit) {
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

        SDL_Flip(sdl);
        SDL_FillRect(sdl, nullptr, 0x0);
    }

    SDL_Quit();
}
