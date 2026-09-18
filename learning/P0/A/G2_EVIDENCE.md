# G2 A 실행 증거

현재 수업 제작은 **Draft**, 실제 학습자는 **Planned**다. 아래는 통합 담당자가 직렬 실행한 원본 JSON을 A가 직접 읽어 확인한 결과이며 수업 재현이나 화면 검수는 아니다.

| 실행 | HEAD·결과 | 증거 |
|---|---|---|
| 최초 통합 Editor | `dba8afa521c68dc48cc07207d22b7f72c06c4283`, Pass/Exit0, 2026-09-18 15:14:39~15:14:58 KST | `C:/Users/iam12/P0_reference_integration/Saved/P0Runs/G2-composed-editor/result.json`, 전체 `build.log` |
| 최초 전체 NullRHI 자동화 | 같은 HEAD, 전체30Pass/1Fail/0NotRun | `C:/Users/iam12/P0_reference_integration/Saved/P0Runs/G2-composed-automation-initial/result.json`, `Report/index.json`, 전체 `engine.log` |
| A 전투 부분집합 | `LD.P0.G2.Combat.*` 7Success/0Fail | 위 `Report/index.json`의 fullTestPath/state를 직접 확인 |
| 실패 분석 | 유일 실패 `G0.Integration.LoginReadinessOrders`: 이전 Stub 문구와 새 서비스 준비 문구 차이 | 실제 Phase/참가자 수는 정상. `eafc378`에서 문구 전체 일치 대신2참가자·Preparing 의미를 검사하도록 수정 |
| A 역할 후속 Editor | `ca5b67238987ebeb227f0be85154b5a7d4bdf7c6`, Pass/Exit0, 2026-09-18 15:42:57~15:43:24 KST | `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G2-A-editor/result.json`, 전체 `build.log` |
| A 역할 G2 NullRHI 자동화 | 같은 HEAD,10Pass(9무경고/1경고)/0Fail/0NotRun | `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G2-A-combat/result.json`, `report/index.json`, 전체 `engine.log` |

7개 Pass는 AllSixteenBasicAttacks, DamageAndTargetBoundaries, DuplicateDamageAndSingleDeath, ExactDuePositionAndNoBackdating, InitialCadenceMoveAndReplenishment, SharedTargetRemovalAndStop, UnitPresentationPreservesCanonical이다. 고정 데이터16종 공격·정확 예정 사거리·타이머·단일 사망·종료·표시 파생값을 실제 Unreal 코드로 확인했으며 정지 적/직접 시각 주입 transient fixture다.

그 후 독립 조립 리뷰가 비정렬 RPC 명령 시각에 더 이른 공격/보상이 뒤로 밀리는 결함을 발견했다. `eafc378163f3d9ed939821d153228f9b9d29943e`는 전투 strict-before hook과 전역 예정 순서를 수정하고 CommandClockOrdersEarlierHitAndSameTimeSale/EarlierKillFundsExternalPurchase를 추가했다. 후속 A 역할 실행에서 **전투9개 모두 무경고 Pass**를 직접 확인했다. 이전7개 결과를 재사용하지 않고 새 실행 HEAD와 결과를 연결했다.

이번에 확인하지 않은 범위: PIE, G2 실제 첫 소환→처치→보상 두 프로세스, G2 패키지, 지연·동시 명령, 반복 매치·UI 수명, 대표 부하, Android. G1 패키지 결과가 존재해도 G2 성공으로 대체하지 않는다.

현재 WorldTime이 타이머 뒤 입력과 공유될 수 있다는 후속 엔진 소스 검토는 [연결 수업의 재현·기대·한계](G2_03_MATCH.md)에 기록했다. 수정 `198f7a25194f5b4f98d96556e8267e6697c391dd`와 신규 `LD.P0.G2.Integration.TimerBeforeSameWorldTimeSale`도 실제 Unreal Pass다. 다만 판매 시 fixture World가 엔진 WorldContext에 등록되지 않아 `UWorld::DestroyActor: World has no context!` 경고1건을 남겼다. `engine.log`2494줄과 해당 테스트의 warnings=1을 직접 확인했다. fixture 생성에서 CreateNewWorldContext/SetCurrentWorld, 정리에서 DestroyWorldContext를 짝으로 연결하는 최소 수정 후 무경고 재검증은 대기 중이다. 제품 코드 변경이나 경고 필터링은 하지 않았다. 엔진 호출 순서 확인과 직접 시각 주입 자동화를 실제 OS 입력 재현으로 표시하지 않는다.
