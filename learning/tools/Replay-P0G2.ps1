<#
.SYNOPSIS
Assemble six G2 reference lessons in a NEW detached worktree at the canonical G1 base.
.DESCRIPTION
Requires a full reviewed SourceSha with no default. This restores explicit reference
files to verify lesson dependencies, not learner implementation or lesson completion.
No merge, commit, cleanup, editor, build, game or port is used.

Interleaved order: B01 contracts -> A01 prepared unit -> A02 combat/death ->
B01 board/economy -> B02 atomic commands/envelope -> A03 match -> B03 HUD -> checks.
Controller is a complete final file at step05: its input methods belong to B03 and
require the later widgets. This is one combined assembly, not intermediate builds.
Tests/LDCombatTests includes A01/A02; LDGameplayCommandTests includes B01/B02;
LDLifecycleTests covers A03 integration. Each lesson keeps its own observation limits.

Read lessons in the reference tree, not the replay tree's G1 documentation.
For preflight, provide RepositoryRoot, a NEW absolute ReplayRoot, a reviewed full
SourceSha, and -ValidateOnly. Omit -ValidateOnly only to create and assemble the
new worktree. An existing path is always rejected and no cleanup is performed.
The manifest is Saved/P0Runs/<RunId>/assembly.json; replay HEAD stays at G1.
Pair subsequent Editor, automation and real pair results with this manifest and SHA.
Blob equality does not prove actual gameplay, whole-match balance or Android.
#>
param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$ReplayRoot,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$SourceSha,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId = ('Replay-G2-assembly-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$BaseSha = '4861b987f3e2fe78bcc159d1b6a85008543a938b'
$SourceSha = $SourceSha.ToLowerInvariant()
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path.TrimEnd('\', '/')
if (![IO.Path]::IsPathFullyQualified($ReplayRoot)) { throw 'ReplayRoot must be a new absolute path.' }
$ReplayRoot = [IO.Path]::GetFullPath($ReplayRoot).TrimEnd('\', '/')
if (Test-Path -LiteralPath $ReplayRoot) { throw 'ReplayRoot already exists. Preserve it and choose a new path.' }
if (!(Test-Path -LiteralPath (Split-Path $ReplayRoot -Parent) -PathType Container)) {
    throw 'The parent of ReplayRoot must already exist.'
}

function Invoke-ReplayGit {
    param([string]$Root, [string[]]$GitArguments)
    $Output = & git -C $Root @GitArguments
    if ($LASTEXITCODE -ne 0) { throw "git failed: $($GitArguments -join ' ')" }
    return $Output
}

$GitRoot = (Invoke-ReplayGit $RepositoryRoot @('rev-parse', '--show-toplevel')).Trim().Replace('/', '\').TrimEnd('\')
if ($GitRoot -ne $RepositoryRoot.Replace('/', '\')) { throw 'RepositoryRoot must be an existing worktree root.' }
$ResolvedSource = (Invoke-ReplayGit $RepositoryRoot @('rev-parse', '--verify', "${SourceSha}^{commit}")).Trim()
if ($ResolvedSource -ne $SourceSha) { throw 'SourceSha must be the full commit SHA, not a tag or tree.' }
& git -C $RepositoryRoot merge-base --is-ancestor $BaseSha $SourceSha
if ($LASTEXITCODE -ne 0) { throw 'SourceSha must descend from the canonical G1 base.' }
$SourceHeadBefore = (Invoke-ReplayGit $RepositoryRoot @('rev-parse', 'HEAD')).Trim()
foreach ($Line in (Invoke-ReplayGit $RepositoryRoot @('worktree', 'list', '--porcelain'))) {
    if ($Line.StartsWith('worktree ')) {
        $ExistingRoot = [IO.Path]::GetFullPath($Line.Substring(9)).TrimEnd('\', '/')
        if ($ReplayRoot.Equals($ExistingRoot, [StringComparison]::OrdinalIgnoreCase) -or
            $ReplayRoot.StartsWith($ExistingRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'ReplayRoot must be outside every existing worktree.'
        }
    }
}

$SourceRoot = 'Source/Mobile_defense_clone/'
# These are complete files from the final integrated source. Assembly is reference reproduction,
# not a claim that the learner wrote the code. Compile only after every step has been assembled.
$Steps = @(
    @{ Name = '00 provided G1 inputs and execution tools'; Lesson = 'Common'; Kind = 'Provided'; Sha = $BaseSha; Paths = @(
        'Mobile_defense_clone.uproject',
        'Config/DefaultEngine.ini', 'Config/DefaultGame.ini', 'Config/DefaultInput.ini',
        'Content/LD/Core/BP_LDGameMode.uasset', 'Content/LD/Maps/L_P0.umap', 'Content/LD/Materials/M_P0Flat.uasset',
        'Content/LD/Data/GameRules.json', 'Content/LD/Data/DT_Units.json', 'Content/LD/Data/DT_EnemyTypes.json',
        'Content/LD/Data/DT_Waves.json', 'Content/LD/Data/DT_SummonProfiles.json', 'Content/LD/Data/DT_SpawnProfiles.json',
        'tools/Build-P0Editor.ps1', 'tools/Build-P0Package.ps1', 'tools/Test-P0Automation.ps1', 'tools/Sync-P0Data.ps1'
    ) },
    @{ Name = '01 B board/economy value contracts before actor'; Lesson = 'B/G2-01-board-economy.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Board/LDBoardTypes.h", "${SourceRoot}Economy/LDEconomyTypes.h"
    ) },
    @{ Name = '02 A prepared unit, committed placement and presentation'; Lesson = 'A/G2_01_UNIT.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Battle/LDUnitActor.h", "${SourceRoot}Battle/LDUnitActor.cpp"
    ) },
    @{ Name = '03 A damage/death contracts, exact positions and attack clock'; Lesson = 'A/G2_02_COMBAT.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Battle/LDCombatEvents.h", "${SourceRoot}Battle/LDCombatRules.h", "${SourceRoot}Battle/LDCombatRules.cpp",
        "${SourceRoot}Battle/LDRouteModel.h", "${SourceRoot}Battle/LDRouteModel.cpp",
        "${SourceRoot}Battle/LDEnemyActor.h", "${SourceRoot}Battle/LDEnemyActor.cpp",
        "${SourceRoot}Battle/LDCombatService.h", "${SourceRoot}Battle/LDCombatService.cpp"
    ) },
    @{ Name = '04 B board preparation, stack operations and economy source'; Lesson = 'B/G2-01-board-economy.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Board/LDBoardManager.h", "${SourceRoot}Board/LDBoardManager.cpp",
        "${SourceRoot}Economy/LDEconomyService.h", "${SourceRoot}Economy/LDEconomyService.cpp"
    ) },
    @{ Name = '05 B atomic commands, deduplication, rewards and owner envelope'; Lesson = 'B/G2-02-command-lifetime.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Network/LDCommandProcessor.h", "${SourceRoot}Network/LDCommandProcessor.cpp",
        "${SourceRoot}Core/LDPlayerController.h", "${SourceRoot}Core/LDPlayerController.cpp"
    ) },
    @{ Name = '06 A match ownership, subscriptions, command clock and shutdown'; Lesson = 'A/G2_03_MATCH.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Core/LDGameMode.h", "${SourceRoot}Core/LDGameMode.cpp"
    ) },
    @{ Name = '07 B HUD, selection, input and uncertain response'; Lesson = 'B/G2-03-hud-input.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}UI/LDG1BoardWidget.h", "${SourceRoot}UI/LDG1BoardWidget.cpp",
        "${SourceRoot}UI/LDGameplayWidget.h", "${SourceRoot}UI/LDGameplayWidget.cpp",
        "${SourceRoot}Board/LDBoardPresentation.h", "${SourceRoot}Board/LDBoardPresentation.cpp"
    ) },
    @{ Name = '08 normal local unit presentation and independent expectations'; Lesson = 'A/B six lessons and Integration'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Core/LDLocalPresentationSubsystem.cpp",
        "${SourceRoot}Tests/LDCombatTests.cpp", "${SourceRoot}Tests/LDGameplayCommandTests.cpp",
        "${SourceRoot}Tests/LDLifecycleTests.cpp"
    ) },
    @{ Name = '09 provided rejection audio and deterministic fixture helpers'; Lesson = 'Common / B/G2-03-hud-input.md'; Kind = 'Provided'; Sha = $SourceSha; Paths = @(
        'Content/LD/Audio/S_P0Rejected.uasset', 'tools/Create-P0FeedbackAudio.py', 'tools/Find-P0FixtureSeed.mjs'
    ) },
    @{ Name = '10 provided explicit Development two-process fixture'; Lesson = 'Integration'; Kind = 'ProvidedVerification'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Verification/LDG2ProbeSubsystem.h", "${SourceRoot}Verification/LDG2ProbeSubsystem.cpp",
        'tools/Run-P0Pair.ps1'
    ) }
)
$AllPaths = @($Steps | ForEach-Object { $_.Paths })
if (@($AllPaths | Select-Object -Unique).Count -ne $AllPaths.Count) { throw 'Manifest contains duplicate paths.' }
$FinalPaths = @($Steps | Where-Object { $_.Sha -eq $SourceSha } | ForEach-Object { $_.Paths })
$ChangedPaths = @(Invoke-ReplayGit $RepositoryRoot @('-c', 'core.quotePath=false', 'diff', '--name-only', $BaseSha, $SourceSha, '--', 'Source', 'Config', 'Content', 'tools', 'Build', 'Plugins', ':(glob)*.uproject', ':(glob)**/*.uplugin'))
$Uncovered = @($ChangedPaths | Where-Object { $_ -notin $FinalPaths })
if ($Uncovered.Count -gt 0) {
    throw "SourceSha contains runtime/tool changes outside the reviewed G2 lesson manifest. Update the lesson and explicit paths first: $($Uncovered -join ', ')"
}
foreach ($Step in $Steps) {
    foreach ($RelativePath in $Step.Paths) {
        $ObjectType = (Invoke-ReplayGit $RepositoryRoot @('cat-file', '-t', "$($Step.Sha):$RelativePath")).Trim()
        if ($ObjectType -ne 'blob') { throw "Expected a source file: $RelativePath" }
    }
}
if ($ValidateOnly) {
    Write-Output "Preflight Pass: base=$BaseSha source=$SourceSha; $($ChangedPaths.Count) changed paths covered; $($AllPaths.Count) manifest files; no worktree created."
    return
}

$RunRoot = Join-Path $ReplayRoot "Saved/P0Runs/$RunId"
$Manifest = [System.Collections.Generic.List[object]]::new()
$Evidence = [ordered]@{
    Scope = 'Reference file assembly only; not learner implementation or Unreal execution'
    Result = 'Running'; ReferenceMaterialStatus = 'Draft'; LearnerStatus = 'Planned'
    StartedFrom = $BaseSha; SourceSha = $SourceSha; RepositoryRoot = $RepositoryRoot; ProjectRoot = $ReplayRoot
    Created = (Get-Date).ToString('o'); SourceHeadBefore = $SourceHeadBefore; Files = @()
    Build = 'NotRun'; Automation = 'NotRun'; PIE = 'NotRun'; TwoProcess = 'NotRun'; Package = 'NotRun'; Android = 'NotRun'
}
try {
    Invoke-ReplayGit $RepositoryRoot @('worktree', 'add', '--detach', $ReplayRoot, $BaseSha)
    New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null
    if ((Invoke-ReplayGit $ReplayRoot @('rev-parse', 'HEAD')).Trim() -ne $BaseSha -or
        (Invoke-ReplayGit $ReplayRoot @('branch', '--show-current')) -or
        (Invoke-ReplayGit $ReplayRoot @('status', '--porcelain'))) {
        throw 'The newly created worktree must be clean and detached at the canonical G1 base.'
    }
    foreach ($Step in $Steps) {
        Invoke-ReplayGit $ReplayRoot (@('restore', "--source=$($Step.Sha)", '--worktree', '--') + $Step.Paths)
        foreach ($RelativePath in $Step.Paths) {
            $SourceBlob = (Invoke-ReplayGit $ReplayRoot @('rev-parse', "$($Step.Sha):$RelativePath")).Trim()
            $WorkingBlob = (Invoke-ReplayGit $ReplayRoot @('hash-object', "--path=$RelativePath", (Join-Path $ReplayRoot $RelativePath))).Trim()
            if ($SourceBlob -ne $WorkingBlob) { throw "Restored file differs: $RelativePath" }
            $Manifest.Add([ordered]@{
                Step = $Step.Name; Lesson = $Step.Lesson; Kind = $Step.Kind; Path = $RelativePath; SourceSha = $Step.Sha
                ExpectedBlob = $SourceBlob; ActualBlob = $WorkingBlob; Result = 'Pass'
            })
        }
        Write-Output "$($Step.Name): $($Step.Paths.Count) files restored and blob-checked."
    }
    $ReplayHead = (Invoke-ReplayGit $ReplayRoot @('rev-parse', 'HEAD')).Trim()
    if ($ReplayHead -ne $BaseSha -or (Invoke-ReplayGit $ReplayRoot @('branch', '--show-current'))) {
        throw 'Replay HEAD must remain detached at G1; no completed branch is merged.'
    }
    if (Invoke-ReplayGit $ReplayRoot @('diff', '--cached', '--name-only')) {
        throw 'Replay assembly must not stage files.'
    }
    $Evidence.Result = 'Pass'
    $Evidence.ReplayHead = $ReplayHead
    $Evidence.SourceHeadAfter = (Invoke-ReplayGit $RepositoryRoot @('rev-parse', 'HEAD')).Trim()
    $Evidence.ChangedPathsCovered = $ChangedPaths
    Invoke-ReplayGit $ReplayRoot @('status', '--short') | Set-Content -LiteralPath (Join-Path $RunRoot 'status.txt') -Encoding utf8
}
catch {
    $Evidence.Result = 'Fail'
    $Evidence.Error = $_.Exception.Message
    throw
}
finally {
    if (Test-Path -LiteralPath $RunRoot) {
        $Evidence.Files = $Manifest.ToArray()
        $Evidence.Finished = (Get-Date).ToString('o')
        $Evidence | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $RunRoot 'assembly.json') -Encoding utf8
    }
}
Write-Output "Reference assembly ready: $RunRoot/assembly.json"
Write-Output 'No build/editor/game was launched. All existing worktrees and branches are preserved; no cleanup or commit is performed.'
Write-Output 'The replay HEAD remains G1. Pair every subsequent build/test result with this assembly manifest and SourceSha.'

