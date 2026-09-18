# 전투·경제 연결 — P0 / 통합 / G2

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-COMBAT-01, TASK-ECON-01, TASK-BOARD-01, TASK-NET-01; [공통 구현 계약](../../docs/technical/IMPLEMENTATION_SHARED.md), [독립 기대값](../../docs/production/evidence/RUN-20260918-G2/REVIEW_PLAN.md) |
| 참고 자료 제작 상태 | Draft |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | 4861b987f3e2fe78bcc159d1b6a85008543a938b / 실행·재현 완료 후 고정 |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | Schema2/Rules0.3.0, G1 카메라·경로 통과; A Unit/Combat/Mode와 B Board/Economy/Processor/Controller/HUD |
| 제공 코드 / 직접 작성할 코드 | 제공: G1 공통 기반·기존 UI v2·JSON·단색 재질·검사기·100ms 알림음. 직접 작성: A/B 역할 수업의 런타임 코드·독립 기대 검사. 완성 참고본 복원은 학습자 직접 구현 완료가 아님 |

## 이번에 만들 동작

각 참가자가 자기 UMG 소환 버튼을 누르면 서버가 개인 재화와 보드를 함께 확정하고, 배치된 유닛이 공용 적을 공격한다. 한 번의 사망은 양쪽 개인 경제에 한 번씩 보상을 준다. 뭉치의 이동·보충은 기존 개체와 공격 시계를 유지한다. 제품 규칙은 [COMMON](COMMON.md)과 연결된 현행 명세가 원본이다. 이 수업에서는 연결 순서와 실패 관찰만 추가한다.

G2 실행 픽스처는 정지 HP70 N01 한 마리와 구매 자금을 얻기 위한 HP1 적100마리를 사용한다. 보드·경제 값을 직접 쓰지 않고 실제 공격과 보상을 거친다. 이는 10웨이브 플레이나 밸런스 검수의 대체물이 아니다. 정상 실행에는 `-P0Probe=G2`를 붙이지 않는다.

## 코드 작성 순서

1. `Board/LDBoardTypes.h`, `Economy/LDEconomyTypes.h`, `Battle/LDCombatEvents.h`의 값 전달 계약부터 작성한다. MatchId·PlayerIndex·InstanceId·Revision과 서버 시각을 함께 넘긴다.
2. A가 `Battle/LDUnitActor.*` 준비/확정/표시를 작성하고 B가 이를 `LDBoardManager`의 준비 함수에 주입한다. 준비 실패는 경제 확정 전에 취소한다. UI와 전투 조회에는 확정된 Actor만 나타난다.
3. B가 `LDEconomyService`의 복사 RNG 계획과 `LDBoardManager`의 배치 계획을 작성한다. `LDCommandProcessor`는 두 계획을 재검사→두 원본 확정→응답 캐시→게시한다. 외부 delegate를 공동 확정 구간에 호출하지 않는다.
4. A가 `LDCombatRules`→`LDEnemyActor` HP→`LDCombatService` 순으로 연결한다. 예정 타격 시각의 canonical 위치로 거리를 검사하며 표시 위치를 논리로 되돌리지 않는다. `NextAttackAt` 원본은 CombatService 하나뿐이다.
5. `LDGameMode`가 서비스를 소유·주입하고 BoardCommit을 등록/해제·배치 갱신으로 연결한다. Death를 Processor에 넣어 중복 제거 후 양쪽 경제를 정산한다. 외부 명령 앞에서는 `<명령시각` 타격/보상을 먼저 처리하고, 같은 시각의 타격은 명령 뒤에 남긴다.
6. `LDLocalPresentationSubsystem`이 기존/새 UnitActor에 로컬 참가자 표시 변환을 전달한다. root canonical 좌표와 로컬 Y반사를 혼동하지 않는다.
7. B의 Controller는 개인 `{ConnectionEpoch, Board, Economy}` 단일 owner-only envelope를 관찰한다. 성공·만료 응답의 두 Revision을 모두 관찰하기 전 다음 조작을 열지 않는다. HUD는 이 복사본만 읽고 가격/RNG/보상을 결정하지 않는다.
8. `LDGameplayWidget`의 실제 버튼·보드 선택/드래그·실패 피드백을 연결한다. NativeDestruct는 클릭 delegate를 해제하고 Controller는 제거된 HUD를 새로 만든다. 미응답 명령은 위젯 수명과 무관하게 같은 키로 재확인한다.
9. 독립 자동화 후 `Verification/LDG2ProbeSubsystem.*`로 두 프로세스 실행을 검증한다. 이 코드는 Development 명시 옵션에서만 생성되고 런타임 제품 상태 원본을 대신하지 않는다.

핵심 흐름: 로컬 UMG/입력 → 소유 Controller RPC → 참가자/세대/중복 검사 → 더 이른 타격·보상 → 보드/경제 계획 → 공동 확정 → 원응답 캐시 → GameMode 등록·표시 스냅샷 → 복제 확인 후 UI 완료. 종료는 접수 닫기→마지막 보상 Drain→타이머/구독/서비스 정리이며 확정 응답 캐시는 연결 종료까지 남긴다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `Content/LD/Maps/L_P0` → World Settings | G1과 동일 BP_LDGameMode; native ALDGameMode가 서버 서비스를 생성 | 두 참가자 준비 후 Running, G1 Probe만 Preparing 유지 |
| 2 | Unit/Enemy C++ 기본 컴포넌트 | CanonicalRoot 아래 engine Cube/Sphere; M_P0Flat MID Color | 논리 중심은 셀, 모델만 뭉치 슬롯/0.15초 이동 표시 |
| 3 | `LDGameplayWidget::RebuildWidget` | native UUserWidget; WidgetTree SafeZone→Canvas→Text/Button; 별도 WBP 없음 | 선택·합성·판매·재화·중앙 소환을 기존 v2 위치에 표시 |
| 4 | HUD NativeConstruct | Summon.OnClicked→OnSummon→Controller.RequestSummon; Merge/Sell도 같은 순서 | 버튼이 서버 명령을 만들고 보드 입력으로 전파되지 않음 |
| 5 | HUD NativeTick | v2 1080×2340 좌표를 SafeZone Canvas 크기로 축소; 소환 Rect=(336,1940,408,220), 합성=(304,1718,268,54), 판매=(588,1718,268,54) | 화면 크기와 실제 버튼 hit rect 일치 |
| 6 | `Content/LD/Audio/S_P0Rejected` | tools/Create-P0FeedbackAudio.py로 생성된 48kHz/mono/100ms/220Hz SoundWave; PC의 SoftObjectPtr 경로로 로드 | 최종 거절 한 번에 붉은 버튼·메시지·알림음. 기존 에셋을 자동 덮어쓰지 않음 |
| 7 | Project Packaging | G1 데이터 UFS·맵·재질 유지; 알림음은 런타임 SoftObjectPtr 경로에 포함 | 패키지에서 데이터/에셋 로드 검사는 G3 최종 패키지로 다시 확인 |

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| A/B 공용 UnitActor 병합 | 같은 파일 선택 후 빌드 | 동일 blob 확인 후 해결. 최초 shell이 Git 오류 뒤 빌드를 계속해 conflict marker 컴파일 실패; 이후 exit code guard 적용 | Saved/P0Runs/G2-services-editor-initial, G2-services-editor-fix1 |
| 첫 통합 자동화 | 전투·명령·기존 게이트 모두 통과 | 31중30Pass, G0의 옛 Stub 문구 기대1Fail. 참가자·Phase 의미 검사로 수정 | [첫 자동화 요약](../../docs/production/evidence/RUN-20260918-G2/automation-initial.json) |
| 예정 타격·명령 순서 | 10.025 타격/보상은10.04 명령 앞,10.025 명령은 동시 타격 앞 | 정밀 `<t`/`<=t` 분리 후33Pass/0Fail | Saved/P0Runs/G2-ui-clock-automation-fix1; 화면 검사 아님 |
| UI Editor 빌드 | native UMG 컴파일 | 부모 Visibility를 숨긴 지역 변수 C4458; OverlayVisibility로 변경 후 Pass | Saved/P0Runs/G2-ui-editor, G2-ui-clock-editor-fix1 |
| 첫 소환→처치, 뭉치·판매·합성 | 실제 버튼·RPC·개체·경제가 일치 | 첫 두 프로세스17단계 Pass. HUD 제외 캡처와 응답/추가 Actor 검사 누락을 발견해 증거 범위를 제한하고 검사기를 보완 | Saved/P0Runs/G2-two-process-initial; HUD 최종 증거로 사용하지 않음 |
| 보완 후 HUD·만료·재생성 | HUD 포함 PNG, 누락 응답/유령 Actor 검출, 양쪽 새 HUD 클릭 한 번 | 재검증 중 | 최종 고정 후 증거 연결 |

전체 실패 분석·파일/함수·리뷰 상태는 [REVIEW_FINDINGS](../../docs/production/evidence/RUN-20260918-G2/REVIEW_FINDINGS.md)에서 관리한다. 전체 로그는 Saved/P0Runs에 보존하며 정적 검사, Editor 컴파일, NullRHI 자동화, 실제 GPU 실행을 섞지 않는다.

## 상대에게 전달하고 통합하기

시작점은 G1 통합 `4861b987`이다. A/B 수업 순서로 파일을 조립하고 공통 DTO를 비교한 뒤 최종 통합 소스를 하나로 사용한다. learn 브랜치는 `8c6856d`를 유지하며 이 완성본을 병합하지 않는다. 역할 완료 SHA·통합 완료 SHA·재현 helper 명령은 최종 검증 후 여기에 고정한다.

통합 폴더에서 `pwsh -File tools/Build-P0Editor.ps1 -RunId <새이름>`, `pwsh -File tools/Test-P0Automation.ps1 -Filter LD.P0 -RunId <새이름>`, `pwsh -File tools/Run-P0Pair.ps1 -Probe G2 -RenderOffscreen -RunId <새이름>` 순으로 실행한다. RunId가 이미 있으면 다른 이름을 쓰며 이전 증거를 삭제하지 않는다. 실제 창 조작은 마지막 명령의 RenderOffscreen을 생략한다. 픽스처는 완료 후 자신이 만든 프로세스만 종료한다.

## 이해 확인

- 준비한 Actor가 있는데도 확정 조회에서 보이지 않아야 하는 이유는 무엇인가?
- 처치 보상이 소환 검증보다 늦으면10.04 구매 결과가 어떻게 달라지는가?
- 같은 ConnectionEpoch의 캐시 응답과 새 epoch 초기 스냅샷을 왜 따로 검사하는가?
- 작은 변형: 픽스처의 첫 적 HP를71로 바꾸고 타격 횟수 기대를 먼저 계산하라. 가격·드롭률·제품 데이터를 바꾸지 말고 별도 연습 커밋으로 남긴다.
- 작은 변형: 이동 직전 공격 예약이 잠금보다 늦은 경우를 재현하고 기존 타이머가 유지되는지 검사한다.

## 단계 완료

- [ ] 수업 순서의 새 G1 출발점 조립·빌드·실행 재현을 확인했다.
- [ ] 실제 화면·명령 결과·시작/완료 SHA를 연결했다.
- [ ] 코드·학습 자료·보드·검수 기록과 독립 리뷰를 동기화했다.
- [x] 실제 학습자 진행은 Planned이며 참고 제작과 분리했다.
- [x] G3의10웨이브·보스·PC 최종 패키지·지연·반복 플레이와 G4 실기기는 별도 미검증이다.

G2 차단 결함 수정과 재현 통과 전 G3 전투/웨이브 확장을 시작하지 않는다.
