---
id: TECH-001
version: 0.2.8
status: Draft
owner: unassigned
updated: 2026-09-11
---

# UE5 구현·데이터·성능

[기획 허브](../README.md) · [작업 보드](../production/BOARD.md) · [검수 기준](../BACKLOG_QA.md)

문서 상태와 담당자는 위 메타데이터를 기준으로 한다. 사용자 확정 조건은 결정 기록을 따른다. 나머지 내용은 프로토타입용 설계안이다. 기획 작성 상태와 실제 구현 상태는 별도로 관리한다.

이 파일이 해당 기능의 편집 원본이다. 기존 GDD 장 번호를 유지해 이전 기획과 대응시킨다. 수치·데이터 타입은 허브의 원본 구분표를 따라 함께 변경한다.

## 15. UE5 프로젝트 구조

### 15.1 프로젝트 설정 방향

DEC-008에 따라 현재 `Mobile_defense_clone.uproject`와 C++ 모듈 `Mobile_defense_clone`을 유지하고 디펜스 전용 구조를 추가한다. 기존 TopDown·Strategy·TwinStick 템플릿은 초기 참고용으로 보존하고 사용 여부와 참조를 확인하며 정리한다. TASK-CORE-01은 현재 프로젝트의 기반 확인과 빌드 경로 검증으로 시작한다.

설치 환경과 프로젝트 시작 절차의 원본은 [개발 환경과 프로젝트 시작 안내](DEVELOPMENT_SETUP.md)다. 엔진 후보·고정 조건은 DEC-009, Windows 개발 도구의 공식 권장 기반 조합은 DEC-010, Android 도구의 검증 조합은 DEC-011을 따른다. 설치 확인과 기준 선택, PC·Android 빌드 성공을 구분한다.

DEC-012의 [PowerShell 빌드·실행 절차](BUILD_RUN.md)에 프로젝트 파일 생성, Editor·PC·Android 빌드 명령과 산출물·로그·검증 조건을 둔다. 실제 실행과 엔진 기준 고정은 해당 실행 증거로 확인한다.

기본 이동 캐릭터가 필요하지 않은 디펜스 전장에는 별도의 CameraPawn을 사용한다. 콘텐츠 제작은 Blueprint로, 소환·전투·재화·웨이브·네트워크 검증은 C++로 둔다. 핵심 규칙을 Level Blueprint에 넣지 않는다.

초기 플러그인은 Enhanced Input, Niagara, 프로젝트에서 쓰는 온라인 연결 모듈만 사용한다. GAS는 이 규모에서 필수가 아니며, 스킬 실행기와 상태 컴포넌트로 시작한다. 복잡한 조합형 능력 요구가 늘면 별도 검토한다.

### 15.2 클래스 책임

| 클래스·에셋 | 책임 | 권한 |
|---|---|---|
| ALDGameMode : AGameModeBase | 매치 상태, 참가자 등록, 승패 확정, 결과 생성 | 서버만 |
| ALDGameState : AGameStateBase | WaveIndex, Phase, WaveEndTime, ActiveEnemyCount, Result, 보스 요약 | 서버 작성·모두 복제 |
| ALDPlayerState : APlayerState | PlayerIndex, 표시 이름, 공개 자원 요약, 연결/봇 상태 | 서버 작성·모두 복제 |
| ALDPlayerController | 서버 명령 RPC, 개인 경제 상태 복제, UI 연결 | 서버+소유 클라이언트 |
| ALDCameraPawn | 카메라 프레이밍, 화면 좌표 변환 | 로컬 표시 |
| ALDBoardManager | 타일·개체 배치 상태, 원자적 배치 트랜잭션, 보드 버전 | 서버 작성·모두 복제 |
| ALDWaveDirector | 웨이브 데이터 해석, 스폰 예약, 마감 처리 | 서버만 |
| ULDBattleSubsystem : UWorldSubsystem | 적 등록, 전투 20Hz 스텝, 피해·효과·표적 조회 | 서버 로직; 클라이언트 표시 데이터는 복제 액터 사용 |
| ALDUnitActor | InstanceId, UnitId, CellId와 시각 상태 | 서버 생성·모두 복제 |
| ALDEnemyActor | EnemyId, HP, 경로 상태, 효과 요약 | 서버 생성·모두 복제 |
| ULDEconomyService | 가격·재화·강화·보장 카운터 계산 | 서버 소유 객체, RPC 없음 |
| ULDResultSaveService | 로컬 결과 중복 방지 또는 계정 서비스 연동 | 모드별 구현 |
| WBP_HUD 등 | 상태 표시, 명령 의도 전송 | 로컬만 |

UWorldSubsystem/UObject 서비스가 자동 복제된다고 가정하지 않는다. 네트워크 상태는 GameState·PlayerController·BoardManager·전투 액터에 둔다. GameMode는 서버에만 존재하고 공유 상태는 GameState로 전달하는 UE 구조를 따른다. [Game Mode and Game State](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-mode-and-game-state-in-unreal-engine)

### 15.3 폴더

```text
Content/LD/
  Core/            BP_GameMode, BP_GameState, BP_PlayerController
  Maps/            L_Boot, L_Lobby, L_Workshop, L_TestCombat
  Data/            DataTables, DA_GameRules, DA_UnitVisuals
  Units/           Base, Common, Rare, Epic, Legendary, Mythic
  Enemies/         Base, Normal, Boss
  Board/           Tiles, Spline, Decorations
  UI/              Lobby, Battle, Panels, Result, Common
  Input/           IA_*, IMC_Battle, IMC_Menu
  Art/             Meshes, Materials, Textures, Animations
  FX/              Niagara, Decals
  Audio/           SFX, Music, Mix
  Tests/           TestMaps, FunctionalTests
Source/Mobile_defense_clone/
  Core/  Battle/  Board/  Economy/  Network/  Data/  Save/
```

위 구조는 기존 프로젝트에 추가할 디펜스 전용 구성이며 아직 구현 완료를 뜻하지 않는다. 기존 런타임 모듈과 Game/Editor 타깃 이름은 `Mobile_defense_clone`을 유지한다. 표의 `ALD*`·`ULD*` 클래스 접두사는 디펜스 코드의 이름 규칙으로 사용하며 별도의 `LuckyWorkshop` 모듈 생성이나 이름 변경을 요구하지 않는다.

프로젝트 저장소는 코드·설정·콘텐츠 원본을 버전 관리하고, Binaries·Intermediate·Saved·DerivedDataCache는 생성물로 분리한다. 큰 바이너리는 팀에서 사용하는 LFS 또는 Perforce 정책을 첫날 정한다.

### 15.4 Blueprint 구현 순서

1. `BP_Board`에서 24개 칸 위치와 닫힌 Spline을 노출한다.
2. `BP_EnemyBase`는 EnemyActor의 데이터·시각만 연결한다.
3. `BP_UnitBase`는 UnitId에 맞는 Visual DataAsset을 읽는다.
4. `WBP_SummonButton.OnClicked`는 PC의 요청 함수만 호출한다.
5. PC 서버 처리 후 보드와 자원 RepNotify가 HUD를 갱신한다.
6. 획득 이벤트는 EventId를 포함해 VFX를 1회만 재생한다.
7. UnitVisual DataAsset으로 메시·AnimBP·VFX·아이콘을 교체한다.

단순 UI 이벤트는 Event Dispatcher로 연결하고, 매 프레임 바인딩으로 전체 DataTable을 검색하지 않는다. 개발 편의를 위한 임시 Blueprint 계산도 서버 권한 함수 안에 두어 멀티플레이로 바꾸면서 규칙을 다시 작성하지 않게 한다.

## 16. 서버 권한·복제·명령 처리

### 16.1 서버가 결정하는 것

RNG·소환 결과·재화·합성 소비·스킬 발동·HP·사망·웨이브·승패·보상은 서버가 결정한다. 클라이언트는 선택 강조·드래그 미리보기·카메라·소리·VFX를 처리한다. 확정 전 결과 모델이나 재화를 예측 확정하지 않는다. UE의 서버 권한 구조와 초기부터 멀티플레이를 고려하는 권고를 따른다. [Networking Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/networking-overview-for-unreal-engine)

### 16.2 명령 계약

DEC-021: 서버 권한·중복 소비 방지·재시도 계약은 프로젝트 기술 기준으로 유지한다. 아래 Single/Lock/Target/Swap은 기존 초안의 예약 계약이며 원작 기능의 존재를 입증하지 않는다. [원작 대조](../product/ORIGINAL_REFERENCE.md) 전에는 이 필드를 근거로 자체 버튼·기능을 추가하지 않는다. 원작에서 확인한 입력·결과에 맞춰 필요한 명령과 검수 기준을 함께 갱신한다.

DEC-020에 따라 공통 요청 정보와 명령별 payload를 분리한다. 아래는 구현할 논리 계약이며 실제 RPC/직렬화 코드를 생성한 것은 아니다. 명령 타입과 일치하는 payload 하나만 허용하고 필수값 누락·다른 타입의 필드·알 수 없는 enum은 거절한다.

```text
FLDCommand
  ConnectionEpoch: uint64        // 서버가 부여한 현재 연결 세대
  RequestId: uint32              // 세대 내 1부터 증가, 재시도에는 같은 번호
  CommandType: enum              // Summon, Merge, Craft, Sell, Swap, Upgrade, Move, Lock, Target
  ExpectedBoardRevision: int32   // Upgrade는 -1, 나머지는 현재 자기 보드 버전
  Payload: CommandType별 구조    // 아래 표의 필수 항목만

FLDCommandResult
  ConnectionEpoch: uint64        // 응답 대상 요청의 세대, 새 세대 발급과 구분
  RequestId: uint32
  ResultCode: enum
  NewBoardRevision: int32        // 이 응답을 확정한 시점의 자기 보드 버전
  EconomyRevision: int32         // 이 응답을 확정한 시점의 개인 경제 버전
  CreatedInstanceIds: uint64[]
  RemovedInstanceIds: uint64[]
  EventId: uint64                // 재전달 시 동일, 연출 없으면 0
```

#### 명령별 필수 정보

| 명령 | Payload | 서버가 확인·결정할 내용 |
|---|---|---|
| Summon | Source: enum Gold/Star | 모드에서 허용된 재화 종류인지 검사. 가격·확률 프로파일·보장·결과·배치 칸은 서버 결정. P0는 Gold만 허용 |
| Merge | SelectedInstanceId: uint64, ConsumedInstanceIds: uint64[3] | 선택 ID가 소비 목록에 포함되고 3개가 서로 다른 동일 소유·동일 UnitId인지 검사. 선택 개체의 서버상 칸을 결과 배치 기준으로 사용 |
| Craft | RecipeId: FName, ConsumedInstanceIds: uint64[3] | 정확한 서로 다른 재료 3개·레시피 일치 검사. 결과와 전설 재료의 배치 칸은 서버 결정. P1 |
| Sell | InstanceId: uint64 | 지정한 1개체만 판매, 환급액은 서버 데이터로 계산 |
| Swap | InstanceId: uint64, TargetUnitId: FName | 영웅/전설 재료 1개를 같은 등급의 다른 종류로 교환. 비용·횟수 서버 검사. 스택 위치 맞교환을 뜻하지 않음. P1 |
| Upgrade | UpgradeId: FName | 허용된 강화 항목·현재 레벨·비용·상한 검사 후 한 단계 구매. 목표 레벨·가격을 클라이언트가 지정하지 않음. P1 |
| Move | MoveMode: enum Stack/Single, SourceInstanceId: uint64, DestinationCellId: int32 | SourceInstanceId의 현재 칸을 서버에서 조회. Stack은 해당 칸 전체, Single은 해당 개체만 이동. 자기 목적지와 영향받는 모든 개체·수용량 검사 |
| Lock | InstanceId: uint64, DesiredLocked: bool | 해당 1개체의 원하는 잠금 상태 설정. 잠긴 개체에도 소유자의 해제 요청 허용 |
| Target | InstanceId: uint64, DesiredPriority: enum Nearest/BossFirst | 해당 1개체의 원하는 표적 우선순위 설정. 실제 적 ID·피해는 클라이언트가 결정하지 않음 |

InstanceId는 0이 아닌 서버 발급 논리 ID다. 합성·제작 목록은 정확히 3개이며 중복을 제거해 받아들이지 않는다. 목록 순서는 의미가 없고 서버는 RequestId 내용 비교 시 오름차순으로 정규화한다. 합성의 기준은 목록 첫 원소가 아니라 SelectedInstanceId다. 소비 대상이 바뀌면 다른 개체로 자동 대체하지 않고 거절·재선택한다. 데이터 ID는 UTF-8 기준 최대 64바이트, 공통 필드와 payload의 직렬화 크기는 전송 헤더를 제외하고 최대 1,024바이트를 초기 상한으로 사용한다. 디코딩 시 길이를 먼저 제한하고 FName은 해당 명령의 서버 허용 데이터 목록에서만 해석한다. 새 유닛 목록은 명령당 최대 1개, 소비 목록은 최대 3개다.

클라이언트는 플레이어 소유 PlayerController의 Server RPC로 보낸다. 소유권이 없는 BoardManager에 직접 서버 RPC를 호출하지 않는다. 발신자·플레이어 인덱스·현재 MatchId는 서버의 연결·참가자 문맥에서 얻는다. ConnectionEpoch는 그 문맥과 일치하는지 검사하는 값이며 클라이언트가 새 세대를 발급하지 않는다. 가격·RNG·결과 유닛·소유권을 payload로 지정할 수 없다.

Move의 Stack/Single 구분은 전송 계약이다. 4번 보드 규칙 검토는 임의 예외 선택 대신 원작 동작 대조로 진행한다. 미확인 재료 교환의 결과 배치를 먼저 설계하지 않는다. P0/P1 범위는 DEC-019를 따르되 DEC-021의 원작 대조 조건을 함께 적용하며, 계약에 존재해도 해당 모드에서 제외되거나 원작 근거가 없는 기능은 실행 대상으로 확정하지 않는다.

#### 처리 순서·버전

| 검증 순서 | 검사 |
|---|---|
| 1 | 연결·참가자·ConnectionEpoch 일치, payload 크기·타입·필수값·목록 길이 검사 |
| 2 | 요청 키와 정규화한 내용 비교. 캐시/처리 중 요청은 재실행 없이 응답, 오래된 번호는 거절 |
| 3 | 새 요청을 참가자별 직렬 처리에 등록. 명령 수 제한과 Preparing/Running·모드별 기능 허용 검사 |
| 4 | Upgrade 외에는 ExpectedBoardRevision 일치 검사. 각 DataId·CellId의 허용 범위 검사 |
| 5 | 정확한 개체의 존재·소유·명령별 잠금/예약 상태, 목적지 수용 가능 여부 검사 |
| 6 | 골드·별조각·강화 레벨·구매/교환 한도 검사 |
| 7 | 가상 적용으로 변경 후 보드 유효성 확인. 거절 시 게임 상태·RNG 변화 없음 |
| 8 | 자원·개체·RNG·변경된 Revision과 확정 응답을 함께 반영한 뒤 결과 전송 |

보드의 배치·구성·Locked·TargetPriority가 바뀐 트랜잭션은 자기 BoardRevision을 한 번 증가시킨다. 경제 상태가 바뀌면 EconomyRevision도 한 번 증가시킨다. 변경이 없는 거절·중복 응답·이미 같은 값인 Lock/Target은 버전을 증가시키거나 새 EventId를 만들지 않는다. 잠금·우선순위 요청도 버전 검사를 먼저 통과해야 한다. Upgrade는 보드 버전 비교를 하지 않고 서버의 현재 경제 상태로 구매 가능 여부를 검사한다. 자동 처치 보상으로 경제 버전이 바뀌는 것만으로 보드 명령을 거절하지 않는다.

#### 중복·응답 유실

개인별 동시 변경 요청은 클라이언트에서 1개로 제한하고 서버도 참가자별 명령 적용을 직렬화한다. 서버는 초당 8명령·순간 버스트 12명령의 기존 제한을 유지하며 재확인 트래픽에도 전송 제한을 적용한다. 핑은 별도 제한이다. 재전송은 같은 연결 세대·번호·내용을 보존하고 중복 클릭으로 새 요청을 생성하지 않는다.

요청 키는 서버 문맥의 `(MatchId, 참가자, ConnectionEpoch, RequestId)`다. 내용 비교에는 CommandType·ExpectedBoardRevision·명령별 모든 필수값을 포함한다. 최근 **확정 결과 256개**는 성공·거절 모두 내용과 응답을 캐시한다. 처리 중 요청은 별도로 보관하며 확정 전에 기록을 퇴출하지 않는다. 같은 키·같은 내용이 처리 중이면 Pending, 완료됐으면 원래 응답을 반환한다. 같은 키·다른 내용은 RequestIdConflict로 거절하고 원래 요청·응답을 보존한다. 결과 캐시 조회는 현재 Phase·보드 버전 검사보다 먼저이므로 성공 뒤 Result 전환이나 보드 변경이 있어도 원래 결과를 재전달한다.

캐시에서 빠진 요청의 실행을 막기 위해 세대별 HighestAdmittedRequestId를 별도로 보관한다. 유효한 형식의 새 번호를 등록할 때 최댓값을 갱신하고, 등록 후의 RateLimited·게임 규칙 거절도 확정 결과로 남긴다. 캐시·처리 중 목록에 없고 최댓값 이하인 번호는 RequestExpired로 거절한다. 이 거절은 과거 실행 여부가 미확인이라는 뜻이며 다시 실행하지 않는다. 새 번호는 최댓값보다 커야 한다. uint32를 순환 재사용하지 않고 소진 전 처리 중 요청을 정리한 뒤 서버가 새 세대를 발급한다.

응답이 불명확할 때 클라이언트는 같은 요청만 재확인한다. RequestExpired이면 응답 시점의 보드·경제 Revision 이상인 복제 상태를 각각 받은 후 사용자가 다시 선택하도록 안내한다. ConnectionEpoch가 바뀐 경우에도 이전 요청을 새 번호로 자동 재실행하지 않는다. 새 연결의 초기 상태 동기화가 끝나기 전에는 새 변경 입력을 막는다. 별도 결과 조회 명령은 추가하지 않고 기존 상태 복제를 사용한다. Pending이나 타임아웃을 실패·환불로 단정하지 않는다. 서버 재시작 등으로 캐시·번호 상한을 잃으면 동일 세대를 계속 사용하지 않는다. 이 계약은 프로세스 장애를 넘는 영속 명령 복구를 제공하지 않는다.

캐시 응답의 Revision·생성/소비 목록은 과거 결과 식별용이다. UI는 더 오래된 응답으로 최신 복제 상태를 덮어쓰거나 유닛을 다시 생성/제거하지 않는다. EventId는 매치 내에서 중복 재생을 막는다. 새 거절에는 생성/소비 목록을 비우고 EventId=0을 사용한다.

#### 결과 코드와 UI 대응

| 코드 | 의미·대응 |
|---|---|
| Success / NoChange | 정상 확정 / 원하는 Lock·Target 값과 이미 동일. 복제 상태를 반영하고 요청 종료 |
| Pending | 동일 요청 처리 중. 새 명령을 만들지 않고 대기 |
| RequestIdConflict | 같은 번호에 다른 내용. 실행·원래 캐시 변경 없이 거절, 클라이언트 요청 생성 오류 기록 |
| RequestExpired / InvalidEpoch | 기록이 없는 오래된 번호 / 연결 세대 불일치. 최신 상태 반영 후 재선택 |
| InvalidPayload / InvalidData / InvalidCell | 구조·필수값·enum 또는 데이터/칸 범위 오류. 요청 구성 오류 기록 |
| PhaseNotAllowed / FeatureDisabled | 현재 매치 단계 또는 P0/P1 모드에서 불허 |
| StaleBoard | 보드 버전 불일치. 최신 보드 반영 후 다시 선택 |
| MissingInstance / NotOwner / Locked / Busy | 개체 없음·소유권·소비 잠금·예약/이동 제한에 따른 거절 |
| NoSpace / InsufficientResource / LimitReached | 배치 공간·재화·구매/교환 상한 사유 표시 |
| RateLimited | 등록된 요청은 실행하지 않음. 대기 후 사용자 새 입력만 새 번호로 처리 |

별조각 도전의 추첨 실패는 명령 거절이 아니다. 비용·카운터를 정상 반영한 Success이며 생성 목록이 비어 있을 수 있다. 결과 연출로 실패를 표시하더라도 비용을 환불하거나 같은 요청을 새 추첨으로 처리하지 않는다.

### 16.3 복제 데이터

| 대상 | 빈도 목표 | 데이터 |
|---|---|---|
| 매치 상태 | 변경 시, 시간은 종료 시각만 | Phase, WaveIndex, WaveEndTime, 결과 |
| 공용 적 수·보스 HP | 5~10Hz 또는 의미 있는 변화 | 카운트, 두 보스 체력 |
| 보드 | 변경 시 | 슬롯의 InstanceIds, UnitId, 개체별 Locked·TargetPriority, BoardRevision |
| 개인 자원 | 변경 시 소유자 | Gold, Stars, 업그레이드, 보장 카운터, EconomyRevision |
| 아군 자원 요약 | 2Hz | Gold, Stars, 역할 태그 |
| 적 이동 | 초기 10Hz 기준 | 경로 거리, 속도, 서버 시각, 이동 상태 |
| 공격 연출 | 묶음 또는 낮은 빈도의 비신뢰 이벤트 | EventId, 공격자, 표적, VFX 종류 |

신뢰 RPC는 구매처럼 낮은 빈도의 명령과 응답에 사용한다. 모든 일반 공격을 Reliable Multicast로 보내지 않는다. 영속 상태가 진실이며 연출 패킷이 누락돼도 HP·유닛·승패는 맞아야 한다. 복제 도착 순서를 가정하지 않고 Revision을 비교하며, 연출 대상 액터가 아직 없으면 짧게 보류하거나 생략한다.

### 16.4 적 이동 동기화

서버가 이동 상태를 20Hz로 계산하고 클라이언트는 받은 경로 거리·시각·속도로 보간 표시한다. 일시 둔화·스턴·속도 변경은 새 기준점을 복제한다. 일반 위치 ReplicateMovement와 자체 경로 보간을 동시에 사용하여 두 체계가 위치를 덮어쓰지 않게 한다.

경로를 여러 바퀴 돌 수 있으므로 누적 이동 거리를 별도로 갖고 표시 위치만 `distance mod 4000`으로 변환한다. 3990→10 구간을 보간할 때 경로를 반대로 가로지르지 않도록 누적 거리 기준으로 보간한다. 논리 위치는 클라이언트 예측값을 받아 수정하지 않는다.

### 16.5 액터 풀링

첫 구현에서는 서버 권한 Spawn/Destroy로 정확성을 검증한다. 모바일 성능에서 생성 스파이크가 확인되면 적 최대 128개 및 VFX 풀을 도입한다. 풀링 시 HP, 효과, 타이머, TargetId, SpawnSerial, NetDormancy, 가시성, 충돌, GenerationId를 재초기화한다. 재사용된 액터에 이전 세대의 연출/피해 이벤트가 도착하면 버린다.

### 16.6 전용 서버 전환

리슨 서버는 제작 검증에 쓰고, 온라인 프로필 보상과 안정적 재접속이 필요한 P2에서 전용 서버로 전환한다. Epic의 전용 서버 튜토리얼은 엔진 소스 빌드와 C++ 멀티플레이 프로젝트를 요구하므로 이 준비를 P2 일정에 포함한다. [Dedicated Server 설정](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers-in-unreal-engine)

전용 서버는 배틀 상태만 유지한다. 계정 서비스·방 관리·서버 프로세스 배정·결과 저장은 별도 책임이다. 서비스 선택과 실제 운영 비용은 CCU 목표와 배포 지역이 정해진 뒤 산정한다. 이 문서에는 확인하지 않은 고정 서버 요금을 넣지 않는다.

## 17. 데이터·디버그·운영

### 17.1 데이터 원본

`data/DT_Units.json`, `DT_Skills.json`, `DT_Recipes.json`, `DT_EnemyTypes.json`, `DT_Waves.json`, `DT_SpawnProfiles.json`, `DT_SummonProfiles.json`, `DT_Upgrades.json`을 동봉한다. 행 이름은 Name이며, 문자열 ID는 FName으로 읽는다. 타입과 단위는 DATA_SCHEMA 문서가 정의한다.

DataTable을 가져오기 전에 대응 Row Struct가 필요하고 필드 이름을 맞춰야 한다. 기본 데이터 가져오기 방식은 Epic의 Data Driven Gameplay 문서를 참고한다. 시각 에셋은 별도 DataAsset에 연결하여 밸런스 표가 무거운 에셋을 직접 로딩하지 않게 한다. [Data Driven Gameplay](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-driven-gameplay-elements-in-unreal-engine)

### 17.2 재현성·버전

각 매치는 MatchId, BuildVersion, RulesVersion, DataHash, 서버 전용 MatchSeed를 기록한다. 플레이어별로 골드 소환·합성·별조각 도전 RNG 스트림을 분리해 상대의 입력 순서나 VFX·스폰 순서가 개인 추첨 결과를 바꾸지 않게 한다. 실서비스 추첨 결과는 서버에서 로그를 남기고 시드를 클라이언트에 배포하지 않는다.

동일 seed만으로 전체 3D 전투가 비트 단위로 재현된다고 약속하지 않는다. 재현 테스트에는 입력 명령 순서·서버 스텝·버전·스냅샷을 함께 사용한다. 테스트 시드는 Development 빌드에서만 직접 입력할 수 있다.

### 17.3 필수 디버그 화면

현재 웨이브, 적 수, 서버 스텝 시간, 골드·별조각, 소환 횟수, 두 보장 카운터, 유닛 수, 보드 버전, 선택 대상, 적 상태이상, 추첨 로그를 한 화면에서 확인한다. QA 명령은 특정 웨이브 진입, 자원 지급, 유닛 지급, 적 99/100/101개, 보스 HP 1, 패킷 지연 시나리오를 포함한다. Shipping에서 명령 진입점을 제거한다.

### 17.4 분석 이벤트

| 이벤트 | 필수 필드 |
|---|---|
| MatchStart | MatchId, BuildVersion, Mode, DeviceProfile, BotPresent |
| SummonResolved | PlayerIndex, Source, OutcomeTier, UnitId, PityBefore/After, Cost |
| MergeResolved | 입력 UnitId/3개, OutputId, Wave, Time |
| CraftResolved | RecipeId, Wave, Time |
| UpgradePurchased | UpgradeId, NewLevel, Wave |
| WaveEnded | Wave, AliveCount, TeamDamageByType, PlayerGold |
| MatchEnded | Result, Reason, CompletedWaves, Duration, 보스 잔여HP, BotSeconds |
| PerformanceSample | GameThreadMs, RenderThreadMs, GPUms, Memory, DeviceProfile |

로그는 디버깅에 필요한 최소 계정 식별자를 사용하고, 채팅 내용이나 장치의 불필요한 개인정보를 수집하지 않는다. 분석 이벤트가 실패해도 전투를 막지 않는다. 보상 기록은 일반 분석 로그와 별도의 신뢰 가능한 저장 경로로 처리한다.

### 17.5 초기 플레이 테스트 지표

튜토리얼을 완료한 신규 플레이어의 첫 일반 모드 승률 목표는 40~60%, 숙련된 고정 듀오는 70~90%로 가정한다. 아직 측정된 수치가 아니다. 신규 20팀 이상·숙련 10팀 이상의 반복 세션을 우선 모으고 표본이 작다는 점을 기록한다.

첫 합성 시간, 10웨이브 보스 실패율, 전설·신화 최초 시각, 재료 교환 사용률, 가득 찬 보드 체류 시간, 두 플레이어 피해 비중, 이탈 원인을 함께 본다. 높은 피해량만으로 지원 유닛의 기여를 평가하지 않는다. 평균 외에 최악·최선 시드와 하위 10% 결과를 따로 확인한다.

## 18. 성능 예산·검증

### 18.1 모바일 렌더링 기준

DEC-013에 따라 **Mobile Forward와 사전 계산한 조명 중심의 단순 라이팅**을 모바일 렌더링 기준으로 확정한다. 작은 고정 전장의 배경 조명을 베이크하고 실시간 광원·그림자 비용을 제한하는 제작 방향이다. 기존 Lumen·Nanite·Virtual Shadow Maps 비활성 제작안을 유지하며, 모바일 머티리얼과 일반 메시·LOD로 표현한다.

Directional Light 1개와 단순 환경광을 중심으로 구성한다. 정적 배경의 조명은 베이크하고 움직이는 수호자·적의 밝기와 바닥 그림자는 별도로 확인한다. 캐릭터 바닥 그림자 또는 제한된 동적 그림자를 사용하며, 기본 공격마다 Point Light를 생성하지 않는 기존 제작 기준을 유지한다. 베이크 조명만으로 움직이는 액터의 조명·그림자가 완성됐다고 판단하지 않는다.

Epic은 사전 계산 조명을 사용하는 프로젝트에 Mobile Forward를 권장하며, 복잡한 동적 조명에는 Mobile Deferred가 유리할 수 있다고 설명한다. 이번 선택은 현재 전장·조명 설계에 따른 판단이며 모든 기기에서 Forward가 더 빠르다는 의미는 아니다. 동적 광원 중심으로 아트 방향이 바뀌거나 기기 실측에서 병목이 확인되면 같은 장면·기기 조건으로 비교하고 결정을 갱신한다. [Epic 모바일 렌더링 비교](https://dev.epicgames.com/documentation/en-us/unreal-engine/mobile-rendering-and-shading-modes-for-unreal-engine)

현재 프로젝트는 `r.Mobile.ShadingPath=1`, `r.AllowStaticLighting=False`이므로 결정된 방향을 적용하기 위한 설정·맵 작업이 남아 있다. 적용할 키·현재값·후속 검증은 [개발 환경 안내](DEVELOPMENT_SETUP.md)에 기록한다. `r.ForwardShading`은 데스크톱 렌더러 설정이므로 Mobile Forward 선택과 구분한다. 이 결정 반영에서 Config·맵·머티리얼을 변경하거나 조명을 베이크하지 않았다. DEC-018로 초기 OpenGL ES 3.2·Mobile HDR On·MSAA 2x를 확정했으며 실제 적용·표시·기기 성능 검증은 남아 있다.

### 18.2 초기 예산

| 항목 | 목표·상한 |
|---|---|
| 일반 모바일 | 30FPS, 장시간 p95 프레임 33.3ms 이내 목표 |
| 상위 모바일 | 선택 60FPS, p95 16.7ms 이내 목표 |
| 서버 논리 | 20Hz, 스텝 50ms; 전투 처리 p95 10ms 이내 목표 |
| 테스트 액터 | 수호자 72 + 적 128 + 핵심 VFX 동시 부하 |
| GPU/스레드 | 30FPS 목표에서 각 병목 경로 p95 약 25ms 이하, 여유 포함 |
| 메모리 | 모바일 총 프로세스 1.2GB 이내를 첫 측정 목표로 설정 |
| Draw Call | 모바일 전투 화면 약 200 이하부터 시작, 실측 후 프로파일별 조정 |
| 네트워크 | 클라이언트당 평균 다운로드 40KB/s 이하를 초기 목표로 측정 |
| 로딩 | 전장 첫 로딩 10초 이하 목표, 캐시된 재진입 5초 이하 |

이 값들은 보증치가 아니다. 최소 지원 기기는 첫 실기기 빌드 결과를 바탕으로 확정한다. 기준 기기 제안은 Galaxy A54급 Android 6GB, 상위 비교 기기는 Galaxy S23급, iOS 후속은 iPhone 12급으로 둔다. 구매 권고나 성능 보장 의미는 없다.

### 18.3 측정 방법

에디터 FPS가 아닌 패키징한 Development 빌드에서 Unreal Insights, stat unit, stat gpu, 메모리·네트워크 프로파일을 기록한다. GPU와 게임 스레드 시간을 단순 합산해 프레임 시간으로 오해하지 않고 병목 경로를 판단한다.

최악 부하 장면에서 20분 연속 실행하고 평균·p95·최대 프레임, 메모리 증감, 열로 인한 성능 저하를 기록한다. 낮은 VFX 모드에서도 경고·상태·피해 판정이 동일해야 한다. 풀링·LOD·VFX 절감은 한 번에 한 항목씩 적용해 효과를 측정한다.

### 18.4 핵심 기능 검수

확률 합계·레시피 ID·데이터 타입 같은 정적 검증과 실제 UE 플레이 검증을 구분한다. 동봉 스크립트는 정적 데이터 및 경제 계산만 검사한다. 전투 밸런스, 모델 애니메이션, 서버 동기화, 모바일 성능 검증은 프로젝트 구현 후 수행해야 한다.

실행해야 할 상세 사례는 BACKLOG_QA에 제공한다. 특히 동일 개체 동시 합성, 가득 찬 보드 소환, 보장 직전 실패, 99↔100 적 수 경계, 보스 마감 시각 타격, 경로 한 바퀴 보간, 재접속 보상 중복을 필수로 둔다.
