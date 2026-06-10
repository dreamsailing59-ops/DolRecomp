# frontend

everything that deals with reading and understanding GameCube binaries.

## what goes here

- **DOL/REL loader** — parse the executable format, pull out sections, entry points, etc
- **PPC disassembler** — turn raw bytes into PowerPC instructions we can reason about
- **control flow analysis** — figure out functions, basic blocks, branches, all that graph stuff
- **symbol handling** — load symbol maps, name resolution, whatever helps us understand what we're looking at

## rough plan

1. get DOL loading working first (it's simpler than REL)
2. basic PPC decode — doesn't need to be perfect, just enough to start
3. build the CFG
4. REL support later (relocatable modules are a whole thing)
