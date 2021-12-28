#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>
#include <algorithm>

#include <SDL2/SDL.h>

#include "NDS.h"
#include "NDSCart.h"
#include "GBACart.h"
#include "GPU.h"
#include "SPU.h"
#include "Wifi.h"

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

void main_loop(char *rompath, char *srampath)
{
    NDS::Init();
    u32 mainScreenPos[3];
    mainScreenPos[0] = 0;
    mainScreenPos[1] = 0;
    mainScreenPos[2] = 0;
    autoScreenSizing = 0;
    GPU::InitRenderer(videoRenderer);
    GPU::SetRenderSettings(videoRenderer, videoSettings);
    SPU::SetInterpolation(0);

    if (!NDS::LoadROM(rompath, srampath, true))
    {
        printf("failed to load the rom %s\n", rompath);
        return;
    }

    u32 nframes = 0;
    double perfCountsSec = 1.0 / SDL_GetPerformanceFrequency();
    double lastTime = SDL_GetPerformanceCounter() * perfCountsSec;
    double frameLimitError = 0.0;
    double lastMeasureTime = lastTime;

    char melontitle[100];
    EmuRunning = 1;
    printf("set to emurunning=1\n");

    while (EmuRunning != 0)
    {
        process_input();

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
            break;
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
    GPU::DeInitRenderer();
    NDS::DeInit();
}

void destroy()
{
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
    main_loop(argv[1], argv[2]);

    destroy();
    return EXIT_SUCCESS;
}
