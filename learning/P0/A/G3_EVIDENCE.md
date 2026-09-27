# G3 A 공통 증거·전달 기록

참고 자료 제작 **Draft**, 실제 학습자 **Planned**. 시작 SHA는 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`이며 **최종 Source SHA·참고 완료 SHA는 미정**이다. A 초기 기능의 `0d358bc`와 기존 패키지의 `0e473f4`는 중간 입력이며 현재 수업의 완료 SHA가 아니다. learn 브랜치에 완성 코드를 병합하거나 제공 파일 조립을 학습자의 직접 구현으로 기록하지 않는다.

공통 규칙은 [작업 기록](../../../docs/production/P0_REFERENCE_RUN.md), [현행 전투 명세](../../../docs/design/BATTLE.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md)에 둔다. 실행 수치·환경·결함 상태의 원본은 아래 세 문서다. 수업마다 같은 표를 복사하지 않고 해당 기능의 관찰과 차이만 기록한다.

| 정식 근거 | 읽을 내용 | 이 수업에 적용하는 한계 |
|---|---|---|
| [SUMMARY](../../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md) | 컴파일·자동화·PIE·패키지·네트워크·학습 재현의 입력과 실제 결과 | 이전 소스의 성공을 보충 소스의 실행으로 승계하지 않음 |
| [REVIEW_FINDINGS](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md) | 독립 기대값, 실제 실패→수정, 정적 발견과 실행 실패의 구분, NET-LIFE01 | Unreal 회귀가 닫힌 결함도 패키지 재전송까지 실행했다는 뜻은 아님 |
| [PERFORMANCE](../../../docs/production/evidence/RUN-20260918-G3/PERFORMANCE.md) | 실제 20분 PC 패키지 부하·CSV·수거·측정 환경과 한계 | 명시 고정 부하의 결과. 자연 플레이·Android 성능·보편적인 무누수를 증명하지 않음 |

20분 부하는 이제 정식 증거에 기록되어 있다. 이를 진행 중으로 쓰지 않는다. 다만 후속 세대 수정과 보충 검사까지 포함한 **수업 절차의 새 조립·빌드·실행은 대기**이므로 G3 수업은 Draft다. Android 실기기는 별도 G4 NotRun이다.

## 수업 순서와 작성/제공 경계

1. [G3-A-01](G3_01_WAVES.md): 공용 DTO→Director의 생성·사망·승리 조건.
2. [G3-A-02](G3_02_TIMELINE.md): Mode/State의 단일 사건 순서→보상 Drain→종료.
3. [G3-A-03](G3_03_WIDGETS.md): 값으로 갱신하는 전투/결과 위젯→B의 생성·해제 연결.
4. [G3-A-04](G3_04_SESSION_LIFETIME.md): 매치를 넘어가는 명령 세대→종료·재진입·실제 네트워크 경계 관찰.

직접 작성할 제품 코드는 각 수업의 파일 순서를 따른다. G2 기반, B 담당 Processor/Controller/Entry, 데이터·맵·아트는 계약에 맞는 제공 입력이다. `Tests/LDPieTests.cpp`와 `Verification/LDG3*ProbeSubsystem.*`, 빌드/실행 도구는 **제공 검수 코드**다. 학습자는 기대값을 먼저 설명하고 실행하되, 제공 fixture를 만든 실적으로 쓰지 않는다. 필요한 런타임 파일은 Source/Content/Config에 있으며 learning 폴더를 로드하지 않는다.

## 출발점에서 재현하는 절차

공유 리소스와 Editor/패키지/포트는 통합 담당자가 순차 사용한다. 사용자 파일이 있는 폴더를 덮어쓰거나 실행 중인 Editor를 임의 종료하지 않는다. 명령 인자와 출력 위치의 기준은 [통합 수업](../G3_INTEGRATION.md)이다.

1. 새 경로에서 시작 SHA `f735b588`를 확보한다. `learning/tools/Replay-P0G3.ps1`로 최초 66개 입력을 조립한다. 원래 입력은 `0981d07307112857dfdf0e91c79bcecdbcbc291b`의 manifest이며 C++ `0e473f4`·Config `98727f0`를 구분한다. 이 명령은 현재 최종 제품을 이미 검증한 것으로 표시하지 않는다.
2. [문서화된 보충 절차](../G3_INTEGRATION.md)대로 `learning/tools/Apply-P0G3Supplement.ps1`에 **향후 확정할 full Source SHA**와 새 RunId를 전달하여 73개 입력으로 갱신한다. 기존 66개 해시와 새 7개 부재를 먼저 확인하고 변경 전 파일은 새 증거 폴더에 보존한다. 불일치를 무시하거나 현재 HEAD를 최종 SHA로 추정하지 않는다.
3. 새 입력 manifest에서 파일별 blob/SHA256, 제공·작성 분류, 변경 전후를 대조한다. 부분 묶음은 다음 묶음을 참조하므로 전체 조립 후 Editor 빌드를 수행한다. 실제 재현본 HEAD는 시작 SHA에 남을 수 있으므로 실행 입력 식별에 manifest를 반드시 함께 쓴다.
4. 새 RunId로 `tools/Build-P0Editor.ps1` → `tools/Test-P0Automation.ps1 -Filter LD.P0` → `tools/Test-P0PIE.ps1`을 순차 실행한다. 실제 GPU PIE 필터는 `LD.PIE.P0.Session`이며 NullRHI 자동화에 포함되지 않는다. 프로세스 exit0만 보지 말고 report의 Fail/Warnings/NotRun, proof, 실제 양쪽 PNG, PIE World0와 원설정 복원을 대조한다.
5. 같은 입력으로 Win64 패키지를 만든다. 자연 규칙의 `Run-P0G3.ps1`과 보충 `Run-P0G3Supplement.ps1 -Probe G3Boundary|G3NetConflict|G3Entry`를 별도 RunId·포트로 순차 실행한다. 양쪽 JSON·실제 RPC·정상 종료·PNG를 모두 확인한다. 생성·HP·등록 시각을 바꾼 Boundary와 무료 재료의 NetConflict는 자연 플레이/밸런스 결과에 합치지 않는다.
6. 실패하면 원본 로그와 입력을 보존하고 실제 원인·수정 SHA·같은 독립 기대값의 재실행을 기록한다. 최종 Source SHA, 두 단계 manifest, 위 실행 결과를 연결한 뒤 해당 수업의 필수 절차가 재현된 범위만 Verified로 바꾼다. 실제 학습자 상태는 별도다.

**현재 상태:** 최초 66개 조립의 실제 증거는 [G3_REPLAY](../evidence/G3_REPLAY/README.md)에 있다. 73개 보충까지 포함하는 이번 전체 재현은 미실행이며, 통합 브랜치의 최신 자동화·PIE 성공을 이 절차의 대체 증거로 쓰지 않는다. 이후 실행 결과는 SUMMARY에 한 번만 추가한다.

## 전달 커밋과 통합 순서

| 변경 묶음 | 계약·연결 순서 |
|---|---|
| `e61c414` → `61fb3a7` | Battle DTO/위젯 header→Director/Mode/State/native widget 구현 |
| B `8405a93` + `c697c91` | Processor guard 해제→Death Drain→복사한 AfterExternalCommandClock 실행→세션 재조회. A finalizer와 함께 연결 |
| `b425226` + `f796c19` | 확정 피해 관찰과 Actor 사망 출처. 관찰자는 게임 상태 원본을 소유하지 않음 |
| `83bd8fc` → `2a9d346` | A01: 종료 요청 이후 두 번째 공격을 막되 첫 사망 보상은 Drain |
| `162fd7f` → `decb709` | A02: Result 게시 전에 마지막 Actor HP를 조회 snapshot에 반영 |
| `1d06558` → `53333d9` → `0d358bc` | G2/G3Load 준비 대기 회귀→제품 수정→실제 TimerManager delegate 검사 수정 |
| `62b5180` → `53af399` | NET-LIFE01 실제 실패 회귀→process 수명의 세대 발급기. 제품 DTO는 유지 |
| `bb18616` 및 후속 검수 수정 | Boundary 제공 fixture. 세대 수정과 같은 최종 소스에 조립해야 함; 이 SHA는 제품 완료 SHA가 아님 |

B Entry 스타일 수명(PKG01)과 필수 효과음 cook(PKG02) 등 공용 의존도 manifest에 포함한다. native 위젯만 복사하고 B/Config를 생략하면 검수한 조합과 달라진다. 에셋·설정은 통합 담당자 소유이며 A 수업에서 동시에 저장하지 않는다.

<a id="review-regressions"></a>

## 실패 원인과 학습 포인트

| 사례 | 원인→수정 포인트 | 실제 증거의 위치·범위 |
|---|---|---|
| A01 | observer의 Abort가 후보만 저장하여 두 번째 타격이 진행됨→제품 종료 API 자체에서 접수/전투 즉시 닫기, 이미 확정한 Death는 Drain | REVIEW_FINDINGS의 A01 실패→수정 표. fixture의 직접 Combat.Stop으로 결함을 숨기지 않음 |
| A02 | 마지막 Actor HP가 Result snapshot에 반영되지 않음→새 타격/승리 평가 없이 마지막 조회 갱신 후 게시 | REVIEW_FINDINGS의 A02. 수치 자동화와 실제 화면은 별도 |
| 준비 분기 | fixture Preparing 마감0을 제품 wave 시작으로 해석→두 참가자가 준비될 때까지 RefreshReadiness 소유 전이만 허용 | REVIEW_FINDINGS의 준비 smoke 및 컴파일 수정. 최초 private API 호출 C2248도 실제 실패로 보존 |
| 미래 사망 fixture | 미래 시각71 보고 뒤 오래된 격자70.1로 승리를 당김→해당 사망 시각 전에는 판정 보류 | SUMMARY/REVIEW_FINDINGS의 VictoryRequiresAllThreeConditions. 제품의 자연 전투가 미래 사망을 생성했다는 주장은 하지 않음 |
| NET-LIFE01 | World마다 epoch1/2를 재사용→process 수명 발급. 새 Controller로 옛 payload를 재전송하는 반례 | REVIEW_FINDINGS의 NET-LIFE01. 오래된 Actor 채널 패킷과 다른 재현 |
| PIE/표시 검사 | private Editor API 접근, 같은 프레임의 오래된 host 화면, 잘못된 GC 유지 플래그 등 제공 검사 코드의 문제 | REVIEW_FINDINGS의 해당 실제 실패와 수정. 제품 결함과 검사기 결함을 구분 |

상세 수치·SHA·원본 로그는 정식 리뷰에 둔다. 코드 검토로만 찾은 callable 자기해제 위험이나 검수기 반례는 실제 게임 실패가 있었다고 바꾸어 기록하지 않는다.

## 구조 선택 대조

| 규약 | 실제 코드 경로·책임 | 검수에서 확인할 경계 |
|---|---|---|
| ARCH-01 책임 | Mode가 순서/종료, Director가 일정/적 소속, Widget이 값 표시를 담당 | Widget의 시간0이나 actor 개수로 승패를 변경하지 않음 |
| ARCH-02 의존 | Mode.InitGameState가 Data/State/Combat을 Director.Initialize로 주입; Controller가 Widget에 snapshot 전달 | Battle/Economy가 구체 Widget을 찾아 역호출하지 않음 |
| ARCH-03 상태 | GameState.BattleSnapshot이 공용 원본, EnemyActor가 HP 원본, process 발급기는 명령 세대만 소유 | Actor→보스 조회 snapshot 동기화와 세대/재화 RNG 분리 |
| ARCH-04 처리 | Processor.SubmitAtTime의 준비/확정/알림과 guard 해제 후 Drain, Mode.FinalizePendingTerminal | 실패/중복 무소비, 승인한 첫 보상이 Result 전에 보임, callback 뒤 세션 재조회 |
| ARCH-05 수명 | Mode의 LogicTimer/서비스 종료, Combat/Director.Stop, ResultWidget.NativeDestruct, process epoch | 종료 후 다음 타격0, 옛 구독0, 새 World의 옛 요청 거절, 검수 weak ref 수거 |
| ARCH-06 검증 | LDWaveTests/LDLifecycleTests의 독립 수치, LDPieTests의 실제2월드, Development 제공 probe | 계산/모의 시각·실제 PIE·별도 패키지·물리 Android를 서로 대체하지 않음 |

통합 후 구현자와 구분된 리뷰의 결론과 미검증은 REVIEW_FINDINGS에 둔다. 이 표는 코드 선택 이유이며 정적 표만으로 구조 검수 또는 런타임 통과를 선언하지 않는다.

## Verified 진입 조건

- [ ] 시작 SHA→66개 조립→73개 보충의 문서 절차를 새 경로에서 재현하고 최종 full Source SHA를 확정했다.
- [ ] 새 입력의 Editor·필수 자동화·실제 PIE·패키지/네트워크 결과를 구분해 보존했다.
- [ ] 각 수업의 정상·실패·중복·종료/시간 경계에 독립 기대값과 실제 관찰을 연결했다.
- [ ] 양쪽 화면, 결과 후 UI 재생성/입력 차단/구독·타이머 정리, 같은 프로세스 재진입을 확인했다.
- [ ] 독립 리뷰의 차단 결함을 수정·재검증하고 코드·수업·정식 검수의 입력이 일치한다.
- [x] 이전 20분 성능의 입력/환경/측정 한계와 Android G4 NotRun을 분리했다.
