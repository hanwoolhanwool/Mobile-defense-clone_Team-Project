# G3 독립 리뷰 관찰

상태: **A01/A02·fixture 준비 순서 수정 및56개 회귀 Pass / 부하 v2 내부 관찰 Pass·종합 Fail 보존 / G3 게이트 미완료**. 검토일 2026-09-27. 구현자와 분리한 리뷰다. 1차 고정 소스 `86faa69`(제품 A `61fb3a7`, B `d2183ae8`, 통합 검사기/설정 `86faa69`),2차 `13264a9`,3차 `330283c` 및 A01/A02·준비 순서·검사용 출력 경로 수정의 필요한 diff를 읽었다. 아래 정적 관찰은 빌드·실행 Pass가 아니며, 제공된 자동화/PIE/Editor-game 증거를 범위별로 구분했다. 리뷰어는 커밋·빌드·에디터 실행을 수행하지 않았다.

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
| 수준·상태 | **차단 결함 수정·회귀 Pass**. 정적 반례 전달→실제 Failure→제품 수정 후54개 자동화 Pass. 상세 SHA와 증거는 아래 후속 표 |
| 파일·함수 | `Source/Mobile_defense_clone/Core/LDGameMode.cpp`의 `AbortMatch/RequestTerminal/AdvanceTimelineBefore`; `Source/Mobile_defense_clone/Battle/LDCombatService.cpp`의 `ResolveScheduledAttacks` |
| 재현 | 실제 두 유닛의 공격을 같은 시각11에 예약하고 범위 안 HP70 적2개를 둔다. 첫 `OnDamageCommitted` observer에서 **Mode.AbortMatch만** 호출한다. fixture가 Combat.Stop을 추가로 호출하지 않는다 |
| 수정 전 코드 결과 | Mode가 timeline 진행 중이면 PendingResult와 접수 닫힘만 설정한다. Combat의 공격 반복은 `!bStopped`만 검사하므로 다음 유닛의 공격도 계속하여 두 번째 적 HP를 변경한다. Mode.HandleEnemyDeath는 이미 PendingResult가 있어 두 번째 Death의 보상/Director 카운터 처리를 거절할 수 있다. 따라서 종료 요청 이후의 HP와 공용 N·보상 상태가 갈라진다 |
| 기대 | 첫 이미 확정한 피해/사망/보상은1회 유지하고, 종료 요청 이후 두 번째 공격은 실행하지 않는다. 첫 보상 Drain 후 Result를 게시하며 둘째 적 HP70·일반 잔여1을 보존한다 |
| 수정 방향 | terminal 요청이 들어온 순간 전투 반복의 다음 사건을 막는 명시적 취소/중지 계약을 Mode→Combat에 연결한다. 이미 승인한 Death 큐는 보존하고 Result 전 Drain한다. 같은 원칙으로 WaveDirector/남은 timeline 단계도 정지시키며 외부 observer가 하위 서비스 Stop까지 알아서 호출해야 하는 의존성을 만들지 않는다 |
| 기존 검사 한계 | `CommittedDeathBeforeObserverAbort`는 callback 안에서 AbortMatch에 이어 **Combat.Stop을 직접 호출**하고 유닛도1개이므로 이 API 단독 종료 반례를 검사하지 않는다. 기존54Pass를 이 반례의 통과 증거로 쓰지 않는다. 두 유닛 fixture에서 AbortMatch만 호출하는 회귀가 필요 |

### 전달된 실제 검사 증거

| 층·소스 | 직접 읽은 결과 | 범위·한계 |
|---|---|---|
| Unreal NullRHI 자동화 / `f4ad32c786cb6023014d48e0435d9460e4411dd3` | `Saved/P0Runs/G3-final-automation-v1/result.json`:54Pass, 경고0, 실패0, NotRun0. report에서 G3 wave7개 성공 확인 | 준비 동률·생성 일정·100 latch·승리 진리표·보스 마감/판매·부분 생성·observer+수동Stop 경로. 실제 화면/패키지 또는 위 A01의 API 단독 Abort 증거가 아님 |
| 실제 GPU PIE / `2f144fe4852ede5b96e6f51683e2df3b527ccb37` | `Saved/P0Runs/G3-actual-pie-v2/result.json`:1Pass, PID53912, SettingsRestored=true. `-proof/pie-proof.json`에서 서로 다른 listen/client PIE World·소유0/1·양쪽 소환1/gold80·remote RPC·wave1 count2/2·Aborted·PIE World0과 설정복원 확인 | 실제 에디터 PIE2월드이며 별도2프로세스 패키지는 아님. 통합 담당이 양쪽PNG의 WAVE1/20초/N2를 직접 확인했다. 이 리뷰어는 JSON·코드·범위만 읽었으며 해당 PNG를 재열람했다고 기록하지 않음.10웨이브/승리/패키지 반복·네트워크 지연·부하는 포함되지 않음 |

이330283c 재리뷰는 요청된 V02/metric/time·부분 생성·observer 종료 diff와 위 증거로 한정했다. 새 `G3Load`의 전체 검사기는 이 범위에서 읽거나 승인하지 않았다. **당시 제품 차단 A01을 기록했고 아래 후속 실패→수정 검증으로 연결했다.**

## A01/A02 실패 재현과 최소 수정 검증

### A01 — 종료 요청 뒤 다음 타격

| 단계 | 정확한 소스·실제 결과 | 근거·리뷰 |
|---|---|---|
| 반례 강화 | A 테스트 `83bd8fc1e10b8cdeb9e999561cc119efe2f094c2` | 수동 Combat.Stop을 제거하고 두 유닛 due11·적각HP70·피해 통지1·둘째HP70·양쪽 보상1·잔여N1을 독립 기대값으로 검사 |
| 수정 전 실제 실행 | 통합 `15f14224a6a79159d82f7d45eb2ca83f8625db29`,1Fail | [자동화 결과](abort-regression-before.json), [실패 구간](abort-regression-before-errors.json). `G3-A01-before-test`: DamageCount 실제2/기대1, 둘째 적 HP 실제0/기대70. Unreal 프로세스 exit0이어도 테스트 Fail로 정확히 기록 |
| 최소 제품 수정 | A `2a9d3469143bafa4c3d58dab261f05fbd528edf7` | `ALDGameMode::RequestTerminal`에서 PendingResult·접수 잠금 직후 Combat.Stop. 공격 반복은 다음 사건 전에 중지되며 Processor를 닫지 않아 첫 확정 Death 큐는 Drain→Result까지 유지. observer가 하위 서비스 종료를 직접 호출할 필요 제거 |
| 수정 후 실제 실행 | 통합 `6b59c582fb4d2c75472c28292008e873c2f6d8ec`, 전체 LD.P0 **54Pass/경고0/Fail0/NotRun0** | [수정 후 결과](abort-regression-after-54.json), 전체 로그 `Saved/P0Runs/G3-A01-after-automation`. A01 강화 회귀를 포함한다. 정적 수정과 실제 before/after 결과를 함께 확인하여 A01을 닫음 |

위 실행은 NullRHI의 실제 Unreal 생산 객체/함수 회귀이며 GPU 화면·최종 패키지·모바일 Pass가 아니다.

### A02 — 중단 결과의 보스 HP가 마지막 확정 타격보다 오래됨

| 항목 | 내용 |
|---|---|
| 상태 | **실제 실패→최소 수정→수정 후56개 회귀 Pass**, 아래 e4a02a4 결과로 닫음 |
| 파일·함수 | `Source/Mobile_defense_clone/Core/LDGameMode.cpp`의 `AdvanceTimelineBefore/FinalizePendingTerminal`; `Source/Mobile_defense_clone/Battle/LDWaveDirector.cpp`의 `RefreshCombatView` |
| 재현·원인 | 두 B01의 HP6000/방어20, 두 유닛의 due11, 물리 공격120으로 첫 타격이100 피해를 확정한 직후 damage observer에서 Mode.AbortMatch만 호출. A01 수정은 다음 공격을 멈추지만 Mode의 Pending 분기가 정기 RefreshCombatView를 건너뛰고, 기존 FinalizePendingTerminal도 동기화 없이 Result를 게시함 |
| 영향 | 실제 적 Actor HP는5900인데 결과의 GameState/HUD Boss.HP는6000. R16의 표시·원본 일치 위반이며 서버 GameState와 그 복제 증거만 비교하는 V02 검사로는 검출 불가 |
| 독립 기대·회귀 | A 테스트 `162fd7f8cf328620eb8d9d77f2b06cb627fcd2c1`: 물리120×100/(100+20)=100, 첫 보스5900/다른 보스6000. 첫 유일한 Result observer 안에서 실제 Actor HP와 snapshot HP를 비교. 피해1회·양쪽 재화100 유지·결과Aborted·후속 revision 불변도 검사 |
| 수정 전 실제 실패 | 통합 `530327ae03966afd1a704a5d043d9100b70c87a3`; `G3-A02-before-test`1Fail. [결과](boss-view-regression-before.json), [실패 구간](boss-view-regression-before-errors.json): 기대5900/실제6000, 최초 결과 통지에서 불일치 확인 |
| 최소 수정 | A `decb709977b6f76b8ab1436f2ae88416450dd592`: `FinalizePendingTerminal`에서 승인 Death Drain 후 `WaveDirector->RefreshCombatView()`를 호출하고 그 다음 GameState.FinalizeResult. 마지막 확정 Actor HP를 반영할 뿐 전투 진행·새 타격·승리 평가를 호출하지 않음 |
| 코드 재리뷰 |6줄 변경과 기존 RefreshCombatView를 대조했다. Mode-origin Abort의 Director는 아직 살아 있어 HP를 게시할 수 있고, Result 전이라 GameState.UpdateBattle이 허용된다. 이후 기존 StopMatchServices가 정리한다. 이 최소 수정에서 추가 차단은 발견하지 않았다. 이후 e4a02a4의 실제 회귀 Pass를 확인하여 A02를 닫음 |

### 별도 부하 fixture의 준비 단계 실패

[G3-load-smoke-v1 결과](load-smoke-initial-failure.json)는 소스 `6b59c582fb4d2c75472c28292008e873c2f6d8ec`, 실제 Editor-game 두 프로세스, 부하 유지 요청10초의 **Fail**이다. 대표20분·2000회 검수의 Pass나 성능 수치로 사용하지 않는다. host 로그에는 참가자0만 등록된 뒤 `P0 terminal result=3 reason=5 time=0.000000`이 남았다.

원인은 명시 G3Load/G2 combat-only fixture가 Preparing 상태이지만 준비 마감0·WaveDirector 없음에도 일반 timeline 준비 종료 경로로 진입한 것이다. A 수정 `53333d9e69c3095d54ef4499e40e83e734eae820`는 해당 fixture의 Preparing에서 timeline을 기다리고, 두 참가자 준비 후 RefreshReadiness가 Running을 여는 기존 경로만 사용한다. 준비 중 fixture 명령도 막는다. 정상 제품의 Preparing10초 규칙은 바꾸지 않는다. 이9줄 diff를 확인했으나 준비 회귀의 private 접근 컴파일 수정과 smoke 재실행은 통합 담당 진행 중이며 Pass로 표시하지 않는다.

위 단계 당시 판정은 A01 닫힘/A02와 부하 재검증 대기였으며, 아래 실제 후속 결과로 갱신한다. 전체 로그/실패 입력은 보존했다.

## e4a02a4의56개 회귀와 부하 v2 관찰

### A02와 준비 순서의 실제 후속 결과

[56개 자동화 결과](integration-automation-56.json)의 정확한 소스는 `e4a02a4fc369601ff5434c9101c070998502b373`이다. `Saved/P0Runs/G3-review-final-automation-fix1/result.json`과 report를 직접 읽어 **56Pass/경고0/Fail0/NotRun0**을 확인했다. `CommittedBossHPBeforeObserverAbortResult`, 강화된 `CommittedDeathBeforeObserverAbort`, `CombatFixturesWaitForBothParticipants`가 모두 Success다. 따라서 A02의 기대5900/실제6000 Failure와 수정 후 Pass가 연결되며 A01도 유지된다.

준비 회귀의 컴파일 실패는 private 함수 직접 호출을 제거한 `0d358bc5af920166bc517431848700d8c9c6a98f`로 수정됐다. 테스트는 public TimerManager API로 실제 등록 타이머 delegate를 실행하고, fixture 안에서 GFrameCounter를 저장·복원한다. G2/G3Load 옵션 각각0인/1인에서 Preparing을 유지하고,2인 준비 때만 Running·명령 허용, 자동 웨이브/미작성 적0을 검사한다. 정상 매치의 Loading30초/Preparing10초 검사는 별도 기존 회귀로 유지된다. 테스트 통과는 실제 장기 부하나 네트워크 패키지 통과를 의미하지 않는다.

### G3-load-smoke-v2: 내부 성공과 종합 실패를 분리

[부하 v2 내부 관찰](load-smoke-v2-observations.json)과 `Saved/P0Runs/G3-load-smoke-v2/pair.json`을 대조했다. 소스는 같은 e4a02a4, 실제 별도 UnrealEditor-game host PID54640/client PID43640,540×1170·60FPS 제한·VSync0·RenderOffscreen·무음이며 **10초 요청 smoke**다.

| 층 | 실제 결과·근거 |
|---|---|
| 실행 종합 | **Fail 유지**. launcher가 요구한 host/client 직하 result.json을 찾지 못해 `client exited without valid result`로 종료. 성공한 내부 검사만으로 pair.json을 Pass로 고치지 않음 |
| 원인 | 전환용 초기 World의 subsystem이 요청 경로에 samples.csv를 먼저 만들었다. 실제 게임 World는 기존 파일 보존 분기에 의해 새 GUID 하위 경로를 사용했으므로 launcher 계약과 결과 경로가 달라짐 |
| host 실제 내부 증거 | `host/2326BD984D988DA649AFD8954F83A6F0/result.json`:69개 검사 Pass, 유지12.009초,40유닛·일반99·보스2의 최소 점유 유지,16종 공격 관찰.25배치/2000개 고유 사망 모두 실제 기본 공격 경로, 추가 강제 피해 fallback0 |
| client 실제 내부 증거 | `client/749FBE4D462B185E3B6A878AD12D0282/result.json`:33개 검사 Pass, 유지11.989초,40유닛·일반99·보스2 관찰,적2101개·유닛40개 식별자 관찰 |
| 수명·정리 | host의 배치별 death/reward once·등록/수집 약한 참조0과 종료 logic timer 해제 검사 Pass. 최종 `all-2141-fixture-actors-garbage-collected`/client `client-2141-observed-actors-collected`도 Pass. 양쪽 완료 handshake·CSV 쓰기 완료true |
| 한계 | 제품의 확률·HP·시계 그대로 진행한 자연 플레이가 아니라 명시 부하 fixture다. **최종 패키지·대표20분·Android 성능은 미실행**. 이 약12초 유지와2000회 수명 관찰만으로 장시간 메모리 추세·성능 예산 달성을 선언하지 않음 |

### 검사용 출력 경로·네트워크 누계 수정

`6c89c815cf5d84cd8ac7b479af71a2f66f98de52`는 output 경로 선점을 Initialize에서 제거하고 실제 Sample/프로파일 시작/결과 작성 시 `EnsureOutputDirectory`로 지연한다. 기존 증거 보존 분기는 유지된다. `adb45cde7939d3cbbd853b5f9b3f442a7fb3e52c`는 Sample에 연결 수명의 누적 byte/packet/loss를 기록한다. 초기화·제품 전투/경제·승패 규칙을 변경한 diff가 아니다. 이 두 수정의 코드 범위에서 추가 중대 결함은 발견하지 않았으나 **수정 후 새 run의 종합 Pass는 아직 제시되지 않았다**.

현재 독립 리뷰 판정: **확인한 제품 차단 A01/A02와 준비 순서 결함은 회귀로 닫힘**. 최종 패키지2인·지연/회복·5시드·동일 프로세스 반복·대표20분과 최종 수명/성능 결과 연결이 남아 **G3 게이트는 미완료**다. 이전 smoke의 aggregate Fail은 그대로 보존한다.

통합 실행 후속: `de6e2f62f94660161f5863013ea3353f7a3a1e92`의 새 `G3-load-smoke-v3`는 22:26~22:28 실제 실행에서 **pair Pass**, host69/client33 Pass였다. 요청한 역할 폴더에 결과/CSV/PNG를 저장했고25배치2000 자연 사망·fallback0·최종 GC 검사·완료 handshake가 유지됐다. [새 요약](load-smoke-v3.json). 출력 경로 결함은 이 후속 실행으로 닫으며, 짧은 Editor-game smoke를 최종 패키지20분 성능 증거로 승격하지 않는다.

## PKG01 — 시작 주소 입력창 스타일의 수명 (Closed)

첫 Win64 패키지는 빌드·쿠킹·아카이브를 통과했지만, 실제 client 실행의 프레임2에서 크래시했다. [원본 실행·오류](packaged-entry-crash.json). `UI/LDEntryWidget.cpp::NativeTick`이 지역 `FEditableTextBoxStyle`을 `UEditableTextBox::SetWidgetStyle`에 전달한다. 로컬 UE5.8의 `EditableTextBox.cpp:392`는 자기 프로퍼티에 복사한 뒤 Slate에는 여전히 `&InStyle`을 전달하고, `SEditableTextBox.cpp:113`는 이 포인터를 저장한다. 따라서 tick 반환 후 임시 스타일/FontObject가 무효가 된다. 실제 스택은 `GetInterfaceAddress→FSlateFontInfo::GetCompositeFont→SlatePrepass`였다.

영향: 최종 패키지 시작 화면에서 참가 전 크래시, G3 차단. 수정 방향: 입력창이 UPROPERTY로 소유한 스타일의 안정 주소를 전달하고 글자 크기가 달라질 때만 갱신. Editor에서 드러나지 않은 메모리 수명 문제이므로 단위 회귀만으로 닫지 않고 패키지 시작·반복 복귀를 다시 실행한다. 기존 실패 run은 보존한다.

`501be9035b02e172e356151abe0c1606304b11ce`를 통합한 `0e473f4af380506d209a95f7ec42eccf89c69df4`는 B 역할 Editor72.19초 및 전체57개 무경고 자동화 Pass([Editor](role-b-final-editor.json), [57개](role-b-final-57.json)). 새 `OwnedAddressStyleSurvivesPrepass`는 실제 Slate 입력 자식의 반환 후 prepass·별도 소유 스타일 값 변경·GC·CompositeFont 유효를 확인한다. 독립 읽기 리뷰에서도 안정 주소와 소멸 순서를 확인했고 추가 차단 결함은 없었다. 최종 cooked 재실행 전까지 PKG01은 Open이다.

수정된 실제 패키지 `Replay-G3-package-five-seeds-fix1`에서는 양쪽 시작 버튼·매치 진입·재화 부족 거절·1웨이브·HUD 두 차례 재생성을 크래시 없이 진행했다. 후속 PKG02로 중단했으므로 반복 Entry 복귀 전까지 PKG01을 완전히 닫지는 않는다.

후속 `Replay-G3-package-five-seeds-fix2`에서는 같은 host/client 프로세스로 네 판의 결과 복귀와 다섯 번째 새 매치 진입을 완료했다. 양쪽 `result-return-slate-click`·`entry-slate-button`·서로 다른 `new-match-context`를 확인했고 크래시·검사 실패0이었다. 따라서 수정 후 실제 패키지 Entry↔Match 3회 이상 조건을 충족해 PKG01을 닫는다. 최종 다섯 판·지연/회복·대표 부하는 별도로 종합 판정한다.

## PKG02 — 거절 효과음 쿠킹 누락 (쿠킹/로딩 Closed, 청취 NotRun)

같은 실제 패키지에서 양쪽 `rejection-audio-asset-loaded` 검사가 실패했고 host `LoadPackage` 로그가 `/Game/LD/Audio/S_P0Rejected`의 부재를 확인했다. Editor에서는 실제 에셋을 로드했으나 문자열로 지연 참조한 효과음이 최종 cook에 들어가지 않았다. 영향은 재화 부족 등 거절 시 짧은 음향 피드백 누락이며, 데이터/재화/게임 진행에는 영향이 없었다. 자동 플레이를 계속해 Fail을 희석하지 않고 로그/progress/제어된 종료 근거를 보존했다.

수정 `98727f0`: `Config/DefaultGame.ini`의 기존 ProjectPackagingSettings에 `DirectoriesToAlwaysCook=/Game/LD/Audio`를 추가했다. 해당 폴더의 필수 에셋은 S_P0Rejected 한 개다. 다른 Source의 동적 런타임 경로도 검색했고 기존 메시·재질은 ConstructorHelpers의 하드 참조, 두 맵은 cook 명령에 명시돼 있었다. 새 패키지에서 정상 Controller 거절 경로의 실제 로드 확인이 필요하다. 무음 검수는 들리는 소리의 품질을 검증하지 않는다.

후속 `Replay-G3-package-audio-fixed` compile/cook/archive34.77초 Pass. 새 실제 패키지 `Replay-G3-package-five-seeds-fix2`의 첫 판에서 host/client 모두 `unaffordable-purchase-rejected`와 `rejection-audio-asset-loaded` Pass를 확인했다. 따라서 PKG02의 쿠킹/로딩 결함은 닫혔고 음향 청취는 NotRun이다.5시드 전체와 반복 Entry 복귀는 진행 중이며 이 초기 성공으로 G3를 완료 처리하지 않는다.

## 최종 검사기 리뷰와 측정 한계

실제 RPC 로그 감사는 응답21회만 세던 검사에서 누락을 찾았다. 초반 재전송만 성공하고 Result 뒤 재전송이 없어도 Pass할 수 있었다. `Test-P0G3Evidence.ps1`은 이제 host의 매치/최종 결과 로그 뒤 원래 SERVER 응답과 CLIENT 수신, 보드 revision 변경 뒤 원응답, 요청한 매치 수와 GUID 유일성을 따로 요구한다. client 수신 순서는 같은 PC의 UTC 로그 시각을 사용하며 별도 기기 시계 동기화나 client 자체 Result 표시 시각을 증명하지 않는다. 실제 실행 종료 후 이 강화 검사로 판정한다.

독립 ARCH-01~06 리뷰에서 PC→Processor 권한/캐시 순서, GameState/Director 소유권, Mode 타이머·구독 해제, UI 반환 delegate 해제, GameInstance의 weak ticker 취소에 추가 제품 차단 결함은 발견하지 않았다. 부하 검사의 유료 경제·웨이브·밸런스 우회는 명시 fixture로 제한된다. 직접 피해 fallback도 fixture의 종합 Pass를 만들 수 있으므로 보고서에서 `naturalDeaths=2000`, `fixtureDamageDeaths=0`을 별도로 확인한다.

측정 한계: CSV Combat/Timeline은 프레임 내 합이며 서로 중첩된다. probe의 기존 p95와 분석 도구의 nearest-rank p95는 표본·clock·산식이 달라 섞지 않는다. client samples의 첫 전체 개체 관측과 서버 sustain 시작 순서는 양방향 오차가 있으므로20분 실측 구간은 profile의 Phase/SustainSeconds로 판단한다. 해당 분석 설명을 수정했다. client의 매 배치 GC 후 메모리는 수집하지 않았으며 최종 수거 검사와 구분한다.

비차단 도구 부채: `LDG3LoadProbeSubsystem::Deinitialize`는 구독을 해제하지만 비정상 World 종료 중 자신이 시작한 global CSV 캡처를 종료하지 않는다. 현재 도구는 한 프로세스의 정상 완료에서 EndCapture와 쓰기 완료를 기다린 뒤 프로세스를 종료한다. 캡처 도중 World를 전환해 같은 프로세스로 검사를 재사용하는 경로는 지원·검증하지 않았다. 이 경로를 추가할 때 캡처 소유권에 따른 중단 정리가 필요하다.
