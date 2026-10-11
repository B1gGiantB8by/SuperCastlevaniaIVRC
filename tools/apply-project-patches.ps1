param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
foreach ($entry in @(@('snesrecomp', 'snesrecomp-v0.2.patch'), @('recomp-ui', 'recomp-ui-v0.2.patch'), @('recomp-ui', 'sciv-frontend.patch'))) {
    $target = Join-Path $root $entry[0]
    $patch = Join-Path $PSScriptRoot $entry[1]
    git -C $target apply --reverse --check $patch 2>$null
    if ($LASTEXITCODE -eq 0) { continue }
    git -C $target apply --check $patch
    if ($LASTEXITCODE) { throw "Patch conflicts: $patch" }
    git -C $target apply $patch
    if ($LASTEXITCODE) { throw "Could not apply $patch" }
}
