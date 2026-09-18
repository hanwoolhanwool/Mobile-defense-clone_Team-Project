# G1 경로·개체 준비 메모

초기 계획 메모를 보존한다. 이후 G0 통합 `4787bf1`을 받아 G1-A 코드 `bce4b7b`를 작성했다. 실제 구현/학습 절차는 [경로 모델](G1_01_ROUTE_MODEL.md), [적 개체](G1_02_ENEMY_ACTOR.md)를 따른다. 이 메모의 후보 API 중 실제로 선택한 선언은 코드 헤더가 원본이다. A Editor와 NullRHI 자동화5개는 [Pass](G1_EVIDENCE/README.md), 실제 양쪽 게임 화면은 **NotRun**이다. 전투·HP·피해·보상은 G1 양쪽 화면 통과 뒤의 범위다.

## 근거와 충돌 해석

- [DEC-030/031/033/039/043/044](../../../docs/DECISIONS.md): 두 화면 모두 왼쪽 생성·자기 보드 아래·중앙 왼쪽→오른쪽, 중앙 이후 자기 경로 복귀, 생성 시 RouteIndex 유지. 구2700cm·12칸·플레이어0에만 적용하던 좌우 해석은 현행 결정으로 대체됐다.
- [공통 경로 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [데이터11.1](../../../docs/DATA_SCHEMA.md), [A-04](../../../docs/technical/IMPLEMENTATION_A.md#a04), [PLAN-ROUTE-01/PLAN-VIS-02](../../../docs/design/P0_REPLAN.md): 현재 점4개·길이3080cm, 각 그룹 최소2바퀴 동일 개체 추적, 서버 좌표와 로컬 표시/입력 변환 분리.
- 공통 문서의 B Spline 제공은 구현 방법 초안이다. 이번 루트 지시에 따라 같은 검증된 PointsByGateCm를 직접 선형 보간한다. 데이터의 `Closed=true`, `LinearSegments=true`와 동작은 같다. 맵에 Spline이 없다고 경로를 다른 숫자로 추측하지 않는다.
- 현재 A API `ULDGameData::GetRules().PointsByGateCm/LengthPerGateCm`, `TryGetEnemyRow(N01).SpeedCmPerSec`, `FLDMatchContext`, GameState 서버 시각을 소비한다. A 브랜치의 Core는 통합 최신본과 다르므로 착수 전에 통합 G0 SHA를 받는다.

새 제품 결정은 없다. 로컬 반사와 복제 방식은 기존 확정 동작을 지키기 위한 구현 선택이다.

## 구현 단위와 연결 API 제안

| 파일/타입 후보 | 책임·API |
|---|---|
| `Battle/LDRouteModel.h/.cpp` | UObject 없는 경로 계산. `TryBuild(Points, OutDefinition, OutError)`, `TrySampleDistance(Definition, TotalDistanceCm, OutPosition, OutTangent, OutLap)`. 점·구간 누적길이·총길이를 초기화 때 계산하고 매 tick 재계산하지 않음 |
| `Battle/LDEnemyActor.h/.cpp` | 서버 발급 MatchId/EnemyId/SpawnSerial/RouteIndex는 수명 동안 고정. `InitializeRoute(...)`, `AdvanceRouteTo(ServerSeconds)`, `StopRoute()`, `GetRouteSnapshot()` const 조회. G1에는 HP/전투 API 없음 |
| `FLDRouteSnapshot` | 고정 식별자 + 단조 증가 TotalDistanceCm + SampleServerSeconds + 활성 여부. 누적거리를 나머지 거리로 덮어쓰지 않음 |
| Core 연결부(통합 담당자와 범위 합의) | G1 명시적 경로 시연 모드에서 N01 두 개를 서버가 한 번 생성. 각 경로에 ID1/2, SpawnSerial1/2. 서버20Hz 스텝 하나가 같은 시각을 AdvanceRouteTo에 전달. 반복 초기화는 추가 생성하지 않음 |
| B `Board/LDBoardGeometry.h` 후보 | `FLDViewTransform{LocalPlayerIndex}.ToPresentation(raw)` / `ToCanonical(shown)`. Player0은 그대로, Player1은 Y만 반사. 자기역 변환 |
| B Controller 후보 | `GetLocalParticipantIndex()`, `OnLocalViewReady(PlayerIndex)`. 소유 문맥 복제 도착/로컬 화면 재생성 시 동일 연결 경로. 등록/해제 주체는 Controller; actor가 매 tick 전체 Controller를 탐색하지 않음 |

`InitializeRoute`는 서버만 호출하며 문맥·0이 아닌 ID·RouteIndex0/1·유효한 경로·양수 유한 속도·유한 시작시각을 검증한다. 같은 초기화는 기존 값을 보존하고 다른 ID/경로로 재초기화는 거절한다. 이전/비유한 Advance 시각은 거절, 같은 시각은 부작용 없는 no-op다.

복제는 경로 snapshot 한 곳을 원본으로 사용하며 `ReplicateMovement`와 자체 이동 보간을 동시에 켜지 않는다. 고정4점 경로는 초기 복제 또는 검증된 로컬 공통 규칙 주입 중 통합 연결 비용이 작은 방식으로 확정한다. 클라이언트는 서버 sample과 동기화된 서버 시각으로 표시만 계산하고 ID·RouteIndex·권위 누적거리를 수정하지 않는다. 보간 지연/오차는 네트워크 검수 기록에서 별도로 측정한다.

actor root의 기준은 canonical raw 좌표다. B 변환은 표시 component의 위치·방향에만 적용한다. 예를 들어 raw Upper `(490,560,0)`은 Player1 표시에서 `(490,-560,0)`이 되지만 RouteIndex는1 그대로다. camera screen-right가 `-worldX`이면 두 참가자 모두 raw 중앙 `(490,0)→(-490,0)`을 화면 왼쪽→오른쪽으로 본다. 카메라 회전 때문에 EnemyId·CellId·경로를 재발급하지 않는다.

종료는 생성/진행 접수 닫기 → 서버 스텝 타이머 해제 → actor StopRoute(반복 안전) → 표시/문맥 구독 해제 → 시연 actor 정리 순서다. EndPlay·준비 취소는 처치나 보상 이벤트를 내지 않는다. 루트가 맵·에셋을 직렬 생성하며 A는 기본 표시 component 계약만 전달한다.

## 구현과 독립적인 기대값

현재 데이터 N01 속도150cm/s, 두 경로 길이3080cm다. 아래 숫자는 명세 좌표에서 직접 계산했다. 1바퀴 `20.533333…초`, 2바퀴 `41.066666…초`다. 실제 샘플은2바퀴를 넘길 때까지 수집하여 경계 양쪽을 검사한다.

| 누적거리(cm) | Lower raw 위치 | Upper raw 위치 | 불변식 |
|---:|---|---|---|
| 0 | `(490,-560,0)` | `(490,560,0)` | 처음의 ID·RouteIndex 고정 |
| 280 | `(490,-280,0)` | `(490,280,0)` | 중앙 진입 전 자기 영역 |
| 560 | `(490,0,0)` | `(490,0,0)` | 중앙 시작, 두 개체는 별개 ID |
| 1050 | `(0,0,0)` | `(0,0,0)` | 중앙 같은 진행 방향 `(-1,0,0)` |
| 1540 | `(-490,0,0)` | `(-490,0,0)` | 중앙 끝, 다음 구간은 각자 영역 |
| 2100 | `(-490,-560,0)` | `(-490,560,0)` | 상대 경로로 전환되지 않음 |
| 3080 | `(490,-560,0)` | `(490,560,0)` | Lap1, 누적거리3080 유지 |
| 6160 | `(490,-560,0)` | `(490,560,0)` | Lap2, actor/ID/RouteIndex/총개체수2 유지 |
| 6440 | `(490,-280,0)` | `(490,280,0)` | Lap2 이후 정상 진행, 방향 유지 |

필수 검사: 코너 `d±epsilon`, 0/1/2바퀴 정확 경계, 큰 단일 step의 다중 구간/바퀴 통과, 중복 시각 no-op, 역행/NaN/음수거리·0길이/점 부족 실패, 종료 후 Advance 무변경, actor 재초기화의 기존ID/거리 보존. 순수 계산 Automation과 실제 actor/복제/화면 검수를 구분한다.

실제 G1은 서버 로그의 MatchId/EnemyId/SpawnSerial/RouteIndex/거리/Lap/시각과 두 참가자 캡처를 짝지어 최소2바퀴를 확인한다. 양쪽 화면 자기 보드 아래·두 생성점 왼쪽·중앙 좌→우, B의18개 자기셀 및18개 상대셀 입력, 화면비별 투영/역투영이 모두 통과하기 전 G2 기본 공격을 시작하지 않는다. 경로가 계산 테스트를 통과했다고 화면 검수를 Pass로 올리지 않는다.
