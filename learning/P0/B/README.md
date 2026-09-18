# P0 B 참고 구현 학습 순서

참고 제작은 수업별 범위로 판정하며 **G1 두 수업·G2 세 수업 Verified**, 실제 학습자는 **Planned**다. 이 폴더는 2026-09-18 새 출발점의 참고 제작 기록이며 이전 P0 실행 증거를 재사용하지 않는다.

| 순서 | 수업 | 진입 조건 | 상태 |
|---|---|---|---|
| 1 | [B-G0-01 독립 공통 기반](G0-01-foundation.md) | 공통 출발 커밋·Schema2/Rules0.3.0 자료 확보 | Draft |
| 2 | [B-G0-02 명령 입구와 중복 방지](G0-02-commands.md) | 첫 수업 타입·로더·매치 문맥 | Draft |
| 3 | [B-G1-01 좌표와 로컬 변환](G1-01-geometry.md) | 통합 G0 Editor 빌드와 리뷰 | Verified — 조립·좌표·실제 G1 두 화면 |
| 4 | [B-G1-02 카메라와 전체 셀 입력](G1-02-view-input.md) | G1 좌표·A 경로 계약·공통 표시 재료 | Verified — 조립·카메라·7화면/전체셀/EngineTouch |
| 5 | [B-G2-01 보드와 경제 공동 확정](G2-01-board-economy.md) | G1 통과·A 준비 UnitActor·공통 배치/처치 값 | Verified — 별도 조립·실제 서비스/Actor·전투 연결 |
| 6 | [B-G2-02 중복 요청과 연결 세대](G2-02-command-lifetime.md) | 두 원본 서비스·요청 캐시 기반 | Verified — 세대/만료/응답 순서·실제 소유 RPC |
| 7 | [B-G2-03 HUD와 뭉치 조작](G2-03-hud-input.md) | 개인 Snapshot·실제 A UnitActor | Verified — 실제 GPU/Slate·EngineTouch·HUD 재생성 |

공통 규칙 원본은 [공통 구현 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), 수업 제작·실제 학습 구분은 [학습 운영](../../WORKFLOW.md)이다. B 독립 G0 완료 소스는 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`, 공통 출발은 `8c6856d235de87cc28c12b49ca775bd0937334a5`다. [G0 별도 수업 재현](../evidence/G0_REPLAY/SUMMARY.md)에서 Editor/자동화4개 Pass를 확인했으나 G0 기반/명령 수업의 실제 PIE·소유 RPC·응답 유실 검수는 남아 있어 Draft다. 학습자의 `learn/p0-b`에는 참고 완성 코드를 병합하지 않는다.

G1은 두 화면 모두 오른쪽 `-WorldX`, 자기 보드 아래, 참가자1의 로컬 Y반사와 같은 입력 역변환을 실제로 확인했다. 서버 Actor root·RouteIndex·CellId는 바꾸지 않는다. 카메라 축·높이와 작은 화면 경계선 실패/수정은 G1-02에 기록했다.

G2 공통 시작점은 `4861b987f3e2fe78bcc159d1b6a85008543a938b`, 완료 소스는 `ae6be1b0b06ed733425e01632a341fb4db4cad59`다. [초기 계약 합의 메모](G2-contract-notes.md)는 구현 전 기록으로 보존한다. [공통 재현 절차](../evidence/G2_REPLAY/README.md)대로 새 detached `C:/Users/iam12/P0_lesson_replay_g2`에서 G1부터56파일을 조립했다. 최초 제품 소스5baa960에 검사 보완만 ae6be1b까지 적용했고, replay HEAD가 G1인 이유와 파일 blob 일치는 공통 manifest로 추적한다. Editor122.13초 Pass, 전체 자동화39무경고 Pass, 보완 Editor Pass·명령12무경고 Pass, 실제 두 GPU 프로세스20단계 host213/client57검사 Pass를 확인했다. [B 최종 선별 근거](evidence/G2-final-summary.json)와 [정식 G2 검수](../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md)가 최신 판정이며 초기 실패/경고 기록은 보존한다.

G2 Verified는 첫 소환·기본 공격·처치/보상·뭉치 이동/보충/합성/판매·서버 실패/중복·개인 상태 도착순서·실제 Slate/EngineTouch·HUD 재생성의 재현 범위다. 인위적 네트워크 지연/유실·최종 PC 패키지10웨이브·PIE·물리 입력·사운드 청취(`-nosound`)·Android는 미검증이다. 60FPS 제한의 작은 정지 적 픽스처 P95 약16.667ms는 대표 P0 부하 성능으로 사용하지 않는다. 세 learn 브랜치와 실제 학습자 상태는 변경하지 않는다.

G1 최종 소스는 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`다. canonical G0 `649c1dedd6832c41089a76b59bc76518cd262296`부터 새 detached worktree에서36파일을 수업 순서대로 조립해 Editor Pass88.39초, 전체 `LD.P0` 22Pass, 실제 두 프로세스7화면 host1338/client1336검사 Pass를 확인했다. [공통 재현 시작 문서](../evidence/G1_REPLAY/README.md) → [실제 결과와 증거](../evidence/G1_REPLAY/SUMMARY.md) 순서로 읽는다. PIE·최종 패키지·OS 입력/물리 터치·Android·G2 명령 RPC·반복 매치/UI 재생성은 이 Verified 판정 밖이다. 참고 재현은 학습자가 직접 구현했다는 기록이 아니며 실제 학습 상태를 올리지 않는다.
