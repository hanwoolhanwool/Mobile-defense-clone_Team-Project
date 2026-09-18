param(
    [Parameter(Mandatory = $true)][string]$RepositoryRoot,
    [Parameter(Mandatory = $true)][string]$ReplayRoot,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$SourceSha,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId = ('Replay-G1-assembly-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$BaseSha = '649c1dedd6832c41089a76b59bc76518cd262296'
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
if ($LASTEXITCODE -ne 0) { throw 'SourceSha must descend from the canonical G0 base.' }
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
    @{ Name = '00 G0 provided data, map, material, configuration and build tools'; Kind = 'Provided'; Sha = $BaseSha; Paths = @(
        'Config/DefaultEngine.ini', 'Config/DefaultGame.ini', 'Config/DefaultInput.ini',
        'Content/LD/Core/BP_LDGameMode.uasset', 'Content/LD/Maps/L_P0.umap', 'Content/LD/Materials/M_P0Flat.uasset',
        'Content/LD/Data/GameRules.json', 'Content/LD/Data/DT_Units.json', 'Content/LD/Data/DT_EnemyTypes.json',
        'Content/LD/Data/DT_Waves.json', 'Content/LD/Data/DT_SummonProfiles.json', 'Content/LD/Data/DT_SpawnProfiles.json',
        'tools/Build-P0Editor.ps1', 'tools/Test-P0Automation.ps1', 'tools/Sync-P0Data.ps1'
    ) },
    @{ Name = '01 A/G1-01 route declarations and calculation'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Battle/LDRouteModel.h", "${SourceRoot}Battle/LDRouteModel.cpp"
    ) },
    @{ Name = '02 A/G1-02 persistent enemy declarations and implementation'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Battle/LDEnemyActor.h", "${SourceRoot}Battle/LDEnemyActor.cpp"
    ) },
    @{ Name = '03 B/G1-01 immutable cell geometry and view transform'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Board/LDBoardGeometry.h", "${SourceRoot}Board/LDBoardGeometry.cpp", "${SourceRoot}Board/LDViewTransform.h"
    ) },
    @{ Name = '04 B/G1-02 camera, widget and input composition'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Board/LDBoardPresentation.h", "${SourceRoot}Board/LDBoardPresentation.cpp",
        "${SourceRoot}UI/LDG1BoardWidget.h", "${SourceRoot}UI/LDG1BoardWidget.cpp",
        "${SourceRoot}Core/LDPlayerController.h", "${SourceRoot}Core/LDPlayerController.cpp"
    ) },
    @{ Name = '05 integration module dependencies and normal local presentation'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Mobile_defense_clone.Build.cs", "${SourceRoot}Core/LDLocalPresentationSubsystem.h",
        "${SourceRoot}Core/LDLocalPresentationSubsystem.cpp"
    ) },
    @{ Name = '06 independent route, lifecycle and geometry expectations'; Kind = 'LearnerAuthoredReference'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Tests/LDRouteTests.cpp", "${SourceRoot}Tests/LDBoardGeometryTests.cpp"
    ) },
    @{ Name = '07 provided explicit Development execution fixture'; Kind = 'ProvidedVerification'; Sha = $SourceSha; Paths = @(
        "${SourceRoot}Verification/LDG1ProbeSubsystem.h", "${SourceRoot}Verification/LDG1ProbeSubsystem.cpp",
        'tools/Run-P0Pair.ps1'
    ) }
)
$FinalPaths = @($Steps | Where-Object { $_.Sha -eq $SourceSha } | ForEach-Object { $_.Paths })
$ChangedPaths = @(Invoke-ReplayGit $RepositoryRoot @('diff', '--name-only', $BaseSha, $SourceSha, '--', 'Source', 'Config', 'Content', 'tools'))
$Uncovered = @($ChangedPaths | Where-Object { $_ -notin $FinalPaths })
if ($Uncovered.Count -gt 0) {
    throw "SourceSha contains runtime/tool changes outside the reviewed G1 lesson manifest. Update the lesson and explicit paths first: $($Uncovered -join ', ')"
}
foreach ($Step in $Steps) {
    foreach ($RelativePath in $Step.Paths) {
        $ObjectType = (Invoke-ReplayGit $RepositoryRoot @('cat-file', '-t', "$($Step.Sha):$RelativePath")).Trim()
        if ($ObjectType -ne 'blob') { throw "Expected a source file: $RelativePath" }
    }
}
if ($ValidateOnly) {
    Write-Output "Preflight Pass: base=$BaseSha source=$SourceSha; $($ChangedPaths.Count) changed paths covered; no worktree created."
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
        throw 'The newly created worktree must be clean and detached at the canonical G0 base.'
    }
    foreach ($Step in $Steps) {
        Invoke-ReplayGit $ReplayRoot (@('restore', "--source=$($Step.Sha)", '--worktree', '--') + $Step.Paths)
        foreach ($RelativePath in $Step.Paths) {
            $SourceBlob = (Invoke-ReplayGit $ReplayRoot @('rev-parse', "$($Step.Sha):$RelativePath")).Trim()
            $WorkingBlob = (Invoke-ReplayGit $ReplayRoot @('hash-object', "--path=$RelativePath", (Join-Path $ReplayRoot $RelativePath))).Trim()
            if ($SourceBlob -ne $WorkingBlob) { throw "Restored file differs: $RelativePath" }
            $Manifest.Add([ordered]@{
                Step = $Step.Name; Kind = $Step.Kind; Path = $RelativePath; SourceSha = $Step.Sha
                ExpectedBlob = $SourceBlob; ActualBlob = $WorkingBlob; Result = 'Pass'
            })
        }
        Write-Output "$($Step.Name): $($Step.Paths.Count) files restored and blob-checked."
    }
    $ReplayHead = (Invoke-ReplayGit $ReplayRoot @('rev-parse', 'HEAD')).Trim()
    if ($ReplayHead -ne $BaseSha -or (Invoke-ReplayGit $ReplayRoot @('branch', '--show-current'))) {
        throw 'Replay HEAD must remain detached at G0; no completed branch is merged.'
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
Write-Output 'The replay HEAD remains G0. Pair every subsequent build/test result with this assembly manifest and SourceSha.'
