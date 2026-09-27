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
| 8 | [B-G3-01 진입과 반복 복귀](G3-01-entry-return.md) | G2 통합·A 결과 계약·Entry 맵 | Draft — 패키지 PKG01 수정·B 57자동화 Pass, 패키지 재검수 대기 |
| 9 | [B-G3-02 전투·결과 HUD](G3-02-battle-result-hud.md) | A 단일 BattleSnapshot·두 위젯 | Draft — 실제 PIE 양쪽 Running 확인, 최종 패키지 결과/복귀 대기 |
| 10 | [B-G3-03 최종 보상 확정 경계](G3-03-clock-finalization.md) | G2 명령/보상 큐·A 시간 진행 | Draft — 실제 서비스·Mode 경계 자동화 Pass, 패키지 검수 대기 |

공통 규칙 원본은 [공통 구현 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), 수업 제작·실제 학습 구분은 [학습 운영](../../WORKFLOW.md)이다. B 독립 G0 완료 소스는 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`, 공통 출발은 `8c6856d235de87cc28c12b49ca775bd0937334a5`다. [G0 별도 수업 재현](../evidence/G0_REPLAY/SUMMARY.md)에서 Editor/자동화4개 Pass를 확인했으나 G0 기반/명령 수업의 실제 PIE·소유 RPC·응답 유실 검수는 남아 있어 Draft다. 학습자의 `learn/p0-b`에는 참고 완성 코드를 병합하지 않는다.

G1은 두 화면 모두 오른쪽 `-WorldX`, 자기 보드 아래, 참가자1의 로컬 Y반사와 같은 입력 역변환을 실제로 확인했다. 서버 Actor root·RouteIndex·CellId는 바꾸지 않는다. 카메라 축·높이와 작은 화면 경계선 실패/수정은 G1-02에 기록했다.

G2 공통 시작점은 `4861b987f3e2fe78bcc159d1b6a85008543a938b`, 완료 소스는 `ae6be1b0b06ed733425e01632a341fb4db4cad59`다. [초기 계약 합의 메모](G2-contract-notes.md)는 구현 전 기록으로 보존한다. [공통 재현 절차](../evidence/G2_REPLAY/README.md)대로 새 detached `C:/Users/iam12/P0_lesson_replay_g2`에서 G1부터56파일을 조립했다. 최초 제품 소스5baa960에 검사 보완만 ae6be1b까지 적용했고, replay HEAD가 G1인 이유와 파일 blob 일치는 공통 manifest로 추적한다. Editor122.13초 Pass, 전체 자동화39무경고 Pass, 보완 Editor Pass·명령12무경고 Pass, 실제 두 GPU 프로세스20단계 host213/client57검사 Pass를 확인했다. [B 최종 선별 근거](evidence/G2-final-summary.json)와 [정식 G2 검수](../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md)가 최신 판정이며 초기 실패/경고 기록은 보존한다.

G2 Verified는 첫 소환·기본 공격·처치/보상·뭉치 이동/보충/합성/판매·서버 실패/중복·개인 상태 도착순서·실제 Slate/EngineTouch·HUD 재생성의 재현 범위다. 인위적 네트워크 지연/유실·최종 PC 패키지10웨이브·PIE·물리 입력·사운드 청취(`-nosound`)·Android는 미검증이다. 60FPS 제한의 작은 정지 적 픽스처 P95 약16.667ms는 대표 P0 부하 성능으로 사용하지 않는다. 세 learn 브랜치와 실제 학습자 상태는 변경하지 않는다.

G1 최종 소스는 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`다. canonical G0 `649c1dedd6832c41089a76b59bc76518cd262296`부터 새 detached worktree에서36파일을 수업 순서대로 조립해 Editor Pass88.39초, 전체 `LD.P0` 22Pass, 실제 두 프로세스7화면 host1338/client1336검사 Pass를 확인했다. [공통 재현 시작 문서](../evidence/G1_REPLAY/README.md) → [실제 결과와 증거](../evidence/G1_REPLAY/SUMMARY.md) 순서로 읽는다. PIE·최종 패키지·OS 입력/물리 터치·Android·G2 명령 RPC·반복 매치/UI 재생성은 이 Verified 판정 밖이다. 참고 재현은 학습자가 직접 구현했다는 기록이 아니며 실제 학습 상태를 올리지 않는다.

<a id="g3-evidence"></a>

## G3 검증 범위

G3 공통 시작은 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`, 현재 참고 구현 소스는 `0e473f4af380506d209a95f7ec42eccf89c69df4`다. 이 SHA는 제품·검사 코드의 현재 기준이며 G3 게이트 완료 SHA가 아니다. 세 수업 모두 **Draft**, 실제 학습자와 `learn/p0-b`는 **Planned**를 유지한다. [통합 수업의 조립·실행 순서](../G3_INTEGRATION.md), [공통 계약](../COMMON.md), [독립 구조 리뷰](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)를 먼저 읽고 B03→B01→B02 순으로 상대 코드와 조립한다.

| 증거 종류 | 실제 확인과 입력 기준 | 확인하지 않은 범위 |
|---|---|---|
| 새 수업 조립·파일 대조 | G2 detached `C:/Users/iam12/P0_lesson_replay_g3`에 최초65파일, 도구 보완 뒤66파일. [기존 최종 manifest](../evidence/G3_REPLAY/final-inputs.json)의 입력은 `5359cda`, 런타임은 `de6e2f6` | 파일 blob 일치는 실행 검수나 학습자 구현 완료가 아님 |
| 새 재현 Editor·NullRHI 자동화 | [Editor](../evidence/G3_REPLAY/editor.json) Pass, [56종 자동화](../evidence/G3_REPLAY/automation.json) 56Pass/0Warning/0Fail. 위 기존 조립 입력의 결과 | 이후 Entry 수정 `0e473f4`의 패키지 재검수로 쓰지 않음 |
| 실제 GPU PIE | [PIE 수명 기록](../evidence/G3_REPLAY/pie-proof.json): 두 네트워크 World, 양쪽 소환1/gold80, 준비→Running, 명시 Aborted 뒤 서비스 정리·World0·원설정 복원. [host](../evidence/G3_REPLAY/host-running.png)/[client](../evidence/G3_REPLAY/client-running.png) 546×720에서 WAVE1/20초/N2·자기 보드 아래 | Entry 여행·10웨이브·최종 패키지5판 증거가 아님 |
| 기존 G2 조작 회귀 | [새 재현본 G2 결과](../evidence/G3_REPLAY/g2-regression.json): 별도 Editor-game 두 프로세스 host213/client57 Pass | 고정 적 픽스처의 조작·소유 RPC·HUD 재생성 범위. 새 G3 Result/Entry 반복 복귀를 대체하지 않음 |
| 패키지 최초 Entry 실행 | [PKG01 원본 실패](../../../docs/production/evidence/RUN-20260918-G3/packaged-entry-crash.json): 실제 cooked client 프레임2 font/Slate prepass 접근 위반, pair Fail 보존 | 패키징 성공과 실행 성공은 다름. 미완성5시드를 성공 횟수로 세지 않음 |
| PKG01 수정 후 B 역할 검증 | `501be9035b02e172e356151abe0c1606304b11ce`의 owned WidgetStyle 수정이 `0e473f4`에 포함. B `Saved/P0Runs/G3-B-final-editor` Editor **72.19초 Pass**, `G3-B-final-automation` 전체 **57Pass/0Warning/0Fail**, 새 `OwnedAddressStyleSurvivesPrepass` 포함 | NullRHI 자동화이므로 실제 cooked 렌더링·물리 입력 통과가 아님 |
| 수정 입력의 새 재현본 반영 | 재현 폴더 `Saved/P0Runs/Replay-G3-assembly/entry-style-amendment.json`: 이전 입력 `5359cda` → `0e473f4`, Entry3파일과 읽기 전용 분석기1파일 SHA256 기록. 에셋/설정/데이터 변경 없음 | 기존66파일 manifest만으로 새 수정 실행을 주장하지 않음. 수정 패키지 재실행 대기 |
| 최종 패키지·성능·기기 | 수정 후 패키지5시드·반복 재매치·600초 지연/손실/회복, 대표 부하20분, Android 실기기는 **NotRun** | 10초 Editor-game 부하 smoke나 이전 게이트 통과로 대체하지 않음 |

B 역할 결과 원본은 `C:/Users/iam12/P0_reference_b/Saved/P0Runs/G3-B-final-editor/{result.json,build.log}`와 `G3-B-final-automation/{result.json,report/index.json,engine.log}`다. 새 재현본 전체 로그는 `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/Replay-G3-*`에 보존한다. 재현 worktree HEAD는 시작 `f735b588`에 유지되므로 실행 JSON의 HEAD와 최종 입력 manifest·추가 수정 기록을 함께 읽는다. 최초 패키지 충돌 로그는 덮어쓰지 않으며 수정 후 결과는 새 RunId로 남긴다.

다음 재현은 `learning/tools/Replay-P0G3.ps1`에 존재하지 않는 별도 `-ReplayRoot`, 위 전체 `-SourceSha 0e473f4af380506d209a95f7ec42eccf89c69df4`, 새 RunId를 전달한다. 역할 파일은 후속 묶음을 참조하므로 전체 조립 뒤 빌드한다. Editor→자동화→PIE→새 패키지→반복 두 프로세스 순서와 도구 인수는 통합 수업을 따른다. 최종 패키지와 필수 수명·네트워크·부하 검수까지 재현한 뒤에만 수업별 Verified 범위를 결정한다.
