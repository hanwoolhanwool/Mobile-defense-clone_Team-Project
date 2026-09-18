# G2 A/B 연결 계약 준비 메모

**계획 메모만 작성했다. G2 소스·에셋·빌드·위젯 구현은 시작하지 않았다.** G1 실제 양쪽 화면 통과 후 루트의 착수 지시와 동일 통합 SHA를 받아 적용한다. 이 문서는 수업 Verified나 기능 완료 증거가 아니다.

근거: [공통 계약·초기화·확정 순서](../../../docs/technical/IMPLEMENTATION_SHARED.md), [A-03/04](../../../docs/technical/IMPLEMENTATION_A.md), [보드4.3/4.4](../../../docs/design/BOARD_UI.md#unit-stacks), [전투5.3/8.1/8.2](../../../docs/design/BATTLE.md), [경제6.1/6.2](../../../docs/design/SUMMON_ECONOMY.md), Schema2/Rules0.3.0의 GameRules·Units·EnemyTypes. 공통 문서의 전투5.4 참조는 현재 명세의5.3 경계 순서로 읽는다.

## 상태 원본과 합의한 공개 값

| 원본 | 소유자·변경 경로 |
|---|---|
| 매치 Phase/문맥 | GameState 원본, GameMode만 생성·연결·전이. GameMode가 서비스를 UPROPERTY로 소유하고 이벤트 연결/해제 |
| 존재·소유·CellId·인구·BoardRevision·이동 잠금 | B BoardManager. UnitActor의 배치는 파생 복사본 |
| 재화·PaidSummonCount·RNG·EconomyRevision | B EconomyService. CommandProcessor가 보드와 공동 확정 |
| 개체별 NextAttackAt·공격 예약/식별자 | A CombatService 서버 상태. actor Tick/애니메이션은 피해 계산 안 함 |
| 적 HP·alive→dead | A EnemyActor 서버 상태. A CombatService가 유효 피해를 요청하고 한 번의 사망 사실을 게시 |
| mesh/HUD/사거리 원 | 서버 값에서 파생된 로컬 표시. canonical 중심과 실제 RangeCm를 사용 |

B와 메시지로 합의한 값 계약:

- `FLDPlacedUnit`: `uint64 InstanceId`, `FName UnitId`, `int32 PlayerIndex`, `int32 CellId`, `double MoveBlockedUntilServerSeconds`. 마지막 필드는 B 이동 잠금 원본의 복사다.
- `FLDBoardCommit`: `FGuid MatchId`, `int32 PlayerIndex`, `uint64 BoardRevision`, `double CommitServerSeconds`, 변경 사유 `Summon/Move/Merge/Sell`, `AddedOrUpdatedUnits`, `RemovedInstanceIds`. 서버가 공동 확정 후 게시하는 값이며 클라이언트 쓰기 명령이 아니다.
- `FLDCombatDeath`: `FGuid MatchId`, `uint64 DeathEventId/EnemyId/SpawnSerial`, `FName EnemyTypeId`, `int32 SpawnWaveIndex`, `double SpawnedServerSeconds/DeathServerSeconds`. 금액·수혜자 필드는 없다. B가 검증된 서버 규칙에서 결정한다.

위 필드의 의미는 합의했으며 C++ enum/컨테이너 선언은 착수 때 하나의 헤더 원본으로 고정한다. 개체 ID를 뭉치 ID나 CellId로 대체하지 않는다.

## 호출 API와 거래 흐름

| 제공자 | 공개 API 제안 | 연결 이유 |
|---|---|---|
| A UnitActor | `bool InitializePrepared(const FLDPlacedUnit&, const FLDUnitRow&, const FTransform&)` | B가 비활성 actor를 준비. 성공해도 아직 등록/복제/공격 없음 |
| A UnitActor | `void ApplyCommittedPlacement(const FLDPlacedUnit&, const FTransform&)` | B가 준비 검증을 끝낸 뒤 새 actor 활성 또는 기존 actor 배치 갱신. fallible 로딩/스폰은 이 시점 전에 끝남 |
| B BoardManager | `TryGetCellTransform(PlayerIndex,CellId,OutTransform)`, `TryGetCommittedUnitActor(InstanceId,OutActor)`, `OnBoardCommitted(Commit)` | 원점 fallback 금지, 준비 중 actor 조회 금지 |
| A CombatService | `RegisterCommittedUnit(Unit,CommitServerSeconds)`, `UnregisterUnit(InstanceId)`, `AdvanceCombatTo(ServerSeconds)`, `Stop()` | GameMode 연결부만 등록·해제·스텝 수행. 중복 등록은 기존 타이머 유지 |
| A CombatService | `OnEnemyDeathCommitted(Death)` | 확정 사망을 GameMode 연결부에 게시 |
| B CommandProcessor | `EnqueueCombatReward(Death)`, 스텝말 내부 보상 Drain | 서버 내부 호출 전용, 클라이언트 보상 RPC 없음 |
| B EconomyService | `ApplyCombatReward(Death)` 내부 진입 | `(MatchId,DeathEventId,Recipient)`로 두 참가자에게 각각 한 번 지급 |

첫 소환 흐름은 CommandProcessor 검증 → Economy 후보/RNG 복사 → Board 후보/비활성 actor 준비 → 양쪽 최종 재검증 → 재화/RNG/보드/Revision을 실패 없는 내부 값 반영으로 공동 확정 → 배치 활성/Commit 통지 → GameMode가 제거 ID를 전투 해제하고 추가/갱신 actor를 조회하여 등록 → 그 다음 UI/최종 응답 게시다. 준비·재검증 실패는 actor 정리 후 원본 전부 불변이며 전투 등록0이다.

G2에서는 서버 UObject `ULDCombatService`를 GameMode가 명시적으로 소유하는 안을 사용한다. 세계 자동 Tick에 계산을 맡기지 않고20Hz 스텝 호출을 받으며, G1의 논리 시각과 현재 표시 시각을 분리한다. 규칙 계산은 별도 순수 함수로 두고 서비스는 등록·예정 이벤트·권한·순서를 담당한다. 최종 클래스명은 착수 때 공통 문서의 BattleSubsystem 초안과 함께 정리한다.

새 ID만 `NextAttackAt = CommitServerSeconds + InitialAttackDelaySeconds(0.25)`로 등록한다. 기존 등록·이동·보충에서는 NextAttackAt을 다시 만들지 않는다. 수동 Move는 B가 관련 모든 개체의 이동 잠금을 `max(기존, CommitTime+0.30)`으로 확정하며 공격 가능시각은 `max(NextAttackAt, MoveBlockedUntilServerSeconds)`다. 자동 판매 보충은 기존 ID/공격타이머/이동잠금 그대로이며 새0.25/0.30 지연을 만들지 않는다.

기존 위치가 바뀌면 UnitActor는 보드 복사본과 canonical 위치만 갱신한다. A 서비스의 ID별 공격 상태는 남아 있고 UI 사거리 원은 같은 raw 중심/RangeCm를 투영한다. 같은 BoardRevision 재전달은 연결부에서 중복 반영하지 않는다. UnitActor의 EndPlay/준비취소/판매는 Enemy 사망 보상으로 취급하지 않는다.

## 기본 공격·처치·시간 순서

- 표적은 canonical XY 거리≤RangeCm인 살아 있는 적 전체에서 최인접→SpawnSerial→EnemyId 순이다. RouteIndex/보드 소유로 중앙 공용 적을 차단하지 않는다.
- P0 피해는 기본 공격력에 물리 `100/(100+max(Armor,-50))` 또는 마법 `1-clamp(Resistance,0,.75)`를 적용한 뒤 양수0.5올림 한 번이다. 강화/치명타/스킬/마나 실행은 없다.
- 같은 논리시각은 명령 → 예정 유효 타격 → 사망/보상 → 생성·적 수 패배 → 마감 → 승리 → 다음 웨이브 순이다. 다음 외부 명령/최종 Result 게시 전에 그 스텝의 확정 보상을 Drain한다.
- 사망 대상·제거 유닛·변경된 매치/종료 상태는 피해 확정 직전에 다시 검사한다. 동일 DamageEventId는 HP를 두 번 줄이지 않고, 서로 다른 동시 치명타도 alive→dead 전이/DeathEventId/양쪽 보상은 한 번이다.
- 판매가 먼저 확정되면 아직 발사되지 않은 해당 개체의 예약 공격은 취소한다. 이미 독립 예약된 투사체는 공격 당시 snapshot을 사용한다. 타깃이 없던 과거 시간에 새 타격을 만들어 소급하지 않는다.
- 현재 데이터에는 투사체 비행속도/게임 판정 지연 값이 없고 P0는 근접/투사체 **표현**을 구분한다. 기본 피해는 예정 공격시각에 확정하고 투사체는 그 사실을 표현하는 안을 루트에 전달했다. 게임 판정에 실제 비행 지연을 추가하는 것은 새 수치/규칙 결정이므로 임의로 도입하지 않는다.

종료 순서는 새 외부 명령 닫기 → 승인된 해당 스텝 피해/사망/보상 flush → 결과 게시 → 전투/생성 예약 정리 → GameMode 연결 해제다. Result 이후 새 사망은 보상하지 않으며 확정된 기존 명령 재전송은 B 결과 캐시를 사용한다.

## 구현과 독립적인 기대값

| 사례·픽스처 | 기대 결과 |
|---|---|
| 시작100, 첫 유료 소환1회 | 소환자80/유료횟수1/개체1, 상대100. 같은 명령 반복은 그대로 |
| 정지 표적 N01 HP70/Armor0, C01 공격15·간격1, commit0 | 실제 사거리 유지 픽스처에서 .25/1.25/2.25/3.25/4.25에5회 타격→HP55/40/25/10/0. 사망1회 |
| 위 일반 적 처치 | 양쪽+1. 첫 소환자81/상대101. Death 재전달10회도 추가0 |
| C01 사거리175 | XY175 포함,175+epsilon 제외. 같은 거리면 먼저 생성된 적, 같으면 낮은 EnemyId |
| 피해 계산 | C01 Physical15/Armor20→13, Armor−50→30; C03 Magic19/Resistance.10→17, .75→5 |
| 유닛 둘이 HP70 적에 서로 다른40피해 동시 확정 | HP0, 적 수−1, 사망/개인별보상 각1회. 동일 피해 이벤트 재전달은 추가HP감소0 |
| NextAttackAt10, 수동 이동9.90 | 기존 NextAttackAt10 유지, MoveBlockedUntil10.20, 가장 빠른 공격10.20. NextAttackAt11이면11 유지 |
| [3,1] 판매 보충 | 총인구4→3, 가장 작은 donor InstanceId가 앞 칸으로 이동, 해당 ID/타이머/기존이동잠금 그대로. 새 소환/등록 타이머 없음 |
| 준비 실패·소유권 실패·가득 참·최종 재검증 실패 | 돈/보드/인구/유료횟수/RNG/두Revision/기존타이머 불변, 신규 전투등록0 |
| 같은 commit/register 재전달 | 등록 수 증가0, NextAttackAt/기존 예약 동일 |
| 같은 시각 판매→미발사 공격 | 판매 확정 후 그 유닛의 새 피해0 |
| 예정 타격시각=Deadline / Deadline+epsilon | 전자는 마감 판정 전에 인정, 후자는 늦게 실행해도 소급 인정하지 않음(G3 실제 보스 경계로 재검증) |
| 빠른 보스 처치 정확30초 /30초+epsilon | 양쪽 보스1마리당100골드+3별 /100골드+2별. 최종 보상은 Result 전 flush |
| 준비취소·반복Stop·EndPlay 후 콜백 | 피해/사망/보상0, 등록·예약·구독 정리 반복 안전 |

정지 표적·고정 UnitId·임의40피해·시간 직접 주입은 검증 전용 픽스처다. 실게임 소환 확률/이동을 바꾸거나 해당 계산 결과를 실제 첫 소환→전투 성공 증거로 쓰지 않는다. G2 실제 통합에서는 기존 UI/명령으로 첫 소환부터 처치·양쪽 재화까지 관찰하고 이동/합성/판매/보충 뒤 공격 상태를 다시 확인한다.
