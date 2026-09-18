# G0 수업 출발점 재현 — 2026-09-18

실행자는 통합 담당자, 기록 검토자는 독립 리뷰 담당자다. 수업의 [공통 절차](../../COMMON.md#g0-replay)를 새 detached worktree 두 곳에서 실행했다. 전체 참고 브랜치를 병합하지 않고 명시된 제공 파일과 수업 소스만 조립했다. 실제 학습자 작성·이수 상태는 Planned다.

| 범위 | A | B |
|---|---|---|
| 프로젝트 | `C:/Users/iam12/P0_lesson_replay_a` | `C:/Users/iam12/P0_lesson_replay_b` |
| 공통 HEAD | `8c6856d235de87cc28c12b49ca775bd0937334a5` | 같은 출발 SHA |
| 완료 소스 | `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6` | `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` |
| 제공 도구·설정·JSON | `81ba665adff6c60ef95f79319b3115050937a1c1` | 같은 제공 SHA |
| 파일 조립 | 21개 blob 일치, Pass | 27개 blob 일치, Pass |
| Editor 컴파일 | Pass/exit0, UBT 64.39초 | Pass/exit0, UBT 73.28초 |
| NullRHI 자동화 | 4Success/0Fail/0NotRun, exit0 | 4Success/0Fail/0NotRun, exit0 |
| 테스트 시간 KST | 13:54:06.713~13:54:26.443 | 13:57:03.002~13:57:22.312 |
| 조립파일 실행 후 재대조 | 21개 모두 원본 blob 유지 | 27개 모두 원본 blob 유지 |

환경은 실제 engine.log/build.log에서 UE 5.8.2-56702186, Windows 11 25H2(10.0.26200.9457), AMD Ryzen 5 7500F, NVIDIA RTX 4060 Ti, MSVC toolchain 14.50.35738/설치14.50.35717, Windows SDK10.0.26100.0을 확인했다. 빌드는 Win64 Development Editor, MaxParallelActions=4다. 위 시간은 컴파일 시간이며 게임 프레임 성능 측정이 아니다. NullRHI는 GPU 화면을 렌더링하지 않는다.

## 작은 증거와 전체 로그 위치

- A: [조립 manifest](a-assembly.json), [Editor 결과](a-editor-result.json), [자동화 결과](a-tests-result.json).
- B: [조립 manifest](b-assembly.json), [Editor 결과](b-editor-result.json), [자동화 결과](b-tests-result.json).
- [테스트별 상태 요약](test-summary.json)은 원본 report/index.json의 fullTestPath/state만 추렸다. 원본 보고서와 engine.log의 Test Completed 행에서도 모든 Success를 확인했다.
- 전체 로그는 각 프로젝트의 `Saved/P0Runs/Replay-G0-{A,B}-editor/build.log`, `Replay-G0-{A,B}-tests/engine.log`, `process.log`, `report/index.json`에 보존했다. 큰 로그와 엔진 출력의 계정/로컬 토큰은 이 폴더에 복제하지 않는다.

assembly.json의 Build/Automation=NotRun은 **조립 도구가 끝난 시점**의 원본 기록이다. 이후 실행 결과는 위 별도 result.json이 원본이다. 두 파일을 함께 읽어야 한다. 실제 HEAD는 출발점이며 조립 소스는 미커밋이므로 HEAD만으로 검사 코드를 식별하지 않는다.

실행 후 두 재현 폴더의 Config/DefaultEngine.ini에 엔진 AndroidFileServer가 로컬 SecurityToken을 생성한 것을 관찰했다. 조립 manifest 밖의 에디터 부수효과이며 기본 맵/GameMode 변경은 없다. 토큰 값은 증거에 포함하지 않았고 기존 파일은 수정/삭제하지 않았다. manifest에 포함된 소스·Config/DefaultGame.ini·JSON·도구는 실행 후에도 원본과 모두 일치한다.

## 수업별 판정과 관찰

| 수업 | 재현하여 확인한 것 | 참고 제작 판정 | 남은 검사 |
|---|---|---|---|
| A-01 데이터 | A-01/02 합친 Editor 빌드; 실제 로더의 정상16종/10웨이브,14개 정책·좌표 mutation, 누락/참조 오류·원자 게시, 같은 snapshot 재호출 | Verified — 데이터 수업의 코드·NullRHI 재현 범위 | 패키지 UFS 실제 로딩·게임 화면은 후속 게이트이며 이 판정에 포함하지 않음 |
| A-02 매치 | 같은 합친 빌드; ParticipantIdentity 값 경계; 통합 canonical의 실제 Mode/Controller 수명 회귀는 별도12개 suite에서 Pass | Draft | A 단독 PIE 두 접속·셋째 접속·실제 복제/화면 |
| B-01 기반 | B-01/02 합친 Editor 빌드; BIndependentLoader1개, W10 일반 생성0/간격0 허용 | Draft | B GameMode 실제 PIE 준비/접속·복제 |
| B-02 명령 | 같은 합친 빌드; AdmissionAndReplay/LimitsAndExpiry/PayloadNormalization3개 | Draft | 실제 소유 RPC 전송·응답 제한/유실·양쪽 복제/UI |

A-01의 실제 오류 관찰은 `GameRules.json.Defeat.ContinuousSeconds`의 지원 범위 거절, `Board.XCentersCm`의 셀 간격 오류, `Board.YCentersByPlayer`의 중앙 경로 분리 오류다. 자동화가 Saved 아래 별도 픽스처를 만들었으며 원본 게임 데이터는 바꾸지 않았다. 실패해야 하는 입력의 거절을 Pass로 기록한 것이며 엔진 실행 실패를 숨긴 것이 아니다.

A 필터4개 중 ParticipantIdentity는 A-02와 공유하는 값 계약이다. B 필터4개 중1개는 기반,3개는 명령이다. 두 수업이 같은 모듈의 헤더/클래스를 참조하므로 역할마다 합쳐 한 번 빌드했고 수업마다 같은 빌드를 중복 실행하지 않았다.

G0 통합 canonical은 `649c1dedd6832c41089a76b59bc76518cd262296`이며 [통합 기록](../../INTEGRATION.md)의 Editor/자동화12종·독립 리뷰 판정과 별개로 이 역할 재현을 보존한다. A `4787bf1a3a0d866aa206d148586594b3c710f957`, B `9129c016efd4a652501f93658fd2831a830c8ab1`에는 canonical의 Source/Config/Content가 동일하게 반영됐다. 이는 기존 독립 완료소스로 수업을 재현한 사실을 대체하지 않는다. `learn/p0-a`, `learn/p0-b`, `learn/p0-integration`은 모두 출발 SHA 그대로 확인했다.
