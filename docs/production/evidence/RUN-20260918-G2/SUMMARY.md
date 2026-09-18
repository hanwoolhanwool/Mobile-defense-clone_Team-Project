# G2 전투·경제 참고 구현 검수

상태: **Pass (G2 범위)**. 새 수업 출발점 재현·최종 GPU 두 프로세스·독립 리뷰를 완료했다. P0 전체는 G3/G4 검수 전 InProgress다. 공통 시작은 `4861b987f3e2fe78bcc159d1b6a85008543a938b`다.

## 실행 범위별 결과

| 범위 | 실제 결과 | SHA·증거 |
|---|---|---|
| A 역할 Editor | Pass, 약27초 | ca5b672; A Saved/P0Runs/G2-A-editor |
| A 전투·조립 자동화 | 10Pass(9무경고+1WorldContext 경고),0Fail | A Saved/P0Runs/G2-A-combat. 16종 실제 공격·피해·시각·동일 프레임 판매 포함 |
| B 역할 Editor | Pass,25.06초 | 4a69fe9; B Saved/P0Runs/G2-B-editor |
| B 보드·경제·Controller 자동화 | 7Pass(3무경고+4경고),0Fail | B Saved/P0Runs/G2-B-commands. WorldContext3경고와 역할 폴더 미수신 사운드1경고; 원본 보존 후 원인 수정 |
| 통합 Editor | Pass | 7aebcf8; Saved/P0Runs/G2-final-candidate-editor |
| 통합 NullRHI 자동화 | 39Pass(38무경고+1A fixture 경고),0Fail/0NotRun | Saved/P0Runs/G2-final-candidate-automation. 경고 수정과 새 재현 실행 대기 |
| 첫 실제 GPU 두 프로세스 | 17단계의 당시 검사 Pass; 후속 리뷰에서 검사 누락 확인 | [당시 보장 범위](pair-initial-scope.json). 최종 HUD·응답·추가 Actor 보장으로 사용하지 않음 |
| 강화 실제 GPU 두 프로세스 | Fail, 검사기 Conflict 입력이 구조적으로 잘못됨 | [실패 원인](pair-ui-fix2-failure.json); HUD 포함 캡처는 실제 확인. 유효 Revision 차이로 수정 |
| 수업 새 G1 출발점 재현 | 56파일 blob일치·Editor122.13초·39무경고Pass | 제품5baa960, 새 detached G1; learn 세 브랜치8c6856d 유지 |
| 독립 리뷰 보충 | Editor·12무경고Pass,0Fail/0NotRun | 최종ae6be1b0b06ed733425e01632a341fb4db4cad59; 제품diff0, 검사기3파일만 변경. 원래39개와7중복, 총44종 |
| 최종 실제 GPU 두 프로세스 | 20단계 host213/client57 Pass | [최종 실행](review-pair.json)·[검사/PNG 해시](review-pair-summary.json); 실제 Slate 버튼·EngineTouch 드래그·HUD 재생성 |
| 최종 PC 패키지·지연·10웨이브 | NotRun | G3 범위. 앞선 G1 패키지 증거를 재사용하지 않음 |
| PIE·Android 실기기 | NotRun | 실제 Editor-game 두 프로세스와 다른 범위 |

## 실패와 수정

- 초기 공용 UnitActor 병합 충돌 상태에서 shell이 빌드를 계속해 컴파일 실패했다. 양쪽 동일 blob을 확인해 선택했고 이후 모든 종속 Git 작업은 종료 코드 검사 뒤 빌드한다. Saved/P0Runs/G2-services-editor-initial과fix1을 보존했다.
- 새 타깃에 과거 공격 시각을 적용하던 오류, 예정 시각 대신 프레임 끝 위치로 사거리를 검사하던 오류를 수정했다. 실제16종 공격·거리·시각 회귀를 포함한다.
- 게시 중 epoch 변경으로 새 세대 캐시를 오염시키던 오류는 원문맥 값 복사·콜백 후 재조회·게시 전 원응답 저장으로 해결했다. 실제 Board delegate가 epoch를 교체하는 회귀를 통과했다.
- 이전 예정 타격/보상을 건너뛴 즉시 명령과 같은 WorldTime의 늦은 입력을 분리해 수정했다. 현재 WorldTime은 열어 두고 `<Now` 격자만 확정한다. 실제 엔진 호출 순서 근거와 Mode 타이머 본문/PC 판매 회귀는 [독립 리뷰](REVIEW_FINDINGS.md)의 I-01과 Tests/LDLifecycleTests.cpp에 있다.
- UMG 부모 Visibility를 숨긴 지역 변수 때문에 C4458 컴파일 실패했고 이름을 변경했다. 현재 단일 epoch envelope·만료 Revision 대기·HUD 재생성과 동일 요청 재확인을 연결했다.
- 검사기의 HUD 제외 캡처, 미응답 단계 건너뛰기, 추가 committed Actor 누락, 잘못된 Conflict payload를 수정했다. 완료 요청의 클라이언트 불필요 응답은 정책상 무시하므로 서버 API 원응답 검사를 실제 네트워크 응답 관찰로 표시하지 않는다.
- 자동화 요약에서 succeededWithWarnings를 누락한 도구 결함을 수정했다. 원본 로그/옛 result.json은 보존하고 [정확한 성공·경고 총계](automation-count-audit.json)를 별도로 기록했다. 첫34검사는33Pass/1Fail, 후속36·37검사는 모두Pass다.

## 구조와 한계

[독립 기대값24개](REVIEW_PLAN.md)와 [파일·함수·영향·수정 리뷰](REVIEW_FINDINGS.md)가 검수 기준이다. BoardManager는 개체·칸·잠금, EconomyService는 개인 재화·RNG, CombatService는 공격 시각, EnemyActor는 HP·사망을 소유한다. GameMode가 주입·게시·종료 순서를 연결하며 HUD는 복제된 값만 읽는다. 필수 런타임·에셋·데이터는 learning 폴더를 참조하지 않는다.

GPU 픽스처는 seed1776, 정지 HP70 적1마리와 HP1 구매 자금용 적100마리를 사용한다. 명령·공격·처치·보상은 제품 코드를 그대로 실행하지만 웨이브/밸런스 검수는 아니다. 프레임 수치는60FPS 제한·작은 부하의 관찰값이며 대표 부하 성능은 G3에서 측정한다. 호스트 환경은 [공통 실행 기록](../../P0_REFERENCE_RUN.md#측정-환경과-한계)에 고정한다.

최종 [ARCH-01~06 및 수명·권한 리뷰](FINAL_REVIEW.md)는 차단 결함0으로 G2를 승인한다. 준비 Actor 검사의 잘못된 전체 플래그 가정과 Touch 인수 컴파일 실패도 실제 실패로 보존하고 수정 후 통과했다. 제품 런타임 완료5baa960, 검사기 포함 재현 완료ae6be1b다.
