# 준비된 유닛과 표시 — P0 / A / G2-A-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·정식 설계 | TASK-UNIT-01 / [A 구현](../../../docs/technical/IMPLEMENTATION_A.md), [공통 거래 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 / 실제 개발 상태 | Draft / Planned |
| 참고 시작/코드 SHA | `4861b987f3e2fe78bcc159d1b6a85008543a938b` / `932bf449641b3c28d56f708a1ea90c9b38ac2596`, 표시 보완 `5880ce91a75c3eb6adbaefbfc940bded18914308` |
| 실제 개발 시작/완료 SHA | 자기 G1 통합 결과 / 미생성 |
| 상대 산출물 | B BoardTypes `9658c55`, BoardManager `84389ce`; Schema2 / Rules0.3.0 |
| 제공 / 직접 작성 | 제공: G1 맵·M_P0Flat·Engine Cube/Sphere, B 배치 계약. 직접 작성: Battle/LDUnitActor.h/.cpp와 해당 자동화 |

## 이번에 만들 동작

보드가 유닛을 준비해도 아직 보이지 않고 복제·공격에 참여하지 않는다. 공동 확정 뒤 같은 actor를 활성화한다. 이후 수동 이동이나 판매 보충은 actor와 InstanceId를 유지하며 canonical 중심만 즉시 옮긴다. mesh는 Rules.VisualMoveSeconds(.15초) 동안 이동한다. 공격 타이머는 이 클래스에 없다.

## 코드 작성 순서

1. `Battle/LDUnitActor.h`: 배치 복사본과 표시 snapshot, InitializePrepared/ApplyCommittedPlacement/DeactivateCommitted를 선언한다. 공격력 행은 서버만 보관하고 필요한 사거리·공격 표현만 복제한다.
2. 생성자에서 CanonicalRoot, PresentationMesh, ProjectileMesh를 만들고 충돌·overlap을 끈다. 초기 Replicates=false, 표시 숨김이다.
3. InitializePrepared에서 식별·데이터·에셋·Transform을 검증하고 읽기 전용 UnitRow와 파생 배치만 저장한다. 재화·보드·서비스 등록을 호출하지 않는다.
4. ApplyCommittedPlacement는 B가 모든 실패 가능 준비를 마친 뒤 호출한다. 배치/중심을 적용하고 Replicates를 활성화한다. B는 셀의 전체 구성원을 InstanceId 오름차순으로 정렬하여 SetPresentationSlot(slot,Rules.VisualMoveSeconds)를 게시한다. InstanceId 나머지 연산으로 슬롯을 만들지 않는다.
5. SetLocalViewPlayerIndex는 로컬 연결부가 호출한다. 준비 전에는 숨긴다. 표시 변환은 공통 FLDViewTransform만 사용한다. 동일 셀3개체 슬롯은 mesh 오프셋이며 사거리 중심에는 더하지 않는다.
6. PresentCommittedAttack은 서버 확정 DamageEventId·예정 시각·목표 위치만 표시한다. 근접은 .18초 앞으로 흔들림, 투사체는 .18초 작은 구체 이동이다. 이것은 외형 시간이며 피해 지연이 아니다. 표시 완료 이벤트는 전투나 보상을 호출하지 않는다.
7. EndPlay에서 표시/Tick/머티리얼 참조를 정리한다. 준비 취소나 유닛 판매가 적 사망 이벤트를 발생시키지 않게 한다.

ARCH-01~06의 적용: 표시 actor와 보드/경제/전투 원본을 분리했고, BoardManager→UnitActor 단방향 준비·게시만 허용했다. 단일 배치 원본은 B이며 actor는 복사본이다. 준비 후 복제 게시, 구독 없는 표시 Tick, 종료 후 호출 거절을 실제 함수로 확인한다.

## Unreal 설정 순서

| 순서 | 위치 | 프로퍼티·값·연결 | 예상 화면 |
|---|---|---|---|
| 1 | native LDUnitActor Components | CanonicalRoot 아래 두 mesh, Collision=NoCollision, Overlap=false | 준비 중 숨김 |
| 2 | PresentationMesh | Engine Cube, scale(.50,.50,.60), M_P0Flat.Color | Common 녹색/Rare 파랑/Epic 보라/Legendary 금색 |
| 3 | ProjectileMesh | Engine Sphere, scale(.12,.12,.12) | Projectile 행에서만 짧은 구체 이동 |
| 4 | 로컬 표시 | 셀 중심+슬롯(-28,0)/(28,0)/(0,50), Z+35; owner1 Y반사 | 같은 셀3개체 구분, 자기 보드 아래 |
| 5 | BoardManager.PublishPrepared | 전체 셀 정렬 슬롯→배치 확정→통지 | 보충 뒤 남은 구성원의 슬롯도 갱신 |
| 6 | Core/LDLocalPresentationSubsystem | 기존·늦게 복제된 Unit에 SetLocalViewPlayerIndex | A가 Controller를 탐색하지 않음 |
| 7 | Blueprint/UMG/맵 | 이 수업에서 새 저장·부모 변경 없음; G1 L_P0 사용 | 조작 HUD는 B 수업에서 연결 |

## 실행·실패·수정 기록

| 조건 | 독립 기대 결과 | 실제 결과·범위 |
|---|---|---|
| 준비 성공/취소 | 확정 전 복제·표시·전투등록0 | 소스 구현; B 공동 확정 검수와 함께 확인 필요 |
| 동일 셀 슬롯 변경 | mesh만 이동, canonical 중심 불변 | 최초 통합 `UnitPresentationPreservesCanonical` Pass |
| canonical X0→140 이동 | 논리 즉시140, mesh .075초70/.15초140 이동량 | 같은 자동화 Pass |
| 표시와 중복 등록 | NextAttackAt 유지 | 최초 통합 자동화 Pass |
| EndPlay 후 로컬 표시 | false, 공격 불가 | 최초 통합 자동화 Pass |
| 소스 스타일 | 오류0 | 50파일 검사 Pass; Unreal 컴파일 증거가 아님 |

실행 SHA·원본 로그·전체 검사 실패와 후속 재검증 상태는 [A 공통 G2 증거](G2_EVIDENCE.md)에서 관리한다. 최초 최소 구현은 셀 중심에 mesh를 겹쳐 그렸다. 통합 사전 리뷰가3명 뭉치 식별과 .15초 이동 보간 누락을 발견해 `5880ce9`에서 수정했다. 실제 화면 실패를 재현한 기록은 아니다. 실제 조작·2인·패키지 검수 결과는 별도로 추가해야 한다.

## 상대에게 전달하고 통합하기

B는 헤더를 받은 뒤 준비 actor에 InitializePrepared를 호출하고 실패하면 공동 확정 없이 정리한다. 성공 시 BoardManager 게시 순서대로 슬롯/배치/통지를 적용한다. GameMode가 통지로 CombatService에 등록한다. A actor 파일은 학습 폴더에 의존하지 않는다. 참고 코드 SHA는 상단, 실제 학습자 작성·통합 SHA는 아직 없다.

## 이해 확인

- mesh 슬롯을 actor root에 더하면 왜 같은 뭉치의 공격 사거리가 달라지는가?
- Prepare에서 복제를 켜거나 공격 서비스에 등록하면 취소 거래가 무엇을 남기는가?
- 작은 변형: 테스트에서 슬롯을0→2로 바꾸고 canonical 중심/공격 시각이 같은지 확인한다. 제품 배치 규칙은 바꾸지 않는다.

## 단계 완료

- [x] 파일·설정·제공 범위와 연결 계약을 기록했다.
- [ ] 시작점에서 수업을 재현하고 실제 결과와 SHA를 연결했다.
- [ ] B 보드·실제 두 프로세스와 합쳐 표시·조작을 확인했다.
- [x] 미검증은 후속 코드 재검증·수업 재현·실제 화면·패키지·Android이며 다음 수업은 서버 전투다. Verified가 아니다.
