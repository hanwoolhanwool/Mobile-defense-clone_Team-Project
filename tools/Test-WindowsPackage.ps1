#Requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$ArchiveRoot)
$ErrorActionPreference = 'Stop'
$ArchiveRoot = (Resolve-Path -LiteralPath $ArchiveRoot).Path
$Roots = @($ArchiveRoot, (Join-Path $ArchiveRoot 'Windows'))
$Packages = @($Roots | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'Mobile_defense_clone.exe') })
if ($Packages.Count -ne 1) { throw 'Expected exactly one packaged bootstrap executable in the archive root or its Windows directory.' }
$PackageRoot = $Packages[0]
$Executable = Join-Path $PackageRoot 'Mobile_defense_clone.exe'
$Runtime = Join-Path $PackageRoot 'Mobile_defense_clone\Binaries\Win64\Mobile_defense_clone.exe'
$Paks = Join-Path $PackageRoot 'Mobile_defense_clone\Content\Paks'
if (!(Test-Path -LiteralPath $Runtime)) { throw 'Packaged runtime executable missing.' }
$Content = @(Get-ChildItem -LiteralPath $Paks -File | Where-Object { $_.Extension -in @('.pak', '.utoc', '.ucas') })
if ($Content.Count -eq 0 -or @($Content | Where-Object Length -le 0).Count -gt 0) { throw 'Cooked content missing or empty.' }
if (@($Content | Where-Object Extension -eq '.utoc').Count -gt 0 -and @($Content | Where-Object Extension -eq '.ucas').Count -eq 0) { throw 'IoStore container data missing.' }
[pscustomobject]@{
    Result = 'Pass'
    Scope = 'Artifact structure only; runtime and input are separate checks'
    PackageRoot = $PackageRoot
    Executable = $Executable
    Runtime = $Runtime
    ExecutableSHA256 = (Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash
    RuntimeSHA256 = (Get-FileHash -LiteralPath $Runtime -Algorithm SHA256).Hash
    ContentFiles = @($Content | ForEach-Object { [ordered]@{Name=$_.Name;Bytes=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash} })
}
