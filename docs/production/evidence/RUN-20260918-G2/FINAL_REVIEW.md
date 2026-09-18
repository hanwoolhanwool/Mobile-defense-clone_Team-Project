# G2 최종 독립 리뷰 대조표

제품 검토 기준 SHA는 `5baa96059e94a142d45206290010b373cf39ea19`, 최종 검증 보강 SHA는 `ae6be1b0b06ed733425e01632a341fb4db4cad59`다. 둘 사이 Source/Config/Content/tools 변경은 Tests1파일과 Verification2파일뿐임을 대조했다. 구현자 A/B 및 통합 작성자와 구분된 리뷰 관점으로 고정 소스와 실제 테스트의 기대값을 읽었다. 엔진·빌드·소스 변경은 수행하지 않았다. 규칙 원본은 [24개 독립 기대값](REVIEW_PLAN.md), 구조 원본은 [ARCH-01~06](../../../technical/CODING_STANDARD.md)이며 수정 이력은 [REVIEW_FINDINGS](REVIEW_FINDINGS.md)에 둔다.

**현재 판정: G2 승인 가능, 발견된 제품 차단 결함 없음.** `5baa960`의 새 G1 출발점 조립56파일 불일치0·Editor Pass·Unreal 자동화39개 무경고 Pass 이후, 보존한 재현에 검증 보강을 적용하여 다시56파일 일치·Editor Pass·명령12개 무경고 Pass/0 Fail/0 NotRun·Engine 터치 드래그 포함20단계 실제 GPU host213/client57개 Pass를 직접 확인했다. 준비 상태 검사 오류도 수정·재검증했다. 제품 코드가 동일하므로 기존39개 전체와 변경 영향 명령12개의 증거를 연결하며44개 전체를 한 번에 재실행했다고 표시하지 않는다. 이는 P0 전체 완료를 뜻하지 않는다.

## 증거의 구분

| 구분 | 실제로 확인한 내용 | 사용 한계 |
|---|---|---|
| 역할 A | `ca5b672` Editor Pass, `G2-A-combat` 10 Pass(9무경고+1경고) | 전투9개와 실제 GameMode/PC 조립1개. OS 입력이나 실제 네트워크 경계 검사는 아님 |
| 역할 B | `4a69fe9` Editor Pass, `G2-B-commands/report/index.json` 7 Pass(3무경고+4경고) | 원본 result.json의3은 무경고 성공만 센 값. WorldContext·역할 폴더 사운드 누락 경고는 통합에서 수정 |
| 통합 기존 실행 | `Saved/P0Runs/G2-final-candidate-automation`의 result/report: `7aebcf8`, 39 Pass/0 Fail/0 NotRun | G0/G1 회귀22개+G2 17개. NullRHI이며 렌더링·패키지·실기기 증거가 아님 |
| 동일 시작점 재현 | `C:/Users/iam12/P0_lesson_replay_g2/Saved/P0Runs/Replay-G2-assembly/assembly.json`:56파일 blob 일치. `Replay-G2-editor`: Pass·122.13초, `Replay-G2-automation`:39개 무경고 Pass/0 Fail/0 NotRun | HEAD는 G1 출발점이므로 실행 결과와 조립 manifest의 SourceSha=5baa960을 함께 확인 |
| 최초17단계 GPU | 당시 검사 성공이나 응답 누락·추가 Actor·HUD 캡처 지적이 있었음 | 보강된20단계 검사의 성공으로 재사용하지 않음 |
| 보강 GPU 첫 실행 | 잘못된 Conflict payload로 실패 후 수정 | 입력이 구조적으로 유효해야 Conflict 우선순위 검사가 성립함. 결과를 숨기지 않음 |
| 최초 최종20단계 GPU | `Replay-G2-pair/host/result.json`212개, client57개 모두 Pass. root가 양쪽stage19·hoststage6 PNG 직접 확인 | 실제 Slate 입력 주입·owning RPC·단일 보상·HUD 재생성. stage12는 Controller 직접 Move 경로였음 |
| 추가 명령 회귀 최초 | `Saved/P0Runs/G2-supplemental-commands`: `c7ed2da`, 11 Pass/1 Fail/0 NotRun | `PreparedIsolationAndIdReservation`의 actor 전역 Hidden/Collision 기대2개가 실패. 현재 제품은 모든 mesh의 Visibility=false/NoCollision/Overlap=false이므로 그 실제 상태를 검사하도록 수정 요청 |
| 보충 재현 최종 | 같은 재현 경로 `Replay-G2-review-fix1-assembly`:56blob 일치·제품 불변, `review-fix1-editor` Pass, `review-commands`12개 무경고 Pass, `review-pair` host213/client57개 Pass. root가 최신 양쪽stage19/hoststage6 PNG도 직접 확인 | 보완5개 명령 검사·Engine InputTouch의 여러 프레임 drag binding 포함. 최종 금2/159·인구5/2, 실패 금9/인구4/가격28·붉은 거절 안내. `-nosound`, 물리 입력·패키지·Android 아님 |

후속 검토: `ebdf6f4`는 비어 있지 않은 실제 PrimitiveComponent 집합의 Visibility/Collision/Overlap과 비복제·미확정·파괴·ID 보존을 검사하도록 바꿨다. `c2ec079`는 GPU stage12를 Controller 직접 Move 호출에서 여러 프레임의 Engine InputTouch Began→Moved→Ended→Controller drag binding→명령/배치 검증으로 보강했고 `ae6be1b`에서 UE5.8 FTouchId 인수형을 맞췄다. 실제 보충 GPU에서 source-selected·Success·Actor/NextAttackAt·등록 수·양쪽 배치·필수응답 모두 확인했다. 이것은 실제 엔진 입력 처리 경로의 검증이며 물리 터치는 G4에 남는다.

과거 집계는 [자동화 집계 감사](automation-count-audit.json)가 원본 요약의 누락을 정정한다. 최초는33 Pass/1 Fail, 후속36 Pass, 그다음37 Pass다. 경고를 성공 합계에서 빼거나 실패로 바꾸지 않는다. 전체 범위와 원본 로그는 [검수 요약](SUMMARY.md)에 연결한다.

## 24개 기대값과 실제 코드·검사

아래 경로는 `Source/Mobile_defense_clone/` 기준이다. 자동화 이름은 `LD.P0.G2.` 접두어를 생략한다. **UE 확인**은 같은 시작점 재현의 실제 자동화에서 해당 입력을 실행했다는 뜻이고 해당 R의 모든 조합을 검사했다는 뜻이 아니다. **GPU 확인**은 위20단계 실제 두 프로세스의 명시적 fixture 실행이며 정적 확인·미실행과 구분한다.

| ID | 최종 코드의 책임·함수 | 실제 기대값/검사와 현재 범위 | 미검증·다음 위치 |
|---|---|---|---|
| R01 | `Economy/LDEconomyService::TryPrepare`, `Network/LDCommandProcessor::ExecuteCommand` | `Commands.AtomicSummonAndFailure`: 실제4회 비용92→8/n4, 다음 실패 원본 불변, 게시 시80/인구1. GPU stage0/5의 처치+1이 포함된80/9도 확인 | 일반 처치 없는8과 fixture 처치 후9를 혼용하지 않음 |
| R02 | `Battle/LDUnitActor` 생성자·`InitializePrepared`, `Board/LDBoardManager::TryPrepare/CancelPrepared` | 준비 ID 확정 조회 거절·취소2회/후행 원본 보존 UE 확인. 추가 `PreparedIsolationAndIdReservation`: 실제 모든 primitive 비표시/NoCollision/NoOverlap·비복제/미확정·취소 Actor 파괴·다음 성공ID1 UE Pass | 준비 중 성공 알림0의 별도 계수는 없으며 통지 호출이 PublishPrepared에만 있음을 정적 확인 |
| R03 | `Board::ValidateCommand/TryPrepare`, `Economy::TryPrepare`, `Processor::ExecuteCommand` | 재화 부족/준비 어댑터 null·인구20/외부 소유 실패 signature 불변. 추가 방어 배치는 추첨 복사본 후 비P0 M01 거절·RNG 포함 불변, 준비 수명 검사는 취소/한 번 실패 후 다음 성공ID1 UE Pass |18칸 완전 점유는 정상 P0 불변식에서 도달 불가이며 M01 검사를 만석 재현으로 표시하지 않음. 패키지 에셋 누락 주입은 G3에서 로딩 검사와 구분 |
| R04 | `Board/Economy::ValidatePrepared`, `Processor::ExecuteCommand`의 확정 직전 검사 | `RewardsAndPreparedRevision`: 준비 후 다른 명령을 확정하면 두 계획 stale, 반복 취소가 나중 확정 원본을 되돌리지 않음 | 존재·잠금·수용량을 따로 비정상 변경한 fixture는 없음. 공개 변경은 Revision을 증가시키므로 동일 방어가 적용됨을 코드로 확인 |
| R05 | `Processor::ExecuteCommand`, `Board::PublishPrepared`, `Economy::PublishPrepared` | `AtomicSummonAndFailure`: 첫 게시부터 보드·금액 동시 확정 및 같은 요청 캐시 Success. `EpochReplacementDuringPublication`: 게시 중 epoch 변경 후 새 요청1은 새 구매·pop2/gold58 | 확정 구간은 값 대입·준비 Actor 등록이며 스폰/delegate 없음. 실제 동시 네트워크 부하는 G3 |
| R06 | `Board::FindPlacementCell/TryPrepare`, `CombatService::RegisterCommittedUnit` | `StackMoveMergeAndRefill`: seed1776의 실제7소환이 C01, ID1~3=17/4~6=11/7=5, 인구1씩 증가. GPU 단계마다 기존 Actor/NextAttackAt 보존 확인 | player1 셀23은 데이터·BoardGeometry 기준이며 실제 양쪽 배치 일치/표시는 GPU 확인 |
| R07 | `Board::TryPrepare` Move 분기 | 전체3마리 빈칸 이동/.2초 재이동 거절·원본 불변 UE 확인. 최종 GPU는 여러 프레임 Engine InputTouch drag→명령/배치까지 Pass. 추가 `WholeStackSwapKinds`: 같은/다른 종류3+2 전체교환, ID/Actor5개 보존·생성/제거0·잠금2.3·경제/RNG 불변 UE Pass | 교환 setup은 알려진 종류를 실제 준비 API로 지급한 명시적 보드 fixture이며 유료 소환 확률 검사가 아님. 물리 터치는 G4 |
| R08 | `Board::ValidateCommand/TryPrepare`, `Economy::TryPrepare/DrawUnit` | 정확3소비/1생성/인구−2/n7유지/첫빈17 UE 확인. G0 중복 ID 구조 거절, GPU 다른 칸 재료 거절. 추가 `GradeSalesAndLegendaryMerge`: 실제3전설 자료의 FeatureDisabled·전체 signature 불변 UE Pass | 존재하지 않는 재료 ID의 별도 서비스 조합은 미실행이며 동일 유효성 분기 정적 확인. RNG 균등 통계 검수는 G3 밸런스와 별도 기록 |
| R09 | `Economy::TryPrepare` Sell, `Board::TryPrepare` donor 선택 | 일반 환급14·보충 Actor 동일 UE/GPU 확인. 추가 `GradeSalesAndLegendaryMerge`: 실제 n4 구매 뒤 R01/E01/L01 판매 별1/2/4·금8/n4/가격28/RNG유지·정확1개 제거 UE Pass | 알려진 상위 등급 초기 지급은 명시적 fixture. 동시 판매/구매 네트워크 순서는 G3 |
| R10 | `Processor::SubmitAtTime/CacheResult`, `GameMode::StopMatchServices` | 원 EventId 재전달·signature 불변, 게시 중 같은 요청 Success, G0 TerminalSessionReplay의 실제Mode/PC Abort 후 원응답/Conflict·새요청 거절 UE 확인 | 처리 중 Pending 분기 자체는 코드 확인. GPU stage16은 서버 PC API의 원결과 비교이며 완료응답이 네트워크로 재전달됐다는 검사는 아님. Result 실게임 경로는 G3 |
| R11 | `Processor::SubmitAtTime`, `PC::ClientCommandResult/OnRep_ConnectionEpoch` | G0 LimitsAndExpiry의256축출/세대 거절, `ExpiredRequiresCurrentRevisions`의 실제축출→보상 후 rev1/2, `ControllerResponseSnapshotOrders`의 새 세대 스냅샷 차단 UE 확인 | 이전 세대의 지연 응답이 실제 통신으로 도착하는 검사는 G3. 처리 중 다른 내용 Conflict는 분기 정적 확인 |
| R12 | `Board::ValidateCommand`, `PC::SubmitServerCommand`, `Economy::ApplyCombatReward` | 실제 외부 소유 판매가 양쪽 원본 불변, 다른 Match/epoch 거절 UE 확인. GPU stage13의 owning PC 외부 개체 조작 거절·원본 불변 확인. 보상 금액/수혜자 필드·보상 RPC 없음 | 악성 다중 요청/지연은 G3 |
| R13 | `CombatService::RegisterCommittedUnit/AdvanceCombatInternal` | `AllSixteenBasicAttacks`의 .20무피해/.25첫공격·개별 다음 간격, `InitialCadenceMoveAndReplenishment`의 같은ID 등록 수1/NextAttackAt 유지 UE 확인. GPU 등록 수/Actor 일치 확인 | 동일 보드 commit 재통지는 Mode Revision으로 차단함을 코드 확인 |
| R14 | `Board::ValidateCommand/TryPrepare`, `CombatService::ResolveScheduledAttacks` | 실제 .2 재이동 거절, 전투 fixture에서 NextAttackAt5.25 유지·잠금5.4까지 피해0/정확5.4 타격 UE 확인. 추가 교환 검사는 양쪽5개체 잠금2.3 확인 | 기존 더 긴 타이머12의 숫자 조합은 미실행이며 max 구현으로 확인 |
| R15 | `Board::TryPrepare` Sell, `CombatService::RegisterCommittedUnit` | 보충 Actor/ID 동일·새 생성0, 전투 fixture의6.4 예약/5.4잠금 유지 UE 확인. GPU stage11의 실제 서비스 결합 타이머 보존 확인 | 보충 즉시 새 중심 공격의 별도 시각 fixture는 없음 |
| R16 | `CombatService::AdvanceCombatBefore/ResolveScheduledAttacks`, `GameMode::AdvanceLogic` | `ExactDuePositionAndNoBackdating`의 새 관찰.30 vs 기존 due.275, `SharedTargetRemovalAndStop`, `CommandClockOrdersEarlierHitAndSameTimeSale`, 실제 Mode/PC `Integration.TimerBeforeSameWorldTimeSale` UE 확인 | 타이머 본문을 직접 호출하는 실제 조립 검사이며 OS 입력/TimerManager 발화 자체의 증거는 아님. 합성 제거도 동일 unregister 경로임을 코드 확인 |
| R17 | `CombatRules::SelectTarget`, `EnemyActor::TryGetCanonicalPositionAt` |175포함/175.001제외·XY만 사용·SpawnSerial/EnemyId tie·nearest·반대 Route1 공용 적 양쪽 공격, 실제 이동 적 due 위치 경계 UE 확인 | 움직이는 다수 적과 시각 표시 종합 검수·대표 부하는 G3 |
| R18 | `CombatRules::TryCalculateDamage`, `CombatService` | 수작업16행 피해/간격/사거리/표현 표와 실제 두 타격 대조. 물리13/30, 마법17/5, 실패 출력 보존 UE 확인 | 정지 HP10000 fixture이며16종 실매치 균형 검수는 G3. 스킬/강화/마나/치명타 실행 경로를 추가하지 않음 |
| R19 | `EnemyActor::TryApplyDamage`, `CombatService::ResolveScheduledAttacks` | C01 HP55/40/25/10/0 및 같은 스텝 추가피해0, 다른40피해 둘·동일DamageId 재전달→HP0/사망1·늦은피해 거절 UE 확인. GPU 실제 처치·중복 사망 단일 보상 확인 | 표현은 확정 cue만 받고 피해 호출하지 않음을 정적 확인 |
| R20 | `Processor::EnqueueCombatReward/DrainCombatRewards`, `Economy::ApplyCombatReward` | 일반Death2회·boss30/30.0001→양쪽301/별5 UE 확인. GPU 실제사망 중복도81/101. 추가 `TenfoldDeathAndTwoFastBosses`: 각 빠른boss사망10회→양쪽+200/별6·Revision2/RNG불변 UE Pass | 내부 사망 사실을 공급한 정산 계약 검사. 실제 보스 생성/마감/종료 정산은 G3 |
| R21 | `GameMode::AdvanceLogic/AdvanceBeforeExternalCommand`, `CombatService` |10.025 타격→10.04 구매 전 보상 반영27+1−28=0, 동일시각 판매 우선, 현재WorldTime 경계를 열어 두는 Mode/PC 검사 UE 확인 | 생성·적 한도·보스 마감·최종 Result 순서는 아직 G3 코드/검수. G2 성공으로 마감 정확시각 검사를 대체할 수 없음 |
| R22 | 아래 ARCH 대조표 및 네 서비스·조립 주체 | 원본 위치와 의존 방향, 준비/확정/게시 경계를 실제 파일·함수로 확인. 복제 snapshot/Actor 배치는 파생값이며 원본 수정 경로가 아님 | G3 Wave/Result 추가 뒤 동일 원본/의존 규약 재검토 필요 |
| R23 | `GameMode::StopMatchServices/ReleasePlayerSessions`, `CombatService::Stop`, Actor EndPlay | 비권한/다른Match 피해 거절·cross-world 등록 거절·Stop2회·새공격/등록0·구독 해제·Close 후 새보상0·실제Mode Abort/Logout/EndPlay 반복 UE 확인 | 실게임 Result·반복 매치·최종 패키지 타이머/구독 누수는 G3. 기기 전환·Android 수명은 G4 |
| R24 | `PC::ClientCommandResult/UpdateGameplayView`, `GameplayWidget::NativeConstruct/NativeDestruct` | 실제 PC/LocalPlayer/PlayerState의 Success/Expired×응답/스냅샷 선착4조합·한 Revision 부족 차단·새epoch 초기 동기화 UE 확인. GPU 양쪽 HUD 재생성/단일버튼효과 확인 | 역순 지연은 응답 bucket clock fixture이며 실제 네트워크 지연 아님. 네트워크 지연·동시 요청 부하는 G3, 물리 터치/SafeArea는 G4 |

## ARCH-01~06 구조 대조

| 규약 | 실제 경로·책임 및 선택 이유 | 리뷰 결론과 남은 한계 |
|---|---|---|
| ARCH-01 책임 | `PC`는 의도·요청·응답 대기, `GameplayWidget`은 표시/클릭, `BoardManager`는 칸·개체·잠금, `EconomyService`는 비용/RNG/정산, `CombatRules/Service`는 계산/예약 | 경제 공식이 HUD나 GameMode에 복제되지 않음. 네이티브 UMG를 선택하여 기존 layout-spec 값을 화면 표시로 연결; 거대한 공용 관리자에 규칙을 모으지 않음 |
| ARCH-02 의존 | GameMode 생성·주입→Processor→Board/Economy, Board 확정 delegate→GameMode→Combat, Combat 사망→GameMode→Processor 내부 큐 | Board/Economy/Combat가 UI/Controller를 역탐색하지 않음. `LDLocalPresentationSubsystem`만 로컬 표시 주체를 연결하고 World 수명에서 해제 |
| ARCH-03 상태 | Board States/NextInstanceId, Economy States/RandomStreams/RewardedDeaths, Combat Units.NextAttackAt, Enemy CombatSnapshot.HP, Processor Session 캐시 | const/값 snapshot 조회. Actor Placement·owner envelope·UI 선택은 복사본/뷰 상태. 동일 Match 안 두 수혜자 공동 지급이 단일 DeathId 집합으로 보장되며 새 Match는 새서비스 |
| ARCH-04 처리 | Processor: 값을 복사해 준비→두 ValidatePrepared→두 CommitPrepared→원세대 CacheResult→Publish. 콜백 전후 Session 재조회 | 준비에만 실패 가능한 Actor 초기화·RNG 복사본 사용. 통지 전에 두 원본/응답 확정. 처리 중 재진입은 Busy/Pending/Conflict이며 현재 거래를 중복 확정하지 않음 |
| ARCH-05 수명 | GameMode UPROPERTY 서비스, Board UPROPERTY Actor, Combat 약한 Actor 참조; StopMatchServices에서 접수닫기→보상Drain→Close→Stop→delegate 제거 | 새 손상/보상 차단과 기존 응답 캐시를 구분. PC가 요청을 소유하므로 HUD 재생성은 새 요청을 만들지 않음. 실제 반복 매치·Result 최종 보상은 G3 검수 전 |
| ARCH-06 검증 | 독립16행 표·수작업 금액·시각 경계·실제 Mode/PC fixture, GPU의 명시적 개발 전용 fixture, 단계별 결과/ID집합 검사 | 자동화 집계 오류·잘못된 Conflict payload·누락된 응답/유령 Actor 검사를 리뷰에서 발견·수정. 표의 정적/UE/GPU/패키지/기기 범위를 합치지 않음 |

확인된 구조 선택에서 G2 제품 코드의 신규 차단 결함은 발견하지 않았고 요청한 G2 보완 회귀·입력 검수도 통과했다. G3는 최종 PC 패키지 데이터/에셋·청음·실제 지연/동시 요청·반복 매치·10웨이브/보스 마감/최종 승패·대표 부하·밸런스를, G4는 설치/실기기 실행·물리 터치/SafeArea·기기 성능을 각각 수행해야 한다. 이 표에 남긴 방어 조합의 정적 확인을 실제 실행으로 바꾸어 기록하지 않는다.

최종 GPU의 host/client 프레임 p95는 각각16.6669ms였지만60FPS 제한·정지 HP70 적1마리·HP1 자금용 적100마리의 작은 기능 fixture이므로 대표 부하 Pass나 성능 개선 근거로 사용하지 않는다.40유닛·일반 적 상한·보스가 함께 있는 대표 부하는 G3에서 측정해야 한다. 측정 환경은 SUMMARY가 가리키는 공통 실행 기록을 따른다.
