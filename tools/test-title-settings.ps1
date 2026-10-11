$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    $directory=Join-Path $PWD 'build/title-menu-test'
    $config=Join-Path $directory 'settings-v2.ini'
    @('[General]','Autosave=0','RunAhead=0','[Graphics]','OutputMethod=OpenGL','Renderer=opengl','Fullscreen=0','WindowScale=3','LinearFiltering=0','Shader=assets/shaders/sharp.glslp','[Sound]','EnableAudio=1','Volume=100','[Controller]','SourceP1=1','GamepadDeadzone=3277','[Rewind]','Enabled=1','Depth=300','Interval=3') | Set-Content $config
    function MoveDown($count) { for($i=0;$i -lt $count;$i++) { 'press down 1';'wait 2' } }
    $commands=@('turbo on','wait 2100','press start 1','wait 120')
    $commands+=(MoveDown 2)+@('press start 1','wait 2','press start 1','wait 2') # Options -> Video
    $commands+=(MoveDown 3)+@('press right 1','wait 2') # Smoothing
    $commands+=(MoveDown 4)+@('press right 1','wait 2') # Sharp -> Warm Composite (past Motion)
    $commands+=@('press b 1','wait 2')+(MoveDown 1)+@('press start 1','wait 2') # Audio
    $commands+=(MoveDown 1)+@('press left 1','wait 2','press b 1','wait 2') # 95% volume
    $commands+=(MoveDown 3)+@('press start 1','wait 2')+(MoveDown 1)+@('press right 1','wait 2','press b 1','wait 2') # Controls deadzone
    $commands+=(MoveDown 4)+@('press start 1','wait 2')+(MoveDown 1)+@('press right 1','wait 2','quit') # Rewind depth
    $script=Join-Path $directory 'settings-v2.txt'
    $commands | Set-Content $script
    & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --config $config --script $script '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'settings-v2.log')
    if($LASTEXITCODE) {throw 'Settings navigation failed'}
    $saved=Get-Content $config -Raw
    foreach($pattern in @('LinearFiltering\s*=\s*1','Shader\s*=\s*assets/shaders/warm-composite.glslp','Volume\s*=\s*95','GamepadDeadzone\s*=\s*4915','Depth\s*=\s*360')) {
        if($saved -notmatch $pattern) {throw "Setting was not persisted: $pattern"}
    }
    if(Select-String -Path (Join-Path $directory 'settings-v2.log') -SimpleMatch 'in-game launcher OPEN') {throw 'Settings opened a separate launcher'}
    Write-Output 'Video, shader, audio, controls and rewind changes applied and persisted in the title menu.'
} finally {Pop-Location}

