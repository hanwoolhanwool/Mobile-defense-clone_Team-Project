param(
    [string]$ProjectRoot = (Split-Path $PSScriptRoot -Parent),
    [string]$Filter = 'LD.P0.G0',
    [string]$RunId = ('Tests-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
New-Item -ItemType Directory -Force -Path $RunRoot | Out-Null
$Editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$Project = Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'
$Arguments = @($Project,'-unattended','-NullRHI','-nosound','-nosplash','-culture=en',"-ExecCmds=Automation RunTests $Filter",'-TestExit=Automation Test Queue Empty',"-ReportExportPath=$RunRoot/report", "-abslog=$RunRoot/engine.log")
$Result = [ordered]@{Scope='Unreal automation with NullRHI; not PIE or visual gameplay'; Filter=$Filter; Started=(Get-Date).ToString('o'); Head=(& git -C $ProjectRoot rev-parse HEAD); Command=$Editor; Arguments=$Arguments}
& $Editor @Arguments *> "$RunRoot/process.log"
$Result.ExitCode = $LASTEXITCODE
$ReportFile = Join-Path $RunRoot 'report/index.json'
if (Test-Path -LiteralPath $ReportFile) {
    $Report = Get-Content -LiteralPath $ReportFile -Raw | ConvertFrom-Json
    $Result.Succeeded = $Report.succeeded
    $Result.Failed = $Report.failed
    $Result.NotRun = $Report.notRun
    $Result.Result = if ($Result.ExitCode -eq 0 -and $Result.Succeeded -gt 0 -and $Result.Failed -eq 0 -and $Result.NotRun -eq 0) {'Pass'} else {'Fail'}
} else { $Result.Result = 'Fail'; $Result.Error = 'Automation report missing; exit zero alone is not evidence.' }
$Result.Finished = (Get-Date).ToString('o')
$Result | ConvertTo-Json -Depth 5 | Set-Content "$RunRoot/result.json" -Encoding utf8
$Result | ConvertTo-Json -Depth 5
if ($Result.Result -ne 'Pass') {
    Select-String -Path "$RunRoot/engine.log" -Pattern 'Error:|Fail|No automation tests|Test Completed' | Select-Object -Last 25
    exit 1
}
