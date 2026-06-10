# runtime

the support layer that recompiled code links against to actually work.

## what goes here

- **CPU state** — emulated PPC register file (GPRs, FPRs, SPRs, CR, etc)
- **memory** — address translation, the GC's memory map (24MB main, 16MB ARAM, hardware regs)
- **hardware stubs** — GX (GPU), AI (audio), DI (disc), SI (controllers), EXI, etc
- **OS/HLE** — high-level emulation of common SDK functions (OSAlloc, DVDRead, that kind of thing)
- **entry point** — the main() that boots the recompiled game

## notes

this is where most of the "make it actually run" work happens
