param([switch]$Preview)
$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $compiler='C:/Users/rijac/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/clang.exe'
    & $compiler -std=c11 -Wall -Wextra -Isrc -Isnesrecomp/runner/src tools/achievement-test.c src/achievements.c -o build/achievement-test.exe
    if($LASTEXITCODE) { throw 'Achievement test compilation failed' }
    $binary=Join-Path $PWD 'build/achievement-test.exe'
    $folder=Join-Path $PWD 'build/achievement-unit'
    New-Item -ItemType Directory -Force $folder | Out-Null
    Push-Location $folder
    try {
        & $binary
        if($LASTEXITCODE) { throw 'Achievement regression failed' }
        & $binary reload
        if($LASTEXITCODE) { throw 'Persistence reload failed' }
    } finally { Pop-Location }
    $failureFolder=Join-Path $PWD 'build/achievement-save-failure'
    New-Item -ItemType Directory -Force $failureFolder | Out-Null
    Push-Location $failureFolder
    try {
        'blocked storage' | Set-Content saves
        & $binary failure
        if($LASTEXITCODE) { throw 'Save failure regression failed' }
    } finally { Pop-Location }
    if($Preview) {
        $toolchain=Split-Path (Split-Path $compiler)
        & $compiler -std=c11 -Isrc -Isnesrecomp/runner/src -Irecomp-ui/src -Irecomp-ui/src/common "-I$toolchain/deps/include" -DLNG_SDL3=1 -DSDL_MAIN_HANDLED=1 tools/achievement-overlay-test.c src/achievements.c src/achievement_overlay.c recomp-ui/src/common/launcher_gl.c snesrecomp/runner/src/snes_overlay_draw.c -lopengl32 -o build/achievement-overlay-test.exe
        if($LASTEXITCODE) { throw 'Overlay test compilation failed' }
        New-Item -ItemType Directory -Force build/achievement-preview/assets | Out-Null
        Copy-Item -LiteralPath assets/castlevania -Destination build/achievement-preview/assets -Recurse -Force
        Copy-Item -LiteralPath recomp-ui/assets/common/fonts -Destination build/achievement-preview/assets -Recurse -Force
        Push-Location build/achievement-preview
        try {
            & ../achievement-overlay-test.exe
            if($LASTEXITCODE) { throw 'Overlay preview failed' }
        } finally { Pop-Location }
        Write-Output 'Card preview: build/achievement-preview/achievement-preview.ppm'
    }
} finally { Pop-Location }
