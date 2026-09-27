# 처음 시작하기 — 초기 설정부터 A안·B안 개발까지

**현재 참고 구현:** 2026-09-18 공통 출발점에서 새로 제작했고 G0~G3 PC 재현 검수를 확인했다. [이번 작업·프로젝트·검증 범위](production/P0_REFERENCE_RUN.md)를 먼저 읽는다. 실제 학습자는 공통 출발점에서 [P0 재기획](design/P0_REPLAN.md)의 G0→G1 순서로 직접 구현한다. 9/16 초기화 이전의 완료 기록은 이번 검증 근거로 쓰지 않는다. 실제 청음·Android G4는 미완료다.

**대상:** 이 프로젝트에 처음 참여하는 개발자 두 명 · **기준일:** 2026-09-28

이 프로젝트는 Unreal Engine 5로 만드는 모바일 우선 3D 협동 디펜스입니다. 먼저 두 사람 모두 자기 PC에서 프로젝트를 실행하고, 이후 **A안은 전투·웨이브**, **B안은 경제·보드**를 따라 진행합니다. 여기서 A안·B안은 [기존에 정한 두 담당 역할](production/TEAM_ROLES.md)을 뜻하며, 두 사람의 결과를 하나의 게임으로 합칩니다.

**역할별 실행 안내:** [개발 계획서](production/DEVELOPMENT_PLAN.md) → [A 개발 문서](production/DEVELOPMENT_A.md) / [B 개발 문서](production/DEVELOPMENT_B.md) → 기존 구현 설계 순서로 읽습니다. 전체 일정과 P0~P2 단계별 작업을 먼저 확인할 수 있습니다.

처음부터 모든 기획서를 읽을 필요는 없습니다. 아래 순서대로 진행하고, 작업에 필요한 세부 문서는 각 단계의 링크에서 확인하세요.

```text
공통 초기 설정
도구 설치 → 프로젝트 받기 → 내 PC에서 빌드·실행 → 역할과 연결 방식 정하기
                                 │
                  ┌──────────────┴──────────────┐
                  │                             │
          A안 · 전투와 웨이브             B안 · 경제와 보드
          매치·데이터·전투                전장·입력·소환·배치
                  │                             │
                  └──────────────┬──────────────┘
                                 │
                첫 소환 → 한 웨이브 전투 → P0 통합 검수
```

**바로 이동:** [공통 초기 설정](#setup) · [A안](#track-a) · [B안](#track-b) · [통합하기](#integration) · [매일 작업하는 방법](#daily) · [막혔을 때](#troubleshooting)

> **현재 출발점**
> 2026-09-13 기록에서는 Windows 기본 빌드와 템플릿 맵 실행을 확인했습니다. 새 PC에서는 다시 확인해야 합니다. Android는 도구 조합 적용·패키징·실기기 검증이 남아 있으며, 현재 기본 맵은 TopDown 템플릿입니다. 디펜스 기능의 진행 상태는 [작업 보드](production/BOARD.md)를 확인하세요.

<a id="setup"></a>

## 1. 공통 초기 설정 — 두 사람 모두 진행

### 1-1. 개발 도구 설치하기

먼저 Windows 개발에 필요한 도구를 준비합니다. 아래 버전은 **저장소에 정한 기준**입니다. 다른 버전을 사용 중이라면 [환경 원본](technical/DEVELOPMENT_SETUP.md)과 맞는지 확인하세요.

| 도구 | 준비할 기준 | 쓰는 곳 |
|---|---|---|
| Unreal Engine | **5.8.2 후보** | 에디터 실행·C++ 빌드·패키징 |
| Visual Studio | **2026 18.10 Stable 계열** | C++ 개발. 설치 관리자에서 C++ 데스크톱 개발·C++ 게임 개발 구성 요소 확인 |
| MSVC | **14.50 계열**, 실제 컴파일러 패치 14.50.35723 이상 | 현재 Config의 디렉터리 버전은 `14.50.35717` |
| Windows SDK | **10.0.26100.0** | Windows 빌드 |
| PowerShell | **7.2 이상**, 실행 명령 `pwsh` | 저장소 빌드 스크립트 실행 |
| Git | 설치 후 저장소 접근 가능 | 프로젝트 받기·작업 브랜치·변경 공유 |
| Node.js / Python | **Node.js 24.15.0 / Python 3** | 문서·데이터 검사 / C++ 포맷 도구 설치 |

UE 빌드는 기본적으로 엔진에 포함된 .NET SDK를 사용합니다. Android 도구는 Windows 첫 실행을 마친 후 [A안의 Android 준비](#android)에서 이어갑니다. B도 추후 실기기 검수에 참여합니다.

**다음으로 넘어갈 기준:** PowerShell에서 `git --version`, `node --version`, `python --version`, `pwsh --version`이 실행되고, UE·VS·MSVC·Windows SDK가 설치되어 있습니다. Python 명령으로 Store만 열리면 실제 Python 설치 경로를 확인하세요.

### 1-2. 프로젝트 받기

팀에서 **함께 출발할 원격 브랜치 이름**을 확인합니다. 진행 중인 문서·코드가 작업 브랜치에만 있을 수 있으므로, 처음 받은 기본 브랜치만 보고 같은 상태라고 판단하지 않습니다. 원격에 없는 필수 파일은 작성자에게 함께 전달받습니다.

아래는 **새 PC에서 처음 받는 경우**의 예시입니다. `C:\Work`는 원하는 작업 위치로 바꿔도 됩니다. PowerShell에서 한 줄씩 실행하세요.

```powershell
New-Item -ItemType Directory -Path 'C:\Work' -Force | Out-Null
Set-Location 'C:\Work'
$StartBranch = Read-Host '팀에서 공유한 원격 시작 브랜치 이름'
git clone --branch $StartBranch https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project.git Mobile_defense_clone
if ($LASTEXITCODE -ne 0) { throw '저장소 접근 권한과 브랜치 이름을 확인하세요.' }
Set-Location '.\Mobile_defense_clone'
git status --short
git rev-parse HEAD
```

이미 프로젝트가 있다면 그 폴더에서 시작합니다. `git status --short`로 진행 중인 변경부터 확인하고, 현재 작업을 보존한 상태에서 팀의 시작 기준을 맞춥니다.

두 사람의 `git rev-parse HEAD` 결과와 필요한 로컬 파일을 대조하세요. 루트에 `Mobile_defense_clone.uproject`, `Source`, `Config`, `Content`, `docs`, `tools`가 보여야 합니다. Git 작성자 이름·이메일도 확인하고, 비어 있으면 아래 명령으로 이 저장소에 설정합니다.

```powershell
git config --get user.name
git config --get user.email
# 위 값이 없거나 이 저장소에서 다른 정보를 쓸 때만 실행합니다.
git config user.name (Read-Host '커밋에 사용할 이름')
git config user.email (Read-Host '커밋에 사용할 이메일')
```

**다음으로 넘어갈 기준:** 같은 출발점을 확보했고, `.uproject`와 소스·콘텐츠를 자기 PC에서 찾을 수 있습니다.

### 1-3. 프로젝트 생성과 첫 C++ 빌드

**이후 명령은 `.uproject`가 있는 프로젝트 루트에서 실행합니다.** 에디터와 Live Coding은 종료한 상태로 시작하세요. Live Coding은 에디터를 켠 채 C++ 변경을 반영하는 기능입니다.

현재 [Config/DefaultEngine.ini](../Config/DefaultEngine.ini)의 Windows 설정은 다음과 같습니다.

```ini
Compiler=VisualStudio2026
CompilerVersion=14.50.35717
WindowsSDKVersion=10.0.26100.0
```

내 PC의 MSVC 디렉터리 버전과 SDK가 이 값과 맞아야 합니다. MSVC 폴더 이름과 실제 `cl.exe` 버전은 다를 수 있습니다. 값이 다르면 [빌드 안내의 3절](technical/BUILD_RUN.md)에 따라 설치 버전과 선택값을 확인하고, 기존 Windows 섹션의 키를 수정합니다. 같은 키를 추가로 붙여 넣지 않습니다.

아래 명령은 **현재 작업 폴더의 프로젝트**를 빌드합니다. 엔진 설치 위치가 다르면 첫 줄만 자신의 경로로 바꾸세요.

```powershell
$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
$ProjectFile = (Resolve-Path '.\Mobile_defense_clone.uproject').Path
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'

& $BuildBat -projectfiles "-project=$ProjectFile" -game -engine -2026
if ($LASTEXITCODE -ne 0) { throw '프로젝트 파일 생성 실패: 위 로그를 확인하세요.' }

& $BuildBat Mobile_defense_cloneEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Editor 빌드 실패: 위 로그를 확인하세요.' }
```

생성된 `.sln`은 C++ 편집에, `.uproject`는 Unreal Editor 실행에 사용합니다. VS에서 빌드할 때는 **Development Editor / Win64** 구성을 선택합니다.

**다음으로 넘어갈 기준:** 프로젝트 파일 생성과 Editor 빌드가 성공하고 `Binaries/Win64/UnrealEditor-Mobile_defense_clone.dll`이 생성됩니다. 자세한 로그 보관 방법은 [빌드 안내](technical/BUILD_RUN.md)를 따릅니다.

### 1-4. 기본 맵을 직접 실행해 보기

같은 PowerShell 창에서 다음 명령으로 에디터를 엽니다.

```powershell
$EditorExe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
& $EditorExe $ProjectFile '/Game/TopDown/Lvl_TopDown' -log
```

1. TopDown 기본 맵이 열리는지 확인합니다.
2. **Play**를 눌러 에디터 안에서 플레이를 시작합니다. 이 실행 방식을 **PIE**라고 부릅니다.
3. 바닥을 클릭해 캐릭터가 이동하는지 확인합니다.
4. **Stop**으로 플레이를 중지한 뒤 에디터를 종료합니다.

이어서 Windows 패키지를 확인합니다. **패키지**는 에디터 없이 실행할 수 있도록 만든 게임 폴더입니다. 다음 예시는 현재 Config의 MSVC 버전을 사용하며, 다른 설치 경로는 `-EngineRoot`로 지정합니다.

```powershell
pwsh -NoProfile -File .\tools\Invoke-PrototypeBuild.ps1 -Stage Windows -CompilerVersion 14.50.35717
```

스크립트 출력에서 `Result: Pass`를 확인한 다음, `Executable`에 표시된 실행 파일을 열어 **맵 표시 → 클릭 이동 → 정상 종료**를 확인합니다. 명령 실행 성공과 실제 플레이 성공은 따로 기록합니다.

> **평소 수정할 폴더를 구분하세요.**
> 이 스크립트는 `Saved/BuildRuns/<실행 시각>/Workspace/`에 복사본을 만들어 빌드합니다. 계속 개발할 때는 처음 받은 프로젝트 루트의 `.uproject`·`.sln`을 여세요. 빌드 결과는 `result.json`, 상세 로그는 해당 실행 폴더의 `Logs/`에 있습니다. 패키지를 전달할 때는 실행 파일이 들어 있는 패키지 폴더 전체를 전달합니다.

**다음으로 넘어갈 기준:** 내 PC에서 PIE와 Windows 패키지의 표시·이동·종료를 모두 확인했습니다.

### 1-5. 제출 전 검사 도구 준비하기

저장소 루트에서 다음을 한 번 실행합니다. C++ 포맷 도구 **clang-format 20.1.8**을 프로젝트 안의 전용 환경에 설치합니다. 별도로 가상 환경을 활성화할 필요는 없습니다.

```powershell
python -m venv Saved/Tooling/code-style
if ($LASTEXITCODE -ne 0) { throw 'Python 가상 환경 생성 실패' }
& .\Saved\Tooling\code-style\Scripts\python.exe -m pip install --disable-pip-version-check --require-hashes --only-binary=:all: --no-deps -r tools/style/requirements.txt
if ($LASTEXITCODE -ne 0) { throw '포맷 도구 설치 실패' }
node tools/check-project.mjs
```

`check-project`는 문서·데이터·C++ 포맷을 검사합니다. Unreal 빌드나 게임 플레이는 앞 단계처럼 직접 확인해야 합니다. 설치 문제가 생기면 [코드 스타일 안내](technical/CODE_STYLE.md)를 확인하세요.

### 1-6. 파일 위치와 두 사람의 연결 방식 익히기

| 찾는 것 | 위치와 사용 방법 |
|---|---|
| C++ 기능 구현 | `Source/Mobile_defense_clone/`. 기존 모듈 안에 필요한 기능 폴더 추가 |
| 새 디펜스 에셋 | `Content/LD/`. 에디터에서는 `/Game/LD/`로 표시 |
| 공유 프로젝트 설정 | `Config/`. 개인 PC 전용 설정과 구분 |
| 지금 할 작업·담당자 | [작업 보드](production/BOARD.md) |
| 기능의 완료 조건 | [백로그와 QA](BACKLOG_QA.md) |
| 규칙·데이터 수정 | 해당 기능 명세와 `tools/build-design-data.mjs`. `data/*.json`은 생성 결과 |

디펜스 전용 폴더·클래스는 필요할 때 추가할 구조입니다. 처음 받았을 때 없을 수 있습니다. 기존 TopDown·Strategy·TwinStick 템플릿은 참고용으로 보존합니다.

기능을 나누기 전에 아래 내용을 짧게 함께 정하고 [설계 메모 양식](TEMPLATES.md#implementation-note)에 남깁니다.

- **담당자와 첫 작업:** 누가 A/B를 맡는지, 가용 시간, 이번에 끝낼 작은 작업 한 개.
- **공통 식별자와 좌표:** 플레이어·유닛·칸을 어떻게 식별하고, 화면 입력을 어느 보드 좌표로 전달하는지.
- **요청과 결과:** 소환·이동 요청에 필요한 값, 성공·실패 응답, 같은 요청이 다시 왔을 때 처리 방식.
- **상태 변경과 연결:** 매치 초기화, 확정된 배치의 전투 반영, 처치 보상을 전달하는 시점과 담당 클래스.

서버가 게임 결과를 확정하고 클라이언트는 입력과 표시를 맡습니다. 네트워크로 함수를 요청하는 방식을 **RPC**, 서버 상태를 클라이언트에 전달하는 것을 **복제**라고 부릅니다. 경제·보드 상태를 함께 바꾸는 처리는 `CommandProcessor`가 조정합니다. 자세한 책임은 [구현 설계 규약](technical/ARCHITECTURE.md#implementation-rules)을 따릅니다.

**공통 준비 완료 체크**

- [ ] 두 사람의 시작 브랜치·커밋과 필수 파일을 확인했다.
- [ ] 내 PC에서 Editor 빌드, PIE, Windows 패키지 실행을 확인했다.
- [ ] `node tools/check-project.mjs` 검사가 통과했다.
- [ ] A/B 실제 담당자·첫 작업·가용 시간을 작업 보드에 기록했다.
- [ ] 공통 ID·좌표·요청/응답·상태 소유와 첫 통합 방식을 정했다.

<a id="track-a"></a>

## 2. A안 — 전투·웨이브 담당으로 시작하기

**첫 목표는 B가 소환한 유닛이 적을 공격하고, 한 웨이브를 끝내는 것입니다.** A는 매치·공통 데이터·전투·웨이브와 전투/결과 UI를 맡으며, Android 빌드·실기기 검수를 주관합니다.

아래 첫 작업을 파악한 뒤 [A 구현 설계](technical/IMPLEMENTATION_A.md)에서 단계별 클래스·Unreal 설정·실패 검수·B에게 넘길 결과를 따라갑니다. 실제 API와 공동 통합 지점은 [공통 구현 계약](technical/IMPLEMENTATION_SHARED.md)에 있습니다.

처음에는 [전투 명세](design/BATTLE.md), [데이터 명세](DATA_SCHEMA.md), [역할별 클래스 책임](production/TEAM_ROLES.md)을 읽습니다. 전투 수치와 판정은 [원작 대조 기록](product/ORIGINAL_REFERENCE.md)의 확인 범위를 함께 봅니다.

| 순서 | 할 일 | 이번 단계에서 확인할 결과 |
|---|---|---|
| A1. 기반 준비 | [TASK-DATA-01](BACKLOG_QA.md#TASK-DATA-01)의 데이터 타입·로딩·검증과 [TASK-NET-01](BACKLOG_QA.md#TASK-NET-01)의 매치 초기화·공용 상태를 작은 작업으로 나눠 구현 | 같은 데이터 기준으로 매치를 시작하고, 두 클라이언트가 공용 상태를 받음 |
| A2. 첫 소환 연결 | B의 Controller·CommandProcessor·경제·보드를 매치 초기화에 연결하고 확정된 유닛을 전투에 반영 | 두 화면에서 같은 소환 결과를 확인 |
| A3. 기본 전투 | B의 맵·경로와 NET/DATA 기반이 준비되면 [TASK-COMBAT-01](BACKLOG_QA.md#TASK-COMBAT-01)의 적 이동·표적 선택·기본 공격·피해·사망 구현 | 유닛이 적을 공격하고 사망과 처치 보상이 한 번만 처리됨 |
| A4. 웨이브와 결과 | [TASK-WAVE-01](BACKLOG_QA.md#TASK-WAVE-01)의 10웨이브·보스·승패와 [TASK-UI-01](BACKLOG_QA.md#TASK-UI-01)의 전투 정보·결과 위젯 연결 | 웨이브가 진행되고 종료 결과가 한 번 확정됨 |
| A5. Android 전투 검수 | [TASK-MOB-01](BACKLOG_QA.md#TASK-MOB-01)의 전투 빌드를 기기에서 실행하고 B와 터치 조작 확인 | P0 전투를 터치로 10웨이브 완주한 기록 확보 |

**첫 기능 PR 예시:** A1 중 “유닛 데이터 한 행을 읽고, 누락된 ID를 오류로 알리는 흐름”부터 제출합니다. 변경한 타입·실제 로딩 경로·정상/실패 확인 결과를 보여 주세요. 이 작은 PR 하나로 TASK-DATA-01 전체를 완료 처리하지는 않습니다.

**B에게 받을 것:** 보드 좌표·적 경로, 확정된 유닛의 생성/제거/배치 결과, HUD에 전투 위젯을 넣을 위치. 처치 보상은 식별 가능한 전투 결과로 전달하고, B의 서버 명령 처리 경로에서 경제 상태에 반영하도록 연결합니다.

<a id="android"></a>

### Android 준비는 기능 개발과 나누어 진행하기

현재 다음 착수 후보는 **Android 도구·설정·기본 패키징 준비**입니다. A1~A4를 모두 끝낼 때까지 기다릴 필요는 없습니다. 기본 패키징을 확인한 뒤, 전투와 UI가 합쳐지면 A5에서 전투 빌드로 다시 검사합니다.

| 순서 | 준비·확인할 내용 |
|---|---|
| 도구 맞추기 | 프로젝트 목표: Android Studio Koala 2024.1.2 Patch 1, SDK API 36, Build Tools 36.0.0, NDK r27c / 27.2.12479018, OpenJDK 21.0.3 |
| 경로와 설정 적용 | [빌드 안내](technical/BUILD_RUN.md) 2절 공통 경로 → 3절 Android 경로·설정 적용. SDK·NDK·JDK의 실제 선택값 확인 |
| 기본 APK 만들기 | 같은 안내의 6절에서 Development·게임 데이터 포함 APK, ARM64·ETC2 대상 패키징 |
| 내용 검사 | ABI·최소 API 26·Target API 36·앱 ID와 쿠킹 로그 확인. 그래픽 기준은 OpenGL ES 3.2·Mobile HDR·MSAA 2x |
| 기기 실행 | 기기를 정한 뒤 모델·OS·GPU·메모리·ABI와 USB 디버깅 연결을 확인하고 설치·표시·터치·SafeArea 검사 |

이 조합은 **프로젝트의 검증 목표이며 Android 성공 기록은 아직 없습니다.** 설치되어 있다는 사실과 UE가 실제로 사용하는 버전을 구분하세요. `Invoke-PrototypeBuild.ps1`은 현재 `Editor`와 `Windows` 단계만 지원하므로 Android는 위 수동 절차를 사용합니다.

기기가 정해지기 전에도 도구·기본 패키징 준비를 진행할 수 있습니다. 전투 맵이 준비되면 패키징 대상 맵을 바꾸고, 최종 완료는 실기기 전투 결과까지 확인합니다.

<a id="track-b"></a>

## 3. B안 — 경제·보드 담당으로 시작하기

**첫 목표는 소환 버튼 한 번으로 재화가 한 번 차감되고, 보드에 유닛이 놓이는 것입니다.** B는 전장·카메라·입력·명령 처리·경제·배치·HUD를 맡으며, PC 패키징 2인 검수를 주관합니다.

아래 첫 작업을 파악한 뒤 [B 구현 설계](technical/IMPLEMENTATION_B.md)에서 단계별 클래스·Unreal 설정·실패 검수·A에게 넘길 결과를 따라갑니다. 실제 API와 공동 통합 지점은 [공통 구현 계약](technical/IMPLEMENTATION_SHARED.md)에 있습니다.

처음에는 [보드·UI 명세](design/BOARD_UI.md), [소환·경제 명세](design/SUMMON_ECONOMY.md), [소환 처리 예시](technical/ARCHITECTURE.md#implementation-rules)를 읽습니다.

| 순서 | 할 일 | 이번 단계에서 확인할 결과 |
|---|---|---|
| B1. 원작 규칙 대조 | [원작 대조 기록](product/ORIGINAL_REFERENCE.md)에 보드·이동·소환·합성·판매의 입력 전후 결과 정리 | 구현할 규칙과 아직 확인이 필요한 항목을 구분 |
| B2. 전장과 입력 | 확인된 규칙으로 [TASK-MAP-01](BACKLOG_QA.md#TASK-MAP-01)의 보드·경로·카메라와 좌표 변환 구현 | 두 플레이어 관점에서 같은 보드 칸을 올바르게 지정 |
| B3. 첫 소환 | [TASK-NET-01](BACKLOG_QA.md#TASK-NET-01)의 Controller·RPC·명령 처리와 [TASK-ECON-01](BACKLOG_QA.md#TASK-ECON-01)의 비용·소환·보장 연결 | 같은 요청을 재전송해도 재화 차감과 배치가 한 번만 발생 |
| B4. 보드 조작 | [TASK-BOARD-01](BACKLOG_QA.md#TASK-BOARD-01)의 이동·스택·합성·판매 구현 | 성공 시 결과가 일치하고, 실패 시 재화·유닛이 손실되지 않음 |
| B5. HUD와 PC 검수 | [TASK-UI-01](BACKLOG_QA.md#TASK-UI-01)의 HUD·짧은 안내에 A의 위젯 연결 후 [TASK-TEST-01](BACKLOG_QA.md#TASK-TEST-01) 수행 | PC 2인·지연 상황에서 표시와 실제 상태가 일치 |

**첫 기능 PR 예시:** B1에서 보드 규칙을 확인한 뒤, B2 중 “테스트 맵에서 입력한 위치를 보드 좌표로 표시”하는 작은 작업부터 제출합니다. 맵·입력 에셋 위치와 두 관점의 확인 결과를 보여 주세요. 소환 구현은 A의 데이터·매치 기반과 요청 계약이 준비된 다음 연결합니다.

**A에게 받을 것:** 데이터 타입·로딩 결과, 매치 초기화와 PlayerState 연결, 전투에서 필요한 유닛 정보, 처치 보상 결과와 전투/결과 위젯.

보드 칸 수는 DEC-039의 **가로 6칸×세로 3칸, 개인 18칸·전체 36칸**을 적용합니다. 원작 확인 전의 소환 비용·환급값을 확정값으로 구현하지 않습니다. 보드와 경제를 둘 다 B가 맡아도 `EconomyService`와 `BoardManager`가 상대의 상태를 직접 고치지 않도록 하고, 함께 바뀌는 처리는 `CommandProcessor`에서 조정합니다.

<a id="integration"></a>

## 4. 두 결과를 합치는 순서

먼저 G0에서 각자 만든 공통 기반과 현재/최대 수·보스 마감 계약을 맞추고, G1에서 두 그룹의 자기 보드 순환·3D 가시성·전 셀 입력을 확인합니다. 그 뒤 두 사람이 각자 큰 기능을 모두 끝내기 전에 아래 세 장면을 함께 실행합니다. 단계별 표는 진행 안내이며 개별 작업의 전체 완료 조건은 [백로그와 QA](BACKLOG_QA.md)를 따릅니다.

| 함께 볼 장면 | A가 준비할 것 | B가 준비할 것 | 통과를 확인하는 방법 |
|---|---|---|---|
| **① 첫 소환** | 매치·데이터·공용 상태·유닛 연결 | 소환 입력·명령·경제·보드 | 두 화면의 유닛·배치·재화 일치. 중복 요청은 한 번만 반영 |
| **② 한 웨이브** | 적·공격·사망·보상 전달 | 이동·합성·판매·보상 처리 | 소환한 유닛이 싸우고 처치 보상은 한 번 지급 |
| **③ P0 전체** | 10웨이브·보스·승패·결과 UI·Android 빌드 | HUD 통합·안내·PC 2인 및 지연 검수 | 아래 P0 완료 기준을 함께 검사 |

**P0는 첫 통합 검증 단계**입니다. 작업 기준은 16종 유닛의 기본 공격·10웨이브이며, 다음 결과가 필요합니다.

- PC 패키징 2인 구성에서 상태 일치, 중복 소비·유닛 손실 0건.
- N ≥ M 도달 즉시 패배·10웨이브 보스 시간초과·최종 종료 경계와 결과 단일 확정. 현재/최대 수·남은 시간의 두 화면 일치. 이전 3초 유예는 폐기합니다.
- Android 실기기에서 터치만으로 10웨이브 완주, 화면·SafeArea·입력과 30FPS 초기 측정.
- 5판 반복의 시드·버전·승패·관찰 기록. 자연 플레이와 경계 재현은 구분.

고유 스킬·등급별 공격 강화·소환 확률 강화·정식 튜토리얼은 P1에서 이어갑니다. 원작 확인에 따라 바뀌는 규칙은 관련 명세·데이터·QA에 먼저 반영합니다.

**Android는 A, PC 2인은 B가 실행과 기록을 주관**하고 두 사람 모두 참여합니다. 발견한 버그는 원인 기능의 담당자가 수정합니다. 전체 QA나 통합 수정을 한 사람에게 몰지 않습니다.

<a id="daily"></a>

## 5. 매일 작업하는 방법

### 시작할 때

1. [작업 보드](production/BOARD.md)에서 맡을 작업 한 개와 선행 조건을 확인합니다.
2. 이번 작업을 반나절~2일 정도로 확인할 수 있는 크기로 나누고, 성공 모습을 한 문장으로 적습니다.
3. 공통 기준에서 작업 브랜치를 만듭니다. 예: A는 `feature/data-unit-loading`, B는 `feature/board-coordinate-input`. 현재 미커밋 변경을 먼저 확인하세요.
4. 같은 `.uasset`·`.umap`이나 공통 C++ 타입을 만질 때는 상대와 편집 순서를 맞춥니다.

### 끝낼 때

```powershell
node tools/check-project.mjs
git diff --check
git status --short
```

검사가 통과하면 변경 종류에 맞춰 C++ 빌드·PIE·패키지 또는 기능 QA를 확인합니다. [PR 양식](../.github/pull_request_template.md)에 **바뀐 동작·실행 방법·검증 결과·남은 문제**를 적고 상대에게 리뷰를 요청합니다. 상대는 설명을 듣고 실패 상황도 재현해 봅니다.

기능 규칙·데이터를 바꿨다면 [관리 방식](WORKFLOW.md)에 따라 원본과 생성 결과를 함께 갱신합니다. 테스트 결과는 [검수 기록](production/TEST_RUNS.md)에, 공유할 증거는 [증거 보관 규칙](production/EVIDENCE.md)에 따라 남깁니다. `Saved/` 안의 로그는 Git에 자동으로 포함되지 않습니다.

작업 상태는 **Backlog → Ready → InProgress → Review → QA → Done** 순서로 관리합니다. 작은 PR 통과와 상위 작업 전체 완료를 구분하고, 전체 완료 조건을 확인한 뒤 Done으로 옮깁니다. 커밋 예시는 `feat(battle): 기본 공격 피해 처리 추가`처럼 [커밋 규약](../COMMIT_CONVENTION.md)을 따릅니다.

<a id="troubleshooting"></a>

## 6. 막혔을 때 먼저 확인할 것

| 증상 | 먼저 해 볼 일 |
|---|---|
| 문서나 도구 파일이 안 보임 | 팀에서 공유한 브랜치·커밋인지, 원격에 없는 필수 파일이 있는지 확인 |
| MSVC 또는 SDK를 못 찾음 | 설치한 디렉터리·실제 컴파일러 버전과 Config·빌드 인자를 [빌드 안내](technical/BUILD_RUN.md) 3절에서 대조 |
| `.uproject`를 열 때 모듈 오류 | 에디터·Live Coding 종료 후 1-3의 Editor 빌드를 실행하고 첫 오류 확인 |
| `pwsh`를 못 찾거나 스크립트 버전 오류 | PowerShell 7.2 이상 설치·PATH 확인. Windows PowerShell 5.1과 구분 |
| 수정했는데 다시 열면 반영되지 않음 | `Saved/BuildRuns/.../Workspace/`가 아닌 실제 작업 루트의 소스·프로젝트인지 확인 |
| C++ 포맷 검사 실패 | [코드 스타일 안내](technical/CODE_STYLE.md)의 `--fix` 방법으로 출력된 파일만 수정 후 재검사 |
| 패키징 Pass인데 실행이 안 됨 | `result.json`의 실제 실행 경로·패키지 폴더 전체·실행 로그 확인 |
| Android 기기가 아직 없음 | 도구·기본 APK 준비부터 진행하고 실기기 결과는 NotRun으로 기록 |
| 한국어 에디터 시작 검사에서 오류 | [기존 진단 기록](production/evidence/RUN-20260913-03.txt)과 비교. 한국어 15건·영어 0건 관찰 이력이 있으며 현재 오류의 원인은 별도로 확인 |

문제를 공유할 때는 **실행한 명령, 브랜치·커밋, 오류가 처음 나온 로그, 기대한 결과와 실제 결과**를 함께 전달하세요.

---

이 문서는 처음 참여한 사람이 따라가는 읽기용 안내입니다. 환경·명령의 원본은 [개발 환경](technical/DEVELOPMENT_SETUP.md)과 [빌드 절차](technical/BUILD_RUN.md), 역할은 [2인 개발 역할](production/TEAM_ROLES.md), 현재 상태는 [작업 보드](production/BOARD.md)에서 관리합니다.
