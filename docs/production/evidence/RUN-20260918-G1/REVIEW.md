# G1 실행 및 독립 구조 리뷰

대상: A `bce4b7b`, B `82cf174`, 최초 통합 실행 `73b872b`. 실제 실패를 보존하고 수정본을 재검증한다.

| ID | 파일·함수·재현 | 영향·수정 | 상태 |
|---|---|---|---|
| R-G1-01 | Board/LDBoardPresentation.cpp 생성자. 실제540×1170에서 셀60px 대신130px, cell0 X595·cell5 X-55 | UE의 기본 Y축 FOV 유지가 OrthoWidth를 높이 기준으로 해석. Camera.bOverrideAspectRatioAxisConstraint=true, MaintainXFOV 명시 | `cf3f610` 수정, 실제 재검증 Pass |
| R-G1-02 | Battle/LDEnemyActor.cpp AdvanceRouteTo. actor tick 후 subsystem이 과거20Hz 단계시각으로 mesh 덮어씀 | host 표시가 최대7.5cm 뒤로 이동. canonical은 단계시각 유지, mesh는 현재 view clock 사용 | `bb1d100` 수정, HostStepKeepsCurrentViewClock 회귀 추가. 최초 host2실패/client0 |
| R-G1-03 | Verification/LDG1ProbeSubsystem.cpp 고정 벽시계 화면 전환 | 첫 스크린샷에서 프레임 정지 후 resize가 반영되기 전 검사·이전 터치 중 덮어쓰기. 실제 게임 결함과 검사기 결함을 구분 | 실제 크기·터치 종료·PNG 완료 후 다음 단계로 변경 |
| R-G1-04 | Run-P0Pair.ps1 예외/리뷰 | 둘째 프로세스 생성 실패 시 첫 프로세스 잔류 위험 | 생성한 Process 객체만 finally에서 종료, 결과/예외 보존 |
| R-G1-05 | 화면비·표시 검사 범위 리뷰 | 9:20·세로3:4 누락, 메시 가시성은 로그만 기록 | 7화면비·메시 좌표/가시성·Engine InputTouch 추가. OS 창의1440px 한도는 offscreen 실제 렌더링으로 별도 검증 |
| R-G1-06 | Board/LDBoardPresentation.cpp 카메라 생성자. 실제1280×720에서 모든 셀/적 메시가 사라지고 자기18칸 입력·72모서리·터치17건 실패 | UE의 unconstrained 직교 투영이 높이를 타깃 거리로 추정해 view origin을 Z−1로 이동. Z0 보드가 카메라 뒤가 됨. bUseCameraHeightAsViewTarget=false로 고정 카메라 높이를 유지 | `5a46b30` 수정, 실제 재검증 Pass |
| R-G1-07 | Tests/LDBoardGeometryTests.cpp EngineOrthoProjection. transient world에서 Camera 미활성 | CalcCamera가 perspective fallback해 양쪽 비율의 모든 투영 기대값 실패. InitializeActorsForPlay·Camera.Activate·mode/z 전제 검사 추가 | `d7daeb6` 수정, Editor·실제 UE 회귀1Pass |
| R-G1-08 | Board/LDBoardPresentation.cpp ApplyViewportLayout. 가로 화면에서3cm 간격이0.79px로 축소 | 실제 PNG에서 일부 세로 셀 경계가 사라짐. 표시용 inner mesh XY만 조정해 최소1.5px 간격 확보, 논리좌표·입력 불변 | `bac0077` 수정, 재현 worktree 양쪽 실제 가로 화면 Pass |

독립 리뷰는 실제 엔진 LevelTick.cpp의 actor tick → tickable subsystem 순서와 CameraStackTypes.cpp의 투영 계산을 대조했다. 상태·권한·의존·수명 검토에서 추가 G1 차단 결함은 발견하지 않았다. normal `LDLocalPresentationSubsystem`이 로컬 준비/actor 생성 이벤트를 연결하며 domain actor가 Controller/UI를 역탐색하지 않는다. 종료에서 timer/delegate를 해제한다.

첫 실제 결과: [실행 명령](first-pair.json), [실패 요약·두 바퀴 관찰](first-pair-summary.json), [잘린 실제 화면](first-host-clipped.png). 전체 로그·양쪽7캡처는 통합 `Saved/P0Runs/G1-two-process-first/`. 두 경로 ID1001/1002는 양쪽에서6165cm 이후까지 유지됐지만 화면/입력 실패가 있어 G1 Fail이다.

두 번째 실제 결과(`ceac593`): [실행 명령](second-pair.json), [실패 요약](second-pair-summary.json), [가로 화면 관찰](second-landscape-clipped.png). 세로·태블릿6종은 전체 셀·모서리·터치·표시 크기 검사를 통과했고 양쪽 경로2회전·호스트 표시 시각도 통과했다. 가로 화면만 양쪽 각각107건 실패해 G1 Fail을 유지했다. CameraStackTypes.cpp의 unconstrained 경로가 SceneView.cpp의 UpdateOrthoPlanes를 호출하며, half OrthoWidth2426.67이 카메라 Z2400보다 커 높이 보정2400+near1이 적용되는 것을 대조했다. 단순 bAutoCalculateOrthoPlanes=false만으로 이 경로를 막을 수 없었다.

세 번째 실제 결과(`efa2aa9`): [실행 명령](third-pair.json), [양쪽1338Pass 요약](third-pair-summary.json). root가14개 PNG를 모두 열어 두6×3보드·자기 보드 아래·한글 레이블·왼쪽 생성·중앙 오른쪽 화살표·전장 가시성을 확인했다. 숫자 검사는 모두 통과했지만 [가로 경계선](third-landscape-thin-grid.png)은 일부가 사라져 별도 표시 수정 R-G1-08을 진행했다. 이 관찰은 자동 입력 검사와 시각 검토가 각각 필요한 이유다.

구현자와 구분된 gate_review는 `5a46b30`을 CameraComponent.GetCameraView→LocalPlayer.GetProjectionData→CameraStackTypes→SceneView.UpdateOrthoPlanes까지 대조했다. false가 문제 보정을 직접 차단하며 새로운 권한·수명·의존 관계 결함은 없음을 확인했다. 최초 새 자동화 fixture의 미활성 카메라 문제는 root 실행에서 검출하고 수정 후 재실행했다.

빌드 실패: B UMG의 지역 Slot이 UWidget::Slot을 가림(C4458) → CanvasSlot. 통합 fixture의 bInitialized 이름 숨김·Min int32/size_t 불일치 → 지역 이름·명시적 정수 타입 수정. [통합 컴파일 실패 구간](integration-build-errors.log). A 경로5·B 보드3·G0포함통합20 계산 검사는 최초 Pass였으나 실제 화면 실패를 대체하지 않는다.

한계: same-world Controller 교체 시 로컬 표시 subsystem의 재탐색은 아직 없다. 현재 매치는 새 world를 사용하는 범위이며 G3 재경기/종료에서 확인한다. 합성 Engine touch는 OS 마우스·실제 Android 손가락 입력과 다르다. 화면 PNG의 시각 검토는 숫자 검사와 별도로 수행한다. G1의 적2개 성능은 P0 대표 부하 성능이 아니다.
