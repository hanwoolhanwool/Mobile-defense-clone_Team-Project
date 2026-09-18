# 공통 계약과 제공 범위

공통 설명의 원본은 [이번 작업 기록](../../docs/production/P0_REFERENCE_RUN.md)과 [서버·클라이언트 계약](../../docs/technical/IMPLEMENTATION_SHARED.md)이다. 수업에는 해당 단계의 구현 순서와 차이만 추가한다.

## 제공 코드와 직접 작성

- 출발점에 제공: 기존 Unreal 모듈/템플릿/사용자 에셋, 현행 `data/` JSON·검증 도구, 규칙/설계/학습 양식.
- A/B가 각각 직접 작성: G0 값 타입, Schema2/Rules0.3.0 로더, 자기 관점의 최소 매치 기반. 첫 통합에서 두 구현을 비교하고 한 상태 원본으로 정리한다.
- 참고 통합에서 제공하는 도구: `tools/Sync-P0Data.ps1`(런타임 데이터 복사/대조), `Build-P0Editor.ps1`(컴파일 로그), `Test-P0Automation.ps1`(계산/계약 검사). 게임 코드는 이 학습 폴더에 의존하지 않는다.
- Stub: G0에서 아직 없는 경제·보드·전투는 성공을 반환하지 않는다. 실제 연결 단계에서 제거하고 재검사한다.

## 실행과 증거의 구분

`node tools/check-project.mjs`는 문서/데이터/서식 검사다. `pwsh -File tools/Build-P0Editor.ps1`은 Unreal 컴파일이다. `pwsh -File tools/Test-P0Automation.ps1`은 실제 UE 코드의 NullRHI 자동화이며 화면/입력 검수가 아니다. PIE, PC 패키지 두 프로세스, Android 실기기는 각각 별도 증거를 남긴다.

전체 로그는 각 worktree의 `Saved/P0Runs/<RunId>/`. 보관할 결과는 정식 `docs/production/evidence/`에 복사한다. 출발점·현재 커밋·미커밋 차이·환경·입력·기대값·관찰·실패와 수정·미검증을 기록한다.

<a id="g0-replay"></a>

## G0 출발점부터 참고 파일 조립하기

이 절차는 **참고 수업에 적힌 입력과 파일 목록으로 빌드 가능한 참고본을 재현하는 검사**다. 학습자의 직접 구현·이해 확인·수업 이수 증거가 아니다. 2026-09-18 A/B 각각 실제 조립·Editor·자동화를 통과했다. A-01 데이터 수업의 참고 제작만 해당 범위에서 Verified이며 PIE/RPC가 남은 A-02·B-01·B-02는 Draft, 실제 학습은 모두 Planned다. `learn/p0-*`에 적용하지 않으며 전체 참고 브랜치 병합도 하지 않는다. [결과·환경·한계](evidence/G0_REPLAY/SUMMARY.md).

| 입력 | 고정 SHA | 필요한 범위 |
|---|---|---|
| 공통 출발점 | `8c6856d235de87cc28c12b49ca775bd0937334a5` | 기본 프로젝트·사용자 에셋·현행 명세·data 원본 |
| 제공 도구·데이터·설정 | `81ba665adff6c60ef95f79319b3115050937a1c1` | 아래 도구3개, DefaultGame.ini, Content JSON6개만 |
| A 완료 소스 | `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6` | A의 Data/Core·Build.cs·LDDataTests만 |
| B 완료 소스 | `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` | B의 Data/Core/Network·Build.cs·LDCommandTests만 |

A/B 소스 커밋만 복원하면 Content 런타임 입력과 실행 도구가 빠진다. 제공 입력은 `tools/Sync-P0Data.ps1`, `tools/Build-P0Editor.ps1`, `tools/Test-P0Automation.ps1`, `Config/DefaultGame.ini`, `Content/LD/Data/{GameRules,DT_Units,DT_EnemyTypes,DT_Waves,DT_SummonProfiles,DT_SpawnProfiles}.json`이다. UFS 설정은 `[/Script/UnrealEd.ProjectPackagingSettings]`의 `+DirectoriesToAlwaysStageAsUFS=(Path="LD/Data")`다. 프로젝트의 기본 맵·GameMode를 바꾸는 DefaultEngine.ini는 이 조립 절차에 포함하지 않는다.

PowerShell에서 기존 에디터·빌드·패키지 작업이 끝난 뒤 아래를 실행한다. 예시 폴더가 이미 있으면 다른 **새 폴더**를 사용한다. 파일을 지우거나 기존 worktree를 초기화하지 않는다.

```powershell
$ReferenceRoot = 'C:/Users/iam12/P0_reference_integration'
$ReplayRoot = 'C:/Users/iam12/P0_lesson_replay_a'
$Role = 'A' # B 재현은 새 P0_lesson_replay_b 폴더와 B를 사용
$BaseSha = '8c6856d235de87cc28c12b49ca775bd0937334a5'
if (Test-Path -LiteralPath $ReplayRoot) { throw '새 재현 폴더를 지정하세요.' }
git -C $ReferenceRoot worktree add --detach $ReplayRoot $BaseSha
if ($LASTEXITCODE -ne 0) { throw 'worktree 생성 실패' }
pwsh -NoProfile -File "$ReferenceRoot/learning/tools/Replay-P0Lesson.ps1" `
    -ProjectRoot $ReplayRoot -Role $Role -RunId "Replay-G0-$Role-assembly"
if ($LASTEXITCODE -ne 0) { throw '참고 파일 조립 실패' }
```

[조립 도구](../tools/Replay-P0Lesson.ps1)는 대상이 출발 SHA의 깨끗한 detached worktree인지 확인한 뒤 **명시된 파일만** `git restore --source=<고정 SHA> --worktree -- <파일 목록>`으로 순서대로 가져온다. 제공 입력 → MatchTypes/Build.cs → Data → B Network → GameState/PlayerState → B Controller → GameMode → 역할 Tests 순서다. 각 파일의 Git blob을 원본과 대조하고 `Saved/P0Runs/<RunId>/assembly.json`에 단계·경로·원본 SHA·blob을 남긴다. HEAD·참고/학습 브랜치·인덱스를 바꾸지 않는다. 중간 실패 후에는 부분 조립 파일을 보존하고 새 재현 폴더에서 다시 시작한다.

```powershell
pwsh -NoProfile -File "$ReplayRoot/tools/Build-P0Editor.ps1" `
    -ProjectRoot $ReplayRoot -RunId "Replay-G0-$Role-editor"
if ($LASTEXITCODE -ne 0) { throw 'Editor 빌드 실패: build.log 확인' }
$Filter = if ($Role -eq 'A') { 'LD.P0.G0.Data' } else { 'LD.P0.G0.Commands' }
pwsh -NoProfile -File "$ReplayRoot/tools/Test-P0Automation.ps1" `
    -ProjectRoot $ReplayRoot -Filter $Filter -RunId "Replay-G0-$Role-tests"
if ($LASTEXITCODE -ne 0) { throw '자동화 실패: result.json 및 engine.log 확인' }
Get-Content -LiteralPath "$ReplayRoot/Saved/P0Runs/Replay-G0-$Role-editor/result.json"
Get-Content -LiteralPath "$ReplayRoot/Saved/P0Runs/Replay-G0-$Role-tests/result.json"
git -C $ReplayRoot rev-parse HEAD
```

도구 기본 엔진은 `C:/Program Files/Epic Games/UE_5.8`이며 다른 설치 위치면 두 명령 모두 `-EngineRoot`로 지정한다. Editor는 exit0, 자동화는 exit0·Succeeded=4·Failed=0·NotRun=0을 모두 요구한다. 전체 로그는 파일로 보존하고 실패 원인 주변만 읽는다. 조립된 파일은 HEAD에 커밋되지 않았으므로 검사 result.json의 HEAD=출발 SHA만으로 소스를 식별하지 않는다. 반드시 assembly.json의 파일별 원본과 함께 인용한다.

G0-01과 G0-02는 같은 모듈로 컴파일한다. A의 로더 테스트가 G0-02의 MatchTypes를 사용하고, B의 GameMode가 G0-02의 Controller/Processor를 참조한다. 따라서 **각 역할의 두 수업 파일을 모두 작성한 뒤 Editor 빌드는 한 번** 수행한다. A 필터4개는 데이터·참가자 값 계약 검사이고 Core 매치 실행 검사가 아니다. B 필터4개 중 BIndependentLoader 1개는 G0-01, AdmissionAndReplay/LimitsAndExpiry/PayloadNormalization 3개는 G0-02의 부분 검사다. 각 수업 이름에 따라 동일 빌드를 반복하거나 부분 Pass를 역할 전체·네트워크 Pass로 올리지 않는다.

수업 재현을 완료한 담당자는 원본 SHA/조립 manifest, 실제 새 로그 경로, 실행 환경, 기대값과 결과, 미검증을 각 수업에 연결한다. 참고 조립·Editor·자동화의 재현을 통과했어도 A-02의 PIE 수명/접속, B-02의 RPC 양방향, 통합 종료/준비 경계 관찰이 없으면 해당 수업 전체를 Verified로 표시하지 않는다. 실제 학습자는 제공 도구/JSON만 받아 수업 순서로 소스를 직접 작성하고 자기 커밋·실행·변형 과제·상대 리뷰를 따로 남긴다.

## 이번 재현 결과

| 역할 | 조립·Editor·자동화 | 증거 |
|---|---|---|
| A | 21개 파일 blob 일치, Editor Pass(64.39초), LD.P0.G0.Data4Success/0Fail/0NotRun | [manifest](evidence/G0_REPLAY/a-assembly.json), [빌드](evidence/G0_REPLAY/a-editor-result.json), [테스트](evidence/G0_REPLAY/a-tests-result.json) |
| B | 27개 파일 blob 일치, Editor Pass(73.28초), LD.P0.G0.Commands4Success/0Fail/0NotRun | [manifest](evidence/G0_REPLAY/b-assembly.json), [빌드](evidence/G0_REPLAY/b-editor-result.json), [테스트](evidence/G0_REPLAY/b-tests-result.json) |

실행 후 조립 대상 blob 불일치도0이다. 엔진이 manifest 밖의 DefaultEngine.ini에 로컬 AndroidFileServer 토큰을 생성한 부수효과와 전체 로그 위치는 [재현 요약](evidence/G0_REPLAY/SUMMARY.md)에 기록했다. 토큰 값은 증거에 복사하지 않았다. G0 canonical 통합·역할 반영 SHA는 [통합 기록](INTEGRATION.md)에 관리하며 세 learn 브랜치는 출발점에 그대로 유지됐다.

## 확인 문제

1. 화면을 뒤집어도 CellId와 RouteIndex를 바꾸면 안 되는 이유는 무엇인가?
2. 로더 검증 통과와 게임 실행 통과는 어떤 증거가 다른가?
3. 명령 중복 캐시는 Phase 검사를 왜 앞서야 하는가?

변형 과제: 학습자 브랜치에서 로더 입력의 RulesVersion만 바꿔 준비가 차단되는지 확인하고, 원본 규칙 파일 변경을 제품에 통합하지 않는다.
