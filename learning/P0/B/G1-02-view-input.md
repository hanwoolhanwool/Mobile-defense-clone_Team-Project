# 두 화면의 카메라와 같은 좌표로 누르기 — P0 / B / G1-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-MAP-01·TASK-NET-01, [전장 UI](../../../docs/design/BOARD_UI.md), [B 구현 설계](../../../docs/technical/IMPLEMENTATION_B.md) |
| 참고 자료 제작 상태 | Draft — 역할/통합 실제 검수·수업 재현 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | canonical G0 `9129c016efd4a652501f93658fd2831a830c8ab1` / 소스 `82cf174cab14746c4fc57867efd5c4522903ca2c` |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | [G1-01](G1-01-geometry.md), G0 참가자 PlayerIndex, root 제공 M_P0Flat/기본 맵, A 경로 표시 |
| 제공 코드 / 직접 작성할 코드 | 제공: 공통 맵·Unlit 재료·엔진 Cube, root 실행 probe. 직접 작성: BoardPresentation·Native UMG·Controller 로컬 입력·ViewportFit 테스트 |

## 이번에 만들 동작

각 클라이언트가 자기 로컬 보드 표시와 직교 카메라를 만들고, 실제 SafeZone 크기에 맞춰 전장을 정사각형 칸으로 배치한다. mouse/touch와 실행 검수 probe는 동일한 `InputScreenPosition`을 사용한다. 잘못된 위치나 상대 보드를 누르면 실패 이유를 표시하고 이전 자기 선택은 보존한다. G1에는 소환·이동 명령·전투·재화 처리가 없다.

공통 도구·로그 구분은 [공통 자료](../COMMON.md)를 따른다. 이 수업의 핵심은 위젯 좌표를 서버 보드 좌표로 사용하지 않고, 실제 카메라 역투영과 G1-01의 Y반사를 연결하는 것이다.

## 코드 작성 순서

1. `Board/LDBoardGeometry.*`의 FLDBoardViewportLayout: 안전영역에 v2의960×1080 전장을 동일 배율로 fit한다. 원본1120×1260cm에 대한 PixelsPerCm, OrthoWidth, 카메라 중심을 한 계산에서 만든다.
2. `Board/LDBoardPresentation.*`: 로컬 Actor root는 원점에 둔다. CameraComponent와 각 셀의 visual component를 소유한다. 배경의 짙은 경로·베이지 셀·선택 색은 M_P0Flat의 Color를 사용한다. 이 Actor는 복제하지 않고 충돌·Tick을 끈다.
3. `UI/LDG1BoardWidget.*`: Native UUserWidget 안에 USafeZone → CanvasPanel → 제목/보드 구분/선택/실패 문구를 구성한다. 화면을 이미지 한 장으로 그리지 않는다. SafeCanvas의 실제 geometry를 LocalToViewport로 변환해 물리 pixel 안전영역을 Controller에 제공한다. 위젯은 서버 상태를 변경하지 않는다.
4. `Core/LDPlayerController.*`: 기존 RPC를 유지한다. 로컬 PlayerTick은 복제 PlayerIndex와 viewport 준비를 기다린 뒤 검증 데이터를 한 번 읽고 표시·위젯을 생성한다. viewport/SafeArea가 바뀌면 같은 layout으로 카메라를 갱신한다.
5. mouse/touch → `InputScreenPosition` → 전장 범위 → `DeprojectScreenPositionToWorld` → z0 plane → `ToCanonical` → 셀/소유 검증 → 선택 색/문구 순서로 연결한다. 실패는 서버 명령이나 비용 변화로 이어지지 않는다.
6. `ProjectCellToScreen`은 canonical z0의 셀 중심을 같은 변환으로 투영한다. root probe는 별도로 계산한 예상 좌표도 실제 UE 투영에 넣어, 자기 함수를 자기 기대값으로만 검증하지 않는다.
7. 연결 세대 변경·Logout/EndPlay는 위젯 제거, 로컬 표시 Actor 제거, 선택/좌표 캐시 초기화를 수행한다. `OnLocalViewReady`는 Controller 수명에 속하며 EndPlay에서 Clear한다.

ARCH-01/02: 표시 Actor는 Controller·경제를 찾지 않으며 위젯도 명령 상태를 소유하지 않는다. Controller가 생성·연결 경계다. ARCH-03: canonical geometry, 로컬 presentation, 서버 명령 상태를 분리한다. ARCH-04: 선택 실패는 기존 선택·서버 상태를 바꾸지 않는다. ARCH-05: UObject/Actor/위젯 참조는 UPROPERTY, 로컬 객체 정리는 Controller가 담당한다. ARCH-06: 실제 viewport 입력과 계산 검사를 구분한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `/Game/LD/Maps/L_P0` | 제공 BP_LDGameMode의 ControllerClass=LDPlayerController | G0 로그인/준비 경로 유지 |
| 2 | `/Game/LD/Materials/M_P0Flat` | Unlit/Opaque, VectorParameter Color → EmissiveColor | root 제공 공통 에셋, 동시 편집 금지 |
| 3 | `/Engine/BasicShapes/Cube` | 셀140×140cm, 경로 포함1120×1260cm, NoCollision/CastShadow=false | Native Actor가 Runtime component 생성 |
| 4 | BoardPresentation.Camera | Orthographic, Pitch=-90, Yaw=90, Z2400, ConstrainAspectRatio=false, near1/far10000 | 양쪽 화면 right=-WorldX |
| 5 | 1080×2340 기준 카메라 | OrthoWidth1260cm, 위치(0,-128.333,2400), 전장(60,520)~(1020,1600)px | 이 값은 layout 계산 결과. 고정 해상도 강제값이 아님 |
| 6 | Native UMG | SafeZone → CanvasPanel, 제목·상대 보드·내 보드·선택·거절 TextBlock, 전체 HitTestInvisible | 전장 입력을 차단하지 않는 G1 최소 표시 |
| 7 | Controller InputComponent | LeftMouseButton Press / Touch1 Press → 공통 InputScreenPosition | 추가 Blueprint 입력 그래프 없음 |
| 8 | viewport 변경 | UI SafeCanvas 물리 pixel rect → 동일 layout → CameraOrthoWidth/위치 갱신 | 화면비별 칸·길이 같은 배율 유지 |

별도 BP_Camera·BP_Board·WBP 생성은 이 구현에 필요하지 않다. 네이티브 표시 클래스가 위 값을 생성한다. 공통 Build.cs의 SlateCore 의존성은 통합 담당자가 반영한다. 구현 이유는 필수 화면/입력 계약을 코드로 재현하고 바이너리 편집 충돌을 줄이기 위해서다.

기준 화면 예상: 셀120×120px, 내 필드720×360px, 생성점표시(120,1540)px, 중앙(120,1060)→(960,1060)px. 플레이어0/1 모두 같은 화면 위치이며 canonical Y가 다르다. 선택 셀은 녹청색, 상대 선택 실패는 `상대 보드는 조작할 수 없습니다` 문구다. 위 값은 기대값이며 실제 캡처는 아직 미등록이다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 1080×2340 기본 화면 | field60,520,960,1080·셀120px | NotRun | ViewportAndSafeAreaFit 계산 + 실제 probe 별도 |
| 540×1170·720×1280·800×1280·1280×720 | 전체 field fit·정사각형 셀·입력 일치 | NotRun | root 실제 창 크기 변경 probe |
| 비대칭 안전영역20/44/12/32 | field가 안전영역 안에 동일 배율 fit | NotRun | 계산 검사, 실기기 SafeArea와 구분 |
| 양쪽 화면36중심 | 자기18 Selected·상대18 NotOwner | NotRun | 실제 Project/Deproject + 공통 InputScreenPosition |
| 자기18셀 내부4모서리±62cm | 실제 보이는 셀과 같은 CellId | NotRun | root 독립 예상값 probe |
| 상대/경로/화면밖 누르기 | 선택 불변·구체적인 거절문구 | NotRun | 직접 실행 검수 |
| 연결 세대 변경/종료 | 위젯·로컬 Actor·선택 정리 | NotRun | 실제 UI 재생성/다음 매치 후속 |
| source style/diff | 오류0 | Pass | `Saved/P0Evidence/G1-B/style.log` |
| Editor 최초 빌드 | C++/UHT 컴파일 성공 | Fail: C4458, 수정 후 재검증 대기 | `Saved/P0Runs/G1-B-editor/build.log`, UI/LDG1BoardWidget.cpp42 |
| UE 자동화/실제 네트워크·패키지/Android | 각각 근거 확보 | NotRun | 루트 직렬 실행 대기 |

현재까지 실제 게임 실패를 관찰한 기록은 없다. 구성상 피한 문제는 (1) 화면마다 서버 좌표 변경, (2) 화면비별 X/Y 별도 확대, (3) visual과 다른 좌표의 입력 검사, (4) 터치 뒤 합성 mouse 입력의 중복 전달이다. 터치 발생 직후0.15초의 mouse 경로만 억제하며 직접 touch/probe는 같은 선택 경로를 유지한다. 실제 실패가 발견되면 원인·수정 SHA·재검증을 추가한다.

최초 Unreal Editor 컴파일에서 AddLabel의 지역변수 `Slot`이 UWidget의 동일 이름 멤버를 가려 MSVC C4458 오류가 발생했다. 지역변수를 `CanvasSlot`으로 바꿨다. 이는 동작 변경 없이 소유·슬롯 의미를 더 명확히 한 수정이다. 실제 Editor 재빌드와 자동화 결과는 후속 실행에 연결한다.

## 상대에게 전달하고 통합하기

커밋 `82cf174cab14746c4fc57867efd5c4522903ca2c`. root에 `IsLocalBoardReady`, `GetLocalParticipantIndex`, `InputScreenPosition`, `ProjectCellToScreen`, `GetSelectedCellId`, `GetLastHitCellId`, `GetLastCellInputResult`, `GetBoardViewportLayout`를 전달한다. A에는 `FLDViewTransform`과 `OnLocalViewReady(int32)`를 전달한다. root fixture가 A 적의 `SetLocalViewPlayerIndex`를 연결하고 서버에서 두 경로를 진행한다. 실제 G1 통합은 A/B Editor → 계산 → 두 별도 프로세스 화면/입력/경로2바퀴 → 수정 → 재검증 → 리뷰 순서다. 별도 패키지 PC 한 판은 G3 검수다.

완료 코드가 실제 학습자의 구현이라는 뜻은 아니다. 학습자는 자신의 G0 통합본에서 위 순서로 작성하고 자기 실행/통합 SHA를 남긴다. 수업 전체 재현이 아직 없으므로 Verified로 표시하지 않는다.

## 이해 확인

- 물리 viewport pixel과 UMG DPI 좌표를 섞으면 어떤 위치 오차가 생기는가?
- 카메라가 가로 화면에 맞춰 멀어져도 서버 사거리·경로 길이를 바꾸지 않는 이유는 무엇인가?
- 작은 변형: 안전영역 위쪽을44px 늘린 계산 fixture를 만들고 모든 field 모서리가 여전히 안에 들어가는지 확인한다.
- 작은 변형: 선택한 셀 뒤 상대 셀을 누르고 선택색이 유지되는지 확인한다.
- G2 진입 조건은 두 참가자의 전체셀 실제 입력과 양쪽2바퀴·동일 EnemyId/RouteIndex 검증 및 G1 통합 리뷰 통과다.

## 단계 완료

- [x] 제공 에셋·Native UMG·카메라·입력 연결 순서를 기록했다.
- [ ] 역할/통합 실행과 실제 캡처·시작/완료 SHA를 연결했다.
- [ ] 시작점으로부터 수업을 재현했다.
- [x] G2·패키지·실기기의 별도 의존성과 미검증 범위를 명시했다.
