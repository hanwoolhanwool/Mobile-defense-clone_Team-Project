# G1 수업의 참고 코드 조립 재현

| 항목 | 값 |
|---|---|
| 참고 자료 제작 상태 | Draft — 이 절차의 실제 조립·실행 전 |
| 실제 학습자 상태 | Planned — 이 도구 실행으로 변경하지 않음 |
| 시작 SHA | `649c1dedd6832c41089a76b59bc76518cd262296` (canonical G0) |
| 완료 소스 SHA | 실행자가 수정·검증된 G1 통합의 전체 40자리 SHA를 반드시 입력 |
| 조립/Editor/자동화/실제 두 프로세스 | NotRun / NotRun / NotRun / NotRun |
| PIE/PC 패키지/OS 입력/Android 실기기 | NotRun / NotRun / NotRun / NotRun |

[WORKFLOW](../../../WORKFLOW.md)의 참고 제작 재현과 실제 학습 개발 구분을 따른다. [공통 원본](../../COMMON.md), [통합 수업](../../G1_INTEGRATION.md)의 규칙·설정 설명은 반복하지 않는다. 이 문서는 [수업 양식](../../../templates/LESSON.md)의 기준점·작성 순서·실행 증거 부분을 보완하는 검수 절차다.

`Replay-P0G1.ps1`은 G0에서 새 **detached worktree 하나**를 만들고, 수업 순서대로 명시된 파일만 `git restore --source=<SHA> --worktree -- <paths>`로 조립한다. 완성 브랜치를 병합하지 않는다. 완성 파일을 복사하는 목적은 참고 자료의 필수 파일·설정·상대 의존성 누락을 검사하는 것이다. 학습자가 코드를 직접 작성하거나 이해했다는 증거는 아니다. 실제 학습자는 각자의 G0 통과 결과에서 수업을 따라 직접 작성하고 자기 커밋·검수 결과를 남긴다.

## 조립 범위와 순서

| 단계 | 수업/담당 | 파일과 확인 범위 |
|---|---|---|
| 00 제공 | G0 | 6종 JSON, BP_LDGameMode/L_P0/M_P0Flat, DefaultEngine/Game/Input.ini, Build/Test/Sync 도구. G0 SHA에서 복원·blob 확인 |
| 01 직접 작성 참고 | [A 경로 모델](../../A/G1_01_ROUTE_MODEL.md) | `Battle/LDRouteModel.h` → `.cpp` |
| 02 직접 작성 참고 | [A 개체](../../A/G1_02_ENEMY_ACTOR.md) | `Battle/LDEnemyActor.h` → `.cpp`; 최종 통합 소스는 공통 ViewTransform을 사용 |
| 03 직접 작성 참고 | [B 좌표](../../B/G1-01-geometry.md) | `Board/LDBoardGeometry.h` → `.cpp` → `LDViewTransform.h` |
| 04 직접 작성 참고 | [B 뷰·입력](../../B/G1-02-view-input.md) | `Board/LDBoardPresentation.*` → `UI/LDG1BoardWidget.*` → `Core/LDPlayerController.*` |
| 05 직접 작성 참고 | 통합 | `Mobile_defense_clone.Build.cs` → `Core/LDLocalPresentationSubsystem.*` |
| 06 직접 작성 참고 | A/B 검증 | `Tests/LDRouteTests.cpp` → `Tests/LDBoardGeometryTests.cpp`; 전체 파일에는 각 수업과 통합 회귀 기대값이 함께 있음 |
| 07 제공 검사기 | 통합 실행 지원 | `Verification/LDG1ProbeSubsystem.*` → `tools/Run-P0Pair.ps1`; 명시적 Development 실행 fixture |

제공 15개와 G1 최종 소스 21개를 검사한다. 표의 `.*`는 각각 헤더→구현 순서이며 도구에는 개별 경로가 열거돼 있다. 최종 `SourceSha`가 G0 이후 위 목록 밖의 Source/Config/Content/tools 변경을 포함하면 조립 전에 거절한다. 필요한 새 파일이 생겼다면 먼저 수업과 도구의 명시적 목록을 갱신한다.

이것은 **A/B와 통합의 결합 재현 한 번**이다. A 개체의 공통 변환 헤더는 03단계에서, B 컨트롤러의 UI/모듈 의존성은 04~05단계에서 준비되므로 중간 파일 단계마다 빌드하지 않는다. 07단계 이후 Editor 빌드 한 번으로 전체 연결을 검사한다. 자동화 필터의 `LD.P0.G1.Route`는 경로·개체 계산/수명, `LD.P0.G1.Board`는 좌표·레이아웃을 확인한다. 필터별 결과는 해당 범위의 부분 검증이며 독립 역할 브랜치 빌드나 양쪽 게임 화면을 대체하지 않는다. G0 회귀도 필요하므로 실행 예시는 `LD.P0` 전체 필터를 사용한다.

## 실행 절차

기존 `P0_lesson_replay_a`, `P0_lesson_replay_b`와 모든 learn/reference 브랜치는 보존한다. 기존 경로가 있으면 새 이름을 고른다. 스크립트는 기존 폴더를 재사용·삭제·초기화하지 않으며 실패한 새 worktree도 조사용으로 남긴다. 완료 소스는 최초 실패 SHA가 아니라 카메라·표시 시각·검사기 수정과 검증이 포함된 확정 커밋을 지정한다.

```powershell
$ReferenceRoot = 'C:/Users/iam12/P0_reference_integration'
$ReplayRoot = 'C:/Users/iam12/P0_lesson_replay_g1'
$G1SourceSha = Read-Host '검증할 G1 통합의 전체 40자리 커밋 SHA'
$ReplayScript = Join-Path $ReferenceRoot 'learning/tools/Replay-P0G1.ps1'

# 선택: 커밋/경로 목록만 검사하며 아직 worktree를 만들지 않는다.
pwsh -File $ReplayScript -RepositoryRoot $ReferenceRoot -ReplayRoot $ReplayRoot `
    -SourceSha $G1SourceSha -ValidateOnly

# 참고 파일 조립. 새 detached worktree를 생성하며 빌드/에디터는 실행하지 않는다.
pwsh -File $ReplayScript -RepositoryRoot $ReferenceRoot -ReplayRoot $ReplayRoot `
    -SourceSha $G1SourceSha -RunId Replay-G1-assembly
```

`Saved/P0Runs/Replay-G1-assembly/assembly.json`의 `Result=Pass`, 각 파일 `ExpectedBlob=ActualBlob`, `ReplayHead=StartedFrom`을 확인한다. `SourceSha`는 검수 대상 완성 소스이며 HEAD는 계속 G0다. 이후 빌드/자동화 도구가 기록하는 HEAD도 G0이므로 **실행 result.json만으로 완료 소스를 판단하지 않고 조립 manifest를 반드시 함께 연결한다**. 조립 후 임의 소스 변경이 생겼다면 최초 manifest만으로 동일 코드 검수를 주장하지 않는다.

다음은 통합 담당자가 다른 Editor/빌드/포트 작업을 마친 뒤 **직렬로** 실행한다. 이 문서 제작 때 실행된 명령으로 취급하지 않는다. 실패하면 다음 단계로 진행하지 않고 해당 run 폴더의 전체 로그를 보존한다.

```powershell
pwsh -File (Join-Path $ReplayRoot 'tools/Sync-P0Data.ps1') -Check
if ($LASTEXITCODE -ne 0) { throw 'P0 data check failed' }

pwsh -File (Join-Path $ReplayRoot 'tools/Build-P0Editor.ps1') `
    -ProjectRoot $ReplayRoot -RunId Replay-G1-editor
if ($LASTEXITCODE -ne 0) { throw 'Editor build failed' }

pwsh -File (Join-Path $ReplayRoot 'tools/Test-P0Automation.ps1') `
    -ProjectRoot $ReplayRoot -Filter LD.P0 -RunId Replay-G1-tests
if ($LASTEXITCODE -ne 0) { throw 'Unreal automation failed' }

pwsh -File (Join-Path $ReplayRoot 'tools/Run-P0Pair.ps1') `
    -RunId Replay-G1-pair -Port 17977 -RenderOffscreen
if ($LASTEXITCODE -ne 0) { throw 'Two-process G1 execution failed' }
```

여기서는 G0에 저장된 맵·재질을 사용하며 에셋 생성기를 재실행하지 않는다. `L_P0`의 GameMode 설정, native 카메라 속성, SafeZone/Canvas 연결 값과 예상 화면은 통합/B 수업에서 대조한다. 필수 런타임은 Source/Config/Content에 있고 learning에서 데이터를 로드하지 않는다. 재현 worktree의 learning 문서는 G0 버전이므로 수업은 원래 통합 경로에서 읽는다.

`Run-P0Pair`는 UnrealEditor `-game` 두 프로세스의 실제 GPU 렌더링·복제·Engine InputTouch 바인딩 검사다. PIE, PC 패키지, OS 마우스/물리 터치, Android 검수가 아니다. TCP 18077/18078과 UDP 17977도 사용한다. 실행기와 다른 담당자의 포트 사용을 직렬 조율한다.

## 증거 기록 양식

실행 후 이 표의 실제 결과를 채우고 작은 JSON/요약만 이 폴더로 복사한다. 전체 로그·14장의 원본 화면은 해당 `Saved/P0Runs`에 보존하며 필요한 대표 화면·실패 구간만 링크한다. Editor가 로컬 설정값을 자동 저장했다면 변경된 키 이름·실행 영향만 기록하고 민감한 토큰 값을 문서에 복사하지 않는다.

| 검수 | 독립 기대값 | 실제 결과/실행 환경/증거 경로 |
|---|---|---|
| 조립 | 시작 G0, 최종 SourceSha, 36개 blob 일치, detached 유지 | NotRun; `assembly.json`, `status.txt` |
| 데이터 | Schema2/Rules0.3.0와 canonical 6개 JSON 일치 | NotRun; Sync 출력/종료 코드 |
| Editor | UHT·컴파일·링크 성공 | NotRun; `Replay-G1-editor/result.json`, 전체 `build.log` |
| Unreal 자동화 | 실패0·미실행0, G0 및 Route/Board 테스트명·개수 기록 | NotRun; `Replay-G1-tests/result.json`, report/index.json, 실패 시 해당 engine.log 구간 |
| 두 프로세스 | host/client 모두 Pass, 참가자0/1, 같은 ID 1001/1002가 각6160cm 이상 유지 | NotRun; `Replay-G1-pair/pair.json`, 양쪽 result.json |
| 화면·입력 | 7화면비 각각36셀, 자기18칸 선택·상대18칸 거절, 자기 보드 아래·생성 왼쪽·중앙 같은 방향 | NotRun; 양쪽 view-0~6.png와 JSON 검사 항목, 직접 열어 본 화면의 구체 관찰 |
| 경계 회귀 | host 현재시각100.125/논리100.10에서 root15cm·mesh18.75cm, landscape에서도 z0 평면 입력 일치 | NotRun; 테스트명과 실패/수정/재실행 결과 |
| 수명 | 2바퀴 동일 actor/ID, Stop 후 진행·재초기화 거절, 종료 뒤 표시 콜백 거절 | NotRun; 자동화 항목과 두 프로세스 관찰 범위 구분 |
| 성능 | 환경·frame count·p50/p95 기록 | NotRun; 두 구체만의 G1 fixture 수치이며 P0 대표 부하가 아님 |
| PIE/패키지/물리 입력/Android | 별도 실행 증거 필요 | NotRun; 이번 조립/Editor-game 결과로 통과 처리하지 않음 |

새 실패는 `파일·함수 / 입력·재현 / 기대와 실제 / 영향 / 수정 커밋 / 재검사`로 기록한다. 예: 필수 source blob 누락은 조립 실패이고, `.h`/`.cpp`를 중간 단계에서 따로 빌드한 오류는 결합 의존성 문제이며, 투영/입력 불일치는 실제 두 프로세스 검수 실패다. 서로 다른 실패 범위를 같은 Pass로 합치지 않는다.

완료 시 조립 manifest와 Editor·자동화·실제 화면 결과를 해당 수업의 범위와 대조한 뒤 참고 제작 상태만 갱신한다. A의 계산 수업과 표시/입력/통합 수업은 필요한 실행 범위가 다르다. blob 조립 Pass만으로 Verified를 표시하지 않는다. 실제 학습자 상태는 계속 Planned다. G2는 G1 필수 실제 검수가 모두 통과한 뒤 진입한다.

## 재현 확인 질문

- 왜 재현 worktree HEAD가 G0인데도 완료 소스의 빌드 결과를 연결할 수 있는가? 어떤 manifest가 없으면 근거가 끊기는가?
- A 개체 파일을 복원한 직후 빌드하지 않는 이유와 03~05단계의 상대 의존성을 설명할 수 있는가?
- 작은 변형: `-ValidateOnly`에 40자 미만 SHA 또는 기존 G0 replay 경로를 입력하여 파일을 바꾸지 않고 거절되는지 확인한다. 실제 학습 코드에는 이 변형을 반영하지 않는다.

- [ ] 이 절차로 새 G1 detached worktree를 조립했다.
- [ ] Editor/자동화/실제 두 화면 결과를 조립 manifest와 연결했다.
- [ ] 실패 원인·수정·재검증과 아직 실행하지 않은 범위를 기록했다.
- [ ] 기존 G0 재현 worktree와 learn/reference 브랜치가 보존됐다.
