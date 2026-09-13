---
id: DATA-SCHEMA
version: 0.1.0
status: Draft
owner: unassigned
updated: 2026-09-14
---

# 데이터 명세 및 UE5 가져오기

[기획 허브](README.md) · [관리 방식](WORKFLOW.md) · [데이터 원본/생성 관계](../data/README.md)

버전 0.1.0. 이 파일은 기획 데이터와 코드 사이의 계약이다. 동봉 JSON은 초기 데이터이며 프로젝트를 자동 생성하거나 에셋을 제공하지 않는다.

## 1. 사용 순서

DEC-021의 [원작 대조 기록](product/ORIGINAL_REFERENCE.md)을 먼저 확인한다. 현재 JSON은 기존 자체 설계의 데이터이며 원작 실측 데이터가 아니다. 재료 교환·잠금·Single 이동·타깃 토글 관련 필드가 있어도 해당 기능의 구현을 확정하지 않는다. 이번 변경은 적용 기준 정리로 필드·수치·RulesVersion을 유지하며, 원작 규칙 확인 후 생성기·JSON·명세·QA를 함께 수정한다.

1. UE5 C++ 프로젝트에서 아래 테이블별 `USTRUCT(BlueprintType)`을 정의한다.
2. 각 구조체는 `FTableRowBase`를 상속하고 `GENERATED_BODY()` 및 각 필드의 `UPROPERTY(EditAnywhere, BlueprintReadOnly)`를 둔다.
3. 에디터를 빌드한 다음 JSON을 DataTable로 가져오고 해당 Row Struct를 선택한다.
4. 첫 행의 `Name`은 RowName이며 별도 UPROPERTY로 중복 선언하지 않는다.
5. 나머지 필드 이름·대소문자·타입은 아래와 일치시킨다. 누락/추가 필드 경고를 무시하지 않는다.
6. UnitId·SkillId·RecipeId의 참조가 모두 존재하는지 에디터 검증기를 추가한다.
7. `VisualId`는 직접 에셋 경로가 아니므로 별도의 `DA_UnitVisuals`에서 연결한다.

설계 파일의 이름만 바꿔 UE 콘텐츠 폴더에 복사하면 자동 임포트·플레이되는 구조가 아니다. `GameRules.json`도 별도의 로더 또는 `DA_GameRules`에 옮기는 구현이 필요하다. P0는 `GameRules.P0Overrides`를 명시적으로 적용하여 영웅 이상·별조각·계정 보상을 비활성화한다.

DEC-019로 P0 강화·희귀 고유 스킬도 제외했다. 현재 JSON의 P0Overrides에는 이를 위한 별도 활성화 키가 없으므로 기존 JSON을 읽기만 해도 제한이 적용된다고 가정하지 않는다. TASK-DATA-01/TASK-COMBAT-01/TASK-ECON-01에서 P0 모드에 따른 스킬 실행 제한·강화 레벨 0·구매 거절을 구현하고 검증한다. 전체 DT_Units의 SkillId 참조와 P1 스킬·강화 데이터는 유지한다. 이번 문서 결정은 필드·수치·생성 JSON을 변경하지 않아 데이터 형식 버전과 RulesVersion은 0.1.0을 유지하며, 향후 명시적 활성화 필드를 추가할 때 생성기·로더·검증기·버전을 함께 변경한다.

## 2. 공통 타입 규칙

| 데이터 | C++ 타입 | 해석 |
|---|---|---|
| Name | RowName | 고유 키, 구조체의 필드 아님 |
| *Id, Grade, DamageType, Trigger, EffectType, Source, CounterMode, Kind, Stat | FName | 문자열 값 검증 후 코드 분기 |
| DisplayName | FString | 초기 한국어 명칭. 출시 시 String Table로 현지화 |
| 수량·레벨·비용·가중치 | int32 | 음수 불가 |
| 공격력·시간·거리·비율 | float | cm, 초, 0~1 비율. 공격력 최종 반올림은 피해 시점 |
| 면역/활성 여부 | bool | true/false |
| None | NAME_None | 참조 또는 기능 없음 |

FName 문자열을 C++ enum으로 교체할 경우 데이터 문자열 변환까지 같이 변경한다. 예를 들어 임의의 `ELDGrade` enum을 만든 뒤 변환 없이 기존 JSON이 같은 방식으로 들어온다고 가정하지 않는다.

## 3. DT_Units → FLDUnitRow

| 필드 | 타입 | 의미 |
|---|---|---|
| DisplayName | FString | 수호자 표시 이름 |
| Grade | FName | Common/Rare/Epic/Legendary/Mythic |
| BaseAttack | float | 강화 전 기본 공격력 |
| AttackIntervalSeconds | float | 기본 공격 간격 |
| RangeCm | float | 칸 중심 기준 XY 사거리 |
| DamageType | FName | Physical/Magic |
| SkillId | FName | DT_Skills의 RowName, 일반은 None |
| MaxStack | int32 | 칸당 동종 개체 수 상한, 1 또는 3 |
| SellGold | int32 | 개체 한 개 판매 환급 |
| VisualId | FName | 시각 DataAsset 키 |

초기 20행, 등급마다 4행. 자동 소환 종류 확률은 해당 등급의 모든 행에 균등하다. 도감 발견 여부로 풀을 필터링하지 않는다. P0에서 AllowedGrades로 제한하되 GoldProfile도 반드시 함께 바꾼다.

## 4. DT_Skills → FLDSkillRow

평평한 구조로 여러 스킬 유형을 담는다. 사용하지 않는 수치는 0이며 아무 효과를 만들지 않는다. 효과가 있는 스킬을 피해 0이라는 이유로 건너뛰지 않는다.

| 필드 | 타입 | 의미 |
|---|---|---|
| Trigger | FName | AttackCount / Cooldown / Aura |
| EffectType | FName | Stun, ExtraShot, AreaDamage, AreaDebuff, Chain, Aura, AreaBurn, ChainBoss |
| CooldownSeconds | float | Cooldown일 때 주기 및 첫 준비 시간 |
| TriggerAttackCount | int32 | AttackCount일 때 발동에 필요한 기본 공격 수 |
| DamageCoefficient | float | 직접 스킬 피해 계수, 0이면 직접 피해 없음 |
| DamageType | FName | Physical/Magic. 직접·DoT 스킬 피해에 적용 |
| RadiusCm | float | AoE 또는 오라 반경. 0이면 단일 |
| MaxTargets | int32 | 광역·연쇄 최대 수. 오라는 72 |
| ChainRadiusCm | float | 이전 적에서 다음 연쇄 후보까지 거리 |
| SlowPct | float | 둔화 비율. 보스 상한은 따로 적용 |
| StunSeconds | float | 기절 지속시간 |
| ArmorBreak | float | 절대 방어력 감소값 |
| DurationSeconds | float | 둔화·방어 약화·화상 유지 시간 |
| DotCoefficient | float | 화상 한 틱 계수 |
| DotTicks | int32 | 재적용 없는 화상의 예약 틱 수. 억제·사망·전투 종료 시 실제 피해 횟수는 줄어들 수 있음 |
| DotIntervalSeconds | float | 화상 틱 간격, 첫 틱은 적용 후 이 시간 뒤 |
| AuraAttackPct | float | 공격력 버프 비율 |
| AuraHastePct | float | 공격속도 버프 비율 |
| BossBonusCoefficient | float | ChainBoss 최초 대상이 보스일 때 추가 피해 계수 |

스킬 실행기는 EffectType별 행동을 구현한다. 데이터에 값이 있다는 것만으로 효과가 자동 생성되지 않는다. Stun은 유효 기본 공격 뒤 원래 대상이 살아 있을 때 적용하고 사망한 대상에서 다른 적으로 전이하지 않는다. Chain/ChainBoss는 원래 기본 공격 대상이 사망한 경우, 해당 공격 시점 위치를 중심으로 250cm 이내의 살아 있는 가장 가까운 적을 최초 대상으로 다시 선정한다. 후보가 없으면 해당 발동은 소멸한다. 이때 보스 추가 피해 여부는 실제 최초 연쇄 대상을 따른다.

AreaDebuff는 피해·둔화·방어 약화를 모두 지원한다. M03은 이 세 값을 사용하고, E01/L02는 직접 피해가 0이다. 같은 스킬에서 피해를 먼저 적용하고 살아 있는 대상에게 디버프를 적용한다. 자기 자신의 방어 약화가 첫 타격부터 소급 적용되지 않는다.

오라는 피해 표적 탐색을 사용하지 않는다. RangeCm와 독립적으로 RadiusCm 안의 양쪽 보드 수호자를 찾고, 자신도 포함한다. Aura의 MaxTargets=72는 팀 전체 논리 상한이다.

화상의 강도는 시전 시 ATK 스냅샷×DotCoefficient/DotIntervalSeconds로 비교하며 동률은 먼저 적용된 원천이 우선한다. 활성 원천만 틱 피해를 주지만 비활성 원천의 남은 시간과 예정 틱도 같은 시간축으로 소모한다. 강한 화상이 끝났다고 억제됐던 틱을 몰아서 적용하지 않는다. 동일 원천의 재적용은 ExpireTime을 갱신하고 최대3개의 향후 틱을 유지하되 이미 예정된 NextTickTime을 앞당기지 않는다. 이 규칙으로 재적용을 통한 즉시 추가 피해를 방지한다.

만료 시각과 같은 예약 틱은 포함한다. 현재 화상 데이터의 `DotTicks=3`, `DotIntervalSeconds=1`, `DurationSeconds=3`은 적용 후 1·2·3초의 예약을 뜻하며, 상태를 먼저 제거해 2틱으로 줄이지 않는다. 같은 시각의 틱·재적용·사망·전투 마감 처리 순서는 [전투 명세 8.3.1](design/BATTLE.md)이 원본이다. 이는 기존 3틱 명세의 해석 정리로 JSON 필드·수치·데이터 형식 버전·RulesVersion 0.1.0을 유지한다. 원작 화상의 존재·수치·중첩 방식이 검증되었다는 뜻은 아니다.

## 5. DT_Recipes → FLDRecipeRow

| 필드 | 타입 | 의미 |
|---|---|---|
| OutputUnitId | FName | 신화 결과 |
| LegendaryUnitId / EpicUnitId / RareUnitId | FName | 각각 전설·영웅·희귀 재료 |
| LegendaryCount / EpicCount / RareCount | int32 | 모두 1 |
| GoldCost | int32 | 현재 0 |

레시피는 입력 개체 ID 세 개의 **중복 여부**를 검사한다. 다른 종류의 레시피 요구를 같은 InstanceId로 동시에 충족할 수 없다. 각 재료의 소유자와 잠금 상태도 검사한다. 결과는 소비되는 전설 재료 칸에 배치한다. 제작 도중 아트 로딩 실패는 임시 메시로 대체하고 완료된 재화를 다시 소비하지 않는다.

## 6. DT_EnemyTypes → FLDEnemyRow

| 필드 | 타입 | 의미 |
|---|---|---|
| DisplayName | FString | 적 이름 |
| Kind | FName | Normal/Boss |
| HPScale | float | 일반 적: 웨이브 NormalBaseHP에 곱함 |
| FixedHP | int32 | 보스 고정 HP, 일반은 0 |
| SpeedCmPerSec | float | 기본 경로 이동 속도 |
| Armor | float | 기본 물리 방어 |
| MagicResistance | float | 기본 마법 저항 비율 |
| StunImmune | bool | 보스 true |
| SlowCap | float | 일반 0.5, 보스 0.25 |
| AbilityId | FName | None / SelfHaste / NormalEnemyHaste |
| AbilityIntervalSeconds | float | 보스 생성 이후 능력 주기 |
| AbilityDurationSeconds | float | 속도 효과 지속 |
| SpeedBuffPct | float | 속도 증가 비율 |

같은 종류 적에게 동일 가속이 겹치면 최대값만 적용한다. 최종 속도=`BaseSpeed*(1+StrongestSpeedBuff)*(1-EffectiveSlow)`, 기절 중은 0. B03의 가속 범위는 전장의 살아 있는 일반 적 전체다.

## 7. DT_SpawnProfiles → FLDSpawnProfileRow

N01Weight, N02Weight, N03Weight, N04Weight는 int32이며 합계 10,000. 적 구성은 무작위 추첨이 아니라 수량 배분에 사용한다.

개체 수 K에 대해 `ideal_i=K*weight_i/10000`, `count_i=floor(ideal_i)`를 먼저 계산한다. 나머지 개체는 `ideal_i-count_i`가 큰 유형부터 한 개씩 배정한다. 동률은 ID 오름차순이다. 실제 순서는 아직 남은 유형 중 `alreadySpawned_i/count_i`가 가장 작은 것을 고르며 동률은 ID 오름차순. count_i=0인 유형은 제외한다.

예: K=13, Runner 75/25이면 수량은 N01=10, N02=3이다. 두 생성점 모두 이 수량을 사용하므로 전체는 26마리다. K=12, Mixed는 각각 3마리다.

## 8. DT_Waves → FLDWaveRow

| 필드 | 타입 | 의미 |
|---|---|---|
| WaveIndex | int32 | 1~30 연속 |
| DurationSeconds | float | 30 |
| SpawnWindowSeconds | float | 일반 20, 보스 0 |
| NormalCountPerGate | int32 | 일반 적 생성점 하나의 수 |
| NormalBaseHP | int32 | 일반 적 기준 HP, 유형 배율 적용 전 |
| SpawnProfileId | FName | 일반 구성, 보스는 None |
| BossId | FName | 보스 유형, 일반 웨이브는 None |
| BossCountPerGate | int32 | 보스 웨이브 1, 그 외 0 |
| GoldRewardPerPlayer | int32 | 완료 시 개인별 골드 |
| StarRewardPerPlayer | int32 | 완료 시 개인별 별조각 |

각 웨이브는 일반 또는 보스 중 하나만 새로 생성한다. 이전 웨이브 적은 유지한다. P0에서는 1~10행을 사용하되 일반 적 프로파일을 Early로 오버라이드하고 별조각 보상을 비활성화한다. P0의 승리는 설정된 FinalWave=10의 보스·잔여 적 제거에 적용한다. 일반 GDD에서 언급하는 30웨이브 최종 판정을 이 모드에서 10으로 치환한다.

## 9. DT_SummonProfiles → FLDSummonProfileRow

| 필드 | 타입 | 의미 |
|---|---|---|
| Source | FName | Gold/Star |
| CostStars | int32 | Gold는 0, Star는 2 |
| FailWeight / CommonWeight / RareWeight / EpicWeight / LegendaryWeight / MythicWeight | int32 | 합계 10,000 |
| CounterMode | FName | ConsecutiveCommon / ConsecutiveBelowEpic |
| PityThreshold | int32 | 기본 프로파일에서 다음 시도 보장 진입 값 |
| GuaranteedProfileId | FName | 보장용 프로파일, None이면 없음 |

**CounterMode는 프로파일의 확률과 별개인 카운터 갱신 규칙**이다. 기본 프로파일 threshold에 도달하면 다음 시도에서 보장 프로파일로 추첨하고, 결과에 따라 카운터를 초기화한다. 보장 프로파일의 threshold=0은 다시 자기 자신으로 보장을 적용하라는 뜻이 아니다. Source별 기본 프로파일을 먼저 선택하고 그 아래에서 한 번만 전환한다.

골드 가격은 GameRules의 소환 횟수 공식으로 계산한다. Star_Default의 실패도 정상적인 확정 결과여서 별조각을 소비하고 카운터를 증가시킨다. 공간·비용·리비전 검사에서 거절된 요청은 추첨에 들어가지 않는다.

정수 r∈[0,9999]를 결과 순서 Fail→Common→Rare→Epic→Legendary→Mythic 누적 구간에 매핑한다. 선택된 등급 안의 4개 UnitId를 오름차순으로 정렬하고 별도 공정한 정수 추첨으로 하나를 고른다. Blueprint `RandomInteger(Max)`와 `RandomIntegerInRange(Min,Max)`의 상한 포함 여부를 혼동하지 않는다.

## 10. DT_Upgrades → FLDUpgradeRow

MaxLevel, BaseCost, CostIncrement는 int32, Stat는 FName, ValuePerLevel은 float. 현재 레벨 L의 다음 비용은 `BaseCost+CostIncrement*L`. 구매 후 L+1이 되며 효과는 `(L+1)*ValuePerLevel`. 최대 레벨에서 버튼 비활성 및 서버 거절을 모두 구현한다.

## 11. GameRules.json

섹션별 필드가 자체 설명식 이름으로 제공된다. Session, Board, Path, Defeat, Economy, Combat, Connectivity, Progression, P0Overrides가 있다. cm·초·정수 비용 규칙은 테이블과 동일하다.

JSON 설정을 로드할 경우 모든 키를 무조건 런타임 반영하지 말고 명시적 구조로 검증한다. 이 문서의 버전에서는 진행 중 매치에 데이터 핫 리로드를 하지 않는다. 로비에서 일치하는 RulesVersion/DataHash를 확정하고 매치 동안 고정한다.

## 12. 런타임 상태 계약

명령 데이터는 DEC-020의 [기술 명세 16.2절](technical/ARCHITECTURE.md)을 단일 원본으로 사용한다. 기존 범용 InstanceIds·DestinationCellId·TargetDataId 구조는 공통 ConnectionEpoch/RequestId/CommandType/ExpectedBoardRevision과 명령별 payload로 대체한다. 명령 타입·enum·필수 목록·결과 코드·길이 제한·중복 처리 순서는 해당 원본을 따른다.

이 변경은 구현 전 명령 계약을 구체화한 것이며 현재 DataTable/GameRules JSON 필드와 수치는 변경하지 않는다. 데이터 형식 버전·RulesVersion 0.1.0은 유지한다. 실제 RPC·입력 기록 형식에 적용할 때 BuildVersion 및 입력 로그 형식 버전을 함께 갱신하고 옛 범용 명령을 새 계약으로 추측해 읽지 않는다. 동일 빌드·모드·데이터 기준의 참가자만 매치를 시작한다.

| 구조 | 필수 필드 |
|---|---|
| FUnitInstance | InstanceId(uint64), UnitId, OwnerPlayerIndex, CellId, NextAttackTime, NextSkillTime, AttackCounter, MoveBlockedUntil, Locked, TargetPriority, GenerationId |
| FBoardState | BoardIndex, BoardRevision, Cells[12]의 InstanceIds, UnitsById |
| FEconomyState | Gold, Stars, SuccessfulGoldSummons, GoldPityCount, StarPityCount, AttackLevel, HasteLevel, ExchangeCount, EconomyRevision |
| FEnemyState | EnemyId, EnemyTypeId, SpawnSerial, GenerationId, HP, MaxHP, UnwrappedPathDistance, BaseSpeed, StatusSources |
| FStatusSource | SkillId, SourceInstanceId, EffectKind, Magnitude, StartTime, ExpireTime, NextTickTime, RemainingTicks, AttackSnapshot |
| FMatchState | Phase, RunStartTime, WaveIndex, WaveEndTime, ActiveEnemyCount, OverloadStartedAt, Result, FailureReason, RewardedWaveIds |
| FCommandSessionState | 서버 전용 MatchId·참가자 문맥, ConnectionEpoch(uint64), HighestAdmittedRequestId(uint32), 처리 중 요청의 정규화한 내용, 최근 확정 응답 256개와 원래 내용 |

FEconomyState는 서버에 영속하며 접속 PlayerController가 바뀌어도 참가자 상태에 연결한다. OwnerOnly 복제를 PlayerController에서 하더라도 서버 보관 원본까지 Controller 파괴와 함께 잃어버리지 않게 한다. InstanceId/EnemyId는 한 매치에서 재사용하지 않는다. 풀 액터 재사용과 논리 ID는 별개다.

## 13. 재생성·검증

```powershell
node tools/build-design-data.mjs
node tools/validate-design-data.mjs
```

첫 명령은 데이터 JSON을 덮어써 재생성한다. 직접 JSON만 편집하면 다음 생성 때 잃어버리므로 원본 스크립트 수치도 같이 변경한다. 두 번째 명령은 검증 실패 시 종료 코드 1, 성공 시 0이다. `docs/VALIDATION_REPORT.md`를 최신 결과로 갱신한다. 이 스크립트는 UE 컴파일/임포트 테스트나 전투 시뮬레이션을 수행하지 않는다.
