---
id: TECH-CODE-STYLE
version: 0.1.0
status: Baseline
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-04
applies_to: 로컬·GitHub CI 코드 포맷 검사와 기존 템플릿 이행
baseline_basis: DEC-025 사용자 코드 규약·포맷 설정·자동 검사 작성 지시
---

# 코드 스타일 설정과 자동 검사

[작성 규약](CODING_STANDARD.md) · [이름·폴더·데이터](NAMING_AND_STRUCTURE.md) · [관리 방식](../WORKFLOW.md)

개발자는 파일을 편집한 뒤 아래 검사를 실행하고, GitHub CI는 같은 검사를 읽기 전용으로 반복한다. CI가 코드를 고치거나 자동 커밋하지 않는다.

## 1. 도구와 적용 범위

| 설정·검사 | 적용 |
|---|---|
| [.editorconfig](../../.editorconfig) | UTF-8·LF·끝 개행, C++/C# 탭 4칸, 그 외 기본 공백 2칸. Markdown 끝 공백과 보관 증거·바이너리는 별도 처리 |
| [.clang-format](../../.clang-format) | C++ Allman·탭 4칸·120칸 목표·포인터 표기·UE 매크로 처리. include 순서를 재정렬하지 않음 |
| [C++ 검사](../../tools/check-code-style.mjs) | `Source/` 아래 `.h`, `.hpp`, `.cpp`, `.cc`, `.cxx`, `.inl` 전체 탐색. 신규·변경 파일은 파일 전체를 포맷 결과와 비교 |
| [서식 샘플](../../tools/style/LDStyleExample.h) | 항상 검사. UE 없이도 리플렉션 매크로 설정의 서식 확인 |
| [기존 파일 기준](../../tools/style/legacy-baseline.json) | 도입 당시 C++ 48개 파일의 경로·내용 해시가 모두 같을 때만 기존 서식 허용 |
| [통합 검사](../../tools/check-project.mjs) | 문서·생성 데이터·증거 검사에 C++ 스타일 검사 추가 |

clang-format **20.1.8**로 로컬/CI 버전을 고정한다. Visual Studio에 내장된 다른 버전이 설치되어 있어도 자동 대체하지 않는다. 설치가 없거나 버전이 다르면 실패한다. 포맷 설정 옵션과 명령은 [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) 및 [Style Options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)를 참고하고 변경할 때 고정 바이너리로 확인한다.

EditorConfig는 지원하는 편집기에서 저장 서식을 맞추는 설정이며 모든 언어의 자동 검사기는 아니다. 저장소 전체를 재저장하지 않는다. 특히 `docs/production/evidence/`는 검증 해시를 위해 원래 바이트를 보존한다. 근거: [EditorConfig](https://editorconfig.org/).

## 2. Windows 로컬 설치

저장소 루트에서 Python 3와 Node.js 24.15.0을 사용한다. Python은 formatter 설치용이며 UE 빌드 도구 버전과 별개다. 아래 venv와 설치 파일은 Git에서 제외되는 `Saved/` 안에 둔다.

```powershell
python -m venv Saved/Tooling/code-style
if ($LASTEXITCODE -ne 0) { throw 'venv 생성 실패' }
& .\Saved\Tooling\code-style\Scripts\python.exe -m pip install --disable-pip-version-check --require-hashes --only-binary=:all: --no-deps -r tools/style/requirements.txt
if ($LASTEXITCODE -ne 0) { throw 'formatter 설치 실패' }
node tools/check-code-style.mjs
```

`python`이 없거나 Windows Store 별칭만 있다면 실제 Python 3 실행 파일의 절대 경로로 첫 명령을 실행한다. `Activate.ps1` 실행은 필요 없다. 검사기가 위 venv의 실행 파일을 기본 사용한다. 별도 설치를 사용할 때만 `$env:CLANG_FORMAT = '실제 clang-format.exe 절대 경로'`를 설정한다. 그 경우에도 20.1.8 확인은 생략되지 않는다.

설치 패키지는 [clang-format-wheel](https://github.com/ssciwr/clang-format-wheel)이 제공하는 바이너리이며, [requirements.txt](../../tools/style/requirements.txt)에 Windows x64·Linux x64 wheel의 버전과 SHA-256을 고정했다. 다른 OS/아키텍처를 추가할 때는 해당 배포의 해시·실행 검증을 함께 추가한다. 시스템 LLVM이나 Visual Studio 설치를 변경하지 않는다.

## 3. 검사와 수정

```powershell
# 읽기 전용: 전체 프로젝트 검사
node tools/check-project.mjs

# 읽기 전용: C++ 포맷만 빠르게 확인
node tools/check-code-style.mjs

# 도구 변경 시: 정상·오류 입력 테스트
node --test tools/validate-planning.test.mjs tools/check-code-style.test.mjs
```

서식 오류가 있으면 출력된 실제 파일 경로를 `--fix`에 전달한다. 예를 들어 `LDUnitActor.h/.cpp`를 만든 뒤에는 다음처럼 실행한다. 아직 없는 예제 경로를 생성하는 명령은 아니다.

```powershell
node tools/check-code-style.mjs --fix Source/Mobile_defense_clone/Battle/LDUnitActor.h Source/Mobile_defense_clone/Battle/LDUnitActor.cpp
git diff --check
git diff -- Source/Mobile_defense_clone/Battle
node tools/check-code-style.mjs
```

`--fix`는 명시한 파일만 수정한다. 폴더·와일드카드·Source 밖 경로는 허용하지 않으며 예외는 전용 서식 샘플뿐이다. 수정 결과에서 include 순서·매크로·주석을 확인한다. 포맷 차이가 큰 기존 파일은 같은 PR에서 포맷 전용 커밋과 기능 변경 커밋을 구분할 수 있다.

## 4. 기존 템플릿 이행 규칙

기준 커밋 `ed4239321aae2900641a17bd7a1bc39c2307e855`의 원래 C++ 파일 48개를 [해시 목록](../../tools/style/legacy-baseline.json)에 기록했다. 비교 시 Git의 Windows 체크아웃 차이를 위해 CRLF만 LF로 정규화한다. 다른 공백·코드·주석 변경은 모두 변경으로 취급한다.

- 경로와 내용이 같으면 기존 서식을 보존한다. 현재 템플릿 전체가 새 규약을 통과했다는 뜻은 아니다.
- 파일을 조금이라도 수정하거나 이름을 바꾸면 해당 파일 전체가 검사 대상이 된다. 새 파일은 항상 검사한다.
- 파일 삭제는 허용한다. 남은 기준 목록 항목은 새 파일 경로에 효력이 없다.
- CI 통과를 위해 기준 해시를 새 내용으로 갱신하거나 예외 파일을 추가하지 않는다. 기준 목록 변경이 필요하면 별도 정책 변경으로 이유와 범위를 리뷰한다. 자동 갱신 명령은 제공하지 않는다.
- `clang-format off` 주석으로 새 코드의 서식을 숨기지 않는다. 포맷 도구 업그레이드는 버전·wheel 해시·설정·샘플·테스트를 같은 변경에서 검증한다.

## 5. GitHub CI와 자동화 한계

[워크플로](../../.github/workflows/documentation.yml)는 PR 생성·새 커밋 push·재오픈, main push, 기본 브랜치 등록 후 수동 실행에 동작한다. 경로 필터는 없다. `documentation` 작업에서 해시가 고정된 formatter를 설치한 뒤 통합 검사와 두 테스트 파일을 실행한다. 기존 main의 필수 검사 이름을 유지하여 스타일 위반도 병합을 차단한다. 구체적인 보호 설정과 승인 수는 [관리 방식](../WORKFLOW.md)을 따른다.

| 자동 확인 | 별도 리뷰·실행 확인 |
|---|---|
| C++ 포맷·새 파일 UTF-8 BOM/개행, 고정 버전, 기준 해시 | 클래스 책임·이름의 의미·폴더 배치 |
| 기존 데이터 검사기의 필드·범위·참조 조건 | 신규 스키마 설계·원작 일치·밸런스·UE 임포트 |
| 문서 링크·메타데이터·기록/증거 정합성 | UHT·컴파일·include 충분성·중괄호 누락·메모리 수명 |
| 검사기의 정상 입력·위반 입력 거절 테스트 | Blueprint 그래프·에셋 이름·참조·권한·PIE·패키징·모바일 성능 |

현재 clang-tidy, Unreal 빌드 CI, Blueprint/에셋 전수 검사는 구현하지 않았다. 필요하면 별도 작업에서 엔진 실행 환경·분석 규칙·판정 기준을 정한다. 이번 도입의 실제 검사 기록은 [RUN-20260914-04](../production/TEST_RUNS.md)에 남긴다.
