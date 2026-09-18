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
| G0 | InProgress | A/B 공통 기반 독립 구현, 통합 비교·3개 Editor 빌드 대기 |
| G1 | NotRun | G0 의존 |
| G2 | NotRun | G1 실제 실행/양쪽 화면 통과 의존 |
| G3 | NotRun | G2 의존, PC 패키지 별도 두 프로세스·5판 필요 |
| G4 | NotRun | SDK36/BuildTools36.0.0/NDK27.2.12479018 설치 확인, adb 장치0. 사용자 USB 디버깅 연결 요청 |

원격 push/PR 병합/외부 배포/기존 작업 삭제는 수행하지 않는다. 전체 로그는 Saved/P0Runs에 저장하고 핵심 결과는 이 기록과 정식 검수 기록에 연결한다.
