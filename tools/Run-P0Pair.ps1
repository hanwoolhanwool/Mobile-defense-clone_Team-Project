param(
    [Parameter(Mandatory = $true)][string]$RunId,
    [string]$GameExecutable = '',
    [ValidateSet('G1')][string]$Probe = 'G1',
    [int]$Port = 17777,
    [int]$Width = 540,
    [int]$Height = 1170,
    [int]$TimeoutSeconds = 110,
    [switch]$RenderOffscreen,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a simple new RunId.' }
if ($Port -lt 1024 -or $Port -gt 65535 -or $Width -lt 320 -or $Height -lt 320) { throw 'Invalid port or viewport.' }
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw "Port $Port is in use." }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'RunId already exists; preserve it and choose a new one.' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$UsingEditor = !$GameExecutable
if ($UsingEditor) { $GameExecutable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
if (!(Test-Path -LiteralPath $GameExecutable)) { throw "Executable missing: $GameExecutable" }
$Pairs = @()
$Metadata = [ordered]@{Scope=$(if ($UsingEditor) {'Two separate UnrealEditor -game processes; not PIE or packaged execution'} else {'Two separate packaged game processes'}); Probe=$Probe; Started=(Get-Date).ToString('o'); Head=(& git -C $ProjectRoot rev-parse HEAD); Executable=$GameExecutable; RenderOffscreen=[bool]$RenderOffscreen; Port=$Port; Processes=@(); Result='Running'}
$AllPassed = $false
try {
foreach ($Role in @('host','client')) {
    $Output = Join-Path $RunRoot $Role
    New-Item -ItemType Directory -Path $Output | Out-Null
    $Arguments = @()
    if ($UsingEditor) { $Arguments += (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject') }
    $Arguments += $(if ($Role -eq 'host') { '/Game/LD/Maps/L_P0?listen' } else { "127.0.0.1:$Port" })
    if ($UsingEditor) { $Arguments += '-game' }
    $Arguments += @('-windowed','-ForceRes',"-ResX=$Width", "-ResY=$Height", "-port=$Port", '-nosplash', '-nosound', '-unattended', '-culture=ko', "-P0Probe=$Probe", "-P0ProbeOutput=$Output", "-abslog=$Output/engine.log", '-ExecCmds=t.IdleWhenNotForeground 0,t.MaxFPS 60,r.VSync 0')
    if ($RenderOffscreen) { $Arguments += '-RenderOffscreen' }
    $QuotedArguments = ($Arguments | ForEach-Object { '"' + $_.Replace('"','\"') + '"' }) -join ' '
    $Process = Start-Process -FilePath $GameExecutable -ArgumentList $QuotedArguments -PassThru -WindowStyle Hidden -RedirectStandardOutput "$Output/stdout.log" -RedirectStandardError "$Output/stderr.log"
    $Pairs += [pscustomobject]@{Role=$Role; Process=$Process; Arguments=$Arguments; Output=$Output}
    $Metadata.Processes = @($Pairs | ForEach-Object { @{Role=$_.Role; Id=$_.Process.Id; Arguments=$_.Arguments} })
    $Metadata | ConvertTo-Json -Depth 6 | Set-Content "$RunRoot/pair.json" -Encoding utf8
    if ($Role -eq 'host') { Start-Sleep -Seconds 5 }
}
$Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while (($Pairs | Where-Object { !$_.Process.HasExited }) -and (Get-Date) -lt $Deadline) { Start-Sleep -Milliseconds 500 }
$AllPassed = $true
foreach ($Pair in $Pairs) {
    if (!$Pair.Process.HasExited) {
        $AllPassed = $false
        continue
    }
    $ResultPath = Join-Path $Pair.Output 'result.json'
    if (!(Test-Path -LiteralPath $ResultPath)) { $AllPassed = $false; continue }
    $Result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
    if ($Result.result -ne 'Pass' -or $Pair.Process.ExitCode -ne 0) { $AllPassed = $false }
}
} catch {
    $AllPassed = $false
    $Metadata.Error = $_.Exception.Message
} finally {
    foreach ($Pair in $Pairs) {
        # Cleanup is restricted to Process objects created here, including partial startup/parse failures.
        if (!$Pair.Process.HasExited) {
            $AllPassed = $false
            $Pair.Process.CloseMainWindow() | Out-Null
            if (!$Pair.Process.WaitForExit(5000)) { $Pair.Process.Kill(); $Pair.Process.WaitForExit(5000) | Out-Null }
        }
    }
$Metadata.Finished = (Get-Date).ToString('o')
$Metadata.Result = if ($AllPassed) {'Pass'} else {'Fail'}
$Metadata | ConvertTo-Json -Depth 6 | Set-Content "$RunRoot/pair.json" -Encoding utf8
}
Write-Output "$($Metadata.Result): $RunRoot/pair.json"
foreach ($Pair in $Pairs) {
    Write-Output "$($Pair.Role): $($Pair.Output)"
    if (!$AllPassed -and (Test-Path "$($Pair.Output)/engine.log")) {
        Select-String -Path "$($Pair.Output)/engine.log" -Pattern 'LogLDP0Probe|Error:|Fatal error' | Select-Object -Last 12
    }
}
exit $(if ($AllPassed) {0} else {1})
