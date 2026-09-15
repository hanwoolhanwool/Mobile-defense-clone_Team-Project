---
id: TECH-BUILD
version: 0.1.8
status: Draft
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-01
applies_to: UE 5.8.2 후보 / Windows Development·Android 준비
verified: 2026-09-13
verified_run: RUN-20260913-03
verification_scope: 토큰 분리 전 Windows 기본 빌드·PIE·패키지 입력/종료만; Android 및 변경 후 재실행 제외
---

# PowerShell 빌드·실행 절차

[개발 환경·버전 기준](DEVELOPMENT_SETUP.md) · [결정 기록](../DECISIONS.md) · [검수 기록](../production/TEST_RUNS.md)

DEC-012에 따라 공통 PowerShell 명령을 실행 절차의 원본으로 사용한다. 에디터 메뉴와 Project Launcher는 보조 수단이다. 아래 명령은 UE 5.8.2의 배치 파일·UBT·UAT 소스와 공식 문서를 확인한 실행 안내다. Windows 기본 빌드·실행의 최신 증거는 RUN-20260913-02/03이며 Android는 미실행이다. 문서 검토일과 실제 실행 검증일을 구분한다.

## 1. 실행 전 조건과 순서

1. DEC-010/011의 도구를 설치하고 정확한 패치·경로를 확인한다. 현재 설치 현황과 목표 버전은 개발 환경 안내에서 구분한다.
2. 아래 Windows 도구 설정을 적용한 뒤 프로젝트 파일을 생성하고 Development Editor를 빌드한다.
3. 현재 기본 맵을 에디터에서 실행하고, 에디터를 종료한 뒤 Windows Development 패키징·실행을 확인한다.
4. Android는 도구 설정과 모바일 설정 결정이 모두 적용된 뒤 Development 패키징·실기기 실행을 확인한다.
5. 실행별 명령·버전·로그·산출물·결과를 검수 기록에 연결한다. DEC-009의 엔진 고정에는 양 플랫폼 빌드 Pass가 필요하며, 기기 실행·게임 QA 결과는 별도 항목으로 기록한다.

소스 빌드 및 패키징 전에는 에디터와 Live Coding을 종료한다. 최초 재현성 검증은 선택한 소스·콘텐츠·설정을 모두 포함한 별도 작업 폴더에서 수행한다. 미커밋·미추적 입력이 있으면 Git checkout만으로 같은 입력이 만들어지지 않으므로 입력 파일 목록·변경 내역을 먼저 보존한다. 기준 커밋과 로컬 설정 차이를 함께 기록한다. 기존 작업 폴더를 지우는 정리 명령은 이 절차에 포함하지 않는다.

## 2. 공통 경로와 로그

### 별도 작업 폴더에서 실행하는 스크립트

2026-09-13 TASK-CORE-01에서 [Invoke-PrototypeBuild.ps1](../../tools/Invoke-PrototypeBuild.ps1)을 추가했다. PowerShell 7.2 이상에서 현재 프로젝트의 Source·Config·Content·Build·Plugins 입력을 새 `Saved/BuildRuns/<시각>/Workspace/`에 복사·해시 비교한 뒤 프로젝트 파일 생성과 Editor 빌드를 수행한다. Windows 단계는 이어서 기본 맵 패키징까지 수행한다. 실제 설치된 14.50 계열 디렉터리 버전을 인자로 전달한다.

```powershell
# <MSVC 디렉터리 버전>은 설치 및 cl.exe 패치 확인 후 치환한다.
pwsh -File tools/Invoke-PrototypeBuild.ps1 -Stage Editor -CompilerVersion <MSVC 디렉터리 버전>
pwsh -File tools/Invoke-PrototypeBuild.ps1 -Stage Windows -CompilerVersion <MSVC 디렉터리 버전>
```

프로젝트 Windows 설정에도 같은 Compiler·CompilerVersion·WindowsSDKVersion을 지정한 뒤 실행한다. 스크립트는 설정과 인자의 일치를 검사한다. 명령·로그는 각 실행의 `Logs/`, 입력 해시는 `input-manifest.json`, 단계 결과는 `result.json`에 남는다. 스크립트의 Pass는 지정 단계의 빌드·산출물 검사이며 PIE·화면·기본 입력·Android 실기기 테스트를 의미하지 않는다. 현재 실행 결과는 검수 기록을 따른다. 아래 공통 수동 명령도 같은 검증 기준으로 사용할 수 있다.

Windows 산출물은 [Test-WindowsPackage.ps1](../../tools/Test-WindowsPackage.ps1)로 검사한다. 2026-09-13 실제 UAT는 지정한 archive 루트에 실행 파일을 직접 만들었으므로 `Windows/Windows/`를 강제하지 않는다. archive 루트 또는 그 아래 Windows 중 정확히 한 패키지만 허용하고 실제 런타임·쿠킹 콘텐츠·해시를 확인한다. 초기 경로 가정 오류는 원래 실패 결과에 보존했으며 수정한 검사로 기존 산출물을 재검증했다.

Windows PowerShell 또는 PowerShell에서 저장소 루트로 이동한 뒤 같은 세션에서 필요한 단계만 순서대로 실행한다. 엔진 경로는 PC에 맞게 바꾸되 `Build.version`이 DEC-009의 후보와 일치해야 한다. 실행할 때마다 새 Run 디렉터리를 만든다.

```powershell
$ProjectRoot = (Get-Location).Path
$ProjectFile = Join-Path $ProjectRoot 'Mobile_defense_clone.uproject'
$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$UatBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$EditorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$RunId = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$RunRoot = Join-Path $ProjectRoot "Saved\BuildRuns\$RunId"
$LogRoot = Join-Path $RunRoot 'Logs'
foreach ($RequiredPath in @($ProjectFile, $BuildBat, $UatBat, $EditorExe)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) { throw "경로 없음: $RequiredPath" }
}
New-Item -ItemType Directory -Path $LogRoot -ErrorAction Stop | Out-Null

function Invoke-RecordedNative {
    param([string]$File, [string[]]$Arguments, [string]$LogName)
    $LogFile = Join-Path $LogRoot $LogName
    @{ File = $File; Arguments = $Arguments; Started = (Get-Date).ToString('o') } |
        ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $LogFile -Encoding UTF8
    & $File @Arguments 2>&1 | Tee-Object -FilePath $LogFile -Append
    $NativeExitCode = $LASTEXITCODE
    Add-Content -LiteralPath $LogFile -Value "ExitCode=$NativeExitCode"
    if ($NativeExitCode -ne 0) { throw "실패: $File (exit=$NativeExitCode), 로그: $LogFile" }
}

Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine\Build\Build.version') |
    Set-Content -LiteralPath (Join-Path $LogRoot 'engine-version.json') -Encoding UTF8
git rev-parse HEAD | Set-Content (Join-Path $LogRoot 'git-head.txt')
git status --short | Set-Content (Join-Path $LogRoot 'git-status.txt')
$PSVersionTable | Out-String | Set-Content (Join-Path $LogRoot 'powershell.txt')
```

`git-head.txt`만으로 미커밋·미추적 입력을 식별할 수 없다. 해당 입력의 스냅샷 또는 파일별 해시와 변경 내역을 별도로 남긴다. `Saved/`는 Git 제외 경로이므로 최종 증거는 [증거 보관 규칙](../production/EVIDENCE.md)에 따라 복사하고 해시를 확인한다.

## 3. 도구 버전 적용과 확인

### Windows

설치 후 `Config/DefaultEngine.ini`의 **기존** Windows 설정 섹션에 아래 키를 병합한다. `<...>`는 설치한 MSVC의 UBT 식별 버전으로 바꾼다. 같은 키를 중복 추가하지 않는다. Windows 도구 선택 키는 RUN-20260913-02에서 적용했다. 새 PC에서는 실제 설치 경로를 다시 확인한다.

```ini
[/Script/WindowsTargetPlatform.WindowsTargetSettings]
Compiler=VisualStudio2026
CompilerVersion=<설치 후 확인한 MSVC 14.50의 정확한 UBT 버전>
WindowsSDKVersion=10.0.26100.0
```

`CompilerVersion`은 실제 설치 경로·UBT 보고와 맞춰 지정한다. 설치 디렉터리 번호와 `cl.exe` ProductVersion을 별도 기록하고, 실제 컴파일러가 DEC-010의 14.50.35723 이상인 14.50 계열인지 확인한다. 설치 후 확인 예시:

```powershell
$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Invoke-RecordedNative -File $VsWhere -Arguments @('-products', '*', '-format', 'json') -LogName 'visual-studio.txt'
$CompilerExe = Read-Host '선택한 VS 2026의 cl.exe 전체 경로 (VC/Tools/MSVC/.../bin/Hostx64/x64/cl.exe)'
(Get-Item -LiteralPath $CompilerExe).VersionInfo |
    Format-List FileName, ProductVersion, FileVersion | Out-String |
    Set-Content (Join-Path $LogRoot 'msvc.txt')
Get-Item -LiteralPath (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include\10.0.26100.0\um\Windows.h') |
    Select-Object FullName | Out-String | Set-Content (Join-Path $LogRoot 'windows-sdk.txt')
$BundledDotnet = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
Invoke-RecordedNative -File $BundledDotnet -Arguments @('--info') -LogName 'bundled-dotnet.txt'
if (Get-Command dotnet -ErrorAction SilentlyContinue) {
    Invoke-RecordedNative -File (Get-Command dotnet).Source -Arguments @('--list-sdks') -LogName 'system-dotnet.txt'
}
```

위 .NET 경로는 현재 Windows x64 환경용이다. UE의 `GetDotnetPath.bat`는 기본적으로 엔진 동봉 .NET 10.0을 선택한다. 시스템 설치 목록과 실제 UBT/UAT 사용값을 구분하고, `UE_USE_SYSTEM_DOTNET` 등 기존 환경 설정 때문에 선택이 달라지면 그 이유·정확한 패치를 기록한다. 파일 존재 확인은 구성 요소 설치·빌드 성공의 증거가 아니다.

### Android

`Config/DefaultEngine.ini`의 기존 섹션에 도구 선택 키와 DEC-018의 초기 앱·호환성 기준을 병합한다. SDKAPILevelOverride는 빌드 도구의 플랫폼 선택이며 MinSDKVersion·TargetSDKVersion과 구분한다.

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
SDKAPILevelOverride=android-36
BuildToolsOverride=36.0.0
MinSDKVersion=26
TargetSDKVersion=36
PackageName=com.luckyworkshop.defense.prototype
ApplicationDisplayName=행운공방 디펜스 (개발)
bBuildForES31=True
bSupportsVulkan=False
bSupportsVulkanSM5=False
Orientation=Portrait
```

`bBuildForES31`은 UE 5.8.2에서 OpenGL ES 3.2를 선택하는 키다. Mobile Forward·베이크 조명 설정은 개발 환경 안내의 5절을 함께 적용하고, 같은 RendererSettings 섹션에 DEC-018의 HDR·AA 값을 병합한다.

```ini
[/Script/Engine.RendererSettings]
r.MobileHDR=True
r.Mobile.AntiAliasing=3
r.MSAACount=2
```

에디터 재시작·셰이더 컴파일 후 적용값을 확인한다. MSAA 2x는 초기 요청값이며 실제 기기 적용은 실행 진단으로 확인한다. 최소 SDK 26은 설치 OS 하한으로, 최소 지원 기기나 성능 보장과 구분한다. 출시 앱 식별자·서명은 별도 결정 사항이다.

설치한 경로를 입력하고 현재 PowerShell 및 자식 프로세스에 적용한다. 다음 값은 시스템 전체 환경 변수를 영구 변경하지 않는다.

```powershell
$AndroidSdkRoot = Read-Host 'API 36과 Build Tools 36.0.0이 설치된 Android SDK 전체 경로'
$AndroidJdkRoot = Read-Host 'OpenJDK 21.0.3 전체 경로'
$AndroidNdkRoot = Join-Path $AndroidSdkRoot 'ndk\27.2.12479018'
$env:ANDROID_HOME = $AndroidSdkRoot
$env:NDKROOT = $AndroidNdkRoot
$env:JAVA_HOME = $AndroidJdkRoot
$AdbExe = Join-Path $AndroidSdkRoot 'platform-tools\adb.exe'
foreach ($AndroidToolPath in @(
    (Join-Path $AndroidSdkRoot 'platforms\android-36\android.jar'),
    (Join-Path $AndroidSdkRoot 'build-tools\36.0.0\aapt2.exe'),
    (Join-Path $AndroidNdkRoot 'source.properties'),
    (Join-Path $AndroidJdkRoot 'bin\java.exe'), $AdbExe
)) {
    if (-not (Test-Path -LiteralPath $AndroidToolPath)) { throw "도구 없음: $AndroidToolPath" }
}
@{ SDK = $AndroidSdkRoot; NDK = $AndroidNdkRoot; JDK = $AndroidJdkRoot } |
    ConvertTo-Json | Set-Content (Join-Path $LogRoot 'android-paths.json')
Get-Content (Join-Path $AndroidNdkRoot 'source.properties') |
    Set-Content (Join-Path $LogRoot 'ndk-version.txt')
Get-Content (Join-Path $AndroidSdkRoot 'build-tools\36.0.0\source.properties') |
    Set-Content (Join-Path $LogRoot 'android-build-tools.txt')
Invoke-RecordedNative -File (Join-Path $AndroidJdkRoot 'bin\java.exe') -Arguments @('-version') -LogName 'java.txt'
Invoke-RecordedNative -File $AdbExe -Arguments @('version') -LogName 'adb-version.txt'
```

Android Studio의 Help > About에서 버전을 기록한다. UE의 Android SDK 설정에 `SDKPath`·`NDKPath`·`JavaPath`가 이미 지정되어 있거나 AutoSDK가 활성화되어 있으면 환경 변수만으로 선택값이 고정됐다고 판단하지 않는다. 해당 설정과 실행 로그를 확인해 같은 경로를 사용하도록 맞춘 후 빌드한다.

Android 패키징 전에는 DEC-013의 Mobile Forward·단순 베이크 조명 방향에 맞춰 개발 환경 안내의 설정·맵 조명 작업을 적용하고 저장한다. 초기 패키지는 DEC-015의 Development APK·게임 데이터 내부 포함, DEC-016의 ARM64 단일 ABI, DEC-017의 ETC2 단일 쿠킹 대상으로 설정한다. 앱 식별자, 최소·대상 SDK, 그래픽 API·HDR/AA는 위 DEC-018 기준을 적용한다. 처음 Gradle 의존성을 받는 실행에는 네트워크가 필요할 수 있으며 다운로드된 도구·의존성이 달라지면 실행 기록에 남긴다.

## 4. 프로젝트 파일 생성과 에디터

```powershell
Invoke-RecordedNative -File $BuildBat -Arguments @(
    '-projectfiles', "-project=$ProjectFile", '-game', '-engine', '-2026'
) -LogName 'generate-project-files.txt'

Invoke-RecordedNative -File $BuildBat -Arguments @(
    'Mobile_defense_cloneEditor', 'Win64', 'Development',
    "-Project=$ProjectFile", '-WaitMutex', '-NoHotReloadFromIDE'
) -LogName 'build-editor.txt'

& $EditorExe $ProjectFile '/Game/TopDown/Lvl_TopDown' '-log' "-abslog=$LogRoot\editor.log"
```

예상 산출물은 루트의 `.sln`, `Intermediate/ProjectFiles/`, `Binaries/Win64/UnrealEditor-Mobile_defense_clone.dll`이다. 실제 생성 파일과 로그를 확인한다. 에디터에서 기본 맵이 열리고 PIE로 플레이를 시작·종료할 수 있는지 관찰한 뒤 에디터를 종료한다. 현재 맵은 기존 템플릿이며 디펜스 기능 통과로 기록하지 않는다.

## 5. Windows Development 패키징·실행

```powershell
$WindowsArchive = Join-Path $RunRoot 'Packages\Windows'
Invoke-RecordedNative -File $UatBat -Arguments @(
    'BuildCookRun', "-project=$ProjectFile", '-noP4', '-unattended', '-utf8output',
    '-target=Mobile_defense_clone', '-platform=Win64', '-clientconfig=Development',
    '-build', '-cook', '-stage', '-pak', '-package', '-archive',
    '-map=/Game/TopDown/Lvl_TopDown', "-archivedirectory=$WindowsArchive"
) -LogName 'package-windows.txt'
Get-ChildItem -LiteralPath $WindowsArchive -Recurse -File |
    Select-Object FullName, Length | Out-String -Width 300 |
    Set-Content (Join-Path $LogRoot 'windows-artifacts.txt')
```

정상 종료 코드와 함께 실행 파일·쿠킹 콘텐츠가 이번 Run 폴더에 생성됐는지 확인한다. UAT가 출력한 실제 배포 루트를 사용한다. 일반적인 Windows 산출물 구조에서는 다음과 같이 부트스트랩 실행 파일을 실행한다. 경로가 다르면 파일 목록으로 확인한 경로로 수정한다.

```powershell
$WindowsExe = Join-Path $WindowsArchive 'Windows\Mobile_defense_clone.exe'
if (-not (Test-Path -LiteralPath $WindowsExe)) { throw '산출물 목록에서 실행 파일 경로를 확인하세요.' }
& $WindowsExe '-log' "-abslog=$LogRoot\windows-game.log"
```

맵 로딩, 기본 입력, 정상 종료를 확인한다. 패키징 종료 코드 0만으로 실행 확인을 Pass 처리하지 않는다. 초기 검증은 현재 맵과 참조 콘텐츠 범위이며, 디펜스 맵을 추가한 뒤에는 `-map`과 기본 맵을 함께 갱신한다.

## 6. Android Development 패키징·실기기

DEC-015에 따라 초기 기기 검증에는 **Development APK에 게임 데이터를 포함**한다. 3절의 Android 선행 조건을 완료하고 `Config/DefaultEngine.ini`의 기존 AndroidRuntimeSettings 섹션에 다음 키를 병합한 뒤 실행한다. 현재 문서 작성으로 Config가 변경되지는 않았다.

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bEnableBundle=False
bPackageDataInsideApk=True
bBuildForArm64=True
bBuildForX8664=False
```

아래 명령의 `-clientconfig=Development`를 유지하고 ABI는 DEC-016의 ARM64만 포함한다. DEC-017에 따라 `-cookflavor=ETC2`를 명시한다. 패키징과 서명은 개발 검증용으로 확인하고 스토어 배포용 설정은 출시 준비 시 별도로 정한다.

```powershell
$AndroidArchive = Join-Path $RunRoot 'Packages\Android'
Invoke-RecordedNative -File $UatBat -Arguments @(
    'BuildCookRun', "-project=$ProjectFile", '-noP4', '-unattended', '-utf8output',
    '-target=Mobile_defense_clone', '-platform=Android', '-clientconfig=Development',
    '-cookflavor=ETC2', '-build', '-cook', '-stage', '-pak', '-package', '-archive',
    '-map=/Game/TopDown/Lvl_TopDown', "-archivedirectory=$AndroidArchive"
) -LogName 'package-android.txt'
Get-ChildItem -LiteralPath $AndroidArchive -Recurse -File |
    Select-Object FullName, Length | Out-String -Width 300 |
    Set-Content (Join-Path $LogRoot 'android-artifacts.txt')
Invoke-RecordedNative -File $AdbExe -Arguments @('devices', '-l') -LogName 'android-devices.txt'
```

이번 Run의 UAT 명령과 쿠킹 로그에서 Android_ETC2 대상을 확인하고 최종 APK 경로·크기·해시를 연결한다. Android_ASTC나 Android_Multi를 대상으로 실행했다면 DEC-017의 실행 증거로 기록하지 않는다. 검증 대상 APK에 쿠킹한 게임 데이터가 포함됐는지 생성 Gradle 설정·패키징 로그와 APK 내부 파일로 확인한다. 중간 빌드 폴더에 데이터 파일이 존재하는지만으로 외부 배포 파일이 필요하다고 판단하지 않는다. 설치·실행은 지정한 APK 하나로 완료되어야 하며, 검증 기기의 이전 빌드가 남긴 외부 데이터에 의존하지 않는 조건인지 기록한다. 여러 APK가 생성되면 해당 Run의 ETC2·ARM64 변형과 해시를 명시한다.

1. 기기의 개발자 옵션·USB 디버깅을 켜고 연결 요청을 기기에서 허용한다. `adb devices -l`에서 상태가 `device`인지 확인한다.
2. 이번 Run에서 생성된 데이터 포함 APK를 선택하고 아래 명령으로 지정 기기에 설치한다.
3. 설치 후 기기에서 앱 아이콘을 눌러 시작한다. 게임 콘텐츠의 추가 복사·다운로드 없이 기본 맵이 표시되는지, 터치 반응·종료 및 크래시 유무를 기록한다. 이 조건은 게임 플레이의 네트워크 사용을 금지한다는 의미가 아니다.

```powershell
$DeviceSerial = Read-Host 'adb devices에 표시된 검증 기기 시리얼'
if ([string]::IsNullOrWhiteSpace($DeviceSerial)) { throw '기기를 지정하세요.' }
Invoke-RecordedNative -File $AdbExe -Arguments @('-s', $DeviceSerial, 'shell', 'getprop', 'ro.product.cpu.abilist') -LogName 'android-device-abis.txt'
```

기기 ABI 출력에 `arm64-v8a`가 포함된 것을 확인한 뒤 다음 APK 검사·설치를 실행한다. ABI 목록을 읽지 못했거나 해당 ABI가 없으면 설치 단계로 넘어가지 않는다.

```powershell
$ApkPath = Read-Host '이번 Run의 데이터 포함 APK 전체 경로'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$ApkArchive = [System.IO.Compression.ZipFile]::OpenRead($ApkPath)
try {
    $ApkAbis = @($ApkArchive.Entries | ForEach-Object {
        if ($_.FullName -match '^lib/([^/]+)/[^/]+\.so$') { $Matches[1] }
    } | Sort-Object -Unique)
    $ApkAbis | Set-Content (Join-Path $LogRoot 'android-apk-abis.txt')
    if ($ApkAbis.Count -ne 1 -or $ApkAbis[0] -ne 'arm64-v8a') {
        throw "DEC-016 ABI 불일치: $($ApkAbis -join ', ')"
    }
} finally {
    $ApkArchive.Dispose()
}
$Aapt2Exe = Join-Path $AndroidSdkRoot 'build-tools\36.0.0\aapt2.exe'
Invoke-RecordedNative -File $Aapt2Exe -Arguments @('dump', 'badging', $ApkPath) -LogName 'android-apk-metadata.txt'
Invoke-RecordedNative -File $Aapt2Exe -Arguments @('dump', 'xmltree', $ApkPath, '--file', 'AndroidManifest.xml') -LogName 'android-apk-manifest.txt'
```

설치 전에 AAPT2 출력의 패키지명 `com.luckyworkshop.defense.prototype`, 최소 SDK 26, 대상 SDK 36, 앱 표시 이름 `행운공방 디펜스 (개발)`을 확인한다. 생성 Manifest의 그래픽 요구 선언도 OpenGL ES 3.2 선택과 대조한다. 플러그인·생성 설정 때문에 값이 다르면 원인을 해결한 새 APK로 다시 검사한 뒤 설치한다. 한글 출력의 인코딩 문제가 있으면 리소스·설치 후 표시를 함께 확인하며 깨진 출력만으로 이름을 판정하지 않는다.

```powershell
Invoke-RecordedNative -File $AdbExe -Arguments @('-s', $DeviceSerial, 'install', '-r', $ApkPath) -LogName 'android-install.txt'
```

설치·실행에 별도 OBB/AFS 전송이 필요하거나 AAB만 생성됐다면 DEC-015의 검증 기준을 충족하지 않은 것으로 기록하고 패키징 설정을 점검한다. 초기 절차에서 외부 데이터 설치로 자동 전환하지 않는다. 콘텐츠 증가로 APK 크기·전송 시간·설치에 문제가 생기면 실제 크기와 실패 로그를 근거로 패키지 방식을 재검토한다.

기기에서 앱을 실행·확인한 직후 로그를 수집한다. 오류가 발생하면 재현 시각과 증상을 함께 남긴다.

```powershell
Invoke-RecordedNative -File $AdbExe -Arguments @(
    '-s', $DeviceSerial, 'logcat', '-d', '-v', 'threadtime', 'UE:V', 'AndroidRuntime:E', '*:S'
) -LogName 'android-runtime.txt'
```

위 필터로 원인이 안 보이면 재현 시각·앱 프로세스에 맞춰 필요한 로그 범위를 넓힌다. 실기기 30FPS·SafeArea·카메라와 전투 성능 검수는 TASK-MOB-01의 별도 결과로 남긴다.

DEC-018 검증에서는 실제 선택된 RHI가 OpenGL ES인지, 기기의 API·드라이버 지원과 HDR·AA 적용값이 기준에 맞는지 기록한다. 개발 빌드의 로그·콘솔 조회로 `r.MobileHDR`, `r.Mobile.AntiAliasing`, `r.MSAACount`를 확인하고 실제 렌더 타깃의 MSAA 샘플 수도 개발용 렌더링 진단으로 확인한다. 설정 요청값만 일치하고 실제 적용이 확인되지 않았다면 그 항목은 NotRun으로 남긴다. APK Manifest 검사로 실행 시 HDR·MSAA 검증을 대신하지 않는다.

DEC-014 화면·입력 검증에서는 세로 방향, 실제 화면비·DPI·SafeArea와 기본 HUD/상세 패널의 표시를 기록한다. 기본 맵 실행 확인 후 전용 HUD가 준비된 빌드에서 QA-VIS-02/03 및 QA-MOB-01/04를 수행한다. 기본 템플릿의 표시·터치 성공만으로 디펜스의 전체 핵심 흐름을 Pass 처리하지 않는다.

## 7. 판정·실패 확인·증거

아래는 새 실행을 판정할 기준이다. 과거 통과를 새 실행에 승계하지 않는다. 최신 플랫폼별 결과·잔여 문제는 [검수 기록](../production/TEST_RUNS.md)과 [완료 판정](../production/verification.json)을 따른다.

| 단계 | Pass 기준 | 우선 확인할 위치 |
|---|---|---|
| 환경 | 해당 플랫폼의 DEC-009/010/011 버전·경로와 실제 선택값 일치 | 이번 Run의 버전 파일, UBT/UAT 도구 선택 로그 |
| 프로젝트 파일 | 명령 성공 및 프로젝트 파일 생성 | project-files.log 또는 generate-project-files.txt |
| Editor 빌드 | 명령 성공 및 모듈 생성 | editor.log 또는 build-editor.txt, UBT 상세 로그 |
| Editor 실행 | 기본 맵·PIE 시작/입력/종료 확인 | editor-play.log와 관찰 기록 |
| PC 빌드 | 컴파일·쿠킹·패키징 성공, 실행 파일·콘텐츠 확인 | windows-package.log 또는 package-windows.txt |
| PC 실행 | 패키지에서 맵·입력·정상 종료 확인 | windows-game.log와 관찰 기록 |
| Android 빌드 | Development·ETC2·데이터 포함 APK·ARM64 단일 ABI 확인 | 패키징 로그와 APK 검사 |
| Android 앱 정보 | 앱 식별자·SDK·그래픽 요구 선언 대조 | APK 메타데이터 |
| Android 실행 | APK 설치, 외부 게임 데이터 없이 표시·터치·종료 | 기기 관찰과 로그 |
| Android 렌더링 | OpenGL ES·HDR·MSAA·베이크 조명 표시 확인 | 기기 렌더링 진단 기록 |

실패하면 다음 단계로 넘어가지 않고 첫 원인 오류를 기록한다. UAT 요약의 `Unknown Error`만 남기지 말고 해당 실행의 UBT·Cook·Gradle 로그를 함께 보관한다. 상세 로그의 실제 경로는 UAT 출력에 표시되며 일반적으로 사용자 AppData의 AutomationTool/UnrealBuildTool 로그 폴더와 프로젝트 `Saved/Logs/`에서 확인한다. 설치 누락, 버전 선택 불일치, 컴파일, 쿠킹, 패키징, 설치, 실행을 구분해 재현 명령과 수정 후 새 Run을 연결한다.

`production/TEST_RUNS.md`에는 Run ID·일시·실행자, 소스 입력 식별자, 엔진/도구 패치와 실제 경로, 명령·종료 코드, 빌드 설정·맵·모바일 설정, 산출물 위치·해시, 기기 모델·OS·해상도, 단계별 결과, 실패 원인·재검증을 기록한다. 검증 파일을 `docs/production/evidence/` 또는 팀 보관 위치로 복사해 링크하고, 패키지·대용량 로그는 보관 위치와 해시로 식별한다. 외부 공유본에서는 개인 경로·기기 식별자 등 불필요한 정보를 제외한다.

## 8. 보조 UI와 확인 근거

에디터의 Platforms > Project Launcher에서 같은 플랫폼·Development·맵·쿠킹 조건의 프로필을 만들면 생성된 UAT 명령을 비교할 수 있다. 메뉴 실행으로 검증했어도 실제 명령·도구 버전·로그·산출물은 같은 Run 형식으로 남긴다.

- [Epic 빌드 작업 안내](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine): UAT/BuildCookRun과 단계, Project Launcher의 명령 생성.
- [Epic 빌드 설정](https://dev.epicgames.com/documentation/unreal-engine/build-configuration-for-unreal-engine): UBT 설정의 역할.
- [Android adb 안내](https://developer.android.com/tools/adb), [Logcat 안내](https://developer.android.com/tools/logcat): 기기 지정, APK 설치와 로그 수집.
- [AAPT2 안내](https://developer.android.com/tools/aapt2): dump badging과 dump xmltree로 최종 APK 앱 정보·Manifest 검사.
- UE 5.8.2 로컬 소스: `Engine/Build/BatchFiles/Build.bat`, `GetDotnetPath.bat`; `Engine/Source/Programs/UnrealBuildTool/Platform/Windows/UEBuildWindows.cs`; Android의 `AndroidPlatformSDK.cs`, `AndroidToolChain.cs`, `UEDeployAndroid.cs`; `Engine/Source/Programs/AutomationTool/AutomationUtils/ProjectParams.cs`, `Android/AndroidPlatform.Automation.cs`. 위 설정 키와 명령 인자를 읽기 전용으로 확인했다.
