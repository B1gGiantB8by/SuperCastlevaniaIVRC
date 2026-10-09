param([switch]$Regenerate)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $toolchain = 'C:/Users/rijac/.local/share/retcomm/toolchains/cmake-clang-v1/latest'
    $python = 'C:/Users/rijac/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    $env:PATH = "$toolchain/bin;$env:PATH"
    $analyzerSource = Join-Path $PSScriptRoot '../tools/cli/snesrecomp-cli-windows-x86_64/_internal/recompiler-rs/target/release/snesrecomp-analyze.exe'
    $analyzerDir = Join-Path $PSScriptRoot 'snesrecomp/recompiler-rs/target/release'
    New-Item -ItemType Directory -Force -Path $analyzerDir | Out-Null
    Copy-Item -LiteralPath $analyzerSource -Destination (Join-Path $analyzerDir 'snesrecomp-analyze.exe') -Force
    & (Join-Path $PSScriptRoot 'tools/apply-project-patches.ps1')
    if ($Regenerate) {
        $identity = @{}
        Get-Content rom_identity.txt | ForEach-Object {
            if ($_ -match '^([^#=]+)=(.*)$') { $identity[$matches[1].Trim()] = $matches[2].Trim() }
        }
        & $python snesrecomp/snesrecomp_cli.py generate --rom '../Super Castlevania IV (USA).sfc' --project-root . --cfg-dir recomp --out-dir src/gen --funcs-h recomp/funcs.h --expected-crc32 $identity.expected_crc32 --expected-sha256 $identity.expected_sha256
        if ($LASTEXITCODE) { throw 'Code generation failed' }
    }
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "-DCMAKE_PREFIX_PATH=$toolchain/deps"
    if ($LASTEXITCODE) { throw 'CMake configuration failed' }
    cmake --build build --parallel 8
    if ($LASTEXITCODE) { throw 'Build failed' }
} finally { Pop-Location }

