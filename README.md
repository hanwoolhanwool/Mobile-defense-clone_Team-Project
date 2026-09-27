# Team-Project
2026.09-12

## 현재 작업 상태

현재 P0 참고 구현을 A/B 독립 브랜치와 통합 worktree에서 제작·검증하고 있습니다. G0~G2는 통과했으며 G3 PC 패키지 반복·네트워크·부하 검사와 G4 Android 검수가 남아 있습니다. 실제 학습자는 공통 출발점에서 시작하는 Planned 상태입니다. [이번 작업과 검증 근거](docs/production/P0_REFERENCE_RUN.md)를 확인하세요.

실행할 프로젝트는 `C:/Users/iam12/P0_reference_integration/Mobile_defense_clone.uproject`입니다. 원래 `Mobile_defense_clone` 폴더는 기획·기본 프로젝트 출발점을 보존합니다. 통합 프로젝트의 `L_P0Entry`에서 한 쪽은 호스트, 다른 쪽은 호스트 IPv4 주소로 참가합니다. 같은 PC의 두 실행에서는 `127.0.0.1:7777`을 사용합니다. 에디터의 단일 PIE 실행만으로 패키지 2인 검수를 대신하지 않습니다.

## 기획 관리

[기획 허브](docs/README.md)에서 최신 기능 명세, 작업 보드, 의사결정, 검수 결과를 확인합니다.

[관리 방식](docs/WORKFLOW.md) · [작업 보드](docs/production/BOARD.md) · [전체 기획 읽기본](docs/GDD_행운공방디펜스_UE5.md)

**완료 목표:** [개발 계획서](docs/production/DEVELOPMENT_PLAN.md). 아트 에셋 사용, 주 5일 기준의 단계별 마감·주별 작업·검수/수정 여유를 확인합니다.

**각자 개발 시작:** [A 개발 문서](docs/production/DEVELOPMENT_A.md) · [B 개발 문서](docs/production/DEVELOPMENT_B.md). 단계별 구현 순서·주별 작업·상대 전달물·검수 기준을 정리했습니다.

## 협업 가이드

- **처음 참여한다면: [초기 설정 → A안·B안 개발 시작 가이드](docs/GETTING_STARTED.md)**
- [2인 개발 역할 — A 전투·웨이브 / B 경제·보드](docs/production/TEAM_ROLES.md)
- 구현 설계: [A 전투·웨이브](docs/technical/IMPLEMENTATION_A.md) · [B 경제·보드](docs/technical/IMPLEMENTATION_B.md) · [공통 API·통합 순서](docs/technical/IMPLEMENTATION_SHARED.md)
- [전체 개발 과정과 현재 진행 상황](docs/production/PROJECT_STATUS.md)
- [개발 시작 전 사전 준비·세팅 한눈에 보기](docs/technical/SETUP_CHECKLIST.md)
- [개발 환경과 프로젝트 시작 안내](docs/technical/DEVELOPMENT_SETUP.md)
- [Git 커밋 규약](COMMIT_CONVENTION.md)
- [C++·Blueprint 코드 작성 규약](docs/technical/CODING_STANDARD.md)
- [구현 설계 규약·상태 소유권·소환 처리 예시](docs/technical/ARCHITECTURE.md#implementation-rules)
- [클래스·폴더·데이터 규칙](docs/technical/NAMING_AND_STRUCTURE.md)
- [포맷 설정·설치·CI 검사](docs/technical/CODE_STYLE.md)

<!-- optional-learning:start -->
## 선택 학습 자료

`learning/README.md`에는 P1·P2의 향후 제작 계획과 공통 양식만 남아 있습니다. 기존 P0 A/B 자료는 삭제했으며 새 구현과 검수에 맞춰 다시 작성할 예정입니다.

이 자료는 나중에 `learning/`과 이 안내 블록만 제거할 수 있도록 정식 개발 문서·빌드에서 분리되어 있습니다.
<!-- optional-learning:end -->
