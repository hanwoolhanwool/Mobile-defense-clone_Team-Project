# G0 실제 접속 검사의 공통 실행 절차

이 폴더는 **검사용 제공 코드**다. 런타임 프로젝트는 이 학습 폴더를 로드하지 않는다. A/B의 독립 제품 코드를 최신 통합 코드로 바꾸지 않고 실제 Editor PIE 접속·RPC·종료 경계를 확인한다. 아직 실행 전인 검사는 NotRun이며, 아래 도구를 설치했다고 수업이나 학습자가 완료되는 것이 아니다.

| 재현 입력 | 제품 기준 | 제공 검사 | 필터 |
|---|---|---|---|
| A 공통 출발점 조립21파일 | `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6` | [A 설치·독립 기대](A/README.md), 검사 `fdc12b7e881e231c846c42baa6b2c52360003f52` | `LD.PIE.G0.A.MatchLifecycle` |
| A 별도 누락 입력20파일 | 같은 A 제품, GameRules.json을 처음부터 제공하지 않음 | 같은 A 검사 | `LD.PIE.G0.A.MissingData` |
| B 공통 출발점 조립27파일 | `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` | [B 설치·독립 기대](B/README.md), 검사 `5f1f08606f87e264b5ff0a12d957dbc7720f7e6c` | `LD.PIE.G0.B.OwnedRPC` |
| G0 canonical detached checkout | `649c1dedd6832c41089a76b59bc76518cd262296` | 같은 B 제공 검사, Editor에서만 `LD_G0_CANONICAL_PIE=1` | `LD.PIE.G0.Canonical.TerminalCache` |

기존 `[A/B] replay`를 사용할 때는 원래 assembly manifest의 파일 blob을 먼저 대조한다. 변경됐다면 해당 파일을 덮어쓰지 말고 새 detached worktree에서 [공통 조립 도구](../Replay-P0Lesson.ps1)를 사용한다. `learn/*` 브랜치에 검사용 완성 코드나 참고 구현을 병합하지 않는다. canonical은 위 고정 커밋에서 별도의 새 detached worktree를 만든다.

누락 입력은 기존 정상 재현본에서 파일을 지워서 만들지 않는다. 공통 출발점 `8c6856d235de87cc28c12b49ca775bd0937334a5`의 새 깨끗한 detached worktree에 다음 옵션을 준다. 도구는20파일만 복원하며 의도한 누락을 manifest에 기록한다. 정상 입력 검사를 생략하는 것은 이 명시 fixture에만 적용한다.

```powershell
& "$ReferenceRoot/learning/tools/Replay-P0Lesson.ps1" `
  -ProjectRoot $NewMissingReplay -Role A -RunId G0-missing-assembly -MissingRulesFixture
```

A/B 각 설치 안내에 따라 `LDG0PieTests.cpp`를 대상의 `Source/Mobile_defense_clone/Tests`에 복사하고 Build.cs에 **Editor 조건부** `UnrealEd`·`SlateCore` 의존만 추가한다. canonical에는 같은 조건에서 `PrivateDefinitions.Add("LD_G0_CANONICAL_PIE=1");`도 추가한다. 제품 Core/Data/Network와 기존 테스트를 수정하지 않는다. 추가 전 원본 blob, 설치 후 제공 cpp와 Build.cs의 SHA256을 별도 설치 기록에 남긴다.

빌드·Editor·패키지·성능 검사가 겹치지 않도록 통합 담당자가 실행 자원을 직렬로 사용한다. 각 대상은 전체 조립과 제공 검사 설치가 끝난 뒤 Editor를 한 번 빌드하고 해당 PIE 필터를 한 번 실행한다. 정상 A 필터 안에서는 엔진이 두 PIE 세션을 순서대로 시작·종료한다. 기존 값 자동화4/12종은 제품 변경 없이 반복할 필요가 없다.

```powershell
& "$ReplayRoot/tools/Build-P0Editor.ps1" -RunId G0-PIE-editor
& "$ReferenceRoot/learning/tools/Test-P0G0PIE.ps1" `
  -ProjectRoot $ReplayRoot -RunId G0-actual-PIE -Filter LD.PIE.G0.A.MatchLifecycle
```

예시 RunId가 있으면 새 이름을 사용한다. 실행기는 GPU Editor를 별도 프로세스로 열고 `Saved/P0Runs/<RunId>/`에 전체 로그·Automation 최종 보고서, `<RunId>-proof/`에 관찰 JSON·화면을 보존한다. proof가 먼저 작성된 뒤 엔진이 예상 오류 횟수를 최종 판정하므로 **프로세스 exit0, 최종 Automation 성공1/실패0, proof Pass, 설정·observer·packet 복원**을 모두 요구한다. proof 한 파일만으로 Pass를 선언하지 않는다. 컴파일 오류·실제 실패는 입력과 로그를 보존하고 제공 검사 또는 제품의 어느 문제인지 구분해 수정한다.

검사는 native GameMode Override를 쓰는 문서상 허용 경로다. BP 변형 제작/저장, 게임 전장·전투·UMG, 별도 패키지2프로세스, Android 실기기 검수를 대신하지 않는다. B의 외래 응답/Pending 응답은 명시적 서버 RPC 주입 fixture이며 제품이 Pending을 생성했다고 기록하지 않는다. 실제 전송 손실과 응답 예산 고갈은 다른 단계와 증거로 남긴다.
