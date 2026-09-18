# G2 독립 정적 리뷰 — 전투·경제·조립

검토 대상: `84389ce22205c8906592465be4d40e42434dda01`, BoardManager·EconomyService·CommandProcessor·Controller 개인 스냅샷. worktree의 후속 미커밋 변경 대신 해당 SHA의 `git show/diff`를 읽었다. 구현자와 별도 검토이며 코드 수정·빌드·실행은 하지 않았다. 아래 재현은 정적 코드 경로에서 도출한 실행 입력이고 **실제 재현 결과는 모두 NotRun**이다. UI·테스트·GameMode 연결은 작성 중이므로 아직 없는 기능을 완료 결함으로 분류하지 않았다.

기준은 [독립 기대값](REVIEW_PLAN.md)의 R01~12/R20/R22~24와 [명령·세대 계약](../../../technical/ARCHITECTURE.md)이다.

| ID·우선순위 | 파일·함수·기준 SHA의 줄 | 재현 상황과 영향 | 수정 방향·확인 상태 |
|---|---|---|---|
| B-01·정적 해소/실행 대기 | `Network/LDCommandProcessor.cpp`, `SubmitAtTime` 120/154/169/173, `RegisterParticipant` 42 | epoch1 Request1 소환의 `OnBoardCommitted` 수신부에서 epoch2를 등록한다. 저장했던 Session 포인터의 값이 교체된 뒤, 원명령 복귀 시 epoch1 응답이 epoch2 Request1 캐시에 쓰인다. 새 연결 첫 명령이 옛 응답/Conflict를 받고 진행이 막힐 수 있다. 다른 참가자 추가로 TMap 저장소가 재배치되는 경우 보관 포인터도 무효화될 수 있다. `DrainCombatRewards`의 경제 알림도 같은 재진입 경계다 | `892f3334965f4e2071cefd458ae2da9c8f21b1ac` 재리뷰: Context/Command 값 복사, Drain/게시 뒤 원세대 재조회, 준비 후 마지막 세대 검사, 게시 전 원세대 결과 캐시 반영. 저장 Session 포인터를 콜백 뒤 쓰지 않음. 실제 Board.OnBoardCommitted에서 epoch2로 교체하는 `EpochReplacementDuringPublication`의 새 요청1/pop2/gold58 기대값도 확인. UE 실행 NotRun |
| B-02·정적 해소/실행 대기 | `Network/LDCommandProcessor.cpp`, `SubmitAtTime` 140~146 | 처리 중 동일 참가자/RequestId를 다른 payload로 재제출하면 Busy다. 현재 계약은 RequestIdConflict이며 원요청·응답을 보존해야 한다 | `892f333`에 포함된 수정 재리뷰: 참가자+epoch+RequestId가 같을 때 내용별 Pending/Conflict, 다른 키 Busy로 구분. 이미 확정·캐시된 결과는 게시 중 재조회에도 원응답. UE 실행 NotRun |
| B-03·차단 | `Core/LDPlayerController.cpp`, `OnRep_ConnectionEpoch` 48~54, `IsGameplaySnapshotReady` 175~179 | 같은 매치에서 연결 세대가 바뀌어도 기존 Board/Economy snapshot의 MatchId/player가 동일하다. 로컬 보드 준비가 끝나면 새 연결 초기 스냅샷 전에 Ready가 true가 되어 초기 동기화 차단 조건을 만족하지 못한다 | ConnectionEpoch와 두 스냅샷을 하나의 owner-only 복제 envelope로 묶고 Ready를 현재 epoch와 대조한다. 선행 도착 스냅샷을 OnRep 세대 변경에서 무조건 지우지 않는다. B가 envelope와 성공 응답의 두 Revision 대기 경로 구현 중, 재리뷰/실행 대기 |

검토 범위에서 추가 중대 결함을 발견하지 않은 경로: 실패 준비는 원본 RNG와 NextInstanceId를 진행시키지 않으며 최종 검증 뒤에만 두 원본을 바꾼다. 두 원본 사이에 delegate/스폰을 넣지 않고 이후 배치·경제 알림을 게시한다. 판매 보충은 최소 donor ID와 기존 잠금을 보존하고 합성은 세 개체 소비 후 배치한다. 사망 보상은 매치 일치 검사 후 양쪽 수혜자의 잔액을 먼저 함께 확정하고 알림을 보낸다. 서비스가 한 매치와 두 수혜자를 함께 소유하므로 현재 DeathEventId 집합은 이 범위에서 매치·수혜자별 단일 지급과 동등하다. UPROPERTY의 Data/Actor/서비스 참조와 World가 소유한 준비 Actor의 수명도 확인했다. 이는 정적 검토 결과이며 기능 Pass가 아니다.

후속 통합 확인: Processor.Close는 큐를 버리므로 승인된 최종 보상을 먼저 Drain한 뒤 닫아야 한다. Close 이후 신규 접수/보상 차단과 기존 캐시 재전달을 실제 GameMode 종료 경로에서 확인한다. Board.Close는 접수만 닫으며 Actor/전투 예약/구독 정리는 조립 주체의 종료 순서에서 검증한다. 준비 어댑터 실패·최종 stale plan·게시 재진입·응답/복제 도착 순서는 실제 서비스/Controller fixture와 두 프로세스에서 다시 확인한다.

## A 수정과 실제 조립 재리뷰

고정 검토 SHA: 최초 전투 `19aa7e1e0e92fa1aa5b0287b47b536f6950feb37` → 수정 `28ead1bc530bef05cc9c06330238b9185a213a6b`, GameMode 조립 `8356a211c1cbbec29ed04a7694e9325e3e2111a7`. 통합 반영 지점은 `dba8afa`이며 이 기록 시점의 root Editor/자동화 실행은 아직 결과 대기다. R13~23에 해당하는 코드 경로만 읽었으며 실제 검수 Pass를 추가하지 않았다.

| ID·상태 | 함수·재현/영향 | 수정·검토 결론 |
|---|---|---|
| A-01·정적 해소/실행 대기 | 최초 `LDCombatService::AdvanceCombatTo` 158~163에서 예약 중 새 최인접 대상으로 바꾸며 SpawnedTime만 보정하여 관찰 전 피해시각을 기록함 | `28ead1bc`는 기존 예약을 예정 DueSeconds의 적 위치에서 다시 고르고, 그때 유효한 대상이 없으면 현재 관찰시각으로 새 예약. 타격 후 살아 있는 대상의 다음 예약도 보존. `ExactDuePositionAndNoBackdating`는 .275 시각 거리175/175.001과 .30 관찰을 분리. 코드 경로상 해소, UE 실행 NotRun |
| A-02·정적 해소/실행 대기 | 적을 현재 스텝까지 먼저 이동한 뒤 과거 DueSeconds의 사거리를 현재 위치로 검사하여175cm 진입/이탈 경계를 오판 | `EnemyActor::TryGetCanonicalPositionAt`의 순수 경로 sample로 예정시각의 거리·최인접·타격 연출 위치를 검사. .275 위치175에서 유효했던 타격은 .30 root177.5에서도 인정하며 root를 되감지 않음. 신규 관찰로 들어온 표적은 .30으로 기록. UE 실행 NotRun |
| A-03·정적 보강/실행 대기 | 등록 시 다른 world Actor 또는 오래된 보드 통지가 들어올 가능성 | Service Unit/Enemy 등록과 타격 재검증에 동일 World 검사. GameMode는 Commit.MatchId·PlayerIndex·증가 Revision·현재 BoardRevision을 검사하고 현재 보드의 확정 Actor를 조회하여 소유/종류/셀을 대조. 제거 등록 해제가 새 등록보다 먼저. cross-world Unit 거절 회귀 코드 확인, UE 실행 NotRun |
| I-01·차단/수정 대기 | `8356a211`의 `LDGameMode::AdvanceLogic` 358~372와 `LDPlayerController::SubmitServerCommand` → `CommandProcessor::Submit`의 시간 흐름 불일치. 마지막 전투스텝10.00, 기존 유닛 예정 타격10.025, 판매 RPC10.04가 timer10.05 전에 오면 판매가 먼저 즉시 확정되고 더 이른 타격은 unregister로 사라짐. 그 타격의 보상이10.04 구매를 가능하게 해야 하는 경우도 잘못 거절됨 | 명령 timestamp보다 이른 예정 사건과 보상을 먼저 처리하고 같은 timestamp에는 명령이 먼저인 단일 논리 순서로 연결한다. .05 단계 누락을 뒤늦게 따라잡는 것만으로 비정렬 RPC 시각을 보호하지 못한다. 실제 Controller→Processor→GameMode/Combat 경로에서 이전 타격→후행 판매/구매 및 동일시각 판매→타격취소를 각각 검증해야 함. A/root 전달, 실제 재현 NotRun |

조립의 정적 수명 확인: 서비스는 GameMode UPROPERTY로 유지되고, 논리 타이머는 정수 step 번호를 사용한다. StopMatchServices는 접수 차단→보상 Drain→Processor.Close→전투 Stop→구독 해제를 연결한다. 다만 AbortMatch는 Aborted를 먼저 게시하며 실제 승패 Result 경로는 아직 G3 범위이므로 최종 보상/결과 게시 순서를 통과로 판단하지 않았다. UI epoch envelope(B-03), 반복 매치, 실제 네트워크 도착 순서와 부하는 후속 검수다.
