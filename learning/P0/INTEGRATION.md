# P0 게이트 통합

참고 제작은 G0 코드 게이트와 native 통합 재현 **Verified**, 전체 P0 자료는 후속 G3/G4 검수 중 / 실제 학습 **Planned**다. A/B G0 수업은 각 독립 출발 소스의 실제 PIE까지 확인했다. 공통 출발점과 실행 도구는 [COMMON](COMMON.md), 최신 상태는 [작업 기록](../../docs/production/P0_REFERENCE_RUN.md)을 따른다.

G0에서 독립 작성한 A/B 공통 코드의 타입·로더·초기화·상태 원본을 비교한다. 합의한 한 구현만 실행하고 B의 Controller/CommandProcessor를 GameMode 연결부에 결합한다. 각 역할과 통합 Editor 빌드, 독립 기대값 검사, ARCH-01~06 리뷰 후 G1에 들어간다.

G1 이후에도 각 기능 커밋→통합→실제 실행→학습 기록을 반복한다. 참고 완성 코드를 learn 브랜치에 병합하지 않는다. G1 실제 양쪽 화면/입력 이전 G2 확장은 금지한다.

## G0 계약 비교와 채택 결과

| 항목 | A 독립 구현 `4cc3e0f…` | B 독립 구현 `03acb67…` | 통합 선택과 이유 |
|---|---|---|---|
| 공개 값/API | Match/Participant/Data 값과 const 조회 API | 같은 공개 계약을 별도 구현 | 같은 이름의 공통 타입을 한 번만 정의. 독립 작성 이력은 각 역할 SHA에 보존 |
| 데이터 로더 | 필드·정책·좌표·참조별 오류, 전체 후보 검증 후 한 번 게시 | FReader가 오류를 모아 후보 스냅샷을 게시 | A 로더 채택. 중요 정책 변조·보드 좌표 중첩·뒤늦은 참조 실패를 명시적으로 검출하고 실패 주입 검사가 있음 |
| JSON 입력 | GameRules/Units/EnemyTypes/Waves/SummonProfiles/SpawnProfiles 6개 | 앞의5개 소비, SpawnProfiles 별도 파일을 읽지 않음 | canonical 런타임 입력6개. B의5개 소비 성공을 누락6번째 입력 검증으로 해석하지 않음 |
| 모듈 의존 | Json | Json, JsonUtilities | 실제 소비하는 Json만 유지. 런타임 로딩에 에디터 임포트 모듈 불필요 |
| 공용 원본·참가자 | GameState Phase/MatchContext/ReadinessReason, PlayerState 소유자 문맥+공개 보드번호 | 최소 공용 상태와 B 서비스 연결 | A의 상태·진단 원본 채택. 개인 재화/RNG나 보드 상태를 GameState/PlayerState에 중복 저장하지 않음 |
| 명령·로컬 추적 | B 서비스 없음, CanAcceptCommands=false | 서버 Processor, 소유 PlayerController RPC/요청 추적 | B Network/Controller 채택. 서버 Cmd만 세대별 캐시·최고번호를 소유; 화면은 명령 결과를 경제 원본에 역적용하지 않음 |
| 연결부 | A GameMode가 데이터·공용 상태·참가자 수명 관리 | B GameMode가 Processor/Controller도 연결 | A GameMode에 B Processor 연결을 추가. 두 GameMode를 병존시키거나 템플릿 Controller에 게임 규칙을 넣지 않음 |
| 전투·경제·보드 | 미구현 Stub | 미구현 Stub | G0에서 실제 명령 접수는 닫힘. 정상 소환·공격이 있다는 성공을 반환하지 않음 |

정확한 역할 전체 SHA와 제공 파일 SHA는 [공통 입력 표](COMMON.md#g0-replay)에 한 번 관리한다. 통합 후보 `3b4105e`에는 A 공통 구현, B 명령 구현, GameMode 연결과 `Tests/LDLifecycleTests.cpp`가 포함됐다. 이 SHA는 최종 승인점이 아니며 아래 독립 리뷰 수정 이전의 비교 기준이다.

## 작성·설정·검사 순서

1. 각 역할의 독립 재현은 [공통 조립 절차](COMMON.md#g0-replay)와 A-01→A-02, B-01→B-02 수업으로 먼저 확인한다. 완성 브랜치를 merge하여 수업 파일 누락을 가리지 않는다.
2. 통합 소스 작성 순서는 A `Data/LDMatchTypes.h`, `Data/LDGameData.*`, `Core/LDGameState.*`, `Core/LDPlayerState.*`, `Mobile_defense_clone.Build.cs` → B `Network/LDCommandTypes.*`, `Network/LDCommandProcessor.*`, `Core/LDPlayerController.*` → 통합 `Core/LDGameMode.*`다. 구체 타입의 생성·연결은 GameMode에 모은다.
3. 기본 테스트는 A `Tests/LDDataTests.cpp`4개, B `Tests/LDCommandTests.cpp`4개다. 통합 `Tests/LDLifecycleTests.cpp`는 MatchLifecycle/LoginReadinessOrders/TerminalSessionReplay/PendingLoginLogout4개로 매치 전이·참가자·초기화/종료 연결을 검사한다. 통합의 `BIndependentLoader`라는 기존 테스트 이름은 B 독립 로더가 실행된다는 뜻이 아니다. 통합 바이너리에서는 채택한 A 로더를 호출한다.
4. 제공 canonical JSON6개와 UFS 설정을 확인한다. 규칙의 원본은 `data/`, 제품 런타임 경로는 `Content/LD/Data`다. `Sync-P0Data.ps1 -Check`의 파일 대조와 실제 패키지 로딩 결과는 구분한다.
5. G0 소스의 클래스 기본값은 GameStateClass=LDGameState, PlayerStateClass=LDPlayerState, PlayerControllerClass=LDPlayerController, DefaultPawnClass=None이다. G0에 CameraPawn·WBP_HUD는 없다. **독립 역할 재현본**은 통합 전용 맵/BP나 기본 맵 변경을 가져오지 않았으며 템플릿 화면을 native 통합 클래스 검수로 세지 않는다.
6. 통합 담당자는 별도 직렬 작업으로 `/Game/LD/Maps/L_P0`와 `/Game/LD/Core/BP_LDGameMode`를 생성·저장했다([에셋 생성 결과](../../docs/production/evidence/RUN-20260918-G0/assets.json)). 이 증거 범위는 에셋 생성/저장이고 런타임 검증은 아니다. 실제 화면 검사 때 L_P0의 World Settings→GameMode Override 및 BP Class Defaults의 위4개 값·실제 생성 로그를 대조하고 Play→Number of Players=2/Play As Listen Server를 설정한다. G0 판정 시점 PIE 양쪽 화면은 NotRun이며 G1에서 전장 구도·입력 검수로 이어간다.
7. 통합 코드 완료 SHA와 미커밋 diff를 기록하고 Editor를 한 번 빌드한 뒤 `Test-P0Automation.ps1 -Filter LD.P0.G0`로 실제 전체 테스트를 실행한다. 개별 역할4Pass와 통합 suite Pass를 별도 로그로 남긴다. 실패가 있으면 원인·수정 SHA·해당 회귀 결과를 연결한 뒤 독립 리뷰를 다시 받는다.

관찰할 흐름은 서버 `InitGameState→LoadP0→GameState 초기화→Processor 생성/초기화`, 참가자 `PostLogin→서버 문맥 발급→PlayerState/Processor 등록→Controller 연결`, 응답 `Controller Server RPC→Processor 캐시/검증→소유자 Client RPC→MatchId/Epoch/RequestId 대조`다. 공용 Phase는 서버 GameState, 명령 기록은 서버 Processor, 로컬 PendingCommand는 소유 Controller에 각각 한 원본만 둔다.

## 독립 리뷰와 실패 원인

| 재현 조건 | 기대 결과 | 발견한 원인·수정 방향 | 현재 증거 |
|---|---|---|---|
| ContinuousSeconds=3, Timing.Order 변경, P0 허용목록 변조 | 같은 버전의 지원하지 않는 정책 거절, 원본 미게시 | A 초기 로더가 일부 정책을 읽지 않았음. Require 검증과 필드별 mutation 추가 | A 완료소스 및 Data 자동화에서 수정 확인; 실제 로그는 A 수업 링크 |
| 같은 X중심 중복 또는 두 보드 Y중첩 | 구체 좌표 오류로 로딩 거절 | 개수·유한값만 검사하던 조건을 CellSize 간격·중앙 분리 조건으로 보완 | A 완료소스와 실패 주입 테스트에서 확인 |
| 캐시 응답 확정→매치 서비스 종료→같은 요청 재전송 | 원래 결과 유지, 새 요청은 거절 | 종료 시 Controller 문맥까지 해제하던 문제를 `a02efc1`에서 접수 종료/Logout·EndPlay 최종 해제로 분리 | TerminalSessionReplay 실제 UE Success, 독립 재리뷰 해소 |
| PostLogin→필수 데이터/서비스 준비→중복 등록 | 늦은 준비 뒤 정확히 한 번 연결, ID/세대 보존 | 미준비 PostLogin 누락을 `a02efc1`에서 pending weak Controller·공통 멱등 등록으로 수정 | LoginReadinessOrders/PendingLoginLogout 실제 UE Success, 독립 재리뷰 해소 |

초기 `LDLifecycleTests`는 GameState/PlayerState를 직접 생성한 전이 검사여서 GameMode→Controller 경유 결함을 검출하지 못했다. 수정 후 추가3개는 실제 UWorld/GameMode/Controller를 만들고 엔진 PreInitializeComponents→제품 InitGameState·로더·접수 연결을 통과한다. TerminalSessionReplay는 종료 뒤 원응답뿐 아니라 내용 변경 시 RequestIdConflict도 확인하므로 단순 PhaseNotAllowed 반환을 통과시키지 않는다. 독립 재리뷰에서 차단 결함0을 확인했다([정식 리뷰](../../docs/production/evidence/RUN-20260918-G0/REVIEW.md)). SubmitServerCommand는 실제 Server RPC와 공유하는 권한 검사 경계지만 테스트가 RPC 전송·owner 복제까지 실행하는 것은 아니다. NullRHI 월드 테스트의 통과를 별도 프로세스 네트워크·PIE 화면·Android 검수로 확대하지 않는다.

## 전달·재현 상태와 다음 의존성

G0 canonical 통합 SHA는 `649c1dedd6832c41089a76b59bc76518cd262296`다. 실제 학습 통합 SHA는 미생성이고 모든 실제 학습 진도는 Planned다.

| 검증/반영 | 식별점 | 실제 결과·근거 |
|---|---|---|
| 통합 수정 소스 | `a02efc1f55d8a49f93f46d7d058c73f71a4255ae` | 종료 캐시·준비 순서2건 수정; 새3개 회귀 추가 |
| 통합 Editor 실행 | HEAD `efb7ed77a10415976198319f0dbb9c8499661a51` | Pass/exit0, [원본 결과](../../docs/production/evidence/RUN-20260918-G0/integration-editor-final.json) |
| 통합 자동화 실행 | HEAD `379751719f5172ec12208e10397877902781bac9` | 12Success/0Fail/0NotRun, [원본 결과](../../docs/production/evidence/RUN-20260918-G0/integration-tests-final.json) |
| A/B 독립 수업 재현 | 출발점+명시 제공/역할 파일, HEAD는 출발점 유지 | A21/B27파일 조립해시 일치, 각각 Editor Pass·자동화4Success, [manifest·결과·환경](evidence/G0_REPLAY/SUMMARY.md) |
| canonical A 반영 | `4787bf1a3a0d866aa206d148586594b3c710f957` | canonical과 Source/Config/Content diff0 |
| canonical B 반영 | `9129c016efd4a652501f93658fd2831a830c8ab1` | canonical과 Source/Config/Content diff0 |
| learn3개 보존 | 모두 `8c6856d235de87cc28c12b49ca775bd0937334a5` | 완성 코드 병합 없음; 학습자 완료 이력 없음 |

Editor→자동화→canonical 사이의 Source/Config/Content가 동일함을 대조했다. 단독 A/B 완료소스 조립 결과와 통합 canonical 반영점을 섞어 기록하지 않는다. 2026-09-27 후속 재현에서 A-02/B-01/B-02도 각 native 경로의 실제 PIE를 확인해 Verified다. 조립은 참고 코드 재현이며 학습자의 직접 작성 증거가 아니다.

구조 리뷰는 ARCH-01 책임, ARCH-02 연결 경계, ARCH-03 상태 원본, ARCH-04 준비 전 성공 금지, ARCH-05 종료/초기화 멱등과 GC 참조, ARCH-06 구현과 독립된 실패·경계 검사를 위 실코드 경로로 확인했다. 종료 캐시·늦은 초기화 결함은 수정·실제 회귀·독립 재검토를 통과해 G0 코드 게이트가 Pass다. 아래 후속 PIE는 실제 네트워크 기반만 검증하며 전장 표시·패키지·Android는 각 게이트의 증거를 사용한다.

## native canonical 실제 네트워크 재현

시작/제품 완료점은 위 canonical `649c1dedd6832c41089a76b59bc76518cd262296`이며, 별도 detached `C:/Users/iam12/P0_lesson_replay_g0_integration`에서 실행했다. 제공 검사기 `5f1f08606f87e264b5ff0a12d957dbc7720f7e6c`만 추가하고 Editor 의존 및 `LD_G0_CANONICAL_PIE=1`을 적용한다. [공통 제공 절차](../tools/G0Replay/README.md)와 [정확한 설치·실행 순서](evidence/G0_REPLAY/B-network.md)를 따른다. 런타임 원본과 검사용 추가 파일을 혼동하지 않는다.

`LD.PIE.G0.Canonical.TerminalCache`의 실제 listen/client 두 월드는 소유 RPC→응답→재전송을 수행했다. 잘못된 epoch/응답을 무시하고, 송신100%손실의 Pending 유지 뒤 같은 요청 재시도, 응답 예산 소진 뒤 회복을 확인했다. 종료 뒤 기존 id3 전체 원응답 유지, 같은 ID의 다른 내용 Conflict, 새 id4 PhaseNotAllowed를 실제 전송에서 확인했다. [Editor54.72초 Pass](evidence/G0_REPLAY/canonical-native-editor.json), [PIE1Success/0Fail](evidence/G0_REPLAY/canonical-native-pie.json), [관찰·상태/패킷/관찰자 복원](evidence/G0_REPLAY/canonical-native-proof.json). NavMesh 재구성/CrowdFollowing 경고 이벤트2개를 남긴 경고 성공이며, 무경고로 기록하지 않는다.

native TopDown에는 G0 전용 HUD가 없으므로 실제 두 참가자의 복제·소유·응답 관찰을 예상 결과로 삼는다. 수동 Blueprint 변형, 완성된 전장, 별도 프로세스 패키지와 물리 입력은 이 재현의 범위가 아니다. 원본 source649c와 제공 fixture5f1을 이 순서로 재현한 범위에서 통합 수업을 Verified로 판정하고, 실제 학습자의 SHA는 미생성으로 유지한다.

이해 확인: 왜 Processor가 캐시를 보존해도 Controller 연결을 끊으면 멱등 응답이 깨지는가? 왜 BIndependentLoader 테스트 이름만 보고 통합에서 B 로더를 사용한다고 단정할 수 없는가? 작은 변형: 학습자 테스트 픽스처에서 초기화 호출 순서를 바꾸고 참가자 ID/세대가 한 번만 정해지는지 확인한다. 제품 규칙이나 역할 기준 SHA를 이 실습으로 바꾸지 않는다.

G1 후속: [두 경로와 양쪽 입력 통합 수업](G1_INTEGRATION.md), [시작점 재현 증거](evidence/G1_REPLAY/SUMMARY.md). G1은 df8a2f27의 실제 양쪽7화면비·전체 입력·두 바퀴를 통과했다. G2 전투·경제는 이 통과 뒤 진행한다.
