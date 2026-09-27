<#
.SYNOPSIS
Assemble the six G3 reference lessons in a NEW detached worktree at the canonical G2 base.
.DESCRIPTION
Requires an explicit full reviewed SourceSha. This copies complete reference files in
the A/B lesson dependency order; it does not claim learner implementation or lesson
completion. No merge, commit, cleanup, editor, build, game, process, device or port is used.

Order: A01 battle values/state -> A01 enemies/director -> A02 timeline ->
B03 clock finalization -> A03 battle/result widgets -> B01 entry/return ->
B02 Controller/HUD -> independent expectations -> provided assets and verification.
Whole final files refer to later groups. Build only after the entire assembly succeeds;
the intermediate steps are not independently buildable checkpoints.

Read the G3 lessons in the reference tree, not the replay tree's G2 documentation.
The explicit manifest includes the natural-play and representative-load verification
tools. Every listed file, including the load probe and evidence audit, must exist in the
reviewed SourceSha; an unfinished source is rejected before creating the worktree.
All changed runtime/tool paths must be covered by final-source manifest entries.
Deleted or renamed-away manifest files require a reviewed manifest change; this tool
does not silently remove files or infer new assembly paths from a diff.

For read-only preflight, supply RepositoryRoot, a NEW absolute ReplayRoot, a full
SourceSha, and -ValidateOnly. Omit -ValidateOnly to create and assemble that new path.
An existing path is always rejected. Failed attempts are preserved, never cleaned up.
The manifest is Saved/P0Runs/<RunId>/assembly.json. Replay HEAD remains at the G2 base.
Blob equality proves file assembly only. Pair subsequent build, automation, actual PIE,
package, network, natural-play, load/lifetime and Android results with this manifest.
Neither this script nor a successful preflight marks a lesson Verified.
#>
param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$ReplayRoot,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$SourceSha,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId = ('Replay-G3-assembly-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$BaseSha = 'f735b5889a5bd197e46d29bdfaa2b38c246d5ea6'
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
if (!$GitRoot.Equals($RepositoryRoot.Replace('/', '\'), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'RepositoryRoot must be an existing worktree root.'
}
$ResolvedSource = (Invoke-ReplayGit $RepositoryRoot @('rev-parse', '--verify', "${SourceSha}^{commit}")).Trim()
if ($ResolvedSource -ne $SourceSha) { throw 'SourceSha must be the full commit SHA, not a tag or tree.' }
& git -C $RepositoryRoot merge-base --is-ancestor $BaseSha $SourceSha
if ($LASTEXITCODE -ne 0) { throw 'SourceSha must descend from the canonical G2 base.' }
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
# Complete final files are reference implementations, never records of learner-written code.
# Keep these paths explicit; uncovered changes must cause preflight to fail rather than broaden scope.
$Steps = @(
    @{ Name = '00 provided G2 inputs and unchanged execution tools'; Lesson = 'Common'; Kind = 'Provided'; Sha = $BaseSha; Paths = @(
        'Mobile_defense_clone.uproject', 'Config/DefaultGame.ini', 'Config/DefaultInput.ini',
        'Content/LD/Core/BP_LDGameMode.uasset', 'Content/LD/Materials/M_P0Flat.uasset',
        'Content/LD/Audio/S_P0Rejected.uasset',
        'Content/LD/Data/GameRules.json', 'Content/LD/Data/DT_Units.json', 'Content/LD/Data/DT_EnemyTypes.json',
        'Content/LD/Data/DT_Waves.json', 'Content/LD/Data/DT_SummonProfiles.json', 'Content/LD/Data/DT_SpawnProfiles.json',
        'tools/Build-P0Editor.ps1', 'tools/Sync-P0Data.ps1', 'tools/Test-P0Automation.ps1', 'tools/Run-P0Pair.ps1'
    ) },
    @{ Name = '01 A common battle values and replicated state source'; Lesson = 'A/G3_01_WAVES.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Data/LDBattleTypes.h", "${SourceRoot}Core/LDGameState.h", "${SourceRoot}Core/LDGameState.cpp"
    ) },
    @{ Name = '02 A enemy death time, combat evidence and wave director'; Lesson = 'A/G3_01_WAVES.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Battle/LDEnemyActor.h", "${SourceRoot}Battle/LDEnemyActor.cpp",
        "${SourceRoot}Battle/LDCombatService.h", "${SourceRoot}Battle/LDCombatService.cpp",
        "${SourceRoot}Battle/LDWaveDirector.h", "${SourceRoot}Battle/LDWaveDirector.cpp"
    ) },
    @{ Name = '03 A loading, preparation, timeline and terminal ownership'; Lesson = 'A/G3_02_TIMELINE.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Core/LDGameMode.h", "${SourceRoot}Core/LDGameMode.cpp"
    ) },
    @{ Name = '04 B clock guard, final rewards and completion callback lifetime'; Lesson = 'B/G3-03-clock-finalization.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Network/LDCommandProcessor.h", "${SourceRoot}Network/LDCommandProcessor.cpp"
    ) },
    @{ Name = '05 A battle status and result views'; Lesson = 'A/G3_03_WIDGETS.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}UI/LDBattleStatusWidget.h", "${SourceRoot}UI/LDBattleStatusWidget.cpp",
        "${SourceRoot}UI/LDResultWidget.h", "${SourceRoot}UI/LDResultWidget.cpp"
    ) },
    @{ Name = '06 B entry, address validation, travel and local return'; Lesson = 'B/G3-01-entry-return.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Core/LDGameInstance.h", "${SourceRoot}Core/LDGameInstance.cpp",
        "${SourceRoot}Core/LDEntryGameMode.h", "${SourceRoot}Core/LDEntryGameMode.cpp",
        "${SourceRoot}Core/LDEntryPlayerController.h", "${SourceRoot}Core/LDEntryPlayerController.cpp",
        "${SourceRoot}UI/LDEntryWidget.h", "${SourceRoot}UI/LDEntryWidget.cpp"
    ) },
    @{ Name = '07 B Controller composition, HUD lifecycle and terminal input'; Lesson = 'B/G3-02-battle-result-hud.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Core/LDPlayerController.h", "${SourceRoot}Core/LDPlayerController.cpp",
        "${SourceRoot}UI/LDG1BoardWidget.h", "${SourceRoot}UI/LDG1BoardWidget.cpp",
        "${SourceRoot}UI/LDGameplayWidget.h", "${SourceRoot}UI/LDGameplayWidget.cpp"
    ) },
    @{ Name = '08 A independent wave, deadline and service lifecycle expectations'; Lesson = 'A/G3_01_WAVES.md + A/G3_02_TIMELINE.md'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Tests/LDWaveTests.cpp", "${SourceRoot}Tests/LDLifecycleTests.cpp"
    ) },
    @{ Name = '09 B independent entry, HUD and completion expectations'; Lesson = 'B G3 three lessons'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Tests/LDEntryAndHudTests.cpp"
    ) },
    @{ Name = '10 provided entry and battle maps, config and asset packaging'; Lesson = 'G3_INTEGRATION.md / Unreal settings'; Kind = 'Provided'; Sha = $SourceSha; Paths = @(
        'Config/DefaultEngine.ini', 'Content/LD/Maps/L_P0.umap', 'Content/LD/Maps/L_P0Entry.umap',
        'tools/Create-P0Assets.py', 'tools/Build-P0Package.ps1'
    ) },
    @{ Name = '11 provided actual PIE verification and editor-only module support'; Lesson = 'G3_INTEGRATION.md / PIE'; Kind = 'ProvidedVerification'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Mobile_defense_clone.Build.cs", "${SourceRoot}Tests/LDPieTests.cpp", 'tools/Test-P0PIE.ps1'
    ) },
    @{ Name = '12 provided natural two-process play and evidence audit'; Lesson = 'G3_INTEGRATION.md / packaged network and five seeds'; Kind = 'ProvidedVerification'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Verification/LDG3ProbeSubsystem.h", "${SourceRoot}Verification/LDG3ProbeSubsystem.cpp",
        'tools/Run-P0G3.ps1', 'tools/Test-P0G3Evidence.ps1'
    ) },
    @{ Name = '13 provided explicit representative load and lifetime fixture'; Lesson = 'G3_INTEGRATION.md / representative load'; Kind = 'ProvidedVerification'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Verification/LDG3LoadProbeSubsystem.h", "${SourceRoot}Verification/LDG3LoadProbeSubsystem.cpp",
        'tools/Run-P0Load.ps1'
    ) }
)
$AllPaths = @($Steps | ForEach-Object { $_.Paths })
if (@($AllPaths | Select-Object -Unique).Count -ne $AllPaths.Count) { throw 'Manifest contains duplicate paths.' }
$FinalPaths = @($Steps | Where-Object { $_.Sha -eq $SourceSha } | ForEach-Object { $_.Paths })
$ChangedPaths = @(Invoke-ReplayGit $RepositoryRoot @('-c', 'core.quotePath=false', 'diff', '--name-only', $BaseSha, $SourceSha, '--', 'Source', 'Config', 'Content', 'tools', 'Build', 'Plugins', ':(glob)*.uproject', ':(glob)**/*.uplugin'))
$Uncovered = @($ChangedPaths | Where-Object { $_ -notin $FinalPaths })
if ($Uncovered.Count -gt 0) {
    throw "SourceSha contains runtime/tool changes outside the reviewed G3 lesson manifest. Update the lesson and explicit paths first: $($Uncovered -join ', ')"
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
$CreatedRunRoot = $false
$Manifest = [System.Collections.Generic.List[object]]::new()
$Evidence = [ordered]@{
    Scope = 'Reference file assembly only; not learner implementation or Unreal execution'
    Result = 'Running'; ReferenceMaterialStatus = 'Draft'; LearnerStatus = 'Planned'
    StartedFrom = $BaseSha; SourceSha = $SourceSha; RepositoryRoot = $RepositoryRoot; ProjectRoot = $ReplayRoot
    Created = (Get-Date).ToString('o'); SourceHeadBefore = $SourceHeadBefore; Files = @()
    Build = 'NotRun'; Automation = 'NotRun'; PIE = 'NotRun'; TwoProcess = 'NotRun'; Package = 'NotRun'
    Network = 'NotRun'; NaturalPlay = 'NotRun'; RepresentativeLoad = 'NotRun'; Lifetime = 'NotRun'; Android = 'NotRun'
}
try {
    Invoke-ReplayGit $RepositoryRoot @('worktree', 'add', '--detach', $ReplayRoot, $BaseSha)
    if ((Invoke-ReplayGit $ReplayRoot @('rev-parse', 'HEAD')).Trim() -ne $BaseSha -or
        (Invoke-ReplayGit $ReplayRoot @('branch', '--show-current')) -or
        (Invoke-ReplayGit $ReplayRoot @('status', '--porcelain'))) {
        throw 'The newly created worktree must be clean and detached at the canonical G2 base.'
    }
    if (Test-Path -LiteralPath $RunRoot) { throw 'Evidence path already exists; preserve it and choose a new replay path.' }
    New-Item -ItemType Directory -Path $RunRoot | Out-Null
    $CreatedRunRoot = $true
    foreach ($Step in $Steps) {
        Invoke-ReplayGit $ReplayRoot (@('restore', "--source=$($Step.Sha)", '--worktree', '--') + $Step.Paths)
        foreach ($RelativePath in $Step.Paths) {
            $SourceBlob = (Invoke-ReplayGit $ReplayRoot @('rev-parse', "$($Step.Sha):$RelativePath")).Trim()
            $WorkingPath = Join-Path $ReplayRoot $RelativePath
            $WorkingBlob = (Invoke-ReplayGit $ReplayRoot @('hash-object', "--path=$RelativePath", $WorkingPath)).Trim()
            if ($SourceBlob -ne $WorkingBlob) { throw "Restored file differs: $RelativePath" }
            $Manifest.Add([ordered]@{
                Step = $Step.Name; Lesson = $Step.Lesson; Kind = $Step.Kind; Path = $RelativePath; SourceSha = $Step.Sha
                ExpectedBlob = $SourceBlob; ActualBlob = $WorkingBlob
                WorkingSHA256 = (Get-FileHash -LiteralPath $WorkingPath -Algorithm SHA256).Hash.ToLowerInvariant()
                Result = 'Pass'
            })
        }
        Write-Output "$($Step.Name): $($Step.Paths.Count) files restored and blob-checked."
    }
    $ReplayHead = (Invoke-ReplayGit $ReplayRoot @('rev-parse', 'HEAD')).Trim()
    if ($ReplayHead -ne $BaseSha -or (Invoke-ReplayGit $ReplayRoot @('branch', '--show-current'))) {
        throw 'Replay HEAD must remain detached at G2; no completed branch is merged.'
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
    if ($CreatedRunRoot) {
        $Evidence.Files = $Manifest.ToArray()
        $Evidence.Finished = (Get-Date).ToString('o')
        $Evidence | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $RunRoot 'assembly.json') -Encoding utf8
    }
}
Write-Output "Reference assembly ready: $RunRoot/assembly.json"
Write-Output 'No build/editor/game was launched. Existing worktrees and branches are preserved; no cleanup or commit is performed.'
Write-Output 'Replay HEAD remains at the G2 base. Pair every later verification result with this assembly manifest and SourceSha.'
