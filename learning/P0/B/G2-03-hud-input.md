# 개인 HUD와 뭉치 조작 — P0 / B / G2-03

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UI-01·TASK-INPUT-01, [UI v2](../../../docs/design/BOARD_UI.md#ingame-ui-v2), [layout-spec](../../../docs/design/assets/ingame-ui-v2/layout-spec.json) |
| 참고 자료 제작 상태 | Draft — 코드·설정 기록, 실제 화면/입력 및 수업 조립 재현 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | G1 `4861b987f3e2fe78bcc159d1b6a85008543a938b` + 앞 두 수업 / UI 초안 `22f31513f0e0b836200f2b161847e15e37410075`, 수정 최신 `f00f8fb27c514d9a44fddecaf624f7b378416662` |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A 확정 UnitActor의 배치·RangeCm·슬롯 표시, B 원자 서비스/개인 Snapshot, 공통 실패 SoundWave |
| 제공 코드 / 직접 작성할 코드 | 제공: G1 map/camera/재료·A UnitActor·root 실패음. 직접 작성: LDGameplayWidget, Controller intent/drag/pending, 사거리 표시 |

## 이번에 만들 동작

소환 버튼은 현재 가격과 처리 대기를 표시한다. 칸을 선택하면 유닛 이름·수량·사거리를 읽고 동일 칸 세 개체 합성 또는 한 개체 판매를 요청한다. 뭉치는 누른 칸에서 다른 자기 칸까지 끌어 이동한다. 상대 보드·실패 명령에는 한국어 이유를 표시한다. UI가 제거됐다가 재생성되어도 클릭 한 번은 요청 한 번이다.

공통 규칙은 [COMMON](../COMMON.md), 원자 명령은 앞 두 수업을 따른다. HUD는 서버 원본을 수정하지 않고 개인 envelope와 A Actor의 읽기 값을 표시한다. 신화·룰렛·강화·초월 버튼은 추가하지 않는다.

## 코드 작성 순서

1. `Core/LDPlayerController.*`: `RequestSummon/RequestMergeSelection/RequestSellSelection/RequestMove`가 UI/키 입력을 기존 `SubmitLocalCommand`로 전달한다. UI에서 비용·추첨 결과·참가자 신원을 생성하지 않는다. 요청은 현재 BoardRevision과 실제 ID를 포함한다.
2. 같은 셀에서 가장 작은 InstanceId를 선택 대표로 사용한다. Merge는 그 셀의 정확한 세 ID를 정렬해 전달한다. 최종 소유권·같은 종류·잠금 검증은 서버가 수행한다.
3. 마우스 왼쪽 또는 Touch1 누름은 기존 `InputScreenPosition`을 사용한다. 12px 이상 끌어 놓으면 같은 경로로 목적 셀을 조회해 Move를 요청한다. Touch 이후 .15초의 합성 mouse는 소비하지 않고 Touch2도 조작하지 않는다.
4. `UI/LDGameplayWidget.*`: 네이티브 WidgetTree에 SafeZone→Canvas→선택/재화/피드백 TextBlock과 소환/합성/판매 Button을 만든다. `NativeConstruct`는 AddUniqueDynamic, `NativeDestruct`는 자기 OnClicked만 RemoveAll한다. 서비스 delegate나 반복 타이머를 위젯에 만들지 않는다.
5. Canvas와 root는 SelfHitTestInvisible, 텍스트는 HitTestInvisible, 버튼은 PreciseTap이다. 버튼의 실제 CachedGeometry를 pixel rect로 읽어 `GetActionScreenRect`와 `IsOverAction`에 공유한다. 같은 영역 입력을 보드로 전파하지 않는다.
6. 개인 envelope가 준비되고 pending이 없을 때만 새 명령 버튼을 연다. 합성은 수량3/전설 아님, 판매는 선택 ID 있음 조건을 추가한다. 부족한 골드의 소환은 서버 거절을 받아 버튼 붉은 반응 .25초와 실패 사운드/이유를 표시한다.
7. `Board/LDBoardPresentation.*`: 선택된 A Actor의 실제 RangeCm와 canonical 중심을 받아 로컬 표시 좌표로 사거리 원을 그린다. 48개 선분은 NoCollision, 최소 표시선1.5px이며 캐시가 바뀔 때만 갱신한다. 전투 판정은 이 선분을 사용하지 않는다.
8. G1 안내 위젯의 아래 중복 문구만 숨긴다. HUD RemoveFromParent 뒤에는 Controller가 IsInViewport를 확인해 새 위젯을 만든다. EndPlay/세대 변경은 drag·선택·range 참조와 UI를 정리한다.

ARCH-01~03: 위젯은 조회/intent, Controller는 입력과 소유 RPC, Presentation은 표시를 맡는다. ARCH-04/05: 서버 확정 전 값을 낙관적으로 원본에 쓰지 않고 delegate를 생성/제거 수명에 맞춘다. ARCH-06: 실제 Slate 입력이 GetActionScreenRect를 통해 같은 버튼 경로를 사용하게 검증한다.

## Unreal 설정 순서

에디터에서 별도 WBP를 그려 네이티브 UI와 중복 표시하지 않는다. 설정 위치는 아래 네이티브 클래스이며 Blueprint 디자이너를 사용한다면 부모·계층·값을 똑같이 옮기되 검증 없이 대체하지 않는다.

| 순서 | 위치·에셋 | 프로퍼티·연결과 값 | 기대 화면 |
|---|---|---|---|
| 1 | 기존 G1 맵의 LDPlayerController | GameAndUI, 마우스 표시, capture 시 커서 숨김 false | 보드와 UMG 둘 다 입력 |
| 2 | LDGameplayWidget.RebuildWidget | SafeZone→Canvas, AddToViewport ZOrder20 | notch/SafeArea 안에 전체 HUD |
| 3 | 선택·합성·판매 | 기준1080×2340: 선택(144,1610,744,90), 합성(304,1718,268,54), 판매(588,1718,268,54) | 이름×수량/사거리, 두 조작 버튼 |
| 4 | 재화·소환·피드백 | (240,1840,562,66), (336,1940,408,220), (90,2180,900,95) | 골드·행운석·인구/20, 현재 가격 |
| 5 | NativeTick 레이아웃 | Scale=min(안전영역W/1080,H/2340), 가로 중앙 정렬 | 종횡비별 동일 논리 버튼 배치 |
| 6 | LDPlayerController.RejectedSound | `/Game/LD/Audio/S_P0Rejected.S_P0Rejected` | 서버 거절 시 로컬 재생 |
| 7 | root 실패음 제공 | SoundWave, 48kHz mono,220Hz,100ms; 생성기 `tools/Create-P0FeedbackAudio.py` | import/load는 root 확인, 실제 청취 별도 |
| 8 | PC 키 | S 소환, M 합성, X 판매 | 버튼과 같은 intent/API |

예상 첫 화면은 인구0/20·골드100·돌0·소환20, 성공 후 인구1/20·골드80·소환22다. 같은 종류3개는 A 슬롯 표시로 세 개체가 구분되고 선택 원의 반지름은 실제 데이터 사거리다. 완성 화면 캡처는 실제 GPU 검수 후 연결한다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과·수정 |
|---|---|---|
| 첫 UI Editor 빌드 | C++/UHT 통과 | Fail: LDG1BoardWidget.cpp 지역 Visibility가 UWidget 멤버 숨김(C4458). [실패 발췌](evidence/G2-initial/ui-build-failure.txt), `0d6c0160c87a54e0fd11594ee06c3dcc6ec09e7b`에서 OverlayVisibility로 변경, 재빌드 대기 |
| 3개 파일 수정 후 소스 검사 | 규약·포맷 오류0 | Pass:48checked/0errors, 게임 실행 증거 아님 |
| 실제 Slate 버튼 click/touch | 해당 요청1회, 보드 선택 불변 | NotRun, root 검수 fixture 준비 |
| UI RemoveFromParent 후 재생성 | 버튼 구독1개·현재 값 유지 | 누락 정적 발견: 포인터만 확인하면 재생성 안 됨. `64acff687b7b31ca39abb69bc99819016cacc75c` IsInViewport 확인 추가, 실행 대기 |
| 실패 소환 | 재화/RNG 불변, 빨간 .25초·이유·실패음 | 서버 불변은 G2-01 검사 Pass, 실제 UI/소리는 NotRun |
| 개인 상태보다 응답 선도착 | 두 Revision 도착 전 조작 대기 | 구현 완료, 실제 네트워크 도착순서 검수 대기 |
| 화면 회전·다른 비율 | 보드/버튼 좌표·range 일치 | G1 좌표 Pass와 구별하여 G2 UI는 NotRun |

사운드 원본은 root `5536803`에서 생성/import/load했다는 제공 기록이며 이 역할 브랜치가 직접 새 에셋을 만든 것이 아니다. 소리 출력·Android 터치·SafeArea는 실제 기기/출력 검수 전까지 미검증이다.

후속 통합 `cb6c631`은 실제 Editor Pass와 전체 UE 자동화34Pass/0Fail/0NotRun을 확인했다. [선별 결과](evidence/G2-initial/commands-final-review-summary.json)에 버전·범위를 남겼다. 위 C4458 수정의 재빌드는 통과했으며 실제 HUD 생성/재생성·Slate 클릭·캡처는 root의 별도 GPU 실행에서 검수 중이다. 자동화 통과로 화면 결과를 앞당겨 표시하지 않는다.

## 상대에게 전달하고 통합하기

G2-01/02 서비스와 A 실제 UnitActor를 연결한 뒤 UI 소스22f→0d6→64ac→f00 수정들을 통합한다. root는 실패음과 실제 `GetActionScreenRect` 기반 Slate click/touch를 제공한다. 읽기 API는 `GetLastResult`, 개인 Snapshot, `GetSelectedCellId`, `GetSelectedInstanceId`, `GetActionScreenRect`다. 검증 도구가 성공 버튼 delegate를 직접 호출해 실제 입력 통과로 표시하지 않는다. `learning` 폴더는 실행 자산·설정에서 참조하지 않는다.

## 이해 확인

- 위젯에서 소환 비용을 먼저 빼면 응답 재전달·거절에 어떤 문제가 생기는가?
- 같은 Touch가 mouse로도 전달될 때 RequestId 중복 방지와 입력 중복 소비 방지는 왜 둘 다 필요한가?
- 작은 변형: HUD를 제거하고 다음 프레임에 재생성한 뒤 클릭1회가 n을1만 증가시키는지 확인한다. 서버 상태/수업 출발 브랜치를 초기화하지 않는다.
- 다음 조건: 실제 두 프로세스 UI·사거리·지연/중복·반복 UI 검수와 A 전투 결합 Pass 후 수업 조립을 재현한다. Android는 G4의 별도 실기기 검수다.

## 단계 완료

- [x] 파일 순서·UMG 계층·값·입력/API·실패 원인을 기록했다.
- [x] 참고 코드와 제공 에셋·픽스처 범위를 구분했다.
- [ ] 수정 후 Editor·실제 GPU 화면·Slate/EngineTouch 입력을 통과했다.
- [ ] 수업 시작점의 별도 조립 재현을 마쳤다.
- [x] 미검증을 이전 G1/서버 계산 통과와 구분했다.
