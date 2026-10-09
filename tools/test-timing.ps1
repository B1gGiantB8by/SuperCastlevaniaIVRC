$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '..')
$previousDump=$env:SNESRECOMP_DUMP_DIR
try {
    foreach($mode in @('original','enhanced')) {
        foreach($entry in @('cave','vipers','mode7','dracula')) {
            $dir=Join-Path $PWD "build/timing-tests/$mode-$entry"
            New-Item -ItemType Directory -Force $dir | Out-Null
            $start=switch($entry) {
                cave {'--level-select=7'}
                vipers {'--boss-rush=3'}
                mode7 {'--level-select=11'}
                dracula {'--boss-rush=14'}
            }
            @('turbo on','until16 0070 == 0005 2500','wait 120',
              'dump start','press y 240','dump end','quit') |
                Set-Content (Join-Path $dir 'input.txt')
            $env:SNESRECOMP_DUMP_DIR=$dir
            & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher "--$mode-timing" $start --script (Join-Path $dir 'input.txt') '../Super Castlevania IV (USA).sfc' *> (Join-Path $dir 'run.log')
            if($LASTEXITCODE) {throw "$mode $entry failed: $dir/run.log"}
            if(Select-String -Path (Join-Path $dir 'run.log') -Pattern 'step cap|unresolved|fatal|BRK' -Quiet) {throw "$mode $entry runtime diagnostic"}
            $ram=[IO.File]::ReadAllBytes((Join-Path $dir 'end.wram.bin'))
            if($ram[0x70] -ne 5) {throw "$mode $entry left gameplay"}
            Write-Output "$mode $entry passed"
        }
    }
} finally { $env:SNESRECOMP_DUMP_DIR=$previousDump; Pop-Location }
