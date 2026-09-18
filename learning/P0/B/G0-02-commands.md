# 명령 입구와 중복 방지 — P0 / B / G0-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01, [명령 계약 16.2](../../../docs/technical/ARCHITECTURE.md), [B 구현 설계](../../../docs/technical/IMPLEMENTATION_B.md) |
| 참고 자료 제작 상태 | Draft |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 소스 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` |
| 실제 개발 시작/완료 SHA | 공통 출발점 / 미생성 |
| 필요한 상대 산출물·버전 | [G0-01](G0-01-foundation.md)의 공통 타입·매치·로더, 준비/종료 호출 |
| 제공 코드 / 직접 작성할 코드 | 제공: FLDMatchContext 등 직접 합의한 선언, 검증 기대표. 직접 작성: Network·PlayerController·명령 자동화. Board/Economy는 명시적 Stub |

## 이번에 만들 동작

동일 요청을 여러 번 보내도 결과가 바뀌지 않는 명령 입구를 만든다. 서버 문맥에서 참가자 신원을 얻고, 클라이언트는 서버가 발급한 연결 세대와 증가 번호만 사용한다. G0는 게임 상태를 변경하는 경제·보드 구현이 없으므로 실제 소환 성공은 발생하지 않는다.

명령 키·순서·한도는 정식 16.2가 원본이다. 이 수업의 차이는 P0 필드를 모두 고정 크기로 선언하여 네트워크 입력에서 배열/문자열 크기에 따른 메모리 할당을 만들지 않는 것이다. 합성 재료는 고정 3개 ID이며 순서 정규화 전에 중복을 거절한다. 비활성 payload 필드에 값이 들어오면 거절한다. P1/P2 예약 타입은 활성화하지 않는다.

## 코드 작성 순서

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
| 5 | 이후 G1/G2 연결 | Board/경제 준비 완료 후 SetAcceptingCommands 호출, 실제 executor 추가 | G0 Stub 제거 후 기능 검수 재실행 |

실제 G0 화면·PIE 관찰은 아직 없다. 헤더·C++ 포맷과 소스의 기대 흐름만 확인됐다.

## 실행·실패·수정 기록

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
| RPC·패키지·Android | 실제 실행 결과 확보 | NotRun | 통합 담당자의 후속 게이트 |

위 명령3종 Pass는 2026-09-18 UE Automation/NullRHI에서 실제 C++ 테스트를 실행한 결과다. 증거는 `Saved/P0Runs/G0-B-tests/engine.log`, `report/index.json`이다. 최초 suite는 BIndependentLoader 1건이 실패했으므로 당시 G0 전체 통과가 아니었다. 수정 후 `Saved/P0Runs/G0-B-tests-fix1/result.json`은 4Pass/0Fail/0NotRun이다. 원인·수정은 첫 수업에 기록했다. 실제 PIE·화면·별도 프로세스 네트워크 통과로 확대 해석하지 않는다.

리뷰에서 응답의 매치 식별 누락이 발견됐다. 세대/번호만 같으면 다른 매치의 지연 응답을 구분하기 어렵기 때문에 MatchId를 결과에 넣고 Controller가 owner-only CurrentMatchId와 대조하도록 수정했다. 네트워크 실행으로 확인할 것은 남아 있다.

응답 제한은 재확인 트래픽에도 적용된다. 한도 초과 시 원래 결과를 다른 결과로 바꾸지 않고 응답을 보류하므로 Pending이 남을 수 있다. 현재 RetryPendingCommand API만 제공한다. G2 UI 연결 때 동일 번호를 사용하는 제한된 재시도와 사용자 대기 표시를 붙이고, 종료·응답 유실 검수를 해야 한다. RequestExpired 때 최신 보드/경제 Revision 동기화를 기다리는 UI도 G2 의존성이다. G0에는 비동기 준비 작업이 없으므로 진행 중 Pending 중복을 재현하는 검증은 G2에서 추가한다.

## 상대에게 전달하고 통합하기

커밋 `18b1ace8cb24bd915cf4a2d660047538fd7ab0c6`. 통합 순서는 공통 Match/Data 선언 확정 → B Network/Controller 반영 → GameMode가 UPROPERTY로 Processor 보관 → 참가자 등록과 종료 호출 연결 → 동일 통합 SHA에서 Editor 및 LD.P0.G0.Commands 실행이다. A에는 `InitializeServerSession`, `ShutdownServerSession`, Processor `Initialize/RegisterParticipant/Close`를 전달한다. G0의 SetAcceptingCommands(true)는 경제/보드 기능 완료를 뜻하지 않는다. 새 UObject Processor로 다음 매치를 시작하며 기존 인스턴스를 재초기화하지 않는다.

통합 참고 완료 SHA는 아직 없고, 실제 학습 통합 커밋도 없다. 학습자 작업은 공통 출발점에서 직접 작성한다.

## 이해 확인

- 캐시 검사를 Phase 검사 뒤로 옮기면 어떤 재전송 오류가 생기는가?
- RequestExpired와 실패한 명령의 차이는 무엇이며 자동 재실행하면 왜 위험한가?
- 작은 변형: 3번 재료 ID만 바꾼 동일 RequestId와 재료 순서만 바꾼 동일 RequestId를 비교한다.
- 작은 변형: 0.124초와0.125초의 요청 결과를 비교하고 서버 시각의 단위를 설명한다.
- 다음 단계: G0 역할/통합 Editor 빌드·자동화·리뷰. 그 뒤 G1 두 화면·전체 셀 입력 통과 후에만 경제/전투를 확장한다.

## 단계 완료

- [x] 고정 크기 payload·키·상태 소유권·명시 Stub 범위를 기록했다.
- [ ] 설명대로 재현한 Editor/자동화 실행 증거를 확보했다.
- [ ] 통합 Controller와 네트워크에서 양쪽 응답을 확인했다.
- [x] Pending 재시도·최신 Revision 동기화·공동 확정의 후속 의존성을 명시했다.
