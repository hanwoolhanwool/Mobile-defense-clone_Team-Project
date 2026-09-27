param([Parameter(Mandatory=$true)][string]$RunRoot)
$ErrorActionPreference = 'Stop'
$RunRoot = (Resolve-Path -LiteralPath $RunRoot).Path
$Output = Join-Path $RunRoot 'wire-evidence.json'
if (Test-Path -LiteralPath $Output) { throw 'Existing evidence is preserved.' }
$Pair = Get-Content -LiteralPath "$RunRoot/pair.json" -Raw | ConvertFrom-Json
$HostResult = Get-Content -LiteralPath "$RunRoot/host/result.json" -Raw | ConvertFrom-Json
$ClientResult = Get-Content -LiteralPath "$RunRoot/client/result.json" -Raw | ConvertFrom-Json
$Rows = @()
$Checks = @()
foreach ($Role in @('host','client')) {
    $Lines = Get-Content -LiteralPath "$RunRoot/$Role/engine.log"
    for ($LineIndex = 0; $LineIndex -lt $Lines.Count; ++$LineIndex) {
        if ($Lines[$LineIndex] -match 'P0WIRE (SERVER|CLIENT) (match=(\w+) epoch=(\d+) id=(\d+) code=(\d+) board=(\d+) economy=(\d+) event=(\d+) created=(\S+) moved=(\S+) removed=(\S+))') {
            $Rows += [pscustomobject]@{Role=$Role;Line=$LineIndex+1;Direction=$Matches[1];Signature=$Matches[2];MatchId=$Matches[3];Epoch=$Matches[4];RequestId=[int]$Matches[5];Code=[int]$Matches[6];Board=[int]$Matches[7];Economy=[int]$Matches[8];Event=$Matches[9];Created=$Matches[10]}
        }
    }
}
foreach ($Match in $HostResult.matches) {
    $RemoteMatch = @($ClientResult.matches | Where-Object { $_.matchId -eq $Match.matchId })
    $Checks += [pscustomobject]@{Name='both-results';MatchId=$Match.matchId;Pass=($RemoteMatch.Count -eq 1 -and $Match.result -eq $RemoteMatch[0].result -and $Match.reason -eq $RemoteMatch[0].reason)}
    foreach ($Role in @('host','client')) {
        $Received = @($Rows | Where-Object { $_.Role -eq $Role -and $_.Direction -eq 'CLIENT' -and $_.MatchId -eq $Match.matchId -and $_.RequestId -eq 1 -and $_.Code -eq 0 })
        $Epoch = if ($Received.Count) { $Received[0].Epoch } else { '' }
        $Server = @($Rows | Where-Object { $_.Direction -eq 'SERVER' -and $_.MatchId -eq $Match.matchId -and $_.Epoch -eq $Epoch -and $_.RequestId -eq 1 -and $_.Code -eq 0 })
        $Distinct = @(@($Received.Signature) + @($Server.Signature) | Select-Object -Unique)
        $Conflict = @($Rows | Where-Object { $_.Role -eq $Role -and $_.Direction -eq 'CLIENT' -and $_.MatchId -eq $Match.matchId -and $_.Epoch -eq $Epoch -and $_.RequestId -eq 1 -and $_.Code -eq 3 })
        $Checks += [pscustomobject]@{Name='twenty-real-replays-original-response';MatchId=$Match.matchId;Role=$Role;ServerCount=$Server.Count;ReceivedCount=$Received.Count;Signatures=$Distinct.Count;Pass=($Received.Count -ge 21 -and $Server.Count -ge 21 -and $Distinct.Count -eq 1)}
        $Checks += [pscustomobject]@{Name='same-key-different-payload-conflict';MatchId=$Match.matchId;Role=$Role;ReceivedCount=$Conflict.Count;Pass=($Conflict.Count -ge 1)}
        $First = if ($Received.Count) {$Received[0]} else {$null}
        $Checks += [pscustomobject]@{Name='first-summon-single-commit';MatchId=$Match.matchId;Role=$Role;Pass=($First -and $First.Board -eq 1 -and $First.Economy -eq 1 -and $First.Event -ne '0' -and $First.Created -match '^\d+,$')}
    }
}
$Checks += [pscustomobject]@{Name='strict-connected-minimum';HostSeconds=$HostResult.connectedGameplaySeconds;ClientSeconds=$ClientResult.connectedGameplaySeconds;Minimum=$Pair.MinimumSeconds;Pass=($HostResult.connectedGameplaySeconds -ge $Pair.MinimumSeconds -and $ClientResult.connectedGameplaySeconds -ge $Pair.MinimumSeconds)}
$Passed = $Pair.Result -eq 'Pass' -and @($Checks | Where-Object {!$_.Pass}).Count -eq 0
[ordered]@{Scope='Read-only real Controller RPC arrival/response log validation; server API tests are separate';Result=$(if($Passed){'Pass'}else{'Fail'});Created=(Get-Date).ToString('o');Source=$Pair.Head;Checks=$Checks;Rows=$Rows} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Output -Encoding utf8
Write-Output "$(if($Passed){'Pass'}else{'Fail'}): $Output"
if (!$Passed) { $Checks | Where-Object {!$_.Pass} | ConvertTo-Json -Depth 4; exit 1 }
