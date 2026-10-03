# 두 개발자를 위한 Git · 프로젝트 사용 안내

**함께 볼 곳은 `reference/p0-integration`, 직접 만들 곳은 `learn/p0-a`와 `learn/p0-b`, 학습 결과를 합칠 곳은 `learn/p0-integration`입니다.** 두 개발자가 저장소를 따로 만들 필요는 없습니다. 같은 GitHub 저장소를 각자의 PC에 내려받고, 참고용 프로젝트와 개발용 프로젝트를 다른 폴더에서 엽니다.

이 문서는 2026-10-03에 실제 로컬 브랜치·worktree와 원격 브랜치를 확인하여 작성했습니다. 아래 원격 게시·새 PC 설치·학습 병합은 **앞으로 팀이 실행할 절차**입니다. 이 안내 제작 과정에서는 GitHub에 올리거나 학습 브랜치에 완성 코드를 넣지 않았습니다. 현재 참고본은 G0~G3의 명시된 PC 검증 범위를 확인한 구현이며, 실제 청음·Android 실기기 검수가 남아 P0 최종 완료 상태는 아닙니다.

> 먼저 두 사람이 같은 참고본으로 게임과 코드를 확인하세요. 수업은 참고 폴더의 HTML에서 읽고, 코드는 자기 학습 폴더에서 작성합니다. 학습 출발점에는 완성된 게임과 최신 HTML이 들어 있지 않는 것이 정상입니다.

## 1. 지금 필요한 일부터 고르기

| 지금 하려는 일 | 이동할 곳 | 준비 사항 |
|---|---|---|
| 이 PC에서 현재 게임·코드 확인 | [현재 PC의 폴더](#current-pc) → [게임 열기](#run) | 기존 통합 참고 폴더 사용 |
| 다른 개발자의 PC로 참고본 전달 | [담당자의 첫 공유](#publish) → [각 PC의 첫 설정](#new-pc) | 참고·학습 6개 브랜치를 원격에 게시해야 함 |
| A 또는 B가 직접 구현 시작 | [각 PC의 첫 설정](#new-pc) → [매일 작업](#daily) | 자기 역할의 learn 브랜치와 별도 폴더 |
| A/B 결과를 하나의 게임으로 연결 | [리뷰와 통합](#integration) | 같은 게이트의 계약·실행 증거·상호 리뷰 |
| 에셋 충돌·잘못된 폴더가 걱정됨 | [파일 관리](#files) → [문제 해결](#troubleshooting) | 작업 전 브랜치와 변경 파일 확인 |

<a id="snapshot"></a>

## 2. 현재 저장소 상태

공유 대상은 [Mobile-defense-clone_Team-Project](https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project) 하나입니다. 저장소 주소는 다음과 같습니다.

```text
https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project.git
```

| 브랜치 | 확인한 위치 | 확인 당시 상태 | 앞으로의 용도 |
|---|---|---|---|
| `reference/p0-integration` | 로컬 | `cbd65cd` | 두 사람이 함께 확인할 통합 참고본 |
| `reference/p0-a` | 로컬 | `cbd65cd` | A 참고 구현 이력·게이트별 비교 |
| `reference/p0-b` | 로컬 | `cbd65cd` | B 참고 구현 이력·게이트별 비교 |
| `learn/p0-a` | 로컬 | `8c6856d` | A가 수업을 따라 직접 작성 |
| `learn/p0-b` | 로컬 | `8c6856d` | B가 수업을 따라 직접 작성 |
| `learn/p0-integration` | 로컬 | `8c6856d` | 두 사람이 작성한 결과만 통합 |
| `docs/operations-baseline` | 로컬 / 원격 | 로컬 `8c6856d`, 원격 `a931387` | 원래 프로젝트·기획 출발점 보존 |
| `main` | 로컬 / 원격 | `c366829` | 검증·리뷰한 제품 변경의 최종 반영 대상 |

**읽기 전용 원격 조회 결과, reference·learn 6개 브랜치는 아직 GitHub에 없습니다.** 지금 다른 PC에서 기본 clone만 하면 현재 참고본을 볼 수 없습니다. 표의 원격 상태는 작성 시점의 관찰이며, 공유 후에는 아래 조회로 다시 확인합니다. 원격 접근 권한·보호 규칙은 이번에 변경하거나 재검증하지 않았습니다.

```powershell
git ls-remote --heads origin main docs/operations-baseline reference/p0-a reference/p0-b reference/p0-integration learn/p0-a learn/p0-b learn/p0-integration
```

기준점은 전체 SHA로 기록합니다. 브랜치는 새 커밋이 생기면 이동하지만 SHA는 특정 시점의 파일을 가리킵니다.

| 구분 | 전체 SHA | 해석 |
|---|---|---|
| 학습 공통 출발점 | `8c6856d235de87cc28c12b49ca775bd0937334a5` | learn 세 브랜치의 시작점 |
| 안내 작성 전 참고본 | `cbd65cd54296c69812a5752522a0ce4f015b2b05` | 기존 HTML 학습실까지 포함. 이 안내 커밋 후 참고 브랜치의 HEAD는 달라짐 |
| 최신 팔레트 PC 패키지의 소스 입력 | `65b7feebceb3b2aa94e1b191a010038706369d79` | [빌드·실행 증거](../docs/production/evidence/RUN-20260928-08/SUMMARY.md)로 식별. HTML 문서 커밋을 게임 재빌드 증거로 사용하지 않음 |

현재 reference A/B/integration의 끝점은 같습니다. A와 B가 서로 다른 완성 게임을 쓰는 구조가 아닙니다. 역할별 독립 구현과 통합 선택은 커밋 이력과 [G0 통합 기록](P0/INTEGRATION.md)에서 비교합니다. 앞으로 학습자가 구현을 시작하면 자신의 learn 브랜치가 출발점에서 진행됩니다. 참고본 전체 병합으로 학습을 대신하지 않습니다.

## 3. 저장소 하나, 용도별 폴더 여러 개

| 용어 | 이 프로젝트에서의 의미 |
|---|---|
| GitHub 원격 저장소 | 서로 다른 PC가 커밋을 주고받는 공유 장소 |
| clone | 다른 PC에 저장소 이력과 파일을 내려받는 작업 |
| branch | 참고본·A 작업·B 작업·통합 결과의 진행 위치 |
| worktree | 같은 PC의 Git 이력을 공유하면서 다른 브랜치 파일을 별도 폴더에 여는 기능 |
| commit / SHA | 검토 가능한 변경 묶음 / 그 시점을 가리키는 식별자 |
| fetch / push | 원격의 새 이력을 받기 / 내 커밋을 원격으로 보내기. fetch만으로 열려 있는 프로젝트 파일은 바뀌지 않음 |

```text
같은 GitHub 저장소
  ├─ reference/p0-integration  ──→ A와 B 모두 읽고 실행
  │     ↑ reference/p0-a, reference/p0-b의 참고 구현 이력
  │
  └─ 공통 출발점 8c6856d
        ├─ learn/p0-a ── A 직접 구현 ──┐
        └─ learn/p0-b ── B 직접 구현 ──┴─→ learn/p0-integration
                                              │
                      검증한 통합 결과를 A/B에 반영해 다음 기능 진행
                                              │
                              제품 반영 시 별도 리뷰·PR → main
```

reference에서 learn으로 완성 코드를 통째로 병합하는 화살표는 없습니다. 수업에 지정된 제공 도구·데이터만 명시된 파일 목록과 SHA로 받습니다. 직접 작성할 코드와의 구분은 [공통 수업](P0/COMMON.md)과 각 수업을 따릅니다.

<a id="current-pc"></a>

## 4. 현재 이 PC에서는 어디를 열어야 하나요?

| 폴더 | 사용할 때 | 주의점 |
|---|---|---|
| `C:/Users/iam12/P0_reference_integration` | 현재 참고 게임·코드·HTML 확인 | 두 개발자의 공통 비교 기준 |
| `C:/Users/iam12/P0_reference_a` | A 참고 구현 이력 확인 | 같은 Git 저장소의 별도 worktree |
| `C:/Users/iam12/P0_reference_b` | B 참고 구현 이력 확인 | 같은 Git 저장소의 별도 worktree |
| `C:/Users/iam12/Mobile_defense_clone` | 원래 기획·기본 프로젝트 확인 | 현재 참고 게임을 여는 폴더가 아님. 보존 |
| `P0_lesson_replay_*`, `P0_g3_net_review` | 과거 재현·검수 입력 보존 | 새 기능 개발 폴더로 사용하지 않음. 일부는 HEAD와 실제 조립 입력이 다름 |

현재 learn 브랜치에는 연결된 worktree가 없습니다. **같은 PC에서 처음 학습 폴더를 만들 때만** 다음을 실행합니다. 기존 폴더 또는 이미 사용 중인 브랜치라면 아래 오류를 해결하고 기존 경로를 사용하세요. 다른 PC의 처음 설정은 다음 절차를 사용합니다.

```powershell
$ReferenceRoot = 'C:/Users/iam12/P0_reference_integration'
git -C $ReferenceRoot worktree list
git -C $ReferenceRoot worktree add 'C:/Users/iam12/P0_learn_a' learn/p0-a
if ($LASTEXITCODE -ne 0) { throw 'A worktree 생성 실패: 기존 연결과 폴더를 확인하세요.' }
git -C $ReferenceRoot worktree add 'C:/Users/iam12/P0_learn_b' learn/p0-b
if ($LASTEXITCODE -ne 0) { throw 'B worktree 생성 실패: 기존 연결과 폴더를 확인하세요.' }
git -C $ReferenceRoot worktree add 'C:/Users/iam12/P0_learn_integration' learn/p0-integration
if ($LASTEXITCODE -ne 0) { throw '통합 worktree 생성 실패: 기존 연결과 폴더를 확인하세요.' }
```

worktree는 한 브랜치를 같은 저장소의 여러 폴더에 동시에 체크아웃하지 못하게 합니다. 현재 reference 브랜치들은 이미 연결되어 있으므로 기존 폴더를 사용합니다. 이 PC의 폴더를 다른 PC와 실시간 동기화하여 공동 편집하지 말고, 각 PC가 clone한 뒤 Git으로 변경을 주고받습니다.

<a id="publish"></a>

## 5. 담당자 한 명이 처음 공유하기

**이 절차는 아직 실행하지 않았습니다. 원격 게시가 결정된 뒤 저장소 담당자가 한 번 수행합니다.** 계정 초대·쓰기 권한도 담당자가 확인합니다. 이 문서를 읽는 것만으로 GitHub 권한이나 브랜치가 만들어지지 않습니다.

1. 참고 A/B/통합의 파일 변경을 검토하고 커밋합니다. 미커밋 파일은 push에 포함되지 않습니다.
2. learn 세 브랜치가 여전히 학습 공통 출발점인지 확인합니다. 누군가 이미 학습을 시작했다면 그 커밋을 보존하고 이 초기 배포 절차를 다시 적용하지 않습니다.
3. 원격을 다시 조회합니다. 아래와 달리 같은 이름의 원격 브랜치가 생겼다면 fetch 후 이력 차이를 검토합니다. 강제 덮어쓰기를 하지 않습니다.
4. 아래 첫 명령은 전송 예정 내용을 확인하는 dry-run입니다. **두 번째 push만 실제 게시**합니다. `--atomic`은 여섯 브랜치가 모두 반영되거나 모두 거절되게 요청합니다. 서버가 지원하지 않으면 실패 원인을 검토하고 멈춥니다.

```powershell
Set-Location 'C:/Users/iam12/P0_reference_integration'
git status --short
git branch -vv
git remote -v
git ls-remote --heads origin 'reference/p0-*' 'learn/p0-*'

# 검토용: 원격 브랜치를 만들지 않음
git push --dry-run --atomic origin reference/p0-a reference/p0-b reference/p0-integration learn/p0-a learn/p0-b learn/p0-integration
if ($LASTEXITCODE -ne 0) { throw '전송 예정 검사 실패. 실제 게시를 진행하지 마세요.' }
```

```powershell
# 실제 공유를 승인·결정한 담당자만 실행
git push --atomic -u origin reference/p0-a reference/p0-b reference/p0-integration learn/p0-a learn/p0-b learn/p0-integration
if ($LASTEXITCODE -ne 0) { throw '게시 실패. 인증·이력·보호 규칙을 확인하세요.' }
git ls-remote --heads origin 'reference/p0-*' 'learn/p0-*'
git rev-parse reference/p0-integration
```

담당자는 마지막의 **통합 참고본 전체 SHA**를 두 개발자에게 전달합니다. 두 사람 모두 내려받은 뒤 이 값과 비교합니다. `main`이나 원래 문서 브랜치를 함께 올릴 필요는 없습니다. `--all`, `--mirror`, 강제 push는 이 절차에 사용하지 않습니다.

<a id="new-pc"></a>

## 6. 개발자 A·B가 자기 PC에 처음 준비하기

먼저 Git, PowerShell 7, UE 5.8.2와 해당 프로젝트의 C++ 빌드 도구를 준비합니다. 저장소 설정은 Visual Studio 2026, MSVC `14.50.35717`, Windows SDK `10.0.26100.0`을 지정하고 있습니다. `.uproject`는 엔진 계열 `5.8`을 가리키므로 설치 패치도 [개발 환경](../docs/technical/DEVELOPMENT_SETUP.md)·[빌드 절차](../docs/technical/BUILD_RUN.md)와 대조합니다. 임의의 최신 엔진으로 에셋을 저장하지 않습니다. HTML 읽기에는 Unreal이나 Node 설치가 필요 없고, HTML 재생성에는 Node.js 20 이상이 필요합니다.

아래 예시는 **새 PC / 존재하지 않는 `C:/P0Team` 폴더**용입니다. A는 `$Role = 'a'`, B는 `'b'`로 지정합니다. 앞 절의 원격 게시가 끝나야 성공합니다. 기본 clone 폴더 `repo`는 Git 관리용이며, 실제 프로젝트는 `reference`와 `learn-a` 또는 `learn-b`에서 엽니다.

```powershell
# GUIDE: fresh-setup
$Role = 'a' # B는 'b'
$TeamRoot = 'C:/P0Team' # 이미 있다면 새 경로 지정
$RepoUrl = 'https://github.com/hanwoolhanwool/Mobile-defense-clone_Team-Project.git'
$BaseSha = '8c6856d235de87cc28c12b49ca775bd0937334a5'
if ($Role -notin @('a', 'b')) { throw '역할은 a 또는 b입니다.' }
if (Test-Path -LiteralPath $TeamRoot) { throw '기존 폴더를 보존하고 다른 새 경로를 지정하세요.' }
function TeamGit {
    & git @args
    if ($LASTEXITCODE -ne 0) { throw 'Git 실패: 바로 위 오류를 확인하세요.' }
}
New-Item -ItemType Directory -Path $TeamRoot | Out-Null
$RepoRoot = Join-Path $TeamRoot 'repo'
$ReferenceRoot = Join-Path $TeamRoot 'reference'
$LearnRoot = Join-Path $TeamRoot "learn-$Role"
TeamGit clone $RepoUrl $RepoRoot
TeamGit -C $RepoRoot fetch origin
TeamGit -C $RepoRoot show-ref --verify refs/remotes/origin/reference/p0-integration
$LearningStart = TeamGit -C $RepoRoot rev-parse "origin/learn/p0-$Role"
if ($LearningStart -ne $BaseSha) { throw '이미 학습이 진행된 브랜치입니다. 담당자와 이력을 확인하고 이어받으세요.' }
TeamGit -C $RepoRoot worktree add --track -b reference/p0-integration $ReferenceRoot origin/reference/p0-integration
TeamGit -C $RepoRoot worktree add --track -b "learn/p0-$Role" $LearnRoot "origin/learn/p0-$Role"
TeamGit -C $ReferenceRoot rev-parse HEAD
TeamGit -C $LearnRoot rev-parse HEAD
TeamGit -C $RepoRoot worktree list
```

마지막 결과는 참고 SHA=담당자가 전달한 값, 학습 SHA=`8c6856d…`여야 합니다. 다르면 구현을 시작하기 전에 어느 버전을 공유할지 맞춥니다. 이미 구현 중인 역할 브랜치를 다른 사람에게 인계할 때에는 출발점으로 되돌리지 않고 해당 역할의 현재 원격 HEAD·진행 기록을 이어받습니다.

커밋 작성자는 각자 자신의 실제 이름과 이메일로 지정합니다. 아래 명령은 해당 clone과 연결된 worktree에 적용되며, 다른 사람 계정을 복사하지 않습니다. 인증은 Git의 로그인 절차를 사용하고 토큰을 저장소 파일이나 주소에 넣지 않습니다.

```powershell
git -C $RepoRoot config user.name (Read-Host '커밋에 사용할 이름')
git -C $RepoRoot config user.email (Read-Host '커밋에 사용할 이메일')
```

예상 폴더 구조는 다음과 같습니다. 다른 PC의 worktree는 서로 독립적이며, A가 만든 커밋을 B가 받으려면 push와 fetch가 필요합니다.

```text
A의 PC                         B의 PC
C:/P0Team/                    C:/P0Team/
  repo/                         repo/
  reference/   ← 공통 참고 →     reference/
  learn-a/     ← A 직접 작성     learn-b/ ← B 직접 작성
  integration/ ← 통합 담당 PC 한 곳에서만 만들기
```

`reference/learning/html/index.html`에서 수업을 읽으면서 `learn-a/Mobile_defense_clone.uproject` 또는 `learn-b/Mobile_defense_clone.uproject`를 개발합니다. **참고 HTML을 보기 위해 reference 전체를 learn에 병합하지 마세요.**

<a id="run"></a>

## 7. 두 사람 모두 참고 게임을 확인하기

### 코드를 읽고 에디터에서 열기

현재 PC에서는 `C:/Users/iam12/P0_reference_integration/Mobile_defense_clone.uproject`, 새 PC에서는 `C:/P0Team/reference/Mobile_defense_clone.uproject`를 엽니다. 폴더 이름뿐 아니라 터미널의 브랜치·SHA도 확인합니다. 에디터는 **게임·맵·UMG를 확인할 때** 필요하며, HTML 읽기와 Git 정리에는 필요하지 않습니다.

새 clone에는 빌드 결과가 없으므로 C++ Editor 빌드를 먼저 합니다. 아래는 **참고 폴더에서만** 실행하는 명령입니다. 학습 출발점에는 이 제공 빌드 도구가 아직 없을 수 있으며, 학습자는 G0 수업의 제공 입력 순서를 따릅니다. 빌드하거나 브랜치의 파일을 바꾸기 전에는 해당 프로젝트의 에디터를 저장하고 닫습니다.

```powershell
$ProjectRoot = 'C:/P0Team/reference' # 현재 PC는 P0_reference_integration 경로
git -C $ProjectRoot status --short
git -C $ProjectRoot branch --show-current
git -C $ProjectRoot rev-parse HEAD
$RunId = 'Team-editor-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
pwsh -NoProfile -File "$ProjectRoot/tools/Build-P0Editor.ps1" -ProjectRoot $ProjectRoot -RunId $RunId
if ($LASTEXITCODE -ne 0) { throw 'Saved/P0Runs의 build.log에서 첫 원인 오류를 확인하세요.' }
Invoke-Item "$ProjectRoot/Mobile_defense_clone.uproject"
```

엔진 설치 위치가 다르면 빌드에 `-EngineRoot '<실제 UE_5.8 폴더>'`를 지정합니다. 정상 참고 프로젝트의 시작 맵은 `Content/LD/Maps/L_P0Entry`, 시작 화면의 버튼은 **호스트 시작 / 주소로 참가**입니다. 예상과 다르면 자동으로 맵을 바꾸기보다 프로젝트 경로·SHA·빌드 성공 여부를 먼저 확인합니다.

### 게임만 먼저 보기: PC 패키지

현재 PC의 최신 팔레트 패키지는 다음 위치에 있습니다. 이는 로컬 산출물이며 Git clone에 포함되지 않습니다.

```text
C:/Users/iam12/P0_lesson_replay_g3/Saved/P0Runs/P0-grade-palette-package/Package/Windows/Mobile_defense_clone.exe
```

다른 PC에 패키지를 전달할 때에는 **Windows 폴더 전체**를 압축해 별도로 전달합니다. 실행 파일 하나만 복사하면 콘텐츠·라이브러리가 빠집니다. 소스 SHA, 빌드 결과, 압축 파일의 SHA256을 함께 기록합니다. 전달·업로드는 이번 안내 제작에서 수행하지 않았습니다. 참고 소스로 새 패키지를 만들 경우 해당 참고 폴더에서 아래를 실행합니다.

```powershell
$PackageRun = 'Team-package-' + (Get-Date -Format 'yyyyMMdd-HHmmss')
pwsh -NoProfile -File "$ProjectRoot/tools/Build-P0Package.ps1" -Platform Win64 -RunId $PackageRun
if ($LASTEXITCODE -ne 0) { throw 'Saved/P0Runs의 package.log를 확인하세요.' }
# 산출물: $ProjectRoot/Saved/P0Runs/$PackageRun/Package/Windows/
```

### 함께 관찰할 순서

1. 두 사람의 참고 소스 SHA 또는 패키지 해시가 같은지 확인합니다.
2. 동일 PC에서는 패키지를 두 번 실행합니다. 첫 창에서 ‘호스트 시작’, 둘째 창에서 `127.0.0.1:7777`을 입력하고 ‘주소로 참가’를 누릅니다.
3. 서로 다른 PC의 같은 로컬 네트워크에서는 호스트 PC의 실제 IPv4와 포트를 입력합니다. `127.0.0.1`은 자기 PC이므로 상대 주소가 아닙니다. 방화벽·포트 사용 상태를 확인합니다. 기존 동일 PC 검수는 두 물리 PC의 네트워크 성공 증거가 아닙니다. 인터넷 중계·계정 매칭을 제공하는 구성으로 해석하지 않습니다.
4. 양쪽 자기 보드가 아래에 보이는지, 첫 소환·이동·합성·판매와 실패 피드백이 어떻게 동작하는지 함께 확인합니다.
5. 한 판의 웨이브·보스·승패, 결과에서 시작 화면 복귀를 확인하고 각자 관찰과 질문을 적습니다. 실제 플레이하지 않은 항목은 미실행으로 남깁니다.

이 단계는 완성 형태를 공유하는 오리엔테이션입니다. 관찰·빌드 성공을 학습자의 직접 구현이나 모든 게이트 재검증으로 기록하지 않습니다. 실제 게임 검수 절차는 [G3 통합 수업](P0/G3_INTEGRATION.md)과 [제품 검수 기록](../docs/production/TEST_RUNS.md)을 따릅니다.

<a id="daily"></a>

## 8. 매일 각자 개발하는 순서

| 담당 | 주 작업 | 열 프로젝트 |
|---|---|---|
| A | 매치·데이터·유닛·피해·사망·웨이브·승패·전투/결과 위젯 | `learn-a/Mobile_defense_clone.uproject` |
| B | 보드·경로·카메라·입력·명령·경제·조작/HUD | `learn-b/Mobile_defense_clone.uproject` |
| 해당 게이트 통합 담당 | 공통 계약 선택·통합·양쪽 실행 검증 | `integration/Mobile_defense_clone.uproject` |

G0의 공통 기반은 A/B가 같은 계약으로 각각 구현하고 차이·선택 이유를 비교한 뒤 하나로 합칩니다. 이후 공용 타입·설정·생성기는 수정 담당을 먼저 정합니다. 같은 파일의 양쪽 독립 수정이 필요한 경우 통합 순서와 충돌 예상 부분을 미리 기록합니다.

**작업 시작:** 자기 폴더에서 아래 상태를 확인합니다. `$Role`은 A=`a`, B=`b`입니다. `merge --ff-only`는 내 역할의 원격 이력을 앞으로만 반영하며 이력이 갈라졌다면 멈춥니다. 미커밋 변경이 있다면 먼저 관련 파일을 검토·커밋하고, 무관한 파일은 보존한 채 원인을 확인합니다.

```powershell
$Role = 'a'
$LearnRoot = "C:/P0Team/learn-$Role"
git -C $LearnRoot branch --show-current
git -C $LearnRoot status --short
# 위 출력이 내 learn 브랜치이고, 미커밋 변경이 없을 때만 계속
git -C $LearnRoot fetch origin
if ($LASTEXITCODE -ne 0) { throw '원격 이력 수신 실패' }
git -C $LearnRoot merge --ff-only "origin/learn/p0-$Role"
if ($LASTEXITCODE -ne 0) { throw '이력이 갈라졌습니다. 덮어쓰지 말고 로그를 비교하세요.' }
```

**구현·검증:** 수업 한 기능을 구현하고 필요한 검사와 실제 실행을 합니다. 상대 기능이 없으면 명시된 Stub으로 검사하고 연결 후 다시 확인합니다. 전체 기능을 끝낼 때까지 통합을 미루지 않고, 검토·빌드 가능한 반나절~2일 단위로 전달합니다.

**커밋·공유:** `git diff`로 변경을 읽고 관련 파일만 지정합니다. 아래 `git add`의 경로와 커밋 메시지는 **A의 유닛 기능 변경을 가정한 예시**이므로 실제 작업의 파일·수업·작업 ID로 바꿉니다. `git add .`로 다른 사람 파일·로컬 설정을 일괄 포함하지 않습니다.

```powershell
git -C $LearnRoot diff --stat
git -C $LearnRoot diff
git -C $LearnRoot add -- Source/Mobile_defense_clone/Battle/LDUnitActor.cpp
git -C $LearnRoot diff --cached
# 관련 헤더·검사·기록 파일도 실제 변경 목록을 보고 추가한 후 커밋
git -C $LearnRoot commit -m 'feat(unit): 기본 공격 대상 선택 구현'
if ($LASTEXITCODE -ne 0) { throw '커밋 실패' }
# 팀의 원격 공유를 시작한 뒤에만 실행
git -C $LearnRoot push origin "learn/p0-$Role"
if ($LASTEXITCODE -ne 0) { throw '공유 실패. 로컬 커밋을 보존하고 원인을 확인하세요.' }
```

전달에는 **커밋 SHA, 바꾼 API·에셋, 실행한 검사와 결과, 미검증·남은 문제**를 넣습니다. 상대는 같은 SHA를 기준으로 리뷰합니다. 메시지 형식은 [커밋 규약](../COMMIT_CONVENTION.md), 상태·할 일의 원본은 [작업 보드](../docs/production/BOARD.md)를 따릅니다.

<a id="integration"></a>

## 9. 리뷰하고 학습 통합 브랜치로 합치기

먼저 게이트별로 통합 담당 한 명을 정합니다. 아래는 **담당 PC의 새 통합 폴더**를 만드는 최초 명령입니다. 이미 있으면 그 폴더를 사용합니다. 같은 로컬 저장소에서 사용 중인 브랜치에 두 번째 worktree를 만들지 않습니다.

```powershell
# GUIDE: integration-setup
$RepoRoot = 'C:/P0Team/repo'
$IntegrationRoot = 'C:/P0Team/integration'
git -C $RepoRoot fetch origin
if ($LASTEXITCODE -ne 0) { throw '원격 이력 수신 실패' }
git -C $RepoRoot worktree add --track -b learn/p0-integration $IntegrationRoot origin/learn/p0-integration
if ($LASTEXITCODE -ne 0) { throw '통합 폴더·기존 로컬 브랜치 연결을 확인하세요.' }
```

통합은 다음 순서로 진행합니다.

1. A/B가 기능 커밋을 공유하고 상대가 코드·상태 소유권·권한·수명·실패 사례를 검토합니다. PR을 쓰면 대상 브랜치는 `learn/p0-integration`입니다. 참고 브랜치를 대상으로 올리지 않습니다.
2. 통합 담당자가 깨끗한 통합 폴더에서 원격 이력을 받고 **리뷰한 A/B의 전체 SHA를 고정**합니다. 그 사이 역할 브랜치가 진행돼도 검토하지 않은 새 커밋을 끼워 넣지 않습니다.
3. 공통 계약의 선택을 반영하여 먼저 필요한 역할부터 병합합니다. G0는 중복 기반 코드의 충돌이 예상되므로 Git의 자동 병합 결과만으로 합격시키지 않습니다. 상대 역할도 병합하고 각 병합의 충돌을 끝낸 뒤 다음 단계로 갑니다.
4. Editor 빌드·해당 게이트 검수·실행 증거를 통합 코드에서 확인하고, 실패하면 통합 수정과 재검증을 기록합니다.
5. 통합 결과를 리뷰한 뒤 공유합니다. A/B는 자기 미커밋 변경을 정리하고 **검증된 통합 SHA**를 각각 병합하여 다음 기능의 공통 상태를 맞춥니다.

아래는 로컬 통합을 먼저 검증하는 방식입니다. PR을 사용하는 경우 이 단계는 PR 검토용 로컬 확인이며, 웹에서 병합한 결과는 fetch 후 다시 대조합니다. 로컬 merge와 웹의 squash/rebase를 중복 적용하지 않습니다. 두 학습 역할이 반복해서 합쳐지는 이력은 merge commit 방식으로 유지하는 것을 권장합니다.

```powershell
$IntegrationRoot = 'C:/P0Team/integration'
git -C $IntegrationRoot status --short
git -C $IntegrationRoot fetch origin
if ($LASTEXITCODE -ne 0) { throw '원격 이력 수신 실패' }
git -C $IntegrationRoot merge --ff-only origin/learn/p0-integration
if ($LASTEXITCODE -ne 0) { throw '통합 이력 차이를 먼저 검토하세요.' }
$ReviewedA = Read-Host '리뷰한 A 전체 SHA'
$ReviewedB = Read-Host '리뷰한 B 전체 SHA'
git -C $IntegrationRoot merge --no-ff $ReviewedA -m 'merge(learning): A 게이트 구현 통합'
if ($LASTEXITCODE -ne 0) { throw 'A 병합을 해결·검증한 뒤 다음 명령으로 진행하세요.' }
git -C $IntegrationRoot merge --no-ff $ReviewedB -m 'merge(learning): B 게이트 구현 통합'
if ($LASTEXITCODE -ne 0) { throw 'B 병합을 해결·검증한 뒤 검수를 진행하세요.' }
# 여기서 통합 빌드·검수·상호 리뷰. 실제 결과와 SHA 기록
git -C $IntegrationRoot rev-parse HEAD
```

로컬 통합 방식으로 검수·리뷰를 완료했고 팀의 원격 공유가 시작된 경우에만 담당자가 `git -C $IntegrationRoot push origin learn/p0-integration`을 실행합니다. PR 병합 방식을 택했으면 해당 원격 결과를 가져오고 최종 SHA를 확인합니다. 아래는 **각 역할 폴더에서 검증된 통합 결과를 받는 단계**입니다.

```powershell
git -C $LearnRoot fetch origin
if ($LASTEXITCODE -ne 0) { throw '원격 이력 수신 실패' }
$VerifiedIntegration = Read-Host '검수 통과한 통합 전체 SHA'
git -C $LearnRoot merge $VerifiedIntegration --no-edit
if ($LASTEXITCODE -ne 0) { throw '충돌을 해결하고 영향 범위를 다시 검사하세요.' }
# 빌드·관련 검사 후 내 역할 브랜치를 공유
```

충돌이 생기면 파일별로 어떤 상태가 원본인지·어느 API를 사용할지 결정하고, 해결한 파일만 `git add`한 뒤 `git merge --continue`합니다. 전체 파일에 무조건 ours/theirs를 적용하지 않습니다. 병합 전 깨끗한 작업 폴더였고 병합 중 작성한 별도 변경도 보존했다면 `git merge --abort`로 병합을 중단할 수 있습니다.

`main` 반영은 별도 제품 검증과 PR 절차입니다. 문서 CI는 Unreal 빌드·패키지 실행·Android 실기기 검수를 대신하지 않습니다. 현재 main에는 최초 커밋만 있으므로, 처음 제품 PR을 만들 때는 기획·기반부터 누적된 전체 차이를 함께 검토해야 합니다. 이 안내의 완료나 참고본 확인만으로 main을 병합하거나 P0 최종 완료로 표시하지 않습니다.

<a id="files"></a>

## 10. 무엇을 Git으로 공유하나요?

| 종류 | 공유 방식 | 관리 기준 |
|---|---|---|
| `Source/`, `Config/`, `.uproject`, `Content/` | Git에 관련 변경 커밋 | 코드·공유 설정·맵·에셋이 재현 입력 |
| `data/`, `tools/`, `docs/` | Git | 명세·데이터 원본·생성 결과·제품 증거를 함께 검토 |
| `learning/` | 참고 브랜치에 Git | 수업 원문과 HTML. 실제 학습 이수로 자동 기록하지 않음 |
| `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `.vs/`, `.sln` | 각 PC에서 생성 | 현재 무시 규칙 확인. 빌드 폴더를 강제 추가하지 않음 |
| `Saved/`의 로그·패키지 | 로컬 보존 / 필요한 결과를 별도 전달 | clone에 포함되지 않음. 선택 증거는 지정된 증거 폴더와 SHA·해시로 연결 |
| 개인 설정·토큰·로그인 정보 | 공유 제외 | 에디터가 설정을 자동 수정했는지 diff 확인 |
| HTML 개인 메모·진도 | 브라우저의 JSON 백업 | Git 커밋·팀 작업 보드·공식 검증 상태와 별개 |

### 맵·Blueprint·공용 설정의 충돌 방지

`.uasset`와 `.umap`은 현재 `.gitattributes`에서 바이너리로 지정되어 있으며 **Git LFS 필터나 잠금이 설정되어 있지 않습니다.** Git이 에셋 공동 편집을 자동 조정해 준다고 기대하지 않습니다. LFS 도입은 기존 이력·협업 환경을 검토할 별도 변경이며 이번 안내에서 적용하지 않았습니다.

작업 보드에 맵·Blueprint·생성기·공용 Config의 파일 담당과 사용 시간을 기록합니다. 한 사람이 저장·커밋하고 상대가 그 SHA를 받은 뒤 다음 편집을 시작합니다. 바이너리 충돌은 텍스트처럼 합치지 않고 양쪽 버전을 보존하여 선택한 기준 에셋 위에 필요한 변경을 에디터에서 재적용합니다. 에셋 이름 변경은 UE 에디터에서 참조와 redirector를 함께 검토합니다.

같은 PC의 에디터 빌드·데이터/에셋 생성기·검수 포트·Android 기기는 동시에 점유하지 않습니다. A/B의 worktree가 달라도 공유 엔진·기기·포트는 충돌할 수 있습니다. 각 Run에는 새 이름을 사용해 이전 증거를 덮어쓰지 않습니다.

<a id="update-reference"></a>

## 11. 참고본과 HTML이 업데이트되면

참고본 유지 담당은 수정·검증한 버전을 별도 커밋으로 공유하고 새 SHA와 바뀐 범위를 알려 줍니다. 두 개발자는 참고 폴더의 에디터를 닫고 미커밋 변경이 없는지 확인한 뒤 다음을 실행합니다. 참고본에서 만든 실험 변경이 있다면 먼저 별도 커밋/보존 경로로 남기고 업데이트 여부를 판단합니다.

```powershell
$ReferenceRoot = 'C:/P0Team/reference'
git -C $ReferenceRoot status --short
git -C $ReferenceRoot fetch origin
if ($LASTEXITCODE -ne 0) { throw '원격 이력 수신 실패' }
git -C $ReferenceRoot merge --ff-only origin/reference/p0-integration
if ($LASTEXITCODE -ne 0) { throw '참고 폴더의 로컬 변경과 이력 차이를 확인하세요.' }
git -C $ReferenceRoot rev-parse HEAD
```

HTML만 바뀌었다면 열려 있는 학습실을 새로고침하면 됩니다. 게임 코드·설정·에셋이 바뀌었다면 영향에 맞게 다시 빌드하고 실행합니다. 두 사람이 사용할 참고 SHA를 먼저 맞추며, **reference 업데이트를 learn에 통째로 병합하지 않습니다.** 학습자의 진행 중 코드에는 필요한 수정 원리와 계약을 이해하고 직접 반영합니다.

수업을 수정할 때 원본은 Markdown입니다. 참고 폴더에서 `node learning/tools/build-html.mjs`로 HTML을 재생성하고 `node learning/tools/build-html.mjs --check`, `node learning/tools/validate.mjs`를 실행합니다. 생성 HTML만 직접 고치지 않습니다. PC·브라우저·파일 경로를 바꾸기 전에는 학습실 ‘나의 학습 기록 → 기록 백업’에서 개인 JSON을 내보냅니다.

<a id="troubleshooting"></a>

## 12. 자주 막히는 지점

| 증상 | 원인부터 확인 | 다음 행동 |
|---|---|---|
| clone했는데 참고 게임·HTML이 없음 | main이나 원래 출발 폴더를 열었는지, 원격 게시가 끝났는지 | 원격 branch 확인 → fetch → reference worktree 사용 |
| `invalid reference` / 원격 브랜치가 없음 | 참고·학습 6개 브랜치가 아직 게시되지 않았거나 권한 없음 | 담당자에게 저장소 접근·실제 게시 SHA 확인 |
| branch already checked out | 같은 로컬 저장소의 다른 worktree가 사용 중 | `git worktree list`의 기존 경로 사용. 강제 연결 해제하지 않음 |
| 폴더가 이미 존재한다는 오류 | 이전 설치/재현 파일이 남아 있음 | 상태 확인 후 기존 폴더 사용 또는 새 이름 선택. 삭제로 해결하지 않음 |
| 수정했는데 GitHub에서 안 보임 | 저장 → add → commit → push 중 어디까지 했는지 | status·최근 커밋·upstream 확인. 파일 저장만으로 공유되지 않음 |
| push 거절 / ff-only 실패 | 상대의 새 커밋, 서로 갈라진 이력, 권한 또는 보호 규칙 | fetch 후 `git log --oneline --graph --all -20`으로 비교. 강제 push 금지 |
| C++ 모듈을 찾지 못하거나 빌드 실패 | 새 clone의 빌드 미생성, 엔진/툴체인 차이 | 해당 폴더 Editor 빌드의 첫 원인 오류 확인 |
| 빈 템플릿·옛 화면이 열림 | learn 출발점 또는 원래 프로젝트를 열었는지 | `.uproject` 전체 경로·현재 SHA 확인 |
| 두 번째 PC가 참가하지 못함 | 서로 다른 버전, 호스트 주소, 방화벽, 포트 점유 | 같은 패키지 확인 → LAN IPv4:7777 → 양쪽 로그 보존 |
| 학습 체크가 다른 PC에서 사라짐 | 진도는 브라우저/파일 위치별 개인 기록 | 이전 환경에서 JSON 백업 후 새 학습실로 복원 |

작업을 잃지 않는 기본값은 **확인하고 보존한 뒤 원인을 해결하기**입니다. `reset --hard`, `clean -fd`, 강제 push, 기존 worktree 삭제를 일반 복구 절차로 쓰지 않습니다. 서로의 변경이 포함된 커밋을 되돌려야 하면 영향과 복구 방법을 먼저 리뷰합니다.

## 13. 둘이 공유할 최소 기록

아래 항목이면 상대가 같은 상태를 열고 문제를 재현할 수 있습니다. 파일 이름만 전달하거나 ‘내 PC에서는 됨’으로 완료를 기록하지 않습니다.

```text
역할 / 작업 ID / 게이트:
저장소 / 브랜치 / 전체 SHA:
프로젝트 폴더 / 엔진·도구 버전:
변경 목적 / 전달 API·에셋:
검사 종류 / 명령·관찰 절차 / 결과 / 증거 경로:
미검증·남은 문제 / 다음 연결 순서:
상대 리뷰 / 통합 SHA:
```

처음에는 두 사람이 같은 참고본의 소환·전투·이동·합성·판매·결과 흐름을 설명할 수 있는지 확인합니다. 이후 [P0 시작](P0/README.md) → [공통 계약](P0/COMMON.md) → [A](P0/A/README.md) / [B](P0/B/README.md) → [게이트 통합](P0/INTEGRATION.md) 순으로 진행합니다. 전체 운영 원칙은 [학습 WORKFLOW](WORKFLOW.md), 제품 상태의 원본은 [정식 WORKFLOW](../docs/WORKFLOW.md)와 [작업 보드](../docs/production/BOARD.md)에 둡니다.

## 14. 이 안내에서 확인한 것과 남은 확인

실제 로컬·원격 브랜치, 연결된 worktree, 무시 규칙, 에셋 속성, 시작 맵·접속 UI 코드, 제공 빌드 스크립트를 읽고 경로를 대조했습니다. HTML 생성·링크 검사와 Git 절차의 재현 결과는 [안내 검증 기록](git-guide-verification.json)에 분리해 기록합니다. 로컬 Git 절차 재현은 작은 검사용 저장소를 사용하며 제품 구현 검증으로 표시하지 않습니다.

원격 게시·계정 초대·두 개발자의 새 PC 설치·실제 학습 커밋·팀 PR/병합은 실행하지 않았습니다. 이번 변경은 안내·HTML에 한정하여 Unreal과 게임 검수를 새로 수행하지 않습니다. 기존 HTML UI 검증과 이번 문서 정적 검사는 별개이며, 자동 브라우저의 로컬 파일 접근 제한 때문에 이번 안내의 브라우저 화면·버튼은 새로 검수하지 않았습니다. 실제 학습자 상태와 미완료 제품 게이트를 그대로 유지합니다.
