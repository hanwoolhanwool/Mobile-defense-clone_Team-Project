---
id: TECH-IMPL-SHARED
version: 0.1.9
status: Draft
owner: Codex
updated: 2026-09-17
reviewed: 2026-09-15
review_run: RUN-20260915-04
applies_to: A/B 구현 설계의 공통 계약·통합 순서·단계 제출 기준
---

# A/B 공통 구현 계약과 통합 순서

**새 착수 기준(2026-09-16):** [촬영본 기반 P0 재기획](../design/P0_REPLAN.md)의 G0~G4와 PLAN 검수를 먼저 적용한다. 종전 12칸/치수/경제 수치에 대한 통과는 기존 프로토타입 결과다. 새 레이아웃은 DEC-039의 6열×3행, 자기 18칸·상대 18칸 전체를 검사하고 원작 조작 근거를 연결한다. 기획용 생성 데이터는 DEC-043~047의 Schema2/Rules0.3.0이며 새 게임 코드와 실행 결과는 별도 검수한다.

[A 구현 설계](IMPLEMENTATION_A.md) · [B 구현 설계](IMPLEMENTATION_B.md) · [역할 분담](../production/TEAM_ROLES.md) · [시작 가이드](../GETTING_STARTED.md)

## 1. 이 문서를 사용하는 시점

두 개발자가 공통 초기 설정을 마친 뒤 첫 기능 코드를 작성하기 전에 함께 읽는다. A는 전투·웨이브, B는 경제·보드를 구현하며 한 프로젝트로 통합한다. 이 문서는 상대 기능을 호출하기 위해 필요한 최소 계약과 전달 순서를 정한다.

**현재는 구현 설계다.** 아래 클래스·파일·API·에셋은 앞으로 만들 대상이며 생성·컴파일·실행 완료된 결과가 아니다. API 이름과 추가 값 타입은 이번 설계안이다. 첫 공동 작업에서 실제 헤더로 옮기고 UHT·컴파일·직렬화 검증 결과에 맞춰 이 문서와 두 역할 문서를 함께 갱신한다.

규칙의 우선순위는 [결정 기록](../DECISIONS.md), [기능 명세](../README.md), [아키텍처](ARCHITECTURE.md), 이 문서 순서다. 특히 DEC-019의 P0 범위, DEC-020의 요청 계약, DEC-021/022의 원작 대조, DEC-026의 상태·수명 규칙, DEC-027의 역할을 유지한다. 기존 작업의 상태·선행 조건·실제 담당자는 [작업 보드](../production/BOARD.md)를 따른다.

## 2. 함께 준비할 최소 기반

[로드맵 G0~G4](../production/ROADMAP.md#p0-gates)에 맞춰 현재/최대 몬스터 수, 생성 시 고정 RouteIndex, 보스 도전 시작/제한/마감 시각, 결과/원인의 계약을 함께 작성한다. 기존 P0 참고 코드와 학습 작업은 [초기화](../production/P0_RESET.md)로 삭제했다. 새 로더·판정·복제·표시를 함께 설계/구현하며 이전 수업이나 실행 SHA를 출발점으로 삼지 않는다. 새 구현/실행 전에는 Verified로 표시하지 않는다.

| 항목 | A가 준비 | B가 준비 | 연결 완료 조건 |
|---|---|---|---|
| 빌드 기준 | 기존 모듈 안의 데이터·매치 타입 | 기존 모듈 안의 명령·보드 타입 | 같은 커밋에서 Editor 빌드, 템플릿 클래스에 새 기능 의존 없음 |
| 데이터 | Row Struct·로더·검증, 읽기 전용 P0 규칙 | 경제·보드에 필요한 필드·실패 조건 | 필드와 실제 로딩 경로 확인, 미확인 원작 수치는 별도 표시 |
| 매치 | GameMode·GameState·PlayerState, 매치/참가자 문맥 | Controller·CommandProcessor·EconomyService·BoardManager 초기화 진입점 | 필수 객체와 규칙 준비 전 명령 접수·전투 진행 없음 |
| 전장 | 경로 소비 조건·유닛 초기화 조건 | 보드 좌표·닫힌 경로·카메라·테스트 맵 | 양쪽 관점에서 같은 논리 ID, 경로 길이·좌표 변환 확인 |
| 화면 | 전투·결과 위젯의 입력 모델 | HUD 슬롯·로컬 Controller 연결 | 상대 위젯 내부 그래프를 수정하지 않고 삽입 가능 |

하위 기능을 전부 완성한 뒤 매치를 연결하지 않는다. 필요한 헤더·실패 반환·초기화 경계의 계약을 먼저 맞추고, 사용자 지시에 따라 공통 기반도 A/B 각 브랜치에서 독립 구현한다. G0 통합 때 두 구현을 비교하여 공통 실행 코드는 하나로 정리한다. A가 먼저 완성할 때까지 B가 기다리는 구조로 만들지 않는다. 임시 구현은 명시적으로 준비 안 됨/기능 비활성 결과를 반환하며 성공을 흉내 내지 않는다. 다른 기능은 월드 없는 계산 테스트나 검증 전용 픽스처로 진행한다.

### 중앙 경로 합의 — DEC-030

새 런타임은 [데이터 명세 11.1](../DATA_SCHEMA.md)의 두 경로 `Paths.PointsByGateCm` 계약을 공통 입력으로 사용한다. 기획용 생성 JSON 0.3.0의 두 경로와 동일하게 읽는다. Lower/0와 Upper/1의 생성점은 각각 (490,-560), (490,560)이며 (490,0)→(-490,0)을 공유한다. A는 경로 식별·생성·누적 거리 이동을, B는 동일 좌표의 Spline·중앙 통로 표시를 각자 구현한다. 첫 통합에서 같은 위치·같은 진행 방향·양쪽 유닛의 공용 적 공격을 검증한다. DEC-033에 따라 중앙 이후 자기 생성 보드 둘레로 복귀하는 순환은 확정 규칙이다. RouteIndex는 생성 시 정한 값을 유지한다. 3080cm 길이와 좌표는 6열 보드에 맞춘 임시 설계값으로, 3D 구도 검수에 따라 조정 가능하다. 화면 해상도 차이는 로컬 카메라/UI로 처리하며 서버 경로·사거리·이동 속도를 클라이언트별로 바꾸지 않는다. DEC-031에 따라 두 참가자 모두 자기 보드가 아래이며 생성점은 왼쪽이다. B가 로컬 뷰의 좌우 보정과 같은 행렬의 입력 역투영을 구현한다. 카메라 반전 때문에 상대 화면의 우측 생성을 허용하지 않는다.

### 보드 차원 합의 — DEC-039

플레이어당 Columns=6, Rows=3, CellsPerPlayer=18로 구현한다. CellId는 `BoardIndex*(Rows*Columns)+RowIndex*Columns+ColumnIndex`로 인코딩하고 행/열/소유 보드 범위를 따로 검사한다. 보드 0은 0~17, 보드 1은 18~35이며 카메라 반전으로 바뀌지 않는다. 인구 상한은 별도 필드로 유지하고 칸 수×3으로 계산하지 않는다.

## 3. 파일 편집과 상태 원본

소스 루트는 `Source/Mobile_defense_clone/`, 콘텐츠 루트는 `Content/LD/`다. 표의 경로는 이 루트에 상대적이며 신규 파일은 필요한 단계에서 만든다.

| 원본·연결 지점 | 작성 주담당 | 구현 경로·규칙 |
|---|---|---|
| 매치·참가자 문맥, 공용 상태 | A | `Core/LDGameMode.*`, `LDGameState.*`, `LDPlayerState.*` |
| 공통 값 타입·규칙 조회 | A, B 공동 검토 | `Data/LDMatchTypes.h`, `LDUnitRow.h` 등. JSON 필드는 DATA_SCHEMA 유지 |
| 사용자 명령·응답 | B, A 공동 검토 | `Network/LDCommandTypes.h`. ARCHITECTURE 16.2를 실제 타입으로 옮김 |
| 배치 확정 통지 | B, A 공동 검토 | `Board/LDBoardTypes.h`. 서버 보드 상태의 값 복사본 |
| 처치 결과 통지 | A, B 공동 검토 | `Battle/LDCombatEvents.h`. 서버에서 확정한 사실 |
| 서버 생성·구독·연결 | A | GameMode의 매치 초기화/종료 경로. 양쪽 구체 타입 연결은 이곳에 집중 |
| 로컬 생성·구독·연결 | B | PlayerController의 로컬 초기화/해제 경로 |
| 매치·웨이브·결과 상태 | A | 서버 GameState가 원본, GameMode가 전이 |
| 개인 재화·RNG·경제 Revision | B | 서버 EconomyService가 원본, PC 개인 복제본/PlayerState 공개 요약은 파생 값 |
| 유닛 존재·소유·칸·보드 Revision | B | 서버 BoardManager가 원본, UnitActor의 식별·칸 정보는 파생 값 |
| 적 HP·경로·사망 | A | 서버 EnemyActor가 원본, BattleSubsystem이 계산·처리 |

같은 바이너리 에셋은 한 사람이 편집한다. `BP_GameMode`는 A, `BP_PlayerController`·`WBP_HUD`·제품 맵은 B가 편집한다. A의 전투 테스트 맵은 `Tests/TestMaps/L_TestCombat`, B의 보드 테스트 맵은 `Tests/TestMaps/L_TestBoard`로 분리하고 통합 맵은 B가 반영한다. 맵의 조명 빌드 데이터와 외부 액터 파일도 같은 편집 범위에 포함한다. `Build.cs`, 공유 Config, 데이터 생성기는 변경 전 상대에게 범위를 알리고 직렬로 반영한다.

<a id="contracts"></a>

## 4. 최소 API와 값 타입 설계

아래 선언은 **네이티브 C++ 계약 초안**이다. 완전한 헤더나 Blueprint 노드 정의가 아니다. export 매크로, include, USTRUCT/UPROPERTY, delegate 선언, RPC의 UFUNCTION·직렬화는 실제 구현 때 추가한다. 네이티브 `uint64` 식별자를 모든 Blueprint 핀에서 직접 사용할 수 있다고 가정하지 않고, 화면에는 필요한 표시 값/선택 핸들을 별도로 노출한다.

| 값 타입 | 필요한 정보 | 원본·수명 |
|---|---|---|
| `FLDMatchContext` | `FGuid MatchId`, `FName RulesVersion` | A가 매치 생성 시 고정. FGuid는 이번 C++ 설계안 |
| `FLDParticipantContext` | MatchId, `int32 PlayerIndex`, `uint64 ConnectionEpoch` | 서버 참가자 등록/연결 정보. 클라이언트 payload에서 신원을 받지 않음 |
| `FLDPlacedUnit` | `uint64 InstanceId`, `FName UnitId`, `int32 PlayerIndex`, `int32 CellId`, `double MoveBlockedUntilServerSeconds` | B의 확정된 유닛 배치·이동 잠금 값. InstanceId는 매치 내 유일 |
| `FLDBoardCommit` | MatchId, `int32 PlayerIndex/BoardRevision`, `double CommitServerSeconds`, 변경 사유 Summon/Move/Merge/Sell, 추가/갱신된 FLDPlacedUnit 목록, 제거된 InstanceId 목록 | B가 공동 확정 후 게시. 수신자가 임의 보드 변경에 사용하지 않음 |
| `FLDCombatDeath` | MatchId, `uint64 DeathEventId/EnemyId/SpawnSerial`, `FName EnemyTypeId`, `int32 SpawnWaveIndex`, `double SpawnedServerSeconds/DeathServerSeconds` | A가 살아 있음→사망 전이에서 1회 생성. 재전달해도 동일 ID. 보상 금액·수혜자는 포함하지 않음 |
| `FLDCommand` / `FLDCommandResult` | 연결 세대·요청 번호·명령별 payload / 결과·두 Revision·생성/제거 ID·EventId | 기존 ARCHITECTURE 16.2 그대로. 서버 내부 보상과 구분 |

`CellId`는 두 보드를 통틀어 유일한 논리 칸 번호다. DEC-039의 인코딩에 따라 보드 0은 0~17, 보드 1은 18~35를 사용하며 보드 내부의 0~17 번호와 혼용하지 않는다. 서버 조회는 참가자와 CellId를 함께 받아 전체 범위와 해당 참가자의 보드 소유권을 검사한다. 카메라를 180도 돌려도 CellId·InstanceId를 재발급하지 않는다. 신규 struct의 필드 표기는 구현 시 고정하며 기존 JSON/명령 필드와 같은 의미의 별칭을 여러 개 만들지 않는다.

| 제공자 | 제안 API·이벤트 | 호출자와 의미 |
|---|---|---|
| B · Controller | `void InitializeServerSession(const FLDParticipantContext& Context, ULDCommandProcessor& Processor)` | A GameMode가 서버 참가자 등록 시 연결. Context는 서버 전용 |
| B · CommandProcessor | `FLDCommandResult Submit(const FLDParticipantContext& Context, const FLDCommand& Command)` | 서버 Controller만 사용자 요청 전달. 처리 중이면 Pending, 최종 응답은 완료 경로로 전달 |
| B · BoardManager | `bool TryGetCellTransform(int32 PlayerIndex, int32 CellId, FTransform& OutTransform) const` | 생성·연결 경로가 좌표 조회. 실패하면 원점에 대신 배치하지 않음 |
| B · BoardManager | `bool TryGetCommittedUnitActor(uint64 InstanceId, ALDUnitActor*& OutActor) const` | A의 서버 연결부가 확정 액터 조회. 준비 중 액터는 반환하지 않음 |
| B · BoardManager | `OnBoardCommitted(const FLDBoardCommit& Commit)` | A의 연결부가 추가/이동/제거를 구분해 전투 참가 상태에 반영 |
| A · UnitActor | `bool InitializePrepared(const FLDPlacedUnit& Unit, const FLDUnitRow& Row, const FTransform& Transform)` | B의 액터 준비 경로. 초기화만 하고 전투·충돌·복제·연출에 참여하지 않음 |
| A · UnitActor | `void ApplyCommittedPlacement(const FLDPlacedUnit& Unit, const FTransform& Transform)` | B의 확정/게시 경로. 기존 개체 이동은 공격 쿨다운을 초기화하지 않음 |
| A · CombatService | `void RegisterCommittedUnit(ALDUnitActor& Unit, double CommitServerSeconds)` / `void UnregisterUnit(uint64 InstanceId)` | A의 연결부에서 호출. 동일 ID 중복 등록/제거에 안전, 재등록은 기존 공격 타이머 보존 |
| A · CombatService | `AdvanceCombatTo(double ServerSeconds)`, `Stop()`, `OnEnemyDeathCommitted(const FLDCombatDeath& Death)` | GameMode가20Hz 논리 시각을 전달하고 확정 사망을 B의 내부 보상 진입점에 연결 |
| B · CommandProcessor | `void EnqueueCombatReward(const FLDCombatDeath& Death)` | 서버 연결부만 호출. 클라이언트 RPC 없음. 금액·대상은 서버 규칙으로 결정 |

최종 응답은 `(MatchId, 참가자, ConnectionEpoch, RequestId)`에 묶는다. 연결부가 해당 Controller의 소유 클라이언트 응답과 개인 복제 스냅샷 게시를 연결한다. 큐에 넣은 요청의 `Pending`은 실패나 환불이 아니다. Controller 교체 후 이전 세대 응답을 새 요청에 적용하지 않는다.

P0 월드 경로는 검증된 `GameRules.Paths.PointsByGateCm`의 닫힌 polyline을 A의 RouteModel/EnemyActor가 사용하고 B의 전장 표시가 같은 좌표를 사용한다. 맵 Spline 편집값을 별도 경로 원본으로 두지 않는다. 경제 계산이 경로 액터를 참조하거나 전투 계산이 EconomyService를 찾아가는 의존 관계는 만들지 않는다.

G2 연결은 GameMode가 UPROPERTY로 소유하는 `ULDCombatService`를 사용한다. 기존 BattleSubsystem 초안의 역할을 유지하면서 서버 수명과 고정 단계 호출을 명시한다. BoardManager가 존재·배치·이동 잠금의 원본이며 CombatService는 InstanceId별 `NextAttackAt`의 원본이다. 처음 등록한 개체만 확정 시각+.25로 초기화한다. 수동 이동은 B가 `max(기존 잠금, 확정 시각+.30)`을 적용하고 A는 `max(NextAttackAt, 이동 잠금)`부터 공격한다. 판매 보충은 기존 ID·잠금·공격 타이머를 유지하며 새 초기 지연을 넣지 않는다.

현행 P0 데이터에는 게임 판정용 투사체 비행시간이 없다. 기본 피해는 서버 예정 공격 시각에 확정하고 근접·투사체 연출은 그 사실을 표현한다. 표현 완료 콜백은 피해나 처치 보상을 다시 발생시키지 않는다. 이는 P0 기본 공격의 표현과 판정을 분리하는 구현이며 별도 비행시간 규칙을 추가하지 않는다.

<a id="lifecycle"></a>

## 5. 서버 초기화·유닛 생성·보상·종료 순서

### 5.1 매치 초기화

1. A가 매치 문맥과 공용 상태를 만들고 규칙/필수 에셋을 읽는다. 규칙 준비 실패는 사유와 함께 Aborted로 처리한다.
2. A의 연결부가 B의 보드와 경제·명령 객체를 생성/연결한다. GameMode가 UObject 서비스를 UPROPERTY 참조로 보관한다.
3. A가 참가자를 등록하고 B의 Controller 서버 진입점을 연결한다. 로그인과 데이터 준비 순서가 달라도 같은 초기화를 두 번 실행하지 않는다.
4. 서버 필수 객체·참가자 연결·데이터 준비가 끝난 뒤 명령 접수를 열고, 매치 Phase에 따라 전투를 시작한다.
5. B의 소유 로컬 Controller가 복제 상태 준비를 확인해 입력·HUD를 연결한다. UI Construct 순서에 매치 초기화를 의존시키지 않는다.

GameMode는 서버 전용, GameState는 클라이언트가 읽을 공용 상태의 복제 창구다. 맵별 GameMode Override와 GameMode의 클래스 기본값을 함께 확인한다. [Epic Game Mode and Game State](https://dev.epicgames.com/documentation/unreal-engine/game-mode-and-game-state-in-unreal-engine)

### 5.2 소환·이동·제거

DEC-037의 소환 변경안은 동일 소유/UnitId의 기존 여유 뭉치 추가 또는 새 뭉치 생성으로 구분한다. 셀/뭉치와 개별 InstanceId를 분리하고 확정 시 수용량 3을 재검증한다. A는 기존 개체의 상태를 유지한 채 새 개체를 등록하고 B는 수량·선택·합성/판매 UI를 갱신한다. 사거리 표시는 A의 실제 전투 값/위치를 공유한다.

1. B의 CommandProcessor가 요청을 검증하고 EconomyService와 BoardManager에 임시 변경안을 준비시킨다.
2. B의 BoardManager가 A의 UnitActor 클래스로 비활성 액터를 준비한다. 에셋 로딩·스폰·초기화 실패는 이 단계에서 반환하고 준비물을 정리한다.
3. 재검증 후 경제·보드·RNG·Revision을 공동 확정한다. 이 구간에는 Blueprint 이벤트·delegate·latent 호출을 넣지 않는다.
4. 확정 후 새 액터의 표시/복제를 활성화하고 배치 통지를 게시한다. A의 연결부가 새 유닛을 전투에 등록한다. 초기화 콜백 자체는 공격·보상·성공 이벤트를 내보내지 않는다.
5. 이동은 같은 InstanceId를 유지하고 배치만 갱신한다. 제거는 전투 등록·예약을 해제한 뒤 액터를 정리한다. 이미 수집한 전투 후보도 피해 직전에 유효성을 재확인한다.
6. 준비 취소와 일반 EndPlay를 적 처치로 해석하지 않는다. 준비 취소·이미 제거된 ID·종료 후 지연 이벤트는 부작용 없이 끝나야 한다.

보드 원본과 액터 생성이 따로 성공하지 않도록 한다. 확정 후 활성화가 실패할 가능성이 남으면 구현 단계에서 노출 전 전체 복구와 실패 주입 검증을 추가한다. 생성 성공만 확인하고 골드를 먼저 차감하는 구현은 완료로 인정하지 않는다.

### 5.3 처치 보상과 종료

A는 한 적에 대한 사망을 한 번 확정하고 식별 가능한 결과를 보낸다. B는 `(MatchId, DeathEventId, 보상 대상 참가자)`로 중복 적용을 막고 해당 참가자의 소환·판매와 같은 처리 순서에 넣는다. 보상량과 수혜자는 서버 규칙에서 읽고 클라이언트나 시각 VFX에서 받지 않는다.

한 전투 스텝에서 확정된 처치 보상은 다음 외부 명령 처리/최종 결과 게시 전에 정해진 내부 처리 단계에서 반영한다. 승패 판정 순서는 유지하고, 그 판정에서 승인된 최종 웨이브 보상까지 내부 확정 경로로 처리한 뒤 종료 결과를 게시한다. 두 사람은 구현 시 [전투 명세 5.3](../design/BATTLE.md)의 경계 순서와 대조한다. Result 이후 도착한 새 사망 통지로 재보상하지 않는다.

종료 시 A는 전투·스폰 예약을 멈추고 서버 연결을 해제한다. B는 새 명령 접수를 닫고 준비 작업·입력·UI 구독을 정리한다. 이미 확정된 동일 요청은 ARCHITECTURE 16.2의 캐시 규칙에 따라 원래 결과를 재전달한다. UI 제거는 이미 접수한 명령의 취소가 아니다.

<a id="milestones"></a>

## 6. 두 사람이 맞출 통합 지점

아래 번호는 개발 단계이며 새로운 백로그 작업 ID나 날짜가 아니다. 상위 TASK 완료 조건은 그대로 적용한다.

| 통합 지점 | A 단계 | B 단계 | 통과 증거 |
|---|---|---|---|
| 공통 기반 | A-01·A-02 | B-01·B-02 | 헤더·실패 반환·좌표 계약, 같은 커밋 Editor 빌드. 준비 전 요청 거절 |
| G1 전장/입력 | A-04의 두 경로 이동·공동 공격 | B-01/02의 3D 구도·카메라·입력 | 각자 2바퀴 자기 보드 복귀, 두 화면 방향·확정 전 셀 투영/입력 일치 |
| 첫 소환 | A-02·A-03 | B-03 | 두 화면에서 같은 유닛·소유·배치, 개인 재화 차감 1회. 실패 시 전체 원본 불변 |
| 한 웨이브 | A-04·A-05 일부 | B-04 | 소환→공격→사망→양쪽 보상, 이동/합성/판매 후 전투 등록 일치 |
| P0 완주 | A-05·A-06·A-07 | B-05·B-06 | PC 패키징 2인·지연·종료 경계와 Android 10웨이브 터치 완주 |

각 지점은 코드·콘텐츠·Config·데이터 기준 커밋, 재현 맵, 실행 순서, 정상/실패 기대 결과, 실제 결과를 한 묶음으로 전달한다. 헤더 변경을 받았으면 해당 Blueprint도 로드/컴파일하고 같은 통합 맵을 확인한다.

첫 소환은 A가 A-03의 준비 액터·API를 먼저 제공하고 B가 B-03의 실제 공동 확정에 연결한 뒤 함께 검수한다. A-03과 B-03의 전체 완료를 서로의 착수 조건으로 삼지 않는다. 최소 선언의 빌드 통과와 실제 기능 통합 통과를 각각 기록한다.

HUD도 B가 슬롯·수명 계약을, A가 표시 입력·이벤트 계약을 먼저 제공한다. A-06의 위젯 내부와 B-05의 HUD를 각각 만든 뒤 실제 삽입을 함께 검수한다. 상대 위젯/화면 전체 완료를 계약 전달의 선행 조건으로 삼지 않는다.

빠른 반복은 PIE의 Number of Players=2, Play As Listen Server로 시작할 수 있다. 단일 프로세스 PIE만으로 최종 통과하지 않고 PC 패키징 두 프로세스/두 장치 검수를 수행한다. [Epic PIE Multiplayer Options](https://dev.epicgames.com/documentation/unreal-engine/play-in-editor-multiplayer-options-in-unreal-engine)

<a id="delivery"></a>

## 7. 단계 제출 시 남길 기록

- 상위 TASK와 이번 단계, 실제 담당자·리뷰 담당자.
- 시작/완료 커밋, 변경 소스·에셋·공유 Config와 편집 순서.
- Unreal 설정 위치·프로퍼티 이름·값·설정 이유. 새로 만든 프로퍼티와 엔진 기본 설정을 구분.
- 필요한 상대 기능 버전, 변경한 API, 실패·종료·재시도 처리.
- 빌드/PIE/패키지/기기 중 실행한 범위, 재현 입력·기대값·실제값과 증거.
- 다음 단계에서 필요한 상대 산출물과 전달 조건.

공식 작업 상태와 실행 결과는 [작업 보드](../production/BOARD.md)·[검수 기록](../production/TEST_RUNS.md)에 연결한다. 문서 정합성 통과와 실제 기능 통과를 구분한다. P1 미배정 기능·P2 상세 담당·실제 일정은 이 문서에서 임의 배정하지 않는다.
