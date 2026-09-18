# 작업 보드

[기획 허브](../README.md) · [2인 개발 역할](TEAM_ROLES.md) · [작업 정의·완료 기준](../BACKLOG_QA.md) · [로드맵](ROADMAP.md) · [검수 기록](TEST_RUNS.md)

**현재 상태:** 2026-09-18 공통 출발점을 보존하고 A/B 독립 G0 및 참고 통합을 구현했다. DATA/NET/TEST/MOB/MAP/UI 6개는 참고 제작 InProgress이며 실제 학습자는 Planned다. G0 코드 게이트 통과 후 G1 경로·보드·입력 통합 검수 중이다. TASK-CORE-01의 기존 환경 기록은 이번 게임 검수 증거로 재사용하지 않는다. [이번 작업](P0_REFERENCE_RUN.md).

**DEC-037 반영:** P0 TASK-ECON-01/BOARD-01/UI-01에 최대 3 뭉치·소환 자동 추가·합성/판매/사거리 UI를 포함한다. [QA-BOARD-11](../BACKLOG_QA.md#QA-BOARD-11)·[QA-BOARD-12](../BACKLOG_QA.md#QA-BOARD-12)·[QA-BOARD-13](../BACKLOG_QA.md#QA-BOARD-13)을 새 구현에서 검수한다. 작업 수·상태는 유지한다.

**12월 22일 목표:** [개발 계획서](DEVELOPMENT_PLAN.md). 아래 P1/P2 계획 역할을 편성했으며 실제 담당자·작업 상태는 유지한다. 아트는 에셋 연동, 미니게임은 종류 확정 전 가배정이다.

**DEC-040 UI 기준:** TASK-MAP-01/TASK-UI-01은 [v2 도안·치수](../design/BOARD_UI.md#ingame-ui-v2)의 1080×2340·정사각형 칸·균일 통로 폭을 적용한다. QA-VIS-02/03과 PLAN-VIS-01에서 기준 화면 및 다른 화면비의 비율·입력을 확인한다. 문서 채택이며 작업 상태·게임 구현 완료를 변경하지 않는다.

**DEC-041 패널 기준:** TASK-UI-02의 신화·강화·룰렛과 TASK-UX-01/SAVE-01의 설정은 [채택한 4개 화면](../design/BOARD_UI.md#ingame-ui-panels)을 따른다. QA-VIS-02·QA-MOB-03/04의 화면·상태·입력·설정 유지 검수에 연결하며 기존 P1/P2 단계와 작업 상태는 유지한다.

**DEC-042 미션 기준:** TASK-UI-02의 미션 화면은 [현재 도안](../design/BOARD_UI.md#mission-ui)을 사용한다. QA-VIS-02·QA-MOB-04로 배치·진척/완료 표시·스크롤·닫기·입력 소비를 검사하며 자동지급·현재/누적·판당1회는 DEC-046으로 TASK-ECON-02에 연결한다. 작업 수·단계·진행 상태는 유지한다.

## 사용 기준

**단계 반영 점검:** [최신 로드맵](ROADMAP.md#phase-plan)과 [점검 결과/구현 차이](PHASE_ALIGNMENT.md)를 적용한다. P0는 G0~G4 순서와 DEC-033/034를 필수 검수한다. P1/P2 후보는 상위 작업 연결이며 상세 범위·수용 기준 준비 전 Ready/Done으로 승격하지 않는다. 기존 37개 작업 상태·실제 담당자는 유지한다.

착수 순서와 남은 선행 조건은 [개발 전 문서 검토 종합](DEVELOPMENT_READINESS.md)을 참조한다. Windows 준비는 아래 Done 기록을 따르며, 다음 행동은 [현재 진행 상황](PROJECT_STATUS.md)의 A4를 따른다. 보드 차원은 DEC-039의 6열×3행(개인 18칸·전체 36칸)을 적용한다. TASK-MAP-01/TASK-BOARD-01의 물리 치수·조작과 TASK-ECON-01의 경제 규칙은 원작 대조 후 구현 기준을 갱신한다. 문서의 기존 24칸·자체 비용·환급값을 원작 확정값으로 사용하지 않는다.

DEC-021에 따라 게임 규칙·조작 작업은 [원작 대조 기록](../product/ORIGINAL_REFERENCE.md)을 확인한다. 원작 미확인 자체 규칙·재료 교환은 기존 Must/Should 표기만으로 구현 대상으로 확정하지 않는다. 실제 동작 확인 후 관련 명세·데이터·수용 기준을 함께 갱신한다. 이번 문서 검토로 작업 상태·담당자를 변경하지 않는다.

이 파일이 현재 작업 상태·실제 담당자·작업별 계획 역할의 원본이다. 2026-09-11에 기존33개 작업을 초기 등록했다. 초기 Backlog는 기존 코드가 미구현이라는 판정이 아니라 진척을 확인하지 않은 등록 상태다. 실제 착수 작업의 담당자는 아래 표와 작업 메모를 따른다.

2026-09-15 DEC-027로 **A 전투·웨이브 / B 경제·보드** 역할을 채택했다. 아래 마지막 열은 계획 역할이며 실제 담당자 열과 구분한다. A/B에 대응하는 팀원·가용 시간은 OPEN-001로 미정이다. P0의 남은 10개 작업과 P1의 직접 연속 기능에 계획 역할을 표시했고, 나머지 P1·P2 상세 배정은 첫 P0 실제 공수에 따라 정한다. TASK-CORE-01의 완료 담당·근거와 기존 37개 작업 상태는 유지하고 DEC-036의 로비 미니게임 작업을 Backlog로 추가해 총 38개다.

TASK-NET-01의 주관 A는 매치·공용 복제를, B는 명령 처리·경제/보드 복제를 맡는다. TASK-UI-01의 주관 B는 HUD·조작·안내를, A는 전투 정보·결과 위젯을 맡는다. DATA 작업은 A가 구조·로딩·검증을 주관하고 각 기능 담당자가 해당 데이터를 작성한다. Android 빌드는 A, PC 2인 검수는 B 주관이며 버그는 기능 담당자가 수정한다. 클래스·에셋 편집 책임과 상호 리뷰·업무량 조정은 [역할 기준](TEAM_ROLES.md)을 따른다.

단계 P0/P1/P2는 마일스톤, 우선순위 Must/Should/Could는 해당 단계의 중요도다. 둘을 혼용하지 않는다. 다음 착수 후보는 G0의 TASK-DATA-01/TASK-NET-01 계약과 G1의 TASK-MAP-01 준비이고 TASK-MOB-01 Android 도구·기본 패키징을 병행하며, 원작 확인이 필요한 작업은 확인된 규칙부터 진행한다. 실제 착수 지시는 별도 개발 요청 범위를 따른다.

상태: Backlog → Ready → InProgress → Review → QA → Done. 외부 의존성으로 막히면 Blocked와 원인을 기록한다. 완료할 때 아래 작업 메모에 증거를 연결한다.

QA 열은 대표 회귀 검사이며 완료 조건 전체를 대체하지 않는다. 링크된 작업 정의의 수용 기준도 함께 충족해야 한다. P0 검사는 P0 프로파일을 적용한다.

DEC-043에 따라 P0는 16종·10웨이브 기본 공격과 짧은 조작 안내까지다. 등급별 공격 강화·소환 확률 강화는 TASK-ECON-02, 고유 스킬은 TASK-COMBAT-02, 정식 튜토리얼은 TASK-UX-01의 P1 범위로 유지한다. P0 전체 통과에는 TASK-TEST-01의 PC 2인 검증과 TASK-MOB-01의 Android 실기기 터치 완주가 모두 필요하다. 기기 미정은 P0 최종 통과의 대기 사유이며 이번 문서 반영으로 개별 작업 상태·담당자를 변경하지 않는다.

| 작업 | 단계 | 우선순위 | 상태 | 담당자 | 명세 | 대표 검수 | 계획 역할 |
|---|---|---|---|---|---|---|---|
| [TASK-CORE-01](../BACKLOG_QA.md#TASK-CORE-01) | P0 | Must | Done | Codex | [TECH-001](../technical/ARCHITECTURE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-CORE-01) | — (완료 기록 유지) |
| [TASK-MAP-01](../BACKLOG_QA.md#TASK-MAP-01) | P0 | Must | InProgress | Codex 참고 제작 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-VIS-03](../BACKLOG_QA.md#QA-VIS-03), [QA-WAVE-04](../BACKLOG_QA.md#QA-WAVE-04) | B |
| [TASK-NET-01](../BACKLOG_QA.md#TASK-NET-01) | P0 | Must | InProgress | Codex 참고 제작 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03) | A 주관 · B 명령 처리 |
| [TASK-DATA-01](../BACKLOG_QA.md#TASK-DATA-01) | P0 | Must | InProgress | Codex 참고 제작 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-RNG-01](../BACKLOG_QA.md#QA-RNG-01) | A 주관 · 각 기능 데이터는 A/B |
| [TASK-COMBAT-01](../BACKLOG_QA.md#TASK-COMBAT-01) | P0 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-DMG-01](../BACKLOG_QA.md#QA-DMG-01), [QA-ECO-03](../BACKLOG_QA.md#QA-ECO-03), [QA-WAVE-04](../BACKLOG_QA.md#QA-WAVE-04) | A |
| [TASK-ECON-01](../BACKLOG_QA.md#TASK-ECON-01) | P0 | Must | Backlog | 미지정 | [SPEC-SUMMON](../design/SUMMON_ECONOMY.md) | [QA-ECO-01](../BACKLOG_QA.md#QA-ECO-01), [QA-BOARD-01](../BACKLOG_QA.md#QA-BOARD-01), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03), [QA-STACK-01](../BACKLOG_QA.md#QA-STACK-01), [QA-STACK-02](../BACKLOG_QA.md#QA-STACK-02) | B |
| [TASK-BOARD-01](../BACKLOG_QA.md#TASK-BOARD-01) | P0 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-BOARD-02](../BACKLOG_QA.md#QA-BOARD-02), [QA-BOARD-03](../BACKLOG_QA.md#QA-BOARD-03), [QA-BOARD-04](../BACKLOG_QA.md#QA-BOARD-04), [QA-BOARD-08](../BACKLOG_QA.md#QA-BOARD-08) | B |
| [TASK-WAVE-01](../BACKLOG_QA.md#TASK-WAVE-01) | P0 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-TIME-02](../BACKLOG_QA.md#QA-TIME-02), [QA-TIME-03](../BACKLOG_QA.md#QA-TIME-03), [QA-TIME-07](../BACKLOG_QA.md#QA-TIME-07) | A |
| [TASK-UI-01](../BACKLOG_QA.md#TASK-UI-01) | P0 | Must | InProgress | Codex 참고 제작 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-MOB-01](../BACKLOG_QA.md#QA-MOB-01), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) | B 주관 · A 전투/결과 UI |
| [TASK-TEST-01](../BACKLOG_QA.md#TASK-TEST-01) | P0 | Must | InProgress | Codex 참고 제작 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-02](../BACKLOG_QA.md#QA-NET-02), [QA-NET-03](../BACKLOG_QA.md#QA-NET-03) | B |
| [TASK-MOB-01](../BACKLOG_QA.md#TASK-MOB-01) | P0 | Must | InProgress | Codex 참고 제작 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-MOB-01](../BACKLOG_QA.md#QA-MOB-01), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) | A |
| [TASK-DATA-02](../BACKLOG_QA.md#TASK-DATA-02) | P1 | Must | Backlog | 미지정 | [SPEC-UNITS](../design/UNITS.md) | [QA-RNG-01](../BACKLOG_QA.md#QA-RNG-01), [QA-BOARD-10](../BACKLOG_QA.md#QA-BOARD-10) | A 주관 · 각 기능 데이터는 A/B |
| [TASK-COMBAT-02](../BACKLOG_QA.md#TASK-COMBAT-02) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-DMG-03](../BACKLOG_QA.md#QA-DMG-03), [QA-DMG-05](../BACKLOG_QA.md#QA-DMG-05), [QA-DMG-07](../BACKLOG_QA.md#QA-DMG-07), [QA-DMG-10](../BACKLOG_QA.md#QA-DMG-10), [QA-DMG-11](../BACKLOG_QA.md#QA-DMG-11), [QA-DMG-12](../BACKLOG_QA.md#QA-DMG-12), [QA-DMG-13](../BACKLOG_QA.md#QA-DMG-13), [QA-DMG-14](../BACKLOG_QA.md#QA-DMG-14) | A |
| [TASK-ECON-02](../BACKLOG_QA.md#TASK-ECON-02) | P1 | Must | Backlog | 미지정 | [SPEC-SUMMON](../design/SUMMON_ECONOMY.md) | [QA-RNG-04](../BACKLOG_QA.md#QA-RNG-04), [QA-ECO-06](../BACKLOG_QA.md#QA-ECO-06), [QA-ECO-07](../BACKLOG_QA.md#QA-ECO-07), [QA-BOARD-10](../BACKLOG_QA.md#QA-BOARD-10), [QA-MISSION-01](../BACKLOG_QA.md#QA-MISSION-01), [QA-DUNGEON-02](../BACKLOG_QA.md#QA-DUNGEON-02) | B |
| [TASK-WAVE-02](../BACKLOG_QA.md#TASK-WAVE-02) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [QA-WAVE-01](../BACKLOG_QA.md#QA-WAVE-01), [QA-WAVE-02](../BACKLOG_QA.md#QA-WAVE-02), [QA-WAVE-03](../BACKLOG_QA.md#QA-WAVE-03), [QA-TIME-06](../BACKLOG_QA.md#QA-TIME-06), [QA-HUNT-01](../BACKLOG_QA.md#QA-HUNT-01), [QA-DUNGEON-01](../BACKLOG_QA.md#QA-DUNGEON-01) | A |
| [TASK-BOT-01](../BACKLOG_QA.md#TASK-BOT-01) | P1 | Must | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-BOT-01) | A |
| [TASK-UI-02](../BACKLOG_QA.md#TASK-UI-02) | P1 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-BOARD-09](../BACKLOG_QA.md#QA-BOARD-09), [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) | B |
| [TASK-UX-01](../BACKLOG_QA.md#TASK-UX-01) | P1 | Must | Backlog | 미지정 | [SPEC-BOARD](../design/BOARD_UI.md) | [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02), [QA-MOB-03](../BACKLOG_QA.md#QA-MOB-03), [QA-MOB-04](../BACKLOG_QA.md#QA-MOB-04) | B |
| [TASK-SAVE-01](../BACKLOG_QA.md#TASK-SAVE-01) | P1 | Must | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-01](../BACKLOG_QA.md#QA-SAVE-01), [QA-SAVE-03](../BACKLOG_QA.md#QA-SAVE-03), [QA-META-01](../BACKLOG_QA.md#QA-META-01) | B |
| [TASK-ART-01](../BACKLOG_QA.md#TASK-ART-01) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-04](../BACKLOG_QA.md#QA-VIS-04) | A 캐릭터 / B 월드·UI |
| [TASK-ART-02](../BACKLOG_QA.md#TASK-ART-02) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-04](../BACKLOG_QA.md#QA-VIS-04) | A |
| [TASK-ART-03](../BACKLOG_QA.md#TASK-ART-03) | P1 | Must | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01), [QA-VIS-02](../BACKLOG_QA.md#QA-VIS-02) | A 적·보스 / B 맵 |
| [TASK-FX-01](../BACKLOG_QA.md#TASK-FX-01) | P1 | Should | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-PERF-03](../BACKLOG_QA.md#QA-PERF-03), [QA-VIS-01](../BACKLOG_QA.md#QA-VIS-01) | A 전투 / B 소환·합성 |
| [TASK-AUDIO-01](../BACKLOG_QA.md#TASK-AUDIO-01) | P1 | Should | Backlog | 미지정 | [ART-001](../art/ART_DIRECTION.md) | [QA-MOB-03](../BACKLOG_QA.md#QA-MOB-03) | A 전투 / B UI·로비 |
| [TASK-PERF-01](../BACKLOG_QA.md#TASK-PERF-01) | P1 | Must | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-PERF-01](../BACKLOG_QA.md#QA-PERF-01), [QA-PERF-02](../BACKLOG_QA.md#QA-PERF-02), [QA-PERF-03](../BACKLOG_QA.md#QA-PERF-03) | A 주관 / B UI·에셋 지원 |
| [TASK-BAL-01](../BACKLOG_QA.md#TASK-BAL-01) | P1 | Must | Backlog | 미지정 | [SPEC-BATTLE](../design/BATTLE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-BAL-01) | A 전투 / B 경제 |
| [TASK-QA-01](../BACKLOG_QA.md#TASK-QA-01) | P1 | Must | Backlog | 미지정 | [PLAN-001](ROADMAP.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-QA-01) | A/B 공동 |
| [TASK-SERVER-01](../BACKLOG_QA.md#TASK-SERVER-01) | P2 | Should | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [QA-NET-01](../BACKLOG_QA.md#QA-NET-01), [QA-NET-02](../BACKLOG_QA.md#QA-NET-02) | A |
| [TASK-ACCOUNT-01](../BACKLOG_QA.md#TASK-ACCOUNT-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-02](../BACKLOG_QA.md#QA-SAVE-02), [QA-NET-08](../BACKLOG_QA.md#QA-NET-08) | B 주관 / A 서버 결과 |
| [TASK-ROOM-01](../BACKLOG_QA.md#TASK-ROOM-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-SAVE-04](../BACKLOG_QA.md#QA-SAVE-04) | B 주관 / A 서버 배정 |
| [TASK-MINIGAME-01](../BACKLOG_QA.md#TASK-MINIGAME-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md#lobby-minigames) | [QA-MINI-01](../BACKLOG_QA.md#QA-MINI-01), [QA-MINI-02](../BACKLOG_QA.md#QA-MINI-02), [QA-MINI-03](../BACKLOG_QA.md#QA-MINI-03) | A/B 각 1종 가배정·규칙 확정 후 조정 |
| [TASK-REJOIN-01](../BACKLOG_QA.md#TASK-REJOIN-01) | P2 | Should | Backlog | 미지정 | [SPEC-COOP](../design/COOP_META.md) | [QA-REJOIN-01](../BACKLOG_QA.md#QA-REJOIN-01), [QA-REJOIN-02](../BACKLOG_QA.md#QA-REJOIN-02), [QA-REJOIN-03](../BACKLOG_QA.md#QA-REJOIN-03), [QA-REJOIN-04](../BACKLOG_QA.md#QA-REJOIN-04) | A 주관 / B 계정·보상·UI |
| [TASK-OPS-01](../BACKLOG_QA.md#TASK-OPS-01) | P2 | Should | Backlog | 미지정 | [TECH-001](../technical/ARCHITECTURE.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-OPS-01) | A 주관 / B 계정 로그 |
| [TASK-RELEASE-01](../BACKLOG_QA.md#TASK-RELEASE-01) | P2 | Should | Backlog | 미지정 | [PLAN-001](ROADMAP.md) | [작업 완료 조건](../BACKLOG_QA.md#TASK-RELEASE-01) | A/B 공동 |

## 개발 도구 확장

2026-09-12에 아래 4개를 추가해 당시 등록 작업은 37개였다. 현재는 DEC-036의 P2 로비 미니게임 작업을 더해 총 38개다. P1 Should 확장 계획이며 기존 33개의 상태·담당자는 유지한다. 상세 기능·선행·완료 조건은 [도구 작업 연결](../technical/AUTOMATION.md#tool-task-map)을 따른다. 구현 일정·공수는 미산정이며 P0의 필수 도구 구축으로 해석하지 않는다.

| 작업 | 단계 | 우선순위 | 상태 | 담당자 | 명세 | 대표 검수 | 계획 역할 |
|---|---|---|---|---|---|---|---|
| [TASK-TOOLS-01](../BACKLOG_QA.md#TASK-TOOLS-01) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md), [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-01](../BACKLOG_QA.md#QA-TOOLS-01), [QA-TOOLS-02](../BACKLOG_QA.md#QA-TOOLS-02) | 미정 |
| [TASK-TOOLS-02](../BACKLOG_QA.md#TASK-TOOLS-02) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md) | [QA-TOOLS-03](../BACKLOG_QA.md#QA-TOOLS-03) | 미정 |
| [TASK-TOOLS-03](../BACKLOG_QA.md#TASK-TOOLS-03) | P1 | Should | Backlog | 미지정 | [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-04](../BACKLOG_QA.md#QA-TOOLS-04), [QA-TOOLS-05](../BACKLOG_QA.md#QA-TOOLS-05), [QA-TOOLS-06](../BACKLOG_QA.md#QA-TOOLS-06) | 미정 |
| [TASK-TOOLS-04](../BACKLOG_QA.md#TASK-TOOLS-04) | P1 | Should | Backlog | 미지정 | [SPEC-EDITOR](../technical/EDITOR_TOOLS.md), [SPEC-AUTOMATION](../technical/AUTOMATION.md) | [QA-TOOLS-07](../BACKLOG_QA.md#QA-TOOLS-07), [QA-TOOLS-08](../BACKLOG_QA.md#QA-TOOLS-08) | 미정 |

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
