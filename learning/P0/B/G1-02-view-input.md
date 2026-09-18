# 두 화면의 카메라와 같은 좌표로 누르기 — P0 / B / G1-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-MAP-01·TASK-NET-01, [전장 UI](../../../docs/design/BOARD_UI.md), [B 구현 설계](../../../docs/technical/IMPLEMENTATION_B.md) |
| 참고 자료 제작 상태 | Draft — 역할 Editor/계산 Pass, 두 번째 실제 실행에서 세로6종 Pass·가로 Fail, 카메라 높이 수정 재검증 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | canonical G0 `9129c016efd4a652501f93658fd2831a830c8ab1` / 구현 `82cf174cab14746c4fc57867efd5c4522903ca2c`, 수정·검증 `d5274dccdc11e1736e59239e558e8fb2ace7ee14` |
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
| 4 | BoardPresentation.Camera | Orthographic, Pitch=-90, Yaw=90, Z2400, ConstrainAspectRatio=false, OverrideAspectRatioAxisConstraint=true, AspectRatioAxisConstraint=MaintainXFOV, UseCameraHeightAsViewTarget=false, near1/far10000 | 양쪽 화면 right=-WorldX, OrthoWidth를 실제 가로 폭으로 해석하고 카메라 높이의 자동 투영 보정 방지 |
| 5 | 1080×2340 기준 카메라 | OrthoWidth1260cm, 위치(0,-128.333,2400), 전장(60,520)~(1020,1600)px | 이 값은 layout 계산 결과. 고정 해상도 강제값이 아님 |
| 6 | Native UMG | SafeZone → CanvasPanel, 제목·상대 보드·내 보드·선택·거절 TextBlock, 전체 HitTestInvisible | 전장 입력을 차단하지 않는 G1 최소 표시 |
| 7 | Controller InputComponent | LeftMouseButton Press / Touch1 Press → 공통 InputScreenPosition | 추가 Blueprint 입력 그래프 없음 |
| 8 | viewport 변경 | UI SafeCanvas 물리 pixel rect → 동일 layout → CameraOrthoWidth/위치 갱신 | 화면비별 칸·길이 같은 배율 유지 |

별도 BP_Camera·BP_Board·WBP 생성은 이 구현에 필요하지 않다. 네이티브 표시 클래스가 위 값을 생성한다. 공통 Build.cs의 SlateCore 의존성은 통합 담당자가 반영한다. 구현 이유는 필수 화면/입력 계약을 코드로 재현하고 바이너리 편집 충돌을 줄이기 위해서다.

기준 화면 예상: 셀120×120px, 내 필드720×360px, 생성점표시(120,1540)px, 중앙(120,1060)→(960,1060)px. 플레이어0/1 모두 같은 화면 위치이며 canonical Y가 다르다. 선택 셀은 녹청색, 상대 선택 실패는 `상대 보드는 조작할 수 없습니다` 문구다. 위 값은 기대값이며 실제 캡처는 아직 미등록이다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 1080×2340 기본 화면 | field60,520,960,1080·셀120px | 실제 두 번째 실행 Pass | 통합 G1-two-process-fix1의 view-1 |
| 540×1170 첫 별도 프로세스 실행 | 셀60px·field480×540px가 창 안에 표시 | 최초 Fail: 셀130px, 가로 보드 잘림 → 축 수정 후 두 번째 실행 Pass | root 실제 화면/projection 관찰, 아래 실패 분석 |
| 두 번째 실행의 세로6종 | 540×1170·1080×2340·720×1280·720×1600·768×1024·800×1280, 표시/36중심/72모서리/EngineTouch | 양쪽 Pass | 통합 G1-two-process-fix1 view-0~5 |
| 두 번째 실행의1280×720 | 전체 field fit·셀 표시·실입력 일치 | 양쪽 각107 Fail, 셀 표시 소실 | [실패 요약](evidence/G1-build-tests/landscape-failure-summary.json), height 수정 재검증 전 |
| 비대칭 안전영역20/44/12/32 | field가 안전영역 안에 동일 배율 fit | Pass, 계산 검사 | 실기기 SafeArea는 별도 미검증 |
| 양쪽 화면36중심 | 자기18 Selected·상대18 NotOwner | 세로6종 Pass, 가로 Fail | 실제 Project/Deproject + 공통 InputScreenPosition |
| 자기18셀 내부4모서리±62cm | 실제 보이는 셀과 같은 CellId | 세로6종 Pass, 가로 Fail | root 독립 예상값 probe |
| 상대/경로/화면밖 누르기 | 선택 불변·구체적인 거절문구 | NotRun | 직접 실행 검수 |
| 연결 세대 변경/종료 | 위젯·로컬 Actor·선택 정리 | NotRun | 실제 UI 재생성/다음 매치 후속 |
| source style/diff | 오류0 | Pass | `Saved/P0Evidence/G1-B/style.log` |
| Editor 최초 빌드 | C++/UHT 컴파일 성공 | Fail: C4458 | [최초 결과](evidence/G1-build-tests/editor-initial-result.json), [실패 발췌](evidence/G1-build-tests/compiler-failure-excerpt.txt) |
| Editor 수정 후 재빌드 | C++/UHT 컴파일 성공 | Pass, UBT14.66초 | [수정 후 결과](evidence/G1-build-tests/editor-fixed-result.json) |
| UE 보드 자동화 | 3Pass/0Fail/0NotRun | Pass | [결과](evidence/G1-build-tests/automation-result.json), [세부 요약](evidence/G1-build-tests/automation-summary.json) |
| 실제 네트워크/패키지/Android | 각 범위의 검수 근거 확보 | 네트워크 화면 결과 미확정, 패키지/Android NotRun | 계산·컴파일 Pass와 구분 |

첫 실제 두 프로세스 검사에서 카메라 확대 실패를 발견했다. 540×1170 화면에서 기대한 셀60px 대신130px로 투영되었고, 셀0/5의 화면X가595/-55여서 보드가 좌우로 잘렸다. `FLDBoardViewportLayout` 계산은 맞았지만 CameraComponent가 축 제약을 재정의하지 않아 LocalPlayer의 MaintainYFOV 설정을 상속했다. UE5.8의 `Engine/Source/Runtime/Engine/Private/Camera/CameraStackTypes.cpp` 284~307행은 이 경우 XAxisMultiplier=1170/540을 적용하고, OrthoWidth를 그 값으로 나눈다. 따라서 실제 확대율도130/60=1170/540이 되었다. 계산 fixture에는 이 엔진 투영 단계가 없어서 검사3개가 Pass여도 실제 화면 실패를 잡지 못했다.

첫 카메라 수정 `cf3f61072ebb3fc5502c051cca62d1c5e23a9c04`는 생성자의 `bOverrideAspectRatioAxisConstraint=true`, `AspectRatioAxisConstraint=AspectRatio_MaintainXFOV` 두 설정이다. 기대값은540×1170에서 셀60×60px, 전장(30,260)~(510,800)px, 셀0/5 중심X420/120이다. 두 번째 별도 프로세스 실행에서 이 화면과 세로/태블릿6종의 투영·전체셀 입력이 양쪽 Pass였다. 실제 pair SHA는 `ceac593614e40548bcac6c5322faff2929d22568`, 2026-09-18 14:24~14:25 KST다. `UnrealEditor -game -RenderOffscreen` 실제 렌더링 두 프로세스이며 PIE·최종 패키지·물리 터치 검수가 아니다.

그러나1280×720만 양쪽 각107개 검사가 실패했다. host의 `view-6.png`를 직접 확인하면 전장 바탕만 남고 베이지 셀/적 표시가 사라졌다. 입력도 Z0 교차 거리가 음수여서 거절됐다. UE5.8 `CameraStackTypes.cpp::CalculateProjectionMatrixGivenViewRectangle`의 unconstrained 직교 경로는 `UpdateOrthoPlanes`를 호출한다. `SceneView.cpp::FSceneViewProjectionData::UpdateOrthoPlanes`는 UseCameraHeightAsViewTarget이 켜져 있으면 min(CameraZ, HalfOrthoWidth)를 추가로 전진시킨다. 이 가로 화면은 HalfOrthoWidth=2426.6667cm가 CameraZ=2400cm보다 커서 투영 원점이 Z−1로 이동했다. `bUpdateOrthoPlanes=false`만으로는 이 직접 호출을 막지 못한다.

두 번째 수정은 생성자의 `bUseCameraHeightAsViewTarget=false`다. 전장/입력 평면을 카메라 앞에 유지하며 정상 near-plane 보정만 허용한다. `LD.P0.G1.Board.EngineOrthoProjection` 회귀 검사는 실제 BoardPresentation의 CalcCamera와 UE 투영/역투영 함수를 통과시킨다. 독립 기대값은540×1170 셀60px,1280×720 셀36.9230769px, 입력 ray 원점이 Z7보다 높고 아래로 향함, Z0/Z7 표면이 clip 깊이[0,1] 안에 있음이다. 자체 레이아웃 계산을 다시 기대값으로 쓰지 않으며 첫 축 보정과 이번 높이 보정의 회귀를 함께 검출한다. 이 수정/새 검사의 Unreal 빌드·실행은 아직 NotRun이다. root 재빌드와 양쪽7화면 재검증 후 결과를 연결한다. 수정 SHA는 이 기록과 함께 변경한 BoardPresentation/Tests의 커밋으로 추적한다.

표시와 입력의 서버 좌표 불변, 동일 배율 확대, 터치 뒤 합성 mouse 입력 억제도 별도로 유지한다. 터치 발생 직후0.15초의 mouse 경로만 억제하며 직접 touch/probe는 같은 선택 경로를 유지한다.

최초 Unreal Editor 컴파일에서 AddLabel의 지역변수 `Slot`이 UWidget의 동일 이름 멤버를 가려 MSVC C4458 오류가 발생했다. `d5274dccdc11e1736e59239e558e8fb2ace7ee14`에서 지역변수를 `CanvasSlot`으로 바꿨다. 2026-09-18 14:13 KST의 실제 재빌드는 Pass, 14:15 KST `LD.P0.G1.Board`는 3Pass/0Fail/0NotRun이다. root는 같은 Windows11·UE5.8.2·MSVC 환경에서 직렬 실행했다. 실행 명령·SHA·시간은 위 JSON에 보존했고 전체 로그는 `Saved/P0Runs/G1-B-editor-fix1`, `G1-B-tests`에 있다. UBT14.66초는 빌드 시간이며 게임 프레임 성능이 아니다.

지금 보존한 검수 증거는 compact JSON과 성공/실패 구간뿐이다. 큰 engine.log는 학습 폴더에 복사하지 않았다. 실제 화면 캡처·양쪽 공통 입력·경로 두 바퀴·반복 UI 수명·패키지·Android는 별도 관찰 결과가 확보되기 전 미검증으로 유지한다. 수업 전체를 Verified로 바꾸지 않는다.

## 상대에게 전달하고 통합하기

커밋 `82cf174cab14746c4fc57867efd5c4522903ca2c`. root에 `IsLocalBoardReady`, `GetLocalParticipantIndex`, `InputScreenPosition`, `ProjectCellToScreen`, `GetSelectedCellId`, `GetLastHitCellId`, `GetLastCellInputResult`, `GetBoardViewportLayout`를 전달한다. A에는 `FLDViewTransform`과 `OnLocalViewReady(int32)`를 전달한다. root fixture가 A 적의 `SetLocalViewPlayerIndex`를 연결하고 서버에서 두 경로를 진행한다. 실제 G1 통합은 A/B Editor → 계산 → 두 별도 프로세스 화면/입력/경로2바퀴 → 수정 → 재검증 → 리뷰 순서다. 별도 패키지 PC 한 판은 G3 검수다.

완료 코드가 실제 학습자의 구현이라는 뜻은 아니다. 학습자는 자신의 G0 통합본에서 위 순서로 작성하고 자기 실행/통합 SHA를 남긴다. 수업 전체 재현이 아직 없으므로 Verified로 표시하지 않는다.

## 이해 확인

- 물리 viewport pixel과 UMG DPI 좌표를 섞으면 어떤 위치 오차가 생기는가?
- 카메라가 가로 화면에 맞춰 멀어져도 서버 사거리·경로 길이를 바꾸지 않는 이유는 무엇인가?
- OrthoWidth 계산 검사가 Pass인데 실제 셀이130px가 된 원인은 무엇이며, 실제 카메라 투영 검사가 왜 별도로 필요한가?
- 작은 변형: 안전영역 위쪽을44px 늘린 계산 fixture를 만들고 모든 field 모서리가 여전히 안에 들어가는지 확인한다.
- 작은 변형: 선택한 셀 뒤 상대 셀을 누르고 선택색이 유지되는지 확인한다.
- G2 진입 조건은 두 참가자의 전체셀 실제 입력과 양쪽2바퀴·동일 EnemyId/RouteIndex 검증 및 G1 통합 리뷰 통과다.

## 단계 완료

- [x] 제공 에셋·Native UMG·카메라·입력 연결 순서를 기록했다.
- [x] 역할 Editor/계산 실행과 수정 SHA·compact 증거를 연결했다.
- [ ] 통합 실제 화면 캡처·입력·두 바퀴 증거를 연결했다.
- [ ] 시작점으로부터 수업을 재현했다.
- [x] G2·패키지·실기기의 별도 의존성과 미검증 범위를 명시했다.
