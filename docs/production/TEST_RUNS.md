# 검수 실행 기록

[기획 허브](../README.md) · [QA 기대 결과](../BACKLOG_QA.md) · [작업 보드](BOARD.md)

기대 결과와 실제 관찰을 구분합니다. 결과는 Pass/Fail/NotRun으로 기록하고, 구현 작업 Done의 근거로 사용할 때 실행한 범위를 확인합니다.

## RUN-20260911-01 · 기획 원본 분리

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: 기존 GDD의20개 장을10개 기능·주제 원본으로 분리.
- 결과: **Pass**. 일회성 이관 스크립트에서 원본 장1~20과 이동 후 본문을 비교했고 누락·중복 없이 일치함을 확인. 이동에 필요한 상대 링크만 보정.
- 이후 의도적 변경: 결정·가정에 ID 부여, 관리 방식 추가, 현재 프로젝트 상태 명시. 게임 수치는 변경하지 않음.
- 관찰 출력: `preservedChapters=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20], editableDocuments=10`.
- Git 상태: 로컬 미커밋. UE 컴파일·플레이 테스트 미실행.

## RUN-20260911-02 · 문서와 데이터 검사

- 실행일: 2026-09-11 / 실행자: Codex.
- 환경: Windows PowerShell, Node.js, 현재 프로젝트 루트.
- 결과: **Pass**. 문서 원본10개, 개발 작업33개, QA 시나리오71개의 참조와 읽기본 최신 여부를 확인했고 오류0건. 기존 초기 데이터 검사도 통과.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, `node tools/validate-design-data.mjs`.
- 범위: 문서 메타데이터·링크·보드/QA 참조·읽기본 최신 여부 및 기존 초기 데이터 일관성.
- 데이터 RulesVersion: 0.1.0. 실제 게임 로직과 문서가 일치하는지는 이번 검사 범위에 포함하지 않음.
- 증거: [검사 실행 로그](evidence/RUN-20260911-02.txt), [데이터 검사 보고서](../VALIDATION_REPORT.md).

## RUN-20260911-03 · 에디터·자동화 확장 설계 문서

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: 에디터·자동화 별도 Draft 문서 2개, 기획 허브·관리 방식·변경 이력 연결.
- 결과: **Pass**. 문서 링크·표 형식·기존 작업/QA 참조·전체 GDD 최신 여부 검사에서 오류 0건. 새 문서 2개의 메타데이터·제목·코드 블록·문자 인코딩도 별도 읽기 전용 검사 통과.
- 관찰: 새 문서별 기능 6개, 완료 조건 9개. 기존 GDD 원본 10개·20장, 작업 33개, QA 71개 유지. 자동화 문서의 첫 회귀 10개는 기존 QA를 참조한 제안이며 실행한 게임 테스트가 아님.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 새 문서의 메타데이터·구조를 확인한 일회성 Node.js 검사.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-03.txt).
- Git 상태: 로컬 미커밋. 게임 코드·데이터 수치·원본 위치 이관·CI/예약 실행 설정은 이번 작업 범위에 포함하지 않음. UE 컴파일·플레이·기기 검수 미실행.

## RUN-20260911-04 · 프로젝트 출발점 결정 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-008, 개발 환경 안내, 제품·기술 명세, 첫날 계획·TASK-CORE-01, 허브·관리 방식·변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건. 새 환경 안내의 메타데이터와 설치 관찰값을 확인.
- 관찰: 전체 GDD는 기존 원본 10개·20장, 작업 33개, QA 71개를 유지. 신규 프로젝트 생성 전제와 기존 소스 경로 표기를 현재 프로젝트 유지 결정에 맞춤.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문구 검색 및 문서 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-04.txt).
- 검증 경계: 설치 환경은 이전 읽기 전용 조사에서 확인한 관찰값이다. 이번 변경에서 UE 컴파일·게임 실행·PC/Android 패키징은 미실행. 작업 보드의 구현 상태는 변경하지 않음.

## RUN-20260911-05 · 엔진 기준 후보와 고정 절차 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-009, 개발 환경 안내의 엔진 후보·고정 조건, 제품 기준, OPEN-002, 첫날 계획·PC/Android 작업 조건, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건.
- 관찰: UE 5.8.2 후보 선택과 양 플랫폼 빌드 통과 후 고정 절차를 사용자 확정으로 기록. 실제 PC·Android 빌드는 NotRun, 최종 버전 고정은 검증 대기로 구분.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문구 검색 및 문서 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-05.txt).
- 검증 경계: UE 컴파일·쿠킹·패키징·기기 실행은 미실행. 프로젝트·엔진 설정과 구현 작업 상태는 변경하지 않음.

## RUN-20260911-06 · Windows 권장 개발 도구 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-010, 개발 환경 안내의 Windows 권장 버전·근거·적용 절차, 제품·기술 명세, OPEN-002, TASK-CORE-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건.
- 관찰: Windows 도구의 사용자 확정 기준과 기존 설치 현황을 구분. MSVC 설치 디렉터리와 실제 컴파일러 버전을 함께 기록. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문서 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-06.txt).
- 검증 경계: 개발 도구 설치·빌드 설정 변경·UE 컴파일·쿠킹·패키징·기기 실행은 미실행. Android 도구 조합은 이 결정에 포함하지 않음.

## RUN-20260911-07 · Android 개발 도구 검증 기준 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-011, 개발 환경 안내의 Android 버전·근거 차이·적용 확인 조건, 제품·기술 명세, OPEN-002, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건.
- 관찰: Android 도구의 사용자 확정 기준과 설치 관찰값을 구분하고 UE 5.8.2 SDK 메타데이터 값을 재확인. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문서 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-07.txt).
- 검증 경계: 도구 설치·프로젝트 설정 변경·UE 컴파일·쿠킹·패키징·기기 실행은 미실행. 최소·대상 SDK 및 모바일 렌더링 설정은 후속 결정.

## RUN-20260911-08 · PowerShell 빌드·실행 절차 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-012, TECH-BUILD 신규 문서, 개발 환경 안내·기술 명세·허브·관리 방식·TASK-CORE-01/TASK-MOB-01·변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건. 새 문서의 PowerShell 코드 블록 10개를 파서로 검사해 문법 오류 0건.
- 관찰: UE 5.8.2 소스에서 Windows/Android 도구 선택 키, 프로젝트 생성 옵션, UAT 인자와 설치 스크립트 동작 확인. 실행 순서·산출물·로그·단계별 판정 기준을 문서화. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, PowerShell `Parser.ParseInput`을 통한 문서 코드 블록 정적 검사 및 문서 검토.
- 증거: [문서·구문 검사 실행 로그](evidence/RUN-20260911-08.txt).
- 검증 경계: 파서는 명령을 실행하지 않았으며, 도구 설치·환경 변수/프로젝트 설정 적용·UE 컴파일·쿠킹·패키징·기기 실행은 모두 미실행. 실제 명령의 성공은 후속 환경 적용·플랫폼별 실행으로 확인해야 함.

## RUN-20260911-09 · 모바일 렌더링 방향 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-013, 기술·제품·아트 명세, 개발 환경 안내의 현재값·적용 목표·검증 조건, 빌드 안내, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건.
- 관찰: Mobile Forward·단순 베이크 조명 방향을 사용자 확정으로 기록. 현재 Config의 Mobile Deferred·정적 조명 비허용 등과 적용 목표를 구분. UE 5.8.2 RendererSettings.h와 Epic 안내에서 설정 의미·재시작 조건을 확인. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문서·설정의 읽기 전용 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-09.txt).
- 검증 경계: Config·맵·머티리얼 변경, 셰이더 재컴파일·조명 베이크·UE 빌드·기기 실행은 미실행. 렌더링 적용·표시·성능 결과는 NotRun이며 문서 검사로 대체하지 않음.

## RUN-20260911-10 · 화면·입력 기준 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-014, 전장·UI 및 제품 명세, 개발 환경·빌드 안내, TASK-MAP-01/TASK-UI-01, QA-VIS-02/QA-MOB-04, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사에서 오류 0건.
- 관찰: 1080×1920 설계 기준과 실제 해상도를 구분. 세로 화면비·SafeArea·DPI에 따른 HUD/카메라 맞춤과 터치 검증 조건을 기록. 프로젝트 Config의 화면 방향 미지정 및 입력 설정을 읽기 전용 확인. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문서·UE Android 방향 설정 정의 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-10.txt).
- 검증 경계: Config·코드·위젯·카메라 변경, 패키징·기기 실행은 미실행. 화면비·안전 영역·실제 터치 결과는 NotRun이며 문서 검사로 대체하지 않음.

## RUN-20260911-11 · 초기 Android 데이터 포함 APK 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-015, 개발 환경·빌드 안내, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건. 빌드 안내의 PowerShell 블록 9개 구문 검사 오류 0건.
- 관찰: UE 5.8.2의 Android 설정 키·엔진 기본값과 프로젝트 미지정 상태를 읽기 전용 확인. Development 데이터 포함 APK의 생성·단독 설치·맵 실행 기준으로 절차 정합화. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, PowerShell `Parser.ParseInput` 및 문서 검토.
- 증거: [문서·구문 검사 실행 로그](evidence/RUN-20260911-11.txt).
- 검증 경계: 구문 검사는 예시 명령을 실행하지 않았으며 Config 변경·UE 패키징·기기 설치/실행은 미실행. 실제 APK 데이터 포함·설치·맵 실행 결과는 NotRun.

## RUN-20260911-12 · Android 검증 기기 미정 기록

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: OPEN-008, OPEN-003의 구분, 개발 환경 안내, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건.
- 관찰: 사용자의 기기·OS 미정 답변을 기록하고 기존 비교 기기를 실제 선정으로 취급하지 않도록 명시. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 관련 문서 검토.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-12.txt).
- 검증 경계: 기기 선정·구매, 프로젝트 설정·패키징·실기기 실행은 미실행. 새 호환성 값은 확정하지 않음.

## RUN-20260911-13 · Android ARM64 단일 ABI 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-016, 개발 환경·빌드 안내, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건. 빌드 안내의 PowerShell 블록 10개 구문 검사 오류 0건.
- 관찰: ARM64 활성·x86_64 비활성 설정 키와 기존 파일 관찰을 구분. 최종 APK 네이티브 라이브러리의 ABI 목록과 기기의 지원 ABI 확인 절차 추가. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, PowerShell `Parser.ParseInput` 및 문서 검토.
- 증거: [문서·구문 검사 실행 로그](evidence/RUN-20260911-13.txt).
- 검증 경계: 구문 검사는 예시를 실행하지 않았으며 Config 변경·APK 생성/내부 검사·기기 설치/실행은 미실행. 실제 ARM64 패키징 결과는 NotRun.

## RUN-20260911-14 · Android ETC2 단일 쿠킹 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-017, 개발 환경·빌드 안내, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건. 빌드 안내의 PowerShell 블록 10개 구문 검사 오류 0건.
- 관찰: UAT 명령을 -cookflavor=ETC2로 명시하고 단일 쿠킹 대상과 자산별 내부 포맷·Multi 설정을 구분. ETC2 쿠킹 로그·APK 연결과 기기 표시 검증 조건 추가. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, PowerShell `Parser.ParseInput`, UE 대상 정의·문서 검토.
- 증거: [문서·구문 검사 실행 로그](evidence/RUN-20260911-14.txt).
- 검증 경계: 구문 검사는 예시를 실행하지 않았으며 Config·텍스처 자산 변경, 실제 쿠킹·APK 생성·기기 실행은 미실행. 실제 ETC2 쿠킹·표시 결과는 NotRun.

## RUN-20260911-15 · 초기 Android 설정 묶음 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-018, 개발 환경·빌드 안내, 기술 명세, TASK-MOB-01, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건. 빌드 안내의 PowerShell 블록 11개 구문 검사 오류 0건.
- 관찰: OpenGL ES 3.2·HDR On·MSAA 2x·최소 SDK 26·대상 SDK 36·개발 앱 식별자/표시 이름의 현재값과 적용 목표를 구분. 최종 APK 앱 정보·Manifest 검사와 실제 RHI/HDR/MSAA 검증 조건 연결. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, PowerShell `Parser.ParseInput`, UE 설정 정의·AAPT2 공식 명령 안내 및 문서 검토.
- 증거: [문서·구문 검사 실행 로그](evidence/RUN-20260911-15.txt).
- 검증 경계: 구문 검사는 예시 명령을 실행하지 않았으며 도구 설치·Config/코드/에셋 변경·UE 빌드·APK 검사·기기 실행은 미실행. 실제 빌드·호환성·표시·성능 결과는 NotRun. 1번 주요 문서 결정 정리와 개발 작업 완료는 구분.

## RUN-20260911-16 · P0 범위·완료 기준 묶음 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-019, 제품·전투·경제·유닛·UI 명세, 데이터 적용 안내, 첫 10일 계획·백로그·QA·로드맵·작업 보드 안내, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건.
- 관찰: 8종·10웨이브·보스 1종 유지. 강화·희귀 고유 스킬·정식 튜토리얼은 P1로 정합화. P0 모드의 스킬 미발동·강화 구매 거절과 짧은 안내·미구현 버튼 숨김을 수용 기준에 반영. PC 2인 검증과 Android 실기기 터치 완주를 P0 필수 조건으로 연결. 작업 33개·QA 71개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 범위·일정·QA·기존 데이터 필드의 읽기 전용 대조.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-16.txt).
- 검증 경계: 코드·Config·에셋·데이터 JSON 변경, UE 빌드·전투·기기 검사는 미실행. 작업 상태·담당자는 유지. P0 실제 기능·완주 결과는 NotRun이며 문서 검사로 대체하지 않음.

## RUN-20260911-17 · 명령 payload·중복 처리 계약 반영

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-020, 기술 명세 16.2, 데이터 런타임 계약·경제·UI·자동화·백로그/QA·작업 보드 안내, 변경 이력 및 전체 GDD 읽기본.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건.
- 관찰: 공통 요청과 9개 전용 payload, 합성 선택 기준·이동 모드·원하는 설정값, 캐시·처리 중 요청·별도 번호 상한, 내용 충돌·만료·상태 동기화·늦은 응답 규칙 정합화. QA 5개 추가로 총 76개, 작업 33개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 명령·UI·데이터·QA 계약의 문서 대조.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-17.txt).
- 검증 경계: C++/RPC·Config·에셋·JSON·작업 상태 변경 및 UE 빌드·네트워크 재전송 테스트는 미실행. 실제 동작은 NotRun이며 문서 검사 통과로 대체하지 않음. 일부 이동·교환의 배치 예외는 다음 결정으로 남김.

## RUN-20260911-18 · 원작 재현 우선·1차 문서 대조

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-021·OPEN-009, 원작 대조 기록, 제품·보드/UI·경제·기술·데이터 적용 안내·백로그/QA·작업 보드·허브·변경 이력·전체 GDD.
- 결과: **Pass**. 문서 링크·표·원본 메타데이터·작업/QA 참조·읽기본 최신 여부 검사 오류 0건.
- 관찰: 공식 소개와 외부 자료를 구분하고 재료 표시/소비 잠금 혼동, 자체 교환·개별 이동 버튼·타깃 토글·수치의 근거 부족을 명시. 작업 33개·QA 76개 유지.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 원작 근거와 명세 적용 우선순위의 문서 대조.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-18.txt). 원작 자료 링크·한계는 [대조 기록](../product/ORIGINAL_REFERENCE.md) 참조.
- 검증 경계: 원작 앱·영상 장면의 직접 동작 검수, UE 빌드·게임 기능 테스트는 미실행. 코드·Config·에셋·JSON·작업 상태 변경 없음. 정적 검사 통과를 원작 재현 성공으로 판정하지 않음.

## RUN-20260911-19 · 원작 공개 화면 2차 대조

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: 원작 대조 OBS-01~05, 보드·경제 명세, 변경 이력·전체 GDD.
- 결과: **Pass**. 문서 정적 검사 오류 0건. 공개 스크린샷의 관찰 항목 5개 기록.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`, 브라우저에서 S7 그림 4·7·8 및 S2 Tip #1 이미지 확인.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-19.txt), [화면 위치·관찰 내용](../product/ORIGINAL_REFERENCE.md). 스크린샷 원본은 외부 출처에 있으며 저장소에는 복제하지 않음.
- 검증 경계: 정지 화면은 이동·합성 실행 결과의 증거가 아님. 원작 앱 실행·현재 버전 일치·UE 빌드·게임 QA는 미실행. 작업 33개·QA 76개와 데이터 형식/수치 유지.

## RUN-20260911-20 · 원작 모드 선택·영상 동작 대조

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: DEC-022·OPEN-009, 원작 대조 OBS-06~09, 제품·보드·경제·허브·변경 이력·전체 GDD.
- 결과: **Pass**. 문서 정적 검사 오류 0건. 영상 관찰 항목 4개와 공식 스토어 표시 버전 2.0.11 기록.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`. 브라우저의 재생/정지·프레임 이동으로 원작 대조 기록에 지정한 S8 구간 확인.
- 증거: [검사 실행 로그](evidence/RUN-20260911-20.txt), [영상 위치와 전후 관찰](../product/ORIGINAL_REFERENCE.md). 영상 파일은 저장소에 복제하지 않음.
- 검증 경계: 영상은 보통 모드이나 앱 버전·OS 미상. 재생 속도·편집으로 전투 타이머와 입력 횟수는 검증하지 않음. 원작 앱 직접 조작·현재 Android 일치·UE 빌드·게임 QA는 미실행. 작업 33개·QA 76개 유지.

## RUN-20260911-21 · 화상 마지막 틱·만료 문서 검토

- 실행일: 2026-09-11 / 실행자: Codex.
- 범위: 전투·유닛·데이터 명세, QA-DMG-11~14, P1 전투 작업 연결, 자동화 설계, 변경 이력·전체 GDD.
- 결과: **Pass**. 문서 정적 검사 2,707개, 오류 0건. QA 총 80개. 1·2·3초 예약과 3초 만료의 문서상 충돌을 정리했으며 런타임 관찰 결과는 아님.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260911-21.txt).
- 검증 경계: 원작 화상 규칙 대조·UE 빌드·게임 QA는 미실행. QA 4개는 향후 실행 기준으로 추가. 코드·Config·에셋·JSON·RulesVersion·작업 33개의 상태 유지.

## RUN-20260912-01 · 에디터·자동화 계약 문서 검토

- 실행일: 2026-09-12 / 실행자: Codex.
- 범위: 에디터/자동화 원본, 작업·QA·보드 연결, 허브·변경 이력.
- 결과: **Pass**. 문서 정적 검사 2,875개, 오류 0건. 작업 37개·QA 88개 연결 확인. 저장·적용·실행 계약과 실패 복구 경로는 문서 검토이며 실제 구현 관찰 결과가 아님.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`. 현재 생성기·데이터 검증기의 파일 쓰기 경로는 코드 읽기로 확인.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260912-01.txt).
- 검증 경계: 신규 API·CLI·에디터·임포트 복구·UE 게임 QA는 미구현 또는 미실행이며 통과 판정하지 않음. 도구 작업 4개·QA 8개를 계획으로 추가. 기존 작업 상태·게임 데이터·P0 범위 유지.

## RUN-20260913-01 · 개발 전 문서 검토 종합

- 실행일: 2026-09-13 / 실행자: Codex.
- 범위: 종합 문서·기획 허브·작업 보드·로드맵·변경 이력·전체 GDD.
- 결과: **Pass**. 문서 정적 검사 2,971개, 오류 0건. 6개 검토 항목의 문서 반영과 실제 검증을 구분하고 OPEN-001~009의 확인 시점을 관련 작업에 연결.
- 검사: `node tools/build-planning.mjs`, `node tools/validate-planning.mjs`. 결정·원작 대조·환경·저장·기존 검수 기록의 문서 대조.
- 증거: [문서 검사 실행 로그](evidence/RUN-20260913-01.txt).
- 검증 경계: 현재 설치 환경 재조사·추가 원작 검수·UE 빌드·게임 QA 미실행. 기존 작업 37개 상태·QA 88개·코드·Config·에셋·JSON 유지. 정적 Pass는 게임 구현 준비 전체의 통과가 아님.

## RUN-20260913-02 · TASK-CORE-01 환경 적용·Windows 빌드

- 기록 판정: Fail — 전체 외부 빌드 스크립트 결과. 단계별 성공은 아래 원래 기록 참조.
- 실행일: 2026-09-13 / 실행자: Codex.
- 입력: 변경 전 606개·142,091,676바이트를 `Saved/BuildRuns/RUN-20260913-02/Input/`에 보존, 파일별 SHA-256 확인. 실제 빌드는 도구 선택 설정 적용 후 별도 `Saved/BuildRuns/20260913-030434-725/Workspace/` 사용.
- 환경: UE 5.8.2 CL 56702186, VS Community 2026 18.10.12201.205 Stable, MSVC 디렉터리 14.50.35717 / 실제 14.50.35738.0, Windows SDK 10.0.26100.0, UE 동봉 .NET SDK 10.0.203. 설치 종료 코드 0·재부팅 요구 없음.
- 명령: `pwsh -NoProfile -File tools/Invoke-PrototypeBuild.ps1 -Stage Windows -CompilerVersion 14.50.35717`. 맵 `/Game/TopDown/Lvl_TopDown`, Win64 Development, 쿠킹·pak·stage·archive.
- 결과: 프로젝트 파일 생성·Editor 컴파일·UAT Windows 패키징 **Pass**(UAT ExitCode=0). 외부 빌드 스크립트는 잘못된 `Windows/Windows/` 산출물 경로 가정으로 **Fail**을 반환. 원래 `result.json`은 보존하고 수정한 검사·실행 검수는 RUN-20260913-03에 기록.
- 증거: [실행 요약](evidence/RUN-20260913-02.txt). 전체 입력·패키지는 위 `Saved/BuildRuns/`에 보관. 명령·상세 로그·결과·입력 해시는 2026-09-14 [보관 목록](evidence/archive-manifest.json)에 따라 저장소로 복사했다.
- 검증 경계: 기존 TopDown 템플릿 기본 빌드 검증. 디펜스 기능·PC 협동·Android 패키징/실기기·성능 검수 아님. 인증된 adb 기기는 0개이며 엔진 최종 고정·P0 완료는 보류.

## RUN-20260913-03 · Windows 산출물 재검사·기본 맵 실행

- 기록 판정: Pass — TASK-CORE-01의 Windows 기본 빌드·입력·종료 범위.
- 실행일: 2026-09-13 / 실행자: Codex / 선행 RUN-20260913-02.
- 수정: `Test-WindowsPackage.ps1`로 실제 archive 루트 및 선택적 Windows 하위 폴더를 판별. 부트스트랩·런타임·쿠킹 콘텐츠 존재/크기/해시 검사. 비패키지 폴더를 넣은 거절 검사도 통과. 빌드 스크립트가 같은 검사를 호출하도록 연결.
- 산출물 재검사: **Pass**. `Saved/BuildRuns/20260913-030434-725/Packages/Windows/Mobile_defense_clone.exe`, 내부 런타임 332,111,872바이트와 쿠킹 컨테이너 확인. 게임 전체 폴더가 실행 산출물이며 exe만 단독 배포하지 않음.
- 화면·입력: **Pass**. Windows Computer Use로 패키징 게임의 기본 맵·캐릭터 표시 → 바닥 클릭 후 원형 시작대에서 이동 → Alt+F4 종료 확인. Editor에서 기본 맵 → Play → 클릭 이동 → Stop → Editor 종료 확인. 양쪽 실행 로그의 `LogExit: Exiting.` 확인.
- 추가 진단: 한국어 Editor 시작에 `LogAutomationTest: Error: Condition failed` 15건. 동일한 `-nullrhi -unattended -nosound -seconds=3`에 culture만 바꾼 진단에서 ko=15, en=0, 둘 다 프로세스 종료 코드 0. 설치 엔진 `Core/Tests/Experimental/UnifiedError/UnifiedErrorTests.cpp`의 영어 문자열 비교와 `LowLevelTestAdapter.h`의 Condition failed 출력 경로를 확인. 언어 의존 엔진 테스트 문제로 판단하며 엔진 코드를 수정하거나 검사를 비활성화하지 않음.
- 판정 범위: TASK-CORE-01의 Windows 기본 빌드·입력·종료 완료 조건 충족. 엔진 스모크 전체 Pass 또는 Android/디펜스/P0 완료 판정 아님. 한국어 엔진 진단 문제는 최종 엔진 적합성 검토 시 재확인.
- 증거: [요약·관찰 기록](evidence/RUN-20260913-03.txt), [산출물 크기·SHA-256](evidence/RUN-20260913-03-artifacts.json). 전체 `editor.log`, `windows-package.log`, `editor-play.log`, `windows-game.log`, `editor-smoke-ko/en.log`, `runtime-observations.json`은 원래 빌드 실행 폴더 및 [증거 보관 목록](evidence/archive-manifest.json)에 보관. [조건별 완료 판정](verification.json)은 2026-09-14 기존 기록 대조 결과다.
- 보존 확인: 원본 Source/Content 533개 파일의 SHA-256 변경 0건. 문서 정적 검사 결과는 실행 요약에 기록.

## RUN-20260914-01 · 문서 운영 보완 검증

- 실행일: 2026-09-14 / 실행자: Codex.
- 범위: DEC-023, 문서 상태·초안 예시·담당/검토일, 검사 도구와 CI 설정, 기존 증거 보관, 로컬 Git 기준점.
- 결과: **Pass** — 생성 데이터·검증 보고서 최신 여부, 문서/메타데이터·현황·완료 증거·보관 해시 검사, 검사기 회귀 테스트 15개, actionlint v1.7.12의 CI 문법 검사를 통과했다. 세부 검사 수는 아래 로그를 따른다.
- 증거: [검사 실행 로그](evidence/RUN-20260914-01.txt).
- 검증 경계: 기존 Source·Content·게임 데이터의 의미 변경 없음. AFS 토큰은 Config/UserEngine.ini 로컬 계층으로 분리하며 새 Android/UE 실행 검증으로 표시하지 않음. 원격 push·GitHub Actions 실제 실행·필수 검사 설정은 미수행.

## 게임 기능·기기 테스트

현재 이 기록에는 전투 구현, 소환 RPC, Android/iOS 패키징, 모바일 성능, 온라인 재접속의 실제 실행 증거가 등록되지 않았습니다. 기존 프로젝트의 구현 여부는 작업을 착수하며 확인합니다. 데이터·문서 검사 통과만으로 해당 작업을 Done 처리하지 않습니다.

추가 실행 기록은 [템플릿](../TEMPLATES.md)의 Run 양식으로 작성합니다. 이전 실패 결과를 지우지 않고 수정 후 재실행을 새 Run ID로 남깁니다.
