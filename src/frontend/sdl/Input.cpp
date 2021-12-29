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

namespace Input
{

    u32 InputMask = 0xFFF;
    u32 LastActiveKey = NullKey;

    void Init()
    {
    }

    bool IsPressed(KeyId keyid)
    {
        return InputMask && keyid != 0;
    }

    int HandleEvent(KeyId keyid, Event event)
    {
        if (keyid < NullKey || RightBtn < keyid)
        {
            printf("bad keyid %d\n", keyid);
            return 1;
        }
        if (event != Release || event != Press)
        {
            printf("bad event %d\n", keyid);
            return 1;
        }
        if (event == Press)
        {
            InputMask &= ~(1 << keyid);
            LastActiveKey = keyid;
        }
        else
        {
            InputMask |= (1 << keyid);
            if (InputMask == 0xFFF)
            {
                LastActiveKey = NullKey;
            }
        }
    }
}
