$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
$previousDump=$env:SNESRECOMP_DUMP_DIR
try {
    $rooms=@(0x00,0x02,0x05,0x08,0x09,0x0a,0x0c,0x0d,0x0e,0x12,0x15,0x16,0x17,0x18,0x19,0x1a,0x1c,0x1f,0x23,0x26,0x2a,0x2c,0x2e,0x31,0x37,0x39,0x3c,0x3e,0x3f,0x42)
    $maps=@(4,7,10,14,16,19,21,23,25,27)
    foreach($map in @(0,1)) {
        $entries=if($map) {$maps} else {1..$rooms.Count}
        foreach($entry in $entries) {
            $directory=Join-Path $PWD "build/level-select-$map-$entry"
            New-Item -ItemType Directory -Force $directory | Out-Null
            $script=Join-Path $directory 'input.txt'
            $commands=@('turbo on')
            if($map) {$commands+=@('until16 0070 == 000b 2500','wait 120','dump map','press start 1')}
            $commands+=@('until16 0070 == 0005 2500','wait 15','dump stage','quit')
            $commands | Set-Content $script
            $env:SNESRECOMP_DUMP_DIR=$directory
            $option=if($map) {"--level-map=$entry"} else {"--level-select=$entry"}
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher $option --script $script '../Super Castlevania IV (USA).sfc' *> (Join-Path $directory 'run.log')
            if($LASTEXITCODE) {throw "Entry $entry map=$map process failed"}
            $ram=[IO.File]::ReadAllBytes((Join-Path $directory 'stage.wram.bin'))
            if($ram[0x70] -ne 5 -or $ram[0x86] -ne $rooms[$entry-1] -or $ram[0x13f4] -eq 0) {throw "Entry $entry map=$map failed state=$($ram[0x70]) room=$($ram[0x86]) hp=$($ram[0x13f4])"}
            if($map) {
                $mapRam=[IO.File]::ReadAllBytes((Join-Path $directory 'map.wram.bin'))
                if($mapRam[0x70] -ne 11) {throw "Entry $entry did not stay in map transition"}
            }
            Write-Output "Entry $entry map=$map passed"
        }
    }
} finally {$env:SNESRECOMP_DUMP_DIR=$previousDump; Pop-Location}
