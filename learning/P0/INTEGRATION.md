# P0 게이트 통합

참고 제작 Draft / 실제 학습 Planned. 공통 출발점과 실행 도구는 [COMMON](COMMON.md), 최신 상태는 [작업 기록](../../docs/production/P0_REFERENCE_RUN.md)을 따른다.

G0에서 독립 작성한 A/B 공통 코드의 타입·로더·초기화·상태 원본을 비교한다. 합의한 한 구현만 실행하고 B의 Controller/CommandProcessor를 GameMode 연결부에 결합한다. 각 역할과 통합 Editor 빌드, 독립 기대값 검사, ARCH-01~06 리뷰 후 G1에 들어간다.

G1 이후에도 각 기능 커밋→통합→실제 실행→학습 기록을 반복한다. 참고 완성 코드를 learn 브랜치에 병합하지 않는다. G1 실제 양쪽 화면/입력 이전 G2 확장은 금지한다.

현재 G0 비교·수정·통합 SHA와 실행 증거는 생성 중이며 Verified가 아니다.
