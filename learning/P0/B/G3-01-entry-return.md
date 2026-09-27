# P0 진입과 반복 매치 복귀 — G3 / B / 01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UI-01, TASK-TEST-01; [B 구현 8~9절](../../../docs/technical/IMPLEMENTATION_B.md), [G3 재기획](../../../docs/design/P0_REPLAN.md) |
| 참고 자료 제작 상태 | Draft — 코드 제공, 새 조립·실제 여행 재현 전 |
| 실제 개발 상태 | Planned — 학습자가 작성한 것으로 기록하지 않음 |
| 참고 시작/완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / 제품 초안 `d2183ae8c809541da4602b99964f90fd22b50b93` |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A `L_P0` 매치와 결과 계약; 통합 담당의 새 `L_P0Entry` 맵·Config·쿠킹 설정 |
| 제공 코드 / 직접 작성할 코드 | G2 기준·빌드/패키지 도구·G3 검사 코드는 제공. 아래 Entry 8파일을 직접 작성. A 전투 코드는 상대 제공 |

공통 규칙과 검증 구분은 [P0 공통](../COMMON.md), 진행 상태 구분은 [학습 운영](../../WORKFLOW.md)에 둔다. 이 수업은 P0의 터치 진입·결과 복귀만 제공하며 정식 로비나 매칭 시스템을 만들지 않는다.

## 이번에 만들 동작

프로세스를 켜면 로컬 시작 화면이 나온다. 호스트 시작은 `L_P0?listen`으로 이동하고, 다른 프로세스는 숫자 IPv4 주소를 입력해 참가한다. 시작 화면에 머무는 동안 매치 서비스나 참가 대기 시계가 돌지 않는다. 결과의 복귀 버튼 또는 연결 실패는 같은 프로세스 안에서 시작 화면으로 돌아간다.

매치 상태를 보관하지 않는 `ULDGameInstance`는 여행 중복 방지와 로컬 오류 문구만 소유한다. 매치마다 새 GameMode/Controller가 생성되므로 이전 명령·보드·타이머를 재사용하지 않는다. 전역 엔진 실패 delegate는 자기 World/자기 pending 연결인지 먼저 확인해 다중 PIE의 다른 참가자에게 영향을 주지 않는다.

## 코드 작성 순서

1. `Source/Mobile_defense_clone/Core/LDGameInstance.h/.cpp`: `NormalizeJoinAddress` → `TryBeginMatchTravel` → `RequestEntryReturn` → 엔진 실패 구독·해제 순서. 주소는 IPv4 네 octet와 선택 포트만 받고 `?listen`, 경로, 공백 명령, 세미콜론을 거절한다. `RequestEntryReturn`은 1회만 예약하고 다음 Core ticker에서 `OpenLevel`한다. 엔진 `Browse/TickWorldTravel` 실패 콜백 안에서 여행을 재진입시키지 않는다.
2. `Core/LDEntryGameMode.h/.cpp`: `AGameModeBase` 부모, Entry Controller, Pawn 없음. 전투 GameMode를 상속하면 시작 화면도 30초 대기 종료가 될 수 있어 독립시킨다.
3. `Core/LDEntryPlayerController.h/.cpp`: BeginPlay에서 `NotifyEntryReady`, UIOnly 입력, 위젯 생성. `RequestHost/RequestJoin`은 동일 GI 여행 gate를 거친다. EndPlay에서 위젯 제거, UI를 외부에서 제거한 경우 tick에서 현재 상태로 한 번 재생성한다.
4. `UI/LDEntryWidget.h/.cpp`: Border→SafeZone→Canvas 아래 제목·안내·Host·주소·Join·오류 텍스트를 구성한다. 버튼 `AddUniqueDynamic`/`RemoveAll`로 수명을 맞춘다. 주소 입력은 `UEditableTextBox`, 가상 키보드 `Default/OnAllFocusEvents`다. 실제 Android 키보드 표시는 G4에서 검수한다.
5. 제공 `Tests/LDEntryAndHudTests.cpp`의 `LD.P0.G3.Entry.*`는 문자열 경계와 여행 의도/복귀 gate만 확인한다. 이 테스트는 실제 맵 로드나 네트워크 접속 검수를 대체하지 않는다.

호출 흐름은 `UMG OnClicked → EntryController → GI TryBeginMatchTravel → OpenLevel/ClientTravel`이다. 실패 시 `Engine delegate → 소유 World 검사 → GI RequestEntryReturn → 다음 Core tick → Entry BeginPlay/NotifyEntryReady`로 연결된다. `Shutdown`과 Entry 준비는 남은 ticker를 제거한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | Project Settings → Maps & Modes | Game Instance Class=`/Script/Mobile_defense_clone.LDGameInstance` | 맵 여행 사이 로컬 실패 사유 유지 |
| 2 | 새 맵 `/Game/LD/Maps/L_P0Entry` | World Settings → GameMode Override=`LDEntryGameMode`; Pawn/전투 Actor 없음 | 독립 로컬 진입 화면 |
| 3 | Maps & Modes | Game Default Map/Editor Startup Map=`L_P0Entry`; 기존 `L_P0`는 `BP_LDGameMode` 유지 | 시작 화면과 실제 listen 매치 분리 |
| 4 | Packaging → Maps to cook | `L_P0Entry`, `L_P0` 둘 다 포함 | 결과 복귀 후 맵 누락 방지 |
| 5 | native `ULDEntryWidget` | 기준1080×1600; Host=(140,600,800,150), 주소=(140,890,800,130), Join=(140,1070,800,150) | SafeZone 안에서 가로/세로 최소 배율과 중앙 정렬 |

Blueprint/UMG 추가 에셋은 필요 없다. UI 구성은 C++ `RebuildWidget`에 있다. 통합 담당만 맵·Config·빌드/에디터를 편집한다. `SetJoinAddressText`는 검사용으로 편집 상자 텍스트만 바꾸며 참가 검증이나 여행 gate를 우회하지 않는다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| Entry에서60초 대기 | LoadingTimeout 없음, Host/Join 유지 | 미실행 | 실제 패키지 필요 |
| Host 클릭→다른 프로세스 주소 입력/Join | 두 참가자 준비·개인 보드 표시 | 미실행 | 실제 별도 패키지 두 개 필요 |
| 잘못된 주소·포트0·65536·`?listen` | 여행 없음, 입력 안내 | 미실행 | `LD.P0.G3.Entry.NumericAddressBoundary` + UI |
| 두 번 빠른 Host/Join | 여행 의도1개 | 미실행 | `TravelIntentAndReturnLifetime` + 실제 클릭 |
| 호스트 결과 복귀·peer 연결 종료 | peer도 Entry와 오류 문구, 이전 타이머 정리 | 미실행 | 실제 네트워크 여행 |
| 같은 두 프로세스에서3회 이상 재매치 | 각 매치 새 MatchId·초기 재화·RequestId, 위젯/구독 누적 없음 | 미실행 | G3 반복 플레이 probe |

현재 `git diff --check`만 통과했다. 컴파일·자동화·PIE·패키지·Android 결과는 아직 없다. 엔진 API 사전 확인에서 EditableTextBox는 `GetFont/SetFont`가 없어 `GetWidgetStyle/SetWidgetStyle`의 `TextStyle.Font`로 작성했다. 이것은 실행 실패를 수정한 기록이 아니라 로컬 UE5.8 헤더 확인 결과다.

## 상대에게 전달하고 통합하기

- API: `ALDEntryPlayerController.GetEntryActionScreenRect(true/false, Rect)`, `SetJoinAddressText`, `RequestHost`, `RequestJoin`; `ULDGameInstance.RequestEntryReturn`, `NotifyEntryReady`.
- 코드 `d2183ae`; 계약 선행 A `e61c414`, Processor 경계 `8405a93`. 통합은 계약→B Source→A 위젯 cpp→Config/맵→Editor→Entry/2인/복귀→패키지 반복 검수 순서다.
- 필수 에셋 `L_P0Entry`는 통합 담당 생성 예정이다. 현재 수업만 복사하면 맵 여행은 실행할 수 없다.

## 이해 확인

1. Entry GameMode가 전투 GameMode를 상속하지 않는 이유를 수명과 30초 대기 조건으로 설명한다.
2. 여행 중 버튼이 비활성화돼도 GI에 중복 gate가 필요한 이유는 무엇인가?
3. 작은 변형: 오류 문구를 바꾸되 서버 ResultReason이나 경제 스냅샷을 바꾸지 않고 UI만 갱신한다. 잘못된 주소가 기존 정상 주소를 재사용하지 않는 검사를 유지한다.
4. 호스트가 복귀할 때 peer가 겪는 네트워크 실패와 사용자가 누른 복귀를 실제 로그에서 구분한다.

## 단계 완료

- [x] 파일 순서·설정값·상태 소유권과 제공 코드를 기록했다.
- [ ] 맵/Config를 포함해 새 조립에서 재현했다.
- [ ] 실제 두 프로세스3회 재매치와 UI 재생성을 확인했다.
- [ ] Android 가상 키보드·SafeArea·물리 터치를 확인했다.

다음 단계 진입은 A 결과 위젯 연결과 실제 Entry/복귀 검수다. 통합 실행 증거와 완료 SHA를 추가하기 전에는 Verified로 올리지 않는다.
