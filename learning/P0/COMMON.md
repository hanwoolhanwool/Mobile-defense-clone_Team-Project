# 공통 계약과 제공 범위

공통 설명의 원본은 [이번 작업 기록](../../docs/production/P0_REFERENCE_RUN.md)과 [서버·클라이언트 계약](../../docs/technical/IMPLEMENTATION_SHARED.md)이다. 수업에는 해당 단계의 구현 순서와 차이만 추가한다.

## 제공 코드와 직접 작성

- 출발점에 제공: 기존 Unreal 모듈/템플릿/사용자 에셋, 현행 `data/` JSON·검증 도구, 규칙/설계/학습 양식.
- A/B가 각각 직접 작성: G0 값 타입, Schema2/Rules0.3.0 로더, 자기 관점의 최소 매치 기반. 첫 통합에서 두 구현을 비교하고 한 상태 원본으로 정리한다.
- 참고 통합에서 제공하는 도구: `tools/Sync-P0Data.ps1`(런타임 데이터 복사/대조), `Build-P0Editor.ps1`(컴파일 로그), `Test-P0Automation.ps1`(계산/계약 검사). 게임 코드는 이 학습 폴더에 의존하지 않는다.
- Stub: G0에서 아직 없는 경제·보드·전투는 성공을 반환하지 않는다. 실제 연결 단계에서 제거하고 재검사한다.

## 실행과 증거의 구분

`node tools/check-project.mjs`는 문서/데이터/서식 검사다. `pwsh -File tools/Build-P0Editor.ps1`은 Unreal 컴파일이다. `pwsh -File tools/Test-P0Automation.ps1`은 실제 UE 코드의 NullRHI 자동화이며 화면/입력 검수가 아니다. PIE, PC 패키지 두 프로세스, Android 실기기는 각각 별도 증거를 남긴다.

전체 로그는 각 worktree의 `Saved/P0Runs/<RunId>/`. 보관할 결과는 정식 `docs/production/evidence/`에 복사한다. 출발점·현재 커밋·미커밋 차이·환경·입력·기대값·관찰·실패와 수정·미검증을 기록한다.

## 확인 문제

1. 화면을 뒤집어도 CellId와 RouteIndex를 바꾸면 안 되는 이유는 무엇인가?
2. 로더 검증 통과와 게임 실행 통과는 어떤 증거가 다른가?
3. 명령 중복 캐시는 Phase 검사를 왜 앞서야 하는가?

변형 과제: 학습자 브랜치에서 로더 입력의 RulesVersion만 바꿔 준비가 차단되는지 확인하고, 원본 규칙 파일 변경을 제품에 통합하지 않는다.
