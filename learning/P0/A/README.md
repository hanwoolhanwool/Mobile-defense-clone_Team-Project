# P0 A 참고 구현 수업

참고 자료 제작 **Draft**, 실제 학습자 진행 **Planned**. 현재 범위는 G0 기반과 G1 경로·표시이며 전투 구현·실행 통과를 뜻하지 않는다.

공통 출발점은 `8c6856d235de87cc28c12b49ca775bd0937334a5`, A의 G0 코드 기준은 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`다. 최초 구현 `0bab1ad7241444fa73ce2ca41dbd8863046351dd`에서 Editor 빌드가 실패하여 오류 경로 문자열 연결을 수정했다. `reference/p0-a`는 비교용이고 `learn/p0-a`는 출발점에서 직접 작성한다. 참고 코드를 학습 브랜치에 병합하지 않는다.

1. [A-01: 검증된 P0 규칙 snapshot](G0_01_DATA.md)
2. [A-02: 서버 매치와 참가자 수명](G0_02_MATCH.md)
3. [G1-A-01: 닫힌 선형 경로 계산](G1_01_ROUTE_MODEL.md)
4. [G1-A-02: 영속 적 개체와 로컬 표시](G1_02_ENEMY_ACTOR.md)

공통 규칙·권한·호출 계약은 [공통 구현 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [데이터 명세](../../../docs/DATA_SCHEMA.md), [제작 및 실제 학습 분리](../../WORKFLOW.md)를 읽는다. 수업은 해당 기능의 차이와 재현 절차만 기록한다.

2026-09-18 A Editor 재빌드 Pass, Unreal NullRHI 자동화4개 Pass/0Fail/0NotRun을 확인했다. [실제 증거](evidence/G0_RUNTIME.md). 공통 통합 검수가 끝나야 G1 경로 수업으로 진행한다. 이 수업의 문서 재현은 아직 수행하지 않았으므로 Verified가 아니다. PIE·패키지·네트워크·Android는 미실행이다.

G0 통합을 받은 G1 공통 시작은 `4787bf1a3a0d866aa206d148586594b3c710f957`이다. G1-A 코드 `bce4b7b0abe7787e1efe54e9af8805612bdffe5d`는 경로 계산·적 이동·표시만 포함한다. G1 A Editor Pass, NullRHI 자동화5Pass/0Fail/0NotRun을 [실제 증거](G1_EVIDENCE/README.md)에서 확인했다. 최초 통합2프로세스에서 호스트 표시 시각 문제가 발견되어 `bb1d100`으로 수정하고 [수업에 실패 원인](G1_02_ENEMY_ACTOR.md)을 기록했다. 수정 후 화면·신규 회귀 테스트 재검증 대기이며 참고 수업 Draft/실제 학습 Planned를 유지한다.
