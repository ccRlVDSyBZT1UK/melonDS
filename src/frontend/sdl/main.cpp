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
#include "NDSCart_SRAMManager.h"

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
u32 audioFreq = 0;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *screen[2];
SDL_AudioDeviceID audioDevice;
SDL_Rect screenrect[2];

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
    window = SDL_CreateWindow("", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT * 2, 0);
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateSoftwareRenderer(surface);
    //SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    if (window == NULL | renderer == NULL)
    {
        return false;
    }
    SDL_JoystickEventState(SDL_ENABLE);

    screen[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    screen[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    screenrect[0] = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    screenrect[1] = {0, WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT};

#ifdef AUDIO_ENABLED
    audioFreq = 48000; // TODO: make configurable?
    SDL_AudioSpec want, get;
    memset(&want, 0, sizeof(SDL_AudioSpec));
    want.freq = audioFreq;
    want.format = AUDIO_S16LSB;
    want.channels = 2;
    want.samples = 1024;
    want.callback = NULL;
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &get, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!audioDevice)
    {
        printf("Audio init failed: %s\n", SDL_GetError());
    }
    else
    {
        audioFreq = get.freq;
        printf("Audio output frequency: %d Hz\n", audioFreq);
        SDL_PauseAudioDevice(audioDevice, 0);
    }
#endif

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
    case SDLK_ESCAPE:
        EmuRunning = 0;
        break;
    default:
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
        if (event->button.y >= WINDOW_HEIGHT)
        {
            HandleTouch(Input::StartTouch, event->button.x, event->button.y - WINDOW_HEIGHT);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.y >= WINDOW_HEIGHT)
        {
            HandleTouch(Input::EndTouch, event->button.x, event->button.y - WINDOW_HEIGHT);
        }
        break;
    case SDL_MOUSEMOTION:
        if (event->motion.state & SDL_BUTTON_LMASK && event->button.y >= WINDOW_HEIGHT)
        {
            HandleTouch(Input::DragTouch, event->motion.x, event->motion.y - WINDOW_HEIGHT);
        }
        break;
    case SDL_FINGERDOWN:
        if (event->tfinger.y >= 0.5)
        {
            HandleTouch(Input::StartTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(2.0 * (event->tfinger.y - 0.5) * float(WINDOW_HEIGHT)));
        }
        break;
    case SDL_FINGERUP:
        if (event->tfinger.y >= 0.5)
        {
            HandleTouch(Input::EndTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(2.0 * (event->tfinger.y - 0.5) * float(WINDOW_HEIGHT)));
        }
        break;
    case SDL_FINGERMOTION:
        if (event->tfinger.y >= 0.5)
        {
            HandleTouch(Input::DragTouch, int(event->tfinger.x * float(WINDOW_WIDTH)), int(2.0 * (event->tfinger.y - 0.5) * float(WINDOW_HEIGHT)));
        }
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

#define AUDIO_BUF_SIZE 4096
#define AUDIO_FREQ 32823.6328125
s16 buf_in[4096];
s16 buf_out[4096];

void ResampleAndQueueAudio()
{
    float scaled_freq = (float)audioFreq / AUDIO_FREQ;
    int sample_count = (int)(AUDIO_BUF_SIZE / 2.0f / scaled_freq);
    int samples_read = SPU::ReadOutput(buf_in, sample_count);
    int out_len = (int)((float)samples_read * scaled_freq);
    for (int i = 0; i < out_len; i++)
    {
        float input_pos = (float)i * AUDIO_FREQ / (float)audioFreq;
        int input_index = (int)input_pos;
        float coef = input_pos - (int)input_pos;
        int x = i * 2;
        int p0 = input_index * 2;
        int p1 = input_index * 2 + 2;
        if (input_index == samples_read - 1)
        {
            p1 = p0;
        }
        buf_out[x] = (s16)(coef * buf_in[p0] + (1.0 - coef) * buf_in[p1]);
        buf_out[x + 1] = (s16)(coef * buf_in[p0 + 1] + (1.0 - coef) * buf_in[p1 + 1]);
    }
    if (SDL_QueueAudio(audioDevice, buf_out, out_len * 2 * sizeof(s16)) < 0)
    {
        printf("queue audio failed: %s\n", SDL_GetError());
    }
}

void paint()
{
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
    if (SDL_RenderCopy(renderer, screen[0], NULL, &screenrect[0]) < 0)
    {
        printf("render copy failed for screen 0: %s\n", SDL_GetError());
    }
    SDL_Rect dstrect1 = {0, WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT};
    if (SDL_RenderCopy(renderer, screen[1], NULL, &screenrect[1]) < 0)
    {
        printf("render copy failed for screen 1: %s\n", SDL_GetError());
    }
    SDL_UpdateWindowSurface(window);
}

u64 totalframes;
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

    paint();
#ifdef AUDIO_ENABLED
    ResampleAndQueueAudio();
#endif
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
    totalframes++;
    nframes++;
    if (nframes >= 30)
    {
        double time = SDL_GetPerformanceCounter() * perfCountsSec;
        double dt = time - lastMeasureTime;
        lastMeasureTime = time;

        u32 fps = round(nframes / dt);
        nframes = 0;

        float fpstarget = 1.0 / frametimeStep;

        printf("%s [%d/%.0f] melonDS\n", MELONDS_VERSION, fps, fpstarget);
    }
    if (NDSCart_SRAMManager::NeedsFlush())
    {
        NDSCart_SRAMManager::FlushSecondaryBuffer(NULL, 0);
#ifdef EMSCRIPTEN
        EM_ASM(
            FS.syncfs(function(err)
                      {
                          if (err != null)
                          {
                              console.log("syncfs failed");
                          }
                          else
                          {
                              console.log("synced idbfs after SRAM flush")
                          }
                      }););
#endif
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
