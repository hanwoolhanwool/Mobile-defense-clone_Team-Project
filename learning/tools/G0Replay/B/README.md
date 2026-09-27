# G0 B 실제 PIE·소유 RPC 검사 제공 파일

상태: **작성·정적 대조 완료 / Unreal 컴파일·실행 NotRun**. 학습자 진행은 Planned다. 이 파일은 검사 준비 기록이며 G0 수업을 Verified로 올리는 증거가 아니다. [기존 G0 재현 범위](../../../P0/evidence/G0_REPLAY/SUMMARY.md)에 빠진 실제 PIE/소유 RPC를 보완한다.

제공 원본은 [LDG0PieTests.cpp](LDG0PieTests.cpp)다. 제품 API·런타임 기능·에셋·설정은 변경하지 않는다. G3 제품 파일을 G0에 가져오지 않는다.

## 입력 확인과 설치 순서

1. B 독립 재현본은 `C:/Users/iam12/P0_lesson_replay_b`, HEAD는 공통 출발 `8c6856d235de87cc28c12b49ca775bd0937334a5`다. 실제 역할 소스는 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`, 제공 기반은 `81ba665adff6c60ef95f79319b3115050937a1c1`이다. `Saved/P0Runs/Replay-G0-B-assembly/assembly.json`의 **27파일 모두** `git hash-object`로 대조해 일치한 뒤 이 검사를 추가했다. HEAD만으로 조립 입력을 판정하지 않는다.
2. 원본을 `Source/Mobile_defense_clone/Tests/LDG0PieTests.cpp`로 복사한다. 작성자가 이 복사까지 수행했고 두 파일의 SHA256 일치를 확인했다. 파일을 교체하는 재시도에서는 먼저 기존 입력을 확인·보존한다.
3. 역할 원본 `Source/Mobile_defense_clone/Mobile_defense_clone.Build.cs`의 기존 내용에 다음 **Editor 전용 의존 블록만** 추가한다. B 독립 파일에는 Json/JsonUtilities가 이미 있으므로 다시 추가하지 않는다. 실제 B 재현본에는 아래 블록만 추가했고 기존 manifest와 달라진 파일은 Build.cs 한 개뿐이다.

```csharp
if (Target.bBuildEditor)
{
    // Provided G0 replay PIE fixture only; no runtime or packaged dependency.
    PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "SlateCore" });
}
```

4. B 독립 빌드에는 별도 define을 넣지 않는다. 필터는 **`LD.PIE.G0.B.OwnedRPC`** 하나다. fixture는 `WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && DO_ENABLE_NET_TEST`에서만 등록된다. 조건이 맞지 않아 테스트가 검색되지 않으면 통과로 간주하지 않는다.
5. canonical의 별도 재현은 `649c1dedd6832c41089a76b59bc76518cd262296` 제품을 쓰는 새 worktree에 같은 검사 파일을 설치한다. 위 Editor 의존 블록 안에 `PrivateDefinitions.Add("LD_G0_CANONICAL_PIE=1");`를 추가한다. 이때는 **`LD.PIE.G0.Canonical.TerminalCache`**만 등록된다. B 독립과 canonical 실행을 별도 RunId·결과로 보관한다. 원래 B GameMode에 AbortMatch를 추가하지 않는다.

기존 Config/DefaultEngine.ini의 에디터 부수효과·사용자 변경과 에셋은 보존한다. 새 검사용 파일은 원래27개 manifest와 구별하고, 현재 제공 파일의 SHA256·Build.cs 의존 diff를 실행 결과에 함께 기록한다.

## 실행 순서와 출력 계약

빌드·GPU 에디터·포트는 통합 담당이 직렬로 사용한다. 이 작업에서는 실행하지 않았다.

1. 대상 재현본의 Editor Development 빌드 후 기존 G0 NullRHI 자동화를 먼저 수행한다.
2. 통합 담당의 `learning/tools/Test-P0G0PIE.ps1`로 해당 필터를 실행한다. 직접 에디터를 실행할 경우 `-ExecCmds="Automation RunTests LD.PIE.G0.B.OwnedRPC" -TestExit="Automation Test Queue Empty" -P0PIERun=<새 이름>-proof`와 별도 automation report/log 경로를 지정한다. 실제 GPU Editor가 필요하며 `-NullRHI` 실행은 이 증거로 쓰지 않는다.
3. 필터는 기존 PIE World나 기존 ProcessEvent observer가 있으면 거절한다. 새 세션은 `PlayAsListenServer`, 동일 프로세스 안의 실제 PIE 네트워크 World 2개, 540×720 요청 창으로 연다. `Request.GameModeOverride=ALDGameMode::StaticClass()`와 기존 `/Game/TopDown/Lvl_TopDown`을 사용한다. 맵/BP를 수정하지 않는다. **native 선택 경로** 검사이며 BP GameMode 연결·게임 HUD·시각 배치는 미검증이다.
4. 결과는 `Saved/P0Runs/<P0PIERun 값>/pie-proof.json`이다. 기존 폴더는 덮어쓰지 않는다. JSON에는 `result`, `productSource`, `canonicalTerminalVariant`, `settingsRestored`, `packetSettingsRestored`, `eventObserverRestored`, `sendObserverRestored`, 실제 서버 수신 요청/클라이언트 수신 응답, 서버 RPC 송신 시도와 단계 관찰이 들어간다. 요청별 서버 수신 시각·프레임도 기록하며 자동화 보고서의 실패/경고와 함께 읽는다.
5. 정상·실패·60초 단계 제한 뒤 모두 관측 delegate와 packet 설정을 정리하고 PIE를 종료한다. World0·원래 Play 설정 복원을 확인한 뒤 proof를 저장한다. 엔진 자체 crash는 후속 정리나 JSON 저장을 보장할 수 없으므로 원래 engine.log를 실패 증거로 남긴다.

## 구현과 독립적인 기대값

| 단계 | 입력·독립 기대 | 실제로 관측하는 경로 |
|---|---|---|
| 준비/신원 | host/client Preparing, 같은 유효 MatchId, PlayerIndex0/1, 서로 다른 양의 epoch, 소유 PC의 MatchId/epoch 복제 | GameState/PlayerState와 owner-only PC 프로퍼티를 읽기 전용 조회. 실제 ListenServer/Client World와 NetConnection 확인 |
| 최초 소유 명령 | G0 Board/Economy Stub이므로 **PhaseNotAllowed**. 성공 재화/보드 효과를 만들지 않음 | 로컬 SubmitLocalCommand→Server RPC→server Processor→Client RPC→OnCommandCompleted |
| 즉시 재요청 | pending 중 새 의도는 거절, RetryPending은 번호1을 재사용. 서버 실제 요청2개·클라 실제 원응답2개, terminal 완료1회·cache1개 | `_Implementation`을 직접 호출하지 않음. host의 로컬 완료는 원격 전송 횟수에 포함하지 않음 |
| 다른 소유자 문맥 | 자기 소유 RPC에 host epoch를 실으면 서버 문맥과 달라 **InvalidEpoch**, cache/완료 수 불변. 비로컬 서버 PC의 SubmitLocalCommand는 false | G0 payload에는 PlayerIndex가 없으므로 `NotOwner` 결과나 상대 보드 조작을 검사했다고 주장하지 않음 |
| 전송 손실 | **client PIE NetDriver만 outgoing PktLoss100**을 설정한0.4초 동안 번호2의 서버 dispatch0·pending 유지 | NetDriver 설정을 구조체 전체로 저장·복원. 서버 driver·전역 console 설정은 변경하지 않음 |
| 지연/외래 응답 | pending 번호2에 서버가 보낸 다른 MatchId 성공701·다른 epoch 성공702는 무시, 동일 문맥 Pending703만 통지·pending 유지 | 명시적인 **서버 Client RPC 주입 fixture**이며 G0 제품이 Pending을 생성했다고 기록하지 않음. 실제 수신 event701~703을 먼저 확인 |
| 회복·동일 번호 | packet 설정 복원 후 RetryPending도 번호2. 실제 서버 dispatch3개·PhaseNotAllowed 응답3개, 새 terminal 완료1회·총cache2개 | Reliable 전송 재시도와 애플리케이션 Retry를 함께 관측. 응답 rate-limit 토큰을 의도적으로 소진하지 않음 |
| 별도 응답 제한 | 전송 설정 복원 뒤 같은 client 업데이트에서 완료된 번호2의 실제 RPC32회(Burst12 초과)→새 SubmitLocalCommand 번호3. 서버가 번호3을 실제 처리해 cache3이 되었지만 **번호3 SendRPC 시도0·client Pending 유지·terminal2회**여야 함 | 서버 NetDriver.SendRPCDel을 읽기 전용 관측해 transport에 보내기 전 응답 제한을 구별. 직접 토큰/시계 변경이나 메시지 차단 없음. 실제 제한을 관측하지 못하면 실패 |
| 응답 제한 후 재시도 | 실제 제한 관측 뒤0.3초(8회/초 기준 최소2.4토큰) 대기→RetryPending 번호3. 서버 번호3 실제 dispatch2회·처음 저장한 PhaseNotAllowed 전달·cache3 유지·terminal 총3회 | 단순히 클라 응답이 늦은 것을 제한으로 판단하지 않음. 첫 처리 뒤 cache3/송신0과 이후 송신·실제 수신·완료를 모두 확인 |
| canonical 전용 종료 | 응답 예산을 추가0.5초 충전한 뒤 공개 Mode.AbortMatch→양쪽 Aborted. 동일 번호3은 모든 필드가 기존 응답과 동일, 내용만 바꾼 번호3은 RequestIdConflict, 새 번호4는 PhaseNotAllowed. cache4·terminal 완료 총3회 | 실제 raw RPC로 확인하므로 pending 없음 때문에 UI가 무시한 응답도 보존. canonical 제품에서만 컴파일 |
| 정리 | PIE World0, 원래 Play 설정·해당 client packet 구조체·observer 복원 | packet 설정 API의 내부 강제 적용 상태는 해당 임시 NetDriver가 PIE 종료로 제거됨. 다른 세션에는 적용하지 않음 |

관측은 기존 `AActor::ProcessEventDelegate`와 해당 서버 NetDriver의 `SendRPCDel`이 unbound일 때만 설치하고, 해당 두 PC와 RPC 이름만 읽는다. ProcessEvent에서는 항상 false를 반환하고 SendRPC의 차단 인수를 변경하지 않아 제품 dispatch를 가로막거나 결과를 바꾸지 않는다. 실제 server 수신·서버 응답 송신 시도·실제 client 수신을 구분하고 종료 전에 제거한다. 기존 observer를 덮어쓰지 않으며 실행 중 외부에서 교체된 observer도 지우지 않는다.

## 남은 검수와 한계

- 현재는 코드·엔진 헤더 대조와 형식 검사만 완료했다. **Editor compile / automation / 실제 PIE / canonical 종료 변형은 모두 NotRun**이다.
- 100% client outgoing packet loss와 응답 제한은 별도 단계다. 후속 응답 제한 단계는 실제 수신·cache 확정 후 서버 송신 시도0이 관측되어야 한다. 서버 프레임 분할·지연으로 토큰이 먼저 충전돼 새 응답이 송신되면 **실패**하며,32회를 보냈다는 정적 사실로 통과시키지 않는다. 서버 수신 프레임/시간·송신 시도 목록을 보존해 필요 시 해당 PIE 전송 단계만 조정한 새 실행으로 확인한다. 고정 시각 수치 경계는 기존 G0 Processor 자동화와 구분한다.
- 실제 네트워크 월드 두 개이지만 서로 다른 OS 프로세스 패키지 검사는 아니다. BP 연결, UI, 데이터 cook, Android 실기기, G2 경제·전투, G3 승패·부하는 검수 범위 밖이다.
- 독립 B의 종료 API를 늘리지 않는다. canonical 변형이 실패하면 원래 B 수업의 결과와 합쳐 Pass로 만들지 않는다. 원래 역할 소스·실행 입력·새 fixture SHA를 함께 판정한다.
