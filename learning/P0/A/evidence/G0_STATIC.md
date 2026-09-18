# A G0 정적 검사

- 작업 경로: `C:/Users/iam12/P0_reference_a`
- 브랜치: `reference/p0-a`
- 출발점: `8c6856d235de87cc28c12b49ca775bd0937334a5`
- 코드 기준: `0bab1ad7241444fa73ce2ca41dbd8863046351dd`
- 실행일: 2026-09-18

실행 명령:

```powershell
$env:CLANG_FORMAT='C:/Users/iam12/Mobile_defense_clone/Saved/Tooling/code-style/Scripts/clang-format.exe'
$sourceFiles = @(rg --files Source/Mobile_defense_clone/Data Source/Mobile_defense_clone/Core Source/Mobile_defense_clone/Tests)
node tools/check-code-style.mjs --fix @sourceFiles
node tools/check-code-style.mjs
git diff --check
```

관찰한 도구 출력:

```text
Formatted 10 explicit file(s). Review the diff and run the check again.
Code style: 11 checked (includes formatting sample), 48 unchanged legacy files, 0 errors.
```

`git diff --check` 출력 없음, 명령 exit code 0. 이 파일은 짧은 실제 출력의 기록이며 전체 Unreal 로그가 아니다. Unreal 컴파일·Automation 실행·PIE·PC 패키지·네트워크·Android·학습 재현 결과는 모두 **NotRun**이다. 에디터와 빌드는 동시 실행하지 않고 통합 담당자가 별도로 수행한다.

학습 초안 작성 뒤 `node tools/check-project.mjs`도 exit code 0이었다. 전체 출력은 [G0_PROJECT_CHECK.log](G0_PROJECT_CHECK.log)에 보존했다. 데이터 검사2689건·기획 검사6601건·코드 서식11개가 통과했다. 데이터 검사에 나타나는 전체80웨이브 합계는 기획 데이터 정적 검사이며 P0 10웨이브 실행 결과가 아니다.

정적 리뷰에서 정책 변경을 같은 RulesVersion으로 통과시키는 누락을 발견하여 ContinuousSeconds/CountedEnemyKinds/Timing.Order/합성·부분 뭉치/허용 목록 검사와 실패 변조 테스트를 추가했다. 좌표가 유한수인지만 확인하던 구현에 보드 간격·분리·경로 둘레 검사를 추가했다. 이러한 코드 수정은 실제 테스트 통과 증거와 구분한다.
