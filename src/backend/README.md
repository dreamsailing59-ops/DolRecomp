# backend

turns analyzed PPC into native code for the host machine.

## what goes here

- **IR (intermediate representation)** — our own simplified instruction set to bridge PPC → native
- **code generation** — lower the IR into actual x86_64 / arm64 / whatever
- **register allocation** — mapping PPC GPRs and FPRs to host registers
- **optimization** — constant folding, dead code removal, the usual compiler stuff (keep it simple though)

## targets

- x86_64 is the priority (most people are on that)
- arm64 would be cool for macOS and handheld PCs
- keep the arch-specific stuff isolated so adding targets isn't a nightmare