param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$GameExecutable = '',
    [ValidateRange(10,7200)][int]$LoadSeconds = 1200,
    [int]$Port = 18777,
    [switch]$RenderOffscreen,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a new simple RunId.' }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Previous evidence is preserved.' }
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw 'Game port is in use.' }
$UsingEditor = !$GameExecutable
if ($UsingEditor) { $GameExecutable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe' }
if (!(Test-Path -LiteralPath $GameExecutable)) { throw 'Executable missing.' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Record = [ordered]@{Scope='Explicit Development 40 units + 99 normals + 2 bosses workload, then 2000 lifecycles. Not natural-play or balance evidence.';EditorGame=$UsingEditor;Source=(& git -C $ProjectRoot rev-parse HEAD);Executable=$GameExecutable;ExecutableSHA256=(Get-FileHash -LiteralPath $GameExecutable -Algorithm SHA256).Hash;LoadSeconds=$LoadSeconds;Resolution=@(540,1170);FPSLimit=60;VSync=0;RenderOffscreen=[bool]$RenderOffscreen;Started=(Get-Date).ToString('o');Processes=@();Result='Running'}
$Pairs = @()
try {
    foreach ($Role in @('host','client')) {
        $Output = Join-Path $RunRoot $Role
        New-Item -ItemType Directory -Path $Output | Out-Null
        $Arguments = @()
        if ($UsingEditor) { $Arguments += (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject') }
        $Arguments += $(if($Role -eq 'host') {'/Game/LD/Maps/L_P0?listen'} else {"127.0.0.1:$Port"})
        if ($UsingEditor) { $Arguments += '-game' }
        $ToolPort = $Port + $(if($Role -eq 'host'){100}else{101})
        if (Get-NetTCPConnection -LocalPort $ToolPort -State Listen -ErrorAction SilentlyContinue) { throw 'Tool port occupied.' }
        $Arguments += @('-windowed','-ForceRes','-ResX=540','-ResY=1170','-nosound','-nosplash','-unattended','-culture=ko',"-port=$Port",'-P0Probe=G3Load',"-P0LoadSeconds=$LoadSeconds",'-P0Seed=1776',"-P0ProbeOutput=$Output", "-abslog=$Output/engine.log", "-ModelContextProtocolPort=$ToolPort", '-csvGpuStats', '-csvCompression=0', '-ExecCmds=t.IdleWhenNotForeground 0,t.MaxFPS 60,r.VSync 0')
        if ($RenderOffscreen) {$Arguments += '-RenderOffscreen'}
        $Quoted = ($Arguments | ForEach-Object {'"'+$_.Replace('"','\"')+'"'}) -join ' '
        $Process = Start-Process -FilePath $GameExecutable -ArgumentList $Quoted -PassThru -WindowStyle Hidden -RedirectStandardOutput "$Output/stdout.log" -RedirectStandardError "$Output/stderr.log"
        $Pairs += [pscustomobject]@{Role=$Role;Process=$Process;Output=$Output;Arguments=$Arguments}
        $Record.Processes = @($Pairs | ForEach-Object {@{Role=$_.Role;Id=$_.Process.Id;Arguments=$_.Arguments}})
        $Record | ConvertTo-Json -Depth 6 | Set-Content "$RunRoot/pair.json" -Encoding utf8
        if ($Role -eq 'host') {Start-Sleep -Seconds 3}
    }
    $Deadline = (Get-Date).AddSeconds($LoadSeconds+1850)
    while (($Pairs | Where-Object {!$_.Process.HasExited}) -and (Get-Date) -lt $Deadline) {
        foreach ($Pair in $Pairs) {
            if ($Pair.Process.HasExited -and (!(Test-Path "$($Pair.Output)/result.json") -or $Pair.Process.ExitCode -ne 0)) {throw "$($Pair.Role) exited without valid result."}
            if ((Test-Path "$($Pair.Output)/result.json") -and (Get-Content "$($Pair.Output)/result.json" -Raw|ConvertFrom-Json).result -eq 'Fail') {throw "$($Pair.Role) fixture checks failed."}
        }
        Start-Sleep -Milliseconds 500
    }
    foreach ($Pair in $Pairs) {
        if (!$Pair.Process.HasExited) {throw 'Fixture timed out.'}
        $Result = Get-Content "$($Pair.Output)/result.json" -Raw | ConvertFrom-Json
        if ($Result.result -ne 'Pass' -or $Pair.Process.ExitCode -ne 0) {throw "$($Pair.Role) failed."}
    }
    $Record.Result='Pass'
} catch { $Record.Result='Fail'; $Record.Error=$_.Exception.Message }
finally {
    foreach($Pair in $Pairs) {if(!$Pair.Process.HasExited){$Pair.Process.CloseMainWindow()|Out-Null; if(!$Pair.Process.WaitForExit(5000)){$Pair.Process.Kill();$Pair.Process.WaitForExit()}}}
    $Record.Finished=(Get-Date).ToString('o')
    $Record|ConvertTo-Json -Depth 6|Set-Content "$RunRoot/pair.json" -Encoding utf8
}
Write-Output "$($Record.Result): $RunRoot/pair.json"
if($Record.Result -ne 'Pass'){exit 1}
