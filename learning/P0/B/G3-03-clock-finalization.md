# 명령 시각과 최종 보상 확정 경계 — G3 / B / 03

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-NET-01, TASK-TEST-01; [공통 수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [코딩 규약](../../../docs/technical/CODING_STANDARD.md) |
| 참고 자료 제작 상태 | Draft — 실제 자동화·타임라인 통합 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / 제품 API `8405a9395b5c512b11bc92e8337a7eec6281bb69`, 검사 `d2183ae8c809541da4602b99964f90fd22b50b93` |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A GameMode의 명령 이전 시계 진행과 보류된 결과 확정 |
| 제공 코드 / 직접 작성할 코드 | G2 Processor·보상/경제 서비스 제공. 아래 Processor 4줄 경계를 직접 작성. 검사는 제공 코드 |

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
| Before에서 보스 처치와 terminal 감지 | 보상 큐 유지·admission만 닫음 | 미실행 | A 코드 통합 필요 |
| After 실행 | gold200/stars3, 이후Close | 미실행 | `LD.P0.G3.Commands.RewardBeforeTerminalClose` |
| 같은 요청 재전송 | PhaseNotAllowed 원응답, 보상/Finalizer1회 | 미실행 | 위 자동화 |
| 실제 보스 마감 시각의 타격 | 정확한 시각 타격만 인정, 보상 후 결과 | 미실행 | A 경계 검사 + 실제 패키지 |

이 결함 가능성은 G3 조립 전에 A가 기존 guard/Close 코드를 대조해 발견했다. 아직 실제 게임에서 관측한 실패로 기록하지 않는다. 빌드·자동화·패키지는 통합 담당이 직렬 실행한다.

## 상대에게 전달하고 통합하기

`8405a93` API를 A/B 모두 먼저 받는다. A는 Before에서 Close하지 않고 결과를 보류한다. root는 `d2183ae` 검사와 A 실제 타임라인을 함께 빌드한 후 보스 마감/최종 잔여 적0/보상 표시 순서를 검수한다. 실제 학습자 브랜치에는 참고 완성 코드를 합치지 않는다.

## 이해 확인

1. 단순히 `bProcessing`을 없애면 어떤 콜백 재진입 문제가 생기는가?
2. `Close` 전에 Drain을 호출해도 Before guard 안에서는 보상이 반영되지 않는 이유는 무엇인가?
3. 작은 변형: 최종 보스 대신 N01 처치 사실을 넣고 기대 gold101/stars0로 검사를 바꾼다. 제품 규칙 파일은 수정하지 않는다.

## 단계 완료

- [x] 작은 변경의 설계 이유·정확한 호출 순서·독립 기대값을 적었다.
- [ ] 실제 자동화와 A 타임라인 통합을 통과했다.
- [ ] 새 수업 조립·최종 SHA·실행 증거를 연결했다.

미검증: 동시 명령과 보스 시간 경계의 실제 네트워크 실행. 이를 G3 실제 패키지 검수와 분리해 완료 표시한다.
