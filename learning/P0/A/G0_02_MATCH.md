# 서버 매치와 참가자 수명 — P0 / A / A-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01 중 A / [A-02 설계](../../../docs/technical/IMPLEMENTATION_A.md#a02) |
| 참고 자료 제작 상태 | Draft — 출발점 조립·합친 컴파일 재현 Pass, PIE 접속·복제 미검증 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 독립 코드 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`; 출발 HEAD의 [조립 manifest](../evidence/G0_REPLAY/a-assembly.json)로 재현 |
| 실제 개발 시작/완료 SHA | 출발점만 준비 / 미생성 |
| 필요한 상대 산출물·버전 | A-01 규칙 로더, B Controller·CommandProcessor 선언 및 구현 |
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
| ARCH-06 검증 | 실제 코드 Automation 신원 경계 + Editor/UHT 필수. 네트워크 복제·반복 매치는 별도 실행 필요 |

## Unreal 설정 순서

아래 에셋 작성은 재현 절차이며 실제 A 역할에서 생성/저장한 바이너리 에셋이 아니다. 공용 에셋·맵 편집권은 통합 담당자가 직렬로 갖는다.

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | Content Browser `/Game/LD/Core` → Blueprint Class → All Classes | `BP_GameMode` 부모 ALDGameMode, `BP_GameState` 부모 ALDGameState | 기본 템플릿 GameMode를 상속해 규칙을 섞지 않음 |
| 2 | BP_GameMode → Class Defaults → Classes | Game State Class=BP_GameState, Player State Class=LDPlayerState | 두 참가자에게 같은 공통 상태 복제 |
| 3 | A 단독 G0 Class Defaults | Player Controller Class=기본 PlayerController, Default Pawn Class=None | 카메라/보드 준비를 성공으로 가장하지 않음; 게임 화면 완성 단계 아님 |
| 4 | G0 통합 후 Class Defaults | Player Controller Class=B의 LDPlayerController, Default Pawn Class=None | G0에는 CameraPawn이 없다. G1에서 실제 구현 후 별도 연결 |
| 5 | 테스트 레벨 → World Settings → GameMode Override | BP_GameMode 또는 native LDGameMode | 메뉴 기본값과 실제 맵 override가 다를 수 있으므로 생성 로그 확인 |
| 6 | Play 드롭다운 → Advanced Settings → Multiplayer | Number of Players=2, Net Mode=Play As Listen Server | G0 공통 상태만 관찰; 최종 PC 두 프로세스 검수를 대신하지 않음 |
| 7 | UMG/HUD | G0 A에서 새 위젯 없음 | 화면상의 성공 대신 준비 사유가 복제되는지 먼저 확인 |

예상 관찰은 `LogLDMatch: G0 match <GUID> rules=0.3.0 units=16 waves=10`, 참가자0/1의 서로 다른 epoch, `Preparing: 2/2 participants; Stub: ... not connected`다. 전장/버튼/공격이 나타나는 것은 이 수업의 기대 결과가 아니다. 실제 화면 캡처·PIE 로그는 아직 없다.

## 실행·실패·수정 기록

새 출발점의 조립·컴파일 재현은 Pass다. [공통 재현 절차](../COMMON.md#g0-replay)로 A-01/02를 함께 조립·빌드했고 `LD.P0.G0.Data`4Success 중 ParticipantIdentity가 이 수업의 값 경계 부분 검사다([새 증거](../evidence/G0_REPLAY/SUMMARY.md)). 이 결과는 A 독립 GameMode의 접속·종료 실행이나 네트워크 복제의 Pass가 아니다. 별도로 수정한 canonical 통합은 실제 UWorld/GameMode/Controller 수명4개를 포함한12개 자동화가 Pass지만 실제 PIE 두 화면/RPC는 남아 있어 수업 상태를 Draft로 유지한다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 정상 로드 + 두 접속 | 동일 MatchId, PlayerIndex0/1, Preparing, 요청 비활성 | NotRun | PIE 2인 예정 |
| 빈 문맥·PlayerIndex=-1/2·epoch0 | IsValid=false | Pass | `LD.P0.G0.Data.ParticipantIdentity`; [실제 증거](evidence/G0_RUNTIME.md) |
| 동일 초기화/동일 PostLogin 재호출 | ID·세대·구독 중복 없음 | 코드 경로 검토, 실제 실행 NotRun | GameState/PlayerState Initialize, GameMode PostLogin |
| 데이터 누락 | 구체적 오류와 Aborted, 새 접수0 | NotRun | A-01 실패 픽스처 + 매치 실행 필요 |
| 세 번째 참가자 | 슬롯을 덮어쓰지 않고 거절 | NotRun | 세 접속 실행 필요 |
| Result/Aborted 뒤 재진입·반복 EndPlay | 상태 되돌림0, 예약/구독 없음 | 정적 경로 검토, 실제 실행 NotRun | SetPhase/StopMatchServices |
| C++ 서식/공백 | 오류0 | Pass | [정적 기록](evidence/G0_STATIC.md) |
| Editor 컴파일 | UHT/C++/링크 성공 | 첫 빌드 Fail; 로더 문자열 오류 수정 후 재빌드 Pass | [A-01 실패 기록](G0_01_DATA.md), [실제 증거](evidence/G0_RUNTIME.md) |
| 네트워크·패키지·Android | 각 검수 성공 | NotRun | 통합 담당자가 직렬 실행 예정 |

실제 컴파일 실패와 문자열 연결 수정 후 재빌드 Pass는 A-01에 기록했다. 게임 실행 화면은 아직 없다. 정적 리뷰 수정도 A-01의 정책/좌표 검증에 기록했으며 해당 변조 입력이 실제 UE 자동화에서 거절되는 것을 확인했다.

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

- [x] 출발점의 A-01/02 파일 조립·UFS 설정·합친 Editor 빌드를 재현했다. 독립 PIE 설정/실행은 남아 있다.
- [x] Unreal 컴파일·신원 자동화 결과와 시작/코드/실행 커밋을 연결했다. 통합 수명 회귀는 Pass, A 독립 PIE 접속·복제는 미완료다.
- [ ] 필요한 상대 기능을 합쳐 확인했다.
- [x] 미검증 범위와 다음 단계의 의존성을 명시했다.
