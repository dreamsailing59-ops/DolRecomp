# core

shared foundations that everything else depends on.

## what goes here

- **types** — common typedefs, enums, PPC instruction definitions
- **config** — project-wide settings, build options
- **logging** — debug output, error reporting
- **utilities** — endian helpers, bit manipulation, file I/O wrappers

keep this lean. if something is only used by one module, it belongs in that module.