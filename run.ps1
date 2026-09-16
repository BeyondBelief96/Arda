#Requires -Version 5.1
<#
.SYNOPSIS
  Build (unless -NoBuild) and run an Arda executable from the repo root.

.EXAMPLE
  .\run.ps1                 # Debug, vs preset
  .\run.ps1 release         # Release, vs preset
  .\run.ps1 -Preset ninja   # Debug via Ninja Multi-Config
  .\run.ps1 -NoBuild        # just launch what's already built
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Debug',

    [ValidateSet('vs', 'ninja')]
    [string]$Preset = 'vs',

    [string]$Target = 'arda_scene',

    [switch]$NoBuild,

    # Everything after -- is forwarded to the executable.
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$AppArgs
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

# Accept lowercase shorthand: .\run.ps1 release
$Config = (Get-Culture).TextInfo.ToTitleCase($Config.ToLower()) -replace 'Relwithdebinfo', 'RelWithDebInfo' -replace 'Minsizerel', 'MinSizeRel'

$binaryDir = Join-Path $PSScriptRoot "build\$Preset"
if (-not (Test-Path $binaryDir)) { throw "Not configured. Run:  cmake --preset $Preset" }

if (-not $NoBuild) {
    # --config works for every config, including ones with no build preset.
    cmake --build $binaryDir --config $Config --target $Target
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$exe = Join-Path $binaryDir "bin\$Config\$Target.exe"
if (-not (Test-Path $exe)) {
    throw "Not found: $exe"
}

& $exe @AppArgs
exit $LASTEXITCODE
