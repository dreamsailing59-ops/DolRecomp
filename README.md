# DolRecomp

the best gamecube recompiler ever made

(rewrite of original DolRecomp and 3rd rewrite overall)

## what is this

a static recompiler for GameCube DOL executables. takes PowerPC machine code and spits out native code you can run on modern hardware.

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

## building

TODO — the entire project

## contributing

no