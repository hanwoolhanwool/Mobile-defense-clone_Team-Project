# 예정 공격과 단일 사망 — P0 / A / G2-A-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-BATTLE-01 / [전투8.1·5.3](../../../docs/design/BATTLE.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작/코드 SHA | `4861b987f3e2fe78bcc159d1b6a85008543a938b` / 최초 `19aa7e1`, 예정 위치 수정 `28ead1bc530bef05cc9c06330238b9185a213a6b`, 명령 시각 수정 `eafc378163f3d9ed939821d153228f9b9d29943e` |
| 실제 개발 시작/완료 SHA | 자기 G1 통합 결과 / 미생성 |
| 상대 산출물 | A UnitActor, B 확정 배치·잠금 통지; 보상은 B 내부 큐 |
| 제공 / 직접 작성 | 제공: 검증된16행·G1 RouteModel. 직접 작성: CombatEvents/Rules/Service, EnemyActor HP 추가, Tests/LDCombatTests.cpp |

## 이번에 만들 동작

확정된 새 개체는 commit+.25부터 공격한다. canonical XY 사거리 안 최인접 적을 고르고 동률이면 SpawnSerial, EnemyId 순이다. 경로/플레이어 번호로 중앙 공용 적을 막지 않는다. 물리/마법 피해 계산 뒤 적 HP를 서버에서 낮추고 alive→dead를 한 번만 게시한다. 보상량을 전투 서비스에서 결정하지 않는다.

## 코드 작성 순서

1. `Battle/LDCombatEvents.h`: FLDDamageEvent와 FLDCombatDeath를 값 계약으로 선언한다. EnemyId는 매치 내 유일하므로 한 번의 사망 DeathEventId도 같은 값을 쓴다. UI 입력이나 보상 금액을 포함하지 않는다.
2. `Battle/LDCombatRules.*`: 순수 피해 계산과 target 선택을 만든다. 물리 방어 하한−50, 마법 저항0~.75, 마지막0.5올림만 적용한다. 유효하지 않은 입력은 출력값을 보존한다.
3. `Battle/LDEnemyActor.*`: InitializeRoute 뒤 InitializeCombat으로 HP/생성 사실을 준비한다. TryApplyDamage는 서버·MatchId·EnemyId·DamageEventId·생성 시각을 확인하고 같은 이벤트는 두 번 적용하지 않는다. 사망 때 경로를 멈추고 표시를 숨긴다. G1 fixture는 InitializeCombat을 호출하지 않으므로 기존 표시 동작을 유지한다.
4. `Battle/LDCombatService.*`: GameMode 소유 UObject에 InstanceId별 actor 약한 참조·NextAttackAt·예약을 둔다. 중복 등록/이동/보충은 기존 타이머를 바꾸지 않는다. 수동 이동은 B 잠금과 max를 취한다. 새 ID에만 .25초를 넣는다.
5. AdvanceCombatBefore는 외부 명령 시각보다 이른 사건만, AdvanceCombatTo는 해당 시각까지 처리한다. 같은 시각도 exclusive→inclusive 순서로 두 번 처리할 수 있어 명령이 경계 타격보다 먼저 확정된다. 예정 공격은 Enemy.TryGetCanonicalPositionAt(DueSeconds)의 순수 좌표로 사거리/nearest를 검사한다. canonical root는 해당 적의 예정 시각→마지막 스텝 시각 순서로만 진행하며 과거로 되돌리지 않는다. 이전에 대상이 없던 구간은 새 대상 관찰 시각부터만 예약한다.
6. 예정 시각→InstanceId의 전역 최소 사건을 반복 처리하고 확정 직전에 매치·등록·생존·현재 유닛 존재를 다시 검사한다. 큰 시각 도약에도 이미 예약된 후속 공격을 같은 순서로 처리한다. 피해 성공 뒤 NextAttackAt을 갱신하고 표시 cue를 게시한다. 사망 delegate 뒤에는 수신자가 상태를 제거했을 수 있으므로 이전 map 참조를 사용하지 않는다.
7. Stop은 반복 안전하게 등록·예약·사망 구독을 정리한다. 다른 World의 unit/enemy를 등록하지 못한다. 경제 객체를 탐색하거나 직접 변경하지 않는다.

ARCH-01/02는 순수 규칙→서버 서비스→actor 사실/표현으로 나눈 의존 관계, ARCH-03은 HP/타이머/보드 원본 분리, ARCH-04는 피해 확정 뒤 사망 게시, ARCH-05는 약한 참조·재검증·Stop, ARCH-06은 실제 코드 자동화와 명시적 fixture/실행 범위 구분에 적용한다.

## Unreal 설정 순서

| 순서 | 위치 | 값·연결 | 예상 관찰 |
|---|---|---|---|
| 1 | C++ GameMode 소유 서비스 | NewObject CombatService→Initialize(Context,Rules) | 클라이언트 자동 Tick 없음 |
| 2 | Enemy spawn 연결 | InitializeRoute→InitializeCombat→RegisterEnemy | HP70 N01 fixture는 정지 속도0 허용; 제품 속도150 그대로 |
| 3 | Board 확정 통지 | 현재 actor 조회→RegisterCommittedUnit(Unit,CommitSeconds) | 첫 예정 시각+.25 |
| 4 | GameMode 타이머 |20Hz 시각→AdvanceCombatTo→보상Drain | HUD 재화는 B 복제에서 갱신 |
| 5 | Blueprint·UMG·맵 | 새로운 저장 없음; 충돌/애니메이션으로 피해 적용 금지 | 공격 표현은 G2-A-01, HUD는 B |

## 실행·실패·수정 기록

독립 기대값 표는 [준비 계약 메모](G2_CONTRACT_NOTE.md)의 숫자를 그대로 사용한다. 테스트 기대값은 실행 결과에서 다시 생성하지 않는다.

| 자동화 | 검증할 독립 기대값 | 결과 |
|---|---|---|
| AllSixteenBasicAttacks |16행 모두 정확 사거리 첫 .25/다음 개별 간격 피해·표현 cue | 최초 통합 Pass |
| DamageAndTargetBoundaries | 물리15/방어20→13, −50→30; 마법19/.1→17, .75→5;175포함/175.001제외·동률 | 최초 통합 Pass |
| DuplicateDamageAndSingleDeath |70HP에40+40→HP0·사망1; 중복/오래된 매치/클라이언트 거절 | 최초 통합 Pass |
| InitialCadenceMoveAndReplenishment | C01 HP55/40/25/10/0, 이동/보충 NextAttackAt 보존·잠금 경계 | 최초 통합 Pass |
| SharedTargetRemovalAndStop | 양쪽 유닛이 Route1 중앙 적 공격, 판매 예약 취소, Stop 후 피해0 | 최초 통합 Pass |
| UnitPresentationPreservesCanonical | 슬롯·.15 보간은 canonical/타이머를 바꾸지 않음 | 최초 통합 Pass |
| ExactDuePositionAndNoBackdating | due.275 거리175/step.30 거리177.5는 .275 인정; 반대175.001→172.501은 .30부터 | 최초 통합 Pass |
| CommandClockOrdersEarlierHitAndSameTimeSale | hit10.025는 sale10.04보다 먼저; sale10.025는 타격 취소 | 후속 NotRun |
| EarlierKillFundsExternalPurchase | 명령 전 금27/가격28에서 hit10.025 보상+1→10.04 구매 성공·잔액0 | 후속 NotRun |
| 소스 스타일/공백 | 오류0 | Pass,50파일; Unreal 실행 증거 아님 |

실행 SHA·원본 로그와 후속 재검증 상태는 [A 공통 G2 증거](G2_EVIDENCE.md)를 따른다. 독립 정적 리뷰는 최초 서비스가 스텝 끝 위치로 과거 예정 타격을 판정하고, target 교체 시 사거리 진입 전으로 소급할 수 있음을 발견했다. `28ead1bc`는 정확 예정 시각의 경로 sample과 새 관찰 시각을 분리했고 첫7개 Unreal 자동화가 통과했다. 후속 조립 리뷰의 RPC 시각 결함은 `eafc378`에서 strict-before hook과 전역 예정 순서로 수정했다. 첫7개 Pass는 그 후속 코드의 검수 Pass로 재사용하지 않는다.

## 상대에게 전달하고 통합하기

CombatService의 공개 API와 Enemy HP 조회는 각 헤더 한 곳에서 관리한다. GameMode가 보드 확정으로 등록/제거하고 사망을 Processor.EnqueueCombatReward에 연결한다. B는 한 스텝 끝이나 다음 외부 명령 전에 보상을 Drain한다. fixture의 고정 UnitId/정지 적/직접 시각 주입을 정상 소환·이동 규칙으로 복사하지 않는다.

## 이해 확인

- 논리20Hz라도 commit+.25가 스텝 경계와 일치하지 않는 이유는 무엇인가?
- duplicate 피해와 서로 다른 두 치명타를 구분하지 않으면 보상이 어떻게 중복되는가?
- 작은 변형: 검증 fixture의 commit을 .025→.035로 바꾸고 정확 예정 위치를 손계산한다. 제품 수치는 변경하지 않는다.

## 단계 완료

- [x] 소스·상태·호출·독립 기대값을 기록했다.
- [ ] 시작점 재현·Editor/자동화와 SHA를 연결했다.
- [ ] B 실제 첫 소환→처치→양쪽 보상을 두 프로세스에서 검수했다.
- [x] 미검증은 자동화 실행·실게임·패키지·부하·Android다. G3 마감/웨이브/승패는 구현하지 않았으며 G2 통과 후 별도 검증한다.
