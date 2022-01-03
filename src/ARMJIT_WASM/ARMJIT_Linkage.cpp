
#include "ARMJIT_Compiler.h"

void ARM_Dispatch(ARM *cpu, ARMJIT::JitBlockEntry entry)
{
    //printf("total calls: %d\n", ARMJIT::total_calls);
    //printf("start PC=%d\n", cpu->CurInstr);
    ((compiled_block)entry)();
    //printf("end PC=%d\n", cpu->CurInstr);
    return;
}