# 서버 매치와 참가자 수명 — P0 / A / A-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01 중 A / [A-02 설계](../../../docs/technical/IMPLEMENTATION_A.md#a02) |
| 참고 자료 제작 상태 | Verified — 고정 독립 A 소스의 TopDown/native LDGameMode 조립·Editor·실제 PIE 정상/누락 경로. 수동 BP 에셋 재생성은 미검증 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 독립 코드 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`; 출발 HEAD의 [조립 manifest](../evidence/G0_REPLAY/a-assembly.json)로 재현 |
| 실제 개발 시작/완료 SHA | 출발점만 준비 / 미생성 |
| 필요한 상대 산출물·버전 | A-01 규칙 로더. 독립 A 실행은 B 서비스를 Stub으로 두며 B Controller·Processor 실제 연결은 통합 수업 |
| 제공 코드 / 직접 작성할 코드 | 제공: 기본 UE 프로젝트·A-01 직접 작성 결과. 직접 작성: Data/LDMatchTypes.h, Core/LDGameMode.*, Core/LDGameState.*, Core/LDPlayerState.* |

## 이번에 만들 동작

서버가 매치 GUID와 참가자 보드 번호·연결 세대를 발급한다. GameState는 공통 문맥·Phase·준비가 막힌 이유를 복제한다. 두 참가자와 데이터가 있어도 B 서비스가 없는 A G0 단독 실행은 Preparing에 머무르며 CanAcceptCommands는 false다. 이 Stub은 성공 응답이나 전투 진행을 만들지 않는다.

규칙/좌표/명령의 상세 계약은 [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md#contracts), 초기화/종료 원칙은 [수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md#lifecycle)이 원본이다. G0는 연결 경계를 만들며 웨이브·보스 상태·결과 위젯은 후속 단계에서 추가한다.

## 코드 작성 순서

빌드 가능한 작성 순서는 **이 수업1번 MatchTypes → A-01 데이터·테스트 → 이 수업2~6번 Core**다. [공통 재현 절차](../COMMON.md#g0-replay)의 `-Role A`는 이 파일 목록을 고정 SHA에서 순서대로 조립한다. A 단독 완료 소스에는 B Network/Controller가 없고 기본 PlayerController를 사용한다. 이 차이를 실패로 간주하여 통합 완성 소스를 조립 폴더에 추가하지 않는다.

1. `Data/LDMatchTypes.h`: Phase enum, MatchContext, ParticipantContext를 작성한다. participant의 uint64 ConnectionEpoch는 Blueprint 핀으로 직접 노출하지 않는다. IsValid는 PlayerIndex 0/1, 비어 있지 않은 MatchId, Epoch>0을 요구한다.
2. `Core/LDGameState.h/.cpp`: 복제 필드와 OnRep를 선언한다. InitializeMatch는 중복 동일 문맥을 허용하고 다른 매치로 덮어쓰지 않는다. SetPhase는 서버만 호출할 수 있고 Loading→Preparing→Running→Result, 비종료→Aborted만 허용한다. Result/Aborted는 되돌리지 않는다.
3. `Core/LDPlayerState.h/.cpp`: 서버 발급 문맥을 한 번 초기화한다. 같은 문맥의 재호출만 허용한다. ConnectionEpoch 포함 문맥은 소유자에게만 복제하고 PublicPlayerIndex는 모두에게 복제한다. 경제 원본·RNG를 PlayerState에 추가하지 않는다.
4. `Core/LDGameMode.h/.cpp`: native GameStateClass/PlayerStateClass를 지정한다. InitGameState에서 UPROPERTY GameData 생성·검증→매치 문맥 발급→Preparing 순서로 처리한다. 실패 시 사유를 게시하고 Aborted로 종료한다.
5. PostLogin에서 슬롯0/1을 순서대로 배정하고 NextConnectionEpoch를 발급한다. 같은 Controller 등록은 중복 처리하지 않는다. 세 번째 접속은 GameSession.KickPlayer로 거절한다. Logout은 약한 참조 슬롯을 해제하고 준비 이유를 갱신한다.
6. EndPlay/AbortMatch는 StopMatchServices로 모은다. bEnding으로 중복 종료를 막고 타이머와 참가자 관찰 참조를 정리한다. 현재 B 객체·전투 예약·delegate 연결은 없으며, 통합에서 실제 소유 참조와 연결 해제를 추가해야 한다.

호출 및 상태 흐름: 서버 `InitGameState → GameData.LoadP0 → GameState.InitializeMatch/SetPhase(Preparing)`; 서버 `PostLogin → PlayerState.InitializeParticipant → Participants 약한 참조 저장 → readiness 복제`; 클라이언트 `GameState.OnRep_CommonState → OnMatchStateChanged`. 클라이언트 화면 캐시는 원본을 쓰지 않는다.

| 구조 기준 | 이번 코드 경로와 이유 |
|---|---|
| ARCH-01 책임 | GameMode는 생성·연결·전이, GameState는 공통 원본/복제, PlayerState는 참가자 공개 요약, GameData는 검증된 값 snapshot |
| ARCH-02 의존 | Core가 Data를 읽음. Data는 Controller·UI를 찾지 않음. 미연결 B 서비스를 전투 계산에서 탐색하지 않음 |
| ARCH-03 원본 | Phase의 원본은 서버 GameState, 규칙은 매치별 ULDGameData. 참가자 연결은 GameMode가 발급 |
| ARCH-04 처리 | 데이터 전체 검증 뒤 게시. 경제/보드 공동 확정은 이 단계에서 구현하지 않음 |
| ARCH-05 수명 | UObject 소유는 UPROPERTY/TObjectPtr, Controller 관찰은 TWeakObjectPtr, 종료는 한 경로 |
| ARCH-06 검증 | 실제 코드 Automation 신원 경계 + Editor/UHT. 독립 A의 실제 PIE 두 세션·셋째 접속·복제·누락 초기화는 [별도 실행](../evidence/G0_REPLAY/a-network.md)으로 확인 |

## Unreal 설정 순서

**확인한 native 경로:** 기존 `/Game/TopDown/Lvl_TopDown`을 요청 한정 native ALDGameMode override로 실행한다. 부모 클래스 기본값의 GameState=LDGameState, PlayerState=LDPlayerState, PlayerController=기본 PlayerController, DefaultPawn=None을 사용한다. 제공 검사기는 ListenServer·한 프로세스2인·요청 창540×720·온라인 subsystem 끔으로 설정하고 종료 후 원래 Editor 설정을 복원한다. 맵/BP를 저장하지 않는다. 설치·누락 fixture·실행 인수와 기대값은 [실제 native 재현 절차](../evidence/G0_REPLAY/a-network.md)에 둔다.

아래 **선택적 수동 BP 구성안은 미검증**이다. native 경로와 같은 클래스를 연결하는 방법을 보존하며 BP 에셋 생성/저장을 실제 통과로 기록하지 않는다. 공용 에셋·맵 편집권은 통합 담당자가 직렬로 갖는다.

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | Content Browser `/Game/LD/Core` → Blueprint Class → All Classes | `BP_GameMode` 부모 ALDGameMode, `BP_GameState` 부모 ALDGameState | 기본 템플릿 GameMode를 상속해 규칙을 섞지 않음 |
| 2 | BP_GameMode → Class Defaults → Classes | Game State Class=BP_GameState, Player State Class=LDPlayerState | 두 참가자에게 같은 공통 상태 복제 |
| 3 | A 단독 G0 Class Defaults | Player Controller Class=기본 PlayerController, Default Pawn Class=None | 카메라/보드 준비를 성공으로 가장하지 않음; 게임 화면 완성 단계 아님 |
| 4 | G0 통합 후 Class Defaults | Player Controller Class=B의 LDPlayerController, Default Pawn Class=None | G0에는 CameraPawn이 없다. G1에서 실제 구현 후 별도 연결 |
| 5 | 테스트 레벨 → World Settings → GameMode Override | BP_GameMode 또는 native LDGameMode | 메뉴 기본값과 실제 맵 override가 다를 수 있으므로 생성 로그 확인 |
| 6 | Play 드롭다운 → Advanced Settings → Multiplayer | Number of Players=2, Net Mode=Play As Listen Server | G0 공통 상태만 관찰; 최종 PC 두 프로세스 검수를 대신하지 않음 |
| 7 | UMG/HUD | G0 A에서 새 위젯 없음 | 화면상의 성공 대신 준비 사유가 복제되는지 먼저 확인 |

예상 관찰은 `LogLDMatch: G0 match <GUID> rules=0.3.0 units=16 waves=10`, 참가자0/1의 서로 다른 epoch, `Preparing: 2/2 participants; Stub: ... not connected`다. 실제 정상 두 세션에서 이를 확인했다. [host](../evidence/G0_REPLAY/a-native-host.png)·[client](../evidence/G0_REPLAY/a-native-client.png)는 템플릿 벽·하늘만 보이는546×720 기본 카메라 화면이며 G0 HUD/전장/버튼은 없다. 상태 Pass는 화면 모양이 아니라 실제 World의 복제값과 OnRep 관찰로 판단한다.

## 실행·실패·수정 기록

새 출발점의 조립·컴파일과 `LD.P0.G0.Data`4Success 중 ParticipantIdentity 값 경계는 [기존 증거](../evidence/G0_REPLAY/SUMMARY.md)다. 2026-09-27 추가한 **독립 A 실제 PIE**는 제품4cc3e0f를 유지하고 제공 검사기fdc12b7만 별도로 설치했다. 정상 두 세션 및 JSON 누락의 별도 조립 한 세션을 각각 빌드·실행해 report Success1/Fail0, 설정 복원·World0을 확인했다. 이 추가 실행으로 native 경로의 접속·종료·실제 복제 결손을 채웠다. canonical/B 성공이나 G3 성공을 독립 A에 승계하지 않았다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 정상 로드 + 두 접속 | 동일 MatchId, PlayerIndex0/1, Preparing, 요청 비활성 | Pass | 실제 listen/client2 World, epoch1/2, client OnRep·owner private 복제 경계 |
| 빈 문맥·PlayerIndex=-1/2·epoch0 | IsValid=false | Pass | `LD.P0.G0.Data.ParticipantIdentity`; [실제 증거](evidence/G0_RUNTIME.md) |
| 동일 초기화/동일 PostLogin 재호출 | ID·세대·원래 객체 보존 | Pass | 실제 Mode 공개 초기화/PostLogin 재호출, 로더 동일·다른 문맥 거절 |
| 데이터 누락 | 구체적 오류와 Aborted, 새 접수0 | Pass | 신규20파일 fixture에서 GameRules.json을 처음부터 제외; 실제 Mode 로딩 Abort |
| 세 번째 참가자 | 슬롯을 덮어쓰지 않고 거절 | Pass | RequestLateJoin의 실제 NetConnection/PostLogin→Logout/Destroy, 원래2슬롯 유지 |
| Result/Aborted 뒤 재진입·반복 EndPlay | 상태 되돌림0, 예약 정리 | Pass | 공개 전이 fixture→client 복제·역전 거절, 실제 Mode-bound 검사 타이머 제거. 새 PIE MatchId 변경·각World0 |
| C++ 서식/공백 | 오류0 | Pass | [정적 기록](evidence/G0_STATIC.md) |
| Editor 컴파일 | UHT/C++/링크 성공 | 첫 빌드 Fail; 로더 문자열 오류 수정 후 재빌드 Pass | [A-01 실패 기록](G0_01_DATA.md), [실제 증거](evidence/G0_RUNTIME.md) |
| 실제 PIE 네트워크 | 해당 native 절차의 복제·접속·종료 | Pass, 경고 있음 | [실행·입력·경고·한계](../evidence/G0_REPLAY/a-network.md); B 게임명령 RPC는 다른 수업 |
| 선택 BP 재생성·패키지·Android | 각 검수 성공 | NotRun | 이 수업 Verified 범위에 포함하지 않음 |

실제 컴파일 실패와 문자열 연결 수정 후 재빌드 Pass는 A-01에 기록했다. 이번 정상 PIE는 경고4개(셋째 거절 ConnectionLost1/CrowdFollowing3), 누락은 CrowdFollowing1개와 함께 성공했다. 무경고로 기록하지 않는다. 제공 검사기의 delegate 조기 구독·네트워크 observer 수명·시작 timeout 문제는 **실행 전 정적 발견**으로 수정했고 실제 런타임 실패로 분류하지 않는다. 상세 SHA와 원문은 공통 [A 네트워크 증거](../evidence/G0_REPLAY/a-network.md)에서 관리한다.

## 상대에게 전달하고 통합하기

GameState API는 `InitializeMatch`, `SetPhase`, `GetPhase`, `GetMatchContext`, `SetReadinessReason`, `OnMatchStateChanged`; PlayerState API는 `InitializeParticipant`, `GetParticipantContext`, `GetPlayerIndex`다. 최종 선언은 코드 헤더를 따른다.

통합 순서: A/B 독립 공통 구현 차이 비교 → 하나의 GameState/PlayerState/로더 선택 → GameMode에 B Processor 소유 참조 생성 → 참가자 등록과 `Controller.InitializeServerSession` 연결 → 필수 실제 서비스 준비 전 접수 닫힘 유지 → 종료 시 접수·진행을 닫고 동일 연결의 캐시 응답 경로 유지 → Logout/EndPlay에서 최종 session/구독 해제 → G0 통합 빌드/실행. Board/Economy는 G0에 없는 Stub이며 실제 구현 단계에서 소유 참조를 추가한다. A의 하드 false Stub은 실제 서비스 검증과 함께 교체해야 한다. 통합에서 드러난 초기화 순서·종료 경계 수정은 [통합 기록](../INTEGRATION.md)에 따로 남기며 A 단독 완료 SHA의 검수 결과로 소급하지 않는다.

독립 재현 기준은 위 표의 A 커밋이고, G0 canonical `649c1dedd6832c41089a76b59bc76518cd262296` 및 역할 반영점은 [통합 기록](../INTEGRATION.md)에 있다. 실제 학습 통합 SHA는 미생성이다. canonical 반영을 A/B가 그 코드를 각각 독립 작성한 것으로 기록하지 않는다.

## 이해 확인

- PlayerState가 개인 재화와 RNG의 원본까지 소유하면 어떤 책임 충돌이 생기는가?
- 클라이언트가 PlayerIndex를 보낸다고 참가자로 신뢰하면 왜 다른 보드 조작이 가능한가?
- 같은 접속 세대의 재시도와 새 연결의 요청은 왜 다른 식별 키를 갖는가?
- 작은 변형: 테스트 레벨에서 데이터 폴더를 별도 누락 픽스처로 주입하는 테스트를 추가하고 Aborted 뒤 Running 전이가 거절되는지 확인한다. 제품 Content를 삭제하지 않는다.
- 다음 단계는 A/B/통합 Editor 빌드·자동화, 실제 G0 초기화/종료 검수 뒤 G1 경로/두 화면 구도로 진행한다. 전투 확장은 G1 통과 후다.

## 단계 완료

- [x] 출발점의 A-01/02 파일 조립·UFS 설정·합친 Editor 빌드 및 native 독립 PIE를 재현했다.
- [x] 정상 접속/복제/중복 초기화/셋째 거절/반복 종료·재시작과 누락 로딩 Abort를 고정 제품 소스에서 확인했다.
- [x] 필요한 A-01 로더를 합쳐 확인했고, B 미연결 Stub과 후속 통합 책임을 구분했다.
- [ ] 선택적 BP 에셋 재생성은 미검증이며 위 native 경로의 Verified 판정에 포함하지 않는다.
- [x] 미검증 범위와 다음 단계의 의존성을 명시했다.
