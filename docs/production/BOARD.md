# 작업 보드

[기획 허브](../README.md) · [작업 정의·완료 기준](../BACKLOG_QA.md) · [로드맵](ROADMAP.md) · [검수 기록](TEST_RUNS.md)

## 사용 기준

착수 순서와 남은 선행 조건은 [개발 전 문서 검토 종합](DEVELOPMENT_READINESS.md)을 참조한다. Windows 준비는 아래 Done 기록을 따르며, 다음 행동은 [현재 진행 상황](PROJECT_STATUS.md)의 A4를 따른다. TASK-MAP-01/TASK-BOARD-01의 보드 차원·조작과 TASK-ECON-01의 경제 규칙은 원작 대조 후 구현 기준을 갱신한다. 문서의 기존 24칸·자체 비용·환급값을 원작 확정값으로 사용하지 않는다.

DEC-021에 따라 게임 규칙·조작 작업은 [원작 대조 기록](../product/ORIGINAL_REFERENCE.md)을 확인한다. 원작 미확인 자체 규칙·재료 교환은 기존 Must/Should 표기만으로 구현 대상으로 확정하지 않는다. 실제 동작 확인 후 관련 명세·데이터·수용 기준을 함께 갱신한다. 이번 문서 검토로 작업 상태·담당자를 변경하지 않는다.

이 파일이 현재 작업 상태와 실제 담당자의 원본이다. 2026-09-11에 기존33개 작업을 초기 등록했다. 초기 Backlog는 기존 코드가 미구현이라는 판정이 아니라 진척을 확인하지 않은 등록 상태다. 실제 착수 작업의 담당자는 아래 표와 작업 메모를 따른다.

단계 P0/P1/P2는 마일스톤, 우선순위 Must/Should/Could는 해당 단계의 중요도다. 둘을 혼용하지 않는다. 다음 착수 후보는 TASK-MOB-01의 Android 도구·기본 패키징 준비이며, 원작 확인이 필요한 작업은 확인된 규칙부터 진행한다. 실제 착수 지시는 별도 개발 요청 범위를 따른다.

상태: Backlog → Ready → InProgress → Review → QA → Done. 외부 의존성으로 막히면 Blocked와 원인을 기록한다. 완료할 때 아래 작업 메모에 증거를 연결한다.

QA 열은 대표 회귀 검사이며 완료 조건 전체를 대체하지 않는다. 링크된 작업 정의의 수용 기준도 함께 충족해야 한다. P0 검사는 P0 프로파일을 적용한다.

DEC-019에 따라 P0는 8종·10웨이브 기본 공격과 짧은 조작 안내까지다. 공격 훈련·장치 가속은 TASK-ECON-02, 희귀 고유 스킬은 TASK-COMBAT-02, 정식 튜토리얼은 TASK-UX-01의 P1 범위로 유지한다. P0 전체 통과에는 TASK-TEST-01의 PC 2인 검증과 TASK-MOB-01의 Android 실기기 터치 완주가 모두 필요하다. 기기 미정은 P0 최종 통과의 대기 사유이며 이번 문서 반영으로 개별 작업 상태·담당자를 변경하지 않는다.

| 작업 | 단계 | 우선순위 | 상태 | 담당자 | 명세 | 대표 검수 |
|---|---|---|---|---|---|---|
| [TASK-CORE-01](../BACKLOG_QA.md#TASK-CORE-01) | P0 | Must | Done | Codex | [TECH-001](../technical/ARCHITECTURE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-CORE-01) |
| [TASK-MAP-01](../BACKLOG_QA.md#TASK-MAP-01) | P0 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-VIS-03](../BACKLOG_QA.md#QA-VIS-03), [QA-WAVE-04](../BACKLOG_QA.md#QA-WAVE-04) |
| [TASK-NET-01](../BACKLOG_QA.md#TASK-NET-01) | P0 | Must | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03) |
| [TASK-DATA-01](../BACKLOG_QA.md#TASK-DATA-01) | P0 | Must | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-RNG-01](../BACKLOG_QA.md#QA-RNG-01) |
| [TASK-COMBAT-01](../BACKLOG_QA.md#TASK-COMBAT-01) | P0 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-DMG-01](../BACKLOG_QA.md#QA-DMG-01), [QA-ECO-03](../BACKLOG_QA.md#QA-ECO-03), [QA-WAVE-04](../BACKLOG_QA.md#QA-WAVE-04) |
| [TASK-ECON-01](../BACKLOG_QA.md#TASK-ECON-01) | P0 | Must | Backlog | 미지정 | [SPEC-SUMMON](../design/SUMMON_ECONOMY.md) | [QA-ECO-01](../BACKLOG_QA.md#QA-ECO-01), [QA-BOARD-01](../BACKLOG_QA.md#QA-BOARD-01), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03) |
| [TASK-BOARD-01](../BACKLOG_QA.md#TASK-BOARD-01) | P0 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-BOARD-02](../BACKLOG_QA.md#QA-BOARD-02), [QA-BOARD-03](../BACKLOG_QA.md#QA-BOARD-03), [QA-BOARD-04](../BACKLOG_QA.md#QA-BOARD-04), [QA-BOARD-08](../BACKLOG_QA.md#QA-BOARD-08) |
| [TASK-WAVE-01](../BACKLOG_QA.md#TASK-WAVE-01) | P0 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-TIME-02](../BACKLOG_QA.md#QA-TIME-02), [QA-TIME-03](../BACKLOG_QA.md#QA-TIME-03), [QA-TIME-07](../BACKLOG_QA.md#QA-TIME-07) |
| [TASK-UI-01](../BACKLOG_QA.md#TASK-UI-01) | P0 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-MOB-01](../BACKLOG_QA.md#QA-MOB-01), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) |
| [TASK-TEST-01](../BACKLOG_QA.md#TASK-TEST-01) | P0 | Must | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-02](../BACKLOG_QA.md#QA-NET-02), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03) |
| [TASK-MOB-01](../BACKLOG_QA.md#TASK-MOB-01) | P0 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-MOB-01](../BACKLOG_QA.md#QA-MOB-01), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) |
| [TASK-DATA-02](../BACKLOG_QA.md#TASK-DATA-02) | P1 | Must | Backlog | 미지정 | [SPEC-UNITS](../design/UNITS.md) | [QA-RNG-01](../BACKLOG_QA.md#QA-RNG-01), [QA-BOARD-10](../BACKLOG_QA.md#QA-BOARD-10) |
| [TASK-COMBAT-02](../BACKLOG_QA.md#TASK-COMBAT-02) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-DMG-03](../BACKLOG_QA.md#QA-DMG-03), [QA-DMG-05](../BACKLOG_QA.md#QA-DMG-05), [QA-DMG-07](../BACKLOG_QA.md#QA-DMG-07), [QA-DMG-10](../BACKLOG_QA.md#QA-DMG-10), [QA-DMG-11](../BACKLOG_QA.md#QA-DMG-11), [QA-DMG-12](../BACKLOG_QA.md#QA-DMG-12), [QA-DMG-13](../BACKLOG_QA.md#QA-DMG-13), [QA-DMG-14](../BACKLOG_QA.md#QA-DMG-14) |
| [TASK-ECON-02](../BACKLOG_QA.md#TASK-ECON-02) | P1 | Must | Backlog | 미지정 | [SPEC-SUMMON](../design/SUMMON_ECONOMY.md) | [QA-RNG-04](../BACKLOG_QA.md#QA-RNG-04), [QA-ECO-06](../BACKLOG_QA.md#QA-ECO-06), [QA-ECO-07](../BACKLOG_QA.md#QA-ECO-07), [QA-BOARD-10](../BACKLOG_QA.md#QA-BOARD-10) |
| [TASK-WAVE-02](../BACKLOG_QA.md#TASK-WAVE-02) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-WAVE-01](../BACKLOG_QA.md#QA-WAVE-01), [QA-WAVE-02](../BACKLOG_QA.md#QA-WAVE-02), [QA-WAVE-03](../BACKLOG_QA.md#QA-WAVE-03), [QA-TIME-06](../BACKLOG_QA.md#QA-TIME-06) |
| [TASK-BOT-01](../BACKLOG_QA.md#TASK-BOT-01) | P1 | Must | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-BOT-01) |
| [TASK-UI-02](../BACKLOG_QA.md#TASK-UI-02) | P1 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-BOARD-09](../BACKLOG_QA.md#QA-BOARD-09), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) |
| [TASK-UX-01](../BACKLOG_QA.md#TASK-UX-01) | P1 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-MOB-03](../BACKLOG_QA.md#QA-MOB-03), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) |
| [TASK-SAVE-01](../BACKLOG_QA.md#TASK-SAVE-01) | P1 | Must | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-01](../BACKLOG_QA.md#QA-SAVE-01), [QA-SAVE-03](../BACKLOG_QA.md#QA-SAVE-03) |
| [TASK-ART-01](../BACKLOG_QA.md#TASK-ART-01) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-04](../BACKLOG_QA.md#QA-VIS-04) |
| [TASK-ART-02](../BACKLOG_QA.md#TASK-ART-02) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-04](../BACKLOG_QA.md#QA-VIS-04) |
| [TASK-ART-03](../BACKLOG_QA.md#TASK-ART-03) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02) |
| [TASK-FX-01](../BACKLOG_QA.md#TASK-FX-01) | P1 | Should | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-PERF-03](../BACKLOG_QA.md#QA-PERF-03), [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01) |
| [TASK-AUDIO-01](../BACKLOG_QA.md#TASK-AUDIO-01) | P1 | Should | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-MOB-03](../BACKLOG_QA.md#QA-MOB-03) |
| [TASK-PERF-01](../BACKLOG_QA.md#TASK-PERF-01) | P1 | Must | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-PERF-01](../BACKLOG_QA.md#QA-PERF-01), [QA-PERF-02](../BACKLOG_QA.md#QA-PERF-02), [QA-PERF-03](../BACKLOG_QA.md#QA-PERF-03) |
| [TASK-BAL-01](../BACKLOG_QA.md#TASK-BAL-01) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-BAL-01) |
| [TASK-QA-01](../BACKLOG_QA.md#TASK-QA-01) | P1 | Must | Backlog | 미지정 | [PLAN-001](ROADMAP.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-QA-01) |
| [TASK-SERVER-01](../BACKLOG_QA.md#TASK-SERVER-01) | P2 | Should | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-02](../BACKLOG_QA.md#QA-NET-02) |
| [TASK-ACCOUNT-01](../BACKLOG_QA.md#TASK-ACCOUNT-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-02](../BACKLOG_QA.md#QA-SAVE-02), [QA-NET-08](../BACKLOG_QA.md#QA-NET-08) |
| [TASK-ROOM-01](../BACKLOG_QA.md#TASK-ROOM-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-04](../BACKLOG_QA.md#QA-SAVE-04) |
| [TASK-REJOIN-01](../BACKLOG_QA.md#TASK-REJOIN-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-REJOIN-01](../BACKLOG_QA.md#QA-REJOIN-01), [QA-REJOIN-02](../BACKLOG_QA.md#QA-REJOIN-02), [QA-REJOIN-03](../BACKLOG_QA.md#QA-REJOIN-03), [QA-REJOIN-04](../BACKLOG_QA.md#QA-REJOIN-04) |
| [TASK-OPS-01](../BACKLOG_QA.md#TASK-OPS-01) | P2 | Should | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-OPS-01) |
| [TASK-RELEASE-01](../BACKLOG_QA.md#TASK-RELEASE-01) | P2 | Should | Backlog | 미지정 | [PLAN-001](ROADMAP.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-RELEASE-01) |

## 개발 도구 확장

2026-09-12에 아래 4개를 추가해 등록 작업은 총 37개다. P1 Should 확장 계획이며 기존 33개의 상태·담당자는 유지한다. 상세 기능·선행·완료 조건은 [도구 작업 연결](../technical/AUTOMATION.md#tool-task-map)을 따른다. 구현 일정·공수는 미산정이며 P0의 필수 도구 구축으로 해석하지 않는다.

| 작업 | 단계 | 우선순위 | 상태 | 담당자 | 명세 | 대표 검수 |
|---|---|---|---|---|---|---|
| [TASK-TOOLS-01](../BACKLOG_QA.md#TASK-TOOLS-01) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md), [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-01](../BACKLOG_QA.md#QA-TOOLS-01), [QA-TOOLS-02](../BACKLOG_QA.md#QA-TOOLS-02) |
| [TASK-TOOLS-02](../BACKLOG_QA.md#TASK-TOOLS-02) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md) | [QA-TOOLS-03](../BACKLOG_QA.md#QA-TOOLS-03) |
| [TASK-TOOLS-03](../BACKLOG_QA.md#TASK-TOOLS-03) | P1 | Should | Backlog | 미지정 | [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-04](../BACKLOG_QA.md#QA-TOOLS-04), [QA-TOOLS-05](../BACKLOG_QA.md#QA-TOOLS-05), [QA-TOOLS-06](../BACKLOG_QA.md#QA-TOOLS-06) |
| [TASK-TOOLS-04](../BACKLOG_QA.md#TASK-TOOLS-04) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md), [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-07](../BACKLOG_QA.md#QA-TOOLS-07), [QA-TOOLS-08](../BACKLOG_QA.md#QA-TOOLS-08) |

## 작업 메모·증거

DEC-020 추가 수용 기준은 [명령 검수](../BACKLOG_QA.md#QA-CMD-01), [요청 내용 충돌](../BACKLOG_QA.md#QA-NET-09), [캐시 퇴출 후 재전송](../BACKLOG_QA.md#QA-NET-10)을 참조한다. TASK-NET-01/TASK-ECON-01/TASK-BOARD-01/TASK-UI-01/TASK-TEST-01의 역할별 적용 범위는 백로그에 기록했다. 이번 명세 반영으로 상태·담당자·실행 결과를 변경하지 않는다.

### TASK-CORE-01

- 담당자: Codex. 2026-09-13 사용자 `계속 알아서 진행해줘` 지시로 착수.
- 완료한 부분: 기존 입력 606개·142,091,676바이트의 스냅샷 복사 및 SHA-256 일치 확인. 최초 수집 목록의 경로 구분자 중복을 제거한 실제 파일 수다. UE 5.8.2, VS·SDK·JDK 설치 현황 재확인.
- 입력 보존: `Saved/BuildRuns/RUN-20260913-02/Input/`, `input-manifest.json`, Git 상태·HEAD 기록. 미커밋·미추적 입력과 Content의 생성 조명 데이터도 포함.
- 환경 적용: VS Community 2026 18.10 Stable 설치 종료 코드 0, 재부팅 요구 없음. MSVC 14.50.35738·SDK 10.0.26100.0의 UBT 선택 확인. 프로젝트에 명시적 도구 버전 적용.
- 완료 범위: 별도 작업 폴더의 프로젝트 생성·Editor 빌드·Win64 Development 컴파일/쿠킹/패키징, 기본 맵의 PIE 시작·클릭 이동·중지 및 패키징 게임의 표시·클릭 이동·정상 종료 검증.
- 완료 판정 연결: [완료 조건별 실행·증거](verification.json). 2026-09-14 기존 결과를 대조해 등록했으며 신규 UE 실행이 아니다.
- 증거: RUN-20260913-02의 빌드 로그, RUN-20260913-03의 산출물 재검사·실행 관찰. 초기 검사 스크립트의 archive 경로 오판은 수정하고 실패 기록 보존.
- 남은 범위: 한국어 Editor 시작 시 엔진 스모크 오류 15건(동일 조건 영어 0건)은 엔진 언어 의존 진단으로 기록. 엔진 내부 테스트 전체 통과·Android 빌드·엔진 최종 고정·디펜스 기능/P0 통과를 이 Done으로 판정하지 않음. 원본 소스·콘텐츠 533개 해시 불변.

진행 중이거나 완료한 작업의 후속 메모는 아래 형식을 사용한다.

```markdown
### TASK-ID
담당자:
진행 상황 / 남은 일:
차단 원인·해결 담당·재확인 조건(해당 시):
관련 변경/이슈/PR:
검수 Run ID와 증거:
```

## 외부 작업 도구로 이관할 때

GitHub Projects·Jira 등을 도입하면 TASK ID를 유지한 채 각 작업을 이슈에 연결한다. 이 파일을 상태 원본으로 계속 사용할지, 외부 도구로 완전히 이관할지 한 번 결정한다. 두 곳의 상태를 손으로 따로 갱신하지 않는다. 현재 원격 이슈나 프로젝트 보드는 생성하지 않았다.
