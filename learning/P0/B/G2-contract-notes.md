# 공동 확정과 전투 연결 준비 메모 — P0 / B / G2 준비

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-BOARD-01·TASK-ECON-01·TASK-NET-01, [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md#contracts), [B-03/04](../../../docs/technical/IMPLEMENTATION_B.md#b03) |
| 참고 자료 제작 상태 | Planned — G1 실제 재검증 중의 API 합의 메모, 구현 수업 아님 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | 메모 기준 `cf3f61072ebb3fc5502c051cca62d1c5e23a9c04` / G2 구현 미생성 |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | G1 통합 Pass, A의 UnitActor 준비/배치 적용·CombatService 등록/처치 계약 |
| 제공 코드 / 직접 작성할 코드 | 제공: G0 명령/매치 기반·G1 좌표. 향후 작성: BoardManager·EconomyService 공동 확정, A 연결부와 검사. 현재 소스·에셋·UI 변경 없음 |

공통 설명은 [COMMON](../COMMON.md), 준비→공동 확정→게시·매치 종료는 [공통 수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md#lifecycle)을 따른다. 이 메모에는 이번 역할 연결의 차이와 독립 기대값만 둔다. 초기 B 설계의 미정 배치 문구보다 DEC-044/045와 현행 [뭉치 규칙](../../../docs/design/BOARD_UI.md#unit-stacks), [경제 6~7절](../../../docs/design/SUMMON_ECONOMY.md)이 우선한다. 데이터는 런타임 `Content/LD/Data/GameRules.json`, `DT_Units.json`, `DT_SummonProfiles.json`의 Schema2/Rules0.3.0을 읽는다.

## 이번에 만들 동작

서버가 첫 소환의 재화·유료 소환 횟수·RNG·배치와 개체를 함께 확정한 뒤 A의 기본 공격에 등록한다. 뭉치 이동·판매 보충은 기존 InstanceId와 공격 타이머를 보존한다. 합성은 선택한 한 칸의 정확히 세 ID를 제거하고 새 ID를 하나 생성한다. 처치는 양쪽 경제에 정확히 한 번 반영한다. 인구20과 칸당3, 종류별 여유 뭉치 최대1을 각각 지킨다. P1의 마나·스킬·룰렛·던전·강화는 구현하지 않는다.

## 코드 작성 순서

다음은 G1 통과 후의 예정 순서다. 현재 이 이름의 서비스/API가 구현됐다는 뜻이 아니다.

1. `Board/LDBoardTypes.h`: `FLDPlacedUnit{InstanceId, UnitId, PlayerIndex, CellId, MoveBlockedUntilServerSeconds}` 값 계약. `FLDBoardCommit{MatchId, PlayerIndex, BoardRevision, CommitServerSeconds, ChangeReason, AddedOrUpdatedUnits, RemovedInstanceIds}`로 게시한다. ChangeReason은 Summon/Move/Merge/Sell이며 복제된 값으로 클라이언트가 서버 원본을 다시 만들지 않는다.
2. `Board/LDBoardManager.*`: 존재·소유·셀·인구·잠금·BoardRevision의 서버 원본. 셀 변환/확정 Actor 조회와 const 값 스냅샷을 제공한다. 인구는 실제 개체 수에서 계산하며 HUD가 별도 원본 카운터를 만들지 않는다.
3. `Economy/LDEconomyService.*`: 참가자별 Gold/Stars/PaidSummonCount/RNG/EconomyRevision 원본. 추첨은 복사한 RNG에서만 수행한다. 같은 등급의 UnitId 후보는 일정한 순서로 고정한 뒤 균등 추첨해 TMap 순회 순서에 결과를 맡기지 않는다.
4. `Network/LDCommandProcessor.*`: 준비 계획 두 개를 소유하고 참가자 요청·내부 보상을 직렬 처리한다. 서버 신원·중복 캐시·종료 후 원응답 재전달은 G0 계약을 유지한다.
5. 준비 단계에서 A의 비활성 UnitActor를 만들고 모든 필수 에셋/초기화를 완료한다. 준비 Actor는 전투·충돌·복제·성공 이벤트에 참여하지 않는다. ID도 임시 계획에 예약하고 성공 전 원본 발급 상태를 확정하지 않는다.
6. `ValidatePrepared`로 양측 Revision·대상 ID·잠금·수용량을 재검증한다. 공동 확정 내부는 이미 검증된 값 교체이며 실패할 외부 작업·delegate·Blueprint·latent를 넣지 않는다. 양측 반영 이후 Actor에 배치 적용/활성화 → 보드 통지 → 제거 Actor 정리 순으로 게시한다. 통지 연결부는 제거 ID의 전투 등록을 먼저 해제한다.
7. `Core/LDGameMode.*` 연결부가 B 보드 통지를 받아 A 전투 등록/제거를 연결하고 A 처치 통지를 B 내부 큐에 넣는다. CombatService가 BoardManager/EconomyService를 탐색하거나 직접 수정하지 않는다.

### A와 B의 최소 공개 계약

| 제공자 | API·값 | 의미 |
|---|---|---|
| B BoardManager | `TryGetCellTransform(PlayerIndex, CellId, OutTransform)` | 소유권을 검사한 canonical 좌표. 실패를 원점으로 대체하지 않음 |
| B BoardManager | `TryGetCommittedUnitActor(InstanceId, OutActor)`, `OnBoardCommitted(Commit)` | 준비 Actor는 조회 불가, 통지는 두 원본 확정 후 |
| A UnitActor | `InitializePrepared(Unit, Row, Transform)` | 비활성 준비; 실패하면 B가 취소·정리 |
| A UnitActor | `ApplyCommittedPlacement(Unit, Transform)` | B 배치/잠금의 파생 복사본; 기존 공격 타이머 초기화 없음 |
| A CombatService | `RegisterCommittedUnit(Unit, CommitServerSeconds)`, `UnregisterUnit(InstanceId)` | 첫 등록만 초기 공격 예정 시각=확정+.25; 동일 ID 재등록/제거는 멱등 |
| A 처치 이벤트 | `FLDCombatDeath` + `OnEnemyDeathCommitted` | 서버 MatchId/DeathEventId/EnemyId/SpawnSerial/EnemyTypeId/SpawnWaveIndex/SpawnedServerSeconds/DeathServerSeconds |
| B Processor | `EnqueueCombatReward(Death)` | 서버 내부 큐; 사용자 RPC/RequestId 소비 없음 |
| B EconomyService | 내부 `ApplyCombatReward(Death)` | Processor의 정해진 drain 단계에서만 호출. 서버 규칙으로 수혜자·금액 결정 |

A/B가 위 배치·사망 필드와 연결 순서에 합의했다. BoardRevision은 기존 명령/응답의 int32와 맞추고, InstanceId/DeathEventId/EnemyId/SpawnSerial은 uint64다. 사망 값에는 클라이언트 금액·수혜자 필드를 넣지 않는다. B의 이동 잠금 원본은 `MoveBlockedUntilServerSeconds`, A의 공격 스케줄 원본은 `NextAttackAt`으로 분리한다. 수동 Move는 이동·교환에 참여한 모든 ID에 `max(기존 잠금, 확정 시각+.30)`을 적용한다. A의 실제 공격 가능 시각은 `max(NextAttackAt, 이동 잠금)`이며 기존 NextAttackAt 자체를 초기화하지 않는다. 판매 보충은 기존 ID·잠금·공격 타이머를 유지하고 새 개체 초기 지연+.25를 넣지 않는다. .15초 모델 보간은 논리 셀 이동과 분리한다.

A의 P0 기본 피해는 서버 예정 공격 시각에 확정하고 투사체는 그 결과를 표현한다. 현행 P0 데이터에 별도 게임 판정용 비행시간이 없으므로 표현 완료 콜백에서 다시 피해를 주지 않는다. 판매·합성으로 제거된 ID의 아직 확정하지 않은 공격은 피해 직전 존재/등록 검증으로 거절한다. 이미 확정된 공격 결과를 판매가 되돌리지 않는다.

### 공동 확정과 불변식

- 소환: 추첨 전에 인구와 모든 가능한 결과의 수용 가능성을 검사한다. 결과 종류의 여유 뭉치→`PlacementOrderByPlayer` 첫 빈칸. 성공만 n+1; n=현재 성공한 유료 소환 횟수, 가격20+2n.
- 이동: 뭉치 전체를 빈칸으로 이동하거나 상대 칸의 뭉치 전체와 교환한다. 같은 종류도 교환이며 수동 분할·흡수는 없다. 존재·인구·UnitId·타이머를 재생성하지 않는다.
- 판매: 정확한 한 ID 제거. 가득 찬 뭉치에서 판매해 다른 여유 뭉치가 있다면 그 여유 뭉치의 최소 InstanceId를 가져와 보충한다. 인구−1, 새 소환 이벤트 없음.
- 합성: 선택 ID가 속한 같은 칸의 서로 다른 동일 UnitId 세 개만 소비. 결과는 상위 등급4종 균등, 여유 뭉치→소비 후 첫 빈칸. 선택 칸 우선권 없음. 전설 합성 거절, 인구−2, PaidSummonCount 불변.
- 실패/취소/최종 재검증 실패: 재화·PaidSummonCount·RNG·두 Revision·보드·전투 등록·성공 이벤트 불변. 명령 실패 결과 캐시/요청 수락 이력은 G0 규칙에 따라 바뀔 수 있으며 게임 상태 불변식과 구분한다.
- 이동은 BoardRevision만 변경한다. 소환/합성/판매는 보드와 경제 변경을 함께 게시하고 합성의 RNG 변경도 EconomyRevision에 포함한다. 중복 요청은 원래 결과만 재전달한다.
- 처치 보상은 `(MatchId, DeathEventId, Recipient)`로 중복 방지한다. 진행 중 트랜잭션에 delegate로 재진입하지 않으며 전투 스텝 말의 정해진 drain에서 다음 외부 명령/최종 결과 전에 확정한다. 종료 후 새 처치 통지는 무효다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 준비 메모 | 변경 없음 | 기존 G1 맵/재료/카메라 유지 | G1 실제 검수와 에디터·바이너리 작업 충돌 방지 |
| G2 착수 후 | A UnitActor 준비 경로 | 준비 중 Hidden/NoCollision/전투 미등록, 확정 후 활성 | 원본 확정 전에 화면에 유령 유닛이나 공격이 나타나지 않음 |
| G2 착수 후 | GameMode 연결부 | 보드 확정→전투 등록, 처치 확정→내부 보상 큐 | 서비스 간 직접 수정 대신 매치 수명에서 구독·해제 |

예상 화면은 첫 유닛이 자기 화면 순서 첫 칸에 나타나고 골드가100→80, 다음 소환 가격22로 바뀌는 것이다. 같은 종류 네 번째 개체는 기존3+새1 뭉치로 표시한다. 실제 화면·설정값·위젯 연결은 G2 구현과 실행 후 작은 수업에 별도로 남긴다.

## 실행·실패·수정 기록

기대값은 명세와 수작업 계산으로 먼저 정했다. 현재 모든 G2 검사는 NotRun이며 이 표는 실제 통과 기록이 아니다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 시작100, 성공 소환4회 | 비용20+22+24+26=92, 골드8, n4, 다음28 | NotRun | 경제/공동 확정 예정 |
| 이후 일반1마리 판매 | 환급14, 골드22, n4, 다음28, 인구−1 | NotRun | 서버 확정 순서 예정 |
| 같은 종류 연속1~7개 | [1]→[2]→[3]→[3,1]→[3,2]→[3,3]→[3,3,1] | NotRun | 배치 순서/인구/여유 뭉치 예정 |
| [3,2]의 가득 찬 뭉치에서 판매 | [3,1], 기존 여유 뭉치 최소 ID 이동, 총 인구5→4 | NotRun | ID·타이머·새 등록 이벤트 대조 예정 |
| 기존 NextAttackAt=12, 수동 이동 t10 | ID 동일, NextAttackAt12 유지, 잠금10.3 | NotRun | 전투/B 배치 결합 예정 |
| 기존 NextAttackAt=10.1, 수동 이동 t10 | 공격10.3부터 가능, 초기+.25로 덮지 않음 | NotRun | 이동 경계 예정 |
| 판매 보충 t10, NextAttackAt10.1 | 같은 ID/NextAttackAt10.1 보존, 신규 지연 없음 | NotRun | 자동 이동과 신규 생성 구분 예정 |
| 인구20·잔액부족·Actor 준비 실패·최종 재검증 실패 | 모든 게임 원본/RNG/Revision 불변, 임시 Actor 정리 | NotRun | 각 실패를 분리 주입 예정 |
| 같은 요청 2회·같은 키 다른 내용 | 성공/비용/추첨1회, 원응답 재전달 또는 Conflict | NotRun | 실제 Processor 경로 예정 |
| 일반 적1마리 사망·같은 이벤트 재전달 | 두 참가자 각+1골드, 재전달+0 | NotRun | A 사망→B 큐→경제 예정 |
| 보스 생성부터 정확히30초 사망 | 두 참가자 각+100골드/+3행운석 | NotRun | fast <=30, 30초 초과는+2행운석 예정 |
| 빠른 보스2마리 사망 | 두 참가자 각+200골드/+6행운석, 최종 결과 전 반영 | NotRun | 최종 보상/종료 예정 |
| Result 뒤 새 처치, 이전 확정 명령 재조회 | 추가 보상0, 명령 원응답 유지 | NotRun | G0 수명 경로 회귀 예정 |

빈칸이 없는 상태의 수용 검사는 방어적 규칙도 포함한다. P0의16종·같은 종류 여유 뭉치 최대1·인구20을 동시에 지키는 정상 보드는18개 칸을 모두 채울 수 없다(18뭉치의 최소 인구는22). 따라서 해당 테스트에 비정상/격리 보드 fixture를 사용했다면 실제 플레이 재현으로 기록하지 않는다. 인구20 거절은 실제 도달 가능한 정상 경계다.

## 상대에게 전달하고 통합하기

A에 배치 값/확정 시각/이동 잠금 복사본과 BoardManager 조회 API를 전달한다. A는 준비 UnitActor, 타이머 보존 등록/배치 적용, 중복 안전 제거, 생성·사망 시각을 포함한 서버 사망 이벤트를 제공한다. root가 G1 실제 통과 후 G2 착수를 지시하면 타입 선언→A/B 독립 기능과 명시 Stub→공동 확정→전투/경제 연결→실패 주입→실제 두 화면 순서로 진행한다. 이 메모 커밋을 받아도 G2 소스가 제공되는 것은 아니다.

## 이해 확인

- 보드 준비 Actor가 존재한다는 사실과 유닛이 전투에 등록됐다는 사실은 왜 다른가?
- 이동 잠금과 공격 예정 시각을 하나의 타이머로 합치면 보충/연속 이동에서 어떤 규칙이 깨지는가?
- RNG를 복사해 추첨해도 보드 실패 뒤 원본 RNG를 저장하면 어떤 검사가 실패하는가?
- 작은 변형: t10.2에 다시 이동을 요청하면 기존 잠금10.3 전에 거절되고 모든 원본이 보존되는지 기대값을 적는다.
- 작은 변형: 동시에 소환/판매가 들어오는 두 처리 순서에서 일반 판매 환급을 각각 계산한다.

## 단계 완료

- [x] 상태 원본·공동 확정·상대 계약의 책임을 메모했다.
- [x] 정상·실패·중복·종료·시간 경계 기대값을 구현 전에 기록했다.
- [ ] G1 실제 검수와 리뷰가 통과해 G2 구현 착수 조건을 충족했다.
- [ ] 실제 코드·에디터/화면·실행 증거·시작/완료 SHA를 연결했다.
- [ ] 시작점에서 수업을 재현하고 Verified 판정을 받았다.
- [x] 미검증 상태와 P1 비확장 범위를 명시했다.
