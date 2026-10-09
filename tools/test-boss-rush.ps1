param([switch]$Smoke, [switch]$Damage, [switch]$Transitions, [switch]$Continue)
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $compiler = 'C:/Users/rijac/.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/clang.exe'
    & $compiler -std=c11 -Isrc -Isnesrecomp/runner/src tools/boss-rush-test.c src/boss_rush.c src/achievements.c -o build/boss-rush-test.exe
    if ($LASTEXITCODE) { throw 'Boss Rush test compilation failed' }
    $testBinary=Join-Path $PWD 'build/boss-rush-test.exe'
    $testFolder=Join-Path $PWD 'build/boss-rush-unit'
    New-Item -ItemType Directory -Force $testFolder | Out-Null
    Push-Location $testFolder
    try { & $testBinary } finally { Pop-Location }
    if ($LASTEXITCODE) { throw 'Boss Rush regression failed' }
    if ($Continue -or $Smoke) {
        $previousDump = $env:SNESRECOMP_DUMP_DIR
        try {
            $directory = Join-Path $PWD 'build/boss-rush-continue'
            New-Item -ItemType Directory -Force $directory | Out-Null
            $scriptPath = Join-Path $directory 'input.txt'
            @('turbo on', 'until16 1ffe0 == 4252 2500',
              'until16 0070 == 0005 2500', 'wait 240', 'poke 13f4 0000',
              'until16 0070 == 0007 2000', 'wait 180', 'dump menu',
              'press start 1', 'until16 1ffe4 == 0000 1000',
              'until16 0070 == 0005 2500', 'dump continued', 'quit') | Set-Content $scriptPath
            $env:SNESRECOMP_DUMP_DIR = $directory
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --boss-rush=7 --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
            if ($LASTEXITCODE) { throw 'Boss Rush Continue run failed' }
            $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'continued.wram.bin'))
            if ($ram[0x1ffe2] -ne 1 -or $ram[0x1ffe4] -ne 0 -or $ram[0x86] -ne 7 -or $ram[0x13f4] -ne 16 -or $ram[0x1ffe8] -ne 0 -or $ram[0x1ffe9] -ne 0) {
                throw 'Continue did not start a fresh Rowdain encounter'
            }
            Write-Output 'Guest Game Over -> Continue restarts Boss Rush at Rowdain with a reset timer.'
        } finally { $env:SNESRECOMP_DUMP_DIR = $previousDump }
    }
    if ($Damage -or $Smoke) {
        $previousDump = $env:SNESRECOMP_DUMP_DIR
        try {
            $directory = Join-Path $PWD 'build/boss-rush-grakul-damage'
            New-Item -ItemType Directory -Force $directory | Out-Null
            $scriptPath = Join-Path $directory 'input.txt'
            # Let the actual guest collision and damage routines run. Unlike
            # activation smoke tests, this must never force invulnerability.
            @('turbo on', 'until16 1ffe0 == 4252 2500',
              'until16 0070 == 0005 2500', 'wait 350', 'press right 50',
              'until16 13f4 != 0010 1000', 'dump contact',
              'wait 100',
              'until16 13f4 != 000e 1000', 'dump attack', 'quit') | Set-Content $scriptPath
            $env:SNESRECOMP_DUMP_DIR = $directory
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --boss-rush=7 --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
            if ($LASTEXITCODE) { throw 'Grakul damage run failed' }
            foreach ($check in @(@('contact', 14), @('attack', -1))) {
                $ram = [IO.File]::ReadAllBytes((Join-Path $directory "$($check[0]).wram.bin"))
                $health = [int]$ram[0x13f4] + 256 * [int]$ram[0x13f5]
                $unexpectedHealth = if ($check[1] -lt 0) { $health -le 0 -or $health -ge 14 } else { $health -ne $check[1] }
                if ($unexpectedHealth -or $ram[0x1ffe2] -ne 1 -or $ram[0xb8] -ne 11) {
                    throw "Grakul $($check[0]) damage was unexpected: health=$health; see $directory/run.log"
                }
            }
            Write-Output 'Grakul contact and subsequent attack both survived.'
            # Exercise the guest's real defeat sequence with an almost-expired
            # countdown, rather than clearing the boss flag ourselves.
            @('turbo on', 'until16 1ffe0 == 4252 2500',
              'until16 0070 == 0005 2500', 'wait 240',
              'poke 13f0 0800', 'poke 0586 0000',
              'until16 1ffe4 == 0006 1000', 'until16 0070 == 0005 2500',
              'wait 180', 'dump entered', 'press right 50',
              'until16 13f4 != 0010 1000', 'dump transitioned-contact',
              'wait 200', 'dump transitioned-attack', 'quit') | Set-Content $scriptPath
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --boss-rush=6 --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'transition.log')
            if ($LASTEXITCODE) { throw 'Grakul transition run failed' }
            foreach ($check in @(@('entered', 16), @('transitioned-contact', -1), @('transitioned-attack', -1))) {
                $ram = [IO.File]::ReadAllBytes((Join-Path $directory "$($check[0]).wram.bin"))
                $health = [int]$ram[0x13f4] + 256 * [int]$ram[0x13f5]
                $timer = [int]$ram[0x13f0] + 256 * [int]$ram[0x13f1]
                # The preceding fight changes RNG, so the next hit may come
                # from debris or the axe. Both must damage Simon and be survivable.
                $unexpectedHealth = if ($check[1] -lt 0) { $health -le 0 -or $health -ge 16 } else { $health -ne $check[1] }
                if ($unexpectedHealth -or $timer -le 8 -or $ram[0x1ffe2] -ne 1 -or $ram[0xb8] -ne 11) {
                    throw "Grakul transition $($check[0]) failed: health=$health timer=$timer; see $directory/transition.log"
                }
            }
            Write-Output 'Grakul transition resets the countdown and survives contact and attack.'
        } finally { $env:SNESRECOMP_DUMP_DIR = $previousDump }
    }
    if ($Transitions -or $Smoke) {
        $previousDump = $env:SNESRECOMP_DUMP_DIR
        try {
            $directory = Join-Path $PWD 'build/boss-rush-frankenstein-transition'
            New-Item -ItemType Directory -Force $directory | Out-Null
            $scriptPath = Join-Path $directory 'input.txt'
            @('turbo on', 'until16 1ffe0 == 4252 2500',
              'until16 0070 == 0005 2500', 'wait 350',
              'poke 05b2 0000', 'poke 0592 0d00', 'poke 13f6 0000',
              'until16 1ffe4 == 0007 2000', 'until16 0070 == 0005 2500',
              'dump spawn', 'until16 00b8 == 0008 1000', 'dump active', 'wait 240', 'press right 40',
              'until16 13f4 != 0010 1000', 'dump hit', 'quit') | Set-Content $scriptPath
            $env:SNESRECOMP_DUMP_DIR = $directory
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --boss-rush=7 --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
            if ($LASTEXITCODE) { throw 'Frankenstein transition run failed' }
            foreach ($capture in @('spawn', 'active')) {
                $ram = [IO.File]::ReadAllBytes((Join-Path $directory "$capture.wram.bin"))
                if ($ram[0x13f4] -ne 16 -or $ram[0x1ffe2] -ne 1 -or $ram[0x1ffe4] -ne 7 -or $ram[0x4a] -ne 0) {
                    throw "Frankenstein $capture failed; see $directory/run.log"
                }
                if ($capture -eq 'active' -and $ram[0xb8] -ne 8) { throw 'Frankenstein did not activate' }
                if ($ram[0xbc] -ne 0 -or $ram[0xbd] -ne 0) { throw 'Loading protection leaked into Frankenstein combat' }
            }
            $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'hit.wram.bin'))
            $health = [int]$ram[0x13f4] + 256 * [int]$ram[0x13f5]
            if ($health -le 0 -or $health -ge 16 -or $ram[0x1ffe2] -ne 1) { throw "Frankenstein first hit failed: health=$health" }
            Write-Output 'Grakul defeat -> Frankenstein: alive through fade and first hit, loading protection removed.'
            $directory = Join-Path $PWD 'build/boss-rush-dracula-transition'
            New-Item -ItemType Directory -Force $directory | Out-Null
            $scriptPath = Join-Path $directory 'input.txt'
            @('turbo on', 'until16 1ffe0 == 4252 2500',
              'until16 0070 == 0005 2500', 'wait 240',
              'poke 00b8 0000', 'poke 13f6 0000',
              'until16 0086 == 0042 1000', 'until16 0070 == 0004 2500',
              'dump inherited', 'until16 0086 == 0043 1000',
              'until16 0070 == 0005 2500', 'dump spawn',
              'until16 0592 == 0003 1800', 'wait 20', 'dump visible', 'quit') | Set-Content $scriptPath
            $env:SNESRECOMP_DUMP_DIR = $directory
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --boss-rush=13 --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
            if ($LASTEXITCODE) { throw 'Dracula transition run failed' }
            $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'spawn.wram.bin'))
            if ($ram[0x54e] -ne 0xa5 -or $ram[0x13f4] -ne 16 -or $ram[0xbc] -ne 0) { throw 'Dracula spawn is not clear of the floor with normal damage enabled' }
            $inherited = [IO.File]::ReadAllBytes((Join-Path $directory 'inherited.vram.bin'))
            $spawn = [IO.File]::ReadAllBytes((Join-Path $directory 'spawn.vram.bin'))
            $nonzero = 0
            foreach ($i in 0x8000..0x8fff) {
                if ($spawn[$i] -ne $inherited[$i]) { throw 'Dracula cape graphics were overwritten' }
                $nonzero += [int]$spawn[$i]
            }
            if (!$nonzero) { throw 'Dracula inherited graphics are empty' }
            $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'visible.wram.bin'))
            if ($ram[0xb8] -ne 14 -or $ram[0x13f4] -eq 0 -or $ram[0x54e] -gt 0xa5) { throw 'Dracula appearance or floor collision failed' }
            Write-Output 'Death -> Dracula: clear spawn, inherited cape graphics retained, first appearance reached.'
        } finally { $env:SNESRECOMP_DUMP_DIR = $previousDump }
    }
    if ($Smoke) {
        $previousDump = $env:SNESRECOMP_DUMP_DIR
        try {
            for ($boss = 1; $boss -le 14; $boss++) {
                $directory = Join-Path $PWD "build/boss-rush-smoke-$boss"
                New-Item -ItemType Directory -Force $directory | Out-Null
                $scriptPath = Join-Path $directory 'input.txt'
                $introFrames = if ($boss -eq 3) { 810 } else { 240 }
                @('turbo on', 'forcepoke 00bc ff7f',
                  'until16 1ffe0 == 4252 2500', 'until16 0070 == 0005 2500',
                  "wait $introFrames", 'dump fight', 'quit') | Set-Content $scriptPath
                $env:SNESRECOMP_DUMP_DIR = $directory
                & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher "--boss-rush=$boss" --script $scriptPath '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
                if ($LASTEXITCODE) { throw "Boss $boss smoke run failed" }
                $ram = [IO.File]::ReadAllBytes((Join-Path $directory 'fight.wram.bin'))
                if ($ram[0xb8] -eq 0 -or $ram[0x1ffe2] -ne 1 -or $ram[0x1ffe4] -ne $boss-1) {
                    throw "Boss $boss did not activate; see $directory/run.log"
                }
                Write-Output "Boss $boss / 14 activated."
            }
        } finally { $env:SNESRECOMP_DUMP_DIR = $previousDump }
    }
} finally { Pop-Location }
