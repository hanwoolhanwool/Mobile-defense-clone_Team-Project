---
id: TECH-CODING
version: 0.1.0
status: Baseline
owner: Codex
updated: 2026-09-14
reviewed: 2026-09-14
review_run: RUN-20260914-04
applies_to: 신규·수정 C++와 Blueprint 작성·리뷰
baseline_basis: DEC-025 사용자 코드 규약·포맷 설정·자동 검사 작성 지시
---

# C++·Blueprint 코드 작성 규약

[기획 허브](../README.md) · [이름·폴더·데이터 규칙](NAMING_AND_STRUCTURE.md) · [포맷 설치·검사](CODE_STYLE.md)

이 문서는 이 프로젝트에서 새 코드를 작성하거나 기존 코드를 수정할 때 사용하는 기준이다. Epic의 C++·Blueprint 권장 사항을 바탕으로 프로젝트 규칙을 정했다. 게임 기능의 구현 여부와 수치는 [아키텍처](ARCHITECTURE.md), [데이터 명세](../DATA_SCHEMA.md), DEC-021의 원작 대조 조건을 따른다.

`필수`는 작성·리뷰 기준이다. 자동으로 강제하는 범위는 [코드 스타일 검사](CODE_STYLE.md)에 따로 표시한다. 규칙 예외가 필요하면 PR에 대상·이유·영향·후속 조치를 적고 규약 변경을 함께 검토한다. 주석으로 포맷 검사를 끄거나 기존 파일 기준 목록에 새 코드를 추가해 통과시키지 않는다.

## 1. C++ 파일과 서식

- UTF-8 BOM 없음, LF, 파일 끝 개행. 들여쓰기 탭 1개=4칸, 줄 길이 목표 120칸, Allman 중괄호를 사용한다. 긴 문자열·매크로 등 분리할 수 없는 구문은 포맷 결과를 따른다.
- `if/else`, 반복문에는 본문이 한 줄이어도 중괄호를 쓴다. 자동 포맷은 중괄호를 추가하지 않으므로 리뷰에서 확인한다.
- 클래스 헤더/구현은 같은 기본 이름의 `.h`/`.cpp`로 묶는다. 주 클래스 하나를 기준으로 나누고, 관련 소형 enum/struct는 같은 헤더에 둘 수 있다.
- 헤더는 `#pragma once`, 필요한 include, 전방 선언, 타입 선언 순서다. 리플렉션을 쓰는 헤더의 `파일명.generated.h`는 **마지막 include**로 둔다. 타입 선언보다 앞의 전방 선언은 그 뒤에 올 수 있다.
- `.cpp`의 첫 include는 대응하는 자기 헤더다. 이후 필요한 프로젝트·엔진 헤더를 직접 포함한다. 다른 헤더나 Unity 빌드의 우연한 포함에 의존하지 않는다. 새 모듈 의존성은 Build.cs에 추가 이유를 남긴다.
- UHT가 처리하는 타입을 사용자 namespace 안에 넣지 않는다. `.cpp` 내부 전용 보조 함수·상수는 익명 namespace로 숨길 수 있다.
- 클래스 멤버는 공개 API, protected 확장 지점, private 내부 상태로 묶는다. 멤버마다 명시적으로 초기값을 정하고 `override`, 가능한 `const`를 사용한다.
- 새 원본에 타사의 저작권 문구를 복사하지 않는다. 기존 템플릿의 라이선스·저작권 표기는 보존한다.

서식 기본 근거: [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine). 120칸 제한과 적용 범위는 프로젝트 선택이며 포맷의 실제 결과는 고정된 `.clang-format`이 결정한다.

## 2. 이름·타입·API

이름 표와 폴더의 원본은 [이름·폴더·데이터 규칙](NAMING_AND_STRUCTURE.md)이다. C++ 식별자는 영어 PascalCase, bool 변수는 `b` 접두사를 사용한다. 함수는 `TryPlaceUnit`, `GetWaveIndex`, `IsConfigured`, 이벤트 수신기는 `Handle...`처럼 역할이 드러나게 쓴다. 축약어·`Temp`, `Manager2`, `NewFunction` 같은 임시 이름을 제품 코드에 남기지 않는다.

직렬화·복제 경계에는 `int32`, `uint32`, `uint64` 등 명시적 크기의 타입을 쓴다. ID는 `FName`, 화면 표시에는 현지화 가능한 `FText`, 문자열 처리에는 `FString`을 기본으로 한다. 기존 데이터의 `FString DisplayName` 등 계약은 자동 변경하지 않는다. 단위는 새 API 이름에 `Seconds`, `Cm`, `Ratio`로 표현하고, `0~1`과 `0~100`을 혼용하지 않는다.

읽기 전용 복합 입력은 보통 `const FType&`, 작은 값은 값 전달을 사용한다. 출력 인자는 `OutResult`처럼 표시한다. `auto`는 우변에서 타입이 분명하거나 iterator처럼 긴 경우 사용한다. 오류가 정상적으로 발생할 수 있는 명령은 실패 이유가 있는 결과 타입을 반환하고, 성공인 척 기본값을 반환하지 않는다.

## 3. UObject 수명·Blueprint 노출

| 상황 | 기준 |
|---|---|
| UObject가 소유하는 리플렉션 멤버 참조 | `UPROPERTY()`와 `TObjectPtr<T>` 사용. 참조가 GC에 노출되어야 함 |
| 수명을 연장하면 안 되는 관찰·캐시 | `TWeakObjectPtr<T>`, 사용 시 유효성 확인 |
| 지연 로딩할 콘텐츠 참조 | `TSoftObjectPtr<T>`/`TSoftClassPtr<T>`, 로딩 완료·실패·소유자 종료 처리 |
| 함수 인자·짧은 범위의 참조 | raw pointer 가능. 멤버 장기 보관과 구분 |
| 일반 C++ 자원 | UE의 `TUniquePtr` 등 명확한 소유권 사용. UObject에 일반 shared/unique 소유권을 적용하지 않음 |

타이머·delegate·비동기 콜백은 소유자 종료 시 해제하거나 약한 참조로 유효성을 확인한다. `EndPlay` 이후 콜백이 상태를 변경하지 않게 한다. UObject를 `new/delete`로 관리하지 않고 대상에 맞게 `NewObject`/`SpawnActor`를 사용한다. 근거: [Unreal Object Pointers](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine).

Blueprint 노출은 사용할 API에 한정한다. 조회 값은 `BlueprintReadOnly`, 실행 진입점은 `BlueprintCallable`, 부작용 없는 조회만 `BlueprintPure`로 노출한다. `BlueprintReadWrite`로 서버의 재화·전투 상태를 직접 수정하게 하지 않는다. `Category = "LD|기능"`과 편집 범위(`EditDefaultsOnly`, `EditInstanceOnly` 등)를 명시한다. 범위 제한 메타데이터는 편집 편의이며 서버 입력 검증을 대체하지 않는다.

리플렉션 서식 예시는 [LDStyleExample.h](../../tools/style/LDStyleExample.h)를 참고한다. 이 파일은 포맷 검사 샘플이며 게임 모듈에 컴파일되거나 DataTable로 가져온 결과가 아니다.

## 4. 실행 비용·권한·오류

- 소환·전투·재화·웨이브·저장 결과의 확정은 C++의 서버 권한 경로에서 처리한다. 클라이언트는 요청과 시각 피드백을 담당한다. 상세 클래스 책임과 RPC 계약은 [아키텍처](ARCHITECTURE.md)가 원본이다.
- 서버 RPC는 `ServerRequest...`, 소유 클라이언트 통지는 `Client...`, 멀티캐스트는 `Multicast...`, 복제 알림은 `OnRep_...`로 구분한다. 이름은 권한을 보장하지 않는다. 적절한 `UFUNCTION` 지정·복제 객체·소유 연결과 서버 검증을 함께 구현한다.
- 서버는 소유권·요청 형식·현재 상태·비용·중복 요청을 검증한다. 클라이언트가 보낸 결과·가격을 그대로 적용하지 않는다. 서비스 UObject/Subsystem이 자동 복제된다고 가정하지 않는다.
- Tick은 필요한 객체만 켠다. 매 프레임 전체 액터·DataTable 조회, 반복 로드, 대량 할당을 피하고 이벤트·타이머·캐시를 사용한다. 변경의 효과는 실제 프로파일링으로 판단한다.
- `check`는 개발 불변식, `ensure`는 진단용으로 사용한다. 사용자 입력·네트워크 실패·에셋 누락은 명시적인 오류 분기로 처리한다. Shipping에서 제거될 수 있는 검사에 필수 부작용을 넣지 않는다.
- 새 기능의 로그 카테고리는 `LogLD기능`으로 구분한다. 오류에는 요청 ID·대상 ID·실패 이유를 남기고 매 Tick 반복 로그는 피한다. 토큰·비밀번호·개인 계정 정보는 로그에 쓰지 않는다.
- 주석은 제약·이유·단위·수명·권한을 설명한다. 한국어 설명을 허용하고 코드 식별자는 영어로 유지한다. TODO에는 연결 작업 ID 또는 이슈와 완료 조건을 남긴다.

## 5. Blueprint 작성

Blueprint는 콘텐츠 조합·표시·연출·입력을 담당한다. 복잡한 반복 계산과 게임 규칙의 권위 있는 구현 위치는 C++로 유지한다. 이벤트 중심 구현과 함수 활용의 근거: [Blueprint Best Practices](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-best-practices-in-unreal-engine).

| 항목 | 프로젝트 규칙 |
|---|---|
| Event Graph | 이벤트 진입·흐름 연결 중심. 책임이 바뀌거나 재사용되는 처리 단위를 이름 있는 함수로 분리 |
| 함수·지역 변수 | 재사용 동작은 함수, 임시 계산은 지역 변수. 전역 멤버에 임시값을 공유하지 않음 |
| Macro·latent 흐름 | 함수로 표현하기 어려운 실행 핀 흐름 등 필요한 경우만 macro 사용. Delay/비동기 작업은 취소·중복 실행 조건을 명시 |
| 그래프 배치 | 실행 흐름을 좌→우, 관련 계산을 가까이 배치. 필요한 reroute로 교차를 줄이고 주석 박스에는 처리 목적·권한을 표시 |
| 노출 변수 | C++와 같은 이름 원칙. Instance Editable/Expose on Spawn은 실제 제작 입력만 허용하고 Category·Tooltip·기본값 지정 |
| 참조·통신 | 반복 Cast 체인 대신 적합한 직접 참조·interface·dispatcher 사용. 순환 참조·불필요한 hard reference와 로드 의존 확인 |
| 위젯 | 입력 의도를 Controller로 전달. 재화·전투 상태 직접 수정 금지. 상태 변경 이벤트로 표시를 갱신 |
| Construction Script | 편집기에서 반복 실행됨을 고려한 배치·미리보기만 처리. 저장·네트워크 명령·게임 진행 상태 변경 금지 |
| Level Blueprint | 해당 맵의 임시 연결만 사용. 재사용 게임 규칙·저장·경제 처리 금지 |
| 개발 임시물 | 미사용 노드·끊긴 선·임시 Print String 정리. 테스트 전용 출력은 빌드에서 제외되는 경로 사용 |

## 6. 변경별 검수

1. C++ 변경: 포맷 검사 후 해당 Editor 타깃을 빌드한다. UHT/UPROPERTY 변경이면 Unreal 빌드 결과로 검증하고, 관련 Blueprint 자식의 컴파일·기본값·참조를 확인한다.
2. Blueprint 변경: 수정 에셋과 관련 자식을 Compile/Save하고 경고를 확인한다. 해당 맵 PIE에서 입력·실패·중복 동작을 확인한다. 협동 권한에 영향이 있으면 서버와 클라이언트를 함께 검수한다.
3. 에셋 이동·이름 변경: 참조와 redirector를 확인하고 관련 맵 로드·쿠킹 결과를 기록한다. 파일 탐색기로 `.uasset` 이름을 바꾸지 않는다.
4. 데이터 변경: 생성기·스키마·소비 코드·QA의 일치와 `node tools/check-project.mjs` 결과를 남긴다. 임포트 구현이 있으면 UE 재임포트·참조 검사도 수행한다.
5. PR에는 실행한 명령·빌드/맵/기기·결과·미실행 이유를 적는다. [PR 양식](../../.github/pull_request_template.md)과 [실행 기록](../production/TEST_RUNS.md)을 사용한다.

현재 GitHub CI는 C++ 포맷과 문서·생성 데이터의 정적 검사다. Blueprint 그래프·리플렉션 의미·메모리 수명·권한·UE 빌드 성공은 자동 판정하지 않는다.
