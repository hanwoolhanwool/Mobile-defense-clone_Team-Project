#Requires -Version 7.2
[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Windows')][string]$Stage = 'Editor',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [Parameter(Mandatory)][string]$CompilerVersion,
    [string]$WindowsSdkVersion = '10.0.26100.0'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$RunRoot = Join-Path $ProjectRoot ('Saved\BuildRuns\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$Workspace = Join-Path $RunRoot 'Workspace'
$LogRoot = Join-Path $RunRoot 'Logs'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$UatBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$ProjectFile = Join-Path $Workspace 'Mobile_defense_clone.uproject'
$Result = [ordered]@{ Stage = $Stage; Result = 'Running'; RunRoot = $RunRoot; Started = (Get-Date).ToString('o'); CompilerVersion = $CompilerVersion; WindowsSdkVersion = $WindowsSdkVersion }
New-Item -ItemType Directory -Path $Workspace, $LogRoot | Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $RunRoot 'build-script.ps1')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Test-WindowsPackage.ps1') -Destination (Join-Path $RunRoot 'Test-WindowsPackage.ps1')

function Invoke-BuildStep([string]$Name, [string]$Program, [string[]]$NativeArguments) {
    @{ Program = $Program; Arguments = $NativeArguments } | ConvertTo-Json -Depth 4 |
        Set-Content (Join-Path $LogRoot "$Name-command.json") -Encoding utf8
    & $Program @NativeArguments *> (Join-Path $LogRoot "$Name.log")
    $Code = $LASTEXITCODE
    if ($Code -ne 0) { throw "$Name failed with exit code $Code. See $LogRoot\$Name.log" }
}

try {
    if ($CompilerVersion -notmatch '^14\.50\.\d+$') { throw 'DEC-010 requires a discovered MSVC 14.50 toolchain directory version.' }
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $Instances = & $VsWhere -products '*' -version '[18.0,19.0)' -format json | ConvertFrom-Json
    $CompilerFiles = @($Instances | Where-Object isComplete | ForEach-Object { Join-Path $_.installationPath "VC\Tools\MSVC\$CompilerVersion\bin\Hostx64\x64\cl.exe" } | Where-Object { Test-Path -LiteralPath $_ })
    if ($CompilerFiles.Count -eq 0) { throw 'Requested compiler is not present in a complete stable VS 2026 installation.' }
    $CompilerProduct = (Get-Item -LiteralPath $CompilerFiles[0]).VersionInfo.ProductVersion
    if ([version]$CompilerProduct -lt [version]'14.50.35723.0' -or [version]$CompilerProduct -ge [version]'14.51.0.0') { throw "Unexpected compiler patch: $CompilerProduct" }
    $SdkHeader = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include\$WindowsSdkVersion\um\Windows.h"
    if (!(Test-Path -LiteralPath $SdkHeader)) { throw 'Selected Windows SDK is missing.' }
    $Result.CompilerProductVersion = $CompilerProduct
    $EngineConfig = Get-Content (Join-Path $ProjectRoot 'Config\DefaultEngine.ini') -Raw
    $WindowsConfig = [regex]::Match($EngineConfig, '(?ms)^\[/Script/WindowsTargetPlatform.WindowsTargetSettings\]\r?\n(.*?)(?=^\[|\z)').Groups[1].Value
    foreach ($Expected in @{Compiler='VisualStudio2026';CompilerVersion=$CompilerVersion;WindowsSDKVersion=$WindowsSdkVersion}.GetEnumerator()) {
        $Values = @([regex]::Matches($WindowsConfig, ('(?m)^' + [regex]::Escape($Expected.Key) + '=(.*?)\r?$')) | ForEach-Object { $_.Groups[1].Value.Trim() })
        if ($Values.Count -ne 1 -or $Values[0] -ne $Expected.Value) { throw "Project Windows setting must match this run: $($Expected.Key)=$($Expected.Value)" }
    }
    foreach ($Required in @($BuildBat, $UatBat, (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'))) {
        if (!(Test-Path -LiteralPath $Required)) { throw "Missing input: $Required" }
    }
    $EngineVersion = Get-Content (Join-Path $EngineRoot 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
    if ("$($EngineVersion.MajorVersion).$($EngineVersion.MinorVersion).$($EngineVersion.PatchVersion)" -ne '5.8.2') { throw 'Engine differs from DEC-009 candidate.' }
    $EngineVersion | ConvertTo-Json | Set-Content (Join-Path $LogRoot 'engine-version.json') -Encoding utf8
    $InputFiles = @(Get-Item (Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'))
    foreach ($Directory in @('Source', 'Config', 'Content', 'Build', 'Plugins')) {
        $InputDirectory = Join-Path $ProjectRoot $Directory
        if (Test-Path -LiteralPath $InputDirectory) {
            $InputFiles += Get-ChildItem -LiteralPath $InputDirectory -File -Recurse |
                Where-Object { $_.FullName -notmatch '[\\/](Binaries|Intermediate|Saved|DerivedDataCache)[\\/]' }
        }
    }
    $Manifest = foreach ($InputFile in $InputFiles) {
        $Relative = [IO.Path]::GetRelativePath($ProjectRoot, $InputFile.FullName)
        $Destination = Join-Path $Workspace $Relative
        New-Item -ItemType Directory -Path (Split-Path $Destination) -Force | Out-Null
        Copy-Item -LiteralPath $InputFile.FullName -Destination $Destination
        $Hash = (Get-FileHash -LiteralPath $InputFile.FullName -Algorithm SHA256).Hash
        if ((Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -ne $Hash) { throw "Input copy changed: $Relative" }
        [ordered]@{ Path = $Relative; SHA256 = $Hash; Bytes = $InputFile.Length }
    }
    $Manifest | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $RunRoot 'input-manifest.json') -Encoding utf8
    $Common = @("-Project=$ProjectFile", '-WaitMutex', '-NoHotReloadFromIDE', '-2026', "-CompilerVersion=$CompilerVersion", "-WindowsSdkVersion=$WindowsSdkVersion", '-MaxParallelActions=4')
    Invoke-BuildStep 'project-files' $BuildBat @('-projectfiles', "-project=$ProjectFile", '-game', '-engine', '-2026')
    Invoke-BuildStep 'editor' $BuildBat (@('Mobile_defense_cloneEditor', 'Win64', 'Development') + $Common)
    $EditorDll = Join-Path $Workspace 'Binaries\Win64\UnrealEditor-Mobile_defense_clone.dll'
    if (!(Test-Path -LiteralPath $EditorDll)) { throw 'Editor build returned success without the expected module DLL.' }
    if ($Stage -eq 'Windows') {
        Invoke-BuildStep 'windows-package' $UatBat @('BuildCookRun', "-project=$ProjectFile", '-noP4', '-unattended', '-utf8output', '-target=Mobile_defense_clone', '-platform=Win64', '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-package', '-archive', '-map=/Game/TopDown/Lvl_TopDown', "-archivedirectory=$RunRoot\Packages\Windows")
        $Package = & (Join-Path $PSScriptRoot 'Test-WindowsPackage.ps1') -ArchiveRoot (Join-Path $RunRoot 'Packages\Windows')
        $Package | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $RunRoot 'package-artifacts.json') -Encoding utf8
        $Result.Executable = $Package.Executable
    }
    $Result.Result = 'Pass'
} catch {
    $Result.Result = 'Fail'
    $Result.Error = $_.Exception.Message
} finally {
    $Result.Finished = (Get-Date).ToString('o')
    $Result | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $RunRoot 'result.json') -Encoding utf8
    $Result | ConvertTo-Json -Depth 4
}
if ($Result.Result -ne 'Pass') { exit 1 }
