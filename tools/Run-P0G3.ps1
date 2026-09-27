param(
    [Parameter(Mandatory = $true)][string]$RunId,
    [string]$GameExecutable = '',
    [int]$MatchCount = 1,
    [int]$MinimumSeconds = 0,
    [int]$TimeoutSeconds = 1500,
    [ValidateSet(0,150,300)][int]$RTTMilliseconds = 0,
    [ValidateSet(0,1,3)][int]$PacketLossPercent = 0,
    [int]$Port = 17777,
    [int]$Width = 540,
    [int]$Height = 1170,
    [int]$MaxFPS = 60,
    [switch]$RenderOffscreen,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a new simple RunId.' }
if ($MatchCount -lt 1 -or $MatchCount -gt 20 -or $MinimumSeconds -lt 0 -or $TimeoutSeconds -le $MinimumSeconds) { throw 'Invalid duration or match count.' }
if ($Port -lt 1024 -or $Port -gt 65400 -or $Width -lt 320 -or $Height -lt 320) { throw 'Invalid port or viewport.' }
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw "Port $Port already in use." }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Existing run preserved. Choose a new RunId.' }
$UsingEditor = !$GameExecutable
if ($UsingEditor) { $GameExecutable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
if (!(Test-Path -LiteralPath $GameExecutable)) { throw "Executable missing: $GameExecutable" }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Metadata = [ordered]@{
    Scope = $(if ($UsingEditor) {'Actual GPU two separate Editor-game processes; not PIE or package'} else {'Actual GPU two separate packaged processes'})
    Probe = 'G3'; Started = (Get-Date).ToString('o'); Head = (& git -C $ProjectRoot rev-parse HEAD)
    Executable = $GameExecutable; ExecutableSHA256 = (Get-FileHash -LiteralPath $GameExecutable -Algorithm SHA256).Hash
    Matches = $MatchCount; MinimumSeconds = $MinimumSeconds; RequestedRTTMilliseconds = $RTTMilliseconds
    OutgoingDelayPerEndpointMs = ($RTTMilliseconds / 2); OutgoingLossPercentPerEndpoint = $PacketLossPercent
    NetworkScope = 'Engine packet emulation in each direction. RTT measured by client unreliable echo; lost echo fraction is not equal to per-direction configured packet loss.'
    Resolution = @($Width,$Height); MaxFPS = $MaxFPS; VSync = 0; RenderOffscreen = [bool]$RenderOffscreen
    Strategy = 'Automated Slate summon/merge/sale and Controller move using normal product rules; no HP/gold/wave clock override.'
    Seeds = @(1776,42,1729,2026,9001); Processes = @(); Result = 'Running'
}
$Pairs = @()
$AllPassed = $false
try {
    foreach ($Role in @('host','client')) {
        $Output = Join-Path $RunRoot $Role
        New-Item -ItemType Directory -Path $Output | Out-Null
        $Arguments = @()
        if ($UsingEditor) { $Arguments += (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject') }
        $Arguments += '/Game/LD/Maps/L_P0Entry'
        if ($UsingEditor) { $Arguments += '-game' }
        $ToolPort = $Port + $(if ($Role -eq 'host') {100} else {101})
        if (Get-NetTCPConnection -LocalPort $ToolPort -State Listen -ErrorAction SilentlyContinue) { throw "Tool port $ToolPort is occupied." }
        $Arguments += @("-ModelContextProtocolPort=$ToolPort",'-windowed','-ForceRes',"-ResX=$Width", "-ResY=$Height", "-port=$Port", '-nosplash', '-nosound', '-unattended', '-culture=ko', '-P0Probe=G3', "-P0Role=$Role", "-P0PeerAddress=127.0.0.1:$Port", "-P0Matches=$MatchCount", "-P0MinimumSeconds=$MinimumSeconds", "-P0TimeoutSeconds=$TimeoutSeconds", '-P0Seed=1776', "-P0ProbeOutput=$Output", "-abslog=$Output/engine.log", "-ExecCmds=t.IdleWhenNotForeground 0,t.MaxFPS $MaxFPS,r.VSync 0", "-PktLagMin=$($RTTMilliseconds / 2)", "-PktLagMax=$($RTTMilliseconds / 2)", "-PktLoss=$PacketLossPercent")
        if ($RenderOffscreen) { $Arguments += '-RenderOffscreen' }
        $QuotedArguments = ($Arguments | ForEach-Object { '"' + $_.Replace('"','\"') + '"' }) -join ' '
        $Process = Start-Process -FilePath $GameExecutable -ArgumentList $QuotedArguments -PassThru -WindowStyle Hidden -RedirectStandardOutput "$Output/stdout.log" -RedirectStandardError "$Output/stderr.log"
        $Pairs += [pscustomobject]@{Role=$Role; Process=$Process; Arguments=$Arguments; Output=$Output}
        $Metadata.Processes = @($Pairs | ForEach-Object { @{Role=$_.Role; Id=$_.Process.Id; Arguments=$_.Arguments} })
        $Metadata | ConvertTo-Json -Depth 8 | Set-Content "$RunRoot/pair.json" -Encoding utf8
        if ($Role -eq 'host') { Start-Sleep -Seconds 3 }
    }
    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds + 40)
    $LastMemorySample = Get-Date
    'Time,Role,PID,WorkingSet64,PrivateMemorySize64,CPUSeconds' | Set-Content "$RunRoot/process-samples.csv" -Encoding utf8
    while (($Pairs | Where-Object { !$_.Process.HasExited }) -and (Get-Date) -lt $Deadline) {
        $FailedPeer = $false
        foreach ($Pair in $Pairs) {
            $Pair.Process.Refresh()
            if ($Pair.Process.HasExited) {
                $ResultPath = Join-Path $Pair.Output 'result.json'
                if ($Pair.Process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $ResultPath)) { $FailedPeer = $true }
                elseif ((Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json).result -ne 'Pass') { $FailedPeer = $true }
            } elseif (((Get-Date) - $LastMemorySample).TotalSeconds -ge 10) {
                '{0},{1},{2},{3},{4},{5}' -f (Get-Date).ToString('o'),$Pair.Role,$Pair.Process.Id,$Pair.Process.WorkingSet64,$Pair.Process.PrivateMemorySize64,$Pair.Process.TotalProcessorTime.TotalSeconds | Add-Content "$RunRoot/process-samples.csv"
            }
        }
        if (((Get-Date) - $LastMemorySample).TotalSeconds -ge 10) { $LastMemorySample = Get-Date }
        if ($FailedPeer) { $Metadata.Error = 'Peer failed; preserved evidence and stopped dependent peer.'; break }
        Start-Sleep -Milliseconds 500
    }
    $AllPassed = $true
    foreach ($Pair in $Pairs) {
        $ResultPath = Join-Path $Pair.Output 'result.json'
        if (!$Pair.Process.HasExited -or !(Test-Path -LiteralPath $ResultPath)) { $AllPassed = $false; continue }
        $Result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
        if ($Pair.Process.ExitCode -ne 0 -or $Result.result -ne 'Pass') { $AllPassed = $false }
    }
    if ($AllPassed) {
        $HostLog = Get-Content -LiteralPath "$RunRoot/host/engine.log" -Raw
        $ObservedSeeds = @([regex]::Matches($HostLog, 'G[23] match ([A-Fa-f0-9-]+) rules=\S+ seed=(-?\d+)') | ForEach-Object {
            [pscustomobject]@{MatchId=$_.Groups[1].Value; Seed=[int]$_.Groups[2].Value}
        })
        $HostResult = Get-Content -LiteralPath "$RunRoot/host/result.json" -Raw | ConvertFrom-Json
        $SeedChecks = @($HostResult.matches | ForEach-Object {
            $Match = $_
            $Adopted = @($ObservedSeeds | Where-Object { $_.MatchId -eq $Match.matchId })
            [pscustomobject]@{MatchId=$Match.matchId; RequestedSeed=$Match.seed; ObservedSeed=$(if($Adopted.Count -eq 1){$Adopted[0].Seed}else{$null}); Pass=($Adopted.Count -eq 1 -and $Adopted[0].Seed -eq $Match.seed)}
        })
        $Metadata.SeedChecks = $SeedChecks
        if ($SeedChecks.Count -ne $HostResult.completedMatches -or ($SeedChecks | Where-Object { !$_.Pass })) {
            $AllPassed = $false
            $Metadata.Error = 'Recorded requested seed differs from the server adopted seed log.'
        }
        foreach ($Pair in $Pairs) {
            $EngineLog = Get-Content -LiteralPath "$($Pair.Output)/engine.log" -Raw
            if ($EngineLog -notmatch "PktLagMin set to $($RTTMilliseconds / 2)" -or $EngineLog -notmatch "PktLoss set to $PacketLossPercent") {
                $AllPassed = $false
                $Metadata.Error = 'Engine packet simulation settings were not observed in both process logs.'
            }
        }
    }
} catch {
    $AllPassed = $false
    $Metadata.Error = $_.Exception.Message
} finally {
    foreach ($Pair in $Pairs) {
        if (!$Pair.Process.HasExited) {
            $AllPassed = $false
            $Pair.Process.CloseMainWindow() | Out-Null
            if (!$Pair.Process.WaitForExit(5000)) { $Pair.Process.Kill(); $Pair.Process.WaitForExit(5000) | Out-Null }
        }
    }
    $Metadata.Finished = (Get-Date).ToString('o')
    $Metadata.Result = if ($AllPassed) {'Pass'} else {'Fail'}
    $Metadata | ConvertTo-Json -Depth 8 | Set-Content "$RunRoot/pair.json" -Encoding utf8
}
Write-Output "$($Metadata.Result): $RunRoot/pair.json"
if (!$AllPassed) {
    foreach ($Pair in $Pairs) {
        if (Test-Path "$($Pair.Output)/engine.log") {
            Select-String -Path "$($Pair.Output)/engine.log" -Pattern 'LogLDG3Probe|Error:|Fatal error' | Select-Object -Last 12
        }
    }
}
exit $(if ($AllPassed) {0} else {1})
