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
| 8 | [B-G3-01 진입과 반복 복귀](G3-01-entry-return.md) | G2 통합·A 결과 계약·Entry 맵 | Draft — 실제 패키지5판·같은 프로세스4회 복귀 Pass, PKG01 닫힘 |
| 9 | [B-G3-02 전투·결과 HUD](G3-02-battle-result-hud.md) | A 단일 BattleSnapshot·두 위젯 | Draft — 패키지 양쪽 결과·게임 HUD 재생성 Pass, 필수 부하 진행 중 |
| 10 | [B-G3-03 최종 보상 확정 경계](G3-03-clock-finalization.md) | G2 명령/보상 큐·A 시간 진행 | Draft — 서비스/Mode 경계·패키지 중복 RPC/회복 Pass, 정확한 경계 범위 구분 |

공통 규칙 원본은 [공통 구현 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), 수업 제작·실제 학습 구분은 [학습 운영](../../WORKFLOW.md)이다. B 독립 G0 완료 소스는 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`, 공통 출발은 `8c6856d235de87cc28c12b49ca775bd0937334a5`다. [G0 별도 수업 재현](../evidence/G0_REPLAY/SUMMARY.md)에서 Editor/자동화4개 Pass를 확인했으나 G0 기반/명령 수업의 실제 PIE·소유 RPC·응답 유실 검수는 남아 있어 Draft다. 학습자의 `learn/p0-b`에는 참고 완성 코드를 병합하지 않는다.

G1은 두 화면 모두 오른쪽 `-WorldX`, 자기 보드 아래, 참가자1의 로컬 Y반사와 같은 입력 역변환을 실제로 확인했다. 서버 Actor root·RouteIndex·CellId는 바꾸지 않는다. 카메라 축·높이와 작은 화면 경계선 실패/수정은 G1-02에 기록했다.

G2 공통 시작점은 `4861b987f3e2fe78bcc159d1b6a85008543a938b`, 완료 소스는 `ae6be1b0b06ed733425e01632a341fb4db4cad59`다. [초기 계약 합의 메모](G2-contract-notes.md)는 구현 전 기록으로 보존한다. [공통 재현 절차](../evidence/G2_REPLAY/README.md)대로 새 detached `C:/Users/iam12/P0_lesson_replay_g2`에서 G1부터56파일을 조립했다. 최초 제품 소스5baa960에 검사 보완만 ae6be1b까지 적용했고, replay HEAD가 G1인 이유와 파일 blob 일치는 공통 manifest로 추적한다. Editor122.13초 Pass, 전체 자동화39무경고 Pass, 보완 Editor Pass·명령12무경고 Pass, 실제 두 GPU 프로세스20단계 host213/client57검사 Pass를 확인했다. [B 최종 선별 근거](evidence/G2-final-summary.json)와 [정식 G2 검수](../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md)가 최신 판정이며 초기 실패/경고 기록은 보존한다.

G2 Verified는 첫 소환·기본 공격·처치/보상·뭉치 이동/보충/합성/판매·서버 실패/중복·개인 상태 도착순서·실제 Slate/EngineTouch·HUD 재생성의 재현 범위다. 인위적 네트워크 지연/유실·최종 PC 패키지10웨이브·PIE·물리 입력·사운드 청취(`-nosound`)·Android는 미검증이다. 60FPS 제한의 작은 정지 적 픽스처 P95 약16.667ms는 대표 P0 부하 성능으로 사용하지 않는다. 세 learn 브랜치와 실제 학습자 상태는 변경하지 않는다.

G1 최종 소스는 `df8a2f27dd962a4d9f9f4051e51f3332245ba40a`다. canonical G0 `649c1dedd6832c41089a76b59bc76518cd262296`부터 새 detached worktree에서36파일을 수업 순서대로 조립해 Editor Pass88.39초, 전체 `LD.P0` 22Pass, 실제 두 프로세스7화면 host1338/client1336검사 Pass를 확인했다. [공통 재현 시작 문서](../evidence/G1_REPLAY/README.md) → [실제 결과와 증거](../evidence/G1_REPLAY/SUMMARY.md) 순서로 읽는다. PIE·최종 패키지·OS 입력/물리 터치·Android·G2 명령 RPC·반복 매치/UI 재생성은 이 Verified 판정 밖이다. 참고 재현은 학습자가 직접 구현했다는 기록이 아니며 실제 학습 상태를 올리지 않는다.

<a id="g3-evidence"></a>

## G3 검증 범위

G3 공통 시작은 `f735b5889a5bd197e46d29bdfaa2b38c246d5ea6`다. [최종66파일 대조](../../../docs/production/evidence/RUN-20260918-G3/audited-final-inputs.json)의 조립 SourceSha는 `0981d07307112857dfdf0e91c79bcecdbcbc291b`, 런타임 C++는 `0e473f4af380506d209a95f7ec42eccf89c69df4`, 쿠킹 Config는 `98727f04e7c563a854a103ad26152cff5ed652a6`다. 패키징 뒤 변경은 읽기 전용 `Test-P0G3Evidence.ps1`·`Analyze-P0Load.py` 두 도구뿐이며 런타임/데이터/에셋/설정은 그대로다. G3 게이트 완료 SHA는 아직 없다. 대표 부하20분 검수가 진행 중이므로 세 수업 모두 **Draft**, 실제 학습자와 `learn/p0-b`는 **Planned**를 유지한다.

[정식 G3 결과](../../../docs/production/evidence/RUN-20260918-G3/SUMMARY.md)를 공통 실행 원본으로 사용한다. [통합 수업의 조립·실행 순서](../G3_INTEGRATION.md), [공통 계약](../COMMON.md), [독립 구조 리뷰](../../../docs/production/evidence/RUN-20260918-G3/REVIEW_FINDINGS.md)를 먼저 읽고 B03→B01→B02 순으로 상대 코드와 조립한다. 아래 수치는 2026-09-27의 새 실행이며 과거 P0 완료 기록을 재사용하지 않는다.

| 증거 종류 | 실제 확인과 입력 기준 | 확인하지 않은 범위 |
|---|---|---|
| 새 수업 조립·파일 대조 | G2 detached `C:/Users/iam12/P0_lesson_replay_g3`에 최초65파일→66파일. [이전 manifest](../evidence/G3_REPLAY/final-inputs.json)는 입력 `5359cda`/런타임 `de6e2f6`; 이후 Entry·쿠킹·읽기 도구 수정을 위 최종66파일 manifest로 추적 | 파일 blob 일치는 실행 검수나 학습자 구현 완료가 아님 |
| 새 재현 Editor·NullRHI 자동화 | [기존 Editor](../evidence/G3_REPLAY/editor.json)와 [56종 자동화](../evidence/G3_REPLAY/automation.json) 56Pass/0Warning/0Fail. Entry 수정 반영 후 [Editor](../evidence/G3_REPLAY/entry-fixed-editor.json)와 [Entry3종](../evidence/G3_REPLAY/entry-fixed-automation.json) 3Pass/0Warning/0Fail | 새 재현본56종+후속 Entry3종이다. 59개의 서로 다른 검사나 수정 후 전체57 재실행으로 합산하지 않음 |
| 실제 GPU PIE | [PIE 수명 기록](../evidence/G3_REPLAY/pie-proof.json): 두 네트워크 World, 양쪽 소환1/gold80, 준비→Running, 명시 Aborted 뒤 서비스 정리·World0·원설정 복원. [host](../evidence/G3_REPLAY/host-running.png)/[client](../evidence/G3_REPLAY/client-running.png) 546×720에서 WAVE1/20초/N2·자기 보드 아래 | Entry 여행·10웨이브·최종 패키지5판 증거가 아님 |
| 기존 G2 조작 회귀 | [새 재현본 G2 결과](../evidence/G3_REPLAY/g2-regression.json): 별도 Editor-game 두 프로세스 host213/client57 Pass | 고정 적 픽스처의 조작·소유 RPC·HUD 재생성 범위. 새 G3 Result/Entry 반복 복귀를 대체하지 않음 |
| PKG01 실패→수정→실행 | [원본 Fail](../../../docs/production/evidence/RUN-20260918-G3/packaged-entry-crash.json)은 cooked client 프레임2 font/Slate 접근 위반. `501be90`의 owned WidgetStyle 수정 후 최종 패키지 Entry 진입·같은 프로세스4회 결과 복귀/5개 매치 통과로 **Closed** | 최초 Fail은 보존하며 수정 전 중단한 판을 성공 횟수로 세지 않음 |
| 수정 후 통합/B 역할 자동화 | `0e473f4` 통합과 B 역할 전체 각각 **57Pass/0Warning/0Fail**. B [Editor](../../../docs/production/evidence/RUN-20260918-G3/role-b-final-editor.json) **72.19초 Pass**, [57종](../../../docs/production/evidence/RUN-20260918-G3/role-b-final-57.json)에 새 `OwnedAddressStyleSurvivesPrepass` 포함 | NullRHI 자동화와 위 새 재현본의56+Entry3 실행을 구별함 |
| PKG02 실패→쿠킹 수정→로드 | [원본 Fail](../../../docs/production/evidence/RUN-20260918-G3/packaged-audio-missing.json)은 필수 거절 효과음 누락. `98727f0`에서 `/Game/LD/Audio`를 명시 cook한 뒤 새 패키지 정상 거절 경로의 실제 에셋 로드 Pass로 **쿠킹/로딩 Closed** | `-nosound`이므로 소리 청취는 NotRun. C++/게임 규칙을 변경한 수정이 아님 |
| 최종 Win64 패키지5시드 | [반복 플레이](../../../docs/production/evidence/RUN-20260918-G3/package-five-seeds-summary.json) **host82/client72**, [실제 RPC 로그](../../../docs/production/evidence/RUN-20260918-G3/package-five-seeds-wire.json) **62검사 Pass**. 같은 두 프로세스4회 복귀/5매치, 각 판 게임 HUD3회 재생성, 최종 전투/보드/재화 일치. 요청 RTT150ms·방향별 손실1%, 연결 시간 약1250/1252초 | 실제 Slate 버튼·Controller 의도 API 자동 플레이다. 사람/학습자의 조작·승리율 또는 정확한 보스 마감 시각 타격의 패키지 검증은 아님 |
| 지연·손실 회복 | [회복 실행](../../../docs/production/evidence/RUN-20260918-G3/package-recovery-summary.json) **host19/client17 Pass**, 실제 RPC 로그 Pass. 요청 RTT300ms·방향별 손실3%→연결120초에 해제, 한 판 약250초·client Pending 재시도17회·최종 상태 일치 | echo P95 약349.6ms는 회복 전/후를 합친244표본이다. 회복 전후 각 구간 지표나 손실률로 해석하지 않음 |
| 대표 부하·기기 | 최종 패키지 대표 부하20분 **InProgress**, Android 실기기 **NotRun** | 짧은 Editor-game 부하 smoke로20분을 대체하지 않으며 현재 성능·장기 수명 통과를 선언하지 않음 |

B 역할 결과 원본은 `C:/Users/iam12/P0_reference_b/Saved/P0Runs/G3-B-final-editor/{result.json,build.log}`와 `G3-B-final-automation/{result.json,report/index.json,engine.log}`다. 새 재현본 전체 로그는 `C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/Replay-G3-*`, 최종 패키지5판은 `Replay-G3-package-five-seeds-fix2`, 회복은 `Replay-G3-package-recovery`에 보존한다. 실행 파일은 `Replay-G3-package-audio-fixed/Package/Windows/Mobile_defense_clone/Binaries/Win64/Mobile_defense_clone.exe`이며 두 실행의 동일 SHA256은 정식 결과에 있다. 재현 worktree HEAD는 시작 `f735b588`에 유지되므로 실행 JSON의 HEAD와 최종 입력 manifest·추가 수정 기록을 함께 읽는다. 최초 실패 로그와 pair Fail은 그대로 남긴다.

실제 패키지540×1170 PNG의 [host Wave10](../../../docs/production/evidence/RUN-20260918-G3/package-host-wave10.png)/[client Wave10](../../../docs/production/evidence/RUN-20260918-G3/package-client-wave10.png)은 양쪽 자기 보드가 아래이며 보스 HP5832/5895가 같다. [host Result](../../../docs/production/evidence/RUN-20260918-G3/package-host-result.png)/[client Result](../../../docs/production/evidence/RUN-20260918-G3/package-client-result.png)는 HP435/3975·일반 적0·패배/보스 제한 시간 초과·시작 화면 복귀 버튼·하단 종료 안내가 일치한다. 소환/합성/판매는 비활성 표시다. 다섯 시드 모두 정상 규칙의 BossTimeout 패배였으며 자동 전략의 기록을 사람의 밸런스 평가나 자연 승리 증거로 쓰지 않는다. 결과 위젯 자체 재생성·빠른 이중 복귀·종료 뒤 실제 터치/드래그와 Android는 별도 미검증이다.

다음 재현은 `learning/tools/Replay-P0G3.ps1`에 존재하지 않는 별도 `-ReplayRoot`, 위 전체 `-SourceSha 0981d07307112857dfdf0e91c79bcecdbcbc291b`, 새 RunId를 전달한다. 역할 파일은 후속 묶음을 참조하므로 전체 조립 뒤 빌드한다. Editor→자동화→PIE→새 패키지→반복 두 프로세스 순서와 도구 인수는 통합 수업을 따른다. 필수 수명·부하와 수업별 남은 UI 검수를 확인한 뒤 Verified 범위를 결정하며 Android는 G4에서 별도로 판정한다.
