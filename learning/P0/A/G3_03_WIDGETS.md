# 공용 전투 정보와 결과 표시 — P0 / A / G3-A-03

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UI-01 중 A / [A-06](../../../docs/technical/IMPLEMENTATION_A.md#a06), [UI v2](../../../docs/design/BOARD_UI.md#ingame-ui-v2) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작 / 현재 A Source SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / `0d358bc5af920166bc517431848700d8c9c6a98f` — native 위젯 최초 구현 `61fb3a7`, 수업 조립 재현 완료 아님 |
| 실제 개발 시작 / 완료 SHA | 자기 G2 통합 결과 / 미생성 |
| 상대 산출물 | B Controller의 GameState 구독·Widget 생성/갱신/해제·시작 화면 복귀 |
| 제공 / 직접 작성 | 제공: G2 UI 입력과 SafeArea 계약. 직접 작성: UI/LDBattleStatusWidget.h/.cpp, UI/LDResultWidget.h/.cpp |

## 이번에 만들 동작

상단에 실제 웨이브·서버 마감 기준 남은 시간·일반 적 N/100·두 보스 HP를 표시한다. 결과 중앙 패널은 승리/패배/종료, 사유, 도달 웨이브와 시작 화면 복귀 의도를 제공한다. actor 포인터나 경제 변경 API를 위젯에 전달하지 않는다.

## 코드 작성 순서

1. `LDBattleStatusWidget.h`에 `UpdateView(const FLDBattleSnapshot&, double ServerNow)`만 표시 입력으로 선언한다. 매치·보스 actor 탐색, 서버 결과 판정, 별도 타이머를 넣지 않는다.
2. `RebuildWidget`은 SafeZone→Canvas→Status/BossStatus 순으로 native UMG를 만든다. 글자는 HitTestInvisible이며 뒤 보드 입력을 가로채지 않는다. Reference v2 상단228~518 범위를 사용한다.
3. `UpdateView`는 Phase에 맞는 Loading/Preparing/일반/보스 deadline을 고르고 `ceil(max(0,deadline-ServerNow))`로 남은 정수초를 표시한다. 0초 표시는 판정 근거가 아니다. N≥90은 숫자를 경고색으로 표시하되 패배는 서버 snapshot만 따른다.
4. `NativeTick`은 Canvas의 실제 SafeArea 크기에서 공통 배율과 가운데 오프셋을 구해 위치/글자 크기만 갱신한다. B가 호출하는 `UpdateView`가 서버 동기 시각을 전달한다.
5. `LDResultWidget`은 SafeZone→Canvas→배경/ResultText/ReturnButton을 만든다. `UpdateView`는 snapshot의 Result/Reason만 한국어로 표시한다. 보스 actor가 아직 복제되지 않아도 snapshot으로 표시 가능하다.
6. `NativeConstruct`는 버튼 OnClicked를 AddUniqueDynamic 한 번 연결한다. `HandleReturn`은 `OnReturnRequested` multicast 의도만 내보낸다. 실제 Travel/접속 정리는 B가 맡는다.
7. `NativeDestruct`는 버튼 구독과 OnReturnRequested를 해제한다. `GetReturnButtonScreenRect`는 실제 cached geometry를 viewport pixel로 바꿔 검수 입력 위치를 제공한다. 이미지에 보이는 사각형을 임의 추측하지 않는다.

구조 이유: 전투 규칙과 복귀 경로를 위젯에 넣으면 UI 제거가 게임 진행/세션 종료와 얽힌다. 값만 받는 표시 위젯은 늦은 생성·재생성 때 현재 상태를 다시 전달하면 되고, 서버에 영향을 주지 않는다.

## Unreal 설정 순서

| 순서 | 위치·에셋 | 부모/프로퍼티/연결과 값 | 예상 화면 |
|---|---|---|---|
| 1 | Content Browser | 추가 WBP 바이너리 없음. native ULDBattleStatusWidget/ULDResultWidget 사용 | 별도 BP 편집 없이 동일 위젯 |
| 2 | B Controller | CreateWidget(owning PC)→AddToViewport→즉시 UpdateView | 늦게 생성해도 현재 수치 표시 |
| 3 | Battle 위젯 | SafeZone→Canvas; Status 기준(240,228,600,232), BossStatus(180,476,720,42) | v2 상단 영역, 보드와 분리 |
| 4 | 글자 | 기본 엔진 폰트, Status34/Boss22 기준 배율, 최소10/8 | 한국어·숫자 읽힘은 실제 화면 검수 필요 |
| 5 | Result 위젯 | 배경 SafeArea x10~90%, y32~65%; 본문 x12~88%, y35~55%; 버튼 x27~73%, y57~63%, 높이최소40 | 중앙 결과와 복귀 버튼 |
| 6 | 이벤트 | Result.OnReturnRequested.AddUObject(B PC 복귀 핸들러) | 클릭은 복귀 의도1회 |
| 7 | 제거 | B RemoveAll/RemoveFromParent, widget NativeDestruct에서 내부구독 Clear | 중복 버튼/이전 세대 통지 없음 |

기술 명세의 WBP 이름은 사용할 수 있는 BP 구성안이었다. 현재 P0는 기존 native UMG 방식과 같은 계약을 사용한다. 바이너리 에셋/맵을 두 역할에서 동시에 저장할 필요를 줄이고 파일 작성 순서로 재현할 수 있다. 새로운 제품 기능 선택은 아니다.

## 실행·실패·수정 기록

| 조건 | 기대 결과 | 실제 결과·범위 |
|---|---|---|
| 첫 연결·준비·일반 웨이브 | 실제 Phase/남은초/Wave/N 표시, 가짜10웨이브 예시값 없음 | 실제 PIE v2에서 양쪽 wave1/N2, 소유 소환·Gold80 확인; 후속 수정 뒤 미재실행 |
| 보스 actor 미도착·snapshot 선도착 | 두 HP를 값으로 표시, actor 역참조0 | 코드 경로 검토, 실행 미검수 |
| 패배/승리/Aborted | 각각 사유·웨이브·복귀 표시 | B 통합 완료; 실제 PIE v2 Aborted 복제 확인, 최종 패키지 표시 검수 대기 |
| 보스 비치명타격 직후 Abort | 결과 최초 게시에서 실제 Actor HP5900/6000과 snapshot 일치 | A02 실제 Fail→Mode의 마지막 HP 갱신→NullRHI 회귀 Pass; 실제 화면은 후속 검수 |
| HUD3회 재생성 | 구독1회/클릭1의도, 현재 상태 즉시표시 | B 수명 자동화/패키지 검수 대기 |
| 좁은 화면/SafeArea | 상단·핵심 조작 잘림 없음, Return button 실측rect와 입력 일치 | 패키지/Android 미검수 |

실제 캡처·컴파일·수명 결과는 [G3 A 공통 증거](G3_EVIDENCE.md)에 연결한다. PIE v1은 같은 프레임의 host WAVE0/00:00 잔상이 있어 표시 검수를 보류했고, 양쪽 Running 뒤0.5초 대기한 v2는 실제2개 PIE World·양쪽 PNG·원설정 복원까지 Pass했다. 요청540×1170의 실제 창은 데스크톱 제약으로546×720이다. 이 한 크기의 관찰로 모든 화면비/물리 SafeArea/최소 글자 가독성을 통과 처리하지 않는다. 이후 A01/A02/준비 분기 수정 뒤 PIE·최종 패키지·수업 조립 재현은 미실행이다.

## 상대에게 전달하고 통합하기

`e61c414` header계약과 `61fb3a7` cpp를 함께 전달한다. B의 `d2183ae` Controller 통합은 GameState 구독·서버 시각·두 widget 수명과 복귀를 맡는다. A는 화면 내부 및 값 형식 수정만 담당한다. 양쪽 실제 화면·버튼 위치 검수 후 공통 통합 수업에서 최종 SHA를 고정한다.

## 이해 확인

- UI 남은 시간이0이 되어도 클라이언트가 패배를 확정하면 안 되는 이유는 무엇인가?
- actor 복제보다 먼저 도착한 결과 snapshot을 값으로 표시하면 어떤 수명 오류를 피하는가?
- 작은 변형: 테스트 표시용으로 동일 snapshot을 두 번 전달하고 Return widget을 세 번 재생성하여 클릭 callback 수가 늘지 않는지 확인한다. 제품 결과/경제는 변경하지 않는다.

## 단계 완료

- [x] native UMG 계층·좌표·프로퍼티·연결/해제·B 전달 API를 기록했다.
- [ ] 실제 두 화면·화면비·PIE/패키지·늦은 생성·재생성을 검수했다.
- [ ] 출발점 파일 조립 재현 후 Verified를 표시했다.
- [ ] G3 독립 리뷰와 패키지 검수를 완료했다. Android 물리 SafeArea/터치는 G4 미검수다.
