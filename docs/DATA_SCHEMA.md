---
id: DATA-SCHEMA
version: 0.2.0
status: Draft
owner: unassigned
updated: 2026-09-17
---

# 데이터 명세 및 UE5 가져오기

현재 SchemaVersion **2**, RulesVersion **0.3.0**. 데이터는 설계용 JSON이며 UE Row Struct·로더·게임 코드는 새로 구현해야 한다. 원본은 [생성기](../tools/build-design-data.mjs), 생성 결과는 data/, 규칙 원본은 기능별 기획 문서다. 구버전 필드를 묵시적으로 해석하지 않고 버전 불일치 시 시작을 거절한다.

## 1. 사용 순서

생성기 편집 → JSON 생성 → 정적 검증 → UE 구조체/로더 갱신 → 임포트·명령/전투·PC/Android 검수 순이다. P0/P1 기능 플래그를 로딩 시 고정한다. 고유 콘텐츠의 비활성 행을 발견했다고 활성화하지 않는다.

## 2. 공통 타입 규칙

DataTable은 객체 배열이며 Name은 고유 FName 행 키다. 같은 표의 모든 행은 같은 필드를 가진다. Id/enum 문자열·bool·int32 수량/가중치·float 시간/거리/계수를 구분한다. Weight는 0~10,000 정수, 확률 합계 10,000이다. None은 없는 참조를 나타낸다. 원시 Name과 콘텐츠 표시 이름은 구분한다.

GameRules는 중첩 설정 객체다. SummonMaxGold=null은 가격 상한 없음이며 0으로 변환하지 않는다. 시간은 서버 게임 초, 위치/속도는 cm·cm/s, RangeCells는 칸 한 변 대비 반지름이다. UI px를 거리로 쓰지 않는다.

## 3. DT_Units → FLDUnitRow

Name/DisplayName/Grade, DesignStatus, EnabledInP0, BaseAttack, AttackIntervalSeconds, AttackPresentation(Melee/Projectile), RangeCells, RangeCm, DamageType, SkillId, LegacySkillId, MaxStack, SaleCurrency, SalePolicy, SaleAmount, VisualId.

P0 활성 16종(일반~전설 각 4). M01~04는 DeferredConcept이며 비활성이다. SkillId=None, 기존 스킬 참조는 LegacySkillId로 보존한다. RangeCm=RangeCells×CellSizeCm, MaxStack=3. 일반은 HalfNextPaidSummonFloor 정책이며 SaleAmount=0을 실제 무료 판매로 해석하지 않는다. 희귀/영웅/전설은 Fixed Stars 1/2/4, 신화는 DisabledPendingConcept이다. 종전 SellGold 필드는 폐기한다.

## 4. DT_Skills → FLDSkillRow

기존 Trigger·EffectType·CooldownSeconds·TriggerAttackCount·DamageCoefficient·DamageType·RadiusCm·MaxTargets·ChainRadiusCm·SlowPct·StunSeconds·ArmorBreak·DurationSeconds·DotCoefficient·DotTicks·DotIntervalSeconds·AuraAttackPct·AuraHastePct·BossBonusCoefficient를 보존한다. Enabled=false, DesignStatus=DeferredConcept 추가. 16행은 컨셉 대기 참고 자료이며 현재 효과/마나 정책과 검증한 뒤 새 스킬 표로 확정한다.

## 5. DT_Recipes → FLDRecipeRow

OutputUnitId·LegendaryUnitId·EpicUnitId·RareUnitId와 각 수량·GoldCost를 보존하되 4행 모두 Enabled=false/DeferredConcept이다. 기존 레시피 재료·결과 위치를 현재 확정 규칙으로 사용하지 않는다.

## 6. DT_EnemyTypes → FLDEnemyRow

Name·DisplayName·Kind·DesignStatus·HPScale·FixedHP·SpeedCmPerSec·Armor·MagicResistance·StunImmune·SlowCap·AbilityId. 현재 N01~04는 후보, B01~08은 시험 슬롯이다. 웨이브는 N01만 활성 사용, P0 보스는 B01 한 종류의 2개체다. 보통/어려움 보스 StunImmune=false, SlowCap=0.8. 출처 없는 보스 고유 패턴은 활성화하지 않는다.

## 7. DT_SpawnProfiles → FLDSpawnProfileRow

N01Weight~N04Weight의 합계 10,000. Early는 N01 100%이며 현재 일반 웨이브 모두 Early를 쓴다. 다른 프로파일은 후속 후보다. 프로파일 수량 배분을 사용할 때 최대 나머지 방식·동률 ID 오름차순으로 합계 수를 보존한다.

## 8. DT_Waves → FLDWaveRow

80행, WaveIndex=1~80 연속. P0는 FinalWave=10으로 1~10행만 사용한다.

| 필드 | 일반 웨이브 | 10배수 보스 |
|---|---|---|
| DurationSeconds | 20 | 60(고정 진행 길이가 아니라 보스 제한) |
| SpawnWindowSeconds / SpawnIntervalSeconds | 20 / 1 | 0 / 0 |
| FirstSpawnOffsetSeconds | 0 | 0 |
| NormalCountPerGate / BossCountPerGate | 20 / 0 | 0 / 1 |
| BossDeadlineSeconds / PostBossDelaySeconds | 0 / 0 | 60 / 2 |
| GoldRewardPerPlayer / StarRewardPerPlayer | 0 / 0 | 0 / 0 |

NormalBaseHP·SpawnProfileId·BossId는 참조/시험 수치다. 보스 처치/빠른 보상은 개별 적 사망 이벤트에서 Economy 규칙으로 지급한다. 웨이브 보상 필드로 중복 지급하지 않는다. 조기 완료를 반영한 다음 시작 시각을 누적 계산하며 단일 고정 20초/60초 수식으로 80개 시작 시각을 미리 확정하지 않는다.

## 9. DT_SummonProfiles → FLDSummonProfileRow

Name·Source(Gold/Star)·Level·CostStars·FailWeight·CommonWeight·RareWeight·EpicWeight·LegendaryWeight·MythicWeight·PityThreshold. Gold_Default와 Gold_Lv01~10, Roulette_Rare/Epic/Legendary의 14행이다. PityThreshold=0, 보장 프로파일/카운터는 사용하지 않는다. 소환 확률 강화 단계와 행을 명시적으로 연결한다. 실제 가격은 Gold일 때 Economy의 유료 횟수에서 계산한다.

## 10. DT_Upgrades → FLDUpgradeRow

Name·Currency·Grades·MaxLevel·BaseCost·CostIncrement·Stat·ValuePerLevel·SuccessWeight. Grades는 쉼표로 나눈 허용 enum 목록 또는 AllSummonable이며 로딩 시 파싱/검증한다. MaxLevel=10, 다음 비용 BaseCost+CostIncrement×현재단계. 공격 3그룹은 ValuePerLevel=0.5, 소환 확률은 단계 인덱스 +1이다. SuccessWeight=10000. P0에서는 구매 거절, P1에서 활성화한다.

## 11. GameRules.json

Session, Board, Paths, Defeat, Victory, Timing, Economy, Combat, Difficulty, Hunt, Dungeon, Missions, Meta, Connectivity, Progression, TestProfiles, P0Overrides로 구성한다. 구 Path 단일 외곽 구조와 SchemaVersion1의 판매/확률/보상 해석은 폐기한다. Progression의 종전 계정 XP 값은 ProvisionalLegacyNotActiveP0이며 실제 메타 보상표를 대신하지 않는다.

### 11.1 중앙 합류 경로 — DEC-030/033/044

PointsByGateCm는 Lower [(490,-560),(490,0),(-490,0),(-490,-560)], Upper [(490,560),(490,0),(-490,0),(-490,560)]의 닫힌 선형 경로다. 각각 3080cm이며 중앙 (490,0)→(-490,0)을 공유한다. RouteIndex와 거리로 적 위치를 계산하고 중앙 이후 자기 생성 보드로 복귀한다. UI v2와 같은 칸/길 폭 비율을 맞춘 임시 140cm 시험 레이아웃이다.

Board는 6열×3행, 개인18칸, 기본 인구20, 합산 던전 인구다. CellId=BoardIndex×18+Row×6+Column. 자동 배치는 PlacementOrderByPlayer의 화면 기준 순서이며 단순 CellId 오름차순이 아니다. 좌표/간격과 두 참가자의 화면/입력을 함께 검수한다.

### 11.2 활성 단계·보정

P0의 AllowedGrades는 Common/Rare/Epic/Legendary, MergeInputGrades는 Common/Rare/Epic, EnableStars=true다. 스킬·강화·룰렛·사냥·미션·던전·신화·메타는 false다. Baseline은 일반 적100/인구20, AdjustedFixture는113/26이다. AdjustedFixture가 실제 유물 획득 구현을 의미하지 않는다. 유물 기능을 켜서 보정과 중복 적용하지 않는다.

## 12. 런타임 상태 계약

MatchId·PlayerId·ConnectionEpoch·RequestId·내용 해시·수락 순번으로 변경 명령을 식별한다. 같은 키 같은 내용은 결과 재사용, 다른 내용은 충돌 거절, 퇴출된 과거 요청은 재실행하지 않는다. BoardRevision/EconomyRevision을 공동 확정하며 RNG는 서버에만 둔다.

UnitInstance는 InstanceId, UnitId, Owner, Area(Field/Dungeon), CellId, Mana, NextAttackAt, SkillCooldowns를 갖는다. 이동·판매 보충·던전 왕복은 같은 개체를 이전한다. 같은 종류/구역별 여유 뭉치 ≤1, 인구 합산·판매-1·합성-2·이동0 불변식을 검사한다. 던전 ReturnCell/ReturnAll은 전체 목적지 사전 검증 후 전원 처리다.

EnemyInstance는 EnemyId·Kind·SpawnWave·RouteIndex·SpawnTime·HP·효과·마감을 저장한다. 상단 수는 Normal만 포함, 처치 보상은 SpawnWave 기준이다. 종료 경계와 처리 순서는 [전투 명세](design/BATTLE.md), 명령별 payload는 [아키텍처](technical/ARCHITECTURE.md)를 따른다.

## 13. 재생성·검증

node tools/build-design-data.mjs → node tools/validate-design-data.mjs. --check는 쓰기 없이 생성물/보고서 최신 여부를 검사한다. node --test tools/validate-design-data.test.mjs는 잘못된 확률·보상·뭉치·마감·복귀 계약을 주입해 검출 여부를 확인한다. 기획 문서는 node tools/build-planning.mjs와 node tools/validate-planning.mjs로 점검한다. 이 검사는 게임 실행 테스트가 아니다.
