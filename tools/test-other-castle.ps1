param([switch]$Smoke)
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $compiler = 'C:/Users/rijac/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/clang.exe'
    & $compiler -std=c11 -Isrc -Isnesrecomp/runner/src tools/other-castle-test.c src/other_castle.c snesrecomp/runner/src/sha256.c -o build/other-castle-test.exe
    if ($LASTEXITCODE) { throw 'Other Castle test compilation failed' }
    $hack = (Resolve-Path '../SuperCastlevaniaIVOtherCastle.sfc').Path
    $stock = (Resolve-Path '../Super Castlevania IV (USA).sfc').Path
    $test = Join-Path $PWD 'build/other-castle-test.exe'
    New-Item -ItemType Directory -Force build/other-castle-unit | Out-Null
    Push-Location build/other-castle-unit
    try { & $test $hack $stock } finally { Pop-Location }
    if ($LASTEXITCODE) { throw 'Other Castle regression failed' }
    if ($Smoke) {
        $previousDump = $env:SNESRECOMP_DUMP_DIR
        $romCache = Join-Path $PWD 'build/rom.cfg'
        $cacheBytes = if (Test-Path -LiteralPath $romCache) { [IO.File]::ReadAllBytes($romCache) } else { $null }
        try {
            $directory = Join-Path $PWD 'build/other-castle-smoke'
            New-Item -ItemType Directory -Force $directory | Out-Null
            $inputFile = Join-Path $directory 'input.txt'
            @('turbo on', 'wait 1800', 'press start 2', 'wait 180',
              'press start 2', 'wait 180', 'press start 2', 'wait 1200',
              'dump gameplay', 'press right 120', 'press y 30', 'wait 120',
              'dump moved', 'quit') | Set-Content $inputFile
            $env:SNESRECOMP_DUMP_DIR = $directory
            # Omit --other-castle to exercise automatic ROM identity detection.
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --script $inputFile --rom $hack *> (Join-Path $directory 'run.log')
            if ($LASTEXITCODE) { throw 'Other Castle smoke run failed' }
            $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'gameplay.wram.bin'))
            if ($ram[0x70] -ne 5) { throw 'Other Castle did not enter gameplay' }
            $before = (Get-FileHash (Join-Path $directory 'gameplay.fb.bgrx')).Hash
            $after = (Get-FileHash (Join-Path $directory 'moved.fb.bgrx')).Hash
            if ($before -eq $after) { throw 'Other Castle frame did not change after gameplay inputs' }
            Write-Output 'Other Castle reached level 1 and rendered movement/attack input.'
        } finally {
            $env:SNESRECOMP_DUMP_DIR = $previousDump
            if ($null -ne $cacheBytes) { [IO.File]::WriteAllBytes($romCache, $cacheBytes) }
        }
    }
} finally { Pop-Location }
