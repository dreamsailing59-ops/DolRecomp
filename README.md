# DolRecomp

DolRecomp is is a tool to statically recompile GameCube DOL executables into C code that can be compiled for any platform. It functions similarly to [N64Recomp](https://github.com/N64Recomp/N64Recomp) (Which targets N64 binaries), and is a modern successor to [GCRecompiler](https://github.com/ExpansionPak/GCRecompiler). It is also a rewrite of "DolRecomp", Which was the original successor to GCRecompiler was hardcoded for [**Paper Mario: The Thousand-Year Door**](https://en.wikipedia.org/wiki/Paper_Mario:_The_Thousand-Year_Door) only. This newer rewrite aims to work with any GameCube DOL executable.

## structure

```
src/
  frontend/     - DOL/REL loading, PPC disassembly, control flow analysis
  backend/      - code generation, target arch emission (x86_64, arm64, etc)
  runtime/      - the glue that makes recompiled code actually work (HW regs, memory, OS)
  core/         - shared types, config, logging, the stuff everything else depends on

tools/          - standalone utilities
docs/           - notes, specs, rambling design docs
scripts/        - build helpers, CI stuff, whatever
include/        - public headers if we end up needing them
```

## Opcodes

The current CPU opcode set has 90 implemented opcodes.

| Area | Opcodes |
|------|---------|
| Immediate arithmetic | addi, addic, addic., addis, mulli, subfic |
| Register arithmetic | add, addc, adde, addze, neg, subf, subfc, subfe, subfze |
| Compare / branch / CR | b[l][a], bc[l][a], bclr/blr, bcctr/bctr, cmp/cmpw, cmpi/cmpwi, cmpl/cmplw, cmpli/cmplwi, crand, crandc, creqv, crnand, crnor, cror, crorc, crxor, mcrf, mfcr, mtcrf |
| Logical / rotate / shift | and, andc, andi., andis., cntlzw, eqv, extsb, extsh, nand, nor, or, orc, ori, oris, rlwimi, rlwinm, rlwnm, slw, sraw, srawi, srw, xor, xori, xoris |
| Loads | lbz, lbzu, lbzx, lbzux, lha, lhau, lhax, lhaux, lhz, lhzu, lhzx, lhzux, lmw, lwz, lwzu, lwzx, lwzux |
| Stores | stb, stbu, stbux, stbx, sth, sthu, sthux, sthx, stmw, stw, stwu, stwux, stwx |
| SPR moves | mfspr/mflr/mfctr/mfxer, mtspr/mtlr/mtctr/mtxer |

## building

DolRecomp is pure C++ with no external submodules, so if you have VSCode you can simply build the project by pressing `Ctrl + Shift + P` (`Command + Shift + P` on macOS) and selecting `CMake: Build`.

On windows. If you prefer using the command line, you can use the following commands in this exact order

```
mkdir build
cd build
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

## contributing

no
