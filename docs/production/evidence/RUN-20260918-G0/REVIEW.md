# G0 독립 리뷰

대상: 통합 `3b4105e`, 구현자와 별도 검토 역할. 과거 구현 기록은 사용하지 않았다.

| ID / 규약 | 파일·함수 | 재현·영향 | 수정 방향·현재 상태 |
|---|---|---|---|
| R-G0-01 / ARCH-05·06 | Core/LDGameMode.cpp StopMatchServices, Core/LDPlayerController.cpp ServerRequestCommand | 처리된 요청 뒤 매치 Abort, 같은 연결에서 동일 요청 재전송. Controller의 Processor/문맥이 사라져 캐시 원응답 대신 PhaseNotAllowed | 접수 종료와 실제 Logout/EndPlay 해제 분리. Mode/Controller 경로 회귀 추가 중 |
| R-G0-02 / ARCH-05·06 | Core/LDGameMode.cpp PostLogin·InitGameState | 서비스 준비 전 로그인 후 InitGameState. 누락된 참가자가 재등록되지 않아 0/2·Epoch0 유지 | pending weak 참가자와 멱등 등록. 두 초기화 순서·중복 호출 회귀 추가 중 |
| R-G0-03 / ARCH-03·04 | Data/LDGameData.cpp | 정책 변조·보드 겹침을 유효값으로 받던 초안 | 14개 정책/좌표 변조 거절 및 기존 스냅샷 불변 테스트로 수정 확인. A 자동화4 Pass |

검토에서 확인한 구조: GameMode 구성, GameState 매치 원본, PlayerState 참가자 복제, Controller 입력/RPC, Processor 명령 기록, Data 불변 스냅샷을 분리했다. UObject 서비스는 UPROPERTY로 보존하고 참가자는 Weak로 참조한다. 권한 가드와 MatchId/Epoch 응답 필터가 있다. G0 Stub은 기능 성공을 만들지 않는다.

한계: R-G0-01/02 수정·새 회귀가 통과하기 전 G0 완료 아님. G0 검토는 아직 없는 보드/경제 공동 확정·전투·실제 네트워크·패키지 수명을 검증하지 않는다.
