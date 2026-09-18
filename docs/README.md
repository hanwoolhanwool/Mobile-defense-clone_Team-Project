# 기획 허브

**현재 상태:** [기존 P0 구현·A/B 학습 초기화 완료](production/P0_RESET.md). 기본 프로젝트와 최신 기획에서 새로 시작하며 P0 기능·수업은 미착수다.

**단계 반영 확인:** [P0·P1·P2 로드맵](production/ROADMAP.md#phase-plan) · [초기화 전 단계 반영 점검 이력](production/PHASE_ALIGNMENT.md).

**현재 규칙:** DEC-043~047, P0 일반~전설16종·기본100/20·보스2/60초, P1 보통/어려움80웨이브·사냥/미션/던전·보물/유물/펫 실제 시스템. [확정표와 남은 일](design/P0_REPLAN.md), 데이터 Schema2/Rules0.3.0. UI 원본은 보존하고 수치/조건은 최신 규칙에 연결한다.

**지금의 시작점:** [촬영본 분석 18종](product/CAPTURE_ANALYSIS_20260916.md) → [P0 재기획·A/B 동시 개발·검수](design/P0_REPLAN.md). 2.0.12 촬영본을 기준으로 구도를 다시 맞추며, 이전 프로토타입의 4×3·경제 수치는 새 확정값이 아니다.

**현재 UI 디자인:** [v2 도안·요소별 좌표/비율](design/BOARD_UI.md#ingame-ui-v2) · [Figma 수정본](https://www.figma.com/design/w50Z6dSdg76QxF3ilaRpqB?node-id=15-2). 1080×2340 기준, 개인 6열×3행·칸 120×120px·모든 이동 통로 폭 120px. 원본 v1은 보존하고 v2를 현재 기준으로 적용한다(DEC-040).

**상세 패널 디자인:** [신화·강화·룰렛·설정 명세/개별 이미지](design/BOARD_UI.md#ingame-ui-panels) · [Figma 4개 화면](https://www.figma.com/design/w50Z6dSdg76QxF3ilaRpqB?node-id=22-2). DEC-041로 화면 배치·구성을 채택했으며 개발/QA 기준에 연결했다. 비용·확률·레벨·설정 기본값은 확인된 데이터로 연결한다.

**미션 디자인:** [현재 미션 UI 명세/이미지](design/BOARD_UI.md#mission-ui) · [Figma](https://www.figma.com/design/w50Z6dSdg76QxF3ilaRpqB?node-id=28-2). DEC-042로 조건·수집/수량 진척·보상·완료 표시·스크롤/닫기 구성을 문서 기준으로 반영했다. 집계와 보상 지급 규칙은 별도로 확인한다.

**이 페이지를 시작점으로 사용합니다.** 제품 방향, 기능별 규칙, 개발 작업, 결정 기록을 여기서 찾을 수 있습니다.

모바일 우선 · UE5 · 3D 협동 디펜스 · 문서 체계 0.2 · 정리일 2026-09-15

**완료 목표:** [개발 계획서](production/DEVELOPMENT_PLAN.md). 9/21 시작 · P0 10/16 · P1 핵심11/13/전체11/20 · 외부2인 최소 전투11/6 · P2 기능12/4 · 최종 검수12/11 · 인수12/22. 하루8시간은 계획 가정이며 P2 선행과 P1 마무리 각2일 교환으로 총 공수를 유지한다.

## 지금 볼 곳

| 하려는 일 | 문서 |
|---|---|
| 전체 범위·일정·공수 확인 | [개발 계획서](production/DEVELOPMENT_PLAN.md) |
| A의 P0~P2 작업·주별 산출물·검수 확인 | [A 개발 문서 — 전투·웨이브·서버](production/DEVELOPMENT_A.md) |
| B의 P0~P2 작업·주별 산출물·검수 확인 | [B 개발 문서 — 경제·보드·계정/로비](production/DEVELOPMENT_B.md) |
| 처음 참여해서 초기 설정 후 A안·B안으로 개발 시작 | [처음 시작하기 — 공통 설정·역할별 첫 작업·통합](GETTING_STARTED.md) |
| 전체 개발 과정과 현재 위치를 한눈에 확인 | [전체 개발 과정과 현재 진행 상황](production/PROJECT_STATUS.md) |
| A/B 담당 범위·Blueprint 사용·학습과 통합 책임 확인 | [2인 개발 역할과 협업](production/TEAM_ROLES.md) |
| A 담당 코드·Unreal 설정·단계별 검수 설계 | [A 구현 설계 — 전투·웨이브](technical/IMPLEMENTATION_A.md) |
| B 담당 코드·Unreal 설정·단계별 검수 설계 | [B 구현 설계 — 경제·보드](technical/IMPLEMENTATION_B.md) |
| 함께 사용할 API·초기화·통합·학습 산출물 확인 | [A/B 공통 구현 계약](technical/IMPLEMENTATION_SHARED.md) |
| 개발 전 도구·프로젝트·Android 세팅 확인 | [사전 준비·세팅 한눈에 보기](technical/SETUP_CHECKLIST.md) |
| 개발 착수 가능 범위와 남은 선행 조건 확인 | [개발 전 문서 검토 종합](production/DEVELOPMENT_READINESS.md) |
| 어떤 게임인지, MVP에 무엇이 포함되는지 확인 | [제품 방향과 범위](product/OVERVIEW.md) |
| 현재 인게임 UI 도안·치수·정사각형/통로 비율 확인 | [전장·모바일 UI v2 명세](design/BOARD_UI.md#ingame-ui-v2) |
| 원작 재현 근거와 미확인 동작 확인 | [원작 대조 기록](product/ORIGINAL_REFERENCE.md) |
| 현재 프로젝트에서 개발 시작 | [개발 환경과 프로젝트 시작 안내](technical/DEVELOPMENT_SETUP.md) |
| C++·Blueprint 작성·리뷰 | [코드 작성 규약](technical/CODING_STANDARD.md) · [클래스·폴더·데이터 규칙](technical/NAMING_AND_STRUCTURE.md) |
| 기능의 책임·호출 관계·상태 소유권 설계 | [구현 설계 규약·소환 예시·검수 기준](technical/ARCHITECTURE.md#implementation-rules) · [설계 메모 양식](TEMPLATES.md#implementation-note) |
| 코드 포맷 설치·자동 검사 | [코드 스타일 설정과 검사](technical/CODE_STYLE.md) |
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

DEC-030/031의 전장은 두 참가자 화면 모두 **좌측 상단·좌측 하단에서 생성**하고 중앙에서 **왼쪽→오른쪽으로 합류**한다. 자기 보드는 아래쪽에 유지한다. [전장·카메라 명세](design/BOARD_UI.md)와 [2인 PIE 검수 절차](technical/EDITOR_TOOLS.md#pie-spawn-check)를 따른다.

DEC-027에 따라 학습 목적의 2인 개발은 **A 전투·웨이브 / B 경제·보드**로 진행합니다. 두 사람 모두 C++·Blueprint·UI·네트워크·검수를 맡으며 Android 빌드는 A, PC 2인 검수는 B가 주관합니다. [역할·협업 기준](production/TEAM_ROLES.md)과 [작업별 계획 역할](production/BOARD.md)을 반영했습니다. 주 5일 병행은 DEC-038로 확정했으며 A/B에 대응하는 실제 팀원·하루 가용 시간은 미정입니다. 개발 작업 상태는 유지합니다.

DEC-026으로 구현 설계 규약을 보완했습니다. 클래스 책임, 허용 의존 관계, 상태 원본과 변경 경로, 공동 확정·종료 규칙을 [아키텍처 15.5~15.7](technical/ARCHITECTURE.md#implementation-rules)에 두고 코드 규약·PR 검수에 연결했습니다. 소환 흐름과 테스트 사례는 첫 기능 구현 기준이며 실제 구현·검증 완료를 뜻하지 않습니다.

2026-09-14 문서 운영 보완과 main 보호 규칙을 적용했습니다. 2026-09-16 브랜치명 변경 후 현재 [초안 PR #2](https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project/pull/2)에서 GitHub CI를 사용합니다. DEC-025로 C++·Blueprint 작성 규약, 클래스·폴더·데이터 규칙, .editorconfig·.clang-format과 새 코드·수정 코드 포맷 검사를 추가했습니다. 사용 명령과 기존 템플릿 이행 범위는 [코드 스타일 안내](technical/CODE_STYLE.md), 실제 검증은 [실행 기록](production/TEST_RUNS.md)을 확인합니다.

2026-09-13 TASK-CORE-01의 Windows 기반 검증을 완료했습니다. VS 2026·MSVC·SDK를 적용하고 별도 복사본의 빌드·패키징·기본 맵 입력/종료를 확인했습니다. 한국어 Editor의 엔진 시작 검사 문제와 Android 미검증은 [실행 기록](production/TEST_RUNS.md)에 남겼으며 엔진 최종 고정·P0 완료는 아직입니다.

6개 문서 검토 항목의 [종합 결과](production/DEVELOPMENT_READINESS.md)를 정리했습니다. Windows 준비 이후 Android 도구·패키징 준비로 이어갈 수 있으며, 원작 규칙 확정·기기 선정·실행 검증의 남은 조건은 영향을 받는 작업별로 연결했습니다. 문서 정리 완료가 게임 전체의 구현 준비·빌드 성공을 뜻하지 않습니다.

DEC-032로 이번 화면 대조에는 사용자 촬영본의 2.0.12 · 보통 모드를 적용합니다. [18종 분석](product/CAPTURE_ANALYSIS_20260916.md)에서 확인된 화면과 미확인 규칙을 구분했습니다. DEC-022의 2.0.11과 과거 영상 OBS-06~09는 이전 조사 이력으로 보존하며, 현재 버전에서의 실제 입력·결과 검증은 OPEN-009로 관리합니다.

DEC-021에 따라 게임 규칙·조작은 원작 재현을 우선합니다. 기존 자체 수치·편의 기능은 원작 대조 전까지 구현 기준으로 확정하지 않습니다. 개발 환경과 P0 중간 검증 범위에 대한 기존 사용자 결정은 유지합니다.

사용자가 확정한 조건은 UE5, 3D 에셋, 원작을 참고한 디펜스, 모바일 우선입니다. 세로 화면·2인 협동·P0 10/P1 80웨이브와 확정 규칙을 따릅니다. Android→iOS 검증 순서·실전 수치·공수는 작업용 기준이며 실제 검수가 필요합니다. 확정 근거와 가정은 [DEC-LOG](DECISIONS.md)에서 관리합니다.

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
| 역할별 책임·학습·통합 기준 | [2인 개발 역할](production/TEAM_ROLES.md). 작업별 계획 역할은 보드, A/B에 대응하는 실명·가용 시간은 OPEN-001에서 확인 |
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
