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
$Terminals = @()
function Get-LogTimestamp([string]$Line) {
    if ($Line -notmatch '^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]') { return $null }
    $Parsed = [datetime]::MinValue
    $Style = [Globalization.DateTimeStyles]::AssumeUniversal -bor [Globalization.DateTimeStyles]::AdjustToUniversal
    if ([datetime]::TryParseExact($Matches[1], 'yyyy.MM.dd-HH.mm.ss:fff', [Globalization.CultureInfo]::InvariantCulture, $Style, [ref]$Parsed)) { return $Parsed }
    return $null
}
foreach ($Role in @('host','client')) {
    $Lines = Get-Content -LiteralPath "$RunRoot/$Role/engine.log"
    $ActiveMatch = ''
    for ($LineIndex = 0; $LineIndex -lt $Lines.Count; ++$LineIndex) {
        $Timestamp = Get-LogTimestamp $Lines[$LineIndex]
        if ($Role -eq 'host' -and $Lines[$LineIndex] -match 'LogLDMatch:.*P0 match ([A-Fa-f0-9]{32}) rules=') { $ActiveMatch = $Matches[1] }
        if ($Role -eq 'host' -and $Lines[$LineIndex] -match 'LogLDMatch:.*P0 terminal result=(\d+) reason=(\d+) time=([\d.]+)') {
            $Terminals += [pscustomobject]@{MatchId=$ActiveMatch;Line=$LineIndex+1;Timestamp=$Timestamp;Result=[int]$Matches[1];Reason=[int]$Matches[2];ServerSeconds=[double]::Parse($Matches[3], [Globalization.CultureInfo]::InvariantCulture)}
        }
        if ($Lines[$LineIndex] -match 'P0WIRE (SERVER|CLIENT) (match=(\w+) epoch=(\d+) id=(\d+) code=(\d+) board=(\d+) economy=(\d+) event=(\d+) created=(\S+) moved=(\S+) removed=(\S+))') {
            $Rows += [pscustomobject]@{Role=$Role;Line=$LineIndex+1;Timestamp=$Timestamp;Direction=$Matches[1];Signature=$Matches[2];MatchId=$Matches[3];Epoch=$Matches[4];RequestId=[int]$Matches[5];Code=[int]$Matches[6];Board=[int]$Matches[7];Economy=[int]$Matches[8];Event=$Matches[9];Created=$Matches[10]}
        }
    }
}
$HostMatches = @($HostResult.matches)
$ClientMatches = @($ClientResult.matches)
$HostIds = @($HostMatches | ForEach-Object { $_.matchId } | Select-Object -Unique)
$ClientIds = @($ClientMatches | ForEach-Object { $_.matchId } | Select-Object -Unique)
$Checks += [pscustomobject]@{Name='complete-unique-match-results';Requested=$Pair.Matches;HostCount=$HostMatches.Count;ClientCount=$ClientMatches.Count;Pass=($Pair.Matches -gt 0 -and $HostResult.result -eq 'Pass' -and $ClientResult.result -eq 'Pass' -and $HostMatches.Count -eq $Pair.Matches -and $ClientMatches.Count -eq $Pair.Matches -and $HostIds.Count -eq $Pair.Matches -and $ClientIds.Count -eq $Pair.Matches)}
foreach ($Match in $HostResult.matches) {
    $RemoteMatch = @($ClientResult.matches | Where-Object { $_.matchId -eq $Match.matchId })
    $Checks += [pscustomobject]@{Name='both-results';MatchId=$Match.matchId;Pass=($RemoteMatch.Count -eq 1 -and $Match.result -eq $RemoteMatch[0].result -and $Match.reason -eq $RemoteMatch[0].reason)}
    $Terminal = @($Terminals | Where-Object { $_.MatchId -eq $Match.matchId })
    $TerminalMatches = $Terminal.Count -eq 1 -and $null -ne $Terminal[0].Timestamp -and $Terminal[0].Result -eq $Match.result -and $Terminal[0].Reason -eq $Match.reason -and [math]::Abs($Terminal[0].ServerSeconds - $Match.resultServerSeconds) -lt 0.00001
    $Checks += [pscustomobject]@{Name='authoritative-terminal-log-matches-result';MatchId=$Match.matchId;TerminalLines=@($Terminal.Line);Pass=$TerminalMatches}
    foreach ($Role in @('host','client')) {
        $Received = @($Rows | Where-Object { $_.Role -eq $Role -and $_.Direction -eq 'CLIENT' -and $_.MatchId -eq $Match.matchId -and $_.RequestId -eq 1 -and $_.Code -eq 0 })
        $Epoch = if ($Received.Count) { $Received[0].Epoch } else { '' }
        $Server = @($Rows | Where-Object { $_.Role -eq 'host' -and $_.Direction -eq 'SERVER' -and $_.MatchId -eq $Match.matchId -and $_.Epoch -eq $Epoch -and $_.RequestId -eq 1 -and $_.Code -eq 0 })
        $Distinct = @(@($Received.Signature) + @($Server.Signature) | Select-Object -Unique)
        $Conflict = @($Rows | Where-Object { $_.Role -eq $Role -and $_.Direction -eq 'CLIENT' -and $_.MatchId -eq $Match.matchId -and $_.Epoch -eq $Epoch -and $_.RequestId -eq 1 -and $_.Code -eq 3 })
        $Checks += [pscustomobject]@{Name='twenty-real-replays-original-response';MatchId=$Match.matchId;Role=$Role;ServerCount=$Server.Count;ReceivedCount=$Received.Count;Signatures=$Distinct.Count;Pass=($Received.Count -ge 21 -and $Server.Count -ge 21 -and $Distinct.Count -eq 1)}
        $Checks += [pscustomobject]@{Name='same-key-different-payload-conflict';MatchId=$Match.matchId;Role=$Role;ReceivedCount=$Conflict.Count;Pass=($Conflict.Count -ge 1)}
        $First = if ($Received.Count) {$Received[0]} else {$null}
        $Checks += [pscustomobject]@{Name='first-summon-single-commit';MatchId=$Match.matchId;Role=$Role;Pass=($First -and $First.Board -eq 1 -and $First.Economy -eq 1 -and $First.Event -ne '0' -and $First.Created -match '^\d+,$')}
        # Server line order establishes Result before cache lookup. Client arrival uses the UTC
        # timestamps of this same-PC pair; no client-local Result marker is inferred from a count.
        $PostTerminalServer = @()
        $PostTerminalReceived = @()
        if ($TerminalMatches -and $First) {
            $PostTerminalServer = @($Server | Where-Object { $_.Line -gt $Terminal[0].Line -and $_.Signature -eq $First.Signature -and $null -ne $_.Timestamp })
            if ($PostTerminalServer.Count) {
                $Sent = $PostTerminalServer[0]
                $PostTerminalReceived = @($Received | Where-Object { $_.Signature -eq $First.Signature -and $null -ne $_.Timestamp -and $_.Timestamp -ge $Sent.Timestamp -and ($Role -ne 'host' -or $_.Line -gt $Sent.Line) })
            }
        }
        $Checks += [pscustomobject]@{Name='original-response-after-authoritative-result';MatchId=$Match.matchId;Role=$Role;TerminalLines=@($Terminal.Line);ServerLines=@($PostTerminalServer.Line);ReceivedLines=@($PostTerminalReceived.Line);Pass=($PostTerminalServer.Count -ge 1 -and $PostTerminalReceived.Count -ge 1)}
        $ChangedBoard = @($Rows | Where-Object { $_.Role -eq 'host' -and $_.Direction -eq 'SERVER' -and $_.MatchId -eq $Match.matchId -and $_.Epoch -eq $Epoch -and $_.RequestId -ne 1 -and $_.Code -eq 0 -and $_.Board -gt 1 } | Select-Object -First 1)
        $AfterBoardChange = @()
        if ($ChangedBoard.Count -and $First) { $AfterBoardChange = @($Server | Where-Object { $_.Line -gt $ChangedBoard[0].Line -and $_.Signature -eq $First.Signature }) }
        $Checks += [pscustomobject]@{Name='original-response-after-board-change';MatchId=$Match.matchId;Role=$Role;ChangedBoardLines=@($ChangedBoard.Line);ServerLines=@($AfterBoardChange.Line);Pass=($AfterBoardChange.Count -ge 1)}
    }
}
$Checks += [pscustomobject]@{Name='strict-connected-minimum';HostSeconds=$HostResult.connectedGameplaySeconds;ClientSeconds=$ClientResult.connectedGameplaySeconds;Minimum=$Pair.MinimumSeconds;Pass=($HostResult.connectedGameplaySeconds -ge $Pair.MinimumSeconds -and $ClientResult.connectedGameplaySeconds -ge $Pair.MinimumSeconds)}
$Passed = $Pair.Result -eq 'Pass' -and @($Checks | Where-Object {!$_.Pass}).Count -eq 0
[ordered]@{Scope='Read-only real Controller RPC arrival/response log validation; server API tests are separate';Limits=@('Client receipt ordering uses UTC timestamps from the same-PC host/client run; separate machines require an independently verified clock basis.','A terminal replay is proven by an authoritative Result log followed by the original SERVER response and a matching CLIENT arrival. This does not assert a client-local Result marker.','Wire signatures do not independently prove in-flight Pending handling or count UI effects. Automation and terminal snapshot checks remain separate evidence.');Result=$(if($Passed){'Pass'}else{'Fail'});Created=(Get-Date).ToString('o');Source=$Pair.Head;AuditorSHA256=(Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash;ExecutableSHA256=$Pair.ExecutableSHA256;Checks=$Checks;Terminals=$Terminals;Rows=$Rows} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Output -Encoding utf8
Write-Output "$(if($Passed){'Pass'}else{'Fail'}): $Output"
if (!$Passed) { $Checks | Where-Object {!$_.Pass} | ConvertTo-Json -Depth 4; exit 1 }
