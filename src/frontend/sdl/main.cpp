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
    if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
    {
        printf("set nearest neighbor rendering hint\n");
    }
    else
    {
        printf("failed to set nearest neighbor rendering\n");
    }

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
    window = SDL_CreateWindow("", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateSoftwareRenderer(surface);
    //SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
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
    videoSettings.GL_ScaleFactor = 1;
    videoSettings.GL_BetterPolygons = true;
    videoRenderer = 0;

    NDS::Init();
    GPU::InitRenderer(videoRenderer);
    GPU::SetRenderSettings(videoRenderer, videoSettings);
    SPU::SetInterpolation(0);

    return true;
}

void process_keyboard_event(SDL_Event *event)
{
    SDL_Keycode key = event->key.keysym.sym;
    Input::ButtonEvent evt = event->key.type == SDL_KEYDOWN ? Input::Press : Input::Release;
    switch (key)
    {
    case SDLK_UP:
        HandleButton(Input::UpDir, evt);
        break;
    case SDLK_DOWN:
        HandleButton(Input::DownDir, evt);
        break;
    case SDLK_LEFT:
        HandleButton(Input::LeftDir, evt);
        break;
    case SDLK_RIGHT:
        HandleButton(Input::RightDir, evt);
        break;
    case SDLK_a:
        HandleButton(Input::ABtn, evt);
        break;
    case SDLK_b:
        HandleButton(Input::BBtn, evt);
        break;
    case SDLK_x:
        HandleButton(Input::XBtn, evt);
        break;
    case SDLK_y:
        HandleButton(Input::YBtn, evt);
        break;
    case SDLK_COMMA:
        HandleButton(Input::LeftBtn, evt);
        break;
    case SDLK_SEMICOLON:
        HandleButton(Input::StartBtn, evt);
        break;
    case SDLK_QUOTE:
        HandleButton(Input::SelectBtn, evt);
        break;
    case SDLK_PERIOD:
        HandleButton(Input::RightBtn, evt);
        break;
    case SDLK_q:
        HandleButton(Input::PowerBtn, evt);
        break;
    case SDLK_s:
        if (event->key.type == SDL_KEYDOWN)
        {
            ToggleScreen();
        }
        break;
    case SDLK_ESCAPE:
        EmuRunning = 0;
        break;
    }
}

void process_event(SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_QUIT:
        EmuRunning = 0;
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        process_keyboard_event(event);
        break;
    case SDL_MOUSEBUTTONDOWN:
        HandleTouch(Input::StartTouch, event->button.x, event->button.y);
        break;
    case SDL_MOUSEBUTTONUP:
        HandleTouch(Input::EndTouch, event->button.x, event->button.y);
        break;
    case SDL_MOUSEMOTION:
        if (event->motion.state & SDL_BUTTON_LMASK)
        {
            HandleTouch(Input::DragTouch, event->motion.x, event->motion.y);
        }
        break;
    case SDL_FINGERDOWN:
        HandleTouch(Input::StartTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(event->tfinger.y * float(WINDOW_HEIGHT)));
        break;
    case SDL_FINGERUP:
        HandleTouch(Input::EndTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(event->tfinger.y * float(WINDOW_HEIGHT)));
        break;
    case SDL_FINGERMOTION:
        HandleTouch(Input::DragTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(event->tfinger.y * float(WINDOW_HEIGHT)));
        break;
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

void reset_loop()
{
    nframes = 0;
    perfCountsSec = 1.0 / SDL_GetPerformanceFrequency();
    lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
    frameLimitError = 0.0;
    lastMeasureTime = lastTime;
    printf("setting EmuRunning=1\n");
    EmuRunning = 1;
}

void main_loop()
{
    if (EmuRunning == 0)
        return;

    process_input();
    NDS::SetKeyMask(InputMask);
    switch (GetTouchState())
    {
    case Input::StartTouch:
    case Input::DragTouch:
        NDS::TouchScreen(TouchX, TouchY);
        break;
    case Input::EndTouch:
        NDS::ReleaseScreen();
        break;
    }

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
    if (SDL_RenderCopy(renderer, screen[PrimaryScreen], NULL, NULL) < 0)
    {
        printf("render copy failed: %s\n", SDL_GetError());
    }
    SDL_UpdateWindowSurface(window);

    double frametimeStep = nlines / (60.0 * 263.0);
    {
        bool limitfps = false;

        double practicalFramelimit = limitfps ? frametimeStep : 1.0 / 1000.0;

        double curtime = SDL_GetPerformanceCounter() * perfCountsSec;

        frameLimitError += practicalFramelimit - (curtime - lastTime);
        if (frameLimitError < -practicalFramelimit)
            frameLimitError = -practicalFramelimit;
        if (frameLimitError > practicalFramelimit)
            frameLimitError = practicalFramelimit;

        if (round(frameLimitError * 1000.0) > 0.0)
        {
            //SDL_Delay(round(frameLimitError * 1000.0));
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
