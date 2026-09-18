# A·B 개발 학습 자료

**선택 자료 · 현재 상태: 기존 P0 삭제, P1/P2 계획·양식만 보존**

Codex가 A 개발자 시점과 B 개발자 시점으로 각각 기능을 구현하고 제작 과정을 기록한 뒤 통합한다. 실제 개발자 A와 B는 각자 별도의 개발 브랜치에서 자기 자료를 따라 구현하고, 자신들이 작성한 결과를 통합한다. 이 흐름을 P0·P1·P2와 이후 마일스톤에 반복한다.

2026-09-16 사용자 요청으로 기존 P0 참고 구현·A/B 학습 작업과 P0 자료를 삭제했다. [초기화 내역](../docs/production/P0_RESET.md). 새 P0 수업과 브랜치는 아직 없으며 실제 구현/검수에 맞춰 다시 작성한다. 아래 P1/P2는 이후 제작 계획이다.

## 읽는 순서

1. [제작·학습·브랜치 운영](WORKFLOW.md)을 읽는다.
2. 해당 마일스톤의 공통 출발점을 확인한다.
3. A 또는 B 경로에서 코드 작성과 Unreal 설정을 순서대로 재현한다.
4. 단계마다 상대 역할의 산출물을 전달받아 함께 검증한다.
5. 통합 결과를 검수하고 다음 마일스톤으로 이어간다.

| 마일스톤 | 시작점 | A 경로 | B 경로 | 통합 | 현재 상태 |
|---|---|---|---|---|---|
| P0 | [새 계획](../docs/design/P0_REPLAN.md) | 삭제·재작성 대기 | 삭제·재작성 대기 | 새 구현 검수 대기 | 기존 자료 제거 완료 |
| P1 | [범위·기준점](P1/README.md) · [공통 준비](P1/COMMON.md) | [전투·콘텐츠 확장](P1/A/README.md) | [경제·UI 확장](P1/B/README.md) | [P1 통합](P1/INTEGRATION.md) | 같은 구조 준비, 일부 역할 미배정 |
| P2 | [범위·기준점](P2/README.md) · [공통 준비](P2/COMMON.md) | [A 경로](P2/A/README.md) | [B 경로](P2/B/README.md) | [P2 통합](P2/INTEGRATION.md) | 구조만 준비, 서비스 범위·역할 미확정 |

## 폴더 구조

```text
learning/
  README.md
  WORKFLOW.md
  REMOVAL.md
  templates/
    LESSON.md
    MILESTONE.md
  tools/
    validate.mjs
  P1/                       # P2/도 같은 구조. P0/는 현재 삭제됨
    README.md               # 범위·출발/완료 기준점·제작/학습 상태
    COMMON.md               # 공통 기반과 상대 산출물 전달 규칙
    A/
      README.md             # A의 학습 순서와 질문
    B/
      README.md             # B의 학습 순서와 질문
    INTEGRATION.md          # A/B 연결 순서와 실제 검수 기록
```

실제 단계를 제작할 때 A/ 또는 B/에 `01-주제.md`와 필요한 캡처를 추가한다. 캡처 파일은 같은 역할 폴더 안 `assets/`에 둔다. 아직 없는 수업 파일·코드·화면을 완성본처럼 링크하지 않는다.

## 정식 개발 문서와의 관계

학습 자료는 [역할 분담](../docs/production/TEAM_ROLES.md), [A 구현 설계](../docs/technical/IMPLEMENTATION_A.md), [B 구현 설계](../docs/technical/IMPLEMENTATION_B.md), [공통 구현 계약](../docs/technical/IMPLEMENTATION_SHARED.md)을 참조한다. 게임 규칙·API·작업 상태·제품 QA의 원본은 정식 문서다.

이 폴더에는 설명·실습 순서·질문·학습용 캡처·참고/실제 개발 기준점의 대응을 둔다. 런타임 코드·필수 에셋·빌드 설정·제품 완료 증거의 유일한 원본을 두지 않는다. 정식 문서/코드에서 이 폴더로 필수 참조를 만들지 않는다.

## 검사와 제거

저장소 루트에서 `node learning/tools/validate.mjs`로 구조·링크·정식 문서/소스/설정/도구의 텍스트 참조를 확인한다. 이 검사는 선택 실행이며 정식 빌드·필수 CI에 연결하지 않는다. Unreal 바이너리 에셋의 참조나 실제 기능 성공 여부는 검증하지 않는다.

나중에 필요 없으면 [제거 안내](REMOVAL.md)에 따라 이 폴더와 루트 README의 선택 안내 블록을 제거할 수 있다. 문서 삭제와 Git 브랜치/worktree 삭제는 별도 작업이다.
