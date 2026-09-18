# P0 구현 초기화 — 2026-09-16

[현재 진행](PROJECT_STATUS.md) · [새 P0 계획](../design/P0_REPLAN.md) · [단계 로드맵](ROADMAP.md#p0-gates) · [검수 기록](TEST_RUNS.md)

## 현재 상태

사용자 요청으로 기존 P0 참고 구현·A/B 구현 브랜치·학습 작업을 삭제했다. 새 P0 기능과 학습 자료는 **미착수**다. 현재 열 프로젝트는 `C:/Users/iam12/Mobile_defense_clone/Mobile_defense_clone.uproject` 하나다. 기본 Unreal 템플릿과 최신 기획에서 다시 시작한다.

## 제거한 범위

- `Mobile_defense_reference_p0`: 전투·경제·보드·HUD·경로 C++, LD 맵/에셋, 전용 도구·테스트·패키징 결과와 P0 수업/캡처를 포함한 작업 폴더 전체.
- `Mobile_defense_learn_p0_a`, `Mobile_defense_learn_p0_b`, `Mobile_defense_learn_p0_integration`: 학습 작업 폴더 전체.
- 로컬 브랜치 8개: `reference/p0-a`, `reference/p0-b`, `reference/p0-common`, `reference/p0-integration`, `learn/p0-a`, `learn/p0-b`, `learn/p0-base`, `learn/p0-integration`.
- 남은 기본 프로젝트의 P0 A/B 학습 목차·공통·통합 자료 디렉터리. 새 수업이나 완료 SHA로 대체하지 않았다.
- 삭제 대상의 열린 Unreal 에디터를 종료하고 최근 프로젝트 목록의 해당 경로를 정리했다.

## 보존한 출발점

- 최신 게임 기획·촬영본 18쌍과 분석·P0/P1/P2 로드맵·A/B 역할 및 향후 구현 설계.
- 기본 프로젝트/모듈, TopDown·Strategy·TwinStick 템플릿, 설치된 개발 도구와 기존 환경 설정.
- 구현 전부터 있던 기획용 JSON 예시·생성기·문서/데이터 검사 도구. 이 데이터는 새 기획을 구현한 결과가 아니며 새 로더/스키마와 함께 검토해야 한다.
- P1/P2의 미작성 학습 계획과 공통 수업 양식. 검증된 새 P0가 생기기 전까지 진입 대기다.
- 기존 Git 커밋 이력과 기본 프로젝트에 보관된 결정·검수 이력. 과거 통과는 새 P0 완료 근거가 아니다. 원격 `reference/p0*`·`learn/p0*` 브랜치는 조회 결과 없었다.

TASK-CORE-01의 Done은 기존 Windows 환경·템플릿 실행 준비 기록이므로 유지한다. 게임 기능 10개는 Backlog이며 P0 완료 상태가 아니다.

## 다시 시작할 순서

1. G0: 새 출발점에서 A/B가 공통 계약을 맞추고 각자 기반을 작성한다. 현재/최대 몬스터 수·RouteIndex·보스 마감·단일 결과의 데이터/복제 계약을 먼저 정의한다.
2. G1: 3D 전장, 두 화면의 왼쪽 생성, 중앙 같은 방향 공동 공격, 각자 자기 보드 복귀와 전체 셀 입력을 검증한다.
3. G2~G4: 핵심 기능 → PC 2인 → Android 실기기 순서로 구현·검수하고 새 실행에 근거해 A/B 학습 자료를 다시 만든다.

이번 요청의 완료 범위는 기존 P0 제거와 출발점 정리다. 신규 브랜치·게임 코드·학습 수업은 아직 만들지 않았다.
