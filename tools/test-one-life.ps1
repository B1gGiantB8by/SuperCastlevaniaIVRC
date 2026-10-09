$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
$previousDump = $env:SNESRECOMP_DUMP_DIR
try {
    $directory = Join-Path $PWD 'build/one-life-regression'
    New-Item -ItemType Directory -Force $directory | Out-Null
    $scriptPath = Join-Path $directory 'input.txt'
    $commands = @('turbo on', 'until16 0070 == 0005 2500', 'wait 120')
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        $commands += @('poke 13f4 0000', 'until16 0070 == 0007 2000',
            'wait 180', 'turbo off', 'press start 1',
            'until16 0070 == 0005 2500', 'wait 120', "dump continue_$attempt", 'turbo on')
    }
    $commands += @('forcepoke 00bc ff7f', 'forcepoke 054e 8000',
        'press right 670', 'dump bridge', 'wait 30', 'dump bridge_middle', 'quit')
    $commands | Set-Content $scriptPath
    $configPath = Join-Path $directory 'config.ini'
    (Get-Content build/config.ini -Raw).Replace('Fullscreen = 1', 'Fullscreen = 0') | Set-Content $configPath
    $env:SNESRECOMP_DUMP_DIR = $directory
    $process = Start-Process -FilePath (Join-Path $PWD 'build/SuperCastlevaniaIvSNESRecomp.exe') -WindowStyle Hidden -PassThru -ArgumentList @('--one-life', '--no-launcher', '--config', $configPath, '--script', $scriptPath, '"../Super Castlevania IV (USA).sfc"')
    if (!$process.WaitForExit(120000)) {
        $process.Kill()
        throw 'One Life Continue did not finish within two minutes.'
    }
    if ($process.ExitCode) { throw 'One Life Continue smoke failed.' }
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        $ram = [IO.File]::ReadAllBytes((Join-Path $directory "continue_$attempt.wram.bin"))
        if ($ram[0x70] -ne 5 -or $ram[0x86] -ne 0 -or $ram[0x7c] -ne 1 -or $ram[0x13e2] -ne 0 -or $ram[0x13f4] -ne 16) {
            throw "Continue $attempt did not restore level 1, one life, music and health."
        }
    }
    Write-Output 'Three normal-speed Continue cycles passed. Bridge captures saved for visual review.'
} finally { $env:SNESRECOMP_DUMP_DIR = $previousDump; Pop-Location }
