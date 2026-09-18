# P0 규칙 snapshot — P0 / A / A-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-DATA-01 / [A-01 설계](../../../docs/technical/IMPLEMENTATION_A.md#a01) |
| 참고 자료 제작 상태 | Draft |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 수정 코드 `4cc3e0fd63d074df2d2e4568cbc0889cd0ecc2a6`; 실행 HEAD `0efd2bb8c32f99e5814d6aa4d4891ee9d5e96c69` (수업 재현 미완료) |
| 실제 개발 시작/완료 SHA | 출발점만 준비 / 미생성 |
| 필요한 상대 산출물·버전 | B의 독립 공통 기반, Schema2 / Rules0.3.0 |
| 제공 코드 / 직접 작성할 코드 | 제공: `data/` 기획 JSON, 스타일 도구. 직접 작성: Data/LDGameData.h/.cpp, Tests/LDDataTests.cpp, Build.cs의 Json 의존성 |

## 이번에 만들 동작

매치가 `Content/LD/Data/`의 JSON 여섯 개를 한 번 읽고, 검증을 모두 통과한 P0 행만 공개한다. 유닛 16종·웨이브 10개·일반 적 N01·보스 B01을 읽는다. 비활성 P1 행은 실행 데이터에 넣지 않는다. 규칙이 잘못되면 파일·행·필드가 포함된 오류를 반환하고 부분 로딩 결과를 공개하지 않는다.

DataTable 에디터 임포트 대신 네이티브 JSON 로더를 선택했다. 기획용 원본과 패키지 런타임 입력을 동일 스키마로 읽고 실패를 자동화할 수 있다. `FLDUnitRow` 등은 UObject 참조 없는 값 구조체다. Name은 UnitId로 한 번 해석하고 UPROPERTY RowName을 중복 선언하지 않는다. 런타임은 `learning/`이나 프로젝트 루트 `data/`를 찾지 않는다.

## 코드 작성 순서

출발점부터의 입력·명령·증거 형식은 [공통 재현 절차](../COMMON.md#g0-replay)를 따른다. A-02의 `Data/LDMatchTypes.h` 값 계약을 먼저 작성한다. 이 수업의 `Tests/LDDataTests.cpp`가 해당 헤더를 포함하므로, A-01만 복원한 상태를 완전한 빌드 단위로 간주하지 않는다. 참고 파일 조립은 `-Role A`이며 고정 소스는 위 `4cc3e0f…`다. 소스 작성 순서와 참고 파일 복원은 같은 결과를 비교하기 위한 별개 행위다.

1. `Source/Mobile_defense_clone/Data/LDGameData.h`: Unit/Enemy/Wave 값 타입 → FLDGameRules → ULDGameData 조회/로딩 API 순서로 작성한다. 상태는 private이고 GetRules/GetUnits/GetWaves는 const 참조다. 실패한 TryGet 조회는 호출자의 출력값을 바꾸지 않는다.
2. `Data/LDGameData.cpp`: FCheckedObject의 필수 타입/범위 오류 반환, 파일/배열 로딩, 중복 Name 검사 순서로 작성한다. 직접 Get 호출은 앞 단계에서 타입 확인한 Name에만 쓴다.
3. ParseRules/ParseLayout에서 버전·P0 정책·소유 셀 순열·좌표 간격·서로 분리된 두 보드·닫힌 경로 길이와 같은 중앙 방향을 검사한다. 단순히 JSON 파싱 성공을 유효한 규칙으로 취급하지 않는다.
4. ParseUnits → ParseEnemies → ParseWaves → ParseProfiles를 작성한다. 요구 P0 행 수·참조·판매 정책·가중치 합계·생성 구간 경계를 검사한다.
5. LoadP0FromDirectory에서 모든 후보를 지역 변수로 구성한 뒤 마지막에 원본 필드로 이동한다. 같은 경로의 두 번째 호출은 고정 snapshot을 유지한다. 다른 경로를 다시 읽으려면 새 데이터 객체를 만든다.
6. `Mobile_defense_clone.Build.cs`의 PrivateDependencyModuleNames에 `Json`을 추가한다. 런타임 파서에 에디터 임포트 모듈이나 JsonUtilities는 필요하지 않다.
7. `Tests/LDDataTests.cpp`: 구현 숫자에서 기대값을 얻지 않고 현행 명세의 16/10/18/20, 9743/198/49/10, C01 공격15·사거리175, 보스1/진영·60초를 명시한다. 변조·누락·뒤늦은 참조 실패에서도 원본이 비어 있는지 확인한다.

호출 흐름은 `GameMode.InitGameState → NewObject<ULDGameData> → LoadP0 → LoadP0FromDirectory → 후보 파싱/검증 → 한 번 게시`다. 원본 소유자는 GameMode가 UPROPERTY로 보관한 GameData이며, 규칙 소비자는 읽기 전용 API만 쓴다.

## Unreal 설정 순서

아래는 재현 지침이며 A 역할에서 직접 에디터 설정을 수행한 기록이 아니다.

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | 파일 `Content/LD/Data/` | GameRules, DT_Units, DT_EnemyTypes, DT_Waves, DT_SummonProfiles, DT_SpawnProfiles JSON을 같은 출발점 `data/`에서 복사 | 런타임 입력; 에디터에서 DataTable로 생성하지 않음 |
| 2 | Project Settings → Packaging → Additional Non-Asset Directories to Package | `LD/Data` | 패키지 UFS에서 FFileHelper로 읽기. 공용 Config 담당자가 설정/검증 |
| 3 | C++ Editor 빌드 | `Mobile_defense_cloneEditor`, Win64, Development | UHT·Json 헤더·링크 확인; 역할 에디터를 닫은 상태에서 빌드 |
| 4 | Tools → Test Automation | `LD.P0.G0.Data` 검색/선택 | 4개 테스트가 등록되어야 함 |
| 5 | Blueprint·UMG·맵 | A-01은 새 에셋/위젯/레벨 변경 없음 | UI 생성 전에 규칙 로딩을 완료하게 함 |

빌드 도구 경로는 통합 기록의 현재 엔진 경로를 사용한다. 명령줄 자동화는 `UnrealEditor-Cmd.exe <절대 프로젝트 경로> -unattended -NullRHI -ExecCmds="Automation RunTests LD.P0.G0.Data" -TestExit="Automation Test Queue Empty" -log=<절대 로그 경로>`다. 명령을 시작한 프로세스 종료 코드와 테스트 실패 개수를 둘 다 확인한다.

## 실행·실패·수정 기록

아래 Pass는 기존 A 참고 제작 실행 결과다. 새 detached 출발점에서의 **수업 재현 결과는 NotRun**이며 공통 절차의 `Replay-G0-A-assembly/assembly.json`, `Replay-G0-A-editor/result.json`, `Replay-G0-A-tests/result.json`을 실제 생성한 후 별도로 기록한다. 원본 테스트4개 중 ParticipantIdentity는 A-02와 공유하는 값 계약 부분 검사다. 재현 명령이 적혀 있다는 이유만으로 아래 결과를 재현 Pass로 복사하지 않는다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| C++ 스타일 검사 | 신규10개+샘플1개 오류0 | Pass, 11 checked / 48 unchanged legacy / 0 errors | `node tools/check-code-style.mjs`, clang-format20.1.8; [기록](evidence/G0_STATIC.md) |
| git diff --check | 공백 오류0 | Pass | [기록](evidence/G0_STATIC.md) |
| 정상 런타임 JSON | 유닛16·웨이브10·P1 제외 | Pass | `LD.P0.G0.Data.ValidP0Snapshot`; [실제 증거](evidence/G0_RUNTIME.md) |
| 같은 LoadP0 재호출 | 고정 snapshot 유지 | Pass | 위 테스트의 idempotent assertion |
| Schema/Rules 불일치·필수 Economy 누락·P1 활성 | false, 구체적 오류, 원본 미게시 | Pass | `RejectChangedContractsAtomically` |
| 유예 패배3초·변경된 이벤트 순서·잘못된 허용 목록 | false | 정적 리뷰 후 검증 추가, 실제 UE 자동화 Pass | 같은 mutation 테스트; 변경 정책이 필드별 오류와 함께 거절됨 |
| X 좌표 중복·보드 Y 중첩 | false | 간격/분리/둘레 검사 추가 후 UE 자동화 Pass | 같은 mutation 테스트; 각각 XCentersCm/YCentersByPlayer 오류 |
| B01→MISSING_BOSS, 없는 디렉터리 | false, 유닛/웨이브 부분 게시0; 올바른 경로로 재시도 가능 | Pass | `InvalidRowsAndMissingFiles` |
| 첫 Editor 빌드 | UHT/C++/링크 성공 | Fail: LDGameData.cpp554 C2110/C2661 | `Saved/P0Runs/G0-A-editor/build.log`; 전체 실패 로그 보존, 통합 담당자가 실제 실행 |
| 오류 문자열 연결 수정 후 Editor 재빌드 | 컴파일 성공 | Pass, exit0, 19.26초 | `G0-A-editor-fix1`; [실제 증거](evidence/G0_RUNTIME.md) |
| PIE·PC 패키지·네트워크·Android | 각각 별도 검수 | 모두 NotRun | 통합 담당자가 직렬 실행 예정 |

자동화에서 변조한 입력은 `Saved/Automation/LD/P0/G0/<GUID>/`에 보존된다. 실패 로그의 fixture 경로를 LoadP0FromDirectory에 넣어 재현하며, 원본 data/와 Content 입력은 변경하지 않는다. 스냅샷 검사는 게임 실행·패키지 로딩의 대체 증거가 아니다.

실제 첫 빌드에서 `TEXT("DT_Units.json") + TEXT("[")`는 두 문자열 포인터를 더하므로 C2110이 발생했다. 뒤의 FCheckedObject 생성 오류는 연쇄 오류였다. 왼쪽을 `FString(TEXT("DT_Units.json["))`으로 만들어 값 문자열 연결로 고쳤다. 포맷/정적 검사는 C++ 타입 검사를 대신하지 못한다는 사례다. 수정 후 실제 Editor 재빌드가 성공했고 NullRHI 자동화4개가 모두 통과했다. 게임 실행 화면과 패키지 데이터 로딩은 아직 검증하지 않았다.

## 상대에게 전달하고 통합하기

API: `LoadP0`, `LoadP0FromDirectory`, `IsLoaded`, `GetRules`, `GetUnits`, `GetWaves`, `TryGetUnitRow`, `TryGetEnemyRow`. `Data/LDGameData.h`가 실제 선언 원본이다. 규칙의 RulesVersion은 FName, 시간·거리는 double, 셀 순서는 `TArray<TArray<int32>>`다.

B의 독립 로더와 타입을 먼저 비교하고 공통 타입·런타임 로더를 하나만 선택한다. A/B Editor 빌드를 각각 확인한 후 통합 Editor 빌드와 네 테스트를 다시 실행한다. 통합 시 Content JSON 여섯 개와 UFS 설정도 함께 받아야 한다. A 코드 커밋은 위 표에 고정했다. 통합 참고 SHA·실제 학습 통합 SHA는 아직 미확정이다.

## 이해 확인

- 후보 Units를 멤버에 먼저 채운 뒤 Wave 검증에 실패하면 어떤 상태가 관찰되는가?
- SummonMaxGold=null을 0으로 해석하면 왜 소환 가격이 달라지는가?
- 작은 변형: Saved 픽스처의 DT_Units에 중복 Name과 잘못된 Grade를 각각 넣고 오류 위치를 확인한다. 제품 데이터는 바꾸지 않는다.
- G1 진입 조건: 실제 Editor 빌드 및 테스트, 공통 계약 비교, 패키징 경로 설정 확인. 이 수업을 출발점부터 재현하지 않았으므로 Verified로 올리지 않는다.

## 단계 완료

- [ ] 코드·에디터 설정·제공 파일 범위가 재현 가능하다.
- [x] Unreal 컴파일·자동화 결과와 시작/코드/실행 커밋을 연결했다. PIE·패키지 실행은 미완료다.
- [ ] 필요한 상대 기능을 합쳐 확인했다.
- [x] 미검증 범위와 다음 단계의 의존성을 명시했다.
