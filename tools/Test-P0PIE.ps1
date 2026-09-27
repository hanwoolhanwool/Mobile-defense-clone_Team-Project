param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a new simple RunId.' }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
$ProofId = "$RunId-proof"
if ((Test-Path -LiteralPath $RunRoot) -or (Test-Path -LiteralPath "$RunRoot-proof")) { throw 'Previous evidence is preserved. Choose a new RunId.' }
if (Get-NetTCPConnection -LocalPort 17879 -State Listen -ErrorAction SilentlyContinue) { throw 'PIE tool port is in use.' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$Arguments = @((Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'), '-unattended', '-nosound', '-nosplash', '-RenderOffscreen', '-culture=ko', '-P0Seed=1776', '-ModelContextProtocolPort=17879', "-P0PIERun=$ProofId", '-ExecCmds=Automation RunTests LD.PIE.P0.Session', '-TestExit=Automation Test Queue Empty', "-ReportExportPath=$RunRoot/report", "-abslog=$RunRoot/engine.log")
$Result = [ordered]@{Scope='Actual GPU Editor PIE; two in-process network worlds'; Head=(& git -C $ProjectRoot rev-parse HEAD); Started=(Get-Date).ToString('o'); Command=$Editor; Arguments=$Arguments; Result='Running'}
$Process = $null
try {
    $Quoted = ($Arguments | ForEach-Object { '"' + $_.Replace('"','\"') + '"' }) -join ' '
    $Process = Start-Process -FilePath $Editor -ArgumentList $Quoted -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunRoot/stdout.log" -RedirectStandardError "$RunRoot/stderr.log"
    $Result.ProcessId = $Process.Id
    if (!$Process.WaitForExit(300000)) { throw 'PIE verification timed out after 300 seconds.' }
    $Result.ExitCode = $Process.ExitCode
    $Report = Get-Content -LiteralPath "$RunRoot/report/index.json" -Raw | ConvertFrom-Json
    $Proof = Get-Content -LiteralPath "$RunRoot-proof/pie-proof.json" -Raw | ConvertFrom-Json
    $Result.Succeeded = [int]$Report.succeeded + [int]$Report.succeededWithWarnings
    $Result.Failed = $Report.failed
    $Result.SettingsRestored = $Proof.settingsRestored
    $Result.Result = if ($Result.ExitCode -eq 0 -and $Result.Succeeded -eq 1 -and $Report.failed -eq 0 -and $Proof.result -eq 'Pass' -and $Proof.settingsRestored) {'Pass'} else {'Fail'}
} catch {
    $Result.Result = 'Fail'
    $Result.Error = $_.Exception.Message
} finally {
    if ($Process -and !$Process.HasExited) { $Process.CloseMainWindow() | Out-Null; if (!$Process.WaitForExit(5000)) { $Process.Kill(); $Process.WaitForExit() } }
    $Result.Finished = (Get-Date).ToString('o')
    $Result | ConvertTo-Json -Depth 5 | Set-Content "$RunRoot/result.json" -Encoding utf8
}
Write-Output "$($Result.Result): $RunRoot/result.json"
if ($Result.Result -ne 'Pass') { exit 1 }
