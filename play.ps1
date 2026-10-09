$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/build/SuperCastlevaniaIvSNESRecomp.exe" --rom "$PSScriptRoot/../Super Castlevania IV (USA).sfc" @args
exit $LASTEXITCODE
