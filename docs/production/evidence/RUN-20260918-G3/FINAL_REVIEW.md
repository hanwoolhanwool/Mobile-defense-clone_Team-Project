# G3 최종 독립 리뷰 대응표

2026-09-28 최종 PC 증거 갱신. **독립 리뷰의 열린 중대 제품 결함 0 / G3 자동·화면 검수 Pass, 실제 청음 대기 / Android G4 NotRun / P0 최종 미완료**. 제품 구현자와 분리하여 코드 diff와 원본 실행 결과를 읽었다. 이 문서 작성 중 빌드·에디터·패키지를 실행하지 않았으며 [독립 기대표](REVIEW_PLAN.md)는 변경하지 않았다. 아래의 Pass는 명시한 실행 층과 입력만 뜻한다. 실제 학습자 진행 및 수업 Verified 판정과 별개다.

## 입력과 증거 범위

- **UE58**: 제품 `53af3993b49234b4f78b0c0213c4f3ed5041f7be`, [전체 LD.P0 58 Success / 경고·Fail·NotRun 0](epoch-after-58.json). NullRHI의 실제 Unreal 객체 검사이며 화면·네트워크 패키지 검수가 아니다. 상세 test name 원본은 통합 `Saved/P0Runs/G3-epoch-after-58/report/index.json`.
- **PIE**: 같은 제품 SHA의 [실제 GPU 2 World PIE](terminal-ui-after.json), [전체 proof](terminal-ui-after-proof.json), [요약](terminal-ui-summary.json). 1 Success, 설정/자기 관찰자 복원, 종료 World 0. 종료 Result/Status 재생성·Engine InputKey/InputTouch 검사를 포함하며 물리 입력은 아니다.
- **FIVE / REC / LOAD**: [5시드](package-five-seeds-summary.json), [300ms/3% 회복](package-recovery-summary.json), [20분 부하·2000회 수명](PERFORMANCE.md)의 기존 Win64 Development 패키지 Pass. C++ `0e473f4af380506d209a95f7ec42eccf89c69df4`, cook 설정 `98727f04e7c563a854a103ad26152cff5ed652a6`, exe SHA256 `4339F4E3C166D244DC77D780C4E26FBC423E30E7BDB0923C1E39628F5B2B6B54`. [66개 파일 입력](package-final-inputs.json)과 재현 HEAD `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`는 다르다. 이 실행이 이후 epoch 수정의 패키지 검증을 대신하지 않는다.
- **BOUNDARY / ENTRY / CONFLICT**: 최종 실제 GPU Win64 두 프로세스 패키지 [보충 공통 판정](SUPPLEMENTS.md). 소스 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`, [73파일 입력](final-package-inputs.json), exe SHA256 `349F4660D6A0729807EA4FFD04DEE7A6D732001D00D830488EF00C42C049B177`. [Boundary](final-G3Boundary-pair.json) host179/client86, [Entry](final-G3Entry-pair.json) host35/client29, [NetConflict](final-G3NetConflict-pair.json) host27/client14 모두 Pass, 실패 checks0·상태 일치·양쪽 exit0·critical0. 자연5시드의 구 패키지 결과를 새 epoch 회귀로 대신하지 않고 이 세 실행으로 연결했다. 앞선 Editor-game/실패 실행도 보존한다.
- **DETAIL-UE**: 검사 전용 `f64cc671848560923595cc1955efe12620f326de`의 `LDWaveTests.cpp` +197줄 diff를 독립 대조했다. [통합 Editor](final-detail-editor.json), [통합 Waves9종](final-detail-waves.json), [재현 입력·Editor·Waves9종](REPLAY_EVIDENCE.md) 모두 Pass, Waves 각각 경고·Fail·NotRun0. 이전 e89a1fa의29.999 준비/N101 방어와 새 P+9.999 명령/9→10 잔여 Actor/11.04 한도 선처리를 포함한다. [최종 재현 blob 목록](replay-evidence/G3_REPLAY-final-detail-inputs.json)에서 제품·fixture·설정·데이터·맵 변경0이며 **f64의 테스트 소스를 e89 패키지 입력으로 소급하지 않는다**.

검사 구현 위치: [웨이브](../../../../Source/Mobile_defense_clone/Tests/LDWaveTests.cpp), [전투](../../../../Source/Mobile_defense_clone/Tests/LDCombatTests.cpp), [명령](../../../../Source/Mobile_defense_clone/Tests/LDGameplayCommandTests.cpp), [수명](../../../../Source/Mobile_defense_clone/Tests/LDLifecycleTests.cpp), [Entry/결과](../../../../Source/Mobile_defense_clone/Tests/LDEntryAndHudTests.cpp). 아래 test name은 이 파일들의 실제 함수/등록 이름을 가리킨다. 공통 규칙 설명은 [작업 기록](../../P0_REFERENCE_RUN.md)과 기대표를 따른다.

## G3-R01~R24

| 항목 | 실제 확인한 코드·증거 | 판정 범위·남은 한계 |
|---|---|---|
| R01 초기화·Loading | DETAIL-UE `LoadingPreparationAndExactReadiness`:29.999/30.0 ready 허용,30.001 timeout·late 등록 거절。UE 데이터 누락/준비 순서 회귀. ENTRY 실제 Entry35초 무시계→Loading30초 Aborted→늦은 Join 미등록/초기값 유지→중복 복귀 차단·client 실제 네트워크 종료 | 해당 UE·PKG 기대 표본 확인. 패키지 LoadingTimeout과 UE 데이터 실패는 서로 다른 증거이며 손상 pak 실행을 주장하지 않음 |
| R02 Preparing | UE58 첫 소환100→80·준비10초·첫 적2. DETAIL-UE P+9.999 실제 PC 구매22/판매12 뒤 gold70/pop1/paid2/revision3,기존 Actor/ID·due.25 보존 후 실제 Running 전이. BOUNDARY 유료 소환→이동 | 해당 UE·PKG 기대 표본 확인. WorldTime 지정 fixture이며 Running 뒤 만료된 공격 예약까지 동결해야 한다고 해석하지 않음 |
| R03 생성 일정 | UE58 `ExactSpawnScheduleAndTenWaves`: Director/Actor 예정0·1·2초 총6,19.999 총40,다음 wave42,일반 총360·보스2·wave10 | UE 통과 범위. KillAll 시간 fixture를 자연 플레이로 기록하지 않음 |
| R04 hitch | UE58 일정·정확 타격 위치. BOUNDARY case0 실제 host 약200ms 정지 뒤 D−.001/D 타격의 원시각 유지·보상 후 승리 | PKG 확인. 등록한 due fixture이며 일반적인 모든 프레임 정지 시간을 보증하지 않음 |
| R05 보스 진입 | UE58 B01 둘,HP6000/방어20/저항.10·공통D·finalSpawns. DETAIL-UE wave9 ID321/322·route0/1·HP112의 같은 Actor가 wave10에도 생존:일반2+보스2. FIVE 실제 wave10 | 명시 처치358회로 한도를 피한 UE carryover fixture. 부분 스폰 실패는 R15 |
| R06 적 한도 | UE58 `ImmediateCapAndDeathDeduplication`:N99 유지,100 게시 중 재진입 차단,후속 생성/구제 정지. DETAIL-UE 손상 snapshot fixture의 생성N101→즉시 잠금/원시각 패배/보상0. BOUNDARY case3 양쪽 실제N99→100/EnemyLimit | UE·PKG 확인. N101은 공용 수량 주입이며 정상 Actor101 생성/부하로 기록하지 않음 |
| R07 동일시각 사망/생성 | UE58 위 검사: 실제 HP/사망 처리 후0·1·2처치와 예약 생성 대조,99−2+2=99 및100 단일 latch·보상 확인 | 사망 주입 fixture임을 유지. 해당 행은 실제 PKG 타격 두 번을 관찰했다는 뜻이 아님 |
| R08 보스 마감 | UE58 `ProductionClockBossBoundaryAndSale`:69.999/70/70.001 실제 Combat·Mode·Director. BOUNDARY case0 조기/정확 타격·승리,case1 D+.001 취소·D 패배 | UE·PKG 확인. HP1/예약 due를 제공하되 실제 타격·종료·보상은 제품 경로 |
| R09 명령 전 시계 | UE58 70.04 새 요청 거절·원응답, `EarlierKillFundsExternalPurchase`:27+1−28=0. DETAIL-UE N99→World11.04 실제 PC 제출이11초 생성N100/EnemyLimit을 먼저 확정→PhaseNotAllowed,보드/경제/RNG 불변·이전 성공 응답 전체 재전송 | 누락됐던 cap+.04 표본 UE 확인. 네트워크 패킷이 정확 시각에 도착한 실행이라는 뜻은 아님 |
| R10 열린 현재 시각 | UE58 `ProductionClockBossBoundaryAndSale`, `TimerBeforeSameWorldTimeSale`: timer 먼저 실행한 같은 WorldTime의 실제 PC 판매가 due 타격보다 먼저 확정 | fixture가 WorldTime을 지정한 UE 결과. 실제 네트워크 요청이 정확D에 도착하는 PKG 증거와 구분 |
| R11 처치·최종 보상 | UE58 `RewardsAndPreparedRevision`:30/30.0001 별5·중복 제거; `TenfoldDeathAndTwoFastBosses`, `RewardBeforeTerminalClose`,실제 Actor wave boundary·A01. FIVE와 BOUNDARY 양쪽 최종 금280/180/281/80·별4/2/4/0 일치 | UE·PKG 확인. 숫자 보상 fixture와 실제 Actor/Result 경로 구분 유지 |
| R12 승리 | UE58 `VictoryRequiresAllThreeConditions`:8조합/실제 보스 사망/일반1→0/no wave11. BOUNDARY case0 승리 및 case2 양쪽보스 사망·N1로D 이후 Running→D+2 마지막 사망 즉시 승리 | 최종 PKG Victory/잔여 조건 확인. 자연5판 전승이나 자연 전략 클리어 증거는 아님 |
| R13 종료 재진입 | UE58 `CommittedDeathBeforeObserverAbort`, `CommittedBossHPBeforeObserverAbortResult`, `TerminalSessionReplay`,이중 EndPlay. PIE 종료 입력/RPC0·타이머 해제. FIVE Result 후 캐시 응답. 최종 세 보충의 종료 잠금·정리·정상 exit | 과거 실패 보존. 수정 후 실행에서만 Closed, 숫자상 checks 증가를 고유 규칙 증가로 해석하지 않음 |
| R14 권한·세대 | GameState 권한·terminal guard,Mode 등록,Processor Context 검사. UE58 `PreviousMatchRequestRejected`. BOUNDARY 새 PC에 이전 성공 payload를 실제3회 전송→InvalidEpoch 및 서버 보드/경제/RNG/캐시 불변,새 요청 정상 | `53af399` 최종 PKG 조건 Closed. 이전 Actor 채널의 지연 패킷과 구분; 재시작 간 epoch는 확률적 충돌 감소 |
| R15 최종 로딩 | FIVE/REC cooked Entry·맵·JSON·폰트·메시·결과·효과음 로드. 최종73파일 새 패키지의 Entry/전장/결과 실제 실행. UE 데이터 누락·`PartialBossSpawnFailureAndForeignWorld` 실패 정리. PKG01/02 Closed | **실제 청음 NotRun**. 원계획의 정상 PKG 로드·UE 실패 주입 층은 충족한다. 손상 pak 제작은 추가 필수 요구가 아니며 실행했다고 표시하지 않음 |
| R16 표시·재생성 | FIVE 양쪽540×1170 wave10/Result,PIE Result/Status 새1개·옛 구독0·4개 수거,A02 HP5900. 최종 BOUNDARY 양쪽9장·ENTRY360×780 Loading/복귀 관찰. [독립 client 원본9장](final-boundary-client-visual.md) 수치/문구/버튼 정상 | 원본에서 소환·합성·판매 글자 정상. 축소 preview 오인을 결함으로 등록하지 않음. 모든 화면비/모든 프레임/물리 터치 보증 아님 |
| R17 PC 한 판 | FIVE/REC actual GPU Win64 두 PID의 모든 핵심 조작→10wave→Defeat→복귀. 최종 BOUNDARY 승리2/시간초과1/한도1 및 ENTRY 정상 진입/복귀·CONFLICT 실제RPC | 자연5판은 모두 BossTimeout,보충 승리는 명시 fixture. 두 층의 범위를 합쳐 자연 플레이 승률로 만들지 않음 |
| R18 지연150/손실1% | FIVE endpoint별 송신75ms/1%,연결 게임1250/1252초,실제 RPC 원응답·최종 상태 일치. echo 실측은 [요약](SUMMARY.md) | 요청RTT150과 관찰 echo 평균 약186~198ms를 구별. 같은 PC 두 프로세스이며 두 실기기 네트워크가 아님 |
| R19 지연300/회복 | REC endpoint별 송신150ms/3%→120초에 해제→약250초 완주; client retry17·최종 수렴,실제 wire Pass | echo 전체244개 p95 349.6ms는 손실/회복 혼합; 구간별 p95는 미측정 |
| R20 중복·경합 | UE58 명령·원자성·conflict/expiry,FIVE 실제RPC62검사·종료 뒤 원응답/음성대조。최종 CONFLICT 실제 두 distinct ID 동일3재료:Success1/StaleBoard1·인구3→1·RNG1회·캐시2/게시1·양쪽 전체 응답 일치 | PKG 확인. 무료 재료 fixture와 raw RPC burst이며 Pending UI 중복 클릭 검사로 확대하지 않음 |
| R21 반복 매치·UI | FIVE 같은 두 PID의5 MatchId·4회 실제 복귀·판별 HUD재생성3회,PIE 구독/GC. BOUNDARY 같은 두 PID의 혼합 승패4 MatchId/3복귀·새 epoch·이전payload 거절. ENTRY 옛 World/PC/Result 수거·network observer 해제 | 최종 PKG 조건 확인. 4매치/5매치 및2000 수명 표본을 모든 장기 수명 보증으로 확대하지 않음 |
| R22 다섯 시드 | FIVE 1776/42/1729/2026/9001 전부 기록,10wave BossTimeout·일반0,피해·첫합성·재화·HUD·입력 전략 보존 | 명시 자동 조작/자연 제품 규칙. 사람 플레이·학습자 완료·전승·개별16종 밸런스 보증 아님 |
| R23 대표 부하 | LOAD 각40유닛/99일반/2보스,CSV warmup20초 후1204초·72183/72185프레임. 서버 Combat 프레임 합계p95 .859ms | 무료배치·고정HP·수동부하 fixture. Timeline과 중첩,client Combat 열 없음,60FPS 제한·낮은 해상도. 실측 상세/한계는 PERFORMANCE |
| R24 2000 수명 | LOAD 실제 기본 공격 자연사망2000/fallback0/25묶음,양쪽2141 weak 수거·최종활성0·서버등록0·종료 handshake | Director 자연0.25초 정리 대신 fixture 직접 unregister/destroy. client 중간GC메모리 없음,서버 마지막10GC 기울기+41332B/배치; 보편적 무누수 판정 금지 |

## ARCH-01~06 코드 대조

| 규약 | 실제 책임·원본·수명 경로와 결론 | 검증 근거/한계 |
|---|---|---|
| ARCH-01 책임 | `LDGameMode::AdvanceTimelineBefore`가 사건 순서·종료를 조립하고 `LDWaveDirector::BeginWave/SpawnEnemy/ProcessEventsAt/EvaluateVictory`, `LDCombatService::ResolveScheduledAttacks`, `LDCommandProcessor::DrainCombatRewards`가 각 규칙을 담당한다. PC/Result/Status는 입력·snapshot 표시다. 추가 제품 차단 발견 없음 | UE58 시각/결과 테스트·FIVE. 검수 GI probe의 강제 설정은 기본 제품 흐름과 구분 |
| ARCH-02 의존 | Mode 초기화가 Data/State/Combat/Board/Economy/Processor를 생성·주입하고 typed delegate를 연결한다. Board/Economy는 상호 호출하지 않으며 Combat/Director는 구체 PC/UMG를 찾아 재화를 바꾸지 않는다 | 실제 include/API 및 Initialize/HandleEnemyDeath/HandleBoardCommitted/StopMatchServices 대조. fixture 전용 `!UE_BUILD_SHIPPING` friend는 생산 결과 setter 대체 없음 |
| ARCH-03 원본·권한 | 서버 Board/Economy/EnemyActor/CommandProcessor가 각 보드·재화/RNG·HP·캐시 원본. 서버 GameState Battle은 공용 복제 상태, Director의 cursor/등록ID는 일정·수명 자료이며 `SpawnEnemy/HandleEnemyDeath/RefreshCombatView`→UpdateBattle로 N/HP를 게시한다. PC snapshot은 조회용 | HasAuthority/MatchId/World/epoch 검증, V01/V02 고유집합 비교, A02 회귀. process epoch 발급기는 신원용으로만 추가됐으며 경제 RNG와 독립 |
| ARCH-04 원자성·순서 | Processor `ExecuteCommand`의 Board/Economy TryPrepare→재검증→두 CommitPrepared→PublishPrepared 순서. 실패 때 원본/RNG 불변. Mode `<WorldNow` 시간 처리→보상 drain→spawn/count→deadline/victory; RequestTerminal이 즉시 접수/Combat을 잠그고 FinalizePendingTerminal이 승인 보상/마지막HP 후 Result 게시 | UE58 실패·중복·종료와 A01/A02 before/after. CONFLICT 실제 패키지 경쟁 요청·BOUNDARY 마감/보상과 ENTRY 종료 확인 |
| ARCH-05 수명 | Mode UPROPERTY 서비스 소유·EndPlay→StopMatchServices/ReleasePlayerSessions; Combat/Director Stop 등록/delegate 정리. PC EndPlay 세션/Pending/UI 해제, Result NativeDestruct 반환 바인딩 제거. GI RequestEntryReturn은 weak ticker·중복 gate, NotifyEntryReady/Shutdown은 ticker/자기 failure handle 제거 | PIE 새 위젯/옛 구독/GC·FIVE 4회 복귀·ENTRY 실제 패키지 여행. LOAD의 비정상 캡처 중 World 전환 시 CSV 종료는 도구 부채 |
| ARCH-06 검증 | 규칙 숫자·실패/재진입·원응답·동시성·권한·수명을 실제 코드와 대조. UE/PIE/Editor-game/PKG/Android를 별도 판정하고 구현 전 기대표는 보존 | 이 표 자체는 실행 증거가 아니다. 보충 fixture 실패를 수정해도 새 실행 전 Pass 불가. 학습 재현은 별도66→73→테스트1파일의 실제 조립/실행 증거로 PC 범위만 Verified |

## 닫힌 결함과 남은 조건

원인·파일·수정 경로는 [REVIEW_FINDINGS](REVIEW_FINDINGS.md)가 원본이다.

| 결함 | 닫힌 근거 | 남은 범위 |
|---|---|---|
| A01 Abort 뒤 두 번째 타격 | [수정 전 Damage2/둘째HP0 Fail](abort-regression-before-errors.json)→RequestTerminal 즉시 Combat.Stop→[54 Pass](abort-regression-after-54.json), UE58 포함 | 새 현상 없음 |
| A02 종료 snapshot의 오래된 보스HP | [기대5900/실제6000 Fail](boss-view-regression-before-errors.json)→FinalizePendingTerminal의 RefreshCombatView→[56 Pass](integration-automation-56.json), UE58 포함 | 새 현상 없음 |
| PKG01 임시 Entry 스타일 수명 | [cooked 첫 화면 크래시](packaged-entry-crash.json)→WidgetStyle 소유 주소→FIVE/REC의 실제 Entry와 반복 복귀 | 최종 BOUNDARY/ENTRY에서 재확인 |
| PKG02 효과음 cook 누락 | [실제 로드 Fail](packaged-audio-missing.json)→cook 포함→FIVE/REC 양쪽 로드 Pass | 실제 청취 NotRun |
| UI-TEST01 잘못된 PIE GC flags | [실제 ensure/Fail](terminal-ui-before-errors.json)→엔진 KEEPFLAGS 정책→[PIE Pass](terminal-ui-after.json), 이전 위젯4개 수거 | 물리 입력과 구별 |
| NET-LIFE01 새 판의 이전 epoch 재사용 | [실제 구매 재승인 Fail](epoch-before-errors.json)→제품53af399→UE58 Pass | BOUNDARY에서 새 Controller로 이전 payload3회 재전송 거절·새 요청 정상 |
| 보충 검사기 결함 | Unity writer 충돌5ce7fb6 compile 수정, 첫 프레임 Slate 클릭4002bd4, 로그 dispatch/timeout/UI 캡처ab03161 수정→BOUNDARY-EG fix2 실제4case Pass. fix1 실제 InvalidEpoch 수신을 검사기가 놓친 Fail 보존 | 최종 PKG 세 실행과 원본 양쪽 화면 확인까지 Closed. 이전 Fail 보존 |

최종 PC 패키지·경계 세부 표본·양쪽 원본 화면 검수는 위 증거로 닫혔다. 최종 역할 A/B Editor도 canonical 반영 후 각각 Pass([결과](final-role-build-summary.json)). 남은 필수 실행은 무음 검사와 별도로 실제 청음이다. 문서·데이터·학습 링크 마감 검사는 통합 담당이 별도 실행해 SUMMARY에 연결한다. **Android G4는 별도 필수 미완료**: [최근 준비 확인](android-latest-readiness.json)은 UE Android 구성 요소 없음·ADB 기기0이며 설치/실행/물리 터치/SafeArea/10웨이브/기기 성능 NotRun이다.

기술 부채/측정 한계는 병목과 구분한다. 서버 Combat 프레임 합계 p95 .859ms로 지속적인 전투 CPU 병목은 관찰되지 않아 추측 최적화를 추가하지 않았다. 최대 Frame/Render 약76/89ms 원인 미확정, client GC 중간표본 없음, CSV 비정상 종료 정리 미지원, 두 실제 기기/무제한FPS/장기 전체 무누수는 미검증이다. 기본 런타임의 학습 폴더 의존은 확인되지 않았으며 제공 probe는 명시 옵션에서만 생성된다. P1/P2 확장은 이 리뷰 범위에 없다.
