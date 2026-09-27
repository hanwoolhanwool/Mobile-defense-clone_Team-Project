param(
    [Parameter(Mandatory=$true)][string]$ReferenceRoot,
    [Parameter(Mandatory=$true)][string]$AReplayRoot,
    [Parameter(Mandatory=$true)][string]$MissingReplayRoot,
    [Parameter(Mandatory=$true)][string]$BReplayRoot,
    [Parameter(Mandatory=$true)][string]$CanonicalReplayRoot,
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference = 'Stop'
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a new simple RunId.' }
$ReferenceRoot = (Resolve-Path -LiteralPath $ReferenceRoot).Path
$RunRoot = Join-Path $ReferenceRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'Previous queue evidence is preserved.' }
# Performance/game runs must finish before even the first build starts.
foreach ($Port in @(17777,18777)) {
    if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw "P0 game port $Port is in use; wait for its owner to finish." }
}
$Entries = @(
    @{Name='A'; Root=$AReplayRoot; Filter='LD.PIE.G0.A.MatchLifecycle'},
    @{Name='A-missing'; Root=$MissingReplayRoot; Filter='LD.PIE.G0.A.MissingData'},
    @{Name='B'; Root=$BReplayRoot; Filter='LD.PIE.G0.B.OwnedRPC'},
    @{Name='canonical'; Root=$CanonicalReplayRoot; Filter='LD.PIE.G0.Canonical.TerminalCache'}
)
foreach ($Entry in $Entries) {
    $Entry.Root = (Resolve-Path -LiteralPath $Entry.Root).Path
    if (!(Test-Path -LiteralPath (Join-Path $Entry.Root 'Source/Mobile_defense_clone/Tests/LDG0PieTests.cpp'))) { throw "Install the provided harness first: $($Entry.Name)" }
    foreach ($Suffix in @('editor','pie','pie-proof')) {
        if (Test-Path -LiteralPath (Join-Path $Entry.Root "Saved/P0Runs/$RunId-$($Entry.Name)-$Suffix")) { throw "Previous role evidence is preserved: $($Entry.Name)" }
    }
}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Record = [ordered]@{Scope='Serial builds and actual Editor PIE of independent historical G0 inputs; not package or Android';Started=(Get-Date).ToString('o');Result='Running';Stages=@()}
try {
    foreach ($Entry in $Entries) {
        $Stage = [ordered]@{Role=$Entry.Name;Root=$Entry.Root;Filter=$Entry.Filter;Build='NotRun';PIE='NotRun'}
        $Record.Stages += $Stage
        & (Join-Path $Entry.Root 'tools/Build-P0Editor.ps1') -RunId "$RunId-$($Entry.Name)-editor" -EngineRoot $EngineRoot *> "$RunRoot/$($Entry.Name)-build.log"
        $BuildExit = $LASTEXITCODE
        $Build = Get-Content -LiteralPath (Join-Path $Entry.Root "Saved/P0Runs/$RunId-$($Entry.Name)-editor/result.json") -Raw | ConvertFrom-Json
        $Stage.Build = $Build.Result
        $Record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$RunRoot/queue.json" -Encoding utf8
        if ($BuildExit -ne 0 -or $Build.Result -ne 'Pass') { throw "$($Entry.Name) Editor build failed; inspect its saved build log before continuing." }
        & (Join-Path $ReferenceRoot 'learning/tools/Test-P0G0PIE.ps1') -ProjectRoot $Entry.Root -RunId "$RunId-$($Entry.Name)-pie" -Filter $Entry.Filter -EngineRoot $EngineRoot *> "$RunRoot/$($Entry.Name)-pie.log"
        $PIEExit = $LASTEXITCODE
        $PIE = Get-Content -LiteralPath (Join-Path $Entry.Root "Saved/P0Runs/$RunId-$($Entry.Name)-pie/result.json") -Raw | ConvertFrom-Json
        $Stage.PIE = $PIE.Result
        $Record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$RunRoot/queue.json" -Encoding utf8
        if ($PIEExit -ne 0 -or $PIE.Result -ne 'Pass') { throw "$($Entry.Name) actual PIE failed; preserve evidence and investigate before continuing." }
        Write-Output "$($Entry.Name): Editor and actual PIE Pass."
    }
    $Record.Result = 'Pass'
} catch {
    $Record.Result = 'Fail'
    $Record.Error = $_.Exception.Message
} finally {
    $Record.Finished = (Get-Date).ToString('o')
    $Record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$RunRoot/queue.json" -Encoding utf8
}
Write-Output "$($Record.Result): $RunRoot/queue.json"
if ($Record.Result -ne 'Pass') { exit 1 }
