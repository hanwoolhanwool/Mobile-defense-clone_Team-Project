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
| G2 | NotRun | G1 실제 실행/양쪽 화면 통과 의존 |
| G3 | NotRun | G2 의존, PC 패키지 별도 두 프로세스·5판 필요 |
| G4 | NotRun | SDK36/BuildTools36.0.0/NDK27.2.12479018/JDK21.0.3 준비. Android 실제 빌드 exit6: UE Android 선택 구성 요소 누락. 사용자가 설치 진행, adb 장치0 |

원격 push/PR 병합/외부 배포/기존 작업 삭제는 수행하지 않는다. 전체 로그는 Saved/P0Runs에 저장하고 핵심 결과는 이 기록과 정식 검수 기록에 연결한다.

## G1 실행·재현·통합

- 최종 소스 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`; G0 `649c1de`의 새 detached `C:/Users/iam12/P0_lesson_replay_g1`에 수업 순서36파일 조립/blob 대조. 기존 G0 재현·원본·learn 브랜치는 보존했다.
- 역할 Editor: A Pass, B 지역 UMG Slot 이름 숨김 수정 후 Pass. 통합 컴파일 fixture 이름/정수 타입 실패 수정 후 Pass. 최종 통합 Editor와 새 재현 Editor Pass, 재현88.39초.
- 실제 실패: 세로 FOV축 확대→MaintainXFOV, host 표시 되감김→현재 view clock, resize/터치 단계 덮어쓰기→완료 조건 전환, 가로 자동 높이 보정→명시false, 축소 셀 경계 소실→표시 간격 최소1.5px. 새 카메라 테스트의 미활성 fixture도 초기화 보완 후 통과. [파일·재현·수정·리뷰](evidence/RUN-20260918-G1/REVIEW.md).
- 최종 실제 별도2프로세스: host1338/client1336 Pass, 차이2개는 서버 생성 검사. 양쪽7해상도·자기18칸/상대18칸·72내부모서리·EngineTouch·ID/RouteIndex2바퀴를 확인했다. [요약·PNG 해시](evidence/RUN-20260918-G1/final-pair-summary.json). NullRHI 자동화22Pass와 GPU 화면 검사를 구분했다.
- 수업 재현은 참고 제작이며 실제 학습자는 Planned. 전체 로그/14PNG는 재현폴더 Saved/P0Runs/Replay-G1-*. PIE·PC 패키지·전투·경제·물리 터치·Android는 이 통과 범위가 아니다.
- 적2개 fixture의60FPS 제한 P95는 양쪽16.667ms. 측정 환경은 아래와 같으며 대표 부하 측정은 G3에서 수행한다.

## G0 실행·리뷰 관찰

- A 독립 구현 `0bab1ad` → 컴파일 실패(C++ 문자열 포인터 덧셈) 수정 `4cc3e0f`: Editor Pass, 데이터 자동화4 Pass. 증거: A worktree `Saved/P0Runs/G0-A-editor-fix1`, `G0-A-tests`.
- B 독립 구현 `18b1ace` → W10 보스 전용 행의 일반 생성 간격0을 거절하던 결함 수정 `03acb67`: Editor Pass, 명령·데이터 자동화4 Pass. 증거: B worktree `Saved/P0Runs/G0-B-editor-fix1`, `G0-B-tests-fix1`.
- 통합 `3b4105e`: Editor Pass(26.89초), NullRHI 자동화9 Pass. 증거: 통합 `Saved/P0Runs/G0-integration-editor`, `G0-integration-tests`. 화면·입력·네트워크 실행 증거가 아니다.
- 독립 리뷰: GameMode.StopMatchServices가 Controller 문맥까지 해제하여 같은 연결의 종료 후 재전송에 캐시 원응답 대신 PhaseNotAllowed를 반환. 접수 종료와 연결 해제를 분리하고 Mode/Controller 경로 회귀 추가 중.
- 독립 리뷰: 서비스 준비 전 PostLogin 참가자가 누락됨. 약한 대기 목록과 준비 완료 재연결로 순서 독립·중복 초기화 방지를 검사 중.
- 에셋 생성: `/Game/LD/Maps/L_P0`, `/Game/LD/Core/BP_LDGameMode`(native LDGameMode 부모). 시작/기본 맵·GameMode를 연결했다. 에디터 생성·저장만 확인했으며 실제 화면은 G1에서 검수한다.
- 구성 이유: 런타임 JSON은 `Content/LD/Data`의 UFS로 패키징한다. AndroidFileServer는 내장 APK+adb 경로에 필요 없어 비활성화했고 자동 토큰 생성을 차단한다.

## 측정 환경과 한계

Windows11 Pro 10.0.26200, Ryzen5 7500F(6C/12T), RAM32GiB, RTX4060Ti(driver32.0.15.9186), UE5.8.2 CL56702186, MSVC14.50.35738, WindowsSDK10.0.26100. 컴파일·NullRHI 자동화 시간은 게임 프레임 성능이 아니다. 대표 부하의 렌더링·네트워크·Android 성능은 아직 측정하지 않았다.
