param(
    [ValidateSet('Win64','Android')][string]$Platform = 'Win64',
    [string]$RunId = ('Package-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [string]$AndroidSdk = "$env:LOCALAPPDATA/Android/Sdk",
    [string]$AndroidJdk = ''
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
if ($RunId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Use a simple new RunId.' }
$RunRoot = Join-Path $ProjectRoot "Saved/P0Runs/$RunId"
if (Test-Path -LiteralPath $RunRoot) { throw 'RunId already exists; preserve it and choose a new one.' }
New-Item -ItemType Directory -Path $RunRoot | Out-Null
if (!(Test-Path (Join-Path $ProjectRoot 'Content/LD/Maps/L_P0.umap'))) { throw 'Create and verify L_P0 before packaging.' }
if (!(Test-Path (Join-Path $ProjectRoot 'Content/LD/Maps/L_P0Entry.umap'))) { throw 'Create and verify L_P0Entry before packaging.' }
$Project = Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'
$Uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
$Arguments = @('BuildCookRun',"-project=$Project",'-noP4','-unattended','-utf8output','-target=Mobile_defense_clone',"-platform=$Platform",'-clientconfig=Development','-build','-cook','-stage','-pak','-package','-archive','-ubtargs=-NoHotReloadFromIDE -MaxParallelActions=4','-map=/Game/LD/Maps/L_P0+/Game/LD/Maps/L_P0Entry',"-archivedirectory=$RunRoot/Package")
if ($Platform -eq 'Android') {
    if (!$AndroidJdk) { $AndroidJdk = Join-Path $ProjectRoot 'Saved/Tooling/jdk21/jdk-21.0.3+9' }
    $env:ANDROID_HOME = $AndroidSdk
    $env:NDKROOT = Join-Path $AndroidSdk 'ndk/27.2.12479018'
    $env:JAVA_HOME = $AndroidJdk
    foreach ($Required in @("$AndroidSdk/platforms/android-36/android.jar", "$env:NDKROOT/source.properties", "$AndroidJdk/bin/java.exe")) {
        if (!(Test-Path -LiteralPath $Required)) { throw "Missing Android input: $Required" }
    }
    $Arguments += '-cookflavor=ETC2'
}
$Result = [ordered]@{Scope='Compile/cook/package only; installation and gameplay separate'; Platform=$Platform; Started=(Get-Date).ToString('o'); Head=(& git -C $ProjectRoot rev-parse HEAD); Command=$Uat; Arguments=$Arguments}
& $Uat @Arguments *> "$RunRoot/package.log"
$Result.ExitCode = $LASTEXITCODE
$Result.Finished = (Get-Date).ToString('o')
$Result.Result = if ($Result.ExitCode -eq 0) {'Pass'} else {'Fail'}
$Result | ConvertTo-Json -Depth 4 | Set-Content "$RunRoot/result.json" -Encoding utf8
Write-Output "$($Result.Result): $RunRoot/package.log"
if ($Result.ExitCode -ne 0) { Get-Content "$RunRoot/package.log" -Tail 35 }
exit $Result.ExitCode
