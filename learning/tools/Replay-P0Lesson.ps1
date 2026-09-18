param(
    [Parameter(Mandatory = $true)][string]$ProjectRoot,
    [Parameter(Mandatory = $true)][ValidateSet('A', 'B')][string]$Role,
    [string]$RunId = ('Replay-G0-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
)
$ErrorActionPreference = 'Stop'
$BaseSha = '8c6856d235de87cc28c12b49ca775bd0937334a5'
$ProvidedSha = '81ba665adff6c60ef95f79319b3115050937a1c1'
$SourceSha = if ($Role -eq 'A') {
    '4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6'
} else {
    '03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92'
}
$ReplayRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\', '/')

function Invoke-ReplayGit {
    param([string[]]$GitArguments)
    $Output = & git -C $ReplayRoot @GitArguments
    if ($LASTEXITCODE -ne 0) { throw "git failed: $($GitArguments -join ' ')" }
    return $Output
}

$GitRoot = (Invoke-ReplayGit @('rev-parse', '--show-toplevel')).Trim().Replace('/', '\').TrimEnd('\')
if ($GitRoot -ne $ReplayRoot.Replace('/', '\')) { throw 'ProjectRoot must be the worktree root.' }
if ((Invoke-ReplayGit @('rev-parse', 'HEAD')).Trim() -ne $BaseSha) { throw 'Replay requires the exact common base SHA.' }
if ((Invoke-ReplayGit @('branch', '--show-current'))) { throw 'Use a separate detached worktree, never a learn/reference branch.' }
if ((Invoke-ReplayGit @('status', '--porcelain'))) { throw 'Replay requires a clean worktree; preserve existing changes elsewhere.' }
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'RunId must be a simple directory name.' }
$RunRoot = Join-Path $ReplayRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'This RunId exists; choose a new RunId to preserve its evidence.' }
New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null
$Manifest = [System.Collections.Generic.List[object]]::new()
$SourceRoot = 'Source/Mobile_defense_clone/'

function Restore-ReplayStep {
    param([string]$Step, [string]$Sha, [string[]]$Paths)
    Invoke-ReplayGit (@('restore', "--source=$Sha", '--worktree', '--') + $Paths)
    foreach ($RelativePath in $Paths) {
        $SourceBlob = (Invoke-ReplayGit @('rev-parse', "${Sha}:$RelativePath")).Trim()
        $WorkingBlob = (Invoke-ReplayGit @('hash-object', "--path=$RelativePath", (Join-Path $ReplayRoot $RelativePath))).Trim()
        if ($WorkingBlob -ne $SourceBlob) { throw "Restored file differs: $RelativePath" }
        $Manifest.Add([ordered]@{ Step = $Step; Path = $RelativePath; SourceSha = $Sha; Blob = $SourceBlob })
    }
    Write-Output "$Step : $($Paths.Count) files restored and blob-checked."
}

Restore-ReplayStep 'Provided tools, UFS configuration and canonical six JSON files' $ProvidedSha @(
    'tools/Sync-P0Data.ps1', 'tools/Build-P0Editor.ps1', 'tools/Test-P0Automation.ps1',
    'Config/DefaultGame.ini', 'Content/LD/Data/GameRules.json', 'Content/LD/Data/DT_Units.json',
    'Content/LD/Data/DT_EnemyTypes.json', 'Content/LD/Data/DT_Waves.json',
    'Content/LD/Data/DT_SummonProfiles.json', 'Content/LD/Data/DT_SpawnProfiles.json'
)
Restore-ReplayStep '01 shared identity and module dependencies' $SourceSha @(
    "${SourceRoot}Data/LDMatchTypes.h", "${SourceRoot}Mobile_defense_clone.Build.cs"
)
Restore-ReplayStep '02 role-specific data declarations and loader' $SourceSha @(
    "${SourceRoot}Data/LDGameData.h", "${SourceRoot}Data/LDGameData.cpp"
)
if ($Role -eq 'B') {
    Restore-ReplayStep '03 B command value types and server admission' $SourceSha @(
        "${SourceRoot}Network/LDCommandTypes.h", "${SourceRoot}Network/LDCommandTypes.cpp",
        "${SourceRoot}Network/LDCommandProcessor.h", "${SourceRoot}Network/LDCommandProcessor.cpp"
    )
}
Restore-ReplayStep '04 common state and participant state' $SourceSha @(
    "${SourceRoot}Core/LDGameState.h", "${SourceRoot}Core/LDGameState.cpp",
    "${SourceRoot}Core/LDPlayerState.h", "${SourceRoot}Core/LDPlayerState.cpp"
)
if ($Role -eq 'B') {
    Restore-ReplayStep '05 B owned command RPC controller' $SourceSha @(
        "${SourceRoot}Core/LDPlayerController.h", "${SourceRoot}Core/LDPlayerController.cpp"
    )
}
Restore-ReplayStep '06 role-specific composition and lifecycle' $SourceSha @(
    "${SourceRoot}Core/LDGameMode.h", "${SourceRoot}Core/LDGameMode.cpp"
)
$TestFile = if ($Role -eq 'A') { 'Tests/LDDataTests.cpp' } else { 'Tests/LDCommandTests.cpp' }
Restore-ReplayStep '07 role-specific automation expectations' $SourceSha @("$SourceRoot$TestFile")
& (Join-Path $ReplayRoot 'tools/Sync-P0Data.ps1') -Check
if ((Invoke-ReplayGit @('rev-parse', 'HEAD')).Trim() -ne $BaseSha) { throw 'Replay unexpectedly changed HEAD.' }
$Evidence = [ordered]@{
    Scope = 'Reference file assembly only; not learner implementation or build/gameplay validation'
    Role = $Role; StartedFrom = $BaseSha; ProvidedFrom = $ProvidedSha; RoleSourceFrom = $SourceSha
    ProjectRoot = $ReplayRoot; Created = (Get-Date).ToString('o'); Files = $Manifest.ToArray()
    Build = 'NotRun'; Automation = 'NotRun'; PIE = 'NotRun'; Package = 'NotRun'; Android = 'NotRun'
}
$Evidence | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $RunRoot 'assembly.json') -Encoding utf8
Invoke-ReplayGit @('status', '--short') | Set-Content -LiteralPath (Join-Path $RunRoot 'status.txt') -Encoding utf8
Write-Output "Reference assembly ready: $RunRoot/assembly.json"
Write-Output 'HEAD and all branches are unchanged. Follow COMMON.md for serial Editor build and filtered automation.'
