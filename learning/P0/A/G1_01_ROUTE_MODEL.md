# 닫힌 선형 경로 계산 — P0 / A / G1-A-01

## 상태와 기준점

| 항목 | 값 |
|---|---|
| 상위 TASK·설계 | TASK-MAP-01 중 경로, PLAN-ROUTE-01 / [공통 경로 계약](../../../docs/technical/IMPLEMENTATION_SHARED.md) |
| 참고 자료 제작 상태 / 실제 개발 상태 | Verified — 파일 조립·Editor·자동화·G1 두 프로세스 재현 범위 / Planned |
| 참고 재현 시작/완료 소스 SHA | `649c1dedd6832c41089a76b59bc76518cd262296` / `df8a2f27dd962a4d9f9f4051e51f3332245ba40a` |
| 실제 개발 시작/완료 SHA | 자기 G0 통합 결과 사용 / 미생성 |
| 필요한 상대 산출물 | G0 검증된 GameRules2/0.3.0, B의 동일 데이터 전장 표시 |
| 제공 / 직접 작성 | 제공: Content/LD/Data와 G0 공통 타입. 직접 작성: Battle/LDRouteModel.h/.cpp, Tests/LDRouteTests.cpp의 계산 테스트 |

## 이번에 만들 동작

전장 경로의4점을 닫힌 선형 구간으로 읽고 누적거리에서 위치·방향·완료 바퀴 수를 구한다. 데이터의3080cm마다 시작점으로 돌아오지만 누적거리는 계속 증가한다. 같은 중앙 구간을 지나더라도 별개의 경로 정의를 유지한다.

규칙과 독립 기대좌표는 [G1 준비 메모](G1_ROUTE_NOTE.md)의 한 표에서 관리한다. Spline 대신 데이터 그대로 선형 계산을 사용하며, 화면 표시/입력용 반사는 이 계산에 넣지 않는다. 이 구조는 Actor/World 없이 코너·경계와 여러 바퀴를 확인할 수 있다.

## 코드 작성 순서

1. `Battle/LDRouteModel.h`: FLDRouteModel의 TryInitialize/TrySample, 읽기 전용 길이/점 조회를 선언한다. 위치·접선·Lap 출력은 성공했을 때만 바꾼다.
2. `Battle/LDRouteModel.cpp`: TryInitialize에서 최소3점·유한 좌표·0이 아닌 각 구간을 검사한다. 마지막→첫 점 구간을 포함한 누적길이를 임시 배열에 만들고 모두 성공한 뒤 교체한다. 실패한 재초기화는 기존 검증된 경로를 보존한다.
3. TrySample은 누적거리를 검증하고 `floor(거리/길이)`와 `fmod(거리,길이)`를 따로 계산한다. 구간 선택은 끝점보다 작은 구간으로 하여 정확한 코너에서는 다음 진행 방향을 사용한다.
4. TryPredictPresentationDistance는 sample과 화면 서버시각의 차이를0~0.25초로 제한한다. 늦은 네트워크 갱신 중 표시 예측을 제한하는 값이며 적의 실제 거리나 속도를 바꾸지 않는다. 정지상태는 예측0이다.
5. `Tests/LDRouteTests.cpp`의 PolylineCornersAndTwoLaps/InvalidInputPreservesOutput/DelayedPresentationClock을 작성한다. 기대좌표는 코드 결과를 다시 사용하지 않고 명세 숫자를 직접 적는다.

호출 흐름: 서버 초기화 때 데이터4점→모델 검증/구간 캐시→서버 시각에서 구한 누적거리→TrySample→canonical 위치. 표시 흐름은 마지막 복제 sample→최대0.25초 예측거리→같은 TrySample→B의 표시 변환이다. 모델은 이동 상태의 원본이나 네트워크 권한을 갖지 않는다.

## Unreal 설정 순서

| 순서 | 위치 | 값·연결 | 관찰 |
|---|---|---|---|
| 1 | C++ Editor 타깃 | 기존 모듈에 두 모델 파일 추가, Build.cs 추가 의존성 없음 | 실제 Editor compile Pass |
| 2 | Tools → Test Automation | `LD.P0.G1.Route` | 호스트 시각 회귀를 포함한6개 Pass; 전체22개 재현에 포함 |
| 3 | Blueprint·UMG·맵 | 이 수업에서 변경 없음 | World 없는 경로 계산이므로 화면 캡처를 생성하지 않음 |

## 실행·실패·수정 기록

| 사례 | 독립 기대 결과 | 실제 결과·증거 |
|---|---|---|
| 0/560/1540/2100/3080/6160cm | 지정 코너·원래 보드 복귀, Lap0/1/2 | Pass, `PolylineCornersAndTwoLaps` |
| 코너 전후0.001cm | 점프 없이 연속, 정확 코너는 다음 구간 접선 | Pass, 같은 테스트 |
| 음수/NaN/무한거리·범위 초과Lap | false, 호출자 출력 불변 | Pass, `InvalidInputPreservesOutput` |
| 점 부족·중복 인접점 | false, 이전 정상 경로 보존 | Pass, 같은 테스트 |
| 100ms 지연, 속도150 | 표시거리+15cm | Pass, `DelayedPresentationClock` |
| 긴 지연/역행 시각/정지 | 최대+37.5cm / 추가예측0 / 추가예측0 | Pass, 같은 테스트 |
| clang-format20.1.8 검사 | 오류0 | Pass: 24 checked,48 legacy,0 errors; 코드 커밋 전 실행 |
| git diff --check | 공백 오류0 | Pass |

최초 A 역할의 Editor·5자동화 [증거](G1_EVIDENCE/README.md)와 별도로, 새 작업 경로에서 수업 순서로 파일을 조립하여 Editor Pass(88.39초), 전체22자동화 Pass/0Fail/0NotRun과 실제 두 프로세스 화면 검수를 통과했다. 시작점·파일별 blob·로그·화면·제한은 [공통 재현 결과](../evidence/G1_REPLAY/SUMMARY.md)를 따른다. 호스트 표시 시각 실패의 원인·수정·회귀 Pass는 [Actor 수업](G1_02_ENEMY_ACTOR.md)에 기록했다. 0.25초 넘는 지연에서는 표시가 마지막 sample 앞으로37.5cm까지 진행한 뒤 새 sample을 기다린다. 인위적 네트워크 지연 상태의 화면 품질은 아직 미측정이다.

## 상대에게 전달하고 통합하기

경로 모델은 B 헤더·Controller·Spline에 의존하지 않는다. A actor가 모델을 사용하고 B는 동일 원본 좌표로 전장을 표시한다. 실제 API 선언은 `Battle/LDRouteModel.h`에 있다. 통합 완료 소스 SHA는 상단 표, 연결 순서는 [G1 통합 수업](../G1_INTEGRATION.md)을 따른다. 실제 학습자의 작성·통합 SHA는 미생성이다.

## 이해 확인

- 누적거리를 fmod 결과로 덮어쓰면 두 바퀴 추적과 시간 복구에서 무엇을 잃는가?
- 정확한 코너의 접선을 이전 구간으로 잡으면 회전 표시가 어떻게 달라지는가?
- 작은 변형: 검증 전용 삼각형 경로에서 마지막→첫 점 구간을 직접 손계산하고 TrySample과 비교한다. 제품4점은 바꾸지 않는다.
- 다음 단계는 같은 코드로 Actor 수업을 재현한다. 참고 구현은 두 참가자 화면·입력·두 바퀴 검수를 통과하여 G2 진입 조건을 충족했다. 학습자는 자기 구현에서 같은 증거를 남긴 뒤 진행한다.

## 단계 완료

- [x] 새 작업 경로에서 시작점과 수업 순서로 참고 파일을 조립하여 재현했다. 학습자 작성 이력은 아니다.
- [x] 실제 Unreal 컴파일·NullRHI 자동화 결과와 소스 manifest를 연결했다.
- [x] 상대 전장과 통합한 실제 두 프로세스에서 양쪽 경로·두 바퀴를 확인했다.
- [x] 미검증 범위와 다음 진입 조건을 명시했다.
