# 두 경로와 양쪽 입력 연결 — P0 / 통합 / G1

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-MAP-01/NET-01/UI-01/TEST-01, [재기획 게이트](../../docs/design/P0_REPLAN.md) |
| 참고 자료 제작 상태 | Verified — 아래 시작점 재현과 실제 양쪽 화면 범위 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | G0 `649c1dedd6832c41089a76b59bc76518cd262296` / 수정 소스 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`(재현 완료) |
| 실제 개발 시작/완료 SHA | 공통 출발점 / 미생성 |
| 필요한 상대 산출물·버전 | A RouteModel/EnemyActor, B BoardGeometry/Controller/ViewTransform/Widget, 공통 Schema2/Rules0.3.0 |
| 제공 코드 / 직접 작성할 코드 | 제공: G0 맵·단색 재질·JSON·실행 로그 도구. 직접 작성: 역할 수업 소스와 아래 표시 연결. Probe는 명시적 검증 픽스처이며 전투 구현이 아님 |

## 이번에 만들 동작

서버가 동일한 적 개체2개를 각각3080cm 경로로 두 바퀴 이동시킨다. 두 참가자는 자기 보드를 아래에 보고, 양쪽 모두 왼쪽에서 생성된 적이 중앙을 왼쪽→오른쪽으로 지나 자기 경로로 돌아가는 것을 본다. 소유 보드18칸을 선택하고 상대18칸을 거절한다. 전체 계약은 [COMMON](COMMON.md) 원본을 따른다.

## 코드 작성 순서

1. A [경로 모델](A/G1_01_ROUTE_MODEL.md) → [EnemyActor](A/G1_02_ENEMY_ACTOR.md), B [좌표](B/G1-01-geometry.md) → [카메라·입력](B/G1-02-view-input.md)를 각 역할에서 작성·빌드한다.
2. A의 `bce4b7b`와 B의 `82cf174`를 G0 통합에 병합한다. `Build.cs`의 SlateCore 의존성을 추가한다. B 빌드 수정 `d5274dc`를 반영한다.
3. `Core/LDLocalPresentationSubsystem.*`: local Controller 준비 이벤트와 world actor 생성 이벤트를 연결한다. 준비 전 타이머는0.1초마다 local Controller를 찾고, 준비되면 해제한다. 이미 생성된 EnemyActor와 이후 생성된 Actor에 로컬 index를 전달한다. Deinitialize에서 delegate·timer를 해제한다.
4. `Battle/LDEnemyActor.cpp` 표시 위치·방향 계산을 `Board/LDViewTransform.h`로 합친다. 서버 위치/RouteIndex는 바꾸지 않는다. A 역할의 임시 identity/Y반사 구현을 통합에서 제거한다.
5. `Verification/LDG1ProbeSubsystem.*`: Development의 `-P0Probe=G1` 때만 생성한다. 서버에서1001/1002 ID, speed150,20Hz 단계; 클라이언트에서 실제 투영·36셀·셀 내부 모서리·Engine touch 바인딩·화면 PNG를 검사한다. 정상 제품 실행에서는 생성되지 않는다.
6. actual run이 발견한 A 표시시각 수정 `bb1d100`·회귀 `4c36e60`, B 카메라 축 수정 `cf3f610`을 반영한다. 검사기의 화면 전환도 실제 resize·터치·저장 완료 뒤 진행하도록 고친다.
7. 가로 화면의 자동 높이 보정 수정 `5a46b30`, 실제 카메라를 활성화한 회귀 fixture `d7daeb6`, 축소 화면의 최소1.5px 셀 경계 `bac0077`을 적용한다. CellVisuals는 표시용 참조이며 XY 크기만 바꾸고 논리140cm·입력 영역·카메라는 유지한다.

흐름: 서버 GameMode 문맥 → 고정20Hz Actor snapshot 복제 → local 표시 연결 → Camera 투영 → mouse/touch Controller 공통 입력 → canonical 좌표 복원 → owner 검사 → 선택/UI 갱신. 논리 원본과 화면 복제본이 분리돼 있으므로 view Y반사가 서버의 공격/경로 좌표를 바꾸지 않는다.

## Unreal 설정 순서

| 순서 | 위치·에셋 | 프로퍼티·연결 값 | 기대 관찰 |
|---|---|---|---|
| 1 | Content/LD/Maps/L_P0 → World Settings | GameMode Override=BP_LDGameMode(native LDGameMode 부모), Pawn=None | Controller가 local BoardPresentation 생성 |
| 2 | Project Settings → Maps & Modes | EditorStartupMap/GameDefaultMap=L_P0, GameMode=BP_LDGameMode | 원래 TopDown 템플릿 대신 P0 열림 |
| 3 | Content/LD/Materials/M_P0Flat | Unlit, Color VectorParameter → Emissive | 엔진 Cube/Sphere의 색상 MID 변경 |
| 4 | LDBoardPresentation의 Camera C++ 기본값 | Orthographic, pitch-90/yaw90, axis override=true/MaintainXFOV, auto planes=false, use height as target=false, near1/far10000 | 세로·가로 모두 보드 전체 표시·정사각 셀 |
| 5 | LDG1BoardWidget(native UUserWidget) | WidgetTree SafeZone→Canvas→레이블. SafeZone 실제 pixel rect를 Controller에 전달 | DPI·SafeArea와 입력/카메라가 같은 rect 사용 |
| 6 | 생성 도구(필요한 신규 에셋만) | `UnrealEditor-Cmd.exe <uproject> -unattended -nullrhi -ExecutePythonScript=<root>/tools/Create-P0Assets.py` | 저장된 에셋 생성 결과. 화면 검수와 별도 |

## 실행·실패·수정 기록

```powershell
pwsh -File tools/Build-P0Editor.ps1 -RunId G1-replay-editor
pwsh -File tools/Test-P0Automation.ps1 -Filter LD.P0 -RunId G1-replay-contracts
pwsh -File tools/Run-P0Pair.ps1 -RunId G1-replay-pair -RenderOffscreen
```

새 RunId를 쓴다. 실행기는 별도 UnrealEditor `-game` 두 프로세스이며 PIE 또는 PC 패키지가 아니다. RenderOffscreen은 실제 GPU 렌더링이며 NullRHI가 아니다. 물리 모니터 크기로 큰 세로 창이 잘리는 한계를 피하고 각 해상도의 실제 viewport를 검사한다. Engine InputTouch는 합성 이벤트이고 OS/실기기 입력은 후속이다.

| 입력/조건 | 기대 결과 | 실제 최초 결과·수정 | 증거 |
|---|---|---|---|
| 두 바퀴 | 같은 ID/RouteIndex와 Actor,6160cm 이상 | 양쪽6165cm 이후 확인 | [첫 실행 요약](../../docs/production/evidence/RUN-20260918-G1/first-pair-summary.json) |
| 세로540×1170 | 셀60px·전장 전체 | Fail:130px/양쪽 잘림 → FOV축 수정 | [실제 실패 화면](../../docs/production/evidence/RUN-20260918-G1/first-host-clipped.png) |
| world100.125, step100.10 | root15cm, mesh18.75cm | Fail:mesh15cm → view clock으로 수정 | HostStepKeepsCurrentViewClock 회귀 |
| resize 후 입력·스크린샷 | 실제 크기 확인·터치 종료 뒤 다음 화면 | 최초 검사기가 프레임 지연에 단계 건너뜀 → 완료 조건 기반 전환 | [리뷰](../../docs/production/evidence/RUN-20260918-G1/REVIEW.md) |
| 가로1280×720 | 셀36.923px·Z0 입력 평면이 카메라 앞 | 두 번째 실행에서 각107건 실패 → 자동 높이 보정 해제 | [가로 실패](../../docs/production/evidence/RUN-20260918-G1/second-landscape-clipped.png) |
| 카메라 회귀 fixture | production CalcCamera가 직교 view 반환 | 미활성 Camera로 perspective fallback → InitializeActorsForPlay·Activate·전제 검사 후1Pass | Saved/P0Runs/G1-engine-projection-fix1 |
| 수정본 양쪽7화면비 | 36셀·72내부모서리·EngineTouch·2바퀴 | 각1338검사 Pass. PNG14장 별도 확인 시 가로 경계선 일부 소실 → 최소1.5px 표시 보완 | [세 번째 실행](../../docs/production/evidence/RUN-20260918-G1/third-pair-summary.json) |

검수 화면비는7개(9:19.5 두 해상도,9:16,9:20,3:4,5:8,가로16:9)다. 각 `Saved/P0Runs/<RunId>/{host,client}/result.json`, `view-0.png`~`view-6.png`, engine.log와 pair.json을 함께 읽는다. JSON Pass만으로 화면 품질이나 실제 Android 터치를 Verified로 올리지 않는다.

## 시작점 재현

[G1 재현 절차](evidence/G1_REPLAY/README.md)에 따라 새 detached worktree를 만든다. `Replay-P0G1.ps1`은 G0에서 위 수업 순서대로 제공 파일과 참고 작성 파일36개를 조립하고 각 Git blob을 대조한다. 실제 학습자가 작성했다는 뜻은 아니며 learn 브랜치를 변경하지 않는다. SourceSha는 위 전체40자리 값을 사용한다. 재현 worktree에서 Editor→전체 LD.P0 자동화→두 프로세스 실행을 직렬 수행하고, 결과의 HEAD가 G0인 이유를 assembly.json의 SourceSha와 함께 기록한다. 기존 파일/브랜치를 덮지 않고 새 절대 경로와 RunId를 쓴다.

최종 재현은 Editor Pass·자동화22Pass·실제 양쪽1338/1336Pass다. [공통 재현 결과](evidence/G1_REPLAY/SUMMARY.md)에 시작/완료 SHA·조립 blob·실행·화면·범위가 연결돼 있다.

## 상대에게 전달하고 통합하기

위 소스 SHA·입력 이벤트 경계·읽기 전용 snapshot·맵/재질을 전달한다. 양쪽 캡처를 열어 자기 보드 아래/생성 왼쪽/중앙 동일방향/안전영역/텍스트를 확인하고 단일 통합 SHA를 역할 브랜치에 반영한다. G1 실제 통과 전 G2 전투를 추가하지 않는다.

## 이해 확인

- 같은 OrthoWidth인데 세로 화면에서 확대된 이유를 엔진 축 선택으로 설명할 수 있는가?
- Actor tick 후20Hz 논리 단계가 표시를 다시 감으면 어떤 현상이 생기는가?
- 작은 변형: 검수 픽스처 speed만75로 바꾸고 두 바퀴 도달 시간이 두 배가 되는지 관찰한다. 제품 데이터에는 반영하지 않는다.

## 단계 완료

- [x] 역할 API·통합 작성 순서·설정 위치·실패 원인을 기록했다.
- [x] 수정 후 실제 화면/전체 입력/두 바퀴를 통과했다.
- [x] 이 시작점과 절차에서 수업 재현을 확인했다.
- [x] PIE·패키지·물리 입력·P0 대표 부하 미검증을 구분했다.
