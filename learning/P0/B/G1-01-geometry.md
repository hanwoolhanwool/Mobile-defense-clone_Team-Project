# 변하지 않는 셀과 로컬 좌표 — P0 / B / G1-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-MAP-01, [전장·UI v2](../../../docs/design/BOARD_UI.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 상태 | Draft — 실제 화면·수업 재현 검증 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | canonical G0 `9129c016efd4a652501f93658fd2831a830c8ab1` / 소스 `82cf174cab14746c4fc57867efd5c4522903ca2c` |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성, 실제 학습자는 자기 G0 통과 후 진입 |
| 필요한 상대 산출물·버전 | Schema2/Rules0.3.0 좌표, A의 EnemyId·RouteIndex 불변 계약 |
| 제공 코드 / 직접 작성할 코드 | 제공: canonical G0·JSON. 직접 작성: LDBoardGeometry와 LDViewTransform, 독립 기대값 테스트 |

## 이번에 만들 동작

두 보드의 36개 논리 CellId를 같은 월드 좌표로 읽는다. 각 플레이어는 자기 18칸을 선택할 수 있고 상대 칸은 NotOwner로 거절한다. 화면 관점은 참가자1의 Y만 반사하여 처리한다. CellId·서버 좌표·적의 경로 번호는 화면 관점 때문에 바뀌지 않는다.

공통 규칙·버전·실행 도구는 [공통 자료](../COMMON.md)가 원본이다. 여기서 만드는 Geometry는 유닛 존재·재화 원본이 아니다. G2 BoardManager의 권위 있는 보드 상태와 분리된 읽기 전용 좌표 조회다.

## 코드 작성 순서

소스 루트 `Source/Mobile_defense_clone/`에서 다음 순서로 작성한다.

1. `Board/LDBoardGeometry.h`: FLDBoardGeometry와 ELDCellInputResult를 선언한다. `Initialize`는 G0의 검증된 Rules를 받고, 좌표는 외부에 수정 가능한 컨테이너로 노출하지 않는다.
2. `Board/LDBoardGeometry.cpp`: 6열·3행·개인18칸과 유한 좌표를 검사한 뒤 임시36개 중심을 확정한다. 실패하면 이전 유효 geometry를 보존한다.
3. `TryGetCellCenter`는 논리 번호로 조회한다. `TryGetCellAtCanonicalPosition`은 140cm 정사각형 전체를 판정하며 공유 경계는 반열림 구간으로 정확히 한 칸에 속한다. 길은 셀이 아니다.
4. `ValidateSelection`에서 셀 범위와 개인 소유를 나눠 검사한다. 실패는 임의 원점·기본 셀 성공으로 대체하지 않는다.
5. `Board/LDViewTransform.h`: `ToPresentation(Vector, PlayerIndex)`와 `ToCanonical(Vector, PlayerIndex)`를 같은 자기역 Y반사로 작성한다. 카메라 양쪽 yaw90 기준에서 화면 오른쪽은 -WorldX다.
6. `Tests/LDBoardGeometryTests.cpp`: 구현에서 값을 꺼내 기대값을 만들지 않고, 확정된 셀 중심·v2 화면 위치·좌우/상하 의미를 별도 상수로 적는다.

ARCH-01~03: 순수 좌표 계산은 보드 경제·Controller·위젯을 호출하지 않는다. 서버 규칙과 로컬 좌표의 소유권을 분리한다. ARCH-04: 유효한 전체 좌표를 만든 뒤 교체한다. ARCH-05: 이 값 타입은 UObject/타이머를 소유하지 않는다. ARCH-06: 계산 검사와 다음 수업의 실제 카메라/입력 검사를 구분한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | Content/LD/Data/GameRules.json | Board.Columns6, Rows3, CellsPerPlayer18, CellSizeCm140 | 현행 제공 데이터. 에디터에서 임의 수정하지 않는다 |
| 2 | 네이티브 Geometry | X=-350,-210,-70,70,210,350; 아래Y=-420,-280,-140; 위Y=140,280,420 | CellId는 보드0=0~17, 보드1=18~35 |
| 3 | 네이티브 ViewTransform | player0 identity, player1 Y반사 | 자기 보드 아래, 생성점 왼쪽 |
| 4 | Session Frontend → Automation | LD.P0.G1.Board.CanonicalCellsAndOwnership, LocalReflectionContract | 월드 화면 없이 실행하는 계산 검사 |

이 수업만으로 새 Blueprint·UMG·맵은 만들지 않는다. 실제 표시 객체와 입력은 다음 수업에서 연결한다. 예상 관찰은 보드0의 화면 첫 칸 Cell17, 보드1의 화면 첫 칸 Cell23이다. 이것은 각자의 보드 위쪽 왼쪽 칸을 뜻하며 Canonical 행/열 번호를 바꾼 것이 아니다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 36셀 중심과 각 내부9점 | 같은 CellId 조회 | NotRun | CanonicalCellsAndOwnership |
| 자기18칸/상대18칸 | Selected / NotOwner | NotRun | 같은 자동화 |
| 중앙길(0,0) | OutsideBoard, 임의 셀 없음 | NotRun | 같은 자동화 |
| 공유 X경계(-280,-420) | Cell1 한 칸 | NotRun | 같은 자동화 |
| Rows를4로 바꾼 교체 | 실패·이전 좌표 유지 | NotRun | 같은 자동화 |
| 양쪽 생성점 변환 | 둘 다 표시좌표(490,-560), 역변환 원본 복원 | NotRun | LocalReflectionContract |
| 소스 스타일·diff 공백 | 오류0 | Pass | `Saved/P0Evidence/G1-B/style.log`:27checked/0errors |
| 실제 화면·입력·두 바퀴 식별자 | 다음 수업·통합에서 확인 | NotRun | 계산 Pass로 대체하지 않음 |

설계 중 제거한 오류 가능성: player1을 180도 돌리면 생성점의 X도 반전돼 오른쪽 생성이 된다. Y반사만 로컬 표시와 입력 역변환에 사용하도록 계약을 확정했다. 구현 후 실패·수정이 생기면 실제 로그와 함께 이 표를 갱신한다.

## 상대에게 전달하고 통합하기

커밋 `82cf174cab14746c4fc57867efd5c4522903ca2c`. A에게 순수 `FLDViewTransform` 두 API를 전달한다. A는 적 root·복제 snapshot을 canonical로 유지하고 visual mesh에만 변환을 사용한다. root 연결부는 로컬 Controller의 준비된 PlayerIndex를 A의 `SetLocalViewPlayerIndex`에 전달한다. 먼저 공통 변환 헤더 → B 표시/입력 → A 적 표시 → 실제 두 화면 순서로 통합한다. G1 통합 SHA와 실행 증거는 아직 없다. 실제 학습자 통합 기록도 아직 없다.

## 이해 확인

- 월드 X를 반사하지 않은 이유를 왼쪽 생성점과 중앙 이동 방향으로 설명한다.
- 화면에서 보이는 첫 칸이 논리 Cell0이 아니어도 문제가 없는 이유는 무엇인가?
- 작은 변형: 같은 화면 점을 player0/player1의 역변환으로 계산하고 원본Y가 어떻게 달라지는지 확인한다. 서버 좌표 데이터는 변경하지 않는다.
- 다음 단계는 실제 렌더링·투영/역투영을 연결한 G1-02다. 계산 검사만으로 G2 전투에 진입하지 않는다.

## 단계 완료

- [x] 제공 입력과 직접 작성할 코드·소유권·호출 순서를 기록했다.
- [ ] 시작점 재현과 UE 계산 검사 증거를 연결했다.
- [ ] A의 실제 적 표시와 두 화면에서 확인했다.
- [x] 실화면 검증과 G2 진입 의존성을 명시했다.
