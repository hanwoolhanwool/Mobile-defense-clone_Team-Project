# A G0 native 매치 재현 — 2026-09-27

**A-02 참고 자료 Verified: 기존 TopDown 맵 + native LDGameMode 경로. 실제 학습자는 Planned.** 수동 BP_GameMode/BP_GameState 에셋 생성, B 명령 RPC, 최종 패키지·Android를 이 판정에 포함하지 않는다. G3 실행의 성공을 G0에 소급한 것이 아니라, G0 독립 완료 소스를 새로 실제 실행했다.

## 입력과 실행

공통 HEAD `8c6856d235de87cc28c12b49ca775bd0937334a5`, 제품 A 코드 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`, 제공 데이터/설정 `81ba665adff6c60ef95f79319b3115050937a1c1`을 [기존 조립](a-assembly.json)로 복원했다. 제공 검사기는 `fdc12b7e881e231c846c42baa6b2c52360003f52`의 `learning/tools/G0Replay/A/LDG0PieTests.cpp`이며 SHA256은 `A60B9877198BF20D5909F724987058CEAF992D592B504EC320A84510E167878E`다. 제품 Core/Data는 변경하지 않았다. 검사용 파일을 Tests에 추가하고 Build.cs의 Editor 조건에만 UnrealEd/SlateCore를 추가했다. [실행 뒤 입력 대조](a-native-inputs.json)는 이 Build.cs를 제외한 정상20개/누락19개 원래 입력의 blob 불일치0이다.

정상 프로젝트는 `C:/Users/iam12/P0_lesson_replay_a`, 누락 입력은 **별도 신규 detached** `C:/Users/iam12/P0_lesson_replay_a_missing`이다. 누락 fixture는 [20파일 조립](a-missing-assembly.json) 시 `Content/LD/Data/GameRules.json`을 처음부터 제외했다. 기존 프로젝트·정상 재현본의 JSON을 삭제하거나 바꾸지 않았다. 조립 manifest의 NotRun은 조립 당시 상태이며 아래 실제 결과가 후속 증거다.

| 범위 | Editor | 실제 GPU PIE | 최종 report·proof |
|---|---|---|---|
| A 정상 | [Pass/exit0](a-native-editor.json),23:48:53~23:49:35 KST | `LD.PIE.G0.A.MatchLifecycle`,23:49:36~23:50:08, PID20484 | [실행](a-native-pie.json), [proof](a-native-proof.json): Success1/Fail0,2세션·셋째 거절·설정 복원 |
| A 누락 | [Pass/exit0](a-missing-editor.json),23:50:08~23:51:28 KST | `LD.PIE.G0.A.MissingData`,23:51:29~23:51:59, PID48816 | [실행](a-missing-pie.json), [proof](a-missing-proof.json): Success1/Fail0,1세션·실제 로딩 Abort·설정 복원 |

두 실행 모두 **경고가 있는 Success**다. [원본 report 필드·경고 이벤트](a-native-reports.json)를 보존했다. runner의 `Warnings=1`은 경고가 있는 **테스트 수**이며 경고 메시지 개수가 아니다. 정상은 셋째 거절의 ConnectionLost1개와 RecastNavMesh를 찾지 못한 CrowdFollowing3개로 총4개, 누락은 CrowdFollowing1개다. 테스트의 네트워크 event는 해당 ConnectionLost가 요청한 셋째 PIE World임을 확인했다. NavMesh 경고는 남겨 두며 이 수업은 NavMesh/AI 이동을 검증하지 않는다. 무경고 통과로 기록하지 않는다.

## 독립 기대와 실제 관찰

- 실제 EWorldType::PIE의 listen/server와 client 소유자0/1, 동일 MatchId·epoch1/2, 타인 공개 번호 복제와 private 문맥 비복제, Preparing/readiness의 실제 client OnRep를 확인했다. A Stub 접수는 false다.
- 공개 InitGameState/InitializeMatch/InitializeParticipant/PostLogin 동일 입력 재호출 뒤 기존 로더·매치·epoch가 유지됐다. 다른 매치/epoch 초기화와 client 상태 변경은 거절됐다.
- `RequestLateJoin`으로 실제 세 번째 NetConnection/PostLogin→Logout/Destroy를 관찰했다. 임의 Controller 생성 검사가 아니며 기존 두 슬롯과 epoch를 보존했다.
- 첫 세션 Result, 둘째 Aborted는 공개 GameState 전이를 통한 **명시적 종료 fixture**다. 전투 승패가 아니다. 양쪽 종료 상태 복제·Running/Preparing 역전 거절·같은 종료 멱등성을 확인했다. 실제 Mode에 바인딩한 검사 타이머는 반복 EndPlay 후 제거됐다. 두 세션 모두 PIE World0으로 끝나며 새 매치 GUID는 이전과 달랐다.
- 누락 fixture는 실제 Mode.InitGameState→LoadP0 실패로 `GameRules.json: cannot read required runtime JSON`을 양쪽에 게시하고 Aborted·입력 닫힘·Running 재진입 거절을 확인했다. 이미 종료한 Mode에 새 타이머를 넣지 않았다. 반복 EndPlay와 PIE 종료 후 World0을 확인했다.

[정상 host](a-native-host.png)·[정상 client](a-native-client.png), [누락 host](a-missing-host.png)·[누락 client](a-missing-client.png)는 실제 창546×720이다. G0 native 기본 카메라에서 템플릿 벽·하늘만 보이고 게임 HUD/전장/버튼은 없다. 정상과 Aborted의 화면이 비슷하므로 PNG만으로 매치 상태를 판정하지 않으며 실제 복제/OnRep와 report가 근거다. 정상 둘째 세션 캡처534×708은 원본 proof 경로에 보존했다.

## 같은 절차로 다시 실행

1. [공통 조립 절차](../../COMMON.md#g0-replay)로 **새** detached 폴더에 A21파일을 조립한다. 기존 재현본을 초기화하지 않는다. 누락 검사는 다른 새 폴더에 같은 도구의 `-Role A -MissingRulesFixture`로20파일을 조립한다.
2. [제공 검사기 설치 설명](../../../tools/G0Replay/A/README.md)에 따라 고정 fdc12b7 파일을 `Source/Mobile_defense_clone/Tests/LDG0PieTests.cpp`로 복사하고 Editor-only 의존을 추가한다. SHA256을 위 값과 비교한다. 제공 검사는 직접 작성할 매치 코드가 아니다.
3. 기존 빌드/게임 작업이 끝난 뒤 해당 폴더의 `tools/Build-P0Editor.ps1 -ProjectRoot <대상> -RunId <새 ID>-editor`를 실행한다. 컴파일 실패를 제품 API 변경이나 최신 G3 코드 복사로 우회하지 않는다.
4. 참고 저장소의 `learning/tools/Test-P0G0PIE.ps1 -ProjectRoot <대상> -RunId <새 ID>-pie -Filter LD.PIE.G0.A.MatchLifecycle`를 실행한다. 누락 fixture는 필터만 `LD.PIE.G0.A.MissingData`로 바꾼다. 엔진은 UE5.8, GPU Editor이며 `-NullRHI`를 쓰지 않는다. 새 RunId/전용17879 도구 포트와 직렬 실행을 유지한다.
5. 제공 검사는 `/Game/TopDown/Lvl_TopDown`·native ALDGameMode override·ListenServer·한 프로세스의2인·창540×720·온라인 subsystem 끔을 요청 단위로 설정한다. 맵/BP를 저장하지 않는다. 기존 Editor 설정을 복제하고 마지막에 원래 config 전체를 복원한다. 원래 PIE 세션이 열려 있으면 교체하지 않고 실패한다.
6. `Saved/P0Runs/<ID>-pie/result.json`, `report/index.json`, `<ID>-pie-proof/pie-proof.json`을 함께 확인한다. proof의 Pass는 저장 시 관찰 범위이므로 **최종 Automation report Success도 필수**다. expectedErrorsMet/settingsRestored=true, 오류0, 정상2세션 또는 누락1세션, World0과 위 관찰을 확인하고 경고 원문도 남긴다. `engine.log`/`build.log`는 원본 폴더에 유지한다.

실제 원본 RunId는 `G0-actual-network-replay-A-{editor,pie,pie-proof}` 및 누락 폴더의 `G0-actual-network-replay-A-missing-{editor,pie,pie-proof}`다. queue는 통합 `Saved/P0Runs/G0-actual-network-replay/queue.json`에 있으며 B/canonical 결과와 개별 수업 검증 범위를 섞지 않는다.

## 제공 검사기의 정적 수정과 한계

실행 전 독립 리뷰에서 세 결함을 수정했다. `fc30113`은 PIE 시작 명령 생성자의 전역 delegate 구독을 실제 큐 순서의 Update까지 늦췄고, `844f9ca`는 네트워크 감시를 두 세션 전체에 유지하며 참가자 구독 해제와 분리했다. `fdc12b7`은 내부 명령의 private InternalUpdate를 직접 구동할 수 없어 wrapper의60초 시작 제한을 추가하고, 오류 뒤 새 세션을 시작하지 않고 정리/복원으로 진행한다. **실제 실행 실패를 관찰한 수정이 아니라 실행 전 정적 발견**이다. 동기 엔진 호출이 멈추면 실행 도구의300초 제한을 사용한다.

제품 수업의 실제 실패→수정 기록은 기존 A-01 컴파일 오류/문자열 수정에 남는다. 이번 독립 G0 PIE는 처음 실행한 두 필터 모두 위 경고와 함께 통과했다. B 실제 명령 RPC·canonical 통합·후속 경로/전투·최종 패키지·Android는 별도 수업/검수다. 수동 BP_GameMode/BP_GameState 생성은 보존된 선택 절차이며 이 Verified 경로에 포함하지 않는다.
