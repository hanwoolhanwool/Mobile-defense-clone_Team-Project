# 보드·경제·전투의 서버 연결 — P0 / A / G2-A-03

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-BATTLE-01 / [공통 수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md#lifecycle) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작/코드 SHA | `4861b987f3e2fe78bcc159d1b6a85008543a938b` / 조립 `8356a211c1cbbec29ed04a7694e9325e3e2111a7`, 시각 수정 `eafc378163f3d9ed939821d153228f9b9d29943e` |
| 실제 개발 시작/완료 SHA | 자기 G1 통합 결과 / 미생성 |
| 상대 산출물 | B Board/Economy/Processor/Controller `84389ce`(A 수신 `f800d70`), A CombatService `28ead1bc` |
| 제공 / 직접 작성 | 제공: G0 매치/참가자 수명, G1 맵. 직접 작성: Core/LDGameMode.*의 서비스 조립·통지·20Hz 호출 |

## 이번에 만들 동작

검증된 데이터와 두 참가자가 준비되면 명령을 열고 전투를 실행한다. 보드 통지는 전투에, 사망 사실은 경제 내부 큐에 연결한다. G1 명시적 probe는 Preparing과 명령 닫힘을 유지한다. G2 일반 실행에는 자동 웨이브 생성·승패·결과가 아직 없다.

## 코드 작성 순서

1. GameMode.h에 세 서비스 UPROPERTY와 delegate handle·LogicTimer를 선언한다. GameState/PlayerState에 경제 원본을 중복 추가하지 않는다. 개인 복제는 B Controller owner-only snapshot이다.
2. InitGameState에서 기존 데이터/문맥/Processor 초기화 뒤 Board/Economy/Combat을 생성하고 Initialize/BindServices를 모두 확인한다. 실패하면 Aborted로 정리한다. 서버 `-P0Seed=<int>`가 있으면 재현 seed로 사용하고 없으면 MatchId에서 seed를 만든다. seed는 서버 로그에만 남긴다.
3. RegisterParticipant는 세션을 연결한 뒤 Board/Economy snapshot을 소유 Controller에 게시한다. BeginPlay와 두 참가자·서비스 준비가 모두 끝나야 Running/명령 접수가 열린다.
4. RefreshReadiness에서 명시적 `-P0Probe=G1`은 준비 상태로 유지한다. G2에서는 GameRules.LogicHz=20 타이머를 연결한다. 정수 LogicStep으로 누락된 .05초 시각을 순서대로 계산하여 누적 오차를 피한다.
5. HandleBoardCommitted는 MatchId/PlayerIndex/Revision과 현재 Board snapshot을 확인한다. 제거를 먼저 Combat에서 해제하고 추가/갱신 ID는 Board의 확정 actor 조회를 거쳐 등록한다. 별도 actor 검색이나 원점 fallback을 쓰지 않는다.
6. Processor.BeforeExternalCommand를 AdvanceBeforeExternalCommand에 연결한다. 명령시각보다 이른 전투 사건을 처리한 뒤 Processor가 재진입 guard를 해제하고 보상을 Drain하여 재화/보드 검증보다 먼저 반영한다. 동일 시각 사건은 남겨 명령 우선 순서를 지킨다. 타이머의 오래된 스텝은 전투 시계를 되감지 않는다.
7. HandleEnemyDeath는 Processor.EnqueueCombatReward로 사실만 전달한다. AdvanceLogic의 각 스텝 말에도 DrainCombatRewards한다. EconomyChanged는 소유 Controller snapshot을 다시 게시한다.
8. StopMatchServices는 새 명령 닫기→기존 보상 Drain/Processor Close→사망 구독 해제/Combat Stop→보드/경제 구독 해제·Close다. Controller의 확정 응답 캐시는 종료 상태에서도 유지하고 EndPlay 때 세션을 해제한다. Running 참가자 이탈은 Aborted로 정리한다.

ARCH-01~06: GameMode는 서비스 구성·수명·통지만 맡고 피해/배치/재화 계산은 해당 서비스가 가진다. 모든 변경은 서버 경로이며 복제 snapshot은 표시 창구다. 확정 전에 UI 성공 이벤트를 보내지 않고 명시적 handle과 타이머를 정리한다.

## Unreal 설정 순서

| 순서 | 위치 | 값·연결 | 예상 관찰 |
|---|---|---|---|
| 1 | `/Game/LD/Maps/L_P0` | 기존 native LDGameMode 사용, 공유 맵 변경은 통합 담당자만 | 두 참가자 연결 |
| 2 | 서버 실행 인자 | G2 probe는 통합 담당자가 제공; `-P0Seed=1`은 선택적 QA 재현값 | 같은 참가자별 RNG 시작값 |
| 3 | G1 회귀 실행 | `-P0Probe=G1` | 전투·명령 닫힘, 기존 경로 fixture만 동작 |
| 4 | GameMode 서비스 연결 | Board.OnBoardCommitted / Economy.OnEconomyChanged / Combat.OnEnemyDeathCommitted | 첫 소환→등록→처치→두 개인 재화 |
| 5 | UMG·Blueprint | A 신규 에셋 저장 없음, B HUD는 Controller snapshot 구독 | 돈/개체수·실패 피드백은 B 수업 참조 |

## 실행·실패·수정 기록

| 조건 | 기대 결과 | 실제 결과 |
|---|---|---|
| 준비·두 참가자 | 정상 G2만 Running/명령 수락 | 소스 연결 완료, 실제 실행 NotRun |
| 같은 보드 commit | revision 중복 무시, 기존 공격 타이머 보존 | 소스 검토, 통합 자동화 대기 |
| 죽음/중복 보상 | 양쪽 개인에게1회, 다음 외부 명령 전 반영 | B 중복 방지에 연결, 실제 한 사이클 NotRun |
| 종료/이탈·반복 정리 | 새 공격/명령 거절, 예약·구독·보드 actor 정리 | 소스 연결 완료, 실제 반복 매치 NotRun |
| G1 회귀 | Preparing/명령 닫힘·양쪽 경로 유지 | 명시적 분기 작성, 재실행 대기 |
| 스타일/공백 | 오류0 | Pass,50파일; Unreal 실행 증거 아님 |

개발 중 상대 서비스가 없던 G0는 명시적 Stub이었다. 이번에는 B 실제 서비스 헤더·구현을 받은 뒤 연결했으며 성공을 흉내 내는 Stub을 넣지 않았다. 정식 런타임 생성/웨이브는 G3의 후속 의존성이며 G2 probe의 정지 적은 별도 검증 fixture로 기록한다.

최초 통합 Editor와 A 전투7자동화는 통과했지만 전체 검사는 G0의 낡은 Stub 문구 기대값1개로 실패했다. 문구 전체 비교 대신 준비 Phase/참가자 수를 확인하도록 바꿨다. 독립 리뷰는10.04초 RPC 판매가10.025초 예약 공격보다 먼저 적용되는 시각 결함도 발견했으며 위 hook으로 수정했다. 실행 근거·첫 실패·후속 재검증 상태는 [공통 G2 증거](G2_EVIDENCE.md)에서 관리한다.

## 상대에게 전달하고 통합하기

순서: B 공통 타입→A Unit/Events→B Board/Economy/Processor→A Combat→GameMode 연결→통합 담당자의 로컬 표시/fixture→Editor·자동화→실제 첫 소환부터 양쪽 보상/이동/합성/판매를 확인한다. 권한 있는 서버 fixture는 GetBoardManager/GetEconomyService/GetCombatService/GetCommandProcessor를 사용하고 제품 데이터나 UI를 전투 원본 수정 경로로 만들지 않는다.

## 이해 확인

- 같은 보드 Revision 재전달을 actor 재생성으로 처리하면 어떤 개체 상태가 사라지는가?
- Result를 먼저 게시하고 보상을 나중에 Drain하면 어떤 마지막 보상이 유실되는가?
- 작은 변형: 자동화에서 같은 commit을 두 번 보내 등록 수/NextAttackAt을 비교한다. 네트워크 성공을 해당 모의 검사로 대체하지 않는다.

## 단계 완료

- [x] 시작점·파일·서버 연결·상대 계약을 기록했다.
- [ ] 새 작업 경로에서 수업을 재현했다.
- [ ] Editor·자동화·실제 두 프로세스 실행과 SHA를 연결했다.
- [x] 미검증은 위 모든 실행·반복 매치·패키지·성능·Android다. G2 통과와 수업 재현 뒤에만 Verified/다음 게이트를 기록한다.
