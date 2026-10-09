$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
$previousDump=$env:SNESRECOMP_DUMP_DIR
try {
    $directory=Join-Path $PWD 'build/title-menu-test'
    New-Item -ItemType Directory -Force $directory | Out-Null
    $env:SNESRECOMP_DUMP_DIR=$directory
    $prefix=@('turbo on','wait 2100','press start 1','wait 120')
    $down=@('press down 1','wait 2')
    $tests=@{
        start=@('press start 1','wait 250','dump start','quit')
        password=$down+@('press start 1','wait 250','press start 4','wait 120','dump password','quit')
        rush=$down+$down+$down+@('press start 1','wait 2','press start 1','until16 0070 == 0005 2500','dump rush','quit')
        level=$down+$down+$down+@('press start 1','wait 2')+$down+$down+@('press start 1','wait 2','press start 1','until16 0070 == 0005 2500','dump level','quit')
        quit=$down+$down+$down+$down+$down+@('press start 1','wait 2')+$down+@('press start 1','wait 120','quit')
    }
    foreach($name in @('start','password','rush','level','quit')) {
        $script=Join-Path $directory "$name.txt"
        $prefix+$tests[$name] | Set-Content $script
        & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --script $script '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory "$name.log")
        if($LASTEXITCODE) { throw "$name failed" }
        if($name -eq 'quit') {
            if(!(Select-String -Path (Join-Path $directory "$name.log") -SimpleMatch 'title menu quit')) { throw 'Quit did not exit through title menu' }
        } else {
            $ram=[IO.File]::ReadAllBytes((Join-Path $directory "$name.wram.bin"))
            if($name -in @('rush','level') -and $ram[0x70] -ne 5) { throw "$name did not reach gameplay" }
            if($name -eq 'start' -and $ram[0x32] -ne 4) { throw 'Start did not reach native name entry' }
            if($name -eq 'password' -and $ram[0x32] -ne 6) { throw 'Password did not reach native password entry' }
        }
        Write-Output "$name passed"
    }
} finally { $env:SNESRECOMP_DUMP_DIR=$previousDump; Pop-Location }
