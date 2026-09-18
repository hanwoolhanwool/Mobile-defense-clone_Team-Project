param([switch]$Check)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$TargetRoot = Join-Path $ProjectRoot 'Content/LD/Data'
$Names = @('GameRules.json','DT_Units.json','DT_EnemyTypes.json','DT_Waves.json','DT_SummonProfiles.json','DT_SpawnProfiles.json')
if (!$Check) { New-Item -ItemType Directory -Force -Path $TargetRoot | Out-Null }
foreach ($Name in $Names) {
    $Source = Join-Path (Join-Path $ProjectRoot 'data') $Name
    $Target = Join-Path $TargetRoot $Name
    if (!$Check) { Copy-Item -LiteralPath $Source -Destination $Target }
    if (!(Test-Path -LiteralPath $Target) -or (Get-FileHash -LiteralPath $Source).Hash -ne (Get-FileHash -LiteralPath $Target).Hash) {
        throw "Runtime data differs: $Name"
    }
}
Write-Output "P0 runtime data: $($Names.Count) files match."
