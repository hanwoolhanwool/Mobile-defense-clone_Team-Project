# 열린 현재 시각과 단일 종료 — P0 / A / G3-A-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-WAVE-01 / [사건 순서](../../../docs/design/BATTLE.md), [공통 종료 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작 / 완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / **미정**. 최종 Source SHA도 미정이며 중간 구현·최초66파일 증거는 [공통 기록](G3_EVIDENCE.md)에 구분 |
| 실제 개발 시작 / 완료 SHA | 자기 G2 통합 결과 / 미생성 |
| 상대 산출물 | G3-A-01 Director, B Processor.AfterExternalCommandClock `8405a93` 및 자기해제 보호 `c697c91` |
| 제공 / 직접 작성 | 제공: G2 매치·실제 서비스. 직접 작성: Core/LDGameMode.*, LDGameState.*와 독립 기대값. 제공 검수 코드: Tests/LDWaveTests.cpp, LDLifecycleTests.cpp, LDPieTests.cpp, G3 probe; 통과 결과를 제품 구현 대신 사용하지 않음 |

## 이번에 만들 동작

30초 동안 두 참가자와 서비스 준비를 기다리고, 준비 완료 후10초 동안 보드 명령을 허용한다. Running에서 동일 시각은 명령→타격→사망/보상→생성/한도→보스 마감→승리→다음 웨이브로 닫는다. D에 맞는 타격은 인정하고 D+.001은 프레임이 늦어도 인정하지 않는다.

공통 설명·수치·검수 종류는 [G3 A 공통 기록](G3_EVIDENCE.md)과 [G2 시계의 근거](G2_03_MATCH.md)에 둔다. 이 수업은 G2의 전투만 앞당기는 hook을 전체 사건 순서로 확장하는 차이를 설명한다.

## 코드 작성 순서

1. GameState의 별도 Phase 복제를 `BattleSnapshot.Phase`로 통합한다. `GetPhase`는 같은 원본을 읽는다. `UpdateBattle`는 서버·MatchId·단조 웨이브·비종료 상태를 검증하고 Revision을 증가시킨다. `FinalizeResult`만 최종 결과/이유/예정 시각을 한 번 게시한다.
2. GameMode.InitGame에서 실제 매치 진입 시각을 기록한다. 개발 빌드의 `?P0Seed=<int>` 여행 옵션→`-P0Seed` 인자→새 MatchId seed 순으로 선택한다. Shipping은 seed 강제 옵션을 쓰지 않는다.
3. InitGameState는 Loading 원본·30초 마감을 만들고 A/B 서비스를 주입한다. BeginPlay는 참가자 수와 별개로20Hz 타이머를 시작한다. RefreshReadiness는 두 참가자·서비스·BeginPlay를 모두 확인하고 준비 마감을 `Now+10`으로 기록한 뒤 Preparing과 명령 접수를 연다. 동일30초의 준비는 먼저 허용하고, 이전 시각 timeout이 확정된 뒤에는 재개하지 않는다.
4. 명시적 G1 probe는 준비 상태에서 전투/명령을 닫는다. G2/G3Load는 두 참가자·서비스·BeginPlay 전까지 Preparing에서 기다린다. 이때 `CanAcceptCommands`는 false이고 타임라인은 준비 마감0을 제품 웨이브 시작으로 해석하지 않는다. `RefreshReadiness`만 준비 완료 즉시 Running으로 전환한다. 웨이브 없는 검수 fixture이며 제품 일반 실행과 섞지 않는다.
5. `AdvanceTimelineBefore(t)`는 `min(다음20Hz 격자, 다음스폰/웨이브/마감)` 중 `<t`인 시각을 순서대로 처리한다. 현재 WorldTime은 명령을 위해 열어 두고, 임의 epsilon으로 미래 타격을 당기지 않는다. 남은 격자 사이 타격은 `AdvanceCombatBefore(t)`로 처리한다.
6. 각 닫힌 시각에서 Combat→보상 Drain→보스 조회 갱신→Director 사건 처리 순서를 지킨다. 명령 hook 안은 Processor 재진입 guard 상태이므로 보상은 큐에 쌓이고 hook 복귀 후 Drain된다. 이 구간에 결과를 바로 게시하거나 Processor.Close를 호출하지 않는다.
7. `RequestTerminal`은 최초 결과 후보를 보관하고 접수와 Combat를 즉시 닫는다. 같은 시각에 이미 예정된 두 번째 공격도 취소하지만 Processor는 아직 닫지 않아 승인된 첫 처치 보상이 남는다. B의 `AfterExternalCommandClock`는 guard 해제→보상 Drain→이 후단 호출→세션 재조회 순으로 실행한다. `FinalizePendingTerminal`은 최종 경제 snapshot→마지막 확정 보스 HP 조회 갱신→GameState Result 순서로 게시하고 나머지 서비스/타이머/구독을 닫는다. 조회 갱신은 새 공격·승리 평가를 하지 않는다. 일반 타이머도 같은 finalizer를 사용한다. 이 임시 후보는 미완료 transaction이며 별도 복제 Result 원본이 아니다.
8. EndPlay는 세션까지 해제한다. Preparing/Running 참가자 이탈은 Aborted다. 종료 후 동일 RequestId 캐시는 원래 결과를 재전달하지만 새 명령·생성·타격은 진행하지 않는다.
9. 독립 기대값을 먼저 기록하고 제공 `LD.P0.G3.Waves.*`의 assertion과 대조한다. 실제 World/Mode/PC/Board/Combat/Enemy를 사용하되 시각·HP를 직접 지정한 부분을 명시적 fixture로 기록한다.

10. 결과 복귀로 새 World가 만들어져도 이전 payload의 세대가 재사용되지 않도록 [G3-A-04](G3_04_SESSION_LIFETIME.md)를 적용한다. 이 수명 차이는 위 명령/전투 시계나 결과 캐시를 새로 소유하는 변경이 아니다.

## Unreal 설정 순서

| 순서 | 위치 | 값·연결 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `/Game/LD/Core/BP_LDGameMode` | native LDGameMode 부모 유지 | 공용 맵/바이너리는 통합 담당자만 저장 |
| 2 | GameMode 서비스 구독 | BeforeExternalCommand→AdvanceBeforeExternalCommand, AfterExternalCommandClock→FinalizePendingTerminal | 보상 큐를 닫기 전에 Drain |
| 3 | Director.OnTerminalRequested | GameMode.RequestTerminal | UI나 WaveDirector가 경제를 직접 닫지 않음 |
| 4 | 실행 | `L_P0?listen?P0Seed=1776`, 상대 `127.0.0.1:7777` | 개발 재현 seed, 양쪽 준비/마감 일치 |
| 5 | 명시 fixture | `-P0Probe=G1`, `G2`, `G3Load`를 해당 검수에서만 사용 |10웨이브 일반 플레이 증거로 쓰지 않음 |

## 실행·실패·수정 기록

아래 실제 결과는 당시 입력에서 관찰한 이력이다. 이번 수업의 최종73파일 입력을 출발점에서 다시 조립한 결과는 아직 미실행이다. 공통 실행 수치·SHA는 [정식 SUMMARY](../../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md), 실패 원본은 [정식 리뷰](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)를 따른다.

| 조건 | 독립 기대 결과 | 실제 결과·범위 |
|---|---|---|
| 두 번째 참가자30초 /30.001초 | 정확30은 Preparing40, 늦으면 LoadingTimeout | NullRHI 자동화 Pass |
| 준비 중 첫 실제구매 |100→80/n1/인구1, 시작해도 동일 actor | NullRHI 자동화 Pass |
| 두 보스 due69.999 /70 /한쪽70.001, deadline70 | Victory /Victory /Defeat, 늦은 타격 보상0 | 실제 서비스 조립 NullRHI 자동화 Pass |
| 타이머를 WorldTime70에 먼저 호출 후 같은시각PC판매 | 판매한 공격 취소→보스 생존패배 | NullRHI 자동화 Pass |
| D+.04 새 구매와 앞선 timeout | clock 후단 결과/보상 게시 뒤 PhaseNotAllowed, 부분소비0 | NullRHI 자동화 Pass |
| 최종 결과 observer | 먼저 최종 골드/별을 관찰, 반복 Result0 | NullRHI 자동화 Pass |
| A01: due11 공격2, HP70 적2; 첫 피해 관찰자에서 Mode.AbortMatch만 호출 | 공격1/첫HP0/다른HP70/양쪽Gold101 | 실제 Fail(공격2·다른HP0)→즉시 Combat.Stop 수정→Pass |
| A02: HP6000 보스2, 물리120·방어20; 첫100피해 직후 Abort | 결과 관찰자가 Actor와 같은5900/6000을 읽음; 다음 공격0 | 실제 Fail(GS6000/Actor5900)→결과 전 조회 갱신→Pass |
| G2/G3Load:0인→1인35초→2인 | 준비 대기/명령 거절→즉시Running; Wave0/적0 | 실제2프로세스 smoke 조기 Abort→수정 후 실제 엔진 타이머 delegate 자동화 Pass |

설계 중 발견한 실패 경로는 `BeforeExternalCommand` 안에서 terminal→Close하면 bProcessing 때문에 Drain이 무효이고 Close가 남은 보상을 지운다는 점이다. B 후단 delegate를 추가하여 해결했으며, 최종 보스 보상이 Result observer에 먼저 보이는 검사를 넣었다. 이는 코드 검토로 찾은 위험이며 실제 실패 로그가 발생한 것으로 쓰지 않는다. 실행 실패·수정은 [공통 증거](G3_EVIDENCE.md)에 이어 기록한다.

준비 대기 회귀의 최초 코드는 private AdvanceLogic 직접 호출로 C2248 컴파일 실패했다. `0d358bc`는 public TimerManager.Tick으로 등록된 실제 delegate를 구동하고 프레임 식별자와 프로브 옵션을 scope 종료 시 복원한다. 이후 자동화·준비 분기 smoke·PIE·패키지의 입력과 수치는 정식 SUMMARY/REVIEW에 둔다.

기존 패키지는 같은 프로세스의 반복 매치·결과 복귀·지연망 회복·원래 응답 재전달을 관찰했지만 모두 자연 패배였다. 정확 D/D+.001, 혼합 승패, 옛 payload의 새 Controller 재전송은 최종 보충 입력의 별도 검사 대상이다. 고정 부하 실측은 정식 PERFORMANCE의 이전 입력 범위로 유지한다.

## 상대에게 전달하고 통합하기

B는 공용 snapshot 조회·두 위젯 수명·복귀를 연결한다. Processor 후단 API와 A terminal 조립을 반드시 함께 통합한다. `bProcessing` 중 외부 완료 callback이 세션을 교체할 수 있으므로 callback 뒤 Session 포인터는 다시 조회한다. 통합 순서/커밋과73파일 적용 절차는 [공통 기록](G3_EVIDENCE.md)을 따른다. 다음 UI 연결은 G3-A-03, 새 매치 세대는 G3-A-04다.

## 이해 확인

- Combat를 현재 시각까지 먼저 실행한 뒤 boss deadline을 검사하면 D+.001이 어떻게 잘못 인정되는가?
- 타이머가 현재 WorldTime을 먼저 닫으면 같은 WorldTime 판매가 뒤늦게 오는 실제 엔진 순서에서 어떤 규칙이 깨지는가?
- 종료 후보만 저장하고 Combat를 멈추지 않을 때 HP와 보상 원본이 왜 어긋나는가? 종료 직전 HP 조회 갱신은 왜 새 전투 진행과 달라야 하는가?
- 작은 변형: 같은 HP1 fixture에서 둘 모두 D를 한쪽만 D+.001로 바꾸고, 최종 보상·일반 수·ResultRevision을 함께 비교한다.

## 단계 완료

- [x] 사건 순서·상태 원본·호출 경계·타이머/구독 정리를 기록했다.
- [x] 당시 Editor/자동화/PIE/패키지 이력과 실패 원인을 정식 증거에 연결했다.
- [ ] 문서 시작점→66파일→73파일 보충의 새 Editor·자동화·실제 PIE·패키지 실행을 완료하고 최종 Source SHA를 기록했다.
- [x] 실제 PC 패키지·PIE·지연망·반복 매치에서 위 관찰 범위를 확인했다. 경계 fixture와 자연 패배 실행은 구분한다.
- [ ] 독립 리뷰 차단0 및 정식 G3 검수 후 다음 게이트에 진입한다. Android는 별도 G4다.
