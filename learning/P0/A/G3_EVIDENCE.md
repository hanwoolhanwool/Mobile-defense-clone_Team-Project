# G3 A 공통 증거·전달 기록

참고 자료 제작 **Draft**, 실제 학습자 **Planned**. 공통 G2 출발점 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`. 이 기록의 코드 작성 완료는 실행·재현·G3 통과를 뜻하지 않는다. 제품 기준은 [공통 작업 기록](../../../docs/production/P0_REFERENCE_RUN.md), [전투 명세](../../../docs/design/BATTLE.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md)이다.

현재 A Source는 `0d358bc5af920166bc517431848700d8c9c6a98f`, 아래 최신 통합 컴파일/자동화 기준은 `e4a02a4fc369601ff5434c9101c070998502b373`이다. 증거 파일의 공통 위치는 `C:/Users/iam12/P0_reference_integration/Saved/P0Runs/`이며 표의 실행 ID를 이 경로 뒤에 붙인다. `result.json`은 실행 범위/HEAD/요약, `report/index.json`은 각 자동화의 실제 실패·성공, `engine.log` 또는 `build.log`는 원본이다. 이 문서가 확인한 최신 실행은2026-09-27 22:15 KST이다.

## 변경과 전달

| 커밋 | 역할·변경 | API/의존성 |
|---|---|---|
| `e61c414` | A 공용 조회 계약 | FLDBattleSnapshot, ELDMatchResult/Reason, GameState.GetBattleSnapshot, native 두 widget header |
| `8405a93` (A 수신 `682c48f`) | B Processor 후단 | Before guard 해제→Drain→AfterExternalCommandClock→세션 재조회 |
| `b425226` | A 피해 관찰 | Combat.OnDamageCommitted(Event,PlayerIndex,EffectiveDamage), Stop 때 Clear |
| `61fb3a7` | A 제품 구현 | UObject WaveDirector, Mode 단일 timeline/terminal, GameState 결과, native widget cpp |
| `c697c91` (A 수신 `8f89116`) | B 수명 보완 | 후단 delegate 복사본 Execute로 자기 Close/Unbind 때 callable 수명 보존 |
| `f796c19` | A 실제 생산 조립 검사·사망 출처 | LD.P0.G3.Waves 5종, Enemy.DeathServerSeconds와 Director 대조 |
| `83bd8fc` → `2a9d346` | A01 실패 회귀 → 즉시 전투 중지 | Mode.AbortMatch만 호출해 후속 동일 시각 공격 취소, 승인 사망 Drain 유지 |
| `162fd7f` → `decb709` | A02 실패 회귀 → 최종 보스 HP 동기화 | Result 직전 Director.RefreshCombatView; 새 공격/승리 평가 없음 |
| `1d06558` → `53333d9` → `0d358bc` | 준비 대기 회귀 → Mode 수정 → 검사 컴파일 수정 | G2/G3Load 두 참가자 대기, 실제 TimerManager delegate 실행, 임시 옵션/프레임 복원 |

A는 자기 worktree의 허용 Source/학습 문서만 편집했다. 에디터·컴파일·바이너리·공용 설정·포트·기기는 통합 담당자가 직렬 실행한다. UI 원본 아트 추가/P1/P2는 구현하지 않았다. learn 브랜치는 공통 출발점에 둔다.

## 실행 범위와 독립 기대값

| 검사 | 범위 | 상태 |
|---|---|---|
| diff 공백·clang-format | 변경 C++ 파일 | 적용·통과; 문서 검사 결과는 문서 커밋 전달에 별도 기록 |
| 최신 통합 Editor | Unreal C++/UHT/link | `G3-review-final-editor-fix1`: Pass, build.log25.32초 |
| 최신 A 역할 Editor | 별도 A worktree 빌드 | 이번 Source 수정 뒤 별도 실행 증거 미수신; 통합 빌드와 구분 |
| LD.P0.G3.Waves.* 9종 | 실제 Mode/State/PC/Board/Combat/Enemy, 명시적 시간/HP fixture | `G3-review-final-automation-fix1`:9개 Success |
| 전체 LD.P0 회귀 | G0/G1/G2와 G3, UI/명령/수명 포함; NullRHI | 같은 실행에서56개 무경고 Pass, Fail/NotRun0 |
| 실제 PIE | GPU Editor, 한 프로세스의 listen/client2 World | `G3-actual-pie-v2`:1개 Pass, 원설정 복원; 후속 제품 수정 뒤 미재실행 |
| PC 최종 패키지2프로세스 | 최종 데이터/에셋·UI·10웨이브·복귀·네트워크 | 현재 A Source 기준 NotRun; 앞선 실행을 최종 패키지 통과로 승계하지 않음 |
| G3Load 초기2프로세스 smoke | Editor `-game`,10초 설정의 명시 부하 fixture | `G3-load-smoke-v1`: 준비 전 조기 Abort로 Fail, 측정 완료 아님 |
|5시드·20분 대표 부하·2000회 수명 | 정상/부하 조건을 구분한 최종 Source 실행 | 이 기록에 최종 완료 증거 없음 |
| Android 실기기 | 물리 터치/SafeArea/10웨이브/성능 | NotRun |

기대값은 구현 결과로 재계산하지 않는다. 시작10일 때 일반 wave1 생성 시각10~29에 각2, wave2 첫30에는 누적42,9일반 누적360, wave10보스시작190/마감250이다. 보스2×6000HP는 일반 수에 포함하지 않는다. N99에서 처치0/1/2 후 생성2는 각각100 즉시패배/100 즉시패배/99 계속이다. F(최종생성)/B(양보스사망)/Z(일반0)의8조합에서 전부참만 승리다.

보스 경계 검사는 실제 PC 유료 소환·이동으로 두 유닛을 배치한 뒤 **초기 공격 예정 시각만** 명시 재등록하고 실제 B01에5999피해를 넣어 HP1로 만든다. deadline70에 due69.999/70은 승리, 한쪽70.001 또는 같은70 판매는 패배다. 이 검사는 자연 플레이·밸런스 증거가 아니다. 두60초 보스처치면 구매후Gold80+200=280/별4, 한쪽만처치면180/별2, host판매+한쪽처치면80+11+100=191이다. Result 통지 observer가 이미 이 최종 재화를 읽어야 한다.

## 작성 중 확인한 위험과 대응

- Processor Before hook 안은 bProcessing이므로 Drain이 무효다. 그 안에서 Close하면 확정 사망 큐가 사라진다. terminal 후보를 latch하고 새 접수/전투를 먼저 닫은 뒤, guard 해제/Drain 이후의 후단에서 결과 게시/Close하도록 B와 계약했다. 이 초기 계약 위험 자체를 실제 실행 실패로 적지 않으며, 이후 A01/A02의 별도 실제 실패는 아래에 남긴다.
- 후단에서 Close가 자기 delegate를 Unbind한다. B는 callable 복사본을 실행하도록 보완했다.
- G2의 전투만 AdvanceBefore 하면 이전 boss deadline을 건너뛸 수 있다. Mode가20Hz 격자와 모든 Director 경계를 하나의 시간순 루프로 처리한다.
- HP0 actor의 사망 시각까지 실제 snapshot에 기록하고 Director에서 생성/사망 시각을 대조하여 별도 사건 신고가 빠른 보상 시각을 바꾸지 못하게 했다.
- native UMG의 최소 글자 크기·보스 두 HP 한 줄은 실제 좁은 화면 검수 전이며, 화면을 보고 필요한 수정만 한다.

## 재현 절차와 다음 조건

1. G2 완료 출발점의 별도 detached 작업 경로를 만든다. 기존 폴더/학습자 작업을 덮어쓰지 않는다.
2. [G3-A-01](G3_01_WAVES.md)→[G3-A-02](G3_02_TIMELINE.md)→[G3-A-03](G3_03_WIDGETS.md)의 파일 작성 순서로 A 파일을 조립하고 B Processor/Controller/Entry를 함께 연결한다. 최종 source manifest는 통합 검수 후 고정한다.
3. 프로젝트 `tools/Build-P0Editor.ps1 -RunId <새 ID>`로 컴파일 로그를 보존한다. `tools/Test-P0Automation.ps1 -Filter LD.P0.G3.Waves -RunId <새 ID>`로 필수 경계를 실행하고 실제 Report의 Fail/Warnings/NotRun을 확인한다. G0~G2 영향 회귀는 통합 담당자가 선정한다.
4. 최신 Source에서 root의 실제 PIE/최종 패키지2프로세스·5시드·반복/네트워크·부하 절차를 실행하고 양쪽 캡처·실측·실패 원인·수정 SHA를 정식 G3 검수에 연결한다. 과거 PIE v2 성공은 보존하지만 후속 제품 수정 뒤 재실행을 대체하지 않는다. 기록 파일이 아직 없는 절차를 완료로 체크하지 않는다.
5. 수업 출발점과 파일 조립으로 재현된 범위만 Verified로 바꾸고, 실제 학습자의 Planned는 유지한다. G3 차단 결함을 수정·재검수한 뒤 G4로 진행한다.

## 초기 실제 통합 검수와 수정

- 통합0ae913c Editor48.96초 Pass. 전체 LD.P0 52종 중51개 무경고Pass, VictoryRequiresAllThreeConditions 1개Fail. 원본 로그는 통합 Saved/P0Runs/G3-integration-automation-initial에 보존했다.
- 실패는 fixture가 시간70.1까지만 진행한 뒤 실제 Enemy.TryApplyDamage에 미래 사망시각71을 직접 넣고, 이후 밀린70.1 격자를 처리한 조건이다. 제품에서는 Combat가 각 격자까지의 타격만 확정하여 이런 미래 보고를 만들지 않는다. 하지만 EvaluateVictory의 min(격자,사망시각)은 그 입력을 조용히70.1로 소급했다. 미래 확정 시각은 아직 판정하지 않고 정확한 LastDeathServerSeconds에서 승리를 확정하도록 보완했다. 기대71은 유지했으며 최신56개 검사에 포함된 VictoryRequiresAllThreeConditions는 Pass했다.
## 추가 전달·미검증 범위

- `b88506d`: N100 공개 알림 전에 새 명령 접수를 닫는다. 같은 알림 observer가 실제 PC구매를 시도해 PhaseNotAllowed를 받는 회귀를 추가했다. 수량 상태는 Result 전 별도로 보이며 결과 후보를 취소할 수 없다.
- `bc99d37`: nonShipping의 명시적 `-P0Probe=G3Load`만 기존 G2 combat-only 분기에 넣었다. 부하 고정 fixture이며 정상10웨이브/밸런스 실행이 아니다.
- `baa4be6` + `2fbe86a`: 위 미래 사망 fixture의 결과 시각을 당기지 않는다. 마지막 사망 시각보다 이른 격자에서는 승리를 아직 판정하지 않고 실제 사망 시각을 그대로 사용한다.
- `27c2875`: `LD.PIE.P0.Session` 별도 필터의 실제 GPU Editor PIE 검사를 작성했다. `FStartPIEForAutomationCommand`가 listen/client2개의 실제 EWorldType::PIE World를 시작한다. 양쪽 소유 PC의 Preparing 소환/실제 RPC/골드80/인구1→10초 뒤 Running→양쪽 Slate 창 PNG→명시적 Abort 복제→FEndPlayMap→PIE World0을 검사한다. 이 필터는 NullRHI `LD.P0`에 포함되지 않는다. TimeSeconds나 PIE World를 임의 생성하지 않는다. v1/v2 실행 범위는 아래와 같다.
- PIE 설정은 복제한 ULevelEditorPlaySettings로 시작하고 원본 config property 전체를 보관한다. 로컬 UE5.8 PlayLevel.cpp의 PIE 종료가 CDO 창 위치를 저장하므로 검사 종료 뒤 원래 config 값을 복원하고 동일성을 확인한다. 기존 PIE 세션이나 같은 증거 RunId가 있으면 대체/덮어쓰기하지 않고 실패한다. Editor-only UnrealEd 모듈 연결은 통합 담당자 소유다.
- `001b78f`: 엔진 CsvProfiler의 LDP0/Combat, LDP0/Timeline CPU 범위를 추가했다. Timeline에는 Combat이 포함되므로 둘을 더하지 않는다. CSV 값은 프레임별 합계이며20Hz 개별 스텝 p95와 같은 지표가 아니다. 수집 오버헤드·FPS/VSync·대표 부하/측정 구간은 통합 실측에서 기록한다. 아직 성능 수치나 목표 통과를 보고할 근거가 없다.
## PIE 실행 관찰과 후속 수정

- 최초 PIE 코드 컴파일은 `G3-pie-editor-v1/build.log`에서 ULevelEditorPlaySettings의 private ClientWindowWidth/Height·AdditionalServerGameOptions 접근 C2248로 실패했다. `2087f6f`는 로컬 공개 `SetClientWindowSize`를 사용하고 별도 server option 직접 쓰기를 제거했다. Editor 실행의 `-P0Seed=1776`로 seed를 제공한다.
- 통합 `G3-actual-pie-v1`은 실제 Editor PIE 자동화1개 Pass, 두 실제 World의 listen/client RPC 소환·각 인구1/Gold80, Running N2/2, EndPlay 뒤 PIE World0 및 원설정 복원을 확인했다. 이는 별도 프로세스 패키지 검사나 실제 사용자의 수동 조작이 아니다.
- 요청 창540×1170은 데스크톱 제약으로 실제 캡처546×720이었다. root의 PNG 직접 검토에서 v1 host 이미지는 같은 프레임 UMG 갱신 이전 WAVE0/00:00, client는WAVE1을 표시했다. 게임 상태 검사는 통과했지만 이 캡처로 양쪽 최종 표시를 통과 처리하지 않았다. `b794473`에서 양쪽 Running 확인 뒤0.5초 더 기다리고 캡처하도록 바꿨다.
- `G3-actual-pie-v2`는 통합 `2f144fe4852ede5b96e6f51683e2df3b527ccb37`에서 실제 GPU PIE1개 Pass다. `G3-actual-pie-v2-proof/pie-proof.json`은 실제 PIE World/ListenServer·Client/소유자0·1, RPC 완료·각Gold80/인구1, wave1/N2·2, Aborted 복제·전투 등록 해제, 종료 후 PIE World0·설정 복원을 기록한다. 같은 폴더의 host-running.png/client-running.png는 각각546×720이다. root가 양쪽 이미지를 확인했다. A01/A02/준비 분기 수정 전의 성공이므로 최신 Source PIE·최종 패키지·수업 출발점 재현은 여전히 미실행이다.
- `c76bb35`: Combat.GetRegisteredEnemyCount는 living 수와 구분되는 등록 map 수, Mode.IsLogicTimerActive는 자기 LogicTimer만 읽는다. 장기/종료 검수의 읽기 전용 접점이다.
- `8cebbd4` + `df231fa`: Director.Initialize의 선택적 FLDEnemyActorFactory 한 점에서 실제 Actor 생성 실패를 주입한다. 제품 기본 경로는 기존 SpawnActor다. 두 번째 boss가 nullptr 또는 다른 World actor면 최종생성 false→Aborted/등록0/타이머0/보상0을 검사하며 외부 World actor는 초기화·삭제하지 않는다. callable을 복사해 재진입 Stop의 자기해제에 안전하게 하고 이미 쓰인 actor도 새 소유물로 확정하지 않는다.
- `b794473` + `a57341e`: 사망 게임 사건을 먼저 게시하고 복사한 피해 관찰자에 확정 값만 전달한다. Mode는 callback 뒤 terminal 후보를 다시 확인하여 같은 시각 남은 생성 단계도 중단한다. 다만 초기 회귀는 관찰자가 직접 Combat.Stop까지 호출해 Mode.AbortMatch만 사용했을 때의 결함을 가렸다. 아래 A01에서 검사를 강화하고 제품 종료 API를 수정했다.

역할 Build.cs의 Editor-only UnrealEd 의존은 root `bca735c`를 A `79e45f4`로 수신했다. 현재 A Source는 문서 첫머리의 SHA이며, 학습자가 그 완성 코드를 작성한 것으로 기록하지 않는다.

<a id="review-regressions"></a>

## 독립 리뷰 회귀와 수정

| 사례·실제 재현 | 실패 원인과 수정 | 수정 전 증거 | 수정 후 증거 |
|---|---|---|---|
| A01: 실제 일반 적2개 HP70, 강한 유닛2개 due11. 첫 OnDamage에서 Mode.AbortMatch만 호출 | PendingResult만 세워 Combat 내부 루프가 두 번째 HP도0으로 변경했지만 그 사망 보상은 거절됨. `83bd8fc`로 숨겨진 직접 Stop을 제거한 회귀를 먼저 적용하고, `2a9d346`에서 접수 잠금 직후 Combat.Stop. Processor는 승인된 첫 사망의 Drain까지 보존 | `G3-A01-before-test`, HEAD `15f14224a6a79159d82f7d45eb2ca83f8625db29`:1 Fail, 기대 공격1/실제2, 다른HP70/실제0 | `G3-A01-after-automation`, HEAD `6b59c582fb4d2c75472c28292008e873c2f6d8ec`:전체54개 무경고 Pass. 이후 최신56개에서도 Pass |
| A02: 실제 B01 두 개 HP6000, 물리120/방어20→독립 기대100피해, 첫 비치명타격 직후 Abort | Actor는5900이지만 terminal 후보 후 평상시 조회 갱신을 건너뛰어 Result의 GS HP는6000. `162fd7f` 회귀를 먼저 적용하고 `decb709`에서 Death Drain→RefreshCombatView→Result 순서로 수정 | `G3-A02-before-test`, HEAD `530327ae03966afd1a704a5d043d9100b70c87a3`:1 Fail, Result observer의 기대5900/실제6000 두 assertion | `G3-review-final-automation-fix1`:해당 검사 Success. 최초 결과 게시에서 Actor/GS5900·6000 일치, 공격1, 보상0, 반복 결과 변경0 |
| G2/G3Load 준비 대기: 실제 Editor `-game` 별도 host/client 실행, host 첫 timer | fixture Preparing의 마감0을 일반 웨이브 시작으로 해석해 아직 없는 Director 때문에 time0 Aborted. `1d06558`은 실제 CLI 두 옵션의 준비 대기 회귀, `53333d9`는 RefreshReadiness의 두 참가자 확인 전 timeline 대기/CanAcceptCommands false | `G3-load-smoke-v1/pair.json`:Fail. host engine.log1828에 `result=3 reason=5 time=0`, 두 번째 참가자 등록 전. 이는 패키지·정상 플레이·부하 측정 통과가 아님 | 최신56개 중 CombatFixturesWaitForBothParticipants Success. 실제 TimerManager delegate로0인/1인35초 대기·구매 거절·2인 즉시Running/소환80·Wave0/적0 확인. 수정 후 별도2프로세스 smoke는 이 기록에서 미실행 |
| 준비 회귀 검사 코드의 컴파일 오류 | private AdvanceLogic 직접 호출3곳의 C2248. `0d358bc`에서 제품 API 공개 없이 public TimerManager.Tick으로 기존 등록 delegate를 실행. pending timer 활성화와 프레임당1회 제한을 고려하고 임시 GFrameCounter/프로브 CLI는 scope 종료 때 복원 | `G3-review-final-editor/build.log`:LDWaveTests.cpp180/189/206 C2248, 컴파일 Fail | `G3-review-final-editor-fix1/build.log`:25.32초 Pass. 같은 HEAD `e4a02a4fc369601ff5434c9101c070998502b373` 전체56개 무경고 Pass/Fail0/NotRun0 |

`G3-A01-before-test`·`G3-A01-after-automation`·`G3-A02-before-test`·`G3-review-final-automation-fix1`의 자동화는 NullRHI 실제 C++ 검사다. Editor `-game` smoke와 컴파일 결과는 그와 구분하며, 수치 검사 성공을 실제 게임 화면 통과로 바꿔 적지 않는다. A01/A02 검사는 원래의 독립 기대값을 유지한 채 실패→제품 수정→통과로 연결했다. 두 회귀 모두 관찰자가 직접 서비스를 정리하여 제품 종료 결함을 숨기지 않는다.

최신56개 자동화가 통과해도 A 역할 최신 빌드, 수정 후 실제 PIE/최종 패키지·네트워크·대표 부하, 수업 출발점 조립 재현, Android 실기기 검수는 별도다. 이 확인이 남아 있어 세 수업은 계속 **Draft**, 실제 학습자 진행은 **Planned**다.
