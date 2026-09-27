# 명령 시각과 최종 보상 확정 경계 — G3 / B / 03

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01, TASK-TEST-01; [공통 수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [코딩 규약](../../../docs/technical/CODING_STANDARD.md) |
| 참고 자료 제작 상태 | Draft — 실제 서비스/Mode 경계·패키지 중복 RPC·회복 Pass, 필수 부하 진행 중 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / 조립 `0981d07307112857dfdf0e91c79bcecdbcbc291b`; C++ `0e473f4`, cook Config `98727f0` ([공통 입력](README.md#g3-evidence), 게이트 완료 미확정) |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A GameMode의 명령 이전 시계 진행과 보류된 결과 확정 |
| 제공 코드 / 직접 작성할 코드 | G2 Processor·보상/경제 서비스 제공. 아래 Processor의 clock 완료 경계·delegate 수명 처리를 직접 작성. 검사는 제공 코드 |

실행별 SHA·새 재현본·증거 경로는 [B 공통 검증 기록](README.md#g3-evidence)을 따른다. 이 수업은 UI 표시보다 앞선 서버 보상 확정 순서를 확인한다.

## 이번에 만들 동작

명령 도착 시각보다 앞선 전투를 진행하다 최종 보스를 처치하면, 최종 보상이 먼저 경제 원본에 반영되고 그 다음 결과를 게시·종료한다. 이후 도착한 구매는 거절된다. 같은 요청 재전송은 원래 거절을 돌려주고 결과나 보상을 다시 발생시키지 않는다.

G2의 `BeforeExternalCommand`는 외부 콜백 재진입을 막는 `bProcessing` guard 안에서 실행한다. 따라서 콜백 안에서 `Close`하면 아직 큐에 있는 처치 보상이 지워진다. 새 `AfterExternalCommandClock`은 의존 방향을 뒤집지 않고 조립자인 GameMode에게 올바른 종료 시점을 알려준다. Processor는 웨이브/승패 규칙을 알지 않는다.

## 코드 작성 순서

1. `Network/LDCommandProcessor.h`: `FSimpleDelegate AfterExternalCommandClock`를 추가한다.
2. `.cpp SubmitAtTime`: Before guard 종료→`DrainCombatRewards`→After→`FindSession` 재조회 순서로 연결한다. 보상 또는 After의 외부 콜백이 세션을 바꿀 수 있으므로 이전 Session 포인터를 재사용하지 않는다.
3. `.cpp Close`: Before와 After delegate를 모두 해제한다. 기존 확정 캐시 보존과 새 접수 차단은 유지한다.
   `c697c91`에서는 After를 지역 `FSimpleDelegate`로 복사한 뒤 호출한다. 그 콜백이 Close를 호출해 원본 delegate를 해제해도 현재 callable의 수명이 끝나지 않게 한다.
4. 제공 `Tests/LDEntryAndHudTests.cpp`의 `RewardBeforeTerminalClose`를 읽는다. 실제 Board/Economy/Processor를 만들고 Before에서 B01 처치 사실을 큐에 넣고 admission만 닫는다. After에서 기대 gold200/stars3을 확인하고 Close한다. 최종 구매·중복 요청 후 population0/보상1회/Finalizer1회를 확인한다.
5. A GameMode는 Before에서 시간 경계와 보류 terminal을 결정하고, After에서 보상 확인 후 Result 게시·서비스 종료를 수행한다. 일반 고정 tick 경로도 Drain 후 같은 확정 함수를 호출한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | native `ULDCommandProcessor` | 런타임 GameMode가 Before/After 바인딩 | Blueprint에서 승패/경제 계산하지 않음 |
| 2 | `L_P0` | 기존 GameMode/2인 설정 유지 | 마지막 처치 보상 표시 후 결과 |

새 Blueprint·UMG·맵·데이터 설정은 없다. [HUD 수업](G3-02-battle-result-hud.md)과 A 결과 수업이 표시를 담당한다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| Before에서 보스 처치와 terminal 감지 | 보상 큐 유지·admission만 닫음 | 실제 서비스 자동화 Pass | `LD.P0.G3.Commands.RewardBeforeTerminalClose` |
| After 실행 | gold200/stars3, 이후Close | Pass | B 현재 전체57자동화의 위 검사 |
| 같은 요청 재전송 | PhaseNotAllowed 원응답, population0·보상/Finalizer1회 | Pass | 같은 검사; 네트워크를 거치지 않은 실제 Processor 호출 |
| 보스 마감·같은 시각 판매 경계 | 열린/닫힌 시각 계약대로 타격·보상·종료 순서 확정 | 실제 Mode/Combat 자동화 Pass | `LD.P0.G3.Waves.ProductionClockBossBoundaryAndSale`; 패키지에서 정확히 마감과 같은 시각의 타격/판매를 별도로 주입한 증거는 아님 |
| 실제 RPC의 같은 소환 키20회 이상 재전송·다른 payload | 원응답 동일·효과 중복 없음, 변경 요청 Conflict | 최종5시드 실제 수신/송신 로그62검사 Pass. 첫 소환 응답 host23/client24회 모두 원래 signature 동일 | [실제 RPC 로그](../../../docs/production/evidence/RUN-20260918-G3/package-five-seeds-wire.json), 최종 보드/재화 대조는 별도 실행 검사 |
| 서버 Result 이후 과거 소환 키 재전송 | 종료 뒤에도 원응답 그대로, 재화/보드 재적용 없음 | Pass: 서버 terminal 로그 뒤 원 SERVER 응답과 대응 CLIENT 도착을 확인 | 같은 PC 시각 기준. 클라이언트 화면 Result 표시 직후 정확한 타격 경계 검수로 확대하지 않음 |
| 지연·손실 후 정상 회복 | Pending 재시도에도 한 결과로 수렴 | 실제 패키지 회복1판 Pass, client 재시도17회·최종 양쪽 상태 일치 | [공통 회복 결과](README.md#g3-evidence); 실제 in-flight Pending의 세부 분기는 별도 자동화 |

Before 안의 Close 위험은 G3 조립 전에 A가 guard/Close 코드를 대조해 발견한 정적 결함이며 실제 패키지에서 관측한 실패로 바꾸어 기록하지 않는다. 보상 기대값은 규칙에서 독립적으로 gold200/stars3으로 정한 뒤 실제 Board/Economy/Processor와 비교했다. 수정 후 새 재현본의 기존56자동화와 현재 `0e473f4` 통합/B 전체57자동화가 각각 통과했다. 이후 Entry3종 재현·최종66파일·실제 PIE·G2 회귀·패키지 네트워크는 [공통 기록](README.md#g3-evidence)의 입력/범위로 구분한다.

최종 패키지는 정상 시계/규칙의5시드 모두 Wave10 BossTimeout 패배였고, 종료 보스 HP·일반 적0·양쪽 개인 보드/경제 상태가 일치했다. 실제 네트워크 중복/충돌/서버 종료 후 캐시 응답을 확인했지만 자연 승리나 마감 시각에 정확히 맞춘 타격을 성공시킨 검수는 아니다. 정확한 시간 경계·최종 보상 gold200/stars3의 근거는 위 실제 서비스/Mode 자동화에 남기며 패키지 증거와 합쳐 하나의 실행처럼 쓰지 않는다.

후속 A01/A02 리뷰는 외부 observer가 종료를 재진입할 때 승인된 처치 보상 또는 마지막 실제 보스 HP가 최종 결과에 빠지는 문제를 실제 실패 자동화로 확인했다. A 수정·재검증과 현재의 `CommittedDeathBeforeObserverAbort`/`CommittedBossHPBeforeObserverAbortResult` Pass는 [독립 리뷰 기록](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)에 연결한다. B는 이 규칙을 Processor에 복제하지 않고 Drain 후 A finalizer를 호출하는 경계를 유지한다.

## 상대에게 전달하고 통합하기

`8405a93` API를 A/B 모두 먼저 받고 `c697c91`의 callback 수명 보완을 연결한다. A는 Before에서 Close하지 않고 결과를 보류한다. 현재 조립 `0981d073`에는 A 승인 처치/마지막 HP 수정과 B 검사의 C++ `0e473f4`가 포함되며 cook Config까지 함께 준비한다. `AfterExternalCommandClock`은 매치 조립 때 한 번 바인딩하고 종료 때 해제한다. 통합 순서는 실제 서비스/Mode 경계 자동화→패키지 결과/최종 일반 적0·중복 요청→지연/손실 회복→대표 부하/수명이다. 앞의 세 실행 범위는 통과했고 부하는 진행 중이다. 실제 학습자 브랜치에는 참고 완성 코드를 합치지 않는다.

## 이해 확인

1. 단순히 `bProcessing`을 없애면 어떤 콜백 재진입 문제가 생기는가?
2. `Close` 전에 Drain을 호출해도 Before guard 안에서는 보상이 반영되지 않는 이유는 무엇인가?
3. 작은 변형: 최종 보스 대신 N01 처치 사실을 넣고 기대 gold101/stars0로 검사를 바꾼다. 제품 규칙 파일은 수정하지 않는다.

## 단계 완료

- [x] 작은 변경의 설계 이유·정확한 호출 순서·독립 기대값을 적었다.
- [x] 실제 서비스·A 타임라인 자동화를 통과했다.
- [x] 새 수업 조립·현재 소스 SHA·실행별 입력 근거를 연결했다.
- [x] 수정 후 패키지에서5시드·실제 중복/충돌/종료 후 원응답·회복과 최종 상태 일치를 재현했다.
- [ ] 패키지에서 정확한 마감 시각의 타격/판매·최종 보스 처치 보상을 별도 재현했다.

남은 범위는 위 정확한 경계의 패키지 재현과 진행 중인 대표 부하20분 결과 연결이다. 패키지5시드·반복 매치·지연/손실 회복 통과가 이 별도 경계 검수를 대체하지 않는다. PKG01 실패/수정/실제 재검증은 [Entry 수업](G3-01-entry-return.md)에 기록한다. Android는 NotRun이며 필수 게이트 판정 전 Draft를 유지한다.
