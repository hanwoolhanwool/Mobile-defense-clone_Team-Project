# 보드와 경제의 공동 확정 — P0 / B / G2-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-BOARD-01·TASK-ECON-01, [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [경제](../../../docs/design/SUMMON_ECONOMY.md), [뭉치 규칙](../../../docs/design/BOARD_UI.md#unit-stacks) |
| 참고 자료 제작 상태 | Draft — 서버 코드·UE 명령 픽스처 검증, 수업 시작점 조립 재현 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | 시작 `4861b987f3e2fe78bcc159d1b6a85008543a938b` / 역할 핵심 `84389ce22205c8906592465be4d40e42434dda01`, 최종 G2 완료 미정 |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A CombatEvents `3abb28e3023f7d8676c70d451fadadfdb4151f4b`, 준비 UnitActor `932bf449641b3c28d56f708a1ea90c9b38ac2596`, 표시 슬롯 `5880ce91a75c3eb6adbaefbfc940bded18914308` |
| 제공 코드 / 직접 작성할 코드 | 제공: G1 기반·JSON·A의 위 산출물. 직접 작성: Board/Economy 타입·서비스·준비/확정 검사. 테스트의 보상·20개체 직접 배치 픽스처는 제품의 소환 성공을 대신하지 않음 |

## 이번에 만들 동작

첫 소환은 골드100→80, 유료 소환 횟수0→1, 다음 가격22, 인구0→1을 함께 확정한다. 중간 실패는 RNG까지 보존한다. 3개체 뭉치의 이동·교환은 ID를 유지하고, 판매 보충은 다른 여유 뭉치의 최소 ID를 옮긴다. 합성은 같은 셀의 같은 종류 세 ID만 소비한다.

전체 규칙 설명은 [COMMON](../COMMON.md)과 정식 계약이 원본이다. 이 수업의 구조 선택은 상태 원본 세 개를 분리하는 데 있다. B BoardManager가 존재·셀·인구·잠금, B EconomyService가 돈·유료 횟수·RNG, A CombatService가 공격 타이머를 소유한다. Actor와 HUD에 별도 원본을 만들지 않는다.

## 코드 작성 순서

소스 루트는 `Source/Mobile_defense_clone/`다. 공통 헤더 완료 `9658c55d5a1b0f4b7fb4115db5c799791542ad4b`와 핵심 완료 SHA의 diff를 순서대로 비교한다.

1. `Board/LDBoardTypes.h`에서 배치 값, 변경 사유, Commit, 읽기 전용 Snapshot을 선언한다. `Economy/LDEconomyTypes.h`에는 RNG를 제외한 개인 경제 Snapshot을 둔다. BoardRevision/EconomyRevision은 기존 응답과 같은 int32다.
2. `Economy/LDEconomyService.h/.cpp`: `Initialize(Context, Data, Seed)`가 두 참가자의 원본을 만든다. Player0은 Seed, Player1은 uint32 덧셈으로 Seed+2654435761을 사용한다. `TryPrepare`가 복사 RNG에서 등급→정렬된 UnitId 후보를 추첨한다. 가격·환급은 현재 데이터로 계산하고 실패 계획은 원본에 쓰지 않는다.
3. `Board/LDBoardManager.h/.cpp`: `ValidateCommand`에서 소유·현재 Revision·인구·잠금을 먼저 확인한다. `TryPrepare`는 값 배열을 복사하고 새 Actor만 비활성으로 준비한다. 소환은 여유 뭉치→화면 배치 순서 첫 빈칸이며 ID 발급 원본은 아직 증가하지 않는다.
4. Move는 선택 셀 전체와 목적 셀 전체를 교환하고 수동 이동 잠금만 `max(기존, 시각+.30)`으로 갱신한다. Sell은 한 ID 제거 후 필요한 기존 ID 보충을 수행한다. Merge는 정확한 세 ID 제거 후 결과 종류의 여유 뭉치→첫 빈칸을 사용한다. 타이머를 초기화하는 A 신규 등록 경로를 기존 개체에 쓰지 않는다.
5. `ValidatePrepared`는 두 Revision, RNG 원본, 예약 ID, 새 Actor 준비 상태를 마지막으로 확인한다. `CommitPrepared`는 검증된 값과 Actor 참조를 바꾸며 delegate를 호출하지 않는다. Processor가 두 서비스 확정 뒤 `PublishPrepared`를 호출한다.
6. 보드 게시에서 셀별 ID 오름차순 슬롯0~2를 A `SetPresentationSlot(slot, .15)`에 전달한다. 배치 적용→보드 통지→제거 Actor 비활성/Destroy 순서다. A 연결부는 통지에서 제거 ID를 먼저 전투 해제한다. 준비 취소는 멱등이며 임시 Actor만 정리한다.
7. `ApplyCombatReward`는 Match/EventId를 검증하고 양쪽 경제를 함께 바꾼 뒤 통지한다. 동일 사망은 한 번만 반영하며 RNG를 소비하지 않는다. 큐와 외부 명령 직렬화는 다음 수업에서 연결한다.
8. `Tests/LDGameplayCommandTests.cpp`: 수작업 가격·고정 seed 결과·예상 셀·원본 전체 signature로 검증한다. 없는 Actor를 반환하는 명시적 준비 adapter는 실패 경로 검사용이며 제품 설정 플래그가 아니다.

ARCH-01~06 적용 경로: 값 계약(Types), 단일 원본(두 Service), 명시 연결(Processor/GameMode), 공동 확정 전 검증(Prepared), 취소/제거의 멱등 정리(Cancel/Publish/Close), 독립 기대값(Tests)로 나눈다. 미래 장르용 범용 트랜잭션 프레임워크는 만들지 않는다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | 기존 G1 `Content/LD` 맵·JSON | Schema2/Rules0.3.0 그대로 | 데이터와 셀 원본 유지 |
| 2 | A 네이티브 LDUnitActor | 준비: Hidden, NoCollision, Replicates=false | 확정 전 유령 유닛·공격 없음 |
| 3 | A UnitActor 표시 | `/Engine/BasicShapes/Sphere`, `/Game/LD/Materials/M_P0Flat`, 슬롯0~2, VisualMoveSeconds=.15 | 최대3개체가 별도로 보임; 논리 중심·사거리 불변 |
| 4 | 네이티브 LDGameMode 연결 | Board 확정→Combat 등록, Economy 변경→PC Snapshot | Blueprint에서 상태를 직접 수정하지 않음 |
| 5 | Session Frontend → Automation | `LD.P0.G2.Commands` | 실제 서비스+Actor 계산 검사; UI/네트워크 검수와 별도 |

이 수업에서 새 Blueprint·UMG를 작성하지 않는다. 실제 표시와 입력 연결은 G2-03에서 수행한다.

UE 자동화의 FGameplayFixture는 CreateWorld 뒤 `GEngine->CreateNewWorldContext(...).SetCurrentWorld(World)`를 등록하고, 종료 시 DestroyWorld 뒤 DestroyWorldContext를 호출한다. 처음에는 이 엔진 문맥이 없어 판매/준비 취소/합성에서 DestroyActor 경고가 발생했다. 제품 Actor 수명 코드를 숨기거나 경고를 무시하는 대신 fixture를 실제 엔진 정리 조건에 맞췄다. 경고가 있는 성공과 무경고 성공을 선별 요약에서 구분한다.

## 실행·실패·수정 기록

2026-09-18 통합 `dba8afa521c68dc48cc07207d22b7f72c06c4283`의 UE NullRHI 명령 자동화 5개가 성공했다(2건 무경고·3건 Actor 정리 경고). [선별 요약](evidence/G2-initial/commands-summary.json)은 전체33성공(30무경고+3경고)/1Fail을 보존한다. 처음 harness가 succeededWithWarnings를 합산하지 않아 성공을30개로 보고했으며 원본 report로 정정했다. 실패1건은 A G0의 옛 Stub 기대 문구로 전체 검수 Pass가 아니다. 픽스처 시간은 게임 FPS/성능 측정이 아니다.

| 입력/조건 | 독립 기대 결과 | 실제 결과 | 실행 범위 |
|---|---|---|---|
| 시작100, 소환4회 | 비용20+22+24+26, 잔액8, n4, 다음28 | Pass | AtomicSummonAndFailure |
| 다섯 번째 소환·NULL Actor 준비 | InsufficientResource / InvalidData, 돈·돌·n·RNG·보드·Revision 불변 | Pass | 같은 테스트, 명시 adapter |
| 일반 유닛 판매 | 현재 다음 가격28의 절반14 환급, n4 유지 | Pass | 같은 테스트 |
| seed1776, 보상으로 재원 마련 후 소환7회 | C01×7, 셀17에3·11에3·5에1 | Pass | StackMoveMergeAndRefill |
| 첫 full stack ID1 판매 | ID7이 셀17로 보충, 동일 Actor·ID, 인구6 | Pass | 같은 테스트; A NextAttackAt 결합 별도 |
| 이동 직후 .2초 재이동 | Locked, 모든 게임 원본 불변 | Pass | 같은 테스트 |
| 다른 요청 확정 후 이전 준비 계획 | 마지막 검증 실패, 취소2회도 새 원본 보존 | Pass | RewardsAndPreparedRevision |
| 인구20, 상대 ID 판매 | LimitReached / NotOwner, 양쪽 원본 불변 | Pass | PopulationOwnershipAndPreparedIsolation |
| normal 중복, 보스30초/30초 초과 | 각 금화301·돌5, 중복 추가0 | Pass | RewardsAndPreparedRevision |
| 실제 첫 소환→기본 공격→처치→보상 | 실제 RPC와 양쪽 화면 일치 | NotRun | root 통합 실행 대기 |

추가 독립 기대 검사 `b7696baaa97c4bf0200ef9cb25dfad2888836354`는 실제 Editor Pass 후 `G2-supplemental-commands`에서 12건 중11Pass/1Fail이었다. `PreparedIsolationAndIdReservation`의 Actor 전역 IsHidden/GetActorEnableCollision 기대 두 항목이 실패했다. 원인은 제품이 각 PrimitiveComponent에 Visibility=false·CollisionEnabled=NoCollision·GenerateOverlapEvents=false를 설정하는데 테스트가 Actor 전역 플래그를 요구한 것이다. 제품 코드를 바꾸지 않고 실제 Primitive가 하나 이상 존재하는지 확인한 뒤 모든 Primitive의 비표시·무충돌·Overlap 비활성을 검사하도록 수정한다. 비복제·미확정·취소 Destroy·실패/취소 후 첫 성공 ID1 검사는 유지한다. 이 수정의 UE 재실행은 아직 대기다. 나머지 신규 전체 뭉치 교환·등급 판매/전설 합성 거절·2보스 사망 각10회 중복·방어적 비P0 배치 거절 검사는 모두 무경고 Pass였다.

방어적 NoSpace 검사는 M01 비P0 결과를 공개 준비 API에 전달한 거절이다. 정상 P0에서18칸 점유는 종류별 여유 뭉치 최대1·16종 조건 때문에 가득 찬 뭉치 최소2개가 필요해 인구최소22가 되므로 인구20 상한 안에서 도달할 수 없다. 이를 실제 포화 보드 검수로 표시하지 않는다.

## 상대에게 전달하고 통합하기

공통 헤더→A 준비 UnitActor→B 두 Service/Processor→A GameMode/Combat 연결→UI 순서로 통합한다. 전달 API는 `OnBoardCommitted`, `TryGetCommittedUnitActor`, `TryGetCellTransform`, `GetSnapshot`, `OnEconomyChanged`다. `GetRandomState`는 서버 검증용 읽기 조회이며 클라이언트 Snapshot/RPC에 넣지 않는다. B 독립 테스트에서는 실제 전투 처치 대신 명시적 서버 사망 값을 주입했으므로 A 처치 이벤트와 연결 후 다시 확인해야 한다. 완료 통합/실제 학습자 SHA는 아직 미생성이다.

## 이해 확인

- 복사 RNG를 사용해도 마지막 Revision 검증이 필요한 이유는 무엇인가?
- 판매 보충을 새 Actor 생성으로 구현하면 ID와 공격 예정 시각에 어떤 문제가 생기는가?
- 작은 변형: 실패 adapter를 사용해 준비 실패 전후 signature와 살아 있는 Actor 수를 비교한다. 제품 소환 확률·초기 재화는 바꾸지 않는다.
- 다음 단계 조건: 현재 서비스 검사 Pass에 더해 G2-02의 중복·수명 검사를 수행한다. G2 게이트 통과는 실제 A/B 연결 실행 뒤에만 판정한다.

## 단계 완료

- [x] 제공/직접 작성 범위·원본·순서·독립 기대값을 기록했다.
- [x] 실제 UE 명령 픽스처 결과를 소스 SHA와 연결했다.
- [ ] 시작점에서 수업 순서대로 별도 조립 재현했다.
- [ ] A 전투 타이머·실제 소유 RPC·패키지 UI와 재검증했다.
- [x] 미검증과 다음 의존성을 구분했다.
