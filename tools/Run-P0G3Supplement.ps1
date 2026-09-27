param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [ValidateSet('G3Boundary','G3NetConflict','G3Entry')][string]$Probe = 'G3Boundary',
    [string]$GameExecutable = '',
    [int]$TimeoutSeconds = 600,
    [int]$Port = 17777,
    [int]$Width = 540,
    [int]$Height = 1170,
    [switch]$RenderOffscreen,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$' -or $TimeoutSeconds -lt 300 -or $Port -lt 1024 -or $Port -gt 65400 -or $Width -lt 320 -or $Height -lt 320) { throw 'Invalid run, duration, viewport or port.' }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Existing evidence is preserved. Choose a new RunId.' }
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw 'Game port is already in use.' }
$UsingEditor = !$GameExecutable
if ($UsingEditor) { $GameExecutable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
if (!(Test-Path -LiteralPath $GameExecutable)) { throw 'Executable is missing.' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Record = [ordered]@{
    Scope = $(if($UsingEditor){'Actual GPU two-process Editor-game supplement; not packaged'}else{'Actual GPU two-process packaged supplement'})
    Probe = $Probe
    Started = (Get-Date).ToString('o'); Head = (& git -C $ProjectRoot rev-parse HEAD)
    RunnerSHA256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
    Executable = $GameExecutable; ExecutableSHA256 = (Get-FileHash -LiteralPath $GameExecutable -Algorithm SHA256).Hash
    EditorModuleSHA256 = $(if($UsingEditor){(Get-FileHash -LiteralPath (Join-Path $ProjectRoot 'Binaries/Win64/UnrealEditor-Mobile_defense_clone.dll') -Algorithm SHA256).Hash}else{$null})
    ExpectedCases = $(if($Probe -eq 'G3Boundary'){4}else{0}); Resolution = @($Width,$Height); MaxFPS = 60; RenderOffscreen = [bool]$RenderOffscreen
    Fixture = 'Explicit Development supplement. Each role result records its exact setup and observations. Not natural balance or physical input.'
    Processes = @(); Result = 'Running'
}
$Pairs = @()
$AllPassed = $false
try {
    foreach($Role in @('host','client')) {
        $Output = Join-Path $RunRoot $Role
        New-Item -ItemType Directory -Path $Output | Out-Null
        $Arguments = @()
        if($UsingEditor) { $Arguments += (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject') }
        if($Probe -eq 'G3NetConflict') {
            $Arguments += $(if($Role -eq 'host'){'/Game/LD/Maps/L_P0?listen'}else{"127.0.0.1:$Port"})
        } else {
            $Arguments += '/Game/LD/Maps/L_P0Entry'
        }
        if($UsingEditor) { $Arguments += '-game' }
        $ToolPort = $Port + $(if($Role -eq 'host'){100}else{101})
        if(Get-NetTCPConnection -LocalPort $ToolPort -State Listen -ErrorAction SilentlyContinue) { throw 'Tool port is already in use.' }
        $Arguments += @("-ModelContextProtocolPort=$ToolPort",'-windowed','-ForceRes',"-ResX=$Width","-ResY=$Height", "-port=$Port",'-nosplash','-nosound','-unattended','-culture=ko',"-P0Probe=$Probe", "-P0Role=$Role", "-P0PeerAddress=127.0.0.1:$Port", "-P0ProbeOutput=$Output", "-P0TimeoutSeconds=$TimeoutSeconds",'-P0Seed=1776','-P0CommandTrace',"-abslog=$Output/engine.log",'-ExecCmds=t.IdleWhenNotForeground 0,t.MaxFPS 60,r.VSync 0')
        $Arguments += "-P0EntryRunId=$RunId"
        if($RenderOffscreen) { $Arguments += '-RenderOffscreen' }
        $Quoted = ($Arguments | ForEach-Object { '"' + $_.Replace('"','\"') + '"' }) -join ' '
        $Process = Start-Process -FilePath $GameExecutable -ArgumentList $Quoted -PassThru -WindowStyle Hidden -RedirectStandardOutput "$Output/stdout.log" -RedirectStandardError "$Output/stderr.log"
        $Pairs += [pscustomobject]@{Role=$Role;Process=$Process;Output=$Output;Arguments=$Arguments}
        $Record.Processes = @($Pairs | ForEach-Object { @{Role=$_.Role;PID=$_.Process.Id;Arguments=$_.Arguments} })
        $Record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$RunRoot/pair.json" -Encoding utf8
        if($Role -eq 'host') { Start-Sleep -Seconds 3 }
    }
    $Deadline = (Get-Date).AddSeconds($TimeoutSeconds + 40)
    while(($Pairs | Where-Object { !$_.Process.HasExited }) -and (Get-Date) -lt $Deadline) {
        $PeerFailed = $false
        foreach($Pair in $Pairs) {
            $Pair.Process.Refresh()
            if($Pair.Process.HasExited) {
                $ResultPath = Join-Path $Pair.Output 'result.json'
                if($Pair.Process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $ResultPath)) { $PeerFailed = $true }
                elseif((Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json).result -ne 'Pass') { $PeerFailed = $true }
            }
        }
        if($PeerFailed) { throw 'Peer failed; inspect preserved role evidence.' }
        Start-Sleep -Milliseconds 500
    }
    $AllPassed = $true
    $RoleResults = @()
    $Evidence = @{}
    foreach($Pair in $Pairs) {
        if(!$Pair.Process.HasExited) { throw 'Supplement run timed out.' }
        $Result = Get-Content -LiteralPath "$($Pair.Output)/result.json" -Raw | ConvertFrom-Json
        $Evidence[$Pair.Role] = $Result
        $Log = Get-Content -LiteralPath "$($Pair.Output)/engine.log" -Raw
        $Cases = @(if($null -ne $Result.cases){ $Result.cases })
        $FailedChecks = @($Result.checks | Where-Object { $_.pass -ne $true })
        $CriticalLog = $Log -match 'Fatal error:|Assertion failed:|Ensure condition failed:|=== Critical error:'
        $CaseCountValid = $Record.ExpectedCases -eq 0 -or $Cases.Count -eq $Record.ExpectedCases
        $RequiredImages = @()
        if($Probe -eq 'G3Boundary') {
            $RequiredImages = @('case0-ready.png','case0-result.png','case1-ready.png','case1-result.png','case2-ready.png','case2-result.png','case3-ready.png','case3-result.png','case2-normal-wait.png')
        } elseif($Probe -eq 'G3Entry') {
            $RequiredImages = @('entry-initial.png','entry-idle35.png','late-terminal.png','entry-returned.png')
            if($Pair.Role -eq 'host') { $RequiredImages += 'loading-timeout.png' }
        }
        $MissingImages = @($RequiredImages | Where-Object { !(Test-Path -LiteralPath (Join-Path $Pair.Output $_)) })
        $RolePass = $Pair.Process.ExitCode -eq 0 -and $Result.result -eq 'Pass' -and $CaseCountValid -and $Result.checks.Count -gt 0 -and $FailedChecks.Count -eq 0 -and !$CriticalLog -and $MissingImages.Count -eq 0
        $RoleResults += [pscustomobject]@{Role=$Pair.Role;ExitCode=$Pair.Process.ExitCode;Cases=$Cases.Count;Checks=$Result.checks.Count;FailedChecks=$FailedChecks.Count;CriticalLog=$CriticalLog;RequiredImages=$RequiredImages.Count;MissingImages=$MissingImages;Pass=$RolePass}
        $AllPassed = $AllPassed -and $RolePass
    }
    $PairConsistent = $false
    if($Probe -eq 'G3Boundary') {
        $PairConsistent = $Evidence.host.completedCases -eq 4 -and $Evidence.client.completedCases -eq 4 -and $Evidence.host.returns -eq 3 -and $Evidence.client.returns -eq 3 -and @($Evidence.host.cases.matchId | Select-Object -Unique).Count -eq 4
        for($Index=0; $Index -lt 4; $Index++) {
            $HostCase = $Evidence.host.cases[$Index]
            $ClientCase = $Evidence.client.cases[$Index]
            $PairConsistent = $PairConsistent -and $HostCase.caseIndex -eq $Index -and $ClientCase.caseIndex -eq $Index -and $HostCase.matchId -eq $ClientCase.matchId -and $HostCase.battle -eq $ClientCase.battle
        }
    } elseif($Probe -eq 'G3Entry') {
        $PairConsistent = $Evidence.host.runId -eq $RunId -and $Evidence.client.runId -eq $RunId -and $Evidence.host.terminalView.matchId -and $Evidence.host.terminalView.matchId -eq $Evidence.client.terminalView.matchId -and $Evidence.host.terminalView.revision -eq $Evidence.client.terminalView.revision
    } else {
        $PairConsistent = $Evidence.host.matchId -and $Evidence.host.matchId -eq $Evidence.client.matchId
    }
    $Record.PairStateConsistent = [bool]$PairConsistent
    $AllPassed = $AllPassed -and $PairConsistent
    $Record.RoleResults = $RoleResults
} catch {
    $AllPassed = $false
    $Record.Error = $_.Exception.Message
} finally {
    foreach($Pair in $Pairs) {
        if(!$Pair.Process.HasExited) {
            $AllPassed = $false
            $Pair.Process.CloseMainWindow() | Out-Null
            if(!$Pair.Process.WaitForExit(5000)) { $Pair.Process.Kill(); $Pair.Process.WaitForExit(5000) | Out-Null }
        }
    }
    $Record.Finished = (Get-Date).ToString('o')
    $Record.Result = $(if($AllPassed){'Pass'}else{'Fail'})
    $Record | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$RunRoot/pair.json" -Encoding utf8
}
Write-Output "$($Record.Result): $RunRoot/pair.json"
exit $(if($AllPassed){0}else{1})
