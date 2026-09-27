# G3 독립 리뷰 관찰

상태: **330283c 변경 정적 재리뷰 / 제품 A01 Open / G3 게이트 미완료**. 검토일 2026-09-27. 구현자와 분리한 리뷰다. 1차 고정 소스 `86faa69`(제품 A `61fb3a7`, B `d2183ae8`, 통합 검사기/설정 `86faa69`),2차 `13264a9`,3차 `330283c`의 필요한 diff를 읽었다. 아래 정적 관찰은 빌드·실행 Pass가 아니며, 제공된 자동화/PIE/Editor-game 증거를 범위별로 구분했다. 리뷰어는 커밋·빌드·에디터 실행을 수행하지 않았다.

## 구현 전 계약 대조

[24개 독립 기대값](REVIEW_PLAN.md)은 현행 BATTLE 5.1~5.3, 공통 상태/보상 계약, P0 완료 조건과 대조했다. P0의10웨이브·16종 기본 공격만 대상으로 하며 사냥·던전·스킬·11웨이브 이후 구현을 요구하지 않는다. 40유닛·일반99·보스2는 통합 담당의 P0 대표 부하 계획이며, 기존 P1 스트레스 수치72/128과 구별한다.5판 전승 또는 실측 전 성능 달성을 요구하지 않는다.

계획 초안의 두 부분을 바로잡았다.

1. **Loading 동률 미정 제거:** 실제 listen Loading의 L+30, 두 참가자/필수 서비스 준비 완료 P부터10초, entry UI 대기에는 시계 없음. 통합 담당이 정한 열린 현재 WorldTime 정책을 R01에 반영했다. 정확한 마감 ready는 경계가 열려 있을 때 인정하며 timeout 확정 뒤 재개하지 않는다.
2. **GameState 원본 표기 수정:** 초안의 “GameState/HUD는 조회용 복제본”은 서버 GameState까지 포함하는 잘못된 표현이었다. 서버 GameState는 공용 매치·웨이브·결과의 원본이다. Director의 실행 cursor/등록 ID집합은 일정 진행·중복 제거에 필요한 내부 상태로 구분하고, 같은 N·보스 상태·마감·결과를 독립 수정하는 제2원본을 두지 않는 것으로 ARCH-03을 고쳤다.

## G2에서 G3로 연결할 때의 정적 위험

아래는 G2의 의도된 게이트 경계를 읽고 정한 **후속 구현 검토 지점**이며, 아직 작성되지 않은 G3 코드의 결함 판정이 아니다. 실행 통과 근거로도 사용하지 않는다.

| ID | 현재 파일·함수 | 재현 조건·위험 | G3 수정/검토 방향·기대 검증 |
|---|---|---|---|
| G3-PRE-01 | `Source/Mobile_defense_clone/Core/LDGameMode.cpp` `AdvanceBeforeExternalCommand`, `AdvanceLogic` | 기존 생산 경로는 명령 t 직전에 Combat의<t만 진행한다. G3에서도 이를 유지하면 t 이전 보스 timeout/수량 패배가 확정되지 않은 채 구매가 성공하거나, 마감 뒤 공격이 보스를 처치할 수 있음 | 타임라인은 전투·생성·수량·마감을 예정 시각별로 나눠 닫고, terminal 뒤 명령을 다시 검증한다. R04/R08/R09/R10으로 실제 Mode/PC/Combat/Director 경로 대조 |
| G3-PRE-02 | 같은 파일 `RefreshReadiness`, `InitGameState` | G2는 두 참가자가 준비되면 즉시 Running이며 Preparing10초·Loading30초가 없다. 단순 타이머 추가나 UI 시계로 전이하면 준비 상태를 중복 초기화하거나 정확한 deadline을 먼저 닫을 수 있음 | 서버 상태 전이만 시간 원본으로 사용. 실제매치/entry 구분, 시작점을 한 번 확정, 현재 경계를 열린 상태로 유지. R01/R02와 G1/G2 명시 fixture 분리 확인 |
| G3-PRE-03 | 같은 파일 `StopMatchServices`; `Source/Mobile_defense_clone/Core/LDGameState.cpp` `SetPhase` | 기존 종료는 bEnding을 먼저 latch하고 승인 보상을 Drain한 후 서비스/delegate를 닫는다. Director가 별도로 Result를 게시하거나 callback에서 종료 후 반복을 계속하면 최종 보상 유실·후속 스폰·이중 결과 위험 | Result 값/원인의 단일 원본·전이, 보상 확정 후 게시, callback 직후 terminal 재확인, 완료 요청 캐시 수명 유지. R06/R11/R12/R13 |
| G3-PRE-04 | `Source/Mobile_defense_clone/Core/LDGameState.h/.cpp` 및 새 Director의 후속 diff | GameState에 N/Boss/Deadline을 추가하면서 Director에도 가변 복사본을 둘 경우 중복 Death·생성 실패·보스 actor 도착 순서에서 둘이 갈라질 위험 | 실제 상태 소유 표로 서버 원본·내부 cursor/ID집합·조회 복제본을 구분. 게시 경로를 하나로 만들고 실패·중복·종료 후 불변을 R05/R06/R12/R14/R15에서 검증 |

## 1차 통합 코드 정적 리뷰

제품 변경에서 다음 경로는 기대 계약을 따르는 것을 확인했다. 이는 해당 회귀 실행을 면제하지 않는다.

- `ALDGameMode::AdvanceTimelineBefore`는 현재 시각을 열어두고 논리 스텝/Director의 다음 이벤트 중 빠른 시각까지만 전투→보상→웨이브 사건을 처리한다. 마지막 `<외부 명령 시각` 전투 처리는 앞선 생성·마감 경계를 닫은 뒤 수행한다. `ULDCommandProcessor::SubmitAtTime`은 clock guard를 푼 뒤 승인 보상을 Drain하고 terminal을 확정한 후 접수 가능 상태를 검사한다. R04/R08/R09/R10의 생산 조립 대상이다.
- `ULDWaveDirector::SpawnEnemy`는 각 생성 직후 N을 증가시키며100이면 즉시 terminal을 요청한다. 상대 생성 루프도 bStopped를 재확인한다. `HandleEnemyDeath`는 현재 Match와 실제 등록 Actor의 사망/HP/SpawnSerial/웨이브/타입을 검사한 뒤 등록을 제거하므로 같은 ID의 두 번째 Death는 거절된다. 최종 생성 완료는 두 보스 생성 후 게시하고 승리는 F/B/Z를 함께 검사한다.
- 서버 `ALDGameState::BattleSnapshot`이 공용 상태 원본이고 Director의 cursor/ID집합은 일정 진행·중복 제거용이다. 보스 HP는 EnemyActor 원본을 읽어 게시한다. UI가 HP/카운트/승패를 확정하지 않는다. `ULDGameInstance`에는 여행과 로컬 오류 표시만 있고 매치·경제 원본은 없다.
- terminal 요청을 즉시 외부 게시하지 않고 `FinalizePendingTerminal`에서 보상 Drain→GameState 결과→서비스 종료 순으로 처리한다. PC의 서버 Processor와 확정 캐시는 연결 종료 전까지 유지한다. 결과 UI와 entry는 각 Controller의 소유 위젯이며 EndPlay에서 해제하고 GameInstance의 엔진 실패 구독은 Shutdown에서 제거한다.

## 수정이 필요한 검사기 결함

아래 V01/V02는 **검사기가 불일치를 Pass할 수 있는 정적 반례**다. 제품에서 실제 해당 불일치가 발생했다는 뜻은 아니다. 실행 재현·수정·회귀는 아직 NotRun이다.

| ID / 수준 | 파일·함수 | 재현 입력과 현재 결과 | 영향·수정 방향 | 상태 |
|---|---|---|---|---|
| G3-V01 / 차단 | `Source/Mobile_defense_clone/Verification/LDG3ProbeSubsystem.cpp` `RecordMatch` | 기대 committed ID가{1,2}이고 실제 Actor2개가 모두ID1, 같은 UnitId/소유/칸을 가진 경우. 기존 각 Actor의 Expected 조회와 ActualCount==Expected.Num이 모두 참 | ID2 누락과 ID1 중복을 숨기던 문제. `72235ba`의 SeenIds 중복 거절·기대 개수/고유 개수 비교로 이 반례를 닫음. 첫 정상 실행의 Actor 집합 검사는 양쪽 Pass이며, 중복을 주입한 음성 회귀 실행은 아직 미제공 | 코드 수정 확인; 음성 회귀 NotRun |
| G3-V02 / 차단 | 같은 파일 `RecordMatch` | 기존에는 Bosses·Deadline·FinalSpawnsComplete가 달라도 Pass. `72235ba` 이후 실제 Bosses=[ID361,ID361], 서버=[ID361,ID362]도 Num과 각 FindByPredicate가 모두 성공 가능했음 | `28a5f2e`는 BossIds/BossRoutes 중복을 거절하고 Phase/LoadingDeadline까지 비교한다. 고유 실제 ID·동일 개수·각 ID의 서버 항목 일치로 기존 중복 반례가 거절됨. 역순은 ID 매칭으로 허용하되 값 불일치는 실패 | 코드 수정 확인; 중복/역순 음성 회귀 실행은 별도 |

## 실행 증거가 아직 부족한 항목

현재 검사기가 모든 G3 검수의 완성본이라고 가정하지 않는다. 아래는 추가 실행/기록 계획과 연결할 미완료 항목이며, 새 제품 기능을 요구하지 않는다.

| 항목 | 현재 코드와 제한 | 필요한 증거 |
|---|---|---|
| 시드·밸런스 | `RecordMatch`의 seed는 서버가 실제 채택한 값을 읽지 않고 고정 배열 인덱스를 기입한다. JSON에는 보스 잔여 HP·첫 합성 시각·양쪽 피해 기여가 아직 없다 | 서버 초기화 로그/채택 시드와 각 MatchId를 대조하고5판 전체 기록에 누락 값을 연결. 배열에 적힌 숫자만으로5개 서로 다른 실제 시드 검수 Pass 불가 |
| UI 재생성 | `HUDRecreations`는 위젯 제거 호출 수를 센다. 새 위젯 생성/화면 인스턴스가 하나인지, 재생성 후 버튼당 새 요청1회인지 검사하지 않는다 | 이전/새 인스턴스 수명·현재 위젯 수·재생성 후 명령 수 비교. 제거3회라는 숫자만으로 R21의 중복 구독/타이머 정리 Pass 불가 |
| 네트워크600초 | `MinimumSeconds`는 entry·결과·비연결 대기를 포함한 프로세스 벽시간이다 | 실제 NetDriver 연결/에뮬레이션 적용 구간과 요청·복제 관찰 구간의 합계를 별도 기록. 벽시간600초를 실제 조건 유지600초로 대체하지 않음. 요청 설정 방향당75/150ms는 UE5.8 `NetConnection.cpp`의 outgoing PktLagMin/Max 경로와 맞으며 실제 RTT/손실 표본은 후속 |
| 종료/반복/시간 한계 | suite timeout은 실패를 기록하고 종료하며 결과 개수와 최소 시간을 별도 검사한다. match 진입은 새 MatchId일 때만 `BeginMatch`를 호출하지만 이전 ID 재사용/이전 World 참조 무효 검사는 아직 없음 | 실제 같은 두 PID에서3회 이상 복귀·재진입, 새 MatchId 목록의 중복 없음·이전 World/Actor 참조 정리·terminal 뒤 불변·필수 보스/수량 경계 실행을 별도 연결 |

## 13264a9 재리뷰와 첫 실행의 제한

### 제품 수정 재검토

| 수정 | 파일·함수·재현 | 재리뷰 결론 |
|---|---|---|
| `b88506d` 생성 한도 게시 순서 | `ULDWaveDirector::SpawnEnemy`: N99에서 생성으로100이 되며 GameState observer가 동기적으로 구매를 재진입 | terminal 요청으로 새 명령 접수를 닫은 뒤 N100을 게시한다. 기존 게시→차단 틈을 닫는 순서다. `ImmediateCapAndDeathDeduplication`에 observer의 실제 PC Submit 거절 기대가 있음. 해당 테스트 실행 결과는 별도 전달 전까지 미검증 |
| `baa4be6`/`2fbe86a` 실제 사망 시각 | `ALDEnemyActor::TryApplyDamage`, `ULDWaveDirector::HandleEnemyDeath/EvaluateVictory`: 마지막 사망의 보고 시각을 이전 타임라인 시각으로 낮추거나 아직 닫지 않은 미래 사망을 승리로 게시 | Actor가 확정한 DeathServerSeconds를 저장하고 Death의 생성/사망 시각을 원본과 대조한다. LastDeath가 현재 평가 시각보다 뒤면 승리 보류, 결과 시각은 실제 LastDeath 그대로 사용한다. 수치 기대를 과거 Min으로 맞추지 않는 수정이며 실제 경계 회귀 실행은 별도 |
| `c697c91`/`8f89116` 완료 callback 수명 | `ULDCommandProcessor::SubmitAtTime`: AfterExternalCommandClock→FinalizePendingTerminal→Close가 실행 중 member delegate를 Unbind | 지역 FSimpleDelegate 복사본을 실행하여 Close 이후에도 호출 객체가 반환까지 유지된다. guard 해제→보상 Drain→완료→접수 상태 재검사 순서는 유지 |
| `bc99d37` 부하 fixture 경계 | `ALDGameMode::InitGameState`의 비 Shipping `P0Probe=G3Load` | 기존 G2 fixture 경로를 명시적으로 사용하며 자연 G3 실행의 정상 웨이브를 변경하지 않는다. 부하 fixture 성능을 자연 승패 검증과 혼동하지 않아야 함 |

GameInstance/entry/return/network 코드는1차 이후 제품 diff가 없어 불필요한 재독을 생략했다. GI는 이전 World의 timer 대신 core ticker로 여행하고 엔진 실패 구독을 Shutdown에서 해제한다. 실제 결과→entry→새 매치 반복은 아래 첫1판 실행으로 확인되지 않았다.

### 검사기·PIE 변화

- `0ae913c`의 피해 기여·첫 합성은 서버 Combat/Board 확정 이벤트에서 수집하고, 새 매치 연결 전 기존 delegate를 해제한다. 보스 잔여 HP와 함께 증거 채널로만 복제하며 제품 경제/HP에 쓰지 않는다. 첫 실행에서 각2개 값이 관찰됐지만, probe는 복제된 지표 배열 길이2를 아직 완료 전제 조건으로 검사하지 않는다. 느린/역순 복제에서 빈 배열을 기록해도 완료 처리하지 않도록 확인이 필요하다.
- HUD는 제거 호출 수 대신 다른 새 인스턴스가 viewport에 정확히1개 있을 때 재생성 횟수를 증가시킨다. 첫 실행 양쪽3회 관찰. 버튼당 명령1회와 timer/delegate 수명 전체는 이 카운터만으로 통과가 아니다.
- 네트워크 시간은 Preparing/Running이며 NetDriver 연결이 있는 구간만 합산하도록 바뀌었다. 다만 `Finish`의 `ConnectedGameplaySeconds + 1 >= MinimumSeconds`는 요청보다 최대1초 모자란 값도 Pass할 수 있다.600초 필수 검수는 허용 오차를 완료 근거로 삼지 말고 양쪽 raw 값이 실제600초 이상인지 확인해야 한다. 여행 중 tick 공백이 첫 연결 delta에 포함되지 않는지도 시간 구간 증거로 대조한다.
- `13264a9`의 launcher seed 정규식을 보존된 기존 host 로그에 **읽기 전용으로** 적용한 결과 MatchId `B8FE6DCF439D880A4C28F9B8719498B5`/1776 한 건을 찾았다. 기존 종합 Fail을 Pass로 고치지 않았고 새 runner 실행 Pass도 주장하지 않는다.0.5초 UMG 대기 후 캡처 변경은 코드 확인만 했으며 새 이미지 관찰은 아직 없다.
- `LD.PIE.P0.Session`은 실제 `FStartPIEForAutomationCommand`로 listen/client PIE 두 World를 띄우도록 작성됐다. 각 로컬 보드/첫 구매/원격 RPC·Preparing→Running·명시 Abort·EndPIE·기존 Editor 설정 복원을 검사한다. UE5.8 엔진의 start command 소멸자가 임시 설정의 AddToRoot를 정리하는 것을 원본 코드로 확인했다. 이는 작성된 절차 검토이며 **실제 PIE Pass가 아니다**. terminal 전체 snapshot·10웨이브·반복 매치/오래된 World 참조 무효까지 검사하는 테스트도 아니다.

### 보존된 첫 Editor-game 실행

근거: `Saved/P0Runs/G3-natural-editor-launcher-fix1/{pair.json,host/result.json,client/result.json,host/engine.log}`. 실행 파일은 UnrealEditor이며 **PIE·쿠킹 패키지가 아니다**. run metadata의 Head는 `f0cf89d82e3e66422b2bd771ea8adfc15e30766d`로,13264a9 전체 수정의 실행 증거로 사용하지 않는다.

| 구분 | 실제 기록 |
|---|---|
| 종합 결과 | **Fail**. launcher가 실제 `P0 match` 대신 이전 `G3` 접두사로 seed를 검색하여 ObservedSeed=null. 원본 결과 보존 |
| 내부 관찰 | host16/client14 검사 Pass. 각19개 성공 명령·실패0, client 동일 Pending 재전송17. 이 숫자는 내부 검사 범위만 의미 |
| 자연 제품규칙 결과 | 각1판, 자동 Slate/Controller 조작, Wave10 BossTimeout 패배, 일반N0. B01 두 HP467/3901, 최대각6000, 둘 다 생존 |
| 기록 지표 | 양쪽 피해20467/19325, 첫 합성 서버시각6.542977/34.578892, HUD 새 인스턴스각3회. 연결 플레이 host250.774초/client250.838초 |
| 조건·한계 | 요청 RTT0/손실0, client echo253/253. 시드1개뿐이며5시드·3회 복귀·150/1%600초·300/3%·대표 부하·2000회 수명·실제 PIE·최종 패키지 검수는 이 실행에 포함되지 않음 |

현재 제품 차단 결함: **재리뷰 범위에서 새로 확인한 항목 없음, 필수 실행 판정 전**. 검사기: **V01 코드 수정 확인 / V02 중복 보스 반례 Open**, 지표 배열·정확한 연결 시간·나머지 필수 증거는 미완료다. G3 게이트를 통과로 승격하지 않는다.

## 330283c 재리뷰

### 닫힌 검사기 코드 결함과 확인 범위

- **V02 코드 수정 확인:** `28a5f2e`의 고유 Boss ID/Route 검사, Phase/LoadingDeadline 대조를 확인했다. 고유 실제 ID집합과 동일 개수, 각 ID의 값 일치가 함께 필요하므로 앞선 중복 반례는 더 이상 Pass하지 않는다. 실제 terminal snapshot의 필드를 비교하되 새 실행의 검사 결과로 자동 승격하지 않는다.
- **지표 도착:** `TickLocal`은 FinalBoards/FinalEconomies에 더해 EffectiveDamageByPlayer/FirstMergeAtByPlayer의 길이가 각각2가 되기 전 RecordMatch/ack를 하지 않는다. 지표가 끝까지 오지 않으면 suite timeout으로 Fail하므로 빈 배열을 정상 완료로 기록하던 틈을 닫았다.
- **정확한 최소 연결 시간:** `Finish`의 +1초 허용을 제거했다. host의 완료 허용에는2초 여유를 더하지만 양쪽 최종 검사는 각각 raw connectedGameplaySeconds≥MinimumSeconds를 요구한다.600초 실행 증거가 새로 생겼다는 뜻은 아니다.
- **부분 보스 생성:** `ULDWaveDirector::Initialize`의 단일 ActorFactory seam으로 두 번째 생성 실패를 주입한다. `SpawnEnemy`가 함수 객체를 복사하여 내부 Stop에도 callback 수명을 유지하며, 같은 World/서버권한/아직 ID0인 반환 Actor만 자기 준비물로 정리한다. 다른 World 또는 이미 사용된 Actor를 임의 초기화/삭제하지 않는다. 실패는 Aborted, FinalSpawnsComplete=false, 추가 생성·등록·타이머 정리로 이어진다.
- **확정 사망→관찰 순서:** `ULDCombatService::ResolveScheduledAttacks`는 사망 사실을 먼저 통지한 뒤 복사한 damage observer에 확정 피해를 게시한다. observer가 서비스를 Stop해도 이미 승인한 사망 보상은 사라지지 않는 순서다. 다만 아래 A01처럼 **AbortMatch만 호출하는 종료 API 계약**에는 빈틈이 남았다.

### 추가 제품 차단: G3-A01

| 항목 | 내용 |
|---|---|
| 수준·상태 | **차단 / Open**, 정적 코드 반례. runtime 재현은 아직 미실행이며 root/A에게 전달 |
| 파일·함수 | `Source/Mobile_defense_clone/Core/LDGameMode.cpp`의 `AbortMatch/RequestTerminal/AdvanceTimelineBefore`; `Source/Mobile_defense_clone/Battle/LDCombatService.cpp`의 `ResolveScheduledAttacks` |
| 재현 | 실제 두 유닛의 공격을 같은 시각11에 예약하고 범위 안 HP70 적2개를 둔다. 첫 `OnDamageCommitted` observer에서 **Mode.AbortMatch만** 호출한다. fixture가 Combat.Stop을 추가로 호출하지 않는다 |
| 현재 코드 결과 | Mode가 timeline 진행 중이면 PendingResult와 접수 닫힘만 설정한다. Combat의 공격 반복은 `!bStopped`만 검사하므로 다음 유닛의 공격도 계속하여 두 번째 적 HP를 변경한다. Mode.HandleEnemyDeath는 이미 PendingResult가 있어 두 번째 Death의 보상/Director 카운터 처리를 거절할 수 있다. 따라서 종료 요청 이후의 HP와 공용 N·보상 상태가 갈라진다 |
| 기대 | 첫 이미 확정한 피해/사망/보상은1회 유지하고, 종료 요청 이후 두 번째 공격은 실행하지 않는다. 첫 보상 Drain 후 Result를 게시하며 둘째 적 HP70·일반 잔여1을 보존한다 |
| 수정 방향 | terminal 요청이 들어온 순간 전투 반복의 다음 사건을 막는 명시적 취소/중지 계약을 Mode→Combat에 연결한다. 이미 승인한 Death 큐는 보존하고 Result 전 Drain한다. 같은 원칙으로 WaveDirector/남은 timeline 단계도 정지시키며 외부 observer가 하위 서비스 Stop까지 알아서 호출해야 하는 의존성을 만들지 않는다 |
| 기존 검사 한계 | `CommittedDeathBeforeObserverAbort`는 callback 안에서 AbortMatch에 이어 **Combat.Stop을 직접 호출**하고 유닛도1개이므로 이 API 단독 종료 반례를 검사하지 않는다. 기존54Pass를 이 반례의 통과 증거로 쓰지 않는다. 두 유닛 fixture에서 AbortMatch만 호출하는 회귀가 필요 |

### 전달된 실제 검사 증거

| 층·소스 | 직접 읽은 결과 | 범위·한계 |
|---|---|---|
| Unreal NullRHI 자동화 / `f4ad32c786cb6023014d48e0435d9460e4411dd3` | `Saved/P0Runs/G3-final-automation-v1/result.json`:54Pass, 경고0, 실패0, NotRun0. report에서 G3 wave7개 성공 확인 | 준비 동률·생성 일정·100 latch·승리 진리표·보스 마감/판매·부분 생성·observer+수동Stop 경로. 실제 화면/패키지 또는 위 A01의 API 단독 Abort 증거가 아님 |
| 실제 GPU PIE / `2f144fe4852ede5b96e6f51683e2df3b527ccb37` | `Saved/P0Runs/G3-actual-pie-v2/result.json`:1Pass, PID53912, SettingsRestored=true. `-proof/pie-proof.json`에서 서로 다른 listen/client PIE World·소유0/1·양쪽 소환1/gold80·remote RPC·wave1 count2/2·Aborted·PIE World0과 설정복원 확인 | 실제 에디터 PIE2월드이며 별도2프로세스 패키지는 아님. 통합 담당이 양쪽PNG의 WAVE1/20초/N2를 직접 확인했다. 이 리뷰어는 JSON·코드·범위만 읽었으며 해당 PNG를 재열람했다고 기록하지 않음.10웨이브/승리/패키지 반복·네트워크 지연·부하는 포함되지 않음 |

이 재리뷰는 요청된 V02/metric/time·부분 생성·observer 종료 diff와 위 증거로 한정했다. 새 `G3Load`의 전체 검사기와 부하 실행 결과는 이번 범위에서 읽거나 승인하지 않았다. **현재 제품 차단 A01이 남아 G3 통과를 보류한다.**
