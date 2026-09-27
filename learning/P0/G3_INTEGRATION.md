# 10웨이브·진입·반복 실행 — P0 / 통합 / G3

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-WAVE-01, TASK-NET-01, TASK-UI-01; [공통 계약](COMMON.md), [독립 기대값](../../docs/production/evidence/RUN-20260918-G3/REVIEW_PLAN.md) |
| 참고 자료 제작 상태 | Draft — 조립·Editor·57자동화·실제 PIE·최종 패키지5시드/회복 통과, 대표 부하 진행 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | f735b5889a5bd197e46d29bdfaa2b38c246d5ea6 / 검증 입력0981d07307112857dfdf0e91c79bcecdbcbc291b (실행 Config98727f0, 후처리 검사기만 갱신), C++는0e473f4af380506d209a95f7ec42eccf89c69df4와 동일 (게이트 완료 미확정) |
| 실제 개발 시작/완료 SHA | 미생성 / 미생성 |
| 필요한 상대 산출물·버전 | G2 통합, Schema2/Rules0.3.0; A Director/공용 상태/전투·결과 위젯, B 진입/복귀/Controller |
| 제공 코드 / 직접 작성할 코드 | 제공: 기존 JSON·UI v2·검사기·에셋 생성기. 직접 작성: A/B G3 수업의 런타임 코드와 독립 기대 검사. 참고 파일 조립은 학습자 구현 완료가 아님 |

## 이번에 만들 동작

시작 화면의 호스트/주소 참가로 두 사용자가 한 매치에 들어간다. 10초 준비 후 일반9웨이브와 보스10웨이브를 진행하고 승리·패배·중단 결과를 함께 표시한다. 결과에서 시작 화면으로 돌아와 같은 프로세스로 다음 매치를 시작한다. P0 수치·좌표·경제의 원본은 [COMMON](COMMON.md)에 연결된 현행 명세이며 여기에서 복제하지 않는다.

## 코드 작성 순서

1. A `Data/LDBattleTypes.h` → `Core/LDGameState.*`: 공용 상태·두 보스·서버 마감 시각·종료 원인을 값 스냅샷으로 정의한다. Phase 원본을 별도로 늘리지 않는다.
2. A `Battle/LDWaveDirector.*`: 다음 생성 cursor와 살아있는 ID 집합을 관리한다. Actor는 HP, GameState는 공용 표시 원본을 유지한다. 마지막 생성 완료·보스 둘 사망·일반0을 모두 확인한다.
3. A `Core/LDGameMode.*`: Loading/Preparing과 timeline을 연결한다. 이전 시각을 닫고 현재 시각의 명령을 받은 뒤 타격·보상·생성·한도·마감·승리 순으로 확정한다. 늦은 프레임도 사건 시각을 건너뛰지 않는다.
4. B `Network/LDCommandProcessor.*`: 외부 명령 clock guard를 해제한 뒤 대기 보상을 Drain하고 종료를 확정한다. 종료 접수는 먼저 닫고 이미 확정한 요청 캐시는 연결 종료까지 유지한다.
5. A `UI/LDBattleStatusWidget.*`, `LDResultWidget.*`: 서버 snapshot을 표시한다. UI의 0초나 애니메이션이 승패를 결정하지 않는다.
6. B `Core/LDGameInstance.*`, `LDEntryGameMode.*`, `LDEntryPlayerController.*`, `UI/LDEntryWidget.*`: 주소 검증 → 정상 travel → 네트워크 실패/결과 복귀를 구현한다. Browse callback 안에서 다시 Browse하지 않고 다음 tick에 복귀한다.
7. B `Core/LDPlayerController.*`: 전투/결과 위젯 생성·현재 상태 전달·해제·복귀 요청을 연결한다. 제거된 HUD는 한 개의 새 인스턴스로 재생성하고 종료 뒤 게임 명령은 막는다.
8. 통합에서 두 맵과 GameInstance를 연결한 후 Editor 컴파일 → 독립 자동화 → 실제 PIE → 패키지2프로세스 순으로 검사한다. `Verification`의 명시 옵션 fixture는 정상 플레이나 학습자 진행과 구분한다.

호출 흐름: 시작 UMG → EntryController → travel → Mode 서비스 생성/참가자 등록 → Loading 준비 확인 → timeline → Director/Combat → GameState 복제 → Controller 위젯 갱신 → 결과 버튼 → GameInstance 다음 tick 복귀. EndPlay는 접수·타이머·delegate·서비스를 정리한다. UI가 먼저 사라져도 서버 상태 소유권은 바뀌지 않는다.

## Unreal 설정 순서

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | `Content/LD/Maps/L_P0Entry` → World Settings | GameMode Override=`LDEntryGameMode` | 호스트 버튼, IPv4[:port] 입력, 참가 버튼 |
| 2 | `Content/LD/Maps/L_P0` → World Settings | 기존 `BP_LDGameMode`(부모 `LDGameMode`) | 개인 보드 아래, 공용 웨이브/시간/일반 수 상단 |
| 3 | Project Settings → Maps & Modes | Editor Startup Map/Game Default Map=`L_P0Entry`, Game Instance Class=`LDGameInstance` | 실행마다 최소 진입 화면, 매치간 복귀 상태 유지 |
| 4 | `LDEntryWidget::RebuildWidget` | native UMG, 버튼 → EntryController Host/Join | 별도 WBP 수작업 연결 없음 |
| 5 | `LDPlayerController::UpdateBattleView` | BattleStatus/Result native 위젯 생성; Result.OnReturnRequested → RequestReturnToEntry | 결과 원인과 복귀 버튼, 기존 제목 중복 표시 숨김 |
| 6 | `LDBattleStatusWidget::NativeTick` | v2 1080×2340 기준 Status=(240,228,600,232), Boss=(180,476,720,42); SafeZone 크기로 축소 | 보스 두 HP와 시간/웨이브가 보드 밖 상단에 표시 |
| 7 | 패키징 | `Build-P0Package.ps1`의 두 맵 cook, 기존 UFS JSON·폰트·M_P0Flat·S_P0Rejected | 학습 폴더 없이 데이터/에셋 로딩 |

에셋 생성은 `tools/Create-P0Assets.py`를 에디터에서 실행한다. 기존 에셋을 삭제하지 않으며 다른 GameMode를 발견하면 먼저 차이를 확인한다. native UMG 작성 위치는 각 역할 수업을 따른다.

## 실행·실패·수정 기록

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 첫 자연 규칙2프로세스, seed1776 | 규칙대로10웨이브 결과, 서버/양쪽 상태 일치 | 보스 시간초과 패배, N0, HP467/3901. 내부 host16/client14 Pass. launcher seed 로그 문구 불일치로 종합Fail 보존 | [첫 실행 요약](../../docs/production/evidence/RUN-20260918-G3/first-editor-pair.json); Editor-game, 패키지 아님 |
| 현재 프레임에 웨이브 전환·캡처 | 현재 HUD와 snapshot 일치 | host 캡처가 UMG 갱신보다 빨라 WAVE0. 전환 후0.5초 예약으로 수정 후 양쪽 WAVE1/20초/N2 관찰 | 실제 PIEv2 및 새 재현 PIE 양쪽 PNG 직접 대조 |
| 보스 패배 결과 | 종료 안내·비활성 조작·복귀 버튼 | 하단 참가자 대기 문구를 종료 안내로 수정 `28a5f2e`; 최종 결과 화면 재확인 진행 | 최초 실패 PNG 보존 |
| 새 재현본의 실제 GPU PIE 두 네트워크 월드 | 각소환1/gold80, Running, 종료시 월드0/설정복원 | 1Pass, 설정복원true; 양쪽546×720 화면에서 WAVE1/20초/N2·gold80/pop1 확인 | 재현 폴더 `Saved/P0Runs/Replay-G3-pie` 및 `-proof`; 제품 승리/10웨이브 검사 아님 |
| 부분 보스 생성 실패·정확 마감·한도·종료 재진입 | 부분 성공 게시/중복 보상/추가 공격 없이 일관 종료 | A01/A02/검사용 준비 순서 결함 수정 후56종 무경고Pass, 새 재현본도56Pass | [리뷰 기록](../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md) |
| 부하 출력 경로·2000회 수명 smoke | 실제 게임 World가 지정 폴더에 한 결과 저장, 실제 처치·정리 | 전환 World의 경로 선점으로v2 종합Fail. 출력 지연 초기화 후v3 host69/client33 및 종합Pass | [smoke v3](../../docs/production/evidence/RUN-20260918-G3/load-smoke-v3.json);10초 유지 Editor-game,20분 패키지 성능 아님 |

컴파일 실패·모의 자동화·실제 PIE·Editor-game·패키지·Android 결과를 합쳐 하나의 Pass로 쓰지 않는다. 전체 로그와 PNG는 각 Saved/P0Runs 새 RunId에 보존한다.

## 상대에게 전달하고 통합하기

A의 DTO/Director/Mode/위젯을 먼저 받고 B의 clock 완료 접점·Controller·Entry/GI를 연결한다. `learning/tools/Replay-P0G3.ps1 -RepositoryRoot C:/Users/iam12/P0_reference_integration -ReplayRoot <존재하지 않는 절대 경로> -SourceSha 0981d07307112857dfdf0e91c79bcecdbcbc291b -RunId <새 이름>`으로 기준 `f735b588`의 새 detached worktree에66파일을 수업 순서로 조립하고 blob을 대조한다.15개는 제공 기반,51개는 G3 최종 입력이다. 중간 파일 묶음은 다음 묶음을 참조하므로 전체 조립 후 빌드한다. 조립은 학습자의 직접 구현을 대신하지 않는다.

이번 실제 재현 경로는 `C:/Users/iam12/P0_lesson_replay_g3`다. 초기65파일 manifest와 제공 도구·Entry 스타일·효과음 cook 수정 후66파일 blob 대조를 [재현 증거](evidence/G3_REPLAY/README.md)에 보관했다. 패키지/Editor 실패는 보존하고 필요한 변경만 적용했다. 이 폴더 HEAD는 출발점 f735b588에 그대로 있으므로 실행 기록의 HEAD만 최종 코드로 해석하지 말고 반드시 manifest의 SourceSha/파일 blob을 함께 대조한다. 세 learn 브랜치는 최초 공통8c6856d에 유지하며 완성 코드를 병합하지 않는다.

통합 참고 프로젝트는 `C:/Users/iam12/P0_reference_integration/Mobile_defense_clone.uproject`이다. 에디터 Play의 Net Mode=Play As Listen Server, Number of Players=2로 설정하고 `L_P0`를 연다. 시작 화면 흐름은 별도 게임2프로세스 또는 패키지에서 첫 창 호스트, 둘째 창 `127.0.0.1` 참가로 확인한다. 기본 포트7777은 다른 실행과 겹치지 않게 한다.

자동 재현은 `tools/Build-P0Editor.ps1` → `tools/Test-P0Automation.ps1 -Filter LD.P0` → `tools/Test-P0PIE.ps1` → `tools/Build-P0Package.ps1 -Platform Win64` → `tools/Run-P0G3.ps1 -GameExecutable <새 패키지 실행파일>` 순이다. 모든 도구에 새 RunId를 지정한다. PIE 도구는 기존 세션을 대체하지 않고 별도 에디터에서2월드를 열고 원래 설정을 복원한다. G3 자연 실행은 정상 규칙의 자동 조작이며 `G3Load`의 HP/부하 고정 fixture와 구별한다.

## 이해 확인

- 보스 HP0인데 잔여 일반 적1이면 왜 즉시 승리할 수 없는가?
- 정확 마감 시각의 판매와 공격 순서가 바뀌면 결과가 어떻게 달라지는가?
- 종료 뒤 같은 요청의 원응답을 반환하면서 새 구매는 막는 이유를 설명하라.
- 작은 변형: 마감 D−0.001/D/D+0.001의 기대값 표를 먼저 작성하고 테스트만 변형하라. 제품의60초 규칙은 바꾸지 않는다.
- 작은 변형: 위젯을 Running/Result에서 제거·재생성해 서버 시계와 자원 원본이 유지되는지 관찰하라.

## 단계 완료

- [x] 새 출발점 조립·빌드·실행을 재현하고 입력 manifest/검증 소스 SHA를 고정했다. 게이트 완료 SHA는 대표 부하·최종 문서 확인 후 기록한다.
- [x] 최종 패키지2인·5시드·600초 이상 지연/손실·회복·중복·반복 매치를 확인했다. [정식 실행 증거](../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md).
- [ ] 대표 부하20분과2000회 실제 수명·정리·메모리를 측정했다.
- [ ] 독립 구조 리뷰·학습 문서·작업 보드·검수 기록을 동기화했다.
- [x] Android 실기기는 별도 G4 NotRun이며 P0 최종 완료와 혼동하지 않는다.

G3 검수가 남아 Draft다. 학습자는 Planned이며 실제 진행 기록은 별도로 작성한다. P0 완료 전 P1/P2로 확장하지 않는다.
