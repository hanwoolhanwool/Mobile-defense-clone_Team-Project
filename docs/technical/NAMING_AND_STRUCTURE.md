---
id: TECH-NAMING
version: 0.1.0
status: Baseline
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-04
applies_to: 신규 클래스·소스·콘텐츠·데이터 이름과 배치
baseline_basis: DEC-025 사용자 코드 규약·포맷 설정·자동 검사 작성 지시
---

# 클래스·폴더·데이터 규칙

[C++·Blueprint 규약](CODING_STANDARD.md) · [아키텍처](ARCHITECTURE.md) · [데이터 명세](../DATA_SCHEMA.md)

이 문서는 새 이름을 만드는 기준이다. 기존 모듈·타깃 `Mobile_defense_clone`, 템플릿 이름, 직렬화된 필드·ID는 호환성 때문에 유지한다. 클래스 책임은 아키텍처, 실제 데이터 필드·타입은 데이터 명세가 원본이다. 이 문서의 예시는 새 게임 기능이나 수치를 확정하지 않는다.

## 1. C++ 이름

| 대상 | 규칙 | 예시 |
|---|---|---|
| Actor 계열 | A + LD + 역할 | `ALDUnitActor`, `ALDGameMode` |
| UObject·Component·Subsystem | U + LD + 역할 | `ULDEconomyService`, `ULDBattleSubsystem` |
| struct·일반 값 타입 | F + LD + 역할 | `FLDUnitRow`, `FLDCommandResult` |
| enum class | E + LD + 역할, 값은 의미 있는 PascalCase | `ELDCommandResultCode` |
| 네이티브 interface | ULD 리플렉션 선언 + ILD 인터페이스 | `ULDInteractable`, `ILDInteractable` |
| 파일 | 주 타입에서 UE 타입 접두사만 제거 | `ALDUnitActor` → `LDUnitActor.h/.cpp` |
| 함수·일반 변수 | 역할이 보이는 PascalCase | `TryPlaceUnit`, `UnitId`, `OutResult` |
| bool 변수·질의 | 변수는 b, 질의 함수는 Is/Has/Can 등 | `bEnabled`, `IsConfigured` |
| 상수 | 의미 있는 PascalCase, 우선 constexpr/const | `MaxRequestCount` |
| delegate·이벤트 | F 접두사와 동작을 표현, 노출 이벤트는 On... | `FLDOnStateChanged`, `OnStateChanged` |
| 로그 카테고리 | LogLD + 기능 | `LogLDEconomy` |

코드·폴더 식별자에 공백·한글·불필요한 버전 꼬리표(`Final2`, `New`)를 사용하지 않는다. `LD`는 프로젝트 전용 타입 구분이다. UE 프레임워크가 요구하는 접두사를 대체하지 않는다. 다른 모듈에 노출할 타입·함수에는 기존 모듈 export 매크로 `MOBILE_DEFENSE_CLONE_API`를 사용한다.

## 2. 소스와 도구 배치

기존 모듈 안에 필요한 기능 폴더를 작업할 때 추가한다. 빈 폴더나 새 모듈을 미리 대량 생성하지 않는다.

| 경로 | 책임 |
|---|---|
| `Source/Mobile_defense_clone/Core/` | GameMode·GameState·Controller·CameraPawn, 게임 흐름 |
| `.../Battle/` | 전투·웨이브·유닛·적·효과 |
| `.../Board/` | 보드 상태·배치·좌표 변환 |
| `.../Economy/` | 소환·소비·재화·강화 계산 |
| `.../Network/` | 요청/응답 계약·네트워크 검증 보조 |
| `.../Data/` | 행 struct·데이터 접근·검증 |
| `.../Save/` | 저장 형식·버전·로드/복구 |
| `tools/` | 저장소 검증·생성·빌드 스크립트 |
| `tools/style/` | 포맷 버전·기존 파일 기준·서식 샘플 |

`Variant_Strategy`, `Variant_TwinStick`, 기존 TopDown 코드는 참고 템플릿이다. 새 디펜스 로직은 담당 기능 폴더에 넣는다. 같은 모듈 안에서는 현재의 헤더/구현 인접 배치를 유지한다. 모듈을 분리하거나 Public/Private 구조를 도입할 때는 include 경로와 Build.cs 의존성을 함께 변경한다.

폴더만으로 런타임/Editor 코드가 분리되지는 않는다. 새 Editor 전용 API는 런타임 패키징에 포함되지 않도록 Editor 모듈 또는 적절한 빌드 조건을 설계한다. 플러그인·새 소스 루트를 추가하는 PR에는 포맷 검사 대상 확장도 포함한다.

Node 도구는 기존의 `동사-대상.mjs`, 테스트는 `*.test.mjs`, PowerShell은 기존의 `Verb-Noun.ps1` 이름을 따른다. C# Build/Target 파일 이름은 Unreal 규칙을 유지하며 `.editorconfig`의 탭 설정을 사용한다. 현재 C#·PowerShell·Node 전용 정적 분석기는 도입하지 않았다.

## 3. 콘텐츠 이름과 경로

전용 콘텐츠의 루트는 `Content/LD/`이며 에디터에서는 `/Game/LD/`로 보인다. 아래는 타입별 기본 접두사다. 형태는 `접두사_대상_설명_변형`이며 설명·변형은 필요할 때만 붙인다. 접두사 선택의 근거: [Epic Recommended Asset Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects).

| 종류 | 접두사·예시 | 기본 위치 |
|---|---|---|
| Blueprint 클래스 | `BP_GameMode`, `BP_UnitBase` | `Core/`, `Units/`, `Enemies/`, `Board/` 등 해당 기능 |
| Widget Blueprint | `WBP_HUD`, `WBP_SummonButton` | `UI/`의 화면·공용 하위 폴더 |
| Blueprint Interface | `BPI_Interactable` | 해당 기능, 공유할 때 `Core/` |
| Animation Blueprint | `ABP_Unit` | 해당 유닛 또는 `Art/Animations/` |
| 맵 | `L_Boot`, `L_TestCombat` | 제품은 `Maps/`, 검증 전용은 `Tests/TestMaps/` |
| DataTable·DataAsset | `DT_Units`, `DA_GameRules` | `Data/` |
| Input Action·Context | `IA_Select`, `IMC_Battle` | `Input/` |
| Static·Skeletal Mesh | `SM_BoardTile`, `SK_Unit` | `Art/Meshes/` 또는 소유 유닛 폴더 |
| Material·Instance·Texture | `M_Unit`, `MI_Unit_Red`, `T_Unit_BaseColor` | `Art/Materials/`, `Art/Textures/` 또는 소유 유닛 폴더 |
| Niagara System | `NS_Summon` | `FX/Niagara/` |
| Sound Wave·Cue | `A_ButtonClick`, `SC_ButtonClick` | `Audio/SFX/`, `Audio/Music/` |

여러 기능에서 공유하는 아트는 `Art/`, 특정 유닛만 사용하는 콘텐츠는 해당 유닛 폴더에 모은다. 동일 원본을 양쪽에 복사하지 않는다. 기존 맵과 BP 이름을 이 표에 맞추려고 일괄 변경하지 않는다. 외부 패키지는 출처·라이선스·원래 구조를 보존하고 참조 의존성을 검토한다.

이동·이름 변경은 Unreal Content Browser에서 수행한다. 참조가 있는 타입/프로퍼티를 바꿀 때는 영향 에셋 목록, 필요한 Core Redirects 또는 재저장 절차, 로드·쿠킹 검수를 PR에 남긴다. redirector 정리는 관련 에셋이 로드·저장 가능한 상태에서 수행하고 변경 파일을 함께 제출한다.

## 4. 데이터 작성과 호환성

- 행 ID는 표시 이름과 분리한 안정적인 키다. 현재 `C01`, `V_C01` 등 기존 키를 유지하고 재사용·대소문자만 다른 중복을 만들지 않는다. 새 ID 체계는 해당 데이터 명세에 먼저 기록한다.
- DataTable JSON의 `Name`은 RowName이다. `FLDUnitRow` 같은 `FTableRowBase` 파생 struct에서 `Name` 멤버를 중복 선언하지 않는다.
- 필드·타입·기본값·단위·허용 범위·참조 대상·None 의미는 [DATA_SCHEMA.md](../DATA_SCHEMA.md)에 정의한다. bool의 `b`나 새 단위 접미사 규칙을 이유로 기존 JSON 필드를 임의로 바꾸지 않는다.
- 신규 런타임 타입은 역할에 맞는 enum을 사용할 수 있다. 기존 JSON의 `Grade`, `DamageType` 등 FName 필드를 enum으로 바꾸려면 변환·임포터·검증기·호환 절차를 함께 변경한다.
- 설정 원본, 실행 중 상태, 세이브 상태를 분리한다. 런타임 진행 값을 DataTable/DataAsset 원본에 기록하지 않는다. 사용자에게 보여 주는 문구는 장기적으로 String Table/FText를 사용하되 현재 스키마 변경은 별도 작업이다.
- `data/*.json`의 편집 원본은 [데이터 원본 안내](../../data/README.md)에 지정된 생성기다. 생성 JSON·검증 보고서·전체 GDD 읽기본은 직접 수정하지 않는다.
- 변경은 명세와 생성기 수정 → 데이터 생성 → 소비 코드/임포트 반영 → 참조·범위·호환 검증 → 결과 기록 순서로 처리한다. 스키마나 소비 해석이 바뀌면 `RulesVersion` 및 저장/로드 호환 전략을 검토한다.
- 서버에서 쓰는 값은 서버 기준 데이터로 검증한다. 데이터 범위 검사를 통과했다는 이유로 게임 밸런스나 원작 일치를 확정하지 않는다. DEC-021이 우선한다.

## 5. 제출 범위

Source·공유 Config·Content 원본·문서·도구를 제출한다. `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, 로컬 IDE 설정·자격 증명은 생성물/개인 설정으로 분리한다. 현재 제외 목록은 [.gitignore](../../.gitignore)를 따른다. `Config/User*.ini`를 공유 설정으로 복사하지 않는다.

`.uasset`/`.umap`은 바이너리로 취급한다. 같은 에셋을 동시에 편집하지 않도록 담당 범위를 먼저 공유하고, 텍스트 충돌처럼 수동 병합하지 않는다. 현재 저장소에 LFS 잠금이나 자동 에셋 이름 검사가 적용된 것으로 가정하지 않는다. 필요하면 별도 운영 변경으로 도입한다.
