# P0 A 참고 구현 수업

참고 자료 제작은 A-01, G1-A-01/02, G2-A-01/02/03 **Verified**, A-02와 G3-A-01/02/03 **Draft**이며 실제 학습자 진행은 모두 **Planned**다. Verified는 아래 재현 증거의 범위이며10웨이브·패키지·Android를 포함한 P0 완료를 뜻하지 않는다.

공통 출발점은 `8c6856d235de87cc28c12b49ca775bd0937334a5`, A의 G0 코드 기준은 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`다. 최초 구현 `0bab1ad7241444fa73ce2ca41dbd8863046351dd`에서 Editor 빌드가 실패하여 오류 경로 문자열 연결을 수정했다. `reference/p0-a`는 비교용이고 `learn/p0-a`는 출발점에서 직접 작성한다. 참고 코드를 학습 브랜치에 병합하지 않는다.

1. [A-01: 검증된 P0 규칙 snapshot](G0_01_DATA.md)
2. [A-02: 서버 매치와 참가자 수명](G0_02_MATCH.md)
3. [G1-A-01: 닫힌 선형 경로 계산](G1_01_ROUTE_MODEL.md)
4. [G1-A-02: 영속 적 개체와 로컬 표시](G1_02_ENEMY_ACTOR.md)
5. [G2-A-01: 준비된 유닛과 표시](G2_01_UNIT.md)
6. [G2-A-02: 예정 공격과 단일 사망](G2_02_COMBAT.md)
7. [G2-A-03: 서버 서비스 연결](G2_03_MATCH.md)
8. [G3-A-01: 예정 생성과 일반 적 수](G3_01_WAVES.md)
9. [G3-A-02: 열린 현재 시각과 단일 종료](G3_02_TIMELINE.md)
10. [G3-A-03: 공용 전투 정보와 결과 표시](G3_03_WIDGETS.md)
11. [G4-A-01: Android 패키지와 실기기](G4_01_ANDROID.md) — Draft, 구성 요소·기기 대기

G2 세 수업은 G1 완료 `4861b987f3e2fe78bcc159d1b6a85008543a938b`에서 수업 순서로56파일을 조립하고 최종 소스 `ae6be1b0b06ed733425e01632a341fb4db4cad59`까지 재현한 범위에서 **Verified**다. [공통 재현 절차·증거](../evidence/G2_REPLAY/README.md), [정식 검수](../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md), [A 역할 경고와 최종 결과 구분](G2_EVIDENCE.md)을 따른다. Editor·39개 무경고 자동화·후속12개 무경고 검사·실제 두 프로세스20단계를 확인했다. 실제 학습자는 **Planned**이며 학습 브랜치에는 완성 코드를 병합하지 않았다.

공통 규칙·권한·호출 계약은 [공통 구현 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [데이터 명세](../../../docs/DATA_SCHEMA.md), [제작 및 실제 학습 분리](../../WORKFLOW.md)를 읽는다. 수업은 해당 기능의 차이와 재현 절차만 기록한다.

G0 A-01은 출발점 파일 조립·Editor·로더 자동화를 [재현](../evidence/G0_REPLAY/SUMMARY.md)했다. 최초 A 역할의 [실행 증거](evidence/G0_RUNTIME.md)와 수업 재현은 구분한다. A-02의 개별 수업 재현 상태는 해당 수업을 따른다.

G1 재현 시작은 canonical G0 `649c1dedd6832c41089a76b59bc76518cd262296`, 완료 소스는 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`다. 새 detached 작업 경로에서 수업 순서로 조립한36파일의 blob 일치, Editor Pass, 전체22자동화 Pass와 실제 두 프로세스의 양쪽7화면·두 바퀴·입력을 [공통 재현 증거](../evidence/G1_REPLAY/SUMMARY.md)로 확인했다. 최초 호스트 표시 실패와 수정은 [Actor 수업](G1_02_ENEMY_ACTOR.md)에 남겼다. 재현 절차는 [G1 통합 수업](../G1_INTEGRATION.md)을 따른다. PIE·패키지·물리 터치·Android는 이 검수에 포함하지 않는다.

G3 세 수업은 구현 코드를 작성한 Draft다. [공통 G3 증거·통합 순서](G3_EVIDENCE.md)에서 실제 빌드·실행·재현 상태를 구분하며, 완료 코드를 학습자가 작성했다고 기록하지 않는다.
