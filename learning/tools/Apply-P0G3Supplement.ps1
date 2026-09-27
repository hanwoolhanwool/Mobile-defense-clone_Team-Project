<#
Continue the documented G3 replay from its audited 66-file input set.
Only the explicit 73-file lesson manifest is touched. Existing changed inputs are
copied to a NEW evidence directory before replacement; no files are removed.
This records assembly, not compile, runtime verification or learner completion.
#>
param(
    [Parameter(Mandatory=$true)][string]$RepositoryRoot,
    [Parameter(Mandatory=$true)][string]$ReplayRoot,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$SourceSha,
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId
)
$ErrorActionPreference = 'Stop'
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$ReplayRoot = (Resolve-Path -LiteralPath $ReplayRoot).Path
$Base = 'f735b5889a5bd197e46d29bdfaa2b38c246d5ea6'
if ((& git -C $ReplayRoot rev-parse HEAD) -ne $Base) { throw 'Replay must retain the documented G2 starting HEAD.' }
& git -C $ReplayRoot symbolic-ref -q HEAD *> $null
if ($LASTEXITCODE -eq 0) { throw 'Only a detached replay may be amended; never a learn branch.' }
$PriorFile = Join-Path $RepositoryRoot 'learning/P0/evidence/G3_REPLAY/audited-final-inputs.json'
$Prior = Get-Content -LiteralPath $PriorFile -Raw | ConvertFrom-Json
if ($Prior.Files.Count -ne 66) { throw 'Expected the preserved audited 66-file prior manifest.' }
$Added = @(
    'Source/Mobile_defense_clone/Verification/LDG3BoundaryProbeSubsystem.h',
    'Source/Mobile_defense_clone/Verification/LDG3BoundaryProbeSubsystem.cpp',
    'Source/Mobile_defense_clone/Verification/LDG3EntryProbeSubsystem.h',
    'Source/Mobile_defense_clone/Verification/LDG3EntryProbeSubsystem.cpp',
    'Source/Mobile_defense_clone/Verification/LDG3NetConflictProbeSubsystem.h',
    'Source/Mobile_defense_clone/Verification/LDG3NetConflictProbeSubsystem.cpp',
    'tools/Run-P0G3Supplement.ps1'
)
$RunRoot = Join-Path $ReplayRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Previous evidence is preserved; choose a new RunId.' }
& (Join-Path $PSScriptRoot 'Replay-P0G3.ps1') -RepositoryRoot $RepositoryRoot -ReplayRoot "$ReplayRoot-preflight-$RunId" -SourceSha $SourceSha -ValidateOnly
if ($LASTEXITCODE -ne 0) { throw 'Final-source manifest preflight failed.' }
foreach ($File in $Prior.Files) {
    $Actual = (Get-FileHash -LiteralPath (Join-Path $ReplayRoot $File.Path) -Algorithm SHA256).Hash
    if ($Actual -ne $File.SHA256) { throw "An existing replay input changed outside this amendment: $($File.Path)" }
}
foreach ($Path in $Added) {
    if (Test-Path -LiteralPath (Join-Path $ReplayRoot $Path)) { throw "New input already exists; preserve and review it: $Path" }
}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Record = [ordered]@{ Scope='Assembly only: documented 66 inputs plus seven provided verification files'; SourceSha=$SourceSha; ReplayHead=$Base; PriorManifest=$PriorFile; Started=(Get-Date).ToString('o'); Result='Running'; Files=@(); Replaced=@() }
try {
    foreach ($Path in @($Prior.Files.Path) + $Added) {
        $Target = Join-Path $ReplayRoot $Path
        $ExpectedBlob = (& git -C $RepositoryRoot rev-parse "${SourceSha}:$Path").Trim()
        if ($LASTEXITCODE -ne 0) { throw "Missing reviewed input: $Path" }
        $BeforeBlob = if(Test-Path -LiteralPath $Target){(& git -C $ReplayRoot hash-object $Path).Trim()}else{''}
        if ($BeforeBlob -ne $ExpectedBlob) {
            if ($BeforeBlob) {
                $Backup = Join-Path $RunRoot "before/$Path"
                New-Item -ItemType Directory -Force -Path (Split-Path $Backup -Parent) | Out-Null
                Copy-Item -LiteralPath $Target -Destination $Backup
            }
            New-Item -ItemType Directory -Force -Path (Split-Path $Target -Parent) | Out-Null
            $Start = [Diagnostics.ProcessStartInfo]::new('git')
            $Start.UseShellExecute = $false
            $Start.CreateNoWindow = $true
            $Start.RedirectStandardOutput = $true
            $Start.RedirectStandardError = $true
            foreach($Argument in @('-C',$RepositoryRoot,'show',"${SourceSha}:$Path")) { $Start.ArgumentList.Add($Argument) }
            $Process = [Diagnostics.Process]::Start($Start)
            $Stream = [IO.File]::Create($Target)
            try { $Process.StandardOutput.BaseStream.CopyTo($Stream) } finally { $Stream.Dispose() }
            $ErrorText = $Process.StandardError.ReadToEnd()
            $Process.WaitForExit()
            if($Process.ExitCode -ne 0) { throw "git show failed for $Path : $ErrorText" }
            $Record.Replaced += [ordered]@{Path=$Path;BeforeBlob=$BeforeBlob;AfterBlob=$ExpectedBlob;Backup=$(if($BeforeBlob){"before/$Path"}else{$null})}
        }
        $ActualBlob = (& git -C $ReplayRoot hash-object $Path).Trim()
        if ($LASTEXITCODE -ne 0 -or $ActualBlob -ne $ExpectedBlob) { throw "Assembled blob mismatch: $Path" }
        $Record.Files += [ordered]@{Path=$Path;ExpectedBlob=$ExpectedBlob;ActualBlob=$ActualBlob;SHA256=(Get-FileHash -LiteralPath $Target -Algorithm SHA256).Hash}
    }
    $Record.Result = 'Pass'
} catch {
    $Record.Result = 'Fail'
    $Record.Error = $_.Exception.Message
} finally {
    $Record.Finished = (Get-Date).ToString('o')
    $Record | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath "$RunRoot/assembly.json" -Encoding utf8
}
Write-Output "$($Record.Result): $RunRoot/assembly.json"
if ($Record.Result -ne 'Pass') { exit 1 }
