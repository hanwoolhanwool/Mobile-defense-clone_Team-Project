# G3 A 공통 증거·전달 기록

참고 자료 제작 **Draft**, 실제 학습자 **Planned**. 공통 G2 출발점 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`. 이 기록의 코드 작성 완료는 실행·재현·G3 통과를 뜻하지 않는다. 제품 기준은 [공통 작업 기록](../../../docs/production/P0_REFERENCE_RUN.md), [전투 명세](../../../docs/design/BATTLE.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md)이다.

## 변경과 전달

| 커밋 | 역할·변경 | API/의존성 |
|---|---|---|
| `e61c414` | A 공용 조회 계약 | FLDBattleSnapshot, ELDMatchResult/Reason, GameState.GetBattleSnapshot, native 두 widget header |
| `8405a93` (A 수신 `682c48f`) | B Processor 후단 | Before guard 해제→Drain→AfterExternalCommandClock→세션 재조회 |
| `b425226` | A 피해 관찰 | Combat.OnDamageCommitted(Event,PlayerIndex,EffectiveDamage), Stop 때 Clear |
| `61fb3a7` | A 제품 구현 | UObject WaveDirector, Mode 단일 timeline/terminal, GameState 결과, native widget cpp |
| `c697c91` (A 수신 `8f89116`) | B 수명 보완 | 후단 delegate 복사본 Execute로 자기 Close/Unbind 때 callable 수명 보존 |
| `f796c19` | A 실제 생산 조립 검사·사망 출처 | LD.P0.G3.Waves 5종, Enemy.DeathServerSeconds와 Director 대조 |

A는 자기 worktree의 허용 Source/학습 문서만 편집했다. 에디터·컴파일·바이너리·공용 설정·포트·기기는 통합 담당자가 직렬 실행한다. UI 원본 아트 추가/P1/P2는 구현하지 않았다. learn 브랜치는 공통 출발점에 둔다.

## 실행 범위와 독립 기대값

| 검사 | 범위 | 상태 |
|---|---|---|
| diff 공백·clang-format | 변경 C++ 파일 | 작성 중 적용, 최종 문서/데이터 검사는 통합 대기 |
| A Editor | Unreal C++/UHT/link | NotRun, root 실행 대기 |
| LD.P0.G3.Waves.* 5종 | 실제 Mode/State/PC/Board/Combat/Enemy와 명시적 시간/HP fixture | NotRun |
| G0/G1/G2 회귀 | 이전 계약 유지; G2 fixture 명시 | NotRun |
| PIE·PC 최종 패키지2프로세스 | 실제 UI/10웨이브·복귀·네트워크 | NotRun |
|5시드·20분 대표 부하·2000회 수명 | 강제 경계 fixture와 별도 정상/부하 조건 | NotRun |
| Android 실기기 | 물리 터치/SafeArea/10웨이브/성능 | NotRun |

기대값은 구현 결과로 재계산하지 않는다. 시작10일 때 일반 wave1 생성 시각10~29에 각2, wave2 첫30에는 누적42,9일반 누적360, wave10보스시작190/마감250이다. 보스2×6000HP는 일반 수에 포함하지 않는다. N99에서 처치0/1/2 후 생성2는 각각100 즉시패배/100 즉시패배/99 계속이다. F(최종생성)/B(양보스사망)/Z(일반0)의8조합에서 전부참만 승리다.

보스 경계 검사는 실제 PC 유료 소환·이동으로 두 유닛을 배치한 뒤 **초기 공격 예정 시각만** 명시 재등록하고 실제 B01에5999피해를 넣어 HP1로 만든다. deadline70에 due69.999/70은 승리, 한쪽70.001 또는 같은70 판매는 패배다. 이 검사는 자연 플레이·밸런스 증거가 아니다. 두60초 보스처치면 구매후Gold80+200=280/별4, 한쪽만처치면180/별2, host판매+한쪽처치면80+11+100=191이다. Result 통지 observer가 이미 이 최종 재화를 읽어야 한다.

## 작성 중 확인한 위험과 대응

- Processor Before hook 안은 bProcessing이므로 Drain이 무효다. 그 안에서 Close하면 확정 사망 큐가 사라진다. terminal 후보를 latch하고 새 접수를 먼저 닫은 뒤, guard 해제/Drain 이후의 후단에서 결과 게시/Close하도록 B와 계약했다. 실제 실행 실패를 관찰한 것으로 적지 않는다.
- 후단에서 Close가 자기 delegate를 Unbind한다. B는 callable 복사본을 실행하도록 보완했다.
- G2의 전투만 AdvanceBefore 하면 이전 boss deadline을 건너뛸 수 있다. Mode가20Hz 격자와 모든 Director 경계를 하나의 시간순 루프로 처리한다.
- HP0 actor의 사망 시각까지 실제 snapshot에 기록하고 Director에서 생성/사망 시각을 대조하여 별도 사건 신고가 빠른 보상 시각을 바꾸지 못하게 했다.
- native UMG의 최소 글자 크기·보스 두 HP 한 줄은 실제 좁은 화면 검수 전이며, 화면을 보고 필요한 수정만 한다.

## 재현 절차와 다음 조건

1. G2 완료 출발점의 별도 detached 작업 경로를 만든다. 기존 폴더/학습자 작업을 덮어쓰지 않는다.
2. [G3-A-01](G3_01_WAVES.md)→[G3-A-02](G3_02_TIMELINE.md)→[G3-A-03](G3_03_WIDGETS.md)의 파일 작성 순서로 A 파일을 조립하고 B Processor/Controller/Entry를 함께 연결한다. 최종 source manifest는 통합 검수 후 고정한다.
3. 프로젝트 `tools/Build-P0Editor.ps1 -RunId <새 ID>`로 컴파일 로그를 보존한다. `tools/Test-P0Automation.ps1 -Filter LD.P0.G3.Waves -RunId <새 ID>`로 필수 경계를 실행하고 실제 Report의 Fail/Warnings/NotRun을 확인한다. G0~G2 영향 회귀는 통합 담당자가 선정한다.
4. root의 실제 PIE/최종 패키지2프로세스·5시드·반복/네트워크·부하 절차를 실행하고 양쪽 캡처·실측·실패 원인·수정 SHA를 정식 G3 검수에 연결한다. 기록 파일이 아직 없는 절차를 완료로 체크하지 않는다.
5. 수업 출발점과 파일 조립으로 재현된 범위만 Verified로 바꾸고, 실제 학습자의 Planned는 유지한다. G3 차단 결함을 수정·재검수한 뒤 G4로 진행한다.

## 초기 실제 통합 검수와 수정

- 통합0ae913c Editor48.96초 Pass. 전체 LD.P0 52종 중51개 무경고Pass, VictoryRequiresAllThreeConditions 1개Fail. 원본 로그는 통합 Saved/P0Runs/G3-integration-automation-initial에 보존했다.
- 실패는 fixture가 시간70.1까지만 진행한 뒤 실제 Enemy.TryApplyDamage에 미래 사망시각71을 직접 넣고, 이후 밀린70.1 격자를 처리한 조건이다. 제품에서는 Combat가 각 격자까지의 타격만 확정하여 이런 미래 보고를 만들지 않는다. 하지만 EvaluateVictory의 min(격자,사망시각)은 그 입력을 조용히70.1로 소급했다. 미래 확정 시각은 아직 판정하지 않고 정확한 LastDeathServerSeconds에서 승리를 확정하도록 보완했다. 기대71은 유지하고 재검수 대기다.