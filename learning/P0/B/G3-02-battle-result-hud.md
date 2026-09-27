# 전투·결과 HUD를 읽기 전용으로 연결하기 — G3 / B / 02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UI-01, TASK-TEST-01; [B HUD 계약](../../../docs/technical/IMPLEMENTATION_B.md), [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 상태 | Draft — A 위젯 통합·B Editor·실제 PIE 확인, 최종 패키지 결과/복귀 검수 대기 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6` / 현재 소스 `0e473f4af380506d209a95f7ec42eccf89c69df4` (게이트 완료 미확정) |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | A 계약 `e61c414`의 `FLDBattleSnapshot`, `ALDGameState`, `ULDBattleStatusWidget`, `ULDResultWidget`; 실제 구현과 종료/HP 수정까지 현재 통합 소스에 포함 |
| 제공 코드 / 직접 작성할 코드 | A DTO/전투/결과 위젯과 G2는 제공. B `LDPlayerController`, `LDGameplayWidget` 변경을 직접 작성 |

빌드·실행별 SHA, 새 재현본66파일과 Entry 수정 기록, 로그 위치는 [B 공통 검증 기록](README.md#g3-evidence)을 따른다. 아래에는 이 수업의 HUD 관찰과 남은 검수만 적는다.

## 이번에 만들 동작

Loading/Preparing/Running 화면 위에 서버의 웨이브·적 수·보스 HP/마감 정보를 표시한다. Result/Aborted에서는 결과와 복귀 버튼을 보여주고 새 소환·이동·합성·판매·보드 선택을 막는다. 결과가 여러 번 복제되거나 위젯이 제거·재생성되어도 복귀 한 번만 요청한다. 준비 단계의 경제 조작 가능 여부는 서버 admission과 PC의 동일 Phase 조건을 각각 적용한다.

설계 이유는 [ARCH-01~06](../../../docs/technical/CODING_STANDARD.md)의 상태 원본과 수명 분리다. B HUD는 HP/웨이브/승패를 계산하지 않는다. A GameState의 한 `BattleSnapshot`과 서버 시각을 `UpdateView`로 전달하고, A가 포맷한 화면을 배치한다. 명령 응답·개인 스냅샷의 기존 epoch/Revision 동기화는 그대로 사용한다.

## 코드 작성 순서

1. `Core/LDPlayerController.h`: Battle/Result 위젯 UPROPERTY, `UpdateBattleView`, `CanUseGameplayActions`, `RequestReturnToEntry`, 검사용 `GetReturnButtonScreenRect`를 추가한다.
2. `Core/LDPlayerController.cpp`: `UpdateGameplayView`에서 Battle 뷰를 먼저 갱신한다. 참가자 보드가 아직 준비되지 않았어도 LoadingTimeout/Aborted 결과는 보여야 한다. GameState 읽기→Status Widget 생성/갱신→terminal일 때 Result 생성/갱신 순서다.
3. Result 생성 후 `OnReturnRequested.AddUObject`로 PC의 복귀 의도 하나만 연결한다. 제거/세션 교체/EndPlay에서 `RemoveAll(this)` 후 `RemoveFromParent`한다. 매 프레임 delegate를 추가하지 않는다. GameState 구독 없이 현재 스냅샷을 읽으므로 위젯 재생성이 서버 상태를 바꾸지 않는다.
4. `CanUseGameplayActions`는 MatchId 일치·개인 스냅샷 준비·Preparing 또는 Running·복귀 미요청을 확인한다. `Request*` 의도 API와 `UI/LDGameplayWidget.cpp`의 버튼 활성화에서 함께 사용한다. 서버 Processor의 권한·Phase 검증은 여전히 최종 판정이다.
5. `InputScreenPosition`의 terminal guard와 드래그 초기화로 결과 화면 뒤 칸 선택/이동이 새 요청을 만들지 않게 한다. 기존 요청의 동일 키 재전송은 서버 원응답 확인 경로이므로 `RetryPendingCommand`와 캐시 자체를 지우지 않는다.
6. `RequestReturnToEntry`는 로컬 Controller·terminal·GI가 있을 때만 GI 복귀를 요청한다. PC의 `bEntryReturnRequested`와 GI의 예약 gate가 반복 이벤트를 막는다.
7. `UI/LDG1BoardWidget.h/.cpp`에 `SetBattleOverlayVisible(bool)`을 추가한다. A 전투 Widget이 viewport에 있으면 옛 Title/OpponentLabel만 숨긴다. Development의 명시적 `-P0Probe=G1/G2`에서는 새 Battle/Result Widget 생성을 생략해 과거 게이트 픽스처의 표시를 보존한다. Shipping에는 이 probe 예외를 넣지 않는다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `L_P0` / 기존 `BP_LDGameMode` | A GameState와 B `LDPlayerController` 사용 | 서버 단일 BattleSnapshot |
| 2 | native `ULDGameplayWidget` | 기존 SafeZone와1080×2340 v2 하단 배치 유지, z20 | 소환·합성·판매·재화 표시 |
| 3 | native `ULDBattleStatusWidget` | PC에서 CreateWidget/AddToViewport z30, 매 tick `UpdateView(snapshot, serverNow)` | 전장 위 전투 상태 |
| 4 | native `ULDResultWidget` | terminal일 때 z100, `OnReturnRequested`→PC | 보드 위 결과/복귀 |
| 5 | Project Settings | [Entry 수업](G3-01-entry-return.md)의 GI·맵 설정 | 같은 프로세스에서 복귀 |

추가 WBP Blueprint 그래프는 없다. A 위젯 내부 위치/텍스트 값은 A 수업을 원본으로 사용한다. P1 메뉴/강화/신화/룰렛을 추가하지 않는다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| Loading에서 보드 미준비→Aborted | 결과/복귀가 표시됨 | 미실행 | 실제 한 참가자 대기/초기화 실패 |
| Preparing에서 소환→Running | 서버 경제 확정, 전투는 Running부터 | 실제 PIE 양쪽 소환1/gold80·Running 확인; 준비시간 경계는 별도 자동화 Pass | [새 재현 PIE](../evidence/G3_REPLAY/pie-proof.json), B 57자동화의 `LoadingPreparationAndExactReadiness` |
| Result에서 S/M/X·터치/드래그 | 새 요청/상태 변화0 | 미실행 | PC/Slate/EngineTouch 검수 |
| Result Widget RemoveFromParent→재생성 | 표시 복구, 복귀1회 | 미실행 | 실제 GPU UI 수명 |
| 결과 여러 번 갱신·빠른 복귀 클릭2번 | Entry 여행1회 | 미실행 | GI gate 단위 검사 + 실제 여행 |
| 양쪽 Running UI | 같은 웨이브·일반 수·개인 상태 표시 | 실제 PIE WAVE1/20초/N2, gold80/pop1, 자기 보드 아래 관찰 | [host](../evidence/G3_REPLAY/host-running.png)/[client](../evidence/G3_REPLAY/client-running.png), 546×720 |
| 양쪽 최종 UI·지연·중복·재매치 | 보스 HP/승패·revision 일치, 복귀1회 | 수정 후 패키지 검수 NotRun | PIE의 Running 관찰을 최종 결과 증거로 확대하지 않음 |

통합 전 배치 대조에서 기존 G1 Title y146px와 상대 라벨 y240px(540×1170)이 A Status y114~230px와 Boss y238~259px에 겹쳤다. `558c168`에서 전투 Widget의 실제 viewport 존재에 맞춰 두 기존 라벨만 숨겼고, `82ce249`에서 probe 예외를 GameMode와 같은 Development/대소문자 규칙으로 맞췄다. 이후 실제 PIE 양쪽 화면은 공용 전투 상태와 보드를 확인했다. 요청 해상도540×1170과 실제 창 테두리 포함546×720을 구분한다.

첫 자연 규칙 Editor-game은10웨이브 BossTimeout 결과와 보스 HP467/3901을 관찰했지만 [종합 결과는 Fail](../../../docs/production/evidence/RUN-20260918-G3/first-editor-pair.json)이다. host16/client14 내부 검사나 HUD 재생성 기록으로 이를 Pass로 바꾸지 않는다. 결과 뒤 하단에 잘못 남은 참가자 대기 문구는 `28a5f2e`에서 종료 안내로 고쳤으며, 최종 수정 패키지 결과 화면 재확인은 남았다.

현재 `0e473f4`의 B Editor72.19초·57자동화 Pass와, 기존 조립 입력의 실제 PIE·G2 host213/client57 회귀를 [공통 기록](README.md#g3-evidence)에서 구분한다. G2 fixture의 HUD 재생성은 새 Result/Entry 반복 복귀 증거가 아니다. 첫 cooked 실행은 HUD까지 가기 전 Entry에서 PKG01로 실패했고 `501be90`의 스타일 소유권 수정 후 자동화는 통과했다. 실패 원인·수정 절차는 [Entry 수업](G3-01-entry-return.md)에만 관리한다. 최종 패키지5시드·20분 부하·Android는 NotRun이다.

## 상대에게 전달하고 통합하기

API: `GameState.GetBattleSnapshot`, `BattleStatusWidget.UpdateView`, `ResultWidget.UpdateView/OnReturnRequested/GetReturnButtonScreenRect`, `PlayerController.CanUseGameplayActions/GetReturnButtonScreenRect/RequestReturnToEntry`, `G1BoardWidget.SetBattleOverlayVisible`. A 계약 `e61c414`→B `d2183ae`→A 실제 위젯/타임라인 `61fb3a`(선행 `b425226`)→B 상단 연결 `558c168/82ce249`가 초기 통합 순서다. 현재 재현은 종료/HP·안내 문구·Entry 수정까지 포함한 `0e473f4` 전체를 사용한다. B 역할 빌드·자동화는 통과했으며 패키지 결과/복귀는 다음 통합 검수다.

## 이해 확인

1. 개인 경제 스냅샷과 전투 GameState 스냅샷이 서로 다른 이유는 무엇인가?
2. terminal 때 새 명령을 막으면서 이미 확정된 요청 캐시를 유지해야 하는 이유는 무엇인가?
3. 작은 변형: Result 위젯을 실제로 한 번 제거해 다음 tick에 다시 나타나는지 확인하고, 복귀 delegate 개수가 늘지 않았음을 실제 여행 횟수로 증명한다.
4. 클라이언트 HUD가 표시한 0초만 보고 패배를 결정하면 왜 보스 마감 타격 경계가 깨지는가?

## 단계 완료

- [x] 파일 순서·소유권·API·입력/구독 해제를 기록했다.
- [x] A 위젯 cpp와 최종 타임라인을 통합해 B 역할 Editor·자동화를 통과했다.
- [x] 새 재현본 실제 PIE의 양쪽 Running 화면과 종료 정리 범위를 확인했다.
- [ ] 실제 두 화면·terminal 입력·UI 재생성·복귀를 재현했다.
- [x] 시작점→현재 소스·이전 입력·수정 기록별 증거를 연결했다.

Verified 진입 조건은 현재 수정 입력으로 실제 패키지의 terminal 입력·결과 재생성·반복 복귀를 재현하는 것이다. Android SafeArea/터치는 별도 G4 검수다.
