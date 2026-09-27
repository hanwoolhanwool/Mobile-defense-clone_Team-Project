# P0 진입과 반복 매치 복귀 — G3 / B / 01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UI-01, TASK-TEST-01; [B 구현 8~9절](../../../docs/technical/IMPLEMENTATION_B.md), [G3 재기획](../../../docs/design/P0_REPLAN.md) |
| 참고 자료 제작 상태 | Draft — 새 조립·Editor·자동화·실제 패키지5판/4회 복귀 Pass, 필수 부하 진행 중 |
| 실제 개발 상태 | Planned — 학습자가 작성한 것으로 기록하지 않음 |
| 참고 시작/완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / 조립 `0981d07307112857dfdf0e91c79bcecdbcbc291b`; C++ `0e473f4`, cook Config `98727f0` ([공통 입력](README.md#g3-evidence), 게이트 완료 미확정) |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A `L_P0` 매치와 결과 계약; 통합 담당의 새 `L_P0Entry` 맵·Config·쿠킹 설정 |
| 제공 코드 / 직접 작성할 코드 | G2 기준·빌드/패키지 도구·G3 검사 코드는 제공. 아래 Entry 8파일을 직접 작성. A 전투 코드는 상대 제공 |

공통 규칙은 [P0 공통](../COMMON.md), 실행별 SHA·검증 범위·로그 위치는 [B 공통 검증 기록](README.md#g3-evidence), 진행 상태 구분은 [학습 운영](../../WORKFLOW.md)에 둔다. 이 수업은 P0의 터치 진입·결과 복귀만 제공하며 정식 로비나 매칭 시스템을 만들지 않는다.

## 이번에 만들 동작

프로세스를 켜면 로컬 시작 화면이 나온다. 호스트 시작은 `L_P0?listen`으로 이동하고, 다른 프로세스는 숫자 IPv4 주소를 입력해 참가한다. 시작 화면에 머무는 동안 매치 서비스나 참가 대기 시계가 돌지 않는다. 결과의 복귀 버튼 또는 연결 실패는 같은 프로세스 안에서 시작 화면으로 돌아간다.

매치 상태를 보관하지 않는 `ULDGameInstance`는 여행 중복 방지와 로컬 오류 문구만 소유한다. 매치마다 새 GameMode/Controller가 생성되므로 이전 명령·보드·타이머를 재사용하지 않는다. 전역 엔진 실패 delegate는 자기 World/자기 pending 연결인지 먼저 확인해 다중 PIE의 다른 참가자에게 영향을 주지 않는다.

## 코드 작성 순서

1. `Source/Mobile_defense_clone/Core/LDGameInstance.h/.cpp`: `NormalizeJoinAddress` → `TryBeginMatchTravel` → `RequestEntryReturn` → 엔진 실패 구독·해제 순서. 주소는 IPv4 네 octet와 선택 포트만 받고 `?listen`, 경로, 공백 명령, 세미콜론을 거절한다. `RequestEntryReturn`은 1회만 예약하고 다음 Core ticker에서 `OpenLevel`한다. 엔진 `Browse/TickWorldTravel` 실패 콜백 안에서 여행을 재진입시키지 않는다.
2. `Core/LDEntryGameMode.h/.cpp`: `AGameModeBase` 부모, Entry Controller, Pawn 없음. 전투 GameMode를 상속하면 시작 화면도 30초 대기 종료가 될 수 있어 독립시킨다.
3. `Core/LDEntryPlayerController.h/.cpp`: BeginPlay에서 `NotifyEntryReady`, UIOnly 입력, 위젯 생성. `RequestHost/RequestJoin`은 동일 GI 여행 gate를 거친다. EndPlay에서 위젯 제거, UI를 외부에서 제거한 경우 tick에서 현재 상태로 한 번 재생성한다.
4. `UI/LDEntryWidget.h/.cpp`: Border→SafeZone→Canvas 아래 제목·안내·Host·주소·Join·오류 텍스트를 구성한다. 버튼 `AddUniqueDynamic`/`RemoveAll`로 수명을 맞춘다. 주소 입력은 `UEditableTextBox`, 가상 키보드 `Default/OnAllFocusEvents`다. `UpdateAddressFontSize`에서는 입력창의 **UPROPERTY `WidgetStyle` 자체**를 참조해 크기가 바뀔 때만 `SetWidgetStyle`에 전달한다. 지역 복사본을 전달하면 아래 PKG01이 재발한다. 실제 Android 키보드 표시는 G4에서 검수한다.
5. 제공 `Tests/LDEntryAndHudTests.cpp`의 `LD.P0.G3.Entry.*`는 문자열 경계, 여행 의도/복귀 gate, 실제 Entry의 Slate 글꼴 수명을 확인한다. `OwnedAddressStyleSurvivesPrepass`는 크기33/19/42와 별도의 owned style 변경이 다음 prepass에 반영되는지, GC 뒤 원래 FontObject/CompositeFont가 유지되는지 검사한다. 이 테스트는 실제 패키지 렌더링·맵 로드·접속 검수를 대체하지 않는다.

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
| Host 클릭→다른 프로세스 주소 입력/Join | 두 참가자 준비·개인 보드 표시 | Pass: 첫 진입의 양쪽 실제 Slate 버튼·새 매치 문맥 확인 | 최종5시드 패키지; 주소 텍스트는 제공 probe가 설정하므로 물리 키보드 입력 증거는 아님 |
| 잘못된 주소·포트0·65536·`?listen` | 여행 없음, 입력 안내 | 주소 정규화·거절 Pass, 실제 UI 안내 검수 대기 | B 57자동화 중 `NumericAddressBoundary` |
| 두 번 빠른 Host/Join·중복 실패 통지 | 여행 의도/예약1개, Entry 준비 후 새 의도 허용 | GI gate·ticker 정리 Pass, 실제 클릭/여행 검수 대기 | B 57자동화 중 `TravelIntentAndReturnLifetime` |
| 첫 cooked Entry 렌더링 | 제목·Host·주소·Join 유지 | **Fail PKG01→Closed**: client 프레임2 접근 위반 수정 후 새 패키지 진입·반복 복귀 Pass | [원본 실패](../../../docs/production/evidence/RUN-20260918-G3/packaged-entry-crash.json)와 [최종5판](../../../docs/production/evidence/RUN-20260918-G3/package-five-seeds-summary.json)을 별도 보존 |
| 입력창 크기 변경→후속 prepass→GC | 원래 FontObject와 소유 스타일 유지 | 수정 후 Pass | B 57자동화 중 `OwnedAddressStyleSurvivesPrepass`; NullRHI |
| 호스트 결과 복귀·peer 연결 종료 | peer도 Entry와 오류 문구, 이전 타이머 정리 | 미실행 | 실제 네트워크 여행 |
| 같은 두 프로세스에서3회 이상 재매치 | 각 매치 새 MatchId·초기 재화·명령 문맥, 새 위젯 하나 | Pass: 양쪽4회 실제 결과 복귀 클릭·서로 다른5개 매치·각 판 게임 HUD3회 재생성 | 최종5시드. 후속 host 진입은 시드 지정 OpenLevel, client 재참가는 정상 요청 API를 사용하므로 모든 재진입 버튼의 실제 클릭 증거로 확대하지 않음 |

PKG01은 실제 패키지 실패다. 첫 패키지는 만들어졌지만 `SEditableText::SynchronizeTextStyle → FSlateFontInfo::GetCompositeFont → UObjectBaseUtility::GetInterfaceAddress`에서 client가 종료했다. 원인은 초기 `NativeTick`의 지역 `FEditableTextBoxStyle`이었다. 로컬 UE5.8 `UMG/Private/Components/EditableTextBox.cpp:392`의 setter는 프로퍼티에 복사한 뒤 **호출자가 전달한 `&InStyle`**을 Slate에 전달하고, `Slate/Private/Widgets/Input/SEditableTextBox.cpp:113`은 그 주소를 보관한다. 따라서 함수 반환 뒤 폰트·브러시를 읽는 주소가 유효하지 않았다. 기본 폰트가 패키지에 없다고 단정하거나 엔진/에셋을 바꾸지 않았다.

수정 `501be9035b02e172e356151abe0c1606304b11ce`는 이미 UObject가 소유한 스타일의 안정 주소를 사용한다. 통합/B 전체57자동화와 새 재현본의 기존56종+후속 Entry3종은 [공통 검증 기록](README.md#g3-evidence)에서 구별한다. 이어 실제 수정 패키지에서 같은 프로세스4회 결과 복귀와5개 새 매치를 확인해 PKG01을 닫았다. 중간 재실행은 필수 거절 효과음 누락(PKG02)으로 중단했으며, 통합 담당이 cook 설정을 보완한 최종 패키지에서 로드도 통과했다. PKG02의 원본 실패·수정/청취 한계는 공통 기록에 둔다. 이전 Fail을 덮어쓰거나 그 실행을 성공한5판에 합산하지 않는다.

## 상대에게 전달하고 통합하기

- API: `ALDEntryPlayerController.GetEntryActionScreenRect(true/false, Rect)`, `SetJoinAddressText`, `RequestHost`, `RequestJoin`; `ULDGameInstance.RequestEntryReturn`, `NotifyEntryReady`.
- 초안 `d2183ae`와 계약 선행 A `e61c414`, Processor 경계 `8405a93` 뒤 **PKG01 수정 `501be90`을 포함한 현재 `0e473f4`**를 사용한다. 이전 초안을 최종 코드로 복사하지 않는다. 통합은 계약→B Source→A 위젯 cpp→Config/맵→Editor→Entry/2인/복귀→패키지 반복 검수 순서다.
- 필수 `L_P0Entry`·`L_P0`와 GI/cook 설정은 통합 입력에 포함되어 있다. [공통 재현 절차](README.md#g3-evidence)의 최종 조립 `0981d073`과 Config `98727f0`를 함께 사용한다. Entry 스타일 수정→새 Editor/Entry3종→필수 효과음 cook 보완→새 패키지→5시드/회복 순서의 증거를 확인한다. 그 뒤 런타임 코드/데이터/에셋/설정 변경은 없고 읽기 분석 도구 두 개만 보완됐다.

## 이해 확인

1. Entry GameMode가 전투 GameMode를 상속하지 않는 이유를 수명과 30초 대기 조건으로 설명한다.
2. 여행 중 버튼이 비활성화돼도 GI에 중복 gate가 필요한 이유는 무엇인가?
3. 작은 변형: 오류 문구를 바꾸되 서버 ResultReason이나 경제 스냅샷을 바꾸지 않고 UI만 갱신한다. 잘못된 주소가 기존 정상 주소를 재사용하지 않는 검사를 유지한다.
4. 호스트가 복귀할 때 peer가 겪는 네트워크 실패와 사용자가 누른 복귀를 실제 로그에서 구분한다.
5. `SetWidgetStyle`의 매개변수가 `const&`인데도 왜 지역 복사본이 안전하지 않았는가? 저장 주소의 소유자와 실제 소비 시점을 설명하고, 글꼴 크기를 바꾸는 작은 변형 뒤 후속 prepass 검사를 반복한다.

## 단계 완료

- [x] 파일 순서·설정값·상태 소유권과 제공 코드를 기록했다.
- [x] 맵/Config를 포함한 기존 입력 조립·빌드를 확인하고 PKG01 수정 입력을 별도 기록했다.
- [x] 현재 소스의 주소·여행 gate·Slate 스타일 수명 자동화를 확인했다.
- [x] 수정 후 cooked Entry 렌더링·접속·결과 복귀를 새 재현본에서 확인했다.
- [x] 실제 같은 두 프로세스4회 복귀/5개 매치와 게임 HUD 재생성을 확인했다.
- [ ] Entry60초 대기·빠른 중복 클릭·호스트 먼저 복귀 시 peer 오류 문구를 실제 UI에서 확인했다.
- [ ] Android 가상 키보드·SafeArea·물리 터치를 확인했다.

최종5시드와 지연/손실 회복은 통과했다. 다음 단계는 진행 중인 대표 부하20분과 위 남은 UI 수명 검수의 결과 연결이다. Android 가상 키보드·SafeArea·물리 터치는 G4 **NotRun**이며, 필수 범위가 남은 현재는 Draft를 유지한다.
