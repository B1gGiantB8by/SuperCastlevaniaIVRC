param([string]$Rom = (Join-Path $PSScriptRoot '../../Super Castlevania IV (USA) (MSU1).sfc/Super_Castlevania_IV_(USA)_(MSU1).sfc'))
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$romPath=(Resolve-Path -LiteralPath $Rom).Path
$previousTrace=$env:SCIV_MSU_TRACE
$previousDump=$env:SNESRECOMP_DUMP_DIR
$cache=Join-Path $root 'build/rom.cfg'
$cached=if(Test-Path -LiteralPath $cache){[IO.File]::ReadAllBytes($cache)}else{$null}
Push-Location $root
try {
    $directory=Join-Path $root 'build/msu-smoke'
    New-Item -ItemType Directory -Force $directory | Out-Null
    $script=Join-Path $directory 'input.txt'
    @('turbo on','wait 1800','press start 2','wait 180','press start 2','wait 180','press start 2','wait 1200','dump gameplay','quit') | Set-Content $script
    $env:SCIV_MSU_TRACE='1'
    $env:SNESRECOMP_DUMP_DIR=$directory
    & ./build/SuperCastlevaniaIvSNESRecomp.exe --no-launcher --script $script --rom $romPath *> (Join-Path $directory 'run.log')
    if($LASTEXITCODE){throw 'MSU-1 smoke run failed'}
    $ram=[IO.File]::ReadAllBytes((Join-Path $directory 'gameplay.wram.bin'))
    if($ram[0x70] -ne 5){throw 'MSU-1 ROM did not reach gameplay'}
    $log=Get-Content (Join-Path $directory 'run.log') -Raw
    foreach($track in @(20,22,1)) {
        if($log -notmatch "\[sciv-msu\] track=$track file=.* found=1"){throw "MSU-1 track $track was not opened"}
    }
    Write-Output 'MSU-1 passed: gameplay reached; tracks 20, 22 and 1 opened from the supplied pack.'
} finally {
    $env:SCIV_MSU_TRACE=$previousTrace
    $env:SNESRECOMP_DUMP_DIR=$previousDump
    if($null -ne $cached){[IO.File]::WriteAllBytes($cache,$cached)}else{Remove-Item -LiteralPath $cache -ErrorAction SilentlyContinue}
    Pop-Location
}
