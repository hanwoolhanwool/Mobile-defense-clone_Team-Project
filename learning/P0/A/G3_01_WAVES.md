# 예정 생성과 일반 적 수 — P0 / A / G3-A-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-WAVE-01 / [전투 5.1~5.3](../../../docs/design/BATTLE.md), [A-05](../../../docs/technical/IMPLEMENTATION_A.md#a05) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작 / 구현 완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / `f796c19d5319e47ffc0b405e62f699b25e19a641` — 실행·재현 완료 아님 |
| 실제 개발 시작 / 완료 SHA | 자기 G2 통합 결과 / 미생성 |
| 상대 산출물 | G2 Combat/Board/Economy/Processor, G3-A-02의 사건 순서 조립 |
| 제공 / 직접 작성 | 제공: G2 실제 서비스·로더·맵·경로. 직접 작성: Data/LDBattleTypes.h, Battle/LDWaveDirector.h/.cpp, EnemyActor의 사망 시각 |

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
7. `RefreshCombatView`는 실제 보스 HP 변화를 조회 모델에 게시한다. `EvaluateVictory`는 최종 생성 완료·두 보스 HP0·일반0을 모두 확인한다. `Stop`은 일정·구독·등록을 닫는다. 결과 확정과 보상/서비스 정리는 다음 수업의 GameMode에 맡긴다.

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

| 입력/조건 | 독립 기대 결과 | 실제 결과·범위 |
|---|---|---|
| 시작10, 프레임12.6 | 생성 시각10/10/11/11/12/12,6마리 | 자동화 작성, 미실행 |
| 시작10,29.999→30.0001 |40→42, wave1→2, 이전 actor 유지 | 자동화 작성, 미실행 |
| 정상 적을 실제로 처치하며9웨이브 | 누적360 일반, 양쪽 Gold460; wave10에 보스2/HP6000/마감250 | 자동화 작성, 미실행; 강제 피해 fixture |
| N99, 같은시각 처치0/1/2 후 생성2 |100즉시패배 /100즉시패배 /99계속 | 자동화 작성, 미실행 |
| 중복/위조 Death | 원본 actor·등록에 맞는 단일 사망만 처리 | 코드 방어 작성; 실행 결과는 공통 기록 대기 |
| 종료 뒤 다음 생성 | 생성/11웨이브0, 결과 불변 | 자동화 작성, 미실행 |

실제 화면은 아직 검수하지 않았다. 정상 기대 관찰은 상단 일반 적 수가1초마다2 증가하고, 처치에 따라 줄며, 보스는 N에 포함되지 않는 것이다. 사망0.25초 actor 정리는 네트워크와 장기 부하에서 추가 검증해야 한다.

## 상대에게 전달하고 통합하기

계약 `e61c414` → B clock 후단 `8405a93`/자기해제 보호 `c697c91` → A runtime `61fb3a7` → 사망 시각/검사 `f796c19` 순이다. `GetWaveDirector`, `GetBattleSnapshot`은 통합 검수 조회 창구다. UObject를 별도 Blueprint로 중복 생성하지 않는다. B는 snapshot을 위젯에 전달하고 UI 타이머로 승패를 변경하지 않는다.

## 이해 확인

- 한 번에 두 적을 생성한 뒤 N≥100을 검사하면 N99에서 어떤 불필요한 개체가 확정되는가?
- 살아 있는 집합에서 제거했지만 actor/Combat 등록을 정리하지 않으면20분 뒤 무엇이 누적되는가?
- 작은 변형: 테스트에서 시작 시각을10에서100으로 바꾸고 상대 예정 시각0/1/2가 유지되는지 독립 기대값을 바꿔 확인한다. 제품 웨이브 수·속도는 바꾸지 않는다.

## 단계 완료

- [x] 제공/작성 경계·파일 순서·설정·API·독립 기대값을 기록했다.
- [ ] 새 출발점에서 파일 조립→Editor→자동화→양쪽 패키지 화면 재현을 확인했다.
- [ ] 실패 기록·수정 SHA·최종 증거를 연결했다.
- [ ]10웨이브 패키지·지연망·반복 매치·대표 부하·Android를 확인했다. 다음 게이트 진입은 정식 G3 검수 이후다.
