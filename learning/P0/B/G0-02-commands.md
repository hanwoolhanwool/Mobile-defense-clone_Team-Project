# 명령 입구와 중복 방지 — P0 / B / G0-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01, [명령 계약 16.2](../../../docs/technical/ARCHITECTURE.md), [B 구현 설계](../../../docs/technical/IMPLEMENTATION_B.md) |
| 참고 자료 제작 상태 | Verified — 독립 B 조립·자동화·native 실제 소유 RPC/응답 제한 재현, canonical 종료 캐시 별도 Pass. 선택 BP 연결 제외 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 소스 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` |
| 실제 개발 시작/완료 SHA | 공통 출발점 / 미생성 |
| 필요한 상대 산출물·버전 | [G0-01](G0-01-foundation.md)의 공통 타입·매치·로더, 준비/종료 호출 |
| 제공 코드 / 직접 작성할 코드 | 제공: FLDMatchContext 등 직접 합의한 선언, 검증 기대표·실제 PIE fixture `5f1f086`. 직접 작성: Network·PlayerController·명령 값 자동화. Board/Economy는 명시적 Stub |

## 이번에 만들 동작

동일 요청을 여러 번 보내도 결과가 바뀌지 않는 명령 입구를 만든다. 서버 문맥에서 참가자 신원을 얻고, 클라이언트는 서버가 발급한 연결 세대와 증가 번호만 사용한다. G0는 게임 상태를 변경하는 경제·보드 구현이 없으므로 실제 소환 성공은 발생하지 않는다.

명령 키·순서·한도는 정식 16.2가 원본이다. 이 수업의 차이는 P0 필드를 모두 고정 크기로 선언하여 네트워크 입력에서 배열/문자열 크기에 따른 메모리 할당을 만들지 않는 것이다. 합성 재료는 고정 3개 ID이며 순서 정규화 전에 중복을 거절한다. 비활성 payload 필드에 값이 들어오면 거절한다. P1/P2 예약 타입은 활성화하지 않는다.

## 코드 작성 순서

[공통 재현 절차](../COMMON.md#g0-replay)의 `-Role B`를 사용한다. G0-01의 MatchTypes/데이터/상태 → 이 수업의 Network/Controller → G0-01의 GameMode → LDCommandTests 순서로 모든 소스를 조립한 뒤 Editor 빌드는 한 번이다. 같은 커밋의 한 테스트 파일에 로더1개와 명령3개가 있으므로 `LD.P0.G0.Commands` 전체4개 결과에서 부분 결과를 구분한다.

1. `Network/LDCommandTypes.h/.cpp`: enum·요청·결과를 작성한다. 응답에는 MatchId를 포함한다. `IsValidPayload`는 형식만 검사하고, 게임 규칙은 향후 경제/보드에 맡긴다. `Normalized`는 합성 재료 순서만 정렬하며 선택 InstanceId를 바꾸지 않는다.
2. `Network/LDCommandProcessor.h/.cpp`: Initialize → RegisterParticipant → Submit 순서로 연결한다. 세대별 최고 번호·최근256개 결과·순서를 서버 객체에 둔다. 처리 순서는 서버 신원/세대 → 구조 → 캐시/충돌/만료 → 새 번호 등록 → 한도 → 준비 여부 → 명시 Stub이다.
3. `SubmitAtTime`에 명시 시각을 전달해 12개 burst·초당8개 경계를 테스트한다. 실제 Submit은 FPlatformTime의 단조 시각을 사용한다. 캐시는 Prepare/Result 변경보다 먼저 검사한다.
4. `Core/LDPlayerController.h/.cpp`: 소유 PlayerController에 Server Reliable RPC를 둔다. UI는 SubmitLocalCommand만 호출한다. RequestId·PendingCommand는 Controller 수명에 유지하고 위젯 재생성과 분리한다. 재시도는 RetryPendingCommand로 동일 번호·같은 내용을 재전송한다.
5. 소유 클라이언트에 CurrentMatchId·ConnectionEpoch를 복제하고 ClientCommandResult에서 현재 매치·세대·요청이 모두 같은지 검사한다. 과거 결과로 최신 상태를 덮어쓰는 기능은 만들지 않는다.
6. Close는 접수를 영구 종료하지만 캐시는 보존한다. EndPlay는 서버 연결·미확정 로컬 추적·delegate를 정리한다. 새로운 매치/세대는 이전 요청을 자동 실행하지 않는다.
7. `Tests/LDCommandTests.cpp`에 아래 구현 독립 기대 결과를 연결한다. Board/Economy 원본이 아직 없으므로 재화·RNG 불변 검사는 G2 통합에서 추가한다.

ARCH-01~03: Controller는 입력/응답 추적, Processor는 중복/접수 원본, Economy/Board는 향후 각 게임 상태 원본이다. ARCH-04: G0에서는 모든 요청이 거절돼 성공 이벤트가 없다. G2의 공동 확정은 아직 구현되지 않았다. ARCH-05: UI delegate의 등록 해제는 향후 UI 연결자가 담당하고 Controller EndPlay가 마지막으로 Clear한다. ARCH-06: 아래 테스트는 형식·경계 계산 검증이며 실제 RPC 네트워크 검수는 별도다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | G0 검증 GameMode | PlayerControllerClass=LDPlayerController | 플레이어 소유 객체를 통해서만 서버 RPC 호출 |
| 2 | GameMode PostLogin | Processor.RegisterParticipant → Controller.InitializeServerSession | 문맥은 클라이언트 payload에서 받지 않음 |
| 3 | Session Frontend → Automation | LD.P0.G0.Commands.AdmissionAndReplay, LimitsAndExpiry, PayloadNormalization | 계산 검증 3종 실행 |
| 4 | UI/Blueprint | G0 신규 에셋 없음, 바인딩 없음 | 실패 결과 표시 HUD는 G2 이후. 현재 기대는 응답 코드 관찰 |
| 5 | 제공 native PIE 검사 | 기존 TopDown 맵·LDGameMode override·ListenServer/2인/동일 프로세스; B 필터 `LD.PIE.G0.B.OwnedRPC` | 제공 Editor 검사 설치 뒤 실행. BP/맵 저장 변경 없음 |
| 6 | 이후 G1/G2 연결 | Board/경제 준비 완료 후 SetAcceptingCommands 호출, 실제 executor 추가 | G0 Stub 제거 후 기능 검수 재실행 |

실제 native PIE에서 두 World·owner 문맥 복제·소유 RPC 전송/응답과 종료를 확인했다. G0 게임 HUD는 없으므로 기대/관찰은 명령 결과 코드와 실제 객체 상태다. 공통 설치·새 RunId·필터·로그 위치는 [B G0 실제 네트워크 재현](../evidence/G0_REPLAY/B-network.md)에 모았다. 선택 Blueprint GameMode 에셋 연결이나 별도 패키지 화면을 검수한 것으로 확대하지 않는다.

## 실행·실패·수정 기록

아래 값 자동화는 최초 B 역할 작성본과 새 detached 출발점의 재현 결과다([기존 재현 요약](../evidence/G0_REPLAY/SUMMARY.md), [자동화4종](../evidence/G0_REPLAY/b-tests-result.json)). 2026-09-27에는 같은03acb67 제품의 native 실제 PIE1Pass/0Fail와 canonical649c1de의 별도 PIE1Pass/0Fail로 소유 RPC·유실/응답 제한·종료 캐시를 재현했다. 경고 이벤트는 B1/canonical2이며 [공통 실행 기록](../evidence/G0_REPLAY/B-network.md)에 원문을 보존한다. 이 범위로 Verified를 판정하되 학습자가 직접 작성했다는 기록은 아니므로 학습자는 Planned다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 준비 전 RequestId=1 | PhaseNotAllowed, EventId=0, Revision=0 | Pass | AdmissionAndReplay |
| 같은 키 같은 내용, 준비 상태 변경 후 재요청 | 최초 PhaseNotAllowed 유지 | Pass | AdmissionAndReplay |
| 같은 키·ExpectedBoardRevision 변경 | RequestIdConflict, 원래 캐시 보존 | Pass | AdmissionAndReplay |
| 준비 flag만 열고 새 번호 | FeatureDisabled, 경제/보드 성공 흉내 없음 | Pass | AdmissionAndReplay |
| Close 후 기존 번호/새 번호 | 원래 결과 / PhaseNotAllowed | Pass | AdmissionAndReplay |
| 매치/연결 세대 위조 | InvalidEpoch | Pass | AdmissionAndReplay |
| 동일 시각12개 뒤13번째 | 13번째 RateLimited 캐시 | Pass | LimitsAndExpiry |
| 정확히0.125초 후 새 요청 | 한 토큰 복구 | Pass | LimitsAndExpiry |
| 257개 확정 후1번 재요청 | 캐시256 유지·RequestExpired·재실행 없음 | Pass | LimitsAndExpiry |
| 합성 재료 순서만 변경/중복 ID | 동일 내용 / InvalidPayload | Pass | PayloadNormalization |
| C++ 포맷·diff 공백 | 0 오류 | Pass | `Saved/P0Evidence/G0-B/style.log`, git diff --cached --check |
| Editor/UHT | 실제 컴파일 성공 | Pass | `Saved/P0Runs/G0-B-editor/`, UE5.8.2/MSVC |
| 실제 소유 RPC·같은 번호 재전송 | 번호1 실제 왕복2회, 완료1·cache1 | Pass | [B 실제 proof](../evidence/G0_REPLAY/B-native-proof.json)의 owned-rpc |
| 다른 소유자의 epoch·비로컬 PC 의도 | InvalidEpoch / false, 완료·캐시 불변 | Pass | 같은 proof의 owner-mismatch. 상대 보드가 없는 G0이므로 NotOwner 보드 검사는 아님 |
| client 전송 손실·old match/epoch·Pending 응답 | 번호2 pending 유지, 외래 성공 무시, 같은 문맥 Pending만 수용 | Pass | 해당 NetDriver loss100/0.4초와 명시 서버 Client-RPC 응답 fixture를 구분 |
| 손실 복원 뒤 RetryPending | 같은 번호2·새 완료1·cache 총2 | Pass | 실제 서버 dispatch3/같은 원응답 수신 |
| 응답 예산 고갈 뒤 충전·Retry | 번호3은 처리/cache3 됐지만 최초 송신0·pending 유지, 충전 뒤 같은 응답·완료 총3 | Pass | 실제 재전송32+새 명령1의33 dispatch·서버 송신 관측·후속 Retry. 전송 손실과 별도 단계 |
| canonical Mode.AbortMatch 뒤 동일/변경/새 명령 | 원응답 동일 / RequestIdConflict / PhaseNotAllowed; cache4·완료3 | Pass | [canonical 실제 proof](../evidence/G0_REPLAY/canonical-native-proof.json), `_Implementation` 직접 호출 없음 |
| 선택 BP 연결·별도 프로세스 패키지·Android | 해당 경로의 실제 실행 | NotRun | native 두 PIE World를 패키지/기기 통과로 세지 않음 |

위 명령3종 Pass는 2026-09-18 UE Automation/NullRHI에서 실제 C++ 테스트를 실행한 결과다. 증거는 `Saved/P0Runs/G0-B-tests/engine.log`, `report/index.json`이다. 최초 suite는 BIndependentLoader 1건이 실패했으므로 당시 G0 전체 통과가 아니었다. 수정 후 `Saved/P0Runs/G0-B-tests-fix1/result.json`은 4Pass/0Fail/0NotRun이다. 원인·수정은 첫 수업에 기록했다. 실제 PIE·화면·별도 프로세스 네트워크 통과로 확대 해석하지 않는다.

리뷰에서 응답의 매치 식별 누락이 발견됐다. 세대/번호만 같으면 다른 매치의 지연 응답을 구분하기 어렵기 때문에 MatchId를 결과에 넣고 Controller가 owner-only CurrentMatchId와 대조하도록 수정했다. 이번 실제 PIE에서는 서버 Client RPC로 보낸 외래 MatchId/event701·외래 epoch/event702가 실제 도착했지만 완료/마지막 결과를 덮어쓰지 않았고, 같은 문맥 Pending/event703만 수용했다. 이는 명시 응답 fixture이며 제품의 성공이나 비동기 작업을 만들어낸 검사가 아니다.

응답 제한은 재확인 트래픽에도 적용된다. 한도 초과 시 원래 결과를 바꾸지 않고 응답을 보류하므로 Pending이 남을 수 있다. 실제 B PIE에서32개 재전송 뒤 새 번호3을 서버가 처리해cache3이 됐지만, 그 응답의 서버 송신 시도는0이었다. 충전 뒤 RetryPendingCommand가 같은 번호3의 원래 PhaseNotAllowed를 받고 terminal 완료를 한 번만 늘렸다. 그 전에 수행한 client outgoing packet loss100 단계와 혼동하지 않는다. 송신/수신/완료 관측은 제공 검사가 담당하고 게임 코드의 토큰·시계를 직접 고치지 않았다.

G2에서는 UI 대기 표시·제한된 재시도·RequestExpired 뒤 최신 Revision 동기화를 붙여야 한다. G0에는 비동기 준비 작업이 없으므로 서버가 처리 중인 같은 명령을 실제 executor에 중복 투입하는 검증은 G2 의존성이다. 이번 Pending 응답 주입 Pass가 그 기능을 완성하지 않는다.

## 상대에게 전달하고 통합하기

최초 전달 커밋 `18b1ace8cb24bd915cf4a2d660047538fd7ab0c6`, 이 수업 재현의 완료 소스는 로더 수정까지 포함한 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`다. 통합 순서는 A Match/Data·공용 상태 채택 → B Network/Controller 반영 → GameMode가 UPROPERTY로 Processor 보관 → 참가자 등록과 종료 호출 연결 → 동일 통합 SHA에서 Editor 및 LD.P0.G0.Commands 실행이다. A에는 `InitializeServerSession`, `ShutdownServerSession`, Processor `Initialize/RegisterParticipant/Close`를 전달한다. 매치 종료 시 Close로 새 접수를 닫되 캐시 응답에 필요한 연결은 유지하고 Logout/EndPlay에서 최종 해제해야 한다. 단독 Close 테스트만으로 Controller 경유 종료 검사를 대신할 수 없다. 통합 수정과 재검증은 [통합 기록](../INTEGRATION.md)에서 분리한다. G0의 SetAcceptingCommands(true)는 경제/보드 기능 완료를 뜻하지 않는다. 새 UObject Processor로 다음 매치를 시작하며 기존 인스턴스를 재초기화하지 않는다.

G0 canonical은 `649c1dedd6832c41089a76b59bc76518cd262296`이며 종료·준비 순서 수정과 실제 Mode/Controller 회귀12개 결과는 [통합 기록](../INTEGRATION.md)에 있다. 제공 `5f1f086` 검사에만 Editor define `LD_G0_CANONICAL_PIE=1`을 켠 별도 재현은 Mode.AbortMatch가 실제 양쪽에 복제된 뒤 raw RPC의 동일/변경/새 명령을 확인했다. 독립 B의 제품 API를 늘리거나 최신 G3 파일을 섞지 않았다. 통합은 기존 값 자동화→제공 native PIE→Play/packet/observer 복원·World0 순으로 확인한다. [공통 실행 기록](../evidence/G0_REPLAY/B-network.md)의 API·증거·범위를 전달하고, 실제 학습 통합 커밋과 세 learn 브랜치는 변경하지 않는다.

## 이해 확인

- 캐시 검사를 Phase 검사 뒤로 옮기면 어떤 재전송 오류가 생기는가?
- RequestExpired와 실패한 명령의 차이는 무엇이며 자동 재실행하면 왜 위험한가?
- 작은 변형: 3번 재료 ID만 바꾼 동일 RequestId와 재료 순서만 바꾼 동일 RequestId를 비교한다.
- 작은 변형: 0.124초와0.125초의 요청 결과를 비교하고 서버 시각의 단위를 설명한다.
- 다음 단계: G0 역할/통합 Editor 빌드·자동화·리뷰. 그 뒤 G1 두 화면·전체 셀 입력 통과 후에만 경제/전투를 확장한다.

## 단계 완료

- [x] 고정 크기 payload·키·상태 소유권·명시 Stub 범위를 기록했다.
- [x] 설명대로 새 출발점에서 B-01/02를 합친 Editor/자동화 재현 증거를 확보했다.
- [x] 독립 B의 실제 소유 RPC·문맥 복제·손실·응답 제한 후 동일 번호 재시도를 확인했다.
- [x] canonical Controller의 실제 네트워크 종료 후 동일/변경/새 명령 응답과 수명 정리를 확인했다.
- [x] Pending 재시도·최신 Revision 동기화·공동 확정의 후속 의존성을 명시했다.

Verified 범위는 제공 native TopDown/LDGameMode 경로다. 선택 BP 연결·게임 HUD·별도 프로세스 패키지·Android·G2 보드/경제 효과는 이 수업의 통과 범위 밖이다.
