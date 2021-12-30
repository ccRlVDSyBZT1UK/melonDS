#ifndef PLATFORMINPUT_H
#define PLATFORMINPUT_H

#include "types.h"
namespace Input
{
    enum ButtonId
    {
        ABtn,
        BBtn,
        SelectBtn,
        StartBtn,
        RightDir,
        LeftDir,
        UpDir,
        DownDir,
        RightBtn,
        LeftBtn,
        XBtn,
        YBtn,
        PowerBtn,
        NullBtn,
    };

    enum ButtonEvent
    {
        Release,
        Press,
    };

    enum TouchEvent
    {
        StartTouch,
        DragTouch,
        EndTouch,
        ActiveTouch,
        NullTouch,
    };

}

extern "C"
{

    int HandleButton(Input::ButtonId keyid, Input::ButtonEvent event);
    int HandleTouch(Input::TouchEvent evt, u16 x, u16 y);
    Input::TouchEvent GetTouchState();
    void ToggleScreen();
    bool SaveState();
    bool LoadRom(char *name, u8 *data, u32 datalen, char *sramname);
}
extern u32 InputMask;
extern Input::TouchEvent TouchState;
extern u16 TouchX, TouchY;
extern u8 PrimaryScreen;

#endif