# G2 A 실행 증거

현재 A G2 세 수업은 **Verified — 참고 파일 조립·Editor·자동화·명시적 G2 GPU 두 프로세스 재현 범위**, 실제 학습자는 **Planned**다. 공통 절차와 원본 연결은 [재현 증거](../evidence/G2_REPLAY/README.md), 전체 검수·한계는 [정식 G2 검수](../../../docs/production/evidence/RUN-20260918-G2/SUMMARY.md)에서 관리한다. 아래 초기 역할 결과와 최종 새 경로 재현은 서로 다른 실행이다.

| 실행 | HEAD·결과 | 증거 |
|---|---|---|
| 최초 통합 Editor | `dba8afa521c68dc48cc07207d22b7f72c06c4283`, Pass/Exit0, 2026-09-18 15:14:39~15:14:58 KST | `C:/Users/iam12/P0_reference_integration/Saved/P0Runs/G2-composed-editor/result.json`, 전체 `build.log` |
| 최초 전체 NullRHI 자동화 | 같은 HEAD, 전체30Pass/1Fail/0NotRun | `C:/Users/iam12/P0_reference_integration/Saved/P0Runs/G2-composed-automation-initial/result.json`, `Report/index.json`, 전체 `engine.log` |
| A 전투 부분집합 | `LD.P0.G2.Combat.*` 7Success/0Fail | 위 `Report/index.json`의 fullTestPath/state를 직접 확인 |
| 실패 분석 | 유일 실패 `G0.Integration.LoginReadinessOrders`: 이전 Stub 문구와 새 서비스 준비 문구 차이 | 실제 Phase/참가자 수는 정상. `eafc378`에서 문구 전체 일치 대신2참가자·Preparing 의미를 검사하도록 수정 |
| A 역할 후속 Editor | `ca5b67238987ebeb227f0be85154b5a7d4bdf7c6`, Pass/Exit0, 2026-09-18 15:42:57~15:43:24 KST | `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G2-A-editor/result.json`, 전체 `build.log` |
| A 역할 G2 NullRHI 자동화 | 같은 HEAD,10Pass(9무경고/1경고)/0Fail/0NotRun | `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G2-A-combat/result.json`, `report/index.json`, 전체 `engine.log` |
| 최종 새 경로 조립 | 시작 `4861b987f3e2fe78bcc159d1b6a85008543a938b`, 최초 소스 `5baa96059e94a142d45206290010b373cf39ea19`→완료 소스 `ae6be1b0b06ed733425e01632a341fb4db4cad59`;56파일 모두 blob 일치 | `C:/Users/iam12/P0_lesson_replay_g2/Saved/P0Runs/Replay-G2-assembly/assembly.json`, `Replay-G2-review-fix1-assembly/assembly.json` 직접 확인 |
| 재현 Editor·전체 자동화 | Editor122.13초 Pass, 전체39Pass/0경고/0Fail/0NotRun | 같은 재현 폴더 `Replay-G2-editor/result.json`, `Replay-G2-automation/result.json` 및 report의 A10개 무경고 Success 직접 확인 |
| 후속 검사기 보완 | review-fix1 Editor Pass, review-commands12Pass/0경고/0Fail/0NotRun | `Replay-G2-review-fix1-editor/result.json`, `Replay-G2-review-commands/result.json` 직접 확인 |
| 최종 실제 두 프로세스 |20단계,host213/client57 검사 모두 Pass, 실제 EngineTouch 이동 포함 | `Replay-G2-review-pair/pair.json`, `host/result.json`, `client/result.json` 직접 확인 |

7개 Pass는 AllSixteenBasicAttacks, DamageAndTargetBoundaries, DuplicateDamageAndSingleDeath, ExactDuePositionAndNoBackdating, InitialCadenceMoveAndReplenishment, SharedTargetRemovalAndStop, UnitPresentationPreservesCanonical이다. 고정 데이터16종 공격·정확 예정 사거리·타이머·단일 사망·종료·표시 파생값을 실제 Unreal 코드로 확인했으며 정지 적/직접 시각 주입 transient fixture다.

그 후 독립 조립 리뷰가 비정렬 RPC 명령 시각에 더 이른 공격/보상이 뒤로 밀리는 결함을 발견했다. `eafc378163f3d9ed939821d153228f9b9d29943e`는 전투 strict-before hook과 전역 예정 순서를 수정하고 CommandClockOrdersEarlierHitAndSameTimeSale/EarlierKillFundsExternalPurchase를 추가했다. 후속 A 역할 실행에서 **전투9개 모두 무경고 Pass**를 직접 확인했다. 이전7개 결과를 재사용하지 않고 새 실행 HEAD와 결과를 연결했다.

최종 재현은 실제 Slate 버튼·Controller RPC·복제·첫 처치/양쪽 보상·보충/이동/합성·실패/재전송·HUD 제거/재생성을 포함한다. 정지 HP70 적과 구매 자금용 HP1 적은 검증 전용이며10웨이브나 밸런스 성공을 뜻하지 않는다. 미검증은 PIE·최종 PC 패키지·지연망·반복10웨이브 매치·대표 부하·물리 마우스/터치·사운드 청취·Android다. G1 패키지 결과를 G2 성공으로 대체하지 않는다. 재현 HEAD는 G1에 유지되므로 완료 소스는 HEAD 대신56파일 manifest로 식별한다.

현재 WorldTime이 타이머 뒤 입력과 공유될 수 있다는 후속 엔진 소스 검토는 [연결 수업의 재현·기대·한계](G2_03_MATCH.md)에 기록했다. 수정 `198f7a25194f5b4f98d96556e8267e6697c391dd`의 `TimerBeforeSameWorldTimeSale`은 최초 A 역할에서 Pass였지만 WorldContext 미등록 경고1건을 남겼다. 원본 `engine.log`2494줄과 warnings=1은 보존했다. `915148e`에서 fixture 생성/정리에 CreateNewWorldContext/SetCurrentWorld와 DestroyWorldContext를 짝으로 연결한 뒤 최종 새 경로의 같은 회귀가 **무경고 Success**임을 직접 확인했다. 제품 코드 변경이나 경고 필터링은 하지 않았다. 이 경계의 직접 시각 주입 자동화를 실제 OS 입력 재현으로 표시하지 않는다.
