# 영속 적 개체와 로컬 표시 — P0 / A / G1-A-02

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·설계 | TASK-MAP-01, PLAN-ROUTE-01/PLAN-VIS-02 / [A-04 설계](../../../docs/technical/IMPLEMENTATION_A.md#a04) |
| 참고 자료 제작 상태 / 실제 개발 상태 | Draft / Planned |
| 참고 시작/코드 SHA | `4787bf1a3a0d866aa206d148586594b3c710f957` / `bce4b7b0abe7787e1efe54e9af8805612bdffe5d` |
| 실제 개발 시작/완료 SHA | 자기 G0 통합 결과 사용 / 미생성 |
| 필요한 상대 산출물 | 경로 모델, B 로컬 참가자 준비 통지·전장/카메라, 루트의 명시적 G1 시연 fixture |
| 제공 / 직접 작성 | 제공: Engine Sphere, `/Game/LD/Materials/M_P0Flat` Color 머티리얼. 직접 작성: Battle/LDEnemyActor.h/.cpp, Tests/LDRouteTests.cpp의 Actor/View 테스트 |

## 이번에 만들 동작

서버가 생성한 적 하나가 중앙과 자기 보드 둘레를 두 바퀴 돌아도 MatchId/EnemyId/RouteIndex와 actor가 유지된다. 마지막 서버 거리·시각만 복제하고, 로컬 참가자1은 표시 mesh의 Y를 반사하여 자기 보드를 아래로 본다. canonical root는 그대로다. 이 단계는 적의 이동만 구현하며 HP·피해·처치 보상은 없다.

## 코드 작성 순서

1. `Battle/LDEnemyActor.h`: FLDEnemyRouteSnapshot에 MatchId/EnemyId/RouteIndex/TotalDistanceCm/SampleServerSeconds/SpeedCmPerSecond/bActive를 선언한다. actor API는 InitializeRoute, AdvanceRouteTo, StopRoute, SetLocalViewPlayerIndex, const snapshot 조회다.
2. 생성자에 CanonicalRoot와 PresentationMesh를 만든다. collision/overlap은 비활성, ReplicateMovement=false다. 경로점은 초기 복제, snapshot은 갱신 복제다. 표시 Tick이 권위 이동을 계산하지 않는다.
3. InitializeRoute는 서버 권한/식별자/경로/속도/시각을 검증한다. 초기화 실패 전까지 복제를 끄고, 모든 필드와 모델이 준비된 뒤 SetReplicates(true)를 호출한다. 이는 initial-only 경로점이 빈 배열로 먼저 전송되는 수명 결함을 막는다.
4. AdvanceRouteTo는 외부20Hz 연결부만 호출한다. `현재시각-초기시각`에서 누적거리를 구하여 한 번의 늦은 호출도 여러 구간·바퀴를 넘을 수 있다. 같은 시각은 no-op, 이전/비유한 시각은 거절한다. 재초기화는 같은 값일 때만 현재상태를 유지하고 다른 ID·경로는 거절한다.
5. OnRep_RoutePoints/OnRep_RouteSnapshot은 도착 순서가 달라도 두 입력이 준비되면 canonical root와 표시를 갱신한다. 클라이언트는 권위 이동을 호출하지 못한다.
6. SetLocalViewPlayerIndex를 루트 bootstrap에서 호출한다. 준비 전 mesh를 숨긴다. RefreshPresentation에서 예측 raw 위치를 계산하고 표시mesh만 Y반사·높이35cm로 이동한다. A 역할은 명시적 identity/Y반사 fixture이며 통합에서는 B의 `FLDViewTransform::ToPresentation`으로 같은 결과를 연결한다.
7. StopRoute는 활성상태를 내리고 이후 초기화/진행을 막는다. 정지는 사망이 아니며 마지막 위치를 표시한다. EndPlay는 tick과 표시를 끄고 material 참조를 해제한다. actor 자체 타이머·전투 이벤트·Controller 구독은 없다.

ARCH-01/02: 모델/권위 actor/로컬 bootstrap을 분리하고 actor에서 Controller를 찾지 않는다. ARCH-03: RouteSnapshot은 서버 원본, 표시mesh는 파생 값이다. ARCH-04: 준비→복제 게시 순서다. ARCH-05: 외부 진행 타이머는 생성자인 루트 fixture/후속 service가 해제하고 actor는 Stop/EndPlay를 처리한다. ARCH-06: transient world의 실제 Actor 검사와 두 프로세스 화면 검수를 구분한다.

## Unreal 설정 순서

| 순서 | 위치·에셋 | 값·연결 | 예상 관찰 |
|---|---|---|---|
| 1 | native LDEnemyActor Components | CanonicalRoot→PresentationMesh | root는raw, mesh만 표시 변환 |
| 2 | PresentationMesh | `/Engine/BasicShapes/Sphere.Sphere`, scale0.45, Collision=NoCollision, GenerateOverlap=false | 반지름22.5cm 구체 |
| 3 | material slot0 | `/Game/LD/Materials/M_P0Flat.M_P0Flat`, dynamic parameter `Color` | Route0 주황(1,.35,.08), Route1 하늘색(.05,.7,1) |
| 4 | 표시 위치 | raw 변환 뒤 Z+35cm | 바닥 위 두 색 구체, root Z는0 |
| 5 | `/Game/LD/Maps/L_P0` | 루트 소유 통합 맵. B 전장/카메라와 G1 명시적 `-P0Probe=G1` fixture 연결 | 서버ID1001/1002가 각 경로로 이동; 구체 둘은 중앙에서도 서로 다른 ID |
| 6 | 외부 시연 연결부 | N01 speed150, route0/1, StartServerTime, 20Hz AdvanceRouteTo; local index 준비 때 SetLocalViewPlayerIndex | 두 화면 자기보드 아래·생성왼쪽·중앙좌→우 |
| 7 | Blueprint·UMG | A는 새 Blueprint/위젯/맵 저장 없음 | 기본 C++ actor를 스폰하며 HUD/입력은 B 소유 |

에디터/포트/맵 조작은 루트가 직렬 수행한다. 단독 A actor 수업은 실제 두 화면을 제공하지 않는다. 위 예상 화면의 실측·캡처는 통합 G1 실행 후 기록해야 한다.

## 실행·실패·수정 기록

| 사례 | 기대 결과 | 실제 결과·증거 |
|---|---|---|
| 두 actor를6440cm까지 진행 | 총actor2, 같은ID/RouteIndex, 원래 보드 복귀 | Pass, `ActorIdentityTwoLapsAndStop` |
| 같은초기화/같은시각 | 거리·개체 보존 | Pass, 같은 테스트 |
| 다른ID/경로·역행·NaN | false, 원본 불변 | Pass, 같은 테스트 |
| Stop 두 번·종료후진행·준비취소뒤초기화 | 추가진행0, 재시작0 | Pass, 같은 테스트 |
| Player1 view/예측 | mesh만Y반사, canonical root·누적거리 불변 | Pass, `LocalViewDoesNotMutateCanonicalState` |
| simulated proxy의 초기화/진행 | 서버권한 거절 | Pass, 같은 테스트; 실제 네트워크 검수는 별도 |
| EndPlay 후 표시 콜백 | mesh숨김, 표시재개거절 | Pass, 같은 테스트 |
| 스타일/공백 | 오류0 | Pass,24 checked/48 legacy/0 errors, git diff --check |
| Editor | 컴파일/UHT/링크 성공 | Pass,20.25초; [실제 증거](G1_EVIDENCE/README.md) |
| PIE·PC2인·화면비·Android | 각각 필수 검수 | 모두 NotRun |

작성 중 수명 검토에서 초기 경로점이 빈 채 먼저 복제될 위험을 확인하여 준비 완료 후 복제를 활성화했다. 실제 실패를 재현했다고 기록하지 않으며 actor 자동화에서 준비 전복제false/준비후true가 Pass였다. 표시예측 상한은 계산 테스트가 통과했지만 실제 지연 상태의 화면 품질·오차·성능은 미측정이다.

## 상대에게 전달하고 통합하기

필수 API 선언은 `Battle/LDEnemyActor.h`, 코드 SHA는 상단 표다. 루트는 InitializeRoute(MatchId,1001/1002,0/1,Points,150,StartTime), 20Hz AdvanceRouteTo를 연결한다. B의 Controller 준비 이벤트에서 기존/늦게 복제된 actor 모두 SetLocalViewPlayerIndex를 받도록 bootstrap한다. B Controller에 A 구체 의존을 추가하지 않고 루트 연결부가 두 API를 연결한다.

통합 순서: A 역할 Editor/5자동화 → B 전장/뷰 변경 통합 → A의 명시적 Y반사 fixture를 공통 변환으로 연결 → 통합 Editor/자동화 → PC 두 화면의2바퀴/18셀전수/상대조작거절/화면비 검사. 종료는 외부 타이머 해제 후 actor StopRoute와 정리다. 통합/실제 학습 SHA는 미확정이다.

## 이해 확인

- 화면의 mesh 위치를 actor root에 그대로 쓰면 상대 참가자의 전투 거리 판정이 왜 달라지는가?
- initial-only 경로점을 초기화보다 먼저 복제하면 늦게 초기화된 actor가 왜 영구히 보이지 않을 수 있는가?
- 작은 변형: transient fixture의 표시시각을 서버 sample보다5초 앞으로 보내고 mesh와 canonical 위치를 각각 출력한다. 제품 속도/경로는 바꾸지 않는다.
- 다음 진입 조건: 실제 G1 양쪽화면/두바퀴/입력 전수통과. 이전 컴파일 Pass만으로 G2를 시작하지 않는다.

## 단계 완료

- [ ] 시작점에서 수업 설명대로 재현했다.
- [x] 실제 Unreal 컴파일·NullRHI 자동화와 SHA를 연결했다. 양쪽 화면은 미실행이다.
- [ ] B 전장과 통합하여2바퀴를 확인했다.
- [x] 미검증 범위·표시fixture 제거지점·다음 진입 조건을 명시했다.
