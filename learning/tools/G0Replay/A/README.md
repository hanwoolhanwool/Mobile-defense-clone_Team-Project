# 독립 G0 A의 실제 PIE 검사용 제공 코드

상태는 **작성 완료 / 실행 NotRun**이다. 참고 수업 및 학습자 상태를 변경하지 않는다. 제품 소스는 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`, 공통 출발점은 `8c6856d235de87cc28c12b49ca775bd0937334a5`다. 이 파일은 학습자가 직접 작성할 제품 코드가 아니라 독립 재현 검사를 위한 제공 코드다.

기존 `C:/Users/iam12/P0_lesson_replay_a/Saved/P0Runs/Replay-G0-A-assembly/assembly.json`의21파일을 추가 전 대조하여 모두 같은 blob임을 확인했다. 추가 후 원래 manifest에서 달라지는 파일은 Editor 전용 의존을 더한 Build.cs뿐이다. Core/Data/기존 LDDataTests와 제공 JSON은 변경하지 않는다. 기존 DefaultEngine.ini의 manifest 밖 엔진 부수효과는 보존하며 값은 증거에 노출하지 않는다.

## 설치·빌드

1. 공통 `Replay-P0Lesson.ps1 -Role A`로 조립한 새 경로 또는 원본21파일이 여전히 일치하는 독립 A 재현본을 사용한다. 현재 G3나 canonical 제품 코드로 교체하지 않는다.
2. 이 폴더의 `LDG0PieTests.cpp`를 대상 `Source/Mobile_defense_clone/Tests/LDG0PieTests.cpp`로 복사한다. 검사 파일의 해시를 설치 manifest에 별도로 남긴다.
3. 대상 `Source/Mobile_defense_clone/Mobile_defense_clone.Build.cs`의 기존 Json 의존 아래에 다음 조건부 의존만 추가한다. 제품 Core/Data API는 변경하지 않는다.

```csharp
if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "SlateCore" });
}
```

4. 통합 담당자가 현재 빌드/에디터/패키지 작업 종료 후 대상 프로젝트의 Editor를 한 번 빌드한다. 원본 로그를 별도 RunId로 보존한다. 검사 컴파일 오류를 제품 API 공개나 최신 제품 코드 복사로 우회하지 않는다.

## 실행 순서와 입력

GPU `UnrealEditor.exe`에서 각 필터를 별도 새 RunId로 실행한다. NullRHI로 실행하지 않는다. 기존 PIE 세션과 같은 증거 폴더가 있으면 실패하며 덮어쓰지 않는다.

| 대상 입력 | 필터 | 검사 범위·독립 기대 |
|---|---|---|
| 원본21파일 + 위 제공 검사/Editor 의존 | `LD.PIE.G0.A.MatchLifecycle` | 실제 listen/client2 World, native A Mode/base PlayerController, Preparing·동일 MatchId·owner0/1·서로 다른 epoch·타인 private 문맥 비복제·명령 Stub 닫힘. 공개 동일 InitGameState/InitializeMatch/InitializeParticipant/PostLogin을 반복해 원본 보존. 실제 RequestLateJoin으로 셋째 NetConnection/PostLogin→Logout/Destroy와 기존2슬롯 보존. Result 복제/역전이 거절, EndPlay2회/Mode-bound 검사 타이머 제거. PIE 종료→새 PIE 매치 GUID→Aborted 복제/종료. |
| **별도 신규 fixture**: 원래 제공 입력 중 `Content/LD/Data/GameRules.json`만 처음부터 조립하지 않음. 기존 재현본/제품 파일을 삭제·이동·변조하지 않음 | `LD.PIE.G0.A.MissingData` | 실제 Mode.InitGameState가 로딩 실패→Aborted. 양쪽 구체적 GameRules.json 오류 일치, 로더 미게시, 명령 닫힘, Running/Preparing 역전이 거절, 반복 EndPlay/PIE World0. 정상 입력에서 잘못 실행하면 전제 검사로 Fail. |

인수는 기존 실행 도구와 호환된다.

```text
-unattended -nosound -nosplash -RenderOffscreen -culture=ko
-P0PIERun=<새 실행 ID>-proof
-ExecCmds="Automation RunTests LD.PIE.G0.A.MatchLifecycle"
-TestExit="Automation Test Queue Empty"
-ReportExportPath=<새 보고서 폴더>
-abslog=<새 engine.log 경로>
```

누락 입력 fixture는 ExecCmds의 필터만 `LD.PIE.G0.A.MissingData`로 바꾼다. root의 공통 실행 도구가 엔진·프로젝트·로그·RunId를 지정하며 포트와 기기는 직렬 관리한다. 이 제공 코드 자체는 포트나 기본 맵/프로젝트 설정을 변경하지 않는다.

두 필터 모두 `Saved/P0Runs/<새 실행 ID>-proof/pie-proof.json`을 저장한다. `result`, `settingsRestored`, `processId`, `observations`는 G3 PIE 증거 형식과 같고, `completedSessions`, `missingDataFixture`, `thirdRemoteRejected`를 추가한다. 양쪽 실제 창 PNG도 같은 폴더에 저장한다. 정상 검사는 세션2개, 누락 검사는1개 완료 및 `settingsRestored=true`가 필요하다. 원래 Editor play config를 보관하고 끝나면 전체 config 프로퍼티를 복원해 동일성을 검사한다.

## 관찰과 한계

- `/Game/TopDown/Lvl_TopDown`에 **요청 한정 native ALDGameMode override**를 쓴다. 맵은 저장하지 않는다. 수업에서 허용한 native 경로의 재현이며 BP_GameMode/BP_GameState 생성은 검증하지 않는다.
- G0에는 전장·전투·명령 성공·HUD가 없다. PNG는 실제 창 기록이며 Phase/문맥/오류의 통과 근거는 해당 World의 실제 복제값과 OnRep 관찰이다. Result/두 번째 세션 Aborted는 공개 GameState API를 통한 명시적인 전이 fixture이지 자연 플레이 승패가 아니다. 누락 입력의 Aborted만 실제 로딩 실패 경로다.
- 셋째 접속은 임의 Controller 생성이 아니라 `GEditor->RequestLateJoin()`이다. 예상된 ConnectionLost/FailureReceived 엔진 로그만 허용하며 별도 NetworkFailure event가 **셋째 PIE World에만 속하는지** 검사한다. 원래 두 참가자의 이탈/네트워크 실패는 검사 실패다. 실제 실패 로그가 다른 형태라면 원본을 보존하고 원인을 확인한 뒤 검사기만 수정한다.
- 첫 정상 세션에서 의도적으로 등록한 Mode-bound60초 타이머가 변경하지 않은 제품 EndPlay에 의해 제거되는지 검사한다. 누락 입력에서는 이미 서비스가 닫혔으므로 종료 뒤 새 타이머를 넣지 않는다.
- B의 게임 명령 RPC, canonical G0 통합, 최종 PC 패키지, 물리 입력·Android, Blueprint 생성은 이 검사의 범위가 아니다. 해당 검수를 완료로 승계하지 않는다.

엔진 실행 뒤 원래 제품 blob 유지, 새 검사/의존 해시, 빌드·자동화·PIE 결과와 실패 원인을 수업 증거에 별도로 연결해야 한다. 실행 전에는 이 제공 코드만으로 수업을 Verified로 바꾸지 않는다.
