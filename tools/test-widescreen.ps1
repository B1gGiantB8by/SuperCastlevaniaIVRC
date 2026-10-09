param(
    [switch]$Smoke,
    [switch]$Scroll,
    [int[]]$Stages = (1..30),
    [string]$Toolchain = 'C:/Users/rijac/.local/share/retcomm/toolchains/cmake-clang-v1/latest',
    [string]$Python = 'C:/Users/rijac/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
)
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    New-Item -ItemType Directory -Force build/widescreen-tests | Out-Null
    & "$Toolchain/bin/clang.exe" -std=gnu11 -DSDL_MAIN_HANDLED -DSNESRECOMP_SDL3=1 "-I$Toolchain/deps/include" -Isrc -Isnesrecomp/runner/src -Isnesrecomp/runner/src/desktop tools/widescreen-settings-test.c src/bezel.c src/sciv_widescreen.c -o build/widescreen-tests/settings-test.exe
    if ($LASTEXITCODE) { throw 'Widescreen settings test compilation failed' }
    Push-Location build/widescreen-tests
    try {
        ./settings-test.exe
        if ($LASTEXITCODE) { throw 'Widescreen settings regression failed' }
    } finally { Pop-Location }
    if ($Smoke) {
        & $Python tools/test-widescreen.py @Stages
        if ($LASTEXITCODE) { throw 'Widescreen stage regression failed' }
    }
    if ($Scroll) {
        & "$Toolchain/bin/clang.exe" -std=gnu11 -Isnesrecomp/runner/src tools/rewind-hold-test.c snesrecomp/runner/src/snes_rewind.c snesrecomp/runner/src/snes_overlay_draw.c -o build/rewind-hold-test.exe
        if ($LASTEXITCODE) { throw 'Rewind regression compilation failed' }
        ./build/rewind-hold-test.exe
        if ($LASTEXITCODE) { throw 'Rewind regression failed' }
        & $Python tools/test-widescreen-scroll.py
        if ($LASTEXITCODE) { throw 'Widescreen scrolling/rewind regression failed' }
    }
} finally { Pop-Location }
