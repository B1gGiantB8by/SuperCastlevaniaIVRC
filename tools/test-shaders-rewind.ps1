param([string]$Compiler = 'clang')
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    & $Compiler -std=gnu11 -Isnesrecomp/runner/src tools/rewind-hold-test.c snesrecomp/runner/src/snes_rewind.c snesrecomp/runner/src/snes_overlay_draw.c -o build/rewind-hold-test.exe
    if ($LASTEXITCODE) { throw 'Rewind test compilation failed' }
    & ./build/rewind-hold-test.exe
    if ($LASTEXITCODE) { throw 'Rewind regression failed' }
    $directory = Join-Path $PWD 'build/shader-tests'
    New-Item -ItemType Directory -Force $directory | Out-Null
    $times = @{}
    foreach ($preset in @('sharp', 'crt-soft', 'lcd-grid', 'warm-composite')) {
        $config = Join-Path $directory "$preset.ini"
        @('[General]', 'Autosave = 0', 'DisableFrameDelay = 0',
          '[Graphics]', 'Fullscreen = 0', 'WindowScale = 2', 'OutputMethod = OpenGL',
          'VSync = 0', "Shader = assets/shaders/$preset.glslp", '[Sound]', 'EnableAudio = 1') | Set-Content $config
        foreach ($mode in @('normal', 'turbo', 'resume')) {
            $script = Join-Path $directory "$preset-$mode.txt"
            $commands = switch ($mode) {
                normal { @('wait 180', 'quit') }
                turbo { @('turbo on', 'wait 180', 'quit') }
                resume { @('turbo on', 'wait 30', 'turbo off', 'wait 180', 'quit') }
            }
            $commands | Set-Content $script
            $log = Join-Path $directory "$preset-$mode.log"
            $timer = [Diagnostics.Stopwatch]::StartNew()
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --config $config --script $script '../Super Castlevania IV (USA).sfc' *> $log
            $timer.Stop()
            if ($LASTEXITCODE) { throw "$preset $mode failed: $log" }
            if (Select-String -Path $log -Pattern 'Unable to read|Couldn.t read shader|linking shader|error:|GL CALLBACK|fatal|hold restore refused' -Quiet) {
                throw "$preset $mode runtime diagnostic: $log"
            }
            $times[$mode] = $timer.Elapsed.TotalSeconds
        }
        if ($times.turbo -ge $times.normal * 0.85) { throw "$preset Turbo did not accelerate execution" }
        if ($times.resume -lt $times.normal * 0.85) { throw "$preset did not resume normal pacing" }
        Write-Output ("{0}: shader loaded; normal {1:F2}s, Turbo {2:F2}s, released {3:F2}s" -f $preset, $times.normal, $times.turbo, $times.resume)
    }
} finally { Pop-Location }
