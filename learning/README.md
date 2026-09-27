# A·B 개발 학습 자료

**선택 자료 · 현재 상태: 새 P0의 G0~G3 PC 재현 Verified / G4 Draft / 실제 학습자 Planned**

Codex가 A 개발자 시점과 B 개발자 시점으로 각각 기능을 구현하고 제작 과정을 기록한 뒤 통합한다. 실제 개발자 A와 B는 각자 별도의 개발 브랜치에서 자기 자료를 따라 구현하고, 자신들이 작성한 결과를 통합한다. 이 흐름을 P0·P1·P2와 이후 마일스톤에 반복한다.

2026-09-16 [초기화](../docs/production/P0_RESET.md) 이후 새 공통 출발점8c6856d에서 P0 참고 구현과 수업을 작성했다. [P0 시작 문서](P0/README.md)의 실제 재현 범위와 미검증을 구분한다. learn 세 브랜치는 출발점에 유지하고 학습자의 완성 코드로 기록하지 않는다. 실제 청음·Android 검수가 남아 P0는 최종 미완료이며 아래 P1/P2는 기존 계획이다.

## 읽는 순서

브라우저에서 읽으려면 [HTML 학습실](html/index.html)을 연다. 왼쪽에서 A12개·B11개·통합4개의 실습 순서를 탐색하고, 전체 문서에서64개의 제목·본문·코드를 검색한다. 수업은 구획별 카드와 목차로 읽으며 이전/다음 이동·코드 복사·개별/전체 실습 인쇄를 제공한다. 각 Markdown에 대응하는 개별 HTML도 보존한다. 인터넷이나 Unreal Editor 실행은 필요 없다. 정식 명세·코드·검수 증거 링크는 원본 파일을 사용하므로 저장소 폴더 구조를 유지한다.

메모·북마크·개인 실습 체크는 **HTML 학습실 시작 페이지의 이 브라우저 기록**이다. 원문의 참고 제작 체크·Verified/Draft·실제 학습자 Planned·SHA와는 분리된다. 원문 체크가 개인 완료로 자동 복사되지 않으며 개인 실습 확인은 세 항목을 직접 체크한 뒤 기록한다. 개별 HTML에서 학습실로 열면 같은 기록을 사용한다. 브라우저/파일 경로를 옮기거나 브라우저 데이터를 지우기 전에는 ‘나의 학습 기록 → 기록 백업’으로 JSON을 내보내고 새 학습실에서 불러온다. 복원과 초기화는 현재 개인 기록을 바꾸므로 확인 대화상자를 거친다. 저장이 허용되지 않는 환경에서는 안내에 따라 현재 탭의 기록을 백업한다.

1. [제작·학습·브랜치 운영](WORKFLOW.md)을 읽는다.
2. 해당 마일스톤의 공통 출발점을 확인한다.
3. A 또는 B 경로에서 코드 작성과 Unreal 설정을 순서대로 재현한다.
4. 단계마다 상대 역할의 산출물을 전달받아 함께 검증한다.
5. 통합 결과를 검수하고 다음 마일스톤으로 이어간다.

| 마일스톤 | 시작점 | A 경로 | B 경로 | 통합 | 현재 상태 |
|---|---|---|---|---|---|
| P0 | [시작](P0/README.md) · [공통](P0/COMMON.md) | [A 수업](P0/A/README.md) | [B 수업](P0/B/README.md) | [게이트 통합](P0/INTEGRATION.md) | PC 명시 범위 Verified, G4 Draft, 학습자 Planned |
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
  P0/                       # P1/, P2/도 같은 공통 구조
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

HTML 원본은 각 Markdown이다. 수정 후 `node learning/tools/build-html.mjs`로 전체 읽기본을 다시 만들고 `node learning/tools/build-html.mjs --check`로 원본 해시·내용·학습 문서 링크와 앵커의 일치를 확인한다. HTML을 직접 수정하지 않는다. 생성기는 Node.js20 이상과 동봉한 marked17.0.5(MIT)를 쓰며 별도 설치·네트워크 요청이 없다. 생성 파일과 도구 모두 이 선택 학습 폴더 안에 있다. HTML 변환으로 참고 자료나 실제 학습자의 상태를 변경하지 않는다.

나중에 필요 없으면 [제거 안내](REMOVAL.md)에 따라 이 폴더와 루트 README의 선택 안내 블록을 제거할 수 있다. 문서 삭제와 Git 브랜치/worktree 삭제는 별도 작업이다.
