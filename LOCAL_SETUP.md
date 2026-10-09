# Super Castlevania IV local recompilation

The project uses the US headerless 1 MiB ROM located in the parent directory.
CRC32: `b64ffb12`. SHA256:
`0ef6f4cce5a2273fa49fe1ce724e0048a8e39c91da6b00dbb693fe1ba909177d`.
The original ROM remains unchanged and outside the project repository.

## Build and launch

From PowerShell in this directory:

```powershell
.\build.ps1 -Regenerate
.\play.ps1
```

`build.ps1` uses the existing retcomm Clang/CMake/Ninja/SDL3 toolchain and the
Codex bundled Python. The native analyzer comes from the official snesrecomp
`v0.2.0-alpha` Windows release extracted into `../tools/cli/`.
The build helper copies that analyzer into the framework's default
`snesrecomp/recompiler-rs/target/release/` location, so the launcher's Generate
and rebuild operation also works without a special environment variable or Rust.
`tools/launcher-rebuild.patch` preserves inherited project permissions on
Windows staging directories, captures emitter errors in JSON progress logs,
and uses the exact CMake target name supplied by this project's CMake file.
This fixes regeneration across the sandbox and desktop user accounts and
avoids a lowercased executable path becoming an invalid Ninja target.
These paths are specific to this computer; edit the helper if relocating it.
The executable is `build/SuperCastlevaniaIvSNESRecomp.exe`.

Keyboard controls are configured in `build/keybinds.ini`: arrows move,
Enter is Start, Z is SNES B (jump), A is SNES Y (whip), X is SNES A,
S is SNES X. Compatible controllers are detected by SDL.
LT opens the rewind filmstrip (`[GamepadMap] Rewind = L2` in
`build/config.ini`). Use D-pad left/right to select a past frame, Xbox B
to resume there, or Xbox A to cancel. Controller bindings reload on restart.
`tools/rewind-hotkey.patch` clears the opening request before the modal loop,
preventing LT/F12 from reopening rewind immediately after confirm or cancel.
The rebuilt executable was checked with F12 and Enter: the menu closed and
the game resumed.

## Current implementation

Framework revision: `cf01c554f6745edcd197fd6c363ca350f6a40c77`.
The stock scaffold's beam frame driver is used. Generation discovers 53 exact
variants across banks 00 and 02, emitting 48 native variants and retaining
5 LLE variants. Reset and NMI remain explicitly interpreted entry points.
This is an initial hybrid static recompilation, with interpreter fallback;
it is not a complete analysis of every game routine or a validated full-game port.

`tools/interrupt-scheduling.patch` is a local framework fix applied by the
build helper. Interrupt-handler execution must yield to an asserted IRQ and
an armed frame deadline, just as whole-program execution does. Without this
change the boot test spins at `$00:8550`, hits the interpreter instruction
cap, and subsequently corrupts control flow. The patch remains uncommitted
in the framework submodule and is saved separately for reproducibility.

## Validation

Release build and regeneration succeeded. A headless 600-frame boot rendered
the Konami logo; a headless 1,800-frame run reached the title screen and exited
normally without interpreter-cap/BRK diagnostics. Captures and logs are in
`build/boot.png`, `build/title.png`, `build/smoke.log`, and `build/title-smoke.log`.
Headless checks exercise CPU, hardware, and framebuffer output, but do not
verify audible sound or hardware/reference accuracy. Full-game playthrough,
independent reference comparison, and broader native-code coverage remain.

`tools/gameplay-smoke.txt` supplies scripted Start, movement, and attack inputs.
A 3,300-frame input run passed the title and name-entry screens and reached
the new-game introduction, exiting normally without interpreter-cap or BRK
diagnostics (`build/gameplay-smoke.log`, `build/gameplay.png`). This does not
yet establish in-level movement, combat, or complete-game correctness.
Generated C and the ROM are excluded by the project's `.gitignore`.
