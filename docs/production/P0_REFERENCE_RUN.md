# P0 참고 구현 작업 기록 — 2026-09-18

이 기록은 이번 작업의 공통 계약·입력·검증 색인이다. 과거 실행의 Pass를 승계하지 않는다. 참고 제작과 실제 학습자의 진행은 별개다.

## 출발점과 보존

- 원래 HEAD: `a93138791dba9854f8be42a9480c652c03b926d6`, 원래 브랜치 `docs/operations-baseline`.
- 공통 출발 SHA: `8c6856d235de87cc28c12b49ca775bd0937334a5`.
- 실제 변경 157개(31,048,281 bytes)를 파일별 SHA256/사본으로 먼저 보존했다. 원본 프로젝트 `Saved/P0Runs/20260918-start/input-manifest.json`, `input/`, `working.patch`, `selected-inputs.txt` 참조.
- 포함: 현재 기획/결정/데이터/검증 도구, UI 자료, 학습 운영 양식과 기존 P1/P2 목차, 해당 문서의 근거 링크가 가리키는 과거 기록, 관련 README/커밋·리뷰 규약. 기존 P1/P2 자료는 입력 보존이며 구현 확장이 아니다.
- 제외: 생성 Binaries/Intermediate/Saved/DerivedDataCache, 사용자 IDE 설정, outputs. 기존 사용자 파일을 삭제하거나 수정하지 않았다. 추적 가능한 미커밋 입력 중 과제와 무관한 파일은 발견되지 않았다.
- 원본 폴더는 출발점 보존용이다. 참고 작업 폴더: `C:/Users/iam12/P0_reference_a`, `P0_reference_b`, `P0_reference_integration`.
- `reference/p0-a`, `reference/p0-b`, `reference/p0-integration`은 각각 별도 worktree. `learn/p0-a`, `learn/p0-b`, `learn/p0-integration`은 출발 SHA에 고정, 완성 코드 병합 없음. 실제 학습자 상태 Planned.

## 최초 확인 계약과 근거

| 계약 | 현행 원본 / 적용 |
|---|---|
| 범위·게이트 | `docs/design/P0_REPLAN.md`; G1 실제 화면/입력 통과 전 G2 전투 확장 금지 |
| 충돌 우선순위 | `docs/DECISIONS.md` DEC-039~046 → 기능 명세 → 기술 설계. BACKLOG의 옛 8종 표현은 DEC-043의 16종으로 해석 |
| 버전·범위 | SchemaVersion 2 / RulesVersion 0.3.0; P0 활성 16종, FinalWave 10, 고유 스킬·강화·신화 비활성 |
| 좌표 | CellId=Board*18+Row*6+Column; 0~17/18~35; 해상도와 무관. 월드 칸/길140cm, 경로3080cm, 로컬 표시 칸/길120px 기준 |
| 경로 | `data/GameRules.json` Paths.PointsByGateCm; RouteIndex 고정, 중앙 (490,0)→(-490,0), 자기 보드로 복귀 |
| 상태·API | `docs/technical/IMPLEMENTATION_SHARED.md`; GameState 매치, EconomyService 재화/RNG, BoardManager 개체/칸, EnemyActor HP/경로. GameMode가 연결 |
| 수명·원자성 | 준비→공동 확정→게시; 실패 시 RNG 포함 불변; 서버 신원/세대; 요청·처치 중복 방지; 종료 시 구독/예약 정리 |
| 검수 기대값 | `docs/BACKLOG_QA.md` QA-RNG/ECO/BOARD/TIME/NET 및 P0_REPLAN의 PLAN 기준 |
| 구조 | `docs/technical/CODING_STANDARD.md` ARCH-01~06; 분리·의존·원본·원자성·수명·실행 검수 |
| 학습 | `learning/WORKFLOW.md`, `learning/templates/LESSON.md`; 재현 확인 전 Draft, 학습자는 별도 Planned |
| 도구 | UE5.8.2 CL56702186, Unreal MCP toolsets 실제 열거 확인. 빌드/에디터/기기/공용 설정은 통합 담당이 직렬 사용 |

## 이번 실행 상태

| 구분 | 결과 | 증거 / 남은 의존성 |
|---|---|---|
| 출발 문서·데이터·스타일 | Pass | 원본 Saved/P0Runs/20260918-start/check-project.log; 문서6601 checks. 게임 실행 증거 아님 |
| G0 | Pass | A/B/통합 Editor Pass, A4/B4/통합12 자동화 Pass. 초기화·종료 2건 a02efc1 수정·재리뷰 완료. 실제 RPC/PIE는 G1~G3 검수 |
| G1 | Pass | 소스 df8a2f2. 새 G0 재현 Editor·22자동화·실제2프로세스7화면비·전체 셀·2바퀴 Pass. 카메라/표시/검사기 실패 수정·리뷰 완료 |
| G2 | Pass | 시작4861b987→제품5baa960/검사포함ae6be1b. 새56파일 조립·Editor·39+보충12 무경고Pass(44종); 실제GPU20단계 host213/client57 Pass. 독립 리뷰 차단0; [증거](evidence/RUN-20260918-G2/SUMMARY.md) |
| G3 | InProgress | 2026-09-27 재개. G2 통합 f735b5889a5bd197e46d29bdfaa2b38c246d5ea6에서 A 웨이브/시간/결과, B HUD/진입/복귀 통합. 종료 재진입·보스 HP 반영·검수용 준비 순서 수정 후 e4a02a4의 독립 자동화56종 무경고 Pass. 실제 GPU PIEv2 통과 범위 별도. 최종 패키지/5판/20분 부하/새 수업 재현 진행 중 |
| G4 | NotRun | SDK36/BuildTools36.0.0/NDK27.2.12479018/JDK21.0.3 준비. Android 실제 빌드 exit6: UE Android 선택 구성 요소 누락. 사용자가 설치 진행, adb 장치0 |

원격 push/PR 병합/외부 배포/기존 작업 삭제는 수행하지 않는다. 전체 로그는 Saved/P0Runs에 저장하고 핵심 결과는 이 기록과 정식 검수 기록에 연결한다.

## 2026-09-27 재개와 G3 연결

원래 출발점과 세 learn 브랜치8c6856d, A/B 독립 worktree를 보존했다. 중단 직전 B 수업 커밋1ba9613을 검토·통합하고 문서6937 checks/스타일55파일/학습415링크 오류0을 확인했다. G2 전체 재빌드는 소스 변경 근거가 없어 반복하지 않는다. 시작시 존재하던 프로젝트 인수 없는 UnrealEditor PID52900은 종료하지 않았다.

공용 API는 `Data/LDBattleTypes.h`의 단일 FLDBattleSnapshot과 `GameState.GetBattleSnapshot()`이다. A Director는 일정 cursor/살아있는 ID 집합, GameState는 서버 공용 상태/종료 결과, Actor는 HP를 소유한다. B가 A의 BattleStatus/Result native UMG를 만들고 수명을 관리한다. P0 최소 진입 화면은 `L_P0Entry`→호스트/IPv4 참가→`L_P0`→결과 복귀이며 정식 로비 기능을 확대하지 않는다.

시간 구현 선택: 실제 listen 매치 Loading 진입부터30초, 두 참가자/필수 준비 완료부터 Preparing10초. 진입 화면 대기는 활성 매치가 아니다. 현재 WorldTime은 명령/준비 처리에 열고 이전 시각만 닫으므로 정확 로딩 마감의 준비는 마감 확정 전 허용한다. 이미 확정한 timeout은 늦은 접속으로 취소하지 않는다. 보스 마감·생성·승리도 같은 timeline에 포함한다. 명령 clock hook 종료는 접수닫기→guard해제→보상Drain→최종 결과/Close 순서이며 캐시응답은 유지한다.

네트워크 도구 근거: UE5.8 `Engine/Private/Net/NetEmulationHelper.cpp::FPacketSimulationSettings::ParseSettings`가 PktLagMin/Max와 PktLoss를 지원함을 로컬 엔진 소스로 확인했다. 각 endpoint 송신 지연75/150ms로 왕복150/300ms를 요청하며 실제 echo RTT·전송 통계를 별도 기록한다. 자동 플레이는 소환/합성/판매 Slate 버튼·정상 이동 의도를 사용하고 재화/HP/시간을 바꾸지 않는다. 별도 부하 fixture와 자연 규칙 플레이는 구분한다.

Android 재확인: UE5.8 Binaries에Android 없음·adb0. 사용자가 Launcher 구성 요소 설치와 USB 디버깅 허용 후 기기 모델을 알려주도록 요청했고 독립 PC 작업을 진행한다. G4 Pass 아님.

## G1 실행·재현·통합

- 최종 소스 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`; G0 `649c1de`의 새 detached `C:/Users/iam12/P0_lesson_replay_g1`에 수업 순서36파일 조립/blob 대조. 기존 G0 재현·원본·learn 브랜치는 보존했다.
- 역할 Editor: A Pass, B 지역 UMG Slot 이름 숨김 수정 후 Pass. 통합 컴파일 fixture 이름/정수 타입 실패 수정 후 Pass. 최종 통합 Editor와 새 재현 Editor Pass, 재현88.39초.
- 실제 실패: 세로 FOV축 확대→MaintainXFOV, host 표시 되감김→현재 view clock, resize/터치 단계 덮어쓰기→완료 조건 전환, 가로 자동 높이 보정→명시false, 축소 셀 경계 소실→표시 간격 최소1.5px. 새 카메라 테스트의 미활성 fixture도 초기화 보완 후 통과. [파일·재현·수정·리뷰](evidence/RUN-20260918-G1/REVIEW.md).
- 최종 실제 별도2프로세스: host1338/client1336 Pass, 차이2개는 서버 생성 검사. 양쪽7해상도·자기18칸/상대18칸·72내부모서리·EngineTouch·ID/RouteIndex2바퀴를 확인했다. [요약·PNG 해시](evidence/RUN-20260918-G1/final-pair-summary.json). NullRHI 자동화22Pass와 GPU 화면 검사를 구분했다.
- 수업 재현은 참고 제작이며 실제 학습자는 Planned. 전체 로그/14PNG는 재현폴더 Saved/P0Runs/Replay-G1-*. PIE·PC 패키지·전투·경제·물리 터치·Android는 이 통과 범위가 아니다.
- 적2개 fixture의60FPS 제한 P95는 양쪽16.667ms. 측정 환경은 아래와 같으며 대표 부하 측정은 G3에서 수행한다.
- 후속 Win64 Development 패키지158.70초 Pass, 실제 패키지 별도2프로세스 G1 host1338/client1336 Pass. [에셋/데이터 기반 패키지 결과](evidence/RUN-20260918-G1/package-pair-summary.json). G2/G3 코드가 추가되기 전 G1 기반만 검증했으며 최종10웨이브 패키지는 후속이다.

## G0 실행·리뷰 관찰

- A 독립 구현 `0bab1ad` → 컴파일 실패(C++ 문자열 포인터 덧셈) 수정 `4cc3e0f`: Editor Pass, 데이터 자동화4 Pass. 증거: A worktree `Saved/P0Runs/G0-A-editor-fix1`, `G0-A-tests`.
- B 독립 구현 `18b1ace` → W10 보스 전용 행의 일반 생성 간격0을 거절하던 결함 수정 `03acb67`: Editor Pass, 명령·데이터 자동화4 Pass. 증거: B worktree `Saved/P0Runs/G0-B-editor-fix1`, `G0-B-tests-fix1`.
- 통합 `3b4105e`: Editor Pass(26.89초), NullRHI 자동화9 Pass. 증거: 통합 `Saved/P0Runs/G0-integration-editor`, `G0-integration-tests`. 화면·입력·네트워크 실행 증거가 아니다.
- 독립 리뷰: GameMode.StopMatchServices가 Controller 문맥까지 해제하여 같은 연결의 종료 후 재전송에 캐시 원응답 대신 PhaseNotAllowed를 반환. 접수 종료와 연결 해제를 분리하고 Mode/Controller 경로 회귀 추가 중.
- 독립 리뷰: 서비스 준비 전 PostLogin 참가자가 누락됨. 약한 대기 목록과 준비 완료 재연결로 순서 독립·중복 초기화 방지를 검사 중.
- 에셋 생성: `/Game/LD/Maps/L_P0`, `/Game/LD/Core/BP_LDGameMode`(native LDGameMode 부모). 시작/기본 맵·GameMode를 연결했다. 에디터 생성·저장만 확인했으며 실제 화면은 G1에서 검수한다.
- 구성 이유: 런타임 JSON은 `Content/LD/Data`의 UFS로 패키징한다. AndroidFileServer는 내장 APK+adb 경로에 필요 없어 비활성화했고 자동 토큰 생성을 차단한다.

<a id="측정-환경과-한계"></a>
## 측정 환경과 한계

Windows11 Pro 10.0.26200, Ryzen5 7500F(6C/12T), RAM32GiB, RTX4060Ti(driver32.0.15.9186), UE5.8.2 CL56702186, MSVC14.50.35738, WindowsSDK10.0.26100. 컴파일·NullRHI 자동화 시간은 게임 프레임 성능이 아니다. 대표 부하의 렌더링·네트워크·Android 성능은 아직 측정하지 않았다.

## 2026-09-27 G3 중간 실행 증거

- f4ad32c 통합 Editor34.56초·LD.P0 54Pass/0Warning/0Fail. A d64af106 역할 Editor50.06초·Waves7Pass/0Warning. 후속 리뷰 결함은 이 통과로 덮지 않는다.
- 실제 PIEv2(2f144fe): listen/client2World, 각소환1·골드80, 준비→Running, 명시종료→World0·설정복원. 양쪽546×720 PNG 직접 관찰에서 WAVE1/20초/N2가 일치했다. 패키지/10웨이브 증거가 아니다.
- 최초 자연 Editor-game2프로세스는 각1판10웨이브BossTimeout/N0/보스HP467·3901. 내부host16/client14Pass지만 launcher seed로그정규식 오류로 종합Fail을 보존했다. 수정후 최종패키지재실행예정. 자동전략이며 학습자 플레이가 아니다.
- 문서·데이터 검사Pass, 스타일77파일오류0, 학습55문서457링크오류0. 실행로그는Saved/P0Runs/G3-*, 요약은docs/production/evidence/RUN-20260918-G3에분리보존한다.
