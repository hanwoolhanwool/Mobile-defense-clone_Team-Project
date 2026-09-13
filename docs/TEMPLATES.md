---
id: DOC-TEMPLATES
version: 0.2.0
status: Baseline
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-01
applies_to: 문서·작업·검수 작성 양식
baseline_basis: DEC-023 사용자 보완 진행 지시
---

# 실무 템플릿

[기획 허브](README.md) · 필요한 양식만 복사합니다. 한 줄 수정에 전체 양식을 채우지 않습니다.

## 기능 명세

```markdown
---
id: SPEC-새기능
version: 0.1
status: Draft
owner: unassigned
updated: YYYY-MM-DD
# 문서 검토 후에만 추가: reviewed, review_run, applies_to
# 실제 실행 후에만 추가: verified, verified_run, verification_scope
---

# 기능명

## 목적과 플레이어 경험
누가 어떤 상황에서 무엇을 할 수 있어야 하는가?

## 범위
이번 마일스톤의 포함/제외 항목.

## 규칙과 예외
입력, 전제조건, 비용, 상태변경, 실패 처리, 권한.
수치가 있는 데이터의 편집 원본과 단위.

## 화면과 피드백
터치 동작, 비활성 조건, 오류 문구, 접근 경로.

## 완료 조건
- Given: ... / When: ... / Then: ...

## 연결
작업 ID, 데이터 파일, QA ID, 관련 결정.

## 남은 가정
미정 내용, 작업용 기본값, 검증할 시점.
```

새 문서를 전체 GDD에도 포함하려면 build-planning의 원본 목록과 장 번호 체계를 의도적으로 갱신합니다. 현재 읽기본은 기존20장 이관을 검증하는 기준으로 고정돼 있습니다. 작은 새 규칙은 먼저 해당 기능 원본의 하위 절에 추가합니다.

## 개발 작업 — 원작 확인 후 채우는 소환 작업 예시

TASK-ECON-01의 작성 양식이다. 경제·확률·빈 칸 처리·보장은 DEC-021에 따라 원작 대조 전 초안이다. 이 양식을 복사해도 구현 규칙이 확정되지 않는다. [경제 명세](design/SUMMON_ECONOMY.md), [원작 대조](product/ORIGINAL_REFERENCE.md), [완료 조건](BACKLOG_QA.md#TASK-ECON-01)을 확인하고 빈칸을 실제 근거로 채운다.

```markdown
제목: [TASK-ECON-01] 서버 권한 소환 구현
단계: P0
상태: Backlog
담당자: 미지정
명세: SPEC-SUMMON, TECH-001, DATA_SCHEMA
선행: TASK-DATA-01, TASK-NET-01, 구현할 원작 규칙 확인

목표: 두 클라이언트에서 같은 소환 결과와 자원 상태를 확인한다.
확인한 원작 버전·모드·관찰 ID:
확정한 소환 비용·증가식·결과 등급·보장 여부:
빈 칸 부족·실패 시 처리 근거:
미확인 규칙과 이번 구현에서 제외할 범위:

완료 조건:
- 확인한 초기 자원 [값]에서 [횟수]회 요청 후 잔액 [값]과 다음 비용 [값]을 확인한다.
- 같은 RequestId를 재전송해도 한 번만 소비한다(DEC-020 기술 계약).
- 두 클라이언트의 자원·보드가 일치한다.
- 등급·보장·실패 사례는 확인한 명세와 QA 기대값에 맞춰 추가한다.

관련 QA / 실행 Run / 실제 결과 / 증거:
Done 판정 시 verification.json의 조건별 Pass·실행·증거 연결:
남은 문제:
```

## 기획 변경 제안

```markdown
ID: CR-번호
상태: Proposed / Applied / Rejected
작성일·작성자:
연결 명세·작업:

문제: 어떤 관찰 때문에 바꾸는가?
현재 동작/값:
제안 동작/값:
대안과 선택 이유:
영향: 데이터, UI, 네트워크, 세이브, 성능, 일정, QA 중 해당 항목.
결정 근거: 사용자 지시 또는 책임자의 판단과 날짜.
검증 방법:
반영 결과·변경 이력 ID:
```

## 검수 실행 기록

```markdown
Run ID: RUN-YYYYMMDD-NN
관련 작업·QA:
실행자 / 실행일:
빌드·Git 커밋 또는 미커밋 표시:
RulesVersion / 데이터 해시(데이터 관련 검사 시):
환경: UE 버전, 기기, OS, 해상도, 네트워크 조건.
결과: Pass / Fail / NotRun
실제 관찰값:
증거: 로그·영상·스크린샷의 저장소 경로 또는 내부 링크.
보관: EVIDENCE.md에 따른 경로·SHA-256·보관 만료/유지 조건.
Done 판정: verification.json의 작업 ID·조건별 결과·판정 Run·증거 연결.
실패 시 재현 순서:
남은 문제·수정 작업:
```

## 의사결정 기록

```markdown
ID: DEC-번호
기록일:
상태: Confirmed / Working / Proposed / Superseded
문제와 제약:
선택과 근거:
고려한 대안:
영향과 재검토 조건:
연결 명세·변경:
대체 관계(있는 경우):
```

오타나 단순 리팩터링에는 결정 기록을 만들지 않습니다. 장기 방향·비용·협업에 영향을 주는 선택에 사용합니다.
