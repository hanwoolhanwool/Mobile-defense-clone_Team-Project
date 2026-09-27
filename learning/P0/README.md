# P0 참고 구현 학습 시작

참고 제작 상태 **Draft**, 실제 개발자 A/B 상태 **Planned**. G0~G4 검수 진행 중이며 P0 완료본이 아니다.

공통 출발 SHA: `8c6856d235de87cc28c12b49ca775bd0937334a5`. `learn/p0-a`, `learn/p0-b`, `learn/p0-integration`은 이 출발점에 유지한다. 참고 코드를 실제 학습 결과로 기록하지 않는다.

순서: [공통 계약](COMMON.md) → [A 수업](A/README.md)과 [B 수업](B/README.md) → [각 게이트 통합](INTEGRATION.md). [제작 방식](../WORKFLOW.md)과 [수업 양식](../templates/LESSON.md)을 사용한다.

현재 참고 프로젝트는 `C:/Users/iam12/P0_reference_integration/Mobile_defense_clone.uproject`다. 원래 폴더는 기획·기본 프로젝트 출발점을 보존한다. 완료 SHA와 실제 검증 범위는 [작업 기록](../../docs/production/P0_REFERENCE_RUN.md) 및 각 수업에서 갱신한다.

## 수업 순서와 진입 조건

| 순서 | 공통 / A / B / 통합 | 상태 |
|---|---|---|
| G0 | Schema2 타입·로더·매치 / 명령·거절 / 독립 구현 비교·Editor 빌드 | 코드 게이트 Pass, 수업 재현 A/B각4Pass·A데이터수업 Verified |
| G1 | 경로 생성·이동 / 보드·카메라·전 셀 입력 / 양쪽 실제 화면·2바퀴 | Pass, 역할·[통합 수업](G1_INTEGRATION.md) 재현 Verified |
| G2 | 16종 기본 공격·사망 / 경제·뭉치·합성·판매 / 단일 보상·실패 불변 | Pass, 39+보충12자동화·실제 두 프로세스20단계; A/B·[통합 수업](G2_INTEGRATION.md) 재현 Verified |
| G3 | 웨이브·보스·결과 / HUD / PC 별도2프로세스·5판·성능 | 수정 후56자동화 무경고·실제 PIEv2·짧은2000회 부하 검사 통과; 최종 패키지/20분 부하/새 재현 진행, [통합 수업](G3_INTEGRATION.md) Draft |
| G4 | Android 빌드·기기 / 터치·SafeArea / 실기기10웨이브 | 기기 연결 및 통합 빌드 필요 |

수업 문서의 절차로 시작 SHA에서 재현하고 실행 증거까지 연결한 경우만 Verified로 변경한다.
