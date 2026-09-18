# 독립 공통 기반 — P0 / B / G0-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-DATA-01, TASK-NET-01, [공통 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md), [ARCH-01~06](../../../docs/technical/CODING_STANDARD.md) |
| 참고 자료 제작 상태 | Draft — 수업 시작점에서 설명대로 별도 재현 전 |
| 실제 개발 상태 | Planned |
| 참고 시작/완료 SHA | `8c6856d235de87cc28c12b49ca775bd0937334a5` / 수정 소스 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92` |
| 실제 개발 시작/완료 SHA | 공통 출발점 / 미생성 |
| 필요한 상대 산출물·버전 | A와 합의한 FLDMatchContext·FLDParticipantContext·FLDGameRules 선언, Schema2/Rules0.3.0 |
| 제공 코드 / 직접 작성할 코드 | 제공: 기존 프로젝트·data JSON·계약 문서. 직접 작성: 아래 Data/Core 공통 기반. A 구현을 복사하지 않는다. |

## 이번에 만들 동작

Content의 P0 JSON을 읽으면 16 활성 유닛·10웨이브·6×3 보드 규칙이 준비된다. 기존 80웨이브 원본의 P0Overrides를 적용한다. 로더는 유효한 전체 임시 스냅샷을 만들기 전 런타임 원본을 바꾸지 않는다. 잘못된 버전·누락 파일·중복 ID·보드 소유 범위가 틀리면 오류를 반환한다. GameMode는 초기 실패를 Aborted로 게시하고, 성공해도 미구현 전투·보드·경제 때문에 Preparing에 머문다.

공통 규칙·상태 소유권·초기화 순서는 위 공통 계약이 원본이다. B 구현의 차이는 단일 FReader가 오류를 모으고 숫자·문자열·배열을 검증한 뒤 값 스냅샷을 교체하는 것이다. GameMode가 B의 CommandProcessor와 Controller까지 연결하므로 명령 입구의 준비 여부를 실제 객체 관계로 표현한다. A와 동일한 공개 선언을 맞췄지만 로더·초기화 구현은 별도로 작성했다.

## 코드 작성 순서

소스 루트는 `Source/Mobile_defense_clone/`이다.

[공통 재현 절차](../COMMON.md#g0-replay)의 `-Role B`로 출발점에 제공 파일과 이 수업/G0-02 소스만 조립한다. 고정 소스는 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`이며 A나 통합 로더로 대체하지 않는다. 실제 작성은 아래1~4 → G0-02의 Network/Controller → 아래5~7 순서다. GameMode와 테스트 파일이 명령 코드에 의존하므로 두 수업을 합쳐 한 번 빌드한다.

1. `Data/LDMatchTypes.h`: 매치·참가자 식별과 Phase. 참가자 신원은 서버 발급 문맥이며 명령 payload에는 넣지 않는다.
2. `Data/LDGameData.h`: 읽기 전용 값 구조체와 조회 API를 A와 합의한다. `Data/LDGameData.cpp`: ReadFile → 타입 검사 → P0 추출 → ID·범위 검사 → 한 번의 스냅샷 교체 순으로 작성한다. 오류는 `OutError`로 반환하며 로드 실패를 기본값 성공으로 바꾸지 않는다.
3. `Core/LDGameState.*`: 서버만 MatchContext와 Phase를 바꾼다. Phase 전이 목록을 한 함수에서 검사한다. 클라이언트는 복제된 값을 읽는다.
4. `Core/LDPlayerState.*`: 공개 참가자 문맥의 복제본이다. 신원이 이미 있으면 다른 신원으로 덮어쓰지 않는다.
5. 다음 수업의 Processor·Controller 헤더를 작성한 후 `Core/LDGameMode.*`: InitGameState에서 데이터 → Processor → Preparing, PostLogin에서 2개 참가자 슬롯 → PlayerState → Processor 등록 → Controller 연결. GameMode는 서비스 UObject를 UPROPERTY로 보관한다.
6. Logout/EndPlay에서 명령 연결과 참가자 약한 참조를 정리한다. 서비스는 Controller/위젯을 역탐색하지 않는다.
7. `Mobile_defense_clone.Build.cs`에 Json·JsonUtilities 의존성을 추가한다. `Tests/LDCommandTests.cpp`의 BIndependentLoader를 작성하고 빌드 담당자에게 전달한다.

ARCH-01~03: GameMode 연결·GameState 공용 복제·로더 규칙·Processor 명령 기록이 분리돼 있다. ARCH-04: 로더 실패 시 기존 검증값이 보존된다. ARCH-05: UPROPERTY가 서비스 수명을 보장하고 참가자 참조는 Weak다. ARCH-06: 로더 계산 검증과 Editor/PIE 검증을 별개로 남긴다.

## Unreal 설정 순서

아래 조작은 실행 예정 절차이며 생성된 Blueprint·맵으로 표시하지 않는다.

| 순서 | 에디터 위치·에셋 | 부모/프로퍼티/연결과 값 | 이유·기대 화면 |
|---|---|---|---|
| 1 | 프로젝트 Content/LD/Data | B 로더가 읽는 GameRules, DT_Units, DT_Waves, DT_EnemyTypes, DT_SummonProfiles 5개; 공통 제공 입력은 DT_SpawnProfiles까지 6개 | B 소비 범위5개와 최종 canonical 제공 범위6개를 구분. 모두 data 원본과 동일 |
| 2 | Project Settings → Packaging | Additional Non-Asset Directories to Package에 LD/Data | 패키지의 UFS 경로 로딩. 공통 설정은 통합 담당자가 반영 |
| 3 | 기존 `/Game/TopDown/Lvl_TopDown` | 별도 검증 세션에서 World Settings → GameMode Override=`LDGameMode` | 기존 맵을 저장 변경하지 않고 G0 초기화 확인 |
| 4 | C++ GameMode 기본값 | GameStateClass=LDGameState, PlayerStateClass=LDPlayerState, PlayerControllerClass=LDPlayerController, DefaultPawnClass=None | G0는 카메라·플레이 가능한 유닛이 없다 |
| 5 | Tools → Session Frontend → Automation | `LD.P0.G0.Commands.BIndependentLoader` 실행 | 계산 검사. 실제 게임 화면 통과와 별개 |
| 6 | PIE 설정 | Number of Players=2, Net Mode=Play As Listen Server | Preparing 유지와 서로 다른 ParticipantIndex. 화면 구도는 G1에서 검사 |

Blueprint·UMG 생성/연결은 G0에 없다. 이 단계의 예상 관찰값은 Output Log의 `G0 B ready: 16 units, 10 waves; board/economy Stub keeps admission closed`와 GameState Phase=Preparing이다. 아직 실제 화면·PIE 관찰값은 없다.

## 실행·실패·수정 기록

새 detached 출발점의 수업 재현은 NotRun이다. 공통 절차의 assembly/editor/tests 결과를 실제 생성한 뒤 기존 제작 실행과 분리해 남긴다. B 필터 전체4개 중 `BIndependentLoader` 1개가 이 수업의 부분 검사이며 나머지3개는 G0-02다. 네 테스트를 통과해도 B GameMode 초기화/종료·PIE·RPC를 검증한 것으로 표시하지 않는다.

| 입력/조건 | 기대 결과 | 실제 결과 | 실행 범위·증거 |
|---|---|---|---|
| 신규 C++ 포맷 검사 | 새 파일 16개와 예시 파일에 오류 0 | Pass, 17 checked·48 legacy·0 errors | `Saved/P0Evidence/G0-B/style.log` |
| 현재 Content JSON | 활성16·10웨이브·18셀·인구20 로드 | 최초 Fail → 수정 후 Pass | `Saved/P0Runs/G0-B-tests-fix1/report/index.json` |
| 이미 로드한 뒤 존재하지 않는 디렉터리로 교체 | false·기존16종 유지 | Pass | 같은 자동화의 실패 사례 |
| Editor 빌드/UHT | 타입·복제 선언 컴파일 | 최초 및 수정 후 Pass | UE5.8.2/MSVC, `Saved/P0Runs/G0-B-editor-fix1/result.json` |
| PIE·PC 패키지·네트워크·Android | 각 게이트 지정 결과 | NotRun | 정적 검사로 대체하지 않음 |

작성 중 발견한 문제: 명령 종료 뒤 단순 준비 bool을 true로 되돌리면 접수가 재개될 수 있었다. Processor에 종료 latch를 추가하고 Close 뒤 재개 시도를 자동화에 넣었다. 실제 자동화는 Pass였다.

2026-09-18 실제 UE 자동화 최초 결과는 명령3종 Pass·BIndependentLoader Fail이었다. W10은 보스만 생성하므로 NormalCountPerGate=0과 SpawnIntervalSeconds=0이 맞는데, 모든 웨이브에서 양수 간격을 요구한 로더 조건이 이를 거절했다. `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`에서 음수는 거절하고, 일반 생성 수가 양수일 때만 0을 거절하도록 수정했다. 테스트에도 보스 전용10웨이브의 0/0 기대를 추가했다. 이는 제품 규칙 변경이 아닌 로더 결함 수정이다. 전체 UE 프로세스 exit code가0이었어도 JSON report.failed=1과 실제 로그를 보고 실패로 기록했다.

수정 후 실제 결과: `G0-B-editor-fix1` 13:35:23~13:35:43 KST Pass, `G0-B-tests-fix1` 13:36:28~13:36:45 KST 4Pass/0Fail/0NotRun. 실행환경은 Windows11 25H2·UE5.8.2·MSVC·Ryzen5 7500F·RAM32GB다. 자동화는 NullRHI이며 렌더링 성능·실제 게임 통과·Android 성능을 측정한 결과가 아니다. 각 result.json에 정확한 명령·프로젝트 경로·기준 SHA가 있고 build.log/engine.log에 전체 출력이 있다.

반복 실행은 통합의 [공통 실행 절차](../COMMON.md)에서 Build-P0Editor와 Test-P0Automation을 사용한다. 역할 검증은 ProjectRoot를 `C:/Users/iam12/P0_reference_b`, Filter를 `LD.P0.G0.Commands`, 새 RunId로 지정한다. 도구와 LD/Data staging은 공통 제공 파일이며 학습자가 직접 구현할 게임 코드와 구분한다. 이 역할 분기에는 공통 학습 홈 파일이 아직 없어 learning 검사에 누락4건이 있었으며, B 수업 링크/표 오류는 없었다. 통합 후 전체 검사를 다시 수행해야 한다.

## 상대에게 전달하고 통합하기

전달 커밋 `18b1ace8cb24bd915cf4a2d660047538fd7ab0c6`, 로더 수정 `03acb67e95a804b2d49e4f17fa3d4ec5a41dfd92`. A/B 공통 Data/Core 구현을 비교하고 한 원본을 선택한다. B 로더의 파일 소비는5개이며 DT_SpawnProfiles의 Early 행을 별도 검증하지 않는다. 통합은 명시적 정책·참조·좌표 검증과6개 입력을 갖춘 A 로더를 채택했다([선택 비교](../INTEGRATION.md)). 따라서 B 단독4Pass는 최종 통합 로더/데이터 검사를 대신하지 않는다. B 전용 Controller·Network·Tests는 다음 수업 순서로 통합한다. Blueprint 에셋은 이번 커밋에 없다. 통합 참고 SHA와 새 실행 증거는 통합 담당자가 기록한다. 실제 학습 통합 SHA는 아직 없다.

## 이해 확인

- Outer가 지정된 UObject를 UPROPERTY로도 보관해야 하는 이유는 무엇인가?
- 로더가 15번째 유닛에서 실패했을 때 공개 GetUnits 값은 언제 바뀌어야 하는가?
- 작은 변형: 복사한 테스트 데이터의 CellsPerPlayer를 17로 바꾸고 오류·기존 스냅샷 보존을 확인한다. 제품 데이터에는 이 변형을 반영하지 않는다.
- 다음 단계: 양쪽 Editor 빌드·로더 자동화·공통 비교 리뷰 후 통합 G0가 확인돼야 G1 실제 화면 작업에 진입한다.

## 단계 완료

- [x] 제공 파일과 직접 작성할 파일·설계 경계를 기록했다.
- [ ] 시작점에서 본 절차를 재현하고 실제 실행 결과를 연결했다.
- [ ] A 공통 구현과 비교·통합하고 Editor 검사를 통과했다.
- [x] G1·실기기와 구분한 미검증 범위를 명시했다.
