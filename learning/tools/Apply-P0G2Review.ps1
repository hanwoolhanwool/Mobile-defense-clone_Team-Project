param(
    [Parameter(Mandatory=$true)][string]$ReplayRoot,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{40}$')][string]$SourceSha,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId='Replay-G2-review-assembly'
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$ReplayRoot=(Resolve-Path -LiteralPath $ReplayRoot).Path
$InitialSource='5baa96059e94a142d45206290010b373cf39ea19'
$Base='4861b987f3e2fe78bcc159d1b6a85008543a938b'
$OriginalManifest=Join-Path $ReplayRoot 'Saved/P0Runs/Replay-G2-assembly/assembly.json'
$RunRoot=Join-Path $ReplayRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Preserve the existing review run and choose a new RunId.' }
function Invoke-ReviewGit([string[]]$Arguments) {
    $Output=& git -C $ReplayRoot @Arguments
    if ($LASTEXITCODE -ne 0) { throw "git failed: $($Arguments -join ' ')" }
    return $Output
}
if ((Invoke-ReviewGit @('rev-parse','HEAD')).Trim() -ne $Base) { throw 'Expected the original detached G1 replay HEAD.' }
& git -C $ReplayRoot symbolic-ref --quiet HEAD *> $null
if ($LASTEXITCODE -eq 0) { throw 'Only the detached reference replay can be amended.' }
$Manifest=Get-Content -LiteralPath $OriginalManifest -Raw | ConvertFrom-Json
if ($Manifest.Result -ne 'Pass' -or $Manifest.SourceSha -ne $InitialSource) { throw 'Expected the verified 5baa960 assembly manifest.' }
if ((Invoke-ReviewGit @('rev-parse',"${SourceSha}^{commit}")).Trim() -ne $SourceSha) { throw 'Use a full commit SHA.' }
& git -C $ReplayRoot merge-base --is-ancestor $InitialSource $SourceSha
if ($LASTEXITCODE -ne 0) { throw 'The review source must descend from the initial assembly.' }
$Allowed=@(
    'Source/Mobile_defense_clone/Tests/LDGameplayCommandTests.cpp',
    'Source/Mobile_defense_clone/Verification/LDG2ProbeSubsystem.h',
    'Source/Mobile_defense_clone/Verification/LDG2ProbeSubsystem.cpp'
)
$Changed=@(Invoke-ReviewGit @('diff','--name-only',$InitialSource,$SourceSha,'--','Source','Config','Content','tools','Build','Plugins','Mobile_defense_clone.uproject'))
if (@($Changed | Where-Object {$_ -notin $Allowed}).Count -gt 0) { throw 'Unexpected product change: use a fresh full replay instead.' }
foreach($File in $Manifest.Files) {
    $Actual=(Invoke-ReviewGit @('hash-object','--',$File.Path)).Trim()
    if ($Actual -ne $File.ActualBlob) { throw "Existing replay file changed; preserve it: $($File.Path)" }
}
# Preserve all three old files before changing only these verified, agent-assembled files.
New-Item -ItemType Directory -Path "$RunRoot/before" | Out-Null
foreach($Path in $Changed) {
    Copy-Item -LiteralPath (Join-Path $ReplayRoot $Path) -Destination (Join-Path "$RunRoot/before" ([IO.Path]::GetFileName($Path)))
    Invoke-ReviewGit @('restore',"--source=$SourceSha",'--worktree','--',$Path) | Out-Null
}
$Files=@()
foreach($File in $Manifest.Files) {
    $Expected=(Invoke-ReviewGit @('rev-parse',"${SourceSha}:$($File.Path)")).Trim()
    $Actual=(Invoke-ReviewGit @('hash-object','--',$File.Path)).Trim()
    if ($Actual -ne $Expected) { throw "Final blob mismatch: $($File.Path)" }
    $Files += [ordered]@{Path=$File.Path;Step=$File.Step;Kind=$File.Kind;ExpectedBlob=$Expected;ActualBlob=$Actual;Result='Pass'}
}
[ordered]@{
    Scope='Guarded supplementary tests and engine drag input; no product runtime change, learner branch edit, cleanup, build or game execution'
    Result='Pass';StartedFrom=$Base;PreviousSourceSha=$InitialSource;SourceSha=$SourceSha
    PreviousManifestSHA256=(Get-FileHash -LiteralPath $OriginalManifest -Algorithm SHA256).Hash
    ChangedPaths=$Changed;Files=$Files;ReplayHead=(Invoke-ReviewGit @('rev-parse','HEAD')).Trim();Created=(Get-Date).ToString('o')
} | ConvertTo-Json -Depth 7 | Set-Content "$RunRoot/assembly.json" -Encoding utf8
Write-Output "Pass: $($Files.Count) final blobs match $SourceSha; $($Changed.Count) verification-only files changed."
