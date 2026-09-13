# 기획 허브

**이 페이지를 시작점으로 사용합니다.** 제품 방향, 기능별 규칙, 개발 작업, 결정 기록을 여기서 찾을 수 있습니다.

모바일 우선 · UE5 · 3D 협동 디펜스 · 문서 체계 0.2 · 정리일 2026-09-13

## 지금 볼 곳

| 하려는 일 | 문서 |
|---|---|
| 전체 개발 과정과 현재 위치를 한눈에 확인 | [전체 개발 과정과 현재 진행 상황](production/PROJECT_STATUS.md) |
| 개발 전 도구·프로젝트·Android 세팅 확인 | [사전 준비·세팅 한눈에 보기](technical/SETUP_CHECKLIST.md) |
| 개발 착수 가능 범위와 남은 선행 조건 확인 | [개발 전 문서 검토 종합](production/DEVELOPMENT_READINESS.md) |
| 어떤 게임인지, MVP에 무엇이 포함되는지 확인 | [제품 방향과 범위](product/OVERVIEW.md) |
| 원작 재현 근거와 미확인 동작 확인 | [원작 대조 기록](product/ORIGINAL_REFERENCE.md) |
| 현재 프로젝트에서 개발 시작 | [개발 환경과 프로젝트 시작 안내](technical/DEVELOPMENT_SETUP.md) |
| 빌드·실행 명령과 로그 확인 | [PowerShell 빌드·실행 절차](technical/BUILD_RUN.md) |
| 언리얼 내부 콘솔·MCP 서버·성능 명령 확인 | [언리얼 내부 콘솔 주요 명령어](technical/UNREAL_CONSOLE_COMMANDS.md) |
| 지금 할 작업과 상태 확인 | [작업 보드](production/BOARD.md) |
| 게임 규칙 변경 | 아래 기능별 명세에서 해당 파일 편집 |
| 개발용 에디터·자동화 설계 확인 | [에디터 기능 설계](technical/EDITOR_TOOLS.md) · [자동화 기능 설계](technical/AUTOMATION.md) |
| 왜 그렇게 정했는지 확인 | [결정 기록](DECISIONS.md) |
| 어떻게 변경·검수·완료 처리하는지 확인 | [관리 방식](WORKFLOW.md) |
| 변경 내역 확인 | [변경 이력](CHANGELOG.md) |
| 전체 기획을 한 번에 읽기 | [전체 GDD 읽기본](GDD_행운공방디펜스_UE5.md) — 자동 생성 |

## 현재 기준

2026-09-14 문서 운영 보완을 적용했습니다. [관리 방식](WORKFLOW.md)의 통합 검사·CI 설정·문서 검토일·증거 보관 기준을 사용합니다. 작업 브랜치를 원격에 push하고 [초안 PR #1](https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project/pull/1)에서 [첫 GitHub CI 성공](https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project/actions/runs/34788260254)을 확인했습니다. 이어서 main에 PR·필수 CI·최신 기준 반영·관리자 적용·강제 push/삭제 금지 규칙을 적용했습니다. 코드 작성 규약의 현재 보유 현황은 관리 방식에서 확인합니다.

2026-09-13 TASK-CORE-01의 Windows 기반 검증을 완료했습니다. VS 2026·MSVC·SDK를 적용하고 별도 복사본의 빌드·패키징·기본 맵 입력/종료를 확인했습니다. 한국어 Editor의 엔진 시작 검사 문제와 Android 미검증은 [실행 기록](production/TEST_RUNS.md)에 남겼으며 엔진 최종 고정·P0 완료는 아직입니다.

6개 문서 검토 항목의 [종합 결과](production/DEVELOPMENT_READINESS.md)를 정리했습니다. Windows 준비 이후 Android 도구·패키징 준비로 이어갈 수 있으며, 원작 규칙 확정·기기 선정·실행 검증의 남은 조건은 영향을 받는 작업별로 연결했습니다. 문서 정리 완료가 게임 전체의 구현 준비·빌드 성공을 뜻하지 않습니다.

DEC-022로 원작 대조 대상은 공식 스토어에서 확인한 Android 2.0.11 · 보통 모드로 정했습니다. 해당 버전의 실제 동작 검증과 남은 입력 조건은 OPEN-009에서 관리합니다. 영상으로 확인한 합성·판매·배치 사례는 원작 대조 기록의 OBS-06~09를 참조합니다.

DEC-021에 따라 게임 규칙·조작은 원작 재현을 우선합니다. 기존 자체 수치·편의 기능은 원작 대조 전까지 구현 기준으로 확정하지 않습니다. 개발 환경과 P0 중간 검증 범위에 대한 기존 사용자 결정은 유지합니다.

사용자가 확정한 조건은 UE5, 3D 에셋, 원작을 참고한 디펜스, 모바일 우선입니다. 세로 화면, Android→iOS 검증 순서, 2인·30웨이브·수치·일정은 작업용 설계 기준입니다. 확정 근거와 가정은 [DEC-LOG](DECISIONS.md)에서 관리합니다.

현재 `Mobile_defense_clone` 프로젝트·모듈을 유지하고 디펜스 전용 구조를 추가하는 방향을 사용자와 확정했습니다(DEC-008). 소스는 기존 모듈 안에, 전용 콘텐츠는 `Content/LD/` 아래에 구성합니다. 설치 환경과 후속 결정은 개발 시작 안내에서 관리합니다. 기존 게임 기능을 전수 검증하지 않았으며, 기획 문서가 있다고 구현이 완료된 것으로 표시하지 않습니다.

## 기능별 편집 원본

| ID | 문서 | 다루는 내용 | 기존 GDD 장 |
|---|---|---|---|
| PRD-001 | [제품 방향과 범위](product/OVERVIEW.md) | 목표, 대상 경험, P0/P1/P2, 제외 범위 | 1~3 |
| SPEC-BOARD | [전장·배치·모바일 UI](design/BOARD_UI.md) | 좌표, 카메라, 스택, 터치, 화면, 튜토리얼 | 4·12 |
| SPEC-BATTLE | [전투·웨이브·승패](design/BATTLE.md) | 시간, 피해, 상태이상, 적, 보스 | 5·8·10 |
| SPEC-SUMMON | [소환·합성·재화](design/SUMMON_ECONOMY.md) | 골드, 강화, 판매, 확률, 보장, 제작 | 6~7 |
| SPEC-UNITS | [수호자·스킬](design/UNITS.md) | 역할, 콘텐츠 목록, 스킬 세부 | 9 |
| SPEC-COOP | [협동·접속·성장·저장](design/COOP_META.md) | 봇, 재접속, 계정 성장, 보상 | 11·13 |
| ART-001 | [3D 아트·연출·사운드](art/ART_DIRECTION.md) | 모델·리그·LOD, VFX, 납품 기준 | 14 |
| TECH-001 | [UE5 구현·데이터·성능](technical/ARCHITECTURE.md) | 클래스, 서버, 복제, 데이터, 성능 | 15~18 |
| PLAN-001 | [로드맵](production/ROADMAP.md) | 마일스톤, 일정 가정, 완료 기준 | 19 |
| DEC-LOG | [결정 기록](DECISIONS.md) | 결정 근거, 미확정 항목, 가정 | 20 |

## 개발 도구 확장 설계

아래 문서는 기존 게임 기획을 기준으로 작성한 별도 확장 설계 초안입니다. 구현 완료를 뜻하지 않으며, 기존 20장 전체 GDD 읽기본에는 포함하지 않습니다.

저장·임포트 복구, 실행 입력·결과·취소 계약과 [개발 작업 연결](technical/AUTOMATION.md#tool-task-map)을 구체화했습니다. 추가한 P1 도구 작업 4개는 미착수이며 P0 완료 조건을 확대하지 않습니다. 게임 규칙에 관련된 도구 사례도 원작 대조 기준을 따릅니다.

| ID | 문서 | 다루는 내용 |
|---|---|---|
| SPEC-EDITOR | [게임 개발 에디터](technical/EDITOR_TOOLS.md) | 전투 테스트 콘솔, 웨이브·유닛·경제·보드·외형 편집, 저장·적용, 단계별 완료 조건 |
| SPEC-AUTOMATION | [게임 제작·검증 자동화](technical/AUTOMATION.md) | 데이터 검증·임포트, 회귀·확률·봇·협동·성능 검사, 공유 실행 계약과 결과 기록 |

## 작업·데이터·검수

| 항목 | 원본과 역할 |
|---|---|
| 작업 상태·실제 담당자 | [작업 보드](production/BOARD.md). 현재 유일한 진행 상태 원본 |
| 작업 정의·선행 조건·완료 기준 | [백로그와 QA 명세](BACKLOG_QA.md). 작업은 `TASK-` 접두사로 인용 |
| QA 기대 결과 | [백로그와 QA 명세](BACKLOG_QA.md). 테스트는 `QA-` 접두사로 구분 |
| 테스트 실행 결과·증거 | [검수 기록](production/TEST_RUNS.md). 시나리오 목록과 실제 통과를 구분 |
| 데이터 필드·타입 | [데이터 명세](DATA_SCHEMA.md) |
| 초기 밸런스 편집 원본 | [build-design-data.mjs](../tools/build-design-data.mjs). 현재 수치와 생성 공식의 원본 |
| UE 임포트용 결과 | [data 폴더 안내](../data/README.md). JSON은 위 스크립트에서 생성 |
| 데이터 검사 결과 | [검증 보고서](VALIDATION_REPORT.md). 검사 스크립트가 생성 |
| 완료 판정·증거 보관 | [조건별 완료 판정](production/verification.json) · [증거 보관 규칙](production/EVIDENCE.md) |
| 새 명세·변경 제안·검수 양식 | [실무 템플릿](TEMPLATES.md) |

## 원본을 하나로 유지하는 규칙

- 게임 규칙 문장은 해당 기능별 명세에서 편집합니다. 전체 GDD는 읽기용으로 재생성합니다.
- 확률·스탯 등 초기 데이터는 생성 스크립트에서 편집하고 JSON을 생성합니다. JSON만 직접 고치지 않습니다.
- 문서에 반복해서 나온 수치는 설명용 요약입니다. 수치를 바꿀 때 관련 명세의 예시·표와 검수 기준도 같은 변경에 갱신합니다.
- 작업 상태는 작업 보드에서 먼저 바꿉니다. 전체 진행 상황·세팅 요약은 기준일을 표시한 조회용 요약이며 주요 변경 때 원본에 맞춰 갱신합니다. 요약에서만 작업을 완료 처리하지 않습니다.
- 현재 파일명을 유지하고 변경 내역은 Git과 변경 이력으로 남깁니다. `최종`, `최종2` 파일을 만들지 않습니다.

기획자용 스프레드시트를 도입하면 밸런스 편집 원본을 그 파일로 전환하는 변경을 한 번 기록합니다. 이때 기존 스크립트와 스프레드시트를 동시에 수동 편집하지 않습니다.

## 현재 작업 흐름

```text
작업 보드에서 작업 선택
 → 연결된 명세·데이터·완료 기준 확인
 → 명세/구현 변경
 → 필요한 결정과 변경 이력 기록
 → 검수 실행 및 증거 연결
 → 작업 상태 갱신
```

관리 방식은 제품 요구사항과 완료 기준을 함께 두는 [Atlassian의 PRD 안내](https://www.atlassian.com/agile/product-management/requirements), 버전 관리 문서와 결정 상태를 사용하는 [GitLab의 설계 문서 운영 사례](https://handbook.gitlab.com/handbook/engineering/architecture/workflow/)를 참고해 이 프로젝트 규모에 맞게 구성했습니다.
