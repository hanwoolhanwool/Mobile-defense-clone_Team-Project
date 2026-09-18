# A G0 Unreal 컴파일과 자동화

직접 읽어 확인한 실행 HEAD는 `0efd2bb8c32f99e5814d6aa4d4891ee9d5e96c69`이며, 코드 수정은 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`다. 실행은 통합 담당자가 자원을 직렬 사용하여 수행했다. A는 아래 실제 결과 파일과 로그를 직접 확인했다.

| 범위 | 결과 | 실제 증거 |
|---|---|---|
| A Editor 재빌드 | Pass, exit0, 19.26초 | [결과](G0_EDITOR_RESULT.json), [전체 빌드 로그](G0_EDITOR_BUILD.log) |
| Unreal NullRHI 자동화 | 4Pass / 0Fail / 0NotRun, exit0 | [결과](G0_TEST_RESULT.json), [테스트·변조 결과 발췌](G0_TEST_EVENTS.log) |
| PIE·게임 화면·패키지·네트워크·Android | NotRun | 이 실행 범위에 포함하지 않음 |
| 수업 시작점부터 재현 | NotRun | 참고 Draft / 실제 학습 Planned 유지 |

환경: Windows, UE5.8, `Mobile_defense_cloneEditor` Win64 Development, VS14.50.35738/MSVC 도구, Windows SDK10.0.26100.0. `-MaxParallelActions=4`. 자동화는 `-unattended -NullRHI -nosound -nosplash -culture=en`, 필터 `LD.P0.G0`로 실행했다. 빌드 시간은 증분 빌드 경과시간이며 게임 성능 수치가 아니다.

테스트 네 개의 실제 결과는 다음과 같다.

1. `InvalidRowsAndMissingFiles`: 없는 보스 참조/파일 실패, 뒤늦은 실패 시 부분 게시0, 올바른 경로 재시도 Pass.
2. `ParticipantIdentity`: 빈 문맥/음수·범위 밖 보드/epoch0 거절 Pass.
3. `RejectChangedContractsAtomically`: 14개 규칙 변조의 거절·진단·원본 미게시 Pass. SchemaVersion, RulesVersion, EnableSkills, ContinuousSeconds, Timing.Order, MergeUsesSelectedCell, MaxPartialStacksPerUnitPerArea, AllowedGrades, MergeInputGrades, AllowedNormalEnemyIds, CountedEnemyKinds, 중복 X 좌표, 겹치는 Y 보드, Economy 누락을 포함한다.
4. `ValidP0Snapshot`: 현행 P0 값·16종·10웨이브·비활성 행 제외·중복 로딩 보존 Pass.

전체 자동화 로그·보고서는 `C:/Users/iam12/P0_reference_a/Saved/P0Runs/G0-A-tests/engine.log`, `report/index.json`에 보존했다. 전체 첫 실패 빌드 로그는 `Saved/P0Runs/G0-A-editor/build.log`, 수정 성공 로그는 `Saved/P0Runs/G0-A-editor-fix1/build.log`다. JSON 결과에는 실행 인자·시각·HEAD·종료코드가 들어 있다.

초기 코드의 두 TEXT 포인터 덧셈은 실제 컴파일이 검출했고 FString 연결로 수정하여 재빌드 성공했다. 초기 정적 검사 기록은 보존하며 그 시점의 NotRun을 과거 실행 성공으로 바꾸지 않는다. 정책·좌표 검사는 정적 리뷰로 보완한 뒤 이번 실제 자동화가 통과했다.

에디터가 `Config/DefaultEngine.ini`에 생성한 SecurityToken 줄은 값 출력 없이 출발점의 원래 주석으로 복원했다. 사용자 `UserEngine.ini`는 읽거나 수정하지 않았고 Config는 이 문서 커밋에 포함하지 않는다.

결과 동기화 뒤 `node tools/check-project.mjs`를 다시 실행하여 exit0을 확인했다. [전체 정적 검사 로그](G0_POSTRUN_PROJECT_CHECK.log). `git diff --check`도 통과했다.
