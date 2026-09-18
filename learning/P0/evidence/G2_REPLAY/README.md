# G2 수업 재현 증거

실제 학습자는 Planned다. 이 폴더는 참고 소스를 수업 순서로 조립하고 실행한 제작 검수다. G1 `4861b987f3e2fe78bcc159d1b6a85008543a938b`에서 새 detached `C:/Users/iam12/P0_lesson_replay_g2`를 만들고 최종 런타임 참고 소스 `5baa96059e94a142d45206290010b373cf39ea19`의 명시56파일을 조립했다.

- [파일·단계·제공 구분 manifest](assembly.json): 모두 원본 Git blob 일치. HEAD는 G1이고 세 learn 브랜치는 공통 출발점8c6856d 유지.
- [Editor 빌드](editor.json): Pass,122.13초. 새 경로의 전체 컴파일이며 이전 DLL 복사가 아니다.
- [Unreal 자동화](automation.json): 39Pass,0경고,0Fail,0NotRun. NullRHI 실행으로 화면 검수를 대신하지 않는다.
- [실제 별도2프로세스 요약](../../../../docs/production/evidence/RUN-20260918-G2/replay-pair-summary.json): host212/client57 검사,0Fail.20단계의 실제 Slate 버튼·Controller RPC·복제·공격/처치·보상·보충/이동·합성·실패/재전송·HUD 재생성을 관찰했다.
- 직접 확인한 화면: [host 최종](../../../../docs/production/evidence/RUN-20260918-G2/host-final.png), [client 최종](../../../../docs/production/evidence/RUN-20260918-G2/client-final.png), [골드 부족 거절](../../../../docs/production/evidence/RUN-20260918-G2/host-rejected.png).

호스트 최종 골드2·인구5·다음36, 클라이언트159·인구2·다음24를 관찰했다. 두 화면의 위/아래 보드 배치는 서로 반사되며 canonical 개체·셀은 같았다. 거절 화면은 골드9·인구4·다음28 그대로이고 소환 버튼이 붉게 변하며 “골드가 부족합니다”를 표시했다. 양쪽 HUD를 실제 RemoveFromParent한 후 새 위젯의 버튼을 눌렀을 때 각각 새 명령1회만 발생했다.

전체 로그와 PNG9개는 재현 폴더 `Saved/P0Runs/Replay-G2-*`에 보존된다. engine.log의 개발용 ToolsetRegistry Python 초기화 오류는 Editor의 `-game` 실행에서 발생한 엔진 플러그인 메시지이며 자동화/프로젝트 코드 실패와 구분한다. 최종 PC 패키지는 G3에서 별도로 실행한다.

60FPS 제한에서 P95 양쪽16.667ms다. 적 HP70 한 마리와 구매 자금용 HP1 적100마리(동시최대12)를 둔 명시적 픽스처이며 10웨이브나 대표 부하 측정이 아니다. 실제 하드웨어 터치·물리 마우스·사운드 청취(`-nosound`)·PIE·패키지·Android는 이 실행의 검증 범위가 아니다. 환경·실패 분석·추가 검증 상태는 [정식 G2 검수](../../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md)에서 관리한다.

## 독립 리뷰 보충 완료

최종 참고 소스는 `ae6be1b0b06ed733425e01632a341fb4db4cad59`다. [첫 보충56파일 manifest](review-assembly.json)와 [FTouchId 수정 후 최종56파일 manifest](review-fix1-assembly.json)는 원래 조립파일의 hash를 확인하고 변경 전 사본을 보존했다. Source/Config/Content/tools에서 제품 변경은 없고 Tests1개·Verification2개만 추가/수정했다.

- [최종 보충 Editor](review-editor.json) Pass. 첫 보충 빌드의 C2665는 EngineTouch 인수를 FTouchId로 수정해 재검증했다.
- [보충12검사](review-commands.json) 무경고Pass. 원래39개와7개 중복되어 전체 증거는44종이며 한 번의 전체44검사가 아니다. 준비 격리 검사의 Actor 플래그 가정 실패는 PrimitiveComponent의 실제 가시성·NoCollision·overlapfalse로 수정했다.
- [최종 실제 두 프로세스](../../../../docs/production/evidence/RUN-20260918-G2/review-pair-summary.json): host213/client57 Pass. Stage12는 실제 EngineTouch Began/Moved/Ended→소유 Controller 요청→기존 개체/타이머 유지까지 검증했다.
- 직접 열람한 최종 화면: [host](../../../../docs/production/evidence/RUN-20260918-G2/review-host-final.png), [client](../../../../docs/production/evidence/RUN-20260918-G2/review-client-final.png), [거절](../../../../docs/production/evidence/RUN-20260918-G2/review-host-rejected.png). 앞선 세 이미지/JSON도 당시 증거로 보존한다.

위 실행의 시작점·조립·빌드·검사를 재현했으므로 G2의 A3개/B3개/통합 수업을 해당 범위에서 Verified로 표시한다. 학습자는 Planned이며 사운드 청취·PIE·최종 패키지·실기기 및10웨이브는 여전히 별도 검수다.
