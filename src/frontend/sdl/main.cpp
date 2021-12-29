#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>
#include <algorithm>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include <SDL2/SDL.h>

#include "NDS.h"
#include "NDSCart.h"
#include "GBACart.h"
#include "GPU.h"
#include "SPU.h"
#include "Wifi.h"
#include "Input.h"

#include "Savestate.h"
#define MELONDS_VERSION "0.9.3"
#define MELONDS_URL "http://melonds.kuribo64.net/"

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 192

enum
{
    ROMSlot_NDS = 0,
    ROMSlot_GBA,

    ROMSlot_MAX
};

const unsigned int size = 64;
int EmuRunning = 0;
int autoScreenSizing = 0;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *screen[2];

char ROMPath[ROMSlot_MAX][1024];
char SRAMPath[ROMSlot_MAX][1024];
char PrevSRAMPath[ROMSlot_MAX][1024]; // for savestate 'undo load'
bool SavestateLoaded;
int videoRenderer;
GPU::RenderSettings videoSettings;
bool videoSettingsDirty;

bool init()
{
    // http://stackoverflow.com/questions/14543333/joystick-wont-work-using-sdl
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_HAPTIC) < 0)
    {
        printf("SDL couldn't init rumble\n");
    }
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
        printf("SDL couldn't init joystick\n");
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("SDL shat itself :(\n");
        return false;
    }

    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    if (window == NULL | renderer == NULL)
    {
        return false;
    }
    SDL_JoystickEventState(SDL_ENABLE);

    screen[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);
    screen[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);

    SavestateLoaded = false;
    memset(ROMPath[ROMSlot_NDS], 0, 1024);
    memset(ROMPath[ROMSlot_GBA], 0, 1024);
    memset(SRAMPath[ROMSlot_NDS], 0, 1024);
    memset(SRAMPath[ROMSlot_GBA], 0, 1024);
    memset(PrevSRAMPath[ROMSlot_NDS], 0, 1024);
    memset(PrevSRAMPath[ROMSlot_GBA], 0, 1024);
    videoSettingsDirty = false;
    videoSettings.Soft_Threaded = false;
    videoSettings.GL_ScaleFactor = 1;
    videoSettings.GL_BetterPolygons = true;
    videoRenderer = 0;
    return true;
}

void process_event(SDL_Event *event)
{
    SDL_Keycode key = event->key.keysym.sym;
    if (key == SDLK_ESCAPE)
    {
        EmuRunning = 0;
    }
    Event evt;
    if (event->key.type == SDL_KEYDOWN)
    {
        evt = Press;
    }
    else
    {
        evt = Release;
    }
    switch (key)
    {
    case SDLK_UP:
        HandleEvent(UpDir, evt);
    case SDLK_DOWN:
        HandleEvent(DownDir, evt);
    case SDLK_LEFT:
        HandleEvent(LeftDir, evt);
    case SDLK_RIGHT:
        HandleEvent(RightDir, evt);
    case SDLK_a:
        HandleEvent(ABtn, evt);
    case SDLK_b:
        HandleEvent(BBtn, evt);
    case SDLK_x:
        HandleEvent(XBtn, evt);
    case SDLK_y:
        HandleEvent(YBtn, evt);
    case SDLK_COMMA:
        HandleEvent(LeftBtn, evt);
    case SDLK_SEMICOLON:
        HandleEvent(StartBtn, evt);
    case SDLK_QUOTE:
        HandleEvent(SelectBtn, evt);
    case SDLK_PERIOD:
        HandleEvent(RightBtn, evt);
    case SDLK_q:
        HandleEvent(PowerBtn, evt);
    }
}

void process_input()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        process_event(&event);
    }
}

void update()
{
}

void draw()
{
}

u32 nframes;
double perfCountsSec;
double lastTime;
double frameLimitError;
double lastMeasureTime;

bool init_loop(char *rompath, char *srampath)
{
    NDS::Init();
    GPU::InitRenderer(videoRenderer);
    GPU::SetRenderSettings(videoRenderer, videoSettings);
    SPU::SetInterpolation(0);

    if (!NDS::LoadROM(rompath, srampath, true))
    {
        printf("failed to load the rom %s\n", rompath);
        return false;
    }

    nframes = 0;
    perfCountsSec = 1.0 / SDL_GetPerformanceFrequency();
    lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
    frameLimitError = 0.0;
    lastMeasureTime = lastTime;
    EmuRunning = 1;
    printf("set to emurunning=1\n");
    return true;
}

void main_loop()
{
    process_input();
    NDS::SetKeyMask(Input::InputMask);

    // emulate
    u32 nlines = NDS::RunFrame();

    //FrontBufferLock.lock();
    int frontBufferIdx = GPU::FrontBuffer;
    //FrontBufferLock.unlock();

    if (SDL_UpdateTexture(screen[0], NULL, GPU::Framebuffer[frontBufferIdx][0], 1024) < 0)
    {
        printf("update texture failed: %s\n", SDL_GetError());
    }
    if (SDL_UpdateTexture(screen[1], NULL, GPU::Framebuffer[frontBufferIdx][1], 1024) < 0)
    {
        printf("update texture failed: %s\n", SDL_GetError());
    }
    SDL_SetRenderDrawColor(renderer, 0xA6, 0xA6, 0xA6, 0xFF);
    SDL_RenderClear(renderer);
    if (SDL_RenderCopy(renderer, screen[1], NULL, NULL) < 0)
    {
        printf("render copy failed: %s\n", SDL_GetError());
    }
    SDL_RenderPresent(renderer);

    if (EmuRunning == 0)
        return;
    double frametimeStep = nlines / (60.0 * 263.0);
    {
        bool limitfps = true;

        double practicalFramelimit = limitfps ? frametimeStep : 1.0 / 1000.0;

        double curtime = SDL_GetPerformanceCounter() * perfCountsSec;

        frameLimitError += practicalFramelimit - (curtime - lastTime);
        if (frameLimitError < -practicalFramelimit)
            frameLimitError = -practicalFramelimit;
        if (frameLimitError > practicalFramelimit)
            frameLimitError = practicalFramelimit;

        if (round(frameLimitError * 1000.0) > 0.0)
        {
            SDL_Delay(round(frameLimitError * 1000.0));
            double timeBeforeSleep = curtime;
            curtime = SDL_GetPerformanceCounter() * perfCountsSec;
            frameLimitError -= curtime - timeBeforeSleep;
        }

        lastTime = curtime;
    }
    nframes++;
    if (nframes >= 30)
    {
        double time = SDL_GetPerformanceCounter() * perfCountsSec;
        double dt = time - lastMeasureTime;
        lastMeasureTime = time;

        u32 fps = round(nframes / dt);
        nframes = 0;

        float fpstarget = 1.0 / frametimeStep;

        printf("[%d/%.0f] melonDS\n" MELONDS_VERSION, fps, fpstarget);
    }
}

void destroy()
{
    GPU::DeInitRenderer();
    NDS::DeInit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void main_loop_simple()
{
    EmuRunning = 1;
    while (EmuRunning != 0)
    {
        process_input();
        SDL_SetRenderDrawColor(renderer, 0xA6, 0xA6, 0xA6, 0xFF);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    printf("melonDS " MELONDS_VERSION "\n");
    printf(MELONDS_URL "\n");

    init();
    init_loop("assetdir/platinum.nds", "assetdir/platinum.nds.sram");
    printf("calling into main loop\n");
#ifdef EMSCRIPTEN
    emscripten_set_main_loop(main_loop, -1, 1);
#else
    while (EmuRunning)
    {
        main_loop();
    }
#endif
    destroy();
    return EXIT_SUCCESS;
}
