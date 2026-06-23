#ifndef DOLRECOMP_CPU_H
#define DOLRECOMP_CPU_H

#include "types.h"

// Minimal CPU support ABI for generated code and CPU tests.

#define GC_MAIN_RAM_SIZE    (24 * 1024 * 1024)
#define GC_RAM_BASE         0x80000000u
#define GC_RAM_UNCACHED     0xC0000000u

#define PPC_EXC_PROGRAM     0x00000001u
#define PPC_PROGRAM_TRAP    0x00000001u
#define PPC_PROGRAM_PRIV    0x00000002u

typedef struct {
    u32 gpr[32];
    f64 fpr[32];
    f64 ps1[32];
    u32 pc;
    u32 lr;
    u32 ctr;
    u32 cr;
    u32 xer;
    u32 fpscr;
    u32 msr;
    u32 sr[16];
    u32 exception;
    u32 program_exception;
    u32 reserve_addr;
    bool reserve_valid;

    u8* ram;
    u32 ram_size;
} CPUState;

bool cpu_init(CPUState* cpu);
void cpu_free(CPUState* cpu);
void cpu_reset(CPUState* cpu);

u64  mem_read64(CPUState* cpu, u32 addr);
void mem_write64(CPUState* cpu, u32 addr, u64 value);
u32  mem_read32(CPUState* cpu, u32 addr);
void mem_write32(CPUState* cpu, u32 addr, u32 value);
u16  mem_read16(CPUState* cpu, u32 addr);
void mem_write16(CPUState* cpu, u32 addr, u16 value);
u8   mem_read8(CPUState* cpu, u32 addr);
void mem_write8(CPUState* cpu, u32 addr, u8 value);

f64 ppc_approx_reciprocal(f64 value);
f64 ppc_approx_rsqrt(f64 value);
bool ppc_fres(CPUState* cpu, f64 value, f64* result);
bool ppc_frsqrte(CPUState* cpu, f64 value, f64* result);
void ppc_ps_res(CPUState* cpu, f64 a, f64 b, f64* result_a, f64* result_b);
void ppc_ps_rsqrte(CPUState* cpu, f64 a, f64 b, f64* result_a, f64* result_b);
bool ppc_fma(CPUState* cpu, f64 a, f64 c, f64 b, bool single,
             bool subtract, bool negative, f64* output);
bool ppc_fctiw(CPUState* cpu, f64 value, bool toward_zero, u64* result);
bool ppc_add_overflowed(u32 a, u32 b, u32 result);
bool ppc_trap_condition(u8 to, u32 a, u32 b);
void ppc_set_xer_ov(CPUState* cpu, bool ov);
void ppc_fpscr_updated(CPUState* cpu);
void ppc_memory_fence(void);

#endif /* DOLRECOMP_CPU_H */
