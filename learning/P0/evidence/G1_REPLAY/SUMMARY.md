# G1 수업 재현 결과

참고 제작 재현 **Pass**. 실제 학습자 상태 **Planned**이며 learn 브랜치에 코드를 병합하지 않았다.

- 시작: canonical G0 `649c1dedd6832c41089a76b59bc76518cd262296`.
- 완료 소스: `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`.
- 새 detached 경로: `C:/Users/iam12/P0_lesson_replay_g1`. HEAD는 G0에 유지했다. [절차](README.md)의8단계·36파일은 [assembly.json](assembly.json)에서 원본/재현 blob이 모두 일치한다.
- 재현 Editor: [Pass](editor.json),88.39초. 전체 UE 계약 자동화: [22Success/0Fail/0NotRun](automation.json). 이 검사는 NullRHI이며 화면 검수가 아니다.
- 실제 두 프로세스: [명령·종료 결과](../../../../docs/production/evidence/RUN-20260918-G1/final-pair.json), [상세 요약·캡처 해시](../../../../docs/production/evidence/RUN-20260918-G1/final-pair-summary.json). listen host1338/client1336 Pass. 차이2개는 서버 전용 경로 생성 검사다.
- 두 참가자에서 자기 보드 아래, 생성 왼쪽, 중앙 같은 오른쪽 방향, 전체36셀 투영/소유18칸 수락/상대18칸 거절/자기72모서리/36개 EngineTouch를7뷰포트에서 검사했다. 두 적 ID1001/1002와 RouteIndex·Actor가6160cm 이상 유지됐다.
- 화면 크기:540×1170,1080×2340,720×1280,720×1600,768×1024,800×1280,1280×720. 셀폭 각각60/120/65.641/80/52.513/65.641/36.923px. 최종 세로·가로 캡처에서 두6×3 전체 경계·한글·선택·생성·중앙 화살표를 확인했다. 직전 동일 표시의14장도 모두 열람했고 축소 경계 수정 후 가로 두 화면을 재확인했다.
- 전체 로그·14PNG·원본 JSON: 재현 폴더 `Saved/P0Runs/Replay-G1-{assembly,editor,contracts,pair}`. 원본 로그를 축약하거나 과거 결과로 대체하지 않았다.

실제 화면: [host 세로](../../../../docs/production/evidence/RUN-20260918-G1/final-host-view-0.png), [client 세로](../../../../docs/production/evidence/RUN-20260918-G1/final-client-view-0.png), [host 가로](../../../../docs/production/evidence/RUN-20260918-G1/final-host-view-6.png), [client 가로](../../../../docs/production/evidence/RUN-20260918-G1/final-client-view-6.png). 실패 원인·수정·독립 리뷰는 [공통 리뷰](../../../../docs/production/evidence/RUN-20260918-G1/REVIEW.md)를 따른다.

범위와 한계: UnrealEditor `-game` 별도2프로세스, 실제 GPU offscreen 렌더링과 실제 엔진 입력 바인딩을 사용했다. PIE·패키지·물리 터치·Android는 포함하지 않는다. 고정150cm/s 적2개가 있는 명시적 G1 fixture이며 전투·경제·웨이브 성공을 뜻하지 않는다. 프레임 P95 host16.6670/client16.6669ms는60FPS 제한·캡처/resize를 포함한 적2개 참고치다. P0 대표 부하나 기기 성능 판단에 사용하지 않는다. 측정 PC는 정식 [공통 작업 기록](../../../../docs/production/P0_REFERENCE_RUN.md)에 고정했다.

이 재현은 참고 수업 A-G1-01/02, B-G1-01/02와 G1 통합을 해당 범위에서 Verified로 표시하는 근거다. 실제 개발자는 자기 출발점에서 직접 작성하고 별도 SHA·실행 증거를 남긴다. G2는 이 G1 통과 후 시작한다.

후속 추가 검수: G1 코드와 에셋의 Win64 Development 패키지158.70초 Pass, 실제 패키지2프로세스의7화면비·입력·2회전도 host1338/client1336 Pass. [패키지 결과](../../../../docs/production/evidence/RUN-20260918-G1/package-foundation.json)와 [실행 요약](../../../../docs/production/evidence/RUN-20260918-G1/package-pair-summary.json)을 함께 읽는다. 패키지 양쪽 세로 PNG에서 한글·재질·보드·경로 로딩을 확인했다. 위 재현 절차의 범위는 그대로이며 이 추가 증거를 G2/G3 전투·경제·10웨이브 패키지 통과로 확대하지 않는다.
