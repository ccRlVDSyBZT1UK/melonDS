#include "ARMJIT_Compiler.h"
#include "../wasmblr/wasmblr.h"
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

typedef void (*compiled_block)(void);

void call_inception()
{
    printf("invoked inception call\n");
}

void HandleARM9(int loops)
{
    //printf("start PC=%d\n", NDS::ARM9->CurInstr);
    ARMJIT::total_calls++;
    for (int i = 0; i < loops; i++)
    {
        //printf("loop %d PC=%d\n", i, NDS::ARM9->CurInstr);
        NDS::ARM9->handle();
    }
    //printf("end PC=%d\n", NDS::ARM9->CurInstr);
}

void HandleARM7(int loops)
{
    //printf("start PC=%d\n", NDS::ARM7->CurInstr);
    ARMJIT::total_calls++;
    for (int i = 0; i < loops; i++)
    {
        //printf("loop %d PC=%d\n", i, NDS::ARM7->CurInstr);
        NDS::ARM7->handle();
    }
    //printf("end PC=%d\n", NDS::ARM7->CurInstr);
}

EM_JS(
    compiled_block, compile_and_instantiate, (unsigned char *src, int srclen, int addr),
    {
        console.log("src=", src, "srclen=", srclen);
        const start = performance.now();
        const buf = HEAPU8.subarray(src, src + srclen);
        const m = new WebAssembly.Module(buf);
        const instance = new WebAssembly.Instance(m, {melonds : {__indirect_function_table : Module.asm.__indirect_function_table}, env : {memory : Module.asm.memory}});
        console.log("created instance, took", performance.now() - start, "ms");
        Module.asm.__indirect_function_table.set(addr, instance.exports.exec);
        return addr;
    });

EM_JS(int, grow_table, (int n),
      {
          console.log("table from Compiler()=", Module.asm.__indirect_function_table);
          const prelen = Module.asm.__indirect_function_table.length;
          Module.asm.__indirect_function_table.grow(n);
          return prelen;
      });

namespace ARMJIT
{
    int total_calls = 0;

    Compiler::Compiler()
    {
        table_capacity = 100;
        table_len = 0;
        base_function = grow_table(table_capacity);
        printf("initialized ARMJIT compiler with base_function=%d and table_size=%d\n", base_function, table_capacity);
        printf("table offset for call_inception=%d\n", (int)call_inception);
    }

    JitBlockEntry Compiler::CompileBlock(ARM *cpu, bool thumb, FetchedInstr instrs[], int instrsCount, bool hasMemInstr)
    {
        if (table_capacity == table_len)
        {
            printf("JIT near memory full, resetting...\n");
            ResetBlockCache();
            table_len = 0;
        }
        printf("compiling block with %d instructions\n", instrsCount);
        wasmblr::CodeGenerator cg;
        auto voidvoid_sig = cg.function(
            {cg.i32},
            {},
            [&]() {

            });

        auto exec_func = cg.function(
            {},
            {},
            [&]()
            {
                cg.i32.const_(instrsCount);
                cg.i32.const_((int)(cpu->Num ? HandleARM7 : HandleARM9));
                cg.call_indirect(0);
            });
        cg.export_(exec_func, "exec");
        cg.import_table("melonds", "__indirect_function_table", base_function + table_capacity, 0);
        //cg.import_mem("melonds", "mem", 1, 0);
        cg.memory(0);
        auto bytes = cg.emit();
        compiled_block f = compile_and_instantiate(bytes.data(), bytes.size(), base_function + (table_len++));
        printf("compiled new function f=%d\n", f);
        return f;
    }

    bool Compiler::CanCompile(bool thumb, u16 kind)
    {
        //printf("checking if compilable %d", kind);
        return false;
    }

    void Compiler::Reset()
    {
    }

    u32 Compiler::SubEntryOffset(JitBlockEntry entry)
    {
        return (int)entry - base_function;
    }

    JitBlockEntry Compiler::AddEntryOffset(u32 offset)
    {
        return (JitBlockEntry)(base_function + offset);
    }

    bool Compiler::IsJITFault(u8 *pc)
    {
        return false;
    }

    u8 *Compiler::RewriteMemAccess(u8 *pc)
    {
        return 0;
    }

};