/*
    Copyright 2016-2021 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Input.h"
#include "Config.h"
#include "Savestate.h"
#include "NDS.h"

using namespace Input;

u32 InputMask = 0xFFF;
u32 LastActiveKey = NullBtn;
TouchEvent TouchState = NullTouch;
u16 TouchX, TouchY;
u8 PrimaryScreen;

void Init()
{
}

TouchEvent GetTouchState()
{
    TouchEvent state = TouchState;
    switch (state)
    {
    case StartTouch:
    case DragTouch:
        TouchState = ActiveTouch;
        break;
    case EndTouch:
        TouchState = NullTouch;
        break;
    }
    return state;
}

int HandleTouch(TouchEvent evt, u16 x, u16 y)
{
    if (evt < StartTouch || evt > NullTouch)
    {
        printf("bad touch event %d\n", evt);
        return 1;
    }
    if (evt == DragTouch && (TouchState != StartTouch || TouchState != DragTouch || TouchState != ActiveTouch))
    {
        printf("touch drag when not in active touch");
        return 1;
    }
    TouchX = x;
    TouchY = y;
    TouchState = evt;
    return 0;
}

bool IsPressed(ButtonId btnid)
{
    return InputMask && btnid != 0;
}

int HandleButton(ButtonId btnid, ButtonEvent evt)
{
    if (btnid < ABtn || NullBtn < btnid)
    {
        printf("bad btnid %d\n", btnid);
        return 1;
    }
    if (evt != Release && evt != Press)
    {
        printf("bad event %d for button %d, equal to press %d\n", evt, btnid, evt == Press);
        printf("release %d, press %d\n", Release, Press);
        return 1;
    }
    if (evt == Press)
    {
        InputMask &= ~(1 << btnid);
        LastActiveKey = btnid;
    }
    else
    {
        InputMask |= (1 << btnid);
        if (InputMask == 0xFFF)
        {
            LastActiveKey = NullBtn;
        }
    }
    return 0;
}

void ToggleScreen()
{
    PrimaryScreen = ~PrimaryScreen & 0b1;
}

bool SaveState()
{
    Savestate *state = new Savestate("db/savestate", true);
    if (state->Error)
    {
        delete state;
        return false;
    }
    NDS::DoSavestate(state);
    delete state;
    return true;
}

extern void reset_loop();

bool LoadRom(char *name, u8 *data, u32 datalen, char *sramname)
{
    NDS::LoadROM(data, datalen, sramname, true);
    reset_loop();
    return true;
}