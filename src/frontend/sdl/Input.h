#ifndef PLATFORMINPUT_H
#define PLATFORMINPUT_H

#include "types.h"

namespace Input
{
    extern u32 InputMask;

    enum KeyId
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
        NullKey,
    };

    enum Event
    {
        Release,
        Press,
    };
}

#endif