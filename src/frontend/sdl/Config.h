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

#ifndef PLATFORMCONFIG_H
#define PLATFORMCONFIG_H

enum
{
    HK_Lid = 0,
    HK_Mic,
    HK_Pause,
    HK_Reset,
    HK_FastForward,
    HK_FastForwardToggle,
    HK_FullscreenToggle,
    HK_SwapScreens,
    HK_SolarSensorDecrease,
    HK_SolarSensorIncrease,
    HK_FrameStep,
    HK_MAX
};

namespace Config
{

    struct ConfigEntry
    {
        char Name[32];
        int Type;
        void *Value;
        int DefaultInt;
        const char *DefaultStr;
        int StrLength; // should be set to actual array length minus one
    };

    extern int KeyMapping[12];
    extern int JoyMapping[12];

    extern int HKKeyMapping[HK_MAX];
    extern int HKJoyMapping[HK_MAX];

    extern int JoystickID;

    extern int WindowWidth;
    extern int WindowHeight;
    extern int WindowMaximized;

    extern int ScreenRotation;
    extern int ScreenGap;
    extern int ScreenLayout;
    extern int ScreenSwap;
    extern int ScreenSizing;
    extern int ScreenAspectTop;
    extern int ScreenAspectBot;
    extern int IntegerScaling;
    extern int ScreenFilter;

    extern int ScreenUseGL;
    extern int ScreenVSync;
    extern int ScreenVSyncInterval;

    extern int _3DRenderer;
    extern int Threaded3D;

    extern int GL_ScaleFactor;
    extern int GL_BetterPolygons;

    extern int LimitFPS;
    extern int AudioSync;
    extern int ShowOSD;

    extern int ConsoleType;
    extern int DirectBoot;

#ifdef JIT_ENABLED
    extern int JIT_Enable;
    extern int JIT_MaxBlockSize;
    extern int JIT_BranchOptimisations;
    extern int JIT_LiteralOptimisations;
    extern int JIT_FastMemory;
#endif

    extern int ExternalBIOSEnable;

    extern char BIOS9Path[1024];
    extern char BIOS7Path[1024];
    extern char FirmwarePath[1024];

    extern char DSiBIOS9Path[1024];
    extern char DSiBIOS7Path[1024];
    extern char DSiFirmwarePath[1024];
    extern char DSiNANDPath[1024];

    extern int DLDIEnable;
    extern char DLDISDPath[1024];
    extern int DLDISize;
    extern int DLDIReadOnly;
    extern int DLDIFolderSync;
    extern char DLDIFolderPath[1024];

    extern int DSiSDEnable;
    extern char DSiSDPath[1024];
    extern int DSiSDSize;
    extern int DSiSDReadOnly;
    extern int DSiSDFolderSync;
    extern char DSiSDFolderPath[1024];

    extern int FirmwareOverrideSettings;
    extern char FirmwareUsername[64];
    extern int FirmwareLanguage;
    extern int FirmwareBirthdayMonth;
    extern int FirmwareBirthdayDay;
    extern int FirmwareFavouriteColour;
    extern char FirmwareMessage[1024];
    extern char FirmwareMAC[18];
    extern int RandomizeMAC;

    extern int SocketBindAnyAddr;
    extern char LANDevice[128];
    extern int DirectLAN;

    extern int SavestateRelocSRAM;

    extern int AudioInterp;
    extern int AudioBitrate;
    extern int AudioVolume;
    extern int MicInputType;
    extern char MicWavPath[1024];

    extern char LastROMFolder[1024];

    extern char RecentROMList[10][1024];

    extern int EnableCheats;

    extern int MouseHide;
    extern int MouseHideSeconds;
    extern int PauseLostFocus;

    void Load();
    void Save();

}

#endif // PLATFORMCONFIG_H
