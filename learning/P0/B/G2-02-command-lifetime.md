# 중복 요청과 연결 세대 수명 — P0 / B / G2-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01, [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [네트워크 16.2](../../../docs/technical/ARCHITECTURE.md) |
| 참고 자료 제작 상태 | Draft — 원자 명령·세대 교체 UE 검사 통과, 후속 만료/UI 대기 검사 및 수업 재현 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | 시작 `84389ce22205c8906592465be4d40e42434dda01` / 역할 최신 `f00f8fb27c514d9a44fddecaf624f7b378416662`, G2 완료 미정 |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | G2-01 서비스, A GameMode/Combat의 명령 시각 이전 전투 처리 |
| 제공 코드 / 직접 작성할 코드 | 제공: G0 요청 값·캐시 기반과 G2-01 서비스. 직접 작성: 실명령 공동 확정, 보상 큐, 콜백 세대 방어, 개인 Snapshot 도착 게이트 |

## 이번에 만들 동작

소환 요청을 다시 보내도 비용·추첨·배치는 한 번만 바뀐다. 게시 콜백에서 연결 세대가 교체되어도 이전 결과가 새 세대의 요청1 캐시에 들어가지 않는다. 응답보다 개인 상태가 늦게 도착하면 UI는 두 Revision을 기다린다. 기록이 만료된 요청은 자동으로 새 번호를 만들지 않는다.

전체 식별자·캐시·상태 소유권은 [COMMON](../COMMON.md)과 정식 계약을 읽는다. 이 수업은 코드가 외부 콜백을 호출하는 순간 포인터와 연결 문맥이 바뀔 수 있다는 차이에 집중한다.

## 코드 작성 순서

1. `Network/LDCommandProcessor.h/.cpp`의 `BindServices`는 같은 MatchId의 두 서비스만 받는다. `ExecuteCommand`는 보드 사전검사→경제 계획→보드 계획→두 최종검사→두 값 확정→원응답 캐시→게시 순서다. 외부 콜백은 공동 확정 안에 두지 않는다.
2. `SubmitAtTime` 입구에서 Context/Command를 값으로 복사한다. 호출자가 넘긴 Controller의 문맥을 게시 콜백이 바꿔도 이미 받은 요청 신원은 바뀌지 않는다. 캐시된 같은 내용은 원응답, 같은 키 다른 내용은 Conflict다. 처리 중 같은 키도 내용을 비교한다.
3. `BeforeExternalCommand(ServerSeconds)`를 처리 중 guard 아래 실행한다. A는 그 시각보다 엄격하게 앞선 공격만 먼저 확정한다. Processor가 guard를 푼 뒤 사망 큐를 drain하고 세션을 재조회한다. 동일 시각은 명령 우선이다. 캐시 재전송은 이 clock hook을 다시 실행하지 않는다.
4. `EnqueueCombatReward`는 사용자 RequestId와 별도의 서버 큐다. 진행 중 요청에는 재진입하지 않고 drain 단계에서 경제에 적용한다. 보상 게시 또는 보드 게시 후 Session 포인터를 재사용하지 않고 원래 Context로 다시 찾는다.
5. 확정 결과는 원래 세션에 게시 전에 저장한다. 게시 중 RegisterParticipant/Logout이 세션을 교체하거나 TMap을 재배치할 수 있기 때문이다. 이후 새 세대에 이전 결과를 저장하지 않는다. 종료는 신규 admission만 닫고 원래 캐시 재조회는 남긴다.
6. `Core/LDPlayerController.*`의 OwnerOnly `FLDOwnerGameplaySnapshot{ConnectionEpoch, Board, Economy}`를 단일 복제 envelope로 둔다. 경제 원본은 EconomyService이며 PlayerState에 경제 복제본을 더 만들지 않는다. `IsGameplaySnapshotReady`는 세대·MatchId·PlayerIndex를 모두 대조한다.
7. PC는 한 Pending만 유지한다. 성공 응답의 두 Revision 이상이 오기 전 다음 변형 입력을 막는다. 만료 응답에도 현재 두 Revision을 포함하고, 도착 뒤 선택/drag를 지워 사용자가 다시 선택하게 한다. 세대 교체는 이전 Pending을 새 번호로 재실행하지 않는다.
8. 응답 미확정 시 1초 간격 세 번만 자동 재확인한다. 이후 HUD의 같은 버튼은 `응답 재확인`으로 바뀌며 기존 Pending만 다시 보낸다. 비용 환불·실패·새 소환으로 단정하지 않는다.
9. `Tests/LDGameplayCommandTests.cpp`의 실제 게시 delegate에서 epoch를 바꿔 검증한다. 서버 캐시 만료 검사는 성공 요청 뒤 256개 후속 요청으로 실제 캐시를 축출하고 다음 보상 이후 Revision을 확인한다.

ARCH-03/04: Processor는 Board/Economy의 명시 API만 조립하고 GameMode가 A와의 의존성을 연결한다. ARCH-05: callback 전에 캐시가 확정돼야 하며 세션 주소를 수명 보장으로 오해하지 않는다. ARCH-06: 실제 delegate 재진입 검사와 별도 프로세스 네트워크를 구분한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | 기존 LDGameMode 네이티브 조립 | Board/Economy 생성 후 Processor.BindServices | 미연결 Stub 성공을 가정하지 않음 |
| 2 | GameMode Combat 연결 | BeforeExternalCommand→strict-before 전투, 사망→Enqueue | 판매 전에 이미 예정된 앞선 공격을 처리 |
| 3 | LDPlayerController 네이티브 | Server RPC Reliable, OwnerOnly envelope | 금액·RNG·신원은 클라이언트 입력에서 받지 않음 |
| 4 | Session Frontend | LD.P0.G2.Commands.EpochReplacementDuringPublication / ExpiredRequiresCurrentRevisions | callback/기록 만료의 실제 코드 경로 |

새 Blueprint·맵·타이머 에셋은 없다. UI pending 표시 연결은 다음 수업이다.

## 실행·실패·수정 기록

| 재현 상황·함수 | 영향·수정 | 실제 검증 |
|---|---|---|
| SubmitAtTime 처리 중 같은 키 다른 내용 | Busy가 아닌 Conflict. 원요청 내용 보존; `c20c209a4ef40a2e44702ebf8801401553030a66` | 이후 명령 5개 Pass |
| 보드 게시 delegate에서 epoch1→2 교체 | 기존 Session 포인터로 epoch1 결과가 새 캐시에 들어갈 수 있음. 문맥 값 복사·게시 전 캐시·콜백 후 재조회; `892f3334965f4e2071cefd458ae2da9c8f21b1ac` | EpochReplacementDuringPublication Pass: 새epoch 요청1이 두 번째 구매, 잔액58 |
| old Snapshot이 남은 상태에서 epoch OnRep | Match/Player만 같으면 UI가 너무 일찍 열림. envelopeepoch 대조; `22f31513f0e0b836200f2b161847e15e37410075` | 독립 정적 리뷰 해소, 실제 도착순서 검사 대기 |
| 마지막 전투10.0, 공격due10.025, 판매10.04 | 앞선 공격이 판매로 사라짐. strict-before hook `16ad92fa073d4bb7b5c8f4fc68fa11257a090f88`, A 정확 시각 처리와 결합 | A 통합 수정/실행 대기 |
| 캐시에서 요청1 축출 뒤 재조회 | 응답 Revision0과 즉시 UI 해제는 최신 상태를 보장 못함. 두 Revision 포함·동기화 후 재선택; `f00f8fb27c514d9a44fddecaf624f7b378416662` | 새 서버 회귀 추가, 실행 대기; 지연 Snapshot PC 검수 별도 |
| 응답 세 번 재시도 후 여전히 미확정 | UI가 영구 잠긴 채 끝나지 않도록 같은 Pending 수동 재확인; `64acff687b7b31ca39abb69bc99819016cacc75c` | 실제 UI 검수 대기 |

초기 근거는 [통합5개 명령 검사](evidence/G2-initial/commands-summary.json)다. 후속 통합 `cb6c631`의 [실제 UE 검사](evidence/G2-initial/commands-final-review-summary.json)는 전체34Pass/0Fail/0NotRun이며 명령6건에 새 ExpiredRequiresCurrentRevisions를 포함한다. 위 표에서 실행 대기로 기록한 서버 만료 회귀는 이 후속 실행에서 Pass로 갱신한다. 실제 PC의 응답/스냅샷 도착순서 및 사용자 입력·소리 결과는 이 NullRHI 검사가 보증하지 않는다. PIE, 응답 유실, logout/반복 매치의 새 결합 검수는 미실행이다.

## 상대에게 전달하고 통합하기

G2-01→c20→892→clock hook→envelope/UI→f00 순서의 변경을 통합한다. A가 예전843 Processor를 함께 가지고 있으면 그 파일은 변경되지 않았는지 diff를 확인하고 B 최신 이력을 유지한다. GameMode는 hook을 한 번 Bind하고 종료에 해제한다. Combat에서 큐로 전달한 사망은 Processor가 guard 뒤 drain한다. UI는 `GetLastResult`, `GetBoardSnapshot`, `GetEconomySnapshot`, `IsGameplaySnapshotReady`, `HasPendingCommand`를 읽는다. 실제 학습자는 자기 상대 산출물과 동일 API를 연결하고 자기 SHA를 기록한다.

## 이해 확인

- 같은 요청번호가 왜 다른 연결 세대에서는 새 요청일 수 있는가?
- delegate 이전에 저장한 TMap 내부 포인터를 이후에 사용하면 어떤 일이 생길 수 있는가?
- 작은 변형: 게시 콜백에서 epoch를 교체하는 대신 연결을 제거하고 원본 비용/생성 수가 여전히 한 번인지 확인한다.
- 다음 조건: A strict-before 시간 경계 검사, 서버 만료 회귀, 실제 PC snapshot 도착순서와 지연/중복 RPC 통과가 필요하다.

## 단계 완료

- [x] 단일 원본·콜백 수명·실패 원인/수정 SHA를 연결했다.
- [x] 세대 교체의 실제 UE delegate 경로를 확인했다.
- [ ] 최신 수정과 실제 네트워크 지연·응답 유실을 재검증했다.
- [ ] 수업 시작점 조립 재현을 완료했다.
- [x] 정적 리뷰·자동화·실게임 미검증을 구분했다.
