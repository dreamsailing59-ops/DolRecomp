# DolRecomp

DolRecomp is a static recompiler for GameCube and Wii PowerPC code. It reads DOLs, decodes the CPU instructions, and writes split C output that can be built somewhere else. (here it is pc)

Right now the focus is the CPU side. Runtime work is going to be separate.

It is in the same general lane as [N64Recomp](https://github.com/N64Recomp/N64Recomp), and it replaces the older [GCRecompiler](https://github.com/ExpansionPak/GCRecompiler) style of one-game hardcoding with something that can handle normal GameCube/Wii executables. Wii U support has started too: RPX files can be opened, executable sections can be decoded, and the CPU profile is marked as Espresso.

## structure

```
src/
  frontend/     - DOL/RPX loading and PPC decode
  backend/      - C output
  core/         - CPU helpers used by generated code and tests

tools/          - standalone utilities
docs/           - notes and specs
scripts/        - build/test helpers
include/        - public headers if needed later
```

## Opcodes

The current CPU opcode set has 236 implemented opcodes.

| Area | Opcodes |
|------|---------|
| Immediate arithmetic | addi, addic, addic., addis, mulli, subfic |
| Register arithmetic | add, addo, addc, addco, adde, addeo, addme, addmeo, addze, addzeo, divw, divwo, divwu, divwuo, mulhw, mulhwu, mullw, mullwo, neg, nego, subf, subfo, subfc, subfco, subfe, subfeo, subfme, subfmeo, subfze, subfzeo |
| Compare / branch / CR | b[l][a], bc[l][a], bclr/blr, bcctr/bctr, cmp/cmpw, cmpi/cmpwi, cmpl/cmplw, cmpli/cmplwi, crand, crandc, creqv, crnand, crnor, cror, crorc, crxor, mcrf, mcrxr, mfcr, mtcrf, rfi, sc, tw, twi |
| Logical / rotate / shift | and, andc, andi., andis., cntlzw, eqv, extsb, extsh, nand, nor, or, orc, ori, oris, rlwimi, rlwinm, rlwnm, slw, sraw, srawi, srw, xor, xori, xoris |
| Loads | lbz, lbzu, lbzx, lbzux, lfd, lfdu, lfdux, lfdx, lfs, lfsu, lfsux, lfsx, lha, lhau, lhax, lhaux, lhbrx, lhz, lhzu, lhzx, lhzux, lmw, lswi, lswx, lwarx, lwbrx, lwz, lwzu, lwzx, lwzux |
| Stores | stb, stbu, stbux, stbx, stfd, stfdu, stfdux, stfdx, stfiwx, stfs, stfsu, stfsux, stfsx, sth, sthbrx, sthu, sthux, sthx, stmw, stswi, stswx, stw, stwbrx, stwcx., stwu, stwux, stwx |
| Floating point | fabs, fadd, fadds, fcmpo, fcmpu, fctiw, fctiwz, fdiv, fdivs, fmadd, fmadds, fmr, fmsub, fmsubs, fmul, fmuls, fneg, fnabs, fnmadd, fnmadds, fnmsub, fnmsubs, fres, frsp, frsqrte, fsel, fsub, fsubs |
| FPSCR control | mcrfs, mffs, mtfsb0, mtfsb1, mtfsf, mtfsfi |
| Paired-single memory | psq_l, psq_lu, psq_lux, psq_lx, psq_st, psq_stu, psq_stux, psq_stx |
| Paired-single arithmetic | ps_abs, ps_add, ps_cmpo0, ps_cmpo1, ps_cmpu0, ps_cmpu1, ps_div, ps_madd, ps_madds0, ps_madds1, ps_merge00, ps_merge01, ps_merge10, ps_merge11, ps_mr, ps_msub, ps_mul, ps_muls0, ps_muls1, ps_nabs, ps_neg, ps_nmadd, ps_nmsub, ps_res, ps_rsqrte, ps_sel, ps_sub, ps_sum0, ps_sum1 |
| Cache / memory control | dcbf, dcbi, dcbst, dcbt, dcbtst, dcbz, dcbz_l, eieio, icbi, isync, sync, tlbie, tlbsync |
| SPR / system moves | eciwx, ecowx, mfmsr, mfspr/mflr/mfctr/mfxer, mfsr, mfsrin, mftb/mftbu, mtmsr, mtspr/mtlr/mtctr/mtxer, mtsr, mtsrin |

## building

DolRecomp is C and builds with CMake. zlib is optional, but needed for compressed RPX sections. The CMake file will try devkitPro's MSYS2 zlib first when it is installed.

From the repo root:

```
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build -j14
```

## usage

For Wii title lookup, grab the local GameTDB/WiiTDB list once:

```
dolrecomp.exe --setup
```

Wii DOLs need the six-character title ID:

```
dolrecomp.exe -j14 path\to\main.dol SUKE01 build
```

GameCube DOLs do not use a title ID:

```
dolrecomp.exe --gamecube path\to\main.dol build
```

Wii U uses the Espresso CPU profile and takes an RPX:

```
dolrecomp.exe --cpu espresso path\to\main.rpx build
```

todo: add more documentation here as it grows

If the last argument is a directory, Wii output goes under `<title-id>_generated\`. GameCube and Wii U use `generated\`. If the last argument ends in `.c`, that exact C file is used. `-jN` controls how many worker jobs write split C chunks.

If `database\titles.txt` is missing during Wii mode, DolRecomp warns and falls back to GameCube mode.

## contributing

todo
