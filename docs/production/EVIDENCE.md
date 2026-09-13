---
id: OPS-EVIDENCE
version: 0.1.0
status: Baseline
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-01
applies_to: 문서·빌드·검수 증거 보관
baseline_basis: DEC-023 사용자 보완 진행 지시
---

# 검증 증거 보관

[관리 방식](../WORKFLOW.md) · [검수 기록](TEST_RUNS.md) · [완료 판정](verification.json)

## 저장 위치와 보관 기간

| 대상 | 위치 | 보관 기준 |
|---|---|---|
| Run 요약·판정·작업/QA 연결 | TEST_RUNS.md 및 verification.json | 프로젝트 유지 기간 전체. 이전 실패를 삭제하거나 성공으로 덮어쓰지 않음 |
| 완료 판정에 필요한 작은 로그·명령·환경·입력 목록·관찰 | evidence/archived/Run ID/ | 프로젝트 유지 기간 전체. Git으로 보관하고 파일별 SHA-256 검사 |
| 대용량 패키지·정확한 전체 입력 스냅샷 | Saved/BuildRuns 또는 팀 아티팩트 보관소 | 실행일부터 최소 30일. 마일스톤 승인·회귀 문제 해결·필수 증거 이관 확인 전 삭제 금지 |
| 릴리스 패키지·회귀 재현에 유일한 입력 | 팀 아티팩트 보관소 | 해당 버전 지원 종료까지. 보관소를 정하지 못했다면 로컬 원본 유지 |

현재 팀 원격 아티팩트 보관소는 미지정이다. 로컬 Saved 폴더는 공유·백업 보장이 없으므로, 완료 판정에 필요한 텍스트 증거를 저장소에 복사했다. 만료일이 지나도 자동 삭제하지 않는다.

## 이번에 보존한 증거

2026-09-13의 Windows 빌드·실행에 필요한 명령, 환경, 입력 파일 목록, 상세 로그, 원래 Fail 결과, 산출물 재검사, 실행 관찰을 복사했다. [보관 목록·원본 경로·해시](evidence/archive-manifest.json)는 29개 파일을 식별한다. 원본 바이트를 유지했으며 Git 개행 변환도 끈다.

패키지와 전체 Input/Workspace 스냅샷은 원래 Saved/BuildRuns에 남긴다. 실행 파일은 [기존 산출물 해시](evidence/RUN-20260913-03-artifacts.json)로 식별한다. 로컬 기준 커밋은 현재 공유 입력이며 9월 13일의 토큰 포함 전체 스냅샷과 동일하다고 주장하지 않는다. 과거 입력 목록과 토큰 분리 변경을 함께 보존한다.

## 새 Run을 보관할 때

1. 검수 기록에 실행자·일시·빌드/입력 식별자·판정 범위·결과·미검증 항목을 적는다.
2. 필요한 작은 로그와 명령·관찰을 evidence/archived/Run ID/에 복사한다. 개인 토큰·비밀번호가 포함된 설정 원본이나 전체 Input 폴더를 그대로 올리지 않는다.
3. archive-manifest.json의 files에 저장소 상대 path, 원래 source, bytes, 소문자 SHA-256을 추가한다. 기존 증거는 덮어쓰지 않고 새 Run으로 연결한다.
4. 완료 판정에는 verification.json의 runs와 completions를 추가한다. TEST_RUNS의 해당 절에 `- 기록 판정: Pass` 또는 Fail/NotRun을 적고 JSON과 일치시킨다. 조건의 충분성은 BACKLOG_QA의 수용 기준과 대조한다. Fail/NotRun을 완료 판정 Run으로 사용하지 않는다.
5. node tools/check-project.mjs를 실행해 링크·실행 관계·파일 해시·현황을 확인한다. 원격 보관소로 이관할 때는 접근 가능한 위치와 해시를 갱신한 뒤 로컬 자료를 정리한다.

문서 정적 검사와 데이터 검산만으로 UE 빌드·게임 기능·Android 검수를 통과 처리하지 않는다.
