## CHIP-8 Emulator

### General

This repository contains my attempt at an CHIP-8 emulator. This is the first emulator I have built and the same project that I implemented using the C Language.
The emulator is fully functional and supports enabling/disabling quirks in chip-8 emulators to be able to run games that rely on them.

#### Available flags:
- \-\-disable-vf-reset  Setting this flag leaves the vF register as undefined after and/or/xor operations.

- \-\-disable-memory-index-increment  Setting this flag prevents the save/load op-codes from incrementing the index register.

- \-\-disable-disable-display-wait  Setting this flag allows unlimited sprite draws per second, otherwise this is limited to 60 halting the execution, while waiting for the next sprite draw.

- \-\-disable-clipping  Disables dropping sprite pixels that go beyond the screen border. They will then be wrapped to the other side of the screen, that is left/top.

- \-\-shifting-vx  The shifting op codes only utilize the register pointer to by X instead of setting X to the shifted Y register.

- \-\-jumping-use-vx  The jumping op codes jump to the given address + the value in the register pointed to by X instead of always using register 0. 

The default without any flags applied looks like this, when using Timendus quirks test [link](https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file).

### Build and Run

- To build the project, `cmake` is required.
- First clone the repository with its submodules (SDL) to prepare for building: `git clone git@github.com:boenders/c-chip-8.git --recurse-submodules --shallow-submodules`
- Then run the script `build.sh`, which sets up cmake for the project and then builds a binary. The final binary will be located at: `build/c-chip-8`

- Instead of using the build script it is also possible to manually setup cmake and build the project.


