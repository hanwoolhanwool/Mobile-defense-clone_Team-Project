# 새 매치의 명령 세대 — P0 / A / G3-A-04

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-WAVE-01의 종료/재진입, [공통 명령·수명 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [코딩 규약](../../../docs/technical/CODING_STANDARD.md) |
| 참고 자료 제작 / 실제 개발 상태 | **Verified — 아래 명시한 PC 재현 범위** / Planned |
| 참고 시작 / 완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / `f64cc671848560923595cc1955efe12620f326de` (제품+검사 소스). 실제 패키지 입력은 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`; 마지막 차이는 테스트 파일만 |
| 실제 개발 시작 / 완료 SHA | 자기 G3-A-03 통합 결과 / 미생성 |
| 상대 산출물 | B의 Controller 소유 RPC, Processor 세대 검증/완료 캐시, Entry/GI의 결과 복귀와 새 접속 |
| 제공 코드 / 직접 작성 | 제공: G2/B 계약과 `Tests/LDLifecycleTests.cpp`, `Tests/LDPieTests.cpp`, `Verification/LDG3BoundaryProbeSubsystem.*`, 실행 도구. 직접 작성: `Core/LDGameMode.cpp` 발급기·등록 연결 및 `LDGameMode.h`의 World별 카운터 제거 |

전체 코드/실행 수치와 재현 입력은 [G3 A 공통 기록](G3_EVIDENCE.md), 실패→수정 원본은 [정식 리뷰 NET-LIFE01](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)에 둔다. 자동화 통과를 패키지 또는 이 문서 절차의 재현 완료로 바꾸지 않는다.

PC Verified는 [통합 수업의3단계 재현](../G3_INTEGRATION.md)과 [최종 PC 보충 증거](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md)에 연결한 실행 범위다. 선택 변형 과제·미관찰 입력 조합은 완료로 올리지 않는다. Android G4와 실제 청음은 NotRun이며 P0 최종 완료가 아니다. 실제 학습자는 Planned를 유지한다.

## 이번에 만들 동작

첫 매치를 종료한 뒤 같은 서버 프로세스에서 새 매치를 시작한다. 이전에 실제 성공했던 첫 소환 명령을 **새 소유 Controller로** 다시 전송하면 InvalidEpoch를 받는다. 새 보드·골드·유료 소환 수·난수·결과 캐시는 그대로이며, 새 세대의 첫 정상 소환은 성공한다. 결과 화면을 다시 만들어도 새 명령 접수나 이전 매치 callback이 살아나지 않는다.

이는 오래된 Actor의 NetGUID를 대상으로 늦게 도착한 reliable 패킷이 새 Actor로 자동 전달된다는 가정이 아니다. 검사 대상은 애플리케이션이 보관한 **이전 payload를 새 Controller에 재전송하는 경우**다. RequestId가 다시1이 되는 것은 허용하지만, 그 RequestId를 해석하는 세대는 재사용하지 않는다.

## 코드 작성 순서

1. 변경 전에 `Network/LDCommandTypes.h`의 `FLDCommand`를 읽는다. MatchId는 없으며 ConnectionEpoch/RequestId/ExpectedBoardRevision와 고정 길이 명령 데이터만 전달한다. `LDPlayerController::SubmitServerCommand`가 신뢰하는 현재 ServerContext를 붙이고, `LDCommandProcessor::SubmitAtTime`이 현재 세대와 요청 세대를 비교한다. 가격·RNG·플레이어 정체성은 클라이언트가 결정하지 않는다.
2. `Core/LDGameMode.h`에서 World마다 초기값1로 재생성되는 NextConnectionEpoch를 제거한다. 이 값이 GameState에 공개된 게임 상태인 것처럼 별도로 복제하지 않는다.
3. `Core/LDGameMode.cpp`의 익명 namespace에 GameThread 전용 `AllocateConnectionEpoch()`를 작성한다. static uint64의 수명은 매치 World보다 긴 서버 프로세스다. 최초 값은 새 FGuid 일부로 만들고0이면1로 바꾼다. 이후 발급 때마다 증가한다. 경제 RandomStream을 쓰지 않아 식별자 발급이 소환 추첨 상태를 소비하지 않는다.
4. uint64가 소진되어0이 되면 계속0을 반환한다. 1로 되감지 않는다. 기존 ParticipantContext/PlayerState 검증이0을 거절하고 초기화를 중단하도록 기존 실패 경로를 유지한다. 프로세스 내 재사용을 막는 계약이며, 프로세스 재시작 간 충돌 감소는 확률적이다. 전역 영구 식별자 시스템이나 저장 기능을 추가하지 않는다.
5. `RegisterParticipant`가 초기화·인원·기존 등록 검사를 통과한 뒤 이 발급기를 호출하도록 연결한다. 동일 Controller의 중복 PostLogin/등록은 기존 context를 유지한다. 새 World/재접속은 새 세대이고, Result 후 동일 매치의 완료 캐시는 연결 종료까지 보존한다.
6. B의 Controller/Processor 계약을 변경하지 않은 채 제공 회귀 `LD.P0.G3.Lifetime.PreviousMatchRequestRejected`를 읽는다. 실제 첫 World에서 성공한 payload를 저장→종료→새 World/빈 보드→같은 payload 재전송 순서다. 새 세대가 단순히 더 큰 작은 정수라고 가정하거나 fixture에서 임의의 틀린 epoch를 만들면 이 결함을 검증하지 못한다.
7. 제공 Boundary에서는 실제 RPC 도착을 `P0CommandTrace`로 확인한 뒤 별도 owner 관찰 채널로 서버 보드/경제/cache/RNG 불변을 확인한다. 이 ACK가 오기 전에 새 구매를 하지 않는다. uint64 epoch/개체 ID는 JSON 문자열로 보존한다. double로 변환한 값을 세대 비교에 사용하지 않는다.

호출 흐름은 `새 World.InitGameState → PostLogin/준비 확인 → RegisterParticipant → process 세대 발급 → PC 서버 context/개인 복제 → 클라이언트 명령 → 현재 소유 PC의 ServerRequestCommand → Processor 세대 비교 → 성공 실행 또는 부작용 없는 거절`이다. 세션은 PC/Processor가 관리하고, 공개 전투 상태는 GameState가, 보드와 재화는 각각의 서비스가 계속 소유한다.

구조 선택은 작은 process 수명 발급 함수다. 새로운 Manager/Subsystem·DTO MatchId 필드·전역 경제 원본을 도입하지 않는다. 명령 경계에서 정체성을 검사하고, World 종료가 생명주기가 더 긴 식별자 발급기를 초기화하지 않도록 수명을 맞춘다. 상세 ARCH-01~06 검토는 정식 리뷰에 연결한다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `/Game/LD/Maps/L_P0`, `/Game/LD/Core/BP_LDGameMode` | 기존 native LDGameMode 부모와 클래스 설정 유지. 발급기는 C++ 내부 함수로 BP 프로퍼티 없음 | 같은 전장·UI, 제품 표시 변경 없음 |
| 2 | 기본 실행 | 기존 Entry에서 Host, 다른 프로세스에서 주소 Join. 포트는 통합 담당자가 비충돌 값 선택 | 두 개인 보드와 Loading→Preparing→Running |
| 3 | 결과 복귀 | 기존 native Result 버튼→B Controller→Entry/GI Travel 경로 | 동일 프로세스에서 새 MatchId/새 세대가 시작됨 |
| 4 | 실제 PIE 검수 | 제공 `Test-P0PIE.ps1`, 필터 `LD.PIE.P0.Session`; listen/client 실제 PIE World2 | Result/Status 재생성, 입력 차단, 종료 후 World0와 원래 Editor 설정 복원 |
| 5 | Development 패키지 보충 | 제공 `Run-P0G3Supplement.ps1 -Probe G3Boundary`, 새 RunId/양쪽 출력 폴더/포트 | 혼합 승패4매치·Return3회. `-P0CommandTrace`는 진단 로그만 제공 |

새 BP/UMG/맵 바이너리를 만들거나 Project Settings를 임의 바꾸지 않는다. 검수 도구가 소유한 Editor 설정은 종료 뒤 복원한다. native 위젯의 실제 계층·프로퍼티는 [G3-A-03](G3_03_WIDGETS.md)에 둔다.

## 실행·실패·수정 기록

| 입력/조건 | 구현과 독립적인 기대 | 현재 실제 결과·범위 |
|---|---|---|
| 첫 World에서 첫 유료 소환, 같은 World에서 같은 명령 중복 | 한 번만 소비, 동일 응답/개체; 중복 접속은 context 유지 | 기존 명령/수명 회귀. 현재 소스의 실행 결과는 SUMMARY에서 확인 |
| 종료→새 World→원래 성공한 id1/옛 epoch/revision0 재전송 | InvalidEpoch; gold100/인구0/n0/revision0/RNG/cache 불변 | NET-LIFE01 실제 수정 전 실패→`53af399` 후 자동화 통과. 최종73파일 재현과 패키지의 실제 구 payload InvalidEpoch·서버 원본 불변도 [최종 보충 검수](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md)에서 Pass |
| 새 세대의 첫 정상 명령 | 새 매치 소유 보드에서 소환1·gold80 | 최종 Boundary에서 구 payload 거절 확인 후 새 정상 소환·이동도 성공. 별도 소켓 RPC 증거는 [최종 보충 검수](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md) |
| 세대 발급 소진0 | 기존 context 검증 실패; 이전 세대 재사용0 | 코드 경로 검토. 실제 uint64 전체 발급 실행은 하지 않음 |
| Result 후 위젯 제거/재생성·S/M/X/터치/직접 intent | 표시1쌍/소유 복귀 구독1; 새 서버 요청0, 전투/보드/경제/선택 상태 불변 | 통합의 제공 실제 PIE 수명 검수는 정식 리뷰에 있음. 최종 재현본 실제 PIE에서도 Pass; 물리 키/터치 검사로 확대하지 않음 |
| 같은 두 PID에서4매치·옛 payload3회 | MatchId4개/Return3회, 각 새 세대 구분, 양쪽 스냅샷 일치 | e89a1fa 실제 패키지에서4사례·3회 Slate 복귀·옛 요청 거절·양쪽 상태/화면 Pass. [최종 보충 검수](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md) |

수정 전에는 GameMode마다 epoch가 다시1/2가 되어 새 Controller의 현재 context와 이전 payload가 같아졌다. Processor는 정상 새 요청으로 처리하여 골드·보드·난수·캐시를 변경했다. `62b5180` 실패 회귀→`53af399` 최소 수정의 원본 결과/수치는 정식 리뷰에 보존한다. 기대값을 구현 결과에 맞추거나 과거 payload 대신 임의 epoch를 써서 통과시키지 않는다.

제공 Boundary는 준비 잔여와 앞 웨이브를 건너뛰고, 보스/일반 HP를 비치명적으로1까지 낮추며, 원래 유닛의 공격 예정 시각을 명시 재등록한다. 한도 사례는 예약 일반 생성을 멈추고 실제 생성 함수로99→100을 만든다. 실제 World 시계/치명 타격/보상/결과는 제품 경로다. 이 입력 변경·원래 유닛 ID·Actor HP·예정/관찰 시각을 증거에 남기며, 자연 승리/밸런스 결과와 분리한다.

## 상대에게 전달하고 통합하기

A는 제품 `53af399`의 발급 수명 변경과 회귀 기대값을 B/통합에 전달한다. B는 기존 요청 DTO/Controller의 세대 수신·새 매치 로컬 RequestId 초기화·옛 응답 거절을 유지한다. `bb18616` 등 제공 Boundary는 같은 제품 버전에 붙여 검수하고 정상 실행에서는 생성하지 않는다. 상세 전달 커밋/보충 입력은 [공통 기록](G3_EVIDENCE.md)의 순서를 따른다.

참고 완료 제품+검사 소스는 표의 f64cc671, 실제 패키지 소스는 e89a1fa다. 학습자의 실제 통합 SHA는 미생성이다.73파일 해시 일치와3단계 재현은 공통 기록에 연결하며 제공 조립을 학습자의 직접 작성 완료로 기록하지 않는다.

## 이해 확인

- 공개 GameState의 MatchId가 달라도, wire payload에 MatchId가 없으면 어떤 다른 식별자의 수명이 필요할까?
- 같은 Controller의 중복 로그인, Result 후 완료 요청 재전달, 새 World의 옛 요청 재전송은 왜 기대가 다른가?
- uint64를 JSON 숫자로 쓰면 실제 값이 다른 두 epoch를 같다고 관찰할 수 있는 이유는 무엇인가?
- 경제 RNG로 초기 epoch를 만들면 소환 회귀와 어떤 불필요한 의존이 생길까?
- 작은 변형: 제공 회귀에서 첫 매치의 두 번째 정상 요청을 저장하도록 바꾸고, 새 매치 첫 구매 전에 거절/불변을 검사한다. 제품 규칙·DTO·세대 발급기는 바꾸지 않는다. 변형 입력의 기대 결과와 실제 값을 별도 RunId에 남긴다.

## 단계 완료

- [x] 파일 순서·API·수명 선택 이유·설정·제공/작성 구분을 기록했다.
- [x] 실제 실패→수정의 독립 기대와 정식 증거를 연결했다.
- [x] 시작 SHA의66파일 조립→문서화된73파일 보충→Editor/자동화/실제 PIE를 새로 재현했다.
- [x] 최종 패키지에서 원래 payload 재전송,4매치/3복귀,양쪽 JSON/RPC/화면/정리를 확인했다.
- [x] 최종 Source/완료 SHA와 실행 증거를 고정하고 독립 리뷰 후 Verified로 바꿨다.
- [x] 학습자는 Planned로 유지하고 Android는 별도 G4 NotRun으로 남긴다.
