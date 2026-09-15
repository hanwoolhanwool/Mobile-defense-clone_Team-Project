---
id: TECH-SETUP
version: 0.1.13
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

# 개발 환경과 프로젝트 시작 안내

[기획 허브](../README.md) · [프로젝트 구조](ARCHITECTURE.md) · [결정 기록](../DECISIONS.md) · [개발 작업](../BACKLOG_QA.md)

이 문서는 개발 환경의 결정과 실제 확인 결과를 기록하는 원본이다. 문서 전체는 Draft이며, 프로젝트 출발점은 DEC-008, 엔진 기준 후보와 고정 절차는 DEC-009, Windows 개발 도구의 권장 기준은 DEC-010, Android 도구의 검증 기준은 DEC-011을 따른다. 설치된 도구와 빌드 검증을 마친 팀 공통 버전은 구분한다.

## 1. 확정한 프로젝트 출발점

현재 저장소의 `Mobile_defense_clone.uproject`와 C++ 모듈 `Mobile_defense_clone`을 유지하고, 그 안에 디펜스 전용 구조를 추가한다. 새 Blank 프로젝트 생성이나 프로젝트·모듈 이름 변경을 착수 절차에 포함하지 않는다.

| 대상 | 적용 기준 |
|---|---|
| 프로젝트 파일 | 저장소 루트의 `Mobile_defense_clone.uproject` 사용 |
| C++ 코드 | 기존 `Source/Mobile_defense_clone/` 안에 기능별 디렉터리 추가 |
| 디펜스 콘텐츠 | `Content/LD/` 아래에 전용 맵·Blueprint·UI·데이터·아트 구성 |
| 빌드 타깃 | 기존 `Mobile_defense_clone` 및 `Mobile_defense_cloneEditor` 유지 |
| 기존 템플릿 | TopDown·Strategy·TwinStick 소스와 콘텐츠는 초기 참고용으로 보존 |
| 템플릿 정리 | 디펜스에서의 사용 여부와 에셋·클래스 참조를 확인한 뒤 필요한 정리 작업 수행 |

`Content/LD/`와 기능별 소스 하위 폴더는 앞으로 구현할 구조다. 이 문서 반영으로 해당 폴더나 게임 기능이 생성된 것은 아니다. 클래스·폴더의 상세 책임은 프로젝트 구조 문서에서 관리한다.

## 2. 현재 확인한 설치 환경

### 2026-09-13 TASK-CORE-01 실행 관찰

사용자 `계속 알아서 진행해줘` 지시에 따라 환경 적용·빌드 검증을 시작했다. 기존 입력은 `Saved/BuildRuns/RUN-20260913-02/Input/`에 606개 파일로 보존했고 SHA-256으로 복사 일치를 확인했다. 아래 실행 현황이 뒤의 2026-09-11 설치 전 관찰보다 최신이며, 모바일 설정·게임 구현 완료를 뜻하지 않는다.

| 항목 | 이번 관찰·적용 | 검증 경계 |
|---|---|---|
| UE | 5.8.2 / Changelist 56702186 | 후보 유지, Android 빌드 전 최종 고정 아님 |
| Visual Studio | Community 2026 18.10.12201.205, Stable. Microsoft 서명 유효 설치 파일 사용, 설치 종료 코드 0 | vswhere isComplete=True, isRebootRequired=False. 실제 빌드 결과는 검수 기록 참조 |
| MSVC | 디렉터리 14.50.35717, 실제 cl.exe 14.50.35738.0 | UBT 로그에서 실제 14.50.35738 선택 확인 |
| Windows SDK | 10.0.26100.0 헤더 및 UBT 선택 확인 | 실제 빌드 결과 별도 기록 |
| .NET | 시스템 SDK 10.0.201, UE 동봉 SDK 10.0.203 | UBT 실행은 UE 동봉 10.0 사용 |
| Windows 프로젝트 설정 | Compiler=VisualStudio2026, CompilerVersion=14.50.35717, WindowsSDKVersion=10.0.26100.0 적용 | 모바일 렌더링·앱 설정은 이번 Windows 작업에서 변경하지 않음 |
| Android 설치 현황 | Studio AI-252.27397.103.2522.14617522, 동봉 JDK 21.0.8, Build Tools 36.1.0, NDK 28.0.13004108/29.0.14206865 | DEC-011 목표 조합과 차이 있음. Android 도구 적용·빌드는 후속 작업 |

빌드 스크립트와 실행 범위는 [빌드 절차](BUILD_RUN.md), 진행 상태는 [작업 보드](../production/BOARD.md), 실제 통과·실패는 [검수 기록](../production/TEST_RUNS.md)을 따른다.

### 2026-09-11 설치 전 관찰

2026-09-11에 현재 개발 PC에서 읽기 전용으로 확인했다. 다음 값은 설치 현황이며 팀 공통 툴체인의 확정 또는 새 빌드 성공 기록이 아니다.

| 항목 | 관찰한 값 | 확인 근거 |
|---|---|---|
| UE | 5.8.2, Changelist 56702186 | 설치 엔진의 `Engine/Build/Build.version` 및 기존 프로젝트 실행 로그 |
| 프로젝트 엔진 연결 | 5.8 | `.uproject`의 `EngineAssociation` |
| Visual Studio | 2022 Community 17.14.26 | `vswhere` 설치 정보 |
| MSVC | 설치 디렉터리 14.44.35207, 실제 컴파일러 14.44.35222.0 | Visual Studio 2022의 `VC/Tools/MSVC/` 및 `bin/Hostx64/x64/cl.exe`의 ProductVersion |
| Windows SDK | 10.0.22621.0 | Windows Kits의 `Include/` 설치 디렉터리 |
| .NET SDK | 미확인 | 별도 설치 목록 확인 필요 |
| Android SDK Platform | android-36 | 로컬 Android SDK의 `platforms/` 설치 디렉터리 |
| Android Build Tools | 36.1.0 | 로컬 Android SDK의 `build-tools/` 설치 디렉터리 |
| Android NDK | 28.0.13004108, 29.0.14206865 | 로컬 Android SDK의 `ndk/` 설치 디렉터리 |
| Android Studio·JDK 설치 | 미확인 | 설치 버전 및 실제 UE 빌드에서 사용할 경로·버전 확인 필요 |

MSVC·SDK 디렉터리 존재만으로 구성 요소의 완전성이나 UE 호환성을 판정하지 않는다. 기존 UE 실행 로그도 현재 소스의 클린 빌드와 Android 패키징 성공을 대신하지 않는다. 도구 버전 확정과 실행 증거는 후속 결정·검증으로 갱신한다.

### 2.1 엔진 기준 후보와 고정 조건

DEC-009에 따라 현재 설치된 **UE 5.8.2를 개발 기준 후보로 선택**한다. 동일 엔진 버전으로 PC·Android 빌드를 검증하고, 두 플랫폼 모두 통과하면 개발 기준 버전으로 고정한다. 후보 선택과 이 고정 절차는 사용자 확정 사항이며, 실제 검증·고정은 아직 완료되지 않았다.

| 항목 | 검증 범위·조건 | 현재 결과 |
|---|---|---|
| 기준 후보 | UE 5.8.2, 현재 설치본 Changelist 56702186 | 후보 선택 확정 |
| PC 빌드 | 별도 작업 폴더의 Windows Development 컴파일·쿠킹·패키징, 사용 엔진·컴파일러·SDK와 로그 기록 | Pass — RUN-20260913-02/03. 외부 스크립트 최초 실패와 수정 후 산출물 판정은 검수 기록 참조 |
| Android 빌드 | 같은 UE 5.8.2에서 Android 검증 빌드의 컴파일·쿠킹·패키징, 사용 SDK·NDK·JDK와 로그 기록 | NotRun |
| 개발 기준 고정 | 위 두 빌드 모두 Pass인 실행 기록을 연결하고 실제 성공한 도구 조합을 기록 | 검증 대기 |

엔진 기준 검증은 현재 프로젝트에서 가능한 기본 빌드로 수행하며, 디펜스 전투 전체 완성을 기다리지 않는다. Android 실기기의 터치·카메라·SafeArea·성능 검수는 TASK-MOB-01에서 이어가고 빌드 통과와 구분한다. 후보 선택만으로 TASK-CORE-01 또는 TASK-MOB-01을 완료 처리하지 않는다.

한 플랫폼이라도 실패하면 실패 원인과 사용한 도구 조합을 남기고 수정 후 재검증한다. 엔진 버전을 변경할 필요가 생기면 새 후보와 변경 근거를 다음 결정으로 기록한다. 고정 후에도 패치 변경 시 PC·Android 빌드 검증 결과를 갱신한다.

`.uproject`의 `EngineAssociation=5.8` 표기만으로 패치 버전이 고정됐다고 판단하지 않는다. 실제 빌드에 사용한 엔진의 `Build.version`과 빌드 로그를 근거로 패치·Changelist를 기록한다. 이번 결정 반영에서는 프로젝트 설정이나 설치 엔진을 변경하지 않았다.

### 2.2 Windows 개발 도구 권장 기준

DEC-010에 따라 **UE 5.8의 공식 권장을 기준으로 선택한 아래 조합**을 사용한다. 도구 설치·Windows 설정 적용·기본 빌드 검증은 RUN-20260913-02/03에서 확인했다. 정확한 설치 관찰은 이 문서 2절, 실제 실행 결과는 검수 기록이 원본이다.

| 항목 | 선택한 기준 | 적용·검증 상태 |
|---|---|---|
| Visual Studio | 2026, 18.10.0 Stable | 2026-09-13 설치·Windows 검증 확인 |
| MSVC | 14.50 계열 최신 패치, 실제 컴파일러 버전 14.50.35723 이상 | UBT의 14.50.35738 선택 확인 |
| Windows SDK | 10.0.26100.0 | 설치·UBT 선택 확인 |
| .NET SDK | 10.0 계열 | 시스템 10.0.201·UE 동봉 10.0.203 관찰 |

2026-09-11 확인 기준으로 Epic은 UE 5.8 일반 개발에 Visual Studio 2026을 권장하며 MSVC 14.50, Windows SDK 10.0.26100 이상, VS 2026용 .NET 10.0을 안내한다. 이 프로젝트는 그 범위에서 위 조합을 선택했다. [Epic 개발 환경 안내](https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)

Visual Studio 18.10.0은 2026-09-08에 출시된 Stable이며 이번 결정 시점의 기준이다. 문서의 버전이 이후 최신 릴리스에 자동으로 따라가는 의미는 아니다. [Microsoft 릴리스 기록](https://learn.microsoft.com/en-us/visualstudio/releases/2026/release-history)

설치된 UE 5.8.2의 `Engine/Config/Windows/Windows_SDK.json`도 MSVC 14.50 계열을 우선하며 실제 컴파일러 14.50.35722 이하의 해당 계열을 제외한다. 따라서 14.50 계열에서 수정 패치를 선택하고 설치 디렉터리 버전과 `cl.exe`의 실제 버전을 함께 기록한다.

적용 시 다음을 확인한다.

1. Visual Studio Installer에서 MSVC 14.50 계열과 Windows SDK 10.0.26100.0을 개별 구성 요소로 선택하고, 정확한 MSVC·.NET SDK 패치를 기록한다.
2. 프로젝트 빌드 설정에 사용할 MSVC·Windows SDK 버전을 명시한다. 구체적인 설정 파일·명령은 빌드·실행 절차에서 정리하며 설치된 엔진의 내부 SDK 기준 파일을 수정하는 방식으로 고정하지 않는다.
3. 실제 빌드 로그에서 선택된 컴파일러·SDK가 지정값과 일치하는지 확인한다. 설치 성공만으로 프로젝트 빌드 통과를 기록하지 않는다.
4. 성공한 실행에는 엔진 버전, Visual Studio 버전, MSVC 설치 디렉터리·실제 컴파일러 버전, Windows SDK, .NET SDK, 명령과 로그를 남긴다.

현재 설치된 도구 목록은 관찰 기록으로 유지한다. 실제 설치·검증 후 그 목록과 성공한 도구 조합을 갱신하며, 엔진 최종 고정에는 DEC-009의 Android 빌드 조건도 충족해야 한다. Android 도구 조합은 아래 DEC-011 기준을 따른다.

### 2.3 Android 개발 도구 검증 기준

DEC-011에 따라 아래 조합을 Android 빌드의 검증 기준으로 선택한다. 사용자가 조합을 확정했으며, 해당 조합으로 설치·설정·패키징을 완료했다는 의미는 아니다.

| 항목 | 선택한 기준 | 적용·검증 상태 |
|---|---|---|
| Android Studio | Koala 2024.1.2 Patch 1 | 설치 확인·검증 대기 |
| Android SDK Platform | API 36 (`android-36`) | 설치 디렉터리 확인, UE 선택·검증 대기 |
| Android Build Tools | 36.0.0 | 설치·검증 대기 |
| Android NDK | r27c (`27.2.12479018`) | 설치·검증 대기 |
| JDK | OpenJDK 21.0.3 | 설치·검증 대기 |

2026-09-11에 확인한 Epic 안내와 설치된 UE 5.8.2의 파일에는 SDK·Build Tools 버전 차이가 있다. 이번 선택은 SDK Platform·Build Tools·NDK에 설치 엔진의 SDK 메타데이터를 우선 적용하고, Android Studio·JDK에는 Epic 안내를 적용한 검증 조합이다.

| 확인 근거 | SDK Platform | Build Tools | 선택에 사용한 범위 |
|---|---|---|---|
| [Epic Android 시작 안내](https://dev.epicgames.com/documentation/unreal-engine/android-quick-start) | 권장 API 35 | 35.0.1 | Android Studio Koala 2024.1.2 Patch 1, OpenJDK 21.0.3 |
| UE 5.8.2 `Engine/Config/Android/Android_SDK.json` | android-36 | 36.0.0 | SDK Platform·Build Tools 및 NDK r27c/27.2.12479018 |
| UE 5.8.2 `Engine/Extras/Android/SetupAndroid.bat`의 수동 실행 기본값 | android-36 | 36.1.0 | 메타데이터와 다른 기본값이 있음을 확인 |

적용 시 설치 스크립트의 기본값이나 가장 높은 설치 버전을 그대로 사용하지 않고 위 기준 버전과 실제 선택 경로를 확인한다. 구체적인 설치·빌드 명령은 [빌드·실행 절차](BUILD_RUN.md)를 따르며, 엔진 내부 SDK 메타데이터를 수정해 버전을 맞추지 않는다. 기존 설치 목록의 NDK 28/29와 Build Tools 36.1.0은 관찰 기록으로 유지한다.

SDK Platform API 36 선택은 사용할 SDK의 기준이다. 앱의 최소 SDK 26·대상 SDK 36은 DEC-018, ARM64 ABI는 DEC-016을 따른다. 렌더링 방향은 DEC-013과 DEC-018을 따르며 실제 설정 적용·검증은 남아 있다.

실제 검증에서는 Android Studio 버전, SDK·Build Tools·NDK 버전과 경로, `java -version` 결과, UE가 선택한 도구의 로그, 실행 명령과 패키징 결과를 함께 기록한다. 현재 Android 컴파일·쿠킹·패키징 결과는 **NotRun**이며, DEC-009의 PC·Android 빌드 통과 후 엔진 고정 조건을 유지한다.

로컬 Android File Server 토큰은 Git 제외 파일 `Config/UserEngine.ini`의 동일 설정 섹션에 보관한다. UE 5.8.2 `ConfigHierarchy.h`의 GameDirUser 계층을 사용한다. 기준 커밋에는 토큰을 넣지 않는다. 새 PC에서 AFS를 사용하려면 로컬 토큰을 먼저 설정하고 적용값을 확인한다. 이 분리 이후 Android 패키징은 미실행이며 기존 Windows 실행을 재검증한 것으로 표시하지 않는다.

## 3. 현재 프로젝트에서 시작할 때

1. 저장소 루트의 프로젝트 파일과 `Source/`, `Config/`, `Content/`를 기준으로 작업한다.
2. 현재 기본 맵 `/Game/TopDown/Lvl_TopDown`과 `BP_TopDownGameMode`를 기존 템플릿 설정으로 식별한다.
3. 디펜스의 CameraPawn·GameMode·전장 맵은 위의 전용 소스·콘텐츠 경로에 추가한다.
4. 디펜스 전장 구현 후 연결할 기본 맵·GameMode·입력·모바일 설정의 변경 목록과 검증 결과를 기록한다.
5. 프로젝트 기반 빌드는 TASK-CORE-01, 보드·카메라는 TASK-MAP-01, Android 검증은 TASK-MOB-01의 완료 조건에 따라 확인한다.

## 4. 빌드·실행 절차

DEC-012에 따라 [PowerShell 빌드·실행 절차](BUILD_RUN.md)를 공통 실행 안내로 사용한다. 환경 확인 → 프로젝트 파일 생성 → Development Editor 빌드·실행 → Windows Development 패키징·실행 → Android Development 패키징·실기기 확인 → 결과 기록 순서다. 명령, 도구 선택 설정 키, 예상 산출물, 실패 로그 위치와 단계별 Pass 조건은 해당 문서에서 관리한다.

명령 안내 작성과 실제 실행은 구분한다. Windows 도구 적용·기본 빌드·실행은 RUN-20260913-02/03에서 확인했다. Android 도구 적용·패키징·실기기는 NotRun이며 현재 결과는 검수 기록을 따른다. Android 명령은 확정한 모바일 설정의 적용을 선행 조건으로 하며, 엔진 고정은 DEC-009의 양 플랫폼 빌드 통과 후에 수행한다.

## 5. 모바일 렌더링 기준과 적용 대기 항목

DEC-013으로 Mobile Forward와 사전 계산 조명 중심의 단순 라이팅을 확정했다. 상세 제작 기준과 재검토 조건은 [기술 명세 18.1](ARCHITECTURE.md)에 둔다. 다음은 2026-09-11에 `Config/DefaultEngine.ini`에서 읽은 값과 적용 목표다. 설치 엔진의 기능 지원 여부나 실제 기기에서 해당 기능이 동작했다는 관찰은 아니다.

| 설정 키 | 현재 파일 값 | 적용 목표·의미 |
|---|---|---|
| r.Mobile.ShadingPath | 1 | 0: Mobile Forward |
| r.AllowStaticLighting | False | True: 사전 계산 조명 생성·사용 허용 |
| r.DynamicGlobalIlluminationMethod | 1 | 0: Lumen 동적 GI 사용 해제, 베이크 조명은 별도 생성 |
| r.ReflectionMethod | 1 | 0: Lumen 반사 사용 해제, 필요한 환경 반사는 별도 제작 |
| r.Nanite.ProjectEnabled | True | False: 기존 일반 메시·LOD 제작안 적용 |
| r.Shadow.Virtual.Enable | 1 | 0: 기존 Virtual Shadow Maps 비활성 제작안 적용 |
| r.MobileHDR | True | True: DEC-018, 베이크 조명 중심의 Mobile HDR 활성 |
| r.Mobile.AntiAliasing | 2 | 3: DEC-018, MSAA 선택 |
| r.MSAACount | 4 | 2: DEC-018, MSAA 2x 요청 |
| r.ForwardShading | False | 모바일 전환에 사용하는 키가 아님. 이번 결정으로 데스크톱 Forward를 요구하지 않음 |

위 목표를 적용할 때 `[/Script/Engine.RendererSettings]`의 기존 키를 수정하고 중복 추가하지 않는다. 에디터를 재시작하고 셰이더 컴파일이 끝난 뒤 실제 설정을 확인한다. `r.AllowStaticLighting=True`만으로 조명 데이터가 생성되지는 않는다. 사용하는 전장 맵에서 사전 계산 조명을 허용하는지, 메시의 라이트맵 UV·해상도와 광원 설정이 준비됐는지 확인하고 조명을 빌드해 맵과 생성 데이터를 함께 저장한다. 현재 템플릿 및 앞으로 만들 디펜스 맵의 조명 구성은 아직 검증하지 않았다.

Directional Light 1개·단순 환경광과 베이크 배경을 기본으로 삼고, 움직이는 수호자·적의 밝기·바닥 그림자·공격 경고의 가독성을 별도 확인한다. DEC-018로 Mobile HDR On·MSAA 2x·OpenGL ES 3.2를 선택했다. 그림자 품질 조정과 현재 파일의 Ray Tracing 등 추가 옵션 검토는 설정 적용·기기 측정 작업에서 수행한다. 이번 문서 반영으로 Config를 변경하지 않는다.

| 적용·검증 항목 | 완료 증거 | 현재 결과 |
|---|---|---|
| 설정 반영 | 위 키의 변경 내역, 재시작·셰이더 컴파일 후 실제 모바일 렌더링 경로 확인 | NotRun |
| 맵 조명 제작 | 베이크 완료, 맵·조명 데이터 저장, 미빌드 조명 오류 확인 | NotRun |
| 모바일 표시 | 타깃 모바일 미리보기와 Android 패키지에서 배경·이동 액터·그림자·필수 VFX 가독성 확인 | NotRun |
| 실행·성능 | DEC-012의 빌드·실행 로그와 TASK-MOB-01의 기기·장면·프레임 측정 | NotRun |

PC 에디터 화면만으로 모바일 표시·성능을 판정하지 않는다. 검증할 장면의 배경 조명, 이동 액터, 최대 동시 VFX 조건을 기록하고 기존 30FPS 목표는 실측으로 확인한다. 결정 반영은 문서에 한정하며 프로젝트 설정·콘텐츠 적용과 베이크·패키징은 아직 수행하지 않았다.

근거: [Epic 모바일 렌더링 비교](https://dev.epicgames.com/documentation/en-us/unreal-engine/mobile-rendering-and-shading-modes-for-unreal-engine), [모바일 성능·조명 안내](https://dev.epicgames.com/documentation/en-us/unreal-engine/performance-guidelines-for-mobile-devices-in-unreal-engine), 설치된 UE 5.8.2의 `Engine/Source/Runtime/Engine/Classes/Engine/RendererSettings.h`에 정의된 설정 키와 재시작 조건.

## 6. 화면·입력 기준과 적용 대기 항목

DEC-014로 세로 고정, 다양한 세로 화면비·SafeArea 대응, 터치 우선을 확정했다. 1080×1920은 UI 설계 기준이다. 카메라·HUD·입력 동작의 원본은 [전장·모바일 UI 명세](../design/BOARD_UI.md)이며, 실제 렌더링 해상도나 DPI 곡선의 숫자를 이번 결정으로 고정하지 않는다.

| 대상 | 현재 관찰 | 적용·검증 기준 |
|---|---|---|
| Android 화면 방향 | 프로젝트 Config에 Orientation 명시 없음. 최종 생성 Manifest·기기 동작 미확인 | AndroidRuntimeSettings 섹션에 Orientation=Portrait를 적용하고 패키지의 실제 방향·기기 실행 확인 |
| 설계 해상도·DPI | 실제 UMG 위젯 구성·스케일 적용 미검증 | 1080×1920 설계 기준, 실제 뷰포트·DPI에 맞춘 배치와 터치 영역 확인 |
| SafeArea | 위젯 계층·기기별 안전 영역 미검증 | 핵심 HUD·버튼·패널 조작을 UMG Safe Zone 안에 배치, 노치·시스템 영역과 겹치지 않는지 확인 |
| 카메라 | 디펜스 전용 카메라의 화면비 대응 미검증 | 실제 전장 영역에 경로 네 모서리와 두 보드를 맞추고 화면 변화 후 입력 역투영 일치 확인 |
| 터치 | DefaultInput.ini에 bUseMouseForTouch=False, DefaultTouchInterface=None | 전용 버튼·터치 액션 연결 및 전체 핵심 흐름의 실기기 조작 확인. 이 두 값만으로 터치 지원 여부를 판정하지 않음 |

설정 적용 시 `Config/DefaultEngine.ini`의 `[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`에 `Orientation=Portrait`를 병합한다. 키와 값은 UE 5.8.2의 `Engine/Source/Runtime/Android/AndroidRuntimeSettings/Classes/AndroidRuntimeSettings.h`에서 확인했다. 실제 앱 화면은 실행 환경에서 관찰한 뷰포트 크기로 계산하고, 설정값만으로 화면 크기·안전 영역이 고정됐다고 가정하지 않는다.

검증은 QA-VIS-02의 화면비·SafeArea, QA-VIS-03의 두 관점 입력, QA-MOB-01의 드래그 취소, QA-MOB-04의 터치 전용 흐름으로 연결한다. 기본 HUD의 전체 전장 가시성과 상세 패널을 열었을 때의 허용된 가림은 UI 명세에 따라 구분한다. 기기·OS·실제 뷰포트·DPI·안전 영역, 화면별 표시·터치 결과를 기록한다. 설정·카메라·위젯 적용, 패키징 및 기기 검증 결과는 현재 **NotRun**이다.

## 7. Android 초기 검증 패키지

DEC-015에 따라 초기 실기기 검증은 **Development APK에 게임 데이터를 포함하는 방식**으로 수행한다. 검증 대상 APK 하나로 설치·맵 실행을 확인해 초기 설치 절차를 단순하게 유지한다. 콘텐츠가 늘면 APK 크기와 전송·설치 시간도 커지므로 실행 기록에 크기·해시를 남기고 실제 문제가 생기면 패키지 방식을 재검토한다. 스토어 배포 형식·출시 서명은 별도 결정 사항이다.

| 항목 | 현재 확인 | 적용 기준 |
|---|---|---|
| 빌드 구성 | DEC-012 명령에 Development 지정, 실행 미완료 | -clientconfig=Development 유지 |
| bEnableBundle | 프로젝트 Config에 명시 없음, 엔진 BaseEngine.ini 기본값 False | AndroidRuntimeSettings 섹션에 False 명시, APK 생성 확인 |
| bPackageDataInsideApk | 프로젝트 Config에 명시 없음, 엔진 BaseEngine.ini 기본값 false | AndroidRuntimeSettings 섹션에 True 명시, APK 내부 게임 데이터 포함 확인 |
| 설치·실행 | 현재 NotRun | 지정한 APK 하나로 설치·실행, 별도 OBB/AFS 전송이나 기존 외부 게임 데이터에 의존하지 않는지 확인 |

구체적인 설정 병합과 실행 명령은 [빌드·실행 안내](BUILD_RUN.md)에 둔다. 현재값은 파일을 읽은 관찰이며 최종 병합 설정·패키징 성공을 뜻하지 않는다. 설정 적용·패키징·기기 검증은 아직 실행하지 않았다. ABI는 DEC-016, 텍스처 쿠킹은 DEC-017, SDK·그래픽 API·앱 식별자는 DEC-018을 적용한 뒤 검증한다.

근거: [Epic Android 설정 안내](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-settings-in-the-unreal-engine-project-settings)의 데이터 포함·AAB 설정, UE 5.8.2 `Engine/Source/Runtime/Android/AndroidRuntimeSettings/Classes/AndroidRuntimeSettings.h` 및 `Engine/Config/BaseEngine.ini`의 키·기본값.

### 7.1 Android CPU 지원 범위

DEC-016에 따라 초기 Android 빌드는 **ARM64(arm64-v8a) 단일 ABI**로 구성한다. `Config/DefaultEngine.ini`의 `[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`에 다음 값을 명시해 빌드한다.

| 설정 | 현재 파일 관찰 | 적용 목표 |
|---|---|---|
| bBuildForArm64 | 프로젝트 Config에 명시 없음, UE 5.8.2 BaseEngine.ini 기본값 true | True |
| bBuildForX8664 | 프로젝트 Config에 명시 없음, UE 5.8.2 BaseEngine.ini 기본값 false | False |

기본값이 목표와 같아도 프로젝트 적용·패키징 검증이 완료된 것은 아니다. 생성 Gradle 설정·빌드 로그와 최종 APK의 `lib/` 아래 ABI 목록을 확인해 `arm64-v8a`만 포함됐는지 기록한다. 실제 기기에서는 `ro.product.cpu.abilist`에 `arm64-v8a`가 포함되는지 확인한 뒤 설치·실행한다. 기기의 64비트 CPU 탑재 여부만으로 OS의 앱 ABI 지원을 대신 판정하지 않는다.

이 선택은 초기 Android 바이너리의 CPU 범위다. Windows 개발 빌드 구성은 기존 기준을 유지하고, 기기 모델·최소/대상 SDK·GPU·그래픽 API·텍스처 지원과 실제 성능은 각각 별도로 확인한다. 설정 변경·APK 생성·기기 검증은 현재 **NotRun**이다.

근거: [Epic Android 시작 안내](https://dev.epicgames.com/documentation/unreal-engine/android-quick-start)의 64비트 ARM CPU 지원 기준, UE 5.8.2의 `AndroidRuntimeSettings.h` 및 `BaseEngine.ini`에 정의된 ARM64·x86_64 설정. 자세한 경로는 위 패키지 설정 근거와 같다.

### 7.2 Android 텍스처 쿠킹 대상

DEC-017에 따라 초기 Android 빌드는 **ETC2 단일 쿠킹 대상**을 사용한다. DEC-015의 데이터 포함 Development APK와 DEC-016의 ARM64 구성에 적용하며, UAT 명령에 `-cookflavor=ETC2`를 명시한다. 보조 UI에서 패키징할 때도 Android (ETC2)를 선택한다.

이 선택은 패키징의 Android 변형을 고정하는 것이다. `bMultiTargetFormat_*` 키는 Multi 변형의 포함 형식 설정이므로 ETC2 단일 대상 선택을 대신하지 않는다. 현재 UE BaseEngine.ini는 Multi용 ETC2·DXT·ASTC를 모두 활성화하지만, 이번 결정 반영에서 그 기본값이나 프로젝트 Config를 변경하지 않는다.

| 검증 항목 | 완료 기준 | 현재 결과 |
|---|---|---|
| 실행 대상 | 기록된 UAT 명령에 -cookflavor=ETC2, 쿠킹 로그에서 Android_ETC2 대상 확인 | NotRun |
| 산출물 연결 | 해당 Run의 쿠킹·패키징 로그와 최종 데이터 포함 APK의 경로·크기·해시 연결 | NotRun |
| 실제 표시 | 선정 기기에서 배경·유닛·알파 포함 UI/VFX·노멀맵 표시와 텍스처 지원 오류 유무 확인 | NotRun |

단일 쿠킹 대상이 모든 텍스처 자산을 같은 내부 픽셀 형식으로 강제한다는 의미는 아니다. 자산 용도·압축 설정에 따른 UE의 포맷 매핑은 유지하고, 형식이 다른 자산은 해당 설정과 실제 표시를 함께 확인한다. APK 파일명만으로 ETC2 쿠킹 성공을 판정하지 않는다.

ETC2 선택은 그래픽 API·최소 OS·지원 기기 전체의 확정과 별개다. 기기 선정 후 화질·메모리·APK 크기를 측정해 다른 형식과 비교할 필요가 생기면 새 결정으로 기록한다. 근거: [Epic Android 텍스처 형식 안내](https://dev.epicgames.com/documentation/en-us/unreal-engine/android-development-basics-for-unreal-engine), UE 5.8.2 `Engine/Source/Developer/Android/AndroidTargetPlatformSettings/Public/AndroidTargetPlatformSettings.h`의 ETC2 대상·포맷 매핑 및 UAT `ProjectParams.cs`의 cookflavor 처리.

### 7.3 초기 그래픽 API·OS·앱 식별자

DEC-018에 따라 OpenGL ES 3.2, Mobile HDR On, MSAA 2x, 최소 Android 8.0/API 26, Target SDK 36과 아래 개발용 앱 식별자를 묶어서 확정했다. 앞선 Mobile Forward·베이크 조명, ARM64·ETC2·데이터 포함 Development APK 기준과 함께 사용한다.

| AndroidRuntimeSettings 키 | 현재 파일 관찰 | 적용 목표 |
|---|---|---|
| bBuildForES31 | 프로젝트 Config에 명시 없음, 엔진 기본값 true | True: UE 5.8.2 UI의 Support OpenGL ES3.2 |
| bSupportsVulkan | 프로젝트 Config에 명시 없음, 엔진 기본값 true | False |
| bSupportsVulkanSM5 | 프로젝트 Config에 명시 없음, 엔진 기본값 false | False |
| MinSDKVersion | 프로젝트 Config에 명시 없음, 엔진 기본값 26 | 26: Android 8.0 |
| TargetSDKVersion | 프로젝트 Config에 명시 없음, 엔진 기본값 36 | 36 |
| PackageName | 프로젝트 Config에 명시 없음, 엔진 기본값 com.YourCompany.[PROJECT] | com.luckyworkshop.defense.prototype |
| ApplicationDisplayName | 프로젝트 Config에 명시 없음 | 행운공방 디펜스 (개발) |

`bBuildForES31`은 기존 키 이름이며 UE 5.8.2에서 표시하는 기능은 OpenGL ES 3.2다. 초기 빌드는 Vulkan 두 옵션을 끄고 OpenGL ES 경로로 검증한다. 실제 기기의 API·드라이버 지원과 선택된 RHI를 실행 로그로 확인한다. Vulkan 비교가 필요해지면 기기 측정 결과를 근거로 별도 결정한다.

최소 SDK 26은 설치 가능한 OS의 하한이며 해당 OS의 모든 기기에서 성능을 보장한다는 뜻이 아니다. ARM64·GPU/API 지원도 충족해야 한다. Target SDK 36은 앱이 대상으로 삼는 Android 동작 기준이며 최소 OS를 Android API 36으로 올리는 설정이 아니다. 엔진·플러그인이 생성 Manifest 값을 조정할 수 있으므로 최종 APK에서 최소·대상 SDK를 비교하고 불일치를 해결한 뒤 통과 처리한다. 출시 시점의 스토어 정책 적합성은 별도 검토한다.

앱 식별자와 표시 이름은 초기 개발 검증용이다. 출시용 식별자·서명은 별도로 정한다. APK의 패키지명·SDK 값·앱 이름·그래픽 요구 선언은 AAPT2 출력으로 확인하고 설치 후 표시 이름도 관찰한다. HDR·AA 설정과 실제 MSAA 샘플 수는 실행 로그·개발용 렌더링 진단으로 확인하며 Manifest 검사로 대신하지 않는다. 기기 지원이나 프로필에 따라 요청값과 실제 적용값이 다르면 차이를 기록하고 원인을 확인한다.

근거: UE 5.8.2의 `AndroidRuntimeSettings.h`, `BaseEngine.ini`, `RendererSettings.h`; [Epic 모바일 조명 안내](https://dev.epicgames.com/documentation/en-us/unreal-engine/performance-guidelines-for-mobile-devices-in-unreal-engine), [Android SDK 값의 의미](https://developer.android.com/guide/topics/manifest/uses-sdk-element), [AAPT2 검사 안내](https://developer.android.com/tools/aapt2). 설정·APK 검사·기기 실행 결과는 **NotRun**이다.

## 8. 문서 결정 이후 적용·검증할 내용

2026-09-11 사용자 답변에 따라 첫 Android 검증 기기의 모델·OS 버전은 **미정(OPEN-008)**으로 기록한다. 기술 명세의 Galaxy A54급·S23급은 비교 예시이며 실제 보유·선정 기기로 취급하지 않는다. 첫 실기기 검사 전에 모델·OS·GPU·메모리를 확인하고 빌드·실행 기록에 남긴다. 한 기기의 테스트 통과와 제품의 최소 지원 기기 확정도 구분한다.

기기 선정 전에도 도구 설치·PC/Android 패키징 준비를 진행할 수 있다. 실제 기기 설치·터치·SafeArea·성능 검증은 기기가 정해진 뒤 수행하며 현재 NotRun을 유지한다. 초기 ABI·텍스처·최소/대상 SDK·그래픽 API는 DEC-016~018로 확정했다.

| 순서 | 항목 | 남은 내용 |
|---|---|---|
| 1 | 프로젝트 출발점 | DEC-008로 확정, 위 기준 적용 |
| 2 | 도구 버전 | UE 5.8.2 후보·고정 절차는 DEC-009, Windows 권장 조합은 DEC-010, Android 검증 조합은 DEC-011로 확정. Windows 설치·도구 선택·기본 빌드/실행은 RUN-20260913-02/03에서 확인. Android 적용·검증과 엔진 최종 고정은 남음 |
| 3 | 빌드·실행 절차 | DEC-012로 공통 명령·산출물·로그·성공 기준 문서화. 환경 적용 후 실제 실행·증거 기록은 남음 |
| 4 | 모바일 설정·검증 | 렌더링 DEC-013, 화면·입력 DEC-014, 초기 패키지 DEC-015, ARM64 DEC-016, ETC2 DEC-017, 앱 식별자·최소/대상 SDK·그래픽 API·HDR/AA DEC-018 확정. 설정·맵 적용, APK 검사와 실제 기기 검증 대기 |

1번 개발 환경·빌드 재현성에 관한 주요 문서 결정은 정리했다. 실제 도구 설치·정확한 패치 기록·설정 적용·빌드·기기 검증은 개발 작업으로 남긴다. TASK-CORE-01/TASK-MOB-01의 완료나 엔진 최종 고정을 뜻하지 않는다. 다음 문서 논의는 2번 P0 범위와 일정·작업 우선순위의 일치 여부다.
