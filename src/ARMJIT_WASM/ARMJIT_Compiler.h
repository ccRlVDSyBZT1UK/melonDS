/*
    Copyright 2016-2021 Arisotura, RSDuck

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

#ifndef ARMJIT_WASM_COMPILER_H
#define ARMJIT_WASM_COMPILER_H

#include "../ARM.h"
#include "../ARMJIT.h"

#include "../ARMJIT_Internal.h"
#include "../ARMJIT_RegisterCache.h"

#include <unordered_map>

typedef void (*compiled_block)(void);
namespace ARMJIT
{

    extern int total_calls;

    class Compiler
    {
    public:
        typedef void (Compiler::*CompileFunc)();

        Compiler();
        ~Compiler(){};

        JitBlockEntry CompileBlock(ARM *cpu, bool thumb, FetchedInstr instrs[], int instrsCount, bool hasMemInstr);
        bool CanCompile(bool thumb, u16 kind);
        void Reset();
        u32 SubEntryOffset(JitBlockEntry entry);
        JitBlockEntry AddEntryOffset(u32 offset);
        bool IsJITFault(u8 *pc);
        u8 *RewriteMemAccess(u8 *pc);

    private:
        int base_function;
        int table_len;
        int table_capacity;
    };

}

extern "C"
{
    void call_inception();
}

#endif
