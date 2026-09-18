param(
    [string]$ProjectRoot = (Split-Path $PSScriptRoot -Parent),
    [string]$RunId = ('G0-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectFile = Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
New-Item -ItemType Directory -Force -Path $RunRoot | Out-Null
$Arguments = @('Mobile_defense_cloneEditor','Win64','Development',"-Project=$ProjectFile",'-WaitMutex','-NoHotReloadFromIDE','-MaxParallelActions=4')
$Build = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$Result = [ordered]@{ Scope='Unreal Editor compile only'; Started=(Get-Date).ToString('o'); Command=$Build; Arguments=$Arguments; Head=(& git -C $ProjectRoot rev-parse HEAD); Result='Running' }
$Result | ConvertTo-Json -Depth 4 | Set-Content "$RunRoot/result.json" -Encoding utf8
& $Build @Arguments *> "$RunRoot/build.log"
$Result.ExitCode = $LASTEXITCODE
$Result.Result = if ($LASTEXITCODE -eq 0) { 'Pass' } else { 'Fail' }
$Result.Finished = (Get-Date).ToString('o')
$Result | ConvertTo-Json -Depth 4 | Set-Content "$RunRoot/result.json" -Encoding utf8
Write-Output "$($Result.Result): $RunRoot/build.log"
if ($Result.ExitCode -ne 0) { Get-Content "$RunRoot/build.log" -Tail 35 }
exit $Result.ExitCode
