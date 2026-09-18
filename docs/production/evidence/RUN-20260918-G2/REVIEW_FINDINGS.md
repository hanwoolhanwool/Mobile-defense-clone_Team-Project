# G2 독립 정적 리뷰 — B 첫 구현

검토 대상: `84389ce22205c8906592465be4d40e42434dda01`, BoardManager·EconomyService·CommandProcessor·Controller 개인 스냅샷. worktree의 후속 미커밋 변경 대신 해당 SHA의 `git show/diff`를 읽었다. 구현자와 별도 검토이며 코드 수정·빌드·실행은 하지 않았다. 아래 재현은 정적 코드 경로에서 도출한 실행 입력이고 **실제 재현 결과는 모두 NotRun**이다. UI·테스트·GameMode 연결은 작성 중이므로 아직 없는 기능을 완료 결함으로 분류하지 않았다.

기준은 [독립 기대값](REVIEW_PLAN.md)의 R01~12/R20/R22~24와 [명령·세대 계약](../../../technical/ARCHITECTURE.md)이다.

| ID·우선순위 | 파일·함수·기준 SHA의 줄 | 재현 상황과 영향 | 수정 방향·확인 상태 |
|---|---|---|---|
| B-01·차단 | `Network/LDCommandProcessor.cpp`, `SubmitAtTime` 120/154/169/173, `RegisterParticipant` 42 | epoch1 Request1 소환의 `OnBoardCommitted` 수신부에서 epoch2를 등록한다. 저장했던 Session 포인터의 값이 교체된 뒤, 원명령 복귀 시 epoch1 응답이 epoch2 Request1 캐시에 쓰인다. 새 연결 첫 명령이 옛 응답/Conflict를 받고 진행이 막힐 수 있다. 다른 참가자 추가로 TMap 저장소가 재배치되는 경우 보관 포인터도 무효화될 수 있다. `DrainCombatRewards`의 경제 알림도 같은 재진입 경계다 | 콜백을 넘어 Session 포인터를 보관하지 않고 원래 Context/Epoch로 재조회한다. 확정 응답을 원세대에 게시 전 캐시하거나 세대 등록을 직렬화한다. 보상 Drain 후에도 원세대 유효성을 재검사한다. 실제 게시 delegate에서 세대 교체/다른 참가자 등록 회귀 필요. B/root 전달, 수정 대기 |
| B-02·수정 필요 | `Network/LDCommandProcessor.cpp`, `SubmitAtTime` 140~146 | 처리 중 동일 참가자/RequestId를 다른 payload로 재제출하면 Busy다. 현재 계약은 RequestIdConflict이며 원요청·응답을 보존해야 한다 | 실행 중 키 일치와 내용 비교를 분리한다. 같은 내용 Pending, 다른 내용 Conflict, 다른 요청 Busy. B가 후속 `c20c209`에서 수정했다고 전달했으나 해당 수정 SHA의 재리뷰/실행은 아직 확인하지 않음 |
| B-03·차단 | `Core/LDPlayerController.cpp`, `OnRep_ConnectionEpoch` 48~54, `IsGameplaySnapshotReady` 175~179 | 같은 매치에서 연결 세대가 바뀌어도 기존 Board/Economy snapshot의 MatchId/player가 동일하다. 로컬 보드 준비가 끝나면 새 연결 초기 스냅샷 전에 Ready가 true가 되어 초기 동기화 차단 조건을 만족하지 못한다 | ConnectionEpoch와 두 스냅샷을 하나의 owner-only 복제 envelope로 묶고 Ready를 현재 epoch와 대조한다. 선행 도착 스냅샷을 OnRep 세대 변경에서 무조건 지우지 않는다. B가 envelope와 성공 응답의 두 Revision 대기 경로 구현 중, 재리뷰/실행 대기 |

검토 범위에서 추가 중대 결함을 발견하지 않은 경로: 실패 준비는 원본 RNG와 NextInstanceId를 진행시키지 않으며 최종 검증 뒤에만 두 원본을 바꾼다. 두 원본 사이에 delegate/스폰을 넣지 않고 이후 배치·경제 알림을 게시한다. 판매 보충은 최소 donor ID와 기존 잠금을 보존하고 합성은 세 개체 소비 후 배치한다. 사망 보상은 매치 일치 검사 후 양쪽 수혜자의 잔액을 먼저 함께 확정하고 알림을 보낸다. 서비스가 한 매치와 두 수혜자를 함께 소유하므로 현재 DeathEventId 집합은 이 범위에서 매치·수혜자별 단일 지급과 동등하다. UPROPERTY의 Data/Actor/서비스 참조와 World가 소유한 준비 Actor의 수명도 확인했다. 이는 정적 검토 결과이며 기능 Pass가 아니다.

후속 통합 확인: Processor.Close는 큐를 버리므로 승인된 최종 보상을 먼저 Drain한 뒤 닫아야 한다. Close 이후 신규 접수/보상 차단과 기존 캐시 재전달을 실제 GameMode 종료 경로에서 확인한다. Board.Close는 접수만 닫으며 Actor/전투 예약/구독 정리는 조립 주체의 종료 순서에서 검증한다. 준비 어댑터 실패·최종 stale plan·게시 재진입·응답/복제 도착 순서는 실제 서비스/Controller fixture와 두 프로세스에서 다시 확인한다.
