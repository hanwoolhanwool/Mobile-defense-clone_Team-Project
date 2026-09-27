# 예정 생성과 일반 적 수 — P0 / A / G3-A-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-WAVE-01 / [전투 5.1~5.3](../../../docs/design/BATTLE.md), [A-05](../../../docs/technical/IMPLEMENTATION_A.md#a05) |
| 참고 자료 제작 / 실제 개발 상태 | **Verified — 아래 명시한 PC 재현 범위** / Planned |
| 참고 시작 / 완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / `f64cc671848560923595cc1955efe12620f326de` (제품+검사 소스). 실제 패키지 입력은 `e89a1fabaf5ef5e3a1d03a09397806814551ec20`; 마지막 차이는 테스트 파일만 |
| 실제 개발 시작 / 완료 SHA | 자기 G2 통합 결과 / 미생성 |
| 상대 산출물 | G2 Combat/Board/Economy/Processor, G3-A-02의 사건 순서 조립 |
| 제공 / 직접 작성 | 제공: G2 실제 서비스·로더·맵·경로. 직접 작성: Data/LDBattleTypes.h, Battle/LDWaveDirector.h/.cpp, EnemyActor의 사망 시각. 제공 검수: LDWaveTests/LDPieTests와 G3 probe; 테스트 기대값은 먼저 독립 작성 |

PC Verified는 [통합 수업의3단계 재현](../G3_INTEGRATION.md)과 [최종 PC 보충 증거](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md)에 연결한 실행 범위다. 선택 변형 과제·미관찰 입력 조합은 완료로 올리지 않는다. Android G4와 실제 청음은 NotRun이며 P0 최종 완료가 아니다. 실제 학습자는 Planned를 유지한다.

## 이번에 만들 동작

서버가 일반 웨이브마다 진영별20개체를0~19초에 생성하고20초에 다음 웨이브를 시작한다. 이전 개체는 유지한다.10웨이브에서는 B01 두 개체를 만들고, 성공한 두 생성 뒤에만 최종 생성 완료를 표시한다. 일반 적 수100 도달은 한 개체를 늘리는 함수 안에서 패배를 잠근다.

공통 규칙·증거·통합 상태는 [G3 A 공통 기록](G3_EVIDENCE.md)에서 관리한다. 생성/시간/HP를 바꾼 경계 fixture와 정상 플레이를 구분하며 이 문서의 구현 설명만으로 Verified를 표시하지 않는다.

## 코드 작성 순서

1. `Data/LDBattleTypes.h`에 공용 조회 snapshot을 만든다. Phase/Result/Revision, WaveIndex/FinalWave, 준비·웨이브·보스 마감, 일반 적 수, 두 보스 요약이 포함된다. HP 원본은 EnemyActor이고 복제용 보스 요약은 그 값을 읽는다.
2. `Battle/LDWaveDirector.h`에 주입받은 GameData/GameState/Combat UPROPERTY와 일정 cursor, 살아 있는 ID→약한 actor 참조, 일반 ID집합을 선언한다. 별도 Actor/타이머·에디터 배치·UI 포인터가 필요 없는 UObject다.
3. `Initialize`에서 같은 World·서버 권한·같은 RulesVersion을 확인한다. `StartAt`은 Running 한 번만 허용하고 `BeginWave(1)`을 호출한다.
4. `BeginWave`는 데이터 행에서 시작/마감 시각을 만든다. `GetNextEventSeconds`는 다음 스폰 또는 웨이브/보스 경계만 반환한다. 실제 실행 프레임을 생성 예정 시각으로 덮어쓰지 않는다.
5. `SpawnEnemy`는 행→경로→HP/생성 시각→Combat 등록을 모두 성공한 뒤 ID를 소비하고 일반 수/보스 요약을 게시한다. 실패한 자기 소유의 새 actor만 Destroy하고 원인 있는 Aborted를 요청한다. 선택적 ActorFactory 주입점은 이 생성 한 곳뿐이고, 기본 제품 경로는 SpawnActor다. 다른 World 또는 이미 초기화된 actor는 새 소유물로 다루지 않는다. 일반 수 증가는 하나씩 검사하여 첫100 이후 상대 진영 스폰도 중단한다.
6. `EnemyActor.TryApplyDamage`의 단일 HP0 전이에 DeathServerSeconds를 기록한다. `HandleEnemyDeath`는 MatchId/등록 actor/HP0/종류/생성·사망 시각을 대조한 뒤 일반 집합·Combat 등록에서 제거한다. 동일 사망을 다시 받으면 등록이 없어 무시한다. 죽은 actor는0.25초 뒤 해제하여 매치 내 누적을 막는다.
7. `RefreshCombatView`는 실제 보스 HP 변화를 조회 모델에 게시한다. 타격 관찰자에서 Abort를 요청한 경우에도 Mode가 Result 직전에 마지막 확정 HP를 동기화한다. 이 조회 호출은 새 공격이나 승리 판정을 실행하지 않는다. `EvaluateVictory`는 최종 생성 완료·두 보스 HP0·일반0을 모두 확인한다. `Stop`은 일정·구독·등록을 닫는다. 결과 확정과 보상/서비스 정리는 다음 수업의 GameMode에 맡긴다.

상태 소유 이유: Director는 일정 cursor와 적 소속을 소유한다. 공용 매치/일반 수/결과의 유일한 공개 원본은 GameState다. UI에서 actor를 세거나, Director에 별도 Result 원본을 만들지 않는다. 보스 HP 요약은 Actor의 권한 있는 HP를 복제하는 조회 값이다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `/Game/LD/Maps/L_P0` | 기존 BP_LDGameMode/native LDGameMode 유지 | 같은 두 보드·경로 |
| 2 | World Outliner | WaveDirector actor를 배치하지 않음 | Mode가 UObject를1개 생성; 중복 스폰 방지 |
| 3 | GameMode.InitGameState | NewObject → Initialize(GameData, GameState, Combat) → OnTerminalRequested 연결 | 서버 한 곳에서 소유/수명 연결 |
| 4 | `Content/LD/Data` | 기존 Schema2/Rules0.3.0의 웨이브/적 JSON, FinalWave10 | 학습 폴더 참조 없음 |
| 5 | 표시 | 기존 ALDEnemyActor 구형 mesh/route color 사용 | 두 경로의 ID 유지; 고유 보스 기술 없음 |

## 실행·실패·수정 기록

아래 초기 실패/결과는 당시 입력의 이력으로 보존한다. 최종73파일을 시작점에서 재조립해 검증했으며, 마지막 테스트 보강은 별도 Editor·Waves9종으로 확인했다. 공통 실행 수치·SHA는 [정식 SUMMARY](../../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md), 실패 원본은 [정식 리뷰](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)를 따른다.

| 입력/조건 | 독립 기대 결과 | 실제 결과·범위 |
|---|---|---|
| 시작10, 프레임12.6 | 생성 시각10/10/11/11/12/12,6마리 | NullRHI 자동화 Pass |
| 시작10,29.999→30.0001 |40→42, wave1→2, 이전 actor 유지 | NullRHI 자동화 Pass |
| 정상 적을 실제로 처치하며9웨이브 | 누적360 일반, 양쪽 Gold460; wave10에 보스2/HP6000/마감250 | NullRHI 자동화 Pass; 강제 피해 fixture |
| N99, 같은시각 처치0/1/2 후 생성2 |100즉시패배 /100즉시패배 /99계속 | NullRHI 자동화 Pass; N100 알림 중 새 구매도 거절 |
| 중복/위조 Death | 원본 actor·등록에 맞는 단일 사망만 처리 | NullRHI 자동화 Pass |
| 두 번째 보스 생성 실패/외부 World actor 반환 | 최종생성 false, Aborted, 등록0; 외부 actor 보존 | NullRHI 자동화 Pass |
| wave9 첫 일반 ID321/322·Route0/1을 남기고 나머지 명시 처치→wave10 | 같은 실제 Actor/ID/Route/HP112, N2+보스2·등록4, 358처치 보상으로 각Gold458 | f64cc671 추가 회귀, 통합/재현본 [최종 Waves9종](../evidence/G3_REPLAY/final-detail-waves.json) 무경고 Pass; 자연 플레이가 아닌 명시 처치 fixture |
| 종료 뒤 다음 생성 | 생성/11웨이브0, 결과 불변 | NullRHI 자동화 Pass |

생성·승리 조합의 자동화와 실제 패키지 자연 플레이를 구분한다. 기존 자연 패키지는10웨이브 보스 시간초과·잔여 일반0·11웨이브 없음 및 양쪽 상태 일치를 보였다. 강제 피해 fixture의 승리 검사를 자연 승리로 기록하지 않는다. 사망 후 actor/등록 수거와20분 부하의 실제 측정 범위·한계는 [PERFORMANCE](../../../docs/production/evidence/RUN-20260918-G3/PERFORMANCE.md)에 있다. 후속 Boundary의 승리/한도 사례는 [최종 보충 검수](../../../docs/production/evidence/RUN-20260918-G3/SUPPLEMENTS.md)의 실제 패키지 결과·양쪽 화면으로 확인했다.

## 상대에게 전달하고 통합하기

계약 `e61c414` → B clock 후단 `8405a93`/자기해제 보호 `c697c91` → A runtime `61fb3a7` → 사망 시각/검사 `f796c19` 순이다. `GetWaveDirector`, `GetBattleSnapshot`은 통합 검수 조회 창구다. UObject를 별도 Blueprint로 중복 생성하지 않는다. B는 snapshot을 위젯에 전달하고 UI 타이머로 승패를 변경하지 않는다.

최신 상태를 재현할 때는 [공통 증거의 후속 수정 순서](G3_EVIDENCE.md#review-regressions)를 함께 적용한다. A02는 위젯이 읽는 보스 HP를 Actor 원본과 맞추는 통합 수정이므로 초기 구현 SHA만 받으면 누락된다.

## 이해 확인

- 한 번에 두 적을 생성한 뒤 N≥100을 검사하면 N99에서 어떤 불필요한 개체가 확정되는가?
- 살아 있는 집합에서 제거했지만 actor/Combat 등록을 정리하지 않으면20분 뒤 무엇이 누적되는가?
- 작은 변형: 테스트에서 시작 시각을10에서100으로 바꾸고 상대 예정 시각0/1/2가 유지되는지 독립 기대값을 바꿔 확인한다. 제품 웨이브 수·속도는 바꾸지 않는다.

다음은 [G3-A-02](G3_02_TIMELINE.md)다. 생성 함수 단독 검사만 통과한 상태에서 결과를 직접 게시하지 말고, Mode의 보상 Drain/종료 순서와 연결한 뒤 실제 두 화면으로 재검증한다.

## 단계 완료

- [x] 제공/작성 경계·파일 순서·설정·API·독립 기대값을 기록했다.
- [x] 최초66파일의 조립·실행 이력을 공통 근거에 연결했다. 후속 입력의 재현을 대신하지 않는다.
- [x] 같은 시작점에 문서화된73파일 보충을 적용하고 최종 Source SHA·Editor·자동화·PIE·패키지 경계를 새로 확인했다.
- [x] 확인한 실패 기록·수정 SHA·통합 자동화 증거를 연결했다. Android·청음이 남아 P0 최종 완료는 아니다.
- [x]10웨이브 패키지·지연망·반복 매치를 확인했다. 자동 전술의5패배는 사람의 플레이 밸런스 검증이 아니다.
- [x] 기존20분 부하의 입력·실측·한계는 정식 PERFORMANCE에 연결했다.
- [x] PC 명시 범위의 리뷰·수업 재현 증거를 고정했다. Android G4와 실제 청음은 NotRun으로 남긴다.
